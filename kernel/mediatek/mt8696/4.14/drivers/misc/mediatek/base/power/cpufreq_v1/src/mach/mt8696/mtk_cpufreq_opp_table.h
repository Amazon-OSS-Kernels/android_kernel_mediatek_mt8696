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

#include "mtk_cpufreq_struct.h"
#include "mtk_cpufreq_config.h"

/* 8696 merged case*/
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_LL_FY		1794000         /* KHz */
#define CPU_DVFS_FREQ1_LL_FY		1760000         /* KHz */
#define CPU_DVFS_FREQ2_LL_FY		1730000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY		1696000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY		1600000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY		1500000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY		1400000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY		1300000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY		1200000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY		1100000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY		1000000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY		900000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY		825000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY		750000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY		675000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY		600000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY		1230000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY		1202000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY		1174000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY		1146000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY		1097000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY		1048000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY		999000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY		950000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY		846000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY		778000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY		709000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY		640000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY		589000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY		537000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY		485000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY		400000		/* KHz */

/* for DVFS OPP table L */
#define CPU_DVFS_VOLT0_VPROC1_FY	 111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY	 106250		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY	 100625		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY	 95000		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY	 90000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY	 85000		/* 10uV */

/* for DVFS OPP table CCI */
#define CPU_DVFS_VOLT0_VPROC3_FY	 111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC3_FY	 106250		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC3_FY	 100625		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC3_FY	 95000		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC3_FY	 91250		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC3_FY	 87500		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC3_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC3_FY	 85000		/* 10uV */

/* 8696_FY1 separated case */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_LL_FY1		1794000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY1		1760000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY1		1730000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY1		1696000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY1		1600000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY1		1500000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY1		1400000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY1		1300000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY1		1200000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY1		1100000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY1		1000000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY1		900000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY1		825000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY1		750000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY1		675000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY1		600000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY1		1230000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY1		1202000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY1		1174000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY1		1146000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY1		1097000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY1		1048000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY1		999000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY1		950000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY1		846000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY1		778000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY1		709000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY1		640000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY1		589000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY1		537000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY1		485000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY1		400000		/* KHz */

/* for DVFS OPP table L */
#define CPU_DVFS_VOLT0_VPROC1_FY1	111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY1	106250		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY1	100625		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY1	95000		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY1	90000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY1	85000		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY1	80000		 /* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY1	77500		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY1	75000		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY1	72500		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY1	70000		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY1	67500		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY1	65625		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY1	63750		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY1	61875		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY1	60000		/* 10uV */

/* for DVFS OPP table CCI */
#define CPU_DVFS_VOLT0_VPROC3_FY1	111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC3_FY1	106250		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC3_FY1	100625		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC3_FY1	95000		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC3_FY1	91250		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC3_FY1	87500		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC3_FY1	83750		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC3_FY1	80000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC3_FY1	76250		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC3_FY1	73750		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC3_FY1	71250		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC3_FY1	68750		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC3_FY1	66875		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC3_FY1	65000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC3_FY1	63125		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC3_FY1	60000		/* 10uV */

/* DVFS OPP table */
#define OPP_TBL(cluster, seg, lv, vol)	\
static struct mt_cpu_freq_info opp_tbl_##cluster##_e##lv##_0[] = {        \
	OP                                                                \
(CPU_DVFS_FREQ0_##cluster##_##seg, CPU_DVFS_VOLT0_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ1_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ2_##cluster##_##seg, CPU_DVFS_VOLT2_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ3_##cluster##_##seg, CPU_DVFS_VOLT3_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ4_##cluster##_##seg, CPU_DVFS_VOLT4_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ5_##cluster##_##seg, CPU_DVFS_VOLT5_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ6_##cluster##_##seg, CPU_DVFS_VOLT6_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ7_##cluster##_##seg, CPU_DVFS_VOLT7_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ8_##cluster##_##seg, CPU_DVFS_VOLT8_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ9_##cluster##_##seg, CPU_DVFS_VOLT9_VPROC##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ10_##cluster##_##seg, CPU_DVFS_VOLT10_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ11_##cluster##_##seg, CPU_DVFS_VOLT11_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ12_##cluster##_##seg, CPU_DVFS_VOLT12_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ13_##cluster##_##seg, CPU_DVFS_VOLT13_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ14_##cluster##_##seg, CPU_DVFS_VOLT14_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ15_##cluster##_##seg, CPU_DVFS_VOLT15_VPROC##vol##_##seg), \
}

OPP_TBL(LL,   FY, 0, 1); /* opp_tbl_LL_e0_0   */
OPP_TBL(CCI, FY, 0, 3); /* opp_tbl_CCI_e0_0 */

OPP_TBL(LL,   FY1, 1, 1); /* opp_tbl_LL_e1_0   */
OPP_TBL(CCI, FY1, 1, 3); /* opp_tbl_CCI_e1_0 */


/* v1.3 */
static struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0,
			ARRAY_SIZE(opp_tbl_LL_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_0,
			ARRAY_SIZE(opp_tbl_LL_e1_0) },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_CCI_e0_0,
			ARRAY_SIZE(opp_tbl_CCI_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_CCI_e1_0,
			ARRAY_SIZE(opp_tbl_CCI_e1_0) },
	},
};

/* 16 steps OPP table */
static struct mt_cpu_freq_method opp_tbl_method_LL_FY[] = {	/* 8696 */
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_FY[] = {	/* 8696 */
	/* POS,	CLK */
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_FY1[] = {	/* 8696 */
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_FY1[] = {	/* 8696 */
	/* POS,	CLK */
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
};


struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_FY1 },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_CCI_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_CCI_FY1 },
	},
};
