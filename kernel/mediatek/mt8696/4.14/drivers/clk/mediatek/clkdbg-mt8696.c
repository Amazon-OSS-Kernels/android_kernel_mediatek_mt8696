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
#include <linux/io.h>

#include "clkdbg.h"

#define DUMP_INIT_STATE		0

/*
 * clkdbg dump_regs
 */

enum {
	topckgen,
	infracfg,
	pericfg,
	apmixedsys,
	fhctl,
	mcucfg,
	ipsys,
	ether,
	mfgcfg,
	mmsys,
	imgsys,
	vdecsoc,
	vdeccore,
	vencsys,
};

#define REGBASE_V(_phys, _id_name) { .phys = _phys, .name = #_id_name }

/*
 * checkpatch.pl ERROR:COMPLEX_MACRO
 *
 * #define REGBASE(_phys, _id_name) [_id_name] = REGBASE_V(_phys, _id_name)
 */

static struct regbase rb[] = {
	[topckgen] = REGBASE_V(0x10000000, topckgen),
	[infracfg] = REGBASE_V(0x10001000, infracfg),
	[pericfg]   = REGBASE_V(0x10003000, pericfg),
	[apmixedsys]  = REGBASE_V(0x1000c000, apmixedsys),
	[fhctl]     = REGBASE_V(0x10209e00, fhctl),
	[mcucfg]  = REGBASE_V(0x0c530000, mcucfg),
	[ipsys]  = REGBASE_V(0x10218000, ipsys),
	[ether]  = REGBASE_V(0x1101a000, ether),
	[mfgcfg]  = REGBASE_V(0x13fbf000, mfgcfg),
	[mmsys]    = REGBASE_V(0x14000000, mmsys),
	[imgsys]   = REGBASE_V(0x15000000, imgsys),
	[vdecsoc]  = REGBASE_V(0x1600f000, vdecsoc),
	[vdeccore]    = REGBASE_V(0x1602f000, vdeccore),
	[vencsys]   = REGBASE_V(0x18000000, vencsys),
};

