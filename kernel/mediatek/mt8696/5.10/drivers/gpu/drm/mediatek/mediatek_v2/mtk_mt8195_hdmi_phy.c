// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_platform.h>

#include "mtk_hdmi_regs.h"
#include "mtk_hdmi_phy.h"

static const char * const mtk_hdmi_phy_clk_names[MTK_HDMI_PHY_CLK_COUNT] = {
	[MTK_HDMI_PHY_XTAL_SEL] = "hdmi_xtal_sel",
};

unsigned char mtk_hdmi_phy_log = 1;

#define HDMI_PHY_LOG(fmt, arg...) \
	do {	if (mtk_hdmi_phy_log) { \
		pr_info("[HDMI][phy] %s,%d "fmt, __func__, __LINE__, ##arg); \
		} \
	} while (0)

#define HDMI_PHY_FUNC()	\
	do {	if (mtk_hdmi_phy_log) \
		pr_info("[HDMI][hpy] %s\n", __func__); \
	} while (0)

static inline bool mtk_hdmi_phy_readbit(struct mtk_hdmi_phy *hdmi_phy,
	unsigned short reg, unsigned int offset)
{
	return (readl(hdmi_phy->regs + reg) & offset) ? true : false;
}

unsigned int mtk_hdmi_phy_read(
	struct mtk_hdmi_phy *hdmi_phy, unsigned short reg)
{
	return readl(hdmi_phy->regs + reg);
}

void mtk_hdmi_phy_write(struct mtk_hdmi_phy *hdmi_phy,
	unsigned short reg, unsigned int val)
{
	writel(val, hdmi_phy->regs + reg);
}

static inline void mtk_hdmi_phy_mask(struct mtk_hdmi_phy *hdmi_phy,
	unsigned int reg, unsigned int val, unsigned int mask)
{
	unsigned int tmp;

	tmp = readl(hdmi_phy->regs + reg) & ~mask;
	tmp |= (val & mask);
	writel(tmp, hdmi_phy->regs + reg);
}

static inline struct mtk_hdmi_phy *to_mtk_hdmi_phy(struct clk_hw *hw)
{
	return container_of(hw, struct mtk_hdmi_phy, txpll_hw);
}

//can.zeng todo verify
static inline struct mtk_hdmi *to_mtk_hdmi(struct mtk_hdmi_phy *hdmi_phy)
{
	return global_mtk_hdmi;
}

/*********Analog API export to HDMI Digital start*****/
void mtk_hdmi_ana_fifo_en(struct mtk_hdmi_phy *hdmi_phy)
{
	HDMI_PHY_FUNC();

	/*make data fifo writable for hdmi2.0*/
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_ANA_CTL, reg_ana_hdmi20_fifo_en,
		reg_ana_hdmi20_fifo_en);
}


//void hdmitx_confighdmisetting(unsigned int resolutionmode)
void mtk_tmds_high_bit_clk_ratio(struct mtk_hdmi *hdmi, bool enable)
{
	struct mtk_hdmi_phy *hdmi_phy =	hdmi->hdmi_phy_base;

	HDMI_PHY_FUNC();

	mtk_hdmi_ana_fifo_en(hdmi_phy);

/* HDMI 2.0 specification, 3.4Gbps <= TMDS Bit Rate <= 6G,
 * clock bit ratio 1:40, under 3.4Gbps, clock bit ratio 1:10
 */
	if (enable == true) {
		HDMI_PHY_LOG("Over 3.4Gbps, clk bit ratio 1:40\n");
		mtk_hdmi_phy_mask(hdmi_phy, HDMI20_CLK_CFG,
			(0x2 << reg_txc_div_SHIFT), reg_txc_div);
	} else {
		HDMI_PHY_LOG("Under 3.4Gbps, clk bit ratio 1:10\n");
		mtk_hdmi_phy_mask(hdmi_phy, HDMI20_CLK_CFG, 0, reg_txc_div);
	}
}

/*********Analog API export to HDMI Digital end*******/

const char *txpll_parent_dts_names[TXPLL_CLK_PARENT_COUNT] = {
	"tvdpll", "hdmirx_clk", "xtal26m"};

static int mtk_hdmi_txpll_select_source(struct clk_hw *hw,
	unsigned char hdmirx_ref_ck_div,
	unsigned char ad_respll_ck_div)
{

	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);

	if ((hdmi_phy->clk_parent < PLLGP1_PLLGP_2H_PLL2) ||
		(hdmi_phy->clk_parent >= TXPLL_CLK_PARENT_COUNT)) {
		HDMI_PHY_LOG("%s: err: wrong clk parent\n", __func__);
		return -EINVAL;
	}

/* if txpll source is hdmirx_ref_ck or ad_respll_ck:
 * 5-bit divider: 0b00000=0x0-> /1, 0b00001=0x1 -> /2, 0b11111=0x1f -> /32
 */
	if (hdmi_phy->clk_parent == PLLGP1_PLLGP_2H_PLL2) {
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			(ad_respll_ck_div - 1) << REG_RESPLL_DIV_CONFIG_SHIFT,
			REG_RESPLL_DIV_CONFIG);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x1 << REG_HDMITX_REF_RESPLL_SEL_SHIFT,
			REG_HDMITX_REF_RESPLL_SEL);
		HDMI_PHY_LOG("txpll source clk is TVD PLL\n");
		HDMI_PHY_LOG("ad_respll_clk_div = %x\n", ad_respll_ck_div);

	} else if (hdmi_phy->clk_parent == HDMIRX) {
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			(hdmirx_ref_ck_div - 1) << REG_HDMIRX_DIV_CONFG_SHIFT,
			REG_HDMIRX_DIV_CONFG);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x1 << REG_HDMITX_REF_XTAL_SEL_SHIFT,
			REG_HDMITX_REF_XTAL_SEL);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x0 << REG_HDMITX_REF_RESPLL_SEL_SHIFT,
			REG_HDMITX_REF_RESPLL_SEL);
		HDMI_PHY_LOG("txpll source clk is HDMIRX REF CLK\n");
		HDMI_PHY_LOG("hdmirx_ref_clk_div = %x\n", hdmirx_ref_ck_div);

	} else if (hdmi_phy->clk_parent == XTAL26M) {
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x0 << REG_HDMITX_REF_XTAL_SEL_SHIFT,
			REG_HDMITX_REF_XTAL_SEL);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x0 << REG_HDMITX_REF_RESPLL_SEL_SHIFT,
			REG_HDMITX_REF_RESPLL_SEL);
		HDMI_PHY_LOG("txpll source clk is xTal26M CLK\n");

	}

	/*DA_HDMITX21_REF_CK for TXPLL input source*/
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_10,
		0x0 << RG_HDMITXPLL_REF_CK_SEL_SHIFT, RG_HDMITXPLL_REF_CK_SEL);

	return 0;
}

static int mtk_hdmi_txpll_performance_setting(struct clk_hw *hw)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);
	struct mtk_hdmi *hdmi = to_mtk_hdmi(hdmi_phy);

