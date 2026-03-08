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
#include <linux/syscore_ops.h>
#include <linux/version.h>

#define WARN_ON_CHECK_PLL_FAIL		0
#define CLKDBG_CCF_API_4_4	1

#define TAG	"[clkchk] "

#define clk_warn(fmt, args...)	pr_notice(TAG fmt, ##args)

#if !CLKDBG_CCF_API_4_4

/* backward compatible */

static const char *clk_hw_get_name(const struct clk_hw *hw)
{
	return __clk_get_name(hw->clk);
}

static bool clk_hw_is_prepared(const struct clk_hw *hw)
{
	return __clk_is_prepared(hw->clk);
}

static bool clk_hw_is_enabled(const struct clk_hw *hw)
{
	return __clk_is_enabled(hw->clk);
}

#endif /* !CLKDBG_CCF_API_4_4 */

static const char * const *get_all_clk_names(void)
{
	static const char * const clks[] = {
		/* plls */
		"mainpll",
		"univpll",
		"vdecpll",
		"etherpll",
		"osdpll",
		"apll1",
		"apll2",
		"apll3",
		"apll4",
		"apll5",
		"hdmirx_apll",
		"msdcpll",
		"tvdpll",
		"mmpll",
		"armpll",
		"ccipll",
		/* topckgen */
		"syspll_d2",
		"syspll1_d2",
		"syspll1_d4",
		"syspll1_d8",
		"syspll1_d16",
		"syspll1_d32",
		"syspll_d3",
		"syspll2_d2",
		"syspll2_d4",
		"syspll2_d8",
		"syspll_d5",
		"syspll3_d2",
		"syspll3_d4",
		"syspll3_d8",
		"syspll_d7",
		"syspll4_d4",
		"syspll4_d8",
		"univpll_d2",
		"univpll1_d2",
		"univpll1_d4",
		"univpll1_d8",
		"univpll1_d16",
		"univpll_d3",
		"univpll2_d2",
		"univpll2_d4",
		"univpll2_d8",
		"univpll2_d16",
		"univpll_d5",
		"univpll3_d2",
		"univpll3_d4",
		"univpll3_d8",
		"univpll3_d16",
		"univpll_d7",
		"univpll4_d2",
		"univpll_d26",
		"univpll_d52",
		"apll1_d2",
		"apll1_d4",
		"apll1_d8",
		"apll1_d16",
		"apll1_d3",
		"apll2_d2",
		"apll2_d4",
		"apll2_d8",
		"apll2_d16",
		"apll2_d3",
		"apll3_d2",
		"apll3_d4",
		"apll3_d8",
		"apll3_d16",
		"apll3_d3",
		"apll4_d2",
		"apll4_d4",
		"apll4_d8",
		"apll4_d16",
		"apll4_d3",
		"apll5_d2",
		"apll5_d4",
		"apll5_d8",
		"apll5_d16",
		"apll5_d3",
		"hdmirx_apll_d2",
		"hdmirx_apll_d4",
		"hdmirx_apll_d8",
		"hdmirx_apll_d16",
		"hdmirx_apll_d3",
		"hdmirx_apll_d6",
		"osdpll_d2",
		"osdpll_d4",
		"osdpll_d8",
		"osdpll_d3",
		"osdpll1_d2",
		"osdpll1_d4",
		"osdpll1_d8",
		"osdpll1_d16",
		"etherpll_500m",
		"etherpll_250m",
		"etherpll_125m",
		"etherpll_50m",
		"vdecpll_d2",
		"tvdpll_d2",
		"tvdpll_d4",
		"tvdpll_d3",
		"tvdpll_d6",
		"msdcpll_d2",
		"msdcpll_d4",
		"msdcpll_d8",
		"hdmitx_pixel",
		"hdmitx_pixel_d2",
		"hdmitx_pixel_d4",
		"hdmitx_pixel_d3",
		"hdmitx_pixel_d6",
		"clkrtc_int",
		"clkrtc_ext",
		"clk26md2",
		"dmpll",
		"dmpll_d2",
		"axi_sel",
		"scp_sel",
		"memsel",
		"memref_104_sel",
		"mem52m_sel",
		"sd_sel",
		"mmsel",
		"vdec_sel",
		"vdec_slow_sel",
		"venc_sel",
		"mfg_sel",
		"rsz_sel",
		"uart_sel",
		"spi_sel",
		"usb20_sel",
		"usb30_sel",
		"msdc50_0_hc_sel",
		"msdc50_0_sel",
		"msdc30_1_sel",
		"msdc30_2_sel",
		"msdc50_2_hc_sel",
		"msdc0p_aessel",
		"intdir_sel",
		"audio_sel",
		"audio_26msel",
		"aud_intbussel",
		"apll_sel",
		"apll2_sel",
		"apll3_sel",
		"apll4_sel",
		"apll5_sel",
		"apll6_sel",
		"a1syshp_sel",
		"a2syshp_sel",
		"a3syshp_sel",
		"a4syshp_sel",
		"asml_sel",
		"asmm_sel",
		"asmh_sel",
		"aud_iec_sel",
		"i2so1_sel",
		"i2so2_sel",
		"i2si1_sel",
		"i2si2_sel",
		"rv33_sel",
		"stc_top_27msel",
		"osd_sel",
		"vdo3_sel",
		"vdo4_sel",
		"hd_sel",
		"nr_sel",
		"pe2_mac_p0_sel",
		"hdcp_sel",
		"hdcp_24msel",
		"rtc_sel",
		"spinor_sel",
		"eth_250msel",
		"eth_125msel",
		"eth_50mrmsel",
		"i2c_sel",
		"pwminfra_sel",
		"gcpu_sel",
		"ecc_sel",
		"di_sel",
		"nfi2x_sel",
		"spinfi_sel",
		"hd20_dacr_sel",
		"hd20_hdcp_sel",
		"hdmi_sel",
		"nna0_sel",
		"hdmi_apb_sel",
		"cupmsel",
		"apll1_ref_sel",
		"apll2_ref_sel",
		"apll3_ref_sel",
		"apll4_ref_sel",
		"apll5_ref_sel",
		"hdmirx_apll_sel",
		"i2si1_m",
		"i2si2_m",
		"i2so1_m",
		"i2so2_m",
		"aud_iec",
		"apll12_div0",
		"apll12_div1",
		"apll12_div2",
		"apll12_div3",
		"apll12_div4",
		"apll1_d3ab",
		"apll2_d3ab",
		"apll3_d3ab",
		"apll4_d3ab",
		"apll5_d3ab",
		"hmr_appd3ab",
		"osdpll_d3ab",
		"eplld10",
		"hmt_px_d3",
		"tvdpll_d3ab",
		"hmr_axi",
		"ssusb_top",
		"ssusb_phy",
		"ssusb_u2_phy",
		"mcu_cpu_pll_sel",
		"mcu_buspll_sel",
		/* infracfg */
		"infra_dbgclk",
		"infra_audio",
		"infra_gce",
		"infra_m4u",
		"infra_cq_dma",
		"infra_kp",
		"infra_iic_ao",
		"infra_cec",
		"infra_iic",
		"infra_pwm0_ao",
		"infra_pwm1_ao",
		"infra_uart_ao",
		"infra_pwmao",
		"infra_ipsys",
		"infra2l3c",
		"infra_trng",
		/* pericfg */
		"peri_nfi",
		"peri_therm",
		"peri_pwm0",
		"peri_pwm1",
		"peri_pwm2",
		"peri_pwm3",
		"peri_pwm4",
		"peri_pwm5",
		"peri_pwm",
		"peri_usb0",
		"peri_ap_dma",
		"peri_msdc30_0",
		"peri_msdc30_1",
		"peri_msdc30_2",
		"peri_uart0",
		"peri_uart1",
		"peri_i2c0",
		"peri_i2c1",
		"peri_auxadc",
		"peri_spi0",
		"peri_flash",
		"peri_spi2",
		"peri_sflash",
		"peri_gmac",
		"peri_pcie0",
		"peri_gmac_pclk",
		"peri_ptp_therm",
		"peri_mm_apb",
		"peri_msdc0_en",
		"peri_msdc1_en",
		"peri_msdc2_en",
		"peri_msdc0_h_en",
		"peri_msdc2_h_en",
		/* apmixed */
		"vdecpll_27m",
		/* ipsys */
		"nna0",
		"nna0_26men",
		"nna0_pwr",
		/* ether */
		"eth_apb_pclken",
		"eth_axi_mclken",
		"eth_rx_i_rmiien",
		"eth_tx_i_rmiien",
		"eth_rx_ien",
		"eth_tx_ien",
		"eth_mac_exten",
		"eth_nclk_ien",
		/* mfg */
		"mfg_bg3d",
		/* mmsys */
		"mmcg0_smi_com",
		"mmcg1_smi_l0",
		"mmcg2_dram_l",
		"mmcg3_fakeg",
		"mmcg4_smi_l4",
		"mmcg5_smi_l1",
		"mmcg6_smi_l5",
		"mmcg7_smi_l6",
		"mmcg8_smi_l7",
		"mmcg9_vdec2img",
		"mmcg10_vdout_mm",
		"mmcg11_smi_l3",
		"mmcg13_fmtter",
		"mmcg14_disp_mix",
		"mmcg15_osd_fbdc",
		"mmcg16_of",
		"mmcg17_of_fifo",
		"mmcg18_of_hdr",
		"mmcg19_of_si2mi",
		"mmcg20_hdr_adl",
		"mmcg21_hdr_fifo",
		"mmcg22_hdr_vdo",
		"mmcg23_rgb2hdmi",
		"mmcg25_dgi",
		"mmcg0_sd_ppf",
		"mmcg1_p2i",
		"mmcg2_vm_top",
		"mmcg3_hdmi_md",
		"mmcg4_video_in",
		"mmcg5_w2d",
		"mmcg7_ou",
		"mmcg8_ou_fifo",
		"mmcg9_ou_hdr",
		"mmcg12_smi_l0",
		/* imgsys */
		"img_vdo3",
		"img_dispfmt3",
		"img_r2r",
		"img_smi_l5",
		"img_smi_l6",
		"img_dramc_l8",
		"img_vdo4",
		"img_dispfmt4",
		"img_irt_dma",
		"imgrsz",
		"img_vdo_di",
		"img_dispfmt_di",
		"img_nr",
		"img_wr_channel",
		"img_mvdo_fifo",
		"img_mfilm_grain",
		"img_mhdr_vdo_fe",
		"img_mvdo_fe_adl",
		"img_svdo_fifo",
		"img_sfilmgrain",
		"img_shdr_vdo_fe",
		/* vdsoc */
		"vdsoc_l1_socen",
		"vdsoc_lsen",
		"vdsoc_ls_ativ",
		"vdsoc_vsen",
		"vdsoc_vs_ativ",
		/* vdecore */
		"vdec_l1en",
		"vdec_laten",
		"vdec_lat_ativ",
		"vdecen",
		"vdec_ativ",
		/* venc */
		"venc_smi_con",
		"venc_con",
		/* end */
		NULL
	};

	return clks;
}

