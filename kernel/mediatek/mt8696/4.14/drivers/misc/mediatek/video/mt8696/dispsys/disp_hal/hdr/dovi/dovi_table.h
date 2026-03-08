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

#ifndef _DOVI_TABLE_H_
#define _DOVI_TABLE_H_

#include "hdmitx.h"
#include "disp_dovi_common_if.h"

#define DOVI_GFX_FE_REG_NUM (0x200/4)
#define DOVI_BE_REG_NUM (0x200/4)

#define MAX_FILE_NAME_LEN 70
#define MAX_DOVI_TEST_CASE_ID 250
#define MID_DOVI_TEST_CASE_ID 50

#define OUT_FORMAT_DOVI 0
#define OUT_FORMAT_HDR10 1
#define OUT_FORMAT_SDR8 2

//not need
struct dovi2hdr10_out_map_table_t {
	uint32_t test_case_id;
	uint32_t dovi2hdr10_mapping;
};
struct dovi_graphic_info_table_t {
	uint32_t test_case_id;
	char graphic_file_name[MAX_FILE_NAME_LEN];
	int f_graphic_on;
	enum pri_mode_t priority_mode;
	enum graphic_format_t g_format;
};

struct dobly_resolution {
	uint16_t htotal;
	uint16_t vtotal;
	uint16_t width;
	uint16_t height;
	uint16_t frequency;
	bool is_progressive;
	bool is_hd;
	enum HDMI_VIDEO_RESOLUTION res_mode;
};

struct dovi_timing_info {
	uint32_t sync_width;
	uint32_t x_zone;
	uint32_t y_zone;
	uint32_t sync_delay;
};

enum disp_mode_t {
	NO_VIDEO = 0,
	NORMAL_MODE = 1,
	PIP_MODE = 2,
	SBS_MODE = 3,
};

enum vdo_inout_type_t {
	VDO_TYPE_OTT = 0,
	VDO_TYPE_HDMI1P4 = 1,
	VDO_TYPE_HDMI2P0 = 2,
	VDO_TYPE_HDMI2P1 = 3,
};

enum output_css_t {
	OUT_CSS_UYVY = 0,
	OUT_CSS_P444 = 1,
};

enum input_format_t {
	FORMAT_SDR = 0,
	FORMAT_HDR = 1,
	FORMAT_HLG = 2,
	FORMAT_DOVI_LL = 3,
	FORMAT_DOVI = 4,
};

enum input_BD_t {
	BIT_DEPTH_8 = 0,
	BIT_DEPTH_10 = 1,
	BIT_DEPTH_12 = 2,
};

enum input_css_t {
	CSS_P420 = 0,
	CSS_I444 = 1,
	CSS_UYVY = 2,
};

struct dovi_out_format_table_t {
	uint32_t test_case_id;
	uint32_t out_format;
};

struct dovi_case_info_t {
	uint32_t test_case_id;
	char vsvdb_file_name[MAX_FILE_NAME_LEN];
	uint32_t use_ll;
	uint32_t ll_rgb_desired;
};

struct dovi_mode_priority_t {
	uint32_t test_case_id;
	enum disp_mode_t disp_mode;
	enum pri_mode_t priority_mode;
};

struct dovi_vdo_inout_type_t {
	uint32_t test_case_id;
	enum vdo_inout_type_t vdo_in_type;
	enum vdo_inout_type_t vdo_out_type;
};

struct dovi_drm_type_t {
	uint32_t test_case_id;
	enum vdo_inout_type_t vdo_in_type;
	char drm_file_name[MAX_FILE_NAME_LEN];
};

struct dovi_vsif_type_t {
	uint32_t test_case_id;
	enum vdo_inout_type_t vdo_in_type;
	char vsif_file_name[MAX_FILE_NAME_LEN];
};

struct dovi_vsif_vsem_type_t {
	uint32_t test_case_id;
	enum vdo_inout_type_t vdo_in_type;
	char vsif_file_name[MAX_FILE_NAME_LEN];
	char vsem_file_name[MAX_FILE_NAME_LEN];
};

