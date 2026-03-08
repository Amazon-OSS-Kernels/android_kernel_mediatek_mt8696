// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2020 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *     IMX355mipi_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define_v4l2.h"
#include "kd_imgsensor_errcode.h"

#include "imx355mipiraw_Sensor.h"

#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"

#define read_cmos_sensor(...) subdrv_i2c_rd_u8(__VA_ARGS__)
#define write_cmos_sensor(...) subdrv_i2c_wr_u8(__VA_ARGS__)
#define imx355_table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u8(__VA_ARGS__)
#define IMX355_N_ROUND_UP(n, d) ((((n) + (d) - 1) / (d)) * (d))

/************************Modify Following Strings for Debug********************/
#define PFX "IMX355_camera_sensor"
#define LOG_1 LOG_INF("IMX355,MIPI 4LANE\n")
/************************   Modify end    *************************************/
#define HDR_raw12 0
#define PIXEL_RATE 288000000
//#undef IMX355_24_FPS

#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = IMX355_SENSOR_ID,

	.checksum_value = 0xafd83a68,

	.pre = {
		.pclk = 288000000,
		.linelength = 3672,
		.framelength = 2614,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3280,
		.grabwindow_height = 2464,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = PIXEL_RATE,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 288000000,
		.linelength = 3672,
		.framelength = 2614,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3280,
		.grabwindow_height = 2464,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = PIXEL_RATE,
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 288000000,
		.linelength = 3672,
		.framelength = 2614,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3280,
		.grabwindow_height = 2464,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = PIXEL_RATE,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 288000000,
		.linelength = 3672,
		.framelength = 2614,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3280,
		.grabwindow_height = 2464,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = PIXEL_RATE,
		.max_framerate = 300,
	},

	.margin = 10,		/* sensor framelength & shutter margin */
	.min_shutter = 1,	/* min shutter */
	.min_gain = 64,
	.max_gain = 1024,
	.min_gain_iso = 100,
	.exp_step = 2,
	.gain_step = 1,
	.gain_type = 0,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,	/* isp gain delay frame for AE cycle */
	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.temperature_support = 0,	/* 1, support; 0,not support */
	.sensor_mode_num = 4,	/* support sensor mode num */

	.cap_delay_frame = 1,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 1,	/* enter video delay frame num */
	.hs_video_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_8MA,	/* mclk driving current */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,	/* mipi lane num */
	.i2c_addr_table = {0x34, 0x6c, 0x20, 0xff},
	.i2c_speed = 400,	/* i2c read/write speed */
};


/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] = {
	{3280, 2464, 0, 0, 3280, 2464, 3280, 2464,
	0000, 0000, 3280, 2464, 0, 0, 3280, 2464},	/* Preview */
	{3280, 2464, 0, 0, 3280, 2464, 3280, 2464,
	0000, 0000, 3280, 2464, 0, 0, 3280, 2464},	/* capture */
	{3280, 2464, 0, 0, 3280, 2464, 3280, 2464,
	0000, 0000, 3280, 2464, 0, 0, 3280, 2464},	/* Video */
	{3280, 2464, 0, 0, 3280, 2464, 3280, 2464,
	0000, 0000, 3280, 2464, 0, 0, 3280, 2464},	/* */
};

