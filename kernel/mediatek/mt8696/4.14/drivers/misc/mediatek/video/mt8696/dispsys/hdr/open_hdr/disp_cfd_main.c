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

#define LOG_TAG "CFD_MAIN"

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
#include "disp_cfd_main.h"
#include "disp_cfd_hal.h"
#include "disp_cfd_tz_client.h"
#include "disp_cfd_debug.h"
#include "disp_hdr_if.h"
#include "disp_adl_if.h"
#include "disp_ml_if.h"
#include "disp_hdr_main.h"



#define MAX_NUM_WINDOWS 3
#define CFD_MULBASE 10

#define CFD_DTS_MAX_ND_SIZE 50
char cfd_dts_nd_name[CFD_DTS_MAX_ND_SIZE] = "mediatek,mt8696-hdr";
struct disp_cfd_context _cfd_inst[CFD_MAX_LAYER_NUM];
uintptr_t _disp_cfd_hw_base[CFD_MAX_LAYER_NUM] = {0};
uintptr_t _disp_cfd_xvycc_hw_base;
uintptr_t _disp_cfd_be_hw_base;

unsigned int cfd_dbg_level;
struct disp_hw disp_cfd_driver;

struct disp_cfd_context *cfd_get_inst(uint32_t layer_id)
{
	return &_cfd_inst[layer_id];
}

static void _cfd_mutex_init(uint32_t layer_id)
{
	mutex_init(&(_cfd_inst[layer_id].lock));
}

static void _cfd_mutex_lock(uint32_t layer_id)
{
	mutex_lock(&(_cfd_inst[layer_id].lock));
}

static void _cfd_mutex_unlock(uint32_t layer_id)
{
	mutex_unlock(&(_cfd_inst[layer_id].lock));
}

struct disp_hw *disp_cfd_get_drv(void)
{
	return &disp_cfd_driver;
}

int disp_cfd_set_gce_handle(uint32_t layer_id,
	void *pv_handle)
{
	if (!_cfd_inst[layer_id].inited)
		return CFD_RET_UNINIT;
	if (pv_handle == NULL)
		return CFD_RET_INV_ARG;
	_cfd_inst[layer_id].gce_handle = (struct cmdqRecStruct *)pv_handle;
	return CFD_RET_OK;
}

int disp_cfd_dbg_lvl_enable(uint32_t level, uint32_t enable)
{
	if (enable)
		cfd_dbg_level |= (1 << level);
	else
		cfd_dbg_level &= !(1 << level);
	cfd_default("set cfd dbg level %d enable %d 0x%X\n",
		level, enable, cfd_dbg_level);
	return CFD_RET_OK;
}

int disp_cfd_tz_dbg_lvl_enable(uint32_t level, uint32_t enable)
{
	if (enable)
		cfd_sec_debug_level_init(level);
	else
		cfd_sec_debug_level_init(0);
	return CFD_RET_OK;
}

