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

#include <linux/vmalloc.h>
#include <linux/types.h>

#define LOG_TAG "HDR_IF"

#include "disp_vdp_if.h"
#include "mtk_disp_mgr.h"
#include "disp_hw_mgr.h"
#include "fmt_hal.h"
#include "disp_path.h"
#include "disp_dovi_common_if.h"
#include "disp_dovi_main.h"
#include "dovi_be_hal.h"
#include "dovi_gfx_fe_hal.h"
#include "dovi_vdo_fe_hal.h"
#include "disp_sys_hal.h"
#include "disp_hdr_main.h"
#include "disp_dovi_vdo_fe_if.h"
#include "disp_dovi_gfx_fe_if.h"
#include "disp_dovi_be_if.h"
#include "disp_dovi_io.h"
#include "disp_adl_if.h"
#include "disp_info.h"
#include "disp_dovi_tz_client.h"
#include "disp_cfd_main.h"
#include "disp_hdr_if.h"
#include "disp_ml_if.h"
#include "disp_hdr_cmd.h"
#include "internal_hdmi_drv.h"

struct video_buffer_info hdr_video_layer[V_G_LAYER_MAX];
struct mtk_disp_buffer hdr_osd_layer[V_G_LAYER_MAX];
struct disp_hw_common_info dovi_common_info;

enum hdr_output_type hdr_output_signal_type;
bool bsub_exist;
enum HDR_PATH hdr_path_select = DEFAULT_PATH;
uint32_t ui_force_hdr_type;
uint32_t hdr_vsync_cnt;
uint32_t time_check;
uint32_t line_cnt[15];
uint32_t disp_hdr_irq_event;
uint32_t adl_mode;

/*record hdr 4layer enable or disable*/
bool hdr_fe_en[LAYER_MAX];
bool hdr_trig_algn_mvid[LAYER_MAX];
bool hdr_vdo_be_en;
uint32_t hdr_input_width[LAYER_MAX];
uint32_t hdr_input_height[LAYER_MAX];
uint32_t hdr_sof_start;
uint32_t hdr_sof_end;

/*delay hdr change*/
bool delay_hdr_chg;
uint32_t delay_hdr_cur_vsync;
uint32_t delay_hdr_num = 7;
uint32_t delay_hdr_mute_num = 7;


#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
void hdr_set_HDMI_BT2020_signal(bool enable_bt2020)
{
	static int bt2020_enabled = -1;
	if (enable_bt2020 && disp_common_info.tv.is_support_bt2020) {
		/* enable bt2020 */
		if (!bt2020_enabled || bt2020_enabled == -1) {
			hdr_printf("enable BT2020 signal\n");
			vBT2020Enable(true);
		}
		bt2020_enabled = true;
	} else {
		/* disable bt2020 */
		if (bt2020_enabled || bt2020_enabled == -1) {
			hdr_printf("disable BT2020 signal\n");
			vBT2020Enable(false);
		}
		bt2020_enabled = false;
	}
}
#endif

bool hdr10_plus_get_frame_delay_flag(void)
{
	if (dovi_common_info.tv.is_support_hdr10_plus &&
		disp_cfd_drv_get_frame_async(0))
		return true;
	else
		return false;
}

void hdr10_plus_set_graphic_overlay_flag(uint32_t id,
	bool change_graphic_overlay_flag)
{
	disp_cfd_drv_set_user_gfx_overlay(id, change_graphic_overlay_flag);
}

void set_hdr_path_black_unblack(uint32_t id, bool black)
{

}

void get_hdr_path_info(void)
{

}

void set_hdr_path_info(void)
{

}

void disp_hdr_vsync_handle(uint32_t i, uint32_t vsync)
{

	if (i != 0)
		return;
	hdr_vsync_cnt = vsync;

	if (delay_hdr_chg &&
		(delay_hdr_cur_vsync + delay_hdr_num) <= hdr_vsync_cnt) {
		delay_hdr_chg = false;
		disp_hdr_config_hdmi_signal_delay(hdr_path_select);
	}
	if (dovi_idk_dump && (i == 0)) {
	/* just for dolby idk. Guarantee osd not to cover
	 *video even if video is full screen
	 */
#if 0
		_vdp_cli_debug_set_disp_area(params_count,
			vdp_show);
#endif
		if (idk_dump_vsync_cnt >= 0)
			idk_dump_vsync_cnt++;

		if (idk_dump_vsync_cnt == 2) {
			if (!dovi_idk_dump_set_vin) {
			#if 0 // 8696 review
				disp_dovi_idk_dump_vin(
					true,
					VIDEOIN_SRC_SEL_DOLBY3,
					VDEOIN_FORMAT_444);
			#endif
				dovi_idk_dump_set_vin = true;
			} //else 8696 review
				//videoin_hal_enable(true);
		}
		if (idk_dump_vsync_cnt == 15)
			// 8696 review
			; //videoin_hal_enable(false);
		if (idk_dump_vsync_cnt == 20)
			disp_dovi_idk_dump_frame();
	}

	if (dovi_idk_dump)
		idk_dump_vsync_cnt = 0;
}

