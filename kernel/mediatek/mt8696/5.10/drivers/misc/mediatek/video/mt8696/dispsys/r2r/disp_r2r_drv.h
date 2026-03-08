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

#ifndef __DISP_R2R_DRV_H__
#define __DISP_R2R_DRV_H__

#include "disp_info.h"

#include <linux/list.h>
#include <linux/mutex.h>
#include "disp_r2r_hal.h"
#include "disp_hw_mgr.h"
#include "hdmitx.h"

enum disp_r2r_status_e {
	R2R_STATUS_STOPPED,
	R2R_STATUS_STARTED,
	R2R_STATUS_WAIT_BUF,
	R2R_STATUS_PROCCING,
	R2R_STATUS_MAX
};

struct disp_r2r_context {
	uintptr_t r2r_hw_base;
	uint32_t io_reg_base;
	struct mutex lock;
	bool inited;
	uint32_t irq_no;
	uint32_t reg_conf_mode;
	enum HDMI_VIDEO_RESOLUTION res;
	struct R2R_Register_Table *preg_tbl;
	struct cmdqRecStruct *gce_handle;
	enum disp_r2r_status_e r2r_state;
};

enum R2R_DATA_COLOR_FORMAT {
	DISP_R2R_COLOR_FORMAT_UNKNOWN = 0,
	DISP_R2R_COLOR_FORMAT_RGB888 = 1,
	DISP_R2R_COLOR_FORMAT_BGR888 = 2,
	DISP_R2R_COLOR_FORMAT_YUV422 = 3,
	DISP_R2R_COLOR_FORMAT_YUV444 = 4,
	DISP_R2R_COLOR_FORMAT_MAX,
};

enum DATA_BIT_DEPTH {
	DEPTH_8_BIT = 0,
	DEPTH_10_BIT = 1,
	DEPTH_12_BIT = 2,
	DEPTH_BIT_MAX,
};

enum HDR_TYPE_ENUM {
	HDR_TYPE_NONE = 0,
	HDR_TYPE_ST2084 = 1,
	HDR_TYPE_HLG = 2,
	HDR_TYPE_DOVI = 3,
	HDR_TYPE_PLUS = 4,
	HDR_TYPE_MAX,
};

enum DOVI_SUB_TYPE_ENUM {
	DOVI_SUB_TYPE_NONE = 0,
	DOVI_SUB_TYPE_STD = 1,
	DOVI_SUB_TYPE_LL = 2,
	DOVI_SUB_TYPE_VSEM = 3,
	DOVI_SUB_TYPE_MAX,
};

enum R2R_BUFFER_STATE {
	BUFFER_CREATE,
	BUFFER_INSERT,
	BUFFER_REG_CONFIGED,
	BUFFER_REG_UPDATED,
	BUFFER_READ_DONE,
	BUFFER_DROPPED
};

struct r2r_src_range {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
};

struct r2r_video_buffer_info {
	struct list_head list;
	unsigned int layer_id;
	unsigned int layer_enable;
	bool secruity_en;
	enum R2R_BUFFER_STATE buf_state;
	unsigned int r2r_yaddr;
	unsigned int r2r_caddr;
	unsigned int r2r_ccaddr;
	struct ion_handle *ion_hndy;
	struct ion_handle *ion_hndc;
	struct ion_handle *ion_hndcc;
	int ion_fd;
	int acquire_fence_fd;
	int release_fence_fd;
	int current_fence_index;
	uint32_t src_duration;
	enum R2R_DATA_COLOR_FORMAT src_fmt;
	struct r2r_src_range src;

	bool is_interlace;
	enum DATA_BIT_DEPTH bit_depth;
	enum DISP_DR_TYPE_T hdr_type;
	enum DOVI_SUB_TYPE_ENUM dovi_sub_type;
	bool is_bt2020;
	uint32_t color_primaries;
	uint32_t transfer_characteristics;
	uint32_t matrix_coeffs;
	bool is_seamless;
	bool is_jumpmode;
	uint64_t pts;
	unsigned int fps;

	unsigned int meta_data_size;
	void *meta_data;
	struct VID_STATIC_HDMI_MD_T hdr10_info;
	struct mtk_disp_hdr_md_info_t hdr_info;
	struct disp_hw_common_info common_info;
};

void disp_r2r_drv_init(uintptr_t reg_base);
void disp_r2r_drv_uninit(void);
int disp_r2r_set_clk_enable(uint32_t enable);
int disp_r2r_map_timing(enum HDMI_VIDEO_RESOLUTION res,
	struct disp_r2r_timing *pst_timing);
void disp_r2r_drv_set_resolution(uint32_t hw_id,
	enum HDMI_VIDEO_RESOLUTION res);
void disp_r2r_drv_enable(bool fgEn, uint32_t mode);
void disp_r2r_drv_clr_irq(uint32_t irq);
void disp_r2r_drv_flush_reg(uint32_t mode);
void disp_r2r_drv_config_frame(struct r2r_video_buffer_info *buf_info);
int disp_r2r_dbg_level_enable(uint32_t level, uint32_t enable);
void disp_r2r_drv_set_pattern(uint32_t res_id, uint32_t enable,
	uint32_t y_data, uint32_t cb_data, uint32_t cr_data);
void disp_r2r_drv_path_sel(bool fg_on);
int disp_r2r_drv_set_conf_mode(uint32_t mode);
void disp_r2r_drv_set_sof(void);
struct r2r_video_buffer_info *r2r_get_buf_info(void);
void r2r_release_buf_info(struct list_head *list);
void disp_r2r_drv_get_resolution(uint32_t hw_id,
	enum HDMI_VIDEO_RESOLUTION *pe_res);
void disp_r2r_drv_read_reg(uint32_t rel_offset, uint32_t size);
void disp_r2r_drv_write_reg(uint32_t rel_offset, uint32_t value);
struct disp_r2r_context *r2r_get_inst(void);
int disp_r2r_drv_set_gce_handle(void *pv_handle);
int disp_r2r_drv_gce_trigger(void);
int disp_r2r_drv_set_test(uint32_t test_no);


#endif
