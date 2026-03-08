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

#ifndef _DISP_FEFIFO_HW_H_
#define _DISP_FEFIFO_HW_H_

#include <linux/types.h>

#define FE_FIFO_MAX_LAYER_NUM  4
#define HAL_FEFIFO_REG_NUM (0x100/4)
#define HAL_FEFIFO_REG_CONF_CNT (0x100/4)

#define DISP_FEFIFO_VDOFE0_REG_BASE (0x15010000)
#define DISP_FEFIFO_VDOFE1_REG_BASE (0x15017000)
#define DISP_FEFIFO_GFXFE0_REG_BASE (0x14010000)
#define DISP_FEFIFO_GFXFE1_REG_BASE (0x14011000)

#define FEFIFO_REG_MASK_DEF (0xFFFFFFFF)

#define FEFIFO_FULL_TH 0x000
#define FEFIFO_CONTROL 0x004
#define CTL_CLR_BY_VS_FLD (0xF << 0)
#define CTL_CLR_IRQ_FLD   (0x1 << 22)
#define CTL_FORCE_IRQ_FLD (0x1 << 23)
#define CTL_SRC_IN_SWAP_FLD (0x1F << 24)
#define CTL_ALPHA_SEL_FLD (0x1 << 29)
#define CTL_SOFT_RST_FLD  (0x3 << 30)

#define FEFIFO_PATGEN_CTL 0x02C
#define FE_PATGEN_EN_FLD  (0x1 << 0)

#define FEFIFO_TG_CFG0 0x030
#define FE_PATGEN_HTOTAL_FLD  (0xFFFF << 0)
#define FE_PATGEN_VTOTAL_LSB_FLD (0xFFFF << 16)

#define FEFIFO_TG_CFG1 0x034
#define FE_PAGTGEN_HACTIVE_FLD (0xFFFF << 0)
#define FE_PAGTGEN_VACTIVE_FLD (0xFFFF << 16)

#define FEFIFO_TG_CFG2 0x038
#define FE_PAGTGEN_HSYNCW_FLD (0xFF << 0)
#define FE_PAGTGEN_VSYNCW_FLD (0xFF << 8)

#define FEFIFO_TG_CFG3 0x03C
#define FE_PAGTGEN_HFRONT_FLD (0xFFFF << 0)
#define FE_PAGTGEN_VFRONT_FLD (0xFFFF << 16)

#define FEFIFO_TG_CFG4 0x040
#define FE_VS_DLY_FLD (0xFFFF << 0)
#define FE_PAGTGEN_ALPHA_FLD (0xFF << 16)

#define FEFIFO_PATGEN_DATA0 0x044
#define FE_PATGEN_ACTIVE_Y_FLD (0xFFFF << 0)
#define FE_PATGEN_ACTIVE_CB_FLD (0xFFFF << 16)

#define FEFIFO_PATGEN_DATA1 0x048
#define FE_PATGEN_ACTIVE_CR_FLD (0xFFFF << 0)
#define FE_PATGEN_BLANK_YC_FLD (0xFFFF << 16)

#define FEFIFO_PATGEN_CFG 0x04C
#define FE_PATGEN_ACTIVE_CFG_FLD (0x1 << 0)
#define FE_PATGEN_BLANK_CFG_FLD  (0x1 << 1)

#define FEFIFO_SHDW_CFG 0x050
#define FE_SHDW_EN_FLD (0x1 << 0)
#define FE_SHDW_TRIG_FLD  (0x1 << 1)

#define FEFIFO_TIMING_CONF 0x05C
#define TM_HACTIVE_FLD (0xFFFF << 0)
#define TM_VACTIVE_FLD (0xFFFF << 16)

#define FEFIFO_DATA_REORDER 0x060
#define FE_SRC_10B_MSB_ALIGN_FLD (0x1 << 0)
#define FE_SRC_8B_MSB_ALIGN_FLD (0x3 << 2)
#define FE_SRC_IN_BIT_SEL_FLD (0x3 << 6)

#define FEFIFO_TG_CFG5 0x06C
#define FE_PATGEN_VTOTAL_MSB_FLD (0xFFFF << 0)


/********************************
 * DISP FE FIFO Register
 ********************************/
struct disp_fefifo_reg_t {
	/* DWORD - 000 */
uint32_t: 16;
	uint32_t fifo_full_th: 16;

	/* DWORD - 004 */
	uint32_t fe_clr_fifo_wr_by_vs: 1;
	uint32_t fe_clr_fifo_rd_by_vs: 1;
	uint32_t fe_clr_fifo_waddr_by_vs: 1;
	uint32_t fe_clr_fifo_raddr_by_vs: 1;
	uint32_t fifo_force_rd: 1;
	uint32_t data_2p_swap: 1;
	uint32_t sof_clr_rdy_timng: 1;
	uint32_t rst_use_vs_rd: 1;
uint32_t: 8;
	uint32_t fe_1p_i_crc_start: 1;
	uint32_t fe_1p_i_crc_clr: 1;
	uint32_t fe_2p_i_crc_start: 1;
	uint32_t fe_2p_i_crc_clr: 1;
	uint32_t fe_2p_o_crc_start: 1;
	uint32_t fe_2p_o_crc_clr: 1;
	uint32_t fifo_irq_clr: 1;
	uint32_t fifo_irq_force: 1;
	uint32_t data_1p_i_swap: 5;
	uint32_t alpha_sel: 1;
	uint32_t soft_rst: 2;

