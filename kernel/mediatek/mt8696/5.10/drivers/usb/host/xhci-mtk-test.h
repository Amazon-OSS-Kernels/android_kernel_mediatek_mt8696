// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek xHCI Host Controller Driver
 *
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Zhanyong Wang <zhanyong.wang@mediatek.com>
 */


#ifndef __XHCI_MTK_TEST_H
#define __XHCI_MTK_TEST_H

#ifdef CONFIG_USB_XHCI_MTK_DEBUGFS
int mu3h_hqa_create_attr(struct device *dev);
void mu3h_hqa_remove_attr(struct device *dev);
#else
static inline int mu3h_hqa_create_attr(struct device *dev)
{
	return 0;
}
static inline void mu3h_hqa_remove_attr(struct device *dev)
{
}
#endif
#endif /* __XHCI_MTK_TEST_H */
