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


#ifndef _DOVI_BE_HAL_H_
#define _DOVI_BE_HAL_H_

#include "dovi_be_hw.h"
#include "disp_hw_mgr.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"

enum dovi_be_out_mode {
	DOVI_CORE3_IPT444_BYPASS,
	DOVI_CORE3_IPT422_TUNNELED,
	DOVI_CORE3_HDR10,
	DOVI_CORE3_SDR10,
	DOVI_CORE3_SDR,
	DOVI_CORE3_YCC422_TUNNELED
};

int dovi_be_hal_init(char *reg_base[]);
int dovi_be_hal_isr(void);
int dovi_be_hal_status(char *reg_base[]);
int dovi_be_hal_config_reg(uint32_t *p_be_reg);
int dovi_be_hal_set_enable(uint32_t enable);
int dovi_be_hal_dither_bypass(uint32_t bypass);
int dovi_be_hal_set_out_mode(enum dovi_be_out_mode mode);
int dovi_be_hal_set_out_fix_pattern_enable(bool enable);

extern uint32_t dovi_idk_test;
extern bool be_bypass_dither;
extern uint32_t g_force_dolby;

#endif
