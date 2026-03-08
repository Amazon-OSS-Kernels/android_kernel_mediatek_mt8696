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

#define LOG_TAG "FEFIFO"

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
#include "disp_fefifo_if.h"



#define FEFIFO_DTS_MAX_ND_SIZE 50
char fefifo_dts_nd_name[FEFIFO_DTS_MAX_ND_SIZE] = "mediatek,mt8696-fefifo";
uintptr_t _disp_fefifo_hw_base[FEFIFO_MAX_LAYER_NUM] = {0};

static int _fefifo_parse_dev_node(void)
{
	struct device_node *np;
	unsigned int reg_value;
	unsigned int irq_value;
	unsigned int irq_no;
	char nd_name[FEFIFO_DTS_MAX_ND_SIZE];
	struct disp_hw *fefifo_drv = disp_fefifo_get_drv();

	sprintf(nd_name, "%s", fefifo_dts_nd_name);

	np = of_find_compatible_node(NULL, NULL, nd_name);
	if (np == NULL) {
		fefifo_error("dts error, no fefifo device node %s.\n", nd_name);
		return FEFIFO_RET_ERROR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);
	of_property_read_u32_index(np, "interrupts", 1, &irq_value);

	/* get fefifo reg base 0x14010000 */
	_disp_fefifo_hw_base[2] = (uintptr_t)of_iomap(np, 0);
	/* get fefifo reg base 0x14011000 */
	_disp_fefifo_hw_base[3] = (uintptr_t)of_iomap(np, 1);
	/* get fefifo reg base 0x15010000 */
	_disp_fefifo_hw_base[0] = (uintptr_t)of_iomap(np, 2);
	/* get fefifo reg base 0x15017000 */
	_disp_fefifo_hw_base[1] = (uintptr_t)of_iomap(np, 3);

	irq_no = irq_of_parse_and_map(np, 0);
	fefifo_drv->irq_num = 0;
	if (irq_no == 0)
		fefifo_error("fefifo get irq from dts fail\n");
	else {
		fefifo_drv->irq[fefifo_drv->irq_num].value = irq_no;
		fefifo_drv->irq[fefifo_drv->irq_num].irq = DISP_IRQ_FEFIFO_VDO3;
		fefifo_drv->irq_num++;
	}
	irq_no = irq_of_parse_and_map(np, 1);
	if (irq_no == 0)
		fefifo_error("fefifo get irq from dts fail\n");
	else {
		fefifo_drv->irq[fefifo_drv->irq_num].value = irq_no;
		fefifo_drv->irq[fefifo_drv->irq_num].irq = DISP_IRQ_FEFIFO_VDO4;
		fefifo_drv->irq_num++;
	}
	irq_no = irq_of_parse_and_map(np, 2);
	if (irq_no == 0)
		fefifo_error("fefifo get irq from dts fail\n");
	else {
		fefifo_drv->irq[fefifo_drv->irq_num].value = irq_no;
		fefifo_drv->irq[fefifo_drv->irq_num].irq = DISP_IRQ_FEFIFO_FHD;
		fefifo_drv->irq_num++;
	}
	irq_no = irq_of_parse_and_map(np, 3);
	if (irq_no == 0)
		fefifo_error("fefifo get irq from dts fail\n");
	else {
		fefifo_drv->irq[fefifo_drv->irq_num].value = irq_no;
		fefifo_drv->irq[fefifo_drv->irq_num].irq = DISP_IRQ_FEFIFO_UHD;
		fefifo_drv->irq_num++;
	}

	fefifo_info("fefifo addr [%p, %p, %p, %p]\n",
		(void *)_disp_fefifo_hw_base[0],
		(void *)_disp_fefifo_hw_base[1],
		(void *)_disp_fefifo_hw_base[2],
		(void *)_disp_fefifo_hw_base[3]);

	return FEFIFO_RET_OK;
}

