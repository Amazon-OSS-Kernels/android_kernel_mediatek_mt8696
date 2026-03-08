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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "clk-mtk.h"
#include "clk-gate.h"
#include "clk-mux.h"

#include <dt-bindings/clock/mt8696-clk.h>

static DEFINE_SPINLOCK(mt8696_clk_lock);

static const struct mtk_fixed_clk top_fixed_clks[] = {
	FIXED_CLK(CLK_TOP_NULL, "clk_null", NULL, 0),
};

static const struct mtk_fixed_factor top_divs[] = {
	FACTOR(CLK_TOP_SYSPLL_D2, "syspll_d2", "mainpll", 1, 2),
	FACTOR(CLK_TOP_SYSPLL1_D2, "syspll1_d2", "syspll_d2", 1, 2),
	FACTOR(CLK_TOP_SYSPLL1_D4, "syspll1_d4", "syspll_d2", 1, 4),
	FACTOR(CLK_TOP_SYSPLL1_D8, "syspll1_d8", "syspll_d2", 1, 8),
	FACTOR(CLK_TOP_SYSPLL1_D16, "syspll1_d16", "syspll_d2", 1, 16),
	FACTOR(CLK_TOP_SYSPLL1_D32, "syspll1_d32", "syspll_d2", 1, 32),
	FACTOR(CLK_TOP_SYSPLL_D3, "syspll_d3", "mainpll", 1, 3),
	FACTOR(CLK_TOP_SYSPLL2_D2, "syspll2_d2", "syspll_d3", 1, 2),
	FACTOR(CLK_TOP_SYSPLL2_D4, "syspll2_d4", "syspll_d3", 1, 4),
	FACTOR(CLK_TOP_SYSPLL2_D8, "syspll2_d8", "syspll_d3", 1, 8),
	FACTOR(CLK_TOP_SYSPLL_D5, "syspll_d5", "mainpll", 1, 5),
	FACTOR(CLK_TOP_SYSPLL3_D2, "syspll3_d2", "syspll_d5", 1, 2),
	FACTOR(CLK_TOP_SYSPLL3_D4, "syspll3_d4", "syspll_d5", 1, 4),
	FACTOR(CLK_TOP_SYSPLL3_D8, "syspll3_d8", "syspll_d5", 1, 8),
	FACTOR(CLK_TOP_SYSPLL_D7, "syspll_d7", "mainpll", 1, 7),
	FACTOR(CLK_TOP_SYSPLL4_D4, "syspll4_d4", "syspll_d7", 1, 4),
	FACTOR(CLK_TOP_SYSPLL4_D8, "syspll4_d8", "syspll_d7", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL_D2, "univpll_d2", "univpll", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL1_D2, "univpll1_d2", "univpll_d2", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL1_D4, "univpll1_d4", "univpll_d2", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL1_D8, "univpll1_d8", "univpll_d2", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL1_D16, "univpll1_d16", "univpll_d2", 1, 16),
	FACTOR(CLK_TOP_UNIVPLL_D3, "univpll_d3", "univpll", 1, 3),
	FACTOR(CLK_TOP_UNIVPLL2_D2, "univpll2_d2", "univpll_d3", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL2_D4, "univpll2_d4", "univpll_d3", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL2_D8, "univpll2_d8", "univpll_d3", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL2_D16, "univpll2_d16", "univpll_d3", 1, 16),
	FACTOR(CLK_TOP_UNIVPLL_D5, "univpll_d5", "univpll", 1, 5),
	FACTOR(CLK_TOP_UNIVPLL3_D2, "univpll3_d2", "univpll_d5", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL3_D4, "univpll3_d4", "univpll_d5", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL3_D8, "univpll3_d8", "univpll_d5", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL3_D16, "univpll3_d16", "univpll_d5", 1, 16),
	FACTOR(CLK_TOP_UNIVPLL_D7, "univpll_d7", "univpll", 1, 7),
	FACTOR(CLK_TOP_UNIVPLL4_D2, "univpll4_d2", "univpll_d7", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL_D26, "univpll_d26", "univpll", 1, 52),
	FACTOR(CLK_TOP_UNIVPLL_D52, "univpll_d52", "univpll", 1, 104),
	FACTOR(CLK_TOP_APLL1_D2, "apll1_d2", "apll1", 1, 2),
	FACTOR(CLK_TOP_APLL1_D4, "apll1_d4", "apll1", 1, 4),
	FACTOR(CLK_TOP_APLL1_D8, "apll1_d8", "apll1", 1, 8),
	FACTOR(CLK_TOP_APLL1_D16, "apll1_d16", "apll1", 1, 16),
	FACTOR(CLK_TOP_APLL1_D3, "apll1_d3", "apll1", 1, 3),
	FACTOR(CLK_TOP_APLL2_D2, "apll2_d2", "apll2", 1, 2),
	FACTOR(CLK_TOP_APLL2_D4, "apll2_d4", "apll2", 1, 4),
	FACTOR(CLK_TOP_APLL2_D8, "apll2_d8", "apll2", 1, 8),
	FACTOR(CLK_TOP_APLL2_D16, "apll2_d16", "apll2", 1, 16),
	FACTOR(CLK_TOP_APLL2_D3, "apll2_d3", "apll2", 1, 3),
	FACTOR(CLK_TOP_APLL3_D2, "apll3_d2", "apll3", 1, 2),
	FACTOR(CLK_TOP_APLL3_D4, "apll3_d4", "apll3", 1, 4),
	FACTOR(CLK_TOP_APLL3_D8, "apll3_d8", "apll3", 1, 8),
	FACTOR(CLK_TOP_APLL3_D16, "apll3_d16", "apll3", 1, 16),
	FACTOR(CLK_TOP_APLL3_D3, "apll3_d3", "apll3", 1, 3),
	FACTOR(CLK_TOP_APLL4_D2, "apll4_d2", "apll4", 1, 2),
	FACTOR(CLK_TOP_APLL4_D4, "apll4_d4", "apll4", 1, 4),
	FACTOR(CLK_TOP_APLL4_D8, "apll4_d8", "apll4", 1, 8),
	FACTOR(CLK_TOP_APLL4_D16, "apll4_d16", "apll4", 1, 16),
	FACTOR(CLK_TOP_APLL4_D3, "apll4_d3", "apll4", 1, 3),
	FACTOR(CLK_TOP_APLL5_D2, "apll5_d2", "apll5", 1, 2),
	FACTOR(CLK_TOP_APLL5_D4, "apll5_d4", "apll5", 1, 4),
	FACTOR(CLK_TOP_APLL5_D8, "apll5_d8", "apll5", 1, 8),
	FACTOR(CLK_TOP_APLL5_D16, "apll5_d16", "apll5", 1, 16),
	FACTOR(CLK_TOP_APLL5_D3, "apll5_d3", "apll5", 1, 3),
	FACTOR(CLK_TOP_HDMIRX_APLL_D2, "hdmirx_apll_d2", "hdmirx_apll", 1, 2),
	FACTOR(CLK_TOP_HDMIRX_APLL_D4, "hdmirx_apll_d4", "hdmirx_apll", 1, 4),
	FACTOR(CLK_TOP_HDMIRX_APLL_D8, "hdmirx_apll_d8", "hdmirx_apll", 1, 8),
	FACTOR(CLK_TOP_HDMIRX_APLL_D16, "hdmirx_apll_d16", "hdmirx_apll",
		1, 16),
	FACTOR(CLK_TOP_HDMIRX_APLL_D3, "hdmirx_apll_d3", "hdmirx_apll", 1, 3),
	FACTOR(CLK_TOP_HDMIRX_APLL_D6, "hdmirx_apll_d6", "hdmirx_apll", 1, 6),
	FACTOR(CLK_TOP_OSDPLL_D2, "osdpll_d2", "osdpll", 1, 2),
	FACTOR(CLK_TOP_OSDPLL_D4, "osdpll_d4", "osdpll", 1, 4),
	FACTOR(CLK_TOP_OSDPLL_D8, "osdpll_d8", "osdpll", 1, 8),
	FACTOR(CLK_TOP_OSDPLL_D3, "osdpll_d3", "osdpll", 1, 3),
	FACTOR(CLK_TOP_OSDPLL1_D2, "osdpll1_d2", "osdpll_d3", 1, 2),
	FACTOR(CLK_TOP_OSDPLL1_D4, "osdpll1_d4", "osdpll_d3", 1, 4),
	FACTOR(CLK_TOP_OSDPLL1_D8, "osdpll1_d8", "osdpll_d3", 1, 8),
	FACTOR(CLK_TOP_OSDPLL1_D16, "osdpll1_d16", "osdpll_d3", 1, 16),
	FACTOR(CLK_TOP_ETHERPLL_500M, "etherpll_500m", "etherpll", 1, 1),
	FACTOR(CLK_TOP_ETHERPLL_250M, "etherpll_250m", "etherpll_500m", 1, 2),
	FACTOR(CLK_TOP_ETHERPLL_125M, "etherpll_125m", "etherpll_500m", 1, 4),
	FACTOR(CLK_TOP_ETHERPLL_50M, "etherpll_50m", "etherpll_500m", 1, 10),
	FACTOR(CLK_TOP_VDECPLL_D2, "vdecpll_d2", "vdecpll", 1, 2),
	FACTOR(CLK_TOP_TVDPLL_D2, "tvdpll_d2", "tvdpll", 1, 2),
	FACTOR(CLK_TOP_TVDPLL_D4, "tvdpll_d4", "tvdpll", 1, 4),
	FACTOR(CLK_TOP_TVDPLL_D3, "tvdpll_d3", "tvdpll", 1, 3),
	FACTOR(CLK_TOP_TVDPLL_D6, "tvdpll_d6", "tvdpll", 1, 6),
	FACTOR(CLK_TOP_MSDCPLL_D2, "msdcpll_d2", "msdcpll", 1, 2),
	FACTOR(CLK_TOP_MSDCPLL_D4, "msdcpll_d4", "msdcpll", 1, 4),
	FACTOR(CLK_TOP_MSDCPLL_D8, "msdcpll_d8", "msdcpll", 1, 8),
	FACTOR(CLK_TOP_HDMITX_PIXEL, "hdmitx_pixel", "clk_null", 1, 1),
	FACTOR(CLK_TOP_HDMITX_PIXEL_D2, "hdmitx_pixel_d2", "hdmitx_pixel",
		1, 2),
	FACTOR(CLK_TOP_HDMITX_PIXEL_D4, "hdmitx_pixel_d4", "hdmitx_pixel",
		1, 4),
	FACTOR(CLK_TOP_HDMITX_PIXEL_D3, "hdmitx_pixel_d3", "hdmitx_pixel",
		1, 3),
	FACTOR(CLK_TOP_HDMITX_PIXEL_D6, "hdmitx_pixel_d6", "hdmitx_pixel",
		1, 6),
	FACTOR(CLK_TOP_CLKRTC_INT, "clkrtc_int", "clk26m", 1, 813),
	FACTOR(CLK_TOP_CLKRTC_EXT, "clkrtc_ext", "clk_null", 1, 1),
	FACTOR(CLK_TOP_DMPLL, "dmpll", "clk_null", 1, 1),
	FACTOR(CLK_TOP_DMPLL_D2, "dmpll_d2", "dmpll", 1, 2),
};