void disp_hdr_set_hdmi_signal(uint32_t hdr_type, bool enable,
	bool bt2020_enable, struct VID_PLA_HDR_METADATA_INFO_T rHdr)
{
	struct disp_hw_tv_capbility *tv_cap = &dovi_common_info.tv;

	switch (hdr_type) {
	case HDR_OUT_TYPE_HDR10:
		if (enable) {
			vSetStaticHdrType(GAMMA_ST2084);
			vVdpSetHdrMetadata(true, rHdr);
		}
		vHdrEnable(enable);
		hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	case HDR_OUT_TYPE_HLG:
		if (enable) {
			vSetStaticHdrType(GAMMA_HLG);
			vVdpSetHdrMetadata(true, rHdr);
		}
		vHdrEnable(enable);
		hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	case HDR_OUT_TYPE_DV_STD:
		vDolbyHdrEnable(enable);
		break;
	case HDR_OUT_TYPE_DV_LL:
		vLowLatencyDolbyVisionEnable(enable);
		hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	case HDR_OUT_TYPE_HDR10PLUS:
		if (enable) {
			vSetStaticHdrType(GAMMA_ST2084);
			vVdpSetHdrMetadata(true, rHdr);
		}
		if (tv_cap->hdr10_plus_app_ver == 0xFF)
			vHdr10PlusEnable(enable);
		hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	case HDR_OUT_TYPE_HDR10PLUS_VSIF:
		if (enable) {
			vSetStaticHdrType(GAMMA_ST2084);
			vVdpSetHdrMetadata(true, rHdr);
		}
		if (tv_cap->hdr10_plus_app_ver != 0xFF)
			vHdr10PlusVSIFEnable(enable,
			ui_force_hdr_type, dolby_out_format);
		hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	case HDR_OUT_TYPE_SDR_2020:
		hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	case HDR_OUT_TYPE_SDR:
		hdr_printf("sdr type\n");
		hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	default:
		hdr_printf("%s error\n", __func__);
		break;
	}
}
void disp_hdr_config_hdmi_signal_delay(uint32_t path)
{
	enum hdr_output_type out_format = 0;
	bool bt2020_enable = 0;
	struct VID_PLA_HDR_METADATA_INFO_T rHdr = { 0 };

	if (path == DOVI_PATH) {
		dovi_get_hdmi_output_format(&out_format, &bt2020_enable);
		dovi_get_hdr10_metadata(&rHdr, 0);
	} else {
		disp_cfd_drv_get_hdmi_output_format(0,
			&out_format, &bt2020_enable);
		disp_cfd_drv_get_hdr10_metadata(&rHdr, 0);
	}
	if (hdr_output_signal_type != out_format)
		hdr_printf("delay and final not match %d %d\n",
		hdr_output_signal_type,
		out_format);
	hdr_output_signal_type = out_format;

	disp_hdr_set_hdmi_signal(out_format, true,
			bt2020_enable, rHdr);

	// update dynamic md
	if (out_format == HDR_OUT_TYPE_HDR10PLUS_VSIF) {
		disp_cfd_drv_fill_dyn_metadata(VID_PLA_DR_TYPE_HDR10_PLUS_VSIF,
			&rHdr);
		vVdpSetHdrMetadata(true, rHdr);
	} else if (out_format == HDR_OUT_TYPE_HDR10PLUS) {
		disp_cfd_drv_fill_dyn_metadata(VID_PLA_DR_TYPE_HDR10_PLUS,
			&rHdr);
		vVdpSetHdrMetadata(true, rHdr);
	}
	disp_mix_hal_set_black_pattern(false);
	hdr_printf("end delay set hdmi type %d %d %d %d\n", out_format,
			hdr_path_select, delay_hdr_cur_vsync, hdr_vsync_cnt);

}

