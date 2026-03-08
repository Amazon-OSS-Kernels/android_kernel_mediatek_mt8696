/*
 * Copyright (C) 2017 MediaTek Inc.
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

#define LOG_TAG "HDR_MAIN"

#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <uapi/linux/sched/types.h>

#include "disp_hw_mgr.h"
#include "disp_dovi_main.h"
#include "hdmitx.h"
#include "disp_irq.h"
#include "disp_path.h"
#include "disp_hdr_if.h"
#include "disp_hw_log.h"
#include "disp_vdp_vsync.h"
#include "fmt_hal.h"
#include "fmt_def.h"
#include "vdout_sys_hal.h"
#include "disp_dovi_common_if.h"
#include "disp_hdr_main.h"
#include "disp_cfd_main.h"
#include "disp_hdr_cmd.h"

#define MULBASE 10
#define N 50

static struct task_struct *disp_hdr_thread0, *disp_hdr_thread1;
static struct task_struct *disp_hdr_thread2, *disp_hdr_thread3;
static struct task_struct *disp_hdr_thread4;

static wait_queue_head_t disp_hdr_thread_wq0, disp_hdr_thread_wq1;
static wait_queue_head_t disp_hdr_thread_wq2, disp_hdr_thread_wq3;
static wait_queue_head_t disp_hdr_thread_wq4;

static atomic_t gWakeupHdrThread0, gWakeupHdrThread1;
static atomic_t gWakeupHdrThread2, gWakeupHdrThread3;
static atomic_t gWakeupHdrThread4;

static struct mutex disp_hdr_thread_mutex;

struct mutex disp_hdr_path_mutex;
struct mutex disp_hdr_stop_mutex;
struct mutex disp_hdr_cfg_hdmi_mutex;
uint32_t vdp_start_st[V_G_LAYER_MAX];
uint32_t disp_hdr_event;
uint64_t disp_hdr_thread_cnt;
uint32_t befifo_irq_cnt;
bool hdr_init_done;
#ifdef CONFIG_HDMI_BLACK
bool fg_hdr_deep_suspend;
#endif
uint32_t hdr_res_width;
uint32_t hdr_res_height;
uint32_t force_sdr_output;

void disp_hdr_set_event(uint32_t event)
{
	mutex_lock(&disp_hdr_thread_mutex);
	disp_hdr_event |= event;
	mutex_unlock(&disp_hdr_thread_mutex);
}

void disp_hdr_wakeup_routine(uint32_t thread_id)
{
	switch (thread_id) {
	case 0:
		atomic_set(&gWakeupHdrThread0, 1);
		wake_up(&disp_hdr_thread_wq0);
		break;
	case 1:
		atomic_set(&gWakeupHdrThread1, 1);
		wake_up(&disp_hdr_thread_wq1);
		break;
	case 2:
		atomic_set(&gWakeupHdrThread2, 1);
		wake_up(&disp_hdr_thread_wq2);
		break;
	case 3:
		atomic_set(&gWakeupHdrThread3, 1);
		wake_up(&disp_hdr_thread_wq3);
		break;
	case 4:
		atomic_set(&gWakeupHdrThread4, 1);
		wake_up(&disp_hdr_thread_wq4);
		break;
	}
}

static int disp_hdr_routine0(void *data)
{
	int wait_ret = 0;

	while (!kthread_should_stop()) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq0,
			atomic_read(&gWakeupHdrThread0));
		if (wait_ret)
			hdr_error("wait hdr routine0 error");
		atomic_set(&gWakeupHdrThread0, 0);

		++disp_hdr_thread_cnt;

		if ((disp_hdr_event & HDR_EVENT_VLAYER0_CFG_DONE)
			&& (disp_hdr_event & HDR_EVENT_VLAYER1_CFG_DONE)) {
			disp_hdr_event &= ~HDR_EVENT_VLAYER0_CFG_DONE;
			disp_hdr_event &= ~HDR_EVENT_VLAYER1_CFG_DONE;

			disp_hdr_path_judge();
		}
	}
	return 0;
}

static int disp_hdr_routine1(void *data)
{
	struct video_buffer_info *buf_sub = NULL;
	int wait_ret = 0;

	while (!kthread_should_stop()) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq1,
			atomic_read(&gWakeupHdrThread1));
		if (wait_ret)
			hdr_error("wait hdr routine1 error");
		atomic_set(&gWakeupHdrThread1, 0);

		if (bsub_exist) {
			buf_sub = &hdr_video_layer[LAYER1];
			disp_cfd_config_video_frame(1, buf_sub,
				&(hdr_common_info.tv));
		}
	}
	return 0;
}

static int disp_hdr_routine2(void *data)
{
	struct mtk_disp_buffer *disp_buf_osd = NULL;
	int wait_ret = 0;

	disp_buf_osd = &hdr_osd_layer[LAYER0];

	while (!kthread_should_stop()) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq2,
			atomic_read(&gWakeupHdrThread2));
		if (wait_ret)
			hdr_error("wait hdr routine2 error");
		atomic_set(&gWakeupHdrThread2, 0);

		if (disp_buf_osd == NULL) {
			pr_err("disp_buf_osd NULL\n");
			continue;
		}

		if (!tv_info_set_by_cmd)
			disp_hw_mgr_get_info(&hdr_common_info);

		disp_cfd_config_graphic_frame(2, disp_buf_osd,
			&(hdr_common_info.tv));

		/* generate cfg setting */

		/* gce update */
	}
	return 0;
}

