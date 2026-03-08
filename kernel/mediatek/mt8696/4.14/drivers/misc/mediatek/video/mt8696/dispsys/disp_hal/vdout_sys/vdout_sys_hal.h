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

#ifndef _VDOUT_HAL_H_
#define _VDOUT_HAL_H_

#include <linux/printk.h>
#include <linux/types.h>

#include "disp_reg.h"
#include "vdout_sys_hw.h"
#include "hdmitx.h"

extern struct vdout_context_t vdout;

/* temp define, it will be modified */
#define VDOUT_SYS_BASE vdout.init_param.sys_reg_base
#define MM_SYS_BASE		vdout.init_param.mmsys_reg_base

/**********************************************************************
 *  VDOUT System & Clock Macros
 *********************************************************************/

#define vWriteVDOUT(dAddr, dVal) WriteREG32((VDOUT_SYS_BASE + dAddr), dVal)
#define dReadVDOUT(dAddr) ReadREG32(VDOUT_SYS_BASE + dAddr)
#define vWriteVDOUTMsk(dAddr, dVal, dMsk)                                      \
	vWriteVDOUT((dAddr),                                                   \
		    (dReadVDOUT(dAddr) & (~(dMsk))) | ((dVal) & (dMsk)))

#define vWriteMMSYS(dAddr, dVal)  WriteREG32((MM_SYS_BASE + dAddr), dVal)
#define dReadMMSYS(dAddr)         ReadREG32(MM_SYS_BASE + dAddr)
#define vWriteMMSYSMsk(dAddr, dVal, dMsk)		\
	vWriteMMSYS((dAddr),						\
		(dReadMMSYS(dAddr) & (~(dMsk))) | ((dVal) & (dMsk)))

struct vdout_init_param {
	uintptr_t sys_reg_base;
	uintptr_t mmsys_reg_base;
};

struct vdout_context_t {
	bool inited;
	struct vdout_init_param init_param;
};

enum VDOUT_SYS_IRQ_BIT {
	VDOUT_SYS_IRQ_OSD3_FRAME_END = 0x1 << 0,
	VDOUT_SYS_IRQ_OSD3_FRAME_START = 0x1 << 1,
	VDOUT_SYS_IRQ_OSD3_VSYNC = 0x1 << 4,
	VDOUT_SYS_IRQ_FMT_ACTIVE_START = 0x1 << 7,
	VDOUT_SYS_IRQ_FMT_ACTIVE_END = 0x1 << 8,
	VDOUT_SYS_IRQ_FMT_VSYNC = 0x1 << 9,
	VDOUT_SYS_IRQ_BEFIFO_ERR = 0x1 << 10,
	VDOUT_SYS_IRQ_BEFIFO = 0x1 << 11,
	VDOUT_SYS_IRQ_FEFIFO_FHD = 0x1 << 12,
	VDOUT_SYS_IRQ_FEFIFO_UHD = 0x1 << 13,
};

enum DISP_PATH {
	DISP_PATH_MAIN_VDO,
	DISP_PATH_SUB_VDO,
	DISP_PATH_OSD_FHD,
	DISP_PATH_OSD_UHD
};

enum DISP_MIX_INPUT_SRC {
	MIX_IN_SRC_ARGB,
	MIX_IN_SRC_ARBG,
	MIX_IN_SRC_AGRB,
	MIX_IN_SRC_AGBR,
	MIX_IN_SRC_ABRG,
	MIX_IN_SRC_ABGR
};

enum BE_DATA_REORDER {
	COLOR_FORMAT_VYU = 0,
	COLOR_FORMAT_VUY,
	COLOR_FORMAT_YUV,
	COLOR_FORMAT_YVU,
	COLOR_FORMAT_UYV,
	COLOR_FORMAT_UVY
};


