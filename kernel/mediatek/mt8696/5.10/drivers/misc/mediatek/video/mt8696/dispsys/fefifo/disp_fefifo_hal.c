/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define LOG_TAG "FEFIFO_HAL"

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include "disp_hw_log.h"
#include "disp_hw_mgr.h"
#include "disp_info.h"
#include "disp_clk.h"
#include "disp_irq.h"
#include "disp_path.h"
#include "fmt_def.h"
#include "fmt_hal.h"
#include "disp_fefifo_hw.h"
#include "disp_fefifo_debug.h"
#include "disp_fefifo_hal.h"
#include "disp_fefifo_drv.h"


struct disp_fefifo_conf_reg _disp_fefifo_sw_base[FE_FIFO_MAX_LAYER_NUM];
struct FEFIFO_Register_Table _disp_fefifo_conf_table[FE_FIFO_MAX_LAYER_NUM];
struct disp_fefifo_context *_pt_fefifo_inst[FEFIFO_MAX_LAYER_NUM];

uint32_t iFEFIFOReadREG32(uint32_t reg32, uint32_t client)
{
	uintptr_t reg = 0;
	uint32_t value = 0;
	uintptr_t hw_base_addr = 0;

	hw_base_addr = _pt_fefifo_inst[client]->fefifo_hw_base;
	if (hw_base_addr != 0) {
		reg = hw_base_addr + reg32;
		value = ReadREG32(reg);
	}
	return value;
}

int vFEFIFOWriteFldAlign(uint32_t addr, uint32_t val, uint32_t msk,
	uint32_t conf_mode, uint32_t client)
{
	uintptr_t reg_va = 0;
	uint32_t reg_pa = 0;
	uint32_t u4Count = 0;
	uintptr_t hw_base_addr = 0;
	struct FEFIFO_Register_Table *ptRegTable = NULL;
	union disp_fefifo_hal_union *pfefifo_hw_reg = NULL;

	if (_pt_fefifo_inst[client] == NULL)
		return FEFIFO_RET_UNINIT;

	hw_base_addr = _pt_fefifo_inst[client]->fefifo_hw_base;
	ptRegTable = &_disp_fefifo_conf_table[client];
	pfefifo_hw_reg = (union disp_fefifo_hal_union *)hw_base_addr;

	if (client == 0)
		reg_pa = DISP_FEFIFO_VDOFE0_REG_BASE;
	else if (client == 1)
		reg_pa = DISP_FEFIFO_VDOFE1_REG_BASE;
	else if (client == 2)
		reg_pa = DISP_FEFIFO_GFXFE0_REG_BASE;
	else if (client == 3)
		reg_pa = DISP_FEFIFO_GFXFE1_REG_BASE;
	else {
		fefifo_error("invalid fefifo client[%d]\n", client);
		return FEFIFO_RET_INV_ARG;
	}

	if ((hw_base_addr == 0) || !(_pt_fefifo_inst[client]->inited))
		return FEFIFO_RET_UNINIT;

	switch (conf_mode) {
	case FEFIFO_REG_CONF_CLIENT_RIU:
		reg_va = hw_base_addr + addr;
		fefifo_info("RIU[0x%p 0x%x 0x%x]\n", (void *)reg_va, val, msk);
		WriteREG32Msk(reg_va, val, msk);
		break;

	case FEFIFO_REG_CONF_CLIENT_RIU_SHDW:
		if (pfefifo_hw_reg->field.shdw_en) {
			reg_va = hw_base_addr + addr;
			WriteREG32Msk(reg_va, val, msk);
		} else {
			reg_va = hw_base_addr + addr;
			WriteREG32Msk((hw_base_addr+FEFIFO_SHDW_CFG),
				0x1, FE_SHDW_EN_FLD);
			WriteREG32Msk(reg_va, val, msk);
		}
		break;

	case FEFIFO_REG_CONF_CLIENT_SW_SHDW:
		_disp_fefifo_sw_base[client].fefifo_reg.reg[addr/4] &=
			(~((uint32_t)(msk)));
		_disp_fefifo_sw_base[client].fefifo_reg.reg[addr/4] |=
			(val & msk);
		_disp_fefifo_sw_base[client].fefifo_reg_mask[addr/4] |= msk;
		break;

	case FEFIFO_REG_CONF_CLIENT_GCE:
		reg_pa = reg_pa + addr;
		u4Count = ptRegTable->depth;
		if (u4Count < HAL_FEFIFO_REG_CONF_CNT) {
			ptRegTable->address[u4Count] = reg_pa;
			ptRegTable->value[u4Count] = val;
			ptRegTable->mask[u4Count] = msk;
			ptRegTable->depth++;
			fefifo_info("GCE[0x%x 0x%x 0x%x %d]\n",
				reg_pa, val, msk, u4Count);

		} else
			fefifo_error("[FEFIFO][%s][%d]count = %d overflow\n",
			__func__, __LINE__, u4Count);
		break;

	case FEFIFO_REG_CONF_CLIENT_ML:
		reg_pa = reg_pa + addr;
		u4Count = ptRegTable->depth;
		if (u4Count < HAL_FEFIFO_REG_CONF_CNT) {
			ptRegTable->address[u4Count] = reg_pa;
			ptRegTable->value[u4Count] = val;
			ptRegTable->mask[u4Count] = msk;
			ptRegTable->depth++;
			fefifo_info("ML[0x%x 0x%x 0x%x %d]\n",
				reg_pa, val, msk, u4Count);
		} else
			fefifo_error("[FEFIFO][%s][%d]count = %d overflow\n",
			__func__, __LINE__, u4Count);
		break;

	default:
		fefifo_error("[FEFIFO]config reg fail m=%d overflow\n",
			conf_mode);
		break;
	}
	return FEFIFO_RET_OK;
}

