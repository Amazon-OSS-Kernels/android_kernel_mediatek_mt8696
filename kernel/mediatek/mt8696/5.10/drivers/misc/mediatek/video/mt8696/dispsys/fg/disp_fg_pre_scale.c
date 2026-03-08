// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/kernel.h>

#include "disp_hw_mgr.h"
#include "disp_adl_if.h"
#include "disp_fg_if.h"
#include "fg_hal.h"
#include "disp_info.h"
#include "disp_fg_drv.h"
#include "disp_fg_pre_drv.h"

#define V_SHIFT_BITS	6
#define H_SHIFT_BITS	9
#define SCALE_AVERAGE	2
#define VALID_VALUE	(0x1F << 6)
#define MAX_VALUE	(0x1 << 5)

#define H_VALID_VALUE	(0x7 << 9)
#define H_MAX_VALUE	(0x1 << 3)

#define V_FACTOR	0x800
#define V_C_FACTOR	0x1000
#define H_FACTOR	0x1000

void pre_up_scale_y(
	int *src_buffer,
	int *dst_buffer_v,
	int *dst_buffer_h,
	unsigned int src_width,
	unsigned int src_height,
	unsigned int dst_width,
	unsigned int dst_height)
{
	unsigned int src_i = 0;
	unsigned int src_j = 0;

	unsigned int v_scale = 0;
	unsigned int h_scale = 0;
	unsigned int v_weight = 0;
	unsigned int h_weight = 0;

	unsigned int dst_i = 0;
	unsigned int dst_j = 0;
	unsigned int tmp_factor = 0;

	v_scale = (src_height * V_FACTOR) / dst_height;
	h_scale = (src_width * H_FACTOR) / dst_width;

	/* vertical scale */
	for (dst_i = 0; (dst_i < dst_height) && (src_i < (src_height - 1));
	     dst_i++) {
		for (dst_j = 0; dst_j < src_width; dst_j++) {
			tmp_factor = ((v_weight & VALID_VALUE) >> V_SHIFT_BITS);
			dst_buffer_v[dst_i * src_width + dst_j] =
				(src_buffer[src_i * src_width + dst_j] *
					(MAX_VALUE - tmp_factor) +
				 src_buffer[(src_i + 1) * src_width + dst_j] *
					(tmp_factor) + (MAX_VALUE) /
					SCALE_AVERAGE) /
				 (MAX_VALUE);
		}

		v_weight += v_scale;
		if (v_weight >= V_FACTOR) {
			v_weight -= V_FACTOR;
			src_i++;
		}
	}

	for (; dst_i < dst_height; dst_i++)
		for (dst_j = 0; dst_j < src_width; dst_j++)
			dst_buffer_v[dst_i * src_width + dst_j] =
				src_buffer[src_i * src_width + dst_j];

	/* horizontal scale */
	for (dst_i = 0; dst_i < dst_height; dst_i++) {
		h_weight = 0;
		for (dst_j = 0, src_j = 0;
		     (dst_j < dst_width) && (src_j < (src_width - 1));
		     dst_j++) {
			tmp_factor = ((h_weight & H_VALID_VALUE) >>
				H_SHIFT_BITS);
			dst_buffer_h[dst_i * dst_width + dst_j] =
				(dst_buffer_v[dst_i * src_width + src_j] *
					(H_MAX_VALUE - tmp_factor) +
				 dst_buffer_v[dst_i * src_width + src_j + 1] *
					tmp_factor + H_MAX_VALUE /
					SCALE_AVERAGE) /
				 H_MAX_VALUE;

			h_weight += h_scale;
			if (h_weight >= H_FACTOR) {
				h_weight -= H_FACTOR;
				src_j++;
			}
		}

		for (; dst_j < dst_width; dst_j++)
			dst_buffer_h[dst_i * dst_width + dst_j] =
			dst_buffer_v[dst_i * src_width + src_j];
	}
}

