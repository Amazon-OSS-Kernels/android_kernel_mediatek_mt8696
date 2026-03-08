/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_STATIC_POWER_MTK8696_H__
#define __MTK_STATIC_POWER_MTK8696_H__

//#define SPOWER_NOT_READY 1

//#define WITHOUT_LKG_EFUSE

#define PRECISE_NODE

/* mv, "Leakage domain" */
#define V_OF_FUSE_CPU 1118 /* BCPU/LCPU/MCPU */
#define V_OF_FUSE_GPU 800 /* GPU */
#define V_OF_FUSE_VSRAM_PROC 1150 /* VSRAM_LCPU */

/* "ATE Test condition" */
#define T_OF_FUSE 25

/* devinfo offset for each bank */
/* CCI use LL leakage */
/* "Efuse_address" */
#define DEVINFO_IDX_LL 10 //0x11C107B8[7:0]

#define DEVINFO_OFF_LL 0
/* default leakage value for each bank */
#define DEF_CPULL_LEAKAGE 0

enum {
	MTK_SPOWER_CPULL,
	/* MTK_SPOWER_CCI, */
	/* MTK_SPOWER_GPU, */
	/* MTK_SPOWER_VSRAM_PROC, */
	MTK_SPOWER_MAX
};

enum {
	MTK_LL_LEAKAGE,
	/* MTK_CCI_LEAKAGE, */
	/* MTK_GPU_LEAKAGE, */
	/* MTK_VSRAM_PROC_LEAKAGE, */
	MTK_LEAKAGE_MAX
};

/* record leakage information that read from efuse */
struct spower_leakage_info {
	const char *name;
	unsigned int devinfo_idx;
	unsigned int devinfo_offset;
	unsigned int value;
	unsigned int v_of_fuse;
	int t_of_fuse;
};

extern struct spower_leakage_info spower_lkg_info[MTK_SPOWER_MAX];

/* efuse mapping */
/* 3967 modify */
#define LL_DEVINFO_DOMAIN (BIT(MTK_SPOWER_CPULL))

/* used to calculate total leakage that search from raw table */
#define DEFAULT_CORE_INSTANCE 4
#define DEFAULT_LL_CORE_INSTANCE 4
#define DEFAULT_INSTANCE 1

extern char *spower_name[];
extern char *leakage_name[];
extern int default_leakage[];
extern int devinfo_idx[];
extern int devinfo_offset[];
extern int devinfo_table[];

#endif
