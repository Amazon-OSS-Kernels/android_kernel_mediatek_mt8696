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

#ifndef _DISP_SYS_HAL_H_
#define _DISP_SYS_HAL_H_

#include "disp_reg.h"
#include <linux/printk.h>
#include <linux/types.h>
#include "disp_path.h"

extern struct disp_sys_context_t disp_sys;

#define DISPSYS_BASE disp_sys.init_param.dispsys_reg_base
#define MMSYS_BASE disp_sys.init_param.mmsys_reg_base

enum DISP_SYS_IRQ_BIT {
	DISP_SYS_IRQ_DISP3_END = 0x1 << 0,
	DISP_SYS_IRQ_VDO3_UNDERRUN = 0x1 << 1,
	DISP_SYS_IRQ_DISP3_VSYNC = 0x1 << 2,
	DISP_SYS_IRQ_R2R_VSYNC = 0x1 << 5,
	DISP_SYS_IRQ_FEFIFO_VDO3 = 0x1 << 6,
	DISP_SYS_IRQ_DISP4_END = 0x1 << 13,
	DISP_SYS_IRQ_VDO4_UNDERRUN = 0x1 << 14,
	DISP_SYS_IRQ_DISP4_VSYNC = 0x1 << 15,
	DISP_SYS_IRQ_FEFIFO_VDO4 = 0x1 << 16
};

struct disp_sys_init_param {
	uintptr_t dispsys_reg_base;
	uintptr_t mmsys_reg_base;
};

struct disp_sys_context_t {
	bool inited;
	struct disp_sys_init_param init_param;
	uint64_t *reg_mode;
	union disp_sys_union_t *sw_disp_sys_reg;
};

#define DISP_SYS_LOG_D(format...) pr_debug("[DISPSYS] " format)
#define DISP_SYS_LOG_I(format...) pr_info("[DISPSYS] " format)

enum DISP_TYPE { DISP_MAIN, DISP_SUB };

void disp_sys_hal_shadow_en(bool en);
void disp_sys_hal_shadow_update(void);
void disp_sys_clear_irq(enum DISP_SYS_IRQ_BIT irq);
void disp_sys_clear_irq_all(void);
int disp_sys_hal_init(struct disp_sys_init_param *param);
int disp_sys_hal_video_preultra_en(enum DISP_TYPE type, bool en);
int disp_sys_hal_video_ultra_en(enum DISP_TYPE type, bool en);
void disp_sys_hal_mvdo_out_select(enum DISP_PATH_HW_ID id, bool enable);
void disp_sys_hal_m_film_grain_enable(bool enable);
void disp_sys_hal_m_hdr_vdo_fe_enable(bool enable);
void disp_sys_hal_svdo_out_select(bool enable);
void disp_sys_hal_s_film_grain_enable(bool enable);
void disp_sys_hal_s_hdr_vdo_fe_enable(bool enable);
void disp_sys_hal_set_hdr_drop(uint8_t id, bool en);
void disp_sys_hal_set_hdr_drop_shadow(uint8_t id, bool en);
void disp_sys_hal_deinit(void);
void disp_sys_hal_update_hw_register(void);

#endif
