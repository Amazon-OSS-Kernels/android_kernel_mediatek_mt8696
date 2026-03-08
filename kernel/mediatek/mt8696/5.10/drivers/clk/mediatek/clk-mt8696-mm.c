// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Qiqi Wang <qiqi.wang@mediatek.com>

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt8696-clk.h>

static const struct mtk_gate_regs mm0_cg_regs = {
	.set_ofs = 0x104,
	.clr_ofs = 0x108,
	.sta_ofs = 0x100,
};

static const struct mtk_gate_regs mm1_cg_regs = {
	.set_ofs = 0x114,
	.clr_ofs = 0x118,
	.sta_ofs = 0x110,
};

#define GATE_MM0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mm0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_MM1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mm1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate mm_clks[] = {
	/* MM0 */
	GATE_MM0(CLK_MMCG0_SMI_COM, "mmcg0_smi_com", "mmsel", 0),
	GATE_MM0(CLK_MMCG1_SMI_L0, "mmcg1_smi_l0", "mmsel", 1),
	GATE_MM0(CLK_MMCG2_DRAM_L, "mmcg2_dram_l", "mmsel", 2),
	GATE_MM0(CLK_MMCG3_FAKEG, "mmcg3_fakeg", "mmsel", 3),
	GATE_MM0(CLK_MMCG4_SMI_L4, "mmcg4_smi_l4", "mmsel", 4),
	GATE_MM0(CLK_MMCG5_SMI_L1, "mmcg5_smi_l1", "mmsel", 5),
	GATE_MM0(CLK_MMCG6_SMI_L5, "mmcg6_smi_l5", "mmsel", 6),
	GATE_MM0(CLK_MMCG7_SMI_L6, "mmcg7_smi_l6", "mmsel", 7),
	GATE_MM0(CLK_MMCG8_SMI_L7, "mmcg8_smi_l7", "mmsel", 8),
	GATE_MM0(CLK_MMCG9_VDEC2IMG, "mmcg9_vdec2img", "mmsel", 9),
	GATE_MM0(CLK_MMCG10_VDOUT_MM, "mmcg10_vdout_mm", "mmsel", 10),
	GATE_MM0(CLK_MMCG11_SMI_L3, "mmcg11_smi_l3", "mmsel", 11),
	GATE_MM0(CLK_MMCG13_FMTTER, "mmcg13_fmtter", "mmsel", 13),
	GATE_MM0(CLK_MMCG14_DISP_MIX, "mmcg14_disp_mix", "mmsel", 14),
	GATE_MM0(CLK_MMCG15_OSD_FBDC, "mmcg15_osd_fbdc", "mmsel", 15),
	GATE_MM0(CLK_MMCG16_OF, "mmcg16_of", "osd_sel", 16),
	GATE_MM0(CLK_MMCG17_OF_FIFO, "mmcg17_of_fifo", "mmsel", 17),
	GATE_MM0(CLK_MMCG18_OF_HDR, "mmcg18_of_hdr", "mmsel", 18),
	GATE_MM0(CLK_MMCG19_OF_SI2MI, "mmcg19_of_si2mi", "mmsel", 19),
	GATE_MM0(CLK_MMCG20_HDR_ADL, "mmcg20_hdr_adl", "mmsel", 20),
	GATE_MM0(CLK_MMCG21_HDR_FIFO, "mmcg21_hdr_fifo", "mmsel", 21),
	GATE_MM0(CLK_MMCG22_HDR_VDO, "mmcg22_hdr_vdo", "mmsel", 22),
	GATE_MM0(CLK_MMCG23_RGB2HDMI, "mmcg23_rgb2hdmi", "mmsel", 23),
	GATE_MM0(CLK_MMCG25_DGI, "mmcg25_dgi", "mmsel", 25),
	/* MM1 */
	GATE_MM1(CLK_MMCG0_SD_PPF, "mmcg0_sd_ppf", "sd_sel", 0),
	GATE_MM1(CLK_MMCG1_P2I, "mmcg1_p2i", "mmsel", 1),
	GATE_MM1(CLK_MMCG2_VM_TOP, "mmcg2_vm_top", "mmsel", 2),
	GATE_MM1(CLK_MMCG3_HDMI_MD, "mmcg3_hdmi_md", "mmsel", 3),
	GATE_MM1(CLK_MMCG4_VIDEO_IN, "mmcg4_video_in", "mmsel", 4),
	GATE_MM1(CLK_MMCG5_W2D, "mmcg5_w2d", "mmsel", 5),
	GATE_MM1(CLK_MMCG7_OU, "mmcg7_ou", "osd_sel", 12),
	GATE_MM1(CLK_MMCG8_OU_FIFO, "mmcg8_ou_fifo", "mmsel", 13),
	GATE_MM1(CLK_MMCG9_OU_HDR, "mmcg9_ou_hdr", "mmsel", 14),
	GATE_MM1(CLK_MMCG12_SMI_L0, "mmcg12_smi_l0", "mmsel", 17),
};

static int clk_mt8696_mm_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_MM_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	r = mtk_clk_register_gates(node, mm_clks, ARRAY_SIZE(mm_clks), clk_data);
	if (r)
		goto free_mm_clk_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r) {
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
		goto free_mm_clk_data;
	}

	return r;

free_mm_clk_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static const struct of_device_id of_match_clk_mt8696_mm[] = {
	{ .compatible = "mediatek,mt8696-mmsys", },
	{}
};

static struct platform_driver clk_mt8696_mm_drv = {
	.probe = clk_mt8696_mm_probe,
	.driver = {
		.name = "clk-mt8696-mm",
		.of_match_table = of_match_clk_mt8696_mm,
	},
};

static int __init clk_mt8696_mm_init(void)
{
	return platform_driver_register(&clk_mt8696_mm_drv);
}

arch_initcall(clk_mt8696_mm_init);