static int disp_fefifo_init(struct disp_hw_common_info *info)
{
	uint32_t u4Idx = 0;

	fefifo_info("%s begin\n", __func__);
	_fefifo_parse_dev_node();
	for (u4Idx = 0; u4Idx < FEFIFO_MAX_LAYER_NUM; u4Idx++) {
		disp_fefifo_drv_init(u4Idx, _disp_fefifo_hw_base[u4Idx]);
#ifdef DISP_GCE_SUPPORT
	disp_fefifo_drv_set_gce_handle(u4Idx,
			(void *)info->gce_handle);
#else
	disp_fefifo_drv_set_gce_handle(u4Idx, NULL);
#endif
	}
	fefifo_debug_init();
	fefifo_info("%s done\n", __func__);

	//defaults enable fifo for test
	for (u4Idx = 2; u4Idx < FEFIFO_MAX_LAYER_NUM; u4Idx++) {
		if ((info != NULL) && (info->resolution != NULL)) {
			disp_fefifo_drv_set_sof(u4Idx);
			disp_fefifo_set_clk_enable(u4Idx, 1);
			disp_fefifo_drv_set_shadow(u4Idx, true, true);
			disp_fefifo_set_src_res(u4Idx,
			info->resolution->width,
			info->resolution->height, 0);
			disp_fefifo_drv_enable(u4Idx, true, 0);
		}
	}
	return FEFIFO_RET_OK;
}

static int disp_fefifo_deinit(void)
{
	uint32_t u4Idx = 0;

	fefifo_default("%s deinit\n", __func__);
	for (u4Idx = 0; u4Idx < FEFIFO_MAX_LAYER_NUM; u4Idx++)
		disp_fefifo_drv_uninit(u4Idx);
	fefifo_debug_deinit();
	return FEFIFO_RET_OK;
}

int disp_fefifo_start(struct disp_hw_common_info *info,
	unsigned int layer_id)
{
	fefifo_default("%s id:%d\n", __func__, layer_id);
	if ((info == NULL) || (info->resolution == NULL))
		return FEFIFO_RET_INV_ARG;
	// clk on
	disp_fefifo_drv_set_sof(layer_id);
	disp_fefifo_set_clk_enable(layer_id, 1);
	// hw on
	disp_fefifo_drv_set_shadow(layer_id, true, true);
	disp_fefifo_set_src_res(layer_id,
				info->resolution->width,
				info->resolution->height, 0);
	disp_fefifo_drv_enable(layer_id, true, 0);
	return FEFIFO_RET_OK;
}

int disp_fefifo_stop(unsigned int layer_id)
{
	fefifo_default("%s id:%d\n", __func__, layer_id);
	// hw off
	disp_fefifo_drv_enable(layer_id, false, 0);
	//clk off
	disp_fefifo_set_clk_enable(layer_id, 0);

	return FEFIFO_RET_OK;
}

int disp_fefifo_suspend(void)
{
	uint32_t layer_id = 0;

	for (layer_id = 2; layer_id < FEFIFO_MAX_LAYER_NUM; layer_id++) {
		disp_fefifo_drv_enable(layer_id, false, 0);
		disp_fefifo_set_clk_enable(layer_id, 0);
	}
	return FEFIFO_RET_OK;
}

int disp_fefifo_resume(void)
{
	uint32_t u4Idx = 0;
	uint32_t u4width = 1920;
	uint32_t u4height = 1080;

	// start will open clk and src
	for (u4Idx = 2; u4Idx < FEFIFO_MAX_LAYER_NUM; u4Idx++) {
		disp_fefifo_drv_set_sof(u4Idx);
		disp_fefifo_set_clk_enable(u4Idx, 1);
		disp_fefifo_drv_get_src_res(u4Idx,
			&u4width, &u4height);
		disp_fefifo_set_src_res(u4Idx,
			u4width, u4height, 0);
		disp_fefifo_drv_enable(u4Idx, true, 0);
	}

	return FEFIFO_RET_OK;
}

