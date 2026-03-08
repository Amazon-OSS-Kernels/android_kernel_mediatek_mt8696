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

#ifndef _DISP_DOVI_COMMON_IF_H_
#define _DISP_DOVI_COMMON_IF_H_

#include "disp_info.h"
#include "disp_hw_mgr.h"
#include "hdmitx.h"
#include "disp_dovi_md_parser.h"

#define MAX_FILENAME_LENGTH (256)
#define MAX_NUM_INPUT (4)

#define DV_SCRAMBLE_ADL_SIZE_MAX  (2048)
#define DV_ADL_SIZE    (1024)
#define DV_REG_NUM (300)

#define DOVI_MD_SIZE    (1024)
#define DOVI_COMP_SIZE    (1800)
#define DOVI_MD_DW_SIZE    (256)
#define DOVI_COMP_DW_SIZE    (450)


#define DV_OUT_LAYER (2)

#define DV_ALL_REG_NUM 2000

#define DV_B0103_ADL_SIZE               (256)
#define DV_B0202_SMLUTS_ADL_SIZE        (512)
#define DV_B0202_SMLUTI_ADL_SIZE        (512)
#define DV_B0202_TMLUTS_ADL_SIZE        (512)
#define DV_B0202_TMLUTI_ADL_SIZE        (512)

#define DV_OSD_B0103_ADL_SIZE           (256)
#define DV_OSD_B0202_SMLUTS_ADL_SIZE    (512)
#define DV_OSD_B0202_SMLUTI_ADL_SIZE    (512)
#define DV_OSD_B0202_TMLUTS_ADL_SIZE    (512)
#define DV_OSD_B0202_TMLUTI_ADL_SIZE    (512)
#define DV_SCRAMBLE_ADL_SIZE            (2048) //16 * 128
#define DV_MD_PER_PKT_SIZE (124) //128- 4 crc bytes

#define L11_MD_PRESENT 1
#define L11_CONTENT_GAME 2
#define L11_WHITE_POINT 8

enum DV_RPU_TYPE {
	HEVC_RPU = 0,
	AV1_RPU = 1,
	ATSC_RPU = 2,
	DVB_RPU  = 3,
};

struct dv_all_reg_tab {
	uint32_t used_depth;
	uint32_t reg_addr[DV_ALL_REG_NUM];
	uint16_t reg_value[DV_ALL_REG_NUM];
	uint16_t reg_mask[DV_ALL_REG_NUM];
};

struct dv_reg_tbl {
	uint32_t used_depth;
	uint32_t reg_addr[DV_REG_NUM];
	uint16_t reg_value[DV_REG_NUM];
	uint16_t reg_mask[DV_REG_NUM];
};

struct dv_adl_tbl {
	uint32_t max_size;
	uint32_t used_size;
	uint16_t client;
	uint8_t  data[DV_ADL_SIZE];
};


struct dv_reg_hw_output {
	struct dv_reg_tbl reg_table;
	struct dv_adl_tbl adl_table;
};

struct dv_vdo_fe_output {
	struct dv_reg_hw_output dv_comp_hw_output;
	struct dv_reg_hw_output dv_ctrl_hw_output;
	struct dv_reg_hw_output dv_vdo_dm_hw_output;
	struct dv_reg_hw_output dv_vdo_lut_hw_output;
	struct dv_reg_hw_output dv_b0103_hw_output;
	struct dv_reg_hw_output dv_b0202_ss_hw_output;
	struct dv_reg_hw_output dv_b0202_si_hw_output;
	struct dv_reg_hw_output dv_b0202_ts_hw_output;
	struct dv_reg_hw_output dv_b0202_ti_hw_output;
};
struct dv_gfx_fe_output {
	struct dv_reg_hw_output dv_gop_hw_output;
	struct dv_reg_hw_output dv_osd_lut_hw_output;
	struct dv_reg_hw_output dv_osd_b0103_hw_output;
	struct dv_reg_hw_output dv_osd_b0202_ss_hw_output;
	struct dv_reg_hw_output dv_osd_b0202_si_hw_output;
	struct dv_reg_hw_output dv_osd_b0202_ts_hw_output;
	struct dv_reg_hw_output dv_osd_b0202_ti_hw_output;
};
struct dv_be_output {
	struct dv_reg_hw_output dv_be_dm_hw_output;
	struct dv_reg_hw_output dv_reorder_hw_output;
	struct dv_reg_hw_output dv_dither_hw_output;
	struct dv_reg_hw_output dv_scramble_hw_output;
};
struct dv_hw_reg_backup {
	uint32_t version;
	uint16_t length;
	struct dv_vdo_fe_output dv_vdo_fe_out[DV_OUT_LAYER];
	struct dv_gfx_fe_output dv_gfx_fe_out[DV_OUT_LAYER];
	struct dv_be_output dv_be_out;
};

