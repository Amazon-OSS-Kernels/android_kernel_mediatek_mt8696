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

#define LOG_TAG "VDP"

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
#include <uapi/linux/time.h>
#include <uapi/linux/sched/types.h>
#include <linux/time64.h>
#include <linux/jiffies.h>

#include "disp_hw_log.h"
#include "disp_hw_mgr.h"
#include "disp_info.h"
#include "disp_vdp_if.h"
#include "vdp_hal.h"
#include "vdp_hw.h"
#include "mtk_sync.h"
#include "disp_clk.h"
#include "disp_irq.h"
#include "disp_path.h"
#include "disp_vdp_sec.h"
#include "disp_cfd_main.h"
#include "disp_dovi_common_if.h"
#include "disp_hdr_if.h"
#include "disp_fefifo_if.h"
/*#include "vdp_fence.h"*/
/*#include "vdp_ion.h"*/
/*for kzalloc */
/*#include <linux/vmalloc.h>*/
//#include "ion_drv.h"
#include <linux/slab.h>

/*#include "mtk_sync.h"*/

#include "disp_vdp_debug.h"
#include "disp_vdp_vsync.h"
#include "fmt_def.h"
#include "fmt_hal.h"
/*#include "disp_drv_platform.h"*/
#include "disp_vdp_cli.h"

#include "disp_reg.h"
#include "internal_hdmi_drv.h"
#define FENCE_TIMEOUT 1000
#define INVALID_PTS 0xffffffff
#define HDR_MAX_LUMINANCE_LIMIT 10000

#define IS_HD_RES(res) (res == HDMI_VIDEO_1280x720p_60Hz || \
			res == HDMI_VIDEO_1280x720p_50Hz)
/*
 ** we don't have active start signal.
 ** but VSYNC IRQ is a little earlier before Active Start.
 ** So we delay 2ms in VSYNC IRQ to simulate Active Start.
 */
#define USE_FAKE_ACTIVE_START                                                  \
	0 /* set timer in VSYNC IRQ to simulate HW Active start */

unsigned int vdp_dbg_level = (0
			      /* | DDP_FUNC_LOG */
			      /* | DDP_FLOW_LOG */
			      /* | DDP_COLOR_FORMAT_LOG */
			      /* | DDP_FB_FLOW_LOG */
			      /* | DDP_RESOLUTION_LOG */
			      /* | DDP_OVL_FB_LOG */
			      /*| VDP_FENCE_LOG*/
			      /* | DDP_TVE_FENCE_LOG */
			      /* | DDP_FENCE1_LOG */
			      /* | DDP_FENCE2_LOG */
				/*| VDP_AVSYNC_LOG*/
			      );

struct list_head vdp_buffer_lis;
static DEFINE_MUTEX(_disp_fence_mutex);

static struct workqueue_struct *release_buffer_wq[2];

/* whether vdp init is OK.
 ** not allow to handle irq if disp_vdp_init() not run.
 */
static bool vdp_init_done;
static atomic_t vdp_suspend;
static bool hdmi_resolution_changed;
enum HDMI_VIDEO_RESOLUTION current_resolution;

struct task_struct *vdp_set_input_buffer_worker_task;
wait_queue_head_t vdp_set_input_buffer_irq_wq;
atomic_t vdp_set_input_buffer_irq_event = ATOMIC_INIT(0);
struct task_struct *vdp_get_input_buffer_worker_task;
struct video_layer_info vdp_video_layer[VIDEO_LAYER_MAX_COUNT];
struct video_layer_info *video_layer = vdp_video_layer;
struct disp_hw_common_info disp_common_info;
enum DISP_DR_TYPE_T force_dr_range;
uint32_t force_decode_allm;
uint32_t vdp_disp_test;

uint32_t idk_vdo_en;
uint32_t idk_gfx_en;
uint32_t idk_vdo_pts[2];
uint8_t idk_vdo_start[2];
uint32_t vdp_not_mix = 3, vdp_not_display = 3;
#ifdef DISP_QMS_SUPPORT
uint32_t set_src_fps;
bool set_qms_en;
uint32_t vdp_qms_fps[2];
#endif

struct mtk_disp_vdp_cap vdp_scale_info[VIDEO_LAYER_MAX_COUNT];

bool is_hd_resolution(void)
{
	return IS_HD_RES(current_resolution);
}

static uint64_t get_current_time_ns(void)
{
	struct timespec64 t;

	ktime_get_ts64(&t);

	return (t.tv_sec & 0xFFF) * 1000000000 + t.tv_nsec;
}

#if 0
int vdp_clear_incoming_buffer(disp_video_buffer_info *buffer_info)
{
	int i, ret = 0;
	struct sync_fence *fence;

	for (i = 0; i < MAX_VIDEO_INPUT_CONFIG; i++) {
		disp_video_layer_config *layer = &buffer_info->layers[i];

		if (layer->fence_fd < 0)
			continue; /* Nothing to wait on */

		if (layer->fence_fd == 0) {
			DDPERR(" invalid fence fd %d\n", layer->fence_fd);
			continue; /* Nothing to wait on */
		}
		fence = sync_fence_fdget(layer->fence_fd);
		if (!fence) {
			/* This is bad, but we just log an error and continue */
			DDPERR(" failed to import sync fence\n");
			fence = NULL;
			continue;
		}

		sync_fence_put(fence);
	}
	return ret;
}
#endif

static int _vdp_parse_dev_node(void)
{
	struct device_node *np;
	unsigned int reg_value;
	unsigned int irq_value;
	struct disp_hw *vdp_drv = disp_vdp_get_drv();

	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-vdo");
	if (np == NULL) {
		DISP_LOG_E("dts error, no vdo device node.\n");
		return VDP_DTS_ERROR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);
	of_property_read_u32_index(np, "interrupts", 1, &irq_value);

	disp_vdo_reg_base[0] = (char *)of_iomap(np, 0); /*vdo3, 15001000 */
	disp_vdo_reg_base[1] = (char *)of_iomap(np, 1); /*vdo4, 15008000 */

	vdp_drv->irq[0].value = irq_of_parse_and_map(np, 0);

	if (vdp_drv->irq[0].value == 0)
		DISP_LOG_E("VDP get irq from dts fail\n");
	else {
		vdp_drv->irq[0].irq = DISP_IRQ_DISP3_VSYNC;
		vdp_drv->irq_num = 1;
	}

	if (vdp_drv->irq[0].value == 0)
		return VDP_DTS_ERROR;

	return VDP_OK;
}

#if 0
static int _h_down_cap(uint32_t layer_id, uint32_t src_w,
uint32_t out_w, uint32_t out_h)
{
	int factor = 0;
	int in_h_limit = 0;

	DISP_LOG_D("layer_id: %d, src_w: %d, out_w: %d, out_h: %d\n",
		   layer_id, src_w, out_w, out_h);

	factor = (src_w * 0x1000) / out_w;

	if (out_h == 2160)
		in_h_limit = 4096;
	else if (out_h == 1080)
		in_h_limit = 1920;
	else if (out_h == 720)
		in_h_limit = 1280;
	else if (out_h < 720)
		in_h_limit = 720;
	else
		in_h_limit = 0;

	if ((out_w > src_w) || (out_w == 0) || (src_w == 0) || (factor > 0xFFFF)
	    || (out_w & 1) || (src_w > in_h_limit)
		) {
		DISP_LOG_D("Not able to downscale\n");
		return 0;
	}

	DISP_LOG_D("Be able to downscale\n");
	return 1;
}
#endif

bool force_dsd_off;
static int _vdp_dsd_cap(struct mtk_disp_vdp_cap *vdp_cap, uint32_t out_w,
			uint32_t out_h)
{
	if (force_dsd_off)
		return 0;

	if (((vdp_cap->src.width > out_w) || (vdp_cap->src.height > out_h))) {
		if (out_h < vdp_cap->tgt.height * 2)
			return 1;
		else
			return 0;
	} else
		return 0;
}

#if (VIDEO_USE_HW_SHADOW && VIDEO_REDUCE_BUFFER)
void disp_vdp_release_buffer(struct work_struct *work)
{
	struct video_layer_info *layer_info = NULL;
	struct timespec64 ts;
	unsigned long timestap = 0;

	layer_info = container_of(work, struct video_layer_info, task_work);
	if (layer_info == NULL) {
		DISP_LOG_E("get VDP layer info NULL\n");
		return;
	}

	mutex_lock(&(layer_info->sync_lock));
	do {
		struct video_buffer_info *buf = NULL;
		struct video_buffer_info *temp = NULL;

		list_for_each_entry_safe(buf, temp, &(layer_info->buf_list),
					 list) {
			if (layer_info->release_timeline_idx >
			    buf->current_fence_index) {
				/* signal the fence */
				if (layer_info->timeline != NULL) {
					timeline_inc(layer_info->timeline, 1);
					 buf->time_fence_signal =
						sched_clock();
					if (print_video_fence_history) {
						ktime_get_raw_ts64(&ts);
						timestap =
					(unsigned long)ts.tv_sec * 1000 +
					ts.tv_nsec / 1000000;
						DISP_LOG_N(
						"fence%d time %lld, wait 0x%llx, pts %lld, timer %ld\n",
					buf->current_fence_index,
					(buf->time_fence_signal -
					buf->time_fence_create),
					(buf->time_start_display -
					buf->time_fence_create),
					buf->pts, timestap);
					}
				}

				layer_info->release_idx++;
				vdp_printf(VDP_AVSYNC_LOG,
					   "release buf %llu %d rel %d %d\n",
					   buf->pts,
					   buf->current_fence_index,
					   layer_info->release_timeline_idx,
					   layer_info->release_idx);
				/*free ion handle */
				disp_hw_mgr_dma_buffer_release(&buf->mmu1);

				/* add buffer list to free buffer pool */
				list_del_init(&buf->list);
				release_buf_info(&buf->list,
						 layer_info->layer_id);
				break;
			}
		}

	} while (0);
	mutex_unlock(&(layer_info->sync_lock));
}
#endif

