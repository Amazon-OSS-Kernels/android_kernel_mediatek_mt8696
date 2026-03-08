/*
 * mt8532-snd-utils.h  --  Mediatek 8532 sound utility
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

#include "mt8532-snd-utils.h"
#include "mt8532-afe-common.h"
#include <sound/core.h>
#include <sound/control.h>
#include <sound/soc.h>


static struct snd_kcontrol *snd_ctl_find_name(struct snd_card *card,
	unsigned char *name)
{
	struct snd_kcontrol *kctl;

	if (!card || !name) {
		pr_info("%s null handle\n", __func__);
		return NULL;
	}

	list_for_each_entry(kctl, &card->controls, list) {
		if (!strncmp(kctl->id.name, name, sizeof(kctl->id.name)))
			return kctl;
	}

	return NULL;
}

int mt8532_snd_ctl_notify(struct snd_card *card,
	unsigned char *ctl_name, unsigned int mask)
{

	struct snd_kcontrol *kctl;

	kctl = snd_ctl_find_name(card, ctl_name);
	if (!kctl) {
		pr_info("%s can not find ctl %s\n", __func__, ctl_name);
		return -1;
	}

	snd_ctl_notify(card, mask, &kctl->id);

	return 0;
}

unsigned int mt8532_snd_get_dai_format(const char *fmt_str)
{
	size_t i;

	static const struct {
		char *name;
		unsigned int val;
	} fmt_table[] = {
		{"i2s", SND_SOC_DAIFMT_I2S},
		{"right_j", SND_SOC_DAIFMT_RIGHT_J},
		{"left_j", SND_SOC_DAIFMT_LEFT_J},
		{"dsp_a", SND_SOC_DAIFMT_DSP_A},
		{"dsp_b", SND_SOC_DAIFMT_DSP_B},
		{"ac97", SND_SOC_DAIFMT_AC97},
		{"pdm", SND_SOC_DAIFMT_PDM},
		{"msb", SND_SOC_DAIFMT_MSB},
		{"lsb", SND_SOC_DAIFMT_LSB},
	};

	for (i = 0; i < ARRAY_SIZE(fmt_table); i++) {
		if (strcmp(fmt_str, fmt_table[i].name) == 0)
			return fmt_table[i].val;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mt8532_snd_get_dai_format);

int mt8532_snd_get_etdm_format(unsigned int dai_fmt)
{
	switch (dai_fmt) {
	case SND_SOC_DAIFMT_I2S:
		return MT8532_ETDM_FORMAT_I2S;
	case SND_SOC_DAIFMT_LEFT_J:
		return MT8532_ETDM_FORMAT_LJ;
	case SND_SOC_DAIFMT_RIGHT_J:
		return MT8532_ETDM_FORMAT_RJ;
	case SND_SOC_DAIFMT_DSP_A:
		return MT8532_ETDM_FORMAT_DSPA;
	case SND_SOC_DAIFMT_DSP_B:
		return MT8532_ETDM_FORMAT_DSPB;
	default:
		break;
	}

	return -1;
}

