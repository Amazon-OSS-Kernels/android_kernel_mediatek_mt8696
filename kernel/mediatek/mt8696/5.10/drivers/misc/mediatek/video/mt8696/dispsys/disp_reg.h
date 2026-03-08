/*
 * Copyright (C) 2015-2020 MediaTek Inc.
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

#ifndef _DISP_REG_H_
#define _DISP_REG_H_

#include "disp_type.h"
#include "mt-plat/sync_write.h"
#include <linux/types.h>
#include "disp_hw_mgr.h"
#ifdef DISP_GCE_SUPPORT
#include "mtk-cmdq.h"
#endif

#ifdef DISP_GCE_SUPPORT
#define WriteREG32_Cmdq(pkt, _reg32_, _val_) do {	\
	if (pkt == NULL)	\
		mt_reg_sync_writel(_val_, (unsigned long *)(_reg32_))	\
	else	\
		cmdq_pkt_write_value_addr(pkt, _reg32_, _val_, (~0x0))	\
} while (0)
#define WriteREG32Msk_Cmdq(pkt, arg, val, msk) do { \
	if (pkt == NULL)  \
		WriteREG32((arg), (ReadREG32(arg) & (~((uint32_t)(msk)))) |  \
			(((uint32_t)(val)) & ((uint32_t)(msk))))  \
	else  \
		cmdq_pkt_write_value_addr(pkt, arg, val, msk)	\
} while (0)
#else
#define WriteREG32_Cmdq(pkt, _reg32_, _val_)                           \
	mt_reg_sync_writel(_val_, (unsigned long *)(_reg32_))

#define WriteREG32Msk_Cmdq(pkt, arg, val, msk)                         \
	WriteREG32((arg), (ReadREG32(arg) & (~((uint32_t)(msk)))) |  \
			(((uint32_t)(val)) & ((uint32_t)(msk))))
#endif
#define WriteREG32(_reg32_, _val_)                                    \
	mt_reg_sync_writel(_val_, (unsigned long *)(_reg32_))

#define ReadREG32(reg32) __raw_readl((unsigned long *)(reg32))

#define WriteREG32Msk(arg, val, msk)                                   \
	WriteREG32((arg), (ReadREG32(arg) & (~((uint32_t)(msk)))) |     \
				  (((uint32_t)(val)) & ((uint32_t)(msk))))
#define REG_MASK(idx) ((uint64_t)1LL << (idx))

#define IS_REG_SET(r, mask) ((r) & (mask))
#define REG_SET(r, mask) ((r) |= (mask))
#define REG_RESET(r, mask) ((r) &= ~(mask))

#endif
