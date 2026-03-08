// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#if (defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) || defined(CONFIG_OPTEE))
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/byteorder/generic.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <hdmitx.h>
#include <hdmictrl.h>
#include <hdmi_ctrl.h>

#include "tz_cross/trustzone.h"
#include "tz_cross/ta_test.h"
#include "tz_cross/ta_mem.h"
#include "kree/system.h"
#include "kree/mem.h"

#include "tz_cross/ta_hdmitx.h"
#include "hdmi_ca.h"
#include "hdmihdcp.h"

KREE_SESSION_HANDLE ca_hdmi_handle;
KREE_SESSION_HANDLE hdmitx_mem_session;

bool fgCaHDMICreate(void)
{
	int tz_ret = 0;

	tz_ret = KREE_CreateSession(TZ_TA_HDMI_UUID, &ca_hdmi_handle);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		/* Should provide strerror style error string in UREE. */
		TX_DEF_LOG("Create ca_hdmi_handle Error: %d\n", tz_ret);
		return false;
	}
	TX_DEF_LOG("Create ca_hdmi_handle ok: %d\n", tz_ret);

	tz_ret = KREE_CreateSession(TZ_TA_MEM_UUID, &hdmitx_mem_session);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]Create memory session error %d\n", tz_ret);
		return false;
	}
	TX_DEF_LOG("Create ca_hdmi_memory_handle ok: %d\n", tz_ret);

	return true;
}

bool fgCaHDMIClose(void)
{
	int tz_ret = 0;

	tz_ret = KREE_CloseSession(ca_hdmi_handle);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		/* Should provide strerror style error string in UREE. */
		TX_DEF_LOG("Close ca_hdmi_handle Error: %d\n", tz_ret);
		return false;
	}
	TX_DEF_LOG("Close ca_hdmi_handle ok: %d\n", tz_ret);

	tz_ret = KREE_CloseSession(hdmitx_mem_session);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("Close HDMITX memory session error %d\n", tz_ret);
		return false;
	}
	TX_DEF_LOG("Close ca_hdmi_memory_handle ok: %d\n", tz_ret);

	return true;
}

void vCaHDMIWriteReg(unsigned int u4addr, unsigned int u4data)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[2];

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA]TEE ca_hdmi_handle=0\n");
		return;
	}

	param[0].value.a = u4addr & 0xFFF;
	param[0].value.b = 0;
	param[1].value.a = u4data;
	param[1].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_WRITE_REG,
				     TZ_ParamTypes2(TZPT_VALUE_INPUT,
				     TZPT_VALUE_INPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS)
		TX_DEF_LOG("[CA] HDMI_TA_WRITE_REG err:%X\n", tz_ret);

}

void vCaHDMIWriteHDCPRST(unsigned int u4addr, unsigned int u4data)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[2];

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA]TEE ca_hdmi_handle=0\n");
		return;
	}

	param[0].value.a = u4addr;
	param[0].value.b = 0;
	param[1].value.a = u4data;
	param[1].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_HDCP_RST,
				     TZ_ParamTypes2(TZPT_VALUE_INPUT,
				     TZPT_VALUE_INPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS)
		TX_DEF_LOG("[CA] HDMI_TA_WRITE_REG err:%X\n", tz_ret);

}

void vCaHDMIWriteHdcpCtrl(unsigned int u4addr, unsigned int u4data)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[2];

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG(" TEE ca_hdmi_handle=0\n");
		return;
	}

	param[0].value.a = u4addr;
	param[0].value.b = 0;
	param[1].value.a = u4data;
	param[1].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_WRITE_REG,
				     TZ_ParamTypes2(TZPT_VALUE_INPUT,
				     TZPT_VALUE_INPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS)
		TX_DEF_LOG("[CA]HDMI_TA_WRITE_REG err:%X\n", tz_ret);
}

bool fgCaHDMIInstallHdcpKey(unsigned char *pdata, unsigned int u4Len)
{
	TX_DEF_LOG("[CA] %s()\n", __func__);
	return true;

	/* HDMI service call KeyManager CA API in userspace,
	 * then KeyManager Ta deliver decrtpted hdcp key to
	 * HDMI Ta directly. Not use the obsoleted flow like:
	 * 1.HDMI service deliver encrypted key to
	 *   HDMI driver from userspace to kernel;
	 * 2.HDMI driver Ca deliver encrypted key to HDMI TA;
	 * 3.HDMI Ta decrypt the encrypted key.
	 */
}

