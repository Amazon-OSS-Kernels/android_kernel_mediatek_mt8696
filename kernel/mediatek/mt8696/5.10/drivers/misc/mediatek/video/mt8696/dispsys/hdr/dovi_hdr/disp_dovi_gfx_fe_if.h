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

#ifndef _DISP_DOVI_GFX_FE_IF_H_
#define _DISP_DOVI_GFX_FE_IF_H_

#include "disp_adl_if.h"

int dovi_gfx_fe_init(char *reg_base[]);
int dovi_gfx_fe_config_lut(uint32_t id, uint32_t *p_gfx_fe_lut);
int dovi_gfx_fe_status(char *reg_base[]);
int dovi_gfx_fe_uninit(void);

extern struct device *dovi_dev;
#endif
