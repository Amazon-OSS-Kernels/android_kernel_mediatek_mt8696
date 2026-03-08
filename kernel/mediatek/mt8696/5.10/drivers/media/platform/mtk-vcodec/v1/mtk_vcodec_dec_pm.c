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
#include <soc/mediatek/smi.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of.h>

#include "mtk_vcodec_dec_pm.h"
#include "mtk_vcodec_util.h"
#include "mtk_vcu.h"

#ifdef CONFIG_MTK_PSEUDO_M4U
#include <mach/mt_iommu.h>
#include "mach/pseudo_m4u.h"
#include "smi_port.h"
#endif

static struct ion_client *ion_vdec_client;
static struct platform_device *pm_dev[MTK_VDEC_HW_NUM] = {NULL};
struct vdec_larb_data {
	unsigned int	larb_id;
	struct device	*dev;
};

void mtk_dec_init_ctx_pm(struct mtk_vcodec_ctx *ctx)
{
	ctx->input_driven = 0;
	ctx->user_lock_hw = 1;
}
#ifndef FPGA_PWRCLK_API_DISABLE
static unsigned long get_vdecpll_clock_freq(void)
{
	unsigned long rate = 680 * 1000 * 1000;
	return rate;
}
#endif
int mtk_vcodec_init_dec_pm(struct mtk_vcodec_dev *mtkdev)
{
	int ret = 0;
#ifndef FPGA_PWRCLK_API_DISABLE
	/* struct device_node *node; */
	struct platform_device *pdev;
	struct mtk_vcodec_pm *pm;

	pdev = mtkdev->plat_dev;
	pm_runtime_enable(&pdev->dev);
	pm = &mtkdev->pm;
	pm->mtkdev = mtkdev;
	pm->chip_node = of_find_compatible_node(NULL,
		NULL, "mediatek,mt8195-vcodec-dec");
	pm->larbvdec = &pdev->dev;
	pdev = mtkdev->plat_dev;
	pm->dev = &pdev->dev;

	if (pm->chip_node) {

		pm->vdecpll_ck = devm_clk_get(&pdev->dev, "vdecpll_ck");
		if (IS_ERR(pm->vdecpll_ck)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get vdecpll_ck\n");
			return PTR_ERR(pm->vdecpll_ck);
		}

		ret = clk_set_rate(pm->vdecpll_ck, get_vdecpll_clock_freq());
		if (ret)
			mtk_v4l2_err("set vdecpll_ck  fail, ret:%d\n", ret);

		/* soc */
		pm->clk_MT_CG_SOC_LARB1 = devm_clk_get(&pdev->dev, "MT_CG_SOC_LARB1");
		if (IS_ERR(pm->clk_MT_CG_SOC_LARB1)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_SOC_LARB1\n");
			return PTR_ERR(pm->clk_MT_CG_SOC_LARB1);
		}
		pm->clk_MT_CG_SOC_LAT = devm_clk_get(&pdev->dev, "MT_CG_SOC_LAT");
		if (IS_ERR(pm->clk_MT_CG_SOC_LAT)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_SOC_LAT\n");
			return PTR_ERR(pm->clk_MT_CG_SOC_LAT);
		}
		pm->clk_MT_CG_SOC_VDEC = devm_clk_get(&pdev->dev, "MT_CG_SOC_VDEC");
		if (IS_ERR(pm->clk_MT_CG_SOC_VDEC)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_SOC_VDEC\n");
			return PTR_ERR(pm->clk_MT_CG_SOC_VDEC);
		}

		/* core0 */
		pm->clk_MT_CG_LARB1 = devm_clk_get(&pdev->dev, "MT_CG_LARB1");
		if (IS_ERR(pm->clk_MT_CG_LARB1)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_LARB1\n");
			return PTR_ERR(pm->clk_MT_CG_LARB1);
		}
		pm->clk_MT_CG_LAT = devm_clk_get(&pdev->dev, "MT_CG_LAT");
		if (IS_ERR(pm->clk_MT_CG_LAT)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_LAT\n");
			return PTR_ERR(pm->clk_MT_CG_LAT);
		}
		pm->clk_MT_CG_VDEC = devm_clk_get(&pdev->dev, "MT_CG_VDEC");
		if (IS_ERR(pm->clk_MT_CG_VDEC)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_VDEC\n");
			return PTR_ERR(pm->clk_MT_CG_VDEC);
		}

		/* core1 */
		pm->clk_MT_CG_CORE1_LARB1 = devm_clk_get(&pdev->dev, "MT_CG_CORE1_LARB1");
		if (IS_ERR(pm->clk_MT_CG_CORE1_LARB1)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_CORE1_LARB1\n");
			return PTR_ERR(pm->clk_MT_CG_CORE1_LARB1);
		}
		pm->clk_MT_CG_CORE1_LAT = devm_clk_get(&pdev->dev, "MT_CG_CORE1_LAT");
		if (IS_ERR(pm->clk_MT_CG_CORE1_LAT)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_CORE1_LAT\n");
			return PTR_ERR(pm->clk_MT_CG_CORE1_LAT);
		}
		pm->clk_MT_CG_CORE1_VDEC = devm_clk_get(&pdev->dev, "MT_CG_CORE1_VDEC");
		if (IS_ERR(pm->clk_MT_CG_CORE1_VDEC)) {
			mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get MT_CG_CORE1_VDEC\n");
			return PTR_ERR(pm->clk_MT_CG_CORE1_VDEC);
		}
	} else
		mtk_v4l2_err("[VCODEC][ERROR] DTS went wrong...");

	atomic_set(&pm->dec_active_cnt, 0);
	memset(pm->vdec_racing_info, 0, sizeof(pm->vdec_racing_info));
	mutex_init(&pm->dec_racing_info_mutex);
#endif
	ion_vdec_client = NULL;
	return ret;
}

