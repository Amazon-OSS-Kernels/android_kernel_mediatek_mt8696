/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __DISP_R2R_IF_H__
#define __DISP_R2R_IF_H__

#include "disp_info.h"
#include <linux/list.h>
#include <linux/mutex.h>
#include "disp_hw_mgr.h"


#define R2R_DRV_NAME "disp_drv_r2r"
#define R2R_NO_ION_FD ((int)(~0U >> 1))

#define R2R_INVALID_ION_FD (-1)
#define R2R_INVALID_FENCE_FD (-1)

struct R2R_ACTIVE_SHIFT_T {
	uint32_t hshitf;
	uint32_t hpos;
	uint32_t vshitf;
	uint32_t vpos;
};

enum R2R_LAYER_STATE {
	R2R_LAYER_IDLE,
	R2R_LAYER_RUNNING,
	R2R_LAYER_STOPPING,
	R2R_LAYER_STOPPED,
	R2R_LAYER_START,
};

enum R2R_LAYER_TYPE {
	R2R_LAYER_HDMIRX,
	R2R_LAYER_VID,
	R2R_LAYER_GFX,
	R2R_LAYER_SIDEBAND,
	R2R_LAYER_AEE,
	R2R_LAYER_UNKNOWN
};

struct r2r_layer_info {
	unsigned int inited;
	struct mutex sync_lock;
	unsigned int layer_id;
	struct work_struct task_work;
	wait_queue_head_t wait_queue;
	unsigned int fence_idx;
	unsigned int timeline_idx;
	unsigned int release_timeline_idx;
	unsigned int release_idx;
	unsigned int fence_fd;
	struct sw_sync_timeline *timeline;
	struct list_head buf_list;
	uint64_t last_pts;

	enum R2R_LAYER_STATE state;
	unsigned int display_duration;
	unsigned int vsync_duration;
	bool enable;
	bool layer_start;

	struct mtk_disp_range src_rgn;
	enum R2R_LAYER_TYPE type;
	struct r2r_video_buffer_info *video_buf;
};

struct r2r_layer_info  *get_r2r_layer_info(void);
int disp_r2r_start(struct disp_hw_common_info *info,
	unsigned int layer_id);
int disp_r2r_stop(unsigned int layer_id);

#endif
