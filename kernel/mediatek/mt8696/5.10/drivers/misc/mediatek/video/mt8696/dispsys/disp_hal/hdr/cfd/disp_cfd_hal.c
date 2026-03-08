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

#define LOG_TAG "CFD_HAL"

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
#include "disp_cfd_hw.h"
#include "disp_cfd_hdr_hw.h"
#include "disp_cfd_debug.h"
#include "disp_cfd_hal.h"
#include "disp_cfd_main.h"
#include "disp_adl_if.h"
#include "disp_ml_if.h"


struct CFD_Register_Table _disp_cfd_conf_table[CFD_MAX_LAYER_NUM];
struct disp_cfd_context *_pt_cfd_inst[CFD_MAX_LAYER_NUM];

uint32_t iCFDReadREG32(uint32_t reg32, uint32_t client)
{
	uintptr_t reg = 0;
	uint32_t value = 0;
	uintptr_t hw_base_addr = 0;

	hw_base_addr = _pt_cfd_inst[client]->cfd_hw_base;
	if (hw_base_addr != 0) {
		reg = hw_base_addr + reg32;
		value = ReadREG32(reg);
	}
	return value;
}

int vCFDWriteFldAlign(uint32_t addr, uint16_t val, uint16_t msk,
	uint32_t conf_mode, uint32_t client)
{
	uintptr_t reg_va = 0;
	uint32_t reg_pa = 0;
	uint32_t u4Count = 0;
	uintptr_t hw_base_addr_va = 0;
	struct CFD_Register_Table *ptRegTable = NULL;

	if (_pt_cfd_inst[client%4] == NULL)
		return CFD_RET_UNINIT;

	switch (client) {
	case 0:
		hw_base_addr_va = _pt_cfd_inst[client]->cfd_hw_base;
		ptRegTable = &_disp_cfd_conf_table[client];
		reg_pa = DISP_CFD_VDOFE0_REG_BASE;
		break;
	case 1:
		hw_base_addr_va = _pt_cfd_inst[client]->cfd_hw_base;
		ptRegTable = &_disp_cfd_conf_table[client];
		reg_pa = DISP_CFD_VDOFE1_REG_BASE;
		break;
	case 2:
		hw_base_addr_va = _pt_cfd_inst[client]->cfd_hw_base;
		ptRegTable = &_disp_cfd_conf_table[client];
		reg_pa = DISP_CFD_GFXFE0_REG_BASE;
		break;
	case 3:
		hw_base_addr_va = _pt_cfd_inst[client]->cfd_hw_base;
		ptRegTable = &_disp_cfd_conf_table[client];
		reg_pa = DISP_CFD_GFXFE1_REG_BASE;
		break;
	case 4:
		hw_base_addr_va = _pt_cfd_inst[0]->cfd_hw_base_ext;
		ptRegTable = &_disp_cfd_conf_table[0];
		reg_pa = DISP_CFD_XVYCC0_REG_BASE;
		break;
	case 5:
		hw_base_addr_va = _pt_cfd_inst[1]->cfd_hw_base_ext;
		ptRegTable = &_disp_cfd_conf_table[1];
		reg_pa = DISP_CFD_XVYCC1_REG_BASE;
		break;
	default:
		cfd_error("invalid cfd client[%d]\n", client);
		return CFD_RET_INV_ARG;
	}

	if ((hw_base_addr_va == 0) || !(_pt_cfd_inst[client%4]->inited))
		return CFD_RET_UNINIT;

	switch (conf_mode) {
	case CFD_REG_CONF_CLIENT_RIU:
		reg_va = hw_base_addr_va + addr;
		WriteREG32Msk(reg_va, val, msk);
		cfd_info("RIU[0x%p 0x%x 0x%x %d %d]\n",
			(void *)reg_va, val, msk,
			conf_mode, client);
		break;

	case CFD_REG_CONF_CLIENT_GCE:
		reg_pa = reg_pa + addr;
		u4Count = ptRegTable->depth;
		if (u4Count < HAL_CFD_REG_CONF_CNT) {
			ptRegTable->address[u4Count] = reg_pa;
			ptRegTable->value[u4Count] = val;
			ptRegTable->mask[u4Count] = msk;
			ptRegTable->depth++;
			cfd_info("GCE[0x%x 0x%x 0x%x %d %d]\n",
				reg_pa, val, msk,
				ptRegTable->depth, client);
		} else
			cfd_error("[CFD][%s][%d] u16Count = %d  overflow !\n",
			__func__, __LINE__, u4Count);
		break;

	case CFD_REG_CONF_CLIENT_ML:
		reg_pa = reg_pa + addr;
		u4Count = ptRegTable->depth;
		if (u4Count < HAL_CFD_REG_CONF_CNT) {
			ptRegTable->address[u4Count] = reg_pa;
			ptRegTable->value[u4Count] = val;
			ptRegTable->mask[u4Count] = msk;
			ptRegTable->depth++;
			cfd_info("ML[0x%x 0x%x 0x%x %d %d]\n",
				reg_pa, val, msk,
				ptRegTable->depth, client);
		} else
			cfd_error("[CFD][%s][%d] u16Count = %d overflow!\n",
			__func__, __LINE__, u4Count);
		break;

	default:
		cfd_error("[CFD]config reg fail conf_mode = %d overflow!\n",
			conf_mode);
		break;
	}
	return CFD_RET_OK;
}

