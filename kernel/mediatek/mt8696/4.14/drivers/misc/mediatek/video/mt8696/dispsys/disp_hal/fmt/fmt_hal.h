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
 * $Author: chuanfei.wang $
 * $Date: 2016/08/10 $
 * $RCSfile: fmt_hal.h,v $
 */

#ifndef _FMT_HAL_H_
#define _FMT_HAL_H_

/* -----------------------------------------------------------------------------
 */
/* Include files */
/* -----------------------------------------------------------------------------
 */
#include <linux/types.h>

#include "fmt_def.h"
#include "fmt_hw.h"
#include "hdmitx.h"
#include "disp_hw_mgr.h"

#define DISPFMT_H_FACTOR 0x1000

struct fmt_init_pararm {
	uintptr_t vdout_fmt_reg_base;
	uintptr_t disp_fmt_reg_base[DISP_FMT_CNT]; /*fmt1, fmt2*/
	uintptr_t io_reg_base;
};

struct fmt_active_info {
	uint32_t h_begine;
	uint32_t h_end;
	uint32_t v_odd_begine;
	uint32_t v_odd_end;
	uint32_t v_even_begine;
	uint32_t v_even_end;
};

struct fmt_delay_info {
	int adj_f;
	int v_delay;
	int h_delay;
	enum HDMI_VIDEO_RESOLUTION res;
};

enum FMT_SOF_TYPE {
	FMT_SOF_0_M_VDO_MAIN_STA,	/*0*/
	FMT_SOF_0_M_VDO_MAIN_END,	/*1*/
	FMT_SOF_1_M_VDO_AUX_STA,	/*2*/
	FMT_SOF_1_M_VDO_AUX_END,	/*3*/
	FMT_SOF_2_M_FE_FIFO_STA,	/*4*/
	FMT_SOF_2_M_FE_FIFO_END,	/*5*/
	FMT_SOF_3_STA,				/*6*/
	FMT_SOF_3_END,				/*7*/
	FMT_SOF_4_M_FILM_GRAIN_1_STA,	/*8*/
	FMT_SOF_4_M_FILM_GRAIN_1_END,	/*9*/
	FMT_SOF_5_M_FILM_GRAIN_2_STA,	/*10*/
	FMT_SOF_5_M_FILM_GRAIN_2_END,	/*11*/
	FMT_SOF_6_M_DOLBY_FE_STA,		/*12*/
	FMT_SOF_6_M_DOLBY_FE_END,		/*13*/
	FMT_SOF_7_S_VDO_MAIN_STA,		/*14*/
	FMT_SOF_7_S_VDO_MAIN_END,		/*15*/
	FMT_SOF_8_S_VDO_AUX_STA,		/*16*/
	FMT_SOF_8_S_VDO_AUX_END,		/*17*/
	FMT_SOF_9_S_FE_FIFO_STA,		/*18*/
	FMT_SOF_9_S_FE_FIFO_END,		/*19*/
	FMT_SOF_10_S_FILM_GRAIN_0_STA,	/*20*/
	FMT_SOF_10_S_FILM_GRAIN_0_END,	/*21*/
	FMT_SOF_11_S_FILM_GRAIN_1_STA,	/*22*/
	FMT_SOF_11_S_FILM_GRAIN_1_END,	/*23*/
	FMT_SOF_12_S_FILM_GRAIN_2_STA,	/*24*/
	FMT_SOF_12_S_FILM_GRAIN_2_END,	/*25*/
	FMT_SOF_13_S_DOLBY_FE_STA,		/*26*/
	FMT_SOF_13_S_DOLBY_FE_END,		/*27*/
	FMT_SOF_14_UHD_OSD_STA,			/*28*/
	FMT_SOF_14_UHD_OSD_END,			/*29*/
	FMT_SOF_15_UHD_FE_FIFO_STA,		/*30*/
	FMT_SOF_15_UHD_FE_FIFO_END,		/*31*/
	FMT_SOF_16_UHD_DOLBY_FE_STA,	/*32*/
	FMT_SOF_16_UHD_DOLBY_FE_END,	/*33*/
	FMT_SOF_17_FHD_OSD_STA,			/*34*/
	FMT_SOF_17_FHD_OSD_END,			/*35*/
	FMT_SOF_18_FHD_FE_FIFO_STA,		/*36*/
	FMT_SOF_18_FHD_FE_FIFO_END,		/*37*/
	FMT_SOF_19_FHD_DOLBY_FE_STA,	/*38*/
	FMT_SOF_19_FHD_DOLBY_FE_END,	/*39*/
	FMT_SOF_20_DISP_MIX_STA,		/*40*/
	FMT_SOF_20_DISP_MIX_END,		/*41*/
	FMT_SOF_21_DOLBY_BE_STA,		/*42*/
	FMT_SOF_21_DOLBY_BE_END,		/*43*/
	FMT_SOF_22_BE_FIFO_STA,			/*44*/
	FMT_SOF_22_BE_FIFO_END,			/*45*/
	FMT_SOF_23_DISP_MUNULOAD_STA,	/*46*/
	FMT_SOF_23_DISP_MUNULOAD_END,	/*47*/
	FMT_SOF_24_MMSYS_MUNULOAD_STA,	/*48*/
	FMT_SOF_24_MMSYS_MUNULOAD_END,	/*49*/
	FMT_SOF_25_VIDEO_MARK_STA,		/*50*/
	FMT_SOF_25_VIDEO_MARK_END,		/*51*/
	FMT_SOF_26_R2R_STA,				/*52*/
	FMT_SOF_26_R2R_END,				/*53*/
	FMT_SOF_27_M_FILM_GRAIN_3_STA,	/*54*/
	FMT_SOF_27_M_FILM_GRAIN_3_END,	/*55*/
	FMT_SOF_28_M_FILM_GRAIN_4_STA,	/*56*/
	FMT_SOF_28_M_FILM_GRAIN_4_END,	/*57*/
	FMT_SOF_29_S_FILM_GRAIN_3_STA,	/*58*/
	FMT_SOF_29_S_FILM_GRAIN_3_END,	/*59*/
	FMT_SOF_30_S_FILM_GRAIN_4_STA,	/*60*/
	FMT_SOF_30_S_FILM_GRAIN_4_END,	/*61*/
	FMT_SOF_31_STA,					/*62*/
	FMT_SOF_31_END,					/*63*/
	FMT_SOF_NUM
};