bool enable_frame_drop_log;

int disp_vdp_init(struct disp_hw_common_info *info)
{
	int i = 0;
	char name[16] = {0};
	int ret = 0;

	DISP_LOG_D("%s start\n", __func__);
	/*parser dts file for register base & irq */
	_vdp_parse_dev_node();
	vdp_cli_init();
	/*Init variables */
	for (i = 0; i < VIDEO_LAYER_MAX_COUNT; i++) {
		memset((void *)(&video_layer[i]), 0,
		       sizeof(struct video_layer_info));
		video_layer[i].layer_id = i;
		ret = sprintf(name, "vdp_timeline%d", i);
		if (ret <= 0)
			DISP_LOG_E("%s sprintf fail%d\n", __func__, ret);

		video_layer[i].timeline =
			(struct sync_timeline *)timeline_create(name);

		INIT_LIST_HEAD(&video_layer[i].buf_list);
		mutex_init(&(video_layer[i].sync_lock));
		mutex_init(&(video_layer[i].play_state_lock));
		video_layer[i].inited = 1;
		video_layer[i].timeline_idx = 0;
		video_layer[i].release_timeline_idx = 0;
		video_layer[i].release_idx = 0;
		video_layer[i].last_pts = 0xffffffff;
		video_layer[i].fence_idx = 0;
		video_layer[i].display_duration = 0;
		video_layer[i].vsync_duration = 0;
		video_layer[i].enable = false;
		video_layer[i].layer_start = false;
		video_layer[i].dsd_en = false;
		video_layer[i].secure_en = false;
		video_layer[i].secure2normal = false;
		video_layer[i].state = VDP_LAYER_IDLE;
		video_layer[i].hdr10_type = HDR10_TYPE_NONE;
		video_layer[i].videobuf = NULL;

#ifdef DISP_GCE_SUPPORT
		video_layer[i].new_trigger = false;
#endif

#if (VIDEO_USE_HW_SHADOW && VIDEO_REDUCE_BUFFER)
		INIT_WORK(&video_layer[i].task_work, disp_vdp_release_buffer);
#endif
		init_waitqueue_head(&video_layer[i].wait_queue);
	}

	memset((void *)&disp_common_info, 0,
	       sizeof(struct disp_hw_common_info));

	/* create work queue for release buffer */
	release_buffer_wq[0] =
		create_singlethread_workqueue("vdp0_release_buffer");
	release_buffer_wq[1] =
		create_singlethread_workqueue("vdp1_release_buffer");

	/*Create semaphore */

	/*Create thread */

	vdp_debug_init();

	vdp_vsync_init();

	/*vdp_hal_init();*/

	/*vdp_kthread_init();*/

	force_dsd_off = true;
	vdp_init_done = true;
	enable_frame_drop_log = false;

	atomic_set(&vdp_suspend, 0);

	DISP_LOG_D("%s end\n", __func__);
	return VDP_OK;
}

int disp_vdp_deinit(void)
{
#ifdef build_fail
	ion_client_destroy(vdp_ion_client);
#endif
	return VDP_OK;
}

bool disp_vdp_check_layer_id(const int layer_id, const int line_num)
{
	if (layer_id >= VIDEO_LAYER_MAX_COUNT) {
		DISP_LOG_E("video layer index[%d] invalid, line[%d]\n",
			   layer_id, line_num);
		return false;
	}
	return true;
}

int disp_vdp_get_vsync_duration(const struct disp_hw_resolution *res)
{
	int vsync_duration = TIME_BASE / res->frequency;

	if (res->is_fractional) {
		if (res->frequency == 60)
			vsync_duration =
				TIME_BASE * 100 / (res->frequency * 100 - 6);
		else if (res->frequency == 30)
			vsync_duration =
				TIME_BASE * 100 / (res->frequency * 100 - 3);
		else if (res->frequency == 24)
			vsync_duration =
				TIME_BASE * 1000 / (res->frequency * 1000 - 24);
	}

	return vsync_duration;
}

uint32_t disp_vdp_get_source_duration(uint32_t fps,
				      const struct disp_hw_resolution *info)
{
	uint32_t source_duration = TIME_BASE * 100 / 5994; /* 59.94 fps */
	if (vdo_set_fps > 0)
		fps = vdo_set_fps;

	if (fps != 0) {
		if (fps == 2397)
			source_duration = TIME_BASE * 1000 / 23976;
		else
			source_duration = TIME_BASE * 100 / fps;
	}

	/* adjust 60fps, must not adjust 59.94 fps resolution. will cause av not
	 * sync
	 */
	if (info->frequency == 60 && !info->is_fractional) {
		/* 60fps */
		switch (fps) {
		case 2397:
		case 2400:
			/*output: 2 3 2 3 2 3 ... */
			source_duration = TIME_BASE * 100 / 2400;
			break;
		case 2997:
		case 3000:
			/* output: 2 2 2 2 ... */
			source_duration = TIME_BASE * 100 / 3000;
			break;
		case 5994:
		case 6000:
			/* output: 1 1 1 1 ... */
			source_duration = TIME_BASE * 100 / 6000;
			break;
		case 2500:
		case 5000:
		default:
			break;
		}
	}

	return source_duration;
}

bool main_fmt_first;
bool sub_fmt_first;

/*Open the power & clock */
int disp_vdp_start(struct disp_hw_common_info *info, unsigned int layer_id)
{
	enum FMT_TV_TYPE tv_type;
	struct video_layer_info *layer_info = NULL;
	int ret = 0;
	int sof_start = 0, sof_end = 0;
	struct disp_hw *vdp_drv = disp_vdp_get_drv();

	DISP_LOG_I("%s %d start\n", __func__, layer_id);
	/*vdo init */
	if (!disp_vdp_check_layer_id(layer_id, __LINE__))
		return VDP_INVALID_INDEX;

	if (((vdp_not_display == 0) || (vdp_not_display == 2)) &&
		(layer_id == 0)) {
		DISP_LOG_I("vdp0 drop all the video buffer for debug\n");
		return VDP_FAIL;
	}

	if (((vdp_not_display == 1) || (vdp_not_display == 2)) &&
		(layer_id == 1)) {
		DISP_LOG_I("vdp1 drop all the video buffer for debug\n");
		return VDP_FAIL;
	}

	layer_info = &video_layer[layer_id];

	mutex_lock(&(layer_info->play_state_lock));

	DISP_LOG_I("layer_start %d\n", layer_info->layer_start);

	/* VDP is already started bypass. */
	if (layer_info->layer_start) {
		DISP_LOG_I("%s already start\n", __func__);

		mutex_unlock(&(layer_info->play_state_lock));
		return VDP_ALREADY_START;
	}
	layer_info->layer_start = true;
	/* if we stop, then start in next few vsync, maybe stop flow has not
	 *finished yet.
	 ** we need to wait last stop flow finished, then run start flow.
	 */
	ret = wait_event_timeout(layer_info->wait_queue,
				 layer_info->state == VDP_LAYER_IDLE,
				 msecs_to_jiffies(1000));

	if (ret == 0) {
		DISP_LOG_E("wait layer state idle timeout\n");

		mutex_unlock(&(layer_info->play_state_lock));
		return 0;
	}

	vdp_hal_init(layer_id);
	memcpy((void *)&disp_common_info, (const void *)info,
	       sizeof(struct disp_hw_common_info));
	if (info->resolution->frequency == 25 ||
	    info->resolution->frequency == 50)
		tv_type = FMT_TV_TYPE_PAL;
	else
		tv_type = FMT_TV_TYPE_NTSC;

	DISP_LOG_I("%s %d, res %d %d dispsys_shadow %d\n",
		   __func__,
		   layer_id,
		   info->resolution->res_mode,
		   current_resolution,
		   layer_info->dispsys_shadow_enable);
	/*enable disp_fmt */
	vdp_drv->drv_call(DISP_CMD_VDP_START, &layer_id);

	if (layer_id == 0) {
		fmt_hal_clock_on_off(DISP_FMT_MAIN, true);
		fmt_hal_set_mode(DISP_FMT_MAIN, info->resolution->res_mode,
				 true);
		disp_path_set_hw_path(DISP_PATH_M_VDO, true);
		/*disp_path_set_hw_path(DISP_PATH_M_VDO_OUT, true);*/
		disp_path_get_sof_info(DISP_SOF_0_M_VDO_MAIN_STA,
			DISP_SOF_0_M_VDO_MAIN_END, &sof_start, &sof_end);
		fmt_hal_set_sof(FMT_SOF_0_M_VDO_MAIN_STA,
			FMT_SOF_0_M_VDO_MAIN_END, sof_start, sof_end);
		fmt_hal_set_tv_type(DISP_FMT_MAIN, tv_type);
		//fmt_hal_hw_shadow_enable(DISP_FMT_MAIN); to be confirm
		disp_clock_smi_larb_en(DISP_SMI_LARB5, true);
		disp_clock_smi_larb_en(DISP_SMI_LARB6, true);
		disp_clock_enable(DISP_CLK_VDO3, true);
		disp_sys_hal_video_ultra_en(DISP_MAIN, true);

		video_layer[0].vsync_duration =
			disp_vdp_get_vsync_duration(info->resolution);
		video_layer[0].display_duration = 0;
		main_fmt_first = true;
	} else if (layer_id == 1) {
		fmt_hal_clock_on_off(DISP_FMT_SUB, true);
		fmt_hal_set_mode(DISP_FMT_SUB, info->resolution->res_mode,
				 true);
		disp_path_set_hw_path(DISP_PATH_S_VDO, true);
		/*disp_path_set_hw_path(DISP_PATH_S_VDO_OUT, true);*/
		disp_path_get_sof_info(DISP_SOF_7_S_VDO_MAIN_STA,
			DISP_SOF_7_S_VDO_MAIN_END, &sof_start, &sof_end);
		fmt_hal_set_sof(FMT_SOF_7_S_VDO_MAIN_STA,
			FMT_SOF_7_S_VDO_MAIN_END, sof_start, sof_end);
		fmt_hal_set_tv_type(DISP_FMT_SUB, tv_type);
		//fmt_hal_hw_shadow_enable(DISP_FMT_SUB);
		disp_clock_smi_larb_en(DISP_SMI_LARB5, true);
		disp_clock_smi_larb_en(DISP_SMI_LARB6, true);
		disp_clock_enable(DISP_CLK_VDO4, true);
		disp_sys_hal_video_preultra_en(DISP_SUB, true);

		video_layer[1].vsync_duration =
			disp_vdp_get_vsync_duration(info->resolution);
		video_layer[1].display_duration = 0;
		sub_fmt_first = true;

	} else {
		DISP_LOG_E("Invalid video id %d\n", layer_id);

		mutex_unlock(&(layer_info->play_state_lock));
		return VDP_INVALID_INDEX;
	}

	if(is_hd_resolution() &&
	   !video_layer[layer_id].dispsys_shadow_enable) {
		video_layer[layer_id].dispsys_shadow_enable = true;
		disp_hw_mgr_dispsys_shadow_enable(true);
	}

	// start fefifo
	disp_fefifo_start(info, layer_id);

	video_layer[layer_id].state = VDP_LAYER_START;
	video_layer[layer_id].dsd_en = false;
	video_layer[layer_id].type = DISP_LAYER_UNKNOWN;

	#ifdef DISP_QMS_SUPPORT
	video_layer[layer_id].last_fps = 0;
	video_layer[layer_id].qms_en = 0;
	video_layer[layer_id].qms_exit = 0;
	vdp_qms_fps[layer_id] = 0;
	#endif

	vdp_disable_active_zone(layer_info);
	DISP_LOG_I("%s layer_start %d end\n",
		   __func__, layer_info->layer_start);

	mutex_unlock(&(layer_info->play_state_lock));

	/*dispfmt setting? */
	return VDP_OK;
}

