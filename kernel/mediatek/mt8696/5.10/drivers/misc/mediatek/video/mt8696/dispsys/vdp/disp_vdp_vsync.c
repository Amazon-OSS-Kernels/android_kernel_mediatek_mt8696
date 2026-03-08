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

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/wait.h>
#define LOG_TAG "VDP_VSYNC"

#include "disp_vdp_if.h"
#include "disp_vdp_vsync.h"
#include "vdp_hal.h"
#include "vdp_hw.h"
/*#include "mt85xx.h"*/
#include "disp_hw_log.h"
#include "disp_hw_mgr.h"
//#include "ion_drv.h"
#include "mtk_disp_mgr.h"
#include "mtk_sync.h"
#include "sync_file.h"
#include "disp_clk.h"
#include "disp_type.h"
#include "disp_path.h"
#include "disp_sys_hal.h"
#include "disp_vdp_cli.h"
#include "disp_vdp_sec.h"
#include "fmt_hal.h"
#include "disp_hdr_if.h"
#include "disp_hdr_main.h"
#include "vdout_sys_hal.h"
#include "disp_hdr_if.h"
//#include "videoin_hal.h" 8696 review
#include "disp_mix_hal.h"
#include "disp_fefifo_drv.h"
#include "disp_fefifo_if.h"
#include "disp_dovi_common_if.h"
#include "disp_dovi_io.h"
#ifdef DISP_GCE_SUPPORT
#include "cmdq-sec.h"
#endif

#include <uapi/linux/sched/types.h>
#include <linux/sched/types.h>

uint32_t vdp_line_cnt[VDP_HW_NS][VDP_MAX_REC];

#define DUMP_VDP_LINE(_vdp_id, vsync_cnt)					\
do {								\
	if ((vdp_dbg_level & VDP_REC_LINE_LOG) &&		\
	    _vdp_id < VDP_HW_NS) {				\
		uint32_t idx, ret = 0;				\
		char line[VDP_REC_BUF];					\
		ret += snprintf(line + ret, VDP_REC_BUF, "%10d", vsync_cnt);	\
		for (idx = 0; idx < VDP_MAX_REC; idx++)		\
			ret += snprintf(line + ret, VDP_REC_BUF - ret, "%5d",	\
			vdp_line_cnt[_vdp_id][idx]);			\
		vdp_printf(VDP_REC_LINE_LOG, "[VDP%d] line %s\n", _vdp_id, line);	\
	}								\
} while (0)

UINT32 _u4VdpISRCount; /* VDP ISR count */
UINT32 _u4VdpISRCount2; /* VDP ISR count */

UINT32 u4VsyncLineTest;
enum HDR10_TYPE_ENUM hdr_enable; /* record buffer HDR enable status */

/*for vdp main thread wake up */
static struct task_struct *disp_vdp_thread0, *disp_vdp_thread1;
static wait_queue_head_t disp_vdp_wq0, disp_vdp_wq1;
static atomic_t gWakeupVdpSwThread0, gWakeupVdpSwThread1;

#ifdef DISP_GCE_SUPPORT
static wait_queue_head_t disp_vdp_gce_wq0, disp_vdp_gce_wq1;
static atomic_t gWakeupVdpGceThread0, gWakeupVdpGceThread1;
struct vdp_hal_fb_addr cur_fb_addr[2];
#endif
#ifdef DISP_QMS_SUPPORT
bool vdp_en_mvrr_frac = true;
#endif

static LIST_HEAD(video_info_pool_main);
static LIST_HEAD(video_info_pool_sub);

static DEFINE_MUTEX(video_info_pool_main_mutex);
static DEFINE_MUTEX(video_info_pool_sub_mutex);

static DEFINE_MUTEX(fence_buffer_mutex);

static int vdp_routine(void *data);

bool print_video_fence_history;

int debug_frame_count[2]; /* vdp show fps */
uint32_t dovi_w_drop;
uint32_t idk_now_num[2];
uint32_t idk_stop_num;
uint32_t idk_vdp_num[2];
uint32_t idk_close_area;
uint32_t idk_no_drop;
uint32_t last_ion_fd[VIDEO_LAYER_MAX_COUNT];

struct video_buffer_info *
disp_video_init_buf_info(struct video_buffer_info *buf)
{
	if (buf == NULL) {
		DISP_LOG_E("buffer info is null\n");
		return buf;
	}
	memset(buf, 0, sizeof(struct video_buffer_info));
	INIT_LIST_HEAD(&buf->list);

	buf->release_fence_fd = VDP_INVALID_FENCE_FD;
	buf->layer_id = 0xFF;
	buf->src_phy_addr = 0;
	buf->source_duration = 0;
	return buf;
}

int vdp_get_list_count(struct list_head *head)
{
	int count = 0;
	struct list_head *pos, *n;

	if (head == NULL || list_empty(head))
		return count;

	list_for_each_safe(pos, n, head) { count++; }

	return count;
}

struct video_buffer_info *vdp_alloc_buf_info(void)
{
	static int allocated_count;
	int value[4] = {0};

	mutex_lock(&video_info_pool_main_mutex);
	value[0] = vdp_get_list_count(&video_info_pool_main);
	mutex_unlock(&video_info_pool_main_mutex);

	mutex_lock(&video_info_pool_sub_mutex);
	value[1] = vdp_get_list_count(&video_info_pool_sub);
	mutex_unlock(&video_info_pool_sub_mutex);

	mutex_lock(&video_layer[0].sync_lock);
	value[2] = vdp_get_list_count(&video_layer[0].buf_list);
	mutex_unlock(&video_layer[0].sync_lock);

	mutex_lock(&video_layer[1].sync_lock);
	value[3] = vdp_get_list_count(&video_layer[1].buf_list);
	mutex_unlock(&video_layer[1].sync_lock);

	DISP_LOG_I("create new buf node, main_pool:%d, sub_pool:%d\n", value[0],
		   value[1]);
	DISP_LOG_I("main_working_pool:%d, sub_working_pool:%d total:%d\n",
		   value[2], value[3], allocated_count++);

	return kzalloc(sizeof(struct video_buffer_info), GFP_KERNEL);
}

/**
 * Query a @mtkfb_fence_buf_info node from @info_pool_head, if empty
 * create a new one
 */
struct video_buffer_info *vdp_get_buf_info(enum VIDEO_LAYER_ID id)
{
	struct video_buffer_info *info = NULL;

	if (id == DISP_MAIN_VIDEO) {
		mutex_lock(&video_info_pool_main_mutex);
		if (!list_empty(&video_info_pool_main)) {
			info = list_first_entry(&video_info_pool_main,
						struct video_buffer_info, list);
			list_del_init(&info->list);
			mutex_unlock(&video_info_pool_main_mutex);
			disp_video_init_buf_info(info);
		} else {
			mutex_unlock(&video_info_pool_main_mutex);
			info = vdp_alloc_buf_info();
			disp_video_init_buf_info(info);
		}
	} else if (id == DISP_SUB_VIDEO) {
		mutex_lock(&video_info_pool_sub_mutex);
		if (!list_empty(&video_info_pool_sub)) {
			info = list_first_entry(&video_info_pool_sub,
						struct video_buffer_info, list);
			list_del_init(&info->list);
			mutex_unlock(&video_info_pool_sub_mutex);
			disp_video_init_buf_info(info);
		} else {
			mutex_unlock(&video_info_pool_sub_mutex);
			info = vdp_alloc_buf_info();
			disp_video_init_buf_info(info);
		}
	} else
		DISP_LOG_E("%s fail\n", __func__);
	return info;
}

void release_buf_info(struct list_head *list, unsigned int layer_id)
{
	if (layer_id == MAIN_VIDEO_INDEX) {
		mutex_lock(&video_info_pool_main_mutex);
		list_add_tail(list, &video_info_pool_main);
		mutex_unlock(&video_info_pool_main_mutex);
	} else if (layer_id == SUB_VIDEO_INDEX) {
		mutex_lock(&video_info_pool_sub_mutex);
		list_add_tail(list, &video_info_pool_sub);
		mutex_unlock(&video_info_pool_sub_mutex);
	} else
		DISP_LOG_E("release invalid layer_id:%d buffer\n", layer_id);
}

void vdp_release_buffer(unsigned int layer_id)
{
	struct video_buffer_info *buf = NULL;
	struct video_buffer_info *temp = NULL;
	struct video_layer_info *layer_info = NULL;
	bool is_Y_C_independent = false;
	struct timespec64 ts;
	unsigned long timestap = 0;

	if (layer_id == MAIN_VIDEO_INDEX)
		layer_info = &video_layer[0];
	else
		layer_info = &video_layer[1];

	list_for_each_entry_safe(buf, temp, &(layer_info->buf_list), list) {
		if (((layer_info->timeline_idx - 1) ==
		     buf->current_fence_index) ||
		    (layer_info->state == VDP_LAYER_STOPPED)) {
			/*signal the fence */
			if ((buf->ion_fd & 0xFFFF000) > 0)
				is_Y_C_independent = true;
			if (layer_info->timeline != NULL) {
				#if 0
				DISP_LOG_E("rel tl_idx|%d, cur_fen_idx|%d\n",
					layer_info->timeline_idx,
					buf->current_fence_index);
				#endif
				timeline_inc(layer_info->timeline, 1);
				buf->time_fence_signal = sched_clock();
				if (print_video_fence_history) {
					ktime_get_raw_ts64(&ts);
					timestap = (unsigned long)ts.tv_sec *
							   1000 +
						   ts.tv_nsec / 1000000;
					DISP_LOG_N(
						"fence%d time 0x%llx, wait 0x%llx pts %lld, timer %ld\n",
						buf->current_fence_index,
						(buf->time_fence_signal -
						 buf->time_fence_create),
						(buf->time_start_display -
						 buf->time_fence_create),
						buf->pts, timestap);
				}
			}
			layer_info->release_idx++;
			/*free ion handle */
			disp_hw_mgr_dma_buffer_release(&buf->mmu1);
			if (is_Y_C_independent)
				disp_hw_mgr_dma_buffer_release(&buf->mmu2);

			list_del_init(&buf->list);
			release_buf_info(&buf->list, layer_id);
			break;
		}
	}
}

void vdp_isr(void)
{
	_u4VdpISRCount2++;

	if (vdp_dbg_level & (VDP_CAPTURE_LOG))
		u4VsyncLineTest = VDOUT_LINE_CNT;

	if (video_layer[0].display_duration >= video_layer[0].vsync_duration)
		video_layer[0].display_duration -=
			video_layer[0].vsync_duration;
	if (video_layer[1].display_duration >= video_layer[1].vsync_duration)
		video_layer[1].display_duration -=
			video_layer[1].vsync_duration;
#if !VIDEO_USE_HW_SHADOW
	vdp_hal_isr(!video_layer[0].secure_en, !video_layer[1].secure_en);
#endif
}

