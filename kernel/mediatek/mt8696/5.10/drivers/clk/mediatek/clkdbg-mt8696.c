// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Qiqi Wang <qiqi.wang@mediatek.com>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <dt-bindings/power/mt8696-power.h>

#include "clkdbg.h"
#include "clkchk.h"
#include "clk-fmeter.h"

static void __iomem *scpsys_base, *topck_base;

/*
 * clkdbg dump_state
 */

#define FMCLK(_t, _i, _n) { .type = _t, .id = _i, .name = _n }

static const struct fmeter_clk fclks[] = {
	FMCLK(CKGEN,  1, "hf_faxi_ck"),
	FMCLK(CKGEN,  2, "hf_fscp_ck"),
	FMCLK(CKGEN,  3, "hf_fmem_ck"),
	FMCLK(CKGEN,  4, "hf_fmem_ref_104m_ck"),
	FMCLK(CKGEN,  5, "hf_fmem_ref_52m_ck"),
	FMCLK(CKGEN,  6, "hf_fsd_ck"),
	FMCLK(CKGEN,  7, "hf_fmm_ck"),
	FMCLK(CKGEN,  8, "hf_fvdec_ck"),
	FMCLK(CKGEN,  9, "hf_fvdec_slow_ck"),
	FMCLK(CKGEN, 10, "hf_fvenc_ck"),
	FMCLK(CKGEN, 11, "hf_fmfg_ck"),
	FMCLK(CKGEN, 12, "hf_frsz_ck"),
	FMCLK(CKGEN, 13, "f_fuart_ck"),
	FMCLK(CKGEN, 14, "hf_fspi_ck"),
	FMCLK(CKGEN, 15, "f_fusb20_ck"),
	FMCLK(CKGEN, 16, "f_fusb30_ck"),
	FMCLK(CKGEN, 17, "hf_fmsdc50_0_hclk_ck"),
	FMCLK(CKGEN, 18, "hf_fmsdc50_0_ck"),
	FMCLK(CKGEN, 19, "hf_fmsdc30_1_ck"),
	FMCLK(CKGEN, 20, "hf_fmsdc30_2_ck"),
	FMCLK(CKGEN, 21, "hf_fmsdc50_2_hclk_ck"),
	FMCLK(CKGEN, 22, "hf_fmsdc0p_aes_ck"),
	FMCLK(CKGEN, 23, "hf_fintdir_ck"),
	FMCLK(CKGEN, 24, "hf_faudio_ck"),
	FMCLK(CKGEN, 25, "hf_faudio_26m_ck"),
	FMCLK(CKGEN, 26, "hf_faud_intbus_ck"),
	FMCLK(CKGEN, 27, "hf_fapll_ck"),
	FMCLK(CKGEN, 28, "hf_fapll2_ck"),
	FMCLK(CKGEN, 29, "hf_fapll3_ck"),
	FMCLK(CKGEN, 30, "hf_fapll4_ck"),
	FMCLK(CKGEN, 31, "hf_fapll5_ck"),
	FMCLK(CKGEN, 32, "hf_fapll6_ck"),
	FMCLK(CKGEN, 33, "hf_fa1sys_hp_ck"),
	FMCLK(CKGEN, 34, "hf_fa2sys_hp_ck"),
	FMCLK(CKGEN, 35, "hf_fa3sys_hp_ck"),
	FMCLK(CKGEN, 36, "hf_fa4sys_hp_ck"),
	FMCLK(CKGEN, 37, "hf_fasm_l_ck"),
	FMCLK(CKGEN, 38, "hf_fasm_m_ck"),
	FMCLK(CKGEN, 39, "hf_fasm_h_ck"),
	FMCLK(CKGEN, 40, "hf_faud_iec_clk"),
	FMCLK(CKGEN, 41, "hf_fi2so1_mck"),
	FMCLK(CKGEN, 42, "hf_fi2so2_mck"),
	FMCLK(CKGEN, 43, "hf_fi2si1_mck"),
	FMCLK(CKGEN, 44, "hf_fi2si2_mck"),
	FMCLK(CKGEN, 45, "hf_frv33_ck"),
	FMCLK(CKGEN, 46, "hf_fstc_top_27m_clk"),
	FMCLK(CKGEN, 47, "hf_fosd_ck"),
	FMCLK(CKGEN, 48, "hf_fvdo3_ck"),
	FMCLK(CKGEN, 49, "hf_fvdo4_ck"),
	FMCLK(CKGEN, 50, "hf_fhd_ck"),
	FMCLK(CKGEN, 51, "hf_fnr_ck"),
	FMCLK(CKGEN, 52, "hf_fpe2_mac_p0_ck"),
	FMCLK(CKGEN, 53, "hf_fhdcp_ck"),
	FMCLK(CKGEN, 54, "hf_fhdcp_24m_ck"),
	FMCLK(CKGEN, 55, "f_frtc_ck"),
	FMCLK(CKGEN, 56, "hf_fspinor_ck"),
	FMCLK(CKGEN, 57, "hf_fether_250m_ck"),
	FMCLK(CKGEN, 58, "hf_fether_125m_ck"),
	FMCLK(CKGEN, 59, "hf_fether_50m_rmii_ck"),
	FMCLK(CKGEN, 60, "hf_fi2c_ck"),
	FMCLK(CKGEN, 61, "hf_fpwm_infra_ck"),
	FMCLK(CKGEN, 62, "hf_fgcpu_ck"),
	FMCLK(CKGEN, 63, "hf_fecc_ck"),
	FMCLK(CKGEN, 64, "hf_fdi_ck"),
	FMCLK(CKGEN, 65, "hf_fnfi2x_ck"),
	FMCLK(CKGEN, 66, "hf_fspinfi_ck"),
	FMCLK(CKGEN, 67, "hf_fhd20_dacr_ref_clk"),
	FMCLK(CKGEN, 68, "hf_fhd20_hdcp_cclk"),
	FMCLK(CKGEN, 69, "hf_fhdmi_xtal"),
	FMCLK(CKGEN, 70, "hf_fnna0_clk"),
	FMCLK(CKGEN, 71, "hf_fhdmi_apb_ck"),
	FMCLK(CKGEN, 72, "hg_mcupm_ck"),
	FMCLK(ABIST,  1, "AD_ARMPLL_CK_D2"),
	FMCLK(ABIST,  2, "AD_ARMPLL_CK_D3"),
	FMCLK(ABIST,  5, "AD_HDMITXPLL_PIXEL_CK"),
	FMCLK(ABIST, 12, "DA_HDMITX20_REF_CK"),
	FMCLK(ABIST, 32, "AD_ARMPLL_CORE_CK1400M"),
	FMCLK(ABIST, 34, "AD_CCIPLL_CORE_CK1400M"),
	FMCLK(ABIST, 36, "AD_MAINPLL_2184M_CK"),
	FMCLK(ABIST, 37, "AD_UNIVPLL_2496M_CK"),
	FMCLK(ABIST, 38, "AD_APHYA_26M_CK"),
	FMCLK(ABIST, 39, "AD_SYS_FS26M_CK"),
	FMCLK(ABIST_2,  1, "AD_MAIN_H1092M_CK"),
	FMCLK(ABIST_2,  2, "AD_MAIN_H728M_CK"),
	FMCLK(ABIST_2,  3, "AD_MAIN_H436P8M_CK"),
	FMCLK(ABIST_2,  4, "AD_MAIN_H312M_CK"),
	FMCLK(ABIST_2,  5, "AD_UNIV_1248M_CK"),
	FMCLK(ABIST_2,  6, "AD_UNIV_832M_CK"),
	FMCLK(ABIST_2,  7, "AD_UNIV_499P2M_CK"),
	FMCLK(ABIST_2,  8, "AD_UNIV_356P6M_CK"),
	FMCLK(ABIST_2,  9, "AD_UNIV_192M_CK"),
	FMCLK(ABIST_2, 10, "AD_APLL1_CK"),
	FMCLK(ABIST_2, 11, "AD_APLL2_CK"),
	FMCLK(ABIST_2, 12, "AD_APLL3_CK"),
	FMCLK(ABIST_2, 13, "AD_APLL4_CK"),
	FMCLK(ABIST_2, 14, "AD_APLL5_CK"),
	FMCLK(ABIST_2, 15, "AD_HDMIRX_APLL_CK"),
	FMCLK(ABIST_2, 16, "f_frtc_pre_ck"),
	FMCLK(ABIST_2, 17, "AD_MMPLL_650M_CK"),
	FMCLK(ABIST_2, 18, "AD_OSDPLL_1296M_CK"),
	FMCLK(ABIST_2, 19, "AD_ETHERPLL_500M_CK"),
	FMCLK(ABIST_2, 21, "AD_VDECPLL_624M_CK"),
	FMCLK(ABIST_2, 22, "AD_TVDPLL_594M_CK"),
	FMCLK(ABIST_2, 23, "AD_MSDCPLL_400M_CK"),
	FMCLK(ABIST_2, 24, "DDR_DMPLL_CK"),
	FMCLK(ABIST_2, 25, "hf_fi2si1_m_ck"),
	FMCLK(ABIST_2, 26, "hf_fi2si2_m_ck"),
	FMCLK(ABIST_2, 27, "hf_fi2so1_m_ck"),
	FMCLK(ABIST_2, 28, "hf_fi2so2_m_ck"),
	FMCLK(ABIST_2, 29, "hf_faud_iec_ck"),
	FMCLK(ABIST_2, 30, "AD_OSD_432M_CK"),
	{}
};

