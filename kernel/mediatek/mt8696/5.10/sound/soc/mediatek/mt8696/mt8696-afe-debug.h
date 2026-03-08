/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8696-afe-debug.h  --  Mediatek 8696 audio debugfs
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Sail Yang <sail.yang@mediatek.com>
 *
 */

#ifndef __MT8696_AFE_DEBUG_H__
#define __MT8696_AFE_DEBUG_H__

struct mtk_base_afe;

void mt8696_afe_init_debugfs(struct mtk_base_afe *afe);

void mt8696_afe_cleanup_debugfs(struct mtk_base_afe *afe);

#endif
