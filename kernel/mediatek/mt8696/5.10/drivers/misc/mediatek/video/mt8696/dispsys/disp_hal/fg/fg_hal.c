// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/wait.h>

#include "disp_adl_if.h"
#include "disp_fg_if.h"

#include "fg_hal.h"
#include "fg_hw.h"

#define WRITE_FGS_ADL_DATA_FORMAT(baseaddr, index, bit, hw_adl, v_index) \
do {												\
	*(baseaddr + index + 9) = (((hw_adl->y_grain_block[7 + v_index] & bit)) >> 2);	\
	*(baseaddr + index + 8) = (((hw_adl->y_grain_block[7 + v_index] & bit) << 10)	\
				  + (hw_adl->y_grain_block[6 + v_index] & bit)) >> 4;	\
	*(baseaddr + index + 7) = (((hw_adl->y_grain_block[6 + v_index] & bit) << 10)	\
				  + (hw_adl->y_grain_block[5 + v_index] & bit)) >> 6;	\
	*(baseaddr + index + 6) = (((hw_adl->y_grain_block[5 + v_index] & bit) << 10)	\
				  + (hw_adl->y_grain_block[4 + v_index] & bit)) >> 8;	\
	*(baseaddr + index + 5) = (((hw_adl->y_grain_block[4 + v_index] & bit)) >> 0);	\
	*(baseaddr + index + 4) = (((hw_adl->y_grain_block[3 + v_index] & bit)) >> 2);	\
	*(baseaddr + index + 3) = (((hw_adl->y_grain_block[3 + v_index] & bit) << 10)	\
				  + (hw_adl->y_grain_block[2 + v_index] & bit)) >> 4;	\
	*(baseaddr + index + 2) = (((hw_adl->y_grain_block[2 + v_index] & bit) << 10)	\
				  + (hw_adl->y_grain_block[1 + v_index] & bit)) >> 6;	\
	*(baseaddr + index + 1) = (((hw_adl->y_grain_block[1 + v_index] & bit) << 10)	\
				  + (hw_adl->y_grain_block[0 + v_index] & bit)) >> 8;	\
	*(baseaddr + index + 0) = (((hw_adl->y_grain_block[0 + v_index] & bit)) >> 0);	\
} while (0)

#define WRITE_FGS_ADL_DATA_FORMAT_bt(baseaddr, index, bit, hw_adl, v_index) \
do {												\
	*(baseaddr + index + 2) = (((hw_adl->y_grain_block[1 + v_index] & bit)) >> 6); \
	*(baseaddr + index + 1) = (((hw_adl->y_grain_block[1 + v_index] & bit) << 10) \
				  + (hw_adl->y_grain_block[0 + v_index] & bit)) >> 8; \
	*(baseaddr + index + 0) = (((hw_adl->y_grain_block[1 + v_index] & bit) << 10) \
				  + (hw_adl->y_grain_block[0 + v_index] & bit)) >> 0;	\
} while (0)


#define WRITE_FGS_ADL_DATA_FORMAT_CB(baseaddr, index, bit, hw_adl, v_index) \
do {												\
	*(baseaddr + index + 9) = (((hw_adl->cb_grain_block[7 + v_index] & bit)) >> 2); \
	*(baseaddr + index + 8) = (((hw_adl->cb_grain_block[7 + v_index] & bit) << 10) \
				  + (hw_adl->cb_grain_block[6 + v_index] & bit)) >> 4; \
	*(baseaddr + index + 7) = (((hw_adl->cb_grain_block[6 + v_index] & bit) << 10) \
				  + (hw_adl->cb_grain_block[5 + v_index] & bit)) >> 6; \
	*(baseaddr + index + 6) = (((hw_adl->cb_grain_block[5 + v_index] & bit) << 10) \
				  + (hw_adl->cb_grain_block[4 + v_index] & bit)) >> 8; \
	*(baseaddr + index + 5) = (((hw_adl->cb_grain_block[4 + v_index] & bit)) >> 0); \
	*(baseaddr + index + 4) = (((hw_adl->cb_grain_block[3 + v_index] & bit)) >> 2); \
	*(baseaddr + index + 3) = (((hw_adl->cb_grain_block[3 + v_index] & bit) << 10) \
				  + (hw_adl->cb_grain_block[2 + v_index] & bit)) >> 4; \
	*(baseaddr + index + 2) = (((hw_adl->cb_grain_block[2 + v_index] & bit) << 10) \
				  + (hw_adl->cb_grain_block[1 + v_index] & bit)) >> 6; \
	*(baseaddr + index + 1) = (((hw_adl->cb_grain_block[1 + v_index] & bit) << 10) \
				  + (hw_adl->cb_grain_block[0 + v_index] & bit)) >> 8; \
	*(baseaddr + index + 0) = (((hw_adl->cb_grain_block[0 + v_index] & bit)) >> 0);	\
} while (0)

