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

#ifndef __DISP_HDR_MAIN_H__
#define __DISP_HDR_MAIN_H__

#include "disp_hdr_if.h"

#define HDR_DRV_NAME  "disp_drv_hdr"

#define HDR_EVENT_VLAYER0_CFG_DONE (0x1 << 0)
#define HDR_EVENT_VLAYER1_CFG_DONE (0x1 << 1)
#define HDR_EVENT_GLAYER0_THREAD_WAKEUP (0x1 << 2)
#define HDR_EVENT_GLAYER1_THREAD_WAKEUP (0x1 << 3)

#define HDR_CFD_LAYER_MAIN_VID 0
#define HDR_CFD_LAYER_SUB_VID  1
#define HDR_CFD_LAYER_FHD_GFX  2
#define HDR_CFD_LAYER_UHD_GFX  3


extern struct video_buffer_info hdr_video_layer[V_G_LAYER_MAX];
extern struct mtk_disp_buffer hdr_osd_layer[V_G_LAYER_MAX];
extern bool bsub_exist;
extern bool dolby_force_output;
extern uint32_t g_force_dolby;
extern uint32_t g_force_hdr;
extern uint32_t g_ic_version;
extern uint32_t g_force_open_hdr;
extern uint32_t g_dovi_efuse;
extern uint32_t dovi_vs10_path_en;
extern uint32_t dovi_enable;
extern uint32_t dolby_path_ready2start;
extern unsigned int g_out_format;
extern enum HDMI_VIDEO_RESOLUTION old_resolution;
extern enum dovi_signal_format_t dolby_out_format;
extern struct disp_hw_common_info dovi_common_info;
extern bool tv_info_set_by_cmd;
extern bool hdr_fe_en[LAYER_MAX];
extern bool hdr_vdo_be_en;
extern enum hdr_output_type hdr_output_signal_type;
extern uint32_t hdr_vsync_cnt;
extern uint32_t disp_hdr_irq_event;
extern uint32_t adl_mode;
extern bool dovi_off_delay_needed;

void disp_hdr_set_event(uint32_t event);
void disp_hdr_wakeup_routine(uint32_t thread_id);
void vdp_fill_metadata_info(enum VID_PLA_DR_TYPE_T type,
	struct VID_PLA_HDR_METADATA_INFO_T *rHdr,
	void *buf_add, uint32_t len);
void vdp_config_hdmi_signal(enum HDR10_TYPE_ENUM hdr_type,
	struct VID_STATIC_HDMI_MD_T *metadata,
	bool enable);
int set_hdmi_signal(struct video_buffer_info *buf);
void pass_osd_buffer_info(struct mtk_disp_buffer *osd_disp_buffer,
			  struct disp_hw_common_info *common_info);
int disp_hdr_cmd(enum DISP_CMD cmd, void *data);

#endif
