/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _HDMI_CA_H_
#define _HDMI_CA_H_
#if (defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) || defined(CONFIG_OPTEE))


enum CA_HDCP_CONTENT_STREAM_MANAGEMENT_TYPE {
	//content stream may be transmitted by repeater to all hdcp devices.
	CA_HDCP_CONTENT_STREAM_MANAGEMENT_TYPE_0 = 0x0,
	//content stream must not be transmitted by repeater to hdcp 1.x devices and 2.0 repeaters.
	CA_HDCP_CONTENT_STREAM_MANAGEMENT_TYPE_1 = 0x1,
	CA_HDCP_CONTENT_STREAM_MANAGEMENT_TYPE_RESERVE = 0xff,
};

bool fgCaHDMICreate(void);
bool fgCaHDMIClose(void);
void vCaHDMIWriteReg(unsigned int u4addr, unsigned int u4data);
bool fgCaHDMIInstallHdcpKey(unsigned char *pdata, unsigned int u4Len);
bool fgCaHDMIGetAKsv(unsigned char *pdata);
bool fgCaHDMIGetEfuse(unsigned char *pdata);
bool fgCaHDMILoadHDCPKey(void);
bool fgCaHDMILoadROM(void);
void vCaHDMIWriteHdcpCtrl(unsigned int u4addr, unsigned int u4data);

bool fgCaHDMITestHDCPVersion(void);
extern int hdmi_audio_signal_state(unsigned int state);
extern unsigned int _Cmd_GCPU_KP_Set(unsigned int bIndex);
extern void vCaHDMIWriteHDCPRST(unsigned int u4addr, unsigned int u4data);
void fgCaHDMISetTzLogLevel(unsigned int loglevel);
extern bool fgCaHDMIGetTAStatus(unsigned char *pdata);
extern bool fgCaHDMIGetHdr10pVSIFInfo(unsigned char *pdata);
extern bool fgCaHDMILoadEMP(unsigned char en,
	unsigned int num, unsigned char *pdata);
extern int hdmi_audio_signal_state(unsigned int state);
extern bool fgCaHDMISetContentStreamManage(unsigned int u4ContentStreamManage);
extern bool fgCaHDMIGetContentStreamManage(unsigned int *pdata);
#endif
#endif
