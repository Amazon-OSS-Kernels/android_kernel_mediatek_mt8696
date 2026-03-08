// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/time.h>

#include "disp_adl_if.h"
#include "disp_fg_debug.h"
#include "disp_fg_if.h"
#include "disp_fg_drv.h"
#include "fg_hal.h"

static struct dentry *fg_debugfs;
static int fg_debug_inited;
static bool fg_dump_param_enable;

static char FG_STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/fg\n"
	"ACTION:\n"
	"       fg_en:enable                fg_en:0/1\n";

/* raw data + \n */
static char *fg_dbg_buf;
static char fg_cmd_buf[512];

struct mtk_av1_film_grain_params fg_param = {
	.apply_grain = 1,
	.grain_seed = 7391,
	.update_grain = 1,
	.film_grain_params_ref_idx = 0,
	.num_y_points = 4,
	.point_y_value = {0, 27, 67, 255},
	.point_y_scaling = {112, 112, 72, 49},
	.chroma_scaling_from_luma = 0,
	.num_cb_points = 4,
	.point_cb_value = {0, 27, 94, 255},
	.point_cb_scaling = {41, 41, 15, 11},
	.num_cr_points = 3,
	.point_cr_value = {0, 54, 255},
	.point_cr_scaling = {18, 17, 7},
	.grain_scaling = 3 + 8,
	.ar_coeff_lag = 3,
	.ar_coeffs_y = {
			127,
			128,
			125,
			142,
			129,
			127,
			128,
			128,
			131,
			130,
			116,
			126,
			132,
			127,
			126,
			129,
			130,
			159,
			138,
			129,
			130,
			143,
			117,
			161},
	.ar_coeffs_cb = {
			128,
			128,
			124,
			139,
			132,
			128,
			128,
			128,
			129,
			140,
			97,
			117,
			130,
			128,
			128,
			133,
			86,
			205,
			142,
			124,
			129,
			134,
			103,
			211,
			137},
	.ar_coeffs_cr = {
			128,
			127,
			126,
			139,
			132,
			128,
			128,
			128,
			130,
			137,
			99,
			120,
			129,
			128,
			128,
			132,
			96,
			203,
			137,
			126,
			130,
			134,
			111,
			196,
			118},
	.ar_coeff_shift = 0x7,
	.grain_scale_shift = 0,
	.cb_mult = 128,
	.cb_luma_mult = 192,
	.cb_offset = 256,
	.cr_mult = 128,
	.cr_luma_mult = 192,
	.cr_offset = 256,
	.overlap_flag = 1,
	.clip_to_restricted_range = 0,
};

struct mtk_av1_film_grain_params fg_param_t0009 = {
	.apply_grain = 1,
	.grain_seed = 0xB0AF,
	.update_grain = 1,
	.film_grain_params_ref_idx = 0,
	.num_y_points = 8,
	.point_y_value = {16, 58, 87, 97, 112, 126, 141, 199},
	.point_y_scaling = {0, 126, 120, 122, 125, 131, 139, 153},
	.chroma_scaling_from_luma = 0,
	.num_cb_points = 8,
	.point_cb_value = {16, 59, 66, 73, 79, 86, 151, 192},
	.point_cb_scaling = {0, 68, 76, 82, 85, 86, 95, 101},
	.num_cr_points = 8,
	.point_cr_value = {16, 59, 89, 99, 114, 129, 144, 203},
	.point_cr_scaling = {0, 64, 80, 86, 90, 93, 97, 85},
	.grain_scaling = 0xA,
	.ar_coeff_lag = 3,
	.ar_coeffs_y = {
			0x04,
			0x01,
			0x03,
			0x00,
			0x01,
			0xFD,
			0x08,
			0xFD,
			0x07,
			0xE9,
			0x01,
			0xE7,
			0x00,
			0xF6,
			0x06,
			0xEF,
			0xFC,
			0x35,
			0x24,
			0x05,
			0xFB,
			0xEF,
			0x08,
			0x42,},
	.ar_coeffs_cb = {
			0x00,
			0xFE,
			0xFE,
			0x08,
			0x05,
			0xFF,
			0x01,
			0xFF,
			0x05,
			0x10,
			0xDF,
			0xF7,
			0x06,
			0xFF,
			0xFD,
			0x0A,
			0xD1,
			0x3F,
			0x00,
			0xF1,
			0x03,
			0x0B,
			0xD6,
			0x4B,
			0xBB},
	.ar_coeffs_cr = {
			0x01,
			0xFF,
			0xFF,
			0x09,
			0x05,
			0x00,
			0x01,
			0xFF,
			0x05,
			0x0F,
			0xE0,
			0xF6,
			0x08,
			0xFE,
			0xFC,
			0x0B,
			0xD2,
			0x3E,
			0x01,
			0xF0,
			0x03,
			0x0D,
			0xD5,
			0x4B,
			0xC9},
	.ar_coeff_shift = 0x7,
	.grain_scale_shift = 2,
	.cb_mult = 0x80,
	.cb_luma_mult = 0xC0,
	.cb_offset = 0x100,
	.cr_mult = 0x80,
	.cr_luma_mult = 0xC0,
	.cr_offset = 0x100,
	.overlap_flag = 1,
	.clip_to_restricted_range = 0,
};

