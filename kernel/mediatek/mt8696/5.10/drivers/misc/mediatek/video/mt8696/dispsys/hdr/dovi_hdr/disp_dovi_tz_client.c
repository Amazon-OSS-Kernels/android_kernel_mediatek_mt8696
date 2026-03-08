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

#define LOG_TAG "DOVI_TZ_CLIENT"

#include <linux/slab.h>
#include <linux/types.h>
#include "disp_info.h"
#include "dovi_log.h"
#include "disp_dovi_md_parser.h"
#include "dovi_common_hal.h"
#include "disp_dovi_tz_client.h"


struct dovi_share_memory_info_t *dovi_share_mem;

#if DOVI_TZ_OK
#include <kree/mem.h>
#include <kree/system.h>
#include <tz_cross/ta_mem.h>
#include <tz_cross/trustzone.h>
#include "tz_cross/ta_mem.h"



static KREE_SESSION_HANDLE dovi_tz_session;
static KREE_SESSION_HANDLE dovi_tz_mem_session;
static KREE_SHAREDMEM_HANDLE dovi_tz_mem_handle;

static bool dovi_sec_share_mem_inited;
static bool dovi_sec_md_parser_inited;
static bool dovi_sec_cp_test_inited;

static enum dovi_status dovi_sec_create_session(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	int ret;

	do {
		if (dovi_tz_session == 0) {
			ret = KREE_CreateSession(TZ_TA_DOVI_UUID,
				&dovi_tz_session);
			if (ret != TZ_RESULT_SUCCESS) {
				dovi_error("create tz_session fail:%d\n",
					ret);
				status = DOVI_STATUS_ERROR;
				break;
			}
		}
	} while (0);

	if (status != DOVI_STATUS_OK)
		dovi_error("create session fail:%d\n", status);
	else
		dovi_printf("create session dovi_tz_session[0x%X]\n",
		dovi_tz_session);

	return status;
}

static enum dovi_status dovi_sec_create_share_memory(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t size = sizeof(struct dovi_share_memory_info_t);

	if (dovi_share_mem) {
		dovi_error("share memory already created %p size %u\n",
			dovi_share_mem, size);
		return DOVI_STATUS_OK;
	}

	dovi_share_mem = kmalloc(size, GFP_KERNEL);
	if (!dovi_share_mem) {
		dovi_error("share memory already fail %p size %u\n",
			dovi_share_mem, size);
		return DOVI_STATUS_OK;
	}

	dovi_printf("share memory create 0x%p size %u\n",
		dovi_share_mem, size);

	memset((void *)dovi_share_mem, 0, size);

	if (status != DOVI_STATUS_OK)
		dovi_error("create share memory fail size %d 0x%p\n",
		size, dovi_share_mem);
	else
		dovi_info("create share memory ok size %d 0x%p\n",
		size, dovi_share_mem);

	return DOVI_STATUS_OK;
}

enum dovi_status dovi_sec_create_share_mem_session(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	int ret;

	do {
		if (dovi_tz_mem_session == 0) {
			ret = KREE_CreateSession(TZ_TA_MEM_UUID,
				&dovi_tz_mem_session);
			if (ret != TZ_RESULT_SUCCESS) {
				dovi_error("createtz_mem_session fail:%d\n",
					ret);
				status = DOVI_STATUS_ERROR;
				break;
			}
		}
	} while (0);

	if (status != DOVI_STATUS_OK)
		dovi_error("create session fail:%d\n", status);
	else
		dovi_printf("create session dovi_tz_mem_session[0x%X]\n",
		dovi_tz_mem_session);

	return status;
}

