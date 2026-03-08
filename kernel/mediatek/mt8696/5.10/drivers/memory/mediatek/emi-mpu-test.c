// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/arm-smccc.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <soc/mediatek/emi.h>

#define MTK_SIP_E_NOT_SUPPORTED     0xFFFFFFFE

struct emi_mpu_test {
	unsigned long long dram_base;
	unsigned int region_cnt;
	unsigned int domain_cnt;
	unsigned int addr_align;
	unsigned int show_region;
	struct device_driver *driver;
};

static struct emi_mpu_test *global_mpu_test;

static unsigned int emimpu_read_protection(
	unsigned int reg_type, unsigned int region, unsigned int dgroup)
{
	struct arm_smccc_res smc_res;

	arm_smccc_smc(MTK_SIP_EMIMPU_CONTROL, MTK_EMIMPU_READ,
		reg_type, region, dgroup, 0, 0, 0, &smc_res);

	return (unsigned int)smc_res.a0;
}

/*
 * mtk_emimpu_set_protection - set emimpu protect into device
 * @rg_info:	the target region information
 *
 * Returns 0 on success, -EINVAL if rg_info is invalid,
 * -ENODEV if the emi-mpu driver is not probed successfully,
 * -EPERM if the SMC call returned failure
 */
static int mtk_emimpu_set_protection(struct emimpu_region_t *rg_info)
{
	struct emi_mpu_test *mpu;
	unsigned int start, end;
	unsigned int group_apc;
	unsigned int d_group;
	struct arm_smccc_res smc_res;
	int i, j;

	if (!rg_info)
		return -EINVAL;

	if (!(rg_info->apc)) {
		pr_info("%s: fail, protect without init\n", __func__);
		return -EINVAL;
	}

	mpu = global_mpu_test;
	if (!mpu)
		return -ENODEV;

	d_group = mpu->domain_cnt / 8;

	start = (unsigned int)
		(rg_info->start >> (mpu->addr_align)) |
		(rg_info->rg_num << 24);

	for (i = d_group - 1; i >= 0; i--) {
		end = (unsigned int)
			(rg_info->end >> (mpu->addr_align)) |
			(i << 24);

		for (group_apc = 0, j = 0; j < 8; j++)
			group_apc |=
				((rg_info->apc[i * 8 + j]) & 0xF) << (4 * j);
		if ((i == 0) && rg_info->lock)
			end |= (0x1 << 31);

		arm_smccc_smc(MTK_SIP_EMIMPU_CONTROL, MTK_EMIMPU_SET,
			start, end, group_apc, 0, 0, 0, &smc_res);
		if (smc_res.a0) {
			pr_info("%s:%d failed to set region permission, ret=0x%lx\n",
				__func__, __LINE__, smc_res.a0);
			return -EPERM;
		}

	}

	return 0;
}

/*
 * mtk_emimpu_clear_protection - clear emimpu protection
 * @rg_info:	the target region information
 *
 * Returns 0 on success, -EINVAL if rg_info is invalid,
 * -ENODEV if the emi-mpu driver is not probed successfully,
 * -EPERM if the SMC call returned failure
 */
static int mtk_emimpu_clear_protection(struct emimpu_region_t *rg_info)
{
	struct emi_mpu_test *mpu;
	struct arm_smccc_res smc_res;

	if (!rg_info)
		return -EINVAL;

	mpu = global_mpu_test;
	if (!mpu)
		return -ENODEV;

	if (rg_info->rg_num > mpu->region_cnt) {
		pr_info("%s: region %u overflow\n", __func__, rg_info->rg_num);
		return -EINVAL;
	}

	arm_smccc_smc(MTK_SIP_EMIMPU_CONTROL, MTK_EMIMPU_CLEAR,
		rg_info->rg_num, 0, 0, 0, 0, 0, &smc_res);
	if (smc_res.a0) {
		pr_info("%s:%d failed to clear region permission, ret=0x%lx\n",
			__func__, __LINE__, smc_res.a0);
		return -EPERM;
	}
	return 0;
}