/*Close the power & clock*/
int disp_vdp_stop(unsigned int layer_id)
{
	struct video_layer_info *layer_info = NULL;

	//idk2.6 use vdp layer
	if (idk_vdo_en)
		return VDP_OK;
	/* struct vdp_hal_config_info config_info = {0}; */
	if (!disp_vdp_check_layer_id(layer_id, __LINE__))
		return VDP_INVALID_INDEX;

	DISP_LOG_I("%s %d\n", __func__, layer_id);

	layer_info = &video_layer[layer_id];

	mutex_lock(&(layer_info->play_state_lock));

	DISP_LOG_I("layer_start %d res %d\n",
		  layer_info->layer_start,
		  current_resolution);

	/* VDP is already stopped bypass. */
	if (!video_layer[layer_id].layer_start) {
		DISP_LOG_I("already stop %d %d\n",
			   layer_info->layer_start,
			   video_layer[layer_id].layer_start);

		mutex_unlock(&(layer_info->play_state_lock));
		return VDP_ALREADY_STOP;
	}

	#ifdef DISP_QMS_SUPPORT
	if (video_layer[layer_id].qms_en ||
		(disp_common_info.vrr_info->vrr_mode == 1)) {
		video_layer[layer_id].qms_exit = true;
		video_layer[layer_id].last_fps = 0;
		vdp_qms_fps[layer_id] = 0;
		DISP_LOG_I("start exit qms\n");
	}
	#endif
	video_layer[layer_id].layer_start = false;
	video_layer[layer_id].type = DISP_LAYER_UNKNOWN;
	video_layer[layer_id].state = VDP_LAYER_STOPPING;
	video_layer[layer_id].last_pts = INVALID_PTS;
	video_layer[layer_id].allm_en = false;

	vdp_stop_disable_hw(layer_id);
	set_fs_index(layer_id, 0);

	DISP_LOG_I("%s layer_start %d end\n",
		   __func__, layer_info->layer_start);

	if (is_hd_resolution() ||
	    layer_info->dispsys_shadow_enable) {
		layer_info->dispsys_shadow_enable = false;
		disp_hw_mgr_dispsys_shadow_enable(false);
	}

	mutex_unlock(&(layer_info->play_state_lock));

	return VDP_OK;
}

int disp_vdp_suspend(void)
{
	uint32_t layer_enable =
		video_layer[0].layer_start | (video_layer[1].layer_start << 1);
	int ret = 0;
	int i = 0;

	DISP_LOG_I("%s layer_enable %u\n", __func__, layer_enable);

	for (i = 0; i < 2; i++) {
		if (layer_enable & BIT(i))
			disp_vdp_stop(i);
	}

	for (i = 0; i < 2; i++) {
		if (layer_enable & BIT(i)) {
			ret = wait_event_timeout(video_layer[i].wait_queue,
						 video_layer[i].state ==
							 VDP_LAYER_IDLE,
						 msecs_to_jiffies(1000));

			if (ret == 0) {
				DISP_LOG_E(
					"wait layer[%d] state idle timeout\n",
					i);
				return -1;
			}
		}
	}

	atomic_set(&vdp_suspend, 1);

	DISP_LOG_I("%s done\n", __func__);

	return VDP_OK;
}

int disp_vdp_resume(void)
{
	DISP_LOG_I("%s\n", __func__);

	atomic_set(&vdp_suspend, 0);

	return VDP_OK;
}

bool disp_vdp_check_dsd_condition(enum HDMI_VIDEO_RESOLUTION res_mode,
				  uint32_t src_width, uint32_t src_height)
{
	if ((res_mode == HDMI_VIDEO_1920x1080p_60Hz) ||
	    (res_mode == HDMI_VIDEO_1920x1080p_50Hz) ||
	    (res_mode == HDMI_VIDEO_1280x720p_60Hz) ||
	    (res_mode == HDMI_VIDEO_1280x720p_50Hz) ||
	    (res_mode == HDMI_VIDEO_720x480p_60Hz) ||
	    (res_mode == HDMI_VIDEO_720x576p_50Hz)) {
		/*1080p to 720p can't use dsd solution */
		if ((res_mode == HDMI_VIDEO_1280x720p_60Hz) ||
		    (res_mode == HDMI_VIDEO_1280x720p_50Hz)) {
			if ((src_width <= 1920) && (src_height <= 1080))
				return false;
			else
				return true;
		} else
			return true;
	} else
		return false;
}