enum dovi_status dovi_sec_create_share_mem_handle(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	struct KREE_SHAREDMEM_PARAM dovi_param;
	int ret;

	dovi_param.buffer = (void *)dovi_share_mem;
	dovi_param.size = sizeof(struct dovi_share_memory_info_t);

	do {
		if (dovi_tz_mem_handle == 0) {
			ret = KREE_RegisterSharedmem(dovi_tz_mem_session,
				&dovi_tz_mem_handle, &dovi_param);
			if (ret != TZ_RESULT_SUCCESS) {
				status = DOVI_STATUS_ERROR;
				break;
			}
		}
	} while (0);

	if (status != DOVI_STATUS_OK)
		dovi_error("create dv_tz_mem_hdl fail:%d\n",
		status);
	else {
		dovi_printf(
			"create dv_tz_mem_hdl[0x%X] pointer 0x%p\n",
			dovi_tz_mem_handle,
			(void *)dovi_share_mem);
	}

	return status;
}

static enum dovi_status dovi_sec_map_direction(
	enum DOVI_TZ_CALL_DIR direct,
	enum TZ_PARAM_TYPES *pType)
{
	switch (direct) {
	case DOVI_TZ_CALL_DIR_MEM_INPUT:
		*pType = TZPT_MEM_INPUT;
		break;
	case DOVI_TZ_CALL_DIR_MEM_OUTPUT:
		*pType = TZPT_MEM_OUTPUT;
		break;
	case DOVI_TZ_CALL_DIR_MEM_INOUT:
		*pType = TZPT_MEM_INOUT;
		break;
	case DOVI_TZ_CALL_DIR_MEMREF_INPUT:
		*pType = TZPT_MEMREF_INPUT;
		break;
	case DOVI_TZ_CALL_DIR_MEMREF_OUTPUT:
		*pType = TZPT_MEMREF_OUTPUT;
		break;
	case DOVI_TZ_CALL_DIR_MEMREF_INOUT:
		*pType = TZPT_MEMREF_INOUT;
		break;
	case DOVI_TZ_CALL_DIR_VALUE_INPUT:
		*pType = TZPT_VALUE_INPUT;
		break;
	case DOVI_TZ_CALL_DIR_VALUE_OUTPUT:
		*pType = TZPT_VALUE_OUTPUT;
		break;
	case DOVI_TZ_CALL_DIR_VALUE_INOUT:
		*pType = TZPT_VALUE_INOUT;
		break;
	case DOVI_TZ_CALL_DIR_NONE:
		*pType = TZPT_NONE;
		break;
	default:
		dovi_error("invalid call direction:%d\n",
			direct);
		return DOVI_STATUS_ERROR;
	}
	return DOVI_STATUS_OK;
}


enum dovi_status dovi_sec_service_call(
	enum DOVI_TZ_CALL_CMD cmd,
	enum DOVI_TZ_CALL_DIR direct,
	void *buffer, uint32_t size)
{
	uint32_t paramTypes = 0;
	union MTEEC_PARAM dovi_param[4];
	enum TZ_PARAM_TYPES type;
	int ret;
	enum dovi_status status = DOVI_STATUS_OK;

	if (dovi_tz_session == 0) {
		dovi_error("dovi_tz_session not created\n");
		return DOVI_STATUS_ERROR;
	}

	status = dovi_sec_map_direction(direct, &type);
	if (status != DOVI_STATUS_OK)
		return status;

	paramTypes = TZ_ParamTypes1(type);

	dovi_param[0].mem.buffer = buffer;
	dovi_param[0].mem.size = size;

	dovi_info("cmd %d, paratype %d buf %p, size %u\n",
		  cmd, paramTypes, buffer, size);

	ret = KREE_TeeServiceCall(dovi_tz_session, cmd,
		paramTypes, dovi_param);
	if (ret != TZ_RESULT_SUCCESS) {
		dovi_error("call fail: cmd[%d] ret[%d]\n",
			cmd, ret);
		return DOVI_STATUS_ERROR;
	}

	return DOVI_STATUS_OK;
}

