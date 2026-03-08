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

#ifndef _DISP_VDP_VSYNC_H_
#define _DISP_VDP_VSYNC_H_
/*#include <mach/sync_write.h>*/
/*#include <mach/mt_typedefs.h>*/
#include "disp_vdp_if.h"
#include "disp_hdr_if.h"
#include "vdp_hal.h"
#include <linux/types.h>

enum vdp_time_rec {
	VDP_ROUTINE_START, /* 0 */
	VDP_FMT_SETTING,
	VDP_MIX_CONFIG,
	VDP_DISPFMT_IN,
	VDP_DISPFMT_OUT,
	VDP_SEC_IN,	/* 5 */
	VDP_SEC_IN_1,	/* 6*/
	VDP_SEC_IN_2,	/* 7 */
	VDP_SEC_IN_3,	/* 8 */
	VDP_SEC_IN_4,	/* 9 */
	VDP_SEC_IN_5,	/* 10 */
	VDP_SEC_IN_6,	/* 11 */
	VDP_SEC_OUT,	/* 12 */
	VDP_MAX_REC,
};
#define VDP_REC_BUF	100
#define VDOUT_LINE_CNT	(HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF)

extern uint32_t vdp_line_cnt[VDP_HW_NS][VDP_MAX_REC];

#define REC_VDP_LINE(_vdp_id, _rec_id)				\
do {								\
	if ((vdp_dbg_level & VDP_REC_LINE_LOG) &&		\
	    _vdp_id < VDP_HW_NS &&				\
	    _rec_id < VDP_MAX_REC) {				\
		vdp_line_cnt[_vdp_id][_rec_id] = VDOUT_LINE_CNT;	\
	}							\
} while (0)

extern uint32_t osd_enable;
extern struct video_layer_info *video_layer;
extern struct video_playback_info video_play_info;
extern struct disp_hw_common_info disp_common_info;
extern struct mtk_disp_hdr_md_info_t vdp_hdr_info[VDP_MAX];
extern uint32_t hdr_frame_num;
extern uint32_t idk_stop_frame_num;

#define NORMAL_PLAYBACK_DURATION 1
#define INTERLACE_PLAYBACK_DUARATION 3
#define DOVI_RES4K60_TMDS_RATE 590

enum METADATA_SET {
	METADATA_NORMAL = 0,
	METADATA_ONLY,
	METADATA_STORY,
	METADATA_UNKNOWN,
};

enum WAKE_UP_TYPE {
	WAKEUP_MAIN = 0,
	WAKEUP_SUB,
	WAKEUP_ALL,
	WAKEUP_UNKNOWN,
};

void disp_fg_config(u32 fg_hw_id, struct mtk_av1_film_grain_params *fg_param);
void disp_fg_config_scale_info(u32 hw_id, struct video_scale_info *scale_info);

void vdp_vsync_init(void);
void vdp_wakeup_routine(enum WAKE_UP_TYPE type);
void vdp_isr(void);
void vdp_disable_active_zone(struct video_layer_info *layer_info);
struct video_buffer_info *vdp_get_buf_info(enum VIDEO_LAYER_ID id);
void release_buf_info(struct list_head *list, unsigned int layer_id);
int vdp_check_dsd_available(unsigned char vdp_id,
			    struct vdp_hal_config_info *hal_config);
void vdp_update_dovi_path_delay(int dsd_en);

extern int dovi_core1_hal_is_support(void);
extern uint32_t dovi_path_en;
void dovi_path_disable_by_vs10(void);
void dovi_path_enable_by_vs10(void);
void mdp_print(char *pBuffer);
void vdp_dovi_set_out_format(struct disp_hw_common_info *info,
			     struct video_buffer_info *buf_main,
			     struct video_buffer_info *buf_sub);
void vdp_update_dovi_path_delay(int dsd_en);
int vdp_stop_disable_hw(unsigned int layer_id);
bool vdp_check_2160p60_timing(enum HDMI_VIDEO_RESOLUTION res_mode);
int vdp_metadata_async(void);
#ifdef DISP_GCE_SUPPORT
void vdp_prepare_GCE_command(struct cmdq_pkt *pkt_main,
	struct cmdq_pkt *pkt_sub);

#endif
extern struct video_layer_info *video_layer;
extern bool dovi_idk_dump;
extern uint32_t dovi_idk_disp_cnt;
extern uint32_t g_ic_version;
extern bool is_hd_resolution(void);
extern uint32_t idk_now_num[2];
extern uint32_t idk_stop_num;
extern uint32_t idk_vdp_num[2];
extern uint32_t idk_close_area;
extern uint32_t idk_no_drop;
extern uint32_t last_ion_fd[VIDEO_LAYER_MAX_COUNT];
extern void dovi_idk_settings(uint32_t idk_set);
extern UINT32 _u4VdpISRCount;
extern UINT32 _u4VdpISRCount2; /* VDP ISR count */
extern struct mtk_disp_vdp_cap vdp_scale_info[VIDEO_LAYER_MAX_COUNT];
#endif
