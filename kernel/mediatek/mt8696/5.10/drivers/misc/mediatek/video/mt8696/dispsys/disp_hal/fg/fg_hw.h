/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#ifndef __FG_HW_H__
#define __FG_HW_H__

#include <linux/types.h>

#define FG_REG_XX(n)		((n) * 4)

/* reg 0x00 */
#define FG_REG_AR_COFF_Y_0	(FG_REG_XX(0))
/* reg 0x18 */
#define FG_REG_AR_COFF_CB_0	(FG_REG_XX(0x6))
/* reg 0x34 */
#define FG_REG_AR_COFF_CR_0	(FG_REG_XX(0xD))

#endif
