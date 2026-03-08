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

#ifndef _DOVI_CORE2_HAL_H_
#define _DOVI_CORE2_HAL_H_

#include "dovi_gfx_fe_hw.h"

int dovi_gfx_fe_hal_init(char *reg_base[]);
int dovi_gfx_fe_hal_uninit(void);
int dovi_gfx_fe_hal_status(char *reg_base[]);
int dovi_gfx_fe_hal_isr(void);
int dovi_gfx_fe_hal_set_enable(uint8_t id, uint32_t enable);
uint32_t dovi_gfx_fe_hal_control_config(struct disp_hw_resolution *info);
int dovi_gfx_fe_hal_config_lut(uint32_t lut_addr);
int dovi_gfx_fe_hal_config_reg(uint32_t *p_gfx_fe_reg);

#define IS_DISP_WITH_EXT(mode) ( \
	(mode == HDMI_VIDEO_720x480i_60Hz) || \
	(mode == HDMI_VIDEO_720x480p_60Hz)) \

extern uint32_t dovi_idk_test;
extern unsigned int g_force_dolby;
extern uint32_t hdr_input_width[4];
extern uint32_t hdr_input_height[4];

#endif				/*  */
