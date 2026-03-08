/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#define LOG_TAG "DOVI_COMMON_IF"
#include <linux/types.h>

#include "disp_info.h"

#include "disp_dovi_md_parser.h"
#include "disp_hw_mgr.h"
#include "dovi_vdo_fe_hal.h"
#include "dovi_gfx_fe_hal.h"
#include "dovi_be_hal.h"
#include "disp_dovi_vdo_fe_if.h"
#include "disp_dovi_gfx_fe_if.h"
#include "disp_dovi_be_if.h"
#include "disp_adl_if.h"
#include "disp_ml_if.h"
#include "dovi_log.h"
#include "disp_dovi_tz_client.h"
#include "dovi_table.h"
#include "disp_dovi_io.h"
#include "disp_vdp_vsync.h"
#include "disp_path.h"
#include "fmt_hal.h"
#include "disp_dovi_main.h"
#include "disp_hdr_main.h"
#include "disp_info.h"
#include "disp_hdr_if.h"
#include "disp_dovi_common_if.h"
#include "disp_fefifo_if.h"
#include "dovi_common_hal.h"


#define START_CODE_LEN_MAX 5
/* 0x7C 0x01 0x19 */
#define NAL_TYPE_LEN_MAX 3

uint32_t _u4VSyncCount;
uint32_t dovi_enable;
/*dovi_enable enable
 * 0 0 already disable   0
 * 0 1 disable to enable 1
 * 1 0 enable to disable 2
 * 1 1 already enable    3
 */
uint32_t dovi_proc_state;

uint32_t dovi_out_res;
uint32_t dovi_out_width;
uint32_t dovi_out_height;

bool dovi_md_parser_enable;
struct dv_reg_tbl hw_reset_table;

/*IDK2.6 new define*/
struct src_params_t *p_src_param;
struct cp_param_t *p_cp_param;
uint32_t *p_sec_layer;
struct dv_hw_reg *p_dv_out_params;
uint32_t *p_hdmi_md;
struct mtk_disp_hdr10_md_t *p_hdr10_md;
struct vsif_param_t *p_vsif;

uint32_t dump_md_enable;
uint32_t reg_test_enable;
int *p_profile4;
bool set_graphic_max_lum_enable;
bool set_video_max_lum_enable;

int graphic_max_lum;
int video_max_lum;

struct mtk_disp_hdr_md_info_t dovi_hdr_md_info[V_G_LAYER_MAX];
/* record whether current main path is playing dovi video. */
uint32_t dovi_path_en;
uint32_t dovi_vs10_path_en;
uint32_t dovi_path_ready2start;
enum dovi_signal_format_t dovi_out_format;
bool dovi_force_output;
bool dovi_out_format_change;
enum dovi_signal_format_t dovi_force_out_format;
enum HDMI_VIDEO_RESOLUTION old_resolution;
struct mutex disp_dovi_mutex;
uint32_t dovi_hdmi_brightness_en;
uint32_t dovi_hdmi_brightness;
uint32_t sdk_vsem;

/*
 *IDK Cert special flow
 *In IDK Cert, we must set one buf show idk_dump_vsync_cnt vsyncs
 *for dump data from dovi be by video in.
 */
bool dovi_idk_dump;
uint32_t dovi_idk_disp_cnt;
int32_t idk_dump_vsync_cnt;
bool dovi_idk_dump_set_vin;
uint32_t dovi_ipcore_version = 4;
struct dv_reg_tbl dv_bypass_table[2];
int32_t fhd_color_format;
int32_t uhd_color_format;
bool dovi_black_pattern_en;
uint32_t dovi_black_pattern_cnt;
uint32_t dovi_black_pattern_cnt_max = 5;
uint32_t f_graphic_off_cmd;
uint32_t set_vsvdb;
uint32_t dovi_get_ext_md;

void dovi_set_hw_path(void)
{
	int sof_start = 0;
	int sof_end = 0;

	if (dv_vdo_fe_en[0]) {
		disp_path_set_hw_path(DISP_PATH_M_HDR_VDO_FE, 1);
		disp_path_get_sof_info(DISP_SOF_6_M_HDR_FE_STA,
			DISP_SOF_6_M_HDR_FE_END,
			&sof_start, &sof_end);
		fmt_hal_set_sof(FMT_SOF_6_M_HDR_FE_STA,
			FMT_SOF_6_M_HDR_FE_END,
			sof_start, sof_end);
	} else
		disp_path_set_hw_path(DISP_PATH_M_HDR_VDO_FE, 0);

	if (dv_vdo_fe_en[1]) {
		disp_path_set_hw_path(DISP_PATH_S_HDR_VDO_FE, 1);
		disp_path_get_sof_info(DISP_SOF_13_S_HDR_FE_STA,
			DISP_SOF_13_S_HDR_FE_END,
			&sof_start, &sof_end);
		fmt_hal_set_sof(FMT_SOF_13_S_HDR_FE_STA,
			FMT_SOF_13_S_HDR_FE_END,
			sof_start, sof_end);
	} else
		disp_path_set_hw_path(DISP_PATH_S_HDR_VDO_FE, 0);

	if (dv_gfx_fe_en[0]) {
		disp_path_set_hw_path(DISP_PATH_FHD_HDR_GFX_FE, 1);
		disp_path_get_sof_info(DISP_SOF_19_FHD_HDR_FE_STA,
			DISP_SOF_19_FHD_HDR_FE_END,
			&sof_start, &sof_end);
		fmt_hal_set_sof(FMT_SOF_19_FHD_HDR_FE_STA,
			FMT_SOF_19_FHD_HDR_FE_END,
			sof_start, sof_end);
	} else
		disp_path_set_hw_path(DISP_PATH_FHD_HDR_GFX_FE, 0);

	if (dv_gfx_fe_en[1]) {
		disp_path_set_hw_path(DISP_PATH_UHD_HDR_GFX_FE, 1);
		disp_path_get_sof_info(DISP_SOF_16_UHD_HDR_FE_STA,
			DISP_SOF_16_UHD_HDR_FE_END,
			&sof_start, &sof_end);
		fmt_hal_set_sof(FMT_SOF_16_UHD_HDR_FE_STA,
			FMT_SOF_16_UHD_HDR_FE_END,
			sof_start, sof_end);
	} else
		disp_path_set_hw_path(DISP_PATH_UHD_HDR_GFX_FE, 0);

	if (dv_vdo_be_en) {
		disp_path_set_hw_path(DISP_PATH_DISP_HDR_VDO_BE, 1);
		disp_path_get_sof_info(DISP_SOF_21_HDR_BE_STA,
			DISP_SOF_21_HDR_BE_END,
			&sof_start, &sof_end);
		fmt_hal_set_sof(FMT_SOF_21_HDR_BE_STA,
			FMT_SOF_21_HDR_BE_END,
			sof_start, sof_end);
	} else
		disp_path_set_hw_path(DISP_PATH_DISP_HDR_VDO_BE, 0);

	dovi_default("set path(%d %d)(%d %d)(%d)\n",
		dv_vdo_fe_en[0], dv_vdo_fe_en[1],
		dv_gfx_fe_en[0], dv_gfx_fe_en[1],
		dv_vdo_be_en);

}
uint32_t dovi_set_video_info(struct mtk_disp_hdr_md_info_t *hdr_metadata)
{
	dovi_func();

	p_src_param[LAYER0].width = dovi_out_width;
	p_src_param[LAYER0].height = dovi_out_height;

	switch (hdr_metadata->dr_range) {
	case DISP_DR_TYPE_DOVI:
		p_src_param[LAYER0].input_format = SIGNAL_FORMAT_DOVI;
		break;
	case DISP_DR_TYPE_HDR10:
		p_src_param[LAYER0].input_format = SIGNAL_FORMAT_HDR10;
		break;
	case DISP_DR_TYPE_HLG:
		p_src_param[LAYER0].input_format = SIGNAL_FORMAT_HLG;
		break;
	default:
		p_src_param[LAYER0].input_format = SIGNAL_FORMAT_SDR10;
		break;
	}

	p_src_param[LAYER0].dr_type = hdr_metadata->dr_range;
	p_src_param[LAYER0].input_mode = INPUT_MODE_OTT;

	p_src_param[LAYER0].src_yuv_range = SIGNAL_RANGE_SMPTE;
	p_src_param[LAYER0].src_bit_depth = 10;
	p_src_param[LAYER0].color_format = CP_CLR_YUV;
	p_src_param[LAYER0].chroma_format = CHROMA_FORMAT_UYVY;
	p_src_param[LAYER0].src_fps = 60;

	dovi_info("set video info:W:%d x H:%d, input_format:%d\n",
		     p_src_param[LAYER0].width, p_src_param[LAYER0].height,
		     p_src_param[LAYER0].input_format);

	return DOVI_RET_OK;
}

uint32_t dovi_set_sub_video_info(struct mtk_disp_hdr_md_info_t *hdr_metadata)
{
	dovi_func();

	p_src_param[LAYER1].width = dovi_out_width;
	p_src_param[LAYER1].height = dovi_out_height;

	switch (hdr_metadata->dr_range) {
	case DISP_DR_TYPE_DOVI:
		p_src_param[LAYER1].input_format = SIGNAL_FORMAT_DOVI;
		break;
	case DISP_DR_TYPE_HDR10:
		p_src_param[LAYER1].input_format = SIGNAL_FORMAT_HDR10;
		break;
	case DISP_DR_TYPE_HLG:
		p_src_param[LAYER1].input_format = SIGNAL_FORMAT_HLG;
		break;
	default:
		p_src_param[LAYER1].input_format = SIGNAL_FORMAT_SDR10;
		break;
	}

	p_src_param[LAYER1].dr_type = hdr_metadata->dr_range;
	p_src_param[LAYER1].input_mode = INPUT_MODE_OTT;

	p_src_param[LAYER1].src_yuv_range = SIGNAL_RANGE_SMPTE;
	p_src_param[LAYER1].src_bit_depth = 10;
	p_src_param[LAYER1].color_format = CP_CLR_YUV;
	p_src_param[LAYER1].chroma_format = CHROMA_FORMAT_UYVY;
	p_src_param[LAYER1].src_fps = 60;

	dovi_info("set sub video info:W:%d x H:%d, input_format:%d\n",
		     p_src_param[LAYER1].width, p_src_param[LAYER1].height,
		     p_src_param[LAYER1].input_format);

	return DOVI_RET_OK;
}

uint32_t dovi_set_video_input_format(enum signal_fmt_t e_input_format)
{
	if (p_cp_param != NULL) {
		p_src_param[LAYER0].input_format = e_input_format;

		dovi_info("[DOVI]Re-Update HDR Type Input:%d\n",
			e_input_format);
	} else
		dovi_info("%s share mem not init\n", __func__);

	return DOVI_RET_OK;
}

uint32_t dovi_get_input_format(enum DISP_DR_TYPE_T *dovi_input_dr)
{
	if ((p_cp_param != NULL) && (dovi_input_dr != NULL)) {
		switch (p_src_param[LAYER0].input_format) {
		case SIGNAL_FORMAT_DOVI:
			*dovi_input_dr = DISP_DR_TYPE_DOVI;
			break;
		case SIGNAL_FORMAT_HDR10:
			*dovi_input_dr = DISP_DR_TYPE_HDR10;
			break;
		case SIGNAL_FORMAT_HLG:
			*dovi_input_dr = DISP_DR_TYPE_HLG;
			break;
		case SIGNAL_FORMAT_SDR8:
			*dovi_input_dr = DISP_DR_TYPE_SDR;
			break;
		default:
			*dovi_input_dr = DISP_DR_TYPE_SDR;
			break;
		}
	}
	return DOVI_RET_OK;
}

uint32_t dovi_get_input_format1(enum DISP_DR_TYPE_T *dovi_input_dr)
{
	if ((p_cp_param != NULL) && (dovi_input_dr != NULL)) {
		switch (p_src_param[LAYER1].input_format) {
		case SIGNAL_FORMAT_DOVI:
			*dovi_input_dr = DISP_DR_TYPE_DOVI;
			break;
		case SIGNAL_FORMAT_HDR10:
			*dovi_input_dr = DISP_DR_TYPE_HDR10;
			break;
		case SIGNAL_FORMAT_HLG:
			*dovi_input_dr = DISP_DR_TYPE_HLG;
			break;
		case SIGNAL_FORMAT_SDR8:
			*dovi_input_dr = DISP_DR_TYPE_SDR;
			break;
		default:
			*dovi_input_dr = DISP_DR_TYPE_SDR;
			break;
		}
	}
	return DOVI_RET_OK;
}

uint32_t dovi_set_graphic_format(uint32_t g_format)
{
	dovi_func();
	dovi_info("set_graphic_format %d\n", g_format);

	p_src_param[LAYER2].color_format = g_format;

	return DOVI_RET_OK;
}

uint32_t dovi_set_uhd_graphic_format(uint32_t g_format)
{
	dovi_func();
	dovi_info("set_graphic_format %d\n", g_format);

	p_src_param[LAYER3].color_format = g_format;

	return DOVI_RET_OK;
}

uint32_t dovi_set_be_out_css(enum chroma_format_t out_css)
{
	dovi_func();
	dovi_info("dovi set out_css %d\n", out_css);

	if (p_cp_param != NULL)
		p_cp_param->out_chroma_format = out_css;

	return DOVI_RET_OK;
}

uint32_t dovi_set_graphic_info(uint32_t ucOn)
{
	dovi_func();
	dovi_info("set_graphic_on %d\n", ucOn);

	if ((p_src_param[LAYER2].en == ucOn) && !dovi_idk_test)
		return DOVI_RET_OK;

	if (ucOn)
		p_src_param[LAYER2].en = true;
	else
		p_src_param[LAYER2].en = false;

	if (f_graphic_off_cmd)
		p_src_param[LAYER2].en = false;

	p_src_param[LAYER2].input_mode = INPUT_MODE_GRAPHICS;
	p_src_param[LAYER2].input_format = SIGNAL_FORMAT_SDR8;
	p_src_param[LAYER2].src_bit_depth = 8;
	p_src_param[LAYER2].src_yuv_range = SIGNAL_RANGE_FULL;

	if (dovi_idk_test && (pip_fhd_format == FORMAT_HDR)) {
		p_src_param[LAYER2].input_format = SIGNAL_FORMAT_HDR8;
		p_src_param[LAYER2].src_yuv_range = SIGNAL_RANGE_SMPTE;
	} else if (dovi_idk_test && (pip_fhd_format == FORMAT_DOVI))
		p_src_param[LAYER2].input_format = SIGNAL_FORMAT_DOVI;

	return DOVI_RET_OK;
}

uint32_t dovi_set_graphic_info_uhd(uint32_t ucOn)
{
	dovi_func();
	dovi_info("set_graphic_on uhd %d\n", ucOn);

	if ((p_src_param[LAYER3].en == ucOn) && !dovi_idk_test)
		return DOVI_RET_OK;

	if (ucOn)
		p_src_param[LAYER3].en = true;
	else
		p_src_param[LAYER3].en = false;

	if (f_graphic_off_cmd)
		p_src_param[LAYER3].en = false;

	p_src_param[LAYER3].input_mode = INPUT_MODE_GRAPHICS;
	p_src_param[LAYER3].input_format = SIGNAL_FORMAT_SDR8;
	p_src_param[LAYER3].src_bit_depth = 8;
	p_src_param[LAYER3].src_yuv_range = SIGNAL_RANGE_FULL;

	if (dovi_idk_test && (pip_uhd_format == FORMAT_HDR)) {
		p_src_param[LAYER3].input_format = SIGNAL_FORMAT_HDR8;
		p_src_param[LAYER3].src_yuv_range = SIGNAL_RANGE_SMPTE;
	} else if (dovi_idk_test && (pip_uhd_format == FORMAT_DOVI))
		p_src_param[LAYER3].input_format = SIGNAL_FORMAT_DOVI;

	return DOVI_RET_OK;
}

