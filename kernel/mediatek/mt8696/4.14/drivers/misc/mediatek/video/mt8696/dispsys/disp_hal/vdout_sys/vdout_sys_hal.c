/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include "hdmitx.h"

#include "disp_clk.h"
#include "disp_def.h"
#include "disp_type.h"
#include "vdout_sys_hal.h"
#include "vdout_sys_hw.h"

struct vdout_context_t vdout;

void vdout_sys_hal_4k2k_clock_enable(uint32_t res)
{
	VDOUT_LOG_D("set 4k2k clock enable, res=%d\n", res);

	if (fgResIs4k2k(res)) {
		if ((res == HDMI_VIDEO_3840x2160P_60HZ) ||
		    (res == HDMI_VIDEO_3840x2160P_50HZ) ||
		    (res == HDMI_VIDEO_4096x2160P_60HZ) ||
		    (res == HDMI_VIDEO_4096x2160P_50HZ)) {
			vWriteVDOUTMsk(VDTCLK_CFG3, 0,
				       VDO4_296M_EN | FMT_296M_EN);
			vWriteVDOUTMsk(HDMI_CONFIG_SUB, 0,
				       RGB2HDMI_296M_EN_SUB);
			vWriteVDOUTMsk(VDTCLK_CONFIG4, 0, VDO3_296M_EN);
			vWriteVDOUTMsk(VDTCLK_CONFIG4,
					RGB2HDMI_594M_EN | FMT_594M_EN |
						VDO3_594M_EN | VDO4_594M_EN,
					RGB2HDMI_594M_EN | FMT_594M_EN |
						VDO3_594M_EN | VDO4_594M_EN);
		} else {
			vWriteVDOUTMsk(VDTCLK_CFG3, VDO4_296M_EN | FMT_296M_EN,
				       VDO4_296M_EN | FMT_296M_EN);
			vWriteVDOUTMsk(VDTCLK_CONFIG4, VDO3_296M_EN,
					VDO3_296M_EN);
			vWriteVDOUTMsk(HDMI_CONFIG_SUB, RGB2HDMI_296M_EN_SUB,
				       RGB2HDMI_296M_EN_SUB);
			vWriteVDOUTMsk(VDTCLK_CONFIG4, 0,
					RGB2HDMI_594M_EN | FMT_594M_EN |
						VDO3_594M_EN | VDO4_594M_EN);
		}
	} else {
		vWriteVDOUTMsk(VDTCLK_CFG3, 0, VDO4_296M_EN | FMT_296M_EN);
		vWriteVDOUTMsk(HDMI_CONFIG_SUB, 0, RGB2HDMI_296M_EN_SUB);
		vWriteVDOUTMsk(VDTCLK_CONFIG4, 0,
				VDO3_296M_EN | RGB2HDMI_594M_EN | FMT_594M_EN |
					VDO3_594M_EN | VDO4_594M_EN);
	}
}

void vdout_sys_hal_set_hdmi(enum HDMI_VIDEO_RESOLUTION res)
{
	if (fgIsHDRes(res))
		vWriteVDOUTMsk(HDMI_CONFIG_SUB,
			       SELF_OPT_HDMI_SUB | VDOUT_CLK_HDMI_HD_SUB,
			       SELF_OPT_HDMI_SUB | VDOUT_CLK_HDMI_HD_SUB |
				       HDMI_PRGS27M_SUB); /*bit2,3*/
	else
		vWriteVDOUTMsk(HDMI_CONFIG_SUB, HDMI_PRGS27M_SUB,
			       SELF_OPT_HDMI_SUB | VDOUT_CLK_HDMI_HD_SUB |
				       HDMI_PRGS27M_SUB); /*bit8*/

	if (res == HDMI_VIDEO_1920x1080p_50Hz ||
	    res == HDMI_VIDEO_1920x1080p_60Hz)
		vWriteVDOUTMsk(HDMI_CONFIG_SUB, VDOUT_CLK_HDMI_1080P_SUB,
			       VDOUT_CLK_HDMI_1080P_SUB); /*bit5*/
	else
		vWriteVDOUTMsk(HDMI_CONFIG_SUB, 0, VDOUT_CLK_HDMI_1080P_SUB);

	/*vdout 74m*/
	if ((res == HDMI_VIDEO_1280x720p_60Hz) ||
	    (res == HDMI_VIDEO_1280x720p_50Hz) ||
	    (res == HDMI_VIDEO_1920x1080p_30Hz) ||
	    (res == HDMI_VIDEO_1920x1080p_25Hz) ||
	    (res == HDMI_VIDEO_1920x1080p_24Hz) ||
	    (res == HDMI_VIDEO_1920x1080p_23Hz) ||
	    (res == HDMI_VIDEO_1920x1080p_29Hz))
		vWriteVDOUTMsk(RW_VDOUT_CLK, HD_HALF, HD_HALF);
	else
		vWriteVDOUTMsk(RW_VDOUT_CLK, 0, HD_HALF);

	vdout_sys_hal_4k2k_clock_enable(res);
}

