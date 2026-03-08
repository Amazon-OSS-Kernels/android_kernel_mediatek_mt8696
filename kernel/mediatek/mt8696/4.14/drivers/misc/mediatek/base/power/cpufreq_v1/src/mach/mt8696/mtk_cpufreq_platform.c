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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>

#ifdef CONFIG_MTK_FREQ_HOPPING
#include <mtk_freqhopping_drv.h>
#else
#define FH_PLL0 0 /* LL : ARMPLL */
#define FH_PLL7 7 /* CCI : CCIPLL */
#endif

#include "mtk_cpufreq_platform.h"
#include "../../mtk_cpufreq_hybrid.h"
#include "mtk_devinfo.h"

static struct regulator *regulator_proc;
static struct regulator *regulator_sram;

static unsigned long apmixed_base	= 0x1000c000;

#define APMIXED_NODE		"mediatek,mt8696-apmixedsys"
#define MCUCFG_NODE		"mediatek,mt8696-mcucfg"
#define ARMPLL_LL_CON1		(apmixed_base + 0x114)	/* ARMPLL1 */
#define CCIPLL_CON0		(apmixed_base + 0x2a0)	/* CCIPLL posdiv */
#define CCIPLL_CON1		(apmixed_base + 0x2a4)	/* CCIPLL pcw */

static unsigned int pmic_ready_flag;

struct cpudvfs_doe dvfs_doe = {
		.doe_flag = 0,
		.dtsn = {"L-table",  "CCI-table"},
		.state = 1,
		.change_flag = 0,
		.lt_rs_t = UP_SRATE,
		.lt_dw_t = DOWN_SRATE,
		.bg_rs_t = UP_SRATE,
		.bg_dw_t = DOWN_SRATE,
};
struct mt_cpu_dvfs cpu_dvfs[NR_MT_CPU_DVFS] = {
	[MT_CPU_DVFS_LL] = {
		.name		= __stringify(MT_CPU_DVFS_LL),
		.id		= MT_CPU_DVFS_LL,
		.cpu_id		= 0,
		.idx_normal_max_opp = -1,
		.Vproc_buck_id	= CPU_DVFS_VPROC,
		.Vsram_buck_id	= CPU_DVFS_VSRAM,
		.Pll_id		= PLL_LL_CLUSTER,
	},

	[MT_CPU_DVFS_CCI] = {
		.name		= __stringify(MT_CPU_DVFS_CCI),
		.id		= MT_CPU_DVFS_CCI,
		.cpu_id		= 10,
		.idx_normal_max_opp = -1,
		.Vproc_buck_id	= CPU_DVFS_VPROC,
		.Vsram_buck_id	= CPU_DVFS_VSRAM,
		.Pll_id		= PLL_CCI_CLUSTER,
	},
};

static unsigned int get_cur_volt_proc_cpu(struct buck_ctrl_t *buck_p)
{
	unsigned int rdata;

	rdata = regulator_get_voltage(regulator_proc) / 10;

	return rdata;
}

static unsigned int ma5721b_vsram_transfer2pmicval(unsigned int volt)
{
	return ((volt - VSRAM_BASE) + VSRAM_STEP - 1) / VSRAM_STEP;
}

static unsigned int ma5721b_vsram_transfer2volt(unsigned int val)
{
	return val * VSRAM_STEP + VSRAM_BASE;
}

static unsigned int mt6393_vproc_transfer2pmicval(unsigned int volt)
{
	return ((volt - VPROC_BASE) + VPROC_STEP - 1) / VPROC_STEP;
}

static unsigned int mt6393_vproc_transfer2volt(unsigned int val)
{
	return val * VPROC_STEP + VPROC_BASE;
}

static unsigned int get_cur_volt_sram_cpu(struct buck_ctrl_t *buck_p)
{
	unsigned int rdata = 0;

	if (!IS_ERR(regulator_sram))
		rdata = regulator_get_voltage(regulator_sram) / 10;

	return rdata;
}

/* upper layer CANNOT use 'set' function in secure path */
static struct buck_ctrl_ops buck_ops_mt6393_vproc = {
	.get_cur_volt		= get_cur_volt_proc_cpu,
	.transfer2pmicval	= mt6393_vproc_transfer2pmicval,
	.transfer2volt		= mt6393_vproc_transfer2volt,
};

