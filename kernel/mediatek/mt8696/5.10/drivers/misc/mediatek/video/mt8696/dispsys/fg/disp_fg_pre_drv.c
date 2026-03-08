// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/kernel.h>

#include "disp_hw_mgr.h"
#include "disp_adl_if.h"
#include "disp_fg_if.h"
#include "fg_hal.h"
#include "disp_info.h"
#include "disp_fg_drv.h"
#include "disp_fg_pre_drv.h"
#include "disp_fg_pre_scale.h"

static void fg_luma_auto_reg_filter(struct mtk_av1_film_grain_params *params,
				    struct fg_gns_basic_info *gns_info,
				    struct fg_hw_adl_output *params_hw_adl)
{
	MS_U8 left_pad = gns_info->left_pad;
	MS_U8 right_pad = gns_info->right_pad;
	MS_U8 top_pad = gns_info->top_pad;
	MS_U8 bottom_pad = gns_info->bottom_pad;
	MS_U8 luma_block_size_y = gns_info->luma_block_size_y;
	MS_U8 luma_block_size_x = gns_info->luma_block_size_x;
	MS_U8 luma_grain_stride = gns_info->luma_grain_stride;

	MS_S32 *luma_grain_block = params_hw_adl->y_grain_block;

	int rounding_offset = gns_info->rounding_offset;
	int grain_min = gns_info->grain_min;
	int grain_max = gns_info->grain_max;
	int coef_stride = FG_GRAIN_MARGIN_NS * params->ar_coeff_lag + 1;

	int i = 0, j = 0;
	int row = 0, col = 0;

	FG_FUNC();

	FG_LOG_D("coef_stride %d grain_min %d grain_max %d\n",
		 coef_stride,
		 grain_min,
		 grain_max);

	for (i = top_pad; i < luma_block_size_y - bottom_pad; i++) {
		for (j = left_pad; j < luma_block_size_x - right_pad; j++) {
			int wsum = 0;

			for (row = -params->ar_coeff_lag; row < 0; row++) {
				for (col = -params->ar_coeff_lag;
				     col < params->ar_coeff_lag + 1; col++) {
					wsum = wsum +
					       params->ar_coeffs_y[(row +
					       params->ar_coeff_lag)*
					       (coef_stride) + (col +
					       params->ar_coeff_lag)] *
					       luma_grain_block[(i + row) *
					       luma_grain_stride + j + col];
				}
			}

			for (col = -params->ar_coeff_lag; col < 0; col++) {
				wsum = wsum +
				       params->ar_coeffs_y[params->ar_coeff_lag
				       * coef_stride +
				       (col + params->ar_coeff_lag)] *
				       luma_grain_block[i *
				       luma_grain_stride + j + col];
			}
			luma_grain_block[i * luma_grain_stride + j] =
				clamp(luma_grain_block[i * luma_grain_stride +
				j] + ((wsum + rounding_offset) >>
				params->ar_coeff_shift), grain_min, grain_max);
		}
	}

	FG_SW_FILTER("pad [%u %u %u %u] %ux%u %u lag %u rounding %d %d %d\n",
		     left_pad, top_pad,
		     right_pad, bottom_pad,
		     luma_block_size_x, luma_block_size_y,
		     luma_grain_stride, params->ar_coeff_lag,
		     rounding_offset,
		     i, j);
}

static void disp_fg_luma_crop_ar_block(struct fg_gns_basic_info *gns_info,
				      struct fg_hw_adl_output *params_hw_adl,
				      struct fg_gns_ar_block_info *gns_ar_info)
{
	MS_U8 left_pad = gns_info->left_pad;
	MS_U8 right_pad = gns_info->right_pad;
	MS_U8 top_pad = gns_info->top_pad;
	MS_U8 bottom_pad = gns_info->bottom_pad;
	MS_U8 ar_padding = gns_info->ar_padding;

	MS_U8 luma_block_size_y = gns_info->luma_block_size_y;
	MS_U8 luma_block_size_x = gns_info->luma_block_size_x;
	MS_U8 luma_grain_stride = gns_info->luma_grain_stride;

	MS_S32 *luma_grain_block = params_hw_adl->y_grain_block;

	int luma_ar_stride = gns_info->luma_ar_stride;
	int *luma_ar_block = gns_ar_info->luma_ar_block;
	int luma_ar_i_idx = 0;
	int luma_ar_j_idx = 0;
	int i = 0, j = 0;

	FG_FUNC();

	for (i = top_pad + (FG_GRAIN_MARGIN_NS * ar_padding);
	     i < luma_block_size_y - bottom_pad; i++) {
		luma_ar_j_idx = 0;
		for (j = left_pad + FG_GRAIN_MARGIN_NS * ar_padding;
		     j < luma_block_size_x - (right_pad + FG_GRAIN_MARGIN_NS *
					      ar_padding);
		     j++) {
			luma_ar_block[luma_ar_i_idx * luma_ar_stride +
				luma_ar_j_idx] = luma_grain_block[i *
				luma_grain_stride + j];
			luma_ar_j_idx++;
		}

		luma_ar_i_idx++;
	}

	FG_LOG_D("luma ar size %dx%d\n", luma_ar_i_idx, luma_ar_j_idx);
}

static void fg_luma_scale_ar_block(struct fg_gns_basic_info *gns_info,
				   struct fg_gns_ar_block_info *gns_ar_info)
{
	int luma_ar_block_size_y = gns_info->luma_ar_block_size_y;
	int luma_ar_block_size_x = gns_info->luma_ar_block_size_x;

	int scaled_luma_ar_x = gns_ar_info->scaled_luma_ar_x;
	int scaled_luma_ar_y =  gns_ar_info->scaled_luma_ar_y;

