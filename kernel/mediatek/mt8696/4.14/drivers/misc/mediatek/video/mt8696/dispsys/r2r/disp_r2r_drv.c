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

#define LOG_TAG "R2R_DRV"

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/memory.h>
#include <linux/module.h>
#include "disp_hw_log.h"
#include "disp_hw_mgr.h"
#include "disp_info.h"
#include "disp_clk.h"
#include "disp_irq.h"
#include "disp_path.h"
#include "fmt_def.h"
#include "fmt_hal.h"
#include "disp_r2r_debug.h"
#include "disp_r2r_drv.h"
#include "disp_r2r_hal.h"
#include "disp_r2r_hw.h"
#include "disp_r2r_if.h"


#define R2R_FULL_SCREEN_IDX 2 /*0-default,  1-dovi*/
#define R2R_DTS_MAX_ND_SIZE 50
#define R2R_TEST_FRAME_MAX_SIZE (4096*2160*4)

/*for r2r main thread wake up */
static struct task_struct *disp_r2r_thread;
static wait_queue_head_t disp_r2r_wq;
static unsigned int gWakeupR2rSwThread;
struct r2r_layer_info *pt_r2r_layer;
struct disp_r2r_context _r2r_inst;
unsigned int r2r_dbg_level;
struct device *r2r_dev;
dma_addr_t r2r_yaddr_mva;
dma_addr_t r2r_caddr_mva;
dma_addr_t r2r_ccaddr_mva;
uint64_t *r2r_yaddr_va;
uint64_t *r2r_caddr_va;
uint64_t *r2r_ccaddr_va;


static LIST_HEAD(r2r_video_info_pool_main);
static DEFINE_MUTEX(r2r_video_info_pool_main_mutex);
static DEFINE_MUTEX(r2r_fence_buffer_mutex);


