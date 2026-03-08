// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#define LOG_TAG "VDP"

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "disp_hw_mgr.h"
#include "disp_adl_if.h"
#include "disp_fg_if.h"
#include "disp_fg_drv.h"
#include "disp_fg_debug.h"
#include "fg_hal.h"
#include "disp_path.h"
#include "fmt_hal.h"

u32 fg_dbg_level = FG_DEFAULT_LOG;

bool fg_start[MAX_FG];

static struct adl_src_tbl fg_adl_tbl[MAX_FG];
static struct mtk_av1_film_grain_params fg_params[MAX_FG];

static struct disp_fg_sof fg_sof[MAX_FG][DISP_FG_SOF_NR] = {
	{
		{DISP_SOF_4_M_FILM_GRAIN_1_STA, DISP_SOF_4_M_FILM_GRAIN_1_END},
		{DISP_SOF_4_M_FILM_GRAIN_1_STA, DISP_SOF_4_M_FILM_GRAIN_1_END},
		{DISP_SOF_5_M_FILM_GRAIN_2_STA, DISP_SOF_5_M_FILM_GRAIN_2_END},
		{DISP_SOF_27_M_FILM_GRAIN_3_STA, DISP_SOF_27_M_FILM_GRAIN_3_END},
		{DISP_SOF_28_M_FILM_GRAIN_4_STA, DISP_SOF_28_M_FILM_GRAIN_4_END},
	},
	{
		{DISP_SOF_10_S_FILM_GRAIN_0_STA, DISP_SOF_10_S_FILM_GRAIN_0_END},
		{DISP_SOF_11_S_FILM_GRAIN_1_STA, DISP_SOF_11_S_FILM_GRAIN_1_END},
		{DISP_SOF_12_S_FILM_GRAIN_2_STA, DISP_SOF_12_S_FILM_GRAIN_2_END},
		{DISP_SOF_29_S_FILM_GRAIN_3_STA, DISP_SOF_29_S_FILM_GRAIN_3_END},
		{DISP_SOF_30_S_FILM_GRAIN_4_STA, DISP_SOF_30_S_FILM_GRAIN_4_END},
	}
};

static int disp_fg_parse_dev_node(void)
{
	struct device_node *np;
	u32 reg_value;
	u32 reg_values[8];
	u32 idx;
	int cnt;

	FG_FUNC();

	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-fg");
	if (np == NULL) {
		FG_ERR("dts error, no fg device node.\n");
		return FG_FAIL;
	}

	cnt = of_property_count_u32_elems(np, "reg");
	if (cnt < 0) {
		for (idx = 0; idx < 8; idx++)
			reg_values[idx] = 0;

		FG_LOG_I("dts no reg info\n");
	} else {
		of_property_read_u32_array(np, "reg", reg_values, cnt);

		for (idx = 0; idx < cnt; idx++)
			FG_LOG_I("reg_values 0x%x\n", reg_values[idx]);
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);
	FG_LOG_I("fg reg_value 0x%x\n", reg_value);

	 /* main fg, 15011000 */
	 /* sub fg, 15014000 */
	for (idx = 0; idx < MAX_FG; idx++) {
		disp_fg_reg_base[idx] = of_iomap(np, idx);
		FG_LOG_I("fg %d base %p\n", idx, disp_fg_reg_base[idx]);
	}


	return FG_OK;
}

static void disp_fg_init_adl_tbl(u32 fg_hw_id, struct adl_src_tbl *adl_tbl)
{
	if (!adl_tbl)
		return;

	FG_FUNC();

	if (fg_hw_id == MAIN_FG)
		adl_tbl->client = FILM_GRAIN_MAIN;
	else
		adl_tbl->client = FILM_GRAIN_SUB;
	adl_tbl->fgrain.p_data = kmalloc(FILMG_SRC_LEN, GFP_KERNEL);
	adl_tbl->fgrain.used_size = FILMG_SRC_LEN;

	FG_LOG_I("fg %d adl_tbl base 0x%p\n", fg_hw_id, adl_tbl->fgrain.p_data);
}

static int disp_fg_init(struct disp_hw_common_info *info)
{
	u32 idx;

	FG_FUNC();

	disp_fg_parse_dev_node();

	for (idx = 0; idx < MAX_FG; idx++)
		disp_fg_init_adl_tbl(idx, &fg_adl_tbl[idx]);

	fg_debug_init();

	fg_sec_init();

	return FG_OK;
}

static int disp_fg_deinit(void)
{
	FG_FUNC();

	return FG_OK;
}

static int disp_fg_suspend(void)
{
	FG_FUNC();

	return FG_OK;
}

static int disp_fg_resume(void)
{
	FG_FUNC();

	return FG_OK;
}

static int disp_fg_irq_handler(uint32_t irq)
{
	return FG_OK;
}

static void disp_fg_set_sof(u32 layer_id)
{
	struct disp_fg_sof *sof;
	u32 idx = 0;
	int sof_start = 0, sof_end = 0;

	FG_FUNC();

	for (idx = 0; idx < DISP_FG_SOF_NR; idx++) {
		sof = &fg_sof[layer_id][idx];

		disp_path_get_sof_info(sof->sof_start, sof->sof_end, &sof_start, &sof_end);
		fmt_hal_set_sof(sof->sof_start, sof->sof_end, sof_start, sof_end);

		FG_LOG_I("fg sof %d %d 0x%X 0x%X\n",
			 sof->sof_start,
			 sof->sof_end,
			 sof_start, sof_end);
	}
}

