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

#ifndef __DISP_CFD_MAIN_H__
#define __DISP_CFD_MAIN_H__

#include <linux/list.h>
#include <linux/mutex.h>
#include "disp_hw_mgr.h"
#include "hdmitx.h"
#include "disp_info.h"
#include "disp_cfd_hal.h"
#include "disp_vdp_if.h"
#include "disp_cfd_tz_client.h"

#define CFD_LAYER_VDO_FE0  (0)
#define CFD_LAYER_VDO_FE1  (1)
#define CFD_LAYER_GFX_FE0  (2)
#define CFD_LAYER_GFX_FE1  (3)
#define CFD_MAX_LAYER_NUM  (4)

#define CFD_CTRLBIT_HDR10PLUS_ASYNC       (1 << 0)
#define CFD_CTRLBIT_HDR10PLUS_GFX_OVERLAY (1 << 1)


enum DISP_CFD_STATE_E {
	DISP_CFD_STATE_UNINIT,
	DISP_CFD_STATE_INITED,
	DISP_CFD_STATE_START,
	DISP_CFD_STATE_STOPING,
	DISP_CFD_STATE_STOPPED,
	DISP_CFD_STATE_UNKNOWN
};

enum DISP_CFD_FEATURE_E {
	DISP_CFD_BYPASS = 0,
	DISP_CFD_HDR2HDR = 1,
	DISP_CFD_HDR2SDR = 3,
	DISP_CFD_HDR2HLG = 4,
	DISP_CFD_SDR2HDR = 5,
	DISP_CFD_HLG2HLG = 6,
	DISP_CFD_HLG2SDR = 7,
	DISP_CFD_HLG2HDR = 8,
	DISP_CFD_SDR2HLG = 9,
	DISP_CFD_HDR10PLUS2HDR10PLUS = 10,
	DISP_CFD_HDR10PLUS2HDR10 = 11,
	DISP_CFD_HDR10PLUS2SDR = 12,
	DISP_CFD_HDR10PLUS2VSIF = 13,
	DISP_CFD_R2Y = 14,
	DISP_CFD_BYPASS_SDR2020 = 15,
	DISP_CFD_SDR2SDR_2020 = 16,
	DISP_CFD_HDR2SDR_2020 = 17,
	DISP_CFD_HLG2SDR_2020 = 18,
	DISP_CFD_SDR2020_2R709 = 19,
	DISP_CFD_OUTOF_HANDLE = 20,
	DISP_CFD_FEATURE_MAX
};

struct disp_cfd_context {
	uintptr_t cfd_hw_base;
	uintptr_t cfd_hw_base_ext;
	uint32_t io_reg_base;
	uint32_t io_reg_base_ext;
	struct mutex lock;
	bool inited;
	bool enabled;
	bool force_hdr;
	bool src_bt2020;
	uint32_t feature_ctrlbit;
	uint32_t reg_conf_mode;
	uint32_t layer_id;
	uint32_t src_width;
	uint32_t src_heght;
	uint32_t alpha;
	uint32_t hdr_type;
	uint32_t frame_no;
	enum DISP_CFD_STATE_E cfd_state;
	enum DISP_CFD_FEATURE_E feature_type;
	enum HDMI_VIDEO_RESOLUTION res;
	struct CFD_Register_Table *preg_tbl;
	struct cmdqRecStruct *gce_handle;
};

int disp_cfd_tz_dbg_lvl_enable(uint32_t level, uint32_t enable);
int disp_cfd_dbg_lvl_enable(uint32_t level, uint32_t enable);
int disp_cfd_init(struct disp_hw_common_info *info);
int disp_cfd_deinit(void);
int disp_cfd_set_cmd(uint32_t layer_id, enum DISP_CMD cmd, void *data);
int disp_cfd_config_video_frame(uint32_t layer_id,
	struct video_buffer_info *buf_info,
	struct disp_hw_tv_capbility *tv_cap);
