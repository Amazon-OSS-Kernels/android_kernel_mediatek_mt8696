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

#define LOG_TAG "DOVI_MAIN"

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
#include "disp_dovi_tz_client.h"
#include "dovi_common_hal.h"
#include "disp_dovi_cmd.h"
#include "disp_dovi_vdo_fe_if.h"
#include "disp_dovi_gfx_fe_if.h"
#include "disp_dovi_be_if.h"
#include "disp_dovi_io.h"
#include "dovi_table.h"
#include "fmt_hal.h"
#include "vdout_sys_hal.h"
#include "disp_path.h"
#include "disp_irq.h"
#include "disp_vdp_if.h"
#include "disp_hdr_if.h"
#include "disp_dovi_unit_test.h"
struct disp_hw_resolution dovi_res = {
	.res_mode = 0xff,
};

char dovi_dts_nd_name[DOVI_DTS_ND_MAX_SIZE] = "mediatek,mt8696-hdr";

char *dovi_reg_base[DOVI_CORE_MAX];

struct device *dovi_dev;
static bool dovi_init_done;
uint32_t osd_enable;
bool dovi_mute;
struct dovi_out_info_t dovi_out_info;
bool priority_mode_change;
uint32_t dovi_idk_test;
uint32_t dovi_idk_file_id;
int ll_rgb_desired;
int dump_crycb;
int dump_bit_depth;
int dump_format;
int idk_dump_sub;
int idk_dump_fefifo;
int dump_big_file;
int dump_times;
bool dovi_logo_show;
uint32_t f_graphic_on;
char *uhd_fmt_reg_base;
char *uhd_pla_reg_base;
char *uhd_scl_reg_base;
char *fhd_fmt_reg_base;
char *fhd_pla_reg_base;
char *fhd_scl_reg_base;
char *vdout_reg_base;
uint32_t uhd_active_zone;
uint32_t fhd_active_zone;
bool fhd_scale_to_uhd;
int ll_format;
uint32_t idk_vsem;

enum input_format_t pip_fhd_format;
enum input_format_t pip_uhd_format;

uint32_t dovi_sdk_test;
uint32_t dovi_sdk_file_id;
uint32_t layer_info_set_by_cmd;
struct video_buffer_info hdr_test_layer[4];

const struct disp_hw_resolution disp_res_tbl[] = {
	{2200, 1125, 1920, 1080, 60, true, true, HDMI_VIDEO_1920x1080p_60Hz,
	 false},
	{4400, 2250, 3840, 2160, 60, true, true, HDMI_VIDEO_3840x2160P_60HZ,
	 false},
};

uint32_t idk_graphic_header[] = {
	0xE0000000,		/* color mode */
	0xFFC00000,		/* 23:0 -> 27:4 */
	0xE40001E0,		/* 1080p */
	0x00000000,
	0x00001000,
	0x00001000,
	0x00000001,		/* 3:2 -> 31:30  */
	0x51800000,		/* 24:23 -> 29:28  */
	0x00000000,
	0x04380780,
	0x00000438,
	0x00000780,
};

static int _dovi_parse_dev_node(void)
{
	struct device_node *np;
	uint32_t reg_value;
	char nd_name[DOVI_DTS_ND_MAX_SIZE];

	sprintf(nd_name, "%s", dovi_dts_nd_name);

	np = of_find_compatible_node(NULL, NULL, nd_name);
	if (np == NULL) {
		dovi_error("dts error, no device node %s.\n", nd_name);
		return DOVI_STATUS_ERROR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);

	/* get fhd fe reg base 0x14009000 -0x14009A00 */
	dovi_reg_base[DOVI_FHD_FE] = (char *)of_iomap(np, 0);
	/* get uhd fe reg base 0x1400A000 -0x14009A00 */
	dovi_reg_base[DOVI_UHD_FE] = (char *)of_iomap(np, 1);
	/* get uhd fe reg base 0x1400B000 -0x1400B400 */
	dovi_reg_base[DOVI_VDO_BE] = (char *)of_iomap(np, 2);
	/* get mvdo fe reg base 0x15012000 -0x15013000 */
	dovi_reg_base[DOVI_MVDO_FE] = (char *)of_iomap(np, 3);
	/* get svdo fe reg base 0x15015000 -0x15016000 */
	dovi_reg_base[DOVI_SVDO_FE] = (char *)of_iomap(np, 4);

	dovi_printf("dovi regbase 0x%p,0x%p,0x%p,0x%p,0x%p",
		dovi_reg_base[DOVI_FHD_FE],
		dovi_reg_base[DOVI_UHD_FE],
		dovi_reg_base[DOVI_VDO_BE],
		dovi_reg_base[DOVI_MVDO_FE],
		dovi_reg_base[DOVI_SVDO_FE]);

	/* for idk cert */
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-osd");
	if (np == NULL) {
		dovi_error("dts error, no osd device node.\n");
		return DOVI_STATUS_ERROR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);
	/*osd2: uhd 0x14004000 */
	uhd_fmt_reg_base = (char *)of_iomap(np, 0);
	uhd_pla_reg_base = uhd_fmt_reg_base + 0x100;
	uhd_scl_reg_base = uhd_fmt_reg_base + 0x200;

	/*osd3: 0x14003000 */
	fhd_fmt_reg_base = (char *)of_iomap(np, 1);
	fhd_pla_reg_base = fhd_fmt_reg_base + 0x100;
	fhd_scl_reg_base = fhd_fmt_reg_base + 0x200;


	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-fmt");
	if (np == NULL) {
		dovi_error("dts error, no osd device node.\n");
		return DOVI_STATUS_ERROR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);

	vdout_reg_base = (char *)of_iomap(np, 1);

	return DOVI_STATUS_OK;
}

int disp_dovi_resolution_change(const struct disp_hw_resolution *info)
{
	if (info == NULL) {
		dovi_error("%s error params\n", __func__);
		return -1;
	}

	if (dovi_res.res_mode != info->res_mode)
		dovi_res = *info;

	dovi_set_out_res(info->res_mode, info->width, info->height);

	return DOVI_STATUS_OK;
}

void disp_dovi_init_sec_by_cmd(void)
{
	dovi_sec_init();
	disp_dovi_common_init();
	dovi_default("dovi sec init by cmd\n");
}

void dovi_update_gfx_fe_setting(void)
{
	uint16_t width = 0;
	uint16_t height = 0;
	uint16_t regidx = 0;

	width = dovi_res.width;
	height = dovi_res.height;

	for (regidx = 0; regidx < DOVI_GFX_FE_REG_NUM; regidx++)
		Dv_WriteREG((dovi_reg_base[DOVI_FHD_FE] + 0x200 + regidx*4),
		dovi_v26_1080p_sdr_all_gfxfe_reg[regidx]);

	//change fe width by res
	Dv_WriteREG(dovi_reg_base[DOVI_FHD_FE] + 0x3d0, height);

	for (regidx = 0; regidx < DOVI_GFX_FE_REG_NUM; regidx++)
		Dv_WriteREG((dovi_reg_base[DOVI_UHD_FE] + 0x200 + regidx*4),
		dovi_v26_1080p_sdr_all_gfxfe_reg[regidx]);

	//change fe width by res
	Dv_WriteREG(dovi_reg_base[DOVI_UHD_FE] + 0x3d0, height);


	for (regidx = 0; regidx < DOVI_BE_REG_NUM; regidx++) {
		if (g_out_format == DOVI_FORMAT_DOVI) {
			Dv_WriteREG(dovi_reg_base[DOVI_VDO_BE] +
				0x200 + (regidx*4),
				dovi_v26_1080p_sdr_ipt_be_reg[regidx]);
		} else if (g_out_format == DOVI_FORMAT_HDR10) {
			Dv_WriteREG(dovi_reg_base[DOVI_VDO_BE] +
				0x200 + (regidx*4),
				dovi_v26_1080p_sdr_hdr10_be_reg[regidx]);
		} else if (g_out_format == DOVI_FORMAT_HLG) {
			Dv_WriteREG(dovi_reg_base[DOVI_VDO_BE] +
				0x200 + (regidx*4),
				dovi_v26_1080p_sdr_hlg_be_reg[regidx]);
		} else if (g_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY) {
			Dv_WriteREG(dovi_reg_base[DOVI_VDO_BE] +
				0x200 + (regidx*4),
				dovi_v26_1080p_sdr_ll_be_reg[regidx]);
		} else if (g_out_format == DOVI_FORMAT_SDR) {
			Dv_WriteREG(dovi_reg_base[DOVI_VDO_BE] +
				0x200 + (regidx*4),
				dovi_v26_1080p_sdr_sdr_be_reg[regidx]);
		} else if (g_out_format == DOVI_FORMAT_VSEM_DOVI) {
			Dv_WriteREG(dovi_reg_base[DOVI_VDO_BE] +
				0x200 + (regidx*4),
				dovi_v26_1080p_sdr_vsem_be_reg[regidx]);
		} else if (g_out_format == DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY) {
			Dv_WriteREG(dovi_reg_base[DOVI_VDO_BE] +
				0x200 + (regidx*4),
				dovi_v26_1080p_sdr_vsemll_be_reg[regidx]);
		}
	}
	WriteREG32(dovi_reg_base[DOVI_VDO_BE] + 0x3d0, height);

	dovi_printf(" %s width %d height %d end\n", __func__, width, height);


}

