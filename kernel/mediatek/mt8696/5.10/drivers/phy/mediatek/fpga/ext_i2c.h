/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 * Author Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

#ifndef __I2C_V1_H__
#define __I2C_V1_H__

#include "phy-mtk-u3phy-i2c.h"

#define USB_I2C_BASE      (0x11D02000)

/* V1 I2C Register */
#define MT_I2C_DATA_PORT                (USB_I2C_BASE + 0x0000)
#define MT_I2C_SLAVE_ADDR               (USB_I2C_BASE + 0x0004)
#define MT_I2C_INTR_MASK                (USB_I2C_BASE + 0x0008)
#define MT_I2C_INTR_STAT                (USB_I2C_BASE + 0x000c)
#define MT_I2C_CONTROL                  (USB_I2C_BASE + 0x0010)
#define MT_I2C_TRANSFER_LEN             (USB_I2C_BASE + 0x0014)
#define MT_I2C_TRANSAC_LEN              (USB_I2C_BASE + 0x0018)
#define MT_I2C_DELAY_LEN                (USB_I2C_BASE + 0x001c)
#define MT_I2C_TIMING                   (USB_I2C_BASE + 0x0020)
#define MT_I2C_START                    (USB_I2C_BASE + 0x0024)
#define MT_I2C_LTIMING                  (USB_I2C_BASE + 0x002c)
#define MT_I2C_HS                       (USB_I2C_BASE + 0x0030)
#define MT_I2C_IO_CONFIG                (USB_I2C_BASE + 0x0034)
#define MT_I2C_FIFO_ADDR_CLR            (USB_I2C_BASE + 0x0038)
#define MT_I2C_DEBUG                    (USB_I2C_BASE + 0x0044)
#define MT_I2C_CLOCK_DIV                (USB_I2C_BASE + 0x0048)
#define MT_I2C_SOFTRESET                (USB_I2C_BASE + 0x0050)
#define MT_I2C_TRAFFIC                  (USB_I2C_BASE + 0x0054)
#define MT_I2C_SHAPE                    (USB_I2C_BASE + 0x006c)
#define MT_I2C_DEBUGSTAT                (USB_I2C_BASE + 0x00e4)
#define MT_I2C_DEBUGCTRL                (USB_I2C_BASE + 0x00e8)
#define MT_I2C_FIFO_STAT                (USB_I2C_BASE + 0x00f4)
#define MT_I2C_FIFO_THRESH              (USB_I2C_BASE + 0x00f8)
#define MT_I2C_HW_CG_EN                 (USB_I2C_BASE + 0x0f88)

/* MT_I2C_TRANSFER_LEN */
#define I2C_TRANS_LEN_MASK              (0xff)

/* MT_I2C_DELAY_LEN */
#define I2C_DELAY_LEN(x)                (((x) & 0xff) << 0)

/* MT_I2C_TIMING */
#define I2C_TC_STEP_CNT_DIV(x)          (((x) & 0x3f) << 0)
#define I2C_TC_SAMPLE_CNT_DIV(x)        (((x) & 0x7) << 8)

/* MT_I2C_START */
#define I2C_START                       (0x1 << 0)

/* MT_I2C_LTIMING */
#define I2C_LT_LSTEP_CNT_DIV(x)         (((x) & 0x3f) << 0)
#define I2C_LT_LSAMPLE_CNT_DIV(x)       (((x) & 0x7) << 6)
#define I2C_LT_LHS_STEP_CNT_DIV(x)      (((x) & 0x7) << 9)
#define I2C_LT_LHS_SAMPLE_CNT_DIV(x)    (((x) & 0x7) << 12)
#define I2C_LT_HS_HOLD_SEL              (0x1 << 15)

/* MT_I2C_HS */
#define I2C_HS_EN                       (0x1 << 0)
#define I2C_HS_NACK_ERR_DET_EN          (0x1 << 1)
#define I2C_HS_MASTER_CODE(x)           (((x) & 0x7) << 4)
#define I2C_HS_STEP_CNT_DIV(x)          (((x) & 0x7) << 8)
#define I2C_HS_SAMPLE_CNT_DIV(x)        (((x) & 0x7) << 12)

/* MT_I2C_INTR_STAT */
#define I2C_HS_NACKERR                  (0x1 << 2)
#define I2C_ACKERR                      (0x1 << 1)
#define I2C_TRANSAC_COMP                (0x1 << 0)
#define I2C_INTR_MASK           (I2C_TRANSAC_COMP | I2C_ACKERR | I2C_HS_NACKERR)

/* MT_I2C_CONTROL */
#define TRANS_LEN_CHG           (0x1 << 6)
#define ACK_ERR_DET_EN          (0x1 << 5)
#define DIR_CHG                 (0x1 << 4)
#define CLK_EXT                 (0x1 << 3)
#define DMA_EN                  (0x1 << 2)
#define RS_STOP                 (0x1 << 1)
#define I2C_CTRL_MASK           (0x3f << 1)

/* MT_I2C_FIFO_ADDR_CLR */
#define FIFO_ADDR_CLR           (0x1 << 0)

/* MT_I2C_CLOCK_DIV */
#define I2C_CLK_DIV(x)          (((x) & 0x1f) << 0)
#define I2C_CLK_HS_DIV(x)       (((x) & 0x1f) << 8)

/* MT_I2C_SOFTRESET */
#define I2C_SOFT_RST            (0x1 << 0)

/* MT_I2C_FIFO_STAT */
#define FIFO_RD_ADDR(x)         (((x) >> 10) & 0xf)

/* I2C Configuration */

#define I2C_CLK_RATE            13000
#define I2C_FIFO_SIZE           8

#define MAX_ST_MODE_SPEED       100     /* khz */
#define MAX_FS_MODE_SPEED       400     /* khz */
#define MAX_HS_MODE_SPEED       3400    /* khz */

#define MAX_DMA_TRANS_SIZE      252     /* Max(255) aligned to 4 bytes = 252 */
#define MAX_DMA_TRANS_NUM       256

#define MAX_SAMPLE_CNT_DIV      8
#define MAX_STEP_CNT_DIV        64
#define MAX_HS_STEP_CNT_DIV     8

#define I2C_TIMEOUT_TH          200     /* ms, wait for response timeout */

static inline void i2c_set_trans_len(u32 len)
{

}

static inline void i2c_set_trans_ctrl(u32 ctrl)
{

}

static inline void i2c_set_hs_mode(int enable)
{

}

enum i2c_spd_mode {
	ST_MODE = 0,
	FS_MODE,
	HS_MODE,
};

/* I2C Status Code */
#define B_OK 0
#define E_I2C_SET_SPEED_FAIL_OVER_SPEED 0xA001
#define E_I2C_READ_FAIL_ZERO_LENGTH 0xA002
#define E_I2C_READ_FAIL_HS_NACKERR 0xA003
#define E_I2C_READ_FAIL_ACKERR 0xA004
#define E_I2C_READ_FAIL_TIMEOUT  0xA005
#define E_I2C_WRITE_FAIL_ZERO_LENGTH 0xA006
#define E_I2C_WRITE_FAIL_HS_NACKERR 0xA007
#define E_I2C_WRITE_FAIL_ACKERR 0xA008
#define E_I2C_WRITE_FAIL_TIMEOUT  0xA009

#endif
