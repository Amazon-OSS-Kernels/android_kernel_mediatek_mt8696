// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/devfreq.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/sched/clock.h>
#include <linux/of.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/fb.h>
#include <linux/notifier.h>

#include "governor.h"

#include <mt-plat/aee.h>
#include <mt-plat/mtk_secure_api.h>
#include <mtk_dramc.h>

#include "helio-dvfsrc.h"

static struct helio_dvfsrc *dvfsrc;
static DEFINE_MUTEX(sw_emi_mutex);
static DEFINE_MUTEX(sw_vcore_mutex);
static DEFINE_SPINLOCK(force_req_lock);
static DEFINE_SPINLOCK(spm_lock);
static int local_spm_load_firmware_status = 1;
static int boot_opp = 2;
static struct opp_profile opp_table[VCORE_DVFS_OPP_NUM];

enum vcorefs_smc_cmd {
	VCOREFS_SMC_CMD_0,
	VCOREFS_SMC_CMD_1,
	VCOREFS_SMC_CMD_2,
	VCOREFS_SMC_CMD_3,
	NUM_VCOREFS_SMC_CMD,
};

static void dvfsrc_opp_level_mapping(void)
{
	int i;

	for (i = 0; i < VCORE_DVFS_OPP_NUM; i++) {
		opp_table[i].ddr_Mbps = dram_steps_freq(i);
		opp_table[i].vcore_uv = 800000;
	}
	if (dvfsrc->type)
		opp_table[VCORE_DVFS_OPP_NUM - 1].vcore_uv = 900000;
}

static u32 spm_read(u32 offset)
{
	unsigned int val;

	regmap_read(dvfsrc->spm_map, offset, &val);

	return val;
}

static u32 spm_write(u32 offset, u32 val)
{
	return regmap_write(dvfsrc->spm_map, offset, val);
}

#define spm_rmw(offset, val, mask, shift) \
		spm_write(offset, (spm_read(offset) & ~(mask << shift)) \
				| (val << shift))

static void spm_dvfs_init(u32 enable)
{
	unsigned long flags;

	spin_lock_irqsave(&spm_lock, flags);
	mt_secure_call(MTK_SIP_KERNEL_SPM_VCOREFS_ARGS,
			VCOREFS_SMC_CMD_0, enable, boot_opp, 0);
	spin_unlock_irqrestore(&spm_lock, flags);
}

static void dvfsrc_set_sw_req(u32 id, u32 data)
{
	spm_rmw(SPM_DVFS_MISC, 0,
		0x1, SPM_DVFSRC_ENABLE_SHIFT);
	switch (id) {
	case 0:
		spm_write(RC_SW_LEVEL0, data);
		break;
	case 1:
		spm_write(RC_SW_LEVEL1, data);
		break;
	case 2:
		spm_write(RC_SW_LEVEL2, data);
		break;
	default:
		break;
	}
	/*spm_rmw(SPM_DVFS_MISC, 0, 0x1, SPM_DVFS_FORCE_ENABLE_SHIFT); */
	spm_rmw(SPM_DVFS_MISC, 1,
		0x1, SPM_DVFSRC_ENABLE_SHIFT);
}

static void dvfsrc_set_force(int enable, u32 data)
{
	u32 level;

	if (dvfsrc->type && data == 2)
		level = 0x10002;
	else
		level = data;

	if (enable) {
		spm_rmw(SPM_DVFS_MISC, 0,
			0x1, SPM_DVFSRC_ENABLE_SHIFT);
		spm_write(SPM_FORCE_DVFS, data);
		spm_rmw(SPM_DVFS_MISC, 1,
				0x1, SPM_DVFS_FORCE_ENABLE_SHIFT);
		spm_rmw(SPM_DVFS_MISC, 1,
			0x1, SPM_DVFSRC_ENABLE_SHIFT);
	} else {
		spm_rmw(SPM_DVFS_MISC, 0,
			0x1, SPM_DVFSRC_ENABLE_SHIFT);
		spm_write(SPM_FORCE_DVFS, 0x0);
		spm_write(RC_SW_LEVEL2, 0x1);
		spm_rmw(SPM_DVFS_MISC, 0,
			0x1, SPM_DVFS_FORCE_ENABLE_SHIFT);
		spm_rmw(SPM_DVFS_MISC, 1,
			0x1, SPM_DVFSRC_ENABLE_SHIFT);
	}
}

static void spm_go_to_vcorefs(u32 dvfs_type)
{
	unsigned long flags;

	spin_lock_irqsave(&spm_lock, flags);
	mt_secure_call(MTK_SIP_KERNEL_SPM_VCOREFS_ARGS,
			VCOREFS_SMC_CMD_3, 0, 0, 0);

	spin_unlock_irqrestore(&spm_lock, flags);
}

