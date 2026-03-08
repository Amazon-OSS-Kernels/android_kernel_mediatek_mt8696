/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __DISP_CFD_TZ_CLIENT_H__
#define __DISP_CFD_TZ_CLIENT_H__

#include "disp_info.h"

#include <linux/list.h>
#include <linux/mutex.h>

#include "disp_hw_mgr.h"
#include "hdmitx.h"

#define PTA_CFD_NAME "cfd_ta"

#define TZ_TA_CFD_UUID   "64697370-5f63-6664-5f74-615f75756964"

#define CFD_LAYER_MAX 4
#define CFD_HDR_GENERAL_HW_OUTPUT_NUM 1000
#define CFD_POSTSDR_GENERAL_HW_OUTPUT_NUM 500
#define CFD_GOP_HDR_GENERAL_HW_OUTPUT_NUM 1000
#define CFD_GOP_POSTSDR_GENERAL_HW_OUTPUT_NUM 100
#define CFD_ADL_LUT_SIZE_B0103  (256 * 4)
#define CFD_ADL_LUT_SIZE_B0102  (512 * 4)
#define CFD_ADL_LUT_SIZE_B0105  (512 * 4)
#define CFD_ADL_LUT_SIZE_OOTF   (512 * 4)
#define CFD_ADL_LUT_SIZE_GOP_DEGAMMA  (256 * 4)

struct CFD_SRC_ACTIVE_T {
	uint32_t layer_id;
	uint32_t width;
	uint32_t height;
};

struct CFD_SRC_ORDER_T {
	uint32_t layer_id;
	uint32_t in_order;
	uint32_t out_order;
};

enum cfd_status {
	CFD_STATUS_OK = 0,
	CFD_STATUS_ERROR = 1,
};


#define BS_BUF_SIZE          1024
#define CFD_MD_DW_SIZE       256

enum CFD_TZ_CALL_CMD {
	CFD_TZ_CALL_CMD_INIT,
	CFD_TZ_CALL_CMD_DEINIT,
	CFD_TZ_CALL_CMD_SEND_CLI_INFO,
	CFD_TZ_CALL_CMD_INIT_SHARE_MEMORY,
	CFD_TZ_CALL_CMD_DEBUG_LEVEL_INIT,
	CFD_TZ_CALL_CMD_REGISTER_IRQ,
	CFD_TZ_CALL_CMD_SHARE_MEMORY_INIT,
	CFD_TZ_CALL_CMD_SEC_MEM_COPY,
	CFD_TZ_CALL_CMD_CONFIG_FRAME_LAYER0,
	CFD_TZ_CALL_CMD_CONFIG_FRAME_LAYER1,
	CFD_TZ_CALL_CMD_CONFIG_FRAME_LAYER2,
	CFD_TZ_CALL_CMD_CONFIG_FRAME_LAYER3,
	CFD_TZ_CALL_CMD_TEST_CASE,
	CFD_TZ_CALL_CMD_BACKUP_HDR10PLUS_SECURE_HANDLE,
	CFD_TZ_CALL_CMD_PARSER_HDR10PLUS_SECURE_HANDLE,
	CFD_TZ_CALL_CMD_MAX,
};

enum CFD_TZ_CALL_DIR {
	CFD_TZ_CALL_DIR_NONE = 0,
	CFD_TZ_CALL_DIR_VALUE_INPUT = 1,
	CFD_TZ_CALL_DIR_VALUE_OUTPUT = 2,
	CFD_TZ_CALL_DIR_VALUE_INOUT = 3,
	CFD_TZ_CALL_DIR_MEM_INPUT = 4,
	CFD_TZ_CALL_DIR_MEM_OUTPUT = 5,
	CFD_TZ_CALL_DIR_MEM_INOUT = 6,
	CFD_TZ_CALL_DIR_MEMREF_INPUT = 7,
	CFD_TZ_CALL_DIR_MEMREF_OUTPUT = 8,
	CFD_TZ_CALL_DIR_MEMREF_INOUT = 9,
};

struct cfd_control_param_t {
	unsigned char u8Input_Source;
	unsigned char u8Input_Format;
	unsigned char u8InputDataDepth;
	unsigned char u8Input_DataFormat;
	unsigned char u8Input_IsFullRange;
	unsigned char u8Input_HDRMode;
	unsigned char u8Output_Source;
	unsigned char u8Output_Format;
	unsigned char u8Output_DataFormat;
	unsigned char u8Output_IsFullRange;
	unsigned char u8Output_HDRMode;
	unsigned short u16Source_Max_Luminance;    //data * 1 nits
	unsigned short u16Source_Med_Luminance;   //data * 1 nits
	unsigned short u16Source_Min_Luminance;    //data * 0.0001 nits
	unsigned short u16Target_Max_Luminance;    //data * 1 nits
	unsigned short u16Target_Med_Luminance;   //data * 1 nits
	unsigned short u16Target_Min_Luminance;
	unsigned short src_width;
	unsigned short src_height;
};