void vdp_vsync_init(void)
{
	/*INT32 i;*/
	/*x_os_isr_fct pfnOldIsr;*/
	/*DDPFUNC();*/

	struct sched_param param = {.sched_priority = 99};
	/*RAI_Init();*/

	init_waitqueue_head(&disp_vdp_wq0);
	init_waitqueue_head(&disp_vdp_wq1);
	atomic_set(&gWakeupVdpSwThread0, 0);
	atomic_set(&gWakeupVdpSwThread1, 0);

#ifdef DISP_GCE_SUPPORT
	init_waitqueue_head(&disp_vdp_gce_wq0);
	init_waitqueue_head(&disp_vdp_gce_wq1);
	atomic_set(&gWakeupVdpGceThread0, 0);
	atomic_set(&gWakeupVdpGceThread1, 0);
#endif

	disp_vdp_thread0 = kthread_create(vdp_routine,
					  (void *)(&(video_layer[0].layer_id)),
					  "video_disp0");

	disp_vdp_thread1 = kthread_create(vdp_routine,
					  (void *)(&(video_layer[1].layer_id)),
					  "video_disp1");
	wake_up_process(disp_vdp_thread0);
	wake_up_process(disp_vdp_thread1);

	sched_setscheduler(disp_vdp_thread0, SCHED_FIFO, &param);

	print_video_fence_history = false;
	hdr_enable = HDR10_TYPE_NONE;

	/*VDP_FrcInit ();*/

	/*VPQ_Init();*/
	/*clear message queue */
	/*VDP_MEMSET ((void *) _arMsgQ, 0, sizeof (VDP_MSG_Q_T) * VDP_MAX_NS);*/
	/*VDP_MEMSET ((void *) VdpCmdQ, 0, sizeof (VDP_CMD_Q_T));*/

	/*for (i = 0; i < VDP_MAX_NS; i++)*/
	/*VdpProcSequence[i] = i;*/

	/*reg ISR (move to HAL ? Since VECTOR is platform dependent!)*/
	/*VERIFY(x_reg_isr(VECTOR_DISP_VSYNC, _VDPVsyncIsr, &pfnOldIsr) ==
	 * OSR_OK);
	 */
}

void vdp_wakeup_routine(enum WAKE_UP_TYPE type)
{
	if (type == WAKEUP_ALL) {
		atomic_set(&gWakeupVdpSwThread0, 1);
		wake_up(&disp_vdp_wq0);
		atomic_set(&gWakeupVdpSwThread1, 1);
		wake_up(&disp_vdp_wq1);
	} else if (type == WAKEUP_MAIN) {
		atomic_set(&gWakeupVdpSwThread0, 1);
		wake_up(&disp_vdp_wq0);
	} else if (type == WAKEUP_SUB) {
		atomic_set(&gWakeupVdpSwThread1, 1);
		wake_up(&disp_vdp_wq1);
	}
}

#ifdef DISP_GCE_SUPPORT
void vdp_wakeup_GCE(unsigned int layer_id)
{
	if (layer_id == 0) {
		atomic_set(&gWakeupVdpGceThread0, 1);
		wake_up(&disp_vdp_gce_wq0);
	} else if (layer_id == 1) {
		atomic_set(&gWakeupVdpGceThread1, 1);
		wake_up(&disp_vdp_gce_wq1);
	}
}

void vdp_prepare_GCE_command(struct cmdq_pkt *pkt_main,
	struct cmdq_pkt *pkt_sub)
{
	int ret = 0;

	if (video_layer[0].buffer_sec)
		cmdq_sec_pkt_set_data(pkt_main,
		0,
		1 << CMDQ_SEC_DISP_VDO3_Y0,
		CMDQ_SEC_PRIMARY_DISP, CMDQ_METAEX_FD);

	if (video_layer[1].buffer_sec)
		cmdq_sec_pkt_set_data(pkt_sub,
		0,
		1 << CMDQ_SEC_DISP_VDO4_Y0,
		CMDQ_SEC_PRIMARY_DISP, CMDQ_METAEX_FD);
	/*update main*/
	if (video_layer[0].layer_start) {
		ret = wait_event_timeout(disp_vdp_gce_wq0,
			atomic_read(&gWakeupVdpGceThread0),
			msecs_to_jiffies(1000));

		if (ret == 0) {
			DISP_LOG_E("wait vdp0 process done timeout\n");
			atomic_set(&gWakeupVdpGceThread0, 0);
		} else {
			atomic_set(&gWakeupVdpGceThread0, 0);
			vdp_hal_isr(1, 0);
			if (video_layer[0].buffer_sec) {
				vdp_hal_config_sec_buffer(0, &cur_fb_addr[0]);
				fmt_hal_config_sec_register(0, pkt_main);
			}
		}
	}
	/*update sub */
	if (video_layer[1].layer_start) {
		ret = wait_event_timeout(disp_vdp_gce_wq1,
				atomic_read(&gWakeupVdpGceThread1),
				msecs_to_jiffies(1000));
		if (ret == 0) {
			DISP_LOG_E("wait vdp1 process done timeout\n");
			atomic_set(&gWakeupVdpGceThread1, 0);
		} else {
			atomic_set(&gWakeupVdpGceThread1, 0);
			vdp_hal_isr(0, 1);
			if (video_layer[1].buffer_sec) {
				vdp_hal_config_sec_buffer(1,
					&cur_fb_addr[1]);
				fmt_hal_config_sec_register(1,
					pkt_sub);
				}

		}
	}
}
#endif

static void vdp_config_film_grain(u32 vdp_id, struct video_buffer_info *buf)
{
	struct mtk_av1_film_grain_params *fg_param = NULL;
	struct mtk_disp_film_grain_md_t film_grain_info = {0};
	int ret = 0;
	struct video_scale_info scale_info = {0};
	struct mtk_disp_vdp_cap *vdp_cap = &vdp_scale_info[vdp_id];

	if (!buf->is_film_grain)
		return;
	if (buf->film_grain_info.svp) {
		memset(&film_grain_info, 0, sizeof(film_grain_info));
		film_grain_info.sec_handle =
			buf->film_grain_info.sec_handle;
		film_grain_info.offset =
			buf->film_grain_info.sec_handle_offset;
		film_grain_info.len = buf->film_grain_info.len;
		film_grain_info.pts = buf->film_grain_info.pts;
		/* get film grain info from tz */
		ret = fg_sec_get_params(&film_grain_info);
		if (ret)
			return;
		memcpy(buf->film_grain_info.buff, film_grain_info.addr,
			film_grain_info.len);

		vdp_printf(VDP_FG_LOG,
			   "fg handle %d %d %d pts %lld\n",
			   film_grain_info.sec_handle,
			   film_grain_info.offset,
			   film_grain_info.len,
			   film_grain_info.pts);
	}

	if (vdp_cap->need_resizer) {
		scale_info.src_w = vdp_cap->crop.width;
		scale_info.src_h = vdp_cap->crop.height;
	} else {
		scale_info.src_w = buf->crop.width;
		scale_info.src_h = buf->crop.height;
	}
	scale_info.dst_w = buf->tgt.width;
	scale_info.dst_h = buf->tgt.height;
	disp_fg_config_scale_info(vdp_id, &scale_info);

	fg_param = (struct mtk_av1_film_grain_params *)buf->film_grain_info.buff;
	disp_fg_config(vdp_id, fg_param);
}

/*convert the input information to hw structure */
void vdp_set_hal_config(struct video_buffer_info *buf1,
			struct video_buffer_info *buf2,
			struct video_buffer_info *buf3,
			struct vdp_hal_config_info *hal_config)
{
	bool is_Y_C_independent = false;

	mutex_lock(&video_layer[hal_config->vdp_id].sync_lock);
	if (buf1 == NULL) {
		DISP_LOG_E("current buffer info is NULL\n");
		mutex_unlock(&video_layer[hal_config->vdp_id].sync_lock);
		return;
	}
	if (((buf1->ion_fd & 0xFFFF0000) > 0) && (!buf1->secruity_en))
		is_Y_C_independent = true;

	/* hdr10 plus metadata copy */
	hal_config->cur_fb_info.is_hdr10plus =
		(buf1->hdr10_type == HDR10_TYPE_PLUS);
	hal_config->cur_fb_info.metadata_sec_handle =
		buf1->hdr_info.metadata_info.dovi_metadata.sec_handle;
	hal_config->cur_fb_info.metadata_size =
		buf1->hdr_info.metadata_info.dovi_metadata.len;

	hal_config->is_el_exist = false;
	hal_config->enable = true;
	hal_config->is_second_field = false;
	hal_config->cur_fb_info.is_secruity = buf1->secruity_en;
	if (buf1->secruity_en || video_layer[buf1->layer_id].secure2normal) {
		hal_config->cur_fb_info.fb_addr.sec_handle = buf1->src_phy_addr;
		hal_config->cur_fb_info.fb_addr.addr_y = buf1->ofst_y;
		hal_config->cur_fb_info.fb_addr.addr_c = buf1->ofst_c;
		hal_config->cur_fb_info.fb_addr.buffer_size = buf1->buffer_size;
	} else {
		if (is_Y_C_independent) {
			hal_config->cur_fb_info.fb_addr.addr_y =
				buf1->src_phy_addr + buf1->ofst_y;
			hal_config->cur_fb_info.fb_addr.addr_c =
				buf1->src_phy_addr2 + buf1->ofst_c;
		} else {
			hal_config->cur_fb_info.fb_addr.addr_y =
				buf1->src_phy_addr + buf1->ofst_y;
			hal_config->cur_fb_info.fb_addr.addr_c =
				buf1->src_phy_addr + buf1->ofst_c;
		}
	}

	hal_config->cur_fb_info.pts = buf1->pts;

	hal_config->cur_fb_info.is_10bit = buf1->is_10bit;
	hal_config->cur_fb_info.is_10bit_tile_mode =
		buf1->is_10bit_lbs2bit_tile_mode;
	hal_config->cur_fb_info.is_dovi = buf1->is_dovi;
	hal_config->cur_fb_info.is_interlace = buf1->is_interlace;
	hal_config->cur_fb_info.is_ufo = buf1->is_ufo;
	hal_config->cur_fb_info.is_jumpmode = buf1->is_jumpmode;

	hal_config->cur_fb_info.fb_size.pic_w = buf1->crop.width;
	hal_config->cur_fb_info.fb_size.pic_h = buf1->crop.height;
	hal_config->cur_fb_info.fb_size.buff_w = buf1->src.pitch;
	hal_config->cur_fb_info.fb_size.buff_h = buf1->src.height;
	hal_config->cur_fb_info.src_region.x = buf1->src.x;
	hal_config->cur_fb_info.src_region.y = buf1->src.y;
	hal_config->cur_fb_info.src_region.width = buf1->crop.width;
	hal_config->cur_fb_info.src_region.height = buf1->crop.height;
	hal_config->cur_fb_info.out_region.x = buf1->tgt.x;
	hal_config->cur_fb_info.out_region.y = buf1->tgt.y;
	hal_config->cur_fb_info.out_region.width = buf1->tgt.width;
	hal_config->cur_fb_info.out_region.height = buf1->tgt.height;
	if ((buf1->src_fmt == DISP_HW_COLOR_FORMAT_YUV420_BLOCK) ||
	    (buf1->src_fmt == DISP_HW_COLOR_FORMAT_YUV422_BLOCK))
		hal_config->cur_fb_info.is_scan_line = false;
	else if ((buf1->src_fmt == DISP_HW_COLOR_FORMAT_YUV420_RASTER) ||
		 (buf1->src_fmt == DISP_HW_COLOR_FORMAT_YUV422_RASTER))
		hal_config->cur_fb_info.is_scan_line = true;
	else {
		DISP_LOG_E("VDP -- invalid color format%d\n",
			buf1->src_fmt);
		hal_config->cur_fb_info.is_scan_line = true;
	}

	if ((buf1->src_fmt == DISP_HW_COLOR_FORMAT_YUV420_BLOCK) ||
	    (buf1->src_fmt == DISP_HW_COLOR_FORMAT_YUV420_RASTER))
		hal_config->cur_fb_info.is_yuv422 = false;
	else if ((buf1->src_fmt == DISP_HW_COLOR_FORMAT_YUV422_BLOCK) ||
		 (buf1->src_fmt == DISP_HW_COLOR_FORMAT_YUV422_RASTER))
		hal_config->cur_fb_info.is_yuv422 = true;
	else {
		DISP_LOG_E("VDP -- invalid color format%d\n",
			buf1->src_fmt);
		hal_config->cur_fb_info.is_yuv422 = true;
	}

