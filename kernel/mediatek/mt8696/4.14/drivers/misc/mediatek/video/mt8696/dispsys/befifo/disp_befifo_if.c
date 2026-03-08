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

#define LOG_TAG "BEFIFO"

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
#include "disp_befifo_debug.h"
#include "disp_befifo_drv.h"
#include "disp_befifo_if.h"
#include <linux/delay.h>



#define BEFIFO_DTS_MAX_ND_SIZE 50
char befifo_dts_nd_name[BEFIFO_DTS_MAX_ND_SIZE] = "mediatek,mt8696-befifo";
uintptr_t _disp_befifo_hw_base;
#ifdef CONFIG_HDMI_BLACK
bool fg_be_fifo_deep_suspend;
#endif

static int _befifo_parse_dev_node(void)
{
	struct device_node *np;
	unsigned int reg_value;
	unsigned int irq_value;
	unsigned int irq_no;
	char nd_name[BEFIFO_DTS_MAX_ND_SIZE];
	struct disp_hw *befifo_drv = disp_befifo_get_drv();

	sprintf(nd_name, "%s", befifo_dts_nd_name);

	np = of_find_compatible_node(NULL, NULL, nd_name);
	if (np == NULL) {
		befifo_error("dts error, no befifo device node %s.\n", nd_name);
		return BEFIFO_RET_ERROR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);
	of_property_read_u32_index(np, "interrupts", 1, &irq_value);

	/* get befifo reg base 0x1400f000 */
	_disp_befifo_hw_base = (uintptr_t)of_iomap(np, 0);
	irq_no = irq_of_parse_and_map(np, 0);

	if (irq_no == 0)
		befifo_error("befifo get irq from dts fail\n");
	else {
		befifo_drv->irq[0].value = irq_no;
		befifo_drv->irq[0].irq = DISP_IRQ_BEFIFO;
		befifo_drv->irq_num = 1;
	}

	befifo_info("befifo va addr %p\n", (void *)_disp_befifo_hw_base);

	return BEFIFO_RET_OK;
}

static int disp_befifo_init(struct disp_hw_common_info *info)
{
	befifo_info("%s begin\n", __func__);

	_befifo_parse_dev_node();
	if ((info != NULL) && (info->resolution != NULL))
		disp_befifo_drv_init(_disp_befifo_hw_base,
		info->resolution->res_mode);

#ifdef DISP_GCE_SUPPORT
	if ((info != NULL) && (info->gce_handle != NULL))
		disp_befifo_drv_set_gce_handle((void *)info->gce_handle);
#endif
	befifo_debug_init();
	// clk on
	disp_befifo_drv_set_clk_enable(1);
	disp_befifo_drv_set_sof();
	// path on
	// timing on
	if ((info != NULL) && (info->resolution != NULL))
		disp_befifo_drv_set_resolution(0, info->resolution->res_mode);
	// hw on
	disp_befifo_drv_enable(true, 0);
	befifo_info("%s done\n", __func__);
	return BEFIFO_RET_OK;
}

static int disp_befifo_deinit(void)
{
	befifo_default("%s start\n", __func__);
	// path off
	disp_befifo_drv_set_sof();

	// hw off
	disp_befifo_drv_enable(false, 0);

	// clk off
	befifo_debug_deinit();
	disp_befifo_drv_uninit();
	return BEFIFO_RET_OK;
}

int disp_befifo_start(struct disp_hw_common_info *info,
	unsigned int layer_id)
{

	return BEFIFO_RET_OK;
}

int disp_befifo_stop(unsigned int layer_id)
{

	return BEFIFO_RET_OK;
}

#ifdef CONFIG_HDMI_BLACK
int disp_befifo_deep_suspend(void)
{
	disp_befifo_drv_enable(false, 0);
	disp_befifo_drv_set_clk_enable(0);
	fg_be_fifo_deep_suspend = true;
	befifo_default("deep suspend\n");
	return BEFIFO_RET_OK;
}
#endif

