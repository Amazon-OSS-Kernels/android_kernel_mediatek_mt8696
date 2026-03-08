/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __FG_HW_H__
#define __FG_HW_H__

#include <linux/types.h>

#define FG_REG_MAIN_BASE	0x15011000
#define FG_REG_SUB_BASE		0x15014000
#define FG_REG_MAX_LEN		0x1000
#define FG_REG_RANG_BIT		12
#define FG_REG_BASE(reg)	((reg) >> FG_REG_RANG_BIT)

#define FG_SW_FILTER_EN(en)	((en) << 14)
#define FG_REG_XX(n)		((n) * 4)

/* reg 0x00 */
#define FG_REG_AR_COFF_Y_0	(FG_REG_XX(0))
/* reg 0x18 */
#define FG_REG_AR_COFF_CB_0	(FG_REG_XX(0x6))
/* reg 0x34 */
#define FG_REG_AR_COFF_CR_0	(FG_REG_XX(0xD))

#define FG_REG_DUMP_PITCH	0x10
#define FG_REG_OFFSET_1		0x4
#define FG_REG_OFFSET_2		0x8
#define FG_REG_OFFSET_3		0xC

#endif /* __FG_HW_H__ */
