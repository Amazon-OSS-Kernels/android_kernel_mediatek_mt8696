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

#define LOG_TAG "FEFIFO_DRV"

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include "disp_hw_log.h"
#include "disp_hw_mgr.h"
#include "disp_info.h"
#include "disp_clk.h"
#include "disp_irq.h"
#include "disp_path.h"
#include "fmt_def.h"
#include "fmt_hal.h"
#include "disp_fefifo_debug.h"
#include "disp_fefifo_drv.h"
#include "disp_fefifo_hal.h"
#include "disp_fefifo_hw.h"



struct disp_fefifo_context _fefifo_inst[FEFIFO_MAX_LAYER_NUM];
unsigned int fefifo_dbg_level;


struct disp_fefifo_context *fefifo_get_inst(uint32_t layer_id)
{
	return &_fefifo_inst[layer_id];
}

static void _fefifo_mutex_init(uint32_t layer_id)
{
	mutex_init(&(_fefifo_inst[layer_id].lock));
}

static void _fefifo_mutex_lock(uint32_t layer_id)
{
	mutex_lock(&(_fefifo_inst[layer_id].lock));
}

static void _fefifo_mutex_unlock(uint32_t layer_id)
{
	mutex_unlock(&(_fefifo_inst[layer_id].lock));
}

void disp_fefifo_drv_init(uint32_t layer_id, uintptr_t reg_base)
{
	memset((void *)&(_fefifo_inst[layer_id]), 0,
		sizeof(struct disp_fefifo_context));
	_fefifo_mutex_init(layer_id);
	_fefifo_inst[layer_id].fefifo_hw_base = reg_base;
	_fefifo_inst[layer_id].reg_conf_mode = FEFIFO_REG_CONF_CLIENT_RIU;
	if (layer_id == 0)
		_fefifo_inst[layer_id].io_reg_base =
		DISP_FEFIFO_VDOFE0_REG_BASE;
	else if (layer_id == 1)
		_fefifo_inst[layer_id].io_reg_base =
		DISP_FEFIFO_VDOFE1_REG_BASE;
	else if (layer_id == 2)
		_fefifo_inst[layer_id].io_reg_base =
		DISP_FEFIFO_GFXFE0_REG_BASE;
	else if (layer_id == 3)
		_fefifo_inst[layer_id].io_reg_base =
		DISP_FEFIFO_GFXFE1_REG_BASE;

	fefifo_hal_init(layer_id, reg_base);
	_fefifo_inst[layer_id].inited = true;
}

void disp_fefifo_drv_uninit(uint32_t layer_id)
{
	_fefifo_mutex_lock(layer_id);
	fefifo_hal_uninit(layer_id);
	memset((void *)&(_fefifo_inst[layer_id]), 0,
		sizeof(struct disp_fefifo_context));
	_fefifo_mutex_unlock(layer_id);
}

int disp_fefifo_drv_set_gce_handle(uint32_t layer_id,
	void *pv_handle)
{
	if (!_fefifo_inst[layer_id].inited)
		return FEFIFO_RET_UNINIT;
	if (pv_handle == NULL)
		return FEFIFO_RET_INV_ARG;
	_fefifo_inst[layer_id].gce_handle = NULL;
	return FEFIFO_RET_OK;
}

int disp_fefifo_set_clk_enable(uint32_t layer_id, uint32_t enable)
{
	if (enable) {
		if (_fefifo_inst[layer_id].clk_on)
			return FEFIFO_RET_OK;

		if (layer_id == 0)
			disp_clock_enable(DISP_CLK_M_VDO_FE_FIFO, true);
		else if (layer_id == 1)
			disp_clock_enable(DISP_CLK_S_VDO_FE_FIFO, true);
		else if (layer_id == 2)
			disp_clock_enable(DISP_CLK_FHD_GFX_FE_FIFO, true);
		else if (layer_id == 3)
			disp_clock_enable(DISP_CLK_UHD_GFX_FE_FIFO, true);

		_fefifo_inst[layer_id].clk_on = true;

	} else {
		if (_fefifo_inst[layer_id].clk_on == false)
			return FEFIFO_RET_OK;

		if (layer_id == 0)
			disp_clock_enable(DISP_CLK_M_VDO_FE_FIFO, false);
		else if (layer_id == 1)
			disp_clock_enable(DISP_CLK_S_VDO_FE_FIFO, false);
		else if (layer_id == 2)
			disp_clock_enable(DISP_CLK_FHD_GFX_FE_FIFO, false);
		else if (layer_id == 3)
			disp_clock_enable(DISP_CLK_UHD_GFX_FE_FIFO, false);

		_fefifo_inst[layer_id].clk_on = false;
	}
	return FEFIFO_RET_OK;
}

