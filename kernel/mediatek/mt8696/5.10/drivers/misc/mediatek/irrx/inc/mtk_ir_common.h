/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_IR_COMMON_H__
#define __MTK_IR_COMMON_H__

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#else
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#endif


struct mtk_ir_msg {
	u32 scancode;
	u32 keycode;
};

struct mtk_ir_mouse_code {
	u32 scanleft;
	u32 scanright;
	u32 scanup;
	u32 scandown;
	u32 scanenter;
	u32 scanwheelup;
	u32 scanwheeldown;
	u32 scanswitch;
#if IS_ENABLED(CONFIG_MTK_IRRX_MOUSE_DRAG)
	u32 dragswitch;
#endif
};

struct mtk_ir_mouse_step {
	u32 x_step_s;
	u32 y_step_s;
	u32 x_step_l;
	u32 y_step_l;
};

enum MTK_IR_DEVICE_MODE {
	MTK_IR_AS_IRRX = 0,
	MTK_IR_AS_MOUSE,
};

enum MTK_IR_MODE {
	MTK_IR_NORMAL,
	MTK_IR_FACTORY = 4,
	MTK_IR_MAX,
	UNKNOWN_BOOT
};

#if IS_ENABLED(CONFIG_MTK_IRRX_MOUSE_DRAG)
enum MTK_IR_MOUSE_MODE {
	MTK_IR_AS_NORMAL_MOUSE = 0,
	MTK_IR_AS_DRAG_MOUSE,
};
#endif

#define BTN_NONE                    0XFFFFFFFF
#define BTN_INVALID_KEY             -1

#define MTK_IR_CHUNK_SIZE sizeof(struct mtk_ir_msg)

#ifdef __KERNEL__

extern int ir_log_debug_on;
extern void mtk_ir_core_log_always(const char *fmt, ...);

#define MTK_IR_DEBUG			1
#define MTK_IR_TAG				"[MTK_IRRX]"
#define MTK_IR_FUN(f) \
	do { \
		if (ir_log_debug_on) \
			pr_info(MTK_IR_TAG"%s++++\n", __func__); \
		else \
			pr_debug(MTK_IR_TAG"%s++++\n", __func__); \
	} while (0)

#define MTK_IR_LOG(fmt, args...) \
	do { \
		if (ir_log_debug_on) \
			pr_info(MTK_IR_TAG fmt, ##args); \
		else if (MTK_IR_DEBUG) \
			pr_debug(MTK_IR_TAG fmt, ##args); \
	} while (0)

#define MTK_IR_ERR(fmt, args...) \
	pr_info(MTK_IR_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define MTK_IR_KEY_LOG(fmt, arg...) \
	mtk_ir_core_log_always("[IRRX_KEY] "fmt, ##arg)

extern void AssertIR(const char *szExpress, const char *szFile, int i4Line);
#undef ASSERT
#define ASSERT(x)	((x) ? (void)0 : AssertIR(#x, __FILE__, __LINE__))

#else

#define MTK_IR_TAG      "[MTK_IR_USER]"
#define MTK_IR_LOG(fmt, args...)    pr_debug(MTK_IR_TAG fmt, ##args)

extern void Assert(const char *szExpress, const char *szFile, int i4Line);

#define ASSERT(x)        ((x) ? (void)0 : Assert(#x, __FILE__, __LINE__))

#endif

#endif