void fefifo_hal_clr_isr(uint32_t layer_id, uint32_t irq)
{

}

int fefifo_hal_isr(uint32_t layer_id, uint32_t conf_mode)
{
	uint32_t idx = 0;
	uint32_t u4Count = 0;
	uintptr_t hw_base_addr = 0;
	uint32_t hw_pa_base_addr = 0;
	struct FEFIFO_Register_Table *ptRegTable = NULL;
	union disp_fefifo_hal_union *pfefifo_hw_reg = NULL;

	if (_pt_fefifo_inst[layer_id] == NULL)
		return FEFIFO_RET_UNINIT;

	hw_base_addr = _pt_fefifo_inst[layer_id]->fefifo_hw_base;
	hw_pa_base_addr = _pt_fefifo_inst[layer_id]->io_reg_base;
	pfefifo_hw_reg = (union disp_fefifo_hal_union *)
		_pt_fefifo_inst[layer_id]->fefifo_hw_base;
	ptRegTable = &_disp_fefifo_conf_table[layer_id];

	if ((hw_base_addr == 0) || (hw_pa_base_addr == 0))
		return FEFIFO_RET_UNINIT;

	if (conf_mode == FEFIFO_REG_CONF_CLIENT_RIU_SHDW)
		WriteREG32Msk((hw_base_addr + FEFIFO_SHDW_CFG), (0x1 << 1),
		FE_SHDW_TRIG_FLD);
	else if (conf_mode == FEFIFO_REG_CONF_CLIENT_SW_SHDW) {
		for (idx = 0; idx < HAL_FEFIFO_REG_NUM; idx++) {
			pfefifo_hw_reg->reg[idx] =
			_disp_fefifo_sw_base[layer_id].fefifo_reg.reg[idx];
			_disp_fefifo_sw_base[layer_id].fefifo_reg_mask[idx] = 0;
		}
	} else if (conf_mode == FEFIFO_REG_CONF_CLIENT_GCE) {
		u4Count = ptRegTable->depth;
		for (idx = 0; idx < u4Count; idx++) {
			if ((ptRegTable->address[idx] >=
				(uint32_t)hw_pa_base_addr)
			&& (ptRegTable->address[idx] <=
			((uint32_t)hw_pa_base_addr + 0x1000))) {
				//call gce write api
				// call
			}
		}
		fefifo_default("GCE fefifo[%d] flush cmd:%d",
			layer_id, u4Count);
		ptRegTable->depth = 0;
	} else if (conf_mode == FEFIFO_REG_CONF_CLIENT_ML) {
		u4Count = ptRegTable->depth;
		for (idx = 0; idx < u4Count; idx++) {
			if ((ptRegTable->address[idx] >=
				(uint32_t)hw_pa_base_addr)
			&& (ptRegTable->address[idx] <=
			((uint32_t)hw_pa_base_addr + 0x1000))) {
				//call gce write api
			}
		}
		fefifo_default("ML fefifo[%d] flush cmd:%d\n",
			layer_id, u4Count);
		ptRegTable->depth = 0;
	}
	return FEFIFO_RET_OK;
}


