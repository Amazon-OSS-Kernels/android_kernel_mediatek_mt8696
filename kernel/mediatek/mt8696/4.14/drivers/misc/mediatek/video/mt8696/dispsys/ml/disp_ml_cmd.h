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

#ifndef __DISP_ML_CMD_H__
#define __DISP_ML_CMD_H__

#include "ml_hal.h"
extern unsigned int ml_dbg_level;

#define ML_LOG_LEVEL1 1
#define ML_LOG_LEVEL2 2
#define ML_LOG_LEVEL3 4
#define ML_TEST_TABLE_SIZE 3200 //400x8bytes


#define ml_printf(fmt, args...) pr_info("[ml] "fmt, ##args)

#define ml_error(format, ...) \
	ml_printf("error : "format, ##__VA_ARGS__)

#define ml_info(format, ...) do { \
	if (ml_dbg_level & (ML_LOG_LEVEL1)) { \
		ml_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define ml_irq(format, ...) do { \
	if (ml_dbg_level & (ML_LOG_LEVEL2)) { \
		ml_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define ml_reg(format, ...) do { \
	if (ml_dbg_level & (ML_LOG_LEVEL3)) { \
		ml_printf("info : "format, ##__VA_ARGS__); \
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
			ml_error("[ERROR]return error: %d\n" \
			"  file : %s, line : %d\n",\
			ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif

void ml_debug_init(void);
void ml_debug_deinit(void);

extern uintptr_t ml_reg_base[ML_REG_ID_MAX];
#endif
