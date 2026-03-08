// SPDX-License-Identifier: GPL-2.0
/*
 * mt8696-evb.c  --  MT8696 machine driver
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Sail Yang <sail.yang@mediatek.com>
 *
 */

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "mt8696-afe-common.h"

#define BACKEND_WITH_ENDPOINT

#define PREFIX	"mediatek,"
#define ENUM_TO_STR(enum) #enum

enum PINCTRL_PIN_STATE {
	PIN_STATE_DEFAULT = 0,
	PIN_STATE_EXTAMP_ON,
	PIN_STATE_EXTAMP_OFF,
	PIN_STATE_MAX
};

static const char * const mt8696_evb_pin_str[PIN_STATE_MAX] = {
	"default",
	"extamp_on",
	"extamp_off",
};

struct mt8696_evb_etdm_ctrl_data {
	unsigned int mck_multp_in;
	unsigned int mck_multp_out;
	unsigned int lrck_width_in;
	unsigned int lrck_width_out;
	unsigned int fix_rate_in;
	unsigned int fix_rate_out;
	unsigned int fix_bit_width_in;
	unsigned int fix_bit_width_out;
	unsigned int fix_channels_in;
	unsigned int fix_channels_out;
};

struct mt8696_evb_priv {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_states[PIN_STATE_MAX];
	struct mt8696_evb_etdm_ctrl_data etdm_data[MT8696_ETDM_SETS];
	struct device_node *afe_plat_node;
};

struct mt8696_dai_link_prop {
	char *name;
	unsigned int link_id;
};

static const struct snd_soc_dapm_widget mt8696_evb_widgets[] = {
#ifdef BACKEND_WITH_ENDPOINT
	SND_SOC_DAPM_OUTPUT("HDMI Output"),
	SND_SOC_DAPM_OUTPUT("BTSCO Output"),
	SND_SOC_DAPM_INPUT("BTSCO Input"),
#endif
};

static const struct snd_soc_dapm_route mt8696_evb_routes[] = {
#ifdef BACKEND_WITH_ENDPOINT
	{"HDMI Output", NULL, "ETDM1 Playback"},
	{"BTSCO Output", NULL, "ETDM2 Playback"},
	{"ETDM2 Capture", NULL, "BTSCO Input"},
#endif
};

enum {
	/* FE */
	DAI_LINK_AFE_FE_BASE = 0,
	DAI_LINK_DL8_PLAYBACK = DAI_LINK_AFE_FE_BASE,
	DAI_LINK_DL5_PLAYBACK,
	DAI_LINK_UL1_CAPTURE,
#ifdef CONFIG_SND_SOC_MTK_BTCVSD
	DAI_LINK_BTCVSD_RX,
	DAI_LINK_BTCVSD_TX,
	DAI_LINK_AFE_FE_END = DAI_LINK_BTCVSD_TX,
#else
	DAI_LINK_AFE_FE_END = DAI_LINK_UL1_CAPTURE,
#endif
	/* BE */
	DAI_LINK_AFE_BE_BASE,
	DAI_LINK_ETDM1_OUT = DAI_LINK_AFE_BE_BASE,
	DAI_LINK_ETDM2_IN,
	DAI_LINK_ETDM2_OUT,
	DAI_LINK_AFE_BE_END = DAI_LINK_ETDM2_OUT,
	DAI_LINK_NUM
};

static int link_to_dai(int link_id)
{
	switch (link_id) {
	case DAI_LINK_ETDM1_OUT:
		return MT8696_AFE_IO_ETDM1_OUT;
	case DAI_LINK_ETDM2_OUT:
		return MT8696_AFE_IO_ETDM2_OUT;
	case DAI_LINK_ETDM2_IN:
		return MT8696_AFE_IO_ETDM2_IN;
	default:
		break;
	}
	return -1;
}

