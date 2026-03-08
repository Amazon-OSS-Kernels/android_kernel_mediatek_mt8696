// SPDX-License-Identifier: GPL-2.0
/*
 * mt8532-afe-utils.c  --  Mediatek 8532 audio utility
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
 */

#include "mt8532-afe-utils.h"
#include "mt8532-afe-common.h"
#include "mt8532-reg.h"
#include "../common/mtk-base-afe.h"
#include <linux/device.h>


#ifdef COMMON_CLOCK_FRAMEWORK_API
static const char *aud_clks[MT8532_CLK_NUM] = {
	//[MT8532_CLK_TOP_SYSPLL1_D8] = "syspll1_d8",
	//[MT8532_CLK_TOP_AXI_SEL] = "axi_sel",
	[MT8532_CLK_TOP_AUDIO_26MSEL] = "audio_26msel",
	[MT8532_CLK_TOP_AUDIO_SEL] = "audio_sel",
	[MT8532_CLK_TOP_AUD_INTBUSSEL] = "aud_intbussel",
	[MT8532_CLK_INFRA_IPSYS] = "infra_ipsys",
	[MT8532_CLK_TOP_A1SYSHP_SEL] = "a1syshp_sel",
	[MT8532_CLK_TOP_INTDIR_SEL] = "intdir_sel",
	[MT8532_CLK_TOP_A2SYSHP_SEL] = "a2syshp_sel",
	[MT8532_CLK_TOP_A3SYSHP_SEL] = "a3syshp_sel",
	[MT8532_CLK_TOP_A4SYSHP_SEL] = "a4syshp_sel",
	[MT8532_CLK_TOP_APLL3_D4] = "apll3_d4",
	[MT8532_CLK_TOP_APLL4_D4] = "apll4_d4",
	[MT8532_CLK_TOP_APLL5_D4] = "apll5_d4",
	[MT8532_CLK_TOP_HDMIRX_APLL_D6] = "hdmirx_apll_d6",
	[MT8532_CLK_TOP_APLL_SEL] = "apll_sel",
	[MT8532_CLK_TOP_APLL2_SEL] = "apll2_sel",
	[MT8532_CLK_TOP_APLL3_SEL] = "apll3_sel",
	[MT8532_CLK_TOP_APLL4_SEL] = "apll4_sel",
	[MT8532_CLK_TOP_APLL5_SEL] = "apll5_sel",
	[MT8532_CLK_TOP_APLL1_D8] = "apll1_d8",
	[MT8532_CLK_TOP_APLL2_D8] = "apll2_d8",
	[MT8532_CLK_TOP_APLL3_D8] = "apll3_d8",
	[MT8532_CLK_TOP_APLL4_D8] = "apll4_d8",
	[MT8532_CLK_TOP_APLL5_D8] = "apll5_d8",
	[MT8532_CLK_TOP_ASML_SEL] = "asml_sel",
	[MT8532_CLK_TOP_ASMM_SEL] = "asmm_sel",
	[MT8532_CLK_TOP_ASMH_SEL] = "asmh_sel",
	[MT8532_CLK_TOP_APLL12_DIV4] = "apll12_div4",
	[MT8532_CLK_TOP_APLL12_DIV0] =  "apll12_div0",
	[MT8532_CLK_TOP_APLL12_DIV1] =  "apll12_div1",
	[MT8532_CLK_TOP_APLL12_DIV2] =  "apll12_div2",
	[MT8532_CLK_TOP_APLL12_DIV3] =  "apll12_div3",
	[MT8532_CLK_TOP_AUD_IEC] = "hf_faud_iec_ck",
	[MT8532_CLK_TOP_I2SI1_M] =  "hf_fi2si1_m_ck",
	[MT8532_CLK_TOP_I2SI2_M] =  "hf_fi2si2_m_ck",
	[MT8532_CLK_TOP_I2SO1_M] =  "hf_fi2so1_m_ck",
	[MT8532_CLK_TOP_I2SO2_M] =  "hf_fi2so2_m_ck",
	[MT8532_CLK_TOP_I2SI1_SEL] =  "i2si1_sel",
	[MT8532_CLK_TOP_I2SI2_SEL] =  "i2si2_sel",
	[MT8532_CLK_TOP_I2SO1_SEL] =  "i2so1_sel",
	[MT8532_CLK_TOP_I2SO2_SEL] =  "i2so2_sel",
	[MT8532_CLK_APMIXED_APLL1] =  "apll1",
	[MT8532_CLK_APMIXED_APLL2] =  "apll2",
	[MT8532_CLK_APMIXED_APLL3] =  "apll3",
	[MT8532_CLK_APMIXED_APLL4] =  "apll4",
	[MT8532_CLK_APMIXED_APLL5] =  "apll5",
};
#endif

