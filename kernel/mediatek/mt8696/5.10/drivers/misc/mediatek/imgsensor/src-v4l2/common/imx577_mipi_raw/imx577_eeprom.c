// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2020 MediaTek Inc.


#define PFX "IMX577_eeprom"

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/types.h>

//#include "kd_camera_typedef.h"
#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"

#include "imx577_eeprom.h"

#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)

#define Sleep(ms) mdelay(ms)

static struct EEPROM_DATA_INFO eeprom_data_info[] = {
	{/* MTK_FMT */
		.SensorID_addr = SENSOR_ID_ADDR,
		.SensorID_size = SENSOR_ID_SIZE,
	},
	{/* FB_FMT */
		.SensorID_addr = SENSOR_ID_ADDR,
		.SensorID_size = SENSOR_ID_SIZE,
	},
};

static DEFINE_MUTEX(gimx577_eeprom_mutex);

static bool selective_read_eeprom(struct subdrv_ctx *ctx,
	kal_uint16 addr, BYTE *data)
{
	if (addr > IMX577_MAX_OFFSET)
		return false;

	if (adaptor_i2c_rd_u8(ctx->i2c_client,
			IMX577_EEPROM_SLAVE_ADDR >> 1, addr, data) < 0)
		return false;

	return true;
}

static bool read_imx577_eeprom(struct subdrv_ctx *ctx,
	kal_uint16 addr, BYTE *data, int size)
{
	int i = 0;
	int offset = addr;

	/*LOG_INF("enter read_eeprom size = %d\n", size);*/
	for (i = 0; i < size; i++) {
		if (!selective_read_eeprom(ctx, offset, &data[i]))
			return false;
		/*LOG_INF("read_eeprom 0x%0x %d\n", offset, data[i]);*/
		offset++;
	}
	return true;
}

static struct EEPROM_DATA_INFO *get_eeprom_data_info(struct subdrv_ctx *ctx)
{
	static struct EEPROM_DATA_INFO *pinfo;

	mutex_lock(&gimx577_eeprom_mutex);

	if (pinfo == NULL)
		pinfo = &eeprom_data_info[FB_FMT];

	mutex_unlock(&gimx577_eeprom_mutex);

	return pinfo;
}

unsigned int read_imx577_SensorID(struct subdrv_ctx *ctx, UINT32 *u4data)
{
	static BYTE IMX577_SensorID_data[SENSOR_ID_SIZE] = { 0 };
	static unsigned int readed_size;
	struct EEPROM_DATA_INFO *pinfo = get_eeprom_data_info(ctx);
	int i = 0;

	//ctx->i2c_write_id = IMX577_EEPROM_SLAVE_ADDR;
	LOG_INF("----read sensorID, slave_addr=0x%x, id_addr = 0x%x, size = %u\n",
		IMX577_EEPROM_SLAVE_ADDR, pinfo->SensorID_addr, pinfo->SensorID_size);

	mutex_lock(&gimx577_eeprom_mutex);
	if ((readed_size == 0) &&
			read_imx577_eeprom(ctx,
							pinfo->SensorID_addr,
							IMX577_SensorID_data,
							pinfo->SensorID_size)) {
		readed_size = pinfo->SensorID_size;
	}
	mutex_unlock(&gimx577_eeprom_mutex);

	for (i = 0; i < SENSOR_ID_SIZE; i++) {
		LOG_INF("----read IMX577 SensorID data[%d]=0x%x\n",
			i, IMX577_SensorID_data[i]);
	}

	/* TODO: how to check the sensor ID  format*/
	*u4data = (((IMX577_SensorID_data[0] & 0xFF) << 8) |
				(IMX577_SensorID_data[1] & 0xFF));
	//memcpy(data, IMX577_SensorID_data, pinfo->SensorID_size);
	return readed_size;
}

