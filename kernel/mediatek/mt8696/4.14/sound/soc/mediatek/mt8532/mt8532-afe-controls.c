/*
 * mt8532-afe-controls.c  --  Mediatek 8532 audio controls
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
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

#include "mt8532-afe-controls.h"
#include "mt8532-afe-common.h"
#include "mt8532-reg.h"
#include "mt8532-afe-utils.h"
#include "../common/mtk-base-afe.h"
#include <sound/soc.h>
#include <linux/kernel.h>

#define ENUM_TO_STR(enum) #enum

#define MT8532_APLL_ADJUST_RANGE 163840

enum mixer_control_func {
	CTRL_SINEGEN_LOOPBACK_MODE = 0,
	CTRL_SINEGEN_TIMING,
	CTRL_ETDM_IN_DATA_SRC,
	CTRL_ETDM_IN_SLAVE_MODE_SRC,
	CTRL_TDMOUT_CONN_SRC,
	CTRL_MULTI_IN_BCK_SEL,
	CTRL_MULTI_IN_LRCK_SEL,
	CTRL_MULTI_IN_SDATA0_SEL,
	CTRL_MULTI_IN_SDATA1_SEL,
	CTRL_MULTI_IN_SDATA2_SEL,
	CTRL_MULTI_IN_SDATA3_SEL,
	CTRL_SPDIF_IN_PORT_SEL,
	CTRL_MULTI_IN_DATA_SRC,
	CTRL_TV_INPUT_PATH_SEL,
	CTRL_ENGENSYS_DOMAIN,
	CTRL_ETDM_MOD,
	CTRL_ETDM_DSD_BIT,
	CTRL_ETDM_DSD_CLK,
};

enum multi_in_data_src {
	MULTI_IN_FROM_SPLIN = 0,
	MULTI_IN_FROM_ETDM_OUT3,
	MULTI_IN_FROM_ETDM_IN2_EXT,
	MULTI_IN_FROM_EARC,
	MULTI_IN_FROM_HDMIRX,
	MULTI_IN_FROM_AUD2_HDMI,
};

enum sinegen_loopback_mode {
	SINEGEN_O02_O33_CM0 = 0,
	SINEGEN_O26_O27_UL9 = 0x11,
	SINEGEN_O40_O47_CM1 = 0x12,
	SINEGEN_O40_O41_UL2 = 0x16,
	SINEGEN_O00_O01_PCMTX = 0x17,
	SINEGEN_O36_O37_UL5 = 0x18,
	SINEGEN_O38_O39_UL10 = 0x1d,
	SINEGEN_UL3 = 0x1e,
	SINEGEN_O34_O35_UL4 = 0x1f,
	SINEGEN_O96_O97_GASRC0_IN = 0x25,
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
	SINEGEN_8K_A1 = 0,
	SINEGEN_12K_A1,
	SINEGEN_16K_A1,
	SINEGEN_24K_A1,
	SINEGEN_32K_A1,
	SINEGEN_48K_A1,
	SINEGEN_96K_A1,
	SINEGEN_192K_A1,
	SINEGEN_384K_A1,
	SINEGEN_ETDMOUT1_1X,
	SINEGEN_ETDMOUT2_1X,
	SINEGEN_ETDMOUT3_1X,
	SINEGEN_ETDMIN1_1X,
	SINEGEN_ETDMIN2_1X,
	SINEGEN_EXT_PCM_1X,
	SINEGEN_7D35K_A2,
	SINEGEN_11D025K_A2,
	SINEGEN_14D7K_A2,
	SINEGEN_22D05K_A2,
	SINEGEN_29D4K_A2,
	SINEGEN_44D1K_A2,
	SINEGEN_88D2K_A2,
	SINEGEN_176D4K_A2,
	SINEGEN_352D8K_A2,
	SINEGEN_ETDMIN1_NX,
	SINEGEN_ETDMIN2_NX,
	SINEGEN_8K_A3,
	SINEGEN_12K_A3,
	SINEGEN_16K_A3,
	SINEGEN_24K_A3,
	SINEGEN_32K_A3,
	SINEGEN_48K_A3,
	SINEGEN_96K_A3,
	SINEGEN_192K_A3,
	SINEGEN_384K_A3,
	SINEGEN_8K_A4,
	SINEGEN_12K_A4,
	SINEGEN_16K_A4,
	SINEGEN_24K_A4,
	SINEGEN_32K_A4,
	SINEGEN_48K_A4,
	SINEGEN_96K_A4,
	SINEGEN_192K_A4,
	SINEGEN_384K_A4,
	SINEGEN_SGEN_ERR,
};

enum etdm_in_data_src {
	ETDM_IN_FROM_PAD = 0,
	ETDM_IN_FROM_ETDM_OUT1,
	ETDM_IN_FROM_ETDM_OUT2,
	ETDM_IN_FROM_ETDM_OUT3,
};

enum etdm_in_slave_mode_src {
	ETDM_IN_SLAVE_FROM_SELF = 0,
	ETDM_IN_SLAVE_FROM_ETDM_OUT1,
	ETDM_IN_SLAVE_FROM_ETDM_OUT2,
	ETDM_IN_SLAVE_FROM_ETDM_OUT3,
};

enum tdmout_conn_src {
	I00 = 0,
	I01,
	I02,
	I03,
	I04,
	I05,
	I06,
	I07,
	I08,
	I09,
	I10,
	I11,
	I12,
	I13,
	I14,
	I15,
};

static const char *const etdm_mod_sel_func[] = {
	ENUM_TO_STR(PCM_MOD),
	ENUM_TO_STR(DSD_MOD),
};

static const char *const etdm_dsd_bit_sel_func[] = {
	ENUM_TO_STR(DSD_8_BIT),
	ENUM_TO_STR(DSD_16_BIT),
	ENUM_TO_STR(DSD_24_BIT),
	ENUM_TO_STR(DSD_32_BIT),
};

static const char *const etdm_dsd_clk_sel_func[] = {
	ENUM_TO_STR(DSD_2P8M),
	ENUM_TO_STR(DSD_5P6M),
	ENUM_TO_STR(DSD_11P2M),
};

static const char *const engen_apll_sel_func[] = {
	ENUM_TO_STR(FS_ENGEN_A1SYS),
	ENUM_TO_STR(FS_ENGEN_A2SYS),
	ENUM_TO_STR(FS_ENGEN_A3SYS),
	ENUM_TO_STR(FS_ENGEN_A4SYS),
	ENUM_TO_STR(FS_ENGEN_26M),
};

static const char *const multi_in_data_src_func[] = {
	ENUM_TO_STR(MULTI_IN_FROM_SPLIN),
	ENUM_TO_STR(MULTI_IN_FROM_ETDM_OUT3),
	ENUM_TO_STR(MULTI_IN_FROM_ETDM_IN2_EXT),
	ENUM_TO_STR(MULTI_IN_FROM_EARC),
	ENUM_TO_STR(MULTI_IN_FROM_HDMIRX),
	ENUM_TO_STR(MULTI_IN_FROM_AUD2_HDMI),
};

static const char *const tv_input_path_sel_func[] = {
	ENUM_TO_STR(TV_INPUT_PATH_SEL_NONE),
	ENUM_TO_STR(PATH_SEL_INTERNAL_EARC),
	ENUM_TO_STR(PATH_SEL_INTERNAL_ARC),
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
	ENUM_TO_STR(SINEGEN_8K_A1),
	ENUM_TO_STR(SINEGEN_12K_A1),
	ENUM_TO_STR(SINEGEN_16K_A1),
	ENUM_TO_STR(SINEGEN_24K_A1),
	ENUM_TO_STR(SINEGEN_32K_A1),
	ENUM_TO_STR(SINEGEN_48K_A1),
	ENUM_TO_STR(SINEGEN_96K_A1),
	ENUM_TO_STR(SINEGEN_192K_A1),
	ENUM_TO_STR(SINEGEN_384K_A1),

	ENUM_TO_STR(SINEGEN_ETDMOUT1_1X),
	ENUM_TO_STR(SINEGEN_ETDMOUT2_1X),
	ENUM_TO_STR(SINEGEN_ETDMOUT3_1X),
	ENUM_TO_STR(SINEGEN_ETDMIN1_1X),
	ENUM_TO_STR(SINEGEN_ETDMIN2_1X),
	ENUM_TO_STR(SINEGEN_EXT_PCM_1X),

	ENUM_TO_STR(SINEGEN_7D35K_A2),
	ENUM_TO_STR(SINEGEN_11D025K_A2),
	ENUM_TO_STR(SINEGEN_14D7K_A2),
	ENUM_TO_STR(SINEGEN_22D05K_A2),
	ENUM_TO_STR(SINEGEN_29D4K_A2),
	ENUM_TO_STR(SINEGEN_44D1K_A2),
	ENUM_TO_STR(SINEGEN_88D2K_A2),
	ENUM_TO_STR(SINEGEN_176D4K_A2),
	ENUM_TO_STR(SINEGEN_352D8K_A2),

	ENUM_TO_STR(SINEGEN_ETDMIN1_NX),
	ENUM_TO_STR(SINEGEN_ETDMIN2_NX),

	ENUM_TO_STR(SINEGEN_8K_A3),
	ENUM_TO_STR(SINEGEN_12K_A3),
	ENUM_TO_STR(SINEGEN_16K_A3),
	ENUM_TO_STR(SINEGEN_24K_A3),
	ENUM_TO_STR(SINEGEN_32K_A3),
	ENUM_TO_STR(SINEGEN_48K_A3),
	ENUM_TO_STR(SINEGEN_96K_A3),
	ENUM_TO_STR(SINEGEN_192K_A3),
	ENUM_TO_STR(SINEGEN_384K_A3),
	ENUM_TO_STR(SINEGEN_8K_A4),
	ENUM_TO_STR(SINEGEN_12K_A4),
	ENUM_TO_STR(SINEGEN_16K_A4),
	ENUM_TO_STR(SINEGEN_24K_A4),
	ENUM_TO_STR(SINEGEN_32K_A4),
	ENUM_TO_STR(SINEGEN_48K_A4),
	ENUM_TO_STR(SINEGEN_96K_A4),
	ENUM_TO_STR(SINEGEN_192K_A4),
	ENUM_TO_STR(SINEGEN_384K_A4),
};

static const char *const etdm_in_data_src_func[] = {
	ENUM_TO_STR(ETDM_IN_FROM_PAD),
	ENUM_TO_STR(ETDM_IN_FROM_ETDM_OUT1),
	ENUM_TO_STR(ETDM_IN_FROM_ETDM_OUT2),
	ENUM_TO_STR(ETDM_IN_FROM_ETDM_OUT3),
};

static const char *const etdm_in_slave_mode_src_func[] = {
	ENUM_TO_STR(ETDM_IN_SLAVE_FROM_SELF),
	ENUM_TO_STR(ETDM_IN_SLAVE_FROM_ETDM_OUT1),
	ENUM_TO_STR(ETDM_IN_SLAVE_FROM_ETDM_OUT2),
	ENUM_TO_STR(ETDM_IN_SLAVE_FROM_ETDM_OUT3),
};

static const char *const tdmout_conn_src_func[] = {
	ENUM_TO_STR(I00),
	ENUM_TO_STR(I01),
	ENUM_TO_STR(I02),
	ENUM_TO_STR(I03),
	ENUM_TO_STR(I04),
	ENUM_TO_STR(I05),
	ENUM_TO_STR(I06),
	ENUM_TO_STR(I07),
	ENUM_TO_STR(I08),
	ENUM_TO_STR(I09),
	ENUM_TO_STR(I10),
	ENUM_TO_STR(I11),
	ENUM_TO_STR(I12),
	ENUM_TO_STR(I13),
	ENUM_TO_STR(I14),
	ENUM_TO_STR(I15),
};

static const char *const multi_in_bck_sel_func[] = {
	"SPLIN_BCK",
	"ETDM_IN2_SLAVE_BCK",
	"NO_BCK",
	"EARC_INT_BCK",
	"NO_BCK",
	"ETDM_OUT3_MASTER_BCK",
	"HDMIRX_INT_BCK",
	"AUD2_HDMI_BCK",
};

static const char *const multi_in_lrck_sel_func[] = {
	"SPLIN_LRCK",
	"ETDM_IN2_SLAVE_LRCK",
	"AUD2_HDMI_LRCK",
	"EARC_INT_LRCK",
	"AUD2_HDMI_SDATA4",
	"ETDM_OUT3_MASTER_LRCK",
	"HDMIRX_INT_LRCK",
	"ETDM_OUT3_SDATA4",
};

static const char *const multi_in_sdata_sel_func[] = {
	"SPLIN_SDATA",
	"ETDM_OUT3_SDATA",
	"ETDM_IN2_SDATA",
	"EARC_SDATA",
	"HDMIRX_INT_SDATA",
	"NO_SDATA",
	"AUD2_HDMI_SDATA",
};

static const char *const spdif_in_port_sel_func[] = {
	ENUM_TO_STR(SPDIF_IN_PORT_NONE),
	ENUM_TO_STR(SPDIF_IN_PORT_OPT),
	ENUM_TO_STR(SPDIF_IN_PORT_COAXIAL),
	ENUM_TO_STR(SPDIF_IN_PORT_ARC),
};

static const struct soc_enum mt8532_afe_soc_enums[] = {
	[CTRL_MULTI_IN_DATA_SRC] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(multi_in_data_src_func),
				    multi_in_data_src_func),
	[CTRL_TV_INPUT_PATH_SEL] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tv_input_path_sel_func),
				    tv_input_path_sel_func),
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
	[CTRL_TDMOUT_CONN_SRC] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tdmout_conn_src_func),
				    tdmout_conn_src_func),
	[CTRL_MULTI_IN_BCK_SEL] =
		SOC_ENUM_SINGLE(AFE_LOOPBACK_CFG0, 0,
			ARRAY_SIZE(multi_in_bck_sel_func),
			multi_in_bck_sel_func),
	[CTRL_MULTI_IN_LRCK_SEL] =
		SOC_ENUM_SINGLE(AFE_LOOPBACK_CFG0, 3,
			ARRAY_SIZE(multi_in_lrck_sel_func),
			multi_in_lrck_sel_func),
	[CTRL_MULTI_IN_SDATA0_SEL] =
		SOC_ENUM_SINGLE(AFE_LOOPBACK_CFG0, 6,
			ARRAY_SIZE(multi_in_sdata_sel_func),
			multi_in_sdata_sel_func),
	[CTRL_MULTI_IN_SDATA1_SEL] =
		SOC_ENUM_SINGLE(AFE_LOOPBACK_CFG0, 9,
			ARRAY_SIZE(multi_in_sdata_sel_func),
			multi_in_sdata_sel_func),
	[CTRL_MULTI_IN_SDATA2_SEL] =
		SOC_ENUM_SINGLE(AFE_LOOPBACK_CFG0, 12,
			ARRAY_SIZE(multi_in_sdata_sel_func),
			multi_in_sdata_sel_func),
	[CTRL_MULTI_IN_SDATA3_SEL] =
		SOC_ENUM_SINGLE(AFE_LOOPBACK_CFG0, 15,
			ARRAY_SIZE(multi_in_sdata_sel_func),
			multi_in_sdata_sel_func),
	[CTRL_SPDIF_IN_PORT_SEL] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spdif_in_port_sel_func),
				    spdif_in_port_sel_func),
	[CTRL_ENGENSYS_DOMAIN] =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(engen_apll_sel_func),
				    engen_apll_sel_func),
	[CTRL_ETDM_MOD] =
			SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(etdm_mod_sel_func),
						etdm_mod_sel_func),
	[CTRL_ETDM_DSD_BIT] =
			SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(etdm_dsd_bit_sel_func),
						etdm_dsd_bit_sel_func),
	[CTRL_ETDM_DSD_CLK] =
			SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(etdm_dsd_clk_sel_func),
						etdm_dsd_clk_sel_func),
};

static int mt8532_afe_etdm_mod_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	size_t i;
	unsigned int etdm_mod = PCM_MOD;
	struct {
		char *name;
		unsigned int val;
		unsigned int stream;
	} etdm_table[] = {
		{ "ETDM1_OUT",	MT8532_ETDM1, SNDRV_PCM_STREAM_PLAYBACK},
		{ "ETDM3_OUT",	MT8532_ETDM3, SNDRV_PCM_STREAM_PLAYBACK},
	};

	for (i = 0; i < ARRAY_SIZE(etdm_table); i++) {
		if (strstr(name, etdm_table[i].name)) {
			unsigned int id = etdm_table[i].val;
			unsigned int dir = etdm_table[i].stream;

			etdm_mod = afe_priv->etdm_data[id].dsd_mod[dir];
		}
	}
	ucontrol->value.integer.value[0] = etdm_mod;
	return 0;

}

static int mt8532_afe_etdm_mod_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	size_t i;
	struct {
		char *name;
		unsigned int val;
		unsigned int stream;
	} etdm_table[] = {
		{ "ETDM1_OUT",	MT8532_ETDM1, SNDRV_PCM_STREAM_PLAYBACK},
		{ "ETDM3_OUT",	MT8532_ETDM3, SNDRV_PCM_STREAM_PLAYBACK},
	};

	for (i = 0; i < ARRAY_SIZE(etdm_table); i++) {
		if (strstr(name, etdm_table[i].name)) {
			unsigned int id = etdm_table[i].val;
			unsigned int dir = etdm_table[i].stream;

			afe_priv->etdm_data[id].dsd_mod[dir] =
				ucontrol->value.integer.value[0];
		}
	}
	return 0;
}

static int mt8532_afe_etdm_dsd_bit_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	size_t i;
	unsigned int dsd_bit = DSD_8_BIT;
	struct {
		char *name;
		unsigned int val;
		unsigned int stream;
	} etdm_table[] = {
		{ "ETDM1_OUT",	MT8532_ETDM1, SNDRV_PCM_STREAM_PLAYBACK},
		{ "ETDM3_OUT",	MT8532_ETDM3, SNDRV_PCM_STREAM_PLAYBACK},
	};

	for (i = 0; i < ARRAY_SIZE(etdm_table); i++) {
		if (strstr(name, etdm_table[i].name)) {
			unsigned int id = etdm_table[i].val;
			unsigned int dir = etdm_table[i].stream;

			dsd_bit = afe_priv->etdm_data[id].dsd_bit[dir];
		}
	}
	ucontrol->value.integer.value[0] = dsd_bit;
	return 0;

}

static int mt8532_afe_etdm_dsd_bit_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	size_t i;
	struct {
		char *name;
		unsigned int val;
		unsigned int stream;
	} etdm_table[] = {
		{ "ETDM1_OUT",	MT8532_ETDM1, SNDRV_PCM_STREAM_PLAYBACK},
		{ "ETDM3_OUT",	MT8532_ETDM3, SNDRV_PCM_STREAM_PLAYBACK},
	};

	for (i = 0; i < ARRAY_SIZE(etdm_table); i++) {
		if (strstr(name, etdm_table[i].name)) {
			unsigned int id = etdm_table[i].val;
			unsigned int dir = etdm_table[i].stream;

			afe_priv->etdm_data[id].dsd_bit[dir] =
				ucontrol->value.integer.value[0];
		}
	}
	return 0;
}

static int mt8532_afe_etdm_dsd_clk_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	size_t i;
	unsigned int dsd_clk = DSD_2P8M;
	struct {
		char *name;
		unsigned int val;
		unsigned int stream;
	} etdm_table[] = {
		{ "ETDM1_OUT",	MT8532_ETDM1, SNDRV_PCM_STREAM_PLAYBACK},
		{ "ETDM3_OUT",	MT8532_ETDM3, SNDRV_PCM_STREAM_PLAYBACK},
	};

	for (i = 0; i < ARRAY_SIZE(etdm_table); i++) {
		if (strstr(name, etdm_table[i].name)) {
			unsigned int id = etdm_table[i].val;
			unsigned int dir = etdm_table[i].stream;

			dsd_clk = afe_priv->etdm_data[id].dsd_clk[dir];
		}
	}
	ucontrol->value.integer.value[0] = dsd_clk;
	return 0;

}

static int mt8532_afe_etdm_dsd_clk_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	size_t i;
	struct {
		char *name;
		unsigned int val;
		unsigned int stream;
	} etdm_table[] = {
		{ "ETDM1_OUT",	MT8532_ETDM1, SNDRV_PCM_STREAM_PLAYBACK},
		{ "ETDM3_OUT",	MT8532_ETDM3, SNDRV_PCM_STREAM_PLAYBACK},
	};

	for (i = 0; i < ARRAY_SIZE(etdm_table); i++) {
		if (strstr(name, etdm_table[i].name)) {
			unsigned int id = etdm_table[i].val;
			unsigned int dir = etdm_table[i].stream;

			afe_priv->etdm_data[id].dsd_clk[dir] =
				ucontrol->value.integer.value[0];
		}
	}
	return 0;
}


#if 1
static int mt8532_afe_engen_apll_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	size_t i;
	unsigned int fs_1xen_sys = FS_ENGEN_A1SYS;
	struct {
		char *name;
		unsigned int val;
	} febe_table[] = {
		{ "DLM",	MT8532_AFE_MEMIF_DLM },
		{ "DL2",	MT8532_AFE_MEMIF_DL2 },
		{ "DL3",	MT8532_AFE_MEMIF_DL3 },
		{ "DL6",	MT8532_AFE_MEMIF_DL6 },
		{ "DL7",	MT8532_AFE_MEMIF_DL7 },
		{ "DL10",	MT8532_AFE_MEMIF_DL10 },
		{ "UL1",	MT8532_AFE_MEMIF_UL1 },
		{ "UL2",	MT8532_AFE_MEMIF_UL2 },
		{ "UL3",	MT8532_AFE_MEMIF_UL3 },
		{ "UL4",	MT8532_AFE_MEMIF_UL4 },
		{ "UL5",	MT8532_AFE_MEMIF_UL5 },
		{ "UL6",	MT8532_AFE_MEMIF_UL5 },
		{ "UL8",	MT8532_AFE_MEMIF_UL8 },
		{ "UL9",	MT8532_AFE_MEMIF_UL9 },
		{ "UL10",	MT8532_AFE_MEMIF_UL10 },
		{ "ETDM1_OUT",	MT8532_AFE_IO_ETDM1_OUT },
		{ "ETDM1_IN",	MT8532_AFE_IO_ETDM1_IN },
		{ "ETDM2_OUT",	MT8532_AFE_IO_ETDM2_OUT },
		{ "ETDM2_IN",	MT8532_AFE_IO_ETDM2_IN },
		{ "ETDM3_OUT",	MT8532_AFE_IO_ETDM3_OUT },
		{ "GASRC0", MT8532_AFE_IO_GASRC0 },
		{ "GASRC1", MT8532_AFE_IO_GASRC1 },
		{ "GASRC2", MT8532_AFE_IO_GASRC2 },
		{ "GASRC3", MT8532_AFE_IO_GASRC3 },
		{ "GASRC4", MT8532_AFE_IO_GASRC4 },
		{ "GASRC5", MT8532_AFE_IO_GASRC5 },
		{ "GASRC6", MT8532_AFE_IO_GASRC6 },
		{ "GASRC7", MT8532_AFE_IO_GASRC7 },
		{ "GASRC8", MT8532_AFE_IO_GASRC8 },
		{ "GASRC9", MT8532_AFE_IO_GASRC9 },
		{ "GASRC10",	MT8532_AFE_IO_GASRC10 },
		{ "GASRC11",	MT8532_AFE_IO_GASRC11 },
		{ "GASRC12",	MT8532_AFE_IO_GASRC12 },
		{ "GASRC13",	MT8532_AFE_IO_GASRC13 },
		{ "GASRC14",	MT8532_AFE_IO_GASRC14 },
		{ "GASRC15",	MT8532_AFE_IO_GASRC15 },
		{ "GASRC16",	MT8532_AFE_IO_GASRC16 },
		{ "GASRC17",	MT8532_AFE_IO_GASRC17 },
		{ "GASRC18",	MT8532_AFE_IO_GASRC18 },
		{ "GASRC19",	MT8532_AFE_IO_GASRC19 },
	};

	for (i = 0; i < ARRAY_SIZE(febe_table); i++) {
		if (strstr(name, febe_table[i].name)) {
			unsigned int id = febe_table[i].val;

			if (id < MT8532_AFE_MEMIF_NUM)
				fs_1xen_sys = afe_priv->fe_data[id].fs_1xen_sys;
			else
				fs_1xen_sys = afe_priv->be_data[id].fs_1xen_sys;
			break;
		}
	}
	ucontrol->value.integer.value[0] = fs_1xen_sys;
	return 0;

}


static int mt8532_afe_engen_apll_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	size_t i;
	struct {
		char *name;
		unsigned int val;
	} febe_table[] = {
		{ "DLM",	MT8532_AFE_MEMIF_DLM },
		{ "DL2",	MT8532_AFE_MEMIF_DL2 },
		{ "DL3",	MT8532_AFE_MEMIF_DL3 },
		{ "DL6",	MT8532_AFE_MEMIF_DL6 },
		{ "DL7",	MT8532_AFE_MEMIF_DL7 },
		{ "DL10",	MT8532_AFE_MEMIF_DL10 },
		{ "UL1",	MT8532_AFE_MEMIF_UL1 },
		{ "UL2",	MT8532_AFE_MEMIF_UL2 },
		{ "UL3",	MT8532_AFE_MEMIF_UL3 },
		{ "UL4",	MT8532_AFE_MEMIF_UL4 },
		{ "UL5",	MT8532_AFE_MEMIF_UL5 },
		{ "UL6",	MT8532_AFE_MEMIF_UL5 },
		{ "UL8",	MT8532_AFE_MEMIF_UL8 },
		{ "UL9",	MT8532_AFE_MEMIF_UL9 },
		{ "UL10",	MT8532_AFE_MEMIF_UL10 },
		{ "ETDM1_OUT",	MT8532_AFE_IO_ETDM1_OUT },
		{ "ETDM1_IN",	MT8532_AFE_IO_ETDM1_IN },
		{ "ETDM2_OUT",	MT8532_AFE_IO_ETDM2_OUT },
		{ "ETDM2_IN",	MT8532_AFE_IO_ETDM2_IN },
		{ "ETDM3_OUT",	MT8532_AFE_IO_ETDM3_OUT },
		{ "GASRC0",	MT8532_AFE_IO_GASRC0 },
		{ "GASRC1",	MT8532_AFE_IO_GASRC1 },
		{ "GASRC2",	MT8532_AFE_IO_GASRC2 },
		{ "GASRC3",	MT8532_AFE_IO_GASRC3 },
		{ "GASRC4",	MT8532_AFE_IO_GASRC4 },
		{ "GASRC5",	MT8532_AFE_IO_GASRC5 },
		{ "GASRC6",	MT8532_AFE_IO_GASRC6 },
		{ "GASRC7",	MT8532_AFE_IO_GASRC7 },
		{ "GASRC8",	MT8532_AFE_IO_GASRC8 },
		{ "GASRC9",	MT8532_AFE_IO_GASRC9 },
		{ "GASRC10",	MT8532_AFE_IO_GASRC10 },
		{ "GASRC11",	MT8532_AFE_IO_GASRC11 },
		{ "GASRC12",	MT8532_AFE_IO_GASRC12 },
		{ "GASRC13",	MT8532_AFE_IO_GASRC13 },
		{ "GASRC14",	MT8532_AFE_IO_GASRC14 },
		{ "GASRC15",	MT8532_AFE_IO_GASRC15 },
		{ "GASRC16",	MT8532_AFE_IO_GASRC16 },
		{ "GASRC17",	MT8532_AFE_IO_GASRC17 },
		{ "GASRC18",	MT8532_AFE_IO_GASRC18 },
		{ "GASRC19",	MT8532_AFE_IO_GASRC19 },
	};

	for (i = 0; i < ARRAY_SIZE(febe_table); i++) {
		if (strstr(name, febe_table[i].name)) {
			unsigned int id = febe_table[i].val;

			if (id < MT8532_AFE_MEMIF_NUM)
				afe_priv->fe_data[id].fs_1xen_sys =
					ucontrol->value.integer.value[0];
			else
				afe_priv->be_data[id].fs_1xen_sys =
					ucontrol->value.integer.value[0];
			break;
		}
	}

	return 0;
}
#endif

static int mt8532_afe_singen_enable_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON0, &val);

	mt8532_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & AFE_SINEGEN_CON0_EN);

	return 0;
}

static int mt8532_afe_singen_enable_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt8532_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0]) {
		mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_TML);

		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON0,
				   AFE_SINEGEN_CON0_EN,
				   AFE_SINEGEN_CON0_EN);
	} else {
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON0,
				   AFE_SINEGEN_CON0_EN,
				   0x0);

		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_TML);
	}

	mt8532_afe_disable_main_clk(afe);

	return 0;
}
#if 0
static int mt8532_afe_sinegen_loopback_mode_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int mode;
	unsigned int val;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON0, &val);

	mt8532_afe_disable_main_clk(afe);

	mode = (val & AFE_SINEGEN_CON2_MODE_MASK) >> 27;

	if (mode >= SINEGEN_NONE)
		mode = SINEGEN_NONE;

	ucontrol->value.integer.value[0] = mode;

	return 0;
}

static int mt8532_afe_sinegen_loopback_mode_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int mode;
	unsigned int val;

	mode = ucontrol->value.integer.value[0];

	val = (mode << 27) & AFE_SINEGEN_CON2_MODE_MASK;

	mt8532_afe_enable_main_clk(afe);

	regmap_update_bits(afe->regmap, AFE_SINEGEN_CON0,
			   AFE_SINEGEN_CON2_MODE_MASK, val);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}
#endif
static int mt8532_afe_sinegen_timing_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int timing = SINEGEN_8K_A1;
	unsigned int val = 0;
	unsigned int val2 = 0;

	mt8532_afe_enable_reg_rw_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON1, &val);
	regmap_read(afe->regmap, A3_A4_TIMING_SEL0, &val2);

	mt8532_afe_disable_reg_rw_clk(afe);

	val = (val & AFE_SINEGEN_CON1_TIMING_CH1_MASK) >> 16;
	val2 = (val2 & AFE_A3A4_TIMING_SEL0_SGEN_CH1_MASK) >> 2;

	switch (val) {
	case AFE_SINEGEN_CON1_TIMING_8K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_8K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_8K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_8K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_12K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_12K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_12K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_12K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_16K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_16K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_16K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_16K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_24K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_24K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_24K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_24K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_32K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_32K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_32K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_32K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_48K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_48K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_48K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_48K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_96K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_96K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_96K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_96K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_192K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_192K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_192K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_192K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_384K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_384K_A1;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A3)
			timing = SINEGEN_384K_A3;
		else if (val2 == AFE_A3A4_TIMING_SEL0_A4)
			timing = SINEGEN_384K_A4;
		break;
	case AFE_SINEGEN_CON1_TIMING_7D35K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_7D35K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	case AFE_SINEGEN_CON1_TIMING_11D025K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_11D025K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	case AFE_SINEGEN_CON1_TIMING_14D7K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_14D7K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	case AFE_SINEGEN_CON1_TIMING_22D05K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_22D05K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	case AFE_SINEGEN_CON1_TIMING_29D4K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_29D4K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	case AFE_SINEGEN_CON1_TIMING_44D1K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_44D1K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	case AFE_SINEGEN_CON1_TIMING_88D2K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_88D2K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	case AFE_SINEGEN_CON1_TIMING_176D4K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_176D4K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	case AFE_SINEGEN_CON1_TIMING_352D8K:
		if (val2 == AFE_A3A4_TIMING_SEL0_A1A2)
			timing = SINEGEN_352D8K_A2;
		else
			timing = SINEGEN_SGEN_ERR;
		break;
	default:
		timing = SINEGEN_8K_A1;
		break;
	}

	ucontrol->value.integer.value[0] = timing;

	return 0;
}

static int mt8532_afe_sinegen_timing_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int timing;
	unsigned int val;
	unsigned int val2;

	timing = ucontrol->value.integer.value[0];

	switch (timing) {
	case SINEGEN_8K_A1:
		val = AFE_SINEGEN_CON1_TIMING_8K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_12K_A1:
		val = AFE_SINEGEN_CON1_TIMING_12K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_16K_A1:
		val = AFE_SINEGEN_CON1_TIMING_16K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_24K_A1:
		val = AFE_SINEGEN_CON1_TIMING_24K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_32K_A1:
		val = AFE_SINEGEN_CON1_TIMING_32K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_48K_A1:
		val = AFE_SINEGEN_CON1_TIMING_48K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_96K_A1:
		val = AFE_SINEGEN_CON1_TIMING_96K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_192K_A1:
		val = AFE_SINEGEN_CON1_TIMING_192K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_384K_A1:
		val = AFE_SINEGEN_CON1_TIMING_384K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_8K_A3:
		val = AFE_SINEGEN_CON1_TIMING_8K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_12K_A3:
		val = AFE_SINEGEN_CON1_TIMING_12K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_16K_A3:
		val = AFE_SINEGEN_CON1_TIMING_16K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_24K_A3:
		val = AFE_SINEGEN_CON1_TIMING_24K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_32K_A3:
		val = AFE_SINEGEN_CON1_TIMING_32K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_48K_A3:
		val = AFE_SINEGEN_CON1_TIMING_48K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_96K_A3:
		val = AFE_SINEGEN_CON1_TIMING_96K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_192K_A3:
		val = AFE_SINEGEN_CON1_TIMING_192K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_384K_A3:
		val = AFE_SINEGEN_CON1_TIMING_384K;
		val2 = AFE_A3A4_TIMING_SEL0_A3;
		break;
	case SINEGEN_8K_A4:
		val = AFE_SINEGEN_CON1_TIMING_8K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_12K_A4:
		val = AFE_SINEGEN_CON1_TIMING_12K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_16K_A4:
		val = AFE_SINEGEN_CON1_TIMING_16K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_24K_A4:
		val = AFE_SINEGEN_CON1_TIMING_24K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_32K_A4:
		val = AFE_SINEGEN_CON1_TIMING_32K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_48K_A4:
		val = AFE_SINEGEN_CON1_TIMING_48K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_96K_A4:
		val = AFE_SINEGEN_CON1_TIMING_96K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_192K_A4:
		val = AFE_SINEGEN_CON1_TIMING_192K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_384K_A4:
		val = AFE_SINEGEN_CON1_TIMING_384K;
		val2 = AFE_A3A4_TIMING_SEL0_A4;
		break;
	case SINEGEN_7D35K_A2:
		val = AFE_SINEGEN_CON1_TIMING_7D35K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_11D025K_A2:
		val = AFE_SINEGEN_CON1_TIMING_11D025K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_14D7K_A2:
		val = AFE_SINEGEN_CON1_TIMING_14D7K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_22D05K_A2:
		val = AFE_SINEGEN_CON1_TIMING_22D05K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_29D4K_A2:
		val = AFE_SINEGEN_CON1_TIMING_29D4K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_44D1K_A2:
		val = AFE_SINEGEN_CON1_TIMING_44D1K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_88D2K_A2:
		val = AFE_SINEGEN_CON1_TIMING_88D2K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_176D4K_A2:
		val = AFE_SINEGEN_CON1_TIMING_176D4K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	case SINEGEN_352D8K_A2:
		val = AFE_SINEGEN_CON1_TIMING_352D8K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	default:
		val = AFE_SINEGEN_CON1_TIMING_8K;
		val2 = AFE_A3A4_TIMING_SEL0_A1A2;
		break;
	}

	mt8532_afe_enable_reg_rw_clk(afe);

	regmap_update_bits(afe->regmap, AFE_SINEGEN_CON1,
			   AFE_SINEGEN_CON1_TIMING_CH1_MASK |
			   AFE_SINEGEN_CON1_TIMING_CH2_MASK,
			   AFE_SINEGEN_CON1_TIMING_CH1(val) |
			   AFE_SINEGEN_CON1_TIMING_CH2(val));
	regmap_update_bits(afe->regmap, A3_A4_TIMING_SEL0,
			   AFE_A3A4_TIMING_SEL0_SGEN_CH1_MASK |
			   AFE_A3A4_TIMING_SEL0_SGEN_CH2_MASK,
			   AFE_A3A4_TIMING_SEL0_CH1(val2) |
			   AFE_A3A4_TIMING_SEL0_CH2(val2));

	mt8532_afe_disable_reg_rw_clk(afe);

	return 0;
}

static int mt8532_afe_ul8_sinegen_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, ASYS_TOP_CON, &val);

	mt8532_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & ASYS_TCON_UL8_USE_SINEGEN);

	return 0;
}

static int mt8532_afe_ul8_sinegen_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt8532_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, ASYS_TOP_CON,
				   ASYS_TCON_UL8_USE_SINEGEN,
				   ASYS_TCON_UL8_USE_SINEGEN);
	else
		regmap_update_bits(afe->regmap, ASYS_TOP_CON,
				   ASYS_TCON_UL8_USE_SINEGEN,
				   0x0);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}

static int mt8532_afe_mini_clk_en_get(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	//unsigned int val = 0;

	ucontrol->value.integer.value[0] = afe->mini_clk_enable;
	return 0;
}

static int mt8532_afe_mini_clk_en_put(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	if (ucontrol->value.integer.value[0] != afe->mini_clk_enable) {
		if (ucontrol->value.integer.value[0]) {
			mt8532_afe_enable_reg_rw_clk(afe);
			afe->mini_clk_enable = 1;
		} else {
			mt8532_afe_disable_reg_rw_clk(afe);
			afe->mini_clk_enable = 0;
		}
	}

	return 0;
}

static int mt8532_afe_tdm_out1_sinegen_get(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_reg_rw_clk(afe);

	regmap_read(afe->regmap, ETDM_COWORK_CON3, &val);

	mt8532_afe_disable_reg_rw_clk(afe);

	ucontrol->value.integer.value[0] =
		(val & ETDM_COWORK_CON3_OUT1_USE_SINEGEN);

	return 0;
}

static int mt8532_afe_tdm_out1_sinegen_put(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt8532_afe_enable_reg_rw_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_OUT1_USE_SINEGEN,
				   ETDM_COWORK_CON3_OUT1_USE_SINEGEN);
	else
		regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_OUT1_USE_SINEGEN,
				   0x0);

	mt8532_afe_disable_reg_rw_clk(afe);

	return 0;
}

static int mt8532_afe_tdm_out2_sinegen_get(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_reg_rw_clk(afe);

	regmap_read(afe->regmap, ETDM_COWORK_CON3, &val);

	mt8532_afe_disable_reg_rw_clk(afe);

	ucontrol->value.integer.value[0] =
		(val & ETDM_COWORK_CON3_OUT2_USE_SINEGEN);

	return 0;
}

static int mt8532_afe_tdm_out2_sinegen_put(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt8532_afe_enable_reg_rw_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_OUT2_USE_SINEGEN,
				   ETDM_COWORK_CON3_OUT2_USE_SINEGEN);
	else
		regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_OUT2_USE_SINEGEN,
				   0x0);

	mt8532_afe_disable_reg_rw_clk(afe);

	return 0;
}

static int mt8532_afe_tdm_out3_sinegen_get(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_reg_rw_clk(afe);

	regmap_read(afe->regmap, ETDM_COWORK_CON3, &val);

	mt8532_afe_disable_reg_rw_clk(afe);

	ucontrol->value.integer.value[0] =
		(val & ETDM_COWORK_CON3_OUT3_USE_SINEGEN);

	return 0;
}

static int mt8532_afe_tdm_out3_sinegen_put(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt8532_afe_enable_reg_rw_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_OUT3_USE_SINEGEN,
				   ETDM_COWORK_CON3_OUT3_USE_SINEGEN);
	else
		regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_OUT3_USE_SINEGEN,
				   0x0);

	mt8532_afe_disable_reg_rw_clk(afe);

	return 0;
}


static int mt8532_afe_multi_in_data_source_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int src = MULTI_IN_FROM_SPLIN;
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_LOOPBACK_CFG0, &val);

	mt8532_afe_disable_main_clk(afe);

	val = val & MULTI_IN_DAT0_SEL_MASK;

	if (val == MULTI_IN_DAT0_SEL_SPLIN)
		src = MULTI_IN_FROM_SPLIN;
	else if (val == MULTI_IN_DAT0_SEL_ETDM_OUT3)
		src = MULTI_IN_FROM_ETDM_OUT3;
	else if (val == MULTI_IN_DAT0_SEL_ETDM_IN2)
		src = MULTI_IN_FROM_ETDM_IN2_EXT;
	else if (val == MULTI_IN_DAT0_SEL_EARC)
		src = MULTI_IN_FROM_EARC;
	else if (val == MULTI_IN_DAT0_SEL_HDMIRX)
		src = MULTI_IN_FROM_HDMIRX;
	else if (val == MULTI_IN_DAT0_SEL_AUD2_HDMI)
		src = MULTI_IN_FROM_AUD2_HDMI;

	ucontrol->value.integer.value[0] = src;

	return 0;
}

static int mt8532_afe_multi_in_data_source_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int src;

	src = ucontrol->value.integer.value[0];

	mt8532_afe_enable_main_clk(afe);

	if (src == MULTI_IN_FROM_SPLIN)
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_DAT0_SEL_MASK,
			   MULTI_IN_DAT0_SEL_SPLIN);
	else if (src == MULTI_IN_FROM_ETDM_OUT3)
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_DAT0_SEL_MASK,
			   MULTI_IN_DAT0_SEL_ETDM_OUT3);
	else if (src == MULTI_IN_FROM_ETDM_IN2_EXT)
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_DAT0_SEL_MASK,
			   MULTI_IN_DAT0_SEL_ETDM_IN2);
	else if (src == MULTI_IN_FROM_EARC)
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_DAT0_SEL_MASK,
			   MULTI_IN_DAT0_SEL_EARC);
	else if (src == MULTI_IN_FROM_HDMIRX)
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_DAT0_SEL_MASK,
			   MULTI_IN_DAT0_SEL_HDMIRX);
	else if (src == MULTI_IN_FROM_AUD2_HDMI)
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_DAT0_SEL_MASK,
			   MULTI_IN_DAT0_SEL_AUD2_HDMI);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}

static int mt8532_afe_tv_input_path_sel_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int src = TV_INPUT_PATH_SEL_NONE;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_LOOPBACK_CFG0, &val);

	mt8532_afe_disable_main_clk(afe);

	val = val & MULTI_IN_DAT0_SEL_MASK;

	if (val == MULTI_IN_DAT0_SEL_SPLIN &&
	    spdif_in->port != SPDIF_IN_PORT_NONE)
		src = PATH_SEL_INTERNAL_ARC;
	else if (val == MULTI_IN_DAT0_SEL_EARC)
		src = PATH_SEL_INTERNAL_EARC;
	else {
		src = TV_INPUT_PATH_SEL_NONE;
		dev_dbg(afe->dev, "%s it's not ARC or EARC path.\n", __func__);
	}

	ucontrol->value.integer.value[0] = src;

	return 0;
}

static int mt8532_afe_tv_input_path_sel_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int src;
	struct snd_kcontrol *kctl_ul1, *kctl_earc;
	struct snd_ctl_elem_id elem_id;
	struct snd_ctl_elem_value *ucontrol_ul1, *ucontrol_earc;

	mt8532_afe_enable_main_clk(afe);

	src = ucontrol->value.integer.value[0];
	ucontrol_ul1 = kzalloc(sizeof(*ucontrol_ul1), GFP_KERNEL);
	if (!ucontrol_ul1)
		return -ENOMEM;
	ucontrol_earc = kzalloc(sizeof(*ucontrol_earc), GFP_KERNEL);
	if (!ucontrol_earc) {
		kfree(ucontrol_ul1);
		return -ENOMEM;
	}
	memset(&elem_id, 0, sizeof(elem_id));
	elem_id.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
	strlcpy(elem_id.name, "UL1 Mux", sizeof(elem_id.name));
	kctl_ul1 = snd_ctl_find_id(afe_priv->card, &elem_id);
	strlcpy(elem_id.name, "Earc_Enable", sizeof(elem_id.name));
	kctl_earc = snd_ctl_find_id(afe_priv->card, &elem_id);
	if (!kctl_ul1 || !kctl_earc) {
		dev_info(afe->dev, "cannot find kctl ul1 or earcrx.\n");
		kfree(ucontrol_ul1);
		kfree(ucontrol_earc);
		return -EINVAL;
	}

	if (src == PATH_SEL_INTERNAL_ARC) {
		ucontrol_ul1->value.enumerated.item[0] = 0; //"Spdif_In"
		kctl_ul1->put(kctl_ul1, ucontrol_ul1);
		ucontrol_earc->value.integer.value[0] = 1; //earc_enable
		kctl_earc->put(kctl_earc, ucontrol_earc);
		mt8532_afe_handle_spdif_in_port_change(afe, SPDIF_IN_PORT_ARC);
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_BCK_SEL_MASK|MULTI_IN_LRCK_SEL_MASK|
			   MULTI_IN_DAT0_SEL_MASK|MULTI_IN_DAT1_SEL_MASK|
			   MULTI_IN_DAT2_SEL_MASK|MULTI_IN_DAT3_SEL_MASK,
			   MULTI_IN_BCK_SEL_SPLIN|MULTI_IN_LRCK_SEL_SPLIN|
			   MULTI_IN_DAT0_SEL_SPLIN|MULTI_IN_DAT1_SEL_SPLIN|
			   MULTI_IN_DAT2_SEL_SPLIN|MULTI_IN_DAT3_SEL_SPLIN);
	} else if (src == PATH_SEL_INTERNAL_EARC) {
		ucontrol_ul1->value.enumerated.item[0] = 1; //"Multi_In"
		kctl_ul1->put(kctl_ul1, ucontrol_ul1);
		ucontrol_earc->value.integer.value[0] = 1; //earc_enable
		kctl_earc->put(kctl_earc, ucontrol_earc);
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_BCK_SEL_MASK|MULTI_IN_LRCK_SEL_MASK|
			   MULTI_IN_DAT0_SEL_MASK|MULTI_IN_DAT1_SEL_MASK|
			   MULTI_IN_DAT2_SEL_MASK|MULTI_IN_DAT3_SEL_MASK,
			   MULTI_IN_BCK_SEL_EARC|MULTI_IN_LRCK_SEL_EARC|
			   MULTI_IN_DAT0_SEL_EARC|MULTI_IN_DAT1_SEL_EARC|
			   MULTI_IN_DAT2_SEL_EARC|MULTI_IN_DAT3_SEL_EARC);
	} else {
		ucontrol_ul1->value.enumerated.item[0] = 0; //"Spdif_In"
		kctl_ul1->put(kctl_ul1, ucontrol_ul1);
		ucontrol_earc->value.integer.value[0] = 0; //earc_disable
		kctl_earc->put(kctl_earc, ucontrol_earc);
		mt8532_afe_handle_spdif_in_port_change(afe,
					SPDIF_IN_PORT_NONE);
		regmap_update_bits(afe->regmap, AFE_LOOPBACK_CFG0,
			   MULTI_IN_BCK_SEL_MASK|MULTI_IN_LRCK_SEL_MASK|
			   MULTI_IN_DAT0_SEL_MASK|MULTI_IN_DAT1_SEL_MASK|
			   MULTI_IN_DAT2_SEL_MASK|MULTI_IN_DAT3_SEL_MASK,
			   MULTI_IN_BCK_SEL_SPLIN|MULTI_IN_LRCK_SEL_SPLIN|
			   MULTI_IN_DAT0_SEL_SPLIN|MULTI_IN_DAT1_SEL_SPLIN|
			   MULTI_IN_DAT2_SEL_SPLIN|MULTI_IN_DAT3_SEL_SPLIN);
	}

	mt8532_afe_disable_main_clk(afe);
	kfree(ucontrol_ul1);
	kfree(ucontrol_earc);

	return 0;
}

static int mt8532_afe_etdm_in_data_source_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int src = ETDM_IN_FROM_PAD;
	unsigned int val = 0;

	if (!strcmp(kcontrol->id.name, "ETDM_IN1_Data_Source_Select")) {
		//mt8532_afe_enable_main_clk(afe);
		regmap_read(afe->regmap, ETDM_COWORK_CON1, &val);
		//mt8532_afe_disable_main_clk(afe);

		val = val & ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_MASK;

		if (val == ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_PAD)
			src = ETDM_IN_FROM_PAD;
		else if (val == ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT1)
			src = ETDM_IN_FROM_ETDM_OUT1;
		else if (val == ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT2)
			src = ETDM_IN_FROM_ETDM_OUT2;
		else if (val == ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT3)
			src = ETDM_IN_FROM_ETDM_OUT3;
	} else if (!strcmp(kcontrol->id.name, "ETDM_IN2_Data_Source_Select")) {
		//mt8532_afe_enable_main_clk(afe);
		regmap_read(afe->regmap, ETDM_COWORK_CON3, &val);
		//mt8532_afe_disable_main_clk(afe);
		val = val & ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_MASK;

		if (val == ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_PAD)
			src = ETDM_IN_FROM_PAD;
		else if (val == ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT1)
			src = ETDM_IN_FROM_ETDM_OUT1;
		else if (val == ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT2)
			src = ETDM_IN_FROM_ETDM_OUT2;
		else if (val == ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT3)
			src = ETDM_IN_FROM_ETDM_OUT3;
	}

	ucontrol->value.integer.value[0] = src;

	return 0;
}

static int mt8532_afe_etdm_in_data_source_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int src;

	src = ucontrol->value.integer.value[0];
	//mt8532_afe_enable_main_clk(afe);

	if (!strcmp(kcontrol->id.name, "ETDM_IN1_Data_Source_Select")) {
		if (src == ETDM_IN_FROM_PAD)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON1,
				   ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_MASK |
				   ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_MASK,
				   ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_PAD |
				   ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_PAD);
		else if (src == ETDM_IN_FROM_ETDM_OUT1)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON1,
				   ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_MASK |
				   ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_MASK,
				   ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT1 |
				   ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_OUT1);
		else if (src == ETDM_IN_FROM_ETDM_OUT2)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON1,
				   ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_MASK |
				   ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_MASK,
				   ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT2 |
				   ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_OUT2);
		else if (src == ETDM_IN_FROM_ETDM_OUT3)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON1,
				   ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_MASK |
				   ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_MASK,
				   ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT3 |
				   ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_OUT3);
	} else if (!strcmp(kcontrol->id.name, "ETDM_IN2_Data_Source_Select")) {
		if (src == ETDM_IN_FROM_PAD)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_MASK |
				   ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_MASK,
				   ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_PAD |
				   ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_PAD);
		else if (src == ETDM_IN_FROM_ETDM_OUT1)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_MASK |
				   ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_MASK,
				   ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT1 |
				   ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_OUT1);
		else if (src == ETDM_IN_FROM_ETDM_OUT2)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_MASK |
				   ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_MASK,
				   ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT2 |
				   ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_OUT2);
		else if (src == ETDM_IN_FROM_ETDM_OUT3)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON3,
				   ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_MASK |
				   ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_MASK,
				   ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT3 |
				   ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_OUT3);
	}

	//mt8532_afe_disable_main_clk(afe);

	return 0;
}

static int mt8532_afe_etdm_in_slave_mode_src_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int src = ETDM_IN_SLAVE_FROM_SELF;
	unsigned int val = 0;

	if (!strcmp(kcontrol->id.name, "ETDM_IN1_Slave_Mode_Source_Select")) {
		mt8532_afe_enable_main_clk(afe);
		regmap_read(afe->regmap, ETDM_COWORK_CON1, &val);
		mt8532_afe_disable_main_clk(afe);
		val = val & ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_MASK;

		if (val == ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_SELF)
			src = ETDM_IN_SLAVE_FROM_SELF;
		else if (val == ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT1_MAS)
			src = ETDM_IN_SLAVE_FROM_ETDM_OUT1;
		else if (val == ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT2_MAS)
			src = ETDM_IN_SLAVE_FROM_ETDM_OUT2;
		else if (val == ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT3_MAS)
			src = ETDM_IN_SLAVE_FROM_ETDM_OUT3;
	} else if (!strcmp(kcontrol->id.name,
			   "ETDM_IN2_Slave_Mode_Source_Select")) {
		mt8532_afe_enable_main_clk(afe);
		regmap_read(afe->regmap, ETDM_COWORK_CON2, &val);
		mt8532_afe_disable_main_clk(afe);
		val = val & ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_MASK;

		if (val == ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_SELF)
			src = ETDM_IN_SLAVE_FROM_SELF;
		else if (val == ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT1_MAS)
			src = ETDM_IN_SLAVE_FROM_ETDM_OUT1;
		else if (val == ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT2_MAS)
			src = ETDM_IN_SLAVE_FROM_ETDM_OUT2;
		else if (val == ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT3_MAS)
			src = ETDM_IN_SLAVE_FROM_ETDM_OUT3;
	}

	ucontrol->value.integer.value[0] = src;

	return 0;
}

static int mt8532_afe_etdm_in_slave_mode_src_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int src;

	src = ucontrol->value.integer.value[0];
	mt8532_afe_enable_main_clk(afe);

	if (!strcmp(kcontrol->id.name, "ETDM_IN1_Slave_Mode_Source_Select")) {
		if (src == ETDM_IN_SLAVE_FROM_SELF)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON1,
				   ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_MASK,
				   ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_SELF);
		else if (src == ETDM_IN_SLAVE_FROM_ETDM_OUT1)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON1,
				   ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_MASK,
				   ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT1_MAS);
		else if (src == ETDM_IN_SLAVE_FROM_ETDM_OUT2)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON1,
				   ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_MASK,
				   ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT2_MAS);
		else if (src == ETDM_IN_SLAVE_FROM_ETDM_OUT3)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON1,
				   ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_MASK,
				   ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT3_MAS);
	} else if (!strcmp(kcontrol->id.name,
			"ETDM_IN2_Slave_Mode_Source_Select")) {
		if (src == ETDM_IN_SLAVE_FROM_SELF)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON2,
				   ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_MASK,
				   ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_SELF);
		else if (src == ETDM_IN_SLAVE_FROM_ETDM_OUT1)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON2,
				   ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_MASK,
				   ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT1_MAS);
		else if (src == ETDM_IN_SLAVE_FROM_ETDM_OUT2)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON2,
				   ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_MASK,
				   ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT2_MAS);
		else if (src == ETDM_IN_SLAVE_FROM_ETDM_OUT3)
			regmap_update_bits(afe->regmap, ETDM_COWORK_CON2,
				   ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_MASK,
				   ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT3_MAS);
	}
	mt8532_afe_disable_main_clk(afe);
	return 0;
}
#if 0
static int mt8532_afe_pcm_loopback_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, PCM_INTF_CON2, &val);

	mt8532_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] = (val & PCM_INTF_CON2_LPBK_EN);

	return 0;
}

static int mt8532_afe_pcm_loopback_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt8532_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, PCM_INTF_CON2,
				   PCM_INTF_CON2_LPBK_EN,
				   PCM_INTF_CON2_LPBK_EN);
	else
		regmap_update_bits(afe->regmap, PCM_INTF_CON2,
				   PCM_INTF_CON2_LPBK_EN,
				   0x0);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}
#endif

#if 0
static int mt8532_afe_dmic_sinegen_enable_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_DMIC0_UL_SRC_CON1, &val);

	mt8532_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] =
		(val & DMIC_UL_CON1_SGEN_EN) ? 1 : 0;

	return 0;
}

static int mt8532_afe_dmic_sinegen_enable_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val_cfg = 0;
	unsigned int val_en = 0;
	unsigned int read_val = 0;
	unsigned int regs[] = {
		AFE_DMIC0_UL_SRC_CON1,
		AFE_DMIC1_UL_SRC_CON1,
		AFE_DMIC2_UL_SRC_CON1,
		AFE_DMIC3_UL_SRC_CON1,
	};
	int i;

	mt8532_afe_enable_main_clk(afe);
	val_cfg |= DMIC_UL_CON1_SGEN_CH2_AMP_DIV(6) |
		   DMIC_UL_CON1_SGEN_CH2_FREQ_DIV(1) |
		   DMIC_UL_CON1_SGEN_CH1_AMP_DIV(6) |
		   DMIC_UL_CON1_SGEN_CH1_FREQ_DIV(1);

	regmap_read(afe->regmap, AFE_DMIC0_UL_SRC_CON0, &read_val);

	switch (read_val & (0x7 << 17)) {
	case DMIC_UL_CON0_VOCIE_MODE_8K:
		val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(0) |
			   DMIC_UL_CON1_SGEN_CH1_SINE_MODE(0);
		break;
	case DMIC_UL_CON0_VOCIE_MODE_16K:
		val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(3) |
			   DMIC_UL_CON1_SGEN_CH1_SINE_MODE(3);
		break;
	case DMIC_UL_CON0_VOCIE_MODE_32K:
		val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(6) |
			   DMIC_UL_CON1_SGEN_CH1_SINE_MODE(6);
		break;
	case DMIC_UL_CON0_VOCIE_MODE_48K:
		val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(8) |
			   DMIC_UL_CON1_SGEN_CH1_SINE_MODE(8);
		break;
	default:
		val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(0) |
			   DMIC_UL_CON1_SGEN_CH1_SINE_MODE(0);
		break;
	}

	val_en |= DMIC_UL_CON1_SGEN_EN;

	if (ucontrol->value.integer.value[0]) {
		for (i = 0; i < ARRAY_SIZE(regs); i++)
			regmap_update_bits(afe->regmap, regs[i],
					   val_cfg | val_en,
					   val_cfg | val_en);
	} else {
		for (i = 0; i < ARRAY_SIZE(regs); i++)
			regmap_update_bits(afe->regmap, regs[i],
					   val_en,
					   0x0);
	}

	mt8532_afe_disable_main_clk(afe);

	return 0;
}

static int mt8532_afe_dmic_rampgen_enable_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, DMIC_TOP_CON, &val);

	mt8532_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] =
		(val & DMIC_TOP_CON_DMIC_SGEN_RAMPGEN_EN) ? 1 : 0;

	return 0;
}

static int mt8532_afe_dmic_rampgen_enable_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val_cfg = 0;
	unsigned int val_en = 0;
	unsigned int regs[] = {
		DMIC_TOP_CON,
		DMIC2_TOP_CON,
		DMIC3_TOP_CON,
		DMIC4_TOP_CON,
	};
	int i;

	mt8532_afe_enable_main_clk(afe);

	val_cfg |= DMIC_TOP_CON_DMIC_SGEN_RAMPGEN_FREQ_DIV(0x1F);

	val_en |= DMIC_TOP_CON_DMIC_RAMPGEN_LON |
		  DMIC_TOP_CON_DMIC_RAMPGEN_RON |
		  DMIC_TOP_CON_DMIC_SGEN_RAMPGEN_EN;

	if (ucontrol->value.integer.value[0]) {
		for (i = 0; i < ARRAY_SIZE(regs); i++)
			regmap_update_bits(afe->regmap, regs[i],
					   val_cfg | val_en,
					   val_cfg | val_en);
	} else {
		for (i = 0; i < ARRAY_SIZE(regs); i++)
			regmap_update_bits(afe->regmap, regs[i],
					   val_en,
					   0x0);
	}

	mt8532_afe_disable_main_clk(afe);

	return 0;
}
#endif
#if 0
static int mt8532_afe_gasrc_in_sinegen_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON1, &val);

	mt8532_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] =
		(val & AFE_SINEGEN_CON1_GASRC_IN_SGEN);

	return 0;
}

static int mt8532_afe_gasrc_in_sinegen_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt8532_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON1,
				   AFE_SINEGEN_CON1_GASRC_IN_SGEN,
				   AFE_SINEGEN_CON1_GASRC_IN_SGEN);
	else
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON1,
				   AFE_SINEGEN_CON1_GASRC_IN_SGEN,
				   0x0);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}

static int mt8532_afe_gasrc_out_sinegen_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_SINEGEN_CON1, &val);

	mt8532_afe_disable_main_clk(afe);

	ucontrol->value.integer.value[0] =
		(val & AFE_SINEGEN_CON1_GASRC_OUT_SGEN);

	return 0;
}

static int mt8532_afe_gasrc_out_sinegen_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);

	mt8532_afe_enable_main_clk(afe);

	if (ucontrol->value.integer.value[0])
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON1,
				   AFE_SINEGEN_CON1_GASRC_OUT_SGEN,
				   AFE_SINEGEN_CON1_GASRC_OUT_SGEN);
	else
		regmap_update_bits(afe->regmap, AFE_SINEGEN_CON1,
				   AFE_SINEGEN_CON1_GASRC_OUT_SGEN,
				   0x0);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}

static int get_tdmout_conn_reg_shift(char *name,
	unsigned int *reg, unsigned int *shift)
{
	int ret = 0;

	if (!strcmp(name, "TDMOUT_O00_Source_Select")) {
		*reg = AFE_TDMOUT_CONN0;
		*shift = 0;
	} else if (!strcmp(name, "TDMOUT_O01_Source_Select")) {
		*reg = AFE_TDMOUT_CONN0;
		*shift = 4;
	} else if (!strcmp(name, "TDMOUT_O02_Source_Select")) {
		*reg = AFE_TDMOUT_CONN0;
		*shift = 8;
	} else if (!strcmp(name, "TDMOUT_O03_Source_Select")) {
		*reg = AFE_TDMOUT_CONN0;
		*shift = 12;
	} else if (!strcmp(name, "TDMOUT_O04_Source_Select")) {
		*reg = AFE_TDMOUT_CONN0;
		*shift = 16;
	} else if (!strcmp(name, "TDMOUT_O05_Source_Select")) {
		*reg = AFE_TDMOUT_CONN0;
		*shift = 20;
	} else if (!strcmp(name, "TDMOUT_O06_Source_Select")) {
		*reg = AFE_TDMOUT_CONN0;
		*shift = 24;
	} else if (!strcmp(name, "TDMOUT_O07_Source_Select")) {
		*reg = AFE_TDMOUT_CONN0;
		*shift = 28;
	} else if (!strcmp(name, "TDMOUT_O08_Source_Select")) {
		*reg = AFE_TDMOUT_CONN1;
		*shift = 0;
	} else if (!strcmp(name, "TDMOUT_O09_Source_Select")) {
		*reg = AFE_TDMOUT_CONN1;
		*shift = 4;
	} else if (!strcmp(name, "TDMOUT_O10_Source_Select")) {
		*reg = AFE_TDMOUT_CONN1;
		*shift = 8;
	} else if (!strcmp(name, "TDMOUT_O11_Source_Select")) {
		*reg = AFE_TDMOUT_CONN1;
		*shift = 12;
	} else if (!strcmp(name, "TDMOUT_O12_Source_Select")) {
		*reg = AFE_TDMOUT_CONN1;
		*shift = 16;
	} else if (!strcmp(name, "TDMOUT_O13_Source_Select")) {
		*reg = AFE_TDMOUT_CONN1;
		*shift = 20;
	} else if (!strcmp(name, "TDMOUT_O14_Source_Select")) {
		*reg = AFE_TDMOUT_CONN1;
		*shift = 24;
	} else if (!strcmp(name, "TDMOUT_O15_Source_Select")) {
		*reg = AFE_TDMOUT_CONN1;
		*shift = 28;
	} else {
		ret = -1;
	}

	return ret;
}

static int mt8532_afe_tdm_conn_source_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int src;
	unsigned int reg;
	unsigned int shift;
	unsigned int val;
	int ret;

	ret = get_tdmout_conn_reg_shift(kcontrol->id.name, &reg, &shift);
	if (ret)
		return ret;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, reg, &val);

	mt8532_afe_disable_main_clk(afe);

	src = (val >> shift) & 0xf;

	ucontrol->value.integer.value[0] = src;

	return 0;
}

static int mt8532_afe_tdm_conn_source_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int src;
	unsigned int reg;
	unsigned int shift;
	int ret;

	src = ucontrol->value.integer.value[0];

	ret = get_tdmout_conn_reg_shift(kcontrol->id.name, &reg, &shift);
	if (ret)
		return ret;

	mt8532_afe_enable_main_clk(afe);

	regmap_update_bits(afe->regmap, reg, 0xf << shift, src << shift);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}

static int mt8532_afe_spdif_output_iec61937_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_control_data *data = &afe_priv->ctrl_data;

	ucontrol->value.integer.value[0] = data->spdif_output_iec61937;

	return 0;
}

static int mt8532_afe_spdif_output_iec61937_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_control_data *data = &afe_priv->ctrl_data;

	data->spdif_output_iec61937 = (ucontrol->value.integer.value[0]) ?
				 true : false;

	dev_dbg(afe->dev, "%s iec61937 %d\n", __func__,
		data->spdif_output_iec61937);

	return 0;
}
#endif

static int mt8532_afe_spdif_in_port_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	ucontrol->value.integer.value[0] = afe_priv->spdif_in_data.port;

	return 0;
}

static int mt8532_afe_spdif_in_port_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int port = ucontrol->value.integer.value[0];

	dev_dbg(afe->dev, "%s port %u\n", __func__, port);

	mt8532_afe_handle_spdif_in_port_change(afe, port);

	return 0;
}

static int mt8532_afe_spdif_in_chs_data_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = 24;
	return 0;
}

static int mt8532_afe_spdif_in_chs_data_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;
	int i;

	if (spdif_in->port != SPDIF_IN_PORT_NONE) {
		mt8532_afe_enable_main_clk(afe);

		for (i = 0; i < SPDIF_CHSTS_NUM; i++)
			regmap_read(afe->regmap,
				    AFE_SPDIFIN_CHSTS1 + i * 4,
				    &spdif_in->subdata.ch_status[i]);

		mt8532_afe_disable_main_clk(afe);
	}

	memcpy((void *)ucontrol->value.bytes.data,
	       spdif_in->subdata.ch_status,
	       sizeof(spdif_in->subdata.ch_status));

	return 0;
}

static int mt8532_afe_spdif_in_rate_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	return 0;
}

static int mt8532_afe_spdif_in_rate_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;

	ucontrol->value.integer.value[0] = spdif_in->subdata.rate;

	return 0;
}

static int mt8532_afe_spdif_in_iec958_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 1;
	return 0;
}

static int mt8532_afe_spdif_in_iec958_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;
	int i;

	if (spdif_in->port != SPDIF_IN_PORT_NONE) {
		mt8532_afe_enable_main_clk(afe);

		for (i = 0; i < 6; i++)
			regmap_read(afe->regmap,
				    AFE_SPDIFIN_CHSTS1 + i * 4,
				    &spdif_in->subdata.ch_status[i]);

		mt8532_afe_disable_main_clk(afe);
	} else {
		memset((void *)spdif_in->subdata.ch_status,
		       0xff, sizeof(spdif_in->subdata.ch_status));
	}

	memcpy((void *)ucontrol->value.iec958.status,
	       spdif_in->subdata.ch_status,
	       sizeof(spdif_in->subdata.ch_status));

	return 0;
}

static int mt8532_afe_spdif_in_stream_type_info(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	return 0;
}

static int mt8532_afe_spdif_in_stream_type_get(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);
	regmap_read(afe->regmap, AFE_MPHONE_MULTI_MON, &val);
	mt8532_afe_disable_main_clk(afe);

	if (val & AFE_MPHONE_MULTI_MON_DEC)
		val = val & AFE_MPHONE_MULTI_MON_ROUGH;
	else
		val = 0xff;

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int mt8532_afe_spdif_in_user_code_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = SPDIF_USERCODE_NUM;
	return 0;
}

static int mt8532_afe_spdif_in_user_code_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;
	int i;

	if (spdif_in->port != SPDIF_IN_PORT_NONE) {
		mt8532_afe_enable_main_clk(afe);
		for (i = 0; i < SPDIF_USERCODE_NUM; i++)
			regmap_read(afe->regmap,
				    SPDIFIN_USERCODE1 + i * 4,
				    &spdif_in->subdata.user_code[i]);

		mt8532_afe_disable_main_clk(afe);
	}

	memcpy((void *)ucontrol->value.bytes.data,
	       spdif_in->subdata.user_code,
	       sizeof(spdif_in->subdata.user_code));

	return 0;
}

static int mt8532_afe_spdif_in_information_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = sizeof(struct mt8532_spdif_in_subdata) /
			sizeof(unsigned int);

	return 0;
}

static int mt8532_afe_spdif_in_information_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;
	int i, index;

	index = 0;
	ucontrol->value.integer.value[index++] = spdif_in->subdata.rate;

	for (i = 0; i < SPDIF_USERCODE_NUM; i++)
		ucontrol->value.integer.value[i + (index++)] =
			spdif_in->subdata.user_code[i];

	for (i = 0; i < SPDIF_CHSTS_NUM; i++)
		ucontrol->value.integer.value[i + (index++)] =
			spdif_in->subdata.ch_status[i];

	ucontrol->value.integer.value[index++] = spdif_in->subdata.stream_type;

	return 0;
}

static int mt8532_afe_multi_in_information_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = sizeof(struct mt8532_multi_in_mon_info) /
		sizeof(unsigned int);

	return 0;
}

static int mt8532_afe_multi_in_information_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_multi_in_mon_info *mon_info =
		&priv->multi_in_mon_info;
	int index = 0;

	ucontrol->value.integer.value[index++] = mon_info->rough_type;
	ucontrol->value.integer.value[index++] = mon_info->data_type;
	ucontrol->value.integer.value[index++] = mon_info->bitstream_number;
	ucontrol->value.integer.value[index++] = mon_info->pc_b12_to_b5;

	return 0;
}

static int mt8532_afe_arc_in_format_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	static const char * const texts[5] =
		{"Unlock", "LPCM", "AC3", "EAC3", "Unknown"};

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 5;
	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;
	strncpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item],
		sizeof(uinfo->value.enumerated.name));
	return 0;
}

static int mt8532_afe_arc_in_format_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &priv->spdif_in_data;
	struct mt8532_multi_in_mon_info *mon_info =
		&priv->multi_in_mon_info;

	if (spdif_in->subdata.rate == 0) {
		ucontrol->value.enumerated.item[0] = 0; //Unlock
	} else {
		ucontrol->value.enumerated.item[0] = 4; //Unknown
		if (mon_info->rough_type == 0) {
			ucontrol->value.enumerated.item[0] = 1; //LPCM
		} else if (mon_info->rough_type == 1) {
			if (mon_info->data_type == 1)
				ucontrol->value.enumerated.item[0] = 2; //AC3
			else if (mon_info->data_type == 21)
				ucontrol->value.enumerated.item[0] = 3; //EAC3
		}
	}
	return 0;
}

static int mt8532_afe_arc_in_rate_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;

	return 0;
}

static int mt8532_afe_arc_in_rate_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;

	ucontrol->value.integer.value[0] = spdif_in->subdata.rate;

	return 0;
}

static int mt8532_afe_arc_in_ch_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	static const char * const texts[3] =
		{"Unlock", "1ch", "2ch"};

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 3;
	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;
	strncpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item],
		sizeof(uinfo->value.enumerated.name));
	return 0;
}

static int mt8532_afe_arc_in_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
		struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;

	if (spdif_in->subdata.rate > 0)
		ucontrol->value.enumerated.item[0] = 2; //ARC input is 2ch
	else
		ucontrol->value.enumerated.item[0] = 0; //ARC input is unlock
	return 0;
}

#if 0
static int mt8532_apll1_clk_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct clk *clk;

	clk = priv->clocks[MT8532_CLK_APMIXED_APLL1];

	ucontrol->value.integer.value[0] = clk_get_rate(clk);
	return 0;
}

static int mt8532_apll1_clk_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct clk *clk;
	unsigned long rate, min;

	rate = ucontrol->value.integer.value[0];

	min = MT8532_APLL1_RATE - MT8532_APLL_ADJUST_RANGE;

	rate = max(rate, min);

	dev_dbg(afe->dev, "%s rate %lu\n", __func__, rate);

	clk = priv->clocks[MT8532_CLK_APMIXED_APLL1];

	clk_set_rate(clk, rate);

	return 0;
}

static int mt8532_apll2_clk_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct clk *clk;

	clk = priv->clocks[MT8532_CLK_APMIXED_APLL2];

	ucontrol->value.integer.value[0] = clk_get_rate(clk);
	return 0;
}

static int mt8532_apll2_clk_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *plat = snd_soc_kcontrol_platform(kcontrol);
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(plat);
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct clk *clk;
	unsigned long rate, min;

	rate = ucontrol->value.integer.value[0];

	min = MT8532_APLL2_RATE - MT8532_APLL_ADJUST_RANGE;

	rate = max(rate, min);

	dev_dbg(afe->dev, "%s rate %lu\n", __func__, rate);

	clk = priv->clocks[MT8532_CLK_APMIXED_APLL2];

	clk_set_rate(clk, rate);

	return 0;
}
#endif

#define SND_SOC_CTL_RO(xname, xhandler_info, xhandler_get) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_READ | \
		  SNDRV_CTL_ELEM_ACCESS_VOLATILE, \
	.info = xhandler_info, .get = xhandler_get }

static const struct snd_kcontrol_new mt8532_afe_controls[] = {
#if 1
	SOC_ENUM_EXT("MULTI_IN_Data_Source_Select",
		     mt8532_afe_soc_enums[CTRL_MULTI_IN_DATA_SRC],
		     mt8532_afe_multi_in_data_source_get,
		     mt8532_afe_multi_in_data_source_put),
	SOC_ENUM_EXT("TV_Input_Path_Select",
		     mt8532_afe_soc_enums[CTRL_TV_INPUT_PATH_SEL],
		     mt8532_afe_tv_input_path_sel_get,
		     mt8532_afe_tv_input_path_sel_put),
	SOC_ENUM("Multi_In_Bck_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_BCK_SEL]),
	SOC_ENUM("Multi_In_Lrck_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_LRCK_SEL]),
	SOC_ENUM("Multi_In_Sdata0_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_SDATA0_SEL]),
	SOC_ENUM("Multi_In_Sdata1_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_SDATA1_SEL]),
	SOC_ENUM("Multi_In_Sdata2_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_SDATA2_SEL]),
	SOC_ENUM("Multi_In_Sdata3_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_SDATA3_SEL]),

	SOC_SINGLE_BOOL_EXT("SineGen_Enable_Switch",
			    0,
			    mt8532_afe_singen_enable_get,
			    mt8532_afe_singen_enable_put),
#endif
#if 0
	SOC_ENUM_EXT("SineGen_Loopback_Mode_Select",
		     mt8532_afe_soc_enums[CTRL_SINEGEN_LOOPBACK_MODE],
		     mt8532_afe_sinegen_loopback_mode_get,
		     mt8532_afe_sinegen_loopback_mode_put),
#endif
#if 1
	SOC_ENUM_EXT("SineGen_Timing_Select",
		     mt8532_afe_soc_enums[CTRL_SINEGEN_TIMING],
		     mt8532_afe_sinegen_timing_get,
		     mt8532_afe_sinegen_timing_put),

	SOC_SINGLE_RANGE("SineGen_Amp_Div_Ch1",
			 AFE_SINEGEN_CON0, 5, 0, 7, 0),
	SOC_SINGLE_RANGE("SineGen_Amp_Div_Ch2",
			 AFE_SINEGEN_CON0, 17, 0, 7, 0),
	SOC_SINGLE_RANGE("SineGen_Freq_Div_Ch1",
			 AFE_SINEGEN_CON0, 0, 0, 31, 0),
	SOC_SINGLE_RANGE("SineGen_Freq_Div_Ch2",
			 AFE_SINEGEN_CON0, 12, 0, 31, 0),

	SOC_SINGLE_BOOL_EXT("Mini_Clk_En",
			    0,
			    mt8532_afe_mini_clk_en_get,
			    mt8532_afe_mini_clk_en_put),

	SOC_SINGLE_BOOL_EXT("ETDM_OUT1_SineGen_Select",
			    0,
			    mt8532_afe_tdm_out1_sinegen_get,
			    mt8532_afe_tdm_out1_sinegen_put),
	SOC_SINGLE_BOOL_EXT("ETDM_OUT2_SineGen_Select",
					0,
					mt8532_afe_tdm_out2_sinegen_get,
					mt8532_afe_tdm_out2_sinegen_put),
	SOC_SINGLE_BOOL_EXT("ETDM_OUT3_SineGen_Select",
					0,
					mt8532_afe_tdm_out3_sinegen_get,
					mt8532_afe_tdm_out3_sinegen_put),


	SOC_SINGLE_BOOL_EXT("UL8_SineGen_Select",
			    0,
			    mt8532_afe_ul8_sinegen_get,
			    mt8532_afe_ul8_sinegen_put),
#endif

	SOC_ENUM_EXT("ETDM_IN1_Data_Source_Select",
		     mt8532_afe_soc_enums[CTRL_ETDM_IN_DATA_SRC],
		     mt8532_afe_etdm_in_data_source_get,
		     mt8532_afe_etdm_in_data_source_put),
	SOC_ENUM_EXT("ETDM_IN1_Slave_Mode_Source_Select",
		     mt8532_afe_soc_enums[CTRL_ETDM_IN_SLAVE_MODE_SRC],
		     mt8532_afe_etdm_in_slave_mode_src_get,
		     mt8532_afe_etdm_in_slave_mode_src_put),

	SOC_ENUM_EXT("ETDM_IN2_Data_Source_Select",
		     mt8532_afe_soc_enums[CTRL_ETDM_IN_DATA_SRC],
		     mt8532_afe_etdm_in_data_source_get,
		     mt8532_afe_etdm_in_data_source_put),
	SOC_ENUM_EXT("ETDM_IN2_Slave_Mode_Source_Select",
		     mt8532_afe_soc_enums[CTRL_ETDM_IN_SLAVE_MODE_SRC],
		     mt8532_afe_etdm_in_slave_mode_src_get,
		     mt8532_afe_etdm_in_slave_mode_src_put),
#if 0
	SOC_SINGLE_BOOL_EXT("PCM_Tx2Rx_Loopback_Switch",
			    0,
			    mt8532_afe_pcm_loopback_get,
			    mt8532_afe_pcm_loopback_put),
	SOC_SINGLE_BOOL_EXT("DMIC_SineGen_Enable_Switch",
		     0,
		     mt8532_afe_dmic_sinegen_enable_get,
		     mt8532_afe_dmic_sinegen_enable_put),
#if 0
	SOC_SINGLE_BOOL_EXT("DMIC_RampGen_Enable_Switch",
		     0,
		     mt8532_afe_dmic_rampgen_enable_get,
		     mt8532_afe_dmic_rampgen_enable_put),
#endif
	SOC_SINGLE_BOOL_EXT("GASRC_In_SineGen_Select",
			    0,
			    mt8532_afe_gasrc_in_sinegen_get,
			    mt8532_afe_gasrc_in_sinegen_put),
	SOC_SINGLE_BOOL_EXT("GASRC_Out_SineGen_Select",
			    0,
			    mt8532_afe_gasrc_out_sinegen_get,
			    mt8532_afe_gasrc_out_sinegen_put),
	SOC_ENUM_EXT("TDMOUT_O00_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O01_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O02_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O03_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O04_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O05_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O06_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O07_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O08_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O09_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O10_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O11_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O12_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O13_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O14_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_ENUM_EXT("TDMOUT_O15_Source_Select",
		     mt8532_afe_soc_enums[CTRL_TDMOUT_CONN_SRC],
		     mt8532_afe_tdm_conn_source_get,
		     mt8532_afe_tdm_conn_source_put),
	SOC_SINGLE_BOOL_EXT("Spdif_Output_IEC61937_Switch",
			    0,
			    mt8532_afe_spdif_output_iec61937_get,
			    mt8532_afe_spdif_output_iec61937_put),
	SOC_ENUM("Multi_In_Bck_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_BCK_SEL]),
	SOC_ENUM("Multi_In_Lrck_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_LRCK_SEL]),
	SOC_ENUM("Multi_In_Sdata0_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_SDATA0_SEL]),
	SOC_ENUM("Multi_In_Sdata1_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_SDATA1_SEL]),
	SOC_ENUM("Multi_In_Sdata2_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_SDATA2_SEL]),
	SOC_ENUM("Multi_In_Sdata3_Select",
		 mt8532_afe_soc_enums[CTRL_MULTI_IN_SDATA3_SEL]),
#endif

#if 1
	SOC_ENUM_EXT("Spdif_In_Port_Select",
		     mt8532_afe_soc_enums[CTRL_SPDIF_IN_PORT_SEL],
		     mt8532_afe_spdif_in_port_get,
		     mt8532_afe_spdif_in_port_put),
	SND_SOC_CTL_RO("Spdif_In_Channel_Status_Data",
		       mt8532_afe_spdif_in_chs_data_info,
		       mt8532_afe_spdif_in_chs_data_get),
	SND_SOC_CTL_RO(SNDRV_CTL_NAME_IEC958("", CAPTURE, DEFAULT),
		       mt8532_afe_spdif_in_iec958_info,
		       mt8532_afe_spdif_in_iec958_get),
	SND_SOC_CTL_RO("Spdif_In_Rate",
		       mt8532_afe_spdif_in_rate_info,
		       mt8532_afe_spdif_in_rate_get),
	SND_SOC_CTL_RO("Spdif_In_Stream_Type",
			mt8532_afe_spdif_in_stream_type_info,
			mt8532_afe_spdif_in_stream_type_get),
	SND_SOC_CTL_RO("Spdif_In_User_Code",
			mt8532_afe_spdif_in_user_code_info,
			mt8532_afe_spdif_in_user_code_get),
	SND_SOC_CTL_RO("Spdif_In_Info",
			mt8532_afe_spdif_in_information_info,
			mt8532_afe_spdif_in_information_get),
	SND_SOC_CTL_RO("Multi_In_Info",
			mt8532_afe_multi_in_information_info,
			mt8532_afe_multi_in_information_get),
	SND_SOC_CTL_RO("ARC_In_Format",
			mt8532_afe_arc_in_format_info,
			mt8532_afe_arc_in_format_get),
	SND_SOC_CTL_RO("ARC_In_Rate",
			mt8532_afe_arc_in_rate_info,
			mt8532_afe_arc_in_rate_get),
	SND_SOC_CTL_RO("ARC_In_Channel",
			mt8532_afe_arc_in_ch_info,
			mt8532_afe_arc_in_ch_get),
#endif
#if 0
	SOC_SINGLE("Spdif_Out_to_Spdif_In_Loopback_Switch",
		   AFE_SPDIFIN_CFG1, 14, 1, 0),
	SOC_SINGLE_EXT("Apll1_Clk", 0, 0,
		MT8532_APLL1_RATE + MT8532_APLL_ADJUST_RANGE, 0,
		mt8532_apll1_clk_get, mt8532_apll1_clk_set),
	SOC_SINGLE_EXT("Apll2_Clk", 0, 0,
		MT8532_APLL2_RATE + MT8532_APLL_ADJUST_RANGE, 0,
		mt8532_apll2_clk_get, mt8532_apll2_clk_set),
	SOC_ENUM_EXT("DL2_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
#endif

#if 1
	SOC_ENUM_EXT("DL3_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("DL6_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("DL7_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("DL10_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("DLM_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL1_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL2_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL3_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL4_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL5_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL6_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL8_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL9_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("UL10_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
#endif

#if 1
	SOC_ENUM_EXT("GARC0_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC1_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC2_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC3_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC4_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC5_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC6_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC7_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC8_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC9_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC10_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC11_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC12_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC13_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC14_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC15_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC16_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC17_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC18_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("GARC19_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("ETDM1_OUT_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("ETDM1_IN_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("ETDM2_OUT_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("ETDM2_IN_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("ETDM3_OUT_Apll_Sel",
		     mt8532_afe_soc_enums[CTRL_ENGENSYS_DOMAIN],
		     mt8532_afe_engen_apll_get,
		     mt8532_afe_engen_apll_put),
	SOC_ENUM_EXT("ETDM1_OUT_MOD_Sel",
				 mt8532_afe_soc_enums[CTRL_ETDM_MOD],
				 mt8532_afe_etdm_mod_get,
				 mt8532_afe_etdm_mod_put),
	SOC_ENUM_EXT("ETDM3_OUT_MOD_Sel",
					 mt8532_afe_soc_enums[CTRL_ETDM_MOD],
					 mt8532_afe_etdm_mod_get,
					 mt8532_afe_etdm_mod_put),
	SOC_ENUM_EXT("ETDM1_OUT_DSD_BIT_Sel",
				 mt8532_afe_soc_enums[CTRL_ETDM_DSD_BIT],
				 mt8532_afe_etdm_dsd_bit_get,
				 mt8532_afe_etdm_dsd_bit_put),
	SOC_ENUM_EXT("ETDM3_OUT_DSD_BIT_Sel",
				mt8532_afe_soc_enums[CTRL_ETDM_DSD_BIT],
				mt8532_afe_etdm_dsd_bit_get,
				mt8532_afe_etdm_dsd_bit_put),
	SOC_ENUM_EXT("ETDM1_OUT_DSD_CLK_Sel",
				 mt8532_afe_soc_enums[CTRL_ETDM_DSD_CLK],
				 mt8532_afe_etdm_dsd_clk_get,
				 mt8532_afe_etdm_dsd_clk_put),
	SOC_ENUM_EXT("ETDM3_OUT_DSD_CLK_Sel",
				mt8532_afe_soc_enums[CTRL_ETDM_DSD_CLK],
				mt8532_afe_etdm_dsd_clk_get,
				mt8532_afe_etdm_dsd_clk_put),
#endif
};


int mt8532_afe_add_controls(struct snd_soc_platform *platform)
{
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	afe_priv->card = platform->component.card->snd_card;

	return snd_soc_add_platform_controls(platform, mt8532_afe_controls,
					     ARRAY_SIZE(mt8532_afe_controls));
}