#define REGNAME(_base, _ofs, _name)	\
	{ .base = &rb[_base], .ofs = _ofs, .name = #_name }

static struct regname rn[] = {
	REGNAME(topckgen, 0x010, CLK_CFG_0),
	REGNAME(topckgen, 0x020, CLK_CFG_1),
	REGNAME(topckgen, 0x030, CLK_CFG_2),
	REGNAME(topckgen, 0x040, CLK_CFG_3),
	REGNAME(topckgen, 0x050, CLK_CFG_4),
	REGNAME(topckgen, 0x060, CLK_CFG_5),
	REGNAME(topckgen, 0x070, CLK_CFG_6),
	REGNAME(topckgen, 0x080, CLK_CFG_7),
	REGNAME(topckgen, 0x090, CLK_CFG_8),
	REGNAME(topckgen, 0x0a0, CLK_CFG_9),
	REGNAME(topckgen, 0x0b0, CLK_CFG_10),
	REGNAME(topckgen, 0x0c0, CLK_CFG_11),
	REGNAME(topckgen, 0x0d0, CLK_CFG_12),
	REGNAME(topckgen, 0x0e0, CLK_CFG_13),
	REGNAME(topckgen, 0x0f0, CLK_CFG_14),
	REGNAME(topckgen, 0x100, CLK_CFG_15),
	REGNAME(topckgen, 0x110, CLK_CFG_16),
	REGNAME(topckgen, 0x120, CLK_CFG_17),
	REGNAME(topckgen, 0x338, CLK_AUDDIV_4),
	REGNAME(topckgen, 0x328, CLK_AUDDIV_2),
	REGNAME(topckgen, 0x334, CLK_AUDDIV_3),
	REGNAME(topckgen, 0x320, CLK_AUDDIV_0),
	REGNAME(topckgen, 0x338, CLK_AUDDIV_4),
	REGNAME(topckgen, 0x170, CLK_MISC_CFG_0),
	REGNAME(topckgen, 0x180, CLK_MISC_CFG_1),
	REGNAME(mcucfg, 0xA2A0, CPU_PLLDIV_CFG0),
	REGNAME(mcucfg, 0xA2E0, BUS_PLLDIV_CFG),
	REGNAME(infracfg, 0x48, INFRA_PDN),
	REGNAME(infracfg, 0x88, TRNG_PDN),
	REGNAME(pericfg, 0x18, PERI_GLOBALCON_PDN0),
	REGNAME(pericfg, 0x1c, PERI_GLOBALCON_PDN1),
	REGNAME(pericfg, 0x042C, PERI_MSDC_CLK_EN),
	REGNAME(apmixedsys, 0x012C, MAINPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0120, MAINPLL_CON0),
	REGNAME(apmixedsys, 0x0124, MAINPLL_CON1),
	REGNAME(apmixedsys, 0x0128, MAINPLL_CON2),
	REGNAME(apmixedsys, 0x013C, UNIVPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0130, UNIVPLL_CON0),
	REGNAME(apmixedsys, 0x0134, UNIVPLL_CON1),
	REGNAME(apmixedsys, 0x0138, UNIVPLL_CON2),
	REGNAME(apmixedsys, 0x019C, VDECPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0190, VDECPLL_CON0),
	REGNAME(apmixedsys, 0x0194, VDECPLL_CON1),
	REGNAME(apmixedsys, 0x0198, VDECPLL_CON2),
	REGNAME(apmixedsys, 0x018C, ETHERPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0180, ETHERPLL_CON0),
	REGNAME(apmixedsys, 0x0184, ETHERPLL_CON1),
	REGNAME(apmixedsys, 0x0188, ETHERPLL_CON2),
	REGNAME(apmixedsys, 0x020C, OSDPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0200, OSDPLL_CON0),
	REGNAME(apmixedsys, 0x0204, OSDPLL_CON1),
	REGNAME(apmixedsys, 0x0208, OSDPLL_CON2),
	REGNAME(apmixedsys, 0x0220, APLL1_PWR_CON0),
	REGNAME(apmixedsys, 0x0210, APLL1_CON0),
	REGNAME(apmixedsys, 0x0214, APLL1_CON1),
	REGNAME(apmixedsys, 0x0218, APLL1_CON2),
	REGNAME(apmixedsys, 0x021c, APLL1_CON3),
	REGNAME(apmixedsys, 0x0234, APLL2_PWR_CON0),
	REGNAME(apmixedsys, 0x0224, APLL2_CON0),
	REGNAME(apmixedsys, 0x0228, APLL2_CON1),
	REGNAME(apmixedsys, 0x022c, APLL2_CON2),
	REGNAME(apmixedsys, 0x0230, APLL2_CON3),
	REGNAME(apmixedsys, 0x0248, APLL3_PWR_CON0),
	REGNAME(apmixedsys, 0x0238, APLL3_CON0),
	REGNAME(apmixedsys, 0x023C, APLL3_CON1),
	REGNAME(apmixedsys, 0x0240, APLL3_CON2),
	REGNAME(apmixedsys, 0x0244, APLL3_CON3),
	REGNAME(apmixedsys, 0x025C, APLL4_PWR_CON0),
	REGNAME(apmixedsys, 0x024C, APLL4_CON0),
	REGNAME(apmixedsys, 0x0250, APLL4_CON1),
	REGNAME(apmixedsys, 0x0254, APLL4_CON2),
	REGNAME(apmixedsys, 0x0258, APLL4_CON3),
	REGNAME(apmixedsys, 0x0270, APLL5_PWR_CON0),
	REGNAME(apmixedsys, 0x0260, APLL5_CON0),
	REGNAME(apmixedsys, 0x0264, APLL5_CON1),
	REGNAME(apmixedsys, 0x0268, APLL5_CON2),
	REGNAME(apmixedsys, 0x026c, APLL5_CON3),
	REGNAME(apmixedsys, 0x0284, HDMIRX_APLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0274, HDMIRX_APLL_CON0),
	REGNAME(apmixedsys, 0x0278, HDMIRX_APLL_CON1),
	REGNAME(apmixedsys, 0x027c, HDMIRX_APLL_CON2),
	REGNAME(apmixedsys, 0x0280, HDMIRX_APLL_CON3),
	REGNAME(apmixedsys, 0x015C, MSDCPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0150, MSDCPLL_CON0),
	REGNAME(apmixedsys, 0x0154, MSDCPLL_CON1),
	REGNAME(apmixedsys, 0x0158, MSDCPLL_CON2),
	REGNAME(apmixedsys, 0x017C, TVDPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0170, TVDPLL_CON0),
	REGNAME(apmixedsys, 0x0174, TVDPLL_CON1),
	REGNAME(apmixedsys, 0x0178, TVDPLL_CON2),
	REGNAME(apmixedsys, 0x014C, MMPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0140, MMPLL_CON0),
	REGNAME(apmixedsys, 0x0144, MMPLL_CON1),
	REGNAME(apmixedsys, 0x0148, MMPLL_CON2),
	REGNAME(apmixedsys, 0x011C, ARMPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x0110, ARMPLL_CON0),
	REGNAME(apmixedsys, 0x0114, ARMPLL_CON1),
	REGNAME(apmixedsys, 0x0118, ARMPLL_CON2),
	REGNAME(apmixedsys, 0x02AC, CCIPLL_PWR_CON0),
	REGNAME(apmixedsys, 0x02A0, CCIPLL_CON0),
	REGNAME(apmixedsys, 0x2A4, CCIPLL_CON1),
	REGNAME(apmixedsys, 0x2A8, CCIPLL_CON2),
	REGNAME(ipsys, 0x208, IPNNA_NNA0_CG_EN),
	REGNAME(ipsys, 0x228, IPNNA_NNA0_PWR_ENABLE),
	REGNAME(ether, 0x300, ETHER_CKEN_0),
	REGNAME(mfgcfg, 0x0, MFG_CG),
	REGNAME(mmsys, 0x100, MMSYS_CG_0),
	REGNAME(mmsys, 0x110, MMSYS_CG_1),
	REGNAME(imgsys, 0xc, SYS_CFG_0C),
	REGNAME(imgsys, 0x28, SYS_CFG_28),
	REGNAME(vdecsoc, 0x8, LARB_CKEN_CON),
	REGNAME(vdecsoc, 0x200, LAT_CKEN),
	REGNAME(vdecsoc, 0x0, VDEC_CKEN),
	REGNAME(vdeccore, 0x8, LARB_CKEN_CON),
	REGNAME(vdeccore, 0x200, LAT_CKEN),
	REGNAME(vdeccore, 0x0, VDEC_CKEN),
	REGNAME(vencsys, 0x0, VENC_CG),
	{}
};

static const struct regname *get_all_regnames(void)
{
	return rn;
}

static void __init init_regbase(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(rb); i++)
		rb[i].virt = ioremap(rb[i].phys, PAGE_SIZE);
}

