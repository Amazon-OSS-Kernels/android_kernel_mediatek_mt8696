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

#ifndef _DOVI_COMMON_HAL_H_
#define _DOVI_COMMON_HAL_H_

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include "dovi_log.h"
#include "disp_dovi_common_if.h"
#include "mt-plat/sync_write.h"
#include "dovi_vdo_fe_hw.h"
#include "dovi_gfx_fe_hw.h"
#include "dovi_be_hw.h"


#define DOVI_FE_LUT_SIZE 0x4000
#define DOVI_BE_LUT_SIZE 0x1000

#define DOVI_IOMMU_SUPPORT 1

#define MAX_COMP_REG_NUM             (300)
#define MAX_CRTL_REG_NUM             (100)
#define MAX_DM_REG_NUM               (300)
#define MAX_GOP_REG_NUM              (100)
#define MAX_DITHER_REG_NUM           (20)
#define MAX_SCRAMBLE_REG_NUM         (20)
#define MAX_REORDER_REG_NUM          (10)
#define MAX_LUT_REG_NUM              (10)

extern int dump_bit_depth;

enum HDR_FE_ID {
	FE0 = 0,
	FE1 = 1,
	FE_MAX,
};

struct dv_sw_reg {
	uint32_t *value;
	uint8_t *update;
};

#define Dv_WriteREG(arg, val) \
	mt_reg_sync_writel(val, (unsigned long *)(arg))
#define Dv_ReadREG(arg) __raw_readl((unsigned long *)(arg))

#define Dv_WriteREGMsk(arg, val, msk) \
	Dv_WriteREG((arg), (Dv_ReadREG(arg) & (~(uint32_t)(msk))) \
	| (((uint32_t)(val)) & ((uint32_t)(msk))))


#define dv_w_regtbl_2bytes(addr, val, mask, tbl) \
	do { \
		tbl->reg_addr[tbl->used_depth] = (addr); \
		tbl->reg_value[tbl->used_depth] = (val); \
		tbl->reg_mask[tbl->used_depth] = (mask); \
		tbl->used_depth++; \
	} while (0)

#define dv_w_regtbl_4bytes(addr, val, mask, tbl) \
	do { \
		dv_w_regtbl_2bytes(addr, (val & 0xFFFF), \
		(mask & 0xFFFF), tbl); \
		dv_w_regtbl_2bytes((addr + 4), (val >> 16), \
		(mask >> 16), tbl); \
	} while (0)

#define dv_w_regtbl_8bytes(addr, val, mask, tbl)\
	do { \
		dv_w_regtbl_4bytes(addr, (val & 0xFFFFFFFF), \
		(mask & 0xFFFFFFFF), tbl); \
		dv_w_regtbl_4bytes((addr + 8), (val >> 32), \
		(mask >> 32), tbl); \
	} while (0)

//reg_bl_coeff_y50
#define reg_bl_coeff_y50(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02C0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mode
#define reg_mode(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0C04_HDR_BE_BK7E + reg, \
	((_val) << 15), 0x8000, _obj)

//reg_csa2csb_m0
#define reg_csa2csb_m0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0880_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

//reg_bl_coeff_y70
#define reg_bl_coeff_y70(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02F0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

//reg_bl_coeff_v21
#define reg_bl_coeff_v21(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03D0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

//reg_csc2ipt_m4
#define reg_csc2ipt_m4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08B4_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


//reg_bl_pivot_u1
#define reg_bl_pivot_u1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_030C_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

//reg_mst_hdr_oot_eidb_20
#define reg_mst_hdr_oot_eidb_20(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0768_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

//reg_mmr_coeff_u12
#define reg_mmr_coeff_u12(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04A0_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

//reg_spatial_filter_coeff_hor_02
#define reg_spatial_filter_coeff_hor_02(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A28_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

//reg_cup420_en_vs
#define reg_cup420_en_vs(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 14) & 0xFF00), 0x4000, _obj)

//reg_uvc_new_en
#define reg_uvc_new_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B00_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x01, _obj)

//reg_bl_coeff_u11
#define reg_bl_coeff_u11(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0340_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

//reg_mst_hdr_oot_eidb_18
#define reg_mst_hdr_oot_eidb_18(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0764_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

//reg_hdr_h_size
#define reg_hdr_h_size(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09CC_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_vdr_bit_depth
#define reg_vdr_bit_depth(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0C60_HDR_BE_BK7E + reg, \
	((_val) & 0xFF), 0x03, _obj)


//reg_mst_hdr_gamma_output_limit_0
#define reg_mst_hdr_gamma_output_limit_0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08E4_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

//reg_bl_coeff_y52
#define reg_bl_coeff_y52(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02D0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

//reg_spatial_filter_coeff_hor_prog
#define reg_spatial_filter_coeff_hor_prog(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A20_HDR_HVSU_BK7D + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

//reg_bl_coeff_y11
#define reg_bl_coeff_y11(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0268_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

//reg_el_coeff_y0
#define reg_el_coeff_y0(_val, reg, _obj) do { \
	dv_w_regtbl_2bytes(SC1_REG_0C08_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C08_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
} while (0)

//reg_oot_3x1_m1
#define reg_oot_3x1_m1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0718_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mapping_idc_u
#define reg_mapping_idc_u(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0204_HDR_POLY_BK40 + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_y2r_byp_shift
#define reg_y2r_byp_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFF), 0x0C, _obj)

	//reg_mst_hdr_gamma_eidb_26
#define reg_mst_hdr_gamma_eidb_26(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0910_HDR_DM_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)


	//reg_bl_pivot_y2
#define reg_bl_pivot_y2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0228_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_mst_hdr_oot_eidb_13
#define reg_mst_hdr_oot_eidb_13(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0758_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)


	//reg_low_y_close_to_ori_slope_up
#define reg_low_y_close_to_ori_slope_up(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B0C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x3F, _obj)

	//reg_mmr_coeff_u2
#define reg_mmr_coeff_u2(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0434_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)


	//reg_hdr_1pto2p_byp_en
#define reg_hdr_1pto2p_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09E8_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0x1000, _obj)

	//reg_dm_src_sel
#define reg_dm_src_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09EC_HDR_DM_FE + reg, \
	(((_val) << 7) & 0xFF), 0x80, _obj)


	//reg_spatial_filter_coeff_ver_103_uv
#define reg_spatial_filter_coeff_ver_103_uv(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A64_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0x0300, _obj)

	//reg_csc2ipt_m5
#define reg_csc2ipt_m5(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08B8_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mmr_coeff_v16
#define reg_mmr_coeff_v16(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_05B4_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_gamma_eidb_08
#define reg_mst_hdr_gamma_eidb_08(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0900_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)


	//reg_mst_hdr_oot_eidb_30
#define reg_mst_hdr_oot_eidb_30(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_077C_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mst_hdr_gamma_end_diff_0
#define reg_mst_hdr_gamma_end_diff_0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08E0_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b02_byp_en
#define reg_b02_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 7) & 0xFF), 0x80, _obj)

	//reg_hdr_in_src_sel
#define reg_hdr_in_src_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09E4_HDR_DM_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0700, _obj)


	//reg_mst_hdr_gamma_output_limit_1
#define reg_mst_hdr_gamma_output_limit_1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08EC_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_gamma_eidb_10
#define reg_mst_hdr_gamma_eidb_10(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0900_HDR_DM_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_mst_hdr_gamma_end_diff_2
#define reg_mst_hdr_gamma_end_diff_2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08F0_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0105_byp_en
#define reg_b0105_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 5) & 0xFF), 0x20, _obj)

	//reg_mmr_coeff_v9
#define reg_mmr_coeff_v9(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0568_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_mmr_coeff_v4
#define reg_mmr_coeff_v4(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0534_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_low_y_close_to_ori_en
#define reg_low_y_close_to_ori_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B00_HDR_HVSU_BK7D + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_csa2csb_m6
#define reg_csa2csb_m6(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0898_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_25
#define reg_mst_hdr_oot_eaoff_25(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07E4_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_bl_coeff_y40
#define reg_bl_coeff_y40(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02A8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_lut_load_en_7a
#define reg_lut_load_en_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0980_HDR_DM_FE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_uvc_low_sat_prot_thrd
#define reg_uvc_low_sat_prot_thrd(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B14_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0xFF00, _obj)

	//reg_bl_coeff_u01
#define reg_bl_coeff_u01(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0328_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_el_coeff_v2
#define reg_el_coeff_v2(_val, reg, _obj) do { \
	dv_w_regtbl_2bytes(SC1_REG_0C58_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C5C_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
	} while (0)

	//reg_el_coeff_u2
#define reg_el_coeff_u2(_val, reg, _obj)  do {\
	dv_w_regtbl_2bytes(SC1_REG_0C38_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C3C_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
	} while (0)

	//reg_spatial_filter_coeff_hor_07
#define reg_spatial_filter_coeff_hor_07(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A3C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_mst_hdr_gamma_eaoff_16
#define reg_mst_hdr_gamma_eaoff_16(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0940_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_oot_eaoff_00
#define reg_mst_hdr_oot_eaoff_00(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0780_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_cup420_tb_md
#define reg_cup420_tb_md(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0808_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_mst_hdr_gamma_eidb_22
#define reg_mst_hdr_gamma_eidb_22(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_090C_HDR_DM_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_lut_init_addr_7b
#define reg_lut_init_addr_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03A4_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0x07FF, _obj)

	//reg_mmr_coeff_v20
