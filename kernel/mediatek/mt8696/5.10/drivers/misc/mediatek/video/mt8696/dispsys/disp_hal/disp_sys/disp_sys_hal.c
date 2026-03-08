/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include <linux/slab.h>

#include "disp_sys_hal.h"
#include "disp_sys_hw.h"
#include "hdmitx.h"

struct disp_sys_context_t disp_sys;
bool dv_drop_pixel_en[2];

void disp_sys_hal_mvdo_out_select(enum DISP_PATH_HW_ID id, bool enable)
{
	if ((id != DISP_PATH_M_VDO) &&  (id != DISP_PATH_DISP_R2R)
		&& (id != DISP_PATH_DISP_DGI)) {
		DISP_SYS_LOG_I("disp path hw id is wrong, id:%d\n", id);
	}
	if (enable) {
		if (id == DISP_PATH_M_VDO)
			WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_OUT_SELECT),
				DISP_MAIN_OUT_SELECT_VDO3,
				DISP_MAIN_OUT_SELECT_MASK);
		if (id == DISP_PATH_DISP_R2R)
			WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_OUT_SELECT),
				DISP_MAIN_OUT_SELECT_R2R,
				DISP_MAIN_OUT_SELECT_MASK);
		if (id == DISP_PATH_DISP_DGI)
			WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_OUT_SELECT),
				DISP_MAIN_OUT_SELECT_DGI,
				DISP_MAIN_OUT_SELECT_MASK);
	} else
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_OUT_SELECT),
			DISP_MAIN_OUT_SELECT_FIX_REG_OUT,
			DISP_MAIN_OUT_SELECT_MASK);

}
void disp_sys_hal_m_film_grain_enable(bool enable)
{
	DISP_SYS_LOG_I("m film grain enable:%d\n", enable);
	if (enable) {
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_PATH_SEL),
			TO_M_FILM_GRAIN, MVDO_FE_FIFO_SOUT_SEL_MASK);
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_PATH_SEL),
			FORM_M_FILM_GRAIN, M_FILM_GRAIN_PATH_SEL_MASK);
	} else {
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_PATH_SEL),
			TO_M_FILM_GRAIN_SEL, MVDO_FE_FIFO_SOUT_SEL_MASK);
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_PATH_SEL),
			FROM_M_VDO_FE_FIFO_SOUT, M_FILM_GRAIN_PATH_SEL_MASK);
	}
}

void disp_sys_hal_m_hdr_vdo_fe_enable(bool enable)
{
	DISP_SYS_LOG_I("m vdo fe enable:%d\n", enable);
	if (enable) {
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_PATH_SEL),
			TO_M_HDR_VDO_FE, M_FILM_GRAIN_SOUT_SEL_MASK);
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_PATH_SEL),
			FROM_M_HDR_VDO_FE, M_VDO_HDR_PATH_SEL_MASK);
	} else {
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_PATH_SEL),
			TO_M_HDR_SEL, M_FILM_GRAIN_SOUT_SEL_MASK);
		WriteREG32Msk((DISPSYS_BASE + DISP_MAIN_PATH_SEL),
			FROM_M_FILM_GRAIN_SEL, M_VDO_HDR_PATH_SEL_MASK);
	}
}


void disp_sys_hal_svdo_out_select(bool enable)
{

	if (enable) {
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			DISP_SUB_OUT_SELECT_VDO4, DISP_SUB_OUT_SELECT_MASK);
	} else
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			DISP_SUB_OUT_SELECT_FIX_REG_OUT,
			DISP_MAIN_OUT_SELECT_MASK);
}

void disp_sys_hal_s_film_grain_enable(bool enable)
{
	DISP_SYS_LOG_I("s film grain bypass:%d\n", enable);
	if (enable) {
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			TO_S_FILM_GRAIN, SVDO_FE_FIFO_SOUT_SEL_MASK);
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			FORM_S_FILM_GRAIN, S_FILM_GRAIN_PATH_SEL_MASK);
	} else {
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			TO_S_FILM_GRAIN_SEL, SVDO_FE_FIFO_SOUT_SEL_MASK);
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			FROM_S_VDO_FE_FIFO_SOUT, S_FILM_GRAIN_PATH_SEL_MASK);
	}
}