void disp_fefifo_set_src_res(uint32_t layer_id, uint32_t u4width,
	uint32_t u4height, uint32_t conf_md)
{
	_fefifo_inst[layer_id].src_width = u4width;
	_fefifo_inst[layer_id].src_heght = u4height;
	fefifo_hal_set_src_active(layer_id, u4width, u4height, conf_md);
}

void disp_fefifo_drv_get_src_res(uint32_t layer_id,
	uint32_t *pu4width, uint32_t *pu4height)
{
	if ((pu4width != NULL) && (pu4height != NULL)) {
		*pu4width = _fefifo_inst[layer_id].src_width;
		*pu4height = _fefifo_inst[layer_id].src_heght;
	}
}

int disp_fefifo_drv_set_conf_mode(uint32_t layer_id, uint32_t mode)
{
	_fefifo_inst[layer_id].reg_conf_mode = mode;
	fefifo_default("%s conf mode[%d %d]!\n",
		__func__, layer_id, mode);
	return FEFIFO_RET_OK;
}


int disp_fefifo_map_timing_gen(uint32_t layer_id,
	enum HDMI_VIDEO_RESOLUTION res,
	struct disp_fefifo_timing *pst_timing)
{
	if (pst_timing == NULL)
		return FEFIFO_RET_INV_ARG;