static const char *ccf_state(struct clk_hw *hw)
{
	if (__clk_get_enable_count(hw->clk))
		return "enabled";

	if (clk_hw_is_prepared(hw))
		return "prepared";

	return "disabled";
}

static void print_enabled_clks(void)
{
	const char * const *cn = get_all_clk_names();

	clk_warn("enabled clks:\n");

	for (; *cn; cn++) {
		struct clk *c = __clk_lookup(*cn);
		struct clk_hw *c_hw = __clk_get_hw(c);
		struct clk_hw *p_hw;

		if (IS_ERR_OR_NULL(c) || !c_hw)
			continue;

		p_hw = clk_hw_get_parent(c_hw);

		if (!p_hw)
			continue;

		if (!clk_hw_is_prepared(c_hw) && !__clk_get_enable_count(c))
			continue;

		clk_warn("[%-17s: %8s, %3d, %3d, %10ld, %17s]\n",
			clk_hw_get_name(c_hw),
			ccf_state(c_hw),
			clk_hw_is_prepared(c_hw),
			__clk_get_enable_count(c),
			clk_hw_get_rate(c_hw),
			p_hw ? clk_hw_get_name(p_hw) : "- ");
	}
}

static void check_pll_off(void)
{
	static const char * const off_pll_names[] = {
		"univpll",
		"vdecpll",
		"etherpll",
		"osdpll",
		"apll1",
		"apll2",
		"apll3",
		"apll4",
		"apll5",
		"hdmirx_apll",
		"msdcpll",
		"tvdpll",
		"mmpll",
		NULL
	};

	static struct clk *off_plls[ARRAY_SIZE(off_pll_names)];

	struct clk **c;
	int invalid = 0;
	char buf[128] = {0};
	int n = 0;

	if (!off_plls[0]) {
		const char * const *pn;

		for (pn = off_pll_names, c = off_plls; *pn; pn++, c++)
			*c = __clk_lookup(*pn);
	}

	for (c = off_plls; *c; c++) {
		struct clk_hw *c_hw = __clk_get_hw(*c);

		if (!c_hw)
			continue;

		if (!clk_hw_is_prepared(c_hw) && !clk_hw_is_enabled(c_hw))
			continue;

		n += snprintf(buf + n, sizeof(buf) - n, "%s ",
				clk_hw_get_name(c_hw));

		invalid++;
	}

	if (invalid) {
		clk_warn("unexpected unclosed PLL: %s\n", buf);
		print_enabled_clks();

#if WARN_ON_CHECK_PLL_FAIL
		WARN_ON(1);
#endif
	}
}

static int clkchk_syscore_suspend(void)
{
	check_pll_off();

	return 0;
}

static void clkchk_syscore_resume(void)
{
}

static struct syscore_ops clkchk_syscore_ops = {
	.suspend = clkchk_syscore_suspend,
	.resume = clkchk_syscore_resume,
};

static int __init clkchk_init(void)
{
	if (!of_machine_is_compatible("mediatek,mt8696"))
		return -ENODEV;

	register_syscore_ops(&clkchk_syscore_ops);

	return 0;
}
subsys_initcall(clkchk_init);