enum dovi_status dovi_sec_share_mem_service_call(
	enum DOVI_TZ_CALL_CMD cmd,
	enum DOVI_TZ_CALL_DIR direct,
	void *buffer, uint32_t size)
{
	uint32_t paramTypes = 0;
	union MTEEC_PARAM dovi_param[4];
	enum TZ_PARAM_TYPES type;
	int ret;
	enum dovi_status status = DOVI_STATUS_OK;

	if (dovi_tz_session == 0) {
		dovi_error("dovi_tz_session not created\n");
		return DOVI_STATUS_ERROR;
	}

	status = dovi_sec_map_direction(direct, &type);
	if (status != DOVI_STATUS_OK) {
		dovi_error("dovi_sec_map_direction fail\n");
		return status;
	}

	paramTypes = TZ_ParamTypes1(type);

	dovi_param[0].memref.handle =
		(uint32_t) (*(uint32_t *) buffer);
	dovi_param[0].memref.offset = 0;
	dovi_param[0].memref.size = size;

	dovi_info(" cmd %d buf 0x%p size %u handle 0x%X\n",
		  cmd, buffer, size,
		  dovi_param[0].memref.handle);

	ret = KREE_TeeServiceCall(dovi_tz_session,
		cmd, paramTypes, dovi_param);
	if (ret != TZ_RESULT_SUCCESS) {
		dovi_error("call fail: cmd[%d] ret[0x%X]\n",
			cmd, ret);
		return DOVI_STATUS_ERROR;
	}

	return DOVI_STATUS_OK;
}

enum dovi_status dovi_sec_init(void)
{
	static bool dovi_sec_inited;
	enum dovi_status status = DOVI_STATUS_OK;

	dovi_func_default();
	if (dovi_sec_inited)
		return DOVI_STATUS_OK;
	dovi_sec_inited = true;

	do {
		status = dovi_sec_create_session();
		if (status != DOVI_STATUS_OK)
			break;

		status = dovi_sec_create_share_memory();
		if (status != DOVI_STATUS_OK)
			break;

		status =
			dovi_sec_create_share_mem_session();
		if (status != DOVI_STATUS_OK)
			break;

		status =
			dovi_sec_create_share_mem_handle();
		if (status != DOVI_STATUS_OK)
			break;

		#if 1
		status = dovi_sec_share_memory_init();
		if (status != DOVI_STATUS_OK)
			break;
		#endif

	} while (0);

	dovi_sec_status();

	if (status != DOVI_STATUS_OK)
		dovi_error("init dovi secure fail[%d]\n", status);

	return status;
}

enum dovi_status dovi_sec_share_memory_init(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t size =
		sizeof(struct dovi_share_memory_info_t);

	if (dovi_sec_share_mem_inited) {
		dovi_error("share mem already init\n");
		return DOVI_STATUS_OK;
	}

	status = dovi_sec_share_mem_service_call(
		DOVI_TZ_CALL_CMD_SHARE_MEMORY_INIT,
		DOVI_TZ_CALL_DIR_MEMREF_INPUT,
		(void *)&dovi_tz_mem_handle, size);

	if (status != DOVI_STATUS_OK)
		dovi_error("init share mem fail [%d][%d]\n",
		status, size);

	return status;
}

enum dovi_status dovi_sec_md_parser_init(void)
{
	enum dovi_status status = DOVI_STATUS_OK;

	if (dovi_sec_md_parser_inited) {
		dovi_error("dv_md_parser already inited\n");
		return DOVI_STATUS_OK;
	}
	dovi_sec_md_parser_inited = true;

#if 1
	status = dovi_sec_service_call(
	DOVI_TZ_CALL_CMD_MD_PARSER_INIT,
	DOVI_TZ_CALL_DIR_NONE, NULL, 0);
#else
#endif
	if (status != DOVI_STATUS_OK)
		dovi_error("dovi secure fail[%d]\n", status);

	return status;
}

enum dovi_status dovi_sec_md_parser_uninit(void)
{
	enum dovi_status status = DOVI_STATUS_OK;