/* no matter txpll input source is HDMIRX_REF_CK, xTal26M or TVD PLL,
 * the performance configuration is the same.
 * RG_HDMITXPLL_BP2 always 1'b1 = 0x1
 * RG_HDMITXPLL_BC[1:0] always 2'b11 = 0x3
 * RG_HDMITXPLL_IC[4:0] always 5'b00001 = 0x1
 * RG_HDMITXPLL_BR[2:0] stage treatment:
 *      24bit or 48bit->3'b001 = 0x1
 *      30bit or 36bit->3'b011 = 0x3
 * RG_HDMITXPLL_IR[4:0] stage treatment:
 *      24bit,30bit,48bit ->5'b00010 = 0x2
 *      36bit ->5'b00011 = 0x3
 * RG_HDMITXPLL_BP[3:0] always 4'b1111 = 0xf
 * RG_HDMITXPLL_IBAND_FIX_EN, RG_HDMITXPLL_RESERVE[14] always 2'b00 = 0x0
 * RG_HDMITXPLL_HIKVCO always 1'b1 = 0x1
 */

	/* BP2 */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_0,
		0x1 << RG_HDMITXPLL_BP2_SHIFT, RG_HDMITXPLL_BP2);

	/* BC */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
		0x3 << RG_HDMITXPLL_BC_SHIFT, RG_HDMITXPLL_BC);

	/* IC */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
		0x1 << RG_HDMITXPLL_IC_SHIFT, RG_HDMITXPLL_IC);

	/* BR */
	if ((hdmi->color_depth == HDMI_8_BIT) ||
		(hdmi->color_depth == HDMI_10_BIT) ||
		(hdmi->color_depth == HDMI_16_BIT))
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
			0x2 << RG_HDMITXPLL_BR_SHIFT, RG_HDMITXPLL_BR);
	else if (hdmi->color_depth == HDMI_12_BIT)
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
			0x1 << RG_HDMITXPLL_BR_SHIFT, RG_HDMITXPLL_BR);
	else {
		HDMI_PHY_LOG("%s, unknown color depth\n", __func__);
		return -EINVAL;
	}

	/* IR */
	if ((hdmi->color_depth == HDMI_8_BIT) ||
		(hdmi->color_depth == HDMI_16_BIT))
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
			0x2 << RG_HDMITXPLL_IR_SHIFT, RG_HDMITXPLL_IR);
	else if (hdmi->color_depth == HDMI_10_BIT)
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
			0x3 << RG_HDMITXPLL_IR_SHIFT, RG_HDMITXPLL_IR);
	else if (hdmi->color_depth == HDMI_12_BIT)
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
			0x4 << RG_HDMITXPLL_IR_SHIFT, RG_HDMITXPLL_IR);
	else {
		HDMI_PHY_LOG("%s, unknown color depth\n", __func__);
		return -EINVAL;
	}

	/* BP */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
		0xf << RG_HDMITXPLL_BP_SHIFT, RG_HDMITXPLL_BP);

	/* IBAND_FIX_EN, RESERVE[14] */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_0,
		0x0 << RG_HDMITXPLL_IBAND_FIX_EN_SHIFT,
		RG_HDMITXPLL_IBAND_FIX_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_1,
		0x0 << RG_HDMITXPLL_RESERVE_BIT14_SHIFT,
		RG_HDMITXPLL_RESERVE_BIT14);

	/* HIKVCO */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
		0x0 << RG_HDMITXPLL_HIKVCO_SHIFT, RG_HDMITXPLL_HIKVCO);

	/* HREN */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_0,
		0x1 << RG_HDMITXPLL_HREN_SHIFT, RG_HDMITXPLL_HREN);

	/* LVR_SEL */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_0,
		0x1 << RG_HDMITXPLL_LVR_SEL_SHIFT, RG_HDMITXPLL_LVR_SEL);

	/* RG_HDMITXPLL_RESERVE[12:11] */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_1,
		0x3 << RG_HDMITXPLL_RESERVE_BIT12_11_SHIFT, RG_HDMITXPLL_RESERVE_BIT12_11);

	/* TCL_EN */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_0,
		0x1 << RG_HDMITXPLL_TCL_EN_SHIFT, RG_HDMITXPLL_TCL_EN);

	if ((hdmi_phy->efuse == 0) || (hdmi_phy->phy_data->efuse_sw_mode == true)) {
		/* disable read calibration impedance from efuse */
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_1,
			0x1f << RG_INTR_IMP_RG_MODE_SHIFT,
			RG_INTR_IMP_RG_MODE);
	}

	return 0;
}