struct dv_comp_reg {
	uint32_t  rpu_VDR_bit_depth;
	uint32_t  rpu_BL_bit_depth;
	uint32_t  rpu_EL_bit_depth;
	uint32_t  coefficient_log2_denom;
	uint32_t  el_spatial_resampling_filter_flag;
	uint32_t  disable_EL_flag;
	uint32_t  num_pivots_y;
	uint32_t  pivot_value_y[9];
	uint32_t  order_y[8];
	uint32_t  coeff_y[24];
	uint32_t  NLQ_offset_y;
	uint32_t  NLQ_coeff_y[3];
	uint32_t  num_pivots_cb;
	uint32_t  pivot_value_cb[5];
	uint32_t  mapping_idc_cb;
	uint32_t  order_cb[4];
	uint32_t  coeff_cb[12];
	uint64_t  coeff_mmr_cb[22];
	uint32_t  NLQ_offset_cb;
	uint32_t  NLQ_coeff_cb[3];
	uint32_t  num_pivots_cr;
	uint32_t  pivot_value_cr[5];
	uint32_t  mapping_idc_cr;
	uint32_t  order_cr[4];
	uint32_t  coeff_cr[12];
	uint64_t  coeff_mmr_cr[22];
	uint32_t  NLQ_offset_cr;
	uint32_t  NLQ_coeff_cr[3];
	uint16_t  vdr_bit_depth;

	uint8_t spatial_resampling_mode_hor_idc;
	uint8_t spatial_resampling_mode_ver_idc;
	uint32_t reg_spatial_filter_coeff_hor_00;
	uint32_t reg_spatial_filter_coeff_hor_01;
	uint32_t reg_spatial_filter_coeff_hor_prog;
	uint32_t reg_spatial_filter_coeff_hor_02;
	uint32_t reg_spatial_filter_coeff_hor_03;
	uint32_t reg_spatial_filter_coeff_hor_04;
	uint32_t reg_spatial_filter_coeff_hor_05;
	uint32_t reg_spatial_filter_coeff_ver_001;
	uint32_t reg_spatial_filter_coeff_ver_002;
	uint32_t reg_spatial_filter_coeff_ver_prog;
	uint32_t reg_spatial_filter_coeff_hor_06;
	uint32_t reg_spatial_filter_coeff_ver_003;
	uint32_t reg_spatial_filter_coeff_ver_004;
	uint32_t reg_spatial_filter_coeff_hor_07;
	uint32_t reg_spatial_filter_coeff_ver_101;
	uint32_t reg_spatial_filter_coeff_ver_102;
	uint32_t reg_spatial_filter_coeff_ver_103;
	uint32_t reg_spatial_filter_coeff_ver_104;
	uint32_t reg_spatial_filter_coeff_ver_002_uv;
	uint32_t reg_spatial_filter_coeff_ver_102_uv;
	uint32_t reg_spatial_filter_coeff_ver_103_uv;
	uint32_t reg_spatial_filter_coeff_ver_prog_uv;
};

struct dv_hdr_ctrl_reg {
	uint8_t dm_src_sel;
	uint8_t osd_hdr_sel;
	/* hw_version 0(dual layer) start */
	uint8_t hdr_in_src_sel;
	uint8_t dc0_to_dst_sel;
	uint8_t dc1_to_dst_sel;
	uint8_t hdr_1pto2p_byp_en;
	uint8_t hdr_de_gen_last_en;
	uint8_t hdr2dc0_ack_en;
	uint8_t hdr2dc1_ack_en;
	uint8_t hdr_id_vs_inv;
	uint8_t hdr_dc_vs_inv;
	uint8_t hdr_dcsub_vs_inv;
	/* hw_version 1(single layer) start */
	uint8_t hdr2ip_force_ack;
	uint8_t hdr2ip_path_en;
	/* hw_version 1(single layer) end*/
	uint8_t dm_msb_align_en;
	uint32_t hdr_h_size;
	uint32_t hdr_v_size;
	uint8_t aoi_en;
	uint8_t edclk_en;
	uint8_t clk_lut;
	uint8_t reg_scramble_clk_lut;
};

struct dv_dm_b01 {
	/* B01 top reg */
	uint8_t reg_b01_out_12bits_en;
};

struct dv_dm_b0101 {
	uint8_t cup420_en;
	uint8_t repeat_en;
	uint8_t _422to444_en;
	uint8_t cup420_43mode;
	uint8_t cup420_cbcr_cross_en;
	uint8_t cup420_0101_mode;
	uint8_t cup420_la_md;
	uint8_t cup420_tb_md;
	uint8_t cup420_c_in_r_ch;
	uint8_t b0101_clp_sel;
};

