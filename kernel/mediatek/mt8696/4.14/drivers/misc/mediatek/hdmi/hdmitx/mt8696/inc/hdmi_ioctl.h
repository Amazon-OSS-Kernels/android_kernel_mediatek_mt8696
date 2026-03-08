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