	int *luma_ar_block = gns_ar_info->luma_ar_block;
	int *scaled_luma_ar = gns_ar_info->scaled_luma_ar;
	int *v_scaled_luma_ar = gns_ar_info->v_scaled_luma_ar;

	FG_FUNC();
	FG_LOG_D("luma ar size %dx%d -> %dx%d\n",
		 luma_ar_block_size_x,
		 luma_ar_block_size_y,
		 scaled_luma_ar_x,
		 scaled_luma_ar_y);

	if ((scaled_luma_ar_x > luma_ar_block_size_x) ||
	    (scaled_luma_ar_y > luma_ar_block_size_y))
		pre_up_scale_y(luma_ar_block,
			       v_scaled_luma_ar,
			       scaled_luma_ar,
			       luma_ar_block_size_x,
			       luma_ar_block_size_y,
			       scaled_luma_ar_x,
			       scaled_luma_ar_y);
	else
		pre_down_scale(luma_ar_block,
			       v_scaled_luma_ar,
			       scaled_luma_ar,
			       luma_ar_block_size_x,
			       luma_ar_block_size_y,
			       scaled_luma_ar_x,
			       scaled_luma_ar_y);
}

static void disp_fg_luma_fill_auto_reg(struct fg_gns_basic_info *gns_info,
				      struct fg_hw_adl_output *params_hw_adl,
				      struct fg_gns_ar_block_info *gns_ar_info)
{
	MS_U8 left_pad = gns_info->left_pad;
	MS_U8 right_pad = gns_info->right_pad;
	MS_U8 top_pad = gns_info->top_pad;
	MS_U8 bottom_pad = gns_info->bottom_pad;
	MS_U8 ar_padding = gns_info->ar_padding;

	int luma_ar_i_idx = gns_info->luma_ar_block_size_x;
	int luma_ar_j_idx = gns_info->luma_ar_block_size_y;

	int scaled_luma_ar_x = gns_ar_info->scaled_luma_ar_x;
	int scaled_luma_ar_y = gns_ar_info->scaled_luma_ar_y;

	MS_U8 luma_block_size_y = gns_info->luma_block_size_y;
	MS_U8 luma_block_size_x = gns_info->luma_block_size_x;

	MS_S32 *luma_grain_block = params_hw_adl->y_grain_block;

	MS_U8 luma_grain_stride = gns_info->luma_grain_stride;

	int i = 0, j = 0;

	/* -------------------fill Y auto-regressive -------- */
	int luma_y_blocks = 0;
	int luma_x_blocks = 0;
	int luma_x_left_marginal = left_pad + (FG_GRAIN_MARGIN_NS * ar_padding);
	int luma_y_top_marginal = top_pad + (FG_GRAIN_MARGIN_NS * ar_padding);
	int scaled_luma_ar_i_idx = 0;
	int scaled_luma_ar_j_idx = 0;
	int luma_ar_offset_x = get_random_number(FG_RANDOM_NUM_BITS);
	int luma_ar_offset_y = get_random_number(FG_RANDOM_NUM_BITS);

	int max_luma_offset_x = scaled_luma_ar_x > luma_ar_j_idx ?
		scaled_luma_ar_x - luma_ar_j_idx : 0;
	int max_luma_offset_y = scaled_luma_ar_y > luma_ar_i_idx ?
		scaled_luma_ar_y - luma_ar_i_idx : 0;

	int *scaled_luma_ar = gns_ar_info->scaled_luma_ar;

	FG_FUNC();

	FG_LOG_D("luma ar %dx%d -> %dx%d max offset %dx%d\n",
		 luma_ar_i_idx, luma_ar_j_idx,
		 scaled_luma_ar_x, scaled_luma_ar_y,
		 max_luma_offset_x, max_luma_offset_y);

	if (max_luma_offset_x > 0)
		luma_ar_offset_x = (luma_ar_offset_x * max_luma_offset_x) >>
				   FG_RANDOM_NUM_BITS;
	else
		luma_ar_offset_x = 0;


	if (max_luma_offset_y > 0)
		luma_ar_offset_y = (luma_ar_offset_y * max_luma_offset_y) >>
				   FG_RANDOM_NUM_BITS;
	else
		luma_ar_offset_y = 0;

	for (i = top_pad + (FG_GRAIN_MARGIN_NS * ar_padding);
	     i < luma_block_size_y - bottom_pad; i++) {
		for (j = left_pad + FG_GRAIN_MARGIN_NS * ar_padding;
		     j < luma_block_size_x - (right_pad + FG_GRAIN_MARGIN_NS *
					      ar_padding);
		     j++) {
			luma_x_blocks = (j - luma_x_left_marginal) /
				scaled_luma_ar_x;
			luma_y_blocks = (i - luma_y_top_marginal) /
				scaled_luma_ar_y;
			scaled_luma_ar_i_idx = (i - luma_y_top_marginal) -
				(scaled_luma_ar_y * luma_y_blocks) +
				luma_ar_offset_y;
			scaled_luma_ar_j_idx = (j - luma_x_left_marginal) -
				(scaled_luma_ar_x * luma_x_blocks) +
				luma_ar_offset_x;
			luma_grain_block[i * luma_grain_stride + j] =
				scaled_luma_ar[scaled_luma_ar_i_idx *
				scaled_luma_ar_x + scaled_luma_ar_j_idx];
		}
	}
}

static void fg_luma_fill_auto_scale_down(struct fg_gns_basic_info *gns_info,
					 struct fg_hw_adl_output *params_hw_adl,
					 struct fg_gns_ar_block_info *gns_ar)
{
	MS_U8 left_pad = gns_info->left_pad;
	MS_U8 right_pad = gns_info->right_pad;
	MS_U8 top_pad = gns_info->top_pad;
	MS_U8 bottom_pad = gns_info->bottom_pad;
	MS_U8 ar_padding = gns_info->ar_padding;

