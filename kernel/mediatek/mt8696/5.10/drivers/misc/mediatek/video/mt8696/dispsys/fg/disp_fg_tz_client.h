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

#ifndef __DISP_FG_TZ_CLIENT_H__
#define __DISP_FG_TZ_CLIENT_H__

#include "disp_info.h"

#define BS_BUF_SIZE		1024

#define TZ_TA_FG_UUID		"96fa7b2e-62b2-28ef-64a8-12f9bcc84946"

enum FG_TZ_CALL_CMD {
	FG_TZ_CALL_CMD_SET_DEBUG_LEVEL,
	FG_TZ_CALL_CMD_FIND_FG_INFO_BUFFER,
	FG_TZ_CALL_CMD_MAX,
};

enum FG_TZ_CALL_DIR {
	FG_TZ_CALL_DIR_NONE = 0,
	FG_TZ_CALL_DIR_VALUE_INPUT = 1,
	FG_TZ_CALL_DIR_VALUE_OUTPUT = 2,
	FG_TZ_CALL_DIR_VALUE_INOUT = 3,
	FG_TZ_CALL_DIR_MEM_INPUT = 4,
	FG_TZ_CALL_DIR_MEM_OUTPUT = 5,
	FG_TZ_CALL_DIR_MEM_INOUT = 6,
	FG_TZ_CALL_DIR_MEMREF_INPUT = 7,
	FG_TZ_CALL_DIR_MEMREF_OUTPUT = 8,
	FG_TZ_CALL_DIR_MEMREF_INOUT = 9,
};

struct disp_fg_share_memory_info_t {
	uint32_t len;
	uint32_t sec_handle;
	uint32_t offset;
	uint8_t buff[BS_BUF_SIZE];
	/* out: control_path_test out dm info */
	uint32_t log_level;
};

int fg_sec_get_params(struct mtk_disp_film_grain_md_t *fg_info);
#endif				/* __DISP_DOVI_SEC_H__ */
