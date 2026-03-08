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

#define LOG_TAG "CFD_TZ_CLIENT"

#include <kree/mem.h>
#include <kree/system.h>
#include <linux/slab.h>
#include "disp_info.h"
#include "disp_cfd_hal.h"
#include "disp_cfd_tz_client.h"
#include "disp_cfd_debug.h"
#include "disp_hw_log.h"
#include <tz_cross/ta_mem.h>
#include <tz_cross/trustzone.h>

static struct cfd_share_memory_info_t_ar *pt_cfd_share_mem_array;
struct cfd_share_memory_info_t *cfd_share_mem[CFD_LAYER_MAX];

static KREE_SESSION_HANDLE cfd_tz_session;
static KREE_SESSION_HANDLE cfd_tz_mem_session;
static KREE_SHAREDMEM_HANDLE cfd_tz_mem_handle;
static KREE_SESSION_HANDLE cfd_tz_vdp_session;

static bool cfd_sec_share_mem_inited;

struct cfd_share_memory_info_t *cfd_sec_get_share_mem(int layer_id)
{
	if ((layer_id < 0) || (pt_cfd_share_mem_array == NULL)
		|| (cfd_share_mem[layer_id] == NULL)) {
		cfd_error("[CFD]get sec shm fail :%d\n", layer_id);
		return NULL;
	}
	return cfd_share_mem[layer_id];
}
static enum cfd_status cfd_sec_create_session(void)
{
	enum cfd_status status = CFD_STATUS_OK;
	int ret;

	do {
		if (cfd_tz_session == 0) {
			ret = KREE_CreateSession(TZ_TA_CFD_UUID,
				&cfd_tz_session);
			if (ret != TZ_RESULT_SUCCESS) {
				cfd_error("[CFD]create tz_session fail:%d\n",
					ret);
				status = CFD_STATUS_ERROR;
				break;
			}
		}
	} while (0);

	if (status != CFD_STATUS_OK)
		cfd_error("create session fail:%d\n", status);
	else
		cfd_info("create session cfd_tz_session[0x%X]\n",
		cfd_tz_session);

	return status;
}

static enum cfd_status cfd_sec_create_share_memory(void)
{
	enum cfd_status status = CFD_STATUS_OK;
	void *tmp_buf = NULL;

	uint32_t size = sizeof(struct cfd_share_memory_info_t_ar);

	if (pt_cfd_share_mem_array) {
		cfd_error("share memory already created %p size %u\n",
			pt_cfd_share_mem_array, size);
		return CFD_STATUS_OK;
	}

	tmp_buf = kmalloc(size, GFP_KERNEL);
	pt_cfd_share_mem_array =
		(struct cfd_share_memory_info_t_ar *)tmp_buf;
	if (!pt_cfd_share_mem_array) {
		cfd_error("share memory already fail %p size %u\n",
			pt_cfd_share_mem_array, size);
		return CFD_STATUS_OK;
	}

	cfd_info("share memory create ok %p size %u\n",
			pt_cfd_share_mem_array, size);

	cfd_share_mem[0] = &(pt_cfd_share_mem_array->cfd_shm_info[0]);
	cfd_share_mem[1] = &(pt_cfd_share_mem_array->cfd_shm_info[1]);
	cfd_share_mem[2] = &(pt_cfd_share_mem_array->cfd_shm_info[2]);
	cfd_share_mem[3] = &(pt_cfd_share_mem_array->cfd_shm_info[3]);

	cfd_info("share memory create 0x%p 0x%p 0x%p 0x%p size %u\n",
		cfd_share_mem[0], cfd_share_mem[1], cfd_share_mem[2],
		cfd_share_mem[3], size);

	memset((void *)cfd_share_mem[0], 0x00,
		sizeof(struct cfd_share_memory_info_t));
	memset((void *)cfd_share_mem[1], 0x00,
		sizeof(struct cfd_share_memory_info_t));
	memset((void *)cfd_share_mem[2], 0x00,
		sizeof(struct cfd_share_memory_info_t));
	memset((void *)cfd_share_mem[3], 0x00,
		sizeof(struct cfd_share_memory_info_t));