int disp_dovi_gfx_set_defaut_setting(uint32_t id)
{
	uint32_t *p_gfx_fe_lut = NULL;
	uint32_t height = dovi_res.height;
	uint32_t regidx = 0;
	char *reg_base = NULL;

	if (!dovi_path_en)
		return 0;

	switch (dovi_out_format) {
	case DOVI_FORMAT_DOVI:
		p_gfx_fe_lut = dovi_sdr_ipt_gfx_fe_lut;
		break;
	case DOVI_FORMAT_DOVI_LOW_LATENCY:
		p_gfx_fe_lut = dovi_sdr_ll_gfx_fe_lut;
		break;
	case DOVI_FORMAT_HDR10:
		p_gfx_fe_lut = dovi_sdr_hdr10_gfx_fe_lut;
		break;
	case DOVI_FORMAT_HLG:
		p_gfx_fe_lut = dovi_sdr_hlg_gfx_fe_lut;
		break;
	case DOVI_FORMAT_SDR:
		p_gfx_fe_lut = dovi_sdr_sdr_gfx_fe_lut;
		break;
	case DOVI_FORMAT_VSEM_DOVI:
		p_gfx_fe_lut = dovi_sdr_vsem_gfx_fe_lut;
		break;
	case DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY:
		p_gfx_fe_lut = dovi_sdr_vsemll_gfx_fe_lut;
		break;
	default:
		break;
	}


	disp_fefifo_drv_set_input_order(id + 2, 5);
	dovi_gfx_fe_config_lut(id, p_gfx_fe_lut);

	if (id == 0)
		reg_base = dovi_reg_base[DOVI_FHD_FE];
	else
		reg_base = dovi_reg_base[DOVI_UHD_FE];

	for (regidx = 0; regidx < DOVI_GFX_FE_REG_NUM; regidx++)
		Dv_WriteREG((reg_base + 0x200 + regidx*4),
		dovi_v26_1080p_sdr_all_gfxfe_reg[regidx]);

	//change fe width by res
	Dv_WriteREG(reg_base + 0x3d0, height);
	Dv_WriteREG(reg_base + 0x3c4, adl_mode);

	dovi_printf("%s %d %d\n", __func__, height, dovi_out_format);

	return 0;
}

int disp_dovi_force_gfx_vs10(void)
{
	uint32_t *p_gfx_fe_lut = NULL;
	uint32_t *p_be_scm_lut = NULL;
	uint32_t *p_reg_tbl = NULL;
	uint32_t reg_tbl_len = 0;
	uint32_t width = dovi_res.width;

	dovi_out_format = (enum dovi_signal_format_t)g_out_format;
	dovi_path_en = 1;

	switch (g_out_format) {
	case DOVI_FORMAT_DOVI:
		p_gfx_fe_lut = dovi_sdr_ipt_gfx_fe_lut;
		p_be_scm_lut = dovi_sdr_ipt_be_lut;
		if (width > 0 && width <= 720)
			p_reg_tbl = dv_480p_sdr_ipt_mmsys_reg_tbl;
		else if (width > 720 && width <= 1280)
			p_reg_tbl = dv_720p_sdr_ipt_mmsys_reg_tbl;
		else if (width > 1280 && width <= 1920)
			p_reg_tbl = dv_1080p_sdr_ipt_mmsys_reg_tbl;
		else if (width > 1920 && width <= 4096)
			p_reg_tbl = dv_2160p_sdr_ipt_mmsys_reg_tbl;

		reg_tbl_len = 228;
		break;
	case DOVI_FORMAT_DOVI_LOW_LATENCY:
		p_gfx_fe_lut = dovi_sdr_ll_gfx_fe_lut;

		if (width > 0 && width <= 720)
			p_reg_tbl = dv_480p_sdr_ll_mmsys_reg_tbl;
		else if (width > 720 && width <= 1280)
			p_reg_tbl = dv_720p_sdr_ll_mmsys_reg_tbl;
		else if (width > 1280 && width <= 1920)
			p_reg_tbl = dv_1080p_sdr_ll_mmsys_reg_tbl;
		else if (width > 1920 && width <= 4096)
			p_reg_tbl = dv_2160p_sdr_ll_mmsys_reg_tbl;
		reg_tbl_len = 212;
		break;
	case DOVI_FORMAT_HDR10:
		p_gfx_fe_lut = dovi_sdr_hdr10_gfx_fe_lut;

		if (width > 0 && width <= 720)
			p_reg_tbl = dv_480p_sdr_hdr_mmsys_reg_tbl;
		else if (width > 720 && width <= 1280)
			p_reg_tbl = dv_720p_sdr_hdr_mmsys_reg_tbl;
		else if (width > 1280 && width <= 1920)
			p_reg_tbl = dv_1080p_sdr_hdr_mmsys_reg_tbl;
		else if (width > 1920 && width <= 4096)
			p_reg_tbl = dv_2160p_sdr_hdr_mmsys_reg_tbl;
		reg_tbl_len = 212;
		break;
	case DOVI_FORMAT_HLG:
		p_gfx_fe_lut = dovi_sdr_hlg_gfx_fe_lut;
		break;
	case DOVI_FORMAT_VSEM_DOVI:
		p_gfx_fe_lut = dovi_sdr_vsem_gfx_fe_lut;
		break;
	case DOVI_FORMAT_VSEM_DOVI_LOW_LATENCY:
		p_gfx_fe_lut = dovi_sdr_vsemll_gfx_fe_lut;
		break;
	case DOVI_FORMAT_SDR:
		p_gfx_fe_lut = dovi_sdr_sdr_gfx_fe_lut;
		if (width > 0 && width <= 720)
			p_reg_tbl = dv_480p_sdr_sdr_mmsys_reg_tbl;
		else if (width > 720 && width <= 1280)
			p_reg_tbl = dv_720p_sdr_sdr_mmsys_reg_tbl;
		else if (width > 1280 && width <= 1920)
			p_reg_tbl = dv_1080p_sdr_sdr_mmsys_reg_tbl;
		else if (width > 1920 && width <= 4096)
			p_reg_tbl = dv_2160p_sdr_sdr_mmsys_reg_tbl;

		reg_tbl_len = 212;
		break;
	default:
		break;
	}

	dovi_gfx_fe_hal_set_enable(LAYER0, true);
	dovi_gfx_fe_hal_set_enable(LAYER1, true);
	dovi_be_hal_set_enable(true);
	disp_fefifo_drv_set_input_order(2, 5);
	disp_fefifo_drv_set_input_order(3, 5);

	dovi_gfx_fe_config_lut(LAYER0, p_gfx_fe_lut);
	dovi_gfx_fe_config_lut(LAYER1, p_gfx_fe_lut);

	if (g_out_format == DOVI_FORMAT_DOVI)
		dovi_be_config_lut(p_be_scm_lut);

	dovi_update_gfx_fe_setting();

	//dovi_mmsys_update_reg(p_reg_tbl, reg_tbl_len);

	dovi_printf("%s %d %d\n", __func__, width, g_out_format);

	return 0;
}

int disp_dovi_init(struct disp_hw_common_info *info)
{
	if (info == NULL) {
		dovi_error("%s error params\n", __func__);
		return -1;
	}

	dovi_dev = disp_hw_mgr_get_dev();

	disp_dovi_resolution_change(info->resolution);

	/*parser dts file for register base */
	_dovi_parse_dev_node();

	dovi_debug_init();

	dovi_vdo_fe_init(dovi_reg_base);
	dovi_gfx_fe_init(dovi_reg_base);
	dovi_be_init(dovi_reg_base);

	dovi_sec_init();

	disp_dovi_common_init();

	osd_enable = 0;
	dovi_idk_test = 0;
	dovi_idk_file_id = 0;
	dovi_init_done = true;
	return DOVI_STATUS_OK;
}

void disp_dovi_isr(void)
{
	/* vdo_fe isr */
	dovi_vdo_fe_hal_isr();

	/* gfx_fe isr */
	dovi_gfx_fe_hal_isr();

	/* be isr */
	dovi_be_hal_isr();

}

int disp_dovi_suspend(void)
{
	if (ui_force_hdr_type)
		dovi_default("suspend in dovi path\n");

	return DOVI_STATUS_OK;
}

int disp_dovi_resume(void)
{
	if (ui_force_hdr_type) {
		dovi_vs10_path_en = 0;
		dovi_enable = 0;
		dovi_default("resume in dovi path\n");
	} else
		dovi_default("resume in normal path\n");

	return DOVI_STATUS_OK;
}

int disp_dovi_deinit(void)
{
	dovi_func();
	dovi_init_done = false;
	dovi_sec_md_parser_uninit();
	return DOVI_STATUS_OK;
}

int disp_dovi_show_tv_cap(struct disp_hw_tv_capbility *cap)
{
	if (cap) {
		dovi_default("support_hdr %d\n", cap->is_support_hdr);
		dovi_default("support_dovi %d\n", cap->is_support_dovi);
		dovi_default("support_dovi_4k60p %d\n",
			cap->is_support_dovi_2160p60);
		dovi_default("support_601 %d\n", cap->is_support_601);
		dovi_default("support_709 %d\n", cap->is_support_709);
		dovi_default("support_2020 %d\n", cap->is_support_bt2020);
	} else
		dovi_error("cap pointer is null %p\n", cap);

	return DOVI_STATUS_OK;
}

int disp_dovi_show_res_status(enum HDMI_VIDEO_RESOLUTION res)
{
	if (res >= HDMI_VIDEO_RESOLUTION_NUM)
		return -1;

	dovi_default("htotal %d vtotal %d\n",
		     dobly_resolution_table[res].htotal,
		     dobly_resolution_table[res].vtotal);
	dovi_default("width %d height %d\n",
		     dobly_resolution_table[res].width,
		     dobly_resolution_table[res].height);
	dovi_default("frequency %d progressive %d\n",
		     dobly_resolution_table[res].frequency,
		     dobly_resolution_table[res].is_progressive);
	dovi_default("hd %d mod %d\n",
		     dobly_resolution_table[res].is_hd,
		     dobly_resolution_table[res].res_mode);
	dovi_default("output resolution = %s\n", dobly_resstr[res]);

	return DOVI_STATUS_OK;
}

int disp_dovi_show_dev_status(void)
{
	dovi_default("dovi_dev %p %d\n", dovi_dev, g_dovi_efuse);

	dovi_default("dovi device node name %s\n", dovi_dts_nd_name);

	dovi_default("VDOFE%d base=0x%p 0x%p\n",
		DOVI_MVDO_FE,
		dovi_reg_base[DOVI_MVDO_FE],
		dovi_reg_base[DOVI_SVDO_FE]);
	dovi_default("GFXFE%d base=0x%p 0x%p\n",
		DOVI_FHD_FE,
		dovi_reg_base[DOVI_FHD_FE],
		dovi_reg_base[DOVI_UHD_FE]);
	dovi_default("BE%d base=0x%p\n",
		DOVI_VDO_BE,
		dovi_reg_base[DOVI_VDO_BE]);

	return DOVI_STATUS_OK;
}

