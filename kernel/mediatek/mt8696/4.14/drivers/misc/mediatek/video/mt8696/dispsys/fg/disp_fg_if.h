/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __DISP_FG_IF_H__
#define __DISP_FG_IF_H__

#define FG_DRV_NAME "disp_drv_fg"

#define FG_DEFAULT_LOG		(1 << 0)
#define FG_FUNC_LOG		(1 << 1)
#define FG_DEBUG_LOG		(1 << 2)
#define FG_SW_FILTER_LOG	(1 << 3)
#define FG_SW_PERFORMANCE_LOG	(1 << 4)

#define FG_OK		0
#define FG_FAIL	-1

#define DISP_FG_SOF_NR		5

extern u32 fg_dbg_level;

#define fg_printf(level, string, args...)			\
do {								\
	if (fg_dbg_level & (level)) {				\
		pr_info(string, ##args);			\
	}							\
} while (0)

#define FG_LOG_I(string, args...) fg_printf(FG_DEFAULT_LOG,	\
	"[FG] %s | %d "string, \
	__func__, __LINE__, ##args)

#define FG_LOG_D(string, args...) fg_printf(FG_DEBUG_LOG,	\
	"[FG] %s | %d "string, \
	__func__, __LINE__, ##args)

#define FG_FUNC() fg_printf(FG_FUNC_LOG, "[FG] %s | %d\n",	\
	__func__, __LINE__)

#define FG_ERR(string, args...) pr_info("[FG] %s | %d error: "string,	\
	__func__, __LINE__, ##args)

#define FG_SW_FILTER(string, args...) \
	fg_printf(FG_SW_FILTER_LOG, "[FG] "string, ##args)

#define FG_SW_PER(string, args...) \
	fg_printf(FG_SW_PERFORMANCE_LOG, "[FG] "string, ##args)

struct disp_fg_sof {
	u32 sof_start;
	u32 sof_end;
};

void disp_fg_config(u32 fg_hw_id, struct mtk_av1_film_grain_params *fg_param);
void disp_fg_config_scale_info(u32 hw_id, struct video_scale_info *scale_info);
u8 *disp_fg_get_adl_tbl(u32 fg_hw_id);
struct mtk_av1_film_grain_params *disp_fg_get_param(u32 fg_hw_id);

int disp_fg_set_dbg_level_enable(uint32_t level, uint32_t enable);

extern int fg_sec_init(void);
extern void fg_dbg_dump_param(u32 hw_id);
#endif
