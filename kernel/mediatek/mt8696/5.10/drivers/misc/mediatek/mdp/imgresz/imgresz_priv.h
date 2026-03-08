/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef MTK_IMGRESZ_PRIV_H
#define MTK_IMGRESZ_PRIV_H

#include "imgresz.h"

#define loginfo(lvl, string, args...) do {\
		if (imgresz_log_level & lvl)\
			pr_info("[ImgResz]"string, ##args);\
		} while (0)
#define logwarn(string, args...) pr_info("[ImgResz]"string, ##args)

enum imgresz_chip_ver {
	IMGRESZ_CURR_CHIP_VER_BASE = 0,
	IMGRESZ_CURR_CHIP_VER_8697,
	IMGRESZ_CURR_CHIP_VER_8695,
	IMGRESZ_CURR_CHIP_VER_8696,
	IMGRESZ_CURR_CHIP_VER_MAX
};

enum imgresz_resample_method_t {
	IMGRESZ_RESAMPLE_METHOD_M_TAP,/* multi-tap */
	IMGRESZ_RESAMPLE_METHOD_4_TAP,
	IMGRESZ_RESAMPLE_METHOD_8_TAP
};

enum imgresz_buf_main_format_t {
	IMGRESZ_BUF_MAIN_FORMAT_Y_C,
	IMGRESZ_BUF_MAIN_FORMAT_Y_CB_CR,
	IMGRESZ_BUF_MAIN_FORMAT_INDEX,
	IMGRESZ_BUF_MAIN_FORMAT_ARGB,
	IMGRESZ_BUF_MAIN_FORMAT_AYUV,
	IMGRESZ_BUF_MAIN_FORMAT_GREY,
};

enum imgresz_yuv_format_t {
	IMGRESZ_YUV_FORMAT_420,
	IMGRESZ_YUV_FORMAT_422,
	IMGRESZ_YUV_FORMAT_444,
};

enum imgresz_argb_format_t {
	IMGRESZ_ARGB_FORMAT_0565,
	IMGRESZ_ARGB_FORMAT_1555,
	IMGRESZ_ARGB_FORMAT_4444,
	IMGRESZ_ARGB_FORMAT_8888,
};

struct imgresz_buf_format {
	enum imgresz_buf_main_format_t mainformat;
	enum imgresz_yuv_format_t yuv_format;
	enum imgresz_argb_format_t argb_format;

	unsigned int h_sample[3];
	unsigned int v_sample[3];
	bool block;
	bool progressive;
	bool top_field;
	bool bit10;
	bool jump_10bit;
	unsigned int pixelformat;
};

struct imgresz_hal_info {
	unsigned int h8_factor_y;
	unsigned int h8_offset_y;
	unsigned int h8_factor_cb;
	unsigned int h8_offset_cb;
	unsigned int h8_factor_cr;
	unsigned int h8_offset_cr;
	unsigned int hsa_factor_y;
	unsigned int hsa_offset_y;
	unsigned int hsa_factor_cb;
	unsigned int hsa_offset_cb;
	unsigned int hsa_factor_cr;
	unsigned int hsa_offset_cr;
	unsigned int v4_factor_y;
	unsigned int v4_factor_cb;
	unsigned int v4_factor_cr;
	unsigned int v4_offset_y;
	unsigned int v4_offset_cb;
	unsigned int v4_offset_cr;
	unsigned int vm_factor_y;
	unsigned int vm_factor_cb;
	unsigned int vm_factor_cr;
	unsigned int vm_offset_y;
	unsigned int vm_offset_cb;
	unsigned int vm_offset_cr;
	bool vm_scale_up_y;
	bool vm_scale_up_cb;
	bool vm_scale_up_cr;
};

struct imgresz_partition_info {
	unsigned int src_x_offset;
	unsigned int src_x_offset_c;
	unsigned int src_w;
	unsigned int src_w_y;
	unsigned int src_w_c;
	unsigned int src_w_cr;
	unsigned int src_h;
	unsigned int src_h_y;
	unsigned int src_h_c;
	unsigned int src_h_cr;
	unsigned int dst_x_offset;
	unsigned int dst_w;
};

struct imgresz_scale_data {
	enum imgresz_ticket_fun_type funtype;
	enum imgresz_scale_mode scale_mode;

	enum imgresz_resample_method_t h_method;
	enum imgresz_resample_method_t v_method;
	struct imgresz_src_buf_info src_buf;
	struct imgresz_dst_buf_info dst_buf;
	struct imgresz_buf_format src_format;
	struct imgresz_buf_format dst_format;
	struct imgresz_rm_info rm_info;
	struct imgresz_jpg_info jpg_info;
	struct imgresz_partial_buf_info partial_info;
	struct imgresz_hal_info hal_info;
	struct imgresz_partition_info partition;

	bool ufo_v_partition;
	bool ufo_h_partition;
	bool ufo_page0;
	bool one_phase;
	bool outstanding;
};

bool imgresz_src_is_ufo(const enum imgresz_ufo_type ufotype);

#endif
