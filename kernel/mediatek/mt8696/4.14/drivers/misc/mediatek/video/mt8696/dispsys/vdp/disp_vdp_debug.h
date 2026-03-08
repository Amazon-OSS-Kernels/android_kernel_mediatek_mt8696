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

#ifndef __DISP_VDP_DEBUG_H__
#define __DISP_VDP_DEBUG_H__

#include "disp_info.h"
#include <linux/kernel.h>

void vdp_debug_init(void);
void vdp_debug_exit(void);

extern int disp_vdp_dbg_level_enable(uint32_t level, uint32_t enable);
extern void disp_vdp_set_not_mix(uint32_t not_mix_type);
extern void disp_vdp_set_not_display(uint32_t not_disp_type);

extern enum DISP_DR_TYPE_T force_dr_range;
extern uint32_t force_decode_allm;
extern uint32_t vdp_disp_test;
extern bool force_dsd_off;
extern bool enable_frame_drop_log;
extern uint32_t dovi_w_drop;

#ifndef STR_CONVERT
#define STR_CONVERT(p, val, base, action)                 \
	do {                                                  \
		int ret = 0;                                      \
		const char *tmp;                                  \
		tmp = strsep(p, ",");                             \
		if (tmp == NULL)                                  \
			break;                                        \
		if (strcmp(#base, "int") == 0)                    \
			ret = kstrtoint(tmp, 0, (int *)val);          \
		else if (strcmp(#base, "uint") == 0)              \
			ret = kstrtouint(tmp, 0, (unsigned int *)val);\
		else if (strcmp(#base, "ul") == 0)                \
			ret = kstrtoul(tmp, 0, (unsigned long *)val); \
		if (ret != 0) {                                   \
			DISP_LOG_E(                                   \
	"[ERROR]kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n", \
				ret, __FILE__, __LINE__); \
			action; \
			} \
	} while (0)
#endif

#endif /* __DISP_VDP_DEBUG_H__ */
