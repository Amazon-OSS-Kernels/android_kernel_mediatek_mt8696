/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef MT_POWERTHROTTLE_H
#define MT_POWERTHROTTLE_H

struct mtk_cpu_freq_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_volt;  /* mv * 1000 */
	const unsigned int cpufreq_volt_org;    /* mv * 1000 */
};

struct mt_cpu_power_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_ncpu;
	unsigned int cpufreq_power;
};

struct mtk_cpu_dvfs {
	/* opp (freq) table */
	struct mtk_cpu_freq_info *opp_tbl;       /* OPP table */
	int nr_opp_tbl;                         /* size for OPP table */
	/* power table */
	struct mt_cpu_power_info *power_tbl;
	unsigned int nr_power_tbl;
};

struct mtk_throttle_ptable {
	/* unsort power table */
	struct mt_cpu_power_info *power_tbl;
	unsigned int nr_power_tbl;
};

#define OPP(khz, volt) {            \
	.cpufreq_khz = khz,             \
	.cpufreq_volt = volt,           \
	.cpufreq_volt_org = volt,       \
}

/* need dvfs provide opp table*/
extern struct mtk_cpu_freq_info opp_tbl_default[];
/* for thermal visit */
extern struct mtk_cpu_dvfs cpu_dvfs_infor;
extern struct mtk_throttle_ptable ptable_infor;
extern unsigned long clipped_freq;

extern int setup_power_table_tk(void);
extern void dump_power_table(void);
#endif

