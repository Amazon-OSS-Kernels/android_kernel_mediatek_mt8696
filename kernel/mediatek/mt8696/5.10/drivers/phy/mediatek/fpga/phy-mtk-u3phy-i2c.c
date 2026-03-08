// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

#include "phy-mtk-u3phy-i2c.h"

#define PHY_TRUE 1
#define PHY_FALSE 0

#define SSUSB_FPGA_I2C_OUT	0x0
#define SSUSB_FPGA_I2C_SDA_OUT	BIT(0)
#define SSUSB_FPGA_I2C_SDA_OEN	BIT(1)
#define SSUSB_FPGA_I2C_SCL_OUT	BIT(2)
#define SSUSB_FPGA_I2C_SCL_OEN	BIT(3)

#define SSUSB_FPGA_I2C_IN	0x4
#define SSUSB_FPGA_I2C_SDA_IN	BIT(0)
#define SSUSB_FPGA_I2C_SCL_IN	BIT(1)

enum i2c_dir {
	I2C_INPUT = 0,
	I2C_OUTPUT,
};

enum i2c_pin {
	I2C_SDA = 0,
	I2C_SCL,
};

#define I2C_DELAY 10

static void i2c_dummy_delay(u32 count)
{
	do {
		count--;
		udelay(1);
	} while (count > 0);
}

static void gpio_set_direction(void *port, enum i2c_pin pin, enum i2c_dir dir)
{
	u32 temp;
	void *addr;

	addr = port + SSUSB_FPGA_I2C_OUT;
	temp = readl(addr);

	if (pin == I2C_SDA) {
		if (dir == I2C_OUTPUT)
			temp |= SSUSB_FPGA_I2C_SDA_OEN;
		else
			temp &= ~SSUSB_FPGA_I2C_SDA_OEN;
	} else {
		if (dir == I2C_OUTPUT)
			temp |= SSUSB_FPGA_I2C_SCL_OEN;
		else
			temp &= ~SSUSB_FPGA_I2C_SCL_OEN;
	}
	writel(temp, addr);
}

static void gpio_set_value(void *port, enum i2c_pin pin, bool value)
{
	u32 temp;
	void *addr;

	addr = port + SSUSB_FPGA_I2C_OUT;
	temp = readl(addr);

	if (pin == I2C_SDA) {
		if (value)
			temp |= SSUSB_FPGA_I2C_SDA_OUT;
		else
			temp &= ~SSUSB_FPGA_I2C_SDA_OUT;
	} else {
		if (value)
			temp |= SSUSB_FPGA_I2C_SCL_OUT;
		else
			temp &= ~SSUSB_FPGA_I2C_SCL_OUT;
	}
	writel(temp, addr);
}

static int gpio_get_value(void *port, enum i2c_pin pin)
{
	u32 temp;
	void *addr;

	addr = port + SSUSB_FPGA_I2C_IN;
	temp = readl(addr);

	if (pin == I2C_SDA)
		temp &= SSUSB_FPGA_I2C_SDA_IN;
	else
		temp &= SSUSB_FPGA_I2C_SCL_IN;

	return !!temp;
}

void i2c_stop(void *port)
{
	gpio_set_direction(port, I2C_SDA, I2C_OUTPUT);
	gpio_set_value(port, I2C_SCL, 0);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SDA, 0);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SDA, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_direction(port, I2C_SCL, I2C_INPUT);
	gpio_set_direction(port, I2C_SDA, I2C_INPUT);
}

/* Prepare the I2C_SDA and I2C_SCL for sending/receiving */
static void i2c_start(void *port)
{
	gpio_set_direction(port, I2C_SCL, I2C_OUTPUT);
	gpio_set_direction(port, I2C_SDA, I2C_OUTPUT);
	gpio_set_value(port, I2C_SDA, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SDA, 0);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 0);
	i2c_dummy_delay(I2C_DELAY);
}

