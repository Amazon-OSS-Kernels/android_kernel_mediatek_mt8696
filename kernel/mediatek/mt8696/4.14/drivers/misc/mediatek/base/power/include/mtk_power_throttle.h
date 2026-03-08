/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef MT_THROTTLE_H
#define MT_THROTTLE_H


extern int setup_power_table_tk(void);
extern void dump_power_table(void);
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);
extern struct proc_dir_entry *mtk_thermal_get_proc_drv_therm_dir_entry(void);
extern int hpt_set_cpu_num_limit(unsigned int little_cpu, unsigned int big_cpu);
#endif

