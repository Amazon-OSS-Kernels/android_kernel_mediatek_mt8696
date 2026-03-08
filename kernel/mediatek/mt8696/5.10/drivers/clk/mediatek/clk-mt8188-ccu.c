// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Chun-Jie Chen <chun-jie.chen@mediatek.com>

#include "clk-gate.h"
#include "clk-mtk.h"
#include <dt-bindings/clock/mt8188-clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static const struct mtk_gate_regs ccu_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_CCU(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &ccu_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

static const struct mtk_gate ccu_clks[] = {
	GATE_CCU(CLK_CCU_LARB27, "ccu_larb27", "ccu_sel", 0),
	GATE_CCU(CLK_CCU_AHB, "ccu_ahb", "ccu_ahb_sel", 1),
	GATE_CCU(CLK_CCU_CCU0, "ccu_ccu0", "ccu_sel", 2),
};

static const struct mtk_clk_desc ccu_desc = {
	.clks = ccu_clks,
	.num_clks = ARRAY_SIZE(ccu_clks),
};

static const struct of_device_id of_match_clk_mt8188_ccu[] = {
	{
		.compatible = "mediatek,mt8188-ccusys",
		.data = &ccu_desc,
	}, {
		/* sentinel */
	}
};

static struct platform_driver clk_mt8188_ccu_drv = {
	.probe = mtk_clk_simple_probe,
	.driver = {
		.name = "clk-mt8188-ccu",
		.of_match_table = of_match_clk_mt8188_ccu,
	},
};

module_platform_driver(clk_mt8188_ccu_drv);
MODULE_LICENSE("GPL");