/* return 0 --> ack */
static u32 i2c_send_byte(void *port, u8 data)
{
	int i, ack;

	gpio_set_direction(port, I2C_SDA, I2C_OUTPUT);

	for (i = 8; --i > 0;) {
		gpio_set_value(port, I2C_SDA, (data >> i) & 0x1);
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(port, I2C_SCL,  1);
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(port, I2C_SCL,  0);
		i2c_dummy_delay(I2C_DELAY);
	}
	gpio_set_value(port, I2C_SDA, (data >> i) & 0x1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL,  1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL,  0);
	i2c_dummy_delay(I2C_DELAY);

	gpio_set_value(port, I2C_SDA, 0);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_direction(port, I2C_SDA, I2C_INPUT);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 1);
	i2c_dummy_delay(I2C_DELAY);
	/* ack 1: error, 0:ok */
	ack = gpio_get_value(port, I2C_SDA);
	gpio_set_value(port, I2C_SCL, 0);
	i2c_dummy_delay(I2C_DELAY);

	return (ack == 1) ? PHY_FALSE : PHY_TRUE;
}

static void i2c_receive_byte(void *port, u8 *data, u8 ack)
{
	u32 temp = 0;
	int i;

	gpio_set_direction(port, I2C_SDA, I2C_INPUT);

	for (i = 8; --i >= 0;) {
		temp <<= 1;
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(port, I2C_SCL, 1);
		i2c_dummy_delay(I2C_DELAY);
		temp |= gpio_get_value(port, I2C_SDA);
		gpio_set_value(port, I2C_SCL, 0);
		i2c_dummy_delay(I2C_DELAY);
	}
	gpio_set_direction(port, I2C_SDA, I2C_OUTPUT);
	gpio_set_value(port, I2C_SDA, ack);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 0);
	i2c_dummy_delay(I2C_DELAY);
	*data = (u8)temp;
}

static int i2c_write_reg(void *port, u8 i2c_addr, u8 addr, u8 data)
{
	int ack = 0;

	i2c_start(port);

	ack = i2c_send_byte(port, (i2c_addr << 1) & 0xff);
	if (ack)
		ack = i2c_send_byte(port, addr);
	else
		return PHY_FALSE;

	ack = i2c_send_byte(port, data);
	if (ack) {
		i2c_stop(port);
		return PHY_TRUE;
	}

	return PHY_FALSE;
}

static int i2c_read_reg(void *port, u8 i2c_addr, u8 addr, u8 *data)
{
	int ack = 0;

	i2c_start(port);

	ack = i2c_send_byte(port, (i2c_addr << 1) & 0xff);
	if (ack)
		ack = i2c_send_byte(port, addr);
	else
		return PHY_FALSE;

	i2c_start(port);

	ack = i2c_send_byte(port, ((i2c_addr << 1) & 0xff) | 0x1);
	/* ack 0: ok, 1 error */
	if (ack)
		i2c_receive_byte(port, data, 1);
	else
		return PHY_FALSE;

	i2c_stop(port);
	return ack;
}

int phy_writeb(void *port, u8 i2c_addr, u8 addr, u8 value)
{
	int ret;

	ret = i2c_write_reg(port, i2c_addr, addr, value);
	if (ret == PHY_FALSE) {
		PHY_LOG("Write failed(i2c_addr: %x, addr: 0x%x, val: 0x%x)\n",
		      i2c_addr, addr, value);
		return PHY_FALSE;
	}

	return PHY_TRUE;
}

u8 phy_readb(void *port, u8 i2c_addr,  u8 addr)
{
	u8 buf;
	int ret;

	ret = i2c_read_reg(port, i2c_addr, addr, &buf);
	if (ret == PHY_FALSE) {
		PHY_LOG("Read failed(i2c_addr: %x, addr: 0x%x)\n",
				i2c_addr, addr);
		return ret;
	}

	return buf;
}

u32 get_phy_version(void *port)
{
	u32 version = 0;

	phy_writeb(port, 0x60, 0xff, PHY_VERSION_BANK);

	version = phy_readl(port, 0x60, PHY_VERSION_ADDR);
	PHY_LOG("@ ssusb phy version: %x, i2c_port_base: 0x%p\n",
			version, port);

	return version;
}