uint32_t dovi_set_gfx_rpu_info(void)
{
	char *gfx_md_file_name;
	uint32_t md_len = 100;

	dovi_func();

	gfx_md_file_name =
		get_dovi_pip_gfx1_md(dovi_idk_file_id);
	if (gfx_md_file_name != NULL)
		disp_dovi_load_buffer(gfx_md_file_name,
		(uint8_t *)(p_src_param[LAYER2].rpu_bs_buffer),
		md_len);
	p_src_param[LAYER2].rpu_bs_len = md_len;
	dovi_info("idk2.6 gfx rpu md %p len %d\n",
		(p_src_param[LAYER2].rpu_bs_buffer),
		p_src_param[LAYER2].rpu_bs_len);

	return DOVI_RET_OK;
}

uint32_t dovi_set_priority_mode(uint32_t mode)
{
	p_cp_param->priority_mode = mode;

	if (force_priority_mode)
		p_cp_param->priority_mode =
		(enum pri_mode_t)priority_mode;

	if (!dovi_idk_test) {
		if (p_cp_param->priority_mode == G_PRIORITY)
			dovi_set_dovi2hdr10_mapping(1);
		else
			dovi_set_dovi2hdr10_mapping(0);
	}
	dovi_info("set_priority_mode %d %d %d\n", mode,
		p_cp_param->output_format, p_cp_param->priority_mode);

	return DOVI_RET_OK;
}

uint32_t dovi_set_test_mode(int mode)
{
	p_cp_param->test_mode = mode;

	dovi_info("set_test_mode %d\n", p_cp_param->test_mode);

	return DOVI_RET_OK;
}

uint32_t dovi_set_support_el(int value)
{
	//p_cp_param->support_el = value;

	//dovi_info("set_support_el %d\n", p_cp_param->support_el);

	return DOVI_RET_OK;
}

uint32_t dovi_get_priority_mode(uint32_t *mode)
{
	dovi_func();
	dovi_info("get_priority_mode %d\n",
		p_cp_param->priority_mode);
	*mode = p_cp_param->priority_mode;

	return DOVI_RET_OK;
}

uint32_t dovi_update_graphic_info(void)
{
	int max_lum = 0;

	dovi_func();
	dovi_info("update_graphic_info\n");

	if (p_cp_param->output_format == SIGNAL_FORMAT_SDR8)
		max_lum = 1000000;
	else if (p_cp_param->output_format == SIGNAL_FORMAT_HDR10)
		max_lum = 3000000;
	else if (p_cp_param->output_format == SIGNAL_FORMAT_HLG)
		max_lum = 1000000;
	else if (p_cp_param->output_format == SIGNAL_FORMAT_DOVI)
		max_lum = 3000000;
	else
		max_lum = 3000000;

	p_src_param[LAYER2].min = 50;
	p_src_param[LAYER3].min = 50;
	p_src_param[LAYER2].max = max_lum;
	p_src_param[LAYER3].max = max_lum;

	if (set_graphic_max_lum_enable) {
		p_src_param[LAYER2].max = graphic_max_lum * 10000;
		p_src_param[LAYER3].max = graphic_max_lum * 10000;
	}

	dovi_info("set graphic_max_lum %d!\n", p_src_param[LAYER2].max);
	p_src_param[LAYER2].src_bit_depth = 8;
	p_src_param[LAYER3].src_bit_depth = 8;
	/*if (!dovi_idk_test) */
	p_src_param[LAYER2].color_format = CP_CLR_RGB;
	p_src_param[LAYER3].color_format = CP_CLR_RGB;
	p_src_param[LAYER2].chroma_format = CHROMA_FORMAT_P444;
	p_src_param[LAYER3].chroma_format = CHROMA_FORMAT_P444;

	if (dovi_idk_dump) {
		if (pip_fhd_format == FORMAT_HDR)
			p_src_param[LAYER2].max = 10000000;
		else
			p_src_param[LAYER2].max = 3000000;

		if (pip_uhd_format == FORMAT_HDR)
			p_src_param[LAYER3].max = 10000000;
		else
			p_src_param[LAYER3].max = 3000000;

		p_src_param[LAYER2].color_format = fhd_color_format;
		p_src_param[LAYER3].color_format = uhd_color_format;
		dovi_info("set idk gfx_max_lum %d color format %d %d!\n",
			p_src_param[LAYER2].max,
			p_src_param[LAYER2].color_format,
			p_src_param[LAYER3].color_format);
	}

	return DOVI_RET_OK;
}

uint32_t dovi_set_output_format(enum dovi_signal_format_t out_format)
{
	enum signal_fmt_t format = SIGNAL_FORMAT_DOVI;

	dovi_func();

	/*  The output are all dovi, when we set dovi or dovi_ll.
	 *  dovi core will convert dovi to dovi_ll according
	 *  the vsvdb and use_ll.
	 */

	/*if vsem form1 keep output disable be 444to422*/
	if (out_format == DOVI_FORMAT_VSEM_DOVI)
		dovi_set_be_out_css(CHROMA_FORMAT_P444);
	else
		dovi_set_be_out_css(CHROMA_FORMAT_UYVY);

	switch (out_format) {
	case DOVI_FORMAT_DOVI_LOW_LATENCY:
	case DOVI_FORMAT_DOVI:
	case DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY:
	case DOVI_FORMAT_VSEM_DOVI:
		format = SIGNAL_FORMAT_DOVI;
		break;
	case DOVI_FORMAT_HDR10:
		format = SIGNAL_FORMAT_HDR10;
		break;
	case DOVI_FORMAT_SDR:
		format = SIGNAL_FORMAT_SDR8;
		break;
	case DOVI_FORMAT_SDR_2020:
		format = SIGNAL_FORMAT_SDR10;
		break;
	case DOVI_FORMAT_HLG:
		format = SIGNAL_FORMAT_HLG;
		break;
	default:
		break;
	}

	/*trick for 8695 can play hlg in dovi path */
	//if (out_format == DOVI_FORMAT_HLG)
	//	out_format = DOVI_FORMAT_HDR10;

	p_cp_param->output_format = format;
	dovi_info("set output format %d\n", format);

	return DOVI_RET_OK;
}

uint32_t dovi_get_output_format(void)
{
	dovi_func();
	dovi_info("get output format %d\n", p_cp_param->output_format);

	return (uint32_t) p_cp_param->output_format;
}

