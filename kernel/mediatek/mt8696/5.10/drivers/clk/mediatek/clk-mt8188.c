// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Chun-Jie Chen <chun-jie.chen@mediatek.com>

#include "clk-gate.h"
#include "clk-mtk.h"
#include "clk-mux.h"
#include <dt-bindings/clock/mt8188-clk.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

static DEFINE_SPINLOCK(mt8188_clk_lock);

static const struct mtk_fixed_clk top_fixed_clks[] = {
	FIXED_CLK(CLK_TOP_ULPOSC, "ulposc_ck", NULL, 260000000),
	FIXED_CLK(CLK_TOP_MPHONE_SLAVE_BCK, "mphone_slave_bck", NULL, 49152000),
	FIXED_CLK(CLK_TOP_PAD_FPC, "pad_fpc_ck", NULL, 50000000),
	FIXED_CLK(CLK_TOP_466M_FMEM, "hd_466m_fmem_ck", NULL, 533000000),
	FIXED_CLK(CLK_TOP_PEXTP_PIPE, "pextp_pipe", NULL, 250000000),
	FIXED_CLK(CLK_TOP_DSI_PHY, "dsi_phy", NULL, 500000000),
};

static const struct mtk_fixed_factor top_divs[] = {
	FACTOR(CLK_TOP_MFGPLL, "mfgpll_ck", "mfgpll", 1, 1),
	FACTOR(CLK_TOP_MAINPLL, "mainpll_ck", "mainpll", 1, 1),
	FACTOR(CLK_TOP_MAINPLL_D3, "mainpll_d3", "mainpll_ck", 1, 3),
	FACTOR(CLK_TOP_MAINPLL_D4, "mainpll_d4", "mainpll_ck", 1, 4),
	FACTOR(CLK_TOP_MAINPLL_D4_D2, "mainpll_d4_d2", "mainpll_d4", 1, 2),
	FACTOR(CLK_TOP_MAINPLL_D4_D4, "mainpll_d4_d4", "mainpll_d4", 1, 4),
	FACTOR(CLK_TOP_MAINPLL_D4_D8, "mainpll_d4_d8", "mainpll_d4", 1, 8),
	FACTOR(CLK_TOP_MAINPLL_D5, "mainpll_d5", "mainpll_ck", 1, 5),
	FACTOR(CLK_TOP_MAINPLL_D5_D2, "mainpll_d5_d2", "mainpll_d5", 1, 2),
	FACTOR(CLK_TOP_MAINPLL_D5_D4, "mainpll_d5_d4", "mainpll_d5", 1, 4),
	FACTOR(CLK_TOP_MAINPLL_D5_D8, "mainpll_d5_d8", "mainpll_d5", 1, 8),
	FACTOR(CLK_TOP_MAINPLL_D6, "mainpll_d6", "mainpll_ck", 1, 6),
	FACTOR(CLK_TOP_MAINPLL_D6_D2, "mainpll_d6_d2", "mainpll_d6", 1, 2),
	FACTOR(CLK_TOP_MAINPLL_D6_D4, "mainpll_d6_d4", "mainpll_d6", 1, 4),
	FACTOR(CLK_TOP_MAINPLL_D6_D8, "mainpll_d6_d8", "mainpll_d6", 1, 8),
	FACTOR(CLK_TOP_MAINPLL_D7, "mainpll_d7", "mainpll_ck", 1, 7),
	FACTOR(CLK_TOP_MAINPLL_D7_D2, "mainpll_d7_d2", "mainpll_d7", 1, 2),
	FACTOR(CLK_TOP_MAINPLL_D7_D4, "mainpll_d7_d4", "mainpll_d7", 1, 4),
	FACTOR(CLK_TOP_MAINPLL_D7_D8, "mainpll_d7_d8", "mainpll_d7", 1, 8),
	FACTOR(CLK_TOP_MAINPLL_D9, "mainpll_d9", "mainpll_ck", 1, 9),
	FACTOR(CLK_TOP_UNIVPLL, "univpll_ck", "univpll", 1, 1),
	FACTOR(CLK_TOP_UNIVPLL_D2, "univpll_d2", "univpll_ck", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D3, "univpll_d3", "univpll_ck", 1, 3),
	FACTOR(CLK_TOP_UNIVPLL_D4, "univpll_d4", "univpll_ck", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL_D4_D2, "univpll_d4_d2", "univpll_d4", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D4_D4, "univpll_d4_d4", "univpll_d4", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL_D4_D8, "univpll_d4_d8", "univpll_d4", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL_D5, "univpll_d5", "univpll_ck", 1, 5),
	FACTOR(CLK_TOP_UNIVPLL_D5_D2, "univpll_d5_d2", "univpll_d5", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D5_D4, "univpll_d5_d4", "univpll_d5", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL_D5_D8, "univpll_d5_d8", "univpll_d5", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL_D6, "univpll_d6", "univpll_ck", 1, 6),
	FACTOR(CLK_TOP_UNIVPLL_D6_D2, "univpll_d6_d2", "univpll_d6", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D6_D4, "univpll_d6_d4", "univpll_d6", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL_D6_D8, "univpll_d6_d8", "univpll_d6", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL_D7, "univpll_d7", "univpll_ck", 1, 7),
	FACTOR(CLK_TOP_UNIVPLL_192M, "univpll_192m", "univpll_ck", 1, 13),
	FACTOR(CLK_TOP_UNIVPLL_192M_D4, "univpll_192m_d4", "univpll_192m", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL_192M_D8, "univpll_192m_d8", "univpll_192m", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL_192M_D10, "univpll_192m_d10", "univpll_192m", 1, 10),
	FACTOR(CLK_TOP_UNIVPLL_192M_D16, "univpll_192m_d16", "univpll_192m", 1, 16),
	FACTOR(CLK_TOP_UNIVPLL_192M_D32, "univpll_192m_d32", "univpll_192m", 1, 32),
	FACTOR(CLK_TOP_IMGPLL, "imgpll_ck", "imgpll", 1, 1),
	FACTOR(CLK_TOP_APLL1, "apll1_ck", "apll1", 1, 1),
	FACTOR(CLK_TOP_APLL1_D3, "apll1_d3", "apll1_ck", 1, 3),
	FACTOR(CLK_TOP_APLL1_D4, "apll1_d4", "apll1_ck", 1, 4),
	FACTOR(CLK_TOP_APLL2, "apll2_ck", "apll2", 1, 1),
	FACTOR(CLK_TOP_APLL2_D3, "apll2_d3", "apll2_ck", 1, 3),
	FACTOR(CLK_TOP_APLL2_D4, "apll2_d4", "apll2_ck", 1, 4),
	FACTOR(CLK_TOP_APLL3, "apll3_ck", "apll3", 1, 1),
	FACTOR(CLK_TOP_APLL3_D4, "apll3_d4", "apll3_ck", 1, 4),
	FACTOR(CLK_TOP_APLL4, "apll4_ck", "apll4", 1, 1),
	FACTOR(CLK_TOP_APLL4_D4, "apll4_d4", "apll4_ck", 1, 4),
	FACTOR(CLK_TOP_APLL5, "apll5_ck", "apll5", 1, 1),
	FACTOR(CLK_TOP_APLL5_D4, "apll5_d4", "apll5_ck", 1, 4),
	FACTOR(CLK_TOP_MMPLL, "mmpll_ck", "mmpll", 1, 1),
	FACTOR(CLK_TOP_MMPLL_D4, "mmpll_d4", "mmpll_ck", 1, 4),
	FACTOR(CLK_TOP_MMPLL_D4_D2, "mmpll_d4_d2", "mmpll_d4", 1, 2),
	FACTOR(CLK_TOP_MMPLL_D5, "mmpll_d5", "mmpll_ck", 1, 5),
	FACTOR(CLK_TOP_MMPLL_D5_D2, "mmpll_d5_d2", "mmpll_d5", 1, 2),
	FACTOR(CLK_TOP_MMPLL_D5_D4, "mmpll_d5_d4", "mmpll_d5", 1, 4),
	FACTOR(CLK_TOP_MMPLL_D6, "mmpll_d6", "mmpll_ck", 1, 6),
	FACTOR(CLK_TOP_MMPLL_D6_D2, "mmpll_d6_d2", "mmpll_d6", 1, 2),
	FACTOR(CLK_TOP_MMPLL_D7, "mmpll_d7", "mmpll_ck", 1, 7),
	FACTOR(CLK_TOP_MMPLL_D9, "mmpll_d9", "mmpll_ck", 1, 9),
	FACTOR(CLK_TOP_TVDPLL1, "tvdpll1_ck", "tvdpll1", 1, 1),
	FACTOR(CLK_TOP_TVDPLL1_D2, "tvdpll1_d2", "tvdpll1_ck", 1, 2),
	FACTOR(CLK_TOP_TVDPLL1_D4, "tvdpll1_d4", "tvdpll1_ck", 1, 4),
	FACTOR(CLK_TOP_TVDPLL1_D8, "tvdpll1_d8", "tvdpll1_ck", 1, 8),
	FACTOR(CLK_TOP_TVDPLL1_D16, "tvdpll1_d16", "tvdpll1_ck", 1, 16),
	FACTOR(CLK_TOP_TVDPLL2, "tvdpll2_ck", "tvdpll2", 1, 1),
	FACTOR(CLK_TOP_TVDPLL2_D2, "tvdpll2_d2", "tvdpll2_ck", 1, 2),
	FACTOR(CLK_TOP_TVDPLL2_D4, "tvdpll2_d4", "tvdpll2_ck", 1, 4),
	FACTOR(CLK_TOP_TVDPLL2_D8, "tvdpll2_d8", "tvdpll2_ck", 1, 8),
	FACTOR(CLK_TOP_TVDPLL2_D16, "tvdpll2_d16", "tvdpll2_ck", 1, 16),
	FACTOR(CLK_TOP_MSDCPLL, "msdcpll_ck", "msdcpll", 1, 1),
	FACTOR(CLK_TOP_MSDCPLL_D2, "msdcpll_d2", "msdcpll_ck", 1, 2),
	FACTOR(CLK_TOP_MSDCPLL_D16, "msdcpll_d16", "msdcpll_ck", 1, 16),
	FACTOR(CLK_TOP_ETHPLL, "ethpll_ck", "ethpll", 1, 1),
	FACTOR(CLK_TOP_ETHPLL_D2, "ethpll_d2", "ethpll_ck", 1, 2),
	FACTOR(CLK_TOP_ETHPLL_D4, "ethpll_d4", "ethpll_ck", 1, 4),
	FACTOR(CLK_TOP_ETHPLL_D8, "ethpll_d8", "ethpll_ck", 1, 8),
	FACTOR(CLK_TOP_ETHPLL_D10, "ethpll_d10", "ethpll_ck", 1, 10),
	FACTOR(CLK_TOP_ADSPPLL, "adsppll_ck", "adsppll", 1, 1),
	FACTOR(CLK_TOP_ADSPPLL_D2, "adsppll_d2", "adsppll_ck", 1, 2),
	FACTOR(CLK_TOP_ADSPPLL_D4, "adsppll_d4", "adsppll_ck", 1, 4),
	FACTOR(CLK_TOP_ADSPPLL_D8, "adsppll_d8", "adsppll_ck", 1, 8),
	FACTOR(CLK_TOP_ULPOSC_D2, "ulposc_d2", "ulposc_ck", 1, 2),
	FACTOR(CLK_TOP_ULPOSC_D4, "ulposc_d4", "ulposc_ck", 1, 4),
	FACTOR(CLK_TOP_ULPOSC_D8, "ulposc_d8", "ulposc_ck", 1, 8),
	FACTOR(CLK_TOP_ULPOSC_D7, "ulposc_d7", "ulposc_ck", 1, 7),
	FACTOR(CLK_TOP_ULPOSC_D10, "ulposc_d10", "ulposc_ck", 1, 10),
	FACTOR(CLK_TOP_ULPOSC_D16, "ulposc_d16", "ulposc_ck", 1, 16),
};

