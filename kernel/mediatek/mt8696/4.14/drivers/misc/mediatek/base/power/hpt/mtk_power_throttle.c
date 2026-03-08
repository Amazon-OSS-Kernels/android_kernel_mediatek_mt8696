/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kernel.h> /* ARRAY_SIZE */
#include <linux/slab.h> /* kzalloc */
#include "leakage_table_v2/mtk_static_power.h"	/* static power */
#include <linux/cpufreq.h>
#include "mtk_power_throttle.h"
#include "mach/mtk_thermal.h"
#include <linux/proc_fs.h>  /* proc_mkdir, proc_create */
#include <linux/seq_file.h> /* seq_printf, single_open */
#include <linux/uaccess.h>  /* copy_from_user */
#include "mtk_throttle_table.h"



static unsigned int limited_max_ncpu;
static unsigned int limited_max_freq;

#define NR_MAX_CPU      (4)

static unsigned long previous_limit_power;
/*
 * min_thermal_limit_cpu only be modify by hpt
 */
static unsigned int min_thermal_limit_cpu = 1;
//static unsigned int max_thermal_limit_cpu = NR_MAX_CPU;
static unsigned int min_thermal_limit_freq;
static unsigned int max_thermal_limit_freq = UINT_MAX;

struct proc_dir_entry __attribute__((weak))
*mtk_thermal_get_proc_drv_therm_dir_entry(void)
{
	pr_info("Fail: %s /proc/driver/thermal doesn't exist\n"
			"maybe thermal config is not enable\n",	__func__);
	return NULL;
}

int __attribute__((weak)) mtktscpu_debug_log = 0;


static DEFINE_MUTEX(power_throttle_lock);

void lock_power_throttle(void)
{
	mutex_lock(&power_throttle_lock);
}

void unlock_power_throttle(void)
{
	mutex_unlock(&power_throttle_lock);
}

void update_thermal_limit_protect(void)
{
	if (previous_limit_power != 0)
		mt_cpufreq_thermal_protect(previous_limit_power);
}

void update_min_thermal_limit_cpu(unsigned int num)
{
	if (num < 1 || num > NR_MAX_CPU) {
		pr_info("min limit cpu num is invalid!!!!\n");
		return;
	}
	min_thermal_limit_cpu = num;
	pr_info("%s: min_thermal_limit_cpu changes to %d\n",
				__func__, min_thermal_limit_cpu);
	pr_info("previous_limit_power:%lu\n", previous_limit_power);
}

static int thermal_limit_freq_proc_show(
	struct seq_file *m, void *v)
{
	//int i = 0;

	seq_puts(m, "min_thermal_limit_freq  max_thermal_limit_freq\n");
	seq_printf(m, "%u %u\n",
		min_thermal_limit_freq, max_thermal_limit_freq);
//TODO
	#if 0
	for (; i < NR_MAX_OPP_TBL; i++) {
		i % 2 ? seq_printf(m, "%u\t", opp_tbl_default[i].cpufreq_khz) :
		seq_printf(m, "%u\n", opp_tbl_default[i].cpufreq_khz);
	}


#endif
	return 0;
}

static ssize_t thermal_limit_freq_proc_write(struct file *file,
					       const char __user *buffer,
					       size_t count, loff_t *pos)
{
	int len = 0, temp1, temp2;
	char desc[30];

	len = min(count, sizeof(desc) - 1);
	if (len < 0 || len >= 30)
		return -EINVAL;

	memset(desc, 0, sizeof(desc));
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%u %u", &temp1, &temp2) == 2) {
		if (temp1 <= temp2) {
			lock_power_throttle();
			min_thermal_limit_freq = temp1;
			max_thermal_limit_freq = temp2;
			unlock_power_throttle();
			update_thermal_limit_protect();
		}
		return count;
	}
	return -EINVAL;
}

static int thermal_limit_freq_proc_proc_open(struct inode *inode,
				  struct file *file)
{
	return single_open(file, thermal_limit_freq_proc_show,
			   PDE_DATA(inode));
}

static const struct
file_operations thermal_limit_freq_proc_proc_fops = {
	.owner = THIS_MODULE,
	.open = thermal_limit_freq_proc_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = thermal_limit_freq_proc_write,
};

