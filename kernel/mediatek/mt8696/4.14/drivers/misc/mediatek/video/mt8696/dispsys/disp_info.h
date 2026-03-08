/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __DISP_INFO_H__
#define __DISP_INFO_H__

#include "hdmitx.h"
#include "linux/types.h"

#ifndef DOVI_MD_MAX_LEN
#define DOVI_MD_MAX_LEN 1024
#endif

#define DISP_BUFFER_MAX 4
#define MAX_PIVOT       9
#define MAX_POLY_ORDER  2
#define MAX_PRIMARY_INDEX 18
#define MAX_DM_EXT_BLOCKS 255
#define MAX_UNKNOWN_MD_SIZE 256
#define CONFIG_DOVI_SUPPORT 1


enum MTK_DISPIF_TYPE {
	DISPIF_TYPE_DBI,
	DISPIF_TYPE_DPI,
	DISPIF_TYPE_DSI,
	DISPIF_TYPE_DPI0,
	DISPIF_TYPE_DPI1,
	DISPIF_TYPE_DSI0,
	DISPIF_TYPE_DSI1,
	HDMI,
	HDMI_SMARTBOOK,
	MHL,
	DISPIF_TYPE_CVBS,
	SLIMPORT
};

enum MTK_DISPIF_DEVICE_TYPE {
	MTK_DISPIF_PRIMARY_LCD = 0,
	MTK_DISPIF_HDMI,
	MTK_DISPIF_CVBS,
	MTK_MAX_DISPLAY_COUNT
};

enum MTK_HDR_TYPE_FLAG {
	MTK_HDR_TYPE_DOVI = 1 << 0,
	MTK_HDR_TYPE_HDR10 = 1 << 1,
	MTK_HDR_TYPE_HLG = 1 << 2
};

struct mtk_disp_info {
	uint32_t display_id;
	uint32_t isHwVsyncAvailable;
	enum MTK_DISPIF_TYPE displayType;
	uint32_t displayWidth;
	uint32_t displayHeight;
	uint32_t displayFormat;
	uint32_t vsyncFPS;
	uint32_t physicalWidth;
	uint32_t physicalHeight;
	uint32_t isConnected;
	uint32_t lcmOriginalWidth;
	uint32_t lcmOriginalHeight;
	uint32_t disp_resolution;
};

struct mtk_disp_hdr_info_t {
	uint32_t colorPrimaries;       /* colour_primaries emum */
	uint32_t transformCharacter;   /* transfer_characteristics emum */
	uint32_t matrixCoeffs;	 /* matrix_coeffs emum */
	uint32_t displayPrimariesX[3]; /* display_primaries_x */
	uint32_t displayPrimariesY[3]; /* display_primaries_y */
	uint32_t whitePointX;	  /* white_point_x */
	uint32_t whitePointY;	  /* white_point_y */
	/* max_display_mastering_luminance */
	uint32_t maxDisplayMasteringLuminance;
	/* min_display_mastering_luminance*/
	uint32_t minDisplayMasteringLuminance;
	uint32_t maxCll;
	uint32_t maxFall;
};

enum DISP_LAYER_TYPE {
	DISP_LAYER_VDP,
	DISP_LAYER_OSD,
	DISP_LAYER_SIDEBAND,
	DISP_LAYER_AEE,
	DISP_LAYER_UNKNOWN
};

enum DISP_BUFFER_SOURCE {
	/* ion buffer */
	DISP_BUFFER_ION = 0,
	/* dim layer, const alpha */
	DISP_BUFFER_ALPHA = 1,
	/* mva buffer */
	DISP_BUFFER_MVA = 2,
};

enum DISP_MGR_USER {
	DISP_USER_HWC,
	DISP_USER_AVSYNC,
	DISP_USER_AEE,
	DISP_USER_PANDISP,
	DISP_USER_INVALID
};

#define MAKE_DISP_HW_FORMAT_ID(id, bpp) (((id) << 8) | (bpp))