#define WRITE_FGS_ADL_DATA_FORMAT_CB_bt(baseaddr, index, bit, hw_adl, v_index) \
do {												\
	*(baseaddr + index + 4) = (((hw_adl->cb_grain_block[3 + v_index] & bit)) >> 2); \
	*(baseaddr + index + 3) = (((hw_adl->cb_grain_block[3 + v_index] & bit) << 10) \
				 + ((hw_adl->cb_grain_block[2 + v_index] & bit) << 0)) >> 4; \
	*(baseaddr + index + 2) = (((hw_adl->cb_grain_block[2 + v_index] & bit) << 10) \
				 + ((hw_adl->cb_grain_block[1 + v_index] & bit) << 0)) >> 6; \
	*(baseaddr + index + 1) = (((hw_adl->cb_grain_block[1 + v_index] & bit) << 10) \
				  + (hw_adl->cb_grain_block[0 + v_index] & bit)) >> 8; \
	*(baseaddr + index + 0) = (((hw_adl->cb_grain_block[0 + v_index] & bit)) >> 0);	\
} while (0)

#define WRITE_FGS_ADL_DATA_FORMAT_CR(baseaddr, index, bit, hw_adl, v_index) \
do {												\
	*(baseaddr + index + 19) = (((hw_adl->cr_grain_block[7 + v_index] & bit)) >> 2); \
	*(baseaddr + index + 18) = (((hw_adl->cr_grain_block[7 + v_index] & bit) << 10) \
				  + ((hw_adl->cr_grain_block[6 + v_index] & bit) << 0)) >> 4; \
	*(baseaddr + index + 17) = (((hw_adl->cr_grain_block[6 + v_index] & bit) << 10) \
				  + ((hw_adl->cr_grain_block[5 + v_index] & bit) << 0)) >> 6; \
	*(baseaddr + index + 16) = (((hw_adl->cr_grain_block[5 + v_index] & bit) << 10) \
				   + (hw_adl->cr_grain_block[4 + v_index] & bit)) >> 8; \
	*(baseaddr + index + 15) = (((hw_adl->cr_grain_block[4 + v_index] & bit)) >> 0); \
	*(baseaddr + index + 14) = (((hw_adl->cr_grain_block[3 + v_index] & bit)) >> 2); \
	*(baseaddr + index + 13) = (((hw_adl->cr_grain_block[3 + v_index] & bit) << 10) \
				  + ((hw_adl->cr_grain_block[2 + v_index] & bit) << 0)) >> 4; \
	*(baseaddr + index + 12) = (((hw_adl->cr_grain_block[2 + v_index] & bit) << 10) \
				  + ((hw_adl->cr_grain_block[1 + v_index] & bit) << 0)) >> 6; \
	*(baseaddr + index + 11) = (((hw_adl->cr_grain_block[1 + v_index] & bit) << 10) \
				   + (hw_adl->cr_grain_block[0 + v_index] & bit)) >> 8; \
	*(baseaddr + index + 10) = (((hw_adl->cr_grain_block[0 + v_index] & bit)) >> 0);	\
} while (0)

#define WRITE_FGS_ADL_DATA_FORMAT_CR_bt(baseaddr, index, bit, hw_adl, v_index)		\
do {												\
	*(baseaddr + index + 14) = (((hw_adl->cr_grain_block[3 + v_index] & bit)) >> 2);	\
	*(baseaddr + index + 13) = (((hw_adl->cr_grain_block[3 + v_index] & bit) << 10)	\
				  + ((hw_adl->cr_grain_block[2 + v_index] & bit) << 0)) >> 4;\
	*(baseaddr + index + 12) = (((hw_adl->cr_grain_block[2 + v_index] & bit) << 10)	\
				  + ((hw_adl->cr_grain_block[1 + v_index] & bit) << 0)) >> 6;\
	*(baseaddr + index + 11) = (((hw_adl->cr_grain_block[1 + v_index] & bit) << 10)	\
				   + (hw_adl->cr_grain_block[0 + v_index] & bit)) >> 8;	\
	*(baseaddr + index + 10) = (((hw_adl->cr_grain_block[0 + v_index] & bit)) >> 0);	\
} while (0)

