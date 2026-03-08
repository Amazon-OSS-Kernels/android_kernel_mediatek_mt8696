// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

//#include <drm/drmP.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include "mtk_log.h"


static int mtk_disp_power_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_enable(dev);

	DDPINFO("%s-\n", __func__);
	return 0;
}

static int mtk_disp_power_dev_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static const struct of_device_id disp_power_driver_dt_match[] = {
	{.compatible = "mediatek,mt8195-vdosys_client"},
	{.compatible = "mediatek,mt8188-vdosys_client"},
	{},
};

MODULE_DEVICE_TABLE(of, disp_power_driver_dt_match);

struct platform_driver mtk_disp_power_dev_driver = {
	.probe = mtk_disp_power_dev_probe,
	.remove = mtk_disp_power_dev_remove,
	.driver = {
		.name = "mediatek-disp-power",
		.owner = THIS_MODULE,
		.of_match_table = disp_power_driver_dt_match,
	},
};