static u32 fg_alloc_dbg_buf(void)
{
	/* raw data 13358 * 2
	 * line count 1287 * 4
	 * \n 1287
	 */
	u32 buf_size = 2 * FILMG_SRC_LEN + 8 * FILMG_MAX_CMD_LEN;

	if (!fg_dbg_buf) {
		fg_dbg_buf = kzalloc(buf_size, GFP_KERNEL);
		FG_LOG_I("alloc buf 0x%p size %d\n", fg_dbg_buf, buf_size);
	}

	return buf_size;
}

static void fg_free_dbg_buf(void)
{
	if (fg_dbg_buf) {
		FG_LOG_I("free buf 0x%p\n", fg_dbg_buf);
		fg_dbg_buf = NULL;
	}

	kfree(fg_dbg_buf);
}

static u32 fg_dump_lut(u8 *dbg_buf,
		       u8 *p_data,
		       u32 buf_size,
		       u32 fg_idx)
{
	u32 idx = fg_idx;
	u8 tmp_buf[50];
	u32 tmp_buf_size = sizeof(tmp_buf);
	u32 line = 0;
	s32 tmp_idx = 0, tmp_len = 0;

	FG_LOG_I("fg lut\n");

	memset(tmp_buf, 0, tmp_buf_size);
	for (idx = 0; (idx < FG_TBL_LUT_SIZE) && (buf_size >= tmp_idx); idx += 3, line++) {
		tmp_len = snprintf(tmp_buf, tmp_buf_size,
			"[%04d] %02X%02X%02X\n",
			line,
			p_data[idx + 2],
			p_data[idx + 1],
			p_data[idx + 0]);
		if (tmp_len < 0 || tmp_len >= tmp_buf_size)
			break;
		tmp_idx += snprintf(dbg_buf + tmp_idx, buf_size - tmp_idx, "%s", tmp_buf);
		if (tmp_idx  < 0 || tmp_idx >= buf_size)
			break;
		pr_info("[%06d] [%02d] %s", tmp_idx, tmp_len, tmp_buf);
	}

	return idx;
}

static u32 fg_dump_y_nosie(u8 *dbg_buf,
			u8 *p_data,
			u32 buf_size,
			u32 fg_idx)
{
	u32 idx = fg_idx, idy = 0;
	u8 tmp_buf[50];
	u32 tmp_buf_size = sizeof(tmp_buf);
	u32 line = 0;
	s32 tmp_idx = 0, tmp_len = 0;

	FG_LOG_I("fg y noise\n");
	memset(tmp_buf, 0, tmp_buf_size);
	for (; (idx < FG_TBL_C_GNS_OFFSET) && (buf_size >= tmp_idx); idx += FILMG_YN_CMD, line++) {
		tmp_len = 0;
		for (idy = 0; idy < FILMG_YN_CMD; idy++) {
			tmp_len += snprintf(tmp_buf + tmp_len,
					    tmp_buf_size - tmp_len,
					    "%02X",
					    p_data[idx + FILMG_YN_CMD - 1 - idy]);
			if (tmp_len < 0 || tmp_len >= tmp_buf_size)
				break;
		}
		tmp_idx += snprintf(dbg_buf + tmp_idx,
				    buf_size - tmp_idx,
				    "[%04d] %s\n",
				    line,
				    tmp_buf);
		if (tmp_idx  < 0 || tmp_idx >= buf_size)
			break;
		pr_info("[%06d] [%02d] %s", tmp_idx, tmp_len, tmp_buf);
	}

	return idx;
}

