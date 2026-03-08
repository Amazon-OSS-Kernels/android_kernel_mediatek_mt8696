/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Scott Wang <scott.wang@mediatek.com>
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
#ifndef _DTS_IOMMU_PORT_MT8696_H_
#define _DTS_IOMMU_PORT_MT8696_H_

#define MTK_M4U_ID(larb, port)	(((larb) << 5) | (port))

/* larb0 */
#define M4U_PORT_UHD_OSD_HEADER		MTK_M4U_ID(0, 0)
#define M4U_PORT_UHD_OSD		MTK_M4U_ID(0, 1)
#define M4U_PORT_UHD_HDR_GFX_FE_ADL	MTK_M4U_ID(0, 2)
#define M4U_PORT_DISP_FAKE0		MTK_M4U_ID(0, 3)

/* larb1 */
#define M4U_PORT_HW_VDEC_MC_EXT		MTK_M4U_ID(1, 0)
#define M4U_PORT_HW_VDEC_UFO_EXT	MTK_M4U_ID(1, 1)
#define M4U_PORT_HW_VDEC_PP_EXT		MTK_M4U_ID(1, 2)
#define M4U_PORT_HW_VDEC_PRED_RD_EXT	MTK_M4U_ID(1, 3)
#define M4U_PORT_HW_VDEC_PRED_WR_EXT	MTK_M4U_ID(1, 4)
#define M4U_PORT_HW_VDEC_PPWRAP_EXT	MTK_M4U_ID(1, 5)
#define M4U_PORT_HW_VDEC_TILE_EXT	MTK_M4U_ID(1, 6)
#define M4U_PORT_HW_VDEC_VLD_EXT	MTK_M4U_ID(1, 7)
#define M4U_PORT_HW_VDEC_VLD2_EXT	MTK_M4U_ID(1, 8)
#define M4U_PORT_HW_VDEC_AVC_MV_EXT	MTK_M4U_ID(1, 9)

/* larb3 */
#define M4U_PORT_VENC_RCPU		MTK_M4U_ID(3, 0)
#define M4U_PORT_VENC_REC_FRM		MTK_M4U_ID(3, 1)
#define M4U_PORT_VENC_BSDMA		MTK_M4U_ID(3, 2)
#define M4U_PORT_VENC_SV_COMV		MTK_M4U_ID(3, 3)
#define M4U_PORT_VENC_RD_COMV		MTK_M4U_ID(3, 4)
#define M4U_PORT_VENC_CUR_CHROMA	MTK_M4U_ID(3, 5)
#define M4U_PORT_VENC_REF_CHROMA	MTK_M4U_ID(3, 6)
#define M4U_PORT_VENC_CUR_LUMA		MTK_M4U_ID(3, 7)
#define M4U_PORT_VENC_REF_LUMA		MTK_M4U_ID(3, 8)

/* larb4 */
#define M4U_PORT_FHD_OSD_HEADER		MTK_M4U_ID(4, 0)
#define M4U_PORT_FHD_OSD		MTK_M4U_ID(4, 1)
#define M4U_PORT_VIDEO_IN_CH0		MTK_M4U_ID(4, 2)
#define M4U_PORT_VIDEO_IN_CH1		MTK_M4U_ID(4, 3)
#define M4U_PORT_VIDEO_IN_CH2		MTK_M4U_ID(4, 4)
#define M4U_PORT_W2D_CH0		MTK_M4U_ID(4, 5)
#define M4U_PORT_W2D_CH1		MTK_M4U_ID(4, 6)
#define M4U_PORT_W2D_CH2		MTK_M4U_ID(4, 7)
#define M4U_PORT_HDR_VDO_BE_ADL		MTK_M4U_ID(4, 8)
#define M4U_PORT_HDR_VDO_BE_ML		MTK_M4U_ID(4, 9)
#define M4U_PORT_DISP_FAKE4		MTK_M4U_ID(4, 10)