static const char * const axi_parents[] = {
	"clk26m",
	"mainpll_d4_d4",
	"mainpll_d7_d2",
	"mainpll_d4_d2",
	"mainpll_d5_d2",
	"mainpll_d6_d2",
	"ulposc_d4"
};

static const char * const spm_parents[] = {
	"clk26m",
	"ulposc_d10",
	"mainpll_d7_d4",
	"clk32k"
};

static const char * const scp_parents[] = {
	"clk26m",
	"univpll_d4",
	"mainpll_d6",
	"univpll_d6",
	"univpll_d4_d2",
	"mainpll_d4_d2",
	"univpll_d3",
	"mainpll_d3"
};

static const char * const bus_aximem_parents[] = {
	"clk26m",
	"mainpll_d7_d2",
	"mainpll_d4_d2",
	"mainpll_d5_d2",
	"mainpll_d6"
};

static const char * const vpp_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"mainpll_d5_d2",
	"mmpll_d6_d2",
	"univpll_d5_d2",
	"univpll_d4_d2",
	"mmpll_d4_d2",
	"mmpll_d7",
	"univpll_d6",
	"mainpll_d4",
	"mmpll_d5",
	"tvdpll1_ck",
	"tvdpll2_ck",
	"univpll_d4",
	"mmpll_d4"
};

static const char * const ethdr_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"mainpll_d5_d2",
	"mmpll_d6_d2",
	"univpll_d5_d2",
	"univpll_d4_d2",
	"mmpll_d4_d2",
	"mmpll_d7",
	"univpll_d6",
	"mainpll_d4",
	"mmpll_d5_d4",
	"tvdpll1_ck",
	"tvdpll2_ck",
	"univpll_d4",
	"mmpll_d4"
};

static const char * const ipe_parents[] = {
	"clk26m",
	"imgpll_ck",
	"mainpll_d4",
	"mmpll_d6",
	"univpll_d6",
	"mainpll_d6",
	"mmpll_d4_d2",
	"univpll_d4_d2",
	"mainpll_d4_d2",
	"mmpll_d6_d2",
	"univpll_d5_d2",
	"mainpll_d7"
};

static const char * const cam_parents[] = {
	"clk26m",
	"tvdpll1_ck",
	"mainpll_d4",
	"mmpll_d4",
	"univpll_d4",
	"univpll_d5",
	"univpll_d6",
	"mmpll_d7",
	"univpll_d4_d2",
	"mainpll_d4_d2",
	"imgpll_ck"
};

static const char * const ccu_parents[] = {
	"clk26m",
	"univpll_d6",
	"mainpll_d4_d2",
	"mainpll_d4",
	"univpll_d5",
	"mainpll_d6",
	"mmpll_d6",
	"mmpll_d7",
	"univpll_d4_d2",
	"univpll_d7"
};

static const char * const ccu_ahb_parents[] = {
	"clk26m",
	"univpll_d6",
	"mainpll_d4_d2",
	"mainpll_d4",
	"univpll_d5",
	"mainpll_d6",
	"mmpll_d6",
	"mmpll_d7",
	"univpll_d4_d2",
	"univpll_d7"
};

static const char * const img_parents[] = {
	"clk26m",
	"imgpll_ck",
	"univpll_d4",
	"mainpll_d4",
	"univpll_d5",
	"mmpll_d6",
	"mmpll_d7",
	"univpll_d6",
	"mainpll_d6",
	"mmpll_d4_d2",
	"univpll_d4_d2",
	"mainpll_d4_d2",
	"univpll_d5_d2"
};

static const char * const camtm_parents[] = {
	"clk26m",
	"univpll_d4_d4",
	"univpll_d6_d2",
	"univpll_d6_d4"
};

static const char * const dsp_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"univpll_d4_d2",
	"univpll_d5",
	"univpll_d4",
	"mmpll_d4",
	"mainpll_d3",
	"univpll_d3"
};

static const char * const dsp1_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"mainpll_d4_d2",
	"univpll_d5",
	"mmpll_d5",
	"univpll_d4",
	"mainpll_d3",
	"univpll_d3"
};

static const char * const dsp2_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"mainpll_d4_d2",
	"univpll_d5",
	"mmpll_d5",
	"univpll_d4",
	"mainpll_d3",
	"univpll_d3"
};

static const char * const dsp3_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"mainpll_d4_d2",
	"univpll_d5",
	"mmpll_d5",
	"univpll_d4",
	"mainpll_d3",
	"univpll_d3"
};

static const char * const dsp4_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"univpll_d4_d2",
	"mainpll_d4",
	"univpll_d4",
	"mmpll_d4",
	"mainpll_d3",
	"univpll_d3"
};

static const char * const dsp5_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"univpll_d4_d2",
	"mainpll_d4",
	"univpll_d4",
	"mmpll_d4",
	"mainpll_d3",
	"univpll_d3"
};

static const char * const dsp6_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"univpll_d4_d2",
	"mainpll_d4",
	"univpll_d4",
	"mmpll_d4",
	"mainpll_d3",
	"univpll_d3"
};

static const char * const dsp7_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"univpll_d4_d2",
	"univpll_d5",
	"univpll_d4",
	"mmpll_d4",
	"mainpll_d3",
	"univpll_d3"
};

static const char * const mfg_core_tmp_parents[] = {
	"clk26m",
	"mainpll_d5_d2",
	"univpll_d6",
	"univpll_d7"
};

static const char * const camtg_parents[] = {
	"clk26m",
	"univpll_192m_d8",
	"univpll_d6_d8",
	"univpll_192m_d4",
	"univpll_192m_d10",
	"clk13m",
	"univpll_192m_d16",
	"univpll_192m_d32"
};

static const char * const camtg2_parents[] = {
	"clk26m",
	"univpll_192m_d8",
	"univpll_d6_d8",
	"univpll_192m_d4",
	"univpll_192m_d10",
	"clk13m",
	"univpll_192m_d16",
	"univpll_192m_d32"
};

static const char * const camtg3_parents[] = {
	"clk26m",
	"univpll_192m_d8",
	"univpll_d6_d8",
	"univpll_192m_d4",
	"univpll_192m_d10",
	"clk13m",
	"univpll_192m_d16",
	"univpll_192m_d32"
};

static const char * const uart_parents[] = {
	"clk26m",
	"univpll_d6_d8"
};

static const char * const spi_parents[] = {
	"clk26m",
	"mainpll_d5_d4",
	"mainpll_d6_d4",
	"univpll_d6_d4",
	"univpll_d6_d2",
	"mainpll_d6_d2",
	"mainpll_d4_d4",
	"univpll_d5_d4"
};

static const char * const msdc5hclk_parents[] = {
	"clk26m",
	"mainpll_d4_d2",
	"mainpll_d6_d2"
};

static const char * const msdc50_0_parents[] = {
	"clk26m",
	"msdcpll_ck",
	"msdcpll_d2",
	"univpll_d4_d4",
	"mainpll_d6_d2",
	"univpll_d4_d2"
};

static const char * const msdc30_1_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"mainpll_d6_d2",
	"mainpll_d7_d2",
	"msdcpll_d2"
};

static const char * const msdc30_2_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"mainpll_d6_d2",
	"mainpll_d7_d2",
	"msdcpll_d2"
};

static const char * const intdir_parents[] = {
	"clk26m",
	"univpll_d6",
	"mainpll_d4",
	"univpll_d4"
};

static const char * const aud_intbus_parents[] = {
	"clk26m",
	"mainpll_d4_d4",
	"mainpll_d7_d4"
};

static const char * const audio_h_parents[] = {
	"clk26m",
	"univpll_d7",
	"apll1_ck",
	"apll2_ck"
};

static const char * const pwrap_ulposc_parents[] = {
	"clk26m",
	"ulposc_d10",
	"ulposc_d7",
	"ulposc_d8",
	"ulposc_d16",
	"mainpll_d4_d8",
	"univpll_d5_d8",
	"tvdpll1_d16"
};

static const char * const atb_parents[] = {
	"clk26m",
	"mainpll_d4_d2",
	"mainpll_d5_d2"
};

