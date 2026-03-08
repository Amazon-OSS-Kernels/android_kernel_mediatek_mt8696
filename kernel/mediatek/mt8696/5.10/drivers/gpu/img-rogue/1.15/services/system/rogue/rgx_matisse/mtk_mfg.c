// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/preempt.h>
#include <linux/ioport.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/init.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/syscalls.h>
#include <linux/arm-smccc.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
/*#include <mt-plat/mtk_freqhopping_api.h>*/
/*#include "mtk_mfg_counter.h"*/
#include "mtk_mfg.h"

/*
 * This will disable any clk/pll/regulator setting operations.
 * We use this to ensure HW is using default value from preloader.
 */
/*#define MFG_BRING_UP_ALL_ON*/

#define MFG_READ32(r) readl(mfg_start + (r))
#define MFG_WRITE32(v, r) writel((v), mfg_start + (r))
#define MFG_SET(b, r) set_bit((b), mfg_start + (r))

static struct platform_device *mfg_dev;

#ifndef MFG_BRING_UP_ALL_ON

static struct clk *clk_pll;
static struct clk *clk_mfg_sel;

struct mfg_opp_park {
	unsigned long freq;
	long volt;
	const char *name;
	struct clk *clk_pll;
};

static struct mfg_opp_park opp_park[] = {
	{
		.freq = 800000000,
		.volt = 900000,
		.name = "univpll624m",
		.clk_pll = NULL
	},
	{
		.freq = 700000000,
		.volt = 800000,
		.name = "univpll624m",
		.clk_pll = NULL
	},
	{
		.freq = 400000000,
		.volt = 700000,
		.name = "univpll312m",
		.clk_pll = NULL
	}
};
#define MFG_OPP_PARK_LENGTH (ARRAY_SIZE(opp_park))

static struct regulator *vgpu;
static struct regulator *vgpu_sram;
static bool runtime_vgpu_ctrl;

static DEFINE_MUTEX(enable_lock);
static int enable_count;

static void __iomem *mfg_start;

/*
 * NOTE. Normally disable hwapm while enable counter.
 */
static bool enable_hwapm = true;

static bool pll_support_hopping;
/*
 * Store freq set by GPU driver.
 * They are for 'snap' getters .
 */
static unsigned long current_freq;
static int current_volt;
static int current_volt_sram;

enum {
	PD_ASYNC = 0,
	PD_2D,
	PD_3D,
	PD_MAX
};

static int dcm_level = 2;