int disp_vdp_get_info(struct disp_hw_common_info *info)
{
	bool use_crop = false;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t dst_width = 0;
	uint32_t dst_height = 0;
	struct mtk_disp_vdp_cap *vdp_cap;

	vdp_cap = &(info->vdp_cap);

	vdp_cap->need_resizer = false; /*set default value */
	if (((vdp_cap->src.width != vdp_cap->crop.width) ||
	     (vdp_cap->src.height != vdp_cap->crop.height)) &&
	    (vdp_cap->crop.width != 0) && (vdp_cap->crop.height != 0)) {
		use_crop = true;
		width = vdp_cap->crop.width;
		height = vdp_cap->crop.height;
	} else {
		use_crop = false;
		width = vdp_cap->src.width;
		height = vdp_cap->src.height;
	}
	dst_width = vdp_cap->tgt.width;
	dst_height = vdp_cap->tgt.height;

	vdp_printf(
		VDP_RESOLUTION_LOG,
		"src(%d %d %d %d) crop(%d %d %d %d) tgt(%d %d %d %d) res(%d %d)\n",
		vdp_cap->src.x, vdp_cap->src.y, vdp_cap->src.width,
		vdp_cap->src.height, vdp_cap->crop.x, vdp_cap->crop.y,
		vdp_cap->crop.width, vdp_cap->crop.height, vdp_cap->tgt.x,
		vdp_cap->tgt.y, vdp_cap->tgt.width, vdp_cap->tgt.height,
		info->resolution->width, info->resolution->height);

	do {
		/* for vdp dump buffer debug:
		 ** force use MDP to dump MDP buffer
		 ** the MDP output buffer is VDP input buffer
		 */
		if ((vdp_cli_get()->enable_force_use_mdp) &&
		    (width > dst_width)) {
			DISP_LOG_I("force use MDP for debug\n");
			vdp_cap->need_resizer = true;
			break;
		}

		/*for dovi idk2.6 PIP and sbs case*/
		/*pip case sub video use mdp*/
		if (idk_vdo_en == 2 &&
			((vdp_cap->tgt.width < info->resolution->width) ||
			(vdp_cap->tgt.height < info->resolution->height))) {
			vdp_cap->need_resizer = true;
			break;
		}

		/*pip case main and sub video use mdp*/
		if (idk_vdo_en == 3) {
			vdp_cap->need_resizer = true;
			break;
		}

		/* buffer & HDMI resolution are not the same.
		 ** in this case, can't use H Down Scale
		 ** only can use DSD or imgresz.
		 */
		if ((width > info->resolution->width) ||
		    (height > info->resolution->height)) {
			if (!_vdp_dsd_cap(vdp_cap, info->resolution->width,
					  info->resolution->height))
				vdp_cap->need_resizer =
					true; /* DSD not available */
			else {
				if (disp_vdp_check_dsd_condition(
					    info->resolution->res_mode, width,
					    height))
					vdp_cap->need_resizer =
						false; /* DSD available */
				else
					vdp_cap->need_resizer = true;
			}

			break;
		}

		/* buffer & HDMI resolution are the same.
		 ** in this case, we can use H Down scale
		 */
		if ((width > dst_width * 8) || (height > dst_height * 2) ||
		    ((width > 1920) && (height == dst_height * 2)))
			vdp_cap->need_resizer = true;

		/* in 4k output resoltuion, if video source is less than
		 * 1080p, use image resizer instead of vdp h down scale
		 */
		if ((info->resolution->width > 1920) &&
		    (width <= 1920) &&
		    (width > dst_width))
			vdp_cap->need_resizer = true;

		if ((vdp_cap->layer_id == 3) && ((height < dst_height * 4)))
			vdp_cap->need_resizer = false;
	} while (0);

	if (vdp_cap->layer_id <= VDP_2)
		memcpy((void *)&vdp_scale_info[vdp_cap->layer_id],
		       (void *)vdp_cap,
		       sizeof(struct mtk_disp_vdp_cap));

	vdp_printf(
		VDP_RESOLUTION_LOG,
		"layer_id=%d, use_resizer=%d\n",
		vdp_cap->layer_id,
		 vdp_cap->need_resizer);

	return VDP_OK;
}

int disp_vdp_change_resolution(const struct disp_hw_resolution *info)
{
	int h_start = 0, v_start_odd = 0, v_start_even = 0;
	enum FMT_TV_TYPE tv_type;
	int sof_start = 0, sof_end = 0;
	int i = 0;

	/* dovi res change is moved to hdr.c */

	if (current_resolution == info->res_mode)
		return VDP_OK;

	DISP_LOG_I("vdp resolution change %d -> %d\n",
		   current_resolution, info->res_mode);

	current_resolution = info->res_mode;

	hdmi_resolution_changed = true;

	if (info->frequency == 25 || info->frequency == 50)
		tv_type = FMT_TV_TYPE_PAL;
	else
		tv_type = FMT_TV_TYPE_NTSC;

	disp_path_get_active_zone(0, info->res_mode, &h_start, &v_start_odd,
				  &v_start_even);

	if (video_layer[0].layer_start) {
		/* set HTotal & VTotal pixel for spec resolution. */
		fmt_hal_set_mode(DISP_FMT_MAIN, info->res_mode, true);
		fmt_hal_set_tv_type(DISP_FMT_MAIN, tv_type);
		fmt_hal_enable(DISP_FMT_MAIN, true);
		if (dovi_path_en != 1) {
			disp_path_get_sof_info(DISP_SOF_0_M_VDO_MAIN_STA,
			DISP_SOF_0_M_VDO_MAIN_END, &sof_start, &sof_end);
			fmt_hal_set_sof(FMT_SOF_0_M_VDO_MAIN_STA,
			FMT_SOF_0_M_VDO_MAIN_END, sof_start, sof_end);
		}
		#ifndef DISP_GCE_SUPPORT
		fmt_hal_hw_shadow_enable(DISP_FMT_MAIN);
		#endif
		video_layer[0].vsync_duration =
			disp_vdp_get_vsync_duration(info);
	}

	if (video_layer[1].layer_start) {
		/* set HTotal & VTotal pixel for spec resolution. */
		fmt_hal_set_mode(DISP_FMT_SUB, info->res_mode, true);
		fmt_hal_set_tv_type(DISP_FMT_SUB, tv_type);
		fmt_hal_enable(DISP_FMT_SUB, true);
		if (dovi_path_en != 1) {
			disp_path_get_sof_info(DISP_SOF_7_S_VDO_MAIN_STA,
			DISP_SOF_7_S_VDO_MAIN_END, &sof_start, &sof_end);
			fmt_hal_set_sof(FMT_SOF_7_S_VDO_MAIN_STA,
			FMT_SOF_7_S_VDO_MAIN_END, sof_start, sof_end);
		}
		#ifndef DISP_GCE_SUPPORT
		fmt_hal_hw_shadow_enable(DISP_FMT_SUB);
		#endif
		video_layer[1].vsync_duration =
			disp_vdp_get_vsync_duration(info);
	}

	vdp_hal_config_timing(0, info, h_start, v_start_odd, v_start_even);

	for (i = 0; i < 2; i++) {
		mutex_lock(&(video_layer[i].play_state_lock));

		DISP_LOG_I("[VDP%d] layer_start %d dispsys_shadow %d\n",
			   i,
			   video_layer[i].layer_start,
			   video_layer[i].dispsys_shadow_enable);

		if (video_layer[i].layer_start &&
		    is_hd_resolution() &&
		    !video_layer[i].dispsys_shadow_enable) {
			video_layer[i].dispsys_shadow_enable = true;
			disp_hw_mgr_dispsys_shadow_enable(true);
		} else if (!is_hd_resolution() &&
			   video_layer[i].dispsys_shadow_enable) {
			video_layer[i].dispsys_shadow_enable = false;
			disp_hw_mgr_dispsys_shadow_enable(false);
		}

		mutex_unlock(&(video_layer[i].play_state_lock));
	}

	return VDP_OK;
}

int disp_vdp_get_mva(int ion_fd, struct video_buffer_info *buf_info)
{
	bool is_Y_C_independent = false;
	int ret = 0;

	if (ion_fd == -1) {
		vdp_printf(VDP_FLOW_LOG, "invalid ion fd %d\n", ion_fd);
		return -1;
	}

	if ((ion_fd >> 16) > 0)
		is_Y_C_independent = true;

	/*get ion handle */
	if (is_Y_C_independent) {
		ret = disp_hw_mgr_dma_fd_to_buffer(ion_fd & 0xFFFF,
						   &buf_info->mmu1);
		if (ret < 0) {
			DISP_LOG_E("invalid y dma fd %d\n", ion_fd);
			return ret;
		}

		ret = disp_hw_mgr_dma_fd_to_buffer((ion_fd & 0xFFFF0000) >> 16,
						   &buf_info->mmu2);
		if (ret < 0) {
			DISP_LOG_E("invalid c dma fd %d\n", ion_fd);
			return ret;
		}
	} else {
		ret = disp_hw_mgr_dma_fd_to_buffer(ion_fd & 0xFFFF,
						   &buf_info->mmu1);
		if (ret < 0) {
			DISP_LOG_E("invalid single y dma fd %d\n", ion_fd);
			return ret;
		}
	}

	/*get physical address */
	buf_info->src_phy_addr = buf_info->mmu1.buffer_addr.dma_addr;

	if (is_Y_C_independent)
		buf_info->src_phy_addr2 = buf_info->mmu2.buffer_addr.dma_addr;

	return ret;
}

static void disp_vdp_copy_film_grain_md(struct mtk_disp_buffer *config,
					struct video_buffer_info *buf_info)
{
	int ret = 0;
	struct disp_hw *vdp_drv = disp_vdp_get_drv();

	buf_info->is_film_grain = config->is_film_grain;
	if (!config->is_film_grain)
		return;

	vdp_drv->drv_call(DISP_CMD_FG_START, &buf_info->layer_id);

	vdp_printf(VDP_FG_LOG,
		   "disp_fg svp %d offset %d len %d %d\n",
		   config->film_grain_info.svp,
		   config->film_grain_info.offset,
		   config->film_grain_info.len,
		   sizeof(struct mtk_av1_film_grain_params));

	buf_info->film_grain_info.pts = config->pts;
	buf_info->film_grain_info.len = config->film_grain_info.len;
	buf_info->film_grain_info.svp = config->film_grain_info.svp;
	if (!config->secruity_en) { /* normal display buffer */
		if (config->film_grain_info.len >= DOVI_MD_MAX_LEN) {
			DISP_LOG_E(
				"film grain metadata size too long %d\n",
				config->film_grain_info.len);
		}

		ret = copy_from_user(buf_info->film_grain_info.buff,
				    (void __user *)(config->film_grain_info.addr +
						    config->film_grain_info.offset),
				    config->film_grain_info.len);
		if (ret) {
			DISP_LOG_E("copy film grain metadata fail\n");
			buf_info->film_grain_info.len = 0;
		}
	} else { /* secure display buffer */
		buf_info->film_grain_info.sec_handle =
			buf_info->mmu1.buffer_addr.sec_handle;
		buf_info->film_grain_info.sec_handle_offset =
			config->film_grain_info.offset;
		vdp_printf(VDP_FG_LOG,
			   "config handle %d %d %d pts %lld\n",
			   buf_info->film_grain_info.sec_handle,
			   buf_info->film_grain_info.sec_handle_offset,
			   buf_info->film_grain_info.len,
			   buf_info->film_grain_info.pts);
	}
}

