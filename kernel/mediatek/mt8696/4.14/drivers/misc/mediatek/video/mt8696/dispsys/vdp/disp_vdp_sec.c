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
#define LOG_TAG "VDP_SEC"

//#include <kree/mem.h> 8696 review
//#include <kree/system.h> 8696 review
//#include <tz_cross/ta_mem.h> 8696 review
//#include <tz_cross/trustzone.h> 8696 review
#include <asm-generic/io.h>
#include <linux/slab.h>
#include <kree/system.h>
#include <kree/mem.h>
#include <tz_cross/ta_mem.h>
#include <tz_cross/trustzone.h>
#include "disp_hw_log.h"
#include "disp_vdp_if.h"
#include "disp_vdp_sec.h"
#include "disp_vdp_vsync.h"
#include "fmt_hal.h"
#include "vdp_hal.h"
#include "vdp_hw.h"

#if VIDEO_DISPLAY_SECURE_ENABLE
static KREE_SESSION_HANDLE vdp_session;
static KREE_SESSION_HANDLE vdp_mem_session;

/* static struct hdr_share_memory_struct gShareMemory; */
static int _vdp_sec_create_session(void)
{
	int ret;

	do {
		if (vdp_session == 0) {
			ret = KREE_CreateSession(TZ_TA_VDP_UUID,
						 &vdp_session);
			if (ret != TZ_RESULT_SUCCESS) {
				DISP_LOG_E("create vdp_session fail:%d\n", ret);
				return VDP_CREATE_SESSION_FAIL;
			}
		}

	} while (0);

	return VDP_OK;
}

/*static function, create memory session for shared memory*/
static int _vdp_sec_create_mem_session(void)
{
	int ret;

	do {
		if (vdp_mem_session == 0) {
			ret = KREE_CreateSession(TZ_TA_MEM_UUID,
					&vdp_mem_session);
			if (ret != TZ_RESULT_SUCCESS) {
				DISP_LOG_E(
					"create vdp mem session fail:%d\n",
					ret);
				return VDP_CREATE_SESSION_FAIL;
			}
		}
	} while (0);

	return VDP_OK;
}

#if 0
static int _vdp_sec_destroy_session(void)
{
	int ret;

	if (vdp_session != 0) {
		ret = KREE_CloseSession(vdp_session);
		if (ret != TZ_RESULT_SUCCESS) {
			DISP_LOG_E("vdp session invalid,id=%d\n", vdp_session);
			return VDP_DESTROY_SESSION_FAIL;
		}
		vdp_session = 0;
	}
	return VDP_OK;
}
#endif

/* we need to map normal buffer to secure,
 * because this nomal buffer may still display after convert m4u port to secure.
 */
int disp_vdp_sec_init(unsigned int layer_id, uint32_t normal_mva,
		      unsigned int normal_mva_size)
{
	int ret = 0;
	struct vdp_sec_init_info init_info;

	ret = _vdp_sec_create_session();

	if (ret != VDP_OK) {
		DISP_LOG_E("%s create secure session fail\n", __func__);
		return VDP_FAIL;
	}

	ret = _vdp_sec_create_mem_session();
	if (ret != VDP_OK) {
		DISP_LOG_E("%s create mem session fail\n", __func__);
		return VDP_FAIL;
	}


	do {
		fmt_hal_set_secure(layer_id, true);
		/* fill vdp_sec_init_info structure. */
		memset(&init_info, 0, sizeof(init_info));
		init_info.layer_id = layer_id;
		init_info.normal_mva = normal_mva;
		init_info.mva_size = normal_mva_size;

		vdp_hal_get_info(layer_id, &init_info.vdp_data,
				 &init_info.vdp_disp, &init_info.vdp_shadow);
		/* service call */
		ret = vdp_sec_service_call(TZCMD_VDP_INIT,
					   SERVICE_CALL_DIRECTION_INPUT,
					   &init_info, sizeof(init_info));
		if (ret != VDP_OK) {
			DISP_LOG_E("service call fail: cmd[%d] ret[%d]\n",
				   TZCMD_VDP_INIT, ret);
			return VDP_SERVICE_CALL_FAIL;
		}
	} while (0);

	return ret;
}

int disp_vdp_sec_set_log_level(int level)
{
	union MTEEC_PARAM vdp_param[4];
	int tz_ret;
	int status = 0;

	status = _vdp_sec_create_session();
	if (status != VDP_OK)
		return status;

	vdp_param[0].value.a = level;
	vdp_param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(vdp_session, TZCMD_VDP_SET_LOG_LEVEL,
				     TZ_ParamTypes1(TZPT_VALUE_INPUT),
				     vdp_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("service call fail: cmd[%d] ret[%d]\n",
			   TZCMD_VDP_SET_LOG_LEVEL, tz_ret);
		return VDP_SERVICE_CALL_FAIL;
	}
	return VDP_OK;
}

