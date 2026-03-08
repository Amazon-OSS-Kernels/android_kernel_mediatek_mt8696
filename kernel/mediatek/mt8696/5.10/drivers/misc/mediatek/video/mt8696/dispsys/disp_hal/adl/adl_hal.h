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

#ifndef _AUTODOWNLOAD_HAL_H_
#define _AUTODOWNLOAD_HAL_H_
#include <linux/dma-mapping.h>
#include "mt-plat/sync_write.h"
#include "disp_adl_if.h"


enum ADL_CLIT_RST_ID {
	ADL_DSYS_CLIT_RST = 0,
	ADL_MSYS_CLIT_RST = 1,
	ADL_CLIT_RST,
};

enum ADL_CLIT_MODE {
	VFDE_END = 0,
	VS_SOF = 1,
	SW_TIGER = 2,
	MODE_MAX,
};

#define AdlWriteREG(reg, val) \
	mt_reg_sync_writel(val, (unsigned long *)(reg))
#define AdlReadREG(reg) __raw_readl((unsigned long *)(reg))


#define AdlWriteREGMsk(reg, val, msk) \
	AdlWriteREG((reg), (AdlReadREG(reg) & ((~((uint32_t)(msk))))) \
	| (((uint32_t)(val)) & ((uint32_t)(msk))))

#define reg_adl_en(reg, val, msk) \
	AdlWriteREGMsk(reg, val, msk)

#define reg_adl_dram_lsb_addr(reg, val, msk) \
	AdlWriteREGMsk(reg, val, msk)

#define reg_adl_dram_msb_addr(reg, val, msk) \
	AdlWriteREGMsk(reg, val, msk)

#define reg_adl_length(reg, val, msk) \
	AdlWriteREGMsk(reg, val, msk)

#define reg_adl_dma_depth(reg, val, msk) \
	AdlWriteREGMsk(reg, val, msk)

#define reg_adl_init_addr(reg, val, msk) \
	AdlWriteREGMsk(reg, val, msk)

#define reg_adl_wg(reg, val, msk) \
	AdlWriteREGMsk(reg, val, msk)

extern struct disp_adl_clt_context adl_clt_ctx[ADL_CLIT_REG_MAX];
int adl_hal_init(void);
int adl_hal_cfg_client(uint8_t client,
	dma_addr_t dst_mva, uint32_t max_len);

int adl_hal_cfg_client_enable(uint8_t client, bool en, uint8_t en_mode);
int adl_hal_client_flush(struct disp_adl_clt_context *p_clt_ctx);

#endif

