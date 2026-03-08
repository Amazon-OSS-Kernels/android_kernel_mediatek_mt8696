// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Chun-Jie Chen <chun-jie.chen@mediatek.com>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>

#include <dt-bindings/power/mt8188-power.h>

#include "clkchk.h"
#include "clkchk-mt8188.h"

/*
 * clkchk dump_regs
 */

#define REGBASE_V(_phys, _id_name, _pg, _pn) { .phys = _phys,	\
		.name = #_id_name, .pg = _pg, .pn = _pn}

static struct regbase rb[] = {
	[spm] = REGBASE_V(0x10006000, spm, PD_NULL, CLK_NULL),
	[topckgen] = REGBASE_V(0x10000000, topckgen, PD_NULL, CLK_NULL),
	[infracfg_ao] = REGBASE_V(0x10001000, infracfg_ao, PD_NULL, CLK_NULL),
	[apmixedsys] = REGBASE_V(0x1000C000, apmixedsys, PD_NULL, CLK_NULL),
	[audsys_src] = REGBASE_V(0x10b01000, audsys_src, MT8188_POWER_DOMAIN_AUDIO_ASRC, CLK_NULL),
	[audsys] = REGBASE_V(0x10b10000, audsys, MT8188_POWER_DOMAIN_AUDIO, CLK_NULL),
	[pericfg_ao] = REGBASE_V(0x11003000, pericfg_ao, PD_NULL, CLK_NULL),
	[imp_iic_wrap_c] = REGBASE_V(0x11283000, imp_iic_wrap_c, PD_NULL, "i2c_sel"),
	[imp_iic_wrap_w] = REGBASE_V(0x11e02000, imp_iic_wrap_w, PD_NULL, "i2c_sel"),
	[imp_iic_wrap_en] = REGBASE_V(0x11ec2000, imp_iic_wrap_en, PD_NULL, "i2c_sel"),
	[mfgcfg] = REGBASE_V(0x13fbf000, mfgcfg, MT8188_POWER_DOMAIN_MFG1, CLK_NULL),
	[vppsys0] = REGBASE_V(0x14000000, vppsys0, MT8188_POWER_DOMAIN_VPPSYS0, CLK_NULL),
	[wpesys] = REGBASE_V(0x14e00000, wpesys, MT8188_POWER_DOMAIN_WPE, CLK_NULL),
	[wpesys_vpp0] = REGBASE_V(0x14e02000, wpesys_vpp0, MT8188_POWER_DOMAIN_WPE, CLK_NULL),
	[vppsys1] = REGBASE_V(0x14f00000, vppsys1, MT8188_POWER_DOMAIN_VPPSYS1, CLK_NULL),
	[imgsys] = REGBASE_V(0x15000000, imgsys, MT8188_POWER_DOMAIN_IMG_MAIN, CLK_NULL),
	[imgsys1_dip_top] = REGBASE_V(0x15110000, imgsys1_dip_top,
				      MT8188_POWER_DOMAIN_DIP, CLK_NULL),
	[imgsys1_dip_nr] = REGBASE_V(0x15130000, imgsys1_dip_nr, MT8188_POWER_DOMAIN_DIP, CLK_NULL),
	[imgsys_wpe1] = REGBASE_V(0x15220000, imgsys_wpe1, MT8188_POWER_DOMAIN_DIP, CLK_NULL),
	[ipesys] = REGBASE_V(0x15330000, ipesys, MT8188_POWER_DOMAIN_IPE, CLK_NULL),
	[imgsys_wpe2] = REGBASE_V(0x15520000, imgsys_wpe2, MT8188_POWER_DOMAIN_DIP, CLK_NULL),
	[imgsys_wpe3] = REGBASE_V(0x15620000, imgsys_wpe3, MT8188_POWER_DOMAIN_DIP, CLK_NULL),
	[camsys] = REGBASE_V(0x16000000, camsys, MT8188_POWER_DOMAIN_CAM_MAIN, CLK_NULL),
	[camsys_rawa] = REGBASE_V(0x1604f000, camsys_rawa, MT8188_POWER_DOMAIN_CAM_SUBA, CLK_NULL),
	[camsys_yuva] = REGBASE_V(0x1606f000, camsys_yuva, MT8188_POWER_DOMAIN_CAM_SUBA, CLK_NULL),
	[camsys_rawb] = REGBASE_V(0x1608f000, camsys_rawb, MT8188_POWER_DOMAIN_CAM_SUBB, CLK_NULL),
	[camsys_yuvb] = REGBASE_V(0x160af000, camsys_yuvb, MT8188_POWER_DOMAIN_CAM_SUBB, CLK_NULL),
	[ccusys] = REGBASE_V(0x17200000, ccusys, MT8188_POWER_DOMAIN_CAM_MAIN, CLK_NULL),
	[vdecsys_soc] = REGBASE_V(0x1800f000, vdecsys_soc, MT8188_POWER_DOMAIN_VDEC0, CLK_NULL),
	[vdecsys] = REGBASE_V(0x1802f000, vdecsys, MT8188_POWER_DOMAIN_VDEC1, CLK_NULL),
	[vencsys] = REGBASE_V(0x1a000000, vencsys, MT8188_POWER_DOMAIN_VENC, CLK_NULL),
	[vdosys0] = REGBASE_V(0x1c01d000, vdosys0, MT8188_POWER_DOMAIN_VDOSYS0, CLK_NULL),
	[vdosys1] = REGBASE_V(0x1c100000, vdosys1, MT8188_POWER_DOMAIN_VDOSYS1, CLK_NULL),
	{},
};

