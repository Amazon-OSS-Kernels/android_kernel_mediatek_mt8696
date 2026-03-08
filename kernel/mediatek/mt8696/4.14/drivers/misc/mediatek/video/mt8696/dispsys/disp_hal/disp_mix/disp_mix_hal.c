/*
 * Copyright (C) 2019 MediaTek Inc.
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

#include "disp_mix_hal.h"
#include "hdmitx.h"
#include "disp_path.h"
#include "disp_clk.h"

int no_mix_fhd;
int no_mix_uhd;

struct disp_mix_context_t disp_mix;

int32_t disp_mix_hal_clock_on_off(bool on)
{
	DISP_MIX_LOG_D("%s %d\n", __func__, on);
	disp_clock_enable(DISP_CLK_DISP_MIX, on);
	return 0;
}

int32_t disp_mix_hal_dynamic_clk_mgr(bool on)
{
	DISP_MIX_LOG_D("%s %d\n", __func__, on);
	if (on) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_FUNC_DCM0),
			MIX_UPD_REG_CK_EN, MIX_UPD_REG_CK_EN);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_FUNC_DCM1),
			MIX_CK_EN, MIX_CK_EN);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_FUNC_DCM1),
			BG_CLR_CK_EN, BG_CLR_CK_EN);
	}

	return 0;
}

int32_t disp_mix_hal_mix_enable(bool enable)
{
	DISP_MIX_LOG_D("%s %d\n", __func__, enable);
	if (enable)
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_ENABLE),
			MIX_EN, MIX_EN);
	else
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_ENABLE),
			0, MIX_EN);

	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
		0x4 << 9, BACKGROUND_RELAY_MODE);

	return 0;
}

int32_t disp_mix_hal_set_dst_wigth_and_height(enum HDMI_VIDEO_RESOLUTION
	resolution, bool config_hw)
{
	UINT32 width = 0;
	UINT32 height = 0;

	DISP_MIX_LOG_D("%s: %d, config: %d\n", __func__, resolution,
		  config_hw);

	switch (resolution) {
	case HDMI_VIDEO_720x480i_60Hz:
	case HDMI_VIDEO_720x480p_60Hz:
		width = 720;
		height = 480;
		break;

	case HDMI_VIDEO_720x576i_50Hz:
	case HDMI_VIDEO_720x576p_50Hz:
		width = 720;
		height = 576;
		break;

	case HDMI_VIDEO_1280x720p_60Hz:
	case HDMI_VIDEO_1280x720p_50Hz:
	case HDMI_VIDEO_1280x720p3d_50Hz:
	case HDMI_VIDEO_1280x720p_59_94Hz:
	case HDMI_VIDEO_1280x720p3d_60Hz:
		width = 1280;
		height = 720;
		break;

	case HDMI_VIDEO_1920x1080i_60Hz:
	case HDMI_VIDEO_1920x1080i_50Hz:
	case HDMI_VIDEO_1920x1080p_60Hz:
	case HDMI_VIDEO_1920x1080p_50Hz:
	case HDMI_VIDEO_1920x1080p_24Hz:
	case HDMI_VIDEO_1920x1080p_23Hz:
	case HDMI_VIDEO_1920x1080p_25Hz:
	case HDMI_VIDEO_1920x1080p_29Hz:
	case HDMI_VIDEO_1920x1080p_30Hz:
	case HDMI_VIDEO_1920x1080p_59_94Hz:
		width = 1920;
		height = 1080;
		break;

	case HDMI_VIDEO_3840x2160P_23_976HZ:
	case HDMI_VIDEO_3840x2160P_24HZ:
	case HDMI_VIDEO_3840x2160P_25HZ:
	case HDMI_VIDEO_3840x2160P_29_97HZ:
	case HDMI_VIDEO_3840x2160P_30HZ:
	case HDMI_VIDEO_3840x2160P_60HZ:
	case HDMI_VIDEO_3840x2160P_50HZ:
	case HDMI_VIDEO_3840x2160P_59_94HZ:
		width = 3840;
		height = 2160;
		break;

	case HDMI_VIDEO_4096x2160P_24HZ:
	case HDMI_VIDEO_4096x2160P_50HZ:
	case HDMI_VIDEO_4096x2160P_60HZ:
	case HDMI_VIDEO_4096x2160P_59_94HZ:
		width = 4096;
		height = 2160;
		break;

	default:
		DISP_MIX_LOG_D("the resolution is wrong ,res: %d\n",
			resolution);
		break;
	}

	DISP_MIX_LOG_D("%s, width:%d, height:%d\n", __func__, width, height);
	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_ROI_SIZE), width, ROI_W);
	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_ROI_SIZE),
		height << 16, ROI_H);

	return 0;

}

int32_t disp_mix_hal_mix_sw_trigger(enum HDMI_VIDEO_RESOLUTION resolution,
				   bool sw_trigger)
{
	DISP_MIX_LOG_D("%s: %d, config: %d\n", __func__, resolution,
		  sw_trigger);
	/*WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_TRIG), */
		/*MIX_SW_TRIG, MIX_SW_TRIG);*/
	return 0;
}

