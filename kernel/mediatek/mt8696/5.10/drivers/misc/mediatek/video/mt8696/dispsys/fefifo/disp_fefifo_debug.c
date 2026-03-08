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
#define LOG_TAG "FEFIFO_DBG"
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
#include "disp_fefifo_debug.h"
#include "disp_fefifo_drv.h"

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

static struct dentry *fefifo_debugfs;

static int fefifo_debug_inited;

static char FEFIFO_STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/fefifo\n"
	"ACTION:\n"
	"       regr:addr                :regr:0x14010000\n"
	"       regw:addr,value          :regw:0x14010000,0x1\n"
	"       dump_reg:addr,length     :dump_reg:0x14010000,0x1000\n";
/* ---------------------------------------------------------------------------
 */
/* Command Processor */
/* ---------------------------------------------------------------------------
 */
static char fefifo_dbg_buf[2048];
static char fefifo_cmd_buf[512];
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
			fefifo_default(                                   \
	"[ERROR]kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n", \
				ret, __FILE__, __LINE__); \
			action; \
		} \
	} while (0)
#endif

static unsigned int fefifo_read_reg(uintptr_t addr)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

  /*0x14010000 -> 0x1401ffff */
	reg_value = ReadREG32(addr);

	return reg_value;
}

static unsigned int fefifo_write_reg(uintptr_t addr, unsigned int value)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

	/*0x14010000 -> 0x14010fff */
	WriteREG32(addr, value);
	return reg_value;
}