void fefifo_hal_set_src_active(uint32_t layer_id, uint32_t width,
	uint32_t height, uint32_t conf_md)
{
	uint32_t conf_mode = conf_md;

	if ((width != 0) && (height != 0)) {
		vFEFIFOWriteFldAlign(FEFIFO_TIMING_CONF, width, TM_HACTIVE_FLD,
			conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TIMING_CONF, (height << 16),
			TM_VACTIVE_FLD,
			conf_mode, layer_id);
	}
}


void fefifo_hal_set_timing(uint32_t layer_id, void *data)
{
	uint32_t conf_mode = _pt_fefifo_inst[layer_id]->reg_conf_mode;
	struct disp_fefifo_timing *ptfefifo_tg = NULL;

	ptfefifo_tg = (struct disp_fefifo_timing *)data;
	if (ptfefifo_tg != NULL) {
		if (ptfefifo_tg->mode == FEFIFO_TG_MODE_RELAY) {
			vFEFIFOWriteFldAlign(FEFIFO_TIMING_CONF,
				ptfefifo_tg->width,
				TM_HACTIVE_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TIMING_CONF,
				(ptfefifo_tg->height << 16),
				TM_VACTIVE_FLD, conf_mode, layer_id);
		} else {
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG0,
				ptfefifo_tg->htotal,
				FE_PATGEN_HTOTAL_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG0,
				(ptfefifo_tg->vtotal_lsb << 16),
				FE_PATGEN_VTOTAL_LSB_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG5,
				ptfefifo_tg->vtotal_msb,
				FE_PATGEN_VTOTAL_MSB_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG1,
				ptfefifo_tg->width,
				FE_PAGTGEN_HACTIVE_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG1,
				(ptfefifo_tg->height << 16),
				FE_PAGTGEN_VACTIVE_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG3,
				ptfefifo_tg->hfront,
				FE_PAGTGEN_HFRONT_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG3,
				(ptfefifo_tg->vfront << 16),
				FE_PAGTGEN_VFRONT_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG2,
				ptfefifo_tg->hsync_w,
				FE_PAGTGEN_HSYNCW_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG2,
				(ptfefifo_tg->vsync_w << 8),
				FE_PAGTGEN_VSYNCW_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TIMING_CONF,
				ptfefifo_tg->width,
				TM_HACTIVE_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_TIMING_CONF,
				(ptfefifo_tg->height << 16),
				TM_VACTIVE_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_CTL, 0x1,
				FE_PATGEN_EN_FLD, conf_mode, layer_id);
		}
	}
}