struct dv_dm_b0102 {
	uint8_t reg_b0102_byp_en;
	uint8_t reg_b0102_y2r_byp_en;
	uint32_t reg_b0102_y2r_byp_shift;
	int32_t reg_ycbcr_m0;
	int32_t reg_ycbcr_m1;
	int32_t reg_ycbcr_m2;
	int32_t reg_ycbcr_m3;
	int32_t reg_ycbcr_m4;
	int32_t reg_ycbcr_m5;
	int32_t reg_ycbcr_m6;
	int32_t reg_ycbcr_m7;
	int32_t reg_ycbcr_m8;
	int32_t reg_ycbcr_offset_0;
	int32_t reg_ycbcr_offset_1;
	int32_t reg_ycbcr_offset_2;
	uint32_t reg_range_clip;
	int32_t reg_ycbcr_shift;
	int32_t reg_range_min;
	int32_t reg_range_max;
	int32_t reg_range_inv;
};

struct dv_dm_b0103 {
	uint8_t reg_b0103_byp_en;
	int32_t reg_b0103_eotf_mode;
};

struct dv_dm_b0104 {
	uint8_t reg_b0104_byp_en;
	int32_t reg_csa2csb_m0;
	int32_t reg_csa2csb_m1;
	int32_t reg_csa2csb_m2;
	int32_t reg_csa2csb_m3;
	int32_t reg_csa2csb_m4;
	int32_t reg_csa2csb_m5;
	int32_t reg_csa2csb_m6;
	int32_t reg_csa2csb_m7;
	int32_t reg_csa2csb_m8;
	int32_t reg_csa2csb_shift;
};

struct dv_dm_b0105 {
	uint8_t reg_b0105_byp_en;
	uint8_t reg_hdr_gamma_select_en;
};

struct dv_dm_ootf {
	uint8_t reg_ootf_en;
};

struct dv_dm_b0106 {
	uint8_t reg_b0106_byp_en;
	int32_t reg_csc2ipt_m0;
	int32_t reg_csc2ipt_m1;
	int32_t reg_csc2ipt_m2;
	int32_t reg_csc2ipt_m3;
	int32_t reg_csc2ipt_m4;
	int32_t reg_csc2ipt_m5;
	int32_t reg_csc2ipt_m6;
	int32_t reg_csc2ipt_m7;
	int32_t reg_csc2ipt_m8;
	int32_t reg_csc2ipt_shift;
};

struct dv_dm_b02 {
	uint8_t reg_b02_byp_en;
	uint8_t reg_b0202_byp_clamp_en;
	uint8_t reg_ti_byp_en;
	uint8_t reg_ts_byp_en;
	uint8_t reg_si_byp_en;
	uint8_t reg_ss_byp_en;
	uint32_t reg_dm_b0202_shift;
};

struct dv_dm_b04 {
	uint8_t reg_hdr_vdo_be_en;
};

struct dv_dm_b0401 {
	uint8_t reg_b0401_byp_en;
	int32_t reg_b0401_ipt_m0;
	int32_t reg_b0401_ipt_m1;
	int32_t reg_b0401_ipt_m2;
	int32_t reg_b0401_ipt_m3;
	int32_t reg_b0401_ipt_m4;
	int32_t reg_b0401_ipt_m5;
	int32_t reg_b0401_ipt_m6;
	int32_t reg_b0401_ipt_m7;
	int32_t reg_b0401_ipt_m8;
	int32_t reg_b0401_ipt_shift;
};

struct dv_dm_b0402 {
	uint8_t reg_b0402_byp_en;
};

struct dv_dm_b0403 {
	uint8_t reg_b0403_byp_en;
	int32_t reg_b0403_csa2csb_m0;
	int32_t reg_b0403_csa2csb_m1;
	int32_t reg_b0403_csa2csb_m2;
	int32_t reg_b0403_csa2csb_m3;
	int32_t reg_b0403_csa2csb_m4;
	int32_t reg_b0403_csa2csb_m5;
	int32_t reg_b0403_csa2csb_m6;
	int32_t reg_b0403_csa2csb_m7;
	int32_t reg_b0403_csa2csb_m8;
	int32_t reg_b0403_csa2csb_shift;
	int32_t reg_b0403_csa2csb_clp_max;
	int32_t reg_b0403_csa2csb_clp_min;
};

struct dv_dm_b0404 {
	uint8_t reg_b0404_byp_en;
	int32_t reg_b0404_oetf_mode;
};

struct dv_dm_b0405 {
	uint8_t reg_b0405_byp_en;
	int32_t reg_b0405_rgb_m0;
	int32_t reg_b0405_rgb_m1;
	int32_t reg_b0405_rgb_m2;
	int32_t reg_b0405_rgb_m3;
	int32_t reg_b0405_rgb_m4;
	int32_t reg_b0405_rgb_m5;
	int32_t reg_b0405_rgb_m6;
	int32_t reg_b0405_rgb_m7;
	int32_t reg_b0405_rgb_m8;
	int32_t reg_b0405_rgb_offset_0;
	int32_t reg_b0405_rgb_offset_1;
	int32_t reg_b0405_rgb_offset_2;
	int32_t reg_b0405_rgb_shift;
	int32_t reg_b0405_rgb_min;
	int32_t reg_b0405_rgb_inv;
};

