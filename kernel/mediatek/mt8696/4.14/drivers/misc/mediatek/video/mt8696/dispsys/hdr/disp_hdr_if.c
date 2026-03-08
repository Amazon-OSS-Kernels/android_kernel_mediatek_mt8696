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
struct disp_hw_common_info hdr_common_info;

enum hdr_output_type hdr_output_signal_type;
uint32_t hdr_output_emp_type;
bool bsub_exist;
#ifdef CONFIG_DOVI_SUPPORT
enum HDR_PATH hdr_path_select = DEFAULT_PATH;
#else
enum HDR_PATH hdr_path_select = OPENHDR_PATH;
#endif
uint32_t ui_force_hdr_type;
uint32_t hdr_vsync_cnt;
uint32_t time_check;
uint32_t line_cnt[5];
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
uint32_t hdr_frame_num;
uint32_t dovi_vsif_contenttype;
unsigned char dovi_vs10_signal_type;

/*add for allm*/
bool hdr_allm_en;
bool hdr_gfx_allm_en;
bool old_hdr_allm_en;
bool old_hdr_gfx_allm_en;
uint32_t hdr_allm_change;
uint32_t hdr_gfx_allm_change;
bool hdr_allm_ctl_by_cmd;
uint32_t hdr_allm_type;
uint32_t osd_force_allm;
/*ui allm type 0 auto, 1 disable, 2 always enable*/
enum ALLM_UI ui_allm_type = ALLM_INVALID;
enum ALLM_UI ui_allm_type_pre = ALLM_INVALID;
bool b_allm_ctl_force_hdr;

/*delay hdr change*/
bool delay_hdr_chg;
uint32_t delay_hdr_cur_vsync;
uint32_t delay_hdr_num = 7;
uint32_t delay_hdr_mute_num = 7;

bool tv_info_set_by_cmd;
uint32_t use_dv_s_type;
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
	if (hdr_common_info.tv.is_support_hdr10_plus &&
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
}

