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

#ifndef _DISP_R2R_HW_H_
#define _DISP_R2R_HW_H_

#include <linux/types.h>


#define HAL_R2R_REG_NUM (0x100/4)
#define HAL_R2R_REG_CONF_CNT (0x100)

#define DISP_R2R_REG_BASE (0x15006000)
#define R2R_REG_MASK_DEF  (0xFFFFFFFF)

#define R2R_TG_TOTAL_CTRL  (0x000)
#define R2R_TG_V_TOTAL_FLD (0x1FFF << 0)
#define R2R_TG_H_TOTAL_FLD (0x1FFF << 16)
#define R2R_TG_SHDW_EN_FLD (0x1 << 30)
#define R2R_TG_SHDW_SW_TRIG_FLD (0x1 << 31)
#define R2R_TG_VDE_CTRL  (0x004)
#define R2R_TG_VDEW_FLD  (0x1FFF << 0)
#define R2R_TG_HSYNC_W_FLD  (0x1FFF << 16)
#define R2R_TG_MODE444_FLD  (0x1 << 31)
#define R2R_TG_VSYNC_CTRL  (0x008)
#define R2R_TG_VSYNC_END_FLD (0x1FFF << 0)
#define R2R_TG_VSYNC_START_FLD (0x1FFF << 16)
#define R2R_TG_HACTIVE_CTRL  (0x00C)
#define R2R_TG_HACTIVE_END_FLD (0x1FFF << 0)
#define R2R_TG_HACTIVE_START_FLD (0x1FFF << 16)
#define R2R_TG_VACTIVE_CTRL  (0x010)
#define R2R_TG_VACTIVE_END_FLD (0x1FFF << 0)
#define R2R_TG_VACTIVE_START_FLD (0x1FFF << 16)
#define R2R_TG_POLAR_CTRL  (0x014)
#define R2R_TG_REQ_TH_FLD (0xFF << 0)
#define R2R_TG_H_MAX_FLD (0x1FF << 8)
#define R2R_TG_HDEW_PITCH_FLD (0x3FF << 20)
#define R2R_TG_SW_RST_FLD (0x1 << 31)
#define R2R_HW_CTRL0  (0x018)
#define R2R_CTRL_EN_FLD (0x1 << 0)
#define R2R_CTRL_GREQ_EN_FLD (0x1 << 1)
#define R2R_CTRL_BURST4_FLD (0x1 << 2)
#define R2R_CTRL_BURST_EN_FLD (0x1 << 3)
#define R2R_CTRL_MODE_10B_FLD (0x1 << 4)
#define R2R_CTRL_MODE_12B_FLD (0x1 << 5)
#define R2R_CTRL_VS_PUL_ONCE_FLD (0x1 << 6)
#define R2R_CTRL_VS_PUL_EN_FLD (0x1 << 7)
#define R2R_HW_CTRL1 (0x01C)
#define R2R_CTRL_PATGEN_COLOR_FLD (0x1 << 28)
#define R2R_CTRL_PATGEN_EN_FLD (0x1 << 29)
#define R2R_PATGEN_CONF0 (0x020)
#define R2R_PATGEN_BND_H_FLD (0xFFFF << 0)
#define R2R_PATGEN_BND_W_FLD (0xFFFF << 16)
#define R2R_PATGEN_CONF1 (0x024)
#define R2R_PATGEN_COLOR_MOD_FLD (0x3 << 21)
#define R2R_PATGEN_MOD_FLD (0x1 << 23)
#define R2R_PATGEN_FREERUN_SPEED_FLD (0xF << 25)
#define R2R_PATGEN_FREERUN_EN_FLD (0x1 << 29)
#define R2R_HW_CTRL2 (0x028)
#define R2R_CTRL_PATGEN_Y_FLD (0xFFF << 0)
#define R2R_CTRL_422_TO_444_EN_FLD (0x1 << 16)
#define R2R_CTRL_CHROMA_SWAP_FLD (0x1 << 17)
#define R2R_PATGEN_CONF2 (0x02C)
#define R2R_PATGEN_CC_FLD (0xFFF << 0)
#define R2R_PATGEN_C_FLD (0xFFF << 16)
#define R2R_RDMA_ADDR_Y0 (0x030)
#define R2R_RDMA_ADDR_C0 (0x034)
#define R2R_RDMA_ADDR_CC0 (0x038)

#define R2R_INT_STATE (0x07C)
#define R2R_INT_CLR_FLD (0x1 << 0)

#define R2R_DBG_CRC0 (0x0AC)
#define R2R_DBG_CRC1 (0x0B0)
#define R2R_DBG_CRC2 (0x0B4)


