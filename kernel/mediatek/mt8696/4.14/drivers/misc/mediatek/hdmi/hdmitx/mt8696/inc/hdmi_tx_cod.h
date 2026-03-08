/*
 * Copyright (C) 2021 Amazon.com, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __HDMI_TX_COD_H__
#define __HDMI_TX_COD_H__

#include <linux/platform_device.h>

extern void cod_test(void);
extern void cod_init(struct platform_device *pdev);
extern void cod_fini(void);

#endif
