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

#ifndef _DISP_SYS_HW_H_
#define _DISP_SYS_HW_H_

#include <linux/types.h>

#define HAL_DISP_SYS_REG_NUM (0x20 / 4)

#define MMSYS_MEM_PD_SW0 0x58
#define MMSYS_MEM_PD_DRAM2AXI (0x3 << 0)

#define DISP_INT_CLR 0x0000
#define DISP_INT_STATUS 0x0004

#define DISP_MAIN_OUT_SELECT 0x0014
	#define DISP_MAIN_OUT_SELECT_MASK (0x3 << 16)
	#define DISP_MAIN_OUT_SELECT_VDO3 (0x0 << 16)
	#define DISP_MAIN_OUT_SELECT_R2R (0x1 << 16)
	#define DISP_MAIN_OUT_SELECT_DGI (0x2 << 16)
	#define DISP_MAIN_OUT_SELECT_FIX_REG_OUT (0x3 << 16)

#define DISP_MAIN_PATH_SEL 0x002C
	#define MVDO_FE_FIFO_SOUT_SEL_MASK (0x3 << 0)
	#define TO_M_FILM_GRAIN (0x0 << 0)
	#define TO_M_FILM_GRAIN_SEL (0x1 << 0)

	#define M_FILM_GRAIN_PATH_SEL_MASK (0x1 << 2)
	#define FORM_M_FILM_GRAIN (0x0 << 2)
	#define FROM_M_VDO_FE_FIFO_SOUT (0x1 << 2)

	#define M_FILM_GRAIN_SOUT_SEL_MASK (0x1 << 3)
	#define TO_M_HDR_VDO_FE (0x0 << 3)
	#define TO_M_HDR_SEL (0x1 << 3)

	#define M_VDO_HDR_PATH_SEL_MASK (0x3 << 4)
	#define FROM_MVDO_FE_FIEO_SOUT (0x0 << 4)
	#define FROM_M_FILM_GRAIN_SEL (0x1 << 4)
	#define FROM_M_HDR_VDO_FE (0x2 << 4)

#define DISP_SUB_OUT_SELECT 0x18
	#define DISP_SUB_OUT_SELECT_MASK (0x3 << 16)
	#define DISP_SUB_OUT_SELECT_VDO4 (0x0 << 16)
	#define DISP_SUB_OUT_SELECT_R2R (0x1 << 16)
	#define DISP_SUB_OUT_SELECT_FIX_REG_OUT (0x2 << 16)

	#define SVDO_FE_FIFO_SOUT_SEL_MASK (0x3 << 0)
	#define TO_S_FILM_GRAIN (0x0 << 0)
	#define TO_S_FILM_GRAIN_SEL (0x1 << 0)

	#define S_FILM_GRAIN_PATH_SEL_MASK (0x1 << 2)
	#define FORM_S_FILM_GRAIN (0x0 << 2)
	#define FROM_S_VDO_FE_FIFO_SOUT (0x1 << 2)

	#define S_FILM_GRAIN_SOUT_SEL_MASK (0x1 << 3)
	#define TO_S_HDR_VDO_FE (0x0 << 3)
	#define TO_S_HDR_SEL (0x1 << 3)

	#define S_VDO_HDR_PATH_SEL_MASK (0x3 << 4)
	#define FROM_SVDO_FE_FIEO_SOUT (0x0 << 4)
	#define FROM_S_FILM_GRAIN_SEL (0x1 << 4)
	#define FROM_S_HDR_VDO_FE (0x2 << 4)
	#define MAIN_HDR_DROP_2_PIXEL (0x1 << 9)
	#define SUB_HDR_DROP_2_PIXEL (0x1 << 8)

#define DISP_SYS_CFG_14 0x0014
	#define VDO3_C_HD_LENGTH_ULTRA (0x1 << 20)
	#define VDO3_Y_HD_LENGTH_ULTRA (0x1 << 21)
	#define VDO3_C_HD_LENGTH_PREULTRA (0x1 << 22)
	#define VDO3_Y_HD_LENGTH_PERULTRA (0x1 << 23)
	#define VDO4_C_HD_LENGTH_ULTRA (0x1 << 25)
	#define VDO4_Y_HD_LENGTH_ULTRA (0x1 << 26)
	#define VDO4_C_HD_LENGTH_PREULTRA (0x1 << 27)
	#define VDO4_Y_HD_LENGTH_PERULTRA (0x1 << 28)



#define DISP_HDR2SDR_DCM_CTRL 0x0024

#define DISP_SYS_CONFIG_80 0x80
#define DISP_SYS_CONFIG_84 0x84
	#define DISP_SYS_PD_MVDO_FE_PD0 (0x3ffff << 0)
	#define DISP_SYS_PD_MVDO_DISPFMT3 (0x7f << 18)

#define DISP_SYS_CONFIG_68 0x0068
	#define DISP_OUT_SHADOW_EN (0x1 << 0)
	#define DISP_OUT_SHADOW_UPDATE (0x1 << 1)

struct disp_sys_field_t {
	/* DWORD - 000 */
	uint32_t u4YHCOEF0 : 32;

	/* DWORD - 004 */
	uint32_t u4YHCOEF1 : 32;

	/* DWORD - 008 */
	uint32_t u4YHCOEF2 : 32;

	/* DWORD - 00C */
	uint32_t u4YHCOEF3 : 32;

	/* DWORD - 010 */
	uint32_t u4YHCOEF4 : 32;

	/* DWORD - 014 */
	uint32_t u4YHCOEF5 : 32;

	/* DWORD - 018 */
uint32_t: 8;
	uint32_t SUB_DROP_2_PIXEL : 1;
	uint32_t MAIN_DROP_2_PIXEL : 1;
uint32_t: 22;
};

union disp_sys_union_t {
	uint32_t au4Reg[HAL_DISP_SYS_REG_NUM];
	struct disp_sys_field_t rField;
};

#endif
