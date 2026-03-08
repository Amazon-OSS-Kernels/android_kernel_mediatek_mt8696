/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 *
 */

#ifndef _MTK_IMGSYS_FENCES_H_
#define _MTK_IMGSYS_FENCES_H_

#include <linux/sync_file.h>
#include <linux/dma-fence.h>
#include <linux/spinlock.h>

struct mtk_imgsys_fence_context {
	unsigned int context;
	uint32_t last_fence;
	spinlock_t spinlock;
};

struct mtk_imgsys_dev_buffer_fence {
	struct dma_fence base;
	int fence_fd;
	struct mtk_imgsys_fence_context *ctx;
};

struct mtk_imgsys_fence_context *mtk_imgsys_fence_ctx_alloc(void);

struct mtk_imgsys_dev_buffer_fence *mtk_imgsys_fence_alloc(
		struct mtk_imgsys_fence_context *fence_ctx);

int mtk_imgsys_fence_signal(struct mtk_imgsys_dev_buffer_fence *buf_fence);

void mtk_imgsys_fence_ctx_free(struct mtk_imgsys_fence_context *fence_ctx);

#endif /* _MTK_IMGSYS_FENCES_H_ */
