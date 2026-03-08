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

#ifndef __DISP_BEFIFO_DRV_H__
#define __DISP_BEFIFO_DRV_H__

#include "disp_info.h"

#include <linux/list.h>
#include <linux/mutex.h>

#include "disp_hw_mgr.h"
#include "hdmitx.h"
#include "disp_befifo_hal.h"
#include "disp_befifo_if.h"


struct disp_befifo_context {
	uintptr_t befifo_hw_base;
	uint32_t io_reg_base;
	struct mutex lock;
	bool inited;
	uint32_t irq_no;
	uint32_t reg_conf_mode;
	enum HDMI_VIDEO_RESOLUTION res;
	struct BEFIFO_Register_Table *preg_tbl;
	struct cmdqRecStruct *gce_handle;
};

void disp_befifo_drv_init(uintptr_t reg_base, enum HDMI_VIDEO_RESOLUTION res);
void disp_befifo_drv_uninit(void);
void disp_befifo_drv_set_resolution(uint32_t hw_id,
	enum HDMI_VIDEO_RESOLUTION res);
void disp_befifo_drv_get_resolution(uint32_t hw_id,
	enum HDMI_VIDEO_RESOLUTION *pe_res);
void disp_befifo_drv_enable(bool fgEn, uint32_t mode);
void disp_befifo_drv_flush_reg(uint32_t mode);
int disp_befifo_dbg_level_enable(uint32_t level, uint32_t enable);
void disp_befifo_drv_set_pattern(uint32_t res_id, uint32_t enable,
	uint32_t y_data, uint32_t cb_data, uint32_t cr_data);
int disp_befifo_drv_set_clk_enable(uint32_t enable);
void disp_befifo_drv_set_sof(void);
void disp_befifo_drv_path_sel(void);
void disp_befifo_drv_adj_front(struct BEFIFO_ACTIVE_SHIFT_T *pt_shift);
void disp_befifo_drv_clr_irq(uint32_t irq);
void disp_befifo_drv_set_front(uint32_t front_h, uint32_t front_v);
int disp_befifo_drv_set_conf_mode(uint32_t mode);
void disp_befifo_drv_read_reg(uint32_t rel_offset, uint32_t size);
void disp_befifo_drv_write_reg(uint32_t rel_offset, uint32_t value);
struct disp_befifo_context *befifo_get_inst(void);
int disp_befifo_drv_gce_trigger(void);
int disp_befifo_drv_set_gce_handle(void *pv_handle);
void disp_befifo_drv_enable_crc(uint32_t enable, uint32_t mode);
void disp_befifo_drv_set_output_swap(uint32_t swap_type);

#endif