void mtk_vcodec_release_dec_pm(struct mtk_vcodec_dev *dev)
{
}

void mtk_vcodec_dec_pw_on(struct mtk_vcodec_pm *pm, int hw_id)
{
#ifndef FPGA_PWRCLK_API_DISABLE
	int ret = 0;

	mtk_v4l2_debug(1, "hw_id = %d", hw_id);
	ret = pm_runtime_get_sync(pm->dev);
	if (ret < 0)
		mtk_v4l2_err(" %d fail ret =  %d\n", hw_id, ret);

	if (hw_id == MTK_VDEC_CORE || hw_id == MTK_VDEC_CORE1) {
		ret = pm_runtime_get_sync(&pm_dev[MTK_VDEC_CORE]->dev);
		if (ret < 0)
			mtk_v4l2_err(" %d fail ret =  %d\n", hw_id, ret);

		ret = pm_runtime_get_sync(&pm_dev[MTK_VDEC_CORE1]->dev);
		if (ret < 0)
			mtk_v4l2_err(" %d fail ret =  %d\n", hw_id, ret);
	} else if (hw_id == MTK_VDEC_LAT || hw_id == MTK_VDEC_LAT1) {
		ret = pm_runtime_get_sync(&pm_dev[MTK_VDEC_LAT]->dev);
		if (ret < 0)
			mtk_v4l2_err(" %d fail ret =  %d\n", hw_id, ret);
		ret = pm_runtime_get_sync(&pm_dev[MTK_VDEC_LAT1]->dev);
		if (ret < 0)
			mtk_v4l2_err(" %d fail ret =  %d\n", hw_id, ret);
	}
#endif
}

void mtk_vcodec_dec_pw_off(struct mtk_vcodec_pm *pm, int hw_id)
{
#ifndef FPGA_PWRCLK_API_DISABLE
	mtk_v4l2_debug(4, "+ HW ID %d\n", hw_id);
	pm_runtime_put_sync(pm->dev);
	if (hw_id == MTK_VDEC_CORE || hw_id == MTK_VDEC_CORE1) {
		pm_runtime_put_sync(&pm_dev[MTK_VDEC_CORE]->dev);
		pm_runtime_put_sync(&pm_dev[MTK_VDEC_CORE1]->dev);

	} else if (hw_id == MTK_VDEC_LAT || hw_id == MTK_VDEC_LAT1) {
		pm_runtime_put_sync(&pm_dev[MTK_VDEC_LAT]->dev);
		pm_runtime_put_sync(&pm_dev[MTK_VDEC_LAT1]->dev);
	}
#endif
}

