// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Tiffany Lin <tiffany.lin@mediatek.com>
 *
 */

#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of.h>

//#include <soc/mediatek/smi.h>

#include "mtk_vcodec_enc_pm.h"
#include "mtk_vcodec_util.h"
#include "mtk_vcu.h"

#ifdef VENC_ENABLE_MMSRAM
#include "mmsram.h"
#endif
#ifdef CONFIG_MTK_PSEUDO_M4U
#include <mach/mt_iommu.h>
#include "mach/pseudo_m4u.h"
#include "smi_port.h"
#endif

#define USE_GCE 1
#define CORE_NUM 2

struct venc_larb_data {
	unsigned int    larb_id;
	struct device   *dev;
};

static struct ion_client *ion_venc_client;
static struct platform_device *pm_dev[MTK_VENC_HW_NUM] = {NULL};

void mtk_venc_init_ctx_pm(struct mtk_vcodec_ctx *ctx)
{
	ctx->async_mode = 0;
#ifdef VENC_ENABLE_MMSRAM
	ctx->sram_data.size = 0;

	if (set_mmsram_data_if(&ctx->sram_data) >= 0)
		ctx->use_slbc = 1;
	else
		ctx->use_slbc = 0;

	mtk_v4l2_err("set_mmsram_data ,use_slbc=%d  %p %p %d\n",
		ctx->use_slbc, ctx->sram_data.paddr, ctx->sram_data.vaddr, ctx->use_slbc);
#endif
}

int mtk_vcodec_init_enc_pm(struct mtk_vcodec_dev *mtkdev)
{
	int ret = 0;
#ifndef FPGA_PWRCLK_VENC_API_DISABLE
	struct platform_device *pdev;
	struct device *dev;
	struct mtk_vcodec_pm *pm;

	pdev = mtkdev->plat_dev;
	pm = &mtkdev->pm;
	memset(pm, 0, sizeof(struct mtk_vcodec_pm));
	pm->mtkdev = mtkdev;
	pm->dev = &pdev->dev;
	dev = &pdev->dev;

	pm->chip_node = of_find_compatible_node(NULL,
		NULL, "mediatek,mt8195-vcodec-enc");

	pdev = mtkdev->plat_dev;
	pm->dev = &pdev->dev;

	pm->clk_MT_CG_VENC0 = devm_clk_get(&pdev->dev, "MT_CG_VENC0");
	if (IS_ERR(pm->clk_MT_CG_VENC0)) {
		mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_VENC0\n");
		ret = PTR_ERR(pm->clk_MT_CG_VENC0);
	}

	pm->clk_MT_CG_VENC1 = devm_clk_get(&pdev->dev, "MT_CG_VENC1");
	if (IS_ERR(pm->clk_MT_CG_VENC1)) {
		mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_VENC1\n");
		ret = PTR_ERR(pm->clk_MT_CG_VENC1);
	}
#endif
	ion_venc_client = NULL;

	return ret;
}

void mtk_vcodec_release_enc_pm(struct mtk_vcodec_dev *mtkdev)
{
	/* do nothing */
}

void mtk_venc_deinit_ctx_pm(struct mtk_vcodec_ctx *ctx)
{

#ifdef VENC_ENABLE_MMSRAM
	if (ctx->use_slbc == 1) {
		pr_debug("slbc_release, %p\n", &ctx->sram_data);
		clr_mmsram_data_if(&ctx->sram_data);
	}
#endif

}

