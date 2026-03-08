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

#ifndef __DISP_IRQ_H__
#define __DISP_IRQ_H__

#include "disp_sys_hal.h"
#include "vdout_sys_hal.h"
#include <linux/device.h>
#include <linux/types.h>

enum DISP_IRQ_E {
	/*vdout sys irq bit*/
	DISP_IRQ_OSD3_FRAME_END	= 0x0 << 31 | VDOUT_SYS_IRQ_OSD3_FRAME_END,
	DISP_IRQ_OSD3_FRAME_START = 0x0 << 31 | VDOUT_SYS_IRQ_OSD3_FRAME_START,
	DISP_IRQ_OSD3_VSYNC	= 0x0 << 31 | VDOUT_SYS_IRQ_OSD3_VSYNC,
	DISP_IRQ_FMT_ACTIVE_START = 0x0 << 31 | VDOUT_SYS_IRQ_FMT_ACTIVE_START,
	DISP_IRQ_FMT_ACTIVE_END = 0x0 << 31 | VDOUT_SYS_IRQ_FMT_ACTIVE_END,
	DISP_IRQ_FMT_VSYNC = 0x0 << 31 | VDOUT_SYS_IRQ_FMT_VSYNC,
	DISP_IRQ_FEFIFO_UHD = 0x0 << 31 | VDOUT_SYS_IRQ_FEFIFO_UHD,
	DISP_IRQ_FEFIFO_FHD = 0x0 << 31 | VDOUT_SYS_IRQ_FEFIFO_FHD,
	DISP_IRQ_BEFIFO = 0x0 << 31 | VDOUT_SYS_IRQ_BEFIFO,
	/*disp sys irq bit*/
	DISP_IRQ_VDO3_UNDERRUN = 0x1 << 31 | DISP_SYS_IRQ_VDO3_UNDERRUN,
	DISP_IRQ_DISP3_VSYNC = 0x1 << 31 | DISP_SYS_IRQ_DISP3_VSYNC,
	DISP_IRQ_DISP4_END = 0x1 << 31 | DISP_SYS_IRQ_DISP4_END,
	DISP_IRQ_VDO4_UNDERRUN = 0x1 << 31 | DISP_SYS_IRQ_VDO4_UNDERRUN,
	DISP_IRQ_DISP4_VSYNC = 0x1 << 31 | DISP_SYS_IRQ_DISP4_VSYNC,
	DISP_IRQ_R2R_VSYNC = 0x1 << 31 | DISP_SYS_IRQ_R2R_VSYNC,
	DISP_IRQ_FEFIFO_VDO3 = 0x1 << 31 | DISP_SYS_IRQ_FEFIFO_VDO3,
	DISP_IRQ_FEFIFO_VDO4 = 0x1 << 31 | DISP_SYS_IRQ_FEFIFO_VDO4
};

#define DISP_IRQ_IS_VDOUT_SYS(irq) ((irq & 0x80000000) == 0)
#define DISP_IRQ_SUBSYS_BIT(irq) (irq & 0x7fffffff)

int disp_irq_manager(enum DISP_IRQ_E irq);
#endif
