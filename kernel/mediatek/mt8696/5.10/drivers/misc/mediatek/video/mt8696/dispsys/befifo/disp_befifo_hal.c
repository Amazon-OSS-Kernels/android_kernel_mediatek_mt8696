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

#define LOG_TAG "BEFIFO_HAL"

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
#include "disp_befifo_debug.h"
#include "disp_befifo_hw.h"
#include "disp_befifo_hal.h"
#include "disp_befifo_drv.h"



struct disp_befifo_conf_reg _disp_befifo_sw_base;
struct BEFIFO_Register_Table _disp_befifo_conf_table;
struct disp_befifo_context *_pt_befifo_inst;

uint32_t iBEFIFOReadREG32(uint32_t reg32)
{
	uintptr_t reg = 0;
	uint32_t value = 0;
	uintptr_t hw_base_addr = 0;

	hw_base_addr = _pt_befifo_inst->befifo_hw_base;
	if (hw_base_addr != 0) {
		reg = hw_base_addr + reg32;
		value = ReadREG32(reg);
	}
	return value;
}

int vBEFIFOWriteFldAlign(uint32_t addr, uint32_t val, uint32_t msk,
	uint32_t conf_mode)
{
	uintptr_t reg_va = 0;
	uint32_t reg_pa = 0;
	uint32_t u4Count = 0;
	uintptr_t hw_base_addr = 0;
	struct BEFIFO_Register_Table *ptRegTable = NULL;
	union disp_befifo_hal_union *pbefifo_hw_reg = NULL;

	if (_pt_befifo_inst == NULL)
		return BEFIFO_RET_UNINIT;

	hw_base_addr = _pt_befifo_inst->befifo_hw_base;
	ptRegTable = &_disp_befifo_conf_table;
	pbefifo_hw_reg = (union disp_befifo_hal_union *)hw_base_addr;
	reg_pa = DISP_BEFIFO_REG_BASE;

	if ((hw_base_addr == 0) || !(_pt_befifo_inst->inited))
		return BEFIFO_RET_UNINIT;

	switch (conf_mode) {
	case BEFIFO_REG_CONF_CLIENT_RIU:
		reg_va = hw_base_addr + addr;
		befifo_info("RIU[0x%p 0x%x 0x%x]\n", (void *)reg_va, val, msk);
		WriteREG32Msk(reg_va, val, msk);
		break;

	case BEFIFO_REG_CONF_CLIENT_RIU_SHDW:
		if (pbefifo_hw_reg->field.shdw_en) {
			reg_va = hw_base_addr + addr;
			WriteREG32Msk(reg_va, val, msk);
		} else {
			WriteREG32Msk((hw_base_addr + BEFIFO_SHADOW),
				0x1, SHDW_EN_FLD);
			reg_va = hw_base_addr + addr;
			WriteREG32Msk(reg_va, val, msk);
		}
		break;

	case BEFIFO_REG_CONF_CLIENT_SW_SHDW:
		_disp_befifo_sw_base.befifo_reg.reg[addr/4] &=
			(~((uint32_t)(msk)));
		_disp_befifo_sw_base.befifo_reg.reg[addr/4] |= (val & msk);
		_disp_befifo_sw_base.befifo_reg_mask[addr/4] |= msk;
		break;

	case BEFIFO_REG_CONF_CLIENT_GCE:
		reg_pa = DISP_BEFIFO_REG_BASE + addr;
		u4Count = ptRegTable->depth;
		if (u4Count < HAL_BEFIFO_REG_CONF_CNT) {
			ptRegTable->address[u4Count] = reg_pa;
			ptRegTable->value[u4Count] = val;
			ptRegTable->mask[u4Count] = msk;
			ptRegTable->depth++;
			befifo_info("GCE[0x%x 0x%x 0x%x %d]\n",
				reg_pa, val, msk, u4Count);
		} else
			befifo_error("[BEFIFO][%s][%d]count = %d overflow!\n",
			__func__, __LINE__, u4Count);
		break;

	case BEFIFO_REG_CONF_CLIENT_ML:
		reg_pa = DISP_BEFIFO_REG_BASE + addr;
		u4Count = ptRegTable->depth;
		if (u4Count < HAL_BEFIFO_REG_CONF_CNT) {
			ptRegTable->address[u4Count] = reg_pa;
			ptRegTable->value[u4Count] = val;
			ptRegTable->mask[u4Count] = msk;
			ptRegTable->depth++;
			befifo_info("ML[0x%x 0x%x 0x%x %d]\n",
				reg_pa, val, msk, u4Count);
		} else
			befifo_error("[BEFIFO][%s][%d]count = %d overflow!\n",
			__func__, __LINE__, u4Count);
		break;

	default:
		befifo_error("[BEFIFO]conf reg fail m:%d overflow!\n",
			conf_mode);
		break;
	}
	return BEFIFO_RET_OK;
}