bool fgCaHDMIGetEfuse(unsigned char *pdata)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[1];
	unsigned char *ptr;
	unsigned char i;
	KREE_SHAREDMEM_HANDLE hdmitx_shm_handle;
	struct KREE_SHAREDMEM_PARAM hdmitx_shm_param;

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	ptr = kmalloc(1, GFP_KERNEL);
	if (ptr == NULL) {
		TX_DEF_LOG("[CA] AKSV kmalloc failure\n");
		return false;
	}

	hdmitx_shm_param.buffer = ptr;
	hdmitx_shm_param.size = 1;
	tz_ret = KREE_RegisterSharedmem(hdmitx_mem_session,
		&hdmitx_shm_handle, &hdmitx_shm_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_RegisterSharedmem Error\n");
		return false;
	}

	param[0].memref.handle = (uint32_t) hdmitx_shm_handle;
	param[0].memref.offset = 0;
	param[0].memref.size = 1;
	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_GET_EFUSE,
			TZ_ParamTypes1(TZPT_MEMREF_OUTPUT), param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]HDMI_TA_GET_EFUSE err:%X\n", tz_ret);

		tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
			hdmitx_shm_handle);
		if (tz_ret != TZ_RESULT_SUCCESS)
			TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
		hdmitx_shm_handle);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	for (i = 0; i < 1; i++)
		pdata[i] = ptr[i];

	TX_DEF_LOG("[CA]y-t efuse : %x\n",
		   pdata[0]);
	kfree(ptr);
	return true;

}

bool fgCaHDMIGetAKsv(unsigned char *pdata)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[2];
	unsigned char *ptr;
	unsigned char i;
	KREE_SHAREDMEM_HANDLE hdmitx_shm_handle;
	struct KREE_SHAREDMEM_PARAM hdmitx_shm_param;

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	ptr = kmalloc(HDCP_AKSV_COUNT, GFP_KERNEL);
	if (ptr == NULL) {
		TX_DEF_LOG("[CA] AKSV kmalloc failure\n");
		return false;
	}

	hdmitx_shm_param.buffer = ptr;
	hdmitx_shm_param.size = HDCP_AKSV_COUNT;
	tz_ret = KREE_RegisterSharedmem(hdmitx_mem_session,
		&hdmitx_shm_handle, &hdmitx_shm_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_RegisterSharedmem Error\n");
		return false;
	}

	param[0].memref.handle = (uint32_t) hdmitx_shm_handle;
	param[0].memref.offset = 0;
	param[0].memref.size = HDCP_AKSV_COUNT;

#if (defined(CONFIG_MTK_HDMI_POLARITY_SWAP))
	param[1].value.a = 0;
#else
	param[1].value.a = 1;
#endif
	TX_DEF_LOG("[Kara]y-t aksvoffset : 0x%x", param[1].value.a);
	param[1].value.b = 0;
	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_GET_HDCP_AKSV,
			TZ_ParamTypes2(TZPT_MEMREF_OUTPUT,
			TZPT_VALUE_INPUT), param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]HDMI_TA_GET_HDCP_AKSV err:%X\n", tz_ret);

		tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
			hdmitx_shm_handle);
		if (tz_ret != TZ_RESULT_SUCCESS)
			TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
		hdmitx_shm_handle);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	for (i = 0; i < HDCP_AKSV_COUNT; i++)
		pdata[i] = ptr[i];

	TX_DEF_LOG("[CA]hdcp aksv : %x %x %x %x %x\n",
		   pdata[0], pdata[1], pdata[2], pdata[3], pdata[4]);
	kfree(ptr);
	return true;

}

