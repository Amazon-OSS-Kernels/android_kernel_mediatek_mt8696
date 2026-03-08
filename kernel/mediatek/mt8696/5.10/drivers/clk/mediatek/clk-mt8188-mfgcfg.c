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

static const struct mtk_gate_regs mfgcfg_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_MFGCFG(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &mfgcfg_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

static const struct mtk_gate mfgcfg_clks[] = {
	GATE_MFGCFG(CLK_MFGCFG_BG3D, "mfgcfg_bg3d", "mfg_fast_ref_sel", 0),
};

static const struct mtk_clk_desc mfgcfg_desc = {
	.clks = mfgcfg_clks,
	.num_clks = ARRAY_SIZE(mfgcfg_clks),
};

static const struct of_device_id of_match_clk_mt8188_mfgcfg[] = {
	{
		.compatible = "mediatek,mt8188-mfgcfg",
		.data = &mfgcfg_desc,
	}, {
		/* sentinel */
	}
};

static struct platform_driver clk_mt8188_mfgcfg_drv = {
	.probe = mtk_clk_simple_probe,
	.driver = {
		.name = "clk-mt8188-mfgcfg",
		.of_match_table = of_match_clk_mt8188_mfgcfg,
	},
};

module_platform_driver(clk_mt8188_mfgcfg_drv);
MODULE_LICENSE("GPL");