	switch (res) {
	case HDMI_VIDEO_720x480i_60Hz:
		pst_timing->htotal = 0x35A;
		pst_timing->vtotal_lsb = 0x20D;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0x2D0;
		pst_timing->height = 0xF0;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0x7B;
		pst_timing->vfront = 0x13;
		//pst_timing->vfront = 0x20D/2 + 0x13;
		pst_timing->hsync_w = 0x3E;
		pst_timing->vsync_w = 0x06;
		break;

	case HDMI_VIDEO_720x576i_50Hz:
		pst_timing->htotal = 864;
		pst_timing->vtotal_lsb = 625;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 720;
		pst_timing->height = 288;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 133;
		pst_timing->vfront = 23;
		//pst_timing->vfront = 625/2 + 0x23;
		pst_timing->hsync_w = 64;
		pst_timing->vsync_w = 5;
		break;

	case HDMI_VIDEO_720x480p_60Hz:
		pst_timing->htotal = 0x35A;
		pst_timing->vtotal_lsb = 0x20D;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0x2D0;
		pst_timing->height = 0x1E0;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0x7B;
		pst_timing->vfront = 0x25;
		pst_timing->hsync_w = 0x3E;
		pst_timing->vsync_w = 0x06;
		break;

	case HDMI_VIDEO_720x576p_50Hz:
		pst_timing->htotal = 864;
		pst_timing->vtotal_lsb = 625;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 720;
		pst_timing->height = 576;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 133;
		pst_timing->vfront = 45;
		pst_timing->hsync_w = 64;
		pst_timing->vsync_w = 5;
		break;

	case HDMI_VIDEO_1280x720p_60Hz:
	case HDMI_VIDEO_1280x720p_59_94Hz:
		pst_timing->htotal = 0x672;
		pst_timing->vtotal_lsb = 0x2EE;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0x500;
		pst_timing->height = 0x2D0;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0x105;
		pst_timing->vfront = 0x1A;
		pst_timing->hsync_w = 0x28;
		pst_timing->vsync_w = 0x05;
		break;

	case HDMI_VIDEO_1280x720p_50Hz:
		pst_timing->htotal = 1980;
		pst_timing->vtotal_lsb = 750;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 1280;
		pst_timing->height = 720;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 261;
		pst_timing->vfront = 26;
		pst_timing->hsync_w = 40;
		pst_timing->vsync_w = 5;
		break;

	case HDMI_VIDEO_1920x1080p_30Hz:
		pst_timing->htotal = 0x898;
		pst_timing->vtotal_lsb = 0x465;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0x780;
		pst_timing->height = 0x438;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0xE1;
		pst_timing->vfront = 0x2A;
		pst_timing->hsync_w = 0x2C;
		pst_timing->vsync_w = 0x05;
		break;

	case HDMI_VIDEO_1920x1080p_25Hz:
		pst_timing->htotal = 2640;
		pst_timing->vtotal_lsb = 1125;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 1920;
		pst_timing->height = 1080;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 193;
		pst_timing->vfront = 42;
		pst_timing->hsync_w = 44;
		pst_timing->vsync_w = 5;

		break;

	case HDMI_VIDEO_1920x1080p_24Hz:
		pst_timing->htotal = 2750;
		pst_timing->vtotal_lsb = 1125;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 1920;
		pst_timing->height = 1080;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 193;
		pst_timing->vfront = 42;
		pst_timing->hsync_w = 44;
		pst_timing->vsync_w = 5;

		break;

	case HDMI_VIDEO_1920x1080p_23Hz:
		pst_timing->htotal = 2750;
		pst_timing->vtotal_lsb = 1125;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 1920;
		pst_timing->height = 1080;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 193;
		pst_timing->vfront = 42;
		pst_timing->hsync_w = 44;
		pst_timing->vsync_w = 5;
		break;

	case HDMI_VIDEO_1920x1080p_29Hz:
		pst_timing->htotal = 2200;
		pst_timing->vtotal_lsb = 1125;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 1920;
		pst_timing->height = 1080;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 193;
		pst_timing->vfront = 42;
		pst_timing->hsync_w = 44;
		pst_timing->vsync_w = 5;
		break;

	case HDMI_VIDEO_1920x1080p_59_94Hz:
	case HDMI_VIDEO_1920x1080p_60Hz:
		pst_timing->htotal = 0x898;
		pst_timing->vtotal_lsb = 0x465;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0x780;
		pst_timing->height = 0x438;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0xE1;
		pst_timing->vfront = 0x2A;
		pst_timing->hsync_w = 0x2C;
		pst_timing->vsync_w = 0x05;
		break;

	case HDMI_VIDEO_1920x1080p_50Hz:
		pst_timing->htotal = 2640;
		pst_timing->vtotal_lsb = 1125;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 1920;
		pst_timing->height = 1080;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 193;
		pst_timing->vfront = 42;
		pst_timing->hsync_w = 44;
		pst_timing->vsync_w = 5;
		break;

	case HDMI_VIDEO_3840x2160P_23_976HZ:
	case HDMI_VIDEO_3840x2160P_24HZ:
		pst_timing->htotal = 5500;
		pst_timing->vtotal_lsb = 2250;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 3840;
		pst_timing->height = 2160;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 385;
		pst_timing->vfront = 83;
		pst_timing->hsync_w = 88;
		pst_timing->vsync_w = 10;
		break;

	case HDMI_VIDEO_3840x2160P_25HZ:
		pst_timing->htotal = 5280;
		pst_timing->vtotal_lsb = 2250;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 3840;
		pst_timing->height = 2160;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 385;
		pst_timing->vfront = 83;
		pst_timing->hsync_w = 88;
		pst_timing->vsync_w = 10;
		break;

	case HDMI_VIDEO_3840x2160P_29_97HZ:
	case HDMI_VIDEO_3840x2160P_30HZ:
		pst_timing->htotal = 0x1130;
		pst_timing->vtotal_lsb = 0x8CA;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0xF00;
		pst_timing->height = 0x870;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0x181;
		pst_timing->vfront = 0x53;
		pst_timing->hsync_w = 0x58;
		pst_timing->vsync_w = 0x0A;
		break;

	case HDMI_VIDEO_4096x2160P_24HZ:
		pst_timing->htotal = 5500;
		pst_timing->vtotal_lsb = 2250;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 4096;
		pst_timing->height = 2160;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 385;
		pst_timing->vfront = 83;
		pst_timing->hsync_w = 88;
		pst_timing->vsync_w = 10;
		break;

	case HDMI_VIDEO_3840x2160P_60HZ:
		pst_timing->htotal = 0x1130;
		pst_timing->vtotal_lsb = 0x8CA;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0xF00;
		pst_timing->height = 0x870;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0x181;
		pst_timing->vfront = 0x53;
		pst_timing->hsync_w = 0x58;
		pst_timing->vsync_w = 0x0A;
		break;

	case HDMI_VIDEO_3840x2160P_50HZ:
		pst_timing->htotal = 5280;
		pst_timing->vtotal_lsb = 2250;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 3840;
		pst_timing->height = 2160;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 385;
		pst_timing->vfront = 83;
		pst_timing->hsync_w = 88;
		pst_timing->vsync_w = 10;
		break;

	case HDMI_VIDEO_4096x2160P_60HZ:
		pst_timing->htotal = 0x1130;
		pst_timing->vtotal_lsb = 0x8CA;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0x1000;
		pst_timing->height = 0x870;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0xD9;
		pst_timing->vfront = 0x53;
		pst_timing->hsync_w = 0x58;
		pst_timing->vsync_w = 0x0A;
		break;

	case HDMI_VIDEO_4096x2160P_50HZ:
		pst_timing->htotal = 5280;
		pst_timing->vtotal_lsb = 2250;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 4096;
		pst_timing->height = 2160;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 217;
		pst_timing->vfront = 83;
		pst_timing->hsync_w = 88;
		pst_timing->vsync_w = 10;
		break;

	case HDMI_VIDEO_3840x2160P_59_94HZ:
		pst_timing->htotal = 0x1130;
		pst_timing->vtotal_lsb = 0x8CA;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0xF00;
		pst_timing->height = 0x870;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0x181;
		pst_timing->vfront = 0x53;
		pst_timing->hsync_w = 0x58;
		pst_timing->vsync_w = 0x0A;
		break;

	case HDMI_VIDEO_4096x2160P_59_94HZ:
		pst_timing->htotal = 0x1130;
		pst_timing->vtotal_lsb = 0x8CA;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0x1000;
		pst_timing->height = 0x870;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0xD9;
		pst_timing->vfront = 0x53;
		pst_timing->hsync_w = 0x58;
		pst_timing->vsync_w = 0x0A;
		break;

	case HDMI_VIDEO_1920x1080i_60Hz:
		pst_timing->htotal = 0x898;
		pst_timing->vtotal_lsb = 0x465;
		pst_timing->vtotal_msb = 0x0;
		pst_timing->width = 0x780;
		pst_timing->height = 0x21C;
		pst_timing->hoffset = 0x1;
		pst_timing->voffset = 0x1;
		pst_timing->hfront = 0xE1;
		pst_timing->vfront = 0x14;
		//pst_timing->vfront = 0x465/2 + 0x14;
		pst_timing->hsync_w = 0x2C;
		pst_timing->vsync_w = 0x05;
		break;

	case HDMI_VIDEO_1920x1080i_50Hz:
		pst_timing->htotal = 2640;
		pst_timing->vtotal_lsb = 1125;
		pst_timing->vtotal_msb = 0;
		pst_timing->width = 1920;
		pst_timing->height = 540;
		pst_timing->hoffset = 1;
		pst_timing->voffset = 1;
		pst_timing->hfront = 193;
		pst_timing->vfront = 21;
		//pst_timing->vfront = 1125/2 + 21;
		pst_timing->hsync_w = 44;
		pst_timing->vsync_w = 5;
		break;

	case HDMI_VIDEO_1280x720p3d_60Hz:
	case HDMI_VIDEO_1280x720p3d_50Hz:
	case HDMI_VIDEO_1920x1080i3d_60Hz:
	case HDMI_VIDEO_1920x1080i3d_50Hz:
	case HDMI_VIDEO_1920x1080p3d_24Hz:
	case HDMI_VIDEO_1920x1080p3d_23Hz:
		fefifo_default("do not support 3d timing [%d]\n", res);
		break;

	default:
		fefifo_default("do not support not defined timing [%d]\n",
			res);
		break;
	}
	return FEFIFO_RET_OK;
}


