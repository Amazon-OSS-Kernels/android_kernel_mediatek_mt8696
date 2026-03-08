// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>

#include "mtk_drm_mmp.h"
#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_drv.h"
#include "mtk_log.h"
#include "mtk_dump.h"
#include "mtk_ethdr.h"
#include "mtk_disp_merge.h"
#include "mtk_dpi.h"
#include "mtk_hdmi.h"
#include "mtk_hdmi_hdr.h"
#include "mediatek_drm.h"

#define VDO1_CONFIG_SW1_RST_B 0x1D4
	#define HDR_ASYNC_RESET_BIT (BIT(19)|BIT(20)|BIT(21)|BIT(22)|BIT(23))

#define VDO1_CONFIG_HDR_BE_ASYNC_CFG_WD 0xE70
#define ROUND(x, y) ((x/y) + (((x%y) >= (y/2)) ? 1 : 0))
#define MIN(x, y) (((x) <= (y)) ? (x) : (y))
#define VDO1_CONFIG_MIXER_IN1_PAD 0xD40
	#define MIXER_IN1_MODE				REG_FLD(2, 0)
	#define MIXER_IN1_CH_SWAP			REG_FLD(1, 4)
	#define MIXER_IN1_BIWIDTH			REG_FLD(16, 16)
	#define MIXER_INx_MODE_BYPASS 0
	#define MIXER_INx_MODE_EVEN_EXTEND 1
	#define MIXER_INx_MODE_ODD_EXTEND 2
#define VDO1_CONFIG_MIXER_IN2_PAD 0xD44
	#define MIXER_IN2_MODE				REG_FLD(2, 0)
	#define MIXER_IN2_CH_SWAP			REG_FLD(1, 4)
	#define MIXER_IN2_BIWIDTH			REG_FLD(16, 16)
#define VDO1_CONFIG_MIXER_IN3_PAD 0xD48
	#define MIXER_IN3_MODE				REG_FLD(2, 0)
	#define MIXER_IN3_CH_SWAP			REG_FLD(1, 4)
	#define MIXER_IN3_BIWIDTH			REG_FLD(16, 16)
#define VDO1_CONFIG_MIXER_IN4_PAD 0xD4C
	#define MIXER_IN4_MODE				REG_FLD(2, 0)
	#define MIXER_IN4_CH_SWAP			REG_FLD(1, 4)
	#define MIXER_IN4_BIWIDTH			REG_FLD(16, 16)
#define MIX_INTEN 0x4
#define MIX_INTSTA 0x8
	#define MIX_FME_UND_INTSTA BIT(2)
	#define MIX_ABNORM_SOF_INTSTA BIT(5)
	#define MIX_START_INTSTA BIT(6)
#define VDO_FE_HSIZE 0x9CC
#define VDO_FE_VSIZE 0x9D0
#define GFX_DV_FE_VSIZE 0x3D0
#define GFX_TOSD_FE_VSIZE 0x468
#define VDO_BE_VSIZE 0x3D0

static DEFINE_MUTEX(g_set_hdmi_info_lock);

struct mtk_ethdr_data {
	bool tbd; //no use
};

struct device g_dev;
struct mtk_ethdr {
	struct mtk_ddp_comp ddp_comp;
	struct drm_crtc *crtc;
	struct mtk_ddp_comp ethdr_comp[ETHDR_ID_MAX];
	const struct mtk_ethdr_data *data;
	struct clk *async_clk[ETHDR_ASYNC_ID_MAX];
};

static struct dentry *ethdr_debugfs;
static char debug_buffer[128] = {0};
static unsigned int buf_offset;
static struct ST_ETHDR_INTERFACE _stEthdrInterface = {NULL};
static bool bKdrvReady;
static bool bSetHdrOn = 1;
static bool bSetDVOn = 1;
static bool bFirstFrameCalculated;
static bool bNeedFireMenuload;
static bool bDVReady;
static bool bDVEnabled;

#ifdef CONFIG_DRM_MEDIATEK_HDMI
static bool bHdmiPlugged;
static unsigned int _u32HDMITx_SinkSupHDRFormat;

static unsigned char _u8HDMITx_SinkVSVDB[32] = {0};
static unsigned char _u8HDMITx_SinkHDRSMDB[32] = {0};
#endif

static unsigned int _u32DisplayWidth;
static unsigned int _u32DisplayHeight;
static unsigned int _u32DisplayFramerate;
static unsigned char *_DVRawMetadata;
static enum EN_ETHDR_HDR_TYPE _eOutputHdrType = EN_ETHDR_HDR_TYPE_SDR;

static u16 *pu16MLoadVa;
static dma_addr_t dma_MloadHandle;
static u16 *pu16AdlVa;
static dma_addr_t dma_AdlHandle;

static struct mtk_ethdr_metadata _stEthdrMetadata = {0};
static struct mtk_plane_state _stPlaneState[4] = {0};

static struct mtk_ddp_comp *_comp;
static struct ST_KDRV_HDMITX_INFOFRAME stPreHdmiInfoFrame;
static struct ST_DV_VSIF stPreDvVsif;

static const char * const ethdr_comp_str[] = {
	"ETHDR_DISP_MIXER",
	"ETHDR_VDO_FE0",
	"ETHDR_VDO_FE1",
	"ETHDR_GFX_FE0",
	"ETHDR_GFX_FE1",
	"ETHDR_VDO_BE",
	"ETHDR_ADL_DS",
	"ETHDR_ID_MAX"
};

static const char * const ethdr_async_clk_str[] = {
	"hdr_vdo_fe0_async",
	"hdr_vdo_fe1_async",
	"hdr_gfx_fe0_async",
	"hdr_gfx_fe1_async",
	"hdr_vdo_be_async",
	"hdr_async_MAX"
};

static inline struct mtk_ethdr *comp_to_ethdr(struct mtk_ddp_comp *comp)
{
	return container_of(comp, struct mtk_ethdr, ddp_comp);
}

bool mtk_ethdr_isDvMode(void)
{
	unsigned char bDvEnabled = false;

	if (_stEthdrInterface.fpGetDvEnable == NULL)
		return false;
	bDvEnabled = _stEthdrInterface.fpGetDvEnable();
	return (bDVReady && bDvEnabled);
}

void mtk_ethdr_get_output_hdr_type(enum EN_ETHDR_HDR_TYPE *peHdrType)
{
	*peHdrType = _eOutputHdrType;
}

#ifdef CONFIG_DRM_MEDIATEK_HDMI
static bool _mtk_ethdr_GetSinkSupportHDRFormatDV
(struct mtk_hdmi_edid *hdmiEdid, unsigned int *u32SinkSupHDRFormat)
{
	/// DEID documents: CEA-861-E.pdf, CEA-861.3_V16BallotDraft.pdf
	unsigned char u8ExtEdidRawData[EDID_EXT_BLOCK_SIZE];
	unsigned char i, u8CurOffset = 0, u8DescOffset = 0;
	unsigned int u32SupFormat = 0;
	unsigned char u8VSVDB[32];
	unsigned char u8HDRSMDB[32];

	memset(&u8ExtEdidRawData, 0, sizeof(u8ExtEdidRawData));
	memset(&u8VSVDB, 0, sizeof(u8VSVDB));
	memset(&u8HDRSMDB, 0, sizeof(u8HDRSMDB));

	//Get EDID Block1
	memcpy(u8ExtEdidRawData,
		&hdmiEdid->edid[EDID_EXT_BLOCK_SIZE], sizeof(u8ExtEdidRawData));

	u8DescOffset = u8ExtEdidRawData[u8CurOffset+2];
	u8CurOffset += 4;

	while (u8CurOffset < u8DescOffset) {
		unsigned char u8BlkTagCode = 0, u8BlkLen = 0;

		u8BlkTagCode = (u8ExtEdidRawData[u8CurOffset] & 0xE0) >> 5;
		u8BlkLen = u8ExtEdidRawData[u8CurOffset] & 0x1F;

		if (u8BlkTagCode == 7) {  // EXTTAG == 7
			unsigned char u8ExtTagCode = u8ExtEdidRawData[u8CurOffset + 1];
			// Extended Tag Code at second byte

			if (u8ExtTagCode == 1) { //VSVDB
				if ((u8ExtEdidRawData[u8CurOffset + 2] == 0x46) &&
					(u8ExtEdidRawData[u8CurOffset + 3] == 0xD0) &&
					(u8ExtEdidRawData[u8CurOffset + 4] == 0x00)) {
					unsigned char u8VSVDBVersion =
						(u8ExtEdidRawData[u8CurOffset + 5] >> 5);

					//DV SDKv2.3 EDID Test neg case
					if (u8VSVDBVersion == 0)
						u8BlkLen = 0x19;
					else if (u8VSVDBVersion == 1)
						u8BlkLen = (u8ExtEdidRawData[u8CurOffset + 8] == 0)
						? 0x0E : 0x0B;
					else if (u8VSVDBVersion == 2)
						u8BlkLen = 0x0B;

					u32SupFormat |= EN_ETHDR_DV_FORMAT_DV;

					//store  Vendor-Specific Video Data Block
					for (i = 0; i < (u8BlkLen+1); i++)//+1 is for tag code
						u8VSVDB[i] = u8ExtEdidRawData[u8CurOffset + i];
				}
			} else if (u8ExtTagCode == 6) { //CEA_HDR_STATIC_METADATA_DATA_BLOCK
				u32SupFormat |= EN_ETHDR_DV_FORMAT_HDR10;

				//store  HDR Static Metadata Data Block
				for (i = 0; i < (u8BlkLen+1); i++)//+1 is for tag code
					u8HDRSMDB[i] = u8ExtEdidRawData[u8CurOffset + i];
			}
		}
		u8CurOffset += (u8BlkLen + 1); // shift Ext Tag Block
	}

	*u32SinkSupHDRFormat = u32SupFormat;
	memcpy(&_u8HDMITx_SinkVSVDB, &u8VSVDB, sizeof(_u8HDMITx_SinkVSVDB));
	memcpy(&_u8HDMITx_SinkHDRSMDB, &u8HDRSMDB, sizeof(_u8HDMITx_SinkHDRSMDB));

	return true;
}