/********************************
 * DISP R2R Register
 ********************************/
struct disp_r2r_reg_t {
	/* DWORD - 000 */
	uint32_t vsync_total: 13;
uint32_t: 3;
	uint32_t hsync_total: 13;
uint32_t: 1;
	uint32_t shdw_en: 1;
	uint32_t shdw_trig: 1;

	/* DWORD - 004 */
	uint32_t vdew: 13;
uint32_t: 3;
	uint32_t hsync_width: 13;
uint32_t: 2;
	uint32_t mode_444: 1;

	/* DWORD - 008 */
	uint32_t vsync_end: 13;
uint32_t: 3;
	uint32_t vsync_start: 13;
uint32_t: 3;

	/* DWORD - 00C */
	uint32_t hde_end: 13;
uint32_t: 3;
	uint32_t hde_start: 13;
uint32_t: 3;

	/* DWORD - 010 */
	uint32_t vde_end: 13;
uint32_t: 3;
	uint32_t vde_start: 13;
uint32_t: 3;

	/* DWORD - 014 */
	uint32_t req_th: 8;
	uint32_t word_max: 9;
	uint32_t de_polar: 1;
	uint32_t vsync_polar: 1;
	uint32_t hsync_polar: 1;
	uint32_t hdew_pitch: 10;
uint32_t: 1;
	uint32_t sw_rst: 1;

	/* DWORD - 018 */
	uint32_t enable: 1;
	uint32_t greq_enable: 1;
	uint32_t burst4: 1;
	uint32_t burst_en: 1;
	uint32_t mode_10bit: 1;
	uint32_t mode_12bit: 1;
	uint32_t ref2r2r_vs_pul_once: 1;
	uint32_t ref2r2r_vs_pul_en: 1;
	uint32_t ultra_th: 9;
	uint32_t ultra_en: 1;
	uint32_t int_mask: 1;
	uint32_t int_clr: 1;
	uint32_t preultra_th: 9;
	uint32_t preultra_en: 1;
	uint32_t empty_sel: 1;
	uint32_t prefetch_ultra_en: 1;

	/* DWORD - 01C */
	uint32_t auto_cnt_cfg: 3;
	uint32_t auto_en: 1;
	uint32_t auto_cnt_clr: 1;
	uint32_t auto_sel: 2;
uint32_t: 5;
	uint32_t empty_th: 9;
uint32_t: 2;
	uint32_t empty_ckr: 1;
	uint32_t bnd_en: 1;
	uint32_t mode_3d: 2;
	uint32_t inv_3d: 1;
	uint32_t color_en: 1;
	uint32_t patgen_en: 1;
	uint32_t init_420: 1;
	uint32_t mode_420: 1;

	/* DWORD - 020 */
	uint32_t bnd_h: 16;
	uint32_t bnd_w: 16;

	/* DWORD - 024 */
	uint32_t ypat2: 4;
	uint32_t ypat1: 4;
	uint32_t pat_duty_cycle: 5;
	uint32_t left_side: 1;
	uint32_t angle_sel: 3;
	uint32_t freq_sel: 3;
	uint32_t hor_line_en: 1;
	uint32_t color_mode: 2;
	uint32_t pat_mode: 1;
uint32_t: 1;
	uint32_t freerun_speed: 4;
	uint32_t freerun_en: 1;
uint32_t: 2;

	/* DWORD - 028 */
	uint32_t padding_y: 12;
uint32_t: 4;
	uint32_t r2r_422to444_en: 1;
	uint32_t chroma_swap: 1;
uint32_t: 14;

	/* DWORD - 02C */
	uint32_t padding_cc: 12;
uint32_t: 4;
	uint32_t padding_c: 12;
uint32_t: 4;

	/* DWORD - 030 */
	uint32_t y_addr0: 28;
uint32_t: 4;

	/* DWORD - 034 */
	uint32_t c_addr0: 28;
uint32_t: 4;

	/* DWORD - 038 */
	uint32_t cc_addr0: 28;
uint32_t: 4;

	/* DWORD - 03C */
	uint32_t y_addr1: 28;
uint32_t: 4;

	/* DWORD - 040 */
	uint32_t c_addr1: 28;
uint32_t: 4;

	/* DWORD - 044 */
	uint32_t cc_addr1: 28;
uint32_t: 4;

	/* DWORD - 048 */
	uint32_t y_addr2: 28;
uint32_t: 4;

	/* DWORD - 04C */
	uint32_t c_addr2: 28;
uint32_t: 4;

	/* DWORD - 050 */
	uint32_t cc_addr2: 28;
uint32_t: 4;