void __iomem *disp_fg_reg_base[MAX_FG];

static void fg_write_reg(void __iomem *address, u32 value)
{
	writel(value, address);
}

static void fg_write_reg_mask(void __iomem *address, u32 value, u32 mask)
{
	u32 tmp = readl(address);

	tmp = (tmp & ~mask) | (value & mask);
	writel(tmp, address);
}

void fg_hal_reg_update(enum fg_hw_id hw_id, struct fg_hw_reg_output *params_hw_reg)
{
	void __iomem *fg_base = disp_fg_reg_base[hw_id];
	u32 idx;

	FG_FUNC();

	/* ar_coeff_y[0 - 23]
	 * reg 0x00 - 0x14
	 */
	for (idx = 0; idx < 6; idx++) {
		fg_write_reg(fg_base + FG_REG_AR_COFF_Y_0 + 4 * idx,
			     ((params_hw_reg->ar_coeffs_y[4 * idx + 0]) |
			      (params_hw_reg->ar_coeffs_y[4 * idx + 1] << 8) |
			      (params_hw_reg->ar_coeffs_y[4 * idx + 2] << 16) |
			      (params_hw_reg->ar_coeffs_y[4 * idx + 3] << 24)));
	}

	/* ar_coeff_cb[0 - 23]
	 * reg 0x18 - 0x2c
	 */
	for (idx = 0; idx < 6; idx++) {
		fg_write_reg(fg_base + FG_REG_AR_COFF_CB_0 + 4 * idx,
			     ((params_hw_reg->ar_coeffs_cb[4 * idx + 0]) |
			      (params_hw_reg->ar_coeffs_cb[4 * idx + 1] << 8) |
			      (params_hw_reg->ar_coeffs_cb[4 * idx + 2] << 16) |
			      (params_hw_reg->ar_coeffs_cb[4 * idx + 3] << 24)));
	}
	/* idx = 6, ar_coeff_cr[24]
	 * reg 0x30
	 */
	fg_write_reg(fg_base + FG_REG_AR_COFF_CB_0 + 4 * idx,
			     (params_hw_reg->ar_coeffs_cb[4 * idx + 0]));


	/* ar_coeff_cr[0 - 23]
	 * reg 0x34 - 0x48
	 */
	for (idx = 0; idx < 6; idx++) {
		fg_write_reg(fg_base + FG_REG_AR_COFF_CR_0 + 4 * idx,
			     ((params_hw_reg->ar_coeffs_cr[4 * idx + 0]) |
			      (params_hw_reg->ar_coeffs_cr[4 * idx + 1] << 8) |
			      (params_hw_reg->ar_coeffs_cr[4 * idx + 2] << 16) |
			      (params_hw_reg->ar_coeffs_cr[4 * idx + 3] << 24)));
	}
	/* idx = 6 ar_coeff_cr[24]
	 * * reg 0x4c
	 */
	fg_write_reg(fg_base + FG_REG_AR_COFF_CR_0 + 4 * idx,
		     ((params_hw_reg->ar_coeffs_cr[4 * idx + 0]) |
		      (params_hw_reg->ar_coeff_shift << 8)));

	/* d.s c:0x15011054, %le %long 0x00021060 */
	fg_write_reg(fg_base + FG_REG_XX(0x15),
		     ((0 << 4) |	/* 420 repeat la md */
		      (1 << 5) |	/* 420 repeat en*/
		      (1 << 6) |	/* 10b repeat en*/
		      (params_hw_reg->y_bypass_en << 8) |
		      (params_hw_reg->cb_bypass_en << 9) |
		      (params_hw_reg->cr_bypass_en << 10) |
		      (params_hw_reg->clip_to_restricted_range << 11) |
		      (params_hw_reg->overlap_flag << 12) |
		      (0 << 13) |	/* chrome scaling md */
		      FG_SW_FILTER_EN(params_hw_reg->force_write) |
		      (params_hw_reg->chroma_scaling_from_luma << 15) |
		      (params_hw_reg->grain_scaling << 16)));

	/* d.s c:0x15011058, %le %long 0xc0c08080, */
	fg_write_reg(fg_base + FG_REG_XX(0x16),
		     ((params_hw_reg->cb_mult << 0) |
		      (params_hw_reg->cr_mult << 8) |
		      (params_hw_reg->cb_luma_mult << 16) |
		      (params_hw_reg->cr_luma_mult << 24)));

	/*d.s c:0x1501105c, %le %long 0x0000b0af, */
	fg_write_reg(fg_base + FG_REG_XX(0x17),
		     ((params_hw_reg->grain_seed << 0)));

	/* d.s c:0x15011064, %le %long 0x01000100, */
	fg_write_reg(fg_base + FG_REG_XX(0x19),
		     ((params_hw_reg->cb_offset << 0) |
		      (params_hw_reg->cr_offset << 16)));
}

