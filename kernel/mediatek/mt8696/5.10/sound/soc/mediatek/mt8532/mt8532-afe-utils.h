/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8532-afe-utils.h  --  Mediatek 8532 audio utility
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
 */

#ifndef _MT8532_AFE_UTILS_H_
#define _MT8532_AFE_UTILS_H_

#include <linux/types.h>

struct mtk_base_afe;
struct clk;

int mt8532_afe_init_audio_clk(struct mtk_base_afe *afe);

int mt8532_afe_enable_clk(struct mtk_base_afe *afe, struct clk *clk);

void mt8532_afe_disable_clk(struct mtk_base_afe *afe, struct clk *clk);

int mt8532_afe_set_clk_rate(struct mtk_base_afe *afe, struct clk *clk,
			    unsigned int rate);

int mt8532_afe_set_clk_parent(struct mtk_base_afe *afe, struct clk *clk,
			      struct clk *parent);

int mt8532_afe_enable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type);

int mt8532_afe_disable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type);

int mt8532_afe_enable_main_clk(struct mtk_base_afe *afe);

int mt8532_afe_disable_main_clk(struct mtk_base_afe *afe);

int mt8532_afe_enable_reg_rw_clk(struct mtk_base_afe *afe);

int mt8532_afe_disable_reg_rw_clk(struct mtk_base_afe *afe);

int mt8532_afe_enable_afe_on(struct mtk_base_afe *afe);

int mt8532_afe_disable_afe_on(struct mtk_base_afe *afe);

int mt8532_afe_enable_irq(struct mtk_base_afe *afe, unsigned int irq_id);

int mt8532_afe_disable_irq(struct mtk_base_afe *afe, unsigned int irq_id);

int mt8532_afe_handle_spdif_in_port_change(struct mtk_base_afe *afe,
	unsigned int port);

int mt8532_afe_block_dpidle(struct mtk_base_afe *afe);

int mt8532_afe_unblock_dpidle(struct mtk_base_afe *afe);

int mt8532_afe_get_be_idx(int id);

int mt8532_afe_set_be_active(struct mtk_base_afe *afe, int id);

int mt8532_afe_clear_be_active(struct mtk_base_afe *afe, int id);

bool mt8532_afe_is_be_active(struct mtk_base_afe *afe, int id);

#endif