int32_t disp_mix_hal_set_layer_location(struct DISP_PATH_LOCATION_INFO
	*location_info)
{
	enum DISP_PATH_TYPE layer_type = location_info->layer_type;
	UINT32 layer_location_id = location_info->layer_location_id;

	DISP_MIX_LOG_D("%s: type:%d, id:%d\n",
		__func__, layer_type, layer_location_id);

	if ((layer_type == DISP_MAIN_VDO)
		&& (layer_type == layer_location_id)) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			L0_SEL_INTERFACE_L0_SRC, L0_SRC_SEL);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			OUTPUT_L0_SEL_MIXER_L0_SRC, L0_OUT_SEL);
	}

	if ((layer_type == DISP_SUB_VDO) && (layer_type == layer_location_id)) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			L1_SEL_INTERFACE_L1_SRC, L1_SRC_SEL);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			OUTPUT_L1_SEL_MIXER_L1_SRC, L1_OUT_SEL);
	}

	if ((layer_type == DISP_OSD_FHD) && (layer_type == layer_location_id)) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			L2_SEL_INTERFACE_L2_SRC, L2_SRC_SEL);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			OUTPUT_L2_SEL_MIXER_L2_SRC, L2_OUT_SEL);
	}

	if ((layer_type == DISP_OSD_UHD) && (layer_type == layer_location_id)) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			L3_SEL_INTERFACE_L3_SRC, L3_SRC_SEL);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			OUTPUT_L3_SEL_MIXER_L3_SRC, L3_OUT_SEL);
	}

	return 0;

}

