/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/slab.h>
#include <linux/types.h>
#include "disp_info.h"

#include <kree/mem.h>
#include <kree/system.h>
#include <tz_cross/ta_mem.h>
#include <tz_cross/trustzone.h>
#include "tz_cross/ta_mem.h"

#include "disp_fg_tz_client.h"
#include "disp_fg_if.h"

struct disp_fg_share_memory_info_t *fg_share_mem;

static KREE_SESSION_HANDLE fg_tz_session;
static KREE_SESSION_HANDLE fg_tz_mem_session;
static KREE_SHAREDMEM_HANDLE fg_tz_mem_handle;

static bool fg_sec_inited;

static int fg_sec_create_session(void)
{
	int ret = TZ_RESULT_SUCCESS;

	if (fg_tz_session == 0)
		ret = KREE_CreateSession(TZ_TA_FG_UUID, &fg_tz_session);

	if (ret != TZ_RESULT_SUCCESS)
		FG_ERR("create session fail:%d\n", ret);
	else
		FG_LOG_I("create session fg_tz_session[0x%X]\n", fg_tz_session);

	return ret;
}

static int fg_sec_create_share_memory(void)
{
	uint32_t size = sizeof(struct disp_fg_share_memory_info_t);

	if (fg_share_mem) {
		FG_LOG_I("share memory already created %p size %u\n",
			fg_share_mem, size);
		return 0;
	}

	fg_share_mem = kmalloc(size, GFP_KERNEL);
	if (!fg_share_mem) {
		FG_ERR("share memory alloc fail %p size %u\n",
			fg_share_mem, size);
		return -1;
	}

	FG_LOG_I("share memory create 0x%p size %u\n", fg_share_mem, size);

	memset((void *)fg_share_mem, 0, size);

	return 0;
}

static int fg_sec_create_share_mem_session(void)
{
	int ret = TZ_RESULT_SUCCESS;

	if (fg_tz_mem_session == 0)
		ret = KREE_CreateSession(TZ_TA_MEM_UUID, &fg_tz_mem_session);

	if (ret != TZ_RESULT_SUCCESS)
		FG_ERR("create share mem session fail:%d\n", ret);
	else
		FG_LOG_I("create fg_tz_mem_session[0x%X]\n",
			 fg_tz_mem_session);

	return ret;
}

static int fg_sec_create_share_mem_handle(void)
{
	struct KREE_SHAREDMEM_PARAM fg_param;
	int ret = 0;

	if (!fg_share_mem) {
		FG_ERR("share memory not created %p yet!!\n",
			fg_share_mem);
		return -1;
	}

	fg_param.buffer = (void *)fg_share_mem;
	fg_param.size = sizeof(struct disp_fg_share_memory_info_t);

	if (fg_tz_mem_handle == 0)
		ret = KREE_RegisterSharedmem(fg_tz_mem_session, &fg_tz_mem_handle, &fg_param);

	if (ret != TZ_RESULT_SUCCESS)
		FG_ERR("create fg_tz_mem_handle fail:%d\n", ret);
	else
		FG_LOG_I("create fg_tz_mem_handle[0x%X] pointer 0x%p\n",
			 fg_tz_mem_handle,
			 fg_share_mem);

	return ret;
}

static int fg_sec_map_direction(enum FG_TZ_CALL_DIR direct, enum TZ_PARAM_TYPES *pType)
{
	int ret = 0;

	switch (direct) {
	case FG_TZ_CALL_DIR_MEM_INPUT:
		*pType = TZPT_MEM_INPUT;
		break;
	case FG_TZ_CALL_DIR_MEM_OUTPUT:
		*pType = TZPT_MEM_OUTPUT;
		break;
	case FG_TZ_CALL_DIR_MEM_INOUT:
		*pType = TZPT_MEM_INOUT;
		break;
	case FG_TZ_CALL_DIR_MEMREF_INPUT:
		*pType = TZPT_MEMREF_INPUT;
		break;
	case FG_TZ_CALL_DIR_MEMREF_OUTPUT:
		*pType = TZPT_MEMREF_OUTPUT;
		break;
	case FG_TZ_CALL_DIR_MEMREF_INOUT:
		*pType = TZPT_MEMREF_INOUT;
		break;
	case FG_TZ_CALL_DIR_VALUE_INPUT:
		*pType = TZPT_VALUE_INPUT;
		break;
	case FG_TZ_CALL_DIR_VALUE_OUTPUT:
		*pType = TZPT_VALUE_OUTPUT;
		break;
	case FG_TZ_CALL_DIR_VALUE_INOUT:
		*pType = TZPT_VALUE_INOUT;
		break;
	case FG_TZ_CALL_DIR_NONE:
		*pType = TZPT_NONE;
		break;
	default:
		FG_ERR("invalid call direction:%d\n", direct);
		ret = -1;
	}

	return ret;
}