static ssize_t emimpu_ctrl_show(struct device_driver *driver, char *buf)
{
	struct emi_mpu_test *mpu_test;
	ssize_t ret;
	unsigned int i;
	unsigned int region;
	unsigned int apc;
	unsigned long long start, end;
	unsigned int no_use_region_flag = 0;
	static const char *permission[16] = {
		"NO_PROTECTIO",
		"NSEC_W_FORBIDDEN",
		"NSEC_R_FORBIDDEN",
		"NSEC_RW_FORBIDDEN",
		"SEC_W_FORBIDDEN",
		"SEC_W_NSEC_W_FORBIDDEN",
		"SEC_W_NSEC_R_FORBIDDEN",
		"SEC_W_NSEC_RW_FORBIDDEN",
		"SEC_R_FORBIDDEN",
		"SEC_R_NSEC_W_FORBIDDEN",
		"SEC_R_NSEC_R_FORBIDDEN",
		"SEC_R_NSEC_RW_FORBIDDEN",
		"SEC_RW_FORBIDDEN",
		"SEC_RW_NSEC_W_FORBIDDEN",
		"SEC_RW_NSEC_R_FORBIDDEN",
		"ALL_FORBIDDEN"
	};

	mpu_test = global_mpu_test;
	if (!mpu_test)
		return -ENXIO;

	for (ret = 0, region = mpu_test->show_region;
		region < mpu_test->region_cnt; region++) {
		no_use_region_flag = 0;
		start = (unsigned long long)emimpu_read_protection(
			MTK_EMIMPU_READ_SA, region, 0);

		if (start == MTK_SIP_E_NOT_SUPPORTED)
			return -EPERM;

		start = (start << (mpu_test->addr_align)) +
			mpu_test->dram_base;

		end = (unsigned long long)emimpu_read_protection(
			MTK_EMIMPU_READ_EA, region, 0);

		if (end == MTK_SIP_E_NOT_SUPPORTED)
			return -EPERM;

		end = (end << (mpu_test->addr_align)) +
			mpu_test->dram_base;

		if (start > end) {
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
			    "R%u-> no use, disable\n", region);
			no_use_region_flag = 1;
		} else {
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
			    "R%u-> 0x%llx to 0x%llx\n",
			    region, start, end + 0xFFFF);
		}
		if (ret >= PAGE_SIZE)
			return strlen(buf);

		if (no_use_region_flag) {
			ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
			if (ret >= PAGE_SIZE)
				return strlen(buf);

			continue;
		}

		for (i = 0; i < (mpu_test->domain_cnt / 8); i++) {
			apc = emimpu_read_protection(
				MTK_EMIMPU_READ_APC, region, i);
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"%s, %s, %s, %s\n%s, %s, %s, %s\n\n",
				permission[(apc >> 0) & 0xF],
				permission[(apc >> 4) & 0xF],
				permission[(apc >> 8) & 0xF],
				permission[(apc >> 12) & 0xF],
				permission[(apc >> 16) & 0xF],
				permission[(apc >> 20) & 0xF],
				permission[(apc >> 24) & 0xF],
				permission[(apc >> 28) & 0xF]);
			if (ret >= PAGE_SIZE)
				return strlen(buf);
		}
	}

	return strlen(buf);
}

static ssize_t emimpu_ctrl_store
	(struct device_driver *driver, const char *buf, size_t count)
{
	struct emi_mpu_test *mpu_test;
	char *command, *backup_command;
	char *ptr, *token[MTK_EMI_MAX_TOKEN];
	static struct emimpu_region_t *rg_info;
	unsigned long long start = 0, end = 0;
	unsigned long region;
	unsigned long dgroup;
	unsigned long apc;
	int i, j, ret;

	mpu_test = global_mpu_test;
	if (!mpu_test)
		return -EFAULT;

	if (!rg_info) {
		rg_info = kmalloc(sizeof(struct emimpu_region_t), GFP_KERNEL);
		if (!rg_info)
			return -ENOMEM;
		rg_info->apc = kmalloc_array(
			mpu_test->domain_cnt, sizeof(unsigned int),
			GFP_KERNEL);
		if (!(rg_info->apc)) {
			kfree(rg_info);
			rg_info = NULL;
			return -ENOMEM;
		}
		rg_info->lock = false;
	}

	if ((strlen(buf) + 1) > MTK_EMI_MAX_CMD_LEN) {
		pr_info("%s: store command overflow\n", __func__);
		return -EINVAL;
	}

	pr_info("%s: store: %s\n", __func__, buf);

	command = kmalloc((size_t)MTK_EMI_MAX_CMD_LEN, GFP_KERNEL);
	if (!command)
		return -ENOMEM;
	backup_command = command;
	strncpy(command, buf, (size_t)MTK_EMI_MAX_CMD_LEN);

	for (i = 0; i < MTK_EMI_MAX_TOKEN; i++) {
		ptr = strsep(&command, " ");
		if (!ptr)
			break;
		token[i] = ptr;
	}

	if (!strncmp(buf, "SHOW", strlen("SHOW"))) {
		if (i < 2)
			goto emimpu_ctrl_store_end;

		pr_info("%s: %s %s\n", __func__, token[0], token[1]);

		ret = kstrtoul(token[1], 10, &region);
		if (ret != 0)
			pr_info("%s: fail to parse region\n", __func__);

		if (region < mpu_test->region_cnt) {
			mpu_test->show_region = (unsigned int) region;
			pr_info("%s: show_region to %u\n",
				__func__, mpu_test->show_region);
		}
	} else if (!strncmp(buf, "SET", strlen("SET"))) {
		if (i < 3)
			goto emimpu_ctrl_store_end;

		pr_info("%s: %s %s %s\n",
			__func__, token[0], token[1], token[2]);

		ret = kstrtoul(token[1], 10, &dgroup);
		if (ret != 0)
			pr_info("%s: fail to parse dgroup\n", __func__);
		ret = kstrtoul(token[2], 16, &apc);
		if (ret != 0)
			pr_info("[MPU] fail to parse apc\n");

		if (dgroup < (mpu_test->domain_cnt / 8)) {
			pr_info("%s: apc[%lu]: 0x%lx\n",
				__func__, dgroup, apc);

			for (j = 0; j < 8; j++)
				rg_info->apc[dgroup * 8 + j] =
					((unsigned int)apc >> (4 * j)) & 0xF;
		}
		if (dgroup == 0)
			rg_info->lock = apc & 0x80000000;
	} else if (!strncmp(buf, "ON", strlen("ON"))) {
		if (i < 4)
			goto emimpu_ctrl_store_end;

		pr_info("%s: %s %s %s %s\n",
			__func__, token[0], token[1], token[2], token[3]);

		ret = kstrtoull(token[1], 16, &start);
		if (ret != 0)
			pr_info("%s: fail to parse start\n", __func__);
		ret = kstrtoull(token[2], 16, &end);
		if (ret != 0)
			pr_info("%s: fail to parse end\n", __func__);
		ret = kstrtoul(token[3], 10, &region);
		if (ret != 0)
			pr_info("%s: fail to parse region\n", __func__);

		if (region < mpu_test->region_cnt) {
			rg_info->start = start;
			rg_info->end = end;
			rg_info->rg_num = (unsigned int)region;
			mtk_emimpu_set_protection(rg_info);
		}
	} else if (!strncmp(buf, "OFF", strlen("OFF"))) {
		if (i < 2)
			goto emimpu_ctrl_store_end;

		pr_info("%s: %s %s\n", __func__, token[0], token[1]);

		ret = kstrtoul(token[1], 10, &region);
		if (ret != 0)
			pr_info("%s: fail to parse region\n", __func__);

		if (region < mpu_test->region_cnt) {
			rg_info->rg_num = (unsigned int)region;
			mtk_emimpu_clear_protection(rg_info);
		}
	} else
		pr_info("%s: unknown store command\n", __func__);

emimpu_ctrl_store_end:
	kfree(backup_command);

	return count;
}

