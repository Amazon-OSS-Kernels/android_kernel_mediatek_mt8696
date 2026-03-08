/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk_asrc_export_api.h  -- export asrc apis for other drivers.
 *
 * Copyright (C) 2022 MediaTek Inc.
 *
 * Author: Ken Tai <ken.tai@mediatek.com>
 */

#ifndef MTK_ASRC_EXPORT_H_
#define MTK_ASRC_EXPORT_H_

#include "mt8188-asrc-hw.h"

int mtk_asrc_open(enum afe_mem_asrc_id id);
int mtk_asrc_release(enum afe_mem_asrc_id id);
int mtk_asrc_cali_start(enum afe_mem_asrc_id id,
	enum afe_mem_asrc_tracking_mode tracking_mode,
	enum afe_mem_asrc_tracking_source tracking_src,
	enum afe_mem_asrc_cali_clk cali_clk,
	u32 cali_cycle,
	u32 input_freq,
	u32 output_freq);
u64 mtk_asrc_cali_get_result(enum afe_mem_asrc_id id);
int mtk_asrc_stop(enum afe_mem_asrc_id id);
#endif
