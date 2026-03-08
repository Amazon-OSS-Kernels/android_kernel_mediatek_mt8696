// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2020 MediaTek Inc.

/********************Modify Following Strings for Debug***********************/
#define PFX "IMX577_camera_sensor"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__
/****************************   Modify end    *******************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "imx577mipiraw_Sensor.h"
#include "imx577_eeprom.h"

#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"


#define read_cmos_sensor(...) subdrv_i2c_rd_u8(__VA_ARGS__)
//#define read_cmos_sensor(ctx, ...) subdrv_i2c_rd_u16(__VA_ARGS__)
#define write_cmos_sensor(...) subdrv_i2c_wr_u8(__VA_ARGS__)
//#define write_cmos_sensor(ctx, ...) subdrv_i2c_wr_u16(__VA_ARGS__)
#define imx577_table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u8(__VA_ARGS__)
//#define table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u16(__VA_ARGS__)

#define LOG_INF(format, args...)    \
	pr_info(PFX "[%s] " format, __func__, ##args)

#define MULTI_WRITE 0
static int first_n_frame;

static struct imgsensor_info_struct imgsensor_info = {

	.sensor_id = IMX577_SENSOR_ID,

	.checksum_value = 0x58e19b16,

	.pre = {
		.pclk = 420000000,
		.linelength = 4512,
		.framelength = 3102,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4056,
		.grabwindow_height = 3040,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 398400000,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 420000000,
		.linelength = 4512,
		.framelength = 3102,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4056,
		.grabwindow_height = 3040,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 398400000,
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 420000000,
		.linelength = 4512,
		.framelength = 3102,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4056,
		.grabwindow_height = 3040,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 398400000,
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 420000000,
		.linelength = 4512,
		.framelength = 3102,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4056,
		.grabwindow_height = 3040,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 398400000,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 420000000,
		.linelength = 4512,
		.framelength = 3102,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4056,
		.grabwindow_height = 3040,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 398400000,
		.max_framerate = 300,
	},
	.slim_video = {
		.pclk = 420000000,
		.linelength = 4512,
		.framelength = 3102,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4056,
		.grabwindow_height = 3040,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 398400000,
		.max_framerate = 300,
	},
	.custom1 = {
		.pclk = 420000000,
		.linelength = 4512,
		.framelength = 3102,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4056,
		.grabwindow_height = 3040,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 398400000,
		.max_framerate = 300,

	},
	.custom2 = {
		.pclk = 420000000,
		.linelength = 4512,
		.framelength = 3102,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4056,
		.grabwindow_height = 3040,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 398400000,
		.max_framerate = 300,
	},
	.margin = 22,
	.min_shutter = 2,
	.min_gain = 64,
	.max_gain = 24576,
	.min_gain_iso = 100,
	.exp_step = 2,
	.gain_step = 16,
	.gain_type = 0,
	/* max framelength by sensor register's limitation */
	.max_frame_length = 0xff00,
	.ae_shut_delay_frame = 0,

	/* shutter delay frame for AE cycle,
	 * 2 frame with ispGain_delay-shut_delay=2-0=2
	 */
	.ae_sensor_gain_delay_frame = 0,

	/* sensor gain delay frame for AE cycle,
	 * 2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	 */
	.ae_ispGain_delay_frame = 2, /* isp gain delay frame for AE cycle */

	/* The delay frame of setting frame length	*/
	.frame_time_delay_frame = 3,

	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.sensor_mode_num = 7,	/* support sensor mode num */

	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */
	.hs_video_delay_frame = 2, /* enter high speed video  delay frame num */
	.slim_video_delay_frame = 2,	/* enter slim video delay frame num */
	.custom1_delay_frame = 2,
	.custom2_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_6MA,	/* mclk driving current */

	/* sensor_interface_type */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,

	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,

	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	/* sensor output first pixel color */
	.mclk = 24,	/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_4_LANE,	/* mipi lane num */

/* record sensor support all write id addr, only supprt 4must end with 0xff */
	.i2c_addr_table = {0x20, 0x34, 0xff},
	.i2c_speed = 400,
};

static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] = {
	{4056, 3040, 0, 0, 4056, 3040, 4056, 3040,
	 0000, 0000, 4056, 3040, 0, 0, 4056, 3040},
	{4056, 3040, 0, 0, 4056, 3040, 4056, 3040,
	 0000, 0000, 4056, 3040, 0, 0, 4056, 3040},
	{4056, 3040, 0, 0, 4056, 3040, 4056, 3040,
	 0000, 0000, 4056, 3040, 0, 0, 4056, 3040},
	{4056, 3040, 0, 0, 4056, 3040, 4056, 3040,
	 0000, 0000, 4056, 3040, 0, 0, 4056, 3040},
	{4056, 3040, 0, 0, 4056, 3040, 4056, 3040,
	 0000, 0000, 4056, 3040, 0, 0, 4056, 3040},
	{4056, 3040, 0, 0, 4056, 3040, 4056, 3040,
	 0000, 0000, 4056, 3040, 0, 0, 4056, 3040},
	{4056, 3040, 0, 0, 4056, 3040, 4056, 3040,
	 0000, 0000, 4056, 3040, 0, 0, 4056, 3040},

};

static void set_dummy(struct subdrv_ctx *ctx)
{
	/*
	 * LOG_INF("dummyline = %d, dummypixels = %d\n",
	 * ctx->dummy_line, ctx->dummy_pixel);
	 */

	write_cmos_sensor(ctx, 0x0104, 0x01);

	write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
	write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
	//write_cmos_sensor(ctx, 0x0342, ctx->line_length >> 8);
	//write_cmos_sensor(ctx, 0x0343, ctx->line_length & 0xFF);

	write_cmos_sensor(ctx, 0x0104, 0x00);
}

static kal_uint32 return_sensor_id(struct subdrv_ctx *ctx)
{
	kal_uint32 ret = 0x00;

	LOG_INF("0x16 = %x\n", read_cmos_sensor(ctx, 0x0016));
	LOG_INF("0x17 = %x\n", read_cmos_sensor(ctx, 0x0017));

	ret = (read_cmos_sensor(ctx, 0x0016) & 0xFF) << 8 | (read_cmos_sensor(ctx, 0x0017) & 0xFF);
	return ret;
}