struct disp_r2r_timing r2r_timing_tbl[2][HDMI_VIDEO_RESOLUTION_NUM] = {
	{{0x35A, 0x20D, 0x0, 0x2D0, 0xF0, 0x1, 0x1, 0x7B, 0x13, 0x3E, 0x6,
		HDMI_VIDEO_720x480i_60Hz},
	{0x360, 0x271, 0x0, 0x2D0, 0x120, 0x1, 0x1, 0x85, 0x17, 0x40, 0x5,
		HDMI_VIDEO_720x576i_50Hz},
	{0x35A, 0x20D, 0x0, 0x2D0, 0x1E0, 0x1, 0x1, 0x7B, 0x25, 0x3E, 0x6,
		HDMI_VIDEO_720x480p_60Hz},
	{0x360, 0x271, 0x0, 0x2D0, 0x240, 0x1, 0x1, 0x85, 0x2D, 0x40, 0x5,
		HDMI_VIDEO_720x576p_50Hz},
	{0x672, 0x2EE, 0x0, 0x500, 0x2D0, 0x1, 0x1, 0x105, 0x1A, 0x28, 0x5,
		HDMI_VIDEO_1280x720p_60Hz},
	{0x7BC, 0x2EE, 0x0, 0x500, 0x2D0, 0x1, 0x1, 0x105, 0x1A, 0x28, 0x5,
		HDMI_VIDEO_1280x720p_50Hz},
	{0x898, 0x465, 0x0, 0x780, 0x21C, 0x1, 0x1, 0xC1, 0x15, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080i_60Hz},
	{0xA50, 0x465, 0x0, 0x780, 0x21C, 0x1, 0x1, 0xC1, 0x15, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080i_50Hz},
	{0x898, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xC1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_30Hz},
	{0xA50, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xC1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_25Hz},
	{0xABE, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xC1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_24Hz},
	{0xABE, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xC1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_23Hz},
	{0x898, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xC1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_29Hz},
	{0x898, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xC1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_60Hz},
	{0xA50, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xC1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_50Hz},
	{0x157C, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_23_976HZ},
	{0x157C, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_24HZ},
	{0x14A0, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_25HZ},
	{0x1130, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_29_97HZ},
	{0x1130, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_30HZ},
	{0x157C, 0x8CA, 0x0, 0x1000, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_4096x2160P_24HZ},
	{0x1130, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_60HZ},
	{0x14A0, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_50HZ},
	{0x1130, 0x8CA, 0x0, 0x1000, 0x870, 0x1, 0x1, 0xD9, 0x53, 0x58, 0xA,
		HDMI_VIDEO_4096x2160P_60HZ},
	{0x14A0, 0x8CA, 0x0, 0x1000, 0x870, 0x1, 0x1, 0xD9, 0x53, 0x58, 0xA,
		HDMI_VIDEO_4096x2160P_50HZ},
	{0x672, 0x2EE, 0x0, 0x500, 0x2D0, 0x1, 0x1, 0x105, 0x1A, 0x28, 0x5,
		HDMI_VIDEO_1280x720p_59_94Hz},
	{0x898, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xC1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_59_94Hz},
	{0x1130, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x181, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_59_94HZ},
	{0x1130, 0x8CA, 0x0, 0x1000, 0x870, 0x1, 0x1, 0xD9, 0x53, 0x58, 0xA,
		HDMI_VIDEO_4096x2160P_59_94HZ} },
	{{0x35A, 0x20D, 0x0, 0x2D0, 0xF0, 0x1, 0x1, 0x9B, 0x13, 0x3E, 0x6,
		HDMI_VIDEO_720x480i_60Hz},
	{0x360, 0x271, 0x0, 0x2D0, 0x120, 0x1, 0x1, 0xA5, 0x17, 0x40, 0x5,
		HDMI_VIDEO_720x576i_50Hz},
	{0x35A, 0x20D, 0x0, 0x2D0, 0x1E0, 0x1, 0x1, 0x9B, 0x25, 0x3E, 0x6,
		HDMI_VIDEO_720x480p_60Hz},
	{0x360, 0x271, 0x0, 0x2D0, 0x240, 0x1, 0x1, 0xA5, 0x2D, 0x40, 0x5,
		HDMI_VIDEO_720x576p_50Hz},
	{0x672, 0x2EE, 0x0, 0x500, 0x2D0, 0x1, 0x1, 0x125, 0x1A, 0x28, 0x5,
		HDMI_VIDEO_1280x720p_60Hz},
	{0x7BC, 0x2EE, 0x0, 0x500, 0x2D0, 0x1, 0x1, 0x125, 0x1A, 0x28, 0x5,
		HDMI_VIDEO_1280x720p_50Hz},
	{0x898, 0x465, 0x0, 0x780, 0x21C, 0x1, 0x1, 0xE1, 0x15, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080i_60Hz},
	{0xA50, 0x465, 0x0, 0x780, 0x21C, 0x1, 0x1, 0xE1, 0x15, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080i_50Hz},
	{0x898, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xE1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_30Hz},
	{0xA50, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xE1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_25Hz},
	{0xABE, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xE1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_24Hz},
	{0xABE, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xE1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_23Hz},
	{0x898, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xE1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_29Hz},
	{0x898, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xE1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_60Hz},
	{0xA50, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xE1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_50Hz},
	{0x157C, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_23_976HZ},
	{0x157C, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_24HZ},
	{0x14A0, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_25HZ},
	{0x1130, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_29_97HZ},
	{0x1130, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_30HZ},
	{0x157C, 0x8CA, 0x0, 0x1000, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_4096x2160P_24HZ},
	{0x1130, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_60HZ},
	{0x14A0, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_50HZ},
	{0x1130, 0x8CA, 0x0, 0x1000, 0x870, 0x1, 0x1, 0xF9, 0x53, 0x58, 0xA,
		HDMI_VIDEO_4096x2160P_60HZ},
	{0x14A0, 0x8CA, 0x0, 0x1000, 0x870, 0x1, 0x1, 0xF9, 0x53, 0x58, 0xA,
		HDMI_VIDEO_4096x2160P_50HZ},
	{0x672, 0x2EE, 0x0, 0x500, 0x2D0, 0x1, 0x1, 0x125, 0x1A, 0x28, 0x5,
		HDMI_VIDEO_1280x720p_59_94Hz},
	{0x898, 0x465, 0x0, 0x780, 0x438, 0x1, 0x1, 0xE1, 0x2A, 0x2C, 0x5,
		HDMI_VIDEO_1920x1080p_59_94Hz},
	{0x1130, 0x8CA, 0x0, 0xF00, 0x870, 0x1, 0x1, 0x1A1, 0x53, 0x58, 0xA,
		HDMI_VIDEO_3840x2160P_59_94HZ},
	{0x1130, 0x8CA, 0x0, 0x1000, 0x870, 0x1, 0x1, 0xF9, 0x53, 0x58, 0xA,
		HDMI_VIDEO_4096x2160P_59_94HZ} }
};

