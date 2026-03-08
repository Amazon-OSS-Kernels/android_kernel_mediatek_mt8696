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

#define LOG_TAG "DOVI_DEBUG"

#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#include "dovi_log.h"
#include "disp_dovi_io.h"
#include "disp_info.h"
#include "disp_hw_mgr.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"
#include "dovi_table.h"
#include "dovi_vdo_fe_hal.h"
#include "dovi_gfx_fe_hal.h"
#include "dovi_be_hal.h"
#include "disp_dovi_tz_client.h"
#include "disp_path.h"
#include "fmt_hal.h"
#include "disp_hdr_main.h"
#include "disp_dovi_main.h"
#include "disp_dovi_unit_test.h"
#include "disp_dovi_cmd.h"
#include "disp_mix_hal.h"

static int dovi_dbg_init;
unsigned int dovi_dbg_level;// = 4;
static struct dentry *dovi_debugfs;
uint32_t idk_stop_frame_num;

static char dovi_dbg_buf[2048];
static char dovi_cmd_buf[512];
bool dovi_black_en_bycmd;
uint32_t dovi_black_cnt_bycmd;

static const char DOVI_STR_HELP[] = "USAGE:echo [ACTION]>/d/dovi\n";

static void dovi_process_dbg_opt(const char *opt)
{
	int ret = 0;
	char *p;

	if (strncmp(opt, "log_level:", 10) == 0) {
		unsigned int level = 0;
		unsigned int enable = 0;

		p = (char *)opt + 10;
		STR_CVT_U32(&p, &level, goto Error);
		STR_CVT_U32(&p, &enable, goto Error);
		if (enable)
			dovi_dbg_level |= 1 << level;
		else
			dovi_dbg_level &= 0 << level;
		dovi_printf("dovi_log_level %d\n", dovi_dbg_level);
	} else if (strncmp(opt, "log_level_tz:", 13) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 13;
		STR_CVT_U32(&p, &value, goto Error);
		dovi_sec_debug_level_init(value);
	} else if (strncmp(opt, "status", 6) == 0) {
		disp_dovi_status();
	} else if (strncmp(opt, "dump_rpu:", 9) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &value, goto Error);
		dump_rpu_enable = value;
		dovi_printf("set dump rpu enable %d.\n", value);
	} else if (strncmp(opt, "dump_md:", 8) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &value, goto Error);
		dump_md_enable = value;
		dovi_printf("set dump md enable %d.\n", value);
	} else if (strncmp(opt, "vin_dump_bpp:", 13) == 0) {
		unsigned int dump_bpp = 0;

		p = (char *)opt + 13;
		STR_CVT_U32(&p, &dump_bpp, goto Error);
		if (dump_bpp == 10)
			idk_dump_bpp = VIDEOIN_BITMODE_10;
		else if (dump_bpp == 8)
			idk_dump_bpp = VIDEOIN_BITMODE_8;
		else if (dump_bpp == 12)
			idk_dump_bpp = VIDEOIN_BITMODE_12;
	} else if (strncmp(opt, "dovimute:", 9) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &value, goto Error);
		dovi_mute = value;
		dovi_printf("set dovi_mute %d.\n", dovi_mute);
	} else if (strncmp(opt, "osdscale:", 9) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &value, goto Error);
		fhd_scale_to_uhd = value;
		dovi_printf("set fhd_scale_to_uhd %d.\n", fhd_scale_to_uhd);
	}  else if (strncmp(opt, "idkstop:", 8) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &value, goto Error);
		idk_stop_frame_num = value;
		dovi_printf("set stop number %d.\n", idk_stop_frame_num);
	} else if (strncmp(opt, "idk_test:", 9) == 0) {
		uint32_t value1 = 0;
		uint32_t value2 = 0;
		uint32_t disp_cnt = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &value1, goto Error);
		STR_CVT_U32(&p, &value2, goto Error);
		STR_CVT_U32(&p, &disp_cnt, goto Error);

		dovi_idk_file_id = value1;
		dovi_idk_test = value2;
		if (dovi_idk_test)
			disp_dovi_set_idk_info();

		dovi_idk_disp_cnt = disp_cnt;
		if (dovi_idk_disp_cnt > 0)
			dovi_idk_dump = true;
		else
			dovi_idk_dump = false;
		dovi_printf("set dovi_idk_test %d, test on = %d.\n",
		    dovi_idk_file_id, dovi_idk_test);
		dovi_printf("dv_idk_dump %d, dovi_idk_disp_cnt %d!\n",
			   dovi_idk_dump, dovi_idk_disp_cnt);
	} else if (strncmp(opt, "tz_test:", 8) == 0) {
		unsigned int value1 = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &value1, goto Error);

		dovi_printf("set tz_test %d\n", value1);
		disp_dovi_set_tz_test_info(value1);
	} else if (strncmp(opt, "sdk_dump:", 9) == 0) {
		unsigned int value1 = 0;
		unsigned int value2 = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &value1, goto Error);
		STR_CVT_U32(&p, &value2, goto Error);

		dovi_sdk_test = value1;

		if (value2 == 0) {	/* HD src -> HD display */
			dovi_sdk_file_id = 0;
			fhd_scale_to_uhd = 0;
		} else if (value2 == 1) {	/* UHD src -> UHD display */
			dovi_sdk_file_id = 1;
			fhd_scale_to_uhd = 0;
		} else if (value2 == 2) {	/* HD src -> UHD display */
			dovi_sdk_file_id = 0;
			fhd_scale_to_uhd = 1;
		}
		dovi_printf("set test %d, file_id %d fhd_scale_to_uhd %d\n",
			    value1, dovi_sdk_file_id, fhd_scale_to_uhd);
		disp_dovi_set_sdk_info();
	} else if (strncmp(opt, "gfx_max_lum:", 12) == 0) {
		unsigned int value1 = 0;
		unsigned int value2 = 0;

		p = (char *)opt + 12;
		STR_CVT_U32(&p, &value1, goto Error);
		STR_CVT_U32(&p, &value2, goto Error);

		set_graphic_max_lum_enable = value1;
		graphic_max_lum = value2;
		dovi_printf("set %d, graphic_max_lum %d.\n",
			    set_graphic_max_lum_enable, graphic_max_lum);
	} else if (strncmp(opt, "vdo_max_lum:", 12) == 0) {
		unsigned int value1 = 0;
		unsigned int value2 = 0;

		p = (char *)opt + 12;
		STR_CVT_U32(&p, &value1, goto Error);
		STR_CVT_U32(&p, &value2, goto Error);

		set_video_max_lum_enable = value1;
		video_max_lum = value2;
		dovi_printf("set %d, video_max_lum %d.\n",
			    set_video_max_lum_enable, video_max_lum);
	} else if (strncmp(opt, "hdmioutf:", 9) == 0) {
		unsigned int value1 = 0;
		unsigned int value2 = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &value1, goto Error);
		STR_CVT_U32(&p, &value2, goto Error);
		dovi_printf("enable %d, format %d.\n", value1, value2);
		disp_dovi_set_hdr_enable(value1, value2);
	} else if (strncmp(opt, "out_format:", 11) == 0) {
		unsigned int test_case_id = 0;
		unsigned int out_format = 0;

		p = (char *)opt + 11;
		STR_CVT_U32(&p, &test_case_id, goto Error);
		STR_CVT_U32(&p, &out_format, goto Error);

		get_dovi_out_format_status();
		ret = set_dovi_out_format(test_case_id, out_format);
		dovi_printf("set %d out_format %d ret %d\n",
			    test_case_id, out_format, ret);
	} else if (strncmp(opt, "dovi2hdr10:", 11) == 0) {
		unsigned int test_case_id = 0;
		unsigned int out_format = 0;

		p = (char *)opt + 11;
		STR_CVT_U32(&p, &test_case_id, goto Error);
		STR_CVT_U32(&p, &out_format, goto Error);

		get_dovi2hdr10_mapping_type_Status();
		ret = set_dovi2hdr10_mapping_type(test_case_id, out_format);
		dovi_printf("set %d out_format %d ret %d\n",
			    test_case_id, out_format, ret);
	} else if (strncmp(opt, "ll_mode:", 8) == 0) {
		unsigned int test_case_id = 0;
		unsigned int use_ll = 0;
		unsigned int ll_rgb_desired = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &test_case_id, goto Error);
		STR_CVT_U32(&p, &use_ll, goto Error);
		STR_CVT_U32(&p, &ll_rgb_desired, goto Error);

		get_dovi_ll_mode_status();
		ret = set_dovi_ll_mode(test_case_id, use_ll,
			ll_rgb_desired);
		dovi_printf("set  %d use_ll %d ll_rgb %d ret %d\n",
			    test_case_id, use_ll, ll_rgb_desired, ret);
	} else if (strncmp(opt, "ll_format:", 10) == 0) {
		/* set low latency mode yuv(0) or rgb(1) */
		p = (char *)opt + 10;
		STR_CVT_U32(&p, &ll_format, goto Error);
		dovi_printf("set ll_format %d\n", ll_format);
	} else if (strncmp(opt, "set_pri_mode:", 13) == 0) {
		unsigned int force_pri_mode = 0;
		unsigned int pri_mode = 0;

		p = (char *)opt + 13;
		STR_CVT_U32(&p, &force_pri_mode, goto Error);
		STR_CVT_U32(&p, &pri_mode, goto Error);

		set_dovi_priority_mode(force_pri_mode, pri_mode);
		dovi_set_priority_mode(pri_mode);
		dovi_printf("set force_pri_mode %d pri_mode %d\n",
			force_pri_mode, pri_mode);
	} else if (strncmp(opt, "get_pri_mode:", 13) == 0) {
		unsigned int pri_mode = 0;

		dovi_get_priority_mode(&pri_mode);
		dovi_printf("get pri_mode %d\n", pri_mode);
	} else if (strncmp(opt, "test_mode:", 10) == 0) {
		uint32_t value = 0;

		p = (char *)opt + 10;
		STR_CVT_U32(&p, &value, goto Error);

		dovi_set_test_mode(value);
		dovi_printf("test mode: %d\n", value);
	} else if (strncmp(opt, "support_el:", 11) == 0) {
		uint32_t value = 0;

		p = (char *)opt + 11;
		STR_CVT_U32(&p, &value, goto Error);

		dovi_set_support_el(value);
		dovi_printf("support el: %d\n", value);
	} else if (strncmp(opt, "g_format:", 9) == 0) {
		unsigned int force_g_format = 0;
		unsigned int g_format = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &force_g_format, goto Error);
		STR_CVT_U32(&p, &g_format, goto Error);

		set_dovi_g_format(force_g_format, g_format);
		dovi_printf("set force_g_format %d g_format %d\n",
			force_g_format, g_format);
	} else if (strncmp(opt, "black:", 6) == 0) {
		uint32_t black_en = 0;
		uint32_t black_cnt = 0;

		p = (char *)opt + 6;
		STR_CVT_U32(&p, &black_en, goto Error);
		STR_CVT_U32(&p, &black_cnt, goto Error);

		dovi_black_en_bycmd = (bool)black_en;
		dovi_black_cnt_bycmd = black_cnt;
		dovi_printf("set black info %d %d\n", black_en, black_cnt);

	} else if (strncmp(opt, "dump_f:", 7) == 0) {
		uint32_t enable = 0;
		enum VIDEOIN_SRC_SEL src = 0;
		enum VIDEOIN_YCbCr_FORMAT fmt = 0;
		enum VIDEO_BIT_MODE bpp = 0;

		p = (char *)opt + 7;

		STR_CVT_U32(&p, &enable, goto Error);
		STR_CVT_U32(&p, &src, goto Error);
		STR_CVT_U32(&p, &fmt, goto Error);
		STR_CVT_U32(&p, &bpp, goto Error);

		dovi_printf("set dump frame %d src %d fmt %d, bpp %d\n",
			    enable, src, fmt, bpp);
	} else if (strncmp(opt, "alloc:", 6) == 0) {
		uint32_t vdp_id = 0;
		unsigned int enable = 0;

		p = (char *)opt + 6;
		STR_CVT_U32(&p, &vdp_id, goto Error);
		STR_CVT_U32(&p, &enable, goto Error);

		disp_dovi_alloc_input_buf(vdp_id, enable);
	} else if (strncmp(opt, "load:", 5) == 0) {
		unsigned int vdp_id = 0;
		uint32_t pattern = 0;
		enum dovi_clr_fmt color_fmt = 0;

		p = (char *)opt + 5;
		STR_CVT_U32(&p, &vdp_id, goto Error);
		STR_CVT_U32(&p, &pattern, goto Error);
		STR_CVT_U32(&p, &color_fmt, goto Error);

		dovi_default("load vdp %d pattern %d fmt %d.\n",
			vdp_id, pattern, color_fmt);

		disp_dovi_load_video_pattern(vdp_id, pattern,
			color_fmt);
	} else if (strncmp(opt, "dovioutf:", 9) == 0) {
		char *p = (char *)opt + 9;
		unsigned int dovioutf = 0;
		unsigned int dovi_outformat = 0;

		STR_CVT_U32(&p, &dovioutf, goto Error);
		STR_CVT_U32(&p, &dovi_outformat, goto Error);

		dovi_force_output = dovioutf;
		dovi_force_out_format =
			(enum dovi_signal_format_t)dovi_outformat;
		dovi_default(
			"set vdp dovi_force_output %d, dovi_force_out_format %d!\n",
			dovi_force_output, dovi_force_out_format);
	} else if (strncmp(opt, "dovipath:", 9) == 0) {
		uint32_t dovipath = 0;

		char *p = (char *)opt + 9;

		STR_CVT_U32(&p, &dovipath, goto Error);
		disp_path_set_hw_path(DISP_PATH_M_HDR_VDO_FE, 1);
		dovi_default("disp_path_set_dovi %d!\n", dovipath);
	} else if (strncmp(opt, "dovipath2:", 10) == 0) {
		uint32_t dovipath = 0;

		char *p = (char *)opt + 10;

		STR_CVT_U32(&p, &dovipath, goto Error);

		if (!dovipath)
			disp_path_set_hw_path(DISP_PATH_M_HDR_VDO_FE, 0);
		else
			disp_path_set_hw_path(DISP_PATH_M_HDR_VDO_FE, 1);
	} else if (strncmp(opt, "dovi_enable", 11) == 0) {
		dovi_path_enable();
	} else if (strncmp(opt, "dovi_disable", 12) == 0) {
		dovi_path_disable();
	} else if (strncmp(opt, "dv_sec_init", 11) == 0) {
		dovi_default("dv sec init\n");
		disp_dovi_init_sec_by_cmd();
	} else if (strncmp(opt, "hdr_def:", 8) == 0) {
		char *p = (char *)opt + 8;

		STR_CVT_U32(&p, &g_force_dovi, goto Error);
		STR_CVT_U32(&p, &g_force_open_hdr, goto Error);
		STR_CVT_U32(&p, &g_hdr_type, goto Error);
		STR_CVT_U32(&p, &g_out_format, goto Error);
		STR_CVT_U32(&p, &g_dovi_efuse, goto Error);
		dovi_default("set hdr def: %d %d %d %d %d\n",
			g_force_dovi, g_force_open_hdr, g_hdr_type,
			g_out_format, g_dovi_efuse);

	} else if (strncmp(opt, "hdr_cmd:", 8) == 0) {
		uint32_t cmd = 0;
		uint32_t data = 0;

		char *p = (char *)opt + 8;

		STR_CVT_U32(&p, &cmd, goto Error);
		STR_CVT_U32(&p, &data, goto Error);
		dovi_default("set hdr cmd %d %d\n", cmd, data);
		layer_info_set_by_cmd = 1;
		disp_hdr_cmd(cmd, &data);
	} else if (strncmp(opt, "set_tv:", 7) == 0) {
		uint32_t tv_type = 0;

		p = (char *)opt + 7;
		STR_CVT_U32(&p, &tv_type, goto Error);
		disp_hdr_set_tv_info(tv_type);
		dovi_printf("set tv type %d\n", tv_type);

	} else if (strncmp(opt, "set_src:", 8) == 0) {
		uint32_t layer_info = 0;
		uint32_t rpu_id = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &layer_info, goto Error);
		STR_CVT_U32(&p, &rpu_id, goto Error);
		disp_hdr_set_config(layer_info, rpu_id);
		dovi_printf("set layer_info 0x%x\n", layer_info);
	} else if (strncmp(opt, "trig_vdp:", 9) == 0) {
		uint32_t id = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &id, goto Error);
		disp_hdr_trigger_vdp(id);
	} else if (strncmp(opt, "comp:", 5) == 0) {
		uint32_t enable = 0;

		p = (char *)opt + 5;
		STR_CVT_U32(&p, &enable, goto Error);
		b_comp_enable = (bool)enable;
		dovi_printf("set compose enable %d\n", b_comp_enable);
	} else if (strncmp(opt, "subv:", 5) == 0) {
		uint32_t subv_type_val = 0;

		p = (char *)opt + 5;
		STR_CVT_U32(&p, &subv_type_val, goto Error);
		_subv_type = subv_type_val;
		dovi_printf("set subv_type_val %d\n", _subv_type);
	} else if (strncmp(opt, "insize:", 7) == 0) {
		uint32_t id = 0;
		uint32_t width = 0;
		uint32_t height = 0;

		p = (char *)opt + 7;
		STR_CVT_U32(&p, &id, goto Error);
		STR_CVT_U32(&p, &width, goto Error);
		STR_CVT_U32(&p, &height, goto Error);
		disp_dovi_update_input_size(id, width, height);
		dovi_printf("set insize %d %d\n", width, height);
	} else if (strncmp(opt, "test_av1", 8) == 0) {

		p = (char *)opt + 8;
		dovi_av1_parser_test();

#if IS_ENABLED(CONFIG_DOVI_UT_SUPPORT)
	} else if (strncmp(opt, "slt:", 4) == 0) {
		uint32_t slt_case = 0;

		p = (char *)opt + 4;
		STR_CVT_U32(&p, &slt_case, goto Error);
		dovi_slt_case(slt_case);
	} else if (strncmp(opt, "ut_init:", 8) == 0) {
		uint32_t en = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &en, goto Error);
		dovi_default("dovi vfy init\n");
		dovi_ut_init(en);
		videoin_hal_enable(en);
	} else if (strncmp(opt, "ut_case:", 8) == 0) {
		uint32_t case_id = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &case_id, goto Error);

		dovi_set_ut_case(case_id);
	} else if (strncmp(opt, "ut_flush", 8) == 0) {
		dovi_default("dovi ut flush\n")
		dovi_set_ut_flush();
	} else if (strncmp(opt, "ut_path:", 8) == 0) {
		uint32_t timing = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &timing, goto Error);

		dovi_set_path_info(timing);

	} else if (strncmp(opt, "reg_test:", 9) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &value, goto Error);
		disp_dovi_reg_test(value);
		dovi_printf("set reg_test option %d.\n", value);
	} else if (strncmp(opt, "ut_layer:", 9) == 0) {
		uint32_t out_res = 0;
		uint32_t out_format = 0;
		uint32_t ll_on = 0;
		uint32_t ll_rgb = 0;
		uint32_t layer_on[4] = {0};
		uint32_t case_id[4] = {0};

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &out_res, goto Error);
		STR_CVT_U32(&p, &out_format, goto Error);
		STR_CVT_U32(&p, &ll_on, goto Error);
		STR_CVT_U32(&p, &ll_rgb, goto Error);
		STR_CVT_U32(&p, &layer_on[0], goto Error);
		STR_CVT_U32(&p, &case_id[0], goto Error);
		STR_CVT_U32(&p, &layer_on[1], goto Error);
		STR_CVT_U32(&p, &case_id[1], goto Error);
		STR_CVT_U32(&p, &layer_on[2], goto Error);
		STR_CVT_U32(&p, &case_id[2], goto Error);
		STR_CVT_U32(&p, &layer_on[3], goto Error);
		STR_CVT_U32(&p, &case_id[3], goto Error);

		dovi_set_unit_test_info(out_res, out_format, ll_on, ll_rgb,
			layer_on, case_id);