enum DISP_HW_COLOR_FORMAT {
	DISP_HW_COLOR_FORMAT_UNKNOWN = 0,
	DISP_HW_COLOR_FORMAT_RGB565 = MAKE_DISP_HW_FORMAT_ID(1, 0),
	DISP_HW_COLOR_FORMAT_RGB888 = MAKE_DISP_HW_FORMAT_ID(1, 1),
	DISP_HW_COLOR_FORMAT_BGR888 = MAKE_DISP_HW_FORMAT_ID(1, 2),
	DISP_HW_COLOR_FORMAT_ARGB8888 = MAKE_DISP_HW_FORMAT_ID(1, 3),
	DISP_HW_COLOR_FORMAT_ABGR8888 = MAKE_DISP_HW_FORMAT_ID(1, 4),
	DISP_HW_COLOR_FORMAT_RGBA8888 = MAKE_DISP_HW_FORMAT_ID(1, 5),
	DISP_HW_COLOR_FORMAT_BGRA8888 = MAKE_DISP_HW_FORMAT_ID(1, 6),
	DISP_HW_COLOR_FORMAT_YUV420_BLOCK = MAKE_DISP_HW_FORMAT_ID(2, 0),
	DISP_HW_COLOR_FORMAT_YUV422_BLOCK = MAKE_DISP_HW_FORMAT_ID(2, 1),
	DISP_HW_COLOR_FORMAT_YUV420_RASTER = MAKE_DISP_HW_FORMAT_ID(2, 2),
	DISP_HW_COLOR_FORMAT_YUV422_RASTER = MAKE_DISP_HW_FORMAT_ID(2, 3),
	DISP_HW_COLOR_FORMAT_BPP_MASK = 0xFFFF,
};

struct mtk_disp_range {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
};

enum DISP_VIDEO_TYPE {
	VIDEO_YUV_BT601_FULL = 0,
	VIDEO_YUV_BT601 = 1,
	VIDEO_YUV_BT709 = 2
};

enum DISP_ASPECT_RATIO { DISP_ASPECT_RATIO_4_3 = 0, DISP_ASPECT_RATIO_16_9 };

enum DISP_ALPHA_TYPE {
	DISP_ALPHA_ONE = 0,
	DISP_ALPHA_SRC = 1,
	DISP_ALPHA_SRC_INVERT = 2,
	DISP_ALPHA_INVALID = 3,
};

enum VIDEO_LAYER_ID {
	DISP_MAIN_VIDEO = 0,
	DISP_SUB_VIDEO = 1,
	DISP_MAX_VIDEO = 2
};

enum DISP_DR_TYPE_T {
	DISP_DR_TYPE_SDR = 0,
	DISP_DR_TYPE_HDR10,
	DISP_DR_TYPE_DOVI,
	DISP_DR_TYPE_PHLP,
	DISP_DR_TYPE_HLG,
	DISP_DR_TYPE_PHLP_RESVERD
};

struct mtk_disp_hdr10_md_t {
	uint32_t ui2_DisplayPrimariesX[3];
	uint32_t ui2_DisplayPrimariesY[3];
	uint32_t ui2_WhitePointX;
	uint32_t ui2_WhitePointY;
	uint32_t ui2_MaxDisplayMasteringLuminance;
	uint32_t ui2_MinDisplayMasteringLuminance;
	uint32_t ui2_MaxCLL;
	uint32_t ui2_MaxFALL;
};

