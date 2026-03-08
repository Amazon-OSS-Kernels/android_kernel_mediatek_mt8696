/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8532-afe-debug.h  --  Mediatek 8532 audio debugfs
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
 */

#ifndef __MT8532_AFE_DEBUG_H__
#define __MT8532_AFE_DEBUG_H__

struct mtk_base_afe;

void mt8532_afe_init_debugfs(struct mtk_base_afe *afe);

void mt8532_afe_cleanup_debugfs(struct mtk_base_afe *afe);

#endif
