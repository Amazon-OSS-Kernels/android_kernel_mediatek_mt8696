/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef _IRT_IF_H_
#define _IRT_IF_H_

#include <linux/spinlock.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <uapi/linux/dma-heap.h>
#include <linux/dma-direction.h>
#include <linux/scatterlist.h>

enum IRT_STATE {
	IRT_STATE_IDLE = 0,
	IRT_STATE_BUSY = 1,
	IRT_STATE_DONE = 2,
	IRT_STATE_TIMEOUT = 3,
	IRT_STATE_STOP = 4,
};

enum irt_cb_state {
	IRT_CB_DONE,		/* Successfully. */
	IRT_CB_TIMEOUT,		/* HW timeout. */
	IRT_CB_STOP,
};

typedef int (*irt_callback_fun) (enum irt_cb_state hw_state, void *privdata);

struct irt_data {
	void __iomem *irt_reg_base;
	void __iomem *disp_top_reg_base;
	int irq;
	struct clk *clk;
	irt_callback_fun fun_cb;
	void *cb_data;
	enum IRT_STATE state;
	spinlock_t state_lock;
	struct device *dev;
	struct device *larb_dev;

	wait_queue_head_t wait_irt_irq_wq;
	atomic_t wait_irt_irq_flag;
};

enum IRT_DMA_ROT_MODE {
	IRT_DMA_MODE_ROTATE_0 = 0,      /* 0, normal */
	IRT_DMA_MODE_ROTATE_0_MIRROR,   /* 1, Rotate 0 + Mirror */
	IRT_DMA_MODE_ROTATE_90,         /* 2, Rotate 90 */
	IRT_DMA_MODE_ROTATE_90_MIRROR,  /* 3, Rotate 90 + Mirror */
	IRT_DMA_MODE_ROTATE_180,        /* 4, Rotate 180 */
	IRT_DMA_MODE_ROTATE_180_MIRROR, /* 5, Rotate 180 + Mirror */
	IRT_DMA_MODE_ROTATE_270,        /* 6, Rotate 270 */
	IRT_DMA_MODE_ROTATE_270_MIRROR, /* 7, Rotate 270 + Mirror*/
	IRT_DMA_MODE_ROTATE_UNKNOWN,	/* HW NOT supported */
};

enum IRT_DMA_DITHER_MODE {
	IRT_DMA_DITHER_DROP_LSB = 0,
	IRT_DMA_DITHER_LSB_ROUND,
	IRT_DMA_DITHER_MSB_LSB_ROUND,
	IRT_DMA_DITHER_MSB_XOR_LSB_ROUND,
	IRT_DMA_DITHER_MSB_ROUND,
	IRT_DMA_DITHER_UNKNOWN,
};

enum IRT_DMA_SRC_COLOR_MODE {
	IRT_DMA_SRC_COL_MD_YC420_8BIT_BLK = 0,
	IRT_DMA_SRC_COL_MD_YC420_8BIT_BLK_5351,
	IRT_DMA_SRC_COL_MD_YC420_8BIT_SCL,
	IRT_DMA_SRC_COL_MD_YC420_10BIT_RASTER,
	IRT_DMA_SRC_COL_MD_YC420_10BIT_TILE,
	IRT_DMA_SRC_COL_MD_UNKNOWN,
};

enum IRT_DMA_DST_COLOR_MODE {
	IRT_DMA_DST_COL_MD_YC420_8BIT_SCL = 0,
	IRT_DMA_DST_COL_MD_YC420_10BIT_SCL,
	IRT_DMA_DST_COL_MD_UNKNOWN,
};

struct irt_dma_info {
	enum IRT_DMA_ROT_MODE rotate_mode;
	enum IRT_DMA_DITHER_MODE dither_mode;
	enum IRT_DMA_SRC_COLOR_MODE src_color_fmt;
	enum IRT_DMA_DST_COLOR_MODE dst_color_fmt;

	unsigned int src_width_align;
	unsigned int src_height_align;

	unsigned int src_offset_y_len;
	unsigned int src_offset_c_len;
	unsigned int dst_offset_y_len;
	unsigned int dst_offset_c_len;

	int src_fd;
	int dst_fd;
};

void irt_set_log_enable(unsigned int en);
int irt_ticket_get(void);
int irt_ticket_put(void);
int irt_set_callback_fun(irt_callback_fun fun, void *privdata);
int irt_dma_trigger_sync(struct irt_dma_info *irt_info);
struct irt_data *irt_get_data(void);

#endif				/* _IRT_IF_H_ */
