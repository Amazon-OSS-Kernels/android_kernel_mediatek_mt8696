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

static const struct mtk_gate_regs ether_cg_regs = {
	.set_ofs = 0x300,
	.clr_ofs = 0x300,
	.sta_ofs = 0x300,
};

#define GATE_ETHER(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &ether_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate ether_clks[] = {
	GATE_ETHER(CLK_ETH_APB_PCLKEN, "eth_apb_pclken", "axi_sel", 0),
	GATE_ETHER(CLK_ETH_AXI_MCLKEN, "eth_axi_mclken", "axi_sel", 1),
	GATE_ETHER(CLK_ETH_RX_I_RMIIEN, "eth_rx_i_rmiien", "eth_50mrmsel", 2),
	GATE_ETHER(CLK_ETH_TX_I_RMIIEN, "eth_tx_i_rmiien", "eth_50mrmsel", 3),
	GATE_ETHER(CLK_ETH_RX_IEN, "eth_rx_ien", "clk_null", 4),
	GATE_ETHER(CLK_ETH_TX_IEN, "eth_tx_ien", "clk_null", 5),
	GATE_ETHER(CLK_ETH_MAC_EXTEN, "eth_mac_exten", "eth_125msel", 6),
	GATE_ETHER(CLK_ETH_NCLK_IEN, "eth_nclk_ien", "eth_125msel", 7),
};

static int clk_mt8696_ether_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_ETHER_NR_CLK);

	mtk_clk_register_gates(node, ether_clks, ARRAY_SIZE(ether_clks),
							clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	return r;
}

static const struct of_device_id of_match_clk_mt8696_ether[] = {
	{ .compatible = "mediatek,mt8696-ether", },
	{}
};

static struct platform_driver clk_mt8696_ether_drv = {
	.probe = clk_mt8696_ether_probe,
	.driver = {
		.name = "clk-mt8696-ether",
		.of_match_table = of_match_clk_mt8696_ether,
	},
};

builtin_platform_driver(clk_mt8696_ether_drv);