static bool _mtk_ethdr_SetSinkEdidDV(struct mtk_hdmi_edid *hdmiEdid)
{
	struct ST_KDRV_XC_DOVI_SOURCE_SET_SINK_EDID stDoViSetSinkEDID;
	struct ST_KDRV_XC_CFD_CONTROL_INFO stCfdCtrlInfo;
	struct ST_KDRV_XC_DOVI_SOURCE_SET_OUTPUT_FORMAT stDoViOutputFormat;

	memset(&stDoViSetSinkEDID, 0, sizeof(struct ST_KDRV_XC_DOVI_SOURCE_SET_SINK_EDID));
	stDoViSetSinkEDID.u32Version = KDRV_XC_DOVI_SOURCE_SET_SINK_EDID_VERSION;
	stDoViSetSinkEDID.u32Length = sizeof(struct ST_KDRV_XC_DOVI_SOURCE_SET_SINK_EDID);

	//get Sink supported HDR type
	_mtk_ethdr_GetSinkSupportHDRFormatDV(hdmiEdid, &_u32HDMITx_SinkSupHDRFormat);

	//Set Sink VSVDB
	memcpy(&stDoViSetSinkEDID.u8VSVDB, &_u8HDMITx_SinkVSVDB, sizeof(_u8HDMITx_SinkVSVDB));

	//Set Sink HDRSMDB
	memcpy(&stDoViSetSinkEDID.u8HDRSMDB, &_u8HDMITx_SinkHDRSMDB, sizeof(_u8HDMITx_SinkHDRSMDB));

	memset(&stCfdCtrlInfo, 0, sizeof(struct ST_KDRV_XC_CFD_CONTROL_INFO));
	stCfdCtrlInfo.enCtrlType = E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_SET_SINK_EDID;
	stCfdCtrlInfo.pParam = (void *)&stDoViSetSinkEDID;
	stCfdCtrlInfo.u32ParamLen = sizeof(struct ST_KDRV_XC_DOVI_SOURCE_SET_SINK_EDID);
	if (_stEthdrInterface.fpCfdCtrl != NULL)
		_stEthdrInterface.fpCfdCtrl(&stCfdCtrlInfo);

	//Decide & set output HDR type : follow VS10 rule.
	memset(&stDoViOutputFormat, 0, sizeof(struct ST_KDRV_XC_DOVI_SOURCE_SET_OUTPUT_FORMAT));
	stDoViOutputFormat.u32Version = KDRV_XC_DOVI_SOURCE_SET_OUTPUT_FORMAT_VERSION;
	stDoViOutputFormat.u32Length = sizeof(struct ST_KDRV_XC_DOVI_SOURCE_SET_OUTPUT_FORMAT);
	if (_u32HDMITx_SinkSupHDRFormat & EN_ETHDR_DV_FORMAT_DV) {
		enum EN_KDRV_HDMITX_DV_VSVDB_VERSION eDvVsvdbVer = E_KDRV_HDMITX_DV_VSVDB_INVALID;
		struct ST_KDRV_HDMITX_INFOFRAME stHdmiInfoFrame;
		struct ST_DV_VSIF stDvVsif;

		if (_stEthdrInterface.fpGetHdmiSetting != NULL)
			if (!_stEthdrInterface.fpGetHdmiSetting(&stHdmiInfoFrame,
				&stDvVsif, &eDvVsvdbVer)) {
				DDPINFO("[%s]Get HDMI Setting failed!\n", __func__);
				return false;
			}
		if (((eDvVsvdbVer == E_KDRV_HDMITX_DV_VSVDB_V0_4K60_0_YUV422_0_26BYTE)
			|| (eDvVsvdbVer == E_KDRV_HDMITX_DV_VSVDB_V0_4K60_0_YUV422_1_26BYTE)
			|| (eDvVsvdbVer == E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_0_15BYTE)
			|| (eDvVsvdbVer == E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_1_15BYTE)
			|| (eDvVsvdbVer == E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_0_LL_0_12BYTE)
			|| (eDvVsvdbVer == E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_1_LL_1_12BYTE))
			&& (_u32DisplayWidth > 1920)
			&& (_u32DisplayHeight > 1080)
			&& (_u32DisplayFramerate > 30)) {
			stDoViOutputFormat.u8DoViOutputFormat =
				E_KDRV_XC_DOVI_SOURCE_SET_FORMAT_HDR10;
			_eOutputHdrType = EN_ETHDR_HDR_TYPE_HDR10;
		} else {
			stDoViOutputFormat.u8DoViOutputFormat =
				E_KDRV_XC_DOVI_SOURCE_SET_FORMAT_DV;
			_eOutputHdrType = EN_ETHDR_HDR_TYPE_DV;
		}
	} else if (_u32HDMITx_SinkSupHDRFormat & EN_ETHDR_DV_FORMAT_HDR10) {
		stDoViOutputFormat.u8DoViOutputFormat = E_KDRV_XC_DOVI_SOURCE_SET_FORMAT_HDR10;
		_eOutputHdrType = EN_ETHDR_HDR_TYPE_HDR10;
	} else {
		stDoViOutputFormat.u8DoViOutputFormat = E_KDRV_XC_DOVI_SOURCE_SET_FORMAT_SDR;
		_eOutputHdrType = EN_ETHDR_HDR_TYPE_SDR;
	}

	stDoViOutputFormat.u8DoViLowLatencyMode = E_KDRV_XC_DOVI_SOURCE_SET_LOW_LATENCY_MODE;

	memset(&stCfdCtrlInfo, 0, sizeof(struct ST_KDRV_XC_CFD_CONTROL_INFO));
	stCfdCtrlInfo.enCtrlType = E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_SET_OUTPUT_FORMAT;
	stCfdCtrlInfo.pParam = (void *)&stDoViOutputFormat;
	stCfdCtrlInfo.u32ParamLen = sizeof(struct ST_KDRV_XC_DOVI_SOURCE_SET_OUTPUT_FORMAT);
	if (_stEthdrInterface.fpCfdCtrl != NULL)
		_stEthdrInterface.fpCfdCtrl(&stCfdCtrlInfo);

	return true;
}

static bool _mtk_ethdr_SetSinkEdidCfd(struct mtk_hdmi_edid *hdmiEdid)
{
	/// EDID documents: CEA-861-E.pdf, CEA-861.3_V16BallotDraft.pdf
	unsigned char u8EdidRawData[EDID_BLOCK_SIZE+EDID_EXT_BLOCK_SIZE];
	struct ST_KDRV_XC_CFD_EDID stCfdEdid;
	struct ST_KDRV_XC_CFD_CONTROL_INFO stCfdCtrlInfo;
	unsigned short u16Offset = 0;
	unsigned char u8ByteOffset;

	memcpy(u8EdidRawData, hdmiEdid->edid, sizeof(u8EdidRawData));
	memset(&stCfdEdid, 0, sizeof(struct ST_KDRV_XC_CFD_EDID));
	stCfdEdid.u32Version = CFD_EDID_VERSION;
	stCfdEdid.u16Length = sizeof(struct ST_KDRV_XC_CFD_EDID);

	/// 0x00~0x7F bytes
	#define EDID_HEADER_VERSION_OFFSET 18
	#define EDID_HEADER_RED_GREEN_OFFSET 25

	stCfdEdid.u8HDMISinkEDIDBaseBlockVersion = u8EdidRawData[EDID_HEADER_VERSION_OFFSET];
	stCfdEdid.u8HDMISinkEDIDBaseBlockReversion = u8EdidRawData[EDID_HEADER_VERSION_OFFSET+1];
	stCfdEdid.u16Display_Primaries_x[0] =
		COLOR_CHARACTERISTICS_RX(u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+2],
		u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET]);
	stCfdEdid.u16Display_Primaries_x[1] =
		COLOR_CHARACTERISTICS_GX(u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+4],
		u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET]);
	stCfdEdid.u16Display_Primaries_x[2] =
		COLOR_CHARACTERISTICS_BX(u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+6],
		u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+1]);
	stCfdEdid.u16Display_Primaries_y[0] =
		COLOR_CHARACTERISTICS_RY(u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+3],
		u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET]);
	stCfdEdid.u16Display_Primaries_y[1] =
		COLOR_CHARACTERISTICS_GY(u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+5],
		u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET]);
	stCfdEdid.u16Display_Primaries_y[2] =
		COLOR_CHARACTERISTICS_BY(u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+7],
		u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+1]);
	stCfdEdid.u16White_point_x =
		COLOR_CHARACTERISTICS_WX(u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+8],
		u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+1]);
	stCfdEdid.u16White_point_y =
		COLOR_CHARACTERISTICS_WY(u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+9],
		u8EdidRawData[EDID_HEADER_RED_GREEN_OFFSET+1]);
	stCfdEdid.u8HDMISinkEDIDValid = 1;

	/// 0x80~0x83
	// Parse extend
	u16Offset += 128;

	stCfdEdid.u8HDMISinkEDIDCEABlockReversion = u8EdidRawData[u16Offset+1];
	if (u8EdidRawData[u16Offset+3] & EDID_CEA_HEADER_YUV444_MASK)
		/// YUV444
		stCfdEdid.u8HDMISinkSupportYUVFormat |= EDID_YUV444_SUPPORT_MASK;

	if (u8EdidRawData[u16Offset+3] & EDID_CEA_HEADER_YUV422_MASK)
		/// YUV422
		stCfdEdid.u8HDMISinkSupportYUVFormat |= EDID_YUV422_SUPPORT_MASK;

	u8ByteOffset = u8EdidRawData[u16Offset+2];
	////////////m_bSendHDRinfoFrame = FALSE;
	u16Offset += 4;

	while ((u16Offset - 128) < u8ByteOffset) {
		unsigned char u8TagCode = EDID_CEA_TAG_CODE((*(u8EdidRawData+u16Offset)));
		unsigned char u8Length = EDID_CEA_TAG_LENGTH((*(u8EdidRawData+u16Offset)));

		u16Offset += 1;
		if (u8TagCode == 7) { //CEA_EXTENDED_TAG_DATA_BLOCK
			unsigned short u16ExtOffset = 0;
			unsigned char u8ExtTagCode = *(u8EdidRawData+u16Offset+u16ExtOffset);

			u16ExtOffset++;
			if (u8ExtTagCode == 0) {//CEA_VIDEO_CAPABILITY_DATA_BLOCK
				// Video capability data block
				unsigned char u8VidCap = *(u8EdidRawData+u16Offset+u16ExtOffset);

				u16ExtOffset++;
				stCfdEdid.u8HDMISinkVCDBValid = 1;

				if (u8VidCap&VIDEO_CAPABILITY_DB_QY_MASK)
					// QY
					stCfdEdid.u8HDMISinkSupportYUVFormat |=
						EDID_QY_SUPPORT_MASK;

				if (u8VidCap&VIDEO_CAPABILITY_DB_QS_MASK)
					// QS
					stCfdEdid.u8HDMISinkSupportYUVFormat |=
						EDID_QS_SUPPORT_MASK;

			} else if (u8ExtTagCode == 1) { //VSVDB
				unsigned char i = 0, u8CurOffset = 0;

				u8CurOffset = u16Offset - 1;
				//store  Vendor-Specific Video Data Block
				for (i = 0; i < (u8Length+1); i++)//+1 is for tag code
					stCfdEdid.u8VSVDB[i] = u8EdidRawData[u8CurOffset + i];
			} else if (u8ExtTagCode == 5) {//CEA_COLORIMETRY_DATA_BLOCK
				unsigned char u8ColorimetryData =
					*(u8EdidRawData+u16Offset+u16ExtOffset);

				u16ExtOffset++;
				stCfdEdid.u8HDMISinkExtendedColorspace = u8ColorimetryData;
			} else if (u8ExtTagCode == 6) {//CEA_HDR_STATIC_METADATA_DATA_BLOCK
				unsigned char u8Eotf;
				unsigned char u8StaticMetadata;
				unsigned char u8ContentMaxLuminance = 0;
				unsigned char u8ContentMaxFrameAvgLuminance = 0;
				unsigned char u8ContentMinLuminance = 0;

				u8Eotf = *(u8EdidRawData+u16Offset+u16ExtOffset);
				u16ExtOffset++;

				u8StaticMetadata = *(u8EdidRawData+u16Offset+u16ExtOffset);
				u16ExtOffset++;

				if (u16ExtOffset < u8Length) {
					u8ContentMaxLuminance =
						*(u8EdidRawData+u16Offset+u16ExtOffset);
					u16ExtOffset++;
				}

				if (u16ExtOffset < u8Length) {
					u8ContentMaxFrameAvgLuminance =
						*(u8EdidRawData+u16Offset+u16ExtOffset);
					u16ExtOffset++;
				}

				if (u16ExtOffset < u8Length) {
					u8ContentMinLuminance =
						*(u8EdidRawData+u16Offset+u16ExtOffset);
					u16ExtOffset++;
				}

				stCfdEdid.u8HDMISinkHDRDataBlockValid = 1;
				stCfdEdid.u8HDMISinkEOTF = HDR_DB_EOTF(u8Eotf);
				stCfdEdid.u8HDMISinkSM = u8StaticMetadata;
				stCfdEdid.u8HDMISinkHDRDataBlockLength = u8Length;
				stCfdEdid.u8HDMISinkDesiredContentMaxLuminance =
					u8ContentMaxLuminance;
				stCfdEdid.u8HDMISinkDesiredContentMaxFrameAvgLuminance =
					u8ContentMaxFrameAvgLuminance;
				stCfdEdid.u8HDMISinkDesiredContentMinLuminance =
					u8ContentMinLuminance;
			} else if (u8ExtTagCode == 14) //CEA_YCBCR420_VIDEO_DATA_BLOCK
				/// YCbCr 4:2:0  Video data block
				/// YUV420
				stCfdEdid.u8HDMISinkSupportYUVFormat |= EDID_YUV420_SUPPORT_MASK;
			else if (u8ExtTagCode == 15)//CEA_YCBCR420_CAPABILITY_MAP_DATA_BLOCK)
				/// YCbCr 4:2:0  Capability Map Data Block
				/// YUV420
				stCfdEdid.u8HDMISinkSupportYUVFormat |= EDID_YUV420_SUPPORT_MASK;
		}
		u16Offset += u8Length;

	}

	if ((stCfdEdid.u8HDMISinkEOTF & 0x4) == 0x4)
		_u32HDMITx_SinkSupHDRFormat |= EN_ETHDR_DV_FORMAT_HDR10;
	if ((stCfdEdid.u8HDMISinkEOTF & 0x8) == 0x8)
		_u32HDMITx_SinkSupHDRFormat |= EN_ETHDR_DV_FORMAT_HLG;
	if (!bSetHdrOn) {
		stCfdEdid.u8HDMISinkHDRDataBlockValid = 0;
		_u32HDMITx_SinkSupHDRFormat = EN_ETHDR_DV_FORMAT_SDR;
	}
	memset(&stCfdCtrlInfo, 0, sizeof(struct ST_KDRV_XC_CFD_CONTROL_INFO));
	stCfdCtrlInfo.enCtrlType = E_KDRV_XC_CFD_CTRL_SET_EDID;
	stCfdCtrlInfo.pParam = (void *)&stCfdEdid;
	stCfdCtrlInfo.u32ParamLen = sizeof(struct ST_KDRV_XC_CFD_EDID);
	if (_stEthdrInterface.fpCfdCtrl != NULL)
		_stEthdrInterface.fpCfdCtrl(&stCfdCtrlInfo);

	return true;
}