struct rpu_ext_config_fixpt_main_t {
	uint32_t rpu_VDR_bit_depth;
	uint32_t rpu_BL_bit_depth;
	uint32_t rpu_EL_bit_depth;
	uint32_t coefficient_log2_denom;
	uint32_t num_pivots[3];
	uint32_t pivot_value[3][MAX_PIVOT];
	uint32_t mapping_idc[3];
	uint32_t poly_order[3][MAX_PIVOT-1];
	int poly_coef_int[3][MAX_PIVOT-1][MAX_POLY_ORDER+1];
	uint32_t poly_coef[3][MAX_PIVOT-1][MAX_POLY_ORDER+1];
	uint32_t MMR_order[2];
	int MMR_coef_int[2][22];
	uint32_t MMR_coef[2][22];
	unsigned char NLQ_method_idc;
	unsigned char disable_residual_flag;
	unsigned char el_spatial_resampling_filter_flag;
	unsigned char BL_video_full_range_flag;
	uint32_t NLQ_offset[3];
	int NLQ_coeff_int[3][3];
	uint32_t NLQ_coeff[3][3];
	uint32_t spatial_resampling_filter_flag;
	uint32_t spatial_resampling_explicit_filter_flag;
	uint32_t spatial_filter_exp_coef_log2_denom;
	uint32_t spatial_resampling_mode_hor_idc;
	uint32_t spatial_resampling_mode_ver_idc;
	uint32_t spatial_resampling_filter_hor_idc[3];
	uint32_t spatial_resampling_filter_ver_idc[3];
	uint32_t spatial_resampling_luma_pivot[2];
	int spatial_filter_coeff_hor_int[3][8];
	uint32_t spatial_filter_coeff_hor[3][8];
	int spatial_filter_coeff_ver_int[2][3][6];
	uint32_t spatial_filter_coeff_ver[2][3][6];
};

struct dm_metadata_base_t {
	/* signal attributes */
	/* affected_dm_metadata_id<<4|current_dm_metadata_id */
	unsigned char dm_metadata_id;
	unsigned char scene_refresh_flag;
	unsigned char YCCtoRGB_coef0_hi;
	unsigned char YCCtoRGB_coef0_lo;
	unsigned char YCCtoRGB_coef1_hi;
	unsigned char YCCtoRGB_coef1_lo;
	unsigned char YCCtoRGB_coef2_hi;
	unsigned char YCCtoRGB_coef2_lo;
	unsigned char YCCtoRGB_coef3_hi;
	unsigned char YCCtoRGB_coef3_lo;
	unsigned char YCCtoRGB_coef4_hi;
	unsigned char YCCtoRGB_coef4_lo;
	unsigned char YCCtoRGB_coef5_hi;
	unsigned char YCCtoRGB_coef5_lo;
	unsigned char YCCtoRGB_coef6_hi;
	unsigned char YCCtoRGB_coef6_lo;
	unsigned char YCCtoRGB_coef7_hi;
	unsigned char YCCtoRGB_coef7_lo;
	unsigned char YCCtoRGB_coef8_hi;
	unsigned char YCCtoRGB_coef8_lo;
	unsigned char YCCtoRGB_offset0_byte3;
	unsigned char YCCtoRGB_offset0_byte2;
	unsigned char YCCtoRGB_offset0_byte1;
	unsigned char YCCtoRGB_offset0_byte0;
	unsigned char YCCtoRGB_offset1_byte3;
	unsigned char YCCtoRGB_offset1_byte2;
	unsigned char YCCtoRGB_offset1_byte1;
	unsigned char YCCtoRGB_offset1_byte0;
	unsigned char YCCtoRGB_offset2_byte3;
	unsigned char YCCtoRGB_offset2_byte2;
	unsigned char YCCtoRGB_offset2_byte1;
	unsigned char YCCtoRGB_offset2_byte0;
	unsigned char RGBtoLMS_coef0_hi;
	unsigned char RGBtoLMS_coef0_lo;
	unsigned char RGBtoLMS_coef1_hi;
	unsigned char RGBtoLMS_coef1_lo;
	unsigned char RGBtoLMS_coef2_hi;
	unsigned char RGBtoLMS_coef2_lo;
	unsigned char RGBtoLMS_coef3_hi;
	unsigned char RGBtoLMS_coef3_lo;
	unsigned char RGBtoLMS_coef4_hi;
	unsigned char RGBtoLMS_coef4_lo;
	unsigned char RGBtoLMS_coef5_hi;
	unsigned char RGBtoLMS_coef5_lo;
	unsigned char RGBtoLMS_coef6_hi;
	unsigned char RGBtoLMS_coef6_lo;
	unsigned char RGBtoLMS_coef7_hi;
	unsigned char RGBtoLMS_coef7_lo;
	unsigned char RGBtoLMS_coef8_hi;
	unsigned char RGBtoLMS_coef8_lo;
	unsigned char signal_eotf_hi;
	unsigned char signal_eotf_lo;
	unsigned char signal_eotf_param0_hi;
	unsigned char signal_eotf_param0_lo;
	unsigned char signal_eotf_param1_hi;
	unsigned char signal_eotf_param1_lo;
	unsigned char signal_eotf_param2_byte3;
	unsigned char signal_eotf_param2_byte2;
	unsigned char signal_eotf_param2_byte1;
	unsigned char signal_eotf_param2_byte0;
	unsigned char signal_bit_depth;
	unsigned char signal_color_space;
	unsigned char signal_chroma_format;
	unsigned char signal_full_range_flag;
	/* source display attributes */
	unsigned char source_min_PQ_hi;
	unsigned char source_min_PQ_lo;
	unsigned char source_max_PQ_hi;
	unsigned char source_max_PQ_lo;
	unsigned char source_diagonal_hi;
	unsigned char source_diagonal_lo;
	/* extended metadata */
	unsigned char num_ext_blocks;
};