bool fgCaHDMIGetTAStatus(unsigned char *pdata)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[1];
	unsigned char *ptr;
	KREE_SHAREDMEM_HANDLE hdmitx_shm_handle;
	struct KREE_SHAREDMEM_PARAM hdmitx_shm_param;

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	ptr = kmalloc(2, GFP_KERNEL);
	if (ptr == NULL) {
		TX_DEF_LOG("[CA]%s\n", __func__);
		return false;
	}

	hdmitx_shm_param.buffer = ptr;
	hdmitx_shm_param.size = 2;
	tz_ret = KREE_RegisterSharedmem(hdmitx_mem_session,
		&hdmitx_shm_handle, &hdmitx_shm_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_RegisterSharedmem Error\n");
		return false;
	}

	param[0].memref.handle = (uint32_t) hdmitx_shm_handle;
	param[0].memref.offset = 0;
	param[0].memref.size = 2;
	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_READ_STATUS,
				     TZ_ParamTypes1(TZPT_MEMREF_OUTPUT), param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]%s err:%X\n", __func__, tz_ret);

		tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
			hdmitx_shm_handle);
		if (tz_ret != TZ_RESULT_SUCCESS)
			TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
		hdmitx_shm_handle);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	pdata[0] = ptr[0];
	pdata[1] = ptr[1];

	kfree(ptr);
	return true;
}

bool fgCaHDMILoadHDCPKey(void)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[1];

	TX_DEF_LOG("[CA] %s\n", __func__);
	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

#if (defined(CONFIG_MTK_HDMI_POLARITY_SWAP))
	param[0].value.a = 8;
#else
	param[0].value.a = 6;
#endif
	TX_DEF_LOG("[Kara]y-t keyoffset : 0x%x", param[0].value.a);
	param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_LOAD_HDCP_KEY,
				     TZ_ParamTypes1(TZPT_VALUE_INPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA] HDMI_TA_LOAD_HDCP_KEY err:%X\n", tz_ret);
		return false;
	}
	return true;
}

bool fgCaHDMILoadROM(void)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[1];

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	param[0].value.a = 0;
	param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_LOAD_ROM,
				     TZ_ParamTypes1(TZPT_VALUE_INPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]HDMI_TA_LOAD_ROM err:%X\n", tz_ret);
		return false;
	}
	return true;
}

bool fgCaHDMISetContentStreamManage(unsigned int u4ContentStreamManage)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[1];

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	if (u4ContentStreamManage == CA_HDCP_CONTENT_STREAM_MANAGEMENT_TYPE_0)
		param[0].value.a = TA_HDCP_CONTENT_STREAM_MANAGEMENT_TYPE_0;
	else if (u4ContentStreamManage == CA_HDCP_CONTENT_STREAM_MANAGEMENT_TYPE_1)
		param[0].value.a = TA_HDCP_CONTENT_STREAM_MANAGEMENT_TYPE_1;
	else {
		TX_DEF_LOG("[CA] unknown content stream type: 0x%X\n", u4ContentStreamManage);
		return false;
	}
	param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_SET_CONTENT_STREAM_MANAGE,
				TZ_ParamTypes1(TZPT_VALUE_INPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]HDMI_TA_SET_CONTENT_STREAM_MANAGE err:%X\n", tz_ret);
		return false;
	}
	return true;
}


bool fgCaHDMIGetContentStreamManage(unsigned int *pdata)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[1];

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	param[0].value.a = 0;
	param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_GET_CONTENT_STREAM_MANAGE,
				TZ_ParamTypes1(TZPT_VALUE_OUTPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]HDMI_TA_GET_CONTENT_STREAM_MANAGE err:%X\n", tz_ret);
		return false;
	}
	*pdata = param[0].value.a;
	return true;
}


bool fgCaHDMITestHDCPVersion(void)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[1];

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	param[0].value.a = 0;
	param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_TEST_HDCP_VERSION,
				     TZ_ParamTypes1(TZPT_VALUE_INPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]HDMI_TA_TEST_HDCP_VERSION err:%X\n", tz_ret);
		return false;
	}
	return true;
}

void fgCaHDMISetTzLogLevel(unsigned int loglevel)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[2];

	if (ca_hdmi_handle == 0) {
		HDMI_HDCP_LOG(" TEE ca_hdmi_handle=0\n");
		return;
	}

	param[0].value.a = loglevel;
	param[0].value.b = 0;

	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle, HDMI_TA_SET_LOG_LEVEL,
				     TZ_ParamTypes1(TZPT_VALUE_INPUT), param);

	if (tz_ret != TZ_RESULT_SUCCESS)
		TX_DEF_LOG("[CA]HDMI_TA_SET_LOG_LEVEL err:%X\n", tz_ret);
}