int cfd_hal_init(uint32_t layer_id, uintptr_t reg_base)
{
	_pt_cfd_inst[layer_id] = cfd_get_inst(layer_id);
	/*Init register base */
	if (_pt_cfd_inst[layer_id]->inited) {
		cfd_default("cfd hal already inited\n");
		return CFD_RET_UNINIT;
	}
	memset(&_disp_cfd_conf_table[layer_id], 0,
		sizeof(struct CFD_Register_Table));
	_pt_cfd_inst[layer_id]->preg_tbl = &_disp_cfd_conf_table[layer_id];
	return CFD_RET_OK;
}

void cfd_hal_uninit(uint32_t layer_id)
{
	/*Init register base */
	memset(&_disp_cfd_conf_table[layer_id], 0,
		sizeof(struct CFD_Register_Table));
}

int disp_cfd_hal_set_src_res(uint32_t layer_id, uint32_t src_width,
	uint32_t src_height, uint32_t conf_mode)
{
	// src active
	if ((layer_id == 0) || (layer_id == 1)) {
		vCFDWriteFldAlign(VDO_09CC_DM_FE, src_width,
			VDO_09CC_H_SIZE, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09D0_DM_FE, src_height,
			VDO_09D0_V_SIZE, conf_mode, layer_id);
	} else if ((layer_id == 2) || (layer_id == 3)) {
		// only set v size
		vCFDWriteFldAlign(GFX_0468_TCH_OSD_UVC, src_height,
			GFX_0468_VLEN, conf_mode, layer_id);
	}

	return CFD_RET_OK;
}