static const char * const mcu_cpu_pll_parents[] = {
	"clk26m",
	"armpll",
	"syspll_d2",
	"univpll1_d2"
};

static const char * const mcu_buspll_parents[] = {
	"clk26m",
	"ccipll",
	"syspll_d2",
	"univpll1_d2"
};

static struct mtk_composite mcu_muxes[] = {
	/* CPU_PLLDIV_CFG0 */
	MUX_GATE_FLAGS(CLK_MCU_CPU_PLL_SEL, "mcu_cpu_pll_sel",
		mcu_cpu_pll_parents, 0x02A0, 9, 2, -1, CLK_IS_CRITICAL),
	/* BUS_PLLDIV_CFG */
	MUX_GATE_FLAGS(CLK_MCU_BUSPLL_SEL, "mcu_buspll_sel",
		mcu_buspll_parents, 0x02E0, 9, 2, -1, CLK_IS_CRITICAL),
};

static const char * const axi_parents[] = {
	"clk26m",
	"syspll1_d4",
	"syspll3_d2",
	"syspll1_d8",
	"univpll3_d2",
	"univpll2_d4",
	"syspll2_d4",
	"dmpll_d2"
};

static const char * const scp_parents[] = {
	"clk26m",
	"univpll1_d16"
};

static const char * const memsel_parents[] = {
	"clk26m",
	"dmpll",
	"apll1"
};

static const char * const memref_104_parents[] = {
	"clk26m",
	"univpll2_d8"
};

static const char * const mem52m_parents[] = {
	"clk26m",
	"univpll2_d16"
};

static const char * const sd_parents[] = {
	"clk26m",
	"osdpll1_d4",
	"tvdpll_d6",
	"hdmitx_pixel_d6"
};

static const char * const mmsel_parents[] = {
	"clk26m",
	"msdcpll_d2",
	"syspll2_d2",
	"syspll1_d4",
	"syspll3_d2",
	"syspll1_d8",
	"univpll1_d4",
	"univpll2_d4",
	"dmpll"
};

static const char * const vdec_parents[] = {
	"clk26m",
	"vdecpll",
	"osdpll_d4",
	"univpll2_d2",
	"msdcpll_d2",
	"syspll2_d2",
	"univpll1_d4",
	"syspll3_d4",
	"syspll1_d2"
};

static const char * const venc_parents[] = {
	"clk26m",
	"univpll1_d4",
	"msdcpll_d2",
	"syspll2_d2",
	"syspll1_d4",
	"univpll3_d2",
	"syspll3_d2",
	"syspll_d5",
	"univpll2_d2"
};

static const char * const mfg_parents[] = {
	"clk26m",
	"mmpll",
	"dmpll",
	"syspll1_d2",
	"univpll1_d2",
	"vdecpll",
	"univpll_d3",
	"syspll_d3",
	"msdcpll_d2",
	"syspll2_d2",
	"syspll1_d4",
	"msdcpll_d4",
	"univpll2_d2",
	"univpll1_d4",
	"univpll3_d2"
};

static const char * const rsz_parents[] = {
	"clk26m",
	"syspll1_d2",
	"tvdpll",
	"vdecpll_d2",
	"univpll2_d2",
	"osdpll_d4",
	"univpll3_d2",
	"apll5"
};

static const char * const spi_parents[] = {
	"clk26m",
	"univpll2_d8",
	"univpll1_d8",
	"univpll2_d4",
	"univpll3_d4",
	"syspll2_d4",
	"syspll1_d8",
	"univpll1_d16"
};

static const char * const usb20_parents[] = {
	"clk26m",
	"univpll1_d16",
	"univpll3_d8"
};

static const char * const usb30_parents[] = {
	"clk26m",
	"univpll3_d4",
	"univpll3_d8",
	"univpll2_d8"
};

static const char * const msdc50_0_hc_parents[] = {
	"clk26m",
	"syspll1_d4",
	"syspll2_d4",
	"syspll4_d4"
};

static const char * const msdc50_0_parents[] = {
	"clk26m",
	"msdcpll_d2",
	"msdcpll_d4",
	"osdpll_d4",
	"syspll2_d4",
	"msdcpll_d8",
	"univpll2_d4",
	"univpll1_d4"
};

static const char * const msdc30_1_parents[] = {
	"clk26m",
	"univpll2_d4",
	"msdcpll_d4",
	"univpll1_d8",
	"syspll2_d4",
	"msdcpll_d8",
	"syspll1_d8",
	"univpll1_d16"
};

static const char * const msdc30_2_parents[] = {
	"clk26m",
	"msdcpll_d2",
	"msdcpll_d4",
	"univpll1_d8",
	"syspll2_d4",
	"univpll4_d2",
	"univpll2_d2",
	"univpll2_d4"
};

static const char * const msdc0p_aessel_parents[] = {
	"clk26m",
	"msdcpll_d2",
	"syspll1_d2",
	"univpll2_d4"
};

static const char * const intdir_parents[] = {
	"clk26m",
	"univpll2_d2",
	"syspll1_d2",
	"osdpll_d3",
	"univpll1_d2"
};

static const char * const audio_parents[] = {
	"clk26m",
	"syspll3_d8",
	"syspll4_d8",
	"syspll1_d32"
};

static const char * const audio_26msel_parents[] = {
	"clk26m",
	"clk26md2"
};

static const char * const aud_intbussel_parents[] = {
	"clk26m",
	"syspll1_d8",
	"syspll4_d4",
	"univpll3_d4",
	"univpll2_d16",
	"syspll3_d4",
	"syspll3_d8"
};

