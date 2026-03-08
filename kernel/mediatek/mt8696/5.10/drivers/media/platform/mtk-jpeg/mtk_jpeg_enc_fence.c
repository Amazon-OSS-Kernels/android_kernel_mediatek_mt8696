// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 *
 */
#define DEBUG

#include <linux/dma-iommu.h>
#include <linux/freezer.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/file.h>
#include <media/v4l2-event.h>
#include "mtk_jpeg_enc_fence.h"
#include "mtk_jpeg_core.h"

static const char *mtk_jpeg_fence_get_driver_name(struct dma_fence *fence)
{
	return "mtk_jpeg";
}

static const char *mtk_jpeg_fence_get_timeline_name(struct dma_fence *fence)
{
	return "mtk_jpeg_timeline";
}
static bool mtk_jpeg_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static const struct dma_fence_ops mtk_jpeg_fence_ops = {
	.get_driver_name = mtk_jpeg_fence_get_driver_name,
	.get_timeline_name = mtk_jpeg_fence_get_timeline_name,
	.enable_signaling = mtk_jpeg_fence_enable_signaling,
	.wait = dma_fence_default_wait,
};

int mtk_jpeg_fence_create(struct mtk_jpeg_ctx *ctx, struct mtk_v4l2_fence_data **buf_fence)
{
	*buf_fence = (kzalloc(sizeof(**buf_fence), GFP_KERNEL));
	if (!*buf_fence)
		return -1;

	dma_fence_init(&(*buf_fence)->base, &mtk_jpeg_fence_ops, &ctx->fence_lock,
			ctx->fence_context, ++ctx->fence_seqno);


	/* dma_fence_get will be called during sync_file_create. */
	(*buf_fence)->sync_file = sync_file_create(&(*buf_fence)->base);
	if (!(*buf_fence)->sync_file)
		return -ENOMEM;

	(*buf_fence)->fence_fd = get_unused_fd_flags(O_CLOEXEC);
	fd_install((*buf_fence)->fence_fd, (*buf_fence)->sync_file->file);

	pr_info("%s buf_fence: context:%d, fence_fd:%d\n",
		__func__, ctx->fence_context, (*buf_fence)->fence_fd);

	return 0;
}

int mtk_jpeg_fence_signal(struct mtk_jpeg_ctx *ctx, struct mtk_v4l2_fence_data *buf_fence)
{
	if (!buf_fence->sync_file)
		return -EINVAL;

	pr_info("%s buf_fence: context:%d, fence_fd:%d\n",
		__func__, ctx->fence_context, buf_fence->fence_fd);

	dma_fence_signal(&buf_fence->base);

	/* dma_fence_release will be called when refcnt is zero. */
	dma_fence_put(&buf_fence->base);
	buf_fence->fence_fd = -1;

	return 0;
}