void mtk_vcodec_dec_clock_on(struct mtk_vcodec_pm *pm, int hw_id)
{

#ifdef CONFIG_MTK_PSEUDO_M4U
	int i, larb_port_num, larb_id;
	struct M4U_PORT_STRUCT port;
#endif

#ifndef FPGA_PWRCLK_API_DISABLE
	int j, ret;
	struct mtk_vcodec_dev *dev;
	void __iomem *vdec_racing_addr;

	time_check_start(MTK_FMT_DEC, hw_id);
	mtk_vcodec_dec_pw_on(pm, hw_id);

	/* soc */
	ret = clk_prepare_enable(pm->clk_MT_CG_SOC_VDEC);//MT_CG_SOC_VDEC
	if (ret)
		mtk_v4l2_err("clk_prepare_enable clk_MT_CG_SOC_VDEC fail %d",
			ret);
	ret = clk_prepare_enable(pm->clk_MT_CG_SOC_LARB1);
	if (ret)
		mtk_v4l2_err("clk_prepare_enable clk_MT_CG_SOC_LARB1 fail %d",
			ret);

	if (hw_id == MTK_VDEC_CORE || hw_id == MTK_VDEC_CORE1) {
		ret = clk_prepare_enable(pm->clk_MT_CG_VDEC);
		if (ret)
			mtk_v4l2_err("clk_prepare_enable clk_MT_CG_VDEC fail %d",
				ret);
		ret = clk_prepare_enable(pm->clk_MT_CG_LARB1);
		if (ret)
			mtk_v4l2_err("clk_prepare_enable clk_MT_CG_LARB1 fail %d",
				ret);
		ret = clk_prepare_enable(pm->clk_MT_CG_CORE1_VDEC);
		if (ret)
			mtk_v4l2_err("clk_prepare_enable clk_MT_CG_CORE1_VDEC fail %d",
				ret);

		ret = clk_prepare_enable(pm->clk_MT_CG_CORE1_LARB1);
		if (ret)
			mtk_v4l2_err("clk_prepare_enable clk_MT_CG_CORE1_LARB1 fail %d",
				ret);

	} else if (hw_id == MTK_VDEC_LAT || hw_id == MTK_VDEC_LAT1) {
		ret = clk_prepare_enable(pm->clk_MT_CG_SOC_LAT);
		if (ret)
			mtk_v4l2_err("clk_prepare_enable clk_MT_CG_SOC_LAT fail %d",
				ret);

	} else
		mtk_v4l2_err("invalid hw_id %d", hw_id);

	mutex_lock(&pm->dec_racing_info_mutex);
	if (atomic_inc_return(&pm->dec_active_cnt) == 1) {
		/* restore racing info read/write ptr */
		dev = container_of(pm, struct mtk_vcodec_dev, pm);
		vdec_racing_addr =
			dev->dec_reg_base[VDEC_RACING_CTRL] +
				MTK_VDEC_RACING_INFO_OFFSET;
		for (j = 0; j < MTK_VDEC_RACING_INFO_SIZE; j++)
			writel(pm->vdec_racing_info[j],
				vdec_racing_addr + j * 4);
	}
	mutex_unlock(&pm->dec_racing_info_mutex);
	time_check_end(MTK_FMT_DEC, hw_id, 50);
#endif

#ifdef CONFIG_MTK_PSEUDO_M4U
	time_check_start(MTK_FMT_DEC, hw_id);
	if (hw_id == MTK_VDEC_CORE) {
		larb_port_num = SMI_LARB4_PORT_NUM;
		larb_id = 4;

		//enable UFO port
		port.ePortID = M4U_PORT_L5_VDEC_UFO_ENC_EXT_DISP;
		port.Direction = 0;
		port.Distance = 1;
		port.domain = 0;
		port.Security = 0;
		port.Virtuality = 1;
		m4u_config_port(&port);
	} else if (hw_id == MTK_VDEC_LAT) {
		larb_port_num = SMI_LARB5_PORT_NUM;
		larb_id = 5;
	} else {
		larb_port_num = 0;
		larb_id = 0;
		mtk_v4l2_err("invalid hw_id %d", hw_id);
	}

	//enable 34bits port configs & sram settings
	for (i = 0; i < larb_port_num; i++) {
		port.ePortID = MTK_M4U_ID(larb_id, i);
		port.Direction = 0;
		port.Distance = 1;
		port.domain = 0;
		port.Security = 0;
		port.Virtuality = 1;
		m4u_config_port(&port);
	}
	time_check_end(MTK_FMT_DEC, hw_id, 50);
#endif

}

