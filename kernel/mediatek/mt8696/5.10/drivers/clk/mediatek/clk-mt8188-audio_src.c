// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Chun-Jie Chen <chun-jie.chen@mediatek.com>

#include "clk-gate.h"
#include "clk-mtk.h"
#include <dt-bindings/clock/mt8188-clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

static const struct mtk_gate_regs audio_src_cg_regs = {
	.set_ofs = 0x1004,
	.clr_ofs = 0x1004,
	.sta_ofs = 0x1004,
};

#define GATE_AUDIO_SRC(_id, _name, _parent, _shift)			\
	GATE_MTK_FLAGS(_id, _name, _parent, &audio_src_cg_regs, _shift, \
		       &mtk_clk_gate_ops_no_setclr, CLK_IGNORE_UNUSED)

static const struct mtk_gate audio_src_clks[] = {
	GATE_AUDIO_SRC(CLK_AUDIO_SRC_ASRC0, "audio_src_asrc0", "asm_h_sel", 0),
	GATE_AUDIO_SRC(CLK_AUDIO_SRC_ASRC1, "audio_src_asrc1", "asm_h_sel", 1),
	GATE_AUDIO_SRC(CLK_AUDIO_SRC_ASRC2, "audio_src_asrc2", "asm_h_sel", 2),
	GATE_AUDIO_SRC(CLK_AUDIO_SRC_ASRC3, "audio_src_asrc3", "asm_h_sel", 3),
};

static int clk_mt8188_aud_src_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	struct device_node *node = pdev->dev.of_node;
	int r;

	clk_data = mtk_alloc_clk_data(ARRAY_SIZE(audio_src_clks));
	if (!clk_data)
		return -ENOMEM;

	r = mtk_clk_register_gates(node, audio_src_clks, ARRAY_SIZE(audio_src_clks), clk_data);
	if (r)
		goto err_register_gates;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		goto err_clk_provider;

	r = devm_of_platform_populate(&pdev->dev);
	if (r)
		goto err_plat_populate;

	return 0;

err_plat_populate:
	of_clk_del_provider(node);
err_clk_provider:
err_register_gates:
	mtk_free_clk_data(clk_data);
	return r;
}

static const struct mtk_clk_desc audio_src_desc = {
	.clks = audio_src_clks,
	.num_clks = ARRAY_SIZE(audio_src_clks),
};

static const struct of_device_id of_match_clk_mt8188_audio_src[] = {
	{
		.compatible = "mediatek,mt8188-audsys_src",
		.data = &audio_src_desc,
	}, {
		/* sentinel */
	}
};

static struct platform_driver clk_mt8188_audio_src_drv = {
	.probe = clk_mt8188_aud_src_probe,
	.driver = {
		.name = "clk-mt8188-audio_src",
		.of_match_table = of_match_clk_mt8188_audio_src,
	},
};

module_platform_driver(clk_mt8188_audio_src_drv);
MODULE_LICENSE("GPL");