static void set_max_framerate(struct subdrv_ctx *ctx, UINT16 framerate, kal_bool min_framelength_en)
{
	/* kal_int16 dummy_line; */
	kal_uint32 frame_length = ctx->frame_length;
	/* unsigned long flags; */

	LOG_INF("framerate = %d, min framelength should enable %d\n",
		framerate, min_framelength_en);

	frame_length = ctx->pclk / framerate * 10 / ctx->line_length;
	ctx->frame_length =
	    (frame_length > ctx->min_frame_length)
	    ? frame_length : ctx->min_frame_length;

	ctx->dummy_line =
		ctx->frame_length - ctx->min_frame_length;

	/* dummy_line = frame_length - ctx->min_frame_length; */
	/* if (dummy_line < 0) */
	/* ctx->dummy_line = 0; */
	/* else */
	/* ctx->dummy_line = dummy_line; */
	/* ctx->frame_length = frame_length + ctx->dummy_line; */
	if (ctx->frame_length > imgsensor_info.max_frame_length) {
		ctx->frame_length = imgsensor_info.max_frame_length;

		ctx->dummy_line =
			ctx->frame_length - ctx->min_frame_length;
	}
	if (min_framelength_en)
		ctx->min_frame_length = ctx->frame_length;
	set_dummy(ctx);
}				/*      set_max_framerate  */

static void set_shutter(struct subdrv_ctx *ctx, kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;

	ctx->shutter = shutter;

	/* write_shutter(shutter); */
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK
	 * to get exposure larger than frame exposure
	 */
	/* AE doesn't update sensor gain at capture mode,
	 * thus extra exposure lines must be updated here.
	 */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length,
	 * should extend frame length first
	 */
	if (shutter > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;
	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;

	shutter =
(shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;

	shutter =
	(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
	? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk
			/ ctx->line_length * 10 / ctx->frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(ctx, 0x0104, 0x01);
			write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
		      write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
			write_cmos_sensor(ctx, 0x0104, 0x00);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(ctx, 0x0104, 0x01);
		write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
		write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
		write_cmos_sensor(ctx, 0x0104, 0x00);
	}

	/* Update Shutter */
	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */
	write_cmos_sensor(ctx, 0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x0203, shutter & 0xFF);
	write_cmos_sensor(ctx, 0x0104, 0x00);
	LOG_INF("Exit! shutter =%d, framelength =%d, auto_extend=%d\n",
		shutter, ctx->frame_length, read_cmos_sensor(ctx, 0x0350));
}				/*    set_shutter */


/*************************************************************************
 * FUNCTION
 *	set_gain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *	iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint16 set_gain(struct subdrv_ctx *ctx, kal_uint16 gain)
{
	kal_uint16 reg_again = 64;
	kal_uint16 reg_dgain = 256;

	/* gain = total gain */
	/* reg_again = analog gain */
	/* reg_dgain = digital gain  */
	/* Total gain = reg_again x reg_dgain */

	/* 64 corresponds to 1x, 1424 => 22.25x again */
	if (gain < BASEGAIN || gain > 16 * 1424) {
		LOG_INF("Error gain setting");
		if (gain < BASEGAIN)
			gain = BASEGAIN;
	/* dgain up tp 16x */
		else if (gain > 16 * 1424)
			gain = 16 * 1424;
	}

	if (gain <= 1424) {
		reg_again = 1024 - 1024*64/gain;
	} else {
	/*start using dgain for more than 22.25x */
		reg_again = 978;
		reg_dgain = gain*256/1424;
	}

	ctx->gain = gain;
	LOG_INF("gain = %d, reg_again = 0x%x, reg_dgain = 0x%x\n ",
		gain, reg_again, reg_dgain);

	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x0204, (reg_again >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x0205, reg_again & 0xFF);
	write_cmos_sensor(ctx, 0x020E, (reg_dgain >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x020F, reg_dgain & 0xFF);

	write_cmos_sensor(ctx, 0x0104, 0x00);

	return gain;

}				/*      set_gain  */

static void set_mirror_flip(struct subdrv_ctx *ctx, kal_uint8 image_mirror)
{
	kal_uint8 iTemp;

	LOG_INF("image_mirror = %d\n", image_mirror);
	iTemp = read_cmos_sensor(ctx, 0x0101);
	iTemp &= ~0x03;		/* Clear the mirror and flip bits. */

	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor(ctx, 0x0101, iTemp | 0x00);
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor(ctx, 0x0101, iTemp | 0x01);
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor(ctx, 0x0101, iTemp | 0x02);
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor(ctx, 0x0101, iTemp | 0x03);
		break;
	default:
		LOG_INF("Error image_mirror setting\n");
	}
}

/*************************************************************************
 * FUNCTION
 *	night_mode
 *
 * DESCRIPTION
 *	This function night mode of sensor.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void night_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
/*No Need to implement this function*/
}				/*      night_mode      */

static kal_uint32 get_sensor_temperature(struct subdrv_ctx *ctx)
{
	UINT8 temperature;
	INT32 temperature_convert;

	temperature = read_cmos_sensor(ctx, 0x013A);

	if (temperature >= 0xEC)
		temperature_convert = temperature - 0x100;
	else if (temperature >= 0 && temperature <= 0x50)
		temperature_convert = temperature;
	else
		temperature_convert = 0xFFFF;

	LOG_INF("read temperature [%d]\n", temperature_convert);

	return temperature_convert;
}

