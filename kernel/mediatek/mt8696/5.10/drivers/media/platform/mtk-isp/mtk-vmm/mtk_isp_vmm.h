/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_ISP_VMM_H
#define __MTK_ISP_VMM_H

#include <linux/regulator/consumer.h>
#include <linux/mutex.h>
#include <linux/device.h>

#define MAX_VMM_VOLTAGE 725000

enum vmm_isp_enum {
	VMM_ISP_SENINF,
	VMM_ISP_CAM,
	VMM_ISP_IMG,
	VMM_ISP_MAX,
};

struct isp_vmm_device {
	struct device *dev;
	struct mutex isp_vmm_mutex;
	int isp_engine[VMM_ISP_MAX];
};

int isp_direct_set_voltage(struct regulator *regulator, int min_uV,
			   int max_uV, unsigned int isp_type);

#endif
