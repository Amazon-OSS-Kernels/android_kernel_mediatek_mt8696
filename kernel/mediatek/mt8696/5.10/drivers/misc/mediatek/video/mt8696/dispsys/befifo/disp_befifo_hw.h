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

#ifndef _DISP_BEFIFO_HW_H_
#define _DISP_BEFIFO_HW_H_

#include <linux/types.h>

#define HAL_BEFIFO_REG_NUM (0x100/4)
#define HAL_BEFIFO_REG_CONF_CNT (0x100/4)

#define DISP_BEFIFO_REG_BASE (0x1400F000)
#define BEFIFO_REG_MASK_DEF (0xFFFFFFFF)


#define BEFIFO_TG_TOTAL   0x000
#define TG_HTOTAL_FLD (0xFFFF << 0)
#define TG_VTOTAL_LSB_FLD (0xFFFF << 16)
#define BEFIFO_TG_ACTIVE  0x004
#define TG_HACTIVE_FLD (0xFFFF << 0)
#define TG_VACTIVE_FLD (0xFFFF << 16)
#define BEFIFO_TG_OFFSET  0x008
#define TG_HOFFSET_FLD (0xFFFF << 0)
#define TG_VOFFSET_FLD (0xFFFF << 16)
#define BEFIFO_TG_FRONT   0x00C
#define TG_HFRONT_FLD (0xFFFF << 0)
#define TG_VFRONT_FLD (0xFFFF << 16)
#define BEFIFO_TG_SYNCW    0x010
#define TG_HSYNCW_FLD (0xFF << 0)
#define TG_VSYNCW_FLD (0xFF << 8)
#define BEFIFO_CTL     0x014
#define CLR_BY_VS_FLD (0x3F << 4)
#define TG_EN_FLD (0x1 << 10)
#define SWAP_OUT_FLD (0xF << 12)
#define CRC_TRIG_FLD (0x3F << 16)
#define CLR_IRQ_READ_ERR_FLD (0x1 << 23)
#define CLR_IRQ_FRAME_DONE_FLD (0x1 << 24)
#define FORCE_IRQ_FLD (0x3 << 25)


#define BEFIFO_PATGEN_Y_BLANK   0x028
#define BEFIFO_PATGEN_CB_BLANK   0x02C
#define BEFIFO_PATGEN_CR_BLANK   0x030


#define BEFIFO_PATGEN_EN   0x038
#define PATGEN_EN_FLD (0x1 << 0)


#define BEFIFO_PATGEN_TOTAL   0x03C
#define PATGEN_HTOTAL_FLD (0xFFFF << 0)
#define PATGEN_VTOTAL_LSB_FLD (0xFFFF << 16)
#define BEFIFO_PATGEN_ACTIVE   0x040
#define PATGEN_HACTIVE_FLD (0xFFFF << 0)
#define PATGEN_VACTIVE_FLD (0xFFFF << 16)
#define BEFIFO_PATGEN_SYNC_W   0x044
#define PATGEN_HSYNC_W_FLD   (0xFF << 0)
#define PATGEN_VSYNC_W_FLD   (0xFF << 8)
#define BEFIFO_PATGEN_START   0x048
#define PATGEN_HSTART_FLD  (0xFFFF << 0)
#define PATGEN_VSTART_FLD  (0xFFFF << 16)
#define BEFIFO_PATGEN_DATA_CFG1   0x04C
#define BEFIFO_PATGEN_ACTIVE_CR_FLD   (0xFFFF << 0)
#define BEFIFO_PATGEN_ACTIVE_Y_FLD   (0xFFFF << 16)
#define BEFIFO_PATGEN_DATA_CFG2   0x050
#define BEFIFO_PATGEN_ACTIVE_CB_FLD   (0xFFFF << 0)
#define BEFIFO_PATGEN_BLANK_Y_FLD   (0xFFFF << 16)
#define BEFIFO_PATGEN_DATA_CFG3   0x054
#define BEFIFO_PATGEN_BLANK_CR_FLD   (0xFFF << 0)
#define BEFIFO_PATGEN_BLANK_CB_FLD   (0xFFF << 16)


#define BEFIFO_DATA_BLANK_Y   0x058
#define BEFIFO_DATA_BLANK_Y_FLD   (0xFFF << 0)

#define BEFIFO_RD_HACTIVE     0x05C
#define RD_HACTIVE_OFFSET_FLD (0xFFFF << 0)
#define RD_HACTIVE_W_FLD (0xFFFF << 16)

#define BEFIFO_RD_VACTIVE     0x060
#define RD_VACTIVE_OFFSET_FLD (0xFFFF << 0)
#define RD_VACTIVE_W_FLD (0xFFFF << 16)