/*
 * clkdbg fmeter
 */

#include <linux/delay.h>

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	\
	do { writel(val, addr); wmb(); } while (0) /* sync write */
#define clk_writel_mask(addr, mask, val)	\
	clk_writel(addr, (clk_readl(addr) & ~(mask)) | (val))

#define ABS_DIFF(a, b)	((a) > (b) ? (a) - (b) : (b) - (a))

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
	FMCLK(ABIST2,  1, "AD_MAIN_H1092M_CK"),
	FMCLK(ABIST2,  2, "AD_MAIN_H728M_CK"),
	FMCLK(ABIST2,  3, "AD_MAIN_H436P8M_CK"),
	FMCLK(ABIST2,  4, "AD_MAIN_H312M_CK"),
	FMCLK(ABIST2,  5, "AD_UNIV_1248M_CK"),
	FMCLK(ABIST2,  6, "AD_UNIV_832M_CK"),
	FMCLK(ABIST2,  7, "AD_UNIV_499P2M_CK"),
	FMCLK(ABIST2,  8, "AD_UNIV_356P6M_CK"),
	FMCLK(ABIST2,  9, "AD_UNIV_192M_CK"),
	FMCLK(ABIST2, 10, "AD_APLL1_CK"),
	FMCLK(ABIST2, 11, "AD_APLL2_CK"),
	FMCLK(ABIST2, 12, "AD_APLL3_CK"),
	FMCLK(ABIST2, 13, "AD_APLL4_CK"),
	FMCLK(ABIST2, 14, "AD_APLL5_CK"),
	FMCLK(ABIST2, 15, "AD_HDMIRX_APLL_CK"),
	FMCLK(ABIST2, 16, "f_frtc_pre_ck"),
	FMCLK(ABIST2, 17, "AD_MMPLL_650M_CK"),
	FMCLK(ABIST2, 18, "AD_OSDPLL_1296M_CK"),
	FMCLK(ABIST2, 19, "AD_ETHERPLL_500M_CK"),
	FMCLK(ABIST2, 21, "AD_VDECPLL_624M_CK"),
	FMCLK(ABIST2, 22, "AD_TVDPLL_594M_CK"),
	FMCLK(ABIST2, 23, "AD_MSDCPLL_400M_CK"),
	FMCLK(ABIST2, 24, "DDR_DMPLL_CK"),
	FMCLK(ABIST2, 25, "hf_fi2si1_m_ck"),
	FMCLK(ABIST2, 26, "hf_fi2si2_m_ck"),
	FMCLK(ABIST2, 27, "hf_fi2so1_m_ck"),
	FMCLK(ABIST2, 28, "hf_fi2so2_m_ck"),
	FMCLK(ABIST2, 29, "hf_faud_iec_ck"),
	FMCLK(ABIST2, 30, "AD_OSD_432M_CK"),
	{}
};

