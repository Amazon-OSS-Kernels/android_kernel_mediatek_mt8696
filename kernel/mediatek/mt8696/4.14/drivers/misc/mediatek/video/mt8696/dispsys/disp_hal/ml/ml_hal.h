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
#ifndef _MENULOAD_HAL_H_
#define _MENULOAD_HAL_H_

#include "ml_hw.h"
#include <linux/types.h>
#include "mt-plat/sync_write.h"


enum ML_REG_ID {
	ML_MSYS = 0,
	ML_DSYS = 1,
	ML_REG_ID_MAX,
};
#define ML_CMD_ALIGN 4
#define MlWriteREG(reg, val) \
	mt_reg_sync_writel(val, (unsigned long *)(reg))
#define MlReadREG(reg) __raw_readl((unsigned long *)(reg))

#define MlWriteREGMsk(reg, val, msk) \
	MlWriteREG((reg), (MlReadREG(reg) & (~(msk))) | \
	((val) & (msk)))


#define reg_ml_bmsk_en(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

#define reg_ml_depth(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

#define reg_ml_en(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

#define reg_ml_addr0(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

#define reg_ml_addr1(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

#define reg_ml_ds_msk(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

#define reg_ml_ds_w_en(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

#define reg_ml_trig_md(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

#define reg_ml_64b_cmd_en(reg, val, msk) \
	MlWriteREGMsk(reg, val, msk)

int ml_hal_init(void);
int ml_hal_uninit(void);
int ml_hal_isr(void);
void ml_hal_set_test_reg(uint8_t ip,
	dma_addr_t pa, uint32_t len);
extern uintptr_t ml_reg_base[ML_REG_ID_MAX];

#endif

