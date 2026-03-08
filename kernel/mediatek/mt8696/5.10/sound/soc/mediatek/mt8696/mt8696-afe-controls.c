// SPDX-License-Identifier: GPL-2.0
/*
 * mt8696-afe-controls.c  --  Mediatek 8696 audio controls
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Sail Yang <sail.yang@mediatek.com>
 *
 */

#include "mt8696-afe-controls.h"
#include "mt8696-afe-common.h"
#include "mt8696-reg.h"
#include "mt8696-afe-utils.h"
#include "../common/mtk-base-afe.h"
#include <asm/div64.h>

#define ENUM_TO_STR(enum) #enum
#define BYTES_OF_INT64 (8)

enum mixer_control_func {
	CTRL_SINEGEN_LOOPBACK_MODE = 0,
	CTRL_SINEGEN_TIMING,
	CTRL_ETDM_IN_DATA_SRC,
	CTRL_ETDM_IN_SLAVE_MODE_SRC,
};

enum sinegen_loopback_mode {
	SINEGEN_I00_I01 = 0,
	SINEGEN_I02_I03,
	SINEGEN_I04_I11,
	SINEGEN_I12_I19,
	SINEGEN_I20_I21,
	SINEGEN_I22_I29,
	SINEGEN_I32_I33,
	SINEGEN_I34_I35,
	SINEGEN_I36_I37,
	SINEGEN_I38_I39,
	SINEGEN_I40_I41,
	SINEGEN_I42_I47,
	SINEGEN_O00_O01,
	SINEGEN_O02_O03,
	SINEGEN_O04_O11,
	SINEGEN_O12_O13,
	SINEGEN_O14_O15,
	SINEGEN_O16_O17,
	SINEGEN_O20_O21,
	SINEGEN_O42_O43,
	SINEGEN_O44_O45,
	SINEGEN_O46_O47,
	SINEGEN_O48_O49,
	SINEGEN_O26_O41,
	SINEGEN_UL9,
	SINEGEN_UL2,
	SINEGEN_NONE,
};

enum sinegen_timing {
	SINEGEN_8K = 0,
	SINEGEN_12K,
	SINEGEN_16K,
	SINEGEN_24K,
	SINEGEN_32K,
	SINEGEN_48K,
	SINEGEN_96K,
	SINEGEN_192K,
	SINEGEN_384K,
	SINEGEN_7P35K,
	SINEGEN_11P025K,
	SINEGEN_14P7K,
	SINEGEN_22P05K,
	SINEGEN_29P4K,
	SINEGEN_44P1K,
	SINEGEN_88P2K,
	SINEGEN_176P4K,
	SINEGEN_352P8K,
	SINEGEN_DL_1X_EN,
	SINEGEN_SGEN_EN,
};

enum etdm_in_data_src {
	ETDM_IN_FROM_PAD = 0,
	ETDM_IN_FROM_ETDM_OUT1,
	ETDM_IN_FROM_ETDM_OUT2,
};

enum etdm_in_slave_mode_src {
	ETDM_IN_SLAVE_FROM_SELF = 0,
	ETDM_IN_SLAVE_FROM_ETDM_OUT1,
	ETDM_IN_SLAVE_FROM_ETDM_OUT2,
};

static const char *const sinegen_loopback_mode_func[] = {
	ENUM_TO_STR(SINEGEN_I00_I01),
	ENUM_TO_STR(SINEGEN_I02_I03),
	ENUM_TO_STR(SINEGEN_I04_I11),
	ENUM_TO_STR(SINEGEN_I12_I19),
	ENUM_TO_STR(SINEGEN_I20_I21),
	ENUM_TO_STR(SINEGEN_I22_I29),
	ENUM_TO_STR(SINEGEN_I32_I33),
	ENUM_TO_STR(SINEGEN_I34_I35),
	ENUM_TO_STR(SINEGEN_I36_I37),
	ENUM_TO_STR(SINEGEN_I38_I39),
	ENUM_TO_STR(SINEGEN_I40_I41),
	ENUM_TO_STR(SINEGEN_I42_I47),
	ENUM_TO_STR(SINEGEN_O00_O01),
	ENUM_TO_STR(SINEGEN_O02_O03),
	ENUM_TO_STR(SINEGEN_O04_O11),
	ENUM_TO_STR(SINEGEN_O12_O13),
	ENUM_TO_STR(SINEGEN_O14_O15),
	ENUM_TO_STR(SINEGEN_O16_O17),
	ENUM_TO_STR(SINEGEN_O20_O21),
	ENUM_TO_STR(SINEGEN_O42_O43),
	ENUM_TO_STR(SINEGEN_O44_O45),
	ENUM_TO_STR(SINEGEN_O46_O47),
	ENUM_TO_STR(SINEGEN_O48_O49),
	ENUM_TO_STR(SINEGEN_O26_O41),
	ENUM_TO_STR(SINEGEN_UL9),
	ENUM_TO_STR(SINEGEN_UL2),
	ENUM_TO_STR(SINEGEN_NONE),
};

