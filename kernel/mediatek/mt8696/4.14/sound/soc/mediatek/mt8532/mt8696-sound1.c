/*
 * mt8696-sound1.c  --  MT8696 sound1 machine driver
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

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "mt8532-afe-common.h"
#include "mt8532-afe-utils.h"
#include "mt8532-snd-utils.h"

#define TEST_BACKEND_WITH_ENDPOINT

#define PREFIX	"mediatek,"
#define ENUM_TO_STR(enum) #enum
#define MAX_CODEC_CONF 32

enum PINCTRL_PIN_STATE {
	PIN_STATE_DEFAULT = 0,
	PIN_STATE_MAX
};

static const char * const mt8696_sound1_pin_str[PIN_STATE_MAX] = {
	"default",
};

enum {
	/* FE */
	DAI_LINK_FE_BASE = 0,
	DAI_LINK_FE_AFE_BASE = DAI_LINK_FE_BASE,
	DAI_LINK_FE_DLM_PLAYBACK = DAI_LINK_FE_AFE_BASE,
	//DAI_LINK_FE_DL11_PLAYBACK,
	DAI_LINK_FE_DL2_PLAYBACK/* = DAI_LINK_FE_AFE_BASE*/,
	//DAI_LINK_FE_DL3_PLAYBACK,
	//DAI_LINK_FE_DL6_PLAYBACK,
	//DAI_LINK_FE_DL7_PLAYBACK,
	DAI_LINK_FE_DL10_PLAYBACK,

	DAI_LINK_FE_UL1_CAPTURE,
	DAI_LINK_FE_UL2_CAPTURE,
	//DAI_LINK_FE_UL3_CAPTURE,
	DAI_LINK_FE_UL4_CAPTURE,
	//DAI_LINK_FE_UL5_CAPTURE,
	//DAI_LINK_FE_UL6_CAPTURE,
	//DAI_LINK_FE_UL8_CAPTURE,
	//DAI_LINK_FE_UL9_CAPTURE,
	//DAI_LINK_FE_UL10_CAPTURE,
	DAI_LINK_FE_AFE_END,
	DAI_LINK_FE_END = DAI_LINK_FE_AFE_END,
	/* BE */
	DAI_LINK_BE_BASE = DAI_LINK_FE_END,
	DAI_LINK_BE_AFE_BASE = DAI_LINK_BE_BASE,
	DAI_LINK_BE_ETDM1_OUT = DAI_LINK_BE_AFE_BASE,
	DAI_LINK_BE_ETDM1_IN,
	DAI_LINK_BE_ETDM2_OUT,  //= DAI_LINK_BE_AFE_BASE
	DAI_LINK_BE_ETDM2_IN,
	DAI_LINK_BE_ETDM3_OUT,
	//DAI_LINK_BE_PCM_INTF,
	//DAI_LINK_BE_VIRTUAL_DL_SOURCE,
	//DAI_LINK_BE_DMIC,
	DAI_LINK_BE_GASRC0,
	DAI_LINK_BE_GASRC1,
	DAI_LINK_BE_GASRC2,
	DAI_LINK_BE_GASRC3,

	//DAI_LINK_BE_SPDIF_OUT,
	DAI_LINK_BE_SPDIF_IN,
	DAI_LINK_BE_MULTI_IN,
	DAI_LINK_BE_VIRTUAL,
	DAI_LINK_BE_AFE_END,
	DAI_LINK_BE_END = DAI_LINK_BE_AFE_END,
	DAI_LINK_NUM = DAI_LINK_BE_END,
	DAI_LINK_FE_NUM = DAI_LINK_FE_END - DAI_LINK_FE_BASE,
	DAI_LINK_BE_NUM = DAI_LINK_BE_END - DAI_LINK_BE_BASE,
};

struct mt8696_sound1_codec_pll_clk_data {
	unsigned int pll_id;
	unsigned int src_id;
	unsigned int clk_multp;
	unsigned int freq_in;
	unsigned int freq_out;
};

struct mt8696_sound1_codec_dai_data {
	const char *dai_name;
	bool set_pll_clk_in_hw_params;
	struct mt8696_sound1_codec_pll_clk_data pll_clk;
};

struct mt8696_sound1_be_ctrl_data {
	unsigned int mck_multp;
	unsigned int lrck_width;
	unsigned int fix_rate;
	unsigned int fix_channels;
	unsigned int fix_bit_width;
	unsigned int num_codec_dais;
	struct mt8696_sound1_codec_dai_data *codec_dai_data;
};

struct mt8696_sound1_priv {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_states[PIN_STATE_MAX];
	struct mt8696_sound1_be_ctrl_data be_data[DAI_LINK_BE_NUM];
	struct device_node *afe_plat_node;
	struct snd_soc_codec_conf codec_conf[MAX_CODEC_CONF];
	unsigned int num_codec_configs;
};