#define reg_mmr_coeff_v20(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_05E0_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_23
#define reg_mst_hdr_oot_eaoff_23(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07DC_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_gamma_eidb_21
#define reg_mst_hdr_gamma_eidb_21(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_090C_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_el_nlq_offset_v
#define reg_el_nlq_offset_v(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0C44_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_bl_coeff_u02
#define reg_bl_coeff_u02(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0330_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_gamma_eidb_05
#define reg_mst_hdr_gamma_eidb_05(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08FC_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_lut_init_addr_7a
#define reg_lut_init_addr_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09A4_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_bl_pivot_u4
#define reg_bl_pivot_u4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0318_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_el_nlq_offset_y
#define reg_el_nlq_offset_y(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0C04_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_mst_hdr_oot_eaoff_08
#define reg_mst_hdr_oot_eaoff_08(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07A0_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_ycbcr_m5
#define reg_ycbcr_m5(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0834_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_bl_coeff_u22
#define reg_bl_coeff_u22(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0360_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_lut_addr_7b
#define reg_lut_addr_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0384_VDO_DM_BE + reg, \
	((_val) & 0xFF00), 0x07FF, _obj)

	//reg_mmr_coeff_u17
#define reg_mmr_coeff_u17(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04D4_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_csa2csb_m4
#define reg_csa2csb_m4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0890_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


	//reg_bl_coeff_v31
#define reg_bl_coeff_v31(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03E8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_01
#define reg_mst_hdr_oot_eidb_01(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0740_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_mmr_coeff_u1
#define reg_mmr_coeff_u1(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0428_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_ycbcr_m8
#define reg_ycbcr_m8(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0840_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_cup420_43mode
#define reg_cup420_43mode(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0808_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_range_min_7a
#define reg_range_min_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_085C_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_cup420_0101_mode
#define reg_cup420_0101_mode(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0808_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_spatial_filter_coeff_ver_104
#define reg_spatial_filter_coeff_ver_104(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A5C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_mst_hdr_oot_eidb_09
#define reg_mst_hdr_oot_eidb_09(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0750_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_bl_coeff_v12
#define reg_bl_coeff_v12(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03C0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_cup420_c_in_r_ch
#define reg_cup420_c_in_r_ch(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0808_HDR_DM_FE + reg, \
	(((_val) << 5) & 0xFF), 0x20, _obj)

	//reg_el_coeff_y1
#define reg_el_coeff_y1(_val, reg, _obj) do { \
	dv_w_regtbl_2bytes(SC1_REG_0C10_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C14_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
	} while (0)

	//reg_mst_hdr_oot_eidb_25
#define reg_mst_hdr_oot_eidb_25(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0770_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_bl_coeff_y21
#define reg_bl_coeff_y21(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0280_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_26
#define reg_mst_hdr_oot_eaoff_26(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07E8_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_gamma_eaoff_15
#define reg_mst_hdr_gamma_eaoff_15(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0940_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_hdr2dc1_ack_en
#define reg_hdr2dc1_ack_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_SC_BK50_02_L + reg, \
	(((_val) << 5) & 0xFF), 0x20, _obj)


	//reg_mst_hdr_oot_eaoff_09
#define reg_mst_hdr_oot_eaoff_09(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07A4_HDR_TOP_FE + reg, \
	((_val) & 0xFF00), 0x01FF, _obj)

	//reg_spatial_filter_coeff_ver_002
#define reg_spatial_filter_coeff_ver_002(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A44_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_low_y_sat_prot_en
#define reg_low_y_sat_prot_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B00_HDR_HVSU_BK7D + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_mst_hdr_oot_eaoff_27
#define reg_mst_hdr_oot_eaoff_27(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07EC_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_lut_load_en_7b
#define reg_lut_load_en_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0380_VDO_DM_BE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_y2r_byp_en
#define reg_y2r_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_mst_hdr_gamma_eidb_19
#define reg_mst_hdr_gamma_eidb_19(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0908_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0xF000, _obj)

	//reg_mst_hdr_gamma_eaoff_01
#define reg_mst_hdr_gamma_eaoff_01(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0918_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_gamma_eidb_09
#define reg_mst_hdr_gamma_eidb_09(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0900_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_bl_coeff_u12
#define reg_bl_coeff_u12(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0348_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_12
#define reg_mst_hdr_oot_eidb_12(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0758_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_bl_coeff_v02
#define reg_bl_coeff_v02(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03A8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_03
#define reg_mst_hdr_gamma_eaoff_03(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0920_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)


	//reg_bl_pivot_y8
#define reg_bl_pivot_y8(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0240_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_dm_b0202_open_uvc_en
#define reg_dm_b0202_open_uvc_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B4C_HDR_HVSU_BK7D + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_mst_hdr_gamma_eaoff_06
#define reg_mst_hdr_gamma_eaoff_06(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0928_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_main_cb_sub_128_pre_en
#define reg_main_cb_sub_128_pre_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_SC_BK2F_70_L + reg, \
	(((_val) << 8) & 0xFF00), 0x0100, _obj)

	//reg_cup420_la_md
#define reg_cup420_la_md(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0808_HDR_DM_FE + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_scramble_clk_lut
#define reg_scramble_clk_lut(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_CLKGEN2_43_L + reg, \
	((_val) & 0xFF), 0x01, _obj E_DV_IP_SEL_CLK)

	//reg_close_to_one_slope_up
#define reg_close_to_one_slope_up(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B18_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0x3F00, _obj)

	//reg_csa2csb_m5
#define reg_csa2csb_m5(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0894_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


	//reg_bl_coeff_v20
#define reg_bl_coeff_v20(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03C8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mmr_coeff_v21
#define reg_mmr_coeff_v21(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_05E8_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_bl_coeff_y31
#define reg_bl_coeff_y31(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0298_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_28
#define reg_mst_hdr_oot_eidb_28(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0778_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_bl_pivot_v1
#define reg_bl_pivot_v1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0384_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_b0102_byp_en
#define reg_b0102_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)


	//reg_bl_coeff_u21
#define reg_bl_coeff_u21(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0358_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_bl_pivot_y4
#define reg_bl_pivot_y4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0230_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_hdr2dc0_ack_en
#define reg_hdr2dc0_ack_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_SC_BK50_02_L + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_mst_hdr_gamma_eidb_20
#define reg_mst_hdr_gamma_eidb_20(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_090C_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_spatial_filter_coeff_ver_prog_uv
#define reg_spatial_filter_coeff_ver_prog_uv(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A60_HDR_HVSU_BK7D + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)


	//reg_csc2ipt_m0
#define reg_csc2ipt_m0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08A4_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


	//reg_csc2ipt_m2
#define reg_csc2ipt_m2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08AC_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_el_coeff_u0
#define reg_el_coeff_u0(_val, reg, _obj) do { \
	dv_w_regtbl_2bytes(SC1_REG_0C28_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C2C_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
	} while (0)

	//reg_dc1_to_dst_sel
#define reg_dc1_to_dst_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09E4_HDR_DM_FE + reg, \
	(((_val) << 14) & 0xFF00), 0xC000, _obj)

	//reg_mmr_coeff_u3
#define reg_mmr_coeff_u3(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0440_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_lut_sel
#define reg_lut_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0980_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_mst_hdr_oot_eidb_05
#define reg_mst_hdr_oot_eidb_05(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0748_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_bl_pivot_u2
#define reg_bl_pivot_u2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0310_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_spatial_filter_coeff_hor_01
#define reg_spatial_filter_coeff_hor_01(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A24_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_boundary_c22
#define reg_boundary_c22(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B24_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)


	//reg_mmr_coeff_u8
#define reg_mmr_coeff_u8(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0474_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_lut_sel_ip
#define reg_lut_sel_ip(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09A0_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_bl_coeff_v11
#define reg_bl_coeff_v11(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03B8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_29
#define reg_mst_hdr_oot_eaoff_29(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07F4_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mmr_coeff_v7
#define reg_mmr_coeff_v7(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0554_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mmr_coeff_u4
#define reg_mmr_coeff_u4(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0448_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_mmr_coeff_u18
#define reg_mmr_coeff_u18(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04E0_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_spatial_filter_coeff_ver_102_uv
#define reg_spatial_filter_coeff_ver_102_uv(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A64_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_spatial_filter_coeff_ver_prog
#define reg_spatial_filter_coeff_ver_prog(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A40_HDR_HVSU_BK7D + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_mst_hdr_oot_eaoff_10
#define reg_mst_hdr_oot_eaoff_10(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07A8_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_bl_coeff_y01
#define reg_bl_coeff_y01(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0250_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_bl_coeff_v00
#define reg_bl_coeff_v00(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0398_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_main_cmc
#define reg_main_cmc(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_SC_BK2F_70_L + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_uvc_low_th
#define reg_uvc_low_th(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0B3C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFFFF), 0x0FFFFF, _obj)

	//reg_mst_hdr_gamma_eidb_17
#define reg_mst_hdr_gamma_eidb_17(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0908_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_mst_hdr_gamma_eidb_28
#define reg_mst_hdr_gamma_eidb_28(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0914_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mst_hdr_gamma_eidb_07
#define reg_mst_hdr_gamma_eidb_07(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08FC_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0xF000, _obj)

	//reg_bl_coeff_u10
#define reg_bl_coeff_u10(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0338_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mmr_coeff_u15
#define reg_mmr_coeff_u15(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04C0_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_26
#define reg_mst_hdr_oot_eidb_26(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0774_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mmr_coeff_u7
#define reg_mmr_coeff_u7(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0468_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_mst_hdr_oot_eidb_15
#define reg_mst_hdr_oot_eidb_15(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_075C_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_el_coeff_u1
#define reg_el_coeff_u1(_val, reg, _obj) do { \
	dv_w_regtbl_2bytes(SC1_REG_0C30_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C34_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
	} while (0)

	//reg_mmr_coeff_u16
#define reg_mmr_coeff_u16(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04C8_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_mst_hdr_gamma_eaoff_24
#define reg_mst_hdr_gamma_eaoff_24(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0958_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)



	//reg_uvc_low_y_prot_thrd