static const char *const sinegen_timing_func[] = {
	ENUM_TO_STR(SINEGEN_8K),
	ENUM_TO_STR(SINEGEN_12K),
	ENUM_TO_STR(SINEGEN_16K),
	ENUM_TO_STR(SINEGEN_24K),
	ENUM_TO_STR(SINEGEN_32K),
	ENUM_TO_STR(SINEGEN_48K),
	ENUM_TO_STR(SINEGEN_96K),
	ENUM_TO_STR(SINEGEN_192K),
	ENUM_TO_STR(SINEGEN_384K),
	ENUM_TO_STR(SINEGEN_7D35K),
	ENUM_TO_STR(SINEGEN_11D025K),
	ENUM_TO_STR(SINEGEN_14D7K),
	ENUM_TO_STR(SINEGEN_22D05K),
	ENUM_TO_STR(SINEGEN_29D4K),
	ENUM_TO_STR(SINEGEN_44D1K),
	ENUM_TO_STR(SINEGEN_88D2K),
	ENUM_TO_STR(SINEGEN_176D4K),
	ENUM_TO_STR(SINEGEN_352D8K),
	ENUM_TO_STR(SINEGEN_DL_1X_EN),
	ENUM_TO_STR(SINEGEN_SGEN_EN),
};

static const char *const etdm_in_data_src_func[] = {
	ENUM_TO_STR(ETDM_IN_FROM_PAD),
	ENUM_TO_STR(ETDM_IN_FROM_ETDM_OUT1),
	ENUM_TO_STR(ETDM_IN_FROM_ETDM_OUT2),
};

static const char *const etdm_in_slave_mode_src_func[] = {
	ENUM_TO_STR(ETDM_IN_SLAVE_FROM_SELF),
	ENUM_TO_STR(ETDM_IN_SLAVE_FROM_ETDM_OUT1),
	ENUM_TO_STR(ETDM_IN_SLAVE_FROM_ETDM_OUT2),
};

static const struct soc_enum mt8696_afe_soc_enums[] = {
	[CTRL_SINEGEN_LOOPBACK_MODE] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sinegen_loopback_mode_func),
				    sinegen_loopback_mode_func),
	[CTRL_SINEGEN_TIMING] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sinegen_timing_func),
				    sinegen_timing_func),
	[CTRL_ETDM_IN_DATA_SRC] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(etdm_in_data_src_func),
				    etdm_in_data_src_func),
	[CTRL_ETDM_IN_SLAVE_MODE_SRC] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(etdm_in_slave_mode_src_func),
				    etdm_in_slave_mode_src_func),
};

static int mt8696_afe_singen_enable_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int val = 0;

	mt8696_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON0, &val);

	mt8696_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & AFE_SINEGEN_CON0_EN);

	return 0;
}

static int mt8696_afe_singen_enable_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);

	mt8696_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0]) {
		mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_TML);

		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON0,
				   AFE_SINEGEN_CON0_EN,
				   AFE_SINEGEN_CON0_EN);
	} else {
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON0,
				   AFE_SINEGEN_CON0_EN,
				   0x0);

		mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_TML);
	}

	mt8696_afe_disable_main_clk(afe);

	return 0;
}

static int mt8696_afe_sinegen_loopback_mode_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int mode = 0;
	unsigned int val = 0;

	mt8696_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON0, &val);

	mt8696_afe_disable_main_clk(afe);

	mode = (val & AFE_SINEGEN_CON0_MODE_MASK) >> 27;

	if (mode >= SINEGEN_NONE)
		mode = SINEGEN_NONE;

	ucontrol->value.integer.value[0] = mode;

	return 0;
}

static int mt8696_afe_sinegen_loopback_mode_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int mode = 0;
	unsigned int val = 0;

	mode = ucontrol->value.integer.value[0];

	val = (mode << 27) & AFE_SINEGEN_CON0_MODE_MASK;

	mt8696_afe_enable_main_clk(afe);

	regmap_update_bits(afe->regmap, AFE_SINEGEN_CON0,
			   AFE_SINEGEN_CON0_MODE_MASK, val);

	mt8696_afe_disable_main_clk(afe);

	return 0;
}

