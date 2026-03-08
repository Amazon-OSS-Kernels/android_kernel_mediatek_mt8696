/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_DISP_ETHDR_H__
#define __MTK_DISP_ETHDR_H__

#include <linux/uaccess.h>
#include <drm/mediatek_drm.h>

#define CFD_EDID_VERSION 0
#define KDRV_XC_DOVI_SOURCE_INIT_VERSION (1)
#define KDRV_XC_DOVI_SOURCE_SET_OUTPUT_FORMAT_VERSION (1)
#define KDRV_XC_DOVI_SOURCE_SET_INFOFRAME_CONTROL_VERSION (1)
#define KDRV_XC_DOVI_SOURCE_SET_GRAPHIC_PARAM_VERSION (1)
#define KDRV_XC_DOVI_SOURCE_GET_AVI_INFOFRAME_DATA_VERSION (1)
#define KDRV_XC_DOVI_SOURCE_GET_HDR_INFORMATION_VERSION (1)
#define KDRV_XC_DOVI_SOURCE_SET_SINK_EDID_VERSION (2)
#define KDRV_XC_EDID_EXTENDED_TAG_CODE_DATA_BLOCK_MAX_SIZE (32)

#define EDID_BLOCK_SIZE     (128)
#define EDID_EXT_BLOCK_SIZE (128)
#define MAX_DV_RAW_MD_LENGTH (1024)


/// Following define reference CEA-861-E.pdf & CEA-861.3_V16BallotDraft.pdf.
/// YCbCr(YUV) 444 mask define in CEA extend header.
#define EDID_CEA_HEADER_YUV444_MASK 0x20
/// YCbCr(YUV) 422 mask define in CEA extend header.
#define EDID_CEA_HEADER_YUV422_MASK 0x10
/// CEA tag code mask
#define EDID_CEA_TAG_CODE_MASK 0xE0
/// CEA length mask
#define EDID_CEA_TAG_LENGTH_MASK 0x1F
/// YCC Quantization Range at video capability data block in EDID.
#define VIDEO_CAPABILITY_DB_QY_MASK 0x80
/// RGB Quantization Range at video capability data block in EDID.
#define VIDEO_CAPABILITY_DB_QS_MASK 0x40
/// EOEF at HDR data block in EDID.
#define HDR_DB_EOTF_MASK 0x3F
//EDID Block Size
#define EDID_BLOCK_SIZE     (128)
//EDID Extend Block Size
#define EDID_EXT_BLOCK_SIZE (128)
/// Get CEA tag code
#define EDID_CEA_TAG_CODE(x) (((x)&EDID_CEA_TAG_CODE_MASK)>>5)
/// Get CEA tag length
#define EDID_CEA_TAG_LENGTH(x) ((x)&EDID_CEA_TAG_LENGTH_MASK)
/// Get EOTF in HDR data block
#define HDR_DB_EOTF(x) ((x)&HDR_DB_EOTF_MASK)
/// Get Rx in EDID
#define COLOR_CHARACTERISTICS_RX(x, y) (((x)<<2) + (((y)&0xC0)>>6))
/// Get Ry in EDID
#define COLOR_CHARACTERISTICS_RY(x, y) (((x)<<2) + (((y)&0x30)>>4))
/// Get Gx in EDID
#define COLOR_CHARACTERISTICS_GX(x, y) (((x)<<2) + (((y)&0x0C)>>2))
/// Get Gy in EDID
#define COLOR_CHARACTERISTICS_GY(x, y) (((x)<<2) + ((y)&0x03))
/// Get Bx in EDID
#define COLOR_CHARACTERISTICS_BX(x, y) (((x)<<2) + (((y)&0xC0)>>6))
/// Get By in EDID
#define COLOR_CHARACTERISTICS_BY(x, y) (((x)<<2) + (((y)&0x30)>>4))
/// Get Wx in EDID
#define COLOR_CHARACTERISTICS_WX(x, y) (((x)<<2) + (((y)&0x0C)>>2))
/// Get Wy in EDID
#define COLOR_CHARACTERISTICS_WY(x, y) (((x)<<2) + ((y)&0x03))

