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

#ifndef _DISP_FEFIFO_HAL_H_
#define _DISP_FEFIFO_HAL_H_

#include <linux/printk.h>
#include <linux/types.h>

#include "disp_reg.h"
#include "vdout_sys_hw.h"
#include "hdmitx.h"
#include "disp_fefifo_hw.h"


#define FEFIFO_REG_CONF_CLIENT_RIU      0
#define FEFIFO_REG_CONF_CLIENT_RIU_SHDW 1
#define FEFIFO_REG_CONF_CLIENT_SW_SHDW     2
#define FEFIFO_REG_CONF_CLIENT_GCE      3
#define FEFIFO_REG_CONF_CLIENT_ML       4

#define FEFIFO_TG_MODE_RELAY 0
#define FEFIFO_TG_MODE_SELFGEN 1


struct FEFIFO_Register_Table {
	uint32_t depth;
	uint32_t address[HAL_FEFIFO_REG_CONF_CNT];
	uint32_t value[HAL_FEFIFO_REG_CONF_CNT];
	uint32_t mask[HAL_FEFIFO_REG_CONF_CNT];
};

struct disp_fefifo_timing {
	uint16_t mode;
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
};

struct disp_fefifo_pattern {
	uint16_t mode;
	uint16_t ypattern;
	uint16_t cbpattern;
	uint16_t crpattern;
	uint16_t ypattern_b;
	uint16_t cbpattern_b;
	uint16_t crpattern_b;
	uint16_t alpha;
};

void fefifo_hal_clr_isr(uint32_t layer_id, uint32_t irq);
int fefifo_hal_isr(uint32_t layer_id, uint32_t conf_mode);
void fefifo_hal_set_src_active(uint32_t layer_id,
	uint32_t width, uint32_t height, uint32_t conf_md);
void fefifo_hal_set_timing(uint32_t layer_id, void *data);
void fefifo_hal_set_enable(uint32_t layer_id,
	bool fgEnable, uint32_t mode);
void fefifo_hal_set_sof(uint32_t u4sof_s, uint32_t u4sof_e,
	uint32_t u4eof_s, uint32_t u4eof_e);
void fefifo_hal_set_alpha(uint32_t layer_id, uint32_t alpha);
void fefifo_hal_set_pattern(uint32_t layer_id, bool enable,
	void *data, void *pattern);
void fefifo_hal_set_in_swap(uint32_t layer_id, uint32_t u4SwapType);
void fefifo_init_sw_reg(uint32_t layer_id, uintptr_t reg_base);
void fefifo_hal_reset(uint32_t layer_id);
int fefifo_hal_init(uint32_t layer_id, uintptr_t reg_base);
void fefifo_hal_uninit(uint32_t layer_id);
void fefifo_hal_set_shdw(uint32_t layer_id, bool enable, bool fgTrig);

#endif