	/* DWORD - 008 */
	uint32_t fe_pixel_data: 32;

	/* DWORD - 00C */
	uint32_t fifo_full: 1;
	uint32_t fifo_empty: 1;
	uint32_t fifo_err: 1;
uint32_t: 29;

	/* DWORD - 010 */
	uint32_t fifo_dbg_mon: 32;

	/* DWORD - 014 */
	uint32_t data_blank_y: 32;

	/* DWORD - 018 */
	uint32_t data_blank_cb: 32;

	/* DWORD - 01C */
	uint32_t data_blank_cr: 32;

	/* DWORD - 020 */
	uint32_t crc_result_1p_i: 16;
	uint32_t crc_rdy_1p_i: 1;
uint32_t: 15;

	/* DWORD - 024 */
	uint32_t crc_result_2p_i: 16;
	uint32_t crc_rdy_2p_i: 1;
uint32_t: 15;

	/* DWORD - 028 */
	uint32_t crc_result_2p_o: 16;
	uint32_t crc_rdy_2p_o: 1;
uint32_t: 15;

	/* DWORD - 02C */
	uint32_t patgen_en: 1;
	uint32_t hs_polar: 1;
	uint32_t vs_polar: 1;
	uint32_t fifo_vs_err_clr: 1;
uint32_t: 28;

	/* DWORD - 030 */
	uint32_t patgen_htotal: 16;
	uint32_t patgen_vtotal_lsb: 16;

	/* DWORD - 034 */
	uint32_t patgen_hactive: 16;
	uint32_t patgen_vactive: 16;

	/* DWORD - 038 */
	uint32_t patgen_hsync_w: 8;
	uint32_t patgen_vsync_w: 8;
uint32_t: 16;

	/* DWORD - 03C */
	uint32_t patgen_hfront: 16;
	uint32_t patgen_vfront: 16;

	/* DWORD - 040 */
	uint32_t vs_dly: 16;
	uint32_t gfx_patgen_alpha: 8;
uint32_t: 8;

	/* DWORD - 044 */
	uint32_t patgen_active_y: 16;
	uint32_t patgen_active_cb: 16;

	/* DWORD - 048 */
	uint32_t patgen_active_cr: 16;
	uint32_t patgen_blank_yc: 16;

	/* DWORD - 04C */
	uint32_t patgen_pixel_active_cfg_en: 1;
	uint32_t patgen_pixel_blank_cfg_en: 1;
uint32_t: 30;

	/* DWORD - 050 */
	uint32_t shdw_en : 1;
	uint32_t shdw_trigger : 1;
uint32_t: 30;

	/* DWORD - 054 */
uint32_t: 4;
	uint32_t shdw_data_sel: 12;
uint32_t: 16;

	/* DWORD - 058 */
	uint32_t shdw_data: 32;

	/* DWORD - 05C */
	uint32_t fe_hactive: 16;
	uint32_t fe_vactive: 16;

	/* DWORD - 060 */
	uint32_t fe_10b_msb_align: 1;
	uint32_t fe_10b_low_en: 1;
	uint32_t fe_8b_msb_align_en: 2;
	uint32_t fe_8b_hi_mid_low_sel: 2;
	uint32_t fe_source_10b_8b_sel: 2;
uint32_t: 24;

	/* DWORD - 064 */
	uint32_t gen_vs_len: 16;
uint32_t: 16;

	/* DWORD - 068 */
	uint32_t fifo_dbg_sel: 4;
	uint32_t patgen_dbg_sel: 4;
uint32_t: 24;

	/* DWORD - 06C */
	uint32_t patgen_vtotal_msb: 16;
uint32_t: 16;

	/* DWORD - 070 */
uint32_t: 32;

	/* DWORD - 074 */
uint32_t: 32;

	/* DWORD - 078 */
uint32_t: 32;

	/* DWORD - 07C */
uint32_t: 32;

	/* DWORD - 080 */
	uint32_t top_dbg_sig: 32;

	/* DWORD - 084 */
	uint32_t patgen_dbg_mon: 32;

	/* DWORD - 088 */
uint32_t: 32;

	/* DWORD - 08C */
uint32_t: 32;
};

union disp_fefifo_hal_union {
	uint32_t reg[HAL_FEFIFO_REG_NUM];
	struct disp_fefifo_reg_t field;
};

struct disp_fefifo_conf_reg {
	union disp_fefifo_hal_union fefifo_reg;
	uint32_t fefifo_reg_mask[HAL_FEFIFO_REG_NUM];
};


#endif