struct fmt_hd_scl_info {
	bool hd_scl_on;
	int src_w;
	int out_w;
	int in_x_pos;
	int out_x_offset;
	int out_y_odd_pos;
	int out_y_odd_pos_e;
	int out_y_even_pos;
	int out_y_even_pos_e;
	int dispfmt_out_h_start;
	int dispfmt_out_h_end;
	int vdout_out_h_start;
	int vdout_out_h_end;
	enum HDMI_VIDEO_RESOLUTION res;
};

enum DSD_CASE_E {
	DSD_4K_TO_480P,
	DSD_4K_TO_720P,
	DSD_4K_TO_1080P,
	DSD_1080P_TO_480P,
	DSD_1080P_TO_720P,
	DSD_720P_TO_480P,
	DSD_NONE
};

struct fmt_dsd_scl_info {
	bool fg_dsd_scl_on;
	enum DSD_CASE_E dsd_case;
	int src_w;
	int src_h;
	int src_x;
	int src_y;
	int out_w;
	int out_h;
	int out_x;
	int out_y;
	int active_start;
	int active_end;
	int original_active_start;
	int out_h_start; /*return to user*/
	int out_h_end;   /*return to user*/
	enum HDMI_VIDEO_RESOLUTION res;
};

int fmt_hal_set_sof(enum FMT_SOF_TYPE type_start, enum FMT_SOF_TYPE type_end,
	int sof_start, int sof_end);

int fmt_hal_sof_grp0_shadow_enable(bool enable);
void fmt_hal_sof_grp1_shadow_update(void);
int fmt_hal_sof_grp1_shadow_enable(bool enable);
void fmt_hal_sof_grp1_shadow_update(void);