static const struct fmeter_clk *get_all_fmeter_clks(void)
{
	return fclks;
}

#define CLK_MISC_CFG_0	(topck_base + 0x170)
#define CLK_DBG_CFG	(topck_base + 0x1aC)
#define CLK26CALI_0	(topck_base + 0x220)
#define CLK26CALI_1	(topck_base + 0x224)

unsigned int get_abist_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg,
				clk_misc_cfg_0, clk26cali_1;

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	//sel abist_cksw and enable freq meter sel abist
	clk_writel(CLK_DBG_CFG,
		(clk_dbg_cfg & 0xFFC0FFFC)|(ID << 16));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	// select divider
	clk_writel(CLK_MISC_CFG_0,
		(clk_misc_cfg_0 & 0x00FFFFFF) | (3 << 24));

	clk26cali_0 = clk_readl(CLK26CALI_0);
	clk26cali_1 = clk_readl(CLK26CALI_1);
	// bit[12] = 1, enable fmeter
	clk_writel(CLK26CALI_0,
		(clk_readl(CLK26CALI_0) & ~0x1000) | 0x1000);
	// bit[4] = 1, start fmeter
	clk_writel(CLK26CALI_0,
		(clk_readl(CLK26CALI_0) & ~0x10) | 0x10);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		udelay(30);
		i++;
		if (i > 10)
			break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;
	output = ((temp * 26000)) / 1024; // Khz

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return (output * 4);
}