#define BEFIFO_PATGEN_DEF_CONF   0x064
#define BEFIFO_PATGEN_DEF_ACTIVE_FLD   (0x1 << 0)
#define BEFIFO_PATGEN_DEF_BLANK_FLD   (0x1 << 1)

#define BEFIFO_SHADOW     0x068
#define SHDW_EN_FLD  (0x1 << 0)
#define SHDW_TRIGGER_FLD  (0x1 << 1)

#define BEFIFO_SOFT_RESET     0x070
#define SOF_1t2P_CLK_RST  (0x1 << 0)
#define SOF_PIXEL_CLK_RST (0x1 << 1)

#define BEFIFO_TG_TOTAL_MSB     0x078
#define TG_VTOTAL_MSB_FLD 0xFFFF

#define BEFIFO_PATGEN_TOTAL_MSB   0x07C
#define PATGEN_VTOTAL_MSB_FLD 0xFFFF

#define BEFIFO_CRC1_STATUS   0x084
#define BEFIFO_CRC2_STATUS   0x088
#define BEFIFO_CRC3_STATUS   0x08C
#define BEFIFO_HW_STATUS     0x094
#define BEFIFO_VRR_STATUS    0x0AC


/********************************
 * DISP BE FIFO Register
 ********************************/
struct disp_befifo_reg_t {
	/* DWORD - 000 */
	uint32_t tg_htotal: 16;
	uint32_t tg_vtotal_lsb: 16;

	/* DWORD - 004 */
	uint32_t tg_hactive: 16;
	uint32_t tg_vactive: 16;

	/* DWORD - 008 */
	uint32_t tg_hoffset: 16;
	uint32_t tg_voffset: 16;

	/* DWORD - 00C */
	uint32_t tg_hfront: 16;
	uint32_t tg_vfront: 16;

	/* DWORD - 010 */
	uint32_t tg_hsync_w: 16;
	uint32_t tg_vsync_w: 16;

	/* DWORD - 014 */
	uint32_t tg_hs_polar: 1;
	uint32_t tg_vs_polar: 1;
	uint32_t be_2pto1p_di_swap: 1;
	uint32_t be_2pto1p_do_swap: 1;
	uint32_t be_2pto1p_clr_fifo_wr_by_vs: 1;
	uint32_t be_2pto1p_clr_fifo_rd_by_vs: 1;
	uint32_t be_2pto1p_clr_fifo_waddr_by_vs: 1;
	uint32_t be_2pto1p_clr_fifo_raddr_by_vs: 1;
	uint32_t be_rdy2de_clr_fifo_waddr_by_vs: 1;
	uint32_t be_rdy2de_clr_fifo_raddr_by_vs: 1;
	uint32_t tg_en: 1;
uint32_t: 1;
	uint32_t data_out_swap: 4;
	uint32_t be_2pto1p_o_crc_start: 1;
	uint32_t be_2pto1p_o_crc_clr: 1;
	uint32_t be_rdy2de_o_crc_start: 1;
	uint32_t be_rdy2de_o_crc_clr: 1;
	uint32_t be_swap_o_crc_start: 1;
	uint32_t be_swap_o_crc_clr: 1;
uint32_t: 1;
	uint32_t be_rdy2de_irq_clr: 2;
	uint32_t be_rdy2de_irq_force: 2;
	uint32_t be_rdy2de_err_empty_len_clr: 1;
	uint32_t be_rdy2de_rd_err_flag_clr: 1;
	uint32_t be_rdy2de_o_vs_err_clr: 1;
	uint32_t be_rdy2de_dbg_cnt_clr: 1;
	uint32_t sof_clr_be_tg_en: 1;


	/* DWORD - 018 */
uint32_t: 16;
	uint32_t be_2pto1p_fifo_full_th: 16;

	/* DWORD - 01C */
uint32_t: 16;
	uint32_t be_rdy2de_fifo_full_th: 16;

	/* DWORD - 020 */
uint32_t: 32;

	/* DWORD - 024 */
	uint32_t be_2pto1p_dbg_sel: 4;
	uint32_t be_rdy2de_dbg_sel: 4;
	uint32_t be_patgen_dbg_sel: 4;
	uint32_t be_top_dbg_sel: 1;

	/* DWORD - 028 */
	uint32_t data_y_blank: 32;

	/* DWORD - 02C */
	uint32_t data_cb_blank: 32;

	/* DWORD - 030 */
	uint32_t data_cr_blank: 32;

	/* DWORD - 034 */
	uint32_t be_2pto1p_vs_dly: 16;
uint32_t: 16;

