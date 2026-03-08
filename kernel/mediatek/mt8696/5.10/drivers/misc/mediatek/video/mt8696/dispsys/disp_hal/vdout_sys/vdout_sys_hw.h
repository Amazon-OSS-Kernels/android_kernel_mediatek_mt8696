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

#ifndef _VDOUT_HW_H_
#define _VDOUT_HW_H_

#include <linux/types.h>

#define VDOUT_CFG_00 0x500 /* 0x1500 */
	#define MIX_IN1_ALPHA (0x1ff << 0)
	#define MIX_IN2_ALPHA (0x1ff << 16)

#define VDOUT_CFG_01 0x504 /* 0x1504 */
	#define MIX_IN1_CHANNEL_SWAP (0xf << 12)
	#define MIX_IN2_CHANNEL_SWAP (0xf << 16)
	#define MIX_IN3_CHANNEL_SWAP (0xf << 20)
	#define MIX_IN4_CHANNEL_SWAP (0xf << 24)

#define VDOUT_CFG_07 0x524 /* 0x1524 */
	#define MIX2HDR_BE_DATA_REORDER (0x7 << 1)

#define RW_VDOUT_CLK 0x5c0
	#define MAIN_CLK_OFF (0x1 << 0)
	#define HDMI_CLK_OFF (0x1 << 4)
	#define VDOUT_CLK_SELF_OPTION (1 << 16)
	#define HD_2FS_148 (0x1 << 23)
	#define VDOUT_CLK_SCL_HD (0x1 << 24)
	#define VDOUT_CLK_SCL_PRGS (0x1 << 25)
	#define VDOUT_CLK_SCL_1080P (0x1 << 26)
	#define SCL_PRGS27M (0x1 << 27)
	#define SCL_HCKSEL (0x1 << 28)
	#define SCL_VCKSEL (0x1 << 29)
	#define HD_HALF (0x1 << 30)

#define VDTCLK_CFG3 0x5cc /* 0x15cc */
	/* set this bit to 1 when use w2d write 480p output*/
	#define VIDEOIN_NEW_SD_SEL (0x01 << 31)
	/*is used to set video clock to 296MHz for mast clock*/
	#define MAST_296M_EN (0x01 << 25)
	#define DIG_296M_EN (0x01 << 24)
	#define P2I_296M_EN	(0x01 << 22)
	/*is used to set video clock to 296MHz for NR*/
	#define NR_296M_EN (0x01 << 21)
	/*is used to set video clock to 296MHz for HDMI*/
	#define RGB2HDMI_296M_EN (0x01 << 19)
	#define VDO4_296M_EN (0x01 << 17)
	#define FMT_296M_EN (0x01 << 16)


#define HDMI_CONFIG_SUB 0x5d4
	#define SELF_OPT_HDMI_SUB (0x1 << 2)
	#define VDOUT_CLK_HDMI_HD_SUB (0x1 << 3)
	#define VDOUT_CLK_HDMI_1080P_SUB (0x1 << 5)
	#define VDOUT_CLK_HDMI_PRGS_SUB (0x1 << 6)
	#define RGB2HDMI_296M_EN_SUB (0x1 << 7)
	#define HDMI_PRGS27M_SUB (0x1 << 8)
	#define HDMI_HDAUD_CLK_SUB (0x1 << 9)
	#define HDMI_422_TO_420 (0x1 << 11)

#define VDOUT_CFG_06 0x520
	#define FHD_HDR_GFX_FE_BYPASS (0x1 << 0 | 0x0 << 1)
	#define FHD_HDR_GFX_FE_ENABLE (0x0 << 0 | 0x1 << 1)
	#define FHD_HDR_GFX_FE_MASK (0x3 << 0)

	#define MMSYS_MIX_SOUT_SEL_MASK (0x1 << 4)
	#define OUTPUT_TO_HDR_VDO_BE (0x0 << 4)
	#define OUTPUT_TO_HDR_VDO_BE_FIFO (0x1 << 4)

	#define MMSYS_MIX_HDR_BE_SEL_MASK (0x1 << 8)
	#define FROM_MMSYS_MIX_SOUT_SEL (0x0 << 8)
	#define FROM_HDR_VDO_BE (0x1 << 8)


