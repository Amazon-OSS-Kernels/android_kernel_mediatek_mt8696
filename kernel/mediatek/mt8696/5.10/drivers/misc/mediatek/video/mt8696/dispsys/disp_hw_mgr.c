/*
 * disp_hw_mgr.c - display hardware manager
 *
 *  Copyright (C) 2015-2016 MediaTek Inc.
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/sched.h>
#include <linux/sched/sysctl.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/time64.h>
#include "imgresz.h"

#include <linux/dma-mapping.h>

#define LOG_TAG "HW"

#include "disp_clk.h"
#include "disp_hw_debug.h"
#include "disp_hw_log.h"
#include "disp_hw_mgr.h"
#include "disp_irq.h"
#include "disp_path.h"
#include "disp_reg.h"
#include "hdmitx.h"
#include "fmt_hal.h"
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
#include "hdmiedid.h"
#endif
#ifdef DISP_GCE_SUPPORT
#include "mtk-cmdq.h"
#endif
#include "hdmictrl.h"
#include "disp_sys_hal.h"

#ifdef DISP_QMS_SUPPORT
uint64_t fmt_active_irq_cnt;
bool vrr_start_line_mode;
bool vrr_timer_check;
#endif

struct disp_hw *disp_pp_get_drv(void)
{
	return NULL;
}

static struct disp_hw_manager disp_hw_mgr = {
	.irq_num = 0,
	.sequence.init = {

			DISP_MODULE_PMX, DISP_MODULE_OSD, DISP_MODULE_VDP,
			DISP_MODULE_ML, DISP_MODULE_ADL,
			DISP_MODULE_HDR, DISP_MODULE_VIDEOIN, DISP_MODULE_P2I,
			DISP_MODULE_BEFIFO, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
		},
	.sequence.config = {

			DISP_MODULE_OSD, DISP_MODULE_HDR, DISP_MODULE_PMX,
			DISP_MODULE_VDP, DISP_MODULE_VIDEOIN, DISP_MODULE_P2I,
			DISP_MODULE_ML, DISP_MODULE_ADL,
			DISP_MODULE_BEFIFO, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
		},
	.sequence.change_res = {

			DISP_MODULE_PMX, DISP_MODULE_P2I, DISP_MODULE_OSD,
			DISP_MODULE_VDP, DISP_MODULE_HDR, DISP_MODULE_VIDEOIN,
			DISP_MODULE_ML, DISP_MODULE_ADL,
			DISP_MODULE_BEFIFO, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
		},
	.sequence.suspend = {

			DISP_MODULE_P2I, DISP_MODULE_HDR,
			DISP_MODULE_OSD, DISP_MODULE_VIDEOIN,
			DISP_MODULE_ML, DISP_MODULE_ADL,
			DISP_MODULE_BEFIFO, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
			DISP_MODULE_VDP, DISP_MODULE_PMX,
		},
	.sequence.resume = {

			DISP_MODULE_PMX, DISP_MODULE_VDP, DISP_MODULE_P2I,
			DISP_MODULE_ADL, DISP_MODULE_OSD, DISP_MODULE_ML,
			DISP_MODULE_HDR, DISP_MODULE_VIDEOIN,
			DISP_MODULE_BEFIFO, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
		},
#ifdef CONFIG_HDMI_BLACK
	.sequence.deep_suspend = {

			DISP_MODULE_P2I, DISP_MODULE_HDR,
			DISP_MODULE_OSD, DISP_MODULE_VIDEOIN,
			DISP_MODULE_ML, DISP_MODULE_ADL,
			DISP_MODULE_BEFIFO, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
			DISP_MODULE_VDP, DISP_MODULE_PMX,
		},
	.sequence.deep_resume = {

			DISP_MODULE_PMX, DISP_MODULE_VDP, DISP_MODULE_P2I,
			DISP_MODULE_ADL, DISP_MODULE_OSD, DISP_MODULE_ML,
			DISP_MODULE_HDR, DISP_MODULE_VIDEOIN,
			DISP_MODULE_BEFIFO, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
		},
#endif
#ifdef DISP_QMS_SUPPORT
	.sequence.vrr_enable = {

			DISP_MODULE_PMX, DISP_MODULE_BEFIFO, DISP_MODULE_VDP,
			DISP_MODULE_OSD, DISP_MODULE_HDR, DISP_MODULE_VIDEOIN,
			DISP_MODULE_ML, DISP_MODULE_ADL,
			DISP_MODULE_P2I, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
		},
#endif

	.sequence.gce_trigger = {

			DISP_MODULE_P2I, DISP_MODULE_OSD, DISP_MODULE_VDP,
			DISP_MODULE_HDR, DISP_MODULE_VIDEOIN, DISP_MODULE_PMX,
			DISP_MODULE_ML, DISP_MODULE_ADL,
			DISP_MODULE_BEFIFO, DISP_MODULE_FEFIFO, DISP_MODULE_R2R,
			DISP_MODULE_FG,
		},

	.get_drv[DISP_MODULE_OSD] = disp_osd_get_drv,
	.get_drv[DISP_MODULE_VDP] = disp_vdp_get_drv,
	.get_drv[DISP_MODULE_PMX] = disp_pmx_get_drv,
	.get_drv[DISP_MODULE_ML] = disp_ml_get_drv,
	.get_drv[DISP_MODULE_ADL] = disp_adl_get_drv,
	.get_drv[DISP_MODULE_HDR] = disp_hdr_get_drv,
	.get_drv[DISP_MODULE_VIDEOIN] = disp_videoin_get_drv,
	.get_drv[DISP_MODULE_P2I] = disp_p2i_get_drv,
	.get_drv[DISP_MODULE_BEFIFO] = disp_befifo_get_drv,
	.get_drv[DISP_MODULE_FEFIFO] = disp_fefifo_get_drv,
	.get_drv[DISP_MODULE_R2R] = disp_r2r_get_drv,
	.get_drv[DISP_MODULE_FG] = disp_fg_get_drv,

};

static struct disp_hw_manager *mgr = &disp_hw_mgr;
struct device *hw_mgr_dev;
static bool cur_hdmi_hpd_st;
static bool last_hdmi_hpd_st;
static struct mutex hw_mgr_clk_lock;

/*
 * htotal, vtotal, width , height,  freq, progressive,  hd,  resolution
 */
static const struct disp_hw_resolution disp_resolution_table[] = {
	{858, 525, 720, 480, 60, false, false, HDMI_VIDEO_720x480i_60Hz, false},
	{864, 625, 720, 576, 50, false, false, HDMI_VIDEO_720x576i_50Hz, false},
	{858, 525, 720, 480, 60, true, false, HDMI_VIDEO_720x480p_60Hz, false},
	{864, 625, 720, 576, 50, true, false, HDMI_VIDEO_720x576p_50Hz, false},
	{1650, 750, 1280, 720, 60, true, true, HDMI_VIDEO_1280x720p_59_94Hz,
	 true},
	{1650, 750, 1280, 720, 60, true, true, HDMI_VIDEO_1280x720p_60Hz,
	 false},
	{1980, 750, 1280, 720, 50, true, true, HDMI_VIDEO_1280x720p_50Hz,
	 false},
	{2200, 1125, 1920, 1080, 60, false, true, HDMI_VIDEO_1920x1080i_60Hz,
	 false},
	{2640, 1125, 1920, 1080, 50, false, true, HDMI_VIDEO_1920x1080i_50Hz,
	 false},
	{2200, 1125, 1920, 1080, 30, true, true, HDMI_VIDEO_1920x1080p_30Hz,
	 false},
	{2640, 1125, 1920, 1080, 25, true, true, HDMI_VIDEO_1920x1080p_25Hz,
	 false},
	{2750, 1125, 1920, 1080, 24, true, true, HDMI_VIDEO_1920x1080p_24Hz,
	 false},
	{2750, 1125, 1920, 1080, 24, true, true, HDMI_VIDEO_1920x1080p_23Hz,
	 true},
	{2200, 1125, 1920, 1080, 30, true, true, HDMI_VIDEO_1920x1080p_29Hz,
	 true},
	{2200, 1125, 1920, 1080, 60, true, true, HDMI_VIDEO_1920x1080p_59_94Hz,
	 true},
	{2200, 1125, 1920, 1080, 60, true, true, HDMI_VIDEO_1920x1080p_60Hz,
	 false},
	{2640, 1125, 1920, 1080, 50, true, true, HDMI_VIDEO_1920x1080p_50Hz,
	 false},
	{5500, 2250, 3840, 2160, 24, true, true, HDMI_VIDEO_3840x2160P_23_976HZ,
	 true},
	{5500, 2250, 3840, 2160, 24, true, true, HDMI_VIDEO_3840x2160P_24HZ,
	 false},
	{5280, 2250, 3840, 2160, 25, true, true, HDMI_VIDEO_3840x2160P_25HZ,
	 false},
	{4400, 2250, 3840, 2160, 30, true, true, HDMI_VIDEO_3840x2160P_29_97HZ,
	 true},
	{4400, 2250, 3840, 2160, 30, true, true, HDMI_VIDEO_3840x2160P_30HZ,
	 false},
	{5280, 2250, 3840, 2160, 50, true, true, HDMI_VIDEO_3840x2160P_50HZ,
	 false},
	{4400, 2250, 3840, 2160, 60, true, true, HDMI_VIDEO_3840x2160P_59_94HZ,
	 true},
	{4400, 2250, 3840, 2160, 60, true, true, HDMI_VIDEO_3840x2160P_60HZ,
	 false},
	{5500, 2250, 4096, 2160, 24, true, true, HDMI_VIDEO_4096x2160P_24HZ,
	 false},
	{5280, 2250, 4096, 2160, 50, true, true, HDMI_VIDEO_4096x2160P_50HZ,
	 false},
	{4400, 2250, 4096, 2160, 60, true, true, HDMI_VIDEO_4096x2160P_60HZ,
	 false},
};

#define DOWN_SEM()                                                             \
	do {                                                                   \
		if (down_interruptible(&mgr->mgr_sem)) {                       \
			DISP_LOG_E("semaphore down fail at %s:%s line%d\n",    \
				   __FILE__, __func__, __LINE__);              \
		}                                                              \
	} while (0)