int disp_dovi_status(void)
{
	dovi_func();
	if (!tv_info_set_by_cmd)
		disp_hw_mgr_get_info(&hdr_common_info);
	disp_dovi_show_tv_cap(&hdr_common_info.tv);
	disp_dovi_show_res_status(dovi_res.res_mode);

	disp_dovi_show_dev_status();

	dovi_vdo_fe_status(dovi_reg_base);
	dovi_gfx_fe_status(dovi_reg_base);
	dovi_be_status(dovi_reg_base);

	return DOVI_STATUS_OK;
}
int disp_dovi_dump_hdr_info(struct mtk_disp_hdr_md_info_t *hdr_info)
{
	struct mtk_vdp_dovi_md_t *dovi_info = NULL;
	bool dump_enable = false;
	uint8_t *buff = NULL;
	uint32_t rpu_len = 0;

	if (hdr_info->dr_range == DISP_DR_TYPE_DOVI) {
		dovi_info = &hdr_info->metadata_info.dovi_metadata;
		buff = dovi_info->buff;
		rpu_len = dovi_info->len;

		dovi_info("dump info rpu pts %lld len %d addr %p\n",
			  dovi_info->pts, dovi_info->len, dovi_info->buff);

		if (dovi_info->len != 0) {
			uint32_t *addr = (uint32_t *) dovi_info->buff;

			dovi_info("dovi rpu value 0x%X 0x%X 0x%X 0x%X\n",
				  addr[0], addr[1], addr[2], addr[3]);
		}

		dump_enable = true;

	}

	disp_dovi_dump_rpu(dump_enable, buff, rpu_len);

	return DOVI_STATUS_OK;
}

void disp_dovi_set_hdr_enable(uint32_t enable, uint32_t outformat)
{
	set_hdmi_info(outformat, &disp_common_info.tv, enable);
}

uint32_t disp_dovi_set_idk_info(void)
{
	if (dovi_idk_test) {
		uint32_t out_format = 0;
		int use_ll = 0;
		//uint32_t dovi2hdr10_mapping = 0;
		char *vsvdb_file_name = NULL;
		enum pri_mode_t priority_mode = 0;
		char *graphic_file_name = NULL;
		uint32_t graphic_file_size = 0;
		uint32_t dovi_idk_input_type = 0;
		uint32_t dovi_idk_output_type = 0;
		char *drm_file_name = NULL;
		char *vsif_file_name = NULL;
		char *vsem_file_name = NULL;
		uint32_t dovi_input_type1 = 0;
		uint32_t dovi_input_type2 = 0;
		char *vsif_file_name1 = NULL;
		char *vsif_file_name2 = NULL;
		char *rpu_file_name = NULL;

		struct disp_hw *hdr_drv =
			disp_hdr_get_drv();

		fhd_color_format = CP_CLR_RGB;
		uhd_color_format = CP_CLR_RGB;
		fhd_scale_to_uhd = 0;
		out_format = get_dovi_out_format(dovi_idk_file_id);
		use_ll = get_dovi_use_ll(dovi_idk_file_id);
		ll_rgb_desired = get_dovi_ll_rgb_desired(dovi_idk_file_id);
		vsvdb_file_name = get_dovi_vsvdb_file_name(dovi_idk_file_id);

		/* HDR10 dump 10bit */
		if (out_format == DOVI_FORMAT_HDR10
			|| out_format == DOVI_FORMAT_HLG) {
			idk_dump_bpp = VIDEOIN_BITMODE_10;
		} else {
			/* low latency mode, use hdr10 output dump */
			if (use_ll == 1)
				idk_dump_bpp = VIDEOIN_BITMODE_12;
			else
				idk_dump_bpp = VIDEOIN_BITMODE_8;
		}
		if (be_bypass_dither)
			idk_dump_bpp = VIDEOIN_BITMODE_12;

		dovi_set_output_format(out_format);
		dovi_set_low_latency_mode(use_ll, ll_rgb_desired);
		//dovi_set_dovi2hdr10_mapping(dovi2hdr10_mapping);
		dovi_set_vsvdb_file_name(vsvdb_file_name);

		idk_vdo_en = get_dovi_disp_mode(dovi_idk_file_id);
		priority_mode = get_dovi_priority_mode(dovi_idk_file_id);
		dovi_set_priority_mode(priority_mode);

		dovi_default("%s %d pri %d\n",
			__func__, idk_vdo_en, priority_mode);

		if (idk_vdo_en == NORMAL_MODE) {
			//case 50**--53**
			idk_vdo_pts[0] = 0;
			dovi_idk_input_type = get_dovi_input_type(
				dovi_idk_file_id);
			dovi_idk_output_type = get_dovi_output_type(
				dovi_idk_file_id);
			if (dovi_idk_input_type == VDO_TYPE_HDMI1P4)
				drm_file_name = get_dovi_drm_file_name(
					dovi_idk_file_id);
			else if (dovi_idk_input_type == VDO_TYPE_HDMI2P0)
				vsif_file_name = get_dovi_vsif_file_name(
					dovi_idk_file_id);
			else if (dovi_idk_input_type == VDO_TYPE_HDMI2P1) {
				vsif_file_name = get_dovi_vsif_vsem_name1(
					dovi_idk_file_id);
				vsem_file_name = get_dovi_vsif_vsem_name2(
					dovi_idk_file_id);
			}

			if (dovi_idk_file_id == 5021 ||
				dovi_idk_file_id == 5023 ||
				dovi_idk_file_id == 5024 ||
				dovi_idk_file_id == 5025)
				rpu_file_name = get_dovi_rpu_name(
					dovi_idk_file_id);

			if (get_dovi_is_only_gfx_test(dovi_idk_file_id) == 1) {
				idk_gfx_en = 1;
				no_mix_fhd = 1;
				disp_dovi_set_only_gfx_idk_info();
			} else {
				//idk use vdo layer, don't stop in advance
				f_graphic_on = get_dovi_gfx_on_info(
					dovi_idk_file_id);
				dovi_set_graphic_info(f_graphic_on);
				dovi_set_graphic_format(GRAPHIC_SDR_RGB);

				disp_dovi_idk_dump_frame_start(
					dovi_idk_file_id);

				if (f_graphic_on) {
					idk_gfx_en = 1;
					no_mix_uhd = 1;
					graphic_file_name =
						get_dovi_gfx_file_info(
							dovi_idk_file_id);
					if (graphic_file_name == NULL)
						return DOVI_STATUS_ERROR;
					/*set osd load dobly graphic for show */
					if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
						dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;
					graphic_file_size =
		dobly_resolution_table[dovi_res.res_mode].width
		* dobly_resolution_table[dovi_res.res_mode].height * 4;
					disp_dovi_set_osd_clk_enable(true);
					disp_dovi_load_buffer(graphic_file_name,
				(uint8_t *)graphic_idk_load_addr_va,
						graphic_file_size);
					disp_dovi_set_graphic_header(
						graphic_header_idk_load_addr_va,
						graphic_idk_dump_load_pa);
				} else {
					//no gfx case
					no_mix_fhd = 1;
					no_mix_uhd = 1;
				}
			}
		} else {
			//case 54**
			idk_gfx_en = 2;
			idk_vdo_pts[0] = 0;
			idk_vdo_pts[1] = 0;
			no_mix_fhd = 0;
			no_mix_uhd = 0;

			dovi_input_type1 = get_multi_input_type1(
				dovi_idk_file_id);
			dovi_input_type2 = get_multi_input_type2(
				dovi_idk_file_id);
			if (dovi_input_type1 == VDO_TYPE_HDMI2P0)
				vsif_file_name1 = get_dovi_multi_vsif1(
					dovi_idk_file_id);
			if (dovi_input_type2 == VDO_TYPE_HDMI2P0)
				vsif_file_name2 = get_dovi_multi_vsif2(
					dovi_idk_file_id);
			disp_dovi_set_pip_gfx1_info();
			disp_dovi_set_pip_gfx2_info();
		}
		hdr_drv->drv_call(DISP_CMD_OSD_UPDATE,
			&idk_vdo_en);
		disp_dovi_idk_dump_vin(true,
			VIDEOIN_SRC_SEL_VDO_BE_FIFO_OUTPUT,
			VIDEOIN_FORMAT_444);
	}
	return DOVI_STATUS_OK;
}

void disp_dovi_idk_gfx_handle(uint32_t enable)
{
	struct disp_hw *hdr_drv =
		disp_hdr_get_drv();

	hdr_drv->drv_call(DISP_CMD_OSD_UPDATE,
		&enable);
	disp_dovi_idk_set_uhd(true);
}

uint32_t disp_dovi_set_only_gfx_idk_info(void)
{
	if (dovi_idk_test) {
		enum pri_mode_t priority_mode = 0;
		char *graphic_file_name = NULL;
		uint32_t graphic_file_size = 0;

		dovi_default("xiao only gfx test id %d\n",
			dovi_idk_file_id);

		graphic_file_name =
			get_dovi_only_gfx_file_info(dovi_idk_file_id);

		f_graphic_on = 1;
		priority_mode = G_PRIORITY;
		dovi_set_graphic_info(f_graphic_on);
		dovi_set_priority_mode(priority_mode);

		//disp_dovi_alloc_graphic_buffer();
		disp_dovi_idk_dump_frame_start(dovi_idk_file_id);

		/*set osd load dobly graphic for show */
		if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
			dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;
		graphic_file_size =
		dobly_resolution_table[dovi_res.res_mode].width
		    * dobly_resolution_table[dovi_res.res_mode].height * 4;

		if (f_graphic_on && graphic_file_name != NULL) {
			disp_dovi_set_osd_clk_enable(true);
			disp_dovi_load_buffer(graphic_file_name,
				(uint8_t *)graphic_idk_load_addr_va,
				graphic_file_size);
			disp_dovi_set_graphic_header(
				graphic_header_idk_load_addr_va,
				graphic_idk_dump_load_pa);

			disp_dovi_idk_gfx_handle(true);
		}

	}
	return DOVI_STATUS_OK;
}

void disp_dovi_videoin_gfx(void)
{
	disp_dovi_idk_dump_vin(true,
		VIDEOIN_SRC_SEL_VDO_BE_FIFO_OUTPUT,
		VIDEOIN_FORMAT_444);
	dovi_default("xiao dovi_res.width = %d, dovi_res.height =%d\n",
		dovi_res.width, dovi_res.height);
}

void disp_dovi_videoin_off(void)
{
	videoin_hal_enable(false);
}

void disp_dovi_videoin_off_gfx(void)
{
	videoin_hal_enable(false);
}

