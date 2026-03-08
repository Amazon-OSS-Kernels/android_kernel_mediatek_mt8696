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
#include <linux/kernel.h> /* ARRAY_SIZE */
#include <linux/slab.h> /* kzalloc */
#include <linux/cpufreq.h>
#include "mtk_static_power.h"	/* static power */
#include "mtk_static_power_mt8696.h"
#include "mtk_throttle_table.h"

struct mtk_cpu_dvfs cpu_dvfs_infor;
struct mtk_throttle_ptable ptable_infor;
static int power_table_ready;
int throttle_debug_log;
unsigned long clipped_freq;

#define NR_MAX_OPP_TBL  16
#define NR_MAX_CPU      4

struct mtk_cpu_freq_info opp_tbl_default[] = {
	OPP(0, 0), /* KHz , mV * 100 */
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
	OPP(0, 0),
};

void init_mt_cpu_dvfs(struct mtk_cpu_dvfs *p)
{
	p->nr_opp_tbl = ARRAY_SIZE(opp_tbl_default);
	p->opp_tbl = opp_tbl_default;
}

static void _power_calculation(struct mtk_cpu_dvfs *p, int oppidx, int ncpu)
{
/* TBD, need MN confirm */
#define CA35_4CORE_REF_POWER	579		/* mW  */
#define CA35_REF_FREQ	1700000	/* KHz */
#define CA35_REF_VOLT	90000	/* mV * 100 */

	int ref_freq, ref_volt;
	int	p_dynamic = 0, p_leakage = 0;
	int possible_cpu = NR_MAX_CPU;
	int index;

	ref_freq  = CA35_REF_FREQ;
	ref_volt  = CA35_REF_VOLT;

	p_dynamic = CA35_4CORE_REF_POWER;

	p_leakage = mt_spower_get_leakage(MTK_SPOWER_CPULL,
		p->opp_tbl[oppidx].cpufreq_volt / 1000, 85);

	p_dynamic = p_dynamic *
	(p->opp_tbl[oppidx].cpufreq_khz / 1000) / (ref_freq / 1000) *
	p->opp_tbl[oppidx].cpufreq_volt / ref_volt *
	p->opp_tbl[oppidx].cpufreq_volt / ref_volt +
	p_leakage;

	index = (NR_MAX_OPP_TBL * (possible_cpu - 1 - ncpu) + oppidx);
	p->power_tbl[index].cpufreq_ncpu = ncpu + 1;
	p->power_tbl[index].cpufreq_khz
		= p->opp_tbl[oppidx].cpufreq_khz;
	p->power_tbl[index].cpufreq_power
		= ((p_dynamic * (ncpu + 1) / possible_cpu < 0) ? 0
			: p_dynamic * (ncpu + 1) / possible_cpu);

}

void dump_power_table(void)
{
	int	i;

	/* dump power table */
	for (i = 0; i < cpu_dvfs_infor.nr_opp_tbl * NR_MAX_CPU; i++) {
		pr_info("%s[%d] = { .cpufreq_khz = %d,\t.cpufreq_ncpu = %d,\t.cpufreq_power = %d }\n",
		__func__,
		i,
		cpu_dvfs_infor.power_tbl[i].cpufreq_khz,
		cpu_dvfs_infor.power_tbl[i].cpufreq_ncpu,
		cpu_dvfs_infor.power_tbl[i].cpufreq_power
		);
	}
}

void dump_unsort_power_table(void)
{
	int i;

	for (i = 0; i < cpu_dvfs_infor.nr_opp_tbl * NR_MAX_CPU; i++) {
		pr_info("%s[%d] = { .cpufreq_khz = %d,\t.cpufreq_ncpu = %d,\t.cpufreq_power = %d }\n",
		__func__,
		i,
		ptable_infor.power_tbl[i].cpufreq_khz,
		ptable_infor.power_tbl[i].cpufreq_ncpu,
		ptable_infor.power_tbl[i].cpufreq_power
		);
	}
}