#define REGNAME(_base, _ofs, _name)	\
	{ .base = &rb[_base], .ofs = _ofs, .name = #_name }

static struct regname rn[] = {
	REGNAME(spm, 0x16c, PWR_STATUS),
	REGNAME(spm, 0x170, PWR_STATUS_2ND),
	REGNAME(spm, 0x174, CPU_PWR_STATUS),
	REGNAME(spm, 0x178, CPU_PWR_STATUS_2ND),
	REGNAME(spm, 0x300, MFG0_PWR_CON),
	REGNAME(spm, 0x304, MFG1_PWR_CON),
	REGNAME(spm, 0x308, MFG2_PWR_CON),
	REGNAME(spm, 0x30c, MFG3_PWR_CON),
	REGNAME(spm, 0x310, MFG4_PWR_CON),
	REGNAME(spm, 0x324, PEXTP_MAC_TOP_P0_PWR_CON),
	REGNAME(spm, 0x328, PEXTP_PHY_TOP_PWR_CON),
	REGNAME(spm, 0x338, ETHER_PWR_CON),
	REGNAME(spm, 0x34c, AUDIO_PWR_CON),
	REGNAME(spm, 0x350, AUDIO_ASRC_PWR_CON),
	REGNAME(spm, 0x354, ADSP_PWR_CON),
	REGNAME(spm, 0x358, ADSP_INFRA_PWR_CON),
	REGNAME(spm, 0x35c, ADSP_AO_PWR_CON),
	REGNAME(spm, 0x360, VPPSYS0_PWR_CON),
	REGNAME(spm, 0x364, VPPSYS1_PWR_CON),
	REGNAME(spm, 0x368, VDOSYS0_PWR_CON),
	REGNAME(spm, 0x36c, VDOSYS1_PWR_CON),
	REGNAME(spm, 0x370, WPESYS_PWR_CON),
	REGNAME(spm, 0x374, DP_TX_PWR_CON),
	REGNAME(spm, 0x378, EDP_TX_PWR_CON),
	REGNAME(spm, 0x37c, HDMI_TX_PWR_CON),
	REGNAME(spm, 0x380, VDE0_PWR_CON),
	REGNAME(spm, 0x384, VDE1_PWR_CON),
	REGNAME(spm, 0x38c, VEN_PWR_CON),
	REGNAME(spm, 0x394, CAM_MAIN_PWR_CON),
	REGNAME(spm, 0x398, CAM_SUBA_PWR_CON),
	REGNAME(spm, 0x39c, CAM_SUBB_PWR_CON),
	REGNAME(spm, 0x3a0, CAM_VCORE_PWR_CON),
	REGNAME(spm, 0x3a4, IMG_VCORE_PWR_CON),
	REGNAME(spm, 0x3a8, IMG_MAIN_PWR_CON),
	REGNAME(spm, 0x3ac, IMG_DIP_PWR_CON),
	REGNAME(spm, 0x3b0, IMG_IPE_PWR_CON),
	REGNAME(spm, 0x3c4, CSI_RX_TOP_PWR_CON),
	REGNAME(apmixedsys, 0x0, AP_PLL_CON0),
	REGNAME(apmixedsys, 0x4, AP_PLL_CON1),
	REGNAME(apmixedsys, 0x8, AP_PLL_CON2),
	REGNAME(apmixedsys, 0xc, AP_PLL_CON3),
	REGNAME(apmixedsys, 0x34, APLL1_TUNER_CON0),
	REGNAME(apmixedsys, 0x38, APLL2_TUNER_CON0),
	REGNAME(apmixedsys, 0x3c, APLL3_TUNER_CON0),
	REGNAME(apmixedsys, 0x40, APLL4_TUNER_CON0),
	REGNAME(apmixedsys, 0x44, APLL5_TUNER_CON0),
	REGNAME(apmixedsys, 0x204, ARMPLL_LL_CON0),
	REGNAME(apmixedsys, 0x208, ARMPLL_LL_CON1),
	REGNAME(apmixedsys, 0x20c, ARMPLL_LL_CON2),
	REGNAME(apmixedsys, 0x210, ARMPLL_LL_CON3),
	REGNAME(apmixedsys, 0x214, ARMPLL_BL_CON0),
	REGNAME(apmixedsys, 0x218, ARMPLL_BL_CON1),
	REGNAME(apmixedsys, 0x21c, ARMPLL_BL_CON2),
	REGNAME(apmixedsys, 0x220, ARMPLL_BL_CON3),
	REGNAME(apmixedsys, 0x224, CCIPLL_CON0),
	REGNAME(apmixedsys, 0x228, CCIPLL_CON1),
	REGNAME(apmixedsys, 0x22c, CCIPLL_CON2),
	REGNAME(apmixedsys, 0x230, CCIPLL_CON3),
	REGNAME(apmixedsys, 0x304, APLL1_CON0),
	REGNAME(apmixedsys, 0x308, APLL1_CON1),
	REGNAME(apmixedsys, 0x30c, APLL1_CON2),
	REGNAME(apmixedsys, 0x310, APLL1_CON3),
	REGNAME(apmixedsys, 0x314, APLL1_CON4),
	REGNAME(apmixedsys, 0x318, APLL2_CON0),
	REGNAME(apmixedsys, 0x31c, APLL2_CON1),
	REGNAME(apmixedsys, 0x320, APLL2_CON2),
	REGNAME(apmixedsys, 0x324, APLL2_CON3),
	REGNAME(apmixedsys, 0x328, APLL2_CON4),
	REGNAME(apmixedsys, 0x32c, APLL3_CON0),
	REGNAME(apmixedsys, 0x330, APLL3_CON1),
	REGNAME(apmixedsys, 0x334, APLL3_CON2),
	REGNAME(apmixedsys, 0x338, APLL3_CON3),
	REGNAME(apmixedsys, 0x33c, APLL3_CON4),
	REGNAME(apmixedsys, 0x340, MFGPLL_CON0),
	REGNAME(apmixedsys, 0x344, MFGPLL_CON1),
	REGNAME(apmixedsys, 0x348, MFGPLL_CON2),
	REGNAME(apmixedsys, 0x34c, MFGPLL_CON3),
	REGNAME(apmixedsys, 0x404, APLL4_CON0),
	REGNAME(apmixedsys, 0x408, APLL4_CON1),
	REGNAME(apmixedsys, 0x40c, APLL4_CON2),
	REGNAME(apmixedsys, 0x410, APLL4_CON3),
	REGNAME(apmixedsys, 0x414, APLL4_CON4),
	REGNAME(apmixedsys, 0x418, APLL5_CON0),
	REGNAME(apmixedsys, 0x41c, APLL5_CON1),
	REGNAME(apmixedsys, 0x420, APLL5_CON2),
	REGNAME(apmixedsys, 0x424, APLL5_CON3),
	REGNAME(apmixedsys, 0x428, APLL5_CON4),
	REGNAME(apmixedsys, 0x42c, ADSPPLL_CON0),
	REGNAME(apmixedsys, 0x430, ADSPPLL_CON1),
	REGNAME(apmixedsys, 0x434, ADSPPLL_CON2),
	REGNAME(apmixedsys, 0x438, ADSPPLL_CON3),
	REGNAME(apmixedsys, 0x43c, MPLL_CON0),
	REGNAME(apmixedsys, 0x440, MPLL_CON1),
	REGNAME(apmixedsys, 0x444, MPLL_CON2),
	REGNAME(apmixedsys, 0x448, MPLL_CON3),
	REGNAME(apmixedsys, 0x44c, ETHPLL_CON0),
	REGNAME(apmixedsys, 0x450, ETHPLL_CON1),
	REGNAME(apmixedsys, 0x454, ETHPLL_CON2),
	REGNAME(apmixedsys, 0x458, ETHPLL_CON3),
	REGNAME(apmixedsys, 0x45c, MAINPLL_CON0),
	REGNAME(apmixedsys, 0x460, MAINPLL_CON1),
	REGNAME(apmixedsys, 0x464, MAINPLL_CON2),
	REGNAME(apmixedsys, 0x468, MAINPLL_CON3),
	REGNAME(apmixedsys, 0x504, UNIVPLL_CON0),
	REGNAME(apmixedsys, 0x508, UNIVPLL_CON1),
	REGNAME(apmixedsys, 0x50c, UNIVPLL_CON2),
	REGNAME(apmixedsys, 0x510, UNIVPLL_CON3),
	REGNAME(apmixedsys, 0x514, MSDCPLL_CON0),
	REGNAME(apmixedsys, 0x518, MSDCPLL_CON1),
	REGNAME(apmixedsys, 0x51c, MSDCPLL_CON2),
	REGNAME(apmixedsys, 0x520, MSDCPLL_CON3),
	REGNAME(apmixedsys, 0x524, TVDPLL1_CON0),
	REGNAME(apmixedsys, 0x528, TVDPLL1_CON1),
	REGNAME(apmixedsys, 0x52c, TVDPLL1_CON2),
	REGNAME(apmixedsys, 0x530, TVDPLL1_CON3),
	REGNAME(apmixedsys, 0x534, TVDPLL2_CON0),
	REGNAME(apmixedsys, 0x538, TVDPLL2_CON1),
	REGNAME(apmixedsys, 0x53c, TVDPLL2_CON2),
	REGNAME(apmixedsys, 0x540, TVDPLL2_CON3),
	REGNAME(apmixedsys, 0x544, MMPLL_CON0),
	REGNAME(apmixedsys, 0x548, MMPLL_CON1),
	REGNAME(apmixedsys, 0x54c, MMPLL_CON2),
	REGNAME(apmixedsys, 0x550, MMPLL_CON3),
	REGNAME(apmixedsys, 0x554, IMGPLL_CON0),
	REGNAME(apmixedsys, 0x558, IMGPLL_CON1),
	REGNAME(apmixedsys, 0x55c, IMGPLL_CON2),
	REGNAME(apmixedsys, 0x560, IMGPLL_CON3),
	REGNAME(topckgen, 0x020, CLK_CFG_0),
	REGNAME(topckgen, 0x02C, CLK_CFG_1),
	REGNAME(topckgen, 0x038, CLK_CFG_2),
	REGNAME(topckgen, 0x044, CLK_CFG_3),
	REGNAME(topckgen, 0x050, CLK_CFG_4),
	REGNAME(topckgen, 0x05C, CLK_CFG_5),
	REGNAME(topckgen, 0x068, CLK_CFG_6),
	REGNAME(topckgen, 0x074, CLK_CFG_7),
	REGNAME(topckgen, 0x080, CLK_CFG_8),
	REGNAME(topckgen, 0x08C, CLK_CFG_9),
	REGNAME(topckgen, 0x098, CLK_CFG_10),
	REGNAME(topckgen, 0x0A4, CLK_CFG_11),
	REGNAME(topckgen, 0x0B0, CLK_CFG_12),
	REGNAME(topckgen, 0x0BC, CLK_CFG_13),
	REGNAME(topckgen, 0x0C8, CLK_CFG_14),
	REGNAME(topckgen, 0x0D4, CLK_CFG_15),
	REGNAME(topckgen, 0x0E0, CLK_CFG_16),
	REGNAME(topckgen, 0x0EC, CLK_CFG_17),
	REGNAME(topckgen, 0x0F8, CLK_CFG_18),
	REGNAME(topckgen, 0x0104, CLK_CFG_19),
	REGNAME(topckgen, 0x0110, CLK_CFG_20),
	REGNAME(topckgen, 0x011C, CLK_CFG_21),
	REGNAME(topckgen, 0x0128, CLK_CFG_22),
	REGNAME(topckgen, 0x0250, CLK_MISC_CFG_3),
	REGNAME(topckgen, 0x0328, CLK_AUDDIV_2),
	REGNAME(topckgen, 0x0334, CLK_AUDDIV_3),
	REGNAME(topckgen, 0x0338, CLK_AUDDIV_4),
	REGNAME(infracfg_ao, 0x90, MODULE_SW_CG_0),
	REGNAME(infracfg_ao, 0x94, MODULE_SW_CG_1),
	REGNAME(infracfg_ao, 0xac, MODULE_SW_CG_2),
	REGNAME(infracfg_ao, 0xc8, MODULE_SW_CG_3),
	REGNAME(infracfg_ao, 0xe8, MODULE_SW_CG_4),
	REGNAME(infracfg_ao, 0x228, INFRA_TOPAXI_PROTECTEN_STA1),
	REGNAME(infracfg_ao, 0x258, INFRA_TOPAXI_PROTECTEN_STA1_1),
	REGNAME(infracfg_ao, 0x724, INFRA_TOPAXI_PROTECTEN_STA1_2),
	REGNAME(infracfg_ao, 0x2EC, INFRA_TOPAXI_PROTECTEN_MM_STA1),
	REGNAME(infracfg_ao, 0xDD8, INFRA_TOPAXI_PROTECTEN_MM_STA1_2),
	REGNAME(infracfg_ao, 0xB90, INFRA_TOPAXI_PROTECTEN_VDNR_STA1),
	REGNAME(infracfg_ao, 0xBD8, INFRA_TOPAXI_PROTECTEN_SUB_INFRA_VDNR_STA1),
	REGNAME(audsys_src, 0x4, MEM_ASRC_TOP_CON1),
	REGNAME(audsys, 0x0, AUDIO_TOP_0),
	REGNAME(audsys, 0x4, AUDIO_TOP_1),
	REGNAME(audsys, 0x8, AUDIO_TOP_2),
	REGNAME(audsys, 0xc, AUDIO_TOP_3),
	REGNAME(audsys, 0x10, AUDIO_TOP_4),
	REGNAME(audsys, 0x14, AUDIO_TOP_5),
	REGNAME(audsys, 0x18, AUDIO_TOP_6),
	REGNAME(pericfg_ao, 0x18, PERI_MODULE_SW_CG_0),
	REGNAME(imp_iic_wrap_c, 0xe00, AP_CLOCK_CG_CEN),
	REGNAME(imp_iic_wrap_w, 0xe00, AP_CLOCK_CG_WST),
	REGNAME(imp_iic_wrap_en, 0xe00, AP_CLOCK_CG_EST_NOR),
	REGNAME(mfgcfg, 0x0, MFG_CG),
	REGNAME(vppsys0, 0x20, VPPSYS0_CG0),
	REGNAME(vppsys0, 0x2c, VPPSYS0_CG1),
	REGNAME(vppsys0, 0x38, VPPSYS0_CG2),
	REGNAME(wpesys, 0x0, WPESYS_RG_000),
	REGNAME(wpesys_vpp0, 0x5c, CTL_DMA_DCM_DIS),
	REGNAME(wpesys_vpp0, 0x58, CTL_WPE_DCM_DIS),
	REGNAME(vppsys1, 0x100, VPPSYS1_CG_0),
	REGNAME(vppsys1, 0x110, VPPSYS1_CG_1),
	REGNAME(imgsys, 0x0, IMG_MAIN_CG),
	REGNAME(imgsys1_dip_top, 0x0, MACRO_CG),
	REGNAME(imgsys1_dip_nr, 0x0, MACRO_CG),
	REGNAME(imgsys_wpe1, 0x0, MACRO_CG),
	REGNAME(ipesys, 0x0, MACRO_CG),
	REGNAME(imgsys_wpe2, 0x0, MACRO_CG),
	REGNAME(imgsys_wpe3, 0x0, MACRO_CG),
	REGNAME(camsys, 0x0, CAM_MAIN_CG),
	REGNAME(camsys_rawa, 0x0, CAMSYS_CG),
	REGNAME(camsys_yuva, 0x0, CAMSYS_CG),
	REGNAME(camsys_rawb, 0x0, CAMSYS_CG),
	REGNAME(camsys_yuvb, 0x0, CAMSYS_CG),
	REGNAME(ccusys, 0x0, CCUSYS_CG),
	REGNAME(vdecsys_soc, 0x8, LARB_CKEN_CON),
	REGNAME(vdecsys_soc, 0x200, LAT_CKEN),
	REGNAME(vdecsys_soc, 0x0, VDEC_CKEN),
	REGNAME(vdecsys, 0x8, LARB_CKEN_CON),
	REGNAME(vdecsys, 0x200, LAT_CKEN),
	REGNAME(vdecsys, 0x0, VDEC_CKEN),
	REGNAME(vencsys, 0x0, VENCSYS_CG),
	REGNAME(vdosys0, 0x100, GLOBAL0_CG_0),
	REGNAME(vdosys0, 0x110, GLOBAL0_CG_1),
	REGNAME(vdosys0, 0x120, GLOBAL0_CG_2),
	REGNAME(vdosys1, 0x100, VDOSYS1_CG_0),
	REGNAME(vdosys1, 0x110, VDOSYS1_CG_1),
	REGNAME(vdosys1, 0x120, VDOSYS1_CG_2),
	REGNAME(vdosys1, 0x130, VDOSYS1_CG_3),
	REGNAME(vdosys1, 0x140, VDOSYS1_CG_4),
	{},
};

static const struct regname *get_all_mt8188_regnames(void)
{
	return rn;
}

static void init_regbase(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rb) - 1; i++) {
		if (!rb[i].phys)
			continue;

		rb[i].virt = ioremap(rb[i].phys, 0x1000);
	}
}