#define IMX355MIPI_MaxGainIndex (389)
kal_uint16 imx355MIPI_sensorGainMapping[IMX355MIPI_MaxGainIndex][2] = {
	{64, 0},
	{65, 16},
	{66, 32},
	{67, 46},
	{68, 61},
	{69, 75},
	{70, 88},
	{71, 101},
	{72, 114},
	{73, 127},
	{74, 139},
	{75, 151},
	{76, 162},
	{77, 173},
	{78, 184},
	{79, 195},
	{80, 205},
	{81, 215},
	{82, 225},
	{83, 235},
	{84, 244},
	{85, 253},
	{86, 262},
	{87, 271},
	{88, 280},
	{89, 288},
	{90, 296},
	{91, 304},
	{92, 312},
	{93, 320},
	{94, 327},
	{95, 335},
	{96, 342},
	{97, 349},
	{98, 356},
	{99, 363},
	{100, 369},
	{101, 376},
	{102, 382},
	{103, 388},
	{104, 394},
	{105, 400},
	{106, 406},
	{107, 412},
	{108, 418},
	{109, 423},
	{110, 429},
	{111, 434},
	{112, 439},
	{113, 445},
	{114, 450},
	{115, 455},
	{116, 460},
	{117, 464},
	{118, 469},
	{119, 474},
	{120, 478},
	{121, 483},
	{122, 487},
	{123, 492},
	{124, 496},
	{125, 500},
	{126, 504},
	{127, 508},
	{128, 512},
	{129, 516},
	{130, 520},
	{131, 524},
	{132, 528},
	{133, 532},
	{134, 535},
	{135, 539},
	{136, 543},
	{137, 546},
	{138, 550},
	{139, 553},
	{140, 556},
	{141, 560},
	{142, 563},
	{143, 566},
	{144, 569},
	{145, 573},
	{146, 576},
	{147, 579},
	{148, 582},
	{149, 585},
	{150, 588},
	{151, 590},
	{152, 593},
	{153, 596},
	{154, 599},
	{155, 602},
	{156, 604},
	{157, 607},
	{158, 610},
	{159, 612},
	{160, 615},
	{161, 617},
	{162, 620},
	{163, 622},
	{164, 625},
	{165, 627},
	{165, 628},
	{166, 630},
	{167, 632},
	{168, 634},
	{169, 637},
	{170, 639},
	{171, 641},
	{172, 643},
	{173, 646},
	{174, 648},
	{175, 650},
	{176, 652},
	{177, 654},
	{178, 656},
	{179, 658},
	{180, 660},
	{181, 662},
	{182, 664},
	{183, 666},
	{184, 668},
	{185, 670},
	{186, 672},
	{187, 674},
	{188, 676},
	{189, 678},
	{190, 680},
	{191, 681},
	{192, 683},
	{193, 685},
	{194, 687},
	{195, 688},
	{196, 690},
	{197, 692},
	{198, 694},
	{199, 695},
	{200, 697},
	{201, 698},
	{201, 699},
	{202, 700},
	{203, 702},
	{204, 703},
	{205, 705},
	{206, 706},
	{207, 708},
	{208, 709},
	{209, 711},
	{210, 712},
	{211, 714},
	{212, 715},
	{213, 717},
	{214, 718},
	{215, 720},
	{216, 721},
	{217, 722},
	{218, 724},
	{219, 725},
	{220, 727},
	{221, 728},
	{222, 729},
	{223, 731},
	{224, 732},
	{225, 733},
	{226, 735},
	{227, 736},
	{228, 737},
	{229, 738},
	{229, 739},
	{230, 740},
	{231, 741},
	{232, 742},
	{233, 743},
	{234, 744},
	{234, 745},
	{235, 746},
	{236, 747},
	{237, 748},
	{238, 749},
	{239, 750},
	{240, 751},
	{241, 753},
	{242, 754},
	{243, 755},
	{244, 756},
	{245, 757},
	{246, 758},
	{247, 759},
	{248, 760},
	{249, 761},
	{250, 762},
	{251, 763},
	{252, 764},
	{253, 765},
	{254, 766},
	{255, 767},
	{256, 768},
	{257, 769},
	{258, 770},
	{259, 771},
	{260, 772},
	{261, 773},
	{262, 774},
	{263, 775},
	{264, 776},
	{265, 777},
	{266, 778},
	{267, 779},
	{268, 780},
	{269, 781},
	{270, 782},
	{271, 783},
	{273, 784},
	{274, 785},
	{275, 786},
	{276, 787},
	{277, 788},
	{278, 789},
	{280, 790},
	{281, 791},
	{282, 792},
	{283, 793},
	{284, 794},
	{286, 795},
	{287, 796},
	{288, 797},
	{289, 798},
	{291, 799},
	{292, 800},
	{293, 801},
	{295, 802},
	{296, 803},
	{297, 804},
	{299, 805},
	{300, 806},
	{302, 807},
	{303, 808},
	{304, 809},
	{306, 810},
	{307, 811},
	{309, 812},
	{310, 813},
	{312, 814},
	{313, 815},
	{315, 816},
	{316, 817},
	{318, 818},
	{319, 819},
	{321, 820},
	{322, 821},
	{324, 822},
	{326, 823},
	{327, 824},
	{329, 825},
	{330, 826},
	{332, 827},
	{334, 828},
	{336, 829},
	{337, 830},
	{339, 831},
	{341, 832},
	{343, 833},
	{344, 834},
	{346, 835},
	{348, 836},
	{350, 837},
	{352, 838},
	{354, 839},
	{356, 840},
	{358, 841},
	{360, 842},
	{362, 843},
	{364, 844},
	{366, 845},
	{368, 846},
	{370, 847},
	{372, 848},
	{374, 849},
	{376, 850},
	{378, 851},
	{381, 852},
	{383, 853},
	{385, 854},
	{387, 855},
	{390, 856},
	{392, 857},
	{394, 858},
	{397, 859},
	{399, 860},
	{402, 861},
	{404, 862},
	{407, 863},
	{409, 864},
	{412, 865},
	{414, 866},
	{417, 867},
	{420, 868},
	{422, 869},
	{425, 870},
	{428, 871},
	{431, 872},
	{434, 873},
	{436, 874},
	{439, 875},
	{442, 876},
	{445, 877},
	{448, 878},
	{451, 879},
	{455, 880},
	{458, 881},
	{461, 882},
	{464, 883},
	{468, 884},
	{471, 885},
	{474, 886},
	{478, 887},
	{481, 888},
	{485, 889},
	{489, 890},
	{492, 891},
	{496, 892},
	{500, 893},
	{504, 894},
	{508, 895},
	{512, 896},
	{516, 897},
	{520, 898},
	{524, 899},
	{528, 900},
	{532, 901},
	{537, 902},
	{541, 903},
	{546, 904},
	{550, 905},
	{555, 906},
	{560, 907},
	{564, 908},
	{569, 909},
	{574, 910},
	{579, 911},
	{585, 912},
	{590, 913},
	{595, 914},
	{601, 915},
	{606, 916},
	{612, 917},
	{618, 918},
	{624, 919},
	{630, 920},
	{636, 921},
	{642, 922},
	{648, 923},
	{655, 924},
	{661, 925},
	{668, 926},
	{675, 927},
	{682, 928},
	{689, 929},
	{697, 930},
	{704, 931},
	{712, 932},
	{720, 933},
	{728, 934},
	{736, 935},
	{744, 936},
	{753, 937},
	{762, 938},
	{771, 939},
	{780, 940},
	{789, 941},
	{799, 942},
	{809, 943},
	{819, 944},
	{829, 945},
	{840, 946},
	{851, 947},
	{862, 948},
	{873, 949},
	{885, 950},
	{897, 951},
	{910, 952},
	{923, 953},
	{936, 954},
	{949, 955},
	{963, 956},
	{978, 957},
	{992, 958},
	{1008, 959},
	{1024, 960}
};