/************************************************************************
 *    Function : fmt_hal_enable(uint32_t fmt_id, bool enable)
 *    Description : enable vdout/disp fmt
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                enable: true-enable, false-disable
 *    Return      : none
 ***********************************************************************
 */
void fmt_hal_enable(uint32_t fmt_id, bool enable);

/************************************************************************
 *    Function : fmt_hal_get_dispfmt_reg(uint32_t fmt_id);
 *    Description : get disp fmt parameter
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *    Return      : The pointer of disp fmt
 ************************************************************************/
struct disp_fmt_t *fmt_hal_get_dispfmt_reg(uint32_t fmt_id);

/************************************************************************
 *    Function : fmt_hal_get_vdoutfmt_reg(uint32_t fmt_id);
 *    Description : get vdout fmt parameter
 *                fmt_id: 2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *    Return      : The pointer of vdout fmt
 ************************************************************************/
struct vdout_fmt_t *fmt_hal_get_vdoutfmt_reg(uint32_t fmt_id);

/************************************************************************
 *    Function : fmt_hal_set_mode(uint32_t fmt_id,
 *                    enum HDMI_VIDEO_RESOLUTION resolution, bool config_hw)
 *    Description : set vdout/disp fmt timing
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                resolution: the tv resolution
 *                config_hw: if lk is ready , no need to set timing again when
 *normal boot
 *    Return      : none
 ************************************************************************/
void fmt_hal_set_mode(uint32_t fmt_id, enum HDMI_VIDEO_RESOLUTION resolution,
		      bool config_hw);

/************************************************************************
 *    Function : fmt_hal_hw_shadow_enable(uint32_t fmt_id)
 *    Description : enable shadow register
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *    Return      : none
 ************************************************************************/
void fmt_hal_hw_shadow_enable(uint32_t fmt_id);

/************************************************************************
 *    Function : fmt_hal_update_hw_register(void)
 *    Description : update vdout/disp fmt register
 *    Return      : none
 ************************************************************************/
void fmt_hal_update_hw_register(void);

/************************************************************************
 *    Function : fmt_hal_reset(fmt_id)
 *    Description : reset vdout/disp fmt hw
 *    Return      : none
 ************************************************************************/
void fmt_hal_reset(uint32_t fmt_id);

/************************************************************************
 *    Function : fmt_hal_set_pllgp_hdmidds(void)
 *    Description : set hdmi pll
 *    Return      : none
 ************************************************************************/
void fmt_hal_set_pllgp_hdmidds(uint32_t eRes, bool en, bool set_pll,
			       bool fractional);

/************************************************************************
 *    Function : fmt_hal_set_tv_type(uint32_t fmt_id, uint32_t tv_type)
 *    Description : set tv type
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                tv_type: the tv type
 *    Return      : none
 ************************************************************************/
void fmt_hal_set_tv_type(uint32_t fmt_id, uint32_t tv_type);

/************************************************************************
 *    Function : fmt_hal_plane_is_mix(uint32_t plane)
 *    Description : if the plane is mix or not
 *                plane: 0-plane1(main vdo) 1-plane2(sub video) 2-plane3(osd)
 *    Return      : none
 ************************************************************************/
bool fmt_hal_plane_is_mix(uint32_t plane);

/************************************************************************
 *    Function : fmt_hal_set_active_zone_by_res(...)
 *    Description : set active zone ioformation by resolution
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                resolution: resolution of tv
 *                width: h active
 *                height: v active
 *    Return      : int
 ************************************************************************/
int32_t fmt_hal_set_active_zone_by_res(uint32_t fmt_id,
				       enum HDMI_VIDEO_RESOLUTION resolution,
				       uint16_t width, uint16_t height);

/************************************************************************
 *    Function : fmt_hal_set_active_zone(uint32_t fmt_id, struct fmt_active_info
 **active_info);
 *    Description : set active zone ioformation
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                active_info: active information(h active,v active,htotal,
 *vtotal, etc..)
 *    Return      : int
 ************************************************************************/
int32_t fmt_hal_set_active_zone(uint32_t fmt_id,
				struct fmt_active_info *active_info);