int config_frame_count[2];  /* debug vdp valid config */
int config_buffer_count[2]; /* debug vdp total buffer(contain invalid buffer)*/

int disp_vdp_config(struct mtk_disp_buffer *config,
		    struct disp_hw_common_info *info)
{
	/*put the new buffer into the buffer list */
	int ret = VDP_OK;
#ifdef DISP_QMS_SUPPORT
	uint8_t tfr = 0;
	uint32_t brr_freq = 0;
	uint32_t new_fps = 0;
#endif
	struct video_buffer_info *buf_info = NULL;
	struct fence_data fence;
	bool is_Y_C_independent = false;
	uint64_t config_start, config_end;
	uint64_t time;

	config_start = get_current_time_ns();

	if (!disp_vdp_check_layer_id(config->layer_id, __LINE__))
		return VDP_INVALID_INDEX;

	/*add for dovi idk2.6 pip case*/
	if (idk_vdo_en == 2) {
		if ((config->tgt.width < 1920 && config->tgt.height < 1080))
			config->layer_id = 1;
		else
			config->layer_id = 0;
		vdp_cli_get()->target_area.enable = false;
	}

	if (idk_vdo_en == 3) {
		if (config->tgt.x > 0)
			config->layer_id = 1;
		else
			config->layer_id = 0;
		vdp_cli_get()->target_area.enable = false;
	}

	config_buffer_count[config->layer_id]++;

	if ((config->ion_fd >> 16) > 0)
		is_Y_C_independent = true;

	if ((video_layer[config->layer_id].state != VDP_LAYER_START) &&
	    (video_layer[config->layer_id].state != VDP_LAYER_RUNNING)) {
		if (config->ion_fd != VDP_INVALID_ION_FD) {
			DISP_LOG_W(
				"not start yet, drop frame: id[%d] state[%d] fd %d\n",
				config->layer_id,
				video_layer[config->layer_id].state,
				config->ion_fd);
		}
		return VDP_NOT_START;
	}

	/* pip hardware not support 4k frame, drop it */
	if ((config->layer_id == VDP_2) &&
	    (((config->crop.width > 1920) &&
	      (config->crop.width >= config->tgt.width)) ||
	     ((config->crop.pitch > 1920) || (config->src.pitch > 1920)))) {
		DISP_LOG_W(
		"[VDP%d] drop 4k [%d %d %d %d %d] [%d %d %d %d %d] [%d %d %d %d %d]\n",
		config->layer_id,
		config->src.x,
		config->src.y,
		config->src.width,
		config->src.height,
		config->src.pitch,
		config->crop.x,
		config->crop.y,
		config->crop.width,
		config->crop.height,
		config->crop.pitch,
		config->tgt.x,
		config->tgt.y,
		config->tgt.width,
		config->tgt.height,
		config->tgt.pitch);

		return VDP_FAIL;
	}

	if (config->tgt.width > info->resolution->width ||
	    config->tgt.height > info->resolution->height) {
		DISP_LOG_E("the tgt size is error (%d %d) res (%d %d)\n",
			   config->tgt.width, config->tgt.height,
			   info->resolution->width, info->resolution->height);
		return VDP_FAIL;
	}

	/* check buffer PTS and src/target window */
	if (config->pts == video_layer[config->layer_id].last_pts &&
	    !hdmi_resolution_changed &&
	    (config->ion_fd == video_layer[config->layer_id].ion_fd)) {
		if (!memcmp(&video_layer[config->layer_id].src_rgn,
			    &config->src, sizeof(struct mtk_disp_range)) &&
		    !memcmp(&video_layer[config->layer_id].tgt_rgn,
			    &config->tgt, sizeof(struct mtk_disp_range))) {
			DISP_LOG_D(
				"Same video buffer,skip,layerID[%d] pts[%lld]\n",
				config->layer_id, config->pts);
			return VDP_OK;
		}
	}
	if (config->pts < video_layer[config->layer_id].last_pts &&
	    INVALID_PTS != video_layer[config->layer_id].last_pts &&
	    !info->tunnel_playback_and_user_is_hwc)
		DISP_LOG_E("%s pts=%lld,last_pts=%lld",
		__func__, config->pts,
		video_layer[config->layer_id].last_pts);
	hdmi_resolution_changed = false;

	/* get video_buffer_info to store information in mtk_disp_buffer */
	buf_info = vdp_get_buf_info(config->layer_id);
	if (buf_info == NULL) {
		DISP_LOG_E("get layer[%d] input buffer fail\n",
			   config->layer_id);
		return VDP_FAIL;
	}

	/* convert ion_fd to MVA/secure_handle, store in buf_info->src_phy_addr
	 ** for normal buffer: the convert result is MVA.
	 ** for secure buffer: the convert result is secure handle.
	 */
	ret = disp_vdp_get_mva(config->ion_fd, buf_info);
	if (ret < 0) {
		if ((video_layer[config->layer_id].type == config->type) &&
		    (video_layer[config->layer_id].type != DISP_LAYER_UNKNOWN))
			DISP_LOG_D(
				"buffer_layer_id: %d get mva fail ionfd:0x%x waitFence[0x%x]\n",
				config->layer_id, config->ion_fd,
				config->acquire_fence_fd);
		/* release acquired buffer info */
		/* buf_info is not add to any list yet, no need to lock. */
		list_del_init(&buf_info->list);
		release_buf_info(&buf_info->list, config->layer_id);
		return ret;
	}
	video_layer[config->layer_id].ion_fd = config->ion_fd;
	/* add debug decode allm flag for test */
	if (force_decode_allm > 0)
		config->allm_en = true;

	if (config->allm_en != video_layer[config->layer_id].allm_en)
		video_layer[config->layer_id].allm_en = config->allm_en;
	video_layer[config->layer_id].type = config->type;
	#ifdef DISP_GCE_SUPPORT
	video_layer[config->layer_id].buffer_sec = config->secruity_en;
	#endif

	/* calculate buffer display time */
	buf_info->source_duration =
		disp_vdp_get_source_duration(config->fps, info->resolution);

	/* playback window change with video in paused state - refresh display
	 * asap
	 */
	if (config->fps != 0) {
		if (memcmp(&video_layer[config->layer_id].tgt_rgn, &config->tgt,
			   sizeof(struct mtk_disp_range)) ||
		    (video_layer[config->layer_id].last_pts == config->pts))
			buf_info->source_duration = 0;
	}

	/*for frame drop check */
	if (config->fps != 0) {
		if ((config->pts - video_layer[config->layer_id].last_pts) >
		    buf_info->source_duration + 300) { /*300 is deviation */
			if (enable_frame_drop_log)
				DISP_LOG_E(
					"drop frame, last[%lld], current[%lld], fps[%d]\n",
					video_layer[config->layer_id].last_pts,
					config->pts, config->fps);
			else
				DISP_LOG_D(
					"drop frame, last[%lld], current[%lld], fps[%d]\n",
					video_layer[config->layer_id].last_pts,
					config->pts, config->fps);
		}
	}

#ifdef DISP_QMS_SUPPORT
	if (set_qms_en)
		config->qms_en = set_qms_en;

	if (set_src_fps)
		config->fps = set_src_fps;

	buf_info->qms_en = config->qms_en;
	if ((disp_common_info.vrr_info->vrr_mode == 1) &&
		(disp_common_info.tv.u1_sink_qms_en ||
		disp_common_info.tv.u1_sink_cinemavrr))
		buf_info->qms_en = 1;

	buf_info->fps = config->fps;

	if (buf_info->qms_en) {
		switch (config->fps) {
		case 2300:
			new_fps = 23000;
			break;
		case 2397:
			new_fps = 23976;
			break;
		case 2400:
			new_fps = 24000;
			break;
		case 2500:
			new_fps = 25000;
			break;
		case 2900:
			new_fps = 29000;
			break;
		case 2997:
			new_fps = 29970;
			break;
		case 3000:
			new_fps = 30000;
			break;
		case 4795:
			new_fps = 47952;
			break;
		case 4800:
			new_fps = 48000;
			break;
		case 5000:
			new_fps = 50000;
			break;
		case 5994:
			new_fps = 59940;
			break;
		case 6000:
			new_fps = 60000;
			break;
		default:
			new_fps = 0;
			break;
		}

		//calc brr real frequency,x1000
		brr_freq = disp_common_info.resolution->frequency * VRR_PREC;
		if (disp_common_info.resolution->is_fractional)
			brr_freq = brr_freq * VRR_PREC / (VRR_PREC + 1);

		if (config->stream_fps != 0)
			buf_info->fps = config->stream_fps;
		else if (config->fps != 0)
			buf_info->fps = new_fps;
		else
			buf_info->fps = brr_freq;
	}

	/*remove frc for QMS,keep source dura as brr frequency*/
	if (buf_info->qms_en) {
		/*if 60fps source,59.94 output, source_duration keep 60 for drop frame
		 * fps should change to brr to keep qms run
		 */
		if (buf_info->fps == 60000) {
			buf_info->source_duration = TIME_BASE * 100 / 6000;
			buf_info->fps = brr_freq;
		} else
			buf_info->source_duration = TIME_BASE * 1000 / brr_freq;
	}