struct ext_level_1_t {
	unsigned char min_PQ_hi;
	unsigned char min_PQ_lo;
	unsigned char max_PQ_hi;
	unsigned char max_PQ_lo;
	unsigned char avg_PQ_hi;
	unsigned char avg_PQ_lo;
};

struct ext_level_2_t {
	unsigned char target_max_PQ_hi;
	unsigned char target_max_PQ_lo;
	unsigned char trim_slope_hi;
	unsigned char trim_slope_lo;
	unsigned char trim_offset_hi;
	unsigned char trim_offset_lo;
	unsigned char trim_power_hi;
	unsigned char trim_power_lo;
	unsigned char trim_chroma_weight_hi;
	unsigned char trim_chroma_weight_lo;
	unsigned char trim_saturation_gain_hi;
	unsigned char trim_saturation_gain_lo;
	unsigned char ms_weight_hi;
	unsigned char ms_weight_lo;
};

struct ext_level_3_t {
	unsigned char min_PQ_offset_hi;
	unsigned char min_PQ_offset_lo;
	unsigned char max_PQ_offset_hi;
	unsigned char max_PQ_offset_lo;
	unsigned char avg_PQ_offset_hi;
	unsigned char avg_PQ_offset_lo;
};

struct ext_level_4_t {
	unsigned char anchor_PQ_hi;
	unsigned char anchor_PQ_lo;
	unsigned char anchor_power_hi;
	unsigned char anchor_power_lo;
};

struct ext_level_5_t {
	unsigned char active_area_left_offset_hi;
	unsigned char active_area_left_offset_lo;
	unsigned char active_area_right_offset_hi;
	unsigned char active_area_right_offset_lo;
	unsigned char active_area_top_offset_hi;
	unsigned char active_area_top_offset_lo;
	unsigned char active_area_bottom_offset_hi;
	unsigned char active_area_bottom_offset_lo;
};

struct ext_level_6_t {
	unsigned char max_display_mastering_luminance_hi;
	unsigned char max_display_mastering_luminance_lo;
	unsigned char min_display_mastering_luminance_hi;
	unsigned char min_display_mastering_luminance_lo;
	unsigned char max_content_light_level_hi;
	unsigned char max_content_light_level_lo;
	unsigned char max_frame_average_light_level_hi;
	unsigned char max_frame_average_light_level_lo;
};

struct ext_level_8_t {
	unsigned char target_display_index;
	unsigned char trim_slope_hi;
	unsigned char trim_slope_lo;
	unsigned char trim_offset_hi;
	unsigned char trim_offset_lo;
	unsigned char trim_power_hi;
	unsigned char trim_power_lo;
	unsigned char trim_chroma_weight_hi;
	unsigned char trim_chroma_weight_lo;
	unsigned char trim_saturation_gain_hi;
	unsigned char trim_saturation_gain_lo;
	unsigned char ms_weight_hi;
	unsigned char ms_weight_lo;
	unsigned char target_mid_contrast_hi;
	unsigned char target_mid_contrast_lo;
	unsigned char clip_trim_hi;
	unsigned char clip_trim_lo;
	unsigned char saturation_vector_field0;
	unsigned char saturation_vector_field1;
	unsigned char saturation_vector_field2;
	unsigned char saturation_vector_field3;
	unsigned char saturation_vector_field4;
	unsigned char saturation_vector_field5;
	unsigned char hue_vector_field0;
	unsigned char hue_vector_field1;
	unsigned char hue_vector_field2;
	unsigned char hue_vector_field3;
	unsigned char hue_vector_field4;
	unsigned char hue_vector_field5;
};

