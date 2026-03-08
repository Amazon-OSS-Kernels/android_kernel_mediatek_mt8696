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
#define LOG_TAG "CFD_DBG"
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
#include "disp_cfd_debug.h"
#include "disp_cfd_main.h"

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

static struct dentry *cfd_debugfs;

static int cfd_debug_inited;

static char CFD_STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/cfd\n"
	"ACTION:\n"
	"       regr:addr                :regr:0x15012000\n"
	"       regw:addr,value          :regw:0x15012000,0x1\n"
	"       dump_reg:addr,length     :dump_reg:0x15012000,0x1000\n";
/* ---------------------------------------------------------------------------
 */
/* Command Processor */
/* ---------------------------------------------------------------------------
 */
static char cfd_dbg_buf[2048];
static char cfd_cmd_buf[512];
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
			cfd_default(                                   \
	"[ERROR]kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n", \
				ret, __FILE__, __LINE__); \
			action; \
		} \
	} while (0)
#endif

static unsigned int cfd_read_reg(uintptr_t addr)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

  /*0x14010000 -> 0x1401ffff */
	reg_value = ReadREG32(addr);

	return reg_value;
}

static unsigned int cfd_write_reg(uintptr_t addr, unsigned int value)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

	/*0x14010000 -> 0x14010fff */
	WriteREG32(addr, value);
	return reg_value;
}