	if (buf_info->qms_en && (vdp_qms_fps[config->layer_id] != buf_info->fps)) {
		ret = disp_qms_condition_check(buf_info->fps,
			config->is_dovi,
			config->is_hdr10plus);
		if (ret == 0) {
			tfr = disp_tfr_mapping(buf_info->fps);
			vQMSSetting(true, tfr, 0);
			video_layer[config->layer_id].qms_en = true;
			DISP_LOG_I("vdp config qms fps %d %lld %d\n",
				buf_info->fps, fmt_active_irq_cnt,
				fmt_hal_read_vline());
		} else {
			video_layer[config->layer_id].qms_en = false;
			if (disp_common_info.vrr_info->vrr_en == 1)
				disp_queue_vrr_mode_disable();

			DISP_LOG_I("vdp config qms not enable\n");
		}
		vdp_qms_fps[config->layer_id] = buf_info->fps;
	}
#endif

	buf_info->source_duration *= vdp_cli_get()->slow;

	memcpy((void *)(&(buf_info->crop)), (const void *)(&(config->crop)),
	       sizeof(struct mtk_disp_range));
	memcpy((void *)(&(buf_info->src)), (const void *)(&(config->crop)),
	       sizeof(struct mtk_disp_range));
	if (vdp_cli_get()->target_area.enable &&
	    vdp_cli_get()->target_area.layer_id == config->layer_id) {
		/* for debug use.
		 ** adjust buffer display (x_offset, yoffset) and width & height
		 ** use command: cli vdp.show 0 0 1920 1080 1920
		 ** offset(0, 0) width 1920 height 1080 pitch 1920
		 */
		memcpy((void *)(&(buf_info->tgt)),
		       (const void *)(&(vdp_cli_get()->target_area.range)),
		       sizeof(struct mtk_disp_range));
	} else {
		buf_info->tgt.x = config->tgt.x;
		buf_info->tgt.y = config->tgt.y;
		buf_info->tgt.width = config->tgt.width;
		buf_info->tgt.height = config->tgt.height;
		buf_info->tgt.pitch = config->tgt.pitch;
	}
#if 0
		memcpy((char *)(&(buf_info->tgt)),
		       (const char *)(&(config->tgt)),
		       sizeof(struct mtk_disp_range));
#endif

	if (buf_info->src.width == 0) {
		buf_info->src.width = 1920;
		buf_info->src.height = 1080;
		buf_info->src.pitch = 1920;
	}
	if (buf_info->crop.width == 0) {
		buf_info->crop.width = buf_info->src.width;
		buf_info->crop.height = buf_info->src.height;
	}

	if (buf_info->crop.width > 4096 || buf_info->crop.height > 2176) {
		DISP_LOG_E("error crop info (%d %d)\n", buf_info->crop.width,
			   buf_info->crop.height);
		goto release_ion_handle;
	}

	/* for dolby unitTest */
	if (vdp_disp_test) {
		if (vdp_disp_test == 1) {
			buf_info->tgt.x = 0;
			buf_info->tgt.y = 0;
			buf_info->tgt.width =
				disp_common_info.resolution->width;
			buf_info->tgt.height =
				disp_common_info.resolution->height;
		}

		if (vdp_disp_test > 1) {
			buf_info->src.x = 0;
			buf_info->src.y = 0;
			buf_info->src.width =
				disp_common_info.resolution->width;
			buf_info->src.height =
				disp_common_info.resolution->height;

			buf_info->crop.x = 0;
			buf_info->crop.y = 0;
			buf_info->crop.width =
				disp_common_info.resolution->width;
			buf_info->crop.height =
				disp_common_info.resolution->height;
		}
	}

	buf_info->layer_id = config->layer_id;
	buf_info->buf_state = BUFFER_CREATE;
	buf_info->src_fmt = config->src_fmt;
	buf_info->ion_fd = config->ion_fd;
	buf_info->is_10bit = config->is_10bit;
	buf_info->is_10bit_lbs2bit_tile_mode =
		config->is_10bit_lbs2bit_tile_mode;
	buf_info->is_bt2020 = config->is_bt2020;
	buf_info->is_dovi = config->is_dovi;
	buf_info->is_pack_mode = config->is_pack_mode;
	buf_info->is_interlace = !config->is_progressive;
	buf_info->is_seamless = config->is_seamless;
	buf_info->is_ufo = config->is_ufo;
	buf_info->acquire_fence_fd = config->acquire_fence_fd;
	if (buf_info->acquire_fence_fd != -1)
		buf_info->sync_fence =
			sync_file_get_fence(buf_info->acquire_fence_fd);
	buf_info->src_fmt = config->src_fmt;
	buf_info->video_type = config->video_type;
	buf_info->ofst_y = config->ofst_y;
	buf_info->ofst_c = config->ofst_c;
	buf_info->ofst_y_len = config->ofst_y_len;
	buf_info->ofst_c_len = config->ofst_c_len;
	buf_info->buffer_size = config->buffer_size;
	buf_info->secruity_en = config->secruity_en;
	buf_info->is_jumpmode = config->is_jumpmode;
	buf_info->pts = config->pts;
	buf_info->colorPrimaries = config->hdr_info.colorPrimaries;
	buf_info->matrixCoeffs = config->hdr_info.matrixCoeffs;
	buf_info->is_hdr10plus_synced = 0;
	buf_info->is_metadata_async = config->is_metadata_async;
	buf_info->metadata_already_set = false;
	buf_info->is_enter_async_mode = false;
	buf_info->allm_en = config->allm_en;
	buf_info->new_frame = 0;

	/*
	 **the following info is for hdr2sdr,
	 **saving video buffer info and tv info at vdp config,
	 **and used them at vdp routine.
	 */
	memcpy(&(buf_info->video_disp_buffer), config,
	       sizeof(struct mtk_disp_buffer));
	memcpy(&(buf_info->common_info), info,
	       sizeof(struct disp_hw_common_info));

	/* fill HDR10 type */
	if (config->is_hdr10plus)
		buf_info->hdr10_type = HDR10_TYPE_PLUS;
	else if (config->is_hdr) {
		switch (config
				->hdr_info.transformCharacter) {
				/* 16 for ST2084, 18 for HLG */
		case 16:
			buf_info->hdr10_type = HDR10_TYPE_ST2084;
			break;
		case 18:
			buf_info->hdr10_type = HDR10_TYPE_HLG;
			break;
		default:
			DISP_LOG_E("is_hdr:%d invalid hdr type: %d\n",
				   config->is_hdr,
				   config->hdr_info.transformCharacter);
			goto release_ion_handle;
		}
	} else
		buf_info->hdr10_type = HDR10_TYPE_NONE;

	/*
	 ** Note: HDR10 plus also enable config->is_hdr & config->is_hdr10plus
	 ** for HDR10 plus we also need to copy HDR10 meta data & HDR10 plus
	 *metadata.
	 ** when TV don't support HDR10 plus we use HDR10 signal.
	 */
	if (config->is_hdr) {
		buf_info->hdr10_info.fgNeedUpdStaticMeta = true;
		buf_info->hdr10_info.ui2_DisplayPrimariesX[0] =
			(unsigned short)config->hdr_info.displayPrimariesX[0];
		buf_info->hdr10_info.ui2_DisplayPrimariesX[1] =
			(unsigned short)config->hdr_info.displayPrimariesX[1];
		buf_info->hdr10_info.ui2_DisplayPrimariesX[2] =
			(unsigned short)config->hdr_info.displayPrimariesX[2];
		buf_info->hdr10_info.ui2_DisplayPrimariesY[0] =
			(unsigned short)config->hdr_info.displayPrimariesY[0];
		buf_info->hdr10_info.ui2_DisplayPrimariesY[1] =
			(unsigned short)config->hdr_info.displayPrimariesY[1];
		buf_info->hdr10_info.ui2_DisplayPrimariesY[2] =
			(unsigned short)config->hdr_info.displayPrimariesY[2];
		buf_info->hdr10_info.ui2_MaxCLL =
			(unsigned short)config->hdr_info.maxCll;
		buf_info->hdr10_info.ui2_MaxDisplayMasteringLuminance =
			(unsigned short)
				config->hdr_info.maxDisplayMasteringLuminance;
		buf_info->hdr10_info.ui2_MaxFALL =
			(unsigned short)config->hdr_info.maxFall;
		buf_info->hdr10_info.ui2_MinDisplayMasteringLuminance =
			(unsigned short)
				config->hdr_info.minDisplayMasteringLuminance;
		buf_info->hdr10_info.ui2_WhitePointX =
			(unsigned short)config->hdr_info.whitePointX;
		buf_info->hdr10_info.ui2_WhitePointY =
			(unsigned short)config->hdr_info.whitePointY;
		if (buf_info->hdr10_info.ui2_MaxDisplayMasteringLuminance >
			HDR_MAX_LUMINANCE_LIMIT) {
			buf_info->hdr10_info.ui2_MaxDisplayMasteringLuminance =
			4000;
			buf_info->hdr10_info.ui2_MinDisplayMasteringLuminance =
			50;
			vdp_printf(VDP_FLOW_LOG,
			"clip for max luma err: %d\n",
			config->hdr_info.maxDisplayMasteringLuminance);
		}
		if (buf_info->hdr10_info.ui2_MaxCLL >
			HDR_MAX_LUMINANCE_LIMIT) {
			buf_info->hdr10_info.ui2_MaxFALL = 400;
			buf_info->hdr10_info.ui2_MaxCLL = 4000;
		}
	}

#if defined(CONFIG_MTK_HDR10PLUS_SUPPORT)
	/*copy hdr10+ metadata */
	if (config->is_hdr10plus) {
		buf_info->hdr_info.metadata_info.dovi_metadata.pts =
			config->pts;
		buf_info->hdr_info.metadata_info.dovi_metadata.len =
			config->dovi_info.len;
		if (!config->secruity_en) { /* normal display buffer */
			if (config->dovi_info.len >= DOVI_MD_MAX_LEN) {
				DISP_LOG_E(
					"hdr10 plus metadata size too long: %d\n",
					config->dovi_info.len);
				goto release_ion_handle;
			}
			buf_info->hdr_info.metadata_info
				.dovi_metadata.svp = false;

			if (copy_from_user(
				    buf_info->hdr_info.metadata_info
					    .dovi_metadata.buff,
				    (void __user *)(config->dovi_info.addr),
				    config->dovi_info.len)) {
				DISP_LOG_E("copy hdr10 plus metadata fail\n");
				buf_info->meta_data_size = 0;
				goto release_ion_handle;
			}
		} else { /* secure display buffer */
			config->dovi_info.sec_handle =
				buf_info->mmu1.buffer_addr.sec_handle;
			vdp_printf(VDP_DOVI_LOG,
				   "hdr10+ sec (%d %d %d %d)\n",
				   config->dovi_info.fd,
				   config->dovi_info.offset,
				   config->dovi_info.sec_handle,
				   config->dovi_info.len);
			buf_info->hdr_info.metadata_info
				.dovi_metadata.sec_handle =
				config->dovi_info.sec_handle;
			buf_info->hdr_info.metadata_info
				.dovi_metadata.sec_handle_offset =
				config->dovi_info.offset;
			buf_info->hdr_info.metadata_info
				.dovi_metadata.svp = true;
		}
	}
#else
	if (config->is_hdr10plus) {
		config->is_hdr10plus = 0;
		config->is_hdr = 1;
		buf_info->hdr10_type = HDR10_TYPE_ST2084;
	}
#endif