#define UP_SEM() up(&mgr->mgr_sem)

#ifdef CONFIG_HDMI_BLACK
#define IS_DISP_DEEP_SUSPEND() \
	(atomic_read(&mgr->status) == DISP_STATUS_DEEP_SUSPEND)
#define IS_DISP_DEEP_RESUME() \
	(atomic_read(&mgr->status) == DISP_STATUS_DEEP_RESUME)
#endif
#define IS_DISP_SUSPEND() (atomic_read(&mgr->status) == DISP_STATUS_SUSPEND)
#define IS_DISP_RESUME() (atomic_read(&mgr->status) == DISP_STATUS_RESUME)
#define IS_DISP_NORMAL() (atomic_read(&mgr->status) == DISP_STATUS_NORMAL)
#define IS_DISP_DEINIT() (atomic_read(&mgr->status) == DISP_STATUS_DEINIT)

/*
 * After realize, please delete it.
 */
#define DISP_TODO
#ifdef DISP_TODO

struct disp_hw *disp_hdr_bt2020_get_drv(void);

#endif

static void _disp_lock_init(void)
{
	mutex_init(&(mgr->lock));
}

static void _disp_mutex_lock(void)
{
	mutex_lock(&(mgr->lock));
}

static void _disp_mutex_unlock(void)
{
	mutex_unlock(&(mgr->lock));
}

static const struct disp_hw_resolution *
_get_resolution(enum HDMI_VIDEO_RESOLUTION res_mode)
{
	int i;
	const struct disp_hw_resolution *res;
	uint32_t size = ARRAY_SIZE(disp_resolution_table);

	for (i = 0; i < size; i++) {
		res = &disp_resolution_table[i];
		if (res_mode == res->res_mode)
			break;
	}

	memcpy(&mgr->current_res, res, sizeof(struct disp_hw_resolution));
	if (res->res_mode == HDMI_VIDEO_1280x720p_59_94Hz)
		mgr->current_res.res_mode = HDMI_VIDEO_1280x720p_60Hz;
	else if (res->res_mode == HDMI_VIDEO_1920x1080p_59_94Hz)
		mgr->current_res.res_mode = HDMI_VIDEO_1920x1080p_60Hz;
	else if (res->res_mode == HDMI_VIDEO_3840x2160P_59_94HZ)
		mgr->current_res.res_mode = HDMI_VIDEO_3840x2160P_60HZ;

	DISP_LOG_D("%s: %d\n", __func__, res->res_mode);

	return &mgr->current_res;
}

static struct disp_hw_irq *_get_irq(uint32_t value)
{
	int i;

	for (i = 0; i < mgr->irq_num; i++) {
		if (value == mgr->hw_irq[i].value)
			return &mgr->hw_irq[i];
	}

	return NULL;
}

static uint64_t get_current_time_ns(void)
{
	struct timespec64 t;

	ktime_get_ts64(&t);
	return (t.tv_sec & 0xFFF) * 1000000000 + t.tv_nsec;
}


static void _disp_change_resolution_work(struct work_struct *work)
{
	unsigned long start, end;
	int time;
	struct disp_hw *drv =
		container_of(work, struct disp_hw, change_res_work);

	DOWN_SEM();
	DISP_LOG_HW("%s start to change resolution\n", drv->name);
	start = get_current_time_ns();
	drv->change_resolution(mgr->common_info.resolution);
	end = get_current_time_ns();
	time = (int)(end - start);
	DISP_LOG_HW("%s change resolution done, spend %dms\n", drv->name,
		    (time / 1000000));
	DISP_MMP(MMP_DISP_HW_CHANGE_RES, MMP_PULSE, drv->module, time);

	mgr->change_res_module &= (~(1 << drv->module));
	UP_SEM();
	if (mgr->change_res_module == 0) {
		atomic_set(&mgr->change_res_done_flag, 1);
		wake_up(&mgr->change_res_wq);
	}
}

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
static int _get_hdmi_cap(struct disp_hw_tv_capbility *cap)
{
	struct HDMI_EDID_T edid_info = {0};

	hdmi_AppGetEdidInfo(&edid_info);
	if (edid_info.ui1_sink_support_static_hdr & EDID_SUPPORT_SMPTE_ST_2084)
		cap->is_support_hdr = true;
	else
		cap->is_support_hdr = false;

	if (edid_info.ui1_sink_support_static_hdr & EDID_SUPPORT_FUTURE_EOTF)
		cap->is_support_hlg = true;
	else
		cap->is_support_hlg = false;

	if (edid_info.ui1_sink_support_dynamic_hdr & EDID_SUPPORT_DOVI_HDR)
		cap->is_support_dovi = true;
	else
		cap->is_support_dovi = false;

#if defined(CONFIG_MTK_HDR10PLUS_SUPPORT)
	if (edid_info.ui1_sink_support_dynamic_hdr & EDID_SUPPORT_HDR10_PLUS)
		cap->is_support_hdr10_plus = true;
	else
		cap->is_support_hdr10_plus = false;
#endif

	if (edid_info.ui1_sink_support_dynamic_hdr &
	    EDID_SUPPORT_DOVI_HDR_2160P60)
		cap->is_support_dovi_2160p60 = true;
	else
		cap->is_support_dovi_2160p60 = false;

	cap->is_support_601 = true;
	cap->is_support_709 = true;

	if ((edid_info.ui2_sink_colorimetry & SINK_COLOR_SPACE_BT2020_CYCC) ||
	    (edid_info.ui2_sink_colorimetry & SINK_COLOR_SPACE_BT2020_YCC) ||
	    (edid_info.ui2_sink_colorimetry & SINK_COLOR_SPACE_BT2020_RGB))
		cap->is_support_bt2020 = true;
	else
		cap->is_support_bt2020 = false;

	cap->hdr_content_max_luminance =
		edid_info.ui1_sink_hdr_content_max_luminance;
	cap->hdr_content_max_frame_average_luminance =
		edid_info.ui1_sink_hdr_content_max_frame_average_luminance;
	cap->hdr_content_min_luminance =
		edid_info.ui1_sink_hdr_content_min_luminance;

	memcpy((void *)cap->vsvdb_edid,
	       (void *)edid_info.ui1_sink_dovi_block, 0x1A);
	cap->is_support_dovi_low_latency =
		edid_info.ui4_sink_dovi_vsvdb_low_latency_support;
	cap->dovi_vsvdb_version =
		edid_info.ui4_sink_dovi_vsvdb_version;
	cap->dovi_vsvdb_dm_version =
		edid_info.ui4_sink_dovi_vsvdb_dm_version;
	cap->dovi_vsvdb_v2_interface =
		edid_info.ui4_sink_dovi_vsvdb_v2_interface;

	cap->hdr10_plus_app_ver = edid_info.ui1_sink_hdr10plus_app_version;

	DISP_LOG_HW(
		"get hdmi cap, hdr static 0x%X dynamic 0x%X colorimetry 0x%X appversion:%d\n",
		edid_info.ui1_sink_support_static_hdr,
		edid_info.ui1_sink_support_dynamic_hdr,
		edid_info.ui2_sink_colorimetry,
		edid_info.ui1_sink_hdr10plus_app_version);

	DISP_LOG_HW(
		"get hdmi cap, hdr low_latency 0x%X vsvdb_version 0x%X v2_interface 0x%X\n",
		cap->is_support_dovi_low_latency,
		cap->dovi_vsvdb_version,
		cap->dovi_vsvdb_v2_interface);

	cap->supported_resolution = vDispGetHdmiResolution();
	cap->screen_width = edid_info.ui1_Display_Horizontal_Size;
	cap->screen_height = edid_info.ui1_Display_Vertical_Size;
	/* maximum TMDS rate supported over HDMI */
	if (edid_info.ui2_sink_max_tmds_character_rate != 0)
		cap->max_tmds_rate = edid_info.ui2_sink_max_tmds_character_rate;
	else
		cap->max_tmds_rate = edid_info.ui1_sink_max_tmds_clock;

	cap->u1_sink_allm_support = edid_info.u1_sink_allm_support;
	cap->u1_sink_14gamemode_support = edid_info.u1_sink_14gamemode_support;
	DISP_LOG_HW(
		"get hdmi cap,ll:0x%X vsvdb:0x%X if:0x%X 0x%x allm:%d %d\n",
		cap->is_support_dovi_low_latency,
		cap->dovi_vsvdb_version,
		cap->dovi_vsvdb_v2_interface,
		cap->dovi_vsvdb_dm_version,
		cap->u1_sink_allm_support,
		cap->u1_sink_14gamemode_support);
#ifdef DISP_QMS_SUPPORT
		cap->u4_sink_vrr_min = edid_info.u4_sink_vrr_min;
		cap->u4_sink_vrr_max = edid_info.u4_sink_vrr_max;
		cap->u1_sink_cinemavrr = edid_info.u1_sink_cinemavrr;
		cap->u1_sink_mdelta = edid_info.u1_sink_mdelta;
		cap->u1_sink_qms_en = edid_info.u1_sink_qms_en;
		cap->u1_sink_qms_tfr_min = edid_info.u1_sink_qms_tfr_min;
		cap->u1_sink_qms_tfr_max = edid_info.u1_sink_qms_tfr_max;
		DISP_LOG_HW(
			"vrr(%d %d), max_min(%d %d), max_min_tfr(%d %d) mdelta %d\n",
			cap->u1_sink_qms_en,
			cap->u1_sink_cinemavrr,
			cap->u4_sink_vrr_max,
			cap->u4_sink_vrr_min,
			cap->u1_sink_qms_tfr_max,
			cap->u1_sink_qms_tfr_min,
			cap->u1_sink_mdelta);
#endif
	return 0;
}
#endif

#ifdef DISP_QMS_SUPPORT
struct vrr_mode *disp_get_vrr_info(void)
{
	struct vrr_mode *vrr_info = NULL;

	vrr_info = mgr->common_info.vrr_info;

	return vrr_info;
}

