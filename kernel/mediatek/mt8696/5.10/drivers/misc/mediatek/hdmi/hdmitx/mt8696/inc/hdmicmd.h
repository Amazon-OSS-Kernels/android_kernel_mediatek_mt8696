/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __hdmihdmicmd_h__
#define __hdmihdmicmd_h__
#include "hdmihdcp.h"

extern unsigned char cDstStr[60];
extern unsigned char cDstBitStr[30];
extern unsigned char _bEdidData2[256];
extern bool new_edid;
extern void hdmi_InfoframeSetting(unsigned char i1typemode,
	unsigned char i1typeselect);
extern void hdmi_fmtsetting(unsigned int resolutionmode);
extern void rgb2hdmi_setting(unsigned int resolutionmode);
extern void hdmitx_configsetting(unsigned int resolutionmode);
extern const unsigned char _cFsStr[][7];
extern const unsigned char _cBitdeepStr[][7];
extern void HDMI_EnableIrq(void);
extern void HDMI_DisableIrq(void);
extern int hdmi_audiosetting(struct HDMITX_AUDIO_PARA *audio_para);
extern void HdcpService(enum HDCP_CTRL_STATE_T e_hdcp_state);
extern void vHDCP2XInitAuth(void);

#endif
