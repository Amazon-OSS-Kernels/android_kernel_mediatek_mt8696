/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8696-afe-utils.h  --  Mediatek 8696 audio utility
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Sail Yang <sail.yang@mediatek.com>
 *
 */

#ifndef _MT8696_AFE_UTILS_H_
#define _MT8696_AFE_UTILS_H_
#include <sound/soc.h>

struct mtk_base_afe;
struct clk;

int mt8696_afe_init_audio_clk(struct mtk_base_afe *afe);

int mt8696_afe_enable_clk(struct mtk_base_afe *afe, struct clk *clk);

void mt8696_afe_disable_clk(struct mtk_base_afe *afe, struct clk *clk);

int mt8696_afe_set_clk_rate(struct mtk_base_afe *afe, struct clk *clk,
			    unsigned int rate);

int mt8696_afe_set_clk_parent(struct mtk_base_afe *afe, struct clk *clk,
			      struct clk *parent);

int mt8696_afe_enable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type);

int mt8696_afe_disable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type);

int mt8696_afe_is_top_cg_on(struct mtk_base_afe *afe, unsigned int cg_type);

int mt8696_afe_enable_main_clk(struct mtk_base_afe *afe);

int mt8696_afe_disable_main_clk(struct mtk_base_afe *afe);

int mt8696_afe_enable_reg_rw_clk(struct mtk_base_afe *afe);

int mt8696_afe_disable_reg_rw_clk(struct mtk_base_afe *afe);

int mt8696_afe_enable_afe_on(struct mtk_base_afe *afe);

int mt8696_afe_disable_afe_on(struct mtk_base_afe *afe);

int mt8696_afe_enable_irq(struct mtk_base_afe *afe, unsigned int irq_id);

int mt8696_afe_disable_irq(struct mtk_base_afe *afe, unsigned int irq_id);

int mt8696_afe_block_dpidle(struct mtk_base_afe *afe);

int mt8696_afe_unblock_dpidle(struct mtk_base_afe *afe);

int mt8696_afe_enable_apll_tuner_cfg(struct mtk_base_afe *afe,
	unsigned int apll);

int mt8696_afe_disable_apll_tuner_cfg(struct mtk_base_afe *afe,
	unsigned int apll);

int mt8696_afe_enable_apll_associated_cfg(struct mtk_base_afe *afe,
	unsigned int apll);

int mt8696_afe_disable_apll_associated_cfg(struct mtk_base_afe *afe,
	unsigned int apll);

int mt8696_afe_dump_all_registers(struct mtk_base_afe *afe);

extern u64 mtk_timer_get_cnt(u8 timer);

int snd_pcm_update_hw_ptr(struct snd_pcm_substream *substream);
#endif
