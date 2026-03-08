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

#ifndef _DOVI_VDO_FE_HAL_H_
#define _DOVI_VDO_FE_HAL_H_

#include "dovi_vdo_fe_hw.h"
#include "disp_hw_mgr.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"
#include "dovi_vdo_fe_hw.h"

#define HDR_CTRL_HW_VERSION   0x0 // dual layer


extern unsigned int g_force_dolby;
extern uint32_t hdr_input_width[4];
extern uint32_t hdr_input_height[4];

int dovi_vdo_fe_hal_config_reg(uint32_t *p_vdo_fe_reg);
int dovi_vdo_fe_hal_config_lut(uint32_t lut_addr);
int dovi_vdo_fe_hal_set_enable(uint8_t id, uint32_t enable);
int dovi_vdo_fe_hal_isr(void);
int dovi_vdo_fe_hal_init(char *reg_base[]);
int dovi_vdo_fe_hal_uninit(void);
int dovi_vdo_fe_hal_set_composer_mode(bool fgCompEL);

int dovi_vdo_fe_hal_status(char *reg_base[]);
int dovi_vdo_fe_hal_set_out_fix_pattern_enable(bool enable,
uint32_t pattern_i,
uint32_t pattern_cp,
uint32_t pattern_ct);

#endif				/*  */
