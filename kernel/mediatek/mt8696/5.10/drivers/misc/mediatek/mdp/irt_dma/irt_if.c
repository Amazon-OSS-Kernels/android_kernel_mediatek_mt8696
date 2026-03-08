// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/pm_runtime.h>
#include <linux/types.h>
#include <linux/iommu.h>
#include <linux/slab.h>

#include "irt_if.h"
#include "irt_hal.h"
#include "irt_hw.h"
#include "smi.h"

bool irt_log_enable;

#define IRT_INFO(string, args...)  do {\
		if (irt_log_enable)\
			pr_info("[IRT]"string, ##args);\
		} while (0)

#define IRT_ERR(string, args...) pr_info("[IRT]"string, ##args)

static struct irt_data *irt_inst_data;

struct irt_data *irt_get_data(void)
{
	return irt_inst_data;
}

static void irt_smi_larb_en(struct irt_data *data, bool en)
{
	if (en)
		pm_runtime_get_sync(data->larb_dev);
	else
		pm_runtime_put_sync(data->larb_dev);
}

static void irt_pm_en(struct irt_data *data, bool en)
{
	if (en)
		pm_runtime_get_sync(data->dev);
	else
		pm_runtime_put_sync(data->dev);
}

static int irt_power_on(void)
{
	int ret = 0;
	struct irt_data *data = irt_get_data();

	irt_smi_larb_en(data, true);

	irt_pm_en(data, true);

	ret = clk_prepare_enable(data->clk);
	if (ret)
		IRT_ERR("enable clk err\n");

	IRT_INFO("irt power on end\n");

	return ret;
}

static void irt_power_off(void)
{

	struct irt_data *data = irt_get_data();

	clk_disable_unprepare(data->clk);

	irt_pm_en(data, false);

	irt_smi_larb_en(data, false);

	IRT_INFO("irt power off end\n");
}

int irt_ticket_get(void)
{
	unsigned long flags;
	int ret = 0;
	struct irt_data *data = irt_get_data();

	spin_lock_irqsave(&data->state_lock, flags);
	if (data->state == IRT_STATE_IDLE)
		data->state = IRT_STATE_BUSY;
	else
		ret = -EBUSY;
	spin_unlock_irqrestore(&data->state_lock, flags);

	if (!ret)
		irt_power_on();

	return ret;
}

int irt_ticket_put(void)
{
	unsigned long flags;
	int ret = 0;
	struct irt_data *data = irt_get_data();

	spin_lock_irqsave(&data->state_lock, flags);
	if (data->state != IRT_STATE_IDLE)
		data->state = IRT_STATE_IDLE;
	else
		ret = -EINVAL;
	spin_unlock_irqrestore(&data->state_lock, flags);

	if (!ret)
		irt_power_off();

	return 0;
}

static irqreturn_t irt_irq_handle(int irq, void *dev_id)
{
	struct irt_data *data = (struct irt_data *)dev_id;
	unsigned long flags;

	IRT_INFO("%s success\n", __func__);

	irt_dma_hal_clear_irq(data->disp_top_reg_base);

	atomic_set(&data->wait_irt_irq_flag, 1);
	wake_up_interruptible(&data->wait_irt_irq_wq);

	spin_lock_irqsave(&data->state_lock, flags);
	data->state = IRT_STATE_DONE;
	spin_unlock_irqrestore(&data->state_lock, flags);

	return IRQ_HANDLED;
}

static int irt_request_irq(struct irt_data *data)
{
	return devm_request_irq(data->dev, data->irq, irt_irq_handle,
		0, "irt_dma", (void *)data);
}