	/* DWORD - 038 */
	uint32_t patgen_en: 1;
uint32_t: 31;

	/* DWORD - 03C */
	uint32_t patgen_htotal: 16;
	uint32_t patgen_vtotal_lsb: 16;

	/* DWORD - 040 */
	uint32_t patgen_hactive: 16;
	uint32_t patgen_vactive: 16;

	/* DWORD - 044 */
	uint32_t patgen_hsync_w: 8;
	uint32_t patgen_vsync_w: 8;
uint32_t: 16;

	/* DWORD - 048 */
	uint32_t patgen_h_start: 16;
	uint32_t patgen_v_start: 16;

	/* DWORD - 04C */
	uint32_t patgen_active_cr: 16;
	uint32_t patgen_active_y: 16;

	/* DWORD - 050 */
	uint32_t patgen_active_cb: 16;
	uint32_t patgen_blank_yc: 16;

	/* DWORD - 054 */
	uint32_t patgen_blank_cr: 12;
uint32_t: 4;
	uint32_t patgen_blank_cb: 12;
uint32_t: 4;

	/* DWORD - 058 */
	uint32_t data_blank_y: 12;
uint32_t: 24;

	/* DWORD - 05C */
	uint32_t be_rd_hactive_ofst: 16;
	uint32_t be_rd_hactive_w: 16;

	/* DWORD - 060 */
	uint32_t be_rd_vactive_ofst: 16;
	uint32_t be_rd_vactive_w: 16;


	/* DWORD - 064 */
	uint32_t patgen_active_cfg_en: 1;
	uint32_t patgen_blank_cfg_en: 1;
uint32_t: 30;

	/* DWORD - 068 */
	uint32_t shdw_en: 1;
	uint32_t shdw_trigger: 1;
uint32_t: 30;

	/* DWORD - 06C */
uint32_t: 4;
	uint32_t shdw_data_sel: 12;
uint32_t: 16;

	/* DWORD - 070 */
	uint32_t rg_sof_rst: 2;
uint32_t: 30;

	/* DWORD - 074 */
	uint32_t be_2ptop_gen_vs_len: 16;
uint32_t: 16;

	/* DWORD - 078 */
	uint32_t tg_vtotal_msb: 16;
uint32_t: 16;

	/* DWORD - 07C */
	uint32_t patgen_vtotal_msb: 16;
uint32_t: 16;

	/* DWORD - 080 */
	uint32_t be_rdy2de_fifo_empty: 1;
	uint32_t be_rdy2de_fifo_full: 1;
	uint32_t be_2pto1p_fifo_empty: 1;
	uint32_t be_2pto1p_fifo_full: 1;
uint32_t: 28;

	/* DWORD - 084 */
	uint32_t crc_result_2pto1p: 16;
	uint32_t crc_rdy_2pto1p: 1;
uint32_t: 15;

	/* DWORD - 088 */
	uint32_t crc_result_rdy2de: 16;
	uint32_t crc_rdy_rdy2de: 1;
uint32_t: 15;

	/* DWORD - 08C */
	uint32_t crc_result_rdy2de_swap: 16;
	uint32_t crc_rdy_rdy2de_swap: 1;
uint32_t: 15;

	/* DWORD - 090 */
	uint32_t be_top_dbg_mon: 32;

	/* DWORD - 094 */
	uint32_t rdy2de_wr2rd_dly_cnt: 14;
	uint32_t rdy2de_rd2wd_dly_cnt: 14;
	uint32_t rdy2de_rd_err_flag: 1;
	uint32_t rdy2de_o_vsync_err: 1;
	uint32_t rdy2de_irq_b: 1;
	uint32_t rdy2de_frame_done: 1;

	/* DWORD - 098 */
	uint32_t rdy2de_err_empty_len: 13;
uint32_t: 19;

	/* DWORD - 09C */
	uint32_t shdw_data: 32;

	/* DWORD - 0A0 */
	uint32_t read_hcnt: 16;
	uint32_t read_line_cnt: 16;

	/* DWORD - 0A4 */
	uint32_t be_top_dbg_sig: 32;

	/* DWORD - 0A8 */
	uint32_t be_patgen_dbg_sig: 32;

	/* DWORD - 0AC */
	uint32_t be_vcnt_total: 32;
};

union disp_befifo_hal_union {
	uint32_t reg[HAL_BEFIFO_REG_NUM];
	struct disp_befifo_reg_t field;
};

struct disp_befifo_conf_reg {
	union disp_befifo_hal_union befifo_reg;
	uint32_t befifo_reg_mask[HAL_BEFIFO_REG_NUM];
};


#endif
