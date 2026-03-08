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

static const struct mtk_gate_regs cam_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_CAM(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &cam_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

static const struct mtk_gate cam_main_clks[] = {
	GATE_CAM(CLK_CAM_MAIN_LARB13, "cam_main_larb13", "cam_sel", 0),
	GATE_CAM(CLK_CAM_MAIN_LARB14, "cam_main_larb14", "cam_sel", 1),
	GATE_CAM(CLK_CAM_MAIN_CAM, "cam_main_cam", "cam_sel", 2),
	GATE_CAM(CLK_CAM_MAIN_CAM_SUBA, "cam_main_cam_suba", "cam_sel", 3),
	GATE_CAM(CLK_CAM_MAIN_CAM_SUBB, "cam_main_cam_subb", "cam_sel", 4),
	GATE_CAM(CLK_CAM_MAIN_CAMTG, "cam_main_camtg", "cam_sel", 7),
	GATE_CAM(CLK_CAM_MAIN_SENINF, "cam_main_seninf", "cam_sel", 8),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVA, "cam_main_gcamsva", "cam_sel", 9),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVB, "cam_main_gcamsvb", "cam_sel", 10),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVC, "cam_main_gcamsvc", "cam_sel", 11),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVD, "cam_main_gcamsvd", "cam_sel", 12),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVE, "cam_main_gcamsve", "cam_sel", 13),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVF, "cam_main_gcamsvf", "cam_sel", 14),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVG, "cam_main_gcamsvg", "cam_sel", 15),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVH, "cam_main_gcamsvh", "cam_sel", 16),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVI, "cam_main_gcamsvi", "cam_sel", 17),
	GATE_CAM(CLK_CAM_MAIN_GCAMSVJ, "cam_main_gcamsvj", "cam_sel", 18),
	GATE_CAM(CLK_CAM_MAIN_CAMSV_TOP, "cam_main_camsv", "cam_sel", 19),
	GATE_CAM(CLK_CAM_MAIN_CAMSV_CQ_A, "cam_main_camsv_cq_a", "cam_sel", 20),
	GATE_CAM(CLK_CAM_MAIN_CAMSV_CQ_B, "cam_main_camsv_cq_b", "cam_sel", 21),
	GATE_CAM(CLK_CAM_MAIN_CAMSV_CQ_C, "cam_main_camsv_cq_c", "cam_sel", 22),
	GATE_CAM(CLK_CAM_MAIN_FAKE_ENG, "cam_main_fake_eng", "cam_sel", 28),
	GATE_CAM(CLK_CAM_MAIN_CAM2MM0_GALS, "cam_main_cam2mm0_gals", "cam_sel", 29),
	GATE_CAM(CLK_CAM_MAIN_CAM2MM1_GALS, "cam_main_cam2mm1_gals", "cam_sel", 30),
	GATE_CAM(CLK_CAM_MAIN_CAM2SYS_GALS, "cam_main_cam2sys_gals", "cam_sel", 31),
};

static const struct mtk_gate cam_rawa_clks[] = {
	GATE_CAM(CLK_CAM_RAWA_LARBX, "cam_rawa_larbx", "cam_sel", 0),
	GATE_CAM(CLK_CAM_RAWA_CAM, "cam_rawa_cam", "cam_sel", 1),
	GATE_CAM(CLK_CAM_RAWA_CAMTG, "cam_rawa_camtg", "cam_sel", 2),
};

static const struct mtk_gate cam_rawb_clks[] = {
	GATE_CAM(CLK_CAM_RAWB_LARBX, "cam_rawb_larbx", "cam_sel", 0),
	GATE_CAM(CLK_CAM_RAWB_CAM, "cam_rawb_cam", "cam_sel", 1),
	GATE_CAM(CLK_CAM_RAWB_CAMTG, "cam_rawb_camtg", "cam_sel", 2),
};

static const struct mtk_gate cam_yuva_clks[] = {
	GATE_CAM(CLK_CAM_YUVA_LARBX, "cam_yuva_larbx", "cam_sel", 0),
	GATE_CAM(CLK_CAM_YUVA_CAM, "cam_yuva_cam", "cam_sel", 1),
	GATE_CAM(CLK_CAM_YUVA_CAMTG, "cam_yuva_camtg", "cam_sel", 2),
};

static const struct mtk_gate cam_yuvb_clks[] = {
	GATE_CAM(CLK_CAM_YUVB_LARBX, "cam_yuvb_larbx", "cam_sel", 0),
	GATE_CAM(CLK_CAM_YUVB_CAM, "cam_yuvb_cam", "cam_sel", 1),
	GATE_CAM(CLK_CAM_YUVB_CAMTG, "cam_yuvb_camtg", "cam_sel", 2),
};

static const struct mtk_clk_desc cam_main_desc = {
	.clks = cam_main_clks,
	.num_clks = ARRAY_SIZE(cam_main_clks),
};

static const struct mtk_clk_desc cam_rawa_desc = {
	.clks = cam_rawa_clks,
	.num_clks = ARRAY_SIZE(cam_rawa_clks),
};

static const struct mtk_clk_desc cam_rawb_desc = {
	.clks = cam_rawb_clks,
	.num_clks = ARRAY_SIZE(cam_rawb_clks),
};

static const struct mtk_clk_desc cam_yuva_desc = {
	.clks = cam_yuva_clks,
	.num_clks = ARRAY_SIZE(cam_yuva_clks),
};

static const struct mtk_clk_desc cam_yuvb_desc = {
	.clks = cam_yuvb_clks,
	.num_clks = ARRAY_SIZE(cam_yuvb_clks),
};

static const struct of_device_id of_match_clk_mt8188_cam[] = {
	{
		.compatible = "mediatek,mt8188-camsys",
		.data = &cam_main_desc,
	}, {
		.compatible = "mediatek,mt8188-camsys_rawa",
		.data = &cam_rawa_desc,
	}, {
		.compatible = "mediatek,mt8188-camsys_rawb",
		.data = &cam_rawb_desc,
	}, {
		.compatible = "mediatek,mt8188-camsys_yuva",
		.data = &cam_yuva_desc,
	}, {
		.compatible = "mediatek,mt8188-camsys_yuvb",
		.data = &cam_yuvb_desc,
	}, {
		/* sentinel */
	}
};

static struct platform_driver clk_mt8188_cam_drv = {
	.probe = mtk_clk_simple_probe,
	.driver = {
		.name = "clk-mt8188-cam",
		.of_match_table = of_match_clk_mt8188_cam,
	},
};

module_platform_driver(clk_mt8188_cam_drv);
MODULE_LICENSE("GPL");