	int luma_ar_i_idx = gns_info->luma_ar_block_size_x;
	int luma_ar_j_idx = gns_info->luma_ar_block_size_y;

	int scaled_luma_ar_x = gns_ar->scaled_luma_ar_x;
	int scaled_luma_ar_y = gns_ar->scaled_luma_ar_y;

	MS_U8 luma_block_size_y = gns_info->luma_block_size_y;
	MS_U8 luma_block_size_x = gns_info->luma_block_size_x;

	MS_S32 *luma_grain_block = params_hw_adl->y_grain_block;

	MS_U8 luma_grain_stride = gns_info->luma_grain_stride;

	int i = 0, j = 0;

	/* -------------------fill Y auto-regressive -------- */
	int luma_y_blocks = 0;
	int luma_x_blocks = 0;
	int luma_x_left_marginal = left_pad + (FG_GRAIN_MARGIN_NS * ar_padding);
	int luma_y_top_marginal = top_pad + (FG_GRAIN_MARGIN_NS * ar_padding);
	int scaled_luma_ar_i_idx = 0;
	int scaled_luma_ar_j_idx = 0;
	int luma_ar_offset_x = get_random_number(FG_RANDOM_NUM_BITS);
	int luma_ar_offset_y = get_random_number(FG_RANDOM_NUM_BITS);

	int max_luma_offset_x = scaled_luma_ar_x > luma_ar_j_idx ?
		scaled_luma_ar_x - luma_ar_j_idx : 0;
	int max_luma_offset_y = scaled_luma_ar_y > luma_ar_i_idx ?
		scaled_luma_ar_y - luma_ar_i_idx : 0;

	int *scaled_luma_ar = gns_ar->scaled_luma_ar;

	FG_FUNC();

	FG_LOG_D("luma ar %dx%d -> %dx%d max offset %dx%d\n",
		 luma_ar_i_idx, luma_ar_j_idx,
		 scaled_luma_ar_x, scaled_luma_ar_y,
		 max_luma_offset_x, max_luma_offset_y);

	if (max_luma_offset_x > 0)
		luma_ar_offset_x = (luma_ar_offset_x * max_luma_offset_x) >>
				   FG_RANDOM_NUM_BITS;
	else
		luma_ar_offset_x = 0;

	if (max_luma_offset_y > 0)
		luma_ar_offset_y = (luma_ar_offset_y * max_luma_offset_y) >>
				   FG_RANDOM_NUM_BITS;
	else
		luma_ar_offset_y = 0;

	for (i = top_pad + (AR_PARDDING_NS * ar_padding);
	     i < luma_block_size_y - bottom_pad; i++) {
		for (j = left_pad + FG_GRAIN_MARGIN_NS * ar_padding;
		     j < luma_block_size_x - (right_pad + FG_GRAIN_MARGIN_NS *
					      ar_padding);
		     j++) {
			luma_x_blocks = (j - luma_x_left_marginal) /
				scaled_luma_ar_x;
			luma_y_blocks = (i - luma_y_top_marginal) /
				scaled_luma_ar_y;
			scaled_luma_ar_i_idx = (i - luma_y_top_marginal) %
				scaled_luma_ar_y;
			scaled_luma_ar_j_idx = (j - luma_x_left_marginal) %
				scaled_luma_ar_x;
			luma_grain_block[i * luma_grain_stride + j] =
				scaled_luma_ar[scaled_luma_ar_i_idx *
				scaled_luma_ar_x + scaled_luma_ar_j_idx];
		}
	}
}

static void disp_fg_luma_sw_scale(struct mtk_av1_film_grain_params *params,
				  struct fg_gns_basic_info *gns_info,
				  struct fg_hw_adl_output *params_hw_adl,
				  struct fg_gns_ar_block_info *gns_ar_info)
{
	disp_fg_luma_crop_ar_block(gns_info, params_hw_adl, gns_ar_info);
	fg_luma_scale_ar_block(gns_info, gns_ar_info);
	if (gns_ar_info->ar_scale_up)
		disp_fg_luma_fill_auto_reg(gns_info, params_hw_adl,
					   gns_ar_info);
	else
		fg_luma_fill_auto_scale_down(gns_info, params_hw_adl,
					     gns_ar_info);
}

static void fg_chroma_auto_reg_filter(struct mtk_av1_film_grain_params *params,
				      struct fg_gns_basic_info *gns_info,
				      struct fg_hw_adl_output *params_hw_adl)
{
	int coef_stride = AR_PARDDING_NS * params->ar_coeff_lag + 1;
	MS_U8 left_pad = gns_info->left_pad;
	MS_U8 right_pad = gns_info->right_pad;
	MS_U8 top_pad = gns_info->top_pad;
	MS_U8 bottom_pad = gns_info->bottom_pad;
	MS_U8 chroma_block_size_y = gns_info->chroma_block_size_y;
	MS_U8 chroma_block_size_x = gns_info->chroma_block_size_x;
	int rounding_offset = gns_info->rounding_offset;
	int grain_min = gns_info->grain_min;
	int grain_max = gns_info->grain_max;
	MS_U8 luma_grain_stride = gns_info->luma_grain_stride;
	MS_U8 chroma_grain_stride = gns_info->chroma_grain_stride;

	MS_S32 *luma_grain_block = params_hw_adl->y_grain_block;
	MS_S32 *cb_grain_block = params_hw_adl->cb_grain_block;
	MS_S32 *cr_grain_block = params_hw_adl->cr_grain_block;

	int chroma_subsamp_x = gns_info->chroma_subsamp_x;
	int chroma_subsamp_y = gns_info->chroma_subsamp_y;

