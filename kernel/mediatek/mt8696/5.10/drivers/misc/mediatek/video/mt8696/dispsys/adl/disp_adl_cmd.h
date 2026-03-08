/*
 * Copyright (C) 2020 MediaTek Inc.
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

#ifndef __DISP_AUTODOWNLOAD_CMD_H__
#define __DISP_AUTODOWNLOAD_CMD_H__

#include "adl_hal.h"
#include "disp_adl_if.h"

extern unsigned int adl_dbg_level;
#define ADL_LOG_LEVEL1 1
#define ADL_LOG_LEVEL2 2
#define ADL_LOG_LEVEL3 4

#define adl_printf(fmt, args...) pr_info("[adl] "fmt, ##args)

#define adl_error(format, ...) \
	adl_printf("error : "format, ##__VA_ARGS__)

#define adl_info(format, ...) do { \
	if (adl_dbg_level & (ADL_LOG_LEVEL1)) { \
		adl_printf(""format, ##__VA_ARGS__); \
	} \
} while (0)

#define adl_dbg(format, ...) do { \
	if (adl_dbg_level & (ADL_LOG_LEVEL2)) { \
		adl_printf(""format, ##__VA_ARGS__); \
	} \
} while (0)

#define adl_irq(format, ...) do { \
	if (adl_dbg_level & (ADL_LOG_LEVEL3)) { \
		adl_printf(""format, ##__VA_ARGS__); \
	} \
} while (0)


#ifndef STR_CVT
#define STR_CVT(p, val, base, action)\
		do {			\
			int ret = 0;	\
			const char *tmp;	\
			const char A[8] = "int32_t"; \
			const char B[9] = "uint32_t"; \
			const char C[9] = "uint64_t"; \
			tmp = strsep(p, ","); \
			if (tmp == NULL) \
				break; \
			if (strcmp(#base, A) == 0)\
				ret = kstrtoint(tmp, 0, (int *)val); \
			else if (strcmp(#base, B) == 0)\
				ret = kstrtouint(tmp, 0, (unsigned int *)val); \
			else if (strcmp(#base, C) == 0)\
				ret = kstrtoul(tmp, 0, (unsigned long *)val); \
			if (ret != 0) {\
				adl_error("[ERROR]return error: %d\n" \
				"  file : %s, line : %d\n",\
				ret, __FILE__, __LINE__);\
				action; \
			} \
		} while (0)
#endif

#ifndef STR_CVT_U32
#define STR_CVT_U32(p, val, action)\
	do {			\
		int ret = 0;	\
		const char *tmp;	\
		tmp = strsep(p, ","); \
		if (tmp == NULL) \
			break; \
		ret = kstrtouint(tmp, 0, (unsigned int *)val); \
		if (ret != 0) {\
			adl_error("[ERROR]return error: %d\n" \
				"  file : %s, line : %d\n",		\
				ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif




void adl_debug_init(void);
void adl_debug_deinit(void);
extern uintptr_t adl_clt_regbase[ADL_CLIT_REG_MAX];
#endif