	disp_vdp_copy_film_grain_md(config, buf_info);

	vdp_printf(VDP_DOVI_LOG,
		   "vdo%d dovi %d hdr_type %d dr=%d->%d pts = %lld\n",
		   buf_info->layer_id, buf_info->is_dovi,
		   buf_info->hdr10_type, buf_info->hdr_info.dr_range,
		   config->dr_range, buf_info->pts);

	vdp_printf(
		VDP_FLOW_LOG,
		"vdo%d ion=%d 0x%X (0x%X 0x%X 0x%X 0x%X)\n",
		buf_info->layer_id,
		buf_info->ion_fd,
		buf_info->src_phy_addr,
		buf_info->ofst_y,
		buf_info->ofst_c,
		buf_info->ofst_y_len,
		buf_info->ofst_c_len);

	vdp_printf(
		VDP_RESOLUTION_LOG,
		"vdo%d, src(%d %d %d %d %d) crop(%d %d %d %d %d) tgt(%d %d %d %d %d)\n",
		buf_info->layer_id, buf_info->src.x, buf_info->src.y,
		buf_info->src.width, buf_info->src.height, buf_info->src.pitch,
		buf_info->crop.x, buf_info->crop.y, buf_info->crop.width,
		buf_info->crop.height, buf_info->crop.pitch, buf_info->tgt.x,
		buf_info->tgt.y, buf_info->tgt.width, buf_info->tgt.height,
		buf_info->tgt.pitch);

	/* fill Dolby HDR info  vdec only add 0 1 2 dr range*/
	buf_info->hdr_info.dr_range = config->dr_range;
	/* fill hlg type */
	if (buf_info->hdr10_type == HDR10_TYPE_HLG)
		buf_info->hdr_info.dr_range = DISP_DR_TYPE_HLG;

#ifdef CONFIG_DOVI_SUPPORT
	if (config->dr_range == DISP_DR_TYPE_DOVI && g_dovi_efuse) {
		struct mtk_disp_dovi_md_t *dovi_info = &config->dovi_info;
		struct mtk_vdp_dovi_md_t *dovi_md_info =
			&buf_info->hdr_info.metadata_info.dovi_metadata;

		buf_info->hdr_info.dr_range = DISP_DR_TYPE_DOVI;

		dovi_md_info->pts = dovi_info->pts;
		dovi_md_info->len = dovi_info->len;
		dovi_md_info->svp = dovi_info->svp;
		dovi_md_info->keyfrm_len = 0;
		memset(&dovi_md_info->buff, 0, DOVI_MD_MAX_LEN);

		if (dovi_info->len >= DOVI_MD_MAX_LEN || dovi_info->len == 0) {
			DISP_LOG_E("dovi_rpu size error: %d\n", dovi_info->len);
			goto release_ion_handle;
		}

		if (dovi_md_info->svp) {
			dovi_md_info->sec_handle = buf_info->mmu1.buffer_addr.sec_handle;
			dovi_md_info->sec_handle_offset = dovi_info->offset;
			if ((dovi_info->keyfrm_len > dovi_info->len) &&
				(dovi_info->keyfrm_len < DOVI_MD_MAX_LEN) &&
				(dovi_info->keyfrm_len - dovi_info->len > RPU_DEPEND_LEN_MIN)) {
				dovi_md_info->keyfrm_sec_handle_offset = dovi_info->keyfrm_offset;
				dovi_md_info->keyfrm_len = dovi_info->keyfrm_len;

			}
			vdp_printf(VDP_DOVI_LOG,
				"dovi frm pts %lld handle 0x%x %d 0x%x %d 0x%x\n",
				config->pts, dovi_md_info->sec_handle,
				dovi_info->len, dovi_info->offset,
				dovi_info->keyfrm_len, dovi_info->keyfrm_offset);
		} else {
			vdp_printf(
				VDP_DOVI_LOG,
				"dovi frm pts %lld rpu pts %lld addr %p %d %d %d %d\n",
				config->pts, dovi_info->pts, dovi_info->addr,
				dovi_info->len, dovi_info->offset,
				dovi_info->keyfrm_len, dovi_info->keyfrm_offset);

			if (copy_from_user(dovi_md_info->buff,
				(void __user *)(dovi_info->addr + dovi_info->offset),
				dovi_info->len)) {
				DISP_LOG_E("dovi info copy from user fail\n");
				goto release_ion_handle;
			}

			if ((dovi_info->keyfrm_len > dovi_info->len) &&
				(dovi_info->keyfrm_len < DOVI_MD_MAX_LEN) &&
				(dovi_info->keyfrm_len - dovi_info->len > RPU_DEPEND_LEN_MIN)) {
				if (copy_from_user(dovi_md_info->keyfrm_buff,
					(void __user *)(dovi_info->addr + dovi_info->keyfrm_offset),
					dovi_info->keyfrm_len)) {
					DISP_LOG_E("dovi keyfrminfo copy from user fail\n");
					goto release_ion_handle;
				}
				dovi_md_info->keyfrm_len = dovi_info->keyfrm_len;
			}

			if (dovi_md_info->len != 0) {
				uint32_t *addr = (uint32_t *)dovi_md_info->buff;

				vdp_printf(
					VDP_DOVI_LOG,
					"dovi rpu value 0x%X 0x%X 0x%X 0x%X\n",
					addr[0], addr[1], addr[2], addr[3]);
			}
		}
	}
#endif
	/* for Dovi debug.
	 ** force set current buffer as dovi video
	 */
	if (force_dr_range >= DISP_DR_TYPE_HDR10 &&
	    force_dr_range <= DISP_DR_TYPE_PHLP_RESVERD) {
		buf_info->hdr_info.dr_range = force_dr_range - 1;

		if (buf_info->hdr_info.dr_range == DISP_DR_TYPE_DOVI) {
			buf_info->is_dovi = true;
			buf_info->hdr10_type = HDR10_TYPE_NONE;
		} else if (buf_info->hdr_info.dr_range == DISP_DR_TYPE_HDR10) {
			buf_info->is_dovi = false;
			buf_info->hdr10_type = HDR10_TYPE_ST2084;
		} else {
			buf_info->is_dovi = false;
			buf_info->hdr10_type = HDR10_TYPE_NONE;
		}
		vdp_printf(VDP_DOVI_LOG,
			   "force dr range %d dovi %d hdr type %d\n",
			   buf_info->hdr_info.dr_range, buf_info->is_dovi,
			   buf_info->hdr10_type);
	}

/*create release fence fd */

	mutex_lock(&(video_layer[config->layer_id].sync_lock));
	fence.fence = VDP_INVALID_FENCE_FD;
	fence.value = ++(video_layer[config->layer_id].fence_idx);
	mutex_unlock(&(video_layer[config->layer_id].sync_lock));
	ret = sprintf(fence.name, "vdp_fence-%d-%d", config->layer_id, fence.value);
	if (ret < 0) {
		DISP_LOG_E("[vdp] fence.name create fail ret %d\n",
			   ret);
		goto release_ion_handle;
	}

	ret = fence_create(
	(struct sync_timeline *)video_layer[config->layer_id].timeline,
		&fence);
	if (ret != 0) {
		DISP_LOG_E(
	"fd leak, create vdp fence fail value:%d status:%d\n",
			fence.value, ret);
		goto release_ion_handle;
	}