int befifo_hal_isr(uint32_t conf_mode)
{
	uint32_t idx = 0;
	uint32_t u4Count = 0;
	uintptr_t hw_base_addr = 0;
	uint32_t hw_base_pa_addr = 0;
	struct BEFIFO_Register_Table *ptRegTable = NULL;
	union disp_befifo_hal_union *pbefifo_hw_reg = NULL;

	if (_pt_befifo_inst == NULL)
		return BEFIFO_RET_UNINIT;

	hw_base_addr = _pt_befifo_inst->befifo_hw_base;
	hw_base_pa_addr = _pt_befifo_inst->io_reg_base;
	pbefifo_hw_reg =
	(union disp_befifo_hal_union *)(_pt_befifo_inst->befifo_hw_base);
	ptRegTable = &_disp_befifo_conf_table;

	if ((hw_base_addr == 0) || (hw_base_pa_addr == 0))
		return BEFIFO_RET_UNINIT;

	if (conf_mode == BEFIFO_REG_CONF_CLIENT_RIU_SHDW)
		WriteREG32Msk((hw_base_addr + BEFIFO_SHADOW),
			(0x1 << 1), SHDW_TRIGGER_FLD);
	else if (conf_mode == BEFIFO_REG_CONF_CLIENT_SW_SHDW) {
		for (idx = 0; idx < HAL_BEFIFO_REG_NUM; idx++) {
			if (_disp_befifo_sw_base.befifo_reg_mask[idx] != 0) {
				pbefifo_hw_reg->reg[idx] =
				_disp_befifo_sw_base.befifo_reg.reg[idx];
				_disp_befifo_sw_base.befifo_reg_mask[idx] = 0;
			}
		}
	} else if (conf_mode == BEFIFO_REG_CONF_CLIENT_GCE) {
		u4Count = ptRegTable->depth;
		for (idx = 0; idx < u4Count; idx++) {
			if ((ptRegTable->address[idx] >= hw_base_pa_addr)
				&& (ptRegTable->address[idx] <=
				(hw_base_pa_addr + 0x1000))) {
				//call gce write api
			}
		}
		befifo_default("[BEFIFO]config flush cmd = %d done !\n",
			u4Count);
		ptRegTable->depth = 0;
	}
	return BEFIFO_RET_OK;
}

void befifo_hal_set_timing(void *data, uint32_t mode)
{
	uint32_t conf_mode = mode;
	struct disp_befifo_timing *ptbefifo_tg = NULL;

	ptbefifo_tg = (struct disp_befifo_timing *)data;
	if (ptbefifo_tg != NULL) {
		vBEFIFOWriteFldAlign(BEFIFO_TG_TOTAL, ptbefifo_tg->htotal,
			TG_HTOTAL_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_TOTAL,
			(ptbefifo_tg->vtotal_lsb << 16),
			TG_VTOTAL_LSB_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_TOTAL_MSB,
			ptbefifo_tg->vtotal_msb,
			TG_VTOTAL_MSB_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_ACTIVE,
			ptbefifo_tg->width,
			TG_HACTIVE_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_ACTIVE,
			(ptbefifo_tg->height << 16),
			TG_VACTIVE_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_OFFSET,
			ptbefifo_tg->hoffset,
			TG_HOFFSET_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_OFFSET,
			(ptbefifo_tg->voffset << 16),
			TG_VOFFSET_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_FRONT,
			ptbefifo_tg->hfront,
			TG_HFRONT_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_FRONT,
			(ptbefifo_tg->vfront << 16),
			TG_VFRONT_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_SYNCW,
			ptbefifo_tg->hsync_w,
			TG_HSYNCW_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_SYNCW,
			(ptbefifo_tg->vsync_w << 8),
			TG_VSYNCW_FLD, conf_mode);

		vBEFIFOWriteFldAlign(BEFIFO_RD_HACTIVE,
			(ptbefifo_tg->width << 16),
			RD_HACTIVE_W_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_RD_VACTIVE,
			(ptbefifo_tg->height << 16),
			RD_VACTIVE_W_FLD, conf_mode);

		//vBEFIFOWriteFldAlign(BEFIFO_CTL, (0x1 << 10),
		//TG_EN_FLD, conf_mode);
	}
}