static int mt8696_evb_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mt8696_evb_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	int id = rtd->dai_link->id;
	struct mt8696_evb_etdm_ctrl_data *etdm;

	unsigned int mclk_multiplier = 0;
	unsigned int mclk = 0;
	unsigned int lrck_width = 0;
	int slot = 0;
	int slot_width = 0;
	unsigned int slot_bitmask = 0;
	unsigned int idx;
	int ret;

	if (id == DAI_LINK_ETDM1_OUT) {
		idx = MT8696_ETDM1;
		etdm = &priv->etdm_data[idx];
		mclk_multiplier = etdm->mck_multp_out;
		lrck_width = etdm->lrck_width_out;
	} else if (id == DAI_LINK_ETDM2_OUT) {
		idx = MT8696_ETDM2;
		etdm = &priv->etdm_data[idx];
		mclk_multiplier = etdm->mck_multp_out;
		lrck_width = etdm->lrck_width_out;
	} else if (id == DAI_LINK_ETDM2_IN) {
		idx = MT8696_ETDM2;
		etdm = &priv->etdm_data[idx];
		mclk_multiplier = etdm->mck_multp_in;
		lrck_width = etdm->lrck_width_in;
	}

	if (mclk_multiplier > 0) {
		mclk = mclk_multiplier * params_rate(params);

		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk,
					     SND_SOC_CLOCK_OUT);
		if (!ret)
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
		if (!ret)
			return ret;
	}

	return 0;
}

static int mt8696_evb_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
	struct snd_pcm_hw_params *params)
{
	struct mt8696_evb_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	int id = rtd->dai_link->id;
	struct mt8696_evb_etdm_ctrl_data *etdm;

	unsigned int fix_rate = 0;
	unsigned int fix_bit_width = 0;
	unsigned int fix_channels = 0;
	unsigned int idx;

	if (id == DAI_LINK_ETDM1_OUT) {
		idx = MT8696_ETDM1;
		etdm = &priv->etdm_data[idx];
		fix_rate = etdm->fix_rate_out;
		fix_bit_width = etdm->fix_bit_width_out;
		fix_channels = etdm->fix_channels_out;
	} else if (id == DAI_LINK_ETDM2_OUT) {
		idx = MT8696_ETDM2;
		etdm = &priv->etdm_data[idx];
		fix_rate = etdm->fix_rate_out;
		fix_bit_width = etdm->fix_bit_width_out;
		fix_channels = etdm->fix_channels_out;
	} else if (id == DAI_LINK_ETDM2_IN) {
		idx = MT8696_ETDM2;
		etdm = &priv->etdm_data[idx];
		fix_rate = etdm->fix_rate_in;
		fix_bit_width = etdm->fix_bit_width_in;
		fix_channels = etdm->fix_channels_in;
	}

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

static struct snd_soc_ops mt8696_evb_etdm_ops = {
	.hw_params = mt8696_evb_hw_params,
};

/* FE */
SND_SOC_DAILINK_DEFS(HDMI_OUTPUT,
		     DAILINK_COMP_ARRAY(COMP_CPU("DL8")),
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(BTSCO_OUTPUT,
		     DAILINK_COMP_ARRAY(COMP_CPU("DL5")),
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(BTSCO_INPUT,
		     DAILINK_COMP_ARRAY(COMP_CPU("UL1")),
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_EMPTY()));

#ifdef CONFIG_SND_SOC_MTK_BTCVSD
SND_SOC_DAILINK_DEFS(BTCVSD_RX,
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("mt-soc-btcvsd-rx-pcm")));


SND_SOC_DAILINK_DEFS(BTCVSD_TX,
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("mt-soc-btcvsd-tx-pcm")));

#endif