static u32 fg_dump_c_noise(u8 *dbg_buf,
			    u8 *p_data,
			    u32 buf_size,
			    u32 fg_idx)
{
	u32 idx = fg_idx, idy = 0;
	u8 tmp_buf[50];
	u32 tmp_buf_size = sizeof(tmp_buf);
	u32 line = 0;
	s32 tmp_idx = 0, tmp_len = 0;

	FG_LOG_I("fg cbcr noise\n");
	memset(tmp_buf, 0, tmp_buf_size);
	for (; (idx < FILMG_SRC_LEN) && (buf_size >= tmp_idx); idx += FILMG_CBCRN_CMD, line++) {
		tmp_len = 0;
		for (idy = 0; idy < FILMG_CBCRN_CMD; idy++) {
			tmp_len += snprintf(tmp_buf + tmp_len,
					    tmp_buf_size - tmp_len,
					    "%02X",
					    p_data[idx + FILMG_CBCRN_CMD - 1 - idy]);
			if (tmp_len < 0 || tmp_len >= tmp_buf_size)
				break;
		}
		tmp_idx += snprintf(dbg_buf + tmp_idx,
				    buf_size - tmp_idx,
				    "[%04d] %s\n",
				    line,
				    tmp_buf);
		if (tmp_idx  < 0 || tmp_idx >= buf_size)
			break;
		pr_info("[%06d] [%02d] %s", tmp_idx, tmp_len, tmp_buf);
	}

	return idx;
}

static void fg_dump_adl_table(u32 hw_id)
{
	u8 *p_data;

	u8 *dbg_buf;
	u32 buf_size;

	u32 idx = 0;

	buf_size = fg_alloc_dbg_buf();

	dbg_buf = fg_dbg_buf;
	if (!dbg_buf) {
		FG_ERR("debug buf alloc fail!!!\n");
		return;
	}

	p_data = disp_fg_get_adl_tbl(hw_id);
	if (!p_data) {
		FG_ERR("dal table is null!!!\n");
		return;
	}

	FG_LOG_I("dbg_buf 0x%p size %d 0x%p\n", dbg_buf, buf_size, dbg_buf + buf_size);

	idx = fg_dump_lut(dbg_buf, p_data, buf_size, 0);

	idx = fg_dump_y_nosie(dbg_buf, p_data, buf_size, idx);

	idx = fg_dump_c_noise(dbg_buf, p_data, buf_size, idx);

	FG_LOG_I("dump adl table done idx %d\n", idx);

}