static int disp_hdr_routine3(void *data)
{
	struct mtk_disp_buffer *disp_buf_osd = NULL;
	int wait_ret = 0;

	disp_buf_osd = &hdr_osd_layer[LAYER1];

	while (!kthread_should_stop()) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq3,
			atomic_read(&gWakeupHdrThread3));
		if (wait_ret)
			hdr_error("wait hdr routine3 error");
		atomic_set(&gWakeupHdrThread3, 0);

		if (disp_buf_osd == NULL) {
			hdr_printf("disp_buf_osd NULL\n");
			continue;
		}

		if (!tv_info_set_by_cmd)
			disp_hw_mgr_get_info(&hdr_common_info);

		disp_cfd_config_graphic_frame(3, disp_buf_osd,
			&(hdr_common_info.tv));

		/* generate cfg setting */

		/* gce update */
	}
	return 0;
}

static int disp_hdr_routine4(void *data)
{
	uint32_t i = 0;
	int wait_ret = 0;

	while (!kthread_should_stop()) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq4,
			atomic_read(&gWakeupHdrThread4));
		if (wait_ret)
			hdr_error("wait hdr routine4 error");
		atomic_set(&gWakeupHdrThread4, 0);

		if (disp_hdr_irq_event == 0) {
			hdr_printf("no relate irq event\n");
			continue;
		}

		for (i = CLK_CHG_FE0; i < CLK_CHG_BE; i++) {
			if (disp_hdr_irq_event & (1 << i)) {
				disp_hdr_fe_set_clk(i, false);
				disp_hdr_irq_event &=
					~((1 << i) & 0xff);
			}
		}

		if (disp_hdr_irq_event & (1 << CLK_CHG_BE)) {
			disp_hdr_set_vdo_be_clk(false);
			disp_hdr_irq_event &=
				~((1 << CLK_CHG_BE) & 0xff);
		}

		if (disp_hdr_irq_event & (1 << FORCE_HDR_CHG)) {
			disp_hdr_irq_event &= ~((1 << FORCE_HDR_CHG) & 0xff);
			disp_hdr_handle_forcehdr(DISP_CMD_FORCE_HDR,
			(void *)(&ui_force_hdr_type));
		}
	}
	return 0;
}