//for dovi idk
void vdout_sys_hal_idk_set(bool en, uint16_t htotal)
{
	if (en) {
		if (htotal == 4400)
			vWriteVDOUTMsk(0x5C8, (0x0 << 28), (0xf << 28));
		else if (htotal == 2200)
			vWriteVDOUTMsk(0x5C8, (0x2 << 28), (0xf << 28));
		else if (htotal == 1650)
			vWriteVDOUTMsk(0x5C8, (0x3 << 28), (0xf << 28));
		else if (htotal == 858)
			vWriteVDOUTMsk(0x5C8, (0x6 << 28), (0xf << 28));
		else
			vWriteVDOUTMsk(0x5C8, (0x2 << 28), (0xf << 28));//2k60hz
	}
	//vWriteMMSYSMsk(MMSYS_COMMON_CFG0, (0x5 << 16), (0xf << 16));
}

void vdout_sys_hal_videoin_source_sel(enum VIDEOIN_SRC_SEL src_point)
{
	vWriteMMSYSMsk(MMSYS_COMMON_CFG0, src_point, VIDEO_IN_SRC_SEL);
}

void vdout_sys_hal_select_dsd_clk(bool en)
{
	if (en)
		vWriteVDOUTMsk(VDTCLK_CONFIG4, VDO3_150HZ_EN, VDO3_150HZ_EN);
	else
		vWriteVDOUTMsk(VDTCLK_CONFIG4, 0, VDO3_150HZ_EN);
}

void vdout_sys_hal_fhd_hdr_gfx_fe_enable(bool enable)
{
	if (enable)
		vWriteVDOUTMsk(VDOUT_CFG_06, FHD_HDR_GFX_FE_ENABLE,
			FHD_HDR_GFX_FE_MASK);
	else
		vWriteVDOUTMsk(VDOUT_CFG_06, FHD_HDR_GFX_FE_BYPASS,
			FHD_HDR_GFX_FE_MASK);
}

void vdout_sys_hal_uhd_hdr_gfx_fe_enable(bool enable)
{
	if (enable)
		vWriteMMSYSMsk(OSD_UHD_CFG0, UHD_HDR_GFX_FE_ENABLE,
			UHD_HDR_GFX_FE_MASK);
	else
		vWriteMMSYSMsk(OSD_UHD_CFG0, UHD_HDR_GFX_FE_BYPASS,
			UHD_HDR_GFX_FE_MASK);
}

void vdout_sys_hal_4_layers_hdr_gfx2_mix_sout_select(void)
{
	vWriteVDOUTMsk(VDOUT_CFG_02, MVDO_DATA_TO_MIX,
		M_HDR2MIX_SOUT_PATH_SEL_MASK);

	vWriteVDOUTMsk(VDOUT_CFG_02, SVDO_DATA_TO_MIX,
		S_HDR2MIX_SOUT_PATH_SEL_MASK);

	vWriteVDOUTMsk(VDOUT_CFG_02, FHD_GFX_DATA_TO_MIX,
		FHD_GFX2MIX_SOUT_PATH_SEL_MASK);

	vWriteVDOUTMsk(VDOUT_CFG_02, UHD_GFX_DATA_TO_MIX,
		UHD_GFX2MIX_SOUT_PATH_SEL_MASK);

	vWriteVDOUTMsk(VDOUT_CFG_02, DISP_MIXER_OUTPUT, MIX_OUT_SEL_MASK);
}