int setup_power_table(struct mtk_cpu_dvfs *p)
{
	unsigned int pwr_eff_tbl[NR_MAX_OPP_TBL][NR_MAX_CPU];
	int possible_cpu = NR_MAX_CPU;
	int i, j;
	int ret = 0;
	unsigned int index = 0;

	WARN_ON(p == NULL);

	if (p->power_tbl)
		goto out;

	/* allocate power table */
	memset((void *)pwr_eff_tbl, 0, sizeof(pwr_eff_tbl));
	p->power_tbl = kzalloc
		(p->nr_opp_tbl * possible_cpu
		* sizeof(struct mt_cpu_power_info), GFP_KERNEL);

	ptable_infor.power_tbl = kzalloc
			(p->nr_opp_tbl * possible_cpu
			* sizeof(struct mt_cpu_power_info), GFP_KERNEL);

	if ((p->power_tbl == NULL) ||
		(ptable_infor.power_tbl  == NULL)) {
		ret = -ENOMEM;
		goto out;
	}

	p->nr_power_tbl = p->nr_opp_tbl * possible_cpu;
	ptable_infor.nr_power_tbl = p->nr_power_tbl;

	/* calc power and fill in power table */
	for (i = 0; i < p->nr_opp_tbl; i++) {
		for (j = 0; j < possible_cpu; j++) {
			if (pwr_eff_tbl[i][j] == 0)
				_power_calculation(p, i, j);
		}
	}

	for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
		ptable_infor.power_tbl[i].cpufreq_khz = 0;
		ptable_infor.power_tbl[i].cpufreq_ncpu = 0;
		ptable_infor.power_tbl[i].cpufreq_power = 0;
	}

	/* unsort power table */
	for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
		if (p->power_tbl[i].cpufreq_khz != 0) {
			ptable_infor.power_tbl[index].cpufreq_khz =
				p->power_tbl[i].cpufreq_khz;
			ptable_infor.power_tbl[index].cpufreq_ncpu =
				p->power_tbl[i].cpufreq_ncpu;
			ptable_infor.power_tbl[index].cpufreq_power =
				p->power_tbl[i].cpufreq_power;
			index++;
		}
	}

	/* sort power table */
	for (i = p->nr_opp_tbl * possible_cpu; i > 0; i--) {
		for (j = 1; j < i; j++) {
			if (p->power_tbl[j - 1].cpufreq_power
				< p->power_tbl[j].cpufreq_power) {
				struct mt_cpu_power_info tmp;

				tmp.cpufreq_khz	=
					p->power_tbl[j - 1].cpufreq_khz;
				tmp.cpufreq_ncpu =
					p->power_tbl[j - 1].cpufreq_ncpu;
				tmp.cpufreq_power =
					p->power_tbl[j - 1].cpufreq_power;

				p->power_tbl[j - 1].cpufreq_khz   =
					p->power_tbl[j].cpufreq_khz;
				p->power_tbl[j - 1].cpufreq_ncpu  =
					p->power_tbl[j].cpufreq_ncpu;
				p->power_tbl[j - 1].cpufreq_power =
					p->power_tbl[j].cpufreq_power;

				p->power_tbl[j].cpufreq_khz
					= tmp.cpufreq_khz;
				p->power_tbl[j].cpufreq_ncpu
					= tmp.cpufreq_ncpu;
				p->power_tbl[j].cpufreq_power
					= tmp.cpufreq_power;

			}
		}
	}

	/* dump unsort power table */
	dump_unsort_power_table();

out:
	return ret;
}


/**
 * cpufreq_thermal_notifier - notifier callback for cpufreq policy change.
 * @nb:	struct notifier_block * with callback info.
 * @event: value showing cpufreq event for which this function invoked.
 * @data: callback-specific data
 *
 * Callback to hijack the notification on cpufreq policy transition.
 * Every time there is a change in policy, we will intercept and
 * update the cpufreq policy with thermal constraints.
 *
 * Return: 0 (success)
 */

static int mtk_cpufreq_thermal_notifier(struct notifier_block *nb,
				    unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;

	pr_debug("%s %ld\n", __func__, event);

	if (event != CPUFREQ_ADJUST)
		return NOTIFY_DONE;

	/*
	 * policy->max is the maximum allowed frequency defined by user
	 * and clipped_freq is the maximum that thermal constraints
	 * allow.
	 *
	 * If clipped_freq is lower than policy->max, then we need to
	 * readjust policy->max.
	 *
	 * But, if clipped_freq is greater than policy->max, we don't
	 * need to do anything.
	 */

	if (throttle_debug_log  & 0x1)
		pr_info("%s clipped_freq = %ld, policy->max=%d, policy->min=%d\n",
				__func__, clipped_freq,
				policy->max, policy->min);

	/* Since thermal throttling could hogplug CPU, less cpufreq might cause
	 * a performance issue.
	 * Only DVFS TLP feature enable, we can keep the max freq by CPUFREQ
	 * GOVERNOR or Pref service.
	 */
	if ((policy->max != clipped_freq) && (clipped_freq >= policy->min))
		cpufreq_verify_within_limits(policy, 0, clipped_freq);

	return NOTIFY_OK;
}


/* Notifier for cpufreq policy change */
static struct notifier_block thermal_cpufreq_notifier_block = {
	.notifier_call = mtk_cpufreq_thermal_notifier,
};

int setup_power_table_tk(void)
{
	int ret;

	init_mt_cpu_dvfs(&cpu_dvfs_infor);

	if (cpu_dvfs_infor.opp_tbl[0].cpufreq_khz == 0)
		return 0;

	cpufreq_register_notifier(&thermal_cpufreq_notifier_block,
					  CPUFREQ_POLICY_NOTIFIER);

	ret = setup_power_table(&cpu_dvfs_infor);
	power_table_ready = 1;
	return ret;
}
/* late_initcall(setup_power_table_tk); */
