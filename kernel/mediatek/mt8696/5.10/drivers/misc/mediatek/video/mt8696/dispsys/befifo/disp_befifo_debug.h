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

#ifndef __DISP_BEFIFO_DEBUG_H__
#define __DISP_BEFIFO_DEBUG_H__

#include "disp_info.h"
#include <linux/kernel.h>
#include <linux/printk.h>

#define BEFIFO_RET_OK               0
#define BEFIFO_RET_ERROR            1
#define BEFIFO_RET_INV_ARG          2
#define BEFIFO_RET_UNINIT           3
#define BEFIFO_RET_INV_CMD          4
#define BEFIFO_RET_UPDATE_FAIL      5
#define BEFIFO_RET_READ_ERR         6
#define BEFIFO_ERROR_LOG (1 << 0)
#define BEFIFO_FUNC_LOG (1 << 1)
#define BEFIFO_INFO_LOG (1 << 2)
#define BEFIFO_ISR_LOG (1 << 3)


extern unsigned int befifo_dbg_level;

#define befifo_printf(fmt, args...) pr_info("[befifo] "fmt, ##args)

#define befifo_error(format, ...) do { \
	if (1) { \
		befifo_printf("error : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define befifo_func_start() do { \
	if (befifo_dbg_level & (BEFIFO_FUNC_LOG)) { \
		befifo_printf("func| %s start\n", __func__); \
	} \
} while (0)

#define befifo_func_end() do { \
	if (befifo_dbg_level & (BEFIFO_FUNC_LOG)) { \
		befifo_printf("func| %s end\n", __func__); \
	} \
} while (0)

#define befifo_func() do { \
	if (befifo_dbg_level & (BEFIFO_FUNC_LOG)) { \
		befifo_printf("func| %s\n", __func__); \
	} \
} while (0)

#define befifo_func_default() do { \
	if (BEFIFO_FUNC_LOG) { \
		befifo_printf("func| %s\n", __func__); \
	} \
} while (0)

#define befifo_info(format, ...) do { \
	if (befifo_dbg_level & (BEFIFO_INFO_LOG)) { \
		befifo_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define befifo_isr(format, ...) do { \
	if (befifo_dbg_level & (BEFIFO_ISR_LOG)) { \
		befifo_printf("isr : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define	befifo_default(format, ...) \
{ \
	befifo_printf(""format, ##__VA_ARGS__); \
}

#define printf(format, ...) befifo_printf(format, ##__VA_ARGS__)

#define assert(value) \
{ \
	if ((value) == 0) { \
		befifo_error("%s,%d value %d\n", __func__, __LINE__, value); \
	} \
	ASSERT(value); \
}

void befifo_debug_init(void);
void befifo_debug_deinit(void);


#endif /* __DISP_BEFIFO_DEBUG_H__ */