static int mt8696_afe_sinegen_timing_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int timing = 0;
	unsigned int val = 0;

	mt8696_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON1, &val);

	mt8696_afe_disable_main_clk(afe);

	val = (val & AFE_SINEGEN_CON1_TIMING_CH1_MASK) >> 16;

	switch (val) {
	case AFE_SINEGEN_CON1_TIMING_8K:
		timing = SINEGEN_8K;
		break;
	case AFE_SINEGEN_CON1_TIMING_12K:
		timing = SINEGEN_12K;
		break;
	case AFE_SINEGEN_CON1_TIMING_16K:
		timing = SINEGEN_16K;
		break;
	case AFE_SINEGEN_CON1_TIMING_24K:
		timing = SINEGEN_24K;
		break;
	case AFE_SINEGEN_CON1_TIMING_32K:
		timing = SINEGEN_32K;
		break;
	case AFE_SINEGEN_CON1_TIMING_48K:
		timing = SINEGEN_48K;
		break;
	case AFE_SINEGEN_CON1_TIMING_96K:
		timing = SINEGEN_96K;
		break;
	case AFE_SINEGEN_CON1_TIMING_192K:
		timing = SINEGEN_192K;
		break;
	case AFE_SINEGEN_CON1_TIMING_384K:
		timing = SINEGEN_384K;
		break;
	case AFE_SINEGEN_CON1_TIMING_7P35K:
		timing = SINEGEN_7P35K;
		break;
	case AFE_SINEGEN_CON1_TIMING_11P025K:
		timing = SINEGEN_11P025K;
		break;
	case AFE_SINEGEN_CON1_TIMING_14P7K:
		timing = SINEGEN_14P7K;
		break;
	case AFE_SINEGEN_CON1_TIMING_22P05K:
		timing = SINEGEN_22P05K;
		break;
	case AFE_SINEGEN_CON1_TIMING_29P4K:
		timing = SINEGEN_29P4K;
		break;
	case AFE_SINEGEN_CON1_TIMING_44P1K:
		timing = SINEGEN_44P1K;
		break;
	case AFE_SINEGEN_CON1_TIMING_88P2K:
		timing = SINEGEN_88P2K;
		break;
	case AFE_SINEGEN_CON1_TIMING_176P4K:
		timing = SINEGEN_176P4K;
		break;
	case AFE_SINEGEN_CON1_TIMING_352P8K:
		timing = SINEGEN_352P8K;
		break;
	case AFE_SINEGEN_CON1_TIMING_DL_1X_EN:
		timing = SINEGEN_DL_1X_EN;
		break;
	case AFE_SINEGEN_CON1_TIMING_SGEN_EN:
		timing = SINEGEN_SGEN_EN;
		break;
	default:
		timing = SINEGEN_8K;
		break;
	}

	ucontrol->value.integer.value[0] = timing;

	return 0;
}

static int mt8696_afe_sinegen_timing_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int timing;
	unsigned int val;

	timing = ucontrol->value.integer.value[0];

	switch (timing) {
	case SINEGEN_8K:
		val = AFE_SINEGEN_CON1_TIMING_8K;
		break;
	case SINEGEN_12K:
		val = AFE_SINEGEN_CON1_TIMING_12K;
		break;
	case SINEGEN_16K:
		val = AFE_SINEGEN_CON1_TIMING_16K;
		break;
	case SINEGEN_24K:
		val = AFE_SINEGEN_CON1_TIMING_24K;
		break;
	case SINEGEN_32K:
		val = AFE_SINEGEN_CON1_TIMING_32K;
		break;
	case SINEGEN_48K:
		val = AFE_SINEGEN_CON1_TIMING_48K;
		break;
	case SINEGEN_96K:
		val = AFE_SINEGEN_CON1_TIMING_96K;
		break;
	case SINEGEN_192K:
		val = AFE_SINEGEN_CON1_TIMING_192K;
		break;
	case SINEGEN_384K:
		val = AFE_SINEGEN_CON1_TIMING_384K;
		break;
	case SINEGEN_7P35K:
		val = AFE_SINEGEN_CON1_TIMING_7P35K;
		break;
	case SINEGEN_11P025K:
		val = AFE_SINEGEN_CON1_TIMING_11P025K;
		break;
	case SINEGEN_14P7K:
		val = AFE_SINEGEN_CON1_TIMING_14P7K;
		break;
	case SINEGEN_22P05K:
		val = AFE_SINEGEN_CON1_TIMING_22P05K;
		break;
	case SINEGEN_29P4K:
		val = AFE_SINEGEN_CON1_TIMING_29P4K;
		break;
	case SINEGEN_44P1K:
		val = AFE_SINEGEN_CON1_TIMING_44P1K;
		break;
	case SINEGEN_88P2K:
		val = AFE_SINEGEN_CON1_TIMING_88P2K;
		break;
	case SINEGEN_176P4K:
		val = AFE_SINEGEN_CON1_TIMING_176P4K;
		break;
	case SINEGEN_352P8K:
		val = AFE_SINEGEN_CON1_TIMING_352P8K;
		break;
	case SINEGEN_DL_1X_EN:
		val = AFE_SINEGEN_CON1_TIMING_DL_1X_EN;
		break;
	case SINEGEN_SGEN_EN:
		val = AFE_SINEGEN_CON1_TIMING_SGEN_EN;
		break;
	default:
		val = AFE_SINEGEN_CON1_TIMING_8K;
		break;
	}

	mt8696_afe_enable_main_clk(afe);

	regmap_update_bits(afe->regmap, AFE_SINEGEN_CON1,
			   AFE_SINEGEN_CON1_TIMING_CH1_MASK |
			   AFE_SINEGEN_CON1_TIMING_CH2_MASK,
			   AFE_SINEGEN_CON1_TIMING_CH1(val) |
			   AFE_SINEGEN_CON1_TIMING_CH2(val));

	mt8696_afe_disable_main_clk(afe);

	return 0;
}