#ifdef CONFIG_OF
static const struct of_device_id mfg_dt_ids[] = {
	{.compatible = "mediatek,mt8696-mfg"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mfg_dt_ids);

static const struct of_device_id mfg_async_dt_ids[] = {
	{.compatible = "mediatek,mt8696-mfg-async"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mfg_async_dt_ids);

static const struct of_device_id mfg_2d_dt_ids[] = {
	{.compatible = "mediatek,mt8696-mfg-2d"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mfg_2d_dt_ids);

static const struct of_device_id mfg_3d_dt_ids[] = {
	{.compatible = "mediatek,mt8696-mfg-3d"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mfg_3d_dt_ids);

#endif /* CONFIG_OF */

struct platform_device *mtcmos_dev[3];

static int mtcmos_enable(unsigned int idx)
{
	pm_runtime_get_sync(&mtcmos_dev[idx]->dev);
	return 0;
}

static int mtcmos_disable(unsigned int idx)
{
	pm_runtime_put_sync(&mtcmos_dev[idx]->dev);
	return 0;
}

static int mfg_device_async_probe(struct platform_device *pdev)
{
	mtcmos_dev[PD_ASYNC] = pdev;
	pm_runtime_enable(&pdev->dev);
	return 0;
}

static int mfg_device_async_remove(struct platform_device *pdev)
{
	mtcmos_dev[PD_ASYNC] = NULL;
	pm_runtime_disable(&pdev->dev);
	return 0;
}


static struct platform_driver mtk_mfg_async_driver = {
	.probe = mfg_device_async_probe,
	.remove = mfg_device_async_remove,
	.driver = {
		   .name = "mfg_async",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(mfg_async_dt_ids),
		   },
};

static int mfg_device_2d_probe(struct platform_device *pdev)
{
	mtcmos_dev[PD_2D] = pdev;
	pm_runtime_enable(&pdev->dev);
	return 0;
}

static int mfg_device_2d_remove(struct platform_device *pdev)
{
	mtcmos_dev[PD_2D] = NULL;
	pm_runtime_disable(&pdev->dev);
	return 0;
}
static struct platform_driver mtk_mfg_2d_driver = {
	.probe = mfg_device_2d_probe,
	.remove = mfg_device_2d_remove,
	.driver = {
		   .name = "mfg_2d",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(mfg_2d_dt_ids),
		   },
};


static int mfg_device_3d_probe(struct platform_device *pdev)
{
	mtcmos_dev[PD_3D] = pdev;
	pm_runtime_enable(&pdev->dev);
	return 0;
}

static int mfg_device_3d_remove(struct platform_device *pdev)
{
	mtcmos_dev[PD_3D] = NULL;
	pm_runtime_disable(&pdev->dev);
	return 0;
}
static struct platform_driver mtk_mfg_3d_driver = {
	.probe = mfg_device_3d_probe,
	.remove = mfg_device_3d_remove,
	.driver = {
		   .name = "mfg_3d",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(mfg_3d_dt_ids),
		   },
};

static int mfg_regulator_enable(void)
{
	int ret = 0;

	if (vgpu) {
		ret = regulator_enable(vgpu);
		if (ret)
			pr_err("vgpu_sram enable failed %d\n", ret);
	}
	if (vgpu_sram) {
		ret = regulator_enable(vgpu_sram);
		if (ret)
			pr_err("vgpu enable failed %d\n", ret);
	}
	return ret;
}

static void mfg_regulator_disable(void)
{
	if (vgpu_sram)
		regulator_disable(vgpu_sram);
	if (vgpu)
		regulator_disable(vgpu);
}

static void mfg_power_off(void)
{
	if (!enable_hwapm)
		mtcmos_disable(PD_3D);

	clk_disable_unprepare(clk_mfg_sel);

	mtcmos_disable(PD_2D);
	mtcmos_disable(PD_ASYNC);

	clk_disable_unprepare(clk_pll);

	if (runtime_vgpu_ctrl)
		mfg_regulator_disable();
}

static void mfg_power_on(void)
{
	int ret = 0;
	struct arm_smccc_res res;

	if (runtime_vgpu_ctrl) {
		ret = mfg_regulator_enable();
		if (ret < 0)
			return;
	}

	clk_prepare_enable(clk_pll);
	clk_set_parent(clk_mfg_sel, clk_pll);

	mtcmos_enable(PD_ASYNC);
	mtcmos_enable(PD_2D);

	clk_prepare_enable(clk_mfg_sel);

	arm_smccc_smc(MTK_SIP_KERNEL_MFG_CONTROL,
		MFG_RGX_BUS_SECURE_UNLOCK,
		0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	if (ret) {
		pr_err("MFG smc bus unlock failed %d\n", ret);
		return;
	}

	if (!enable_hwapm)
		mtcmos_enable(PD_3D);
}

void mfg_dump_regs_seq(struct seq_file *m, const char *prefix)
{
	int i;

	if (m) {
		if (enable_hwapm)
			seq_puts(m, "hwapm enabled\n");
		else
			seq_puts(m, "hwapm disabled\n");

		seq_printf(m, "dcm level %d\n", dcm_level);

		if (vgpu)
			seq_printf(m, "vgpu: %d, %s\n",
				regulator_get_voltage(vgpu),
				regulator_is_enabled(vgpu) ?
					"enabled":"disabled");
		else
			seq_puts(m, "vgpu: static\n");

		if (vgpu_sram)
			seq_printf(m, "vgpu_sram: %d, %s\n",
				regulator_get_voltage(vgpu_sram),
				regulator_is_enabled(vgpu_sram) ?
					"enabled":"disabled");
		else
			seq_puts(m, "vgpu_sram: static\n");

		for (i = 0; i < PD_MAX; i++)
			seq_printf(m, "Power: %d %d\n",
				i,
				atomic_read(
					&mtcmos_dev[i]->dev.power.usage_count));

		if (clk_pll && clk_mfg_sel) {
			struct clk *parent;

			seq_printf(m, "Clk: %s, Enable: %s, rate: %lu\n",
				__clk_get_name(clk_mfg_sel),
				__clk_is_enabled(clk_mfg_sel) ? "Y" : "N",
				clk_get_rate(clk_mfg_sel));

			parent = clk_get_parent(clk_mfg_sel);
			if (parent)
				seq_printf(m, "--Parent Clk: %s, Enable: %s, rate :%lu\n\n",
					__clk_get_name(parent),
					__clk_is_enabled(parent) ? "Y" : "N",
					clk_get_rate(parent));
			else
				seq_puts(m, "--Parent Clk : NONE\n\n");

		}

		mutex_lock(&enable_lock);
		if (enable_count > 0)
			seq_hex_dump(m, "MFG ",
				DUMP_PREFIX_OFFSET, 4, 4,
				mfg_start, 0x1000, false);
		else
			seq_puts(m, "MFG Power down, no regs\n");
		mutex_unlock(&enable_lock);
	}
}
EXPORT_SYMBOL(mfg_dump_regs_seq);

void mtk_mfg_dump_regs(const char *prefix)
{
	mfg_dump_regs_seq(NULL, prefix);
}

void mfg_write(u32 off, u32 val)
{
	MFG_WRITE32(val, off);
}
EXPORT_SYMBOL(mfg_write);

static void mfg_setup_hwapm(void)
{
	if (enable_hwapm) {
		MFG_WRITE32(0x01a80000, 0xc10);
		MFG_WRITE32(0x00080010, 0xc14);
		MFG_WRITE32(0x00100008, 0xc18);
		MFG_WRITE32(0x00b800c8, 0xc1c);
		MFG_WRITE32(0x00b000c0, 0xc20);
		MFG_WRITE32(0x00c000c8, 0xc24);
		MFG_WRITE32(0x00b000b8, 0xc28);
		MFG_WRITE32(0x00d000d0, 0xc2c);
		MFG_WRITE32(0x00d000d0, 0xc30);
		MFG_WRITE32(0x00d00000, 0xc34);
		MFG_WRITE32(0x9004001b, 0xc00);
		MFG_WRITE32(0x8004001b, 0xc00);
	}
}

static void mfg_setup_dcm(void)
{
	if (dcm_level > 0) {
		MFG_WRITE32(0xf << 22, 0x20);
		if (dcm_level > 1) {
			MFG_WRITE32(0x700, 0xb0);
			MFG_WRITE32(1, 0x24);
		}
	}
}


void mtk_mfg_enable_gpu(void)
{
	mutex_lock(&enable_lock);

	enable_count++;
	/* If there is no error, this should not happen */
	if (enable_count != 1) {
		pr_err("ERR: MFG enable %d", enable_count);
		mutex_unlock(&enable_lock);
		return;
	}

	mfg_power_on();

	mfg_setup_hwapm();

	mfg_setup_dcm();

	mutex_unlock(&enable_lock);
}
EXPORT_SYMBOL(mtk_mfg_enable_gpu);

void mtk_mfg_disable_gpu(void)
{
	mutex_lock(&enable_lock);

	enable_count--;
	/* This should not happen */
	if (enable_count != 0) {
		pr_err("ERR: MFG disable %d", enable_count);
		mutex_unlock(&enable_lock);
		return;
	}

	mfg_power_off();

	mutex_unlock(&enable_lock);
}
EXPORT_SYMBOL(mtk_mfg_disable_gpu);

bool mtk_mfg_is_ready(void)
{
	return (mfg_start != NULL);
}
EXPORT_SYMBOL(mtk_mfg_is_ready);

unsigned long mtk_mfg_get_snap_freq(void)
{
	return current_freq;
}
EXPORT_SYMBOL(mtk_mfg_get_snap_freq);

unsigned long mtk_mfg_get_freq(void)
{
	if (clk_pll)
		return clk_get_rate(clk_pll);
	return current_freq;
}
EXPORT_SYMBOL(mtk_mfg_get_freq);

void mtk_mfg_set_freq(unsigned long freq, bool hopping)
{
	int i, volt;
	struct clk *clk_park = NULL;

	/* Avoid enable/disable while setting freq */
	mutex_lock(&enable_lock);

	volt = mtk_mfg_get_volt();

	for (i = 0; i < MFG_OPP_PARK_LENGTH; i++) {
		if (opp_park[i].volt <= volt) {
			clk_park = opp_park[i].clk_pll;
			break;
		}
	}

	if (clk_park) {
		clk_prepare_enable(clk_park);
		clk_prepare_enable(clk_mfg_sel);
		clk_set_parent(clk_mfg_sel, clk_park);
		clk_set_rate(clk_pll, freq);
		clk_set_parent(clk_mfg_sel, clk_pll);
		clk_disable_unprepare(clk_mfg_sel);
		clk_disable_unprepare(clk_park);
	}

	current_freq = freq;

	mutex_unlock(&enable_lock);
}
EXPORT_SYMBOL(mtk_mfg_set_freq);

bool mtk_mfg_dvfs_idle(void)
{
	return !pll_support_hopping;
}
EXPORT_SYMBOL(mtk_mfg_dvfs_idle);

int mtk_mfg_get_volt(void)
{
	if (vgpu)
		return regulator_get_voltage(vgpu);
	/* TODO: Get fixed value from vcore. */
	return 800000;
}
EXPORT_SYMBOL(mtk_mfg_get_volt);

int mtk_mfg_get_snap_volt(void)
{
	return current_volt;
}

int mtk_mfg_get_volt_sram(void)
{
	if (vgpu_sram)
		return regulator_get_voltage(vgpu_sram);

	/* If no separated vsram, return the vgpu since they are combined. */
	return mtk_mfg_get_volt();
}
EXPORT_SYMBOL(mtk_mfg_get_volt_sram);

int mtk_mfg_get_snap_volt_sram(void)
{
	return current_volt_sram;
}
EXPORT_SYMBOL(mtk_mfg_get_snap_volt);

static int mfg_get_suitable_sram_volt(int volt)
{
	int res;
	/*
	 * Signed off :
	 * v 0.7 sram 0.85
	 * v 0.8 sram 0.9
	 * v 0.9 sram 0.9
	 */

	res = volt + 100000;
	if (res < 850000)
		res = 850000;
	if (res > 900000)
		res = 900000;
	return res;
}

int mtk_mfg_set_volt(int volt)
{
	int ret = 0;

	if (vgpu) {
		int volt_sram;

		volt_sram = mfg_get_suitable_sram_volt(volt);
		if (volt > current_volt) {
			if (vgpu_sram) {
				ret = regulator_set_voltage(vgpu_sram,
					volt_sram, volt_sram);
				if (ret) {
					pr_err("Cannot set vgpu_sram for vgpu up\n");
					return ret;
				}
				current_volt_sram = volt_sram;
			}
			ret = regulator_set_voltage(vgpu, volt, volt);
			if (!ret)
				current_volt = volt;
		} else {
			ret = regulator_set_voltage(vgpu, volt, volt);
			if (!ret) {
				current_volt = volt;
				if (vgpu_sram) {
					ret = regulator_set_voltage(vgpu_sram,
						volt_sram, volt_sram);
					if (!ret)
						current_volt_sram = volt_sram;
					else
						pr_err("Cannot set vgpu_sram for vgpu down, ignore\n");
				}
			}
		}
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mfg_set_volt);

bool mtk_mfg_can_set_volt(int volt)
{
	int vcount, i, v;
	/* 0.8v is alwasy available */
	if (volt == 800000)
		return true;
	if (vgpu) {
		vcount = regulator_count_voltages(vgpu);
		for (i = 0; i < vcount; i++) {
			v = regulator_list_voltage(vgpu, i);
			if (v == volt)
				return true;
		}
	}
	return false;
}
EXPORT_SYMBOL(mtk_mfg_can_set_volt);

bool mtk_mfg_is_hwapm(void)
{
	return enable_hwapm;
}
EXPORT_SYMBOL(mtk_mfg_is_hwapm);

static int mfg_device_probe(struct platform_device *pdev)
{
	int i;

	pr_info("MFG device start probe, hwapm %d, hopping %d\n",
		enable_hwapm, pll_support_hopping);

	if (mtcmos_dev[PD_2D] == NULL) {
		pr_info("PD_2D not probe, set mfg device defer probe!\n");
		return -EPROBE_DEFER;
	}

	vgpu = devm_regulator_get_optional(&pdev->dev, "reg-vgpu");
	if (IS_ERR_OR_NULL(vgpu)) {
		if (PTR_ERR(vgpu) == -EPROBE_DEFER) {
			pr_info("regulator reg-vgpu is defer\n");
			return -EPROBE_DEFER;
		}
		vgpu = NULL;
	}

	vgpu_sram = devm_regulator_get_optional(&pdev->dev, "reg-vgpu-sram");
	if (IS_ERR_OR_NULL(vgpu_sram)) {
		if (PTR_ERR(vgpu_sram) == -EPROBE_DEFER) {
			pr_info("regulator reg-vgpu-sram is defer\n");
			if (vgpu)
				devm_regulator_put(vgpu);
			return -EPROBE_DEFER;
		}
		vgpu_sram = NULL;
	}

	clk_mfg_sel = devm_clk_get(&pdev->dev, "mfg_sel");
	if (IS_ERR(clk_mfg_sel)) {
		dev_err(&pdev->dev, "mfg_sel get failed\n");
		clk_mfg_sel = NULL;
	}

	clk_pll = devm_clk_get(&pdev->dev, "pll");
	if (IS_ERR(clk_pll)) {
		dev_err(&pdev->dev, "pll clk get failed\n");
		clk_pll = NULL;
	}

	for (i = 0; i < MFG_OPP_PARK_LENGTH; i++) {
		struct mfg_opp_park *park = &opp_park[i];
		unsigned long temp_freq;
		struct clk *clk_temp;

		clk_temp = devm_clk_get(&pdev->dev, park->name);
		if (!IS_ERR(clk_temp)) {
			temp_freq = clk_get_rate(clk_temp);
			park->clk_pll = clk_temp;
		} else {
			dev_info(&pdev->dev, "%s get failed\n", park->name);
		}
	}

	mfg_start = of_iomap(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(mfg_start)) {
		mfg_start = NULL;
		goto error_out;
	}

	mfg_dev = pdev;

	pr_info("MFG start is mapped %p\n", mfg_start);

	if (!runtime_vgpu_ctrl)
		mfg_regulator_enable();

	return 0;

error_out:
	if (clk_pll)
		devm_clk_put(&pdev->dev, clk_pll);

	if (clk_mfg_sel)
		devm_clk_put(&pdev->dev, clk_mfg_sel);

	if (vgpu_sram)
		devm_regulator_put(vgpu_sram);

	if (vgpu)
		devm_regulator_put(vgpu);

	if (mfg_start)
		iounmap(mfg_start);

	return -1;
}

static int mfg_device_remove(struct platform_device *pdev)
{
	if (clk_pll)
		devm_clk_put(&pdev->dev, clk_pll);

	if (clk_mfg_sel)
		devm_clk_put(&pdev->dev, clk_mfg_sel);

	if (vgpu && PTR_ERR(vgpu) != -EPROBE_DEFER)
		devm_regulator_put(vgpu);

	if (vgpu_sram && PTR_ERR(vgpu_sram) != -EPROBE_DEFER)
		devm_regulator_put(vgpu_sram);

	if (mfg_start)
		iounmap(mfg_start);

	if (!runtime_vgpu_ctrl)
		mfg_regulator_disable();

	mfg_dev = NULL;

	return 0;
}

static int mfg_suspend(struct device *dev)
{
	if (!runtime_vgpu_ctrl)
		mfg_regulator_disable();
	return 0;
}

static int mfg_resume(struct device *dev)
{
	if (!runtime_vgpu_ctrl)
		mfg_regulator_enable();
	return 0;
}

static const struct dev_pm_ops mfg_pm_ops = {
	.suspend = mfg_suspend,
	.resume = mfg_resume,
};

static struct platform_driver mtk_mfg_driver = {
	.probe = mfg_device_probe,
	.remove = mfg_device_remove,
	.driver = {
		   .name = "mfg",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(mfg_dt_ids),
		   .pm = &mfg_pm_ops,
		   },
};

static int __init mfg_driver_init(void)
{
	int ret;

	pr_info("init mfg driver\n");

	ret = platform_driver_register(&mtk_mfg_driver);

#if !defined(MFG_DIRECT_POWER_CTRL)
	ret = platform_driver_register(&mtk_mfg_async_driver);
	ret = platform_driver_register(&mtk_mfg_3d_driver);
	ret = platform_driver_register(&mtk_mfg_2d_driver);
#endif

	return ret;
}

static void __exit mfg_driver_exit(void)
{
	pr_info("exit mfg driver\n");

#if !defined(MFG_DIRECT_POWER_CTRL)
	platform_driver_unregister(&mtk_mfg_2d_driver);
	platform_driver_unregister(&mtk_mfg_3d_driver);
	platform_driver_unregister(&mtk_mfg_async_driver);
#endif

	platform_driver_unregister(&mtk_mfg_driver);
}

#else /* MFG_BRING_UP_ALL_ON */

static DEFINE_MUTEX(enable_lock);
static int enable_count;
static void __iomem *mfg_start;

#ifdef CONFIG_OF
static const struct of_device_id mfg_dt_ids[] = {
	{.compatible = "mediatek,mt8696-mfg"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mfg_dt_ids);
#endif

void mfg_dump_regs_seq(struct seq_file *m, const char *prefix)
{
	mutex_lock(&enable_lock);
	if (enable_count > 0) {
		if (m)
			seq_hex_dump(m, "MFG ",
				DUMP_PREFIX_OFFSET, 4, 4,
				mfg_start, 0x1000, false);
		else
			print_hex_dump(KERN_INFO, prefix,
				DUMP_PREFIX_OFFSET, 4, 4,
				mfg_start, 0x1000, false);
	} else {
		if (m)
			seq_puts(m, "MFG Power down, no regs dump.\n");
	}
	mutex_unlock(&enable_lock);
}
EXPORT_SYMBOL(mfg_dump_regs_seq);

void mtk_mfg_dump_regs(const char *prefix)
{
	mfg_dump_regs_seq(NULL, prefix);
}

void mfg_write(u32 off, u32 val)
{
	MFG_WRITE32(val, off);
}
EXPORT_SYMBOL(mfg_write);

void mtk_mfg_enable_gpu(void)
{
	mutex_lock(&enable_lock);

	enable_count++;
	/* If there is no error, this should not happen */
	if (enable_count != 1) {
		pr_err("ERR: MFG enable %d", enable_count);
		mutex_unlock(&enable_lock);
		return;
	}

	mutex_unlock(&enable_lock);
}
EXPORT_SYMBOL(mtk_mfg_enable_gpu);

void mtk_mfg_disable_gpu(void)
{
	mutex_lock(&enable_lock);

	enable_count--;
	/* This should not happen */
	if (enable_count != 0) {
		pr_err("ERR: MFG disable %d", enable_count);
		mutex_unlock(&enable_lock);
		return;
	}

	mutex_unlock(&enable_lock);
}
EXPORT_SYMBOL(mtk_mfg_disable_gpu);

bool mtk_mfg_is_ready(void)
{
	return (mfg_start != NULL);
}
EXPORT_SYMBOL(mtk_mfg_is_ready);

unsigned long mtk_mfg_get_snap_freq(void)
{
	return 650000000;
}
EXPORT_SYMBOL(mtk_mfg_get_snap_freq);

unsigned long mtk_mfg_get_freq(void)
{
	return 650000000;
}
EXPORT_SYMBOL(mtk_mfg_get_freq);

void mtk_mfg_set_freq(unsigned long freq, bool hopping)
{
}
EXPORT_SYMBOL(mtk_mfg_set_freq);

bool mtk_mfg_dvfs_idle(void)
{
	return false;
}
EXPORT_SYMBOL(mtk_mfg_dvfs_idle);

int mtk_mfg_get_volt(void)
{
	return 800000;
}
EXPORT_SYMBOL(mtk_mfg_get_volt);

int mtk_mfg_get_volt_sram(void)
{
	return 800000;
}
EXPORT_SYMBOL(mtk_mfg_get_volt_sram);

int mtk_mfg_get_snap_volt(void)
{
	return 800000;
}

int mtk_mfg_get_snap_volt_sram(void)
{
	return 800000;
}
EXPORT_SYMBOL(mtk_mfg_get_snap_volt);

int mtk_mfg_set_volt(int volt)
{
	return 0;
}
EXPORT_SYMBOL(mtk_mfg_set_volt);

bool mtk_mfg_can_set_volt(void)
{
	return false;
}
EXPORT_SYMBOL(mtk_mfg_can_set_volt);

bool mtk_mfg_is_hwapm(void)
{
	return false;
}
EXPORT_SYMBOL(mtk_mfg_is_hwapm);

static int mfg_device_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct arm_smccc_res res;

	pr_info("MFG device start probe with bringup");

	mfg_start = of_iomap(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(mfg_start)) {
		mfg_start = NULL;
		goto error_out;
	}

	mfg_dev = pdev;

	pr_info("MFG start is mapped %p\n", mfg_start);

	arm_smccc_smc(MTK_SIP_KERNEL_MFG_CONTROL,
		MFG_RGX_BUS_SECURE_UNLOCK,
		0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	if (ret) {
		pr_err("MFG smc bus unlock failed %d\n", ret);
		goto error_out;
	}

	return 0;

error_out:

	if (mfg_start)
		iounmap(mfg_start);
	return -1;
}

static int mfg_device_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mtk_mfg_driver = {
	.probe = mfg_device_probe,
	.remove = mfg_device_remove,
	.driver = {
		   .name = "mfg",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(mfg_dt_ids),
		   },
};

static int __init mfg_driver_init(void)
{
	int ret;

	pr_info("init mfg driver\n");

	ret = platform_driver_register(&mtk_mfg_driver);

	return ret;
}

static void __exit mfg_driver_exit(void)
{
	pr_info("exit mfg driver\n");

	platform_driver_unregister(&mtk_mfg_driver);
}

#endif /* MFG_BRING_UP_ALL_ON */

module_init(mfg_driver_init);
module_exit(mfg_driver_exit);
MODULE_LICENSE("GPL v2");