#endif
	} else if (strncmp(opt, "idk_set:", 8) == 0) {
		uint32_t idk_set = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &idk_set, goto Error);
		dovi_default("set idk_set = %d\n", idk_set);
		if (idk_set == 0) {
			no_mix_fhd = 0;
			no_mix_uhd = 0;
			dovi_idk_settings(idk_set);
		}
	} else if (strncmp(opt, "no_mix_gfx:", 11) == 0) {
		uint32_t cmd1 = 0;
		uint32_t cmd2 = 0;

		p = (char *)opt + 11;
		STR_CVT_U32(&p, &cmd1, goto Error);
		STR_CVT_U32(&p, &cmd2, goto Error);
		no_mix_fhd = cmd1;
		no_mix_uhd = cmd2;
		dovi_default("set no_mix_fhd = %d, no_mix_uhd = %d\n",
			no_mix_fhd, no_mix_uhd);
	} else if (strncmp(opt, "gfx_dump:", 9) == 0) {
		uint32_t gfx_dump = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &gfx_dump, goto Error);
		dovi_default("set gfx_dump = %d\n", gfx_dump);
		if (gfx_dump == 1)
			disp_dovi_dump_gfx();
	} else if (strncmp(opt, "gfx_vdoin:", 10) == 0) {
		uint32_t gfx_vdoin = 0;

		p = (char *)opt + 10;
		STR_CVT_U32(&p, &gfx_vdoin, goto Error);
		dovi_default("set gfx_vdoin = %d\n", gfx_vdoin);
		if (gfx_vdoin == 1)
			disp_dovi_videoin_gfx();
		else
			disp_dovi_videoin_off_gfx();
	} else if (strncmp(opt, "dump_crycb:", 11) == 0) {
		p = (char *)opt + 11;
		STR_CVT_U32(&p, &dump_crycb, goto Error);
		dovi_default("set dump_crycb = %d\n", dump_crycb);
	} else if (strncmp(opt, "dump_bit:", 9) == 0) {
		p = (char *)opt + 9;
		STR_CVT_U32(&p, &dump_bit_depth, goto Error);
		dovi_default("set dump_bit_depth = %d\n", dump_bit_depth);
	} else if (strncmp(opt, "f_gfx_off:", 10) == 0) {

		p = (char *)opt + 10;
		STR_CVT_U32(&p, &f_graphic_off_cmd, goto Error);
		dovi_set_graphic_info(1 - f_graphic_off_cmd);
		dovi_printf("force gfx off %d\n", f_graphic_off_cmd);
	} else if (strncmp(opt, "idk_vsem:", 9) == 0) {
		p = (char *)opt + 9;
		STR_CVT_U32(&p, &idk_vsem, goto Error);
		dovi_default("set idk_vsem = %d\n", idk_vsem);
	} else if (strncmp(opt, "sdk_vsem:", 9) == 0) {
		p = (char *)opt + 9;
		STR_CVT_U32(&p, &sdk_vsem, goto Error);
		dovi_default("set sdk_vsem = %d\n", sdk_vsem);
	} else if (strncmp(opt, "mapping:", 8) == 0) {
		uint32_t dovi2hdr10_mapping = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &dovi2hdr10_mapping, goto Error);
		dovi_set_dovi2hdr10_mapping(dovi2hdr10_mapping);
		dovi_default("set dovi2hdr10_mapping = %d\n",
			dovi2hdr10_mapping);
	} else if (strncmp(opt, "vsvdb:", 6) == 0) {
		char *vsvdb_file_name = "vsvdb_push.bin";

		p = (char *)opt + 6;
		STR_CVT_U32(&p, &set_vsvdb, goto Error);
		//if (set_vsvdb)
		//	dovi_set_vsvdb_file_name(vsvdb_file_name);
		dovi_default("set_vsvdb /sdcard/vsvdb/%s\n",
			vsvdb_file_name);
	} else if (strncmp(opt, "tvbri:", 6) == 0) {

		p = (char *)opt + 6;
		STR_CVT_U32(&p, &dovi_hdmi_brightness_en, goto Error);
		STR_CVT_U32(&p, &dovi_hdmi_brightness, goto Error);
		dovi_default("set tv brightness %d %d\n",
			dovi_hdmi_brightness_en, dovi_hdmi_brightness);
	} else if (strncmp(opt, "ext_md:", 7) == 0) {

		p = (char *)opt + 7;
		STR_CVT_U32(&p, &dovi_get_ext_md, goto Error);
		dovi_default("get ext_md %d\n", dovi_get_ext_md);
	} else {

		dovi_error("test debug cmd pass.\n");
		goto Error;
	}

	return;

