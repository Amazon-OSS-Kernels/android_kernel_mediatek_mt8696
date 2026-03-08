/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#ifndef __DISP_FG_DRV_H__
#define __DISP_FG_DRV_H__

#include "fg_hal.h"

#define FG_MAX_PARAM_NS		2

struct disp_fg_info {
	u32 fg_params_idx;
	struct fg_hw_reg_output fg_params_hw_reg[FG_MAX_PARAM_NS];
	struct fg_hw_adl_output fg_params_hw_adl[FG_MAX_PARAM_NS];
	struct fg_hw_reg_output *hw_reg;
	struct fg_hw_adl_output *hw_adl;
};

MS_BOOL disp_fg_handler(u32 fg_hw_id, struct mtk_av1_film_grain_params *fg_param,
			struct adl_src_tbl *adl_tbl);
void disp_fg_force_bypass(u32 fg_hw_id, bool bypass);

#endif
