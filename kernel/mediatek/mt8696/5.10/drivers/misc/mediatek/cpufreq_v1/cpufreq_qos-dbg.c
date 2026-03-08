// SPDX-License-Identifier: GPL-2.0
/*
 * cpufreq_qos-dbg.c - cpufreq qos debug Driver
 *
 * Copyright (c) 2021 MediaTek Inc.
 * Hsin-Hsiung Wang <hsin-hsiung.wang@mediatek.com>
 */

/* system includes */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cpufreq.h>

#include "mtk_cpu_dbg.h"

int cpufreq_qos_cluster = 2;

static inline int mt_qos_get_value(struct pm_qos_constraints *c)
{
	if (plist_head_empty(&c->list))
		return c->no_constraint_value;

	switch (c->type) {
	case PM_QOS_MIN:
		return plist_first(&c->list)->prio;

	case PM_QOS_MAX:
		return plist_last(&c->list)->prio;

	default:
		pr_info("%s: Bad type\n", __func__);
		return PM_QOS_DEFAULT_VALUE;
	}
}

ssize_t dump_freq_qos_info(struct pm_qos_constraints *c, char *buf)
{
	struct freq_qos_request *req;
	char *type;
	int tot_reqs = 0;
	int active_reqs = 0;
	ssize_t i = 0;

	if (plist_head_empty(&c->list)) {
		i += sprintf(buf, "Empty!\n");
		goto out;
	}

	switch (c->type) {
	case PM_QOS_MIN:
		type = "Minimum";
		break;
	case PM_QOS_MAX:
		type = "Maximum";
		break;
	default:
		type = "Unknown";
	}

	plist_for_each_entry(req, &c->list, pnode) {
		char *state = "Default";

		if ((req->pnode).prio != c->default_value) {
			active_reqs++;
			state = "Active";
		}
		tot_reqs++;
		i += scnprintf(&buf[i], (PAGE_SIZE - i - 2), "%d: %d: %sS\n", tot_reqs,
			   (req->pnode).prio, state);
	}

	i += scnprintf(&buf[i], (PAGE_SIZE - i - 2),
			"Type=%s, Value=%d, Requests: active=%d / total=%d\n",
			type, mt_qos_get_value(c), active_reqs, tot_reqs);

out:
	return i;
}

static void mt_get_qos_constraints(struct seq_file *m, int qos_type)
{
	int i = 0;
	struct cpufreq_policy *policy;
	struct pm_qos_constraints *c;

	for (i = 0; i < cpufreq_qos_cluster; i++) {
		char buf[1024] = {0};

		policy = cpufreq_cpu_get(i * 4);
		if (qos_type == PM_QOS_MAX)
			c = &policy->constraints.max_freq;
		else if (qos_type == PM_QOS_MIN)
			c = &policy->constraints.min_freq;
		else {
			pr_info("%s: Bad constraints\n", __func__);
			return;
		}
		dump_freq_qos_info(c, buf);
		seq_printf(m, "Cluster %d\n%s", i, buf);
	}
}

static int cpufreq_qos_min_proc_show(struct seq_file *m, void *v)
{
	mt_get_qos_constraints(m, PM_QOS_MIN);
	return 0;
}

static int cpufreq_qos_max_proc_show(struct seq_file *m, void *v)
{
	mt_get_qos_constraints(m, PM_QOS_MAX);
	return 0;
}

PROC_FOPS_RO(cpufreq_qos_min);
PROC_FOPS_RO(cpufreq_qos_max);

static int create_debug_fs(void)
{
	int i;
	struct proc_dir_entry *eem_dir = NULL;
	struct pentry {
		const char *name;
		const struct proc_ops *fops;
		void *data;
	};

	struct pentry eem_entries[] = {
		PROC_ENTRY(cpufreq_qos_min),
		PROC_ENTRY(cpufreq_qos_max),
	};

	eem_dir = proc_mkdir("cpufreq_qos", NULL);
	for (i = 0; i < ARRAY_SIZE(eem_entries); i++) {
		if (!proc_create(eem_entries[i].name, 0660,
					eem_dir, eem_entries[i].fops)) {
			pr_info("[%s]: create /proc/cpufreq_qos/%s failed\n",
					__func__,
					eem_entries[i].name);
		}
	}

	return 0;
}

int mtk_cpufreq_qos_dbg_init(void)
{
	return create_debug_fs();
}
EXPORT_SYMBOL_GPL(mtk_cpufreq_qos_dbg_init);

MODULE_DESCRIPTION("MTK CPUFREQ QOS Platform Driver v1");
MODULE_AUTHOR("Hsin-Hsiung Wang <hsin-hsiung.wang@mediatek.com>");
MODULE_LICENSE("GPL v2");