static void sensor_init(struct subdrv_ctx *ctx)
{
	write_cmos_sensor(ctx, 0x0136, 0x18);
	write_cmos_sensor(ctx, 0x0137, 0x00);
	write_cmos_sensor(ctx, 0x3C7E, 0x01);
	write_cmos_sensor(ctx, 0x3C7F, 0x02);
	write_cmos_sensor(ctx, 0x38A8, 0x1F);
	write_cmos_sensor(ctx, 0x38A9, 0xFF);
	write_cmos_sensor(ctx, 0x38AA, 0x1F);
	write_cmos_sensor(ctx, 0x38AB, 0xFF);
	write_cmos_sensor(ctx, 0x55D4, 0x00);
	write_cmos_sensor(ctx, 0x55D5, 0x00);
	write_cmos_sensor(ctx, 0x55D6, 0x07);
	write_cmos_sensor(ctx, 0x55D7, 0xFF);
	write_cmos_sensor(ctx, 0x55E8, 0x07);
	write_cmos_sensor(ctx, 0x55E9, 0xFF);
	write_cmos_sensor(ctx, 0x55EA, 0x00);
	write_cmos_sensor(ctx, 0x55EB, 0x00);
	write_cmos_sensor(ctx, 0x575C, 0x07);
	write_cmos_sensor(ctx, 0x575D, 0xFF);
	write_cmos_sensor(ctx, 0x575E, 0x00);
	write_cmos_sensor(ctx, 0x575F, 0x00);
	write_cmos_sensor(ctx, 0x5764, 0x00);
	write_cmos_sensor(ctx, 0x5765, 0x00);
	write_cmos_sensor(ctx, 0x5766, 0x07);
	write_cmos_sensor(ctx, 0x5767, 0xFF);
	write_cmos_sensor(ctx, 0x5974, 0x04);
	write_cmos_sensor(ctx, 0x5975, 0x01);
	write_cmos_sensor(ctx, 0x5F10, 0x09);
	write_cmos_sensor(ctx, 0x5F11, 0x92);
	write_cmos_sensor(ctx, 0x5F12, 0x32);
	write_cmos_sensor(ctx, 0x5F13, 0x72);
	write_cmos_sensor(ctx, 0x5F14, 0x16);
	write_cmos_sensor(ctx, 0x5F15, 0xBA);
	write_cmos_sensor(ctx, 0x5F17, 0x13);
	write_cmos_sensor(ctx, 0x5F18, 0x24);
	write_cmos_sensor(ctx, 0x5F19, 0x60);
	write_cmos_sensor(ctx, 0x5F1A, 0xE3);
	write_cmos_sensor(ctx, 0x5F1B, 0xAD);
	write_cmos_sensor(ctx, 0x5F1C, 0x74);
	write_cmos_sensor(ctx, 0x5F2D, 0x25);
	write_cmos_sensor(ctx, 0x5F5C, 0xD0);
	write_cmos_sensor(ctx, 0x6A22, 0x00);
	write_cmos_sensor(ctx, 0x6A23, 0x1D);
	write_cmos_sensor(ctx, 0x7BA8, 0x00);
	write_cmos_sensor(ctx, 0x7BA9, 0x00);
	write_cmos_sensor(ctx, 0x886B, 0x00);
	write_cmos_sensor(ctx, 0x9002, 0x0A);
	write_cmos_sensor(ctx, 0x9004, 0x1A);
	write_cmos_sensor(ctx, 0x9214, 0x93);
	write_cmos_sensor(ctx, 0x9215, 0x69);
	write_cmos_sensor(ctx, 0x9216, 0x93);
	write_cmos_sensor(ctx, 0x9217, 0x6B);
	write_cmos_sensor(ctx, 0x9218, 0x93);
	write_cmos_sensor(ctx, 0x9219, 0x6D);
	write_cmos_sensor(ctx, 0x921A, 0x57);
	write_cmos_sensor(ctx, 0x921B, 0x58);
	write_cmos_sensor(ctx, 0x921C, 0x57);
	write_cmos_sensor(ctx, 0x921D, 0x59);
	write_cmos_sensor(ctx, 0x921E, 0x57);
	write_cmos_sensor(ctx, 0x921F, 0x5A);
	write_cmos_sensor(ctx, 0x9220, 0x57);
	write_cmos_sensor(ctx, 0x9221, 0x5B);
	write_cmos_sensor(ctx, 0x9222, 0x93);
	write_cmos_sensor(ctx, 0x9223, 0x02);
	write_cmos_sensor(ctx, 0x9224, 0x93);
	write_cmos_sensor(ctx, 0x9225, 0x03);
	write_cmos_sensor(ctx, 0x9226, 0x93);
	write_cmos_sensor(ctx, 0x9227, 0x04);
	write_cmos_sensor(ctx, 0x9228, 0x93);
	write_cmos_sensor(ctx, 0x9229, 0x05);
	write_cmos_sensor(ctx, 0x922A, 0x98);
	write_cmos_sensor(ctx, 0x922B, 0x21);
	write_cmos_sensor(ctx, 0x922C, 0xB2);
	write_cmos_sensor(ctx, 0x922D, 0xDB);
	write_cmos_sensor(ctx, 0x922E, 0xB2);
	write_cmos_sensor(ctx, 0x922F, 0xDC);
	write_cmos_sensor(ctx, 0x9230, 0xB2);
	write_cmos_sensor(ctx, 0x9231, 0xDD);
	write_cmos_sensor(ctx, 0x9232, 0xB2);
	write_cmos_sensor(ctx, 0x9233, 0xE1);
	write_cmos_sensor(ctx, 0x9234, 0xB2);
	write_cmos_sensor(ctx, 0x9235, 0xE2);
	write_cmos_sensor(ctx, 0x9236, 0xB2);
	write_cmos_sensor(ctx, 0x9237, 0xE3);
	write_cmos_sensor(ctx, 0x9238, 0xB7);
	write_cmos_sensor(ctx, 0x9239, 0xB9);
	write_cmos_sensor(ctx, 0x923A, 0xB7);
	write_cmos_sensor(ctx, 0x923B, 0xBB);
	write_cmos_sensor(ctx, 0x923C, 0xB7);
	write_cmos_sensor(ctx, 0x923D, 0xBC);
	write_cmos_sensor(ctx, 0x923E, 0xB7);
	write_cmos_sensor(ctx, 0x923F, 0xC5);
	write_cmos_sensor(ctx, 0x9240, 0xB7);
	write_cmos_sensor(ctx, 0x9241, 0xC7);
	write_cmos_sensor(ctx, 0x9242, 0xB7);
	write_cmos_sensor(ctx, 0x9243, 0xC9);
	write_cmos_sensor(ctx, 0x9244, 0x98);
	write_cmos_sensor(ctx, 0x9245, 0x56);
	write_cmos_sensor(ctx, 0x9246, 0x98);
	write_cmos_sensor(ctx, 0x9247, 0x55);
	write_cmos_sensor(ctx, 0x9380, 0x00);
	write_cmos_sensor(ctx, 0x9381, 0x62);
	write_cmos_sensor(ctx, 0x9382, 0x00);
	write_cmos_sensor(ctx, 0x9383, 0x56);
	write_cmos_sensor(ctx, 0x9384, 0x00);
	write_cmos_sensor(ctx, 0x9385, 0x52);
	write_cmos_sensor(ctx, 0x9388, 0x00);
	write_cmos_sensor(ctx, 0x9389, 0x55);
	write_cmos_sensor(ctx, 0x938A, 0x00);
	write_cmos_sensor(ctx, 0x938B, 0x55);
	write_cmos_sensor(ctx, 0x938C, 0x00);
	write_cmos_sensor(ctx, 0x938D, 0x41);
	LOG_INF("image quality\n");
	write_cmos_sensor(ctx, 0x9827, 0x20);
	write_cmos_sensor(ctx, 0x9830, 0x0A);
	write_cmos_sensor(ctx, 0x9833, 0x0A);
	write_cmos_sensor(ctx, 0x9834, 0x32);
	write_cmos_sensor(ctx, 0x9837, 0x22);
	write_cmos_sensor(ctx, 0x983C, 0x04);
	write_cmos_sensor(ctx, 0x983F, 0x0A);
	write_cmos_sensor(ctx, 0x994F, 0x00);
	write_cmos_sensor(ctx, 0x9A48, 0x06);
	write_cmos_sensor(ctx, 0x9A49, 0x06);
	write_cmos_sensor(ctx, 0x9A4A, 0x06);
	write_cmos_sensor(ctx, 0x9A4B, 0x06);
	write_cmos_sensor(ctx, 0x9A4E, 0x03);
	write_cmos_sensor(ctx, 0x9A4F, 0x03);
	write_cmos_sensor(ctx, 0x9A54, 0x03);
	write_cmos_sensor(ctx, 0x9A66, 0x03);
	write_cmos_sensor(ctx, 0x9A67, 0x03);
	write_cmos_sensor(ctx, 0xA2C9, 0x02);
	write_cmos_sensor(ctx, 0xA2CB, 0x02);
	write_cmos_sensor(ctx, 0xA2CD, 0x02);
	write_cmos_sensor(ctx, 0xB249, 0x3F);
	write_cmos_sensor(ctx, 0xB24F, 0x3F);
	write_cmos_sensor(ctx, 0xB290, 0x3F);
	write_cmos_sensor(ctx, 0xB293, 0x3F);
	write_cmos_sensor(ctx, 0xB296, 0x3F);
	write_cmos_sensor(ctx, 0xB299, 0x3F);
	write_cmos_sensor(ctx, 0xB2A2, 0x3F);
	write_cmos_sensor(ctx, 0xB2A8, 0x3F);
	write_cmos_sensor(ctx, 0xB2A9, 0x0D);
	write_cmos_sensor(ctx, 0xB2AA, 0x0D);
	write_cmos_sensor(ctx, 0xB2AB, 0x3F);
	write_cmos_sensor(ctx, 0xB2BA, 0x2F);
	write_cmos_sensor(ctx, 0xB2BB, 0x2F);
	write_cmos_sensor(ctx, 0xB2BC, 0x2F);
	write_cmos_sensor(ctx, 0xB2BD, 0x10);
	write_cmos_sensor(ctx, 0xB2C0, 0x3F);
	write_cmos_sensor(ctx, 0xB2C3, 0x3F);
	write_cmos_sensor(ctx, 0xB2D2, 0x3F);
	write_cmos_sensor(ctx, 0xB2DE, 0x20);
	write_cmos_sensor(ctx, 0xB2DF, 0x20);
	write_cmos_sensor(ctx, 0xB2E0, 0x20);
	write_cmos_sensor(ctx, 0xB2EA, 0x3F);
	write_cmos_sensor(ctx, 0xB2ED, 0x3F);
	write_cmos_sensor(ctx, 0xB2EE, 0x3F);
	write_cmos_sensor(ctx, 0xB2EF, 0x3F);
	write_cmos_sensor(ctx, 0xB2F0, 0x2F);
	write_cmos_sensor(ctx, 0xB2F1, 0x2F);
	write_cmos_sensor(ctx, 0xB2F2, 0x2F);
	write_cmos_sensor(ctx, 0xB2F9, 0x0E);
	write_cmos_sensor(ctx, 0xB2FA, 0x0E);
	write_cmos_sensor(ctx, 0xB2FB, 0x0E);
	write_cmos_sensor(ctx, 0xB759, 0x01);
	write_cmos_sensor(ctx, 0xB765, 0x3F);
	write_cmos_sensor(ctx, 0xB76B, 0x3F);
	write_cmos_sensor(ctx, 0xB7B3, 0x03);
	write_cmos_sensor(ctx, 0xB7B5, 0x03);
	write_cmos_sensor(ctx, 0xB7B7, 0x03);
	write_cmos_sensor(ctx, 0xB7BF, 0x03);
	write_cmos_sensor(ctx, 0xB7C1, 0x03);
	write_cmos_sensor(ctx, 0xB7C3, 0x03);
	write_cmos_sensor(ctx, 0xB7EF, 0x02);
	write_cmos_sensor(ctx, 0xB7F5, 0x1F);
	write_cmos_sensor(ctx, 0xB7F7, 0x1F);
	write_cmos_sensor(ctx, 0xB7F9, 0x1F);

	write_cmos_sensor(ctx, 0x0138, 0x01);/*enable temperature measure*/
	get_sensor_temperature(ctx);
	LOG_INF("X\n");
}				/*      sensor_init  */