static bool _mtk_ethdr_fill_hdr10p_vsif(unsigned char *vsif)
{
	unsigned char vsif_timing_mode = 0x1;
	unsigned char graphics_overlay_flag = 0x0;
	unsigned int tmp = 0;

	if (vsif == NULL)
		return false;

	memset(vsif, 0, 27);
	vsif[0] = 0x8B;
	vsif[1] = 0x84;
	vsif[2] = 0x90;
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.targetedSystemDisplayMaximumLuminance;
	vsif[3] =
	((_stEthdrMetadata.hdr10PlusDynamicMetadata.applicationVersion << 6) |
	(MIN(ROUND(MIN(tmp, 1024), 32), 31) << 1));
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.averageMaxrgb;
	vsif[4] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[0];
	vsif[5] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[1];
	vsif[6] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[2];
	if (tmp > 100)
		vsif[7] = 255;
	else
		vsif[7] = tmp;
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[3];
	vsif[8] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[4];
	vsif[9] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[5];
	vsif[10] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[6];
	vsif[11] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[7];
	vsif[12] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.distributionMaxrgbPercentiles[8];
	vsif[13] = MIN(ROUND(MIN((tmp/10), 4096), 16), 255);
	vsif[14] =
	((_stEthdrMetadata.hdr10PlusDynamicMetadata.numBezierCurveAnchors << 4) |
	((MIN(ROUND(_stEthdrMetadata.hdr10PlusDynamicMetadata.kneePointX, 4), 1023)
	& 0x03C0) >> 6));
	vsif[15] =
	(((MIN(ROUND(_stEthdrMetadata.hdr10PlusDynamicMetadata.kneePointX, 4), 1023)
	& 0x003F) << 2) |
	((MIN(ROUND(_stEthdrMetadata.hdr10PlusDynamicMetadata.kneePointY, 4), 1023)
	& 0x0300) >> 8));
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.kneePointY;
	vsif[16] = MIN(ROUND(tmp, 4), 1023) & 0xFF;
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[0];
	vsif[17] = MIN(ROUND(tmp, 4), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[1];
	vsif[18] = MIN(ROUND(tmp, 4), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[2];
	vsif[19] = MIN(ROUND(tmp, 4), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[3];
	vsif[20] = MIN(ROUND(tmp, 4), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[4];
	vsif[21] = MIN(ROUND(tmp, 4), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[5];
	vsif[22] = MIN(ROUND(tmp, 4), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[6];
	vsif[23] = MIN(ROUND(tmp, 4), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[7];
	vsif[24] = MIN(ROUND(tmp, 4), 255);
	tmp = _stEthdrMetadata.hdr10PlusDynamicMetadata.bezierCurveAnchors[8];
	vsif[25] = MIN(ROUND(tmp, 4), 255);
	vsif[26] = ((graphics_overlay_flag << 7) | (vsif_timing_mode << 6));

	return true;
}

void mtk_ethdr_set_hdmi_info(void)
{
	enum EN_KDRV_HDMITX_DV_VSVDB_VERSION eDvVsvdbVer = E_KDRV_HDMITX_DV_VSVDB_INVALID;
	unsigned char bDvEnabled = false;
	struct ST_KDRV_HDMITX_INFOFRAME stHdmiInfoFrame = {0};
	struct ST_DV_VSIF stDvVsif = {0};
	struct VID_PLA_HDR_METADATA_INFO_T stMetadata = {0};
	struct device_node *hdmiNode = NULL;
	struct drm_bridge *hdmiBridge = NULL;

	mutex_lock(&g_set_hdmi_info_lock);

	if ((!bHdmiPlugged) || (!bKdrvReady)) {
		mutex_unlock(&g_set_hdmi_info_lock);
		return;
	}

	hdmiNode = of_find_compatible_node(NULL, NULL, "mediatek,mt8195-hdmi");
	if (hdmiNode == NULL) {
		hdmiNode = of_find_compatible_node(NULL, NULL, "mediatek,mt8188-hdmi");
	}
	if (hdmiNode == NULL) {
		DDPINFO("[%s]hdmiNode == NULL!\n", __func__);
		mutex_unlock(&g_set_hdmi_info_lock);
		return;
	}
	hdmiBridge = of_drm_find_bridge(hdmiNode);
	if (hdmiBridge == NULL) {
		DDPINFO("[%s]hdmiBridge == NULL!\n", __func__);
		mutex_unlock(&g_set_hdmi_info_lock);
		return;
	}

	if (!bSetHdrOn) {
		mtk_dpi_set_input_format(false, MTK_DPI_INT_MATRIX_COLOR_FORMAT_BT709);
		/* if not bypass DPI colorspace conversion,
		 * keep DPI input and output colorimetry identical,
		 * DPI only do YUV2RGB conversion, ex. YUV BT709 -> RGB BT709,
		 * not YUV BT709 -> RGB BT601
		 */
		set_hdmi_colorimetry_range(hdmiBridge,
			HDMI_COLORIMETRY_ITU_709, HDMI_EXTENDED_COLORIMETRY_RESERVED,
			HDMI_QUANTIZATION_RANGE_LIMITED, HDMI_YCC_QUANTIZATION_RANGE_LIMITED);

		vSetStaticHdrType(GAMMA_709);
		stMetadata.e_DynamicRangeType = VID_PLA_DR_TYPE_SDR;
		vBT2020Enable(false);
		vHdr10Enable(false);
		_eOutputHdrType = EN_ETHDR_HDR_TYPE_SDR;
		mutex_unlock(&g_set_hdmi_info_lock);
		return;
	}

	if (_stEthdrInterface.fpGetHdmiSetting != NULL)
		if (!_stEthdrInterface.fpGetHdmiSetting(&stHdmiInfoFrame,
			&stDvVsif, &eDvVsvdbVer)) {
			DDPINFO("[%s]Get HDMI Setting failed!\n", __func__);
			mutex_unlock(&g_set_hdmi_info_lock);
			return;
		}
	if (_stEthdrInterface.fpGetDvEnable != NULL)
		bDvEnabled = _stEthdrInterface.fpGetDvEnable();

	if (_stEthdrMetadata.hdrType == MTK_ETHDR_HDR_TYPE_HDR10PLUS) {
		unsigned char vsif[27];

		stMetadata.e_DynamicRangeType = VID_PLA_DR_TYPE_HDR10_PLUS_VSIF;
		stMetadata.fgIsMetadata = true;
		if (!_mtk_ethdr_fill_hdr10p_vsif(vsif)) {
			DDPINFO("[%s]_mtk_ethdr_fill_hdr10p_vsif failed\n", __func__);
			mutex_unlock(&g_set_hdmi_info_lock);
			return;
		}
		stMetadata.metadata_info.hdr10_plus_metadata.hdr10p_metadata_info.ui4_Hdr10PlusAddr
			= (unsigned long)vsif;
		stMetadata.metadata_info.hdr10_plus_metadata.hdr10p_metadata_info.ui4_Hdr10PlusSize
			= 27;
		vVdpSetHdrMetadata(true, stMetadata);
		vHdr10PlusEnable(true);
	}

	if ((memcmp(&stPreHdmiInfoFrame, &stHdmiInfoFrame,
		sizeof(struct ST_KDRV_HDMITX_INFOFRAME)) == 0)
		&& (memcmp(&stPreDvVsif, &stDvVsif,
		sizeof(struct ST_DV_VSIF)) == 0)) {
		mutex_unlock(&g_set_hdmi_info_lock);
		return;
	}


	memcpy(&stPreHdmiInfoFrame, &stHdmiInfoFrame,
		sizeof(struct ST_KDRV_HDMITX_INFOFRAME));
	memcpy(&stPreDvVsif, &stDvVsif,
		sizeof(struct ST_DV_VSIF));

	if ((_u32HDMITx_SinkSupHDRFormat & EN_ETHDR_DV_FORMAT_DV)
		&& bDvEnabled && bDVReady && _eOutputHdrType == EN_ETHDR_HDR_TYPE_DV) {
		mtk_dpi_set_input_format(true, 0);
		vHdr10Enable(false);

		switch (eDvVsvdbVer) {
		case E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_1_LL_1_12BYTE:
		case E_KDRV_HDMITX_DV_VSVDB_V1_4K60_1_YUV422_0_LL_1_12BYTE:
		case E_KDRV_HDMITX_DV_VSVDB_V1_4K60_1_YUV422_1_LL_1_12BYTE:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_2_12Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_2_13Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_2_16Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_3_12Byte_DM3:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_3_12Byte_DM4:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_1_int_0_12Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_1_int_2_12Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_1_YUV422_1_int_1_12Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_1_YUV422_1_int_3_12Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_2_YUV422_0_int_3_12Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_2_YUV422_1_int_1_12Byte:
		case E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_2_YUV422_1_int_3_12Byte:
		{
			stMetadata.e_DynamicRangeType = VID_PLA_DR_TYPE_DOVI_LOWLATENCY;
			stMetadata.metadata_info.dovi_lowlatency_metadata.fgBackltCtrlMdPresent
				= stDvVsif.bBackltCtrl;
			stMetadata.metadata_info.dovi_lowlatency_metadata.ui4_EffTmaxPQ
				= stDvVsif.u16Max_PQ;

			set_hdmi_colorimetry_range(hdmiBridge,
				HDMI_COLORIMETRY_EXTENDED,
				HDMI_EXTENDED_COLORIMETRY_BT2020,
				HDMI_QUANTIZATION_RANGE_LIMITED,
				HDMI_YCC_QUANTIZATION_RANGE_LIMITED);

			vVdpSetHdrMetadata(true, stMetadata);
			vLowLatencyDVEnable(true);
		}
		break;
		case E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_0_LL_0_12BYTE:
		case E_KDRV_HDMITX_DV_VSVDB_V1_4K60_1_YUV422_0_LL_0_12BYTE:
		{
			set_hdmi_colorimetry_range(hdmiBridge,
				HDMI_COLORIMETRY_EXTENDED,
				HDMI_EXTENDED_COLORIMETRY_BT2020,
				HDMI_QUANTIZATION_RANGE_FULL,
				HDMI_YCC_QUANTIZATION_RANGE_FULL);

			vBT2020Enable(true);
			vDVHdrEnable(true, true);
		}
		break;
		default: //standard 14b vsif
		{
			set_hdmi_colorimetry_range(hdmiBridge,
				HDMI_COLORIMETRY_EXTENDED,
				HDMI_EXTENDED_COLORIMETRY_BT2020,
				HDMI_QUANTIZATION_RANGE_FULL,
				HDMI_YCC_QUANTIZATION_RANGE_FULL);

			vBT2020Enable(true);
			vDVHdrEnable(true, false);
		}
		break;
		}
	} else {
		vLowLatencyDVEnable(false);
		vDVHdrEnable(false, false);

		if (_u32HDMITx_SinkSupHDRFormat & EN_ETHDR_DV_FORMAT_HDR10) {
			mtk_dpi_set_input_format(false,
				MTK_DPI_INT_MATRIX_COLOR_FORMAT_2020LIMITED);
			/* if not bypass DPI colorspace conversion,
			 * keep DPI input and output colorimetry identical,
			 * DPI only do YUV2RGB conversion, ex. YUV BT709 -> RGB BT709,
			 * not YUV BT709 -> RGB BT601
			 */
			set_hdmi_colorimetry_range(hdmiBridge,
				HDMI_COLORIMETRY_EXTENDED, HDMI_EXTENDED_COLORIMETRY_BT2020,
				HDMI_QUANTIZATION_RANGE_LIMITED,
				HDMI_YCC_QUANTIZATION_RANGE_LIMITED);
			vSetStaticHdrType(GAMMA_ST2084);
			stMetadata.e_DynamicRangeType = VID_PLA_DR_TYPE_HDR10;
			stMetadata.fgIsMetadata = true;
			stMetadata.metadata_info.hdr10_metadata.fgNeedUpdStaticMeta = true;
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesX[0] =
				stHdmiInfoFrame.u16DisplayPrimariesX[0];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesX[1] =
				stHdmiInfoFrame.u16DisplayPrimariesX[1];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesX[2] =
				stHdmiInfoFrame.u16DisplayPrimariesX[2];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesY[0] =
				stHdmiInfoFrame.u16DisplayPrimariesY[0];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesY[1] =
				stHdmiInfoFrame.u16DisplayPrimariesY[1];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesY[2] =
				stHdmiInfoFrame.u16DisplayPrimariesY[2];
			stMetadata.metadata_info.hdr10_metadata.ui2_WhitePointX =
				stHdmiInfoFrame.u16WhitePointX;
			stMetadata.metadata_info.hdr10_metadata.ui2_WhitePointY =
				stHdmiInfoFrame.u16WhitePointY;
			stMetadata.metadata_info.hdr10_metadata.ui2_MaxDisplayMasteringLuminance =
				stHdmiInfoFrame.u16MasterPanelMaxLuminance;
			stMetadata.metadata_info.hdr10_metadata.ui2_MinDisplayMasteringLuminance =
				stHdmiInfoFrame.u16MasterPanelMinLuminance;
			stMetadata.metadata_info.hdr10_metadata.ui2_MaxCLL =
				stHdmiInfoFrame.u16MaxContentLightLevel;
			stMetadata.metadata_info.hdr10_metadata.ui2_MaxFALL =
				stHdmiInfoFrame.u16MaxFrameAvgLightLevel;
			vVdpSetHdrMetadata(true, stMetadata);
			vBT2020Enable(true);
			vHdr10Enable(true);
			_eOutputHdrType = EN_ETHDR_HDR_TYPE_HDR10;
		} else if (_u32HDMITx_SinkSupHDRFormat & EN_ETHDR_DV_FORMAT_HLG) {
			mtk_dpi_set_input_format(false,
				MTK_DPI_INT_MATRIX_COLOR_FORMAT_2020LIMITED);
			/* if not bypass DPI colorspace conversion,
			 * keep DPI input and output colorimetry identical,
			 * DPI only do YUV2RGB conversion, ex. YUV BT709 -> RGB BT709,
			 * not YUV BT709 -> RGB BT601
			 */
			set_hdmi_colorimetry_range(hdmiBridge,
				HDMI_COLORIMETRY_EXTENDED, HDMI_EXTENDED_COLORIMETRY_BT2020,
				HDMI_QUANTIZATION_RANGE_LIMITED,
				HDMI_YCC_QUANTIZATION_RANGE_LIMITED);
			vSetStaticHdrType(GAMMA_HLG);
			stMetadata.e_DynamicRangeType = VID_PLA_DR_TYPE_HDR10;
			stMetadata.fgIsMetadata = true;
			stMetadata.metadata_info.hdr10_metadata.fgNeedUpdStaticMeta = true;
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesX[0] =
				stHdmiInfoFrame.u16DisplayPrimariesX[0];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesX[1] =
				stHdmiInfoFrame.u16DisplayPrimariesX[1];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesX[2] =
				stHdmiInfoFrame.u16DisplayPrimariesX[2];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesY[0] =
				stHdmiInfoFrame.u16DisplayPrimariesY[0];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesY[1] =
				stHdmiInfoFrame.u16DisplayPrimariesY[1];
			stMetadata.metadata_info.hdr10_metadata.ui2_DisplayPrimariesY[2] =
				stHdmiInfoFrame.u16DisplayPrimariesY[2];
			stMetadata.metadata_info.hdr10_metadata.ui2_WhitePointX =
				stHdmiInfoFrame.u16WhitePointX;
			stMetadata.metadata_info.hdr10_metadata.ui2_WhitePointY =
				stHdmiInfoFrame.u16WhitePointY;
			stMetadata.metadata_info.hdr10_metadata.ui2_MaxDisplayMasteringLuminance =
				stHdmiInfoFrame.u16MasterPanelMaxLuminance;
			stMetadata.metadata_info.hdr10_metadata.ui2_MinDisplayMasteringLuminance =
				stHdmiInfoFrame.u16MasterPanelMinLuminance;
			stMetadata.metadata_info.hdr10_metadata.ui2_MaxCLL =
				stHdmiInfoFrame.u16MaxContentLightLevel;
			stMetadata.metadata_info.hdr10_metadata.ui2_MaxFALL =
				stHdmiInfoFrame.u16MaxFrameAvgLightLevel;
			vVdpSetHdrMetadata(true, stMetadata);
			vBT2020Enable(true);
			vHdr10Enable(true);
			_eOutputHdrType = EN_ETHDR_HDR_TYPE_HLG;
		} else {
			mtk_dpi_set_input_format(false, MTK_DPI_INT_MATRIX_COLOR_FORMAT_BT709);
			/* if not bypass DPI colorspace conversion,
			 * keep DPI input and output colorimetry identical,
			 * DPI only do YUV2RGB conversion, ex. YUV BT709 -> RGB BT709,
			 * not YUV BT709 -> RGB BT601
			 */
			set_hdmi_colorimetry_range(hdmiBridge,
				HDMI_COLORIMETRY_ITU_709, HDMI_EXTENDED_COLORIMETRY_RESERVED,
				HDMI_QUANTIZATION_RANGE_LIMITED,
				HDMI_YCC_QUANTIZATION_RANGE_LIMITED);
			vSetStaticHdrType(GAMMA_709);
			stMetadata.e_DynamicRangeType = VID_PLA_DR_TYPE_SDR;
			vBT2020Enable(false);
			vHdr10Enable(false);
			_eOutputHdrType = EN_ETHDR_HDR_TYPE_SDR;
		}
	}
	mutex_unlock(&g_set_hdmi_info_lock);
}

void mtk_ethdr_notify_hdmi_hpd(bool hpd)
{
	static bool hdmiHpd;
	struct device_node *hdmiNode = NULL;
	struct drm_bridge *hdmiBridge = NULL;
	struct mtk_hdmi_edid *hdmiEdid = NULL;

	if (hpd) {
		DDPINFO("[%s]hdmi tx connected!\n", __func__);
		hdmiHpd = hpd;
		hdmiNode = of_find_compatible_node(NULL, NULL, "mediatek,mt8195-hdmi");
		if (hdmiNode == NULL) {
			hdmiNode = of_find_compatible_node(NULL, NULL, "mediatek,mt8188-hdmi");
		}
		if (hdmiNode == NULL) {
			DDPINFO("[%s]hdmiNode == NULL!\n", __func__);
			return;
		}
		hdmiBridge = of_drm_find_bridge(hdmiNode);
		if (hdmiBridge == NULL) {
			DDPINFO("[%s]hdmiBridge == NULL!\n", __func__);
			return;
		}
		hdmiEdid = mtk_hdmi_get_raw_edid(hdmiBridge);
		if (hdmiEdid == NULL) {
			DDPINFO("[%s]hdmiEdid == NULL!\n", __func__);
			return;
		}
		if (!_mtk_ethdr_SetSinkEdidCfd(hdmiEdid)) {
			DDPINFO("[%s]_mtk_ethdr_SetSinkEdidCfd failed!\n", __func__);
			return;
		}
		if (bDVReady && bSetDVOn)
			if (!_mtk_ethdr_SetSinkEdidDV(hdmiEdid)) {
				DDPINFO("[%s]_mtk_ethdr_SetSinkEdidDV failed!\n", __func__);
				return;
			}
		mtk_ethdr_cal_frame();
		mtk_ethdr_fire();
		bHdmiPlugged = true;
		mtk_ethdr_set_hdmi_info();
	} else {
		DDPINFO("[%s]hdmi tx disconnected!\n", __func__);
		bHdmiPlugged = false;
	}
}
#endif

void mtk_ethdr_cal_frame(void)
{
	struct ST_ETHDR_METADATA stEthdrMetadata;

	memset(&stEthdrMetadata, 0, sizeof(struct ST_ETHDR_METADATA));
	stEthdrMetadata.planeId = _stEthdrMetadata.planeId;
	stEthdrMetadata.ehdrType = (enum EN_ETHDR_HDR_TYPE)_stEthdrMetadata.hdrType;
	if (stEthdrMetadata.ehdrType == EN_ETHDR_HDR_TYPE_HDR10PLUS)
		stEthdrMetadata.ehdrType = EN_ETHDR_HDR_TYPE_HDR10;
	memcpy(&stEthdrMetadata.hdr10StaticMetadata, &_stEthdrMetadata.hdr10StaticMetadata,
		sizeof(struct ST_ETHDR_HDR10_STATIC_METADATA));
	memcpy(&stEthdrMetadata.hdr10PlusDynamicMetadata,
		&_stEthdrMetadata.hdr10PlusDynamicMetadata,
		sizeof(struct ST_ETHDR_HDR10PLUS_DYNAMIC_METADATA));
	stEthdrMetadata.isSecure = _stEthdrMetadata.isSecure;
	stEthdrMetadata.pDVMetadataBuffer = _DVRawMetadata;
	stEthdrMetadata.DVMetadataBufferFd = _stEthdrMetadata.DVMetadataBufferFd;
	stEthdrMetadata.DVMetadataBufferLength = _stEthdrMetadata.DVMetadataBufferLength;
	stEthdrMetadata.DVMetadataBufferOffset = _stEthdrMetadata.DVMetadataBufferOffset;
	stEthdrMetadata.colorPrimaries = _stEthdrMetadata.colorPrimaries;
	stEthdrMetadata.transferCharacteristics = _stEthdrMetadata.transferCharacteristics;
	stEthdrMetadata.matrixCoefficients = _stEthdrMetadata.matrixCoefficients;
	stEthdrMetadata.colorRange = (enum EN_ETHDR_COLOR_RANGE)_stEthdrMetadata.colorRange;
	stEthdrMetadata.videoWidth = _stPlaneState[0].pending.width;
	stEthdrMetadata.videoHeight = _stPlaneState[0].pending.height;

	if (bFirstFrameCalculated == false) {
		stEthdrMetadata.bFirstFrame = true;
		bFirstFrameCalculated = true;
	}

	if (_stEthdrInterface.fpCalFrameInfo != NULL)
		_stEthdrInterface.fpCalFrameInfo(&stEthdrMetadata);
}

void mtk_ethdr_fire(void)
{
	if (_stEthdrInterface.fpMLoadFire != NULL)
		_stEthdrInterface.fpMLoadFire(E_CLIENT_ETHDR, true);
	bNeedFireMenuload = true;
}

void mtk_ethdr_init(void)
{
	struct ST_KDRV_XC_AUTODOWNLOAD_CONFIG_INFO stAutodownloadConfig;

#ifdef CONFIG_DRM_MEDIATEK_HDMI
	struct device_node *hdmiNode = NULL;
	struct drm_bridge *hdmiBridge = NULL;
	struct mtk_hdmi_edid *hdmiEdid = NULL;
#endif

	if (pu16MLoadVa == NULL)
		pu16MLoadVa = dma_alloc_coherent(&g_dev, 0x8000, &dma_MloadHandle,
			GFP_KERNEL);
	if (pu16AdlVa == NULL)
		pu16AdlVa = dma_alloc_coherent(&g_dev, 0x12000, &dma_AdlHandle,
			GFP_KERNEL);

	stAutodownloadConfig.bEnable = true;
	stAutodownloadConfig.enClient = E_KDRV_XC_AUTODOWNLOAD_CLIENT_HDR;
	stAutodownloadConfig.enMode = E_KDRV_XC_AUTODOWNLOAD_ENABLE_MODE;
	stAutodownloadConfig.u32MiuNo = 0;
	stAutodownloadConfig.u32Size = 0x12000;
	stAutodownloadConfig.phyBaseAddr = (unsigned long long)dma_AdlHandle;
	stAutodownloadConfig.virtBaseAddr = (unsigned long long)pu16AdlVa;

	_stEthdrInterface.fpConfigAutoDownload(&stAutodownloadConfig);
	_stEthdrInterface.fpMLoadInit(E_CLIENT_ETHDR, (unsigned long long)dma_MloadHandle,
		(unsigned long long)pu16MLoadVa, 0x8000);
	_stEthdrInterface.fpMLoadEnable(E_CLIENT_ETHDR, true);

	_stEthdrMetadata.planeId = 0;
	_stEthdrMetadata.colorPrimaries = 0;
	_stEthdrMetadata.transferCharacteristics = 0;
	_stEthdrMetadata.matrixCoefficients = 0;
	_stEthdrMetadata.colorRange = MTK_ETHDR_COLOR_RANGE_LIMIT;
	_stEthdrMetadata.hdrType = MTK_ETHDR_HDR_TYPE_SDR;

#ifdef CONFIG_DRM_MEDIATEK_HDMI
	hdmiNode = of_find_compatible_node(NULL, NULL, "mediatek,mt8195-hdmi");
	if (hdmiNode == NULL) {
		hdmiNode = of_find_compatible_node(NULL, NULL, "mediatek,mt8188-hdmi");
	}
	if (hdmiNode == NULL) {
		DDPINFO("[%s]hdmiNode is NULL\n", __func__);
		return;
	}
	hdmiBridge = of_drm_find_bridge(hdmiNode);
	if (hdmiBridge == NULL) {
		DDPINFO("[%s]hdmiBridge is NULL\n", __func__);
		return;
	}
	hdmiEdid = mtk_hdmi_get_raw_edid(hdmiBridge);
	if (hdmiEdid == NULL) {
		DDPINFO("[%s]hdmiEdid is NULL\n", __func__);
		return;
	}
#endif
}

int mtk_ethdr_inform_dvko_insert(void)
{
	struct ST_KDRV_XC_CFD_CONTROL_INFO stCfdCtrlInfo;
	struct ST_KDRV_XC_DOVI_SOURCE_INIT stDoviInitInfo;
	struct ST_KDRV_XC_DOVI_SOURCE_GET_HDR_INFORMATION stDoviGetHdrInfomation;

	//Init DV
	memset(&stDoviInitInfo, 0, sizeof(struct ST_KDRV_XC_DOVI_SOURCE_INIT));
	stDoviInitInfo.bDoviInit = true;
	stDoviInitInfo.u32Length = sizeof(struct ST_KDRV_XC_DOVI_SOURCE_INIT);
	stDoviInitInfo.u32Version = KDRV_XC_DOVI_SOURCE_INIT_VERSION;
	memset(&stCfdCtrlInfo, 0, sizeof(struct ST_KDRV_XC_CFD_CONTROL_INFO));
	stCfdCtrlInfo.enCtrlType = E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_INIT;
	stCfdCtrlInfo.pParam = (void *)&stDoviInitInfo;
	stCfdCtrlInfo.u32ParamLen = sizeof(struct ST_KDRV_XC_DOVI_SOURCE_INIT);
	_stEthdrInterface.fpCfdCtrl(&stCfdCtrlInfo);

	//Check DV Init status
	memset(&stDoviGetHdrInfomation, 0,
		sizeof(struct ST_KDRV_XC_DOVI_SOURCE_GET_HDR_INFORMATION));
	stDoviGetHdrInfomation.u32Length =
		sizeof(struct ST_KDRV_XC_DOVI_SOURCE_GET_HDR_INFORMATION);
	stDoviGetHdrInfomation.u32Version = KDRV_XC_DOVI_SOURCE_GET_HDR_INFORMATION_VERSION;
	memset(&stCfdCtrlInfo, 0, sizeof(struct ST_KDRV_XC_CFD_CONTROL_INFO));
	stCfdCtrlInfo.enCtrlType = E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_GET_HDR_INFORMATION;
	stCfdCtrlInfo.pParam = (void *)&stDoviGetHdrInfomation;
	stCfdCtrlInfo.u32ParamLen = sizeof(struct ST_KDRV_XC_DOVI_SOURCE_GET_HDR_INFORMATION);
	_stEthdrInterface.fpCfdCtrl(&stCfdCtrlInfo);

	_DVRawMetadata = kmalloc(MAX_DV_RAW_MD_LENGTH, GFP_ATOMIC);
	bDVReady = stDoviGetHdrInfomation.bDoViIsInited;
	bDVEnabled = true;
	_stEthdrInterface.fpSetDvEnable(true);

	return 0;
}
EXPORT_SYMBOL(mtk_ethdr_inform_dvko_insert);

int mtk_drm_ioctl_set_hdr_on_off(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	enum MTK_ETHDR_ON_OFF_STATUS  *eOnOffStatus = data;

	switch (*eOnOffStatus) {
	case MTK_ETHDR_HDROFF_DVOFF:
		bSetHdrOn = 0;
		bSetDVOn = 0;
		memset(&_stEthdrMetadata, 0, sizeof(struct mtk_ethdr_metadata));
		_stEthdrMetadata.hdrType = MTK_ETHDR_HDR_TYPE_SDR;
		if (_stEthdrInterface.fpSetDvEnable != NULL)
			_stEthdrInterface.fpSetDvEnable(false);

		_eOutputHdrType = EN_ETHDR_HDR_TYPE_SDR;
		break;
	case MTK_ETHDR_HDRON_DVOFF:
		bSetHdrOn = 1;
		bSetDVOn = 0;
		if (_stEthdrInterface.fpSetDvEnable != NULL)
			_stEthdrInterface.fpSetDvEnable(false);
		break;
	case MTK_ETHDR_HDRON_DVON:
		bSetHdrOn = 1;
		bSetDVOn = 1;
		if (_stEthdrInterface.fpSetDvEnable != NULL)
			_stEthdrInterface.fpSetDvEnable(true);
		break;
	default:
		bSetHdrOn = 1;
		bSetDVOn = 1;
		break;
	}

	return 0;
}

int mtk_drm_ioctl_set_hdr_metadata(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct mtk_ethdr_metadata *pstEthdrMetadata = data;

	if (pstEthdrMetadata == NULL) {
		DDPINFO("[%s]pstEthdrMetadata = NULL\n", __func__);
		return -EFAULT; }

	if (_stEthdrMetadata.planeId != 0)
		return 0;

	if (!bSetHdrOn)
		return 0;
#ifdef CONFIG_DRM_MEDIATEK_HDMI
	if (!bHdmiPlugged)
		return 0;
#endif
	_stEthdrMetadata.planeId = pstEthdrMetadata->planeId;
	_stEthdrMetadata.hdrType = pstEthdrMetadata->hdrType;
	memcpy(&_stEthdrMetadata.hdr10StaticMetadata,
		&pstEthdrMetadata->hdr10StaticMetadata,
		sizeof(struct mtk_ethdr_hdr10_static_metadata));
	memcpy(&_stEthdrMetadata.hdr10PlusDynamicMetadata,
		&pstEthdrMetadata->hdr10PlusDynamicMetadata,
		sizeof(struct mtk_ethdr_hdr10plus_dynamic_metadata));

	_stEthdrMetadata.isSecure = pstEthdrMetadata->isSecure;
	_stEthdrMetadata.DVMetadataBufferFd = pstEthdrMetadata->DVMetadataBufferFd;
	_stEthdrMetadata.DVMetadataBufferLength = pstEthdrMetadata->DVMetadataBufferLength;
	_stEthdrMetadata.DVMetadataBufferOffset = pstEthdrMetadata->DVMetadataBufferOffset;
	if ((pstEthdrMetadata->hdrType == MTK_ETHDR_HDR_TYPE_DV)
		&& (!_stEthdrMetadata.isSecure) && (bDVReady)) {
		struct dma_buf *dma_buf;
		void *vaddr;

		if (pstEthdrMetadata->DVMetadataBufferLength > MAX_DV_RAW_MD_LENGTH) {
			DDPINFO("[%s]DVMetadataBufferLength error! Length = %d\n", __func__,
				pstEthdrMetadata->DVMetadataBufferLength);
			return 0;
		}

		dma_buf = dma_buf_get(pstEthdrMetadata->DVMetadataBufferFd);
		if (IS_ERR_OR_NULL(dma_buf)) {
			DDPINFO("%s: dma_buf_get fail fd=%d ret=0x%p\n",
				__func__, pstEthdrMetadata->DVMetadataBufferFd, dma_buf);
			return PTR_ERR(dma_buf);
		}

		vaddr = dma_buf_vmap(dma_buf);
		vaddr += pstEthdrMetadata->DVMetadataBufferOffset;
		memcpy(_DVRawMetadata, vaddr, pstEthdrMetadata->DVMetadataBufferLength);
		vaddr -= pstEthdrMetadata->DVMetadataBufferOffset;
		dma_buf_vunmap(dma_buf, vaddr);
		dma_buf_put(dma_buf);
	}
	_stEthdrMetadata.colorPrimaries = pstEthdrMetadata->colorPrimaries;
	_stEthdrMetadata.transferCharacteristics = pstEthdrMetadata->transferCharacteristics;
	_stEthdrMetadata.matrixCoefficients = pstEthdrMetadata->matrixCoefficients;
	_stEthdrMetadata.colorRange = pstEthdrMetadata->colorRange;
	_stEthdrMetadata.hdr10StaticMetadata.maxDisplayMasteringLuminance *= 10000;
	if (bDVReady && bSetDVOn) {
		if (_stEthdrMetadata.hdrType == MTK_ETHDR_HDR_TYPE_HDR10PLUS) {
			if (bDVEnabled != false) {
				_stEthdrInterface.fpSetDvEnable(false);
				DDPINFO("[%s]switch to MTK CFD driver\n", __func__);
			}
			bDVEnabled = false;
		} else if (bDVEnabled != true) {
			_stEthdrInterface.fpSetDvEnable(true);
			DDPINFO("[%s]switch to DV driver\n", __func__);
			bDVEnabled = true;
#ifdef CONFIG_DRM_MEDIATEK_HDMI
			mtk_ethdr_notify_hdmi_hpd(true);
#endif
		}
	}
	mtk_ethdr_cal_frame();
	mtk_ethdr_fire();

	return 0;
}

int mtk_ethdr_register_interface(struct ST_ETHDR_INTERFACE *pstEthdrInterface)
{
	if (pstEthdrInterface == NULL) {
		DDPDBG("[%s]pstEthdrInterface == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpMLoadInit == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpMLoadInit == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpConfigAutoDownload == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpConfigAutoDownload == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpMLoadEnable == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpMLoadEnable == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpMLoadWriteCmd == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpMLoadWriteCmd == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpMLoadFire == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpMLoadFire == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpCfdCtrl == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpCfdCtrl == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpCalFrameInfo == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpCalFrameInfo == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpGetHdmiSetting == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpGetHdmiSetting == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpSetDvEnable == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpGetHdmiSetting == NULL\n", __func__);
		goto EXIT;
	}
	if (pstEthdrInterface->fpGetDvEnable == NULL) {
		DDPDBG("[%s]pstEthdrInterface->fpGetHdmiSetting == NULL\n", __func__);
		goto EXIT;
	}

	_stEthdrInterface.fpMLoadInit = pstEthdrInterface->fpMLoadInit;
	_stEthdrInterface.fpConfigAutoDownload = pstEthdrInterface->fpConfigAutoDownload;
	_stEthdrInterface.fpMLoadEnable = pstEthdrInterface->fpMLoadEnable;
	_stEthdrInterface.fpMLoadWriteCmd = pstEthdrInterface->fpMLoadWriteCmd;
	_stEthdrInterface.fpMLoadFire = pstEthdrInterface->fpMLoadFire;
	_stEthdrInterface.fpCfdCtrl = pstEthdrInterface->fpCfdCtrl;
	_stEthdrInterface.fpCalFrameInfo = pstEthdrInterface->fpCalFrameInfo;
	_stEthdrInterface.fpGetHdmiSetting = pstEthdrInterface->fpGetHdmiSetting;
	_stEthdrInterface.fpSetDvEnable = pstEthdrInterface->fpSetDvEnable;
	_stEthdrInterface.fpGetDvEnable = pstEthdrInterface->fpGetDvEnable;

	mtk_ethdr_init();
	bKdrvReady = true;
	return true;

EXIT:
	DDPDBG("[%s]ethdr register interface failed.\n", __func__);

	return false;
}
EXPORT_SYMBOL(mtk_ethdr_register_interface);

void mtk_ethdr_layer_on(unsigned int idx, struct cmdq_pkt *handle)
{
	struct mtk_ethdr *ethdr = comp_to_ethdr(_comp);

	DDPINFO("%s+ idx:%d", __func__, idx);

	if (idx < 4) {
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x24, 1<<idx, 1<<idx);
	}
	DDPINFO("%s-", __func__);
}

void mtk_ethdr_layer_off(unsigned int idx, struct cmdq_pkt *handle)
{
	struct mtk_ethdr *ethdr = comp_to_ethdr(_comp);
	struct mtk_drm_crtc *mtk_crtc = _comp->mtk_crtc;
	resource_size_t regs_pa = mtk_crtc_get_regs_pa(mtk_crtc, _comp->id);

	DDPINFO("%s+ idx:%d", __func__, idx);

	switch (idx) {
	case 0:
	{
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x30, 0, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			regs_pa + VDO1_CONFIG_MIXER_IN1_PAD,
			0, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + VDO_FE_HSIZE,
			0, 0x3FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + VDO_FE_VSIZE,
			0, 0x1FFF);
	}
	break;
	case 1:
	{
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x48, 0, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			regs_pa + VDO1_CONFIG_MIXER_IN2_PAD,
			0, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + VDO_FE_HSIZE,
			0, 0x3FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + VDO_FE_VSIZE,
			0, 0x1FFF);
	}
	break;
	case 2:
	{
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x60, 0, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			regs_pa + VDO1_CONFIG_MIXER_IN3_PAD,
			0, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + GFX_DV_FE_VSIZE,
			0, 0x1FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + GFX_TOSD_FE_VSIZE,
			0, 0x1FFF);
	}
	break;
	case 3:
	{
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x78, 0, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			regs_pa + VDO1_CONFIG_MIXER_IN4_PAD,
			0, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + GFX_DV_FE_VSIZE,
			0, 0x1FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + GFX_TOSD_FE_VSIZE,
			0, 0x1FFF);
	}
	break;
	default:
	{
		DDPINFO("%s Wrong layer ID\n", __func__);
	}
	break;
	}

	DDPINFO("%s-", __func__);
}

void mtk_ethdr_layer_config(unsigned int idx,
				 struct mtk_plane_state *state,
				 struct cmdq_pkt *handle)
{
	unsigned int layer_id = 0;
	unsigned int alpha = 0;
	unsigned int alpha_con = 0;
	unsigned int fmt = 0;
	struct mtk_ethdr *ethdr = comp_to_ethdr(_comp);
	struct mtk_drm_crtc *mtk_crtc = _comp->mtk_crtc;
	bool x_offset_odd = false;
	unsigned int mixer_inx_pad_mode = MIXER_INx_MODE_BYPASS;
	unsigned int mixer_inx_biwidth = state->pending.width/2 - 1;
	unsigned int reg_val;
	resource_size_t regs_pa = mtk_crtc_get_regs_pa(mtk_crtc, _comp->id);

	if (state->comp_state.comp_id)
		layer_id = state->comp_state.lye_id;
	else
		layer_id = idx;

	memcpy(&_stPlaneState[layer_id], state, sizeof(struct mtk_plane_state));

	fmt = state->pending.format;
	alpha_con = state->pending.prop_val[PLANE_PROP_ALPHA_CON];
	alpha = 0xFF & state->pending.prop_val[PLANE_PROP_PLANE_ALPHA];
	if (alpha == 0xFF &&
		(fmt == DRM_FORMAT_RGBX8888 || fmt == DRM_FORMAT_BGRX8888 ||
		fmt == DRM_FORMAT_XRGB8888 || fmt == DRM_FORMAT_XBGR8888))
		alpha_con = 0;

	if (state->pending.dst_x % 2) {
		x_offset_odd = true;

		if (state->pending.width % 2)
			mixer_inx_pad_mode = MIXER_INx_MODE_BYPASS;
		else
			mixer_inx_pad_mode = MIXER_INx_MODE_EVEN_EXTEND;
	} else if (state->pending.width % 2)
		mixer_inx_pad_mode = MIXER_INx_MODE_ODD_EXTEND;

	switch (layer_id) {
	case 0:
	{
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + VDO_FE_HSIZE,
			state->pending.width, 0x3FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + VDO_FE_VSIZE,
			state->pending.height, 0x1FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x30,
			(state->pending.height)<<16|state->pending.width, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x34,
			x_offset_odd<<31|state->pending.dst_y<<16|
			state->pending.dst_x, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x28,
			alpha_con<<8|alpha, 0x1FF);
		reg_val = REG_FLD_VAL(MIXER_IN1_MODE, mixer_inx_pad_mode)|
			REG_FLD_VAL(MIXER_IN1_BIWIDTH, mixer_inx_biwidth);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			regs_pa + VDO1_CONFIG_MIXER_IN1_PAD,
			reg_val, ~0);
	}
	break;
	case 1:
	{
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + VDO_FE_HSIZE,
			state->pending.width, 0x3FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + VDO_FE_VSIZE,
			state->pending.height, 0x1FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x48,
			(state->pending.height)<<16|state->pending.width, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x4C,
			x_offset_odd<<31|state->pending.dst_y<<16|
			state->pending.dst_x, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x40,
			alpha_con<<8|alpha, 0x1FF);
		reg_val = REG_FLD_VAL(MIXER_IN2_MODE, mixer_inx_pad_mode) |
			REG_FLD_VAL(MIXER_IN2_BIWIDTH, mixer_inx_biwidth);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			regs_pa + VDO1_CONFIG_MIXER_IN2_PAD,
			reg_val, ~0);
	}
	break;
	case 2:
	{
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + GFX_DV_FE_VSIZE,
			state->pending.height, 0x1FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + GFX_TOSD_FE_VSIZE,
			state->pending.height, 0x1FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x60,
			(state->pending.height)<<16|state->pending.width, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x64,
			x_offset_odd<<31|state->pending.dst_y<<16|
			state->pending.dst_x, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x58,
			alpha_con<<8|alpha, 0x1FF);
		reg_val = REG_FLD_VAL(MIXER_IN3_MODE, mixer_inx_pad_mode) |
			REG_FLD_VAL(MIXER_IN3_BIWIDTH, mixer_inx_biwidth);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			regs_pa + VDO1_CONFIG_MIXER_IN3_PAD,
			reg_val, ~0);
	}
	break;
	case 3:
	{
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + GFX_DV_FE_VSIZE,
			state->pending.height, 0x1FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + GFX_TOSD_FE_VSIZE,
			state->pending.height, 0x1FFF);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x78,
			(state->pending.height)<<16|state->pending.width, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x7C,
			x_offset_odd<<31|state->pending.dst_y<<16|
			state->pending.dst_x, ~0);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x70,
			alpha_con<<8|alpha, 0x1FF);
		reg_val = REG_FLD_VAL(MIXER_IN4_MODE, mixer_inx_pad_mode) |
			REG_FLD_VAL(MIXER_IN4_BIWIDTH, mixer_inx_biwidth);
		cmdq_pkt_write(handle, _comp->cmdq_base,
			regs_pa + VDO1_CONFIG_MIXER_IN4_PAD,
			reg_val, ~0);
	}
	break;
	default:
	{
		DDPINFO("%s Wrong layer ID\n", __func__);
	}
	break;
	}
	if (bNeedFireMenuload) {
		cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_ADL_DS].regs_pa + 0x74,
			0x9000, 0xF000);
		bNeedFireMenuload = false;
	}
	mtk_merge_layer_config(state, handle);

