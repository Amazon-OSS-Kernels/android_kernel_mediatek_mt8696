// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Qiqi Wang <qiqi.wang@mediatek.com>

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt8696-clk.h>

static const struct mtk_gate_regs img0_cg_regs = {
	.set_ofs = 0x28,
	.clr_ofs = 0x28,
	.sta_ofs = 0x28,
};

static const struct mtk_gate_regs img1_cg_regs = {
	.set_ofs = 0xc,
	.clr_ofs = 0xc,
	.sta_ofs = 0xc,
};

#define GATE_IMG0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &img0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_IMG1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &img1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate img_clks[] = {
	/* IMG0 */
	GATE_IMG0(CLK_IMG_MVDO_FIFO, "img_mvdo_fifo", "hd_sel", 0),
	GATE_IMG0(CLK_IMG_MFILM_GRAIN, "img_mfilm_grain", "mmsel", 1),
	GATE_IMG0(CLK_IMG_MHDR_VDO_FE, "img_mhdr_vdo_fe", "mmsel", 2),
	GATE_IMG0(CLK_IMG_MVDO_FE_ADL, "img_mvdo_fe_adl", "mmsel", 3),
	GATE_IMG0(CLK_IMG_SVDO_FIFO, "img_svdo_fifo", "hd_sel", 4),
	GATE_IMG0(CLK_IMG_SFILMGRAIN, "img_sfilmgrain", "mmsel", 5),
	GATE_IMG0(CLK_IMG_SHDR_VDO_FE, "img_shdr_vdo_fe", "hd_sel", 6),
	/* IMG1 */
	GATE_IMG1(CLK_IMG_VDO3, "img_vdo3", "vdo3_sel", 0),
	GATE_IMG1(CLK_IMG_DISPFMT3, "img_dispfmt3", "vdo3_sel", 1),
	GATE_IMG1(CLK_IMG_R2R, "img_r2r", "hd_sel", 2),
	GATE_IMG1(CLK_IMG_SMI_L5, "img_smi_l5", "mmsel", 4),
	GATE_IMG1(CLK_IMG_SMI_L6, "img_smi_l6", "mmsel", 5),
	GATE_IMG1(CLK_IMG_DRAMC_L8, "img_dramc_l8", "mmsel", 6),
	GATE_IMG1(CLK_IMG_VDO4, "img_vdo4", "vdo4_sel", 7),
	GATE_IMG1(CLK_IMG_DISPFMT4, "img_dispfmt4", "vdo4_sel", 8),
	GATE_IMG1(CLK_IMG_IRT_DMA, "img_irt_dma", "mmsel", 11),
	GATE_IMG1(CLK_IMGRSZ, "imgrsz", "rsz_sel", 12),
	GATE_IMG1(CLK_IMG_VDO_DI, "img_vdo_di", "di_sel", 14),
	GATE_IMG1(CLK_IMG_DISPFMT_DI, "img_dispfmt_di", "di_sel", 15),
	GATE_IMG1(CLK_IMG_NR, "img_nr", "nr_sel", 16),
	GATE_IMG1(CLK_IMG_WR_CHANNEL, "img_wr_channel", "mmsel", 17),
};

static int clk_mt8696_img_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_IMG_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	r = mtk_clk_register_gates(node, img_clks, ARRAY_SIZE(img_clks), clk_data);
	if (r)
		goto free_img_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r) {
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
		goto free_img_data;
	}

	return r;

free_img_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static const struct of_device_id of_match_clk_mt8696_img[] = {
	{ .compatible = "mediatek,mt8696-disp", },
	{}
};

static struct platform_driver clk_mt8696_img_drv = {
	.probe = clk_mt8696_img_probe,
	.driver = {
		.name = "clk-mt8696-img",
		.of_match_table = of_match_clk_mt8696_img,
	},
};

static int __init clk_mt8696_img_init(void)
{
	return platform_driver_register(&clk_mt8696_img_drv);
}

arch_initcall(clk_mt8696_img_init);