/*
 * clkchk pwr_status
 */
static u32 pwr_ofs[STA_NUM] = {
	[PWR_STA] = 0x16C,
	[PWR_STA2] = 0x170,
	[CPU_PWR_STA] = 0x174,
	[CPU_PWR_STA2] = 0x178,
};

u32 *get_spm_pwr_status_array(void)
{
	static void __iomem *scpsys_base, *pwr_addr[STA_NUM];
	static u32 pwr_sta[STA_NUM];
	int i;

	for (i = 0; i < STA_NUM; i++) {
		if (!scpsys_base)
			scpsys_base = ioremap(0x10006000, PAGE_SIZE);

		if (pwr_ofs[i]) {
			pwr_addr[i] = scpsys_base + pwr_ofs[i];
			pwr_sta[i] = clk_readl(pwr_addr[i]);
		}
	}

	return pwr_sta;
}

/*
 * clkchk pwr_state
 */
static struct pvd_msk pvd_pwr_mask[] = {
	{"mfgcfg", CPU_PWR_STA, 0x00000004},		// BIT(2), MFG1
	{"audsys", PWR_STA, 0x00000040},		// BIT(6), AUDIO
	{"audsys_src", PWR_STA, 0x00000080},		// BIT(7), AUDIO_ASRC
	{"vppsys0", PWR_STA, 0x00000800},		// BIT(11), VPPSYS0
	{"vppsys1", PWR_STA, 0x00001000},		// BIT(12), VPPSYS1
	{"vdosys0", PWR_STA, 0x00002000},		// BIT(13), VDOSYS0
	{"vdosys1", PWR_STA, 0x00004000},		// BIT(14), VDOSYS1
	{"wpesys", PWR_STA, 0x00008000},		// BIT(15), WPE
	{"wpesys_vpp0", PWR_STA, 0x00008000},		// BIT(15), WPE
	{"vdecsys_soc", PWR_STA, 0x00080000},		// BIT(19), VDEC0
	{"vdecsys", PWR_STA, 0x00100000},		// BIT(20), VDEC1
	{"vencsys", PWR_STA, 0x00400000},		// BIT(22), VENC
	{"camsys", PWR_STA, 0x01000000},		// BIT(24), CAM_MAIN
	{"ccusys", PWR_STA, 0x01000000},		// BIT(24), CAM_MAIN
	{"camsys_rawa", PWR_STA, 0x02000000},		// BIT(25), CAM_SUBA
	{"camsys_yuva", PWR_STA, 0x02000000},		// BIT(25), CAM_SUBA
	{"camsys_rawb", PWR_STA, 0x04000000},		// BIT(26), CAM_SUBB
	{"camsys_yuvb", PWR_STA, 0x04000000},		// BIT(26), CAM_SUBB
	{"imgsys", PWR_STA, 0x20000000},		// BIT(29), IMG_MAIN
	{"imgsys1_dip_top", PWR_STA, 0x40000000},	// BIT(30), DIP
	{"imgsys1_dip_nr", PWR_STA, 0x40000000},	// BIT(30), DIP
	{"imgsys_wpe1", PWR_STA, 0x40000000},		// BIT(30), DIP
	{"imgsys_wpe2", PWR_STA, 0x40000000},		// BIT(30), DIP
	{"imgsys_wpe3", PWR_STA, 0x40000000},		// BIT(30), DIP
	{"ipesys", PWR_STA, 0x80000000},		// BIT(31), IPE
	{},
};

