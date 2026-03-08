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

#ifndef __DISP_PATH_H__
#define __DISP_PATH_H__

#include "hdmitx.h"
#include <linux/device.h>
#include <linux/types.h>

#define FULL_SCREEN_IDX 2 /*0-default,  1-dovi*/
#define DISP_PATH_SHADOW_ENABLE

struct full_screen_param {
	int h_start;
	int v_odd_start;
	int v_even_start;
	int sof_start;
	int sof_end;
	enum HDMI_VIDEO_RESOLUTION res;
};

enum DISP_SOF_TYPE {
	DISP_SOF_0_M_VDO_MAIN_STA,
	DISP_SOF_0_M_VDO_MAIN_END,
	DISP_SOF_1_M_VDO_AUX_STA,
	DISP_SOF_1_M_VDO_AUX_END,
	DISP_SOF_2_M_FE_FIFO_STA,
	DISP_SOF_2_M_FE_FIFO_END,
	DISP_SOF_3_STA,
	DISP_SOF_3_END,
	DISP_SOF_4_M_FILM_GRAIN_1_STA,
	DISP_SOF_4_M_FILM_GRAIN_1_END,
	DISP_SOF_5_M_FILM_GRAIN_2_STA,
	DISP_SOF_5_M_FILM_GRAIN_2_END,
	DISP_SOF_6_M_HDR_FE_STA,
	DISP_SOF_6_M_HDR_FE_END,
	DISP_SOF_7_S_VDO_MAIN_STA,
	DISP_SOF_7_S_VDO_MAIN_END,
	DISP_SOF_8_S_VDO_AUX_STA,
	DISP_SOF_8_S_VDO_AUX_END,
	DISP_SOF_9_S_FE_FIFO_STA,
	DISP_SOF_9_S_FE_FIFO_END,
	DISP_SOF_10_S_FILM_GRAIN_0_STA,
	DISP_SOF_10_S_FILM_GRAIN_0_END,
	DISP_SOF_11_S_FILM_GRAIN_1_STA,
	DISP_SOF_11_S_FILM_GRAIN_1_END,
	DISP_SOF_12_S_FILM_GRAIN_2_STA,
	DISP_SOF_12_S_FILM_GRAIN_2_END,
	DISP_SOF_13_S_HDR_FE_STA,
	DISP_SOF_13_S_HDR_FE_END,
	DISP_SOF_14_UHD_OSD_STA,
	DISP_SOF_14_UHD_OSD_END,
	DISP_SOF_15_UHD_FE_FIFO_STA,
	DISP_SOF_15_UHD_FE_FIFO_END,
	DISP_SOF_16_UHD_HDR_FE_STA,
	DISP_SOF_16_UHD_HDR_FE_END,
	DISP_SOF_17_FHD_OSD_STA,
	DISP_SOF_17_FHD_OSD_END,
	DISP_SOF_18_FHD_FE_FIFO_STA,
	DISP_SOF_18_FHD_FE_FIFO_END,
	DISP_SOF_19_FHD_HDR_FE_STA,
	DISP_SOF_19_FHD_HDR_FE_END,
	DISP_SOF_20_DISP_MIX_STA,
	DISP_SOF_20_DISP_MIX_END,
	DISP_SOF_21_HDR_BE_STA,
	DISP_SOF_21_HDR_BE_END,
	DISP_SOF_22_BE_FIFO_STA,
	DISP_SOF_22_BE_FIFO_END,
	DISP_SOF_23_DISP_MUNULOAD_STA,
	DISP_SOF_23_DISP_MUNULOAD_END,
	DISP_SOF_24_MMSYS_MUNULOAD_STA,
	DISP_SOF_24_MMSYS_MUNULOAD_END,
	DISP_SOF_25_VIDEO_MARK_STA,
	DISP_SOF_25_VIDEO_MARK_END,
	DISP_SOF_26_R2R_STA,
	DISP_SOF_26_R2R_END,
	DISP_SOF_27_M_FILM_GRAIN_3_STA,
	DISP_SOF_27_M_FILM_GRAIN_3_END,
	DISP_SOF_28_M_FILM_GRAIN_4_STA,
	DISP_SOF_28_M_FILM_GRAIN_4_END,
	DISP_SOF_29_S_FILM_GRAIN_3_STA,
	DISP_SOF_29_S_FILM_GRAIN_3_END,
	DISP_SOF_30_S_FILM_GRAIN_4_STA,
	DISP_SOF_30_S_FILM_GRAIN_4_END,
	DISP_SOF_31_STA,
	DISP_SOF_31_END,
	DISP_SOF_NUM
};