/* BE */
SND_SOC_DAILINK_DEFS(ETDM1_OUT_BE,
		     DAILINK_COMP_ARRAY(COMP_CPU("ETDM1_OUT")),
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(ETDM2_IN_BE,
		     DAILINK_COMP_ARRAY(COMP_CPU("ETDM2_IN")),
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(ETDM2_OUT_BE,
		     DAILINK_COMP_ARRAY(COMP_CPU("ETDM2_OUT")),
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_EMPTY()));

/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link mt8696_evb_dais[] = {
	/* Front End DAI links */
	[DAI_LINK_DL8_PLAYBACK] = {
		.name = "HDMI_OUTPUT",
		.stream_name = "HDMI_Playback",
		.id = DAI_LINK_DL8_PLAYBACK,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(HDMI_OUTPUT),
	},
	[DAI_LINK_DL5_PLAYBACK] = {
		.name = "BTSCO_OUTPUT",
		.stream_name = "DL5_Playback",
		.id = DAI_LINK_DL5_PLAYBACK,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(BTSCO_OUTPUT),
	},
	[DAI_LINK_UL1_CAPTURE] = {
		.name = "BTSCO_INPUT",
		.stream_name = "UL1_Capture",
		.id = DAI_LINK_UL1_CAPTURE,
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(BTSCO_INPUT),
	},
#ifdef CONFIG_SND_SOC_MTK_BTCVSD
	[DAI_LINK_BTCVSD_RX] = {
		.name = "BTCVSD_RX",
		.stream_name = "BTCVSD_Capture",
		.capture_only = 1,
		SND_SOC_DAILINK_REG(BTCVSD_RX),
	},
	[DAI_LINK_BTCVSD_TX] = {
		.name = "BTCVSD_TX",
		.stream_name = "BTCVSD_Playback",
		.playback_only = 1,
		SND_SOC_DAILINK_REG(BTCVSD_TX),
	},
#endif
	/* Back End DAI links */
	[DAI_LINK_ETDM1_OUT] = {
		.name = "ETDM1_OUT BE",
		.no_pcm = 1,
		.id = DAI_LINK_ETDM1_OUT,
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
				SND_SOC_DAIFMT_CBS_CFS,
		.ops = &mt8696_evb_etdm_ops,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(ETDM1_OUT_BE),
	},
	[DAI_LINK_ETDM2_IN] = {
		.name = "ETDM2_IN BE",
		.no_pcm = 1,
		.id = DAI_LINK_ETDM2_IN,
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
				SND_SOC_DAIFMT_CBS_CFS,
		.ops = &mt8696_evb_etdm_ops,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(ETDM2_IN_BE),
	},
	[DAI_LINK_ETDM2_OUT] = {
		.name = "ETDM2_OUT BE",
		.no_pcm = 1,
		.id = DAI_LINK_ETDM2_OUT,
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
				SND_SOC_DAIFMT_CBS_CFS,
		.ops = &mt8696_evb_etdm_ops,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(ETDM2_OUT_BE),
	},
};

static int mt8696_evb_gpio_probe(struct snd_soc_card *card)
{
	struct mt8696_evb_priv *priv = snd_soc_card_get_drvdata(card);
	int ret = 0;
	int i;

	priv->pinctrl = devm_pinctrl_get(card->dev);
	if (IS_ERR(priv->pinctrl)) {
		ret = PTR_ERR(priv->pinctrl);
		dev_info(card->dev, "%s devm_pinctrl_get failed %d\n",
			__func__, ret);
		return ret;
	}

	for (i = 0 ; i < PIN_STATE_MAX ; i++) {
		priv->pin_states[i] = pinctrl_lookup_state(priv->pinctrl,
			mt8696_evb_pin_str[i]);
		if (IS_ERR(priv->pin_states[i])) {
			ret = PTR_ERR(priv->pin_states[i]);
			dev_dbg(card->dev, "%s Can't find pin state %s %d\n",
				 __func__, mt8696_evb_pin_str[i], ret);
		}
	}

	if (IS_ERR(priv->pin_states[PIN_STATE_DEFAULT])) {
		dev_info(card->dev, "%s can't find default pin state\n",
			__func__);
		return 0;
	}

	/* default state */
	ret = pinctrl_select_state(priv->pinctrl,
				   priv->pin_states[PIN_STATE_DEFAULT]);
	if (ret)
		dev_info(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	/* turn off ext amp if exist */
	if (!IS_ERR(priv->pin_states[PIN_STATE_EXTAMP_OFF])) {
		ret = pinctrl_select_state(priv->pinctrl,
			priv->pin_states[PIN_STATE_EXTAMP_OFF]);
		if (ret)
			dev_dbg(card->dev,
				"%s failed to select state %d\n",
				__func__, ret);
	}

	return ret;
}

static void mt8696_evb_parse_of(struct snd_soc_card *card,
				struct device_node *np)
{
	struct mt8696_evb_priv *priv = snd_soc_card_get_drvdata(card);
	size_t i, j;
	int ret;
	char prop[128];
	const char *str;
	unsigned int val;
	int sret = 0;

	static const struct mt8696_dai_link_prop of_dai_links_daifmt[] = {
		{ "dl8",	DAI_LINK_DL8_PLAYBACK },
		{ "dl5",	DAI_LINK_DL5_PLAYBACK },
		{ "ul1",	DAI_LINK_UL1_CAPTURE },
		{ "etdm1-out",	DAI_LINK_ETDM1_OUT },
		{ "etdm2-out",	DAI_LINK_ETDM2_OUT },
		{ "etdm2-in",		DAI_LINK_ETDM2_IN },
	};

	static const struct mt8696_dai_link_prop of_dai_links_etdm[] = {
		{ "etdm1-out",	DAI_LINK_ETDM1_OUT },
		{ "etdm2-out",	DAI_LINK_ETDM2_OUT },
		{ "etdm2-in",		DAI_LINK_ETDM2_IN },
	};

	struct {
		char *name;
		unsigned int val;
	} of_fmt_table[] = {
		{ "i2s",	SND_SOC_DAIFMT_I2S },
		{ "right_j",	SND_SOC_DAIFMT_RIGHT_J },
		{ "left_j",	SND_SOC_DAIFMT_LEFT_J },
		{ "dsp_a",	SND_SOC_DAIFMT_DSP_A },
		{ "dsp_b",	SND_SOC_DAIFMT_DSP_B },
		{ "ac97",	SND_SOC_DAIFMT_AC97 },
		{ "pdm",	SND_SOC_DAIFMT_PDM },
		{ "msb",	SND_SOC_DAIFMT_MSB },
		{ "lsb",	SND_SOC_DAIFMT_LSB },
	};

	snd_soc_of_parse_card_name(card, PREFIX"card-name");

	for (i = 0; i < ARRAY_SIZE(of_dai_links_daifmt); i++) {
		struct snd_soc_dai_link *dai_link =
			&mt8696_evb_dais[of_dai_links_daifmt[i].link_id];
		bool lrck_inverse = false;
		bool bck_inverse = false;

		/* parse format */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-format",
			 of_dai_links_daifmt[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		ret = of_property_read_string(np, prop, &str);
		if (ret == 0) {
			unsigned int format = 0;

			for (j = 0; j < ARRAY_SIZE(of_fmt_table); j++) {
				if (strcmp(str, of_fmt_table[i].name) == 0) {
					format |= of_fmt_table[i].val;
					break;
				}
			}

			dai_link->dai_fmt &= ~SND_SOC_DAIFMT_FORMAT_MASK;
			dai_link->dai_fmt |= format;
		}

		/* parse clock mode */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-master-clock",
			 of_dai_links_daifmt[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0) {
			dai_link->dai_fmt &= ~SND_SOC_DAIFMT_MASTER_MASK;
			if (val)
				dai_link->dai_fmt |= SND_SOC_DAIFMT_CBS_CFS;
			else
				dai_link->dai_fmt |= SND_SOC_DAIFMT_CBM_CFM;
		}

		/* parse lrck inverse */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-lrck-inverse",
			 of_dai_links_daifmt[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		lrck_inverse = of_property_read_bool(np, prop);

		/* parse bck inverse */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-bck-inverse",
			 of_dai_links_daifmt[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		bck_inverse = of_property_read_bool(np, prop);

		dai_link->dai_fmt &= ~SND_SOC_DAIFMT_INV_MASK;

		if (lrck_inverse && bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_IB_IF;
		else if (lrck_inverse && !bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_IB_NF;
		else if (!lrck_inverse && bck_inverse)
			dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_IF;
		else
			dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_NF;
	}

	for (i = 0; i < ARRAY_SIZE(of_dai_links_etdm); i++) {
		unsigned int link_id = of_dai_links_etdm[i].link_id;
		struct snd_soc_dai_link *dai_link = &mt8696_evb_dais[link_id];
		struct mt8696_evb_etdm_ctrl_data *etdm;
		struct device_node *codec_node;

		/* parse mclk multiplier */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-mclk-multiplier",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0) {
			switch (link_id) {
			case DAI_LINK_ETDM1_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM1];
				etdm->mck_multp_out = val;
				break;
			case DAI_LINK_ETDM2_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->mck_multp_out = val;
				break;
			case DAI_LINK_ETDM2_IN:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->mck_multp_in = val;
				break;
			default:
				break;
			}
		}

		/* parse lrck width */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-lrck-width",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0) {
			switch (link_id) {
			case DAI_LINK_ETDM1_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM1];
				etdm->lrck_width_out = val;
				break;
			case DAI_LINK_ETDM2_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->lrck_width_out = val;
				break;
			case DAI_LINK_ETDM2_IN:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->lrck_width_in = val;
				break;
			default:
				break;
			}
		}

		/* parse fix rate */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-fix-rate",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0 && mt8696_afe_rate_supported(val,
		    link_to_dai(link_id))) {
			switch (link_id) {
			case DAI_LINK_ETDM1_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM1];
				etdm->fix_rate_out = val;
				break;
			case DAI_LINK_ETDM2_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->fix_rate_out = val;
				break;
			case DAI_LINK_ETDM2_IN:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->fix_rate_in = val;
				break;
			default:
				break;
			}

			dai_link->be_hw_params_fixup =
				mt8696_evb_be_hw_params_fixup;
		}

		/* parse fix bit width */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-fix-bit-width",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0 && (val == 32 || val == 16)) {
			switch (link_id) {
			case DAI_LINK_ETDM1_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM1];
				etdm->fix_bit_width_out = val;
				break;
			case DAI_LINK_ETDM2_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->fix_bit_width_out = val;
				break;
			case DAI_LINK_ETDM2_IN:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->fix_bit_width_in = val;
				break;
			default:
				break;
			}

			dai_link->be_hw_params_fixup =
				mt8696_evb_be_hw_params_fixup;
		}

		/* parse fix channels */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-fix-channels",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		ret = of_property_read_u32(np, prop, &val);
		if (ret == 0 && mt8696_afe_channel_supported(val,
		    link_to_dai(link_id))) {
			switch (link_id) {
			case DAI_LINK_ETDM1_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM1];
				etdm->fix_channels_out = val;
				break;
			case DAI_LINK_ETDM2_OUT:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->fix_channels_out = val;
				break;
			case DAI_LINK_ETDM2_IN:
				etdm = &priv->etdm_data[MT8696_ETDM2];
				etdm->fix_channels_in = val;
				break;
			default:
				break;
			}

			dai_link->be_hw_params_fixup =
				mt8696_evb_be_hw_params_fixup;
		}

		/* parse codec_of_node */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-audio-codec",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		codec_node = of_parse_phandle(np, prop, 0);
		if (codec_node) {
			dai_link->codecs->of_node = codec_node;
			dai_link->codecs->name = NULL;
		}

		/* parse codec dai name */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-codec-dai-name",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		of_property_read_string(np, prop, &dai_link->codecs->dai_name);

		/* parse ignore pmdown time */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-ignore-pmdown-time",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		if (of_property_read_bool(np, prop))
			dai_link->ignore_pmdown_time = 1;

		/* parse ignore suspend */
		sret = snprintf(prop, sizeof(prop), PREFIX"%s-ignore-suspend",
			 of_dai_links_etdm[i].name);
		if (sret < 0)
			dev_dbg(card->dev, "%s snprintf error\n", __func__);
		if (of_property_read_bool(np, prop))
			dai_link->ignore_suspend = 1;
		of_node_put(codec_node);
	}
}