void vdout_sys_hal_hdr_vdo_be_enable(bool enable)
{
	if (enable) {
		vWriteVDOUTMsk(VDOUT_CFG_06, OUTPUT_TO_HDR_VDO_BE,
			MMSYS_MIX_SOUT_SEL_MASK);
		vWriteVDOUTMsk(VDOUT_CFG_06, FROM_HDR_VDO_BE,
			MMSYS_MIX_HDR_BE_SEL_MASK);
	} else {
		vWriteVDOUTMsk(VDOUT_CFG_06, OUTPUT_TO_HDR_VDO_BE_FIFO,
			MMSYS_MIX_SOUT_SEL_MASK);
		vWriteVDOUTMsk(VDOUT_CFG_06, FROM_MMSYS_MIX_SOUT_SEL,
			MMSYS_MIX_HDR_BE_SEL_MASK);
	}
}

void vdout_sys_p2i_input_mux_src_select(void)
{
	vWriteMMSYSMsk(MMSYS_COMMON_CFG0, VDO_BE_FIFO_OUTPUT,
		P2I_IN_MUX_SEL_MASK);
}

void vdout_sys_vm_input_mux_src_select(bool is_interlaced)
{
	if (is_interlaced)
		vWriteMMSYSMsk(MMSYS_COMMON_CFG0, VM_IN_P2I_OUTPUT,
			VM_IN_MUX_SEL_MASK);
	else
		vWriteMMSYSMsk(MMSYS_COMMON_CFG0, 0,
			VM_IN_MUX_SEL_MASK);
}

void vdout_sys_vm_output_mux_src_select(bool is_interlaced)
{
	if (is_interlaced)
		vWriteMMSYSMsk(MMSYS_COMMON_CFG0, VM_OUT_P2I_OUTPUT,
			VM_OUT_MUX_SEL_MASK);
	else
		vWriteMMSYSMsk(MMSYS_COMMON_CFG0, 0,
			VM_OUT_MUX_SEL_MASK);
}

void vdout_sys_rgb2hdmi_input_mux_src_select(bool is_interlaced)
{
	if (is_interlaced)
		vWriteVDOUTMsk(VDOUT_CFG_02,
			RGB2HDMI_INPUT_FROM_MMSYS_MIX_SOUT,
			RGB2HDMI_INPUT_MUX_SEL_MASK);
	else
		vWriteVDOUTMsk(VDOUT_CFG_02,
			RGB2HDMI_INPUT_FROM_HDR_BE_FIFO_OUTPUT,
			RGB2HDMI_INPUT_MUX_SEL_MASK);
}

void vdout_sys_rgb2hdmi_input_timing_select(bool is_interlaced)
{
	if (is_interlaced)
		vWriteVDOUTMsk(VDOUT_CFG_02,
			RGB2HDMI_INPUT_FROM_P2I,
			RGB2HDMI_TIMING_FROM_P2I_MASK);
	else
		vWriteVDOUTMsk(VDOUT_CFG_02,
			RGB2HDMI_INPUT_FROM_FMTTER,
			RGB2HDMI_TIMING_FROM_P2I_MASK);
}

void vdout_sys_hal_clock_on_off(bool en)
{
	disp_clock_enable(DISP_CLK_VDOUT_SYS, en);
}

void vdout_sys_hal_configure_video_layer_alpha(enum DISP_PATH type,
	UINT32 value)
{
	VDOUT_LOG_D("%s, type: %d, value: 0x%x\n", __func__, type, value);
	if (type == DISP_PATH_MAIN_VDO)
		vWriteVDOUTMsk(VDOUT_CFG_00, value, MIX_IN1_ALPHA);
	if (type == DISP_PATH_SUB_VDO)
		vWriteVDOUTMsk(VDOUT_CFG_00, value << 16, MIX_IN2_ALPHA);
}

void vdout_sys_hal_disp_mix_input_chanel_swap(enum DISP_PATH type,
		enum DISP_MIX_INPUT_SRC input_src)
{
	VDOUT_LOG_D("%s, type: %d, input_src: 0x%x\n",
		__func__, type, input_src);
	switch (type) {
	case DISP_PATH_MAIN_VDO:
		vWriteVDOUTMsk(VDOUT_CFG_01, input_src << 12,
			MIX_IN1_CHANNEL_SWAP);
		break;
	case DISP_PATH_SUB_VDO:
		vWriteVDOUTMsk(VDOUT_CFG_01, input_src << 16,
			MIX_IN2_CHANNEL_SWAP);
		break;
	case DISP_PATH_OSD_FHD:
		vWriteVDOUTMsk(VDOUT_CFG_01, input_src << 20,
			MIX_IN3_CHANNEL_SWAP);
		break;
	case DISP_PATH_OSD_UHD:
		vWriteVDOUTMsk(VDOUT_CFG_01, input_src << 24,
			MIX_IN4_CHANNEL_SWAP);
		break;
	default:
		VDOUT_LOG_E("the type is wrong , type: %d\n", type);
	}
}


