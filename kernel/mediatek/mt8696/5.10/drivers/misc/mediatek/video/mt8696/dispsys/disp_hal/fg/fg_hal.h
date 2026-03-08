/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef __FG_HAL_H__
#define __FG_HAL_H__

#include "disp_info.h"

#ifndef MS_S64
#define MS_S64		s64
#endif

#ifndef MS_U32
#define MS_U32		u32
#endif

#ifndef MS_U16
#define MS_U16		u16
#endif

#ifndef MS_U8
#define MS_U8		u8
#endif

#ifndef MS_BOOL
#define MS_BOOL		bool
#endif

/* 256 x 3 = 768 */
#define FG_TBL_LUT_SIZE			(FILMG_SF_LEN * FILMG_SF_CMD)
/* 803 x 10 = 8030 */
#define FG_TBL_Y_GNS_SIZE		(FILMG_YN_LEN * FILMG_YN_CMD)
/* 768 */
#define FG_TBL_Y_GNS_OFFSET		(FG_TBL_LUT_SIZE)
/* 768 + 8030 = 8798 */
#define FG_TBL_C_GNS_OFFSET		(FG_TBL_Y_GNS_OFFSET + FG_TBL_Y_GNS_SIZE)

#define FG_LUMA_BLOCK_SIZE_Y		73
#define FG_LUMA_BLOCK_SIZE_x		82

#define FG_LUMA_GNS_PITCH		10
#define FG_CHROMA_GNS_PITCH		20

enum fg_hw_id {
	MAIN_FG,
	SUB_FG,
	MAX_FG,
};

struct fg_hw_reg_output {
	MS_U16 grain_seed;
	MS_U8 num_y_points;
	MS_U8 num_cb_points;
	MS_U8 num_cr_points;
	MS_U8 chroma_scaling_from_luma;
	MS_U8 clip_to_restricted_range;
	MS_U8 mc_identity;
	MS_U8 cb_mult;
	MS_U8 cb_luma_mult;
	MS_U16 cb_offset;
	MS_U8 cr_mult;
	MS_U8 cr_luma_mult;
	MS_U16 cr_offset;
	MS_U8 grain_scaling;
	MS_U8 ar_coeff_shift;
	MS_U8 bit_depth;
	MS_U8 apply_grain;
	MS_U8 update_parameters;
	MS_U8 use_high_bit_depth;
	MS_U8 overlap_flag;
	MS_U8 grain_scale_shift;

	MS_U8 parameter_hsize;
	MS_U8 parameter_vsize;

	MS_U8 y_bypass_en;
	MS_U8 cb_bypass_en;
	MS_U8 cr_bypass_en;

	MS_U8 scaling_points_y[AV1_MAX_GRAIN_POINT_CNT][2];
	MS_U8 scaling_points_cb[AV1_MAX_GRAIN_POINT_CNT][2];
	MS_U8 scaling_points_cr[AV1_MAX_GRAIN_POINT_CNT][2];

	MS_U8 ar_coeffs_y[24];
	MS_U8 ar_coeffs_cb[25];
	MS_U8 ar_coeffs_cr[25];
};

struct fg_hw_adl_output {
	MS_U8 u8scaling_lut_y256[256];
	MS_U8 u8scaling_lut_cb256[256];
	MS_U8 u8scaling_lut_cr256[256];
	MS_U16 y_grain_block[5986];
	MS_U16 cb_grain_block[1672];
	MS_U16 cr_grain_block[1672];
};

extern void __iomem *disp_fg_reg_base[];

void fg_hal_bypass(enum fg_hw_id hw_id, bool bypass);

void fg_hal_update_process(enum fg_hw_id hw_id, struct fg_hw_reg_output *params_hw_reg,
			   struct fg_hw_adl_output *params_hw_adl,
			   struct adl_src_tbl *adl_tbl);
#endif