void disp_dovi_dump_gfx(void)
{
	disp_dovi_idk_dump_gfx_frame();
}

void disp_dovi_set_pip_gfx1_info(void)
{
	char *graphic_file_name = NULL;
	uint32_t graphic_file_size = 0;

	graphic_file_name =
		get_dovi_pip_gfx1_gfx_file(dovi_idk_file_id);
	f_graphic_on = 1;
	pip_fhd_format = get_dovi_pip_gfx1_input_format(dovi_idk_file_id);

	dovi_set_graphic_info(f_graphic_on);
	if ((pip_fhd_format == FORMAT_HDR) ||
		(pip_fhd_format == FORMAT_DOVI)) {
		dovi_set_graphic_format(CP_CLR_YUV);
		fhd_color_format = CP_CLR_YUV;
	} else {
		dovi_set_graphic_format(CP_CLR_RGB);
		fhd_color_format = CP_CLR_RGB;
	}

	disp_dovi_idk_dump_frame_start(dovi_idk_file_id);
	disp_dovi_alloc_graphic2_buffer();

	/*set osd load dobly graphic for show */
	if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
		dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;
	if (fhd_scale_to_uhd)
		graphic_file_size = 1920*1080*4;
	else
		graphic_file_size =
		dobly_resolution_table[dovi_res.res_mode].width
			* dobly_resolution_table[dovi_res.res_mode].height * 4;

	if (f_graphic_on && graphic_file_name != NULL) {
		disp_dovi_set_osd_clk_enable(true);
		disp_dovi_load_buffer(graphic_file_name,
			(uint8_t *)graphic2_idk_load_addr_va,
			graphic_file_size);
		disp_dovi_set_graphic2_header(
			graphic2_header_idk_load_addr_va,
			graphic2_idk_dump_load_pa);
	}

	if (pip_fhd_format == FORMAT_DOVI)
		dovi_set_gfx_rpu_info();

}

void disp_dovi_set_pip_gfx2_info(void)
{
	char *graphic_file_name = NULL;
	uint32_t graphic_file_size = 0;

	graphic_file_name =
		get_dovi_pip_gfx2_gfx_file(dovi_idk_file_id);
	pip_uhd_format = get_dovi_pip_gfx2_input_format(dovi_idk_file_id);

	dovi_set_graphic_info_uhd(f_graphic_on);
	if ((pip_uhd_format == FORMAT_HDR) ||
		(pip_uhd_format == FORMAT_DOVI)) {
		dovi_set_uhd_graphic_format(CP_CLR_YUV);
		uhd_color_format = CP_CLR_YUV;
	} else {
		dovi_set_uhd_graphic_format(CP_CLR_RGB);
		uhd_color_format = CP_CLR_RGB;
	}

	/*set osd load dobly graphic for show */
	if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
		dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;
	graphic_file_size =
	dobly_resolution_table[dovi_res.res_mode].width
		* dobly_resolution_table[dovi_res.res_mode].height * 4;

	if (f_graphic_on && graphic_file_name != NULL) {
		//disp_dovi_set_osd_clk_enable(true);
		disp_dovi_load_buffer(graphic_file_name,
			(uint8_t *)graphic_idk_load_addr_va,
			graphic_file_size);
		disp_dovi_set_graphic_header(
			graphic_header_idk_load_addr_va,
			graphic_idk_dump_load_pa);
	}
}

void disp_dovi_set_graphic2_header(uint32_t *va,
	dma_addr_t graphic_pa)
{
	uint32_t graphic_header_size = 48;
	uint32_t idx = 0;
	uint32_t width = 1920;
	uint32_t height = 1080;

	dovi_default("gen header2 va %p pa 0x%x\n", va,
		(uint32_t) graphic_pa);

	memcpy(va, idk_graphic_header,
		graphic_header_size);

	va[1] &= 0xFF000000;
	va[1] |= ((graphic_pa & 0x0FFFFFFF) >> 4);

	va[2] &= 0xFFFFF800;
	va[2] |= ((width * 4) >> 4);

	va[6] &= (~(0x3 << 2));
	va[6] |= (((graphic_pa
		& 0xC0000000) >> 30) << 2);

	va[7] &= (~(0x3 << 23));
	va[7] |= (((graphic_pa
		& 0x30000000) >> 28) << 23);

	va[9] &= 0x0;
	va[9] |= height << 16;
	va[9] |= width;

	va[10] &= 0x0;
	va[10] |= height;

	va[11] &= 0x0;
	va[11] |= width;

	for (idx = 0; idx < graphic_header_size / 4; idx++)
		dovi_default("2 va[%d] 0x%x\n", idx, va[idx]);
}

uint32_t disp_dovi_set_sdk_info(void)
{
	if (dovi_sdk_test) {
		enum pri_mode_t priority_mode;
		char *graphic_file_name = NULL;
		uint32_t graphic_file_size = 0;

		graphic_file_name =
			dovi_graphic_name[dovi_sdk_file_id];

		f_graphic_on = 1;
		priority_mode = G_PRIORITY;
		dovi_set_graphic_info(f_graphic_on);
		dovi_set_priority_mode(priority_mode);

		disp_dovi_alloc_graphic_buffer();

		/*set osd load dobly graphic for show */
		if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
			dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;
		graphic_file_size =
		dobly_resolution_table[dovi_res.res_mode].width
		    * dobly_resolution_table[dovi_res.res_mode].height * 4;

		if (!dovi_sdk_file_id)
			graphic_file_size = 1920 * 1080 * 4;

		if (f_graphic_on && graphic_file_name != NULL) {
			disp_dovi_set_osd_clk_enable(true);
			disp_dovi_load_buffer(graphic_file_name,
				(uint8_t *)graphic_idk_load_addr_va,
				graphic_file_size);
			disp_dovi_set_graphic_header(
				graphic_header_idk_load_addr_va,
				graphic_idk_dump_load_pa);

		}
	}

	disp_dovi_sdk_handle(dovi_sdk_test);

	return DOVI_STATUS_OK;
}

void disp_dovi_set_graphic_header(uint32_t *va,
	dma_addr_t graphic_pa)
{
	uint32_t graphic_header_size = 48;
	uint32_t idx = 0;
	uint32_t width = 0;
	uint32_t height = 0;

	if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
		dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;

	width = dobly_resolution_table[dovi_res.res_mode].width;
	height = dobly_resolution_table[dovi_res.res_mode].height;


	if (fhd_scale_to_uhd) {
		width = 1920;
		height = 1080;
	}
	dovi_default("gen header va %p pa 0x%x\n", va,
		(uint32_t) graphic_pa);

	memcpy(va, idk_graphic_header,
		graphic_header_size);

	va[1] &= 0xFF000000;
	va[1] |= ((graphic_pa & 0x0FFFFFFF) >> 4);

	va[2] &= 0xFFFFF800;
	va[2] |= ((width * 4) >> 4);

	va[6] &= (~(0x3 << 2));
	va[6] |= (((graphic_pa
		& 0xC0000000) >> 30) << 2);

	va[7] &= (~(0x3 << 23));
	va[7] |= (((graphic_pa
		& 0x30000000) >> 28) << 23);

	va[9] &= 0x0;
	va[9] |= height << 16;
	va[9] |= width;

	va[10] &= 0x0;
	va[10] |= height;

	va[11] &= 0x0;
	va[11] |= width;

	for (idx = 0; idx < graphic_header_size / 4; idx++)
		dovi_default("va[%d] 0x%x\n", idx, va[idx]);
}

void disp_dovi_set_osd_clk_enable(bool enable)
{
	if (enable) {
		disp_clock_enable(DISP_CLK_OSDPLL, true);
		disp_clock_smi_larb_en(DISP_SMI_LARB0, true);
		disp_clock_enable(DISP_CLK_OSD_UHD, true);
		disp_clock_enable(DISP_CLK_OSD_SEL, true);
		if (dovi_res.res_mode <= HDMI_VIDEO_1920x1080p_50Hz)
			disp_clock_select_pll(DISP_CLK_OSD_SEL,
			DISP_CLK_OSDPLL_D2);
		else
			disp_clock_select_pll(DISP_CLK_OSD_SEL,
			DISP_CLK_OSDPLL);
	} else {
		disp_clock_enable(DISP_CLK_OSDPLL, false);
		disp_clock_enable(DISP_CLK_OSD_SEL, false);
		disp_clock_smi_larb_en(DISP_SMI_LARB0, false);
		disp_clock_enable(DISP_CLK_OSD_UHD, false);
	}
}

