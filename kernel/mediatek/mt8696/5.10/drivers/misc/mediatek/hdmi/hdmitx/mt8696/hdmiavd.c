// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "hdmiavd.h"
#include "hdmictrl.h"
#include "hdmihdcp.h"

void av_hdmiset(
enum AV_D_HDMI_DRV_SET_TYPE_T e_set_type, const void *pv_set_info,
		unsigned char z_set_info_len)
{
	struct HDMI_AV_INFO_T *prAvInf;

	prAvInf = (struct HDMI_AV_INFO_T *) pv_set_info;

	switch (e_set_type) {
	case HDMI_SET_TURN_OFF_TMDS:
		vTmdsOnOffAndResetHdcp(prAvInf->fgHdmiTmdsEnable);
		break;

	case HDMI_SET_VPLL:
		vChangeVpll(prAvInf->e_resolution, prAvInf->e_deep_color_bit);
		break;


	case HDMI_SET_VIDEO_RES_CHG:
		vChgHDMIVideoResolution();
		break;

	case HDMI_SET_AUDIO_CHG_SETTING:
		vWriteByteHdmiGRL(0x900, 0xffff0200);//1006
		vChgHDMIAudioOutput(prAvInf->e_hdmi_fs, prAvInf->e_resolution,
				    prAvInf->e_deep_color_bit);
		break;

	case HDMI_SET_HDCP_INITIAL_AUTH:
		vHDCPInitAuth();
		break;

	case HDMI_SET_VIDEO_COLOR_SPACE:

		break;

	case HDMI_SET_SOFT_NCTS:
		/* vChgtoSoftNCTS(prAvInf->e_resolution,
		 *prAvInf->u1audiosoft, prAvInf->e_hdmi_fs,
		 * prAvInf->e_deep_color_bit);
		 */
		break;

	case HDMI_SET_HDCP_OFF:
		/* vDisableHDCP(prAvInf->u1hdcponoff); */
		break;

	default:
		break;
	}

}