	int i = 0, j = 0;
	int row = 0, col = 0;
	int k = 0, l = 0;

	FG_FUNC();

	for (i = top_pad; i < chroma_block_size_y - bottom_pad; i++) {
		for (j = left_pad; j < chroma_block_size_x - right_pad; j++) {
			int wsum_cb = 0;
			int wsum_cr = 0;
			int av_luma = 0;
			int luma_coord_y = ((i - top_pad) << chroma_subsamp_y) +
				top_pad;
			int luma_coord_x = ((j - left_pad) << chroma_subsamp_x)
				+ left_pad;

			for (row = -params->ar_coeff_lag; row < 0; row++) {
				for (col = -params->ar_coeff_lag;
				     col < params->ar_coeff_lag + 1; col++) {
					wsum_cb = wsum_cb +
						params->ar_coeffs_cb[(row +
						params->ar_coeff_lag) *
						(coef_stride)+(col +
						params->ar_coeff_lag)] *
						cb_grain_block[(i + row) *
						chroma_grain_stride + j + col];
					wsum_cr = wsum_cr +
						params->ar_coeffs_cr[(row +
						params->ar_coeff_lag) *
						(coef_stride)+(col +
						params->ar_coeff_lag)] *
						cr_grain_block[(i + row) *
						chroma_grain_stride + j + col];
				}
			}

			for (col = -params->ar_coeff_lag; col < 0; col++) {
				wsum_cb = wsum_cb +
					params->ar_coeffs_cb[
					params->ar_coeff_lag *
					coef_stride + (col +
					params->ar_coeff_lag)] *
					cb_grain_block[(i) *
					chroma_grain_stride + j + col];
				wsum_cr = wsum_cr +
					params->ar_coeffs_cr[
					params->ar_coeff_lag *
					coef_stride +
					(col + params->ar_coeff_lag)] *
					cr_grain_block[(i)*
					chroma_grain_stride + j + col];
			}

			if (params->num_y_points > 0) {
				for (k = luma_coord_y;
				     k < luma_coord_y + chroma_subsamp_y + 1;
				     k++)
					for (l = luma_coord_x;
					     l < luma_coord_x +
					     chroma_subsamp_x + 1;
					     l++)
						av_luma +=
						luma_grain_block[k *
						luma_grain_stride + l];
				av_luma =
					(av_luma +
					((1 << (chroma_subsamp_y +
					chroma_subsamp_x)) >> 1)) >>
					(chroma_subsamp_y + chroma_subsamp_x);
				wsum_cb = wsum_cb +
					params->ar_coeffs_cb[
					params->ar_coeff_lag *
					coef_stride + params->ar_coeff_lag] *
					av_luma;
				wsum_cr = wsum_cr +
					params->ar_coeffs_cr[
					params->ar_coeff_lag *
					coef_stride + params->ar_coeff_lag] *
					av_luma;
			}

			if (params->num_cb_points ||
			    params->chroma_scaling_from_luma) {
				cb_grain_block[i * chroma_grain_stride + j] =
					clamp(cb_grain_block[i *
					chroma_grain_stride + j] +
					((wsum_cb + rounding_offset) >>
					params->ar_coeff_shift), grain_min,
					grain_max);
			}

			if (params->num_cr_points ||
			    params->chroma_scaling_from_luma) {
				cr_grain_block[i * chroma_grain_stride + j] =
					clamp(cr_grain_block[i *
					chroma_grain_stride + j] +
					((wsum_cr + rounding_offset) >>
					params->ar_coeff_shift), grain_min,
					grain_max);
			}
		}
	}
}

static void fg_chroma_crop_ar_block(struct fg_gns_basic_info *gns_info,
				    struct fg_hw_adl_output *params_hw_adl,
				    struct fg_gns_ar_block_info *gns_ar_info)
{
	MS_U8 left_pad = gns_info->left_pad;
	MS_U8 right_pad = gns_info->right_pad;
	MS_U8 top_pad = gns_info->top_pad;
	MS_U8 bottom_pad = gns_info->bottom_pad;
	MS_U8 ar_padding = gns_info->ar_padding;

	MS_U8 chroma_block_size_y = gns_info->chroma_block_size_y;
	MS_U8 chroma_block_size_x = gns_info->chroma_block_size_x;
	MS_U8 chroma_grain_stride = gns_info->chroma_grain_stride;

	int chroma_ar_stride = gns_info->chroma_ar_stride;

	MS_S32 *cb_grain_block = params_hw_adl->cb_grain_block;
	MS_S32 *cr_grain_block = params_hw_adl->cr_grain_block;

	int *cb_ar_block = gns_ar_info->cb_ar_block;
	int *cr_ar_block = gns_ar_info->cr_ar_block;

	int i = 0, j = 0;

	int chroma_ar_i_idx = 0;
	int chroma_ar_j_idx = 0;

	FG_FUNC();

	for (i = (top_pad + AR_PARDDING_NS * ar_padding);
	     i < chroma_block_size_y - bottom_pad; i++) {
		chroma_ar_j_idx = 0;
		for (j = (left_pad + AR_PARDDING_NS * ar_padding);
		     j < chroma_block_size_x - (right_pad +
			AR_PARDDING_NS * ar_padding);
		     j++) {
			cr_ar_block[chroma_ar_i_idx *
				    chroma_ar_stride + chroma_ar_j_idx] =
				cr_grain_block[i * chroma_grain_stride + j];
			cb_ar_block[chroma_ar_i_idx * chroma_ar_stride +
				    chroma_ar_j_idx] =
				cb_grain_block[i * chroma_grain_stride + j];
			chroma_ar_j_idx++;
		}

		chroma_ar_i_idx++;
	}