#define reg_uvc_low_y_prot_thrd(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B00_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0xFF00, _obj)

	//reg_lut_wd2
#define reg_lut_wd2(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0998_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)


	//reg_b0103_byp_en
#define reg_b0103_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)


	//reg_mst_hdr_gamma_eidb_27
#define reg_mst_hdr_gamma_eidb_27(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0910_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0xF000, _obj)


	//reg_clip_max
#define reg_clip_max(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_086C_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_17
#define reg_mst_hdr_gamma_eaoff_17(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0994_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)

	//reg_mst_hdr_oot_eidb_07
#define reg_mst_hdr_oot_eidb_07(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_074C_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_el_dat_8b
#define reg_el_dat_8b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A04_HDR_HVSU_BK7D + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_mmr_coeff_v11
#define reg_mmr_coeff_v11(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0580_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_hdrin_shift_el
#define reg_hdrin_shift_el(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09EC_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFF), 0x0C, _obj)

	//reg_ycbcr_offset_1
#define reg_ycbcr_offset_1(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_084C_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_hdr_v_size
#define reg_hdr_v_size(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09D0_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_05
#define reg_mst_hdr_gamma_eaoff_05(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0924_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)

	//reg_mst_hdr_gamma_eaoff_07
#define reg_mst_hdr_gamma_eaoff_07(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0928_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_oot_eaoff_28
#define reg_mst_hdr_oot_eaoff_28(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07F0_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_ycbcr_m3
#define reg_ycbcr_m3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_082C_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_21
#define reg_mst_hdr_gamma_eaoff_21(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0950_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_el_coeff_v0
#define reg_el_coeff_v0(_val, reg, _obj) do { \
	dv_w_regtbl_2bytes(SC1_REG_0C48_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C4C_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
	} while (0)

	//reg_bl_pivot_u0
#define reg_bl_pivot_u0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0308_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_mst_hdr_oot_output_limit_0
#define reg_mst_hdr_oot_output_limit_0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0738_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_csa2csb_m1
#define reg_csa2csb_m1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0884_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_bl_coeff_v32
#define reg_bl_coeff_v32(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03F0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_05
#define reg_mst_hdr_oot_eaoff_05(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0794_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_uvc_low_y_min_strength
#define reg_uvc_low_y_min_strength(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B00_HDR_HVSU_BK7D + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_mst_hdr_gamma_eidb_25
#define reg_mst_hdr_gamma_eidb_25(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0910_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_mst_hdr_gamma_eaoff_10
#define reg_mst_hdr_gamma_eaoff_10(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0930_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mmr_coeff_u20
#define reg_mmr_coeff_u20(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04F4_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_31
#define reg_mst_hdr_oot_eaoff_31(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07FC_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_oot_end_diff_0_sbit
#define reg_mst_hdr_oot_end_diff_0_sbit(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0710_HDR_TOP_FE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_mmr_coeff_v10
#define reg_mmr_coeff_v10(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0574_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_30
#define reg_mst_hdr_oot_eaoff_30(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07F8_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_gamma_sp
#define reg_mst_hdr_gamma_sp(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_08D8_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_ycbcr_m6
#define reg_ycbcr_m6(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0838_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_oot_eidb_22
#define reg_mst_hdr_oot_eidb_22(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_076C_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_bl_coeff_u00
#define reg_bl_coeff_u00(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0320_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_lut_wd_dup_md
#define reg_lut_wd_dup_md(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0980_HDR_DM_FE + reg, \
	(((_val) << 13) & 0xFF00), 0x2000, _obj)

	//reg_oot_3x1_m0
#define reg_oot_3x1_m0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0714_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_csc2ipt_pq_12bits_en
#define reg_csc2ipt_pq_12bits_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08C8_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_ss_byp_en
#define reg_ss_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0974_HDR_DM_FE + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_mst_hdr_gamma_eidb_03
#define reg_mst_hdr_gamma_eidb_03(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08F8_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0xF000, _obj)

	//reg_mst_hdr_gamma_eidb_00
#define reg_mst_hdr_gamma_eidb_00(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08F8_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_bl_coeff_y20
#define reg_bl_coeff_y20(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0278_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_04
#define reg_mst_hdr_oot_eidb_04(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0748_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_bl_pivot_u3
#define reg_bl_pivot_u3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0314_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_mst_hdr_gamma_eaoff_08
#define reg_mst_hdr_gamma_eaoff_08(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_092C_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)

	//reg_mst_hdr_oot_sp
#define reg_mst_hdr_oot_sp(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0728_HDR_TOP_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_low_y_close_to_ori_th_down
#define reg_low_y_close_to_ori_th_down(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B08_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0xFF00, _obj)

	//reg_ycbcr_m4
#define reg_ycbcr_m4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0830_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_spatial_filter_coeff_ver_003
#define reg_spatial_filter_coeff_ver_003(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A48_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_csc2ipt_m1
#define reg_csc2ipt_m1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08A8_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_el_coeff_v1
#define reg_el_coeff_v1(_val, reg, _obj) do { \
	dv_w_regtbl_2bytes(SC1_REG_0C50_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C54_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
	} while (0)

	//reg_mmr_coeff_u10
#define reg_mmr_coeff_u10(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0488_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_bl_coeff_v01
#define reg_bl_coeff_v01(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03A0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_bl_coeff_u20
#define reg_bl_coeff_u20(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0350_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_bl_coeff_y72
#define reg_bl_coeff_y72(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0300_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mmr_coeff_v18
#define reg_mmr_coeff_v18(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_05C8_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_ti_byp_en
#define reg_ti_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0974_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_bl_coeff_y61
#define reg_bl_coeff_y61(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02E0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_ootf_en
#define reg_ootf_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0710_HDR_TOP_FE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_bl_pivot_y5
#define reg_bl_pivot_y5(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0234_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)


	//reg_b0103_eotf_mode
#define reg_b0103_eotf_mode(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_oot_round
#define reg_oot_round(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0710_HDR_TOP_FE + reg, \
	(((_val) << 14) & 0xFF00), 0x4000, _obj)

	//reg_mst_hdr_oot_eidb_21
#define reg_mst_hdr_oot_eidb_21(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0768_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_mst_hdr_oot_eaoff_24
#define reg_mst_hdr_oot_eaoff_24(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07E0_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_ts_byp_en
#define reg_ts_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0974_HDR_DM_FE + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_mst_hdr_oot_eidb_19
#define reg_mst_hdr_oot_eidb_19(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0764_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_mst_hdr_oot_eaoff_04
#define reg_mst_hdr_oot_eaoff_04(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0790_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_uvc_low_y_prot_slope
#define reg_uvc_low_y_prot_slope(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B04_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x3F, _obj)

	//reg_mst_hdr_gamma_eidb_02
#define reg_mst_hdr_gamma_eidb_02(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08F8_HDR_DM_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_mmr_coeff_v2
#define reg_mmr_coeff_v2(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0520_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mmr_coeff_u19
#define reg_mmr_coeff_u19(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04E8_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_hdr_de_gen_last_en
#define reg_hdr_de_gen_last_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09F8_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_uvc_low_sat_prot_slope
#define reg_uvc_low_sat_prot_slope(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B0C_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0x3F00, _obj)

	//reg_mst_hdr_oot_eaoff_22
#define reg_mst_hdr_oot_eaoff_22(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07D8_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_csc2ipt_m8
#define reg_csc2ipt_m8(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08C4_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_spatial_filter_coeff_ver_004
#define reg_spatial_filter_coeff_ver_004(_val, reg, _obj) \
	dv_w_regtbl_2bytes(SC1_REG_0A4C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_csa2csb_m2
#define reg_csa2csb_m2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0888_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_lut_w_pulse_7b
#define reg_lut_w_pulse_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0380_VDO_DM_BE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_ycbcr_shift
#define reg_ycbcr_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	(((_val) << 7) & 0xFFFF), 0x0380, _obj)

	//reg_mst_hdr_oot_eidb_14
#define reg_mst_hdr_oot_eidb_14(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_075C_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mst_hdr_gamma_eaoff_29
#define reg_mst_hdr_gamma_eaoff_29(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_SC_BK7A_59_L + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)

	//reg_bl_pivot_y7
#define reg_bl_pivot_y7(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_023C_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_bl_pivot_y3
#define reg_bl_pivot_y3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_022C_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_ycbcr_offset_0
#define reg_ycbcr_offset_0(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0844_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_lut_addr_7a
#define reg_lut_addr_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0984_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mmr_coeff_u11
#define reg_mmr_coeff_u11(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0494_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_13
#define reg_mst_hdr_gamma_eaoff_13(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0938_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_gamma_eidb_24
#define reg_mst_hdr_gamma_eidb_24(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0910_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mmr_coeff_v8
#define reg_mmr_coeff_v8(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0560_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_low_y_close_to_ori_th_up
#define reg_low_y_close_to_ori_th_up(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B08_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_oot_shift
#define reg_oot_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0710_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x07, _obj)

	//reg_mst_hdr_gamma_eaoff_30
#define reg_mst_hdr_gamma_eaoff_30(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0968_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_oot_end_diff_0
#define reg_mst_hdr_oot_end_diff_0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0730_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_uvc_low_sat_min_strength
#define reg_uvc_low_sat_min_strength(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B14_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mst_hdr_oot_eaoff_18
#define reg_mst_hdr_oot_eaoff_18(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07C8_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_gamma_eidb_14
#define reg_mst_hdr_gamma_eidb_14(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0904_HDR_DM_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)


	//reg_mst_hdr_gamma_output_limit_2
#define reg_mst_hdr_gamma_output_limit_2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08F4_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_boundary_alpha
#define reg_boundary_alpha(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B30_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x3F, _obj)

	//reg_bl_coeff_y51
#define reg_bl_coeff_y51(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02C8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_29
#define reg_mst_hdr_oot_eidb_29(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0778_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_cup420_cbcr_cross_en
#define reg_cup420_cbcr_cross_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0808_HDR_DM_FE + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_mst_hdr_gamma_eaoff_00
#define reg_mst_hdr_gamma_eaoff_00(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0918_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_gamma_eidb_18
#define reg_mst_hdr_gamma_eidb_18(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0908_HDR_DM_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_mmr_coeff_u9
#define reg_mmr_coeff_u9(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0480_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_hdr_dcsub_vs_inv
#define reg_hdr_dcsub_vs_inv(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09E8_HDR_DM_FE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)


	//reg_mst_hdr_oot_eidb_23
#define reg_mst_hdr_oot_eidb_23(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_076C_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_si_byp_en
#define reg_si_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0974_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)


	//reg_mst_hdr_gamma_eidb_16
#define reg_mst_hdr_gamma_eidb_16(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0908_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mmr_coeff_v3
#define reg_mmr_coeff_v3(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0528_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)


	//reg_b0105_gam_en
#define reg_b0105_gam_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_range_max
#define reg_range_max(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0860_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_bl_coeff_y00
#define reg_bl_coeff_y00(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0248_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_01
#define reg_mst_hdr_oot_eaoff_01(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0784_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_bl_coeff_y42
#define reg_bl_coeff_y42(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02B8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_bl_coeff_v10
#define reg_bl_coeff_v10(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03B0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_ycbcr_m2
#define reg_ycbcr_m2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0828_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_close_to_one_th_down
#define reg_close_to_one_th_down(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B1C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_spatial_filter_coeff_ver_103
#define reg_spatial_filter_coeff_ver_103(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A58_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_el_coeff_y2
#define reg_el_coeff_y2(_val, reg, _obj) do { \
	dv_w_regtbl_2bytes(SC1_REG_0C18_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj); \
	dv_w_regtbl_2bytes(SC1_REG_0C1C_HDR_BE_BK7E + reg, \
	(((_val) >> 16) & 0xFF), 0xFF, _obj); \
	} while (0)

	//reg_mst_hdr_gamma_eidb_13