#define EDID_YUV444_SUPPORT_MASK 0x01

#define EDID_YUV422_SUPPORT_MASK 0x02

#define EDID_YUV420_SUPPORT_MASK 0x04

#define EDID_QY_SUPPORT_MASK 0x10

#define EDID_QS_SUPPORT_MASK 0x08



enum EN_MLOAD_CLIENT_TYPE {
	E_CLIENT_ETHDR = 0,
	E_CLIENT_MAX,
};

enum EN_KDRV_XC_AUTODOWNLOAD_CLIENT {
	E_KDRV_XC_AUTODOWNLOAD_CLIENT_HDR,
	E_KDRV_XC_AUTODOWNLOAD_CLIENT_SCRAMBLE,
	E_KDRV_XC_AUTODOWNLOAD_CLIENT_OSD_HDR,
	E_KDRV_XC_AUTODOWNLOAD_CLIENT_OSD_THDR,
	E_KDRV_XC_AUTODOWNLOAD_CLIENT_MAX,
};

enum EN_KDRV_XC_AUTODOWNLOAD_MODE {
	E_KDRV_XC_AUTODOWNLOAD_TRIGGER_MODE,
	E_KDRV_XC_AUTODOWNLOAD_ENABLE_MODE,
};

enum EN_KDRV_XC_CFD_CTRL_TYPE {
	// New
	/// Initialize
	E_KDRV_XC_CFD_CTRL_SET_INIT = 0,
	/// VGA
	E_KDRV_XC_CFD_CTRL_SET_VGA = 1,
	/// TV (ATV)
	E_KDRV_XC_CFD_CTRL_SET_TV = 2,
	/// CVBS (AV)
	E_KDRV_XC_CFD_CTRL_SET_CVBS = 3,
	/// S-Video
	E_KDRV_XC_CFD_CTRL_SET_SVIDEO = 4,
	/// YPbPr
	E_KDRV_XC_CFD_CTRL_SET_YPBPR = 5,
	/// Scart
	E_KDRV_XC_CFD_CTRL_SET_SCART = 6,
	/// HDMI
	E_KDRV_XC_CFD_CTRL_SET_HDMI = 7,
	/// DTV
	E_KDRV_XC_CFD_CTRL_SET_DTV = 8,
	/// DVI
	E_KDRV_XC_CFD_CTRL_SET_DVI = 9,
	/// MM
	E_KDRV_XC_CFD_CTRL_SET_MM = 10,
	/// Panel
	E_KDRV_XC_CFD_CTRL_SET_PANEL = 11,
	/// HDR
	E_KDRV_XC_CFD_CTRL_SET_HDR = 12,
	/// EDID
	E_KDRV_XC_CFD_CTRL_SET_EDID = 13,
	/// OSD
	E_KDRV_XC_CFD_CTRL_SET_OSD = 14,
	/// Fire
	E_KDRV_XC_CFD_CTRL_SET_FIRE = 15,
	/// DLC
	E_KDRV_XC_CFD_CTRL_SET_DLC = 16,
	/// Linear RGB
	E_KDRV_XC_CFD_CTRL_SET_LINEAR_RGB = 17,
	/// Get CFD Output to HDMI Status and dicison.
	E_KDRV_XC_CFD_CTRL_GET_HDMI_STATUS = 18,
	/// Set force HDR Onoff or auto select setting
	E_KDRV_XC_CFD_CTRL_SET_HDR_ONOFF_SETTING = 19,
	/// Status
	E_KDRV_XC_CFD_CTRL_GET_STATUS = 20,
	/// TMO
	E_KDRV_XC_CFD_CTRL_SET_TMO = 21,
	/// Update Status
	E_KDRV_XC_CFD_CTRL_UPDATE_STATUS = 22,
	/// Get Picture Attributes
	E_KDRV_XC_CFD_CTRL_PICTURE_ATTRIBUTE = 23,
	/// Update Info into CFD
	E_KDRV_XC_CFD_CTRL_UPDATE_INFO = 24,
	//// 2086 metadata ref
	E_KDRV_XC_CFD_CTRL_TYPE_EX_2086_METADATA = 25,
	/// Set HDMI Seamless
	E_KDRV_XC_CFD_CTRL_SET_HDMI_SEAMLESS = 26,
	/// Get Status of Paring HDMI Packet
	E_KDRV_XC_CFD_CTRL_GET_PARSING_HDMI_PACKET = 27,
	/// Set QMAP Data
	E_KDRV_XC_CFD_CTRL_SET_QMAPDATA = 28,
	/// Get Histogram
	E_KDRV_XC_CFD_CTRL_GET_IP_HISTOGRAM = 29,
	///3DLUT
	E_KDRV_XC_CFD_CTRL_3DLUT = 30,
	///EOTF
	E_KDRV_XC_CFD_CTRL_EOTF = 31,
	///OETF
	E_KDRV_XC_CFD_CTRL_OETF = 32,
	///HLG GAIN
	E_KDRV_XC_CFD_CTRL_HLGGAIN = 33,
	///GAMMA enable/disable
	E_KDRV_XC_CFD_CTRL_GAMMA_ENABLE = 34,
	///Gamut Pre Control
	E_KDRV_XC_CFD_CTRL_GAMUT_PREVIOUS = 35,
	///Gamut Post Control
	E_KDRV_XC_CFD_CTRL_GAMUT_POST = 36,
	///SDR Gamma Control
	E_KDRV_XC_CFD_CTRL_SDR_GAMMA = 37,
	///SDR Degamma Control
	E_KDRV_XC_CFD_CTRL_SDR_DEGAMMA = 38,
	///Set Yc offset and gain in
	E_KDRV_XC_CFD_CTRL_YCOFFSETGAININ = 39,
	///Set Yc offset and gain out
	E_KDRV_XC_CFD_CTRL_YCOFFSETGAINOUT = 40,
	///Set Yc offset and gain in Sub
	E_KDRV_XC_CFD_CTRL_YCOFFSETGAININSUB = 41,
	///SDR 3DLUT Control
	E_KDRV_XC_CFD_CTRL_SDR_3DLUT = 42,
	/// CFD_CTRL_PICTURE_MODE
	E_KDRV_XC_CFD_CTRL_PICTURE_MODE = 43,
	/// Set Customer IP parameter
	E_KDRV_XC_CFD_CTRL_UPDATE_CUSTOMER_SETTING = 44,
	/// Get CFD version
	E_KDRV_XC_CFD_CTRL_GET_CFD_VERSION = 45,
	/// Update XC status into CFD, refer to ST_KDRV_XC_STATUS_INFO
	E_KDRV_XC_CFD_CTRL_UPDATE_XC_STATUS = 46,
	/// CFD_CTRL_TMO_CUS_MAPPING
	E_KDRV_XC_CFD_CTRL_TMO_CUS_MAPPING = 47,
	//NR SETTINGS
	E_KDRV_XC_CFD_CTRL_SET_NR_SETTINGS = 48,
	/// Set Metadata Path
	E_KDRV_XC_CFD_SET_METADATA_PATH = 49,
	/// Set Metadata to CFD
	E_KDRV_XC_CFD_SET_METADATA = 50,
	//set manual luma curve
	E_KDRV_XC_CFD_CTRL_SET_MANUAL_LUMA_CURVE = 51,
	//Get Luma Info
	E_KDRV_XC_CFD_CTRL_GET_LUMA_INFO = 52,
	//Set Stretch Settings
	E_KDRV_XC_CFD_CTRL_SET_STRETCH_SETTINGS = 53,
	//Get Chroma Info
	E_KDRV_XC_CFD_CTRL_GET_CHROMA_INFO = 54,
	//Set hash Info
	E_KDRV_XC_CFD_CTRL_SET_HASH_INFO = 55,