static struct buck_ctrl_ops buck_ops_ma5721b_vsram = {
	.get_cur_volt		= get_cur_volt_sram_cpu,
	.transfer2pmicval	= ma5721b_vsram_transfer2pmicval,
	.transfer2volt		= ma5721b_vsram_transfer2volt,
};

struct buck_ctrl_t buck_ctrl[NR_MT_BUCK] = {
	[CPU_DVFS_VPROC] = {
		.name		= __stringify(BUCK_mt6393_VPROC),
		.buck_id	= CPU_DVFS_VPROC,
		.buck_ops	= &buck_ops_mt6393_vproc,
	},

	[CPU_DVFS_VSRAM] = {
		.name		= __stringify(BUCK_ma5721b_VSRAM),
		.buck_id	= CPU_DVFS_VSRAM,
		.buck_ops	= &buck_ops_ma5721b_vsram,
	},
};

/* PMIC Part */
void prepare_pmic_config(struct mt_cpu_dvfs *p)
{
}

int __attribute__((weak)) sync_dcm_set_mp0_freq(unsigned int mhz)
{
	return 0;
}

int __attribute__((weak)) sync_dcm_set_mp1_freq(unsigned int mhz)
{
	return 0;
}

int __attribute__((weak)) sync_dcm_set_mp2_freq(unsigned int mhz)
{
	return 0;
}

int __attribute__((weak)) sync_dcm_set_cci_freq(unsigned int mhz)
{
	return 0;
}

/* PLL Part */
void prepare_pll_addr(enum mt_cpu_dvfs_pll_id pll_id)
{
	struct pll_ctrl_t *pll_p = id_to_pll_ctrl(pll_id);

	if (pll_p == NULL)
		return;
	pll_p->armpll_addr = (unsigned int *)(pll_id ==
		PLL_LL_CLUSTER ? ARMPLL_LL_CON1 : CCIPLL_CON1);
}


/* Frequency API */
#define INTEGER_BITS	8
static unsigned int _cpu_freq_calc(struct pll_ctrl_t *pll_p,
	unsigned int pcw, unsigned int posdiv)
{
	int pcwbits;
	int pcwfbits;
	int ibits = INTEGER_BITS;
	u64 vco;
	u8 c = 0;

	if (pll_p->pll_id == PLL_LL_CLUSTER)
		pcwbits = 28;
	else
		pcwbits = 32;

	/* The fractional part of the PLL divider. */
	pcwfbits = pcwbits - ibits;

	vco = (u64)26000 * pcw;

	if (pcwfbits && (vco & _BITMASK_((pcwfbits - 1):0)))
		c = 1;

	vco >>= pcwfbits;

	if (c)
		vco++;

	return (unsigned int)(((unsigned long)vco + posdiv - 1) / posdiv);
}

unsigned int get_cur_phy_freq(struct pll_ctrl_t *pll_p)
{
	unsigned int con1;
	unsigned int cur_khz;
	unsigned int posdiv;
	unsigned int pcw;

	con1 = cpufreq_read(pll_p->armpll_addr);
	if (pll_p->pll_id == PLL_LL_CLUSTER) {
		posdiv = _GET_BITS_VAL_(2:0, con1);
		pcw = (con1 & _BITMASK_(31:4)) >> 4;
	} else {
		posdiv = _GET_BITS_VAL_(6:4, cpufreq_read(CCIPLL_CON0));
		pcw = (con1 & _BITMASK_(31:0));
	}

	posdiv = 1 << posdiv;
	cur_khz = _cpu_freq_calc(pll_p, pcw, posdiv);

	pr_debug("@%s: cur_khz = %d, con1[%p] = 0x%x, posdiv_val = %d, pcw = 0x%x\n",
		__func__, cur_khz, pll_p->armpll_addr,
		con1, posdiv, pcw);

	return cur_khz;
}

/* upper layer CANNOT use 'set' function in secure path */
static struct pll_ctrl_ops pll_ops_ll = {
	.get_cur_freq		= get_cur_phy_freq,
};

static struct pll_ctrl_ops pll_ops_cci = {
	.get_cur_freq		= get_cur_phy_freq,
};