static int irt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct irt_data *data;
	struct resource *res;
	struct device_node *larb_node;
	struct platform_device *larb_pdev;
	int ret;

	larb_node = of_parse_phandle(pdev->dev.of_node, "mediatek,larb", 0);
	if (!larb_node) {
		dev_notice(dev, "[IRT] get larbs np fail\n");
		return -EINVAL;
	}

	larb_pdev = of_find_device_by_node(larb_node);
	of_node_put(larb_node);
	if ((!larb_pdev) || (!larb_pdev->dev.driver)) {
		dev_notice(dev, "[IRT]%s is earlier than SMI\n", __func__);
		return -EPROBE_DEFER;
	}

	data = devm_kzalloc(dev, sizeof(struct irt_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->larb_dev = &larb_pdev->dev;

	data->clk = devm_clk_get(dev, "irt_dma");
	if (IS_ERR(data->clk)) {
		dev_notice(dev, "[IRT] clk is earlier than dispsys\n");
		return -EPROBE_DEFER;
	}
	IRT_INFO("get clk %s 0x%lx\n", "irt_dma", (unsigned long)data->clk);

	data->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_notice(dev, "[IRT]failed to get MEM resource 0\n");
		return -ENXIO;
	}

	data->irt_reg_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(data->irt_reg_base)) {
		dev_notice(dev, "[IRT]get irt reg base err\n");
		return PTR_ERR(data->irt_reg_base);
	}
	IRT_INFO("reg base is 0x%lx\n", (unsigned long)data->irt_reg_base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_notice(dev, "[IRT]failed to get MEM resource 1\n");
		return -ENXIO;
	}

	data->disp_top_reg_base = devm_ioremap(dev,
		res->start, resource_size(res));
	if (IS_ERR(data->disp_top_reg_base)) {
		dev_notice(dev, "[IRT]get disp top reg base err\n");
		return PTR_ERR(data->disp_top_reg_base);
	}

	IRT_INFO("disp top reg base is 0x%lx\n",
		(unsigned long)data->disp_top_reg_base);

	spin_lock_init(&data->state_lock);

	data->irq = platform_get_irq(pdev, 0);
	if (data->irq < 0) {
		dev_notice(dev, "[IRT] get irq err\n");
		return data->irq;
	}
	IRT_INFO("irq is %d\n", data->irq);

	ret = irt_request_irq(data);
	if (ret) {
		dev_notice(dev, "[IRT]failed to install irq (%d)\n", ret);
		return -ENXIO;
	}

	init_waitqueue_head(&data->wait_irt_irq_wq);

	platform_set_drvdata(pdev, (void *)data);

	pm_runtime_enable(dev);

	irt_inst_data = data;

	return ret;
}

static int irt_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&(pdev->dev));

	return 0;
}

static int irt_wait_complete_timeout(struct irt_data *data, unsigned int time)
{
	int ret = 0;
	unsigned long flags;

	if (wait_event_interruptible_timeout(data->wait_irt_irq_wq,
			atomic_read(&data->wait_irt_irq_flag), time) == 0) {
		IRT_ERR("error wait frame done timeout %dms\n", time);
		ret = -ENXIO;
	}

	atomic_set(&data->wait_irt_irq_flag, 0);

	spin_lock_irqsave(&data->state_lock, flags);
	data->state = IRT_STATE_DONE;
	spin_unlock_irqrestore(&data->state_lock, flags);

	IRT_INFO("%s success\n", __func__);
	return ret;
}