int disp_hdr_thread_init(void)
{
	struct sched_param hdr_param = {.sched_priority = 2};

	if (!(disp_hdr_thread0 && disp_hdr_thread1
		&& disp_hdr_thread2 && disp_hdr_thread3
		&& disp_hdr_thread4)) {
		mutex_init(&disp_hdr_thread_mutex);
		hdr_info("mutex init\n");

		init_waitqueue_head(&disp_hdr_thread_wq0);
		init_waitqueue_head(&disp_hdr_thread_wq1);
		init_waitqueue_head(&disp_hdr_thread_wq2);
		init_waitqueue_head(&disp_hdr_thread_wq3);
		init_waitqueue_head(&disp_hdr_thread_wq4);

		atomic_set(&gWakeupHdrThread0, 0);
		atomic_set(&gWakeupHdrThread1, 0);
		atomic_set(&gWakeupHdrThread2, 0);
		atomic_set(&gWakeupHdrThread3, 0);
		atomic_set(&gWakeupHdrThread4, 0);
		hdr_info("wq init\n");

		disp_hdr_thread0 =
			kthread_create(disp_hdr_routine0,
			NULL, "disp_hdr_thread0");

		disp_hdr_thread1 =
			kthread_create(disp_hdr_routine1,
			NULL, "disp_hdr_thread1");

		disp_hdr_thread2 =
			kthread_create(disp_hdr_routine2,
			NULL, "disp_hdr_thread2");

		disp_hdr_thread3 =
			kthread_create(disp_hdr_routine3,
			NULL, "disp_hdr_thread3");

		disp_hdr_thread4 =
			kthread_create(disp_hdr_routine4,
			NULL, "disp_hdr_thread4");

		sched_setscheduler(disp_hdr_thread0, SCHED_RR, &hdr_param);

		wake_up_process(disp_hdr_thread0);
		wake_up_process(disp_hdr_thread1);
		wake_up_process(disp_hdr_thread2);
		wake_up_process(disp_hdr_thread3);
		wake_up_process(disp_hdr_thread4);

		hdr_info("thread %p %p %p %p %p init\n",
			disp_hdr_thread0,
			disp_hdr_thread1,
			disp_hdr_thread2,
			disp_hdr_thread3,
			disp_hdr_thread4);
	}

	return 0;
}


int disp_hdr_init(struct disp_hw_common_info *info)
{
	if (info == NULL) {
		hdr_printf("%s err\n", __func__);
		return -1;
	}

	mutex_init(&disp_hdr_path_mutex);
	mutex_init(&disp_hdr_stop_mutex);
	mutex_init(&disp_hdr_cfg_hdmi_mutex);
	disp_hdr_thread_init();
	/*enable hdr clk and path
	 * fhd hdr always enable when init(rgb2yuv for osd)
	 * hdr be enable when force dovi path
	 */
	disp_hdr_fe_start_stop(LAYER2, true);
	disp_hdr_fe_start_stop(LAYER3, true);
	#ifdef CONFIG_DOVI_SUPPORT
	if (g_force_dovi && g_dovi_efuse)
		disp_hdr_vdo_be_start_stop(true);

	if (g_dovi_efuse) {
		if (disp_dovi_init(info) != 0)
			hdr_printf("dovi_hdr init fail\n");
	}
	#endif
	if (disp_cfd_init(info) != 0)
		hdr_printf("open_hdr init fail\n");


	if (g_hdmi_res == HDMI_VIDEO_1280x720p_59_94Hz)
		old_resolution = HDMI_VIDEO_1280x720p_60Hz;
	else if (g_hdmi_res == HDMI_VIDEO_1920x1080p_59_94Hz)
		old_resolution = HDMI_VIDEO_1920x1080p_60Hz;
	else if (g_hdmi_res == HDMI_VIDEO_3840x2160P_59_94HZ)
		old_resolution = HDMI_VIDEO_3840x2160P_60HZ;
	else
		old_resolution = g_hdmi_res;

	hdr_output_signal_type = (enum hdr_output_type)g_out_format;

	hdr_printf("%s lk info:res = %d, force(%d %d %d %d %d) ver %d\n", __func__,
		old_resolution, g_force_hdr, g_force_dovi, g_dovi_efuse,
		g_force_open_hdr, g_out_format, g_ic_version);

	#ifdef CONFIG_DOVI_SUPPORT
	dovi_out_format = (enum dovi_signal_format_t)g_out_format;
	if (g_force_dovi && g_dovi_efuse)
		disp_dovi_force_gfx_vs10();
	else if (g_force_open_hdr)
	#else
	if (g_force_open_hdr)
	#endif
	{
		// defaults open cfd from lk and just clk on keep
		disp_cfd_enable(HDR_CFD_LAYER_FHD_GFX, true);
	} else {
		// defaults open r2y
		disp_cfd_enable(HDR_CFD_LAYER_FHD_GFX, true);
		#ifdef CONFIG_DOVI_SUPPORT
		if (g_dovi_efuse) {
			disp_cfd_drv_set_bypass(HDR_CFD_LAYER_FHD_GFX);
			disp_cfd_drv_set_r2y(HDR_CFD_LAYER_FHD_GFX, 0);
		}
		#endif
	}

	vdp_start_st[0] = 0;
	vdp_start_st[1] = 0;
	adl_mode = 0x2800;
	hdr_init_done = true;
	hdr_debug_init();

	return 0;
}