void disp_hdr_config_hdmi_signal(uint32_t path)
{
	enum hdr_output_type out_format = 0;
	bool bt2020_enable = 0;
	struct VID_PLA_HDR_METADATA_INFO_T rHdr = { 0 };
	bool bneeddelay = false;

	if (path == DOVI_PATH) {
		dovi_get_hdmi_output_format(&out_format, &bt2020_enable);
		dovi_get_hdr10_metadata(&rHdr, 0);
	} else {
		disp_cfd_drv_get_hdmi_output_format(0,
			&out_format, &bt2020_enable);
		disp_cfd_drv_get_hdr10_metadata(&rHdr, 0);
	}

	if (hdr_output_signal_type != out_format) {
		/*hdr10<->hlg, hlg<->hdr10+, change need delay*/
		if (((hdr_output_signal_type == HDR_OUT_TYPE_HDR10) &&
			(out_format == HDR_OUT_TYPE_HLG)) ||
			((hdr_output_signal_type == HDR_OUT_TYPE_HLG) &&
			(out_format == HDR_OUT_TYPE_HDR10)) ||
			(((hdr_output_signal_type == HDR_OUT_TYPE_HDR10PLUS_VSIF)
			|| (hdr_output_signal_type == HDR_OUT_TYPE_HDR10PLUS)) &&
			(out_format == HDR_OUT_TYPE_HLG)) ||
			((hdr_output_signal_type == HDR_OUT_TYPE_HLG) &&
			((out_format == HDR_OUT_TYPE_HDR10PLUS_VSIF) ||
			(out_format == HDR_OUT_TYPE_HDR10PLUS))))
			bneeddelay = true;

		if (bneeddelay) {
			/*display mute frame*/
			dovi_black_pattern_en = true;
			dovi_black_pattern_cnt = 0;
			dovi_black_pattern_cnt_max = delay_hdr_mute_num;
			if (dovi_black_pattern_cnt_max > 0)
				disp_mix_hal_set_black_pattern(true);

			/*disable pre hdr first*/
			disp_hdr_set_hdmi_signal(hdr_output_signal_type, false,
				false, rHdr);

			hdr_output_signal_type = out_format;
			delay_hdr_cur_vsync = hdr_vsync_cnt;
			delay_hdr_chg = true;

			hdr_printf("start delay set hdmi %d %d\n",
				delay_hdr_cur_vsync, hdr_output_signal_type);
			return;
		} else {
			if (delay_hdr_chg) {
				delay_hdr_chg = false;
				disp_mix_hal_set_black_pattern(false);
				hdr_printf("cancel delay %d\n", hdr_vsync_cnt);
			}
			disp_hdr_set_hdmi_signal(hdr_output_signal_type, false,
					false, rHdr);
		}
		hdr_output_signal_type = out_format;
		disp_hdr_set_hdmi_signal(out_format, true,
			bt2020_enable, rHdr);

		hdr_printf("set hdmi type %d %d\n",
			hdr_output_signal_type, hdr_vsync_cnt);
	}

	if (!delay_hdr_chg) {
		// update static md
		if ((out_format == HDR_OUT_TYPE_HDR10) ||
			(out_format == HDR_OUT_TYPE_HLG) ||
			(out_format == HDR_OUT_TYPE_HDR10PLUS) ||
			(out_format == HDR_OUT_TYPE_HDR10PLUS_VSIF))
			vVdpSetHdrMetadata(true, rHdr);

		// update dynamic md
		if (out_format == HDR_OUT_TYPE_HDR10PLUS_VSIF) {
			disp_cfd_drv_fill_dyn_metadata(VID_PLA_DR_TYPE_HDR10_PLUS_VSIF,
				&rHdr);
			vVdpSetHdrMetadata(true, rHdr);
		} else if (out_format == HDR_OUT_TYPE_HDR10PLUS) {
			disp_cfd_drv_fill_dyn_metadata(VID_PLA_DR_TYPE_HDR10_PLUS,
				&rHdr);
			vVdpSetHdrMetadata(true, rHdr);
		}
	}

}

void disp_hdr_config_video_non(void)
{
	disp_hdr_set_event(HDR_EVENT_VLAYER0_CFG_DONE);
	if ((disp_hdr_event & HDR_EVENT_VLAYER0_CFG_DONE)
		&& (disp_hdr_event & HDR_EVENT_VLAYER1_CFG_DONE)) {
		disp_hdr_wakeup_routine(0);
		hdr_info("special case olny pip with main running\n");
	}
}

int disp_hdr_config_video_info(struct video_buffer_info *buf,
	bool sub_exit)
{
	//uint32_t width = 0;

	if (buf == NULL || buf->layer_id >= V_G_LAYER_MAX) {
		hdr_printf("error video buffer\n");
		return -1;
	}

	if (time_check)
		line_cnt[0] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	hdr_info("video[%d][%d] %d %d %d pts = %lld\n", buf->layer_id,
		hdr_vsync_cnt, buf->hdr_info.dr_range, buf->hdr_info.enable,
		sub_exit, buf->pts);

	if (buf->layer_id == LAYER0) {
		memcpy(&hdr_video_layer[LAYER0], buf, sizeof(*buf));
		bsub_exist = sub_exit;
		if (sub_exit == 0) {
			disp_hdr_set_event(HDR_EVENT_VLAYER0_CFG_DONE);
			disp_hdr_set_event(HDR_EVENT_VLAYER1_CFG_DONE);
		} else
			disp_hdr_set_event(HDR_EVENT_VLAYER0_CFG_DONE);
	} else {
		memcpy(&hdr_video_layer[LAYER1], buf, sizeof(*buf));
		bsub_exist = sub_exit;
		disp_hdr_set_event(HDR_EVENT_VLAYER1_CFG_DONE);
	}

	if ((disp_hdr_event & HDR_EVENT_VLAYER0_CFG_DONE)
		&& (disp_hdr_event & HDR_EVENT_VLAYER1_CFG_DONE))
		disp_hdr_wakeup_routine(0);

	if ((!dolby_path_enable) && (buf->layer_id == LAYER0)) {
		if (hdr_fe_en[LAYER2] && hdr_trig_algn_mvid[LAYER2])
			disp_hdr_wakeup_routine(LAYER2);
		if (hdr_fe_en[LAYER3] && hdr_trig_algn_mvid[LAYER3])
			disp_hdr_wakeup_routine(LAYER3);
	}

	return 0;

}