	cfd_share_mem[0]->layer_id = 0;
	cfd_share_mem[1]->layer_id = 1;
	cfd_share_mem[2]->layer_id = 2;
	cfd_share_mem[3]->layer_id = 3;

	if (status != CFD_STATUS_OK)
		cfd_error("create share memory fail size %d 0x%p\n",
		size, cfd_share_mem[0]);
	else
		cfd_info("create share memory ok size %d 0x%p %d\n",
		sizeof(struct cfd_share_memory_info_t),
		cfd_share_mem[0], cfd_share_mem[0]->layer_id);

	return CFD_STATUS_OK;
}

enum cfd_status cfd_sec_create_share_mem_session(void)
{
	enum cfd_status status = CFD_STATUS_OK;
	int ret;

	do {
		if (cfd_tz_mem_session == 0) {
			ret = KREE_CreateSession(TZ_TA_MEM_UUID,
				&cfd_tz_mem_session);
			if (ret != TZ_RESULT_SUCCESS) {
				cfd_error("createtz_mem_session fail:%d\n",
					ret);
				status = CFD_STATUS_ERROR;
				break;
			}
		}
	} while (0);

	if (status != CFD_STATUS_OK)
		cfd_error("create session fail:%d\n", status);
	else
		cfd_info("create session cfd_tz_mem_session[0x%X]\n",
		cfd_tz_mem_session);

	return status;
}

enum cfd_status cfd_sec_create_vdp_session(void)
{
	enum cfd_status status = CFD_STATUS_OK;
	int ret;

	do {
		if (cfd_tz_vdp_session == 0) {
			ret = KREE_CreateSession(TZ_TA_MEM_UUID,
				&cfd_tz_vdp_session);
			if (ret != TZ_RESULT_SUCCESS) {
				cfd_error("createtz_vdp_session fail:%d\n",
					ret);
				status = CFD_STATUS_ERROR;
				break;
			}
		}
	} while (0);

	if (status != CFD_STATUS_OK)
		cfd_error("create vdp session fail:%d\n", status);
	else
		cfd_info("create session vdp[0x%X]\n",
		cfd_tz_vdp_session);

	return status;
}

enum cfd_status cfd_sec_create_share_mem_handle(void)
{
	enum cfd_status status = CFD_STATUS_OK;
	struct KREE_SHAREDMEM_PARAM cfd_param;
	int ret;

	cfd_param.buffer = (void *)pt_cfd_share_mem_array;
	cfd_param.size = sizeof(struct cfd_share_memory_info_t_ar);

	do {
		if (cfd_tz_mem_handle == 0) {
			ret = KREE_RegisterSharedmem(cfd_tz_mem_session,
				&cfd_tz_mem_handle, &cfd_param);
			if (ret != TZ_RESULT_SUCCESS) {
				status = CFD_STATUS_ERROR;
				break;
			}
		}
	} while (0);

	if (status != CFD_STATUS_OK)
		cfd_error("create dv_tz_mem_hdl fail:%d\n",
		status);
	else {
		cfd_info(
			"create dv_tz_mem_hdl[0x%X] pointer 0x%p\n",
			cfd_tz_mem_handle,
			(void *)pt_cfd_share_mem_array);
	}

	return status;
}

static enum cfd_status cfd_sec_map_direction(
	enum CFD_TZ_CALL_DIR direct,
	enum TZ_PARAM_TYPES *pType)
{
	switch (direct) {
	case CFD_TZ_CALL_DIR_MEM_INPUT:
		*pType = TZPT_MEM_INPUT;
		break;
	case CFD_TZ_CALL_DIR_MEM_OUTPUT:
		*pType = TZPT_MEM_OUTPUT;
		break;
	case CFD_TZ_CALL_DIR_MEM_INOUT:
		*pType = TZPT_MEM_INOUT;
		break;
	case CFD_TZ_CALL_DIR_MEMREF_INPUT:
		*pType = TZPT_MEMREF_INPUT;
		break;
	case CFD_TZ_CALL_DIR_MEMREF_OUTPUT:
		*pType = TZPT_MEMREF_OUTPUT;
		break;
	case CFD_TZ_CALL_DIR_MEMREF_INOUT:
		*pType = TZPT_MEMREF_INOUT;
		break;
	case CFD_TZ_CALL_DIR_VALUE_INPUT:
		*pType = TZPT_VALUE_INPUT;
		break;
	default:
		cfd_error("invalid call direction:%d\n", direct);
		return CFD_STATUS_ERROR;
	}
	return CFD_STATUS_OK;
}

