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

#ifndef _DISP_CFD_HAL_H_
#define _DISP_CFD_HAL_H_

#include <linux/printk.h>
#include <linux/types.h>

#include "disp_reg.h"
#include "vdout_sys_hw.h"
#include "hdmitx.h"
#include "disp_cfd_hw.h"


#define CFD_REG_CONF_CLIENT_RIU      0
#define CFD_REG_CONF_CLIENT_RIU_SHDW 1
#define CFD_REG_CONF_CLIENT_SW_SHDW     2
#define CFD_REG_CONF_CLIENT_GCE      3
#define CFD_REG_CONF_CLIENT_ML       4
extern uint32_t hdr_input_width[4];
extern uint32_t hdr_input_height[4];
extern uint32_t adl_mode;

struct CFD_Register_Table {
	uint32_t depth;
	uint32_t address[HAL_CFD_REG_CONF_CNT];
	uint16_t value[HAL_CFD_REG_CONF_CNT];
	uint16_t mask[HAL_CFD_REG_CONF_CNT];
};

int cfd_hal_init(uint32_t layer_id, uintptr_t reg_base);
void cfd_hal_uninit(uint32_t layer_id);
int disp_cfd_hal_set_src_res(uint32_t layer_id, uint32_t src_width,
	uint32_t src_height, uint32_t conf_mode);
int vCFDWriteFldAlign(uint32_t addr, uint16_t val, uint16_t msk,
	uint32_t conf_mode, uint32_t client);
int disp_cfd_hal_isr(uint32_t layer_id, uint32_t conf_mode);
int disp_cfd_hal_set_bypass(uint32_t layer_id, uint32_t conf_mode);
int disp_cfd_hal_set_r2y(uint32_t layer_id, uint32_t color_mode,
	uint32_t conf_mode);
int disp_cfd_hal_manual_reset(uint32_t layer_id,
	uint32_t conf_mode);
int disp_cfd_hal_set_top_ctrl(uint32_t layer_id,
	uint32_t conf_mode);
int disp_cfd_hal_get_video_dm_wh(uint32_t layer_id,
	uint32_t *dm_w, uint32_t *dm_h);
int disp_cfd_hal_get_gfx_dm_h(uint32_t layer_id,
	uint32_t *dm_h);
int disp_cfd_hal_get_gfx_h(uint32_t layer_id, uint32_t *height);
int disp_cfd_hal_set_cur_res(uint32_t layer_id, uint32_t conf_mode);
#endif