	//if (!dovi_sec_md_parser_inited) {
	//	dovi_error("dv_md_parser already uninited\n");
	//	return DOVI_STATUS_OK;
	//}
	//dovi_sec_md_parser_inited = false;

	status = dovi_sec_service_call(
		DOVI_TZ_CALL_CMD_MD_PARSER_UNINIT,
		DOVI_TZ_CALL_DIR_NONE, NULL, 0);

	if (status != DOVI_STATUS_OK)
		dovi_error("init dv_sec fail[%d]\n", status);

	return status;
}

enum dovi_status dovi_sec_cp_test_init(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t size =
		sizeof(struct dovi_share_memory_info_t);
	char *p_dovi_share_mem_handle =
		(char *)&dovi_tz_mem_handle;

	if (dovi_sec_cp_test_inited)
		dovi_info("dovi_sec_cp_test init again!\n");
	dovi_sec_cp_test_inited = true;

	dovi_info("size %d share %p fra %u, dr %u,_len %u 0x%p\n",
		  size,
		  dovi_share_mem,
		  dovi_share_mem->src_param[0].src_frame_num,
		  dovi_share_mem->src_param[0].dr_type,
		  dovi_share_mem->src_param[0].rpu_bs_len,
		  dovi_share_mem->src_param[0].rpu_bs_buffer);

	status = dovi_sec_share_mem_service_call(
		DOVI_TZ_CALL_CMD_CP_TEST_INIT,
		DOVI_TZ_CALL_DIR_MEMREF_INPUT,
		(void *)p_dovi_share_mem_handle, size);

	if (status != DOVI_STATUS_OK)
		dovi_error("dovi_sec_cp_test init fail[%d]\n",
		status);

	return status;
}

enum dovi_status dovi_sec_find_rpu_buffer(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t len,
	uint32_t offset)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t size = sizeof(struct dovi_share_memory_info_t);
	char *p_dovi_share_mem_handle =
		(char *)&dovi_tz_mem_handle;

	dovi_share_mem->src_param[layer_id].sec_handle_in = sec_handle;
	dovi_share_mem->src_param[layer_id].len_tmp = len;
	dovi_share_mem->sec_layer = layer_id;
	dovi_share_mem->src_param[layer_id].sec_offset = offset;

	status = dovi_sec_share_mem_service_call(
		DOVI_TZ_CALL_CMD_FIND_RPU_BUFFER,
		DOVI_TZ_CALL_DIR_MEMREF_INOUT,
		(void *)p_dovi_share_mem_handle, size);

	dovi_info("%s %d %d %d %d\n", __func__,
		dovi_share_mem->src_param[layer_id].sec_handle_in,
		dovi_share_mem->src_param[layer_id].len_tmp,
		dovi_share_mem->sec_layer,
		dovi_share_mem->src_param[layer_id].sec_handle_out);


	if (status != DOVI_STATUS_OK)
		dovi_error("%s fail[%d]\n", __func__, status);

	return status;
}

enum dovi_status dovi_sec_cp_test_main(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t size =
		sizeof(struct dovi_share_memory_info_t);
	char *p_dovi_share_mem_handle =
		(char *)&dovi_tz_mem_handle;

	status = dovi_sec_share_mem_service_call(
		DOVI_TZ_CALL_CMD_CP_TEST_MAIN,
		DOVI_TZ_CALL_DIR_MEMREF_INOUT,
		(void *)p_dovi_share_mem_handle, size);

	if (status != DOVI_STATUS_OK)
		dovi_error("%s fail[%d]\n", __func__, status);

	return status;
}

enum dovi_status dovi_sec_cp_test_uninit(void)
{
	enum dovi_status status = DOVI_STATUS_OK;

	//if (!dovi_sec_cp_test_inited) {
	//	dovi_error("%s already uninited\n", __func__);
	//	return DOVI_STATUS_OK;
	//}
	//dovi_sec_cp_test_inited = false;