static const char * const sspm_parents[] = {
	"clk26m",
	"mainpll_d7_d2",
	"mainpll_d6_d2",
	"mainpll_d5_d2",
	"mainpll_d9",
	"mainpll_d4_d2"
};

static const char * const dp_parents[] = {
	"clk26m",
	"tvdpll1_d2",
	"tvdpll2_d2",
	"tvdpll1_d4",
	"tvdpll2_d4",
	"tvdpll1_d8",
	"tvdpll2_d8",
	"tvdpll1_d16",
	"tvdpll2_d16"
};

static const char * const edp_parents[] = {
	"clk26m",
	"tvdpll1_d2",
	"tvdpll2_d2",
	"tvdpll1_d4",
	"tvdpll2_d4",
	"tvdpll1_d8",
	"tvdpll2_d8",
	"tvdpll1_d16",
	"tvdpll2_d16"
};

static const char * const dpi_parents[] = {
	"clk26m",
	"tvdpll1_d2",
	"tvdpll2_d2",
	"tvdpll1_d4",
	"tvdpll2_d4",
	"tvdpll1_d8",
	"tvdpll2_d8",
	"tvdpll1_d16",
	"tvdpll2_d16"
};

static const char * const disp_pwm0_parents[] = {
	"clk26m",
	"univpll_d6_d4",
	"ulposc_d2",
	"ulposc_d4",
	"ulposc_d16",
	"ethpll_d4"
};

static const char * const disp_pwm1_parents[] = {
	"clk26m",
	"univpll_d6_d4",
	"ulposc_d2",
	"ulposc_d4",
	"ulposc_d16"
};

static const char * const usb_parents[] = {
	"clk26m",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d5_d2"
};

static const char * const ssusb_xhci_parents[] = {
	"clk26m",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d5_d2"
};

static const char * const usb_2p_parents[] = {
	"clk26m",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d5_d2"
};

static const char * const ssusb_xhci_2p_parents[] = {
	"clk26m",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d5_d2"
};

static const char * const usb_3p_parents[] = {
	"clk26m",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d5_d2"
};

static const char * const ssusb_xhci_3p_parents[] = {
	"clk26m",
	"univpll_d5_d4",
	"univpll_d6_d4",
	"univpll_d5_d2"
};

static const char * const i2c_parents[] = {
	"clk26m",
	"mainpll_d4_d8",
	"univpll_d5_d4"
};

static const char * const seninf_parents[] = {
	"clk26m",
	"univpll_d4_d4",
	"univpll_d6_d2",
	"mainpll_d4_d2",
	"univpll_d7",
	"univpll_d6",
	"mmpll_d6",
	"univpll_d5"
};

static const char * const seninf1_parents[] = {
	"clk26m",
	"univpll_d4_d4",
	"univpll_d6_d2",
	"mainpll_d4_d2",
	"univpll_d7",
	"univpll_d6",
	"mmpll_d6",
	"univpll_d5"
};

static const char * const gcpu_parents[] = {
	"clk26m",
	"mainpll_d6",
	"univpll_d4_d2",
	"mmpll_d5_d2",
	"univpll_d5_d2"
};

static const char * const venc_parents[] = {
	"clk26m",
	"mmpll_d4_d2",
	"mainpll_d6",
	"univpll_d4_d2",
	"mainpll_d4_d2",
	"univpll_d6",
	"mmpll_d6",
	"mainpll_d5_d2",
	"mainpll_d6_d2",
	"mmpll_d9",
	"univpll_d4_d4",
	"mainpll_d4",
	"univpll_d4",
	"univpll_d5",
	"univpll_d5_d2",
	"mainpll_d5"
};

static const char * const vdec_parents[] = {
	"clk26m",
	"mainpll_d5_d2",
	"mmpll_d6_d2",
	"univpll_d5_d2",
	"univpll_d4_d2",
	"mmpll_d4_d2",
	"univpll_d6",
	"mainpll_d5",
	"univpll_d5",
	"mmpll_d6",
	"mainpll_d4",
	"tvdpll2_ck",
	"univpll_d4",
	"imgpll_ck",
	"univpll_d6_d2",
	"mmpll_d9"
};

static const char * const pwm_parents[] = {
	"clk32k",
	"clk26m",
	"univpll_d4_d8",
	"univpll_d6_d4"
};

static const char * const mcupm_parents[] = {
	"clk26m",
	"mainpll_d6_d2",
	"mainpll_d7_d4"
};

static const char * const spmi_p_mst_parents[] = {
	"clk26m",
	"clk13m",
	"ulposc_d8",
	"ulposc_d10",
	"ulposc_d16",
	"ulposc_d7",
	"clk32k",
	"mainpll_d7_d8",
	"mainpll_d6_d8",
	"mainpll_d5_d8"
};

static const char * const spmi_m_mst_parents[] = {
	"clk26m",
	"clk13m",
	"ulposc_d8",
	"ulposc_d10",
	"ulposc_d16",
	"ulposc_d7",
	"clk32k",
	"mainpll_d7_d8",
	"mainpll_d6_d8",
	"mainpll_d5_d8"
};

static const char * const dvfsrc_parents[] = {
	"clk26m",
	"ulposc_d10",
	"univpll_d6_d8",
	"msdcpll_d16"
};

static const char * const tl_parents[] = {
	"clk26m",
	"univpll_d5_d4",
	"mainpll_d4_d4"
};

static const char * const aes_msdcfde_parents[] = {
	"clk26m",
	"mainpll_d4_d2",
	"mainpll_d6",
	"mainpll_d4_d4",
	"univpll_d4_d2",
	"univpll_d6"
};

static const char * const dsi_occ_parents[] = {
	"clk26m",
	"univpll_d6_d2",
	"univpll_d5_d2",
	"univpll_d4_d2"
};

static const char * const wpe_vpp_parents[] = {
	"clk26m",
	"mainpll_d5_d2",
	"mmpll_d6_d2",
	"univpll_d5_d2",
	"mainpll_d4_d2",
	"univpll_d4_d2",
	"mmpll_d4_d2",
	"mainpll_d6",
	"mmpll_d7",
	"univpll_d6",
	"mainpll_d5",
	"univpll_d5",
	"mainpll_d4",
	"tvdpll1_ck",
	"univpll_d4"
};

static const char * const hdcp_parents[] = {
	"clk26m",
	"univpll_d4_d8",
	"mainpll_d5_d8",
	"univpll_d6_d4"
};

static const char * const hdcp_24m_parents[] = {
	"clk26m",
	"univpll_192m_d4",
	"univpll_192m_d8",
	"univpll_d6_d8"
};

static const char * const hdmi_apb_parents[] = {
	"clk26m",
	"univpll_d6_d4",
	"msdcpll_d2"
};

static const char * const snps_eth_250m_parents[] = {
	"clk26m",
	"ethpll_d2"
};

static const char * const snps_eth_62p4m_ptp_parents[] = {
	"apll2_d3",
	"apll1_d3",
	"clk26m",
	"ethpll_d8"
};

static const char * const snps_eth_50m_rmii_parents[] = {
	"clk26m",
	"ethpll_d10"
};

static const char * const adsp_parents[] = {
	"clk26m",
	"clk13m",
	"mainpll_d6",
	"mainpll_d5_d2",
	"univpll_d4_d4",
	"univpll_d4",
	"ulposc_d2",
	"ulposc_ck",
	"adsppll_ck",
	"adsppll_d2",
	"adsppll_d4",
	"adsppll_d8"
};

static const char * const audio_local_bus_parents[] = {
	"clk26m",
	"clk13m",
	"mainpll_d4_d4",
	"mainpll_d7_d2",
	"mainpll_d5_d2",
	"mainpll_d4_d2",
	"mainpll_d7",
	"mainpll_d4",
	"univpll_d6",
	"ulposc_ck",
	"ulposc_d4",
	"ulposc_d2"
};

static const char * const asm_h_parents[] = {
	"clk26m",
	"univpll_d6_d4",
	"univpll_d6_d2",
	"mainpll_d5_d2"
};

static const char * const asm_l_parents[] = {
	"clk26m",
	"univpll_d6_d4",
	"univpll_d6_d2",
	"mainpll_d5_d2"
};

static const char * const apll1_parents[] = {
	"clk26m",
	"apll1_d4"
};

static const char * const apll2_parents[] = {
	"clk26m",
	"apll2_d4"
};

static const char * const apll3_parents[] = {
	"clk26m",
	"apll3_d4"
};

static const char * const apll4_parents[] = {
	"clk26m",
	"apll4_d4"
};

static const char * const apll5_parents[] = {
	"clk26m",
	"apll5_d4"
};

static const char * const i2so1_parents[] = {
	"clk26m",
	"apll1_ck",
	"apll2_ck",
	"apll3_ck",
	"apll4_ck",
	"apll5_ck"
};

static const char * const i2so2_parents[] = {
	"clk26m",
	"apll1_ck",
	"apll2_ck",
	"apll3_ck",
	"apll4_ck",
	"apll5_ck"
};

static const char * const i2si1_parents[] = {
	"clk26m",
	"apll1_ck",
	"apll2_ck",
	"apll3_ck",
	"apll4_ck",
	"apll5_ck"
};

static const char * const i2si2_parents[] = {
	"clk26m",
	"apll1_ck",
	"apll2_ck",
	"apll3_ck",
	"apll4_ck",
	"apll5_ck"
};

static const char * const dptx_parents[] = {
	"clk26m",
	"apll1_ck",
	"apll2_ck",
	"apll3_ck",
	"apll4_ck",
	"apll5_ck"
};

static const char * const aud_iec_parents[] = {
	"clk26m",
	"apll1_ck",
	"apll2_ck",
	"apll3_ck",
	"apll4_ck",
	"apll5_ck"
};

static const char * const a1sys_hp_parents[] = {
	"clk26m",
	"apll1_d4"
};

static const char * const a2sys_parents[] = {
	"clk26m",
	"apll2_d4"
};

static const char * const a3sys_parents[] = {
	"clk26m",
	"apll3_d4",
	"apll4_d4",
	"apll5_d4"
};

static const char * const a4sys_parents[] = {
	"clk26m",
	"apll3_d4",
	"apll4_d4",
	"apll5_d4"
};