int32_t disp_mix_hal_for_layer_swap(struct DISP_PATH_LOCATION_INFO
	*location_info)
{
	enum DISP_PATH_TYPE layer_type = location_info->layer_type;
	UINT32 layer_location_id = location_info->layer_location_id;

	if ((layer_type == DISP_MAIN_VDO)
		&& (layer_type == layer_location_id)) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			L0_SEL_INTERFACE_L0_SRC, L0_SRC_SEL);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			OUTPUT_L0_SEL_MIXER_L0_SRC, L0_OUT_SEL);
	}

	if ((layer_type == DISP_SUB_VDO) && (layer_type == layer_location_id)) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			L1_SEL_INTERFACE_L1_SRC, L1_SRC_SEL);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			OUTPUT_L1_SEL_MIXER_L1_SRC, L1_OUT_SEL);
	}

	if ((layer_type == DISP_OSD_FHD) && (layer_type == layer_location_id)) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			L2_SEL_INTERFACE_L2_SRC, L2_SRC_SEL);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			OUTPUT_L2_SEL_MIXER_L2_SRC, L2_OUT_SEL);
	}

	if ((layer_type == DISP_OSD_UHD) && (layer_type == layer_location_id)) {
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			L3_SEL_INTERFACE_L3_SRC, L3_SRC_SEL);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
			OUTPUT_L3_SEL_MIXER_L3_SRC, L3_OUT_SEL);
	}

	if (layer_type != layer_location_id) {

		if (layer_type == DISP_MAIN_VDO) {
			switch (layer_location_id) {
			case 0:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L0_SEL_INTERFACE_L0_SRC, L0_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L0_SEL_MIXER_L0_SRC, L0_OUT_SEL);
				break;
			case 1:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L1_SEL_INTERFACE_L0_SRC, L1_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L1_SEL_MIXER_L0_SRC, L1_OUT_SEL);
				break;
			case 2:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L2_SEL_INTERFACE_L0_SRC, L2_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L2_SEL_MIXER_L0_SRC, L2_OUT_SEL);
				break;
			case 3:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L3_SEL_INTERFACE_L0_SRC, L3_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L3_SEL_MIXER_L0_SRC, L3_OUT_SEL);
				break;
			default:
				DISP_MIX_LOG_D(
					"wrong layer id, swap main vdo failed.\n");
				return 0;
			}
		}

		if (layer_type == DISP_SUB_VDO) {
			switch (layer_location_id) {
			case 0:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L0_SEL_INTERFACE_L1_SRC, L0_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L0_SEL_MIXER_L1_SRC, L0_OUT_SEL);
				break;
			case 1:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L1_SEL_INTERFACE_L1_SRC, L1_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L1_SEL_MIXER_L1_SRC, L1_OUT_SEL);
				break;
			case 2:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L2_SEL_INTERFACE_L1_SRC, L2_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L2_SEL_MIXER_L1_SRC, L2_OUT_SEL);
				break;
			case 3:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L3_SEL_INTERFACE_L1_SRC, L3_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L3_SEL_MIXER_L1_SRC, L3_OUT_SEL);
				break;
			default:
				DISP_MIX_LOG_D(
					"wrong layer id, swap sub vido failed.\n");
				return 0;
			}
		}

		if (layer_type == DISP_OSD_FHD) {
			switch (layer_location_id) {
			case 0:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L0_SEL_INTERFACE_L2_SRC, L0_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L0_SEL_MIXER_L2_SRC, L0_OUT_SEL);
				break;
			case 1:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L1_SEL_INTERFACE_L2_SRC, L1_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L1_SEL_MIXER_L2_SRC, L1_OUT_SEL);
				break;
			case 2:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L2_SEL_INTERFACE_L2_SRC, L2_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L2_SEL_MIXER_L2_SRC, L2_OUT_SEL);
				break;
			case 3:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L3_SEL_INTERFACE_L2_SRC, L3_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L3_SEL_MIXER_L2_SRC, L3_OUT_SEL);
				break;
			default:
				DISP_MIX_LOG_D(
					"wrong layer id, swap osd fhd failed.\n");
				return 0;
			}
		}

		if (layer_type == DISP_OSD_UHD) {
			switch (layer_location_id) {
			case 0:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L0_SEL_INTERFACE_L3_SRC, L0_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L0_SEL_MIXER_L3_SRC, L0_OUT_SEL);
				break;
			case 1:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L1_SEL_INTERFACE_L3_SRC, L1_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L1_SEL_MIXER_L3_SRC, L1_OUT_SEL);
				break;
			case 2:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L2_SEL_INTERFACE_L3_SRC, L2_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L2_SEL_MIXER_L3_SRC, L2_OUT_SEL);
				break;
			case 3:
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					L3_SEL_INTERFACE_L3_SRC, L3_SRC_SEL);
				WriteREG32Msk(
					(DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
					OUTPUT_L3_SEL_MIXER_L3_SRC, L3_OUT_SEL);
				break;
			default:
				DISP_MIX_LOG_D(
					"wrong layer id, swap uhd osd failed.\n");
				return 0;
			}
		}
	}
	return 0;
}

int32_t disp_mix_hal_set_black_pattern(bool en)
{
	uint32_t black_status = 0;

	if (en) {
		WriteREG32Msk(DISP_MIX_BASE + DISP_MIX_OUTPUT_CCLR,
			HDR_BALCK, HDR_BALCK);
		black_status = ReadREG32(DISP_MIX_BASE + DISP_MIX_OUTPUT_CCLR);
		WriteREG32(DISP_MIX_BASE + DISP_MIX_LAYER0_CCLR, 0xff048080);
		WriteREG32(DISP_MIX_BASE + DISP_MIX_LAYER1_CCLR, 0xff048080);
		WriteREG32(DISP_MIX_BASE + DISP_MIX_LAYER2_CCLR, 0xff048080);
		WriteREG32(DISP_MIX_BASE + DISP_MIX_LAYER3_CCLR, 0xff048080);
		WriteREG32Msk(DISP_MIX_BASE + DISP_MIX_SRC_CTRL,
			0xF << 4, 0xF << 4);
	} else {
		WriteREG32Msk(DISP_MIX_BASE + DISP_MIX_OUTPUT_CCLR,
			0x0, HDR_BALCK);
		black_status = ReadREG32(DISP_MIX_BASE + DISP_MIX_OUTPUT_CCLR);
		if (!(black_status & HDMI_BALCK)) {
			WriteREG32(DISP_MIX_BASE + DISP_MIX_LAYER0_CCLR, 0x0);
			WriteREG32(DISP_MIX_BASE + DISP_MIX_LAYER1_CCLR, 0x0);
			WriteREG32(DISP_MIX_BASE + DISP_MIX_LAYER2_CCLR, 0x0);
			WriteREG32(DISP_MIX_BASE + DISP_MIX_LAYER3_CCLR, 0x0);
			WriteREG32Msk(DISP_MIX_BASE + DISP_MIX_SRC_CTRL,
				0x0 << 4, 0xF << 4);
		}
	}

	DISP_MIX_LOG_I("dispmix black %d 0x%x\n", en, black_status);

	return 0;
}


