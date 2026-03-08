/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __DISP_FG_PRE_DRV_H__
#define __DISP_FG_PRE_DRV_H__

#define FG_RANDOM_NUM_BITS		8
#define AR_PARDDING_NS			2
#define LUMA_SCALE_BLOCK_BUF_NS		2
#define CHROMA_BLOCK_BUF_NS		2
#define CHROMA_SCALE_BLOCK_BUF_NS	3

#define BASE_BIT_DEPTH			8
#define BASE_GRAIN_CENTER		128
#define BASE_GRAIN_MAX			256

#define REVISE_VAL(ori, val)	(((ori) < 0) ? ((ori) + (val)) : (ori))

struct video_scale_info;

struct fg_gns_ar_block_info {
	bool ar_block_alloc;
	bool ar_scale_up;

	int scaled_luma_ar_x;
	int scaled_luma_ar_y;

	int scaled_chroma_ar_x;
	int scaled_chroma_ar_y;

	int *luma_ar_block;
	int *scaled_luma_ar;
	int *v_scaled_luma_ar;

	int *cb_ar_block;
	int *cr_ar_block;
	int *scaled_cb_ar;
	int *scaled_cr_ar;
	int *v_scaled_c_ar;
};

struct fg_gns_basic_info {
	MS_U8 left_pad;
	MS_U8 right_pad;
	MS_U8 top_pad;
	MS_U8 bottom_pad;
	MS_U8 ar_padding;

	MS_U8 luma_block_size_y;
	MS_U8 luma_block_size_x;
	MS_U8 chroma_block_size_y;
	MS_U8 chroma_block_size_x;
	MS_U8 luma_grain_stride;
	MS_U8 chroma_grain_stride;

	int chroma_subsamp_x;
	int chroma_subsamp_y;

	int grain_center;
	int grain_min;
	int grain_max;

	int gauss_sec_shift;
	int rounding_offset;

	int luma_ar_block_size_y;
	int luma_ar_block_size_x;
	int chroma_ar_block_size_y;
	int chroma_ar_block_size_x;
	int luma_ar_stride;
	int chroma_ar_stride;
};

void disp_fg_pre_process_gns(struct mtk_av1_film_grain_params *params,
			     struct fg_gns_basic_info *gns_info,
			     struct fg_hw_adl_output *params_hw_adl,
			     struct fg_gns_ar_block_info *gns_ar_info);

void disp_fg_init_gns_basic_info(struct fg_gns_basic_info *gns_info);

void disp_fg_init_gns_grain_info(struct mtk_av1_film_grain_params *params,
				 struct fg_hw_reg_output *params_hw_reg,
				 struct fg_gns_basic_info *gns_info);

void disp_fg_update_gns_ar_block_info(struct fg_gns_basic_info *gns_info,
				      struct video_scale_info *scale_info,
				      struct fg_gns_ar_block_info *gns_ar_info);

void disp_fg_free_gns_ar_block(struct fg_gns_ar_block_info *gns_ar_info);

void disp_fg_pre_process_ar_coeffs(struct fg_hw_reg_output *hw_reg);

extern int get_random_number(int bits);
#endif /* __DISP_FG_PRE_DRV_H__ */
