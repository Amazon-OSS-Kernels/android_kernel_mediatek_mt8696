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
#define LOG_TAG "VDP_DBG"
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
#include "disp_vdp_debug.h"
#include "disp_vdp_vsync.h"
#include "fmt_hal.h"
#include "vdout_sys_hal.h"
#include "disp_vdp_cli.h"

//#pragma GCC optimize("O0")
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

static struct dentry *vdp_debugfs;
/*static struct dentry *vdp_debugDir;*/
/*static struct dentry *debugfs_dump;*/

static int vdp_debug_inited;

static char VDP_STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/dispsys/vdp\n"
	"ACTION:\n"
	"       regr:addr                :regr:0x42400\n"
	"       regw:addr,value          :regw:0x4243c,0x1\n"
	"       dump_reg:addr,length     :dump_reg:0x42000,0x1000\n";
/* ---------------------------------------------------------------------------
 */
/* Command Processor */
/* ---------------------------------------------------------------------------
 */
static char vdp_dbg_buf[2048];
static char vdp_cmd_buf[512];

static unsigned int vdp_read_reg(unsigned int addr)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

/*0x42000 -> 0x42fff */
/*0x3000 -> 0x3fff */
#if 0
	if (addr >= IO_BASE)
		reg_pa = addr - IO_BASE;
	reg_value = ReadREG32(reg_pa);

#endif
	return reg_value;
}

static unsigned int vdp_write_reg(unsigned int addr, unsigned int value)
{
	/*unsigned int reg_pa = addr;*/
	unsigned int reg_value = 0;

	/*0x42000 -> 0x42fff */
	/*0x3000 -> 0x3fff */

	return reg_value;
}

static int vdp_debug_force_mdp(char *p)
{
	int value = 0;

	struct vdp_cli_setting_struct *setting = vdp_cli_get();

	sscanf(p, "%d", &value);

	setting->enable_force_use_mdp = (value != 0);
	pr_err_ratelimited("set force_mdp:%d\n",
		setting->enable_force_use_mdp);
	return 0;
}

int vdp_debug_set_disp_area(char *p)
{
	int value[6] = {0};
	int ret = 0;

	struct vdp_cli_setting_struct *setting = vdp_cli_get();

	ret = sscanf(p, "%d %d %d %d %d %d",
		   &value[0],
		   &value[1],
		   &value[2],
		   &value[3],
		   &value[4],
		   &value[5]);

	pr_err_ratelimited("%s ret %d\n", __func__, ret);

	if ((value[3] == 0) ||
		(value[4] == 0) ||
		(value[5] == 0))
		setting->target_area.enable = false;
	else
		setting->target_area.enable = true;

	setting->target_area.layer_id = value[0];
	setting->target_area.range.x = value[1];
	setting->target_area.range.y = value[2];
	setting->target_area.range.width = value[3];
	setting->target_area.range.height = value[4];
	setting->target_area.range.pitch = value[5];

	pr_err_ratelimited("set show: enable[%d] layer_id[%d] x[%d] y[%d] width[%d] height[%d] pitch[%d]\n",
		setting->target_area.enable,
		setting->target_area.layer_id,
		setting->target_area.range.x,
		setting->target_area.range.y,
		setting->target_area.range.width,
		setting->target_area.range.height,
		setting->target_area.range.pitch);

	return 0;
}