int disp_fefifo_chg_resolution(const struct disp_hw_resolution *info)
{
	uint32_t u4Idx = 0;

	fefifo_default("%s begin\n", __func__);
	//defaults enable fifo for test
	if (info != NULL) {
		// change active zone
		for (u4Idx = 2; u4Idx < FEFIFO_MAX_LAYER_NUM; u4Idx++)
			disp_fefifo_set_src_res(u4Idx,
			info->width,
			info->height, 0);
	}
	fefifo_default("%s done\n", __func__);

	return FEFIFO_RET_OK;
}

int disp_fefifo_irq_handler(uint32_t irq)
{
	// clear irq
	switch (irq) {
	case DISP_IRQ_FEFIFO_VDO3:
		disp_fefifo_drv_clr_irq(0, irq);
		disp_fefifo_drv_flush_reg(0, 0);
		break;
	case DISP_IRQ_FEFIFO_VDO4:
		disp_fefifo_drv_clr_irq(1, irq);
		disp_fefifo_drv_flush_reg(1, 0);
		break;
	case DISP_IRQ_FEFIFO_FHD:
		disp_fefifo_drv_clr_irq(2, irq);
		disp_fefifo_drv_flush_reg(2, 0);
		break;
	case DISP_IRQ_FEFIFO_UHD:
		disp_fefifo_drv_clr_irq(3, irq);
		disp_fefifo_drv_flush_reg(3, 0);
		break;
	default:
		break;
	}
	return FEFIFO_RET_OK;
}

int disp_fefifo_gce_trigger(struct disp_hw_trigger_info *info)
{
	int ret = FEFIFO_RET_OK;
	uint32_t u4Idx = 0;

	for (u4Idx = 0; u4Idx < FEFIFO_MAX_LAYER_NUM; u4Idx++)
		disp_fefifo_drv_gce_trigger(u4Idx);

	return ret;
}

int disp_fefifo_gce_trigger_by_layer(uint32_t layer_id)
{
	int ret = FEFIFO_RET_OK;

	ret = disp_fefifo_drv_gce_trigger(layer_id);

	return ret;
}

static int disp_fefifo_set_cmd(enum DISP_CMD cmd, void *data)
{
	uint32_t layer_id;
	uint32_t src_width;
	uint32_t src_height;
	struct FEFIFO_SRC_ACTIVE_T *pt_src;
	struct FEFIFO_SRC_ORDER_T *pt_order;

	switch (cmd) {
	case DISP_CMD_FEFIFO_SRC_ACTIVE:
		if (data != NULL) {
			pt_src = (struct FEFIFO_SRC_ACTIVE_T *)data;
			layer_id = pt_src->layer_id;
			src_width = pt_src->width;
			src_height = pt_src->height;
			disp_fefifo_set_src_res(layer_id,
				src_width, src_height, 0);
		}
		break;
	case DISP_CMD_FEFIFO_SRC_SWAP:
		if (data != NULL) {
			pt_order = (struct FEFIFO_SRC_ORDER_T *)data;
			disp_fefifo_drv_set_input_order(pt_order->layer_id,
				pt_order->in_order);
		}
		break;
	case DISP_CMD_FEFIFO_GCE_TRIGGER:
		if (data != NULL)
			disp_fefifo_gce_trigger_by_layer(*(uint32_t *)data);
		break;
	default:
		break;
	}
	return FEFIFO_RET_OK;
}

/*****************fefifo driver****************/
struct disp_hw disp_fefifo_driver = {
	.name = FEFIFO_DRV_NAME,
	.init = disp_fefifo_init,
	.deinit = disp_fefifo_deinit,
	.start = NULL,
	.stop = NULL,
	.suspend = disp_fefifo_suspend,
	.resume = disp_fefifo_resume,
	.get_info = NULL,
	.change_resolution = disp_fefifo_chg_resolution,
	.config = NULL,
	.config_ex = NULL,
	.irq_handler = disp_fefifo_irq_handler,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = NULL,
	.set_cmd = disp_fefifo_set_cmd,
	.gce_trigger = disp_fefifo_gce_trigger,
};

struct disp_hw *disp_fefifo_get_drv(void)
{
	return &disp_fefifo_driver;
}

