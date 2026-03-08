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

#ifndef _DISP_BEFIFO_HAL_H_
#define _DISP_BEFIFO_HAL_H_

#include <linux/printk.h>
#include <linux/types.h>

#include "disp_reg.h"
#include "vdout_sys_hw.h"
#include "hdmitx.h"
#include "disp_befifo_hw.h"

#define BEFIFO_REG_CONF_CLIENT_RIU      0
#define BEFIFO_REG_CONF_CLIENT_RIU_SHDW 1
#define BEFIFO_REG_CONF_CLIENT_SW_SHDW     2
#define BEFIFO_REG_CONF_CLIENT_GCE      3
#define BEFIFO_REG_CONF_CLIENT_ML       4

#define BEFIFO_FRONT_ADJ_MOD_ADD 0
#define BEFIFO_FRONT_ADJ_MOD_DEL 1
#define BEFIFO_FRONT_ADJ_MOD_REP 2


struct BEFIFO_Register_Table {
	uint32_t depth;
	uint32_t address[HAL_BEFIFO_REG_CONF_CNT];
	uint32_t value[HAL_BEFIFO_REG_CONF_CNT];
	uint32_t mask[HAL_BEFIFO_REG_CONF_CNT];
};

struct disp_befifo_timing {
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

struct disp_befifo_pattern {
	uint16_t mode;
	uint16_t ypattern;
	uint16_t cbpattern;
	uint16_t crpattern;
	uint16_t ypattern_b;
	uint16_t cbpattern_b;
	uint16_t crpattern_b;
};

int befifo_hal_isr(uint32_t conf_mode);
void befifo_hal_set_timing(void *data, uint32_t mode);
void befifo_hal_set_enable(bool fgEnable, uint32_t mode);
void befifo_hal_clr_isr(bool fgEnable, uint32_t mode);
void befifo_hal_set_hdelay(uint32_t hDelta, uint32_t pos);
void befifo_hal_set_vdelay(uint32_t vDelta, uint32_t pos);
void befifo_hal_set_front(uint32_t hfront, uint32_t vfront);
void befifo_hal_set_threshold(void);
void befifo_hal_set_background(uint32_t colorY,
	uint32_t colorCb, uint32_t colorCr);
void befifo_hal_set_pattern(void *data, bool fgEnable, void *pattern);
void befifo_hal_set_out_swap(uint32_t u4SwapType);
void befifo_hal_set_shdw(bool enable, bool fgTrig);
void befifo_init_sw_reg(uintptr_t reg_base);
void befifo_hal_reset(void);
int befifo_hal_init(uintptr_t reg_base);
void befifo_hal_uninit(void);
void befifo_hal_enable_crc(bool fgEnable, uint32_t mode);
#ifdef DISP_QMS_SUPPORT
void befifo_hal_set_vtotal_ext(bool fgEnable,
	uint32_t value, uint32_t mode);
#endif


#endif
