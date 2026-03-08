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

/* ---------------------------------- */
/* Video Plane: Interface */
/* ---------------------------------- */

#ifndef _DOVI_CORE1_HAL_C_
#define _DOVI_CORE1_HAL_C_

#include <linux/module.h>
#include <linux/types.h>

#include "disp_hw_mgr.h"
#include "dovi_log.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"
#include "disp_dovi_main.h"
#include "dovi_vdo_fe_hw.h"
#include "dovi_vdo_fe_hal.h"


uint32_t dv_vdo_fe_en[FE_MAX];
int vdo_fe_hal_init;
bool vdo_fe_bypass_cvm;
bool vdo_fe_bypass_csc;
struct disp_hw_resolution vdo_fe_out_res;
struct dv_sw_reg vdo_fe_sw_reg[FE_MAX];

int dovi_vdo_fe_hal_init(char *reg_base[])
{
	if (reg_base == NULL) {
		dovi_error("err params in vdofe hal init\n");
		return -1;
	}

	if (vdo_fe_hal_init) {
		dovi_default("vdo fe hal already inited\n");
		return 0;
	}

	vdo_fe_sw_reg[FE0].value =
		kcalloc(VDO_FE_REG_NUM, sizeof(uint32_t), GFP_KERNEL);
	vdo_fe_sw_reg[FE0].update =
		kcalloc(VDO_FE_REG_NUM, sizeof(uint8_t), GFP_KERNEL);
	if ((vdo_fe_sw_reg[FE0].value == NULL)
		|| (vdo_fe_sw_reg[FE0].update == NULL)) {
		if (vdo_fe_sw_reg[FE0].value != NULL)
			kfree(vdo_fe_sw_reg[FE0].value);
		if (vdo_fe_sw_reg[FE0].update != NULL)
			kfree(vdo_fe_sw_reg[FE0].update);
		dovi_error("vdofe swreg mem all failed\n");
		return -1;
	}

	vdo_fe_sw_reg[FE1].value =
		kcalloc(VDO_FE_REG_NUM, sizeof(uint32_t), GFP_KERNEL);
	vdo_fe_sw_reg[FE1].update =
		kcalloc(VDO_FE_REG_NUM, sizeof(uint8_t), GFP_KERNEL);
	if ((vdo_fe_sw_reg[FE1].value == NULL)
		|| (vdo_fe_sw_reg[FE1].update == NULL)) {
		if (vdo_fe_sw_reg[FE1].value != NULL)
			kfree(vdo_fe_sw_reg[FE1].value);
		if (vdo_fe_sw_reg[FE1].update != NULL)
			kfree(vdo_fe_sw_reg[FE1].update);
		dovi_error("vdofe swreg mem all failed\n");
		return -1;
	}

	vdo_fe_hal_init = 1;
	return 1;
}

int dovi_vdo_fe_hal_uninit(void)
{
	if (!vdo_fe_hal_init) {
		dovi_info("vdo fe hal already uninited\n");
		return 0;
	}

	if (vdo_fe_sw_reg[FE0].value != NULL)
		kfree(vdo_fe_sw_reg[FE0].value);
	if (vdo_fe_sw_reg[FE0].update != NULL)
		kfree(vdo_fe_sw_reg[FE0].update);

	if (vdo_fe_sw_reg[FE1].value != NULL)
		kfree(vdo_fe_sw_reg[FE1].value);
	if (vdo_fe_sw_reg[FE1].update != NULL)
		kfree(vdo_fe_sw_reg[FE1].update);

	dovi_func();
	vdo_fe_hal_init = 0;
	return 1;
}

int dovi_vdo_fe_hal_config_reg(uint32_t *p_vdo_fe_reg)
{
	dovi_vdo_fe_hal_set_composer_mode(false);

	return 1;
}

int dovi_vdo_fe_hal_config_lut(uint32_t lut_addr)
{
	return 1;
}

int dovi_vdo_fe_hal_set_composer_mode(bool fgCompEL)
{
	return 1;
}