	/// CFD CTRL_TYPE_EX start
	/// (Set this base is 0x40000000 for int type)
	E_KDRV_XC_CFD_CTRL_TYPE_EX_BASE = 0x40000000,
	/// OSD_STATUS
	E_KDRV_XC_CFD_CTRL_GET_OSD_PROCESS_CONFIGS = 0x40000001,
	/// HDR Output Seamless
	E_KDRV_XC_CFD_CTRL_SET_OUTPUT_SEAMLESS = 0x40000002,
	/// HDR Output Colorimetry
	E_KDRV_XC_CFD_CTRL_SET_OUTPUT_COLORIMETRY = 0x40000003,
	/// HDR Output HDR Type
	E_KDRV_XC_CFD_CTRL_SET_OUTPUT_HDR_TYPE = 0x40000004,
	/// Enable XC Kernel Log
	E_KDRV_XC_CFD_CTRL_SET_DEBUG_LEVEL = 0x40000005,
	/// Updata HDR Output type lut
	E_KDRV_XC_CFD_CTRL_UPDATE_HDR_OUTPUT_RULE = 0x40000006,
	/// Get CFD Plus info
	E_KDRV_XC_CFD_CTRL_GET_CFDPLUS_INFO = 0x40000007,
	/// Set PostSync mode
	E_KDRV_XC_CFD_CTRL_SET_POSTSYNC_MODE = 0x40000008,