struct dv_dm_b0406 {
	uint8_t reg_b0406_byp_en;
	uint8_t reg_b0406_444to422_cbcr_swap;
	uint16_t reg_b0406_clp_sel;
};

struct dv_be_dm_reg {
	struct dv_dm_b04 b04;
	/* B04 component reg */
	struct dv_dm_b0401 b0401;
	struct dv_dm_b0402 b0402;
	struct dv_dm_b0403 b0403;
	struct dv_dm_b0404 b0404;
	struct dv_dm_b0405 b0405;
	struct dv_dm_b0406 b0406;
};

struct dv_dm_reg {
	struct dv_dm_b01 b01;

	/* B01 component reg */
	struct dv_dm_b0101 b0101;
	struct dv_dm_b0102 b0102;
	struct dv_dm_b0103 b0103;
	struct dv_dm_b0104 b0104;
	struct dv_dm_b0105 b0105;
	struct dv_dm_ootf ootf;
	struct dv_dm_b0106 b0106;

	struct dv_dm_b02 b02;
};

struct dv_gop_b0102 {
	uint8_t reg_b0102_byp_en;
	uint8_t reg_b0102_y2r_byp_en;
	uint32_t reg_b0102_y2r_byp_shift;
	int32_t reg_ycbcr_m0;
	int32_t reg_ycbcr_m1;
	int32_t reg_ycbcr_m2;
	int32_t reg_ycbcr_m3;
	int32_t reg_ycbcr_m4;
	int32_t reg_ycbcr_m5;
	int32_t reg_ycbcr_m6;
	int32_t reg_ycbcr_m7;
	int32_t reg_ycbcr_m8;
	int32_t reg_ycbcr_offset_0;
	int32_t reg_ycbcr_offset_1;
	int32_t reg_ycbcr_offset_2;
	uint32_t reg_range_clip;
	int32_t reg_ycbcr_shift;
	int32_t reg_range_min;
	int32_t reg_range_max;
	int32_t reg_range_inv;
};

struct dv_gop_b0103 {
	uint8_t reg_b0103_byp_en;
	int32_t reg_b0103_eotf_mode;
};

struct dv_gop_b0104 {
	uint8_t reg_b0104_byp_en;
	int32_t reg_csa2csb_m0;
	int32_t reg_csa2csb_m1;
	int32_t reg_csa2csb_m2;
	int32_t reg_csa2csb_m3;
	int32_t reg_csa2csb_m4;
	int32_t reg_csa2csb_m5;
	int32_t reg_csa2csb_m6;
	int32_t reg_csa2csb_m7;
	int32_t reg_csa2csb_m8;
	int32_t reg_csa2csb_shift;
};

struct dv_gop_b0106 {
	uint8_t reg_b0106_byp_en;
	int32_t reg_csc2ipt_m0;
	int32_t reg_csc2ipt_m1;
	int32_t reg_csc2ipt_m2;
	int32_t reg_csc2ipt_m3;
	int32_t reg_csc2ipt_m4;
	int32_t reg_csc2ipt_m5;
	int32_t reg_csc2ipt_m6;
	int32_t reg_csc2ipt_m7;
	int32_t reg_csc2ipt_m8;
	int32_t reg_csc2ipt_shift;
};

struct dv_gop_b02 {
	uint8_t reg_b02_byp_en;
	uint8_t reg_b0202_byp_clamp_en;
	uint8_t reg_dm_b0202_shift;
	uint8_t reg_ti_byp_en;
	uint8_t reg_ts_byp_en;
	uint8_t reg_si_byp_en;
	uint8_t reg_ss_byp_en;
};

struct dv_gop_reg {
	struct dv_gop_b0102 b0102;
	struct dv_gop_b0103 b0103;
	struct dv_gop_b0104 b0104;
	struct dv_gop_b0106 b0106;
	struct dv_gop_b02 b02;
};

struct dv_dither_reg {
	uint8_t hdr_dith_en;
	uint8_t hdr_dith_444md;
	uint8_t hdr_dith_8b_md;
	uint8_t hdr_dith_force_window;
	uint16_t hdr_dith_window;
	uint8_t hdr_byp_dith_reorder;
};

struct dv_reorder_reg {
	uint8_t reg_reorder_en;
	uint8_t reg_byp_y2r_reorder_disable;
};

struct dv_scramble_reg {
	uint32_t reg_meta_pkt_repeat_num;
	uint32_t reg_meta_pkt_num;
	uint32_t reg_meta_len_per_pkt;
	uint32_t vsem_meta_pkt_num;
};

