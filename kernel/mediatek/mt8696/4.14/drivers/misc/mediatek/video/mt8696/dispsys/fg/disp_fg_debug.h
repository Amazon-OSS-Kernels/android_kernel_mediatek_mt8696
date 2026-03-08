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
		if (strcmp(#base, "int") == 0)                    \
			ret = kstrtoint(tmp, 0, (int *)val);          \
		else if (strcmp(#base, "uint") == 0)              \
			ret = kstrtouint(tmp, 0, (unsigned int *)val);\
		else if (strcmp(#base, "ul") == 0)                \
			ret = kstrtoul(tmp, 0, (unsigned long *)val); \
		if (ret != 0) {                                   \
			FG_ERR(                                   \
	"[ERROR]kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n", \
				ret, __FILE__, __LINE__); \
			action; \
			} \
	} while (0)
#endif

extern void fg_sec_status(void);
#endif /* __DISP_VDP_DEBUG_H__ */