	FG_LOG_D("chroma ar size %dx%d\n", chroma_ar_i_idx, chroma_ar_j_idx);
}

static void fg_chroma_scale_ar_block(struct fg_gns_basic_info *gns_info,
				     struct fg_gns_ar_block_info *gns_ar_info)
{
	int chroma_ar_block_size_y = gns_info->chroma_ar_block_size_y;
	int chroma_ar_block_size_x = gns_info->chroma_ar_block_size_x;

	int scaled_chroma_ar_x = gns_ar_info->scaled_chroma_ar_x;
	int scaled_chroma_ar_y = gns_ar_info->scaled_chroma_ar_y;

	int *cb_ar_block = gns_ar_info->cb_ar_block;
	int *cr_ar_block = gns_ar_info->cr_ar_block;
	int *scaled_cb_ar = gns_ar_info->scaled_cb_ar;
	int *scaled_cr_ar = gns_ar_info->scaled_cr_ar;
	int *v_scaled_c_ar = gns_ar_info->v_scaled_c_ar;

	FG_FUNC();

	if ((scaled_chroma_ar_x > chroma_ar_block_size_x) ||
	    (scaled_chroma_ar_y > chroma_ar_block_size_y)) {
		pre_up_scale_c(cb_ar_block,
			       v_scaled_c_ar,
			       scaled_cb_ar,
			       chroma_ar_block_size_x,
			       chroma_ar_block_size_y,
			       scaled_chroma_ar_x,
			       scaled_chroma_ar_y);

		pre_up_scale_c(cr_ar_block,
			       v_scaled_c_ar,
			       scaled_cr_ar,
			       chroma_ar_block_size_x,
			       chroma_ar_block_size_y,
			       scaled_chroma_ar_x,
			       scaled_chroma_ar_y);
	} else {
		pre_down_scale(cb_ar_block,
			       v_scaled_c_ar,
			       scaled_cb_ar,
			       chroma_ar_block_size_x,
			       chroma_ar_block_size_y,
			       scaled_chroma_ar_x,
			       scaled_chroma_ar_y);

		pre_down_scale(cr_ar_block,
			       v_scaled_c_ar,
			       scaled_cr_ar,
			       chroma_ar_block_size_x,
			       chroma_ar_block_size_y,
			       scaled_chroma_ar_x,
			       scaled_chroma_ar_y);
	}
}

static void fg_chroma_fill_auto_reg(struct fg_gns_basic_info *gns_info,
				    struct fg_hw_adl_output *params_hw_adl,
				    struct fg_gns_ar_block_info *gns_ar_info)
{
	MS_U8 left_pad = gns_info->left_pad;
	MS_U8 right_pad = gns_info->right_pad;
	MS_U8 top_pad = gns_info->top_pad;
	MS_U8 bottom_pad = gns_info->bottom_pad;
	MS_U8 ar_padding = gns_info->ar_padding;

	int chroma_ar_j_idx = gns_info->chroma_ar_block_size_y;
	int chroma_ar_i_idx = gns_info->chroma_ar_block_size_x;

	int scaled_chroma_ar_x = gns_ar_info->scaled_chroma_ar_x;
	int scaled_chroma_ar_y = gns_ar_info->scaled_chroma_ar_y;

	MS_U8 chroma_block_size_y = gns_info->chroma_block_size_y;
	MS_U8 chroma_block_size_x = gns_info->chroma_block_size_x;

	MS_S32 *cb_grain_block = params_hw_adl->cb_grain_block;
	MS_S32 *cr_grain_block = params_hw_adl->cr_grain_block;

	MS_U8 chroma_grain_stride = gns_info->chroma_grain_stride;

	int i = 0, j = 0;

	int chroma_y_blocks = 0;
	int chroma_x_blocks = 0;
	int chroma_x_left_marginal = (left_pad + AR_PARDDING_NS *
		ar_padding);
	int chroma_y_top_marginal = (top_pad + AR_PARDDING_NS *
		ar_padding);
	int scaled_chroma_ar_i_idx = 0;
	int scaled_chroma_ar_j_idx = 0;
	int cr_ar_offset_x = get_random_number(FG_RANDOM_NUM_BITS);
	int cr_ar_offset_y = get_random_number(FG_RANDOM_NUM_BITS);
	int cb_ar_offset_x = get_random_number(FG_RANDOM_NUM_BITS);
	int cb_ar_offset_y = get_random_number(FG_RANDOM_NUM_BITS);

	int max_chroma_offset_x = scaled_chroma_ar_x > chroma_ar_j_idx ?
		scaled_chroma_ar_x - chroma_ar_j_idx : 0;
	int max_chroma_offset_y = scaled_chroma_ar_y > chroma_ar_i_idx ?
		scaled_chroma_ar_y - chroma_ar_i_idx : 0;

	int *scaled_cr_ar = gns_ar_info->scaled_cr_ar;
	int *scaled_cb_ar = gns_ar_info->scaled_cb_ar;

	FG_FUNC();

	if (max_chroma_offset_x > 0) {
		cb_ar_offset_x = (cb_ar_offset_x * max_chroma_offset_x) >>
				 FG_RANDOM_NUM_BITS;
		cr_ar_offset_x = (cr_ar_offset_x * max_chroma_offset_x) >>
				 FG_RANDOM_NUM_BITS;
	} else {
		cb_ar_offset_x = 0;
		cr_ar_offset_x = 0;
	}

	if (max_chroma_offset_y > 0) {
		cb_ar_offset_y = (cb_ar_offset_y * max_chroma_offset_y) >>
				 FG_RANDOM_NUM_BITS;
		cr_ar_offset_y = (cr_ar_offset_y * max_chroma_offset_y) >>
				 FG_RANDOM_NUM_BITS;
	} else {
		cb_ar_offset_y = 0;
		cr_ar_offset_y = 0;
	}