static u32 spm_get_dvfs_level(void)
{
	return spm_read(SPM_DVFS_LEVEL);
}

static u32 get_cur_vcore_level(void)
{
	return (spm_get_dvfs_level() >> 16) & 0x3;
}

static u32 get_cur_ddr_level(void)
{
	return spm_get_dvfs_level() & 0x3;
}

static u32 get_target_level(void)
{
	return (spm_read(RC_INFO) & RC_REQ_LSB) >> RC_REQ_SHIFT;
}

static int is_opp_forced(void)
{
	return (spm_read(SPM_DVFS_MISC) >> SPM_DVFS_FORCE_ENABLE_SHIFT) & 0x1;
}

static int commit_data(int type, int data)
{
	int ret = 0;
	int level = 16;
	unsigned long flags;

	if (!is_dvfsrc_enabled()) {
		dev_info(dvfsrc->dev,
			"DVFSRC not ready. type: 0x%x, data: 0x%x\n",
			type, data);
		dvfsrc_dump_reg(NULL);
		return ret;
	}

	switch (type) {
	case PM_QOS_EMI_OPP:
		mutex_lock(&sw_emi_mutex);
		if (data >= DDR_OPP_NUM || data < 0)
			data = DDR_OPP_NUM - 1;
		level = DDR_OPP_NUM - data - 1;
		dvfsrc_set_sw_req(0, level & 0x3);
		if (!is_opp_forced()) {
			ret = dvfsrc_wait_for_completion(
					get_cur_ddr_level() >= level,
					DVFSRC_TIMEOUT);
		}
		mutex_unlock(&sw_emi_mutex);
		break;
	case PM_QOS_VCORE_OPP:
		mutex_lock(&sw_vcore_mutex);
		if (data >= VCORE_OPP_NUM || data < 0)
			data = VCORE_OPP_NUM - 1;
		level = VCORE_OPP_NUM - data - 1;

		dvfsrc_set_sw_req(1, (level << 16) & 0x30000);
		if (!is_opp_forced()) {
			ret = dvfsrc_wait_for_completion(
					(get_target_level() == 0),
					DVFSRC_TIMEOUT);
			ret = dvfsrc_wait_for_completion(
					get_cur_vcore_level() >= level,
					DVFSRC_TIMEOUT);
		}
		mutex_unlock(&sw_vcore_mutex);
		break;
	case PM_QOS_VCORE_DVFS_FIXED_OPP:
		spin_lock_irqsave(&force_req_lock, flags);
		if (data == VCORE_DVFS_OPP_UNREQ)
			dvfsrc_set_force(0, 0);
		if (data >= VCORE_DVFS_OPP_NUM || data < 0)
			data = VCORE_DVFS_OPP_NUM - 1;
		level = VCORE_DVFS_OPP_NUM - data - 1;

		if (level == 0) {
			dev_info(dvfsrc->dev,
				"Invalid data: 0x%x, invalid level: %d\n",
				data, level);
			spin_unlock_irqrestore(&force_req_lock, flags);
			break;
		}
		dvfsrc_set_force(1, level);
		ret = dvfsrc_wait_for_completion(
				spm_get_dvfs_level() == level,
				DVFSRC_TIMEOUT);
		spin_unlock_irqrestore(&force_req_lock, flags);
		break;
	default:
		break;
	}

	if (ret < 0) {
		dev_err(dvfsrc->dev,
			"type: 0x%x, data: 0x%x, level: %d\n", type,
			data, level);
		dvfsrc_dump_reg(NULL);
		aee_kernel_warning("DVFSRC", "%s: failed.", __func__);
	}

	return ret;
}

static void dvfsrc_restore(void)
{
	int i;

	for (i = PM_QOS_EMI_OPP; i < PM_QOS_VCORE_DVFS_FIXED_OPP; i++)
		commit_data(i, pm_qos_request(i));
}

static int spm_load_firmware_status(void)
{
	if (local_spm_load_firmware_status == 1)
		local_spm_load_firmware_status =
		mt_secure_call(MTK_SIP_KERNEL_SPM_FIRMWARE_STATUS, 0, 0, 0, 0);
	/* -1 not init, 0: not loaded, 1: loaded, 2: loaded and kicked */
	if (!local_spm_load_firmware_status)
		dev_err(dvfsrc->dev, "SPM FIRMWARE IS NOT READY\n");
	return local_spm_load_firmware_status;
}

void helio_dvfsrc_enable(int dvfsrc_en)
{
	if (dvfsrc_en > 1 || dvfsrc_en < 0)
		return;

	if (!spm_load_firmware_status())
		return;

	spm_dvfs_init(dvfsrc_en);
	udelay(100);
	dvfsrc_restore();
}

