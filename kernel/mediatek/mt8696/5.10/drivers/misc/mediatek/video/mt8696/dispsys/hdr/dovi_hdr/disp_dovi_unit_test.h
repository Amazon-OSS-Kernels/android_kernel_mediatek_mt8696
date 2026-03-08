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

#ifndef __DISP_DOVI_UNIT_TEST_H__
#define __DISP_DOVI_UNIT_TEST_H__

#if IS_ENABLED(CONFIG_DOVI_UT_SUPPORT)
#include "ml_hal.h"
#include "disp_dovi_main.h"
#include "mt-plat/sync_write.h"


#define DV_UNIT_TEST_IOMMU_SUPPORT 0

#define ut_printf(fmt, args...) pr_info("[dv_ut] "fmt, ##args)

//#define BYTE_PER_LINE 32
#define HDR_FE_LUT_SIZE (512)
#define HDR_MD_SIZE (128)

//512 line x 32 bytes perline
#define ADL_UT_LUT_BUF_SIZE 16384

//1024 line x 32 bytes perline(EMU use 128x32)
#define ADL_UT_SCM_BUF_SIZE 4096

//1280 cmd x 8bytes
#define ML_DSYS_UT_BUF_SIZE 10240

//600 cmd x 8bytes
#define ML_MSYS_UT_BUF_SIZE 4800

#define DV_UT_CASE_MAX_ID 10

#define UT_LAYER 4

#define UT_WriteREG(arg, val) \
	mt_reg_sync_writel((val), (unsigned long *)(arg))
#define UT_ReadREG(arg) __raw_readl((unsigned long *)(arg))
#define UT_WriteREGMsk(arg, val, msk) \
	UT_WriteREG((arg), (UT_ReadREG(arg) & (~(msk))) | ((val) & (msk)))


enum dovi_idk_version {
	DOVI_IDK_230 = 0,
	DOVI_IDK_241 = 1,
	DOVI_IDK_242 = 2,
	DOVI_IDK_MAX
};

enum dv_ut_case {
	MAIN_HDR_CASE = 0,
	SUB_HDR_CASE = 1,
	FHD_HDR_CASE = 2,
	UHD_HDR_CASE = 3,
	MAIN_FHD_MIX_CASE = 4,
	SUB_UHD_MIX_CASE = 5,
	ALL_LAYER_MIX_CASE = 6,
	CASE_MAX
};

enum dovi_resolution {
	DOVI_480P = 0,
	DOVI_576P,
	DOVI_720P,
	DOVI_1080P,
	DOVI_2160P,
	DOVI_2161P,
	DOVI_RESOLUTION_MAX
};

enum dovi_format {
	DOVI_SDR = 0,
	DOVI_HDR10 = 1,
	DOVI_STD = 2,
	DOVI_HLG = 3,
	DOVI_FORMAT_MAX
};

enum dovi_layer {
	DV_MAIN_FE,
	DV_SUB_FE,
	DV_FHD_FE,
	DV_UHD_FE,
	DV_BE
};

enum dovi_unit_id {
	DOVI_UT_INIT = 0,
	DOVI_EXTERN_BYPASS,
	DOVI_INTER_BYPASS,
	DOVI_UT_SET_TIMING_PATH,
	DOVI_CASE_TEST,
	DOVI_BE_REORDE_BYPASS,
	DOVI_BE_SCM_BYPASS,
	DOVI_BE_DITHER_BYPASS,
	DOVI_UNIT_ID_MAX
};

struct dv_layer_info {
	bool on;
	uint32_t case_id;
	uint32_t width;
	uint32_t heigh;
	enum dovi_format in_format;
	uint32_t *lut_addr;
	uint32_t lut_len;
};
struct dovi_unit_info_t {
	struct dv_layer_info dv_layer[UT_LAYER];
	enum dovi_format out_format;
	enum dovi_resolution out_res;
	bool use_ll;
	bool ll_rgb;
	uint32_t *md_addr;
	uint32_t md_len;
	uint32_t *dsys_ml_addr;
	uint32_t dsys_ml_cmd_len;
	uint32_t *msys_ml_addr;
	uint32_t msys_ml_cmd_len;
};

extern struct device *dovi_dev;
extern uintptr_t ml_reg_base[ML_REG_ID_MAX];
extern char *dovi_reg_base[DOVI_CORE_MAX];
void dovi_ut_external_bypass(uint32_t id1);
void dovi_ut_internal_bypass(uint32_t id1);
void dovi_idk_dump_vdo_bypass(uint32_t id1);
void disp_dovi_wakeup_routine(void);
void disp_dovi_ut_ml_update(void);
int disp_dovi_default_path(uint32_t value);
int disp_dovi_default_path_init(uint32_t option);
void dovi_set_unit_test_info(uint32_t out_res, uint32_t out_fmt,
	uint32_t ll, uint32_t ll_rgb,
	uint32_t *layer_on, uint32_t *case_id);
void dovi_set_path_info(uint32_t timing);
void disp_dovi_reg_test(uint32_t value);
void dovi_ut_init(uint32_t en);
void dovi_set_ut_case(uint32_t case_id);
void dovi_set_ut_flush(void);
void dovi_slt_case(uint32_t slt_case);
#endif

#endif