void befifo_hal_set_enable(bool fgEnable, uint32_t mode)
{
	uint32_t conf_mode = mode;

	if (fgEnable) {
		vBEFIFOWriteFldAlign(BEFIFO_CTL, (0x3F << 4),
			CLR_BY_VS_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_CTL, (0x1 << 10),
			TG_EN_FLD, conf_mode);
	} else {
		vBEFIFOWriteFldAlign(BEFIFO_CTL, 0x0, CLR_BY_VS_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_CTL, 0x0, TG_EN_FLD, conf_mode);
	}

}

void befifo_hal_clr_isr(bool fgEnable, uint32_t mode)
{
	vBEFIFOWriteFldAlign(BEFIFO_CTL, (0x1 << 24),
		CLR_IRQ_FRAME_DONE_FLD, mode);
	vBEFIFOWriteFldAlign(BEFIFO_CTL, (0x0 << 24),
		CLR_IRQ_FRAME_DONE_FLD, mode);
}

void befifo_hal_set_hdelay(uint32_t hDelta, uint32_t pos)
{
	uint32_t conf_mode = 0;
	uint32_t u4Front = 0;
	uint32_t u4NewHFront = 0;

	u4Front = iBEFIFOReadREG32(BEFIFO_TG_FRONT);

	if (pos == BEFIFO_FRONT_ADJ_MOD_REP) {
		vBEFIFOWriteFldAlign(BEFIFO_TG_FRONT, hDelta,
			TG_HFRONT_FLD, conf_mode);
	} else if (pos == BEFIFO_FRONT_ADJ_MOD_ADD) {
		u4NewHFront = (u4Front & 0xFFFF) + hDelta;
	} else if (pos == BEFIFO_FRONT_ADJ_MOD_DEL) {
		if ((u4Front & 0xFFFF) > hDelta)
			u4NewHFront = (u4Front & 0xFFFF) - hDelta;
	}

	vBEFIFOWriteFldAlign(BEFIFO_TG_FRONT, u4NewHFront,
		TG_HFRONT_FLD, conf_mode);
}

void befifo_hal_set_vdelay(uint32_t vDelta, uint32_t pos)
{
	uint32_t conf_mode = 0;
	uint32_t u4Front = 0;
	uint32_t u4NewVFront = 0;

	u4Front = iBEFIFOReadREG32(BEFIFO_TG_FRONT);

	if (pos == BEFIFO_FRONT_ADJ_MOD_REP)
		vBEFIFOWriteFldAlign(BEFIFO_TG_FRONT, (vDelta << 16),
			TG_VFRONT_FLD, conf_mode);
	else if (pos == BEFIFO_FRONT_ADJ_MOD_ADD)
		u4NewVFront = ((u4Front >> 16) & 0xFFFF) + vDelta;
	else if (pos == BEFIFO_FRONT_ADJ_MOD_DEL)
		if (((u4Front >> 16) & 0xFFFF) > vDelta)
			u4NewVFront = ((u4Front >> 16) & 0xFFFF) - vDelta;
	vBEFIFOWriteFldAlign(BEFIFO_TG_FRONT, (u4NewVFront << 16),
		TG_VFRONT_FLD, conf_mode);
}


void befifo_hal_set_front(uint32_t hfront, uint32_t vfront)
{
	uint32_t conf_mode = 0;

	vBEFIFOWriteFldAlign(BEFIFO_TG_FRONT, hfront,
		TG_HFRONT_FLD, conf_mode);
	vBEFIFOWriteFldAlign(BEFIFO_TG_FRONT, vfront,
		TG_VFRONT_FLD, conf_mode);
}


void befifo_hal_set_threshold(void)
{

}

