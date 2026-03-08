/* SPDX-License-Identifier: GPL-2.0*/
/* Copyright (c) 2020 MediaTek Inc. */


#ifndef __IMX214_EEPROM_H__
#define __IMX214_EEPROM_H__

#include <linux/types.h>
#include "kd_camera_typedef.h"

#define IMX577_I2C_SPEED       100
#define IMX577_MAX_OFFSET      0xFFFF
#define IMX577_EEPROM_SLAVE_ADDR  0xA8

#define SENSOR_ID_SIZE     32
#define SENSOR_ID_ADDR     0x24

struct EEPROM_DATA_INFO {
	kal_uint16 SensorID_addr;
	unsigned int SensorID_size;
};

enum EEPROM_DATA_INFO_FMT {
	MTK_FMT = 0,
	FB_FMT,
	FMT_MAX
};


/*
 *
 * @param data Buffer
 * @return size of data
 */

unsigned int read_imx577_SensorID(struct subdrv_ctx *ctx, UINT32 *u4data);
#endif