	if (buf1->is_ufo) {
		if (is_Y_C_independent) {
			hal_config->cur_fb_info.fb_addr.addr_y_len =
				buf1->src_phy_addr + buf1->ofst_y_len;
			hal_config->cur_fb_info.fb_addr.addr_c_len =
				buf1->src_phy_addr2 + buf1->ofst_c_len;
		} else {
			hal_config->cur_fb_info.fb_addr.addr_y_len =
				buf1->src_phy_addr + buf1->ofst_y_len;
			hal_config->cur_fb_info.fb_addr.addr_c_len =
				buf1->src_phy_addr + buf1->ofst_c_len;
		}
		if (buf1->secruity_en ||
		    video_layer[buf1->layer_id].secure2normal) {
			hal_config->cur_fb_info.fb_addr.addr_y_len =
				buf1->ofst_y_len;
			hal_config->cur_fb_info.fb_addr.addr_c_len =
				buf1->ofst_c_len;
		}
	}

	if (buf1->is_interlace) {
		if ((buf2 == NULL) || (buf3 == NULL)) {
			DISP_LOG_E("reference buffer info is NULL\n");
			mutex_unlock(
				&video_layer[hal_config->vdp_id].sync_lock);
			return;
		}
	}

	vdp_printf(VDP_AVSYNC_LOG,
		   "cfg hal pts %lld %d disp_dur %d vsync %d %d %d\n",
		   buf1->pts,
		   buf1->source_duration,
		   video_layer[hal_config->vdp_id].display_duration,
		   video_layer[hal_config->vdp_id].vsync_duration,
		   _u4VdpISRCount,
		   _u4VdpISRCount2);

	vdp_printf(VDP_RESOLUTION_LOG,
		   "cfg hal pts %lld fd %d %p (0x%X 0x%X 0x%X 0x%X)\n",
		   buf1->pts,
		   buf1->ion_fd,
		   buf1->src_phy_addr,
		   buf1->ofst_y,
		   buf1->ofst_c,
		   buf1->ofst_y_len,
		   buf1->ofst_c_len);

	vdp_printf(VDP_RESOLUTION_LOG,
		   "addr (0x%X 0x%X 0x%X 0x%X)\n",
		   hal_config->cur_fb_info.fb_addr.addr_y,
		   hal_config->cur_fb_info.fb_addr.addr_c,
		   hal_config->cur_fb_info.fb_addr.addr_y_len,
		   hal_config->cur_fb_info.fb_addr.addr_c_len);

	mutex_unlock(&video_layer[hal_config->vdp_id].sync_lock);

}

#if 0
static void vdp_set_disp_fmt_info(struct fmt_active_info *active_info,
	struct vdp_hal_disp_info *vdp_hal_info)
{
	active_info->h_begine = vdp_hal_info->h_start;
	active_info->h_end = vdp_hal_info->h_end;
	active_info->v_odd_begine = vdp_hal_info->v_odd_start;
	active_info->v_odd_end = vdp_hal_info->v_odd_end;
	active_info->v_even_begine = vdp_hal_info->v_even_start;
	active_info->v_even_end = vdp_hal_info->v_even_end;
}
#endif

static void vdp_state_switch(enum VDP_LAYER_STATE src_state,
			     enum VDP_LAYER_STATE dst_state,
			     struct video_layer_info *layer_info)
{
	if (layer_info->state == src_state)
		layer_info->state = dst_state;
}

static bool vdp_check_2160p_timing(enum HDMI_VIDEO_RESOLUTION res_mode)
{
	switch (res_mode) {
	case HDMI_VIDEO_3840x2160P_23_976HZ:
	case HDMI_VIDEO_3840x2160P_24HZ:
	case HDMI_VIDEO_3840x2160P_25HZ:
	case HDMI_VIDEO_3840x2160P_29_97HZ:
	case HDMI_VIDEO_3840x2160P_30HZ:
	case HDMI_VIDEO_3840x2160P_60HZ:
	case HDMI_VIDEO_3840x2160P_50HZ:
		return true;
	default:
		return false;
	}

	return false;
}

bool vdp_check_2160p60_timing(enum HDMI_VIDEO_RESOLUTION res_mode)
{
	if ((res_mode == HDMI_VIDEO_3840x2160P_60HZ) ||
	    (res_mode == HDMI_VIDEO_3840x2160P_50HZ) ||
	    (res_mode == HDMI_VIDEO_4096x2160P_60HZ) ||
	    (res_mode == HDMI_VIDEO_4096x2160P_50HZ))
		return true;
	else
		return false;
}

#if 0
static int vdp_check_info_update(unsigned char vdp_id,
	struct vdp_hal_config_info *hal_config,
	bool check_only)
{
	struct video_layer_info *layer_info;
	int ret = 0;

	layer_info = &video_layer[vdp_id];
	if (layer_info->enable != hal_config->enable) {
		ret |= VDP_INFO_EN_UPDATE;
		if (!check_only)
			layer_info->enable = hal_config->enable;
	}
	if ((layer_info->src_rgn.x != hal_config->cur_fb_info.src_region.x) ||
	(layer_info->src_rgn.y != hal_config->cur_fb_info.src_region.y) ||
	(layer_info->src_rgn.width !=
	hal_config->cur_fb_info.src_region.width) ||
	(layer_info->src_rgn.height !=
	hal_config->cur_fb_info.src_region.height)) {
		ret |= VDP_INFO_IN_REGION_UPDATE;
	if (!check_only) {
		layer_info->src_rgn.x = hal_config->cur_fb_info.src_region.x;
		layer_info->src_rgn.y = hal_config->cur_fb_info.src_region.y;
		layer_info->src_rgn.width =
			hal_config->cur_fb_info.src_region.width;
		layer_info->src_rgn.height =
			hal_config->cur_fb_info.src_region.height;
		}
	}
	if ((layer_info->tgt_rgn.x !=
		hal_config->cur_fb_info.out_region.x) ||
		(layer_info->tgt_rgn.y !=
		hal_config->cur_fb_info.out_region.y) ||
		(layer_info->tgt_rgn.width !=
		hal_config->cur_fb_info.out_region.width) ||
		(layer_info->tgt_rgn.height !=
		hal_config->cur_fb_info.out_region.height)) {
		ret |= VDP_INFO_OUT_REGION_UPDATE;
		if (!check_only) {
			layer_info->tgt_rgn.x =
				hal_config->cur_fb_info.out_region.x;
			layer_info->tgt_rgn.y =
				hal_config->cur_fb_info.out_region.y;
			layer_info->tgt_rgn.width =
				hal_config->cur_fb_info.out_region.width;
			layer_info->tgt_rgn.height =
				hal_config->cur_fb_info.out_region.height;
		}
	}
	return ret;
}
#endif

int vdp_check_dsd_available(unsigned char vdp_id,
			    struct vdp_hal_config_info *hal_config)
{
	int ret = 0;

	if ((hal_config->cur_fb_info.src_region.height >
	     disp_common_info.resolution->height) ||
	    (hal_config->cur_fb_info.src_region.width >
	     disp_common_info.resolution->width)) {
		if ((disp_common_info.resolution->width >
		     hal_config->cur_fb_info.out_region.width * 8) ||
		    (disp_common_info.resolution->height >
		     hal_config->cur_fb_info.out_region.height * 2))
			ret = 0;
		else {
			if ((disp_common_info.resolution->width == 1280) &&
			    (disp_common_info.resolution->height == 720) &&
			    (hal_config->cur_fb_info.src_region.height <=
			     1080) &&
			    (hal_config->cur_fb_info.src_region.width <= 1920))
				ret = 0;
			else
				ret = 1;
		}
	} else
		ret = 0;

	return ret;
}

static enum DSD_CASE_E vdp_set_dsd_case(uint16_t src_width, uint16_t src_height)
{
	enum DSD_CASE_E dsd_case = DSD_NONE;

	switch (disp_common_info.resolution->res_mode) {
	case HDMI_VIDEO_1920x1080p_60Hz:
	case HDMI_VIDEO_1920x1080p_50Hz:
		if ((src_width > 1920) || (src_height > 1080))
			dsd_case = DSD_4K_TO_1080P;
		break;
	case HDMI_VIDEO_720x480p_60Hz:
	case HDMI_VIDEO_720x576p_50Hz:
		if ((src_width > 1920) || (src_height > 1080))
			dsd_case = DSD_4K_TO_480P;
		else if ((src_width > 1280) || (src_height > 720))
			dsd_case = DSD_1080P_TO_480P;
		else if ((src_width > 720) || (src_height > 480))
			dsd_case = DSD_720P_TO_480P;
		break;
	case HDMI_VIDEO_1280x720p_60Hz:
	case HDMI_VIDEO_1280x720p_50Hz:
		if ((src_width > 1920) || (src_height > 1080))
			dsd_case = DSD_4K_TO_720P;
		else
			dsd_case = DSD_1080P_TO_720P;
		break;
	default:
		dsd_case = DSD_NONE;
		break;
	}
	return dsd_case;
}

static void vdp_select_dsd_pll(unsigned char vdp_id, enum DSD_CASE_E dsd_type)
{
	switch (dsd_type) {
	case DSD_4K_TO_480P:
		/*648M */
		disp_clock_select_pll(DISP_CLK_VDO3_SEL, DISP_CLK_OSDPLL);
		break;
	case DSD_4K_TO_720P:
		/*594 */
		disp_clock_select_pll(DISP_CLK_VDO3_SEL, DISP_CLK_TVDPLL);
		break;
	case DSD_4K_TO_1080P:
		/*594 */
		disp_clock_select_pll(DISP_CLK_VDO3_SEL, DISP_CLK_TVDPLL);
		break;
	case DSD_1080P_TO_480P:
	case DSD_720P_TO_480P:
		if (vdp_id == 0)
			disp_clock_select_pll(DISP_CLK_VDO3_SEL,
					      DISP_CLK_OSDPLL_D4);
		else
			disp_clock_select_pll(DISP_CLK_VDO4_SEL,
					      DISP_CLK_OSDPLL_D4);
		break;
	case DSD_1080P_TO_720P:
		break;
	default:
		break;
	}
}

extern uint32_t vdp_not_mix;
static void vdp_config_fefifo_dispmix(uint8_t vdp_id, uint8_t is_dovi,
	uint32_t x_offset, uint32_t y_offset, uint32_t dst_width,
	uint32_t dst_height, struct fmt_active_info dispfmt_active_info,
	struct fmt_hd_scl_info hd_scl_info)
{
	struct DISP_PATH_LAYER_INFO layer_info = { 0 };
	struct DISP_PATH_LOCATION_INFO location_info = { 0 };
	bool b_dummy = false;
	bool b_hdr_drop = false;
	uint32_t disp_fmt_add = 0;
	uint32_t disp_mix_add = 0;
	uint32_t fe_fifo_add = 0;
	uint32_t disp_v_add = 0;
	uint32_t width_overflow = 0;