struct mt8532_dai_link_prop {
	char *name;
	unsigned int link_id;
};

static const struct snd_soc_dapm_widget mt8696_sound1_widgets[] = {
#ifdef TEST_BACKEND_WITH_ENDPOINT
	SND_SOC_DAPM_INPUT("MULTI In"),
	SND_SOC_DAPM_OUTPUT("ETDM3 Out"),
	SND_SOC_DAPM_INPUT("ETDM1 Out"),
	SND_SOC_DAPM_INPUT("ETDM1 In"),
	SND_SOC_DAPM_OUTPUT("ETDM2 Out"),
	SND_SOC_DAPM_INPUT("ETDM2 In"),
#endif
};

static const struct snd_soc_dapm_route mt8696_sound1_routes[] = {
#ifdef TEST_BACKEND_WITH_ENDPOINT
	{"MULTI Capture", NULL, "MULTI In"},
	{"ETDM3 Out", NULL, "ETDM3 Playback"},
	{"ETDM1 Out", NULL, "ETDM1 Playback"},
	{"ETDM1 Capture", NULL, "ETDM1 In"},
	{"ETDM2 Out", NULL, "ETDM2 Playback"},
	{"ETDM2 Capture", NULL, "ETDM2 In"},
#endif
	//{"DMIC Capture", NULL, "DMIC In"},

};

static int link_to_dai(int link_id)
{
	switch (link_id) {
	case DAI_LINK_BE_ETDM1_OUT:
		return MT8532_AFE_IO_ETDM1_OUT;
	case DAI_LINK_BE_ETDM1_IN:
		return MT8532_AFE_IO_ETDM1_IN;
	case DAI_LINK_BE_ETDM2_OUT:
		return MT8532_AFE_IO_ETDM2_OUT;
	case DAI_LINK_BE_ETDM2_IN:
		return MT8532_AFE_IO_ETDM2_IN;
	default:
		break;
	}
	return -1;
}

static int mt8696_sound1_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mt8696_sound1_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int id = rtd->dai_link->id;
	struct mt8696_sound1_be_ctrl_data *be;
	struct mt8696_sound1_codec_dai_data *codec_dai_data;
	struct mt8696_sound1_codec_pll_clk_data *pll_clk;
	struct snd_soc_dai *codec_dai;
	unsigned int mclk_multiplier = 0;
	unsigned int mclk = 0;
	unsigned int lrck_width = 0;
	int slot = 0;
	int slot_width = 0;
	unsigned int slot_bitmask = 0;
	unsigned int idx, i;
	int ret;

	if (id < DAI_LINK_BE_BASE || id >= DAI_LINK_BE_END)
		return -EINVAL;

	idx = id - DAI_LINK_BE_BASE;
	be = &priv->be_data[idx];

	mclk_multiplier = be->mck_multp;
	lrck_width = be->lrck_width;

	if (mclk_multiplier > 0) {
		mclk = mclk_multiplier * params_rate(params);

		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk,
					     SND_SOC_CLOCK_OUT);
		if (ret)
			return ret;
	}

	slot_width = lrck_width;
	if (slot_width > 0) {
		slot = params_channels(params);
		slot_bitmask = GENMASK(slot - 1, 0);

		ret = snd_soc_dai_set_tdm_slot(cpu_dai,
					       slot_bitmask,
					       slot_bitmask,
					       slot,
					       slot_width);
		if (ret)
			return ret;
	}

	for (i = 0; i < rtd->num_codecs; i++) {
		codec_dai = rtd->codec_dais[i];
		codec_dai_data = &be->codec_dai_data[i];

		if (codec_dai_data &&
		    codec_dai_data->set_pll_clk_in_hw_params &&
		    codec_dai) {
			unsigned int freq_in;
			unsigned int freq_out;

			pll_clk = &codec_dai_data->pll_clk;

			freq_in = pll_clk->freq_in;
			freq_out = pll_clk->freq_out;

			if ((freq_in == 0) || (freq_out == 0)) {
				freq_in = pll_clk->clk_multp *
					params_rate(params);
				freq_out = params_rate(params);
			}

			snd_soc_dai_set_pll(codec_dai,
				pll_clk->pll_id,
				pll_clk->src_id,
				freq_in, freq_out);
		}
	}

	return 0;
}

static int mt8696_sound1_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
	struct snd_pcm_hw_params *params)
{
	struct mt8696_sound1_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	int id = rtd->dai_link->id;
	struct mt8696_sound1_be_ctrl_data *be;
	unsigned int fix_rate = 0;
	unsigned int fix_bit_width = 0;
	unsigned int fix_channels = 0;
	unsigned int idx;

	if (id < DAI_LINK_BE_BASE || id >= DAI_LINK_BE_END)
		return -EINVAL;