int mt8532_afe_init_audio_clk(struct mtk_base_afe *afe)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	size_t i;
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		afe_priv->clocks[i] = devm_clk_get(afe->dev, aud_clks[i]);
		if (IS_ERR(afe_priv->clocks[i])) {
			dev_dbg(afe->dev, "%s devm_clk_get %s fail\n",
				__func__, aud_clks[i]);
			return PTR_ERR(afe_priv->clocks[i]);
		}
	}
#endif
	return 0;
}

int mt8532_afe_enable_clk(struct mtk_base_afe *afe, struct clk *clk)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	if (clk) {
		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_dbg(afe->dev, "Failed to enable clk\n");
			return ret;
		}
	} else {
		dev_dbg(afe->dev, "null clk\n");
	}
#endif
	return 0;
}

void mt8532_afe_disable_clk(struct mtk_base_afe *afe, struct clk *clk)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	if (clk)
		clk_disable_unprepare(clk);
	else
		dev_dbg(afe->dev, "null clk\n");
#endif
}

int mt8532_afe_set_clk_rate(struct mtk_base_afe *afe, struct clk *clk,
			    unsigned int rate)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	if (clk) {
		ret = clk_set_rate(clk, rate);
		if (ret) {
			dev_dbg(afe->dev, "Failed to set rate\n");
			return ret;
		}
	}
#endif
	return 0;
}

int mt8532_afe_set_clk_parent(struct mtk_base_afe *afe, struct clk *clk,
			      struct clk *parent)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	if (clk && parent) {
		ret = clk_set_parent(clk, parent);
		if (ret) {
			dev_dbg(afe->dev, "Failed to set parent\n");
			return ret;
		}
	}
#endif
	return 0;
}

static unsigned int get_top_cg_reg(unsigned int cg_type)
{
	switch (cg_type) {
	case MT8532_TOP_CG_AFE:
	case MT8532_TOP_CG_SPDIFIN_TUNER_APLL:
	case MT8532_TOP_CG_SPDIFIN_TUNER_DBG:
	case MT8532_TOP_CG_APLL_TUNER:
	case MT8532_TOP_CG_APLL2_TUNER:
	case MT8532_TOP_CG_SPDIF_OUT:
	case MT8532_TOP_CG_APLL:
	case MT8532_TOP_CG_APLL2:
	case MT8532_TOP_CG_TML:
		return AUDIO_TOP_CON0;
	case MT8532_TOP_CG_26M_DMIC_TM:
	case MT8532_TOP_CG_26M_DMIC4:
	case MT8532_TOP_CG_26M_DMIC3:
	case MT8532_TOP_CG_26M_DMIC2:
	case MT8532_TOP_CG_26M_DMIC1:
	case MT8532_TOP_CG_A1SYS_HP:
		return AUDIO_TOP_CON1;
	case MT8532_TOP_CG_LINEIN_TUNER:
	case MT8532_TOP_CG_EARC_TUNER:
		return AUDIO_TOP_CON3;
	case MT8532_TOP_CG_I2S_IN:
	case MT8532_TOP_CG_TDM_IN:
	case MT8532_TOP_CG_I2S_OUT:
	case MT8532_TOP_CG_TDM_OUT:
	case MT8532_TOP_CG_HDMI_OUT:
	case MT8532_TOP_CG_ASRC11:
	case MT8532_TOP_CG_ASRC12:
	case MT8532_TOP_CG_MULTI_IN:
	case MT8532_TOP_CG_INTDIR:
	case MT8532_TOP_CG_A1SYS:
	case MT8532_TOP_CG_A2SYS:
	case MT8532_TOP_CG_AFE_CONN:
	case MT8532_TOP_CG_PCMIF:
	case MT8532_TOP_CG_A3SYS:
	case MT8532_TOP_CG_A4SYS:
		return AUDIO_TOP_CON4;
	case MT8532_TOP_CG_GASRC0:
	case MT8532_TOP_CG_GASRC1:
	case MT8532_TOP_CG_GASRC2:
	case MT8532_TOP_CG_GASRC3:
	case MT8532_TOP_CG_GASRC4:
	case MT8532_TOP_CG_GASRC5:
	case MT8532_TOP_CG_GASRC6:
	case MT8532_TOP_CG_GASRC7:
	case MT8532_TOP_CG_GASRC8:
	case MT8532_TOP_CG_GASRC9:
	case MT8532_TOP_CG_GASRC10:
	case MT8532_TOP_CG_GASRC11:
	case MT8532_TOP_CG_GASRC12:
	case MT8532_TOP_CG_GASRC13:
	case MT8532_TOP_CG_GASRC14:
	case MT8532_TOP_CG_GASRC15:
	case MT8532_TOP_CG_GASRC16:
	case MT8532_TOP_CG_GASRC17:
	case MT8532_TOP_CG_GASRC18:
	case MT8532_TOP_CG_GASRC19:
		return AUDIO_TOP_CON6;
	case MT8532_TOP_CG_DMIC0:
	case MT8532_TOP_CG_DMIC1:
	case MT8532_TOP_CG_DMIC2:
	case MT8532_TOP_CG_DMIC3:
		return PWR2_TOP_CON0;
	case MT8532_TOP_CG_A1SYS_TIMING:
	case MT8532_TOP_CG_A2SYS_TIMING:
	case MT8532_TOP_CG_A3SYS_TIMING:
	case MT8532_TOP_CG_A4SYS_TIMING:
	case MT8532_TOP_CG_LP_MOD:
	case MT8532_TOP_CG_LP_26M_ENGEN:
		return ASYS_TOP_CON;
	default:
		return 0;
	}
}