	vdp_printf(VDP_DOVI_LOG, "%s %d (%d %d %d %d) (%d %d %d %d %d %d)\n",
		__func__, is_dovi,
		x_offset, y_offset, dst_width, dst_height,
		dispfmt_active_info.h_begine, dispfmt_active_info.h_end,
		dispfmt_active_info.v_even_begine,
		dispfmt_active_info.v_even_end,
		dispfmt_active_info.v_odd_begine,
		dispfmt_active_info.v_odd_end);
	vdp_printf(VDP_DOVI_LOG, "%s ver(%d) %d %d %d %d %d %d %d\n", __func__,
		g_ic_version,
		hd_scl_info.hd_scl_on,
		hd_scl_info.dispfmt_out_h_start,
		hd_scl_info.dispfmt_out_h_end,
		hd_scl_info.out_y_odd_pos, hd_scl_info.out_y_odd_pos_e,
		hd_scl_info.out_y_even_pos, hd_scl_info.out_y_even_pos_e);

#if 0
	if ((vdp_id == SUB_VIDEO_INDEX) && (_subv_type == 0))
		is_dovi = 0;
	vdp_printf(VDP_DOVI_LOG, "idk24 flow %d\n", is_dovi);
#endif
	if (is_dovi) {
		/*eco flow,8 case of h 4 pixel align and V 2 line align*/
		if (x_offset % 2 == 0) {
			b_dummy = false;
			switch (dst_width % 4) {
			case 1:
				/*fmtwidth+3, fefifo+3 ,E1 mix+2*/
				/*E2 mix +0 & hdrdrop 2*/
				disp_fmt_add = 3;
				fe_fifo_add = 3;
				disp_mix_add = 2;
				b_hdr_drop = true;
				break;
			case 2:
				/*fmtwidth+2, fefifo+2 ,E1 mix+2*/
				/*E2 mix +0 & hdrdrop 2*/
				disp_fmt_add = 2;
				fe_fifo_add = 2;
				disp_mix_add = 2;
				b_hdr_drop = true;
				break;
			case 3:
				/*fmtwidth+1, fefifo+1 ,E1 mix + 0*/
				/*E2 mix+0 & hdrdrop 0*/
				disp_fmt_add = 1;
				fe_fifo_add = 1;
				disp_mix_add = 0;
				b_hdr_drop = false;
				break;
			default:
				/*OK case*/
				disp_fmt_add = 0;
				fe_fifo_add = 0;
				disp_mix_add = 0;
				b_hdr_drop = false;
				break;
			}
		} else {
			b_dummy = true;
			switch (dst_width % 4) {
			case 0:
				/*fmtwidth+3, fefifo+4 ,E1 mix+2*/
				/*E2 mix +0 & hdrdrop 2*/
				disp_fmt_add = 3;
				fe_fifo_add = 4;
				disp_mix_add = 2;
				b_hdr_drop = true;
				break;
			case 1:
				/*fmtwidth+2, fefifo+3 ,E1 mix+2*/
				/*E2 mix +0 & hdrdrop 2*/
				disp_fmt_add = 2;
				fe_fifo_add = 3;
				disp_mix_add = 2;
				b_hdr_drop = true;
				break;
			case 2:
				/*fmtwidth+1, fefifo+2 ,E1 mix + 0*/
				/*E2 mix+0 & hdrdrop 0*/
				disp_fmt_add = 1;
				fe_fifo_add = 2;
				disp_mix_add = 0;
				b_hdr_drop = false;
				break;
			default:
				/*OK case*/
				disp_fmt_add = 0;
				fe_fifo_add = 1;
				disp_mix_add = 0;
				b_hdr_drop = false;
				break;
			}
		}

		if (dst_height % 2 == 1)
			disp_v_add = 1;
		else
			disp_v_add = 0;

		if (dovi_w_drop || (g_ic_version == 1)) {
			// drop 2 pixel by hw
			vdp_printf(VDP_DOVI_LOG, "Drop 2 pixel by HW [%d %d]\n",
			disp_mix_add, b_hdr_drop);
			disp_mix_add = 0;
		}

		layer_info.src_width = dst_width + disp_mix_add;
		layer_info.src_height = dst_height;
		width_overflow = dst_width + disp_mix_add + x_offset;
		if ((width_overflow > disp_common_info.resolution->width)
			&& (x_offset >= disp_mix_add))
			layer_info.x_offset = x_offset - disp_mix_add;
		else
			layer_info.x_offset = x_offset;
		layer_info.y_offset = y_offset;
		layer_info.type = vdp_id;
		if ((vdp_not_mix == 2) ||
			((vdp_not_mix == 0) && (vdp_id == 0)) ||
			((vdp_not_mix == 1) && (vdp_id == 1)) ||
			(video_layer[vdp_id].state == VDP_LAYER_STOPPING) ||
			(video_layer[vdp_id].state == VDP_LAYER_STOPPED)) {
			layer_info.enable = false;
			if ((video_layer[vdp_id].state == VDP_LAYER_STOPPING) ||
			(video_layer[vdp_id].state == VDP_LAYER_STOPPED))
			DISP_LOG_I("enable disp_mix in wrong state%d\n",
			video_layer[vdp_id].state);
		}
		else
			layer_info.enable = true;
		location_info.layer_location_id = vdp_id;
		location_info.layer_type = vdp_id;
		disp_mix_hal_layer_control(&layer_info);
		disp_mix_hal_set_layer_location(&location_info);

		disp_fefifo_set_src_res(vdp_id, dst_width + fe_fifo_add,
			dst_height + disp_v_add, 0);
		disp_set_hdr_fe_input_size(vdp_id, dst_width + fe_fifo_add,
			dst_height);
		if (!hd_scl_info.hd_scl_on) {
			dispfmt_active_info.h_end += disp_fmt_add;
			dispfmt_active_info.v_even_end += disp_v_add;
			dispfmt_active_info.v_odd_end += disp_v_add;
			fmt_hal_set_active_zone(vdp_id, &dispfmt_active_info);
		} else {
			hd_scl_info.dispfmt_out_h_end += disp_fmt_add;
			hd_scl_info.out_y_odd_pos_e += disp_v_add;
			hd_scl_info.out_y_even_pos_e += disp_v_add;
			dispfmt_active_info.v_even_end += disp_v_add;
			dispfmt_active_info.v_odd_end += disp_v_add;
			fmt_hal_set_active_zone(vdp_id, &dispfmt_active_info);
			fmt_hal_set_hdownscl_hv(vdp_id, &hd_scl_info);
		}

		fmt_hal_set_dummy(vdp_id, b_dummy);
		//only E2 this bit work
		if (dovi_w_drop || (g_ic_version == 1)) {
			if (is_hd_resolution())
				disp_sys_hal_set_hdr_drop_shadow(vdp_id, b_hdr_drop);
			else
				disp_sys_hal_set_hdr_drop(vdp_id, b_hdr_drop);
		}

	} else {
		/*only E2 this bit work*/
		if (dovi_w_drop || (g_ic_version == 1)) {
			if (is_hd_resolution())
				disp_sys_hal_set_hdr_drop_shadow(vdp_id, 0);
			else
				disp_sys_hal_set_hdr_drop(vdp_id, 0);
		}

		/*normal flow*/
		if (x_offset % 2 == 0) {
			b_dummy = false;
			if (dst_width % 2 == 1) {
				disp_fmt_add = 1;
				fe_fifo_add = 1;
			}

		} else {
			b_dummy = true;
			if (dst_width % 2 == 0) {
				disp_fmt_add = 1;
				fe_fifo_add = 2;
			} else {
				disp_fmt_add = 0;
				fe_fifo_add = 1;
			}
		}
		layer_info.src_width = dst_width;
		layer_info.src_height = dst_height;
		layer_info.x_offset = x_offset;
		layer_info.y_offset = y_offset;
		layer_info.type = vdp_id;
		if ((vdp_not_mix == 2) ||
			((vdp_not_mix == 0) && (vdp_id == 0)) ||
			((vdp_not_mix == 1) && (vdp_id == 1)) ||
			(video_layer[vdp_id].state == VDP_LAYER_STOPPING) ||
			(video_layer[vdp_id].state == VDP_LAYER_STOPPED)) {
			layer_info.enable = false;
			if ((video_layer[vdp_id].state == VDP_LAYER_STOPPING) ||
			(video_layer[vdp_id].state == VDP_LAYER_STOPPED))
			DISP_LOG_I("enable disp_mix in wrong state%d\n",
			video_layer[vdp_id].state);
		}
		else
			layer_info.enable = true;
		location_info.layer_location_id = vdp_id;
		location_info.layer_type = vdp_id;

		REC_VDP_LINE(vdp_id, VDP_MIX_CONFIG);

		disp_mix_hal_layer_control(&layer_info);

		disp_mix_hal_set_layer_location(&location_info);

		disp_fefifo_set_src_res(vdp_id, dst_width + fe_fifo_add,
			dst_height, 0);
		disp_set_hdr_fe_input_size(vdp_id, dst_width + fe_fifo_add,
			dst_height);
		fmt_hal_set_dummy(vdp_id, b_dummy);

		REC_VDP_LINE(vdp_id, VDP_DISPFMT_IN);

		if (!hd_scl_info.hd_scl_on) {
			dispfmt_active_info.h_end += disp_fmt_add;
			fmt_hal_set_active_zone(vdp_id, &dispfmt_active_info);
		} else {
			hd_scl_info.dispfmt_out_h_end += disp_fmt_add;
			fmt_hal_set_active_zone(vdp_id, &dispfmt_active_info);
			fmt_hal_set_hdownscl_hv(vdp_id, &hd_scl_info);
		}

		REC_VDP_LINE(vdp_id, VDP_DISPFMT_OUT);

		#if 0
		/*for odd/even align with disp_mix*/
		if ((x_offset % 2 == 1) && (dst_width % 2 == 1)) {
			fmt_hal_set_dummy(vdp_id, true);
			disp_fefifo_set_src_res(vdp_id,
				dst_width + 1, dst_height, 0);
		} else if ((x_offset % 2 == 0) && (dst_width % 2 == 1)) {
			dispfmt_active_info.h_end += 1;
			fmt_hal_set_active_zone(vdp_id, &dispfmt_active_info);
			dispfmt_active_info.h_end -= 1;
			disp_fefifo_set_src_res(vdp_id,
				dst_width + 1, dst_height, 0);
		} else if ((x_offset % 2 == 1) && (dst_width % 2 == 0)) {
			fmt_hal_set_dummy(vdp_id, true);
			dispfmt_active_info.h_end += 1;
			fmt_hal_set_active_zone(vdp_id, &dispfmt_active_info);
			dispfmt_active_info.h_end -= 1;
			disp_fefifo_set_src_res(MAIN_VIDEO_INDEX,
			dst_width + 2, dst_height, 0);
		} else
			fmt_hal_set_dummy(vdp_id, false);
		#endif
	}
}

static void vdp_enable_dispfmt_with_shadow(unsigned char vdp_id)
{
	/*work around for dispfmt enable flow*/
	if (vdp_id == MAIN_VIDEO_INDEX) {

		if (main_fmt_first) {
			/* dispfmt enable moved into fmt_hal_hw_shadow_enable
			 * for none gce case, make register write sequence not
			 * interrupted by multi cpu schedule */
			#ifdef DISP_GCE_SUPPORT
			fmt_hal_enable(DISP_FMT_MAIN, true);
			#else
			fmt_hal_hw_shadow_enable(DISP_FMT_MAIN);
			#endif
			main_fmt_first = false;
		}
	} else if (vdp_id == SUB_VIDEO_INDEX) {
		if (sub_fmt_first) {
			#ifdef DISP_GCE_SUPPORT
			fmt_hal_enable(DISP_FMT_SUB, true);
			#else
			fmt_hal_hw_shadow_enable(DISP_FMT_SUB);
			#endif
			sub_fmt_first = false;
		}
	}
}
static void vdp_update_fmt_setting(unsigned char vdp_id,
				   struct vdp_hal_config_info *config_info)
{
	int32_t status = FMT_OK;
	uint8_t dsd_en = 0;
	struct fmt_dsd_scl_info fmt_dsd_scl_info = {0};
	uint32_t h_factor = 0;
	uint32_t src_width = 0;
	uint32_t src_height = 0;
	uint32_t dst_width = 0;
	uint32_t dst_height = 0;
	uint8_t table_id = 0;

