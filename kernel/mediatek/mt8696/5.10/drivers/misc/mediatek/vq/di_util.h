/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef MTK_VQ_UTIL_H
#define MTK_VQ_UTIL_H

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/regmap.h>

/* switch define start */
/* default define 0 */
#define VQ_MVA_MAP_VA                   0
#define DI_LOG_ALL                      1
#define VQ_SUPPORT_ION                  0
#define VQ_SUPPORT_IOMMU_ATTACH         1

/* default define 1 */
#define VQ_WAIT_IRQ                     1
#define VQ_WH_TIMING                    1
#define VQ_TIME_CHECK                   0
#define VQ_SUPPORT_LARB                 0
#define VQ_SUPPORT_REGMAP               0
/* switch define end */

#define VQ_INVALID_DW                   0xffffffff
#define VQ_INVALID_DECIMAL              99

#define DI_RET_OK                       0
#define DI_RET_ERR_PARAM                -1
#define DI_RET_ERR_EXCEPTION            -2

#if VQ_TIME_CHECK
#define VQ_TIME_CHECK_COUNT             10
#endif

enum di_log_level {
	DI_LOG_ERROR,
	DI_LOG_WARN,
	DI_LOG_VQ,
	DI_LOG_FLOW,
	DI_LOG_TIME,
	DI_LOG_IRQ,
	DI_LOG_MAX
};

extern unsigned int di_dbg_level;
#define DI_Printf(level, string, args...) \
{ \
	if (di_dbg_level & (1 << level)) { \
		pr_info("[VQ] %s "string, __func__, ##args); \
} \
}

#if VQ_TIME_CHECK
extern unsigned int _au4VqTimeRec[];

#define VQ_TIME_REC(x) \
{ \
	struct timeval TimeRec; \
	do_gettimeofday(&TimeRec); \
	_au4VqTimeRec[x] = TimeRec.tv_sec * 1000000 + TimeRec.tv_usec; \
}
#endif

#if VQ_SUPPORT_REGMAP
#define MTK_VQ_REG_BASE_COUNT       4
#else
#define MTK_VQ_REG_BASE_COUNT       5
#endif

#define MTK_VQ_SUPPORT_FIELD_TYPE(x) \
	((V4L2_FIELD_NONE == (x)) || \
	(V4L2_FIELD_TOP == (x)) || \
	(V4L2_FIELD_BOTTOM == (x)) || \
	(V4L2_FIELD_INTERLACED_TB == (x)) || \
	(V4L2_FIELD_INTERLACED_BT == (x)))



extern unsigned int _vq_reg_base[MTK_VQ_REG_BASE_COUNT];

#if VQ_SUPPORT_REGMAP
extern struct regmap *_vq_regmap;
#endif

#endif				/* MTK_VQ_UTIL_H */