/* get tv capability */
static enum dovi_signal_format_t get_tv_output_format(
struct disp_hw_tv_capbility *tv_cap,
bool is_2160p60_out,
const struct disp_hw_resolution *resolution)
{
	enum dovi_signal_format_t tv_out_format;
	bool is_low_latency = false;

	/* dovi hw not support 4096 dovi output */
	if ((tv_cap->is_support_dovi_2160p60 ||
		(tv_cap->is_support_dovi && !is_2160p60_out))
	    && (resolution->width <= 3840)) {
		if (tv_cap->is_support_dovi_low_latency) {
			#if 0
			is_low_latency = true;
			#else
			/* vsvdb version2: interface 0 && 1 only supportll,
			 * 2&&3 support both ll and std
			 */
			if (tv_cap->dovi_vsvdb_version == 0x2) {
				if ((tv_cap->dovi_vsvdb_v2_interface
					== 0x0)
				    || (tv_cap->dovi_vsvdb_v2_interface
				    == 0x1))
					is_low_latency = true;
			} else
				is_low_latency = false;
			#endif
		} else
			is_low_latency = false;

		if (is_low_latency)
			tv_out_format = DOVI_FORMAT_DOVI_LOW_LATENCY;
		else
			tv_out_format = DOVI_FORMAT_DOVI;
	} else if (tv_cap->is_support_dovi && is_2160p60_out
	//&& !(tv_cap->is_support_dovi_low_latency)) {
		   && (tv_cap->dovi_vsvdb_version == 0x2)
		   && ((tv_cap->dovi_vsvdb_v2_interface == 0x2)
		   || (tv_cap->dovi_vsvdb_v2_interface == 0x3))) {
		is_low_latency = false;
		tv_out_format = DOVI_FORMAT_DOVI;
	} else if (tv_cap->is_support_dovi_low_latency && is_2160p60_out) {
		is_low_latency = true;
		tv_out_format = DOVI_FORMAT_DOVI_LOW_LATENCY;
	} else if (tv_cap->is_support_hdr)
		tv_out_format = DOVI_FORMAT_HDR10;
	else if (tv_cap->is_support_hlg)
		tv_out_format = DOVI_FORMAT_HLG;
	else
		tv_out_format = DOVI_FORMAT_SDR;

	/*if game source, we keep output as lowlatency when tv support*/
	if ((hdr_allm_en || (ui_allm_type == ALLM_EN))
		&& (tv_cap->is_support_dovi_low_latency)
		&& ((tv_cap->u1_sink_allm_support) ||
		(tv_cap->u1_sink_14gamemode_support))) {
		is_low_latency = true;
		tv_out_format = DOVI_FORMAT_DOVI_LOW_LATENCY;
		dovi_info("keep lowlatency due to allm\n");
	}

	/*check vsvdb dmversion*/
	if ((tv_cap->dovi_vsvdb_version == 2) &&
		((tv_cap->dovi_vsvdb_dm_version == 3) ||
		(tv_cap->dovi_vsvdb_dm_version == 5) ||
		(tv_cap->dovi_vsvdb_dm_version == 6) ||
		(tv_cap->dovi_vsvdb_dm_version == 7))
		&& sdk_vsem) {
		if (tv_out_format == DOVI_FORMAT_DOVI)
			tv_out_format = DOVI_FORMAT_VSEM_DOVI;
		else if (tv_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY)
			tv_out_format = DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY;
	}

	/* make sure chosen format can be supported - if clock/tmds rate is
	 *not enough, move to a lower format
	 */
	if ((tv_cap->max_tmds_rate < DOVI_RES4K60_TMDS_RATE)
		&& is_2160p60_out) {
		if ((tv_out_format == DOVI_FORMAT_DOVI) ||
		    (tv_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY) ||
		    (tv_out_format == DOVI_FORMAT_VSEM_DOVI) ||
		    (tv_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
			dovi_info("2160p60 DOVI canot supported: maxrate=%d\n",
				   tv_cap->max_tmds_rate);
		if (tv_cap->is_support_hdr)
			tv_out_format = DOVI_FORMAT_HDR10;
		else if (tv_cap->is_support_hlg)
			tv_out_format = DOVI_FORMAT_HLG;
		else
			tv_out_format = DOVI_FORMAT_SDR;
	}
	/* HDR10/HLG checks for 4k modes and 10 bits bandwidth - fall back to SDR
	   if bandwidth not enought for 10 bits atleast:
	   1. if colorspace is yuv420, we need character rate of 297 * 10/8 atleast
	   2. if colorspace is yuv422, it will be chosen only if we have sufficient
	   bandwidth.
	   So, we only need a check for 297 * 10 / 8 atleast here */
	if ((resolution->width == 3840 && resolution->height == 2160) &&
	    (tv_out_format == DOVI_FORMAT_HDR10 ||
	     tv_out_format == DOVI_FORMAT_HLG) &&
	    (tv_cap->max_tmds_rate * 8 < RES4K_MIN_TMDS_RATE * 10 )) {
		tv_out_format = DOVI_FORMAT_SDR;
	}

	dovi_info("%s out format %d\n", __func__, tv_out_format);

	return tv_out_format;
}

bool dovi_get_profile4(void)
{
	return *p_profile4;
}

uint32_t dovi_set_target_lum(int target_min_lum, int target_max_lum)
{
	dovi_func();

	p_cp_param->min = target_min_lum;
	p_cp_param->max = target_max_lum;

	return DOVI_RET_OK;
}

uint32_t dovi_update_target_lum(void)
{
	int target_min_lum;
	int target_max_lum;

	dovi_func();

	target_min_lum = 50;
	if (p_src_param[LAYER0].input_format == SIGNAL_FORMAT_DOVI) {
		switch (p_cp_param->output_format) {
		case SIGNAL_FORMAT_DOVI:
			target_max_lum = 5000000;	/* 40000000; */
			break;
		case SIGNAL_FORMAT_HDR10:
			target_max_lum = 10000000;	/* 40000000; */
			break;
		case SIGNAL_FORMAT_HLG:
			target_max_lum = 3000000;	/* 40000000; */
			break;
		default:
			target_max_lum = 1000000;
			break;
		}
	} else if (p_src_param[LAYER0].input_format == SIGNAL_FORMAT_HDR10) {
		switch (p_cp_param->output_format) {
		case SIGNAL_FORMAT_DOVI:
			target_max_lum = 5000000;
			break;
		case SIGNAL_FORMAT_HDR10:
			target_max_lum = 4000000;
			break;
		case SIGNAL_FORMAT_HLG:
			target_max_lum = 3000000;
			break;
		default:
			target_max_lum = 1000000;
			break;
		}
	} else if (p_src_param[LAYER0].input_format == SIGNAL_FORMAT_HLG) {
		switch (p_cp_param->output_format) {
		case SIGNAL_FORMAT_DOVI:
			target_max_lum = 5000000;
			break;
		case SIGNAL_FORMAT_HDR10:
			target_max_lum = 10000000;
			break;
		case SIGNAL_FORMAT_HLG:
			target_max_lum = 3000000;
			break;
		default:
			target_max_lum = 1000000;
			break;
		}
	} else {
		switch (p_cp_param->output_format) {
		case SIGNAL_FORMAT_DOVI:
			target_max_lum = 5000000;
			break;
		case SIGNAL_FORMAT_HDR10:
			target_max_lum = 4000000;
			break;
		case SIGNAL_FORMAT_HLG:
			target_max_lum = 3000000;
			break;
		default:
			target_max_lum = 1000000;
			break;
		}
	}

	if (p_cp_param->output_format == SIGNAL_FORMAT_SDR8)
		p_src_param[LAYER2].max = 1000000;

	/* adjust video lum with cli */
	if (set_video_max_lum_enable)
		target_max_lum = video_max_lum * 10000;

	p_cp_param->min = target_min_lum;
	p_cp_param->max = target_max_lum;

	dovi_info("out_format %d %d, target lum %d %d\n",
		  p_cp_param->output_format, p_src_param[LAYER0].input_format,
		  target_min_lum, target_max_lum);

	return DOVI_RET_OK;
}


uint32_t dovi_set_out_res(uint32_t out_res,
	uint16_t width, uint16_t height)
{
	/* dovi_func(); */
	dovi_out_res = out_res;
	dovi_out_width = width;
	dovi_out_height = height;

	return DOVI_RET_OK;
}

uint32_t dovi_set_composer_mode(bool fgComposerEL)
{
	dovi_vdo_fe_hal_set_composer_mode(fgComposerEL);
	return DOVI_RET_OK;
}

uint32_t dovi_set_low_latency_mode(int use_ll, int ll_rgb_desired)
{
	dovi_func();

	dovi_info("set use_ll %d ll_rgb_desired %d\n",
		use_ll, ll_rgb_desired);

	p_cp_param->use_ll = use_ll;
	p_cp_param->ll_rgb_desired = ll_rgb_desired;

	return DOVI_RET_OK;
}

uint32_t dovi_set_vsem_mode(int use_vsem)
{
	dovi_func();

	dovi_flow("set vsem %d\n", use_vsem);

	p_cp_param->use_vsem = use_vsem;

	return DOVI_RET_OK;
}

uint32_t dovi_get_low_latency_mode(void)
{
	dovi_func();

	dovi_info("get use_ll  %d\n", p_cp_param->use_ll);

	return (uint32_t) p_cp_param->use_ll;
}

uint32_t dovi_set_dovi2hdr10_mapping(int dovi2hdr10_mapping)
{
	dovi_func();

	p_cp_param->dovi2hdr10_mapping = dovi2hdr10_mapping;

	dovi_info("set dovi2hdr10_mapping %d\n",
		dovi2hdr10_mapping);

	return DOVI_RET_OK;
}

uint32_t dovi_set_vsvdb_file_name(char *vsvdb_file_name)
{
	int ret = 0;

	dovi_func();

	memset(p_cp_param->vsvdb_file, 0, MAX_FILENAME_LENGTH);
	ret = sprintf(p_cp_param->vsvdb_file, "/sdcard/vsvdb/%s",
		vsvdb_file_name);
	if (ret < 0) {
		dovi_error("dovi set vsvdb file fail\n");
		return DOVI_RET_ERROR;
	}
	disp_dovi_load_buffer(p_cp_param->vsvdb_file,
			      (unsigned char *)p_cp_param->vsvdb_hdmi,
			      0x1A);
	dovi_info("set vsvdb_file_name %s\n", vsvdb_file_name);

	return DOVI_RET_OK;
}

uint32_t dovi_set_vsvdb_hdmi(char *vsvdb_edid, int len)
{
	uint32_t i = 0;

	dovi_func();

	if ((vsvdb_edid != NULL) && (len <= 0x1A)) {
		memset(p_cp_param->vsvdb_hdmi, 0, 0x1A);
		memcpy((void *)p_cp_param->vsvdb_hdmi,
			(void *)vsvdb_edid, len);
	}
	dovi_vsvdb("vsvdb info below:\n");
	for (i = 0; i < 0x1A; i++)
		dovi_vsvdb("0x%x ", p_cp_param->vsvdb_hdmi[i]);
	return DOVI_RET_OK;
}

bool is_atsc_rpu(uint8_t *src_rpu)
{
	uint8_t atsc_code[7] = {0xB5, 0x00, 0x31, 0x47, 0x41, 0x39, 0x34};
	uint32_t idx = 0;
	bool ret = false;

	if (src_rpu == NULL)
		return ret;

	if ((src_rpu[idx] == atsc_code[0])
		&& (src_rpu[idx + 1] == atsc_code[1])
		&& (src_rpu[idx + 2] == atsc_code[2])
		&& (src_rpu[idx + 3] == atsc_code[3])
		&& (src_rpu[idx + 4] == atsc_code[4])
		&& (src_rpu[idx + 5] == atsc_code[5])
		&& (src_rpu[idx + 6] == atsc_code[6]))
		ret = true;

	return ret;
}

bool is_dvb_rpu(uint8_t *src_rpu)
{
	uint8_t dvb_code[7] = {0xB5, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x00};
	uint32_t idx = 0;
	bool ret = false;

	if (src_rpu == NULL)
		return ret;

	if ((src_rpu[idx] == dvb_code[0])
		&& (src_rpu[idx + 1] == dvb_code[1])
		&& (src_rpu[idx + 2] == dvb_code[2])
		&& (src_rpu[idx + 3] == dvb_code[3])
		&& (src_rpu[idx + 4] == dvb_code[4])
		&& (src_rpu[idx + 5] == dvb_code[5])
		&& (src_rpu[idx + 6] == dvb_code[6]))
		ret = true;

	return ret;
}

enum DV_RPU_TYPE dovi_parse_rpu_av1(uint8_t *src_rpu,
	uint32_t src_rpu_len,
	uint8_t *dst_rpu, uint32_t *dst_rpu_len)
{
	enum DV_RPU_TYPE ret = HEVC_RPU;
	uint8_t itu35_code[7] = {0xB5, 0x00, 0x3b, 0x00, 0x00, 0x08, 0x00};
	//uint8_t atsc_code[7] = {0xB5, 0x00, 0x31, 0x47, 0x41, 0x39, 0x34};
	//uint8_t dvb_code[7] = {0xB5, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x00};
	uint8_t cm_dm_code = 0x08;
	uint8_t dm_dm_code = 0x09;
	uint32_t idx = 0;

	if (src_rpu == NULL || dst_rpu == NULL
		|| dst_rpu_len == NULL) {
		dovi_info("%s params error\n", __func__);
		ret = HEVC_RPU;
		return ret;
	}

	if (is_atsc_rpu(src_rpu)) {
		if ((src_rpu[idx + 7] == cm_dm_code)
			|| (src_rpu[idx + 7] == dm_dm_code)) {
			*dst_rpu_len = src_rpu_len;
			memcpy((void *)dst_rpu, (void *)(src_rpu),
			*dst_rpu_len);
			dovi_rpu("atsc rpu len %d\n", *dst_rpu_len);
			ret = ATSC_RPU;
			return ret;
		}
	}

	if (is_dvb_rpu(src_rpu)) {
		if ((src_rpu[idx + 7] == cm_dm_code)
			|| (src_rpu[idx + 7] == dm_dm_code)) {
			*dst_rpu_len = src_rpu_len;
			memcpy((void *)dst_rpu, (void *)(src_rpu),
			*dst_rpu_len);
			dovi_rpu("dvb rpu %d\n", *dst_rpu_len);
			ret = DVB_RPU;
			return ret;
		}
	}

	if (!((src_rpu[idx] == itu35_code[0])
		&& (src_rpu[idx + 1] == itu35_code[1])
		&& (src_rpu[idx + 2] == itu35_code[2])
		&& (src_rpu[idx + 3] == itu35_code[3])
		&& (src_rpu[idx + 4] == itu35_code[4])
		&& (src_rpu[idx + 5] == itu35_code[5])
		&& (src_rpu[idx + 6] == itu35_code[6]))) {
		dovi_rpu("itu_35 code no match\n");
		ret = HEVC_RPU;
		return ret;
	}

	idx += 7;
	*dst_rpu_len = src_rpu_len - 7;

	memcpy((void *)dst_rpu, (void *)(src_rpu + idx), *dst_rpu_len);

	dovi_rpu("%s rpu len %d,first 0x%p\n", __func__,
		*dst_rpu_len,
		(uint32_t *)dst_rpu);

	ret = AV1_RPU;
	return ret;
}

int dovi_remove_rpu_nal_type(uint32_t first_frame,
			     uint8_t *src_rpu,
			     uint8_t *dst_rpu,
			     uint32_t len)
{
	uint32_t idx = 0;
	uint32_t dst_len = 0;
	uint32_t rpu_len = 0;
	uint32_t nal_len = START_CODE_LEN_MAX;
	uint32_t start_idx = nal_len;
	unsigned char start_code[START_CODE_LEN_MAX] = {
		0x00, 0x00, 0x00, 0x01, 0x19 };

	/* remove start code and find out the nal type start idx */
	for (idx = 0; idx < len; idx++) {
		if ((idx >= 3) &&
		    (src_rpu[idx - 3] == 0x0) &&
		    (src_rpu[idx - 2] == 0x0) &&
		    (src_rpu[idx - 1] == 0x1)) {
			break;
		}
	}

	if (idx >= len) {
		dovi_error("can not find out start code\n");
		return -1;
	}

	/* keep the start code for the first frame */
	if (first_frame) {
		start_idx = nal_len;
		memcpy((VOID *) (dst_rpu), (VOID *) (start_code), nal_len);
	} else
		start_idx = 0;

	dst_len += start_idx;

	/* copy the real rpu to dst bufffer */
	rpu_len = (len - idx - NAL_TYPE_LEN_MAX);
	memcpy((VOID *) (dst_rpu + start_idx),
		(VOID *) (src_rpu + idx + NAL_TYPE_LEN_MAX),
	    rpu_len);

	/* copy the start code to the end of the rpu for md parser */
	dst_len += rpu_len;
	memcpy((VOID *) (dst_rpu + dst_len),
		(VOID *) (start_code), nal_len);

	dst_len += nal_len;

	return dst_len;
}

int disp_dovi_common_init(void)
{
	struct dovi_share_memory_info_t *p_share_mem =
		dovi_share_mem;

	/*idk2.6 new add */
	p_src_param = p_share_mem->src_param;

	p_cp_param = &p_share_mem->cp_param;
	p_sec_layer = &p_share_mem->sec_layer;
	p_vsif = &p_share_mem->vsif;
	p_hdmi_md = p_share_mem->hdmi_md;
	p_hdr10_md = &p_share_mem->hdr10_info_frame;
	p_dv_out_params = &p_share_mem->dv_out_params;
	p_profile4 = &p_share_mem->cp_param.profile;

#if DOVI_TZ_OK
#else
	disp_dovi_common_test(2);
#endif
	mutex_init(&disp_dovi_mutex);
	idk_dump_vsync_cnt = -1;
	return DOVI_RET_OK;
}

int disp_dovi_common_test(uint32_t option)
{
	return DOVI_RET_OK;
}

int disp_dovi_md(void)
{
	if (dump_md_enable) {
		if (p_src_param[LAYER0].input_format == SIGNAL_FORMAT_DOVI) {
			disp_dovi_dump_comp(p_src_param[LAYER0].src_frame_num,
				(unsigned char *)p_src_param[LAYER0].comp_md,
				DOVI_COMP_SIZE);
			disp_dovi_dump_orig_md(p_src_param[LAYER0].src_frame_num,
				(unsigned char *)p_src_param[LAYER0].orig_md,
				p_src_param[LAYER0].orig_md_len);
		}

		disp_dovi_dump_hw_setting(1, p_src_param[LAYER0].src_frame_num,
		  (unsigned char *)p_dv_out_params,
		  sizeof(struct dv_hw_reg));
	}
	return DOVI_RET_OK;
}

int disp_dovi_set_hdr10_param(struct mtk_disp_hdr10_md_t *p_hdr10_param,
			      struct mtk_disp_hdr_md_info_t *hdr_metadata)
{
	struct mtk_disp_hdr10_md_t *p_hdr10_metadata =
		&hdr_metadata->metadata_info.hdr10_metadata;

	if ((p_hdr10_param != NULL) && (p_hdr10_metadata != NULL)) {
		memcpy((void *)p_hdr10_param,
		       (void *)p_hdr10_metadata,
		       sizeof(struct mtk_disp_hdr10_md_t));
	}

	return DOVI_RET_OK;
}

int disp_dovi_update_hdr10_info_frame(
	struct mtk_disp_hdr10_md_t *p_hdr10_info_frame,
	struct mtk_disp_hdr_md_info_t *hdr_metadata)
{
	struct VID_STATIC_HDMI_MD_T *p_hdr10_metadata =
	    &hdr_metadata->hdr10_info.metadata_info.hdr10_metadata;

	hdr_metadata->hdr10_info.e_DynamicRangeType =
		VID_PLA_DR_TYPE_HDR10;
	hdr_metadata->hdr10_info.fgIsMetadata = true;

	p_hdr10_metadata->ui2_DisplayPrimariesX[0] =
		p_hdr10_info_frame->ui2_DisplayPrimariesX[0];
	p_hdr10_metadata->ui2_DisplayPrimariesX[1] =
		p_hdr10_info_frame->ui2_DisplayPrimariesX[1];
	p_hdr10_metadata->ui2_DisplayPrimariesX[2] =
		p_hdr10_info_frame->ui2_DisplayPrimariesX[2];
	p_hdr10_metadata->ui2_DisplayPrimariesY[0] =
		p_hdr10_info_frame->ui2_DisplayPrimariesY[0];
	p_hdr10_metadata->ui2_DisplayPrimariesY[1] =
		p_hdr10_info_frame->ui2_DisplayPrimariesY[1];
	p_hdr10_metadata->ui2_DisplayPrimariesY[2] =
		p_hdr10_info_frame->ui2_DisplayPrimariesY[2];
	p_hdr10_metadata->ui2_WhitePointX =
		p_hdr10_info_frame->ui2_WhitePointX;
	p_hdr10_metadata->ui2_WhitePointY =
		p_hdr10_info_frame->ui2_WhitePointY;
	p_hdr10_metadata->ui2_MaxDisplayMasteringLuminance =
	    p_hdr10_info_frame->ui2_MaxDisplayMasteringLuminance;
	p_hdr10_metadata->ui2_MinDisplayMasteringLuminance =
	    p_hdr10_info_frame->ui2_MinDisplayMasteringLuminance;
	p_hdr10_metadata->ui2_MaxCLL =
		p_hdr10_info_frame->ui2_MaxCLL;
	p_hdr10_metadata->ui2_MaxFALL =
		p_hdr10_info_frame->ui2_MaxFALL;
	p_hdr10_metadata->fgNeedUpdStaticMeta = 1;
	if (dovi_hdmi_brightness_en)
		p_hdr10_metadata->ui2_MaxDisplayMasteringLuminance =
		dovi_hdmi_brightness;

	dovi_info("p_hdr10_metadata ui2_DisplayPrimariesX[%d %d %d]\n",
		  p_hdr10_metadata->ui2_DisplayPrimariesX[0],
		  p_hdr10_metadata->ui2_DisplayPrimariesX[1],
		  p_hdr10_metadata->ui2_DisplayPrimariesX[2]);
	dovi_info("p_hdr10_metadata ui2_DisplayPrimariesY[%d %d %d]\n",
		  p_hdr10_metadata->ui2_DisplayPrimariesY[0],
		  p_hdr10_metadata->ui2_DisplayPrimariesY[1],
		  p_hdr10_metadata->ui2_DisplayPrimariesY[2]);
	dovi_info("p_hdr10_metadata ui2_WhitePointX Y[%d %d]\n",
		  p_hdr10_metadata->ui2_WhitePointX,
		  p_hdr10_metadata->ui2_WhitePointY);
	dovi_info("p_hdr10_metadata max min[%d %d]\n",
		  p_hdr10_metadata->ui2_MaxDisplayMasteringLuminance,
		  p_hdr10_metadata->ui2_MinDisplayMasteringLuminance);
	dovi_info("p_hdr10_metadata ui2_MaxCLL ui2_MaxFALL[%d %d]\n",
		  p_hdr10_metadata->ui2_MaxCLL, p_hdr10_metadata->ui2_MaxFALL);
	return DOVI_RET_OK;
}

void disp_dovi_force_output_setting(struct disp_hw_common_info *info)
{
	char *vsvdb_file_name = "vsvdb_push.bin";
	struct disp_hw_tv_capbility *tv_cap = &info->tv;
	//struct disp_hw *hdr_drv = disp_hdr_get_drv();
	if (!dovi_force_output) {
		DISP_LOG_E("dovi_force_output is false!\n");
		return;
	}

	dovi_out_info.b_gfx_mode = false;
	dovi_out_info.is_low_latency = false;
	dovi_out_info.is_vsem = false;
	if (set_vsvdb) {
		dovi_set_vsvdb_file_name(vsvdb_file_name);
		dovi_out_info.vsvdb_edid = p_cp_param->vsvdb_hdmi;
	} else
		dovi_out_info.vsvdb_edid = tv_cap->vsvdb_edid;

	if ((dovi_force_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY)
		|| (dovi_force_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
		dovi_out_info.is_low_latency = true;
	else
		dovi_out_info.is_low_latency = false;

	if ((dovi_force_out_format == DOVI_FORMAT_VSEM_DOVI)
		|| (dovi_force_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
		dovi_out_info.is_vsem = true;
	else
		dovi_out_info.is_vsem = false;

	dovi_out_format = dovi_force_out_format;

	if (dovi_out_info.out_format != dovi_out_format)
		dovi_out_format_change = true;

	dovi_out_info.out_format = dovi_out_format;

	if (!dovi_idk_dump) {
		dovi_set_output_format(dovi_out_info.out_format);
		dovi_set_low_latency_mode(dovi_out_info.is_low_latency,
			ll_format);
		dovi_set_vsem_mode(dovi_out_info.is_vsem);
		dovi_set_priority_mode(!dovi_out_info.b_gfx_mode);
		dovi_update_graphic_info();
		dovi_set_vsvdb_hdmi(dovi_out_info.vsvdb_edid, 0x1A);
	} else {
		dovi_out_format = dovi_get_output_format();
		if (dovi_get_low_latency_mode() == 1)
			dovi_out_format = DOVI_FORMAT_DOVI_LOW_LATENCY;
		DISP_LOG_N("idk dump output %d!\n", dovi_out_format);
	}

	//set_hdmi_info(dovi_out_format, tv_cap, true);
	disp_hdr_fe_start_stop(0, true);
	//disp_path_set_hw_path(DISP_PATH_M_HDR_VDO_FE, 1);
}

void set_hdmi_info(enum dovi_signal_format_t output_format,
		   struct disp_hw_tv_capbility *tv_cap, bool enable)
{

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
	if (output_format == DOVI_FORMAT_DOVI)
		vDoviHdrEnable(enable);
	else if (output_format == DOVI_FORMAT_HDR10) {
		vSetStaticHdrType(GAMMA_ST2084);
		vHdrEnable(enable);
		if (tv_cap->is_support_bt2020)
			hdr_set_HDMI_BT2020_signal(enable);
	} else if (output_format == DOVI_FORMAT_DOVI_LOW_LATENCY)
		vLowLatencyDoviEnable(enable);
	else if (output_format == DOVI_FORMAT_HLG) {
		vSetStaticHdrType(GAMMA_HLG);
		vHdrEnable(enable);
		if (tv_cap->is_support_bt2020)
			hdr_set_HDMI_BT2020_signal(enable);
	}
#endif
}

void dovi_update_output_setting(struct disp_hw_common_info *info,
			     struct video_buffer_info *buf_main,
			     struct video_buffer_info *buf_sub)
{
	struct disp_hw_tv_capbility *tv_cap = NULL;
	const struct disp_hw_resolution *resolution = NULL;
	bool is_2160p60_out = false;
	enum dovi_signal_format_t original_dovi_out_format = 0;

	if (info == NULL) {
		dovi_error("%s info is null\n", __func__);
		return;
	}

	resolution = info->resolution;
	is_2160p60_out = dovi_check_4k60_timing(resolution->res_mode);
	original_dovi_out_format = dovi_out_format;

	tv_cap = &info->tv;

	dovi_info("%s %d\n", __func__, dovi_out_format);
	dovi_info("support %d %d %d %d 4k60:%d,ll:%d,4kout:%d,ver:%d %d %d %d\n",
		tv_cap->is_support_hdr,
		tv_cap->is_support_hlg,
		tv_cap->is_support_hdr10_plus,
		tv_cap->is_support_dovi,
		tv_cap->is_support_dovi_2160p60,
		tv_cap->is_support_dovi_low_latency,
		is_2160p60_out,
		tv_cap->dovi_vsvdb_version,
		tv_cap->dovi_vsvdb_v2_interface,
		tv_cap->dovi_vsvdb_dm_version,
		tv_cap->is_support_bt2020);

	if (buf_main != NULL) {
		dovi_out_info.b_gfx_mode =
			buf_main->is_dovi_graphic_mode;
		dovi_out_info.is_low_latency =
			buf_main->is_dovi_low_latency;
	} else {
		dovi_out_info.b_gfx_mode = false;
		dovi_out_info.is_low_latency = false;
	}

	dovi_out_info.vsvdb_edid = tv_cap->vsvdb_edid;

	/* get output format depend on tv capability */
	dovi_out_format = get_tv_output_format(tv_cap,
	is_2160p60_out, resolution);

	if ((dovi_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY) ||
		(dovi_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
		dovi_out_info.is_low_latency = true;
	else
		dovi_out_info.is_low_latency = false;

	if ((dovi_out_format == DOVI_FORMAT_VSEM_DOVI) ||
		(dovi_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
		dovi_out_info.is_vsem = true;
	else
		dovi_out_info.is_vsem = false;

	/*if ui select force sdr ,always keep sdr output*/
	if (ui_force_hdr_type == DYNA_SET_FORCE_SDR)
		dovi_out_format = DOVI_FORMAT_SDR;

	if (dovi_force_output) {
		if ((dovi_force_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY) ||
			(dovi_force_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
			dovi_out_info.is_low_latency = true;
		else
			dovi_out_info.is_low_latency = false;

		if ((dovi_force_out_format == DOVI_FORMAT_VSEM_DOVI) ||
			(dovi_force_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
			dovi_out_info.is_vsem = true;
		else
			dovi_out_info.is_vsem = false;

		dovi_out_format = dovi_force_out_format;
		dovi_info("force outformat %d\n", dovi_out_format);
	}

	if (original_dovi_out_format != dovi_out_format) {
		dovi_out_format_change = true;
		dovi_default("%s, out format change %d->%d\n",
			   __func__, original_dovi_out_format,
			   dovi_out_format);
	}

	dovi_out_info.out_format = dovi_out_format;

	if (osd_enable)
		dovi_out_info.b_gfx_mode = true;
	else
		dovi_out_info.b_gfx_mode = false;

	if (!dovi_idk_dump) {
		dovi_set_output_format(dovi_out_info.out_format);
		dovi_set_low_latency_mode(dovi_out_info.is_low_latency,
			ll_format);
		dovi_set_vsem_mode(dovi_out_info.is_vsem);
		dovi_set_priority_mode(!dovi_out_info.b_gfx_mode);
		dovi_update_graphic_info();
		dovi_set_vsvdb_hdmi(dovi_out_info.vsvdb_edid, 0x1A);
	} else {
		dovi_out_format = dovi_get_output_format();
		if (dovi_get_low_latency_mode() == 1)
			dovi_out_format = DOVI_FORMAT_DOVI_LOW_LATENCY;
		DISP_LOG_I("idk dump output %d!\n", dovi_out_format);
	}
}

void dovi_path_enable(void)
{
	//struct disp_hw *hdr_drv = disp_hdr_get_drv();
	#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
	struct VID_PLA_HDR_METADATA_INFO_T rHdr = { 0 };
	struct VID_STATIC_HDMI_MD_T *hdr10_md = NULL;
	#endif

	if (osd_enable == 0) {
		dovi_default("force enable ui for dovi path enable!\n");
		osd_enable = 1;
	}

	dovi_default("force hdr with video[%d %d %d]\n",
		g_force_dovi,
		vdp_start_st[LAYER0],
		vdp_start_st[LAYER1]);

	if (!g_force_dovi && !dovi_idk_test) {
		dovi_black_pattern_en = true;
		dovi_black_pattern_cnt = 0;
		dovi_black_pattern_cnt_max = 5;
		if (dovi_black_en_bycmd)
			dovi_black_pattern_cnt_max = dovi_black_cnt_bycmd;
		if (dovi_black_pattern_cnt_max > 0)
			disp_mix_hal_set_black_pattern(true);
	}

	if (!g_force_dovi) {
		if (vdp_start_st[LAYER0] == 1)
			dovi_vdo_fe_hal_set_enable(LAYER0, true);
		if (vdp_start_st[LAYER1] == 1)
			dovi_vdo_fe_hal_set_enable(LAYER1, true);
		dovi_gfx_fe_hal_set_enable(LAYER0, true);
		dovi_gfx_fe_hal_set_enable(LAYER1, true);
		dovi_be_hal_set_enable(true);
	}

	if (!tv_info_set_by_cmd)
		disp_hw_mgr_get_info(&hdr_common_info);

	dovi_set_video_input_format(SIGNAL_FORMAT_SDR8);
	dovi_update_output_setting(&hdr_common_info, NULL, NULL);

	if ((video_layer[0].state == VDP_LAYER_IDLE)
	    || (video_layer[0].state == VDP_LAYER_STOPPING)
	    || (video_layer[0].state == VDP_LAYER_STOPPED))
		dovi_vdo_fe_hal_set_out_fix_pattern_enable(true,
		0x04, 0x200, 0x200);

	if ((dovi_out_info.out_format == DOVI_FORMAT_DOVI)
		|| (dovi_out_info.out_format == DOVI_FORMAT_VSEM_DOVI)) {
		dovi_out_info.b_gfx_mode = true;
		dovi_set_priority_mode(!dovi_out_info.b_gfx_mode);
		priority_mode_change = true;
	}

	dovi_info("%s not mix graphic %d %d\n", __func__,
		   dovi_hdr_md_info[0].enable, dovi_hdr_md_info[0].dr_range);

	switch (dovi_out_format) {
	case DOVI_FORMAT_DOVI:
		disp_adl_update_uhd_lut(true, dovi_sdr_ipt_gfx_fe_lut);
		break;
	case DOVI_FORMAT_VSEM_DOVI:
		disp_adl_update_uhd_lut(true, dovi_sdr_vsem_gfx_fe_lut);
		break;
	case DOVI_FORMAT_DOVI_LOW_LATENCY:
		disp_adl_update_uhd_lut(true, dovi_sdr_ll_gfx_fe_lut);
		break;
	case DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY:
		disp_adl_update_uhd_lut(true, dovi_sdr_vsemll_gfx_fe_lut);
		break;
	default:
		break;
	}

	if (!g_force_dovi) {
		dovi_hdr_md_info[0].enable = DOVI_INOUT_FORMAT_CHANGE;
		dovi_hdr_md_info[0].dr_range = DISP_DR_TYPE_SDR;
		dovi_hdr_md_info[1].enable = DOVI_INOUT_FORMAT_CHANGE;
		dovi_hdr_md_info[1].dr_range = DISP_DR_TYPE_SDR;
		disp_dovi_process_cmd(LAYER0, DISP_CMD_METADATA_UPDATE,
			&dovi_hdr_md_info[0]);
	} else {
		dovi_hdr_md_info[0].enable = DOVI_INOUT_FORMAT_CHANGE;
		dovi_hdr_md_info[0].dr_range = DISP_DR_TYPE_SDR;
		dovi_hdr_md_info[1].enable = DOVI_INOUT_FORMAT_CHANGE;
		dovi_hdr_md_info[1].dr_range = DISP_DR_TYPE_SDR;
		dovi_info("dobly path enable process!\n");
		disp_dovi_process(1, &dovi_hdr_md_info[0]);
	}

	dovi_path_en = 1;
	dovi_vs10_path_en = ui_force_hdr_type;


	if (!g_force_dovi) {
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
		if ((dovi_path_en)
			&& ((dovi_out_format == DOVI_FORMAT_HDR10)
			|| (dovi_out_format == DOVI_FORMAT_HLG))) {
			memcpy(&rHdr, &dovi_hdr_md_info[0].hdr10_info,
				sizeof(struct VID_PLA_HDR_METADATA_INFO_T));
			rHdr.e_DynamicRangeType = VID_PLA_DR_TYPE_HDR10;
			rHdr.fgIsMetadata = true;
			hdr10_md = &rHdr.metadata_info.hdr10_metadata;
			hdr10_md->fgNeedUpdStaticMeta = true;
			hdr10_md->ui2_DisplayPrimariesX[0] =
				35400;
			hdr10_md->ui2_DisplayPrimariesX[1] =
				8500;
			hdr10_md->ui2_DisplayPrimariesX[2] =
				6550;
			hdr10_md->ui2_DisplayPrimariesY[0] =
				14600;
			hdr10_md->ui2_DisplayPrimariesY[1] =
				39850;
			hdr10_md->ui2_DisplayPrimariesY[2] =
				2300;
			hdr10_md->ui2_WhitePointX = 15635;
			hdr10_md->ui2_WhitePointY = 16450;
			hdr10_md->ui2_MaxCLL = 0;
			hdr10_md->ui2_MaxFALL = 0;
			hdr10_md->ui2_MaxDisplayMasteringLuminance =
				1000;
			hdr10_md->ui2_MinDisplayMasteringLuminance =
				50;
			memcpy(&dovi_hdr_md_info[0].hdr10_info, &rHdr,
				sizeof(struct VID_PLA_HDR_METADATA_INFO_T));
		}
#endif
	}
	g_force_dovi = 0;
}

void dovi_path_disable(void)
{
	//struct disp_hw *hdr_drv = disp_hdr_get_drv();

	if (dovi_vs10_path_en || dovi_path_en) {
		dovi_path_en = 0;
		dovi_vs10_path_en = 0;
		g_force_dovi = 0;
		dovi_path_ready2start = 1;
		if (!dovi_idk_test) {
			dovi_black_pattern_en = true;
			dovi_black_pattern_cnt = 0;
			dovi_black_pattern_cnt_max = 5;
			if (dovi_black_en_bycmd)
				dovi_black_pattern_cnt_max = dovi_black_cnt_bycmd;
			if (dovi_black_pattern_cnt_max > 0)
				disp_mix_hal_set_black_pattern(true);
		}

		dovi_default("set dobly path disable\n");
		dovi_vdo_fe_hal_set_out_fix_pattern_enable(false,
			0x0, 0x0, 0x0);

		dovi_hdr_md_info[0].dr_range = DISP_DR_TYPE_PHLP_RESVERD;
		dovi_hdr_md_info[0].enable = 0;
		dovi_hdr_md_info[1].dr_range = DISP_DR_TYPE_PHLP_RESVERD;
		dovi_hdr_md_info[1].enable = 0;
		disp_dovi_process_cmd(LAYER0, DISP_CMD_METADATA_UPDATE,
			&dovi_hdr_md_info[0]);

	} else if (g_force_dovi) {
		dovi_info("%s for g_force_dovi true, g_hdr_type %u\n",
			__func__, g_hdr_type);
		dovi_vdo_fe_hal_set_enable(0, false);
		dovi_vdo_fe_hal_set_enable(1, false);
		dovi_gfx_fe_hal_set_enable(0, false);
		dovi_gfx_fe_hal_set_enable(1, false);
		dovi_be_hal_set_enable(false);
		dovi_path_en = 0;
		g_force_dovi = 0;
		g_hdr_type = 0;
		g_out_format = 2;
	} else {
		dovi_info("dobly path is not enable!\n");
		return;
	}
}

void dovi_get_hdmi_output_format(uint32_t *out_format,
	bool *bt2020_enable)
{
	if (out_format == NULL) {
		dovi_error("%s error params\n", __func__);
		return;
	}
	switch (dovi_out_format) {
	case DOVI_FORMAT_DOVI:
		*out_format = HDR_OUT_TYPE_DV_STD;
		*bt2020_enable = false; /* does not matter */
		break;
	case DOVI_FORMAT_VSEM_DOVI:
		*out_format = HDR_OUT_TYPE_VSEM_DV_STD;
		*bt2020_enable = true;
		break;
	case DOVI_FORMAT_HDR10:
		*out_format = HDR_OUT_TYPE_HDR10;
		*bt2020_enable = true;
		break;
	case DOVI_FORMAT_SDR:
		*out_format = HDR_OUT_TYPE_SDR;
		*bt2020_enable = false;
		break;
	case DOVI_FORMAT_DOVI_LOW_LATENCY:
		*out_format = HDR_OUT_TYPE_DV_LL;
		*bt2020_enable = true;
		break;
	case DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY:
		*out_format = HDR_OUT_TYPE_VSEM_DV_LL;
		*bt2020_enable = true;
		break;
	case DOVI_FORMAT_HLG:
		*out_format = HDR_OUT_TYPE_HLG;
		*bt2020_enable = true;
		break;
	case DOVI_FORMAT_SDR_2020:
		*out_format = HDR_OUT_TYPE_SDR_2020;
		*bt2020_enable = true;
		break;
	default:
		dovi_error("dovi out format error\n");
		break;
	}
}

int dovi_get_hdr10_metadata(struct VID_PLA_HDR_METADATA_INFO_T *rHdr,
	uint32_t id)
{
	if (rHdr == NULL) {
		dovi_info("get hdr10 md failed\n");
		return 1;
	}

	memcpy(rHdr, &dovi_hdr_md_info[id].hdr10_info,
		sizeof(struct VID_PLA_HDR_METADATA_INFO_T));
	return 0;
}
void set_hdr10_metadata(struct VID_PLA_HDR_METADATA_INFO_T rHdr,
			       struct mtk_disp_hdr_md_info_t dovi_hdr_md_info[],
			       uint32_t id)
{
	memcpy(&rHdr, &dovi_hdr_md_info[id].hdr10_info,
		sizeof(struct VID_PLA_HDR_METADATA_INFO_T));
	vVdpSetHdrMetadata(true, rHdr);
}

bool dovi_check_4k60_timing(enum HDMI_VIDEO_RESOLUTION res_mode)
{
	if ((res_mode == HDMI_VIDEO_3840x2160P_60HZ) ||
	    (res_mode == HDMI_VIDEO_3840x2160P_50HZ) ||
	    (res_mode == HDMI_VIDEO_4096x2160P_60HZ) ||
	    (res_mode == HDMI_VIDEO_4096x2160P_50HZ))
		return true;
	else
		return false;
}

uint32_t dovi_update_res_change(
	struct disp_hw_tv_capbility *tv_cap,
	const struct disp_hw_resolution *resolution)
{
	bool is_4k60_out = dovi_check_4k60_timing(resolution->res_mode);
	enum dovi_signal_format_t dovi_out_format_new;
	uint32_t res_real_change = 0;

	dovi_out_info.vsvdb_edid = tv_cap->vsvdb_edid;

	/* get output format depend on tv capability */
	dovi_out_format_new =
	get_tv_output_format(tv_cap, is_4k60_out, resolution);

	/*if ui select force sdr ,always keep sdr output*/
	if (ui_force_hdr_type == DYNA_SET_FORCE_SDR)
		dovi_out_format_new = DOVI_FORMAT_SDR;

	if (dovi_force_output) {
		switch (dovi_force_out_format) {
		case DOVI_FORMAT_DOVI_LOW_LATENCY:
			dovi_out_info.is_low_latency = true;
			dovi_out_info.is_vsem = false;
			break;
		case DOVI_FORMAT_VSEM_DOVI:
			dovi_out_info.is_low_latency = false;
			dovi_out_info.is_vsem = true;
			break;
		case DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY:
			dovi_out_info.is_low_latency = true;
			dovi_out_info.is_vsem = true;
			break;
		default:
			dovi_out_info.is_low_latency = false;
			dovi_out_info.is_vsem = false;
			break;
		}
		dovi_out_format_new = dovi_force_out_format;
		dovi_info("force outformat %d\n", dovi_out_format_new);
	}

	dovi_out_info.out_format = dovi_out_format_new;

	if (osd_enable)
		dovi_out_info.b_gfx_mode = true;
	else
		dovi_out_info.b_gfx_mode = false;

	if ((dovi_out_info.out_format == DOVI_FORMAT_DOVI_LOW_LATENCY)
		|| (dovi_out_info.out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
		dovi_out_info.is_low_latency = true;
	else
		dovi_out_info.is_low_latency = false;

	if ((dovi_out_info.out_format == DOVI_FORMAT_VSEM_DOVI) ||
		(dovi_out_info.out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY))
		dovi_out_info.is_vsem = true;
	else
		dovi_out_info.is_vsem = false;

	if (dovi_out_format_new == dovi_out_format) {
		dovi_info("out_format is same as before %d\n",
			dovi_out_format);

		if ((dovi_out_format == DOVI_FORMAT_HDR10)
		    || (dovi_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY)
		    || (dovi_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY)
		    || (dovi_out_format == DOVI_FORMAT_HLG)) {
			dovi_set_output_format(dovi_out_info.out_format);
			dovi_set_low_latency_mode(dovi_out_info.is_low_latency,
				ll_format);
			dovi_set_vsem_mode(dovi_out_info.is_vsem);
			dovi_set_priority_mode(!dovi_out_info.b_gfx_mode);
			dovi_update_graphic_info();
			dovi_set_vsvdb_hdmi(dovi_out_info.vsvdb_edid, 0x1A);
			res_real_change = 1;
		}
	}
	if ((old_resolution != resolution->res_mode)
	    || (dovi_out_format_new != dovi_out_format)) {
		dovi_default("hdr old_res=%d, new_res=%d %d %d\n",
			   old_resolution, resolution->res_mode,
			   dovi_out_format, dovi_out_format_new);

		dovi_set_output_format(dovi_out_info.out_format);
		dovi_set_low_latency_mode(dovi_out_info.is_low_latency,
			ll_format);
		dovi_set_vsem_mode(dovi_out_info.is_vsem);
		dovi_set_priority_mode(!dovi_out_info.b_gfx_mode);
		dovi_update_graphic_info();
		dovi_set_vsvdb_hdmi(dovi_out_info.vsvdb_edid, 0x1A);

		switch (dovi_out_format_new) {
		case DOVI_FORMAT_DOVI:
			disp_adl_update_uhd_lut(true, dovi_sdr_ipt_gfx_fe_lut);
			break;
		case DOVI_FORMAT_VSEM_DOVI:
			disp_adl_update_uhd_lut(true, dovi_sdr_vsem_gfx_fe_lut);
			break;
		case DOVI_FORMAT_DOVI_LOW_LATENCY:
			disp_adl_update_uhd_lut(true, dovi_sdr_ll_gfx_fe_lut);
			break;
		case DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY:
			disp_adl_update_uhd_lut(true, dovi_sdr_vsemll_gfx_fe_lut);
			break;
		default:
			break;
		}

		dovi_hdr_md_info[LAYER0].enable =
		DOVI_RESOLUTION_CHANGE;

		res_real_change = 1;

		disp_dovi_process(1, &dovi_hdr_md_info[LAYER0]);
	}
	dovi_out_format = dovi_out_format_new;
	old_resolution = resolution->res_mode;

	return res_real_change;
}

int dovi_get_frame_info(uint32_t id, struct video_buffer_info *buf,
	bool force_update)
{
	struct mtk_disp_hdr10_md_t *dv_hdr10_md = NULL;
	struct VID_STATIC_HDMI_MD_T *buf_hdr10_md = NULL;
	unsigned short mix_lum = 0;
	unsigned short max_lum = 0;
	uint32_t enable = 0;
	int ret = 0;

	if (buf == NULL || id > LAYER1) {
		dovi_printf("%s error %d\n", __func__, id);
		return 0;
	}

	if ((buf->hdr_info.dr_range == DISP_DR_TYPE_DOVI) ||
		(dovi_hdr_md_info[id].dr_range != buf->hdr_info.dr_range)
		|| force_update) {
		if (dovi_hdr_md_info[id].dr_range != buf->hdr_info.dr_range) {
			enable = DOVI_INOUT_FORMAT_CHANGE;
			dovi_dbg("%s[%d],dr range change %d->%d %d\n",
			__func__, id, dovi_hdr_md_info[id].dr_range,
			buf->hdr_info.dr_range, force_update);
		} else {
			if (force_update)
				enable = DOVI_INOUT_FORMAT_CHANGE;
			else
				enable = 1;
			dovi_dbg("%s %d %d %d %lld\n", __func__,
			dovi_hdr_md_info[id].enable,
			dovi_hdr_md_info[id].dr_range,
			buf->hdr_info.enable,
			buf->pts);
		}

		dovi_hdr_md_info[id] = buf->hdr_info;
		dovi_hdr_md_info[id].enable = enable;

		dv_hdr10_md =
			&dovi_hdr_md_info[id].metadata_info.hdr10_metadata;
		buf_hdr10_md = &(buf->hdr10_info);
		mix_lum =
			buf_hdr10_md->ui2_MinDisplayMasteringLuminance;
		max_lum =
			buf_hdr10_md->ui2_MaxDisplayMasteringLuminance;

		/* atsc and dvb dovi need static metadata*/
		if ((buf->hdr_info.dr_range == DISP_DR_TYPE_DOVI) &&
			((buf->hdr10_type == HDR10_TYPE_ST2084) ||
			(buf->hdr10_type == HDR10_TYPE_HLG))) {
			dv_hdr10_md = &p_src_param[id].hdr10_md;
			dv_hdr10_md->ui2_DisplayPrimariesX[0]
				= buf_hdr10_md->ui2_DisplayPrimariesX[0];
			dv_hdr10_md->ui2_DisplayPrimariesX[1]
				= buf_hdr10_md->ui2_DisplayPrimariesX[1];
			dv_hdr10_md->ui2_DisplayPrimariesX[2]
				= buf_hdr10_md->ui2_DisplayPrimariesX[2];
			dv_hdr10_md->ui2_DisplayPrimariesY[0]
				= buf_hdr10_md->ui2_DisplayPrimariesY[0];
			dv_hdr10_md->ui2_DisplayPrimariesY[1]
				= buf_hdr10_md->ui2_DisplayPrimariesY[1];
			dv_hdr10_md->ui2_DisplayPrimariesY[2]
				= buf_hdr10_md->ui2_DisplayPrimariesY[2];
			dv_hdr10_md->ui2_WhitePointX
				= buf_hdr10_md->ui2_WhitePointX;
			dv_hdr10_md->ui2_WhitePointY
				= buf_hdr10_md->ui2_WhitePointY;
			dv_hdr10_md->ui2_MaxDisplayMasteringLuminance
				= max_lum;
			dv_hdr10_md->ui2_MinDisplayMasteringLuminance
				= mix_lum;
			dv_hdr10_md->ui2_MaxCLL
				= buf_hdr10_md->ui2_MaxCLL;
			dv_hdr10_md->ui2_MaxFALL
				= buf_hdr10_md->ui2_MaxFALL;
		} else if ((buf->hdr_info.dr_range == DISP_DR_TYPE_HDR10)
			|| (buf->hdr_info.dr_range == DISP_DR_TYPE_HLG)) {
			dv_hdr10_md->ui2_DisplayPrimariesX[0]
				= buf_hdr10_md->ui2_DisplayPrimariesX[0];
			dv_hdr10_md->ui2_DisplayPrimariesX[1]
				= buf_hdr10_md->ui2_DisplayPrimariesX[1];
			dv_hdr10_md->ui2_DisplayPrimariesX[2]
				= buf_hdr10_md->ui2_DisplayPrimariesX[2];
			dv_hdr10_md->ui2_DisplayPrimariesY[0]
				= buf_hdr10_md->ui2_DisplayPrimariesY[0];
			dv_hdr10_md->ui2_DisplayPrimariesY[1]
				= buf_hdr10_md->ui2_DisplayPrimariesY[1];
			dv_hdr10_md->ui2_DisplayPrimariesY[2]
				= buf_hdr10_md->ui2_DisplayPrimariesY[2];
			dv_hdr10_md->ui2_WhitePointX
				= buf_hdr10_md->ui2_WhitePointX;
			dv_hdr10_md->ui2_WhitePointY
				= buf_hdr10_md->ui2_WhitePointY;
			dv_hdr10_md->ui2_MaxDisplayMasteringLuminance
				= max_lum;
			dv_hdr10_md->ui2_MinDisplayMasteringLuminance
				= mix_lum;
			dv_hdr10_md->ui2_MaxCLL
				= buf_hdr10_md->ui2_MaxCLL;
			dv_hdr10_md->ui2_MaxFALL
				= buf_hdr10_md->ui2_MaxFALL;
		}
		ret = 1;
	}
	return ret;
}
int dovi_frame_commit(struct video_buffer_info *buf_main,
	struct video_buffer_info *buf_sub, bool sub_exist)
{
	bool need_update = false;
	uint32_t ret = 0;

	if ((sub_exist && (buf_sub == NULL)) ||
		(buf_main == NULL)) {
		dovi_error("%s buf is null\n", __func__);
		return -1;
	}

	/*sub hdr layer will disable here if sub video disable*/
	if (!sub_exist)
		dovi_vdo_fe_hal_set_enable(LAYER1, false);

	if (dovi_path_ready2start) {
		/*enable black pattern for adaptive first frame*/
		if (!dovi_path_en && !dovi_idk_test) {
			dovi_black_pattern_en = true;
			dovi_black_pattern_cnt = 0;
			dovi_black_pattern_cnt_max = 5;
			if (dovi_black_en_bycmd)
				dovi_black_pattern_cnt_max = dovi_black_cnt_bycmd;
			if (dovi_black_pattern_cnt_max > 0)
				disp_mix_hal_set_black_pattern(true);
		}
		/*enable dovi hal flag*/
		dovi_vdo_fe_hal_set_enable(LAYER0, true);
		dovi_be_hal_set_enable(true);
		if (sub_exist)
			dovi_vdo_fe_hal_set_enable(LAYER1, true);

		dovi_update_output_setting(&hdr_common_info,
			buf_main, buf_sub);

		dovi_path_en = 1;
		need_update = true;
		dovi_default("dv path en, pts %lld %d %d %d %d %d %d %d\n",
			buf_main->pts, buf_main->hdr_info.dr_range,
			dovi_hdr_md_info[LAYER0].dr_range,
			dovi_out_format, dovi_out_info.is_low_latency,
			dovi_out_info.b_gfx_mode,
			buf_main->hdr10_type, sub_exist);
		dovi_path_ready2start = 0;
	}

	if (dovi_force_output) {
		disp_dovi_force_output_setting(&hdr_common_info);
		dovi_force_output = false;
	}

	if (sub_exist)
		if (dovi_vdo_fe_hal_set_enable(LAYER1, true) == 1)
			need_update = true;

	if (dovi_out_format_change) {
		need_update = true;
		dovi_out_format_change = false;
	}

	/*get metadta info from main sub buffer*/
	if (dovi_path_en || need_update) {

		/*hdr10<-->hlg out change , base main only*/
		if ((dovi_hdr_md_info[LAYER0].dr_range
			!= buf_main->hdr_info.dr_range)
			&& (((dovi_out_format == DOVI_FORMAT_HDR10)
			&& (buf_main->hdr_info.dr_range == DISP_DR_TYPE_HLG))
			|| ((dovi_out_format == DOVI_FORMAT_HLG)
			&& (buf_main->hdr_info.dr_range != DISP_DR_TYPE_HLG))))
			dovi_update_output_setting(&hdr_common_info,
			buf_main, buf_sub);

		ret |= dovi_get_frame_info(LAYER0, buf_main, need_update);
		if (sub_exist)
			ret |= dovi_get_frame_info(LAYER1,
		buf_sub, need_update);

		/*need do dovi process*/
		if (ret > 0)
			disp_dovi_process_cmd(LAYER0, DISP_CMD_METADATA_UPDATE,
				&dovi_hdr_md_info[0]);

		need_update = false;
	}

	return 0;

}

int dovi_config_fefifo_swap(bool swap)
{
	struct disp_hw *hdr_drv = disp_hdr_get_drv();
	struct FEFIFO_SRC_ORDER_T fifo_order = { 0 };

	if (swap) {
		if (dv_gfx_fe_en[0]) {
			fifo_order.layer_id = 2;
			fifo_order.in_order = 5;
			fifo_order.out_order = 0;
			hdr_drv->drv_call(DISP_CMD_FEFIFO_SRC_SWAP,
				&fifo_order);
			dovi_info("set fefifo[2] swap %d\n", befifo_irq_cnt);
		} else {
			fifo_order.layer_id = 2;
			fifo_order.in_order = 0;
			fifo_order.out_order = 0;
			hdr_drv->drv_call(DISP_CMD_FEFIFO_SRC_SWAP,
				&fifo_order);
		}

		if (dv_gfx_fe_en[1]) {
			fifo_order.layer_id = 3;
			fifo_order.in_order = 5;
			fifo_order.out_order = 0;
			hdr_drv->drv_call(DISP_CMD_FEFIFO_SRC_SWAP,
				&fifo_order);
			dovi_info("set fefifo[3] swap %d\n", befifo_irq_cnt);
		} else {
			fifo_order.layer_id = 3;
			fifo_order.in_order = 0;
			fifo_order.out_order = 0;
			hdr_drv->drv_call(DISP_CMD_FEFIFO_SRC_SWAP,
				&fifo_order);
		}
	} else {
		fifo_order.layer_id = 2;
		fifo_order.in_order = 0;
		fifo_order.out_order = 0;
		hdr_drv->drv_call(DISP_CMD_FEFIFO_SRC_SWAP,
				&fifo_order);
		dovi_info("set fefifo[2] no swap %d\n", befifo_irq_cnt);

		fifo_order.layer_id = 3;
		fifo_order.in_order = 0;
		fifo_order.out_order = 0;
		hdr_drv->drv_call(DISP_CMD_FEFIFO_SRC_SWAP,
				&fifo_order);
		dovi_info("set fefifo[3] no swap %d\n", befifo_irq_cnt);
	}

	return 0;
}

int dovi_config_adl(void)
{
	struct adl_src_tbl adl_tbl;
	struct dv_hw_reg *p_reg = NULL;

	p_reg = p_dv_out_params;

	if (dv_vdo_fe_en[0]) {
		/*update mvdo lut*/
		memset(&adl_tbl, 0, sizeof(struct adl_src_tbl));

		adl_tbl.client = DV_ADL_V_MAIN;

		adl_tbl.hdr_b0103.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.g2l[0]);
		adl_tbl.hdr_b0103.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ss.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.smluts[0]);
		adl_tbl.hdr_b0202ss.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202si.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.smluti[0]);
		adl_tbl.hdr_b0202si.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ts.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.tmluts[0]);
		adl_tbl.hdr_b0202ts.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ti.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.tmluti[0]);
		adl_tbl.hdr_b0202ti.used_size = DV_ADL_SIZE;

		adl_tbl.path = DOVI_PATH;
		disp_adl_cfg_client_en(DV_ADL_V_MAIN, 1, 0);
		disp_config_adl_table(&adl_tbl);
	}

	if (dv_vdo_fe_en[1]) {
		/*update svdo lut, tmp use mvdolut*/
		memset(&adl_tbl, 0, sizeof(struct adl_src_tbl));

		adl_tbl.client = DV_ADL_V_SUB;

		adl_tbl.hdr_b0103.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.g2l[1]);
		adl_tbl.hdr_b0103.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ss.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.smluts[1]);
		adl_tbl.hdr_b0202ss.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202si.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.smluti[1]);
		adl_tbl.hdr_b0202si.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ts.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.tmluts[1]);
		adl_tbl.hdr_b0202ts.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ti.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.tmluti[1]);
		adl_tbl.hdr_b0202ti.used_size = DV_ADL_SIZE;

		adl_tbl.path = DOVI_PATH;
		disp_adl_cfg_client_en(DV_ADL_V_SUB, 1, 0);
		disp_config_adl_table(&adl_tbl);
	}

	if (dv_gfx_fe_en[0]) {
		/*update fhd lut*/
		memset(&adl_tbl, 0, sizeof(struct adl_src_tbl));

		adl_tbl.client = DV_ADL_G_FHD;

		adl_tbl.hdr_b0103.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_g2l[0]);
		adl_tbl.hdr_b0103.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ss.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_smluts[0]);
		adl_tbl.hdr_b0202ss.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202si.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_smluti[0]);
		adl_tbl.hdr_b0202si.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ts.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_tmluts[0]);
		adl_tbl.hdr_b0202ts.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ti.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_tmluti[0]);
		adl_tbl.hdr_b0202ti.used_size = DV_ADL_SIZE;

		adl_tbl.path = DOVI_PATH;
		disp_adl_cfg_client_en(DV_ADL_G_FHD, 1, 0);
		disp_config_adl_table(&adl_tbl);
	}

	if (dv_gfx_fe_en[1]) {
		/*update uhd lut*/
		memset(&adl_tbl, 0, sizeof(struct adl_src_tbl));

		adl_tbl.client = DV_ADL_G_UHD;

		adl_tbl.hdr_b0103.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_g2l[1]);
		adl_tbl.hdr_b0103.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ss.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_smluts[1]);
		adl_tbl.hdr_b0202ss.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202si.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_smluti[1]);
		adl_tbl.hdr_b0202si.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ts.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_tmluts[1]);
		adl_tbl.hdr_b0202ts.used_size = DV_ADL_SIZE;

		adl_tbl.hdr_b0202ti.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.gop_tmluti[1]);
		adl_tbl.hdr_b0202ti.used_size = DV_ADL_SIZE;

		adl_tbl.path = DOVI_PATH;
		disp_adl_cfg_client_en(DV_ADL_G_UHD, 1, 0);
		disp_config_adl_table(&adl_tbl);
	}

	if (dv_vdo_be_en) {
		memset(&adl_tbl, 0, sizeof(struct adl_src_tbl));

		adl_tbl.client = DV_SCRM;
		adl_tbl.scmb.p_data =
			(uint8_t *)(&p_reg->dv_lut_tbls.md_pkts);
		adl_tbl.scmb.used_size =
			p_reg->dv_scm.reg_meta_pkt_num * DV_MD_PER_PKT_SIZE;
		adl_tbl.path = DOVI_PATH;
		if (adl_tbl.scmb.used_size > 0) {
			disp_adl_cfg_client_en(DV_SCRM, 1, 0);
			disp_config_adl_table(&adl_tbl);
		}
	}
	if (dv_vdo_fe_en[0] || dv_vdo_fe_en[1] ||
		dv_gfx_fe_en[0] || dv_gfx_fe_en[1] ||
		dv_vdo_be_en)
		disp_adl_thread_wakeup(DOVI_PATH);

	dovi_info("%s end\n", __func__);
	return 0;
}