static const char * const ecc_parents[] = {
	"clk26m",
	"mainpll_d4_d4",
	"mainpll_d5_d2",
	"mainpll_d4_d2",
	"mainpll_d6",
	"univpll_d6"
};

static const char * const spinor_parents[] = {
	"clk26m",
	"clk13m",
	"mainpll_d7_d8",
	"univpll_d6_d8"
};

static const char * const ulposc_parents[] = {
	"ulposc_ck",
	"ethpll_d2",
	"mainpll_d4_d2",
	"ethpll_d10"
};

static const char * const srck_parents[] = {
	"ulposc_d10",
	"clk26m"
};

static const char * const mfg_fast_ref_parents[] = {
	"mfg_core_tmp_sel",
	"mfgpll_ck"
};

static const struct mtk_mux top_mtk_muxes[] = {
	/*
	 * CLK_CFG_0
	 * axi_sel and bus_aximem_sel are bus clocks, should not be closed by Linux.
	 * spm_sel and scp_sel are main clocks in always-on co-processor.
	 */
	MUX_GATE_CLR_SET_UPD_FLAGS(CLK_TOP_AXI_SEL, "axi_sel", axi_parents,
				   0x020, 0x024, 0x028, 0, 4, 7, 0x04, 0, CLK_IS_CRITICAL),
	MUX_GATE_CLR_SET_UPD_FLAGS(CLK_TOP_SPM_SEL, "spm_sel", spm_parents,
				   0x020, 0x024, 0x028, 8, 4, 15, 0x04, 1, CLK_IS_CRITICAL),
	MUX_GATE_CLR_SET_UPD_FLAGS(CLK_TOP_SCP_SEL, "scp_sel", scp_parents,
				   0x020, 0x024, 0x028, 16, 4, 23, 0x04, 2, CLK_IS_CRITICAL),
	MUX_GATE_CLR_SET_UPD_FLAGS(CLK_TOP_BUS_AXIMEM_SEL, "bus_aximem_sel", bus_aximem_parents,
				   0x020, 0x024, 0x028, 24, 4, 31, 0x04, 3, CLK_IS_CRITICAL),
	/* CLK_CFG_1 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_VPP_SEL, "vpp_sel",
			     vpp_parents, 0x02C, 0x030, 0x034, 0, 4, 7, 0x04, 4),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_ETHDR_SEL, "ethdr_sel",
			     ethdr_parents, 0x02C, 0x030, 0x034, 8, 4, 15, 0x04, 5),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_IPE_SEL, "ipe_sel",
			     ipe_parents, 0x02C, 0x030, 0x034, 16, 4, 23, 0x04, 6),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_CAM_SEL, "cam_sel",
			     cam_parents, 0x02C, 0x030, 0x034, 24, 4, 31, 0x04, 7),
	/* CLK_CFG_2 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_CCU_SEL, "ccu_sel",
			     ccu_parents, 0x038, 0x03C, 0x040, 0, 4, 7, 0x04, 8),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_CCU_AHB_SEL, "ccu_ahb_sel",
			     ccu_ahb_parents, 0x038, 0x03C, 0x040, 8, 4, 15, 0x04, 9),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_IMG_SEL, "img_sel",
			     img_parents, 0x038, 0x03C, 0x040, 16, 4, 23, 0x04, 10),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_CAMTM_SEL, "camtm_sel",
			     camtm_parents, 0x038, 0x03C, 0x040, 24, 4, 31, 0x04, 11),
	/* CLK_CFG_3 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSP_SEL, "dsp_sel",
			     dsp_parents, 0x044, 0x048, 0x04C, 0, 4, 7, 0x04, 12),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSP1_SEL, "dsp1_sel",
			     dsp1_parents, 0x044, 0x048, 0x04C, 8, 4, 15, 0x04, 13),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSP2_SEL, "dsp2_sel",
			     dsp2_parents, 0x044, 0x048, 0x04C, 16, 4, 23, 0x04, 14),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSP3_SEL, "dsp3_sel",
			     dsp3_parents, 0x044, 0x048, 0x04C, 24, 4, 31, 0x04, 15),
	/* CLK_CFG_4 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSP4_SEL, "dsp4_sel",
			     dsp4_parents, 0x050, 0x054, 0x058, 0, 4, 7, 0x04, 16),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSP5_SEL, "dsp5_sel",
			     dsp5_parents, 0x050, 0x054, 0x058, 8, 4, 15, 0x04, 17),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSP6_SEL, "dsp6_sel",
			     dsp6_parents, 0x050, 0x054, 0x058, 16, 4, 23, 0x04, 18),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSP7_SEL, "dsp7_sel",
			     dsp7_parents, 0x050, 0x054, 0x058, 24, 4, 31, 0x04, 19),
	/* CLK_CFG_5 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MFG_CORE_TMP_SEL, "mfg_core_tmp_sel",
			     mfg_core_tmp_parents, 0x05C, 0x060, 0x064, 0, 4, 7, 0x04, 20),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_CAMTG_SEL, "camtg_sel",
			     camtg_parents, 0x05C, 0x060, 0x064, 8, 4, 15, 0x04, 21),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_CAMTG2_SEL, "camtg2_sel",
			     camtg2_parents, 0x05C, 0x060, 0x064, 16, 4, 23, 0x04, 22),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_CAMTG3_SEL, "camtg3_sel",
			     camtg3_parents, 0x05C, 0x060, 0x064, 24, 4, 31, 0x04, 23),
	/* CLK_CFG_6 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_UART_SEL, "uart_sel",
			     uart_parents, 0x068, 0x06C, 0x070, 0, 4, 7, 0x04, 24),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SPI_SEL, "spi_sel",
			     spi_parents, 0x068, 0x06C, 0x070, 8, 4, 15, 0x04, 25),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MSDC50_0_HCLK_SEL, "msdc5hclk_sel",
			     msdc5hclk_parents, 0x068, 0x06C, 0x070, 16, 4, 23, 0x04, 26),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MSDC50_0_SEL, "msdc50_0_sel",
			     msdc50_0_parents, 0x068, 0x06C, 0x070, 24, 4, 31, 0x04, 27),
	/* CLK_CFG_7 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MSDC30_1_SEL, "msdc30_1_sel",
			     msdc30_1_parents, 0x074, 0x078, 0x07C, 0, 4, 7, 0x04, 28),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_MSDC30_2_SEL, "msdc30_2_sel",
			     msdc30_2_parents, 0x074, 0x078, 0x07C, 8, 4, 15, 0x04, 29),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_INTDIR_SEL, "intdir_sel",
			     intdir_parents, 0x074, 0x078, 0x07C, 16, 4, 23, 0x04, 30),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUD_INTBUS_SEL, "aud_intbus_sel",
			     aud_intbus_parents, 0x074, 0x078, 0x07C, 24, 4, 31, 0x04, 31),
	/* CLK_CFG_8 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUDIO_H_SEL, "audio_h_sel",
			     audio_h_parents, 0x080, 0x084, 0x088, 0, 4, 7, 0x08, 0),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_PWRAP_ULPOSC_SEL, "pwrap_ulposc_sel",
			     pwrap_ulposc_parents, 0x080, 0x084, 0x088, 8, 4, 15, 0x08, 1),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_ATB_SEL, "atb_sel",
			     atb_parents, 0x080, 0x084, 0x088, 16, 4, 23, 0x08, 2),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SSPM_SEL, "sspm_sel",
			     sspm_parents, 0x080, 0x084, 0x088, 24, 4, 31, 0x08, 3),
	/* CLK_CFG_9 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DP_SEL, "dp_sel",
			     dp_parents, 0x08C, 0x090, 0x094, 0, 4, 7, 0x08, 4),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_EDP_SEL, "edp_sel",
			     edp_parents, 0x08C, 0x090, 0x094, 8, 4, 15, 0x08, 5),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DPI_SEL, "dpi_sel",
			     dpi_parents, 0x08C, 0x090, 0x094, 16, 4, 23, 0x08, 6),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DISP_PWM0_SEL, "disp_pwm0_sel",
			     disp_pwm0_parents, 0x08C, 0x090, 0x094, 24, 4, 31, 0x08, 7),
	/* CLK_CFG_10 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DISP_PWM1_SEL, "disp_pwm1_sel",
			     disp_pwm1_parents, 0x098, 0x09C, 0x0A0, 0, 4, 7, 0x08, 8),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_USB_TOP_SEL, "usb_sel",
			     usb_parents, 0x098, 0x09C, 0x0A0, 8, 4, 15, 0x08, 9),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SSUSB_XHCI_SEL, "ssusb_xhci_sel",
			     ssusb_xhci_parents, 0x098, 0x09C, 0x0A0, 16, 4, 23, 0x08, 10),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_USB_TOP_2P_SEL, "usb_2p_sel",
			     usb_2p_parents, 0x098, 0x09C, 0x0A0, 24, 4, 31, 0x08, 11),
	/* CLK_CFG_11 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SSUSB_XHCI_2P_SEL, "ssusb_xhci_2p_sel",
			     ssusb_xhci_2p_parents, 0x0A4, 0x0A8, 0x0AC, 0, 4, 7, 0x08, 12),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_USB_TOP_3P_SEL, "usb_3p_sel",
			     usb_3p_parents, 0x0A4, 0x0A8, 0x0AC, 8, 4, 15, 0x08, 13),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SSUSB_XHCI_3P_SEL, "ssusb_xhci_3p_sel",
			     ssusb_xhci_3p_parents, 0x0A4, 0x0A8, 0x0AC, 16, 4, 23, 0x08, 14),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_I2C_SEL, "i2c_sel",
			     i2c_parents, 0x0A4, 0x0A8, 0x0AC, 24, 4, 31, 0x08, 15),
	/* CLK_CFG_12 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SENINF_SEL, "seninf_sel",
			     seninf_parents, 0x0B0, 0x0B4, 0x0B8, 0, 4, 7, 0x08, 16),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SENINF1_SEL, "seninf1_sel",
			     seninf1_parents, 0x0B0, 0x0B4, 0x0B8, 8, 4, 15, 0x08, 17),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_GCPU_SEL, "gcpu_sel",
			     gcpu_parents, 0x0B0, 0x0B4, 0x0B8, 16, 4, 23, 0x08, 18),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_VENC_SEL, "venc_sel",
			     venc_parents, 0x0B0, 0x0B4, 0x0B8, 24, 4, 31, 0x08, 19),
	/*
	 * CLK_CFG_13
	 * top_mcupm is main clock in co-processor, should not be handled by Linux.
	 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_VDEC_SEL, "vdec_sel",
			     vdec_parents, 0x0BC, 0x0C0, 0x0C4, 0, 4, 7, 0x08, 20),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_PWM_SEL, "pwm_sel",
			     pwm_parents, 0x0BC, 0x0C0, 0x0C4, 8, 4, 15, 0x08, 21),
	MUX_GATE_CLR_SET_UPD_FLAGS(CLK_TOP_MCUPM_SEL, "mcupm_sel", mcupm_parents,
				   0x0BC, 0x0C0, 0x0C4, 16, 4, 23, 0x08, 22, CLK_IS_CRITICAL),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SPMI_P_MST_SEL, "spmi_p_mst_sel",
			     spmi_p_mst_parents, 0x0BC, 0x0C0, 0x0C4, 24, 4, 31, 0x08, 23),
	/*
	 * CLK_CFG_14
	 * dvfsrc_sel is for internal DVFS usage, should not be handled by Linux.
	 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SPMI_M_MST_SEL, "spmi_m_mst_sel",
			     spmi_m_mst_parents, 0x0C8, 0x0CC, 0x0D0, 0, 4, 7, 0x08, 24),
	MUX_GATE_CLR_SET_UPD_FLAGS(CLK_TOP_DVFSRC_SEL, "dvfsrc_sel", dvfsrc_parents,
				   0x0C8, 0x0CC, 0x0D0, 8, 4, 15, 0x08, 25, CLK_IS_CRITICAL),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_TL_SEL, "tl_sel",
			     tl_parents, 0x0C8, 0x0CC, 0x0D0, 16, 4, 23, 0x08, 26),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AES_MSDCFDE_SEL, "aes_msdcfde_sel",
			     aes_msdcfde_parents, 0x0C8, 0x0CC, 0x0D0, 24, 4, 31, 0x08, 27),
	/* CLK_CFG_15 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DSI_OCC_SEL, "dsi_occ_sel",
			     dsi_occ_parents, 0x0D4, 0x0D8, 0x0DC, 0, 4, 7, 0x08, 28),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_WPE_VPP_SEL, "wpe_vpp_sel",
			     wpe_vpp_parents, 0x0D4, 0x0D8, 0x0DC, 8, 4, 15, 0x08, 29),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_HDCP_SEL, "hdcp_sel",
			     hdcp_parents, 0x0D4, 0x0D8, 0x0DC, 16, 4, 23, 0x08, 30),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_HDCP_24M_SEL, "hdcp_24m_sel",
			     hdcp_24m_parents, 0x0D4, 0x0D8, 0x0DC, 24, 4, 31, 0x08, 31),
	/* CLK_CFG_16 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_HDMI_APB_SEL, "hdmi_apb_sel",
			     hdmi_apb_parents, 0x0E0, 0x0E4, 0x0E8, 0, 4, 7, 0x0C, 0),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SNPS_ETH_250M_SEL, "snps_eth_250m_sel",
			     snps_eth_250m_parents, 0x0E0, 0x0E4, 0x0E8, 8, 4, 15, 0x0C, 1),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SNPS_ETH_62P4M_PTP_SEL, "snps_eth_62p4m_ptp_sel",
			     snps_eth_62p4m_ptp_parents, 0x0E0, 0x0E4, 0x0E8, 16, 4, 23, 0x0C, 2),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SNPS_ETH_50M_RMII_SEL, "snps_eth_50m_rmii_sel",
			     snps_eth_50m_rmii_parents, 0x0E0, 0x0E4, 0x0E8, 24, 4, 31, 0x0C, 3),
	/* CLK_CFG_17 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_ADSP_SEL, "adsp_sel",
			     adsp_parents, 0x0EC, 0x0F0, 0x0F4, 0, 4, 7, 0x0C, 4),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUDIO_LOCAL_BUS_SEL, "audio_local_bus_sel",
			     audio_local_bus_parents, 0x0EC, 0x0F0, 0x0F4, 8, 4, 15, 0x0C, 5),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_ASM_H_SEL, "asm_h_sel",
			     asm_h_parents, 0x0EC, 0x0F0, 0x0F4, 16, 4, 23, 0x0C, 6),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_ASM_L_SEL, "asm_l_sel",
			     asm_l_parents, 0x0EC, 0x0F0, 0x0F4, 24, 4, 31, 0x0C, 7),
	/* CLK_CFG_18 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_APLL1_SEL, "apll1_sel",
			     apll1_parents, 0x0F8, 0x0FC, 0x100, 0, 4, 7, 0x0C, 8),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_APLL2_SEL, "apll2_sel",
			     apll2_parents, 0x0F8, 0x0FC, 0x100, 8, 4, 15, 0x0C, 9),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_APLL3_SEL, "apll3_sel",
			     apll3_parents, 0x0F8, 0x0FC, 0x100, 16, 4, 23, 0x0C, 10),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_APLL4_SEL, "apll4_sel",
			     apll4_parents, 0x0F8, 0x0FC, 0x100, 24, 4, 31, 0x0C, 11),
	/* CLK_CFG_19 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_APLL5_SEL, "apll5_sel",
			     apll5_parents, 0x0104, 0x0108, 0x010C, 0, 4, 7, 0x0C, 12),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_I2SO1_SEL, "i2so1_sel",
			     i2so1_parents, 0x0104, 0x0108, 0x010C, 8, 4, 15, 0x0C, 13),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_I2SO2_SEL, "i2so2_sel",
			     i2so2_parents, 0x0104, 0x0108, 0x010C, 16, 4, 23, 0x0C, 14),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_I2SI1_SEL, "i2si1_sel",
			     i2si1_parents, 0x0104, 0x0108, 0x010C, 24, 4, 31, 0x0C, 15),
	/* CLK_CFG_20 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_I2SI2_SEL, "i2si2_sel",
			     i2si2_parents, 0x0110, 0x0114, 0x0118, 0, 4, 7, 0x0C, 16),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_DPTX_SEL, "dptx_sel",
			     dptx_parents, 0x0110, 0x0114, 0x0118, 8, 4, 15, 0x0C, 17),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_AUD_IEC_SEL, "aud_iec_sel",
			     aud_iec_parents, 0x0110, 0x0114, 0x0118, 16, 4, 23, 0x0C, 18),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_A1SYS_HP_SEL, "a1sys_hp_sel",
			     a1sys_hp_parents, 0x0110, 0x0114, 0x0118, 24, 4, 31, 0x0C, 19),
	/* CLK_CFG_21 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_A2SYS_SEL, "a2sys_sel",
			     a2sys_parents, 0x011C, 0x0120, 0x0124, 0, 4, 7, 0x0C, 20),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_A3SYS_SEL, "a3sys_sel",
			     a3sys_parents, 0x011C, 0x0120, 0x0124, 8, 4, 15, 0x0C, 21),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_A4SYS_SEL, "a4sys_sel",
			     a4sys_parents, 0x011C, 0x0120, 0x0124, 16, 4, 23, 0x0C, 22),
	MUX_GATE_CLR_SET_UPD(CLK_TOP_ECC_SEL, "ecc_sel",
			     ecc_parents, 0x011C, 0x0120, 0x0124, 24, 4, 31, 0x0C, 23),
	/*
	 * CLK_CFG_22
	 * ulposc_sel/srck_sel are clock source of always on co-processor,
	 * should not be closed by Linux.
	 */
	MUX_GATE_CLR_SET_UPD(CLK_TOP_SPINOR_SEL, "spinor_sel",
			     spinor_parents, 0x0128, 0x012C, 0x0130, 0, 4, 7, 0x0C, 24),
	MUX_GATE_CLR_SET_UPD_FLAGS(CLK_TOP_ULPOSC_SEL, "ulposc_sel", ulposc_parents,
				   0x0128, 0x012C, 0x0130, 8, 4, 15, 0x0C, 25, CLK_IS_CRITICAL),
	MUX_GATE_CLR_SET_UPD_FLAGS(CLK_TOP_SRCK_SEL, "srck_sel", srck_parents,
				   0x0128, 0x012C, 0x0130, 16, 4, 23, 0x0C, 26, CLK_IS_CRITICAL),
};