static int mtk_hdmi_txpll_set_hw(struct clk_hw *hw,
	unsigned char hdmirx_ref_ck_div, unsigned char ad_respll_ck_div,
	unsigned char prediv, unsigned char fbkdiv_high,
	unsigned long fbkdiv_low, unsigned char fbkdiv_hs3,
	unsigned char posdiv1, unsigned char posdiv2,
	unsigned char txprediv, unsigned char txposdiv,
	unsigned char digital_div)
{
	unsigned char txposdiv_value = 0;
	unsigned char div3_ctrl_value = 0;
	unsigned char posdiv_vallue = 0;
	unsigned char div_ctrl_value = 0;
	unsigned char reserve_3_2_value = 0;
	unsigned char prediv_value = 0;
	unsigned char reserve13_value = 0;

	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);

	mtk_hdmi_txpll_select_source(hw, hdmirx_ref_ck_div, ad_respll_ck_div);

	mtk_hdmi_txpll_performance_setting(hw);


	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_10,
		0x2 << RG_HDMITX21_BIAS_PE_BG_VREF_SEL_SHIFT,
		RG_HDMITX21_BIAS_PE_BG_VREF_SEL);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_10,
		0x0 << RG_HDMITX21_VREF_SEL_SHIFT, RG_HDMITX21_VREF_SEL);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_9,
		0x2 << RG_HDMITX21_SLDO_VREF_SEL_SHIFT,
		RG_HDMITX21_SLDO_VREF_SEL);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_10,
		0x0 << RG_HDMITX21_BIAS_PE_VREF_SELB_SHIFT,
		RG_HDMITX21_BIAS_PE_VREF_SELB);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_3,
		0x1 << RG_HDMITX21_SLDOLPF_EN_SHIFT, RG_HDMITX21_SLDOLPF_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
		0x1 << RG_HDMITXPLL_PWD_SHIFT, RG_HDMITXPLL_PWD);

	/* TXPOSDIV */
	if (txposdiv == 1)
		txposdiv_value = 0x0;
	else if (txposdiv == 2)
		txposdiv_value = 0x1;
	else if (txposdiv == 4)
		txposdiv_value = 0x2;
	else if (txposdiv == 8)
		txposdiv_value = 0x3;
	else {
		HDMI_PHY_LOG("TXPOSDIV error\n");
		return -EINVAL;
	}
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		txposdiv_value << RG_HDMITX21_TX_POSDIV_SHIFT,
		RG_HDMITX21_TX_POSDIV);

	/* /5, tmds_clk_frequency = tmds_data_frequency / 5 */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x1 << RG_HDMITX21_TX_POSDIV_EN_SHIFT,
		RG_HDMITX21_TX_POSDIV_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x0 << RG_HDMITX21_FRL_EN_SHIFT, RG_HDMITX21_FRL_EN);

	/* TXPREDIV */
	if (txprediv == 2) {
		div3_ctrl_value = 0x0;
		posdiv_vallue = 0x0;
	} else if (txprediv == 4) {
		div3_ctrl_value = 0x0;
		posdiv_vallue = 0x1;
	} else if (txprediv == 6) {
		div3_ctrl_value = 0x1;
		posdiv_vallue = 0x0;
	} else if (txprediv == 12) {
		div3_ctrl_value = 0x1;
		posdiv_vallue = 0x1;
	} else {
		HDMI_PHY_LOG("TXPREDIV error\n");
		return -EINVAL;
	}
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		div3_ctrl_value << RG_HDMITXPLL_POSDIV_DIV3_CTRL_SHIFT,
		RG_HDMITXPLL_POSDIV_DIV3_CTRL);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		posdiv_vallue << RG_HDMITXPLL_POSDIV_SHIFT,
		RG_HDMITXPLL_POSDIV);

	/* POSDIV1 */
	if (posdiv1 == 5)
		div_ctrl_value = 0x0;
	else if (posdiv1 == 10)
		div_ctrl_value = 0x1;
	else if (posdiv1 == (125 / 10))
		div_ctrl_value = 0x2;
	else if (posdiv1 == 15)
		div_ctrl_value = 0x3;
	else {
		HDMI_PHY_LOG("POSDIV1 error\n");
		HDMI_PHY_LOG("POSDIV1 = %d\n", posdiv1);
		div_ctrl_value = 0x1;
		//return -EINVAL;
	}
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		div_ctrl_value << RG_HDMITXPLL_DIV_CTRL_SHIFT,
		RG_HDMITXPLL_DIV_CTRL);

	/* DE add new setting */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_1,
		0x0 << RG_HDMITXPLL_RESERVE_BIT14_SHIFT,
		RG_HDMITXPLL_RESERVE_BIT14);

	/* POSDIV2 */
	if (posdiv2 == 1)
		reserve_3_2_value = 0x0;
	else if (posdiv2 == 2)
		reserve_3_2_value = 0x1;
	else if (posdiv2 == 4)
		reserve_3_2_value = 0x2;
	else if (posdiv2 == 6)
		reserve_3_2_value = 0x3;
	else {
		HDMI_PHY_LOG("POSDIV2 error\n");
		HDMI_PHY_LOG("POSDIV2 = %d\n", posdiv2);
		reserve_3_2_value = 0x1;
		//return -EINVAL;
	}
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_1,
		reserve_3_2_value << RG_HDMITXPLL_RESERVE_BIT3_2_SHIFT,
		RG_HDMITXPLL_RESERVE_BIT3_2);

	/* DE add new setting */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_1,
		0x2 << RG_HDMITXPLL_RESERVE_BIT1_0_SHIFT,
		RG_HDMITXPLL_RESERVE_BIT1_0);

	/* PREDIV */
	if (prediv == 1)
		prediv_value = 0x0;
	else if (prediv == 2)
		prediv_value = 0x1;
	else if (prediv == 4)
		prediv_value = 0x2;
	else {
		HDMI_PHY_LOG("PREDIV error\n");
		HDMI_PHY_LOG("PREDIV = %d\n", prediv);
		//return -EINVAL;
	}
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		prediv_value << RG_HDMITXPLL_PREDIV_SHIFT,
		RG_HDMITXPLL_PREDIV);

	/* FBKDIV_HS3 */
	if (fbkdiv_hs3 == 1)
		reserve13_value = 0x0;
	else if (fbkdiv_hs3 == 2)
		reserve13_value = 0x1;
	else {
		HDMI_PHY_LOG("FBKDIV_HS3 error\n");
		return -EINVAL;
	}
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_1,
		reserve13_value << RG_HDMITXPLL_RESERVE_BIT13_SHIFT,
		RG_HDMITXPLL_RESERVE_BIT13);

	/* FBDIV */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		fbkdiv_high << RG_HDMITXPLL_FBKDIV_high_SHIFT,
		RG_HDMITXPLL_FBKDIV_high);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_3,
		fbkdiv_low << RG_HDMITXPLL_FBKDIV_low_SHIFT,
		RG_HDMITXPLL_FBKDIV_low);

	/* Digital DIVIDER */
	if (hdmi_phy->clk_parent == PLLGP1_PLLGP_2H_PLL2)
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x1 << REG_PIXEL_CLOCK_SEL_SHIFT, REG_PIXEL_CLOCK_SEL);
	else if (hdmi_phy->clk_parent == XTAL26M)
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x0 << REG_PIXEL_CLOCK_SEL_SHIFT, REG_PIXEL_CLOCK_SEL);
	else
		HDMI_PHY_LOG("no need to set Digital Divider path\n");

	HDMI_PHY_LOG("Digital Divider = %d\n", digital_div);
	if (digital_div == 1)
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x0 << REG_HDMITX_PIXEL_CLOCK_SHIFT, REG_HDMITX_PIXEL_CLOCK);
	else {
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			0x1 << REG_HDMITX_PIXEL_CLOCK_SHIFT, REG_HDMITX_PIXEL_CLOCK);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_3,
			(digital_div - 1) << REG_HDMITXPLL_DIV_SHIFT, REG_HDMITXPLL_DIV);
	}

	return 0;
}

#define PCW_DECIMAL_WIDTH 24

static int mtk_hdmi_txpll_calculate_params(struct clk_hw *hw,
	unsigned long rate, unsigned long parent_rate)
{
	int ret;
	unsigned long long tmds_clk = 0, pixel_clk = 0;
	//ref clock from hdmi Rx
	unsigned long long hdmirx_ref_ck = 0;
	unsigned char hdmirx_ref_ck_div = 0;
	//ref clock from tvd pll
	unsigned long long ad_respll_ck = 0;
	unsigned char ad_respll_ck_div = 0;
	//txpll input source frequency
	unsigned long long da_hdmitx21_ref_ck = 0;
	unsigned long long ns_hdmitxpll_ck = 0; //ICO output clk
	//source clk for Display digital
	unsigned long long ad_hdmitxpll_pixel_ck = 0;
	unsigned char digital_div = 0;
	unsigned long long pcw = 0; //FBDIV
	unsigned char txprediv = 0, txposdiv = 0;
	unsigned char fbkdiv_high = 0;
	unsigned long fbkdiv_low = 0;
	unsigned char posdiv1 = 0, posdiv2 = 0;
	unsigned char prediv = 1; //prediv is always 1
	unsigned char fbkdiv_hs3 = 1; //fbkdiv_hs3 is always 1
	int i = 0;
	unsigned char TXPREDIV[4] = {2, 4, 6, 12};

	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);
	struct mtk_hdmi *hdmi = to_mtk_hdmi(hdmi_phy);

	pixel_clk = ((unsigned long long)hdmi->mode.clock) * 1000;//in HZ

/* TMDS clk frequency */
	if ((hdmi->csp == HDMI_COLORSPACE_RGB) || (hdmi->csp == HDMI_COLORSPACE_YUV444)) {
		if (hdmi->color_depth == HDMI_8_BIT)
			tmds_clk = pixel_clk;
		else if (hdmi->color_depth == HDMI_10_BIT)
			tmds_clk = pixel_clk * 5 / 4; // *1.25
		else if (hdmi->color_depth == HDMI_12_BIT)
			tmds_clk = pixel_clk * 3 / 2; // *1.5
		else if (hdmi->color_depth == HDMI_16_BIT)
			tmds_clk = pixel_clk * 2; // *2
		else {
			HDMI_PHY_LOG("%s, unknown color depth\n", __func__);
			return -EINVAL;
		}
	} else if (hdmi->csp == HDMI_COLORSPACE_YUV422)
		tmds_clk = pixel_clk; //YUV420,12bit,TMDS_CLK == 8bit RGB444 TMDS_CLK
	else if (hdmi->csp == HDMI_COLORSPACE_YUV420) {
		if (hdmi->color_depth == HDMI_8_BIT)
			tmds_clk = pixel_clk / 2;
		else if (hdmi->color_depth == HDMI_10_BIT)
			tmds_clk = pixel_clk / 2 * 5 / 4; // *1.25
		else if (hdmi->color_depth == HDMI_12_BIT)
			tmds_clk = pixel_clk / 2 * 3 / 2; // *1.5
		else if (hdmi->color_depth == HDMI_16_BIT)
			tmds_clk = pixel_clk / 2 * 2; // *2
		else {
			HDMI_PHY_LOG("%s, unknown color depth\n", __func__);
			return -EINVAL;
		}
	} else
		HDMI_PHY_LOG("unknown color space\n");

	if ((tmds_clk < hdmi_phy->min_tmds_clock) &&
		(tmds_clk > hdmi_phy->max_tmds_clock)) {
		HDMI_PHY_LOG("%s, TMDS ck out of range\n", __func__);
		return -EINVAL;
	}