void disp_vrr_set_fraction_mvrr(void)
{
	struct vrr_mode *vrr_info = NULL;
	struct vrr_mode vrr_tmp = { 0 };
	int fraction_tmp1 = 0;
	int fraction_tmp2 = 0;
	struct disp_hw *drv = NULL;
	uint32_t line[3] = { 0 };
	bool b_fraction = true;

	vrr_info = mgr->common_info.vrr_info;

	if (vrr_timer_check)
		line[0] = fmt_hal_read_vline();

	_disp_mutex_lock();

	// no need adjust mvrr, but still need trigger startline
	if (vrr_info->mvrr_f_ratio == 0)
		b_fraction = false;

	memcpy(&vrr_tmp, vrr_info, sizeof(struct vrr_mode));

	if (vrr_timer_check)
		line[1] = fmt_hal_read_vline();

	if (b_fraction) {
		fraction_tmp1 = vrr_info->mvrr_fraction_value + vrr_info->mvrr_f_ratio;
		fraction_tmp2 = vrr_info->mvrr_fraction_value + vrr_info->mvrr_c_ratio;

		if (abs(fraction_tmp1) > abs(fraction_tmp2)) {
			vrr_tmp.current_vtotal = vrr_tmp.brr_vtotal + vrr_tmp.mvrr + 1;
			if (vrr_start_line_mode)
				vrr_tmp.trigger_start_line = vrr_tmp.current_vtotal - 1;

			vrr_info->mvrr_fraction_value += vrr_info->mvrr_c_ratio;
		} else {
			vrr_tmp.current_vtotal = vrr_tmp.brr_vtotal + vrr_tmp.mvrr;
			if (vrr_start_line_mode)
				vrr_tmp.trigger_start_line = vrr_tmp.current_vtotal - 1;
			vrr_info->mvrr_fraction_value += vrr_info->mvrr_f_ratio;
		}

		if (vrr_start_line_mode)
			_fmt_set_vrr_trigger_start_line_and_vrr_mode(
			vrr_tmp.trigger_start_line, true);
		else {
			//trigger vdout_fmt new vtotal
			drv = mgr->disp_hw_drv[DISP_MODULE_PMX];
			if (drv && drv->vrr_enable)
				drv->vrr_enable(&vrr_tmp);
		}
	}

	if (vrr_start_line_mode)
		fmt_hal_vrr_timing_trigger();

	if (vrr_timer_check)
		line[2] = fmt_hal_read_vline();

	DISP_LOG_VRR("fraction mvrr %d %d %d %d %d %d %d %d %d\n",
			vrr_info->mvrr_fraction_value,
			abs(fraction_tmp1), abs(fraction_tmp2),
			vrr_tmp.current_vtotal, vrr_tmp.vrr_en,
			vrr_start_line_mode,
			line[0], line[1], line[2]);
	_disp_mutex_unlock();

}

uint8_t disp_tfr_mapping(uint32_t fps)
{
	uint8_t tfr = 0;

	switch (fps) {
	case 23976:
	case 23000:
		tfr = 1;
		break;
	case 24000:
		tfr = 2;
		break;
	case 25000:
		tfr = 3;
		break;
	case 29970:
	case 29000:
		tfr = 4;
		break;
	case 30000:
		tfr = 5;
		break;
	case 47952:
		tfr = 6;
		break;
	case 48000:
		tfr = 7;
		break;
	case 50000:
		tfr = 8;
		break;
	case 59940:
	case 59000:
		tfr = 9;
		break;
	case 60000:
		tfr = 10;
		break;
	case 100000:
		tfr = 11;
		break;
	case 119880:
		tfr = 12;
		break;
	case 120000:
		tfr = 13;
		break;
	default:
		tfr = 0;
		break;
	}
	return tfr;
}

int disp_get_vrr_range(struct disp_hw_tv_capbility *tv_cap,
uint32_t freq, uint32_t *vrr_max, uint32_t *vrr_min)
{
	int ret = -1;
	/* check QMS */
	if (tv_cap->u1_sink_qms_en == 1) {
		if (tv_cap->u1_sink_qms_tfr_min == 1)
			*vrr_min = 23976; //24/1.001x1000
		else {
			if  (tv_cap->u4_sink_vrr_min == 0)
				*vrr_min = 47952; //48/1.001x1000
			else
				*vrr_min = tv_cap->u4_sink_vrr_min * VRR_PREC;
		}

		if (tv_cap->u1_sink_qms_tfr_max == 0)
			*vrr_max = 60000; //60x1000
		else {
			if  (tv_cap->u4_sink_vrr_max == 0)
				*vrr_max = freq;
			else
				*vrr_max = tv_cap->u4_sink_vrr_max * VRR_PREC;
		}
		ret = 0;
	}

	/* check cinema */
	if (tv_cap->u1_sink_cinemavrr == 1) {
		*vrr_min = 23976; //24/1.001x1000;
		if (tv_cap->u4_sink_vrr_max == 0)
			*vrr_max = freq;
		else
			*vrr_max = tv_cap->u4_sink_vrr_max * VRR_PREC;
		ret = 0;
	}
	DISP_LOG_VRR("%s %d %d %d %d %d %d %d %d %d\n", __func__, freq,
		tv_cap->u1_sink_qms_en, tv_cap->u1_sink_cinemavrr,
		tv_cap->u1_sink_qms_tfr_max, tv_cap->u1_sink_qms_tfr_min,
		tv_cap->u4_sink_vrr_max, tv_cap->u4_sink_vrr_max,
		*vrr_max, *vrr_min);
	return ret;
}

void disp_module_vrr_trigger(struct vrr_mode *vrr_info)
{
	uint8_t i = 0;
	enum DISP_MODULE_ENUM module = 0;
	struct disp_hw *drv = NULL;
	uint32_t line_cnt[3] = { 0 };

	if (vrr_info == NULL) {
		DISP_LOG_I("vrr_info error\n");
		return;
	}

	mutex_lock(&(vrr_info->vrr_mutex));

	if (vrr_timer_check)
		line_cnt[0] = fmt_hal_read_vline();

	if (vrr_info->vrr_en) {
		//first vsync to enable qms,need config hdmi
		//if ((vrr_info->current_vtotal == vrr_info->brr_vtotal) &&
		//	(vrr_info->mvrr_delat_step >= 2)) {
		//	vQMSSetting(true, vrr_info->next_tfr, 0);
		//}

		//add current vtotal
		vrr_info->current_vtotal += vrr_info->mvrr_delat_limit;
		if (vrr_info->current_vtotal > vrr_info->brr_vtotal + vrr_info->mvrr)
			vrr_info->current_vtotal = vrr_info->brr_vtotal + vrr_info->mvrr;

		if (vrr_start_line_mode)
			vrr_info->trigger_start_line = vrr_info->current_vtotal - 1;
		else
			vrr_info->trigger_start_line = 0;

		//set hdmi qms Mconst = 1
		if (vrr_info->mvrr_delat_step == 1)
			vQMSSetting(true, vrr_info->next_tfr, 1);
	} else
		vQMSSetting(false, vrr_info->next_tfr, 0);

	if (vrr_timer_check)
		line_cnt[1] = fmt_hal_read_vline();

	//trigger display relate module vrr enable/disable
	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.vrr_enable[i];
		drv = mgr->disp_hw_drv[module];
		if (drv && drv->vrr_enable)
			drv->vrr_enable(vrr_info);
	}

	if (vrr_timer_check)
		line_cnt[2] = fmt_hal_read_vline();

	DISP_LOG_VRR("vrr trigger step %d %d,%lld line(%d %d %d)\n",
		vrr_info->mvrr_delat_step, vrr_info->current_vtotal,
		fmt_active_irq_cnt, line_cnt[0], line_cnt[1], line_cnt[2]);
	if (vrr_info->vrr_en) {
		vrr_info->mvrr_delat_step--;
		if (vrr_info->mvrr_delat_step < 0)
			DISP_LOG_I("vrr trigger step error %d\n",
			vrr_info->mvrr_delat_step);
	}
	mutex_unlock(&(vrr_info->vrr_mutex));
}

int disp_qms_condition_check(uint32_t src_fps,
	bool dovi, bool hdr10p)
{
	struct disp_hw_tv_capbility tv_cap = { 0 };
	const struct disp_hw_resolution *res = NULL;
	uint32_t vrr_max = 0;
	uint32_t vrr_min = 0;
	uint32_t brr_freq = 0;
	uint8_t tfr = 0;

	//get tv cap of vrr and the BRR res
	_get_hdmi_cap(&tv_cap);
	res = mgr->common_info.resolution;

	//prejudge whether dovi output
	#if IS_ENABLED(CONFIG_DOVI_SUPPORT)
	if (disp_hdr_prejudge_dovi_output(dovi, hdr10p)) {
		DISP_LOG_I("prejudge dovi output, qms not enable\n");
		return -1;
	}
	#endif


	//calc brr real frequency,x1000
	brr_freq = res->frequency * VRR_PREC;
	if (res->is_fractional)
		brr_freq = brr_freq * VRR_PREC / (VRR_PREC + 1);

	//BRR freq must >= 50 & vactive >= 720 & progressive timing
	if (!(res->is_progressive && (brr_freq >= 50000)
		&& (res->height >= 720))) {
		DISP_LOG_I("QMS BRR not match request %d\n", res->res_mode);
		return -1;
	}

	//get VRRmax and VRRmin of sink device
	if (disp_get_vrr_range(&tv_cap, brr_freq, &vrr_max, &vrr_min)) {
		DISP_LOG_I("QMS not support by edid\n");
		return -1;
	}

	//check source fps supported by VRR range
	if (src_fps < vrr_min || src_fps > vrr_max) {
		DISP_LOG_I("src fps out vrr range %d %d %d\n", src_fps, vrr_min, vrr_max);
		return -1;
	}

	tfr = disp_tfr_mapping(src_fps);
	if (tfr == 0) {
		DISP_LOG_I("src fps out tfr range\n");
		return -1;
	}

	return 0;
}