static const char * const apll_parents[] = {
	"clk26m",
	"apll1",
	"apll1_d2",
	"apll1_d3",
	"apll1_d4",
	"apll1_d8",
	"apll1_d16",
	"apll3",
	"clk26md2"
};

static const char * const apll2_parents[] = {
	"clk26m",
	"apll2",
	"apll2_d2",
	"apll2_d3",
	"apll2_d4",
	"apll2_d8",
	"apll2_d16",
	"clk26md2"
};

static const char * const apll3_parents[] = {
	"clk26m",
	"apll3",
	"apll3_d2",
	"apll3_d3",
	"apll3_d4",
	"apll3_d8",
	"apll3_d16",
	"clk26md2"
};

static const char * const apll4_parents[] = {
	"clk26m",
	"apll4",
	"apll4_d2",
	"apll4_d3",
	"apll4_d4",
	"apll4_d8",
	"apll4_d16",
	"clk26md2"
};

static const char * const apll5_parents[] = {
	"clk26m",
	"apll5",
	"apll5_d2",
	"apll5_d3",
	"apll5_d4",
	"apll5_d8",
	"apll5_d16",
	"clk26md2"
};

static const char * const apll6_parents[] = {
	"clk26m",
	"hdmirx_apll",
	"hdmirx_apll_d2",
	"hdmirx_apll_d3",
	"hdmirx_apll_d4",
	"hdmirx_apll_d8",
	"hdmirx_apll_d16",
	"clk26md2"
};

static const char * const a1syshp_parents[] = {
	"clk26m",
	"apll1_d2",
	"apll1_d3",
	"apll1_d4",
	"apll1_d8",
	"apll1_d16"
};

static const char * const a2syshp_parents[] = {
	"clk26m",
	"apll2_d2",
	"apll2_d3",
	"apll2_d4",
	"apll2_d8",
	"apll2_d16"
};

static const char * const a3syshp_parents[] = {
	"clk26m",
	"apll3_d2",
	"apll3_d3",
	"apll3_d4",
	"apll3_d8",
	"apll3_d16",
	"apll4_d2",
	"apll4_d3",
	"apll4_d4",
	"apll4_d8",
	"apll4_d16",
	"apll5_d2",
	"apll5_d3",
	"apll5_d4",
	"apll5_d8",
	"apll5_d16",
	"hdmirx_apll_d2",
	"hdmirx_apll_d3",
	"hdmirx_apll_d4",
	"hdmirx_apll_d8",
	"hdmirx_apll_d6",
	"splin_mck_i"
};

static const char * const a4syshp_parents[] = {
	"clk26m",
	"apll3_d2",
	"apll3_d3",
	"apll3_d4",
	"apll3_d8",
	"apll3_d16",
	"apll4_d2",
	"apll4_d3",
	"apll4_d4",
	"apll4_d8",
	"apll4_d16",
	"apll5_d2",
	"apll5_d3",
	"apll5_d4",
	"apll5_d8",
	"apll5_d16",
	"hdmirx_apll_d2",
	"hdmirx_apll_d3",
	"hdmirx_apll_d4",
	"hdmirx_apll_d8",
	"hdmirx_apll_d6",
	"i2si0_mck_i"
};

static const char * const asml_parents[] = {
	"clk26m",
	"univpll2_d8",
	"univpll2_d4",
	"syspll3_d2"
};

static const char * const aud_iec_parents[] = {
	"clk26m",
	"apll1",
	"apll2",
	"apll3",
	"apll4",
	"apll5",
	"hdmirx_apll"
};

static const char * const rv33_parents[] = {
	"clk26m",
	"univpll2_d8",
	"univpll1_d4",
	"syspll1_d2",
	"univpll1_d2",
	"syspll_d3",
	"osdpll_d2",
	"apll5"
};

static const char * const stc_top_27msel_parents[] = {
	"clk26m",
	"osdpll1_d16",
	"vdecpll_27m"
};

static const char * const osd_parents[] = {
	"clk26m",
	"osdpll_d2",
	"osdpll_d4",
	"osdpll_d8",
	"osdpll1_d4",
	"osdpll1_d8",
	"univpll1_d2",
	"univpll1_d4",
	"tvdpll",
	"tvdpll_d2",
	"tvdpll_d4",
	"hdmitx_pixel",
	"hdmitx_pixel_d2",
	"hdmitx_pixel_d4"
};

static const char * const vdo3_parents[] = {
	"clk26m",
	"osdpll_d2",
	"tvdpll",
	"tvdpll_d2",
	"osdpll_d4",
	"osdpll1_d2",
	"osdpll_d8",
	"hdmitx_pixel",
	"hdmitx_pixel_d2",
	"hdmitx_pixel_d3",
	"hdmitx_pixel_d4"
};

static const char * const vdo4_parents[] = {
	"clk26m",
	"osdpll1_d2",
	"osdpll_d8",
	"hdmitx_pixel_d3",
	"hdmitx_pixel_d4",
	"tvdpll_d3",
	"tvdpll_d4"
};

static const char * const hd_parents[] = {
	"clk26m",
	"tvdpll",
	"hdmitx_pixel"
};

static const char * const nr_parents[] = {
	"clk26m",
	"univpll1_d8",
	"syspll2_d4",
	"syspll1_d8",
	"univpll1_d16",
	"univpll3_d4",
	"univpll2_d4",
	"syspll3_d2"
};

static const char * const pe2_mac_p0_parents[] = {
	"clk26m",
	"syspll1_d16",
	"syspll4_d4",
	"syspll2_d8",
	"univpll2_d8",
	"syspll3_d4"
};

static const char * const hdcp_parents[] = {
	"clk26m",
	"syspll4_d4",
	"syspll3_d8",
	"univpll2_d8"
};

static const char * const hdcp_24msel_parents[] = {
	"clk26m",
	"univpll_d26",
	"univpll_d52",
	"univpll2_d16"
};

static const char * const rtc_parents[] = {
	"clkrtc_int",
	"clkrtc_ext",
	"clk26m",
	"univpll3_d16"
};

static const char * const spinor_parents[] = {
	"clk26m",
	"clk26md2",
	"syspll4_d8",
	"univpll2_d16",
	"univpll3_d8"
};

static const char * const eth_250msel_parents[] = {
	"clk26m",
	"etherpll_250m",
	"univpll3_d2"
};

static const char * const eth_125msel_parents[] = {
	"clk26m",
	"etherpll_125m",
	"univpll3_d4"
};

static const char * const eth_50mrmsel_parents[] = {
	"clk26m",
	"etherpll_50m",
	"univpll_d26"
};

static const char * const i2c_parents[] = {
	"clk26m",
	"univpll_d26",
	"univpll2_d8",
	"univpll3_d4",
	"univpll1_d8"
};

static const char * const pwminfra_parents[] = {
	"clk26m",
	"univpll2_d8",
	"univpll3_d4",
	"univpll1_d8"
};

static const char * const gcpu_parents[] = {
	"clk26m",
	"syspll2_d2",
	"syspll1_d4",
	"univpll1_d4",
	"univpll3_d2",
	"univpll3_d4",
	"univpll2_d2"
};

static const char * const ecc_parents[] = {
	"clk26m",
	"univpll2_d4",
	"univpll1_d4",
	"univpll2_d2",
	"syspll1_d2",
	"univpll1_d2"
};

static const char * const di_parents[] = {
	"clk26m",
	"osdpll_d4",
	"syspll1_d4",
	"univpll1_d16",
	"osdpll1_d2",
	"osdpll_d8",
	"osdpll1_d4",
	"osdpll1_d8",
	"hdmitx_pixel_d2",
	"hdmitx_pixel_d3",
	"hdmitx_pixel_d4",
	"hdmitx_pixel_d6"
};

static const char * const nfi2x_parents[] = {
	"clk26m",
	"syspll2_d8",
	"univpll1_d8",
	"syspll2_d4",
	"msdcpll_d4",
	"univpll1_d4",
	"univpll3_d2",
	"syspll2_d2"
};

static const char * const spinfi_parents[] = {
	"clk26m",
	"univpll2_d16",
	"univpll3_d8",
	"syspll1_d16",
	"syspll4_d4",
	"syspll2_d8",
	"univpll2_d8",
	"univpll3_d4"
};

static const char * const hd20_dacr_parents[] = {
	"clk26m",
	"univpll1_d4",
	"univpll1_d8",
	"univpll1_d16"
};