/* DA_HDMITX21_REF_CK:
 * there 3 input clk source :
 * 1st: tvdpll(PLLGP1):
 *     AD_RESPLL_CK: 648M(27M times),
 *                   594M(>HD and integer framerate),
 *                   593.4M(>HD and fractional framerate)
 * 2st: hdmirx clk: 27M,74M,148M,297M,594M,etc, depends on what clk Rx receive
 * 3st: xTal26M crystal clk: always 26M
 */
	if (hdmi_phy->clk_parent == XTAL26M)
		da_hdmitx21_ref_ck = 26000000UL; //in HZ
	else if (hdmi_phy->clk_parent == HDMIRX) {
		hdmirx_ref_ck = hdmi_phy->rx_clk_rate;
		HDMI_PHY_LOG("Rx ref clk, hdmirx_ref_ck=%d\n", hdmirx_ref_ck);
		for (i = 32; i >= 1; i--)
			if (((hdmirx_ref_ck / i / prediv) >= 20000000UL) &&
				((hdmirx_ref_ck / i / prediv) <= 30000000UL))
				break;
		/* constraint: txpll input clk range: 20Mhz~30Mhz */
		if (i == 1 && (((hdmirx_ref_ck / i / prediv) < 20000000UL) ||
			((hdmirx_ref_ck / i / prediv) > 30000000UL))) {
			HDMI_PHY_LOG("%s, Rx ck err\n", __func__);
			return -EINVAL;
		}
		hdmirx_ref_ck_div = i;
		da_hdmitx21_ref_ck = hdmirx_ref_ck / i / prediv;
	} else if (hdmi_phy->clk_parent == PLLGP1_PLLGP_2H_PLL2) {
	//can.zeng todo verify,
	//case 1: ad_respll_ck = parent_rate,
	//case 2: stage treatment according to current pixel clk

		if ((pixel_clk % 27000000) == 0) {
			ad_respll_ck = 648000000;
			ad_respll_ck_div = 24;
		} else if (pixel_clk % 74250000 == 0) {
			ad_respll_ck = 594000000;
			ad_respll_ck_div = 22;
		} else if (pixel_clk % 74135000 == 0) {
			ad_respll_ck = 593400000;
			ad_respll_ck_div = 22;
		} else {
			//can.zeng todo in-regular hdmi resolution clk
			return -EINVAL;
		}

		da_hdmitx21_ref_ck = ad_respll_ck / ad_respll_ck_div;
	} else {
		HDMI_PHY_LOG("%s, unknown clk parent\n", __func__);
		return -EINVAL;
	}

/*  TXPOSDIV stage treatment:
 *	0M  <  TMDS clk  < 54M		  /8
 *	54M <= TMDS clk  < 148.35M    /4
 *	148.35M <=TMDS clk < 296.7M   /2
 *	296.7 <=TMDS clk <= 594M	  /1
 */
	if (tmds_clk < 54000000UL)
		txposdiv = 8;
	else if ((tmds_clk >= 54000000UL) && (tmds_clk < 148350000UL))
		txposdiv = 4;
	else if ((tmds_clk >= 148350000UL) && (tmds_clk < 296700000UL))
		txposdiv = 2;
	else if ((tmds_clk >= 296700000UL) && (tmds_clk <= 594000000UL))
		txposdiv = 1;
	else {
		HDMI_PHY_LOG("%s, tmds clk out of range\n", __func__);
		return -EINVAL;
	}

/* calculate TXPREDIV: can be 2, 4, 6, 12
 * ICO clk = 5*TMDS_CLK*TXPOSDIV*TXPREDIV
 * ICO clk constraint: 5G =< ICO clk <= 12G
 */
	for (i = 0; i < ARRAY_SIZE(TXPREDIV); i++) {
		ns_hdmitxpll_ck = 5 * tmds_clk * txposdiv * TXPREDIV[i];
		if ((ns_hdmitxpll_ck >= 5000000000UL) &&
			(ns_hdmitxpll_ck <= 12000000000UL))
			break;
	}
	if ((i == (ARRAY_SIZE(TXPREDIV) - 1)) &&
		((ns_hdmitxpll_ck < 5000000000UL) ||
		(ns_hdmitxpll_ck > 12000000000UL))) {
		HDMI_PHY_LOG("%s, TXPREDIV, >12G or <5G\n", __func__);
		return -EINVAL;
	}
	if (i < ARRAY_SIZE(TXPREDIV))
		txprediv = TXPREDIV[i];

/* PCW calculation: FBKDIV
 * formula: pcw=(frequency_out*2^pcw_bit) / frequency_in / FBKDIV_HS3;
 * RG_HDMITXPLL_FBKDIV[32:0]:
 * [32,24] 9bit integer, [23,0]:24bit fraction
 */
	pcw = ns_hdmitxpll_ck;
	/*
	 *for (i = 0; i < PCW_DECIMAL_WIDTH; i++)
	 *	pcw *=2;
	 */
	pcw = pcw << PCW_DECIMAL_WIDTH;
	pcw = pcw / da_hdmitx21_ref_ck;
	pcw = pcw / fbkdiv_hs3;

	if ((pcw / (1ULL << 32)) > 1) {
		HDMI_PHY_LOG("%s, PCW out range\n", __func__);
		return -EINVAL;
	} else if ((pcw / (1ULL << 32)) == 1) {
		fbkdiv_high = 1;
		fbkdiv_low = pcw % (1ULL << 32);
	} else {
		fbkdiv_high = 0;
		fbkdiv_low = pcw;
	}
	/* can.zeng todo verify algorithm
	 * if clk input source is HDMI RX, the PCW shall be integer
	 * because the Rx ref clk == pixel clk, or == 1/2 pixel clk,
	 * so the integer pcw to make the txpll output more accurate
	 */
	if (hdmi_phy->clk_parent == HDMIRX) {
		if ((fbkdiv_low & BIT(23)) != 0)
			fbkdiv_low = (fbkdiv_low & (~(GENMASK(23, 0)))) + BIT(24);
		else
			fbkdiv_low = fbkdiv_low & (~(GENMASK(23, 0)));
	}

/* posdiv1:
 * posdiv1 stage treatment according to color_depth:
 * 24bit -> posdiv1 /10, 30bit -> posdiv1 /12.5,
 * 36bit -> posdiv1 /15, 48bit -> posdiv1 /10
 */