static struct mtk_composite top_muxes[] = {
	/* CLK_MISC_CFG_3 */
	MUX(CLK_TOP_MFG_CK_FAST_REF_SEL, "mfg_fast_ref_sel", mfg_fast_ref_parents, 0x0250, 8, 1),
};

static const struct mtk_composite top_adj_divs[] = {
	DIV_GATE(CLK_TOP_APLL12_CK_DIV0, "apll12_div0", "i2si1_sel", 0x0320, 0, 0x0328, 8, 0),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV1, "apll12_div1", "i2si2_sel", 0x0320, 1, 0x0328, 8, 8),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV2, "apll12_div2", "i2so1_sel", 0x0320, 2, 0x0328, 8, 16),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV3, "apll12_div3", "i2so2_sel", 0x0320, 3, 0x0328, 8, 24),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV4, "apll12_div4", "aud_iec_sel", 0x0320, 4, 0x0334, 8, 0),
	DIV_GATE(CLK_TOP_APLL12_CK_DIV9, "apll12_div9", "dptx_sel", 0x0320, 9, 0x0338, 8, 8),
};

static const struct mtk_gate_regs infra_ao0_cg_regs = {
	.set_ofs = 0x80,
	.clr_ofs = 0x84,
	.sta_ofs = 0x90,
};

static const struct mtk_gate_regs infra_ao1_cg_regs = {
	.set_ofs = 0x88,
	.clr_ofs = 0x8c,
	.sta_ofs = 0x94,
};

