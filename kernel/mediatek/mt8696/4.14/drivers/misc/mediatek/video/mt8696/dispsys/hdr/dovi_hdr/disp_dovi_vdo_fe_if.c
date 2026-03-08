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

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>

#include "dovi_log.h"
#include "disp_hw_mgr.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"
#include "dovi_vdo_fe_hal.h"
#include "disp_hdr_if.h"
#include "disp_dovi_vdo_fe_if.h"

uint32_t vdo_fe_init;

int dovi_vdo_fe_init(char *reg_base[])
{

	if (reg_base == NULL) {
		dovi_error("error params in vdofe init\n");
		return -1;
	}

	if (vdo_fe_init) {
		dovi_default("vdo fe already inited\n");
		return 0;
	}

	dovi_vdo_fe_hal_init(reg_base);

	vdo_fe_init = 1;

	return 1;
}


int dovi_vdo_fe_uninit(void)
{
	return 1;
}

int dovi_vdo_fe_status(char *reg_base[])
{
	dovi_vdo_fe_hal_status(reg_base);
	return 1;
}