	/* store HDMI h start / v start */
	int h_start = 0;
	int v_start_odd = 0;
	int v_start_even = 0;

	struct fmt_active_info dispfmt_active_info = {0};  /*0A0/0A4/0A8 */
	struct fmt_active_info vdoutfmt_active_info = {0}; /*3A0/3A4/3A8 */
	struct fmt_hd_scl_info info = {0}; /*070/074/078/07C */
	//struct DISP_PATH_LOCATION_INFO location_info;

	mutex_lock(&video_layer[vdp_id].sync_lock);

	dsd_en = vdp_check_dsd_available(vdp_id, config_info);

	video_layer[vdp_id].enable = config_info->enable;

	/*calculate the h factor for dispfmt */
	src_width = config_info->cur_fb_info.src_region.width;
	src_height = config_info->cur_fb_info.src_region.height;
	dst_width = config_info->cur_fb_info.out_region.width;
	dst_height = config_info->cur_fb_info.out_region.height;
	h_factor = src_width * DISPFMT_H_FACTOR / dst_width;

	/*get the active zone information */
	if (vdp_id == 0)
		table_id = DISP_PATH_M_VDO;
	else
		table_id = DISP_PATH_S_VDO;

	disp_path_get_active_zone(table_id, disp_common_info.resolution->res_mode,
				  &h_start, &v_start_odd, &v_start_even);

	/* fill DISPFMT active zone */
	if (src_width > dst_width) { /* use hdown scale or dsd */
		/* in scale down case, the dispfmt zoom is for input buffer
		 ** v _begin & end is special, need add display y offset.
		 */
		dispfmt_active_info.h_begine = h_start;
		dispfmt_active_info.h_end =
			dispfmt_active_info.h_begine + src_width - 1;
		dispfmt_active_info.v_even_begine =
			v_start_even + config_info->cur_fb_info.out_region.y;
		dispfmt_active_info.v_even_end =
			dispfmt_active_info.v_even_begine + dst_height - 1;
		dispfmt_active_info.v_odd_begine =
			dispfmt_active_info.v_even_begine;
		dispfmt_active_info.v_odd_end = dispfmt_active_info.v_even_end;
	} else { /* no hdown scale or dsd */
		/* in scale up case, the dispfmt zoom is for display area */
		dispfmt_active_info.h_begine =
			h_start + config_info->cur_fb_info.out_region.x;
		dispfmt_active_info.h_end =
			dispfmt_active_info.h_begine + dst_width - 1;
		dispfmt_active_info.v_even_begine =
			v_start_even + config_info->cur_fb_info.out_region.y;
		dispfmt_active_info.v_even_end =
			dispfmt_active_info.v_even_begine + dst_height - 1;
		dispfmt_active_info.v_odd_begine =
			dispfmt_active_info.v_even_begine;
		dispfmt_active_info.v_odd_end = dispfmt_active_info.v_even_end;
	}

	/* fild VDOUT FMT active zone: display area */
	vdoutfmt_active_info.h_begine =
		h_start + config_info->cur_fb_info.out_region.x;
	vdoutfmt_active_info.h_end =
		vdoutfmt_active_info.h_begine + dst_width - 1;
	vdoutfmt_active_info.v_even_begine =
		v_start_even + config_info->cur_fb_info.out_region.y;
	vdoutfmt_active_info.v_even_end =
		vdoutfmt_active_info.v_even_begine + dst_height - 1;
	vdoutfmt_active_info.v_odd_begine = vdoutfmt_active_info.v_even_begine;
	vdoutfmt_active_info.v_odd_end = vdoutfmt_active_info.v_even_end;

	/*get the down scaler information */
	if (src_width > dst_width) {
		info.hd_scl_on = true;
		info.in_x_pos = h_start;
		info.out_x_offset = config_info->cur_fb_info.out_region.x;
		info.out_y_odd_pos =
			v_start_odd + config_info->cur_fb_info.out_region.y;
		info.out_y_odd_pos_e = info.out_y_odd_pos + dst_height;
		info.out_y_even_pos = info.out_y_odd_pos;
		info.out_y_even_pos_e = info.out_y_odd_pos_e;
		info.res = disp_common_info.resolution->res_mode;
		info.src_w = src_width;
		info.out_w = dst_width;
	} else
		info.hd_scl_on = false;

	if (info.hd_scl_on)
		fmt_hal_set_pixel_factor(vdp_id, src_width, 0x1000, 1);
	else {
		fmt_hal_set_pixel_factor(vdp_id, src_width, h_factor, 1);
		fmt_hal_h_down_disable(vdp_id);
	}

	//disable set dispfmt active here,and use vdp_config_fefifo_dispmix
	//fmt_hal_set_active_zone(vdp_id, &dispfmt_active_info);

	/* dispfmt 70/74/78 register setting
	 ** and adjust vdout active zone
	 */
	if ((info.hd_scl_on) && (dsd_en == 0)) {
		status = fmt_hal_h_down_enable(vdp_id, &info);
		if (status == FMT_OK) {
			vdoutfmt_active_info.h_begine = info.vdout_out_h_start;
			vdoutfmt_active_info.h_end = info.vdout_out_h_end;
		} else {
			fmt_hal_set_pixel_factor(vdp_id, src_width, h_factor,
						 1);
			info.hd_scl_on = false;
			DISP_LOG_E("get fmt h_down info fail\n");
		}
	}

	/* adjust vdout active zone sepecial for 2160P */
	if (vdp_check_2160p_timing(disp_common_info.resolution->res_mode)) {
		/* 2160P normal & scale up case */
		vdoutfmt_active_info.h_begine += 0x62;
		vdoutfmt_active_info.h_end += 0x62;

		/* 2160P scale down case */
		if ((info.hd_scl_on) && (dsd_en == 0) &&
		    (vdp_id == DISP_FMT_MAIN)) {
			vdoutfmt_active_info.h_begine += 0x3;
			vdoutfmt_active_info.h_end += 0x3;
		}
	}

	/* set vdout active zone */
	fmt_hal_set_active_zone((vdp_id == DISP_FMT_MAIN) ? VDOUT_FMT
							  : VDOUT_FMT_SUB,
				&vdoutfmt_active_info);

	if (dsd_en == 1) {
		fmt_dsd_scl_info.dsd_case = vdp_set_dsd_case(
			config_info->cur_fb_info.src_region.width,
			config_info->cur_fb_info.src_region.height);
		fmt_dsd_scl_info.fg_dsd_scl_on = true;
		fmt_dsd_scl_info.src_x = config_info->cur_fb_info.src_region.x;
		fmt_dsd_scl_info.src_y = config_info->cur_fb_info.src_region.y;
		fmt_dsd_scl_info.src_w =
			config_info->cur_fb_info.src_region.width;
		fmt_dsd_scl_info.src_h =
			config_info->cur_fb_info.src_region.height;
		fmt_dsd_scl_info.out_x = config_info->cur_fb_info.out_region.x;
		fmt_dsd_scl_info.out_y = config_info->cur_fb_info.out_region.y;
		fmt_dsd_scl_info.out_w =
			config_info->cur_fb_info.out_region.width;
		fmt_dsd_scl_info.out_h =
			config_info->cur_fb_info.out_region.height;
		fmt_dsd_scl_info.original_active_start = h_start;
		fmt_dsd_scl_info.active_start = vdoutfmt_active_info.h_begine;
		fmt_dsd_scl_info.active_end = vdoutfmt_active_info.h_end;
		fmt_hal_set_active_zone(vdp_id, &vdoutfmt_active_info);
		fmt_hal_dsd_enable(vdp_id, &fmt_dsd_scl_info);
		/*
		if ((vdp_id == 0) && (dolby_path_enable == 0))
			disp_path_set_dsd_delay(
				DISP_PATH_MVDO_OUT,
				disp_common_info.resolution->res_mode);
		else if (vdp_id == 1)
			disp_path_set_dsd_delay(
				DISP_PATH_SVDO_OUT,
				disp_common_info.resolution->res_mode);
		*/
		fmt_hal_dsd_set_mode(vdp_id, fmt_dsd_scl_info.dsd_case,
				     disp_common_info.resolution->htotal,
				     disp_common_info.resolution->vtotal);
		vdp_select_dsd_pll(vdp_id, fmt_dsd_scl_info.dsd_case);
		vdout_sys_hal_select_dsd_clk(true);
	} else
		vdout_sys_hal_select_dsd_clk(false);

	if (vdp_id == MAIN_VIDEO_INDEX) {
		vdp_config_fefifo_dispmix(MAIN_VIDEO_INDEX,
			config_info->cur_fb_info.is_dovi,
			config_info->cur_fb_info.out_region.x,
			config_info->cur_fb_info.out_region.y,
			dst_width, dst_height,
			dispfmt_active_info, info);
	}
	else if (vdp_id == SUB_VIDEO_INDEX) {
		vdp_config_fefifo_dispmix(SUB_VIDEO_INDEX,
			config_info->cur_fb_info.is_dovi,
			config_info->cur_fb_info.out_region.x,
			config_info->cur_fb_info.out_region.y,
			dst_width, dst_height,
			dispfmt_active_info, info);
	}
	mutex_unlock(&video_layer[vdp_id].sync_lock);
}

static void vdp_disable_display_path(struct video_layer_info *layer_info)
{
	struct vdp_hal_config_info config_info;
	uint32_t *reg_addr = (uint32_t *)(DISPSYS_BASE + 8);
	uint32_t value = *reg_addr;
	struct DISP_PATH_LAYER_INFO path_layer_info = {0};

	DISP_LOG_N("%s\n", __func__);

	layer_info->last_pts = 0xffffffff;

	memset(&layer_info->src_rgn, 0, sizeof(struct mtk_disp_range));
	memset(&layer_info->tgt_rgn, 0, sizeof(struct mtk_disp_range));
	layer_info->enable = false;
	layer_info->src_phy_addr = 0;
	layer_info->buffer_size = 0;
	layer_info->hdr10_type = HDR10_TYPE_NONE;
	layer_info->videobuf = NULL;
	layer_info->dsd_en = false;

	/* disable VDO */
	memset(&config_info, 0, sizeof(struct vdp_hal_config_info));
	vdp_hal_config(layer_info->layer_id, &config_info);
	/* disable disp_fmt. */
	fmt_hal_enable(layer_info->layer_id, false);

	disp_fefifo_stop(layer_info->layer_id);

	if (layer_info->layer_id == 0) {
		disp_path_set_hw_path(DISP_PATH_M_VDO, false);
		disp_path_set_hw_path(DISP_PATH_M_VDO_OUT, false);

		if (layer_info->secure_en) {
			layer_info->secure_en = false;
			layer_info->secure2normal = false;
			disp_vdp_sec_deinit(layer_info->layer_id);
		}

		fmt_hal_clock_on_off(DISP_FMT_MAIN, false);
		disp_clock_enable(DISP_CLK_VDO3, false);
		vdout_sys_hal_select_dsd_clk(false);
		disp_clock_smi_larb_en(DISP_SMI_LARB5, false);
		disp_clock_smi_larb_en(DISP_SMI_LARB6, false);
		path_layer_info.type = DISP_MAIN_VDO;
		path_layer_info.enable = false;
		disp_mix_hal_layer_control(&path_layer_info);
	} else if (layer_info->layer_id == 1) {
		disp_path_set_hw_path(DISP_PATH_S_VDO, false);
		disp_path_set_hw_path(DISP_PATH_S_VDO_OUT, false);

		if (layer_info->secure_en) {
			layer_info->secure_en = false;
			layer_info->secure2normal = false;
			disp_vdp_sec_deinit(layer_info->layer_id);
		}

		disp_clock_enable(DISP_CLK_VDO4, false);
		fmt_hal_clock_on_off(DISP_FMT_SUB, false);
		vdout_sys_hal_select_dsd_clk(false);
		disp_clock_smi_larb_en(DISP_SMI_LARB5, false);
		disp_clock_smi_larb_en(DISP_SMI_LARB6, false);
		/* if stop sub video, need to set vdout_fmt not mix sub plane.
		 */
		/*fmt_hal_not_mix_plane(FMT_HW_PLANE_2);*/

		path_layer_info.type = DISP_SUB_VDO;
		path_layer_info.enable = false;
		disp_mix_hal_layer_control(&path_layer_info);

	}

	/* reset VDP: write 0 for reset, when stopping video. */
	/* write 0x15000008[0] = 0 for layer 0 */
	/* write 0x15000008[7] = 0 for layer 1 */
	if (layer_info->layer_id == 0) {
		value &= 0xFFFFFFFE;
		//*(reg_addr) = value;
	} else if (layer_info->layer_id == 1) {
		value &= 0xFFFFFF7F;
		//*(reg_addr) = value;
	} else
		DISP_LOG_E("invalid layer id:%d\n", layer_info->layer_id);

	layer_info->state = VDP_LAYER_IDLE;
	wake_up(&layer_info->wait_queue);
}