struct dovi_multi_vsif_t {
	uint32_t test_case_id;
	enum vdo_inout_type_t vdo_in_type1;
	char vsif_file_name1[MAX_FILE_NAME_LEN];
	enum vdo_inout_type_t vdo_in_type2;//gfx input type
	char vsif_file_name2[MAX_FILE_NAME_LEN];//gfx input type
};

struct dovi_rpu_type_t {
	uint32_t test_case_id;
	char rpu_file_name[MAX_FILE_NAME_LEN];
};

struct dovi_output_css_t {
	uint32_t test_case_id;
	enum chroma_format_t out_css;
};

struct dovi_gfx_info_table_t {
	uint32_t test_case_id;
	bool is_graphic_on;
	char graphic_file_name[MAX_FILE_NAME_LEN];
};

struct dovi_only_gfx_info_t {
	uint32_t test_case_id;
	bool is_only_gfx;
	char gfx_file_name[MAX_FILE_NAME_LEN];
	enum input_format_t input_format;
	enum input_BD_t input_BD; //bit depth
	enum input_css_t in_css;
};

struct dovi_pip_gfx_info_t {
	uint32_t test_case_id;
	char gfx_file_name[MAX_FILE_NAME_LEN];
	//char alpha_file_name[MAX_FILE_NAME_LEN];
	enum input_format_t input_format;
	char md_file_name[MAX_FILE_NAME_LEN];
};


/* lk for test */
extern uint32_t sub_sdr_ll_lut[];
extern uint32_t sub_sdr_sdr_lut[];
extern uint32_t sub_sdr_ipt_lut[];
extern uint32_t sub_sdr_hdr10_lut[];
extern uint32_t dovi_sdr_ipt_be_lut[];
extern uint32_t dovi_sdr_sdr_gfx_fe_lut[];
extern uint32_t dovi_sdr_hdr10_gfx_fe_lut[];
extern uint32_t dovi_sdr_hlg_gfx_fe_lut[];
extern uint32_t dovi_sdr_ll_gfx_fe_lut[];
extern uint32_t dovi_sdr_ipt_gfx_fe_lut[];
extern uint32_t dovi_sdr_vsem_gfx_fe_lut[];
extern uint32_t dovi_sdr_vsemll_gfx_fe_lut[];

extern uint32_t dovi_v26_1080p_sdr_all_gfxfe_reg[];
extern uint32_t dovi_v26_1080p_sdr_ipt_be_reg[];
extern uint32_t dovi_v26_1080p_sdr_ll_be_reg[];
extern uint32_t dovi_v26_1080p_sdr_hdr10_be_reg[];
extern uint32_t dovi_v26_1080p_sdr_hlg_be_reg[];
extern uint32_t dovi_v26_1080p_sdr_sdr_be_reg[];
extern uint32_t dovi_v26_1080p_sdr_vsem_be_reg[];
extern uint32_t dovi_v26_1080p_sdr_vsemll_be_reg[];



extern uint32_t dv_480p_sdr_ipt_mmsys_reg_tbl[];
extern uint32_t dv_720p_sdr_ipt_mmsys_reg_tbl[];
extern uint32_t dv_1080p_sdr_ipt_mmsys_reg_tbl[];
extern uint32_t dv_2160p_sdr_ipt_mmsys_reg_tbl[];

extern uint32_t dv_480p_sdr_ll_mmsys_reg_tbl[];
extern uint32_t dv_720p_sdr_ll_mmsys_reg_tbl[];
extern uint32_t dv_1080p_sdr_ll_mmsys_reg_tbl[];
extern uint32_t dv_2160p_sdr_ll_mmsys_reg_tbl[];

extern uint32_t dv_480p_sdr_hdr_mmsys_reg_tbl[];
extern uint32_t dv_720p_sdr_hdr_mmsys_reg_tbl[];
extern uint32_t dv_1080p_sdr_hdr_mmsys_reg_tbl[];
extern uint32_t dv_2160p_sdr_hdr_mmsys_reg_tbl[];

extern uint32_t dv_480p_sdr_sdr_mmsys_reg_tbl[];
extern uint32_t dv_720p_sdr_sdr_mmsys_reg_tbl[];
extern uint32_t dv_1080p_sdr_sdr_mmsys_reg_tbl[];
extern uint32_t dv_2160p_sdr_sdr_mmsys_reg_tbl[];