static void set_dummy(struct subdrv_ctx *ctx)
{
	LOG_INF("%s\n", __func__);
	LOG_INF("dummyline = %d, dummypixels = %d, frame_length(vmax) = %d\n",
		ctx->dummy_line, ctx->dummy_pixel, ctx->frame_length);
	return;
	/*
	 * you can set dummy by dummy_line and dummy_pixel,
	 * or you can set dummy by frame_length and line_length
	 */
	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
	write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
	write_cmos_sensor(ctx, 0x0342, ctx->line_length >> 8);
	write_cmos_sensor(ctx, 0x0343, ctx->line_length & 0xFF);
	write_cmos_sensor(ctx, 0x0104, 0x00);
}				/*    set_dummy  */

static kal_uint32 return_sensor_id(struct subdrv_ctx *ctx)
{
	LOG_INF("%s\n", __func__);
	mDELAY(10);
	return (((read_cmos_sensor(ctx, 0x0016) & 0x0f) << 8) | read_cmos_sensor(ctx, 0x0017));
}

static void set_max_framerate(struct subdrv_ctx *ctx, UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = ctx->frame_length;
	/* unsigned long flags; */

	LOG_INF("%s\n", __func__);
	return;
	LOG_INF("framerate = %d, min framelength should enable %d\n",
			framerate,
			min_framelength_en);

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
}				/*    set_max_framerate  */


static void write_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;

	LOG_INF("%s\n", __func__);

	if (shutter > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;
	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;
	if (shutter < imgsensor_info.min_shutter)
		shutter = imgsensor_info.min_shutter;

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk /
			ctx->line_length * 10 / ctx->frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
		else {
			write_cmos_sensor(ctx, 0x0104, 0x01);
			write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
			write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
			write_cmos_sensor(ctx, 0x0104, 0x00);
		}
	} else {
		write_cmos_sensor(ctx, 0x0104, 0x01);
		write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
		write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
		write_cmos_sensor(ctx, 0x0104, 0x00);
	}

}	/*	write_shutter  */


