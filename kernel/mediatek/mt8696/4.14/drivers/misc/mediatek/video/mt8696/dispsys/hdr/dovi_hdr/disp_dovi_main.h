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

#ifndef __DISP_DOVI_MAIN_H__
#define __DISP_DOVI_MAIN_H__

#include "disp_hdr_if.h"
#define DOVI_DTS_ND_MAX_SIZE 50

#define DOVI_DRV_NAME  "disp_drv_dovi"

#include "disp_dovi_common_if.h"

enum dovi_core_id {
	DOVI_MVDO_FE = 0,
	DOVI_SVDO_FE = 1,
	DOVI_FHD_FE = 2,
	DOVI_UHD_FE = 3,
	DOVI_VDO_BE = 4,
	DOVI_CORE_MAX = 5,
};


#define CHK_CORE_ID(core_id) \
(((core_id) >= DOVI_MVDO_FE) || ((core_id) <= DOVI_VDO_BE))

extern unsigned int g_force_dolby;
extern unsigned int g_hdr_type;
extern unsigned int g_out_format;
extern uint32_t ui_force_hdr_type;
extern bool dump_rpu_enable;
extern uint32_t dovi_idk_test;
extern enum VIDEO_BIT_MODE idk_dump_bpp;
extern uint32_t *graphic_idk_load_addr_va;
extern dma_addr_t graphic_idk_dump_load_pa;
extern uint32_t *graphic_header_idk_load_addr_va;
extern dma_addr_t graphic_header_idk_dump_load_pa;
extern uint32_t dovi_vs10_path_en;
extern uint32_t dovi_enable;
extern uint32_t dv_gfx_fe_en[2];
extern uint32_t dv_gfx_fe_en[2];
extern uint32_t dolby_path_enable;
extern struct disp_hw_common_info dovi_common_info;
extern bool tv_info_set_by_cmd;
extern unsigned char dv_std_tv_vsvdb[];
extern unsigned char dv_ll_tv_vsvdb[];
extern struct mtk_disp_hdr_md_info_t dovi_hdr_md_info[V_G_LAYER_MAX];
extern struct dv_hw_reg *p_dv_out_params;
extern uint32_t dv_gfx_fe_en[2];
extern uint32_t dv_vdo_fe_en[2];
extern uint32_t dv_vdo_be_en;
extern uint32_t dovi_out_height;
extern uint32_t cur_ml_cfg_st[4];
extern uint32_t adl_mode;
extern bool dovi_black_pattern_en;
extern uint32_t dovi_black_pattern_cnt;
extern uint32_t dovi_black_pattern_cnt_max;

int disp_dovi_resolution_change(const struct disp_hw_resolution *info);

extern int disp_dovi_common_init(void);
extern enum dovi_status dovi_sec_init(void);
extern int disp_dovi_process(uint32_t enable,
	struct mtk_disp_hdr_md_info_t *hdr_metadata);
extern uint32_t dovi_set_out_res(uint32_t out_res,
	uint16_t width, uint16_t height);
extern uint32_t disp_dovi_set_idk_info(void);
extern uint32_t disp_dovi_set_sdk_info(void);
int disp_dovi_sdk_handle(uint32_t enable);
void disp_dovi_set_osd_clk_enable(bool enable);
uint32_t disp_dovi_set_osd_showdoblylogo(void);
void disp_dovi_set_graphic_header(uint32_t *va,
	dma_addr_t graphic_pa);
extern void vDolbyHdrEnable(bool fgEnable);
extern void vHdrEnable(bool fgEnable);
extern void vSetStaticHdrType(char bType);
int disp_dovi_show_res_status(enum HDMI_VIDEO_RESOLUTION res);
int disp_dovi_show_tv_cap(struct disp_hw_tv_capbility *cap);
void disp_dovi_set_osd_all_black(void);
void disp_dovi_set_osd_active_zone(void);
void disp_dovi_isr(void);
int disp_dovi_init(struct disp_hw_common_info *info);
int disp_dovi_suspend(void);
int disp_dovi_resume(void);
int disp_dovi_deinit(void);
int disp_dovi_irq_handler(uint32_t irq);
int disp_dovi_process_cmd(uint32_t id,
	enum DISP_CMD cmd,
	void *data);
void disp_dovi_init_sec_by_cmd(void);
void disp_hdr_set_tv_info(uint32_t tv_type);
void disp_hdr_set_config(uint32_t layer_id, uint32_t rpu_id);
extern void vdp_trigger_metadata_config(uint32_t id,
	struct video_buffer_info *curbuf,
	struct video_buffer_info *nextbuf);
extern void disp_fefifo_drv_set_input_order(uint32_t layer_id,
	uint32_t order);
void disp_hdr_trigger_vdp(uint32_t id);
int disp_dovi_force_gfx_vs10(void);
void disp_dovi_set_hdr_fe_size(uint32_t layer_id, uint32_t width,
	uint32_t heigh);
#endif