#ifdef CONFIG_DRM_MEDIATEK_HDMI
	mtk_ethdr_set_hdmi_info();
#endif
}


static void mtk_ethdr_config(struct mtk_ddp_comp *comp,
			      struct mtk_ddp_config *cfg,
			      struct cmdq_pkt *handle)
{
	struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
	struct mtk_ethdr *ethdr = comp_to_ethdr(comp);
	resource_size_t regs_pa = mtk_crtc_get_regs_pa(mtk_crtc, comp->id);

	_u32DisplayWidth = cfg->w;
	_u32DisplayHeight = cfg->h;
	_u32DisplayFramerate = cfg->vrefresh;
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_HDR_BE_ASYNC_CFG_WD,
		(cfg->h << 16 | (cfg->w/2)),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + 0xD64,
		0x20202020,
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		ethdr->ethdr_comp[ETHDR_VDO_BE].regs_pa + VDO_BE_VSIZE,
		cfg->h, 0x1FFF);

	/*mixer layer setting*/
	cmdq_pkt_write(handle, comp->cmdq_base,
		ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x18,
		(cfg->h)<<16|cfg->w, ~0);
}

static void mtk_ethdr_start(struct mtk_ddp_comp *comp,
	struct cmdq_pkt *handle)
{
	struct mtk_ethdr *ethdr = comp_to_ethdr(comp);
	struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
	resource_size_t regs_pa = mtk_crtc_get_regs_pa(mtk_crtc, comp->id);