static struct pvd_msk *get_pvd_pwr_mask(void)
{
	return pvd_pwr_mask;
}

void print_subsys_reg_mt8188(enum chk_sys_id id)
{
	struct regbase *rb_dump;
	const struct regname *rns = &rn[0];
	int i;

	if (rns == NULL)
		return;

	if (id >= chk_sys_num || id < 0) {
		pr_info("wrong id:%d\n", id);
		return;
	}

	rb_dump = &rb[id];

	for (i = 0; i < ARRAY_SIZE(rn) - 1; i++, rns++) {
		if (!is_valid_reg(ADDR(rns)))
			return;

		/* filter out the subsys that we don't want */
		if (rns->base != rb_dump)
			continue;

		pr_info("%-18s: [0x%08x] = 0x%08x\n",
			rns->name, PHYSADDR(rns), clk_readl(ADDR(rns)));
	}
}
EXPORT_SYMBOL(print_subsys_reg_mt8188);

/*
 * clkchk dump_clks
 */
static const char * const off_pll_names[] = {
	"ethpll",
	"msdcpll",
	"tvdpll1",
	"tvdpll2",
	"mmpll",
	"imgpll",
	"univpll",
	"adsppll",
	"apll1",
	"apll2",
	"apll3",
	"apll4",
	"apll5",
	"mfgpll",
	NULL
};

