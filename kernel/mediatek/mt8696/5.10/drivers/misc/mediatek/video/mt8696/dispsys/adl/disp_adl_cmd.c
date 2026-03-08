/*
 * Copyright (C) 2020 MediaTek Inc.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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
#include <linux/dma-mapping.h>
#include <linux/types.h>



#include "disp_type.h"
#include "disp_hw_mgr.h"
#include "adl_hal.h"
#include "disp_adl_if.h"
#include "adl_hw.h"
#include "disp_adl_cmd.h"

static int adl_dbg_init;
static struct dentry *adl_debugfs;
static char adl_dbg_buf[2048];
static char adl_cmd_buf[512];
static const char ADL_STR_HELP[] = "USAGE:echo [ACTION]>/d/adl\n";

unsigned int adl_dbg_level;


/* --------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------- */

static int adl_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t adl_debug_read(struct file *file,
	char __user *ubuf, size_t count, loff_t *ppos)
{
	if (strlen(adl_dbg_buf))
		return simple_read_from_buffer(ubuf, count,
		ppos, adl_dbg_buf,
		strlen(adl_dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count,
		ppos, ADL_STR_HELP,
		strlen(ADL_STR_HELP));

}

static void adl_show_status(unsigned int id)
{

	if (id >= ADL_CLIT_REG_MAX) {
		adl_printf("client not exited");
		return;
	}
	disp_adl_client_status();
}

static void adl_reg_rw_test(unsigned int id)
{
	int i = 0;
	uint32_t value = 0xFFFFFFFF;
	dma_addr_t mva_msb = 0;
	dma_addr_t mva_lsb = 0;
	dma_addr_t mva = 0;

	mva = 0x0;
	mva_msb = ((0xffff0000 & (mva >> 5)) >> 16);
	mva_lsb = ((0x0000ffff & (mva >> 5)));

	for (i = 0; i < ADL_CLIT_REG_MAX; i++) {
		mva_msb = ((0xffff0000 & (mva >> 5)) >> 16);
		mva_lsb = ((0x0000ffff & (mva >> 5)));
		AdlWriteREG(adl_clt_regbase[i] + EN, value);
		AdlWriteREG(adl_clt_regbase[i] + ADDR0, mva_lsb);
		AdlWriteREG(adl_clt_regbase[i] + ADDR1, mva_msb);
		AdlWriteREG(adl_clt_regbase[i] + DEPTH, 0xff);
		AdlWriteREG(adl_clt_regbase[i] + DMA_LEN, 0x80);
		AdlWriteREG(adl_clt_regbase[i] + INIT_ADDR, 0x0);
	}
}

static void adl_process_dbg_opt(const char *opt)
{
	char *p;

	if (strncmp(opt, "log_level:", 10) == 0) {
		unsigned int level = 0;
		unsigned int enable = 0;

		p = (char *)opt + 10;
		STR_CVT_U32(&p, &level, goto Error);
		STR_CVT_U32(&p, &enable, goto Error);
		if (enable)
			adl_dbg_level |= 1 << level;
		else
			adl_dbg_level &= 0 << level;
		adl_printf("adl_log_level %d\n", adl_dbg_level);
	} else if (strncmp(opt, "st:", 3) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 3;
		STR_CVT_U32(&p, &value, goto Error);
		adl_show_status(value);
	} else if (strncmp(opt, "regrw:", 6) == 0) {
		unsigned int value = 0;

		p = (char *)opt + 6;
		STR_CVT_U32(&p, &value, goto Error);
		adl_reg_rw_test(value);
	} else if (strncmp(opt, "flush", 5) == 0) {
		disp_adl_client_flush();
		adl_printf("adl flush done");
	} else if (strncmp(opt, "mode:", 5) == 0) {
		uint32_t client = 0;
		uint32_t clt_mode = 0;
		uint32_t reg_mode = 0;

		p = (char *)opt + 5;

		STR_CVT_U32(&p, &client, goto Error);
		STR_CVT_U32(&p, &clt_mode, goto Error);
		STR_CVT_U32(&p, &reg_mode, goto Error);
		disp_adl_set_client_mode(client, clt_mode, reg_mode);
		adl_printf("adl set mode %d %d %d\n", client,
			clt_mode, reg_mode);
	}
	return;
Error:
	adl_error("parse command error!%s\n", ADL_STR_HELP);
}


static void adl_process_dbg_cmd(char *cmd)
{
	char *tok = NULL;

	adl_printf("cmd: %s\n", cmd);
	/*memset(adl_dbg_buf, 0, sizeof(adl_dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		adl_printf("parse: %s\n", tok);
		adl_process_dbg_opt(tok);
	}
}


static ssize_t adl_debug_write(struct file *file,
	const char __user *ubuf, size_t count,
				loff_t *ppos)
{
	const int debug_bufmax = sizeof(adl_cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		return ret;

	if (copy_from_user(&adl_cmd_buf, ubuf, count))
		return -EFAULT;

	adl_cmd_buf[count] = 0;

	adl_process_dbg_cmd(adl_cmd_buf);

	return ret;
}

static const struct file_operations adl_debug_fops = {
	.read = adl_debug_read,
	.write = adl_debug_write,
	.open = adl_debug_open,
};

void adl_debug_init(void)
{
	if (!adl_dbg_init) {
		adl_dbg_init = 1;
		adl_debugfs = debugfs_create_file("adl",
			S_IFREG | 0444, NULL, (void *)0,
			&adl_debug_fops);
	}
}

void adl_debug_deinit(void)
{
	debugfs_remove(adl_debugfs);
}