	DDPINFO("%s\n", __func__);

	//Mixer
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x120,
			0xffffffff, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x124,
			0xffffffff, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x1C,
			0x00000888, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x24,
			0x0FA50004, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x60,
			0x00000000, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x28,
			0x000021ff, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x40,
			0x000021ff, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x58,
			0x000021ff, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x70,
			0x000021ff, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x20,
			0xFF800080, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + MIX_INTEN,
			MIX_FME_UND_INTSTA|MIX_ABNORM_SOF_INTSTA|MIX_START_INTSTA, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0xC,
			0x00000001, ~0);

	cmdq_pkt_write(handle, comp->cmdq_base,
			regs_pa + 0xD30,
			0x01000100, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			regs_pa + 0xD34,
			0x01000100, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			regs_pa + 0xD38,
			0x01000100, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			regs_pa + 0xD3C,
			0x01000100, ~0);

	//VDO_FE0
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + 0x804,
			0xfd, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + 0x9EC,
			0x80, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + 0x81C,
			0x12E, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + 0x618,
			0x0, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + 0x61C,
			0x2, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + 0x6D0,
			0x8001, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + 0x634,
			0x8000, ~0);
	cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + VDO_FE_HSIZE,
			0, 0x3FFF);
	cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE0].regs_pa + VDO_FE_VSIZE,
			0, 0x1FFF);
	//VDO_FE1
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + 0x804,
			0xfd, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + 0x9EC,
			0x80, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + 0x81C,
			0x12E, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + 0x618,
			0x0, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + 0x61C,
			0x2, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + 0x6D0,
			0x8001, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + 0x634,
			0x8000, ~0);
	cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + VDO_FE_HSIZE,
			0, 0x3FFF);
	cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_FE1].regs_pa + VDO_FE_VSIZE,
			0, 0x1FFF);
	//GFX_FE0
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x100,
			0x8001, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x12c,
			0xE030, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x134,
			0x1C0, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x138,
			0x1E69, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x13C,
			0x1fd7, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x140,
			0xba, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x144,
			0x275, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x148,
			0x3f, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x14c,
			0x1f99, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x150,
			0x1ea6, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x154,
			0x1c2, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x204,
			0xfd, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x3ec,
			0x80, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + 0x21c,
			0x20, ~0);
	cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + GFX_DV_FE_VSIZE,
			0, 0x1FFF);
	cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE0].regs_pa + GFX_TOSD_FE_VSIZE,
			0, 0x1FFF);
	//GFX_FE1
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x100,
			0x8001, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x12c,
			0xE030, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x134,
			0x1C0, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x138,
			0x1E69, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x13C,
			0x1fd7, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x140,
			0xba, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x144,
			0x275, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x148,
			0x3f, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x14c,
			0x1f99, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x150,
			0x1ea6, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x154,
			0x1c2, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x204,
			0xfd, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x3ec,
			0x80, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + 0x21c,
			0x20, ~0);
	cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + GFX_DV_FE_VSIZE,
			0, 0x1FFF);
	cmdq_pkt_write(handle, _comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_GFX_FE1].regs_pa + GFX_TOSD_FE_VSIZE,
			0, 0x1FFF);
	//BE
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_BE].regs_pa + 0x204,
			0x7e, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_BE].regs_pa + 0x320,
			0x00, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
			ethdr->ethdr_comp[ETHDR_VDO_BE].regs_pa + 0x3C8,
			0x01, ~0);
	if (bKdrvReady)
		mtk_ethdr_init();
	mtk_ethdr_cal_frame();
	mtk_ethdr_fire();
}