//can.zeng todo verify, make posdiv2 always 1 temporarily,
//and pixel clock be generated by Digital Divider, max /32
/* posdiv2:
 */
	if ((hdmi->csp == HDMI_COLORSPACE_RGB) || (hdmi->csp == HDMI_COLORSPACE_YUV444)
	|| (hdmi->csp == HDMI_COLORSPACE_YUV420)) {
		if ((hdmi->color_depth == HDMI_8_BIT) ||
			(hdmi->color_depth == HDMI_16_BIT)) {
			posdiv1 = 10; // div 10
			posdiv2 = 1;
			ad_hdmitxpll_pixel_ck = (ns_hdmitxpll_ck / 10) / 1;
		} else if (hdmi->color_depth == HDMI_10_BIT) {
			posdiv1 = 125 / 10; // div 12.5
			posdiv2 = 1;
			ad_hdmitxpll_pixel_ck = ((ns_hdmitxpll_ck * 10) / 125) / 1;
		} else if (hdmi->color_depth == HDMI_12_BIT) {
			posdiv1 = 15; // div 15
			posdiv2 = 1;
			ad_hdmitxpll_pixel_ck = (ns_hdmitxpll_ck / 15) / 1;
		} else {
			HDMI_PHY_LOG("%s, unknown color depth\n", __func__);
			return -EINVAL;
		}
	} else if (hdmi->csp == HDMI_COLORSPACE_YUV422) {
		//yuv422 is the same as yuv444 8bit
		posdiv1 = 10; // div 10
		posdiv2 = 1;
		ad_hdmitxpll_pixel_ck = (ns_hdmitxpll_ck / 10) / 1;
	} else
		HDMI_PHY_LOG("unknown color space\n");
	HDMI_PHY_LOG("ad_hdmitxpll_pixel_clk =%ull\n", ad_hdmitxpll_pixel_ck);

/* Digital clk divider, max /32 */
	digital_div = ad_hdmitxpll_pixel_ck / pixel_clk;
	if (!((digital_div <= 32) && (digital_div >= 1))) {
		HDMI_PHY_LOG("%s, Invalid Digital Divider\n", __func__);
		return -EINVAL;
	}
	HDMI_PHY_LOG("digital divider = %d\n", digital_div);

	ret = mtk_hdmi_txpll_set_hw(hw, hdmirx_ref_ck_div, ad_respll_ck_div,
		prediv, fbkdiv_high, fbkdiv_low, fbkdiv_hs3,
		posdiv1, posdiv2, txprediv, txposdiv, digital_div);
	if (ret != 0) {
		HDMI_PHY_LOG("set HW parameters error!\n");
		return -EINVAL;
	}

	pr_info("[HDMI][TXPLL] Parameter list:\n");
	pr_info("hdmirx_ref_ck_div = %d\n", hdmirx_ref_ck_div);
	pr_info("ad_respll_ck_div = %d\n", ad_respll_ck_div);
	pr_info("prediv = %d\n", prediv);
	pr_info("pcw = %ld\n", pcw);
	pr_info("pcw = %lx\n", pcw);
	pr_info("fbkdiv_high = %x\n", fbkdiv_high);
	pr_info("fbkdiv_low = %lx\n", fbkdiv_low);
	pr_info("fbkdiv_hs3 = %d\n", fbkdiv_hs3);
	pr_info("posdiv1 = %d\n", posdiv1);
	pr_info("posdiv2 = %d\n", posdiv2);
	pr_info("digital_div = %d\n", digital_div);
	pr_info("txprediv = %d\n", txprediv);
	pr_info("txposdiv = %d\n", txposdiv);

	pr_info("hdmi->color_depth=%d\n", hdmi->color_depth);
	pr_info("hdmi->color_space=%d\n", hdmi->csp);
	return 0;
}

static int mtk_hdmi_txpll_drv_setting(struct clk_hw *hw)
{
	unsigned char data_channel_bias, clk_channel_bias;
	unsigned char impedance, impedance_en, calibration;
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);
	struct mtk_hdmi *hdmi = to_mtk_hdmi(hdmi_phy);
	unsigned long tmds_clk;
	unsigned long pixel_clk = hdmi->mode.clock * 1000;//in HZ

	/* TMDS clk frequency */
	if ((hdmi->csp == HDMI_COLORSPACE_RGB) || (hdmi->csp == HDMI_COLORSPACE_YUV444)) {
		if (hdmi->color_depth == HDMI_8_BIT)
			tmds_clk = pixel_clk;
		else if (hdmi->color_depth == HDMI_10_BIT)
			tmds_clk = pixel_clk * 5 / 4; // *1.25
		else if (hdmi->color_depth == HDMI_12_BIT)
			tmds_clk = pixel_clk * 3 / 2; // *1.5
		else if (hdmi->color_depth == HDMI_16_BIT)
			tmds_clk = pixel_clk * 2; // *2
		else {
			HDMI_PHY_LOG("%s, unknown color depth\n", __func__);
			return -EINVAL;
		}
	} else if (hdmi->csp == HDMI_COLORSPACE_YUV422)
		tmds_clk = pixel_clk; //YUV420,12bit,TMDS_CLK == 8bit RGB444 TMDS_CLK
	else if (hdmi->csp == HDMI_COLORSPACE_YUV420) {
		if (hdmi->color_depth == HDMI_8_BIT)
			tmds_clk = pixel_clk / 2;
		else if (hdmi->color_depth == HDMI_10_BIT)
			tmds_clk = pixel_clk / 2 * 5 / 4; // *1.25
		else if (hdmi->color_depth == HDMI_12_BIT)
			tmds_clk = pixel_clk / 2 * 3 / 2; // *1.5
		else if (hdmi->color_depth == HDMI_16_BIT)
			tmds_clk = pixel_clk / 2 * 2; // *2
		else {
			HDMI_PHY_LOG("%s, unknown color depth\n", __func__);
			return -EINVAL;
		}
	} else {
		HDMI_PHY_LOG("unknown color space\n");
		return -EINVAL;
	}

/* bias & impedance setting:
 * 3G < data rate <= 6G: enable impedance 100ohm,
 *      data channel bias 24mA, clock channel bias 20mA
 * pixel clk >= HD,  74.175MHZ <= pixel clk <= 300MHZ:
 *      enalbe impedance 100ohm
 *      data channel 20mA, clock channel 16mA
 * 27M =< pixel clk < 74.175: disable impedance
 *      data channel & clock channel bias 10mA
 */

	/* 3G < data rate <= 6G, 300M < tmds rate <= 594M */
	if ((tmds_clk > 300000000UL) && (tmds_clk <= 594000000UL)) {
		data_channel_bias = 0x3c; //24mA
		clk_channel_bias = 0x34;  //20mA
		impedance_en = 0xf;
		impedance = 0x36; //100ohm
	} else if ((tmds_clk >= 74175000UL) && (tmds_clk <= 300000000UL)) {
		data_channel_bias = 0x34; //20mA
		clk_channel_bias = 0x2c;  //16mA
		impedance_en = 0xf;
		impedance = 0x36; //100ohm
	} else if ((tmds_clk >= 27000000UL) && (tmds_clk < 74175000UL)) {
		data_channel_bias = 0x14; //10mA
		clk_channel_bias = 0x14; //10mA
		impedance_en = 0x0;
		impedance = 0x0;
	} else {
		HDMI_PHY_LOG("%s, CK out of range\n", __func__);
		return -EINVAL;
	}

	/* bias */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_1,
		data_channel_bias << RG_HDMITX21_DRV_IBIAS_D0_SHIFT,
		RG_HDMITX21_DRV_IBIAS_D0);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_1,
		data_channel_bias << RG_HDMITX21_DRV_IBIAS_D1_SHIFT,
		RG_HDMITX21_DRV_IBIAS_D1);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_1,
		data_channel_bias << RG_HDMITX21_DRV_IBIAS_D2_SHIFT,
		RG_HDMITX21_DRV_IBIAS_D2);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_0,
		clk_channel_bias << RG_HDMITX21_DRV_IBIAS_CLK_SHIFT,
		RG_HDMITX21_DRV_IBIAS_CLK);

	/* impedance */
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_0,
		impedance_en << RG_HDMITX21_DRV_IMP_EN_SHIFT,
		RG_HDMITX21_DRV_IMP_EN);

	if (hdmi_phy->efuse == 0) {
		HDMI_PHY_LOG("use default value\n");
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
			impedance << RG_HDMITX21_DRV_IMP_D0_EN1_SHIFT,
			RG_HDMITX21_DRV_IMP_D0_EN1);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
			impedance << RG_HDMITX21_DRV_IMP_D1_EN1_SHIFT,
			RG_HDMITX21_DRV_IMP_D1_EN1);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
			impedance << RG_HDMITX21_DRV_IMP_D2_EN1_SHIFT,
			RG_HDMITX21_DRV_IMP_D2_EN1);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
			impedance << RG_HDMITX21_DRV_IMP_CLK_EN1_SHIFT,
			RG_HDMITX21_DRV_IMP_CLK_EN1);

		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
			0x11 << RG_HDMITX21_INTR_CAL_SHIFT,
			RG_HDMITX21_INTR_CAL);
	} else if (hdmi_phy->phy_data->efuse_sw_mode == true) {
		HDMI_PHY_LOG("use efuse value with sw mode\n");
		impedance = hdmi_phy->efuse & 0x3f;
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
			impedance << RG_HDMITX21_DRV_IMP_D0_EN1_SHIFT,
			RG_HDMITX21_DRV_IMP_D0_EN1);

		impedance = (hdmi_phy->efuse >> 6) & 0x3f;
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
			impedance << RG_HDMITX21_DRV_IMP_D1_EN1_SHIFT,
			RG_HDMITX21_DRV_IMP_D1_EN1);

		impedance = (hdmi_phy->efuse >> 12) & 0x3f;
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
			impedance << RG_HDMITX21_DRV_IMP_D2_EN1_SHIFT,
			RG_HDMITX21_DRV_IMP_D2_EN1);

		impedance = (hdmi_phy->efuse >> 18) & 0x3f;
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
			impedance << RG_HDMITX21_DRV_IMP_CLK_EN1_SHIFT,
			RG_HDMITX21_DRV_IMP_CLK_EN1);

		calibration = (hdmi_phy->efuse >> 24) & 0x1f;
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
			calibration << RG_HDMITX21_INTR_CAL_SHIFT,
			RG_HDMITX21_INTR_CAL);
	}

	return 0;
}