int dovi_config_menuload(void)
{
	struct ml_reg_table reg_tbl;
	struct dv_all_reg_tab *p_dsys_reg_tab = NULL;
	struct dv_all_reg_tab *p_msys_reg_tab = NULL;
	uint32_t padding = 0;
	uint32_t depth = 0;

	p_dsys_reg_tab = &dv_dsys_all_reg;
	p_msys_reg_tab = &dv_msys_all_reg;

	if ((p_dsys_reg_tab->used_depth > DV_ALL_REG_NUM
		|| p_dsys_reg_tab->used_depth == 0) &&
		(p_msys_reg_tab->used_depth > DV_ALL_REG_NUM
		|| p_msys_reg_tab->used_depth == 0)){
		dovi_info("%s reg num oversize %d %d\n", __func__,
			p_dsys_reg_tab->used_depth,
			p_msys_reg_tab->used_depth);
		return -1;
	}
#if 0
	for (depth = 0; depth < p_dsys_reg_tab->used_depth; depth++) {
		dovi_printf("0x%08x 0x%08x\n",
			p_dsys_reg_tab->reg_addr[depth],
			p_dsys_reg_tab->reg_value[depth]);
	}
#endif
	//dmsys
	depth = p_dsys_reg_tab->used_depth;
	if (depth >= DV_ALL_REG_NUM) {
		dovi_printf("dsys cmd size full\n");
		return 0;
	}
	//do 4 cmd align
	dovi_flow("config dsys ml size %d\n", depth);
	padding = (4 - (depth % 4)) % 4;

	while (padding) {
		p_dsys_reg_tab->reg_addr[depth] =
			p_dsys_reg_tab->reg_addr[depth-1];
		p_dsys_reg_tab->reg_value[depth] =
			p_dsys_reg_tab->reg_value[depth-1];
		p_dsys_reg_tab->reg_mask[depth] =
			p_dsys_reg_tab->reg_mask[depth-1];
		depth++;
		if (depth >= DV_ALL_REG_NUM) {
			dovi_printf("dsys cmd size full after align\n");
			break;
		}
		padding--;
	}
	dovi_flow("config dsys ml align size %d\n", depth);

	memset(&reg_tbl, 0, sizeof(struct ml_reg_table));
	reg_tbl.ml_ip = ML_DSYS_IP;
	reg_tbl.depth = depth;
	reg_tbl.p_reg_addr = p_dsys_reg_tab->reg_addr;
	reg_tbl.p_reg_value = p_dsys_reg_tab->reg_value;
	reg_tbl.p_reg_mask = p_dsys_reg_tab->reg_mask;
	reg_tbl.path = ML_DOVI;

	if (depth > 0)
		disp_ml_write_reg_multi(&reg_tbl);

	//mmsys
	depth = p_msys_reg_tab->used_depth;
	if (depth >= DV_ALL_REG_NUM) {
		dovi_printf("msys cmd size full\n");
		return 0;
	}
	dovi_flow("config msys ml size %d\n", depth);
	//do 4 cmd align
	padding = (4 - (depth % 4)) % 4;
	while (padding) {
		p_msys_reg_tab->reg_addr[depth] =
			p_msys_reg_tab->reg_addr[depth-1];
		p_msys_reg_tab->reg_value[depth] =
			p_msys_reg_tab->reg_value[depth-1];
		p_msys_reg_tab->reg_mask[depth] =
			p_msys_reg_tab->reg_mask[depth-1];
		depth++;
		if (depth >= DV_ALL_REG_NUM) {
			dovi_printf("msys cmd size full,keep max depth\n");
			break;
		}
		padding--;
	}
	dovi_flow("config msys ml align size %d\n", depth);

	memset(&reg_tbl, 0, sizeof(struct ml_reg_table));
	reg_tbl.ml_ip = ML_MSYS_IP;
	reg_tbl.depth = depth;
	reg_tbl.p_reg_addr = p_msys_reg_tab->reg_addr;
	reg_tbl.p_reg_value = p_msys_reg_tab->reg_value;
	reg_tbl.p_reg_mask = p_msys_reg_tab->reg_mask;
	reg_tbl.path = ML_DOVI;

	if (depth > 0)
		disp_ml_write_reg_multi(&reg_tbl);

	return 0;
}

