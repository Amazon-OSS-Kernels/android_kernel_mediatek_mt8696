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

#ifndef _DISP_CFD_HW_H_
#define _DISP_CFD_HW_H_

#include <linux/types.h>

#define FE_CFD_MAX_LAYER_NUM 4
#define HAL_CFD_REG_NUM (0x1000/4)
#define HAL_CFD_REG_CONF_CNT (0x1000/4)

#define DISP_CFD_VDOFE0_REG_BASE (0x15012000)
#define DISP_CFD_VDOFE1_REG_BASE (0x15015000)
#define DISP_CFD_XVYCC0_REG_BASE (0x15016000)
#define DISP_CFD_XVYCC1_REG_BASE (0x15016800)
#define DISP_CFD_DISP_ML_REG_BASE (0x15013000)
#define DISP_CFD_DISP_ADL_REG_BASE (0x15013600)
#define DISP_CFD_GFXFE0_REG_BASE (0x14009000)
#define DISP_CFD_GFXFE1_REG_BASE (0x1400A000)
#define DISP_CFD_MM_ML_REG_BASE (0x14013000)
#define DISP_CFD_MM_ADL_REG_BASE (0x14013600)

#define CFD_REG_MASK_DEF (0xFFFFFFFF)


#endif