static const char * const hd20_hdcp_parents[] = {
	"clk26m",
	"univpll2_d8",
	"univpll1_d16",
	"univpll2_d16"
};

static const char * const nna0_parents[] = {
	"clk26m",
	"univpll_d3",
	"syspll_d3",
	"univpll1_d2",
	"univpll2_d2",
	"syspll1_d2",
	"syspll2_d2",
	"univpll2_d4",
	"univpll2_d8",
	"univpll2_d16",
	"univpll1_d4",
	"univpll1_d8",
	"msdcpll",
	"osdpll_d2",
	"apll5"
};

static const char * const hdmi_apb_parents[] = {
	"clk26m",
	"univpll2_d8",
	"univpll2_d4"
};

static const char * const cupmsel_parents[] = {
	"clk26m",
	"syspll2_d8",
	"syspll2_d4"
};

static const char * const apll1_ref_parents[] = {
	"i2si0_mck_i",
	"splin_mck_i"
};

static struct mtk_composite top_misc_muxes[] = {
	/* CLK_AUDDIV_4 */
	MUX_GATE(CLK_TOP_APLL1_REF_SEL, "apll1_ref_sel", apll1_ref_parents,
	    0x338, 8, 1, -1),
	MUX_GATE(CLK_TOP_APLL2_REF_SEL, "apll2_ref_sel", apll1_ref_parents,
	    0x338, 9, 1, -1),
	MUX_GATE(CLK_TOP_APLL3_REF_SEL, "apll3_ref_sel", apll1_ref_parents,
	    0x338, 10, 1, -1),
	MUX_GATE(CLK_TOP_APLL4_REF_SEL, "apll4_ref_sel", apll1_ref_parents,
	    0x338, 11, 1, -1),
	MUX_GATE(CLK_TOP_APLL5_REF_SEL, "apll5_ref_sel", apll1_ref_parents,
	    0x338, 12, 1, -1),
	MUX_GATE(CLK_TOP_HDMIRX_APLL_SEL, "hdmirx_apll_sel", apll1_ref_parents,
	    0x338, 13, 1, -1),
};