static void mtk_ethdr_stop(struct mtk_ddp_comp *comp,
				struct cmdq_pkt *handle)
{
	struct mtk_ethdr *ethdr = comp_to_ethdr(_comp);
	struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
	resource_size_t regs_pa = mtk_crtc_get_regs_pa(mtk_crtc, comp->id);

	DDPINFO("%s\n", __func__);

	cmdq_pkt_write(handle, comp->cmdq_base,
		ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0xC,
		0x0, ~0);
	/* reset mixer*/
	cmdq_pkt_write(handle, comp->cmdq_base,
		ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x14,
		0x1, ~0);
	/* reset async*/
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_SW1_RST_B,
		0x0, HDR_ASYNC_RESET_BIT);

	cmdq_pkt_write(handle, comp->cmdq_base,
		ethdr->ethdr_comp[ETHDR_DISP_MIXER].regs_pa + 0x14,
		0x0, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_SW1_RST_B,
		HDR_ASYNC_RESET_BIT, HDR_ASYNC_RESET_BIT);

	memset(&stPreHdmiInfoFrame, 0, sizeof(struct ST_KDRV_HDMITX_INFOFRAME));
	memset(&stPreDvVsif, 0, sizeof(struct ST_DV_VSIF));
}

static void mtk_ethdr_prepare(struct mtk_ddp_comp *comp)
{
	int i, ret;
	struct mtk_ethdr *priv = dev_get_drvdata(comp->dev);

	DDPINFO("%s\n", __func__);

	ret = pm_runtime_get_sync(comp->dev);
	if (ret < 0)
		DDPINFO("Failed to enable power domain: %d\n", ret);

	for (i = 0; i < ETHDR_ASYNC_ID_MAX; i++) {
		if (priv->async_clk[i] != NULL) {
			ret = clk_prepare_enable(priv->async_clk[i]);
			if (ret)
				DDPDBG("async_clk[%d] prepare enable failed:%s\n", i,
					mtk_dump_comp_str(comp));
		}
	}

	for (i = 0; i < ETHDR_ID_MAX; i++) {
		if (priv->ethdr_comp[i].clk != NULL) {
			ret = clk_prepare_enable(priv->ethdr_comp[i].clk);
			if (ret)
				DDPDBG("ethdr_comp[%d].clk prepare enable failed:%s\n", i,
					mtk_dump_comp_str(comp));
		}
	}
}

