// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Qiqi Wang <qiqi.wang@mediatek.com>

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt8696-clk.h>

static const struct mtk_gate_regs venc_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_VENC(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &venc_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

static const struct mtk_gate venc_clks[] = {
	GATE_VENC(CLK_VENC_SMI_CON, "venc_smi_con", "mmsel", 0),
	GATE_VENC(CLK_VENC_CON, "venc_con", "venc_sel", 4),
};

static int clk_mt8696_venc_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_VENC_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	r = mtk_clk_register_gates(node, venc_clks, ARRAY_SIZE(venc_clks),
							clk_data);
	if (r)
		goto free_venc_clk_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r) {
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
		goto free_venc_clk_data;
	}

	return r;

free_venc_clk_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static const struct of_device_id of_match_clk_mt8696_venc[] = {
	{ .compatible = "mediatek,mt8696-vencsys", },
	{}
};

static struct platform_driver clk_mt8696_venc_drv = {
	.probe = clk_mt8696_venc_probe,
	.driver = {
		.name = "clk-mt8696-venc",
		.of_match_table = of_match_clk_mt8696_venc,
	},
};

static int __init clk_mt8696_venc_init(void)
{
	return platform_driver_register(&clk_mt8696_venc_drv);
}

arch_initcall(clk_mt8696_venc_init);