static unsigned int get_top_cg_mask(unsigned int cg_type)
{
	switch (cg_type) {
	case MT8532_TOP_CG_AFE:
		return AUD_TCON0_PDN_AFE;
	case MT8532_TOP_CG_SPDIFIN_TUNER_APLL:
		return AUD_TCON0_PDN_SPDIFIN_TUNER_APLL;
	case MT8532_TOP_CG_SPDIFIN_TUNER_DBG:
		return AUD_TCON0_PDN_SPDIFIN_TUNER_DBG;
	case MT8532_TOP_CG_SPDIF_OUT:
		return AUD_TCON0_PDN_SPDIF_OUT;
	case MT8532_TOP_CG_APLL_TUNER:
		return AUD_TCON0_PDN_APLL_TUNER;
	case MT8532_TOP_CG_APLL2_TUNER:
		return AUD_TCON0_PDN_APLL2_TUNER;
	case MT8532_TOP_CG_APLL:
		return AUD_TCON0_PDN_APLL;
	case MT8532_TOP_CG_APLL2:
		return AUD_TCON0_PDN_APLL2;
	case MT8532_TOP_CG_TML:
		return AUD_TCON0_PDN_TML;

	case MT8532_TOP_CG_26M_DMIC_TM:
		return AUD_TCON1_PDN_26M_DMIC_TM;
	case MT8532_TOP_CG_26M_DMIC4:
		return AUD_TCON1_PDN_DMIC4_26M_UL_HOP_BCLK;
	case MT8532_TOP_CG_26M_DMIC3:
		return AUD_TCON1_PDN_DMIC3_26M_UL_HOP_BCLK;
	case MT8532_TOP_CG_26M_DMIC2:
		return AUD_TCON1_PDN_DMIC2_26M_UL_HOP_BCLK;
	case MT8532_TOP_CG_26M_DMIC1:
		return AUD_TCON1_PDN_DMIC1_26M_UL_HOP_BCLK;
	case MT8532_TOP_CG_A1SYS_HP:
		return AUD_TCON1_PDN_A1SYS_HP;

	case MT8532_TOP_CG_LINEIN_TUNER:
		return AUD_TCON3_PDN_LINEIN_TUNER;
	case MT8532_TOP_CG_EARC_TUNER:
		return AUD_TCON3_PDN_EARC_TUNER;

	case MT8532_TOP_CG_I2S_IN:
		return AUD_TCON4_PDN_I2S_IN;
	case MT8532_TOP_CG_TDM_IN:
		return AUD_TCON4_PDN_TDM_IN;
	case MT8532_TOP_CG_I2S_OUT:
		return AUD_TCON4_PDN_I2S_OUT;
	case MT8532_TOP_CG_TDM_OUT:
		return AUD_TCON4_PDN_TDM_OUT;
	case MT8532_TOP_CG_HDMI_OUT:
		return AUD_TCON4_PDN_HDMI_OUT;
	case MT8532_TOP_CG_ASRC11:
		return AUD_TCON4_PDN_ASRC11;
	case MT8532_TOP_CG_ASRC12:
		return AUD_TCON4_PDN_ASRC12;
	case MT8532_TOP_CG_MULTI_IN:
		return AUD_TCON4_PDN_MULTI_IN;
	case MT8532_TOP_CG_INTDIR:
		return AUD_TCON4_PDN_INTDIR;
	case MT8532_TOP_CG_A1SYS:
		return AUD_TCON4_PDN_A1SYS;
	case MT8532_TOP_CG_A2SYS:
		return AUD_TCON4_PDN_A2SYS;
	case MT8532_TOP_CG_AFE_CONN:
		return AUD_TCON4_PDN_AFE_CONN;
	case MT8532_TOP_CG_PCMIF:
		return AUD_TCON4_PDN_PCMIF;
	case MT8532_TOP_CG_A3SYS:
		return AUD_TCON4_PDN_A3SYS;
	case MT8532_TOP_CG_A4SYS:
		return AUD_TCON4_PDN_A4SYS;

	case MT8532_TOP_CG_GASRC0:
		return AUD_TCON6_PDN_GASRC0;
	case MT8532_TOP_CG_GASRC1:
		return AUD_TCON6_PDN_GASRC1;
	case MT8532_TOP_CG_GASRC2:
		return AUD_TCON6_PDN_GASRC2;
	case MT8532_TOP_CG_GASRC3:
		return AUD_TCON6_PDN_GASRC3;

	case MT8532_TOP_CG_GASRC4:
		return AUD_TCON6_PDN_GASRC4;
	case MT8532_TOP_CG_GASRC5:
		return AUD_TCON6_PDN_GASRC5;
	case MT8532_TOP_CG_GASRC6:
		return AUD_TCON6_PDN_GASRC6;
	case MT8532_TOP_CG_GASRC7:
		return AUD_TCON6_PDN_GASRC7;


	case MT8532_TOP_CG_GASRC8:
		return AUD_TCON6_PDN_GASRC8;
	case MT8532_TOP_CG_GASRC9:
		return AUD_TCON6_PDN_GASRC9;
	case MT8532_TOP_CG_GASRC10:
		return AUD_TCON6_PDN_GASRC10;
	case MT8532_TOP_CG_GASRC11:
		return AUD_TCON6_PDN_GASRC11;

	case MT8532_TOP_CG_GASRC12:
		return AUD_TCON6_PDN_GASRC12;
	case MT8532_TOP_CG_GASRC13:
		return AUD_TCON6_PDN_GASRC13;
	case MT8532_TOP_CG_GASRC14:
		return AUD_TCON6_PDN_GASRC14;
	case MT8532_TOP_CG_GASRC15:
		return AUD_TCON6_PDN_GASRC15;

	case MT8532_TOP_CG_GASRC16:
		return AUD_TCON6_PDN_GASRC16;
	case MT8532_TOP_CG_GASRC17:
		return AUD_TCON6_PDN_GASRC17;
	case MT8532_TOP_CG_GASRC18:
		return AUD_TCON6_PDN_GASRC18;
	case MT8532_TOP_CG_GASRC19:
		return AUD_TCON6_PDN_GASRC19;

	case MT8532_TOP_CG_DMIC0:
		return PWR2_TOP_CON_PDN_DMIC0;
	case MT8532_TOP_CG_DMIC1:
		return PWR2_TOP_CON_PDN_DMIC1;
	case MT8532_TOP_CG_DMIC2:
		return PWR2_TOP_CON_PDN_DMIC2;
	case MT8532_TOP_CG_DMIC3:
		return PWR2_TOP_CON_PDN_DMIC3;
	case MT8532_TOP_CG_A1SYS_TIMING:
		return ASYS_TCON_A1SYS_TIMING_ON;
	case MT8532_TOP_CG_A2SYS_TIMING:
		return ASYS_TCON_A2SYS_TIMING_ON;
	case MT8532_TOP_CG_A3SYS_TIMING:
		return ASYS_TCON_A3SYS_TIMING_ON;
	case MT8532_TOP_CG_A4SYS_TIMING:
		return ASYS_TCON_A4SYS_TIMING_ON;
	case MT8532_TOP_CG_LP_MOD:
		return ASYS_TCON_LP_MOD_ON;
	case MT8532_TOP_CG_LP_26M_ENGEN:
		return ASYS_TCON_LP_26M_ENGEN_ON;
	default:
		return 0;
	}
}

