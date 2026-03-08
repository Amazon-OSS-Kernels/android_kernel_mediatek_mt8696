/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __HDMI_TX_COD_H__
#define __HDMI_TX_COD_H__

#include <linux/platform_device.h>

extern void cod_test(void);
extern void cod_init(struct platform_device *pdev);
extern void cod_fini(void);

#endif