/*************************************************************************
 * FUNCTION
 *    set_shutter
 *
 * DESCRIPTION
 *    This function set e-shutter of sensor to change exposure time.
 *
 * PARAMETERS
 *    iShutter : exposured lines
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void set_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
	LOG_INF("%s\n", __func__);

	ctx->shutter = shutter;

	write_shutter(ctx, shutter);
} /*    set_shutter */

static kal_uint16 gain2reg(struct subdrv_ctx *ctx, const kal_uint32 gain)
{
	kal_uint16 iI;

	for (iI = 0; iI < (IMX355MIPI_MaxGainIndex-1); iI++) {
		if (gain <= imx355MIPI_sensorGainMapping[iI][0])
			break;
	}

	return imx355MIPI_sensorGainMapping[iI][1];
}

/*************************************************************************
 * FUNCTION
 *    set_gain
 *
 * DESCRIPTION
 *    This function is to set global gain to sensor.
 *
 * PARAMETERS
 *    iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *    the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint16 set_gain(struct subdrv_ctx *ctx, kal_uint32 gain)
{

	kal_uint16 reg_gain;

	LOG_INF("%s\n", __func__);
	/* 0x350A[0:1], 0x350B[0:7] AGC real gain */
	/* [0:3] = N meams N /16 X    */
	/* [4:9] = M meams M X         */
	/* Total gain = M + N /16 X   */

	/*  */
	if (gain < imgsensor_info.min_gain || gain > imgsensor_info.max_gain) {
		LOG_INF("Error gain setting");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else
			gain = imgsensor_info.max_gain;
	}

	reg_gain = gain2reg(ctx, gain);
	ctx->gain = reg_gain;
	LOG_INF("gain = %d, reg_gain = 0x%x\n ", gain, reg_gain);
	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x300B, (reg_gain >> 8) & 0x7);
	write_cmos_sensor(ctx, 0x300A, reg_gain & 0xFF);
	write_cmos_sensor(ctx, 0x0104, 0x00);

	return gain;

}				/*    set_gain  */


