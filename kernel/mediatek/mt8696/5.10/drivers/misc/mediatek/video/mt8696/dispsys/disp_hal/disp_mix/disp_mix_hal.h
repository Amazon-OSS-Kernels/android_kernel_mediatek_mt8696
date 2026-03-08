/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef _DISP_MIX_HAL_H_
#define _DISP_MIX_HAL_H_

#include "disp_reg.h"
#include <linux/printk.h>
#include <linux/types.h>
#include "disp_path.h"

extern struct disp_mix_context_t disp_mix;

#define DISP_MIX_BASE disp_mix.init_param.disp_mix_reg_base

#define DISP_MIX_ENABLE 0x0C
	#define MIX_EN (0x1 << 0)

#define DISP_MIX_TRIG 0x10
	#define MIX_SW_TRIG 0x1

#define DISP_MIX_ROI_SIZE 0x18
	#define ROI_W (0x3fff << 0)
	#define ROI_H (0x1fff << 16)

#define DISP_MIX_DATA_PATH_CTRL 0x1c
	#define OUTPUT_NO_ROUND (0x1 << 3)
	#define SOURCE_RGB_SELECT (0x1 << 7)
	#define L0_LARGE_TIMING_SMALL_REGION_SEL (0x1 << 12)
	#define L1_LARGE_TIMING_SMALL_REGION_SEL (0x1 << 13)
	#define L2_LARGE_TIMING_SMALL_REGION_SEL (0x1 << 14)
	#define L3_LARGE_TIMING_SMALL_REGION_SEL (0x1 << 15)
	#define BACKGROUND_RELAY_MODE (0x7 << 9)

#define DISP_MIX_ROI_BGCLR 0x20
	#define COLOR_BLUE (0xff << 0)
	#define COLOR_GREEN (0xff << 8)
	#define COLOR_RED (0xff << 16)
	#define COLOR_ALPHA (0xff << 24)

#define DISP_MIX_SRC_CTRL 0x24
	#define L0_ENABLE (0x1 << 0)
	#define L1_ENABLE (0x1 << 1)
	#define L2_ENABLE (0x1 << 2)
	#define L3_ENABLE (0x1 << 3)
	#define L0_CONSRC_SEL (0x1 << 4)
	#define L1_CONSRC_SEL (0x1 << 5)
	#define L2_CONSRC_SEL (0x1 << 6)
	#define L3_CONSRC_SEL (0x1 << 7)
	#define FORCE_RELAY_MODE (0x1 << 8)
	#define RELAY_MODE_ENABLE (0x1 << 9)
	#define L0_SRC_SEL (0x3 << 12)
	#define L0_SEL_INTERFACE_L0_SRC (0x0 << 12)
	#define L0_SEL_INTERFACE_L1_SRC (0x1 << 12)
	#define L0_SEL_INTERFACE_L2_SRC (0x2 << 12)
	#define L0_SEL_INTERFACE_L3_SRC (0x3 << 12)
	#define L0_OUT_SEL (0x3 << 14)
	#define OUTPUT_L0_SEL_MIXER_L0_SRC (0x0 << 14)
	#define OUTPUT_L0_SEL_MIXER_L1_SRC (0x1 << 14)
	#define OUTPUT_L0_SEL_MIXER_L2_SRC (0x2 << 14)
	#define OUTPUT_L0_SEL_MIXER_L3_SRC (0x3 << 14)
	#define L1_SRC_SEL (0x3 << 16)
	#define L1_SEL_INTERFACE_L0_SRC (0x0 << 16)
	#define L1_SEL_INTERFACE_L1_SRC (0x1 << 16)
	#define L1_SEL_INTERFACE_L2_SRC (0x2 << 16)
	#define L1_SEL_INTERFACE_L3_SRC (0x3 << 16)
	#define L1_OUT_SEL (0x3 << 18)
	#define OUTPUT_L1_SEL_MIXER_L0_SRC (0x0 << 18)
	#define OUTPUT_L1_SEL_MIXER_L1_SRC (0x1 << 18)
	#define OUTPUT_L1_SEL_MIXER_L2_SRC (0x2 << 18)
	#define OUTPUT_L1_SEL_MIXER_L3_SRC (0x3 << 18)
	#define L2_SRC_SEL (0x3 << 20)
	#define L2_SEL_INTERFACE_L0_SRC (0x0 << 20)
	#define L2_SEL_INTERFACE_L1_SRC (0x1 << 20)
	#define L2_SEL_INTERFACE_L2_SRC (0x2 << 20)
	#define L2_SEL_INTERFACE_L3_SRC (0x3 << 20)
	#define L2_OUT_SEL (0x3 << 22)
	#define OUTPUT_L2_SEL_MIXER_L0_SRC (0x0 << 22)
	#define OUTPUT_L2_SEL_MIXER_L1_SRC (0x1 << 22)
	#define OUTPUT_L2_SEL_MIXER_L2_SRC (0x2 << 22)
	#define OUTPUT_L2_SEL_MIXER_L3_SRC (0x3 << 22)
	#define L3_SRC_SEL (0x3 << 24)
	#define L3_SEL_INTERFACE_L0_SRC (0x0 << 24)
	#define L3_SEL_INTERFACE_L1_SRC (0x1 << 24)
	#define L3_SEL_INTERFACE_L2_SRC (0x2 << 24)
	#define L3_SEL_INTERFACE_L3_SRC (0x3 << 24)
	#define L3_OUT_SEL (0x3 << 26)
	#define OUTPUT_L3_SEL_MIXER_L0_SRC (0x0 << 26)
	#define OUTPUT_L3_SEL_MIXER_L1_SRC (0x1 << 26)
	#define OUTPUT_L3_SEL_MIXER_L2_SRC (0x2 << 26)
	#define OUTPUT_L3_SEL_MIXER_L3_SRC (0x3 << 26)