static struct mtk_mux top_muxes[] = {
	/* CLK_CFG_0 */
	MUX_CLR_SET_UPD_FLAGS(CLK_TOP_AXI_SEL, "axi_sel",
	    axi_parents, 0x010, 0x014, 0x018, 0, 3, 7,
	    0x4, 0, CLK_IS_CRITICAL),
	MUX_CLR_SET_UPD_FLAGS(CLK_TOP_SCP_SEL, "scp_sel",
	    scp_parents, 0x010, 0x014, 0x018, 8, 1, 15,
	    0x4, 1, CLK_IS_CRITICAL),
	MUX_CLR_SET_UPD_FLAGS(CLK_TOP_MEMSEL, "memsel",
	    memsel_parents, 0x010, 0x014, 0x018, 16, 2, 23,
	    0x4, 2, CLK_IS_CRITICAL),
	MUX_CLR_SET_UPD_FLAGS(CLK_TOP_MEMREF_104_SEL, "memref_104_sel",
	    memref_104_parents, 0x010, 0x014, 0x018, 24, 1, 31, 0x4, 3,
	    CLK_IS_CRITICAL),
	/* CLK_CFG_1 */
	MUX_CLR_SET_UPD_FLAGS(CLK_TOP_MEM52M_SEL, "mem52m_sel",
	    mem52m_parents, 0x020, 0x024, 0x028, 0, 1, 7, 0x4, 4,
	    CLK_IS_CRITICAL),
	MUX_CLR_SET_UPD(CLK_TOP_SD_SEL, "sd_sel",
	    sd_parents, 0x020, 0x024, 0x028, 8, 2, 15, 0x4, 5),
	MUX_CLR_SET_UPD(CLK_TOP_MMSEL, "mmsel",
	    mmsel_parents, 0x020, 0x024, 0x028, 16, 4, 23, 0x4, 6),
	MUX_CLR_SET_UPD(CLK_TOP_VDEC_SEL, "vdec_sel",
	    vdec_parents, 0x020, 0x024, 0x028, 24, 4, 31, 0x4, 7),
	/* CLK_CFG_2 */
	MUX_CLR_SET_UPD(CLK_TOP_VDEC_SLOW_SEL, "vdec_slow_sel",
	    vdec_parents, 0x030, 0x034, 0x038, 0, 4, 7, 0x4, 8),
	MUX_CLR_SET_UPD(CLK_TOP_VENC_SEL, "venc_sel",
	    venc_parents, 0x030, 0x034, 0x038, 8, 4, 15, 0x4, 9),
	MUX_CLR_SET_UPD(CLK_TOP_MFG_SEL, "mfg_sel",
	    mfg_parents, 0x030, 0x034, 0x038, 16, 4, 23, 0x4, 10),
	MUX_CLR_SET_UPD(CLK_TOP_RSZ_SEL, "rsz_sel",
	    rsz_parents, 0x030, 0x034, 0x038, 24, 3, 31, 0x4, 11),
	/* CLK_CFG_3 */
	MUX_CLR_SET_UPD(CLK_TOP_UART_SEL, "uart_sel",
	    mem52m_parents, 0x040, 0x044, 0x048, 0, 1, 7, 0x4, 12),
	MUX_CLR_SET_UPD(CLK_TOP_SPI_SEL, "spi_sel",
	    spi_parents, 0x040, 0x044, 0x048, 8, 3, 15, 0x4, 13),
	MUX_CLR_SET_UPD(CLK_TOP_USB20_SEL, "usb20_sel",
	    usb20_parents, 0x040, 0x044, 0x048, 16, 2, 23, 0x4, 14),
	MUX_CLR_SET_UPD(CLK_TOP_USB30_SEL, "usb30_sel",
	    usb30_parents, 0x040, 0x044, 0x048, 24, 2, 31, 0x4, 15),
	/* CLK_CFG_4 */
	MUX_CLR_SET_UPD(CLK_TOP_MSDC50_0_HC_SEL, "msdc50_0_hc_sel",
	    msdc50_0_hc_parents, 0x050, 0x054, 0x058, 0, 2, 7, 0x4, 16),
	MUX_CLR_SET_UPD(CLK_TOP_MSDC50_0_SEL, "msdc50_0_sel",
	    msdc50_0_parents, 0x050, 0x054, 0x058, 8, 3, 15, 0x4, 17),
	MUX_CLR_SET_UPD(CLK_TOP_MSDC30_1_SEL, "msdc30_1_sel",
	    msdc30_1_parents, 0x050, 0x054, 0x058, 16, 3, 23, 0x4, 18),
	MUX_CLR_SET_UPD(CLK_TOP_MSDC30_2_SEL, "msdc30_2_sel",
	    msdc30_2_parents, 0x050, 0x054, 0x058, 24, 3, 31, 0x4, 19),
	/* CLK_CFG_5 */
	MUX_CLR_SET_UPD(CLK_TOP_MSDC50_2_HC_SEL, "msdc50_2_hc_sel",
	    msdc50_0_hc_parents, 0x060, 0x064, 0x068, 0, 2, 7, 0x4, 20),
	MUX_CLR_SET_UPD(CLK_TOP_MSDC0P_AESSEL, "msdc0p_aessel",
	    msdc0p_aessel_parents, 0x060, 0x064, 0x068, 8, 2, 15, 0x4, 21),
	MUX_CLR_SET_UPD(CLK_TOP_INTDIR_SEL, "intdir_sel",
	    intdir_parents, 0x060, 0x064, 0x068, 16, 3, 23, 0x4, 22),
	MUX_CLR_SET_UPD(CLK_TOP_AUDIO_SEL, "audio_sel",
	    audio_parents, 0x060, 0x064, 0x068, 24, 2, 31, 0x4, 23),
	/* CLK_CFG_6 */
	MUX_CLR_SET_UPD(CLK_TOP_AUDIO_26MSEL, "audio_26msel",
	    audio_26msel_parents, 0x070, 0x074, 0x078, 0, 1, 7, 0x4, 24),
	MUX_CLR_SET_UPD(CLK_TOP_AUD_INTBUSSEL, "aud_intbussel",
	    aud_intbussel_parents, 0x070, 0x074, 0x078, 8, 3, 15, 0x4, 25),
	MUX_CLR_SET_UPD(CLK_TOP_APLL_SEL, "apll_sel",
	    apll_parents, 0x070, 0x074, 0x078, 16, 4, 23, 0x4, 26),
	MUX_CLR_SET_UPD(CLK_TOP_APLL2_SEL, "apll2_sel",
	    apll2_parents, 0x070, 0x074, 0x078, 24, 3, 31, 0x4, 27),
	/* CLK_CFG_7 */
	MUX_CLR_SET_UPD(CLK_TOP_APLL3_SEL, "apll3_sel",
	    apll3_parents, 0x080, 0x084, 0x088, 0, 3, 7, 0x4, 28),
	MUX_CLR_SET_UPD(CLK_TOP_APLL4_SEL, "apll4_sel",
	    apll4_parents, 0x080, 0x084, 0x088, 8, 3, 15, 0x4, 29),
	MUX_CLR_SET_UPD(CLK_TOP_APLL5_SEL, "apll5_sel",
	    apll5_parents, 0x080, 0x084, 0x088, 16, 3, 23, 0x4, 30),
	MUX_CLR_SET_UPD(CLK_TOP_APLL6_SEL, "apll6_sel",
	    apll6_parents, 0x080, 0x084, 0x088, 24, 3, 31, 0x8, 0),
	/* CLK_CFG_8 */
	MUX_CLR_SET_UPD(CLK_TOP_A1SYSHP_SEL, "a1syshp_sel",
	    a1syshp_parents, 0x090, 0x094, 0x098, 0, 3, 7, 0x8, 1),
	MUX_CLR_SET_UPD(CLK_TOP_A2SYSHP_SEL, "a2syshp_sel",
	    a2syshp_parents, 0x090, 0x094, 0x098, 8, 3, 15, 0x8, 2),
	MUX_CLR_SET_UPD(CLK_TOP_A3SYSHP_SEL, "a3syshp_sel",
	    a3syshp_parents, 0x090, 0x094, 0x098, 16, 5, 23, 0x8, 3),
	MUX_CLR_SET_UPD(CLK_TOP_A4SYSHP_SEL, "a4syshp_sel",
	    a4syshp_parents, 0x090, 0x094, 0x098, 24, 5, 31, 0x8, 4),
	/* CLK_CFG_9 */
	MUX_CLR_SET_UPD(CLK_TOP_ASML_SEL, "asml_sel",
	    asml_parents, 0x0a0, 0x0a4, 0x0a8, 0, 2, 7, 0x8, 5),
	MUX_CLR_SET_UPD(CLK_TOP_ASMM_SEL, "asmm_sel",
	    asml_parents, 0x0a0, 0x0a4, 0x0a8, 8, 2, 15, 0x8, 6),
	MUX_CLR_SET_UPD(CLK_TOP_ASMH_SEL, "asmh_sel",
	    asml_parents, 0x0a0, 0x0a4, 0x0a8, 16, 2, 23, 0x8, 7),
	MUX_CLR_SET_UPD(CLK_TOP_AUD_IEC_SEL, "aud_iec_sel",
	    aud_iec_parents, 0x0a0, 0x0a4, 0x0a8, 24, 3, 31, 0x8, 8),
	/* CLK_CFG_10 */
	MUX_CLR_SET_UPD(CLK_TOP_I2SO1_SEL, "i2so1_sel",
	    aud_iec_parents, 0x0b0, 0x0b4, 0x0b8, 0, 3, 7, 0x8, 9),
	MUX_CLR_SET_UPD(CLK_TOP_I2SO2_SEL, "i2so2_sel",
	    aud_iec_parents, 0x0b0, 0x0b4, 0x0b8, 8, 3, 15, 0x8, 10),
	MUX_CLR_SET_UPD(CLK_TOP_I2SI1_SEL, "i2si1_sel",
	    aud_iec_parents, 0x0b0, 0x0b4, 0x0b8, 16, 3, 23, 0x8, 11),
	MUX_CLR_SET_UPD(CLK_TOP_I2SI2_SEL, "i2si2_sel",
	    aud_iec_parents, 0x0b0, 0x0b4, 0x0b8, 24, 3, 31, 0x8, 12),
	/* CLK_CFG_11 */
	MUX_CLR_SET_UPD_FLAGS(CLK_TOP_RV33_SEL, "rv33_sel",
	    rv33_parents, 0x0c0, 0x0c4, 0x0c8, 0, 3, 7, 0x8, 13,
	    CLK_IS_CRITICAL),
	MUX_CLR_SET_UPD(CLK_TOP_STC_TOP_27MSEL, "stc_top_27msel",
	    stc_top_27msel_parents, 0x0c0, 0x0c4, 0x0c8, 8, 2, 15, 0x8, 14),
	MUX_CLR_SET_UPD(CLK_TOP_OSD_SEL, "osd_sel",
	    osd_parents, 0x0c0, 0x0c4, 0x0c8, 16, 4, 23, 0x8, 15),
	MUX_CLR_SET_UPD(CLK_TOP_VDO3_SEL, "vdo3_sel",
	    vdo3_parents, 0x0c0, 0x0c4, 0x0c8, 24, 4, 31, 0x8, 16),
	/* CLK_CFG_12 */
	MUX_CLR_SET_UPD(CLK_TOP_VDO4_SEL, "vdo4_sel",
	    vdo4_parents, 0x0d0, 0x0d4, 0x0d8, 0, 3, 7, 0x8, 17),
	MUX_CLR_SET_UPD(CLK_TOP_HD_SEL, "hd_sel",
	    hd_parents, 0x0d0, 0x0d4, 0x0d8, 8, 2, 15, 0x8, 18),
	MUX_CLR_SET_UPD(CLK_TOP_NR_SEL, "nr_sel",
	    nr_parents, 0x0d0, 0x0d4, 0x0d8, 16, 3, 23, 0x8, 19),
	MUX_CLR_SET_UPD(CLK_TOP_PE2_MAC_P0_SEL, "pe2_mac_p0_sel",
	    pe2_mac_p0_parents, 0x0d0, 0x0d4, 0x0d8, 24, 3, 31, 0x8, 20),
	/* CLK_CFG_13 */
	MUX_CLR_SET_UPD(CLK_TOP_HDCP_SEL, "hdcp_sel",
	    hdcp_parents, 0x0e0, 0x0e4, 0x0e8, 0, 2, 7, 0x8, 21),
	MUX_CLR_SET_UPD(CLK_TOP_HDCP_24MSEL, "hdcp_24msel",
	    hdcp_24msel_parents, 0x0e0, 0x0e4, 0x0e8, 8, 2, 15, 0x8, 22),
	MUX_CLR_SET_UPD(CLK_TOP_RTC_SEL, "rtc_sel",
	    rtc_parents, 0x0e0, 0x0e4, 0x0e8, 16, 2, 23, 0x8, 23),
	MUX_CLR_SET_UPD(CLK_TOP_SPINOR_SEL, "spinor_sel",
	    spinor_parents, 0x0e0, 0x0e4, 0x0e8, 24, 3, 31, 0x8, 24),
	/* CLK_CFG_14 */
	MUX_CLR_SET_UPD(CLK_TOP_ETH_250MSEL, "eth_250msel",
	    eth_250msel_parents, 0x0f0, 0x0f4, 0x0f8, 0, 2, 7, 0x8, 25),
	MUX_CLR_SET_UPD(CLK_TOP_ETH_125MSEL, "eth_125msel",
	    eth_125msel_parents, 0x0f0, 0x0f4, 0x0f8, 8, 2, 15, 0x8, 26),
	MUX_CLR_SET_UPD(CLK_TOP_ETH_50MRMSEL, "eth_50mrmsel",
	    eth_50mrmsel_parents, 0x0f0, 0x0f4, 0x0f8, 16, 2, 23, 0x8, 27),
	MUX_CLR_SET_UPD(CLK_TOP_I2C_SEL, "i2c_sel",
	    i2c_parents, 0x0f0, 0x0f4, 0x0f8, 24, 3, 31, 0x8, 28),
	/* CLK_CFG_15 */
	MUX_CLR_SET_UPD(CLK_TOP_PWMINFRA_SEL, "pwminfra_sel",
	    pwminfra_parents, 0x100, 0x104, 0x108, 0, 2, 7, 0x8, 29),
	MUX_CLR_SET_UPD(CLK_TOP_GCPU_SEL, "gcpu_sel",
	    gcpu_parents, 0x100, 0x104, 0x108, 8, 3, 15, 0x8, 30),
	MUX_CLR_SET_UPD(CLK_TOP_ECC_SEL, "ecc_sel",
	    ecc_parents, 0x100, 0x104, 0x108, 16, 3, 23, 0xC, 0),
	MUX_CLR_SET_UPD(CLK_TOP_DI_SEL, "di_sel",
	    di_parents, 0x100, 0x104, 0x108, 24, 4, 31, 0xC, 1),
	/* CLK_CFG_16 */
	MUX_CLR_SET_UPD(CLK_TOP_NFI2X_SEL, "nfi2x_sel",
	    nfi2x_parents, 0x110, 0x114, 0x118, 0, 3, 7, 0xC, 2),
	MUX_CLR_SET_UPD(CLK_TOP_SPINFI_SEL, "spinfi_sel",
	    spinfi_parents, 0x110, 0x114, 0x118, 8, 3, 15, 0xC, 3),
	MUX_CLR_SET_UPD(CLK_TOP_HD20_DACR_SEL, "hd20_dacr_sel",
	    hd20_dacr_parents, 0x110, 0x114, 0x118, 16, 2, 23, 0xC, 4),
	MUX_CLR_SET_UPD(CLK_TOP_HD20_HDCP_SEL, "hd20_hdcp_sel",
	    hd20_hdcp_parents, 0x110, 0x114, 0x118, 24, 2, 31, 0xC, 5),
	/* CLK_CFG_17 */
	MUX_CLR_SET_UPD(CLK_TOP_HDMI_SEL, "hdmi_sel",
	    audio_26msel_parents, 0x120, 0x124, 0x128, 0, 1, 7, 0xC, 6),
	MUX_CLR_SET_UPD(CLK_TOP_NNA0_SEL, "nna0_sel",
	    nna0_parents, 0x120, 0x124, 0x128, 8, 4, 15, 0xC, 7),
	MUX_CLR_SET_UPD(CLK_TOP_HDMI_APB_SEL, "hdmi_apb_sel",
	    hdmi_apb_parents, 0x120, 0x124, 0x128, 16, 2, 23, 0xC, 8),
	MUX_CLR_SET_UPD_FLAGS(CLK_TOP_CUPMSEL, "cupmsel",
	    cupmsel_parents, 0x120, 0x124, 0x128, 24, 2, 31, 0xC, 9,
	    CLK_IS_CRITICAL),
};