	FG_LOG_D("chroma block size %ux%u offset %dx%d\n",
		 chroma_block_size_x, chroma_block_size_y,
		 max_chroma_offset_x, max_chroma_offset_y);

	for (i = (top_pad + AR_PARDDING_NS * ar_padding);
	     i < chroma_block_size_y - bottom_pad; i++) {
		for (j = (left_pad + AR_PARDDING_NS * ar_padding);
		     j < chroma_block_size_x -
			(right_pad + AR_PARDDING_NS * ar_padding);
		     j++) {
			chroma_x_blocks = (j - chroma_x_left_marginal) /
				scaled_chroma_ar_x;
			chroma_y_blocks = (i - chroma_y_top_marginal) /
				scaled_chroma_ar_y;

			scaled_chroma_ar_i_idx = (i - chroma_y_top_marginal) -
				(scaled_chroma_ar_y * chroma_y_blocks) +
				cr_ar_offset_y;
			scaled_chroma_ar_j_idx = (j - chroma_x_left_marginal) -
				(scaled_chroma_ar_x * chroma_x_blocks) +
				cr_ar_offset_x;
			cr_grain_block[i * chroma_grain_stride + j] =
				scaled_cr_ar[scaled_chroma_ar_i_idx *
				scaled_chroma_ar_x + scaled_chroma_ar_j_idx];

			scaled_chroma_ar_i_idx = (i - chroma_y_top_marginal) -
				(scaled_chroma_ar_y * chroma_y_blocks) +
				cb_ar_offset_y;
			scaled_chroma_ar_j_idx = (j - chroma_x_left_marginal) -
				(scaled_chroma_ar_x * chroma_x_blocks) +
				cb_ar_offset_x;
			cb_grain_block[i * chroma_grain_stride + j] =
				scaled_cb_ar[scaled_chroma_ar_i_idx *
				scaled_chroma_ar_x + scaled_chroma_ar_j_idx];
		}
	}
}

static void disp_fg_chroma_sw_scale(struct mtk_av1_film_grain_params *params,
				    struct fg_gns_basic_info *gns_info,
				    struct fg_hw_adl_output *params_hw_adl,
				    struct fg_gns_ar_block_info *gns_ar_info)
{
	fg_chroma_crop_ar_block(gns_info, params_hw_adl, gns_ar_info);
	fg_chroma_scale_ar_block(gns_info, gns_ar_info);
	fg_chroma_fill_auto_reg(gns_info, params_hw_adl, gns_ar_info);
}

static void fg_alloc_luma_ar_block(struct fg_gns_basic_info *gns_info,
				   struct fg_gns_ar_block_info *gns_ar_info)
{
	MS_U8 luma_ar_block_size_y = gns_info->luma_ar_block_size_y;
	MS_U8 luma_ar_block_size_x = gns_info->luma_ar_block_size_x;

	int scaled_luma_ar_x = gns_ar_info->scaled_luma_ar_x;
	int scaled_luma_ar_y = gns_ar_info->scaled_luma_ar_y;

	int luma_ar_block_size = luma_ar_block_size_y * luma_ar_block_size_x;
	int scaled_luma_ar_block_size = 0;

	FG_FUNC();

	scaled_luma_ar_block_size = scaled_luma_ar_x * scaled_luma_ar_y;

	if (luma_ar_block_size > scaled_luma_ar_block_size) {
		scaled_luma_ar_block_size = luma_ar_block_size;
		gns_ar_info->ar_scale_up = false;
	} else {
		gns_ar_info->ar_scale_up = true;
	}

	gns_ar_info->luma_ar_block = vmalloc(luma_ar_block_size * sizeof(int));
	if (!gns_ar_info->luma_ar_block)
		FG_ERR("luma_ar_block alloc fail!");

	gns_ar_info->scaled_luma_ar =
		vmalloc(scaled_luma_ar_block_size * sizeof(int));
	if (!gns_ar_info->scaled_luma_ar)
		FG_ERR("scaled_luma_ar alloc fail!");

	gns_ar_info->v_scaled_luma_ar =
		vmalloc(scaled_luma_ar_block_size * sizeof(int));
	if (!gns_ar_info->v_scaled_luma_ar)
		FG_ERR("v_scaled_luma_ar alloc fail!");

	FG_LOG_D("luma_ar %dx%d -> %dx%d buf %d\n",
		 luma_ar_block_size_x,
		 luma_ar_block_size_y,
		 scaled_luma_ar_x,
		 scaled_luma_ar_y,
		 (luma_ar_block_size * sizeof(int) +
		 scaled_luma_ar_block_size * sizeof(int) *
		 LUMA_SCALE_BLOCK_BUF_NS));
}

static void fg_alloc_chroma_ar_block(struct fg_gns_basic_info *gns_info,
				     struct fg_gns_ar_block_info *gns_ar_info)
{
	MS_U8 chroma_ar_block_size_y = gns_info->chroma_ar_block_size_y;
	MS_U8 chroma_ar_block_size_x = gns_info->chroma_ar_block_size_x;

	int scaled_chroma_ar_x = gns_ar_info->scaled_chroma_ar_x;
	int scaled_chroma_ar_y = gns_ar_info->scaled_chroma_ar_y;

	int scaled_chroma_ar_block_size = 0;

	int chroma_ar_block_size =
		chroma_ar_block_size_y * chroma_ar_block_size_x;

	FG_FUNC();

	scaled_chroma_ar_block_size = scaled_chroma_ar_x * scaled_chroma_ar_y;

	if (chroma_ar_block_size > scaled_chroma_ar_block_size)
		scaled_chroma_ar_block_size = chroma_ar_block_size;

