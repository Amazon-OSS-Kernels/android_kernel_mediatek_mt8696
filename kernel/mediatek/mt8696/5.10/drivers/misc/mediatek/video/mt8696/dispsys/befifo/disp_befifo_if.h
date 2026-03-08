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

#ifndef __DISP_BEFIFO_IF_H__
#define __DISP_BEFIFO_IF_H__

#include "disp_info.h"

#include <linux/list.h>
#include <linux/mutex.h>

#include "disp_hw_mgr.h"
#include "hdmitx.h"

#define BEFIFO_DRV_NAME "disp_drv_befifo"

struct BEFIFO_ACTIVE_SHIFT_T {
	uint32_t hshitf;
	uint32_t hpos;
	uint32_t vshitf;
	uint32_t vpos;
};

#endif