static const struct mtk_clk_divider apmixed_adj_divs[] = {
	DIV_ADJ_FLAGS(CLK_APMIXED_VDECPLL_27M, "vdecpll_27m",
	    "vdecpll", 0x190, 9, 5, CLK_DIVIDER_ROUND_CLOSEST),
};

static const struct mtk_clk_divider top_adj_divs[] = {
	DIV_ADJ_FLAGS(CLK_TOP_I2SI1_M, "i2si1_m",
	    "i2si1_sel", 0x328, 0, 8, CLK_DIVIDER_ROUND_CLOSEST),
	DIV_ADJ_FLAGS(CLK_TOP_I2SI2_M, "i2si2_m",
	    "i2si2_sel", 0x328, 8, 8, CLK_DIVIDER_ROUND_CLOSEST),
	DIV_ADJ_FLAGS(CLK_TOP_I2SO1_M, "i2so1_m",
	    "i2so1_sel", 0x328, 16, 8, CLK_DIVIDER_ROUND_CLOSEST),
	DIV_ADJ_FLAGS(CLK_TOP_I2SO2_M, "i2so2_m",
	    "i2so2_sel", 0x328, 24, 8, CLK_DIVIDER_ROUND_CLOSEST),
	DIV_ADJ_FLAGS(CLK_TOP_AUD_IEC, "aud_iec",
	    "aud_iec_sel", 0x334, 0, 8, CLK_DIVIDER_ROUND_CLOSEST),
};

static const struct mtk_gate_regs top0_cg_regs = {
	.set_ofs = 0x170,
	.clr_ofs = 0x170,
	.sta_ofs = 0x170,
};

static const struct mtk_gate_regs top1_cg_regs = {
	.set_ofs = 0x184,
	.clr_ofs = 0x188,
	.sta_ofs = 0x180,
};

static const struct mtk_gate_regs top2_cg_regs = {
	.set_ofs = 0x320,
	.clr_ofs = 0x320,
	.sta_ofs = 0x320,
};

static const struct mtk_gate_regs top3_cg_regs = {
	.set_ofs = 0x338,
	.clr_ofs = 0x338,
	.sta_ofs = 0x338,
};

#define GATE_TOP0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &top0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

#define GATE_TOP1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &top1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_TOP2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &top2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr,	\
	}

#define GATE_TOP3(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &top3_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate top_clks[] = {
	/* TOP0 */
	GATE_TOP0(CLK_TOP_OSDPLL_D3AB, "osdpll_d3ab", "osdpll_d3", 9),
	GATE_TOP0(CLK_TOP_EPLLD10, "eplld10", "etherpll_50m", 11),
	GATE_TOP0(CLK_TOP_HMT_PX_D3, "hmt_px_d3", "hdmitx_pixel_d3", 12),
	GATE_TOP0(CLK_TOP_TVDPLL_D3AB, "tvdpll_d3ab", "tvdpll_d3", 13),
	GATE_TOP0(CLK_TOP_HMR_AXI, "hmr_axi", "axi_sel", 19),
	/* TOP1 */
	GATE_TOP1(CLK_TOP_SSUSB_TOP, "ssusb_top", "clk26m", 24),
	GATE_TOP1(CLK_TOP_SSUSB_PHY, "ssusb_phy", "clk26m", 25),
	GATE_TOP1(CLK_TOP_SSUSB_U2_PHY, "ssusb_u2_phy", "clk26m", 26),
	/* TOP2 */
	GATE_TOP2(CLK_TOP_APLL12_DIV0, "apll12_div0", "i2si1_m", 0),
	GATE_TOP2(CLK_TOP_APLL12_DIV1, "apll12_div1", "i2si2_m", 1),
	GATE_TOP2(CLK_TOP_APLL12_DIV2, "apll12_div2", "i2so1_m", 2),
	GATE_TOP2(CLK_TOP_APLL12_DIV3, "apll12_div3", "i2so2_m", 3),
	GATE_TOP2(CLK_TOP_APLL12_DIV4, "apll12_div4", "aud_iec", 4),
	/* TOP3 */
	GATE_TOP3(CLK_TOP_APLL1_D3AB, "apll1_d3ab", "apll1_d3", 0),
	GATE_TOP3(CLK_TOP_APLL2_D3AB, "apll2_d3ab", "apll2_d3", 1),
	GATE_TOP3(CLK_TOP_APLL3_D3AB, "apll3_d3ab", "apll3_d3", 2),
	GATE_TOP3(CLK_TOP_APLL4_D3AB, "apll4_d3ab", "apll4_d3", 3),
	GATE_TOP3(CLK_TOP_APLL5_D3AB, "apll5_d3ab", "apll5_d3", 4),
	GATE_TOP3(CLK_TOP_HMR_APPD3AB, "hmr_appd3ab", "hdmirx_apll_d3", 5),
};

static const struct mtk_gate_regs infra0_cg_regs = {
	.set_ofs = 0x40,
	.clr_ofs = 0x44,
	.sta_ofs = 0x48,
};

static const struct mtk_gate_regs infra1_cg_regs = {
	.set_ofs = 0x80,
	.clr_ofs = 0x84,
	.sta_ofs = 0x88,
};

#define GATE_INFRA0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_INFRA1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate infra_clks[] = {
	/* INFRA0 */
	GATE_INFRA0(CLK_INFRA_DBGCLK, "infra_dbgclk", "axi_sel", 0),
	GATE_INFRA0(CLK_INFRA_AUDIO, "infra_audio", "axi_sel", 5),
	GATE_INFRA0(CLK_INFRA_GCE, "infra_gce", "axi_sel", 6),
	GATE_INFRA0(CLK_INFRA_M4U, "infra_m4u", "memsel", 8),
	GATE_INFRA0(CLK_INFRA_CQ_DMA, "infra_cq_dma", "axi_sel", 9),
	GATE_INFRA0(CLK_INFRA_KP, "infra_kp", "axi_sel", 16),
	GATE_INFRA0(CLK_INFRA_IIC_AO, "infra_iic_ao", "i2c_sel", 17),
	GATE_INFRA0(CLK_INFRA_CEC, "infra_cec", "rtc_sel", 18),
	GATE_INFRA0(CLK_INFRA_IIC, "infra_iic", "axi_sel", 19),
	GATE_INFRA0(CLK_INFRA_PWM0_AO, "infra_pwm0_ao", "pwminfra_sel", 24),
	GATE_INFRA0(CLK_INFRA_PWM1_AO, "infra_pwm1_ao", "pwminfra_sel", 25),
	GATE_INFRA0(CLK_INFRA_UART_AO, "infra_uart_ao", "uart_sel", 26),
	GATE_INFRA0(CLK_INFRA_PWMAO, "infra_pwmao", "pwminfra_sel", 27),
	GATE_INFRA0(CLK_INFRA_IPSYS, "infra_ipsys", "axi_sel", 28),
	/* INFRA1 */
	GATE_INFRA1(CLK_INFRA_TRNG, "infra_trng", "axi_sel", 0),
};