void fg_hal_reg_write(u32 addr, u32 val)
{
	void __iomem *va_base = NULL;
	u32 offset = 0;

	FG_LOG_I("write reg 0x%X val 0x%X\n", addr, val);

	if (FG_REG_BASE(addr) == FG_REG_BASE(FG_REG_MAIN_BASE)) {
		va_base = disp_fg_reg_base[0];
		offset = addr - FG_REG_MAIN_BASE;
	} else if (FG_REG_BASE(addr) == FG_REG_BASE(FG_REG_SUB_BASE)) {
		va_base = disp_fg_reg_base[1];
		offset = addr - FG_REG_SUB_BASE;
	} else {
		FG_ERR("invalid reg addr 0x%X\n", addr);
		return;
	}

	fg_write_reg(va_base + offset, val);
}

void fg_hal_reg_dump(enum fg_hw_id hw_id, u32 len)
{
	void __iomem *fg_base = NULL;
	u32 pa_base = 0;
	u32 idx = 0;

	if (hw_id >= MAX_FG || len > FG_REG_MAX_LEN) {
		FG_ERR("invalid parameter hw id %u len 0x%X\n", hw_id, len);
		return;
	}

	fg_base = disp_fg_reg_base[hw_id];
	pa_base = (hw_id == MAIN_FG) ? FG_REG_MAIN_BASE : FG_REG_SUB_BASE;

	FG_LOG_I("dump fg %u reg 0x%X len %u\n", hw_id, pa_base, len);

	for (idx = 0; idx < len; idx += FG_REG_DUMP_PITCH)
		FG_LOG_I("0x%08X | 0x%08X 0x%08X 0x%08X 0x%08X\n",
			 (pa_base + idx),
			 readl(fg_base + idx + 0x0),
			 readl(fg_base + idx + FG_REG_OFFSET_1),
			 readl(fg_base + idx + FG_REG_OFFSET_2),
			 readl(fg_base + idx + FG_REG_OFFSET_3));
}

/*
 * fg raw data table
 * [y cb cr] 256 x 3 = 768 (byte)
 * [y gns]  803 x 10 = 8030
 * [cb cr gns] 228 x 20 = 4560
 * total 768 + 8030 + 4560 = 13358
 * y gns offset 768
 * cb cr gns offset 768 + 8030 = 8798
 *
 * fg adl format table
 * [y cb cr] 256 x 32 = 8192 (byte)
 * [y gns]  803 x 32 = 25696
 * [cb cr gns] 228 x 32 = 7296
 * total 8192 + 25696 + 7296 = 41184
 * y gns offset 8192
 * cb cr gns offset 8192 + 25696 = 33888
 *
 * mt8696 flow
 * 1. fg driver send raw data(13358) to adl driver
 * 2. adl driver transfer table to 41184 byte format
 */
static void fg_hal_adl_update_lut(struct fg_hw_reg_output *params_hw_reg,
				  struct fg_hw_adl_output *params_hw_adl,
				  struct adl_src_tbl *adl_tbl)
{
	int i;

	FG_FUNC();

	for (i = 0; i <= 255; ++i) {
		*(adl_tbl->fgrain.p_data + 0 + 3 * i) = params_hw_adl->u8scaling_lut_y256[i];
		*(adl_tbl->fgrain.p_data + 1 + 3 * i) = params_hw_adl->u8scaling_lut_cb256[i];
		*(adl_tbl->fgrain.p_data + 2 + 3 * i) = params_hw_adl->u8scaling_lut_cr256[i];
	}
}

static void fg_hal_adl_update_gns(struct fg_hw_reg_output *params_hw_reg,
				  struct fg_hw_adl_output *params_hw_adl,
				  struct adl_src_tbl *adl_tbl)
{
	int i;
	int j;
	int lu_grain_i;
	int p_lu_grain_i;
	int chr_grain_i;
	int p_chr_grain_i;
	u32 mask_bit;

	FG_FUNC();

	if (params_hw_reg->bit_depth == 8)
		mask_bit = 0xff;
	else
		mask_bit = 0x3ff;

