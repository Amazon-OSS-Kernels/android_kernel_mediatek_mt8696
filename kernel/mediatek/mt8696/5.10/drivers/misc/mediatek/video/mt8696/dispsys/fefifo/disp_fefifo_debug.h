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

#ifndef __DISP_FEFIFO_DEBUG_H__
#define __DISP_FEFIFO_DEBUG_H__

#include "disp_info.h"
#include <linux/kernel.h>
#include <linux/printk.h>

#define FEFIFO_RET_OK               0
#define FEFIFO_RET_ERROR            1
#define FEFIFO_RET_INV_ARG          2
#define FEFIFO_RET_UNINIT           3
#define FEFIFO_RET_INV_CMD          4
#define FEFIFO_RET_UPDATE_FAIL      5
#define FEFIFO_RET_READ_ERR         6

extern unsigned int fefifo_dbg_level;

#define FEFIFO_ERROR_LOG (1 << 0)
#define FEFIFO_FUNC_LOG (1 << 1)
#define FEFIFO_INFO_LOG (1 << 2)
#define FEFIFO_RPU_LOG (1 << 3)
#define FEFIFO_VSVDB_LOG (1 << 4)


#define fefifo_printf(fmt, args...) pr_info("[fefifo] "fmt, ##args)

#define fefifo_error(format, ...) do { \
	if (1) { \
		fefifo_printf("error : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define fefifo_func_start() do { \
	if (fefifo_dbg_level & (FEFIFO_FUNC_LOG)) { \
		fefifo_printf("func| %s start\n", __func__); \
	} \
} while (0)

#define fefifo_func_end() do { \
	if (fefifo_dbg_level & (FEFIFO_FUNC_LOG)) { \
		fefifo_printf("func| %s end\n", __func__); \
	} \
} while (0)

#define fefifo_func() do { \
	if (fefifo_dbg_level & (FEFIFO_FUNC_LOG)) { \
		fefifo_printf("func| %s\n", __func__); \
	} \
} while (0)

#define fefifo_func_default() do { \
	if (FEFIFO_FUNC_LOG) { \
		fefifo_printf("func| %s\n", __func__); \
	} \
} while (0)

#define fefifo_info(format, ...) do { \
	if (fefifo_dbg_level & (FEFIFO_INFO_LOG)) { \
		fefifo_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define	fefifo_default(format, ...) \
{ \
	fefifo_printf(""format, ##__VA_ARGS__); \
}

#define printf(format, ...) fefifo_printf(format, ##__VA_ARGS__)

#define assert(value) \
{ \
	if ((value) == 0) { \
		fefifo_error("%s,%d value %d\n", __func__, __LINE__, value); \
	} \
	ASSERT(value); \
}

void fefifo_debug_init(void);
void fefifo_debug_deinit(void);


#endif /* __DISP_FEFIFO_DEBUG_H__ */
