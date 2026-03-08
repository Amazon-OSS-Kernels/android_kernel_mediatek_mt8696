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

#define LOG_TAG "R2R_HAL"

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
#include "disp_r2r_debug.h"
#include "disp_r2r_hal.h"
#include "disp_r2r_hw.h"
#include "disp_r2r_drv.h"




struct disp_r2r_conf_reg _disp_r2r_sw_base;
struct R2R_Register_Table _disp_r2r_conf_table;
struct disp_r2r_context *_pt_r2r_inst;

uint32_t iR2RReadREG32(uint32_t reg32)
{
	uintptr_t reg = 0;
	uint32_t value = 0;
	uintptr_t hw_base_addr = 0;

	hw_base_addr = _pt_r2r_inst->r2r_hw_base;
	if (hw_base_addr != 0) {
		reg = hw_base_addr + reg32;
		value = ReadREG32(reg);
	}
	return value;
}

int vR2RWriteFldAlign(uint32_t addr, uint32_t val, uint32_t msk,
	uint32_t conf_mode)
{
	uintptr_t reg_va = 0;
	uint32_t reg_pa = 0;
	uint32_t u4Count = 0;
	uintptr_t hw_base_addr = 0;
	struct R2R_Register_Table *ptRegTable = NULL;
	union disp_r2r_hal_union *pr2r_hw_reg = NULL;

	if (_pt_r2r_inst == NULL)
		return R2R_RET_UNINIT;

	hw_base_addr = _pt_r2r_inst->r2r_hw_base;
	ptRegTable = &_disp_r2r_conf_table;
	pr2r_hw_reg = (union disp_r2r_hal_union *)hw_base_addr;

	if ((hw_base_addr == 0) || !(_pt_r2r_inst->inited))
		return R2R_RET_UNINIT;

	switch (conf_mode) {
	case R2R_REG_CONF_CLIENT_RIU:
		reg_va = hw_base_addr + addr;
		r2r_info("RIU[0x%p 0x%x 0x%x]\n", (void *)reg_va, val, msk);
		WriteREG32Msk(reg_va, val, msk);
		break;

	case R2R_REG_CONF_CLIENT_RIU_SHDW:
		if (pr2r_hw_reg->field.shdw_en) {
			reg_va = hw_base_addr + addr;
			WriteREG32Msk(reg_va, val, msk);
		} else {
			WriteREG32Msk((hw_base_addr + R2R_TG_TOTAL_CTRL),
				(0x1 << 30), R2R_TG_SHDW_EN_FLD);
			reg_va = hw_base_addr + addr;
			WriteREG32Msk(reg_va, val, msk);
		}
		break;

	case R2R_REG_CONF_CLIENT_SW_SHDW:
		_disp_r2r_sw_base.r2r_reg.reg[addr/4] &= (~((uint32_t)(msk)));
		_disp_r2r_sw_base.r2r_reg.reg[addr/4] |= (val & msk);
		_disp_r2r_sw_base.r2r_reg_mask[addr/4] |= msk;
		break;

	case R2R_REG_CONF_CLIENT_GCE:
		reg_pa = DISP_R2R_REG_BASE + addr;
		u4Count = ptRegTable->depth;
		if (u4Count < HAL_R2R_REG_CONF_CNT) {
			ptRegTable->address[u4Count] = reg_pa;
			ptRegTable->value[u4Count] = val;
			ptRegTable->mask[u4Count] = msk;
			ptRegTable->depth++;
			r2r_info("GCE[0x%x 0x%x 0x%x %d]\n",
				reg_pa, val, msk, u4Count);
		} else
			r2r_error("[R2R][%s][%d] u16Count = %d  overflow !\n",
			__func__, __LINE__, u4Count);
		break;

	case R2R_REG_CONF_CLIENT_ML:
		reg_pa = DISP_R2R_REG_BASE + addr;
		u4Count = ptRegTable->depth;
		if (u4Count < HAL_R2R_REG_CONF_CNT) {
			ptRegTable->address[u4Count] = reg_pa;
			ptRegTable->value[u4Count] = val;
			ptRegTable->mask[u4Count] = msk;
			ptRegTable->depth++;
			r2r_info("ML[0x%x 0x%x 0x%x %d]\n",
				reg_pa, val, msk, u4Count);
		} else
			r2r_error("[R2R][%s][%d] u16Count = %d  overflow !\n",
			__func__, __LINE__, u4Count);
		break;

	default:
		r2r_error("[R2R]conf reg fail m = %d  overflow !\n",
			conf_mode);
		break;
	}
	return R2R_RET_OK;
}