struct r2r_video_buffer_info *
disp_r2r_init_buf_info(struct r2r_video_buffer_info *buf)
{
	memset(buf, 0, sizeof(struct r2r_video_buffer_info));
	INIT_LIST_HEAD(&(buf->list));

	buf->release_fence_fd = R2R_INVALID_FENCE_FD;
	buf->ion_hndy = NULL;
	buf->ion_hndc = NULL;
	buf->ion_hndcc = NULL;
	buf->layer_id = 0xFF;
	buf->r2r_yaddr = 0;
	buf->r2r_caddr = 0;
	buf->r2r_ccaddr = 0;
	buf->src_duration = 0;
	return buf;
}

struct disp_r2r_context *r2r_get_inst(void)
{
	return &_r2r_inst;
}

int r2r_get_list_count(struct list_head *head)
{
	int count = 0;
	struct list_head *pos, *n;

	if (head == NULL || list_empty(head))
		return count;

	list_for_each_safe(pos, n, head) { count++; }
	return count;
}

struct r2r_video_buffer_info *r2r_alloc_buf_info(void)
{
	static int allocated_count;
	int value[4] = {0};

	mutex_lock(&r2r_video_info_pool_main_mutex);
	value[0] = r2r_get_list_count(&r2r_video_info_pool_main);
	mutex_unlock(&r2r_video_info_pool_main_mutex);

	mutex_lock(&(pt_r2r_layer->sync_lock));
	value[2] = r2r_get_list_count(&(pt_r2r_layer->buf_list));
	mutex_unlock(&(pt_r2r_layer->sync_lock));

	r2r_default("create new buf node, main_pool:%d, sub_pool:%d\n",
		value[0], value[1]);
	r2r_default("main_working_pool:%d, sub_working_pool:%d total:%d\n",
		   value[2], value[3], allocated_count++);

	return kzalloc(sizeof(struct r2r_video_buffer_info), GFP_KERNEL);
}


struct r2r_video_buffer_info *r2r_get_buf_info(void)
{
	struct r2r_video_buffer_info *info = NULL;

	mutex_lock(&r2r_video_info_pool_main_mutex);
	if (!list_empty(&r2r_video_info_pool_main)) {
		info = list_first_entry(&r2r_video_info_pool_main,
					struct r2r_video_buffer_info, list);
		list_del_init(&info->list);
		mutex_unlock(&r2r_video_info_pool_main_mutex);
		disp_r2r_init_buf_info(info);
	} else {
		mutex_unlock(&r2r_video_info_pool_main_mutex);
		info = r2r_alloc_buf_info();
		if (info != NULL)
			disp_r2r_init_buf_info(info);
		else
			r2r_error("r2r alloc buf fail\n");
	}
	return info;
}

void r2r_release_buf_info(struct list_head *list)
{
	mutex_lock(&r2r_video_info_pool_main_mutex);
	list_add_tail(list, &r2r_video_info_pool_main);
	mutex_unlock(&r2r_video_info_pool_main_mutex);
}

void disp_r2r_wakeup_routine(void)
{
	gWakeupR2rSwThread = 1;
	wake_up(&disp_r2r_wq);
}