static int mt8696_afe_ul8_sinegen_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
#if 0
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int val = 0;

	mt8696_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, ASYS_TOP_CON, &val);

	mt8696_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & ASYS_TCON_UL8_USE_SINEGEN);
#endif
	return 0;
}

static int mt8696_afe_ul8_sinegen_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);

	mt8696_afe_enable_main_clk(afe);
#if 0
	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, ASYS_TOP_CON,
				   ASYS_TCON_UL8_USE_SINEGEN,
				   ASYS_TCON_UL8_USE_SINEGEN);
	else
		regmap_update_bits(afe->regmap, ASYS_TOP_CON,
				   ASYS_TCON_UL8_USE_SINEGEN,
				   0x0);
#endif
	mt8696_afe_disable_main_clk(afe);

	return 0;
}

static int mt8696_afe_etdm_in_data_source_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int src = ETDM_IN_FROM_PAD;
	unsigned int val = 0;

	mt8696_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, ETDM_COWORK_CON3, &val);

	mt8696_afe_disable_main_clk(afe);

	if (!strcmp(kcontrol->id.name, "ETDM_IN2_Data_Source_Select")) {
		val = val & ETDM_COWORK_CON3_IN2_DAT_SEL_MASK;

		if (val == ETDM_COWORK_CON3_IN2_DAT_SEL_IN2)
			src = ETDM_IN_FROM_PAD;
		else if (val == ETDM_COWORK_CON3_IN2_DAT_SEL_OUT1_D0)
			src = ETDM_IN_FROM_ETDM_OUT1;
		else if (val == ETDM_COWORK_CON3_IN2_DAT_SEL_OUT2_D0)
			src = ETDM_IN_FROM_ETDM_OUT2;
	}

	ucontrol->value.integer.value[0] = src;

	return 0;
}

static int mt8696_afe_etdm_in_data_source_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int src;

	src = ucontrol->value.integer.value[0];

	mt8696_afe_enable_main_clk(afe);

	if (!strcmp(kcontrol->id.name, "ETDM_IN2_Data_Source_Select")) {
		if (src == ETDM_IN_FROM_PAD)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_IN2_DAT_SEL_MASK,
				   ETDM_COWORK_CON3_IN2_DAT_SEL_IN2);
		else if (src == ETDM_IN_FROM_ETDM_OUT1)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_IN2_DAT_SEL_MASK,
				   ETDM_COWORK_CON3_IN2_DAT_SEL_OUT1_D0);
		else if (src == ETDM_IN_FROM_ETDM_OUT2)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_IN2_DAT_SEL_MASK,
				   ETDM_COWORK_CON3_IN2_DAT_SEL_OUT2_D0);
	}

	mt8696_afe_disable_main_clk(afe);

	return 0;
}

