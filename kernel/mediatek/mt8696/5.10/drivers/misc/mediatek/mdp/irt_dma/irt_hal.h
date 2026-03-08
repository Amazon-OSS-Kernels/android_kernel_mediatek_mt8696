/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef _IRT_HAL_H_
#define _IRT_HAL_H_

#include <linux/dma-buf.h>

struct mtk_irt_dma {
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	unsigned int dma_addr;
};

struct irt_dma_config_param {
	unsigned int rg_5351_mode_en;
	unsigned int rg_5351_mode_sel;
	unsigned int rg_scan_line;
	unsigned int rg_rotate_mode;
	unsigned int rg_blk_burst_en;
	unsigned int rg_align_vsize;
	unsigned int rg_align_hsize;
	unsigned int rg_y_dram_rd_addr;
	unsigned int rg_c_dram_rd_addr;
	unsigned int rg_y_dram_wr_addr;
	unsigned int rg_c_dram_wr_addr;

	unsigned int rg_src_10bit_en;
	unsigned int rg_10bit_rotate_en;
	unsigned int rg_10bit_mode_sel;
	unsigned int rg_wr_garbage_cancel;
	unsigned int rg_dither_mode;
	unsigned int rg_dither_en;

	unsigned int src_offset_y_len;
	unsigned int src_offset_c_len;
	unsigned int dst_offset_y_len;
	unsigned int dst_offset_c_len;

	int src_fd;
	int dst_fd;

	struct mtk_irt_dma src_irt_dma;
	struct mtk_irt_dma dst_irt_dma;
};

void irt_dma_hal_clear_irq(void __iomem *reg_base);
void irt_dma_hal_hw_reset(void __iomem *reg_base);
int irt_dma_hal_config(struct irt_dma_config_param *config,
	void __iomem *reg_base);
#endif	/*_IRT_HAL_H_*/