struct dv_lut_tbl {
	uint32_t g2l[DV_OUT_LAYER][DV_B0103_ADL_SIZE];
	uint16_t tmluti[DV_OUT_LAYER][DV_B0202_TMLUTI_ADL_SIZE];
	uint16_t tmluts[DV_OUT_LAYER][DV_B0202_TMLUTS_ADL_SIZE];
	uint16_t smluti[DV_OUT_LAYER][DV_B0202_SMLUTI_ADL_SIZE];
	uint16_t smluts[DV_OUT_LAYER][DV_B0202_SMLUTS_ADL_SIZE];
	uint32_t gop_g2l[DV_OUT_LAYER][DV_OSD_B0103_ADL_SIZE];
	uint16_t gop_tmluti[DV_OUT_LAYER][DV_OSD_B0202_TMLUTI_ADL_SIZE];
	uint16_t gop_tmluts[DV_OUT_LAYER][DV_OSD_B0202_TMLUTS_ADL_SIZE];
	uint16_t gop_smluti[DV_OUT_LAYER][DV_OSD_B0202_SMLUTI_ADL_SIZE];
	uint16_t gop_smluts[DV_OUT_LAYER][DV_OSD_B0202_SMLUTS_ADL_SIZE];
	uint8_t md_pkts[DV_SCRAMBLE_ADL_SIZE];
};

struct dv_hw_reg {
	struct dv_comp_reg dv_comp[DV_OUT_LAYER];
	struct dv_dm_reg dv_dm[DV_OUT_LAYER];
	struct dv_hdr_ctrl_reg dv_ctrl[DV_OUT_LAYER];
	struct dv_gop_reg dv_gop[DV_OUT_LAYER];
	struct dv_dither_reg dv_dither;
	struct dv_reorder_reg dv_reorder;
	struct dv_scramble_reg dv_scm;
	struct dv_be_dm_reg dv_be_dm;
	struct dv_lut_tbl dv_lut_tbls;
};

enum DOVI_TZ_CALL_CMD {
	DOVI_TZ_CALL_CMD_SEND_CLI_INFO,
	DOVI_TZ_CALL_CMD_INIT_SHARE_MEMORY,
	DOVI_TZ_CALL_CMD_DEBUG_LEVEL_INIT,
	DOVI_TZ_CALL_CMD_REGISTER_IRQ,
	DOVI_TZ_CALL_CMD_MD_PARSER_INIT,
	DOVI_TZ_CALL_CMD_MD_PARSER_MAIN,
	DOVI_TZ_CALL_CMD_MD_PARSER_UNINIT,
	DOVI_TZ_CALL_CMD_CP_TEST_INIT,
	DOVI_TZ_CALL_CMD_FIND_RPU_BUFFER,
	DOVI_TZ_CALL_CMD_CP_TEST_MAIN,
	DOVI_TZ_CALL_CMD_CP_TEST_UNINIT,
	DOVI_TZ_CALL_CMD_SHARE_MEMORY_INIT,
	DOVI_TZ_CALL_CMD_SEC_MEM_COPY,
	DOVI_TZ_CALL_CMD_MAX,
};

enum DOVI_TZ_CALL_DIR {
	DOVI_TZ_CALL_DIR_NONE = 0,
	DOVI_TZ_CALL_DIR_VALUE_INPUT = 1,
	DOVI_TZ_CALL_DIR_VALUE_OUTPUT = 2,
	DOVI_TZ_CALL_DIR_VALUE_INOUT = 3,
	DOVI_TZ_CALL_DIR_MEM_INPUT = 4,
	DOVI_TZ_CALL_DIR_MEM_OUTPUT = 5,
	DOVI_TZ_CALL_DIR_MEM_INOUT = 6,
	DOVI_TZ_CALL_DIR_MEMREF_INPUT = 7,
	DOVI_TZ_CALL_DIR_MEMREF_OUTPUT = 8,
	DOVI_TZ_CALL_DIR_MEMREF_INOUT = 9,
};

enum input_mode_t {
	INPUT_MODE_OTT = 0,
	INPUT_MODE_HDMI = 1,
	INPUT_MODE_GRAPHICS = 2
};

enum signal_fmt_t {
	SIGNAL_FORMAT_INVALID = -1,
	SIGNAL_FORMAT_DOVI = 0,
	SIGNAL_FORMAT_HDR10 = 1,
	SIGNAL_FORMAT_SDR8 = 2,
	SIGNAL_FORMAT_SDR10 = 3,
	SIGNAL_FORMAT_HLG = 4,
	SIGNAL_FORMAT_HDR8 = 5
};

enum signal_range_t {
	SIGNAL_RANGE_SMPTE = 0,
	SIGNAL_RANGE_FULL = 1,
	SIGNAL_RANGE_SDI = 2
};

enum graphic_format_t {
	GRAPHIC_SDR_YUV = 0,	/* BT.709 YUV BT1886 */
	GRAPHIC_SDR_RGB = 1,	/* BT.709 RGB BT1886 */
	GRAPHIC_HDR_YUV = 2,	/* BT.2020 YUV PQ */
	GRAPHIC_HDR_RGB = 3	/* BT.2020 RGB PQ */
};