static void sensor_init(struct subdrv_ctx *ctx)
{
	LOG_INF("%s\n", __func__);
	write_cmos_sensor(ctx, 0x0100, 0x00);
	write_cmos_sensor(ctx, 0x0136, 0x18); /*external clock setting*/
	write_cmos_sensor(ctx, 0x0137, 0x00);
	write_cmos_sensor(ctx, 0x304E, 0x03); /*register version*/
	write_cmos_sensor(ctx, 0x4348, 0x16); /*global setting*/
	write_cmos_sensor(ctx, 0x4350, 0x19);
	write_cmos_sensor(ctx, 0x4408, 0x0A);
	write_cmos_sensor(ctx, 0x440C, 0x0B);
	write_cmos_sensor(ctx, 0x4411, 0x5F);
	write_cmos_sensor(ctx, 0x4412, 0x2C);
	write_cmos_sensor(ctx, 0x4623, 0x00);
	write_cmos_sensor(ctx, 0x462C, 0x0F);
	write_cmos_sensor(ctx, 0x462D, 0x00);
	write_cmos_sensor(ctx, 0x462E, 0x00);
	write_cmos_sensor(ctx, 0x4684, 0x54);
	write_cmos_sensor(ctx, 0x480A, 0x07);
	write_cmos_sensor(ctx, 0x4908, 0x07);
	write_cmos_sensor(ctx, 0x4909, 0x07);
	write_cmos_sensor(ctx, 0x490D, 0x0A);
	write_cmos_sensor(ctx, 0x491E, 0x0F);
	write_cmos_sensor(ctx, 0x490D, 0x0A);
	write_cmos_sensor(ctx, 0x4921, 0x06);
	write_cmos_sensor(ctx, 0x4923, 0x28);
	write_cmos_sensor(ctx, 0x4924, 0x28);
	write_cmos_sensor(ctx, 0x4925, 0x29);
	write_cmos_sensor(ctx, 0x4926, 0x29);
	write_cmos_sensor(ctx, 0x4927, 0x1F);
	write_cmos_sensor(ctx, 0x4928, 0x20);
	write_cmos_sensor(ctx, 0x4929, 0x20);
	write_cmos_sensor(ctx, 0x492A, 0x20);
	write_cmos_sensor(ctx, 0x492C, 0x05);
	write_cmos_sensor(ctx, 0x492D, 0x06);
	write_cmos_sensor(ctx, 0x492E, 0x06);
	write_cmos_sensor(ctx, 0x492F, 0x06);
	write_cmos_sensor(ctx, 0x4930, 0x03);
	write_cmos_sensor(ctx, 0x4931, 0x04);
	write_cmos_sensor(ctx, 0x4932, 0x04);
	write_cmos_sensor(ctx, 0x4933, 0x05);
	write_cmos_sensor(ctx, 0x595E, 0x01);
	write_cmos_sensor(ctx, 0x5963, 0x01);
	/* enable temperature sensor, TEMP_SEN_CTL: */
	/* write_cmos_sensor(ctx,0x0138, 0x01); */
}
static void capture_write_register(struct subdrv_ctx *ctx)
{
	/* reg_G1_24 */
	LOG_INF("%s\n", __func__);
	write_cmos_sensor(ctx, 0x0100, 0x00);
	write_cmos_sensor(ctx, 0x0112, 0x0A);/*mipi_setting*/
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);
	write_cmos_sensor(ctx, 0x0342, 0x0E);
	write_cmos_sensor(ctx, 0x0343, 0x58);
	write_cmos_sensor(ctx, 0x0340, 0x0A);
	write_cmos_sensor(ctx, 0x0341, 0x36);
	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x00);
	write_cmos_sensor(ctx, 0x0347, 0x00);
	write_cmos_sensor(ctx, 0x0348, 0x0C);
	write_cmos_sensor(ctx, 0x0349, 0xCF);
	write_cmos_sensor(ctx, 0x034A, 0x09);
	write_cmos_sensor(ctx, 0x034B, 0x9F);
	write_cmos_sensor(ctx, 0x0220, 0x00);/*mode setting*/
	write_cmos_sensor(ctx, 0x0222, 0x01);
	write_cmos_sensor(ctx, 0x0900, 0x00);
	write_cmos_sensor(ctx, 0x0901, 0x11);
	write_cmos_sensor(ctx, 0x0902, 0x00);
	write_cmos_sensor(ctx, 0x034C, 0x0C);/*output size setting*/
	write_cmos_sensor(ctx, 0x034D, 0xD0);
	write_cmos_sensor(ctx, 0x034E, 0x09);
	write_cmos_sensor(ctx, 0x034F, 0xA0);
	write_cmos_sensor(ctx, 0x0301, 0x05);/*clock setting*/
	write_cmos_sensor(ctx, 0x0303, 0x01);
	write_cmos_sensor(ctx, 0x0305, 0x02);
	write_cmos_sensor(ctx, 0x0306, 0x00);
	write_cmos_sensor(ctx, 0x0307, 0x78);
	write_cmos_sensor(ctx, 0x030B, 0x01);
	write_cmos_sensor(ctx, 0x030D, 0x02);
	write_cmos_sensor(ctx, 0x030E, 0x00);
	write_cmos_sensor(ctx, 0x030F, 0x3C);
	write_cmos_sensor(ctx, 0x0310, 0x00);
	write_cmos_sensor(ctx, 0x0700, 0x00);
	write_cmos_sensor(ctx, 0x0701, 0x10);
	write_cmos_sensor(ctx, 0x0820, 0x0B);
	write_cmos_sensor(ctx, 0x0821, 0x40);
	write_cmos_sensor(ctx, 0x3088, 0x04);
	write_cmos_sensor(ctx, 0x6813, 0x02);
	write_cmos_sensor(ctx, 0x6835, 0x07);
	write_cmos_sensor(ctx, 0x6836, 0x01);
	write_cmos_sensor(ctx, 0x6837, 0x04);
	write_cmos_sensor(ctx, 0x684D, 0x07);
	write_cmos_sensor(ctx, 0x684E, 0x01);
	write_cmos_sensor(ctx, 0x684F, 0x04);
	write_cmos_sensor(ctx, 0x0202, 0x0A);
	write_cmos_sensor(ctx, 0x0203, 0x2C);
	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);
}
static void preview_setting(struct subdrv_ctx *ctx)
{
	/* reg_G1_24 */
	LOG_INF("%s\n", __func__);
	write_cmos_sensor(ctx, 0x0100, 0x00);
	write_cmos_sensor(ctx, 0x0112, 0x0A);/*mipi_setting*/
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);
	write_cmos_sensor(ctx, 0x0342, 0x0E);
	write_cmos_sensor(ctx, 0x0343, 0x58);
	write_cmos_sensor(ctx, 0x0340, 0x0A);
	write_cmos_sensor(ctx, 0x0341, 0x36);
	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x00);
	write_cmos_sensor(ctx, 0x0347, 0x00);
	write_cmos_sensor(ctx, 0x0348, 0x0C);
	write_cmos_sensor(ctx, 0x0349, 0xCF);
	write_cmos_sensor(ctx, 0x034A, 0x09);
	write_cmos_sensor(ctx, 0x034B, 0x9F);
	write_cmos_sensor(ctx, 0x0220, 0x00);/*mode setting*/
	write_cmos_sensor(ctx, 0x0222, 0x01);
	write_cmos_sensor(ctx, 0x0900, 0x00);
	write_cmos_sensor(ctx, 0x0901, 0x11);
	write_cmos_sensor(ctx, 0x0902, 0x00);
	write_cmos_sensor(ctx, 0x034C, 0x0C);/*output size setting*/
	write_cmos_sensor(ctx, 0x034D, 0xD0);
	write_cmos_sensor(ctx, 0x034E, 0x09);
	write_cmos_sensor(ctx, 0x034F, 0xA0);
	write_cmos_sensor(ctx, 0x0301, 0x05);/*clock setting*/
	write_cmos_sensor(ctx, 0x0303, 0x01);
	write_cmos_sensor(ctx, 0x0305, 0x02);
	write_cmos_sensor(ctx, 0x0306, 0x00);
	write_cmos_sensor(ctx, 0x0307, 0x78);
	write_cmos_sensor(ctx, 0x030B, 0x01);
	write_cmos_sensor(ctx, 0x030D, 0x02);
	write_cmos_sensor(ctx, 0x030E, 0x00);
	write_cmos_sensor(ctx, 0x030F, 0x3C);
	write_cmos_sensor(ctx, 0x0310, 0x00);
	write_cmos_sensor(ctx, 0x0700, 0x00);
	write_cmos_sensor(ctx, 0x0701, 0x10);
	write_cmos_sensor(ctx, 0x0820, 0x0B);
	write_cmos_sensor(ctx, 0x0821, 0x40);
	write_cmos_sensor(ctx, 0x3088, 0x04);
	write_cmos_sensor(ctx, 0x6813, 0x02);
	write_cmos_sensor(ctx, 0x6835, 0x07);
	write_cmos_sensor(ctx, 0x6836, 0x01);
	write_cmos_sensor(ctx, 0x6837, 0x04);
	write_cmos_sensor(ctx, 0x684D, 0x07);
	write_cmos_sensor(ctx, 0x684E, 0x01);
	write_cmos_sensor(ctx, 0x684F, 0x04);
	write_cmos_sensor(ctx, 0x0202, 0x0A);
	write_cmos_sensor(ctx, 0x0203, 0x2C);
	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);
	mDELAY(10);
}