static int mt8696_afe_etdm_in_slave_mode_src_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int src = ETDM_IN_SLAVE_FROM_SELF;
	unsigned int val = 0;

	mt8696_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, ETDM_COWORK_CON2, &val);

	mt8696_afe_disable_main_clk(afe);

	if (!strcmp(kcontrol->id.name, "ETDM_IN2_Slave_Mode_Source_Select")) {
		val = val & ETDM_COWORK_CON2_IN2_SLV_SEL_MASK;

		if (val == ETDM_COWORK_CON2_IN2_SLV_SEL_IN2_SLV)
			src = ETDM_IN_SLAVE_FROM_SELF;
		else if (val == ETDM_COWORK_CON2_IN2_SLV_SEL_OUT1_MAS)
			src = ETDM_IN_SLAVE_FROM_ETDM_OUT1;
		else if (val == ETDM_COWORK_CON2_IN2_SLV_SEL_OUT2_MAS)
			src = ETDM_IN_SLAVE_FROM_ETDM_OUT2;
	}

	ucontrol->value.integer.value[0] = src;

	return 0;
}

static int mt8696_afe_etdm_in_slave_mode_src_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *plat = snd_soc_kcontrol_component(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(plat);
	unsigned int src;

	src = ucontrol->value.integer.value[0];

	mt8696_afe_enable_main_clk(afe);

	if (!strcmp(kcontrol->id.name, "ETDM_IN2_Slave_Mode_Source_Select")) {
		if (src == ETDM_IN_SLAVE_FROM_SELF)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON2,
				   ETDM_COWORK_CON2_IN2_SLV_SEL_MASK,
				   ETDM_COWORK_CON2_IN2_SLV_SEL_IN2_SLV);
		else if (src == ETDM_IN_SLAVE_FROM_ETDM_OUT1)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON2,
				   ETDM_COWORK_CON2_IN2_SLV_SEL_MASK,
				   ETDM_COWORK_CON2_IN2_SLV_SEL_OUT1_MAS);
		else if (src == ETDM_IN_SLAVE_FROM_ETDM_OUT2)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON2,
				   ETDM_COWORK_CON2_IN2_SLV_SEL_MASK,
				   ETDM_COWORK_CON2_IN2_SLV_SEL_OUT2_MAS);
	}

	mt8696_afe_disable_main_clk(afe);

	return 0;
}

static int mt8696_iec_chstatus_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 4;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0x7FFFFFFF;
	uinfo->value.integer.step = 1;
	return 0;
}

static int mt8696_iec_chstatus_set(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);

	afe->iec.ch_status.chl_stat0 = ucontrol->value.integer.value[0];
	afe->iec.ch_status.chl_stat1 = ucontrol->value.integer.value[1];
	afe->iec.ch_status.chr_stat0 = ucontrol->value.integer.value[2];
	afe->iec.ch_status.chr_stat1 = ucontrol->value.integer.value[3];

	return 0;
}

static int mt8696_avsync_mode_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	uinfo->value.integer.step = 1;
	return 0;
}

static int mt8696_avsync_mode_set(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);
	struct mtk_base_afe_memif *memif = &afe->memif[MT8696_AFE_MEMIF_DL8];
	unsigned long flags;

	if (ucontrol->value.integer.value[0]) {
		if (!memif->avsync_mode) {
			memif->avsync_mode = true;
			spin_lock_irqsave(&memif->buf_info_lock, flags);
			memif->buf_reset = true;
			memif->buf_read = memif->buf_write = 0;
			spin_unlock_irqrestore(&memif->buf_info_lock, flags);
		}
	} else
		memif->avsync_mode = false;

	return 0;
}

static int mt8696_avsync_mode_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);

	if (afe->memif[MT8696_AFE_MEMIF_DL8].avsync_mode)
		ucontrol->value.integer.value[0] = 1;
	else
		ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int mt8696_buf_info(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 3;
	return 0;
}

static int mt8696_buf_info_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);
	unsigned long flags = 0;
	struct mtk_base_afe_memif *memif = &afe->memif[MT8696_AFE_MEMIF_DL8];

	spin_lock_irqsave(&memif->buf_info_lock, flags);
	if (ucontrol->value.integer.value[2]) {
		memif->buf_read = memif->buf_write = 0;
		memif->buf_reset = true;
	} else
		memif->buf_reset = false;
	spin_unlock_irqrestore(&memif->buf_info_lock, flags);
	return 0;
}

