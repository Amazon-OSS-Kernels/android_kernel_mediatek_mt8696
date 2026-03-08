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

#include <linux/random.h>
#include <mt-plat/met_drv.h>

/* local includes */
#include "mtk_cpufreq_internal.h"
#include "mtk_cpufreq_platform.h"
#include "mtk_cpufreq_debug.h"
#include "mtk_cpufreq_hybrid.h"
#include "mtk_cpufreq_opp_table.h"
#if defined(CONFIG_MTK_STATIC_POWER) && defined(CONFIG_POWER_TABLE_MT8696)
#include "mtk_throttle_table.h"
#endif

#define DCM_ENABLE 1
/*
 * Global Variables
 */
bool is_in_cpufreq;
DEFINE_MUTEX(cpufreq_mutex);
DEFINE_MUTEX(cpufreq_para_mutex);
struct opp_idx_tbl opp_tbl_m[NR_OPP_IDX];
int dvfs_disable_flag;
unsigned int dvfs_init_flag;
int new_idx_bk;
#ifdef ENABLE_DOE
static void modify_kernel_opp_table_by_doe
	(struct mt_cpu_freq_info *opp, struct mt_cpu_dvfs *p, int j)
{
	struct cpudvfs_doe *d = &dvfs_doe;
	struct buck_ctrl_t *vproc_p;
	int i;

	vproc_p = id_to_buck_ctrl(p->Vproc_buck_id);
	if ((d->doe_flag >> j) & 1) {
		for (i = 0; i < NR_FREQ; i++) {
			opp[i].cpufreq_khz =
			d->dts_opp_tbl[j][i * ARRAY_COL_SIZE] * 1000;
			opp[i].cpufreq_volt =
			vproc_p->buck_ops->transfer2volt
			(d->dts_opp_tbl[j][i * ARRAY_COL_SIZE + 1]);
#if 0
			tag_pr_notice("@@%s cluster = %d flag = %d opp[%d] = %d\n",
			__func__, j, d->doe_flag, i, opp[i].cpufreq_khz);
#endif

		}
	}
}

static void modify_kernel_opp_div_by_doe
	(struct mt_cpu_freq_method *opp_tbl_m, int j)
{
	struct cpudvfs_doe *d = &dvfs_doe;
	int i;

	if ((d->doe_flag >> j) & 1) {
		for (i = 0; i < NR_FREQ; i++) {
			opp_tbl_m[i].clk_div =
			d->dts_opp_tbl[j][i * ARRAY_COL_SIZE + 3];
			opp_tbl_m[i].pos_div =
			d->dts_opp_tbl[j][i * ARRAY_COL_SIZE + 2];
#if 0
			tag_pr_info("@@%s cls = %d posdiv = %d clk_div = %d\n",
					__func__, j, opp_tbl_m[i].pos_div,
					opp_tbl_m[i].clk_div);
#endif
		}
	}
}
#endif
struct mt_cpu_dvfs *id_to_cpu_dvfs(enum mt_cpu_dvfs_id id)
{
	return (id < NR_MT_CPU_DVFS) ? &cpu_dvfs[id] : NULL;
}

struct buck_ctrl_t *id_to_buck_ctrl(enum mt_cpu_dvfs_buck_id id)
{
	return (id < NR_MT_BUCK) ? &buck_ctrl[id] : NULL;
}

struct pll_ctrl_t *id_to_pll_ctrl(enum mt_cpu_dvfs_pll_id id)
{
	return (id < NR_MT_PLL) ? &pll_ctrl[id] : NULL;
}

int _search_available_freq_idx_under_v(struct mt_cpu_dvfs *p,
	unsigned int volt)
{
	int i;

	/* search available voltage */
	for (i = 0; i < p->nr_opp_tbl; i++) {
		if (volt >= cpu_dvfs_get_volt_by_idx(p, i))
			break;
	}

	WARN_ON(i >= p->nr_opp_tbl);

	return i;
}