#define VDOUT_CFG_02 0x508
	#define M_HDR2MIX_SOUT_PATH_SEL_MASK (0x1 << 0)
	#define MVDO_DATA_TO_MIX (0x0 << 0)
	#define MVDO_DATA_BYPASS_MIX_TO_MIX_OUT_SEL (0x1 << 0)

	#define S_HDR2MIX_SOUT_PATH_SEL_MASK (0x1 << 1)
	#define SVDO_DATA_TO_MIX (0x0 << 1)
	#define SVDO_DATA_BYPASS_MIX_TO_MIX_OUT_SEL (0x1 << 1)

	#define FHD_GFX2MIX_SOUT_PATH_SEL_MASK (0x1 << 2)
	#define FHD_GFX_DATA_TO_MIX (0x0 << 2)
	#define FHD_GFX_DATA_BYPASS_MIX_TO_MIX_OUT_SEL (0x1 << 2)

	#define UHD_GFX2MIX_SOUT_PATH_SEL_MASK (0x1 << 3)
	#define UHD_GFX_DATA_TO_MIX (0x0 << 3)
	#define UHD_GFX_DATA_BYPASS_MIX_TO_MIX_OUT_SEL (0x1 << 3)

	#define MIX_OUT_SEL_MASK (0x7 << 4)
	#define M_HDR2MIX_SOUT_PATH_SEL (0x0 << 4)
	#define S_HDR2MIX_SOUT_PATH_SEL (0x1 << 4)
	#define FHD_GFX2MIX_SOUT_PATH_SEL (0x2 << 4)
	#define UHD_GFX2MIX_SOUT_PATH_SEL (0x3 << 4)
	#define DISP_MIXER_OUTPUT (0x4 << 4)

	#define RGB2HDMI_INPUT_MUX_SEL_MASK (0x1 << 8)
	#define RGB2HDMI_INPUT_FROM_MMSYS_MIX_SOUT (0x1 << 8)
	#define RGB2HDMI_INPUT_FROM_HDR_BE_FIFO_OUTPUT (0x0 << 8)

	#define RGB2HDMI_TIMING_FROM_P2I_MASK (0x1 << 9)
	#define RGB2HDMI_INPUT_FROM_FMTTER (0x0 << 9)
	#define RGB2HDMI_INPUT_FROM_P2I (0x1 << 9)


#define OSD_UHD_CFG0 0x700
	#define UHD_HDR_GFX_FE_BYPASS (0x1 << 0 | 0x0 << 1)
	#define UHD_HDR_GFX_FE_ENABLE (0x0 << 0 | 0x1 << 1)
	#define UHD_HDR_GFX_FE_MASK (0x3 << 0)

#define MMSYS_COMMON_CFG0 0x600
	#define P2I_IN_MUX_SEL_MASK (0x3 << 4)
	#define VDO_BE_FIFO_OUTPUT (0x1 << 4)
	#define SD_PPF_OUTPUT (0x2 << 4)

	#define VM_IN_MUX_SEL_MASK (0xf << 8)
	#define VM_IN_P2I_OUTPUT (0xB << 8)

	#define VM_OUT_MUX_SEL_MASK (0xf << 12)
	#define VM_OUT_P2I_OUTPUT (0xB << 12)

	#define VIDEO_IN_SRC_SEL (0xf << 16)

#define VDTCLK_CONFIG4 0x5d0 /* 0x15d0*/
	#define RGB2HDMI_594M_EN (0x01 << 7)
	#define VDO3_594M_EN (0x01 << 10)
	#define VDO3_296M_EN (0x01 << 11)
	#define VDO3_150HZ_EN (0x01 << 12)
	#define DISPFMT3_OFF (0x01 << 13)
	#define FMT_594M_EN (0x01 << 16)
	#define VDO4_594M_EN (0x01 << 17)

#define VDOUT_SYS_CONFIG_F0 0x5f0 /* 0x15f0*/
	#define VDOUT_SYS_SHADOW_EN (0x1 << 0)
#define VDOUT_SYS_CONFIG_F4 0x5f4 /* 0x15f4*/
	#define VDOUT_SYS_SHADOW_UPDATE (0x1 << 0)
#define VDOUT_SYS_CONFIG_E0 0x5e0 /* 0x15e0*/
	#define VDOUT_SYS_SINGLE_SHADOW_EANEBLE_E0 (0xFFFFFFFF << 0)
#define VDOUT_SYS_CONFIG_E4 0x5e4 /* 0x15e4*/
	#define VDOUT_SYS_SINGLE_SHADOW_EANEBLE_E4 (0xFFFFFF << 0)


#define VDOUT_INT_CLR 0xc50
#define VDOUT_INT_STATUS 0xc54
#endif