	/// For DV Source Device API
	/// (Set this base is 0x41000000 for DOVI SOURCE)
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_TYPE_BASE = 0x41000000,
	/// DV source Initialize
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_INIT = 0x41000001,
	/// DV source get HDR associated Information
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_GET_HDR_INFORMATION = 0x41000002,
	/// DV source set output format
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_SET_OUTPUT_FORMAT = 0x41000003,
	/// DV source set control InfoFrame
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_SET_INFOFRAME_CONTROL = 0x41000004,
	/// DV source set Sink EDID Data
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_SET_SINK_EDID = 0x41000005,
	/// DV source reset HDR settings
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_RESET = 0x41000006,
	/// DV source get Auxiliary Video Information Infoframe Data
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_GET_AVI_INFOFRAME_DATA = 0x41000007,
	/// DV source set graphic parameters to dovi driver
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_SET_GRAPHIC_PARAM = 0x41000008,
	/// DV source set HW bond to dovi driver
	E_KDRV_XC_CFD_CTRL_DOVI_SOURCE_SET_HW_BOND = 0x41000009,

	/// Max
	E_KDRV_XC_CFD_CTRL_MAX,
};


struct ST_KDRV_XC_CFD_CONTROL_INFO {
	enum EN_KDRV_XC_CFD_CTRL_TYPE enCtrlType;
	void *pParam;
	unsigned int u32ParamLen; //param length
	unsigned short u16ErrCode;
};

enum EN_ETHDR_HDR_TYPE {
	EN_ETHDR_HDR_TYPE_SDR = 0,
	EN_ETHDR_HDR_TYPE_HDR10,
	EN_ETHDR_HDR_TYPE_HLG,
	EN_ETHDR_HDR_TYPE_HDR10PLUS,
	EN_ETHDR_HDR_TYPE_DV,
	EN_ETHDR_HDR_TYPE_MAX
};

enum EN_ETHDR_COLOR_RANGE {
	EN_ETHDR_COLOR_RANGE_LIMIT = 0,
	EN_ETHDR_COLOR_RANGE_FULL,
	EN_ETHDR_COLOR_RANGE_MAX
};

enum EN_KDRV_XC_HDMI_PORT {
	E_KDRV_XC_HDMI_PORT_TX = 0,
	E_KDRV_XC_HDMI_PORT_RX_0,
	E_KDRV_XC_HDMI_PORT_RX_1,
	E_KDRV_XC_HDMI_PORT_RX_2,
	E_KDRV_XC_HDMI_PORT_RX_3,
};

enum EN_ETHDR_DV_HDR_FORMAT {
	EN_ETHDR_DV_FORMAT_DEFAULT = 0x00000000,
	EN_ETHDR_DV_FORMAT_SDR = 0x00000001,
	EN_ETHDR_DV_FORMAT_HDR10 = 0x00000002,
	EN_ETHDR_DV_FORMAT_DV = 0x00000004,
	EN_ETHDR_DV_FORMAT_HLG = 0x00000008,
};