int _search_available_freq_idx(struct mt_cpu_dvfs *p, unsigned int target_khz,
				      unsigned int relation)
{
	int new_opp_idx = -1;
	int i;

	if (relation == CPUFREQ_RELATION_L) {
		for (i = (signed int)(p->nr_opp_tbl - 1); i >= 0; i--) {
			if (cpu_dvfs_get_freq_by_idx(p, i) >= target_khz) {
				new_opp_idx = i;
				break;
			}
		}
	} else {		/* CPUFREQ_RELATION_H */
		for (i = 0; i < (signed int)p->nr_opp_tbl; i++) {
			if (cpu_dvfs_get_freq_by_idx(p, i) <= target_khz) {
				new_opp_idx = i;
				break;
			}
		}
	}

	return new_opp_idx;
}

#ifdef CONFIG_HYBRID_CPU_DVFS
#ifdef DVFS_CLUSTER_REMAPPING
#ifdef MET_READY
static void _set_met_tag_oneshot(int id, unsigned int target_khz)
{
	if (id == DVFS_CLUSTER_LL)
		met_tag_oneshot(0, "LL", target_khz);
	else if (id == DVFS_CLUSTER_L)
		met_tag_oneshot(0, "L", target_khz);
	else
		met_tag_oneshot(0, "B", target_khz);
}
#endif
#endif
static int _cpufreq_set_locked_secure(struct cpufreq_policy *policy,
	struct mt_cpu_dvfs *p, unsigned int target_khz, int log)
{
	int ret = -1;

	aee_record_cpu_dvfs_step(1);

	if (!policy) {
		tag_pr_notice("Can't get policy of %s\n",
		cpu_dvfs_get_name(p));
		goto out;
	}

#ifdef SINGLE_CLUSTER
	cpuhvfs_set_dvfs(cpufreq_get_cluster_id(p->cpu_id), target_khz);
#else
	cpuhvfs_set_dvfs(arch_get_cluster_id(p->cpu_id), target_khz);
#endif

#ifdef MET_READY
#ifdef DVFS_CLUSTER_REMAPPING
	if (policy->cpu < 4)
		_set_met_tag_oneshot(0, target_khz);
	else if ((policy->cpu >= 4) && (policy->cpu < 8))
		_set_met_tag_oneshot(1, target_khz);
	else if (policy->cpu >= 8)
		_set_met_tag_oneshot(2, target_khz);
#else
	if (policy->cpu < 4)
		met_tag_oneshot(0, "LL", target_khz);
	else if (policy->cpu >= 4)
		met_tag_oneshot(0, "L", target_khz);
	else if (policy->cpu >= 8)
		met_tag_oneshot(0, "B", target_khz);
#endif
#endif

	return 0;

out:
	aee_record_cpu_dvfs_step(0);

	return ret;
}

#endif	/* CONFIG_HYBRID_CPU_DVFS */

static void _mt_cpufreq_set(struct cpufreq_policy *policy,
	struct mt_cpu_dvfs *p, int new_opp_idx,
	enum mt_cpu_dvfs_action_id action)
{
	unsigned int target_freq;
#ifdef CONFIG_HYBRID_CPU_DVFS
	int ret = -1;
	int log = 0;
#endif

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (p->dvfs_disable_by_suspend || p->armpll_is_available != 1)
		return;

	if (do_dvfs_stress_test && action == MT_CPU_DVFS_NORMAL) {
		new_opp_idx = prandom_u32() % p->nr_opp_tbl;
		if (new_opp_idx == p->idx_opp_tbl)
			new_opp_idx = (p->nr_opp_tbl ? p->nr_opp_tbl - 1 : 0);
	}

	if (action != MT_CPU_DVFS_EEM_UPDATE)
		new_idx_bk = new_opp_idx;
	else
		new_idx_bk = -1;

	target_freq = cpu_dvfs_get_freq_by_idx(p, new_opp_idx);

	now[SET_DVFS] = ktime_get();

	aee_record_cpu_dvfs_in(p);

#ifdef CONFIG_HYBRID_CPU_DVFS
	ret = _cpufreq_set_locked_secure(policy, p, target_freq, log);
#endif

	aee_record_cpu_dvfs_out(p);