int disp_vdp_sec_stop_hw(int layer_id)
{
	union MTEEC_PARAM vdp_param[4];
	int tz_ret;

	vdp_param[0].value.a = (uint32_t)layer_id;
	vdp_param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(vdp_session, TZCMD_VDP_STOP,
				     TZ_ParamTypes1(TZPT_VALUE_INPUT),
				     vdp_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("service call fail: cmd[%d] ret[%d]\n",
			   TZCMD_VDP_STOP, tz_ret);
		return VDP_SERVICE_CALL_FAIL;
	}

	return tz_ret;
}

int disp_vdp_sec_disable_active_zone(int layer_id)
{
	union MTEEC_PARAM vdp_param[4];
	int tz_ret;

	vdp_param[0].value.a = (uint32_t)layer_id;
	vdp_param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(vdp_session, TZCMD_VDP_DISABLE_ACTIVE,
				     TZ_ParamTypes1(TZPT_VALUE_INPUT),
				     vdp_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("service call fail: cmd[%d] ret[%d]\n",
			   TZCMD_VDP_DISABLE_ACTIVE, tz_ret);
		return VDP_SERVICE_CALL_FAIL;
	}

	return tz_ret;
}

int disp_vdp_sec_deinit(unsigned int layer_id)
{
	int ret = VDP_OK;
	union MTEEC_PARAM vdp_param[4];
	int tz_ret;
	struct vdp_hal_data_info vdp_data;
	struct vdp_hal_disp_info vdp_disp;
	struct vdo_sw_shadow vdp_shadow;
	KREE_SHAREDMEM_HANDLE vdp_shm_handle1,
		vdp_shm_handle2, vdp_shm_handle3;
	struct KREE_SHAREDMEM_PARAM vdp_shm_param1,
		vdp_shm_param2, vdp_shm_param3;


	vdp_shm_param1.buffer = (void *)&vdp_data;
	vdp_shm_param1.size = sizeof(struct vdp_hal_data_info);

	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle1, &vdp_shm_param1);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E(
			"[VDP_CA]%s KREE_RegisterSharedmem1 fail\n",
			__func__);
		return VDP_FAIL;
	}

	vdp_shm_param2.buffer = (void *)&vdp_disp;
	vdp_shm_param2.size = sizeof(struct vdp_hal_disp_info);
	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle2, &vdp_shm_param2);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E(
			"[VDP_CA]%s KREE_RegisterSharedmem2 fail\n",
			__func__);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		return VDP_FAIL;
	}

	vdp_shm_param3.buffer = (void *)&vdp_shadow;
	vdp_shm_param3.size = sizeof(struct vdo_sw_shadow);
	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle3, &vdp_shm_param3);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E(
			"[VDP_CA]%s KREE_RegisterSharedmem3 fail\n",
			__func__);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
		return VDP_FAIL;
	}

	vdp_param[0].memref.handle = (uint32_t) vdp_shm_handle1;
	vdp_param[0].memref.offset = 0;
	vdp_param[0].memref.size = vdp_shm_param1.size;

	vdp_param[1].memref.handle = (uint32_t) vdp_shm_handle2;
	vdp_param[1].memref.offset = 0;
	vdp_param[1].memref.size = vdp_shm_param2.size;

	vdp_param[2].memref.handle = (uint32_t) vdp_shm_handle3;
	vdp_param[2].memref.offset = 0;
	vdp_param[2].memref.size = vdp_shm_param3.size;

	vdp_param[3].value.a = (uint32_t)layer_id;
	vdp_param[3].value.b = 0;

	tz_ret = KREE_TeeServiceCall(
		vdp_session, TZCMD_VDP_DEINIT,
		TZ_ParamTypes4(TZPT_MEMREF_INOUT,
		TZPT_MEMREF_INOUT, TZPT_MEMREF_INOUT,
		TZPT_VALUE_INPUT),
		vdp_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("service call fail: cmd[%d] ret[%d]\n",
			   TZCMD_VDP_DEINIT, tz_ret);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle3);
		return VDP_SERVICE_CALL_FAIL;
	}

	fmt_hal_set_secure(layer_id, false);

	#if 0 // need debug
	vdp_hal_set_info(layer_id,
			 (struct vdp_hal_data_info *)vdp_param[0].mem.buffer,
			 (struct vdp_hal_disp_info *)vdp_param[1].mem.buffer,
			 (struct vdo_sw_shadow *)vdp_param[2].mem.buffer);
	#endif

	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle3);
	return ret;
}