static const char * const *get_off_pll_names(void)
{
	return off_pll_names;
}

static const char * const notice_off_pll_names[] = {
	NULL
};

static const char * const *get_notice_pll_names(void)
{
	return notice_off_pll_names;
}

static void __iomem *scpsys_base, *pwr_sta, *pwr_sta_2nd, *cpu_sta, *cpu_sta_2nd;

static void dump_pwr_status(void)
{
	static const char * const pwr_names[] = {
		/* PWR_STATUS & PWR_STATUS_2ND */
		[0] = "",
		[1] = "ETHER",
		[2] = "DPY0",
		[3] = "DPY1",
		[4] = "DPM0",
		[5] = "DPM1",
		[6] = "AUDIO",
		[7] = "AUDIO_ASRC",
		[8] = "ADSP",
		[9] = "ADSP_INFRA",
		[10] = "ADSP_AO",
		[11] = "VPPSYS0",
		[12] = "VPPSYS1",
		[13] = "VDOSYS0",
		[14] = "VDOSYS1",
		[15] = "WPESYS",
		[16] = "DP_TX",
		[17] = "EDP_TX",
		[18] = "HDMI_TX",
		[19] = "VDE0",
		[20] = "VDE1",
		[21] = "",
		[22] = "VEN",
		[23] = "",
		[24] = "CAM_MAIN",
		[25] = "CAM_SUBA",
		[26] = "CAM_SUBB",
		[27] = "CAM_VCORE",
		[28] = "IMG_VCORE",
		[29] = "IMG_MAIN",
		[30] = "IMG_DIP",
		[31] = "IMG_IPE",
		/* CPU_PWR_STATUS & CPU_PWR_STATUS_2ND */
		[32] = "MCUPM",
		[33] = "MFG0",
		[34] = "MFG1",
		[35] = "MFG2",
		[36] = "MFG3",
		[37] = "MFG4",
		[38] = "",
		[39] = "IFR",
		[40] = "IFR_SUB",
		[41] = "PERI",
		[42] = "PEXTP_MAC_TOP_P0",
		[43] = "",
		[44] = "PEXTP_PHY_TOP",
		[45] = "",
		[46] = "",
		[47] = "",
		[48] = "",
		[49] = "CSI_RX_TOP",
		[50] = "APHY_N",
		[51] = "APHY_S",
		[52] = "",
		[53] = "",
		[54] = "",
		[55] = "",
		[56] = "",
		[57] = "",
		[58] = "",
		[59] = "",
		[60] = "",
		[61] = "",
		[62] = "",
		[63] = "",
		/* END */
		[64] = NULL,
	};

	u32 pwr_status_val = 0, pwr_status_2nd_val = 0;
	u32 cpu_status_val = 0, cpu_status_2nd_val = 0;
	u8 i = 0;

	if (scpsys_base == NULL || pwr_sta == NULL || pwr_sta_2nd == NULL
				|| cpu_sta == NULL || cpu_sta_2nd == NULL)
		return;

	pwr_status_val = readl(pwr_sta);
	pwr_status_2nd_val = readl(pwr_sta_2nd);
	cpu_status_val = readl(cpu_sta);
	cpu_status_2nd_val = readl(cpu_sta_2nd);

	pr_info("PWR_STATUS: 0x%08x\n", pwr_status_val);
	pr_info("PWR_STATUS_2ND: 0x%08x\n", pwr_status_2nd_val);
	pr_info("CPU_STATUS: 0x%08x\n", cpu_status_val);
	pr_info("CPU_STATUS_2ND: 0x%08x\n", cpu_status_2nd_val);

	for (i = 0; i < 32; i++) {
		const char *st = (pwr_status_val & BIT(i)) != 0U ? "ON" : "OFF";
		const char *st_2nd = (pwr_status_2nd_val & BIT(i)) != 0U ? "ON" : "OFF";

		if (pwr_names[i] == NULL || !strcmp(pwr_names[i], ""))
			continue;

		if (!strcmp(st, "ON") && !strcmp(st_2nd, "ON"))
			pr_info("[%2d]: %3s: %s\n", i, st, pwr_names[i]);
	}

	for (i = 32; i < 64; i++) {
		const char *st = (cpu_status_val & BIT(i-32)) != 0U ? "ON" : "OFF";
		const char *st_2nd = (cpu_status_2nd_val & BIT(i-32)) != 0U ? "ON" : "OFF";

		if (pwr_names[i] == NULL || !strcmp(pwr_names[i], ""))
			continue;

		if (!strcmp(st, "ON") && !strcmp(st_2nd, "ON"))
			pr_info("[%2d]: %3s: %s\n", i, st, pwr_names[i]);
	}
}