/************************************************************************
 *    Function : fmt_hal_set_delay_by_res(uint32_t fmt_id,
 *          enum HDMI_VIDEO_RESOLUTION£¬ resolution);
 *    Description : set delay information via resolution
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                resolution: tv resolution
 *    Return      : int
 ************************************************************************/
int32_t fmt_hal_set_delay_by_res(uint32_t fmt_id,
				 enum HDMI_VIDEO_RESOLUTION resolution);

/************************************************************************
 *    Function : fmt_hal_set_delay(...);
 *    Description : set delay information
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                adj_f: adjustment forward
 *                h_delay: horizontal shift of output vsync
 *                v_delay: vertical shift of output hsync
 *    Return      : int
 ************************************************************************/
int32_t fmt_hal_set_delay(uint32_t fmt_id, uint32_t adj_f, uint32_t h_delay,
			  uint32_t v_delay);

/************************************************************************
 *    Function : fmt_hal_set_pixel_factor(...);
 *    Description : set pixel length and horizontal scaling factor
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                pixel_len: pixel length
 *                hsfactor: horizontal scaling factor
 *                enable: horizontal scaling enable
 *    Return      : int
 ************************************************************************/
int32_t fmt_hal_set_pixel_factor(uint32_t fmt_id, uint32_t pixel_len,
				 uint32_t hsfactor, uint32_t enable);

int32_t fmt_hal_dsd_set_mode(uint32_t fmt_id, enum DSD_CASE_E dsd_case,
			     int h_total, int v_total);
int32_t fmt_hal_dsd_enable(uint32_t fmt_id, struct fmt_dsd_scl_info *info);
int32_t fmt_hal_dsd_set_delay(uint32_t fmt_id,
			      enum HDMI_VIDEO_RESOLUTION resolution);
int32_t fmt_hal_h_down_enable(uint32_t fmt_id, struct fmt_hd_scl_info *info);
int32_t fmt_hal_h_down_disable(uint32_t fmt_id);
int32_t fmt_hal_h_down_cap(uint32_t fmt_id, uint32_t src_w, uint32_t out_w,
			   uint32_t out_h);
int32_t fmt_hal_set_output_444(uint32_t fmt_id, bool is_444);
int32_t fmt_hal_set_uv_swap(uint32_t fmt_id, bool swap);
int32_t fmt_hal_set_secure(uint32_t fmt_id, bool is_secure);
char *fmt_hal_get_sw_register(uint32_t fmt_id, int *size, uint64_t **reg_mode);
int32_t fmt_hal_show_colorbar(bool en);
int32_t fmt_hal_set_dummy(uint32_t fmt_id, bool en);
int32_t fmt_hal_set_hdownscl_hv(uint32_t fmt_id,
	struct fmt_hd_scl_info *info);
int32_t fmt_hal_set_pixel_len(uint32_t fmt_id, uint32_t pixel_len);

#ifdef DISP_GCE_SUPPORT
void fmt_hal_config_sec_register(uint32_t fmt_id, struct cmdq_pkt *pkt);
#endif
/************************************************************************
 *    Function : fmt_hal_shadow_update(void);
 *    Description : shadow update
 *    Return      : none
 ************************************************************************/
void fmt_hal_shadow_update(void);

/************************************************************************
 *    Function : fmt_hal_clock_on_off(uint32_t fmt_id, bool on);
 *    Description : clock on/off fmt
 *                fmt_id: 0-DISP_FMT_MAIN, 1-DISP_FMT_SUB
 *                        2-VDOUT_FMT, 3-VDOUT_FMT_SUB
 *                on: enable or disable
 *    Return      : int
 ************************************************************************/
int32_t fmt_hal_clock_on_off(uint32_t fmt_id, bool on);

/************************************************************************
 *    Function : fmt_hal_init(struct fmt_init_pararm *param);
 *    Description : initialization fmt
 *                param: parameter
 *    Return      : int
 ************************************************************************/
int32_t fmt_hal_init(struct fmt_init_pararm *param);

/************************************************************************
 *    Function : fmt_hal_deinit(void);
 *    Description : de-initialization fmt
 *    Return      : int
 ************************************************************************/
int32_t fmt_hal_deinit(void);

#endif