static void fg_dump_param(u32 hw_id)
{
	struct mtk_av1_film_grain_params *param;
	u32 idx = 0;

	param = disp_fg_get_param(hw_id);
	if (!param) {
		FG_ERR("dal table is null!!!\n");
		return;
	}

	FG_LOG_I("dump av1 film grain param\n");

	pr_info("apply_grain %d\n", param->apply_grain);
	pr_info("grain_seed %d\n", param->grain_seed);
	pr_info("update_grain %d\n", param->update_grain);

	pr_info("num_y_points %d\n", param->num_y_points);
	pr_info("point_y_value | point_y_scaling\n");
	for (idx = 0; idx < param->num_y_points; idx++)
		pr_info("%d	%d\n",
			param->point_y_value[idx],
			param->point_y_scaling[idx]);

	pr_info("chroma_scaling_from_luma %d\n", param->chroma_scaling_from_luma);

	pr_info("num_cb_points %d\n", param->num_cb_points);
	pr_info("point_cb_value | point_cb_scaling\n");
	for (idx = 0; idx < param->num_cb_points; idx++)
		pr_info("%d	%d\n",
			param->point_cb_value[idx],
			param->point_cb_scaling[idx]);

	pr_info("num_cr_points %d\n", param->num_cr_points);
	pr_info("num_cr_points | point_cr_scaling\n");
	for (idx = 0; idx < param->num_cr_points; idx++)
		pr_info("%d	%d\n",
			param->point_cr_value[idx],
			param->point_cr_scaling[idx]);

	pr_info("grain_scaling %d\n", param->grain_scaling);
	pr_info("ar_coeff_lag %d\n", param->ar_coeff_lag);

	pr_info("ar_coeffs_y | ar_coeffs_cb | ar_coeffs_cr\n");
	for (idx = 0; idx < AV1_MAX_AR_COEFFS_CNT; idx++)
		pr_info("%d	%d	%d\n",
			param->ar_coeffs_y[idx],
			param->ar_coeffs_cb[idx],
			param->ar_coeffs_cr[idx]);

	pr_info("ar_coeff_shift %d\n", param->ar_coeff_shift);
	pr_info("grain_scale_shift %d\n", param->grain_scale_shift);
	pr_info("cb_mult %d cb_luma_mult %d cb_offset %d\n",
		param->cb_mult,
		param->cb_luma_mult,
		param->cb_offset);
	pr_info("cr_mult %d cr_luma_mult %d cr_offset %d\n",
		param->cr_mult,
		param->cr_luma_mult,
		param->cr_offset);

	pr_info("overlap_flag %d clip_to_restricted_range %d\n",
		param->overlap_flag,
		param->clip_to_restricted_range);
}

void fg_dbg_dump_param(u32 hw_id)
{
	if (!fg_dump_param_enable)
		return;

	fg_dump_param(hw_id);
}

static void fg_process_dbg_opt(const char *opt)
{
	if (strncmp(opt, "fg_en:", 6) == 0) {
		char *p = (char *)opt + 6;
		struct mtk_av1_film_grain_params *param = NULL;
		u32 enable = 0;

		FG_STR_CONVERT(&p, &enable, uint, goto Error);
		FG_LOG_I("set fg enable %u\n", enable);

		if (enable == 1)
			param = &fg_param;
		else if (enable == 2)
			param = &fg_param_t0009;

		disp_fg_config(0, param);
	} else if (strncmp(opt, "dump_tbl:", 9) == 0) {
		char *p = (char *)opt + 9;
		u32 hw_id = 0;

		FG_STR_CONVERT(&p, &hw_id, uint, goto Error);
		FG_LOG_I("dump fg %d table\n", hw_id);

		fg_dump_adl_table(hw_id);
	} else if (strncmp(opt, "dump_param:", 11) == 0) {
		char *p = (char *)opt + 11;
		u32 hw_id = 0;

		FG_STR_CONVERT(&p, &hw_id, uint, goto Error);
		FG_LOG_I("dump fg %d table\n", hw_id);

		fg_dump_param(hw_id);
	} else if (strncmp(opt, "force_bypass:", 13) == 0) {
		char *p = (char *)opt + 13;
		u32 hw_id = 0;
		u32 bypass = 0;

		FG_STR_CONVERT(&p, &hw_id, uint, goto Error);
		FG_STR_CONVERT(&p, &bypass, uint, goto Error);
		FG_LOG_I("fg %d bypass %d\n", hw_id, bypass);

		disp_fg_force_bypass(hw_id, bypass);
	} else if (strncmp(opt, "dbg_lvl:", 8) == 0) {
		char *p = (char *)opt + 8;
		u32 level = 0;
		u32 enable = 0;

		FG_STR_CONVERT(&p, &level, uint, goto Error);
		FG_STR_CONVERT(&p, &enable, uint, goto Error);
		FG_LOG_I("set fg debug level %d enable %d\n", level, enable);

		disp_fg_set_dbg_level_enable(level, enable);
	} else if (strncmp(opt, "status", 6) == 0) {
		FG_LOG_I("get fg status\n");

		fg_sec_status();
	} else if (strncmp(opt, "sw_filter:", 10) == 0) {
		char *p = (char *)opt + 10;
		uint32_t enable = 0;
		bool sw_filter_enable = 0;

		FG_STR_CONVERT(&p, &enable, uint, goto Error);

		FG_LOG_I("set sw filter enable %u\n", enable);

		sw_filter_enable = enable ? true : false;

		disp_fg_sw_auto_reg_filter_enable(enable);
	} else if (strncmp(opt, "dump_reg:", 9) == 0) {
		char *p = (char *)opt + 9;
		u32 hw_id = 0;
		u32 len = 0;

		FG_STR_CONVERT(&p, &hw_id, uint, goto Error);
		FG_STR_CONVERT(&p, &len, uint, goto Error);

		fg_hal_reg_dump(hw_id, len);
	} else if (strncmp(opt, "write_reg:", 10) == 0) {
		char *p = (char *)opt + 10;
		u32 addr = 0;
		u32 val = 0;

		FG_STR_CONVERT(&p, &addr, uint, goto Error);
		FG_STR_CONVERT(&p, &val, uint, goto Error);

		fg_hal_reg_write(addr, val);
	} else if (strncmp(opt, "dump_param_en:", 14) == 0) {
		char *p = (char *)opt + 14;
		u32 dump_en = 0;

		FG_STR_CONVERT(&p, &dump_en, uint, goto Error);
		FG_LOG_I("dump param enable %u\n", dump_en);

		fg_dump_param_enable = dump_en ? true : false;
	}  else {
		FG_LOG_I(
			"parse command error!\n%s\n\n%s sizeof(FG_STR_HELP) %d\n",
			opt, FG_STR_HELP, (uint32_t)sizeof(FG_STR_HELP));
	}

	FG_LOG_I("done\n");
	return;

Error:
	FG_ERR("parse command error!\n%s\n\n%s", opt, FG_STR_HELP);
}