int is_dvfsrc_enabled(void)
{
	return (spm_read(SPM_DVFS_MISC) >> SPM_DVFSRC_ENABLE_SHIFT) & 0x1;
}

static void get_opp_info(char *p)
{
	p += sprintf(p, "%-24s: %-8u Mbps\n", "DDR",
		get_dram_data_rate());
	p += sprintf(p, "%-24s: %-8u\n", "DDR_TYPE",
		get_ddr_type());
	p += sprintf(p, "%-24s: %-8u\n", "hw_level",
		spm_get_dvfs_level());
}

static void get_dvfsrc_reg(char *p)
{
	p += sprintf(p, "%-24s: 0x%08x\n",
			"SPM_DVFS_LEVEL",
			spm_read(SPM_DVFS_LEVEL));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"SPM_DVS_DFS_LEVEL",
			spm_read(SPM_DVS_DFS_LEVEL));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"SPM_FORCE_DVFS",
			spm_read(SPM_FORCE_DVFS));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"RC_INFO",
			spm_read(RC_INFO));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"SPM_DVFS_MISC",
			spm_read(SPM_DVFS_MISC));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"RC_TARGET_LEVEL",
			spm_read(RC_TARGET_LEVEL));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x\n",
			"RC_RECORD_DVS(DFS)",
			spm_read(RC_RECORD_DVS),
			spm_read(RC_RECORD_DFS));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x\n",
			"RC_SW_LEVEL0(1)(2)",
			spm_read(RC_SW_LEVEL0),
			spm_read(RC_SW_LEVEL1),
			spm_read(RC_SW_LEVEL2));
}

static void get_pm_qos_info(char *p)
{
	p += sprintf(p, "%-24s: 0x%x\n",
			"PM_QOS_EMI_OPP",
			pm_qos_request(PM_QOS_EMI_OPP));
	p += sprintf(p, "%-24s: 0x%x\n",
			"PM_QOS_VCORE_OPP",
			pm_qos_request(PM_QOS_VCORE_OPP));
	p += sprintf(p, "%-24s: 0x%x\n",
			"PM_QOS_FORCE_OPP",
			pm_qos_request(PM_QOS_VCORE_DVFS_FIXED_OPP));
}

char *dvfsrc_dump_reg(char *ptr)
{
	char buf[1024];

	memset(buf, '\0', sizeof(buf));
	get_opp_info(buf);
	if (ptr)
		ptr += sprintf(ptr, "%s\n", buf);
	else
		pr_info("%s\n", buf);

	memset(buf, '\0', sizeof(buf));
	get_dvfsrc_reg(buf);
	if (ptr)
		ptr += sprintf(ptr, "%s\n", buf);
	else
		pr_info("%s\n", buf);

	memset(buf, '\0', sizeof(buf));
	get_pm_qos_info(buf);
	if (ptr)
		ptr += sprintf(ptr, "%s\n", buf);
	else
		pr_info("%s\n", buf);

	return ptr;
}

char *dvfsrc_dump_opp_table(char *ptr)
{
	int i;
	char *buff_end = ptr + PAGE_SIZE;

	for (i = 0; i < VCORE_DVFS_OPP_NUM; i++) {
		ptr += snprintf(ptr, buff_end - ptr,
			"[OPP%-2d]: %-8u uv %-8u Mbps\n",
			i, opp_table[i].vcore_uv, opp_table[i].ddr_Mbps);
	}
	return ptr;
}
static struct devfreq_dev_profile helio_devfreq_profile = {
	.polling_ms	= 0,
};

static int helio_governor_event_handler(struct devfreq *devfreq,
					unsigned int event, void *data)
{
	switch (event) {
	case DEVFREQ_GOV_SUSPEND:
		break;

	case DEVFREQ_GOV_RESUME:
		break;

	default:
		break;
	}
	return 0;
}

static struct devfreq_governor helio_dvfsrc_governor = {
	.name = "helio_dvfsrc",
	.event_handler = helio_governor_event_handler,
};

static int pm_qos_ddr_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	commit_data(PM_QOS_EMI_OPP, l);

	return NOTIFY_OK;
}

static int pm_qos_vcore_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	commit_data(PM_QOS_VCORE_OPP, l);

	return NOTIFY_OK;
}

static int pm_qos_vcore_dvfs_force_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	commit_data(PM_QOS_VCORE_DVFS_FIXED_OPP, l);

	return NOTIFY_OK;
}