void disp_dovi_set_osd_all_black(void)
{
	/* fgUpdate = 0 */
	Dv_WriteREGMsk(uhd_fmt_reg_base, 0x0, 0x1);
	Dv_WriteREGMsk(fhd_fmt_reg_base, 0x0, 0x1);
	/* active zone = 0 */
	uhd_active_zone = Dv_ReadREG(uhd_fmt_reg_base + 0x1c);
	fhd_active_zone = Dv_ReadREG(fhd_fmt_reg_base + 0x1c);
	dovi_default("read uhd/fhd_active_zone 0x%x 0x%x!\n",
		uhd_active_zone, fhd_active_zone);
	Dv_WriteREG(uhd_fmt_reg_base + 0x1c, 0x0);
	Dv_WriteREG(fhd_fmt_reg_base + 0x1c, 0x0);
	/* fgUpdate = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base, 0x1, 0x1);
	Dv_WriteREGMsk(fhd_fmt_reg_base, 0x1, 0x1);
}

void disp_dovi_set_osd_active_zone(void)
{
	/* fgUpdate = 0 */
	Dv_WriteREGMsk(uhd_fmt_reg_base, 0x0, 0x1);
	Dv_WriteREGMsk(fhd_fmt_reg_base, 0x0, 0x1);
	/* set active zone */
	dovi_default("write uhd/fhd_active_zone 0x%x 0x%x!\n",
	uhd_active_zone, fhd_active_zone);
	Dv_WriteREG(uhd_fmt_reg_base + 0x1c, uhd_active_zone);
	Dv_WriteREG(fhd_fmt_reg_base + 0x1c, fhd_active_zone);
	/* fgUpdate = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base, 0x1, 0x1);
	Dv_WriteREGMsk(fhd_fmt_reg_base, 0x1, 0x1);
}

void disp_dovi_set_osd_enable(bool enable)
{
	/* fgUpdate = 0 */
	Dv_WriteREGMsk(uhd_fmt_reg_base, 0x0, 0x1);
	Dv_WriteREGMsk(fhd_fmt_reg_base, 0x0, 0x1);
	if (dovi_sdk_test) {
		dovi_default("open fhd for sdk test\n");
		Dv_WriteREGMsk(uhd_pla_reg_base, 0x1, 0xFFF);
		Dv_WriteREGMsk(fhd_pla_reg_base, 0x1, 0xFFF);
		Dv_WriteREGMsk(fhd_pla_reg_base + 0x4,
			    graphic_header_idk_dump_load_pa >> 4,
			    0xFFFFFFFF);
	} else if (enable == true) {
		dovi_default("open uhd for idk cert %d\n",
			fhd_scale_to_uhd);
		//disp_dovi_set_osd_showdoblylogo();
		/* fgOsdEn = 1 */
		//Dv_WriteREGMsk(uhd_pla_reg_base, 0x1, 0x1);
		//Dv_WriteREGMsk(fhd_pla_reg_base, 0x0, 0x1);
		//set uhd scale params
		if (fhd_scale_to_uhd) {
			Dv_WriteREGMsk(uhd_scl_reg_base, 0x94, 0xFF);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
				0x07800438, 0xFFFFFFFF);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0xc,
				0x780, 0xFFF);
		}
		Dv_WriteREGMsk(uhd_pla_reg_base, 0x1, 0xFFF);
		Dv_WriteREGMsk(fhd_pla_reg_base, 0x1, 0xFFF);
		Dv_WriteREGMsk(uhd_pla_reg_base + 0x4,
			    graphic_header_idk_dump_load_pa >> 4,
			    0xFFFFFFFF);
		if (idk_gfx_en == 2) {
			Dv_WriteREGMsk(fhd_pla_reg_base + 0x4,
			    graphic2_header_idk_dump_load_pa >> 4,
			    0xFFFFFFFF);
			dovi_default("use fhd for idk cert\n");
		}
	} else {
		dovi_default("close uhd for idk cert\n");
		/* fgOsdEn = 0 */
		//Dv_WriteREGMsk(uhd_pla_reg_base, 0x0, 0x1);
		//Dv_WriteREGMsk(fhd_pla_reg_base, 0x0, 0x1);
	}
	/* fgUpdate = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base, 0x1, 0x1);
	Dv_WriteREGMsk(fhd_fmt_reg_base, 0x1, 0x1);
}


uint32_t disp_dovi_set_osd_showdoblylogo(void)
{
	uint32_t vtotal = 0;
	uint32_t htotal = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t osd_hstart = 0;
	uint32_t osd_veven = 0;
	uint32_t osd_vodd = 0;

	if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
		dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;

	vtotal = dobly_resolution_table[dovi_res.res_mode].vtotal;
	htotal = dobly_resolution_table[dovi_res.res_mode].htotal;
	width = dobly_resolution_table[dovi_res.res_mode].width;
	height = dobly_resolution_table[dovi_res.res_mode].height;

	/* fgAlwaysUpdate = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base,
	0x1 << 1, 0x1 << 1);
	/* fgOsd2Prgs = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x8,
	0x1 << 4, 0x1 << 4);

	/* u4VsWidthMain = 6, u4HsWidthMain = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0xC,
	0x6 << 12, 0x1FF << 12);
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0xC,
		0x1 << 22, 0x1FF << 22);

	/* u4AlphaSel = 0x1F */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0xAC,
	0x1F << 20, 0x1F << 20);

	/* u4OvtMain = vtotal, u4OhtMain = htotal */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0xC,
	vtotal, 0xFFF);
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x64,
		htotal, 0xFFF);

	/* u4ScrnHSizeMain = width, u4ScrnVSizeMain = height */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x1C,
	height, 0xFFF);
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x1C,
		width << 16, 0xFFF << 16);

	/* fgAutoSwEn = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x8,
	0x1 << 10, 0x1 << 10);

	/*if (dovi_idk_test)*/
		/*disp_path_set_delay(DISP_PATH_OSD1,*/
		/*dovi_res.res_mode);*/
	/*disp_path_get_active_zone(DISP_PATH_OSD1,*/
		/*dovi_res.res_mode,*/
		/*&osd_hstart, &osd_vodd, &osd_veven);*/

	/* u4ScrnHStartOsd2 = osd_hstart */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x10,
	osd_hstart, 0x3FF);

	/* u4ScrnVStart = osd_veven, VStart = osd_vodd */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x18,
	osd_veven, 0x1FFF);
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x18,
		osd_vodd << 16, 0x1FF << 16);

	/* u4Osd2HStart = 0, u4Osd2VStart = 0 */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x24, 0,
	0xFFFFFFFF);

	/* fgVsEdge = 1, fgHsEdge = 0 */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x8,
	0x1 << 1, 0x1 << 1);
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x8,
		0x0, 0x1);

	/* fg_source_sync_select = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base + 0x8,
	0x1 << 3, 0x1 << 3);

	/* burst_8_enable = 1 */
	Dv_WriteREGMsk(uhd_pla_reg_base + 0xAC,
	0x1 << 28, 0x1 << 28);


	if (fhd_scale_to_uhd) {
		/* fgScEn = 1 */
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x0, 0x94,
		0xFFFFFFFF);

		/* u4SrcHSize = width, u4SrcVSize = height */
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
		0x438, 0x1FFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
			0x780 << 16, 0x1FFF << 16);

		/* u4DstHSize = width, u4DstVSize = height */
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x8,
		height, 0x1FFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x8,
			width << 16, 0x1FFF << 16);

		/* u4VscHSize = width */
		Dv_WriteREGMsk(uhd_scl_reg_base + 0xC,
		0x780, 0x1FFF);

		Dv_WriteREGMsk(uhd_scl_reg_base + 0x10,
			0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x14,
			0x1FFD, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x18,
			0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x1C,
			0x1FFC, 0xFFFFFFFF);
	} else {
		/* fgScEn = 1 */
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x0,
		0x0, 0xFFFFFFFF);

		/* u4SrcHSize = width, u4SrcVSize = height */
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
		height, 0x1FFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
			width << 16, 0x1FFF << 16);

		/* u4DstHSize = width, u4DstVSize = height */
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x8,
		height, 0x1FFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x8,
			width << 16, 0x1FFF << 16);

		/* u4VscHSize = width */
		Dv_WriteREGMsk(uhd_scl_reg_base + 0xC,
		width, 0x1FFF);

		Dv_WriteREGMsk(uhd_scl_reg_base + 0x10,
			0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x14,
			0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x18,
			0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x1C,
			0x0, 0xFFFFFFFF);
	}
	return DOVI_STATUS_OK;
}

void disp_dovi_dump_vsem(void)
{
	uint32_t dovi_vsem_num_pks = 0;
	uint32_t dovi_hdmi_type = 0;
	uint8_t *dovi_hdmi_vsem_addr = NULL;
	uint32_t i = 0;

	dovi_hdmi_vsem_addr = disp_dovi_get_hdmi_vsem_info(
		&dovi_vsem_num_pks, &dovi_hdmi_type);

	for (i = 0; i < dovi_vsem_num_pks * 31; i++)
		dovi_printf("vsem [%d] = 0x%x\n", i, dovi_hdmi_vsem_addr[i]);
}

void dovi_idk_settings(uint32_t idk_set)
{
	int i = 0;
	struct disp_hw *hdr_drv =
		disp_hdr_get_drv();
	struct disp_hw *vdp_drv = disp_vdp_get_drv();

	//idk_set = 0, reset idk settings
	dovi_default("dovi_idk_test %d idk_set %d\n",
		     dovi_idk_test, idk_set);

	no_mix_fhd = 0;
	no_mix_uhd = 0;

	i = idk_vdo_en;
	idk_vdo_en = idk_set;
	idk_gfx_en = idk_set;
	hdr_drv->drv_call(DISP_CMD_OSD_UPDATE,
		&idk_set);
	if (i == 1) {
		idk_vdo_pts[0] = 0;
		vdp_drv->stop(0);
	} else {
		idk_vdo_pts[0] = 0;
		idk_vdo_pts[1] = 0;
		vdp_drv->stop(0);
		vdp_drv->stop(1);
	}

	dovi_idk_test = idk_set;
	f_graphic_on = idk_set;
	dovi_idk_dump = idk_set;
	dovi_logo_show = false;
	idk_vdo_start[0] = 0;
	idk_vdo_start[1] = 0;
	dovi_printf("dump idk_vdo_en= %d, idk_gfx_en=%d\n",
		idk_vdo_en, idk_gfx_en);
	disp_dovi_idk_dump_frame_end();
	//videoin_hal_clock(false);
}

void disp_dovi_idk_set_gfx(bool enable)
{
	uint32_t width = 1920;
	uint32_t height = 1080;
	uint32_t scale_w = 0;
	uint32_t scale_h = 0;

	if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
		dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;

	scale_w = dobly_resolution_table[dovi_res.res_mode].width;
	scale_h = dobly_resolution_table[dovi_res.res_mode].height;

	/* fgUpdate = 0 */
	Dv_WriteREGMsk(fhd_fmt_reg_base, 0x0, 0x1);
	if (enable == true) {
		dovi_default("open fhd for idk cert\n");
		dovi_default("fhd width = %d, height = %d\n", width, height);

		Dv_WriteREGMsk(fhd_pla_reg_base, 0x1, 0x1);
		Dv_WriteREGMsk(fhd_pla_reg_base + 0x4,
			    graphic_header_idk_dump_load_pa >> 4,
			    0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_pla_reg_base, 0x0, 0xF << 8);
		Dv_WriteREGMsk(fhd_fmt_reg_base + 0x8,
			0x412, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_fmt_reg_base + 0xC,
			0x406465, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_fmt_reg_base + 0x24,
		0x0, 0xFFFF);
		Dv_WriteREGMsk(fhd_fmt_reg_base + 0xA4,
			0x01FA06C0, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_fmt_reg_base + 0xCC,
			0xFFFFFFFF, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_pla_reg_base + 0xBC, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_pla_reg_base + 0xC0, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_pla_reg_base + 0xC4, 0x10000, 0xFFFFFFFF);
		if (scale_w == 3840)
			Dv_WriteREGMsk(fhd_scl_reg_base, 0x94, 0xFFFF);
		else
			Dv_WriteREGMsk(fhd_scl_reg_base, 0x80, 0xFFFF);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0x4,
		height, 0x1FFF);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0x4,
			width << 16, 0x1FFF << 16);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0x8,
			scale_h, 0x1FFF);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0x8,
			scale_w << 16, 0x1FFF << 16);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0xC,
		width, 0x1FFF);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0x10, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0x14, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0x18, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(fhd_scl_reg_base + 0x1C, 0x0, 0xFFFFFFFF);
	}
	/* fgUpdate = 1 */
	Dv_WriteREGMsk(fhd_fmt_reg_base, 0x3, 0x3);
}