/*
 * init functions
 */

static struct clkchk_ops clkchk_mt8188_ops = {
	.get_all_regnames = get_all_mt8188_regnames,
	.get_spm_pwr_status_array = get_spm_pwr_status_array,
	.get_pvd_pwr_mask = get_pvd_pwr_mask,
	.get_off_pll_names = get_off_pll_names,
	.get_notice_pll_names = get_notice_pll_names,
	.dump_pwr_status = dump_pwr_status,
};

static int clk_chk_mt8188_probe(struct platform_device *pdev)
{
	init_regbase();

	set_clkchk_ops(&clkchk_mt8188_ops);
	return 0;
}

static struct platform_driver clk_chk_mt8188_drv = {
	.probe = clk_chk_mt8188_probe,
	.driver = {
		.name = "clk-chk-mt8188",
		.owner = THIS_MODULE,
		.pm = &clk_chk_dev_pm_ops,
	},
};

/*
 * init functions
 */

static int __init clkchk_mt8188_init(void)
{
	static struct platform_device *clk_chk_dev;

	scpsys_base = ioremap(0x10006000, PAGE_SIZE);
	pwr_sta = scpsys_base + 0x16C;
	pwr_sta_2nd = scpsys_base + 0x170;
	cpu_sta = scpsys_base + 0x174;
	cpu_sta_2nd = scpsys_base + 0x178;

	clk_chk_dev = platform_device_register_simple("clk-chk-mt8188", -1, NULL, 0);
	if (IS_ERR(clk_chk_dev))
		pr_info("unable to register clk-chk device");

	return platform_driver_register(&clk_chk_mt8188_drv);
}

static void __exit clkchk_mt8188_exit(void)
{
	platform_driver_unregister(&clk_chk_mt8188_drv);
}

subsys_initcall(clkchk_mt8188_init);
module_exit(clkchk_mt8188_exit);
MODULE_LICENSE("GPL");