int disp_vdp_sec_config(struct vdp_hal_config_info *config_info,
			struct dispfmt_setting *dispfmt_info,
			uint32_t osd_enable)
{
	union MTEEC_PARAM vdp_param[4];
	int ret;
	struct tz_disp_hw_common_info tz_disp_common_info = {{0} };
	KREE_SHAREDMEM_HANDLE vdp_shm_handle1,
		vdp_shm_handle2, vdp_shm_handle3;
	struct KREE_SHAREDMEM_PARAM vdp_shm_param1,
		vdp_shm_param2, vdp_shm_param3;

	_vdp_sec_create_session();
	if (vdp_session == 0) {
		DISP_LOG_E(
			"%s error: vdp_session not created\n", __func__);
		return VDP_SESSION_NOT_CREATE;
	}

	REC_VDP_LINE(config_info->vdp_id, VDP_SEC_IN_1);

	memcpy(&tz_disp_common_info.resolution, disp_common_info.resolution,
	       sizeof(tz_disp_common_info.resolution));
	memcpy(&tz_disp_common_info.tv, &disp_common_info.tv,
	       sizeof(tz_disp_common_info.tv));
	tz_disp_common_info.osd_enable = osd_enable;


	vdp_shm_param1.buffer = config_info;
	vdp_shm_param1.size = sizeof(struct vdp_hal_config_info);

	REC_VDP_LINE(config_info->vdp_id, VDP_SEC_IN_2);

	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle1, &vdp_shm_param1);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E(
			"[VDP_CA]%s KREE_RegisterSharedmem1 fail\n",
			__func__);
		return VDP_FAIL;
	}

	vdp_shm_param2.buffer = &tz_disp_common_info;
	vdp_shm_param2.size = sizeof(struct tz_disp_hw_common_info);

	REC_VDP_LINE(config_info->vdp_id, VDP_SEC_IN_3);

	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle2, &vdp_shm_param2);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E(
			"[VDP_CA]%s KREE_RegisterSharedmem2 fail\n",
			__func__);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		return VDP_FAIL;
	}

	vdp_shm_param3.buffer = dispfmt_info;
	vdp_shm_param3.size = sizeof(struct dispfmt_setting);

	REC_VDP_LINE(config_info->vdp_id, VDP_SEC_IN_4);
	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle3, &vdp_shm_param3);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E(
			"[VDP_CA]%s KREE_RegisterSharedmem3 fail\n",
			__func__);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
		return VDP_FAIL;
	}

	vdp_param[0].memref.handle = (uint32_t) vdp_shm_handle1;
	vdp_param[0].memref.offset = 0;
	vdp_param[0].memref.size = vdp_shm_param1.size;

	vdp_param[1].memref.handle = (uint32_t) vdp_shm_handle2;
	vdp_param[1].memref.offset = 0;
	vdp_param[1].memref.size = vdp_shm_param2.size;

	vdp_param[2].memref.handle = (uint32_t) vdp_shm_handle3;
	vdp_param[2].memref.offset = 0;
	vdp_param[2].memref.size = vdp_shm_param3.size;

	REC_VDP_LINE(config_info->vdp_id, VDP_SEC_IN_5);

	ret = KREE_TeeServiceCall(
		vdp_session, TZCMD_VDP_HAL_CONFIG,
		TZ_ParamTypes3(TZPT_MEMREF_INPUT,
		TZPT_MEMREF_INPUT, TZPT_MEMREF_INPUT),
		vdp_param);

	REC_VDP_LINE(config_info->vdp_id, VDP_SEC_IN_6);

	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("service call fail: cmd[%d] ret[%d]\n",
			   TZCMD_VDP_HAL_CONFIG, ret);
		DISP_LOG_E(
			"HDMI_VIDEO_RESOLUTION:%zu uint16_t:%zu bool:%zu long long:%zu\n",
			sizeof(enum HDMI_VIDEO_RESOLUTION), sizeof(uint16_t),
			sizeof(bool), sizeof(long long));
		DISP_LOG_E("(char *):%zu (uint64_t)(%zu) (int)(%zu)\n",
			   sizeof(char *), sizeof(uint64_t), sizeof(int));
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle3);
		return VDP_SERVICE_CALL_FAIL;
	}

	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle3);
	return VDP_OK;
}

static int _vdp_sec_map_direction(enum SERVICE_CALL_DIRECTION direct,
				  enum TZ_PARAM_TYPES *pType)
{
	switch (direct) {
	case SERVICE_CALL_DIRECTION_INPUT:
		*pType = TZPT_MEMREF_INPUT;
		break;
	case SERVICE_CALL_DIRECTION_OUTPUT:
		*pType = TZPT_MEMREF_OUTPUT;
		break;
	case SERVICE_CALL_DIRECTION_INOUT:
		*pType = TZPT_MEMREF_INOUT;
		break;
	default:
		DISP_LOG_E("invalid service call direction:%d\n", direct);
		return VDP_FAIL;
	}
	return VDP_OK;
}


