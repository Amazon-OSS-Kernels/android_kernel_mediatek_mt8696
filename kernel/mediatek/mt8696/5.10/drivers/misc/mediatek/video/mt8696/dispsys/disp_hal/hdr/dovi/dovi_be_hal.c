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
#include <linux/types.h>

#include "dovi_log.h"
#include "dovi_table.h"
#include "disp_hw_mgr.h"
#include "disp_dovi_main.h"
#include "dovi_be_hal.h"
#include "dovi_be_hw.h"

uint32_t dv_vdo_be_en;
int be_hal_init;
bool be_bypass_dither;
struct dv_sw_reg vdo_be_sw_reg;

int dovi_be_hal_init(char *reg_base[])
{
	if (reg_base == NULL) {
		dovi_error("regbase null in be init\n");
		return -1;
	}

	if (be_hal_init) {
		dovi_info("be hal already inited\n");
		return 0;
	}

	vdo_be_sw_reg.value =
		kcalloc(VDO_BE_REG_NUM, sizeof(uint32_t), GFP_KERNEL);
	vdo_be_sw_reg.update =
		kcalloc(VDO_BE_REG_NUM, sizeof(uint8_t), GFP_KERNEL);
	if (vdo_be_sw_reg.value == NULL
		|| vdo_be_sw_reg.update == NULL) {
		if (vdo_be_sw_reg.value != NULL)
			kfree(vdo_be_sw_reg.value);
		if (vdo_be_sw_reg.update != NULL)
			kfree(vdo_be_sw_reg.update);
		dovi_error("vdobe swreg mem all failed\n");
		return -1;
	}

	be_hal_init = 1;
	return 1;
}

int dovi_be_hal_unint(void)
{
	if (be_hal_init == 0)
		return 0;

	if (vdo_be_sw_reg.value != NULL)
		kfree(vdo_be_sw_reg.value);
	if (vdo_be_sw_reg.update != NULL)
		kfree(vdo_be_sw_reg.update);

	be_hal_init = 0;

	return 0;
}

int dovi_be_hal_config_reg(uint32_t *p_be_reg)
{
	return 1;
}

int dovi_be_hal_dither_bypass(uint32_t bypass)
{
	return 1;
}

int dovi_be_hal_set_out_mode(enum dovi_be_out_mode mode)
{

	return 1;
}

int dovi_be_hal_set_out_fix_pattern_enable(bool enable)
{
	return 1;
}

int dovi_be_hal_isr(void)
{
	return 0;
}

int dovi_be_hal_set_enable(uint32_t enable)
{
	if (dv_vdo_be_en != enable) {
		dv_vdo_be_en = enable;
		dovi_printf("%s %d\n", __func__, enable);
	}

	return 1;
}

int dovi_be_hal_status(char *reg_base[])
{
	char *reg = NULL;

	dovi_default("vdo_be_enable %d\n", dv_vdo_be_en);

	reg = reg_base[DOVI_VDO_BE];
	dovi_default("be(0x%p) st:\n", reg);
	//get mainvdo fe sof setting 0x14001228 & 0x1400122c
	dovi_default("0x1a0(0x%x), 0x204(0x%x), 0x210(0x%x)\n",
		Dv_ReadREG(reg + 0x1a0), Dv_ReadREG(reg + 0x204),
		Dv_ReadREG(reg + 0x210));
	dovi_default("0x21c(0x%x), 0x320(0x%x), 0x3c8(0x%x)\n",
		Dv_ReadREG(reg + 0x21c), Dv_ReadREG(reg + 0x320),
		Dv_ReadREG(reg + 0x3c8));
	dovi_default("0x3d0(0x%x), 0x3d8(0x%x), 0x3dc(0x%x)\n",
		Dv_ReadREG(reg + 0x3d0), Dv_ReadREG(reg + 0x3d8),
		Dv_ReadREG(reg + 0x3dc));
	dovi_default("0x3e0(0x%x), 0x3ec(0x%x)\n",
		Dv_ReadREG(reg + 0x3e0), Dv_ReadREG(reg + 0x3ec));

	return 1;
}