#define PLL_HP_CON0		(rb[fhctl].virt + 0x0)
#define CLK_MISC_CFG_0		(rb[topckgen].virt + 0x170)
#define CLK_DBG_CFG		(rb[topckgen].virt + 0x1aC)
#define CLK26CALI_0		(rb[topckgen].virt + 0x220)
#define CLK26CALI_1		(rb[topckgen].virt + 0x224)

unsigned int mt_get_abist_freq(int k1, unsigned int ID)
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
		(clk_misc_cfg_0 & 0x00FFFFFF) | (k1 << 24));

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
		return (output * (k1 + 1));
}

static unsigned int mt_get_ckgen_freq(int k1, unsigned int ID)
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

unsigned int mt_get_abist2_freq(int k1, unsigned int ID)
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
			(clk_misc_cfg_0 & 0x00FFFFFF) | (k1 << 24));

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
		return (output * (k1 + 1));
}

u32 fmeter_freq(enum FMETER_TYPE type, int clk)
{
	if (type == ABIST)
		return mt_get_abist_freq(3, clk);
	else if (type == CKGEN)
		return mt_get_ckgen_freq(0, clk);
	else if (type == ABIST2)
		return mt_get_abist2_freq(3, clk);

	return 0;
}

static u32 measure_stable_fmeter_freq(enum FMETER_TYPE type, int clk)
{
	u32 last_freq = 0;
	u32 freq = fmeter_freq(type, clk);
	u32 maxfreq = max(freq, last_freq);

	while (maxfreq > 0 && ABS_DIFF(freq, last_freq) * 100 / maxfreq > 10) {
		last_freq = freq;
		freq = fmeter_freq(type, clk);
		maxfreq = max(freq, last_freq);
	}

	return freq;
}