int vdp_sec_service_call(enum SERVICE_CALL_CMD cmd,
			 enum SERVICE_CALL_DIRECTION direct, void *buffer,
			 uint32_t size)
{
	unsigned int paramTypes = 0;
	union MTEEC_PARAM vdp_param[4];
	enum TZ_PARAM_TYPES type;
	KREE_SHAREDMEM_HANDLE vdp_shm_handle1,
		vdp_shm_handle2, vdp_shm_handle3, vdp_shm_handle4;
	struct KREE_SHAREDMEM_PARAM vdp_shm_param1,
		vdp_shm_param2, vdp_shm_param3, vdp_shm_param4;
	struct vdp_sec_init_info *vdp_sec_info = NULL;
	int ret;
	int status = 0;

	vdp_sec_info = (struct vdp_sec_init_info *)buffer;

	status = _vdp_sec_create_session();
	if (status != VDP_OK)
		return status;

	status = _vdp_sec_map_direction(direct, &type);
	if (status != VDP_OK)
		return status;

	paramTypes = TZ_ParamTypes4(type, type, type, type);

	/*register share memory*/
	vdp_shm_param1.buffer = (void *)(&vdp_sec_info->vdp_data);
	vdp_shm_param1.size = sizeof(struct vdp_hal_data_info);

	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle1, &vdp_shm_param1);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("[VDP_CA]KREE_RegisterSharedmem1 fail\n");
		return VDP_FAIL;
	}

	vdp_shm_param2.buffer = (void *)(&vdp_sec_info->vdp_disp);
	vdp_shm_param2.size = sizeof(struct vdp_hal_disp_info);
	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle2, &vdp_shm_param2);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("[VDP_CA]KREE_RegisterSharedmem2 fail\n");
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		return VDP_FAIL;
	}

	vdp_shm_param3.buffer = (void *)(&vdp_sec_info->vdp_shadow);
	vdp_shm_param3.size = sizeof(struct vdo_sw_shadow);
	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle3, &vdp_shm_param3);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("[VDP_CA]KREE_RegisterSharedmem3 fail\n");
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
		return VDP_FAIL;
	}

	vdp_shm_param4.buffer = buffer;
	vdp_shm_param4.size = size;
	ret = KREE_RegisterSharedmem(vdp_mem_session,
		&vdp_shm_handle4, &vdp_shm_param4);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("[VDP_CA]KREE_RegisterSharedmem3 fail\n");
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle3);
		return VDP_FAIL;
	}

	vdp_param[0].memref.handle = (uint32_t) vdp_shm_handle1;
	vdp_param[0].memref.offset = 0;
	vdp_param[0].memref.size = vdp_shm_param1.size;

	vdp_param[1].memref.handle = (uint32_t) vdp_shm_handle2;
	vdp_param[1].memref.offset = 0;
	vdp_param[1].memref.size = vdp_shm_param2.size;

	vdp_param[2].memref.handle = (uint32_t) vdp_shm_handle3;
	vdp_param[2].memref.offset = 0;
	vdp_param[2].memref.size = vdp_shm_param3.size;

	vdp_param[3].memref.handle = (uint32_t) vdp_shm_handle4;
	vdp_param[3].memref.offset = 0;
	vdp_param[3].memref.size = vdp_shm_param4.size;

	ret = KREE_TeeServiceCall(vdp_session, cmd, paramTypes, vdp_param);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_LOG_E("service call fail: cmd[%d] ret[%d]\n", cmd, ret);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle3);
		KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle4);
		return VDP_SERVICE_CALL_FAIL;
	}
	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle1);
	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle2);
	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle3);
	KREE_UnregisterSharedmem(vdp_mem_session, vdp_shm_handle4);
	return VDP_OK;
}

#else /*  */

int vdp_sec_service_call(enum SERVICE_CALL_CMD cmd,
			 enum SERVICE_CALL_DIRECTION direct, void *buffer,
			 uint32_t size)
{
	return VDP_OK;
}

int disp_vdp_sec_init(unsigned int layer_id, uint32_t normal_mva,
		      unsigned int normal_mva_size)
{
	return VDP_OK;
}

int disp_vdp_sec_stop_hw(int layer_id)
{
	return VDP_OK;
}

int disp_vdp_sec_deinit(unsigned int layer_id)
{
	return VDP_OK;
}

int disp_vdp_sec_config(struct vdp_hal_config_info *config_info,
			struct dispfmt_setting *dispfmt_info,
			uint32_t osd_enable)
{
	return VDP_OK;
}

#endif /*  */