static void mtk_ethdr_unprepare(struct mtk_ddp_comp *comp)
{
	struct mtk_ethdr *priv = dev_get_drvdata(comp->dev);
	int i;
	int ret;

	DDPINFO("%s\n", __func__);
	for (i = 0; i < ETHDR_ASYNC_ID_MAX; i++)
		clk_disable_unprepare(priv->async_clk[i]);

	for (i = 0; i < ETHDR_ID_MAX; i++)
		clk_disable_unprepare(priv->ethdr_comp[i].clk);

	ret = pm_runtime_put_sync(comp->dev);
	if (ret < 0)
		DDPINFO("Failed to disable power domain: %d\n", ret);
}

static const struct mtk_ddp_comp_funcs mtk_ethdr_funcs = {
	.config = mtk_ethdr_config,
	.start = mtk_ethdr_start,
	.stop = mtk_ethdr_stop,
	.prepare = mtk_ethdr_prepare,
	.unprepare = mtk_ethdr_unprepare,
};

static irqreturn_t mtk_ethdr_mixer_irq_handler(int irq, void *dev_id)
{
	struct mtk_ethdr *priv = dev_id;
	struct mtk_drm_crtc *mtk_crtc = _comp->mtk_crtc;
	struct mtk_ddp_comp *mixer = &priv->ethdr_comp[ETHDR_DISP_MIXER];
	unsigned int val = 0;
	unsigned int ret = 0;

	val = readl(mixer->regs + MIX_INTSTA);

	DRM_MMP_MARK(IRQ, irq, val);

	DDPIRQ("mixer irq, val:0x%x\n", val);

	writel(~val, mixer->regs + MIX_INTSTA);

	if (val & MIX_START_INTSTA) {
		if (mtk_crtc) {
			atomic_set(&mtk_crtc->pf_event, 1);
			wake_up_interruptible(&mtk_crtc->present_fence_wq);
		}
		DDPIRQ("[IRQ] mixer start!\n");
	}
	if (val & MIX_ABNORM_SOF_INTSTA)
		DDPIRQ("[IRQ] mixer: abnormal sof!\n");
	if (val & MIX_FME_UND_INTSTA)
		DDPIRQ("[IRQ] mixer frame underflow!\n");

	ret = IRQ_HANDLED;

	return ret;
}