static int fg_sec_share_mem_service_call(enum FG_TZ_CALL_CMD cmd, enum FG_TZ_CALL_DIR direct,
					 void *buffer, uint32_t size)
{
	uint32_t paramTypes = 0;
	union MTEEC_PARAM fg_param[4];
	enum TZ_PARAM_TYPES type;
	int ret = 0;

	if (fg_tz_session == 0) {
		FG_ERR("fg_tz_session not created\n");
		return -1;
	}

	ret = fg_sec_map_direction(direct, &type);
	if (ret)
		return ret;

	paramTypes = TZ_ParamTypes1(type);

	fg_param[0].memref.handle = (uint32_t)(*(uint32_t *)buffer);
	fg_param[0].memref.offset = 0;
	fg_param[0].memref.size = size;

	FG_LOG_I("cmd %d buf 0x%p size %u handle 0x%X\n",
		  cmd, buffer, size,
		  fg_param[0].memref.handle);

	ret = KREE_TeeServiceCall(fg_tz_session, cmd, paramTypes, fg_param);
	if (ret != TZ_RESULT_SUCCESS)
		FG_ERR("call fail: cmd[%d] ret[0x%X]\n", cmd, ret);

	return ret;
}

void fg_sec_status(void)
{
	uint32_t share_size = sizeof(struct disp_fg_share_memory_info_t);

	FG_LOG_I("sec session 0x%X,session 0x%X, handle 0x%X\n",
		 fg_tz_session,
		 fg_tz_mem_session,
		 fg_tz_mem_handle);

	FG_LOG_I("share mem %p size %d fg_sec_inited %d\n",
		fg_share_mem, share_size,
		fg_sec_inited);
}

int fg_sec_set_debug_level(uint32_t log_level)
{
	int ret = 0;
	uint32_t size = sizeof(struct disp_fg_share_memory_info_t);
	char *p_fg_share_mem_handle =
		(char *)&fg_tz_mem_handle;

	fg_share_mem->log_level = log_level;
	ret = fg_sec_share_mem_service_call(
		FG_TZ_CALL_CMD_SET_DEBUG_LEVEL,
		FG_TZ_CALL_DIR_MEMREF_INPUT,
		(void *)p_fg_share_mem_handle, size);

	if (ret)
		FG_ERR("init debug level init fail [%d]\n",
		ret);

	return ret;
}

int fg_sec_get_params(struct mtk_disp_film_grain_md_t *fg_info)
{
	int ret = 0;
	uint32_t size = sizeof(struct disp_fg_share_memory_info_t);
	char *p_fg_share_mem_handle = (char *)&fg_tz_mem_handle;

	if (!fg_share_mem) {
		FG_ERR("fg_share_mem not created\n");
		return -1;
	}

	fg_share_mem->sec_handle = fg_info->sec_handle;
	fg_share_mem->len = fg_info->len;
	fg_share_mem->offset = fg_info->offset;

	ret = fg_sec_share_mem_service_call(
		FG_TZ_CALL_CMD_FIND_FG_INFO_BUFFER,
		FG_TZ_CALL_DIR_MEMREF_INOUT,
		(void *)p_fg_share_mem_handle, size);

	if (ret)
		FG_ERR("%s fail[%d]\n", __func__, ret);

	fg_info->addr = &fg_share_mem->buff[0];

	return ret;
}

int fg_sec_init(void)
{
	int ret;

	if (fg_sec_inited)
		return 0;

	ret = fg_sec_create_session();
	if (ret) {
		FG_ERR("fg create tz session fail!\n");
		return ret;
	}

	ret = fg_sec_create_share_memory();
	if (ret) {
		FG_ERR("fg create tz share mememory fail!\n");
		return ret;
	}

	ret = fg_sec_create_share_mem_session();
	if (ret) {
		FG_ERR("fg create tz share memory session fail!\n");
		return ret;
	}

	ret = fg_sec_create_share_mem_handle();
	if (ret) {
		FG_ERR("fg create tz share memory handle fail!\n");
		return ret;
	}

	fg_sec_inited = true;

	fg_sec_status();

	return ret;
}