static const struct mtk_gate_regs infra_ao2_cg_regs = {
	.set_ofs = 0xa4,
	.clr_ofs = 0xa8,
	.sta_ofs = 0xac,
};

static const struct mtk_gate_regs infra_ao3_cg_regs = {
	.set_ofs = 0xc0,
	.clr_ofs = 0xc4,
	.sta_ofs = 0xc8,
};

static const struct mtk_gate_regs infra_ao4_cg_regs = {
	.set_ofs = 0xe0,
	.clr_ofs = 0xe4,
	.sta_ofs = 0xe8,
};

#define GATE_INFRA_AO0_FLAGS(_id, _name, _parent, _shift, _flag)	\
	GATE_MTK_FLAGS(_id, _name, _parent, &infra_ao0_cg_regs, _shift, \
		       &mtk_clk_gate_ops_setclr, _flag)

#define GATE_INFRA_AO0(_id, _name, _parent, _shift)			\
	GATE_INFRA_AO0_FLAGS(_id, _name, _parent, _shift, 0)

#define GATE_INFRA_AO1_FLAGS(_id, _name, _parent, _shift, _flag)	\
	GATE_MTK_FLAGS(_id, _name, _parent, &infra_ao1_cg_regs, _shift, \
		       &mtk_clk_gate_ops_setclr, _flag)

#define GATE_INFRA_AO1(_id, _name, _parent, _shift)			\
	GATE_INFRA_AO1_FLAGS(_id, _name, _parent, _shift, 0)

#define GATE_INFRA_AO2(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &infra_ao2_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

#define GATE_INFRA_AO3_FLAGS(_id, _name, _parent, _shift, _flag)	\
	GATE_MTK_FLAGS(_id, _name, _parent, &infra_ao3_cg_regs, _shift, \
		       &mtk_clk_gate_ops_setclr, _flag)

#define GATE_INFRA_AO3(_id, _name, _parent, _shift)			\
	GATE_INFRA_AO3_FLAGS(_id, _name, _parent, _shift, 0)

#define GATE_INFRA_AO4_FLAGS(_id, _name, _parent, _shift, _flag)	\
	GATE_MTK_FLAGS(_id, _name, _parent, &infra_ao4_cg_regs, _shift, \
		       &mtk_clk_gate_ops_setclr, _flag)

#define GATE_INFRA_AO4(_id, _name, _parent, _shift)			\
	GATE_INFRA_AO4_FLAGS(_id, _name, _parent, _shift, 0)

static const struct mtk_gate infra_ao_clks[] = {
	/* INFRA_AO0 */
	GATE_INFRA_AO0(CLK_INFRA_AO_PMIC_TMR, "infra_ao_pmic_tmr", "pwrap_ulposc_sel", 0),
	GATE_INFRA_AO0(CLK_INFRA_AO_PMIC_AP, "infra_ao_pmic_ap", "pwrap_ulposc_sel", 1),
	GATE_INFRA_AO0(CLK_INFRA_AO_PMIC_MD, "infra_ao_pmic_md", "pwrap_ulposc_sel", 2),
	GATE_INFRA_AO0(CLK_INFRA_AO_PMIC_CONN, "infra_ao_pmic_conn", "pwrap_ulposc_sel", 3),
	/* infra_ao_sej is main clock is for secure engine with JTAG support */
	GATE_INFRA_AO0_FLAGS(CLK_INFRA_AO_SEJ, "infra_ao_sej", "axi_sel", 5, CLK_IS_CRITICAL),
	GATE_INFRA_AO0(CLK_INFRA_AO_APXGPT, "infra_ao_apxgpt", "axi_sel", 6),
	GATE_INFRA_AO0(CLK_INFRA_AO_GCE, "infra_ao_gce", "axi_sel", 8),
	GATE_INFRA_AO0(CLK_INFRA_AO_GCE2, "infra_ao_gce2", "axi_sel", 9),
	GATE_INFRA_AO0(CLK_INFRA_AO_THERM, "infra_ao_therm", "axi_sel", 10),
	GATE_INFRA_AO0(CLK_INFRA_AO_PWM_HCLK, "infra_ao_pwm_hclk", "axi_sel", 15),
	GATE_INFRA_AO0(CLK_INFRA_AO_PWM1, "infra_ao_pwm1", "pwm_sel", 16),
	GATE_INFRA_AO0(CLK_INFRA_AO_PWM2, "infra_ao_pwm2", "pwm_sel", 17),
	GATE_INFRA_AO0(CLK_INFRA_AO_PWM3, "infra_ao_pwm3", "pwm_sel", 18),
	GATE_INFRA_AO0(CLK_INFRA_AO_PWM4, "infra_ao_pwm4", "pwm_sel", 19),
	GATE_INFRA_AO0(CLK_INFRA_AO_PWM, "infra_ao_pwm", "pwm_sel", 21),
	GATE_INFRA_AO0(CLK_INFRA_AO_UART0, "infra_ao_uart0", "uart_sel", 22),
	GATE_INFRA_AO0(CLK_INFRA_AO_UART1, "infra_ao_uart1", "uart_sel", 23),
	GATE_INFRA_AO0(CLK_INFRA_AO_UART2, "infra_ao_uart2", "uart_sel", 24),
	GATE_INFRA_AO0(CLK_INFRA_AO_UART3, "infra_ao_uart3", "uart_sel", 25),
	GATE_INFRA_AO0(CLK_INFRA_AO_UART4, "infra_ao_uart4", "uart_sel", 26),
	GATE_INFRA_AO0(CLK_INFRA_AO_GCE_26M, "infra_ao_gce_26m", "clk26m", 27),
	GATE_INFRA_AO0(CLK_INFRA_AO_CQ_DMA_FPC, "infra_ao_dma", "pad_fpc_ck", 28),
	GATE_INFRA_AO0(CLK_INFRA_AO_UART5, "infra_ao_uart5", "uart_sel", 29),
	/* INFRA_AO1 */
	GATE_INFRA_AO1(CLK_INFRA_AO_HDMI_26M, "infra_ao_hdmi_26m", "clk26m", 0),
	GATE_INFRA_AO1(CLK_INFRA_AO_SPI0, "infra_ao_spi0", "spi_sel", 1),
	GATE_INFRA_AO1(CLK_INFRA_AO_MSDC0, "infra_ao_msdc0", "msdc5hclk_sel", 2),
	GATE_INFRA_AO1(CLK_INFRA_AO_MSDC1, "infra_ao_msdc1", "axi_sel", 4),
	GATE_INFRA_AO1(CLK_INFRA_AO_MSDC2, "infra_ao_msdc2", "axi_sel", 5),
	GATE_INFRA_AO1(CLK_INFRA_AO_MSDC0_SRC, "infra_ao_msdc0_clk", "msdc50_0_sel", 6),
	/* infra_ao_dvfsrc is for internal DVFS usage, should not be handled by Linux. */
	GATE_INFRA_AO1_FLAGS(CLK_INFRA_AO_DVFSRC, "infra_ao_dvfsrc",
			     "clk26m", 7, CLK_IS_CRITICAL),
	GATE_INFRA_AO1(CLK_INFRA_AO_TRNG, "infra_ao_trng", "axi_sel", 9),
	GATE_INFRA_AO1(CLK_INFRA_AO_AUXADC, "infra_ao_auxadc", "clk26m", 10),
	GATE_INFRA_AO1(CLK_INFRA_AO_CPUM, "infra_ao_cpum", "axi_sel", 11),
	GATE_INFRA_AO1(CLK_INFRA_AO_HDMI_32K, "infra_ao_hdmi_32k", "clk32k", 12),
	GATE_INFRA_AO1(CLK_INFRA_AO_CEC_66M_HCLK, "infra_ao_cec_66m_hclk", "axi_sel", 13),
	GATE_INFRA_AO1(CLK_INFRA_AO_PCIE_TL_26M, "infra_ao_pcie_tl_26m", "clk26m", 15),
	GATE_INFRA_AO1(CLK_INFRA_AO_MSDC1_SRC, "infra_ao_msdc1_clk", "msdc30_1_sel", 16),
	GATE_INFRA_AO1(CLK_INFRA_AO_CEC_66M_BCLK, "infra_ao_cec_66m_bclk", "axi_sel", 17),
	GATE_INFRA_AO1(CLK_INFRA_AO_PCIE_TL_96M, "infra_ao_pcie_tl_96m", "tl_sel", 18),
	/* infra_ao_dapc is for device access permission control module */
	GATE_INFRA_AO1_FLAGS(CLK_INFRA_AO_DEVICE_APC, "infra_ao_dapc",
			     "axi_sel", 20, CLK_IS_CRITICAL),
	GATE_INFRA_AO1(CLK_INFRA_AO_ECC_66M_HCLK, "infra_ao_ecc_66m_hclk", "axi_sel", 23),
	GATE_INFRA_AO1(CLK_INFRA_AO_DEBUGSYS, "infra_ao_debugsys", "axi_sel", 24),
	GATE_INFRA_AO1(CLK_INFRA_AO_AUDIO, "infra_ao_audio", "axi_sel", 25),
	GATE_INFRA_AO1(CLK_INFRA_AO_PCIE_TL_32K, "infra_ao_pcie_tl_32k", "clk32k", 26),
	GATE_INFRA_AO1(CLK_INFRA_AO_DBG_TRACE, "infra_ao_dbg_trace", "axi_sel", 29),
	GATE_INFRA_AO1(CLK_INFRA_AO_DRAMC_F26M, "infra_ao_dramc26", "clk26m", 31),
	/* INFRA_AO2 */
	GATE_INFRA_AO2(CLK_INFRA_AO_IRTX, "infra_ao_irtx", "axi_sel", 0),
	GATE_INFRA_AO2(CLK_INFRA_AO_DISP_PWM, "infra_ao_disp_pwm", "disp_pwm0_sel", 2),
	GATE_INFRA_AO2(CLK_INFRA_AO_CLDMA_BCLK, "infra_ao_cldmabclk", "axi_sel", 3),
	GATE_INFRA_AO2(CLK_INFRA_AO_AUDIO_26M_BCLK, "infra_ao_audio26m", "clk26m", 4),
	GATE_INFRA_AO2(CLK_INFRA_AO_SPI1, "infra_ao_spi1", "spi_sel", 6),
	GATE_INFRA_AO2(CLK_INFRA_AO_SPI2, "infra_ao_spi2", "spi_sel", 9),
	GATE_INFRA_AO2(CLK_INFRA_AO_SPI3, "infra_ao_spi3", "spi_sel", 10),
	GATE_INFRA_AO2(CLK_INFRA_AO_FSSPM, "infra_ao_fsspm", "sspm_sel", 15),
	GATE_INFRA_AO2(CLK_INFRA_AO_SSPM_BUS_HCLK, "infra_ao_sspm_hclk", "axi_sel", 17),
	GATE_INFRA_AO2(CLK_INFRA_AO_APDMA_BCLK, "infra_ao_apdma_bclk", "axi_sel", 18),
	GATE_INFRA_AO2(CLK_INFRA_AO_SPI4, "infra_ao_spi4", "spi_sel", 25),
	GATE_INFRA_AO2(CLK_INFRA_AO_SPI5, "infra_ao_spi5", "spi_sel", 26),
	GATE_INFRA_AO2(CLK_INFRA_AO_CQ_DMA, "infra_ao_cq_dma", "axi_sel", 27),
	/* INFRA_AO3 */
	GATE_INFRA_AO3(CLK_INFRA_AO_MSDC0_SELF, "infra_ao_msdc0sf", "msdc50_0_sel", 0),
	GATE_INFRA_AO3(CLK_INFRA_AO_MSDC1_SELF, "infra_ao_msdc1sf", "msdc50_0_sel", 1),
	GATE_INFRA_AO3(CLK_INFRA_AO_MSDC2_SELF, "infra_ao_msdc2sf", "msdc50_0_sel", 2),
	GATE_INFRA_AO3(CLK_INFRA_AO_I2S_DMA, "infra_ao_i2s_dma", "axi_sel", 5),
	GATE_INFRA_AO3(CLK_INFRA_AO_AP_MSDC0, "infra_ao_ap_msdc0", "msdc50_0_sel", 7),
	GATE_INFRA_AO3(CLK_INFRA_AO_MD_MSDC0, "infra_ao_md_msdc0", "msdc50_0_sel", 8),
	GATE_INFRA_AO3(CLK_INFRA_AO_MSDC30_2, "infra_ao_msdc30_2", "msdc30_2_sel", 9),
	GATE_INFRA_AO3(CLK_INFRA_AO_GCPU, "infra_ao_gcpu", "gcpu_sel", 10),
	GATE_INFRA_AO3(CLK_INFRA_AO_PCIE_PERI_26M, "infra_ao_pcie_peri_26m", "clk26m", 15),
	GATE_INFRA_AO3(CLK_INFRA_AO_GCPU_66M_BCLK, "infra_ao_gcpu_66m_bclk", "axi_sel", 16),
	GATE_INFRA_AO3(CLK_INFRA_AO_GCPU_133M_BCLK, "infra_ao_gcpu_133m_bclk", "axi_sel", 17),
	GATE_INFRA_AO3(CLK_INFRA_AO_DISP_PWM1, "infra_ao_disp_pwm1", "disp_pwm1_sel", 20),
	GATE_INFRA_AO3(CLK_INFRA_AO_FBIST2FPC, "infra_ao_fbist2fpc", "msdc50_0_sel", 24),
	/* infra_ao_dapc_sync is for device access permission control module */
	GATE_INFRA_AO3_FLAGS(CLK_INFRA_AO_DEVICE_APC_SYNC, "infra_ao_dapc_sync",
			     "axi_sel", 25, CLK_IS_CRITICAL),
	GATE_INFRA_AO3(CLK_INFRA_AO_PCIE_P1_PERI_26M, "infra_ao_pcie_p1_peri_26m", "clk26m", 26),
	/* INFRA_AO4 */
	/* infra_ao_133m_mclk_set/infra_ao_66m_mclk_set are main clocks of peripheral */
	GATE_INFRA_AO4_FLAGS(CLK_INFRA_AO_133M_MCLK_CK, "infra_ao_133m_mclk_set",
			     "axi_sel", 0, CLK_IS_CRITICAL),
	GATE_INFRA_AO4_FLAGS(CLK_INFRA_AO_66M_MCLK_CK, "infra_ao_66m_mclk_set",
			     "axi_sel", 1, CLK_IS_CRITICAL),
	GATE_INFRA_AO4(CLK_INFRA_AO_PCIE_PL_P_250M_P0, "infra_ao_pcie_pl_p_250m_p0",
		       "pextp_pipe", 7),
	GATE_INFRA_AO4(CLK_INFRA_AO_RG_AES_MSDCFDE_CK_0P,
		       "infra_ao_aes_msdcfde_0p", "aes_msdcfde_sel", 18),
};

