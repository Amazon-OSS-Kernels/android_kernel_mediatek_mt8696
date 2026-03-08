/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __DISP_FG_PRE_SCALE_H__
#define __DISP_FG_PRE_SCALE_H__

void pre_up_scale_y(
	int *src_buffer,
	int *dst_buffer_v,
	int *dst_buffer_h,
	unsigned int src_width,
	unsigned int src_height,
	unsigned int dst_width,
	unsigned int dst_height);

void pre_up_scale_c(
	int *src_buffer,
	int *dst_buffer_v,
	int *dst_buffer_h,
	unsigned int src_width,
	unsigned int src_height,
	unsigned int dst_width,
	unsigned int dst_height);

void pre_down_scale(
	int *src_buffer,
	int *dst_buffer_v,
	int *dst_buffer_h,
	unsigned int src_width,
	unsigned int src_height,
	unsigned int dst_width,
	unsigned int dst_height);
#endif /* __DISP_FG_PRE_SCALE_H__ */
