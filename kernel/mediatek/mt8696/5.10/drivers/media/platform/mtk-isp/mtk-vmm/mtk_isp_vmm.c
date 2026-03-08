// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include "mtk_isp_vmm.h"

static struct isp_vmm_device *isp_vmm_dev;

int isp_direct_set_voltage(struct regulator *regulator,
		int min_uV, int max_uV, unsigned int isp_type)
{
	int ret = 0;
	int target_voltage = 0;
	int i;

	if (!isp_vmm_dev->dev) {
		pr_info("%s, device not exist.\n", __func__);
		return -ENODEV;
	}
	mutex_lock(&isp_vmm_dev->isp_vmm_mutex);

	isp_vmm_dev->isp_engine[isp_type] = min_uV;

	for (i = 0; i < VMM_ISP_MAX; i++) {
		if (target_voltage < isp_vmm_dev->isp_engine[i])
			target_voltage = isp_vmm_dev->isp_engine[i];
	}
	ret = regulator_set_voltage(regulator, target_voltage, MAX_VMM_VOLTAGE);

	mutex_unlock(&isp_vmm_dev->isp_vmm_mutex);

	return ret;
}

static int mtk_isp_vmm_probe(struct platform_device *pdev)
{

	pr_info("%s+\n", __func__);

	isp_vmm_dev = devm_kzalloc(&pdev->dev, sizeof(struct isp_vmm_device) * 1, GFP_KERNEL);
	if (!isp_vmm_dev)
		return -ENODEV;

	isp_vmm_dev->dev = &pdev->dev;

	mutex_init(&isp_vmm_dev->isp_vmm_mutex);

	pr_info("%s-\n", __func__);
	return 0;
}

static int mtk_isp_vmm_remove(struct platform_device *pdev)
{
	mutex_destroy(&isp_vmm_dev->isp_vmm_mutex);
	devm_kfree(&pdev->dev, isp_vmm_dev);
	return 0;
}

static const struct of_device_id mtk_isp_vmm_match[] = {
	{ .compatible = "mediatek,isp_vmm", },
	{}
};
MODULE_DEVICE_TABLE(of, mtk_isp_vmm_match);

static struct platform_driver mtk_isp_vmm_driver = {
	.probe   = mtk_isp_vmm_probe,
	.remove  = mtk_isp_vmm_remove,
	.driver  = {
		.name = "mtk-isp-vmm",
		.owner	= THIS_MODULE,
		.of_match_table = mtk_isp_vmm_match,
	}
};
module_platform_driver(mtk_isp_vmm_driver);

MODULE_AUTHOR("Vito Tu <vito.tu@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mediatek ISP VMM driver");