void disp_fefifo_drv_set_pattern(uint32_t layer_id, uint32_t res,
	uint32_t enable, uint32_t y_data, uint32_t cb_data, uint32_t cr_data)
{
	struct disp_fefifo_pattern stPattern;
	struct disp_fefifo_timing timing;

	disp_fefifo_map_timing_gen(layer_id, res, &timing);

	stPattern.ypattern = y_data;
	stPattern.cbpattern = cb_data;
	stPattern.crpattern = cr_data;
	stPattern.mode = 1;
	fefifo_hal_set_pattern(layer_id, enable, (void *)&timing,
		(void *)&stPattern);
}

void disp_fefifo_drv_enable(uint32_t layer_id,
	bool fgEn, uint32_t mode)
{
	_fefifo_mutex_lock(layer_id);
	fefifo_hal_set_enable(layer_id, fgEn, mode);
	_fefifo_inst[layer_id].enabled = fgEn;
	_fefifo_mutex_unlock(layer_id);
}

void disp_fefifo_drv_set_shadow(uint32_t layer_id,
	bool en_shadow, bool en_trig)
{
	fefifo_hal_set_shdw(layer_id, en_shadow, en_trig);
}

void disp_fefifo_drv_clr_irq(uint32_t layer_id, uint32_t irq)
{
	fefifo_hal_clr_isr(layer_id, irq);
}