static int mt8696_buf_info_get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	unsigned int buf_read_cur = 0, buf_read_bef = 0, buf_addition = 0;
	unsigned long flags = 0;
	unsigned long buf_cmp = 0;
	unsigned long long temp_read = 0;
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);
	struct mtk_base_afe_memif *memif = &afe->memif[MT8696_AFE_MEMIF_DL8];

	if (memif->avsync_mode) {
		spin_lock_irqsave(&memif->buf_info_lock, flags);
		temp_read = memif->buf_read;
		buf_read_bef = do_div(temp_read, memif->buffer_size);
		regmap_read(afe->regmap, AFE_DL8_CUR, &buf_read_cur);
		if (buf_read_cur == 0)
			buf_read_cur = memif->phys_buf_addr;

#if defined(FORCE_CLK_SWITCH_FEATURE_ON)
		if (memif->prepared)
			buf_read_cur -= memif->phys_buf_addr;
		else
			buf_read_cur = 0;
#else
		buf_read_cur -= memif->phys_buf_addr;
#endif

		if (buf_read_cur >= buf_read_bef)
			buf_addition = buf_read_cur - buf_read_bef;
		else
			buf_addition = buf_read_cur +
					memif->buffer_size - buf_read_bef;

		buf_cmp = memif->buf_read + buf_addition;
		if (memif->buf_read >= memif->buf_write) {
			dev_info(afe->dev, "%s underrun happened!\n", __func__);
			dev_info(afe->dev, "%s buf_read 0x%llx, buf_write 0x%llx!\n",
				__func__, memif->buf_read, memif->buf_write);
			memif->buf_read = buf_addition;
			memif->buf_reset = true;
		} else {
			memif->buf_read += buf_addition;
		}
		temp_read = memif->buf_read;
		do_div(temp_read, memif->buf_frame_size);
		ucontrol->value.integer.value[0] =
				(long)(do_div(temp_read, 0xffffffff));
		temp_read = memif->buf_write;
		do_div(temp_read, memif->buf_frame_size);
		ucontrol->value.integer.value[1] =
				(long)(do_div(temp_read, 0xffffffff));
		ucontrol->value.integer.value[2] = (long)memif->buf_reset;
		spin_unlock_irqrestore(&memif->buf_info_lock, flags);
	} else {
		ucontrol->value.integer.value[0] = 0;
		ucontrol->value.integer.value[1] = 0;
		ucontrol->value.integer.value[2] = true;
	}
	return 0;
}

static int mt8696_hdmi_force_clk_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	uinfo->value.integer.step = 1;
	return 0;
}

static int mt8696_hdmi_force_clk_set(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);

	dev_info(afe->dev, "%s %ld, previous is %d\n", __func__,
		ucontrol->value.integer.value[0], afe->force_clk_on);

	if (!ucontrol->value.integer.value[0]) {
		if (afe->force_clk_on) {
			mt8696_afe_disable_main_clk(afe);
			afe->force_clk_on = false;
		}
		afe->hdmi_force_clk_switch = ucontrol->value.integer.value[0];
	} else {
		if (!afe->force_clk_on) {
			mt8696_afe_enable_main_clk(afe);
			afe->force_clk_on = true;
		}
		afe->hdmi_force_clk_switch = ucontrol->value.integer.value[0];
	}

	return 0;
}

static int mt8696_hdmi_force_clk_get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = afe->force_clk_on;

	return 0;
}

struct gnt_memif_info {
	int dai_id;
	char dai_name[32];
	unsigned int pbuf_size_reg;
	unsigned int pbuf_size_mask;
	unsigned int pbuf_size_shift;
	unsigned int pbuf_size_unit;
};

struct gnt_memif_info gnt_memif_data[] = {
	{
		.dai_id = MT8696_AFE_MEMIF_DL8,
		.dai_name = "HDMI_OUTPUT",
		.pbuf_size_reg = AFE_MEMIF_BUF_MON4,  //how many data in prefetch buffer
		.pbuf_size_mask = 0xFFFF0000,		  //buffer size valid bit 16~31
		/* buffer size valid bit 16~31, should shif 16bit */
		.pbuf_size_shift = 16,
		.pbuf_size_unit = 64,                 //(64 * n) bits
	},
};

static long long mt8696_afe_get_next_write_timestamp(struct snd_soc_card *card,
				    struct mtk_base_afe *afe, struct gnt_memif_info info)
{
	struct snd_pcm_substream *substream;
	struct snd_soc_pcm_runtime *rtd = NULL;
	struct snd_pcm_runtime *runtime;
	unsigned long flag;
	int ret = 0, rate = 0;
	long long timestamp = 0;
	u64 temp = 0;
	int tmp_size = 0;
	int prefetch_size = 0;
	int remain_size = 0;
	int real_remain_size = 0;

	/* search target substream */
	//rtd = snd_soc_get_pcm_runtime(card, info.dai_name);
	rtd = snd_soc_get_pcm_runtime(card, card->dai_link);
	if (rtd == NULL || rtd->pcm == NULL) {
		dev_info(afe->dev, "%s can not find target substream\n", __func__);
		timestamp = -1;
		return timestamp;
	}

