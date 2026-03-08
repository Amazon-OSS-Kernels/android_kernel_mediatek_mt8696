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



#define LOG_TAG "DOVI_VFY_DRV"

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "disp_info.h"
#include "disp_hw_mgr.h"
#include "disp_clk.h"
#include "dovi_log.h"
#include "disp_dovi_main.h"
#include "dovi_vdo_fe_hal.h"
#include "dovi_gfx_fe_hal.h"
#include "dovi_be_hal.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"
#include "disp_dovi_cmd.h"
#include "disp_dovi_vdo_fe_if.h"
#include "disp_dovi_gfx_fe_if.h"
#include "disp_dovi_be_if.h"
#include "disp_dovi_io.h"
#include "dovi_table.h"
#include "fmt_hal.h"
#include "ml_hal.h"
#include "disp_path.h"
#include "ml_hw.h"
#include "disp_dovi_unit_test.h"
#include "disp_dovi_unit_test_table.h"
#include "disp_dovi_ut_table1.h"
#include "disp_dovi_ut_table2.h"
#include "disp_dovi_ut_table3.h"
#include "disp_dovi_ut_table4.h"
#include "disp_adl_if.h"
#include "disp_ml_if.h"


/* define dovi UT relate module resource */
/* vdo3 0x15001000,1200,1400 */
/* vdo4 0x15008000,8200,8400 */
char *vdo3_reg_base;
char *vdo4_reg_base;

/* osd */
/* fhd 0x14003000 */
/* uhd 0x14004000 */
char *fhd_osd_reg_base;
char *uhd_osd_reg_base;

/* adl */
/* main 0x15013798 */
char *dsys_adl_ml_reg_base;
char *msys_adl_ml_reg_base;

uint32_t *mfe_adl_buf;
dma_addr_t mfe_adl_buf_pa;

/* sub 0x15013780 */
uint32_t *sfe_adl_buf;
dma_addr_t sfe_adl_buf_pa;

/* fhd 0x14013798 */
uint32_t *fhd_fe_adl_buf;
dma_addr_t fhd_fe_adl_buf_pa;

/* uhd 0x14013780 */
uint32_t *uhd_fe_adl_buf;
dma_addr_t uhd_fe_adl_buf_pa;

/* be 0x14013718 */
uint32_t *be_fe_adl_buf;
dma_addr_t be_fe_adl_buf_pa;

/* ml */
/* dsys 0x15013004 */
uint32_t *dsys_ml_buf;
dma_addr_t dsys_ml_buf_pa;

/* msys 0x14013004 */
uint32_t *msys_ml_buf;
dma_addr_t msys_ml_buf_pa;

/* video in 0x14002000*/
char *vin_reg_base;

/*fmt 0x14001000*/
char *fmt_reg_base;

/*fmt 0x14014000*/
char *disp_mix_reg_base;

/*fifo*/
/*fe m fifo 0x15010000*/
char *m_fifo_reg_base;

/*fe m fifo 0x15017000*/
char *s_fifo_reg_base;

/*fe m fifo 0x14010000*/
char *fhd_fifo_reg_base;

/*mmsys config 0x14000000*/
char *mmsys_cfg_reg_base;

/*fe m fifo 0x14011000*/
char *uhd_fifo_reg_base;

/*be fifo 0x1400f000*/
char *be_fifo_reg_base;

/*disptop 0x15000000*/
char *d_top_reg_base;

/*mmtop 0x14001500*/
char *m_top_reg_base;

/*mhdr 0x15012000*/
char *mhdr_fe_reg_base;
/*shdr 0x15015000*/
char *shdr_fe_reg_base;
/*fhdhdr 0x14009000*/
char *fhdr_fe_reg_base;
/*uhdhdr 0x1400a000*/
char *uhdr_fe_reg_base;
/*behdr 0x1400b000*/
char *hdr_be_reg_base;

uint32_t *ut_y_addr_va[2];
uint32_t *ut_c_addr_va[2];
dma_addr_t ut_y_addr_pa[2];
dma_addr_t ut_c_addr_pa[2];
uint32_t *ut_graphic_addr_va[2];
dma_addr_t ut_graphic_addr_pa[2];
uint32_t *ut_graphic_header_va[2];
dma_addr_t ut_graphic_header_pa[2];
uint32_t *ut_vin_y_addr_va;
dma_addr_t ut_vin_y_addr_pa;
uint32_t *ut_vin_c_addr_va;
dma_addr_t ut_vin_c_addr_pa;


uint32_t dovi_option;
static wait_queue_head_t disp_dovi_wq;
static uint32_t disp_dovi_wakeup_thread;
static struct task_struct *disp_dovi_thread;
static struct mutex disp_dovi_main_mutex;

#define DOVI_PATH_TEST_MASK 0x100

bool dovi_ut_enable;
uint32_t d_ml_tbl_dpt;
uint32_t m_ml_tbl_dpt;
bool ml_update;
bool ml_adl_buf_alloced;

struct dovi_unit_info_t dv_ut_info;

void dovi_ut_config_adl(enum ADL_CLIENT clit, bool en,
	uint32_t *lut)
{
	uint32_t *dst_va = NULL;
	dma_addr_t dst_pa = 0;
	uint32_t lut_size = 0;

	switch (clit) {
	case DV_ADL_V_MAIN:
		dst_va = mfe_adl_buf;
		dst_pa = mfe_adl_buf_pa;
		lut_size = ADL_UT_LUT_BUF_SIZE;
		break;
	case DV_ADL_V_SUB:
		dst_va = sfe_adl_buf;
		dst_pa = sfe_adl_buf_pa;
		lut_size = ADL_UT_LUT_BUF_SIZE;
		break;
	case DV_ADL_G_FHD:
		dst_va = fhd_fe_adl_buf;
		dst_pa = fhd_fe_adl_buf_pa;
		lut_size = ADL_UT_LUT_BUF_SIZE;
		break;
	case DV_ADL_G_UHD:
		dst_va = uhd_fe_adl_buf;
		dst_pa = uhd_fe_adl_buf_pa;
		lut_size = ADL_UT_LUT_BUF_SIZE;
		break;
	case DV_SCRM:
		dst_va = be_fe_adl_buf;
		dst_pa = be_fe_adl_buf_pa;
		lut_size = ADL_UT_SCM_BUF_SIZE;
		break;
	default:
		dovi_printf("client error\n");
		break;
	}

	memset(dst_va, 0, lut_size);
	memcpy((void *)dst_va, (void *)lut, lut_size);

	/* todo flush cache */
	dma_map_single(dovi_dev, dst_va, lut_size,
	DMA_TO_DEVICE);
	dma_unmap_single(dovi_dev, dst_pa, lut_size,
	DMA_TO_DEVICE);

	disp_adl_cfg_client_en(clit, en, 0);
	disp_adl_cfg_client(clit, dst_pa, lut_size);
}

void disp_dovi_ut_ml_update(void)
{
	uint32_t mva_msb = 0;
	uint32_t mva_lsb = 0;
	uintptr_t base_reg = 0;
	uint32_t value = 0;
	uint32_t depth = 0;

	if (!ml_update) {
		dovi_error("dovi routine over time\n");
		return;
	}

	if ((d_ml_tbl_dpt % 4) || (m_ml_tbl_dpt % 4)) {
		dovi_error("ml tbl depth error %d %d\n",
			d_ml_tbl_dpt, m_ml_tbl_dpt);
		return;
	}
	if (d_ml_tbl_dpt) {
		mva_msb = ((0xffff0000 & (dsys_ml_buf_pa >> 5)) >> 16);
		mva_lsb = ((0x0000ffff & (dsys_ml_buf_pa >> 5)));

		base_reg = ml_reg_base[ML_DSYS];
		depth = d_ml_tbl_dpt;
		/*dsys bmask enable*/
		reg_ml_bmsk_en((base_reg + ML_MSK_EN), 0x1, 0x1);

		/*08 set en and sw triggle mode*/
		value = 0x8000 | depth / 4;
		reg_ml_en((base_reg + ML_EN), value, 0xffff);

		/*04 ml read length*/
		value = depth / 4;
		reg_ml_depth((base_reg + ML_DEPTH), value, 0xffff);

		/*0c ml addr0*/
		reg_ml_addr0((base_reg + ML_ADDR0), mva_lsb, 0xffff);

		/*10 ml addr1*/
		reg_ml_addr1((base_reg + ML_ADDR1), mva_msb, 0x1fff);

		/*1c ml ds msk*/
		reg_ml_ds_msk((base_reg + ML_DS_MSK), 0xc008, 0xffff);

		/*40 ml ds write en*/
		reg_ml_ds_w_en((base_reg + ML_DS_WE), 0x1004, 0xffff);

		/*1c0 ml 64bit cmd en*/
		reg_ml_64b_cmd_en((base_reg + ML_64B_EN), 0x8003, 0xffff);

		/*74 ml sw trigge mode*/
		reg_ml_64b_cmd_en((base_reg + ML_TRIG_MD), 0x1000, 0x1000);

		/*74 ml sw trigge */
		reg_ml_64b_cmd_en((base_reg + ML_TRIG_MD), 0x9000, 0x9000);
	}
	if (m_ml_tbl_dpt) {
		mva_msb = ((0xffff0000 & (msys_ml_buf_pa >> 5)) >> 16);
		mva_lsb = ((0x0000ffff & (msys_ml_buf_pa >> 5)));

		base_reg = ml_reg_base[ML_MSYS];
		depth = m_ml_tbl_dpt;

		/*dsys bmask enable*/
		reg_ml_bmsk_en((base_reg + ML_MSK_EN), 0x1, 0x1);

		/*08 set en and sw triggle mode*/
		value = 0x8000 | depth / 4;
		reg_ml_en((base_reg + ML_EN), value, 0xffff);

		/*04 ml read length*/
		value = depth / 4;
		reg_ml_depth((base_reg + ML_DEPTH), value, 0xffff);

		/*0c ml addr0*/
		reg_ml_addr0((base_reg + ML_ADDR0), mva_lsb, 0xffff);

		/*10 ml addr1*/
		reg_ml_addr1((base_reg + ML_ADDR1), mva_msb, 0x1fff);

		/*1c ml ds msk*/
		reg_ml_ds_msk((base_reg + ML_DS_MSK), 0xc008, 0xffff);

		/*40 ml ds write en*/
		reg_ml_ds_w_en((base_reg + ML_DS_WE), 0x1004, 0xffff);

		/*1c0 ml 64bit cmd en*/
		reg_ml_64b_cmd_en((base_reg + ML_64B_EN), 0x8003, 0xffff);

		/*74 ml sw trigge mode*/
		reg_ml_64b_cmd_en((base_reg + ML_TRIG_MD), 0x1000, 0x1000);

		/*74 ml sw trigge */
		reg_ml_64b_cmd_en((base_reg + ML_TRIG_MD), 0x9000, 0x9000);
	}
	ml_update = 0;
	d_ml_tbl_dpt = 0;
	m_ml_tbl_dpt = 0;

}

void disp_dovi_wakeup_routine(void)
{
	if (dovi_option & DOVI_PATH_TEST_MASK) {
		wake_up(&disp_dovi_wq);
		disp_dovi_wakeup_thread = 1;

		dovi_info("wakeup thread\n");
	}
}