struct STU_CFD_COLORIMETRY {
	//order R->G->B
	unsigned short u16Display_Primaries_x[3]; //data *0.00002 0xC350 = 1
	unsigned short u16Display_Primaries_y[3]; //data *0.00002 0xC350 = 1
	unsigned short u16White_point_x;  //data *0.00002 0xC350 = 1
	unsigned short u16White_point_y;  //data *0.00002 0xC350 = 1
};

struct STU_CFD_HDR10PLUS_SEI {
	unsigned char u8Itu_t_t35_country_code;
	// set to 0xB5 for HDR10+

	unsigned char u16Itu_t_t35_terminal_provider_code;
	// set to 0x003C for HDR10+

	unsigned char u16Itu_t_t35_terminal_provider_oriented_code;
	// set to 0x0001 for HDR10+

	unsigned char u8Application_Identifier;
	// set to 4 for HDR10+

	unsigned char u8Application_Version;
	// set to 1 for HDR10+

	unsigned char u8Num_Windows;
	// set to 1 for HDR10+

	unsigned int u32Target_System_Display_Max_Luminance;
	// [0, max:10000], value>10000 -> value=10000 nits

	unsigned char u8Target_System_Display_Actual_Peak_Luminance_Flag;
	// set to 0 for HDR10+

	unsigned int u32MaxSCL[3];
	// 0x00000-0x186A0
	// MAY be set to 0x00000 to indicate "Not Used" by the content provider

	unsigned int u32Average_MaxRGB;
	// 0x00000-0x186A0

	unsigned char u8Num_Distributions;
	// set to 9 for HDR10+

	unsigned char u8Distribution_Index[9];
	// [0]: 1
	// [1]: 5
	// [2]: 10
	// [3]: 25
	// [4]: 50
	// [5]: 75
	// [6]: 90
	// [7]: 95
	// [8]: 99

	unsigned int u32Distribution_Values[9];
	// [0]: 1% percentile linearized maxRGB value * 10
	// [1]: DistributionY99 * 10
	// [2]: DistributionY100nit
	// [3]: 25% percentile linearized maxRGB value * 10
	// [4]: 50% percentile linearized maxRGB value * 10
	// [5]: 75% percentile linearized maxRGB value * 10
	// [6]: 90% percentile linearized maxRGB value * 10
	// [7]: 95% percentile linearized maxRGB value * 10
	// [8]: 99.98% percentile linearized maxRGB value * 10
	// [idx=2]: 0-100; [others]: 0x00000-0x186A0

	unsigned short u16Fraction_Bright_Pixels;
	// set to 0 for HDR10+

	unsigned char u8Master_Display_Actual_Peak_Luminance_Flag;
	// set to 0 for HDR10+

	unsigned char u8Tone_Mapping_Flag;
	// set to 0 for Profile A
	// set to 1 for Profile B: with Basis Tone Mapping Curve (Bezier curve)

	unsigned short u16Knee_Point_x;
	unsigned short u16Knee_Point_y;
	// 12 bits [0, 4095]

	unsigned char u8Num_Bezier_Curve_Anchors;
	// 0-9
	// if value=0, no u16Bezier_Curve_Anchors information

	unsigned short u16Bezier_Curve_Anchors[9];
	// 10 bits [0, 1023]

	unsigned char u8Color_Saturation_Mapping_Flag;
	// set to 0 for HDR10+
};

struct cfd_mm_param_t {
	unsigned char u8Colour_primaries;
	unsigned char u8Transfer_Characteristics;
	unsigned char u8Matrix_Coeffs;
	unsigned int u32Master_Panel_Max_Luminance;
	unsigned int u32Master_Panel_Min_Luminance;
	unsigned short u16Max_content_light_level;
	unsigned short u16Max_pic_average_light_level;
	struct STU_CFD_COLORIMETRY stu_Cfd_MM_MasterPanel_ColorMetry;
	struct STU_CFD_HDR10PLUS_SEI stu_Cfd_MM_HDR10plus_SEI;
};

struct STU_CFD_HDR10PLUS_VSVDB {
	unsigned char u8Tag_Code;
	// set to 7 for HDR10+

	unsigned char u8Length;
	// set to 5 for HDR10+

	unsigned char u8Extended_Tag_Code;
	// set to 0x01 for HDR10+

	unsigned int u32IEEE_24Bit_Code;
	// set to 0x90848B for HDR10+

	unsigned char u8Application_Version;
	// set to 1 for HDR10+
};

struct STU_CFD_HDR10PLUS_VSIF {
	unsigned char u8VSIF_Type_Code;
	// set to 0x01 for HDR10+

	unsigned char u8VSIF_Version;
	// set to 0x01 for HDR10+

	unsigned char u8Length;
	// set to 27 for HDR10+