void disp_queue_vrr_mode_disable(void)
{
	struct disp_hw_tv_capbility tv_cap = { 0 };
	struct vrr_mode *vrr_info = NULL;
	const struct disp_hw_resolution *res = NULL;

	//get tv cap of vrr and the BRR res
	_get_hdmi_cap(&tv_cap);
	res = mgr->common_info.resolution;
	vrr_info = mgr->common_info.vrr_info;

	vrr_info->vrr_en = false;
	vrr_info->next_tfr = 0;
	vrr_info->brr_vtotal = res->vtotal;
	vrr_info->mvrr = 0;
	vrr_info->current_vtotal = res->vtotal;
	vrr_info->mvrr_fraction_value = 0;
	vrr_info->mvrr_f_ratio = 0;
	vrr_info->mvrr_c_ratio = 0;
	vrr_info->mvrr_delat_limit = 0;
	vrr_info->mvrr_delat_step = 0;
	vrr_info->trigger_start_line = 0;
	vrr_info->start_line_mode_vtotal = 0;
	vrr_start_line_mode = false;

	disp_module_vrr_trigger(vrr_info);

	DISP_LOG_VRR("%s %d %d\n", __func__, res->res_mode, res->vtotal);
}

int disp_queue_vrr_mode_enable(uint32_t src_fps)
{
	struct disp_hw_tv_capbility tv_cap = { 0 };
	struct vrr_mode *vrr_info = NULL;
	const struct disp_hw_resolution *res = NULL;
	uint32_t vrr_max = 0;
	uint32_t vrr_min = 0;
	uint32_t brr_freq = 0;
	int mvrr_delta = 0;
	uint32_t mvrr = 0;
	uint64_t mvrr_temp = 0;
	uint32_t mvrr_fraction = 0;
	uint32_t mvrr_delta_step = 0;
	uint8_t tfr = 0;

	//get tv cap of vrr and the BRR res
	_get_hdmi_cap(&tv_cap);
	res = mgr->common_info.resolution;

	//calc brr real frequency,x1000
	brr_freq = res->frequency * VRR_PREC;
	if (res->is_fractional)
		brr_freq = brr_freq * VRR_PREC / (VRR_PREC + 1);

	//BRR freq must >= 50 & vactive >= 720 & progressive timing
	if (!(res->is_progressive && (brr_freq >= 50000)
		&& (res->height >= 720))) {
		DISP_LOG_I("QMS BRR not match request %d\n", res->res_mode);
		return -1;
	}

	//get VRRmax and VRRmin of sink device
	if (disp_get_vrr_range(&tv_cap, brr_freq, &vrr_max, &vrr_min)) {
		DISP_LOG_I("QMS not support by edid\n");
		return -1;
	}

	//check source fps supported by VRR range
	if (src_fps < vrr_min || src_fps > vrr_max) {
		DISP_LOG_I("src fps out vrr range\n");
		return -1;
	}

	tfr = disp_tfr_mapping(src_fps);
	if (tfr == 0) {
		DISP_LOG_I("src fps out tfr range\n");
		return -1;
	}

	vrr_info = mgr->common_info.vrr_info;
	//calc startline mode Vtotal,keep startline mode vtotal as max value
	vrr_info->start_line_mode_vtotal = res->vtotal * 60 / 23;

	//calc Mvrr by src_fps and BRR freq & vtotal
	mvrr_temp = ((uint64_t)brr_freq) * VRR_PREC * VRR_PREC;
	do_div(mvrr_temp, src_fps);
	DISP_LOG_VRR("mvrr_temp = %lld\n", mvrr_temp);
	mvrr_temp -= VRR_PREC * VRR_PREC;
	mvrr_temp = mvrr_temp * res->vtotal;
	mvrr_fraction = do_div(mvrr_temp, VRR_PREC * VRR_PREC);
	DISP_LOG_VRR("mvrr_temp= %lld, %d\n", mvrr_temp, mvrr_fraction);

	mvrr = mvrr_temp;
	mvrr_delta = mvrr;

	//mdelta should check when sink_mdelta is 1
	if (/*(tv_cap.u1_sink_cinemavrr == 1) &&*/ tv_cap.u1_sink_mdelta)
		mvrr_delta = res->vtotal / 2;

	//calc mvrr_delat_step
	if (mvrr > mvrr_delta)
		mvrr_delta_step = (mvrr + mvrr_delta-1) / mvrr_delta + 1;
	else
		mvrr_delta_step = 2; //2;

	vrr_info->vrr_en = true;
	vrr_info->next_tfr = tfr;
	vrr_info->brr_vtotal = res->vtotal;
	vrr_info->mvrr = mvrr;
	vrr_info->mvrr_fraction_value = 0 - mvrr_fraction;
	vrr_info->mvrr_f_ratio = 0 - mvrr_fraction;
	vrr_info->mvrr_c_ratio = VRR_PREC * VRR_PREC - mvrr_fraction;
	vrr_info->current_vtotal = res->vtotal;
	vrr_info->mvrr_delat_limit = mvrr_delta;
	vrr_info->mvrr_delat_step = mvrr_delta_step;
	vrr_info->trigger_start_line = 0;

	/*only fraction mvrr case use starline trigger mode*/
	if (vrr_info->mvrr_f_ratio)
		vrr_start_line_mode = true;
	else
		vrr_start_line_mode = false;

	DISP_LOG_VRR("mvrr info %d %d %d %d %d %d %d %d %d %d %d\n",
		brr_freq, src_fps,
		mvrr, mvrr_delta, tfr, mvrr_delta_step, res->res_mode,
		vrr_info->mvrr_f_ratio, vrr_info->mvrr_c_ratio,
		vrr_info->mvrr_fraction_value,
		vrr_info->start_line_mode_vtotal);

	//disp_module_vrr_trigger(vrr_info);

	return 0;
}

int disp_queue_vrr_mode_handle(bool en, uint32_t fps)
{
	struct vrr_mode *vrr_info = NULL;
	const struct disp_hw_resolution *res = NULL;
	uint32_t brr_freq = 0;
	int ret = 0;
	uint8_t tfr = 0;

	_disp_mutex_lock();
	res = mgr->common_info.resolution;
	vrr_info = mgr->common_info.vrr_info;

	if (en)
		ret = disp_queue_vrr_mode_enable(fps); //fps shall x1000
	else {
		//force vrr mode, change back to BRR_VRR setting
		if (vrr_info->vrr_mode == 1) {
			brr_freq = res->frequency * VRR_PREC;
			if (res->is_fractional)
				brr_freq =  brr_freq * VRR_PREC / (VRR_PREC + 1);
			ret = disp_qms_condition_check(brr_freq, 0, 0);
			if (ret == 0) {
				tfr = disp_tfr_mapping(brr_freq);
				vQMSSetting(true, tfr, 0);
				ret = disp_queue_vrr_mode_enable(brr_freq);
			}
		} else if (vrr_info->vrr_mode == 0)
			disp_queue_vrr_mode_disable();
	}
	DISP_LOG_VRR("%s %d %d %d %d\n", __func__, en, fps,
		vrr_info->vrr_mode, ret);
	_disp_mutex_unlock();
	return ret;
}

static int _disp_hw_vrr_kthread(void *data)
{
	struct vrr_mode *vrr_info = NULL;

	vrr_info = mgr->common_info.vrr_info;
	while (!kthread_should_stop()) {
		wait_event_interruptible(mgr->vrr_wq,
			   atomic_read(&(mgr->vrr_flag)));
		atomic_set(&(mgr->vrr_flag), 0);
		if (vrr_info->mvrr_delat_step > 0)
			disp_module_vrr_trigger(vrr_info);
		else
			disp_vrr_set_fraction_mvrr();
	}
	return 0;
}

static int disp_vrr_res_change(struct disp_hw_common_info *info)
{
	uint32_t freq = 0;
	uint8_t tfr = 0;
	int ret = 0;

	//adptive vrr not need vrr res change
	if (info == NULL || (info->vrr_info->vrr_mode == 0))
		return -1;

	//if hotplug during video play tfr should match video fps
	if (mgr->hw_playing_status[0] && (vdp_qms_fps[0] != 0))
		freq = vdp_qms_fps[0];
	else {
		freq = info->resolution->frequency * VRR_PREC;
		if (info->resolution->is_fractional)
			freq = freq * VRR_PREC / (VRR_PREC + 1);
	}

	ret = disp_qms_condition_check(freq, 0, 0);

	if (ret == 0) {
		tfr = disp_tfr_mapping(freq);
		vQMSSetting(true, tfr, 0);
		disp_queue_vrr_mode_enable(freq);
	} else {
		if (info->vrr_info->vrr_en == 1)
			disp_queue_vrr_mode_disable();
	}

	DISP_LOG_I("%s %d %d %d %d done\n", __func__, freq,
		info->resolution->res_mode, ret, info->vrr_info->vrr_en);
	return 0;
}

static int disp_force_vrr_mode_chg(struct disp_hw_common_info *info,
	bool en)
{
	uint32_t freq = 0;
	uint8_t tfr = 0;
	int ret = 0;

	if (info == NULL)
		return -1;

	freq = info->resolution->frequency * VRR_PREC;
	if (info->resolution->is_fractional)
		freq = freq * VRR_PREC / (VRR_PREC + 1);

	if (en) {
		ret = disp_qms_condition_check(freq, 0, 0);
		if (ret == 0) {
			tfr = disp_tfr_mapping(freq);
			if (g_qms)
				vQMSSetting(true, tfr, 1);
			else
				vQMSSetting(true, tfr, 0);
			disp_queue_vrr_mode_enable(freq);
		} else {
			if (info->vrr_info->vrr_mode == 1)
				disp_queue_vrr_mode_disable();
		}
		info->vrr_info->vrr_mode = 1;
	} else {
		if (info->vrr_info->vrr_en == 1)
			disp_queue_vrr_mode_disable();

		info->vrr_info->vrr_mode = 0;
	}

	g_qms = 0;

	DISP_LOG_I("%s %d %d %d done\n", __func__, ret,
		info->vrr_info->vrr_mode, info->vrr_info->vrr_en);
	return 0;
}
#endif