	substream = rtd->pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
	if (substream == NULL) {
		dev_info(afe->dev, "%s can not find target substream\n", __func__);
		timestamp = -1;
		return timestamp;
	}

	/* get target sub stream information */
	snd_pcm_stream_lock_irqsave(substream, flag);
	runtime = substream->runtime;
	if (runtime == NULL) {
		dev_info(afe->dev, "%s() playback substream is not opened(%d)\n",
		    __func__, substream->hw_opened);
		snd_pcm_stream_unlock_irqrestore(substream, flag);
		timestamp = -1;
		return timestamp;
	}

	if ((runtime->status->state == SNDRV_PCM_STATE_OPEN) ||
	    (runtime->status->state == SNDRV_PCM_STATE_DISCONNECTED) ||
	    (runtime->status->state == SNDRV_PCM_STATE_SUSPENDED)) {
		dev_info(afe->dev, "%s() playback state(%d) is not right\n",
		    __func__, runtime->status->state);
		snd_pcm_stream_unlock_irqrestore(substream, flag);
		timestamp = -1;
		return timestamp;
	}
	rate = (int)(runtime->rate);

	if (runtime->status->state == SNDRV_PCM_STATE_XRUN) {
		real_remain_size = 0;
		timestamp = (long long)mtk_timer_get_cnt(6);
		snd_pcm_stream_unlock_irqrestore(substream, flag);
		dev_info(afe->dev, "%s() playback state is xrun\n",
			__func__);
		goto output_cal;
	}

	if ((runtime->status->state != SNDRV_PCM_STATE_XRUN) &&
		  (!snd_pcm_running(substream))) {
		real_remain_size =
			(int)(runtime->buffer_size -
			snd_pcm_playback_avail(runtime) + runtime->delay);
		timestamp = (long long)mtk_timer_get_cnt(6);
		snd_pcm_stream_unlock_irqrestore(substream, flag);
		goto output_cal;
	}

    ///1. read prefetch remained buffer size
	regmap_read(afe->regmap, info.pbuf_size_reg, &tmp_size);
	tmp_size = tmp_size & info.pbuf_size_mask;
	tmp_size = tmp_size >> info.pbuf_size_shift;
	prefetch_size = (info.pbuf_size_unit * tmp_size) / 8;
	prefetch_size = bytes_to_frames(runtime, prefetch_size);

    ///2. calculate dma remained buffer size
	/*update the hw_ptr now*/
	ret = snd_pcm_update_hw_ptr(substream);
	if (ret < 0) {
		real_remain_size = 0;
		snd_pcm_stream_unlock_irqrestore(substream, flag);
		goto output_cal;
	}
	remain_size = (int)(runtime->buffer_size -
		snd_pcm_playback_avail(runtime) + runtime->delay);

	real_remain_size = remain_size + prefetch_size;
	timestamp = (long long)mtk_timer_get_cnt(6);
	dev_dbg(afe->dev, "%s() remain size dma = %d, prefetch = %d, total = %d,timestamp = 0x%16llx\n",
			__func__, remain_size, prefetch_size, real_remain_size, timestamp);
	snd_pcm_stream_unlock_irqrestore(substream, flag);

output_cal:
	temp = (u64)((13000000LL) * (long long)real_remain_size);
	do_div(temp, rate);
	timestamp += temp;
	return timestamp;
}

static int mt8696_hdmi_dai_state_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = afe->hdmi_dac_state;
	return 0;
}

static int mt8696_hdmi_dai_state_info(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0xFFFFFFFF;

	return 0;
}

static int mt8696_afe_hdmi_dai_timestamp_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer64.value[0] = afe->hdmi_dac_timestamp;
	return 0;
}

static int mt8696_afe_hdmi_timestamp_get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	//struct snd_soc_platform *platform = snd_soc_kcontrol_platform(kcontrol);
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);
	struct snd_soc_card *card = component->card;

	ucontrol->value.integer64.value[0] =
	    mt8696_afe_get_next_write_timestamp(card, afe, gnt_memif_data[0]);
	return 0;
}

static int mt8696_hdmi_audio_format_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0x7FFFFFFF;
	uinfo->value.integer.step = 1;
	return 0;
}

static int mt8696_hdmi_audio_format_get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = afe->hdmi_audio_format;

	return 0;
}

static int mt8696_hdmi_audio_format_set(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);

	dev_info(afe->dev, "%s() afe->hdmi_audio_format = %ld\n",
		__func__, ucontrol->value.integer.value[0]);
	afe->hdmi_audio_format = ucontrol->value.integer.value[0];

	return 0;
}