//for test
#if 1

#include <linux/hrtimer.h>
#include <linux/ktime.h>

ktime_t ktime;
struct hrtimer hr_timer;

enum hrtimer_restart disp_r2r_timer_callback(struct hrtimer *hr_timer)
{
	disp_r2r_wakeup_routine();
	return HRTIMER_NORESTART;
}

int disp_r2r_set_timer(void)
{
	int interval = 2000; /* us */
	int status = 0;

	do {
		hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ktime = ktime_set(interval / 1000000,
				  (interval % 1000000) * 1000);
		hr_timer.function = &disp_r2r_timer_callback;
		hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
	} while (0);

	return status;
}
#endif

static int disp_r2r_routine(void *data)
{
	int ret = 0;
	//struct r2r_video_buffer_info *buf = NULL;
	//struct r2r_video_buffer_info *curbuf = NULL;
	//struct r2r_video_buffer_info *nextbuf = NULL;
	//struct r2r_layer_info *layer_info;
	static int r2r_vsync_count;

	while (1) {
		ret = wait_event_interruptible(disp_r2r_wq,
			gWakeupR2rSwThread);
		if (ret)
			r2r_error("wait r2r routine err");
		gWakeupR2rSwThread = 0;
		r2r_vsync_count++;
		r2r_default("r2r thread loop[%d]\n", r2r_vsync_count);
	}
	return ret;
}

void disp_r2r_drv_init(uintptr_t reg_base)
{
	pt_r2r_layer = get_r2r_layer_info();
	memset((void *)&_r2r_inst, 0, sizeof(struct disp_r2r_context));
	init_waitqueue_head(&disp_r2r_wq);
	gWakeupR2rSwThread = 0;

	disp_r2r_thread = kthread_create(disp_r2r_routine,
					  (void *)(&(pt_r2r_layer->layer_id)),
					  "disp_r2r");

	wake_up_process(disp_r2r_thread);

	_r2r_inst.r2r_hw_base = reg_base;
	_r2r_inst.reg_conf_mode = R2R_REG_CONF_CLIENT_RIU;
	_r2r_inst.io_reg_base = DISP_R2R_REG_BASE;

	r2r_hal_init(reg_base);
	_r2r_inst.inited = true;
}

void disp_r2r_drv_uninit(void)
{
	r2r_hal_uninit();
	memset((void *)&_r2r_inst, 0, sizeof(struct disp_r2r_context));
}

int disp_r2r_drv_set_gce_handle(void *pv_handle)
{
	if (!_r2r_inst.inited)
		return R2R_RET_UNINIT;
	if (pv_handle == NULL)
		return R2R_RET_INV_ARG;
	_r2r_inst.gce_handle = (struct cmdqRecStruct *)pv_handle;
	return R2R_RET_OK;
}

int disp_r2r_set_clk_enable(uint32_t enable)
{
	if (enable) {
		disp_clock_enable(DISP_CLK_R2R, true);
		disp_clock_smi_larb_en(DISP_SMI_LARB5, true);
		disp_clock_smi_larb_en(DISP_SMI_LARB6, true);
	} else {
		disp_clock_enable(DISP_CLK_R2R, false);
		disp_clock_smi_larb_en(DISP_SMI_LARB5, false);
		disp_clock_smi_larb_en(DISP_SMI_LARB6, true);
	}
	return R2R_RET_OK;
}

int disp_r2r_drv_set_conf_mode(uint32_t mode)
{
	_r2r_inst.reg_conf_mode = mode;
	r2r_default("%s mode[%d]!\n", __func__, mode);
	return R2R_RET_OK;
}

int disp_r2r_map_timing(enum HDMI_VIDEO_RESOLUTION res,
	struct disp_r2r_timing *pst_timing)
{
	int i;
	struct disp_r2r_timing *tg_point = NULL;
	struct disp_r2r_timing *tg_table = &(r2r_timing_tbl[0][0]);

	if (pst_timing == NULL)
		return R2R_RET_INV_ARG;