static const struct mtk_gate_regs peri0_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0x10,
	.sta_ofs = 0x18,
};

static const struct mtk_gate_regs peri1_cg_regs = {
	.set_ofs = 0xc,
	.clr_ofs = 0x14,
	.sta_ofs = 0x1c,
};

static const struct mtk_gate_regs peri2_cg_regs = {
	.set_ofs = 0x042C,
	.clr_ofs = 0x042C,
	.sta_ofs = 0x042C,
};

#define GATE_PERI0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_PERI1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_PERI2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &peri2_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_no_setclr_inv,	\
	}

static const struct mtk_gate peri_clks[] = {
	/* PERI0 */
	GATE_PERI0(CLK_PERI_NFI, "peri_nfi", "axi_sel", 0),
	GATE_PERI0(CLK_PERI_THERM, "peri_therm", "axi_sel", 1),
	GATE_PERI0(CLK_PERI_PWM0, "peri_pwm0", "pwminfra_sel", 2),
	GATE_PERI0(CLK_PERI_PWM1, "peri_pwm1", "pwminfra_sel", 3),
	GATE_PERI0(CLK_PERI_PWM2, "peri_pwm2", "pwminfra_sel", 4),
	GATE_PERI0(CLK_PERI_PWM3, "peri_pwm3", "pwminfra_sel", 5),
	GATE_PERI0(CLK_PERI_PWM4, "peri_pwm4", "pwminfra_sel", 6),
	GATE_PERI0(CLK_PERI_PWM5, "peri_pwm5", "pwminfra_sel", 7),
	GATE_PERI0(CLK_PERI_PWM, "peri_pwm", "pwminfra_sel", 10),
	GATE_PERI0(CLK_PERI_USB0, "peri_usb0", "axi_sel", 11),
	GATE_PERI0(CLK_PERI_AP_DMA, "peri_ap_dma", "axi_sel", 13),
	GATE_PERI0(CLK_PERI_MSDC30_0, "peri_msdc30_0", "axi_sel", 14),
	GATE_PERI0(CLK_PERI_MSDC30_1, "peri_msdc30_1", "axi_sel", 15),
	GATE_PERI0(CLK_PERI_MSDC30_2, "peri_msdc30_2", "axi_sel", 16),
	GATE_PERI0(CLK_PERI_UART0, "peri_uart0", "uart_sel", 20),
	GATE_PERI0(CLK_PERI_UART1, "peri_uart1", "uart_sel", 21),
	GATE_PERI0(CLK_PERI_I2C0, "peri_i2c0", "i2c_sel", 24),
	GATE_PERI0(CLK_PERI_I2C1, "peri_i2c1", "axi_sel", 25),
	GATE_PERI0(CLK_PERI_AUXADC, "peri_auxadc", "clk26m", 29),
	GATE_PERI0(CLK_PERI_SPI0, "peri_spi0", "spi_sel", 30),
	/* PERI1 */
	GATE_PERI1(CLK_PERI_FLASH, "peri_flash", "spinor_sel", 1),
	GATE_PERI1(CLK_PERI_SPI2, "peri_spi2", "spi_sel", 5),
	GATE_PERI1(CLK_PERI_SFLASH, "peri_sflash", "axi_sel", 11),
	GATE_PERI1(CLK_PERI_GMAC, "peri_gmac", "axi_sel", 12),
	GATE_PERI1(CLK_PERI_PCIE0, "peri_pcie0", "axi_sel", 14),
	GATE_PERI1(CLK_PERI_GMAC_PCLK, "peri_gmac_pclk", "axi_sel", 16),
	GATE_PERI1(CLK_PERI_PTP_THERM, "peri_ptp_therm", "clk26m", 17),
	GATE_PERI1(CLK_PERI_MM_APB, "peri_mm_apb", "axi_sel", 18),
	/* PERI2 */
	GATE_PERI2(CLK_PERI_MSDC0_EN, "peri_msdc0_en", "msdc50_0_sel", 0),
	GATE_PERI2(CLK_PERI_MSDC1_EN, "peri_msdc1_en", "msdc30_1_sel", 1),
	GATE_PERI2(CLK_PERI_MSDC2_EN, "peri_msdc2_en", "msdc30_2_sel", 2),
	GATE_PERI2(CLK_PERI_MSDC0_H_EN, "peri_msdc0_h_en",
				"msdc50_0_hc_sel", 4),
	GATE_PERI2(CLK_PERI_MSDC2_H_EN, "peri_msdc2_h_en",
				"msdc50_2_hc_sel", 5),
};

#define MT8696_PLL_FMAX		(3800UL * MHZ)

#define PLL_B(_id, _name, _reg, _pwr_reg, _en_mask, _flags, \
		_rst_bar_mask, _pcwbits, _pcwibits, _pd_reg, _pd_shift,  \
		_tuner_reg, _tuner_en_reg, _tuner_en_bit,   \
		_pcw_reg, _pcw_shift, _pcw_chg_reg,   \
		_pcw_chg_shift, _div_table) {		\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.rst_bar_mask = BIT(_rst_bar_mask),			\
		.fmax = MT8696_PLL_FMAX,				\
		.pcwbits = _pcwbits,					\
		.pcwibits = _pcwibits,					\
		.pd_reg = _pd_reg,					\
		.pd_shift = _pd_shift,					\
		.tuner_reg = _tuner_reg,				\
		.tuner_en_reg = _tuner_en_reg,				\
		.tuner_en_bit = _tuner_en_bit,				\
		.pcw_reg = _pcw_reg,					\
		.pcw_shift = _pcw_shift,				\
		.pcw_chg_reg = _pcw_chg_reg,    \
		.pcw_chg_shift = _pcw_chg_shift,   \
		.div_table = _div_table,				\
	}

#define PLL(_id, _name, _reg, _pwr_reg, _en_mask, _flags,   \
		_rst_bar_mask, _pcwbits,  _pcwibits, _pd_reg, _pd_shift, \
		_tuner_reg, _tuner_en_reg, _tuner_en_bit,     \
		_pcw_reg, _pcw_shift, _pcw_chg_reg, _pcw_chg_shift) \
	PLL_B(_id, _name, _reg, _pwr_reg, _en_mask, _flags,    \
		_rst_bar_mask, _pcwbits,  _pcwibits, _pd_reg, _pd_shift,   \
		_tuner_reg, _tuner_en_reg, _tuner_en_bit,   \
		_pcw_reg, _pcw_shift, _pcw_chg_reg, _pcw_chg_shift, \
		NULL)

static const struct mtk_pll_div_table armpll_div_table[] = {
	{ .div = 0, .freq = MT8696_PLL_FMAX },
	{ .div = 1, .freq = 1900000000 },
	{ .div = 2, .freq = 850000000 },
	{ .div = 3, .freq = 375000000 },
	{ .div = 4, .freq = 182500000 },
	{ } /* sentinel */
};

static const struct mtk_pll_div_table mmpll_div_table[] = {
	{ .div = 0, .freq = MT8696_PLL_FMAX },
	{ .div = 1, .freq = 1600000000 },
	{ .div = 2, .freq = 800000000 },
	{ .div = 3, .freq = 475000000 },
	{ .div = 4, .freq = 150000000 },
	{ } /* sentinel */
};