enum EN_ETHDR_DV_LOW_LATENCY_MODE {
	EN_ETHDR_DV_MODE_DEFAULT,
	EN_ETHDR_DV_MODE_STANDARD,
	EN_ETHDR_DV_MODE_LOW_LATENCY,
};

enum EN_KDRV_XC_DOVI_SOURCE_SET_HDR_FORMAT {
	/// format DV
	E_KDRV_XC_DOVI_SOURCE_SET_FORMAT_DV = 0,
	/// format HDR10
	E_KDRV_XC_DOVI_SOURCE_SET_FORMAT_HDR10 = 1,
	/// format SDR
	E_KDRV_XC_DOVI_SOURCE_SET_FORMAT_SDR = 2,
	/// SDR 2020 reserved
	/// Max
	E_KDRV_XC_DOVI_SOURCE_SET_HDR_FORMAT_MAX,
};

/// DV source low latency mode
enum EN_KDRV_XC_DOVI_SOURCE_SET_LOW_LATENCY_MODE {
	/// DV standard mode
	E_KDRV_XC_DOVI_SOURCE_SET_STANDARD_MODE = 0,
	/// DV low latency mode
	E_KDRV_XC_DOVI_SOURCE_SET_LOW_LATENCY_MODE = 1,
	/// Max
	E_KDRV_XC_DOVI_SOURCE_SET_LOW_LATENCY_MODE_MAX,
};

enum EN_KDRV_HDMITX_DV_VSVDB_VERSION {
	E_KDRV_HDMITX_DV_VSVDB_INVALID,
	E_KDRV_HDMITX_DV_VSVDB_V0_4K60_0_YUV422_0_26BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V0_4K60_0_YUV422_1_26BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V0_4K60_1_YUV422_0_26BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V0_4K60_1_YUV422_1_26BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_0_15BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_1_15BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_1_YUV422_0_15BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_1_YUV422_1_15BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_0_LL_0_12BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_0_YUV422_1_LL_1_12BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_1_YUV422_0_LL_0_12BYTE,
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_1_YUV422_0_LL_1_12BYTE,//SDK v2.4
	E_KDRV_HDMITX_DV_VSVDB_V1_4K60_1_YUV422_1_LL_1_12BYTE,
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_2_12Byte,//SDK v2.4
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_2_13Byte,//SDK v2.5
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_2_16Byte,//SDK v2.5
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_3_12Byte_DM3,// SDK v2.5
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_0_int_3_12Byte_DM4,// SDK v2.5
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_1_int_0_12Byte,
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_0_YUV422_1_int_2_12Byte,
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_1_YUV422_1_int_1_12Byte,
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_1_YUV422_1_int_3_12Byte,
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_2_YUV422_0_int_3_12Byte,//SDK v2.4
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_2_YUV422_1_int_1_12Byte,
	E_KDRV_HDMITX_DV_VSVDB_v2_YUV444_2_YUV422_1_int_3_12Byte,
};


struct ST_KDRV_XC_DOVI_SOURCE_SET_OUTPUT_FORMAT {
	/// Structure version
	unsigned int u32Version;
	/// Structure length
	unsigned int u32Length;
	/// enum reference 'EN_KDRV_XC_DOVI_SOURCE_SET_HDR_FORMAT'
	unsigned char u8DoViOutputFormat;
	/// enum reference 'EN_KDRV_XC_DOVI_SOURCE_SET_LOW_LATENCY_MODE'
	unsigned char u8DoViLowLatencyMode;
};

struct ST_KDRV_XC_DOVI_SOURCE_SET_SINK_EDID {
	/// Structure version
	unsigned int u32Version;
	/// Structure length
	unsigned int u32Length;
	/// Vendor-Specific Video Data Block
	unsigned char u8VSVDB[KDRV_XC_EDID_EXTENDED_TAG_CODE_DATA_BLOCK_MAX_SIZE];
	/// HDR Static Metadata Data Block
	unsigned char u8HDRSMDB[KDRV_XC_EDID_EXTENDED_TAG_CODE_DATA_BLOCK_MAX_SIZE];
	// HDMI port
	enum EN_KDRV_XC_HDMI_PORT enHDMIPort;
};


