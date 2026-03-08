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

#ifndef _DISP_R2R_HAL_H_
#define _DISP_R2R_HAL_H_

#include <linux/printk.h>
#include <linux/types.h>

#include "disp_reg.h"
#include "vdout_sys_hw.h"
#include "hdmitx.h"
#include "disp_r2r_hw.h"


#define R2R_REG_CONF_CLIENT_RIU      0
#define R2R_REG_CONF_CLIENT_RIU_SHDW 1
#define R2R_REG_CONF_CLIENT_SW_SHDW  2
#define R2R_REG_CONF_CLIENT_GCE      3
#define R2R_REG_CONF_CLIENT_ML       4

#define R2R_FRONT_ADJ_MOD_ADD 0
#define R2R_FRONT_ADJ_MOD_DEL 1
#define R2R_FRONT_ADJ_MOD_REP 2

struct R2R_Register_Table {
	uint32_t depth;
	uint32_t address[HAL_R2R_REG_CONF_CNT];
	uint32_t value[HAL_R2R_REG_CONF_CNT];
	uint32_t mask[HAL_R2R_REG_CONF_CNT];
};

struct disp_r2r_timing {
	uint16_t htotal;
	uint16_t vtotal_lsb;
	uint16_t vtotal_msb;
	uint16_t width;
	uint16_t height;
	uint16_t hoffset;
	uint16_t voffset;
	uint16_t hfront;
	uint16_t vfront;
	uint16_t hsync_w;
	uint16_t vsync_w;
	enum HDMI_VIDEO_RESOLUTION res;
};

struct disp_r2r_pattern {
	uint16_t mode;
	uint16_t ypattern;
	uint16_t cbpattern;
	uint16_t crpattern;
	uint16_t ypattern_b;
	uint16_t cbpattern_b;
	uint16_t crpattern_b;
};

int r2r_hal_isr(uint32_t conf_mode);
void r2r_hal_set_timing(void *data);
void r2r_hal_set_enable(bool fgEnable, uint32_t mode);
void r2r_hal_clr_isr(bool fgEnable, uint32_t mode);
void r2r_hal_set_format(bool is_422, uint32_t bit_dep, uint32_t mode);
void r2r_hal_set_burst_mode(uint32_t bit_dep, uint32_t mode);
void r2r_hal_set_pattern(bool fgEnable, void *pattern, void *data);
void r2r_hal_set_out_swap(uint32_t u4SwapType, uint32_t conf_mode);
void r2r_hal_set_shdw(bool enable, bool sw_trig);
void r2r_hal_set_pitch(uint32_t pitch, uint32_t mode);
void r2r_hal_set_req_threshold(uint32_t req_len, uint32_t mode);
void r2r_hal_conf_frame(uint32_t yaddr, uint32_t caddr,
	uint32_t ccaddr, uint32_t mode);
void r2r_hal_reset(void);
int r2r_hal_init(uintptr_t reg_base);
void r2r_hal_uninit(void);

#endif