int32_t disp_mix_hal_layer_control(struct DISP_PATH_LAYER_INFO *layer_info)
{
	enum DISP_PATH_TYPE type = layer_info->type;
	bool layer_enable = layer_info->enable;
	bool alpha_blending = layer_info->alpha_blending;
	UINT32 alpha_value = layer_info->alpha_value;
	bool pre_multi = layer_info->pre_multi;
	UINT32 src_width = layer_info->src_width;
	UINT32 src_height = layer_info->src_height;
	UINT32 x_offset = layer_info->x_offset;
	UINT32 y_offset = layer_info->y_offset;
	bool large_timing_small_region = layer_info->large_timing_small_region;
	UINT32 mix_layer_con_offset = 0;
	UINT32 mix_layer_src_size_offset = 0;
	UINT32 mix_layer_offset = 0;

	DISP_MIX_LOG_D("%s: type:%d,enable:%d, blending:%d, value:%d\n",
		__func__, type, layer_enable, alpha_blending, alpha_value);
	DISP_MIX_LOG_D("pre_multi:%d, width:%d, height:%d\n",
		pre_multi, src_width, src_height);
	DISP_MIX_LOG_D("x_ofst:%d, y_ofst:%d, l_t_small_region:%d\n",
		x_offset, y_offset, large_timing_small_region);
	switch (type) {
	case DISP_MAIN_VDO:
		mix_layer_con_offset = DISP_MIX_LAYER0_CON;
		mix_layer_src_size_offset = DISP_MIX_LAYER0_SRC_SIZE;
		mix_layer_offset = DISP_MIX_LAYER0_OFFSET;
		if (layer_enable)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				L0_ENABLE, L0_ENABLE);
		else
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				0, L0_ENABLE);
		if (large_timing_small_region)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
				L0_LARGE_TIMING_SMALL_REGION_SEL,
				L0_LARGE_TIMING_SMALL_REGION_SEL);
		else
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
				0, L0_LARGE_TIMING_SMALL_REGION_SEL);
		break;
	case DISP_SUB_VDO:
		mix_layer_con_offset = DISP_MIX_LAYER1_CON;
		mix_layer_src_size_offset = DISP_MIX_LAYER1_SRC_SIZE;
		mix_layer_offset = DISP_MIX_LAYER1_OFFSET;
		if (layer_enable)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				L1_ENABLE, L1_ENABLE);
		else
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				0, L1_ENABLE);
		if (large_timing_small_region)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
				L1_LARGE_TIMING_SMALL_REGION_SEL,
				L1_LARGE_TIMING_SMALL_REGION_SEL);
		else
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
				0, L1_LARGE_TIMING_SMALL_REGION_SEL);
		break;
	case DISP_OSD_FHD:
		mix_layer_con_offset = DISP_MIX_LAYER2_CON;
		mix_layer_src_size_offset = DISP_MIX_LAYER2_SRC_SIZE;
		mix_layer_offset = DISP_MIX_LAYER2_OFFSET;
		if (no_mix_fhd)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				0, L2_ENABLE);
		else if (layer_enable)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				L2_ENABLE, L2_ENABLE);
		else
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				0, L2_ENABLE);
		if (large_timing_small_region)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
				L2_LARGE_TIMING_SMALL_REGION_SEL,
				L2_LARGE_TIMING_SMALL_REGION_SEL);
		else
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
				0, L2_LARGE_TIMING_SMALL_REGION_SEL);
		break;
	case DISP_OSD_UHD:
		mix_layer_con_offset = DISP_MIX_LAYER3_CON;
		mix_layer_src_size_offset = DISP_MIX_LAYER3_SRC_SIZE;
		mix_layer_offset = DISP_MIX_LAYER3_OFFSET;
		if (no_mix_uhd)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				0, L3_ENABLE);
		else if (layer_enable)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				L3_ENABLE, L3_ENABLE);
		else
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_SRC_CTRL),
				0, L3_ENABLE);
		if (large_timing_small_region)
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
				L3_LARGE_TIMING_SMALL_REGION_SEL,
				L3_LARGE_TIMING_SMALL_REGION_SEL);
		else
			WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
				0, L3_LARGE_TIMING_SMALL_REGION_SEL);
		break;
	default:
		DISP_MIX_LOG_D("the type is wrong , type: %d\n", type);
		break;
	}

	if (alpha_blending) {
		WriteREG32Msk((DISP_MIX_BASE + mix_layer_con_offset),
			ALPHA_BLENDING_ENABLE, ALPHA_BLENDING_ENABLE);
		WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_LAYER0_CON),
			alpha_value, ALPHA_MASK);
	} else
		WriteREG32Msk((DISP_MIX_BASE + mix_layer_con_offset), 0,
			ALPHA_BLENDING_ENABLE);
	if (pre_multi)
		WriteREG32Msk((DISP_MIX_BASE + mix_layer_con_offset),
			CLRFMT_PREMULTI, CLRFMT_MASK);
	else
		WriteREG32Msk((DISP_MIX_BASE + mix_layer_con_offset),
			CLRFMT_NON_PREMULTI, CLRFMT_MASK);
	/*set source width and height of layer*/
	WriteREG32Msk((DISP_MIX_BASE + mix_layer_src_size_offset), src_width,
		SRC_W);
	WriteREG32Msk((DISP_MIX_BASE + mix_layer_src_size_offset),
		src_height << 16, SRC_H);
	if ((src_width % 2) != 0)
		WriteREG32Msk((DISP_MIX_BASE + mix_layer_src_size_offset),
			1 << 31, REGION_ODD);
	else
		WriteREG32Msk((DISP_MIX_BASE + mix_layer_src_size_offset),
			0 << 31, REGION_ODD);

	/*set source x_offset and y_offset of layer*/
	WriteREG32Msk((DISP_MIX_BASE + mix_layer_offset), x_offset, X_OFFSET);
	WriteREG32Msk((DISP_MIX_BASE + mix_layer_offset),
		y_offset << 16, Y_OFFSET);

	if ((x_offset % 2) != 0)
		WriteREG32Msk((DISP_MIX_BASE + mix_layer_offset),
			1 << 31, X_OFFSET_ODD);
	else
		WriteREG32Msk((DISP_MIX_BASE + mix_layer_offset),
			0 << 31, X_OFFSET_ODD);

	/*out put no round*/
	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
		OUTPUT_NO_ROUND,
		OUTPUT_NO_ROUND);

	/*disp mix source select rgb*/
	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_DATA_PATH_CTRL),
		SOURCE_RGB_SELECT,
		SOURCE_RGB_SELECT);

	return 0;
}

