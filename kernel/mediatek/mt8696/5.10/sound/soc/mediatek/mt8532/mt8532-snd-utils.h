/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8532-snd-utils.h  --  Mediatek 8532 sound utility
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
 */

#ifndef _MT8532_SND_UTILS_H_
#define _MT8532_SND_UTILS_H_

struct snd_card;

int mt8532_snd_ctl_notify(struct snd_card *card,
	unsigned char *ctl_name, unsigned int mask);

unsigned int mt8532_snd_get_dai_format(const char *fmt_str);

int mt8532_snd_get_etdm_format(unsigned int dai_fmt);

#endif