	if (res >= HDMI_VIDEO_RESOLUTION_NUM) {
		r2r_error("disp_r2r_lk_init not support this res[%d]!\n", res);
		return -1;
	}

	for (i = 0; i < HDMI_VIDEO_RESOLUTION_NUM; i++) {
		if (tg_table[i].res == res) {
			tg_point = &tg_table[i];
			break;
		}
	}
	if (tg_point != NULL) {
		pst_timing->htotal = tg_point->htotal;
		pst_timing->vtotal_lsb = tg_point->vtotal_lsb;
		pst_timing->vtotal_msb = tg_point->vtotal_msb;
		pst_timing->width = tg_point->width;
		pst_timing->height = tg_point->height;
		pst_timing->hoffset = tg_point->hoffset;
		pst_timing->voffset = tg_point->voffset;
		pst_timing->hfront = tg_point->hfront;
		pst_timing->vfront = tg_point->vfront;
		pst_timing->hsync_w = tg_point->hsync_w;
		pst_timing->vsync_w = tg_point->vsync_w;
		pst_timing->res = tg_point->res;
	} else
		return R2R_RET_INV_ARG;

	return R2R_RET_OK;
}

void disp_r2r_drv_set_resolution(uint32_t hw_id,
	enum HDMI_VIDEO_RESOLUTION res)
{
	struct disp_r2r_timing st_timing;

	disp_r2r_map_timing(res, &st_timing);
	r2r_hal_set_timing(&st_timing);
	_r2r_inst.res = res;
}

void disp_r2r_drv_get_resolution(uint32_t hw_id,
	enum HDMI_VIDEO_RESOLUTION *pe_res)
{
	if (pe_res != NULL) {
		// get res
		*pe_res = _r2r_inst.res;
	}
}

void disp_r2r_drv_enable(bool fgEn, uint32_t mode)
{
	r2r_hal_set_enable(fgEn, mode);
	r2r_hal_set_shdw(true, true);
}

void disp_r2r_drv_clr_irq(uint32_t irq)
{
	r2r_hal_clr_isr(true, 0);
}

void disp_r2r_drv_flush_reg(uint32_t mode)
{
	r2r_hal_isr(mode);
}

void disp_r2r_drv_config_frame(struct r2r_video_buffer_info *buf_info)
{
	uint32_t src_bit = 8;
	uint32_t src_w = 0;
	uint32_t src_pitch = 0;
	bool is_422 = false;

	if (buf_info != NULL) {
		src_w = buf_info->src.width;
		if (buf_info->bit_depth == DEPTH_10_BIT)
			src_bit = 10;
		else if (buf_info->bit_depth == DEPTH_12_BIT)
			src_bit = 12;
		else
			src_bit = 8;
		src_pitch = src_w/(128/src_bit);

		if (src_w%(128/src_bit) != 0)
			src_pitch = src_pitch + 1;

		if (buf_info->src_fmt == DISP_R2R_COLOR_FORMAT_YUV422)
			is_422 = true;

		r2r_hal_set_format(is_422, src_bit, 0);
		r2r_hal_set_pitch(src_pitch, 0);

		if (is_422)
			r2r_hal_conf_frame(buf_info->r2r_yaddr,
				buf_info->r2r_caddr, 0,
				_r2r_inst.reg_conf_mode);
		else
			r2r_hal_conf_frame(buf_info->r2r_yaddr,
				buf_info->r2r_caddr,
				buf_info->r2r_ccaddr,
				_r2r_inst.reg_conf_mode);
	}

}

void disp_r2r_drv_path_sel(bool fg_on)
{
	disp_path_set_hw_path(DISP_PATH_DISP_R2R, fg_on);
}

void disp_r2r_drv_set_sof(void)
{
	fmt_hal_set_sof(FMT_SOF_26_R2R_STA,
		FMT_SOF_26_R2R_END, 0x00010002, 0x00010003);
}

void disp_r2r_drv_set_front(uint32_t front_h, uint32_t front_v)
{

}