int disp_hdr_deinit(void)
{
	#ifdef CONFIG_DOVI_SUPPORT
	if (g_dovi_efuse)
		disp_dovi_deinit();
	#endif

	if (disp_cfd_deinit() != 0)
		hdr_printf("open_hdr deinit fail\n");

	hdr_init_done = false;

	return 0;
}

#ifdef CONFIG_HDMI_BLACK
int disp_hdr_deep_suspend(void)
{
	#ifdef CONFIG_DOVI_SUPPORT
	if (dovi_path_en && g_dovi_efuse) {
		disp_hdr_vdo_be_start_stop(false);
		fg_hdr_deep_suspend = true;
		disp_hdr_wakeup_routine(4);
		hdr_printf("hdr deep suspend done\n");
	}
	#endif

	return 0;
}

#endif

int disp_hdr_suspend(void)
{
	#ifdef CONFIG_DOVI_SUPPORT
	if (g_dovi_efuse && (disp_dovi_suspend() == 0))
		hdr_printf("dovi_hdr suspend\n");
	#endif

	disp_hdr_fe_start_stop(LAYER0, false);
	disp_hdr_fe_start_stop(LAYER1, false);
	disp_hdr_fe_start_stop(LAYER2, false);
	disp_hdr_fe_start_stop(LAYER3, false);
#if IS_ENABLED(CONFIG_DOVI_SUPPORT)
	if (!disp_common_info.low_energy_dozing_mode_enable) {
		if (g_dovi_efuse && dovi_path_en) {
			disp_hdr_vdo_be_start_stop(false);
			disp_hdr_wakeup_routine(4);
		}
	}
#endif
	hdr_init_done = false;
	hdr_printf("hdr suspend done\n");
	return 0;
}

int disp_hdr_resume(void)
{
	#ifdef CONFIG_DOVI_SUPPORT
	#ifdef CONFIG_HDMI_BLACK
	if (fg_hdr_deep_suspend && g_dovi_efuse && dovi_path_en) {
		disp_hdr_vdo_be_start_stop(true);
		disp_dovi_force_gfx_vs10();
		fg_hdr_deep_suspend = false;
		hdr_printf("hdr deep resume\n");
	} else {
		if (!disp_common_info.low_energy_dozing_mode_enable
			&& dovi_path_en && g_dovi_efuse) {
			disp_hdr_vdo_be_start_stop(true);
			disp_dovi_force_gfx_vs10();
		}
	}
	#else
	if (!disp_common_info.low_energy_dozing_mode_enable) {
		if (dovi_path_en && g_dovi_efuse) {
			disp_hdr_vdo_be_start_stop(true);
			disp_dovi_force_gfx_vs10();
		}
	}
	#endif

	if (g_dovi_efuse)
		dovi_path_ready2start = 1;
	#endif

	hdr_printf("hdr resume done\n");

	return 0;
}

int disp_hdr_change_resolution(
	const struct disp_hw_resolution *info)
{
	struct disp_hw_tv_capbility tv_cap;
	struct disp_hw *hdr_drv = disp_hdr_get_drv();
	enum HDR_PATH hdr_path = DEFAULT_PATH;
	struct video_buffer_info *cur_buf = NULL;
	uint32_t res_mod = 0;
	#ifdef CONFIG_DOVI_SUPPORT
	uint32_t dv_res_change = 0;
	#endif

	if (info == NULL)
		return -1;

	hdr_res_width = info->width;
	hdr_res_height = info->height;
	hdr_input_width[0] = hdr_res_width;
	hdr_input_height[0] = hdr_res_height;
	hdr_input_width[1] = hdr_res_width;
	hdr_input_height[1] = hdr_res_height;

	res_mod = info->res_mode;
	hdr_drv->drv_call(DISP_CMD_GET_HDMI_CAP, &tv_cap);

	/* for hotplug case, only allm ui force on
	 * ui set as force hdr mode
	 * no allm support tv <-> allm support tv
	 */
	if (ui_allm_type == ALLM_EN) {
		if ((tv_cap.u1_sink_allm_support
			|| tv_cap.u1_sink_14gamemode_support
			|| tv_cap.is_support_dovi_low_latency)
			&& tv_cap.is_support_dovi) {
			disp_hdr_allm_process(&tv_cap);
		} else
			b_allm_ctl_force_hdr = false;
	}
	#ifdef CONFIG_DOVI_SUPPORT
	if (g_dovi_efuse)
		disp_dovi_resolution_change(info);
	if (dovi_path_en && g_dovi_efuse) {
		dv_res_change = dovi_update_res_change(&tv_cap, info);
		hdr_path = DOVI_PATH;
	} else
	#endif
	{
		//opendhr change res
		cur_buf = &hdr_video_layer[LAYER0];
		disp_cfd_chg_output(cur_buf, info, &tv_cap);
		hdr_path = OPENHDR_PATH;
	}

	#ifdef CONFIG_DOVI_SUPPORT
	if (!dv_res_change) {
		hdr_printf("dovi res not real change\n");
		return 0;
	}
	#endif
	disp_hdr_config_hdmi_signal(hdr_path);
	hdr_printf("path[%d] chg res[%d][%dx%d] done\n",
		hdr_path, res_mod, hdr_res_width, hdr_res_height);

	return 0;
}