uint32_t disp_dovi_get_rpu_info(uint32_t idx, uint32_t first_frame,
	struct mtk_vdp_dovi_md_t *dovi_md_info)
{
	enum DV_RPU_TYPE rpu_type = HEVC_RPU;
	unsigned char *rpu_addr = NULL;
	uint32_t rpu_len = 0;

	if (idx >= V_G_LAYER_MAX || dovi_md_info == NULL) {
		dovi_printf("can not get rpu info\n");
		return -1;
	}

	if (dovi_md_info->svp) {
		p_src_param[idx].svp = true;
		p_src_param[idx].sec_handle =
			dovi_md_info->sec_handle;
		if (first_frame && (dovi_md_info->keyfrm_len > dovi_md_info->len)) {
			p_src_param[idx].rpu_bs_len =
				dovi_md_info->keyfrm_len;
			p_src_param[idx].sec_offset =
				dovi_md_info->keyfrm_sec_handle_offset;
		} else {
			p_src_param[idx].rpu_bs_len =
				dovi_md_info->len;
			p_src_param[idx].sec_offset =
				dovi_md_info->sec_handle_offset;
		}

		dovi_rpu("svpfrae:%d, %d hdl:0x%x(%d %d)(0x%x 0x%x)\n", idx,
			first_frame, p_src_param[idx].sec_handle,
			dovi_md_info->len, dovi_md_info->keyfrm_len,
			dovi_md_info->sec_handle_offset,
			dovi_md_info->keyfrm_sec_handle_offset);
	} else {
		p_src_param[idx].svp = false;
		p_src_param[idx].is_rbsp = 0;
		/* remove rpu NAL type */
		if (!layer_info_set_by_cmd) {
			if (first_frame && (dovi_md_info->keyfrm_len > dovi_md_info->len)) {
				rpu_addr = dovi_md_info->keyfrm_buff;
				rpu_len = dovi_md_info->keyfrm_len;
			} else {
				rpu_addr = dovi_md_info->buff;
				rpu_len = dovi_md_info->len;
			}
			rpu_type = dovi_parse_rpu_av1(
				rpu_addr,
				rpu_len,
				p_src_param[idx].rpu_bs_buffer,
				&p_src_param[idx].rpu_bs_len);

			if (rpu_type == HEVC_RPU) {
				p_src_param[idx].rpu_bs_len =
				dovi_remove_rpu_nal_type(
				first_frame,
				rpu_addr,
				p_src_param[idx].rpu_bs_buffer,
				rpu_len);
			} else if (rpu_type == AV1_RPU)
				p_src_param[idx].is_rbsp = 1;

			p_src_param[idx].rpu_type = rpu_type;

			if (p_src_param[idx].rpu_bs_len <= 0) {
				dovi_error("layer[%d]rpu len error\n", idx);
				return DOVI_RET_ERROR;
			}
		} else {
			rpu_addr = p_src_param[idx].rpu_bs_buffer;

			if (layer_info_set_by_cmd == 2) {
				dovi_av1_parser_test();
				p_src_param[idx].is_rbsp = 1;
			} else {
				dovi_md_info->len = 237;
				p_src_param[idx].rpu_bs_len = 240;
				memcpy(rpu_addr, rpu_test_data, 240);
			}
		}

		dovi_rpu("rpu[%d][%d] %d (%d %d) %d\n", idx, first_frame,
			rpu_type, dovi_md_info->len, dovi_md_info->keyfrm_len,
			p_src_param[idx].rpu_bs_len);
	}