void disp_dovi_idk_set_uhd(bool enable)
{
	uint32_t width = 1920;
	uint32_t height = 1080;

	if (dovi_res.res_mode >= HDMI_VIDEO_RESOLUTION_NUM)
		dovi_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;
	width = dobly_resolution_table[dovi_res.res_mode].width;
	height = dobly_resolution_table[dovi_res.res_mode].height;

	/* fgUpdate = 0 */
	Dv_WriteREGMsk(uhd_fmt_reg_base, 0x0, 0x1);
	if (enable == true) {
		dovi_default("open uhd for idk cert\n");
		dovi_default("uhd width = %d, height = %d\n", width, height);

		Dv_WriteREGMsk(uhd_pla_reg_base, 0x1, 0x1);
		Dv_WriteREGMsk(uhd_pla_reg_base + 0x4,
			    graphic_header_idk_dump_load_pa >> 4,
			    0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_pla_reg_base, 0x0, 0xF << 8);

		Dv_WriteREGMsk(uhd_fmt_reg_base + 0x8,
			0x412, 0xFFFFFFFF);
		if (width == 3840)
			Dv_WriteREGMsk(uhd_fmt_reg_base + 0xC,
			0x4038CA, 0xFFFFFFFF);
		else if (width == 1920)
			Dv_WriteREGMsk(uhd_fmt_reg_base + 0xC,
			0x406465, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_fmt_reg_base + 0x24,
			0x0, 0xFFFF);
		Dv_WriteREGMsk(uhd_fmt_reg_base + 0xA4,
			0x01FA06C0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_fmt_reg_base + 0xCC,
			0xFFFFFFFF, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_pla_reg_base + 0xBC, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_pla_reg_base + 0xC0, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_pla_reg_base + 0xC4, 0x10000, 0xFFFFFFFF);

		if (fhd_scale_to_uhd) {
			Dv_WriteREGMsk(uhd_scl_reg_base, 0x94, 0xFFFF);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
				1080, 0x1FFF);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
				1920 << 16, 0x1FFF << 16);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0xC,
				1920, 0x1FFF);
		} else {
			Dv_WriteREGMsk(uhd_scl_reg_base, 0x80, 0xFFFF);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
			height, 0x1FFF);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0x4,
				width << 16, 0x1FFF << 16);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0x8,
			height, 0x1FFF);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0x8,
				width << 16, 0x1FFF << 16);
			Dv_WriteREGMsk(uhd_scl_reg_base + 0xC,
			width, 0x1FFF);
		}

		Dv_WriteREGMsk(uhd_scl_reg_base + 0x10, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x14, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x18, 0x0, 0xFFFFFFFF);
		Dv_WriteREGMsk(uhd_scl_reg_base + 0x1C, 0x0, 0xFFFFFFFF);

	}
	/* fgUpdate = 1 */
	Dv_WriteREGMsk(uhd_fmt_reg_base, 0x3, 0x3);
}

int disp_dovi_idk_handle(uint32_t enable)
{
	struct disp_hw *hdr_drv =
		disp_hdr_get_drv();

	dovi_info("%s en %d idk_test %d logo_show %d\n",
		     __func__, enable,
		     dovi_idk_test, dovi_logo_show);
	if (!dovi_idk_test)
		return 0;

	if (dovi_idk_test) {
		if (idk_gfx_en == 2) {
			dovi_gfx_fe_hal_set_enable(0, true);
			dovi_gfx_fe_hal_set_enable(1, true);
		} else if (idk_gfx_en == 1) {
			no_mix_uhd = 1;
			dovi_gfx_fe_hal_set_enable(0, true);
			dovi_gfx_fe_hal_set_enable(1, false);
		} else {
			no_mix_fhd = 1;
			no_mix_uhd = 1;
			dovi_gfx_fe_hal_set_enable(0, false);
			dovi_gfx_fe_hal_set_enable(1, false);
		}
	}

	/* we must only show dobly logo when idk test */
	if (f_graphic_on) {
		if (enable && dovi_idk_test
			&& !dovi_logo_show) {
			//disp_dovi_set_osd_enable(true);
			disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE,
					true);
			disp_dovi_idk_set_gfx(true);
			dovi_gfx_fe_hal_set_enable(0, true);

			if (idk_gfx_en == 2) {
				disp_clock_enable(DISP_CLK_UHD_HDR_GFX_FE,
					true);
				disp_dovi_idk_set_uhd(true);
				dovi_gfx_fe_hal_set_enable(1, true);
			}
			dovi_logo_show = true;
		} else if (!enable && dovi_idk_test
		&& dovi_logo_show) {
			disp_dovi_set_osd_enable(false);
			disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE,
				false);
			dovi_gfx_fe_hal_set_enable(0, false);
			dovi_logo_show = false;
		}
	} else if (dovi_idk_test) {
		/* fgUpdate = 0 */
		Dv_WriteREGMsk(uhd_fmt_reg_base, 0x0, 0x1);
		Dv_WriteREGMsk(fhd_fmt_reg_base, 0x0, 0x1);
		/* fgOsdEn = 0 */
		Dv_WriteREGMsk(uhd_pla_reg_base, 0x0, 0x1);
		Dv_WriteREGMsk(fhd_pla_reg_base, 0x0, 0x1);
		/* fgUpdate = 1 */
		Dv_WriteREGMsk(uhd_fmt_reg_base, 0x1, 0x1);
		Dv_WriteREGMsk(fhd_fmt_reg_base, 0x1, 0x1);
	}

	/* when play done, we must close
	 *file and free mem
	 */
	if (!enable && dovi_idk_test) {
		idk_vdo_en = 0;
		idk_gfx_en = 0;
		dovi_printf("dump idk_vdo_en= %d, idk_gfx_en=%d\n",
			idk_vdo_en, idk_gfx_en);
		disp_dovi_idk_dump_frame_end();
		dovi_idk_test = 0;
		f_graphic_on = 0;
		dovi_idk_dump = 0;
		hdr_drv->drv_call(DISP_CMD_OSD_UPDATE,
			&enable);
	}
	return DOVI_STATUS_OK;
}

int disp_dovi_sdk_handle(uint32_t enable)
{
	struct disp_hw *hdr_drv =
		disp_hdr_get_drv();

	dovi_default("%s en %d sdk_test %dlogo_show %d\n",
		     __func__, enable,
		     dovi_sdk_test, dovi_logo_show);

	/* we must only show dobly logo when sdk test */
	if (enable && dovi_sdk_test && !dovi_logo_show) {
		hdr_drv->drv_call(DISP_CMD_OSD_UPDATE,
			&enable);

		disp_dovi_set_osd_enable(true);
		disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE, true);
		dovi_logo_show = true;
	} else if (!enable && !dovi_sdk_test
	&& dovi_logo_show) {
		//disp_dovi_set_osd_enable(false);
		//disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE, false);

		hdr_drv->drv_call(DISP_CMD_OSD_UPDATE,
			&enable);
		disp_dovi_free_graphic_buffer();
		dovi_logo_show = false;

		dovi_sdk_test = 0;
		f_graphic_on = 0;
	} else if (dovi_sdk_test) {
		/* fgUpdate = 0 */
		Dv_WriteREGMsk(uhd_fmt_reg_base, 0x0, 0x1);
		Dv_WriteREGMsk(fhd_fmt_reg_base, 0x0, 0x1);
		/* fgOsdEn = 0 */
		Dv_WriteREGMsk(uhd_pla_reg_base, 0x0, 0x1);
		Dv_WriteREGMsk(fhd_pla_reg_base, 0x0, 0x1);
		/* fgUpdate = 1 */
		Dv_WriteREGMsk(uhd_fmt_reg_base, 0x1, 0x1);
		Dv_WriteREGMsk(fhd_fmt_reg_base, 0x1, 0x1);
	}

	return DOVI_STATUS_OK;
}