#define reg_mst_hdr_gamma_eidb_13(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0904_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_uvc_high_th
#define reg_uvc_high_th(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0B34_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFFFF), 0x0FFFFF, _obj)

	//reg_spatial_filter_coeff_ver_101
#define reg_spatial_filter_coeff_ver_101(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A50_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_mmr_coeff_v14
#define reg_mmr_coeff_v14(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_05A0_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_08
#define reg_mst_hdr_oot_eidb_08(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0750_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mst_hdr_gamma_eidb_06
#define reg_mst_hdr_gamma_eidb_06(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08FC_HDR_DM_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_csa2csb_m7
#define reg_csa2csb_m7(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_089C_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


	//reg_mst_hdr_oot_eidb_10
#define reg_mst_hdr_oot_eidb_10(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0754_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mst_hdr_gamma_eidb_29
#define reg_mst_hdr_gamma_eidb_29(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0914_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_bl_coeff_u30
#define reg_bl_coeff_u30(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0368_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_19
#define reg_mst_hdr_oot_eaoff_19(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07CC_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_420repeat_en
#define reg_420repeat_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0x1000, _obj)

	//reg_lut_fast_md_7b
#define reg_lut_fast_md_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0380_VDO_DM_BE + reg, \
	(((_val) << 12) & 0xFF00), 0x1000, _obj)

	//reg_main_cr_sub_128_pre_en
#define reg_main_cr_sub_128_pre_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_SC_BK2F_70_L + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_ycbcr_m0
#define reg_ycbcr_m0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0820_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_31
#define reg_mst_hdr_gamma_eaoff_31(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0968_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_oot_eaoff_11
#define reg_mst_hdr_oot_eaoff_11(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07AC_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_b0104_byp_en
#define reg_b0104_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_mmr_coeff_u14
#define reg_mmr_coeff_u14(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04B4_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_hdrin_shift_bl
#define reg_hdrin_shift_bl(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09EC_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x03, _obj)

#define reg_hdrin_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09EC_HDR_DM_FE + reg, \
	(((_val) << 8) & 0x300), 0x300, _obj)

	//reg_hdr_dith_window
#define reg_hdr_dith_window(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0320_VDO_DM_BE + reg, \
	(((_val) << 5) & 0xFF), 0xE0, _obj)

	//reg_mst_hdr_gamma_eaoff_22
#define reg_mst_hdr_gamma_eaoff_22(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0950_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_oot_eidb_27
#define reg_mst_hdr_oot_eidb_27(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0774_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_mst_hdr_gamma_eidb_04
#define reg_mst_hdr_gamma_eidb_04(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08FC_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mst_hdr_gamma_eaoff_02
#define reg_mst_hdr_gamma_eaoff_02(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_091C_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)


	//reg_mmr_coeff_v15
#define reg_mmr_coeff_v15(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_05A8_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_mmr_coeff_u13
#define reg_mmr_coeff_u13(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_04A8_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_mst_hdr_oot_eaoff_06
#define reg_mst_hdr_oot_eaoff_06(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0798_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_bl_pivot_v3
#define reg_bl_pivot_v3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_038C_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_bl_pivot_y0
#define reg_bl_pivot_y0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0220_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_uvc_sat_gain
#define reg_uvc_sat_gain(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B50_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_oot_eidb_00
#define reg_mst_hdr_oot_eidb_00(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0740_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mst_hdr_gamma_eidb_01
#define reg_mst_hdr_gamma_eidb_01(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08F8_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFF), 0xF0, _obj)

	//reg_uvc_0_th
#define reg_uvc_0_th(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0B44_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFFFF), 0x0FFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_14
#define reg_mst_hdr_gamma_eaoff_14(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_093C_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)

	//reg_mst_hdr_oot_eidb_31
#define reg_mst_hdr_oot_eidb_31(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_077C_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_boundary_c23
#define reg_boundary_c23(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B28_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_mst_hdr_gamma_eaoff_27
#define reg_mst_hdr_gamma_eaoff_27(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0960_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_spatial_filter_coeff_hor_06
#define reg_spatial_filter_coeff_hor_06(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A38_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_lut_wd0_7b
#define reg_lut_wd0_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0388_VDO_DM_BE + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_mmr_coeff_u6
#define reg_mmr_coeff_u6(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0460_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_spatial_filter_coeff_ver_003_uv
#define reg_spatial_filter_coeff_ver_003_uv(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A60_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0x0300, _obj)

	//reg_bl_coeff_y30
#define reg_bl_coeff_y30(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0290_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_csa2csb_shift
#define reg_csa2csb_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	(((_val) << 10) & 0xFF00), 0x0C00, _obj)

	//reg_bl_coeff_y60
#define reg_bl_coeff_y60(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02D8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_11
#define reg_mst_hdr_oot_eidb_11(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0754_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_spatial_filter_coeff_hor_00
#define reg_spatial_filter_coeff_hor_00(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A20_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_mst_hdr_oot_eaoff_03
#define reg_mst_hdr_oot_eaoff_03(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_078C_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_spatial_filter_coeff_hor_05
#define reg_spatial_filter_coeff_hor_05(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A34_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_mst_hdr_oot_eaoff_12
#define reg_mst_hdr_oot_eaoff_12(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07B0_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_bl_coeff_y22
#define reg_bl_coeff_y22(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0288_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)


	//reg_bl_coeff_y71
#define reg_bl_coeff_y71(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02F8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_bl_pivot_v2
#define reg_bl_pivot_v2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0388_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_mst_hdr_gamma_end_diff_1
#define reg_mst_hdr_gamma_end_diff_1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08E8_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_ycbcr_offset_2
#define reg_ycbcr_offset_2(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0854_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_20
#define reg_mst_hdr_oot_eaoff_20(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07D0_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_lut_w_pulse_7a
#define reg_lut_w_pulse_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0980_HDR_DM_FE + reg, \
	(((_val) << 4) & 0xFFFF), 0x10, _obj)

	//reg_mmr_coeff_v19
#define reg_mmr_coeff_v19(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_05D4_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_dm_b0202_shift
#define reg_dm_b0202_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09C8_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_dm_msb_align_en
#define reg_dm_msb_align_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	(((_val) << 5) & 0xFF), 0x20, _obj)

	//reg_lut_wd3
#define reg_lut_wd3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_097C_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_bl_coeff_y41
#define reg_bl_coeff_y41(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02B0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_w_flag_clr_7a
#define reg_w_flag_clr_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0980_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFF00), 0x0200, _obj)

	//reg_mmr_coeff_v17
#define reg_mmr_coeff_v17(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_05C0_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_spatial_resampling_mode_hor_idc
#define reg_spatial_resampling_mode_hor_idc(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A04_HDR_HVSU_BK7D + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_mst_hdr_gamma_eaoff_11
#define reg_mst_hdr_gamma_eaoff_11(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0934_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)

	//reg_mst_hdr_gamma_eaoff_12
#define reg_mst_hdr_gamma_eaoff_12(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0938_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_spatial_filter_coeff_ver_102
#define reg_spatial_filter_coeff_ver_102(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A54_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_bl_dat_8b
#define reg_bl_dat_8b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A04_HDR_HVSU_BK7D + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_lut_wd1
#define reg_lut_wd1(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0990_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_range_inv_7a
#define reg_range_inv_7a(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0864_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFF), 0x01FFFF, _obj)

	//reg_bl_coeff_y02
#define reg_bl_coeff_y02(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0258_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_dc0_to_dst_sel
#define reg_dc0_to_dst_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09E4_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0x3000, _obj)