struct ST_KDRV_XC_CFD_EDID {
	unsigned int  u32Version;
	unsigned short u16Length;

	unsigned char u8HDMISinkHDRDataBlockValid;
	//assign by E_CFD_VALIDORNOT
	//0 :Not valid
	//1 :valid

	unsigned char u8HDMISinkEOTF;
	//byte 3 in HDR static Metadata Data block

	unsigned char u8HDMISinkSM;
	//byte 4 in HDR static Metadata Data block

	unsigned char u8HDMISinkDesiredContentMaxLuminance;
	unsigned char u8HDMISinkDesiredContentMaxFrameAvgLuminance;
	unsigned char u8HDMISinkDesiredContentMinLuminance;
	//byte 5 ~ 7 in HDR static Metadata Data block

	unsigned char u8HDMISinkHDRDataBlockLength;
	//byte 1[4:0] in HDR static Metadata Data block

	/// Display primaries x, data *0.00002 0xC350 = 1
	unsigned short u16Display_Primaries_x[3];
	/// Display primaries y, data *0.00002 0xC350 = 1
	unsigned short u16Display_Primaries_y[3];
	/// White point x, data *0.00002 0xC350 = 1
	unsigned short u16White_point_x;
	/// White point y, data *0.00002 0xC350 = 1
	unsigned short u16White_point_y;
	//address 0x19h to 22h in base EDID

	unsigned char u8HDMISinkEDIDBaseBlockVersion;
	//address 0x12h in EDID base block

	unsigned char u8HDMISinkEDIDBaseBlockReversion;
	//address 0x13h in EDID base block

	unsigned char u8HDMISinkEDIDCEABlockReversion;
	//address 0x01h in EDID CEA block

	//table 59 Video Capability Data Block (VCDB)
	//0:VCDB is not available
	//1:VCDB is available
	unsigned char u8HDMISinkVCDBValid;

	unsigned char u8HDMISinkSupportYUVFormat;
	//bit 0:Support_YUV444
	//bit 1:Support_YUV422
	//bit 2:Support_YUV420

	//QY in Byte#3 in table 59 Video Capability Data Block (VCDB)
	//bit 3:RGB_quantization_range

	//QS in Byte#3 in table 59 Video Capability Data Block (VCDB)
	//bit 4:Y_quantization_range 0:no data(due to CE or IT video) ; 1:selectable

	unsigned char u8HDMISinkExtendedColorspace;
	//byte 3 of Colorimetry Data Block
	//bit 0:xvYCC601
	//bit 1:xvYCC709
	//bit 2:sYCC601
	//bit 3:Adobeycc601
	//bit 4:Adobergb
	//bit 5:BT2020 cl
	//bit 6:BT2020 ncl
	//bit 7:BT2020 RGB

	unsigned char u8HDMISinkEDIDValid;

	/// Vendor-Specific Video Data Block
	unsigned char u8VSVDB[32];
};


struct ST_ETHDR_HDR10_STATIC_METADATA {
	unsigned int colorPrimaries;
	unsigned int transformCharacter;
	unsigned int matrixCoeffs;
	unsigned int displayPrimariesX[3];
	unsigned int displayPrimariesY[3];
	unsigned int whitePointX;
	unsigned int whitePointY;
	unsigned int maxDisplayMasteringLuminance;
	unsigned int minDisplayMasteringLuminance;
	unsigned int maxContentLightLevel;
	unsigned int maxPicAverageLightLevel;
};

struct ST_ETHDR_HDR10PLUS_DYNAMIC_METADATA {
	unsigned int countryCode;/* The value shall be 0xB5 */
	unsigned int terminalProviderCode;/* The value shall be 0x003C */
	unsigned int terminalProviderOrientedCode;/* ST 2094-40 shall be 0x0001 */
	unsigned int applicationIdentifier;/* Application_identifier shall be set to 4 */
	unsigned int applicationVersion;/* Application_version shall be set to 0.  */
	unsigned int frame;
	unsigned int targetedSystemDisplayMaximumLuminance;
	unsigned int targetedSystemDisplayActualPeakLuminanceFlag; /* =0 */
	unsigned int maxscl[3];
	unsigned int averageMaxrgb;
	unsigned int numDistributionMaxrgbPercentiles;
	unsigned int distributionMaxrgbPercentages[10];
	unsigned int distributionMaxrgbPercentiles[10];
	unsigned int fractionBrightPixels;
	unsigned int masteringDisplayActualPeakLuminanceFlag;
	unsigned int toneMappingFlag;
	unsigned int kneePointX;
	unsigned int kneePointY;
	unsigned int numBezierCurveAnchors;/* 2~15 */
	unsigned int bezierCurveAnchors[15];
	unsigned int colorSaturationMappingFlag;/* =0 */
	unsigned int colorSaturationWeight; /* N.A. */
};