enum cfd_status cfd_sec_vdp_service_call(
	enum CFD_TZ_CALL_CMD cmd,
	enum CFD_TZ_CALL_DIR direct,
	void *buffer, uint32_t size)
{
	unsigned int paramTypes = 0;
	union MTEEC_PARAM cfd_param[4];
	enum TZ_PARAM_TYPES type;
	int ret;
	enum cfd_status status = CFD_STATUS_OK;
	KREE_SHAREDMEM_HANDLE cfd_tz_mem_hdr10plus_handle = 0;
	struct KREE_SHAREDMEM_PARAM cfd_mem_param;

	if (cfd_tz_session == 0) {
		cfd_error("cfd_tz_session not created\n");
		return CFD_STATUS_ERROR;
	}

	if (cfd_tz_vdp_session == 0) {
		cfd_error("cfd_tz_vdp_session not created\n");
		return CFD_STATUS_ERROR;
	}

	if (cfd_tz_mem_hdr10plus_handle == 0) {
		cfd_mem_param.buffer = (void *)buffer;
		cfd_mem_param.size = size;
		ret = KREE_RegisterSharedmem(cfd_tz_vdp_session,
			&cfd_tz_mem_hdr10plus_handle, &cfd_mem_param);
		if (ret != TZ_RESULT_SUCCESS) {
			status = CFD_STATUS_ERROR;
			return status;
		}
	}

	status = cfd_sec_map_direction(direct, &type);
	if (status != CFD_STATUS_OK) {
		if (cfd_tz_mem_hdr10plus_handle != 0) {
			KREE_UnregisterSharedmem(cfd_tz_vdp_session,
				cfd_tz_mem_hdr10plus_handle);
			cfd_tz_mem_hdr10plus_handle = 0;
		}
		return status;
	}
	paramTypes = TZ_ParamTypes1(type);

	cfd_param[0].memref.handle =
	(uint32_t)cfd_tz_mem_hdr10plus_handle;
	cfd_param[0].memref.offset = 0;
	cfd_param[0].memref.size = size;

	cfd_info("cmd %d, paratype %d buf %p, size %u\n",
		  cmd, paramTypes, buffer, size);

	ret = KREE_TeeServiceCall(cfd_tz_session, cmd,
		paramTypes, cfd_param);
	if (ret != TZ_RESULT_SUCCESS) {
		cfd_error("call fail: cmd[%d] ret[%d]\n",
			cmd, ret);
		if (cfd_tz_mem_hdr10plus_handle != 0) {
			KREE_UnregisterSharedmem(cfd_tz_vdp_session,
				cfd_tz_mem_hdr10plus_handle);
			cfd_tz_mem_hdr10plus_handle = 0;
		}
		return CFD_STATUS_ERROR;
	}

	if (cfd_tz_mem_hdr10plus_handle != 0) {
		KREE_UnregisterSharedmem(cfd_tz_vdp_session,
			cfd_tz_mem_hdr10plus_handle);
		cfd_tz_mem_hdr10plus_handle = 0;
	}
	return CFD_STATUS_OK;
}