static int mtk_hdmi_txpll_prepare(struct clk_hw *hw)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);

	dev_dbg(hdmi_phy->dev, "%s\n", __func__);

	HDMI_PHY_LOG();

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x1 << RG_HDMITX21_TX_POSDIV_EN_SHIFT,
		RG_HDMITX21_TX_POSDIV_EN);

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_0,
		0xf << RG_HDMITX21_SER_EN_SHIFT, RG_HDMITX21_SER_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x1 << RG_HDMITX21_D0_DRV_OP_EN_SHIFT,
		RG_HDMITX21_D0_DRV_OP_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x1 << RG_HDMITX21_D1_DRV_OP_EN_SHIFT,
		RG_HDMITX21_D1_DRV_OP_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x1 << RG_HDMITX21_D2_DRV_OP_EN_SHIFT,
		RG_HDMITX21_D2_DRV_OP_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x1 << RG_HDMITX21_CK_DRV_OP_EN_SHIFT,
		RG_HDMITX21_CK_DRV_OP_EN);

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x0 << RG_HDMITX21_FRL_D0_EN_SHIFT, RG_HDMITX21_FRL_D0_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x0 << RG_HDMITX21_FRL_D1_EN_SHIFT, RG_HDMITX21_FRL_D1_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x0 << RG_HDMITX21_FRL_D2_EN_SHIFT, RG_HDMITX21_FRL_D2_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x0 << RG_HDMITX21_FRL_CK_EN_SHIFT, RG_HDMITX21_FRL_CK_EN);

	mtk_hdmi_txpll_drv_setting(hw);

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_10,
		0x0 << RG_HDMITX21_BG_PWD_SHIFT, RG_HDMITX21_BG_PWD);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x1 << RG_HDMITX21_BIAS_EN_SHIFT, RG_HDMITX21_BIAS_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_3,
		0x1 << RG_HDMITX21_CKLDO_EN_SHIFT, RG_HDMITX21_CKLDO_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_3,
		0xf << RG_HDMITX21_SLDO_EN_SHIFT, RG_HDMITX21_SLDO_EN);

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		0x1 << DA_HDMITXPLL_PWR_ON_SHIFT, DA_HDMITXPLL_PWR_ON);
	udelay(5);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		0x0 << DA_HDMITXPLL_ISO_EN_SHIFT, DA_HDMITXPLL_ISO_EN);
	udelay(5);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
		0x0 << RG_HDMITXPLL_PWD_SHIFT, RG_HDMITXPLL_PWD);
	udelay(30);

	return 0;
}

static void mtk_hdmi_txpll_unprepare(struct clk_hw *hw)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);

	dev_dbg(hdmi_phy->dev, "%s\n", __func__);
	HDMI_PHY_LOG();

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_10,
		0x1 << RG_HDMITX21_BG_PWD_SHIFT, RG_HDMITX21_BG_PWD);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
		0x0 << RG_HDMITX21_BIAS_EN_SHIFT, RG_HDMITX21_BIAS_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_3,
		0x0 << RG_HDMITX21_CKLDO_EN_SHIFT, RG_HDMITX21_CKLDO_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_3,
		0x0 << RG_HDMITX21_SLDO_EN_SHIFT, RG_HDMITX21_SLDO_EN);

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_2,
		0x1 << RG_HDMITXPLL_PWD_SHIFT, RG_HDMITXPLL_PWD);
	udelay(10);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		0x1 << DA_HDMITXPLL_ISO_EN_SHIFT, DA_HDMITXPLL_ISO_EN);
	udelay(10);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_PLL_CFG_4,
		0x0 << DA_HDMITXPLL_PWR_ON_SHIFT, DA_HDMITXPLL_PWR_ON);

}


//can.zeng todo verify the set_parent function.need to figure out the call trace
/*static int mtk_hdmi_txpll_set_parent(struct clk_hw *hw, u8 index)
 *{
 *	return 0;
 *}
 */

//can.zeng todo verify, how to detemine the return index
/*static u8 mtk_hdmi_txpll_get_parent(struct clk_hw *hw)
 *{
 *	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);
 *
 *	return hdmi_phy->clk_parent;
 *}
 */

static int mtk_hdmi_txpll_set_rate(struct clk_hw *hw, unsigned long rate,
				 unsigned long parent_rate)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);
	int ret;

	HDMI_PHY_LOG();

	dev_dbg(hdmi_phy->dev, "%s: %lu Hz, parent: %lu Hz\n", __func__,
		rate, parent_rate);

	ret = mtk_hdmi_txpll_calculate_params(hw, rate, parent_rate);
	if (ret != 0) {
		HDMI_PHY_LOG("error!!\n");
		return -EINVAL;
	}
	return 0;
}

static long mtk_hdmi_txpll_round_rate(struct clk_hw *hw, unsigned long rate,
				    unsigned long *parent_rate)
{
/* workaround patch: when clk_set_rate(clk, req_rate) is called, if the req_rate
 * is the same as last, CCF core won't call ops.set_rate because ops.round_rate
 * return the new req_rate's round == last clk_core.rate. Always return 1 in
 * this function to make context that new req_rate's round rate never equal last
 * clk_core.rate, so the ops.set_rate can be called in every clk_set_rate.
 */

/*
 *	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);
 *
 *	hdmi_phy->txpll_rate = rate;
 *	return rate;
 */
	return 1;
}