void disp_fefifo_drv_flush_reg(uint32_t layer_id, uint32_t mode)
{
	fefifo_hal_isr(layer_id, mode);
}

void disp_fefifo_drv_set_sof(uint32_t layer_id)
{
	if (layer_id == 0)
		fmt_hal_set_sof(FMT_SOF_2_M_FE_FIFO_STA,
		FMT_SOF_2_M_FE_FIFO_END, 0x00010002, 0x00010003);
	else if (layer_id == 1)
		fmt_hal_set_sof(FMT_SOF_9_S_FE_FIFO_STA,
		FMT_SOF_9_S_FE_FIFO_END, 0x00010002, 0x00010003);
	else if (layer_id == 2)
		fmt_hal_set_sof(FMT_SOF_18_FHD_FE_FIFO_STA,
		FMT_SOF_18_FHD_FE_FIFO_END, 0x00010002, 0x00010003);
	else if (layer_id == 3)
		fmt_hal_set_sof(FMT_SOF_15_UHD_FE_FIFO_STA,
		FMT_SOF_15_UHD_FE_FIFO_END, 0x00010002, 0x00010003);
}

void disp_fefifo_drv_path_sel(uint32_t layer_id)
{

}

void disp_fefifo_drv_set_alpha(uint32_t layer_id, uint32_t alpha)
{
	fefifo_hal_set_alpha(layer_id, alpha);
}

void disp_fefifo_drv_set_input_order(uint32_t layer_id, uint32_t order)
{
	fefifo_hal_set_in_swap(layer_id, order);
}

int disp_fefifo_dbg_lvl_enable(uint32_t level, uint32_t enable)
{
	if (enable)
		fefifo_dbg_level |= (1 << level);
	else
		fefifo_dbg_level = 0;
	//fefifo_dbg_level &= !(1 << level);
	fefifo_default("set fefifo dbg level %d enable %d 0x%X\n",
		level, enable, fefifo_dbg_level);
	return FEFIFO_RET_OK;
}

void disp_fefifo_drv_read_reg(uint32_t layer_id, uint32_t rel_offset,
	uint32_t size)
{
	uint32_t idx = 0;
	uintptr_t va_addr = 0;

	if ((rel_offset < 0x1000) && (layer_id < 4)) {
		for (idx = 0; idx < size; idx++) {
			va_addr =
			_fefifo_inst[layer_id].fefifo_hw_base + rel_offset;
			va_addr += idx * 4;
			fefifo_default("0x%p = 0x%x\n",
				(void *)va_addr, ReadREG32(va_addr));
		}
	}
}

void disp_fefifo_drv_write_reg(uint32_t layer_id,
	uint32_t rel_offset, uint32_t value)
{
	uintptr_t va_addr = 0;

	if (layer_id < 4)
		va_addr = _fefifo_inst[layer_id].fefifo_hw_base;

	if (rel_offset < 0x1000) {
		va_addr += rel_offset;
		WriteREG32(va_addr, value);
		fefifo_default("0x%p = 0x%x\n",
			(void *)va_addr, ReadREG32(va_addr));
	}
}

int disp_fefifo_drv_gce_trigger(uint32_t layer_id)
{
	int ret = FEFIFO_RET_OK;

	if (!(_fefifo_inst[layer_id].inited) ||
		(_fefifo_inst[layer_id].gce_handle == NULL))
		return FEFIFO_RET_UNINIT;
	fefifo_hal_isr(layer_id, FEFIFO_REG_CONF_CLIENT_GCE);
	return ret;
}