/* for start */
void vdp_disable_active_zone(struct video_layer_info *layer_info)
{
	struct fmt_active_info active_info;
	uint32_t *reg_addr = (uint32_t *)(DISPSYS_BASE + 8);
	uint32_t value = *reg_addr;

	/* only 1 display buffer is in	buffer list. */
	memset(&active_info, 0, sizeof(struct fmt_active_info));
	fmt_hal_set_active_zone(layer_info->layer_id == 0 ? VDOUT_FMT
							  : VDOUT_FMT_SUB,
				&active_info);
	fmt_hal_shadow_update();
	fmt_hal_set_active_zone(layer_info->layer_id == 0 ? DISP_FMT_MAIN
							  : DISP_FMT_SUB,
				&active_info);

	/* reset VDP: write 1 for de-reset when start playing. */
	/* write 0x15000008[0] = [1] for layer 0 */
	/* write 0x15000008[7] = [1] for layer 1 */
	if (layer_info->layer_id == 0) {
		value |= 0x00000001;
		*(reg_addr) = value;
	} else if (layer_info->layer_id == 1) {
		value |= 0x00000080;
		*(reg_addr) = value;
	} else
		DISP_LOG_E("%s invalid layer id:%d\n", __func__,
			   layer_info->layer_id);
	value |= 0x00000001;
	*(reg_addr) = value;
}

/* for stop */
void vdp_disable_vdout_active_zone(struct video_layer_info *layer_info)
{
	struct fmt_active_info active_info;
	struct DISP_PATH_LAYER_INFO path_layer_info = {0};

	/* only 1 display buffer is in	buffer list. */
	/*set dispfmt active zone to 0*/
	memset(&active_info, 0, sizeof(struct fmt_active_info));

	if (layer_info->secure_en)
		; // disp_vdp_sec_disable_active_zone(layer_info->layer_id);
		  // 8696 review
	else
		fmt_hal_set_active_zone(layer_info->layer_id == 0
						? DISP_FMT_MAIN
						: DISP_FMT_SUB,
					&active_info);

	/*set disp_mix active zone to 0*/
	path_layer_info.src_width = 0;
	path_layer_info.src_height = 0;
	path_layer_info.x_offset = 0;
	path_layer_info.y_offset = 0;
	if (layer_info->layer_id == 0)
		path_layer_info.type = DISP_MAIN_VDO;
	else
		path_layer_info.type = DISP_SUB_VDO;
	path_layer_info.enable = false;
	disp_mix_hal_layer_control(&path_layer_info);

	/*set vdout fmt active zone to 0, maybe no need in 8696*/
	fmt_hal_set_active_zone(layer_info->layer_id == 0 ? VDOUT_FMT
							  : VDOUT_FMT_SUB,
				&active_info);
	fmt_hal_shadow_update();
}

#define MAX_NUM_WINDOWS 3

int vdp_stop_disable_hw(unsigned int layer_id)
{
	struct video_layer_info *layer_info;

	layer_info = &video_layer[layer_id];

	DISP_LOG_N("STOP: disable active zone\n");
	vdp_disable_vdout_active_zone(layer_info);

	DISP_LOG_N("STOP: layer %d disable ufo and vdo\n", layer_id);
	if (layer_info->secure_en)
		disp_vdp_sec_stop_hw(layer_info->layer_id);
	else {
		vdp_hal_set_enable(layer_info->layer_id, false);
		vdp_hal_isr((layer_info->layer_id == 0) ? true : false,
			    (layer_info->layer_id == 1) ? true : false);
	}

	return 0;
}

int vdp_metadata_async(void)
{
	bool tvAsync = hdr10_plus_get_frame_delay_flag();

	if (tvAsync && (video_layer[0].hdr10_type == HDR10_TYPE_PLUS) &&
		(!video_layer[1].layer_start) && video_layer[0].videobuf) {
		mutex_lock(&video_layer[0].sync_lock);
		disp_hdr_config_video_info(video_layer[0].videobuf,
			video_layer[1].layer_start);
		mutex_unlock(&video_layer[0].sync_lock);
		return 1;
	}
	return 0;
}


static int vdp_get_buffer_metadata_timer(struct video_buffer_info *buf)
{
	ktime_get_raw_ts64(&buf->ts);
	buf->timestap1 = (unsigned long)buf->ts.tv_sec * 1000 +
		buf->ts.tv_nsec / 1000000;

	return 0;
}

static enum METADATA_SET medataConfigTactics(uint32_t id,
	struct video_buffer_info *curbuf,
	struct video_buffer_info *nextbuf)
{
	bool tvAsync = hdr10_plus_get_frame_delay_flag();
	enum METADATA_SET result = METADATA_NORMAL;
	struct video_layer_info *layer_info = &video_layer[id];

	if (curbuf->is_metadata_async) {
		if (tvAsync && (curbuf->hdr10_type == HDR10_TYPE_PLUS)
			&& (!video_layer[1].layer_start)) {
			if (layer_info->videobuf)
				result = METADATA_NORMAL;
			else {
				DISP_LOG_N(
					"layer0, only set md. tv:%d,src:%d\n",
					tvAsync, curbuf->is_metadata_async);
				result = METADATA_ONLY;
			}
			disp_hdr_config_video_info(curbuf,
				video_layer[1].layer_start);
		} else {
			if (layer_info->videobuf) {
				disp_hdr_config_video_info(
					layer_info->videobuf,
					video_layer[1].layer_start);
				result = METADATA_NORMAL;
			} else {
				DISP_LOG_N(
					"layer0, do nothing. tv:%d,src:%d\n",
					tvAsync, curbuf->is_metadata_async);
				result = METADATA_STORY; /*this need pass*/
			}
		}
		layer_info->videobuf = curbuf;
		return result;
	}

	result = METADATA_NORMAL;
	layer_info->videobuf = curbuf;
	if ((id == 0) && tvAsync && (curbuf->hdr10_type == HDR10_TYPE_PLUS)
		&& (!video_layer[1].layer_start)) {
		DISP_LOG_N("enter %s flow %lld\n", __func__, curbuf->pts);
		curbuf->is_enter_async_mode = true;
		if (layer_info->hdr10_type != HDR10_TYPE_PLUS) {
			DISP_LOG_N("first hdr10+, pretype:%d pts:%lld\n",
				layer_info->hdr10_type, curbuf->pts);
			disp_hdr_config_video_info(curbuf, 0);
			vdp_get_buffer_metadata_timer(curbuf);
			curbuf->metadata_already_set = true;
			layer_info->display_duration +=
				layer_info->vsync_duration;
			result = METADATA_ONLY;
		} else {
			if (layer_info->display_duration >=
			    layer_info->vsync_duration) {
				if ((layer_info->display_duration <
				     layer_info->vsync_duration * 2) &&
				    (!curbuf->metadata_already_set)) {
					disp_hdr_config_video_info(curbuf, 0);
					vdp_get_buffer_metadata_timer(curbuf);
					DISP_LOG_N("set metadt pts %lld\n",
						   curbuf->pts);
					curbuf->metadata_already_set = true;
					result = METADATA_ONLY;
				}
			} else {
				if (!curbuf->metadata_already_set) {
					DISP_LOG_N("err,wait vsync buf:%lld\n",
						curbuf->pts);
					disp_hdr_config_video_info(curbuf, 0);
					vdp_get_buffer_metadata_timer(curbuf);
					curbuf->metadata_already_set = true;
					layer_info->display_duration +=
						layer_info->vsync_duration;
					result = METADATA_ONLY;
				} else if (layer_info->display_duration +
						   curbuf->source_duration <
					   layer_info->vsync_duration * 2) {
					if (nextbuf && (nextbuf->hdr10_type ==
							HDR10_TYPE_PLUS)) {
						DISP_LOG_N(
							"pts:%lld->pts:%lld\n",
							curbuf->pts,
							nextbuf->pts);
						nextbuf->is_enter_async_mode =
							true;
						disp_hdr_config_video_info(
							nextbuf, 0);
						vdp_get_buffer_metadata_timer(
							nextbuf);
						nextbuf->metadata_already_set =
							true;
						layer_info->videobuf = nextbuf;
					} else
						DISP_LOG_N(
							"err,no next buffer\n");
				}
			}
		}
		layer_info->hdr10_type = curbuf->hdr10_type;
		return result;
	}

	if (layer_info->display_duration < layer_info->vsync_duration)
		disp_hdr_config_video_info(curbuf, video_layer[1].layer_start);
	return METADATA_NORMAL;
}

void vdp_trigger_metadata_config(uint32_t id, struct video_buffer_info *curbuf,
	struct video_buffer_info *nextbuf)
{
	struct video_layer_info *layer_info = &video_layer[id];

	if (curbuf == NULL || nextbuf == NULL) {
		DISP_LOG_N("buf is null\n");
		return;
	}
	layer_info->display_duration = 0;
	layer_info->vsync_duration = 1500;

	medataConfigTactics(id, curbuf, nextbuf);
	DISP_LOG_N("trigger metadata done\n");
}

