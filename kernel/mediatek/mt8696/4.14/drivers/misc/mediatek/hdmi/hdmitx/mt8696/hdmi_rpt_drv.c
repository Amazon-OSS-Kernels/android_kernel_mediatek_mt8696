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
#ifdef CONFIG_MT8532_HDMITX

#include <linux/kernel.h>
#include "../../../hdmirx/mtk_hdmi_rpt.h"
#include "hdmictrl.h"

enum hdmi_mode_setting hdmi_mode;

void vSetRptMode(struct device *dev,
	enum hdmi_mode_setting mode)
{
	hdmi_mode = mode;

	if (hdmi_mode == HDMI_RPT_PIXEL_MODE) {
		vWriteHdmiGRLMsk(HDMITX_CONFIG, 0, HDMITX_MUX);
		vWriteHdmiGRLMsk(HDMI_BYPS_CFG, 0, rg_full_byps_mode);
	} else if (hdmi_mode == HDMI_RPT_TMDS_MODE) {
		vWriteHdmiGRLMsk(HDMI_BYPS_CFG,
			rg_full_byps_mode, rg_full_byps_mode);
	} else if (hdmi_mode == HDMI_RPT_TMDS_MODE) {
		vWriteHdmiGRLMsk(HDMITX_CONFIG, HDMITX_MUX, HDMITX_MUX);
		vWriteHdmiGRLMsk(HDMI_BYPS_CFG,
			rg_full_byps_mode, rg_full_byps_mode);
	}
}

bool bGetEdid(struct device *dev,
		u8 *p,
		u32 num,
		u32 len)
{
	return 0;
}

void vHdcpStart(struct device *dev,
	bool hdcp_on)
{

}

u8 cHdcpVer(struct device *dev)
{
	return 0;
}

void vHdmiEnable(struct device *dev,
	bool en)
{

}

bool bIsPlugIn(struct device *dev)
{
	return 0;
}

void vSignalOff(struct device *dev)
{

}

void vTxSendPkt(struct device *dev,
	enum packet_type type,
	bool enable,
	u8 *pkt_data)
{

}

bool bVideoConfig(struct device *dev,
	struct MTK_VIDEO_PARA *vid_para)
{
	return 0;
}

bool bAudioConfig(struct device *dev, u8 aud)
{
	vChgHDMIAudioOutput(0, 0, 0);
	return 0;
}

#endif
