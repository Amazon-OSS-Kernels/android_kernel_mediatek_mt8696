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

/* ------------------------------------------ */
/* Include files */
/* ------------------------------------------ */
#include <linux/types.h>

#include "dovi_log.h"
#include "disp_hw_mgr.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"
#include "disp_dovi_main.h"
#include "dovi_table.h"
#include "dovi_gfx_fe_hw.h"
#include "dovi_gfx_fe_hal.h"




/* --------------------------------------------- */
/* Macro definitions */
/* --------------------------------------------- */


/* ---------------------------------------------*/
/* Static variables */
/* ---------------------------------------------*/
/* software register */

uint32_t gfx_fe_hal_init;
uint32_t dv_gfx_fe_en[FE_MAX];

struct dv_sw_reg gfx_fe_sw_reg[FE_MAX];


/* ---------------------------------------------*/
/* Inter-file functions */
/* ---------------------------------------------*/
int dovi_gfx_fe_hal_init(char *reg_base[])
{
	if (reg_base == NULL) {
		dovi_error("regbase null in gfx fe init\n");
		return -1;
	}

	if (gfx_fe_hal_init) {
		dovi_info("gfx_fe hal already inited\n");
		return 0;
	}

	gfx_fe_sw_reg[FE0].value =
			kcalloc(GFX_FE_REG_NUM, sizeof(uint32_t), GFP_KERNEL);
		gfx_fe_sw_reg[FE0].update =
			kcalloc(GFX_FE_REG_NUM, sizeof(uint8_t), GFP_KERNEL);
		if (gfx_fe_sw_reg[FE0].value == NULL
			|| gfx_fe_sw_reg[FE0].update == NULL) {
			if (gfx_fe_sw_reg[FE0].value != NULL)
				kfree(gfx_fe_sw_reg[FE0].value);
			if (gfx_fe_sw_reg[FE0].update != NULL)
				kfree(gfx_fe_sw_reg[FE0].update);
			dovi_error("vdofe swreg mem all failed\n");
			return -1;
		}

		gfx_fe_sw_reg[FE1].value =
			kcalloc(GFX_FE_REG_NUM, sizeof(uint32_t), GFP_KERNEL);
		gfx_fe_sw_reg[FE1].update =
			kcalloc(GFX_FE_REG_NUM, sizeof(uint8_t), GFP_KERNEL);
		if (gfx_fe_sw_reg[FE1].value == NULL
			|| gfx_fe_sw_reg[FE1].update == NULL) {
			if (gfx_fe_sw_reg[FE1].value != NULL)
				kfree(gfx_fe_sw_reg[FE1].value);
			if (gfx_fe_sw_reg[FE1].update != NULL)
				kfree(gfx_fe_sw_reg[FE1].update);
			dovi_error("vdofe swreg mem all failed\n");
			return -1;
		}

	gfx_fe_hal_init = 1;
	return 1;
}

int dovi_gfx_fe_hal_unint(void)
{
	if (gfx_fe_hal_init == 0)
		return 0;

	if (gfx_fe_sw_reg[FE0].value != NULL)
		kfree(gfx_fe_sw_reg[FE0].value);
	if (gfx_fe_sw_reg[FE0].update != NULL)
		kfree(gfx_fe_sw_reg[FE0].update);

	if (gfx_fe_sw_reg[FE1].value != NULL)
		kfree(gfx_fe_sw_reg[FE1].value);
	if (gfx_fe_sw_reg[FE1].update != NULL)
		kfree(gfx_fe_sw_reg[FE1].update);

	gfx_fe_hal_init = 0;
	return 0;

}
uint32_t dovi_gfx_fe_hal_is_support(void)
{
	return 0;
}


int dovi_gfx_fe_hal_config_reg(uint32_t *p_gfx_fe_reg)
{

	return 1;
}

int dovi_gfx_fe_hal_config_lut(uint32_t lut_addr)
{

	return 1;
}

int dovi_gfx_fe_hal_isr(void)
{
	return 0;
}

int dovi_gfx_fe_hal_set_enable(uint8_t id, uint32_t enable)
{
	if (id >= FE_MAX) {
		dovi_error("gfx idx error %d\n", id);
		return -1;
	}

	if (dv_gfx_fe_en[id] != enable) {
		dv_gfx_fe_en[id] = enable;
		dovi_info("%s[%d] %d\n", __func__, id, enable);
	}

	return 0;
}

int dovi_gfx_fe_hal_status(char *reg_base[])
{
	char *reg = NULL;

	dovi_default("vdo_fe_enable %d %d\n",
		dv_gfx_fe_en[FE0], dv_gfx_fe_en[FE1]);

	reg = reg_base[DOVI_FHD_FE];
	dovi_default("fhdfe(0x%p) st:\n", reg);
	//get mainvdo fe sof setting 0x14001218 & 0x1400121c
	dovi_default("0x100(0x%x), 0x204(0x%x), 0x21c(0x%x)\n",
		Dv_ReadREG(reg + 0x100), Dv_ReadREG(reg + 0x204),
		Dv_ReadREG(reg + 0x21c));
	dovi_default("0x3c0(0x%x), 0x3c4(0x%x), 0x3d0(0x%x)\n",
		Dv_ReadREG(reg + 0x3c0), Dv_ReadREG(reg + 0x3c4),
		Dv_ReadREG(reg + 0x3d0));
	dovi_default("0x3e4(0x%x), 0x3e8(0x%x), 0x3f4(0x%x)\n",
		Dv_ReadREG(reg + 0x3e4), Dv_ReadREG(reg + 0x3e8),
		Dv_ReadREG(reg + 0x3f4));

	reg = reg_base[DOVI_UHD_FE];
	dovi_default("uhdfe(0x%p) st:\n", reg);
	//get mainvdo fe sof setting 0x14001218 & 0x1400121c
	dovi_default("0x100(0x%x), 0x204(0x%x), 0x21c(0x%x)\n",
		Dv_ReadREG(reg + 0x100), Dv_ReadREG(reg + 0x204),
		Dv_ReadREG(reg + 0x21c));
	dovi_default("0x3c0(0x%x), 0x3c4(0x%x), 0x3d0(0x%x)\n",
		Dv_ReadREG(reg + 0x3c0), Dv_ReadREG(reg + 0x3c4),
		Dv_ReadREG(reg + 0x3d0));
	dovi_default("0x3e4(0x%x), 0x3e8(0x%x), 0x3f4(0x%x)\n",
		Dv_ReadREG(reg + 0x3e4), Dv_ReadREG(reg + 0x3e8),
		Dv_ReadREG(reg + 0x3f4));

	return 1;
}