int disp_befifo_suspend(void)
{
	if (!disp_common_info.low_energy_dozing_mode_enable) {
		disp_befifo_drv_enable(false, 0);
		disp_befifo_drv_set_clk_enable(0);
	}
	return BEFIFO_RET_OK;
}

int disp_befifo_resume(void)
{
	enum HDMI_VIDEO_RESOLUTION res;

#ifdef CONFIG_HDMI_BLACK
	if (fg_be_fifo_deep_suspend) {
		disp_befifo_drv_set_clk_enable(1);
		disp_befifo_drv_set_sof();
		disp_befifo_drv_get_resolution(0, &res);
		disp_befifo_drv_set_resolution(0, res);
		disp_befifo_drv_enable(true, 1);
		befifo_default("deep resume\n");
		fg_be_fifo_deep_suspend = false;
	} else {
		if (!disp_common_info.low_energy_dozing_mode_enable) {
			disp_befifo_drv_set_clk_enable(1);
			disp_befifo_drv_set_sof();
			disp_befifo_drv_get_resolution(0, &res);
			disp_befifo_drv_set_resolution(0, res);
			disp_befifo_drv_enable(true, 1);
		}
	}
#else
	if (!disp_common_info.low_energy_dozing_mode_enable) {
		disp_befifo_drv_set_clk_enable(1);
		disp_befifo_drv_set_sof();
		disp_befifo_drv_get_resolution(0, &res);
		disp_befifo_drv_set_resolution(0, res);
		disp_befifo_drv_enable(true, 1);
	}
#endif
	return BEFIFO_RET_OK;
}

int disp_befifo_chg_resolution(const struct disp_hw_resolution *info)
{
	if (info != NULL) {
		disp_befifo_drv_set_resolution(0, info->res_mode);
		disp_befifo_drv_enable(true, 1);
	}
	return BEFIFO_RET_OK;
}

int disp_befifo_irq_handler(uint32_t irq)
{
	switch (irq) {
	case DISP_IRQ_BEFIFO:
		befifo_isr("irq %d", irq);
		// clear irq switch to vdou_sys
		//disp_befifo_drv_clr_irq(irq);
		disp_befifo_drv_flush_reg(0);
		break;
	default:
		break;
	}
	return BEFIFO_RET_OK;
}

int disp_befifo_set_cmd(enum DISP_CMD cmd, void *data)
{
	struct BEFIFO_ACTIVE_SHIFT_T *pt_shift_value = NULL;

	switch (cmd) {
	case DISP_CMD_BEFIFO_CHG_TIMING:
		disp_befifo_chg_resolution(
			(const struct disp_hw_resolution *)data);
		break;

	case DISP_CMD_BEFIFO_SHIFT:
		pt_shift_value = (struct BEFIFO_ACTIVE_SHIFT_T *)data;
		disp_befifo_drv_adj_front(pt_shift_value);
		break;

	default:
		break;
	}
	return BEFIFO_RET_OK;
}

int disp_befifo_gce_trigger(struct disp_hw_trigger_info *info)
{
	int ret = BEFIFO_RET_OK;

	ret = disp_befifo_drv_gce_trigger();
	return ret;
}

/*****************befifo driver****************/
struct disp_hw disp_befifo_driver = {
	.name = BEFIFO_DRV_NAME,
	.init = disp_befifo_init,
	.deinit = disp_befifo_deinit,
	.start = disp_befifo_start,
	.stop = disp_befifo_stop,

	#ifdef CONFIG_HDMI_BLACK
	.deep_suspend = disp_befifo_deep_suspend,
	.deep_resume = NULL,
	#endif
	.suspend = disp_befifo_suspend,
	.resume = disp_befifo_resume,
	.get_info = NULL,
	.change_resolution = disp_befifo_chg_resolution,
	.config = NULL,
	.config_ex = NULL,
	.irq_handler = disp_befifo_irq_handler,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = NULL,
	.set_cmd = disp_befifo_set_cmd,
	.gce_trigger = disp_befifo_gce_trigger,
};

struct disp_hw *disp_befifo_get_drv(void)
{
	return &disp_befifo_driver;
}