static void _disp_queue_change_res(void)
{
	int i;
	struct disp_hw *drv;
	enum DISP_MODULE_ENUM module;

	_disp_mutex_lock();
	/*set resolution information to disp_path*/
	disp_path_set_res(
		(struct disp_path_resolution *)mgr->common_info.resolution);

	DOWN_SEM();
	DISP_MMP(MMP_DISP_HW_CHANGE_RES, MMP_START,
		 mgr->common_info.resolution->res_mode, 0);
	DISP_LOG_HW("all sub module change resolution start\n");
	mgr->change_res_module = 0;
	mgr->common_info.hw_mgr_status = DISP_STATUS_CHANGE_RES;
	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.change_res[i];
		drv = mgr->disp_hw_drv[module];

		if (drv && drv->change_resolution) {
			mgr->change_res_module |= (1 << module);

			INIT_WORK(&drv->change_res_work,
				  _disp_change_resolution_work);
			queue_work(mgr->change_res_workq,
				   &drv->change_res_work);
		}
	}
	UP_SEM();

	/* wait for all sub module */
	wait_event(mgr->change_res_wq, atomic_read(&mgr->change_res_done_flag));
	atomic_set(&mgr->change_res_done_flag, 0);
	mgr->common_info.hw_mgr_status = DISP_STATUS_NORMAL;

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
	_get_hdmi_cap(&mgr->common_info.tv);
#endif

	DISP_MMP(MMP_DISP_HW_CHANGE_RES, MMP_END,
		 mgr->common_info.resolution->res_mode, 0);

	/* notify HWC resolution change done
	 * switch_set_state(&mgr->hdmi_res_switch,
	 * mgr->common_info.resolution->res_mode);
	 */

	_disp_mutex_unlock();
	DISP_LOG_HW("all sub module change resolution finish\n");
}

#ifdef DISP_GCE_SUPPORT
static void _disp_wakeup_gce_event(enum DISP_EVENT event)
{
	if (event & DISP_EVENT_GCE) {
		atomic_set(&mgr->gce_flag, event);
		wake_up(&mgr->gce_wq);
	}
}
#endif
static void _disp_wakeup_vsync_event(enum DISP_EVENT event)
{
	if (event) {
		mgr->vsync_ts = ktime_to_ns(ktime_get());
		atomic_set(&mgr->wait_vsync_flag, event);
		wake_up(&mgr->wait_vsync_wq);
	}
}

static int _disp_set_cmd(enum DISP_CMD cmd, void *data)
{
	struct disp_hw *drv;
	int i = 0;
	enum DISP_MODULE_ENUM module;

	if (cmd == DISP_CMD_GET_HDMI_CAP) {
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
		_get_hdmi_cap(&mgr->common_info.tv);
#endif
		if (data)
			memcpy(data, (void *)(&mgr->common_info.tv),
			       sizeof(struct disp_hw_tv_capbility));
		return 0;
	}

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.config[i];
		drv = mgr->disp_hw_drv[module];

		if (drv == NULL || drv->set_cmd == NULL)
			continue;

		drv->set_cmd(cmd, data);
	}
	return 0;
}

static int _disp_event_callback(enum DISP_EVENT event, void *data)
{
	enum HDMI_VIDEO_RESOLUTION res_mode = HDMI_VIDEO_RESOLUTION_NUM;
	const struct disp_hw_resolution *resolution;
#ifdef DISP_QMS_SUPPORT
	bool en = 0;
#endif

	switch (event) {
	case DISP_EVENT_CHANGE_RES:
		res_mode = *(enum HDMI_VIDEO_RESOLUTION *)data;
#if (0)
		if (res_mode == mgr->common_info.resolution->res_mode) {
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
			_get_hdmi_cap(&mgr->common_info.tv);
#endif
			break;
		}
#endif

		resolution = _get_resolution(res_mode);
		if (resolution != NULL) {
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
			_get_hdmi_cap(&mgr->common_info.tv);
#endif
			mgr->common_info.resolution = resolution;
			disp_hw_mgr_get_info(&disp_common_info);
			_disp_queue_change_res();
			#ifdef DISP_QMS_SUPPORT
			disp_vrr_res_change(&(mgr->common_info));
			#endif
		}
		break;
	case DISP_EVENT_FORCE_HDR:
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
		_get_hdmi_cap(&mgr->common_info.tv);
#endif
		disp_hw_mgr_get_info(&disp_common_info);
		mgr->common_info.tv.force_hdr = *(unsigned int *)data;
		_disp_set_cmd(DISP_CMD_FORCE_HDR, data);
		break;
#ifdef DISP_QMS_SUPPORT
	case DISP_EVENT_VRR:
#if IS_ENABLED(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
		_get_hdmi_cap(&mgr->common_info.tv);
#endif
		disp_hw_mgr_get_info(&disp_common_info);
		en = *(bool *)data;
		disp_force_vrr_mode_chg(&(mgr->common_info), en);
		break;
#endif
	case DISP_EVENT_PLUG_OUT:
		disp_osd_get_drv()->drv_call(DISP_CMD_HDMITX_PLUG_OUT, NULL);
		disp_vdp_get_drv()->stop(0);
		disp_vdp_get_drv()->stop(1);
		mgr->hw_playing_status[0] = false;
		mgr->hw_playing_status[1] = false;
		mgr->hdmi_plug_out = true;
		break;

	case DISP_EVENT_PLUG_IN:
		mgr->hdmi_plug_out = false;
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
		_get_hdmi_cap(&mgr->common_info.tv);
#endif
		disp_hw_mgr_get_info(&disp_common_info);
		disp_osd_get_drv()->drv_call(DISP_CMD_HDMITX_PLUG_IN, NULL);
		break;

	case DISP_EVENT_ALLM:
		_disp_set_cmd(DISP_CMD_ALLM_TYPE, data);
		break;

	case DISP_EVENT_LOW_ENERGY_DOZING_MODE:
		disp_common_info.low_energy_dozing_mode_enable
			= *(bool *)data;
		disp_hw_mgr.common_info.low_energy_dozing_mode_enable
			= *(bool *)data;
		DISP_LOG_I("%s low_energy_dozing_mode_enable:%d.\n", __func__,
			disp_common_info.low_energy_dozing_mode_enable);
		break;

	default:
		break;
	}

	return 0;
}

#ifdef DISP_GCE_SUPPORT
static int _disp_hw_gce_kthread(void *data)
{
	int i;
	struct disp_hw *drv;
	enum DISP_MODULE_ENUM module;
	struct disp_hw_trigger_info info = {0};

	while (1) {
		wait_event_interruptible(mgr->gce_wq,
			   atomic_read(&mgr->gce_flag));
		atomic_set(&mgr->gce_flag, 0);

		cmdq_pkt_wfe(mgr->common_info.gce_handle->pkt,
			mgr->common_info.gce_handle->event_id);
		cmdq_pkt_wfe(mgr->common_info.gce_handle->sec_pkt,
			mgr->common_info.gce_handle->event_id);

		info.pkt_update_flag = 0;
		for (i = 0; i < DISP_MODULE_NUM; i++) {
			module = mgr->sequence.gce_trigger[i];
			drv = mgr->disp_hw_drv[module];
			info.pkt = mgr->common_info.gce_handle->pkt;
			info.sec_pkt = mgr->common_info.gce_handle->sec_pkt;

			if (drv && drv->gce_trigger)
				drv->gce_trigger(&info);
		}

		if (info.pkt_update_flag & normal_pkt_updated)
			cmdq_pkt_flush(mgr->common_info.gce_handle->pkt);
		if (info.pkt_update_flag & sec_pkt_updated)
			cmdq_pkt_flush(mgr->common_info.gce_handle->sec_pkt);
		/* cmdq_pkt_dump_buf(mgr->common_info.gce_handle->pkt, 0); */
		/*do gce reset*/
		cmdq_pkt_reset(mgr->common_info.gce_handle->pkt);
		cmdq_pkt_reset(mgr->common_info.gce_handle->sec_pkt);
	}
	return 0;
}
#endif
static irqreturn_t _disp_irq_handler(int value, void *dev_id)
{
	int i;
	struct disp_hw *drv = (struct disp_hw *)dev_id;
	struct disp_hw_irq *hw_irq = _get_irq(value);
	#ifdef DISP_QMS_SUPPORT
	struct vrr_mode *vrr_info = NULL;
	#endif

	DISP_LOG_IRQ("%s irq num: %d\n", __func__, value);

	if (!hw_irq) {
		DISP_LOG_E("hw_irq is NULL, irq num: %d\n", value);
		return IRQ_NONE;
	}

	/*clear irq and printk some logs*/
	disp_irq_manager(hw_irq->irq);

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		drv = mgr->disp_hw_drv[i];
		if (drv && drv->irq_handler)
			drv->irq_handler(hw_irq->irq);
	}

#ifdef DISP_QMS_SUPPORT
	vrr_info = mgr->common_info.vrr_info;
	if (hw_irq->irq == DISP_IRQ_FMT_ACTIVE_START) {
		fmt_active_irq_cnt++;
		DISP_LOG_VRR_IRQ("vrr irq %lld %d %d %d\n", fmt_active_irq_cnt,
			vrr_info->vrr_en, vrr_info->mvrr_delat_step, fmt_hal_read_vline());
		if (vrr_info->vrr_en &&
			((vrr_info->mvrr_delat_step > 0) || vrr_start_line_mode)) {
			atomic_set(&mgr->vrr_flag, 1);
			wake_up(&mgr->vrr_wq);
		}
	}
#endif

	if (hw_irq->irq == DISP_IRQ_FMT_VSYNC)
		_disp_wakeup_vsync_event(DISP_EVENT_VSYNC);

	return IRQ_HANDLED;
}

static int _is_irq_need_enable(enum DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_PMX:
	case DISP_MODULE_VDP:
	case DISP_MODULE_OSD:
	case DISP_MODULE_BEFIFO:
		return 1;
	default:
		return 0;
	}
}

#if 0
static struct disp_hw_irq *_get_irq_by_irqbit(enum DISP_IRQ_E irq)
{
	int i;

	for (i = 0; i < mgr->irq_num; i++) {
		if (irq == mgr->hw_irq[i].irq)
			return &mgr->hw_irq[i];
	}

	return NULL;
}

/* polling irq status to call irq_handler */
static int disp_irq_polling_kthread_func(void *data)
{
	struct disp_hw_irq *irq = NULL;

	while (1) {
		/*read irq status register*/
		mdelay(1000);
		irq = _get_irq_by_irqbit(DISP_IRQ_FMT_VSYNC);
		if (irq)
			_disp_irq_handler(irq->value, NULL);

		mdelay(1);
		irq = _get_irq_by_irqbit(DISP_IRQ_FMT_ACTIVE_START);
		if (irq)
			_disp_irq_handler(irq->value, NULL);
	}

	return 0;
}