static void disp_fg_enable(u32 layer_id, bool enable)
{
	enum DISP_HW_CLK fg_clk = DISP_CLK_M_FILM_GRAIN;
	enum DISP_PATH_HW_ID path_id = DISP_PATH_M_FILM_GRAIN;

	FG_FUNC();

	if (enable)
		disp_fg_set_sof(layer_id);

	if (layer_id == 0) {
		fg_clk = DISP_CLK_M_FILM_GRAIN;
		path_id = DISP_PATH_M_FILM_GRAIN;
	} else {
		fg_clk = DISP_CLK_S_FILM_GRAIN;
		path_id = DISP_PATH_S_FILM_GRAIN;
	}

	disp_clock_enable(fg_clk, enable);

	disp_path_set_hw_path(path_id, enable);
}

static int disp_fg_start(void *data)
{
	uint32_t layer_id = 0;

	if (data == NULL) {
		FG_ERR("err param\n");
		return -1;
	}

	layer_id = *((uint32_t *)data);
	if (layer_id > SUB_FG) {
		FG_ERR("idx err\n");
		return -1;
	}

	FG_FUNC();

	if (fg_start[layer_id])
		return -1;

	disp_fg_enable(layer_id, true);

	fg_start[layer_id] = true;

	return FG_OK;
}

static int disp_fg_stop(void *data)
{
	uint32_t layer_id = 0;

	if (data == NULL) {
		FG_ERR("err param\n");
		return -1;
	}

	layer_id = *((uint32_t *)data);
	if (layer_id > SUB_FG) {
		FG_ERR("invalid layer id %d\n", layer_id);
		return -1;
	}

	FG_FUNC();

	if (!fg_start[layer_id])
		return -1;

	disp_fg_enable(layer_id, false);

	fg_start[layer_id] = false;

	return FG_OK;
}

static int disp_fg_cmd_handler(enum DISP_CMD cmd, void *data)
{
	switch (cmd) {
	case DISP_CMD_FG_START:
		disp_fg_start(data);
		break;
	case DISP_CMD_VDP_STOP:
		disp_fg_stop(data);
		break;
	default:
		break;
	}
	return 0;
}

u8 *disp_fg_get_adl_tbl(u32 fg_hw_id)
{
	struct adl_src_tbl *adl_tbl;

	if (fg_hw_id >= MAX_FG) {
		FG_ERR("invalid hw id %d\n", fg_hw_id);
		return NULL;
	}

	adl_tbl = &fg_adl_tbl[fg_hw_id];

	FG_LOG_I("fg %d adl_tbl base 0x%p\n",
		fg_hw_id, adl_tbl->fgrain.p_data);

	return adl_tbl->fgrain.p_data;
}

struct mtk_av1_film_grain_params *disp_fg_get_param(u32 fg_hw_id)
{
	struct mtk_av1_film_grain_params *param;

	if (fg_hw_id >= MAX_FG) {
		FG_ERR("invalid hw id %d\n", fg_hw_id);
		return NULL;
	}

	param = &fg_params[fg_hw_id];

	FG_LOG_I("fg %d param base 0x%p\n",
		fg_hw_id, param);

	return param;
}

int disp_fg_set_dbg_level_enable(uint32_t level, uint32_t enable)
{
	if (enable)
		fg_dbg_level |= (1 << level);
	else
		fg_dbg_level &= ~(1 << level);
	FG_LOG_I("set dbg level %d enable %d 0x%X\n", level, enable,
		   fg_dbg_level);
	return 0;
}

void disp_fg_config(u32 fg_hw_id, struct mtk_av1_film_grain_params *fg_param)
{
	struct adl_src_tbl *adl_tbl = NULL;
	struct mtk_av1_film_grain_params *param;
	bool ret;

	if (fg_hw_id >= MAX_FG) {
		FG_ERR("invalid hw id %d\n", fg_hw_id);
		return;
	}

	FG_FUNC();

	adl_tbl = &fg_adl_tbl[fg_hw_id];
	param = &fg_params[fg_hw_id];

	if (fg_param)
		memcpy(param, fg_param, sizeof(struct mtk_av1_film_grain_params));

	ret = disp_fg_handler(fg_hw_id, param, adl_tbl);
	FG_LOG_D("fg %d adl_tbl client %d used_size %d base %p\n",
		fg_hw_id,
		adl_tbl->client,
		adl_tbl->fgrain.used_size,
		adl_tbl->fgrain.p_data);

	if (ret) {
		disp_adl_cfg_client_en(adl_tbl->client, 1, 0);
		disp_filmg_config_adl_table(adl_tbl);
	}
}

/***************** driver ************/
struct disp_hw disp_fg_driver = {
	.name = FG_DRV_NAME,
	.init = disp_fg_init,
	.deinit = disp_fg_deinit,
	.suspend = disp_fg_suspend,
	.resume = disp_fg_resume,
	.irq_handler = disp_fg_irq_handler,
	.set_cmd = disp_fg_cmd_handler,
};

struct disp_hw *disp_fg_get_drv(void)
{
	return &disp_fg_driver;
}
