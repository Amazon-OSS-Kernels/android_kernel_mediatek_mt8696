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

#define LOG_TAG "R2R"

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
#include "disp_r2r_debug.h"
#include "disp_r2r_drv.h"
#include "disp_r2r_if.h"



#define R2R_DTS_MAX_ND_SIZE 50
char r2r_dts_nd_name[R2R_DTS_MAX_ND_SIZE] = "mediatek,mt8696-r2r";
uintptr_t _disp_r2r_hw_base;
struct r2r_layer_info r2r_layer;

struct r2r_layer_info  *get_r2r_layer_info(void)
{
	return &r2r_layer;
}
static void _r2r_mutex_init(struct mutex *lock)
{
	mutex_init(lock);
}

static void _r2r_mutex_lock(struct mutex *lock)
{
	mutex_lock(lock);
}

static void _r2r_mutex_unlock(struct mutex *lock)
{
	mutex_unlock(lock);
}

static int _r2r_parse_dev_node(void)
{
	struct device_node *np;
	unsigned int reg_value;
	unsigned int irq_value;
	unsigned int irq_no;
	char nd_name[R2R_DTS_MAX_ND_SIZE];
	struct disp_hw *r2r_drv = disp_r2r_get_drv();

	sprintf(nd_name, "%s", r2r_dts_nd_name);

	np = of_find_compatible_node(NULL, NULL, nd_name);
	if (np == NULL) {
		r2r_error("dts error, no r2r device node %s.\n", nd_name);
		return R2R_RET_ERROR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);
	of_property_read_u32_index(np, "interrupts", 1, &irq_value);

	/* get r2r reg base 0x15006000 */
	_disp_r2r_hw_base = (uintptr_t)of_iomap(np, 0);
	irq_no = irq_of_parse_and_map(np, 0);

	if (irq_no == 0)
		r2r_error("r2r get irq from dts fail\n");
	else {
		r2r_drv->irq[0].value = irq_no;
		r2r_drv->irq[0].irq = DISP_IRQ_R2R_VSYNC;
		r2r_drv->irq_num = 1;
	}
	r2r_info("%s addr 0x%p.\n", __func__, (void *)_disp_r2r_hw_base);

	return R2R_RET_OK;
}

void disp_r2r_release_buffer(struct work_struct *work)
{

}

static int disp_r2r_init(struct disp_hw_common_info *info)
{
	r2r_info("%s begin\n", __func__);
	_r2r_parse_dev_node();
	// layer init
	memset((void *)(&r2r_layer), 0, sizeof(struct r2r_layer_info));
	r2r_layer.layer_id = 0;
	INIT_LIST_HEAD(&(r2r_layer.buf_list));
	_r2r_mutex_init(&(r2r_layer.sync_lock));
	r2r_layer.inited = 1;
	r2r_layer.timeline_idx = 0;
	r2r_layer.release_timeline_idx = 0;
	r2r_layer.release_idx = 0;
	r2r_layer.last_pts = 0xffffffff;
	r2r_layer.fence_idx = 0;
	r2r_layer.display_duration = 0;
	r2r_layer.vsync_duration = 0;
	r2r_layer.enable = false;
	r2r_layer.layer_start = false;
	r2r_layer.state = R2R_LAYER_IDLE;
	r2r_layer.video_buf = NULL;
	INIT_WORK(&r2r_layer.task_work, disp_r2r_release_buffer);
	init_waitqueue_head(&r2r_layer.wait_queue);

	disp_r2r_drv_init(_disp_r2r_hw_base);
	disp_r2r_drv_set_gce_handle((void *)info->gce_handle);
	r2r_debug_init();

	r2r_info("%s done\n", __func__);
	return R2R_RET_OK;
}


int disp_r2r_config(struct mtk_disp_buffer *config,
		    struct disp_hw_common_info *info)
{
	int ret = R2R_RET_OK;
	struct r2r_video_buffer_info *buf_info = NULL;

	if ((r2r_layer.state != R2R_LAYER_START) &&
	    (r2r_layer.state != R2R_LAYER_RUNNING)) {
		return R2R_NOT_START;
	}

	if (config->tgt.width > info->resolution->width ||
	    config->tgt.height > info->resolution->height) {
		r2r_error("the tgt size is error (%d %d) res (%d %d)\n",
			   config->tgt.width, config->tgt.height,
			   info->resolution->width, info->resolution->height);
		return R2R_RET_ERROR;
	}