#endif

static int _disp_request_irq(struct disp_hw *drv)
{
	int i, ret = 0;

	for (i = 0; i < drv->irq_num; i++) {
		if (_is_irq_need_enable(drv->module) && drv->irq[i].value) {
			mgr->hw_irq[mgr->irq_num].value = drv->irq[i].value;
			mgr->hw_irq[mgr->irq_num].irq = drv->irq[i].irq;
			mgr->irq_num++;
			ret = request_irq(drv->irq[i].value,
					  (irq_handler_t)_disp_irq_handler,
					  IRQF_TRIGGER_NONE, drv->name,
					  (void *)drv);
			if (ret) {
				DISP_LOG_E("request %s irq_%d fail\n",
					   drv->name, drv->irq[i].value);
				ret = -EFAULT;
				break;
			}

			DISP_LOG_D("request %s irq_%d hw_irq %d success\n",
				   drv->name, drv->irq[i].value,
				   drv->irq[i].irq);
		}
	}

	return ret;
}

static int _init_common_info(struct disp_hw_common_info *common_info,
			     unsigned int resolution)
{
	enum HDMI_VIDEO_RESOLUTION res_mode = resolution;

	common_info->resolution = _get_resolution(res_mode);

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
	_get_hdmi_cap(&mgr->common_info.tv);
#endif

	return 0;
}

int disp_hw_mgr_set_pm_dev(struct device *dev, enum DISP_PM_TYPE type)
{
	return disp_clock_set_pm_dev(dev, type);
}

struct device *disp_hw_mgr_get_dev(void)
{
	return hw_mgr_dev;
}

static void disp_hw_mgr_set_dev(struct device *dev)
{
	hw_mgr_dev = dev;
}

void disp_hw_mgr_dispsys_shadow_enable(bool enable)
{
	int ret = 0;
	static int hw_mgr_clk_cnt = 0;

	mutex_lock(&hw_mgr_clk_lock);

	if (enable) {
		hw_mgr_clk_cnt++;

		if (hw_mgr_clk_cnt == 1) {
			imgresz_clk_on(0);
			imgresz_clk_on(1);
			ret = mtk_vq_resume();
			disp_sys_hal_shadow_en(enable);
		}
	} else {
		if (hw_mgr_clk_cnt > 0) {
			hw_mgr_clk_cnt--;

			if (hw_mgr_clk_cnt == 0) {
				disp_sys_hal_shadow_en(enable);
				imgresz_clk_off(0);
				imgresz_clk_off(1);
				ret = mtk_vq_suspend();
			}
		}
	}

	DISP_LOG_I("%s enable = %d ret %d clk_cnt %d\n",
		    __func__,
		    enable,
		    ret,
		    hw_mgr_clk_cnt);

	mutex_unlock(&hw_mgr_clk_lock);
}

int disp_hw_mgr_init(struct device *dev, unsigned int resolution)
{
	int i, ret = 0;
	struct disp_hw *hw_drv;
	enum DISP_MODULE_ENUM module;
	struct disp_hw_common_info *common_info;

	if (atomic_read(&mgr->status) != DISP_STATUS_DEINIT) {
		DISP_LOG_D("already inited.\n");
		return 0;
	}
	DISP_LOG_D("%s in\n", __func__);
	disp_hw_mgr_set_dev(dev);
	disp_log_init(dev);
	disp_hw_debug_init();
	_disp_lock_init();
	sema_init(&mgr->mgr_sem, 1);
	init_waitqueue_head(&mgr->gce_wq);
	#ifdef DISP_QMS_SUPPORT
	init_waitqueue_head(&mgr->vrr_wq);
	#endif
	init_waitqueue_head(&mgr->change_res_wq);
	init_waitqueue_head(&mgr->wait_vsync_wq);
	atomic_set(&mgr->change_res_done_flag, 0);
	atomic_set(&mgr->wait_vsync_flag, 0);
	mgr->vsync_ts = 0;
	mgr->change_res_module = 0;
	mgr->hdmi_plug_out = false;
	atomic_set(&mgr->status, DISP_STATUS_INIT);
	common_info = &mgr->common_info;
	_init_common_info(common_info, resolution);
	disp_path_init((struct disp_path_resolution *)common_info->resolution);
	/*create gce handle for every hw*/
#ifdef DISP_GCE_SUPPORT
	mgr->common_info.gce_handle = vmalloc(sizeof(struct cmdqNormalPath));
	if (mgr->common_info.gce_handle == NULL) {
		DISP_LOG_E("malloc memory failed when use gce");
		return 0;
	}
	common_info->gce_handle->clt_base = cmdq_register_device(dev);
	common_info->gce_handle->clt = cmdq_mbox_create(dev, 2);
	common_info->gce_handle->pkt =
		cmdq_pkt_create(common_info->gce_handle->clt);
	common_info->gce_handle->sec_clt = cmdq_mbox_create(dev, 3);
	common_info->gce_handle->sec_pkt =
		cmdq_pkt_create(common_info->gce_handle->sec_clt);
	if ((common_info->gce_handle->clt_base == NULL) ||
		(common_info->gce_handle->clt == NULL) ||
		(common_info->gce_handle->pkt == NULL) ||
		(common_info->gce_handle->sec_clt == NULL) ||
		(common_info->gce_handle->sec_pkt == NULL)) {
		DISP_LOG_E(
			"allocate gce resource fail %p, %p, %p, %p, %p\n",
			common_info->gce_handle->clt_base,
			common_info->gce_handle->clt,
			common_info->gce_handle->pkt,
			common_info->gce_handle->sec_clt,
			common_info->gce_handle->sec_pkt);
		}
	DISP_LOG_E("create cmdq %p, %p, %p, %p, %p\n",
		common_info->gce_handle->clt_base,
		common_info->gce_handle->clt,
		common_info->gce_handle->pkt,
		common_info->gce_handle->sec_clt,
		common_info->gce_handle->sec_pkt);
	of_property_read_u32(dev->of_node, "gce_events",
		&common_info->gce_handle->event_id);
	common_info->gce_handle->event_id = 69; /*be frame done*/
	/*create gce thread for gce processed register setting*/
	atomic_set(&mgr->gce_flag, 0);
	mgr->gce_task = kthread_create(_disp_hw_gce_kthread, NULL,
					   "disp_hw_gce_kthread");
	wake_up_process(mgr->gce_task);

	cmdq_pkt_wfe(mgr->common_info.gce_handle->pkt,
		mgr->common_info.gce_handle->event_id);
#endif
#ifdef DISP_QMS_SUPPORT
	mgr->common_info.vrr_info = vmalloc(sizeof(struct vrr_mode));
	if (mgr->common_info.vrr_info == NULL) {
		DISP_LOG_E("malloc memory failed when vrr tigger");
		return 0;
	}
	memset(mgr->common_info.vrr_info, 0, sizeof(struct vrr_mode));
	mutex_init(&(mgr->common_info.vrr_info->vrr_mutex));
	/* create vrr thread */
	atomic_set(&mgr->vrr_flag, 0);
	mgr->vrr_task = kthread_create(_disp_hw_vrr_kthread, NULL,
		"disp_hw_vrr_kthread");
	wake_up_process(mgr->vrr_task);
#endif

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.init[i];
		mgr->disp_hw_drv[module] = mgr->get_drv[module]();
		hw_drv = mgr->disp_hw_drv[module];
		if (hw_drv) {
			hw_drv->module = module;

			if (hw_drv->init)
				ret = hw_drv->init(common_info);
			if (ret != 0) {
				DISP_LOG_E("%s init fail, module=%d\n",
					   hw_drv->name, module);
				break;
			}

			if (hw_drv->get_info)
				ret = hw_drv->get_info(common_info);
			if (ret != 0) {
				DISP_LOG_E("%s get info fail\n", hw_drv->name);
				break;
			}

			if (hw_drv->set_listener)
				ret = hw_drv->set_listener(
					_disp_event_callback);
			if (ret != 0) {
				DISP_LOG_E("%s set listener fail\n",
					   hw_drv->name);
				break;
			}

			hw_drv->drv_call = _disp_set_cmd;

			ret = _disp_request_irq(hw_drv);
			if (ret != 0)
				break;

			DISP_LOG_D("module %s init done\n", hw_drv->name);
		}
	}
#ifdef DISP_GCE_SUPPORT
	/*cmdq_dump_pkt(mgr->common_info.gce_handle->pkt); Jancsi */
	cmdq_pkt_flush(mgr->common_info.gce_handle->pkt);
	/*do gce reset*/
	mgr->common_info.gce_handle->pkt->cmd_buf_size = 0;
#endif
	/*disp_path_shadow_enable();*/
#if 0
	mgr->hdmi_res_switch.name = "res_hdmi";
	mgr->hdmi_res_switch.index = 0;
	mgr->hdmi_res_switch.state = 0;
	ret = switch_dev_register(&mgr->hdmi_res_switch);
	if (ret) {
		DISP_LOG_E("switch_dev_register %s fail(%d)\n",
				mgr->hdmi_res_switch.name, ret);
		return -EFAULT;
	}
#endif
	mgr->change_res_workq =
		alloc_workqueue("disp_change_res_wq",
				WQ_HIGHPRI | WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (IS_ERR_OR_NULL(mgr->change_res_workq)) {
		DISP_LOG_E("can not alloc workqueue disp_change_res_wq\n");
		return -EFAULT;
	}

	mgr->log_level = DISP_LOG_LEVEL_DEBUG3;
	atomic_set(&mgr->status, DISP_STATUS_NORMAL);
	//lg fhd already enable
	//mgr->hw_playing_status[DISP_OSD_LAYER1] = true;

	mutex_init(&hw_mgr_clk_lock);

	return 0;
}

int disp_hw_mgr_get_info(struct disp_hw_common_info *info)
{
	int ret = 0;
	struct disp_hw_common_info *common_info;

	WARN_ON(!info);

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
	_get_hdmi_cap(&mgr->common_info.tv);
#endif

	common_info = &mgr->common_info;

	if (common_info == NULL) {
		DISP_LOG_E("common info is null, get common info failed\n");
		return -1;
	}

	memcpy(info, common_info, sizeof(struct disp_hw_common_info));

	return ret;
}