void mtk_vcodec_dec_clock_off(struct mtk_vcodec_pm *pm, int hw_id)
{
#ifndef FPGA_PWRCLK_API_DISABLE
	struct mtk_vcodec_dev *dev;
	void __iomem *vdec_racing_addr;
	int i;

	mutex_lock(&pm->dec_racing_info_mutex);
	if (atomic_dec_and_test(&pm->dec_active_cnt)) {
		/* backup racing info read/write ptr */
		dev = container_of(pm, struct mtk_vcodec_dev, pm);
		vdec_racing_addr =
			dev->dec_reg_base[VDEC_RACING_CTRL] +
				MTK_VDEC_RACING_INFO_OFFSET;
		for (i = 0; i < MTK_VDEC_RACING_INFO_SIZE; i++)
			pm->vdec_racing_info[i] =
				readl(vdec_racing_addr + i * 4);
	}
	mutex_unlock(&pm->dec_racing_info_mutex);
	/* soc */
	clk_disable_unprepare(pm->clk_MT_CG_SOC_VDEC);
	clk_disable_unprepare(pm->clk_MT_CG_SOC_LARB1);

	if (hw_id == MTK_VDEC_CORE || hw_id == MTK_VDEC_CORE1) {
		clk_disable_unprepare(pm->clk_MT_CG_VDEC);
		clk_disable_unprepare(pm->clk_MT_CG_LARB1);
		clk_disable_unprepare(pm->clk_MT_CG_CORE1_VDEC);
		clk_disable_unprepare(pm->clk_MT_CG_CORE1_LARB1);
	} else if (hw_id == MTK_VDEC_LAT || hw_id == MTK_VDEC_LAT1) {
		clk_disable_unprepare(pm->clk_MT_CG_SOC_LAT);
	} else
		mtk_v4l2_err("invalid hw_id %d", hw_id);

	mtk_vcodec_dec_pw_off(pm, hw_id);

#endif
}

int mtk_vdec_ion_config_buff(struct dma_buf *dmabuf)
{
/* for dma-buf using ion buffer, ion will check portid in dts
 * So, don't need to config buffer at user side, but remember
 * set iommus attribute in dts file.
 */
	return 0;
}
static int mtk_vdec_larb_probe(struct platform_device *pdev)
{
	struct device           *dev = &pdev->dev;
	struct vdec_larb_data   *data;
	int ret;

	data = devm_kzalloc(dev, sizeof(*dev), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret =
	of_property_read_u32(dev->of_node, "mediatek,larb-id", &data->larb_id);
	if (ret != 0) {
		dev_info(dev, "[%s] failed to find larb-id\n", __func__);
		return ret;
	}
	platform_set_drvdata(pdev, data);
	if (data->larb_id == 21)
		pm_dev[MTK_VDEC_CORE] = pdev;
	else if (data->larb_id == 22)
		pm_dev[MTK_VDEC_CORE1] = pdev;
	else if (data->larb_id == 23)
		pm_dev[MTK_VDEC_LAT] = pdev;
	else if (data->larb_id == 24)
		pm_dev[MTK_VDEC_LAT1] = pdev;
	pm_runtime_enable(dev);
	mtk_v4l2_err("%s enable larb %d\n", __func__, data->larb_id);
	return ret;
}
static int mtk_vdec_larb_remove(struct platform_device *pdev)
{
	mtk_v4l2_err("%s disable larb\n", __func__);
	return 0;
}

static const struct of_device_id mtk_vdec_larb_match[] = {
	{.compatible = "mediatek,mt8195-vcodec-larb",},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_vdec_larb_match);

static struct platform_driver mtk_vdec_larb_driver = {
	.probe  = mtk_vdec_larb_probe,
	.remove = mtk_vdec_larb_remove,
	.driver = {
		.name   = "mtk-vdec-larb",
		.of_match_table = mtk_vdec_larb_match,
	},
};
module_platform_driver(mtk_vdec_larb_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mediatek video codec decoder larb driver");