int disp_r2r_dbg_level_enable(uint32_t level, uint32_t enable)
{
	if (enable)
		r2r_dbg_level |= (1 << level);
	else
		r2r_dbg_level &= !(1 << level);
	r2r_default("set r2r dbg level %d enable %d 0x%X\n", level, enable,
		   r2r_dbg_level);
	return R2R_RET_OK;
}

void disp_r2r_drv_set_pattern(uint32_t res_id, uint32_t enable,
	uint32_t y_data, uint32_t cb_data, uint32_t cr_data)
{
	struct disp_r2r_timing st_timing;
	struct disp_r2r_pattern stPattern;

	disp_r2r_map_timing((enum HDMI_VIDEO_RESOLUTION)res_id, &st_timing);
	stPattern.ypattern = y_data;
	stPattern.cbpattern = cb_data;
	stPattern.crpattern = cr_data;
	stPattern.mode = 0;
	r2r_hal_set_pattern(enable, (void *)&stPattern, (void *)&st_timing);

}

void disp_r2r_drv_read_reg(uint32_t rel_offset, uint32_t size)
{
	uint32_t idx = 0;
	uintptr_t va_addr = 0;

	if (rel_offset < 0x1000) {
		for (idx = 0; idx < size; idx++) {
			va_addr =
				_r2r_inst.r2r_hw_base + rel_offset + idx * 4;
			r2r_default("0x%p = 0x%x\n",
				(void *)va_addr, ReadREG32(va_addr));
		}
	}
}

void disp_r2r_drv_write_reg(uint32_t rel_offset, uint32_t value)
{
	uintptr_t va_addr = _r2r_inst.r2r_hw_base;

	if (rel_offset < 0x1000) {
		va_addr += rel_offset;
		WriteREG32(va_addr, value);
		r2r_default("0x%p = 0x%x\n",
			(void *)va_addr, ReadREG32(va_addr));
	}
}

int disp_r2r_drv_gce_trigger(void)
{
	int ret = R2R_RET_OK;

	if (!_r2r_inst.inited || (_r2r_inst.gce_handle == NULL))
		return R2R_RET_UNINIT;
	r2r_hal_isr(R2R_REG_CONF_CLIENT_GCE);
	return ret;
}

static int disp_r2r_drv_alloc_mem(void)
{
	dma_addr_t mva_address = 0;

	r2r_dev = disp_hw_mgr_get_dev();
	if (r2r_dev == NULL)
		return R2R_RET_ERROR;

	r2r_yaddr_va =
	(uint64_t *)dma_alloc_coherent(r2r_dev,
	R2R_TEST_FRAME_MAX_SIZE, &mva_address, GFP_KERNEL);
	r2r_yaddr_mva = mva_address;

	r2r_caddr_va =
	(uint64_t *)dma_alloc_coherent(r2r_dev,
	R2R_TEST_FRAME_MAX_SIZE, &mva_address, GFP_KERNEL);
	r2r_caddr_mva = mva_address;

	r2r_ccaddr_va =
	(uint64_t *)dma_alloc_coherent(r2r_dev,
	R2R_TEST_FRAME_MAX_SIZE, &mva_address, GFP_KERNEL);
	r2r_ccaddr_mva = mva_address;

	r2r_default("all mem va(0x%p,0x%p,0x%p) mva(0x%x,0x%x,0x%x)\n",
		r2r_yaddr_va, r2r_caddr_va, r2r_ccaddr_va,
		r2r_yaddr_mva, r2r_caddr_mva, r2r_ccaddr_mva);

	return R2R_RET_OK;
}

static int disp_r2r_drv_free_mem(void)
{
	r2r_dev = disp_hw_mgr_get_dev();
	if (r2r_dev == NULL)
		return R2R_RET_ERROR;

	dma_free_coherent(r2r_dev, R2R_TEST_FRAME_MAX_SIZE,
		r2r_yaddr_va, r2r_yaddr_mva);
	dma_free_coherent(r2r_dev, R2R_TEST_FRAME_MAX_SIZE,
		r2r_caddr_va, r2r_caddr_mva);
	dma_free_coherent(r2r_dev, R2R_TEST_FRAME_MAX_SIZE,
		r2r_ccaddr_va, r2r_ccaddr_mva);
	return R2R_RET_OK;
}