int32_t disp_mix_hal_set_background_color(uint32_t background)
{
	uint32_t blue = background & 0xFF;
	uint32_t green = (background & 0xFF00) >> 8;
	uint32_t red = (background & 0xFF0000) >> 16;

	DISP_MIX_LOG_D("blue:0x%x, green:0x%x, red:0x%x\n",
		blue, green, red);
	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_ROI_BGCLR),
		blue, COLOR_BLUE);
	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_ROI_BGCLR),
		(green << 8), COLOR_GREEN);
	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_ROI_BGCLR),
		(red << 16), COLOR_RED);
	WriteREG32Msk((DISP_MIX_BASE + DISP_MIX_ROI_BGCLR),
		(0xff << 24), COLOR_ALPHA);

	return 0;
}

int disp_mix_hal_init(struct disp_mix_init_param *param)
{
	if (disp_mix.inited)
		return 0;

	disp_mix.init_param.disp_mix_reg_base = param->disp_mix_reg_base;
	DISP_MIX_LOG_D("dispsys reg base: 0x%lx\n",
		       disp_mix.init_param.disp_mix_reg_base);
	if (!disp_mix.init_param.disp_mix_reg_base) {
		DISP_MIX_LOG_I("can not get correct register base.\n");
		return -1;
	}

	disp_mix.log_level = 1;
	disp_mix.inited = 1;

	return 0;
}
