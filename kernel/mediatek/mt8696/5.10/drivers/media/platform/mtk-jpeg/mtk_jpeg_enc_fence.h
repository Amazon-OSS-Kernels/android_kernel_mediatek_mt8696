/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 *
 *
 */

#ifndef _MTK_JPEG_FENCES_H_
#define _MTK_JPEG_FENCES_H_

#include <linux/sync_file.h>
#include <linux/dma-fence.h>
#include <linux/spinlock.h>
#include <mtk_sync.h>
#include "mtk_jpeg_core.h"

int mtk_jpeg_fence_create(struct mtk_jpeg_ctx *ctx, struct mtk_v4l2_fence_data **buf_fence);

int mtk_jpeg_fence_signal(struct mtk_jpeg_ctx *ctx, struct mtk_v4l2_fence_data *buf_fence);

#endif /* _MTK_IMGSYS_FENCES_H_ */