int disp_hdr_config_osd_info(struct mtk_disp_buffer *buf)
{
	uint32_t thread_id = 0;

	if (buf == NULL || buf->layer_id >= V_G_LAYER_MAX) {
		hdr_printf("error osd buffer\n");
		return -1;
	}

	if (hdr_trig_algn_mvid[buf->layer_id + V_G_LAYER_MAX])
		hdr_trig_algn_mvid[buf->layer_id + V_G_LAYER_MAX] = false;

	memcpy(&hdr_osd_layer[buf->layer_id], buf, sizeof(*buf));
	hdr_info("osd[%d][%d][%d] config %d\n", buf->layer_id,
		hdr_vsync_cnt, befifo_irq_cnt, dolby_path_enable);
	if (!dolby_path_enable) {
		thread_id = buf->layer_id + V_G_LAYER_MAX;
		disp_hdr_wakeup_routine(thread_id);
	}
	return 0;
}

int disp_hdr_config_osd_info_fake(uint32_t layer_id)
{
	if (layer_id >= V_G_LAYER_MAX) {
		hdr_printf("error fake config %d\n", layer_id);
		return -1;
	}

	hdr_info("osd[%d][%d][%d] config fake %d\n", layer_id,
		hdr_vsync_cnt, befifo_irq_cnt, dolby_path_enable);
	hdr_trig_algn_mvid[layer_id + V_G_LAYER_MAX] = true;
	return 0;
}

int disp_hdr_handle_forcehdr(enum DISP_CMD cmd, void *data)
{
	uint32_t default_path = OPENHDR_PATH;
	uint32_t line_count[3] = { 0 };

	if (data == NULL) {
		hdr_printf("%s error param\n", __func__);
		return -1;
	}

	if (time_check)
		line_count[0] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	if (!tv_info_set_by_cmd)
		disp_hw_mgr_get_info(&dovi_common_info);

	ui_force_hdr_type = *((uint32_t *) data);

	/*fhd hdr always enable
	 * if uiforce && dolby efuse existed,
	 * vdo be enable
	 */
	if (g_dovi_efuse) {
		if ((ui_force_hdr_type == 2) &&
			(hdr_path_select == OPENHDR_PATH) &&
			(vdp_start_st[0] == 1) &&
			((hdr_video_layer[0].hdr_info.dr_range == DISP_DR_TYPE_HLG) ||
			((hdr_video_layer[0].hdr10_type == HDR10_TYPE_PLUS) &&
			(dovi_common_info.tv.is_support_hdr10_plus)))) {
			hdr_printf("[VS10] special case do nothing\n");
			return 0;
		}
		disp_hdr_vdo_be_start_stop((bool)ui_force_hdr_type);
		if (ui_force_hdr_type) {
			default_path = DOVI_PATH;
			hdr_path_select = default_path;
		}
		else {
			disp_ml_set_disable(0);
			if (vdp_start_st[0] == 1 || (vdp_start_st[1] == 1))
				disp_ml_set_disable(1);
		}
		disp_dovi_process_cmd(LAYER0, cmd, data);
	}

	hdr_path_select = default_path;
	if (time_check)
		line_count[1] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	if (default_path == OPENHDR_PATH) {
		//set_fs_index(0);
		disp_cfd_set_cmd(LAYER0, cmd, data);
		if ((vdp_start_st[0] == 1) &&
			(hdr_video_layer[0].is_dolby == false)) {
			disp_hdr_set_event(HDR_EVENT_VLAYER0_CFG_DONE);
			disp_hdr_set_event(HDR_EVENT_VLAYER1_CFG_DONE);
			disp_hdr_wakeup_routine(0);
		} else if ((vdp_start_st[1] == 1) &&
			(hdr_video_layer[1].is_dolby == false))
			disp_hdr_wakeup_routine(1);
		else {
			if (hdr_fe_en[LAYER2]) {
				disp_cfd_set_conf_mode(2,
					CFD_REG_CONF_CLIENT_ML);
				disp_cfd_drv_set_bypass(2);
				disp_cfd_drv_set_r2y(2, 0);
				disp_cfd_set_conf_mode(2,
					CFD_REG_CONF_CLIENT_RIU);
			}
			if (hdr_fe_en[LAYER3]) {
				disp_cfd_set_conf_mode(3,
					CFD_REG_CONF_CLIENT_ML);
				disp_cfd_drv_set_bypass(3);
				disp_cfd_drv_set_r2y(3, 0);
				disp_cfd_set_conf_mode(3,
					CFD_REG_CONF_CLIENT_RIU);
			}
		}
	}

	disp_hdr_config_hdmi_signal(default_path);
	if (time_check)
		line_count[2] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	hdr_printf("ui force hdr type:%d path %d video_st[%d %d],%d, %d %d %d\n",
		ui_force_hdr_type, hdr_path_select,vdp_start_st[0],
		vdp_start_st[1], befifo_irq_cnt,
		line_count[0], line_count[1], line_count[2]);

	return 0;
}


int disp_hdr_set_vdo_be_clk(bool en)
{
	disp_clock_enable(DISP_CLK_HDR_VDO_BE, en);
	disp_clock_smi_larb_en(DISP_SMI_LARB4, en);
	return 0;
}