void pre_up_scale_c(
	int *src_buffer,
	int *dst_buffer_v,
	int *dst_buffer_h,
	unsigned int src_width,
	unsigned int src_height,
	unsigned int dst_width,
	unsigned int dst_height)
{
	unsigned int src_i = 0;
	unsigned int src_j = 0;

	unsigned int v_scale = 0;
	unsigned int h_scale = 0;
	unsigned int v_weight = 0;
	unsigned int h_weight = 0;

	unsigned int dst_i = 0;
	unsigned int dst_j = 0;

	unsigned int tmp_factor = 0;

	v_scale = (src_height * SCALE_AVERAGE * V_FACTOR) / (dst_height);
	h_scale = (src_width * SCALE_AVERAGE * H_FACTOR) /
		  (dst_width * SCALE_AVERAGE);

	/* vertical scale */
	for (dst_i = 0; dst_i < dst_height && src_i < src_height; dst_i++) {
		for (dst_j = 0; dst_j < src_width; dst_j++) {
			dst_buffer_v[dst_i * src_width + dst_j] =
				src_buffer[src_i * src_width + dst_j];
		}

		v_weight += v_scale;
		if (v_weight >= V_C_FACTOR) {
			v_weight -= V_C_FACTOR;
			src_i++;
		}
	}

	for (; dst_i < dst_height; dst_i++) {
		src_i = src_height - 1;
		for (dst_j = 0; dst_j < src_width; dst_j++)
			dst_buffer_v[dst_i * src_width + dst_j] =
			src_buffer[src_i * src_width + dst_j];
	}

	/* horizontal scale */
	for (dst_i = 0; dst_i < dst_height; dst_i++) {
		h_weight = 0;
		for (dst_j = 0, src_j = 0;
		     ((dst_j < dst_width) && (src_j < src_width - 1));
		     dst_j++) {
			tmp_factor = ((h_weight & H_VALID_VALUE) >>
				     H_SHIFT_BITS);
			dst_buffer_h[dst_i * dst_width + dst_j] =
				(dst_buffer_v[dst_i * src_width + src_j] *
					(H_MAX_VALUE - tmp_factor) +
				 dst_buffer_v[dst_i * src_width + src_j + 1] *
				 tmp_factor + H_MAX_VALUE / SCALE_AVERAGE) /
				 H_MAX_VALUE;

			h_weight += h_scale;
			if (h_weight >= H_FACTOR) {
				h_weight -= H_FACTOR;
				src_j++;
			}
		}

		for (; dst_j < dst_width; dst_j++)
			dst_buffer_h[dst_i * dst_width + dst_j] =
			dst_buffer_v[dst_i * src_width + src_j];
	}
}

void pre_down_scale(
	int *src_buffer,
	int *dst_buffer_v,
	int *dst_buffer_h,
	unsigned int src_width,
	unsigned int src_height,
	unsigned int dst_width,
	unsigned int dst_height)
{
	unsigned int src_i = 0;
	unsigned int src_j = 0;

	unsigned int v_scale = 0;
	unsigned int h_scale = 0;

	unsigned int dst_i = 0;
	unsigned int dst_j = 0;

	v_scale = (src_height + dst_height / SCALE_AVERAGE) / dst_height;
	h_scale = (src_width + dst_width / SCALE_AVERAGE) / dst_width;

	FG_LOG_D("down scale size %ux%u -> %ux%u\n",
		 src_width,
		 src_height,
		 dst_width,
		 dst_height);

	/* vertical scale */
	for (dst_i = 0;
	     (dst_i < dst_height) && (src_i < src_height - 1); dst_i++) {
		for (dst_j = 0; dst_j < src_width; dst_j++) {
			dst_buffer_v[dst_i * src_width + dst_j] =
				(src_buffer[src_i * src_width + dst_j] +
				 src_buffer[(src_i + 1) * src_width + dst_j]) /
				 SCALE_AVERAGE;
		}

		src_i += v_scale;
	}

	for (; dst_i < dst_height; dst_i++) {
		for (dst_j = 0; dst_j < src_width; dst_j++) {
			dst_buffer_v[dst_i * src_width + dst_j] =
				src_buffer[(src_height - 1) *
				src_width + dst_j];
		}
	}

	/* horizontal scale */
	for (dst_i = 0; dst_i < dst_height; dst_i++) {
		for (dst_j = 0, src_j = 0;
		     dst_j < dst_width && src_j < src_width-1; dst_j++) {
			dst_buffer_h[dst_i * dst_width + dst_j] =
				(dst_buffer_v[dst_i * src_width + src_j] +
				 dst_buffer_v[dst_i * src_width + src_j + 1]) /
				 SCALE_AVERAGE;

				src_j += h_scale;
		}

		for (; dst_j < dst_width; dst_j++) {
			dst_buffer_h[dst_i * dst_width + dst_j] =
				dst_buffer_v[dst_i*src_width + src_j];
		}
	}
}