#define reg_dm_dither_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09F0_HDR_DM_FE + reg, \
	((_val) & 0x3), 0x3, _obj)


	//reg_mst_hdr_gamma_eidb_15
#define reg_mst_hdr_gamma_eidb_15(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0904_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0xF000, _obj)

	//reg_mst_hdr_oot_eaoff_14
#define reg_mst_hdr_oot_eaoff_14(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07B8_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)


	//reg_bl_coeff_y32
#define reg_bl_coeff_y32(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02A0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_close_to_one_slope_down
#define reg_close_to_one_slope_down(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B18_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x3F, _obj)

	//reg_mst_hdr_oot_eidb_06
#define reg_mst_hdr_oot_eidb_06(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_074C_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_bl_coeff_v30
#define reg_bl_coeff_v30(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03E0_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_02
#define reg_mst_hdr_oot_eaoff_02(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0788_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_gamma_eaoff_04
#define reg_mst_hdr_gamma_eaoff_04(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0920_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_gamma_eaoff_20
#define reg_mst_hdr_gamma_eaoff_20(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_094C_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)


	//reg_csa2csb_m3
#define reg_csa2csb_m3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_088C_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_0408_default setting
#define reg_0804_default(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	((_val) & 0xffff), 0xffff, _obj)

	//reg_0804_default setting
#define reg_081c_default(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	((_val) & 0xffff), 0xffff, _obj)

	//reg_0408_default setting
#define reg_09ec_default(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09EC_HDR_DM_FE + reg, \
	((_val) & 0xffff), 0xffff, _obj)




	//reg_cup420_en
#define reg_cup420_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 13) & 0xFF00), 0x2000, _obj)

	//reg_422to444_en
#define reg_422to444_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_mst_hdr_gamma_eidb_31
#define reg_mst_hdr_gamma_eidb_31(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0914_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0xF000, _obj)

	//reg_bl_coeff_y10
#define reg_bl_coeff_y10(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0260_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mapping_idc_v
#define reg_mapping_idc_v(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0204_HDR_POLY_BK40 + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_close_to_one_en
#define reg_close_to_one_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B00_HDR_HVSU_BK7D + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_b0202_byp_clamp_en
#define reg_b0202_byp_clamp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_mst_hdr_oot_eaoff_15
#define reg_mst_hdr_oot_eaoff_15(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07BC_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)


	//reg_oot_3x1_m2
#define reg_oot_3x1_m2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_071C_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)



	//reg_boundary_c13
#define reg_boundary_c13(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B20_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_aoi_en
#define reg_aoi_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09C8_HDR_DM_FE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_mst_hdr_gamma_eidb_11
#define reg_mst_hdr_gamma_eidb_11(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0900_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0xF000, _obj)

	//reg_mst_hdr_gamma_eaoff_25
#define reg_mst_hdr_gamma_eaoff_25(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0958_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_gamma_eaoff_26
#define reg_mst_hdr_gamma_eaoff_26(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_095C_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)

	//reg_autod_lut_md_7a
#define reg_autod_lut_md_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09C0_HDR_DM_FE + reg, \
	(((_val) << 15) & 0xFFFF), 0xFFFF, _obj)

#define reg_autod_trigger_md_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09C4_HDR_DM_FE + reg, \
	(((_val) << 0) & 0xFFFF), 0xFFFF, _obj)

		//reg_autod_protect
#define reg_autod_protect_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09C0_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)


	//reg_edclk_en
#define reg_edclk_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09E4_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_bl_coeff_y62
#define reg_bl_coeff_y62(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_02E8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_16
#define reg_mst_hdr_oot_eidb_16(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0760_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_b0106_byp_en
#define reg_b0106_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0804_HDR_DM_FE + reg, \
	(((_val) << 6) & 0xFF), 0x40, _obj)

	//reg_csc2ipt_m6
#define reg_csc2ipt_m6(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08BC_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_ycbcr_m7
#define reg_ycbcr_m7(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_083C_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mmr_coeff_v12
#define reg_mmr_coeff_v12(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0588_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_spatial_filter_coeff_hor_03
#define reg_spatial_filter_coeff_hor_03(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A2C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_bl_pivot_v4
#define reg_bl_pivot_v4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0390_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_bl_coeff_v22
#define reg_bl_coeff_v22(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_03D8_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_09
#define reg_mst_hdr_gamma_eaoff_09(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0930_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_oot_eaoff_07
#define reg_mst_hdr_oot_eaoff_07(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_079C_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_bl_pivot_y1
#define reg_bl_pivot_y1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0224_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_clk_lut
#define reg_clk_lut(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_CLKGEN2_25_L + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj E_DV_IP_SEL_CLK)


	//reg_hdr_dc_vs_inv
#define reg_hdr_dc_vs_inv(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09E8_HDR_DM_FE + reg, \
	(((_val) << 14) & 0xFF00), 0x4000, _obj)

	//reg_bl_pivot_v0
#define reg_bl_pivot_v0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0380_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_boundary_c32
#define reg_boundary_c32(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B2C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_w_flag_clr_7b
#define reg_w_flag_clr_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0380_VDO_DM_BE + reg, \
	(((_val) << 9) & 0xFF00), 0x0200, _obj)

	//reg_bl_coeff_u31
#define reg_bl_coeff_u31(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0370_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_b0101_clp_sel
#define reg_b0101_clp_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_087C_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x07, _obj)

	//reg_mst_hdr_gamma_eidb_12
#define reg_mst_hdr_gamma_eidb_12(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0904_HDR_DM_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_el_spatial_resampling_filter_flag
#define reg_el_spatial_resampling_filter_flag(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A04_HDR_HVSU_BK7D + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)


	//reg_lut_fast_md_7a
#define reg_lut_fast_md_7a(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0980_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0x1000, _obj)

	//reg_bl_pivot_y6
#define reg_bl_pivot_y6(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0238_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_mst_hdr_oot_eidb_17
#define reg_mst_hdr_oot_eidb_17(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0760_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_mst_hdr_gamma_eaoff_28
#define reg_mst_hdr_gamma_eaoff_28(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0960_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_oot_eaoff_16
#define reg_mst_hdr_oot_eaoff_16(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07C0_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mmr_coeff_v6
#define reg_mmr_coeff_v6(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0548_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_csc2ipt_m7
#define reg_csc2ipt_m7(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08C0_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


	//reg_mst_hdr_oot_eaoff_21
#define reg_mst_hdr_oot_eaoff_21(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07D4_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mmr_coeff_v5
#define reg_mmr_coeff_v5(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0540_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_spatial_filter_coeff_ver_002_uv
#define reg_spatial_filter_coeff_ver_002_uv(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A60_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_mmr_coeff_u21
#define reg_mmr_coeff_u21(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0500_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mmr_coeff_v13
#define reg_mmr_coeff_v13(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0594_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_oot_eidb_02
#define reg_mst_hdr_oot_eidb_02(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0744_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_low_y_close_to_ori_slope_down
#define reg_low_y_close_to_ori_slope_down(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B04_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0x3F00, _obj)

	//reg_mst_hdr_oot_eaoff_17
#define reg_mst_hdr_oot_eaoff_17(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07C4_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_mst_hdr_gamma_eidb_30
#define reg_mst_hdr_gamma_eidb_30(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0914_HDR_DM_FE + reg, \
	((_val) & 0xFF00), 0x0F00, _obj)

	//reg_spatial_filter_coeff_hor_04
#define reg_spatial_filter_coeff_hor_04(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A30_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_csc2ipt_m3
#define reg_csc2ipt_m3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08B0_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_oot_eaoff_13
#define reg_mst_hdr_oot_eaoff_13(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_07B4_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_dm_b0202_tmo_i_only_en
#define reg_dm_b0202_tmo_i_only_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B4C_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0x01, _obj)


#define reg_thdr_r2y_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0EAC_VDO_FE_DV_WP + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


	//reg_mst_hdr_gamma_eaoff_19
#define reg_mst_hdr_gamma_eaoff_19(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0948_HDR_DM_FE + reg, \
	(((_val) << 9) & 0xFFFF00), 0x03FE00, _obj)

	//reg_mst_hdr_oot_eidb_03
#define reg_mst_hdr_oot_eidb_03(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0744_HDR_TOP_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0F00, _obj)

	//reg_bl_coeff_u32
#define reg_bl_coeff_u32(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0378_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_spatial_filter_coeff_ver_001
#define reg_spatial_filter_coeff_ver_001(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A40_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_lut_wd0_7a
#define reg_lut_wd0_7a(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0988_HDR_DM_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_mmr_coeff_u0
#define reg_mmr_coeff_u0(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0420_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mmr_coeff_v0
#define reg_mmr_coeff_v0(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0508_HDR_MMR_BK41 + reg, \
	(((_val) << 8) & 0xFFFFFFFFFFFFFF00), 0xFFFFFFFFFFFFFF00, _obj)

	//reg_mst_hdr_oot_eidb_24
#define reg_mst_hdr_oot_eidb_24(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0770_HDR_TOP_FE + reg, \
	((_val) & 0xFF), 0x0F, _obj)

	//reg_mmr_coeff_v1
#define reg_mmr_coeff_v1(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0514_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_spatial_resampling_mode_ver_idc
#define reg_spatial_resampling_mode_ver_idc(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0A04_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0x300, _obj)

	//reg_mmr_coeff_u5