static struct snd_soc_card mt8696_evb_card = {
	.name = "mt-snd-card",
	.long_name = "mt8696-evb-card",
	.owner = THIS_MODULE,
	.dai_link = mt8696_evb_dais,
	.num_links = ARRAY_SIZE(mt8696_evb_dais),
	.dapm_widgets = mt8696_evb_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt8696_evb_widgets),
	.dapm_routes = mt8696_evb_routes,
	.num_dapm_routes = ARRAY_SIZE(mt8696_evb_routes),
};

static int mt8696_evb_dev_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt8696_evb_card;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct mt8696_evb_priv *priv;
	int ret, id;
	size_t i;
	size_t dais_num = ARRAY_SIZE(mt8696_evb_dais);

	priv = devm_kzalloc(dev, sizeof(struct mt8696_evb_priv),
			    GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		dev_info(dev, "%s allocate card private data fail %d\n",
			__func__, ret);
		return ret;
	}

	priv->afe_plat_node = of_parse_phandle(dev->of_node, "mediatek,platform", 0);
	if (!priv->afe_plat_node) {
		dev_info(dev, "Property 'platform' missing or invalid\n");
		return -EINVAL;
	}

	for (i = 0; i < dais_num; i++) {
		if (mt8696_evb_dais[i].platforms->name)
			continue;

		id = mt8696_evb_dais[i].id;

		if ((id >= DAI_LINK_AFE_FE_BASE &&
		     id <= DAI_LINK_AFE_FE_END) ||
		    (id >= DAI_LINK_AFE_BE_BASE &&
		     id <= DAI_LINK_AFE_BE_END)) {
			mt8696_evb_dais[i].platforms->of_node = priv->afe_plat_node;
		}
	}

	card->dev = dev;

	snd_soc_card_set_drvdata(card, priv);

	mt8696_evb_gpio_probe(card);

	mt8696_evb_parse_of(card, np);

	ret = devm_snd_soc_register_card(dev, card);
	if (ret) {
		dev_info(dev, "%s snd_soc_register_card fail %d\n",
			__func__, ret);
		of_node_put(priv->afe_plat_node);
		return ret;
	}

	return ret;
}

static int mt8696_evb_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct mt8696_evb_priv *priv = snd_soc_card_get_drvdata(card);

	of_node_put(priv->afe_plat_node);
	return 0;
}

static const struct of_device_id mt8696_evb_dt_match[] = {
	{ .compatible = "mediatek,mt8696-evb", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt8696_evb_dt_match);

static struct platform_driver mt8696_evb_driver = {
	.driver = {
		   .name = "mt8696-evb",
		   .of_match_table = mt8696_evb_dt_match,
#ifdef CONFIG_PM
		   .pm = &snd_soc_pm_ops,
#endif
	},
	.probe = mt8696_evb_dev_probe,
	.remove = mt8696_evb_dev_remove,
};

module_platform_driver(mt8696_evb_driver);

/* Module information */
MODULE_DESCRIPTION("MT8696 EVB SoC machine driver");
MODULE_AUTHOR("Sail Yang <sail.yang@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:mt8696-evb");