	delta[SET_DVFS] = ktime_sub(ktime_get(), now[SET_DVFS]);
	if (ktime_to_us(delta[SET_DVFS]) > ktime_to_us(max[SET_DVFS]))
		max[SET_DVFS] = delta[SET_DVFS];

	FUNC_EXIT(FUNC_LV_LOCAL);
}

/* one action could be combinational set */
void _mt_cpufreq_dvfs_request_wrapper(struct mt_cpu_dvfs *p, int new_opp_idx,
	enum mt_cpu_dvfs_action_id action, void *data)
{
	unsigned long flags;
	int i;
	/* PTP related */
	unsigned int **volt_tbl;
	struct buck_ctrl_t *vproc_p;

	cpufreq_lock(flags);
	/* action switch */
	switch (action) {
	case MT_CPU_DVFS_NORMAL:
#ifdef CONFIG_HYBRID_CPU_DVFS
		_mt_cpufreq_set(p->mt_policy, p, new_opp_idx, action);
#endif
		break;
	case MT_CPU_DVFS_EEM_UPDATE:
		volt_tbl = (unsigned int **)data;
		vproc_p = id_to_buck_ctrl(p->Vproc_buck_id);

		/* Update public table */
		for (i = 0; i < p->nr_opp_tbl; i++)
			p->opp_tbl[i].cpufreq_volt =
			vproc_p->buck_ops->transfer2volt((*volt_tbl)[i]);

		break;
	default:
		break;
	};
	cpufreq_unlock(flags);
}

static int _sync_opp_tbl_idx(struct mt_cpu_dvfs *p)
{
	unsigned int freq;
	int i;
	struct pll_ctrl_t *pll_p = id_to_pll_ctrl(p->Pll_id);

	freq = pll_p->pll_ops->get_cur_freq(pll_p);

	for (i = p->nr_opp_tbl - 1; i >= 0; i--) {
		if (freq <= cpu_dvfs_get_freq_by_idx(p, i))
			break;
	}

	if (WARN(i < 0, "CURR_FREQ %u IS OVER OPP0 %u\n",
				freq, cpu_dvfs_get_max_freq(p)))
		i = 0;

	p->idx_opp_tbl = i;
	aee_record_freq_idx(p, p->idx_opp_tbl);

	return 0;
}

static int _mt_cpufreq_sync_opp_tbl_idx(struct mt_cpu_dvfs *p)
{
	int ret = -1;

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (cpu_dvfs_is_available(p)) {
		ret = _sync_opp_tbl_idx(p);
		cpufreq_ver("%s freq = %d\n", cpu_dvfs_get_name(p),
			cpu_dvfs_get_cur_freq(p));
	}

	FUNC_EXIT(FUNC_LV_LOCAL);

	return ret;
}

static enum mt_cpu_dvfs_id _get_cpu_dvfs_id(unsigned int cpu_id)
{
	int cluster_id;

#ifdef SINGLE_CLUSTER
	cluster_id = cpufreq_get_cluster_id(cpu_id);
#else
	cluster_id = arch_get_cluster_id(cpu_id);
#endif

	return cluster_id;
}

static int _mt_cpufreq_setup_freqs_table(struct cpufreq_policy *policy,
	struct mt_cpu_freq_info *freqs, int num)
{
	struct mt_cpu_dvfs *p;
	int ret = 0;
#ifdef SINGLE_CLUSTER
	struct cpumask cpu_mask;
#endif

	FUNC_ENTER(FUNC_LV_LOCAL);

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));

	ret = cpufreq_frequency_table_cpuinfo(policy, p->freq_tbl_for_cpufreq);

	if (!ret)
		policy->freq_table = p->freq_tbl_for_cpufreq;

#ifdef SINGLE_CLUSTER
	cpufreq_get_cluster_cpus(&cpu_mask, _get_cpu_dvfs_id(policy->cpu));
	cpumask_copy(policy->cpus, &cpu_mask);
#else
	cpumask_copy(policy->cpus, topology_core_cpumask(policy->cpu));
#endif
	cpumask_copy(policy->related_cpus, policy->cpus);

	FUNC_EXIT(FUNC_LV_LOCAL);

	return 0;
}