void befifo_hal_set_background(uint32_t colorY, uint32_t colorCb,
	uint32_t colorCr)
{
	uint32_t conf_mode = 0;

	vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DATA_CFG3, colorCr,
		BEFIFO_PATGEN_BLANK_CR_FLD, conf_mode);
	vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DATA_CFG3, (colorCb << 16),
		BEFIFO_PATGEN_BLANK_CB_FLD, conf_mode);
	vBEFIFOWriteFldAlign(BEFIFO_DATA_BLANK_Y, colorY,
		BEFIFO_DATA_BLANK_Y_FLD, conf_mode);
}

void befifo_hal_set_pattern(void *data, bool fgEnable, void *pattern)
{
	uint32_t conf_mode = 0;
	struct disp_befifo_timing *ptbefifo_tg = NULL;
	struct disp_befifo_pattern *ptPattern = NULL;

	ptbefifo_tg = (struct disp_befifo_timing *)data;
	ptPattern = (struct disp_befifo_pattern *)pattern;
	if ((ptbefifo_tg != NULL) && (ptPattern != NULL)) {
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_TOTAL, ptbefifo_tg->htotal,
			PATGEN_HTOTAL_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_TOTAL,
			(ptbefifo_tg->vtotal_lsb << 16),
			PATGEN_VTOTAL_LSB_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_TOTAL_MSB,
			ptbefifo_tg->vtotal_msb,
			PATGEN_VTOTAL_MSB_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_ACTIVE,
			ptbefifo_tg->width,
			PATGEN_HACTIVE_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_ACTIVE,
			(ptbefifo_tg->height << 16),
			PATGEN_VACTIVE_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_START,
			ptbefifo_tg->hfront,
			PATGEN_HSTART_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_START,
			(ptbefifo_tg->vfront << 16),
			PATGEN_VSTART_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_SYNC_W,
			ptbefifo_tg->hsync_w,
			PATGEN_HSYNC_W_FLD, conf_mode);
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_SYNC_W,
			(ptbefifo_tg->vsync_w << 16),
			PATGEN_VSYNC_W_FLD, conf_mode);
		if (ptPattern->mode == 1) {
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DATA_CFG1,
				ptPattern->crpattern,
				BEFIFO_PATGEN_ACTIVE_CR_FLD, conf_mode);
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DATA_CFG1,
				(ptPattern->ypattern << 16),
				BEFIFO_PATGEN_ACTIVE_Y_FLD, conf_mode);
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DATA_CFG2,
				ptPattern->cbpattern,
				BEFIFO_PATGEN_ACTIVE_CB_FLD, conf_mode);
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DATA_CFG2,
				(ptPattern->ypattern_b << 16),
				BEFIFO_PATGEN_BLANK_Y_FLD, conf_mode);
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DATA_CFG3,
				ptPattern->crpattern_b,
				BEFIFO_PATGEN_BLANK_CR_FLD, conf_mode);
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DATA_CFG3,
				(ptPattern->cbpattern_b << 16),
				BEFIFO_PATGEN_BLANK_CB_FLD, conf_mode);
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DEF_CONF, 0x1,
				BEFIFO_PATGEN_DEF_ACTIVE_FLD, conf_mode);
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DEF_CONF, (0x1 << 1),
				BEFIFO_PATGEN_DEF_BLANK_FLD, conf_mode);
		} else {
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DEF_CONF, 0x0,
				BEFIFO_PATGEN_DEF_ACTIVE_FLD, conf_mode);
			vBEFIFOWriteFldAlign(BEFIFO_PATGEN_DEF_CONF, (0x0 << 1),
				BEFIFO_PATGEN_DEF_BLANK_FLD, conf_mode);
		}
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_EN, 0x1,
			PATGEN_EN_FLD, conf_mode);

	}
	if (fgEnable == false)
		vBEFIFOWriteFldAlign(BEFIFO_PATGEN_EN, 0x0,
		PATGEN_EN_FLD, conf_mode);

}

void befifo_hal_set_out_swap(uint32_t u4SwapType)
{
	uint32_t conf_mode = 0;

	vBEFIFOWriteFldAlign(BEFIFO_CTL, (u4SwapType << 12),
		SWAP_OUT_FLD, conf_mode);
}

void befifo_hal_set_shdw(bool enable, bool fgTrig)
{
	if (enable)
		vBEFIFOWriteFldAlign(BEFIFO_SHADOW, 0x1, SHDW_EN_FLD, 0);
	else
		vBEFIFOWriteFldAlign(BEFIFO_SHADOW, 0x0, SHDW_EN_FLD, 0);
	if (fgTrig)
		vBEFIFOWriteFldAlign(BEFIFO_SHADOW,
			(0x1 << 1), SHDW_TRIGGER_FLD, 0);
	else
		vBEFIFOWriteFldAlign(BEFIFO_SHADOW,
			(0x0 << 1), SHDW_TRIGGER_FLD, 0);
}


