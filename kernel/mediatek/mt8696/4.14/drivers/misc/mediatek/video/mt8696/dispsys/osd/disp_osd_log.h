/*
 * Copyright (C) 2015-2016 MediaTek Inc.
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
#ifndef _H_DRV_OSD_LOG_
#define _H_DRV_OSD_LOG_

#include "disp_osd_debug.h"
#include "disp_osd_if.h"
#ifndef LOG_TAG
#define LOG_TAG
#endif

#define OSD_LOG_D(fmt, args...) pr_info("[OSD]" fmt, ##args)
#define OSD_LOG_I(fmt, args...) pr_info("[OSD]" fmt, ##args)
#define OSD_LOG_W(fmt, args...) pr_info("[OSD]" fmt, ##args)
#define OSD_LOG_E(fmt, args...) pr_info("[OSD]" fmt, ##args)

extern unsigned int osd_dbg_level;
extern bool not_mix_fhd;
extern bool not_mix_uhd;
extern bool fg_debug_update;
extern bool fg_osd_alpha_detect_en;
extern bool fg_osd_debug_dump_en[MAX_OSD_INPUT_CONFIG];

#define OSD_FUNC_LOG (1 << 0)
#define OSD_FLOW_LOG (1 << 1)
#define OSD_COLOR_FORMAT_LOG (1 << 2)
#define OSD_CONFIG_SW_LOG (1 << 3)
#define OSD_WRITE_HW_LOG (1 << 4)
#define OSD_list_LOG (1 << 5)
#define OSD_ION_LOG (1 << 6)
#define OSD_FENCE_LOG (1 << 7)
#define OSD_VYSNC_LOG (1 << 8)

#define OSD_PRINTF(level, string, args...)       \
	do {                                         \
		if (osd_dbg_level & (level)) {           \
			OSD_LOG_I(string, ##args);           \
		}                                        \
	} while (0)

#define OSDIRQ(fmt, args...)                     \
	do {                                         \
		if (osd_irq_log_level())                 \
			OSD_LOG_I(fmt, ##args);              \
	} while (0)

#define OSDDBG(fmt, args...)                     \
	do {                                         \
		if (osd_dbg_log_level())                 \
			OSD_LOG_I(fmt, ##args);              \
	} while (0)

#define OSDMSG(fmt, args...) OSD_LOG_I(fmt, ##args)
#define OSDFUNC() OSD_LOG_I("%s\n", __func__)

#define OSDERR(fmt, args...) OSD_LOG_E(fmt, ##args)

#endif