int disp_cfd_hal_isr(uint32_t layer_id, uint32_t conf_mode)
{
	uint32_t idx = 0;
	uint32_t u4Count = 0;
	uint32_t hw_pa_base_addr = 0;
	uint32_t hw_pa_base_addr_ext = 0;
	uint32_t padding = 0;
	struct CFD_Register_Table *ptRegTable = NULL;
	struct ml_reg_table reg_tbl;

	if (_pt_cfd_inst[layer_id] == NULL)
		return CFD_RET_UNINIT;

	hw_pa_base_addr = _pt_cfd_inst[layer_id]->io_reg_base;
	hw_pa_base_addr_ext = _pt_cfd_inst[layer_id]->io_reg_base_ext;
	ptRegTable = &_disp_cfd_conf_table[layer_id];

	if (hw_pa_base_addr == 0)
		return CFD_RET_UNINIT;

	if (conf_mode == CFD_REG_CONF_CLIENT_GCE) {
		u4Count = ptRegTable->depth;
		for (idx = 0; idx < u4Count; idx++) {
			if ((ptRegTable->address[idx] >= hw_pa_base_addr)
				&& (ptRegTable->address[idx] <=
				(hw_pa_base_addr + 0x1000))) {
				//call ml write api
				//call
			} else if (
			(ptRegTable->address[idx] >= hw_pa_base_addr_ext)
				&& (ptRegTable->address[idx] <=
				(hw_pa_base_addr_ext + 0x1000))) {

			}
		}
		cfd_default("cfd[%d] GCE flush cmd:%d done\n",
			layer_id, u4Count);
		ptRegTable->depth = 0;
	} else if (conf_mode == CFD_REG_CONF_CLIENT_ML) {
		u4Count = ptRegTable->depth;
		memset(&reg_tbl, 0, sizeof(struct ml_reg_table));
		if (layer_id < 2)
			reg_tbl.ml_ip = ML_DSYS_IP;
		else
			reg_tbl.ml_ip = ML_MSYS_IP;

		//do 4 cmd align first
		padding = (4 - (u4Count % 4)) % 4;
		while (padding) {
			ptRegTable->address[u4Count] =
				ptRegTable->address[u4Count-1];
			ptRegTable->value[u4Count] =
				ptRegTable->value[u4Count-1];
			ptRegTable->mask[u4Count] =
				ptRegTable->mask[u4Count-1];
			u4Count++;
			padding--;
		}

		reg_tbl.depth = u4Count;
		reg_tbl.p_reg_addr = ptRegTable->address;
		reg_tbl.p_reg_value = ptRegTable->value;
		reg_tbl.p_reg_mask = ptRegTable->mask;
		reg_tbl.path = ML_OPENHDR;
		if (reg_tbl.depth > 0)
			disp_ml_write_reg_multi(&reg_tbl);

		cfd_info("cfd[%d] ML flush cmd:%d done\n",
			layer_id, u4Count);
		ptRegTable->depth = 0;
		//use sw tirge mode driect update ml
		//disp_ml_isr();
	}
	return CFD_RET_OK;
}

