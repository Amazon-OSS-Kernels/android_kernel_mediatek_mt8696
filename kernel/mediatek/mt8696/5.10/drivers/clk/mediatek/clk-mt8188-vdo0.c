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

static const struct mtk_gate_regs vdo0_0_cg_regs = {
	.set_ofs = 0x104,
	.clr_ofs = 0x108,
	.sta_ofs = 0x100,
};

static const struct mtk_gate_regs vdo0_1_cg_regs = {
	.set_ofs = 0x114,
	.clr_ofs = 0x118,
	.sta_ofs = 0x110,
};

static const struct mtk_gate_regs vdo0_2_cg_regs = {
	.set_ofs = 0x124,
	.clr_ofs = 0x128,
	.sta_ofs = 0x120,
};

#define GATE_VDO0_0(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &vdo0_0_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

#define GATE_VDO0_1(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &vdo0_1_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

#define GATE_VDO0_2(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &vdo0_2_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

static const struct mtk_gate vdo0_clks[] = {
	/* VDO0_0 */
	GATE_VDO0_0(CLK_VDO0_DISP_OVL0, "vdo0_disp_ovl0", "vpp_sel", 0),
	GATE_VDO0_0(CLK_VDO0_FAKE_ENG0, "vdo0_fake_eng0", "vpp_sel", 2),
	GATE_VDO0_0(CLK_VDO0_DISP_CCORR0, "vdo0_disp_ccorr0", "vpp_sel", 4),
	GATE_VDO0_0(CLK_VDO0_DISP_MUTEX0, "vdo0_disp_mutex0", "vpp_sel", 6),
	GATE_VDO0_0(CLK_VDO0_DISP_GAMMA0, "vdo0_disp_gamma0", "vpp_sel", 8),
	GATE_VDO0_0(CLK_VDO0_DISP_DITHER0, "vdo0_disp_dither0", "vpp_sel", 10),
	GATE_VDO0_0(CLK_VDO0_DISP_WDMA0, "vdo0_disp_wdma0", "vpp_sel", 17),
	GATE_VDO0_0(CLK_VDO0_DISP_RDMA0, "vdo0_disp_rdma0", "vpp_sel", 19),
	GATE_VDO0_0(CLK_VDO0_DSI0, "vdo0_dsi0", "vpp_sel", 21),
	GATE_VDO0_0(CLK_VDO0_DSI1, "vdo0_dsi1", "vpp_sel", 22),
	GATE_VDO0_0(CLK_VDO0_DSC_WRAP0, "vdo0_dsc_wrap0", "vpp_sel", 23),
	GATE_VDO0_0(CLK_VDO0_VPP_MERGE0, "vdo0_vpp_merge0", "vpp_sel", 24),
	GATE_VDO0_0(CLK_VDO0_DP_INTF0, "vdo0_dp_intf0", "vpp_sel", 25),
	GATE_VDO0_0(CLK_VDO0_DISP_AAL0, "vdo0_disp_aal0", "vpp_sel", 26),
	GATE_VDO0_0(CLK_VDO0_INLINEROT0, "vdo0_inlinerot0", "vpp_sel", 27),
	GATE_VDO0_0(CLK_VDO0_APB_BUS, "vdo0_apb_bus", "vpp_sel", 28),
	GATE_VDO0_0(CLK_VDO0_DISP_COLOR0, "vdo0_disp_color0", "vpp_sel", 29),
	GATE_VDO0_0(CLK_VDO0_MDP_WROT0, "vdo0_mdp_wrot0", "vpp_sel", 30),
	GATE_VDO0_0(CLK_VDO0_DISP_RSZ0, "vdo0_disp_rsz0", "vpp_sel", 31),
	/* VDO0_1 */
	GATE_VDO0_1(CLK_VDO0_DISP_POSTMASK0, "vdo0_disp_postmask0", "vpp_sel", 0),
	GATE_VDO0_1(CLK_VDO0_FAKE_ENG1, "vdo0_fake_eng1", "vpp_sel", 1),
	GATE_VDO0_1(CLK_VDO0_DL_ASYNC2, "vdo0_dl_async2", "vpp_sel", 5),
	GATE_VDO0_1(CLK_VDO0_DL_RELAY3, "vdo0_dl_relay3", "vpp_sel", 6),
	GATE_VDO0_1(CLK_VDO0_DL_RELAY4, "vdo0_dl_relay4", "vpp_sel", 7),
	GATE_VDO0_1(CLK_VDO0_SMI_GALS, "vdo0_smi_gals", "vpp_sel", 10),
	GATE_VDO0_1(CLK_VDO0_SMI_COMMON, "vdo0_smi_common", "vpp_sel", 11),
	GATE_VDO0_1(CLK_VDO0_SMI_EMI, "vdo0_smi_emi", "vpp_sel", 12),
	GATE_VDO0_1(CLK_VDO0_SMI_IOMMU, "vdo0_smi_iommu", "vpp_sel", 13),
	GATE_VDO0_1(CLK_VDO0_SMI_LARB, "vdo0_smi_larb", "vpp_sel", 14),
	GATE_VDO0_1(CLK_VDO0_SMI_RSI, "vdo0_smi_rsi", "vpp_sel", 15),
	/* VDO0_2 */
	GATE_VDO0_2(CLK_VDO0_DSI0_DSI, "vdo0_dsi0_dsi", "dsi_phy", 0),
	GATE_VDO0_2(CLK_VDO0_DSI1_DSI, "vdo0_dsi1_dsi", "dsi_phy", 8),
	GATE_VDO0_2(CLK_VDO0_DP_INTF0_DP_INTF, "vdo0_dp_intf0_dp_intf", "edp_sel", 16),
};

static const struct mtk_clk_desc vdo0_desc = {
	.clks = vdo0_clks,
	.num_clks = ARRAY_SIZE(vdo0_clks),
};

static const struct of_device_id of_match_clk_mt8188_vdo0[] = {
	{
		.compatible = "mediatek,mt8188-vdosys0",
		.data = &vdo0_desc,
	}, {
		/* sentinel */
	}
};

static struct platform_driver clk_mt8188_vdo0_drv = {
	.probe = mtk_clk_simple_probe,
	.driver = {
		.name = "clk-mt8188-vdo0",
		.of_match_table = of_match_clk_mt8188_vdo0,
	},
};

module_platform_driver(clk_mt8188_vdo0_drv);
MODULE_LICENSE("GPL");
