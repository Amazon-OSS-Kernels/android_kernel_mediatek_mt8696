// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Qiqi Wang <qiqi.wang@mediatek.com>

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt8696-clk.h>

static const struct mtk_gate_regs vdsoc0_cg_regs = {
	.set_ofs = 0x0,
	.clr_ofs = 0x4,
	.sta_ofs = 0x0,
};

static const struct mtk_gate_regs vdsoc1_cg_regs = {
	.set_ofs = 0x200,
	.clr_ofs = 0x204,
	.sta_ofs = 0x200,
};

static const struct mtk_gate_regs vdsoc2_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0xc,
	.sta_ofs = 0x8,
};

#define GATE_VDSOC0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdsoc0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_VDSOC1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdsoc1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_VDSOC2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdsoc2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

static const struct mtk_gate vdsoc_clks[] = {
	/* VDSOC0 */
	GATE_VDSOC0(CLK_VDSOC_VSEN, "vdsoc_vsen", "vdec_sel", 0),
	GATE_VDSOC0(CLK_VDSOC_VS_ATIV, "vdsoc_vs_ativ", "vdec_sel", 4),
	/* VDSOC1 */
	GATE_VDSOC1(CLK_VDSOC_LSEN, "vdsoc_lsen", "vdec_sel", 0),
	GATE_VDSOC1(CLK_VDSOC_LS_ATIV, "vdsoc_ls_ativ", "vdec_sel", 4),
	/* VDSOC2 */
	GATE_VDSOC2(CLK_VDSOC_L1_SOCEN, "vdsoc_l1_socen", "vdec_sel", 0),
};

static int clk_mt8696_vdsoc_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_VDSOC_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	r = mtk_clk_register_gates(node, vdsoc_clks, ARRAY_SIZE(vdsoc_clks),
							clk_data);
	if (r)
		goto free_vdsoc_clk_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r) {
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
		goto free_vdsoc_clk_data;
	}

	return r;

free_vdsoc_clk_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static const struct of_device_id of_match_clk_mt8696_vdsoc[] = {
	{ .compatible = "mediatek,vdec_soc_gcon", },
	{}
};

static struct platform_driver clk_mt8696_vdsoc_drv = {
	.probe = clk_mt8696_vdsoc_probe,
	.driver = {
		.name = "clk-mt8696-vdsoc",
		.of_match_table = of_match_clk_mt8696_vdsoc,
	},
};

static int __init clk_mt8696_vdsoc_init(void)
{
	return platform_driver_register(&clk_mt8696_vdsoc_drv);
}

arch_initcall(clk_mt8696_vdsoc_init);