enum cp_clr_t {
	CP_CLR_YUV = 0,
	CP_CLR_RGB = 1,
	CP_CLR_IPT = 2
};

enum cp_eotf_t {
	CP_EOTF_BT1886 = 0,
	CP_EOTF_PQ = 1,
	CP_EOTF_HLG = 2
};

enum chroma_format_t {
	CHROMA_FORMAT_P420 = 0,
	CHROMA_FORMAT_UYVY = 1,
	CHROMA_FORMAT_P444 = 2,
	CHROMA_FORMAT_I444 = 3
};

enum pri_mode_t {
	G_PRIORITY = 0,
	V_PRIORITY = 1,
};

enum cp_dovi_type_t {
	/**Input is a Dovi signal, and output is a native signal not processed by VS10,
	 **eg. SDR, HDR10 or HLG.
	 **/
	DOVI_TYPE_NONDOVI = 0x0,
	/**Input is Dovi graded content, and output is a Dovi signal. **/
	DOVI_TYPE_DOVI    = 0x1,
	/* 0b0010 - Reserved */
	/* Input is HDR10, and output is a Dovi signal processed by Dovi VS10. */
	DOVI_TYPE_HDR10   = 0x3,
	/* 0100 - Reserved */
	/* Input is SDR, and output is a Dovi signal processed by Dovi VS10. */
	DOVI_TYPE_SDR     = 0x5,
	/* 0b0110 - Reserved */
	/* Input is HLG, and output is a Dovi signal processed by Dovi VS10. */
	DOVI_TYPE_HLG     = 0x7
	/* 0b1000 - 0b1111: Reserved */
};

struct vsif_param_t {
	int low_latency;
	int backlt_ctrl_md_present;
	int source_dm_version;
	int eff_tmax_pq;
	enum cp_dovi_type_t dovi_signal_type;
	int auxiliary_md_present;
	int L11_md_present;
	uint8_t auxiliary_runmode;
	uint8_t auxiliary_runversion;
	uint8_t auxiliary_debug0;
	uint8_t content_type;
	uint8_t white_point;
	uint8_t L11_byte2;
	uint8_t L11_byte3;
	int bt2020_container;
};

/*idk2.6 src params*/
struct src_params_t {
	/* All inputs */
	bool en;
	int width;
	int height;
	int src_fps;
	int src_frame_num;
	enum input_mode_t input_mode;
	enum signal_fmt_t input_format;
	int use_primaries_for_dovi;
	enum DISP_DR_TYPE_T dr_type;
	int primaries[8];
	/* Dovi LL or non Dovi */
	int src_bit_depth;
	enum chroma_format_t chroma_format;
	enum cp_clr_t color_format;
	bool svp;
	uint32_t sec_handle;
	uint32_t rpu_bs_len;
	unsigned char rpu_bs_buffer[BITSTREAM_BUFFER_SIZE];
	uint32_t sec_handle_in;
	uint32_t sec_handle_out;
	uint32_t len_tmp;
	uint32_t sec_handle_len;
	int is_rbsp;
	enum DV_RPU_TYPE rpu_type;
	/* Non Dovi */
	uint32_t min;
	uint32_t max;
	enum cp_eotf_t eotf;
	int gamma;
	enum signal_range_t src_yuv_range;
	int16_t ycc2rgb_matrix[9];
	int ycc2rgb_offset[3];
	int16_t ycc2rgb_scale;
	struct mtk_disp_hdr10_md_t hdr10_md;
	uint32_t comp_md[DOVI_COMP_DW_SIZE];
	uint32_t orig_md[DOVI_MD_DW_SIZE];
	uint32_t orig_md_len;
};

/*
 *    @typedef cp_cli_param_t
 *    @brief Command line parameters.
 *    total 20KB
 */
struct cp_param_t {
	int width;
	int height;
	enum signal_fmt_t output_format;
	enum chroma_format_t out_chroma_format;
	uint32_t min;
	uint32_t max;
	int use_vsem;
	int use_ll;
	int ll_rgb_desired;
	int num_input;
	int pri_input;
	int vpm_trans_timeout;
	int dovi2hdr10_mapping;
	enum pri_mode_t priority_mode;
	int test_mode;
	int always10bit;
	int dump_hks_txt;
	int user_l11;
	uint8_t user_l11_buf[4];
	int profile;
	uint8_t cp_init_update; /*1= need update*/
	uint8_t dm_md_parse_ctrl; /*1 init, 2 main uninit->init,3 sub uninit->init*/
	unsigned char vsvdb_hdmi[0x1A];
	char vsvdb_file[MAX_FILENAME_LENGTH];
	char vsif_file[MAX_NUM_INPUT][MAX_FILENAME_LENGTH];
	char vsem_file[MAX_NUM_INPUT][MAX_FILENAME_LENGTH];
	char drm_file[MAX_NUM_INPUT][MAX_FILENAME_LENGTH];
	char sdp_file[MAX_NUM_INPUT][MAX_FILENAME_LENGTH];
};

