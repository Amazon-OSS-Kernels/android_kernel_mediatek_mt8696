/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8532-afe-controls.h  --  Mediatek 8532 audio controls
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
 */

#ifndef _MT8532_AFE_CONTROLS_H_
#define _MT8532_AFE_CONTROLS_H_

struct snd_soc_component;

int mt8532_afe_add_controls(struct snd_soc_component *platform);

#endif
