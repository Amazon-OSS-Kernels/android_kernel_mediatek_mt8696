// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
 */
#include "pseudo_m4u.h"
#include <dt-bindings/memory/mt8696-larb-port.h>

/*
 * Get the local arbiter ID and the portid within the larb arbiter
 * from mtk_m4u_id which is defined by MTK_M4U_ID.
 */
#define MTK_M4U_TO_LARB(id)		(((id) >> 5) & 0xf)
#define MTK_M4U_TO_PORT(id)		((id) & 0x1f)
bool m4u_portid_valid(const int portID)
{
	unsigned int larb = MTK_M4U_TO_LARB(portID);

	return !!(larb != 2 && larb <= 8);
}

/* portID is from dt-bindings/memory/mtxx-larb-port.h */
/* Only print the port name for debug. */
const char *m4u_get_port_name(const int portID)
{
	const char *p;

	switch (portID) {
	case M4U_PORT_UHD_OSD_HEADER:
		p = "M4U_PORT_UHD_OSD_HEADER";
		break;
	case M4U_PORT_UHD_OSD:
		p = "M4U_PORT_UHD_OSD";
		break;
	case M4U_PORT_UHD_HDR_GFX_FE_ADL:
		p = "M4U_PORT_UHD_HDR_GFX_FE_ADL";
		break;
	case M4U_PORT_DISP_FAKE0:
		p = "M4U_PORT_DISP_FAKE0";
		break;

	/* larb1 */
	case M4U_PORT_HW_VDEC_MC_EXT:
		p = "M4U_PORT_HW_VDEC_MC_EXT";
		break;
	case M4U_PORT_HW_VDEC_UFO_EXT:
		p = "M4U_PORT_HW_VDEC_UFO_EXT";
		break;
	case M4U_PORT_HW_VDEC_PP_EXT:
		p = "M4U_PORT_HW_VDEC_PP_EXT";
		break;
	case M4U_PORT_HW_VDEC_PRED_RD_EXT:
		p = "M4U_PORT_HW_VDEC_PRED_RD_EXT";
		break;
	case M4U_PORT_HW_VDEC_PRED_WR_EXT:
		p = "M4U_PORT_HW_VDEC_PRED_WR_EXT";
		break;
	case M4U_PORT_HW_VDEC_PPWRAP_EXT:
		p = "M4U_PORT_HW_VDEC_PPWRAP_EXT";
		break;
	case M4U_PORT_HW_VDEC_TILE_EXT:
		p = "M4U_PORT_HW_VDEC_TILE_EXT";
		break;
	case M4U_PORT_HW_VDEC_VLD_EXT:
		p = "M4U_PORT_HW_VDEC_VLD_EXT";
		break;
	case M4U_PORT_HW_VDEC_VLD2_EXT:
		p = "M4U_PORT_HW_VDEC_VLD2_EXT";
		break;
	case M4U_PORT_HW_VDEC_AVC_MV_EXT:
		p = "M4U_PORT_HW_VDEC_AVC_MV_EXT";
		break;

	/* larb3 */
	case M4U_PORT_VENC_RCPU:
		p = "M4U_PORT_VENC_RCPU";
		break;
	case M4U_PORT_VENC_REC_FRM:
		p = "M4U_PORT_VENC_REC_FRM";
		break;
	case M4U_PORT_VENC_BSDMA:
		p = "M4U_PORT_VENC_BSDMA";
		break;
	case M4U_PORT_VENC_SV_COMV:
		p = "M4U_PORT_VENC_SV_COMV";
		break;
	case M4U_PORT_VENC_RD_COMV:
		p = "M4U_PORT_VENC_RD_COMV";
		break;
	case M4U_PORT_VENC_CUR_CHROMA:
		p = "M4U_PORT_VENC_CUR_CHROMA";
		break;
	case M4U_PORT_VENC_REF_CHROMA:
		p = "M4U_PORT_VENC_REF_CHROMA";
		break;
	case M4U_PORT_VENC_CUR_LUMA:
		p = "M4U_PORT_VENC_CUR_LUMA";
		break;
	case M4U_PORT_VENC_REF_LUMA:
		p = "M4U_PORT_VENC_REF_LUMA";
		break;

	/* larb4 */
	case M4U_PORT_FHD_OSD_HEADER:
		p = "M4U_PORT_FHD_OSD_HEADER";
		break;
	case M4U_PORT_FHD_OSD:
		p = "M4U_PORT_FHD_OSD";
		break;
	case M4U_PORT_VIDEO_IN_CH0:
		p = "M4U_PORT_VIDEO_IN_CH0";
		break;
	case M4U_PORT_VIDEO_IN_CH1:
		p = "M4U_PORT_VIDEO_IN_CH1";
		break;
	case M4U_PORT_VIDEO_IN_CH2:
		p = "M4U_PORT_VIDEO_IN_CH2";
		break;
	case M4U_PORT_W2D_CH0:
		p = "M4U_PORT_W2D_CH0";
		break;
	case M4U_PORT_W2D_CH1:
		p = "M4U_PORT_W2D_CH1";
		break;
	case M4U_PORT_W2D_CH2:
		p = "M4U_PORT_W2D_CH2";
		break;
	case M4U_PORT_HDR_VDO_BE_ADL:
		p = "M4U_PORT_HDR_VDO_BE_ADL";
		break;
	case M4U_PORT_HDR_VDO_BE_ML:
		p = "M4U_PORT_HDR_VDO_BE_ML";
		break;
	case M4U_PORT_DISP_FAKE4:
		p = "M4U_PORT_DISP_FAKE4";
		break;

	/* larb5 */
	case M4U_PORT_VDO3_UFOD_Y_LENGTH:
		p = "M4U_PORT_VDO3_UFOD_Y_LENGTH";
		break;
	case M4U_PORT_VDO3_UFOD_C_LENGTH:
		p = "M4U_PORT_VDO3_UFOD_C_LENGTH";
		break;
	case M4U_PORT_VDO3_UFOD_Y0:
		p = "M4U_PORT_VDO3_UFOD_Y0";
		break;
	case M4U_PORT_VDO3_UFOD_Y2:
		p = "M4U_PORT_VDO3_UFOD_Y2";
		break;
	case M4U_PORT_VDO3_UFOD_C1:
		p = "M4U_PORT_VDO3_UFOD_C1";
		break;
	case M4U_PORT_VDO3_UFOD_C3:
		p = "M4U_PORT_VDO3_UFOD_C3";
		break;
	case M4U_PORT_VDO4_UFOD_C0:
		p = "M4U_PORT_VDO4_UFOD_C0";
		break;
	case M4U_PORT_VDO4_UFOD_Y1:
		p = "M4U_PORT_VDO4_UFOD_Y1";
		break;
	case M4U_PORT_R2R_Y:
		p = "M4U_PORT_R2R_Y";
		break;
	case M4U_PORT_IMG_UFO_REQ:
		p = "M4U_PORT_IMG_UFO_REQ";
		break;
	case M4U_PORT_IMG_LEN_REQ:
		p = "M4U_PORT_IMG_LEN_REQ";
		break;
	case M4U_PORT_IMG_WR_REQ:
		p = "M4U_PORT_IMG_WR_REQ";
		break;
	case M4U_PORT_M_VDO_ADL:
		p = "M4U_PORT_M_VDO_ADL";
		break;
	case M4U_PORT_M_VDO_ML:
		p = "M4U_PORT_M_VDO_ML";
		break;
	case M4U_PORT_DISP_FAKE5:
		p = "M4U_PORT_DISP_FAKE5";
		break;

	/* larb6 */
	case M4U_PORT_VDO4_UFOD_Y_LENGTH:
		p = "M4U_PORT_VDO4_UFOD_Y_LENGTH";
		break;
	case M4U_PORT_VDO4_UFOD_C_LENGTH:
		p = "M4U_PORT_VDO4_UFOD_C_LENGTH";
		break;
	case M4U_PORT_VDO4_UFOD_Y0:
		p = "M4U_PORT_VDO4_UFOD_Y0";
		break;
	case M4U_PORT_VDO4_UFOD_C1:
		p = "M4U_PORT_VDO4_UFOD_C1";
		break;
	case M4U_PORT_VDO3_UFOD_Y1:
		p = "M4U_PORT_VDO3_UFOD_Y1";
		break;
	case M4U_PORT_VDO3_UFOD_Y3:
		p = "M4U_PORT_VDO3_UFOD_Y3";
		break;
	case M4U_PORT_VDO3_UFOD_C0:
		p = "M4U_PORT_VDO3_UFOD_C0";
		break;
	case M4U_PORT_VDO3_UFOD_C2:
		p = "M4U_PORT_VDO3_UFOD_C2";
		break;
	case M4U_PORT_R2R_C:
		p = "M4U_PORT_R2R_C";
		break;
	case M4U_PORT_R2R_CC:
		p = "M4U_PORT_R2R_CC";
		break;
	case M4U_PORT_IMG_UFO_REQ_CH2:
		p = "M4U_PORT_IMG_UFO_REQ_CH2";
		break;
	case M4U_PORT_IMG_LEN_REQ_CH2:
		p = "M4U_PORT_IMG_LEN_REQ_CH2";
		break;
	case M4U_PORT_IMG_WR_REQ_CH2:
		p = "M4U_PORT_IMG_WR_REQ_CH2";
		break;
	case M4U_PORT_IRT_DMA_RW:
		p = "M4U_PORT_IRT_DMA_RW";
		break;
	case M4U_PORT_S_VDO_ADL:
		p = "M4U_PORT_S_VDO_ADL";
		break;
	case M4U_PORT_S_VDO_ML:
		p = "M4U_PORT_S_VDO_ML";
		break;
	case M4U_PORT_DISP_FAKE6:
		p = "M4U_PORT_DISP_FAKE6";
		break;

	/* larb7 */
	case M4U_PORT_HW_VDEC_LAT_VLD_EXT:
		p = "M4U_PORT_HW_VDEC_LAT_VLD_EXT";
		break;
	case M4U_PORT_HW_VDEC_LAT_VLD2_EXT:
		p = "M4U_PORT_HW_VDEC_LAT_VLD2_EXT";
		break;
	case M4U_PORT_HW_VDEC_LAT_AVC_MV_EXT:
		p = "M4U_PORT_HW_VDEC_LAT_AVC_MV_EXT";
		break;
	case M4U_PORT_HW_VDEC_LAT_PRED_RD_EXT:
		p = "M4U_PORT_HW_VDEC_LAT_PRED_RD_EXT";
		break;
	case M4U_PORT_HW_VDEC_LAT_TILE_EXT:
		p = "M4U_PORT_HW_VDEC_LAT_TILE_EXT";
		break;
	case M4U_PORT_HW_VDEC_LAT_WDMA_EXT:
		p = "M4U_PORT_HW_VDEC_LAT_WDMA_EXT";
		break;
	case M4U_PORT_HW_VDEC_UFO_ENC_EXT:
		p = "M4U_PORT_HW_VDEC_UFO_ENC_EXT";
		break;

	/* larb8 */
	case M4U_PORT_IOMMU_READ:
		p = "M4U_PORT_IOMMU_READ";
		break;
	case M4U_PORT_IOMMU_WRITE:
		p = "M4U_PORT_IOMMU_WRITE";
		break;
	default:
		p = "UNKNOWN PORT";
		break;
	}

	return p;
}