int disp_hdr_set_vdo_be_path(bool en)
{
	enum DISP_PATH_HW_ID hw_id = 0;
	enum FMT_SOF_TYPE hw_sof_start = 0;
	enum FMT_SOF_TYPE hw_sof_end = 0;
	int sof_start = 0x00010002;
	int sof_end = 0x00010033;

	hw_id = DISP_PATH_DISP_HDR_VDO_BE;
	hw_sof_start = FMT_SOF_21_DOLBY_BE_STA;
	hw_sof_end = FMT_SOF_21_DOLBY_BE_END;

	if (en) {
		disp_path_set_hw_path(hw_id, true);
		fmt_hal_set_sof(hw_sof_start, hw_sof_end, sof_start, sof_end);
	} else
		disp_path_set_hw_path(hw_id, 0);

	return 0;

}

int disp_hdr_vdo_be_start_stop(bool en)
{
	if (en != hdr_vdo_be_en) {
		if (en) {
			disp_hdr_set_vdo_be_clk(en);
			disp_hdr_set_vdo_be_path(en);
			disp_hdr_irq_event &= ~((1 << CLK_CHG_BE) & 0xff);
		} else {
			disp_hdr_set_vdo_be_path(en);
			disp_hdr_irq_event |= 1 << CLK_CHG_BE;
			//disp_hdr_set_vdo_be_clk(en);
		}
		hdr_vdo_be_en = en;
		hdr_printf("vdobe start/stop %d %d\n", en, befifo_irq_cnt);
	}
	return 0;

}

int disp_hdr_fe_set_path(uint32_t layer_id, bool en)
{
	enum DISP_PATH_HW_ID hw_id = 0;
	enum FMT_SOF_TYPE hw_sof_start = 0;
	enum FMT_SOF_TYPE hw_sof_end = 0;
	int sof_start = 0x00010002;
	int sof_end = 0x00010033;

	if ((hdr_sof_start != 0) && (hdr_sof_end != 0)) {
		sof_start = hdr_sof_start;
		sof_end = hdr_sof_end;
	}

	if (layer_id >= LAYER_MAX) {
		hdr_printf("%s layer_id error\n", __func__);
		return -1;
	}

	switch (layer_id) {
	case LAYER0:
		hw_id = DISP_PATH_M_HDR_VDO_FE;
		hw_sof_start = FMT_SOF_6_M_DOLBY_FE_STA;
		hw_sof_end = FMT_SOF_6_M_DOLBY_FE_END;
		break;
	case LAYER1:
		hw_id = DISP_PATH_S_HDR_VDO_FE;
		hw_sof_start = FMT_SOF_13_S_DOLBY_FE_STA;
		hw_sof_end = FMT_SOF_13_S_DOLBY_FE_END;
		break;
	case LAYER2:
		hw_id = DISP_PATH_FHD_HDR_GFX_FE;
		hw_sof_start = FMT_SOF_19_FHD_DOLBY_FE_STA;
		hw_sof_end = FMT_SOF_19_FHD_DOLBY_FE_END;
		break;
	case LAYER3:
		hw_id = DISP_PATH_UHD_HDR_GFX_FE;
		hw_sof_start = FMT_SOF_16_UHD_DOLBY_FE_STA;
		hw_sof_end = FMT_SOF_16_UHD_DOLBY_FE_END;
		break;
	default:
		break;
	}

	if (en) {
		disp_path_set_hw_path(hw_id, true);
		fmt_hal_set_sof(hw_sof_start, hw_sof_end, sof_start, sof_end);
	} else
		disp_path_set_hw_path(hw_id, false);

	return 0;

}