void disp_hdr_set_hdmi_signal(uint32_t hdr_type, bool enable,
	bool bt2020_enable, struct VID_PLA_HDR_METADATA_INFO_T rHdr)
{
	struct disp_hw_tv_capbility *tv_cap = &hdr_common_info.tv;

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
		if (enable) {
			dovi_vsif_contenttype = p_vsif->content_type;
			dovi_vs10_signal_type = p_vsif->dovi_signal_type;
			vSetDoviVsifParamter((void *)p_vsif);
		}
		vDoviHdrEnable(enable);
		break;
	case HDR_OUT_TYPE_VSEM_DV_STD:
		vDoviVsemHdrEnable(enable, 1);
		//hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	case HDR_OUT_TYPE_DV_LL:
		if (enable) {
			dovi_vsif_contenttype = p_vsif->content_type;
			dovi_vs10_signal_type = p_vsif->dovi_signal_type;
			vSetDoviVsifParamter((void *)p_vsif);
		}
		vLowLatencyDoviEnable(enable);
		hdr_set_HDMI_BT2020_signal(bt2020_enable);
		break;
	case HDR_OUT_TYPE_VSEM_DV_LL:
		vDoviVsemHdrEnable(enable, 2);
		//hdr_set_HDMI_BT2020_signal(bt2020_enable);
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
			ui_force_hdr_type, dovi_out_format);
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
	uint8_t *md_vsem = NULL;
	uint32_t vsem_pkt_num = 0;
	uint32_t md_pkt_type = 0;
	struct disp_hw_tv_capbility *tv_cap = &hdr_common_info.tv;

	mutex_lock(&disp_hdr_cfg_hdmi_mutex);

	if (path == DOVI_PATH) {
		dovi_get_hdmi_output_format(&out_format, &bt2020_enable);
		dovi_get_hdr10_metadata(&rHdr, 0);
	} else {
		disp_cfd_drv_get_hdmi_output_format(0,
			&out_format, &bt2020_enable);
		disp_cfd_drv_get_hdr10_metadata(&rHdr, 0);
	}

	/*dovi game mode only support when LL output*/
	if ((out_format == HDR_OUT_TYPE_DV_LL)
		|| (out_format == HDR_OUT_TYPE_VSEM_DV_LL)) {
		if (p_vsif != NULL) {
			if (hdr_allm_en || hdr_gfx_allm_en
				|| (ui_allm_type == ALLM_EN)) {
				/* add dummy L11MD when source not contain */
				if (p_vsif->L11_md_present == 0) {
					p_vsif->L11_md_present = L11_MD_PRESENT;
					p_vsif->content_type = L11_CONTENT_GAME;
					p_vsif->white_point = L11_WHITE_POINT;
					p_vsif->L11_byte2 = 0;
					p_vsif->L11_byte3 = 0;
				}
			} else {
				/* if game mode disable,
				 * clear those parameter
				 */
				p_vsif->L11_md_present = 0;
				p_vsif->content_type = 0;
				p_vsif->white_point = 0;
				p_vsif->L11_byte2 = 0;
				p_vsif->L11_byte3 = 0;
			}
		}
	} else if ((out_format == HDR_OUT_TYPE_DV_STD)
		|| (out_format == HDR_OUT_TYPE_VSEM_DV_STD)) {
		if (p_vsif != NULL) {
			//if std mode, clear those parameter
			p_vsif->L11_md_present = 0;
			p_vsif->content_type = 0;
			p_vsif->white_point = 0;
			p_vsif->L11_byte2 = 0;
			p_vsif->L11_byte3 = 0;
		}
	}

	/*keep old vsif case
	 * sny dolby tv, keep old visf
	 * other tv ,dm version < 2 ,keep old vsif
	 */

	if ((out_format == HDR_OUT_TYPE_DV_STD) || (out_format == HDR_OUT_TYPE_DV_LL))
		if ((p_vsif != NULL) &&
			(is_sny_dv_tv() || (tv_cap->dovi_vsvdb_dm_version < 2) ||
			!use_dv_s_type))
			p_vsif->dovi_signal_type = 0;

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
			(out_format == HDR_OUT_TYPE_HDR10PLUS))) ||
			((hdr_output_signal_type == HDR_OUT_TYPE_DV_STD) &&
			(out_format == HDR_OUT_TYPE_DV_LL)) ||
			((hdr_output_signal_type == HDR_OUT_TYPE_DV_LL) &&
			(out_format == HDR_OUT_TYPE_DV_STD)))
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
		} else {
			if (delay_hdr_chg) {
				delay_hdr_chg = false;
				disp_mix_hal_set_black_pattern(false);
				hdr_printf("cancel delay %d\n", hdr_vsync_cnt);
			}
			disp_hdr_set_hdmi_signal(hdr_output_signal_type, false,
					false, rHdr);
		}
		if (!delay_hdr_chg) {
			hdr_output_signal_type = out_format;
			disp_hdr_set_hdmi_signal(out_format, true,
				bt2020_enable, rHdr);
			hdr_printf("set hdmi type %d %d %d\n",
				hdr_output_signal_type, hdr_vsync_cnt, bt2020_enable);
		}
	}
	if (!delay_hdr_chg) {
		// update static md
		if ((out_format == HDR_OUT_TYPE_HDR10) ||
			(out_format == HDR_OUT_TYPE_HLG) ||
			(out_format == HDR_OUT_TYPE_HDR10PLUS) ||
			(out_format == HDR_OUT_TYPE_HDR10PLUS_VSIF))
			vVdpSetHdrMetadata(true, rHdr);
		else if ((out_format == HDR_OUT_TYPE_DV_STD) ||
			(out_format == HDR_OUT_TYPE_DV_LL)) {
			//dovi vsif parameter change ,need re-send vsif
			if ((dovi_vsif_contenttype != p_vsif->content_type)
				|| (dovi_vs10_signal_type != p_vsif->dovi_signal_type)) {
				vSetDoviVsifParamter((void *)p_vsif);
				dovi_vsif_contenttype = p_vsif->content_type;
				dovi_vs10_signal_type = p_vsif->dovi_signal_type;
				hdr_printf("vsif content change1 %d %d %d %d %d %d %d\n",
					p_vsif->low_latency,
					p_vsif->backlt_ctrl_md_present,
					p_vsif->source_dm_version,
					p_vsif->eff_tmax_pq,
					p_vsif->dovi_signal_type,
					p_vsif->auxiliary_md_present,
					p_vsif->L11_md_present);
				hdr_printf("vsif content change2 %d %d %d %d %d %d %d %d\n",
					p_vsif->auxiliary_runmode,
					p_vsif->auxiliary_runversion,
					p_vsif->auxiliary_debug0,
					p_vsif->content_type,
					p_vsif->white_point,
					p_vsif->L11_byte2,
					p_vsif->L11_byte3,
					p_vsif->bt2020_container);
			}
		} else if ((out_format == HDR_OUT_TYPE_VSEM_DV_STD) ||
			(out_format == HDR_OUT_TYPE_VSEM_DV_LL)) {

			if (idk_vsem)
				disp_dovi_dump_vsem();

			md_vsem = disp_dovi_get_hdmi_vsem_info(&vsem_pkt_num,
				&md_pkt_type);

			if (((out_format == HDR_OUT_TYPE_VSEM_DV_STD) && (md_pkt_type != 2))
				|| ((out_format == HDR_OUT_TYPE_VSEM_DV_LL) && (md_pkt_type != 3)))
				hdr_printf("vsem type not match %d %d\n", out_format, md_pkt_type);

			if (((md_pkt_type == 2) || (md_pkt_type == 3)) &&
				(vsem_pkt_num != 0) && (md_vsem != NULL)) {
				rHdr.e_DynamicRangeType = VID_PLA_DR_TYPE_DOVI_VSEM;
				rHdr.metadata_info.dovi_vsem_metadata.PktNum =
					vsem_pkt_num;
				rHdr.metadata_info.dovi_vsem_metadata
					.dovi_vsem_md_info =
				(struct VID_DOVI_VSEM_METADATA_INFO_T *)md_vsem;
				vVdpSetHdrMetadata(true, rHdr);
			}

		}
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
		if (time_check)
			line_cnt[3] = Dv_ReadREG(vdout_reg_base + 0x28) & 0xFFF;
		hdr_video_info("confg hdmi done[%d] %d\n",
			hdr_vsync_cnt, line_cnt[3]);
	}
	/* game mode enable/disable */
	if (hdr_allm_change || hdr_gfx_allm_change) {
		hdmi_game_mode_enable(hdr_allm_en | hdr_gfx_allm_en);
		old_hdr_allm_en = hdr_allm_en;
		old_hdr_gfx_allm_en = hdr_gfx_allm_en;
		hdr_allm_change = 0;
		hdr_gfx_allm_change = 0;
	}
	mutex_unlock(&disp_hdr_cfg_hdmi_mutex);
}