static unsigned int get_top_cg_on_val(unsigned int cg_type)
{
	switch (cg_type) {
	case MT8532_TOP_CG_A1SYS_TIMING:
	case MT8532_TOP_CG_A2SYS_TIMING:
	case MT8532_TOP_CG_A3SYS_TIMING:
	case MT8532_TOP_CG_A4SYS_TIMING:
	case MT8532_TOP_CG_LP_MOD:
	case MT8532_TOP_CG_LP_26M_ENGEN:
		return get_top_cg_mask(cg_type);
	default:
		return 0;
	}
}

static unsigned int get_top_cg_off_val(unsigned int cg_type)
{
	switch (cg_type) {
	case MT8532_TOP_CG_A1SYS_TIMING:
	case MT8532_TOP_CG_A2SYS_TIMING:
	case MT8532_TOP_CG_A3SYS_TIMING:
	case MT8532_TOP_CG_A4SYS_TIMING:
	case MT8532_TOP_CG_LP_MOD:
	case MT8532_TOP_CG_LP_26M_ENGEN:
		return 0;
	default:
		return get_top_cg_mask(cg_type);
	}
}

int mt8532_afe_enable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int reg = get_top_cg_reg(cg_type);
	unsigned int mask = get_top_cg_mask(cg_type);
	unsigned int val = get_top_cg_on_val(cg_type);
	unsigned long flags;
	bool need_update = false;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	afe_priv->top_cg_ref_cnt[cg_type]++;
	if (afe_priv->top_cg_ref_cnt[cg_type] == 1)
		need_update = true;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	if (need_update)
		regmap_update_bits(afe->regmap, reg, mask, val);

	return 0;
}