static const struct mtk_gate_regs peri_ao_cg_regs = {
	.set_ofs = 0x10,
	.clr_ofs = 0x14,
	.sta_ofs = 0x18,
};

#define GATE_PERI_AO(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &peri_ao_cg_regs, _shift, &mtk_clk_gate_ops_setclr)

static const struct mtk_gate peri_ao_clks[] = {
	GATE_PERI_AO(CLK_PERI_AO_ETHERNET, "peri_ao_ethernet", "axi_sel", 0),
	GATE_PERI_AO(CLK_PERI_AO_ETHERNET_BUS, "peri_ao_ethernet_bus", "axi_sel", 1),
	GATE_PERI_AO(CLK_PERI_AO_FLASHIF_BUS, "peri_ao_flashif_bus", "axi_sel", 3),
	GATE_PERI_AO(CLK_PERI_AO_FLASHIF_26M, "peri_ao_flashif_26m", "clk26m", 4),
	GATE_PERI_AO(CLK_PERI_AO_FLASHIFLASHCK, "peri_ao_flashiflashck", "spinor_sel", 5),
	GATE_PERI_AO(CLK_PERI_AO_SSUSB_2P_BUS, "peri_ao_ssusb_2p_bus", "usb_2p_sel", 9),
	GATE_PERI_AO(CLK_PERI_AO_SSUSB_2P_XHCI, "peri_ao_ssusb_2p_xhci", "ssusb_xhci_2p_sel", 10),
	GATE_PERI_AO(CLK_PERI_AO_SSUSB_3P_BUS, "peri_ao_ssusb_3p_bus", "usb_3p_sel", 11),
	GATE_PERI_AO(CLK_PERI_AO_SSUSB_3P_XHCI, "peri_ao_ssusb_3p_xhci", "ssusb_xhci_3p_sel", 12),
	GATE_PERI_AO(CLK_PERI_AO_SSUSB_BUS, "peri_ao_ssusb_bus", "usb_sel", 13),
	GATE_PERI_AO(CLK_PERI_AO_SSUSB_XHCI, "peri_ao_ssusb_xhci", "ssusb_xhci_sel", 14),
	GATE_PERI_AO(CLK_PERI_AO_ETHERNET_MAC, "peri_ao_ethernet_mac_clk", "snps_eth_250m_sel", 16),
	GATE_PERI_AO(CLK_PERI_AO_PCIE_P0_FMEM, "peri_ao_pcie_p0_fmem", "hd_466m_fmem_ck", 24),
};

static const struct mtk_gate_regs top0_cg_regs = {
	.set_ofs = 0x238,
	.clr_ofs = 0x238,
	.sta_ofs = 0x238,
};

static const struct mtk_gate_regs top1_cg_regs = {
	.set_ofs = 0x250,
	.clr_ofs = 0x250,
	.sta_ofs = 0x250,
};

#define GATE_TOP0(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &top0_cg_regs, _shift, &mtk_clk_gate_ops_no_setclr_inv)

#define GATE_TOP1(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &top1_cg_regs, _shift, &mtk_clk_gate_ops_no_setclr_inv)

static const struct mtk_gate top_clks[] = {
	/* TOP0 */
	GATE_TOP0(CLK_TOP_CFGREG_CLOCK_EN_VPP0, "cfgreg_clock_vpp0", "vpp_sel", 0),
	GATE_TOP0(CLK_TOP_CFGREG_CLOCK_EN_VPP1, "cfgreg_clock_vpp1", "vpp_sel", 1),
	GATE_TOP0(CLK_TOP_CFGREG_CLOCK_EN_VDO0, "cfgreg_clock_vdo0", "vpp_sel", 2),
	GATE_TOP0(CLK_TOP_CFGREG_CLOCK_EN_VDO1, "cfgreg_clock_vdo1", "vpp_sel", 3),
	GATE_TOP0(CLK_TOP_CFGREG_CLOCK_ISP_AXI_GALS, "cfgreg_clock_isp_axi_gals", "vpp_sel", 4),
	GATE_TOP0(CLK_TOP_CFGREG_F26M_VPP0, "cfgreg_f26m_vpp0", "clk26m", 5),
	GATE_TOP0(CLK_TOP_CFGREG_F26M_VPP1, "cfgreg_f26m_vpp1", "clk26m", 6),
	GATE_TOP0(CLK_TOP_CFGREG_F26M_VDO0, "cfgreg_f26m_vdo0", "clk26m", 7),
	GATE_TOP0(CLK_TOP_CFGREG_F26M_VDO1, "cfgreg_f26m_vdo1", "clk26m", 8),
	GATE_TOP0(CLK_TOP_CFGREG_AUD_F26M_AUD, "cfgreg_aud_f26m_aud", "clk26m", 9),
	GATE_TOP0(CLK_TOP_CFGREG_UNIPLL_SES, "cfgreg_unipll_ses", "univpll_d2", 15),
	GATE_TOP0(CLK_TOP_CFGREG_F_PCIE_PHY_REF, "cfgreg_f_pcie_phy_ref", "clk26m", 18),
	/* TOP1 */
	GATE_TOP1(CLK_TOP_SSUSB_TOP_REF, "ssusb_ref", "clk26m", 0),
	GATE_TOP1(CLK_TOP_SSUSB_PHY_REF, "ssusb_phy_ref", "clk26m", 1),
	GATE_TOP1(CLK_TOP_SSUSB_TOP_P1_REF, "ssusb_p1_ref", "clk26m", 2),
	GATE_TOP1(CLK_TOP_SSUSB_PHY_P1_REF, "ssusb_phy_p1_ref", "clk26m", 3),
	GATE_TOP1(CLK_TOP_SSUSB_TOP_P2_REF, "ssusb_p2_ref", "clk26m", 4),
	GATE_TOP1(CLK_TOP_SSUSB_PHY_P2_REF, "ssusb_phy_p2_ref", "clk26m", 5),
	GATE_TOP1(CLK_TOP_SSUSB_TOP_P3_REF, "ssusb_p3_ref", "clk26m", 6),
	GATE_TOP1(CLK_TOP_SSUSB_PHY_P3_REF, "ssusb_phy_p3_ref", "clk26m", 7),
};