int disp_r2r_drv_set_test(uint32_t test_no)
{
	struct r2r_video_buffer_info *test_buf = NULL;
	struct disp_r2r_timing st_timing;

	if (test_no == 100) {
		disp_r2r_drv_free_mem();
		return R2R_RET_OK;
	}

	test_buf = kmalloc(
		sizeof(struct r2r_video_buffer_info), GFP_KERNEL);
	if (test_buf == NULL)
		return R2R_RET_ERROR;

	memset(test_buf, 0x0, sizeof(struct r2r_video_buffer_info));
	disp_r2r_drv_alloc_mem();
	test_buf->r2r_yaddr = r2r_yaddr_mva;
	test_buf->r2r_caddr = r2r_caddr_mva;
	test_buf->r2r_ccaddr = r2r_ccaddr_mva;
	test_buf->layer_id = 0;
	disp_r2r_map_timing(_r2r_inst.res, &st_timing);
	test_buf->src.width = st_timing.width;
	test_buf->src.height = st_timing.height;

	switch (test_no) {
	case 1:
		test_buf->bit_depth = DEPTH_8_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV444;
		test_buf->hdr_type = DISP_DR_TYPE_SDR;
		break;
	case 2:
		test_buf->bit_depth = DEPTH_10_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV444;
		test_buf->hdr_type = DISP_DR_TYPE_SDR;
		break;
	case 3:
		test_buf->bit_depth = DEPTH_12_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV444;
		test_buf->hdr_type = DISP_DR_TYPE_SDR;
		break;
	case 4:
		test_buf->bit_depth = DEPTH_8_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV422;
		test_buf->hdr_type = DISP_DR_TYPE_SDR;
		break;
	case 5:
		test_buf->bit_depth = DEPTH_10_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV422;
		test_buf->hdr_type = DISP_DR_TYPE_SDR;
		break;
	case 6:
		test_buf->bit_depth = DEPTH_12_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV422;
		test_buf->hdr_type = DISP_DR_TYPE_SDR;
		break;
	case 7:
		test_buf->bit_depth = DEPTH_10_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV444;
		test_buf->hdr_type = DISP_DR_TYPE_HDR10;
		break;
	case 8:
		test_buf->bit_depth = DEPTH_12_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV444;
		test_buf->hdr_type = DISP_DR_TYPE_HDR10;
		break;
	case 9:
		test_buf->bit_depth = DEPTH_12_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV422;
		test_buf->hdr_type = DISP_DR_TYPE_HDR10;
		break;
	case 10:
		test_buf->bit_depth = DEPTH_10_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_RGB888;
		test_buf->hdr_type = DISP_DR_TYPE_DOVI;
		break;
	case 11:
		test_buf->bit_depth = DEPTH_12_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_RGB888;
		test_buf->hdr_type = DISP_DR_TYPE_DOVI;
		break;
	case 12:
		test_buf->bit_depth = DEPTH_10_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV444;
		test_buf->hdr_type = DISP_DR_TYPE_DOVI;
		break;
	case 13:
		test_buf->bit_depth = DEPTH_12_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV444;
		test_buf->hdr_type = DISP_DR_TYPE_DOVI;
		break;
	case 14:
		test_buf->bit_depth = DEPTH_12_BIT;
		test_buf->src_fmt = DISP_R2R_COLOR_FORMAT_YUV422;
		test_buf->hdr_type = DISP_DR_TYPE_DOVI;
		break;
	default:
		break;
	}
	disp_r2r_drv_config_frame(test_buf);
	r2r_default("config frame r2r 0x%x done\n", r2r_yaddr_mva);
	if (test_buf != NULL) {
		kfree(test_buf);
		test_buf = NULL;
	}
	return R2R_RET_OK;
}

