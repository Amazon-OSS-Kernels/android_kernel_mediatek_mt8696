// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

#include "ext_i2c.h"

/*
 * Set I2C Speed interface: Set internal I2C speed,
 *                          Goal is that get sample_cnt and step_cnt
 * clock: Depends on the current MCU/AHB/APB clock frequency
 * mode:  ST_MODE. (fixed setting for stable I2C transaction)
 * khz:   MAX_ST_MODE_SPEED. (fixed setting for stable I2C transaction)
 *
 * Returns: ERROR_CODE
 */
static u32 i2c_v1_set_speed(u32 clock, enum i2c_spd_mode mode, u32 khz)
{
	u32 ret = B_OK;
	u32 sample_cnt, step_cnt;
	u32 max_step_cnt;
	u32 tmp, sclk;
	u32 diff, min_diff = I2C_CLK_RATE;
	u32 sample = MAX_SAMPLE_CNT_DIV;
	u32 step;

	max_step_cnt = (mode == HS_MODE) ?
				MAX_HS_STEP_CNT_DIV : MAX_STEP_CNT_DIV;
	step = max_step_cnt;

	for (sample_cnt = 1; sample_cnt <= MAX_SAMPLE_CNT_DIV; sample_cnt++) {
		for (step_cnt = 2; step_cnt <= max_step_cnt; step_cnt++) {
			sclk = (clock >> 1) / (sample_cnt * step_cnt);
			if (sclk > khz)
				continue;

			diff = khz - sclk;
			if (diff < min_diff) {
				min_diff = diff;
				sample = sample_cnt;
				step   = step_cnt;
			}
		}
	}
	sample_cnt = sample;
	step_cnt   = step;

	sclk = clock / (2 * sample_cnt * step_cnt);
	if (sclk > khz) {
		ret = E_I2C_SET_SPEED_FAIL_OVER_SPEED;
		return ret;
	}

	step_cnt--;
	sample_cnt--;

	if (mode == HS_MODE) {
		tmp = readl(MT_I2C_HS);
		tmp &= ~(I2C_HS_SAMPLE_CNT_DIV(0x7) |
				I2C_HS_STEP_CNT_DIV(0x7));
		tmp |= I2C_HS_SAMPLE_CNT_DIV(sample_cnt) |
				I2C_HS_STEP_CNT_DIV(step_cnt);
		writel(tmp, MT_I2C_HS);
		i2c_set_hs_mode(1);
	} else {
		tmp = readl(MT_I2C_TIMING);
		tmp &= ~(I2C_TC_SAMPLE_CNT_DIV(0x7) |
				I2C_TC_STEP_CNT_DIV(0x3f));
		tmp |= I2C_TC_SAMPLE_CNT_DIV(sample_cnt) |
				I2C_TC_STEP_CNT_DIV(step_cnt);
		writel(tmp, MT_I2C_TIMING);
		i2c_set_hs_mode(0);
	}

	return ret;
}

/**
 * Initializa the HW I2C module
 * Returns: ERROR_CODE
 */
static u32 i2c_v1_init(void)
{
	u32 ret = B_OK;

	/* Power On I2C Duel, Todo if need */

	/* Reset the HW I2C module */
	 writel(I2C_SOFT_RST, MT_I2C_SOFTRESET);

    /* Set I2C control register */
	i2c_set_trans_ctrl(ACK_ERR_DET_EN | CLK_EXT);

    /* Sset I2C speed mode */
	ret = i2c_v1_set_speed(I2C_CLK_RATE, ST_MODE, MAX_ST_MODE_SPEED);
	if (ret !=  B_OK)
		return ret;

    /* Clear Interrupt status */
	writel(I2C_INTR_MASK, MT_I2C_INTR_STAT);
	writel(0, MT_I2C_TRAFFIC);
	writel(0, MT_I2C_SHAPE);
	writel(I2C_CLK_DIV(0x4) | I2C_CLK_HS_DIV(0x4), MT_I2C_CLOCK_DIV);
	writel(I2C_DELAY_LEN(0x10), MT_I2C_DELAY_LEN);
	writel(I2C_LT_LSTEP_CNT_DIV(0xd) | I2C_LT_LSAMPLE_CNT_DIV(0x1) |
		I2C_LT_LHS_STEP_CNT_DIV(0x4) | I2C_LT_LHS_SAMPLE_CNT_DIV(0x2),
		MT_I2C_LTIMING);

    /* Double Reset the I2C START bit*/
	writel(0, MT_I2C_START);

	PHY_LOG("%s() done\n", __func__);

	return ret;
}

/**
 * Read interface: Read bytes
 *   chip:    I2C chip address, range 0..127 (without RW bit)
 *   buffer:  Where to read/write the data (device address is regarded as data)
 *   len:     How many bytes to read/write
 *
 *   Returns: ERROR_CODE
 */
