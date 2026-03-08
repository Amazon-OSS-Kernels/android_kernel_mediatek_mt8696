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

#ifndef __DISP_R2R_DEBUG_H__
#define __DISP_R2R_DEBUG_H__

#include "disp_info.h"
#include <linux/kernel.h>
#include <linux/printk.h>

#define R2R_RET_OK              0
#define R2R_RET_ERROR           1
#define R2R_NOT_START           2
#define R2R_RET_INV_ARG         3
#define R2R_RET_UNINIT          4
#define R2R_RET_INV_CMD         5
#define R2R_RET_UPDATE_FAIL     6
#define R2R_RET_READ_ERR        7


extern unsigned int r2r_dbg_level;

#define R2R_ERROR_LOG (1 << 0)
#define R2R_FUNC_LOG (1 << 1)
#define R2R_INFO_LOG (1 << 2)
#define R2R_RPU_LOG (1 << 3)
#define R2R_VSVDB_LOG (1 << 4)


#define r2r_printf(fmt, args...) pr_info("[r2r] "fmt, ##args)

#define r2r_error(format, ...) do { \
	if (1) { \
		r2r_printf("error : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define r2r_func_start() do { \
	if (r2r_dbg_level & (R2R_FUNC_LOG)) { \
		r2r_printf("func| %s start\n", __func__); \
	} \
} while (0)

#define r2r_func_end() do { \
	if (r2r_dbg_level & (R2R_FUNC_LOG)) { \
		r2r_printf("func| %s end\n", __func__); \
	} \
} while (0)

#define r2r_func() do { \
	if (r2r_dbg_level & (R2R_FUNC_LOG)) { \
		r2r_printf("func| %s\n", __func__); \
	} \
} while (0)

#define r2r_func_default() do { \
	if (R2R_FUNC_LOG) { \
		r2r_printf("func| %s\n", __func__); \
	} \
} while (0)

#define r2r_info(format, ...) do { \
	if (r2r_dbg_level & (R2R_INFO_LOG)) { \
		r2r_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define	r2r_default(format, ...) \
{ \
	r2r_printf(""format, ##__VA_ARGS__); \
}

#define printf(format, ...) r2r_printf(format, ##__VA_ARGS__)

#define assert(value) \
{ \
	if ((value) == 0) { \
		r2r_error("%s,%d value %d\n", __func__, __LINE__, value); \
	} \
	ASSERT(value); \
}

void r2r_debug_init(void);
void r2r_debug_deinit(void);


#endif /* __DISP_R2R_DEBUG_H__ */