void disp_sys_hal_s_hdr_vdo_fe_enable(bool enable)
{
	DISP_SYS_LOG_I("sub vdo enable:%d\n", enable);
	if (enable) {
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			TO_S_HDR_VDO_FE, S_FILM_GRAIN_SOUT_SEL_MASK);
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			FROM_S_HDR_VDO_FE, S_VDO_HDR_PATH_SEL_MASK);
	} else {
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			TO_S_HDR_SEL, S_FILM_GRAIN_SOUT_SEL_MASK);
		WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
			FROM_S_FILM_GRAIN_SEL, S_VDO_HDR_PATH_SEL_MASK);
	}
}

void disp_sys_hal_shadow_en(bool en)
{
	DISP_SYS_LOG_I("shadow_en: %d\n", en);

	WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CONFIG_68), en,
		      DISP_OUT_SHADOW_EN);

	WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CONFIG_68),
		      DISP_OUT_SHADOW_UPDATE, DISP_OUT_SHADOW_UPDATE);
}

void disp_sys_hal_shadow_update(void)
{
	DISP_SYS_LOG_D("shadow_update be call.\n");

	WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CONFIG_68),
		      DISP_OUT_SHADOW_UPDATE, DISP_OUT_SHADOW_UPDATE);
	WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CONFIG_68), 0,
		      DISP_OUT_SHADOW_UPDATE);
}

void disp_sys_clear_irq(enum DISP_SYS_IRQ_BIT irq)
{
	/*set 1 and 0 to clear interrupt*/
	WriteREG32Msk(DISPSYS_BASE + DISP_INT_CLR, irq, irq);
	WriteREG32Msk(DISPSYS_BASE + DISP_INT_CLR, 0, irq);
}

void disp_sys_clear_irq_all(void)
{
	/*set 1 and 0 to clear interrupt*/
	WriteREG32(DISPSYS_BASE + DISP_INT_CLR, 0xffffffff);
	WriteREG32(DISPSYS_BASE + DISP_INT_CLR, 0);
}

uint32_t disp_sys_read_irq_status(void)
{
	return ReadREG32(MMSYS_BASE + 0x8);
}

int disp_sys_hal_init(struct disp_sys_init_param *param)
{
	if (disp_sys.inited)
		return 0;

	disp_sys.init_param.dispsys_reg_base = param->dispsys_reg_base;
	disp_sys.init_param.mmsys_reg_base = param->mmsys_reg_base;

	if (!disp_sys.init_param.dispsys_reg_base ||
	    !disp_sys.init_param.mmsys_reg_base) {
		DISP_SYS_LOG_I("can not get correct register base.\n");
		return -1;
	}

	disp_sys.sw_disp_sys_reg =
		kmalloc(sizeof(union disp_sys_union_t), GFP_KERNEL);
	if (disp_sys.sw_disp_sys_reg == 0) {
		DISP_SYS_LOG_I("alloc buffer for sw register fail\n");
		return -1;
	}
	memset((void *)(disp_sys.sw_disp_sys_reg), 0,
		   sizeof(union disp_sys_union_t));


	disp_sys.reg_mode = kmalloc(sizeof(uint64_t), GFP_KERNEL);
	if (disp_sys.reg_mode == 0) {
		DISP_SYS_LOG_I("alloc buffer for sw reg mode fail\n");
		return -1;
	}
	*(disp_sys.reg_mode) = 0;

	disp_sys.inited = 1;

	return 0;
}

void disp_sys_hal_deinit(void)
{
	kfree((void *)disp_sys.sw_disp_sys_reg);
	disp_sys.sw_disp_sys_reg = 0;

	kfree((void *)disp_sys.reg_mode);
	disp_sys.reg_mode = 0;
}

int disp_sys_hal_video_preultra_en(enum DISP_TYPE type, bool en)
{
	DISP_SYS_LOG_I("%s, type:%d, en:%d\n", __func__, type, en);
	if (en) {
		if (type == DISP_MAIN) {
			WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CFG_14),
				VDO3_C_HD_LENGTH_PREULTRA,
				VDO3_C_HD_LENGTH_PREULTRA);
			WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CFG_14),
				VDO3_Y_HD_LENGTH_PERULTRA,
				VDO3_Y_HD_LENGTH_PERULTRA);
		} else {
			WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CFG_14),
				VDO4_C_HD_LENGTH_PREULTRA,
				VDO4_C_HD_LENGTH_PREULTRA);
			WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CFG_14),
				VDO4_Y_HD_LENGTH_PERULTRA,
				VDO4_Y_HD_LENGTH_PERULTRA);
		}
	}

	return 0;
}