static kal_uint32 streaming_control(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	mDELAY(10);

	if (enable)
		write_cmos_sensor(ctx, 0x0100, 0x01);
	else
		write_cmos_sensor(ctx, 0x0100, 0x00);

	return ERROR_NONE;
}

static void capture_setting(struct subdrv_ctx *ctx, kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d ihdr:%d\n",
		currefps, ctx->ihdr_mode);
	capture_write_register(ctx);
/*
 *	imx355_table_write_cmos_sensor(ctx, addr_data_pair_capture_imx355,
 *	sizeof(addr_data_pair_capture_imx355) / sizeof(kal_uint16));
 */
}

static void normal_video_setting(struct subdrv_ctx *ctx, kal_uint16 currefps)
{
	LOG_INF("E! %s:%d\n", __func__, currefps);
	capture_write_register(ctx);
}

static void hs_video_setting(struct subdrv_ctx *ctx)
{
	capture_write_register(ctx);
	preview_setting(ctx);
	//	imx355_table_write_cmos_sensor(ctx, addr_data_pair_init_imx355,
	//	sizeof(addr_data_pair_init_imx355) / sizeof(kal_uint16));
}


static kal_uint32 set_test_pattern_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable)
		write_cmos_sensor(ctx, 0x0601, 0x02);
	else
		write_cmos_sensor(ctx, 0x0601, 0x00);

//	spin_lock(&imgsensor_drv_lock);
	ctx->test_pattern = enable;
//	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

