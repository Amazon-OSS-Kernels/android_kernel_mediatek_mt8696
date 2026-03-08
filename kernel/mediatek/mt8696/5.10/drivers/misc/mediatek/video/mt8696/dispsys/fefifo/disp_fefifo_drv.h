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

#ifndef __DISP_FEFIFO_DRV_H__
#define __DISP_FEFIFO_DRV_H__

#include "disp_info.h"

#include <linux/list.h>
#include <linux/mutex.h>
#include "disp_fefifo_hal.h"
#include "disp_hw_mgr.h"
#include "hdmitx.h"
#define FEFIFO_MAX_LAYER_NUM (4)


struct disp_fefifo_context {
	uintptr_t fefifo_hw_base;
	uint32_t io_reg_base;
	struct mutex lock;
	bool inited;
	bool enabled;
	bool clk_on;
	uint32_t reg_conf_mode;
	uint32_t layer_id;
	uint32_t src_width;
	uint32_t src_heght;
	uint32_t alpha;
	enum HDMI_VIDEO_RESOLUTION res;
	struct FEFIFO_Register_Table *preg_tbl;
	struct cmdqRecStruct *gce_handle;
};
void disp_fefifo_drv_init(uint32_t layer_id, uintptr_t reg_base);
void disp_fefifo_drv_uninit(uint32_t layer_id);
void disp_fefifo_set_src_res(uint32_t layer_id, uint32_t u4width,
	uint32_t u4height, uint32_t conf_md);
void disp_fefifo_drv_get_src_res(uint32_t layer_id,
	uint32_t *pu4width, uint32_t *pu4height);
void disp_fefifo_drv_set_pattern(uint32_t layer_id, uint32_t res,
	uint32_t enable, uint32_t y_data, uint32_t cb_data, uint32_t cr_data);
void disp_fefifo_drv_enable(uint32_t layer_id, bool fgEn,
	uint32_t mode);
void disp_fefifo_drv_set_shadow(uint32_t layer_id,
	bool en_shadow, bool en_trig);
void disp_fefifo_drv_flush_reg(uint32_t layer_id, uint32_t mode);
void disp_fefifo_drv_set_alpha(uint32_t layer_id, uint32_t alpha);
void disp_fefifo_drv_set_input_order(uint32_t layer_id,
	uint32_t order);
int disp_fefifo_dbg_lvl_enable(uint32_t level, uint32_t enable);
int disp_fefifo_set_clk_enable(uint32_t layer_id, uint32_t enable);
void disp_fefifo_drv_clr_irq(uint32_t layer_id, uint32_t irq);
int disp_fefifo_drv_set_conf_mode(uint32_t layer_id,
	uint32_t mode);
void disp_fefifo_drv_read_reg(uint32_t layer_id, uint32_t rel_offset,
	uint32_t size);
void disp_fefifo_drv_write_reg(uint32_t layer_id, uint32_t rel_offset,
	uint32_t value);
struct disp_fefifo_context *fefifo_get_inst(uint32_t layer_id);
int disp_fefifo_drv_gce_trigger(uint32_t layer_id);
int disp_fefifo_drv_set_gce_handle(uint32_t layer_id,
	void *pv_handle);
void disp_fefifo_drv_set_sof(uint32_t layer_id);



#endif