static const struct mtk_pll_data plls[] = {
	PLL(CLK_APMIXED_MAINPLL, "mainpll", 0x0120, 0x012C, 0x00000001,
		HAVE_RST_BAR, 20, 32, 8, 0x0120, 4, 0, 0, 0, 0x0124, 0,
		0x0120, 31),
	PLL(CLK_APMIXED_UNIVPLL, "univpll", 0x0130, 0x013C, 0x00000001,
		HAVE_RST_BAR, 20, 32, 8, 0x0130, 4, 0, 0, 0, 0x0134, 0,
		0x0130, 31),
	PLL(CLK_APMIXED_VDECPLL, "vdecpll", 0x0190, 0x019C, 0x00000001,
		0, 0, 32, 8, 0x0190, 4, 0, 0, 0, 0x0194, 0, 0x0190, 31),
	PLL(CLK_APMIXED_ETHERPLL, "etherpll", 0x0180, 0x018C, 0x00000001,
		0, 0, 32, 8, 0x0180, 4, 0, 0, 0, 0x0184, 0, 0x0180, 31),
	PLL(CLK_APMIXED_OSDPLL, "osdpll", 0x0200, 0x020C, 0x00000001,
		0, 0, 32, 8, 0x0200, 4, 0, 0, 0, 0x0204, 0, 0x0200, 31),
	PLL(CLK_APMIXED_APLL1, "apll1", 0x0210, 0x0220, 0x00000001,
		0, 0, 32, 8, 0x0210, 4, 0x0218, 0x0014, 0, 0x0214, 0,
		0x0210, 31),
	PLL(CLK_APMIXED_APLL2, "apll2", 0x0224, 0x0234, 0x00000001,
		0, 0, 32, 8, 0x0224, 4, 0x022C, 0x0014, 1, 0x0228, 0,
		0x0224, 31),
	PLL(CLK_APMIXED_APLL3, "apll3", 0x0238, 0x0248, 0x00000001,
		0, 0, 32, 8, 0x0238, 4, 0x0240, 0x0014, 2, 0x023C, 0,
		0x0238, 31),
	PLL(CLK_APMIXED_APLL4, "apll4", 0x024C, 0x025C, 0x00000001,
		0, 0, 32, 8, 0x024C, 4, 0x0254, 0x0014, 3, 0x0250, 0,
		0x024C, 31),
	PLL(CLK_APMIXED_APLL5, "apll5", 0x0260, 0x0270, 0x00000001,
		0, 0, 32, 8, 0x0260, 4, 0x0268, 0x0014, 4, 0x0264, 0,
		0x0260, 31),
	PLL(CLK_APMIXED_HDMIRX_APLL, "hdmirx_apll", 0x0274, 0x0284, 0x00000001,
		0, 0, 32, 8, 0x0274, 4, 0x027c, 0, 0, 0x0278, 0, 0x0274, 31),
	PLL(CLK_APMIXED_MSDCPLL, "msdcpll", 0x0150, 0x015C, 0x00000001,
		0, 0, 32, 8, 0x0150, 4, 0, 0, 0, 0x0154, 0, 0x0150, 31),
	PLL(CLK_APMIXED_TVDPLL, "tvdpll", 0x0170, 0x017C, 0x00000001,
		0, 0, 32, 8, 0x0170, 4, 0, 0, 0, 0x0174, 0, 0x0170, 31),
	PLL_B(CLK_APMIXED_MMPLL, "mmpll", 0x0140, 0x014C, 0x00000001,
		0, 0, 22, 8, 0x0144, 24, 0, 0, 0, 0x0144, 0, 0x0144, 31,
		mmpll_div_table),
	PLL_B(CLK_APMIXED_ARMPLL, "armpll", 0x0110, 0x011C, 0x00000001,
		0, 0, 28, 8, 0x0114, 0, 0, 0, 0, 0x0114, 4, 0x0114, 3,
		armpll_div_table),
	PLL(CLK_APMIXED_CCIPLL, "ccipll", 0x02A0, 0x02AC, 0x00000001,
		0, 0, 32, 8, 0x02A0, 4, 0, 0, 0, 0x2A4, 0, 0x02A0, 31),
};

static const struct mtk_fixed_factor top_early_divs[] = {
	FACTOR(CLK_TOP_CLK26MD2, "clk26md2", "clk26m", 1, 2),
};

static struct clk_onecell_data *top_clk_data;

static void clk_mt8696_top_init_early(struct device_node *node)
{
	int r, i;

	if (!top_clk_data) {
		top_clk_data = mtk_alloc_clk_data(CLK_TOP_NR_CLK);

		for (i = 0; i < CLK_TOP_NR_CLK; i++)
			top_clk_data->clks[i] = ERR_PTR(-EPROBE_DEFER);
	}

	mtk_clk_register_factors(top_early_divs, ARRAY_SIZE(top_early_divs),
			top_clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, top_clk_data);
	if (r)
		pr_info("%s(): could not register clock provider: %d\n",
			__func__, r);
}

CLK_OF_DECLARE_DRIVER(mt8696_topckgen, "mediatek,mt8696-topckgen",
			clk_mt8696_top_init_early);

static int clk_mt8696_top_probe(struct platform_device *pdev)
{
	int r;
	struct device_node *node = pdev->dev.of_node;
	void __iomem *base;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		pr_err("%s(): ioremap failed\n", __func__);
		return PTR_ERR(base);
	}

	if (!top_clk_data)
		top_clk_data = mtk_alloc_clk_data(CLK_TOP_NR_CLK);

	mtk_clk_register_fixed_clks(top_fixed_clks,
	    ARRAY_SIZE(top_fixed_clks), top_clk_data);
	mtk_clk_register_factors(top_divs, ARRAY_SIZE(top_divs), top_clk_data);
	mtk_clk_register_muxes(top_muxes, ARRAY_SIZE(top_muxes),
					node, &mt8696_clk_lock, top_clk_data);
	mtk_clk_register_composites(top_misc_muxes, ARRAY_SIZE(top_misc_muxes),
					base, &mt8696_clk_lock, top_clk_data);
	mtk_clk_register_dividers(top_adj_divs, ARRAY_SIZE(top_adj_divs),
					base, &mt8696_clk_lock, top_clk_data);
	mtk_clk_register_gates(node, top_clks, ARRAY_SIZE(top_clks),
					top_clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, top_clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	return r;
}

static int clk_mt8696_mcu_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;
	void __iomem *base;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		pr_err("%s(): ioremap failed\n", __func__);
		return PTR_ERR(base);
	}

	clk_data = mtk_alloc_clk_data(CLK_MCU_NR_CLK);

	mtk_clk_register_composites(mcu_muxes, ARRAY_SIZE(mcu_muxes),
	    base, &mt8696_clk_lock, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	return r;
}

static int clk_mt8696_infra_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_INFRA_NR_CLK);

	mtk_clk_register_gates(node, infra_clks, ARRAY_SIZE(infra_clks),
							clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	return r;
}

static int clk_mt8696_peri_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_PERI_NR_CLK);

	mtk_clk_register_gates(node, peri_clks, ARRAY_SIZE(peri_clks),
							clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	return r;
}

static int clk_mt8696_apmixed_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;
	void __iomem *base;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		pr_err("%s(): ioremap failed\n", __func__);
		return PTR_ERR(base);
	}

	clk_data = mtk_alloc_clk_data(CLK_APMIXED_NR_CLK);

	mtk_clk_register_dividers(apmixed_adj_divs,
	ARRAY_SIZE(apmixed_adj_divs), base, &mt8696_clk_lock, clk_data);
	mtk_clk_register_plls(node, plls, ARRAY_SIZE(plls), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	return r;
}

static const struct of_device_id of_match_clk_mt8696[] = {
	{
		.compatible = "mediatek,mt8696-apmixedsys",
		.data = clk_mt8696_apmixed_probe,
	}, {
		.compatible = "mediatek,mt8696-topckgen",
		.data = clk_mt8696_top_probe,
	}, {
		.compatible = "mediatek,mt8696-infracfg",
		.data = clk_mt8696_infra_probe,
	}, {
		.compatible = "mediatek,mt8696-pericfg",
		.data = clk_mt8696_peri_probe,
	}, {
		.compatible = "mediatek,mt8696-mcucfg",
		.data = clk_mt8696_mcu_probe,
	}, {
		/* sentinel */
	}
};

static int clk_mt8696_probe(struct platform_device *pdev)
{
	int (*clk_probe)(struct platform_device *p);
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

static struct platform_driver clk_mt8696_drv = {
	.probe = clk_mt8696_probe,
	.driver = {
		.name = "clk-mt8696",
		.owner = THIS_MODULE,
		.of_match_table = of_match_clk_mt8696,
	},
};

static int __init clk_mt8696_init(void)
{
	return platform_driver_register(&clk_mt8696_drv);
}

arch_initcall(clk_mt8696_init);