void vdout_sys_hal_reorder_before_mmsys_mix_out(enum BE_DATA_REORDER data_type,
	bool need_reorder)
{
	VDOUT_LOG_D("%s, data_type: 0x%x\n", __func__, data_type);
	if (need_reorder)
		vWriteVDOUTMsk(VDOUT_CFG_07, data_type << 1,
		MIX2HDR_BE_DATA_REORDER);
}

void vdout_sys_hal_set_new_sd_sel(bool en)
{
	if (en)
		vWriteVDOUTMsk(VDTCLK_CFG3, VIDEOIN_NEW_SD_SEL,
			       VIDEOIN_NEW_SD_SEL);
	else
		vWriteVDOUTMsk(VDTCLK_CFG3, 0, VIDEOIN_NEW_SD_SEL);
}

void vdout_sys_clear_irq(enum VDOUT_SYS_IRQ_BIT irq)
{
	/*set 1 and 0 to clear interrupt*/
	vWriteVDOUTMsk(VDOUT_INT_CLR, irq, irq);
	vWriteVDOUTMsk(VDOUT_INT_CLR, 0, irq);
}

void vdout_sys_clear_irq_all(void)
{
	/*set 1 and 0 to clear interrupt*/
	vWriteVDOUT(VDOUT_INT_CLR, 0xffffffff);
	vWriteVDOUT(VDOUT_INT_CLR, 0);
}


void vdout_sys_422_to_420(bool en)
{
	if (en)
		vWriteVDOUTMsk(HDMI_CONFIG_SUB, HDMI_422_TO_420,
			       HDMI_422_TO_420);
	else
		vWriteVDOUTMsk(HDMI_CONFIG_SUB, 0, HDMI_422_TO_420);
}

void vdout_sys_hal_shadow_en(bool en)
{
	VDOUT_LOG_D("vdout sys shadow_en: %d\n", en);

	vWriteVDOUTMsk(VDOUT_SYS_CONFIG_F0, en,
		VDOUT_SYS_SHADOW_EN);

	vWriteVDOUTMsk(VDOUT_SYS_CONFIG_F4,
		VDOUT_SYS_SHADOW_UPDATE, VDOUT_SYS_SHADOW_UPDATE);

	vWriteVDOUTMsk(VDOUT_SYS_CONFIG_E0,
		VDOUT_SYS_SINGLE_SHADOW_EANEBLE_E0,
		VDOUT_SYS_SINGLE_SHADOW_EANEBLE_E0);

	vWriteVDOUTMsk(VDOUT_SYS_CONFIG_E4,
		VDOUT_SYS_SINGLE_SHADOW_EANEBLE_E4,
		VDOUT_SYS_SINGLE_SHADOW_EANEBLE_E4);

}

void vdout_sys_hal_shadow_update(void)
{
	VDOUT_LOG_D("vdout sys shadow update.\n");

	vWriteVDOUTMsk(VDOUT_SYS_CONFIG_F4,
		VDOUT_SYS_SHADOW_UPDATE, VDOUT_SYS_SHADOW_UPDATE);
	vWriteVDOUTMsk(VDOUT_SYS_CONFIG_F4, 0,
		VDOUT_SYS_SHADOW_UPDATE);
}

int vdout_sys_hal_init(struct vdout_init_param *param)
{
	if (vdout.inited)
		return -1;

	vdout.inited = true;
	vdout.init_param.sys_reg_base = param->sys_reg_base;
	vdout.init_param.mmsys_reg_base = param->mmsys_reg_base;

	VDOUT_LOG_D("disp sys reg base: 0x%lx, mm sys reg base: 0x%lx\n",
		    vdout.init_param.sys_reg_base,
		    vdout.init_param.mmsys_reg_base);

	if (vdout.init_param.sys_reg_base == 0 ||
	    vdout.init_param.mmsys_reg_base == 0)
		return -1;

	return 0;
}