static void cfd_process_dbg_opt(const char *opt)
{
	if (strncmp(opt, "regr:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int regVal = 0;

		STR_CVT_U32(&p, &addr, goto Error);
		regVal = cfd_read_reg(addr);
		cfd_default("regr: 0x%x = 0x%08x\n", addr, regVal);
	} else if (strncmp(opt, "regw:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int val = 0;
		unsigned int regVal = 0;

		STR_CVT_U32(&p, &addr, goto Error);
		STR_CVT_U32(&p, &val, goto Error);

		cfd_write_reg(addr, val);
		regVal = cfd_read_reg(addr);
		cfd_default("regw: 0x%x, 0x%08x = 0x%08x\n", addr, val, regVal);

	} else if (strncmp(opt, "regr_va:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int client = 0;
		unsigned int addr = 0;
		unsigned int size = 0;

		STR_CVT_U32(&p, &client, goto Error);
		STR_CVT_U32(&p, &addr, goto Error);
		STR_CVT_U32(&p, &size, goto Error);
		cfd_default("regr_va: %d, 0x%x, %d\n", client, addr, size);
		disp_cfd_drv_read_reg(client, addr, size);

	} else if (strncmp(opt, "regw_va:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int client = 0;
		unsigned int addr = 0;
		unsigned int val = 0;

		STR_CVT_U32(&p, &client, goto Error);
		STR_CVT_U32(&p, &addr, goto Error);
		STR_CVT_U32(&p, &val, goto Error);
		cfd_default("regw_va:%d, 0x%x, 0x%x\n", client, addr, val);
		disp_cfd_drv_write_reg(client, addr, val);

	} else if (strncmp(opt, "dbg_lvl:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int level = 0;
		unsigned int enable = 0;

		STR_CVT_U32(&p, &level, goto Error);
		STR_CVT_U32(&p, &enable, goto Error);
		cfd_default("set cfd debug level %d enable 0x%X\n", level,
			   enable);
		disp_cfd_dbg_lvl_enable(level, enable);

	} else if (strncmp(opt, "tz_dbg_lvl:", 11) == 0) {
		char *p = (char *)opt + 11;
		unsigned int level = 0;
		unsigned int enable = 0;

		STR_CVT_U32(&p, &level, goto Error);
		STR_CVT_U32(&p, &enable, goto Error);
		cfd_default("set cfd tz debug level %d enable 0x%X\n", level,
			   enable);
		disp_cfd_tz_dbg_lvl_enable(level, enable);

	} else if (strncmp(opt, "tz_init:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int enable = 0;

		STR_CVT_U32(&p, &enable, goto Error);
		cfd_default("set cfd tz_init %d\n", enable);
		disp_cfd_set_tz_init(enable);

	} else if (strncmp(opt, "conf_mode:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int layer_id = 0;
		unsigned int mode = 0;

		STR_CVT_U32(&p, &layer_id, goto Error);
		STR_CVT_U32(&p, &mode, goto Error);
		disp_cfd_set_conf_mode(layer_id, mode);
		cfd_default("set cfd conf mode %d %d done\n", layer_id, mode);

	} else if (strncmp(opt, "flush:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int flush_mode = 0;
		unsigned int layer_id = 0;

		STR_CVT_U32(&p, &layer_id, goto Error);
		STR_CVT_U32(&p, &flush_mode, goto Error);
		disp_cfd_set_flush_conf(layer_id, flush_mode);
		cfd_default("set cfd flush irq %d %d done\n",
			layer_id, flush_mode);

	} else if (strncmp(opt, "forcehdr:", 9) == 0) {
		char *p = (char *)opt + 9;
		unsigned int force_openhdr = 0;
		unsigned int layer_id = 0;

		STR_CVT_U32(&p, &layer_id, goto Error);
		STR_CVT_U32(&p, &force_openhdr, goto Error);
		disp_cfd_drv_set_forcehdr(layer_id, force_openhdr);
		cfd_default("set cfd force open hdr %d %d done\n",
			layer_id, force_openhdr);

	} else if (strncmp(opt, "status:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int layer_id = 0;

		STR_CVT_U32(&p, &layer_id, goto Error);
		disp_cfd_drv_dump_status(layer_id);
		cfd_default("dump cfd %d status done\n",
			layer_id);

	} else if (strncmp(opt, "start:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int layer_id = 0;
		unsigned int enabled = 0;

		STR_CVT_U32(&p, &layer_id, goto Error);
		STR_CVT_U32(&p, &enabled, goto Error);
		if (enabled > 0) {
			disp_cfd_start(NULL, layer_id);
			disp_cfd_drv_set_bypass(layer_id);
			disp_cfd_drv_set_r2y(layer_id, 0);
		} else
			disp_cfd_stop(layer_id);
		cfd_default("set cfd start %d %d done\n",
			layer_id, enabled);

	} else if (strncmp(opt, "ctrlbit:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int layer_id = 0;
		unsigned int ctrl_bit = 0;

		STR_CVT_U32(&p, &layer_id, goto Error);
		STR_CVT_U32(&p, &ctrl_bit, goto Error);
		disp_cfd_drv_set_feature_ctrlbit(layer_id, ctrl_bit);
		cfd_default("set cfd %d ctrlbit %x\n",
			layer_id, ctrl_bit);

	} else {
		cfd_error(
			"parse command error!\n%s\n\n%s sizeof(CFD_STR_HELP) %d\n",
			opt, CFD_STR_HELP, (uint32_t)sizeof(CFD_STR_HELP));
	}
	return;
Error:
	cfd_error("parse command error!\n%s\n\n%s", opt, CFD_STR_HELP);
}

static void cfd_process_dbg_cmd(char *cmd)
{
	char *tok;

	cfd_default("cmd: %s\n", cmd);
	/*memset(dbg_buf, 0, sizeof(dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		cfd_default("parse: %s\n", tok);
		cfd_process_dbg_opt(tok);
	}
}

/* ---------------------------------------------------------------------------
 */
/* Debug FileSystem Routines */
/* ---------------------------------------------------------------------------
 */

static int cfd_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t cfd_debug_read(struct file *file, char __user *ubuf,
	size_t count, loff_t *ppos)
{
	if (strlen(cfd_dbg_buf))
		return simple_read_from_buffer(ubuf, count, ppos, cfd_dbg_buf,
					       strlen(cfd_dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count, ppos, CFD_STR_HELP,
					       strlen(CFD_STR_HELP));
}

static ssize_t cfd_debug_write(struct file *file,
	const char __user *ubuf,
	size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(cfd_cmd_buf) - 1;
	size_t ret;

	ret = count;
	if (count > debug_bufmax)
		return ret;

	if (copy_from_user(&cfd_cmd_buf, ubuf, count))
		return -EFAULT;

	cfd_cmd_buf[count] = 0;
	cfd_process_dbg_cmd(cfd_cmd_buf);

	return ret;
}

static const struct file_operations cfd_debug_fops = {
	.read = cfd_debug_read,
	.write = cfd_debug_write,
	.open = cfd_debug_open,
};

void cfd_debug_init(void)
{
	if (!cfd_debug_inited) {
		cfd_debug_inited = 1;
		cfd_debugfs =
			debugfs_create_file("cfd", S_IFREG | 0444, NULL,
				(void *)0, &cfd_debug_fops);
	}
}

void cfd_debug_deinit(void)
{
	debugfs_remove(cfd_debugfs);
	cfd_debug_inited = 0;
}