struct ext_level_9_t {
	unsigned char source_primary_index;
	unsigned char source_primary_red_x_hi;
	unsigned char source_primary_red_x_lo;
	unsigned char source_primary_red_y_hi;
	unsigned char source_primary_red_y_lo;
	unsigned char source_primary_green_x_hi;
	unsigned char source_primary_green_x_lo;
	unsigned char source_primary_green_y_hi;
	unsigned char source_primary_green_y_lo;
	unsigned char source_primary_blue_x_hi;
	unsigned char source_primary_blue_x_lo;
	unsigned char source_primary_blue_y_hi;
	unsigned char source_primary_blue_y_lo;
	unsigned char source_primary_white_x_hi;
	unsigned char source_primary_white_x_lo;
	unsigned char source_primary_white_y_hi;
	unsigned char source_primary_white_y_lo;
};

struct ext_level_10_t {
	unsigned char target_display_index;
	unsigned char target_max_PQ_hi;
	unsigned char target_max_PQ_lo;
	unsigned char target_min_PQ_hi;
	unsigned char target_min_PQ_lo;
	unsigned char target_primary_index;
	unsigned char target_primary_red_x_hi;
	unsigned char target_primary_red_x_lo;
	unsigned char target_primary_red_y_hi;
	unsigned char target_primary_red_y_lo;
	unsigned char target_primary_green_x_hi;
	unsigned char target_primary_green_x_lo;
	unsigned char target_primary_green_y_hi;
	unsigned char target_primary_green_y_lo;
	unsigned char target_primary_blue_x_hi;
	unsigned char target_primary_blue_x_lo;
	unsigned char target_primary_blue_y_hi;
	unsigned char target_primary_blue_y_lo;
	unsigned char target_primary_white_x_hi;
	unsigned char target_primary_white_x_lo;
	unsigned char target_primary_white_y_hi;
	unsigned char target_primary_white_y_lo;
};


struct ext_level_11_t {
	unsigned char content_type;
	unsigned char white_point;
	unsigned char byte2;
	unsigned char byte3;
};

struct ext_level_254_t {
	unsigned char dm_mode;
	unsigned char dm_version_index;
};

struct ext_level_255_t {
	unsigned char dm_run_mode;
	unsigned char dm_run_version;
	unsigned char dm_debug0;
	unsigned char dm_debug1;
	unsigned char dm_debug2;
	unsigned char dm_debug3;
};

struct ext_level_unknown_t {
	unsigned char buffer[MAX_UNKNOWN_MD_SIZE];
};

struct dm_metadata_ext_t {
	unsigned char ext_block_length_byte3;
	unsigned char ext_block_length_byte2;
	unsigned char ext_block_length_byte1;
	unsigned char ext_block_length_byte0;
	unsigned char ext_block_level;
	union {
		struct ext_level_1_t level_1;
		struct ext_level_2_t level_2;
		struct ext_level_3_t level_3;
		struct ext_level_4_t level_4;
		struct ext_level_5_t level_5;
		struct ext_level_6_t level_6;
		struct ext_level_8_t level_8;
		struct ext_level_9_t level_9;
		struct ext_level_10_t level_10;
		struct ext_level_11_t level_11;
		struct ext_level_254_t level_254;
		struct ext_level_255_t level_255;
		struct ext_level_unknown_t level_unknown;
	} l;
};


struct dm_metadata_t {
	struct dm_metadata_base_t base;
	struct dm_metadata_ext_t ext[MAX_DM_EXT_BLOCKS];
};

struct mtk_disp_dovi_md_t {
	uint64_t pts;
	uint32_t len;
	bool svp;
	uint32_t sec_handle;
	void *addr;
	int fd;
	uint32_t offset;
	//struct dm_metadata_t *dm_md;
	//struct rpu_ext_config_fixpt_main_t *comp_md;
};