static void custom1_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("custom1 HDR setting E,\n");
	set_mirror_flip(ctx, ctx->mirror);
#if MULTI_WRITE
	imx577_table_write_cmos_sensor(ctx,
		addr_data_pair_custom1_imx577,
		sizeof(addr_data_pair_custom1_imx577) /
		sizeof(kal_uint16));
#else
	LOG_INF("No setting custom1 X,\n");
#endif
	first_n_frame = 4;
	mDELAY(20);
}


static void preview_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	set_mirror_flip(ctx, ctx->mirror);
	write_cmos_sensor(ctx, 0x0112, 0x0A);
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);
	write_cmos_sensor(ctx, 0x0342, 0x11);
	write_cmos_sensor(ctx, 0x0343, 0xA0);
	write_cmos_sensor(ctx, 0x0340, 0x0C);
	write_cmos_sensor(ctx, 0x0341, 0x1E);
	write_cmos_sensor(ctx, 0x3210, 0x00);
	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x00);
	write_cmos_sensor(ctx, 0x0347, 0x00);
	write_cmos_sensor(ctx, 0x0348, 0x0F);
	write_cmos_sensor(ctx, 0x0349, 0xD7);
	write_cmos_sensor(ctx, 0x034A, 0x0B);
	write_cmos_sensor(ctx, 0x034B, 0xDF);
	write_cmos_sensor(ctx, 0x00E3, 0x00);
	write_cmos_sensor(ctx, 0x00E4, 0x00);
	write_cmos_sensor(ctx, 0x00E5, 0x01);
	write_cmos_sensor(ctx, 0x00FC, 0x0A);
	write_cmos_sensor(ctx, 0x00FD, 0x0A);
	write_cmos_sensor(ctx, 0x00FE, 0x0A);
	write_cmos_sensor(ctx, 0x00FF, 0x0A);
	write_cmos_sensor(ctx, 0x0220, 0x00);
	write_cmos_sensor(ctx, 0x0221, 0x11);
	write_cmos_sensor(ctx, 0x0381, 0x01);
	write_cmos_sensor(ctx, 0x0383, 0x01);
	write_cmos_sensor(ctx, 0x0385, 0x01);
	write_cmos_sensor(ctx, 0x0387, 0x01);
	write_cmos_sensor(ctx, 0x0900, 0x00);
	write_cmos_sensor(ctx, 0x0901, 0x11);
	write_cmos_sensor(ctx, 0x0902, 0x00);
	write_cmos_sensor(ctx, 0x3140, 0x02);
	write_cmos_sensor(ctx, 0x3241, 0x11);
	write_cmos_sensor(ctx, 0x3250, 0x03);
	write_cmos_sensor(ctx, 0x3E10, 0x00);
	write_cmos_sensor(ctx, 0x3E11, 0x00);
	write_cmos_sensor(ctx, 0x3F0D, 0x00);
	write_cmos_sensor(ctx, 0x3F42, 0x00);
	write_cmos_sensor(ctx, 0x3F43, 0x00);
	write_cmos_sensor(ctx, 0x0401, 0x00);
	write_cmos_sensor(ctx, 0x0404, 0x00);
	write_cmos_sensor(ctx, 0x0405, 0x10);
	write_cmos_sensor(ctx, 0x0408, 0x00);
	write_cmos_sensor(ctx, 0x0409, 0x00);
	write_cmos_sensor(ctx, 0x040A, 0x00);
	write_cmos_sensor(ctx, 0x040B, 0x00);
	write_cmos_sensor(ctx, 0x040C, 0x0F);
	write_cmos_sensor(ctx, 0x040D, 0xD8);
	write_cmos_sensor(ctx, 0x040E, 0x0B);
	write_cmos_sensor(ctx, 0x040F, 0xE0);
	write_cmos_sensor(ctx, 0x034C, 0x0F);
	write_cmos_sensor(ctx, 0x034D, 0xD8);
	write_cmos_sensor(ctx, 0x034E, 0x0B);
	write_cmos_sensor(ctx, 0x034F, 0xE0);
	write_cmos_sensor(ctx, 0x0301, 0x05);
	write_cmos_sensor(ctx, 0x0303, 0x04);
	write_cmos_sensor(ctx, 0x0305, 0x04);
	write_cmos_sensor(ctx, 0x0306, 0x01);
	write_cmos_sensor(ctx, 0x0307, 0x5E);
	write_cmos_sensor(ctx, 0x0309, 0x0A);
	write_cmos_sensor(ctx, 0x030B, 0x02);
	write_cmos_sensor(ctx, 0x030D, 0x04);
	write_cmos_sensor(ctx, 0x030E, 0x01);
	write_cmos_sensor(ctx, 0x030F, 0x4C);
	write_cmos_sensor(ctx, 0x0310, 0x01);
	write_cmos_sensor(ctx, 0x0820, 0x0F);
	write_cmos_sensor(ctx, 0x0821, 0x90);
	write_cmos_sensor(ctx, 0x0822, 0x00);
	write_cmos_sensor(ctx, 0x0823, 0x00);
	write_cmos_sensor(ctx, 0x3E20, 0x01);
	write_cmos_sensor(ctx, 0x3E37, 0x00);
	write_cmos_sensor(ctx, 0x3F50, 0x00);
	write_cmos_sensor(ctx, 0x3F56, 0x01);
	write_cmos_sensor(ctx, 0x3F57, 0x03);
	write_cmos_sensor(ctx, 0x3C0A, 0x5A);
	write_cmos_sensor(ctx, 0x3C0B, 0x55);
	write_cmos_sensor(ctx, 0x3C0C, 0x28);
	write_cmos_sensor(ctx, 0x3C0D, 0x07);
	write_cmos_sensor(ctx, 0x3C0E, 0xFF);
	write_cmos_sensor(ctx, 0x3C0F, 0x00);
	write_cmos_sensor(ctx, 0x3C10, 0x00);
	write_cmos_sensor(ctx, 0x3C11, 0x02);
	write_cmos_sensor(ctx, 0x3C12, 0x00);
	write_cmos_sensor(ctx, 0x3C13, 0x03);
	write_cmos_sensor(ctx, 0x3C14, 0x00);
	write_cmos_sensor(ctx, 0x3C15, 0x00);
	write_cmos_sensor(ctx, 0x3C16, 0x0C);
	write_cmos_sensor(ctx, 0x3C17, 0x0C);
	write_cmos_sensor(ctx, 0x3C18, 0x0C);
	write_cmos_sensor(ctx, 0x3C19, 0x0A);
	write_cmos_sensor(ctx, 0x3C1A, 0x0A);
	write_cmos_sensor(ctx, 0x3C1B, 0x0A);
	write_cmos_sensor(ctx, 0x3C1C, 0x00);
	write_cmos_sensor(ctx, 0x3C1D, 0x00);
	write_cmos_sensor(ctx, 0x3C1E, 0x00);
	write_cmos_sensor(ctx, 0x3C1F, 0x00);
	write_cmos_sensor(ctx, 0x3C20, 0x00);
	write_cmos_sensor(ctx, 0x3C21, 0x00);
	write_cmos_sensor(ctx, 0x3C22, 0x3F);
	write_cmos_sensor(ctx, 0x3C23, 0x0A);
	write_cmos_sensor(ctx, 0x3E35, 0x01);
	write_cmos_sensor(ctx, 0x3F4A, 0x03);
	write_cmos_sensor(ctx, 0x3F4B, 0xBF);
	write_cmos_sensor(ctx, 0x3F26, 0x00);
	write_cmos_sensor(ctx, 0x0202, 0x0C);
	write_cmos_sensor(ctx, 0x0203, 0x08);
	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);
	write_cmos_sensor(ctx, 0x0210, 0x01);
	write_cmos_sensor(ctx, 0x0211, 0x00);
	write_cmos_sensor(ctx, 0x0212, 0x01);
	write_cmos_sensor(ctx, 0x0213, 0x00);
	write_cmos_sensor(ctx, 0x0214, 0x01);
	write_cmos_sensor(ctx, 0x0215, 0x00);
	mDELAY(10);
}