struct cp_param_t_old {
	int width;   /**<@brief Width of the video frame   */
	int height;  /**<@brief Height of the video frame   */
	enum signal_fmt_t input_format;
	enum signal_fmt_t output_format;
	int md_filter;
	int target_min_lum, target_max_lum;
	int graphic_min_lum, graphic_max_lum;
	enum signal_range_t src_yuv_range;
	int src_chroma_format;
	int src_bit_depth;
	int f_graphic_on;
	int g_bit_depth;
	enum graphic_format_t g_format;
	enum chroma_format_t g_cf;
	enum pri_mode_t priority_mode;
	int src_fps;
	int use_ll;
	int ll_rgb_desired;
	int test_mode;
	int support_el;
	int dovi2hdr10_mapping;
	char vsvdb_file[MAX_FILENAME_LENGTH];
	unsigned char vsvdb_hdmi[0x1A];
};


enum dovi_signal_format_t {
	DOVI_FORMAT_DOVI = 0,
	DOVI_FORMAT_HDR10 = 1,
	DOVI_FORMAT_SDR = 2,
	DOVI_FORMAT_SDR_2020 = 3,
	DOVI_FORMAT_HLG = 4,
	DOVI_FORMAT_DOVI_LOW_LATENCY = 5,
	DOVI_FORMAT_VSEM_DOVI = 6,
	DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY = 7,
	DOVI_FORMAT_INVALID
};

enum dovi_enable_type_t {
	DOVI_INOUT_FORMAT_CHANGE = 2,
	DOVI_PRIORITY_MODE_CHANGE = 3,
	DOVI_RESOLUTION_CHANGE = 4,
};

struct dovi_out_info_t {
	enum dovi_signal_format_t out_format;
	bool b_gfx_mode;
	bool is_low_latency;
	bool is_vsem;
	unsigned char *vsvdb_edid;
};

enum dovi_signal_range_t {
	DOVI_SIG_RANGE_SMPTE = 0,	/* head range */
	DOVI_SIG_RANGE_FULL = 1,	/* full range */
	DOVI_SIG_RANGE_SDI = 2	/* PQ */
};

struct dovi_video_info_t {
	int width;   /**<@brief Width of the video frame   */
	int height;  /**<@brief Height of the video frame   */
	enum dovi_signal_format_t input_format;
	enum dovi_signal_range_t src_yuv_range;
	int src_chroma_format;
	int src_bit_depth;
	int src_fps;
	int orignalsrc_width;	/* for scaling case */
	int orignalsrc_heigth;
};

enum dovi_status {
	DOVI_STATUS_OK = 0,
	DOVI_STATUS_ERROR = 1,
};


struct dovi_graphic_info_t {
	int graphic_min_lum, graphic_max_lum;
	int f_graphic_on;
};

struct vdp_buf_t {
	unsigned char vdp_id;
	struct video_buffer_info *buf;
	int dsd_en;
};

extern struct disp_hw_resolution dovi_res;
extern uint32_t dovi_idk_test;
extern uint32_t force_priority_mode;
extern uint32_t priority_mode;
extern uint32_t dovi_idk_file_id;
extern bool set_graphic_max_lum_enable;
extern bool set_video_max_lum_enable;
extern int32_t fhd_color_format;
extern int32_t uhd_color_format;

extern int graphic_max_lum;
extern int video_max_lum;
extern struct dovi_out_info_t dovi_out_info;
extern bool priority_mode_change;
extern uint32_t dv_vdo_fe_en[2];
extern uint32_t dv_gfx_fe_en[2];
extern uint32_t dv_vdo_be_en;
extern struct disp_hw_common_info hdr_common_info;
extern unsigned char idk_5000_dm_md[];
extern unsigned char idk_5000_comp_md[];
extern unsigned char vsvdb_v1_15[];
extern uint32_t layer_info_set_by_cmd;
extern bool tv_info_set_by_cmd;
extern unsigned char rpu_test_data[];
extern unsigned char av1_obu_data[];
extern uint32_t ui_force_hdr_type;
extern struct dv_all_reg_tab dv_dsys_all_reg;
extern struct dv_all_reg_tab dv_msys_all_reg;
extern uint32_t adl_mode;
extern char *dovi_reg_base[5];
extern char *vdout_reg_base;
extern uint32_t _subv_type;
extern bool dovi_black_en_bycmd;
extern uint32_t dovi_black_cnt_bycmd;
extern bool hdr_allm_en;
extern bool hdr_gfx_allm_en;
extern struct cp_param_t *p_cp_param;