int disp_hw_get_vdp_cap(struct mtk_disp_vdp_cap *vdp_cap)
{
	int ret = 0;
	struct disp_hw *drv;

	memcpy(&(mgr->common_info.vdp_cap), vdp_cap,
	       sizeof(struct mtk_disp_vdp_cap));
	drv = mgr->disp_hw_drv[DISP_MODULE_VDP];
	if (drv && drv->get_info)
		ret = drv->get_info(&mgr->common_info);

	if (!ret)
		vdp_cap->need_resizer = mgr->common_info.vdp_cap.need_resizer;

	return 0;
}

int disp_hw_get_hdmi_cap(struct mtk_disp_hdmi_cap *hdmi_cap)
{
	int ret = 0;

	if (!hdmi_cap)
		return -1;

	memset(hdmi_cap, 0x00, sizeof(struct mtk_disp_hdmi_cap));

	if (mgr->common_info.tv.is_support_hdr)
		hdmi_cap->hdr_type |= MTK_HDR_TYPE_HDR10;
	if (mgr->common_info.tv.is_support_dovi)
		hdmi_cap->hdr_type |= MTK_HDR_TYPE_DOVI;
	if (mgr->common_info.tv.is_support_hlg)
		hdmi_cap->hdr_type |= MTK_HDR_TYPE_HLG;
	if (mgr->common_info.tv.is_support_hdr10_plus)
		hdmi_cap->hdr_type |= MTK_HDR_TYPE_HDR10PLUS;

	hdmi_cap->hdr_content_max_frame_average_luminance =
		mgr->common_info.tv.hdr_content_max_frame_average_luminance;
	hdmi_cap->hdr_content_max_luminance =
		mgr->common_info.tv.hdr_content_max_luminance;
	hdmi_cap->hdr_content_min_luminance =
		mgr->common_info.tv.hdr_content_min_luminance;

	hdmi_cap->supported_resolution =
		mgr->common_info.tv.supported_resolution;
	hdmi_cap->disp_resolution = mgr->common_info.resolution->res_mode;
	hdmi_cap->screen_width = mgr->common_info.tv.screen_width;
	hdmi_cap->screen_height = mgr->common_info.tv.screen_height;
	hdmi_cap->is_support_bt2020 = mgr->common_info.tv.is_support_bt2020;

	return ret;
}

int disp_hw_mgr_config(struct mtk_disp_config *config)
{
	int i, j, k, ret = 0;
	struct disp_hw *drv;
	enum DISP_MODULE_ENUM module;
	bool layer_enable[DISP_LAYER_NUM] = {0};
	bool play_stop_en = true;
	bool is_tunnel = false;
	bool has_invalid_video = false;

	_disp_mutex_lock();

	for (i = 0; i < DISP_BUFFER_MAX; i++) {
		if (config->buffer_info[i].layer_id >= DISP_BUFFER_MAX) {
			DISP_LOG_E("wrong layer id, type:%d\n", config->buffer_info[i].type);
			_disp_mutex_unlock();
			return -1;
		}
	}

	mgr->common_info.osd_swap = 0;

	DISP_MMP_STRUCT(MMP_DISP_HW, config, struct mtk_disp_config);
	/* has video layer or not */
	j = k = 0;
	if (config->user == DISP_USER_HWC) {
		for (i = 0; i < DISP_BUFFER_MAX; i++) {
			if (config->buffer_info[i].layer_enable &&
			    config->buffer_info[i].type == DISP_LAYER_VDP) {
				if (config->buffer_info[i].ion_fd != -1)
					layer_enable[j] = true;
				else
					has_invalid_video = true;

				config->buffer_info[i].layer_id = j;
				j++;

				/* add for vdp drop log*/
				if (mgr->is_tunnel_playback)
					mgr->common_info.tunnel_playback_and_user_is_hwc = true;
				else
					mgr->common_info.tunnel_playback_and_user_is_hwc = false;
			}

			if (config->buffer_info[i].layer_enable &&
			    config->buffer_info[i].type == DISP_LAYER_OSD) {
				if (config->buffer_info[i].crop.width > 1920 &&
					config->buffer_info[i].crop.height > 1080 &&
				    config->buffer_info[i].crop.width <= 8192 &&
				    config->buffer_info[i].buffer_source !=
				    DISP_BUFFER_ALPHA) {
					if (k == 0) {
						k = 1;
						mgr->common_info.osd_swap = 1;
					}
				}

				if (mgr->common_info.osd_swap == 1) {
					if (k == 2)
						k = 0;
				}
				config->buffer_info[i].layer_id =
					k; /* modify the layer id */
				layer_enable[k + DISP_OSD_LAYER1] = true;
				k++;
			}

			if (config->buffer_info[i].layer_enable &&
			    config->buffer_info[i].type == DISP_LAYER_AEE) {
				config->buffer_info[i].type = DISP_LAYER_OSD;
				config->buffer_info[i].layer_id =
					1; /* modify the layer id */
				layer_enable[DISP_OSD_LAYER2] = true;
				k++;
			}
			config->buffer_info[i].layer_order = i;
		}
		if (has_invalid_video) {
			layer_enable[DISP_VDP_LAYER1] =
				mgr->hw_playing_status[DISP_VDP_LAYER1];
			layer_enable[DISP_VDP_LAYER2] =
				mgr->hw_playing_status[DISP_VDP_LAYER2];
		}
	} else if (config->user == DISP_USER_AVSYNC) {
		for (i = 0; i < DISP_BUFFER_MAX; i++) {
			if (config->buffer_info[i].layer_enable &&
			    (config->buffer_info[i].type == DISP_LAYER_VDP ||
			     config->buffer_info[i].type ==
				     DISP_LAYER_SIDEBAND)) {
				config->buffer_info[i].layer_id = j;
				layer_enable[j] = true;
				j++;
				is_tunnel = true;
				mgr->common_info.tunnel_playback_and_user_is_hwc = false;
			}
		}
		/*Don't affect sub path state when avsync operate main path*/
		layer_enable[DISP_VDP_LAYER2] = mgr->hw_playing_status[DISP_VDP_LAYER2];
		/*no ui when tunnel mode playback*/
		layer_enable[DISP_OSD_LAYER1] =
			mgr->hw_playing_status[DISP_OSD_LAYER1];
		layer_enable[DISP_OSD_LAYER2] =
			mgr->hw_playing_status[DISP_OSD_LAYER2];

		if (is_tunnel)
			mgr->is_tunnel_playback = true;
		else
			mgr->is_tunnel_playback = false;
	} else if (config->user == DISP_USER_AEE) {
		memcpy((void *)layer_enable, (void *)mgr->hw_playing_status,
		       sizeof(bool) * DISP_LAYER_NUM);
		layer_enable[DISP_OSD_LAYER2] =
			config->buffer_info[0].layer_enable;
		config->buffer_info[0].layer_id = 1;
	}

	if (mgr->hdmi_plug_out) {
		layer_enable[DISP_VDP_LAYER1] =
			mgr->hw_playing_status[DISP_VDP_LAYER1];
		layer_enable[DISP_VDP_LAYER2] =
			mgr->hw_playing_status[DISP_VDP_LAYER2];
		cur_hdmi_hpd_st = true;
		// log hdmi plug out status
		if (cur_hdmi_hpd_st != last_hdmi_hpd_st) {
			DISP_LOG_I("hdmi plug out, drop video layer\n");
			last_hdmi_hpd_st = cur_hdmi_hpd_st;
		}
	} else {
		cur_hdmi_hpd_st = false;
		// log hdmi plug in status
		if (cur_hdmi_hpd_st != last_hdmi_hpd_st) {
			DISP_LOG_I("hdmi plug in, enable layer\n");
			last_hdmi_hpd_st = cur_hdmi_hpd_st;
		}
	}

	j = k = 0;

	/* play or stop vdp/osd */
	if (play_stop_en) { /* there is no any ui info if config from av sync */
		for (i = 0; i < DISP_LAYER_NUM; i++) {
			if (i < DISP_OSD_LAYER1) {
				drv = mgr->disp_hw_drv[DISP_MODULE_VDP];
				j = i;
			} else {
				drv = mgr->disp_hw_drv[DISP_MODULE_OSD];
				j = i - DISP_OSD_LAYER1;
			}

			if (!mgr->hw_playing_status[i] && layer_enable[i] &&
			    drv && drv->start) {
				DISP_LOG_I(
					"start() hw layer id: %d, layer id: %d\n",
					i, j);
				drv->start(&mgr->common_info, j);
			} else if (mgr->hw_playing_status[i] &&
				   !layer_enable[i] && drv && drv->stop) {
				DISP_LOG_I(
					"stop() hw layer id: %d, layer id: %d\n",
					i, j);
				drv->stop(j);
			}

			mgr->hw_playing_status[i] = layer_enable[i];
		}
	}

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.config[i];
		drv = mgr->disp_hw_drv[module];

		if (drv == NULL ||
		    (drv->config == NULL && drv->config_ex == NULL))
			continue;

		if (module == DISP_MODULE_VDP) {
			for (j = 0; j < DISP_BUFFER_MAX; j++) {
				if (drv->config == NULL)
					continue;
				if (config->buffer_info[j].layer_enable &&
				    (config->buffer_info[j].type ==
					     DISP_LAYER_VDP ||
				     config->buffer_info[j].type ==
					     DISP_LAYER_SIDEBAND))
					drv->config(&config->buffer_info[j],
						    &mgr->common_info);
			}
		} else if (module == DISP_MODULE_OSD) {
			if (drv->config_ex == NULL)
				continue;
			ret = drv->config_ex(config, &mgr->common_info);
		} else {
			for (j = 0; j < DISP_BUFFER_MAX; j++) {
				if (drv->config == NULL)
					continue;
				if (config->buffer_info[j]
					    .layer_enable) /* && */
					/*(config->buffer_info[j].type ==
					 * DISP_LAYER_VDP ||
					 * config->buffer_info[j].type ==
					 * DISP_LAYER_SIDEBAND))
					 */
					ret = drv->config(
						&config->buffer_info[j],
						&mgr->common_info);
			}
		}
	}

	/*after config every hw, it needs to*/
	/*wake up gce to trigger hw packet cmd*/
	#ifdef DISP_GCE_SUPPORT
	_disp_wakeup_gce_event(DISP_EVENT_GCE);
	#endif

	_disp_mutex_unlock();

	return ret;
}