struct ST_ETHDR_METADATA {
	unsigned int planeId;
	enum EN_ETHDR_HDR_TYPE ehdrType;
	struct ST_ETHDR_HDR10_STATIC_METADATA hdr10StaticMetadata;
	struct ST_ETHDR_HDR10PLUS_DYNAMIC_METADATA hdr10PlusDynamicMetadata;
	unsigned int isSecure;
	unsigned int DVMetadataBufferFd;
	unsigned int DVMetadataBufferOffset;
	unsigned char *pDVMetadataBuffer;
	unsigned int DVMetadataBufferLength;
	unsigned int colorPrimaries;
	unsigned int transferCharacteristics;
	unsigned int matrixCoefficients;
	enum EN_ETHDR_COLOR_RANGE colorRange;
	unsigned int videoWidth;
	unsigned int videoHeight;
	bool bFirstFrame;
};

struct ST_KDRV_XC_AUTODOWNLOAD_CONFIG_INFO {
	enum EN_KDRV_XC_AUTODOWNLOAD_CLIENT enClient; /// current client
	unsigned long phyBaseAddr;					  /// baseaddr
	unsigned long virtBaseAddr;					  /// baseaddr
	unsigned int u32Size;						  /// size
	unsigned int u32MiuNo;						 /// miu no
	unsigned char bEnable;
	enum EN_KDRV_XC_AUTODOWNLOAD_MODE enMode;	 /// work mode
};

struct ST_KDRV_XC_DOVI_SOURCE_INIT {
	/// Structure version
	unsigned int u32Version;
	/// Structure length
	unsigned int u32Length;
	/// Init Dovi
	unsigned char bDoviInit;
	/// Window (main or sub window)
	unsigned char  u8Win;
	/// Input source
	unsigned char  u8InputSource;
};

struct ST_KDRV_XC_DOVI_SOURCE_GET_HDR_INFORMATION {
	/// Structure version
	unsigned int u32Version;
	/// Structure length
	unsigned int u32Length;
	/// DV source current output format,
	/// enum reference 'EN_KDRV_XC_DOVI_SOURCE_SET_HDR_FORMAT'
	unsigned char  u8DoViOutputFormat;
	/// DV source current input format,
	///enum reference 'EN_KDRV_XC_DOVI_SOURCE_SET_HDR_FORMAT'
	unsigned char  u8DoViInputFormat;
	/// DV source current init status
	unsigned char bDoViIsInited;
	/// DV source current low latency mode,
	///enum reference 'EN_KDRV_XC_DOVI_SOURCE_SET_LOW_LATENCY_MODE'
	unsigned char  u8DoViOutputLowLatency;
	/// DV source current graphic priority mode,
	///enum reference 'EN_KDRV_XC_DOVI_SOURCE_SET_GRAPHIC_PRIORITY_MODE'
	unsigned char  u8DoViGraphicPriority;
	/// DV source current graphic HDR type,
	///enum reference 'EN_KDRV_XC_DOVI_SOURCE_SET_GRAPHIC_HDR_TYPE'
	unsigned char  u8DoViGraphicHDRType;
};