	idx = id - DAI_LINK_BE_BASE;
	be = &priv->be_data[idx];

	fix_rate = be->fix_rate;
	fix_bit_width = be->fix_bit_width;
	fix_channels = be->fix_channels;

	if (fix_rate > 0) {
		struct snd_interval *rate =
			hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);

		rate->max = rate->min = fix_rate;
	}

	if (fix_bit_width > 0) {
		struct snd_mask *mask =
			hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);

		if (fix_bit_width == 32) {
			snd_mask_none(mask);
			snd_mask_set(mask, SNDRV_PCM_FORMAT_S32_LE);
		} else if (fix_bit_width == 16) {
			snd_mask_none(mask);
			snd_mask_set(mask, SNDRV_PCM_FORMAT_S16_LE);
		}
	}

	if (fix_channels > 0) {
		struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

		channels->min = channels->max = fix_channels;
	}

	return 0;
}

static struct snd_soc_ops mt8696_sound1_etdm_ops = {
	.hw_params = mt8696_sound1_hw_params,
};

#define RSV_DAI_LNIK(x) \
{ \
	.name = #x "_FE", \
	.stream_name = #x, \
	.cpu_dai_name = "snd-soc-dummy-dai", \
	.codec_name = "snd-soc-dummy", \
	.codec_dai_name = "snd-soc-dummy-dai", \
	.platform_name = "snd-soc-dummy" \
}