struct mtk_disp_film_grain_md_t {
	uint64_t pts;
	uint32_t len;
	bool svp;
	uint32_t sec_handle;
	void *addr;
	int fd;
	uint32_t offset;
};

struct mtk_vdp_film_gain_md_t {
	uint64_t pts;
	uint32_t len;
	uint32_t len_from_tz;
	bool svp;
	uint32_t sec_handle;
	uint32_t sec_new_handle_from_tz;
	uint8_t buff[DOVI_MD_MAX_LEN];
};

struct mtk_vdp_dovi_md_t {
	uint64_t pts;
	uint32_t len;
	uint32_t len_from_tz;
	bool svp;
	uint32_t sec_handle;
	uint32_t sec_new_handle_from_tz;
	uint8_t buff[DOVI_MD_MAX_LEN];
	struct dm_metadata_t dm_md;
	struct rpu_ext_config_fixpt_main_t comp_md;
};

struct mtk_vdp_hdr10_plus_svp_handle_t {
	uint32_t layer_id;
	uint32_t len;
	uint32_t len_from_tz;
	uint32_t sec_handle;
	uint64_t pts;
	uint32_t sec_new_handle_from_tz;
};

union mtk_disp_hdr_md_t {
	struct mtk_disp_hdr10_md_t hdr10_metadata;
	struct mtk_vdp_dovi_md_t dovi_metadata;
};

struct mtk_disp_hdr_md_info_t {
	enum DISP_DR_TYPE_T dr_range;
	union mtk_disp_hdr_md_t metadata_info;
	uint32_t enable;
	struct VID_PLA_HDR_METADATA_INFO_T hdr10_info;
};


#define AV1_MAX_GRAIN_POINT_CNT							16
#define AV1_MAX_AR_COEFFS_CNT							25

struct mtk_av1_film_grain_params {
	unsigned char apply_grain;
	unsigned int grain_seed;
	unsigned char update_grain;
	unsigned char film_grain_params_ref_idx;
	unsigned char num_y_points;
	unsigned char point_y_value[AV1_MAX_GRAIN_POINT_CNT];
	unsigned char point_y_scaling[AV1_MAX_GRAIN_POINT_CNT];
	unsigned char chroma_scaling_from_luma;
	unsigned char num_cb_points;
	unsigned char point_cb_value[AV1_MAX_GRAIN_POINT_CNT];
	unsigned char point_cb_scaling[AV1_MAX_GRAIN_POINT_CNT];
	unsigned char num_cr_points;
	unsigned char point_cr_value[AV1_MAX_GRAIN_POINT_CNT];
	unsigned char point_cr_scaling[AV1_MAX_GRAIN_POINT_CNT];
	unsigned char grain_scaling;
	unsigned char ar_coeff_lag;
	int ar_coeffs_y[AV1_MAX_AR_COEFFS_CNT];
	int ar_coeffs_cb[AV1_MAX_AR_COEFFS_CNT];
	int ar_coeffs_cr[AV1_MAX_AR_COEFFS_CNT];
	unsigned char ar_coeff_shift;
	unsigned char grain_scale_shift;
	unsigned short cb_mult;
	unsigned short cb_luma_mult;
	unsigned short cb_offset;
	unsigned short cr_mult;
	unsigned short cr_luma_mult;
	unsigned short cr_offset;
	unsigned char overlap_flag;
	unsigned char clip_to_restricted_range;
};

struct mtk_disp_buffer {
	uint32_t layer_order;
	uint32_t layer_id;
	uint32_t layer_enable;

	enum DISP_LAYER_TYPE type;
	enum DISP_BUFFER_SOURCE buffer_source;
	enum DISP_HW_COLOR_FORMAT src_fmt;

	bool color_key_en;
	int src_color_key;

	bool alpha_en;
	int alpha;

	/* display region */
	struct mtk_disp_range src;
	struct mtk_disp_range crop;
	struct mtk_disp_range tgt;

	/* buffer address and fence */
	int ion_fd;
	bool secruity_en;
	int acquire_fence_fd;
	int release_fence_fd;
	int present_fence_fd;

