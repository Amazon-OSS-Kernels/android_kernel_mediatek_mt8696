// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.
/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 imx709o_eeprom.c
 *
 * Project:
 * --------
 * Description:
 * ------------
 *	 Add APIs to read from EEPROM
 *
 ****************************************************************************/

#define PFX "IMX709O_pdafotp"
#define LOG_INF(format, args...) pr_debug(PFX "[%s] " format, __func__, ##args)

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/slab.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define_v4l2.h"
#include "kd_imgsensor_errcode.h"
#include "imx709omipiraw_Sensor.h"
#include "imx709o_eeprom.h"

#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"

//#define read_cmos_sensor_8(ctx, ...) subdrv_i2c_rd_u8(__VA_ARGS__)
//#define read_cmos_sensor(ctx, ...) subdrv_i2c_rd_u16(__VA_ARGS__)
//#define write_cmos_sensor_8(ctx, ...) subdrv_i2c_wr_u8(__VA_ARGS__)
//#define write_cmos_sensor(...) subdrv_i2c_wr_u16(__VA_ARGS__)
//#define table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u8(__VA_ARGS__)
//#define table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u16(__VA_ARGS__)


static struct EEPROM_PDAF_INFO eeprom_pdaf_info[] = {
	{
		.LRC_addr = OTP_LRC_OFFSET,
		.LRC_size = LRC_SIZE,
	},
};

static DEFINE_MUTEX(gimx709o_eeprom_mutex);

static bool selective_read_eeprom(struct subdrv_ctx *ctx, kal_uint16 addr, BYTE *data)
{

	if (addr > IMX709O_MAX_OFFSET)
		return false;

	if (adaptor_i2c_rd_u8(ctx->i2c_client,
			IMX709O_EEPROM_SLAVE_ADDRESS, addr, data) < 0) {
		return false;
	}
	return true;
}

static bool read_imx709o_eeprom(struct subdrv_ctx *ctx, kal_uint16 addr, BYTE *data, int size)
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


unsigned int read_imx709o_LRC(struct subdrv_ctx *ctx, kal_uint16 *data)
{
	kal_uint16 idx = 0, sensor_startL_reg = 0xCE00, sensor_startR_reg = 0xCF00;
	static BYTE imx709o_LRC_data[LRC_SIZE] = { 0 };
	static unsigned int readed_size;
	struct EEPROM_PDAF_INFO *pinfo = (struct EEPROM_PDAF_INFO *)&eeprom_pdaf_info[0];
	kal_uint8 lrc_flag = 0;

	LOG_INF("read imx709o LRC, otp_offset = %d, size = %u\n",
		pinfo->LRC_addr, pinfo->LRC_size);

	mutex_lock(&gimx709o_eeprom_mutex);
	if ((readed_size == 0) &&
		read_imx709o_eeprom(ctx, pinfo->LRC_addr,
			imx709o_LRC_data, pinfo->LRC_size)) {
		readed_size = pinfo->LRC_size;
	}
	mutex_unlock(&gimx709o_eeprom_mutex);

	for (idx = 0; idx < LRC_SIZE; idx++) {
		if (idx < LRC_SIZE/2) {
			//LRC_Left
			data[2 * idx] = sensor_startL_reg++;
		} else {
			//LRC_Right
			data[2 * idx] = sensor_startR_reg++;
		}
		data[2 * idx + 1] = imx709o_LRC_data[idx];
	}

	for (idx = 0; idx < LRC_SIZE; idx++) {
		if (idx < LRC_SIZE/2) {
			//LRC_Left
			pr_debug("In %s: LRC_Left value[0x%x]:0x%x",
				__func__, data[2 * idx], data[2 * idx + 1]);
		} else {
			//LRC_RIGHT
			pr_debug("In %s: LRC_Right value[0x%x]:0x%x",
				__func__, data[2 * idx], data[2 * idx + 1]);
		}
	}
	read_imx709o_eeprom(ctx, 0x162E, &lrc_flag, 1);
	pr_info("LRC flag0x%x[1:valid, other:Invalid]", lrc_flag);
	return readed_size;
}

unsigned int read_imx709o_QSC(struct subdrv_ctx *ctx, kal_uint16 *data)
{
	kal_uint16 idx = 0, sensor_qsc_address = 0x1000;
	kal_uint8 tmp_QSC_setting[QSC_SIZE];
	kal_uint8 qsc_ver = 0;

	read_imx709o_eeprom(ctx, OTP_QSC_OFFSET, tmp_QSC_setting, QSC_SIZE);
	for (idx = 0; idx < QSC_SIZE; idx++) {
		data[2 * idx] = sensor_qsc_address;
		data[2 * idx + 1] = tmp_QSC_setting[idx];
		sensor_qsc_address += 1;
	}

	for (idx = 0; idx < QSC_SIZE; idx++) {
		pr_debug("alex qsc data imx709o_QSC_setting[0x%x] = 0x%x",
			data[2 * idx], data[2 * idx + 1]);
	}

	read_imx709o_eeprom(ctx, 0x2A32, &qsc_ver, 1);
	pr_info("QSC Version: 0x%x", qsc_ver);
	return 0;
}