int disp_dovi_process_cmd(uint32_t id, enum DISP_CMD cmd,
	void *data)
{
	struct mtk_disp_hdr_md_info_t *hdr_info;
	//uint32_t dovi_gpm;
	uint32_t value = 0;
	uint32_t layer_id = 0;
	uint32_t enable = 0;


	switch (cmd) {
	case DISP_CMD_METADATA_UPDATE:
		hdr_info =
			(struct mtk_disp_hdr_md_info_t *)data;
		if (dump_rpu_enable)
			disp_dovi_dump_hdr_info(hdr_info);

		if (hdr_info->enable == 0) {
			enable = 0;
			dovi_default("disable dv\n");
		} else
			enable = 1;

		disp_dovi_idk_handle(enable);

		/* if priority_mode_change, we must reinit */
		//dovi_get_priority_mode(&dovi_gpm);

		if (priority_mode_change
			&& (hdr_info->enable != 0)) {
			if (hdr_info->enable == 1)
				hdr_info->enable =
				DOVI_PRIORITY_MODE_CHANGE;
			priority_mode_change = false;
		} else if ((p_cp_param->priority_mode == V_PRIORITY)
		&& (osd_enable == 1) && (dovi_idk_test == 0)) {
			dovi_set_priority_mode(0);
			if (hdr_info->enable == 1)
				hdr_info->enable =
				DOVI_PRIORITY_MODE_CHANGE;
		} else if ((p_cp_param->priority_mode == G_PRIORITY)
		&& (osd_enable == 0) && (dovi_idk_test == 0)) {
			dovi_set_priority_mode(1);
			if (hdr_info->enable == 1)
				hdr_info->enable =
				DOVI_PRIORITY_MODE_CHANGE;
		}

		disp_dovi_process(enable, hdr_info);

		//if (idk_vsem)
		//	disp_dovi_dump_vsem();
		if (idk_dump_sub)
			dovi_idk_dump_vdo_bypass(0);
		break;

	case DISP_CMD_OSD_START:
		layer_id = *((uint32_t *) data);
		if (layer_id > 1) {
			dovi_error("osd layer overflow %d\n", layer_id);
			return DOVI_STATUS_ERROR;
		}
		dovi_default("osd(%d) %d %d %d %d %d %d enable\n",
			*((uint32_t *) data), osd_enable, vdp_start_st[0],
			vdp_start_st[1], cur_ml_cfg_st[layer_id+2],
			dv_gfx_fe_en[layer_id], dovi_hdr_md_info[0].dr_range);
		if (!dv_gfx_fe_en[layer_id]) {
			if (((dovi_out_info.out_format
				== DOVI_FORMAT_DOVI) ||
				(dovi_out_info.out_format
				== DOVI_FORMAT_VSEM_DOVI))
				&& (osd_enable == 0)) {
				dovi_out_info.b_gfx_mode =
					true;
				dovi_set_priority_mode(
					!dovi_out_info.b_gfx_mode);
				priority_mode_change =
					true;
			}
			dovi_gfx_fe_hal_set_enable(layer_id, true);
			//update gfx hdr fe when nonvideo to trig
			if (dovi_path_en && (vdp_start_st[0] == 0) &&
				(vdp_start_st[1] == 0) &&
				(cur_ml_cfg_st[layer_id+2] == 0)) {
				if ((dovi_vs10_path_en == 0) &&
					(g_force_dovi != 0)) {
					// force hdr not start
					disp_dovi_force_gfx_vs10();
				} else {
					disp_dovi_gfx_set_defaut_setting(
						layer_id);
					#if 0
					dovi_hdr_md_info[0].enable =
					DOVI_INOUT_FORMAT_CHANGE;
					dovi_hdr_md_info[0].dr_range =
					DISP_DR_TYPE_SDR;
					disp_dovi_process(1,
					&dovi_hdr_md_info[0]);
					#endif
				}
			} else if (dovi_path_en &&
			(vdp_start_st[0] == 1) &&
			(cur_ml_cfg_st[layer_id+2] == 0) &&
			(dovi_hdr_md_info[0].dr_range !=
				DISP_DR_TYPE_DOVI)) {
				//use last frame dr type to update
				disp_dovi_process(1, &dovi_hdr_md_info[0]);
			} else
				disp_dovi_gfx_set_defaut_setting(layer_id);
			osd_enable = 1;
		}
		if ((layer_id == FE1) && dovi_path_en) {
			//gfxfe do not update and use old setting in ml vs10
			//ctrl flow call others nee update new
			dovi_default("uhd gfx on update again\n");
			disp_fefifo_drv_set_input_order(3, 5);
		} else if ((layer_id == FE0) && dovi_path_en) {
			// case for fhd
			dovi_default("fhd gfx on update again\n");
			disp_fefifo_drv_set_input_order(2, 5);
		}
		break;

	case DISP_CMD_OSD_STOP:
		dovi_default("osd(%d) disable\n", *((uint32_t *) data));
		layer_id = *((uint32_t *) data);
		/*to be discuss vs10 flag*/
		if (dv_gfx_fe_en[layer_id])
			dovi_gfx_fe_hal_set_enable(layer_id, false);

		if (!(dv_gfx_fe_en[0] || dv_gfx_fe_en[1])) {
			osd_enable = 0;
			if ((dovi_out_info.out_format
				== DOVI_FORMAT_DOVI) ||
				(dovi_out_info.out_format
				== DOVI_FORMAT_VSEM_DOVI)) {
				dovi_out_info.b_gfx_mode
					= false;
				dovi_set_priority_mode(
					!dovi_out_info.b_gfx_mode);
				priority_mode_change =
					true;
			}
		}
		break;

	case DISP_CMD_DOVI_SET_VIDEO_IN:
		break;

	case DISP_CMD_DOVI_DUMP_FRAME:
		disp_dovi_idk_dump_frame();
		break;

	case DISP_CMD_FORCE_HDR:
		value = *((uint32_t *) data);
		dovi_default("dovi vs10 type:%d\n",
			value);
		if (value > 0)
			dovi_path_enable();
		else
			dovi_path_disable();
		break;

	default:
		break;
	}
	return DOVI_STATUS_OK;
}

void disp_dovi_irq_handle(void)
{
	if (dovi_black_pattern_en
		&& dovi_black_pattern_cnt_max) {
		dovi_black_pattern_cnt++;
		if (dovi_black_pattern_cnt >=
			dovi_black_pattern_cnt_max) {
			disp_mix_hal_set_black_pattern(false);
			dovi_black_pattern_en = false;
			dovi_black_pattern_cnt = 0;
		}
	}
}


int disp_dovi_irq_handler(uint32_t irq)
{
	if (!dovi_init_done)
		return DOVI_STATUS_OK;

	switch (irq) {
	case DISP_IRQ_FMT_VSYNC:
		disp_dovi_irq_handle();
		break;
	case DISP_IRQ_FMT_ACTIVE_START:
		//disp_dovi_wakeup_routine();
		break;
	case DISP_IRQ_FMT_ACTIVE_END:
		//disp_dovi_ut_ml_update();
		break;
	default:
		break;
	}
	return DOVI_STATUS_OK;
}

/*0-sdr tv
 *1-hdr10 tv
 *2-hlg tv
 *3-hdr10p tv
 *4-dovi std tv
 *5-dovi ll tv
 */
void disp_hdr_set_tv_info(uint32_t tv_type)
{
	struct disp_hw_tv_capbility *tv_cap = NULL;
	struct disp_hw_common_info *out_info = NULL;

	out_info = &hdr_common_info;
	tv_cap = &(hdr_common_info.tv);

	memset(out_info, 0, sizeof(struct disp_hw_common_info));

	switch (tv_type) {
	case 0: //sdr tv
		out_info->resolution = &disp_res_tbl[0];

		tv_cap->is_support_hdr = 0;
		tv_cap->is_support_hlg = 0;
		tv_cap->is_support_dovi = 0;
		tv_cap->is_support_dovi_2160p60 = 0;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 0;
		tv_cap->is_support_dovi_low_latency = 0;
		tv_cap->is_support_hdr10_plus = 0;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 0;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 255;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 0;
		tv_cap->screen_height = 0;
		tv_cap->max_tmds_rate = 225;
		memset(tv_cap->vsvdb_edid, 0, 0x1A);
		break;
	case 1: //hdr10TV
		out_info->resolution = &disp_res_tbl[1];

		tv_cap->is_support_hdr = 1;
		tv_cap->is_support_hlg = 0;
		tv_cap->is_support_dovi = 0;
		tv_cap->is_support_dovi_2160p60 = 0;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 0;
		tv_cap->is_support_hdr10_plus = 0;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 0;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 255;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 115;
		tv_cap->screen_height = 65;
		tv_cap->max_tmds_rate = 600;
		memset(tv_cap->vsvdb_edid, 0, 0x1A);
		break;
	case 2: //hdr10plus tv
		out_info->resolution = &disp_res_tbl[1];
		tv_cap->is_support_hdr = 1;
		tv_cap->is_support_hlg = 0;
		tv_cap->is_support_dovi = 0;
		tv_cap->is_support_dovi_2160p60 = 0;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 0;
		tv_cap->is_support_hdr10_plus = 1;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 0;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 0xFF;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 115;
		tv_cap->screen_height = 65;
		tv_cap->max_tmds_rate = 600;
		memset(tv_cap->vsvdb_edid, 0, 0x1A);
		break;
	case 3: //hdr10,hlg,hdr10plus tv
		out_info->resolution = &disp_res_tbl[1];
		tv_cap->is_support_hdr = 1;
		tv_cap->is_support_hlg = 1;
		tv_cap->is_support_dovi = 0;
		tv_cap->is_support_dovi_2160p60 = 0;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 0;
		tv_cap->is_support_hdr10_plus = 1;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 0;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 1;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 89;
		tv_cap->screen_height = 50;
		tv_cap->max_tmds_rate = 600;
		memset(tv_cap->vsvdb_edid, 0, 0x1A);
		break;
	case 4: //dovi std & ll
		out_info->resolution = &disp_res_tbl[1];
		tv_cap->is_support_hdr = 1;
		tv_cap->is_support_hlg = 1;
		tv_cap->is_support_dovi = 1;
		tv_cap->is_support_dovi_2160p60 = 1;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 1;
		tv_cap->is_support_hdr10_plus = 0;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 1;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 255;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 160;
		tv_cap->screen_height = 90;
		tv_cap->max_tmds_rate = 600;
		memcpy(tv_cap->vsvdb_edid,
			dv_std_tv_vsvdb, 0x1A);
		break;
	case 5: //dovi ll only
		out_info->resolution = &disp_res_tbl[0];

		tv_cap->is_support_hdr = 1;
		tv_cap->is_support_hlg = 1;
		tv_cap->is_support_dovi = 1;
		tv_cap->is_support_dovi_2160p60 = 0;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 1;
		tv_cap->is_support_hdr10_plus = 0;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 2;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 255;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 122;
		tv_cap->screen_height = 68;
		tv_cap->max_tmds_rate = 300;
		memcpy(tv_cap->vsvdb_edid,
			dv_ll_tv_vsvdb, 0x1A);
		break;
	case 6: //dovi ll and allm only
		out_info->resolution = &disp_res_tbl[0];
		tv_cap->is_support_hdr = 1;
		tv_cap->is_support_hlg = 1;
		tv_cap->is_support_dovi = 1;
		tv_cap->is_support_dovi_2160p60 = 0;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 1;
		tv_cap->is_support_hdr10_plus = 0;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 2;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 255;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 122;
		tv_cap->screen_height = 68;
		tv_cap->max_tmds_rate = 300;
		tv_cap->u1_sink_allm_support = 1;
		tv_cap->u1_sink_14gamemode_support = 1;
#ifdef DISP_VRR_SUPPORT
		tv_cap->u4_sink_vrr_min = 1;
		tv_cap->u4_sink_vrr_max = 120;
		tv_cap->u1_sink_cinemavrr = 1;
		tv_cap->u1_sink_mdelta = 0;
#endif
		memcpy(tv_cap->vsvdb_edid,
			dv_ll_tv_vsvdb, 0x1A);
		break;
	case 7: //dovi std and allm only
		out_info->resolution = &disp_res_tbl[0];
		tv_cap->is_support_hdr = 1;
		tv_cap->is_support_hlg = 1;
		tv_cap->is_support_dovi = 1;
		tv_cap->is_support_dovi_2160p60 = 1;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 0;
		tv_cap->is_support_hdr10_plus = 0;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 1;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 255;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 122;
		tv_cap->screen_height = 68;
		tv_cap->max_tmds_rate = 300;
		tv_cap->u1_sink_allm_support = 1;
		tv_cap->u1_sink_14gamemode_support = 1;
#ifdef DISP_VRR_SUPPORT
		tv_cap->u4_sink_vrr_min = 1;
		tv_cap->u4_sink_vrr_max = 120;
		tv_cap->u1_sink_cinemavrr = 1;
		tv_cap->u1_sink_mdelta = 0;
#endif
		memcpy(tv_cap->vsvdb_edid,
			vsvdb_v1_15, 0x1A);
		break;
	case 8: //hlg only tv
		out_info->resolution = &disp_res_tbl[1];
		tv_cap->is_support_hdr = 0;
		tv_cap->is_support_hlg = 1;
		tv_cap->is_support_dovi = 0;
		tv_cap->is_support_dovi_2160p60 = 0;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 0;
		tv_cap->is_support_hdr10_plus = 0;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 0;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 255;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 89;
		tv_cap->screen_height = 50;
		tv_cap->max_tmds_rate = 600;
		memset(tv_cap->vsvdb_edid, 0, 0x1A);
		break;
	case 9: //SDR2020 tv
		out_info->resolution = &disp_res_tbl[1];
		tv_cap->is_support_hdr = 0;
		tv_cap->is_support_hlg = 0;
		tv_cap->is_support_dovi = 0;
		tv_cap->is_support_dovi_2160p60 = 0;
		tv_cap->is_support_601 = 1;
		tv_cap->is_support_709 = 1;
		tv_cap->is_support_bt2020 = 1;
		tv_cap->is_support_dovi_low_latency = 0;
		tv_cap->is_support_hdr10_plus = 0;
		tv_cap->supported_resolution = 0;
		tv_cap->hdr_content_max_luminance = 0;
		tv_cap->hdr_content_max_frame_average_luminance = 0;
		tv_cap->hdr_content_min_luminance = 0;
		tv_cap->dovi_vsvdb_version = 0;
		tv_cap->dovi_vsvdb_v2_interface = 0;
		tv_cap->hdr10_plus_app_ver = 0xFF;
		tv_cap->force_hdr = 2;
		tv_cap->screen_width = 89;
		tv_cap->screen_height = 50;
		tv_cap->max_tmds_rate = 600;
		memset(tv_cap->vsvdb_edid, 0, 0x1A);
		break;
	default:
		dovi_default("tv type error\n");
		break;
	}
	dovi_default("set tv type done %d\n", tv_type);
	tv_info_set_by_cmd = 1;
}