	gns_ar_info->cb_ar_block = vmalloc(chroma_ar_block_size * sizeof(int));
	if (!gns_ar_info->cb_ar_block)
		FG_ERR("cb_ar_block alloc fail!");

	gns_ar_info->cr_ar_block = vmalloc(chroma_ar_block_size * sizeof(int));
	if (!gns_ar_info->cr_ar_block)
		FG_ERR("cr_ar_block alloc fail!");

	gns_ar_info->scaled_cb_ar =
		vmalloc(scaled_chroma_ar_block_size * sizeof(int));
	if (!gns_ar_info->scaled_cb_ar)
		FG_ERR("scaled_cb_ar alloc fail!");

	gns_ar_info->scaled_cr_ar =
		vmalloc(scaled_chroma_ar_block_size * sizeof(int));
	if (!gns_ar_info->scaled_cr_ar)
		FG_ERR("scaled_cr_ar alloc fail!");

	gns_ar_info->v_scaled_c_ar =
		vmalloc(scaled_chroma_ar_block_size * sizeof(int));
	if (!gns_ar_info->v_scaled_c_ar)
		FG_ERR("v_scaled_c_ar alloc fail!");

	FG_LOG_D("chroma_ar %dx%d -> %dx%d %d %d buf %d\n",
		 chroma_ar_block_size_x,
		 chroma_ar_block_size_y,
		 scaled_chroma_ar_x,
		 scaled_chroma_ar_y,
		 chroma_ar_block_size,
		 scaled_chroma_ar_block_size,
		 (chroma_ar_block_size * sizeof(int) * CHROMA_BLOCK_BUF_NS +
		 scaled_chroma_ar_block_size * sizeof(int) *
		 CHROMA_SCALE_BLOCK_BUF_NS));
}

void disp_fg_pre_process_gns(struct mtk_av1_film_grain_params *params,
			     struct fg_gns_basic_info *gns_info,
			     struct fg_hw_adl_output *params_hw_adl,
			     struct fg_gns_ar_block_info *gns_ar_info)
{
	FG_FUNC();

	fg_luma_auto_reg_filter(params, gns_info, params_hw_adl);
	disp_fg_luma_sw_scale(params, gns_info, params_hw_adl, gns_ar_info);

	fg_chroma_auto_reg_filter(params, gns_info, params_hw_adl);
	disp_fg_chroma_sw_scale(params, gns_info, params_hw_adl, gns_ar_info);
}

void disp_fg_pre_process_ar_coeffs(struct fg_hw_reg_output *hw_reg)
{
	FG_FUNC();

	memset(hw_reg->ar_coeffs_y, 0, sizeof(hw_reg->ar_coeffs_y));
	memset(hw_reg->ar_coeffs_cb, 0, sizeof(hw_reg->ar_coeffs_cb));
	memset(hw_reg->ar_coeffs_cr, 0, sizeof(hw_reg->ar_coeffs_cr));
}

void disp_fg_init_gns_grain_info(struct mtk_av1_film_grain_params *params,
				 struct fg_hw_reg_output *params_hw_reg,
				 struct fg_gns_basic_info *gns_info)
{
	int grain_center = 0;
	int grain_min = 0;
	int grain_max = 0;

	int rounding_offset = 0;

	FG_FUNC();

	grain_center = BASE_GRAIN_CENTER << (params_hw_reg->bit_depth -
					     BASE_BIT_DEPTH);
	grain_min = 0 - grain_center;
	grain_max = (BASE_GRAIN_MAX << (params_hw_reg->bit_depth -
					BASE_BIT_DEPTH)) - 1 - grain_center;

	rounding_offset = (1 << (params->ar_coeff_shift - 1));

	gns_info->grain_center = grain_center;

	gns_info->grain_min = grain_min;
	gns_info->grain_max = grain_max;
	gns_info->rounding_offset = rounding_offset;

	FG_LOG_D("bpp %u center %d min %d max %d shift %u rounding_offset %d\n",
		 params_hw_reg->bit_depth,
		 grain_center,
		 grain_min,
		 grain_max,
		 params_hw_reg->ar_coeff_shift,
		 rounding_offset);
}