static void capture_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("capture E\n");
	preview_setting(ctx);
}

static void normal_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("normal video E\n");
	preview_setting(ctx);
}

static void hs_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("hs_video E\n");
	preview_setting(ctx);
}

static void slim_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("slim video E\n");
	preview_setting(ctx);
}

static void custom2_setting(struct subdrv_ctx *ctx)
{
	set_mirror_flip(ctx, ctx->mirror);
#if MULTI_WRITE
	imx577_table_write_cmos_sensor(ctx,
		addr_data_pair_custom2_imx577,
		sizeof(addr_data_pair_custom2_imx577) /
		sizeof(kal_uint16));
#else
	LOG_INF("No setting custom2 X,\n");
#endif
	mDELAY(10);
}


/*************************************************************************
 * FUNCTION
 *	get_imgsensor_id
 *
 * DESCRIPTION
 *	This function get the sensor ID
 *
 * PARAMETERS
 *	*sensorID : return the sensor ID
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/

static int get_imgsensor_id(struct subdrv_ctx *ctx, UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry_total = 1;
	kal_uint8 retry_cnt = retry_total;

	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			*sensor_id = return_sensor_id(ctx);
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			{
				read_imx577_SensorID(ctx, sensor_id);
				if (*sensor_id == imgsensor_info.sensor_id) {
					LOG_INF("---- read eeprom, sensor id: 0x%x\n", *sensor_id);
					return ERROR_NONE;
				}
			}
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n",
				ctx->i2c_write_id, *sensor_id);

			retry_cnt--;
		} while (retry_cnt > 0);
		i++;
		retry_cnt = retry_total;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {

	/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
 * FUNCTION
 *	open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int open(struct subdrv_ctx *ctx)
{
	/* const kal_uint8 i2c_addr[] = {
	 * IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	 */
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;

	LOG_INF("imx577,MIPI 4LANE\n");
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			sensor_id = return_sensor_id(ctx);
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, sensor_id);
				break;
			}

			{
				read_imx577_SensorID(ctx, &sensor_id);
				if (sensor_id == imgsensor_info.sensor_id) {
					LOG_INF("---- read eeprom, sensor id: 0x%x\n", sensor_id);
					break;
				}
			}

			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}

	if (imgsensor_info.sensor_id != sensor_id) {
		LOG_INF("Open sensor id fail id: 0x%x\n", sensor_id);
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	/* initail sequence write in  */
	sensor_init(ctx);


	ctx->autoflicker_en = KAL_FALSE;
	ctx->sensor_mode = IMGSENSOR_MODE_INIT;
	ctx->shutter = 0x3D0;
	ctx->gain = 0x80;	/* 0x100; */
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->dummy_pixel = 0;
	ctx->dummy_line = 0;
	ctx->hdr_mode = 0;
	ctx->test_pattern = KAL_FALSE;
	ctx->current_fps = imgsensor_info.pre.max_framerate;

	return ERROR_NONE;
}				/*      open  */