int mt8532_afe_disable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int reg = get_top_cg_reg(cg_type);
	unsigned int mask = get_top_cg_mask(cg_type);
	unsigned int val = get_top_cg_off_val(cg_type);
	unsigned long flags;
	bool need_update = false;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	afe_priv->top_cg_ref_cnt[cg_type]--;
	if (afe_priv->top_cg_ref_cnt[cg_type] == 0)
		need_update = true;
	else if (afe_priv->top_cg_ref_cnt[cg_type] < 0)
		afe_priv->top_cg_ref_cnt[cg_type] = 0;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	if (need_update)
		regmap_update_bits(afe->regmap, reg, mask, val);

	return 0;
}

int mt8532_afe_enable_main_clk(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	//mt8532_afe_enable_clk(afe,
	//	afe_priv->clocks[MT8532_CLK_TOP_SYSPLL1_D8]);
	//mt8532_afe_enable_clk(afe,
	//	afe_priv->clocks[MT8532_CLK_TOP_AXI_SEL]);
	mt8532_afe_enable_clk(afe,
			afe_priv->clocks[MT8532_CLK_INFRA_IPSYS]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUD_INTBUSSEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUDIO_26MSEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUDIO_SEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A1SYSHP_SEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A2SYSHP_SEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A3SYSHP_SEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A4SYSHP_SEL]);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A1SYS_HP);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A1SYS);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A2SYS);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A3SYS);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A4SYS);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_AFE);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_AFE_CONN);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A1SYS_TIMING);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A2SYS_TIMING);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_LP_26M_ENGEN);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A3SYS_TIMING);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A4SYS_TIMING);

	mt8532_afe_enable_afe_on(afe);
	return 0;
}