static void vdp_process_dbg_opt(const char *opt)
{
	/*char *buf = dbg_buf + strlen(dbg_buf);*/

	if (0) {
		DISP_LOG_I("parse command %s\n", opt);
	} else if (strncmp(opt, "regr:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int regVal = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		regVal = vdp_read_reg(addr);
		DISP_LOG_I("regr: 0x%x = 0x%08x\n", addr, regVal);
		/*sprintf(buf, "regr: 0x%x = 0x%08x\n", addr, regVal);*/
	} else if (strncmp(opt, "regw:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int addr = 0;
		unsigned int val = 0;
		unsigned int regVal = 0;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &val, uint, goto Error);

		vdp_write_reg(addr, val);
		regVal = vdp_read_reg(addr);
		DISP_LOG_I("regw: 0x%x, 0x%08x = 0x%08x\n", addr, val, regVal);
		/*sprintf(buf, "regw: 0x%x, 0x%08x = 0x%08x\n", addr, val,
		 * regVal);
		 */
	} else if (strncmp(opt, "enable:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int vdp_id = 0;
		unsigned int enable = 0;

		STR_CONVERT(&p, &vdp_id, uint, goto Error);
		STR_CONVERT(&p, &enable, uint, goto Error);

		DISP_LOG_I(" set vdp %d enable %d\n", vdp_id, enable);

	} else if (strncmp(opt, "dbg_lvl:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int level = 0;
		unsigned int enable = 0;

		STR_CONVERT(&p, &level, uint, goto Error);
		STR_CONVERT(&p, &enable, uint, goto Error);
		DISP_LOG_I("set vdp debug level %d enable 0x%X\n", level,
			   enable);
		disp_vdp_dbg_level_enable(level, enable);

	} else if (strncmp(opt, "not_mix:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int not_mix_mode = 0;

		STR_CONVERT(&p, &not_mix_mode, uint, goto Error);
		DISP_LOG_I("0-not_mix_main | 1-not_mix_sub | 2-not_mix_all | 3-mix_auto\n");
		DISP_LOG_I("set vdp not_mix level %d \n", not_mix_mode);
		disp_vdp_set_not_mix(not_mix_mode);
	} else if (strncmp(opt, "not_disp:", 9) == 0) {
		char *p = (char *)opt + 9;
		unsigned int not_disp_mode = 0;

		STR_CONVERT(&p, &not_disp_mode, uint, goto Error);
		DISP_LOG_I("0-not_disp_main | 1-not_disp_sub | 2-not_disp_all | 3-disp_auto\n");
		DISP_LOG_I("set vdp not_disp level %d \n", not_disp_mode);
		disp_vdp_set_not_display(not_disp_mode);
	} else if (strncmp(opt, "dr:", 3) == 0) {
		char *p = (char *)opt + 3;
		unsigned int dr_range = 0;

		STR_CONVERT(&p, &dr_range, uint, goto Error);
		force_dr_range = dr_range;
		DISP_LOG_I("set vdp force dr range %d\n", dr_range);
	} else if (strncmp(opt, "dm_hdl:", 7) == 0) {
		char *p = (char *)opt + 7;
		unsigned int dovi_drop = 0;

		STR_CONVERT(&p, &dovi_drop, uint, goto Error);
		dovi_w_drop = dovi_drop;
		DISP_LOG_I("set dovi force hdl len %d\n", dovi_w_drop);
	} else if (strncmp(opt, "disp_test:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int disp_test = 0;

		STR_CONVERT(&p, &disp_test, uint, goto Error);
		vdp_disp_test = disp_test;
		DISP_LOG_I("set vdp disp_test %d\n", disp_test);
	} else if (strncmp(opt, "dsd_off:", 8) == 0) {
		unsigned int en = 0;
		char *p;
		int ret = 0;

		p = (char *)opt + 8;
		ret = kstrtoul(p, 10, (unsigned long int *)&en);
		if (ret) {
			DISP_LOG_E("%s: errno %d\n", __func__, ret);
			goto Error;
		}
		force_dsd_off = en;
		DISP_LOG_I("force_dsd_off %d\n", force_dsd_off);
	} else if (strncmp(opt, "df_log_en:", 10) == 0) {
		unsigned int en = 0;
		char *p;
		int ret = 0;

		p = (char *)opt + 10;
		ret = kstrtoul(p, 10, (unsigned long int *)&en);
		if (ret) {
			DISP_LOG_E("%s: errno %d\n", __func__, ret);
			goto Error;
		}
		enable_frame_drop_log = en;
		DISP_LOG_I("enable_frame_drop_log %d\n", enable_frame_drop_log);
	} else if (strncmp(opt, "set_tgt:", 8) == 0) {
		char *p;

		p = (char *)opt + 8;
		vdp_debug_set_disp_area(p);
	} else if (strncmp(opt, "force_mdp:", 10) == 0) {
		char *p;

		p = (char *)opt + 10;
		vdp_debug_force_mdp(p);
	} else if (strncmp(opt, "allm_en:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int allm_en = 0;

		STR_CONVERT(&p, &allm_en, uint, goto Error);
		force_decode_allm = allm_en;

	} else {
		DISP_LOG_E(
			"parse command error!\n%s\n\n%s sizeof(VDP_STR_HELP) %d\n",
			opt, VDP_STR_HELP, (uint32_t)sizeof(VDP_STR_HELP));
	}
	return;
Error:
	DISP_LOG_E("parse command error!\n%s\n\n%s", opt, VDP_STR_HELP);
}

static void vdp_process_dbg_cmd(char *cmd)
{
	char *tok;

	DISP_LOG_I("cmd: %s\n", cmd);
	/*memset(dbg_buf, 0, sizeof(dbg_buf));*/
	while ((tok = strsep(&cmd, "&&")) != NULL) {
		DISP_LOG_I("parse: %s\n", tok);
		vdp_process_dbg_opt(tok);
	}
}

/* ---------------------------------------------------------------------------
 */
/* Debug FileSystem Routines */
/* ---------------------------------------------------------------------------
 */

static int vdp_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t vdp_debug_read(struct file *file, char __user *ubuf,
			      size_t count, loff_t *ppos)
{
	if (strlen(vdp_dbg_buf))
		return simple_read_from_buffer(ubuf, count, ppos, vdp_dbg_buf,
					       strlen(vdp_dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count, ppos, VDP_STR_HELP,
					       strlen(VDP_STR_HELP));
}

static ssize_t vdp_debug_write(struct file *file, const char __user *ubuf,
			       size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(vdp_cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&vdp_cmd_buf, ubuf, count))
		return -EFAULT;

	vdp_cmd_buf[count] = 0;

	vdp_process_dbg_cmd(vdp_cmd_buf);

	return ret;
}

static const struct file_operations vdp_debug_fops = {
	.read = vdp_debug_read,
	.write = vdp_debug_write,
	.open = vdp_debug_open,
};

void vdp_debug_init(void)
{
	if (!vdp_debug_inited) {

		vdp_debug_inited = 1;

		vdp_debugfs =
			debugfs_create_file("vdp", S_IFREG | 0444, NULL,
					    (void *)0, &vdp_debug_fops);
	}
}

void vdp_debug_exit(void)
{
	debugfs_remove(vdp_debugfs);
	vdp_debug_inited = 0;
}