Error:
	dovi_error("parse command error!%s\n",
		DOVI_STR_HELP);
}

static void dovi_process_dbg_cmd(char *cmd)
{
	char *tok = NULL;

	dovi_printf("cmd: %s\n", cmd);
	/*memset(dovi_dbg_buf, 0, sizeof(dovi_dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		dovi_printf("parse: %s\n", tok);
		dovi_process_dbg_opt(tok);
	}
}

/* --------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------- */

static void dovi_process_dbg_opt_simplify(const char *opt)
{
	char *p = NULL;

	if (strncmp(opt, "sdk_dump:", 9) == 0) {
		unsigned int value1 = 0;
		unsigned int value2 = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &value1, goto Error);
		STR_CVT_U32(&p, &value2, goto Error);

		dovi_sdk_test = value1;

		if (value2 == 0) {	/* HD src -> HD display */
			dovi_sdk_file_id = 0;
			fhd_scale_to_uhd = 0;
		} else if (value2 == 1) {	/* UHD src -> UHD display */
			dovi_sdk_file_id = 1;
			fhd_scale_to_uhd = 0;
		} else if (value2 == 2) {	/* HD src -> UHD display */
			dovi_sdk_file_id = 0;
			fhd_scale_to_uhd = 1;
		}
		dovi_printf("set test %d, file_id %d fhd_scale_to_uhd %d\n",
			    value1, dovi_sdk_file_id, fhd_scale_to_uhd);
		disp_dovi_set_sdk_info();
	} else if (strncmp(opt, "gfx_max_lum:", 12) == 0) {
		unsigned int value1 = 0;
		unsigned int value2 = 0;

		p = (char *)opt + 12;
		STR_CVT_U32(&p, &value1, goto Error);
		STR_CVT_U32(&p, &value2, goto Error);

		set_graphic_max_lum_enable = value1;
		graphic_max_lum = value2;
		dovi_printf("set %d, graphic_max_lum %d.\n",
			    set_graphic_max_lum_enable, graphic_max_lum);
	} else if (strncmp(opt, "set_pri_mode:", 13) == 0) {
		unsigned int force_pri_mode = 0;
		unsigned int pri_mode = 0;

		p = (char *)opt + 13;
		STR_CVT_U32(&p, &force_pri_mode, goto Error);
		STR_CVT_U32(&p, &pri_mode, goto Error);

		set_dovi_priority_mode(force_pri_mode, pri_mode);
		dovi_set_priority_mode(pri_mode);
		dovi_printf("set force_pri_mode %d pri_mode %d\n",
			force_pri_mode, pri_mode);
	} else if (strncmp(opt, "dovioutf:", 9) == 0) {
		char *p = (char *)opt + 9;
		unsigned int dovioutf = 0;
		unsigned int dovi_outformat = 0;

		STR_CVT_U32(&p, &dovioutf, goto Error);
		STR_CVT_U32(&p, &dovi_outformat, goto Error);

		dovi_force_output = dovioutf;
		dovi_force_out_format =
			(enum dovi_signal_format_t)dovi_outformat;
		dovi_default(
			"set vdp dovi_force_output %d, dovi_force_out_format %d!\n",
			dovi_force_output, dovi_force_out_format);
	} else if (strncmp(opt, "idk_set:", 8) == 0) {
		uint32_t idk_set = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &idk_set, goto Error);
		dovi_default("set idk_set = %d\n", idk_set);
		if (idk_set == 0) {
			no_mix_fhd = 0;
			no_mix_uhd = 0;
			dovi_idk_settings(idk_set);
		}
	} else if (strncmp(opt, "no_mix_gfx:", 11) == 0) {
		uint32_t cmd1 = 0;
		uint32_t cmd2 = 0;

		p = (char *)opt + 11;
		STR_CVT_U32(&p, &cmd1, goto Error);
		STR_CVT_U32(&p, &cmd2, goto Error);
		no_mix_fhd = cmd1;
		no_mix_uhd = cmd2;
		dovi_default("set no_mix_fhd = %d, no_mix_uhd = %d\n",
			no_mix_fhd, no_mix_uhd);
	} else if (strncmp(opt, "f_gfx_off:", 10) == 0) {
		p = (char *)opt + 10;

		STR_CVT_U32(&p, &f_graphic_off_cmd, goto Error);
		dovi_set_graphic_info(1 - f_graphic_off_cmd);
		dovi_printf("force gfx off %d\n", f_graphic_off_cmd);
	} else if (strncmp(opt, "idk_vsem:", 9) == 0) {
		p = (char *)opt + 9;

		STR_CVT_U32(&p, &idk_vsem, goto Error);
		dovi_default("set idk_vsem = %d\n", idk_vsem);
	} else if (strncmp(opt, "sdk_vsem:", 9) == 0) {
		p = (char *)opt + 9;

		STR_CVT_U32(&p, &sdk_vsem, goto Error);
		dovi_default("set sdk_vsem = %d\n", sdk_vsem);
	} else if (strncmp(opt, "mapping:", 8) == 0) {
		uint32_t dovi2hdr10_mapping = 0;

		p = (char *)opt + 8;
		STR_CVT_U32(&p, &dovi2hdr10_mapping, goto Error);
		dovi_set_dovi2hdr10_mapping(dovi2hdr10_mapping);
		dovi_default("set dovi2hdr10_mapping = %d\n",
			dovi2hdr10_mapping);
	} else if (strncmp(opt, "vdo_max_lum:", 12) == 0) {
		unsigned int value1 = 0;
		unsigned int value2 = 0;

		p = (char *)opt + 12;
		STR_CVT_U32(&p, &value1, goto Error);
		STR_CVT_U32(&p, &value2, goto Error);

		set_video_max_lum_enable = value1;
		video_max_lum = value2;
		dovi_printf("set %d, video_max_lum %d.\n",
			set_video_max_lum_enable, video_max_lum);
	} else {
		dovi_error("test debug cmd pass.\n");
		goto Error;
	}

	return;