	/*fill the release fence fd for hwc */
	config->release_fence_fd = fence.fence;
	buf_info->time_fence_create = sched_clock();
	buf_info->release_fence_fd = fence.fence;
	buf_info->current_fence_index = fence.value;

	config_frame_count[config->layer_id]++;
	if (vdp_cli_get()->enable_pts_debug)
		DISP_LOG_E("config fps %d, pts %lld\n", config->fps,
			config->pts);

	/*insert the video buffer to the buffer list */
	buf_info->buf_state = BUFFER_INSERT;
	mutex_lock(&(video_layer[config->layer_id].sync_lock));
	list_add_tail(&buf_info->list,
		      &(video_layer[config->layer_id].buf_list));
	video_layer[config->layer_id].last_pts = config->pts;
	/* store source and target info to see if reconfigure is needed */
	memcpy((void *)(&(video_layer[config->layer_id].src_rgn)),
	       (const void *)(&(config->src)), sizeof(struct mtk_disp_range));
	memcpy((void *)(&(video_layer[config->layer_id].tgt_rgn)),
	       (const void *)(&(config->tgt)), sizeof(struct mtk_disp_range));

	#ifdef DISP_GCE_SUPPORT
	video_layer[config->layer_id].new_trigger = true;
	#endif
	mutex_unlock(&(video_layer[config->layer_id].sync_lock));

	vdp_printf(VDP_AVSYNC_LOG,
		   "vdo%d pts=%lld rel=%d cur=%d release %d fps %d %d %d",
		   buf_info->layer_id,
		   config->pts,
		   buf_info->release_fence_fd,
		   buf_info->current_fence_index,
		   video_layer[config->layer_id].release_timeline_idx,
		   config->fps,
		   _u4VdpISRCount,
		   _u4VdpISRCount2);

	config_end = get_current_time_ns();
	time = (config_end - config_start) / 1000000;
	if (time > 2)
		vdp_printf(VDP_CAPTURE_LOG,
			   "vdp config exceeds 2ms, time[%dms], pts:%lld\n",
			   time,
			   config->pts);

	return VDP_OK;

release_ion_handle:
	if (buf_info->mmu1.dma_buf) {
		DISP_LOG_E("release dma buffer\n");
		disp_hw_mgr_dma_buffer_release(&buf_info->mmu1);
	}

	/* clean up allocated buffer */
	DISP_LOG_E("vdp layer config fail with error parameter, drop it\n");

	/* buf_info is not add to any list yet, no need to lock. */
	list_del_init(&buf_info->list);
	release_buf_info(&buf_info->list, config->layer_id);

	return VDP_FAIL;
}

#if USE_FAKE_ACTIVE_START
#include <linux/hrtimer.h>
#include <linux/ktime.h>

ktime_t ktime;
struct hrtimer hr_timer;

enum hrtimer_restart vdp_timer_callback(struct hrtimer *hr_timer)
{
	vdp_wakeup_routine(WAKEUP_ALL);
	return HRTIMER_NORESTART;
}

int vdp_set_timer(void)
{
	int interval = 2000; /* us */
	int status = 0;

	do {
		hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ktime = ktime_set(interval / 1000000,
				  (interval % 1000000) * 1000);
		hr_timer.function = &vdp_timer_callback;
		hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
	} while (0);

	return status;
}
#endif

static void disp_vdp_handle_vsync_irq(uint32_t irq)
{
#if (VIDEO_USE_HW_SHADOW && VIDEO_REDUCE_BUFFER)
	unsigned int i = 0;
#endif

	vdp_isr();
#if (VIDEO_USE_HW_SHADOW && VIDEO_REDUCE_BUFFER)
	/* release buffer in vsync irq. */
	for (i = 0; i < VIDEO_LAYER_MAX_COUNT; i++) {
		video_layer[i].release_timeline_idx =
			video_layer[i].timeline_idx;
		queue_work(release_buffer_wq[i],
			   &video_layer[i].task_work);
	}
#endif

#if USE_FAKE_ACTIVE_START
	vdp_set_timer();
#else
	#ifdef DISP_GCE_SUPPORT
	if ((video_layer[0].state == VDP_LAYER_STOPPING) ||
		(video_layer[0].state == VDP_LAYER_STOPPED) ||
		(video_layer[1].state == VDP_LAYER_STOPPING) ||
		(video_layer[1].state == VDP_LAYER_STOPPED))
	#endif
		vdp_wakeup_routine(WAKEUP_ALL);
#endif
}

int disp_vdp_irq_handler(uint32_t irq)
{
	if (!vdp_init_done)
		return VDP_OK;

	if (atomic_read(&vdp_suspend))
		return VDP_OK;

	switch (irq) {
	case DISP_IRQ_FMT_ACTIVE_START:
		disp_vdp_handle_vsync_irq(irq);
		break;
	case DISP_IRQ_FMT_VSYNC:
		break;
	default:
		break;
	}
	return VDP_OK;
}

int disp_vdp_dump(uint32_t level)
{
	return VDP_OK;
}

int disp_vdp_cmd(enum DISP_CMD cmd, void *data)
{
	int is_delay = 0;

	switch (cmd) {
	case DISP_CMD_GET_HDR10PLUS_INFO:
		if (data == NULL) {
			DISP_LOG_E("%s invalid parameter\n", __func__);
			return -1;
		}
		is_delay = vdp_metadata_async();
		*(int *)data = is_delay;
		break;
	default:
		break;
	}

	return 0;
}

int disp_vdp_gce_trigger(struct disp_hw_trigger_info *info)
{
	int ret = VDP_OK;

#ifdef DISP_GCE_SUPPORT
	struct cmdq_pkt *main_pkt = NULL;
	struct cmdq_pkt *sub_pkt = NULL;

	if (video_layer[0].buffer_sec)
		main_pkt = info->sec_pkt;
	else
		main_pkt = info->pkt;

	if (video_layer[1].buffer_sec)
		sub_pkt = info->sec_pkt;
	else
		sub_pkt = info->pkt;
	vdp_hal_get_cmdq_pkt(main_pkt, sub_pkt);

	vdp_prepare_GCE_command(main_pkt, sub_pkt);
	if (video_layer[0].buffer_sec || video_layer[1].buffer_sec)
		info->pkt_update_flag |= sec_pkt_updated;
	if ((!video_layer[0].buffer_sec && video_layer[0].layer_start) ||
		(!video_layer[1].buffer_sec && video_layer[1].layer_start))
		info->pkt_update_flag |= normal_pkt_updated;
#endif

	return ret;
}

int disp_vdp_dbg_level_enable(uint32_t level, uint32_t enable)
{
	if (enable)
		vdp_dbg_level |= (1 << level);
	else
		vdp_dbg_level &= ~(1 << level);
	DISP_LOG_I("set dbg level %d enable %d 0x%X\n", level, enable,
		   vdp_dbg_level);
	return VDP_OK;
}

void disp_vdp_set_not_mix(uint32_t not_mix_type)
{
	if (not_mix_type > 3)
		vdp_not_mix = 3;
	else
		vdp_not_mix = not_mix_type;
}

void disp_vdp_set_not_display(uint32_t not_disp_type)
{
	if (not_disp_type > 3)
			vdp_not_display = 3;
		else
			vdp_not_display = not_disp_type;

}

#ifdef DISP_QMS_SUPPORT
int disp_vdp_vrr_enable(struct vrr_mode *vrr_info)
{
	bool vrr_en;
	uint32_t vtotal_ext;

	if ((video_layer[0].state == VDP_LAYER_IDLE)
		|| (video_layer[0].state == VDP_LAYER_STOPPING)
		|| (video_layer[0].state == VDP_LAYER_STOPPED))
		return 0;

	DISP_LOG_VRR("%s, vrr enable start.\n", __func__);
	vrr_en = vrr_info->vrr_en;
	/*vdp vtotal keep max if qms startline mode enable*/
	if (vrr_en) {
		if (vrr_info->trigger_start_line)
			vtotal_ext = vrr_info->start_line_mode_vtotal;
		else
			vtotal_ext = vrr_info->current_vtotal;
	} else
		vtotal_ext = vrr_info->brr_vtotal;

	DISP_LOG_VRR("%s, enable:%d, vtotal:%d\n",
		__func__, vrr_en, vtotal_ext);

	fmt_hal_dispfmt_vtotal(vtotal_ext);

	DISP_LOG_VRR("%s, vrr enable done.\n", __func__);
	return 0;
}
#endif

/***************** driver ************/
struct disp_hw disp_vdp_driver = {
	.name = VDP_DRV_NAME,
	.init = disp_vdp_init,
	.deinit = disp_vdp_deinit,
	.start = disp_vdp_start,
	.stop = disp_vdp_stop,
	.suspend = disp_vdp_suspend,
	.resume = disp_vdp_resume,
#ifdef DISP_QMS_SUPPORT
	.vrr_enable = disp_vdp_vrr_enable,
#endif
	.get_info = disp_vdp_get_info,
	.change_resolution = disp_vdp_change_resolution,
	.config = disp_vdp_config,
	.irq_handler = disp_vdp_irq_handler,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = disp_vdp_dump,
	.set_cmd = disp_vdp_cmd,
	.gce_trigger = disp_vdp_gce_trigger,
};

struct disp_hw *disp_vdp_get_drv(void)
{
	return &disp_vdp_driver;
}