int r2r_hal_isr(uint32_t conf_mode)
{
	uint32_t idx = 0;
	uint32_t u4Count = 0;
	uintptr_t hw_base_addr = 0;
	uint32_t hw_pa_base_addr = 0;
	uintptr_t reg_va = 0;
	struct R2R_Register_Table *ptRegTable = NULL;
	union disp_r2r_hal_union *pr2r_hw_reg = NULL;

	if (_pt_r2r_inst == NULL)
		return R2R_RET_UNINIT;

	hw_base_addr = _pt_r2r_inst->r2r_hw_base;
	hw_pa_base_addr = _pt_r2r_inst->io_reg_base;
	pr2r_hw_reg = (union disp_r2r_hal_union *)_pt_r2r_inst->r2r_hw_base;
	ptRegTable = &_disp_r2r_conf_table;

	if ((hw_base_addr == 0) || (hw_pa_base_addr == 0))
		return R2R_RET_UNINIT;

	if (conf_mode == R2R_REG_CONF_CLIENT_RIU_SHDW) {
		reg_va = hw_base_addr + R2R_TG_TOTAL_CTRL;
		WriteREG32Msk(reg_va, (0x1 << 31),
			R2R_TG_SHDW_SW_TRIG_FLD);
	} else if (conf_mode == R2R_REG_CONF_CLIENT_SW_SHDW) {
		for (idx = 0; idx < HAL_R2R_REG_NUM; idx++) {
			if (_disp_r2r_sw_base.r2r_reg_mask[idx] != 0) {
				pr2r_hw_reg->reg[idx] =
					_disp_r2r_sw_base.r2r_reg.reg[idx];
				_disp_r2r_sw_base.r2r_reg_mask[idx] = 0;
			}
		}
	} else if (conf_mode == R2R_REG_CONF_CLIENT_GCE) {
		u4Count = ptRegTable->depth;
		for (idx = 0; idx < u4Count; idx++) {
			if ((ptRegTable->address[idx] >= hw_pa_base_addr)
				&& (ptRegTable->address[idx] <=
				(hw_pa_base_addr + 0x1000))) {
				//call gce write api
			}
		}
		r2r_default("GCE r2r flush cmd:%d done\n", u4Count);
		ptRegTable->depth = 0;
	} else if (conf_mode == R2R_REG_CONF_CLIENT_ML) {
		u4Count = ptRegTable->depth;
		for (idx = 0; idx < u4Count; idx++) {
			if ((ptRegTable->address[idx] >= hw_pa_base_addr)
				&& (ptRegTable->address[idx] <=
				(hw_pa_base_addr + 0x1000))) {
				//call gce write api
			}
		}
		r2r_default("ML r2r flush cmd:%d done\n", u4Count);
		ptRegTable->depth = 0;
	}
	return R2R_RET_OK;
}

void r2r_hal_set_timing(void *data)
{
	uint32_t conf_mode = 0;
	struct disp_r2r_timing *ptr2r_tg = NULL;

	ptr2r_tg = (struct disp_r2r_timing *)data;
	if (ptr2r_tg != NULL) {
		vR2RWriteFldAlign(R2R_TG_TOTAL_CTRL, ptr2r_tg->vtotal_lsb,
			R2R_TG_V_TOTAL_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_TOTAL_CTRL, (ptr2r_tg->htotal << 16),
			R2R_TG_H_TOTAL_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_VDE_CTRL, ptr2r_tg->height,
			R2R_TG_VDEW_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_VDE_CTRL, (ptr2r_tg->hsync_w << 16),
			R2R_TG_HSYNC_W_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_VSYNC_CTRL, ptr2r_tg->vsync_w,
			R2R_TG_VSYNC_END_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_VSYNC_CTRL, (0x1 << 16),
			R2R_TG_VSYNC_START_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_HACTIVE_CTRL,
			(ptr2r_tg->hfront + ptr2r_tg->width - 1),
			R2R_TG_HACTIVE_END_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_HACTIVE_CTRL, (ptr2r_tg->hfront << 16),
			R2R_TG_HACTIVE_START_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_VACTIVE_CTRL,
			(ptr2r_tg->vfront + ptr2r_tg->height - 1),
			R2R_TG_VACTIVE_END_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_TG_VACTIVE_CTRL, (ptr2r_tg->vfront << 16),
			R2R_TG_VACTIVE_START_FLD, conf_mode);
	}
}