#define DISP_MIX_LAYER0_CON 0x28
#define DISP_MIX_LAYER1_CON 0x40
#define DISP_MIX_LAYER2_CON 0x58
#define DISP_MIX_LAYER3_CON 0x70
	#define ALPHA_MASK 0xff
	#define ALPHA_BLENDING_ENABLE (0x1 << 8)
	#define CLRFMT_MASK (0xf << 12)
	#define CLRFMT_NON_PREMULTI (0x2 << 12)
	#define CLRFMT_PREMULTI (0x3 << 12)
#define DISP_MIX_LAYER0_SRC_SIZE 0x30
#define DISP_MIX_LAYER1_SRC_SIZE 0x48
#define DISP_MIX_LAYER2_SRC_SIZE 0x60
#define DISP_MIX_LAYER3_SRC_SIZE 0x78
	#define SRC_W (0x3fff << 0)
	#define SRC_H (0x1fff << 16)
	#define REGION_ODD (0x1 << 31)
#define DISP_MIX_LAYER0_OFFSET 0x34
#define DISP_MIX_LAYER1_OFFSET 0x4c
#define DISP_MIX_LAYER2_OFFSET 0x64
#define DISP_MIX_LAYER3_OFFSET 0x7c
	#define X_OFFSET (0x3fff << 0)
	#define Y_OFFSET (0x1fff << 16)
	#define X_OFFSET_ODD (0x1 << 31)

#define DISP_MIX_LAYER0_CCLR 0x3C
#define DISP_MIX_LAYER1_CCLR 0x54
#define DISP_MIX_LAYER2_CCLR 0x6C
#define DISP_MIX_LAYER3_CCLR 0x84
#define DISP_MIX_OUTPUT_CCLR 0x90
	#define HDMI_BALCK (0x1)
	#define HDR_BALCK (0x2)


#define DISP_MIX_FUNC_DCM0 0x120
	#define MIX_UPD_REG_CK_EN (0x1 << 0)
#define DISP_MIX_FUNC_DCM1 0x124
	#define MIX_CK_EN (0x1 << 7)
	#define BG_CLR_CK_EN (0x1 << 9)

extern int no_mix_fhd;
extern int no_mix_uhd;
struct disp_mix_init_param {
	uintptr_t disp_mix_reg_base;
};

struct disp_mix_context_t {
	bool inited;
	struct disp_mix_init_param init_param;
	int log_level;
};

#define DISP_MIX_LOG_D(format...) \
	do {                                             \
		if (disp_mix.log_level >= 2)                 \
			pr_info("[DISPSYS] " format);            \
	} while (0)
#define DISP_MIX_LOG_I(format...) \
	do {                                             \
		if (disp_mix.log_level >= 1)                 \
			pr_info("[DISPSYS] " format);            \
	} while (0)


enum DISP_PATH_TYPE { DISP_MAIN_VDO, DISP_SUB_VDO, DISP_OSD_FHD, DISP_OSD_UHD};

struct DISP_PATH_LAYER_INFO {
	enum DISP_PATH_TYPE type;
	bool enable;
	bool alpha_blending;
	UINT32 alpha_value;
	bool pre_multi;
	UINT32 src_width;
	UINT32 src_height;
	UINT32 x_offset;
	UINT32 y_offset;
	bool large_timing_small_region;
};

struct DISP_PATH_LOCATION_INFO {
	enum DISP_PATH_TYPE layer_type;
	UINT32 layer_location_id;
};

int32_t disp_mix_hal_clock_on_off(bool on);

int32_t disp_mix_hal_dynamic_clk_mgr(bool on);

int32_t disp_mix_hal_mix_enable(bool enable);

int32_t disp_mix_hal_set_dst_wigth_and_height
	(enum HDMI_VIDEO_RESOLUTION resolution, bool config_hw);

int32_t disp_mix_hal_mix_sw_trigger(enum HDMI_VIDEO_RESOLUTION resolution,
				   bool sw_trigger);

int32_t disp_mix_hal_set_layer_location
	(struct DISP_PATH_LOCATION_INFO *location_info);

int32_t disp_mix_hal_layer_control(struct DISP_PATH_LAYER_INFO *layer_info);

int32_t disp_mix_hal_for_layer_swap(struct DISP_PATH_LOCATION_INFO
	*location_info);

int32_t disp_mix_hal_set_background_color(uint32_t background);
int disp_mix_hal_set_log_level(uint32_t level);
int disp_mix_hal_init(struct disp_mix_init_param *param);

int32_t disp_mix_hal_set_black_pattern(bool en);

#endif
