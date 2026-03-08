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

#ifndef __DISP_ML_IF_H__
#define __DISP_ML_IF_H__

#include <linux/types.h>

#define ML_DRV_NAME  "disp_drv_ml"

#define ML_DTS_NAME_SIZE 30

#define ML_IOMMU_SUPPORT 1
#define ML_DSYS_CMD_MAX_SIZE 2000
#define ML_MSYS_CMD_MAX_SIZE 2000
#define ML_REG_COUNT (200)
#define M_HDR_FE_BASE 0x15012000
#define S_HDR_FE_BASE 0x15015000


#define ML_UPDATE_REG_TABLE_SIZE(ADDR, IDX, VAL, MASK, REG) do {  \
		ADDR[IDX] &= 0; \
		ADDR[IDX] |= (uint64_t)(VAL); \
		ADDR[IDX] |= (((uint64_t)(MASK)) << 48); \
		ADDR[IDX] |= ((uint64_t)((REG & 0x1FF) >> 2) << 16); \
		ADDR[IDX] |= ((uint64_t)((REG & 0x1FE00) >> 9) << 24); \
		ADDR[IDX] |= ((uint64_t)(0x53) << 32); \
} while (0)

#define ML_UPDATE_REG_TABLE(ADDR, IDX, VAL, MASK, REG) do {  \
		ADDR[IDX] |= (uint64_t)(VAL); \
		ADDR[IDX] |= (((uint64_t)(MASK)) << 48); \
		ADDR[IDX] |= ((uint64_t)((REG & 0x1FF) >> 2) << 16); \
		ADDR[IDX] |= ((uint64_t)((REG & 0x1FE00) >> 9) << 24); \
		ADDR[IDX] |= ((uint64_t)(0x53) << 32); \
} while (0)

enum ML_FUNC_STATUS {
	ML_ST_OK = 0,
	ML_ST_ERR = 1,
};

enum ML_STATE {
	ML_DISABLE = 0,
	ML_INIT = 1,
	ML_ENABLE = 2,
};

enum ML_SYS_IP {
	ML_MSYS_IP = 0,
	ML_DSYS_IP = 1,
	ML_IP_MAX,
};

enum ML_PATH {
	ML_DEF = 0,
	ML_DOVI = 1,
	ML_OPENHDR = 2,
	ML_PATH_MAX,
};

struct ml_reg_table {
	uint32_t depth;
	uint32_t *p_reg_addr;
	uint16_t *p_reg_value;
	uint16_t *p_reg_mask;
	uint8_t ml_ip;
	uint8_t reg_type; // 0: normal, 1: padding, 2: ctrl
	uint8_t path; //0:other,1: dovi,2:openhdr
};

struct ml_hw_reg_tbl {
	uint32_t depth;
	uint32_t address[ML_REG_COUNT];
	uint16_t value[ML_REG_COUNT];
	uint16_t mask[ML_REG_COUNT];
};

struct disp_ml_clt_context {
	uint32_t state;//0-disable,1-init,2 enable, 3 update
	bool reset;
	uint32_t cmd_depth;
	uintptr_t remap_reg_base;
	uint32_t hw_reg_base;
	uint32_t clt_update_mode; //sw tirg, vfd, vsyncdelay(0,1,2)
	uint32_t reg_conf_mode; //gce, cpu(0,1)
	uint64_t *lut_addr_va;
	dma_addr_t lut_addr_mva;
	uint32_t lut_addr_pa;
	struct ml_hw_reg_tbl *preg_tbl;
	struct cmdqRecStruct *gce_handle;
};

int disp_ml_init(struct disp_hw_common_info *info);
int disp_ml_deinit(void);
int disp_ml_suspend(void);
int disp_ml_resume(void);
int disp_ml_irq_handler(uint32_t irq);
int disp_ml_process_cmd(enum DISP_CMD cmd,
	void *data);
int disp_ml_write_reg_single(uint8_t ml_ip,
	uint32_t reg, uint16_t val, uint16_t mask);

int disp_ml_write_reg_multi(struct ml_reg_table *reg_tbl);
int disp_ml_set_disable(uint8_t ml_ip);
int disp_ml_isr(void);
void disp_ml_clk_on_off(uint8_t ip, bool en);
int disp_ml_get_setting(uint8_t ml_ip);
int disp_ml_set_buffer(uint8_t ml_ip, void *va, uint32_t len);
int disp_ml_clt_set_mode(enum ML_SYS_IP clit, uint8_t clt_update_mode,
	uint8_t reg_cfg_mode);
int disp_ml_client_flush(uint8_t ip);
int disp_ml_client_reset(uint8_t ip);
int disp_ml_get_setting(uint8_t ip);
extern void disp_hdr_get_cur_path(uint8_t *path);

extern struct device *ml_dev;

#endif