int dovi_vdo_fe_hal_set_enable(uint8_t id, uint32_t enable)
{
	if (id >= FE_MAX) {
		dovi_error("%s idx err\n", __func__);
		return -1;
	}
	if (dv_vdo_fe_en[id] != enable) {
		dv_vdo_fe_en[id] = enable;
		dovi_printf("%s[%d] enable %d\n", __func__, id, enable);
		return 1;
	}

	return 0;
}

int dovi_vdo_fe_hal_is_support(void)
{
	return 1;
}

void dovi_vdo_fe_hal_crc(void)
{
}

void dovi_vdo_fe_hal_crc_reset(void)
{

}

int dovi_vdo_fe_hal_isr(void)
{
	return 0;
}

int dovi_vdo_fe_hal_status(char *reg_base[])
{
	char *reg = NULL;

	dovi_default("vdo_fe_enable %d %d\n",
		dv_vdo_fe_en[FE0], dv_vdo_fe_en[FE1]);

	reg = reg_base[DOVI_MVDO_FE];
	dovi_default("mainfe(0x%p) st:\n", reg);
	//get mainvdo fe sof setting 0x14001130 & 0x14001134
	dovi_default("0x618(0x%x), 0x61c(0x%x), 0x6d0(0x%x)\n",
		Dv_ReadREG(reg + 0x618), Dv_ReadREG(reg + 0x61c),
		Dv_ReadREG(reg + 0x6d0));
	dovi_default("0x804(0x%x), 0x808(0x%x), 0x81c(0x%x)\n",
		Dv_ReadREG(reg + 0x804), Dv_ReadREG(reg + 0x808),
		Dv_ReadREG(reg + 0x81c));
	dovi_default("0x9c0(0x%x), 0x9c4(0x%x), 0x9cc(0x%x)\n",
		Dv_ReadREG(reg + 0x9c0), Dv_ReadREG(reg + 0x9c4),
		Dv_ReadREG(reg + 0x9cc));
	dovi_default("0x9d0(0x%x), 0x9e4(0x%x), 0x9ec(0x%x), 0xc60(0x%x)\n",
		Dv_ReadREG(reg + 0x9d0), Dv_ReadREG(reg + 0x9e4),
		Dv_ReadREG(reg + 0x9ec), Dv_ReadREG(reg + 0xc60));

	reg = reg_base[DOVI_SVDO_FE];
	dovi_default("subfe(0x%p) st:\n", reg);
	//get mainvdo fe sof setting 0x14001168 & 0x1400116c
	dovi_default("0x618(0x%x), 0x61c(0x%x), 0x6d0(0x%x)\n",
		Dv_ReadREG(reg + 0x618), Dv_ReadREG(reg + 0x61c),
		Dv_ReadREG(reg + 0x6d0));
	dovi_default("0x804(0x%x), 0x808(0x%x), 0x81c(0x%x)\n",
		Dv_ReadREG(reg + 0x804), Dv_ReadREG(reg + 0x808),
		Dv_ReadREG(reg + 0x81c));
	dovi_default("0x9c0(0x%x), 0x9c4(0x%x), 0x9cc(0x%x)\n",
		Dv_ReadREG(reg + 0x9c0), Dv_ReadREG(reg + 0x9c4),
		Dv_ReadREG(reg + 0x9cc));
	dovi_default("0x9d0(0x%x), 0x9e4(0x%x), 0x9ec(0x%x), 0xc60(0x%x)\n",
		Dv_ReadREG(reg + 0x9d0), Dv_ReadREG(reg + 0x9e4),
		Dv_ReadREG(reg + 0x9ec), Dv_ReadREG(reg + 0xc60));
	return 1;
}

int dovi_vdo_fe_hal_set_out_fix_pattern_enable(bool enable,
					      uint32_t pattern_i,
					      uint32_t pattern_cp,
					      uint32_t pattern_ct)
{
	return 1;
}

#endif