int disp_hdr_fe_set_clk(uint32_t layer_id, bool en)
{
	switch (layer_id) {
	case LAYER0:
		disp_clock_enable(DISP_CLK_M_HDR_VDO_FE, en);
		disp_clock_smi_larb_en(DISP_SMI_LARB5, en);
		break;
	case LAYER1:
		disp_clock_enable(DISP_CLK_S_HDR_VDO_FE, en);
		disp_clock_smi_larb_en(DISP_SMI_LARB6, en);
		break;
	case LAYER2:
		disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE, en);
		disp_clock_smi_larb_en(DISP_SMI_LARB4, en);
		break;
	case LAYER3:
		disp_clock_enable(DISP_CLK_UHD_HDR_GFX_FE, en);
		disp_clock_smi_larb_en(DISP_SMI_LARB0, en);
		break;
	default:
		break;
	}

	return 0;
}
int disp_hdr_fe_start_stop(uint32_t layer_id, bool en)
{
	if (en != hdr_fe_en[layer_id]) {
		if (en) {
			disp_hdr_fe_set_clk(layer_id, en);
			disp_hdr_fe_set_path(layer_id, en);
			//disp_hdr_irq_event &= ~((1 << layer_id) & 0xff);
			if (layer_id < LAYER2)
				disp_dovi_set_adldelay(layer_id);
		} else {
			disp_hdr_fe_set_path(layer_id, en);
			//disp_hdr_irq_event |= 1 << layer_id;
			disp_hdr_fe_set_clk(layer_id, en);
			hdr_input_width[layer_id] = hdr_res_width;
			hdr_input_height[layer_id] = hdr_res_height;
		}
		hdr_fe_en[layer_id] = en;
		hdr_printf("hdrfe[%d] start/stop %d\n", layer_id, en);
	}
	return 0;
}
int disp_hdr_handle_osd_start(enum DISP_CMD cmd, void *data)
{
	uint32_t layer_id = 0;

	if (data == NULL) {
		hdr_printf("%s err param\n", __func__);
		return -1;
	}
	layer_id = *((uint32_t *)data);
	if (layer_id >= V_G_LAYER_MAX) {
		hdr_printf("%s idx err\n", __func__);
		return -1;
	}

	disp_hdr_fe_start_stop(layer_id + 2, true);
	disp_cfd_enable(layer_id + 2, true);
	if (g_dovi_efuse)
		disp_dovi_process_cmd(layer_id, cmd, data);

	return 0;
}
int disp_hdr_handle_osd_stop(enum DISP_CMD cmd, void *data)
{
	uint32_t layer_id = 0;

	if (data == NULL) {
		hdr_printf("%s err param\n", __func__);
		return -1;
	}
	layer_id = *((uint32_t *)data);
	if (layer_id >= V_G_LAYER_MAX) {
		hdr_printf("%s idx err\n", __func__);
		return -1;
	}
	if (g_dovi_efuse)
		disp_dovi_process_cmd(layer_id, cmd, data);
	//hdr_core_handle_other_moudule_call(cmd, data);
	disp_cfd_enable(layer_id + 2, false);
	disp_hdr_fe_start_stop(layer_id + 2, false);
	if (hdr_trig_algn_mvid[layer_id + V_G_LAYER_MAX])
		hdr_trig_algn_mvid[layer_id + V_G_LAYER_MAX] = false;

	return 0;
}


int disp_hdr_handle_vdp_start(enum DISP_CMD cmd, void *data)
{
	uint32_t layer_id = 0;

	if (data == NULL) {
		hdr_printf("%s err param\n", __func__);
		return -1;
	}
	layer_id = *((uint32_t *)data);
	if (layer_id > V_G_LAYER_MAX) {
		hdr_printf("%s idx err\n", __func__);
		return -1;
	}

	dolby_path_ready2start = 1;

	disp_hdr_fe_start_stop(layer_id, true);

	disp_adl_clock_on_off(DISPSYS_ADL, true);

	if (layer_id < V_G_LAYER_MAX)
		vdp_start_st[layer_id] = 1;

	return 0;
}

void disp_hdr_stop_handle(uint32_t layer_id)
{
	//struct disp_hw *hdr_drv = disp_hdr_get_drv();
	enum HDR_PATH current_path = OPENHDR_PATH;
	enum HDR_PATH dst_path = OPENHDR_PATH;
	enum DISP_DR_TYPE_T dovi_input_dr_type = DISP_DR_TYPE_SDR;
	bool real_stop = true;

	/* disable layer and cfd anyway */
	if (layer_id < V_G_LAYER_MAX) {
		vdp_start_st[layer_id] = 0;
		dovi_vdo_fe_hal_set_enable(layer_id, false);
	}
	disp_cfd_enable(layer_id, false);

	if (layer_id == LAYER1) {
		bsub_exist = false;
		dolby_path_ready2start = 1;
	}

	if (vdp_start_st[LAYER0] || vdp_start_st[LAYER1])
		real_stop = false;

	if (!real_stop) {
		hdr_printf("layer[%d] stop, but not real_stop\n", layer_id);
		disp_hdr_fe_start_stop(layer_id, false);
		return;
	}

	/* only real stop need change path */
	if (dolby_path_enable) {
		current_path = DOVI_PATH;
		if (current_path != hdr_path_select)
			hdr_printf("path not match when stop\n");
	}

	if (ui_force_hdr_type && g_dovi_efuse)
		dst_path = DOVI_PATH;
	else
		dst_path = OPENHDR_PATH;

	/* save dst hdr path*/
	hdr_path_select = dst_path;

	hdr_printf("layer[%d][%d]stop path %d %d %d %d\n", layer_id,
		hdr_vsync_cnt, befifo_irq_cnt,
		current_path, dst_path, g_dovi_efuse);

	/* disable dsys ml frist avoid error setting */
	disp_ml_set_disable(ML_DSYS_IP);

	if (dst_path == DOVI_PATH) {
		// reset cfd graphic status first here
		if (hdr_fe_en[LAYER2] && (current_path == OPENHDR_PATH))
			disp_cfd_drv_set_feature_type(LAYER2, DISP_CFD_BYPASS);
		if (hdr_fe_en[LAYER3] && (current_path == OPENHDR_PATH))
			disp_cfd_drv_set_feature_type(LAYER3, DISP_CFD_BYPASS);

		if (dolby_path_enable == 1) {
			/* change dv input type to sdr */
			dovi_hdr_md_info[0].enable =
			DOVI_INOUT_FORMAT_CHANGE;
			dovi_hdr_md_info[0].dr_range = DISP_DR_TYPE_SDR;
			dovi_update_output_setting(&dovi_common_info,
			NULL, NULL);
			dovi_get_input_format(&dovi_input_dr_type);

			disp_dovi_process_cmd(LAYER0, DISP_CMD_METADATA_UPDATE,
				&dovi_hdr_md_info[0]);
			dovi_vs10_path_en = ui_force_hdr_type;
		} else {
			/* call openhdr stop if need */
			disp_hdr_vdo_be_start_stop(true);
			dovi_path_enable();
		}
	} else {

		/*stop from dolby*/
		if (dolby_path_enable) {
			dovi_black_pattern_en = true;
			dovi_black_pattern_cnt = 0;
			dovi_black_pattern_cnt_max = 5;
			if (dovi_black_en_bycmd)
				dovi_black_pattern_cnt_max = dovi_black_cnt_bycmd;
			if (dovi_black_pattern_cnt_max > 0)
				disp_mix_hal_set_black_pattern(true);
		}

		/* disable msys ml because ml will still update dvgfx setting*/
		disp_ml_set_disable(ML_MSYS_IP);
		disp_hdr_vdo_be_start_stop(false);
		//set vdo hdr fe and be internabypss
		disp_dovi_set_internalbyass(0);
		dovi_config_fefifo_swap(false);

		if (dolby_path_enable) {
			dovi_hdr_md_info[0].dr_range =
				DISP_DR_TYPE_PHLP_RESVERD;
			dovi_hdr_md_info[0].enable = 0;

			disp_dovi_process_cmd(LAYER0, DISP_CMD_METADATA_UPDATE,
				&dovi_hdr_md_info[0]);
			dolby_path_enable = 0;
			dovi_vs10_path_en = 0;
		}
	}

	disp_hdr_fe_start_stop(layer_id, false);
	/*video stop ,video adl client should disable*/
	disp_adl_cfg_client_en(DV_ADL_V_MAIN, 0, 0);
	disp_adl_cfg_client_en(DV_ADL_V_SUB, 0, 0);
	disp_adl_clock_on_off(DISPSYS_ADL, false);
	disp_hdr_config_hdmi_signal(dst_path);
	hdr_printf("stop done[%d][%d]\n", hdr_vsync_cnt, befifo_irq_cnt);
}