/*************************************************************************
 * FUNCTION
 *    get_imgsensor_id
 *
 * DESCRIPTION
 *    This function get the sensor ID
 *
 * PARAMETERS
 *    *sensorID : return the sensor ID
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int get_imgsensor_id(struct subdrv_ctx *ctx, UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			*sensor_id = return_sensor_id(ctx);
			/* return_sensor_id(); */
			if (*sensor_id == imgsensor_info.sensor_id) {
				//imx355_read_SPC(ctx, imx355_SPC_data);
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			LOG_INF(
				"Read sensor id fail, write id: 0x%x, id: 0x%x\n",
				ctx->i2c_write_id, *sensor_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
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
 *    open
 *
 * DESCRIPTION
 *    This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *    None
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int open(struct subdrv_ctx *ctx)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;

	LOG_1;


	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			sensor_id = return_sensor_id(ctx);
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF(
					"i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, sensor_id);
				break;
			}
			LOG_INF(
				"Read sensor id fail, write id: 0x%x, id: 0x%x\n",
				ctx->i2c_write_id, sensor_id);
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

	/* initail sequence write in  */
	sensor_init(ctx);

	ctx->autoflicker_en = KAL_FALSE;
	ctx->sensor_mode = IMGSENSOR_MODE_INIT;
	ctx->shutter = 0x3D0;
	ctx->gain = 0x100;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->dummy_pixel = 0;
	ctx->dummy_line = 0;
	ctx->ihdr_mode = 0;
	ctx->test_pattern = KAL_FALSE;
	ctx->current_fps = imgsensor_info.pre.max_framerate;


	return ERROR_NONE;
} /* open */

/*************************************************************************
 * FUNCTION
 *    close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *    None
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int close(struct subdrv_ctx *ctx)
{
	/*No Need to implement this function */
	write_cmos_sensor(ctx, 0x0100, 0x00);	/*stream off */
	return ERROR_NONE;
}				/*    close  */


/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *    This function start the sensor preview.
 *
 * PARAMETERS
 *    *image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *    None
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
}				/*    preview   */

/*************************************************************************
 * FUNCTION
 *    capture
 *
 * DESCRIPTION
 *    This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	ctx->sensor_mode = IMGSENSOR_MODE_CAPTURE;
	ctx->pclk = imgsensor_info.cap.pclk;
	ctx->line_length = imgsensor_info.cap.linelength;
	ctx->frame_length = imgsensor_info.cap.framelength;
	ctx->min_frame_length = imgsensor_info.cap.framelength;
	ctx->autoflicker_en = KAL_FALSE;

	capture_setting(ctx, ctx->current_fps);	/*Full mode */


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

	normal_video_setting(ctx, ctx->current_fps);


	/* set_mirror_flip(sensor_config_data->SensorImageMirror); */

	return ERROR_NONE;
}				/*    normal_video   */

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
	ctx->autoflicker_en = KAL_FALSE;

	hs_video_setting(ctx);
	/* set_mirror_flip(sensor_config_data->SensorImageMirror); */
	return ERROR_NONE;
}				/*    hs_video   */


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
}				/*    get_resolution    */

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
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO] =
		imgsensor_info.hs_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;

	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;

	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->TEMPERATURE_SUPPORT = imgsensor_info.temperature_support;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

/* PDAF_SUPPORT_CAMSV; */
/*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode */
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
}				/*    get_info  */

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
	default:
		LOG_INF("Error ScenarioId setting");
		preview(ctx, image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}				/* control(ctx) */

