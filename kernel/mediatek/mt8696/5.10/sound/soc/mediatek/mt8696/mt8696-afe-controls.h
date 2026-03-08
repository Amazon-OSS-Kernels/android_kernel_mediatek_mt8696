/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8696-afe-controls.h  --  Mediatek 8696 audio controls
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Sail Yang <sail.yang@mediatek.com>
 *
 */

#ifndef _MT8696_AFE_CONTROLS_H_
#define _MT8696_AFE_CONTROLS_H_

struct snd_soc_component;

int mt8696_afe_add_controls(struct snd_soc_component *platform);

#endif