int mt8532_afe_disable_main_clk(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	mt8532_afe_disable_afe_on(afe);

	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A4SYS_TIMING);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A3SYS_TIMING);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_LP_26M_ENGEN);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A2SYS_TIMING);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A1SYS_TIMING);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_AFE_CONN);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_AFE);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A4SYS);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A3SYS);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A2SYS);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A1SYS);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A1SYS_HP);

	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A4SYSHP_SEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A3SYSHP_SEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A2SYSHP_SEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A1SYSHP_SEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUDIO_SEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUDIO_26MSEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUD_INTBUSSEL]);
	mt8532_afe_disable_clk(afe,
				afe_priv->clocks[MT8532_CLK_INFRA_IPSYS]);

	//mt8532_afe_disable_clk(afe,
	//	afe_priv->clocks[MT8532_CLK_TOP_AUD_INTBUSSEL]);
	//mt8532_afe_disable_clk(afe,
	//	afe_priv->clocks[MT8532_CLK_TOP_SYSPLL1_D8]);

	return 0;
}

int mt8532_afe_enable_reg_rw_clk(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	mt8532_afe_enable_clk(afe,
			afe_priv->clocks[MT8532_CLK_INFRA_IPSYS]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUD_INTBUSSEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUDIO_26MSEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUDIO_SEL]);
	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A1SYSHP_SEL]);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A1SYS_HP);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_A1SYS);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_AFE);
	//mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_AFE_CONN);
	return 0;
}

int mt8532_afe_disable_reg_rw_clk(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	//mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_AFE_CONN);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_AFE);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A1SYS);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_A1SYS_HP);

	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_A1SYSHP_SEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUDIO_SEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUDIO_26MSEL]);
	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_AUD_INTBUSSEL]);
	mt8532_afe_disable_clk(afe,
			afe_priv->clocks[MT8532_CLK_INFRA_IPSYS]);
	return 0;
}

int mt8532_afe_enable_afe_on(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned long flags;
	bool need_update = false;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	afe_priv->afe_on_ref_cnt++;
	if (afe_priv->afe_on_ref_cnt == 1)
		need_update = true;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	if (need_update)
		regmap_update_bits(afe->regmap, AFE_DAC_CON0, 0x1, 0x1);

	return 0;
}

int mt8532_afe_disable_afe_on(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned long flags;
	bool need_update = false;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	afe_priv->afe_on_ref_cnt--;
	if (afe_priv->afe_on_ref_cnt == 0)
		need_update = true;
	else if (afe_priv->afe_on_ref_cnt < 0)
		afe_priv->afe_on_ref_cnt = 0;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	if (need_update)
		regmap_update_bits(afe->regmap, AFE_DAC_CON0, 0x1, 0x0);

	return 0;
}

static const struct mtk_base_irq_data *mt8532_get_irq_data(
	struct mtk_base_afe *afe, unsigned int irq_id)
{
	int i;

	for (i = 0; i < afe->irqs_size; i++) {
		if (irq_id == afe->irqs->irq_data[i].id)
			return &afe->irqs->irq_data[i];
	}

	return NULL;
}

int mt8532_afe_enable_irq(struct mtk_base_afe *afe, unsigned int irq_id)
{
	const struct mtk_base_irq_data *data;

	data = mt8532_get_irq_data(afe, irq_id);
	if (!data)
		return -EINVAL;

	regmap_update_bits(afe->regmap,
			   data->irq_en_reg,
			   1 << data->irq_en_shift,
			   1 << data->irq_en_shift);

	return 0;
}

int mt8532_afe_disable_irq(struct mtk_base_afe *afe, unsigned int irq_id)
{
	const struct mtk_base_irq_data *data;

	data = mt8532_get_irq_data(afe, irq_id);
	if (!data)
		return -EINVAL;

	regmap_update_bits(afe->regmap,
			   data->irq_en_reg,
			   1 << data->irq_en_shift,
			   0 << data->irq_en_shift);

	return 0;
}