/*************************************************************************
 * FUNCTION
 *	close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int close(struct subdrv_ctx *ctx)
{
	return ERROR_NONE;
}				/*      close  */

/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 preview(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	ctx->sensor_mode = IMGSENSOR_MODE_PREVIEW;
	ctx->pclk = imgsensor_info.pre.pclk;
	/* ctx->video_mode = KAL_FALSE; */
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	preview_setting(ctx);
	return ERROR_NONE;
}				/*      preview   */

/*************************************************************************
 * FUNCTION
 *	capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	ctx->sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (ctx->current_fps == imgsensor_info.cap1.max_framerate) {
		ctx->pclk = imgsensor_info.cap1.pclk;
		ctx->line_length = imgsensor_info.cap1.linelength;
		ctx->frame_length = imgsensor_info.cap1.framelength;
		ctx->min_frame_length = imgsensor_info.cap1.framelength;
		ctx->autoflicker_en = KAL_FALSE;
	}

	else {
		if (ctx->current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF(
	  "current_fps %d fps is not support, so use cap's setting: %d fps!\n",
		ctx->current_fps, imgsensor_info.cap.max_framerate / 10);

		ctx->pclk = imgsensor_info.cap.pclk;
		ctx->line_length = imgsensor_info.cap.linelength;
		ctx->frame_length = imgsensor_info.cap.framelength;
		ctx->min_frame_length = imgsensor_info.cap.framelength;
		ctx->autoflicker_en = KAL_FALSE;
	}

	capture_setting(ctx);

	mdelay(10);
	return ERROR_NONE;
}				/* capture(ctx) */

static kal_uint32 normal_video(struct subdrv_ctx *ctx,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	ctx->sensor_mode = IMGSENSOR_MODE_VIDEO;
	ctx->pclk = imgsensor_info.normal_video.pclk;
	ctx->line_length = imgsensor_info.normal_video.linelength;
	ctx->frame_length = imgsensor_info.normal_video.framelength;
	ctx->min_frame_length = imgsensor_info.normal_video.framelength;
	/* ctx->current_fps = 300; */
	ctx->autoflicker_en = KAL_FALSE;
	LOG_INF("ihdr enable :%d\n", ctx->hdr_mode);
	normal_video_setting(ctx);
	return ERROR_NONE;
}				/*      normal_video   */

static kal_uint32 hs_video(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	ctx->sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	ctx->pclk = imgsensor_info.hs_video.pclk;
	/* ctx->video_mode = KAL_TRUE; */
	ctx->line_length = imgsensor_info.hs_video.linelength;
	ctx->frame_length = imgsensor_info.hs_video.framelength;
	ctx->min_frame_length = imgsensor_info.hs_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	/* ctx->current_fps = 600; */
	ctx->autoflicker_en = KAL_FALSE;
	hs_video_setting(ctx);

	return ERROR_NONE;
}				/*      hs_video   */