struct pll_ctrl_t pll_ctrl[NR_MT_PLL] = {
	[PLL_LL_CLUSTER] = {
		.name		= __stringify(PLL_LL_CLUSTER),
		.pll_id		= PLL_LL_CLUSTER,
		.hopping_id	= FH_PLL0,	/* ARMPLL1 */
		.pll_ops	= &pll_ops_ll,
	},

	[PLL_CCI_CLUSTER] = {
		.name		= __stringify(PLL_CCI_CLUSTER),
		.pll_id		= PLL_CCI_CLUSTER,
		.hopping_id	= FH_PLL7,	/* CCIPLL */
		.pll_ops	= &pll_ops_cci,
	},
};

/* Always put action cpu at last */
struct hp_action_tbl cpu_dvfs_hp_action[] = {
	{
		.action		= CPUFREQ_CPU_DOWN_PREPARE,
		.cluster	= MT_CPU_DVFS_LL,
		.trigged_core	= 1,
		.hp_action_cfg[MT_CPU_DVFS_LL].action_id = FREQ_LOW,
	},
};

unsigned int nr_hp_action = ARRAY_SIZE(cpu_dvfs_hp_action);

int mt_cpufreq_regulator_map(struct platform_device *pdev)
{
	regulator_proc = regulator_get_optional(&pdev->dev, "VPROC");
	if (GEN_DB_ON(IS_ERR(regulator_proc), "vproc Get Failed")) {
		tag_pr_info("@@vproc Get Failed, ret = %ld\n",
			PTR_ERR(regulator_proc));
		return -ENODEV;
	}

	regulator_sram =
		regulator_get_optional(&pdev->dev, "VSRAM");
	if (GEN_DB_ON(IS_ERR(regulator_sram), "vsram_proc Get Failed")) {
		tag_pr_info("@@vsram Get Failed, ret = %ld\n",
			PTR_ERR(regulator_sram));
		//return -ENODEV;
	}

	pmic_ready_flag = 1;
	tag_pr_info("@@regulator map success!!\n");
	return 0;
}

int mt_cpufreq_dts_map(void)
{
	struct device_node *node;

	/* apmixed */
	node = of_find_compatible_node(NULL, NULL, APMIXED_NODE);
	if (GEN_DB_ON(!node, "APMIXED Not Found"))
		return -ENODEV;

	apmixed_base = (unsigned long)of_iomap(node, 0);
	if (GEN_DB_ON(!apmixed_base, "APMIXED Map Failed"))
		return -ENOMEM;

	return 0;
}

unsigned int _mt_cpufreq_get_cpu_level(void)
{
	unsigned int lv = CPU_LEVEL_0;

	int val = (get_devinfo_with_index(7) & 0xFF);
	tag_pr_info("Settle time(%d, %d) efuse_val = 0x%x, vsram = %d\n",
		UP_SRATE, DOWN_SRATE, val, vsram_separated);

	switch (val) {
	case 1:	/* 1900M */
		break;
	case 2: /* 1820M */
		break;
	case 3: /* 1800M */
		if (vsram_separated)
			lv = CPU_LEVEL_1;
		else
			lv = CPU_LEVEL_0;
		break;
	case 4: /* 1700M */
		break;
	default:
		if (vsram_separated)
			lv = CPU_LEVEL_1;
		else
			lv = CPU_LEVEL_0;
		break;
	}

	return lv;
}

unsigned int cpufreq_get_nr_clusters(void)
{
	return (NR_MT_CPU_DVFS - 1);
}

void cpufreq_get_cluster_cpus(struct cpumask *cpu_mask, unsigned int cid)
{
	if (cid == 0) {
		cpumask_setall(cpu_mask);
	}

	cpufreq_deferred("cluster%d: cpumask = %*pbl\n",
		cid, cpumask_pr_args(cpu_mask));
}

unsigned int cpufreq_get_cluster_id(unsigned int cpu_id)
{
	struct cpumask cpu_mask;
	int i;

	for (i = 0; i < NR_MT_CPU_DVFS - 1; i++) {
		cpufreq_get_cluster_cpus(&cpu_mask, i);
		if (cpumask_test_cpu(cpu_id, &cpu_mask)) {
			cpufreq_deferred("cluster%d: cpumask = %*pbl\n",
			i, cpumask_pr_args(&cpu_mask));
			return i;
		}
	}

	return 0;
}