void r2r_hal_set_enable(bool fgEnable, uint32_t mode)
{
	uint32_t conf_mode = mode;

	if (fgEnable) {
		vR2RWriteFldAlign(R2R_HW_CTRL0, 0x1,
			R2R_CTRL_EN_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 1),
			R2R_CTRL_GREQ_EN_FLD,
			conf_mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 7),
			R2R_CTRL_VS_PUL_EN_FLD,
			conf_mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 6),
			R2R_CTRL_VS_PUL_ONCE_FLD,
			conf_mode);

	} else {
		vR2RWriteFldAlign(R2R_HW_CTRL0, 0x0,
			R2R_CTRL_EN_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x0 << 1),
			R2R_CTRL_GREQ_EN_FLD,
			conf_mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x0 << 7),
			R2R_CTRL_VS_PUL_EN_FLD,
			conf_mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x0 << 6),
			R2R_CTRL_VS_PUL_ONCE_FLD,
			conf_mode);
	}

}

void r2r_hal_clr_isr(bool fgEnable, uint32_t mode)
{
	vR2RWriteFldAlign(R2R_INT_STATE, 0x1, R2R_INT_CLR_FLD, 0);
	vR2RWriteFldAlign(R2R_INT_STATE, 0x0, R2R_INT_CLR_FLD, 0);
}

void r2r_hal_set_format(bool is_422, uint32_t bit_dep, uint32_t mode)
{
	uint32_t conf_mode = mode;

	vR2RWriteFldAlign(R2R_TG_VDE_CTRL, (0x1 << 31),
		R2R_TG_MODE444_FLD, conf_mode);
	if (is_422)
		vR2RWriteFldAlign(R2R_HW_CTRL2, (0x1 << 16),
		R2R_CTRL_422_TO_444_EN_FLD,
		conf_mode);

	if (bit_dep == 10)
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 4),
			R2R_CTRL_MODE_10B_FLD, conf_mode);
	else if (bit_dep == 12)
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 5),
			R2R_CTRL_MODE_12B_FLD, conf_mode);
}


void r2r_hal_set_burst_mode(uint32_t bit_dep, uint32_t mode)
{
	if (bit_dep == 4) {
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 3),
			R2R_CTRL_BURST_EN_FLD, mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 2),
			R2R_CTRL_BURST4_FLD, mode);
	} else if (bit_dep == 8) {
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x0 << 2),
			R2R_CTRL_BURST4_FLD, mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 3),
			R2R_CTRL_BURST_EN_FLD, mode);
	} else {
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x0 << 2),
			R2R_CTRL_BURST4_FLD, mode);
		vR2RWriteFldAlign(R2R_HW_CTRL0, (0x1 << 3),
			R2R_CTRL_BURST_EN_FLD, mode);
	}
}


void r2r_hal_set_pattern(bool fgEnable, void *pattern, void *data)
{
	uint32_t conf_mode = 0;
	struct disp_r2r_pattern *ptPattern = NULL;
	struct disp_r2r_timing *ptr2r_tg = NULL;

	ptr2r_tg = (struct disp_r2r_timing *)data;
	ptPattern = (struct disp_r2r_pattern *)pattern;
	if ((ptr2r_tg != NULL) &&
		(ptPattern != NULL) &&
		(fgEnable == true)) {
		vR2RWriteFldAlign(R2R_PATGEN_CONF0, ptr2r_tg->height,
			R2R_PATGEN_BND_H_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_PATGEN_CONF0, (ptr2r_tg->width << 16),
			R2R_PATGEN_BND_W_FLD, conf_mode);

		vR2RWriteFldAlign(R2R_HW_CTRL2, ptPattern->ypattern,
			R2R_CTRL_PATGEN_Y_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_PATGEN_CONF2,
			(ptPattern->cbpattern << 16),
			R2R_PATGEN_C_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_PATGEN_CONF2, ptPattern->crpattern,
			R2R_PATGEN_CC_FLD, conf_mode);

		vR2RWriteFldAlign(R2R_HW_CTRL1, (0x1 << 28),
			R2R_CTRL_PATGEN_COLOR_FLD, conf_mode);
		vR2RWriteFldAlign(R2R_HW_CTRL1, (0x1 << 29),
			R2R_CTRL_PATGEN_EN_FLD, conf_mode);
	}
	if (fgEnable == false)
		vR2RWriteFldAlign(R2R_HW_CTRL1, (0x0 << 29),
			R2R_CTRL_PATGEN_EN_FLD, conf_mode);

}

