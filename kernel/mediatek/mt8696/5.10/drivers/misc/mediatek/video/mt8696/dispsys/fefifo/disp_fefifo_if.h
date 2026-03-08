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

#ifndef __DISP_FEFIFO_IF_H__
#define __DISP_FEFIFO_IF_H__

#include "disp_info.h"

#include <linux/list.h>
#include <linux/mutex.h>

#include "disp_hw_mgr.h"
#include "hdmitx.h"

#define FEFIFO_DRV_NAME "disp_drv_fefifo"

struct FEFIFO_SRC_ACTIVE_T {
	uint32_t layer_id;
	uint32_t width;
	uint32_t height;
};

struct FEFIFO_SRC_ORDER_T {
	uint32_t layer_id;
	uint32_t in_order;
	uint32_t out_order;
};

int disp_fefifo_start(struct disp_hw_common_info *info,
	unsigned int layer_id);
int disp_fefifo_stop(unsigned int layer_id);

#endif
