/*
 * Copyright (C) 2020 MediaTek Inc.
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

#ifndef __DISP_HDR_IF_H__
#define __DISP_HDR_IF_H__

#include "disp_vdp_if.h"
#include "disp_hw_mgr.h"
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
#include "hdmitx.h"
#endif

#define V_G_LAYER_MAX (2)
enum hdr_output_type {
	HDR_OUT_TYPE_DV_STD = 0,
	HDR_OUT_TYPE_HDR10 = 1,
	HDR_OUT_TYPE_SDR = 2,
	HDR_OUT_TYPE_SDR_2020 = 3,
	HDR_OUT_TYPE_HLG = 4,
	HDR_OUT_TYPE_DV_LL = 5,
	HDR_OUT_TYPE_HDR10PLUS = 6,
	HDR_OUT_TYPE_HDR10PLUS_VSIF = 7,
	HDR_OUT_TYPE_MAX
};

enum LAYER_ID {
	LAYER0 = 0,
	LAYER1 = 1,
	LAYER2 = 2,
	LAYER3 = 3,
	LAYER_MAX
};

enum HDR_IRQ_EVENT_ID {
	CLK_CHG_FE0 = 0,
	CLK_CHG_FE1 = 1,
	CLK_CHG_FE2 = 2,
	CLK_CHG_FE3 = 3,
	CLK_CHG_BE = 4,
	FLAG_CHG = 5,
	FORCE_HDR_CHG = 6,
	HDR_IRQ_EVENT_MAX
};

enum HDR_PATH {
	DEFAULT_PATH = 0,
	DOVI_PATH = 1,
	OPENHDR_PATH = 2,
};

#define HDR_ReadREG(arg) __raw_readl((unsigned long *)(arg))


extern bool dovi_idk_dump;
extern int32_t idk_dump_vsync_cnt;
extern bool dovi_idk_dump_set_vin;
extern struct mtk_disp_hdr_md_info_t dovi_hdr_md_info[V_G_LAYER_MAX];
extern enum dovi_signal_format_t dolby_out_format;
extern struct mutex disp_hdr_mutex;
extern uint32_t disp_hdr_event;
extern uint32_t dolby_path_ready2start;
extern uint32_t dolby_path_enable;
extern uint32_t vdp_start_st[V_G_LAYER_MAX];
extern char *vdout_reg_base;
extern uint32_t osd_enable;
extern bool hdr_init_done;
extern uint32_t befifo_irq_cnt;
extern bool dovi_black_en_bycmd;
extern uint32_t dovi_black_cnt_bycmd;
extern uint32_t hdr_res_width;
extern uint32_t hdr_res_height;

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
extern void vVdpSetHdrMetadata(bool enable,
struct VID_PLA_HDR_METADATA_INFO_T hdr_metadata);
extern void vHdrEnable(bool fgEnable);
extern void vBT2020Enable(bool fgEnable);
extern void vDolbyHdrEnable(bool fgEnable);
extern void vLowLatencyDolbyVisionEnable(bool fgEnable);
extern void vHdr10PlusEnable(bool fgEnable);
extern void vSetStaticHdrType(char bType);
extern void vHdr10PlusVSIFEnable(bool fgEnable,
	unsigned int forcedHdrType, int dolbyOpFmt);
void hdr_set_HDMI_BT2020_signal(bool enable_bt2020);
#endif
extern int32_t disp_mix_hal_set_black_pattern(bool en);

int disp_hdr_config_video_info(struct video_buffer_info *buf,
	bool sub_exit);
void disp_hdr_config_video_non(void);
int disp_hdr_config_osd_info(struct mtk_disp_buffer *video_disp_buf);
int disp_hdr_config_osd_info_fake(uint32_t layer_id);
void disp_hdr_vsync_handle(uint32_t i, uint32_t vsync);
bool hdr10_plus_get_frame_delay_flag(void);
void hdr10_plus_set_graphic_overlay_flag(uint32_t id,
	bool gfx_overlay_flag_chg);
void set_hdr_path_black_unblack(uint32_t id, bool black);

int disp_hdr_handle_forcehdr(enum DISP_CMD cmd, void *data);
int disp_hdr_path_judge(void);
int disp_hdr_handle_vdp_start(enum DISP_CMD cmd, void *data);
int disp_hdr_handle_vdp_stop(enum DISP_CMD cmd, void *data);
int disp_hdr_handle_osd_start(enum DISP_CMD cmd, void *data);
int disp_hdr_handle_osd_stop(enum DISP_CMD cmd, void *data);
int disp_hdr_fe_start_stop(uint32_t layer_id, bool en);
int disp_hdr_vdo_be_start_stop(bool en);
int disp_dovi_hdr_save_sec_rpu(struct mtk_disp_dovi_md_t *dolby_info,
	struct mtk_vdp_dovi_md_t *dovi_md_info);
int disp_hdr_backup_hdr10plus_sec_handle(
	struct mtk_vdp_hdr10_plus_svp_handle_t *sec_info);
struct video_buffer_info *disp_hdr_get_vid_info(uint32_t id);
struct mtk_disp_buffer *disp_hdr_get_gfx_info(uint32_t id);
void disp_set_hdr_fe_input_size(uint32_t layer_id,
	uint32_t u4width, uint32_t u4height);
void disp_hdr_config_hdmi_signal(uint32_t path);
void disp_hdr_config_hdmi_signal_delay(uint32_t path);
void disp_hdr_path_ctl(uint32_t id, uint32_t en, uint32_t type);
void disp_hdr_irq_handle(void);
int disp_hdr_fe_set_clk(uint32_t layer_id, bool en);
int disp_hdr_set_vdo_be_clk(bool en);

#endif