static unsigned long mtk_hdmi_txpll_recalc_rate(struct clk_hw *hw,
					      unsigned long parent_rate)
{
	struct mtk_hdmi_phy *hdmi_phy = to_mtk_hdmi_phy(hw);

	return hdmi_phy->txpll_rate;
}

static const struct clk_ops mtk_hdmi_txpll_ops = {
	.prepare = mtk_hdmi_txpll_prepare,
	.unprepare = mtk_hdmi_txpll_unprepare,
/*	.set_parent = mtk_hdmi_txpll_set_parent,
 *	.get_parent = mtk_hdmi_txpll_get_parent,
 */
	.set_rate = mtk_hdmi_txpll_set_rate,
	.round_rate = mtk_hdmi_txpll_round_rate,
	.recalc_rate = mtk_hdmi_txpll_recalc_rate,
};

void vTxSignalOnOff(struct mtk_hdmi_phy *hdmi_phy, bool OnOff)
{
	if (OnOff) {
		HDMI_PHY_LOG("HDMI Tx TMDS ON\n");

		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_0,
			RG_HDMITX21_DRV_EN, RG_HDMITX21_DRV_EN);
	} else {
		HDMI_PHY_LOG("HDMI Tx TMDS Off\n");

		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_0,
			0x0 << RG_HDMITX21_DRV_EN_SHIFT, RG_HDMITX21_DRV_EN);
/*
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_TX_POSDIV_EN_SHIFT,
 *			RG_HDMITX21_TX_POSDIV_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_FRL_EN_SHIFT,
 *			RG_HDMITX21_FRL_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_0,
 *			0x0 << RG_HDMITX21_SER_EN_SHIFT,
 *			RG_HDMITX21_SER_EN);
 *
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_D0_DRV_OP_EN_SHIFT,
 *			RG_HDMITX21_D0_DRV_OP_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_D1_DRV_OP_EN_SHIFT,
 *			RG_HDMITX21_D1_DRV_OP_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_D2_DRV_OP_EN_SHIFT,
 *			RG_HDMITX21_D2_DRV_OP_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_CK_DRV_OP_EN_SHIFT,
 *			RG_HDMITX21_CK_DRV_OP_EN);
 *
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_FRL_D0_EN_SHIFT,
 *			RG_HDMITX21_FRL_D0_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_FRL_D1_EN_SHIFT,
 *			RG_HDMITX21_FRL_D1_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_FRL_D2_EN_SHIFT,
 *			RG_HDMITX21_FRL_D2_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_6,
 *			0x0 << RG_HDMITX21_FRL_CK_EN_SHIFT,
 *			RG_HDMITX21_FRL_CK_EN);
 *
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_1,
 *			0x0 << RG_HDMITX21_DRV_IBIAS_D0_SHIFT,
 *			RG_HDMITX21_DRV_IBIAS_D0);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_1,
 *			0x0 << RG_HDMITX21_DRV_IBIAS_D1_SHIFT,
 *			RG_HDMITX21_DRV_IBIAS_D1);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_1,
 *			0x0 << RG_HDMITX21_DRV_IBIAS_D2_SHIFT,
 *			RG_HDMITX21_DRV_IBIAS_D2);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_0,
 *			0x0 << RG_HDMITX21_DRV_IBIAS_CLK_SHIFT,
 *			RG_HDMITX21_DRV_IBIAS_CLK);
 *
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_0,
 *			0x0 << RG_HDMITX21_DRV_IMP_EN_SHIFT,
 *			RG_HDMITX21_DRV_IMP_EN);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
 *			0x0 << RG_HDMITX21_DRV_IMP_D0_EN1_SHIFT,
 *			RG_HDMITX21_DRV_IMP_D0_EN1);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
 *			0x0 << RG_HDMITX21_DRV_IMP_D1_EN1_SHIFT,
 *			RG_HDMITX21_DRV_IMP_D1_EN1);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
 *			0x0 << RG_HDMITX21_DRV_IMP_D2_EN1_SHIFT,
 *			RG_HDMITX21_DRV_IMP_D2_EN1);
 *		mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_2,
 *			0x0 << RG_HDMITX21_DRV_IMP_CLK_EN1_SHIFT,
 *			RG_HDMITX21_DRV_IMP_CLK_EN1);
 */
		HDMI_PHY_LOG("vTxSignalOff = %lums\n", jiffies);
	}

}

static void mtk_hdmi_phy_enable_tmds(struct mtk_hdmi_phy *hdmi_phy)
{
	HDMI_PHY_FUNC();

	vTxSignalOnOff(hdmi_phy, true);
	usleep_range(100, 150);
}

static void mtk_hdmi_phy_disable_tmds(struct mtk_hdmi_phy *hdmi_phy)
{
	HDMI_PHY_FUNC();

	vTxSignalOnOff(hdmi_phy, false);
}

static int mtk_hdmi_phy_power_on(struct phy *phy)
{
	struct mtk_hdmi_phy *hdmi_phy = phy_get_drvdata(phy);
	int ret;

	HDMI_PHY_FUNC();

	if (__clk_is_enabled(hdmi_phy->txpll) == false) {
		ret = clk_prepare_enable(hdmi_phy->txpll);
		if (ret < 0)
			return ret;
	}
	udelay(10);
	mtk_hdmi_phy_enable_tmds(hdmi_phy);

	return 0;
}

static int mtk_hdmi_phy_power_off(struct phy *phy)
{
	struct mtk_hdmi_phy *hdmi_phy = phy_get_drvdata(phy);

	HDMI_PHY_FUNC();

	mtk_hdmi_phy_disable_tmds(hdmi_phy);
	udelay(10);
	if (__clk_is_enabled(hdmi_phy->txpll) == true)
		clk_disable_unprepare(hdmi_phy->txpll);

	return 0;
}

static const struct phy_ops mtk_hdmi_phy_ops = {
	.power_on = mtk_hdmi_phy_power_on,
	.power_off = mtk_hdmi_phy_power_off,
	.owner = THIS_MODULE,
};

static int mtk_hdmi_phy_get_all_clk(
	struct mtk_hdmi_phy *hdmi_phy, struct device *dev)
{
	int i, ret;
	/*	int i;
	 *	struct clk *txpll_clk_parents[TXPLL_CLK_PARENT_COUNT];
	 *	const char *txpll_parent_names[TXPLL_CLK_PARENT_COUNT];
	 *	struct clk_init_data clk_init = {
	 *		.ops = &mtk_hdmi_txpll_ops,
	 *		.num_parents = TXPLL_CLK_PARENT_COUNT,
	 *		.parent_names = (const char * const *)txpll_parent_names,
	 *		.flags = CLK_SET_RATE_PARENT | CLK_SET_RATE_GATE,
	 *	};
	 */
	const char *ref_clk_name;
	struct clk_init_data clk_init = {
		.ops = &mtk_hdmi_txpll_ops,
		.num_parents = 1,
		.parent_names = (const char * const *)&ref_clk_name,
		.flags = CLK_SET_RATE_PARENT | CLK_SET_RATE_GATE,
	};

	for (i = 0; i < ARRAY_SIZE(mtk_hdmi_phy_clk_names); i++) {
		hdmi_phy->clk[i] = of_clk_get_by_name(dev->of_node,
			mtk_hdmi_phy_clk_names[i]);

		if (IS_ERR(hdmi_phy->clk[i]))
			return PTR_ERR(hdmi_phy->clk[i]);
	}

	/* specify clock parent 26M for bring up */
	hdmi_phy->clk_parent = XTAL26M;