	unsigned int u32IEEE_24Bit_Code;
	// set to 0x90848B for HDR10+

	unsigned char u8Application_Version;
	// set to 1 for HDR10+

	unsigned char u8Target_System_Display_Max_Luminance;
	// 0-31, unit:32 -> [0, 992]

	unsigned char u8Average_MaxRGB;
	// 0-255, unit:16 -> [0, 4080]

	unsigned char u8Distribution_Values[9];
	// [idx=2]: 0-100
	// [others]: 0-255, unit: 16 -> [0, 4080]

	unsigned short u16Knee_Point_x;
	unsigned short u16Knee_Point_y;
	// 0-1023, unit:4 -> [0, 4092]

	unsigned char u8Num_Bezier_Curve_Anchors;
	// 0-9
	// if value=0, no u16Bezier_Curve_Anchors information

	unsigned char u8Bezier_Curve_Anchors[9];
	// 0-255, unit:4 -> [0, 1020]

	unsigned char u8Graphics_Overlay_Flag;
	// set to 1 to indicate that video signal associated with the HDR10+

	unsigned char u8No_Delay_Flag;
	// SHALL always set to 0
	// set to 0: video signal associated with the HDR10+ Metadata is transm
	//           i.e. HDR10+ Metadata is transmitted
	// set to 1: video signal associated with

};

struct cfd_hdr10_md_t {
	uint32_t ui2_DisplayPrimariesX[3];
	uint32_t ui2_DisplayPrimariesY[3];
	uint32_t ui2_WhitePointX;
	uint32_t ui2_WhitePointY;
	uint32_t ui2_MaxDisplayMasteringLuminance;
	uint32_t ui2_MinDisplayMasteringLuminance;
	uint32_t ui2_MaxCLL;
	uint32_t ui2_MaxFALL;
};

struct CFD_STU_Register_Table {
	uint32_t u32Depth;
	uint32_t au32Addr[CFD_HDR_GENERAL_HW_OUTPUT_NUM];
	uint16_t au16Value[CFD_HDR_GENERAL_HW_OUTPUT_NUM];
	uint16_t au16Mask[CFD_HDR_GENERAL_HW_OUTPUT_NUM];
};

struct CFD_STU_Autodownload_Table {
	uint16_t u16client;
	uint8_t au8Data[CFD_ADL_LUT_SIZE_B0102];
	uint32_t u32Size;
};

struct CFD_HW_OUTPUT {
	uint32_t u32LayerId;
	uint32_t u32Version;
	uint32_t u16Length;
	struct CFD_STU_Register_Table stRegHdrTable;
	struct CFD_STU_Register_Table stRegSdrTable;
	struct CFD_STU_Autodownload_Table steotfAdlTable;
	struct CFD_STU_Autodownload_Table stootfAdlTable;
	struct CFD_STU_Autodownload_Table stoetfAdlTable;
	struct CFD_STU_Autodownload_Table sttmoAdlTable;
};


struct cfd_share_memory_info_t {
	unsigned char layer_id;
	unsigned char log_level;
	unsigned char dbg_id;
	bool svp;
	bool graphic_overlay;
	bool vsif_sync;
	uint32_t frame_num;
	uint32_t sec_handle_type;
	uint32_t sec_handle_len;
	uint32_t sec_handle_in;
	uint32_t sec_handle_out;
	struct cfd_control_param_t ctl_param; /* in: control_path_test in */
	struct cfd_mm_param_t mm_param; /* in: control_path_test in param */
	struct STU_CFD_HDR10PLUS_VSVDB hdr10plus_vsvdb; /* in:  in param */
	struct STU_CFD_HDR10PLUS_VSIF hdr10plus_vsif; /* out:  hdr10 md */
	struct cfd_hdr10_md_t hdr10_info_frame; /* out:  hdr10 md */
	struct CFD_HW_OUTPUT cfd_hw_output; /* out: */
};

struct cfd_share_memory_info_t_ar {
	struct cfd_share_memory_info_t cfd_shm_info[CFD_LAYER_MAX];
};


enum cfd_status cfd_sec_init(void);
enum cfd_status cfd_sec_deinit(void);
enum cfd_status cfd_sec_status(void);
enum cfd_status cfd_sec_config_frame(uint32_t layer_id);
enum cfd_status cfd_sec_test_case(uint32_t layer_id,
	uint32_t test_no, uint32_t mode);
enum cfd_status cfd_backup_hdr10plus_sec_handle(void *buffer,
	uint32_t size);
struct cfd_share_memory_info_t *cfd_sec_get_share_mem(int layer_id);
enum cfd_status cfd_sec_parser_hdr10plus_vsif(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t sec_len);
enum cfd_status cfd_sec_parser_hdr10plus_emp(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t sec_len);
enum cfd_status cfd_sec_debug_level_init(uint32_t cfd_log_level);


#endif