/*
 * return value is the time spend, if fail return 0
 */
int disp_hw_mgr_wait_vsync(unsigned long long *ts)
{
	int ret = 0;

	if (IS_DISP_SUSPEND()) {
		usleep_range(16000, 17000);
		DISP_LOG_D("VSYNC DISP_SLEPT Return\n");
		return 0;
	}

	atomic_set(&mgr->wait_vsync_flag, 0);

	ret = (int)wait_event_timeout(
		mgr->wait_vsync_wq,
		DISP_EVENT_VSYNC & atomic_read(&mgr->wait_vsync_flag),
		msecs_to_jiffies(100));

	if (ret == 0) {
		/*DISP_LOG_E("wait vsync timeout(>HZ/2)\n");*/
		if (ts != NULL)
			*ts = ktime_to_ns(ktime_get());
	} else if (ts != NULL)
		*ts = mgr->vsync_ts;

	return ret;
}

int disp_hw_mgr_dump(uint32_t level)
{
	int i;
	struct disp_hw *drv;

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		drv = mgr->disp_hw_drv[i];
		if (drv && drv->dump)
			drv->dump(level);
	}

	return 0;
}

int disp_hw_mgr_send_event(enum DISP_EVENT event, void *data)
{
	_disp_event_callback(event, data);

	return 0;
}

int disp_hw_mgr_resume(void)
{
	int i, ret = 0;
	struct disp_hw *drv;
	enum DISP_MODULE_ENUM module;

	if (!IS_DISP_SUSPEND()) {
		DISP_LOG_D("the status is not suspend.\n");
		return 0;
	}

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.resume[i];
		drv = mgr->disp_hw_drv[module];
		if (drv && drv->resume)
			ret = drv->resume();
		if (ret != 0)
			break;
	}

	disp_path_reset();

	atomic_set(&mgr->status, DISP_STATUS_RESUME);

	return ret;
}

static void disp_hw_mgr_clear_disp_irq_before_suspend(void)
{
	uint32_t irq_st = 0;

	irq_st = disp_sys_read_irq_status();
	if (irq_st & DISP_IRQ_FMT_VSYNC_STATUS_ON) {
		DISP_LOG_I("it has not cleared fmt vsync irq =0x%X\n", irq_st);
		disp_irq_manager(DISP_IRQ_FMT_VSYNC);
		irq_st = disp_sys_read_irq_status();
		DISP_LOG_I("it has cleared fmt vsync irq done. =0x%X\n", irq_st);
	}
	if (irq_st & DISP_IRQ_FMT_ACTIVE_START_STATUS_ON) {
		DISP_LOG_I("it has not cleared fmt active start irq =0x%X\n", irq_st);
		disp_irq_manager(DISP_IRQ_FMT_ACTIVE_START);
		irq_st = disp_sys_read_irq_status();
		DISP_LOG_I("it has cleared fmt active star irq done. =0x%X\n", irq_st);
	}
	if (irq_st & DISP_IRQ_BEFIFO_STATUS_ON) {
		DISP_LOG_I("it has not cleared befifo irq =0x%X\n", irq_st);
		disp_irq_manager(DISP_IRQ_BEFIFO);
		irq_st = disp_sys_read_irq_status();
		DISP_LOG_I("it has cleared befifo irq done. =0x%X\n", irq_st);
	}
}

int disp_hw_mgr_suspend(void)
{
	int i, ret = 0;
	struct disp_hw *drv;
	enum DISP_MODULE_ENUM module;

	if (IS_DISP_SUSPEND()) {
		DISP_LOG_D("the status is suspend already.\n");
		return 0;
	}

	if (!disp_common_info.low_energy_dozing_mode_enable)
		disp_hw_mgr_clear_disp_irq_before_suspend();

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.suspend[i];
		drv = mgr->disp_hw_drv[module];
		if (drv && drv->suspend)
			ret = drv->suspend();
		if (ret != 0)
			break;
	}

	mgr->hw_playing_status[DISP_OSD_LAYER1] = false;
	mgr->hw_playing_status[DISP_OSD_LAYER2] = false;

	atomic_set(&mgr->status, DISP_STATUS_SUSPEND);

	return ret;
}

#ifdef CONFIG_HDMI_BLACK
int disp_hw_mgr_deep_suspend(void)
{
	int i, ret = 0;
	struct disp_hw *drv;
	enum DISP_MODULE_ENUM module;

	if (!disp_common_info.low_energy_dozing_mode_enable) {
		//when dozing mode disable, dislay has clk off on early suspend
		//but for dozing mode enable, display has clk off on deep suspend
		DISP_LOG_I("display has clk off when dozing mode disable.\n");
		return ret;
	}
	disp_hw_mgr_clear_disp_irq_before_suspend();
	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.deep_suspend[i];
		drv = mgr->disp_hw_drv[module];
		if (drv && drv->deep_suspend)
			ret = drv->deep_suspend();
		if (ret != 0)
			break;
	}

	return ret;
}
#endif

#ifdef CONFIG_HDMI_BLACK
int disp_hw_mgr_deep_resume(void)
{
	int i, ret = 0;
	struct disp_hw *drv;
	enum DISP_MODULE_ENUM module;

	if (!disp_common_info.low_energy_dozing_mode_enable) {
		//when dozing mode disable, dislay has clk off on early suspend
		//but for dozing mode enable, display has clk off on deep suspend
		DISP_LOG_I("display has clk off when dozing mode disable.\n");
		return ret;
	}

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.deep_resume[i];
		drv = mgr->disp_hw_drv[module];
		if (drv && drv->deep_resume)
			ret = drv->deep_resume();
		if (ret != 0)
			break;
	}

	disp_path_reset();

	return ret;
}
#endif

bool disp_hw_mgr_is_slept(void)
{
	return IS_DISP_SUSPEND();
}

void disp_hw_mgr_dma_buffer_release(struct tee_mmu *mmu)
{
	if (!mmu->dma_buf || !mmu->attach || !mmu->sgt) {
		DISP_LOG_E("invalid dma buffer\n");
		return;
	}
	dma_buf_unmap_attachment(mmu->attach,
		mmu->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(mmu->dma_buf, mmu->attach);
	dma_buf_put(mmu->dma_buf);

	mmu->dma_buf = NULL;
	mmu->attach = NULL;
	mmu->sgt = NULL;
}

int disp_hw_mgr_dma_fd_to_buffer(int fd, struct tee_mmu *mmu)
{
	int ret = 0;

	mmu->dma_buf = dma_buf_get(fd);
	if (IS_ERR(mmu->dma_buf)) {
		ret = (int)PTR_ERR(mmu->dma_buf);
		DISP_LOG_E("dma buf get failed, ret:%d\n", ret);
		return ret;
	}
	mmu->attach = dma_buf_attach(mmu->dma_buf, hw_mgr_dev);
	if (IS_ERR(mmu->attach)) {
		ret = (int)PTR_ERR(mmu->attach);
		dma_buf_put(mmu->dma_buf);
		DISP_LOG_E("dma buf attach failed, ret:%d\n", ret);
		return ret;
	}

	mmu->attach->dma_map_attrs |= DMA_ATTR_SKIP_CPU_SYNC;

	mmu->sgt =
		dma_buf_map_attachment(mmu->attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(mmu->sgt)) {
		ret = (int)PTR_ERR(mmu->sgt);
		DISP_LOG_E("dma buf map attachment failed, ret:%d\n", ret);
		dma_buf_detach(mmu->dma_buf, mmu->attach);
		dma_buf_put(mmu->dma_buf);
		return ret;
	}
	if (sg_page(mmu->sgt->sgl) == 0)	//it should be a secure buffer
		mmu->buffer_addr.sec_handle =
			(unsigned int) sg_dma_address(mmu->sgt->sgl);
	else	//it should be a non-secure ION buffer
		mmu->buffer_addr.dma_addr =
			(unsigned int) sg_dma_address(mmu->sgt->sgl);
	DISP_LOG_SYNC("dma fd to buffer, fd:%d, dma_add:0x%x\n",
			fd, mmu->buffer_addr.dma_addr);
	return ret;
}

int disp_hw_mgr_deinit(void)
{
	int i;
	struct disp_hw *drv;
	enum DISP_MODULE_ENUM module;

	if (IS_DISP_DEINIT()) {
		DISP_LOG_E("the status is deinited already\n");
		return 0;
	}

	for (i = 0; i < DISP_MODULE_NUM; i++) {
		module = mgr->sequence.init[i];
		drv = mgr->disp_hw_drv[module];
		if (drv && drv->deinit)
			drv->deinit();
	}

	atomic_set(&mgr->status, DISP_STATUS_DEINIT);

#ifdef DISP_GCE_SUPPORT
	cmdq_pkt_destroy(mgr->common_info.gce_handle->pkt);

	vfree(mgr->common_info.gce_handle);
#endif

#ifdef DISP_QMS_SUPPORT
	vfree(mgr->common_info.vrr_info);
#endif

	return 0;
}

int disp_hw_mgr_status(void)
{
	struct disp_hw_tv_capbility *tv_cap;
	struct disp_hw_tv_capbility tmp_tv_cap;
	const struct disp_hw_resolution *resolution;

	tv_cap = &mgr->common_info.tv;
	resolution = mgr->common_info.resolution;

	DISP_LOG_I("disp hw mgr status:\n");
	DISP_LOG_I("tv cap, hdr static %d dovi %d dovi_2160p60 %d\n",
		   tv_cap->is_support_hdr, tv_cap->is_support_dovi,
		   tv_cap->is_support_dovi_2160p60);

	DISP_LOG_I("resolution %d\n", resolution->res_mode);

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
	_get_hdmi_cap(&tmp_tv_cap);
#endif
	DISP_LOG_I("tmp tv cap, hdr static %d dovi %d dovi_2160p60 %d\n",
		   tmp_tv_cap.is_support_hdr, tmp_tv_cap.is_support_dovi,
		   tmp_tv_cap.is_support_dovi_2160p60);

	return 0;
}
