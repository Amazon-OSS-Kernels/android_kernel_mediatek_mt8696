/*
 * Copyright (C) 2020 MediaTek Inc.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>

#include "disp_hw_mgr.h"
#include "adl_hw.h"
#include "disp_adl_cmd.h"
#include "disp_adl_if.h"
#include "disp_ml_if.h"
#include "ml_hal.h"
#include "ml_hw.h"
#include "adl_hal.h"

int adl_hal_init_done;
uintptr_t adl_clt_regbase[ADL_CLIT_REG_MAX];

static int adl_write2regtable_2bytes(uint32_t addr,
	uint16_t value, uint16_t mask, struct adl_reg_tbl *preg_tbl)
{
	if (mask) {
		if (preg_tbl->depth < ADL_REG_COUNT) {
			preg_tbl->address[preg_tbl->depth] = addr;
			preg_tbl->value[preg_tbl->depth] = value;
			preg_tbl->mask[preg_tbl->depth] = mask;
			preg_tbl->depth++;
			adl_info("update reg 0x%x 0x%x 0x%x %d\n",
				addr, value, mask, preg_tbl->depth);
		} else {
			adl_error("overflow %d\n", preg_tbl->depth);
			return -1;
		}
	}
	return 0;
}


int adl_hal_cfg_client_enable(uint8_t client, bool en, uint8_t en_mode)
{
	uint8_t hw_client = ADL_CLIT_REG_MAX;
	struct disp_adl_clt_context *p_clt_ctx = NULL;
	uint32_t clit_offset = 0;
	bool pre_enable = false;

	switch (client) {
	case DV_ADL_V_MAIN:
	case THDR_ADL_V_MAIN:
		hw_client = ADL_DSYS_CLITA;
		clit_offset = ADL_CLITA_OFFSET;
		break;
	case DV_ADL_V_SUB:
	case THDR_ADL_V_SUB:
		hw_client = ADL_DSYS_CLITE;
		clit_offset = ADL_CLITE_OFFSET;
		break;
	case DV_ADL_G_FHD:
		hw_client = ADL_MSYS_CLITA;
		clit_offset = ADL_CLITA_OFFSET;
		break;
	case DV_ADL_G_UHD:
		hw_client = ADL_MSYS_CLITE;
		clit_offset = ADL_CLITE_OFFSET;
		break;
	case DV_SCRM:
		hw_client = ADL_MSYS_CLITB;
		clit_offset = ADL_CLITB_OFFSET;
		break;
	case THDR_ADL_G_FHD:
		hw_client = ADL_MSYS_CLITC;
		clit_offset = ADL_CLITC_OFFSET;
		break;
	case THDR_ADL_G_UHD:
		hw_client = ADL_MSYS_CLITD;
		clit_offset = ADL_CLITD_OFFSET;
		break;
	case FILM_GRAIN_MAIN:
		hw_client = ADL_DSYS_CLITB;
		clit_offset = ADL_CLITB_OFFSET;
		break;
	case FILM_GRAIN_SUB:
		hw_client = ADL_DSYS_CLITJ;
		clit_offset = ADL_CLITJ_OFFSET;
		break;
	default:
		adl_error("adl client id err\n");
		return -1;
	}

	p_clt_ctx = &adl_clt_ctx[hw_client];
	pre_enable = p_clt_ctx->enabled;

	if (pre_enable != en) {
		p_clt_ctx->enabled = en;
		//p_clt_ctx->clt_update_mode = en_mode;

		if (!en) {
			p_clt_ctx->src_tbl.update = false;
			AdlWriteREGMsk(
				p_clt_ctx->remap_reg_base + clit_offset + EN,
				0x0, 0x3);
		}

		adl_info("adl[%d] set en[%d]\n", hw_client, en);
	}

	return ADL_OK;
}

//flush all client if client ctx enable
int adl_hal_client_flush(struct disp_adl_clt_context *p_clt_ctx)
{
	struct adl_reg_tbl *preg_tbl = NULL;
	struct ml_reg_table ml_reg = { 0 };
	uintptr_t rmap_reg_base = 0;
	uint32_t hw_reg_base = 0;
	uint8_t i = 0;
	uint8_t mode = 0;
	uint32_t depth = 0;

	preg_tbl = p_clt_ctx->preg_tbl;
	mode = p_clt_ctx->reg_conf_mode;
	rmap_reg_base = p_clt_ctx->remap_reg_base;
	hw_reg_base = p_clt_ctx->hw_reg_base;
	depth = preg_tbl->depth;
	adl_info("clt mode %d reg(0x%p 0x%x) depth:%d",
		mode, (void *)rmap_reg_base, hw_reg_base, depth);
	switch (mode) {
	case 0: //gce update
		for (i = 0; i < depth; i++) {
			//call gce api,use hw regbase
			//write_reg(hw_reg_base + preg_tbl->address[i],
			//	preg_tbl->value[i],
			//	preg_tbl->mask[i]);
		}
		break;
	case 1: // ml update use hwreg base
		for (i = 0; i < depth; i++)
			preg_tbl->address[i] += hw_reg_base;
		if (p_clt_ctx->hw_reg_base == ADL_DSYS_REG_BASE)
			ml_reg.ml_ip = ML_DSYS_IP;
		else
			ml_reg.ml_ip = ML_MSYS_IP;

		ml_reg.depth = preg_tbl->depth;
		ml_reg.p_reg_addr = preg_tbl->address;
		ml_reg.p_reg_value = preg_tbl->value;
		ml_reg.p_reg_mask = preg_tbl->mask;

		disp_ml_write_reg_multi(&ml_reg);
		break;
	case 2: // cpu update
		for (i = 0; i < depth; i++) {
			//call cpu api,use remap regbase
			AdlWriteREGMsk(rmap_reg_base + preg_tbl->address[i],
				preg_tbl->value[i],
				preg_tbl->mask[i]);
		}
		break;
	default:
		adl_info("error update mode\n");
		break;
	}
	return 0;

}

int adl_hal_cfg_client(uint8_t client,
	dma_addr_t dst_mva, uint32_t max_len)
{
	uint8_t hw_client = ADL_CLIT_REG_MAX;
	struct disp_adl_clt_context *p_clt_ctx = NULL;
	struct adl_reg_tbl *preg_tbl = NULL;
	uint32_t clit_offset = 0;
	uint32_t clit_sw_trig_en_offset = 0;
	uint32_t clit_sw_trig_offset = 0;
	uint32_t clit_sw_trig_bit_shift = 0;
	uint32_t init_addr = 0;

	dma_addr_t mva_msb = 0;
	dma_addr_t mva_lsb = 0;

	switch (client) {
	case DV_ADL_V_MAIN:
	case THDR_ADL_V_MAIN:
		hw_client = ADL_DSYS_CLITA;
		clit_offset = ADL_CLITA_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER0_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER0_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLIA_SW_TRIG_SHIF;
		break;
	case DV_ADL_V_SUB:
	case THDR_ADL_V_SUB:
		hw_client = ADL_DSYS_CLITE;
		clit_offset = ADL_CLITE_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER0_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER0_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLIE_SW_TRIG_SHIF;
		break;
	case DV_ADL_G_FHD:
		hw_client = ADL_MSYS_CLITA;
		clit_offset = ADL_CLITA_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER0_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER0_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLIA_SW_TRIG_SHIF;
		break;
	case DV_ADL_G_UHD:
		hw_client = ADL_MSYS_CLITE;
		clit_offset = ADL_CLITE_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER0_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER0_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLIE_SW_TRIG_SHIF;
		break;
	case DV_SCRM:
		hw_client = ADL_MSYS_CLITB;
		clit_offset = ADL_CLITB_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER0_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER0_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLIB_SW_TRIG_SHIF;
		break;
	case THDR_ADL_G_FHD:
		hw_client = ADL_MSYS_CLITC;
		clit_offset = ADL_CLITC_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER0_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER0_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLIC_SW_TRIG_SHIF;
		init_addr = 0xffff;
		break;
	case THDR_ADL_G_UHD:
		hw_client = ADL_MSYS_CLITD;
		clit_offset = ADL_CLITD_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER0_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER0_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLID_SW_TRIG_SHIF;
		init_addr = 0xffff;
		break;
	case FILM_GRAIN_MAIN:
		hw_client = ADL_DSYS_CLITB;
		clit_offset = ADL_CLITB_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER0_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER0_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLIB_SW_TRIG_SHIF;
		break;
	case FILM_GRAIN_SUB:
		hw_client = ADL_DSYS_CLITJ;
		clit_offset = ADL_CLITJ_OFFSET;
		clit_sw_trig_en_offset = ADL_SW_TRIGGER1_EN_OFFSET;
		clit_sw_trig_offset = ADL_SW_TRIGGER1_OFFSET;
		clit_sw_trig_bit_shift = ADL_CLIJ_SW_TRIG_SHIF;
		break;
	default:
		break;
	}

	if (hw_client >= ADL_CLIT_REG_MAX)
		return ADL_ERR;

	p_clt_ctx = &adl_clt_ctx[hw_client];
	preg_tbl = adl_clt_ctx[hw_client].preg_tbl;

	adl_info("%s client[%d][0x%p 0x%x][0x%x 0x%x] len %d\n",
		__func__, hw_client,
		(void *)p_clt_ctx->remap_reg_base,
		p_clt_ctx->hw_reg_base,
		p_clt_ctx->lut_addr_mva,
		dst_mva, max_len);

	if (dst_mva != p_clt_ctx->lut_addr_mva) {
		adl_info("%s clt[%d] mva not match\n", __func__, client);
		return ADL_ERR;
	}

	memset(preg_tbl, 0, sizeof(struct adl_reg_tbl));


	mva_msb = ((0xffff0000 & (dst_mva >> 5)) >> 16);
	mva_lsb = ((0x0000ffff & (dst_mva >> 5)));


	//04
	adl_write2regtable_2bytes(clit_offset + ADDR0,
	mva_lsb, 0xffff, preg_tbl);
	//08
	adl_write2regtable_2bytes(clit_offset + ADDR1,
	mva_msb, 0x1FFF, preg_tbl);
	//0c
	adl_write2regtable_2bytes(clit_offset + DEPTH,
	max_len, 0xFFFF, preg_tbl);
	//10
	adl_write2regtable_2bytes(clit_offset + DMA_LEN,
	max_len, 0xFFFF, preg_tbl);
	//14
	adl_write2regtable_2bytes(clit_offset + INIT_ADDR,
	init_addr, 0xFFFF, preg_tbl);

	//0x7C0 wg
	adl_write2regtable_2bytes(ADL_WG_OFFSET,
	0x0, 0xFFFF, preg_tbl);
	//00

	//enable sw trigger
	if (p_clt_ctx->clt_update_mode == 0) {

		adl_write2regtable_2bytes(clit_offset + EN,
			p_clt_ctx->enabled, 0x1, preg_tbl);

		adl_write2regtable_2bytes(clit_sw_trig_en_offset,
			1 << clit_sw_trig_bit_shift,
			1 << clit_sw_trig_bit_shift,
			preg_tbl);

		adl_write2regtable_2bytes(clit_sw_trig_offset,
			1 << clit_sw_trig_bit_shift,
			1 << clit_sw_trig_bit_shift,
			preg_tbl);

	} else {
		adl_write2regtable_2bytes(clit_sw_trig_en_offset,
			0 << clit_sw_trig_bit_shift,
			1 << clit_sw_trig_bit_shift,
			preg_tbl);

		adl_write2regtable_2bytes(clit_sw_trig_offset,
			0 << clit_sw_trig_bit_shift,
			1 << clit_sw_trig_bit_shift,
			preg_tbl);

		adl_write2regtable_2bytes(clit_offset + EN,
			(p_clt_ctx->enabled | 0x2), 0x3, preg_tbl);
	}
	adl_hal_client_flush(p_clt_ctx);
	return ADL_OK;
}


int adl_hal_init(void)
{
	if (adl_hal_init_done) {
		adl_printf("adl hal already inited\n");
		return ADL_OK;
	}

	adl_clt_regbase[ADL_DSYS_CLITA] =
		adl_reg_base[ADL_DSYS] + ADL_CLITA_OFFSET;

	adl_clt_regbase[ADL_DSYS_CLITB] =
		adl_reg_base[ADL_DSYS] + ADL_CLITB_OFFSET;

	adl_clt_regbase[ADL_DSYS_CLITE] =
		adl_reg_base[ADL_DSYS] + ADL_CLITE_OFFSET;

	adl_clt_regbase[ADL_DSYS_CLITJ] =
		adl_reg_base[ADL_DSYS] + ADL_CLITJ_OFFSET;

	adl_clt_regbase[ADL_MSYS_CLITA] =
		adl_reg_base[ADL_MSYS] + ADL_CLITA_OFFSET;

	adl_clt_regbase[ADL_MSYS_CLITB] =
		adl_reg_base[ADL_MSYS] + ADL_CLITB_OFFSET;

	adl_clt_regbase[ADL_MSYS_CLITC] =
		adl_reg_base[ADL_MSYS] + ADL_CLITC_OFFSET;

	adl_clt_regbase[ADL_MSYS_CLITD] =
		adl_reg_base[ADL_MSYS] + ADL_CLITD_OFFSET;

	adl_clt_regbase[ADL_MSYS_CLITE] =
		adl_reg_base[ADL_MSYS] + ADL_CLITE_OFFSET;

	adl_info("adl dsys reg base 0x%p 0x%p\n",
		(void *)adl_clt_regbase[ADL_DSYS_CLITA],
		(void *)adl_clt_regbase[ADL_DSYS_CLITB]);

	adl_info("adl dsys reg base 0x%p 0x%p\n",
		(void *)adl_clt_regbase[ADL_DSYS_CLITE],
		(void *)adl_clt_regbase[ADL_DSYS_CLITJ]);

	adl_info("adl msys reg base 0x%p 0x%p\n",
		(void *)adl_clt_regbase[ADL_MSYS_CLITA],
		(void *)adl_clt_regbase[ADL_MSYS_CLITB]);

	adl_info("adl msys reg base 0x%p 0x%p 0x%p\n",
		(void *)adl_clt_regbase[ADL_MSYS_CLITC],
		(void *)adl_clt_regbase[ADL_MSYS_CLITD],
		(void *)adl_clt_regbase[ADL_MSYS_CLITE]);

	/*ml hal init done*/
	adl_hal_init_done = 1;
	return ADL_OK;
}