/*
 * cpufreq driver
 */
static int _mt_cpufreq_verify(struct cpufreq_policy *policy)
{
	struct mt_cpu_dvfs *p;
	int ret;

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));
	if (!p)
		return -EINVAL;

	ret = cpufreq_frequency_table_verify(policy, p->freq_tbl_for_cpufreq);

	return ret;
}

static int _mt_cpufreq_target(struct cpufreq_policy *policy,
	unsigned int target_freq, unsigned int relation)
{
	struct mt_cpu_dvfs *p;
	unsigned int new_opp_idx;

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));
	if (!p)
		return -EINVAL;

	new_opp_idx = cpufreq_frequency_table_target(policy, target_freq,
							relation);
	if (new_opp_idx >= p->nr_opp_tbl)
		return -EINVAL;

	if (dvfs_disable_flag || p->dvfs_disable_by_suspend ||
					p->dvfs_disable_by_procfs)
		return -EPERM;

	_mt_cpufreq_dvfs_request_wrapper(p, new_opp_idx, MT_CPU_DVFS_NORMAL,
									NULL);

	return 0;
}

#ifndef ONE_CLUSTER
int cci_is_inited;
#endif
static int _mt_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret = -EINVAL;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_MODULE);

	policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	cpumask_setall(policy->cpus);

	policy->cpuinfo.transition_latency = 1000;

#ifdef CPU_DVFS_NOT_READY
	return 0;
#endif

	{
		enum mt_cpu_dvfs_id id = _get_cpu_dvfs_id(policy->cpu);
		struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
		unsigned int lv = _mt_cpufreq_get_cpu_level();
		struct opp_tbl_info *opp_tbl_info;
		struct opp_tbl_m_info *opp_tbl_m_info;
#ifndef ONE_CLUSTER
		struct opp_tbl_m_info *opp_tbl_m_cci_info;
		struct mt_cpu_dvfs *p_cci;
#endif

		cpufreq_ver("DVFS: @%s: %s(cpu_id = %d)\n",
			__func__, cpu_dvfs_get_name(p), p->cpu_id);

		opp_tbl_info = &opp_tbls[id][lv];
#ifdef ENABLE_DOE
		modify_kernel_opp_table_by_doe(opp_tbl_info->opp_tbl, p, id);
#endif
		p->cpu_level = lv;

		ret = _mt_cpufreq_setup_freqs_table(policy,
				opp_tbl_info->opp_tbl, opp_tbl_info->size);

		policy->cpuinfo.max_freq = cpu_dvfs_get_max_freq(p);
		policy->cpuinfo.min_freq = cpu_dvfs_get_min_freq(p);

		opp_tbl_m_info = &opp_tbls_m[id][lv];
#ifdef ENABLE_DOE
		modify_kernel_opp_div_by_doe(opp_tbls_m[id][lv].opp_tbl_m, id);
#endif
		p->freq_tbl = opp_tbl_m_info->opp_tbl_m;

		cpufreq_lock(flags);
		/* Sync p */
		if (_mt_cpufreq_sync_opp_tbl_idx(p) >= 0)
			if (p->idx_normal_max_opp == -1)
				p->idx_normal_max_opp = p->idx_opp_tbl;

		/* use cur phy freq is better */
		policy->cur = cpu_dvfs_get_cur_freq(p);
		policy->max = cpu_dvfs_get_max_freq(p);
		policy->min = cpu_dvfs_get_min_freq(p);
		p->mt_policy = policy;
		p->armpll_is_available = 1;

#ifndef ONE_CLUSTER
		/* Sync cci */
		if (cci_is_inited == 0) {
			p_cci = id_to_cpu_dvfs(MT_CPU_DVFS_CCI);

			/* init cci freq idx */
			if (_mt_cpufreq_sync_opp_tbl_idx(p_cci) >= 0)
				if (p_cci->idx_normal_max_opp == -1)
					p_cci->idx_normal_max_opp =
					p_cci->idx_opp_tbl;

			opp_tbl_m_cci_info = &opp_tbls_m[MT_CPU_DVFS_CCI][lv];
			p_cci->freq_tbl = opp_tbl_m_cci_info->opp_tbl_m;
			p_cci->mt_policy = NULL;
			p_cci->armpll_is_available = 1;
			cci_is_inited = 1;
		}
#endif

#ifdef CONFIG_HYBRID_CPU_DVFS
#ifdef SINGLE_CLUSTER
		cpuhvfs_set_cluster_on_off(cpufreq_get_cluster_id(p->cpu_id),
				1);
#else
		cpuhvfs_set_cluster_on_off(arch_get_cluster_id(p->cpu_id), 1);
#endif
#endif
		cpufreq_unlock(flags);
	}
