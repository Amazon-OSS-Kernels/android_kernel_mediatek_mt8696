// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2021 MediaTek Inc.
// Author: Qiqi Wang <qiqi.wang@mediatek.com>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>

#include <dt-bindings/power/mt8696-power.h>

#include "clkchk.h"
#include "clkchk-mt8696.h"

/*
 * clkchk dump_regs
 */

#define REGBASE_V(_phys, _id_name, _pg, _pn) { .phys = _phys,	\
		.name = #_id_name, .pg = _pg, .pn = _pn}

static struct regbase rb[] = {
	[spm] = REGBASE_V(0x10006000, spm, PD_NULL, CLK_NULL),
	[topckgen] = REGBASE_V(0x10000000, topckgen, PD_NULL, CLK_NULL),
	[infracfg] = REGBASE_V(0x10001000, infracfg, PD_NULL, CLK_NULL),
	[pericfg]   = REGBASE_V(0x10003000, pericfg, PD_NULL, CLK_NULL),
	[apmixedsys]  = REGBASE_V(0x1000c000, apmixedsys, PD_NULL, CLK_NULL),
	[fhctl]     = REGBASE_V(0x10209e00, fhctl, PD_NULL, CLK_NULL),
	[mcucfg]  = REGBASE_V(0x0c530000, mcucfg, PD_NULL, CLK_NULL),
	[ipsys]  = REGBASE_V(0x10218000, ipsys, MT8696_POWER_DOMAIN_NNA0, CLK_NULL),
	[ether]  = REGBASE_V(0x1101a000, ether, MT8696_POWER_DOMAIN_ETHER_PCIE, CLK_NULL),
	[mfgcfg]  = REGBASE_V(0x13fbf000, mfgcfg, MT8696_POWER_DOMAIN_MFG, CLK_NULL),
	[mmsys]    = REGBASE_V(0x14000000, mmsys, MT8696_POWER_DOMAIN_MMSYS_COMMON, CLK_NULL),
	[imgsys]   = REGBASE_V(0x15000000, imgsys, MT8696_POWER_DOMAIN_DISP_MAIN, CLK_NULL),
	[vdecsoc]  = REGBASE_V(0x1600f000, vdecsoc, MT8696_POWER_DOMAIN_VDEC_SOC, CLK_NULL),
	[vdeccore]    = REGBASE_V(0x1602f000, vdeccore, MT8696_POWER_DOMAIN_VDEC_CORE0, CLK_NULL),
	[vencsys]   = REGBASE_V(0x18000000, vencsys, MT8696_POWER_DOMAIN_VENC, CLK_NULL),
	{},
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
	{},
};

static const struct regname *get_all_mt8696_regnames(void)
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
	{"ipsys", PWR_STA, 0x00020000},		// BIT(17), NNA0
	{"ether", PWR_STA, 0x00010000},		// BIT(16), ETHER_PCIE
	{"mfgcfg", PWR_STA, 0x00100000},		// BIT(20), MFG
	{"mmsys", PWR_STA, 0x00000008},		// BIT(3), MMSYS_COMMMON
	{"imgsys", PWR_STA, 0x00000080},		// BIT(7), DISP_MAIN
	{"vdecsoc", PWR_STA, 0x01000000},		// BIT(24), VDEC_SOC
	{"vdeccore", PWR_STA, 0x00000001},		// BIT(0), VDEC_CORE0
	{"vencsys", PWR_STA, 0x00000002},		// BIT(1), VENC
	{},
};

static struct pvd_msk *get_pvd_pwr_mask(void)
{
	return pvd_pwr_mask;
}

void print_subsys_reg_mt8696(enum chk_sys_id id)
{
	struct regbase *rb_dump;
	const struct regname *rns = &rn[0];
	int i;

	if (id >= chk_sys_num) {
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
EXPORT_SYMBOL(print_subsys_reg_mt8696);

/*
 * clkchk dump_clks
 */
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

static void __iomem *scpsys_base, *pwr_sta, *pwr_sta_2nd;

static void dump_pwr_status(void)
{
	static const char * const pwr_names[] = {
		/* PWR_STATUS & PWR_STATUS_2ND */
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

	u32 pwr_status_val = 0, pwr_status_2nd_val = 0;
	u8 i = 0;

	if (scpsys_base == NULL || pwr_sta == NULL || pwr_sta_2nd == NULL)
		return;

	pwr_status_val = readl(pwr_sta);
	pwr_status_2nd_val = readl(pwr_sta_2nd);

	pr_info("PWR_STATUS: 0x%08x\n", pwr_status_val);
	pr_info("PWR_STATUS_2ND: 0x%08x\n", pwr_status_2nd_val);

	for (i = 0; i < 32; i++) {
		const char *st = (pwr_status_val & BIT(i)) != 0U ? "ON" : "OFF";
		const char *st_2nd = (pwr_status_2nd_val & BIT(i)) != 0U ? "ON" : "OFF";

		if (pwr_names[i] == NULL || !strcmp(pwr_names[i], ""))
			continue;

		if (!strcmp(st, "ON") && !strcmp(st_2nd, "ON"))
			pr_info("[%2d]: %3s: %s\n", i, st, pwr_names[i]);
	}
}

/*
 * init functions
 */

static struct clkchk_ops clkchk_mt8696_ops = {
	.get_all_regnames = get_all_mt8696_regnames,
	.get_spm_pwr_status_array = get_spm_pwr_status_array,
	.get_pvd_pwr_mask = get_pvd_pwr_mask,
	.get_off_pll_names = get_off_pll_names,
	.get_notice_pll_names = get_notice_pll_names,
	.dump_pwr_status = dump_pwr_status,
};

static int clk_chk_mt8696_probe(struct platform_device *pdev)
{
	init_regbase();

	set_clkchk_ops(&clkchk_mt8696_ops);
	return 0;
}

static struct platform_driver clk_chk_mt8696_drv = {
	.probe = clk_chk_mt8696_probe,
	.driver = {
		.name = "clk-chk-mt8696",
		.owner = THIS_MODULE,
		.pm = &clk_chk_dev_pm_ops,
	},
};

/*
 * init functions
 */

static int __init clkchk_mt8696_init(void)
{
	static struct platform_device *clk_chk_dev;

	scpsys_base = ioremap(0x10006000, PAGE_SIZE);
	pwr_sta = scpsys_base + 0x16C;
	pwr_sta_2nd = scpsys_base + 0x170;

	clk_chk_dev = platform_device_register_simple("clk-chk-mt8696", -1, NULL, 0);
	if (IS_ERR(clk_chk_dev))
		pr_info("unable to register clk-chk device");

	return platform_driver_register(&clk_chk_mt8696_drv);
}

static void __exit clkchk_mt8696_exit(void)
{
	platform_driver_unregister(&clk_chk_mt8696_drv);
}

subsys_initcall(clkchk_mt8696_init);
module_exit(clkchk_mt8696_exit);
MODULE_LICENSE("GPL");