	status = dovi_sec_service_call(
		DOVI_TZ_CALL_CMD_CP_TEST_UNINIT,
		DOVI_TZ_CALL_DIR_NONE, NULL, 0);

	if (status != DOVI_STATUS_OK)
		dovi_error("dovi_sec_cp_test uninit fail[%d]\n",
		status);

	return status;
}

enum dovi_status dovi_sec_status(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t share_size =
		sizeof(struct dovi_share_memory_info_t);

	dovi_func();

	dovi_info("sec session 0x%X,session 0x%X, handle 0x%X\n",
		  dovi_tz_session, dovi_tz_mem_session,
		  dovi_tz_mem_handle);

	dovi_info("share mem %p size %d\n",
		dovi_share_mem, share_size);


	return status;
}

enum dovi_status dovi_sec_debug_level_init(uint32_t dovi_log_level)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t size = sizeof(struct dovi_share_memory_info_t);
	char *p_dovi_share_mem_handle =
		(char *)&dovi_tz_mem_handle;


	dovi_share_mem->log_level = dovi_log_level;
	status = dovi_sec_share_mem_service_call(
		DOVI_TZ_CALL_CMD_DEBUG_LEVEL_INIT,
		DOVI_TZ_CALL_DIR_MEMREF_INPUT,
		(void *)p_dovi_share_mem_handle, size);

	if (status != DOVI_STATUS_OK)
		dovi_error("init debug level init fail [%d]\n",
		status);

	return 0;
}

enum dovi_status dovi_sec_handle_copy(uint32_t *sec_handle,
	uint32_t len)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t size = sizeof(struct dovi_share_memory_info_t);
	char *p_dovi_share_mem_handle =
		(char *)&dovi_tz_mem_handle;

	dovi_share_mem->src_param[0].sec_handle_in = *sec_handle;
	dovi_share_mem->src_param[0].sec_handle_len = len;
	status = dovi_sec_share_mem_service_call(
		DOVI_TZ_CALL_CMD_SEC_MEM_COPY,
		DOVI_TZ_CALL_DIR_MEMREF_INOUT,
		(void *)p_dovi_share_mem_handle, size);

	if (status != DOVI_STATUS_OK)
		dovi_error("%s fail [%d]\n", __func__, status);

	*sec_handle = dovi_share_mem->src_param[0].sec_handle_out;

	return status;
}

#else

enum dovi_status dovi_sec_md_parser_init(void)
{
	return DOVI_STATUS_OK;
}

enum dovi_status dovi_sec_md_parser_uninit(void)
{
	return DOVI_STATUS_OK;
}

enum dovi_status dovi_sec_cp_test_init(void)
{
	return DOVI_STATUS_OK;
}

enum dovi_status dovi_sec_cp_test_main(void)
{
	return DOVI_STATUS_OK;
}

enum dovi_status dovi_sec_cp_test_uninit(void)
{
	return DOVI_STATUS_OK;
}

enum dovi_status dovi_sec_init(void)
{
	enum dovi_status status = DOVI_STATUS_OK;
	uint32_t size = sizeof(struct dovi_share_memory_info_t);

	if (dovi_share_mem) {
		dovi_error("share memory already created %p size %u\n",
			dovi_share_mem, size);
		return DOVI_STATUS_OK;
	}

	dovi_share_mem = kmalloc(size, GFP_KERNEL);
	if (!dovi_share_mem) {
		dovi_error("share memory already fail %p size %u\n",
			dovi_share_mem, size);
		return DOVI_STATUS_OK;
	}

	memset((void *)dovi_share_mem, 0, size);

	if (status != DOVI_STATUS_OK)
		dovi_error("create share memory fail size %d 0x%p\n",
		size, dovi_share_mem);

	return DOVI_STATUS_OK;
}

#endif
