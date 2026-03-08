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

static const struct mtk_gate_regs vdo1_0_cg_regs = {
	.set_ofs = 0x104,
	.clr_ofs = 0x108,
	.sta_ofs = 0x100,
};

static const struct mtk_gate_regs vdo1_1_cg_regs = {
	.set_ofs = 0x114,
	.clr_ofs = 0x118,
	.sta_ofs = 0x110,
};

static const struct mtk_gate_regs vdo1_2_cg_regs = {
	.set_ofs = 0x124,
	.clr_ofs = 0x128,
	.sta_ofs = 0x120,
};

static const struct mtk_gate_regs vdo1_3_cg_regs = {
	.set_ofs = 0x134,
	.clr_ofs = 0x138,
	.sta_ofs = 0x130,
};

static const struct mtk_gate_regs vdo1_4_cg_regs = {
	.set_ofs = 0x144,
	.clr_ofs = 0x148,
	.sta_ofs = 0x140,
};

#define GATE_VDO1_0(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &vdo1_0_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

#define GATE_VDO1_1(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &vdo1_1_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

#define GATE_VDO1_2(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &vdo1_2_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

#define GATE_VDO1_3(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &vdo1_3_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

#define GATE_VDO1_4(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &vdo1_4_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

static const struct mtk_gate vdo1_clks[] = {
	/* VDO1_0 */
	GATE_VDO1_0(CLK_VDO1_SMI_LARB2, "vdo1_smi_larb2", "vpp_sel", 0),
	GATE_VDO1_0(CLK_VDO1_SMI_LARB3, "vdo1_smi_larb3", "vpp_sel", 1),
	GATE_VDO1_0(CLK_VDO1_GALS, "vdo1_gals", "vpp_sel", 2),
	GATE_VDO1_0(CLK_VDO1_FAKE_ENG0, "vdo1_fake_eng0", "vpp_sel", 3),
	GATE_VDO1_0(CLK_VDO1_FAKE_ENG1, "vdo1_fake_eng1", "vpp_sel", 4),
	GATE_VDO1_0(CLK_VDO1_MDP_RDMA0, "vdo1_mdp_rdma0", "vpp_sel", 5),
	GATE_VDO1_0(CLK_VDO1_MDP_RDMA1, "vdo1_mdp_rdma1", "vpp_sel", 6),
	GATE_VDO1_0(CLK_VDO1_MDP_RDMA2, "vdo1_mdp_rdma2", "vpp_sel", 7),
	GATE_VDO1_0(CLK_VDO1_MDP_RDMA3, "vdo1_mdp_rdma3", "vpp_sel", 8),
	GATE_VDO1_0(CLK_VDO1_VPP_MERGE0, "vdo1_vpp_merge0", "vpp_sel", 9),
	GATE_VDO1_0(CLK_VDO1_VPP_MERGE1, "vdo1_vpp_merge1", "vpp_sel", 10),
	GATE_VDO1_0(CLK_VDO1_VPP_MERGE2, "vdo1_vpp_merge2", "vpp_sel", 11),
	/* VDO1_1 */
	GATE_VDO1_1(CLK_VDO1_VPP_MERGE3, "vdo1_vpp_merge3", "vpp_sel", 0),
	GATE_VDO1_1(CLK_VDO1_VPP_MERGE4, "vdo1_vpp_merge4", "vpp_sel", 1),
	GATE_VDO1_1(CLK_VDO1_VPP2_TO_VDO1_DL_ASYNC, "vdo1_vpp2_to_vdo1_dl_async", "vpp_sel", 2),
	GATE_VDO1_1(CLK_VDO1_VPP3_TO_VDO1_DL_ASYNC, "vdo1_vpp3_to_vdo1_dl_async", "vpp_sel", 3),
	GATE_VDO1_1(CLK_VDO1_DISP_MUTEX, "vdo1_disp_mutex", "vpp_sel", 4),
	GATE_VDO1_1(CLK_VDO1_MDP_RDMA4, "vdo1_mdp_rdma4", "vpp_sel", 5),
	GATE_VDO1_1(CLK_VDO1_MDP_RDMA5, "vdo1_mdp_rdma5", "vpp_sel", 6),
	GATE_VDO1_1(CLK_VDO1_MDP_RDMA6, "vdo1_mdp_rdma6", "vpp_sel", 7),
	GATE_VDO1_1(CLK_VDO1_MDP_RDMA7, "vdo1_mdp_rdma7", "vpp_sel", 8),
	GATE_VDO1_1(CLK_VDO1_DP_INTF0_MMCK, "vdo1_dp_intf0_mmck", "vpp_sel", 9),
	GATE_VDO1_1(CLK_VDO1_DPI0_MM, "vdo1_dpi0_mm_ck", "vpp_sel", 10),
	GATE_VDO1_1(CLK_VDO1_DPI1_MM, "vdo1_dpi1_mm_ck", "vpp_sel", 11),
	GATE_VDO1_1(CLK_VDO1_MERGE0_DL_ASYNC, "vdo1_merge0_dl_async", "vpp_sel", 13),
	GATE_VDO1_1(CLK_VDO1_MERGE1_DL_ASYNC, "vdo1_merge1_dl_async", "vpp_sel", 14),
	GATE_VDO1_1(CLK_VDO1_MERGE2_DL_ASYNC, "vdo1_merge2_dl_async", "vpp_sel", 15),
	GATE_VDO1_1(CLK_VDO1_MERGE3_DL_ASYNC, "vdo1_merge3_dl_async", "vpp_sel", 16),
	GATE_VDO1_1(CLK_VDO1_MERGE4_DL_ASYNC, "vdo1_merge4_dl_async", "vpp_sel", 17),
	GATE_VDO1_1(CLK_VDO1_DSC_VDO1_DL_ASYNC, "vdo1_dsc_vdo1_dl_async", "vpp_sel", 18),
	GATE_VDO1_1(CLK_VDO1_MERGE_VDO1_DL_ASYNC, "vdo1_merge_vdo1_dl_async", "vpp_sel", 19),
	GATE_VDO1_1(CLK_VDO1_PADDING0, "vdo1_padding0", "vpp_sel", 20),
	GATE_VDO1_1(CLK_VDO1_PADDING1, "vdo1_padding1", "vpp_sel", 21),
	GATE_VDO1_1(CLK_VDO1_PADDING2, "vdo1_padding2", "vpp_sel", 22),
	GATE_VDO1_1(CLK_VDO1_PADDING3, "vdo1_padding3", "vpp_sel", 23),
	GATE_VDO1_1(CLK_VDO1_PADDING4, "vdo1_padding4", "vpp_sel", 24),
	GATE_VDO1_1(CLK_VDO1_PADDING5, "vdo1_padding5", "vpp_sel", 25),
	GATE_VDO1_1(CLK_VDO1_PADDING6, "vdo1_padding6", "vpp_sel", 26),
	GATE_VDO1_1(CLK_VDO1_PADDING7, "vdo1_padding7", "vpp_sel", 27),
	GATE_VDO1_1(CLK_VDO1_DISP_RSZ0, "vdo1_disp_rsz0", "vpp_sel", 28),
	GATE_VDO1_1(CLK_VDO1_DISP_RSZ1, "vdo1_disp_rsz1", "vpp_sel", 29),
	GATE_VDO1_1(CLK_VDO1_DISP_RSZ2, "vdo1_disp_rsz2", "vpp_sel", 30),
	GATE_VDO1_1(CLK_VDO1_DISP_RSZ3, "vdo1_disp_rsz3", "vpp_sel", 31),
	/* VDO1_2 */
	GATE_VDO1_2(CLK_VDO1_HDR_VDO_FE0, "vdo1_hdr_vdo_fe0", "vpp_sel", 0),
	GATE_VDO1_2(CLK_VDO1_HDR_GFX_FE0, "vdo1_hdr_gfx_fe0", "vpp_sel", 1),
	GATE_VDO1_2(CLK_VDO1_HDR_VDO_BE, "vdo1_hdr_vdo_be", "vpp_sel", 2),
	GATE_VDO1_2(CLK_VDO1_HDR_VDO_FE1, "vdo1_hdr_vdo_fe1", "vpp_sel", 16),
	GATE_VDO1_2(CLK_VDO1_HDR_GFX_FE1, "vdo1_hdr_gfx_fe1", "vpp_sel", 17),
	GATE_VDO1_2(CLK_VDO1_DISP_MIXER, "vdo1_disp_mixer", "vpp_sel", 18),
	GATE_VDO1_2(CLK_VDO1_HDR_VDO_FE0_DL_ASYNC, "vdo1_hdr_vdo_fe0_dl_async", "vpp_sel", 19),
	GATE_VDO1_2(CLK_VDO1_HDR_VDO_FE1_DL_ASYNC, "vdo1_hdr_vdo_fe1_dl_async", "vpp_sel", 20),
	GATE_VDO1_2(CLK_VDO1_HDR_GFX_FE0_DL_ASYNC, "vdo1_hdr_gfx_fe0_dl_async", "vpp_sel", 21),
	GATE_VDO1_2(CLK_VDO1_HDR_GFX_FE1_DL_ASYNC, "vdo1_hdr_gfx_fe1_dl_async", "vpp_sel", 22),
	GATE_VDO1_2(CLK_VDO1_HDR_VDO_BE_DL_ASYNC, "vdo1_hdr_vdo_be_dl_async", "vpp_sel", 23),
	/* VDO1_3 */
	GATE_VDO1_3(CLK_VDO1_DPI0, "vdo1_dpi0_ck", "vpp_sel", 0),
	GATE_VDO1_3(CLK_VDO1_DISP_MONITOR_DPI0, "vdo1_disp_monitor_dpi0_ck", "vpp_sel", 1),
	GATE_VDO1_3(CLK_VDO1_DPI1, "vdo1_dpi1_ck", "vpp_sel", 8),
	GATE_VDO1_3(CLK_VDO1_DISP_MONITOR_DPI1, "vdo1_disp_monitor_dpi1_ck", "vpp_sel", 9),
	GATE_VDO1_3(CLK_VDO1_DPINTF, "vdo1_dpintf_ck", "vpp_sel", 16),
	GATE_VDO1_3(CLK_VDO1_DISP_MONITOR_DPINTF, "vdo1_disp_monitor_dpintf_ck", "vpp_sel", 17),
	/* VDO1_4 */
	GATE_VDO1_4(CLK_VDO1_26M_SLOW, "vdo1_26m_slow_ck", "clk26m", 8),
};

static const struct mtk_clk_desc vdo1_desc = {
	.clks = vdo1_clks,
	.num_clks = ARRAY_SIZE(vdo1_clks),
};

static const struct of_device_id of_match_clk_mt8188_vdo1[] = {
	{
		.compatible = "mediatek,mt8188-vdosys1",
		.data = &vdo1_desc,
	}, {
		/* sentinel */
	}
};

static struct platform_driver clk_mt8188_vdo1_drv = {
	.probe = mtk_clk_simple_probe,
	.driver = {
		.name = "clk-mt8188-vdo1",
		.of_match_table = of_match_clk_mt8188_vdo1,
	},
};

module_platform_driver(clk_mt8188_vdo1_drv);
MODULE_LICENSE("GPL");