static const struct fmeter_clk *get_all_fmeter_clks(void)
{
	return fclks;
}

struct bak {
	u32 pll_hp_con0;
};

static void *prepare_fmeter(void)
{
	static struct bak regs;

	regs.pll_hp_con0 = clk_readl(PLL_HP_CON0);

	clk_writel(PLL_HP_CON0, 0x0);		/* disable PLL hopping */
	udelay(10);

	return &regs;
}

static void unprepare_fmeter(void *data)
{
	struct bak *regs = data;

	/* restore old setting */
	clk_writel(PLL_HP_CON0, regs->pll_hp_con0);
}

static u32 fmeter_freq_op(const struct fmeter_clk *fclk)
{
	if (fclk->type)
		return measure_stable_fmeter_freq(fclk->type, fclk->id);

	return 0;
}

/*
 * clkdbg dump_state
 */

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

/*
 * clkdbg pwr_status
 */

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
	};

	return pwr_names;
}

u32 get_spm_pwr_status(void)
{
	static void __iomem *scpsys_base, *pwr_sta, *pwr_sta_2nd;

	if (scpsys_base == NULL || pwr_sta == NULL || pwr_sta_2nd == NULL) {
		scpsys_base = ioremap(0x10006000, PAGE_SIZE);
		pwr_sta = scpsys_base + 0x16C;
		pwr_sta_2nd = scpsys_base + 0x170;
	}

	return clk_readl(pwr_sta) & clk_readl(pwr_sta_2nd);
}

/*
 * clkdbg dump_clks
 */

static void setup_provider_clk(struct provider_clk *pvdck)
{
	static const struct {
		const char *pvdname;
		u32 pwr_mask;
	} pvd_pwr_mask[] = {
		{"mmsys", BIT(2) | BIT(3) | BIT(4)},
		{"imgsys", BIT(7) | BIT(8) | BIT(9)},
		{"ether", BIT(16)},
		{"mfgcfg",  BIT(20) | BIT(21)},
		{"vdecsoc", BIT(24)},
		{"vdeccore", BIT(0) | BIT(24)},
		{"vencsys", BIT(1)},
	};

	size_t i;
	const char *pvdname = pvdck->provider_name;

	if (pvdname == NULL)
		return;

	for (i = 0; i < ARRAY_SIZE(pvd_pwr_mask); i++) {
		if (strcmp(pvdname, pvd_pwr_mask[i].pvdname) == 0) {
			pvdck->pwr_mask = pvd_pwr_mask[i].pwr_mask;
			return;
		}
	}
}

/*
 * init functions
 */

static struct clkdbg_ops clkdbg_mt8696_ops = {
	.get_all_fmeter_clks = get_all_fmeter_clks,
	.prepare_fmeter = prepare_fmeter,
	.unprepare_fmeter = unprepare_fmeter,
	.fmeter_freq = fmeter_freq_op,
	.get_all_regnames = get_all_regnames,
	.get_all_clk_names = get_all_clk_names,
	.get_pwr_names = get_pwr_names,
	.setup_provider_clk = setup_provider_clk,
	.get_spm_pwr_status = get_spm_pwr_status,
};

static void __init init_custom_cmds(void)
{
	static const struct cmd_fn cmds[] = {
		{}
	};

	set_custom_cmds(cmds);
}

static int __init clkdbg_mt8696_init(void)
{
	if ((of_machine_is_compatible("mediatek,mt8696") == 0 &&
		of_machine_is_compatible("mediatek,mt8532") == 0))
		return -ENODEV;

	init_regbase();

	init_custom_cmds();
	set_clkdbg_ops(&clkdbg_mt8696_ops);

#if DUMP_INIT_STATE
	print_regs();
	print_fmeter_all();
#endif /* DUMP_INIT_STATE */

	return 0;
}
device_initcall(clkdbg_mt8696_init);
