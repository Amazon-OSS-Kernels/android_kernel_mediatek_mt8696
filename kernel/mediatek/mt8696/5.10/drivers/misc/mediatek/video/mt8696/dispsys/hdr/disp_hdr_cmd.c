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

#define LOG_TAG "HDR_DEBUG"

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

#include "disp_hdr_main.h"
#include "disp_hdr_if.h"
#include "disp_hdr_cmd.h"


static int hdr_dbg_init;
unsigned int hdr_dbg_level;
static struct dentry *hdr_debugfs;

static char hdr_dbg_buf[2048];
static char hdr_cmd_buf[512];

static const char HDR_STR_HELP[] = "USAGE:echo [ACTION]>/d/hdr\n";


static void hdr_process_dbg_opt(const char *opt)
{
	char *p;

	if (strncmp(opt, "log_level:", 10) == 0) {
		unsigned int level = 0;
		unsigned int enable = 0;

		p = (char *)opt + 10;
		STR_CVT_U32(&p, &level, goto Error);
		STR_CVT_U32(&p, &enable, goto Error);
		if (enable)
			hdr_dbg_level |= 1 << level;
		else
			hdr_dbg_level &= 0 << level;
		hdr_printf("hdr_log_level %d\n", hdr_dbg_level);
	} else if (strncmp(opt, "timer:", 6) == 0) {
		unsigned int enable = 0;

		p = (char *)opt + 6;
		STR_CVT_U32(&p, &enable, goto Error);
		time_check = enable;
		hdr_printf("enable hdr time check %d\n", time_check);

	} else if (strncmp(opt, "path:", 5) == 0) {
		uint32_t path_id = 0;
		uint32_t en = 0;
		uint32_t type = 0;

		p = (char *)opt + 5;
		STR_CVT_U32(&p, &path_id, goto Error);
		STR_CVT_U32(&p, &en, goto Error);
		STR_CVT_U32(&p, &type, goto Error);

		disp_hdr_path_ctl(path_id, en, type);

	} else if (strncmp(opt, "adl_m:", 6) == 0) {
		p = (char *)opt + 6;

		STR_CVT_U32(&p, &adl_mode, goto Error);
		hdr_printf("set adl_mode 0x%x\n", adl_mode);
	} else if (strncmp(opt, "sof:", 4) == 0) {
		p = (char *)opt + 4;

		STR_CVT_U32(&p, &hdr_sof_start, goto Error);
		STR_CVT_U32(&p, &hdr_sof_end, goto Error);

		hdr_printf("set sof mode 0x%x 0x%x\n",
			hdr_sof_start, hdr_sof_end);

	} else if (strncmp(opt, "delay:", 6) == 0) {
		p = (char *)opt + 6;

		STR_CVT_U32(&p, &delay_hdr_num, goto Error);
		STR_CVT_U32(&p, &delay_hdr_mute_num, goto Error);

		hdr_printf("set delay %d %d\n", delay_hdr_num,
			delay_hdr_mute_num);
	} else if (strncmp(opt, "fsdr:", 5) == 0) {
		p = (char *)opt + 5;

		STR_CVT_U32(&p, &force_sdr_output, goto Error);

		hdr_printf("forcesdr %d\n", force_sdr_output);
	} else if (strncmp(opt, "allm:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int allm_en = 0;
		unsigned int allm_type = 0;

		STR_CVT_U32(&p, &allm_en, goto Error);
		STR_CVT_U32(&p, &allm_type, goto Error);

		hdr_allm_ctl_by_cmd = (bool)allm_en;
		hdr_allm_type = allm_type;
		hdr_printf("set src allm %d %d!\n", hdr_allm_ctl_by_cmd,
			hdr_allm_type);
	} else if (strncmp(opt, "dv_s_type:", 10) == 0) {
		p = (char *)opt + 10;

		STR_CVT_U32(&p, &use_dv_s_type, goto Error);

		hdr_printf("use_dv_s_type %d\n", use_dv_s_type);
	} else {
		hdr_error("test debug cmd pass.\n");
		goto Error;
	}

	return;

Error:
	hdr_error("parse command error!%s\n",
		HDR_STR_HELP);
}

static void hdr_process_dbg_cmd(char *cmd)
{
	char *tok = NULL;

	hdr_printf("cmd: %s\n", cmd);
	/*memset(hdr_dbg_buf, 0, sizeof(hdr_dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		hdr_printf("parse: %s\n", tok);
		hdr_process_dbg_opt(tok);
	}
}

/* --------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------- */
static void hdr_process_dbg_opt_simplify(const char *opt)
{
	char *p = NULL;

	if (strncmp(opt, "fsdr:", 5) == 0) {
		p = (char *)opt + 5;

		STR_CVT_U32(&p, &force_sdr_output, goto Error);

		hdr_printf("forcesdr %d\n", force_sdr_output);
	} else if (strncmp(opt, "allm:", 5) == 0) {
		unsigned int allm_en = 0;
		unsigned int allm_type = 0;

		p = (char *)opt + 5;
		STR_CVT_U32(&p, &allm_en, goto Error);
		STR_CVT_U32(&p, &allm_type, goto Error);

		hdr_allm_ctl_by_cmd = (bool)allm_en;
		hdr_allm_type = allm_type;
		hdr_printf("set src allm %d %d!\n", hdr_allm_ctl_by_cmd,
			hdr_allm_type);
	} else if (strncmp(opt, "dv_s_type:", 10) == 0) {
		p = (char *)opt + 10;

		STR_CVT_U32(&p, &use_dv_s_type, goto Error);

		hdr_printf("use_dv_s_type %d\n", use_dv_s_type);
	} else {
		hdr_error("test debug cmd pass.\n");
		goto Error;
	}

	return;

Error:
	hdr_error("parse command error!%s\n",
		HDR_STR_HELP);
}


static void hdr_process_dbg_cmd_simplify(char *cmd)
{
	char *tok = NULL;

	hdr_printf("cmd: %s\n", cmd);
	/*memset(hdr_dbg_buf, 0, sizeof(hdr_dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		hdr_printf("parse: %s\n", tok);
		hdr_process_dbg_opt_simplify(tok);
	}
}

static int hdr_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t hdr_debug_read(struct file *file,
	char __user *ubuf, size_t count, loff_t *ppos)
{
	if (strlen(hdr_dbg_buf))
		return simple_read_from_buffer(ubuf, count,
		ppos, hdr_dbg_buf,
		strlen(hdr_dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count,
		ppos, HDR_STR_HELP,
		strlen(HDR_STR_HELP));

}

static ssize_t hdr_debug_write(struct file *file,
	const char __user *ubuf, size_t count,
				loff_t *ppos)
{
	const int debug_bufmax = sizeof(hdr_cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(hdr_cmd_buf, ubuf, count))
		return -EFAULT;

	hdr_cmd_buf[count] = 0;

	hdr_process_dbg_cmd(hdr_cmd_buf);

	return ret;
}

static const struct file_operations hdr_debug_fops = {
	.read = hdr_debug_read,
	.write = hdr_debug_write,
	.open = hdr_debug_open,
};

static ssize_t hdr_debug_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int count = 0;

	if (strlen(hdr_dbg_buf)) {
		count = strlen(hdr_dbg_buf);
		if (!strncpy(buf, hdr_dbg_buf, count))
			return -EFAULT;
	} else {
		count = strlen(HDR_STR_HELP);
		if (!strncpy(buf, HDR_STR_HELP, count))
			return -EFAULT;
	}

	return count;
}

static ssize_t hdr_debug_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	const int debug_bufmax = sizeof(hdr_cmd_buf) - 1;
	size_t ret = 0;

	ret = count;
	if (count > debug_bufmax)
		return -EINVAL;

	if (!strncpy(hdr_cmd_buf, buf, count))
		return -EFAULT;

	hdr_cmd_buf[count] = 0;
	hdr_process_dbg_cmd_simplify(hdr_cmd_buf);

	return count;
}

static struct kobj_attribute hdr_debug_sysfs  = {
	.attr = {
		.name = "hdr",
		.mode = 0664,
	},
	.show  = hdr_debug_show,
	.store = hdr_debug_store,
};

void hdr_debug_init(void)
{
	int retval = 0;

	if (!hdr_dbg_init) {
		hdr_dbg_init = 1;
		hdr_debugfs = debugfs_create_file("hdr",
			S_IFREG | 0444, NULL, (void *)0,
			&hdr_debug_fops);
	}

	/* register sysfs */
	retval = sysfs_create_file(kernel_kobj, &hdr_debug_sysfs.attr);
	if (retval) {
		sysfs_remove_file(kernel_kobj, &hdr_debug_sysfs.attr);
		hdr_error("hdr sysfs file create fail!\n");
	}
}

void hdr_debug_deinit(void)
{
	debugfs_remove(hdr_debugfs);
	sysfs_remove_file(kernel_kobj, &hdr_debug_sysfs.attr);
}