int disp_cfd_hal_set_bypass(uint32_t layer_id, uint32_t conf_mode)
{
	if ((layer_id == 0) || (layer_id == 1)) {
		vCFDWriteFldAlign(VDO_0618_TOP_FE, 0x0, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_061C_TOP_FE, (0x1 << 1),
			VDO_061C_HDR2IP_PATH_EN,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_0634_TOP_FE, (0x1 << 15),
			VDO_0634_HDR_IN_PATH_AUTO_EN, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_0804_DM_FE, 0xFD, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_081C_DM_FE, 0x12E, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09C0_DM_FE, 0x0F, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09C4_DM_FE, 0x00, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09EC_DM_FE, 0x80, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(XVYCC_TOP_CONTROL, 0x0,
			0xFFFF, conf_mode, (layer_id + 4));
		vCFDWriteFldAlign(VDO_0EAC_FE_DV_WP, 0x0,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_0710_TOP_FE, 0x0,
			VDO_0710_OOTF_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09F0_DM_FE, 0x0,
			VDO_09F0_DM_DITH_EN_FLD, conf_mode, layer_id);
	} else if ((layer_id == 2) || (layer_id == 3)) {
		vCFDWriteFldAlign(GFX_0100_DV_WP, 0x1, GFX_0100_CLK_ON_FLD,
			conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0100_DV_WP, (0x1 << 15),
			GFX_0100_PATH_SEL_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0204_HDR_FE, 0xFD, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_021C_HDR_FE, 0x20, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_03C0_HDR_FE, 0x07, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_03C4_HDR_FE, 0x00, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(0x3EC, 0x80, 0xFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0460_TCH_OSD_UVC, 0x01,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0464_TCH_OSD_UVC, 0x00,
			0xFFFF, conf_mode, layer_id);
	}

	return CFD_RET_OK;
}

int disp_cfd_hal_set_r2y(uint32_t layer_id,
	uint32_t color_mode, uint32_t conf_mode)
{
	if ((layer_id == 0) || (layer_id == 1)) {
		vCFDWriteFldAlign(VDO_0EAC_FE_DV_WP, (0x1 << 4),
			VDO_0EAC_R2Y_CLO3X3_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_0EAC_FE_DV_WP, (0x1 << 13),
			VDO_0EAC_R2Y_CR_ADD128_POST_EN_FLD,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_0EAC_FE_DV_WP, (0x1 << 14),
			VDO_0EAC_R2Y_CB_ADD128_POST_EN_FLD,
			conf_mode, layer_id);
	} else if ((layer_id == 2) || (layer_id == 3)) {
		//r2y default
		/*vCFDWriteFldAlign(GFX_012C_DV_WP, (0x1 << 4),
			GFX_012C_COL3X3_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_012C_DV_WP, (0x1 << 13),
			GFX_012C_CR_ADD128_POST_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_012C_DV_WP, (0x1 << 14),
			GFX_012C_CB_ADD128_POST_EN_FLD, conf_mode, layer_id);*/
		//cfd sdr2sdr r2y vdo value
		vCFDWriteFldAlign(GFX_012C_DV_WP, 0xE030,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0134_DV_WP, 0x1C0,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0138_DV_WP, 0x1E69,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_013C_DV_WP, 0x1FD7,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0140_DV_WP, 0xBA,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0144_DV_WP, 0x273,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0148_DV_WP, 0x3F,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_014C_DV_WP, 0x1F99,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0150_DV_WP, 0x1EA6,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0154_DV_WP, 0x1C0,
			0xFFFF, conf_mode, layer_id);
	}

	return CFD_RET_OK;
}

int disp_cfd_hal_manual_reset(uint32_t layer_id,
	uint32_t conf_mode)
{
	if ((layer_id == 0) || (layer_id == 1)) {
		vCFDWriteFldAlign(XVYCC_TOP_CONTROL, 0x0,
			0xFFFF, conf_mode, (layer_id + 4));
		vCFDWriteFldAlign(VDO_0EAC_FE_DV_WP, 0x0,
			0xFFFF, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_0710_TOP_FE, 0x0,
			VDO_0710_OOTF_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09F0_DM_FE, 0x0,
			VDO_09F0_DM_DITH_EN_FLD, conf_mode, layer_id);
	} else if ((layer_id == 2) || (layer_id == 3)) {
		// close r2y
		vCFDWriteFldAlign(GFX_012C_DV_WP, 0x0,
			0xFFFF, conf_mode, layer_id);
		// close adl
		vCFDWriteFldAlign(GFX_0460_TCH_OSD_UVC, 0x1,
			GFX_0460_AUTOD_PROTECT_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0460_TCH_OSD_UVC, 0x0,
			GFX_0460_AUTOD_LUT_MD_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0464_TCH_OSD_UVC, 0x0,
			GFX_0464_AUTOD_TRIG_MD_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_064C_TCH_OSD_UVC, 0x0,
			GFX_064C_THDR_OSD_CLK_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_064C_TCH_OSD_UVC, 0x0,
			GFX_064C_THDR_OSD_CLK_EN_SEL_FLD, conf_mode, layer_id);
	}

	return CFD_RET_OK;
}

int disp_cfd_hal_set_top_ctrl(uint32_t layer_id,
	uint32_t conf_mode)
{
	if ((layer_id == 0) || (layer_id == 1)) {
		// top ctrl
		vCFDWriteFldAlign(VDO_0618_TOP_FE, 0x0, 0xFFFF,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_061C_TOP_FE, (0x1 << 1),
			VDO_061C_HDR2IP_PATH_EN,
			conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_0634_TOP_FE, (0x1 << 15),
			VDO_0634_HDR_IN_PATH_AUTO_EN, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_081C_DM_FE, (0x1 << 5),
			VDO_081C_DM_MSB_ALIGN_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09E4_DM_FE, 0x1,
			VDO_09E4_EDCLK_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09E4_DM_FE, (0x2 << 12),
			VDO_09E4_DC0_TO_DST_SEL_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09E8_DM_FE, (0x1 << 12),
			VDO_09E8_1PTO2P_BYP_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09EC_DM_FE, (0x1 << 7),
			VDO_09EC_DM_SRC_SEL_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09EC_DM_FE, 0x0,
			VDO_09EC_HDRIN_SHIFT_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09F0_DM_FE, 0x0,
			VDO_09F0_DM_DITH_EN_FLD, conf_mode, layer_id);
		// adl
		vCFDWriteFldAlign(VDO_09C0_DM_FE, 0x0,
			VDO_09C0_AUTOD_PROTECT_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09C0_DM_FE, (0x1 << 15),
			VDO_09C0_AUTOD_LUT_MD_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(VDO_09C4_DM_FE, (adl_mode),
			VDO_09C4_TRIG_MD_FLD, conf_mode, layer_id);
	} else if ((layer_id == 2) || (layer_id == 3)) {
		// top ctrl
		vCFDWriteFldAlign(GFX_0100_DV_WP, 0x1,
			GFX_0100_CLK_ON_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0100_DV_WP, 0x0,
			GFX_0100_PATH_SEL_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_021C_HDR_FE, (0x1 << 5),
			GFX_021C_HDR_PROC_MSB_ALIGN_EN_FLD,
			conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_03E4_HDR_FE, 0x1,
			GFX_03E4_EDCLK_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_064C_TCH_OSD_UVC, 0x1,
			GFX_064C_THDR_OSD_CLK_EN_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_064C_TCH_OSD_UVC, (0x1 << 1),
			GFX_064C_THDR_OSD_CLK_EN_SEL_FLD, conf_mode, layer_id);
		// adl
		vCFDWriteFldAlign(GFX_0438_TCH_OSD_UVC, (0x1 << 13),
			GFX_0438_LUT_WD_DUP_MD_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0460_TCH_OSD_UVC, 0x0,
			GFX_0460_AUTOD_PROTECT_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0460_TCH_OSD_UVC, (0x1 << 15),
			GFX_0460_AUTOD_LUT_MD_FLD, conf_mode, layer_id);
		vCFDWriteFldAlign(GFX_0464_TCH_OSD_UVC, 0x2800,
			GFX_0464_AUTOD_TRIG_MD0_DELAY_FLD,
			conf_mode, layer_id);
	}
	return CFD_RET_OK;
}

int disp_cfd_hal_get_video_dm_wh(uint32_t layer_id,
	uint32_t *dm_w, uint32_t *dm_h)
{
	if ((dm_w != NULL) && (dm_h != NULL)) {
		*dm_w = iCFDReadREG32(VDO_09CC_DM_FE, layer_id);
		*dm_h = iCFDReadREG32(VDO_09D0_DM_FE, layer_id);
	}

	return CFD_RET_OK;
}

int disp_cfd_hal_get_gfx_dm_h(uint32_t layer_id,
	uint32_t *dm_h)
{
	if (dm_h != NULL)
		*dm_h = iCFDReadREG32(GFX_03D0_HDR_FE, layer_id);

	return CFD_RET_OK;
}

int disp_cfd_hal_get_gfx_h(uint32_t layer_id,
	uint32_t *height)
{
	if (height != NULL)
		*height = iCFDReadREG32(GFX_0468_TCH_OSD_UVC, layer_id);

	return CFD_RET_OK;
}

int disp_cfd_hal_set_cur_res(uint32_t layer_id, uint32_t conf_mode)
{
	uint32_t u4cur_width = 0;
	uint32_t u4cur_height = 0;

	if ((layer_id == 0) || (layer_id == 1)) {
		disp_cfd_drv_get_video_dm_wh(layer_id,
			&u4cur_width, &u4cur_height);
		if ((u4cur_width != hdr_input_width[layer_id]) ||
			(u4cur_height != hdr_input_height[layer_id])) {
			// src active
			vCFDWriteFldAlign(VDO_09CC_DM_FE,
				hdr_input_width[layer_id],
				VDO_09CC_H_SIZE, conf_mode, layer_id);
			vCFDWriteFldAlign(VDO_09D0_DM_FE,
				hdr_input_height[layer_id],
				VDO_09D0_V_SIZE, conf_mode, layer_id);
			cfd_output_info("id %d mode %d, width %x height %x\n",
				layer_id, conf_mode, hdr_input_width[layer_id],
				hdr_input_height[layer_id]);
		}
	} else if ((layer_id == 2) || (layer_id == 3)) {
		disp_cfd_hal_get_gfx_h(layer_id, &u4cur_height);
		if (u4cur_height != hdr_input_height[layer_id]) {
			// only set v size
			vCFDWriteFldAlign(GFX_0468_TCH_OSD_UVC,
				hdr_input_height[layer_id],
				GFX_0468_VLEN, conf_mode, layer_id);
			cfd_output_info("gfx id %d mode %d height %x\n",
				layer_id, conf_mode,
				hdr_input_height[layer_id]);
		}
	}
	return CFD_RET_OK;
}
