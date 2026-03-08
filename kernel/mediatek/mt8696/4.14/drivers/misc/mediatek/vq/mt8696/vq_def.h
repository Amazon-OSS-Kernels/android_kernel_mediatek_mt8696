/*
 * Copyright (c) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _VQ_DEV_INFO_H_
#define _VQ_DEV_INFO_H_

#ifdef CONFIG_MTK_IN_HOUSE_TEE_SUPPORT
#include <kree/mem.h>
#include <kree/system.h>
#include <tz_cross/ta_mem.h>
#include <tz_cross/trustzone.h>
#endif

#ifdef CONFIG_MTK_NR
#include "nr_hal.h"
#endif

#include "di_hal.h"

#define VQ_COMPATIBLE_NAME		"mediatek,mt8696-vq"

enum MTK_VQ_REG {
	MTK_VQ_REG_MMSYS_CONFIG,
	MTK_VQ_REG_VDOUT,
	MTK_VQ_REG_DISP_TOP,
	MTK_VQ_REG_NR,
	MTK_VQ_REG_DISPFMT_VDO,
	MTK_VQ_REG_WC,
	MTK_VQ_REG_UNKNOWN,
};

enum MTK_VQ_IRQ {
	MTK_VQ_IRQ_WRCH_FRAME_END,
	MTK_VQ_IRQ_NR,
	MTK_VQ_IRQ_DI_VSYNC,
	MTK_VQ_IRQ_DI_FRAME_END,
	MTK_VQ_IRQ_DISP_END,
	MTK_VQ_IRQ_DI_UNDER_RUN,
	MTK_VQ_IRQ_UNKNOWN,
};

struct vq_data {
	struct ion_client *client;
	struct device *dev;
	struct device *larb_dev;
	void __iomem *disp_top_reg_base;

#ifdef CONFIG_MTK_NR
	struct nr_info nr;
#endif
	struct di_info di;

#ifdef CONFIG_MTK_IN_HOUSE_TEE_SUPPORT
	KREE_SESSION_HANDLE vq_session;
#endif
};

#define VQ_INFO(string, args...) \
	DI_Printf(DI_LOG_VQ, string, ##args) \

#define VQ_TIME(string, args...) \
	DI_Printf(DI_LOG_TIME, string, ##args) \

#define VQ_ERR(string, args...) \
	pr_info("[VQ] %s error " string, __func__, ##args)

#define VQ_MSG(string, args...) \
	pr_info("[VQ] %s " string, __func__, ##args)

#endif	 /*_VQ_DEV_INFO_H_*/
