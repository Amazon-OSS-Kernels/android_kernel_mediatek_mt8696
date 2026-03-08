// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 *
 */

#include <linux/device.h>
#include <linux/dma-iommu.h>
#include <linux/freezer.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <media/v4l2-event.h>
#include "mtk_imgsys-dev.h"
#include "mtk_imgsys-hw.h"
#include "mtk_imgsys-sys.h"
#include "mtk_imgsys-fence.h"


static inline struct mtk_imgsys_dev_buffer_fence*
mtk_imgsys_dma_fence_to_fence_ctx(struct dma_fence *fence)
{
	return container_of(fence, struct mtk_imgsys_dev_buffer_fence, base);
}

static const char *mtk_imgsys_fence_get_driver_name(struct dma_fence *fence)
{
	return "mtk_imgsys";
}

static const char *mtk_imgsys_fence_get_timeline_name(struct dma_fence *fence)
{
	return "mtk_imgsys_timeline";
}

static signed long mtk_imgsys_fence_wait(struct dma_fence *fence, bool intr, signed long timeout)
{
	struct mtk_imgsys_dev_buffer_fence *buf_fence = mtk_imgsys_dma_fence_to_fence_ctx(fence);

	pr_info("%s buf_fence: context:%d, seqno:%d, fence_fd:%d\n",
			__func__,
			buf_fence->ctx->context,
			fence->seqno,
			buf_fence->fence_fd);

	return dma_fence_default_wait(fence, intr, timeout);
}

static const struct dma_fence_ops mtk_imgsys_fence_ops = {
	.get_driver_name = mtk_imgsys_fence_get_driver_name,
	.get_timeline_name = mtk_imgsys_fence_get_timeline_name,
	.wait = mtk_imgsys_fence_wait,
};

struct mtk_imgsys_fence_context *mtk_imgsys_fence_ctx_alloc(void)
{
	struct mtk_imgsys_fence_context *fence_ctx;

	fence_ctx = kzalloc(sizeof(*fence_ctx), GFP_KERNEL);
	if (!fence_ctx)
		return ERR_PTR(-ENOMEM);

	fence_ctx->context = dma_fence_context_alloc(1);
	fence_ctx->last_fence = 0;
	spin_lock_init(&fence_ctx->spinlock);

	return fence_ctx;
}

struct mtk_imgsys_dev_buffer_fence *mtk_imgsys_fence_alloc(
		struct mtk_imgsys_fence_context *fence_ctx)
{
	struct sync_file *sync_file;
	struct mtk_imgsys_dev_buffer_fence *buf_fence;
	int fence_fd = -1;

	if (!fence_ctx)
		return ERR_PTR(-EINVAL);

	fence_fd = get_unused_fd_flags(O_CLOEXEC);
	if (fence_fd < 0) {
		pr_info("%s get unused fd failed: fence_fd:%d\n", __func__, fence_fd);
		return ERR_PTR(-EINVAL);
	}

	buf_fence = kzalloc(sizeof(*buf_fence), GFP_KERNEL);
	if (!buf_fence)
		return ERR_PTR(-ENOMEM);

	dma_fence_init(&buf_fence->base, &mtk_imgsys_fence_ops, &fence_ctx->spinlock,
			fence_ctx->context, ++fence_ctx->last_fence);

	sync_file = sync_file_create(&buf_fence->base);
	if (!sync_file) {
		kfree(buf_fence);
		return ERR_PTR(-ENOMEM);
	}

	buf_fence->ctx = fence_ctx;
	buf_fence->fence_fd = fence_fd;
	fd_install(buf_fence->fence_fd, sync_file->file);

	pr_info("%s buf_fence: context:%d, seqno:%d fence_fd:%d\n",
			__func__,
			buf_fence->ctx->context,
			buf_fence->base.seqno,
			buf_fence->fence_fd);

	return buf_fence;
}

int mtk_imgsys_fence_signal(struct mtk_imgsys_dev_buffer_fence *buf_fence)
{
	dma_fence_signal(&buf_fence->base);

	dma_fence_put(&buf_fence->base);

	pr_info("%s buf_fence: context:%d, seqno:%d fence_fd:%d\n",
			__func__,
			buf_fence->ctx->context,
			buf_fence->base.seqno,
			buf_fence->fence_fd);

	return 0;
}

inline void mtk_imgsys_fence_ctx_free(struct mtk_imgsys_fence_context *fence_ctx)
{
	kfree(fence_ctx);
}