	return 0;
}
int disp_dovi_process(uint32_t enable,
	struct mtk_disp_hdr_md_info_t *hdr_metadata)
{
	enum DISP_DR_TYPE_T dv_pre_input_type =
		DISP_DR_TYPE_DOVI;
	enum DISP_DR_TYPE_T dv_pre_input_type1 =
		DISP_DR_TYPE_DOVI;
	uint8_t b_dovi_src = 0;
	uint32_t old_num_input = 0;
	bool b_sub_video_st = 0;
	struct mtk_vdp_dovi_md_t *dovi_md_info_main = NULL;
	struct mtk_vdp_dovi_md_t *dovi_md_info_sub = NULL;
	uint32_t first_frame[2] = { 0 };

	if (hdr_metadata == NULL) {
		dovi_error("hdrmetadata null\n");
		return DOVI_RET_ERROR;
	}

	mutex_lock(&disp_dovi_mutex);

	dovi_proc_state = ((dovi_enable << 1) | enable);

	if (enable && (vdp_start_st[0] == 0) &&
		!((hdr_metadata->dr_range == DISP_DR_TYPE_SDR)
		|| (hdr_metadata->dr_range == DISP_DR_TYPE_PHLP_RESVERD))) {
		dovi_printf("ignore the call after video stop %d %d %d\n",
			vdp_start_st[0], vdp_start_st[1], hdr_metadata->dr_range);
		mutex_unlock(&disp_dovi_mutex);
		return DOVI_RET_OK;
	}