int disp_sys_hal_video_ultra_en(enum DISP_TYPE type, bool en)
{
	DISP_SYS_LOG_I("%s, type:%d, en:%d\n", __func__, type, en);
	if (en) {
		if (type == DISP_MAIN) {
			WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CFG_14),
				VDO3_C_HD_LENGTH_ULTRA, VDO3_Y_HD_LENGTH_ULTRA);
			WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CFG_14),
				VDO3_Y_HD_LENGTH_ULTRA, VDO3_Y_HD_LENGTH_ULTRA);
		} else {
			WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CFG_14),
				VDO4_C_HD_LENGTH_ULTRA, VDO4_C_HD_LENGTH_ULTRA);
			WriteREG32Msk((DISPSYS_BASE + DISP_SYS_CFG_14),
				VDO4_Y_HD_LENGTH_ULTRA, VDO4_Y_HD_LENGTH_ULTRA);
		}
	}

	return 0;
}

void disp_sys_hal_set_hdr_drop(uint8_t id, bool en)
{
	if (id >= 2) {
		DISP_SYS_LOG_I("%s id error %d\n", __func__, id);
		return;
	}

	if (dv_drop_pixel_en[id] != en) {
		if (en) {
			if (id == DISP_MAIN) {
				disp_sys.sw_disp_sys_reg->rField.MAIN_DROP_2_PIXEL = 1;
			} else {
				disp_sys.sw_disp_sys_reg->rField.SUB_DROP_2_PIXEL = 1;
			}
		} else {
			if (id == DISP_MAIN) {
				disp_sys.sw_disp_sys_reg->rField.MAIN_DROP_2_PIXEL = 0;
			} else {
				disp_sys.sw_disp_sys_reg->rField.SUB_DROP_2_PIXEL = 0;
			}
		}
		REG_SET(*disp_sys.reg_mode, REG_MASK(DISP_SUB_OUT_SELECT / 4));
		dv_drop_pixel_en[id] = en;
		DISP_SYS_LOG_I("dv[%d] drop pixle %d\n", id, en);
	}
}

void disp_sys_hal_set_hdr_drop_shadow(uint8_t id, bool en)
{
	if (id >= 2) {
		DISP_SYS_LOG_I("%s id error %d\n", __func__, id);
		return;
	}

	if (dv_drop_pixel_en[id] != en) {
		if (en) {
			if (id == DISP_MAIN) {
				WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
					MAIN_HDR_DROP_2_PIXEL, MAIN_HDR_DROP_2_PIXEL);
			} else {
				WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
					SUB_HDR_DROP_2_PIXEL, SUB_HDR_DROP_2_PIXEL);
			}
		} else {
			if (id == DISP_MAIN) {
				WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
					0 << 9, MAIN_HDR_DROP_2_PIXEL);
			} else {
				WriteREG32Msk((DISPSYS_BASE + DISP_SUB_OUT_SELECT),
					0 << 8, SUB_HDR_DROP_2_PIXEL);
			}
		}
		dv_drop_pixel_en[id] = en;
		DISP_SYS_LOG_I("dv[%d] drop shadow pixle %d\n", id, en);
	}
}

void disp_sys_hal_update_hw_register(void)
{
	uint32_t u4RegIdx = 0;
	uint64_t regMask = 0;

	for (u4RegIdx = 0, regMask = 1; u4RegIdx < HAL_DISP_SYS_REG_NUM;
		 u4RegIdx++, regMask <<= 1) {
		if (IS_REG_SET(*disp_sys.reg_mode, regMask) &&
			(u4RegIdx == (DISP_SUB_OUT_SELECT / 4))) {
			WriteREG32Msk((disp_sys.init_param.dispsys_reg_base + u4RegIdx * 4),
				disp_sys.sw_disp_sys_reg->au4Reg[u4RegIdx],
				1 << 8);
			WriteREG32Msk((disp_sys.init_param.dispsys_reg_base + u4RegIdx * 4),
				disp_sys.sw_disp_sys_reg->au4Reg[u4RegIdx],
				1 << 9);
			/*
			DISP_SYS_LOG_I("DISP_SYS_REG Reg= 0x%x, Val= 0x%x\n",
				(u4RegIdx * 4),
				disp_sys.sw_disp_sys_reg->au4Reg[u4RegIdx]);
			*/
		}
	}

	*disp_sys.reg_mode = 0;
}

