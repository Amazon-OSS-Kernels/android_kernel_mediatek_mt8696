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

/* -------------------------------------- */
/* Include files */
/* -------------------------------------- */

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>

#include "dovi_gfx_fe_hw.h"
#include "disp_hw_mgr.h"
#include "dovi_log.h"
#include "disp_dovi_common_if.h"
#include "dovi_common_hal.h"
#include "dovi_gfx_fe_hal.h"
#include "disp_adl_if.h"
#include "disp_hdr_if.h"
#include "disp_dovi_gfx_fe_if.h"

int gfx_fe_init;

int dovi_gfx_fe_init(char *reg_base[])
{

	if (reg_base == NULL) {
		dovi_error("reg_base null in gfxfe init\n");
		return -1;
	}

	if (gfx_fe_init) {
		dovi_info("gfx_fe already inited\n");
		return 0;
	}

	dovi_gfx_fe_hal_init(reg_base);
	gfx_fe_init = 1;
	return 0;
}

int dovi_gfx_fe_config_lut(uint32_t id, uint32_t *p_gfx_fe_lut)
{
	void *lut_addr = NULL;
	dma_addr_t lut_addr_pa = 0;
	uint8_t adl_client = 0;

	if (p_gfx_fe_lut == NULL || id >= LAYER_MAX) {
		dovi_error("%s err param\n", __func__);
		return -1;
	}

	if (id == LAYER0) {
		adl_client = DV_ADL_G_FHD;
		disp_adl_get_clt_buf(adl_client, &lut_addr, &lut_addr_pa);

	} else {
		adl_client = DV_ADL_G_UHD;
		disp_adl_get_clt_buf(adl_client, &lut_addr, &lut_addr_pa);
	}

	memcpy(lut_addr, (uint8_t *)p_gfx_fe_lut, DOVI_FE_LUT_SIZE);

	dovi_printf("%s 0x%p 0x%x\n", __func__, lut_addr, lut_addr_pa);
	disp_adl_cfg_client_en(adl_client, 1, 0);
	disp_adl_cfg_client(adl_client, lut_addr_pa, DOVI_MD_SIZE);

	return 1;
}

int dovi_gfx_fe_uninit(void)
{
	gfx_fe_init = 0;
	dovi_error("%s\n", __func__);
	return 0;
}

int dovi_gfx_fe_status(char *reg_base[])
{
	dovi_gfx_fe_hal_status(reg_base);
	return 1;
}