int disp_hdr_handle_vdp_stop(enum DISP_CMD cmd, void *data)
{
	uint32_t layer_id = 0;

	if (data == NULL) {
		hdr_printf("%s err param\n", __func__);
		return -1;
	}
	layer_id = *((uint32_t *)data);
	if (layer_id >= V_G_LAYER_MAX) {
		hdr_printf("%s idx err\n", __func__);
		return -1;
	}

	hdr_printf("vdp[%d] stop\n", layer_id);
	disp_hdr_stop_handle(layer_id);
	return 0;

}
int disp_hdr_path_judge(void)
{
	struct video_buffer_info *buf_main = NULL;
	struct video_buffer_info *buf_sub = NULL;
	struct disp_hw_tv_capbility *tv_cap = NULL;
	enum HDR_PATH hdr_path = DEFAULT_PATH;
	enum dovi_signal_format_t def_out_format = DOVI_FORMAT_SDR;

	mutex_lock(&disp_hdr_mutex);
	buf_main = &hdr_video_layer[LAYER0];
	if (bsub_exist)
		buf_sub = &hdr_video_layer[LAYER1];
	if (time_check)
		line_cnt[1] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	hdr_info("judge[%d] %d %d %d %d %d %d pts = %lld\n",
		hdr_vsync_cnt,
		bsub_exist, ui_force_hdr_type,
		g_dovi_efuse, buf_main->hdr10_type,
		buf_main->is_dolby,
		buf_main->hdr_info.dr_range,
		buf_main->pts);

	if (!tv_info_set_by_cmd)
		disp_hw_mgr_get_info(&dovi_common_info);
	tv_cap = &(dovi_common_info.tv);

	if (ui_force_hdr_type) {
		if (g_dovi_efuse) {
			def_out_format = dovi_judge_out_format(tv_cap,
				dovi_common_info.resolution);
			if ((((buf_main->hdr10_type == HDR10_TYPE_PLUS) &&
			(!bsub_exist))
			&& (tv_cap->is_support_hdr10_plus))
			|| (buf_main->hdr_info.dr_range == DISP_DR_TYPE_HLG)
			|| ((buf_main->allm_en)
			&& (buf_main->hdr_info.dr_range != DISP_DR_TYPE_DOVI)
			&& (low_latency_io_mode == HDMI_LOW_LATENCY_MODE_AUTO)
			&& ((def_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY)
			|| (def_out_format == DOVI_FORMAT_DOVI))))
				hdr_path = OPENHDR_PATH;
			else
				hdr_path = DOVI_PATH;
		} else
			hdr_path = OPENHDR_PATH;
	} else {
		if (g_dovi_efuse) {
			if ((buf_main->is_dolby) ||
				(bsub_exist && (buf_sub->is_dolby)))
				hdr_path = DOVI_PATH;
			else
				hdr_path = OPENHDR_PATH;
		} else
			hdr_path = OPENHDR_PATH;
	}

	if (hdr_path == DOVI_PATH) {
		//if (buf_main->is_dolby)
			//set_fs_index(1);
		//else
			//set_fs_index(0);
		if (hdr_path_select != hdr_path) {
			/* disable openhdr path */
			hdr_printf("current path is %d\n", hdr_path);
			hdr_path_select = hdr_path;
		}

		if (time_check)
			line_cnt[2] =
			HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

		dovi_frame_commit(buf_main, buf_sub, bsub_exist);

		/* if dovi path ,vdo be clk and path enable */
		if (hdr_path == DOVI_PATH)
			disp_hdr_vdo_be_start_stop(true);

		if (time_check)
			line_cnt[9] =
			HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;
	} else {
		if (hdr_path_select != hdr_path) {
			hdr_path_select = hdr_path;
			dovi_path_disable();
			disp_ml_set_disable(ML_MSYS_IP);
			disp_ml_set_disable(ML_DSYS_IP);
			disp_hdr_vdo_be_start_stop(false);
			disp_dovi_set_internalbyass(2);
			dovi_config_fefifo_swap(false);
			//set_fs_index(0);
			hdr_printf("current path is %d\n", hdr_path);
		}
		/*wakeup thread1 to process sub video by mtkhdr */
		if (bsub_exist)
			disp_hdr_wakeup_routine(1);
		// cfd config frame
		disp_cfd_config_video_frame(0, buf_main, tv_cap);
	}
	disp_hdr_config_hdmi_signal(hdr_path);
	if (time_check) {
		line_cnt[10] = Dv_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

		hdr_printf("hdr[%lld] %d %d %d %d %d %d %d %d %d %d %d\n",
			buf_main->pts,
			line_cnt[0], line_cnt[1], line_cnt[2],
			line_cnt[3], line_cnt[4], line_cnt[5],
			line_cnt[6], line_cnt[7], line_cnt[8],
			line_cnt[9], line_cnt[10]);
	}

	mutex_unlock(&disp_hdr_mutex);
	return 0;
}

