/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef _MTK_VQ_INFO_H_
#define _MTK_VQ_INFO_H_

#include <linux/dma-buf.h>
#include "linux/types.h"

#define DI_ASYNC_SUPPORT   0

#define MTK_VQ_BUFFER_COUINT  (5)

#define MTK_VQ_DI_INPUTBUFFER  (3)

enum VQ_FIELD_TYPE {
	VQ_FIELD_TYPE_TOP,
	VQ_FIELD_TYPE_BOTTOM,
	VQ_FIELD_TYPE_MAX
};

enum VQ_DI_MODE {
	VQ_DI_MODE_FRAME,
	VQ_DI_MODE_4_FIELD,
	VQ_DI_MODE_FIELD,
	VQ_DI_MODE_MAX
};

enum VQ_COLOR_FMT {
	VQ_COLOR_FMT_420BLK,
	VQ_COLOR_FMT_420SCL,
	VQ_COLOR_FMT_422BLK,
	VQ_COLOR_FMT_422SCL,
	VQ_COLOR_FMT_MAX
};

enum VQ_PATH_MODE {
	VQ_DI_NR_DIRECTLINK_ALL_ENABLE = 0,
	VQ_DI_NR_DIRECTLINK_DI_BYPASS,
	VQ_DI_NR_DIRECTLINK_NR_BYPASS,
	VQ_DI_STANDALONE,
	VQ_NR_STANDALONE
};

enum VQ_TIMING_TYPE {
	VQ_TIMING_TYPE_480P,
	VQ_TIMING_TYPE_576P,
	VQ_TIMING_TYPE_720P,
	VQ_TIMING_TYPE_1080P,
	VQ_TIMING_TYPE_MAX
};
enum VDO_DI_MODE {
	VDO_FRAME_MODE,
	VDO_FIELD_MODE,
	VDO_INTRA_MODE_WITH_EDGE_PRESERVING,
	VDO_4FIELD_MA_MODE,
	VDO_FUSION_MODE,
	VDO_8FIELD_MA_MODE,
	VDO_DI_MODE_NUM
};

struct mtk_vq_config {
	enum VQ_PATH_MODE vq_mode;

	enum VQ_COLOR_FMT src_fmt;
	enum VQ_COLOR_FMT dst_fmt;

	int src_width;
	int src_height;

	int src_align_width;
	int src_align_height;

	unsigned int src_ofset_y_len[MTK_VQ_BUFFER_COUINT];
	unsigned int src_ofset_c_len[MTK_VQ_BUFFER_COUINT];
	unsigned int dst_ofset_y_len;
	unsigned int dst_ofset_c_len;

	/*dma fd */
	int src_fd[MTK_VQ_BUFFER_COUINT];
	int dst_fd;

	/*NR config */
	unsigned int bnr_level;
	unsigned int mnr_level;
	enum VQ_DI_MODE di_mode;
	enum VQ_FIELD_TYPE cur_field;

	bool topfield_first_enable;
	bool h265_enable;
};

struct mtk_vq_dma {
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	unsigned int dma_addr;
};

struct mtk_vq_config_info {
	struct mtk_vq_config *vq_config;

	unsigned int src_mva[MTK_VQ_BUFFER_COUINT];
	unsigned int dst_mva;

	void *src_va[MTK_VQ_BUFFER_COUINT];
	void *dst_va;

	struct mtk_vq_dma src_vq_dma[MTK_VQ_BUFFER_COUINT];
	struct mtk_vq_dma dst_vq_dma;
};

#define MTK_VQ_IOW(num, dtype)     _IOW('O', num, dtype)
#define MTK_VQ_IOR(num, dtype)     _IOR('O', num, dtype)
#define MTK_VQ_IOWR(num, dtype)    _IOWR('O', num, dtype)
#define MTK_VQ_IO(num)             _IO('O', num)

#define MTK_VQ_IOCTL_SET_INPUT_CONFIG    MTK_VQ_IOWR(0xe0, struct mtk_vq_config)

#endif				/* _MTK_VQ_INFO_H_ */