static bool vdp_process_buffer(struct video_buffer_info **buf,
	struct video_buffer_info **nextbuf,
	unsigned int vdp_id)
{
	struct video_buffer_info *temp = NULL;
	bool find_next = false;
	struct video_buffer_info *curbuf = NULL;
	struct video_layer_info *layer_info;
	struct DISP_PATH_LAYER_INFO path_layer_info = {0};


	if (vdp_id > 1) {
		DISP_LOG_E("invalid video id%d\n", vdp_id);
		return false;
	}
	layer_info = &video_layer[vdp_id];

	/* buffer list is not empty, get one frame from
	 * buffer list
	 */
	list_for_each_entry_safe(curbuf, temp,
			&layer_info->buf_list,
			list) {
		if (layer_info->state ==
				  VDP_LAYER_STOPPING) {
			layer_info->state =
				VDP_LAYER_STOPPED;
			find_next = false;

			/*set disp_mix active zone to 0
			 * set vdo not mix
			 */
			path_layer_info.src_width = 0;
			path_layer_info.src_height = 0;
			path_layer_info.x_offset = 0;
			path_layer_info.y_offset = 0;
			if (layer_info->layer_id == 0)
				path_layer_info.type = DISP_MAIN_VDO;
			else
				path_layer_info.type = DISP_SUB_VDO;
			path_layer_info.enable = false;
			disp_mix_hal_layer_control(&path_layer_info);

			DISP_LOG_I(
				"STOPPING:buf:%d tl%d,rel%d, tl_v%d\n",
				curbuf->current_fence_index,
				layer_info->timeline_idx,
				layer_info->release_idx,
				layer_info->timeline->value);
			break;
		} else if (layer_info->state ==
				   VDP_LAYER_STOPPED) {
			vdp_release_buffer(layer_info->layer_id);

			layer_info->timeline_idx =
				curbuf->current_fence_index;
			find_next = false;

			DISP_LOG_I(
				"STOPPED:buf:%d tl%d,rel%d, tl_v%d\n",
					curbuf->current_fence_index,
					layer_info->timeline_idx,
					layer_info->release_idx,
					layer_info->timeline->value);
			break;
		} else if (
				curbuf->current_fence_index ==
				(layer_info->timeline_idx +
				 1)) {
			*buf = curbuf;
			find_next = true;
			vdp_state_switch(
				VDP_LAYER_START,
				VDP_LAYER_RUNNING,
				layer_info);
			vdp_printf(
				VDP_AVSYNC_LOG,
				"Get buf cur_idx|%d, tl_idx|%d %d %d\n",
				curbuf->current_fence_index,
				layer_info->timeline_idx,
				_u4VdpISRCount,
				_u4VdpISRCount2);
			if (curbuf->hdr10_type !=
				HDR10_TYPE_PLUS)
				break;
		} else if (
				curbuf->current_fence_index ==
				(layer_info->timeline_idx +
				 2)) {
			*nextbuf = curbuf;
			vdp_printf(
				VDP_AVSYNC_LOG,
				"Get next cur_idx|%d, tl_idx|%d\n",
				curbuf->current_fence_index,
				layer_info->timeline_idx);
			break;
		}
	}
	return find_next;
}

/*wake up condition :
 *1. vsync active start , not including resolution change duration
 *2. first frame inuput by HWC after resolution change
 */