void fefifo_hal_set_enable(uint32_t layer_id,
	bool fgEnable, uint32_t mode)
{
	uint32_t conf_mode = mode;

	if (fgEnable)
		vFEFIFOWriteFldAlign(FEFIFO_CONTROL, 0xF, CTL_CLR_BY_VS_FLD,
		conf_mode, layer_id);
	else
		vFEFIFOWriteFldAlign(FEFIFO_CONTROL, 0x0, CTL_CLR_BY_VS_FLD,
		conf_mode, layer_id);
}

void fefifo_hal_set_sof(uint32_t u4sof_s, uint32_t u4sof_e,
	uint32_t u4eof_s, uint32_t u4eof_e)
{

}

void fefifo_hal_set_alpha(uint32_t layer_id, uint32_t alpha)
{
	uint32_t conf_mode = 0;

	if (alpha == 0x100) {
		vFEFIFOWriteFldAlign(FEFIFO_CONTROL, (0x0 << 29),
			CTL_ALPHA_SEL_FLD,
			conf_mode, layer_id);
		alpha = 0xFF;
	} else
		vFEFIFOWriteFldAlign(FEFIFO_CONTROL, (0x1 << 29),
			CTL_ALPHA_SEL_FLD,
			conf_mode, layer_id);
	vFEFIFOWriteFldAlign(FEFIFO_TG_CFG4, alpha,
		FE_PAGTGEN_ALPHA_FLD, conf_mode, layer_id);
}

void fefifo_hal_set_pattern(uint32_t layer_id, bool enable,
	void *data, void *pattern)
{
	uint32_t conf_mode = 0;
	struct disp_fefifo_timing *ptfefifo_tg = NULL;
	struct disp_fefifo_pattern *ptPattern = NULL;

	ptfefifo_tg = (struct disp_fefifo_timing *)data;
	ptPattern = (struct disp_fefifo_pattern *)pattern;

	if ((ptfefifo_tg != NULL) && (ptPattern != NULL)) {
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG0, ptfefifo_tg->htotal,
			FE_PATGEN_HTOTAL_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG0,
			(ptfefifo_tg->vtotal_lsb << 16),
			FE_PATGEN_VTOTAL_LSB_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG5, ptfefifo_tg->vtotal_msb,
			FE_PATGEN_VTOTAL_MSB_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG1, ptfefifo_tg->width,
			FE_PAGTGEN_HACTIVE_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG1,
			(ptfefifo_tg->height << 16),
			FE_PAGTGEN_VACTIVE_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG3, ptfefifo_tg->hfront,
			FE_PAGTGEN_HFRONT_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG3,
			(ptfefifo_tg->vfront << 16),
			FE_PAGTGEN_VFRONT_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG2, ptfefifo_tg->hsync_w,
			FE_PAGTGEN_HSYNCW_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TG_CFG2,
			(ptfefifo_tg->vsync_w << 8),
			FE_PAGTGEN_VSYNCW_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TIMING_CONF, ptfefifo_tg->width,
			TM_HACTIVE_FLD, conf_mode, layer_id);
		vFEFIFOWriteFldAlign(FEFIFO_TIMING_CONF,
			(ptfefifo_tg->height << 16),
			TM_VACTIVE_FLD, conf_mode, layer_id);

		if (ptPattern->mode == 1) {
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_DATA0,
				ptPattern->ypattern,
				FE_PATGEN_ACTIVE_Y_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_DATA0,
				(ptPattern->cbpattern << 16),
				FE_PATGEN_ACTIVE_CB_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_DATA1,
				ptPattern->crpattern,
				FE_PATGEN_ACTIVE_CR_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_DATA1,
				(ptPattern->ypattern_b << 16),
				FE_PATGEN_BLANK_YC_FLD,
				conf_mode, layer_id);
			ptPattern->alpha = 0xFF;
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG4,
				(ptPattern->alpha << 16),
				FE_PAGTGEN_ALPHA_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_CFG, 0x1,
				FE_PATGEN_ACTIVE_CFG_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_CFG, (0x1 << 1),
				FE_PATGEN_BLANK_CFG_FLD, conf_mode, layer_id);
		} else {
			ptPattern->alpha = 0xFF;
			vFEFIFOWriteFldAlign(FEFIFO_TG_CFG4,
				(ptPattern->alpha << 16),
				FE_PAGTGEN_ALPHA_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_CFG, 0x0,
				FE_PATGEN_ACTIVE_CFG_FLD, conf_mode, layer_id);
			vFEFIFOWriteFldAlign(FEFIFO_PATGEN_CFG, (0x0 << 1),
				FE_PATGEN_BLANK_CFG_FLD, conf_mode, layer_id);
		}
		vFEFIFOWriteFldAlign(FEFIFO_PATGEN_CTL, 0x1, FE_PATGEN_EN_FLD,
			conf_mode, layer_id);

	}
	if (!enable)
		vFEFIFOWriteFldAlign(FEFIFO_PATGEN_CTL, 0x0, FE_PATGEN_EN_FLD,
			conf_mode, layer_id);

}