int disp_cfd_config_graphic_frame(uint32_t layer_id,
	struct mtk_disp_buffer *buf_info,
	struct disp_hw_tv_capbility *tv_cap);
int disp_cfd_set_test_case(uint32_t layer_id, uint32_t test_no);
int disp_cfd_backup_sec_handle(
	struct mtk_vdp_hdr10_plus_svp_handle_t *hdr10_plus_svp_handle);
int disp_cfd_parser_vsif_in_sec(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t sec_len);
uint8_t disp_cfd_parser_t35_metadata(uint8_t *hdr_metadata,
	uint32_t size,
	struct STU_CFD_HDR10PLUS_SEI *hdr10plus_sei);
int disp_cfd_parser_t35_emp_in_sec(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t sec_len);
int disp_cfd_set_tz_init(uint32_t fg_enable);
int disp_cfd_set_conf_mode(uint32_t layer_id, uint32_t mode);
void disp_cfd_set_flush_conf(uint32_t layer_id, uint32_t conf_mode);
void disp_cfd_drv_read_reg(uint32_t client, uint32_t rel_offset,
	uint32_t size);
void disp_cfd_drv_write_reg(uint32_t client, uint32_t rel_offset,
	uint32_t value);
int disp_cfd_drv_set_bypass(uint32_t layer_id);
int disp_cfd_drv_set_r2y(uint32_t layer_id, uint32_t color_mode);
struct disp_cfd_context *cfd_get_inst(uint32_t layer_id);
int disp_cfd_set_clk_enable(uint32_t layer_id, uint32_t enable);
void disp_cfd_set_sof(uint32_t layer_id);
int disp_cfd_set_gce_handle(uint32_t layer_id,
	void *pv_handle);
int disp_cfd_start(struct disp_hw_common_info *info,
	unsigned int layer_id);
int disp_cfd_stop(unsigned int layer_id);
int disp_cfd_suspend(unsigned int layer_id);
int disp_cfd_resume(unsigned int layer_id);
int disp_cfd_drv_get_hdr10_metadata(
	struct VID_PLA_HDR_METADATA_INFO_T *rHdr,
	uint32_t id);
int disp_cfd_drv_get_hdmi_output_format(uint32_t layer_id,
	uint32_t *out_format, bool *bt2020_enable);
int disp_cfd_drv_fill_dyn_metadata(enum VID_PLA_DR_TYPE_T type,
	struct VID_PLA_HDR_METADATA_INFO_T *rHdr);
int disp_cfd_enable(unsigned int layer_id, bool enable);
int disp_cfd_drv_set_forcehdr(uint32_t layer_id,
	bool force_openhdr);
int disp_cfd_drv_set_user_gfx_overlay(uint32_t layer_id,
	bool gfx_overlay);
int disp_cfd_drv_set_frame_async(uint32_t layer_id,
	bool md_async);
bool disp_cfd_drv_get_frame_async(uint32_t layer_id);
int disp_cfd_drv_dump_status(uint32_t layer_id);
int disp_cfd_drv_set_feature_ctrlbit(uint32_t layer_id,
	uint32_t ctrl_bit);
int disp_cfd_drv_set_feature_type(uint32_t layer_id,
	enum DISP_CFD_FEATURE_E feture_type);
int disp_cfd_chg_output(struct video_buffer_info *buf_info,
	const struct disp_hw_resolution *info,
	struct disp_hw_tv_capbility *tv_cap);
int disp_cfd_drv_decide_video_feature(uint32_t layer_id,
	struct video_buffer_info *buf_info,
	struct disp_hw_tv_capbility *tv_cap);
int disp_cfd_drv_get_video_dm_wh(uint32_t layer_id,
	uint32_t *dm_w, uint32_t *dm_h);
int disp_cfd_drv_get_gfx_dm_h(uint32_t layer_id,
	uint32_t *dm_h);
int disp_cfd_drv_set_video_wh(uint32_t layer_id,
	uint32_t width, uint32_t height);
int disp_cfd_drv_set_gfx_h(uint32_t layer_id,
	uint32_t height);
#endif
