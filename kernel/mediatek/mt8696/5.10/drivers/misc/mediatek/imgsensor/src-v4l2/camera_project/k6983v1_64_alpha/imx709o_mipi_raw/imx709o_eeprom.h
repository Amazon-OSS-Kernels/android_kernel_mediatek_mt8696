/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 imx766o_eeprom.h
 *
 * Project:
 * --------
 * Description:
 * ------------
 *	 Add APIs to read from EEPROM
 *
 ****************************************************************************/

#ifndef __IMX709O_EEPROM_H__
#define __IMX709O_EEPROM_H__

#include "kd_camera_typedef.h"

#include "adaptor-subdrv.h"

#define Sleep(ms) mdelay(ms)

#define IMX709O_EEPROM_SLAVE_ADDRESS 0xA8
#define IMX709O_MAX_OFFSET      0xFFFF

#define OTP_LRC_OFFSET 0x152A
#define LRC_SIZE 260

#define OTP_QSC_OFFSET 0x0E00
#define QSC_SIZE 1560

struct EEPROM_PDAF_INFO {
	kal_uint16 LRC_addr;
	unsigned int LRC_size;
};

/*
 * LRC
 *
 * @param data Buffer
 * @return size of data
 */
unsigned int read_imx709o_LRC(struct subdrv_ctx *ctx, kal_uint16 *data);

unsigned int read_imx709o_QSC(struct subdrv_ctx *ctx, kal_uint16 *data);

#endif