	old_num_input = p_cp_param->num_input;
	b_sub_video_st = p_src_param[LAYER1].en;
	p_cp_param->num_input = 0;
	p_cp_param->cp_init_update = 0;
	p_cp_param->dm_md_parse_ctrl = 0;
	p_cp_param->pri_input = 0;
	p_cp_param->vpm_trans_timeout = 0;

	p_cp_param->num_input++;
	p_src_param[LAYER0].en = (bool) enable;
	p_src_param[LAYER0].dr_type = hdr_metadata->dr_range;
	p_src_param[LAYER0].input_mode = INPUT_MODE_OTT;
	dovi_get_input_format(&dv_pre_input_type);

	if (hdr_metadata->dr_range == DISP_DR_TYPE_DOVI)
		b_dovi_src |= MAIN_SRC_DOVI;

	if (dv_vdo_fe_en[1] &&
		dovi_hdr_md_info[LAYER1].dr_range == DISP_DR_TYPE_DOVI)
		b_dovi_src |= SUB_SRC_DOVI;

	if (enable) {
		if (dv_vdo_fe_en[1]) {
			p_cp_param->num_input++;
			p_src_param[LAYER1].en = true;
			dovi_get_input_format1(&dv_pre_input_type1);
			//dovi_set_sub_video_info(&dovi_hdr_md_info[LAYER1]);
		} else
			p_src_param[LAYER1].en = false;

		if ((!dovi_idk_test) && dv_gfx_fe_en[0]) {
			p_cp_param->num_input++;
			//dovi_set_priority_mode(G_PRIORITY);
			dovi_set_graphic_info(1);
		} else
			dovi_set_graphic_info(0);

		if ((!dovi_idk_test) && dv_gfx_fe_en[1]) {
			p_cp_param->num_input++;
			//dovi_set_priority_mode(G_PRIORITY);
			dovi_set_graphic_info_uhd(1);
		} else if ((idk_gfx_en == 1) && dovi_idk_test) {
			p_cp_param->num_input++;
			dovi_set_graphic_info(1);
		} else if ((idk_gfx_en == 2) && dovi_idk_test) {
			p_cp_param->num_input++;
			dovi_set_graphic_info_uhd(1);

			p_cp_param->num_input++;
			dovi_set_graphic_info(1);
		} else
			dovi_set_graphic_info_uhd(0);
	}

	if ((p_cp_param->num_input > MAX_NUM_INPUT) ||
		(old_num_input > MAX_NUM_INPUT)) {
		dovi_printf("input layer over size %d\n",
			p_cp_param->num_input);
		mutex_unlock(&disp_dovi_mutex);
		return DOVI_RET_ERROR;
	}

	dovi_dbg("%s[%d][%d] %d %d %d %d %d %d %d %d %d num[%d %d][%d %d %d %d]\n",
		__func__, hdr_vsync_cnt, befifo_irq_cnt, dovi_proc_state,
		b_dovi_src,
		hdr_metadata->dr_range, hdr_metadata->enable,
		dovi_hdr_md_info[1].dr_range, dovi_hdr_md_info[1].enable,
		dovi_md_parser_enable, dv_pre_input_type, dv_pre_input_type1,
		old_num_input, p_cp_param->num_input,
		dv_vdo_fe_en[0], dv_vdo_fe_en[1],
		dv_gfx_fe_en[0], dv_gfx_fe_en[1]);

	/* enable or already enable dovi */
	if ((dovi_proc_state & 1) == 1) {
		if ((dovi_proc_state == 1)
		    || (hdr_metadata->enable == DOVI_INOUT_FORMAT_CHANGE)
		    || (dovi_hdr_md_info[LAYER1].enable
		    == DOVI_INOUT_FORMAT_CHANGE)) {
			p_src_param[LAYER0].src_frame_num = 0;
			p_src_param[LAYER1].src_frame_num = 0;
			dovi_get_input_format(&dv_pre_input_type);
			dovi_set_video_info(hdr_metadata);
			dovi_get_input_format1(&dv_pre_input_type1);
			dovi_set_sub_video_info(&dovi_hdr_md_info[LAYER1]);
		}

		/* hw setting generate by control path drv */
		/*main or sub is dovi src*/
		if (b_dovi_src) {
			first_frame[0] = (dovi_proc_state == 1);
			first_frame[1] = (dovi_proc_state == 1);

			/* do init process */
			if (dovi_proc_state == 1) {
				//dovi_sec_md_parser_init();
				if (dv_vdo_fe_en[0])
					p_cp_param->dm_md_parse_ctrl |= 0x1;
				if (dv_vdo_fe_en[1])
					p_cp_param->dm_md_parse_ctrl |= 0x2;
				dovi_md_parser_enable = true;
				p_src_param[LAYER0].src_frame_num = 0;
				p_src_param[LAYER1].src_frame_num = 0;
			}

			if (dovi_md_parser_enable == false) {
				//dovi_sec_md_parser_init();
				if (dv_vdo_fe_en[0])
					p_cp_param->dm_md_parse_ctrl |= 0x1;
				if (dv_vdo_fe_en[1])
					p_cp_param->dm_md_parse_ctrl |= 0x2;
				dovi_md_parser_enable = true;
				first_frame[0] = 1;
				first_frame[1] = 1;
				p_src_param[LAYER0].src_frame_num = 0;
				p_src_param[LAYER1].src_frame_num = 0;
			} else if (dovi_proc_state != 1) {
				if ((b_dovi_src & MAIN_SRC_DOVI) &&
					(dv_pre_input_type != DISP_DR_TYPE_DOVI)) {
					p_cp_param->dm_md_parse_ctrl |= 0x1;
					dovi_md_parser_enable = true;
					first_frame[0] = 1;
					p_src_param[LAYER0].src_frame_num = 0;
				}
				if ((b_dovi_src & SUB_SRC_DOVI) &&
					(dv_pre_input_type1 != DISP_DR_TYPE_DOVI)) {
					p_cp_param->dm_md_parse_ctrl |= 0x2;
					dovi_md_parser_enable = true;
					first_frame[1] = 1;
					p_src_param[LAYER1].src_frame_num = 0;
				}
				/* here add keyfrm case handle when pip change */
				if ((old_num_input != 0) &&
					(old_num_input != p_cp_param->num_input) &&
					(p_src_param[LAYER1].en != b_sub_video_st)) {
					if ((b_dovi_src & MAIN_SRC_DOVI) &&
						(hdr_metadata->metadata_info
						.dovi_metadata.keyfrm_len >
						hdr_metadata->metadata_info
						.dovi_metadata.len)) {
						p_cp_param->dm_md_parse_ctrl |= 0x1;
						dovi_md_parser_enable = true;
						first_frame[0] = 1;
						p_src_param[LAYER0].src_frame_num = 0;
					}
				}
			}

			if (dv_vdo_fe_en[0]
				&& (dovi_hdr_md_info[0].dr_range
				== DISP_DR_TYPE_DOVI)) {
				dovi_md_info_main =
					&(hdr_metadata->metadata_info
					.dovi_metadata);
				disp_dovi_get_rpu_info(LAYER0, first_frame[0],
				dovi_md_info_main);
			}

			if (dv_vdo_fe_en[1]
				&& (dovi_hdr_md_info[1].dr_range
				== DISP_DR_TYPE_DOVI)) {
				dovi_md_info_sub =
					&(dovi_hdr_md_info[1]
					.metadata_info.dovi_metadata);
				disp_dovi_get_rpu_info(LAYER1, first_frame[1],
					dovi_md_info_sub);
			}

			dovi_rpu("frm num %d %d\n",
				p_src_param[LAYER0].src_frame_num,
				p_src_param[LAYER1].src_frame_num);
		}

		if ((hdr_metadata->dr_range == DISP_DR_TYPE_HDR10) ||
			(hdr_metadata->dr_range == DISP_DR_TYPE_HLG))
			disp_dovi_set_hdr10_param(&p_src_param[LAYER0].hdr10_md,
			hdr_metadata);

		if (dv_vdo_fe_en[1]
			&& ((dovi_hdr_md_info[1].dr_range == DISP_DR_TYPE_HDR10)
			|| (dovi_hdr_md_info[1].dr_range == DISP_DR_TYPE_HLG)))
			disp_dovi_set_hdr10_param(&p_src_param[LAYER1].hdr10_md,
			&dovi_hdr_md_info[1]);

		/* hw setting generate by control path drv */
		if (dovi_proc_state == 1) {
			/* do init process */
			dovi_update_target_lum();

			dovi_update_graphic_info();

			/* be enable */
			dovi_be_hal_set_enable(enable);

			//dovi_sec_cp_test_init();
			p_cp_param->cp_init_update = 1;

			if (reg_test_enable) {
				dovi_info("set ipcore with default setting\n");
				if (p_cp_param->output_format
					== SIGNAL_FORMAT_DOVI)
					disp_dovi_common_test(4);
				else
					disp_dovi_common_test(2);
			}
		}

		if (((hdr_metadata->enable == DOVI_INOUT_FORMAT_CHANGE) ||
			(dovi_hdr_md_info[1].enable ==
			DOVI_INOUT_FORMAT_CHANGE))
			&& (dovi_proc_state != 1)) {
			dovi_update_target_lum();
			dovi_update_graphic_info();
			//dovi_sec_cp_test_init();
			p_cp_param->cp_init_update = 1;
		} else if (hdr_metadata->enable == DOVI_RESOLUTION_CHANGE) {
			dovi_update_target_lum();
			dovi_set_video_info(hdr_metadata);
			//dovi_sec_cp_test_init();
			p_cp_param->cp_init_update = 1;
		} else if (hdr_metadata->enable == DOVI_PRIORITY_MODE_CHANGE)
			//dovi_sec_cp_test_init();
			p_cp_param->cp_init_update = 1;
		else if ((old_num_input != 0) &&
			(old_num_input != p_cp_param->num_input))
			p_cp_param->cp_init_update = 1;
			//dovi_sec_cp_test_init();

		*p_profile4 = false;

		memset(p_dv_out_params, 0, sizeof(struct dv_hw_reg));
		if (dovi_sec_cp_test_main() == DOVI_STATUS_ERROR) {
			dovi_printf("control main failed\n");
			mutex_unlock(&disp_dovi_mutex);
			return DOVI_RET_ERROR;
		}

		if (*p_profile4 == 4) {
			dovi_printf("This is profile4-FEL stream,we do not support!\n");
			mutex_unlock(&disp_dovi_mutex);
			return DOVI_RET_ERROR;
		}

		if (dovi_get_ext_md)
			disp_dovi_dump_ext_md();

		if ((p_cp_param->output_format == SIGNAL_FORMAT_HDR10)
		    || (p_cp_param->output_format == SIGNAL_FORMAT_HLG))
			disp_dovi_update_hdr10_info_frame(p_hdr10_md,
			hdr_metadata);

		disp_dovi_md();

		//call adl to generate autodownload cmd
		dovi_config_adl();

		//add regbase
		dovi_get_output_setting(p_dv_out_params, b_dovi_src);

		//call ml to generate menuload cmd
		dovi_config_menuload();

		//call fefifi set swap for dovi gfx fe
		dovi_config_fefifo_swap(true);
		/* switch display path register setting */

		if (dv_vdo_fe_en[0])
			p_src_param[LAYER0].src_frame_num++;
		if (dv_vdo_fe_en[1])
			p_src_param[LAYER1].src_frame_num++;

		if (dovi_proc_state == 1)
			dovi_enable = enable;

		if (dovi_hdr_md_info[LAYER0].enable > 1)
			dovi_hdr_md_info[LAYER0].enable = 1;
		if (dovi_hdr_md_info[LAYER1].enable > 1)
			dovi_hdr_md_info[LAYER1].enable = 1;

		dovi_info("%s end\n", __func__);
	} else if (dovi_proc_state == 2) {
		/* disable dovi path */
		dovi_default("dovi path disable vysnc %d %d\n",
		hdr_vsync_cnt, befifo_irq_cnt);

		/* do uninit process */
		dovi_sec_md_parser_uninit();
		dovi_md_parser_enable = false;

		dovi_sec_cp_test_uninit();

		/* control path uninit */
		dovi_vdo_fe_hal_set_enable(LAYER0, enable);
		dovi_vdo_fe_hal_set_enable(LAYER1, enable);

		/*disable adl*/
		disp_adl_cfg_client_en(DV_ADL_V_MAIN, 0, 0);
		disp_adl_cfg_client_en(DV_ADL_V_SUB, 0, 0);
		disp_adl_cfg_client_en(DV_ADL_G_FHD, 0, 0);
		disp_adl_cfg_client_en(DV_ADL_G_UHD, 0, 0);
		disp_adl_cfg_client_en(DV_SCRM, 0, 0);

		/* gfx_fe path uninit */
		//dovi_gfx_fe_hal_set_enable(LAYER0, enable);
		//dovi_gfx_fe_hal_set_enable(LAYER1, enable);

		/* be path uninit */
		dovi_be_hal_set_enable(enable);

		dovi_config_fefifo_swap(false);

		dovi_enable = enable;
	} else if (dovi_proc_state == 0) {
		/* for lk force hdr, but need change to
		 * adaptive when ALLM on and tv support std+allm
		 */
		dovi_config_fefifo_swap(false);
	}

	mutex_unlock(&disp_dovi_mutex);