void dovi_ut_480p_case(void)
{
	uint32_t *p_dispsys_reg = NULL;
	uint32_t *p_mmsys_reg = NULL;

	uint32_t *p_vdo_fe_lut;
	uint32_t *p_gfx_fe_lut;
	uint32_t *p_vdo_be_lut;
	bool layer_en[4] = {0};
	enum dovi_format out_fmt = 0;
	bool ll_on = 0;
	enum dovi_format in_fmt[4] = { 0 };

	out_fmt = dv_ut_info.out_format;
	ll_on = dv_ut_info.use_ll;
	layer_en[0] = dv_ut_info.dv_layer[0].on;
	in_fmt[0] = dv_ut_info.dv_layer[0].in_format;
	layer_en[1] = dv_ut_info.dv_layer[1].on;
	in_fmt[1] = dv_ut_info.dv_layer[1].in_format;
	layer_en[2] = dv_ut_info.dv_layer[2].on;
	in_fmt[2] = dv_ut_info.dv_layer[2].in_format;
	layer_en[3] = dv_ut_info.dv_layer[3].on;
	in_fmt[3] = dv_ut_info.dv_layer[3].in_format;

	if (out_fmt == DOVI_STD) {
		if (layer_en[0]) {
			p_vdo_fe_lut = idk_242_5005_ipt_ipt_v_lut;
			dovi_ut_config_adl(DV_ADL_V_MAIN, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5005_ipt_ipt_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5005_ipt_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 64;
		}
		if (layer_en[1]) {
			p_vdo_fe_lut = idk_242_5005_ipt_ipt_v_lut;
			dovi_ut_config_adl(DV_ADL_V_SUB, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5005_ipt_ipt_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5005_ipt_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 64;
		}
		if (layer_en[2]) {
			p_gfx_fe_lut = idk_242_5005_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_FHD, 1, p_gfx_fe_lut);
			p_mmsys_reg = idk_242_5005_sdr_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 132;
			d_ml_tbl_dpt = 0;
		}
		if (layer_en[3]) {
			p_gfx_fe_lut = idk_242_5005_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_UHD, 1, p_gfx_fe_lut);
			p_mmsys_reg = idk_242_5005_sdr_ipt_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 132;
			d_ml_tbl_dpt = 0;
		}

		p_vdo_be_lut = idk_242_5005_ipt_ipt_md_lut;
		dovi_ut_config_adl(DV_SCRM, 1, p_vdo_be_lut);
	}

	if (out_fmt == DOVI_SDR) {
		if (layer_en[0]) {
			p_vdo_fe_lut = idk_242_5005_ipt_sdr_v_lut;
			dovi_ut_config_adl(DV_ADL_V_MAIN, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5005_ipt_sdr_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5005_ipt_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 60;
		}
		if (layer_en[1]) {
			p_vdo_fe_lut = idk_242_5005_ipt_sdr_v_lut;
			dovi_ut_config_adl(DV_ADL_V_SUB, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5005_ipt_sdr_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5005_ipt_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 60;
		}
		if (layer_en[2]) {
			p_gfx_fe_lut = idk_242_5005_sdr_sdr_g_lut;
			dovi_ut_config_adl(DV_ADL_G_FHD, 1, p_gfx_fe_lut);
			p_mmsys_reg = idk_242_5005_sdr_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 124;
			d_ml_tbl_dpt = 0;
		}
		if (layer_en[3]) {
			p_gfx_fe_lut = idk_242_5005_sdr_sdr_g_lut;
			dovi_ut_config_adl(DV_ADL_G_UHD, 1, p_gfx_fe_lut);
			p_mmsys_reg = idk_242_5005_sdr_sdr_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 124;
			d_ml_tbl_dpt = 0;
		}
	}

	if (d_ml_tbl_dpt) {
		memset(dsys_ml_buf, 0, ML_DSYS_UT_BUF_SIZE);
		if (p_dispsys_reg != NULL)
			memcpy((void *)dsys_ml_buf, (void *)p_dispsys_reg,
			d_ml_tbl_dpt * 8);
		/* todo flush cache */
		dma_map_single(dovi_dev, dsys_ml_buf,
		d_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
		dma_unmap_single(dovi_dev, dsys_ml_buf_pa,
		d_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
	}

	if (m_ml_tbl_dpt) {
		memset(msys_ml_buf, 0, ML_MSYS_UT_BUF_SIZE);
		if (p_mmsys_reg != NULL)
			memcpy((void *)msys_ml_buf, (void *)p_mmsys_reg,
			m_ml_tbl_dpt * 8);
		/* todo flush cache */
		dma_map_single(dovi_dev, msys_ml_buf,
		m_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
		dma_unmap_single(dovi_dev, msys_ml_buf_pa,
		m_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
	}
}

void dovi_ut_720p_case(void)
{
	uint32_t *p_dispsys_reg = NULL;
	uint32_t *p_mmsys_reg = NULL;

	uint32_t *p_vdo_fe_lut;
	uint32_t *p_gfx_fe_lut;
	uint32_t *p_vdo_be_lut;
	bool layer_en[4] = {0};
	enum dovi_format out_fmt = 0;
	bool ll_on = 0;
	enum dovi_format in_fmt[4] = { 0 };

	out_fmt = dv_ut_info.out_format;
	ll_on = dv_ut_info.use_ll;
	layer_en[0] = dv_ut_info.dv_layer[0].on;
	in_fmt[0] = dv_ut_info.dv_layer[0].in_format;
	layer_en[1] = dv_ut_info.dv_layer[1].on;
	in_fmt[1] = dv_ut_info.dv_layer[1].in_format;
	layer_en[2] = dv_ut_info.dv_layer[2].on;
	in_fmt[2] = dv_ut_info.dv_layer[2].in_format;
	layer_en[3] = dv_ut_info.dv_layer[3].on;
	in_fmt[3] = dv_ut_info.dv_layer[3].in_format;

	if (out_fmt == DOVI_STD) {
		if (layer_en[0]) {
			p_vdo_fe_lut = idk_242_5004_ipt_ipt_v_lut;
			dovi_ut_config_adl(DV_ADL_V_MAIN, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5004_ipt_ipt_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5004_ipt_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[1]) {
			p_vdo_fe_lut = idk_242_5004_ipt_ipt_v_lut;
			dovi_ut_config_adl(DV_ADL_V_SUB, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5004_ipt_ipt_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5004_ipt_ipt_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[2]) {
			p_gfx_fe_lut = idk_242_5004_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_FHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5004_ipt_ipt_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5004_ipt_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[3]) {
			p_gfx_fe_lut = idk_242_5004_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_UHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5004_ipt_ipt_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5004_ipt_ipt_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 132;
		}

		p_vdo_be_lut = idk_242_5004_ipt_ipt_md_lut;
		dovi_ut_config_adl(DV_SCRM, 1, p_vdo_be_lut);
	}

	if (out_fmt == DOVI_SDR) {
		if (layer_en[0]) {
			p_vdo_fe_lut = idk_242_5004_ipt_sdr_v_lut;
			dovi_ut_config_adl(DV_ADL_V_MAIN, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5004_ipt_sdr_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5004_ipt_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[1]) {
			p_vdo_fe_lut = idk_242_5004_ipt_sdr_v_lut;
			dovi_ut_config_adl(DV_ADL_V_SUB, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5004_ipt_sdr_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5004_ipt_sdr_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[2]) {
			p_gfx_fe_lut = idk_242_5004_sdr_sdr_g_lut;
			dovi_ut_config_adl(DV_ADL_G_FHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5004_ipt_sdr_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5004_ipt_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[3]) {
			p_gfx_fe_lut = idk_242_5004_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_UHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5004_ipt_sdr_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5004_ipt_sdr_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 124;
		}

	}

	if (d_ml_tbl_dpt) {
		memset(dsys_ml_buf, 0, ML_DSYS_UT_BUF_SIZE);
		if (p_dispsys_reg != NULL)
			memcpy((void *)dsys_ml_buf, (void *)p_dispsys_reg,
			d_ml_tbl_dpt * 8);
		/* todo flush cache */
		dma_map_single(dovi_dev, dsys_ml_buf,
		d_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
		dma_unmap_single(dovi_dev, dsys_ml_buf_pa,
		d_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
	}

	if (m_ml_tbl_dpt) {
		memset(msys_ml_buf, 0, ML_MSYS_UT_BUF_SIZE);
		if (p_mmsys_reg != NULL)
			memcpy((void *)msys_ml_buf, (void *)p_mmsys_reg,
			m_ml_tbl_dpt * 8);
		/* todo flush cache */
		dma_map_single(dovi_dev, msys_ml_buf,
		m_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
		dma_unmap_single(dovi_dev, msys_ml_buf_pa,
		m_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
	}
}


void dovi_ut_1080p_case(void)
{
	uint32_t *p_dispsys_reg = NULL;
	uint32_t *p_mmsys_reg = NULL;

	uint32_t *p_vdo_fe_lut;
	uint32_t *p_gfx_fe_lut;
	uint32_t *p_vdo_be_lut;
	bool layer_en[4] = {0};
	enum dovi_format out_fmt = 0;
	bool ll_on = 0;
	enum dovi_format in_fmt[4] = { 0 };

	out_fmt = dv_ut_info.out_format;
	ll_on = dv_ut_info.use_ll;
	layer_en[0] = dv_ut_info.dv_layer[0].on;
	in_fmt[0] = dv_ut_info.dv_layer[0].in_format;
	layer_en[1] = dv_ut_info.dv_layer[1].on;
	in_fmt[1] = dv_ut_info.dv_layer[1].in_format;
	layer_en[2] = dv_ut_info.dv_layer[2].on;
	in_fmt[2] = dv_ut_info.dv_layer[2].in_format;
	layer_en[3] = dv_ut_info.dv_layer[3].on;
	in_fmt[3] = dv_ut_info.dv_layer[3].in_format;

	if (out_fmt == DOVI_STD) {
		if (layer_en[0]) {
			p_vdo_fe_lut = idk_242_5000_ipt_ipt_v_lut;
			dovi_ut_config_adl(DV_ADL_V_MAIN, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5000_ipt_ipt_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5000_ipt_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[1]) {
			p_vdo_fe_lut = idk_242_5000_ipt_ipt_v_lut;
			dovi_ut_config_adl(DV_ADL_V_SUB, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5000_ipt_ipt_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5000_ipt_ipt_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[2]) {
			p_gfx_fe_lut = idk_242_5000_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_FHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5000_ipt_ipt_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5000_ipt_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[3]) {
			p_gfx_fe_lut = idk_242_5000_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_UHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5000_ipt_ipt_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5000_ipt_ipt_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 132;
		}

		p_vdo_be_lut = idk_242_5000_ipt_ipt_md_lut;
		dovi_ut_config_adl(DV_SCRM, 1, p_vdo_be_lut);
	}

	if (out_fmt == DOVI_SDR) {
		if (layer_en[0]) {
			p_vdo_fe_lut = idk_242_5000_ipt_sdr_v_lut;
			dovi_ut_config_adl(DV_ADL_V_MAIN, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5000_ipt_sdr_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5000_ipt_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[1]) {
			p_vdo_fe_lut = idk_242_5000_ipt_sdr_v_lut;
			dovi_ut_config_adl(DV_ADL_V_SUB, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5000_ipt_sdr_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5000_ipt_sdr_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[2]) {
			p_gfx_fe_lut = idk_242_5000_sdr_sdr_g_lut;
			dovi_ut_config_adl(DV_ADL_G_FHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5000_ipt_sdr_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5000_ipt_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[3]) {
			p_gfx_fe_lut = idk_242_5000_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_UHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5000_ipt_sdr_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5000_ipt_sdr_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 124;
		}

	}

	if (d_ml_tbl_dpt) {
		memset(dsys_ml_buf, 0, ML_DSYS_UT_BUF_SIZE);
		if (p_dispsys_reg != NULL)
			memcpy((void *)dsys_ml_buf, (void *)p_dispsys_reg,
			d_ml_tbl_dpt * 8);
		/* todo flush cache */
		dma_map_single(dovi_dev, dsys_ml_buf,
		d_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
		dma_unmap_single(dovi_dev, dsys_ml_buf_pa,
		d_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
	}

	if (m_ml_tbl_dpt) {
		memset(msys_ml_buf, 0, ML_MSYS_UT_BUF_SIZE);
		if (p_mmsys_reg != NULL)
			memcpy((void *)msys_ml_buf, (void *)p_mmsys_reg,
			m_ml_tbl_dpt * 8);
		/* todo flush cache */
		dma_map_single(dovi_dev, msys_ml_buf,
		m_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
		dma_unmap_single(dovi_dev, msys_ml_buf_pa,
		m_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
	}
}


void dovi_ut_2160p_case(void)
{
	uint32_t *p_dispsys_reg = NULL;
	uint32_t *p_mmsys_reg = NULL;

	uint32_t *p_vdo_fe_lut;
	uint32_t *p_gfx_fe_lut;
	uint32_t *p_vdo_be_lut;
	bool layer_en[4] = {0};
	enum dovi_format out_fmt = 0;
	bool ll_on = 0;
	enum dovi_format in_fmt[4] = { 0 };

	out_fmt = dv_ut_info.out_format;
	ll_on = dv_ut_info.use_ll;
	layer_en[0] = dv_ut_info.dv_layer[0].on;
	in_fmt[0] = dv_ut_info.dv_layer[0].in_format;
	layer_en[1] = dv_ut_info.dv_layer[1].on;
	in_fmt[1] = dv_ut_info.dv_layer[1].in_format;
	layer_en[2] = dv_ut_info.dv_layer[2].on;
	in_fmt[2] = dv_ut_info.dv_layer[2].in_format;
	layer_en[3] = dv_ut_info.dv_layer[3].on;
	in_fmt[3] = dv_ut_info.dv_layer[3].in_format;

	if (out_fmt == DOVI_STD) {
		if (layer_en[0]) {
			p_vdo_fe_lut = idk_242_5003_ipt_ipt_v_lut;
			dovi_ut_config_adl(DV_ADL_V_MAIN, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5003_ipt_ipt_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5003_ipt_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[1]) {
			p_vdo_fe_lut = idk_242_5003_ipt_ipt_v_lut;
			dovi_ut_config_adl(DV_ADL_V_SUB, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5003_ipt_ipt_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5003_ipt_ipt_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[2]) {
			p_gfx_fe_lut = idk_242_5003_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_FHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5003_ipt_ipt_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5003_ipt_ipt_msys_ml_tbl;
			m_ml_tbl_dpt = 132;
		}
		if (layer_en[3]) {
			p_gfx_fe_lut = idk_242_5003_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_UHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5003_ipt_ipt_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5003_ipt_ipt_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 132;
		}

		p_vdo_be_lut = idk_242_5003_ipt_ipt_md_lut;
		dovi_ut_config_adl(DV_SCRM, 1, p_vdo_be_lut);
	}

	if (out_fmt == DOVI_SDR) {
		if (layer_en[0]) {
			p_vdo_fe_lut = idk_242_5003_ipt_sdr_v_lut;
			dovi_ut_config_adl(DV_ADL_V_MAIN, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5003_ipt_sdr_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5003_ipt_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[1]) {
			p_vdo_fe_lut = idk_242_5003_ipt_sdr_v_lut;
			dovi_ut_config_adl(DV_ADL_V_SUB, 1, p_vdo_fe_lut);
			p_dispsys_reg = idk_242_5003_ipt_sdr_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5003_ipt_sdr_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[2]) {
			p_gfx_fe_lut = idk_242_5003_sdr_sdr_g_lut;
			dovi_ut_config_adl(DV_ADL_G_FHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5003_ipt_sdr_dsys_ml_tbl;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5003_ipt_sdr_msys_ml_tbl;
			m_ml_tbl_dpt = 124;
		}
		if (layer_en[3]) {
			p_gfx_fe_lut = idk_242_5003_sdr_ipt_g_lut;
			dovi_ut_config_adl(DV_ADL_G_UHD, 1, p_gfx_fe_lut);
			p_dispsys_reg = idk_242_5003_ipt_sdr_dsys_ml_tbl_sub;
			d_ml_tbl_dpt = 384;
			p_mmsys_reg = idk_242_5003_ipt_sdr_msys_ml_tbl_uhd;
			m_ml_tbl_dpt = 124;
		}

	}

	if (d_ml_tbl_dpt) {
		memset(dsys_ml_buf, 0, ML_DSYS_UT_BUF_SIZE);
		if (p_dispsys_reg != NULL)
			memcpy((void *)dsys_ml_buf, (void *)p_dispsys_reg,
			d_ml_tbl_dpt * 8);
		/* todo flush cache */
		dma_map_single(dovi_dev, dsys_ml_buf,
		d_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
		dma_unmap_single(dovi_dev, dsys_ml_buf_pa,
		d_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
	}

	if (m_ml_tbl_dpt) {
		memset(msys_ml_buf, 0, ML_MSYS_UT_BUF_SIZE);
		if (p_mmsys_reg != NULL)
			memcpy((void *)msys_ml_buf, (void *)p_mmsys_reg,
			m_ml_tbl_dpt * 8);
		/* todo flush cache */
		dma_map_single(dovi_dev, msys_ml_buf,
		m_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
		dma_unmap_single(dovi_dev, msys_ml_buf_pa,
		m_ml_tbl_dpt * 8,
		DMA_TO_DEVICE);
	}
}

int disp_dovi_default_path(uint32_t option)
{
	uint32_t case_id = 0;

	if (option)
		dovi_ut_enable = true;
	else
		dovi_ut_enable = false;

	dovi_printf("option %d dovi_enable %d\n", option,
		dovi_ut_enable);

	dovi_printf("dovi ut info out(%d %d %d %d)\n",
		dv_ut_info.out_format,
		dv_ut_info.out_res, dv_ut_info.ll_rgb, dv_ut_info.use_ll);

	dovi_printf("layer0 info (%d %d %d %d %d)\n",
		dv_ut_info.dv_layer[0].on,
		dv_ut_info.dv_layer[0].case_id,
		dv_ut_info.dv_layer[0].in_format,
		dv_ut_info.dv_layer[0].width,
		dv_ut_info.dv_layer[0].heigh);
	dovi_printf("layer0 info (%d %d %d %d %d)\n",
		dv_ut_info.dv_layer[1].on,
		dv_ut_info.dv_layer[1].case_id,
		dv_ut_info.dv_layer[1].in_format,
		dv_ut_info.dv_layer[1].width,
		dv_ut_info.dv_layer[1].heigh);
	dovi_printf("layer0 info (%d %d %d %d %d)\n",
		dv_ut_info.dv_layer[2].on,
		dv_ut_info.dv_layer[2].case_id,
		dv_ut_info.dv_layer[2].in_format,
		dv_ut_info.dv_layer[2].width,
		dv_ut_info.dv_layer[2].heigh);
	dovi_printf("layer0 info (%d %d %d %d %d)\n",
		dv_ut_info.dv_layer[3].on,
		dv_ut_info.dv_layer[3].case_id,
		dv_ut_info.dv_layer[3].in_format,
		dv_ut_info.dv_layer[3].width,
		dv_ut_info.dv_layer[3].heigh);

	if (dovi_ut_enable) {
		dovi_info("enable dobly clock:\n");
		if (dv_ut_info.dv_layer[0].on) {
			disp_clock_enable(DISP_CLK_M_HDR_VDO_FE,
				dovi_ut_enable);
			dovi_vdo_fe_hal_set_enable(0, dovi_ut_enable);
			case_id = dv_ut_info.dv_layer[0].case_id;
		}
		if (dv_ut_info.dv_layer[1].on) {
			disp_clock_enable(DISP_CLK_S_HDR_VDO_FE,
				dovi_ut_enable);
			dovi_vdo_fe_hal_set_enable(1, dovi_ut_enable);
			case_id = dv_ut_info.dv_layer[1].case_id;
		}
		if (dv_ut_info.dv_layer[2].on) {
			disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE,
				dovi_ut_enable);
			dovi_gfx_fe_hal_set_enable(0, dovi_ut_enable);
			case_id = dv_ut_info.dv_layer[2].case_id;
		}
		if (dv_ut_info.dv_layer[3].on) {
			disp_clock_enable(DISP_CLK_UHD_HDR_GFX_FE,
				dovi_ut_enable);
			dovi_gfx_fe_hal_set_enable(1, dovi_ut_enable);
			case_id = dv_ut_info.dv_layer[3].case_id;
		}

		disp_clock_enable(DISP_CLK_HDR_VDO_BE,
			dovi_ut_enable);

		dovi_vdo_fe_init(dovi_reg_base);
		dovi_gfx_fe_init(dovi_reg_base);
		dovi_be_init(dovi_reg_base);
		dovi_be_hal_set_enable(dovi_ut_enable);

		switch (case_id) {
		case 5005:
			dovi_ut_480p_case();
			break;
		case 5004:
			dovi_ut_720p_case();
			break;
		case 5000:
			dovi_ut_1080p_case();
			break;
		case 5003:
			dovi_ut_2160p_case();
			break;
		}
	}
	disp_path_set_hw_path(DISP_PATH_M_HDR_VDO_FE, 1);
	disp_path_set_hw_path(DISP_PATH_DISP_HDR_VDO_BE, 1);
	if (!dovi_ut_enable) {
		disp_clock_enable(DISP_CLK_HDR_VDO_BE, dovi_ut_enable);
		disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE, dovi_ut_enable);
		disp_clock_enable(DISP_CLK_UHD_HDR_GFX_FE, dovi_ut_enable);
		disp_clock_enable(DISP_CLK_M_HDR_VDO_FE, dovi_ut_enable);
		disp_clock_enable(DISP_CLK_S_HDR_VDO_FE, dovi_ut_enable);
		dovi_ut_external_bypass(0);
		dovi_ut_external_bypass(1);
		dovi_ut_external_bypass(2);
		dovi_ut_external_bypass(3);

		dovi_info("disable dobly clock\n");
	}

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
	if (option == 3 || option == 4)
		vDoviHdrEnable(true);
	else
		vDoviHdrEnable(false);
#endif

	return DOVI_STATUS_OK;
}

static int dovi_routine(void *data)
{
	while (1) {
		wait_event_interruptible(disp_dovi_wq, disp_dovi_wakeup_thread);
		disp_dovi_wakeup_thread = 0;

		mutex_lock(&disp_dovi_main_mutex);

		if (!(dovi_option & DOVI_PATH_TEST_MASK)) {
			dovi_default("dv_op 0x%X is invalid\n",
				dovi_option);
			mutex_unlock(&disp_dovi_main_mutex);
			continue;
		}

		dovi_option &= ~DOVI_PATH_TEST_MASK;

		disp_dovi_default_path(dovi_option);
		ml_update = 1;

		mutex_unlock(&disp_dovi_main_mutex);
	}

	return DOVI_STATUS_OK;
}

int disp_dovi_default_path_init(uint32_t option)
{
	if (!disp_dovi_thread) {
		mutex_init(&disp_dovi_main_mutex);
		dovi_default("mutex init\n");

		init_waitqueue_head(&disp_dovi_wq);
		disp_dovi_wakeup_thread = 0;
		dovi_default("wq init\n");

		disp_dovi_thread = kthread_create(dovi_routine, NULL,
			"disp_dovi_thread");
		wake_up_process(disp_dovi_thread);
		dovi_default("thread %p init\n", disp_dovi_thread);
	}

	mutex_lock(&disp_dovi_main_mutex);

	dovi_option = option | DOVI_PATH_TEST_MASK;

	mutex_unlock(&disp_dovi_main_mutex);

	return DOVI_STATUS_OK;
}

void dovi_ut_alloc_adl_ml_table_mem(uint32_t en)
{
	uint32_t graphic_size = 4096 * 2160 * 4;
	uint32_t video_size = 4096 * 2176 * 5 / 4;
	uint32_t vin_size = 4096 * 2160;
	uint32_t graphic_header_size = 48;

	if (en && !ml_adl_buf_alloced) {
		ut_y_addr_va[0] =
		    dma_alloc_coherent(dovi_dev, video_size,
		    &ut_y_addr_pa[0], GFP_KERNEL);

		ut_c_addr_va[0] =
		    dma_alloc_coherent(dovi_dev, video_size,
		    &ut_c_addr_pa[0], GFP_KERNEL);

		ut_y_addr_va[1] =
		    dma_alloc_coherent(dovi_dev, video_size,
		    &ut_y_addr_pa[1], GFP_KERNEL);

		ut_c_addr_va[1] =
		    dma_alloc_coherent(dovi_dev, video_size,
		    &ut_c_addr_pa[1], GFP_KERNEL);

		dovi_default("vdo_y mem 0x%p 0x%p 0x%p 0x%p\n",
			ut_y_addr_va[0], (void *)ut_y_addr_pa[0],
			ut_y_addr_va[1], (void *)ut_y_addr_pa[1]);
		dovi_default("vdo_c mem 0x%p 0x%p 0x%p 0x%p\n",
			ut_c_addr_va[0], (void *)ut_c_addr_pa[0],
			ut_c_addr_va[1], (void *)ut_c_addr_pa[1]);

		ut_graphic_addr_va[0] =
		    dma_alloc_coherent(dovi_dev, graphic_size,
		    &ut_graphic_addr_pa[0], GFP_KERNEL);

		ut_graphic_addr_va[1] =
		    dma_alloc_coherent(dovi_dev, graphic_size,
		    &ut_graphic_addr_pa[1], GFP_KERNEL);

		ut_graphic_header_va[0] =
		    dma_alloc_coherent(dovi_dev, graphic_header_size,
		    &ut_graphic_header_pa[0], GFP_KERNEL);

		ut_graphic_header_va[1] =
		    dma_alloc_coherent(dovi_dev, graphic_header_size,
		    &ut_graphic_header_pa[1], GFP_KERNEL);

		dovi_default("osd mem 0x%p 0x%p 0x%p 0x%p\n",
			ut_graphic_addr_va[0], (void *)ut_graphic_addr_pa[0],
			ut_graphic_addr_va[1], (void *)ut_graphic_addr_pa[1]);

		dovi_default("osdhead mem 0x%p 0x%p 0x%p 0x%p\n",
			ut_graphic_header_va[0],
			(void *)ut_graphic_header_pa[0],
			ut_graphic_header_va[1],
			(void *)ut_graphic_header_pa[1]);

		ut_vin_y_addr_va =
		    dma_alloc_coherent(dovi_dev, vin_size,
		    &ut_vin_y_addr_pa, GFP_KERNEL);
		ut_vin_c_addr_va =
		    dma_alloc_coherent(dovi_dev, vin_size,
		    &ut_vin_c_addr_pa, GFP_KERNEL);

		dovi_default("vin mem 0x%p 0x%p 0x%p 0x%p\n",
			ut_vin_y_addr_va, (void *)ut_vin_y_addr_pa,
			ut_vin_c_addr_va, (void *)ut_vin_c_addr_pa);

		dsys_ml_buf = dma_alloc_coherent(dovi_dev,
			ML_DSYS_UT_BUF_SIZE, &dsys_ml_buf_pa, GFP_KERNEL);

		msys_ml_buf = dma_alloc_coherent(dovi_dev,
			ML_MSYS_UT_BUF_SIZE, &msys_ml_buf_pa, GFP_KERNEL);

		mfe_adl_buf = dma_alloc_coherent(dovi_dev,
			ADL_UT_LUT_BUF_SIZE, &mfe_adl_buf_pa, GFP_KERNEL);

		sfe_adl_buf = dma_alloc_coherent(dovi_dev,
			ADL_UT_LUT_BUF_SIZE, &sfe_adl_buf_pa, GFP_KERNEL);

		uhd_fe_adl_buf = dma_alloc_coherent(dovi_dev,
			ADL_UT_LUT_BUF_SIZE, &uhd_fe_adl_buf_pa, GFP_KERNEL);

		fhd_fe_adl_buf = dma_alloc_coherent(dovi_dev,
			ADL_UT_LUT_BUF_SIZE, &fhd_fe_adl_buf_pa, GFP_KERNEL);

		be_fe_adl_buf = dma_alloc_coherent(dovi_dev,
			ADL_UT_SCM_BUF_SIZE, &be_fe_adl_buf_pa, GFP_KERNEL);
		ml_adl_buf_alloced = 1;
	} else if (!en && ml_adl_buf_alloced) {
		dma_free_coherent(dovi_dev,
			video_size, ut_y_addr_va[0], ut_y_addr_pa[0]);
		dma_free_coherent(dovi_dev,
			video_size, ut_y_addr_va[1], ut_y_addr_pa[1]);
		dma_free_coherent(dovi_dev,
			video_size, ut_c_addr_va[0], ut_c_addr_pa[0]);
		dma_free_coherent(dovi_dev,
			video_size, ut_c_addr_va[1], ut_c_addr_pa[1]);

		dma_free_coherent(dovi_dev, graphic_size,
			ut_graphic_addr_va[0], ut_graphic_addr_pa[0]);
		dma_free_coherent(dovi_dev, graphic_size,
			ut_graphic_addr_va[1], ut_graphic_addr_pa[1]);
		dma_free_coherent(dovi_dev, graphic_header_size,
			ut_graphic_header_va[0], ut_graphic_header_pa[0]);
		dma_free_coherent(dovi_dev, graphic_header_size,
			ut_graphic_header_va[1], ut_graphic_header_pa[1]);

		dma_free_coherent(dovi_dev, vin_size,
			ut_vin_y_addr_va, ut_vin_y_addr_pa);
		dma_free_coherent(dovi_dev, vin_size,
			ut_vin_c_addr_va, ut_vin_c_addr_pa);

		dma_free_coherent(dovi_dev,
			ML_DSYS_UT_BUF_SIZE, dsys_ml_buf, dsys_ml_buf_pa);

		dma_free_coherent(dovi_dev,
			ML_MSYS_UT_BUF_SIZE, msys_ml_buf, msys_ml_buf_pa);

		dma_free_coherent(dovi_dev,
			ADL_UT_LUT_BUF_SIZE, mfe_adl_buf, mfe_adl_buf_pa);

		dma_free_coherent(dovi_dev,
			ADL_UT_LUT_BUF_SIZE, sfe_adl_buf, sfe_adl_buf_pa);

		dma_free_coherent(dovi_dev,
			ADL_UT_LUT_BUF_SIZE, fhd_fe_adl_buf, fhd_fe_adl_buf_pa);

		dma_free_coherent(dovi_dev,
			ADL_UT_LUT_BUF_SIZE, uhd_fe_adl_buf, uhd_fe_adl_buf_pa);

		dma_free_coherent(dovi_dev,
			ADL_UT_SCM_BUF_SIZE, be_fe_adl_buf, be_fe_adl_buf_pa);


		ml_adl_buf_alloced = 0;
	}

}

void dovi_idk_dump_vdo_bypass(uint32_t id1)
{
	switch (id1) {
	case DV_MAIN_FE:
		UT_WriteREGMsk(d_top_reg_base + 0x2c, 0x2, 0xFF);
		break;
	case DV_SUB_FE:
		UT_WriteREGMsk(d_top_reg_base + 0x18, 0x2, 0xFF);
		break;
	case DV_FHD_FE:
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x1, 0x3);
		break;
	case DV_UHD_FE:
		UT_WriteREGMsk(mmsys_cfg_reg_base + 0x700, 0x1, 0x3);
		break;
	case DV_BE:
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x1 << 4, 0x10);
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x0 << 8, 0x100);
		break;
	default:
		break;
	}
	UT_WriteREGMsk(m_top_reg_base + 0x20, 0x1 << 4, 0x10);
	UT_WriteREGMsk(m_top_reg_base + 0x20, 0x0 << 8, 0x100);
}

void dovi_ut_external_bypass(uint32_t id1)
{
	switch (id1) {
	case DV_MAIN_FE:
		UT_WriteREGMsk(d_top_reg_base + 0x2c, 0x1 << 3, 0x8);
		UT_WriteREGMsk(d_top_reg_base + 0x2c, 0x1 << 4, 0x30);
		break;
	case DV_SUB_FE:
		UT_WriteREGMsk(d_top_reg_base + 0x18, 0x1 << 3, 0x8);
		UT_WriteREGMsk(d_top_reg_base + 0x18, 0x1 << 4, 0x30);
		break;
	case DV_FHD_FE:
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x1, 0x3);
		break;
	case DV_UHD_FE:
		UT_WriteREGMsk(mmsys_cfg_reg_base + 0x700, 0x1, 0x3);
		break;
	case DV_BE:
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x1 << 4, 0x10);
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x0 << 8, 0x100);
		break;
	default:
		break;
	}
	UT_WriteREGMsk(m_top_reg_base + 0x20, 0x1 << 4, 0x10);
	UT_WriteREGMsk(m_top_reg_base + 0x20, 0x0 << 8, 0x100);
}

void dovi_ut_internal_bypass(uint32_t id1)
{
	switch (id1) {
	case DV_MAIN_FE:
		//extern path
		UT_WriteREGMsk(d_top_reg_base + 0x2c, 0x0 << 3, 0x8);
		UT_WriteREGMsk(d_top_reg_base + 0x2c, 0x2 << 4, 0x30);
		//inter bypass
		UT_WriteREG(mhdr_fe_reg_base + 0x804, 0xfd);
		UT_WriteREG(mhdr_fe_reg_base + 0x9ec, 0x80);
		UT_WriteREG(mhdr_fe_reg_base + 0x618, 0x0);
		UT_WriteREG(mhdr_fe_reg_base + 0x61c, 0x2);
		UT_WriteREG(mhdr_fe_reg_base + 0x634, 0x8000);
		break;
	case DV_SUB_FE:
		UT_WriteREGMsk(d_top_reg_base + 0x18, 0x0 << 3, 0x8);
		UT_WriteREGMsk(d_top_reg_base + 0x18, 0x2 << 4, 0x30);
		UT_WriteREG(shdr_fe_reg_base + 0x804, 0xfd);
		UT_WriteREG(shdr_fe_reg_base + 0x9ec, 0x80);
		UT_WriteREG(shdr_fe_reg_base + 0x618, 0x0);
		UT_WriteREG(shdr_fe_reg_base + 0x61c, 0x2);
		UT_WriteREG(shdr_fe_reg_base + 0x634, 0x8000);
		break;
	case DV_FHD_FE:
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x2, 0x3);
		UT_WriteREG(fhdr_fe_reg_base + 0x100, 0x8001);
		UT_WriteREG(fhdr_fe_reg_base + 0x204, 0xfd);
		UT_WriteREG(fhdr_fe_reg_base + 0x3ec, 0x80);
		UT_WriteREG(fhdr_fe_reg_base + 0x21c, 0x20);
		break;
	case DV_UHD_FE:
		UT_WriteREGMsk(0x14000000 + 0x700, 0x2, 0x3);
		UT_WriteREG(uhdr_fe_reg_base + 0x100, 0x8001);
		UT_WriteREG(uhdr_fe_reg_base + 0x204, 0xfd);
		UT_WriteREG(uhdr_fe_reg_base + 0x3ec, 0x80);
		UT_WriteREG(uhdr_fe_reg_base + 0x21c, 0x20);
		break;
	case DV_BE:
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x0 << 4, 0x10);
		UT_WriteREGMsk(m_top_reg_base + 0x20, 0x1 << 8, 0x100);
		UT_WriteREG(m_top_reg_base + 0x4, 0x7e);
		UT_WriteREG(m_top_reg_base + 0x120, 0x0);
		UT_WriteREG(m_top_reg_base + 0x1c8, 0x1);
		break;
	default:
		break;
	}
	UT_WriteREGMsk(m_top_reg_base + 0x20, 0x0 << 4, 0x10);
	UT_WriteREGMsk(m_top_reg_base + 0x20, 0x1 << 8, 0x100);
	//set sof
	UT_WriteREG(fmt_reg_base + 0x130, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x134, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x168, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x16c, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x200, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x204, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x218, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x21c, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x228, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x22c, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x238, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x23c, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x240, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x244, 0x00010033);
}


void disp_fmt_set_timing(uint32_t timing)
{
	switch (timing) {
	case DOVI_480P://480p
		UT_WriteREG(fmt_reg_base + 0xd4, 0x835a020d);
		UT_WriteREG(fmt_reg_base + 0x94, 0x14148220);
		break;
	case DOVI_720P://720p
		UT_WriteREG(fmt_reg_base + 0xd4, 0x867202ee);
		UT_WriteREG(fmt_reg_base + 0x94, 0x0000E220);
		break;
	case DOVI_1080P://1080p
		UT_WriteREG(fmt_reg_base + 0xd4, 0x88980465);
		UT_WriteREG(fmt_reg_base + 0x94, 0x0000DF20);
		break;
	case DOVI_2160P://2160p
		UT_WriteREG(fmt_reg_base + 0xd4, 0x913008CA);
		UT_WriteREG(fmt_reg_base + 0x94, 0x0000C305);
		break;
	case DOVI_2161P://2161p
		UT_WriteREG(fmt_reg_base + 0xd4, 0x913008CA);
		UT_WriteREG(fmt_reg_base + 0x94, 0x0000C305);
		break;
	default:
		UT_WriteREG(fmt_reg_base + 0xd4, 0x88980465);
		UT_WriteREG(fmt_reg_base + 0x94, 0x0000C305);
		break;
	}
	UT_WriteREG(fmt_reg_base + 0xac, 0x00000403);
	UT_WriteREG(fmt_reg_base + 0xac, 0x00000003);
}

void disp_set_fe_fifo(uint32_t timing)
{
	uint32_t width = 0;
	uint32_t heigh = 0;

	switch (timing) {
	case DOVI_480P:
		width = 720;
		heigh = 480;
		break;
	case DOVI_720P:
		width = 1280;
		heigh = 720;
		break;
	case DOVI_1080P:
		width = 1920;
		heigh = 1080;
		break;
	case DOVI_2160P:
		width = 3840;
		heigh = 2160;
		break;
	case DOVI_2161P:
		width = 4096;
		heigh = 2160;
		break;
	default:
		width = 1920;
		heigh = 1080;
		break;
	}
	UT_WriteREG(m_fifo_reg_base + 0x4, 0xF);
	UT_WriteREG(m_fifo_reg_base + 0x5c,
		(width & 0xffff) | ((heigh & 0xfff) << 16));
	//set sof
	UT_WriteREG(fmt_reg_base + 0x110, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x114, 0x00010003);

	UT_WriteREG(s_fifo_reg_base + 0x4, 0xF);
	UT_WriteREG(s_fifo_reg_base + 0x5c,
		(width & 0xffff) | ((heigh & 0xfff) << 16));
	//set sof
	UT_WriteREG(fmt_reg_base + 0x148, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x14c, 0x00010003);

	UT_WriteREG(fhd_fifo_reg_base + 0x4, 0x0500000F);
	UT_WriteREG(fhd_fifo_reg_base + 0x5c,
		(width & 0xffff) | ((heigh & 0xfff) << 16));
	//set sof
	UT_WriteREG(fmt_reg_base + 0x210, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x214, 0x00010003);

	UT_WriteREG(uhd_fifo_reg_base + 0x4, 0x0500000F);
	UT_WriteREG(uhd_fifo_reg_base + 0x5c,
		(width & 0xffff) | ((heigh & 0xfff) << 16));
	//set sof
	UT_WriteREG(fmt_reg_base + 0x178, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x17C, 0x00010003);

	//set hdrfe sof
	UT_WriteREG(fmt_reg_base + 0x130, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x134, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x168, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x16c, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x200, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x204, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x218, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x21c, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x228, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x22c, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x238, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x23c, 0x00010033);
	UT_WriteREG(fmt_reg_base + 0x240, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x244, 0x00010033);

}

void disp_set_be_fifo(uint32_t timing)
{
	switch (timing) {
	case DOVI_480P:
		UT_WriteREG(be_fifo_reg_base + 0x0, 0x020D035A);
		UT_WriteREG(be_fifo_reg_base + 0x4, 0x01E002D0);
		UT_WriteREG(be_fifo_reg_base + 0x8, 0x00010001);
		UT_WriteREG(be_fifo_reg_base + 0xc, 0x002D007B);
		UT_WriteREG(be_fifo_reg_base + 0x10, 0x0000063E);
		UT_WriteREG(be_fifo_reg_base + 0x5c, 0x02D00000);
		UT_WriteREG(be_fifo_reg_base + 0x60, 0x01E00000);
		UT_WriteREG(be_fifo_reg_base + 0x14, 0x00003430);
		break;
	case DOVI_720P:
		UT_WriteREG(be_fifo_reg_base + 0x0, 0x02EE0672);
		UT_WriteREG(be_fifo_reg_base + 0x4, 0x02D00500);
		UT_WriteREG(be_fifo_reg_base + 0x8, 0x00010001);
		UT_WriteREG(be_fifo_reg_base + 0xc, 0x001D0125);
		UT_WriteREG(be_fifo_reg_base + 0x10, 0x00000528);
		UT_WriteREG(be_fifo_reg_base + 0x5c, 0x05000000);
		UT_WriteREG(be_fifo_reg_base + 0x60, 0x02D00000);
		UT_WriteREG(be_fifo_reg_base + 0x14, 0x000007F0);
		break;
	case DOVI_1080P:
		UT_WriteREG(be_fifo_reg_base + 0x0, 0x04650898);
		UT_WriteREG(be_fifo_reg_base + 0x4, 0x04380780);
		UT_WriteREG(be_fifo_reg_base + 0x8, 0x00010001);
		UT_WriteREG(be_fifo_reg_base + 0xc, 0x002D00E1);
		UT_WriteREG(be_fifo_reg_base + 0x10, 0x0000052C);
		UT_WriteREG(be_fifo_reg_base + 0x5c, 0x07800000);
		UT_WriteREG(be_fifo_reg_base + 0x60, 0x04380000);
		UT_WriteREG(be_fifo_reg_base + 0x14, 0x000037F0);
		break;
	case DOVI_2160P:
		UT_WriteREG(be_fifo_reg_base + 0x0, 0x08CA1130);
		UT_WriteREG(be_fifo_reg_base + 0x4, 0x08700F00);
		UT_WriteREG(be_fifo_reg_base + 0x8, 0x00010001);
		UT_WriteREG(be_fifo_reg_base + 0xc, 0x005401A1);
		UT_WriteREG(be_fifo_reg_base + 0x10, 0x00000A58);
		UT_WriteREG(be_fifo_reg_base + 0x5c, 0x0F000000);
		UT_WriteREG(be_fifo_reg_base + 0x60, 0x08700000);
		UT_WriteREG(be_fifo_reg_base + 0x14, 0x00000430);
		break;
	case DOVI_2161P:
		UT_WriteREG(be_fifo_reg_base + 0x0, 0x08CA1130);
		UT_WriteREG(be_fifo_reg_base + 0x4, 0x08701000);
		UT_WriteREG(be_fifo_reg_base + 0x8, 0x00010001);
		UT_WriteREG(be_fifo_reg_base + 0xc, 0x005401A1);
		UT_WriteREG(be_fifo_reg_base + 0x10, 0x00000A58);
		UT_WriteREG(be_fifo_reg_base + 0x5c, 0x10000000);
		UT_WriteREG(be_fifo_reg_base + 0x60, 0x08700000);
		UT_WriteREG(be_fifo_reg_base + 0x14, 0x00000430);
		break;
	default:
		break;
	}
	//set sof
	UT_WriteREG(fmt_reg_base + 0x230, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x234, 0x00010003);

}

void disp_set_disp_mix_and_path(uint32_t timing)
{
	uint32_t width = 0;
	uint32_t heigh = 0;
	uint32_t in_width = 0;
	uint32_t in_heigh = 0;
	uint32_t plane_en = 0;
	uint32_t small_region = 0;


	if (timing == DOVI_480P) {
		width = 720;
		heigh = 480;
	} else if (timing == DOVI_720P) {
		width = 1280;
		heigh = 720;
	} else if (timing == DOVI_1080P) {
		width = 1920;
		heigh = 1080;
	} else if (timing == DOVI_2160P) {
		width = 3840;
		heigh = 2160;
	} else if (timing == DOVI_2161P) {
		width = 4096;
		heigh = 2160;
	}

	//set video delay alpha 256
	UT_WriteREG(m_top_reg_base + 0x0, 0x01000100);
	//clk on
	UT_WriteREG(disp_mix_reg_base + 0x120, 0xffffffff);
	UT_WriteREG(disp_mix_reg_base + 0x124, 0xffffffff);
	//mix on
	UT_WriteREG(disp_mix_reg_base + 0xc, 0x1);
	UT_WriteREG(disp_mix_reg_base + 0x10, 0x1);
	//mix out region
	UT_WriteREG(disp_mix_reg_base + 0x18,
		((heigh & 0x1fff) << 16) | (width & 0x3fff));

	if (dv_ut_info.dv_layer[0].on) {
		in_width = dv_ut_info.dv_layer[0].width;
		in_heigh = dv_ut_info.dv_layer[0].heigh;
		UT_WriteREG(disp_mix_reg_base + 0x28, 0x000021ff);
		UT_WriteREG(disp_mix_reg_base + 0x30,
			((in_heigh & 0x1fff) << 16) | (in_width & 0x3fff));
		plane_en |= (1 << 0);
		if ((in_width < width) || (in_heigh < heigh))
			small_region |= (1 << 12);
	}

	if (dv_ut_info.dv_layer[1].on) {
		in_width = dv_ut_info.dv_layer[1].width;
		in_heigh = dv_ut_info.dv_layer[1].heigh;
		UT_WriteREG(disp_mix_reg_base + 0x40, 0x000021ff);
		UT_WriteREG(disp_mix_reg_base + 0x48,
			((in_heigh & 0x1fff) << 16) | (in_width & 0x3fff));
		plane_en |= (1 << 1);
		if ((in_width < width) || (in_heigh < heigh))
			small_region |= (1 << 13);
	}

	if (dv_ut_info.dv_layer[2].on) {
		in_width = dv_ut_info.dv_layer[2].width;
		in_heigh = dv_ut_info.dv_layer[2].heigh;
		UT_WriteREG(disp_mix_reg_base + 0x58, 0x000021ff);
		UT_WriteREG(disp_mix_reg_base + 0x60,
			((in_heigh & 0x1fff) << 16) | (in_width & 0x3fff));
		plane_en |= (1 << 2);
		if ((in_width < width) || (in_heigh < heigh))
			small_region |= (1 << 14);
	}

	if (dv_ut_info.dv_layer[3].on) {
		in_width = dv_ut_info.dv_layer[3].width;
		in_heigh = dv_ut_info.dv_layer[3].heigh;
		UT_WriteREG(disp_mix_reg_base + 0x70, 0x000021ff);
		UT_WriteREG(disp_mix_reg_base + 0x78,
			((in_heigh & 0x1fff) << 16) | (in_width & 0x3fff));
		plane_en |= (1 << 3);
		if ((in_width < width) || (in_heigh < heigh))
			small_region |= (1 << 15);
	}

	plane_en |= 0x0fa50000;
	small_region |= 0x888;

	UT_WriteREG(disp_mix_reg_base + 0x24, plane_en);
	UT_WriteREG(disp_mix_reg_base + 0x1c, small_region);

	//sof
	UT_WriteREG(fmt_reg_base + 0x220, 0x00010002);
	UT_WriteREG(fmt_reg_base + 0x224, 0x0001000b);

	//set path hdr internal bypass
	UT_WriteREG(d_top_reg_base + 0x2c, 0x25);
	UT_WriteREG(d_top_reg_base + 0x18, 0x25);
	UT_WriteREG(d_top_reg_base + 0x14, 0x0);
	UT_WriteREG(mmsys_cfg_reg_base + 0x700, 0x2);
	//set vin input point befifo output
	UT_WriteREG(mmsys_cfg_reg_base + 0x600, 0x00050000);
	UT_WriteREG(m_top_reg_base + 0x20, 0x102);
	UT_WriteREG(m_top_reg_base + 0x8, 0x40);
	UT_WriteREG(m_top_reg_base + 0x24, 0x0);

}

void disp_set_vdo_plane(uint32_t timing)
{

	//write common

	if (dv_ut_info.dv_layer[0].on) {
		UT_WriteREG(vdo3_reg_base + 0x6c, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x70, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x74, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x78, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x7c, 0x0);
		UT_WriteREG(vdo3_reg_base + 0xb4, 0x00d0e0f0);
		UT_WriteREG(vdo3_reg_base + 0xb8, 0x00302010);
		UT_WriteREG(vdo3_reg_base + 0xbc, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x1a0, 0x80000040);
		UT_WriteREG(vdo3_reg_base + 0xe4, 0x60);
		//scale
		UT_WriteREG(vdo3_reg_base + 0xb0, 0x10000001);
		//vdo
		UT_WriteREG(vdo3_reg_base + 0x430, 0x3);
		UT_WriteREG(vdo3_reg_base + 0x438, 0x6000000);
		UT_WriteREG(vdo3_reg_base + 0x458, 0xc0000);
		UT_WriteREG(vdo3_reg_base + 0x45c, 0xc0000);
		UT_WriteREG(vdo3_reg_base + 0x414, 0x800);
		UT_WriteREG(vdo3_reg_base + 0x434, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x464, 0x8000);
		UT_WriteREG(vdo3_reg_base + 0x470, 0x10000000);
		UT_WriteREG(vdo3_reg_base + 0x468, 0x20000814);
		UT_WriteREG(vdo3_reg_base + 0x418, 0x1e1e0000);
		UT_WriteREG(vdo3_reg_base + 0x478, 0x00010700);
		UT_WriteREG(vdo3_reg_base + 0x420, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x424, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x428, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x42c, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x450, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x454, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x46c, 0x05110000);
		UT_WriteREG(vdo3_reg_base + 0x4d0, 0x02001433);
		UT_WriteREG(vdo3_reg_base + 0xf8, 0x0);
		UT_WriteREG(vdo3_reg_base + 0x4c0, 0x02b32083);
		UT_WriteREG(vdo3_reg_base + 0x21c, 0x80000008);
		UT_WriteREG(vdo3_reg_base + 0xf4, 0x0);
		//enable ufo
		UT_WriteREG(vdo3_reg_base + 0x240, 0x3);
		UT_WriteREG(vdo3_reg_base + 0x200, ut_y_addr_pa[0]);
		UT_WriteREG(vdo3_reg_base + 0x204, ut_c_addr_pa[0]);
		//420-422 repeater
		UT_WriteREG(vdo3_reg_base + 0x41c, 0x83);
		UT_WriteREG(vdo3_reg_base + 0x424, 0x1);
		UT_WriteREG(vdo3_reg_base + 0x42c, 0x80);
		UT_WriteREG(vdo3_reg_base + 0x47c, 0x100);
		//active
		switch (timing) {
		case DOVI_480P:
			UT_WriteREG(vdo3_reg_base + 0xa0, 0x8070033f);
			UT_WriteREG(vdo3_reg_base + 0xa4, 0x002b020a);
			UT_WriteREG(vdo3_reg_base + 0xa8, 0x002b020a);
			UT_WriteREG(vdo3_reg_base + 0x9C, 0x000002d0);
			UT_WriteREG(vdo3_reg_base + 0x94, 0x00008220);
			UT_WriteREG(vdo3_reg_base + 0xb0, 0x01000001);
			UT_WriteREG(vdo3_reg_base + 0xcc, 0x00100000);
			UT_WriteREG(vdo3_reg_base + 0xe4, 0x00000004);
			UT_WriteREG(vdo3_reg_base + 0xe8, 0x04180356);
			UT_WriteREG(vdo3_reg_base + 0xec, 0x04160356);
			UT_WriteREG(vdo3_reg_base + 0xc8, 0x00000000);
			UT_WriteREG(vdo3_reg_base + 0xd0, 0x067202ee);
			UT_WriteREG(vdo3_reg_base + 0xd4, 0x067202ee);
			UT_WriteREG(vdo3_reg_base + 0xf4, 0x00000000);
			UT_WriteREG(vdo3_reg_base + 0xf8, 0x00100000);
			UT_WriteREG(vdo3_reg_base + 0x410, 0x01e0b45a);
			UT_WriteREG(vdo3_reg_base + 0x414, 0x80000800);
			UT_WriteREG(vdo3_reg_base + 0x464, 0x00008702);
			UT_WriteREG(vdo3_reg_base + 0x4cc, 0x00007601);
			UT_WriteREG(vdo3_reg_base + 0x208, 0x1e000180);
			UT_WriteREG(vdo3_reg_base + 0x20c, 0x00000030);
			UT_WriteREG(vdo3_reg_base + 0x218, 0x87100000);
			UT_WriteREG(vdo3_reg_base + 0x21c, 0x00000004);
			UT_WriteREG(vdo3_reg_base + 0x14c, 0x00212f00);
			//sof
			UT_WriteREG(fmt_reg_base + 0x100, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x104, 0x00020329);
			UT_WriteREG(fmt_reg_base + 0x108, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x10c, 0x00020329);
			break;
		case DOVI_720P:
			UT_WriteREG(vdo3_reg_base + 0xa0, 0x01050604);
			UT_WriteREG(vdo3_reg_base + 0xa4, 0x001a02e9);
			UT_WriteREG(vdo3_reg_base + 0xa8, 0x001a02e9);
			UT_WriteREG(vdo3_reg_base + 0x9C, 0x00000500);
			UT_WriteREG(vdo3_reg_base + 0x94, 0x0000e220);
			UT_WriteREG(vdo3_reg_base + 0xe8, 0x80020661);
			UT_WriteREG(vdo3_reg_base + 0xd0, 0x067202ee);
			UT_WriteREG(vdo3_reg_base + 0xd4, 0x067202ee);
			UT_WriteREG(vdo3_reg_base + 0x410, 0x02d0b4a0);
			UT_WriteREG(vdo3_reg_base + 0x4e0, 0x03000140);
			UT_WriteREG(vdo3_reg_base + 0x208, 0x2d000280);
			UT_WriteREG(vdo3_reg_base + 0x20c, 0x00000050);
			UT_WriteREG(vdo3_reg_base + 0x218, 0x8f100000);
			//sof
			UT_WriteREG(fmt_reg_base + 0x100, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x104, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x108, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x10c, 0x00020328);
			break;
		case DOVI_1080P:
			UT_WriteREG(vdo3_reg_base + 0xa0, 0x00c10840);
			UT_WriteREG(vdo3_reg_base + 0xa4, 0x002a0461);
			UT_WriteREG(vdo3_reg_base + 0xa8, 0x002a0461);
			UT_WriteREG(vdo3_reg_base + 0x9C, 0x00000780);
			UT_WriteREG(vdo3_reg_base + 0x94, 0x0000c220);
			UT_WriteREG(vdo3_reg_base + 0xe8, 0x08c80887);
			UT_WriteREG(vdo3_reg_base + 0xd0, 0x08980465);
			UT_WriteREG(vdo3_reg_base + 0xd4, 0x08980465);
			UT_WriteREG(vdo3_reg_base + 0x410, 0x0438e0f0);
			UT_WriteREG(vdo3_reg_base + 0x4e0, 0x010001e0);
			UT_WriteREG(vdo3_reg_base + 0x208, 0x438003c0);
			UT_WriteREG(vdo3_reg_base + 0x20c, 0x00000078);
			UT_WriteREG(vdo3_reg_base + 0x218, 0x8f100000);
			//sof
			UT_WriteREG(fmt_reg_base + 0x100, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x104, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x108, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x10c, 0x00020328);
			break;
		case DOVI_2160P:
			UT_WriteREG(vdo3_reg_base + 0xa0, 0x00D90FD8);
			UT_WriteREG(vdo3_reg_base + 0xa4, 0x005308C2);
			UT_WriteREG(vdo3_reg_base + 0xa8, 0x005308C2);
			UT_WriteREG(vdo3_reg_base + 0x9C, 0x00000f00);
			UT_WriteREG(vdo3_reg_base + 0x94, 0x0000c220);
			UT_WriteREG(vdo3_reg_base + 0xe8, 0x11921130);
			UT_WriteREG(vdo3_reg_base + 0xd0, 0x913008ca);
			UT_WriteREG(vdo3_reg_base + 0xd4, 0x913008ca);
			UT_WriteREG(vdo3_reg_base + 0x410, 0x1870c0e0);
			UT_WriteREG(vdo3_reg_base + 0x4e0, 0x0b0001c0);
			UT_WriteREG(vdo3_reg_base + 0x208, 0x87000780);
			UT_WriteREG(vdo3_reg_base + 0x20c, 0x000000f0);
			UT_WriteREG(vdo3_reg_base + 0x218, 0x8f100000);
			//sof
			UT_WriteREG(fmt_reg_base + 0x100, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x104, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x108, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x10c, 0x00020328);
			break;
		}

		//rest dispfmt and vdo
		UT_WriteREG(vdo3_reg_base + 0xac, 0x401);
		UT_WriteREG(vdo3_reg_base + 0xac, 0x001);
		UT_WriteREG(vdo3_reg_base + 0x43c, 0xff);
		UT_WriteREG(vdo3_reg_base + 0x43c, 0x00);
		UT_WriteREG(vdo3_reg_base + 0xac, 0x401);
		UT_WriteREG(vdo3_reg_base + 0xac, 0x001);
		UT_WriteREG(vdo3_reg_base + 0x43c, 0xff);
		UT_WriteREG(vdo3_reg_base + 0x43c, 0x00);
	}

	if (dv_ut_info.dv_layer[1].on) {
		UT_WriteREG(vdo4_reg_base + 0x6c, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x70, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x74, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x78, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x7c, 0x0);
		UT_WriteREG(vdo4_reg_base + 0xb4, 0x00d0e0f0);
		UT_WriteREG(vdo4_reg_base + 0xb8, 0x00302010);
		UT_WriteREG(vdo4_reg_base + 0xbc, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x1a0, 0x80000040);
		UT_WriteREG(vdo4_reg_base + 0xe4, 0x60);
		//scale
		UT_WriteREG(vdo4_reg_base + 0xb0, 0x10000001);
		//vdo
		UT_WriteREG(vdo4_reg_base + 0x430, 0x3);
		UT_WriteREG(vdo4_reg_base + 0x438, 0x6000000);
		UT_WriteREG(vdo4_reg_base + 0x458, 0xc0000);
		UT_WriteREG(vdo4_reg_base + 0x45c, 0xc0000);
		UT_WriteREG(vdo4_reg_base + 0x414, 0x800);
		UT_WriteREG(vdo4_reg_base + 0x434, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x464, 0x8000);
		UT_WriteREG(vdo4_reg_base + 0x470, 0x10000000);
		UT_WriteREG(vdo4_reg_base + 0x468, 0x20000814);
		UT_WriteREG(vdo4_reg_base + 0x418, 0x1e1e0000);
		UT_WriteREG(vdo4_reg_base + 0x478, 0x00010700);
		UT_WriteREG(vdo4_reg_base + 0x420, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x424, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x428, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x42c, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x450, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x454, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x46c, 0x05110000);
		UT_WriteREG(vdo4_reg_base + 0x4d0, 0x02001433);
		UT_WriteREG(vdo4_reg_base + 0xf8, 0x0);
		UT_WriteREG(vdo4_reg_base + 0x4c0, 0x02b32083);
		UT_WriteREG(vdo4_reg_base + 0x21c, 0x80000008);
		UT_WriteREG(vdo4_reg_base + 0xf4, 0x0);
		//enable ufo
		UT_WriteREG(vdo4_reg_base + 0x240, 0x3);
		UT_WriteREG(vdo4_reg_base + 0x200, ut_y_addr_pa[1]);
		UT_WriteREG(vdo4_reg_base + 0x204, ut_c_addr_pa[1]);
		//420-422 repeater
		UT_WriteREG(vdo4_reg_base + 0x41c, 0x83);
		UT_WriteREG(vdo4_reg_base + 0x424, 0x1);
		UT_WriteREG(vdo4_reg_base + 0x42c, 0x80);
		UT_WriteREG(vdo4_reg_base + 0x47c, 0x100);
		//active
		switch (timing) {
		case DOVI_480P:
			UT_WriteREG(vdo4_reg_base + 0xa0, 0x8070033f);
			UT_WriteREG(vdo4_reg_base + 0xa4, 0x002b020a);
			UT_WriteREG(vdo4_reg_base + 0xa8, 0x002b020a);
			UT_WriteREG(vdo4_reg_base + 0x9C, 0x000002d0);
			UT_WriteREG(vdo4_reg_base + 0x94, 0x00008220);
			UT_WriteREG(vdo4_reg_base + 0xb0, 0x01000001);
			UT_WriteREG(vdo4_reg_base + 0xcc, 0x00100000);
			UT_WriteREG(vdo4_reg_base + 0xe4, 0x00000004);
			UT_WriteREG(vdo4_reg_base + 0xe8, 0x04180356);
			UT_WriteREG(vdo4_reg_base + 0xec, 0x04160356);
			UT_WriteREG(vdo4_reg_base + 0xc8, 0x00000000);
			UT_WriteREG(vdo4_reg_base + 0xd0, 0x067202ee);
			UT_WriteREG(vdo4_reg_base + 0xd4, 0x067202ee);
			UT_WriteREG(vdo4_reg_base + 0xf4, 0x00000000);
			UT_WriteREG(vdo4_reg_base + 0xf8, 0x00100000);
			UT_WriteREG(vdo4_reg_base + 0x410, 0x01e0b45a);
			UT_WriteREG(vdo4_reg_base + 0x414, 0x80000800);
			UT_WriteREG(vdo4_reg_base + 0x464, 0x00008702);
			UT_WriteREG(vdo4_reg_base + 0x4cc, 0x00007601);
			UT_WriteREG(vdo4_reg_base + 0x208, 0x1e000180);
			UT_WriteREG(vdo4_reg_base + 0x20c, 0x00000030);
			UT_WriteREG(vdo4_reg_base + 0x218, 0x87100000);
			UT_WriteREG(vdo4_reg_base + 0x21c, 0x00000004);
			UT_WriteREG(vdo4_reg_base + 0x14c, 0x00212f00);
			//sof
			UT_WriteREG(fmt_reg_base + 0x100, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x104, 0x00020329);
			UT_WriteREG(fmt_reg_base + 0x108, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x10c, 0x00020329);
			break;
		case DOVI_720P:
			UT_WriteREG(vdo4_reg_base + 0xa0, 0x01050604);
			UT_WriteREG(vdo4_reg_base + 0xa4, 0x001a02e9);
			UT_WriteREG(vdo4_reg_base + 0xa8, 0x001a02e9);
			UT_WriteREG(vdo4_reg_base + 0x9C, 0x00000500);
			UT_WriteREG(vdo4_reg_base + 0x94, 0x0000e220);
			UT_WriteREG(vdo4_reg_base + 0xe8, 0x80020661);
			UT_WriteREG(vdo4_reg_base + 0xd0, 0x067202ee);
			UT_WriteREG(vdo4_reg_base + 0xd4, 0x067202ee);
			UT_WriteREG(vdo4_reg_base + 0x410, 0x02d0b4a0);
			UT_WriteREG(vdo4_reg_base + 0x4e0, 0x03000140);
			UT_WriteREG(vdo4_reg_base + 0x208, 0x2d000280);
			UT_WriteREG(vdo4_reg_base + 0x20c, 0x00000050);
			UT_WriteREG(vdo4_reg_base + 0x218, 0x8f100000);
			//sof
			UT_WriteREG(fmt_reg_base + 0x100, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x104, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x108, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x10c, 0x00020328);
			break;
		case DOVI_1080P:
			UT_WriteREG(vdo4_reg_base + 0xa0, 0x00c10840);
			UT_WriteREG(vdo4_reg_base + 0xa4, 0x002a0461);
			UT_WriteREG(vdo4_reg_base + 0xa8, 0x002a0461);
			UT_WriteREG(vdo4_reg_base + 0x9C, 0x00000780);
			UT_WriteREG(vdo4_reg_base + 0x94, 0x0000c220);
			UT_WriteREG(vdo4_reg_base + 0xe8, 0x08c80887);
			UT_WriteREG(vdo4_reg_base + 0xd0, 0x08980465);
			UT_WriteREG(vdo4_reg_base + 0xd4, 0x08980465);
			UT_WriteREG(vdo4_reg_base + 0x410, 0x0438e0f0);
			UT_WriteREG(vdo4_reg_base + 0x4e0, 0x010001e0);
			UT_WriteREG(vdo4_reg_base + 0x208, 0x438003c0);
			UT_WriteREG(vdo4_reg_base + 0x20c, 0x00000078);
			UT_WriteREG(vdo4_reg_base + 0x218, 0x8f100000);
			//sof
			UT_WriteREG(fmt_reg_base + 0x100, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x104, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x108, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x10c, 0x00020328);
			break;
		case DOVI_2160P:
			UT_WriteREG(vdo4_reg_base + 0xa0, 0x00D90FD8);
			UT_WriteREG(vdo4_reg_base + 0xa4, 0x005308C2);
			UT_WriteREG(vdo4_reg_base + 0xa8, 0x005308C2);
			UT_WriteREG(vdo4_reg_base + 0x9C, 0x00000f00);
			UT_WriteREG(vdo4_reg_base + 0x94, 0x0000c220);
			UT_WriteREG(vdo4_reg_base + 0xe8, 0x11921130);
			UT_WriteREG(vdo4_reg_base + 0xd0, 0x913008ca);
			UT_WriteREG(vdo4_reg_base + 0xd4, 0x913008ca);
			UT_WriteREG(vdo4_reg_base + 0x410, 0x1870c0e0);
			UT_WriteREG(vdo4_reg_base + 0x4e0, 0x0b0001c0);
			UT_WriteREG(vdo4_reg_base + 0x208, 0x87000780);
			UT_WriteREG(vdo4_reg_base + 0x20c, 0x000000f0);
			UT_WriteREG(vdo4_reg_base + 0x218, 0x8f100000);
			//sof
			UT_WriteREG(fmt_reg_base + 0x100, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x104, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x108, 0x00020328);
			UT_WriteREG(fmt_reg_base + 0x10c, 0x00020328);
			break;
		}

		//rest dispfmt and vdo
		UT_WriteREG(vdo4_reg_base + 0xac, 0x401);
		UT_WriteREG(vdo4_reg_base + 0xac, 0x001);
		UT_WriteREG(vdo4_reg_base + 0x43c, 0xff);
		UT_WriteREG(vdo4_reg_base + 0x43c, 0x00);
		UT_WriteREG(vdo4_reg_base + 0xac, 0x401);
		UT_WriteREG(vdo4_reg_base + 0xac, 0x001);
		UT_WriteREG(vdo4_reg_base + 0x43c, 0xff);
		UT_WriteREG(vdo4_reg_base + 0x43c, 0x00);
	}
}

void disp_set_osd_plane(uint32_t timing)
{
	uint32_t *va = NULL;
	dma_addr_t pa = 0;
	char *reg_base = NULL;


	if (dv_ut_info.dv_layer[2].on) {
		va = ut_graphic_header_va[0];
		pa = ut_graphic_addr_pa[0];
		reg_base = fhd_osd_reg_base;
		UT_WriteREG(reg_base + 0x8, 0x432);
		UT_WriteREG(reg_base + 0x70, 0x40);
		switch (timing) {
		case DOVI_480P:
			//set osd head
			va[0] = 0xE0000000;
			va[1] &= 0xFF000000;
			va[1] |= ((pa & 0x0FFFFFFF) >> 4);
			va[2] = 0x1b0000b4;
			va[3] = 0x0;
			va[4] = 0x1000;
			va[5] = 0x1000;
			va[6] &= (~(0x3 << 2));
			va[6] |= (((pa & 0xC0000000) >> 30) << 2);
			va[7] &= (~(0x3 << 23));
			va[7] |= (((pa & 0x30000000) >> 28) << 23);
			va[8] = 0x0;
			va[9] = 0x01e002d0;
			va[10] = 0x01e0;
			va[11] = 0x2d0;
			//set osd
			UT_WriteREG(reg_base + 0xc, 0x0040320d);
			UT_WriteREG(reg_base + 0x10, 0x88);
			UT_WriteREG(reg_base + 0x18, 0x002b002b);
			UT_WriteREG(reg_base + 0x1c, 0x02d001e0);
			UT_WriteREG(reg_base + 0x24, 0x0);
			UT_WriteREG(reg_base + 0x64, 0x35a);

			UT_WriteREG(reg_base + 0x100, 0x00ff0001);
			UT_WriteREG(reg_base + 0x104, ut_graphic_header_pa[0]);

			UT_WriteREG(reg_base + 0x200, 0x04000080);
			UT_WriteREG(reg_base + 0x204, 0x02d001e0);
			UT_WriteREG(reg_base + 0x208, 0x02d001e0);
			UT_WriteREG(reg_base + 0x20c, 0x000002d0);
			UT_WriteREG(reg_base + 0x10c, 0x078F3FCF);
			UT_WriteREG(reg_base + 0x0, 0xf);

			UT_WriteREG(fmt_reg_base + 0x208, 0x00010002);
			UT_WriteREG(fmt_reg_base + 0x20c, 0x00010003);
			break;
		case DOVI_720P:
			//set osd head
			va[0] = 0xE0000000;
			va[1] &= 0xFF000000;
			va[1] |= ((pa & 0x0FFFFFFF) >> 4);
			va[2] = 0x1B000140;
			va[3] = 0x0;
			va[4] = 0x1000;
			va[5] = 0x1000;
			va[6] &= (~(0x3 << 2));
			va[6] |= (((pa & 0xC0000000) >> 30) << 2);
			va[7] &= (~(0x3 << 23));
			va[7] |= (((pa & 0x30000000) >> 28) << 23);
			va[8] = 0x0;
			va[9] = 0x02d00500;
			va[10] = 0x2d0;
			va[11] = 0x500;
			//set osd
			UT_WriteREG(reg_base + 0xc, 0x004032ee);
			UT_WriteREG(reg_base + 0x10, 0x88);
			UT_WriteREG(reg_base + 0x18, 0x001b001b);
			UT_WriteREG(reg_base + 0x1c, 0x050002d0);
			UT_WriteREG(reg_base + 0x24, 0x0);
			UT_WriteREG(reg_base + 0x64, 0x672);

			UT_WriteREG(reg_base + 0x100, 0x00ff0001);
			UT_WriteREG(reg_base + 0x104, ut_graphic_header_pa[0]);

			UT_WriteREG(reg_base + 0x200, 0x04000080);
			UT_WriteREG(reg_base + 0x204, 0x050002d0);
			UT_WriteREG(reg_base + 0x208, 0x050002d0);
			UT_WriteREG(reg_base + 0x20c, 0x00000500);
			UT_WriteREG(reg_base + 0x10c, 0x078F3FCF);
			UT_WriteREG(reg_base + 0x0, 0xf);

			UT_WriteREG(fmt_reg_base + 0x208, 0x00010002);
			UT_WriteREG(fmt_reg_base + 0x20c, 0x00010003);
			break;
		case DOVI_1080P:
			//set osd head
			va[0] = 0xE0000000;
			va[1] &= 0xFF000000;
			va[1] |= ((pa & 0x0FFFFFFF) >> 4);
			va[2] = 0x1B0001e0;
			va[3] = 0x0;
			va[4] = 0x1000;
			va[5] = 0x1000;
			va[6] &= (~(0x3 << 2));
			va[6] |= (((pa & 0xC0000000) >> 30) << 2);
			va[7] &= (~(0x3 << 23));
			va[7] |= (((pa & 0x30000000) >> 28) << 23);
			va[8] = 0x0;
			va[9] = 0x04380780;
			va[10] = 0x438;
			va[11] = 0x780;
			//set osd
			UT_WriteREG(reg_base + 0xc, 0x00403465);
			UT_WriteREG(reg_base + 0x10, 0xc8);
			UT_WriteREG(reg_base + 0x18, 0x002b002b);
			UT_WriteREG(reg_base + 0x1c, 0x07800438);
			UT_WriteREG(reg_base + 0x24, 0x0);
			UT_WriteREG(reg_base + 0x64, 0x898);

			UT_WriteREG(reg_base + 0x100, 0x00ff0001);
			UT_WriteREG(reg_base + 0x104, ut_graphic_header_pa[0]);

			UT_WriteREG(reg_base + 0x200, 0x04000080);
			UT_WriteREG(reg_base + 0x204, 0x07800438);
			UT_WriteREG(reg_base + 0x208, 0x07800438);
			UT_WriteREG(reg_base + 0x20c, 0x00000780);
			UT_WriteREG(reg_base + 0x10c, 0x078F3FCF);
			UT_WriteREG(reg_base + 0x0, 0xf);

			UT_WriteREG(fmt_reg_base + 0x208, 0x00010002);
			UT_WriteREG(fmt_reg_base + 0x20c, 0x00010003);
			break;
		case DOVI_2160P:
			//set osd head
			va[0] = 0xE0000000;
			va[1] &= 0xFF000000;
			va[1] |= ((pa & 0x0FFFFFFF) >> 4);
			va[2] = 0x1B0003c0;
			va[3] = 0x0;
			va[4] = 0x1000;
			va[5] = 0x1000;
			va[6] &= (~(0x3 << 2));
			va[6] |= (((pa & 0xC0000000) >> 30) << 2);
			va[7] &= (~(0x3 << 23));
			va[7] |= (((pa & 0x30000000) >> 28) << 23);
			va[8] = 0x0;
			va[9] = 0x08700f00;
			va[10] = 0x870;
			va[11] = 0xf00;
			//set osd
			UT_WriteREG(reg_base + 0xc, 0x004038ca);
			UT_WriteREG(reg_base + 0x10, 0xc8);
			UT_WriteREG(reg_base + 0x18, 0x002b002b);
			UT_WriteREG(reg_base + 0x1c, 0x0f000870);
			UT_WriteREG(reg_base + 0x24, 0x0);
			UT_WriteREG(reg_base + 0x64, 0x1130);

			UT_WriteREG(reg_base + 0x100, 0x00ff0001);
			UT_WriteREG(reg_base + 0x104, ut_graphic_header_pa[0]);
			UT_WriteREG(reg_base + 0x108, 0x0008FFFF);

			UT_WriteREG(reg_base + 0x200, 0x04000080);
			UT_WriteREG(reg_base + 0x204, 0x0f000870);
			UT_WriteREG(reg_base + 0x208, 0x0f000870);
			UT_WriteREG(reg_base + 0x20c, 0x00000f00);
			UT_WriteREG(reg_base + 0x10c, 0x078F3FCF);
			UT_WriteREG(reg_base + 0x0, 0xf);

			UT_WriteREG(fmt_reg_base + 0x208, 0x00010002);
			UT_WriteREG(fmt_reg_base + 0x20c, 0x00010003);
			break;
		}
	}
	if (dv_ut_info.dv_layer[3].on) {
		va = ut_graphic_header_va[1];
		pa = ut_graphic_addr_pa[1];
		reg_base = uhd_osd_reg_base;
		UT_WriteREG(reg_base + 0x8, 0x432);
		UT_WriteREG(reg_base + 0x70, 0x40);
		switch (timing) {
		case DOVI_480P:
			//set osd head
			va[0] = 0xE0000000;
			va[1] &= 0xFF000000;
			va[1] |= ((pa & 0x0FFFFFFF) >> 4);
			va[2] = 0x1b0000b4;
			va[3] = 0x0;
			va[4] = 0x1000;
			va[5] = 0x1000;
			va[6] &= (~(0x3 << 2));
			va[6] |= (((pa & 0xC0000000) >> 30) << 2);
			va[7] &= (~(0x3 << 23));
			va[7] |= (((pa & 0x30000000) >> 28) << 23);
			va[8] = 0x0;
			va[9] = 0x01e002d0;
			va[10] = 0x01e0;
			va[11] = 0x2d0;
			//set osd
			UT_WriteREG(reg_base + 0xc, 0x0040320d);
			UT_WriteREG(reg_base + 0x10, 0x88);
			UT_WriteREG(reg_base + 0x18, 0x002b002b);
			UT_WriteREG(reg_base + 0x1c, 0x02d001e0);
			UT_WriteREG(reg_base + 0x24, 0x0);
			UT_WriteREG(reg_base + 0x64, 0x35a);

			UT_WriteREG(reg_base + 0x100, 0x00ff0001);
			UT_WriteREG(reg_base + 0x104, ut_graphic_header_pa[1]);

			UT_WriteREG(reg_base + 0x200, 0x04000080);
			UT_WriteREG(reg_base + 0x204, 0x02d001e0);
			UT_WriteREG(reg_base + 0x208, 0x02d001e0);
			UT_WriteREG(reg_base + 0x20c, 0x000002d0);
			UT_WriteREG(reg_base + 0x10c, 0x078F3FCF);
			UT_WriteREG(reg_base + 0x0, 0xf);

			UT_WriteREG(fmt_reg_base + 0x170, 0x00010002);
			UT_WriteREG(fmt_reg_base + 0x174, 0x00010003);
			break;
		case DOVI_720P:
			//set osd head
			va[0] = 0xE0000000;
			va[1] &= 0xFF000000;
			va[1] |= ((pa & 0x0FFFFFFF) >> 4);
			va[2] = 0x1B000140;
			va[3] = 0x0;
			va[4] = 0x1000;
			va[5] = 0x1000;
			va[6] &= (~(0x3 << 2));
			va[6] |= (((pa & 0xC0000000) >> 30) << 2);
			va[7] &= (~(0x3 << 23));
			va[7] |= (((pa & 0x30000000) >> 28) << 23);
			va[8] = 0x0;
			va[9] = 0x02d00500;
			va[10] = 0x2d0;
			va[11] = 0x500;
			//set osd
			UT_WriteREG(reg_base + 0xc, 0x004032ee);
			UT_WriteREG(reg_base + 0x10, 0x88);
			UT_WriteREG(reg_base + 0x18, 0x001b001b);
			UT_WriteREG(reg_base + 0x1c, 0x050002d0);
			UT_WriteREG(reg_base + 0x24, 0x0);
			UT_WriteREG(reg_base + 0x64, 0x672);

			UT_WriteREG(reg_base + 0x100, 0x00ff0001);
			UT_WriteREG(reg_base + 0x104, ut_graphic_header_pa[1]);

			UT_WriteREG(reg_base + 0x200, 0x04000080);
			UT_WriteREG(reg_base + 0x204, 0x050002d0);
			UT_WriteREG(reg_base + 0x208, 0x050002d0);
			UT_WriteREG(reg_base + 0x20c, 0x00000500);
			UT_WriteREG(reg_base + 0x10c, 0x078F3FCF);
			UT_WriteREG(reg_base + 0x0, 0xf);

			UT_WriteREG(fmt_reg_base + 0x170, 0x00010002);
			UT_WriteREG(fmt_reg_base + 0x174, 0x00010003);
			break;
		case DOVI_1080P:
			//set osd head
			va[0] = 0xE0000000;
			va[1] &= 0xFF000000;
			va[1] |= ((pa & 0x0FFFFFFF) >> 4);
			va[2] = 0x1B0001e0;
			va[3] = 0x0;
			va[4] = 0x1000;
			va[5] = 0x1000;
			va[6] &= (~(0x3 << 2));
			va[6] |= (((pa & 0xC0000000) >> 30) << 2);
			va[7] &= (~(0x3 << 23));
			va[7] |= (((pa & 0x30000000) >> 28) << 23);
			va[8] = 0x0;
			va[9] = 0x04380780;
			va[10] = 0x438;
			va[11] = 0x780;
			//set osd
			UT_WriteREG(reg_base + 0xc, 0x00403465);
			UT_WriteREG(reg_base + 0x10, 0xc8);
			UT_WriteREG(reg_base + 0x18, 0x002b002b);
			UT_WriteREG(reg_base + 0x1c, 0x07800438);
			UT_WriteREG(reg_base + 0x24, 0x0);
			UT_WriteREG(reg_base + 0x64, 0x898);

			UT_WriteREG(reg_base + 0x100, 0x00ff0001);
			UT_WriteREG(reg_base + 0x104, ut_graphic_header_pa[1]);

			UT_WriteREG(reg_base + 0x200, 0x04000080);
			UT_WriteREG(reg_base + 0x204, 0x07800438);
			UT_WriteREG(reg_base + 0x208, 0x07800438);
			UT_WriteREG(reg_base + 0x20c, 0x00000780);
			UT_WriteREG(reg_base + 0x10c, 0x078F3FCF);
			UT_WriteREG(reg_base + 0x0, 0xf);

			UT_WriteREG(fmt_reg_base + 0x170, 0x00010002);
			UT_WriteREG(fmt_reg_base + 0x174, 0x00010003);
			break;
		case DOVI_2160P:
			//set osd head
			va[0] = 0xE0000000;
			va[1] &= 0xFF000000;
			va[1] |= ((pa & 0x0FFFFFFF) >> 4);
			va[2] = 0x1B0003c0;
			va[3] = 0x0;
			va[4] = 0x1000;
			va[5] = 0x1000;
			va[6] &= (~(0x3 << 2));
			va[6] |= (((pa & 0xC0000000) >> 30) << 2);
			va[7] &= (~(0x3 << 23));
			va[7] |= (((pa & 0x30000000) >> 28) << 23);
			va[8] = 0x0;
			va[9] = 0x08700f00;
			va[10] = 0x870;
			va[11] = 0xf00;
			//set osd
			UT_WriteREG(reg_base + 0xc, 0x004038ca);
			UT_WriteREG(reg_base + 0x10, 0xc8);
			UT_WriteREG(reg_base + 0x18, 0x002b002b);
			UT_WriteREG(reg_base + 0x1c, 0x0f000870);
			UT_WriteREG(reg_base + 0x24, 0x0);
			UT_WriteREG(reg_base + 0x64, 0x1130);

			UT_WriteREG(reg_base + 0x100, 0x00ff0001);
			UT_WriteREG(reg_base + 0x104, ut_graphic_header_pa[1]);
			UT_WriteREG(reg_base + 0x108, 0x0008FFFF);

			UT_WriteREG(reg_base + 0x200, 0x04000080);
			UT_WriteREG(reg_base + 0x204, 0x0f000870);
			UT_WriteREG(reg_base + 0x208, 0x0f000870);
			UT_WriteREG(reg_base + 0x20c, 0x00000f00);
			UT_WriteREG(reg_base + 0x10c, 0x078F3FCF);
			UT_WriteREG(reg_base + 0x0, 0xf);

			UT_WriteREG(fmt_reg_base + 0x170, 0x00010002);
			UT_WriteREG(fmt_reg_base + 0x174, 0x00010003);
			break;
		}
	}
}

void disp_set_vin(uint32_t timing)
{
	dma_addr_t y_buf = ut_vin_y_addr_pa;
	dma_addr_t c_buf = ut_vin_c_addr_pa;

	UT_WriteREG(vin_reg_base + 0x8, y_buf >> 4);
	UT_WriteREG(vin_reg_base + 0x10, c_buf >> 4);
	UT_WriteREG(vin_reg_base + 0x20, 0x04010224);
	UT_WriteREG(vin_reg_base + 0x58, 0x0007e900);
	UT_WriteREG(vin_reg_base + 0x5c, 0x0);
	UT_WriteREG(vin_reg_base + 0x7c, 0x230e2004);


	switch (timing) {
	case DOVI_480P:
		UT_WriteREG(vin_reg_base + 0xc, 0xc00021df);
		UT_WriteREG(vin_reg_base + 0x14, 0x01df01df);
		UT_WriteREG(vin_reg_base + 0x18, 0x0001007a);
		UT_WriteREG(vin_reg_base + 0x24, 0x00000025);
		UT_WriteREG(vin_reg_base + 0x34, 0x02d00359);
		UT_WriteREG(vin_reg_base + 0x38, 0x002c002c);
		UT_WriteREG(vin_reg_base + 0x3c, 0x002d54e3);
		UT_WriteREG(vin_reg_base + 0x80, 0xb0800000);
		UT_WriteREG(vin_reg_base + 0xd0, 0x01df002c);
		break;
	case DOVI_720P:
		UT_WriteREG(vin_reg_base + 0xc, 0xc00022CF);
		UT_WriteREG(vin_reg_base + 0x14, 0x02CF02CF);
		UT_WriteREG(vin_reg_base + 0x18, 0x000100C9);
		UT_WriteREG(vin_reg_base + 0x24, 0x00000019);
		UT_WriteREG(vin_reg_base + 0x34, 0x05000671);
		UT_WriteREG(vin_reg_base + 0x38, 0x004F004F);
		UT_WriteREG(vin_reg_base + 0x3c, 0x005054e3);
		UT_WriteREG(vin_reg_base + 0x80, 0xb0800000);
		UT_WriteREG(vin_reg_base + 0xd0, 0x02CF004F);
		break;
	case DOVI_1080P:
		UT_WriteREG(vin_reg_base + 0xc, 0xc0002437);
		UT_WriteREG(vin_reg_base + 0x14, 0x04370437);
		UT_WriteREG(vin_reg_base + 0x18, 0x000100C1);
		UT_WriteREG(vin_reg_base + 0x24, 0x0000002B);
		UT_WriteREG(vin_reg_base + 0x34, 0x07800897);
		UT_WriteREG(vin_reg_base + 0x38, 0x00770077);
		UT_WriteREG(vin_reg_base + 0x3c, 0x007854e3);
		UT_WriteREG(vin_reg_base + 0x80, 0xB0800000);
		UT_WriteREG(vin_reg_base + 0xd0, 0x04370077);
		break;
	case DOVI_2160P:
		UT_WriteREG(vin_reg_base + 0xc, 0xc000286F);
		UT_WriteREG(vin_reg_base + 0x14, 0x086F086F);
		UT_WriteREG(vin_reg_base + 0x18, 0x000100C9);
		UT_WriteREG(vin_reg_base + 0x24, 0x00000029);
		UT_WriteREG(vin_reg_base + 0x34, 0x0F00112F);
		UT_WriteREG(vin_reg_base + 0x38, 0x00EF00EF);
		UT_WriteREG(vin_reg_base + 0x3c, 0x00F054e3);
		UT_WriteREG(vin_reg_base + 0x80, 0xb0800000);
		UT_WriteREG(vin_reg_base + 0xd0, 0x086F00EF);
		break;
	}
	UT_WriteREG(vin_reg_base + 0xd4, 0xa000401d);

}
void dovi_set_path_info(uint32_t timing)
{
	//set timing & path
	dovi_default("%s set timing %d\n", __func__, timing);
	disp_fmt_set_timing(timing);
	disp_set_fe_fifo(timing);
	disp_set_be_fifo(timing);
	disp_set_disp_mix_and_path(timing);

	//set vdo
	disp_set_vdo_plane(timing);
	disp_set_osd_plane(timing);
	disp_set_vin(timing);

	//trigger hdr path run
	//disp_dovi_default_path_init(1);
}

void dovi_set_unit_test_info(uint32_t out_res, uint32_t out_fmt,
	uint32_t ll_on, uint32_t ll_rgb,
	uint32_t *layer_on, uint32_t *case_id)
{
	uint8_t i = 0;
	bool invalid_setting = 1;

	if (!(layer_on && case_id)) {
		dovi_error("set unit test info error\n");
		return;
	}
	for (i = 0; i < UT_LAYER; i++) {
		if (layer_on[i]) {
			invalid_setting = 0;
			dv_ut_info.dv_layer[i].on = 1;
			if (case_id[i] == 5005) {
				dv_ut_info.dv_layer[i].case_id = 5005;
				dv_ut_info.dv_layer[i].width = 720;
				dv_ut_info.dv_layer[i].heigh = 480;
			} else if (case_id[i] == 5004) {
				dv_ut_info.dv_layer[i].case_id = 5004;
				dv_ut_info.dv_layer[i].width = 1280;
				dv_ut_info.dv_layer[i].heigh = 720;
			} else if (case_id[i] == 5000) {
				dv_ut_info.dv_layer[i].case_id = 5000;
				dv_ut_info.dv_layer[i].width = 1920;
				dv_ut_info.dv_layer[i].heigh = 1080;
			} else if (case_id[i] == 5003) {
				dv_ut_info.dv_layer[i].case_id = 5003;
				dv_ut_info.dv_layer[i].width = 3840;
				dv_ut_info.dv_layer[i].heigh = 2160;
			}
		}
	}
	if (invalid_setting) {
		//disable hdr.extern bypass
		disp_dovi_default_path_init(0);
		dovi_error("ut setting error\n");
		return;
	}

	dv_ut_info.out_res = (enum dovi_resolution)out_res;
	dv_ut_info.out_format = (enum dovi_format)out_fmt;
	dv_ut_info.use_ll = (bool)ll_on;
	dv_ut_info.ll_rgb = (bool)ll_rgb;

	dovi_printf("dovi ut info out(%d %d %d %d)\n",
		dv_ut_info.out_format,
		dv_ut_info.out_res,
		dv_ut_info.ll_rgb,
		dv_ut_info.use_ll);

	dovi_printf("layer0 info (%d %d %d %d %d)\n",
		dv_ut_info.dv_layer[0].on,
		dv_ut_info.dv_layer[0].case_id,
		dv_ut_info.dv_layer[0].in_format,
		dv_ut_info.dv_layer[0].width,
		dv_ut_info.dv_layer[0].heigh);
	dovi_printf("layer1 info (%d %d %d %d %d)\n",
		dv_ut_info.dv_layer[1].on,
		dv_ut_info.dv_layer[1].case_id,
		dv_ut_info.dv_layer[1].in_format,
		dv_ut_info.dv_layer[1].width,
		dv_ut_info.dv_layer[1].heigh);
	dovi_printf("layer2 info (%d %d %d %d %d)\n",
		dv_ut_info.dv_layer[2].on,
		dv_ut_info.dv_layer[2].case_id,
		dv_ut_info.dv_layer[2].in_format,
		dv_ut_info.dv_layer[2].width,
		dv_ut_info.dv_layer[2].heigh);
	dovi_printf("layer3 info (%d %d %d %d %d)\n",
		dv_ut_info.dv_layer[3].on,
		dv_ut_info.dv_layer[3].case_id,
		dv_ut_info.dv_layer[3].in_format,
		dv_ut_info.dv_layer[3].width,
		dv_ut_info.dv_layer[3].heigh);
}

void disp_dovi_reg_test(uint32_t value)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t reg_val = 0;

	for (i = 0 ; i < DOVI_CORE_MAX; i++)
		for (j = 0; j < 256; j++)
			UT_WriteREG(dovi_reg_base[i] + j*4, 0xffff);

	for (i = 0; i < DOVI_CORE_MAX; i++) {
		dovi_printf("read regbase 0x%p\n", dovi_reg_base[i]);
		for (j = 0; j < 256; j++) {
			reg_val = UT_ReadREG(dovi_reg_base[i] + j*4);
			dovi_printf("reg_val 0x%08x\n", reg_val);
		}
	}
}

void dovi_ut_dts_node_parse(void)
{
	struct device_node *np;
	uint32_t reg_value;

	/*fmt 0x14001000*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-fmt");
	if (np == NULL) {
		dovi_error("dts error, no fmt device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	fmt_reg_base = (char *)of_iomap(np, 1);
	d_top_reg_base = (char *)of_iomap(np, 2);

	dovi_default("ut_parsefmt 0x%p 0x%p\n", fmt_reg_base, d_top_reg_base);

	/*fe fifo 0x14010000, 0x14011000, 0x15010000, 0x15017000*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-fefifo");
	if (np == NULL) {
		dovi_error("dts error, no fefifo device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	m_fifo_reg_base = (char *)of_iomap(np, 2);
	s_fifo_reg_base = (char *)of_iomap(np, 3);
	fhd_fifo_reg_base = (char *)of_iomap(np, 0);
	uhd_fifo_reg_base = (char *)of_iomap(np, 1);
	dovi_default("ut_parsefefifo 0x%p 0x%p 0x%p 0x%p\n",
		fhd_fifo_reg_base, uhd_fifo_reg_base,
		m_fifo_reg_base, s_fifo_reg_base);

	/*befifo 0x1400f000*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-befifo");
	if (np == NULL) {
		dovi_error("dts error, no fmt device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	be_fifo_reg_base = (char *)of_iomap(np, 0);

	dovi_default("ut_parsebefifo 0x%p\n",
		be_fifo_reg_base);



	/*dispmix 0x14014000*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-dispmix");
	if (np == NULL) {
		dovi_error("dts error, dispmix device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	disp_mix_reg_base = (char *)of_iomap(np, 0);

	dovi_default("ut_parsemix 0x%p\n",
		disp_mix_reg_base);

	/*mmsys 0x14000000*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-mmsys");
	if (np == NULL) {
		dovi_error("dts error, mmsys device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	mmsys_cfg_reg_base = (char *)of_iomap(np, 0);
	dovi_default("ut_parsemmsys 0x%p\n",
		mmsys_cfg_reg_base);

	/*vdout 0x14001500*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-vdout");
	if (np == NULL) {
		dovi_error("dts error, vdout device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	m_top_reg_base = (char *)of_iomap(np, 0) + 0x500;

	dovi_default("ut_parse vdout 0x%p\n",
		m_top_reg_base);


	/*vdo  0x15001000 0x15008000*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-vdo");
	if (np == NULL) {
		dovi_error("dts error, vdo device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	vdo3_reg_base = (char *)of_iomap(np, 0);
	vdo4_reg_base = (char *)of_iomap(np, 1);

	dovi_default("ut_parse vdo 0x%p 0x%p\n",
		vdo3_reg_base, vdo4_reg_base);


	/*osd  0x14003000 0x14004000*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-osd");
	if (np == NULL) {
		dovi_error("dts error, osd device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	uhd_osd_reg_base = (char *)of_iomap(np, 0);
	fhd_osd_reg_base = (char *)of_iomap(np, 1);

	dovi_default("ut_parse osd 0x%p 0x%p\n",
		fhd_osd_reg_base, uhd_osd_reg_base);


	/*vin  0x14002000*/
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-videoin");
	if (np == NULL) {
		dovi_error("dts error, vin device node.\n");
		return;
	}
	of_property_read_u32_index(np, "reg", 1, &reg_value);
	vin_reg_base = (char *)of_iomap(np, 0);
	dovi_default("ut_parse vin 0x%p\n",
		vin_reg_base);

}
void dovi_ut_init(uint32_t en)
{
	dovi_default("alloc mva for vfy used\n");
	if (en == 0)
		dovi_ut_alloc_adl_ml_table_mem(false);
	else
		dovi_ut_alloc_adl_ml_table_mem(true);

	dovi_ut_dts_node_parse();
}

void dovi_ut_config_adl_ml(uint32_t case_id)
{
	struct dovi_unit_info_t *p_ut_info = NULL;
	struct dv_layer_info *p_layer = NULL;
	uint32_t lut_len = 0;

	void *va = NULL;
	dma_addr_t mva = 0;


	p_ut_info = &dv_ut_info;

	if (case_id == MAIN_HDR_CASE) {
		p_layer = &(p_ut_info->dv_layer[DV_MAIN_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_V_MAIN, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_V_MAIN, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_V_MAIN, 1, 0);
			disp_adl_cfg_client(DV_ADL_V_MAIN, mva, lut_len);
		}
		lut_len = p_ut_info->md_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_SCRM, &va, &mva);
			memcpy(va, p_ut_info->md_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_SCRM, 0, 2);
			disp_adl_cfg_client_en(DV_SCRM, 1, 0);
			disp_adl_cfg_client(DV_SCRM, mva, lut_len);
		}

	}

	if (case_id == SUB_HDR_CASE) {
		p_layer = &(p_ut_info->dv_layer[DV_SUB_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_V_SUB, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_V_SUB, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_V_SUB, 1, 0);
			disp_adl_cfg_client(DV_ADL_V_SUB, mva, lut_len);
		}
		lut_len = p_ut_info->md_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_SCRM, &va, &mva);
			memcpy(va, p_ut_info->md_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_SCRM, 0, 2);
			disp_adl_cfg_client_en(DV_SCRM, 1, 0);
			disp_adl_cfg_client(DV_SCRM, mva, lut_len);
		}

	}

	if (case_id == FHD_HDR_CASE) {
		p_layer = &(p_ut_info->dv_layer[DV_FHD_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_G_FHD, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_G_FHD, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_G_FHD, 1, 0);
			disp_adl_cfg_client(DV_ADL_G_FHD, mva, lut_len);
		}
		lut_len = p_ut_info->md_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_SCRM, &va, &mva);
			memcpy(va, p_ut_info->md_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_SCRM, 0, 2);
			disp_adl_cfg_client_en(DV_SCRM, 1, 0);
			disp_adl_cfg_client(DV_SCRM, mva, lut_len);
		}

	}

	if (case_id == UHD_HDR_CASE) {
		p_layer = &(p_ut_info->dv_layer[DV_UHD_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_G_UHD, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_G_UHD, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_G_UHD, 1, 0);
			disp_adl_cfg_client(DV_ADL_G_UHD, mva, lut_len);
		}
		lut_len = p_ut_info->md_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_SCRM, &va, &mva);
			memcpy(va, p_ut_info->md_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_SCRM, 0, 2);
			disp_adl_cfg_client_en(DV_SCRM, 1, 0);
			disp_adl_cfg_client(DV_SCRM, mva, lut_len);
		}

	}

	if (case_id == MAIN_FHD_MIX_CASE) {
		p_layer = &(p_ut_info->dv_layer[DV_MAIN_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_V_MAIN, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_V_MAIN, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_V_MAIN, 1, 0);
			disp_adl_cfg_client(DV_ADL_V_MAIN, mva, lut_len);
		}

		p_layer = &(p_ut_info->dv_layer[DV_FHD_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_G_FHD, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_G_FHD, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_G_FHD, 1, 0);
			disp_adl_cfg_client(DV_ADL_G_FHD, mva, lut_len);
		}

		lut_len = p_ut_info->md_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_SCRM, &va, &mva);
			memcpy(va, p_ut_info->md_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_SCRM, 0, 2);
			disp_adl_cfg_client_en(DV_SCRM, 1, 0);
			disp_adl_cfg_client(DV_SCRM, mva, lut_len);
		}

	}

	if (case_id == SUB_UHD_MIX_CASE) {
		p_layer = &(p_ut_info->dv_layer[DV_SUB_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_V_SUB, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_V_SUB, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_V_SUB, 1, 0);
			disp_adl_cfg_client(DV_ADL_V_SUB, mva, lut_len);
		}

		p_layer = &(p_ut_info->dv_layer[DV_UHD_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_G_UHD, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_G_UHD, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_G_UHD, 1, 0);
			disp_adl_cfg_client(DV_ADL_G_UHD, mva, lut_len);
		}

		lut_len = p_ut_info->md_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_SCRM, &va, &mva);
			memcpy(va, p_ut_info->md_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_SCRM, 0, 2);
			disp_adl_cfg_client_en(DV_SCRM, 1, 0);
			disp_adl_cfg_client(DV_SCRM, mva, lut_len);
		}

	}

	if (case_id == ALL_LAYER_MIX_CASE) {
		p_layer = &(p_ut_info->dv_layer[DV_MAIN_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_V_MAIN, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_V_MAIN, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_V_MAIN, 1, 0);
			disp_adl_cfg_client(DV_ADL_V_MAIN, mva, lut_len);
		}
		p_layer = &(p_ut_info->dv_layer[DV_SUB_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_V_SUB, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_V_SUB, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_V_SUB, 1, 0);
			disp_adl_cfg_client(DV_ADL_V_SUB, mva, lut_len);
		}

		p_layer = &(p_ut_info->dv_layer[DV_FHD_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_G_FHD, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_G_FHD, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_G_FHD, 1, 0);
			disp_adl_cfg_client(DV_ADL_G_FHD, mva, lut_len);
		}

		p_layer = &(p_ut_info->dv_layer[DV_UHD_FE]);
		lut_len = p_layer->lut_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_ADL_G_UHD, &va, &mva);
			memcpy(va, p_layer->lut_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_ADL_G_UHD, 0, 2);
			disp_adl_cfg_client_en(DV_ADL_G_UHD, 1, 0);
			disp_adl_cfg_client(DV_ADL_G_UHD, mva, lut_len);
		}

		lut_len = p_ut_info->md_len;
		if (lut_len) {
			disp_adl_get_clt_buf(DV_SCRM, &va, &mva);
			memcpy(va, p_ut_info->md_addr, lut_len * 32);
			disp_adl_set_client_mode(DV_SCRM, 0, 2);
			disp_adl_cfg_client_en(DV_SCRM, 1, 0);
			disp_adl_cfg_client(DV_SCRM, mva, lut_len);
		}

	}


	//update ml
	lut_len = p_ut_info->dsys_ml_cmd_len;
	if (lut_len)
		disp_ml_set_buffer(ML_DSYS_IP,
		p_ut_info->dsys_ml_addr, lut_len);

	lut_len = p_ut_info->msys_ml_cmd_len;
	if (lut_len)
		disp_ml_set_buffer(ML_MSYS_IP,
		p_ut_info->msys_ml_addr, lut_len);

}

void dovi_ut_all_mix_hdr(void)
{
	struct dovi_unit_info_t *p_ut_info = NULL;
	struct dv_layer_info *p_layer_main = NULL;
	struct dv_layer_info *p_layer_fhd = NULL;
	struct dv_layer_info *p_layer_sub = NULL;
	struct dv_layer_info *p_layer_uhd = NULL;
	uint32_t idk_case = 0;
	enum dovi_format out_fmt = 0;



	p_ut_info = &dv_ut_info;
	p_layer_main = &(dv_ut_info.dv_layer[DV_MAIN_FE]);
	if (p_layer_main->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	p_layer_sub = &(dv_ut_info.dv_layer[DV_SUB_FE]);
	if (p_layer_sub->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	p_layer_fhd = &(dv_ut_info.dv_layer[DV_FHD_FE]);
	if (p_layer_fhd->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	p_layer_uhd = &(dv_ut_info.dv_layer[DV_UHD_FE]);
	if (p_layer_uhd->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	idk_case = p_layer_main->case_id;
	out_fmt = p_ut_info->out_format;

	dovi_default("%s idk_case %d outfmt %d\n", __func__,
		idk_case, out_fmt);

	switch (idk_case) {
	case 5000://1080 main & fhd, 480P sub and uhd
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5000_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;

			p_layer_sub->lut_addr =
				idk_ut_5005_mix_ipt_sdr_v_lut;
			p_layer_sub->lut_len = HDR_FE_LUT_SIZE;

			p_layer_fhd->lut_addr =
				idk_ut_5000_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;

			p_layer_uhd->lut_addr =
				idk_ut_5005_mix_ipt_sdr_g_lut;
			p_layer_uhd->lut_len = HDR_FE_LUT_SIZE;

			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;

			p_ut_info->dsys_ml_addr =
				idk_ut_5000_mix_ipt_sdr_dsys_all_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384 * 2;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_mix_ipt_sdr_msys_all_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124 * 2;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr =
				idk_ut_5000_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;

			p_layer_sub->lut_addr =
				idk_ut_5005_mix_ipt_ipt_v_lut;
			p_layer_sub->lut_len = HDR_FE_LUT_SIZE;

			p_layer_fhd->lut_addr =
				idk_ut_5000_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;

			p_layer_uhd->lut_addr =
				idk_ut_5005_mix_ipt_ipt_g_lut;
			p_layer_uhd->lut_len = HDR_FE_LUT_SIZE;

			p_ut_info->md_addr =
				idk_ut_5000_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;

			p_ut_info->dsys_ml_addr =
				idk_ut_5000_mix_ipt_ipt_dsys_all_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384 * 2;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_mix_ipt_ipt_msys_all_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132 * 2;
		}
		break;
	case 5003://2160 main 1080sub
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5003_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;

			p_layer_sub->lut_addr =
				idk_ut_5000_mix_ipt_sdr_v_lut;
			p_layer_sub->lut_len = HDR_FE_LUT_SIZE;

			p_layer_fhd->lut_addr =
				idk_ut_5003_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;

			p_layer_uhd->lut_addr =
				idk_ut_5000_mix_ipt_sdr_g_lut;
			p_layer_uhd->lut_len = HDR_FE_LUT_SIZE;

			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;

			p_ut_info->dsys_ml_addr =
				idk_ut_5003_mix_ipt_sdr_dsys_all_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384 * 2;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_mix_ipt_sdr_msys_all_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124 * 2;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr =
				idk_ut_5003_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;

			p_layer_sub->lut_addr =
				idk_ut_5000_mix_ipt_ipt_v_lut;
			p_layer_sub->lut_len = HDR_FE_LUT_SIZE;

			p_layer_fhd->lut_addr =
				idk_ut_5003_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;

			p_layer_uhd->lut_addr =
				idk_ut_5000_mix_ipt_ipt_g_lut;
			p_layer_uhd->lut_len = HDR_FE_LUT_SIZE;

			p_ut_info->md_addr =
				idk_ut_5003_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;

			p_ut_info->dsys_ml_addr =
				idk_ut_5003_mix_ipt_ipt_dsys_all_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384 * 2;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_mix_ipt_ipt_msys_all_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132 * 2;
		}
		break;
	default:
		dovi_default("idk_case %d error\n", idk_case);
		break;
	}

	dovi_ut_config_adl_ml(ALL_LAYER_MIX_CASE);

}

void dovi_ut_sub_mix_uhd_hdr(void)
{
	struct dovi_unit_info_t *p_ut_info = NULL;
	struct dv_layer_info *p_layer_main = NULL;
	struct dv_layer_info *p_layer_fhd = NULL;
	uint32_t idk_case = 0;
	enum dovi_format out_fmt = 0;



	p_ut_info = &dv_ut_info;
	p_layer_main = &(dv_ut_info.dv_layer[DV_SUB_FE]);
	if (p_layer_main->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	p_layer_fhd = &(dv_ut_info.dv_layer[DV_UHD_FE]);
	if (p_layer_fhd->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	idk_case = p_layer_main->case_id;
	out_fmt = p_ut_info->out_format;

	dovi_default("%s idk_case %d outfmt %d\n", __func__,
		idk_case, out_fmt);

	switch (idk_case) {
	case 5005://480p
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5005_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr =
				idk_ut_5005_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5005_mix_ipt_sdr_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_mix_ipt_sdr_msys_sub_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr = idk_ut_5005_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr = idk_ut_5005_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5005_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5005_mix_ipt_ipt_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_mix_ipt_ipt_msys_sub_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5004://720
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5004_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr =
				idk_ut_5004_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5004_mix_ipt_sdr_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_mix_ipt_sdr_msys_sub_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr = idk_ut_5004_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr = idk_ut_5004_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5004_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5004_mix_ipt_ipt_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_mix_ipt_ipt_msys_sub_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5000://1080
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5000_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr =
				idk_ut_5000_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5000_mix_ipt_sdr_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_mix_ipt_sdr_msys_sub_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr = idk_ut_5000_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr = idk_ut_5000_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5000_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5000_mix_ipt_ipt_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_mix_ipt_ipt_msys_sub_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5003://2160
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5003_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr =
				idk_ut_5003_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5003_mix_ipt_sdr_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_mix_ipt_sdr_msys_sub_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr = idk_ut_5003_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr = idk_ut_5003_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5003_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5003_mix_ipt_ipt_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_mix_ipt_ipt_msys_sub_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	default:
		dovi_default("idk_case %d error\n", idk_case);
		break;
	}

	dovi_ut_config_adl_ml(SUB_UHD_MIX_CASE);

}

void dovi_ut_main_mix_fhd_hdr(void)
{
	struct dovi_unit_info_t *p_ut_info = NULL;
	struct dv_layer_info *p_layer_main = NULL;
	struct dv_layer_info *p_layer_fhd = NULL;
	uint32_t idk_case = 0;
	enum dovi_format out_fmt = 0;



	p_ut_info = &dv_ut_info;
	p_layer_main = &(dv_ut_info.dv_layer[DV_MAIN_FE]);
	if (p_layer_main->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	p_layer_fhd = &(dv_ut_info.dv_layer[DV_FHD_FE]);
	if (p_layer_fhd->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	idk_case = p_layer_main->case_id;
	out_fmt = p_ut_info->out_format;

	dovi_default("%s idk_case %d outfmt %d\n", __func__,
		idk_case, out_fmt);

	switch (idk_case) {
	case 5005://480p
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5005_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr =
				idk_ut_5005_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5005_mix_ipt_sdr_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_mix_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr = idk_ut_5005_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr = idk_ut_5005_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5005_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5005_mix_ipt_ipt_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_mix_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5004://720
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5004_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr =
				idk_ut_5004_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5004_mix_ipt_sdr_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_mix_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr = idk_ut_5004_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr = idk_ut_5004_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5004_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5004_mix_ipt_ipt_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_mix_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5000://1080
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5000_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr =
				idk_ut_5000_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5000_mix_ipt_sdr_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_mix_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr = idk_ut_5000_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr = idk_ut_5000_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5000_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5000_mix_ipt_ipt_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_mix_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5003://2160
		if (out_fmt == DOVI_SDR) {
			p_layer_main->lut_addr =
				idk_ut_5003_mix_ipt_sdr_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr =
				idk_ut_5003_mix_ipt_sdr_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5003_mix_ipt_sdr_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_mix_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer_main->lut_addr = idk_ut_5003_mix_ipt_ipt_v_lut;
			p_layer_main->lut_len = HDR_FE_LUT_SIZE;
			p_layer_fhd->lut_addr = idk_ut_5003_mix_ipt_ipt_g_lut;
			p_layer_fhd->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5003_mix_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5003_mix_ipt_ipt_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_mix_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	default:
		dovi_default("idk_case %d error\n", idk_case);
		break;
	}

	dovi_ut_config_adl_ml(MAIN_FHD_MIX_CASE);

}

void dovi_ut_uhd_hdr(void)
{
	struct dovi_unit_info_t *p_ut_info = NULL;
	struct dv_layer_info *p_layer = NULL;
	uint32_t idk_case = 0;
	enum dovi_format out_fmt = 0;


	p_ut_info = &dv_ut_info;
	p_layer = &(dv_ut_info.dv_layer[DV_UHD_FE]);
	if (p_layer->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	idk_case = p_layer->case_id;
	out_fmt = p_ut_info->out_format;

	dovi_default("%s idk_case %d outfmt %d\n", __func__,
		idk_case, out_fmt);

	switch (idk_case) {
	case 5005://480p
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5005_ipt_sdr_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_ipt_sdr_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5005_ipt_ipt_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5005_ipt_ipt_g_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_ipt_ipt_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5004://720
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5004_ipt_sdr_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_ipt_sdr_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5004_ipt_ipt_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5004_ipt_ipt_g_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_ipt_ipt_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5000://1080
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5000_ipt_sdr_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_ipt_sdr_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5000_ipt_ipt_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5000_ipt_ipt_g_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_ipt_ipt_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5003://2160
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5003_ipt_sdr_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_ipt_sdr_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5003_ipt_ipt_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5003_ipt_ipt_g_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_ipt_ipt_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	default:
		dovi_default("idk_case %d error\n", idk_case);
		break;
	}

	dovi_ut_config_adl_ml(UHD_HDR_CASE);

}

void dovi_ut_fhd_hdr(void)
{
	struct dovi_unit_info_t *p_ut_info = NULL;
	struct dv_layer_info *p_layer = NULL;
	uint32_t idk_case = 0;
	enum dovi_format out_fmt = 0;


	p_ut_info = &dv_ut_info;
	p_layer = &(dv_ut_info.dv_layer[DV_FHD_FE]);
	if (p_layer->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	idk_case = p_layer->case_id;
	out_fmt = p_ut_info->out_format;

	dovi_default("%s idk_case %d outfmt %d\n", __func__,
		idk_case, out_fmt);

	switch (idk_case) {
	case 5005://480p
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5005_ipt_sdr_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_ipt_sdr_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5005_ipt_ipt_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5005_ipt_ipt_g_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_ipt_ipt_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5004://720
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5004_ipt_sdr_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_ipt_sdr_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5004_ipt_ipt_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5004_ipt_ipt_g_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_ipt_ipt_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5000://1080
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5000_ipt_sdr_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_ipt_sdr_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5000_ipt_ipt_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5000_ipt_ipt_g_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_ipt_ipt_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	case 5003://2160
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5003_ipt_sdr_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_ipt_sdr_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 124;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5003_ipt_ipt_g_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5003_ipt_ipt_g_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr = NULL;
			p_ut_info->dsys_ml_cmd_len = 0;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_ipt_ipt_g_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 132;
		}
		break;
	default:
		dovi_default("idk_case %d error\n", idk_case);
		break;
	}

	dovi_ut_config_adl_ml(FHD_HDR_CASE);

}

void dovi_ut_sub_hdr(void)
{
	struct dovi_unit_info_t *p_ut_info = NULL;
	struct dv_layer_info *p_layer = NULL;
	uint32_t idk_case = 0;
	enum dovi_format out_fmt = 0;


	p_ut_info = &dv_ut_info;
	p_layer = &(dv_ut_info.dv_layer[DV_SUB_FE]);
	if (p_layer->on != 1) {
		dovi_default("enable layer and vfy case not match\n");
		return;
	}

	idk_case = p_layer->case_id;
	out_fmt = p_ut_info->out_format;

	dovi_default("%s idk_case %d outfmt %d\n", __func__,
		idk_case, out_fmt);

	switch (idk_case) {
	case 5005://480p
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5005_ipt_sdr_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5005_ipt_sdr_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 60;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5005_ipt_ipt_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5005_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5005_ipt_ipt_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 64;
		}
		break;
	case 5004://720
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5004_ipt_sdr_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5004_ipt_sdr_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 60;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5004_ipt_ipt_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5004_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5004_ipt_ipt_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 64;
		}
		break;
	case 5000://1080
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5000_ipt_sdr_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5000_ipt_sdr_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 60;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5000_ipt_ipt_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5000_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5000_ipt_ipt_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 64;
		}
		break;
	case 5003://2160
		if (out_fmt == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5003_ipt_sdr_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5003_ipt_sdr_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 60;
		} else if (out_fmt == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5003_ipt_ipt_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5003_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5003_ipt_ipt_dsys_sub_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 64;
		}
		break;
	default:
		dovi_default("idk_case %d error\n", idk_case);
		break;
	}

	dovi_ut_config_adl_ml(SUB_HDR_CASE);

}

void dovi_ut_main_hdr(void)
{
	struct dovi_unit_info_t *p_ut_info = NULL;
	struct dv_layer_info *p_layer = NULL;
	uint32_t idk_case = 0;
	enum dovi_format out_format = 0;

	p_ut_info = &dv_ut_info;
	p_layer = &(p_ut_info->dv_layer[DV_MAIN_FE]);

	if (p_layer->on != 1) {
		dovi_default("enable layer and ut case not match\n");
		return;
	}

	idk_case = p_layer->case_id;
	out_format = p_ut_info->out_format;
	dovi_default("%s idk_case %d outfmt %d\n", __func__,
		idk_case, out_format);

	switch (idk_case) {
	case 5005://480p
		if (out_format == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5005_ipt_sdr_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5005_ipt_sdr_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 60;
		} else if (out_format == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5005_ipt_ipt_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5005_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5005_ipt_ipt_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5005_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 64;
		}
		break;
	case 5004://720
		if (out_format == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5004_ipt_sdr_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5004_ipt_sdr_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 60;
		} else if (out_format == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5004_ipt_ipt_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5004_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5004_ipt_ipt_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5004_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 64;
		}
		break;
	case 5000://1080
		if (out_format == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5000_ipt_sdr_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5000_ipt_sdr_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 60;
		} else if (out_format == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5000_ipt_ipt_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5000_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5000_ipt_ipt_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5000_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 64;
		}
		break;
	case 5003://2160
		if (out_format == DOVI_SDR) {
			p_layer->lut_addr = idk_ut_5003_ipt_sdr_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_len = 0;
			p_ut_info->md_addr = NULL;
			p_ut_info->dsys_ml_addr =
				idk_ut_5003_ipt_sdr_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_ipt_sdr_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 60;
		} else if (out_format == DOVI_STD) {
			p_layer->lut_addr = idk_ut_5003_ipt_ipt_v_lut;
			p_layer->lut_len = HDR_FE_LUT_SIZE;
			p_ut_info->md_addr = idk_ut_5003_ipt_ipt_md_lut;
			p_ut_info->md_len = HDR_MD_SIZE;
			p_ut_info->dsys_ml_addr =
				idk_ut_5003_ipt_ipt_dsys_ml_tbl;
			p_ut_info->dsys_ml_cmd_len = 384;

			p_ut_info->msys_ml_addr =
				idk_ut_5003_ipt_ipt_msys_ml_tbl;
			p_ut_info->msys_ml_cmd_len = 64;
		}
		break;
	default:
		dovi_default("idk_case %d error\n", idk_case);
		break;
	}

	dovi_ut_config_adl_ml(MAIN_HDR_CASE);


}
void dovi_set_ut_case(uint32_t case_id)
{
	switch (case_id) {
	case MAIN_HDR_CASE:
		dovi_ut_main_hdr();
		break;
	case SUB_HDR_CASE:
		dovi_ut_sub_hdr();
		break;
	case FHD_HDR_CASE:
		dovi_ut_fhd_hdr();
		break;
	case UHD_HDR_CASE:
		dovi_ut_uhd_hdr();
		break;
	case MAIN_FHD_MIX_CASE:
		dovi_ut_main_mix_fhd_hdr();
		break;
	case SUB_UHD_MIX_CASE:
		dovi_ut_sub_mix_uhd_hdr();
		break;
	case ALL_LAYER_MIX_CASE:
		dovi_ut_all_mix_hdr();
		break;
	default:
		dovi_default("error ut case\n");
	}

}

void dovi_set_ut_flush(void)
{
	disp_adl_isr();
	disp_ml_isr();
}

void dovi_slt_case(uint32_t slt_case)
{

	switch (slt_case) {
	case 0://1080p 4layer mix std output
		break;

	case 1://1080p 4layer mix sdr output
		break;

	case 2://2160 4layer mix std output
		break;

	case 3://2160 4layer mix sdr output
		break;
	default:
		dovi_default("slt case error\n");
		break;
	};
}