static int __init thermal_limit_freq_procfs_init(void)
{
	struct proc_dir_entry *mtktscpu_dir = NULL;

	mtktscpu_dir = mtk_thermal_get_proc_drv_therm_dir_entry();
	if (!mtktscpu_dir)
		pr_info("proc/driver/thermal dir not exist\n");

	if (!proc_create("thermal_limit_freq", 0664, mtktscpu_dir,
				 &thermal_limit_freq_proc_proc_fops))
		pr_info("create /proc/driver/thermal/thermal_limit_freq failed\n");

	return 0;

}

#if 0
struct mt_cpu_power_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_ncpu;
	unsigned int cpufreq_power;
};



struct mtk_cpu_dvfs {
	/* opp (freq) table */
	struct mt_cpu_freq_info *opp_tbl;       /* OPP table */
	int nr_opp_tbl;                         /* size for OPP table */
	/* power table */
	struct mt_cpu_power_info *power_tbl;
	unsigned int nr_power_tbl;
};
#endif

struct mtk_cpu_dvfs __attribute__((weak))
	cpu_dvfs_infor;//only for build pass for early porting.

unsigned long __attribute__((weak))
	clipped_freq;//only for build pass for early porting.

void mt_cpufreq_thermal_protect(unsigned int limited_power)
{

	struct mtk_cpu_dvfs *p;
	int possible_cpu;
	int found = 0;
	int i = 0;

	lock_power_throttle();

	previous_limit_power = limited_power;
	p = &cpu_dvfs_infor;
	WARN_ON(p == NULL);
	possible_cpu = NR_MAX_CPU;

	/* no limited */
	if (limited_power == 0) {
		limited_max_ncpu = possible_cpu;
		limited_max_freq = cpu_dvfs_infor.power_tbl[0].cpufreq_khz;
	} else {

		for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {

			if (p->power_tbl[i].cpufreq_power == 0 ||
				p->power_tbl[i].cpufreq_ncpu == 0 ||
				p->power_tbl[i].cpufreq_khz == 0)
				break;

			if (p->power_tbl[i].cpufreq_power <= limited_power) {
				if (p->power_tbl[i].cpufreq_ncpu >=
						min_thermal_limit_cpu &&
						p->power_tbl[i].cpufreq_ncpu <=
						NR_MAX_CPU &&
						p->power_tbl[i].cpufreq_khz >=
						min_thermal_limit_freq &&
						p->power_tbl[i].cpufreq_khz <=
						max_thermal_limit_freq) {
					limited_max_ncpu =
						p->power_tbl[i].cpufreq_ncpu;
					limited_max_freq =
						p->power_tbl[i].cpufreq_khz;
					found = 1;
					break;
				}
			}
		}

		if (!found) {
			for (i--; i <= p->nr_opp_tbl * possible_cpu
					&& i >= 0; i--) {
				if (p->power_tbl[i].cpufreq_ncpu <
					min_thermal_limit_cpu)
					continue;
				if (p->power_tbl[i].cpufreq_khz >=
						min_thermal_limit_freq &&
					p->power_tbl[i].cpufreq_khz <=
						max_thermal_limit_freq) {
					limited_max_ncpu =
						p->power_tbl[i].cpufreq_ncpu;
					limited_max_freq =
						p->power_tbl[i].cpufreq_khz;
					found = 1;
					break;
				}
			}
		}

		/* not found and use lowest power limit */
		if (!found) {
			pr_info("Warning: not found valid freq and ncpu!!!!!!!\n");
			return;
		}
	}

	clipped_freq = limited_max_freq;

	hpt_set_cpu_num_limit(limited_max_ncpu, 0);
	/* update cpufreq policy */
	cpufreq_update_policy(0);

	unlock_power_throttle();
	if (mtktscpu_debug_log & 0x1) {
		pr_info("%s found = %d, limited_power = %u,",
			__func__, found,
			p->power_tbl[i].cpufreq_power);
		pr_info("freq = %u, cpu_num =%u, table: %d\n",
			p->power_tbl[i].cpufreq_khz,
			p->power_tbl[i].cpufreq_ncpu, i);
		pr_info("%s possible_cpu = %d, cpu_dvfs.power_tbl[0].cpufreq_khz =%u\n",
			__func__, possible_cpu,
			cpu_dvfs_infor.power_tbl[0].cpufreq_khz);
		pr_info("%s limited_max_ncpu = %u, limited_max_freq = %u\n",
			__func__, limited_max_ncpu, limited_max_freq);
	}


}

module_init(thermal_limit_freq_procfs_init);