#define reg_mmr_coeff_u5(_val, reg, _obj)  \
	dv_w_regtbl_8bytes(SC1_REG_0454_HDR_MMR_BK41 + reg, \
	((_val) & 0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_23
#define reg_mst_hdr_gamma_eaoff_23(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0954_HDR_DM_FE + reg, \
	(((_val) << 2) & 0xFFFF), 0x07FC, _obj)

	//reg_mst_hdr_gamma_eidb_23
#define reg_mst_hdr_gamma_eidb_23(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_090C_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0xF000, _obj)

	//reg_bl_coeff_y12
#define reg_bl_coeff_y12(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC1_REG_0270_HDR_POLY_BK40 + reg, \
	((_val) & 0xFFFFFFFF), 0x3FFFFFFF, _obj)

	//reg_close_to_one_th_up
#define reg_close_to_one_th_up(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B1C_HDR_HVSU_BK7D + reg, \
	(((_val) << 8) & 0xFF00), 0xFF00, _obj)

	//reg_linear_ratio
#define reg_linear_ratio(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0B10_HDR_HVSU_BK7D + reg, \
	((_val) & 0xFFFF), 0x7FFF, _obj)

	//reg_csa2csb_m8
#define reg_csa2csb_m8(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_08A0_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_mst_hdr_gamma_eaoff_18
#define reg_mst_hdr_gamma_eaoff_18(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0948_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_el_nlq_offset_u
#define reg_el_nlq_offset_u(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0C24_HDR_BE_BK7E + reg, \
	((_val) & 0xFFFF), 0x3FF, _obj)

	//reg_ycbcr_m1
#define reg_ycbcr_m1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_0824_HDR_DM_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_csc2ipt_shift
#define reg_csc2ipt_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_081C_HDR_DM_FE + reg, \
	(((_val) << 12) & 0xFF00), 0x3000, _obj)

	//reg_hdr_id_vs_inv
#define reg_hdr_id_vs_inv(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC1_REG_09E8_HDR_DM_FE + reg, \
	(((_val) << 13) & 0xFF00), 0x2000, _obj)

	//reg_cmc
#define reg_cmc(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK2F_70_L + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_main_y_sub_16_pre_en
#define reg_main_y_sub_16_pre_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK2F_70_L + reg, \
	(((_val) << 6) & 0xFF), 0x40, _obj)

	//reg_main_y_add_16_post_en
#define reg_main_y_add_16_post_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK2F_70_L + reg, \
	(((_val) << 7) & 0xFF), 0x80, _obj)

	//reg_main_cb_add_128_post_en
#define reg_main_cb_add_128_post_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK2F_70_L + reg, \
	(((_val) << 9) & 0xFF00), 0x0200, _obj)

	//reg_main_cr_add_128_post_en
#define reg_main_cr_add_128_post_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK2F_70_L + reg, \
	(((_val) << 5) & 0xFF), 0x20, _obj)

	//reg_main_band1_peaking_en
#define reg_main_band1_peaking_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK2F_60_L + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_main_band2_peaking_en
#define reg_main_band2_peaking_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK2F_60_L + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_main_dlc_ycv_en
#define reg_main_dlc_ycv_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_48_L + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_main_dlc_ycv_256_en
#define reg_main_dlc_ycv_256_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_48_L + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_main_dlc_adp_en
#define reg_main_dlc_adp_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_49_L + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_main_dlc_adp_lpf_en
#define reg_main_dlc_adp_lpf_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_49_L + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_main_dlc_adp_skin_en
#define reg_main_dlc_adp_skin_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_49_L + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_main_dlc_adp_cplx_en
#define reg_main_dlc_adp_cplx_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_49_L + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_main_dlc_adp_skin_prot_en
#define reg_main_dlc_adp_skin_prot_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_49_L + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_main_dlc_adp_skin_thrd
#define reg_main_dlc_adp_skin_thrd(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_4A_L + reg, \
	((_val) & 0xFF), 0x3F, _obj)

	//reg_main_dlc_adp_skin_gain
#define reg_main_dlc_adp_skin_gain(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_4A_L + reg, \
	(((_val) << 8) & 0xFF00), 0x1F00, _obj)

	//reg_main_dlc_adp_cplx_thrd
#define reg_main_dlc_adp_cplx_thrd(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_4B_L + reg, \
	((_val) & 0xFF), 0x7F, _obj)

	//reg_main_dlc_adp_cplx_gain
#define reg_main_dlc_adp_cplx_gain(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_4B_L + reg, \
	(((_val) << 8) & 0xFF00), 0x1F00, _obj)

	//reg_main_dlc_adp_cplx_region_step
#define reg_main_dlc_adp_cplx_region_step(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_4B_L + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_main_dlc_adp_skin_prot_thrd
#define reg_main_dlc_adp_skin_prot_thrd(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_4C_L + reg, \
	((_val) & 0xFF), 0x7F, _obj)

	//reg_main_dlc_adp_skin_prot_gain
#define reg_main_dlc_adp_skin_prot_gain(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_4C_L + reg, \
	(((_val) << 8) & 0xFF00), 0x1F00, _obj)

	//reg_ycv_dither_en
#define reg_ycv_dither_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_48_L + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_main_bw_en
#define reg_main_bw_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK55_03_L + reg, \
	(((_val) << 7) & 0xFF), 0x80, _obj)

	//reg_main_uvc_en
#define reg_main_uvc_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_10_L + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_main_uvc_czto1_en
#define reg_main_uvc_czto1_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_10_L + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_main_uvc_low_y_czto_ori_en
#define reg_main_uvc_low_y_czto_ori_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_10_L + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_main_uvc_low_y_sat_prot_en
#define reg_main_uvc_low_y_sat_prot_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SC_BK5D_10_L + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_hdr2ip_force_ack
#define reg_hdr2ip_force_ack(_val, reg, _obj) \
	dv_w_regtbl_2bytes(SC1_REG_0618_HDR_TOP_FE + reg, \
	((_val) & 0xff), 0xff, _obj)

	//reg_hdr2ip_path_en
#define reg_hdr2ip_path_en(_val, reg, _obj) \
	dv_w_regtbl_2bytes(SC1_REG_061C_HDR_TOP_FE + reg, \
	((_val) & 0xff), 0x2, _obj)


	//reg_hdr2ip_froce_ack_defalut
#define reg_0618_default(_val, reg, _obj) \
	dv_w_regtbl_2bytes(SC1_REG_0618_HDR_TOP_FE + reg, \
	((_val) & 0xffff), 0xffff, _obj)

	//reg_hdr2ip_path_en_default
#define reg_061c_default(_val, reg, _obj) \
	dv_w_regtbl_2bytes(SC1_REG_061C_HDR_TOP_FE + reg, \
	((_val) & 0xffff), 0xffff, _obj)

	//reg_hdr_path_auto_en_default
#define reg_0634_default(_val, reg, _obj) \
	dv_w_regtbl_2bytes(SC1_REG_0634_HDR_TOP_FE + reg, \
	((_val) & 0xffff), 0xffff, _obj)



	//hdr12bit path and compose in
#define reg_hdr12b_compose_in(_val, reg, _obj) \
	dv_w_regtbl_2bytes(SC1_REG_06D0_HDR_TOP_FE + reg, \
	((_val) & 0xFFFF), 0x8001, _obj)

	//mhdr adl
#define reg_mhdr_read_dram_lsb_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_MHDR_ADL_ADDR_LSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_mhdr_read_dram_msb_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_MHDR_ADL_ADDR_MSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_mhdr_adl_depth(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_MHDR_ADL_DEPTH + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_mhdr_adl_length(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_MHDR_ADL_LEN + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_mhdr_adl_init_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_MHDR_ADL_INIT_ADDR + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_mhdr_adl_clk(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_MHDR_ADL_EN + reg, \
	((_val) & 0x7), 0x7, _obj)

#define reg_mhdr_adl_wg(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_MHDR_WATCH_DOG + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//shdr adl
#define reg_shdr_read_dram_lsb_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SHDR_ADL_ADDR_LSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_shdr_read_dram_msb_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SHDR_ADL_ADDR_MSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_shdr_adl_depth(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SHDR_ADL_DEPTH + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_shdr_adl_length(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SHDR_ADL_LEN + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_shdr_adl_init_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SHDR_ADL_INIT_ADDR + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_shdr_adl_clk(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SHDR_ADL_EN + reg, \
	((_val) & 0x7), 0x7, _obj)

#define reg_shdr_adl_wg(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_SHDR_WATCH_DOG + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

//gfx_fe hdr reg define

#define reg_9100_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0100_GFX_DV_WP + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_9204_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0204_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_921c_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_021C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)\

#define reg_93ec_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03EC_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)




	//reg_b0102_byp_en
#define reg_b0102_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0204_GFX_HDR_FE + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_csc2ipt_m7
#define reg_csc2ipt_m7_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02C0_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_lut_fast_md
#define reg_lut_fast_md_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0380_GFX_HDR_FE + reg, \
	(((_val) << 12) & 0xFF00), 0x1000, _obj)

	//reg_ycbcr_offset_0
#define reg_ycbcr_offset_0_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_0244_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_b0103_eotf_mode
#define reg_b0103_eotf_mode_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_021C_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_csc2ipt_pq_12bits_en
#define reg_csc2ipt_pq_12bits_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02C8_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_csc2ipt_m0
#define reg_csc2ipt_m0_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02A4_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_csa2csb_m1
#define reg_csa2csb_m1_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0284_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_ycbcr_offset_1
#define reg_ycbcr_offset_1_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_024C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_csa2csb_m4
#define reg_csa2csb_m4_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0290_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_autod_protect
#define reg_autod_protect_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C0_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x07, _obj)

	//reg_csc2ipt_shift
#define reg_csc2ipt_shift_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_021C_GFX_HDR_FE + reg, \
	(((_val) << 12) & 0xFF00), 0x3000, _obj)

	//reg_csa2csb_m7