/*get Hdr10p_VSIF_application_version from tz*/
bool fgCaHDMIGetHdr10pVSIFInfo(unsigned char *pdata)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[1];
	unsigned char *ptr;
	KREE_SHAREDMEM_HANDLE hdmitx_shm_handle;
	struct KREE_SHAREDMEM_PARAM hdmitx_shm_param;

	if (ca_hdmi_handle == 0) {
		TX_DEF_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	ptr = kmalloc(3, GFP_KERNEL);
	if (ptr == NULL) {
		TX_DEF_LOG("[CA] hdr_status kmalloc fail!\n");
		return false;
	}
	hdmitx_shm_param.buffer = ptr;
	hdmitx_shm_param.size = 3;
	tz_ret = KREE_RegisterSharedmem(hdmitx_mem_session,
		&hdmitx_shm_handle, &hdmitx_shm_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_RegisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}
	param[0].memref.handle = (uint32_t) hdmitx_shm_handle;
	param[0].memref.offset = 0;
	param[0].memref.size = 3;
	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle,
		HDMI_TA_GET_HDR10p_VSIF_Info,
				     TZ_ParamTypes1(TZPT_MEMREF_OUTPUT), param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]HDMI_TA_GET_HDR10p_VSIF_Info err:%X\n", tz_ret);

		tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
			hdmitx_shm_handle);
		if (tz_ret != TZ_RESULT_SUCCESS)
			TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
		hdmitx_shm_handle);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	pdata[0] = ptr[0];/*HDR10p VSIF application version value*/
	pdata[1] = ptr[1];/*HDR10p VSIF write enable or disable*/
	pdata[2] = ptr[2];/*HDR10p VSIF repeat enable or disable*/

	TX_DEF_LOG("[CA]HDR10p_VSIF_application_version : %x\n",
		   *pdata);
	kfree(ptr);
	return true;
}

/*get Hdr10p_VSIF_application_version from tz*/
bool fgCaHDMILoadEMP(unsigned char en, unsigned int num, unsigned char *pdata)
{
	int tz_ret = 0;
	union MTEEC_PARAM param[4];
	unsigned char *ptr;
	KREE_SHAREDMEM_HANDLE hdmitx_shm_handle;
	struct KREE_SHAREDMEM_PARAM hdmitx_shm_param;

	if (num > DOVI_VSEM_METADATA_MAX_PACKET) {
		TX_DEF_LOG("[CA] %s fail, too long\n", __func__);
		return false;
	}

	if (ca_hdmi_handle == 0) {
		TX_DEF_LOG("[CA] TEE ca_hdmi_handle=0\n");
		return false;
	}

	if (num > DOVI_VSEM_METADATA_MAX_PACKET) {
		TX_DEF_LOG("[CA] emp size err\n");
		return false;
	}

	ptr = kmalloc((num * 31), GFP_KERNEL);
	if (ptr == NULL) {
		TX_DEF_LOG("[CA] hdr_status kmalloc fail!\n");
		return false;
	}
	memcpy(ptr, pdata, (num * 31));

	hdmitx_shm_param.buffer = ptr;
	hdmitx_shm_param.size = num * 31;
	tz_ret = KREE_RegisterSharedmem(hdmitx_mem_session,
		&hdmitx_shm_handle, &hdmitx_shm_param);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_RegisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	param[0].value.a = en;
	param[0].value.b = 0;
	param[1].value.a = num;
	param[1].value.b = 0;
	param[2].memref.handle = (uint32_t) hdmitx_shm_handle;
	param[2].memref.offset = 0;
	param[2].memref.size = num * 31;
	emp_data_is_sending = true;
	tz_ret = KREE_TeeServiceCall(ca_hdmi_handle,
		HDMI_TA_LOAD_EMP,
		TZ_ParamTypes3(TZPT_VALUE_INPUT,
			TZPT_VALUE_INPUT,
			TZPT_MEMREF_INPUT), param);
	emp_data_is_sending = false;
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]HDMI_TA_LOAD_EMP err:%X\n", tz_ret);
		tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
			hdmitx_shm_handle);
		if (tz_ret != TZ_RESULT_SUCCESS)
			TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	tz_ret = KREE_UnregisterSharedmem(hdmitx_mem_session,
		hdmitx_shm_handle);
	if (tz_ret != TZ_RESULT_SUCCESS) {
		TX_DEF_LOG("[CA]KREE_UnregisterSharedmem Error\n");
		kfree(ptr);
		return false;
	}

	kfree(ptr);
	return true;
}
#endif