/* ---------------------------------------------------------------------------
 */
/* Debug FileSystem Routines */
/* ---------------------------------------------------------------------------
 */

static int fg_debug_open(struct inode *inode, struct file *file)
{
	FG_LOG_I("start\n");

	file->private_data = inode->i_private;
	return 0;
}

static ssize_t fg_debug_read(struct file *file, char __user *ubuf,
			      size_t count, loff_t *ppos)
{
	ssize_t read_size = 0;

	FG_LOG_I("start count %lu\n", count);

	if (fg_dbg_buf && strlen(fg_dbg_buf)) {
		read_size = simple_read_from_buffer(ubuf, count, ppos, fg_dbg_buf,
					       strlen(fg_dbg_buf));

		FG_LOG_I("start strlen %lu read_size %lu\n", strlen(fg_dbg_buf), read_size);

		if (read_size == 0)
			fg_free_dbg_buf();
	} else {
		read_size = simple_read_from_buffer(ubuf, count, ppos, FG_STR_HELP,
					       strlen(FG_STR_HELP));
	}

	return read_size;
}

static ssize_t fg_debug_write(struct file *file, const char __user *ubuf,
			       size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(fg_cmd_buf) - 1;
	size_t ret;

	FG_LOG_I("count %lu debug_bufmax %d\n", count, debug_bufmax);

	ret = count;

	if (count > debug_bufmax) {
		FG_ERR("count %lu debug_bufmax %d\n", count, debug_bufmax);
		count = debug_bufmax;
		return 0;
	}

	if (copy_from_user(&fg_cmd_buf, ubuf, count))
		return -EFAULT;

	fg_cmd_buf[count] = 0;

	fg_process_dbg_opt(fg_cmd_buf);

	FG_LOG_I("done\n");

	return ret;
}

static const struct file_operations fg_debug_fops = {
	.read = fg_debug_read,
	.write = fg_debug_write,
	.open = fg_debug_open,
};

void fg_debug_init(void)
{
	FG_FUNC();

	if (!fg_debug_inited) {
		fg_debug_inited = 1;

		fg_debugfs =
			debugfs_create_file("fg", S_IFREG | 0444, NULL,
					    (void *)0, &fg_debug_fops);
	}
}

void fg_debug_exit(void)
{
	FG_FUNC();

	debugfs_remove(fg_debugfs);
	fg_debug_inited = 0;
}