struct ST_KDRV_HDMITX_INFOFRAME {
	unsigned char u8HDMISourceEOTF;
	unsigned char u8HDMISourceSMDID;
	unsigned short u16DisplayPrimariesX[3];
	unsigned short u16DisplayPrimariesY[3];
	unsigned short u16WhitePointX;
	unsigned short u16WhitePointY;
	unsigned short u16MasterPanelMaxLuminance;
	unsigned short u16MasterPanelMinLuminance;
	unsigned short u16MaxContentLightLevel;
	unsigned short u16MaxFrameAvgLightLevel;
	unsigned char u8HDMISourceColorspace;
	unsigned char u8OutputDataFormat;
};

struct ST_DV_VSIF {
	unsigned char bBackltCtrl;
	unsigned short  u16Max_PQ;
	unsigned char   u8Auxiliary_RunMode;
	unsigned char   u8Auxiliary_RunVersion;
	unsigned char   u8Auxiliary_Debug0;
};

struct ST_ETHDR_INTERFACE {
	void (*fpMLoadInit)(enum EN_MLOAD_CLIENT_TYPE _client_type, unsigned long PAddr,
		unsigned long VAddr, unsigned int u32BufByteLen);
	void (*fpMLoadEnable)(enum EN_MLOAD_CLIENT_TYPE _client_type, unsigned char bEnable);
	unsigned char (*fpMLoadWriteCmd)(enum EN_MLOAD_CLIENT_TYPE _client_type,
		unsigned int u32Addr, unsigned short u16Data, unsigned short u16Mask);
	unsigned char (*fpMLoadFire)(enum EN_MLOAD_CLIENT_TYPE _client_type,
		unsigned char bImmeidate);
	unsigned char (*fpConfigAutoDownload)(struct ST_KDRV_XC_AUTODOWNLOAD_CONFIG_INFO
		 *pstConfigInfo);
	unsigned char (*fpCfdCtrl)(struct ST_KDRV_XC_CFD_CONTROL_INFO *pstKdrvCFDCtrlInfo);
	unsigned char (*fpCalFrameInfo)(struct ST_ETHDR_METADATA *pstEthdrMetadata);
	unsigned char (*fpGetHdmiSetting)(struct ST_KDRV_HDMITX_INFOFRAME *pstHdrInfoFrame,
		struct ST_DV_VSIF *pstDvVsif, enum EN_KDRV_HDMITX_DV_VSVDB_VERSION *eDvVsvdbVer);
	void (*fpSetDvEnable)(unsigned char bDvEnable);
	unsigned char (*fpGetDvEnable)(void);
};

enum mtk_ethdr_comp_id {
	ETHDR_DISP_MIXER,
	ETHDR_VDO_FE0,
	ETHDR_VDO_FE1,
	ETHDR_GFX_FE0,
	ETHDR_GFX_FE1,
	ETHDR_VDO_BE,
	ETHDR_ADL_DS,
	ETHDR_ID_MAX
};

enum mtk_ethdr_async_id {
	ETHDR_ASYNC_VDO_FE0,
	ETHDR_ASYNC_VDO_FE1,
	ETHDR_ASYNC_GFX_FE0,
	ETHDR_ASYNC_GFX_FE1,
	ETHDR_ASYNC_VDO_BE,
	ETHDR_ASYNC_ID_MAX
};

int mtk_drm_ioctl_set_hdr_metadata(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
int mtk_drm_ioctl_set_hdr_on_off(struct drm_device *dev, void *data,
	struct drm_file *file_priv);

#ifdef CONFIG_DRM_MEDIATEK_HDMI
void mtk_ethdr_notify_hdmi_hpd(bool hpd);
#endif

void mtk_ethdr_layer_config(unsigned int idx,
					 struct mtk_plane_state *state,
					 struct cmdq_pkt *handle);
void mtk_ethdr_layer_on(unsigned int idx, struct cmdq_pkt *handle);
void mtk_ethdr_layer_off(unsigned int idx, struct cmdq_pkt *handle);
void mtk_ethdr_cal_frame(void);
void mtk_ethdr_fire(void);
bool mtk_ethdr_isDvMode(void);
void mtk_ethdr_get_output_hdr_type(enum EN_ETHDR_HDR_TYPE *peHdrType);
int mtk_ethdr_register_interface(struct ST_ETHDR_INTERFACE *pstEthdrInterface);
int mtk_ethdr_inform_dvko_insert(void);

#endif