static unsigned int get_ckgen_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg,
				clk_misc_cfg_0, clk26cali_1;

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	//sel ckgen_cksw[22] and enable freq meter sel ckgen
	clk_writel(CLK_DBG_CFG,
			(clk_dbg_cfg & 0xFFFF80FC)|(ID << 8)|(0x1));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));

	clk26cali_0 = clk_readl(CLK26CALI_0);
	clk26cali_1 = clk_readl(CLK26CALI_1);
	// bit[12] = 1, enable fmeter
	clk_writel(CLK26CALI_0,
		(clk_readl(CLK26CALI_0) & ~0x1000) | 0x1000);
	// bit[4] = 1, start fmeter
	clk_writel(CLK26CALI_0,
		(clk_readl(CLK26CALI_0) & ~0x10) | 0x10);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;
	output = ((temp * 26000)) / 1024; // Khz

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return output;
}

unsigned int get_abist2_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg,
				clk_misc_cfg_0, clk26cali_1;

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	//sel abist_cksw and enable freq meter sel abist2
	clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xC0FFFFFC)|(ID << 24)|(0x2));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	// select divider
	clk_writel(CLK_MISC_CFG_0,
			(clk_misc_cfg_0 & 0x00FFFFFF) | (3 << 24));

	clk26cali_0 = clk_readl(CLK26CALI_0);
	clk26cali_1 = clk_readl(CLK26CALI_1);
	// bit[12] = 1, enable fmeter
	clk_writel(CLK26CALI_0,
		(clk_readl(CLK26CALI_0) & ~0x1000) | 0x1000);
	// bit[4] = 1, start fmeter
	clk_writel(CLK26CALI_0,
		(clk_readl(CLK26CALI_0) & ~0x10) | 0x10);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;
	output = ((temp * 26000)) / 1024; // Khz

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return (output * 4);
}