static void pm_qos_notifier_register(void)
{
	dvfsrc->pm_qos_ddr_opp_nb.notifier_call =
		pm_qos_ddr_opp_notify;
	dvfsrc->pm_qos_vcore_opp_nb.notifier_call =
		pm_qos_vcore_opp_notify;
	dvfsrc->pm_qos_vcore_dvfs_force_opp_nb.notifier_call =
		pm_qos_vcore_dvfs_force_opp_notify;

	pm_qos_add_notifier(PM_QOS_EMI_OPP,
			&dvfsrc->pm_qos_ddr_opp_nb);
	pm_qos_add_notifier(PM_QOS_VCORE_OPP,
			&dvfsrc->pm_qos_vcore_opp_nb);
	pm_qos_add_notifier(PM_QOS_VCORE_DVFS_FIXED_OPP,
			&dvfsrc->pm_qos_vcore_dvfs_force_opp_nb);
}

static int dvfsrc_fb_notifier_call(struct notifier_block *self,
		unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;

	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
	case FB_BLANK_UNBLANK:
		break;
	case FB_BLANK_POWERDOWN:
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block dvfsrc_fb_notifier = {
	.notifier_call = dvfsrc_fb_notifier_call,
};

static int helio_dvfsrc_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i;
	struct device_node *np = pdev->dev.of_node;
	int ddr_type = get_ddr_type();
	int ddr_hz = get_dram_data_rate();

	dvfsrc = devm_kzalloc(&pdev->dev, sizeof(*dvfsrc), GFP_KERNEL);
	if (!dvfsrc)
		return -ENOMEM;

	dvfsrc->dev = &pdev->dev;


	dev_info(dvfsrc->dev, "#@# %s: ddr=[%d][%d]\n",
		__func__, ddr_type, ddr_hz);
	if (ddr_type < TYPE_LPDDR4) {
		dev_info(dvfsrc->dev, "DDR type not support DFS\n");
		return -ENODEV;
	}

	for (i = 0; i < 3; i++)
		if (ddr_hz == dram_steps_freq(i))
			boot_opp = i;

	dvfsrc->spm_map =
		syscon_regmap_lookup_by_compatible("mediatek,mt8696-scpsys");
	if (IS_ERR(dvfsrc->spm_map)) {
		dev_err(dvfsrc->dev, "no syscon\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "type",
	   (u32 *) &dvfsrc->type))
		dvfsrc->type = 0;
	platform_set_drvdata(pdev, dvfsrc);

	dvfsrc->devfreq = devm_devfreq_add_device(&pdev->dev,
						 &helio_devfreq_profile,
						 "helio_dvfsrc",
						 NULL);

	ret = helio_dvfsrc_add_interface(&pdev->dev);
	if (ret)
		return ret;

	dvfsrc_opp_level_mapping();
	pm_qos_notifier_register();
	spm_go_to_vcorefs(dvfsrc->type);
	helio_dvfsrc_enable(1);
	fb_register_client(&dvfsrc_fb_notifier);
	dev_info(dvfsrc->dev, "init done\n");

	return 0;
}

static int helio_dvfsrc_remove(struct platform_device *pdev)
{
	helio_dvfsrc_remove_interface(&pdev->dev);
	return 0;
}

static const struct of_device_id helio_dvfsrc_of_match[] = {
	{ .compatible = "mediatek,dvfsrc" },
	{ .compatible = "mediatek,dvfsrc-mt8696" },
	{ },
};

MODULE_DEVICE_TABLE(of, helio_dvfsrc_of_match);

static __maybe_unused int helio_dvfsrc_suspend(struct device *dev)
{
	int ret = 0;

	ret = devfreq_suspend_device(dvfsrc->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to suspend the devfreq devices\n");
		return ret;
	}

	return 0;
}

static __maybe_unused int helio_dvfsrc_resume(struct device *dev)
{
	int ret = 0;

	ret = devfreq_resume_device(dvfsrc->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to resume the devfreq devices\n");
		return ret;
	}
	return ret;
}

static SIMPLE_DEV_PM_OPS(helio_dvfsrc_pm, helio_dvfsrc_suspend,
			 helio_dvfsrc_resume);

static struct platform_driver helio_dvfsrc_driver = {
	.probe	= helio_dvfsrc_probe,
	.remove	= helio_dvfsrc_remove,
	.driver = {
		.name = "helio-dvfsrc",
		.pm	= &helio_dvfsrc_pm,
		.of_match_table = helio_dvfsrc_of_match,
	},
};

static int __init helio_dvfsrc_init(void)
{
	int ret = 0;

	ret = devfreq_add_governor(&helio_dvfsrc_governor);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&helio_dvfsrc_driver);
	if (ret)
		devfreq_remove_governor(&helio_dvfsrc_governor);

	return ret;
}
module_init(helio_dvfsrc_init)

static void __exit helio_dvfsrc_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&helio_dvfsrc_driver);

	ret = devfreq_remove_governor(&helio_dvfsrc_governor);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);
}
module_exit(helio_dvfsrc_exit)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Helio dvfsrc driver");