#define SND_SOC_CTL_RO(xname, xhandler_info, xhandler_get) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_READ | \
		  SNDRV_CTL_ELEM_ACCESS_VOLATILE, \
	.info = xhandler_info, .get = xhandler_get }

static const struct snd_kcontrol_new mt8696_afe_controls[] = {
	SOC_SINGLE_BOOL_EXT("SineGen_Enable_Switch",
			    0,
			    mt8696_afe_singen_enable_get,
			    mt8696_afe_singen_enable_put),
	SOC_ENUM_EXT("SineGen_Loopback_Mode_Select",
		     mt8696_afe_soc_enums[CTRL_SINEGEN_LOOPBACK_MODE],
		     mt8696_afe_sinegen_loopback_mode_get,
		     mt8696_afe_sinegen_loopback_mode_put),
	SOC_ENUM_EXT("SineGen_Timing_Select",
		     mt8696_afe_soc_enums[CTRL_SINEGEN_TIMING],
		     mt8696_afe_sinegen_timing_get,
		     mt8696_afe_sinegen_timing_put),
	SOC_SINGLE_RANGE("SineGen_Amp_Div_Ch1",
			 AFE_SINEGEN_CON0, 5, 0, 7, 0),
	SOC_SINGLE_RANGE("SineGen_Amp_Div_Ch2",
			 AFE_SINEGEN_CON0, 17, 0, 7, 0),
	SOC_SINGLE_RANGE("SineGen_Freq_Div_Ch1",
			 AFE_SINEGEN_CON0, 0, 0, 31, 0),
	SOC_SINGLE_RANGE("SineGen_Freq_Div_Ch2",
			 AFE_SINEGEN_CON0, 12, 0, 31, 0),
	SOC_SINGLE_BOOL_EXT("UL8_SineGen_Select",
			    0,
			    mt8696_afe_ul8_sinegen_get,
			    mt8696_afe_ul8_sinegen_put),
	SOC_ENUM_EXT("ETDM_IN2_Data_Source_Select",
		     mt8696_afe_soc_enums[CTRL_ETDM_IN_DATA_SRC],
		     mt8696_afe_etdm_in_data_source_get,
		     mt8696_afe_etdm_in_data_source_put),
	SOC_ENUM_EXT("ETDM_IN2_Slave_Mode_Source_Select",
		     mt8696_afe_soc_enums[CTRL_ETDM_IN_SLAVE_MODE_SRC],
		     mt8696_afe_etdm_in_slave_mode_src_get,
		     mt8696_afe_etdm_in_slave_mode_src_put),
	{
	 .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	 .name = "iec_channel_status_set",
	 .put = mt8696_iec_chstatus_set,
	 .info = mt8696_iec_chstatus_info,
	 .access = SNDRV_CTL_ELEM_ACCESS_WRITE,
	},
	{
	 .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	 .name = "hdmi_output_mode",
	 .info = mt8696_avsync_mode_info,
	 .put = mt8696_avsync_mode_set,
	 .get = mt8696_avsync_mode_get,
	 .access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	},
	{
	 .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	 .name = "hdmi_buf_info",
	 .info = mt8696_buf_info,
	 .put = mt8696_buf_info_set,
	 .get = mt8696_buf_info_get,
	 .access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	},
	{
	 .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	 .name = "hdmi_force_clk_switch",
	 .info = mt8696_hdmi_force_clk_info,
	 .put = mt8696_hdmi_force_clk_set,
	 .get = mt8696_hdmi_force_clk_get,
	 .access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	},
	{
	 .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	 .name = "hdmi_audio_format",
	 .put = mt8696_hdmi_audio_format_set,
	 .info = mt8696_hdmi_audio_format_info,
	 .get = mt8696_hdmi_audio_format_get,
	 .access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	},
	{
	 .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	 .name = "hdmi_dai_state",
	 .info = mt8696_hdmi_dai_state_info,
	 .get = mt8696_hdmi_dai_state_get,
	 .access = SNDRV_CTL_ELEM_ACCESS_READ,
	},
	SND_SOC_BYTES_EXT("hdmi_dai_timestamp", BYTES_OF_INT64,
		mt8696_afe_hdmi_dai_timestamp_get, NULL),
	SND_SOC_BYTES_EXT("hdmi_hrt_timestamp", BYTES_OF_INT64,
		mt8696_afe_hdmi_timestamp_get, NULL),
};

int mt8696_afe_add_controls(struct snd_soc_component *platform)
{
	return snd_soc_add_component_controls(platform, mt8696_afe_controls,
					     ARRAY_SIZE(mt8696_afe_controls));
}