void mtk_vcodec_enc_clock_on(struct mtk_vcodec_ctx *ctx, unsigned int core_id)
{
#ifndef FPGA_PWRCLK_VENC_API_DISABLE
	struct mtk_vcodec_pm *pm = &ctx->dev->pm;
	int ret;

#ifdef CONFIG_MTK_PSEUDO_M4U
	int i, larb_port_num, larb_id;
	struct M4U_PORT_STRUCT port;
#endif

	time_check_start(MTK_FMT_ENC, core_id);
	if ((core_id == MTK_VENC_CORE_0) ||
		(core_id == MTK_VENC_CORE_1)) {
		pm_runtime_get_sync(&pm_dev[MTK_VENC_CORE_0]->dev);
		ret = clk_prepare_enable(pm->clk_MT_CG_VENC0);
		if (ret)
			mtk_v4l2_err("clk_prepare_enable CG_VENC0 fail %d", ret);

		pm_runtime_get_sync(&pm_dev[MTK_VENC_CORE_1]->dev);
		ret = clk_prepare_enable(pm->clk_MT_CG_VENC1);
		if (ret)
			mtk_v4l2_err("clk_prepare_enable CG_VENC1 fail %d", ret);
	} else
		mtk_v4l2_err("invalid core_id %d", core_id);
	time_check_end(MTK_FMT_ENC, core_id, 50);
#endif
#ifdef VENC_ENABLE_MMSRAM
	if (ctx->use_slbc == 1) {
		time_check_start(MTK_FMT_ENC, core_id);
		ret = mmsram_power_on_if(&ctx->sram_data);
		time_check_end(MTK_FMT_ENC, core_id, 50);
	}
#endif

#ifdef CONFIG_MTK_PSEUDO_M4U
	time_check_start(MTK_FMT_ENC, core_id);
	if (core_id == MTK_VENC_CORE_0) {
		larb_port_num = SMI_LARB7_PORT_NUM;
		larb_id = 7;
	} else if (core_id == MTK_VENC_CORE_1) {
		larb_port_num = SMI_LARB8_PORT_NUM;
		larb_id = 8;
	} else {
		larb_port_num = 0;
		larb_id = 0;
		mtk_v4l2_err("invalid core_id %d", core_id);
	}


	//enable 34bits port configs & sram settings
	for (i = 0; i < larb_port_num; i++) {
		if (i == 5 || i == 6 || i == 13 ||
			i == 14 || i == 21 || i == 22) {
			ret = smi_sysram_enable(MTK_M4U_ID(larb_id, i),
				true, "LARB_VENC");
			if (ret)
				mtk_v4l2_err("%#x is not ready err: %#x\n",
					i, ret);
		} else {
			port.ePortID = MTK_M4U_ID(larb_id, i);
			port.Direction = 0;
			port.Distance = 1;
			port.domain = 0;
			port.Security = 0;
			port.Virtuality = 1;
			m4u_config_port(&port);
		}
	}
	time_check_end(MTK_FMT_ENC, core_id, 50);
#endif

}

void mtk_vcodec_enc_clock_off(struct mtk_vcodec_ctx *ctx, unsigned int core_id)
{
#ifndef FPGA_PWRCLK_VENC_API_DISABLE
	struct mtk_vcodec_pm *pm = &ctx->dev->pm;

#ifdef VENC_ENABLE_MMSRAM
	if (ctx->use_slbc == 1)
		mmsram_power_off_if(&ctx->sram_data);
#endif

	if ((core_id == MTK_VENC_CORE_0) ||
		(core_id == MTK_VENC_CORE_1)) {
		clk_disable_unprepare(pm->clk_MT_CG_VENC0);
		clk_disable_unprepare(pm->clk_MT_CG_VENC1);

		pm_runtime_put_sync(&pm_dev[MTK_VENC_CORE_0]->dev);
		pm_runtime_put_sync(&pm_dev[MTK_VENC_CORE_1]->dev);
	} else
		mtk_v4l2_err("invalid core_id %d", core_id);
#endif
}

int mtk_venc_ion_config_buff(struct dma_buf *dmabuf)
{
/* for dma-buf using ion buffer, ion will check portid in dts
 * So, don't need to config buffer at user side, but remember
 * set iommus attribute in dts file.
 */
	return 0;
}

static int mtk_venc_larb_probe(struct platform_device *pdev)
{
	struct device		*dev = &pdev->dev;
	struct venc_larb_data	*data;
	int ret;

	data = devm_kzalloc(dev, sizeof(*dev), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = of_property_read_u32(dev->of_node, "mediatek,larb-id",
			&data->larb_id);
	if (ret != 0) {
		mtk_v4l2_err("%s failed to find larb-id\n", __func__);
		return ret;
	}

	platform_set_drvdata(pdev, data);
	if (data->larb_id == 19)
		pm_dev[MTK_VENC_CORE_0] = pdev;
	else if (data->larb_id == 20)
		pm_dev[MTK_VENC_CORE_1] = pdev;
	pm_runtime_enable(dev);

	mtk_v4l2_err("%s enable larb %d\n", __func__, data->larb_id);
	return ret;
}

static int mtk_venc_larb_remove(struct platform_device *pdev)
{
	mtk_v4l2_err("%s disable larb\n", __func__);
	return 0;
}

static const struct of_device_id mtk_venc_larb_match[] = {
	{.compatible = "mediatek,mt8195-venc-larb",},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_venc_larb_match);

static struct platform_driver mtk_venc_larb_driver = {
	.probe  = mtk_venc_larb_probe,
	.remove = mtk_venc_larb_remove,
	.driver = {
		.name   = "mtk-venc-larb",
		.of_match_table = mtk_venc_larb_match,
	},
};
module_platform_driver(mtk_venc_larb_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mediatek video codec encoder larb driver");


