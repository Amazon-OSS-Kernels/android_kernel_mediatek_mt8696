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

#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include "disp_videoin_if.h"
#include "videoin_hal.h"
/*#include "ion_drv.h"*/
/*#include "mtk_ion.h"*/

static char dbg_buf[2048];
static char STR_HELP[] =
	"USAGE:\n"
	"    echo [ACTION] > /d/videoin\n"
	"ACTION:\n"
	"    echo pa_addr 0x50000000 > /d/videoin    set phyiscial addr\n"
	"    echo enable:1 > /d/videoin		0:disable 1:enable\n"
	"    echo ion_addr > /d/videoin		allocate ion buffer\n"
	"    echo get_ion_buffer > /d/videoin	get ion buffer\n";

static int debug_init;
static struct dentry *debugfs;

static void *buffer_va;
static unsigned int buffer_mva;

#ifndef STR_CVT_U32
#define STR_CVT_U32(p, val, action)\
	do {			\
		int ret = 0;	\
		const char *tmp;	\
		tmp = strsep(p, ","); \
		if (tmp == NULL) \
			break; \
		ret = kstrtouint(tmp, 0, (unsigned int *)val); \
		if (ret != 0) {\
			pr_info("[ERROR]return error: %d\n" \
				"  file : %s, line : %d\n",		\
				ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif

static unsigned int videoin_read_reg(uintptr_t addr)
{
	unsigned int reg_value = 0;

	reg_value = ReadREG32(addr);

	return reg_value;
}

static unsigned int videoin_write_reg(uintptr_t addr, unsigned int value)
{

	unsigned int reg_value = 0;

	WriteREG32(addr, value);

	return reg_value;
}

static UINT8 videoin_range_reg(unsigned int addr,
			     unsigned int *len, uintptr_t *paddr)
{
	unsigned int offset = 0;

	if ((addr >= 0x14002000) && (addr < 0x14002000 + 0x100)) {
		offset = addr - 0x14002000;
		if (offset % 4) {
			pr_info("Address illegal access!\n");
			return false;
		}
		*paddr = videoin.init_param.videoin_reg_base + offset;
		if (len && ((*len + addr) > (0x14002000 + 0x100)))
			*len = 0x100 - offset;
		return true;
	} else if ((addr >= 0x1400e300) && (addr < 0x1400e300 + 0x100)) {
		offset = addr - 0x1400e300;
		if (offset % 4) {
			pr_info("Address illegal access!\n");
			return false;
		}
		*paddr = videoin.init_param.larb4_reg_base + offset;
		if (len && ((*len + addr) > (0x1400e300 + 0x100)))
			*len = 0x100 - offset;
		return true;
	}

	return false;
}

static void videoin_process_dbg_opt(const char *opt)
{
	int ret = 0;
	char *p;

	if (strncmp(opt, "regr:", 5) == 0) {
		unsigned int addr = 0;
		unsigned int regVal = 0;
		uintptr_t v_addr = 0;

		p = (char *)opt + 5;
		if ((p == NULL) || (!strlen(p)))
			goto Error;
		ret = kstrtouint(p, 0, &addr);
		if (ret != 0) {
			pr_info("failed to paraser videoin regr command param!\n");
			goto Error;
		}
		if (videoin_range_reg(addr, NULL, &v_addr)) {
			regVal = videoin_read_reg(v_addr);
			pr_info("regr: 0x%x = 0x%08x\n", addr, regVal);
		} else {
			pr_info("out of address access range!\n");
		}

	} else if (strncmp(opt, "dump_reg:", 9) == 0) {
		unsigned int addr = 0;
		unsigned int length = 0;
		unsigned int regVal = 0;
		unsigned int i = 0;
		uintptr_t v_addr = 0;

		p = (char *)opt + 9;
		STR_CVT_U32(&p, &addr, goto Error);
		STR_CVT_U32(&p, &length, goto Error);
		if (videoin_range_reg(addr, &length, &v_addr)) {
			for (i = 0; i < length; i += 4) {
				regVal = videoin_read_reg(v_addr + i);
				pr_info("regr: 0x%x = 0x%08x\n",
					addr + i, regVal);
			}
		} else {
			pr_info("out of address access range!\n");
		}

	} else if (strncmp(opt, "regw:", 5) == 0) {
		unsigned int addr = 0;
		unsigned int val = 0;
		unsigned int regVal = 0;
		uintptr_t v_addr = 0;

		p = (char *)opt + 5;
		STR_CVT_U32(&p, &addr, goto Error);
		STR_CVT_U32(&p, &val, goto Error);
		if (videoin_range_reg(addr, NULL, &v_addr)) {
			videoin_write_reg(v_addr, val);
			regVal = videoin_read_reg(v_addr);
			pr_info("regw: 0x%x, 0x%08x = 0x%08x\n",
				addr, val, regVal);
		} else {
			pr_info("out of address access range!\n");
		}
	} else if (strncmp(opt, "pa_addr", 7) == 0) {
		unsigned int y_addr = 0;
		unsigned int cb_addr = 0;
		unsigned int cr_addr = 0;
		bool yuv444 = false;

		ret = sscanf(opt, "pa_addr 0x%x 0x%x 0x%x\n",
			&y_addr, &cb_addr, &cr_addr);
		pr_info("ret=%d, y_addr=0x%x, cb_addr=0x%x, cr_addr=0x%x\n",
			ret, y_addr, cb_addr, cr_addr);

		if (ret != 3) {
			pr_info("error to parse cmd %s, ret=%d\n", opt, ret);
			goto Error;
		}

		if (ret == 3)
			yuv444 = true;

		videoin_hal_set_m4u_port(false);
		videoin_hal_update_addr(y_addr, cb_addr, cr_addr, yuv444);
	} else if (strncmp(opt, "enable", 6) == 0) {
		unsigned long value = 0;

		p = (char *)opt + 7;
		ret = kstrtoul(p, 10, (unsigned long int *)&value);
		if (ret) {
			pr_info("%s: errno %d\n", __func__, ret);
			goto Error;
		}
		pr_info("enable video in=%lu\n", value);
		videoin_hal_enable(value);
	} else if (strncmp(opt, "ion_addr", 8) == 0) {
		unsigned int y_addr = 0;
		unsigned int cb_addr = 0;
		unsigned int cr_addr = 0;
		/*it had no andriod ion at early porting stage*/
		/*ret = videoin_debug_get_ion_buffer*/
		/*(&y_addr, &cb_addr, &cr_addr);*/
		if (ret) {
			pr_info("%s: errno %d\n", __func__, ret);
			goto Error;
		}

		videoin_hal_set_m4u_port(true);
		videoin_hal_update_addr(y_addr, cb_addr, cr_addr, false);
	} else if (strncmp(opt, "get_ion_buffer", 14) == 0) {
		int size = 1920 * 1080;

		if (!buffer_mva) {
			pr_info("ion buffer has not be allocated");
			return;
		}
		pr_info("y_buffer=0x%x, cb_buffer=0x%x, y_va=%p, cb_va=%p\n",
			buffer_mva, buffer_mva + size,
			buffer_va, buffer_va + size);
	} else {
		goto Error;
	}

	return;
Error:
	pr_err("%s\n", STR_HELP);
}


static void videoin_process_dbg_cmd(char *cmd)
{
	char *tok;

	if ((cmd == NULL) || (strlen(cmd) > 512))
		return;
	pr_info("cmd: %s\n", cmd);
	memset(dbg_buf, 0, sizeof(dbg_buf));
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		if ((tok == NULL) || (strlen(tok) > 512))
			break;
		pr_info("parse: %s\n", tok);
		videoin_process_dbg_opt(tok);
	}
}

/* ------------------------------- */
/* Debug FileSystem Routines */
/* ------------------------------- */

static int videoin_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static char cmd_buf[512];

static ssize_t videoin_debug_read(struct file *file, char __user *ubuf,
	size_t count, loff_t *ppos)
{
	if (strlen(dbg_buf))
		return simple_read_from_buffer(ubuf, count,
		ppos, dbg_buf, strlen(dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count,
		ppos, STR_HELP, strlen(STR_HELP));

}

static ssize_t videoin_debug_write(struct file *file,
	const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		return ret;

	if (copy_from_user(&cmd_buf, ubuf, count))
		return -EFAULT;

	cmd_buf[count] = 0;

	videoin_process_dbg_cmd(cmd_buf);

	return ret;
}

static const struct file_operations debug_fops = {
	.read = videoin_debug_read,
	.write = videoin_debug_write,
	.open = videoin_debug_open,
};

void videoin_debug_init(void)
{
	if (!debug_init) {
		debug_init = 1;
		debugfs = debugfs_create_file("videoin",
					      S_IFREG | 0444, NULL,
					      (void *)0, &debug_fops);

		pr_debug("video in debug init, fs= %p\n", debugfs);
	}
}

void videoin_debug_deinit(void)
{
	debugfs_remove(debugfs);
}