enum cfd_status cfd_sec_service_call_value(
	enum CFD_TZ_CALL_CMD cmd,
	enum CFD_TZ_CALL_DIR direct,
	uint32_t a, uint32_t b)
{
	unsigned int paramTypes = 0;
	union MTEEC_PARAM cfd_param[4];
	enum TZ_PARAM_TYPES type;
	int ret;
	enum cfd_status status = CFD_STATUS_OK;

	if (cfd_tz_session == 0) {
		cfd_error("cfd_tz_session not created\n");
		return CFD_STATUS_ERROR;
	}

	status = cfd_sec_map_direction(direct, &type);
	if (status != CFD_STATUS_OK)
		return status;

	paramTypes = TZ_ParamTypes1(type);

	cfd_param[0].value.a = a;
	cfd_param[0].value.b = b;

	cfd_info("cmd %d, paratype %d value a %d, b %d\n",
		  cmd, paramTypes, a, b);

	ret = KREE_TeeServiceCall(cfd_tz_session, cmd,
		paramTypes, cfd_param);
	if (ret != TZ_RESULT_SUCCESS) {
		cfd_error("call fail: cmd[%d] ret[%d]\n",
			cmd, ret);
		return CFD_STATUS_ERROR;
	}

	return CFD_STATUS_OK;
}

enum cfd_status cfd_sec_share_mem_service_call(
	enum CFD_TZ_CALL_CMD cmd,
	enum CFD_TZ_CALL_DIR direct,
	void *buffer, uint32_t size)
{
	unsigned int paramTypes = 0;
	union MTEEC_PARAM cfd_param[4];
	enum TZ_PARAM_TYPES type;
	int ret;
	enum cfd_status status = CFD_STATUS_OK;

	if (cfd_tz_session == 0) {
		cfd_error("cfd_tz_session not created\n");
		return CFD_STATUS_ERROR;
	}

	status = cfd_sec_map_direction(direct, &type);
	if (status != CFD_STATUS_OK)
		return status;

	paramTypes = TZ_ParamTypes1(type);

	cfd_param[0].memref.handle =
		(uint32_t) (*(uint32_t *) buffer);
	cfd_param[0].memref.offset = 0;
	cfd_param[0].memref.size = size;

	cfd_info(" cmd %d buf 0x%p size %u type %x handle 0x%X\n",
		  cmd, buffer, size, paramTypes,
		  cfd_param[0].memref.handle);

	ret = KREE_TeeServiceCall(cfd_tz_session,
		cmd, paramTypes, cfd_param);
	if (ret != TZ_RESULT_SUCCESS) {
		cfd_error("call fail: cmd[%d] ret[0x%X]\n",
			cmd, ret);
		return CFD_STATUS_ERROR;
	}

	return CFD_STATUS_OK;
}

enum cfd_status cfd_sec_share_memory_init(void)
{
	enum cfd_status status = CFD_STATUS_OK;
	uint32_t size =
		sizeof(struct cfd_share_memory_info_t_ar);

	if (cfd_sec_share_mem_inited) {
		cfd_error("share mem already init\n");
		return CFD_STATUS_OK;
	}

	status = cfd_sec_share_mem_service_call(
		CFD_TZ_CALL_CMD_SHARE_MEMORY_INIT,
		CFD_TZ_CALL_DIR_MEMREF_INOUT,
		(void *)&cfd_tz_mem_handle, size);

	if (status != CFD_STATUS_OK)
		cfd_error("init share mem fail [%d][%d]\n",
		status, size);

	return status;
}

enum cfd_status cfd_sec_init(void)
{
	static bool cfd_sec_inited;
	enum cfd_status status = CFD_STATUS_OK;

	cfd_func();
	if (cfd_sec_inited)
		return CFD_STATUS_OK;

	do {
		status = cfd_sec_create_session();
		if (status != CFD_STATUS_OK)
			break;

		status = cfd_sec_create_share_memory();
		if (status != CFD_STATUS_OK)
			break;

		status =
			cfd_sec_create_share_mem_session();
		if (status != CFD_STATUS_OK)
			break;

		status =
			cfd_sec_create_vdp_session();
		if (status != CFD_STATUS_OK)
			break;

		status =
			cfd_sec_create_share_mem_handle();
		if (status != CFD_STATUS_OK)
			break;

		status = cfd_sec_share_memory_init();
		if (status != CFD_STATUS_OK)
			break;

		status = cfd_sec_service_call_value(
			CFD_TZ_CALL_CMD_INIT,
			CFD_TZ_CALL_DIR_VALUE_INPUT, 0, 0);
		if (status != CFD_STATUS_OK)
			break;

	} while (0);