	return DOVI_RET_OK;
}

int dovi_mmsys_update_reg(uint32_t *p_reg_tbl, uint32_t len)
{
	if (p_reg_tbl == NULL || len <= 0) {
		dovi_error("%s input error\n", __func__);
		return -1;
	}

	disp_ml_set_buffer(ML_MSYS_IP, (void *)p_reg_tbl, len);
	return 0;
}

void dovi_check_reg_info(struct dv_reg_hw_output *p_reg_out)
{
	struct dv_reg_tbl *reg_table = NULL;
	uint32_t len = 0;
	uint32_t i = 0;

	if (p_reg_out == NULL) {
		dovi_error("%s input error\n", __func__);
		return;
	}

	reg_table = &p_reg_out->reg_table;
	len = reg_table->used_depth;
	for (i = 0; i < len; i++) {
		dovi_reg("addr:0x%08x value:0x%08x msk:0x%08x\n",
			reg_table->reg_addr[i],
			reg_table->reg_value[i],
			reg_table->reg_mask[i]);
	}
}

void disp_dovi_set_tz_test_info(uint32_t id)
{
	uint32_t out_format = 0;
	int use_ll = 0;
	int ll_rgb = 0;
	uint32_t dovi2hdr10_mapping = 0;
	char *vsvdb_file_name = NULL;
	enum pri_mode_t priority_mode = 0;
	enum graphic_format_t g_format = 0;
	uint32_t gfx_on = 0;
	int ret = 0;

	out_format = get_dovi_out_format(id);
	use_ll = get_dovi_use_ll(id);
	ll_rgb = get_dovi_ll_rgb_desired(id);
	dovi2hdr10_mapping =
		get_dovi2hdr10_mapping_type(id);
	vsvdb_file_name = get_dovi_vsvdb_file_name(id);
	gfx_on = get_dovi_graphic_on(id);
	priority_mode = get_dovi_priority_mode(id);
	g_format = get_dovi_g_format(id);

	dovi_set_output_format(out_format);
	dovi_set_low_latency_mode(use_ll, ll_rgb);
	dovi_set_dovi2hdr10_mapping(dovi2hdr10_mapping);

	dovi_set_graphic_info(gfx_on);
	dovi_set_priority_mode(priority_mode);
	dovi_set_graphic_format(g_format);

	memset(p_cp_param->vsvdb_file, 0, MAX_FILENAME_LENGTH);
	ret = sprintf(p_cp_param->vsvdb_file, "%s", vsvdb_file_name);
	if (ret < 0) {
		dovi_error("dovi tz_test fail\n");
		return;
	}

	//load vsvdb
	memset(p_cp_param->vsvdb_hdmi, 0, 0x1A);
	memcpy(p_cp_param->vsvdb_hdmi, vsvdb_v1_15, 0x1A);

	//load compose md and dm  md  to share buffer
	p_src_param[LAYER0].orig_md_len = 123;

	//set video info
	p_cp_param->width = 1920;
	p_cp_param->height = 1080;
	p_src_param[LAYER0].input_format = SIGNAL_FORMAT_DOVI;
	p_src_param[LAYER0].src_yuv_range = SIGNAL_RANGE_SMPTE;
	p_src_param[LAYER0].src_bit_depth = 10;
	p_src_param[LAYER0].src_fps = 60;
	p_cp_param->test_mode = 0;
	//p_cp_param->support_el = 0;

	*p_profile4 = false;
	p_src_param[LAYER0].svp = 0;
	p_src_param[LAYER0].dr_type = DISP_DR_TYPE_DOVI;

	dovi_update_target_lum();

	dovi_update_graphic_info();

	dovi_sec_cp_test_init();

	dovi_sec_cp_test_main();

	dovi_info("tz_test done\n");

}

void dovi_av1_parser_test(void)
{
	enum DV_RPU_TYPE ret = HEVC_RPU;
	uint8_t *src_rpu = NULL;
	uint32_t src_rpu_len = 0;

	src_rpu = av1_obu_data;
	src_rpu_len = 230;
	ret = dovi_parse_rpu_av1(src_rpu,
		src_rpu_len,
		p_src_param[LAYER0].rpu_bs_buffer,
		&p_src_param[LAYER0].rpu_bs_len);

	dovi_info("dst_rpu[%d] 0x%p\n", p_src_param[LAYER0].rpu_bs_len,
		(uint32_t *)p_src_param[LAYER0].rpu_bs_buffer);
}

void disp_dovi_update_input_size(uint32_t layer_id,
	uint32_t u4width, uint32_t u4height)
{
	struct ml_reg_table reg_tbl;
	uint32_t reg_addr[4] = { 0 };
	uint16_t reg_val[4] = { 0 };
	uint16_t reg_mask[4] = { 0xffff, 0xffff, 0xffff, 0xffff };

	if (layer_id == 0) {
		reg_addr[0] = 0x150129cc;
		reg_addr[1] = 0x150129cc;
		reg_addr[2] = 0x150129d0;
		reg_addr[3] = 0x150129d0;
	} else {
		reg_addr[0] = 0x150159cc;
		reg_addr[1] = 0x150159cc;
		reg_addr[2] = 0x150159d0;
		reg_addr[3] = 0x150159d0;
	}

	reg_val[0] = u4width;
	reg_val[1] = u4width;
	reg_val[2] = u4height;
	reg_val[3] = u4height;

	memset(&reg_tbl, 0, sizeof(struct ml_reg_table));
	reg_tbl.ml_ip = ML_DSYS_IP;
	reg_tbl.depth = 4;
	reg_tbl.p_reg_addr = reg_addr;
	reg_tbl.p_reg_value = reg_val;
	reg_tbl.p_reg_mask = reg_mask;
	reg_tbl.reg_type = 1;
	reg_tbl.path = ML_DOVI;

	disp_ml_write_reg_multi(&reg_tbl);

}

void disp_dovi_set_internalbyass(uint8_t id)
{
	struct dv_reg_tbl *p_msys_tbl = NULL;
	struct dv_reg_tbl *p_dsys_tbl = NULL;
	struct ml_reg_table reg_tbl;
	bool msys_bypass = false;
	bool dsys_bypass = false;

	p_msys_tbl = &dv_bypass_table[0];
	p_dsys_tbl = &dv_bypass_table[1];

	memset(p_msys_tbl, 0, sizeof(struct dv_reg_tbl));
	memset(p_dsys_tbl, 0, sizeof(struct dv_reg_tbl));

	if (id == 0)
		msys_bypass = true;
	else if (id == 1)
		dsys_bypass = true;
	else {
		msys_bypass = true;
		dsys_bypass = true;
	}

	if (dsys_bypass) {
		//defalut path internal bypass setting
		reg_0618_default(0, MVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_061c_default(0x2, MVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_0634_default(0x8000, MVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_0804_default(0xfd, MVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_081c_default(0x12e, MVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_09ec_default(0x80, MVDO_HDR_FE_BASE, p_dsys_tbl);

		reg_0618_default(0, SVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_061c_default(0x2, SVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_0634_default(0x8000, SVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_0804_default(0xfd, SVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_081c_default(0x12e, SVDO_HDR_FE_BASE, p_dsys_tbl);
		reg_09ec_default(0x80, SVDO_HDR_FE_BASE, p_dsys_tbl);
	}

	if (msys_bypass) {
		//default gop internal by pass setting
		reg_9100_gop(0x8001, FHD_HDR_FE_BASE, p_msys_tbl);
		reg_9204_gop(0xfd, FHD_HDR_FE_BASE, p_msys_tbl);
		reg_921c_gop(0x20, FHD_HDR_FE_BASE, p_msys_tbl);
		reg_93ec_gop(0x80, FHD_HDR_FE_BASE, p_msys_tbl);
		reg_r2y_gop(0xe030, FHD_HDR_FE_BASE, p_msys_tbl);
		reg_hdr_v_size_gop(dovi_out_height,
			FHD_HDR_FE_BASE, p_msys_tbl);

		reg_9100_gop(0x8001, UHD_HDR_FE_BASE, p_msys_tbl);
		reg_9204_gop(0xfd, UHD_HDR_FE_BASE, p_msys_tbl);
		reg_921c_gop(0x20, UHD_HDR_FE_BASE, p_msys_tbl);
		reg_93ec_gop(0x80, UHD_HDR_FE_BASE, p_msys_tbl);
		reg_r2y_gop(0xe030, UHD_HDR_FE_BASE, p_msys_tbl);
		reg_hdr_v_size_gop(dovi_out_height,
			UHD_HDR_FE_BASE, p_msys_tbl);

		//be dm default internal bypass setting
		reg_b204_defalut(0x7e, VDO_BE_REG_BASE, p_msys_tbl);
		reg_b320_defalut(0x00, VDO_BE_REG_BASE, p_msys_tbl);
		reg_b3c8_defalut(0x1, VDO_BE_REG_BASE, p_msys_tbl);
		/*add 1 cmd for 4 align*/
		reg_b3c8_defalut(0x1, VDO_BE_REG_BASE, p_msys_tbl);
	}

	if (dsys_bypass) {
		dovi_printf("dsys reg-depth %d\n", p_dsys_tbl->used_depth);
		memset(&reg_tbl, 0, sizeof(struct ml_reg_table));
		reg_tbl.ml_ip = ML_DSYS_IP;
		reg_tbl.depth = p_dsys_tbl->used_depth;
		reg_tbl.p_reg_addr = p_dsys_tbl->reg_addr;
		reg_tbl.p_reg_value = p_dsys_tbl->reg_value;
		reg_tbl.p_reg_mask = p_dsys_tbl->reg_mask;
		reg_tbl.path = ML_OPENHDR;
		disp_ml_write_reg_multi(&reg_tbl);
	}

	if (msys_bypass) {
		memset(&reg_tbl, 0, sizeof(struct ml_reg_table));
		dovi_printf("msys reg-depth %d\n", p_msys_tbl->used_depth);
		reg_tbl.ml_ip = ML_MSYS_IP;
		reg_tbl.depth = p_msys_tbl->used_depth;
		reg_tbl.p_reg_addr = p_msys_tbl->reg_addr;
		reg_tbl.p_reg_value = p_msys_tbl->reg_value;
		reg_tbl.p_reg_mask = p_msys_tbl->reg_mask;
		reg_tbl.path = ML_OPENHDR;
		disp_ml_write_reg_multi(&reg_tbl);
	}

}


void disp_dovi_set_adldelay(uint32_t layer_id)
{
	switch (layer_id) {
	case LAYER0:
		Dv_WriteREG(dovi_reg_base[DOVI_MVDO_FE] + 0x9C4, adl_mode);
		break;
	case LAYER1:
		Dv_WriteREG(dovi_reg_base[DOVI_SVDO_FE] + 0x9C4, adl_mode);
		break;
	case LAYER2:
		Dv_WriteREG(dovi_reg_base[DOVI_FHD_FE] + 0x3C4, adl_mode);
		break;
	case LAYER3:
		Dv_WriteREG(dovi_reg_base[DOVI_UHD_FE] + 0x3C4, adl_mode);
		break;
	default:
		break;
	}
}

uint8_t *disp_dovi_get_hdmi_vsem_info(
	uint32_t *dovi_vsem_num_pks, uint32_t *dovi_hdmi_type)
{
	uint8_t *dovi_hdmi_vsem_addr = NULL;

	if ((dovi_hdmi_type != NULL) &&
		(dovi_vsem_num_pks != NULL)) {
		*dovi_hdmi_type = dovi_share_mem->hdmi_md_type;
		*dovi_vsem_num_pks =
			dovi_share_mem->dv_out_params.dv_scm.vsem_meta_pkt_num;
		dovi_hdmi_vsem_addr =
			dovi_share_mem->dv_out_params.dv_lut_tbls.md_pkts;
	}

	return dovi_hdmi_vsem_addr;
}

void disp_dovi_dump_ext_md(void)
{
	uint32_t size = 0;
	uint32_t i, j = 0;
	uint32_t md_base_len = 0;
	uint32_t ext_len = 0;
	uint8_t *p_md;
	uint32_t num_ext_blocks = 0;

	md_base_len = sizeof(struct dm_metadata_base_t);
	size = md_base_len;

	dovi_printf("dump extmdinfo %d %d\n",
		dovi_share_mem->hdmi_md_len, md_base_len);
	if ((dovi_share_mem->hdmi_md_len) > md_base_len) {
		p_md = (uint8_t *)dovi_share_mem->hdmi_md;
		num_ext_blocks = (((struct dm_metadata_base_t *)p_md)->num_ext_blocks);
		if (num_ext_blocks > MAX_DM_EXT_BLOCKS)
			return;
		for (i = 0; i < num_ext_blocks; i++) {
			if ((size + 5) < DOVI_MD_SIZE) {
				ext_len = ((p_md[size] << 24) +
					(p_md[size + 1] << 16) +
					(p_md[size + 2] << 8) + p_md[size + 3]);
				if (ext_len >= MAX_UNKNOWN_MD_SIZE)
					return;

				dovi_printf("ext_md_block[%d] len[%d] lvl[%d]\n", i,
					ext_len, p_md[size + 4]);

				size += 5;

				for (j = 0; (j < ext_len) && ((size + j) < DOVI_MD_SIZE); j++)
					dovi_printf("ext_md[%d] = 0x%x\n", j, p_md[size + j]);

				size += ext_len;
			}
		}
	}

}

bool disp_dovi_get_ext_md_info(
	uint32_t ext_blk_no, uint8_t *ext_buf, uint32_t buf_len)
{
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t md_base_len = 0;
	uint32_t ext_len = 0;
	uint8_t *p_md;
	uint32_t num_ext_blocks = 0;

	md_base_len = sizeof(struct dm_metadata_base_t);
	size = md_base_len;

	if ((ext_buf != NULL) &&
		(dovi_share_mem->hdmi_md_len) > md_base_len) {
		p_md = (uint8_t *)dovi_share_mem->hdmi_md;
		num_ext_blocks = (((struct dm_metadata_base_t *)p_md)->num_ext_blocks);
		if (num_ext_blocks > MAX_DM_EXT_BLOCKS)
			return false;
		for (i = 0; i < num_ext_blocks; i++) {
			if ((size + 5) < DOVI_MD_SIZE) {
				ext_len = ((p_md[size] << 24) +
					(p_md[size + 1] << 16) +
					(p_md[size + 2] << 8) + p_md[size + 3]);

				if (ext_len >= MAX_UNKNOWN_MD_SIZE)
					return false;

				if ((p_md[size + 4] == ext_blk_no) &&
					(ext_len <= buf_len) &&
					((ext_len + size + 5) < DOVI_MD_SIZE)) {
					memcpy(ext_buf, &(p_md[size + 5]), ext_len);
					return true;
				}
				size += (ext_len + 5);
			}
		}
	}

	return false;
}

enum dovi_signal_format_t dovi_judge_out_format(
	struct disp_hw_tv_capbility *tv_cap,
	const struct disp_hw_resolution *resolution)
{
	bool is_4k60_out = dovi_check_4k60_timing(resolution->res_mode);
	enum dovi_signal_format_t dovi_out_format_new =
		DOVI_FORMAT_SDR;

	dovi_out_format_new =
	get_tv_output_format(tv_cap, is_4k60_out, resolution);
	return dovi_out_format_new;
}

