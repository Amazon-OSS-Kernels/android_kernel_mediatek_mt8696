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

#define MAX_FILE_NAME_LEN 70
#define MAX_DOVI_TEST_CASE_ID 150
#define OUT_FORMAT_DOVI 0
#define OUT_FORMAT_HDR10 1
#define OUT_FORMAT_SDR 2

#define DOVI_GFX_FE_REG_NUM (0x200/4)
#define DOVI_BE_REG_NUM (0x200/4)



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

struct dovi_out_format_table_t {
	uint32_t test_case_id;
	uint32_t out_format;
};

struct dovi2hdr10_out_map_table_t {
	uint32_t test_case_id;
	uint32_t dovi2hdr10_mapping;
};


struct dovi_case_info_t {
	uint32_t test_case_id;
	char vsvdb_file_name[MAX_FILE_NAME_LEN];
	uint32_t use_ll;
	uint32_t ll_rgb_desired;
};

struct dovi_graphic_info_table_t {
	uint32_t test_case_id;
	char graphic_file_name[MAX_FILE_NAME_LEN];
	int f_graphic_on;
	enum pri_mode_t priority_mode;
	enum graphic_format_t g_format;
};

/* lk for test */
extern uint32_t sub_sdr_ll_lut[];
extern uint32_t sub_sdr_sdr_lut[];
extern uint32_t sub_sdr_ipt_lut[];
extern uint32_t sub_sdr_hdr10_lut[];
extern uint32_t dovi_sdr_ipt_be_lut[];
extern uint32_t dovi_sdr_sdr_gfx_fe_lut[];
extern uint32_t dovi_sdr_hdr10_gfx_fe_lut[];
extern uint32_t dovi_sdr_ll_gfx_fe_lut[];
extern uint32_t dovi_sdr_ipt_gfx_fe_lut[];

extern uint32_t dovi_v241_1080p_sdr_all_gfxfe_reg[];
extern uint32_t dovi_v241_1080p_sdr_ipt_be_reg[];
extern uint32_t dovi_v241_1080p_sdr_ll_be_reg[];
extern uint32_t dovi_v241_1080p_sdr_hdr10_be_reg[];
extern uint32_t dovi_v241_1080p_sdr_sdr_be_reg[];

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

extern char dovi_graphic_name[][MAX_FILE_NAME_LEN];
extern struct dobly_resolution dobly_resolution_table[];
extern const char *dobly_resstr[HDMI_VIDEO_RESOLUTION_NUM];

extern uint32_t get_dovi_out_format(uint32_t test_case_id);
extern uint32_t get_dovi_out_format_status(void);
extern uint32_t set_dovi_out_format(uint32_t test_case_id,
	uint32_t out_format);
extern char *get_dovi_vsvdb_file_name(uint32_t test_case_id);
extern uint32_t get_dovi2hdr10_mapping_type(uint32_t test_case_id);
extern uint32_t get_dovi2hdr10_mapping_type_Status(void);
extern uint32_t set_dovi2hdr10_mapping_type(uint32_t test_case_id,
	uint32_t dovi2hdr10_mapping);
extern uint32_t get_dovi_use_ll(uint32_t test_case_id);
extern uint32_t get_dovi_ll_rgb_desired(uint32_t test_case_id);
extern uint32_t set_dovi_ll_mode(uint32_t test_case_id, uint32_t use_ll,
	uint32_t ll_rgb_desired);
extern uint32_t get_dovi_ll_mode_status(void);
extern int get_dovi_graphic_on(uint32_t test_case_id);
extern enum pri_mode_t get_dovi_priority_mode(uint32_t test_case_id);
extern enum graphic_format_t get_dovi_g_format(uint32_t test_case_id);
extern void set_dovi_priority_mode(uint32_t force_pri_mode,
	uint32_t pri_mode);
extern void set_dovi_g_format(uint32_t force_gformat, uint32_t gformat);
extern char *get_dovi_graphic_file_name(uint32_t test_case_id);

#endif