//not need
extern struct dobly_resolution dobly_resolution_table[];
extern const char *dobly_resstr[HDMI_VIDEO_RESOLUTION_NUM];
extern char dovi_graphic_name[][MAX_FILE_NAME_LEN];
extern uint32_t get_dovi2hdr10_mapping_type(uint32_t test_case_id);
extern uint32_t get_dovi2hdr10_mapping_type_Status(void);
extern uint32_t set_dovi2hdr10_mapping_type(uint32_t test_case_id,
	uint32_t dovi2hdr10_mapping);
extern int get_dovi_graphic_on(uint32_t test_case_id);
extern enum graphic_format_t get_dovi_g_format(uint32_t test_case_id);

extern uint32_t get_dovi_out_format(uint32_t test_case_id);
extern uint32_t get_dovi_out_format_status(void);
extern uint32_t set_dovi_out_format(uint32_t test_case_id,
	uint32_t out_format);
extern char *get_dovi_vsvdb_file_name(uint32_t test_case_id);
extern uint32_t get_dovi_use_ll(uint32_t test_case_id);
extern uint32_t get_dovi_ll_rgb_desired(uint32_t test_case_id);
extern uint32_t set_dovi_ll_mode(uint32_t test_case_id, uint32_t use_ll,
	uint32_t ll_rgb_desired);
extern uint32_t get_dovi_ll_mode_status(void);
extern enum pri_mode_t get_dovi_priority_mode(uint32_t test_case_id);
extern void set_dovi_priority_mode(uint32_t force_pri_mode,
	uint32_t pri_mode);
extern void set_dovi_g_format(uint32_t force_gformat, uint32_t gformat);
extern enum disp_mode_t get_dovi_disp_mode(uint32_t test_case_id);
extern enum vdo_inout_type_t get_dovi_input_type(uint32_t test_case_id);
extern enum vdo_inout_type_t get_dovi_output_type(uint32_t test_case_id);
extern char *get_dovi_drm_file_name(uint32_t test_case_id);
extern char *get_dovi_vsif_file_name(uint32_t test_case_id);
extern char *get_dovi_vsif_vsem_name1(uint32_t test_case_id);
extern char *get_dovi_vsif_vsem_name2(uint32_t test_case_id);
extern enum vdo_inout_type_t get_multi_input_type1(uint32_t test_case_id);
extern enum vdo_inout_type_t get_multi_input_type2(uint32_t test_case_id);
extern char *get_dovi_multi_vsif1(uint32_t test_case_id);
extern char *get_dovi_multi_vsif2(uint32_t test_case_id);
extern char *get_dovi_rpu_name(uint32_t test_case_id);
extern enum chroma_format_t get_dovi_out_css(uint32_t test_case_id);
extern int get_dovi_gfx_on_info(uint32_t test_case_id);
extern char *get_dovi_gfx_file_info(uint32_t test_case_id);
extern int get_dovi_is_only_gfx_test(uint32_t test_case_id);
extern char *get_dovi_only_gfx_file_info(uint32_t test_case_id);
extern enum input_format_t get_dovi_only_gfx_input_format(
	uint32_t test_case_id);
extern enum input_BD_t get_dovi_only_gfx_input_bd(uint32_t test_case_id);
extern enum input_css_t get_dovi_only_gfx_in_css(uint32_t test_case_id);
extern char *get_dovi_pip_gfx1_gfx_file(uint32_t test_case_id);
extern char *get_dovi_pip_gfx1_md(uint32_t test_case_id);
extern enum input_format_t get_dovi_pip_gfx1_input_format(
	uint32_t test_case_id);
extern char *get_dovi_pip_gfx1_md_file(uint32_t test_case_id);
extern char *get_dovi_pip_gfx2_gfx_file(uint32_t test_case_id);
extern enum input_format_t get_dovi_pip_gfx2_input_format(
	uint32_t test_case_id);
extern char *get_dovi_pip_gfx2_md_file(uint32_t test_case_id);
#endif
