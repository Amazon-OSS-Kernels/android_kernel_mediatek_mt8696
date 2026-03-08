/*
 * mt8696-afe-utils.c  --  Mediatek 8696 audio utility
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Sail Yang <sail.yang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "mt8696-afe-utils.h"
#include "mt8696-afe-common.h"
#include "mt8696-reg.h"
#include "../common/mtk-base-afe.h"
#include <linux/device.h>
#include <linux/pm_qos.h>

static struct pm_qos_request qos_request = { {0} };

#ifdef COMMON_CLOCK_FRAMEWORK_API
static const char *aud_clks[MT8696_CLK_NUM] = {
	[MT8696_CLK_TOP_AUD_26M] = "audio_26msel",
	[MT8696_CLK_TOP_AUD_BUS] = "aud_intbussel",
	[MT8696_CLK_INFRA_AUD] = "infra_audio",
	[MT8696_CLK_FA1SYS] = "a1syshp_sel",
	[MT8696_CLK_FA2SYS] = "a2syshp_sel",
	[MT8696_CLK_FAPLL1] = "apll1",
	[MT8696_CLK_FAPLL2] = "apll2",
	[MT8696_CLK_APLL_SEL] = "apll_sel",
	[MT8696_CLK_APLL2_SEL] = "apll2_sel",
	[MT8696_CLK_APLL12_DIV0] = "apll12_div0",
	[MT8696_CLK_APLL12_DIV2] = "apll12_div2",
	[MT8696_CLK_APLL12_DIV3] = "apll12_div3",
	[MT8696_CLK_ETDM_OUT1_M_SEL] = "i2so2_sel",
	[MT8696_CLK_ETDM_OUT2_M_SEL] = "i2so1_sel",
	[MT8696_CLK_ETDM_IN2_M_SEL] = "i2si1_sel",
};
#endif

struct afe_dump_reg_attr {
	uint32_t offset;
	char *name;
};

#define DUMP_REG_ENTRY(reg) {reg, #reg}

static const struct afe_dump_reg_attr etdm_dump_regs[] = {
	DUMP_REG_ENTRY(AUDIO_TOP_CON0),
	DUMP_REG_ENTRY(AUDIO_TOP_CON4),
	DUMP_REG_ENTRY(AUDIO_TOP_CON5),
	DUMP_REG_ENTRY(ASYS_TOP_CON),
	DUMP_REG_ENTRY(AFE_DAC_CON0),
	DUMP_REG_ENTRY(ETDM_OUT1_CON0),
	DUMP_REG_ENTRY(ETDM_OUT1_CON1),
	DUMP_REG_ENTRY(ETDM_OUT1_CON2),
	DUMP_REG_ENTRY(ETDM_OUT1_CON3),
	DUMP_REG_ENTRY(ETDM_OUT1_CON4),
	DUMP_REG_ENTRY(ETDM_IN2_CON0),
	DUMP_REG_ENTRY(ETDM_IN2_CON1),
	DUMP_REG_ENTRY(ETDM_IN2_CON2),
	DUMP_REG_ENTRY(ETDM_IN2_CON3),
	DUMP_REG_ENTRY(ETDM_IN2_CON4),
	DUMP_REG_ENTRY(ETDM_OUT2_CON0),
	DUMP_REG_ENTRY(ETDM_OUT2_CON1),
	DUMP_REG_ENTRY(ETDM_OUT2_CON2),
	DUMP_REG_ENTRY(ETDM_OUT2_CON3),
	DUMP_REG_ENTRY(ETDM_OUT2_CON4),
	DUMP_REG_ENTRY(ETDM_COWORK_CON0),
	DUMP_REG_ENTRY(ETDM_COWORK_CON1),
	DUMP_REG_ENTRY(ETDM_COWORK_CON3),
};

static const struct afe_dump_reg_attr memif_dump_regs[] = {
	DUMP_REG_ENTRY(ASYS_TOP_CON),
	DUMP_REG_ENTRY(AUDIO_TOP_CON5),
	DUMP_REG_ENTRY(AFE_DAC_CON0),
	DUMP_REG_ENTRY(AFE_DAC_CON1),
	DUMP_REG_ENTRY(AFE_DL5_BASE),
	DUMP_REG_ENTRY(AFE_DL5_CUR),
	DUMP_REG_ENTRY(AFE_DL5_END),
	DUMP_REG_ENTRY(AFE_DL5_CON0),
	DUMP_REG_ENTRY(AFE_DL8_BASE),
	DUMP_REG_ENTRY(AFE_DL8_CUR),
	DUMP_REG_ENTRY(AFE_DL8_END),
	DUMP_REG_ENTRY(AFE_DL8_CON0),
	DUMP_REG_ENTRY(AFE_UL1_BASE),
	DUMP_REG_ENTRY(AFE_UL1_CUR),
	DUMP_REG_ENTRY(AFE_UL1_END),
	DUMP_REG_ENTRY(AFE_UL1_CON0),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON0),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON1),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON2),
};

int mt8696_afe_init_audio_clk(struct mtk_base_afe *afe)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	size_t i;
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		afe_priv->clocks[i] = devm_clk_get(afe->dev, aud_clks[i]);
		if (IS_ERR(afe_priv->clocks[i])) {
			dev_info(afe->dev, "%s devm_clk_get %s fail\n",
				__func__, aud_clks[i]);
			return PTR_ERR(afe_priv->clocks[i]);
		}
	}
#endif
	return 0;
}

int mt8696_afe_enable_clk(struct mtk_base_afe *afe, struct clk *clk)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	if (clk) {
		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_info(afe->dev, "Failed to enable clk\n");
			return ret;
		}
	} else {
		dev_info(afe->dev, "null clk\n");
	}
#endif
	return 0;
}

void mt8696_afe_disable_clk(struct mtk_base_afe *afe, struct clk *clk)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	if (clk)
		clk_disable_unprepare(clk);
	else
		dev_info(afe->dev, "null clk\n");
#endif
}

int mt8696_afe_set_clk_rate(struct mtk_base_afe *afe, struct clk *clk,
			    unsigned int rate)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	if (clk) {
		ret = clk_set_rate(clk, rate);
		if (ret) {
			dev_info(afe->dev, "Failed to set rate\n");
			return ret;
		}
	}
#endif
	return 0;
}

int mt8696_afe_set_clk_parent(struct mtk_base_afe *afe, struct clk *clk,
			      struct clk *parent)
{
#ifdef COMMON_CLOCK_FRAMEWORK_API
	int ret;

	if (clk && parent) {
		ret = clk_set_parent(clk, parent);
		if (ret) {
			dev_info(afe->dev, "Failed to set parent\n");
			return ret;
		}
	}
#endif
	return 0;
}

static unsigned int get_top_cg_reg(unsigned int cg_type)
{
	switch (cg_type) {
	case MT8696_TOP_CG_AFE:
	case MT8696_TOP_CG_TML:
		return AUDIO_TOP_CON0;
	case MT8696_TOP_CG_ETDM_IN:
	case MT8696_TOP_CG_ETDM_OUT1:
	case MT8696_TOP_CG_ETDM_OUT2:
	case MT8696_TOP_CG_A1SYS:
	case MT8696_TOP_CG_A2SYS:
		return AUDIO_TOP_CON4;
	case MT8696_TOP_CG_A1SYS_TIMING:
	case MT8696_TOP_CG_A2SYS_TIMING:
		return ASYS_TOP_CON;
	default:
		return 0;
	}
}

static unsigned int get_top_cg_mask(unsigned int cg_type)
{
	switch (cg_type) {
	case MT8696_TOP_CG_AFE:
		return AUD_TCON0_PDN_AFE;
	case MT8696_TOP_CG_TML:
		return AUD_TCON0_PDN_TML;
	case MT8696_TOP_CG_ETDM_IN:
		return AUD_TCON4_PDN_ETDM_IN;
	case MT8696_TOP_CG_ETDM_OUT1:
		return AUD_TCON4_PDN_ETDM_OUT1;
	case MT8696_TOP_CG_ETDM_OUT2:
		return AUD_TCON4_PDN_ETDM_OUT2;
	case MT8696_TOP_CG_A1SYS:
		return AUD_TCON4_PDN_A1SYS;
	case MT8696_TOP_CG_A2SYS:
		return AUD_TCON4_PDN_A2SYS;
	case MT8696_TOP_CG_A1SYS_TIMING:
		return ASYS_TCON_A1SYS_TIMING_ON;
	case MT8696_TOP_CG_A2SYS_TIMING:
		return ASYS_TCON_A2SYS_TIMING_ON;
	default:
		return 0;
	}
}

static unsigned int get_top_cg_on_val(unsigned int cg_type)
{
	switch (cg_type) {
	case MT8696_TOP_CG_A1SYS_TIMING:
	case MT8696_TOP_CG_A2SYS_TIMING:
		return get_top_cg_mask(cg_type);
	default:
		return 0;
	}
}

static unsigned int get_top_cg_off_val(unsigned int cg_type)
{
	switch (cg_type) {
	case MT8696_TOP_CG_A1SYS_TIMING:
	case MT8696_TOP_CG_A2SYS_TIMING:
		return 0;
	default:
		return get_top_cg_mask(cg_type);
	}
}

int mt8696_afe_enable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned int reg = get_top_cg_reg(cg_type);
	unsigned int mask = get_top_cg_mask(cg_type);
	unsigned int val = get_top_cg_on_val(cg_type);
	unsigned long flags;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	afe_priv->top_cg_ref_cnt[cg_type]++;
	if (afe_priv->top_cg_ref_cnt[cg_type] == 1)
		regmap_update_bits(afe->regmap, reg, mask, val);

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	return 0;
}

int mt8696_afe_disable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned int reg = get_top_cg_reg(cg_type);
	unsigned int mask = get_top_cg_mask(cg_type);
	unsigned int val = get_top_cg_off_val(cg_type);
	unsigned long flags;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	afe_priv->top_cg_ref_cnt[cg_type]--;
	if (afe_priv->top_cg_ref_cnt[cg_type] == 0)
		regmap_update_bits(afe->regmap, reg, mask, val);
	else if (afe_priv->top_cg_ref_cnt[cg_type] < 0)
		afe_priv->top_cg_ref_cnt[cg_type] = 0;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	return 0;
}

int mt8696_afe_is_top_cg_on(struct mtk_base_afe *afe, unsigned int cg_type)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	return afe_priv->top_cg_ref_cnt[cg_type];
}

int mt8696_afe_enable_main_clk(struct mtk_base_afe *afe)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	mt8696_afe_enable_clk(afe, afe_priv->clocks[MT8696_CLK_TOP_AUD_BUS]);
	mt8696_afe_enable_clk(afe, afe_priv->clocks[MT8696_CLK_INFRA_AUD]);
	mt8696_afe_enable_clk(afe, afe_priv->clocks[MT8696_CLK_FA1SYS]);
	mt8696_afe_enable_clk(afe, afe_priv->clocks[MT8696_CLK_FA2SYS]);

	mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_AFE);
	mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_A1SYS);
	mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_A2SYS);
	mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_A1SYS_TIMING);
	mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_A2SYS_TIMING);

	mt8696_afe_enable_afe_on(afe);

	return 0;
}

int mt8696_afe_disable_main_clk(struct mtk_base_afe *afe)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	mt8696_afe_disable_afe_on(afe);

	mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_A2SYS_TIMING);
	mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_A1SYS_TIMING);
	mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_A2SYS);
	mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_A1SYS);
	mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_AFE);

	mt8696_afe_disable_clk(afe, afe_priv->clocks[MT8696_CLK_FA2SYS]);
	mt8696_afe_disable_clk(afe, afe_priv->clocks[MT8696_CLK_FA1SYS]);
	mt8696_afe_disable_clk(afe, afe_priv->clocks[MT8696_CLK_INFRA_AUD]);
	mt8696_afe_disable_clk(afe, afe_priv->clocks[MT8696_CLK_TOP_AUD_BUS]);

	return 0;
}

int mt8696_afe_enable_reg_rw_clk(struct mtk_base_afe *afe)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	mt8696_afe_enable_clk(afe, afe_priv->clocks[MT8696_CLK_TOP_AUD_BUS]);
	mt8696_afe_enable_clk(afe, afe_priv->clocks[MT8696_CLK_INFRA_AUD]);
	mt8696_afe_enable_clk(afe, afe_priv->clocks[MT8696_CLK_FA1SYS]);
	mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_A1SYS);
	mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_AFE);
	return 0;
}

int mt8696_afe_disable_reg_rw_clk(struct mtk_base_afe *afe)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_A1SYS);
	mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_AFE);
	mt8696_afe_disable_clk(afe, afe_priv->clocks[MT8696_CLK_FA1SYS]);
	mt8696_afe_disable_clk(afe, afe_priv->clocks[MT8696_CLK_INFRA_AUD]);
	mt8696_afe_disable_clk(afe, afe_priv->clocks[MT8696_CLK_TOP_AUD_BUS]);
	return 0;
}

int mt8696_afe_enable_afe_on(struct mtk_base_afe *afe)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned long flags;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	afe_priv->afe_on_ref_cnt++;
	if (afe_priv->afe_on_ref_cnt == 1)
		regmap_update_bits(afe->regmap, AFE_DAC_CON0, 0x1, 0x1);

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	return 0;
}

int mt8696_afe_disable_afe_on(struct mtk_base_afe *afe)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned long flags;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	afe_priv->afe_on_ref_cnt--;
	if (afe_priv->afe_on_ref_cnt == 0)
		regmap_update_bits(afe->regmap, AFE_DAC_CON0, 0x1, 0x0);
	else if (afe_priv->afe_on_ref_cnt < 0)
		afe_priv->afe_on_ref_cnt = 0;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	return 0;
}

static const struct mtk_base_irq_data *mt8696_get_irq_data(
	struct mtk_base_afe *afe, unsigned int irq_id)
{
	int i;

	for (i = 0; i < afe->irqs_size; i++) {
		if (irq_id == afe->irqs->irq_data[i].id)
			return &afe->irqs->irq_data[i];
	}

	return NULL;
}

int mt8696_afe_enable_irq(struct mtk_base_afe *afe, unsigned int irq_id)
{
	const struct mtk_base_irq_data *data;

	data = mt8696_get_irq_data(afe, irq_id);
	if (!data)
		return -EINVAL;

	regmap_update_bits(afe->regmap,
			   data->irq_en_reg,
			   1 << data->irq_en_shift,
			   1 << data->irq_en_shift);

	return 0;
}

int mt8696_afe_disable_irq(struct mtk_base_afe *afe, unsigned int irq_id)
{
	const struct mtk_base_irq_data *data;

	data = mt8696_get_irq_data(afe, irq_id);
	if (!data)
		return -EINVAL;

	regmap_update_bits(afe->regmap,
			   data->irq_en_reg,
			   1 << data->irq_en_shift,
			   0 << data->irq_en_shift);

	return 0;
}

int mt8696_afe_block_dpidle(struct mtk_base_afe *afe)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	int cpu_dump_latency = 1600;

	dev_dbg(afe->dev, "%s, block_dpidle_ref_cnt = %d\n",
		__func__, afe_priv->block_dpidle_ref_cnt);

	mutex_lock(&afe_priv->block_dpidle_mutex);

	if (afe_priv->block_dpidle_ref_cnt == 0) {
		//regmap_update_bits(afe_priv->scpsys,
		//	BLOCK_DPIDLE_REG,
		//	BLOCK_DPIDLE_REG_MASK,
		//	BLOCK_DPIDLE_REG_BIT_ON);
		pm_qos_add_request(&qos_request,
			PM_QOS_CPU_DMA_LATENCY, cpu_dump_latency);
	}

	afe_priv->block_dpidle_ref_cnt++;

	mutex_unlock(&afe_priv->block_dpidle_mutex);

	return 0;
}

int mt8696_afe_unblock_dpidle(struct mtk_base_afe *afe)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	dev_dbg(afe->dev, "%s, block_dpidle_ref_cnt = %d\n",
		__func__, afe_priv->block_dpidle_ref_cnt);

	mutex_lock(&afe_priv->block_dpidle_mutex);

	afe_priv->block_dpidle_ref_cnt--;

	if (afe_priv->block_dpidle_ref_cnt == 0) {
		//regmap_update_bits(afe_priv->scpsys,
		//	BLOCK_DPIDLE_REG,
		//	BLOCK_DPIDLE_REG_MASK,
		//	BLOCK_DPIDLE_REG_BIT_OFF);
		pm_qos_remove_request(&qos_request);
	} else if (afe_priv->block_dpidle_ref_cnt < 0) {
		afe_priv->block_dpidle_ref_cnt = 0;
	}

	mutex_unlock(&afe_priv->block_dpidle_mutex);

	return 0;
}

int mt8696_afe_enable_apll_tuner_cfg(struct mtk_base_afe *afe,
	unsigned int apll)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	mutex_lock(&afe_priv->afe_clk_mutex);

	afe_priv->apll_tuner_ref_cnt[apll]++;
	if (afe_priv->apll_tuner_ref_cnt[apll] != 1) {
		mutex_unlock(&afe_priv->afe_clk_mutex);
		return 0;
	}

	if (apll == MT8696_AFE_APLL1) {
		regmap_update_bits(afe->regmap, AFE_APLL_TUNER_CFG,
			AFE_APLL_TUNER_CFG_MASK, 0x412);
		regmap_update_bits(afe->regmap, AFE_APLL_TUNER_CFG,
			AFE_APLL_TUNER_CFG_EN_MASK, 0x1);
	} else {
		regmap_update_bits(afe->regmap, AFE_APLL_TUNER_CFG1,
			AFE_APLL_TUNER_CFG1_MASK, 0x414);
		regmap_update_bits(afe->regmap, AFE_APLL_TUNER_CFG1,
			AFE_APLL_TUNER_CFG1_EN_MASK, 0x1);
	}

	mutex_unlock(&afe_priv->afe_clk_mutex);
	return 0;
}

int mt8696_afe_disable_apll_tuner_cfg(struct mtk_base_afe *afe,
	unsigned int apll)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	mutex_lock(&afe_priv->afe_clk_mutex);

	afe_priv->apll_tuner_ref_cnt[apll]--;
	if (afe_priv->apll_tuner_ref_cnt[apll] == 0) {
		if (apll == MT8696_AFE_APLL1)
			regmap_update_bits(afe->regmap, AFE_APLL_TUNER_CFG,
				AFE_APLL_TUNER_CFG_EN_MASK, 0x0);
		else
			regmap_update_bits(afe->regmap, AFE_APLL_TUNER_CFG1,
				AFE_APLL_TUNER_CFG1_EN_MASK, 0x0);

	} else if (afe_priv->apll_tuner_ref_cnt[apll] < 0) {
		afe_priv->apll_tuner_ref_cnt[apll] = 0;
	}

	mutex_unlock(&afe_priv->afe_clk_mutex);
	return 0;
}

int mt8696_afe_enable_apll_associated_cfg(struct mtk_base_afe *afe,
	unsigned int apll)
{
#ifdef ENABLE_AFE_APLL_TUNER
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	if (apll == MT8696_AFE_APLL1) {
		clk_prepare_enable(afe_priv->clocks[MT8696_CLK_FAPLL1]);
		mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_APLL);
		mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_APLL_TUNER);
		mt8696_afe_enable_apll_tuner_cfg(afe, MT8696_AFE_APLL1);
	} else {
		clk_prepare_enable(afe_priv->clocks[MT8696_CLK_FAPLL2]);
		mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_APLL2);
		mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_APLL2_TUNER);
		mt8696_afe_enable_apll_tuner_cfg(afe, MT8696_AFE_APLL2);
	}
#endif
	return 0;
}

int mt8696_afe_disable_apll_associated_cfg(struct mtk_base_afe *afe,
	unsigned int apll)
{
#ifdef ENABLE_AFE_APLL_TUNER
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	if (apll == MT8696_AFE_APLL1) {
		mt8696_afe_disable_apll_tuner_cfg(afe, MT8696_AFE_APLL1);
		mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_APLL_TUNER);
		mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_APLL);
		clk_disable_unprepare(afe_priv->clocks[MT8696_CLK_FAPLL1]);
	} else {
		mt8696_afe_disable_apll_tuner_cfg(afe, MT8696_AFE_APLL2);
		mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_APLL2_TUNER);
		mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_APLL2);
		clk_disable_unprepare(afe_priv->clocks[MT8696_CLK_FAPLL2]);
	}
#endif
	return 0;
}

int mt8696_afe_dump_all_registers(struct mtk_base_afe *afe)
{
	int i = 0;
	int read_val = 0;

	for (i = 0; i < ARRAY_SIZE(etdm_dump_regs); i++) {
		regmap_read(afe->regmap, etdm_dump_regs[i].offset, &read_val);
		dev_info(afe->dev, "%s = 0x%08x\n", etdm_dump_regs[i].name,
			read_val);
	}

	for (i = 0; i < ARRAY_SIZE(memif_dump_regs); i++) {
		regmap_read(afe->regmap, memif_dump_regs[i].offset, &read_val);
		dev_info(afe->dev, "%s = 0x%08x\n", memif_dump_regs[i].name,
			read_val);
	}

	return 0;
}


