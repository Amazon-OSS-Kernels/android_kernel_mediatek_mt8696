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

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>

#include "dovi_log.h"
#include "disp_hw_mgr.h"
#include "dovi_be_hal.h"
#include "dovi_be_hw.h"
#include "disp_adl_if.h"
#include "disp_dovi_be_if.h"


int be_init;

int dovi_be_init(char *reg_base[])
{
	if (reg_base == NULL) {
		dovi_error("regbase null in vdobe\n");
		return -1;
	}

	if (be_init) {
		dovi_info("vdobe already inited\n");
		return 0;
	}

	dovi_be_hal_init(reg_base);

	be_init = 1;
	return 0;
}

int dovi_be_uninit(void)
{
	be_init = 0;
	dovi_info("%s\n", __func__);
	return 1;
}

int dovi_be_status(char *reg_base[])
{
	dovi_be_hal_status(reg_base);
	return 1;
}

int dovi_be_config_lut(uint32_t *md_lut)
{
	void *lut_addr = NULL;
	dma_addr_t lut_addr_pa = 0;
	uint8_t adl_client = 0;

	if (md_lut == NULL) {
		dovi_error("%s err param\n", __func__);
		return -1;
	}

	disp_adl_get_clt_buf(DV_SCRM, &lut_addr, &lut_addr_pa);
	adl_client = DV_SCRM;

	memcpy(lut_addr, md_lut, DOVI_BE_LUT_SIZE);

	disp_adl_cfg_client_en(adl_client, 1, 0);
	disp_adl_cfg_client(adl_client, lut_addr_pa, DOVI_MD_DW_SIZE);
	dovi_printf("%s 0x%p 0x%x\n", __func__, lut_addr, lut_addr_pa);

	return 1;
}

