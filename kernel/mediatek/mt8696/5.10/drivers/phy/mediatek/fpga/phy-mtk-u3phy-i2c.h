/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 * Author Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

#ifndef _U3PHY_IIC_H_
#define _U3PHY_IIC_H_
#include "mtu3.h"

extern struct device *u3phy_dev;

#define PHY_LOG(fmt, ...)  dev_info(u3phy_dev, fmt, ##__VA_ARGS__)

#define PHY_VERSION_BANK 0x20
#define PHY_VERSION_ADDR 0xe4

/*#define __maybe_unused __attribute__((unused))*/

int phy_writeb(void *port, u8 i2c_addr, u8 addr, u8 value);
u8 phy_readb(void *port, u8 i2c_addr,  u8 addr);
u32 get_phy_version(void *port);

static inline int phy_writel(void *port, u8 i2c_addr, u32 addr, u32 data)
{
	u8 addr8;
	u8 data_0, data_1, data_2, data_3;

	addr8 = addr & 0xff;
	data_0 = data & 0xff;
	data_1 = (data >> 8) & 0xff;
	data_2 = (data >> 16) & 0xff;
	data_3 = (data >> 24) & 0xff;

	phy_writeb(port, i2c_addr, addr8, data_0);
	phy_writeb(port, i2c_addr, addr8 + 1, data_1);
	phy_writeb(port, i2c_addr, addr8 + 2, data_2);
	phy_writeb(port, i2c_addr, addr8 + 3, data_3);

	return 0;
}

static inline u32 phy_readl(void *port, u8 i2c_addr, u32 addr)
{
	u8 addr8;
	u32 data;

	addr8 = addr & 0xff;

	data = phy_readb(port, i2c_addr, addr8);
	data |= (phy_readb(port, i2c_addr, addr8 + 1) << 8);
	data |= (phy_readb(port, i2c_addr, addr8 + 2) << 16);
	data |= (phy_readb(port, i2c_addr, addr8 + 3) << 24);

	return data;
}

static inline u32
phy_readlmsk(void *port, u8 i2c_addr, u32 addr, u32 offset, u32 mask)
{
	return ((phy_readl(port, i2c_addr, addr) & mask) >> offset);
}

static inline int
phy_writelmsk(void *port, u8 i2c_addr, u32 addr, u32 offset, u32 mask, u32 data)
{
	unsigned int old_val;
	unsigned int new_val;

	old_val = phy_readl(port, i2c_addr, addr);
	new_val = (old_val & (~mask)) | ((data << offset) & mask);
	phy_writel(port, i2c_addr, addr, new_val);

	return 0;
}

#endif