	/* luma_grain_block size 73 x 82 = 5986 */
	/* adl table size 73 x 11 x 10 = 8030 */
	for (i = 0; i < 73; i++) {
		/* 10 x 8 = 80, 80 array element of params_hw_adl y_grain_block
		 * convert to 10 x 10 = 100 byte of raw data
		 */
		for (j = 0; j < 10; j++) {
			lu_grain_i = i * 82 + 8 * j;
			p_lu_grain_i = FG_TBL_Y_GNS_OFFSET + FG_LUMA_GNS_PITCH * 11 * i +
				       FG_LUMA_GNS_PITCH * j;
			WRITE_FGS_ADL_DATA_FORMAT(adl_tbl->fgrain.p_data, p_lu_grain_i, mask_bit,
						  params_hw_adl, lu_grain_i);
		}

		/* 2 array element of params_hw_adl y_grain_block
		 * convert to 3 byte of raw data
		 * there are 7 byte zero data of every 110 byte of raw data
		 */
		lu_grain_i = i * 82 + 8 * 10;
		p_lu_grain_i = FG_TBL_Y_GNS_OFFSET + FG_LUMA_GNS_PITCH * 11 * i +
			       FG_LUMA_GNS_PITCH * 10;
		WRITE_FGS_ADL_DATA_FORMAT_bt(adl_tbl->fgrain.p_data, p_lu_grain_i, mask_bit,
					     params_hw_adl, lu_grain_i);
	}

	p_lu_grain_i = FG_TBL_Y_GNS_OFFSET + FG_LUMA_GNS_PITCH * 11 * i +
				       FG_LUMA_GNS_PITCH * 10;

	FG_LOG_D("p_lu_grain_i %d\n", p_lu_grain_i);

	for (i = 0; i < 38; i++) {
		for (j = 0; j < 5; j++) {
			chr_grain_i = i * 44 + 8 * j;
			p_chr_grain_i = FG_TBL_C_GNS_OFFSET + FG_CHROMA_GNS_PITCH * 6 * i +
					FG_CHROMA_GNS_PITCH * j;
			WRITE_FGS_ADL_DATA_FORMAT_CB(adl_tbl->fgrain.p_data, p_chr_grain_i,
						     mask_bit, params_hw_adl, chr_grain_i);
			WRITE_FGS_ADL_DATA_FORMAT_CR(adl_tbl->fgrain.p_data, p_chr_grain_i,
						     mask_bit, params_hw_adl, chr_grain_i);
		}
		chr_grain_i = i * 44 + 8 * 5;
		p_chr_grain_i = FG_TBL_C_GNS_OFFSET + FG_CHROMA_GNS_PITCH * 6 * i +
				FG_CHROMA_GNS_PITCH * 5;
		WRITE_FGS_ADL_DATA_FORMAT_CB_bt(adl_tbl->fgrain.p_data, p_chr_grain_i, mask_bit,
						params_hw_adl, chr_grain_i);
		WRITE_FGS_ADL_DATA_FORMAT_CR_bt(adl_tbl->fgrain.p_data, p_chr_grain_i, mask_bit,
						params_hw_adl, chr_grain_i);
	}

	p_chr_grain_i = FG_TBL_C_GNS_OFFSET + FG_CHROMA_GNS_PITCH * 6 * i +
					FG_CHROMA_GNS_PITCH * 5;

	FG_LOG_D("p_chr_grain_i %d\n", p_chr_grain_i);
}

void fg_hal_bypass(enum fg_hw_id hw_id, bool bypass)
{
	void __iomem *fg_base = disp_fg_reg_base[hw_id];
	u32 bypass_value = 0;

	FG_FUNC();

	if (bypass)
		bypass_value = ((1 << 8) | (1 << 9) | (1 << 10));

	fg_write_reg_mask(fg_base + FG_REG_XX(0x15),
			 bypass_value,
			 ((1 << 8) | (1 << 9) | (1 << 10)));
}

void fg_hal_update_process(enum fg_hw_id hw_id, struct fg_hw_reg_output *params_hw_reg,
			   struct fg_hw_adl_output *params_hw_adl,
			   struct adl_src_tbl *adl_tbl)
{
	FG_FUNC();

	fg_hal_reg_update(hw_id, params_hw_reg);

	fg_hal_adl_update_lut(params_hw_reg, params_hw_adl, adl_tbl);
	fg_hal_adl_update_gns(params_hw_reg, params_hw_adl, adl_tbl);
}