/*uint32_tlayer_id
 *bit 3:0 layerid
 *bit7:4 layer0 buf type 0-sdr 1-hdr10 2-hlg 3-hdr10p 4-dovi
 *bit11:8 layer1 buf type 0-sdr 1-hdr10 2-hlg 3-hdr10p 4-dovi
 */
void disp_hdr_set_config(uint32_t layer_id, uint32_t rpu_id)
{
	struct video_buffer_info *buf_info = NULL;
	struct mtk_disp_buffer *osd_buf = NULL;
	bool layer_en[4] = { 0 };
	struct VID_STATIC_HDMI_MD_T *hdr10_info = NULL;
	bool b_sub_existed = 0;
	uint32_t src_type = 0;

	layer_en[0] = layer_id & 0x1;
	layer_en[1] = layer_id & 0x2;
	layer_en[2] = layer_id & 0x4;
	layer_en[3] = layer_id & 0x8;

	layer_info_set_by_cmd = rpu_id;

	if (layer_en[1] == 1)
		b_sub_existed = 1;

	if (layer_en[0]) {
		buf_info = &hdr_test_layer[0];
		hdr10_info = &buf_info->hdr10_info;
		buf_info->layer_id = 0;
		buf_info->layer_enable = 1;
		src_type = (layer_id & 0xF0) >> 4;
		switch (src_type) {
		case 0:
			buf_info->hdr10_type = HDR10_TYPE_NONE;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_SDR;
			break;
		case 1:
			buf_info->hdr10_type = HDR10_TYPE_ST2084;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_HDR10;
			hdr10_info->fgNeedUpdStaticMeta = true;
			hdr10_info->ui2_DisplayPrimariesX[0] = 35400;
			hdr10_info->ui2_DisplayPrimariesX[1] = 8500;
			hdr10_info->ui2_DisplayPrimariesX[2] = 6550;
			hdr10_info->ui2_DisplayPrimariesY[0] = 14600;
			hdr10_info->ui2_DisplayPrimariesY[1] = 39850;
			hdr10_info->ui2_DisplayPrimariesY[2] = 2300;
			hdr10_info->ui2_WhitePointX = 15635;
			hdr10_info->ui2_WhitePointY = 16450;
			hdr10_info->ui2_MaxCLL = 0;
			hdr10_info->ui2_MaxFALL = 0;
			hdr10_info->ui2_MaxDisplayMasteringLuminance = 1000;
			hdr10_info->ui2_MinDisplayMasteringLuminance = 50;
			break;
		case 2:
			buf_info->hdr10_type = HDR10_TYPE_HLG;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_HLG;
			break;
		case 3:
			buf_info->hdr10_type = HDR10_TYPE_PLUS;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_HDR10;
			buf_info->is_metadata_async = 1;
			break;
		case 4:
			buf_info->hdr10_type = HDR10_TYPE_NONE;
			buf_info->is_dovi = 1;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_DOVI;
			break;
		default:
			buf_info->hdr10_type = HDR10_TYPE_NONE;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_SDR;
			break;
		}
		dovi_default("set src info %d %d %d 0x%p %d\n",
			buf_info->hdr10_type,
			buf_info->is_dovi, buf_info->hdr_info.dr_range,
			buf_info, layer_info_set_by_cmd);
		disp_hdr_config_video_info(buf_info, b_sub_existed);
	}

	if (layer_en[1]) {
		buf_info = &hdr_test_layer[1];
		hdr10_info = &buf_info->hdr10_info;
		buf_info->layer_id = 1;
		buf_info->layer_enable = 1;
		src_type = (layer_id & 0xF0) >> 4;
		switch (src_type) {
		case 0:
			buf_info->hdr10_type = HDR10_TYPE_NONE;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_SDR;
			break;
		case 1:
			buf_info->hdr10_type = HDR10_TYPE_ST2084;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_HDR10;
			hdr10_info->fgNeedUpdStaticMeta = true;
			hdr10_info->ui2_DisplayPrimariesX[0] = 35400;
			hdr10_info->ui2_DisplayPrimariesX[1] = 8500;
			hdr10_info->ui2_DisplayPrimariesX[2] = 6550;
			hdr10_info->ui2_DisplayPrimariesY[0] = 14600;
			hdr10_info->ui2_DisplayPrimariesY[1] = 39850;
			hdr10_info->ui2_DisplayPrimariesY[2] = 2300;
			hdr10_info->ui2_WhitePointX = 15635;
			hdr10_info->ui2_WhitePointY = 16450;
			hdr10_info->ui2_MaxCLL = 0;
			hdr10_info->ui2_MaxFALL = 0;
			hdr10_info->ui2_MaxDisplayMasteringLuminance = 1000;
			hdr10_info->ui2_MinDisplayMasteringLuminance = 50;
			break;
		case 2:
			buf_info->hdr10_type = HDR10_TYPE_HLG;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_HLG;
			break;
		case 3:
			buf_info->hdr10_type = HDR10_TYPE_PLUS;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_HDR10;
			break;
		case 4:
			buf_info->hdr10_type = HDR10_TYPE_NONE;
			buf_info->is_dovi = 1;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_DOVI;
			break;
		default:
			buf_info->hdr10_type = HDR10_TYPE_NONE;
			buf_info->is_dovi = 0;
			buf_info->hdr_info.dr_range = DISP_DR_TYPE_SDR;
			break;
		}

		disp_hdr_config_video_info(buf_info, b_sub_existed);
	}
	if (layer_en[2]) {
		osd_buf = &hdr_test_layer[2].video_disp_buffer;
		osd_buf->layer_id = 0;
		osd_buf->layer_enable = 1;
		disp_hdr_config_osd_info(osd_buf);
	}
	if (layer_en[3]) {
		osd_buf = &hdr_test_layer[3].video_disp_buffer;
		osd_buf->layer_id = 1;
		osd_buf->layer_enable = 1;
		disp_hdr_config_osd_info(osd_buf);
	}

	dovi_default("set layer_on 0x%x(%d %d %d %d)\n", layer_id,
		layer_en[0], layer_en[1], layer_en[2], layer_en[3]);
	dovi_default("set type(%d %d)(%d %d)(%d %d)(%d %d)\n",
		hdr_test_layer[0].hdr10_type, hdr_test_layer[0].is_dovi,
		hdr_test_layer[1].hdr10_type, hdr_test_layer[1].is_dovi,
		hdr_test_layer[2].hdr10_type, hdr_test_layer[2].is_dovi,
		hdr_test_layer[3].hdr10_type, hdr_test_layer[3].is_dovi);
}

void disp_hdr_trigger_vdp(uint32_t id)
{
	struct video_buffer_info *cur_buf = NULL;
	struct video_buffer_info *next_buf = NULL;

	cur_buf = &hdr_test_layer[0];
	next_buf = &hdr_test_layer[1];

	if (!(cur_buf->layer_enable && next_buf->layer_enable)) {
		dovi_default("set buf info first\n");
		return;
	}

	vdp_trigger_metadata_config(id, cur_buf, next_buf);
}

void disp_dovi_set_hdr_fe_size(uint32_t layer_id, uint32_t width,
	uint32_t heigh)
{
	switch (layer_id) {
	case LAYER2:
		Dv_WriteREG(dovi_reg_base[DOVI_FHD_FE] + 0x3d0,
			heigh);
		break;
	case LAYER3:
		Dv_WriteREG(dovi_reg_base[DOVI_UHD_FE] + 0x3d0,
			heigh);
		break;
	default:
		break;
	}
}