struct sof_param {
	int sof_start;
	int sof_end;
	enum DISP_SOF_TYPE start_type;
	enum DISP_SOF_TYPE end_type;
};


enum DISP_PATH_HW_ID {
	DISP_PATH_M_VDO,		/*0*/
	DISP_PATH_M_VDO_OUT,	/*1*/
	DISP_PATH_M_VDO_FE_FIFO,/*2*/
	DISP_PATH_M_FILM_GRAIN,	/*3*/
	DISP_PATH_M_HDR_VDO_FE,	/*4*/
	DISP_PATH_S_VDO,		/*5*/
	DISP_PATH_S_VDO_OUT,	/*6*/
	DISP_PATH_S_VDO_FE_FIFO,/*7*/
	DISP_PATH_S_FILM_GRAIN,	/*8*/
	DISP_PATH_S_HDR_VDO_FE,	/*9*/
	DISP_PATH_FHD_OSD,		/*10*/
	DISP_PATH_FHD_GFX_FE_FIFO,/*11*/
	DISP_PATH_FHD_HDR_GFX_FE,/*12*/
	DISP_PATH_UHD_OSD,		/*13*/
	DISP_PATH_UHD_GFX_FE_FIFO,/*14*/
	DISP_PATH_UHD_HDR_GFX_FE,/*15*/
	DISP_PATH_DISP_MIX,		/*16*/
	DISP_PATH_DISP_HDR_VDO_BE,/*17*/
	DISP_PATH_DISP_HDR_VDO_BE_FIFO,/*18*/
	DISP_PATH_DISP_DISPSYS_MUNULOAD,/*19*/
	DISP_PATH_DISP_MMSYS_MENULOAD,	/*20*/
	DISP_PATH_DISP_R2R,				/*21*/
	DISP_PATH_DISP_DGI,				/*22*/
	DISP_PATH_DISP_P2I,				/*23*/
	DISP_PATH_DISP_ENUM
};

struct disp_path_resolution {
	uint16_t htotal;
	uint16_t vtotal;
	uint16_t width;
	uint16_t height;
	uint16_t frequency;
	bool is_progressive;
	bool is_hd;
	enum HDMI_VIDEO_RESOLUTION res;
};

struct disp_path_context {
	struct disp_path_resolution resolution;
	struct full_screen_param fs_param;
	bool main_path_dolby_enable;
	bool sub_path_dolby_enable;
	bool fhd_gfx_dolby_enable;
	bool uhd_gfx_dolby_enable;
};
int disp_path_get_sof_info(enum DISP_SOF_TYPE start_type,
	enum DISP_SOF_TYPE end_type, int *sof_start, int *sof_end);

int disp_path_set_hw_path(enum DISP_PATH_HW_ID id, bool enable);


int disp_path_get_active_zone(enum DISP_PATH_HW_ID id,
			      enum HDMI_VIDEO_RESOLUTION res, int *h_start,
			      int *v_odd_start, int *v_even_start);

int disp_path_set_res(struct disp_path_resolution *resolution);

int disp_path_reset(void);

int disp_path_init(struct disp_path_resolution *resolution);

void set_fs_index(uint32_t layer, uint32_t idx);

extern unsigned int g_hdmi_res;
extern unsigned int g_force_dovi;
extern unsigned int g_hdr_type;
extern unsigned int g_out_format;
extern struct disp_path_context disp_path;

#endif
