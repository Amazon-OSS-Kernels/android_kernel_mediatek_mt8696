/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __HDMI_IOCTL_H__
#define __HDMI_IOCTL_H__

#include <linux/platform_device.h>

struct notify_dev {
	const char *name;
	struct device *dev;
	int index;
	int state;
	int value;

	ssize_t (*print_name)(struct notify_dev *sdev, char *buf);
	ssize_t (*print_state)(struct notify_dev *sdev, char *buf);
};

extern int hdmitx_uevent_dev_register(struct notify_dev *sdev);
extern void hdmitx_uevent_dev_unregister(struct notify_dev *sdev);
extern int notify_uevent_user(struct notify_dev *sdev, int state);

#endif
