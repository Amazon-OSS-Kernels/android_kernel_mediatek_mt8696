/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Qiqi Wang <qiqi.wang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt8696-clk.h>

static const struct mtk_gate_regs nna0_cg_regs = {
	.set_ofs = 0x208,
	.clr_ofs = 0x208,
	.sta_ofs = 0x208,
};

static const struct mtk_gate_regs nna1_cg_regs = {
	.set_ofs = 0x228,
	.clr_ofs = 0x228,
	.sta_ofs = 0x228,
};

#define GATE_NNA0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &nna0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_NNA1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &nna1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate nna_clks[] = {
	/* NNA0 */
	GATE_NNA0(CLK_NNA0, "nna0", "nna0_sel", 0),
	GATE_NNA0(CLK_NNA0_26MEN, "nna0_26men", "clk26m", 1),
	/* NNA1 */
	GATE_NNA1(CLK_NNA0_PWR, "nna0_pwr", "nna0_sel", 0),
};

static int clk_mt8696_nna_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_NNA_NR_CLK);

	mtk_clk_register_gates(node, nna_clks, ARRAY_SIZE(nna_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	return r;
}

static const struct of_device_id of_match_clk_mt8696_nna[] = {
	{ .compatible = "mediatek,mt8696-nna", },
	{}
};

static struct platform_driver clk_mt8696_nna_drv = {
	.probe = clk_mt8696_nna_probe,
	.driver = {
		.name = "clk-mt8696-nna",
		.of_match_table = of_match_clk_mt8696_nna,
	},
};

builtin_platform_driver(clk_mt8696_nna_drv);