#define reg_csa2csb_m7_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_029C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_csc2ipt_m2
#define reg_csc2ipt_m2_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02AC_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0202_byp_clamp_en
#define reg_b0202_byp_clamp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0204_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_lut_wd1
#define reg_lut_wd1_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_0390_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_csa2csb_m6
#define reg_csa2csb_m6_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0298_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_hdr_gfx_sw_rst
#define reg_hdr_gfx_sw_rst_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03E8_GFX_HDR_FE + reg, \
	(((_val) << 2) & 0xFF), 0x0C, _obj)

	//reg_ycbcr_m6
#define reg_ycbcr_m6_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0238_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_clip_max
#define reg_clip_max_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_026C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_autod_err_clr
#define reg_autod_err_clr_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C0_GFX_HDR_FE + reg, \
	(((_val) << 9) & 0xFF00), 0x0200, _obj)

	//reg_ycbcr_m8
#define reg_ycbcr_m8_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0240_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0106_byp_en
#define reg_b0106_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0204_GFX_HDR_FE + reg, \
	(((_val) << 6) & 0xFF), 0x40, _obj)

	//reg_ti_byp_en
#define reg_ti_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0374_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_dbg_ohdr_en
#define reg_dbg_ohdr_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02E8_GFX_HDR_FE + reg, \
	(((_val) << 7) & 0xFF), 0x80, _obj)

	//reg_ycbcr_shift
#define reg_ycbcr_shift_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_021C_GFX_HDR_FE + reg, \
	(((_val) << 7) & 0xFFFF), 0x0380, _obj)

	//reg_csa2csb_m3
#define reg_csa2csb_m3_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_028C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_ycbcr_m7
#define reg_ycbcr_m7_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_023C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_aoi_h_threshold_low
#define reg_aoi_h_threshold_low_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03D8_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_b0104_byp_en
#define reg_b0104_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0204_GFX_HDR_FE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_lut_rd3
#define reg_lut_rd3_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0378_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x7FFF, _obj)

	//reg_ycbcr_m5
#define reg_ycbcr_m5_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0234_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_autod_lut_md
#define reg_autod_lut_md_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C0_GFX_HDR_FE + reg, \
	(((_val) << 15) & 0xFFFF), 0xFFFF, _obj)

#define reg_autod_trigger_md_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C4_GFX_HDR_FE + reg, \
	(((_val) << 0) & 0xFFFF), 0xFFFF, _obj)


	//reg_ycbcr_m4
#define reg_ycbcr_m4_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0230_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_w_flag_clr
#define reg_w_flag_clr_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0380_GFX_HDR_FE + reg, \
	(((_val) << 9) & 0xFF00), 0x0200, _obj)

	//reg_csc2ipt_m5
#define reg_csc2ipt_m5_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02B8_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_ycbcr_offset_2
#define reg_ycbcr_offset_2_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_0254_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_lut_rd1
#define reg_lut_rd1_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_03B0_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_csc2ipt_m1
#define reg_csc2ipt_m1_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02A8_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0103_byp_en
#define reg_b0103_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0204_GFX_HDR_FE + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_lut_w_pulse
#define reg_lut_w_pulse_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0380_GFX_HDR_FE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_csc2ipt_m8
#define reg_csc2ipt_m8_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02C4_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_lut_load_en
#define reg_lut_load_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0380_GFX_HDR_FE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_aoi_v_threshold_hig
#define reg_aoi_v_threshold_hig_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03DC_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x0FFF, _obj)

	//reg_csa2csb_m8
#define reg_csa2csb_m8_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02A0_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_range_min
#define reg_range_min_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_025C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_bist_fail_vdo_dm_fe
#define reg_bist_fail_vdo_dm_fe_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_02CC_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0x0FFFFFFF, _obj)

	//reg_lut_init_addr
#define reg_lut_init_addr_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03A4_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_csc2ipt_m3
#define reg_csc2ipt_m3_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02B0_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_dbg_ohdr_sel
#define reg_dbg_ohdr_sel_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02D4_GFX_HDR_FE + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_dm_msb_align_en
#define reg_dm_msb_align_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_021C_GFX_HDR_FE + reg, \
	(((_val) << 5) & 0xFF), 0x20, _obj)

	//reg_hdr_v_size
#define reg_hdr_v_size_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03D0_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_aoi_en
#define reg_aoi_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C8_GFX_HDR_FE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_autod_trig_md0_delay
#define reg_autod_trig_md0_delay_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C4_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x7FFF, _obj)

	//reg_aoi_h_threshold_hig
#define reg_aoi_h_threshold_hig_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03D4_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_csa2csb_shift
#define reg_csa2csb_shift_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_021C_GFX_HDR_FE + reg, \
	(((_val) << 10) & 0xFF00), 0x0C00, _obj)

	//reg_lut_rd2
#define reg_lut_rd2_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_03B8_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

#define reg_r2y_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_012C_GFX_DV_WP + reg, \
	((_val) & 0xffff), 0xffff, _obj)


	//reg_dbg_even
#define reg_dbg_even_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02D4_GFX_HDR_FE + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_hdr_gfx_adl_clk_sel_mask
#define reg_hdr_gfx_adl_clk_sel_mask_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03E4_GFX_HDR_FE + reg, \
	(((_val) << 1) & 0xFF), 0x06, _obj)

	//reg_csc2ipt_m6
#define reg_csc2ipt_m6_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02BC_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_dbg_sel
#define reg_dbg_sel_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02D4_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_csa2csb_m0
#define reg_csa2csb_m0_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0280_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0102_y2r_byp_shift
#define reg_b0102_y2r_byp_shift_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_021C_GFX_HDR_FE + reg, \
	(((_val) << 2) & 0xFF), 0x0C, _obj)


	//reg_b02_byp_en
#define reg_b02_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0204_GFX_HDR_FE + reg, \
	(((_val) << 7) & 0xFF), 0x80, _obj)

	//reg_range_inv
#define reg_range_inv_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_0264_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFF), 0x1FFFF, _obj)

	//reg_aoi_v_threshold_low
#define reg_aoi_v_threshold_low_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03E0_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x0FFF, _obj)

	//reg_si_byp_en
#define reg_si_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0374_GFX_HDR_FE + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_lut_wd0
#define reg_lut_wd0_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_0388_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_csc2ipt_m4
#define reg_csc2ipt_m4_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_02B4_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_ss_byp_en
#define reg_ss_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0374_GFX_HDR_FE + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_lut_addr
#define reg_lut_addr_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0384_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x01FF, _obj)

	//reg_hdr_gfx_clk_live
#define reg_hdr_gfx_clk_live_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03E4_GFX_HDR_FE + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_hdr_gfx_sram_pd_en
#define reg_hdr_gfx_sram_pd_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03F4_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_lut_w_pulse_fast
#define reg_lut_w_pulse_fast_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0388_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_ts_byp_en
#define reg_ts_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0374_GFX_HDR_FE + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_range_max
#define reg_range_max_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0260_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_edclk_en
#define reg_edclk_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03E4_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_lut_wd2
#define reg_lut_wd2_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_0398_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_csa2csb_m5
#define reg_csa2csb_m5_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0294_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_lut_sel_ip
#define reg_lut_sel_ip_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03A0_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_autod_err
#define reg_autod_err_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C0_GFX_HDR_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0100, _obj)

	//reg_ycbcr_m0
#define reg_ycbcr_m0_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0220_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_ycbcr_m3
#define reg_ycbcr_m3_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_022C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0105_byp_en
#define reg_b0105_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0204_GFX_HDR_FE + reg, \
	(((_val) << 5) & 0xFF), 0x20, _obj)

	//reg_csa2csb_m2
#define reg_csa2csb_m2_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0288_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_lut_wd_dup_md
#define reg_lut_wd_dup_md_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0380_GFX_HDR_FE + reg, \
	(((_val) << 13) & 0xFF00), 0x2000, _obj)

	//reg_lut_sel
#define reg_lut_sel_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0380_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_lut_rb_en
#define reg_lut_rb_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0380_GFX_HDR_FE + reg, \
	(((_val) << 14) & 0xFF00), 0x4000, _obj)

	//reg_b0102_y2r_byp_en
#define reg_b0102_y2r_byp_en_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_021C_GFX_HDR_FE + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_autod_trig_md
#define reg_autod_trig_md_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C4_GFX_HDR_FE + reg, \
	(((_val) << 15) & 0xFF00), 0x8000, _obj)

	//reg_dm_b0202_shift
#define reg_dm_b0202_shift_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_03C8_GFX_HDR_FE + reg, \
	((_val) & 0xFF), 0x03, _obj)

	//reg_lut_rd0
#define reg_lut_rd0_gop(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC5_REG_GFX_03A8_GFX_HDR_FE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_w_flag_rb
#define reg_w_flag_rb_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0380_GFX_HDR_FE + reg, \
	(((_val) << 8) & 0xFF00), 0x0100, _obj)

	//reg_lut_wd3
#define reg_lut_wd3_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_037C_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0x7FFF, _obj)

	//reg_ycbcr_m2
#define reg_ycbcr_m2_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0228_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_ycbcr_m1
#define reg_ycbcr_m1_gop(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC5_REG_GFX_0224_GFX_HDR_FE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_osd_hdr_sel
#define reg_osd_hdr_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(REG_GOP_HDR_SEL + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)


	//gop fhd adl
#define reg_fhd_read_dram_lsb_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_FHD_ADL_ADDR_LSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_fhd_read_dram_msb_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_FHD_ADL_ADDR_MSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_fhd_adl_depth(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_FHD_ADL_DEPTH + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_fhd_adl_length(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_FHD_ADL_LEN + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_fhd_adl_init_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_FHD_ADL_INIT_ADDR + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_fhd_adl_clk(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_FHD_ADL_EN + reg, \
	((_val) & 0x7), 0x7, _obj)