static kal_uint32 set_video_mode(struct subdrv_ctx *ctx, UINT16 framerate)
{				/* This Function not used after ROME */
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)
		/* Dynamic frame rate */
		return ERROR_NONE;

	ctx->current_fps = framerate;
	set_max_framerate(ctx, ctx->current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(struct subdrv_ctx *ctx,
	kal_bool enable, UINT16 framerate)
{
	LOG_INF("%s\n", __func__);

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
		frame_length =
		    imgsensor_info.pre.pclk /
		    framerate * 10 / imgsensor_info.pre.linelength;
		ctx->dummy_line =
		    (frame_length > imgsensor_info.pre.framelength)
		    ? (frame_length - imgsensor_info.pre.framelength) : 0;
		ctx->frame_length =
			imgsensor_info.pre.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length =
		    imgsensor_info.normal_video.pclk /
		    framerate * 10 / imgsensor_info.normal_video.linelength;

		ctx->dummy_line =
		(frame_length > imgsensor_info.normal_video.framelength)
		? (frame_length - imgsensor_info.normal_video.framelength) : 0;

		ctx->frame_length =
		 imgsensor_info.normal_video.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		if (ctx->current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF(
			"Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			     framerate, imgsensor_info.cap.max_framerate / 10);

		frame_length = imgsensor_info.cap.pclk /
			framerate * 10 / imgsensor_info.cap.linelength;
		ctx->dummy_line =
		    (frame_length > imgsensor_info.cap.framelength)
		    ? (frame_length - imgsensor_info.cap.framelength) : 0;
		ctx->frame_length =
		    imgsensor_info.cap.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk /
		    framerate * 10 / imgsensor_info.hs_video.linelength;
		ctx->dummy_line =
		    (frame_length > imgsensor_info.hs_video.framelength)
		    ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
		ctx->frame_length =
		    imgsensor_info.hs_video.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	default:		/* coding with  preview scenario by default */
		frame_length = imgsensor_info.pre.pclk /
		    framerate * 10 / imgsensor_info.pre.linelength;
		ctx->dummy_line =
		    (frame_length > imgsensor_info.pre.framelength)
		    ? (frame_length - imgsensor_info.pre.framelength) : 0;
		ctx->frame_length =
			imgsensor_info.pre.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
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
	/*LOG_INF("scenario_id = %d\n", scenario_id); */

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

	default:
		break;
	}

	return ERROR_NONE;
}

static unsigned long long get_output_format_by_scenario(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id)
{
		unsigned long long feature_data = 0;

		switch (scenario_id) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
			feature_data
			= (enum ACDK_SENSOR_OUTPUT_DATA_FORMAT_ENUM)
				imgsensor_info.sensor_output_dataformat;
			break;
		default:
			break;
		}
		return feature_data;
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
	case SENSOR_FEATURE_GET_OUTPUT_FORMAT_BY_SCENARIO:
		*(feature_data + 1) = get_output_format_by_scenario(ctx,
				(enum MSDK_SCENARIO_ID_ENUM)*feature_data);
	break;
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
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain(ctx, (UINT32)*feature_data);
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
		LOG_INF("hdr enable :%d\n", (BOOL)*feature_data);
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
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16)*feature_data);
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
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
		(*(feature_data + 1)) = 1;
		(*(feature_data + 2)) = imgsensor_info.margin;
		break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		default:
			*feature_return_para_32 = 1; /*BINNING_AVERAGE*/
			break;
		}
		pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
		*feature_para_len = 4;

		break;
	case SENSOR_FEATURE_GET_SENSOR_SYNC_MODE:
		/*
		 * This is the implementaton of getting sensor sync mode,
		 * please return one of
		 * SENSOR_MASTER_SYNC_MODE/SENSOR_SAVE_SYNC_MODE/SENSOR_NO_SYNC_MODE
		 * by the current sensor sync mode setting
		 */
		*feature_return_para_32 = SENSOR_MASTER_SYNC_MODE;
		*feature_para_len = 4;
		if (*feature_return_para_32 == SENSOR_MASTER_SYNC_MODE) {
			/* master mode */
			LOG_INF("set to master mode\n");
			write_cmos_sensor(ctx, 0x31A1, 0x00);
			write_cmos_sensor(ctx, 0x37B0, 0x36);
		} else {
			/* slave mode */
			LOG_INF("set to slave mode\n");
			write_cmos_sensor(ctx, 0x31A1, 0x0F);
			write_cmos_sensor(ctx, 0x37B0, 0x37);
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
			.hsize = 0x0CD0,
			.vsize = 0x09A0,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0CD0,
			.vsize = 0x09A0,
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
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cap);
		memcpy(fd->entry, frame_desc_cap, sizeof(frame_desc_cap));
		break;
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
	.ana_gain_step = 1,
	.exposure_def = 0x3D0,
	.exposure_max = 0xfff0,
	.exposure_min = 1,
	.exposure_step = 1,
	.max_frame_length = 0xffff,

	.mirror = IMAGE_NORMAL,	/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,	/* current shutter */
	.gain = 0x100,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	/* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	/* auto flicker enable: KAL_FALSE for disable auto flicker,
	 * KAL_TRUE for enable auto flicker
	 */
	.test_pattern = KAL_FALSE,

	/* current scenario id */
	.current_scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW,
	.ihdr_mode = 0,	/* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x34,	/* record current sensor's i2c write id */
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
	{HW_ID_RST, 0, 5},
	{HW_ID_AVDD, 2700000, 1},
	{HW_ID_DVDD, 1800000, 1},
	{HW_ID_DOVDD, 1800000, 1},
	{HW_ID_RST, 1, 5},
	{HW_ID_MCLK, 24, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, 8, 5},
};

const struct subdrv_entry imx355_mipi_raw_entry = {
	.name = "imx355_mipi_raw",
	.id = IMX355_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};