static int mt8532_afe_enable_spdif_in(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int port_sel;
	unsigned int mux_sel;
	unsigned int port = afe_priv->spdif_in_data.port;

	switch (port) {
	case SPDIF_IN_PORT_OPT:
	case SPDIF_IN_PORT_COAXIAL:
	case SPDIF_IN_PORT_ARC:
		mux_sel = afe_priv->spdif_in_data.ports_mux[port];
		break;
	case SPDIF_IN_PORT_NONE:
	default:
		dev_notice(afe->dev, "%s unexpected port %u\n",
			   __func__, port);
		return -EINVAL;
	}

	switch (mux_sel) {
	case SPDIF_IN_MUX_0:
		port_sel = AFE_SPDIFIN_INT_EXT_SEL_OPTICAL;
		break;
	case SPDIF_IN_MUX_1:
		port_sel = AFE_SPDIFIN_INT_EXT_SEL_COAXIAL;
		break;
	case SPDIF_IN_MUX_2:
		port_sel = AFE_SPDIFIN_INT_EXT_SEL_ARC;
		break;
	default:
		dev_notice(afe->dev, "%s unexpected mux_sel %u\n",
			   __func__, mux_sel);
		return -EINVAL;
	}

	mt8532_afe_enable_main_clk(afe);

	mt8532_afe_enable_clk(afe, afe_priv->clocks[MT8532_CLK_TOP_INTDIR_SEL]);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_INTDIR);

	regmap_write(afe->regmap, SPDIFIN_FREQ_INFO, 0x00877986);
	regmap_write(afe->regmap, SPDIFIN_FREQ_INFO_2, 0x006596e8);
	regmap_write(afe->regmap, SPDIFIN_FREQ_INFO_3, 0x000005a5);
	regmap_write(afe->regmap, AFE_SPDIFIN_BR, 0x00039000);

	regmap_update_bits(afe->regmap,
			   AFE_SPDIFIN_INT_EXT2,
			   SPDIFIN_594MODE_MASK,
			   SPDIFIN_594MODE_EN);

	regmap_update_bits(afe->regmap,
			   AFE_SPDIFIN_CFG1,
			   AFE_SPDIFIN_CFG1_SET_MASK,
			   AFE_SPDIFIN_CFG1_INT_BITS |
			   AFE_SPDIFIN_CFG1_SEL_BCK_SPDIFIN |
			   AFE_SPDIFIN_CFG1_FIFOSTART_5POINTS |
			   AFE_SPDIFIN_CFG1_SEL_DEC0_CLK_EN |
			   AFE_SPDIFIN_CFG1_SEL_DEC0_DATA_EN);

	regmap_update_bits(afe->regmap,
			   AFE_SPDIFIN_INT_EXT,
			   AFE_SPDIFIN_INT_EXT_SET_MASK,
			   AFE_SPDIFIN_INT_EXT_DATALAT_ERR_EN |
			   AFE_SPDIFIN_INT_EXT_DERR_NEW_RETEN |
			   port_sel);

	regmap_update_bits(afe->regmap,
			   AFE_SPDIFIN_CFG0,
			   AFE_SPDIFIN_CFG0_SET_MASK,
			   AFE_SPDIFIN_CFG0_FLIP |
			   AFE_SPDIFIN_CFG0_GMAT_BC_256_CYCLES |
			   AFE_SPDIFIN_CFG0_DPERR2IDLE_EN |
			   AFE_SPDIFIN_CFG0_DERR2IDLE_EN |
			   AFE_SPDIFIN_CFG0_INT_EN |
			   AFE_SPDIFIN_CFG0_TIMEOUT2IDLE_EN |
			   AFE_SPDIFIN_CFG0_DE_CNT(4) |
			   AFE_SPDIFIN_CFG0_DE_SEL_CNT |
			   AFE_SPDIFIN_CFG0_MAX_LEN_NUM(237));

	mt8532_afe_enable_irq(afe, MT8532_AFE_IRQ2);

	regmap_update_bits(afe->regmap,
			   AFE_SPDIFIN_CFG0,
			   AFE_SPDIFIN_CFG0_EN,
			   AFE_SPDIFIN_CFG0_EN);

	return 0;
}