static kal_uint32 slim_video(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	ctx->sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	ctx->pclk = imgsensor_info.slim_video.pclk;
	/* ctx->video_mode = KAL_TRUE; */
	ctx->line_length = imgsensor_info.slim_video.linelength;
	ctx->frame_length = imgsensor_info.slim_video.framelength;
	ctx->min_frame_length = imgsensor_info.slim_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	/* ctx->current_fps = 1200; */
	ctx->autoflicker_en = KAL_FALSE;
	slim_video_setting(ctx);

	return ERROR_NONE;
}				/*      slim_video       */

static kal_uint32 custom1(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	ctx->pclk = imgsensor_info.custom1.pclk;
	/* ctx->video_mode = KAL_FALSE; */
	ctx->line_length = imgsensor_info.custom1.linelength;
	ctx->frame_length = imgsensor_info.custom1.framelength;
	ctx->min_frame_length = imgsensor_info.custom1.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	ctx->autoflicker_en = KAL_FALSE;
	custom1_setting(ctx);

	return ERROR_NONE;
}

static kal_uint32 custom2(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	ctx->pclk = imgsensor_info.custom2.pclk;
	/* ctx->video_mode = KAL_FALSE; */
	ctx->line_length = imgsensor_info.custom2.linelength;
	ctx->frame_length = imgsensor_info.custom2.framelength;
	ctx->min_frame_length = imgsensor_info.custom2.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	custom2_setting(ctx);
	return ERROR_NONE;
}			   /*	   stereo	 */

static int get_resolution(
	struct subdrv_ctx *ctx,
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	int i = 0;

	for (i = SENSOR_SCENARIO_ID_MIN; i < SENSOR_SCENARIO_ID_MAX; i++) {
		if (i < imgsensor_info.sensor_mode_num) {
			sensor_resolution->SensorWidth[i] = imgsensor_winsize_info[i].w2_tg_size;
			sensor_resolution->SensorHeight[i] = imgsensor_winsize_info[i].h2_tg_size;
		} else {
			sensor_resolution->SensorWidth[i] = 0;
			sensor_resolution->SensorHeight[i] = 0;
		}
	}

	return ERROR_NONE;
}				/*      get_resolution  */

static int get_info(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* not use */
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* inverse with datasheet */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4;	/* not use */
	sensor_info->SensorResetActiveHigh = FALSE;	/* not use */
	sensor_info->SensorResetDelayCount = 5;	/* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;


	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_PREVIEW] =
		imgsensor_info.pre_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_CAPTURE] =
		imgsensor_info.cap_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_VIDEO] =
		imgsensor_info.video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM1] =
		imgsensor_info.custom1_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM2] =
		imgsensor_info.custom2_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO] =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_SLIM_VIDEO] =
		imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;

	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;

	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;

	sensor_info->FrameTimeDelayFrame =
		imgsensor_info.frame_time_delay_frame;

	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->TEMPERATURE_SUPPORT = 1;


	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3;	/* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2;	/* not use */
	sensor_info->SensorPixelClockCount = 3;	/* not use */
	sensor_info->SensorDataLatchCount = 2;	/* not use */

	sensor_info->SensorWidthSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorPacketECCOrder = 1;

	return ERROR_NONE;
}				/*      get_info  */