enum VIDEOIN_SRC_SEL {
	VIDEOIN_SRC_SEL_VDO_BE_FIFO_OUTPUT2 = 0x0 << 16,
	VIDEOIN_SRC_SEL_MVDO_FE_FIFO_INPUT = 0x1 << 16,
	VIDEOIN_SRC_SEL_SVDO_FE_FIFO_INPUT = 0x2 << 16,
	VIDEOIN_SRC_SEL_FHD_FE_FIFO_INPUT = 0x3 << 16,
	VIDEOIN_SRC_SEL_UHD_FE_FIFO_INPUT = 0x4 << 16,
	VIDEOIN_SRC_SEL_VDO_BE_FIFO_OUTPUT = 0x5 << 16,
	VIDEOIN_SRC_SEL_RGB2HDMI_OUTPUT = 0x6 << 16,
	VIDEOIN_SRC_SEL_HDMI_RX_OUTPUT = 0x9 << 16,
	VIDEOIN_SRC_SEL_SD_PPF_OUTPUT = 0xa << 16,
	VIDEOIN_SRC_SEL_P2I_OUTPUT = 0xb << 16,
	VIDEOIN_SRC_SEL_VM_TOP_OUTPUT = 0xc << 16,
};


#define VDOUT_LOG_E(format...) pr_info("[VDOUT]error: " format)
#define VDOUT_LOG_I(format...) pr_info("[VDOUT] " format)
#define VDOUT_LOG_D(format...) pr_debug("[VDOUT] " format)

/************************************************************************
 *    Function : void vdout_sys_hal_4k2k_clock_enable(uint32_t res)
 *    Description : set clock mux for 4k resolution
 *                res: HDMI output resolution
 *    Return      : None
 ************************************************************************/
void vdout_sys_hal_4k2k_clock_enable(uint32_t res);

/************************************************************************
 *    Function : void vdout_sys_hal_set_hdmi(uint32_t res)
 *    Description : Setting HDMI vdout data path and clock Mux
 *                res: HDMI output resolution
 *    Return      : None
 ************************************************************************/
void vdout_sys_hal_set_hdmi(enum HDMI_VIDEO_RESOLUTION res);

/************************************************************************
 *    Function : void vdout_sys_hal_videoin_source_sel(enum VIDEOIN_SRC_SEL
 *src_point)
 *    Description : select video-in source point
 *                src_point: source point of video-in
 *    Return      : None
 ************************************************************************/
void vdout_sys_hal_videoin_source_sel(enum VIDEOIN_SRC_SEL src_point);

void vdout_sys_hal_fhd_hdr_gfx_fe_enable(bool enable);

void vdout_sys_hal_uhd_hdr_gfx_fe_enable(bool enable);

void vdout_sys_hal_4_layers_hdr_gfx2_mix_sout_select(void);

void vdout_sys_hal_hdr_vdo_be_enable(bool enable);

void vdout_sys_p2i_input_mux_src_select(void);

void vdout_sys_vm_input_mux_src_select(bool is_interlaced);

void vdout_sys_vm_output_mux_src_select(bool is_interlaced);

void vdout_sys_rgb2hdmi_input_mux_src_select(bool is_interlaced);

void vdout_sys_rgb2hdmi_input_timing_select(bool is_interlaced);


void vdout_sys_hal_configure_video_layer_alpha
	(enum DISP_PATH type, UINT32 value);

void vdout_sys_hal_disp_mix_input_chanel_swap(enum DISP_PATH type,
		enum DISP_MIX_INPUT_SRC input_src);

void vdout_sys_hal_reorder_before_mmsys_mix_out(enum BE_DATA_REORDER data_type,
	bool need_reorder);

/************************************************************************
 *    Function : vdout_sys_hal_clock_on_off(bool en)
 *    Description : clock on/off
 *                en: enable or nor
 *    Return      : none
 ************************************************************************/
void vdout_sys_hal_clock_on_off(bool en);
void vdout_sys_hal_select_dsd_clk(bool en);
void vdout_sys_clear_irq(enum VDOUT_SYS_IRQ_BIT irq);
void vdout_sys_clear_irq_all(void);
void vdout_sys_422_to_420(bool en);
void vdout_sys_hal_set_new_sd_sel(bool en);
void vdout_sys_hal_shadow_en(bool en);
void vdout_sys_hal_shadow_update(void);
int vdout_sys_hal_init(struct vdout_init_param *param);
void vdout_sys_hal_idk_set(bool en, uint16_t htotal);

#endif
