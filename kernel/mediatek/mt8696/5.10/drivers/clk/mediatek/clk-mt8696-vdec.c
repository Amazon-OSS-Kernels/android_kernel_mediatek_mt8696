// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Qiqi Wang <qiqi.wang@mediatek.com>

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt8696-clk.h>

static const struct mtk_gate_regs vdec0_cg_regs = {
	.set_ofs = 0x0,
	.clr_ofs = 0x4,
	.sta_ofs = 0x0,
};

static const struct mtk_gate_regs vdec1_cg_regs = {
	.set_ofs = 0x200,
	.clr_ofs = 0x204,
	.sta_ofs = 0x200,
};

static const struct mtk_gate_regs vdec2_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0xc,
	.sta_ofs = 0x8,
};

#define GATE_VDEC0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdec0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_VDEC1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdec1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_VDEC2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdec2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

static const struct mtk_gate vdec_clks[] = {
	/* VDEC0 */
	GATE_VDEC0(CLK_VDECEN, "vdecen", "vdec_sel", 0),
	GATE_VDEC0(CLK_VDEC_ATIV, "vdec_ativ", "vdec_sel", 4),
	/* VDEC1 */
	GATE_VDEC1(CLK_VDEC_LATEN, "vdec_laten", "vdec_sel", 0),
	GATE_VDEC1(CLK_VDEC_LAT_ATIV, "vdec_lat_ativ", "vdec_sel", 4),
	/* VDEC2 */
	GATE_VDEC2(CLK_VDEC_L1EN, "vdec_l1en", "vdec_sel", 0),
};

static int clk_mt8696_vdec_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_VDEC_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	r = mtk_clk_register_gates(node, vdec_clks, ARRAY_SIZE(vdec_clks),
							clk_data);
	if (r)
		goto free_vdec_clk_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r) {
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
		goto free_vdec_clk_data;
	}

	return r;

free_vdec_clk_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static const struct of_device_id of_match_clk_mt8696_vdec[] = {
	{ .compatible = "mediatek,vdec_gcon", },
	{}
};

static struct platform_driver clk_mt8696_vdec_drv = {
	.probe = clk_mt8696_vdec_probe,
	.driver = {
		.name = "clk-mt8696-vdec",
		.of_match_table = of_match_clk_mt8696_vdec,
	},
};

static int __init clk_mt8696_vdec_init(void)
{
	return platform_driver_register(&clk_mt8696_vdec_drv);
}

arch_initcall(clk_mt8696_vdec_init);