	/* DWORD - 054 */
	uint32_t y_addr3: 28;
uint32_t: 4;

	/* DWORD - 058 */
	uint32_t c_addr3: 28;
uint32_t: 4;

	/* DWORD - 05C */
	uint32_t cc_addr3: 28;
uint32_t: 4;

	/* DWORD - 060 */
	uint32_t y_addr4: 28;
uint32_t: 4;

	/* DWORD - 064 */
	uint32_t c_addr4: 28;
uint32_t: 4;

	/* DWORD - 068 */
	uint32_t cc_addr4: 28;
uint32_t: 4;

	/* DWORD - 06C */
	uint32_t y_addr5: 28;
uint32_t: 4;

	/* DWORD - 070 */
	uint32_t c_addr5: 28;
uint32_t: 4;

	/* DWORD - 074 */
	uint32_t cc_addr5: 28;
uint32_t: 4;

	/* DWORD - 078 */
	uint32_t hrt_urgent_th: 9;
uint32_t: 2;
	uint32_t hrt_urgent_en: 1;
uint32_t: 20;

	/* DWORD - 07C */
	uint32_t int_stat: 1;
uint32_t: 31;

	/* DWORD - 080 */
uint32_t: 22;
	uint32_t st00_y_glcomd: 1;
	uint32_t st00_y_gdrdy: 1;
	uint32_t st00_y_gburst: 4;
	uint32_t st00_y_gpreultra: 1;
	uint32_t st00_y_gultra: 1;
	uint32_t st00_y_gclast: 1;
	uint32_t st00_y_greq: 1;

	/* DWORD - 084 */
uint32_t: 4;
	uint32_t st01_y_gaddr: 32;

	/* DWORD - 088 */
uint32_t: 22;
	uint32_t st02_c_glcomd: 1;
	uint32_t st02_c_gdrdy: 1;
	uint32_t st02_c_gburst: 4;
	uint32_t st02_c_gpreultra: 1;
	uint32_t st02_c_gultra: 1;
	uint32_t st02_c_gclast: 1;
	uint32_t st02_c_greq: 1;

	/* DWORD - 08C */
uint32_t: 4;
	uint32_t st03_c_gaddr: 32;

	/* DWORD - 090 */
	uint32_t st04_cc_glcomd: 1;
	uint32_t st04_cc_gdrdy: 1;
	uint32_t st04_cc_gburst: 4;
	uint32_t st04_cc_gpreultra: 1;
	uint32_t st04_cc_gultra: 1;
	uint32_t st04_cc_gclast: 1;
	uint32_t st04_cc_greq: 1;

	/* DWORD - 094 */
uint32_t: 4;
	uint32_t st05_cc_gaddr: 32;

	/* DWORD - 098 */
	uint32_t st06_y_req_mon: 32;

	/* DWORD - 09C */
	uint32_t st07_c_req_mon: 32;

	/* DWORD - 0A0 */
	uint32_t st08_cc_req_mon: 32;

	/* DWORD - 0A4 */
	uint32_t st09_cc_out: 10;
	uint32_t st09_c_out: 10;
	uint32_t st09_y_out: 10;
	uint32_t st09_vsync: 1;
	uint32_t st09_hsync: 1;

	/* DWORD - 0A8 state0a */
	uint32_t st0a_auto_addr_cnt: 3;
uint32_t: 5;
	uint32_t st0a_y_empty: 1;
	uint32_t st0a_c_empty: 1;
	uint32_t st0a_cc_empty: 1;
uint32_t: 1;
	uint32_t st0a_y_hrt_urgent: 1;
	uint32_t st0a_c_hrt_urgent: 1;
	uint32_t st0a_cc_hrt_urgent: 1;
	uint32_t st0a_hrt_urgent: 1;
	uint32_t st0a_y_out: 12;
	uint32_t st0a_de: 1;
	uint32_t st0a_vsync: 1;
	uint32_t st0a_hsync: 1;
	uint32_t st0a_int: 1;

	/* DWORD - 0AC */
	uint32_t crc0_dbg: 32;

	/* DWORD - 0B0 */
	uint32_t crc1_dbg: 32;

	/* DWORD - 0B4 */
	uint32_t crc2_dbg: 32;

	/* DWORD - 0B8 */
uint32_t: 32;

	/* DWORD - 0BC */
uint32_t: 32;
};

union disp_r2r_hal_union {
	uint32_t reg[HAL_R2R_REG_NUM];
	struct disp_r2r_reg_t field;
};

struct disp_r2r_conf_reg {
	union disp_r2r_hal_union r2r_reg;
	uint32_t r2r_reg_mask[HAL_R2R_REG_NUM];
};


#endif