static int control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	ctx->current_scenario_id = scenario_id;
	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		preview(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		capture(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		normal_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		hs_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		slim_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		custom1(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		custom2(ctx, image_window, sensor_config_data);
		break;
	default:
		LOG_INF("Error ScenarioId setting");
		preview(ctx, image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}				/* control(ctx) */



static kal_uint32 set_video_mode(struct subdrv_ctx *ctx, UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)
		/* Dynamic frame rate */
		return ERROR_NONE;
	if ((framerate == 300) && (ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 296;
	else if ((framerate == 150) && (ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 146;
	else
		ctx->current_fps = framerate;
	set_max_framerate(ctx, ctx->current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(struct subdrv_ctx *ctx, kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	if (enable)		/* enable auto flicker */
		ctx->autoflicker_en = KAL_TRUE;
	else			/* Cancel Auto flick */
		ctx->autoflicker_en = KAL_FALSE;
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		frame_length = imgsensor_info.pre.pclk
			/ framerate * 10 / imgsensor_info.pre.linelength;


		ctx->dummy_line =
		  (frame_length > imgsensor_info.pre.framelength)
		? (frame_length - imgsensor_info.pre.framelength) : 0;

		ctx->frame_length =
			imgsensor_info.pre.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		if (framerate == 0)
			return ERROR_NONE;

		frame_length = imgsensor_info.normal_video.pclk
		    / framerate * 10 / imgsensor_info.normal_video.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.normal_video.framelength)
		? (frame_length - imgsensor_info.normal_video.framelength) : 0;

		ctx->frame_length =
		 imgsensor_info.normal_video.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
	if (ctx->current_fps == imgsensor_info.cap1.max_framerate) {
		frame_length = imgsensor_info.cap1.pclk
			/ framerate * 10 / imgsensor_info.cap1.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.cap1.framelength)
		? (frame_length - imgsensor_info.cap1.framelength) : 0;

		ctx->frame_length =
		    imgsensor_info.cap1.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
	} else {
		if (ctx->current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF(
			    "current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			    framerate,
			    imgsensor_info.cap.max_framerate / 10);

		frame_length = imgsensor_info.cap.pclk /
			framerate * 10 / imgsensor_info.cap.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.cap.framelength)
		? (frame_length - imgsensor_info.cap.framelength) : 0;

		ctx->frame_length =
		    imgsensor_info.cap.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		}
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk
			/ framerate * 10 / imgsensor_info.hs_video.linelength;

		ctx->dummy_line =
		    (frame_length > imgsensor_info.hs_video.framelength)
		    ? (frame_length - imgsensor_info.hs_video.framelength) : 0;

		ctx->frame_length =
		    imgsensor_info.hs_video.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk
			/ framerate * 10 / imgsensor_info.slim_video.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.slim_video.framelength)
		? (frame_length - imgsensor_info.slim_video.framelength) : 0;

		ctx->frame_length =
		  imgsensor_info.slim_video.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk
			/ framerate * 10 / imgsensor_info.custom1.linelength;


		ctx->dummy_line =
		  (frame_length > imgsensor_info.custom1.framelength)
		? (frame_length - imgsensor_info.custom1.framelength) : 0;

		ctx->frame_length =
			imgsensor_info.custom1.framelength +
			ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		frame_length = imgsensor_info.custom2.pclk
			/ framerate * 10 / imgsensor_info.custom2.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.custom2.framelength)
		? (frame_length - imgsensor_info.custom2.framelength) : 0;

		ctx->frame_length =
		 imgsensor_info.custom2.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	default:		/* coding with  preview scenario by default */
		frame_length = imgsensor_info.pre.pclk
		    / framerate * 10 / imgsensor_info.pre.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.pre.framelength)
		? (frame_length - imgsensor_info.pre.framelength) : 0;

		ctx->frame_length =
			imgsensor_info.pre.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		LOG_INF("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		*framerate = imgsensor_info.normal_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		*framerate = imgsensor_info.cap.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		*framerate = imgsensor_info.custom2.max_framerate;
		break;
	default:
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("%s: %d\n", __func__, enable);

	if (enable)
		write_cmos_sensor(ctx, 0x0601, 0x02);
	else
		write_cmos_sensor(ctx, 0x0601, 0x00);

	ctx->test_pattern = enable;
	return ERROR_NONE;
}

static kal_uint32 streaming_control(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(ctx, 0x0100, 0X01);
	else {
		get_sensor_temperature(ctx);
		write_cmos_sensor(ctx, 0x0100, 0x00);
	}

	mdelay(10);
	return ERROR_NONE;
}

static int feature_control(struct subdrv_ctx *ctx, MSDK_SENSOR_FEATURE_ENUM feature_id,
			UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;
	//unsigned long long *feature_return_para =
	//(unsigned long long *) feature_para;
	kal_uint32 rate;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	//struct SENSOR_VC_INFO_STRUCT *pvcinfo;
	//SET_SENSOR_AWB_GAIN *pSetSensorAWB =
	//(SET_SENSOR_AWB_GAIN *)feature_para;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("feature_id = %d\n", feature_id);

	switch (feature_id) {

	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		*(feature_data + 2) = imgsensor_info.max_gain;
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_shutter;
		*(feature_data + 2) = imgsensor_info.exp_step;
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = ctx->line_length;
		*feature_return_para_16 = ctx->frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		LOG_INF("feature_Control ctx->pclk = %d, ctx->current_fps = %d\n",
			ctx->pclk, ctx->current_fps);
		*feature_return_para_32 = ctx->pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode(ctx, (BOOL) * feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain(ctx, (UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor(ctx, sensor_reg_data->RegAddr,
			sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(ctx, sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		// get the lens driver ID from EEPROM or just
		// return LENS_DRIVER_ID_DO_NOT_CARE
		// if EEPROM does not exist in camera module.
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(ctx, feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode(ctx, (BOOL)*feature_data_16,
			*(feature_data_16+1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(ctx,
			(enum MSDK_SCENARIO_ID_ENUM)*feature_data,
			*(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(ctx,
			(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data+1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode(ctx, (BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		//for factory mode auto testing
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		LOG_INF("current fps :%d\n", (UINT32)*feature_data);
		ctx->current_fps = *feature_data;
		break;
	case SENSOR_FEATURE_SET_HDR:
		LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
		ctx->ihdr_mode = *feature_data;
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			(UINT32)*feature_data);
		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)
			(uintptr_t)(*(feature_data+1));

		switch (*feature_data_32) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[3],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[4],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data,
			(UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		break;
	case SENSOR_FEATURE_SET_HDR_SHUTTER:
		LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			rate = imgsensor_info.cap.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			rate = imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			rate = imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			rate = imgsensor_info.slim_video.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			rate = imgsensor_info.pre.mipi_pixel_rate;
			break;
		}
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
		break;
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_32 = get_sensor_temperature(ctx);
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(ctx, KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(ctx, *feature_data);
		streaming_control(ctx, KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.cap.framelength << 16)
				+ imgsensor_info.cap.linelength;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.normal_video.framelength << 16)
				+ imgsensor_info.normal_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.hs_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.slim_video.framelength << 16)
				+ imgsensor_info.slim_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.pclk;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
			break;
		}
		break;
	default:
		break;
	}

	return ERROR_NONE;
} /* feature_control(ctx)  */

#ifdef IMGSENSOR_VC_ROUTING
static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0FD8,
			.vsize = 0x0BE0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0FD8,
			.vsize = 0x0BE0,
		},
	},
};

static int get_frame_desc(struct subdrv_ctx *ctx,
		int scenario_id, struct mtk_mbus_frame_desc *fd)
{
	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_prev);
		memcpy(fd->entry, frame_desc_prev, sizeof(frame_desc_prev));
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cap);
		memcpy(fd->entry, frame_desc_cap, sizeof(frame_desc_cap));
		break;
	default:
		return -1;
	}

	return 0;
}
#endif

static const struct subdrv_ctx defctx = {

	.ana_gain_def = 0x100,
	.ana_gain_max = 1024,
	.ana_gain_min = 64,
	.ana_gain_step = 16,
	.exposure_def = 0x14d,
	.exposure_max = 0xff00,
	.exposure_min = 2,
	.exposure_step = 1,
	.max_frame_length = 0xff00,

	.mirror = IMAGE_HV_MIRROR, //IMAGE_NORMAL,	/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x14d,	/* current shutter */
	.gain = 0x100,//current gain
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */

	/* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,

	/* current scenario id */
	.current_scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW,

	.i2c_write_id = 0x20,	/* record current sensor's i2c write id */
};

static int init_ctx(struct subdrv_ctx *ctx,
		struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(ctx, &defctx, sizeof(*ctx));
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static struct subdrv_ops ops = {
	.get_id = get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = open,
	.get_info = get_info,
	.get_resolution = get_resolution,
	.control = control,
	.feature_control = feature_control,
	.close = close,
#ifdef IMGSENSOR_VC_ROUTING
	.get_frame_desc = get_frame_desc,
#endif
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, 24, 0},
	{HW_ID_RST, 0, 5},
	{HW_ID_AVDD, 2900000, 2}, // CAM_3V7_EN ==> PP_3V7_EN
	{HW_ID_DVDD, 1200000, 0}, // PER_EN ==> VCAMA/VCAMD power on
	{HW_ID_DOVDD, 1800000, 5}, // pmic for iovdd

	//{HW_ID_DVDD, 1200000, 0},
	//{HW_ID_PDN, 1, 1},
	//{HW_ID_AVDD, 2900000, 10},
	{HW_ID_MCLK_DRIVING_CURRENT, 6, 1},
	{HW_ID_RST, 1, 5},
};

const struct subdrv_entry imx577_mipi_raw_entry = {
	.name = "imx577_mipi_raw",
	.id = IMX577_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