int disp_dovi_hdr_save_sec_rpu(struct mtk_disp_dovi_md_t *dolby_info,
	struct mtk_vdp_dovi_md_t *dovi_md_info)
{
	if (dolby_info == NULL || dovi_md_info == NULL) {
		hdr_printf("%s params err\n", __func__);
		return -1;
	}
	/* store rpu data into tz buffers,
	 * not transmit to dovi process immediately
	 */
	dovi_sec_find_rpu_buffer(dolby_info->sec_handle, dolby_info->len);

	/* get rpu handle from tz buffers */
	dovi_md_info->sec_handle = dovi_share_mem->sec_handle_out;
	dovi_md_info->len = dolby_info->len;

	return 0;
}

int disp_hdr_backup_hdr10plus_sec_handle(
	struct mtk_vdp_hdr10_plus_svp_handle_t *sec_info)
{
	int ret = 0;

	ret = disp_cfd_backup_sec_handle(sec_info);
	if (ret != 0)
		hdr_printf("back hdr10plus sec handle err\n");
	return ret;
}

struct video_buffer_info *disp_hdr_get_vid_info(uint32_t id)
{
	return &(hdr_video_layer[id]);
}

struct mtk_disp_buffer *disp_hdr_get_gfx_info(uint32_t id)
{
	return &(hdr_osd_layer[id]);
}

void disp_set_hdr_fe_input_size(uint32_t layer_id,
	uint32_t u4width, uint32_t u4height)
{
	uint32_t u4cur_dm_width = 0;
	uint32_t u4cur_dm_height = 0;

	disp_cfd_drv_get_video_dm_wh(layer_id,
		&u4cur_dm_width, &u4cur_dm_height);

	if ((u4cur_dm_width != u4width) ||
		(u4cur_dm_height != u4height)) {
		hdr_input_width[layer_id] = u4width;
		hdr_input_height[layer_id] = u4height;
		if (hdr_path_select == DOVI_PATH)
			disp_dovi_update_input_size(layer_id,
			u4width, u4height);
		else
			disp_cfd_drv_set_video_wh(layer_id,
			u4width, u4height);

		hdr_info("set hdr[%d] inputsize(%d %d)\n",
			layer_id, u4width, u4height);
	}
}

void disp_hdr_irq_handle(void)
{
	hdr_printf("%s %d %d\n", __func__, disp_hdr_irq_event, befifo_irq_cnt);
	if (disp_hdr_irq_event & 0x5F)
		disp_hdr_wakeup_routine(4);
}
void disp_hdr_path_ctl(uint32_t id, uint32_t en, uint32_t type)
{
	switch (type) {
	case 0:
		if (id < LAYER_MAX)
			disp_hdr_fe_start_stop(id, en);
		else
			disp_hdr_vdo_be_start_stop(en);
		break;
	case 1:
		if (id < LAYER_MAX)
			disp_hdr_fe_set_clk(id, en);
		else
			disp_hdr_set_vdo_be_clk(en);
		break;
	case 2:
		if (id < LAYER_MAX)
			disp_hdr_fe_set_path(id, en);
		else
			disp_hdr_set_vdo_be_path(en);
		break;
	default:
		break;
	}
	hdr_printf("hdr_path[%d] en[%d] type[%d]\n", id, en, type);
}

void disp_hdr_get_cur_path(uint8_t *path)
{
	*path = hdr_path_select;
}