#ifdef ENABLE_DOE
	srate_doe();
#endif
	if (ret)
		tag_pr_notice("failed to setup frequency table\n");

	dvfs_init_flag = 1;
	FUNC_EXIT(FUNC_LV_MODULE);

	return ret;
}

static int _mt_cpufreq_exit(struct cpufreq_policy *policy)
{
	return 0;
}

static unsigned int _mt_cpufreq_get(unsigned int cpu)
{
	struct mt_cpu_dvfs *p;

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(cpu));
	if (!p)
		return 0;

	return cpu_dvfs_get_cur_freq(p);
}

static struct freq_attr *_mt_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver _mt_cpufreq_driver = {
	.flags = CPUFREQ_ASYNC_NOTIFICATION,
	.verify = _mt_cpufreq_verify,
	.target = _mt_cpufreq_target,
	.init = _mt_cpufreq_init,
	.exit = _mt_cpufreq_exit,
	.get = _mt_cpufreq_get,
	.name = "mt-cpufreq",
	.attr = _mt_cpufreq_attr,
};

/*
 * Platform driver
 */
static int
_mt_cpufreq_pm_callback(struct notifier_block *nb,
		unsigned long action, void *ptr)
{
	struct mt_cpu_dvfs *p;
	int i;
	unsigned long flags;

	switch (action) {

	case PM_SUSPEND_PREPARE:
		cpufreq_ver("PM_SUSPEND_PREPARE\n");
		cpufreq_lock(flags);
		for_each_cpu_dvfs(i, p) {
			if (!cpu_dvfs_is_available(p))
				continue;
			p->dvfs_disable_by_suspend = true;
		}
		cpufreq_unlock(flags);
		break;
	case PM_HIBERNATION_PREPARE:
		break;

	case PM_POST_SUSPEND:
		cpufreq_ver("PM_POST_SUSPEND\n");
		cpufreq_lock(flags);
		for_each_cpu_dvfs(i, p) {
			if (!cpu_dvfs_is_available(p))
				continue;
			p->dvfs_disable_by_suspend = false;
		}
		cpufreq_unlock(flags);
		break;
	case PM_POST_HIBERNATION:
		break;

	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static int _mt_cpufreq_suspend(struct device *dev)
{
	return 0;
}

static int _mt_cpufreq_resume(struct device *dev)
{
	return 0;
}

static void _hps_request_wrapper(struct mt_cpu_dvfs *p,
	int new_opp_idx, enum hp_action action, void *data)
{
	enum mt_cpu_dvfs_id *id = (enum mt_cpu_dvfs_id *)data;
	struct mt_cpu_dvfs *act_p;

	act_p = id_to_cpu_dvfs(*id);
	/* action switch */
	/* switch (action & ~CPU_TASKS_FROZEN) { */
	switch (action) {
	case CPUFREQ_CPU_ONLINE:
		aee_record_cpu_dvfs_cb(2);
		if (act_p->armpll_is_available == 0 && act_p == p)
			act_p->armpll_is_available = 1;
#ifndef CONFIG_HYBRID_CPU_DVFS
		cpufreq_ver("DVFS - %s, CPU_ONLINE to %d\n",
		cpu_dvfs_get_name(p), new_opp_idx);
		_mt_cpufreq_set(p->mt_policy, p, new_opp_idx,
		MT_CPU_DVFS_ONLINE);
#endif
		break;
	case CPUFREQ_CPU_DOWN_PREPARE:
		aee_record_cpu_dvfs_cb(3);
#ifndef CONFIG_HYBRID_CPU_DVFS
		cpufreq_ver("DVFS - %s, CPU_DOWN_PREPARE to %d\n",
		cpu_dvfs_get_name(p), new_opp_idx);
		_mt_cpufreq_set(p->mt_policy, p, new_opp_idx, MT_CPU_DVFS_DP);
#endif
		if (act_p->armpll_is_available == 1 && act_p == p) {
			act_p->armpll_is_available = 0;
#ifdef CONFIG_HYBRID_CPU_DVFS
			aee_record_cpu_dvfs_cb(4);
			cpuhvfs_set_cluster_on_off(
			cpufreq_get_cluster_id(p->cpu_id), 0);
			aee_record_cpu_dvfs_cb(9);
#endif
			act_p->mt_policy = NULL;
			aee_record_cpu_dvfs_cb(10);
		}
		break;
	default:
		break;
	};
}

static void _mt_cpufreq_cpu_CB_wrapper(enum mt_cpu_dvfs_id cluster_id,
	unsigned int cpus, enum hp_action action)
{
	int i, j;
	struct mt_cpu_dvfs *p;
	unsigned long flags;

	aee_record_cpu_dvfs_cb(1);

	for (i = 0; i < nr_hp_action; i++) {
		if (cpu_dvfs_hp_action[i].cluster == cluster_id &&
			action == cpu_dvfs_hp_action[i].action &&
			cpus == cpu_dvfs_hp_action[i].trigged_core) {
			cpufreq_lock(flags);
			for_each_cpu_dvfs(j, p) {
				if (
cpu_dvfs_hp_action[i].hp_action_cfg[j].action_id != FREQ_NONE) {
					if (
cpu_dvfs_hp_action[i].hp_action_cfg[j].action_id == FREQ_HIGH)
						_hps_request_wrapper(p,
						0, action,
						(void *)&cluster_id);
					else if (
cpu_dvfs_hp_action[i].hp_action_cfg[j].action_id == FREQ_LOW)
						_hps_request_wrapper(p,
						p->nr_opp_tbl - 1, action,
							(void *)&cluster_id);
					else if (
cpu_dvfs_hp_action[i].hp_action_cfg[j].action_id == FREQ_USR_REQ)
						_hps_request_wrapper(p,
			cpu_dvfs_hp_action[i].hp_action_cfg[j].freq_idx,
						action,
							(void *)&cluster_id);
				}
			}
			cpufreq_unlock(flags);
		}
	}
	aee_record_cpu_dvfs_cb(0);
}

static int _mt_cpufreq_cpu_CB(enum hp_action action,
					unsigned int cpu)
{
#if 0
	unsigned int online_cpus = num_online_cpus();
#endif
	struct device *dev;
	enum mt_cpu_dvfs_id cluster_id;
	/* CPU mask - Get on-line cpus per-cluster */
	int i;
	struct mt_cpu_dvfs *p;
	struct cpumask dvfs_cpumask[NR_MT_CPU_DVFS];
	struct cpumask cpu_online_cpumask[NR_MT_CPU_DVFS];
	unsigned int cpus[NR_MT_CPU_DVFS];

	if (dvfs_disable_flag == 1)
		return NOTIFY_OK;

	cluster_id = cpufreq_get_cluster_id(cpu);

	for_each_cpu_dvfs_only(i, p) {
		cpufreq_get_cluster_cpus(&dvfs_cpumask[i], i);
		cpumask_and(&cpu_online_cpumask[i], &dvfs_cpumask[i],
				cpu_online_mask);
		cpus[i] = cpumask_weight(&cpu_online_cpumask[i]);
	}

#if 0
	cpufreq_ver("@%s():%d, cpu = %d, action = %lu, num_online_cpus = %d\n"
	, __func__, __LINE__, cpu, action, online_cpus);
#endif

	dev = get_cpu_device(cpu);

	if (dev) {
		/* switch (action & ~CPU_TASKS_FROZEN) { */
		switch (action) {
		case CPUFREQ_CPU_ONLINE:
		case CPUFREQ_CPU_DOWN_PREPARE:
		case CPUFREQ_CPU_DOWN_FAIED:
			_mt_cpufreq_cpu_CB_wrapper(cluster_id,
						cpus[cluster_id], action);
			break;
		default:
			break;
		}
	}

#if 0
	cpufreq_ver("@%s():%d, cpu = %d, action = %lu, num_online_cpus = %d\n"
	, __func__, __LINE__, cpu, action, online_cpus);
#endif

	return NOTIFY_OK;
}

static int cpuhp_cpufreq_online(unsigned int cpu)
{
	_mt_cpufreq_cpu_CB(CPUFREQ_CPU_ONLINE, cpu);

	return 0;
}

static int cpuhp_cpufreq_offline(unsigned int cpu)
{
	_mt_cpufreq_cpu_CB(CPUFREQ_CPU_DOWN_PREPARE, cpu);

	return 0;
}

static enum cpuhp_state hp_online;
static int _mt_cpufreq_pdrv_probe(struct platform_device *pdev)
{
	unsigned int ret;
	struct mt_cpu_dvfs *p;
	int j;

	FUNC_ENTER(FUNC_LV_MODULE);
	/* init proc */
	cpufreq_procfs_init();
	_mt_cpufreq_aee_init();

	ret = mt_cpufreq_regulator_map(pdev);
	if (ret)
		tag_pr_notice("%s regulator map fail\n", __func__);

#ifdef CONFIG_HYBRID_CPU_DVFS
	/* For SSPM/MCUPM probe */
	cpuhvfs_set_init_sta();
	/* Default disable schedule assist DVFS */
	cpuhvfs_set_sched_dvfs_disable(1);
#endif
	for_each_cpu_dvfs(j, p) {
		/* Prepare pll related address once */
		prepare_pll_addr(p->Pll_id);
		/* Prepare pmic related config once */
		prepare_pmic_config(p);
	}

	cpufreq_register_driver(&_mt_cpufreq_driver);

	hp_online = cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN,
						   "cpu_dvfs:online",
						   cpuhp_cpufreq_online,
						   cpuhp_cpufreq_offline);

	for_each_cpu_dvfs(j, p) {
		_sync_opp_tbl_idx(p);
	}

	pm_notifier(_mt_cpufreq_pm_callback, 0);

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int _mt_cpufreq_pdrv_remove(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	cpuhp_remove_state_nocalls(hp_online);
	cpufreq_unregister_driver(&_mt_cpufreq_driver);

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static const struct dev_pm_ops _mt_cpufreq_pm_ops = {
	.suspend = _mt_cpufreq_suspend,
	.resume = _mt_cpufreq_resume,
	.freeze = _mt_cpufreq_suspend,
	.thaw = _mt_cpufreq_resume,
	.restore = _mt_cpufreq_resume,
};

#ifndef CPU_DVFS_DT_REG
struct platform_device _mt_cpufreq_pdev = {
	.name = "mt-cpufreq",
	.id = -1,
};
#endif

#if defined(CONFIG_OF) && defined(CPU_DVFS_DT_REG)
static const struct of_device_id mt_cpufreq_match[] = {
	{ .compatible = "mediatek,mt-cpufreq", },
	{ },
};
#endif

static struct platform_driver _mt_cpufreq_pdrv = {
	.probe = _mt_cpufreq_pdrv_probe,
	.remove = _mt_cpufreq_pdrv_remove,
	.driver = {
		   .name = "mt-cpufreq",
		   .pm = &_mt_cpufreq_pm_ops,
		   .owner = THIS_MODULE,
#if defined(CONFIG_OF) && defined(CPU_DVFS_DT_REG)
		   .of_match_table = of_match_ptr(mt_cpufreq_match),
#endif
		   },
};

/* Module driver */
static int __init _mt_cpufreq_tbl_init(void)
{
	unsigned int lv = _mt_cpufreq_get_cpu_level();
	struct mt_cpu_dvfs *p;
	int i, j;
	struct opp_tbl_info *info;
	struct cpufreq_frequency_table *table;
#ifdef ENABLE_DOE
	struct cpudvfs_doe *d = &dvfs_doe;

	tag_pr_notice("@@~%s DVFS state = %d\n", __func__, dvfs_doe.state);
	if (!d->state)
		return 0;
#endif

#ifdef CPU_DVFS_NOT_READY
	return 0;
#endif

	/* Prepare OPP table for EEM and throttle table */
	for_each_cpu_dvfs(j, p) {
		info = &opp_tbls[j][lv];
#ifdef ENABLE_DOE
		modify_kernel_opp_table_by_doe(info->opp_tbl, p, j);
#endif
		if (!p->freq_tbl_for_cpufreq) {
			table = kzalloc(
			(info->size + 1) * sizeof(*table), GFP_KERNEL);

			if (!table)
				return -ENOMEM;

			for (i = 0; i < info->size; i++) {
				table[i].driver_data = i;
				table[i].frequency =
				info->opp_tbl[i].cpufreq_khz;
				#if defined(CONFIG_MTK_STATIC_POWER) \
					&& defined(CONFIG_POWER_TABLE_MT8696)
				if (p->id == MT_CPU_DVFS_LL) {
					opp_tbl_default[i].cpufreq_khz =
						info->opp_tbl[i].cpufreq_khz;
					opp_tbl_default[i].cpufreq_volt =
						info->opp_tbl[i].cpufreq_volt;
				}
				#endif
			}

			table[info->size].driver_data = i;
			table[info->size].frequency =
			CPUFREQ_TABLE_END;

			p->opp_tbl = info->opp_tbl;
			p->nr_opp_tbl = info->size;
			p->freq_tbl_for_cpufreq = table;
		}
	}
	return 0;
}

static int __init _mt_cpufreq_pdrv_init(void)
{
	int ret = 0;
	struct cpumask cpu_mask;
	unsigned int cluster_num;
	int i;

#ifdef ENABLE_DOE
	tag_pr_notice("@@~ %s DVFS state = %d\n", __func__, dvfs_doe.state);
	if (!dvfs_doe.state)
		return 0;
#endif

#ifdef CPU_DVFS_NOT_READY
	dvfs_disable_flag = 1;
	return 0;
#endif

	mt_cpufreq_dts_map();

#ifdef SINGLE_CLUSTER
	cluster_num = (unsigned int)cpufreq_get_nr_clusters();
#else
	cluster_num = (unsigned int)arch_get_nr_clusters();
#endif

	for (i = 0; i < cluster_num; i++) {
#ifdef SINGLE_CLUSTER
		cpufreq_get_cluster_cpus(&cpu_mask, i);
#else
		arch_get_cluster_cpus(&cpu_mask, i);
#endif
		cpu_dvfs[i].cpu_id = cpumask_first(&cpu_mask);
		tag_pr_info("cluster_id = %d, cluster_cpuid = %d\n",
		i, cpu_dvfs[i].cpu_id);
	}

#ifdef CONFIG_HYBRID_CPU_DVFS	/* before platform_driver_register */
	ret = cpuhvfs_module_init();
#endif

#ifndef CPU_DVFS_DT_REG
	/* register platform device/driver */
	ret = platform_device_register(&_mt_cpufreq_pdev);

	if (ret) {
		tag_pr_notice("fail to register cpufreq device @ %s()\n",
		__func__);
	}
#endif

	ret = platform_driver_register(&_mt_cpufreq_pdrv);

	if (ret) {
		tag_pr_notice("fail to register cpufreq driver @ %s()\n",
		__func__);
#ifndef CPU_DVFS_DT_REG
		platform_device_unregister(&_mt_cpufreq_pdev);
#endif
	}

	return ret;
}

static void __exit _mt_cpufreq_pdrv_exit(void)
{
	platform_driver_unregister(&_mt_cpufreq_pdrv);
#ifndef CPU_DVFS_DT_REG
	platform_device_unregister(&_mt_cpufreq_pdev);
#endif
}

module_init(_mt_cpufreq_tbl_init);
late_initcall(_mt_cpufreq_pdrv_init);
module_exit(_mt_cpufreq_pdrv_exit);

MODULE_DESCRIPTION("MediaTek CPU DVFS Driver v0.3.1");