static u32 fmeter_freq_op(const struct fmeter_clk *fclk)
{
	if (fclk->type == ABIST)
		return get_abist_freq(fclk->id);
	else if (fclk->type == CKGEN)
		return get_ckgen_freq(fclk->id);
	else if (fclk->type == ABIST_2)
		return get_abist2_freq(fclk->id);

	return 0;
}

static const char * const clk_names[] = {
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
	NULL,
};

static const char * const *get_all_clk_names(void)
{
	return clk_names;
}

static const char * const *get_pwr_names(void)
{
	static const char * const pwr_names[] = {
		[0]  = "VDEC_CON0",
		[1]  = "VENC",
		[2]  = "MMSYS_YOP",
		[3]  = "MMSYS_COMMON",
		[4]  = "OSD_UHD",
		[5]  = "HDMITX",
		[6]  = "HDMIRX",
		[7]  = "DISP_MAIN",
		[8]  = "DISP_SUB",
		[9]  = "DISP_COMMON",
		[10] = "MMCU",
		[11] = "AUDIO",
		[12] = "AUDIO_ASRC",
		[13] = "",
		[14] = "",
		[15] = "SSUSB",
		[16] = "ETHER",
		[17] = "NNA0",
		[18] = "",
		[19] = "",
		[20] = "MFG",
		[21] = "MFG2",
		[22] = "MFG3",
		[23] = "HDMIRX_PHY",
		[24] = "VDEC_SOC",
		[25] = "",
		[26] = "",
		[27] = "",
		[28] = "",
		[29] = "",
		[30] = "",
		[31] = "",
		[32] = NULL,
	};

	return pwr_names;
}

static u32 _get_pwr_status(u32 pwr_sta_ofs, u32 pwr_sta_2nd_ofs)
{
	static void __iomem *pwr_sta, *pwr_sta_2nd;

	pwr_sta = scpsys_base + pwr_sta_ofs;
	pwr_sta_2nd = scpsys_base + pwr_sta_2nd_ofs;

	return readl(pwr_sta) & readl(pwr_sta_2nd);
}

static u32 *get_all_pwr_status(void)
{
	static struct regs {
		u32 pwr_sta_ofs;
		u32 pwr_sta_2nd_ofs;
	} g[] = {
		{0x16c, 0x170},
		{0x174, 0x178},
	};

	static u32 pwr_sta[PWR_STA_GROUP_NR];
	int i;

	for (i = 0; i < PWR_STA_GROUP_NR; i++)
		pwr_sta[i] = _get_pwr_status(g[i].pwr_sta_ofs, g[i].pwr_sta_2nd_ofs);

	return pwr_sta;
}

static struct device_node *get_power_controller(void)
{
	return of_find_compatible_node(NULL, NULL, "mediatek,mt8696-scpsys");
}

/*
 * init functions
 */

static struct clkdbg_ops clkdbg_mt8696_ops = {
	.get_all_fmeter_clks = get_all_fmeter_clks,
	.fmeter_freq = fmeter_freq_op,
	.get_all_clk_names = get_all_clk_names,
	.get_pwr_names = get_pwr_names,
	.get_all_pwr_status = get_all_pwr_status,
	.get_power_controller = get_power_controller,
};

static int clk_dbg_mt8696_probe(struct platform_device *pdev)
{
	pr_notice("%s start\n", __func__);
	set_clkdbg_ops(&clkdbg_mt8696_ops);

	return 0;
}

static struct platform_driver clk_dbg_mt8696_drv = {
	.probe = clk_dbg_mt8696_probe,
	.driver = {
		.name = "clk-dbg-mt8696",
		.owner = THIS_MODULE,
	},
};

/*
 * init functions
 */

static int __init clkdbg_mt8696_init(void)
{
	scpsys_base = ioremap(0x10006000, PAGE_SIZE);
	topck_base = ioremap(0x10000000, PAGE_SIZE);

	return clk_dbg_driver_register(&clk_dbg_mt8696_drv, "clk-dbg-mt8696");
}

static void __exit clkdbg_mt8696_exit(void)
{
	platform_driver_unregister(&clk_dbg_mt8696_drv);
}

subsys_initcall(clkdbg_mt8696_init);
module_exit(clkdbg_mt8696_exit);
MODULE_LICENSE("GPL");