	cfd_sec_status();

	if (status != CFD_STATUS_OK)
		cfd_error("init cfd secure fail[%d]\n", status);
	else
		cfd_sec_inited = true;
	cfd_info("init cfd secure done\n");
	return status;
}

enum cfd_status cfd_sec_deinit(void)
{
	static bool cfd_sec_deinited;
	enum cfd_status status = CFD_STATUS_OK;
	unsigned int idx = 0;

	cfd_func_default();
	if (cfd_sec_deinited)
		return CFD_STATUS_OK;

	status = cfd_sec_service_call_value(
		CFD_TZ_CALL_CMD_DEINIT,
		CFD_TZ_CALL_DIR_VALUE_INPUT, 0, 0);
	if (status != CFD_STATUS_OK)
		cfd_error("cfd set deinit fail[%d]\n", status);

	if (cfd_tz_mem_handle != 0) {
		KREE_UnregisterSharedmem(cfd_tz_mem_session, cfd_tz_mem_handle);
		cfd_tz_mem_handle = 0;
	}

	if (cfd_tz_mem_session != 0) {
		KREE_CloseSession(cfd_tz_mem_session);
		cfd_tz_mem_session = 0;
	}

	if (cfd_tz_vdp_session != 0) {
		KREE_CloseSession(cfd_tz_vdp_session);
		cfd_tz_vdp_session = 0;
	}

	if (cfd_tz_session != 0) {
		KREE_CloseSession(cfd_tz_session);
		cfd_tz_session = 0;
	}

	if (pt_cfd_share_mem_array != NULL) {
		kfree(pt_cfd_share_mem_array);
		pt_cfd_share_mem_array = NULL;
		for (idx = 0; idx < CFD_LAYER_MAX; idx++)
			cfd_share_mem[idx] = NULL;
	}

	cfd_sec_deinited = true;
	cfd_error("deinit cfd secure done\n");
	return status;
}

enum cfd_status cfd_sec_config_frame(uint32_t layer_id)
{
	enum cfd_status status = CFD_STATUS_OK;
	uint32_t size = sizeof(struct cfd_share_memory_info_t_ar);
	char *p_cfd_share_mem_handle = (char *)&cfd_tz_mem_handle;

	if (layer_id == 0) {
		status = cfd_sec_share_mem_service_call(
			CFD_TZ_CALL_CMD_CONFIG_FRAME_LAYER0,
			CFD_TZ_CALL_DIR_MEMREF_INOUT,
			(void *)p_cfd_share_mem_handle, size);
	} else if (layer_id == 1) {
		status = cfd_sec_share_mem_service_call(
			CFD_TZ_CALL_CMD_CONFIG_FRAME_LAYER1,
			CFD_TZ_CALL_DIR_MEMREF_INOUT,
			(void *)p_cfd_share_mem_handle, size);

	} else if (layer_id == 2) {
		status = cfd_sec_share_mem_service_call(
			CFD_TZ_CALL_CMD_CONFIG_FRAME_LAYER2,
			CFD_TZ_CALL_DIR_MEMREF_INOUT,
			(void *)p_cfd_share_mem_handle, size);

	} else if (layer_id == 3) {
		status = cfd_sec_share_mem_service_call(
			CFD_TZ_CALL_CMD_CONFIG_FRAME_LAYER3,
			CFD_TZ_CALL_DIR_MEMREF_INOUT,
			(void *)p_cfd_share_mem_handle, size);
	} else
		cfd_error("%s fail ,invalid layer[%d]\n", __func__, layer_id);

	if (status != CFD_STATUS_OK)
		cfd_error("%s fail[%d]\n", __func__, status);

	return status;
}

enum cfd_status cfd_sec_parser_hdr10plus_vsif(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t sec_len)
{
	enum cfd_status status = CFD_STATUS_OK;
	uint32_t size = sizeof(struct cfd_share_memory_info_t_ar);
	char *p_cfd_share_mem_handle = (char *)&cfd_tz_mem_handle;
	struct cfd_share_memory_info_t *cfd_main_vid_info = NULL;