	/* for pre-multiple alpha */
	enum DISP_ALPHA_TYPE src_alpha;
	enum DISP_ALPHA_TYPE dst_alpha;

	/* if buffer_info is osd plane, should fill osd info*/
	bool is_pvric;

	/* if buffer_info is video plane, should fill video info */
	bool is_ufo;
	bool is_dovi;
	bool is_progressive;
	bool is_10bit;
	bool is_10bit_lbs2bit_tile_mode;
	bool is_hdr;
	bool is_hdr10plus;
	bool is_film_grain;
	bool is_pack_mode;
	bool is_bt2020;
	bool is_seamless;
	bool is_jumpmode;
	bool is_metadata_async;
	uint64_t pts;
	uint32_t fps;
	enum DISP_ASPECT_RATIO aspect_ratio;
	enum DISP_VIDEO_TYPE video_type;
	struct mtk_disp_hdr_info_t hdr_info;
	enum DISP_DR_TYPE_T dr_range;

	/*video buffer offset*/
	uint32_t ofst_y;
	uint32_t ofst_c;
	uint32_t ofst_c_len;
	uint32_t ofst_y_len;
	uint32_t buffer_size;

	uint32_t meta_data_size;
	struct mtk_disp_dovi_md_t dovi_info;

	struct mtk_disp_film_grain_md_t film_grain_info;

	void *meta_data;

	void *src_base_addr;
	void *src_phy_addr;
	uint32_t res_mode;
	/*hdmi ALLM enable flag*/
	bool allm_en;
};

struct mtk_disp_config {
	enum DISP_MGR_USER user;
	uint32_t buffer_num;
	struct mtk_disp_buffer buffer_info[DISP_BUFFER_MAX];
};

struct mtk_disp_vdp_cap {
	struct mtk_disp_range src;
	struct mtk_disp_range crop;
	struct mtk_disp_range tgt;
	enum DISP_HW_COLOR_FORMAT src_fmt;
	bool is_progressive;
	bool is_10bit;
	bool need_resizer;
	uint32_t layer_id;
};

struct mtk_disp_hdmi_cap {
	uint32_t hdr_type;
	uint32_t hdr_content_max_luminance;
	uint32_t hdr_content_max_frame_average_luminance;
	uint32_t hdr_content_min_luminance;
	long long supported_resolution;
	uint32_t disp_resolution;
	unsigned char screen_width;
	unsigned char screen_height;
	bool is_support_bt2020;
};

struct mtk_disp_layer_info {
	uint32_t display_id;
	uint32_t supported_ui_num;
	uint32_t supported_video_num;
};

#define MTK_DISP_IOW(num, dtype) _IOW('O', num, dtype)
#define MTK_DISP_IOR(num, dtype) _IOR('O', num, dtype)
#define MTK_DISP_IOWR(num, dtype) _IOWR('O', num, dtype)
#define MTK_DISP_IO(num) _IO('O', num)

#define MTK_DISP_IOCTL_GET_INFO MTK_DISP_IOR(0xd0, struct mtk_disp_info)
#define MTK_DISP_IOCTL_WAIT_VSYNC MTK_DISP_IO(0xd1)
#define MTK_DISP_IOCTL_SET_INPUT_BUFFER                                        \
	MTK_DISP_IOWR(0xd2, struct mtk_disp_config)
#define MTK_DISP_IOCTL_PLAY MTK_DISP_IO(0xd3)
#define MTK_DISP_IOCTL_STOP MTK_DISP_IO(0xd4)
#define MTK_DISP_IOCTL_SUSPEND MTK_DISP_IO(0xd5)
#define MTK_DISP_IOCTL_RESUME MTK_DISP_IO(0xd6)
#define MTK_DISP_IOCTL_VDP_CAP MTK_DISP_IOWR(0xd7, struct mtk_disp_vdp_cap)
#define MTK_DISP_IOCTL_HDMI_CAP MTK_DISP_IOR(0xd8, struct mtk_disp_hdmi_cap)
#define MTK_DISP_IOCTL_GET_LAYER_INFO                                          \
	MTK_DISP_IOR(0xd9, struct mtk_disp_layer_info)
#endif
