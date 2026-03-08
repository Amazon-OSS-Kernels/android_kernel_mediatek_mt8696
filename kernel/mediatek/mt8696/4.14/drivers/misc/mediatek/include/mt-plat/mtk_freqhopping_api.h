/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef __FREQHOPPING_DRV_H
#define __FREQHOPPING_DRV_H

#include <linux/proc_fs.h>
#include "mtk_freqhopping.h"

int mt_freqhopping_devctl(unsigned int cmd, void *args);
int mt_dfs_armpll(unsigned int pll, unsigned int dds);
int mt_dfs_general_pll(unsigned int pll_id, unsigned int target_dds);
int freqhopping_config(unsigned int pll_id, unsigned long def_set_idx,
			unsigned int enable);
#endif
