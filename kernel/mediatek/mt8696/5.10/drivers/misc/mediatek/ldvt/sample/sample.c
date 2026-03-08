// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <linux/atomic.h>
#include <linux/uaccess.h>

static int __init sample_init(void)
{
return 0;
}

/* should never be called */
static void __exit sample_exit(void)
{
}
module_init(sample_init);
module_exit(sample_exit);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("sample driver");
MODULE_LICENSE("GPL");