static int _cfd_parse_dev_node(void)
{
	struct device_node *np;
	unsigned int reg_value;
	//unsigned int irq_value;
	//unsigned int irq_no;
	char nd_name[CFD_DTS_MAX_ND_SIZE];
	struct disp_hw *cfd_drv = disp_cfd_get_drv();

	sprintf(nd_name, "%s", cfd_dts_nd_name);

	np = of_find_compatible_node(NULL, NULL, nd_name);
	if (np == NULL) {
		cfd_error("dts error, no cfd device node %s.\n", nd_name);
		return CFD_RET_ERROR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);
	//of_property_read_u32_index(np, "interrupts", 1, &irq_value);

	/* get cfd reg base 0x14009000 */
	_disp_cfd_hw_base[2] = (uintptr_t)of_iomap(np, 0);
	/* get cfd reg base 0x1400a000 */
	_disp_cfd_hw_base[3] = (uintptr_t)of_iomap(np, 1);
	/* get cfd reg base 0x1400b000 */
	_disp_cfd_be_hw_base = (uintptr_t)of_iomap(np, 2);
	/* get cfd reg base 0x15012000 */
	_disp_cfd_hw_base[0] = (uintptr_t)of_iomap(np, 3);
	/* get cfd reg base 0x15015000 */
	_disp_cfd_hw_base[1] = (uintptr_t)of_iomap(np, 4);
	/* get cfd reg base 0x15016000 */
	_disp_cfd_xvycc_hw_base = (uintptr_t)of_iomap(np, 5);

	cfd_info("va[0x%p][0x%p][0x%p][0x%p][0x%p][0x%p].\n",
		(void *)_disp_cfd_hw_base[0],
		(void *)_disp_cfd_hw_base[1],
		(void *)_disp_cfd_hw_base[2],
		(void *)_disp_cfd_hw_base[3],
		(void *)_disp_cfd_xvycc_hw_base,
		(void *)_disp_cfd_be_hw_base);
	cfd_drv->irq_num = 0;
	#if 0
	irq_no = irq_of_parse_and_map(np, 0);
	if (irq_no == 0)
		cfd_error("cfd get irq from dts fail\n");
	else {
		cfd_drv->irq[0].irq = 1;
		cfd_drv->irq_num++;

	irq_no = irq_of_parse_and_map(np, 1);
	if (irq_no == 0)
		cfd_error("cfd get irq from dts fail\n");
	else {
		cfd_drv->irq[0].irq = 2;
		cfd_drv->irq_num++;

	irq_no = irq_of_parse_and_map(np, 2);
	if (irq_no == 0)
		cfd_error("cfd get irq from dts fail\n");
	else {
		cfd_drv->irq[0].irq = 3;
		cfd_drv->irq_num++;

	irq_no = irq_of_parse_and_map(np, 3);
	if (irq_no == 0)
		cfd_error("cfd get irq from dts fail\n");
	else {
		cfd_drv->irq[0].irq = 4;
		cfd_drv->irq_num++;
	#endif

	return CFD_RET_OK;
}

int disp_cfd_init(struct disp_hw_common_info *info)
{
	uint32_t u4Idx = 0;

	_cfd_parse_dev_node();
	for (u4Idx = 0; u4Idx < CFD_MAX_LAYER_NUM; u4Idx++) {
		_cfd_mutex_init(u4Idx);
		memset((void *)&(_cfd_inst[u4Idx]), 0,
			sizeof(struct disp_cfd_context));
		_cfd_inst[u4Idx].cfd_hw_base = _disp_cfd_hw_base[u4Idx];
		if (u4Idx == 0)
			_cfd_inst[u4Idx].cfd_hw_base_ext =
				_disp_cfd_xvycc_hw_base;
		else if (u4Idx == 1)
			_cfd_inst[u4Idx].cfd_hw_base_ext =
				_disp_cfd_xvycc_hw_base + 0x800;

		if (info != NULL)
			disp_cfd_set_gce_handle(u4Idx, info->gce_handle);

		cfd_hal_init(u4Idx, _cfd_inst[u4Idx].cfd_hw_base);
		_cfd_inst[u4Idx].inited = true;
		_cfd_inst[u4Idx].reg_conf_mode = CFD_REG_CONF_CLIENT_RIU;
		_cfd_inst[u4Idx].cfd_state = DISP_CFD_STATE_INITED;

		if (u4Idx == 0) {
			_cfd_inst[u4Idx].io_reg_base = DISP_CFD_VDOFE0_REG_BASE;
			_cfd_inst[u4Idx].io_reg_base_ext =
				DISP_CFD_XVYCC0_REG_BASE;
		} else if (u4Idx == 1) {
			_cfd_inst[u4Idx].io_reg_base = DISP_CFD_VDOFE1_REG_BASE;
			_cfd_inst[u4Idx].io_reg_base_ext =
				DISP_CFD_XVYCC1_REG_BASE;
		} else if (u4Idx == 2)
			_cfd_inst[u4Idx].io_reg_base = DISP_CFD_GFXFE0_REG_BASE;
		else if (u4Idx == 3)
			_cfd_inst[u4Idx].io_reg_base = DISP_CFD_GFXFE1_REG_BASE;
	}
	cfd_debug_init();
	cfd_sec_init();
	// defaults on clk
	cfd_info("%s done\n", __func__);
	return CFD_RET_OK;
}

int disp_cfd_deinit(void)
{
	uint32_t u4Idx = 0;

	cfd_default("%s\n", __func__);
	for (u4Idx = 0; u4Idx < CFD_MAX_LAYER_NUM; u4Idx++) {
		cfd_hal_uninit(u4Idx);
		memset((void *)&(_cfd_inst[u4Idx]), 0,
			sizeof(struct disp_cfd_context));
	}
	cfd_sec_deinit();
	cfd_debug_deinit();
	return CFD_RET_OK;
}

int disp_cfd_set_clk_enable(uint32_t layer_id, uint32_t enable)
{
	if (enable) {
		if (layer_id == 0)
			disp_clock_enable(DISP_CLK_M_HDR_VDO_FE, true);
		else if (layer_id == 1)
			disp_clock_enable(DISP_CLK_S_HDR_VDO_FE, true);
		else if (layer_id == 2)
			disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE, true);
		else if (layer_id == 3)
			disp_clock_enable(DISP_CLK_UHD_HDR_GFX_FE, true);
	} else {
		if (layer_id == 0)
			disp_clock_enable(DISP_CLK_M_HDR_VDO_FE, false);
		else if (layer_id == 1)
			disp_clock_enable(DISP_CLK_S_HDR_VDO_FE, false);
		else if (layer_id == 2)
			disp_clock_enable(DISP_CLK_FHD_HDR_GFX_FE, false);
		else if (layer_id == 3)
			disp_clock_enable(DISP_CLK_UHD_HDR_GFX_FE, false);
	}
	return CFD_RET_OK;
}

void disp_cfd_set_sof(uint32_t layer_id)
{
	if (layer_id == 0)
		fmt_hal_set_sof(FMT_SOF_6_M_DOLBY_FE_STA,
		FMT_SOF_6_M_DOLBY_FE_END, 0x00010002, 0x00010033);
	else if (layer_id == 1)
		fmt_hal_set_sof(FMT_SOF_13_S_DOLBY_FE_STA,
		FMT_SOF_13_S_DOLBY_FE_END, 0x00010002, 0x00010033);
	else if (layer_id == 2)
		fmt_hal_set_sof(FMT_SOF_19_FHD_DOLBY_FE_STA,
		FMT_SOF_19_FHD_DOLBY_FE_END, 0x00010002, 0x00010033);
	else if (layer_id == 3)
		fmt_hal_set_sof(FMT_SOF_16_UHD_DOLBY_FE_STA,
		FMT_SOF_16_UHD_DOLBY_FE_END, 0x00050002, 0x00050033);
}

void disp_cfd_path_sel(uint32_t layer_id, bool fg_on)
{
	if (layer_id == 0)
		disp_path_set_hw_path(DISP_PATH_M_HDR_VDO_FE, fg_on);
	else if (layer_id == 1)
		disp_path_set_hw_path(DISP_PATH_S_HDR_VDO_FE, fg_on);
	else if (layer_id == 2)
		disp_path_set_hw_path(DISP_PATH_FHD_HDR_GFX_FE, fg_on);
	else if (layer_id == 3)
		disp_path_set_hw_path(DISP_PATH_UHD_HDR_GFX_FE, fg_on);
}


int disp_cfd_enable(unsigned int layer_id, bool enable)
{
	if (enable) {
		// path on
		disp_cfd_set_sof(layer_id);
		// clk on
	} else {
		// path off
		// clk off deliver to HDR if
		disp_cfd_hal_manual_reset(layer_id,
		_cfd_inst[layer_id].reg_conf_mode);
	}
	//
	_cfd_inst[layer_id].enabled = enable;
	_cfd_inst[layer_id].frame_no = 0;
	if (!enable) {
		_cfd_inst[layer_id].cfd_state = DISP_CFD_STATE_STOPPED;
		_cfd_inst[layer_id].feature_type = DISP_CFD_BYPASS;
		_cfd_inst[layer_id].feature_ctrlbit = 0;
		_cfd_inst[layer_id].src_bt2020 = false;
		_cfd_inst[layer_id].hdr_type = 0;
		_cfd_inst[layer_id].src_width = 0;
		_cfd_inst[layer_id].src_heght = 0;
	}
	return CFD_RET_OK;
}

int disp_cfd_start(struct disp_hw_common_info *info,
	unsigned int layer_id)
{
	// clk on
	disp_cfd_set_clk_enable(layer_id, 1);
	disp_cfd_set_sof(layer_id);

	// path on
	disp_cfd_path_sel(layer_id, true);

	_cfd_inst[layer_id].enabled = true;
	return CFD_RET_OK;
}

int disp_cfd_stop(unsigned int layer_id)
{
	// path off
	disp_cfd_path_sel(layer_id, false);

	//clk off
	disp_cfd_set_clk_enable(layer_id, 0);
	_cfd_inst[layer_id].cfd_state = DISP_CFD_STATE_STOPPED;
	_cfd_inst[layer_id].enabled = false;
	_cfd_inst[layer_id].feature_type = DISP_CFD_BYPASS;

	return CFD_RET_OK;
}

int disp_cfd_suspend(unsigned int layer_id)
{
	// path off
	disp_cfd_path_sel(layer_id, false);
	//clk off delivered to HDR if
	//disp_cfd_set_clk_enable(layer_id, 0);
	return CFD_RET_OK;
}

int disp_cfd_resume(unsigned int layer_id)
{
	// start will open clk and src
	// clk on delivered to HDR if
	//disp_cfd_set_clk_enable(layer_id, 1);
	disp_cfd_set_sof(layer_id);

	// path on
	disp_cfd_path_sel(layer_id, true);
	disp_cfd_drv_set_bypass(layer_id);

	// rgb2yuv for Graphic
	if ((layer_id == 2) || (layer_id == 3))
		disp_cfd_drv_set_r2y(layer_id, 0);

	return CFD_RET_OK;
}

int disp_cfd_chg_output(struct video_buffer_info *buf_info,
	const struct disp_hw_resolution *info,
	struct disp_hw_tv_capbility *tv_cap)
{
	if (_cfd_inst[0].enabled)
		disp_cfd_drv_decide_video_feature(0, buf_info, tv_cap);
	return CFD_RET_OK;
}

int disp_cfd_irq_handler(uint32_t irq)
{
	// clear irq
	return CFD_RET_OK;
}

int disp_cfd_set_conf_mode(uint32_t layer_id, uint32_t mode)
{
	_cfd_inst[layer_id].reg_conf_mode = mode;
	cfd_info("%s conf mode[%d %d]!\n", __func__, layer_id, mode);
	return CFD_RET_OK;
}

void disp_cfd_set_flush_conf(uint32_t layer_id, uint32_t conf_mode)
{
	disp_cfd_hal_isr(layer_id, conf_mode);
}

void disp_cfd_set_force_hdr(uint32_t layer_id, void *data)
{
	uint32_t force_hdrtype = 0;

	if (data != NULL)
		force_hdrtype = *((uint32_t *) data);

	if (force_hdrtype >= 1) {
		// force hdr flag
		_cfd_inst[layer_id].force_hdr = true;

	} else {
		// force hdr disable
		_cfd_inst[layer_id].force_hdr = false;
	}
}

int disp_cfd_set_cmd(uint32_t layer_id, enum DISP_CMD cmd, void *data)
{
	uint32_t src_width;
	uint32_t src_height;
	uint32_t conf_mode = _cfd_inst[layer_id].reg_conf_mode;
	struct CFD_SRC_ACTIVE_T *pt_src = NULL;
	struct mtk_vdp_hdr10_plus_svp_handle_t *hdr10_plus_svp_handle = NULL;

	_cfd_mutex_lock(layer_id);
	switch (cmd) {
	case DISP_CMD_CFD_SRC_ACTIVE:
		if (data != NULL) {
			pt_src = (struct CFD_SRC_ACTIVE_T *)data;
			src_width = pt_src->width;
			src_height = pt_src->height;
			disp_cfd_hal_set_src_res(layer_id,
				src_width, src_height, conf_mode);
		}
		break;
	case DISP_CMD_CFD_FORCE_HDR:
		disp_cfd_set_force_hdr(layer_id, data);
		break;
	case DISP_CMD_CFD_CHG_OUTPUT:

		break;
	case DISP_CMD_CFD_BACKUP_HDR10PLUS_SEC_HANDLE:
		hdr10_plus_svp_handle =
			(struct mtk_vdp_hdr10_plus_svp_handle_t *)data;
		if (hdr10_plus_svp_handle != NULL)
			disp_cfd_backup_sec_handle(hdr10_plus_svp_handle);
		break;
	default:
		break;
	}
	_cfd_mutex_unlock(layer_id);
	return CFD_RET_OK;
}


int disp_cfd_video_param_init(uint32_t layer_id,
	struct video_buffer_info *buf_info,
	struct disp_hw_tv_capbility *tv_cap)
{
	struct cfd_share_memory_info_t *p_share_mem =
		cfd_sec_get_share_mem(layer_id);
	struct STU_CFD_COLORIMETRY *static_md = NULL;

	if (p_share_mem == NULL)
		return CFD_RET_ERROR;
	// cfd para init
	// set para from video/graphic buffer
	p_share_mem->ctl_param.src_height = buf_info->src.height;
	p_share_mem->ctl_param.src_width = buf_info->src.width;
	p_share_mem->ctl_param.u8Input_Source = 0x9;
	p_share_mem->ctl_param.u8Input_DataFormat = 2;
	p_share_mem->ctl_param.u8Input_IsFullRange = 0;
	p_share_mem->ctl_param.u8Output_Source = 0x1;
	p_share_mem->ctl_param.u8Output_DataFormat = 0x1;
	p_share_mem->ctl_param.u8Output_IsFullRange = 0;
	if (_cfd_inst[layer_id].feature_type == DISP_CFD_SDR2HDR) {
		p_share_mem->ctl_param.u8Input_Format = 131;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x0;
		p_share_mem->ctl_param.u8Output_Format = 132;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x2;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_SDR2HLG) {
		p_share_mem->ctl_param.u8Input_Format = 131;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x0;
		p_share_mem->ctl_param.u8Output_Format = 132;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x3;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_HDR2SDR) {
		p_share_mem->ctl_param.u8Input_Format = 132;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x2;
		p_share_mem->ctl_param.u8Output_Format = 131;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x0;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_HLG2SDR) {
		p_share_mem->ctl_param.u8Input_Format = 132;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x3;
		p_share_mem->ctl_param.u8Output_Format = 131;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x0;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_HLG2HDR) {
		p_share_mem->ctl_param.u8Input_Format = 132;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x3;
		p_share_mem->ctl_param.u8Output_Format = 132;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x2;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_HDR2HLG) {
		p_share_mem->ctl_param.u8Input_Format = 132;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x2;
		p_share_mem->ctl_param.u8Output_Format = 132;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x3;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_HDR10PLUS2SDR) {
		p_share_mem->ctl_param.u8Input_Format = 132;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x5;
		p_share_mem->ctl_param.u8Output_Format = 131;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x0;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_SDR2SDR_2020) {
		p_share_mem->ctl_param.u8Input_Format = 131;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x0;
		p_share_mem->ctl_param.u8Output_Format = 132;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x0;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_HDR2SDR_2020) {
		p_share_mem->ctl_param.u8Input_Format = 132;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x2;
		p_share_mem->ctl_param.u8Output_Format = 132;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x0;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_HLG2SDR_2020) {
		p_share_mem->ctl_param.u8Input_Format = 132;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x3;
		p_share_mem->ctl_param.u8Output_Format = 132;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x0;
	} else if (_cfd_inst[layer_id].feature_type ==
		DISP_CFD_SDR2020_2R709) {
		p_share_mem->ctl_param.u8Input_Format = 132;
		p_share_mem->ctl_param.u8Input_HDRMode = 0x0;
		p_share_mem->ctl_param.u8Output_Format = 131;
		p_share_mem->ctl_param.u8Output_HDRMode = 0x0;
	}

	p_share_mem->ctl_param.u16Source_Max_Luminance =
		buf_info->hdr10_info.ui2_MaxDisplayMasteringLuminance;
	p_share_mem->ctl_param.u16Source_Med_Luminance = 120;
	p_share_mem->ctl_param.u16Source_Min_Luminance =
		buf_info->hdr10_info.ui2_MinDisplayMasteringLuminance;
	p_share_mem->ctl_param.u16Target_Max_Luminance = 100;
	p_share_mem->ctl_param.u16Target_Med_Luminance = 50;
	p_share_mem->ctl_param.u16Target_Min_Luminance = 500;

	// mm para static md
	if ((buf_info->hdr10_type == HDR10_TYPE_PLUS) ||
		(buf_info->hdr10_type == HDR10_TYPE_ST2084)) {
		p_share_mem->mm_param.u8Transfer_Characteristics = 16;
		p_share_mem->mm_param.u8Colour_primaries = 9;
		p_share_mem->mm_param.u8Matrix_Coeffs = 9;
	} else if (buf_info->hdr10_type == HDR10_TYPE_HLG) {
		p_share_mem->mm_param.u8Transfer_Characteristics = 18;
		p_share_mem->mm_param.u8Colour_primaries = 9;
		p_share_mem->mm_param.u8Matrix_Coeffs = 9;
	} else {
		p_share_mem->mm_param.u8Transfer_Characteristics = 1;
		p_share_mem->mm_param.u8Colour_primaries = 1;
		p_share_mem->mm_param.u8Matrix_Coeffs = 1;
	}
	p_share_mem->mm_param.u32Master_Panel_Max_Luminance =
		40000000;
	p_share_mem->mm_param.u32Master_Panel_Min_Luminance = 500;
	p_share_mem->mm_param.u16Max_content_light_level =
		buf_info->hdr10_info.ui2_MaxCLL;
	p_share_mem->mm_param.u16Max_pic_average_light_level =
		buf_info->hdr10_info.ui2_MaxFALL;

	static_md =
	&(p_share_mem->mm_param.stu_Cfd_MM_MasterPanel_ColorMetry);
	static_md->u16Display_Primaries_x[0] =
		buf_info->hdr10_info.ui2_DisplayPrimariesX[0];
	static_md->u16Display_Primaries_x[1] =
		buf_info->hdr10_info.ui2_DisplayPrimariesX[1];
	static_md->u16Display_Primaries_x[2] =
		buf_info->hdr10_info.ui2_DisplayPrimariesX[2];
	static_md->u16Display_Primaries_y[0] =
		buf_info->hdr10_info.ui2_DisplayPrimariesY[0];
	static_md->u16Display_Primaries_y[1] =
		buf_info->hdr10_info.ui2_DisplayPrimariesY[1];
	static_md->u16Display_Primaries_y[2] =
		buf_info->hdr10_info.ui2_DisplayPrimariesY[2];
	static_md->u16White_point_x =
		buf_info->hdr10_info.ui2_WhitePointX;
	static_md->u16White_point_y =
		buf_info->hdr10_info.ui2_WhitePointY;

	// dynamic md
	if ((buf_info->hdr10_type == HDR10_TYPE_PLUS) &&
	(buf_info->hdr_info.metadata_info.dovi_metadata.svp)) {
		// parser md
		// setting sei info
		p_share_mem->sec_handle_type = 2;
		p_share_mem->sec_handle_in =
		buf_info->hdr_info.metadata_info.dovi_metadata.sec_handle;
		p_share_mem->sec_handle_len =
		buf_info->hdr_info.metadata_info.dovi_metadata.len;
		p_share_mem->svp = true;
	} else if (buf_info->hdr10_type == HDR10_TYPE_PLUS) {
		//
		p_share_mem->svp = false;
		disp_cfd_parser_t35_metadata(
		buf_info->hdr_info.metadata_info.dovi_metadata.buff,
		buf_info->hdr_info.metadata_info.dovi_metadata.len,
		&(p_share_mem->mm_param.stu_Cfd_MM_HDR10plus_SEI));
	}
	_cfd_inst[layer_id].src_width = buf_info->src.width;
	_cfd_inst[layer_id].src_heght = buf_info->src.height;
	_cfd_inst[layer_id].hdr_type = buf_info->hdr10_type;

	return CFD_RET_OK;
}

int disp_cfd_graphic_param_init(uint32_t layer_id,
	struct mtk_disp_buffer *buf_info,
	struct disp_hw_tv_capbility *tv_cap)
{
	struct cfd_share_memory_info_t *p_share_mem =
		cfd_sec_get_share_mem(layer_id);

	if (p_share_mem != NULL) {
		// cfd para init
		// set para from video/graphic buffer
		p_share_mem->ctl_param.src_height = buf_info->src.height;
		p_share_mem->ctl_param.src_width = buf_info->src.width;
		p_share_mem->ctl_param.u8Input_Source = 0x9;
		p_share_mem->ctl_param.u8Input_Format = 0x3;
		p_share_mem->ctl_param.u8Input_DataFormat = 0;
		p_share_mem->ctl_param.u8Input_IsFullRange = 1;
		p_share_mem->ctl_param.u8Input_HDRMode = 0;
		p_share_mem->ctl_param.u8Output_Source = 0x1;
		p_share_mem->ctl_param.u8Output_Format = 132;
		p_share_mem->ctl_param.u8Output_DataFormat = 0x2;
		p_share_mem->ctl_param.u8Output_IsFullRange = 0x1;
		if (_cfd_inst[layer_id].feature_type == DISP_CFD_SDR2HDR)
			p_share_mem->ctl_param.u8Output_HDRMode = 0x2;
		else if (_cfd_inst[layer_id].feature_type == DISP_CFD_SDR2HLG)
			p_share_mem->ctl_param.u8Output_HDRMode = 0x3;
		else if (_cfd_inst[layer_id].feature_type ==
			DISP_CFD_SDR2SDR_2020) {
			p_share_mem->ctl_param.u8Output_HDRMode = 0x0;
			p_share_mem->ctl_param.u8Output_Format = 132;
		}

		p_share_mem->mm_param.u8Transfer_Characteristics = 0x01;
		p_share_mem->mm_param.u8Colour_primaries = 0x01;
		p_share_mem->mm_param.u8Matrix_Coeffs = 0x01;
		_cfd_inst[layer_id].src_width = buf_info->src.width;
		_cfd_inst[layer_id].src_heght = buf_info->src.height;
		_cfd_inst[layer_id].hdr_type = 0;
	}
	return CFD_RET_OK;
}

int disp_cfd_flip_config(uint32_t layer_id)
{
	uint32_t addr = 0;
	uint16_t value = 0;
	uint16_t msk = 0;
	uint32_t conf_mode = 0;
	uint32_t client = 0;
	uint32_t count = 0;
	uint32_t size = 0;
	uint32_t idx = 0;
	struct cfd_share_memory_info_t *p_share_mem =
		cfd_sec_get_share_mem(layer_id);
	struct adl_src_tbl adl_tbl;

	if (layer_id > 3)
		return CFD_RET_ERROR;

	conf_mode = _cfd_inst[layer_id].reg_conf_mode;
	if (p_share_mem != NULL) {
		// register
		count = p_share_mem->cfd_hw_output.stRegHdrTable.u32Depth;
		for (idx = 0; idx < count; idx++) {
			addr =
			p_share_mem->cfd_hw_output.stRegHdrTable.au32Addr[idx];
			value =
			p_share_mem->cfd_hw_output.stRegHdrTable.au16Value[idx];
			msk =
			p_share_mem->cfd_hw_output.stRegHdrTable.au16Mask[idx];
			client = layer_id;
			vCFDWriteFldAlign(addr, value, msk, conf_mode, client);
		}

		count = p_share_mem->cfd_hw_output.stRegSdrTable.u32Depth;
		for (idx = 0; idx < count; idx++) {
			addr =
			p_share_mem->cfd_hw_output.stRegSdrTable.au32Addr[idx];
			value =
			p_share_mem->cfd_hw_output.stRegSdrTable.au16Value[idx];
			msk =
			p_share_mem->cfd_hw_output.stRegSdrTable.au16Mask[idx];
			client = layer_id + 4;
			vCFDWriteFldAlign(addr, value, msk, conf_mode, client);
		}

		memset(&adl_tbl, 0, sizeof(struct adl_src_tbl));
		if (layer_id == 0)
			adl_tbl.client = THDR_ADL_V_MAIN;
		else if (layer_id == 1)
			adl_tbl.client = THDR_ADL_V_SUB;
		else if (layer_id == 2)
			adl_tbl.client = THDR_ADL_G_FHD;
		else if (layer_id == 3)
			adl_tbl.client = THDR_ADL_G_UHD;

		// adl
		size = p_share_mem->cfd_hw_output.steotfAdlTable.u32Size;
		if ((size > 0) && (layer_id < 2)) {
			// call adl degamma
			adl_tbl.hdr_b0103.p_data =
			p_share_mem->cfd_hw_output.steotfAdlTable.au8Data;
			adl_tbl.hdr_b0103.used_size =
			p_share_mem->cfd_hw_output.steotfAdlTable.u32Size;
		} else if ((size > 0) && (layer_id >= 2)) {
			// call adl degamma
			adl_tbl.tosd_degam.p_data =
			p_share_mem->cfd_hw_output.steotfAdlTable.au8Data;
			adl_tbl.tosd_degam.used_size =
			p_share_mem->cfd_hw_output.steotfAdlTable.u32Size;
		}

		size = p_share_mem->cfd_hw_output.stootfAdlTable.u32Size;
		if (size > 0) {
			// call adl ootf
			// prepare adl table
			adl_tbl.hdr_ootf.p_data =
			p_share_mem->cfd_hw_output.stootfAdlTable.au8Data;
			adl_tbl.hdr_ootf.used_size =
			p_share_mem->cfd_hw_output.stootfAdlTable.u32Size;
		}

		size = p_share_mem->cfd_hw_output.stoetfAdlTable.u32Size;
		if (size > 0) {
			// call adl gamma
			// prepare adl table
			adl_tbl.hdr_b0105.p_data =
			p_share_mem->cfd_hw_output.stoetfAdlTable.au8Data;
			adl_tbl.hdr_b0105.used_size =
			p_share_mem->cfd_hw_output.stoetfAdlTable.u32Size;
		}

		size = p_share_mem->cfd_hw_output.sttmoAdlTable.u32Size;
		if (size > 0) {
			// call adl tmo
			// prepare adl table
			adl_tbl.hdr_b0202ss.p_data =
			p_share_mem->cfd_hw_output.sttmoAdlTable.au8Data;
			adl_tbl.hdr_b0202ss.used_size =
			p_share_mem->cfd_hw_output.sttmoAdlTable.u32Size;

			adl_tbl.hdr_b0202si.p_data =
			p_share_mem->cfd_hw_output.sttmoAdlTable.au8Data;
			adl_tbl.hdr_b0202si.used_size =
			p_share_mem->cfd_hw_output.sttmoAdlTable.u32Size;

			adl_tbl.hdr_b0202ts.p_data =
			p_share_mem->cfd_hw_output.sttmoAdlTable.au8Data;
			adl_tbl.hdr_b0202ts.used_size =
			p_share_mem->cfd_hw_output.sttmoAdlTable.u32Size;

			adl_tbl.hdr_b0202ti.p_data =
			p_share_mem->cfd_hw_output.sttmoAdlTable.au8Data;
			adl_tbl.hdr_b0202ti.used_size =
			p_share_mem->cfd_hw_output.sttmoAdlTable.u32Size;
		}
		adl_tbl.path = OPENHDR_PATH;
		disp_adl_cfg_client_en(adl_tbl.client, 1, 0);
		disp_config_adl_table(&adl_tbl);
		disp_adl_thread_wakeup(OPENHDR_PATH);

		// reset sharemem
		p_share_mem->cfd_hw_output.stRegHdrTable.u32Depth = 0;
		p_share_mem->cfd_hw_output.stRegSdrTable.u32Depth = 0;
		p_share_mem->cfd_hw_output.steotfAdlTable.u32Size = 0;
		p_share_mem->cfd_hw_output.stootfAdlTable.u32Size = 0;
		p_share_mem->cfd_hw_output.stoetfAdlTable.u32Size = 0;
		p_share_mem->cfd_hw_output.sttmoAdlTable.u32Size = 0;
	}

	return CFD_RET_OK;
}

enum hdr_output_type disp_cfd_mapping_out_format(uint32_t layer_id)
{
	enum hdr_output_type out_format = HDR_OUT_TYPE_SDR;

	switch (_cfd_inst[layer_id].feature_type) {
	case DISP_CFD_HDR2HDR:
	case DISP_CFD_SDR2HDR:
	case DISP_CFD_HLG2HDR:
	case DISP_CFD_HDR10PLUS2HDR10:
		out_format = HDR_OUT_TYPE_HDR10;
		break;
	case DISP_CFD_HDR10PLUS2HDR10PLUS:
		out_format = HDR_OUT_TYPE_HDR10PLUS;
		break;
	case DISP_CFD_HDR10PLUS2VSIF:
		out_format = HDR_OUT_TYPE_HDR10PLUS_VSIF;
		break;
	case DISP_CFD_HLG2HLG:
	case DISP_CFD_SDR2HLG:
	case DISP_CFD_HDR2HLG:
		out_format = HDR_OUT_TYPE_HLG;
		break;
	case DISP_CFD_BYPASS_SDR2020:
	case DISP_CFD_SDR2SDR_2020:
	case DISP_CFD_HDR2SDR_2020:
	case DISP_CFD_HLG2SDR_2020:
		out_format = HDR_OUT_TYPE_SDR_2020;
		break;
	default:
		break;
	}

	return out_format;
}

enum DISP_CFD_FEATURE_E disp_cfd_pip_sub_video_decide(
	struct video_buffer_info *buf_info)
{
	enum DISP_CFD_FEATURE_E e_feture_on = DISP_CFD_BYPASS;
	enum hdr_output_type out_format = HDR_OUT_TYPE_SDR;
	bool force_hdr = false;
	bool force_hlg = false;
	bool force_sdr_2020 = false;

	out_format = disp_cfd_mapping_out_format(0);

	switch (out_format) {
	case HDR_OUT_TYPE_HDR10:
	case HDR_OUT_TYPE_HDR10PLUS:
	case HDR_OUT_TYPE_HDR10PLUS_VSIF:
		force_hdr = 1;
		break;
	case HDR_OUT_TYPE_HLG:
		force_hlg = 1;
		break;
	case HDR_OUT_TYPE_SDR_2020:
		force_sdr_2020 = 1;
		break;
	default:
		break;
	}

	switch (buf_info->hdr_info.dr_range) {
	case DISP_DR_TYPE_SDR:
		if (force_hdr)
			e_feture_on = DISP_CFD_SDR2HDR;
		else if (force_hlg)
			e_feture_on = DISP_CFD_SDR2HLG;
		else if (force_sdr_2020 && buf_info->is_bt2020)
			e_feture_on = DISP_CFD_BYPASS_SDR2020;
		else if (force_sdr_2020)
			e_feture_on = DISP_CFD_SDR2SDR_2020;
		else if (buf_info->is_bt2020)
			e_feture_on = DISP_CFD_SDR2020_2R709;
		else
			e_feture_on = DISP_CFD_BYPASS;
		break;
	case DISP_DR_TYPE_HDR10:
		if (buf_info->hdr10_type == HDR10_TYPE_PLUS) {
			if (force_hdr)
				e_feture_on = DISP_CFD_HDR10PLUS2HDR10;
			else if (force_hlg)
				e_feture_on = DISP_CFD_HDR2HLG;
			else if (force_sdr_2020)
				e_feture_on = DISP_CFD_HDR2SDR_2020;
			else
				e_feture_on = DISP_CFD_HDR10PLUS2SDR;
		} else if (buf_info->hdr10_type == HDR10_TYPE_ST2084) {
			if (force_hdr)
				e_feture_on = DISP_CFD_HDR2HDR;
			else if (force_hlg)
				e_feture_on = DISP_CFD_HDR2HLG;
			else if (force_sdr_2020)
				e_feture_on = DISP_CFD_HDR2SDR_2020;
			else
				e_feture_on = DISP_CFD_HDR2SDR;
		}
		break;
	case DISP_DR_TYPE_DOVI:
		e_feture_on = DISP_CFD_OUTOF_HANDLE;
		break;
	case DISP_DR_TYPE_PHLP:
		e_feture_on = DISP_CFD_OUTOF_HANDLE;
		break;
	case DISP_DR_TYPE_HLG:
		if (force_hlg)
			e_feture_on = DISP_CFD_HLG2HLG;
		else if (force_hdr)
			e_feture_on = DISP_CFD_HLG2HDR;
		else if (force_sdr_2020)
			e_feture_on = DISP_CFD_HLG2SDR_2020;
		else
			e_feture_on = DISP_CFD_HLG2SDR;
		break;
	default:
		e_feture_on = DISP_CFD_BYPASS;
		break;
	}

	return e_feture_on;
}


int disp_cfd_drv_decide_video_feature(uint32_t layer_id,
	struct video_buffer_info *buf_info,
	struct disp_hw_tv_capbility *tv_cap)
{
	enum DISP_CFD_FEATURE_E e_feture_on = DISP_CFD_BYPASS;
	bool force_hdr = false;

	force_hdr = _cfd_inst[layer_id].force_hdr;
	if ((buf_info == NULL) || (tv_cap == NULL)) {
		e_feture_on = DISP_CFD_OUTOF_HANDLE;
		_cfd_inst[layer_id].feature_type = e_feture_on;
		return CFD_RET_OK;
	}

	switch (buf_info->hdr_info.dr_range) {
	case DISP_DR_TYPE_SDR:
		if (tv_cap->is_support_hdr && force_hdr)
			e_feture_on = DISP_CFD_SDR2HDR;
		else if (tv_cap->is_support_hlg && force_hdr)
			e_feture_on = DISP_CFD_SDR2HLG;
		else if (!(tv_cap->is_support_bt2020) &&
			buf_info->is_bt2020)
			e_feture_on = DISP_CFD_SDR2020_2R709;
		else if (tv_cap->is_support_bt2020 &&
			buf_info->is_bt2020)
			e_feture_on = DISP_CFD_BYPASS_SDR2020;
		else
			e_feture_on = DISP_CFD_BYPASS;
		break;
	case DISP_DR_TYPE_HDR10:
		if (buf_info->hdr10_type == HDR10_TYPE_PLUS) {
			if (tv_cap->is_support_hdr10_plus &&
				(tv_cap->hdr10_plus_app_ver != 0xFF))
				e_feture_on = DISP_CFD_HDR10PLUS2VSIF;
			else if (tv_cap->is_support_hdr10_plus &&
				(tv_cap->hdr10_plus_app_ver == 0xFF))
				e_feture_on = DISP_CFD_HDR10PLUS2HDR10PLUS;
			else if (tv_cap->is_support_hdr)
				e_feture_on = DISP_CFD_HDR10PLUS2HDR10;
			else if (tv_cap->is_support_hlg && force_hdr)
				e_feture_on = DISP_CFD_HDR2HLG;
			else if (buf_info->is_bt2020 &&
				tv_cap->is_support_bt2020)
				e_feture_on = DISP_CFD_HDR2SDR_2020;
			else
				e_feture_on = DISP_CFD_HDR10PLUS2SDR;
		} else if (buf_info->hdr10_type == HDR10_TYPE_ST2084) {
			if (tv_cap->is_support_hdr)
				e_feture_on = DISP_CFD_HDR2HDR;
			else if (tv_cap->is_support_hlg && force_hdr)
				e_feture_on = DISP_CFD_HDR2HLG;
			else if (buf_info->is_bt2020 &&
				tv_cap->is_support_bt2020)
				e_feture_on = DISP_CFD_HDR2SDR_2020;
			else
				e_feture_on = DISP_CFD_HDR2SDR;
		}
		break;
	case DISP_DR_TYPE_DOVI:
		e_feture_on = DISP_CFD_OUTOF_HANDLE;
		break;
	case DISP_DR_TYPE_PHLP:
		e_feture_on = DISP_CFD_OUTOF_HANDLE;
		break;
	case DISP_DR_TYPE_HLG:
		if (tv_cap->is_support_hlg)
			e_feture_on = DISP_CFD_HLG2HLG;
		else if (tv_cap->is_support_hdr && force_hdr)
			e_feture_on = DISP_CFD_HLG2HDR;
		else if (buf_info->is_bt2020 &&
			tv_cap->is_support_bt2020)
			e_feture_on = DISP_CFD_HLG2SDR_2020;
		else
			e_feture_on = DISP_CFD_HLG2SDR;
		break;
	default:
		e_feture_on = DISP_CFD_BYPASS;
		break;
	}

	if ((layer_id == 1) && (_cfd_inst[0].enabled))
		e_feture_on = disp_cfd_pip_sub_video_decide(buf_info);

	cfd_output_info("cfd decide video dy %d, type %d, force %d, out %d\n",
		buf_info->hdr_info.dr_range, buf_info->hdr10_type,
		force_hdr, e_feture_on);
	_cfd_inst[layer_id].feature_type = e_feture_on;
	return CFD_RET_OK;
}

int disp_cfd_drv_decide_graphic_feature(uint32_t layer_id,
	struct mtk_disp_buffer *buf_info,
	struct disp_hw_tv_capbility *tv_cap)
{
	enum DISP_CFD_FEATURE_E e_feture_on = DISP_CFD_BYPASS;
	enum DISP_CFD_FEATURE_E e_video_feature = DISP_CFD_BYPASS;
	bool force_hdr = false;

	force_hdr = _cfd_inst[layer_id].force_hdr;
	e_video_feature = _cfd_inst[0].feature_type;
	if ((buf_info == NULL) || (tv_cap == NULL)) {
		e_feture_on = DISP_CFD_OUTOF_HANDLE;
		_cfd_inst[layer_id].feature_type = e_feture_on;
		return CFD_RET_OK;
	}

	switch (e_video_feature) {
	case DISP_CFD_HDR2HDR:
	case DISP_CFD_SDR2HDR:
	case DISP_CFD_HLG2HDR:
	case DISP_CFD_HDR10PLUS2HDR10PLUS:
	case DISP_CFD_HDR10PLUS2HDR10:
	case DISP_CFD_HDR10PLUS2VSIF:
		e_feture_on = DISP_CFD_SDR2HDR;
		break;
	case DISP_CFD_HLG2HLG:
	case DISP_CFD_SDR2HLG:
	case DISP_CFD_HDR2HLG:
		e_feture_on = DISP_CFD_SDR2HLG;
		break;
	case DISP_CFD_BYPASS_SDR2020:
	case DISP_CFD_SDR2SDR_2020:
	case DISP_CFD_HDR2SDR_2020:
	case DISP_CFD_HLG2SDR_2020:
		e_feture_on = DISP_CFD_SDR2SDR_2020;
		//e_feture_on = DISP_CFD_BYPASS;
		break;
	default:
		e_feture_on = DISP_CFD_BYPASS;
		break;
	}

	if (tv_cap->is_support_hdr && force_hdr)
		e_feture_on = DISP_CFD_SDR2HDR;
	else if (tv_cap->is_support_hlg && force_hdr)
		e_feture_on = DISP_CFD_SDR2HLG;

	cfd_output_info("cfd decide graphic %d %d out %d\n",
		e_video_feature, force_hdr, e_feture_on);
	_cfd_inst[layer_id].feature_type = e_feture_on;
	return CFD_RET_OK;
}

bool disp_cfd_drv_graphic_feature_chg(
	uint32_t layer_id)
{
	enum DISP_CFD_FEATURE_E e_feture_on = DISP_CFD_BYPASS;
	enum DISP_CFD_FEATURE_E e_video_feature = DISP_CFD_BYPASS;
	bool fg_chg = false;

	e_video_feature = _cfd_inst[0].feature_type;

	switch (e_video_feature) {
	case DISP_CFD_HDR2HDR:
	case DISP_CFD_SDR2HDR:
	case DISP_CFD_HLG2HDR:
	case DISP_CFD_HDR10PLUS2HDR10PLUS:
	case DISP_CFD_HDR10PLUS2HDR10:
	case DISP_CFD_HDR10PLUS2VSIF:
		e_feture_on = DISP_CFD_SDR2HDR;
		break;
	case DISP_CFD_HLG2HLG:
	case DISP_CFD_SDR2HLG:
	case DISP_CFD_HDR2HLG:
		e_feture_on = DISP_CFD_SDR2HLG;
		break;
	case DISP_CFD_BYPASS_SDR2020:
	case DISP_CFD_SDR2SDR_2020:
	case DISP_CFD_HDR2SDR_2020:
	case DISP_CFD_HLG2SDR_2020:
		e_feture_on = DISP_CFD_SDR2SDR_2020;
		break;
	default:
		e_feture_on = DISP_CFD_BYPASS;
		break;
	}

	if (_cfd_inst[layer_id].enabled &&
		(_cfd_inst[layer_id].feature_type != e_feture_on)) {
		cfd_default("graphic[%d] feature chg %d to %d\n", layer_id,
			_cfd_inst[layer_id].feature_type, e_feture_on);
		fg_chg = true;
	}
	return fg_chg;
}

int disp_cfd_config_video_frame(uint32_t layer_id,
	struct video_buffer_info *buf_info,
	struct disp_hw_tv_capbility *tv_cap)
{
	uint32_t conf_mode = 0;
	bool graphic_fhd_update = false;
	bool graphic_uhd_update = false;

	cfd_info("set %s %d\n", __func__, _cfd_inst[layer_id].frame_no);
	if (buf_info == NULL)
		return CFD_RET_INV_ARG;

	if (!(_cfd_inst[layer_id].enabled)) {
		// path o
		disp_cfd_enable(layer_id, true);
	}

	_cfd_inst[layer_id].frame_no++;
	conf_mode = _cfd_inst[layer_id].reg_conf_mode;
	disp_cfd_drv_decide_video_feature(layer_id, buf_info, tv_cap);

	// handle for static graphic case
	if (_cfd_inst[layer_id].frame_no == 1) {
		graphic_fhd_update =
			disp_cfd_drv_graphic_feature_chg(CFD_LAYER_GFX_FE0);
		if ((_cfd_inst[CFD_LAYER_GFX_FE0].enabled) &&
			graphic_fhd_update)
			disp_hdr_wakeup_routine(LAYER2);

		graphic_uhd_update =
			disp_cfd_drv_graphic_feature_chg(CFD_LAYER_GFX_FE1);
		if ((_cfd_inst[CFD_LAYER_GFX_FE1].enabled) &&
			graphic_uhd_update)
			disp_hdr_wakeup_routine(LAYER3);
	}

	if (_cfd_inst[layer_id].feature_type == DISP_CFD_OUTOF_HANDLE)
		disp_cfd_drv_set_bypass(layer_id);
	else if ((_cfd_inst[layer_id].feature_type == DISP_CFD_BYPASS) ||
	(_cfd_inst[layer_id].feature_type == DISP_CFD_HDR2HDR) ||
	(_cfd_inst[layer_id].feature_type == DISP_CFD_HLG2HLG) ||
	(_cfd_inst[layer_id].feature_type == DISP_CFD_HDR10PLUS2HDR10PLUS) ||
	(_cfd_inst[layer_id].feature_type == DISP_CFD_HDR10PLUS2HDR10) ||
	(_cfd_inst[layer_id].feature_type == DISP_CFD_HDR10PLUS2VSIF) ||
	(_cfd_inst[layer_id].feature_type == DISP_CFD_BYPASS_SDR2020)) {
		disp_cfd_drv_set_bypass(layer_id);
		if ((_cfd_inst[layer_id].feature_type ==
			DISP_CFD_HDR10PLUS2VSIF) &&
		(buf_info->hdr_info.metadata_info.dovi_metadata.svp)) {
			disp_cfd_parser_vsif_in_sec(layer_id,
			buf_info->hdr_info.metadata_info
			.dovi_metadata.sec_handle,
			buf_info->hdr_info.metadata_info.dovi_metadata.len);
		} else if ((_cfd_inst[layer_id].feature_type ==
			DISP_CFD_HDR10PLUS2HDR10PLUS) &&
		(buf_info->hdr_info.metadata_info.dovi_metadata.svp)) {
			// paser sec hanlde to tz_cfd
			// tz_cfd query handle and sent to hdmi
			disp_cfd_parser_t35_emp_in_sec(layer_id,
			buf_info->hdr_info.metadata_info
			.dovi_metadata.sec_handle,
			buf_info->hdr_info.metadata_info.dovi_metadata.len);
		}
	} else if (_cfd_inst[layer_id].feature_type == DISP_CFD_R2Y) {
		disp_cfd_drv_set_bypass(layer_id);
		disp_cfd_drv_set_r2y(layer_id, 0);
	} else {
		disp_cfd_set_conf_mode(layer_id, CFD_REG_CONF_CLIENT_ML);
		conf_mode = _cfd_inst[layer_id].reg_conf_mode;
		// cfd driver
		disp_cfd_video_param_init(layer_id, buf_info, tv_cap);
		cfd_sec_config_frame(layer_id);
		// top ctrl
		//disp_cfd_hal_set_src_res(layer_id, buf_info->tgt.width,
		//	buf_info->tgt.height, conf_mode);
		disp_cfd_hal_set_top_ctrl(layer_id, conf_mode);
		// algo config
		disp_cfd_flip_config(layer_id);
	}
	disp_cfd_hal_set_cur_res(layer_id, conf_mode);

	// ml trigger from here
	if (conf_mode == CFD_REG_CONF_CLIENT_ML)
		disp_cfd_set_flush_conf(layer_id, CFD_REG_CONF_CLIENT_ML);

	_cfd_inst[layer_id].cfd_state = DISP_CFD_STATE_START;

	if ((buf_info != NULL) && (buf_info->is_bt2020))
		_cfd_inst[layer_id].src_bt2020 = true;
	else
		_cfd_inst[layer_id].src_bt2020 = false;
	return CFD_RET_OK;
}

int disp_cfd_config_graphic_frame(uint32_t layer_id,
	struct mtk_disp_buffer *buf_info,
	struct disp_hw_tv_capbility *tv_cap)
{
	uint32_t conf_mode = 0;

	if (buf_info == NULL)
		return CFD_RET_INV_ARG;

	cfd_info("set %s %d\n", __func__, _cfd_inst[layer_id].frame_no);

	if (!(_cfd_inst[layer_id].enabled)) {
		// path on
		disp_cfd_enable(layer_id, true);
	}

	_cfd_inst[layer_id].frame_no++;
	conf_mode = _cfd_inst[layer_id].reg_conf_mode;
	disp_cfd_drv_decide_graphic_feature(layer_id, buf_info, tv_cap);

	if ((_cfd_inst[layer_id].feature_type == DISP_CFD_SDR2HDR) ||
		(_cfd_inst[layer_id].feature_type == DISP_CFD_SDR2HLG) ||
		(_cfd_inst[layer_id].feature_type == DISP_CFD_SDR2SDR_2020)) {
		disp_cfd_set_conf_mode(layer_id, CFD_REG_CONF_CLIENT_ML);
		conf_mode = _cfd_inst[layer_id].reg_conf_mode;
		// cfd driver
		disp_cfd_graphic_param_init(layer_id, buf_info, tv_cap);
		cfd_sec_config_frame(layer_id);
		// top ctrl
		disp_cfd_hal_set_src_res(layer_id, buf_info->tgt.width,
			buf_info->tgt.height, conf_mode);
		disp_cfd_hal_set_top_ctrl(layer_id, conf_mode);
		// algo config
		disp_cfd_flip_config(layer_id);
	} else {
		disp_cfd_drv_set_bypass(layer_id);
		disp_cfd_drv_set_r2y(layer_id, 0);
	}
	//disp_cfd_hal_set_cur_res(layer_id, conf_mode);
	// ml trigger from here
	if (conf_mode == CFD_REG_CONF_CLIENT_ML)
		disp_cfd_set_flush_conf(layer_id, CFD_REG_CONF_CLIENT_ML);

	_cfd_inst[layer_id].cfd_state = DISP_CFD_STATE_START;

	return CFD_RET_OK;
}


int disp_cfd_set_test_case(uint32_t layer_id, uint32_t test_no)
{
	cfd_default("set %s %d\n", __func__, test_no);
	cfd_sec_test_case(layer_id, test_no, _cfd_inst[layer_id].reg_conf_mode);
	return CFD_RET_OK;
}

int disp_cfd_backup_sec_handle(
	struct mtk_vdp_hdr10_plus_svp_handle_t *hdr10_plus_svp_handle)
{
	int ret = CFD_RET_OK;

	cfd_info("set %s\n", __func__);
	ret = cfd_backup_hdr10plus_sec_handle(hdr10_plus_svp_handle,
		sizeof(struct mtk_vdp_hdr10_plus_svp_handle_t));
	return ret;
}

int disp_cfd_parser_vsif_in_sec(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t sec_len)
{
	int ret = CFD_RET_OK;

	cfd_info("set %s\n", __func__);
	ret = cfd_sec_parser_hdr10plus_vsif(layer_id,
		sec_handle, sec_len);
	return ret;
}

int disp_cfd_parser_t35_emp_in_sec(uint32_t layer_id,
	uint32_t sec_handle,
	uint32_t sec_len)
{
	int ret = CFD_RET_OK;

	cfd_info("set %s\n", __func__);
	ret = cfd_sec_parser_hdr10plus_emp(layer_id,
		sec_handle, sec_len);
	return ret;
}

int disp_cfd_set_tz_init(uint32_t fg_enable)
{
	cfd_default("set %s %d\n", __func__, fg_enable);
	if (fg_enable)
		cfd_sec_init();
	else
		cfd_sec_deinit();
	return CFD_RET_OK;
}

void disp_cfd_drv_read_reg(uint32_t client, uint32_t rel_offset,
	uint32_t size)
{
	uint32_t idx = 0;
	uintptr_t va_addr = 0;

	if ((rel_offset < 0x1000) && (client < 6)) {
		for (idx = 0; idx < size; idx++) {
			if (client < 4) {
				va_addr = _cfd_inst[client].cfd_hw_base;
				va_addr += rel_offset + idx * 4;
			} else {
				va_addr = _cfd_inst[client%4].cfd_hw_base_ext;
				va_addr += rel_offset + idx * 4;
			}
			cfd_default("0x%p = 0x%x\n",
				(void *)va_addr, ReadREG32(va_addr));
		}
	}
}

void disp_cfd_drv_write_reg(uint32_t client, uint32_t rel_offset,
	uint32_t value)
{
	uintptr_t va_addr = 0;

	if (client < 4)
		va_addr = _cfd_inst[client].cfd_hw_base;
	else
		va_addr = _cfd_inst[client%4].cfd_hw_base_ext;

	if (rel_offset < 0x1000) {
		va_addr += rel_offset;
		WriteREG32(va_addr, value);
		cfd_default("0x%p = 0x%x\n",
			(void *)va_addr, ReadREG32(va_addr));
	}
}

int disp_cfd_drv_set_bypass(uint32_t layer_id)
{
	uint32_t ret;
	uint32_t conf_mode = _cfd_inst[layer_id].reg_conf_mode;

	ret = disp_cfd_hal_set_bypass(layer_id, conf_mode);
	return ret;
}

int disp_cfd_drv_set_r2y(uint32_t layer_id, uint32_t color_format)
{
	uint32_t ret;
	uint32_t color_mode = color_format;
	uint32_t conf_mode = _cfd_inst[layer_id].reg_conf_mode;

	ret = disp_cfd_hal_set_r2y(layer_id, color_mode, conf_mode);
	return ret;
}

int disp_cfd_drv_gce_trigger(uint32_t layer_id)
{
	int ret = CFD_RET_OK;

	if (!(_cfd_inst[layer_id].inited) ||
		(_cfd_inst[layer_id].gce_handle == NULL))
		return CFD_RET_UNINIT;
	disp_cfd_hal_isr(layer_id, CFD_REG_CONF_CLIENT_GCE);
	return ret;
}

int disp_cfd_drv_get_hdmi_output_format(uint32_t layer_id,
	uint32_t *out_format, bool *bt2020_enable)
{
	int ret = CFD_RET_OK;
	enum hdr_output_type out_fmt = HDR_OUT_TYPE_SDR;

	if ((out_format == NULL) || (bt2020_enable == NULL))
		return CFD_RET_INV_ARG;

	if (_cfd_inst[layer_id].enabled)
		out_fmt = disp_cfd_mapping_out_format(layer_id);

	if (_cfd_inst[layer_id].src_bt2020 &&
		((out_fmt == HDR_OUT_TYPE_HDR10) ||
		(out_fmt == HDR_OUT_TYPE_HLG) ||
		(out_fmt == HDR_OUT_TYPE_HDR10PLUS) ||
		(out_fmt == HDR_OUT_TYPE_HDR10PLUS_VSIF)))
		*bt2020_enable = true;
	else if (out_fmt == HDR_OUT_TYPE_SDR_2020)
		*bt2020_enable = true;

	*out_format = out_fmt;
	return ret;
}

int disp_cfd_drv_get_hdr10_metadata(
	struct VID_PLA_HDR_METADATA_INFO_T *rHdr,
	uint32_t id)
{
	struct video_buffer_info *vid_info = NULL;
	enum hdr_output_type out_fmt = HDR_OUT_TYPE_SDR;
	struct VID_STATIC_HDMI_MD_T *static_md = NULL;

	if (id < 2)
		vid_info = disp_hdr_get_vid_info(id);
	if (rHdr == NULL) {
		cfd_default("get hdr10 md failed\n");
		return CFD_RET_ERROR;
	}

	static_md = &(rHdr->metadata_info.hdr10_metadata);
	if (_cfd_inst[id].enabled)
		out_fmt = disp_cfd_mapping_out_format(id);

	switch (_cfd_inst[id].feature_type) {
	case DISP_CFD_HDR2HDR:
	case DISP_CFD_HDR2HLG:
	case DISP_CFD_HLG2HLG:
	case DISP_CFD_HLG2HDR:
	case DISP_CFD_HDR10PLUS2HDR10:
	case DISP_CFD_HDR10PLUS2HDR10PLUS:
	case DISP_CFD_HDR10PLUS2VSIF:
		if (vid_info != NULL) {
			rHdr->e_DynamicRangeType = VID_PLA_DR_TYPE_HDR10;
			memcpy(static_md, &(vid_info->hdr10_info),
			sizeof(struct VID_STATIC_HDMI_MD_T));
		}
		break;
	default:
		// get info from shm memory
		break;
	}

	return CFD_RET_OK;
}

uint16_t cfd_vsif_min(uint16_t sei_value, uint16_t mocecule_de,
	uint32_t mocecule_max, uint16_t denominater, uint16_t max_value)
{
	uint32_t sei_value_new;
	uint32_t temp = 0;
	uint16_t vsif_value = 0;

	temp = (sei_value * CFD_MULBASE) / mocecule_de;
	temp = (temp >
	mocecule_max * CFD_MULBASE) ? mocecule_max * CFD_MULBASE:temp;
	temp = temp/denominater;
	if ((temp % 10) >= 5)
		temp = 1;
	else
		temp = 0;

	sei_value_new = sei_value/mocecule_de;
	sei_value_new =
	(sei_value_new > mocecule_max) ? mocecule_max : sei_value_new;
	vsif_value = (uint16_t)(sei_value_new / denominater + temp);
	vsif_value = (vsif_value > max_value) ? max_value : vsif_value;
	return vsif_value;
}

uint32_t cfd_bitshift(int32_t needbits)
{
	uint32_t retValue = 1;
	int32_t temp = 0;

	temp = needbits;
	while (needbits) {
		retValue *= 2;
		needbits--;
	}
	return (retValue - 1);
}

uint32_t cfd_bitvalue_get(uint8_t *hdr_metadata,
	int32_t needbits, int32_t *remained_bits, uint32_t *addBytes)
{
	int32_t diff = 0;
	int32_t i = 0;
	uint32_t retValue = 0;

	*addBytes = 0;
	diff = needbits - (*remained_bits);
	if (diff < 0) {
		(*remained_bits) -= needbits;
		retValue =
		((*hdr_metadata) >> (*remained_bits)) & cfd_bitshift(needbits);
	} else {
		if (diff == 0) {
			retValue =
			((*hdr_metadata) >> 0) & cfd_bitshift(needbits);
			*addBytes = 1;
			*remained_bits = 8;
		} else {
			if (diff - (diff / 8 * 8) > 0) {
				*addBytes = diff / 8 + 1;
				while (i <= (*addBytes)) {
					retValue += (*(hdr_metadata + i))
					<< (8 * ((*addBytes) - i));
					i++;
				}
				*remained_bits = (*addBytes) * 8 - diff;
				retValue = (retValue >>
				(*remained_bits)) & cfd_bitshift(needbits);
			} else {
				(*addBytes) = diff / 8;
				while (i <= (*addBytes)) {
					retValue +=
					(*(hdr_metadata + i)) <<
					(8 * ((*addBytes) - i));
					i++;
				}

				retValue =
				(retValue >> 0) & cfd_bitshift(needbits);
				*remained_bits = 8;
				(*addBytes)++;
			}
		}
	}
	cfd_md_info("addbytes is %d value is 0x%x remained_bits is %d\n",
		(*addBytes),
		retValue,
		*remained_bits);
	return retValue;
}

uint8_t cfd_user_data_registered_inu_t_t35_new(
	uint8_t *hdr_metadata,
	uint32_t size, uint8_t *vsif, uint32_t ctl_info)
{
	uint8_t country_code = 0;
	uint8_t application_identifier = 0;
	uint8_t application_version = 0;
	uint8_t num_windows = 0;
	uint8_t num_idx = 0;
	uint8_t idx = 0;
	uint32_t sel_tsdmaxl = 0;
	uint8_t vsif_tsdmaxl = 0;
	uint32_t sei_avgmaxrgb = 0;
	uint8_t vsif_avgmaxrgb = 0;
	uint8_t num_sei_dmaxrgb = 0;
	uint32_t sei_dmaxrgb[16];/* distributtion */
	uint8_t sei_dmaxrgb_idx[16];/* distributtion */
	uint8_t vsif_dmaxrgb[16];
	uint8_t tone_mapping_flag = 0;
	uint32_t sei_kpx = 0;
	uint16_t vsif_kpx = 0;
	uint32_t sei_kpy = 0;
	uint16_t vsif_kpy = 0;
	uint8_t num_sei_bz_a = 0;
	uint32_t sei_bz_a[9];
	uint8_t vsif_bz_a[9];
	int32_t remained_bits;
	uint32_t addBytes;
	uint32_t value;
	uint8_t *metadata = NULL;
	uint8_t graphic_overlay_flag = 0;
	uint8_t vsif_timing_mode = 0;

	if (hdr_metadata == NULL || vsif == NULL)
		return 0;
	metadata = hdr_metadata;
	country_code = *metadata;
	//skip country code, privider_code, provider(u(8)+u(16)+u(16))
	metadata += 5;
	application_identifier = *metadata;
	application_version = *(metadata + 1);
	metadata += 2;
	num_windows = (((*metadata) & 0xC0) >> 6);
	remained_bits = 6;

	/* 27 bit */
	sel_tsdmaxl = cfd_bitvalue_get(metadata, 27, &remained_bits, &addBytes);
	metadata += addBytes; /* 2bit */
	cfd_md_info("[CFD]code:0x%x num_wins:%d tsdmaxl:0x%x metadata=0x%x\n",
		country_code,
		num_windows,
		sel_tsdmaxl,
		*metadata);
	remained_bits = 2; /* skip 1 bit for tsdaplflag */
	for (num_idx = 0; num_idx < num_windows; ++num_idx) {
		cfd_md_info("[CFD]num_idx[%d] *(md)=0x%x rd_bits=%d\n",
			num_idx,
			*(metadata),
			remained_bits);

		/* skip maxsel */
		(void)cfd_bitvalue_get(metadata, 51, &remained_bits, &addBytes);
		metadata += addBytes;

		cfd_md_info("checkmaxScl *(metadata)=0x%x remained_bits=%d\n",
			*(metadata),
			remained_bits);

		sei_avgmaxrgb = cfd_bitvalue_get(metadata,
			17, &remained_bits, &addBytes);
		metadata += addBytes;
		cfd_md_info("avgmaxrgb:%d, bits=%d metadata=0x%x, 0x%x, 0x%x\n",
			sei_avgmaxrgb,
			remained_bits,
			*metadata, *(metadata + 1),
			*(metadata + 2));

		num_sei_dmaxrgb = cfd_bitvalue_get(metadata,
			4, &remained_bits, &addBytes);
		metadata += addBytes;
		cfd_md_info("axrgb:%d *(metadata)=0x%x remained_bits=%d\n",
			num_sei_dmaxrgb,
			*(metadata),
			remained_bits);
		memset(sei_dmaxrgb, 0, sizeof(sei_dmaxrgb));
		for (idx = 0; idx < num_sei_dmaxrgb; ++idx) {
			sei_dmaxrgb[idx] = cfd_bitvalue_get(metadata,
				24, &remained_bits, &addBytes);
			metadata += addBytes;
			sei_dmaxrgb_idx[idx] =
				(sei_dmaxrgb[idx] & 0xFE0000) >> 17;
			sei_dmaxrgb[idx] = sei_dmaxrgb[idx] & 0x1FFFF;
			cfd_md_info("maxrgb_idx:%d, value;%d (*metadata)=0x%x\n",
				sei_dmaxrgb_idx[idx],
				sei_dmaxrgb[idx],
				(*metadata));
		}

		cfd_md_info("after dmxrgb: remained_bits =%d, *metadata=0x%x\n",
			remained_bits,
			*metadata);

		/* 10bit */
		(void)cfd_bitvalue_get(metadata, 10, &remained_bits, &addBytes);
		metadata += addBytes;

		cfd_md_info("bright_pixel: remained_bits =%d, *metadata=0x%x\n",
			remained_bits,
			*metadata);
	}

	cfd_md_info("tone_mapping_flag: remained_bits =%d, *metadata=0x%x\n",
		remained_bits,
		*metadata);

	/* skip luminance flag 1bit */
	(void)cfd_bitvalue_get(metadata, 1, &remained_bits, &addBytes);
	metadata += addBytes;

	for (num_idx = 0; num_idx < num_windows; ++num_idx) {
			/*1bit */
		tone_mapping_flag = cfd_bitvalue_get(metadata,
			1, &remained_bits, &addBytes);
		metadata += addBytes;
		cfd_md_info("[%d, %d]flag:%d *metadata=0x%x rd_bits=%d\n",
			num_idx,
			num_windows,
			tone_mapping_flag,
			*metadata,
			remained_bits);
		memset(sei_bz_a, 0, sizeof(sei_bz_a));
		if (tone_mapping_flag) {
			value = cfd_bitvalue_get(metadata,
				24, &remained_bits, &addBytes);
			metadata += addBytes;
			sei_kpx = (value & 0xFFF000) >> 12;
			sei_kpy = (value & 0xFFF);
			cfd_md_info("[CFD]kpx:%d kpy:%d md=0x%x rd_bits=%d\n",
				sei_kpx,
				sei_kpy,
				*metadata,
				remained_bits);
			num_sei_bz_a = cfd_bitvalue_get(metadata,
				4, &remained_bits, &addBytes);
			metadata += addBytes;
			cfd_md_info("[CFD]num_sei_bz_a:%d md=0x%x rd_bits=%d\n",
				num_sei_bz_a,
				*metadata,
				remained_bits);

			for (idx = 0; idx < num_sei_bz_a; ++idx) {
				cfd_md_info("[CFD]*metadata=0x%x rd_bits=%d\n",
					*metadata,
					remained_bits);
				sei_bz_a[idx] = cfd_bitvalue_get(metadata,
					10, &remained_bits, &addBytes);
				metadata += addBytes;
				cfd_md_info("bz_a[%d]=%d md=0x%x rdbits=%d\n",
					idx, sei_bz_a[idx],
					*metadata,
					remained_bits);
			}
		}
		cfd_bitvalue_get(metadata, 1, &remained_bits, &addBytes);
		metadata += addBytes;
	}
	vsif_tsdmaxl = (uint8_t)cfd_vsif_min(sel_tsdmaxl, 1, 1024, 32, 31);
	vsif_avgmaxrgb =
	(uint8_t)cfd_vsif_min(sei_avgmaxrgb, 10, 4096, 16, 255);
	cfd_md_info("[CFD]vsif_tsdmaxl=%d vsif_avgmaxrgb=%d\n",
		vsif_tsdmaxl,
		vsif_avgmaxrgb);

	vsif[0] = (application_version << 6)|(vsif_tsdmaxl << 1);
	vsif[1] = vsif_avgmaxrgb;
	for (idx = 0; idx < 9; ++idx) {
		if ((idx != 2) || (application_version == 0)) {
			vsif_dmaxrgb[idx] =
			(uint8_t)cfd_vsif_min(sei_dmaxrgb[idx],
				10,
				4096,
				16,
				255);
			if ((idx == 8) &&
				(application_version == 0) &&
				(num_sei_dmaxrgb == 10))
				vsif_dmaxrgb[idx] =
				(uint8_t)cfd_vsif_min(sei_dmaxrgb[idx + 1],
					10,
					4096,
					16,
					255);
		} else
			vsif_dmaxrgb[idx] = (uint8_t)sei_dmaxrgb[idx];
		vsif[2 + idx] = vsif_dmaxrgb[idx];
	}
	vsif_kpx = cfd_vsif_min(sei_kpx, 1, 0xFFFF, 4, 1023);
	vsif_kpy = cfd_vsif_min(sei_kpy, 1, 0xFFFF, 4, 1023);
	cfd_md_info("sei_kpx=%d, vsif_kpx=%d, sei_kpy=%d, vsif_kpy=%d\n",
		sei_kpx,
		vsif_kpx,
		sei_kpy,
		vsif_kpy);
	vsif[11] = (num_sei_bz_a << 4) | ((vsif_kpx & 0x3C0) >> 6);
	vsif[12] = ((vsif_kpx & 0x3F) << 2) | ((vsif_kpy & 0x300) >> 8);
	vsif[13] = (vsif_kpy & 0xFF);
	for (idx = 0; idx < 9; ++idx) {
		vsif_bz_a[idx]	= (uint8_t)cfd_vsif_min(sei_bz_a[idx],
				1,
				0xFFFF,
				4,
				255);
		vsif[14+idx] = vsif_bz_a[idx];
		cfd_md_info("[CFD]vsif_bz_a=%d, sei_bz_a=%d\n",
			sei_bz_a[idx],
			vsif_bz_a[idx]);
	}

	if (ctl_info & CFD_CTRLBIT_HDR10PLUS_GFX_OVERLAY)
		graphic_overlay_flag = 1;
	if (ctl_info & CFD_CTRLBIT_HDR10PLUS_ASYNC)
		vsif_timing_mode = 0;
	else
		vsif_timing_mode = 1;
	vsif[23] = (graphic_overlay_flag << 7) | (vsif_timing_mode << 6);

	return 1;
}

uint8_t disp_cfd_parser_t35_metadata(uint8_t *hdr_metadata,
	uint32_t size,
	struct STU_CFD_HDR10PLUS_SEI *hdr10plus_sei)
{
	uint8_t country_code = 0;
	uint8_t application_identifier = 0;
	uint8_t application_version = 0;
	uint8_t num_windows = 0;
	uint8_t num_idx = 0;
	uint8_t idx = 0;
	uint32_t sel_tsdmaxl = 0;
	uint32_t sei_avgmaxrgb = 0;
	uint8_t num_sei_dmaxrgb = 0;
	uint32_t sei_dmaxrgb[16];/* distributtion */
	uint8_t sei_dmaxrgb_idx[16];/* distributtion */
	uint8_t tone_mapping_flag = 0;
	uint32_t sei_kpx = 0;
	uint32_t sei_kpy = 0;
	uint8_t num_sei_bz_a = 0;
	uint32_t sei_bz_a[9];
	int32_t remained_bits;
	uint32_t addBytes;
	uint32_t value;
	uint8_t *metadata = NULL;

	if ((hdr_metadata == NULL) || (hdr10plus_sei == NULL))
		return 0;
	metadata = hdr_metadata;
	country_code = *metadata;
	hdr10plus_sei->u8Itu_t_t35_country_code = country_code;
	//skip country code, privider_code, provider(u(8)+u(16)+u(16))
	metadata += 5;
	application_identifier = *metadata;
	application_version = *(metadata + 1);
	metadata += 2;
	num_windows = (((*metadata) & 0xC0) >> 6);
	remained_bits = 6;

	hdr10plus_sei->u8Application_Version = application_version;
	hdr10plus_sei->u8Application_Identifier = application_identifier;
	hdr10plus_sei->u8Num_Windows = num_windows;

	/* 27 bit */
	sel_tsdmaxl = cfd_bitvalue_get(metadata, 27, &remained_bits, &addBytes);
	hdr10plus_sei->u32Target_System_Display_Max_Luminance = sel_tsdmaxl;

	metadata += addBytes; /* 2bit */
	cfd_md_info("[CFD]code:0x%x num_wins:%d tsdmaxl:0x%x metadata=0x%x\n",
		country_code,
		num_windows,
		sel_tsdmaxl,
		*metadata);
	remained_bits = 2; /* skip 1 bit for tsdaplflag */
	for (num_idx = 0; num_idx < num_windows; ++num_idx) {
		cfd_md_info("[CFD]num_idx[%d] *(md)=0x%x rd_bits=%d\n",
			num_idx,
			*(metadata),
			remained_bits);

		/* skip maxsel */
		hdr10plus_sei->u32MaxSCL[0] =
		cfd_bitvalue_get(metadata, 17, &remained_bits, &addBytes);
		metadata += addBytes;
		hdr10plus_sei->u32MaxSCL[1] =
		cfd_bitvalue_get(metadata, 17, &remained_bits, &addBytes);
		metadata += addBytes;
		hdr10plus_sei->u32MaxSCL[2] =
		cfd_bitvalue_get(metadata, 17, &remained_bits, &addBytes);
		metadata += addBytes;

		cfd_md_info("checkmaxScl *(metadata)=0x%x remained_bits=%d\n",
			*(metadata),
			remained_bits);

		sei_avgmaxrgb = cfd_bitvalue_get(metadata,
			17, &remained_bits, &addBytes);
		hdr10plus_sei->u32Average_MaxRGB = sei_avgmaxrgb;

		metadata += addBytes;
		cfd_md_info("avgmaxrgb:%d, bits=%d metadata=0x%x, 0x%x, 0x%x\n",
			sei_avgmaxrgb,
			remained_bits,
			*metadata, *(metadata + 1),
			*(metadata + 2));

		num_sei_dmaxrgb = cfd_bitvalue_get(metadata,
			4, &remained_bits, &addBytes);
		metadata += addBytes;
		cfd_md_info("axrgb:%d *(metadata)=0x%x remained_bits=%d\n",
			num_sei_dmaxrgb,
			*(metadata),
			remained_bits);
		memset(sei_dmaxrgb, 0, sizeof(sei_dmaxrgb));
		for (idx = 0; idx < num_sei_dmaxrgb; ++idx) {
			sei_dmaxrgb[idx] = cfd_bitvalue_get(metadata,
				24, &remained_bits, &addBytes);
			metadata += addBytes;
			sei_dmaxrgb_idx[idx] =
				(sei_dmaxrgb[idx] & 0xFE0000) >> 17;
			sei_dmaxrgb[idx] = sei_dmaxrgb[idx] & 0x1FFFF;
			cfd_md_info("maxrgb_idx:%d, value;%d (*metadata)=0x%x\n",
				sei_dmaxrgb_idx[idx],
				sei_dmaxrgb[idx],
				(*metadata));
			hdr10plus_sei->u8Distribution_Index[idx] =
				sei_dmaxrgb_idx[idx];
			hdr10plus_sei->u32Distribution_Values[idx] =
				sei_dmaxrgb[idx];
		}

		cfd_md_info("after dmxrgb: remained_bits =%d, *metadata=0x%x\n",
			remained_bits,
			*metadata);

		/* 10bit */
		(void)cfd_bitvalue_get(metadata, 10, &remained_bits, &addBytes);
		metadata += addBytes;

		cfd_md_info("bright_pixel: remained_bits =%d, *metadata=0x%x\n",
			remained_bits,
			*metadata);
	}

	cfd_md_info("tone_mapping_flag: remained_bits =%d, *metadata=0x%x\n",
		remained_bits,
		*metadata);

	/* skip luminance flag 1bit */
	(void)cfd_bitvalue_get(metadata, 1, &remained_bits, &addBytes);
	metadata += addBytes;

	for (num_idx = 0; num_idx < num_windows; ++num_idx) {
			/*1bit */
		tone_mapping_flag = cfd_bitvalue_get(metadata,
			1, &remained_bits, &addBytes);
		metadata += addBytes;
		cfd_md_info("[%d, %d]flag:%d *metadata=0x%x rd_bits=%d\n",
			num_idx,
			num_windows,
			tone_mapping_flag,
			*metadata,
			remained_bits);
		memset(sei_bz_a, 0, sizeof(sei_bz_a));
		if (tone_mapping_flag) {
			value = cfd_bitvalue_get(metadata,
				24, &remained_bits, &addBytes);
			metadata += addBytes;
			sei_kpx = (value & 0xFFF000) >> 12;
			sei_kpy = (value & 0xFFF);
			hdr10plus_sei->u16Knee_Point_x = sei_kpx;
			hdr10plus_sei->u16Knee_Point_y = sei_kpy;
			cfd_md_info("[CFD]kpx:%d kpy:%d md=0x%x rd_bits=%d\n",
				sei_kpx,
				sei_kpy,
				*metadata,
				remained_bits);
			num_sei_bz_a = cfd_bitvalue_get(metadata,
				4, &remained_bits, &addBytes);
			metadata += addBytes;
			cfd_md_info("[CFD]num_sei_bz_a:%d md=0x%x rd_bits=%d\n",
				num_sei_bz_a,
				*metadata,
				remained_bits);

			hdr10plus_sei->u8Num_Bezier_Curve_Anchors =
				num_sei_bz_a;
			for (idx = 0; idx < num_sei_bz_a; ++idx) {
				cfd_md_info("[CFD]*metadata=0x%x rd_bits=%d\n",
					*metadata,
					remained_bits);
				sei_bz_a[idx] = cfd_bitvalue_get(metadata,
					10, &remained_bits, &addBytes);
				metadata += addBytes;
				cfd_md_info("bz_a[%d]=%d md=0x%x rdbits=%d\n",
					idx, sei_bz_a[idx],
					*metadata,
					remained_bits);
				hdr10plus_sei->u16Bezier_Curve_Anchors[idx] =
					sei_bz_a[idx];
			}
		}
		cfd_bitvalue_get(metadata, 1, &remained_bits, &addBytes);
		metadata += addBytes;
	}

	return 1;
}

int disp_cfd_drv_fill_dyn_md_in_sec(enum VID_PLA_DR_TYPE_T type,
	struct VID_PLA_HDR_METADATA_INFO_T *rHdr,
	void *buf_add)
{
	unsigned char vsif[32];
	unsigned int vsiflen = 0;
	struct cfd_share_memory_info_t *p_share_mem =
		cfd_sec_get_share_mem(0);

	if (p_share_mem == NULL)
		return CFD_RET_ERROR;
	memset(vsif, 0, sizeof(vsif));
	vsif[0] = 0x8B;
	vsif[1] = 0x84;
	vsif[2] = 0x90;
	vsif[3] =
	((p_share_mem->hdr10plus_vsif.u8Application_Version << 6) |
	(p_share_mem->hdr10plus_vsif.u8Target_System_Display_Max_Luminance
	<< 1));
	vsif[4] = p_share_mem->hdr10plus_vsif.u8Average_MaxRGB;
	vsif[5] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[0];
	vsif[6] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[1];
	vsif[7] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[2];
	vsif[8] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[3];
	vsif[9] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[4];
	vsif[10] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[5];
	vsif[11] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[6];
	vsif[12] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[7];
	vsif[13] = p_share_mem->hdr10plus_vsif.u8Distribution_Values[8];
	vsif[14] =
	((p_share_mem->hdr10plus_vsif.u8Num_Bezier_Curve_Anchors << 4) |
	((p_share_mem->hdr10plus_vsif.u16Knee_Point_x & 0x03C0) >> 6));
	vsif[15] =
	(((p_share_mem->hdr10plus_vsif.u16Knee_Point_x & 0x003F) << 2) |
	((p_share_mem->hdr10plus_vsif.u16Knee_Point_y & 0x0300) >> 8));
	vsif[16] = (p_share_mem->hdr10plus_vsif.u16Knee_Point_y & 0xFF);
	vsif[17] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[0];
	vsif[18] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[1];
	vsif[19] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[2];
	vsif[20] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[3];
	vsif[21] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[4];
	vsif[22] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[5];
	vsif[23] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[6];
	vsif[24] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[7];
	vsif[25] = p_share_mem->hdr10plus_vsif.u8Bezier_Curve_Anchors[8];
	if ((_cfd_inst[CFD_LAYER_VDO_FE0].feature_ctrlbit & 0x1) == 0)
		p_share_mem->hdr10plus_vsif.u8No_Delay_Flag = 1;
	/* disable graphic_overlay_flag*/
	//if (_cfd_inst[CFD_LAYER_GFX_FE0].enabled ||
	//	_cfd_inst[CFD_LAYER_GFX_FE1].enabled)
	//	p_share_mem->hdr10plus_vsif.u8Graphics_Overlay_Flag = 1;
	vsif[26] =
	((p_share_mem->hdr10plus_vsif.u8Graphics_Overlay_Flag << 7) |
	(p_share_mem->hdr10plus_vsif.u8No_Delay_Flag << 6));

	vsiflen = 27;
	if (buf_add != NULL)
		memcpy(buf_add, vsif, vsiflen);

	return CFD_RET_OK;
}

int disp_cfd_drv_fill_dyn_metadata(enum VID_PLA_DR_TYPE_T type,
	struct VID_PLA_HDR_METADATA_INFO_T *rHdr)
{
	unsigned char vsif[32];
	unsigned int vsiflen = 0;
	unsigned int hdr10plusctlbit = 0;
	unsigned int len = 0;
	void *buf_add = NULL;
	struct video_buffer_info *vid_info = NULL;
	struct VID_HDR10_PLUS_METADATA_INFO_T *phdr10p_metadata_info;

	vid_info = disp_hdr_get_vid_info(0);
	if (vid_info != NULL) {
		buf_add = vid_info->hdr_info.metadata_info.dovi_metadata.buff;
		len = vid_info->hdr_info.metadata_info.dovi_metadata.len;
	}

	rHdr->fgIsMetadata = true;
	rHdr->e_DynamicRangeType = type;
	phdr10p_metadata_info =
		&(rHdr->metadata_info.hdr10_plus_metadata.hdr10p_metadata_info);

	if (type == VID_PLA_DR_TYPE_HDR10_PLUS_VSIF) {
		if (vid_info->hdr_info.metadata_info.dovi_metadata.svp) {
			// call tz and parser
			buf_add =
			vid_info->hdr_info.metadata_info.dovi_metadata.buff;
			disp_cfd_drv_fill_dyn_md_in_sec(type, rHdr, buf_add);
			phdr10p_metadata_info->ui4_Hdr10PlusAddr =
				(unsigned long)buf_add;
			phdr10p_metadata_info->ui4_Hdr10PlusSize = 27;
		} else {
			memset(vsif, 0, sizeof(vsif));
			vsif[0] = 0x8B;
			vsif[1] = 0x84;
			vsif[2] = 0x90;

			/* add vsif control bit   */
			hdr10plusctlbit =
			_cfd_inst[CFD_LAYER_VDO_FE0].feature_ctrlbit;
			/* disable graphic_overlay_flag */
			//if (_cfd_inst[CFD_LAYER_GFX_FE0].enabled ||
			//	_cfd_inst[CFD_LAYER_GFX_FE1].enabled)
			//	hdr10plusctlbit |=
			//	CFD_CTRLBIT_HDR10PLUS_GFX_OVERLAY;
			cfd_user_data_registered_inu_t_t35_new(buf_add,
				len, &(vsif[3]), hdr10plusctlbit);
			vsiflen = 27;
			memcpy(buf_add, vsif, vsiflen);

			phdr10p_metadata_info->ui4_Hdr10PlusAddr =
				(unsigned long)buf_add;
			phdr10p_metadata_info->ui4_Hdr10PlusSize = vsiflen;
		}
	} else {
		phdr10p_metadata_info->ui4_Hdr10PlusAddr =
			(unsigned long)buf_add;
		phdr10p_metadata_info->ui4_Hdr10PlusSize = len;
	}
	return CFD_RET_OK;
}

int disp_cfd_drv_set_forcehdr(uint32_t layer_id,
	bool force_openhdr)
{
	uint32_t ret = CFD_RET_OK;

	_cfd_inst[layer_id].force_hdr = force_openhdr;

	return ret;
}

int disp_cfd_drv_set_user_gfx_overlay(uint32_t layer_id,
	bool gfx_overlay)
{
	uint32_t ret = CFD_RET_OK;

	if (gfx_overlay)
		_cfd_inst[layer_id].feature_ctrlbit |=
		CFD_CTRLBIT_HDR10PLUS_GFX_OVERLAY;

	return ret;
}

int disp_cfd_drv_set_frame_async(uint32_t layer_id,
	bool md_async)
{
	uint32_t ret = CFD_RET_OK;

	if (md_async)
		_cfd_inst[layer_id].feature_ctrlbit |=
		CFD_CTRLBIT_HDR10PLUS_ASYNC;

	return ret;
}

bool disp_cfd_drv_get_frame_async(uint32_t layer_id)
{
	bool ret = false;

	if (_cfd_inst[layer_id].feature_ctrlbit &
		CFD_CTRLBIT_HDR10PLUS_ASYNC)
		ret = true;

	return ret;
}

int disp_cfd_drv_dump_status(uint32_t layer_id)
{
	cfd_default("CFD[%d] status:\n", layer_id);
	cfd_default("Init:%d,En %d,Force:%d,No:%d,Type:%d,Conf:%d,St:%d\n",
		_cfd_inst[layer_id].inited,
		_cfd_inst[layer_id].enabled,
		_cfd_inst[layer_id].force_hdr,
		_cfd_inst[layer_id].frame_no,
		_cfd_inst[layer_id].feature_type,
		_cfd_inst[layer_id].reg_conf_mode,
		_cfd_inst[layer_id].cfd_state);
	cfd_default("Src:%d %d,HDR:%d %d\n",
		_cfd_inst[layer_id].src_width,
		_cfd_inst[layer_id].src_heght,
		_cfd_inst[layer_id].hdr_type,
		_cfd_inst[layer_id].src_bt2020);
	cfd_default("Res:%d\n",
		_cfd_inst[layer_id].res);

	return CFD_RET_OK;
}

int disp_cfd_drv_set_feature_ctrlbit(uint32_t layer_id,
	uint32_t ctrl_bit)
{
	uint32_t ret = CFD_RET_OK;

	_cfd_inst[layer_id].feature_ctrlbit = ctrl_bit;

	return ret;
}

int disp_cfd_drv_set_feature_type(uint32_t layer_id,
	enum DISP_CFD_FEATURE_E feture_type)
{
	uint32_t ret = CFD_RET_OK;

	_cfd_inst[layer_id].feature_type = feture_type;

	return ret;
}

int disp_cfd_drv_get_video_dm_wh(uint32_t layer_id,
	uint32_t *dm_w, uint32_t *dm_h)
{
	disp_cfd_hal_get_video_dm_wh(layer_id, dm_w, dm_h);

	return CFD_RET_OK;
}

int disp_cfd_drv_get_gfx_dm_h(uint32_t layer_id,
	uint32_t *dm_h)
{
	disp_cfd_hal_get_gfx_dm_h(layer_id, dm_h);

	return CFD_RET_OK;
}

int disp_cfd_drv_set_video_wh(uint32_t layer_id,
	uint32_t width, uint32_t height)
{
	struct ml_reg_table reg_tbl;
	uint32_t reg_addr[4] = { 0 };
	uint16_t reg_val[4] = { 0 };
	uint16_t reg_mask[4] = { 0xffff, 0xffff, 0xffff, 0xffff };

	if (layer_id == 0) {
		reg_addr[0] = 0x150129cc;
		reg_addr[1] = 0x150129cc;
		reg_addr[2] = 0x150129d0;
		reg_addr[3] = 0x150129d0;
	} else {
		reg_addr[0] = 0x150159cc;
		reg_addr[1] = 0x150159cc;
		reg_addr[2] = 0x150159d0;
		reg_addr[3] = 0x150159d0;
	}

	reg_val[0] = width;
	reg_val[1] = width;
	reg_val[2] = height;
	reg_val[3] = height;

	memset(&reg_tbl, 0, sizeof(struct ml_reg_table));
	reg_tbl.ml_ip = ML_DSYS_IP;
	reg_tbl.depth = 4;
	reg_tbl.p_reg_addr = reg_addr;
	reg_tbl.p_reg_value = reg_val;
	reg_tbl.p_reg_mask = reg_mask;
	reg_tbl.reg_type = 1;
	reg_tbl.path = ML_OPENHDR;

	disp_ml_write_reg_multi(&reg_tbl);

	return CFD_RET_OK;
}

int disp_cfd_drv_set_gfx_h(uint32_t layer_id,
	uint32_t height)
{
	struct ml_reg_table reg_tbl;
	uint32_t reg_addr[4] = { 0 };
	uint16_t reg_val[4] = { 0 };
	uint16_t reg_mask[4] = { 0xffff, 0xffff, 0xffff, 0xffff };

	if (layer_id == 0) {
		reg_addr[0] = 0x14009468;
		reg_addr[1] = 0x14009468;
		reg_addr[2] = 0x14009468;
		reg_addr[3] = 0x14009468;
	} else {
		reg_addr[0] = 0x1400a468;
		reg_addr[1] = 0x1400a468;
		reg_addr[2] = 0x1400a468;
		reg_addr[3] = 0x1400a468;
	}

	reg_val[0] = height;
	reg_val[1] = height;
	reg_val[2] = height;
	reg_val[3] = height;

	memset(&reg_tbl, 0, sizeof(struct ml_reg_table));
	reg_tbl.ml_ip = ML_MSYS_IP;
	reg_tbl.depth = 4;
	reg_tbl.p_reg_addr = reg_addr;
	reg_tbl.p_reg_value = reg_val;
	reg_tbl.p_reg_mask = reg_mask;
	reg_tbl.reg_type = 1;
	reg_tbl.path = ML_OPENHDR;

	disp_ml_write_reg_multi(&reg_tbl);

	return CFD_RET_OK;
}