static const struct mtk_gate_regs apmixed_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0x8,
	.sta_ofs = 0x8,
};

#define GATE_APMIXED(_id, _name, _parent, _shift)			\
	GATE_MTK(_id, _name, _parent, &apmixed_cg_regs, _shift, &mtk_clk_gate_ops_no_setclr_inv)

static const struct mtk_gate apmixed_clks[] = {
	GATE_APMIXED(CLK_APMIXED_PLL_SSUSB26M_EN, "pll_ssusb26m_en", "clk26m", 1),
};

#define MT8188_PLL_FMAX		(3800UL * MHZ)
#define MT8188_PLL_FMIN		(1500UL * MHZ)
#define MT8188_INTEGER_BITS	8

#define PLL(_id, _name, _reg, _pwr_reg, _en_mask, _flags,		\
	    _rst_bar_mask, _pcwbits, _pd_reg, _pd_shift,		\
	    _tuner_reg, _tuner_en_reg, _tuner_en_bit,			\
	    _pcw_reg, _pcw_shift, _pcw_chg_reg,				\
	    _en_reg, _pll_en_bit) {					\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.rst_bar_mask = _rst_bar_mask,				\
		.fmax = MT8188_PLL_FMAX,				\
		.fmin = MT8188_PLL_FMIN,				\
		.pcwbits = _pcwbits,					\
		.pcwibits = MT8188_INTEGER_BITS,			\
		.pd_reg = _pd_reg,					\
		.pd_shift = _pd_shift,					\
		.tuner_reg = _tuner_reg,				\
		.tuner_en_reg = _tuner_en_reg,				\
		.tuner_en_bit = _tuner_en_bit,				\
		.pcw_reg = _pcw_reg,					\
		.pcw_shift = _pcw_shift,				\
		.pcw_chg_reg = _pcw_chg_reg,				\
		.en_reg = _en_reg,					\
		.pll_en_bit = _pll_en_bit,				\
	}

static const struct mtk_pll_data plls[] = {
	PLL(CLK_APMIXED_ETHPLL, "ethpll", 0x044C, 0x0458, 0,
	    0, 0, 22, 0x0450, 24, 0, 0, 0, 0x0450, 0, 0, 0, 9),
	PLL(CLK_APMIXED_MSDCPLL, "msdcpll", 0x0514, 0x0520, 0,
	    0, 0, 22, 0x0518, 24, 0, 0, 0, 0x0518, 0, 0, 0, 9),
	PLL(CLK_APMIXED_TVDPLL1, "tvdpll1", 0x0524, 0x0530, 0,
	    0, 0, 22, 0x0528, 24, 0, 0, 0, 0x0528, 0, 0, 0, 9),
	PLL(CLK_APMIXED_TVDPLL2, "tvdpll2", 0x0534, 0x0540, 0,
	    0, 0, 22, 0x0538, 24, 0, 0, 0, 0x0538, 0, 0, 0, 9),
	PLL(CLK_APMIXED_MMPLL, "mmpll", 0x0544, 0x0550, 0xff000000,
	    HAVE_RST_BAR, BIT(23), 22, 0x0548, 24, 0, 0, 0, 0x0548, 0, 0, 0, 9),
	PLL(CLK_APMIXED_MAINPLL, "mainpll", 0x045C, 0x0468, 0xff000000,
	    HAVE_RST_BAR, BIT(23), 22, 0x0460, 24, 0, 0, 0, 0x0460, 0, 0, 0, 9),
	PLL(CLK_APMIXED_IMGPLL, "imgpll", 0x0554, 0x0560, 0,
	    0, 0, 22, 0x0558, 24, 0, 0, 0, 0x0558, 0, 0, 0, 9),
	PLL(CLK_APMIXED_UNIVPLL, "univpll", 0x0504, 0x0510, 0xff000000,
	    HAVE_RST_BAR, BIT(23), 22, 0x0508, 24, 0, 0, 0, 0x0508, 0, 0, 0, 9),
	PLL(CLK_APMIXED_ADSPPLL, "adsppll", 0x042C, 0x0438, 0,
	    0, 0, 22, 0x0430, 24, 0, 0, 0, 0x0430, 0, 0, 0, 9),
	PLL(CLK_APMIXED_APLL1, "apll1", 0x0304, 0x0314, 0,
	    0, 0, 32, 0x0308, 24, 0x0034, 0x0000, 12, 0x030C, 0, 0, 0, 9),
	PLL(CLK_APMIXED_APLL2, "apll2", 0x0318, 0x0328, 0,
	    0, 0, 32, 0x031C, 24, 0x0038, 0x0000, 13, 0x0320, 0, 0, 0, 9),
	PLL(CLK_APMIXED_APLL3, "apll3", 0x032C, 0x033C, 0,
	    0, 0, 32, 0x0330, 24, 0x003C, 0x0000, 14, 0x0334, 0, 0, 0, 9),
	PLL(CLK_APMIXED_APLL4, "apll4", 0x0404, 0x0414, 0,
	    0, 0, 32, 0x0408, 24, 0x0040, 0x0000, 15, 0x040C, 0, 0, 0, 9),
	PLL(CLK_APMIXED_APLL5, "apll5", 0x0418, 0x0428, 0,
	    0, 0, 32, 0x041C, 24, 0x0044, 0x0000, 16, 0x0420, 0, 0, 0, 9),
	PLL(CLK_APMIXED_MFGPLL, "mfgpll", 0x0340, 0x034C, 0,
	    0, 0, 22, 0x0344, 24, 0, 0, 0, 0x0344, 0, 0, 0, 9),
};

static int clk_mt8188_top_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	struct device_node *node = pdev->dev.of_node;
	int r;
	void __iomem *base;

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	clk_data = mtk_alloc_clk_data(CLK_TOP_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	mtk_clk_register_fixed_clks(top_fixed_clks, ARRAY_SIZE(top_fixed_clks),
				    clk_data);
	mtk_clk_register_factors(top_divs, ARRAY_SIZE(top_divs), clk_data);
	mtk_clk_register_muxes(top_mtk_muxes, ARRAY_SIZE(top_mtk_muxes), node,
			       &mt8188_clk_lock, clk_data);

	mtk_clk_register_composites(top_muxes, ARRAY_SIZE(top_muxes), base,
				    &mt8188_clk_lock, clk_data);
	mtk_clk_register_composites(top_adj_divs, ARRAY_SIZE(top_adj_divs), base,
				    &mt8188_clk_lock, clk_data);
	r = mtk_clk_register_gates(node, top_clks, ARRAY_SIZE(top_clks), clk_data);
	if (r)
		goto free_top_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		goto free_top_data;

	return r;

free_top_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static int clk_mt8188_infra_ao_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	struct device_node *node = pdev->dev.of_node;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_INFRA_AO_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	r = mtk_clk_register_gates(node, infra_ao_clks, ARRAY_SIZE(infra_ao_clks), clk_data);
	if (r)
		goto free_infra_ao_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		goto free_infra_ao_data;

	return r;

free_infra_ao_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static int clk_mt8188_apmixed_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	struct device_node *node = pdev->dev.of_node;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_APMIXED_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	mtk_clk_register_plls(node, plls, ARRAY_SIZE(plls), clk_data);
	r = mtk_clk_register_gates(node, apmixed_clks, ARRAY_SIZE(apmixed_clks), clk_data);
	if (r)
		goto free_apmixed_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		goto free_apmixed_data;

	return r;

free_apmixed_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static int clk_mt8188_peri_ao_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	struct device_node *node = pdev->dev.of_node;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_PERI_AO_NR_CLK);
	if (!clk_data)
		return -ENOMEM;

	r = mtk_clk_register_gates(node, peri_ao_clks, ARRAY_SIZE(peri_ao_clks), clk_data);
	if (r)
		goto free_peri_ao_data;

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		goto free_peri_ao_data;

	return r;

free_peri_ao_data:
	mtk_free_clk_data(clk_data);
	return r;
}

static const struct of_device_id of_match_clk_mt8188[] = {
	{
		.compatible = "mediatek,mt8188-apmixedsys",
		.data = clk_mt8188_apmixed_probe,
	}, {
		.compatible = "mediatek,mt8188-topckgen",
		.data = clk_mt8188_top_probe,
	}, {
		.compatible = "mediatek,mt8188-infracfg_ao",
		.data = clk_mt8188_infra_ao_probe,
	}, {
		.compatible = "mediatek,mt8188-pericfg_ao",
		.data = clk_mt8188_peri_ao_probe,
	}, {
		/* sentinel */
	}
};

static int clk_mt8188_probe(struct platform_device *pdev)
{
	int (*clk_probe)(struct platform_device *pdev);
	int r;

	clk_probe = of_device_get_match_data(&pdev->dev);
	if (!clk_probe)
		return -EINVAL;

	r = clk_probe(pdev);
	if (r)
		dev_err(&pdev->dev,
			"could not register clock provider: %s: %d\n",
			pdev->name, r);

	return r;
}

static struct platform_driver clk_mt8188_drv = {
	.probe = clk_mt8188_probe,
	.driver = {
		.name = "clk-mt8188",
		.of_match_table = of_match_clk_mt8188,
	},
};

module_platform_driver(clk_mt8188_drv);
MODULE_LICENSE("GPL");