static u32 i2c_v1_read(u8 chip, u8 *buf, int len)
{
	u32 ret = B_OK;
	u32 status;
	int i;

    /* CHECKME. mt65xx doesn't support len = 0. */
	if (!len)
		return E_I2C_READ_FAIL_ZERO_LENGTH;

	/* bit0 = 1 is to indicate read REQ */
	chip = ((chip << 0x1) | 0x1);

	/* control registers */
	writel(chip, MT_I2C_SLAVE_ADDR);
	i2c_set_trans_len(len);
	writel(0x1, MT_I2C_TRANSAC_LEN);
	writel(I2C_INTR_MASK, MT_I2C_INTR_MASK);
	writel(FIFO_ADDR_CLR, MT_I2C_FIFO_ADDR_CLR);

	i2c_set_trans_ctrl(ACK_ERR_DET_EN | CLK_EXT);

	/* start trnasfer transaction */
	writel(I2C_START, MT_I2C_START);

	/* polling mode : see if transaction complete */
	while (1) {
		status = readl(MT_I2C_INTR_STAT);

		if (status & I2C_TRANSAC_COMP &&
				(!FIFO_RD_ADDR(readl(MT_I2C_FIFO_STAT)))) {
			ret = B_OK;
			break;
		} else if (status & I2C_HS_NACKERR) {
			ret = E_I2C_READ_FAIL_HS_NACKERR;
			break;
		} else if (status & I2C_ACKERR) {
			ret = E_I2C_READ_FAIL_ACKERR;
			break;
		}
	}

	writel(I2C_INTR_MASK, MT_I2C_INTR_STAT);

	if (ret == B_OK) {
		for (i = 0; i < len; i++)
			buf[i] = readb(MT_I2C_DATA_PORT);
	}

	/* clear bit mask */
	writel(0, MT_I2C_INTR_MASK);

	return ret;
}

/**
 * Read interface: Write bytes
 * chip:    I2C chip address, range 0..127  (without RW bit)
 * buffer:  Where to read/write the data (device address is regarded as data)
 * len:     How many bytes to read/write
 *
 * Returns: ERROR_CODE
 */
static u32 i2c_v1_write(u8 chip, u8 *buf, int len)
{
	u32 ret = B_OK;
	u32 status;
	int i;

	/* CHECKME. mt65xx doesn't support len = 0. */
	if (!len)
		return E_I2C_WRITE_FAIL_ZERO_LENGTH;

	/* bit0 = 0 is to indicate write REQ */
	chip = chip << 0x1;

	/* control registers */
	writel(chip, MT_I2C_SLAVE_ADDR);
	i2c_set_trans_len(len);
	writel(0x1, MT_I2C_TRANSAC_LEN);
	writel(I2C_INTR_MASK, MT_I2C_INTR_MASK);
	writel(FIFO_ADDR_CLR, MT_I2C_FIFO_ADDR_CLR);

	i2c_set_trans_ctrl(ACK_ERR_DET_EN | CLK_EXT);

	/* start to write data */
	for (i = 0; i < len; i++)
		writeb(buf[i], MT_I2C_DATA_PORT);

	/* start trnasfer transaction */
	writel(I2C_START, MT_I2C_START);

	/* set timer to calculate time avoid timeout without any reaction */

	/* polling mode : see if transaction complete */
	while (1) {
		status = readl(MT_I2C_INTR_STAT);

		if (status & I2C_TRANSAC_COMP) {
			ret = B_OK;
			break;
		} else if (status & I2C_HS_NACKERR) {
			ret = E_I2C_WRITE_FAIL_HS_NACKERR;
			break;
		} else if (status & I2C_ACKERR) {
			ret = E_I2C_WRITE_FAIL_ACKERR;
			break;
		}
	}

	writel(I2C_INTR_MASK, MT_I2C_INTR_STAT);

	/* clear bit mask */
	writel(0, MT_I2C_INTR_MASK);

	return ret;
}

static u32 usb_i2c_read8_v1(u8 i2c_addr, u8 *cmd, u8 *data)
{
	u32 ret = B_OK;

	ret = i2c_v1_write(i2c_addr, cmd, 1);

	if (ret != B_OK)
		return ret;

	return i2c_v1_read(i2c_addr, data, 1);
}

static u32 usb_i2c_write8_v1(u8 i2c_addr, u8 *cmd, u8 *data)
{
	u8 write_data[2];

	write_data[0] = cmd[0];
	write_data[1] = data[0];

	return i2c_v1_write(i2c_addr, write_data, 2);
}

u8 phy_readb(void *port, u8 i2c_addr,  u8 addr)
{
	u8 cmd_buf = addr;
	u8 data_buf = 0;
	int ret;

	ret = usb_i2c_read8_v1(i2c_addr, &cmd_buf, &data_buf);
	if (ret)
		PHY_LOG("%s() rb failed %x\n", __func__, ret);

	return data_buf;
}

int phy_writeb(void *port, u8 i2c_addr, u8 addr, u8 value)
{
	u8 cmd_buf = addr;
	u8 data_buf = value;
	int ret;

	ret = usb_i2c_write8_v1(i2c_addr, &cmd_buf, &data_buf);
	if (ret)
		PHY_LOG("%s() wb failed %x\n", __func__, ret);

	/* return value: 0 is error, !0 is ok, keep align with internal i2c */
	return !ret;
}

/* port is not used for external I2C */
u32 get_phy_version(void *port)
{
	u32 version = 0;

	i2c_v1_init();

	phy_writeb(port, 0x60, 0xff, PHY_VERSION_BANK);

	version = phy_readl(port, 0x60, PHY_VERSION_ADDR);
	PHY_LOG("ssusb phy version: %x, ext_i2c_base: %p\n",
			version, MT_I2C_DATA_PORT);

	return version;
}