	if (layer_id == 0) {
		cfd_main_vid_info = cfd_sec_get_share_mem(layer_id);
		if (cfd_main_vid_info != NULL) {
			cfd_main_vid_info->sec_handle_type = 0;
			cfd_main_vid_info->sec_handle_in = sec_handle;
			cfd_main_vid_info->sec_handle_len = sec_len;
		}
		status = cfd_sec_share_mem_service_call(
			CFD_TZ_CALL_CMD_PARSER_HDR10PLUS_SECURE_HANDLE,
			CFD_TZ_CALL_DIR_MEMREF_INOUT,
			(void *)p_cfd_share_mem_handle, size);
	} else
		cfd_error("%s fail ,invalid layer[%d]\n", __func__, layer_id);

	if (status != CFD_STATUS_OK)
		cfd_error("%s fail[%d]\n", __func__, status);

	return status;
}

enum cfd_status cfd_sec_parser_hdr10plus_emp(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t sec_len)
{
	enum cfd_status status = CFD_STATUS_OK;
	uint32_t size = sizeof(struct cfd_share_memory_info_t_ar);
	char *p_cfd_share_mem_handle = (char *)&cfd_tz_mem_handle;
	struct cfd_share_memory_info_t *cfd_main_vid_info = NULL;

	if (layer_id == 0) {
		cfd_main_vid_info = cfd_sec_get_share_mem(layer_id);
		if (cfd_main_vid_info != NULL) {
			cfd_main_vid_info->sec_handle_type = 1;
			cfd_main_vid_info->sec_handle_in = sec_handle;
			cfd_main_vid_info->sec_handle_len = sec_len;
		}
		status = cfd_sec_share_mem_service_call(
			CFD_TZ_CALL_CMD_PARSER_HDR10PLUS_SECURE_HANDLE,
			CFD_TZ_CALL_DIR_MEMREF_INOUT,
			(void *)p_cfd_share_mem_handle, size);
	} else
		cfd_error("%s fail ,invalid layer[%d]\n", __func__, layer_id);

	if (status != CFD_STATUS_OK)
		cfd_error("%s fail[%d]\n", __func__, status);

	return status;
}

enum cfd_status cfd_sec_status(void)
{
	enum cfd_status status = CFD_STATUS_OK;
	uint32_t share_size =
		sizeof(struct cfd_share_memory_info_t);

	cfd_func();

	cfd_info("sec session 0x%X,session 0x%X, handle 0x%X\n",
		  cfd_tz_session, cfd_tz_mem_session,
		  cfd_tz_mem_handle);

	cfd_info("share mem %p size %d\n",
		cfd_share_mem[0], share_size);

	return status;
}

enum cfd_status cfd_sec_debug_level_init(uint32_t cfd_log_level)
{
	enum cfd_status status = CFD_STATUS_OK;
	unsigned int layer_id = 0;
	unsigned int log_lvl = cfd_log_level;

	status = cfd_sec_service_call_value(
		CFD_TZ_CALL_CMD_DEBUG_LEVEL_INIT,
		CFD_TZ_CALL_DIR_VALUE_INPUT, layer_id, log_lvl);

	if (status != CFD_STATUS_OK)
		cfd_error("init debug level init fail [%d]\n",
		status);

	return status;
}

enum cfd_status cfd_backup_hdr10plus_sec_handle(void *buffer,
	uint32_t size)
{
	int ret = 0;

	ret = cfd_sec_vdp_service_call(
		CFD_TZ_CALL_CMD_BACKUP_HDR10PLUS_SECURE_HANDLE,
		CFD_TZ_CALL_DIR_MEMREF_INOUT, buffer,
		size);
	if (ret != CFD_STATUS_OK) {
		cfd_error("service call fail: cmd[%d] ret[%d]\n",
			   CFD_TZ_CALL_CMD_BACKUP_HDR10PLUS_SECURE_HANDLE,
			   ret);
		return CFD_STATUS_ERROR;
	}

	return ret;
}