void disp_fg_init_gns_basic_info(struct fg_gns_basic_info *gns_info)
{
	MS_U8 luma_block_size_y = 0;
	MS_U8 luma_block_size_x = 0;
	MS_U8 chroma_block_size_y = 0;
	MS_U8 chroma_block_size_x = 0;
	MS_U8 luma_grain_stride = 0;
	MS_U8 chroma_grain_stride = 0;

	int luma_ar_block_size_y = 0;
	int luma_ar_block_size_x = 0;
	int chroma_ar_block_size_y = 0;
	int chroma_ar_block_size_x = 0;
	int luma_ar_stride = 0;
	int chroma_ar_stride = 0;

	int chroma_subsamp_x = 1;
	int chroma_subsamp_y = 1;

	FG_FUNC();

	luma_ar_block_size_y = FG_LUMA_SUBBLOCK_SIZE_Y * FG_LUMA_SUBBLOCK_NS;
	luma_ar_block_size_x = FG_LUMA_SUBBLOCK_SIZE_X * FG_LUMA_SUBBLOCK_NS;
	chroma_ar_block_size_y = FG_CHROMA_SUBBLOCK_SIZE_Y *
				 FG_CHROMA_SUBBLOCK_NS;
	chroma_ar_block_size_x = FG_CHROMA_SUBBLOCK_SIZE_X *
				 FG_CHROMA_SUBBLOCK_NS;

	luma_ar_stride = luma_ar_block_size_x;
	chroma_ar_stride = chroma_ar_block_size_x;

	/* 3 + 2 * 3 + 64 + 0  = 73 */
	luma_block_size_y = FG_LUMA_BLOCK_SIZE_Y;
	/* 3 + 2 * 3 + 64 + + 2 * 3 + 3 = 82 */
	luma_block_size_x = FG_LUMA_BLOCK_SIZE_X;

	chroma_block_size_y = FG_CHROMA_BLOCK_SIZE_Y;
	chroma_block_size_x = FG_CHROMA_BLOCK_SIZE_X;

	luma_grain_stride = luma_block_size_x; /* 82 */
	chroma_grain_stride = chroma_block_size_x; /* 44 */

	gns_info->left_pad = FG_LEFT_PAD;
	gns_info->right_pad = FG_RIGHT_PAD;
	gns_info->top_pad = FG_TOP_PAD;
	gns_info->bottom_pad = FG_BOTTOM_PAD;

	gns_info->ar_padding = FG_AR_PADDING;

	gns_info->luma_block_size_y = luma_block_size_y;
	gns_info->luma_block_size_x = luma_block_size_x;
	gns_info->chroma_block_size_y = chroma_block_size_y;
	gns_info->chroma_block_size_x = chroma_block_size_x;

	gns_info->luma_grain_stride = luma_grain_stride;
	gns_info->chroma_grain_stride = chroma_grain_stride;

	gns_info->chroma_subsamp_x = chroma_subsamp_x;
	gns_info->chroma_subsamp_y = chroma_subsamp_y;

	gns_info->luma_ar_block_size_y = luma_ar_block_size_y;
	gns_info->luma_ar_block_size_x = luma_ar_block_size_x;
	gns_info->chroma_ar_block_size_y = chroma_ar_block_size_y;
	gns_info->chroma_ar_block_size_x = chroma_ar_block_size_x;

	gns_info->luma_ar_stride = luma_ar_stride;
	gns_info->chroma_ar_stride = chroma_ar_stride;
}

void disp_fg_free_gns_ar_block(struct fg_gns_ar_block_info *gns_ar_info)
{
	FG_FUNC();

	if (!gns_ar_info->ar_block_alloc) {
		FG_LOG_I("ar block mem already free!\n");
		return;
	}

	vfree(gns_ar_info->luma_ar_block);
	vfree(gns_ar_info->scaled_luma_ar);
	vfree(gns_ar_info->v_scaled_luma_ar);
	vfree(gns_ar_info->cb_ar_block);
	vfree(gns_ar_info->cr_ar_block);
	vfree(gns_ar_info->scaled_cb_ar);
	vfree(gns_ar_info->scaled_cr_ar);
	vfree(gns_ar_info->v_scaled_c_ar);

	gns_ar_info->luma_ar_block = NULL;
	gns_ar_info->scaled_luma_ar = NULL;
	gns_ar_info->v_scaled_luma_ar = NULL;
	gns_ar_info->cb_ar_block = NULL;
	gns_ar_info->cr_ar_block = NULL;
	gns_ar_info->scaled_cb_ar = NULL;
	gns_ar_info->scaled_cr_ar = NULL;
	gns_ar_info->v_scaled_c_ar = NULL;

	gns_ar_info->ar_block_alloc = false;
}

void disp_fg_update_gns_ar_block_info(struct fg_gns_basic_info *gns_info,
				      struct video_scale_info *scale_info,
				      struct fg_gns_ar_block_info *gns_ar_info)
{
	MS_U8 luma_ar_block_size_y = gns_info->luma_ar_block_size_y;
	MS_U8 luma_ar_block_size_x = gns_info->luma_ar_block_size_x;

	MS_U8 chroma_ar_block_size_y = gns_info->chroma_ar_block_size_y;
	MS_U8 chroma_ar_block_size_x = gns_info->chroma_ar_block_size_x;

	int scaled_luma_ar_x = 0;
	int scaled_luma_ar_y = 0;

	int scaled_chroma_ar_x = 0;
	int scaled_chroma_ar_y = 0;

	scaled_luma_ar_x = luma_ar_block_size_x * scale_info->dst_w /
			   scale_info->src_w;
	scaled_luma_ar_y = luma_ar_block_size_y * scale_info->dst_h /
			   scale_info->src_h;

	scaled_chroma_ar_x = chroma_ar_block_size_x * scale_info->dst_w /
			     scale_info->src_w;
	scaled_chroma_ar_y = chroma_ar_block_size_y * scale_info->dst_h /
			     scale_info->src_h;

	FG_FUNC();

	FG_LOG_D("luma ar block %ux%u -> scaled %dx%d [%ux%u -> %ux%u]\n",
		 luma_ar_block_size_y,
		 luma_ar_block_size_y,
		 scaled_luma_ar_x,
		 scaled_luma_ar_y,
		 scale_info->src_w,
		 scale_info->src_h,
		 scale_info->dst_w,
		 scale_info->dst_h);

	FG_LOG_D("chroma ar block %ux%u -> scaled ar block %dx%d\n",
		 chroma_ar_block_size_x,
		 chroma_ar_block_size_y,
		 scaled_chroma_ar_x,
		 scaled_chroma_ar_y);

	disp_fg_free_gns_ar_block(gns_ar_info);

	gns_ar_info->scaled_luma_ar_x = scaled_luma_ar_x;
	gns_ar_info->scaled_luma_ar_y = scaled_luma_ar_y;

	gns_ar_info->scaled_chroma_ar_x = scaled_chroma_ar_x;
	gns_ar_info->scaled_chroma_ar_y = scaled_chroma_ar_y;

	fg_alloc_luma_ar_block(gns_info, gns_ar_info);
	fg_alloc_chroma_ar_block(gns_info, gns_ar_info);

	gns_ar_info->ar_block_alloc = true;
}
