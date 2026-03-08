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

#ifndef LOG_TAG
#define LOG_TAG "R2R_DBG"
#endif

#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/time.h>
#include "disp_hw_log.h"
#include "disp_path.h"
#include "fmt_hal.h"
#include "vdout_sys_hal.h"
#include "disp_r2r_drv.h"
#include "disp_r2r_debug.h"
#include "disp_r2r_if.h"

/* ---------------------------------------------------------------------------
 */
/* External variable declarations */
/* ---------------------------------------------------------------------------
 */
/* ---------------------------------------------------------------------------
 */
/* Debug Options */
/* ---------------------------------------------------------------------------
 */

static struct dentry *r2r_debugfs;
struct r2r_video_buffer_info debug_buf_info;
static int r2r_debug_inited;

static char R2R_STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/r2r\n"
	"ACTION:\n"
	"       regr:addr                :regr:0x15006000\n"
	"       regw:addr,value          :regw:0x15006000,0x1\n"
	"       dump_reg:addr,length     :dump_reg:0x15006000,0x1000\n";
/* ---------------------------------------------------------------------------
 */
/* Command Processor */
/* ---------------------------------------------------------------------------
 */
static char r2r_dbg_buf[2048];
static char r2r_cmd_buf[512];
#ifndef STR_CONVERT
#define STR_CONVERT(p, val, base, action)                 \
	do {                                                  \
		int ret = 0;                                      \
		const char *tmp;                                  \
		tmp = strsep(p, ",");                             \
		if (tmp == NULL)                                  \
			break;                                        \
		if (strcmp(#base, "int") == 0)                    \
			ret = kstrtoint(tmp, 0, (int *)val);          \
		else if (strcmp(#base, "uint") == 0)              \
			ret = kstrtouint(tmp, 0, (unsigned int *)val);\
		else if (strcmp(#base, "ul") == 0)                \
			ret = kstrtoul(tmp, 0, (unsigned long *)val); \
		if (ret != 0) {                                   \
			r2r_default(                                   \
	"[ERROR]kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n", \
				ret, __FILE__, __LINE__); \
			action; \
		} \
	} while (0)
#endif

static unsigned int r2r_read_reg(uintptr_t addr)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

	/*0x15006000 -> 0x15006fff */
	reg_value = ReadREG32(addr);

	return reg_value;
}

static unsigned int r2r_write_reg(uintptr_t addr, unsigned int value)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

	/*0x15006000 -> 0x15006fff */
	WriteREG32(addr, value);
	return reg_value;
}

static void r2r_process_dbg_opt(const char *opt)
{
	if (strncmp(opt, "regr:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int regVal = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		regVal = r2r_read_reg(addr);
		r2r_default("regr: 0x%x = 0x%08x\n", addr, regVal);
	} else if (strncmp(opt, "regw:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int val = 0;
		unsigned int regVal = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &val, uint, goto Error);

		r2r_write_reg(addr, val);
		regVal = r2r_read_reg(addr);
		r2r_default("regw: 0x%x, 0x%08x = 0x%08x\n", addr, val, regVal);

	} else if (strncmp(opt, "regr_va:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int addr = 0;
		unsigned int size = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &size, uint, goto Error);
		r2r_default("regr_va: 0x%x, %d\n", addr, size);
		disp_r2r_drv_read_reg(addr, size);

	} else if (strncmp(opt, "regw_va:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int addr = 0;
		unsigned int val = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &val, uint, goto Error);
		r2r_default("regw_va: 0x%x, 0x%x\n", addr, val);
		disp_r2r_drv_write_reg(addr, val);

	} else if (strncmp(opt, "enable:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int mode = 0;
		unsigned int enable = 0;

		STR_CONVERT(&p, &enable, uint, goto Error);
		STR_CONVERT(&p, &mode, uint, goto Error);
		disp_r2r_drv_enable(enable, mode);
		DISP_LOG_I(" set r2r %d enable %d\n", enable, mode);

	} else if (strncmp(opt, "dbg_lvl:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int level = 0;
		unsigned int enable = 0;

		STR_CONVERT(&p, &level, uint, goto Error);
		STR_CONVERT(&p, &enable, uint, goto Error);
		r2r_default("set r2r debug level %d enable 0x%X\n", level,
			enable);
		disp_r2r_dbg_level_enable(level, enable);

	} else if (strncmp(opt, "patgen:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int res = 0;
		unsigned int enable = 0;
		unsigned int y_data = 0;
		unsigned int cb_data = 0;
		unsigned int cr_data = 0;

		STR_CONVERT(&p, &res, uint, goto Error);
		STR_CONVERT(&p, &enable, uint, goto Error);
		STR_CONVERT(&p, &y_data, uint, goto Error);
		STR_CONVERT(&p, &cb_data, uint, goto Error);
		STR_CONVERT(&p, &cr_data, uint, goto Error);
		disp_r2r_drv_set_pattern(res, enable, y_data, cb_data, cr_data);
		r2r_default("set r2r pattern gen %d\n", res);

	} else if (strncmp(opt, "tg:", 3) == 0) {
		char *p = (char *)opt + 3;
		unsigned int res = 0;

		STR_CONVERT(&p, &res, uint, goto Error);
		disp_r2r_drv_set_resolution(0, res);
		r2r_default("set r2r res %d\n", res);
	} else if (strncmp(opt, "conf_mode:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int mode = 0;

		STR_CONVERT(&p, &mode, uint, goto Error);
		disp_r2r_drv_set_conf_mode(mode);
		r2r_default("set r2r conf mode %d\n", mode);
	} else if (strncmp(opt, "frame:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int yaddr = 0;
		unsigned int caddr = 0;
		unsigned int ccaddr = 0;

		STR_CONVERT(&p, &yaddr, uint, goto Error);
		STR_CONVERT(&p, &caddr, uint, goto Error);
		STR_CONVERT(&p, &ccaddr, uint, goto Error);
		debug_buf_info.r2r_yaddr = yaddr;
		debug_buf_info.r2r_caddr = caddr;
		debug_buf_info.r2r_ccaddr = ccaddr;
		debug_buf_info.bit_depth = DEPTH_8_BIT;
		debug_buf_info.layer_id = 0;
		debug_buf_info.src_fmt = DISP_R2R_COLOR_FORMAT_YUV444;
		debug_buf_info.hdr_type = DISP_DR_TYPE_SDR;
		debug_buf_info.src.width = 1920;
		debug_buf_info.src.height = 1080;
		disp_r2r_drv_config_frame(&debug_buf_info);
		r2r_default("config frame r2r %x %x %x\n",
			yaddr, caddr, ccaddr);
	} else if (strncmp(opt, "start:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int enabled = 0;

		STR_CONVERT(&p, &enabled, uint, goto Error);
		if (enabled > 0)
			disp_r2r_start(NULL, 0);
		else
			disp_r2r_stop(0);
		r2r_default("set r2r start %d done\n", enabled);
	} else if (strncmp(opt, "test:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int test_no = 0;

		STR_CONVERT(&p, &test_no, uint, goto Error);
		disp_r2r_drv_set_test(test_no);
		r2r_default("set r2r testcase %d\n", test_no);
	} else {
		r2r_error(
			"parse command error!\n%s\n\n%s sizeof(R2R_STR_HELP) %d\n",
			opt, R2R_STR_HELP, (uint32_t)sizeof(R2R_STR_HELP));
	}
	return;
Error:
	r2r_error("parse command error!\n%s\n\n%s", opt, R2R_STR_HELP);
}

static void r2r_process_dbg_cmd(char *cmd)
{
	char *tok;

	r2r_default("cmd: %s\n", cmd);
	/*memset(dbg_buf, 0, sizeof(dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		r2r_default("parse: %s\n", tok);
		r2r_process_dbg_opt(tok);
	}
}

/* ---------------------------------------------------------------------------
 */
/* Debug FileSystem Routines */
/* ---------------------------------------------------------------------------
 */

static int r2r_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t r2r_debug_read(struct file *file, char __user *ubuf,
	size_t count, loff_t *ppos)
{
	if (strlen(r2r_dbg_buf))
		return simple_read_from_buffer(ubuf, count, ppos, r2r_dbg_buf,
					       strlen(r2r_dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count, ppos, R2R_STR_HELP,
					       strlen(R2R_STR_HELP));
}

static ssize_t r2r_debug_write(struct file *file,
	const char __user *ubuf,
	size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(r2r_cmd_buf) - 1;
	size_t ret;

	ret = count;
	if (count > debug_bufmax)
		return -EFAULT;

	if (copy_from_user(&r2r_cmd_buf, ubuf, count))
		return -EFAULT;

	r2r_cmd_buf[count] = 0;
	r2r_process_dbg_cmd(r2r_cmd_buf);

	return ret;
}


static const struct file_operations r2r_debug_fops = {
	.read = r2r_debug_read,
	.write = r2r_debug_write,
	.open = r2r_debug_open,
};

void r2r_debug_init(void)
{
	if (!r2r_debug_inited) {
		r2r_debug_inited = 1;
		r2r_debugfs =
			debugfs_create_file("r2r", S_IFREG | 0444, NULL,
			(void *)0, &r2r_debug_fops);
	}
}

void r2r_debug_deinit(void)
{
	debugfs_remove(r2r_debugfs);
	r2r_debug_inited = 0;
}