/* larb5 */
#define M4U_PORT_VDO3_UFOD_Y_LENGTH	MTK_M4U_ID(5, 0)
#define M4U_PORT_VDO3_UFOD_C_LENGTH	MTK_M4U_ID(5, 1)
#define M4U_PORT_VDO3_UFOD_Y0		MTK_M4U_ID(5, 2)
#define M4U_PORT_VDO3_UFOD_Y2		MTK_M4U_ID(5, 3)
#define M4U_PORT_VDO3_UFOD_C1		MTK_M4U_ID(5, 4)
#define M4U_PORT_VDO3_UFOD_C3		MTK_M4U_ID(5, 5)
#define M4U_PORT_VDO4_UFOD_C0		MTK_M4U_ID(5, 6)
#define M4U_PORT_VDO4_UFOD_Y1		MTK_M4U_ID(5, 7)
#define M4U_PORT_R2R_Y			MTK_M4U_ID(5, 8)
#define M4U_PORT_IMG_UFO_REQ		MTK_M4U_ID(5, 9)
#define M4U_PORT_IMG_LEN_REQ		MTK_M4U_ID(5, 10)
#define M4U_PORT_IMG_WR_REQ		MTK_M4U_ID(5, 11)
#define M4U_PORT_M_VDO_ADL		MTK_M4U_ID(5, 12)
#define M4U_PORT_M_VDO_ML		MTK_M4U_ID(5, 13)
#define M4U_PORT_DISP_FAKE5		MTK_M4U_ID(5, 14)

/* larb6 */
#define M4U_PORT_VDO4_UFOD_Y_LENGTH	MTK_M4U_ID(6, 0)
#define M4U_PORT_VDO4_UFOD_C_LENGTH	MTK_M4U_ID(6, 1)
#define M4U_PORT_VDO4_UFOD_Y0		MTK_M4U_ID(6, 2)
#define M4U_PORT_VDO4_UFOD_C1		MTK_M4U_ID(6, 3)
#define M4U_PORT_VDO3_UFOD_Y1		MTK_M4U_ID(6, 4)
#define M4U_PORT_VDO3_UFOD_Y3		MTK_M4U_ID(6, 5)
#define M4U_PORT_VDO3_UFOD_C0		MTK_M4U_ID(6, 6)
#define M4U_PORT_VDO3_UFOD_C2		MTK_M4U_ID(6, 7)
#define M4U_PORT_R2R_C			MTK_M4U_ID(6, 8)
#define M4U_PORT_R2R_CC			MTK_M4U_ID(6, 9)
#define M4U_PORT_IMG_UFO_REQ_CH2	MTK_M4U_ID(6, 10)
#define M4U_PORT_IMG_LEN_REQ_CH2	MTK_M4U_ID(6, 11)
#define M4U_PORT_IMG_WR_REQ_CH2		MTK_M4U_ID(6, 12)
#define M4U_PORT_IRT_DMA_RW		MTK_M4U_ID(6, 13)
#define M4U_PORT_S_VDO_ADL		MTK_M4U_ID(6, 14)
#define M4U_PORT_S_VDO_ML		MTK_M4U_ID(6, 15)
#define M4U_PORT_DISP_FAKE6		MTK_M4U_ID(6, 16)

/* larb7 */
#define M4U_PORT_HW_VDEC_LAT_VLD_EXT		MTK_M4U_ID(7, 0)
#define M4U_PORT_HW_VDEC_LAT_VLD2_EXT		MTK_M4U_ID(7, 1)
#define M4U_PORT_HW_VDEC_LAT_AVC_MV_EXT		MTK_M4U_ID(7, 2)
#define M4U_PORT_HW_VDEC_LAT_PRED_RD_EXT	MTK_M4U_ID(7, 3)
#define M4U_PORT_HW_VDEC_LAT_TILE_EXT		MTK_M4U_ID(7, 4)
#define M4U_PORT_HW_VDEC_LAT_WDMA_EXT		MTK_M4U_ID(7, 5)
#define M4U_PORT_HW_VDEC_UFO_ENC_EXT		MTK_M4U_ID(7, 6)

/*
 * larb8 DRAM_LARB: NR & VDO2 & WRITE_CHANNEL
 * larb8 only could enable iommu for read/write per larb, could not control
 * iommu enable per hw port.
 */
#define M4U_PORT_IOMMU_READ		MTK_M4U_ID(8, 0)
#define M4U_PORT_IOMMU_WRITE		MTK_M4U_ID(8, 1)

#endif
