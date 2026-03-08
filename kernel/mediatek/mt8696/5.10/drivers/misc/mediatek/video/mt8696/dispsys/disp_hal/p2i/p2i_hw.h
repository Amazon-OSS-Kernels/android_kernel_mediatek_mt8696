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


#ifndef _P2I_HW_H_
#define _P2I_HW_H_

#include "disp_reg.h"

/* ********************************************************************* */
/* VDOUT System & Clock Macros */
/* ********************************************************************* */

#define P2I_CTRL	0x00
#define P2I_CST_ON			(1 << 0)
#define OP_MODE				(3 << 1)
#define TV_FLD				(1 << 3)
#define TVP_FLD_INV			(1 << 4)
#define ADJ_TOTAL_EN		(1 << 5)
#define P2I_INV_LINE_EVEN	(1 << 8)
#define LINE_SHIFT			(1 << 9)
#define AUTO_R_NOEQ_W_EN	(1 << 11)
#define CI_SEL				(1 << 16)
#define CI_ROUND_EN			(1 << 17)
#define CI_REPEAT_EN		(1 << 18)
#define CI_VRF_OFF			(1 << 19)
#define CI_REPEAT_SEL		(1 << 20)
#define HRF_MODE			(1 << 22)
#define VRF_MODE			(1 << 23)
#define CI_Y_OFFSET			(1 << 24)
#define CI_C_OFFSET			(1 << 26)
#define CI_VRF_OPT			(1 << 28)
#define CI_HRF_OPT			(1 << 29)


#define P2I_H_TIME0 0x04
#define P2I_H_TIME1 0x08
#define P2I_H_TIME2 0x0c
#define P2I_V_TIME0 0x10
#define P2I_V_TIME1 0x14
#define P2I_V_TIME2 0x18
#define CI_H_TIME0	0x1c
#define CI_H_TIME1	0x20
#define CI_H_TIME2	0x24
#define CI_V_TIME0	0x28
#define CI_V_TIME1	0x2c
#define VH_TOTAL	0x30
#define P2I_CTRL_2  0x34
#define LINE_SHIFT_MSK	0x03


#endif