int disp_hdr_irq_handler(uint32_t irq)
{
	switch (irq) {
	case DISP_IRQ_BEFIFO:
		befifo_irq_cnt++;
		if (disp_hdr_irq_event)
			disp_hdr_irq_handle();
		break;
	case DISP_IRQ_FMT_VSYNC:
		#ifdef CONFIG_DOVI_SUPPORT
		if (g_dovi_efuse)
			disp_dovi_irq_handler(irq);
		#endif
		break;
	default:
		break;
	}
	return 0;
}

int disp_hdr_cmd(enum DISP_CMD cmd, void *data)
{
	switch (cmd) {
	case DISP_CMD_FORCE_HDR:
		if (force_sdr_output)
			*((uint32_t *) data) = 1;
		ui_force_hdr_type = *((uint32_t *) data);
		g_force_hdr = ui_force_hdr_type;
		#ifdef CONFIG_DOVI_SUPPORT
		if ((dovi_vs10_path_en == ui_force_hdr_type) && g_dovi_efuse) {
			hdr_printf("dovi path is already enabled!\n");
			return 0;
		}
		#endif

		/* only force hdr mode may reject due to allm on */
		if (b_allm_ctl_force_hdr
			&& (ui_force_hdr_type == DYNA_SET_FORCE_HDR)
			&& (disp_common_info.tv.u1_sink_allm_support
			|| disp_common_info.tv.u1_sink_14gamemode_support
			|| disp_common_info.tv.is_support_dovi_low_latency)) {
			hdr_printf("force hdr ctl by allm\n");
			return 0;
		}

		disp_hdr_irq_event |= 1 << FORCE_HDR_CHG;
		//disp_hdr_handle_forcehdr(cmd, data);
		break;
	case DISP_CMD_OSD_START:
		disp_hdr_handle_osd_start(cmd, data);
		break;
	case DISP_CMD_OSD_STOP:
		disp_hdr_handle_osd_stop(cmd, data);
		break;
	case DISP_CMD_VDP_START:
		disp_hdr_handle_vdp_start(cmd, data);
		break;
	case DISP_CMD_VDP_STOP:
		disp_hdr_handle_vdp_stop(cmd, data);
		break;
	case DISP_CMD_GCE_SET_HDR_ID:
		/* save hdr gce memory */
		break;
	case DISP_CMD_ALLM_TYPE:
		disp_hdr_handle_allm_change(data);
		break;
	default:
		#ifdef CONFIG_DOVI_SUPPORT
		if (g_dovi_efuse)
			disp_dovi_process_cmd(LAYER0, cmd, data);
		#endif
		break;
	}
	return 0;
}


struct disp_hw disp_hdr_driver = {
	.name = HDR_DRV_NAME,
	.init = disp_hdr_init,
	.deinit = disp_hdr_deinit,
	.start = NULL,
	.stop = NULL,

#ifdef CONFIG_HDMI_BLACK
	.deep_suspend = disp_hdr_deep_suspend,
	.deep_resume = NULL,
#endif
	.suspend = disp_hdr_suspend,
	.resume = disp_hdr_resume,
	.get_info = NULL,
	.change_resolution = disp_hdr_change_resolution,
	.config = NULL,
	.irq_handler = disp_hdr_irq_handler,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = NULL,
	.set_cmd = disp_hdr_cmd,
};

struct disp_hw *disp_hdr_get_drv(void)
{
	return &disp_hdr_driver;
}