static void fefifo_process_dbg_opt(const char *opt)
{
	int ret = 0;

	if (strncmp(opt, "regr:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int regVal = 0;

		ret = kstrtouint(p, 0, &addr);
		if (ret != 0) {
			fefifo_default("failed to paraser fefifo regr command param!\n");
			goto Error;
		}
		regVal = fefifo_read_reg(addr);
		fefifo_default("regr: 0x%x = 0x%08x\n", addr, regVal);
	} else if (strncmp(opt, "regw:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int val = 0;
		unsigned int regVal = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &val, uint, goto Error);

		fefifo_write_reg(addr, val);
		regVal = fefifo_read_reg(addr);
		fefifo_default("regw: 0x%x, 0x%08x = 0x%08x\n",
			addr, val, regVal);

	} else if (strncmp(opt, "regr_va:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int layer_id = 0;
		unsigned int addr = 0;
		unsigned int size = 0;

		STR_CONVERT(&p, &layer_id, uint, goto Error);
		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &size, uint, goto Error);
		fefifo_default("regr_va: %d, 0x%x, %d\n",
			layer_id, addr, size);
		disp_fefifo_drv_read_reg(layer_id, addr, size);

	} else if (strncmp(opt, "regw_va:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int layer_id = 0;
		unsigned int addr = 0;
		unsigned int val = 0;

		STR_CONVERT(&p, &layer_id, uint, goto Error);
		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &val, uint, goto Error);
		fefifo_default("regw_va:%d, 0x%x, 0x%x\n", layer_id, addr, val);
		disp_fefifo_drv_write_reg(layer_id, addr, val);

	} else if (strncmp(opt, "enable:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int mode = 0;
		unsigned int enable = 0;
		unsigned int layer_id = 0;

		STR_CONVERT(&p, &layer_id, uint, goto Error);
		STR_CONVERT(&p, &enable, uint, goto Error);
		STR_CONVERT(&p, &mode, uint, goto Error);
		disp_fefifo_drv_enable(layer_id, enable, mode);
		DISP_LOG_I(" set fefifo %d enable %d\n", enable, mode);

	} else if (strncmp(opt, "dbg_lvl:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int level = 0;
		unsigned int enable = 0;

		STR_CONVERT(&p, &level, uint, goto Error);
		STR_CONVERT(&p, &enable, uint, goto Error);
		fefifo_default("set fefifo debug level %d enable 0x%X\n", level,
			   enable);
		disp_fefifo_dbg_lvl_enable(level, enable);

	} else if (strncmp(opt, "patgen:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int layer_id = 0;
		unsigned int res = 0;
		unsigned int enable = 0;
		unsigned int y_data = 0;
		unsigned int cb_data = 0;
		unsigned int cr_data = 0;

		STR_CONVERT(&p, &layer_id, uint, goto Error);
		STR_CONVERT(&p, &res, uint, goto Error);
		STR_CONVERT(&p, &enable, uint, goto Error);
		STR_CONVERT(&p, &y_data, uint, goto Error);
		STR_CONVERT(&p, &cb_data, uint, goto Error);
		STR_CONVERT(&p, &cr_data, uint, goto Error);
		disp_fefifo_drv_set_pattern(layer_id, res, enable,
			y_data, cb_data, cr_data);
		fefifo_default("set fefifo pattern gen %d\n", res);
	} else if (strncmp(opt, "alpha:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int layer_id = 0;
		unsigned int alpha = 0;

		STR_CONVERT(&p, &layer_id, uint, goto Error);
		STR_CONVERT(&p, &alpha, uint, goto Error);
		disp_fefifo_drv_set_alpha(layer_id, alpha);
		fefifo_default("set fefifo %d alpha %d\n", layer_id, alpha);

	} else if (strncmp(opt, "inswap:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int layer_id = 0;
		unsigned int swap_order = 0;

		STR_CONVERT(&p, &layer_id, uint, goto Error);
		STR_CONVERT(&p, &swap_order, uint, goto Error);
		disp_fefifo_drv_set_input_order(layer_id, swap_order);
		DISP_LOG_I("set fefifo input swap %d  %d\n",
			layer_id, swap_order);

	} else if (strncmp(opt, "active:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int layer_id = 0;
		unsigned int width = 0;
		unsigned int height = 0;

		STR_CONVERT(&p, &layer_id, uint, goto Error);
		STR_CONVERT(&p, &width, uint, goto Error);
		STR_CONVERT(&p, &height, uint, goto Error);
		disp_fefifo_set_src_res(layer_id, width, height, 0);
		fefifo_default("set fefifo %d src  %d %d\n",
			layer_id, width, height);

	} else if (strncmp(opt, "conf_mode:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int layer_id = 0;
		unsigned int mode = 0;

		STR_CONVERT(&p, &layer_id, uint, goto Error);
		STR_CONVERT(&p, &mode, uint, goto Error);
		disp_fefifo_drv_set_conf_mode(layer_id, mode);
		fefifo_default("set fefifo %d conf mode  %d\n", layer_id, mode);

	} else {
		fefifo_error(
			"parse command error!\n%s\n\n%s sizeof(FEFIFO_STR_HELP) %d\n",
			opt, FEFIFO_STR_HELP,
			(uint32_t)sizeof(FEFIFO_STR_HELP));
	}
	return;
Error:
	fefifo_error("parse command error!\n%s\n\n%s", opt, FEFIFO_STR_HELP);
}

static void fefifo_process_dbg_cmd(char *cmd)
{
	char *tok;

	if ((cmd == NULL) || (strlen(cmd) > 512))
		return;
	fefifo_default("cmd: %s\n", cmd);
	/*memset(dbg_buf, 0, sizeof(dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		fefifo_default("parse: %s\n", tok);
		fefifo_process_dbg_opt(tok);
	}
}

/* ---------------------------------------------------------------------------
 */
/* Debug FileSystem Routines */
/* ---------------------------------------------------------------------------
 */

static int fefifo_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t fefifo_debug_read(struct file *file,
	char __user *ubuf,
	size_t count, loff_t *ppos)
{
	if (strlen(fefifo_dbg_buf))
		return simple_read_from_buffer(ubuf, count, ppos,
			fefifo_dbg_buf, strlen(fefifo_dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count, ppos,
			FEFIFO_STR_HELP, strlen(FEFIFO_STR_HELP));
}

static ssize_t fefifo_debug_write(struct file *file,
	const char __user *ubuf,
	size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(fefifo_cmd_buf) - 1;
	size_t ret;

	ret = count;
	if ((count > debug_bufmax) || (count == 0))
		return -EFAULT;

	if (copy_from_user(&fefifo_cmd_buf, ubuf, count))
		return -EFAULT;

	fefifo_cmd_buf[count] = 0;
	fefifo_process_dbg_cmd(fefifo_cmd_buf);

	return ret;
}

static const struct file_operations fefifo_debug_fops = {
	.read = fefifo_debug_read,
	.write = fefifo_debug_write,
	.open = fefifo_debug_open,
};

void fefifo_debug_init(void)
{
	if (!fefifo_debug_inited) {
		fefifo_debug_inited = 1;
		fefifo_debugfs =
			debugfs_create_file("fefifo", S_IFREG | 0444, NULL,
			(void *)0, &fefifo_debug_fops);
	}
}

void fefifo_debug_deinit(void)
{
	debugfs_remove(fefifo_debugfs);
	fefifo_debug_inited = 0;
}