#define reg_fhd_adl_wg(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_FHD_WATCH_DOG + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//gop uhd adl
#define reg_uhd_read_dram_lsb_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_UHD_ADL_ADDR_LSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_uhd_read_dram_msb_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_UHD_ADL_ADDR_MSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_uhd_adl_depth(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_UHD_ADL_DEPTH + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_uhd_adl_length(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_UHD_ADL_LEN + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


#define reg_uhd_adl_init_addr(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_UHD_ADL_INIT_ADDR + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_uhd_adl_clk(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_UHD_ADL_EN + reg, \
	((_val) & 0x7), 0x7, _obj)

#define reg_uhd_adl_wg(_val, reg, _obj) \
	dv_w_regtbl_2bytes(REG_GOP_UHD_WATCH_DOG + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

//vdo be reg define
#define reg_b204_defalut(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0204_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_b320_defalut(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0320_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_b3c8_defalut(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03C8_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)




	//reg_hdr_vdo_be_en
#define reg_hdr_vdo_be_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0204_VDO_DM_BE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_b0401_byp_en
#define reg_b0401_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0204_VDO_DM_BE + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_b0401_ipt_m0
#define reg_b0401_ipt_m0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02A4_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_m1
#define reg_b0401_ipt_m1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02A8_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_m2
#define reg_b0401_ipt_m2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02AC_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_m3
#define reg_b0401_ipt_m3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02B0_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_m4
#define reg_b0401_ipt_m4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02B4_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_m5
#define reg_b0401_ipt_m5(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02B8_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_m6
#define reg_b0401_ipt_m6(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02BC_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_m7
#define reg_b0401_ipt_m7(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02C0_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_m8
#define reg_b0401_ipt_m8(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02C4_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0401_ipt_shift
#define reg_b0401_ipt_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_021C_VDO_DM_BE + reg, \
	(((_val) << 12) & 0xFF00), 0x3000, _obj)

	//reg_b0402_byp_en
#define reg_b0402_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0204_VDO_DM_BE + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_b0403_byp_en
#define reg_b0403_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0204_VDO_DM_BE + reg, \
	(((_val) << 3) & 0xFF), 0x08, _obj)

	//reg_b0403_csa2csb_m0
#define reg_b0403_csa2csb_m0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0280_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_m1
#define reg_b0403_csa2csb_m1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0284_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_m2
#define reg_b0403_csa2csb_m2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0288_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_m3
#define reg_b0403_csa2csb_m3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_028C_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_m4
#define reg_b0403_csa2csb_m4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0290_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_m5
#define reg_b0403_csa2csb_m5(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0294_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_m6
#define reg_b0403_csa2csb_m6(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0298_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_m7
#define reg_b0403_csa2csb_m7(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_029C_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_m8
#define reg_b0403_csa2csb_m8(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_02A0_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0403_csa2csb_shift
#define reg_b0403_csa2csb_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_021C_VDO_DM_BE + reg, \
	(((_val) << 10) & 0xFF00), 0x0C00, _obj)

	//reg_b0403_csa2csb_clp_max_0
#define reg_b0403_csa2csb_clp_max_0(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC2_REG_0318_VDO_DM_BE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_b0403_csa2csb_clp_min_0
#define reg_b0403_csa2csb_clp_min_0(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC2_REG_030C_VDO_DM_BE + reg, \
	((_val) & 0xFFFFFFFF), 0xFFFFFFFF, _obj)

	//reg_b0404_byp_en
#define reg_b0404_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0204_VDO_DM_BE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_b0404_oetf_mode
#define reg_b0404_oetf_mode(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_021C_VDO_DM_BE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_b0405_byp_en
#define reg_b0405_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0204_VDO_DM_BE + reg, \
	(((_val) << 5) & 0xFF), 0x20, _obj)

	//reg_b0405_rgb_m0
#define reg_b0405_rgb_m0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0220_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_m1
#define reg_b0405_rgb_m1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0224_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_m2
#define reg_b0405_rgb_m2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0228_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_m3
#define reg_b0405_rgb_m3(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_022C_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_m4
#define reg_b0405_rgb_m4(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0230_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_m5
#define reg_b0405_rgb_m5(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0234_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_m6
#define reg_b0405_rgb_m6(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0238_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_m7
#define reg_b0405_rgb_m7(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_023C_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_m8
#define reg_b0405_rgb_m8(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0240_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

	//reg_b0405_rgb_offset_0
#define reg_b0405_rgb_offset_0(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0244_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_b0405_rgb_offset_1
#define reg_b0405_rgb_offset_1(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_024C_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_b0405_rgb_offset_2
#define reg_b0405_rgb_offset_2(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0254_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0x1FFF, _obj)

	//reg_b0405_rgb_shift
#define reg_b0405_rgb_shift(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_021C_VDO_DM_BE + reg, \
	(((_val) << 7) & 0xFFFF), 0x0180, _obj)

	//reg_range_min_7b
#define reg_range_min_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_025C_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0x0FFF, _obj)

	//reg_range_inv_7b
#define reg_range_inv_7b(_val, reg, _obj)  \
	dv_w_regtbl_4bytes(SC2_REG_0264_VDO_DM_BE + reg, \
	((_val) & 0xFFFFFF), 0x01FFFF, _obj)

	//reg_b0406_444to422_byp_en
#define reg_b0406_444to422_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0204_VDO_DM_BE + reg, \
	(((_val) << 6) & 0xFF), 0x40, _obj)

	//reg_b0406_444to422_cbcr_swap
#define reg_b0406_444to422_cbcr_swap(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0210_VDO_DM_BE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_b0406_clp_sel
#define reg_b0406_clp_sel(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_027C_VDO_DM_BE + reg, \
	((_val) & 0xF), 0x3, _obj)

	//reg_hdr_dith_en
#define reg_hdr_dith_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0320_VDO_DM_BE + reg, \
	((_val) & 0xFF), 0x01, _obj)


	//reg_hdr_dith_444md
#define reg_hdr_dith_444md(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0320_VDO_DM_BE + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)

	//reg_hdr_dith_8b_md
#define reg_hdr_dith_8b_md(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0320_VDO_DM_BE + reg, \
	(((_val) << 2) & 0xFF), 0x04, _obj)

	//reg_hdr_pseudo_dith_stop
#define reg_hdr_pseudo_dith_stop(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(SC2_REG_0320_VDO_DM_BE + reg, \
	(((_val) << 8) & 0xFF00), 0x0100, _obj)

	//reg_hdr_dith_force_window
#define reg_hdr_dith_force_window(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_0320_VDO_DM_BE + reg, \
	(((_val) << 4) & 0xFF), 0x10, _obj)

	//reg_reorder_en
#define reg_reorder_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03C8_VDO_DM_BE + reg, \
	(((_val) << 1) & 0xFF), 0x02, _obj)


	//reg_byp_y2r_reorder_disable
#define reg_byp_y2r_reorder_disable(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03EC_VDO_DM_BE + reg, \
	(((_val) << 12) & 0xFF00), 0x1000, _obj)

	//reg_autod_lut_md_7b
#define reg_autod_lut_md_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03C0_VDO_DM_BE + reg, \
	(((_val) << 15) & 0xFF00), 0x8001, _obj)

#define reg_autod_trigger_md_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03C4_VDO_DM_BE + reg, \
	(((_val) << 0) & 0xFFFF), 0xFFFF, _obj)


#define reg_scamble_v_size_7b(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03D0_VDO_DM_BE + reg, \
	(_val&0xFFFF), 0xFFFF, _obj)

	//reg_scramble_byp_en
#define reg_scramble_byp_en(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03C8_VDO_DM_BE + reg, \
	((_val) & 0xFF), 0x01, _obj)

	//reg_meta_pkt_num
#define reg_meta_pkt_num(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03DC_VDO_DM_BE + reg, \
	((_val) & 0xFF), 0xFF, _obj)

	//reg_meta_pkt_repeat_num
#define reg_meta_pkt_repeat_num(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03D8_VDO_DM_BE + reg, \
	((_val) & 0xFF), 0x07, _obj)

	//reg_meta_len_per_pkt
#define reg_meta_len_per_pkt(_val, reg, _obj)  \
	dv_w_regtbl_2bytes(SC2_REG_03E0_VDO_DM_BE + reg, \
	((_val) & 0xFFFF), 0x0FFF, _obj)

//add for EMU scramble adl
	//scramble addr lsb
#define reg_scramble_read_dram_lsb_addr(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(REG_SCM_ADL_ADDR_LSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_scramble_read_dram_msb_addr(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(REG_SCM_ADL_ADDR_MSB + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_scramble_adl_depth(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(REG_SCM_ADL_DEPTH + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

#define reg_scramble_adl_length(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(REG_SCM_ADL_LEN + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


#define reg_scramble_adl_init_addr(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(REG_SCM_ADL_INIT_ADDR + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)


#define reg_scramble_clientc_clk(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(REG_SCM_ADL_CLIC_CLK + reg, \
	((_val) & 0xFF), 0xFF, _obj)


#define reg_scramble_adl_clk(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(REG_SCM_ADL_EN + reg, \
	((_val) & 0x7), 0x7, _obj)


#define reg_scramble_adl_wg(_val, reg, _obj)   \
	dv_w_regtbl_2bytes(REG_SCM_WATCH_DOG + reg, \
	((_val) & 0xFFFF), 0xFFFF, _obj)

extern uint32_t dovi_out_height;
extern uint32_t dovi_out_width;
extern enum dovi_signal_format_t dovi_out_format;
extern uint32_t adl_mode;
extern uint32_t dovi_idk_test;
int dovi_get_output_setting(struct dv_hw_reg *p_hw_params,
	uint8_t is_dolby_src);

#endif
