/*
 * Copyright (C) 2020 MediaTek Inc.
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

#include "disp_hw_mgr.h"
#include "disp_type.h"
#include "ml_hw.h"
#include "disp_ml_if.h"
#include "disp_ml_cmd.h"
#include "ml_hal.h"



int ml_hal_init_done;

int ml_hal_init(void)
{
	if (ml_hal_init_done) {
		ml_printf("ml hal already inited\n");
		return ML_ST_OK;
	}

	/*ml hal init done*/
	ml_hal_init_done = 1;
	return ML_ST_OK;
}

int disp_ml_hal_uninit(void)
{
	return ML_ST_OK;
}


void ml_hal_set_test_reg(uint8_t ip, dma_addr_t mva, uint32_t len)
{
	dma_addr_t mva_msb = 0;
	dma_addr_t mva_lsb = 0;

	uintptr_t base_reg = 0;
	uint32_t value = 0;

	mva_msb = ((0xffff0000 & (mva >> 5)) >> 16);
	mva_lsb = ((0x0000ffff & (mva >> 5)));

	if (ip == ML_DSYS_IP) {
		base_reg = ml_reg_base[ML_DSYS];

		/*dsys bmask enable*/
		reg_ml_bmsk_en((base_reg + ML_MSK_EN), 0x1, 0x1);

		/*08 set en and sw triggle mode*/
		value = 0x8000 | len/4;
		reg_ml_en((base_reg + ML_EN), value, 0xffff);

		/*04 ml read length*/
		value = len/4;
		reg_ml_depth((base_reg + ML_DEPTH), value, 0xffff);

		/*0c ml addr0*/
		reg_ml_addr0((base_reg + ML_ADDR0), mva_lsb, 0xffff);

		/*10 ml addr1*/
		reg_ml_addr1((base_reg + ML_ADDR1), mva_msb, 0x1fff);

		/*1c ml ds msk*/
		reg_ml_ds_msk((base_reg + ML_DS_MSK), 0xc008, 0xffff);

		/*40 ml ds write en*/
		reg_ml_ds_w_en((base_reg + ML_DS_WE), 0x1004, 0xffff);

		/*1c0 ml 64bit cmd en*/
		reg_ml_64b_cmd_en((base_reg + ML_64B_EN), 0x8003, 0xffff);

		/*74 ml sw trigge mode*/
		reg_ml_64b_cmd_en((base_reg + ML_TRIG_MD), 0x1000, 0x1000);

		/*74 ml sw trigge */
		reg_ml_64b_cmd_en((base_reg + ML_TRIG_MD), 0x9000, 0x9000);
	} else {
		base_reg = ml_reg_base[ML_MSYS];

		/*dsys bmask enable*/
		reg_ml_bmsk_en((base_reg + ML_MSK_EN), 0x1, 0x1);

		/*08 set en and sw triggle mode*/
		value = 0x8000 | len/4;
		reg_ml_en((base_reg + ML_EN), value, 0xffff);

		/*04 ml read length*/
		value = len/4;
		reg_ml_depth((base_reg + ML_DEPTH), value, 0xffff);

		/*0c ml addr0*/
		reg_ml_addr0((base_reg + ML_ADDR0), mva_lsb, 0xffff);

		/*10 ml addr1*/
		reg_ml_addr1((base_reg + ML_ADDR1), mva_msb, 0x1fff);

		/*1c ml ds msk*/
		reg_ml_ds_msk((base_reg + ML_DS_MSK), 0xc008, 0xffff);

		/*40 ml ds write en*/
		reg_ml_ds_w_en((base_reg + ML_DS_WE), 0x1004, 0xffff);

		/*1c0 ml 64bit cmd en*/
		reg_ml_64b_cmd_en((base_reg + ML_64B_EN), 0x8003, 0xffff);

		/*74 ml sw trigge mode*/
		reg_ml_64b_cmd_en((base_reg + ML_TRIG_MD), 0x1000, 0x1000);

		/*74 ml sw trigge */
		reg_ml_64b_cmd_en((base_reg + ML_TRIG_MD), 0x9000, 0x9000);
	}

}