Error:
	dovi_error("parse command error!%s\n",
		DOVI_STR_HELP);
}

static void dovi_process_dbg_cmd_simplify(char *cmd)
{
	char *tok = NULL;

	dovi_printf("cmd: %s\n", cmd);
	/*memset(dovi_dbg_buf, 0, sizeof(dovi_dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		dovi_printf("parse: %s\n", tok);
		dovi_process_dbg_opt_simplify(tok);
	}
}

static int dovi_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t dovi_debug_read(struct file *file,
	char __user *ubuf, size_t count, loff_t *ppos)
{
	if (strlen(dovi_dbg_buf))
		return simple_read_from_buffer(ubuf, count,
		ppos, dovi_dbg_buf,
		strlen(dovi_dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count,
		ppos, DOVI_STR_HELP,
		strlen(DOVI_STR_HELP));

}

static ssize_t dovi_debug_write(struct file *file,
	const char __user *ubuf, size_t count,
				loff_t *ppos)
{
	const int debug_bufmax = sizeof(dovi_cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		return ret;

	if (copy_from_user(&dovi_cmd_buf, ubuf, count))
		return ret;

	dovi_cmd_buf[count] = 0;

	dovi_process_dbg_cmd(dovi_cmd_buf);

	return ret;
}

static const struct file_operations dovi_debug_fops = {
	.read = dovi_debug_read,
	.write = dovi_debug_write,
	.open = dovi_debug_open,
};

static ssize_t dovi_debug_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int count = 0;

	if (strlen(dovi_dbg_buf)) {
		count = strlen(dovi_dbg_buf);
		if (!strncpy(buf, dovi_dbg_buf, count))
			return -EFAULT;
	} else {
		count = strlen(DOVI_STR_HELP);
		if (!strncpy(buf, DOVI_STR_HELP, count))
			return -EFAULT;
	}

	return count;
}

static ssize_t dovi_debug_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	const int debug_bufmax = sizeof(dovi_cmd_buf) - 1;
	size_t ret = 0;

	ret = count;
	if (count > debug_bufmax)
		return -EINVAL;

	if (!strncpy(dovi_cmd_buf, buf, count))
		return -EFAULT;

	dovi_cmd_buf[count] = 0;
	dovi_process_dbg_cmd_simplify(dovi_cmd_buf);

	return count;
}

static struct kobj_attribute dovi_debug_sysfs = {
	.attr = {
		.name = "dovi",
		.mode = 0664,
	},
	.show  = dovi_debug_show,
	.store = dovi_debug_store,
};

void dovi_debug_init(void)
{
	int retval = 0;

	if (!dovi_dbg_init) {
		dovi_dbg_init = 1;
		dovi_debugfs = debugfs_create_file("dovi",
			S_IFREG | 0444, NULL, (void *)0,
			&dovi_debug_fops);
	}

	/* register sysfs */
	retval = sysfs_create_file(kernel_kobj, &dovi_debug_sysfs.attr);
	if (retval) {
		sysfs_remove_file(kernel_kobj, &dovi_debug_sysfs.attr);
		dovi_error("dovi sysfs file create fail!\n");
	}
}

void dovi_debug_deinit(void)
{
	debugfs_remove(dovi_debugfs);
	sysfs_remove_file(kernel_kobj, &dovi_debug_sysfs.attr);
}