static int vdp_routine(void *data)
{
	int ret = 0;
	struct video_buffer_info *buf = NULL;
	struct video_buffer_info *nextbuf = NULL;
	struct video_layer_info *layer_info;
	bool find_next = false;
	unsigned int i;
	unsigned int duration;
	struct dma_fence *sync_fence = NULL;
	struct vdp_hal_config_info config_info;
	bool is_Y_C_independent = false;
	int dsd_en = 0;
	int h_start = 0;
	int v_start_odd = 0;
	int v_start_even = 0;
	uint32_t thread_id = *(uint32_t *)data; /* layer id */
	uint32_t video_stream_number = 1;
	unsigned long timestap = 0;
	struct timespec64 ts;
	enum METADATA_SET metadata_result = METADATA_NORMAL;
	static unsigned int layer_show_count[2];
	static uint64_t pre_pts[2];
	int32_t display_vsync_count = 0;
	struct disp_hw *vdp_drv = disp_vdp_get_drv();
	uint8_t table_id = 0;
	struct DISP_PATH_LAYER_INFO path_layer_info = {0};

/* just for dolby idk. Guarantee osd not to cover video even if video is full
 * screen.
 */
#if 0
	int params_count = 7;
	const char *vdp_show[7] = {"vdp.cli", "0", "0", "0",
		"1920", "1080", "1920"};
	const char *vdp_show_4k[7] = {"vdp.cli", "0", "0", "0",
		"3840", "2160", "3840"};
	const char *vdp_show_720p[7] = {"vdp.cli", "0", "0", "0",
		"1280", "720", "1280"};
	const char *vdp_show_480p[7] = {"vdp.cli", "0", "0", "0",
		"720", "480", "720"};
#endif

	while (1) {
		if (thread_id == 0) {

			vdp_printf(VDP_CAPTURE_LOG,
				   "[vdp%d] before wait event %d %d, %d %d",
				   thread_id,
				   _u4VdpISRCount,
				   _u4VdpISRCount2,
				   u4VsyncLineTest,
				   VDOUT_LINE_CNT);

			ret = wait_event_interruptible(disp_vdp_wq0,
					atomic_read(&gWakeupVdpSwThread0));
			if (ret < 0) {
				vdp_printf(VDP_CAPTURE_LOG,
					   "[vdp%d] continue wait event %d %d",
					   thread_id,
					   _u4VdpISRCount,
					   _u4VdpISRCount2);
				continue;
			}

			vdp_printf(VDP_CAPTURE_LOG,
				   "[vdp%d] after wait event %d %d, %d %d",
				   thread_id,
				   _u4VdpISRCount,
				   _u4VdpISRCount2,
				   u4VsyncLineTest,
				   VDOUT_LINE_CNT);

			atomic_set(&gWakeupVdpSwThread0, 0);
			display_vsync_count++;
			_u4VdpISRCount++;
		} else if (thread_id == 1) {
			ret = wait_event_interruptible(disp_vdp_wq1,
					atomic_read(&gWakeupVdpSwThread1));
			if (ret < 0)
				continue;
			atomic_set(&gWakeupVdpSwThread1, 0);
		} else {
			DISP_LOG_E("invalid thread id %d\n", thread_id);
			#ifdef DISP_GCE_SUPPORT
			if (video_layer[thread_id].new_trigger) {
				vdp_wakeup_GCE(thread_id);
				video_layer[thread_id].new_trigger = false;
				}
			#endif
			continue;
		}

		REC_VDP_LINE(thread_id, VDP_ROUTINE_START);

		for (i = thread_id; i < (thread_id + video_stream_number);
		     i++) {
			/*get buffer from input buffer list */
			layer_info = &video_layer[i];

			disp_hdr_vsync_handle(i, display_vsync_count);
			//for dovi idk
			if (dovi_idk_dump && (i == 0)) {
				#if 0
				if ((idk_close_area == 1) ||
					(idk_vdo_en > 1))
					vdp_cli_get()->target_area.enable =
						false;
				else if (dovi_res.width == 3840)
					_vdp_cli_debug_set_disp_area(
					params_count, vdp_show_4k);
				else if (dovi_res.width == 1920)
					_vdp_cli_debug_set_disp_area(
					params_count, vdp_show);
				else if (dovi_res.width == 1280)
					_vdp_cli_debug_set_disp_area(
					params_count, vdp_show_720p);
				else if (dovi_res.width == 720)
					_vdp_cli_debug_set_disp_area(
					params_count, vdp_show_480p);
				#endif

				if (idk_dump_vsync_cnt >= 0)
					idk_dump_vsync_cnt++;

				if (idk_dump_vsync_cnt ==
					(dovi_idk_disp_cnt/2 + 10))
					disp_dovi_idk_dump_frame();
			}

			/* FRC control
			 ** whether the current display buffer display time is up
			 */
			if (vdp_cli_get()->enable_pts_debug)
				layer_show_count[i]++;

/* just for dovi idk => run this code */
#if 1
			if (dovi_idk_dump && idk_stop_num &&
				(idk_now_num[i] >= idk_stop_num))
				continue;
			if (dovi_idk_dump && idk_stop_frame_num > 0) {
				if (hdr_frame_num >= idk_stop_frame_num)
					continue;
			}
			if (dovi_idk_dump && (layer_info->display_duration >=
				layer_info->vsync_duration))
				continue;

			if (dovi_idk_dump && (i == 0))
				idk_dump_vsync_cnt = 0;
#endif
			mutex_lock(&(layer_info->sync_lock));

			if (i == 0 && layer_info->state == VDP_LAYER_RUNNING)
				vdp_printf(VDP_CAPTURE_LOG,
					   "[vdp%d] process buf %d %d, %d %d",
					   i,
					   _u4VdpISRCount,
					   _u4VdpISRCount2,
					   u4VsyncLineTest,
					   VDOUT_LINE_CNT);

			buf = NULL;
			nextbuf = NULL;
			find_next = false;

			if (!list_empty(&layer_info->buf_list))
				find_next = vdp_process_buffer(&buf,&nextbuf,i);
			else if (layer_info->state == VDP_LAYER_STOPPING) {
				layer_info->state =
					VDP_LAYER_STOPPED;
				/*set disp_mix active zone to 0
				 * set vdo not mix
				 */
				path_layer_info.src_width = 0;
				path_layer_info.src_height = 0;
				path_layer_info.x_offset = 0;
				path_layer_info.y_offset = 0;
				if (layer_info->layer_id == 0)
					path_layer_info.type = DISP_MAIN_VDO;
				else
					path_layer_info.type = DISP_SUB_VDO;
				path_layer_info.enable = false;
				disp_mix_hal_layer_control(&path_layer_info);

				DISP_LOG_I("stopping with no buffer\n");
			} else if (layer_info->state == VDP_LAYER_STOPPED) {
				/* current buffer list is empty. */
				pre_pts[i] = 0;
				vdp_drv->drv_call(DISP_CMD_VDP_STOP,
					&(layer_info->layer_id));

				if (is_hd_resolution())
					disp_sys_hal_set_hdr_drop_shadow(layer_info->layer_id,
					false);
				else
					disp_sys_hal_set_hdr_drop(layer_info->layer_id,
					false);

				//here stop video qms
				#ifdef DISP_QMS_SUPPORT
				if (layer_info->qms_exit) {
					disp_queue_vrr_mode_handle(false, 0);
					layer_info->qms_exit = false;
					DISP_LOG_I("vdp stop qms done\n");
				}
				#endif

				vdp_disable_display_path(layer_info);
				DISP_LOG_N("no buffer now, clock off done\n");
			}

			if (!find_next &&
				layer_info->state == VDP_LAYER_RUNNING
				&& video_layer[i].layer_start
				&& video_layer[1 - i].layer_start)
				disp_hdr_config_video_non(i);

			mutex_unlock(&(layer_info->sync_lock));

#if !(VIDEO_USE_HW_SHADOW && VIDEO_REDUCE_BUFFER)
			if (!find_next) {
				if (layer_info->state == VDP_LAYER_RUNNING) {
					mutex_lock(&(layer_info->sync_lock));
					vdp_release_buffer(i);
					mutex_unlock(&(layer_info->sync_lock));
					}
			}
#endif
			if ((!find_next) || (buf == NULL)) {
				#ifdef DISP_GCE_SUPPORT
				if (layer_info->new_trigger) {
					vdp_wakeup_GCE(layer_info->layer_id);
					layer_info->new_trigger = false;
					}
				#endif

				if (thread_id == 0 &&
				    layer_info->state == VDP_LAYER_RUNNING)
					vdp_printf(VDP_AVSYNC_LOG,
						   "[vdp%d] list empty %d %d",
						   thread_id,
						   _u4VdpISRCount,
						   _u4VdpISRCount2);

				continue;
			}

			if (dovi_idk_dump &&
				(buf->current_fence_index ==
				(layer_info->timeline_idx + 1))) {
				idk_now_num[i]++;
				if ((buf->pts == 0 &&
				idk_vdo_pts[i] > buf->pts) ||
				(buf->pts > 0 &&
				idk_vdo_pts[i] == buf->pts &&
				!idk_no_drop)) {
					idk_vdo_pts[i] = 0;
					dovi_idk_settings(0);
			vdp_cli_get()->target_area.enable = false;
					vdp_printf(
				VDP_AVSYNC_LOG,
				"vdp layer stop\n");
					continue;
				} else
					idk_vdo_pts[i] = buf->pts;
			}

#if 1

			/* find a valid buffer */
			if ((buf->ion_fd & 0xFFFF000) > 0)
				is_Y_C_independent = true;
			if (buf->is_interlace)
				duration = INTERLACE_PLAYBACK_DUARATION;
			else
				duration = NORMAL_PLAYBACK_DURATION;

#if !(VIDEO_USE_HW_SHADOW && VIDEO_REDUCE_BUFFER)
			if (layer_info->timeline_idx >
			    layer_info->release_idx) {
				if ((layer_info->timeline_idx -
				     layer_info->release_idx) > duration) {
					/*free the display */
					mutex_lock(&(layer_info->sync_lock));
					vdp_release_buffer(i);
					mutex_unlock(&(layer_info->sync_lock));
					if (layer_info->state ==
					    VDP_LAYER_STOPPED) {
						layer_info->timeline_idx++;
						DISP_LOG_N(
	"releasing1...tl_idx=%d,rel_idx=%d, tl_value=%d\n",
						layer_info->timeline_idx,
						layer_info->release_idx,
						layer_info->timeline->value);
					#ifdef DISP_GCE_SUPPORT
					if (layer_info->new_trigger) {
						vdp_wakeup_GCE(thread_id);
						layer_info->new_trigger = false;
					}
					#endif
					continue;
					}
				} else if (layer_info->state ==
					   VDP_LAYER_STOPPED) {
					/*signal the fence */
					if (layer_info->timeline != NULL)
						timeline_inc(
						layer_info->timeline, 1);
					layer_info->release_idx++;
					/*free ion handle */
					disp_hw_mgr_dma_buffer_release(&buf->mmu1);
					if (is_Y_C_independent)
						disp_hw_mgr_dma_buffer_release(&buf->mmu2);

					mutex_lock(&layer_info->sync_lock);
					list_del_init(&buf->list);
					mutex_unlock(&layer_info->sync_lock);
					release_buf_info(&buf->list, i);
					DISP_LOG_N(
						"releasing2...tl_idx=%d, rel_idx=%d, tl_value=%d\n",
						layer_info->timeline_idx,
						layer_info->release_idx,
						layer_info->timeline->value);
				}
			}
#endif
			//fist time enter qms or fps change in qms mode
			//if ret -1 means video qms should not enable
			#ifdef DISP_QMS_SUPPORT
			if (layer_info->qms_en && buf->qms_en) {
				//fps change need reconfig qms vrr info
				if (buf->fps != layer_info->last_fps) {
					ret = disp_queue_vrr_mode_handle(true, buf->fps);
					if (!ret) {
						layer_info->last_fps = buf->fps;
						DISP_LOG_I("QMS enable at fps %d\n", buf->fps);
					} else {
						disp_queue_vrr_mode_handle(0, buf->fps);
						DISP_LOG_I("QMS shall not enable %d\n", buf->fps);
					}
				} else {
					//get vrr info for adjust fraction mvrr
					// only adjust when qms vrr stable
					//if (vdp_en_mvrr_frac)
					//	disp_vrr_set_fraction_mvrr();
				}
			}
			#endif

			config_info.cur_fb_info.src_region.width =
				buf->crop.width;
			config_info.cur_fb_info.src_region.height =
				buf->crop.height;
			config_info.cur_fb_info.out_region.width =
				buf->tgt.width;
			config_info.cur_fb_info.out_region.height =
				buf->tgt.height;
			dsd_en = vdp_check_dsd_available(i, &config_info);

			mutex_lock(&layer_info->sync_lock);
			metadata_result =
				medataConfigTactics(i, buf, nextbuf);
			mutex_unlock(&layer_info->sync_lock);

			if (metadata_result == METADATA_STORY) {
				DISP_LOG_N(
					"this buffer do nothing, bypass it\n");
				layer_info->timeline_idx++;
				debug_frame_count[layer_info->layer_id]++;
				#ifdef DISP_GCE_SUPPORT
				if (layer_info->new_trigger) {
					vdp_wakeup_GCE(thread_id);
					layer_info->new_trigger = false;
				}
				#endif
				continue;
			} else if (metadata_result == METADATA_ONLY) {
				DISP_LOG_N("only set metadata info\n");
				#ifdef DISP_GCE_SUPPORT
				if (layer_info->new_trigger) {
					vdp_wakeup_GCE(thread_id);
					layer_info->new_trigger = false;
				}
				#endif
				continue;
			}

			if ((pre_pts[i] != buf->pts) &&
			    (layer_info->display_duration >=
			     layer_info->vsync_duration)) {
			    #ifdef DISP_GCE_SUPPORT
				if (layer_info->new_trigger) {
					vdp_wakeup_GCE(thread_id);
					layer_info->new_trigger = false;
				}
				#endif

				vdp_printf(VDP_CAPTURE_LOG,
					   "buf pts %llu %llu dura %d %d %d %d",
					   pre_pts[i],
					   buf->pts,
					   layer_info->display_duration,
					   layer_info->vsync_duration,
					   _u4VdpISRCount,
					   _u4VdpISRCount2);
				continue;
			}
#endif

/*wait for the buffer write operation done */
			if (buf->acquire_fence_fd != -1) {
				sync_fence = sync_file_get_fence(
							buf->acquire_fence_fd);
			if (sync_fence != NULL) {
				ret = dma_fence_wait_timeout(
							sync_fence,
							false,
							100);
			if (ret == 0)
				DISP_LOG_E("wait fence error %d\n", ret);
			dma_fence_put(sync_fence);
			}
			}
			debug_frame_count[layer_info->layer_id]++;

			vdp_enable_dispfmt_with_shadow(i);
/*for secure buffer, should switch to TEE */

#if VIDEO_DISPLAY_SECURE_ENABLE
#ifndef DISP_GCE_SUPPORT
			if (buf->secruity_en && !layer_info->secure_en) {
				DISP_LOG_N(
					"switch layer[%d] from normal to secure map addr[0x%08x] size:[%d]\n",
					layer_info->layer_id,
					layer_info->src_phy_addr,
					layer_info->buffer_size);
				disp_vdp_sec_init(layer_info->layer_id,
						  layer_info->src_phy_addr,
						  layer_info->buffer_size);

				layer_info->secure_en = true;
			}

			if (!buf->secruity_en && layer_info->secure_en) {
				if (layer_info->secure2normal) {
					layer_info->secure_en = false;
					layer_info->secure2normal = false;
					disp_vdp_sec_deinit(
						layer_info->layer_id);
					DISP_LOG_N(
						"switch from secure to normal port\n");
				} else {
					layer_info->secure2normal = true;
					DISP_LOG_N(
						"first switch from secure to normal\n");
				}
			}
#endif
#endif

			layer_info->src_phy_addr = buf->src_phy_addr;
			layer_info->buffer_size = buf->buffer_size;

			/* config hw in TEE, use service call */
			/*set the buffer info to hal structure */
			memset(&config_info, 0,
			       sizeof(struct vdp_hal_config_info));
			config_info.vdp_id = i;

			if (dovi_idk_dump)
				layer_info->display_duration +=
					(layer_info->vsync_duration *
					 dovi_idk_disp_cnt);
			else if (pre_pts[i] != buf->pts) {
				if (buf->source_duration)
					layer_info->display_duration +=
						buf->source_duration;
				else
					layer_info->display_duration = 0;
			}


			#ifdef CONFIG_DOVI_SUPPORT
			if (buf->is_dovi && g_dovi_efuse)
				set_fs_index(i, 1);
			else
			#endif
				set_fs_index(i, 0);

			if (dovi_idk_dump && (idk_vdo_en == 2))
				set_fs_index(i, 1);
			/* get the active zone from display path */
			if (i == 0)
				table_id = DISP_PATH_M_VDO;
			else
				table_id = DISP_PATH_S_VDO;
			disp_path_get_active_zone(
				table_id, disp_common_info.resolution->res_mode,
				&h_start, &v_start_odd, &v_start_even);

			/* set the active zone information to vdp hal */
			vdp_hal_config_timing(i, disp_common_info.resolution,
					      h_start, v_start_odd,
					      v_start_even);

			vdp_config_film_grain(i, buf);

			/* fill vdp_hal_config_info according to
			 * video_buffer_info
			 */
			vdp_set_hal_config(buf, NULL, NULL, &config_info);
			#ifdef DISP_GCE_SUPPORT
			memcpy(&cur_fb_addr[i],
				&config_info.cur_fb_info.fb_addr,
				sizeof(struct vdp_hal_fb_addr));
			#endif
#if 1
			/*get the buffer information & set to vdp/fmt sw shadow
			 */
			/* set dispfmt & vdout active zoon and enable vdout. */
			REC_VDP_LINE(thread_id, VDP_FMT_SETTING);

			vdp_update_fmt_setting(i, &config_info);
#endif
#if VIDEO_DISPLAY_SECURE_ENABLE
#ifndef DISP_GCE_SUPPORT
			if ((layer_info->secure_en) &&
				(layer_info->layer_id <= DISP_FMT_SUB)){
				struct dispfmt_setting dispfmt_info = {};
				uint64_t *reg_mode = NULL;
				char *dispfmt_register = NULL;

				dispfmt_register =
					fmt_hal_get_sw_register(
					layer_info->layer_id,
					&dispfmt_info
					.dispfmt_register_setting_size,
					&reg_mode);

				if (dispfmt_register)
					memcpy(
					(char *)(&dispfmt_info.dispfmt_register_setting),
					(char *)dispfmt_register,
					sizeof(uint32_t) * (HAL_DISP_FMT_MAIN_REG_NUM));

				if (reg_mode != NULL)
					dispfmt_info.dispfmt_register_setting_mask =
						*reg_mode;

#ifdef build_fail
				WARN_ON(dispfmt_info.dispfmt_register_setting ==
					0);
#endif
				REC_VDP_LINE(thread_id, VDP_SEC_IN);
				disp_vdp_sec_config(&config_info, &dispfmt_info,
						    osd_enable);

				REC_VDP_LINE(thread_id, VDP_SEC_OUT);
				/* clear dispfmt register mask */
				if (reg_mode != NULL)
					*reg_mode = 0;
			} else
#endif
#endif
				vdp_hal_config(i, &config_info);

			DUMP_VDP_LINE(i, display_vsync_count);

			if (dsd_en == 1) {
/*Update vdo setting && disp_fmt */
#if VIDEO_DISPLAY_SECURE_ENABLE
				if (!layer_info->secure_en)
#endif
					vdp_hal_set_dsd_config(
						i, config_info.cur_fb_info
							   .src_region.height,
						config_info.cur_fb_info
							.out_region.height,
						disp_common_info.resolution
							->height);

				layer_info->dsd_en = true;
			} else
				layer_info->dsd_en = false;

#ifdef DISP_GCE_SUPPORT
			vdp_wakeup_GCE(i);
			layer_info->new_trigger = false;
#else
#if VIDEO_USE_HW_SHADOW
			if (i == 0)
				vdp_hal_isr(!layer_info->secure_en, false);
			else
				vdp_hal_isr(false, !layer_info->secure_en);
#endif
#endif
			layer_info->timeline_idx++;

			if (vdp_cli_get()->enable_pts_debug) {
				ktime_get_raw_ts64(&ts);
				timestap = (unsigned long)ts.tv_sec * 1000 +
					   ts.tv_nsec / 1000000;

				DISP_LOG_E(
					"routine: layer %d, pre_pts %lld, count %d, duration %d, pts %lld, timer %ld\n",
					i, pre_pts[i], layer_show_count[i],
					layer_info->display_duration, buf->pts,
					timestap);

				layer_show_count[i] = 0;
			}
			pre_pts[i] = buf->pts;

			// buf->time_start_display = sched_clock(); 8696 review
			find_next = false;
		}
	}
	return VDP_OK;
}

void dovi_path_enable_by_vs10(void)
{
}

void dovi_path_disable_by_vs10(void)
{
}