int dovi_remove_rpu_nal_type(unsigned int first_frame,
	unsigned char *src_rpu,
	unsigned char *dst_rpu,
	unsigned int len);
uint32_t dovi_set_out_res(uint32_t out_res, uint16_t width,
	uint16_t height);
uint32_t dovi_set_output_format(enum dovi_signal_format_t out_format);
uint32_t dovi_set_video_info(struct mtk_disp_hdr_md_info_t *hdr_metadata);
uint32_t dovi_set_sub_video_info(struct mtk_disp_hdr_md_info_t *hdr_metadata);
uint32_t dovi_set_video_input_format(enum dovi_signal_format_t e_input_format);
uint32_t dovi_get_input_format(enum DISP_DR_TYPE_T *dovi_input_dr);
uint32_t dovi_get_input_format1(enum DISP_DR_TYPE_T *dovi_input_dr);
uint32_t dovi_set_graphic_format(uint32_t g_format);
uint32_t dovi_set_uhd_graphic_format(uint32_t g_format);
uint32_t dovi_set_graphic_info(uint32_t ucOn);
uint32_t dovi_set_graphic_info_uhd(uint32_t ucOn);
uint32_t dovi_set_gfx_rpu_info(void);
uint32_t dovi_set_composer_mode(bool fgComposerEL);
uint32_t dovi_update_graphic_info(void);

uint32_t dovi_set_priority_mode(uint32_t mode);
uint32_t dovi_get_priority_mode(uint32_t *mode);

uint32_t dovi_set_test_mode(int mode);
uint32_t dovi_set_support_el(int value);

uint32_t dovi_set_low_latency_mode(int use_ll, int ll_rgb_desired);
uint32_t dovi_set_vsem_mode(int use_vsem);
uint32_t dovi_set_dovi2hdr10_mapping(int dovi2hdr10_mapping);
uint32_t dovi_set_vsvdb_file_name(char *vsvdb_file_name);
uint32_t dovi_set_vsvdb_hdmi(char *vsvdb_edid, int len);

uint32_t dovi_update_target_lum(void);
int disp_dovi_common_init(void);
int disp_dovi_common_test(uint32_t option);
uint32_t dovi_get_low_latency_mode(void);
uint32_t dovi_get_output_format(void);
bool dovi_get_profile4(void);
uint32_t dovi_update_res_change(
	struct disp_hw_tv_capbility *tv_cap,
	const struct disp_hw_resolution *resolution);
void dovi_path_disable(void);
void dovi_path_enable(void);
void set_hdmi_info(enum dovi_signal_format_t output_format,
	struct disp_hw_tv_capbility *tv_cap, bool enable);
void dovi_update_output_setting(
	struct disp_hw_common_info *info,
	struct video_buffer_info *buf_main,
	struct video_buffer_info *buf_sub);
void disp_dovi_force_output_setting(struct disp_hw_common_info *info);
int disp_dovi_process(uint32_t enable,
	struct mtk_disp_hdr_md_info_t *hdr_metadata);
int dovi_config_adl(void);
int dovi_config_menuload(void);
bool dovi_check_4k60_timing(enum HDMI_VIDEO_RESOLUTION res_mode);
int dovi_frame_commit(struct video_buffer_info *buf_main,
	struct video_buffer_info *buf_sub, bool sub_exist);
void disp_dovi_set_tz_test_info(uint32_t id);
void set_hdr10_metadata(struct VID_PLA_HDR_METADATA_INFO_T rHdr,
	struct mtk_disp_hdr_md_info_t dovi_hdr_md_info[],
	uint32_t vdp_id);
int dovi_get_hdr10_metadata(struct VID_PLA_HDR_METADATA_INFO_T *rHdr,
	uint32_t id);

void dovi_set_hw_path(void);
void dovi_get_hdmi_output_format(uint32_t *out_format,
	bool *bt2020_enable);
void dovi_av1_parser_test(void);
int dovi_mmsys_update_reg(uint32_t *p_reg_tbl, uint32_t len);
int dovi_config_fefifo_swap(bool swap);
void disp_dovi_update_input_size(uint32_t layer_id,
	uint32_t u4width, uint32_t u4height);
void disp_dovi_set_internalbyass(uint8_t id);
void disp_dovi_set_adldelay(uint32_t layer_id);
uint8_t *disp_dovi_get_hdmi_vsem_info(
	uint32_t *dovi_vsem_num_pks, uint32_t *dovi_hdmi_type);
uint32_t dovi_set_be_out_css(enum chroma_format_t out_css);
bool disp_dovi_get_ext_md_info(
	uint32_t ext_blk_no, uint8_t *ext_buf, uint32_t buf_len);
void disp_dovi_dump_ext_md(void);
enum dovi_signal_format_t dovi_judge_out_format(
	struct disp_hw_tv_capbility *tv_cap,
	const struct disp_hw_resolution *resolution);

#endif