/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link mt8696_sound1_dais[] = {
	/* Front End DAI links */

	[DAI_LINK_FE_DLM_PLAYBACK] = {
		.name = "DLM_FE",
		.stream_name = "DLM Playback",
		.cpu_dai_name = "DLM",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_FE_DLM_PLAYBACK,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	[DAI_LINK_FE_DL2_PLAYBACK] = {
		.name = "DL2_FE",
		.stream_name = "DL2 Playback",
		.cpu_dai_name = "DL2",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_FE_DL2_PLAYBACK,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	[DAI_LINK_FE_DL10_PLAYBACK] = {
		.name = "DL10_FE",
		.stream_name = "DL10 Playback",
		.cpu_dai_name = "DL10",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_FE_DL10_PLAYBACK,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	[DAI_LINK_FE_UL1_CAPTURE] = {
		.name = "UL1_FE",
		.stream_name = "UL1 Capture",
		.cpu_dai_name = "UL1",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_FE_UL1_CAPTURE,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_PRE,
			SND_SOC_DPCM_TRIGGER_PRE
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	[DAI_LINK_FE_UL2_CAPTURE] = {
		.name = "UL2_FE",
		.stream_name = "UL2 Capture",
		.cpu_dai_name = "UL2",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_FE_UL2_CAPTURE,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	[DAI_LINK_FE_UL4_CAPTURE] = {
		.name = "UL4_FE",
		.stream_name = "UL4 Capture",
		.cpu_dai_name = "UL4",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_FE_UL4_CAPTURE,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},

	/* Back End DAI links */
	[DAI_LINK_BE_ETDM1_OUT] = {
		.name = "ETDM1_OUT BE",
		.cpu_dai_name = "ETDM1_OUT",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_ETDM1_OUT,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.ops = &mt8696_sound1_etdm_ops,
		.dpcm_playback = 1,
	},
	[DAI_LINK_BE_ETDM1_IN] = {
		.name = "ETDM1_IN BE",
		.cpu_dai_name = "ETDM1_IN",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_ETDM1_IN,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,   /* set be master*/
		.ops = &mt8696_sound1_etdm_ops,
		.dpcm_capture = 1,
	},
	[DAI_LINK_BE_ETDM2_OUT] = {
		.name = "ETDM2_OUT BE",
		.cpu_dai_name = "ETDM2_OUT",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_ETDM2_OUT,
		.dai_fmt = SND_SOC_DAIFMT_LEFT_J |//1128 modify to LEFT_J
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.ops = &mt8696_sound1_etdm_ops,
		.dpcm_playback = 1,
	},
	[DAI_LINK_BE_ETDM2_IN] = {
		.name = "ETDM2_IN BE",
		.cpu_dai_name = "ETDM2_IN",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_ETDM2_IN,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBM_CFM,   /* set be slave*/
		.ops = &mt8696_sound1_etdm_ops,
		.dpcm_capture = 1,
	},
	[DAI_LINK_BE_ETDM3_OUT] = {
		.name = "ETDM3_OUT BE",
		.cpu_dai_name = "ETDM3_OUT",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_ETDM3_OUT,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.ops = &mt8696_sound1_etdm_ops,
		.dpcm_playback = 1,
	},
#if 0
	[DAI_LINK_BE_PCM_INTF] = {
		.name = "PCM1 BE",
		.cpu_dai_name = "PCM1",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_PCM_INTF,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	[DAI_LINK_BE_DMIC] = {
		.name = "DMIC BE",
		.cpu_dai_name = "DMIC",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_DMIC,
		.dpcm_capture = 1,
	},
#endif

	[DAI_LINK_BE_GASRC0] = {
		.name = "GASRC0 BE",
		.cpu_dai_name = "GASRC0",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_GASRC0,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
#if 1
	[DAI_LINK_BE_GASRC1] = {
		.name = "GASRC1 BE",
		.cpu_dai_name = "GASRC1",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_GASRC1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	[DAI_LINK_BE_GASRC2] = {
		.name = "GASRC2 BE",
		.cpu_dai_name = "GASRC2",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_GASRC2,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	[DAI_LINK_BE_GASRC3] = {
		.name = "GASRC3 BE",
		.cpu_dai_name = "GASRC3",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_GASRC3,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
#endif
#if 0
	[DAI_LINK_BE_SPDIF_OUT] = {
		.name = "SPDIF_OUT BE",
		.cpu_dai_name = "SPDIF_OUT",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_SPDIF_OUT,
		.dpcm_playback = 1,
	},
#endif
	[DAI_LINK_BE_SPDIF_IN] = {
		.name = "SPDIF_IN BE",
		.cpu_dai_name = "SPDIF_IN",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_SPDIF_IN,
		.dpcm_capture = 1,
	},
	[DAI_LINK_BE_MULTI_IN] = {
		.name = "MULTI_IN BE",
		.cpu_dai_name = "MULTI_IN",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_MULTI_IN,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBM_CFM,
		.dpcm_capture = 1,
	},
	[DAI_LINK_BE_VIRTUAL] = {
		.name = "VIRTUAL_IN_OUT BE",
		.cpu_dai_name = "VIRTUAL_IN_OUT",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.id = DAI_LINK_BE_VIRTUAL,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBM_CFM,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
};

static const struct snd_kcontrol_new mt8696_sound1_controls[] = {
};

static int mt8696_sound1_gpio_probe(struct snd_soc_card *card)
{
	struct mt8696_sound1_priv *priv = snd_soc_card_get_drvdata(card);
	int ret = 0;
	int i;

	priv->pinctrl = devm_pinctrl_get(card->dev);
	if (IS_ERR(priv->pinctrl)) {
		ret = PTR_ERR(priv->pinctrl);
		dev_dbg(card->dev, "%s devm_pinctrl_get failed %d\n",
			__func__, ret);
		return ret;
	}

	for (i = 0 ; i < PIN_STATE_MAX ; i++) {
		priv->pin_states[i] = pinctrl_lookup_state(priv->pinctrl,
			mt8696_sound1_pin_str[i]);
		if (IS_ERR(priv->pin_states[i])) {
			ret = PTR_ERR(priv->pin_states[i]);
			dev_dbg(card->dev, "%s Can't find pin state %s %d\n",
				 __func__, mt8696_sound1_pin_str[i], ret);
		}
	}

	if (IS_ERR(priv->pin_states[PIN_STATE_DEFAULT])) {
		dev_dbg(card->dev, "%s can't find default pin state\n",
			__func__);
		return 0;
	}

	/* default state */
	ret = pinctrl_select_state(priv->pinctrl,
				   priv->pin_states[PIN_STATE_DEFAULT]);
	if (ret)
		dev_dbg(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	return ret;
}

static void mt8696_sound1_parse_of_codec_dai_data(
	struct mt8696_sound1_codec_dai_data *codec_dai_data,
	struct device_node *np,
	const char *name)
{
	char prop[128];
	unsigned int val;
	int ret;
	struct mt8696_sound1_codec_pll_clk_data *pll_clk;

	pll_clk = &codec_dai_data->pll_clk;

	snprintf(prop, sizeof(prop),
		PREFIX"%s-set-pll-clk-in-hw-params", name);
	codec_dai_data->set_pll_clk_in_hw_params =
		of_property_read_bool(np, prop);

	snprintf(prop, sizeof(prop), PREFIX"%s-pll-id", name);
	ret = of_property_read_u32(np, prop, &val);
	if (ret == 0)
		pll_clk->pll_id = val;

	snprintf(prop, sizeof(prop), PREFIX"%s-pll-src-id", name);
	ret = of_property_read_u32(np, prop, &val);
	if (ret == 0)
		pll_clk->src_id = val;

	snprintf(prop, sizeof(prop),
		 PREFIX"%s-pll-clk-multiplier", name);
	ret = of_property_read_u32(np, prop, &val);
	if (ret == 0)
		pll_clk->clk_multp = val;

	snprintf(prop, sizeof(prop), PREFIX"%s-pll-freq-in", name);
	ret = of_property_read_u32(np, prop, &val);
	if (ret == 0)
		pll_clk->freq_in = val;

	snprintf(prop, sizeof(prop), PREFIX"%s-pll-freq-out", name);
	ret = of_property_read_u32(np, prop, &val);
	if (ret == 0)
		pll_clk->freq_out = val;
}

static void mt8696_sound1_parse_of_codec(struct device *dev,
	struct device_node *np,
	struct snd_soc_dai_link *dai_link,
	struct mt8696_sound1_priv *priv,
	struct mt8696_sound1_be_ctrl_data *be,
	char *name)
{
	char prop[128];
	const char *of_str;
	int ret;
	unsigned int i, num_codecs, idx;
	struct device_node *codec_node;
	struct snd_soc_codec_conf *codec_conf;
	struct mt8696_sound1_codec_dai_data *codec_dai_data;

	snprintf(prop, sizeof(prop), PREFIX"%s-audio-codec-num", name);
	ret = of_property_read_u32(np, prop, &num_codecs);
	if (ret)
		goto single_codec;

	if (num_codecs == 0)
		return;

	dai_link->codecs = devm_kzalloc(dev,
		num_codecs * sizeof(struct snd_soc_dai_link_component),
		GFP_KERNEL);

	dai_link->num_codecs = num_codecs;
	dai_link->codec_name = NULL;
	dai_link->codec_of_node = NULL;
	dai_link->codec_dai_name = NULL;

	be->num_codec_dais = num_codecs;
	be->codec_dai_data = devm_kzalloc(dev,
		be->num_codec_dais *
		sizeof(struct mt8696_sound1_codec_dai_data),
		GFP_KERNEL);

	for (i = 0; i < num_codecs; i++) {
		codec_node = NULL;

		// parse codec_of_node
		snprintf(prop, sizeof(prop),
			 PREFIX"%s-audio-codec%u",
			 name, i);
		codec_node = of_parse_phandle(np, prop, 0);
		if (codec_node)
			dai_link->codecs[i].of_node = codec_node;
		else {
			// parse codec name
			snprintf(prop, sizeof(prop),
				 PREFIX"%s-codec-name%u",
				 name, i);
			of_property_read_string(np, prop,
				&dai_link->codecs[i].name);
		}

		of_str = NULL;

		// parse codec prefix
		snprintf(prop, sizeof(prop),
			 PREFIX"%s-audio-codec%u-prefix",
			 name, i);
		of_property_read_string(np, prop, &of_str);
		if (of_str) {
			idx = priv->num_codec_configs;

			if (idx < MAX_CODEC_CONF) {
				codec_conf = &priv->codec_conf[idx];

				if (codec_node)
					codec_conf->of_node = codec_node;
				else
					codec_conf->dev_name =
						dai_link->codecs[i].name;

				priv->codec_conf[idx].name_prefix = of_str;
				priv->num_codec_configs++;
			}
		}

		of_str = NULL;

		// parse codec dai name
		snprintf(prop, sizeof(prop),
			 PREFIX"%s-codec-dai-name%u",
			 name, i);
		of_property_read_string(np, prop, &of_str);
		if (of_str) {
			dai_link->codecs[i].dai_name = of_str;

			codec_dai_data = &be->codec_dai_data[i];
			codec_dai_data->dai_name = of_str;

			mt8696_sound1_parse_of_codec_dai_data(codec_dai_data,
				np, codec_dai_data->dai_name);
		}
	}

	return;

single_codec:
	// parse codec_of_node
	snprintf(prop, sizeof(prop), PREFIX"%s-audio-codec", name);
	codec_node = of_parse_phandle(np, prop, 0);
	if (codec_node) {
		dai_link->codec_of_node = codec_node;
		dai_link->codec_name = NULL;
	}

	of_str = NULL;

	// parse codec prefix
	snprintf(prop, sizeof(prop), PREFIX"%s-audio-codec-prefix", name);
	of_property_read_string(np, prop, &of_str);
	if (of_str) {
		idx = priv->num_codec_configs;

		if (idx < MAX_CODEC_CONF) {
			codec_conf = &priv->codec_conf[idx];

			if (codec_node)
				codec_conf->of_node = codec_node;
			else
				codec_conf->dev_name =
					dai_link->codec_name;

			priv->codec_conf[idx].name_prefix = of_str;
			priv->num_codec_configs++;
		}
	}

	of_str = NULL;

	// parse codec dai name
	snprintf(prop, sizeof(prop), PREFIX"%s-codec-dai-name", name);
	of_property_read_string(np, prop, &of_str);
	if (of_str) {
		dai_link->codec_dai_name = of_str;

		be->num_codec_dais = 1;
		be->codec_dai_data = devm_kzalloc(dev,
			be->num_codec_dais *
			sizeof(struct mt8696_sound1_codec_dai_data),
			GFP_KERNEL);

		codec_dai_data = &be->codec_dai_data[0];
		codec_dai_data->dai_name = of_str;

		mt8696_sound1_parse_of_codec_dai_data(codec_dai_data,
			np, codec_dai_data->dai_name);
	}
}

static void mt8696_sound1_parse_of(struct snd_soc_card *card,
				struct device_node *np)
{
	struct mt8696_sound1_priv *priv = snd_soc_card_get_drvdata(card);
	size_t i;
	int ret;
	char prop[128];
	const char *str;
	unsigned int val;
	unsigned int vals[2];
	struct snd_soc_dai_link *dai_link;
	unsigned int link_id;

	static const struct mt8532_dai_link_prop of_dai_links_fe[] = {
		//{"dlm", DAI_LINK_FE_DLM_PLAYBACK},
		//{"dl11", DAI_LINK_FE_DL11_PLAYBACK},
		//{"dl2", DAI_LINK_FE_DL2_PLAYBACK},
		//{"dl3", DAI_LINK_FE_DL3_PLAYBACK},
		//{"dl6", DAI_LINK_FE_DL6_PLAYBACK},
		//{"ul2", DAI_LINK_FE_UL2_CAPTURE},
		//{"ul3", DAI_LINK_FE_UL3_CAPTURE},
		{"ul4", DAI_LINK_FE_UL4_CAPTURE},
		//{"ul5", DAI_LINK_FE_UL5_CAPTURE},
		//{"ul8", DAI_LINK_FE_UL8_CAPTURE},
		//{"ul9", DAI_LINK_FE_UL9_CAPTURE},
		//{"ul10", DAI_LINK_FE_UL10_CAPTURE},
	};

	static const struct mt8532_dai_link_prop of_dai_links_be[] = {
		{"etdm1-out", DAI_LINK_BE_ETDM1_OUT},
		{"etdm1-in", DAI_LINK_BE_ETDM1_IN},
		{"etdm2-out", DAI_LINK_BE_ETDM2_OUT},
		{"etdm2-in", DAI_LINK_BE_ETDM2_IN},
		//{"pcm-intf", DAI_LINK_BE_PCM_INTF},
		//{"dmic", DAI_LINK_BE_DMIC},
		{"multi-in", DAI_LINK_BE_MULTI_IN},
	};

	snd_soc_of_parse_card_name(card, PREFIX"card-name");

	if (of_property_read_bool(np, PREFIX "widgets")) {
		snd_soc_of_parse_audio_simple_widgets(card,
			PREFIX "widgets");
	}

	if (of_property_read_bool(np, PREFIX "routing")) {
		snd_soc_of_parse_audio_routing(card,
			PREFIX "routing");
	}

	for (i = 0; i < ARRAY_SIZE(of_dai_links_fe); i++) {
		bool lrck_inverse = false;
		bool bck_inverse = false;

		link_id = of_dai_links_fe[i].link_id;
		dai_link = &mt8696_sound1_dais[link_id];

		// parse format
		snprintf(prop, sizeof(prop), PREFIX"%s-format",
			 of_dai_links_fe[i].name);
		ret = of_property_read_string(np, prop, &str);
		if (ret == 0) {
			unsigned int format = 0;

			format = mt8532_snd_get_dai_format(str);

			dai_link->dai_fmt &= ~SND_SOC_DAIFMT_FORMAT_MASK;
			dai_link->dai_fmt |= format;
		}

		// parse clock mode
		snprintf(prop, sizeof(prop), PREFIX"%s-master-clock",
			 of_dai_links_fe[i].name);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0) {
			dai_link->dai_fmt &= ~SND_SOC_DAIFMT_MASTER_MASK;
			if (val)
				dai_link->dai_fmt |= SND_SOC_DAIFMT_CBS_CFS;
			else
				dai_link->dai_fmt |= SND_SOC_DAIFMT_CBM_CFM;
		}

		// parse lrck inverse
		snprintf(prop, sizeof(prop), PREFIX"%s-lrck-inverse",
			 of_dai_links_fe[i].name);
		lrck_inverse = of_property_read_bool(np, prop);

		// parse bck inverse
		snprintf(prop, sizeof(prop), PREFIX"%s-bck-inverse",
			 of_dai_links_fe[i].name);
		bck_inverse = of_property_read_bool(np, prop);

		dai_link->dai_fmt &= ~SND_SOC_DAIFMT_INV_MASK;

		if (lrck_inverse && bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_IB_IF;
		else if (lrck_inverse && !bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_IF;
		else if (!lrck_inverse && bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_IB_NF;
		else
			dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_NF;

		// parse trigger order
		snprintf(prop, sizeof(prop), PREFIX"%s-trigger-order",
			 of_dai_links_fe[i].name);
		ret = of_property_read_u32_array(np, prop, vals, 2);
		if (ret == 0) {
			if (vals[0] <= SND_SOC_DPCM_TRIGGER_BESPOKE)
				dai_link->trigger[0] = vals[0];

			if (vals[1] <= SND_SOC_DPCM_TRIGGER_BESPOKE)
				dai_link->trigger[1] = vals[1];
		}
	}

	for (i = 0; i < ARRAY_SIZE(of_dai_links_be); i++) {
		struct mt8696_sound1_be_ctrl_data *be;
		bool lrck_inverse = false;
		bool bck_inverse = false;
		bool hook_be_fixup_cb = false;

		link_id = of_dai_links_be[i].link_id;

		if ((link_id < DAI_LINK_BE_BASE) ||
		    (link_id >= DAI_LINK_BE_END))
			continue;

		dai_link = &mt8696_sound1_dais[link_id];
		be = &priv->be_data[link_id - DAI_LINK_BE_BASE];

		// parse format
		snprintf(prop, sizeof(prop), PREFIX"%s-format",
			 of_dai_links_be[i].name);
		ret = of_property_read_string(np, prop, &str);
		if (ret == 0) {
			unsigned int format = 0;

			format = mt8532_snd_get_dai_format(str);

			dai_link->dai_fmt &= ~SND_SOC_DAIFMT_FORMAT_MASK;
			dai_link->dai_fmt |= format;
		}

		// parse clock mode
		snprintf(prop, sizeof(prop), PREFIX"%s-master-clock",
			 of_dai_links_be[i].name);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0) {
			dai_link->dai_fmt &= ~SND_SOC_DAIFMT_MASTER_MASK;
			if (val)
				dai_link->dai_fmt |= SND_SOC_DAIFMT_CBS_CFS;
			else
				dai_link->dai_fmt |= SND_SOC_DAIFMT_CBM_CFM;
		}

		// parse lrck inverse
		snprintf(prop, sizeof(prop), PREFIX"%s-lrck-inverse",
			 of_dai_links_be[i].name);
		lrck_inverse = of_property_read_bool(np, prop);

		// parse bck inverse
		snprintf(prop, sizeof(prop), PREFIX"%s-bck-inverse",
			 of_dai_links_be[i].name);
		bck_inverse = of_property_read_bool(np, prop);

		dai_link->dai_fmt &= ~SND_SOC_DAIFMT_INV_MASK;

		if (lrck_inverse && bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_IB_IF;
		else if (lrck_inverse && !bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_IF;
		else if (!lrck_inverse && bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_IB_NF;
		else
			dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_NF;

		// parse mclk multiplier
		snprintf(prop, sizeof(prop), PREFIX"%s-mclk-multiplier",
			 of_dai_links_be[i].name);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0)
			be->mck_multp = val;

		// parse lrck width
		snprintf(prop, sizeof(prop), PREFIX"%s-lrck-width",
			 of_dai_links_be[i].name);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0)
			be->lrck_width = val;

		// parse fix rate
		snprintf(prop, sizeof(prop), PREFIX"%s-fix-rate",
			 of_dai_links_be[i].name);
		ret = of_property_read_u32(np, prop, &val);
		if ((ret == 0) && ((link_to_dai(link_id) < 0) ||
		    mt8532_afe_rate_supported(val, link_to_dai(link_id)))) {
			be->fix_rate = val;
			hook_be_fixup_cb = true;
		}

		// parse fix bit width
		snprintf(prop, sizeof(prop), PREFIX"%s-fix-bit-width",
			 of_dai_links_be[i].name);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0 && (val == 32 || val == 16)) {
			be->fix_bit_width = val;
			hook_be_fixup_cb = true;
		}

		// parse fix channels
		snprintf(prop, sizeof(prop), PREFIX"%s-fix-channels",
			 of_dai_links_be[i].name);
		ret = of_property_read_u32(np, prop, &val);
		if ((ret == 0) && ((link_to_dai(link_id) < 0) ||
		    mt8532_afe_channel_supported(val, link_to_dai(link_id)))) {
			be->fix_channels = val;
			hook_be_fixup_cb = true;
		}

		if (hook_be_fixup_cb)
			dai_link->be_hw_params_fixup =
				mt8696_sound1_be_hw_params_fixup;

		mt8696_sound1_parse_of_codec(card->dev, np, dai_link, priv, be,
			of_dai_links_be[i].name);

		// parse ignore pmdown time
		snprintf(prop, sizeof(prop), PREFIX"%s-ignore-pmdown-time",
			 of_dai_links_be[i].name);
		if (of_property_read_bool(np, prop))
			dai_link->ignore_pmdown_time = 1;

		// parse ignore suspend
		snprintf(prop, sizeof(prop), PREFIX"%s-ignore-suspend",
			 of_dai_links_be[i].name);
		if (of_property_read_bool(np, prop))
			dai_link->ignore_suspend = 1;
	}

	if (priv->num_codec_configs > 0) {
		card->num_configs = priv->num_codec_configs;
		card->codec_conf = &priv->codec_conf[0];
	}
}

static struct snd_soc_card mt8696_sound1_card = {
	.name = "mt-snd1-card",
	.long_name = "mt8696-sound1-card",
	.owner = THIS_MODULE,
	.dai_link = mt8696_sound1_dais,
	.num_links = ARRAY_SIZE(mt8696_sound1_dais),
	.controls = mt8696_sound1_controls,
	.num_controls = ARRAY_SIZE(mt8696_sound1_controls),
	.dapm_widgets = mt8696_sound1_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt8696_sound1_widgets),
	.dapm_routes = mt8696_sound1_routes,
	.num_dapm_routes = ARRAY_SIZE(mt8696_sound1_routes),
};

static void mt8696_sound1_cleanup_of_resource(struct snd_soc_card *card)
{
	struct mt8696_sound1_priv *priv = snd_soc_card_get_drvdata(card);
	struct snd_soc_dai_link *dai_link;
	int i, j;

	of_node_put(priv->afe_plat_node);

	for (i = 0, dai_link = card->dai_link;
	     i < card->num_links; i++, dai_link++) {
		if (dai_link->num_codecs > 1) {
			struct snd_soc_dai_link_component *codec;

			for (j = 0, codec = dai_link->codecs;
			     j < dai_link->num_codecs; j++, codec++) {
				if (!codec)
					break;
				of_node_put(codec->of_node);
			}
		} else if (dai_link->num_codecs == 1)
			of_node_put(dai_link->codec_of_node);
	}
}

static int mt8696_sound1_dev_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt8696_sound1_card;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *afe_plat_node;
	struct mt8696_sound1_priv *priv;
	struct platform_device *afe_pdev;
	struct mtk_base_afe *afe;
	int ret, id;
	size_t i;
	size_t dais_num = ARRAY_SIZE(mt8696_sound1_dais);

	afe_plat_node = of_parse_phandle(dev->of_node, "mediatek,platform", 0);
	if (!afe_plat_node) {
		dev_info(dev, "Property 'platform' missing or invalid\n");
		return -EINVAL;
	}
	afe_pdev = of_find_device_by_node(afe_plat_node);
	if (!afe_pdev) {
		dev_info(dev, "'platform' platform_device missing or invalid\n");
		return -EINVAL;
	}
	afe = platform_get_drvdata(afe_pdev);
	if (!afe) {
		dev_info(dev, "'platform' driver missing or invalid\n");
		return -EINVAL;
	}

	for (i = 0; i < dais_num; i++) {
		if (mt8696_sound1_dais[i].platform_name)
			continue;

		id = mt8696_sound1_dais[i].id;

		if ((id >= DAI_LINK_FE_AFE_BASE &&
		     id < DAI_LINK_FE_AFE_END) ||
		    (id >= DAI_LINK_BE_AFE_BASE &&
		     id < DAI_LINK_BE_AFE_END)) {
			mt8696_sound1_dais[i].platform_of_node = afe_plat_node;
		}
	}

	card->dev = dev;

	priv = devm_kzalloc(dev, sizeof(struct mt8696_sound1_priv),
			    GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		dev_dbg(dev, "%s allocate card private data fail %d\n",
			__func__, ret);
		return ret;
	}

	priv->afe_plat_node = afe_plat_node;


	snd_soc_card_set_drvdata(card, priv);

	mt8696_sound1_gpio_probe(card);

	mt8696_sound1_parse_of(card, np);

	mt8532_afe_enable_reg_rw_clk(afe);

	ret = devm_snd_soc_register_card(dev, card);
	if (ret) {
		dev_dbg(dev, "%s snd_soc_register_card fail %d\n",
			__func__, ret);
		mt8532_afe_disable_reg_rw_clk(afe);
		return ret;
	}
	mt8532_afe_disable_reg_rw_clk(afe);

	dev_dbg(dev, "%s %d leave\n", __func__, __LINE__);

	return ret;
}

static int mt8696_sound1_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	mt8696_sound1_cleanup_of_resource(card);

	return 0;
}

static const struct of_device_id mt8696_sound1_dt_match[] = {
	{ .compatible = "mediatek,mt8696-card1", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt8696_sound1_dt_match);

static struct platform_driver mt8696_sound1_driver = {
	.driver = {
		   .name = "mt8696-sound1",
		   .of_match_table = mt8696_sound1_dt_match,
#ifdef CONFIG_PM
		   .pm = &snd_soc_pm_ops,
#endif
	},
	.probe = mt8696_sound1_dev_probe,
	.remove = mt8696_sound1_dev_remove,
};

module_platform_driver(mt8696_sound1_driver);

/* Module information */
MODULE_DESCRIPTION("MT8696 sound1 SoC machine driver");
MODULE_AUTHOR("Wen Cai <wen.cai@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:mt8696-sound1");