static void irt_dma_buffer_release(struct mtk_irt_dma *irt_dma)
{
	if (!irt_dma->dma_buf || !irt_dma->attach || !irt_dma->sgt) {
		IRT_ERR("invalid dma buffer\n");
		return;
	}
	dma_buf_unmap_attachment(irt_dma->attach,
		irt_dma->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(irt_dma->dma_buf, irt_dma->attach);
	dma_buf_put(irt_dma->dma_buf);

	irt_dma->dma_buf = NULL;
	irt_dma->attach = NULL;
	irt_dma->sgt = NULL;
}

static int irt_dma_fd_to_buffer(int fd, struct device *dev, struct mtk_irt_dma *irt_dma)
{
	int ret = 0;

	irt_dma->dma_buf = dma_buf_get(fd);
	if (IS_ERR(irt_dma->dma_buf)) {
		ret = (int)PTR_ERR(irt_dma->dma_buf);
		IRT_ERR("dma buf get failed, ret:%d\n", ret);
		return ret;
	}
	irt_dma->attach = dma_buf_attach(irt_dma->dma_buf, dev);
	if (IS_ERR(irt_dma->attach)) {
		ret = (int)PTR_ERR(irt_dma->attach);
		dma_buf_put(irt_dma->dma_buf);
		IRT_ERR("dma buf attach failed, ret:%d\n", ret);
		return ret;
	}

	irt_dma->sgt = dma_buf_map_attachment(irt_dma->attach, DMA_BIDIRECTIONAL);

	if (IS_ERR(irt_dma->sgt)) {
		ret = (int)PTR_ERR(irt_dma->sgt);
		IRT_ERR("dma buf map attachment failed, ret:%d\n", ret);
		dma_buf_detach(irt_dma->dma_buf, irt_dma->attach);
		dma_buf_put(irt_dma->dma_buf);
		return ret;
	}

	irt_dma->dma_addr = (unsigned int)sg_dma_address(irt_dma->sgt->sgl);

	IRT_INFO("dma fd to buffer, fd:%d, dma_add:0x%x\n", fd, irt_dma->dma_addr);

	return irt_dma->dma_addr;
}

static int irt_dma_prepare_buffer(struct irt_data *data,
	struct irt_dma_config_param *config)
{
	int ret = 0;
	unsigned int mva = 0;

	mva = irt_dma_fd_to_buffer(config->src_fd, data->dev, &config->src_irt_dma);
	if (mva == 0) {
		IRT_ERR("[IRT]Get invalid src_fd %d\n",
			config->src_fd);
		ret = -EFAULT;
		goto error;
	}

	config->rg_y_dram_rd_addr = mva + config->src_offset_y_len;
	config->rg_c_dram_rd_addr = mva + config->src_offset_c_len;

	mva = 0;
	mva = irt_dma_fd_to_buffer(config->dst_fd, data->dev, &config->dst_irt_dma);
	if (mva == 0) {
		IRT_ERR("[IRT]Get invalid dst_fd %d\n",
			config->dst_fd);
		ret = -EFAULT;
		goto error;
	}

	config->rg_y_dram_rd_addr = mva + config->src_offset_y_len;
	config->rg_c_dram_rd_addr = mva + config->src_offset_c_len;

	return 0;

error:
	return ret;
}


static int irt_set_config_info(struct irt_dma_info *irt_info,
	struct irt_dma_config_param *irt_config)
{
	switch (irt_info->src_color_fmt) {
	case IRT_DMA_SRC_COL_MD_YC420_8BIT_BLK:
		irt_config->rg_5351_mode_en = 0;
		irt_config->rg_scan_line = 0;
		irt_config->rg_src_10bit_en = 0;
		irt_config->rg_blk_burst_en = 1;
		break;

	case IRT_DMA_SRC_COL_MD_YC420_8BIT_BLK_5351:
		irt_config->rg_5351_mode_en = 1;
		irt_config->rg_5351_mode_sel = 0x2;
		irt_config->rg_scan_line = 0;
		irt_config->rg_src_10bit_en = 0;
		irt_config->rg_blk_burst_en = 1;
		break;

	case IRT_DMA_SRC_COL_MD_YC420_8BIT_SCL:
		irt_config->rg_5351_mode_en = 0;
		irt_config->rg_scan_line = 1;
		irt_config->rg_src_10bit_en = 0;
		break;

	case IRT_DMA_SRC_COL_MD_YC420_10BIT_RASTER:
		irt_config->rg_5351_mode_en = 0;
		irt_config->rg_scan_line = 0;
		irt_config->rg_src_10bit_en = 1;
		irt_config->rg_blk_burst_en = 1;
		irt_config->rg_10bit_mode_sel = 0;
		if (irt_info->dst_color_fmt ==
			IRT_DMA_DST_COL_MD_YC420_8BIT_SCL) {
			irt_config->rg_10bit_rotate_en = 1;
			irt_config->rg_dither_en = 1;
			irt_config->rg_dither_mode = irt_info->dither_mode;
		} else if (irt_info->dst_color_fmt ==
		IRT_DMA_DST_COL_MD_YC420_10BIT_SCL) {
			irt_config->rg_10bit_rotate_en = 0;
			irt_config->rg_dither_en = 0;
		} else
			IRT_ERR("unknown output colr fmt %d\n",
			irt_info->dst_color_fmt);
		break;

	case IRT_DMA_SRC_COL_MD_YC420_10BIT_TILE:
		irt_config->rg_5351_mode_en = 0;
		irt_config->rg_scan_line = 0;
		irt_config->rg_src_10bit_en = 1;
		irt_config->rg_blk_burst_en = 1;
		irt_config->rg_10bit_mode_sel = 1;
		if (irt_info->dst_color_fmt ==
			IRT_DMA_DST_COL_MD_YC420_8BIT_SCL) {
			irt_config->rg_10bit_rotate_en = 1;
			irt_config->rg_dither_en = 1;
			irt_config->rg_dither_mode = irt_info->dither_mode;
		} else if (irt_info->dst_color_fmt ==
		IRT_DMA_DST_COL_MD_YC420_10BIT_SCL) {
			irt_config->rg_10bit_rotate_en = 0;
			irt_config->rg_dither_en = 0;
		} else
			IRT_ERR("unknown output colr fmt %d\n",
			irt_info->dst_color_fmt);
		break;

	default:
		IRT_ERR("unknown input color fmt %d\n",
			irt_info->src_color_fmt);
	}

	irt_config->rg_align_hsize = irt_info->src_width_align;
	irt_config->rg_align_vsize = irt_info->src_height_align;

	irt_config->rg_rotate_mode = irt_info->rotate_mode;
	irt_config->rg_wr_garbage_cancel = 0;

	irt_config->src_fd = irt_info->src_fd;
	irt_config->dst_fd = irt_info->dst_fd;

	irt_config->src_offset_y_len = irt_info->src_offset_y_len;
	irt_config->src_offset_c_len = irt_info->src_offset_c_len;
	irt_config->dst_offset_y_len = irt_info->dst_offset_y_len;
	irt_config->dst_offset_c_len = irt_info->dst_offset_c_len;

	IRT_INFO(
"5351 mode%d,scane line%d,rotate mode%d,burst read%d size(%d %d)\n",
		irt_config->rg_5351_mode_en, irt_config->rg_scan_line,
		irt_config->rg_rotate_mode,
		irt_config->rg_blk_burst_en, irt_config->rg_align_hsize,
		irt_config->rg_align_vsize);

	IRT_INFO(
"10 bit %d,10bit rotate%d,10bit mode%d,dither%d,dither mode%d\n",
		irt_config->rg_src_10bit_en, irt_config->rg_10bit_rotate_en,
		irt_config->rg_10bit_mode_sel, irt_config->rg_dither_en,
		irt_config->rg_dither_mode);

	IRT_INFO("src_fd:%d,offset(%d %d),dst_fd:%d,offset(%d %d)\n",
		irt_config->src_fd, irt_config->src_offset_y_len,
		irt_config->src_offset_c_len,
		irt_config->dst_fd, irt_config->dst_offset_y_len,
		irt_config->dst_offset_c_len);

	return 0;
}

static int irt_trigger_sync_normal(struct irt_dma_config_param *irt_config)
{
	struct irt_data *data = irt_get_data();

	irt_dma_prepare_buffer(data, irt_config);

	IRT_INFO("src addr (0x%x 0x%x), dst addr (0x%x 0x%x)\n",
		irt_config->rg_y_dram_rd_addr, irt_config->rg_c_dram_rd_addr,
		irt_config->rg_y_dram_wr_addr, irt_config->rg_c_dram_wr_addr);

	irt_dma_hal_hw_reset(data->irt_reg_base);
	irt_dma_hal_config(irt_config, data->irt_reg_base);

	irt_wait_complete_timeout(data, 1000);

	irt_dma_buffer_release(&irt_config->src_irt_dma);
	irt_dma_buffer_release(&irt_config->dst_irt_dma);

	return 0;
}

int irt_dma_trigger_sync(struct irt_dma_info *irt_info)
{
	unsigned int ret = 0;

	struct irt_dma_config_param irt_config;

	memset((void *)&irt_config, 0, sizeof(irt_config));

	irt_set_config_info(irt_info, &irt_config);

	ret = irt_trigger_sync_normal(&irt_config);

	return ret;
}

void irt_set_log_enable(unsigned int en)
{
	if (en)
		irt_log_enable = true;
	else
		irt_log_enable = false;

	pr_info("[IRT] log enable is %d\n", irt_log_enable);

}

static const struct of_device_id irt_of_ids[] = {
	{.compatible = "mediatek,mt8696-irt",},
	{}			/* NULL */
};

static struct platform_driver mtk_irt_driver = {
	.probe = irt_probe,
	.remove = irt_remove,
	/* No need the suspend/resume as the default clk always is disable. */
	.driver = {
		   .name = "mtk-irt",
		   .of_match_table = irt_of_ids,
	}
};

static int __init mtk_irt_init(void)
{
	IRT_INFO("[IRT]%s in\n", __func__);

	if (platform_driver_register(&mtk_irt_driver))
		return -ENODEV;

	IRT_INFO("[IRT]%s out\n", __func__);
	return 0;
}

static void __exit mtk_irt_exit(void)
{
	platform_driver_unregister(&mtk_irt_driver);
}

module_init(mtk_irt_init);
module_exit(mtk_irt_exit);