void disp_hdr_config_video_non(uint32_t id)
{
	if (id == 0)
		disp_hdr_set_event(HDR_EVENT_VLAYER0_CFG_DONE);
	if (id == 1)
		disp_hdr_set_event(HDR_EVENT_VLAYER1_CFG_DONE);

	if ((disp_hdr_event & HDR_EVENT_VLAYER0_CFG_DONE)
		&& (disp_hdr_event & HDR_EVENT_VLAYER1_CFG_DONE)) {
		disp_hdr_wakeup_routine(0);
		hdr_info("layer[%d] no frame but need trigger case\n", id);
	}
}

int disp_hdr_config_video_info(struct video_buffer_info *buf,
	bool sub_exit)
{

	if (buf == NULL || buf->layer_id >= V_G_LAYER_MAX) {
		hdr_printf("error video buffer\n");
		return -1;
	}

	if (idk_vdo_en > 1) {
		sub_exit = true;
		hdr_printf("xiao layer_id %d\n", buf->layer_id);
	}

	if (time_check)
		line_cnt[0] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	hdr_video_info("video[%d][%d] %d %d %d pts = %lld %d %d\n", buf->layer_id,
		hdr_vsync_cnt, buf->hdr_info.dr_range, buf->hdr_info.enable,
		sub_exit, buf->pts, hdr_frame_num, line_cnt[0]);

	if (buf->layer_id == LAYER0) {
		memcpy(&hdr_video_layer[LAYER0], buf, sizeof(*buf));
		hdr_video_layer[LAYER0].new_frame = 1;
		bsub_exist = sub_exit;
		if (sub_exit == 0) {
			disp_hdr_set_event(HDR_EVENT_VLAYER0_CFG_DONE);
			disp_hdr_set_event(HDR_EVENT_VLAYER1_CFG_DONE);
		} else
			disp_hdr_set_event(HDR_EVENT_VLAYER0_CFG_DONE);
	} else {
		memcpy(&hdr_video_layer[LAYER1], buf, sizeof(*buf));
		hdr_video_layer[LAYER1].new_frame = 1;
		bsub_exist = sub_exit;
		disp_hdr_set_event(HDR_EVENT_VLAYER1_CFG_DONE);
	}

	if ((disp_hdr_event & HDR_EVENT_VLAYER0_CFG_DONE)
		&& (disp_hdr_event & HDR_EVENT_VLAYER1_CFG_DONE))
		disp_hdr_wakeup_routine(0);

	if ((!dovi_path_en) && (buf->layer_id == LAYER0)) {
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
	bool b_allm_src = false;

	if (buf == NULL || buf->layer_id >= V_G_LAYER_MAX) {
		hdr_printf("error osd buffer\n");
		return -1;
	}

	if (hdr_trig_algn_mvid[buf->layer_id + V_G_LAYER_MAX])
		hdr_trig_algn_mvid[buf->layer_id + V_G_LAYER_MAX] = false;

	memcpy(&hdr_osd_layer[buf->layer_id], buf, sizeof(*buf));
	hdr_osd_info("osd[%d][%d][%d] config %d\n", buf->layer_id,
		befifo_irq_cnt, hdr_vsync_cnt, dovi_path_en);

	/* add gfx allm flow when ALLM ui set as auto
	 * and gfx frame set as allm source
	 */
	if (osd_force_allm)
		hdr_osd_layer[buf->layer_id].allm_en = 1;

	if (hdr_osd_layer[LAYER0].allm_en || hdr_osd_layer[LAYER1].allm_en)
		b_allm_src = true;
	disp_hdr_handle_allm_change_for_gfx(b_allm_src);

	if (!dovi_path_en) {
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

	hdr_info("osd[%d][%d] config fake %d\n", layer_id,
		hdr_vsync_cnt, dovi_path_en);
	hdr_trig_algn_mvid[layer_id + V_G_LAYER_MAX] = true;
	return 0;
}

int disp_hdr_handle_forcehdr(enum DISP_CMD cmd, void *data)
{
	uint32_t default_path = OPENHDR_PATH;
	uint32_t line_count[3] = { 0 };
	uint32_t force_hdr_type = 0;

	if (data == NULL) {
		hdr_printf("%s error param\n", __func__);
		return -1;
	}

	force_hdr_type = *((uint32_t *) data);

	if (time_check)
		line_count[0] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	if (!tv_info_set_by_cmd)
		disp_hw_mgr_get_info(&hdr_common_info);

	/*fhd hdr always enable
	 * if uiforce && dovi efuse existed,
	 * vdo be enable
	 */
	#ifdef CONFIG_DOVI_SUPPORT
	if (g_dovi_efuse) {
		if ((force_hdr_type == DYNA_SET_FORCE_HDR) &&
			(hdr_path_select == OPENHDR_PATH) &&
			(vdp_start_st[0] == 1) &&
			((hdr_video_layer[0].hdr10_type == HDR10_TYPE_PLUS) &&
			(disp_common_info.tv.is_support_hdr10_plus))) {
			hdr_printf("[VS10] special case do nothing\n");
			return 0;
		}
		disp_hdr_vdo_be_start_stop((bool)force_hdr_type);
		if (force_hdr_type) {
			default_path = DOVI_PATH;
			hdr_path_select = default_path;
		} else {
			disp_ml_set_disable(0);
			if (vdp_start_st[0] == 1 || (vdp_start_st[1] == 1))
				disp_ml_set_disable(1);
		}
		disp_dovi_process_cmd(LAYER0, cmd, data);
	}
	#endif

	hdr_path_select = default_path;
	if (time_check)
		line_count[1] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	if (default_path == OPENHDR_PATH) {
		disp_cfd_set_cmd(LAYER0, cmd, data);
		if ((vdp_start_st[0] == 1) &&
			(hdr_video_layer[0].is_dovi == false)) {
			hdr_video_layer[LAYER0].new_frame = 1;
			disp_hdr_path_judge();
		} else if ((vdp_start_st[1] == 1) &&
			(hdr_video_layer[1].is_dovi == false))
			disp_hdr_wakeup_routine(1);
		else {
			/*if (hdr_fe_en[LAYER2]) {
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
			*/
			if (hdr_fe_en[LAYER2])
				disp_hdr_wakeup_routine(2);
			if (hdr_fe_en[LAYER3])
				disp_hdr_wakeup_routine(3);
		}
	}

	disp_hdr_config_hdmi_signal(default_path);
	if (time_check)
		line_count[2] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	hdr_printf("ui force hdr type:%d path %d video_st[%d %d],%d, %d %d %d\n",
		force_hdr_type, hdr_path_select, vdp_start_st[0],
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
	int sof_end = 0x00030033;

	hw_id = DISP_PATH_DISP_HDR_VDO_BE;
	hw_sof_start = FMT_SOF_21_HDR_BE_STA;
	hw_sof_end = FMT_SOF_21_HDR_BE_END;

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
	int sof_end = 0x00030033;

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
		hw_sof_start = FMT_SOF_6_M_HDR_FE_STA;
		hw_sof_end = FMT_SOF_6_M_HDR_FE_END;
		break;
	case LAYER1:
		hw_id = DISP_PATH_S_HDR_VDO_FE;
		hw_sof_start = FMT_SOF_13_S_HDR_FE_STA;
		hw_sof_end = FMT_SOF_13_S_HDR_FE_END;
		break;
	case LAYER2:
		hw_id = DISP_PATH_FHD_HDR_GFX_FE;
		hw_sof_start = FMT_SOF_19_FHD_HDR_FE_STA;
		hw_sof_end = FMT_SOF_19_FHD_HDR_FE_END;
		break;
	case LAYER3:
		hw_id = DISP_PATH_UHD_HDR_GFX_FE;
		hw_sof_start = FMT_SOF_16_UHD_HDR_FE_STA;
		hw_sof_end = FMT_SOF_16_UHD_HDR_FE_END;
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
			#ifdef CONFIG_DOVI_SUPPORT
			if ((layer_id < LAYER2) && g_dovi_efuse)
				disp_dovi_set_adldelay(layer_id);
			#endif
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
	#ifdef CONFIG_DOVI_SUPPORT
	if (g_dovi_efuse)
		disp_dovi_process_cmd(layer_id, cmd, data);
	#endif

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
	/* need clean gfx allm_en when stop */
	hdr_osd_layer[layer_id].allm_en = 0;
	if (!hdr_osd_layer[LAYER0].allm_en && !hdr_osd_layer[LAYER1].allm_en)
		disp_hdr_handle_allm_change_for_gfx(false);
	#ifdef CONFIG_DOVI_SUPPORT
	if (g_dovi_efuse)
		disp_dovi_process_cmd(layer_id, cmd, data);
	#endif
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

	dovi_path_ready2start = 1;

	disp_hdr_fe_start_stop(layer_id, true);

	disp_adl_clock_on_off(DISPSYS_ADL, true);

	if (layer_id < V_G_LAYER_MAX)
		vdp_start_st[layer_id] = 1;

	return 0;
}

uint32_t disp_hdr_allm_ctl_path(void)
{
	enum dovi_signal_format_t def_out_format = DOVI_FORMAT_SDR;
	uint32_t forcehdr = DYNA_SET_INVALID;
	struct disp_hw_tv_capbility *tv_cap = NULL;

	/* tv support allm but only std_dovi, need change to adaptive
	 * tv support dovi_std and dovi_ll and allm on/off, output need change
	 * this happened when dynamic range setting is forcehdr
	 */
	tv_cap = &disp_common_info.tv;
	def_out_format =
		dovi_judge_out_format(tv_cap, disp_common_info.resolution);

	if (!((ui_force_hdr_type == DYNA_SET_FORCE_HDR)
		|| (g_force_hdr == DYNA_SET_FORCE_HDR)))
		return forcehdr;

	if ((def_out_format == DOVI_FORMAT_DOVI)
		&& (tv_cap->u1_sink_allm_support
		|| tv_cap->u1_sink_14gamemode_support)) {
		forcehdr = DYNA_SET_ADAPTIVE;
		dovi_out_format = DOVI_FORMAT_SDR;
		b_allm_ctl_force_hdr = true;
	} else if ((def_out_format == DOVI_FORMAT_DOVI_LOW_LATENCY)
		&& (dovi_out_format != def_out_format))
		forcehdr = DYNA_SET_FORCE_HDR;

	return forcehdr;
}

void disp_hdr_allm_process(struct disp_hw_tv_capbility *tv_cap)
{
	enum DISP_CMD cmd = DISP_CMD_FORCE_HDR;
	uint32_t forcehdr = DYNA_SET_INVALID;

	if (tv_cap == NULL)
		return;

	/* handle output format change when allm change on UI */
	switch (ui_allm_type) {
	case ALLM_AUTO:
	case ALLM_DIS:
		/* control force hdr or support lowlatency tv
		 * when force hdr on need rejudge format change
		 */
		if ((ui_allm_type_pre == ALLM_EN)
			&& (b_allm_ctl_force_hdr
			|| tv_cap->is_support_dovi_low_latency)) {
			forcehdr = ui_force_hdr_type;
			b_allm_ctl_force_hdr = false;
		}
		break;
	case ALLM_EN:
		b_allm_ctl_force_hdr = false;
		forcehdr = disp_hdr_allm_ctl_path();
		break;
	default:
		break;
	}

	if (forcehdr != DYNA_SET_INVALID) {
		disp_hdr_handle_forcehdr(cmd, (void *)(&forcehdr));
		dovi_vs10_path_en = forcehdr;
	}

	hdr_printf("ui allm type %d %d %d %d %d %d %d %d %d %d\n",
		ui_allm_type_pre, forcehdr, b_allm_ctl_force_hdr,
		dovi_out_format, ui_force_hdr_type, g_force_hdr,
		tv_cap->u1_sink_allm_support,
		tv_cap->u1_sink_14gamemode_support,
		tv_cap->is_support_dovi_low_latency,
		tv_cap->is_support_dovi);
}

void disp_hdr_handle_allm_change(void *data)
{
	struct disp_hw_tv_capbility *tv_cap = NULL;

	if ((data == NULL) || (*((uint32_t *)data) > (ALLM_INVALID - 1))) {
		hdr_error("set allm type error\n");
		return;
	}

	disp_hw_mgr_get_info(&disp_common_info);
	tv_cap = &(disp_common_info.tv);

	ui_allm_type = (enum ALLM_UI)(*((uint32_t *)data));

	/* allm ui change only influence display output format on dovi tv
	 * that support game(allm) or lowlatecy and on forcehdr mode
	 */
	if (((ui_force_hdr_type == DYNA_SET_FORCE_HDR)
		|| (g_force_hdr == DYNA_SET_FORCE_HDR))
		&& (tv_cap->u1_sink_allm_support
		|| tv_cap->u1_sink_14gamemode_support
		|| tv_cap->is_support_dovi_low_latency)
		&& tv_cap->is_support_dovi) {
		disp_hdr_allm_process(tv_cap);
	} else {
		if (b_allm_ctl_force_hdr)
			b_allm_ctl_force_hdr = false;
		hdr_printf("display no need do change\n");
	}

	ui_allm_type_pre = ui_allm_type;
}

void disp_hdr_handle_allm_change_for_gfx(bool b_allm_gfx)
{
	struct disp_hw_tv_capbility *tv_cap = NULL;
	enum DISP_CMD cmd = DISP_CMD_FORCE_HDR;
	uint32_t forcehdr = DYNA_SET_INVALID;

	tv_cap = &(disp_common_info.tv);

	/* only ALLM ui set as auto will check gfx allm src */
	if (tv_cap == NULL || (ui_allm_type != ALLM_AUTO))
		return;

	/* config allm once when change */
	mutex_lock(&disp_hdr_gfx_allm_mutex);
	if (b_allm_gfx && (tv_cap->u1_sink_allm_support
		|| tv_cap->u1_sink_14gamemode_support
		|| tv_cap->is_support_dovi_low_latency))
		hdr_gfx_allm_en = 1;
	else
		hdr_gfx_allm_en = 0;

	if (hdr_gfx_allm_en != old_hdr_gfx_allm_en) {
		hdr_gfx_allm_change = 1;
		if (hdr_gfx_allm_en) {
			/* check need change path for std_only + allm tv
			 * or dovi_ll support tv
			 */
			b_allm_ctl_force_hdr = false;
			forcehdr = disp_hdr_allm_ctl_path();
		} else {
			/* check need change output format to non allm */
			if (b_allm_ctl_force_hdr
				|| tv_cap->is_support_dovi_low_latency) {
				forcehdr = ui_force_hdr_type;
				b_allm_ctl_force_hdr = false;
			}
		}

		if (forcehdr != DYNA_SET_INVALID) {
			disp_hdr_handle_forcehdr(cmd, (void *)(&forcehdr));
			dovi_vs10_path_en = forcehdr;
		}

		disp_hdr_config_hdmi_signal(hdr_path_select);
		hdr_osd_info("[gfx_allm] %d %d %d %d %d\n",
		b_allm_gfx, hdr_gfx_allm_en, ui_force_hdr_type,
		forcehdr, hdr_path_select);
	}
	mutex_unlock(&disp_hdr_gfx_allm_mutex);
}

void disp_hdr_stop_handle(uint32_t layer_id)
{
	//struct disp_hw *hdr_drv = disp_hdr_get_drv();
	enum HDR_PATH current_path = OPENHDR_PATH;
	enum HDR_PATH dst_path = OPENHDR_PATH;
	#ifdef CONFIG_DOVI_SUPPORT
	enum DISP_DR_TYPE_T dovi_input_dr_type = DISP_DR_TYPE_SDR;
	uint32_t allm_forcehdr = DYNA_SET_INVALID;
	#endif
	bool real_stop = true;

	mutex_lock(&disp_hdr_stop_mutex);
	/* disable layer and cfd anyway */
	if (layer_id < V_G_LAYER_MAX) {
		vdp_start_st[layer_id] = 0;
		dovi_vdo_fe_hal_set_enable(layer_id, false);
	}
	disp_cfd_enable(layer_id, false);

	if (layer_id == LAYER1) {
		bsub_exist = false;
		dovi_path_ready2start = 1;
	}

	if (vdp_start_st[LAYER0] || vdp_start_st[LAYER1])
		real_stop = false;

	if (!real_stop) {
		hdr_printf("layer[%d] stop, but not real_stop\n", layer_id);
		disp_hdr_fe_start_stop(layer_id, false);
		#ifdef CONFIG_DOVI_SUPPORT
		if (g_dovi_efuse) {
			dovi_hdr_md_info[layer_id].enable = DOVI_INOUT_FORMAT_CHANGE;
			dovi_hdr_md_info[layer_id].dr_range = DISP_DR_TYPE_SDR;
		}
		#endif
		mutex_unlock(&disp_hdr_stop_mutex);
		return;
	}

	/* only real stop need change path */
	#ifdef CONFIG_DOVI_SUPPORT
	if (dovi_path_en && g_dovi_efuse) {
		current_path = DOVI_PATH;
		if (current_path != hdr_path_select)
			hdr_printf("path not match when stop\n");
	}

	if (ui_force_hdr_type && g_dovi_efuse)
		dst_path = DOVI_PATH;
	else
		dst_path = OPENHDR_PATH;

	/* video stop return homeui, hdr path also need refs ALLM UI setting
	 * when ALLM force on
	 */
	if (ui_allm_type == ALLM_EN) {
		allm_forcehdr = disp_hdr_allm_ctl_path();
		if (allm_forcehdr == DYNA_SET_ADAPTIVE)
			dst_path = OPENHDR_PATH;
		else if (allm_forcehdr == DYNA_SET_FORCE_HDR)
			dst_path = DOVI_PATH;
	}
	#else
	dst_path = OPENHDR_PATH;
	#endif

	/* here stop game mode when stop gamming content */
	if (hdr_allm_en) {
		hdr_allm_en = 0;
		hdmi_game_mode_enable(hdr_allm_en || hdr_gfx_allm_en);
		old_hdr_allm_en = hdr_allm_en;
	} else {
		if ((ui_allm_type == ALLM_EN)
			&& (disp_common_info.tv.u1_sink_allm_support
			|| disp_common_info.tv.u1_sink_14gamemode_support)
			&& !disp_common_info.tv.is_support_dovi_low_latency
			&& disp_common_info.tv.is_support_dovi)
			vHdmiGameModeEn(true);
	}

	/* save dst hdr path*/
	hdr_path_select = dst_path;

	hdr_printf("layer[%d][%d]stop path %d %d %d %d %d %d\n", layer_id,
		hdr_vsync_cnt, befifo_irq_cnt,
		current_path, dst_path, g_dovi_efuse,
		ui_allm_type, dovi_path_en);

	/* disable dsys ml frist avoid error setting */
	disp_ml_set_disable(ML_DSYS_IP);

	if (dst_path == DOVI_PATH) {
		// reset cfd graphic status first here
		if (hdr_fe_en[LAYER2] && (current_path == OPENHDR_PATH))
			disp_cfd_drv_set_feature_type(LAYER2, DISP_CFD_BYPASS);
		if (hdr_fe_en[LAYER3] && (current_path == OPENHDR_PATH))
			disp_cfd_drv_set_feature_type(LAYER3, DISP_CFD_BYPASS);
		#ifdef CONFIG_DOVI_SUPPORT
		if (dovi_path_en == 1 && g_dovi_efuse) {
			/* change dv input type to sdr */
			dovi_hdr_md_info[0].enable =
			DOVI_INOUT_FORMAT_CHANGE;
			dovi_hdr_md_info[0].dr_range = DISP_DR_TYPE_SDR;
			dovi_hdr_md_info[1].enable =
			DOVI_INOUT_FORMAT_CHANGE;
			dovi_hdr_md_info[1].dr_range = DISP_DR_TYPE_SDR;
			dovi_update_output_setting(&hdr_common_info,
			NULL, NULL);
			dovi_get_input_format(&dovi_input_dr_type);

			disp_dovi_process_cmd(LAYER0, DISP_CMD_METADATA_UPDATE,
				&dovi_hdr_md_info[0]);
			dovi_vs10_path_en = ui_force_hdr_type;
		} else if (g_dovi_efuse) {
			/* call openhdr stop if need */
			disp_hdr_vdo_be_start_stop(true);
			dovi_path_enable();
		}
		#endif
	} else {
		#ifdef CONFIG_DOVI_SUPPORT
		/*stop from dovi path*/
		if (dovi_path_en && !dovi_idk_test && g_dovi_efuse) {
			dovi_black_pattern_en = true;
			dovi_black_pattern_cnt = 0;
			dovi_black_pattern_cnt_max = 5;
			if (dovi_black_en_bycmd)
				dovi_black_pattern_cnt_max = dovi_black_cnt_bycmd;
			if (dovi_black_pattern_cnt_max > 0)
				disp_mix_hal_set_black_pattern(true);
		}
		#endif
		/* disable msys ml because ml will still update dvgfx setting*/
		disp_ml_set_disable(ML_MSYS_IP);
		//set vdo hdr fe and be internabypss
		#ifdef CONFIG_DOVI_SUPPORT
		if (g_dovi_efuse) {
			disp_hdr_vdo_be_start_stop(false);
			disp_dovi_set_internalbyass(0);
			dovi_config_fefifo_swap(false);
		}
		#endif

		#ifdef CONFIG_DOVI_SUPPORT
		if (dovi_path_en && g_dovi_efuse) {
			dovi_hdr_md_info[0].dr_range =
				DISP_DR_TYPE_PHLP_RESVERD;
			dovi_hdr_md_info[0].enable = 0;
			dovi_hdr_md_info[1].dr_range =
				DISP_DR_TYPE_PHLP_RESVERD;
			dovi_hdr_md_info[1].enable = 0;
			disp_dovi_process_cmd(LAYER0, DISP_CMD_METADATA_UPDATE,
				&dovi_hdr_md_info[0]);
			dovi_path_en = 0;
			dovi_vs10_path_en = 0;
		}
		#endif
	}

	disp_hdr_fe_start_stop(layer_id, false);
	/*video stop ,video adl client should disable*/
	disp_adl_cfg_client_en(DV_ADL_V_MAIN, 0, 0);
	disp_adl_cfg_client_en(DV_ADL_V_SUB, 0, 0);
	disp_adl_clock_on_off(DISPSYS_ADL, false);
	disp_hdr_config_hdmi_signal(dst_path);
	hdr_printf("stop done[%d][%d]\n", hdr_vsync_cnt, befifo_irq_cnt);
	mutex_unlock(&disp_hdr_stop_mutex);
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

	mutex_lock(&disp_hdr_path_mutex);
	buf_main = &hdr_video_layer[LAYER0];
	if (bsub_exist)
		buf_sub = &hdr_video_layer[LAYER1];
	if (!(buf_main->new_frame || (bsub_exist && buf_sub->new_frame))) {
		hdr_video_info("hdr no new frame %d %d\n",
			buf_main->new_frame, bsub_exist);
		mutex_unlock(&disp_hdr_path_mutex);
		return 0;
	}
	if (time_check)
		line_cnt[1] = HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;

	/*control allm_en by cmd*/
	if (hdr_allm_ctl_by_cmd)
		buf_main->video_disp_buffer.allm_en = hdr_allm_type;

	hdr_frame_num++;
	hdr_video_info("judge[%d] %d %d %d %d %d %d pts %lld %d %d %d\n",
		hdr_vsync_cnt,
		bsub_exist, ui_force_hdr_type,
		g_dovi_efuse, buf_main->hdr10_type,
		buf_main->is_dovi,
		buf_main->hdr_info.dr_range,
		buf_main->pts,
		line_cnt[1],
		buf_main->video_disp_buffer.allm_en,
		hdr_allm_en);
	if (bsub_exist && (buf_sub != NULL)) {
		hdr_video_info("sub info[%d] %d %d %d %d pts = %lld\n",
		hdr_vsync_cnt, g_dovi_efuse, buf_sub->hdr10_type,
		buf_sub->is_dovi,
		buf_sub->hdr_info.dr_range,
		buf_sub->pts);
	}

	if (!tv_info_set_by_cmd)
		disp_hw_mgr_get_info(&hdr_common_info);
	tv_cap = &(hdr_common_info.tv);

	/*
	 * add ALLM control flow for gamming source and allm support tv
	 * ui need set allm as auto or on
	 */
	if (ui_allm_type == ALLM_EN)
		buf_main->video_disp_buffer.allm_en = true;

	if ((ui_allm_type != ALLM_DIS) &&
		(buf_main->video_disp_buffer.allm_en != hdr_allm_en)) {
		hdr_allm_change = 1;
		if ((tv_cap->u1_sink_allm_support ||
			tv_cap->u1_sink_14gamemode_support)
			&& buf_main->video_disp_buffer.allm_en)
			hdr_allm_en = 1;
		else
			hdr_allm_en = 0;
	}

	def_out_format = dovi_judge_out_format(tv_cap,
		hdr_common_info.resolution);

	if (hdr_allm_en && buf_main->is_dovi) {
		/*STD mode do not enable allm */
		if (def_out_format == DOVI_FORMAT_DOVI)
			hdr_allm_en = 0;
	}

	if (old_hdr_allm_en == hdr_allm_en)
		hdr_allm_change = 0;

	hdr_video_info("[Allm] %d %d %d %d %d\n",
		buf_main->video_disp_buffer.allm_en,
		hdr_allm_en, old_hdr_allm_en,
		hdr_allm_change, def_out_format);

	#ifdef CONFIG_DOVI_SUPPORT
	if (ui_force_hdr_type) {
		if (g_dovi_efuse) {
			if (((buf_main->hdr10_type == HDR10_TYPE_PLUS) &&
				((!bsub_exist) || (bsub_exist
				&& (buf_sub != NULL)
				&& (buf_sub->hdr10_type == HDR10_TYPE_PLUS)))
				&& (tv_cap->is_support_hdr10_plus))
				|| (hdr_allm_en && !(buf_main->is_dovi)
				&& (def_out_format ==
				DOVI_FORMAT_DOVI)))
				hdr_path = OPENHDR_PATH;
			else
				hdr_path = DOVI_PATH;
		} else
			hdr_path = OPENHDR_PATH;
	} else {
		if (g_dovi_efuse) {
			if ((buf_main->is_dovi) || (dovi_idk_dump) ||
				(bsub_exist && (buf_sub != NULL)
				&& (buf_sub->is_dovi)))
				hdr_path = DOVI_PATH;
			else
				hdr_path = OPENHDR_PATH;
		} else
			hdr_path = OPENHDR_PATH;
	}
	#else
	hdr_path = OPENHDR_PATH;
	#endif

	if (hdr_path == DOVI_PATH) {
		if (hdr_path_select != hdr_path) {
			/* disable openhdr path */
			hdr_printf("current path is %d\n", hdr_path);
			hdr_path_select = hdr_path;

			/* tv support dovi_std only and game, force allm on,
			 * but play dovi stream need disable game mode here once
			 */
			if ((ui_allm_type == ALLM_EN)
				&& (buf_main->is_dovi)
				&& (def_out_format == DOVI_FORMAT_DOVI)
				&& (tv_cap->u1_sink_allm_support
				|| tv_cap->u1_sink_14gamemode_support))
				vHdmiGameModeEn(false);
		}

		dovi_frame_commit(buf_main, buf_sub, bsub_exist);

		/* if dovi path ,vdo be clk and path enable */
		if (hdr_path == DOVI_PATH)
			disp_hdr_vdo_be_start_stop(true);

		if (time_check)
			line_cnt[2] =
			HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;
	} else {
		if (hdr_path_select != hdr_path) {
			hdr_path_select = hdr_path;
			#ifdef CONFIG_DOVI_SUPPORT
			if (g_dovi_efuse)
				dovi_path_disable();
			#endif
			disp_ml_set_disable(ML_MSYS_IP);
			disp_ml_set_disable(ML_DSYS_IP);
			#ifdef CONFIG_DOVI_SUPPORT
			if (g_dovi_efuse) {
				disp_hdr_vdo_be_start_stop(false);
				disp_dovi_set_internalbyass(2);
				dovi_config_fefifo_swap(false);
			}
			#endif
			hdr_printf("current path is %d\n", hdr_path);
		}
		/*wakeup thread1 to process sub video by mtkhdr */
		if (bsub_exist && (buf_sub != NULL))
			disp_hdr_wakeup_routine(1);
		// cfd config frame
		disp_cfd_config_video_frame(0, buf_main, tv_cap);
		if (time_check)
			line_cnt[2] =
			HDR_ReadREG(vdout_reg_base + 0x28) & 0xFFF;
	}
	buf_main->new_frame = 0;
	if (bsub_exist && (buf_sub != NULL))
		buf_sub->new_frame = 0;
	disp_hdr_config_hdmi_signal(hdr_path);
	if (time_check && !((line_cnt[1] <= line_cnt[2]) &&
		(line_cnt[2] <= line_cnt[3])))
		hdr_printf("over vsync %d %d %d %d\n",
		line_cnt[0], line_cnt[1], line_cnt[2], line_cnt[3]);

	mutex_unlock(&disp_hdr_path_mutex);
	return 0;
}

int disp_dovi_hdr_save_sec_rpu(uint32_t layer_id,
	struct mtk_disp_dovi_md_t *disp_dovi_info,
	struct mtk_vdp_dovi_md_t *dovi_md_info)
{
	if (disp_dovi_info == NULL || dovi_md_info == NULL
		|| (layer_id > 1)) {
		hdr_printf("%s %d params err\n", __func__, layer_id);
		return -1;
	}
	/* store rpu data into tz buffers,
	 * not transmit to dovi process immediately
	 */
	dovi_sec_find_rpu_buffer(layer_id, disp_dovi_info->sec_handle, disp_dovi_info->len);

	/* get rpu handle from tz buffers */
	dovi_md_info->sec_handle = dovi_share_mem->src_param[layer_id].sec_handle_out;
	dovi_md_info->len = disp_dovi_info->len;

	return 0;
}
void disp_idk_disable_module(void)
{
	//disable ml and adl to release band width
	disp_ml_set_disable(ML_DSYS_IP);
	disp_ml_set_disable(ML_MSYS_IP);
	disp_adl_cfg_client_en(DV_ADL_V_MAIN, 0, 0);
	disp_adl_cfg_client_en(DV_ADL_V_SUB, 0, 0);
	disp_adl_cfg_client_en(DV_ADL_G_FHD, 0, 0);
	disp_adl_cfg_client_en(DV_ADL_G_UHD, 0, 0);
	disp_adl_cfg_client_en(DV_SCRM, 0, 0);
	hdr_printf("disable ml and adl\n");
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

	hdr_input_width[layer_id] = u4width;
	hdr_input_height[layer_id] = u4height;
	if ((u4cur_dm_width != u4width) ||
		(u4cur_dm_height != u4height)) {
		#ifdef CONFIG_DOVI_SUPPORT
		if ((hdr_path_select == DOVI_PATH) && g_dovi_efuse)
			disp_dovi_update_input_size(layer_id,
			u4width, u4height);
		else
		#endif
			disp_cfd_drv_set_video_wh(layer_id,
			u4width, u4height);

		hdr_info("set hdr[%d] inputsize(%d %d) %d\n",
			layer_id, u4width, u4height, hdr_path_select);
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