void befifo_init_sw_reg(uintptr_t reg_base)
{
	uint32_t id = 0;

	memcpy((void *)_disp_befifo_sw_base.befifo_reg.reg,
		(const void *)reg_base,
		sizeof(union disp_befifo_hal_union));
	for (id = 0; id < HAL_BEFIFO_REG_NUM; id++)
		_disp_befifo_sw_base.befifo_reg_mask[id] = 0;
}

void befifo_hal_reset(void)
{
	befifo_hal_set_shdw(false, false);
	vBEFIFOWriteFldAlign(BEFIFO_SOFT_RESET, 0x3, 0x3, 0);
	vBEFIFOWriteFldAlign(BEFIFO_SOFT_RESET, 0x0, 0x3, 0);
}

void befifo_hal_set_sof(uint32_t u4sof_s, uint32_t u4sof_e,
	uint32_t u4eof_s, uint32_t u4eof_e)
{
	// set sof
	//call reg setting
}


int befifo_hal_init(uintptr_t reg_base)
{
	_pt_befifo_inst = befifo_get_inst();
	/*Init register base */
	if (_pt_befifo_inst->inited) {
		befifo_default("befifo hal already inited\n");
		return BEFIFO_RET_UNINIT;
	}

	//befifo_init_sw_reg(reg_base);

	memset(&_disp_befifo_conf_table, 0,
		sizeof(struct BEFIFO_Register_Table));
	_pt_befifo_inst->preg_tbl = &_disp_befifo_conf_table;
	return BEFIFO_RET_OK;
}

void befifo_hal_uninit(void)
{
	/*Init register base */
	memset(&_disp_befifo_conf_table, 0,
		sizeof(struct BEFIFO_Register_Table));
}

void befifo_hal_enable_crc(bool fgEnable, uint32_t mode)
{
	uint32_t befifo_crc1 = 0;
	uint32_t befifo_crc2 = 0;
	uint32_t befifo_crc3 = 0;
	uint32_t befifo_status1 = 0;
	uint32_t befifo_status2 = 0;

	befifo_crc1 = iBEFIFOReadREG32(BEFIFO_CRC1_STATUS);
	befifo_crc2 = iBEFIFOReadREG32(BEFIFO_CRC2_STATUS);
	befifo_crc3 = iBEFIFOReadREG32(BEFIFO_CRC3_STATUS);
	befifo_status1 = iBEFIFOReadREG32(BEFIFO_HW_STATUS);
	befifo_status2 = iBEFIFOReadREG32(BEFIFO_VRR_STATUS);

	befifo_default("crc value[%x %x %x] status[%x %x]\n",
		befifo_crc1, befifo_crc2, befifo_crc3,
		befifo_status1, befifo_status2);

	if (fgEnable) {
		vBEFIFOWriteFldAlign(BEFIFO_CTL, (0x0 << 16),
		CRC_TRIG_FLD, mode);
		vBEFIFOWriteFldAlign(BEFIFO_CTL, (0x15 << 16),
		CRC_TRIG_FLD, mode);
	} else {
		vBEFIFOWriteFldAlign(BEFIFO_CTL, (0x0 << 16),
		CRC_TRIG_FLD, mode);
	}
}

#ifdef DISP_QMS_SUPPORT
void befifo_hal_set_vtotal_ext(bool fgEnable,
	uint32_t value, uint32_t mode)
{
	uint32_t vtotal_lsb = 0;
	uint32_t vtotal_msb = 0;

	if (fgEnable) {
		vtotal_lsb = (value & 0xFFFF);
		vtotal_msb = (value & 0xFFFF0000) >> 16;
		vBEFIFOWriteFldAlign(BEFIFO_TG_TOTAL,
			(vtotal_lsb << 16),
			TG_VTOTAL_LSB_FLD, mode);
		vBEFIFOWriteFldAlign(BEFIFO_TG_TOTAL_MSB,
			vtotal_msb,
			TG_VTOTAL_MSB_FLD, mode);
	}
}
#endif