	/* get video_buffer_info to store information in mtk_disp_buffer */
	buf_info = r2r_get_buf_info();
	if (buf_info == NULL) {
		r2r_error("get layer input buffer fail\n");
		return R2R_RET_ERROR;
	}

	/*insert the video buffer to the buffer list */
	buf_info->buf_state = BUFFER_INSERT;
	_r2r_mutex_lock(&(r2r_layer.sync_lock));
	list_add_tail(&buf_info->list,
		      &(r2r_layer.buf_list));
	r2r_layer.last_pts = config->pts;

	if (1) {
		// release ion buffer
		goto release_ion_handle;
	}
	_r2r_mutex_unlock(&(r2r_layer.sync_lock));

	return ret;

release_ion_handle:
	/* clean up allocated buffer */
	r2r_default("r2r layer config fail with error parameter, drop it\n");

	/* buf_info is not add to any list yet, no need to lock. */
	list_del_init(&buf_info->list);
	r2r_release_buf_info(&buf_info->list);

	return R2R_RET_ERROR;
}

static int disp_r2r_deinit(void)
{
	r2r_info("%s start\n", __func__);
	// path off
	disp_r2r_drv_path_sel(false);
	// hw off
	disp_r2r_drv_enable(false, 0);
	// clk off
	disp_r2r_set_clk_enable(0);
	r2r_debug_deinit();
	disp_r2r_drv_uninit();
	return R2R_RET_OK;
}

int disp_r2r_start(struct disp_hw_common_info *info,
	unsigned int layer_id)
{
	//clk on
	disp_r2r_set_clk_enable(1);
	disp_r2r_drv_set_sof();
	//path on
	disp_r2r_drv_path_sel(true);
	return R2R_RET_OK;
}

int disp_r2r_stop(unsigned int layer_id)
{
	//path off
	disp_r2r_drv_path_sel(false);

	//clk on
	disp_r2r_set_clk_enable(0);
	return R2R_RET_OK;
}

int disp_r2r_suspend(void)
{
	disp_r2r_drv_enable(false, 0);
	disp_r2r_set_clk_enable(0);

	return R2R_RET_OK;
}

int disp_r2r_resume(void)
{
	enum HDMI_VIDEO_RESOLUTION res;

	disp_r2r_set_clk_enable(1);
	disp_r2r_drv_set_sof();
	disp_r2r_drv_get_resolution(0, &res);
	disp_r2r_drv_set_resolution(0, res);
	disp_r2r_drv_enable(true, 1);
	return R2R_RET_OK;
}

int disp_r2r_chg_resolution(const struct disp_hw_resolution *info)
{
	if (info != NULL)
		disp_r2r_drv_set_resolution(0, info->res_mode);
	return R2R_RET_OK;
}

int disp_r2r_irq_handler(uint32_t irq)
{
	switch (irq) {
	case DISP_IRQ_R2R_VSYNC:
		// clear irq
		disp_r2r_drv_clr_irq(irq);
		disp_r2r_drv_flush_reg(0);
		break;
	default:
		break;
	}
	return R2R_RET_OK;
}

int disp_r2r_set_cmd(enum DISP_CMD cmd, void *data)
{
	struct r2r_video_buffer_info *buf_info;

	switch (cmd) {
	case DISP_CMD_R2R_SRC_TIMING:
		disp_r2r_chg_resolution(
			(const struct disp_hw_resolution *)data);
		break;

	case DISP_CMD_R2R_CONFIG_FRAME:
		buf_info = (struct r2r_video_buffer_info *)data;
		disp_r2r_drv_config_frame(buf_info);
		break;

	default:
		break;
	}
	return R2R_RET_OK;
}

int disp_r2r_gce_trigger(void)
{
	int ret = R2R_RET_OK;

	ret = disp_r2r_drv_gce_trigger();
	return ret;
}

/*****************r2r driver****************/
struct disp_hw disp_r2r_driver = {
	.name = R2R_DRV_NAME,
	.init = disp_r2r_init,
	.deinit = disp_r2r_deinit,
	.start = NULL,
	.stop = NULL,
	.suspend = NULL,
	.resume = NULL,
	.get_info = NULL,
	.change_resolution = NULL,
	.config = NULL,
	.config_ex = NULL,
	.irq_handler = NULL,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = NULL,
	.set_cmd = NULL,
	.gce_trigger = NULL,
};

struct disp_hw *disp_r2r_get_drv(void)
{
	return &disp_r2r_driver;
}

