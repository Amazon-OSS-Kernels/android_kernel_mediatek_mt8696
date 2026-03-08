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

#ifndef _MT8532_SND_UTILS_H_
#define _MT8532_SND_UTILS_H_

struct snd_card;

int mt8532_snd_ctl_notify(struct snd_card *card,
	unsigned char *ctl_name, unsigned int mask);

unsigned int mt8532_snd_get_dai_format(const char *fmt_str);

int mt8532_snd_get_etdm_format(unsigned int dai_fmt);

#endif