	ref_clk_name = __clk_get_name(hdmi_phy->clk[MTK_HDMI_PHY_XTAL_SEL]);

/*acquire txpll 3 clk parent, can.zeng todo verify
 *	for (i = PLLGP1_PLLGP_2H_PLL2; i < TXPLL_CLK_PARENT_COUNT; i++) {
 *		txpll_clk_parents[i] =
 *			devm_clk_get(dev, txpll_parent_dts_names[i]);
 *		if (IS_ERR(txpll_clk_parents[i])) {
 *			ret = PTR_ERR(txpll_clk_parents[i]);
 *			HDMI_PHY_LOG("Fail to get the %dst ref clk\n", i);
 *
 *			return ret;
 *		}
 *		txpll_parent_names[i] = __clk_get_name(txpll_clk_parents[i]);
 *	}
 *acquire txpll 3 clk parent, can.zeng todo verify
 */

	ret = of_property_read_string(dev->of_node, "clock-output-names",
					  &clk_init.name);
	if (ret < 0) {
		HDMI_PHY_LOG("Failed to read clock-output-names: %d\n", ret);
		return ret;
	}

	hdmi_phy->txpll_hw.init = &clk_init;
	hdmi_phy->txpll = devm_clk_register(dev, &hdmi_phy->txpll_hw);
	if (IS_ERR(hdmi_phy->txpll)) {
		ret = PTR_ERR(hdmi_phy->txpll);
		HDMI_PHY_LOG("Failed to register PLL: %d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_hdmi_detect_clock_channel_only(struct mtk_hdmi_phy *hdmi_phy)
{
	HDMI_PHY_FUNC();
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_1_CFG_5,
		0xfc << RG_HDMITX21_DETECT_CLOCK_CHANNEL_SHIFT,
		RG_HDMITX21_DETECT_CLOCK_CHANNEL);

	return 0;
}

void mtk_hdmi_power_control(struct mtk_hdmi_phy *hdmi_phy, bool enable)
{
	HDMI_PHY_LOG("enable: %d\n", enable);
	if (enable)
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_1,
			0x1 << RG_HDMITX_PWR5V_O_SHIFT, RG_HDMITX_PWR5V_O);
	else
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CTL_1,
			0x0 << RG_HDMITX_PWR5V_O_SHIFT, RG_HDMITX_PWR5V_O);
}

static const struct mtk_hdmi_phy_data mt8195_hdmi_phy_data = {
	.detect_clock_channel_only = false,
	.power_control_available = false,
	.efuse_sw_mode = false,
};

static const struct mtk_hdmi_phy_data mt8188_hdmi_phy_data = {
	.detect_clock_channel_only = true,
	.power_control_available = true,
	.efuse_sw_mode = true,
};

static const struct of_device_id mtk_hdmi_phy_match[] = {
	{ .compatible = "mediatek,mt8195-hdmi-phy", .data = &mt8195_hdmi_phy_data},
	{ .compatible = "mediatek,mt8188-hdmi-phy", .data = &mt8188_hdmi_phy_data},
	{}
};

struct mtk_hdmi_phy *global_hdmi_phy;

static int mtk_hdmi_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_hdmi_phy *hdmi_phy;
	struct resource *mem;
	struct phy *phy;
	struct phy_provider *phy_provider;
	int ret;
	struct nvmem_cell *cell;
	unsigned int *efuse_buffer;
	size_t len;
	const struct of_device_id *of_id;

	HDMI_PHY_LOG("probe start\n");

	hdmi_phy = devm_kzalloc(dev, sizeof(*hdmi_phy), GFP_KERNEL);
	if (!hdmi_phy)
		return -ENOMEM;

	global_hdmi_phy = hdmi_phy;

	ret = mtk_hdmi_phy_get_all_clk(hdmi_phy, dev);
	if (ret) {
		HDMI_PHY_LOG("Failed to get clocks: %d\n", ret);
		return ret;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hdmi_phy->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(hdmi_phy->regs)) {
		ret = PTR_ERR(hdmi_phy->regs);
		HDMI_PHY_LOG("Failed to get memory resource: %d\n", ret);
		return ret;
	}

	/* min & max tmds frequency range:
	 * HDMI protocal specify tmds clock must above 25M,
	 * mt8195 tmds top limit is 594M
	 */
	hdmi_phy->min_tmds_clock = 25000000; //25M
	hdmi_phy->max_tmds_clock = 594000000; //594M

	phy = devm_phy_create(dev, NULL, &mtk_hdmi_phy_ops);
	if (IS_ERR(phy)) {
		HDMI_PHY_LOG("Failed to create HDMI PHY\n");
		return PTR_ERR(phy);
	}
	phy_set_drvdata(phy, hdmi_phy);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR(phy_provider);

	hdmi_phy->dev = dev;

	/* efuse */
	cell = nvmem_cell_get(dev, "hdmitx_phy_efuse");
	if (IS_ERR(cell)) {
		HDMI_PHY_LOG("failed to read efuse\n");
	} else {
		efuse_buffer = (unsigned int *)nvmem_cell_read(cell, &len);
		nvmem_cell_put(cell);
		hdmi_phy->efuse = *efuse_buffer;//hdmitx efuse only 4 bytes
		kfree((u8 *)efuse_buffer);
		HDMI_PHY_LOG("efuse=0x%x\n", hdmi_phy->efuse);
	}

	of_id = of_match_device(mtk_hdmi_phy_match, &pdev->dev);
	if (!of_id) {
		HDMI_PHY_LOG("HDMI device match failed\n");
		return -EPROBE_DEFER;
	}

	hdmi_phy->phy_data = (struct mtk_hdmi_phy_data *)of_id->data;

	if (hdmi_phy->phy_data->detect_clock_channel_only == true)
		mtk_hdmi_detect_clock_channel_only(hdmi_phy);

	if (hdmi_phy->phy_data->power_control_available == true)
		mtk_hdmi_power_control(hdmi_phy, true);

	HDMI_PHY_LOG("probe end\n");

	return of_clk_add_provider(dev->of_node, of_clk_src_simple_get,
				   hdmi_phy->txpll);
}

static int mtk_hdmi_phy_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mtk_hdmi_phy_suspend(struct device *dev)
{
	struct mtk_hdmi_phy *hdmi_phy = global_hdmi_phy;

	HDMI_PHY_FUNC();

	if (hdmi_phy->phy_data->power_control_available == true)
		mtk_hdmi_power_control(hdmi_phy, false);

	HDMI_PHY_LOG("%s hdmi phy suspend success!\n", __func__);
	return 0;
}

static int mtk_hdmi_phy_resume(struct device *dev)
{
	struct mtk_hdmi_phy *hdmi_phy = global_hdmi_phy;

	HDMI_PHY_FUNC();

	if (hdmi_phy->phy_data->power_control_available == true)
		mtk_hdmi_power_control(hdmi_phy, true);

	HDMI_PHY_LOG("%s hdmi phy resume success!\n", __func__);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mtk_hdmi_phy_pm_ops, mtk_hdmi_phy_suspend, mtk_hdmi_phy_resume);

struct platform_driver mtk_hdmi_phy_driver = {
	.probe = mtk_hdmi_phy_probe,
	.remove = mtk_hdmi_phy_remove,
	.driver = {
		.name = "mediatek-hdmi-phy",
		.of_match_table = mtk_hdmi_phy_match,
		.pm = &mtk_hdmi_phy_pm_ops,
	},
};

MODULE_AUTHOR("Can Zeng <can.zeng@mediatek.com>");
MODULE_DESCRIPTION("MediaTek MT8195 HDMI PHY Driver");
MODULE_LICENSE("GPL v2");
