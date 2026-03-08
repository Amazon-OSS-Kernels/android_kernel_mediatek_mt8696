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
#define LOG_TAG "BEFIFO_DBG"
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
#include "disp_befifo_debug.h"
#include "disp_befifo_drv.h"

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

static struct dentry *befifo_debugfs;

static int befifo_debug_inited;

static char BEFIFO_STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/befifo\n"
	"ACTION:\n"
	"       regr:addr                :regr:0x1400f000\n"
	"       regw:addr,value          :regw:0x1400f000,0x1\n"
	"       dump_reg:addr,length     :dump_reg:0x1400f000,0x1000\n";
/* ---------------------------------------------------------------------------
 */
/* Command Processor */
/* ---------------------------------------------------------------------------
 */
static char befifo_dbg_buf[2048];
static char befifo_cmd_buf[512];

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
			befifo_default(                                   \
	"[ERROR]kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n", \
				ret, __FILE__, __LINE__); \
			action; \
		} \
	} while (0)
#endif

static unsigned int befifo_read_reg(uintptr_t addr)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

	/*0x1400f000 -> 0x1400ffff */
	reg_value = ReadREG32(addr);

	return reg_value;
}

static unsigned int befifo_write_reg(uintptr_t addr,
	unsigned int value)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

	/*0x1400f000 -> 0x1400ffff */
	WriteREG32(addr, value);
	return reg_value;
}

static void befifo_process_dbg_opt(const char *opt)
{
	if (strncmp(opt, "regr:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int regVal = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		regVal = befifo_read_reg((uintptr_t)addr);
		befifo_default("regr: 0x%x = 0x%08x\n", addr, regVal);
	} else if (strncmp(opt, "regw:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int val = 0;
		unsigned int regVal = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &val, uint, goto Error);

		befifo_write_reg((uintptr_t)addr, val);
		regVal = befifo_read_reg((uintptr_t)addr);
		befifo_default("regw: 0x%x, 0x%08x = 0x%08x\n",
			addr, val, regVal);
	} else if (strncmp(opt, "regr_va:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int addr = 0;
		unsigned int size = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &size, uint, goto Error);
		befifo_default("regr_va: 0x%x, %d\n", addr, size);
		disp_befifo_drv_read_reg(addr, size);

	} else if (strncmp(opt, "regw_va:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int addr = 0;
		unsigned int val = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &val, uint, goto Error);
		befifo_default("regw_va: 0x%x, 0x%x\n", addr, val);
		disp_befifo_drv_write_reg(addr, val);

	} else if (strncmp(opt, "enable:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int mode = 0;
		unsigned int enable = 0;

		STR_CONVERT(&p, &enable, uint, goto Error);
		STR_CONVERT(&p, &mode, uint, goto Error);
		disp_befifo_drv_enable(enable, mode);
		befifo_default(" set befifo %d enable %d\n", enable, mode);

	} else if (strncmp(opt, "dbg_lvl:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int level = 0;
		unsigned int enable = 0;

		STR_CONVERT(&p, &level, uint, goto Error);
		STR_CONVERT(&p, &enable, uint, goto Error);
		befifo_default("set befifo debug level %d enable 0x%x\n",
			level, enable);
		disp_befifo_dbg_level_enable(level, enable);

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
		disp_befifo_drv_set_pattern(res,
			enable, y_data, cb_data, cr_data);
		befifo_default("set befifo pattern gen %d\n", res);
	} else if (strncmp(opt, "tg:", 3) == 0) {
		char *p = (char *)opt + 3;
		unsigned int res = 0;

		STR_CONVERT(&p, &res, uint, goto Error);
		disp_befifo_drv_set_resolution(0, res);
		befifo_default("set befifo res %d\n", res);

	} else if (strncmp(opt, "front:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int front_h = 0;
		unsigned int front_v = 0;

		STR_CONVERT(&p, &front_h, uint, goto Error);
		STR_CONVERT(&p, &front_v, uint, goto Error);
		disp_befifo_drv_set_front(front_h, front_v);

		befifo_default("set befifo %d %d\n", front_h, front_v);

	} else if (strncmp(opt, "conf_mode:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int mode = 0;

		STR_CONVERT(&p, &mode, uint, goto Error);
		disp_befifo_drv_set_conf_mode(mode);
		befifo_default("set befifo conf mode %d\n", mode);

	} else if (strncmp(opt, "flush:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int flush_mode = 0;

		STR_CONVERT(&p, &flush_mode, uint, goto Error);
		disp_befifo_drv_flush_reg(flush_mode);
		befifo_default("set befifo flush mode %d\n", flush_mode);

	} else if (strncmp(opt, "swap:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int swap_type = 0;

		STR_CONVERT(&p, &swap_type, uint, goto Error);
		disp_befifo_drv_set_output_swap(swap_type);
		befifo_default("set befifo out swap %d\n", swap_type);

	} else if (strncmp(opt, "crc:", 4) == 0) {
		char *p = (char *)opt + 4;
		unsigned int crc_enable = 0;
		unsigned int crc_mode = 0;

		STR_CONVERT(&p, &crc_enable, uint, goto Error);
		STR_CONVERT(&p, &crc_mode, uint, goto Error);
		disp_befifo_drv_enable_crc(crc_enable, crc_mode);
		befifo_default("set befifo crc %d %d\n", crc_enable, crc_mode);

	} else {
		befifo_error(
		"parse command error!\n%s\n\n%s sizeof(BEFIFO_STR_HELP) %d\n",
		opt, BEFIFO_STR_HELP, (uint32_t)sizeof(BEFIFO_STR_HELP));
	}
	return;
Error:
	befifo_error("parse command error!\n%s\n\n%s",
		opt, BEFIFO_STR_HELP);
}

static void befifo_process_dbg_cmd(char *cmd)
{
	char *tok;

	befifo_default("cmd: %s\n", cmd);
	/*memset(dbg_buf, 0, sizeof(dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		befifo_default("parse: %s\n", tok);
		befifo_process_dbg_opt(tok);
	}
}

/* ---------------------------------------------------------------------------
 */
/* Debug FileSystem Routines */
/* ---------------------------------------------------------------------------
 */

static int befifo_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t befifo_debug_read(struct file *file,
	char __user *ubuf,
	size_t count, loff_t *ppos)
{
	if (strlen(befifo_dbg_buf))
		return simple_read_from_buffer(ubuf, count, ppos,
		befifo_dbg_buf, strlen(befifo_dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count, ppos,
		BEFIFO_STR_HELP, strlen(BEFIFO_STR_HELP));
}

static ssize_t befifo_debug_write(struct file *file,
	const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(befifo_cmd_buf) - 1;
	size_t ret;

	ret = count;
	if (count > debug_bufmax)
		return -EFAULT;

	if (copy_from_user(&befifo_cmd_buf, ubuf, count))
		return -EFAULT;

	befifo_cmd_buf[count] = 0;
	befifo_process_dbg_cmd(befifo_cmd_buf);

	return ret;
}

static const struct file_operations befifo_debug_fops = {
	.read = befifo_debug_read,
	.write = befifo_debug_write,
	.open = befifo_debug_open,
};

void befifo_debug_init(void)
{
	if (!befifo_debug_inited) {
		befifo_debug_inited = 1;
		befifo_debugfs =
			debugfs_create_file("befifo", S_IFREG | 0444, NULL,
					    (void *)0, &befifo_debug_fops);
	}
}

void befifo_debug_deinit(void)
{
	debugfs_remove(befifo_debugfs);
	befifo_debug_inited = 0;
}
