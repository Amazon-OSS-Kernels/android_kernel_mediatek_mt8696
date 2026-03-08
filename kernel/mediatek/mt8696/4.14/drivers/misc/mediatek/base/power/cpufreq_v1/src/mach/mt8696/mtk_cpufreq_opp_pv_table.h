/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "mtk_cpufreq_config.h"
#include "mtk_cpufreq_platform.h"

#define NR_FREQ		16
#define ARRAY_COL_SIZE	4

static unsigned int FY_8696Tbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
	/* LL */
	{ 1794, 83, 1, 1 },
	{ 1760, 74, 1, 1 },
	{ 1730, 65, 1, 1 },
	{ 1696, 56, 2, 1 },
	{ 1600, 48, 2, 1 },
	{ 1500, 40, 2, 1 },
	{ 1400, 40, 2, 1 },
	{ 1300, 40, 2, 1 },
	{ 1200, 40, 2, 1 },
	{ 1100, 40, 2, 1 },
	{ 1000, 40, 2, 1 },
	{ 900, 40, 2, 1 },
	{ 825, 40, 4, 1 },
	{ 750, 40, 4, 1 },
	{ 675, 40, 4, 1 },
	{ 600, 40, 4, 1 },


	/* CCI */
	{ 1230, 83, 2, 1 },
	{ 1202, 74, 2, 1 },
	{ 1174, 65, 2, 1 },
	{ 1146, 56, 2, 1 },
	{ 1097, 50, 2, 1 },
	{ 1048, 44, 2, 1 },
	{ 999, 40, 2, 1 },
	{ 950, 40, 2, 1 },
	{ 846, 40, 2, 1 },
	{ 778, 40, 4, 1 },
	{ 709, 40, 4, 1 },
	{ 640, 40, 4, 1 },
	{ 589, 40, 4, 1 },
	{ 537, 40, 4, 1 },
	{ 485, 40, 4, 1 },
	{ 400, 40, 4, 1 },
};

static unsigned int FY1_8696Tbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
	/* LL */
	{ 1794, 83, 1, 1 },
	{ 1760, 74, 1, 1 },
	{ 1730, 65, 1, 1 },
	{ 1696, 56, 2, 1 },
	{ 1600, 48, 2, 1 },
	{ 1500, 40, 2, 1 },
	{ 1400, 32, 2, 1 },
	{ 1300, 28, 2, 1 },
	{ 1200, 24, 2, 1 },
	{ 1100, 20, 2, 1 },
	{ 1000, 16, 2, 1 },
	{ 900, 12, 2, 1 },
	{ 825, 9, 4, 1 },
	{ 750, 6, 4, 1 },
	{ 675, 3, 4, 1 },
	{ 600, 0, 4, 1 },

	/* CCI */
	{ 1230, 83, 2, 1 },
	{ 1202, 74, 2, 1 },
	{ 1174, 65, 2, 1 },
	{ 1146, 56, 2, 1 },
	{ 1097, 50, 2, 1 },
	{ 1048, 44, 2, 1 },
	{ 999, 38, 2, 1 },
	{ 950, 32, 2, 1 },
	{ 846, 26, 2, 1 },
	{ 778, 22, 4, 1 },
	{ 709, 18, 4, 1 },
	{ 640, 14, 4, 1 },
	{ 589, 11, 4, 1 },
	{ 537, 8, 4, 1 },
	{ 485, 5, 4, 1 },
	{ 400, 0, 4, 1 },
};
unsigned int *xrecordTbl[NUM_CPU_LEVEL] = {
	[CPU_LEVEL_0] = &FY_8696Tbl[0][0],
	[CPU_LEVEL_1] = &FY1_8696Tbl[0][0],
};
