/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#ifndef __DISP_FG_DEBUG_H__
#define __DISP_FG_DEBUG_H__

#include "disp_info.h"
#include <linux/kernel.h>

void fg_debug_init(void);
void fg_debug_exit(void);

#ifndef FG_STR_CONVERT
#define FG_STR_CONVERT(p, val, base, action)                 \
	do {                                                  \
		int ret = 0;                                      \
		const char *tmp;                                  \
		tmp = strsep(p, ",");                             \
		if (tmp == NULL)                                  \
			break;                                        \
		ret = kstrtouint(tmp, 0, (unsigned int *)val);\
		if (ret != 0) {                                   \
			FG_ERR(                                   \
			"[ERROR]kstrtouint return error: %d\n" \
				"  file : %s, line : %d\n", \
				ret, __FILE__, __LINE__); \
			action; \
			} \
	} while (0)
#endif

enum fg_hw_id;

extern void fg_sec_status(void);
extern void fg_hal_reg_dump(enum fg_hw_id hw_id, u32 len);
void fg_hal_reg_write(u32 addr, u32 val);
#endif /* __DISP_VDP_DEBUG_H__ */