static int mt8532_afe_disable_spdif_in(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned long flags;

	dev_dbg(afe->dev, "%s\n", __func__);

	spin_lock_irqsave(&afe_priv->spdifin_ctrl_lock, flags);

	regmap_update_bits(afe->regmap,
			   AFE_SPDIFIN_CFG0,
			   AFE_SPDIFIN_CFG0_EN,
			   0x0);

	mt8532_afe_disable_irq(afe, MT8532_AFE_IRQ2);

	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_INTDIR);

	spin_unlock_irqrestore(&afe_priv->spdifin_ctrl_lock, flags);

	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_INTDIR_SEL]);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}

int mt8532_afe_handle_spdif_in_port_change(struct mtk_base_afe *afe,
	unsigned int port)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;
	int ret = 0;

	if (port != SPDIF_IN_PORT_NONE &&
	    port != SPDIF_IN_PORT_OPT &&
	    port != SPDIF_IN_PORT_COAXIAL &&
	    port != SPDIF_IN_PORT_ARC)
		return -EINVAL;

	dev_info(afe->dev, "%s, spdif_in->port:%d, port:%d\n",
		__func__, spdif_in->port, port);

	if (spdif_in->port != port) {
		if (spdif_in->port != SPDIF_IN_PORT_NONE)
			ret = mt8532_afe_disable_spdif_in(afe);

		if (port == SPDIF_IN_PORT_NONE) {
			/* sample rate */
			spdif_in->subdata.rate = 0;
			/* usercode */
			memset((void *)spdif_in->subdata.user_code,
				0xff, sizeof(spdif_in->subdata.user_code));
			/* channel status */
			memset((void *)spdif_in->subdata.ch_status,
				0xff, sizeof(spdif_in->subdata.ch_status));
			/* stream type */
			spdif_in->subdata.stream_type = 0xff;
		}

		spdif_in->port = port;

		if (spdif_in->port != SPDIF_IN_PORT_NONE)
			ret = mt8532_afe_enable_spdif_in(afe);
	}

	return ret;
}

int mt8532_afe_block_dpidle(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	mutex_lock(&afe_priv->block_dpidle_mutex);

	if (afe_priv->block_dpidle_ref_cnt == 0) {
		//regmap_update_bits(afe_priv->scpsys,
		//	BLOCK_DPIDLE_REG,
		//	BLOCK_DPIDLE_REG_MASK,
		//	BLOCK_DPIDLE_REG_BIT_ON);
	}

	afe_priv->block_dpidle_ref_cnt++;

	mutex_unlock(&afe_priv->block_dpidle_mutex);

	return 0;
}

int mt8532_afe_unblock_dpidle(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	mutex_lock(&afe_priv->block_dpidle_mutex);

	afe_priv->block_dpidle_ref_cnt--;

	if (afe_priv->block_dpidle_ref_cnt == 0) {
		//regmap_update_bits(afe_priv->scpsys,
		//	BLOCK_DPIDLE_REG,
		//	BLOCK_DPIDLE_REG_MASK,
		//	BLOCK_DPIDLE_REG_BIT_OFF);
	} else if (afe_priv->block_dpidle_ref_cnt < 0) {
		afe_priv->block_dpidle_ref_cnt = 0;
	}

	mutex_unlock(&afe_priv->block_dpidle_mutex);

	return 0;
}

int mt8532_afe_get_be_idx(int id)
{
	int index;

	if (id < MT8532_AFE_BACKEND_BASE || id >= MT8532_AFE_BACKEND_END)
		return -EINVAL;

	index = id - MT8532_AFE_BACKEND_BASE;

	return index;
}

int mt8532_afe_set_be_active(struct mtk_base_afe *afe, int id)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int index;

	index = mt8532_afe_get_be_idx(id);
	if (index < 0)
		return -EINVAL;

	afe_priv->be_active_status |= BIT(index);

	return 0;
}

int mt8532_afe_clear_be_active(struct mtk_base_afe *afe, int id)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int index;

	index = mt8532_afe_get_be_idx(id);
	if (index < 0)
		return -EINVAL;

	afe_priv->be_active_status &= ~BIT(index);

	return 0;
}

bool mt8532_afe_is_be_active(struct mtk_base_afe *afe, int id)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int index;

	index = mt8532_afe_get_be_idx(id);
	if (index < 0)
		return false;

	if (afe_priv->be_active_status & BIT(index))
		return true;

	return false;
}