void r2r_hal_set_out_swap(uint32_t u4SwapType, uint32_t conf_mode)
{
	vR2RWriteFldAlign(R2R_HW_CTRL2, (u4SwapType << 17),
		R2R_CTRL_CHROMA_SWAP_FLD, conf_mode);
}


void r2r_hal_set_shdw(bool enable, bool sw_trig)
{
	if (enable)
		vR2RWriteFldAlign(R2R_TG_TOTAL_CTRL, (0x1 << 30),
			R2R_TG_SHDW_EN_FLD, 0);
	else
		vR2RWriteFldAlign(R2R_TG_TOTAL_CTRL, (0x0 << 30),
			R2R_TG_SHDW_EN_FLD, 0);
	if (sw_trig)
		vR2RWriteFldAlign(R2R_TG_TOTAL_CTRL, (0x1 << 31),
			R2R_TG_SHDW_EN_FLD, 0);
	else
		vR2RWriteFldAlign(R2R_TG_TOTAL_CTRL, (0x0 << 31),
			R2R_TG_SHDW_EN_FLD, 0);
}

void r2r_hal_set_pitch(uint32_t pitch, uint32_t mode)
{
	vR2RWriteFldAlign(R2R_TG_POLAR_CTRL, (pitch << 8),
		R2R_TG_H_MAX_FLD, mode);
	vR2RWriteFldAlign(R2R_TG_POLAR_CTRL, (pitch << 20),
		R2R_TG_HDEW_PITCH_FLD, mode);
}

void r2r_hal_set_req_threshold(uint32_t req_len, uint32_t mode)
{
	vR2RWriteFldAlign(R2R_TG_POLAR_CTRL, req_len,
		R2R_TG_REQ_TH_FLD, mode);
}

void r2r_hal_conf_frame(uint32_t yaddr,
	uint32_t caddr, uint32_t ccaddr, uint32_t mode)
{
	vR2RWriteFldAlign(R2R_RDMA_ADDR_Y0, (yaddr >> 4), 0xFFFFFFF, mode);
	vR2RWriteFldAlign(R2R_RDMA_ADDR_C0, (caddr >> 4), 0xFFFFFFF, mode);
	vR2RWriteFldAlign(R2R_RDMA_ADDR_CC0, (ccaddr >> 4), 0xFFFFFFF, mode);
}

void r2r_init_sw_reg(uintptr_t reg_base)
{
	uint32_t id = 0;

	memcpy((void *)_disp_r2r_sw_base.r2r_reg.reg, (const void *)reg_base,
		sizeof(union disp_r2r_hal_union));
	for (id = 0; id < HAL_R2R_REG_NUM; id++)
		_disp_r2r_sw_base.r2r_reg_mask[id] = 0;
}

void r2r_hal_reset(void)
{
	vR2RWriteFldAlign(R2R_TG_POLAR_CTRL, (0x1 << 31),
		R2R_TG_SW_RST_FLD, 0);
	vR2RWriteFldAlign(R2R_TG_POLAR_CTRL, (0x0 << 31),
		R2R_TG_SW_RST_FLD, 0);
}

int r2r_hal_init(uintptr_t reg_base)
{
	_pt_r2r_inst = r2r_get_inst();
	/*Init register base */
	if (_pt_r2r_inst->inited) {
		r2r_default("r2r hal already inited\n");
		return R2R_RET_UNINIT;
	}
	//r2r_init_sw_reg(reg_base);
	memset(&_disp_r2r_conf_table, 0,
		sizeof(struct R2R_Register_Table));
	_pt_r2r_inst->preg_tbl = &_disp_r2r_conf_table;
	return R2R_RET_OK;
}

void r2r_hal_uninit(void)
{
	/*Init register base */
	memset(&_disp_r2r_conf_table, 0,
		sizeof(struct R2R_Register_Table));
}