void fefifo_hal_set_in_swap(uint32_t layer_id, uint32_t u4SwapType)
{
	uint32_t conf_mode = 0;

	vFEFIFOWriteFldAlign(FEFIFO_CONTROL, (u4SwapType << 24),
		CTL_SRC_IN_SWAP_FLD, conf_mode, layer_id);
}

void fefifo_hal_set_shdw(uint32_t layer_id, bool enable, bool fgTrig)
{
	if (enable)
		vFEFIFOWriteFldAlign(FEFIFO_SHDW_CFG, 0x1,
		FE_SHDW_EN_FLD, 0, layer_id);
	else
		vFEFIFOWriteFldAlign(FEFIFO_SHDW_CFG, 0x0,
		FE_SHDW_EN_FLD, 0, layer_id);
	if (fgTrig)
		vFEFIFOWriteFldAlign(FEFIFO_SHDW_CFG,
			(0x1 << 1), FE_SHDW_TRIG_FLD,
			0, layer_id);
	else
		vFEFIFOWriteFldAlign(FEFIFO_SHDW_CFG,
			(0x0 << 1), FE_SHDW_TRIG_FLD,
			0, layer_id);
}

void fefifo_init_sw_reg(uint32_t layer_id, uintptr_t reg_base)
{
	uint32_t id = 0;

	memcpy((void *)_disp_fefifo_sw_base[layer_id].fefifo_reg.reg,
		(const void *)reg_base,
		sizeof(union disp_fefifo_hal_union));
	for (id = 0; id < HAL_FEFIFO_REG_NUM; id++)
		_disp_fefifo_sw_base[layer_id].fefifo_reg_mask[id] = 0;
}

void fefifo_hal_reset(uint32_t layer_id)
{
	vFEFIFOWriteFldAlign(FEFIFO_CONTROL, 0x3,
		CTL_SOFT_RST_FLD, 0, layer_id);
	vFEFIFOWriteFldAlign(FEFIFO_CONTROL, 0x0,
		CTL_SOFT_RST_FLD, 0, layer_id);
}

int fefifo_hal_init(uint32_t layer_id, uintptr_t reg_base)
{
	_pt_fefifo_inst[layer_id] = fefifo_get_inst(layer_id);
	/*Init register base */
	if (_pt_fefifo_inst[layer_id]->inited) {
		fefifo_default("fefifo hal already inited\n");
		return FEFIFO_RET_UNINIT;
	}
	//fefifo_init_sw_reg(layer_id, reg_base);
	memset(&_disp_fefifo_conf_table[layer_id],
		0, sizeof(struct FEFIFO_Register_Table));
	_pt_fefifo_inst[layer_id]->preg_tbl =
		&_disp_fefifo_conf_table[layer_id];
	return FEFIFO_RET_OK;
}

void fefifo_hal_uninit(uint32_t layer_id)
{
	/*Init register base */
	memset(&_disp_fefifo_conf_table[layer_id],
	0, sizeof(struct FEFIFO_Register_Table));
}