static DRIVER_ATTR_RW(emimpu_ctrl);

static __init int emimputest_init(void)
{
	struct device_node *node;
	struct platform_device *pdev;
	struct emi_mpu_test *mpu_test;
	int ret;

	pr_info("emimputest was loaded\n");

	node = of_find_compatible_node(NULL, NULL, "mediatek,common-emimpu");
	if (!node) {
		pr_info("emimputest: cannot find emimpu node\n");
		return -ENXIO;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev) {
		pr_info("emimputest: cannot find emimpu pdev\n");
		return -ENXIO;
	}
	if (!pdev->dev.driver) {
		pr_info("emimputest: cannot find emimpu driver\n");
		return -ENXIO;
	}

	mpu_test = kzalloc(sizeof(struct emi_mpu_test), GFP_KERNEL);
	if (!mpu_test)
		return -ENOMEM;

	ret = of_property_read_u32(node,
		"region_cnt", &(mpu_test->region_cnt));
	if (ret) {
		pr_info("emimputest: no region_cnt\n");
		ret = -ENXIO;
		goto free_emi_mpu_test;
	}

	ret = of_property_read_u32(node,
		"domain_cnt", &(mpu_test->domain_cnt));
	if (ret) {
		pr_info("emimputest: no domain_cnt\n");
		ret = -ENXIO;
		goto free_emi_mpu_test;
	}

	ret = of_property_read_u32(node,
		"addr_align", &(mpu_test->addr_align));
	if (ret) {
		pr_info("emimputest: no addr_align\n");
		ret = -ENXIO;
		goto free_emi_mpu_test;
	}

	ret = of_property_read_u64(node,
		"dram_base", &(mpu_test->dram_base));
	if (ret) {
		pr_info("emimputest: no dram_base\n");
		ret = -ENXIO;
		goto free_emi_mpu_test;
	}

	ret = driver_create_file(pdev->dev.driver,
				 &driver_attr_emimpu_ctrl);
	if (ret) {
		pr_info("emimputest: failed to create file\n");
		goto free_emi_mpu_test;
	}
	mpu_test->driver = pdev->dev.driver;

	global_mpu_test = mpu_test;

	return 0;

free_emi_mpu_test:
	kfree(mpu_test);

	return ret;
}

static __exit void emimputest_exit(void)
{
	struct emi_mpu_test *mpu_test = global_mpu_test;

	pr_info("emimputest was unloaded\n");

	driver_remove_file(mpu_test->driver,
			&driver_attr_emimpu_ctrl);

	global_mpu_test = NULL;

	kfree(mpu_test);
}

module_init(emimputest_init);
module_exit(emimputest_exit);

MODULE_DESCRIPTION("MediaTek EMI MPU Driver");
MODULE_LICENSE("GPL v2");