static int mtk_ethdr_bind(struct device *dev, struct device *master,
				void *data)
{
	struct mtk_ethdr *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	int ret;

	DDPINFO("%s\n", __func__);

	ret = mtk_ddp_comp_register(drm_dev, &priv->ddp_comp);
	if (ret < 0) {
		dev_notice(dev, "Failed to register component %s: %d\n",
			dev->of_node->full_name, ret);
		return ret;
	}
	return 0;
}

static void mtk_ethdr_unbind(struct device *dev, struct device *master,
	void *data)
{
	struct mtk_ethdr *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;

	mtk_ddp_comp_unregister(drm_dev, &(priv->ddp_comp));
}

static const struct component_ops mtk_ethdr_component_ops = {
	.bind	= mtk_ethdr_bind,
	.unbind = mtk_ethdr_unbind,
};

static void mtk_ethdr_process_dbg_cmd(char *opt)
{
	unsigned int ret = 0;
	unsigned int en = 0;

	if (strncmp(opt, "sethdrmode:", 11) == 0) {
		ret = sscanf(opt+11, "%d", &en);
		if (ret < 1)
			return;

		pr_info("ETHDR : Set HDR mode : %d\n", en);
		if ((en == MTK_ETHDR_HDRON_DVON) && (!bDVReady)) {
			pr_info("DV is not supported, set HDR on DV off instead.\n", en);
			en = MTK_ETHDR_HDRON_DVOFF;
		}
		mtk_drm_ioctl_set_hdr_on_off(NULL, &en, NULL);
	} else if (strncmp(opt, "gethdroutputtype", 16) == 0) {
		switch (_eOutputHdrType) {
		case EN_ETHDR_HDR_TYPE_SDR:
			pr_info("ETHDR : Current HDR output type : SDR\n");
			break;
		case EN_ETHDR_HDR_TYPE_HDR10:
			pr_info("ETHDR : Current HDR output type : HDR10\n");
			break;
		case EN_ETHDR_HDR_TYPE_HLG:
			pr_info("ETHDR : Current HDR output type : HLG\n");
			break;
		case EN_ETHDR_HDR_TYPE_DV:
			pr_info("ETHDR : Current HDR output type : DV\n");
			break;
		default:
			pr_info("ETHDR : Current HDR output type unknown\n");
			break;
		}
	}
}
static int mtk_ethdr_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t mtk_ethdr_debug_read(struct file *file,
	char __user *ubuf, size_t count, loff_t *ppos)
{
	int n = 0;

	n = strlen(debug_buffer);
	if ((n >= 0) && (n < sizeof(debug_buffer)))
		debug_buffer[n++] = 0;

	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

static ssize_t mtk_ethdr_debug_write(struct file *file,
	const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(debug_buffer) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&debug_buffer, ubuf, count))
		return -EFAULT;

	debug_buffer[count] = 0;
	buf_offset = 0;

	mtk_ethdr_process_dbg_cmd(debug_buffer);

	return ret;
}

static const struct file_operations mtk_ethdr_debug_fops = {
	.read = mtk_ethdr_debug_read,
	.write = mtk_ethdr_debug_write,
	.open = mtk_ethdr_debug_open,
};

int mtk_ethdr_debug_init(void)
{
	ethdr_debugfs = debugfs_create_file("ethdr",
		S_IFREG | 0444, NULL, (void *)0, &mtk_ethdr_debug_fops);

	if (IS_ERR(ethdr_debugfs))
		return PTR_ERR(ethdr_debugfs);


	DDPINFO("register ethdr debugfs success\n");

	return 0;
}

void mtk_ethdr_debug_deinit(void)
{
	debugfs_remove(ethdr_debugfs);
}

static int mtk_ethdr_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ethdr *priv;
	enum mtk_ddp_comp_id comp_id;
	int ret;
	int i;
	struct resource res;

	DDPINFO("%s+\n", __func__);
	memcpy(&g_dev, dev, sizeof(struct device));

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	comp_id = mtk_ddp_comp_get_id(dev->of_node, MTK_DISP_ETHDR);
	if ((int)comp_id < 0) {
		DDPDBG("Failed to identify by alias: %d\n", comp_id);
		return comp_id;
	}

	ret = mtk_ddp_comp_init(dev, dev->of_node, &priv->ddp_comp, comp_id,
				&mtk_ethdr_funcs);
	if (ret != 0) {
		DDPDBG("Failed to initialize component: %d\n", ret);
		return ret;
	}
	_comp = &priv->ddp_comp;

	//set private comp info
	for (i = 0; i < ETHDR_ID_MAX; i++) {
		priv->ethdr_comp[i].id = DDP_COMPONENT_ETHDR;
		priv->ethdr_comp[i].dev = dev;

		/* get the clk for each pseudo ovl comp in the device node */
		priv->ethdr_comp[i].clk = of_clk_get(dev->of_node, i);
		if (IS_ERR(priv->ethdr_comp[i].clk)) {
			priv->ethdr_comp[i].clk = NULL;
			DDPDBG(
				"comp:%d get priviate comp %s clock %d fail!\n",
				comp_id, ethdr_comp_str[i]);
			return PTR_ERR(priv->ethdr_comp[i].clk);
		}

		if (of_address_to_resource(dev->of_node, i, &res) != 0) {
			dev_notice(dev, "Missing reg in for component %s\n",
			ethdr_comp_str[i]);
			return -EINVAL;
		}
		priv->ethdr_comp[i].regs_pa = res.start;
		priv->ethdr_comp[i].regs = of_iomap(dev->of_node, i);
		priv->ethdr_comp[i].irq = of_irq_get(dev->of_node, i);

		DDPINFO("[DRM]regs_pa:0x%lx, regs:0x%p, node:%s\n",
			(unsigned long)priv->ethdr_comp[i].regs_pa,
			priv->ethdr_comp[i].regs, ethdr_comp_str[i]);
	}

	for (i = 0; i < ETHDR_ASYNC_ID_MAX; i++) {
		priv->async_clk[i] = of_clk_get_by_name(dev->of_node,
			ethdr_async_clk_str[i]);
		if (IS_ERR(priv->async_clk[i])) {
			priv->async_clk[i] = NULL;
			DDPDBG("get async_clk[%d] failed!\n", i);
			return PTR_ERR(priv->async_clk[i]);
		}
	}

	priv->data = of_device_get_match_data(dev);

	ret = devm_request_irq(dev, priv->ethdr_comp[ETHDR_DISP_MIXER].irq,
			mtk_ethdr_mixer_irq_handler,
			IRQF_TRIGGER_NONE | IRQF_SHARED, dev_name(dev), priv);
	if (ret < 0) {
		DDPAEE("%s:%d, failed to request irq:%d ret:%d comp_id:%d\n",
				__func__, __LINE__,
				priv->ethdr_comp[ETHDR_DISP_MIXER].irq, ret, comp_id);
		return ret;
	}

	platform_set_drvdata(pdev, priv);

	pm_runtime_enable(dev);

	ret = component_add(dev, &mtk_ethdr_component_ops);
	if (ret != 0) {
		dev_notice(dev, "Failed to add component: %d\n", ret);
		pm_runtime_disable(dev);
	}

	mtk_ethdr_debug_init();
	DDPINFO("%s-\n", __func__);

	return ret;
}

static int mtk_ethdr_remove(struct platform_device *pdev)
{
	int ret;

	component_del(&pdev->dev, &mtk_ethdr_component_ops);

	ret = pm_runtime_put_sync(&pdev->dev);
	if (ret < 0)
		DDPINFO("Failed to disable power domain: %d\n", ret);

	pm_runtime_disable(&pdev->dev);

	mtk_ethdr_debug_deinit();
	return 0;
}

static const struct mtk_ethdr_data mt8195_ethdr_driver_data = {
	.tbd = false,
};

static const struct of_device_id mtk_ethdr_driver_dt_match[] = {
	{ .compatible = "mediatek,mt8195-disp-ethdr",
	  .data = &mt8195_ethdr_driver_data},
	{ .compatible = "mediatek,mt8188-disp-ethdr",
	  .data = &mt8195_ethdr_driver_data},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_ethdr_driver_dt_match);

struct platform_driver mtk_ethdr_driver = {
	.probe = mtk_ethdr_probe,
	.remove = mtk_ethdr_remove,
	.driver = {
		.name = "mediatek-disp-ethdr",
		.owner = THIS_MODULE,
		.of_match_table = mtk_ethdr_driver_dt_match,
	},
};
