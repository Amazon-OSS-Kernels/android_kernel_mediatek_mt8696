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

#ifndef __DISP_DOVI_SEC_H__
#define __DISP_DOVI_SEC_H__

#include "disp_info.h"
#include "dovi_log.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"
#include "dovi_vdo_fe_hw.h"
#include "dovi_gfx_fe_hw.h"
#include "dovi_be_hw.h"
#include "disp_dovi_md_parser.h"

#define BS_BUF_SIZE          1024

#define TZ_TA_DOVI_UUID   "96fa7b2e-62b2-28ef-64a8-12f9bcc84940"




struct dovi_share_memory_info_t {
	enum DISP_DR_TYPE_T dr_type;	/* in: sdr/hdr10 md/dovi rpu stream */
	struct mtk_disp_hdr10_md_t hdr10_md;
	uint32_t frame_num;
	uint32_t rpu_bs_len;
	bool svp;
	uint32_t sec_handle;
	unsigned char rpu_bs_buffer[BITSTREAM_BUFFER_SIZE];
	int is_rbsp;
	struct cp_param_t cp_param;	/* in: control_path_test in param */
	uint32_t comp_md[DOVI_COMP_DW_SIZE];	/* out: out comp info */
	uint32_t orig_md[DOVI_MD_DW_SIZE];	/* out: out dm info */
	uint32_t hdmi_md[DOVI_MD_DW_SIZE];	/* out:  out dm info */
	uint32_t orig_md_len;
	uint32_t hdmi_md_len;
	struct mtk_disp_hdr10_md_t hdr10_info_frame;	/* out:  hdr10 md */
	uint32_t log_level;
	uint32_t sec_handle_in;
	uint32_t sec_handle_out;
	uint32_t len_tmp;
	uint32_t sec_handle_len;
	bool profile4;
	struct rpu_ext_config_fixpt_main_t dv_comp_md;
	struct dm_metadata_t dv_dm_md;
	struct dv_hw_reg dv_out_params;
};

extern struct dovi_share_memory_info_t *dovi_share_mem;
enum dovi_status dovi_sec_service_call(
	enum DOVI_TZ_CALL_CMD cmd,
	enum DOVI_TZ_CALL_DIR direct,
	void *buffer, uint32_t size);
enum dovi_status dovi_sec_share_mem_service_call(
	enum DOVI_TZ_CALL_CMD cmd,
	enum DOVI_TZ_CALL_DIR direct,
	void *buffer, uint32_t size);

enum dovi_status dovi_sec_share_memory_init(void);

enum dovi_status dovi_sec_md_parser_init(void);
enum dovi_status dovi_sec_md_parser_uninit(void);
enum dovi_status dovi_sec_cp_test_init(void);
enum dovi_status dovi_sec_find_rpu_buffer(uint32_t sec_handle,
	uint32_t len);
enum dovi_status dovi_sec_cp_test_main(void);
enum dovi_status dovi_sec_cp_test_uninit(void);
enum dovi_status dovi_sec_status(void);
enum dovi_status dovi_sec_debug_level_init(uint32_t dovi_tz_level);
enum dovi_status dovi_sec_handle_copy(uint32_t *sec_handle,
	uint32_t len);

#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) || defined(CONFIG_TRUSTY)
#define DOVI_TZ_OK 1
#else
#define DOVI_TZ_OK 1 //for unitest
#endif

#endif				/* __DISP_DOVI_SEC_H__ */
