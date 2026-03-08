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

#ifndef __MPU_H__
#define __MPU_H__

#define EMI_MPU_TEST		0

#define EMI_MPU_MAX_CMD_LEN	128
#define EMI_MPU_MAX_TOKEN	5
#define CCCI_STR_MAX_LEN	60

#define NO_PROTECTION           0
#define NSEC_W_FORBIDDEN        1
#define NSEC_R_FORBIDDEN        2
#define NSEC_RW_FORBIDDEN       3
#define SEC_W_FORBIDDEN         4
#define SEC_W_NSEC_W_FORBIDDEN  5
#define SEC_W_NSEC_R_FORBIDDEN	6
#define SEC_W_NSEC_RW_FORBIDDEN 7
#define SEC_R_FORBIDDEN         8
#define SEC_R_NSEC_W_FORBIDDEN  9
#define SEC_R_NSEC_R_FORBIDDEN  10
#define SEC_R_NSEC_RW_FORBIDDEN 11
#define SEC_RW_FORBIDDEN        12
#define SEC_RW_NSEC_W_FORBIDDEN 13
#define SEC_RW_NSEC_R_FORBIDDEN 14
#define ALL_FORBIDDEN           15

#define UNLOCK		0
#define LOCK		1

#define EMI_MPU_DGROUP_NUM	(EMI_MPU_DOMAIN_NUM / 8)

#if (EMI_MPU_DGROUP_NUM == 1)
#define SET_ACCESS_PERMISSION(apc_ary, d7, d6, d5, d4, d3, d2, d1, d0) \
do {\
	apc_ary[0] = 0;\
	apc_ary[0] = \
		(((unsigned int) d7) << 28) | (((unsigned int) d6) << 24) | \
		(((unsigned int) d5) << 20) | (((unsigned int) d4) << 16) | \
		(((unsigned int) d3) << 12) | (((unsigned int) d2) << 8) | \
		(((unsigned int) d1) << 4) | ((unsigned int) d0); \
} while (0)
#elif (EMI_MPU_DGROUP_NUM == 2)
#define SET_ACCESS_PERMISSION(apc_ary, \
	d15, d14, d13, d12, d11, d10, d9, d8, d7, d6, d5, d4, d3, d2, d1, d0) \
do { \
	apc_ary[1] = \
		(((unsigned int) d15) << 28) | (((unsigned int) d14) << 24) | \
		(((unsigned int) d13) << 20) | (((unsigned int) d12) << 16) | \
		(((unsigned int) d11) << 12) | (((unsigned int) d10) << 8) | \
		(((unsigned int) d9) << 4) | ((unsigned int) d8); \
	apc_ary[0] = \
		(((unsigned int) d7) << 28) | (((unsigned int) d6) << 24) | \
		(((unsigned int) d5) << 20) | (((unsigned int) d4) << 16) | \
		(((unsigned int) d3) << 12) | (((unsigned int) d2) << 8) | \
		(((unsigned int) d1) << 4) | ((unsigned int) d0); \
} while (0)
#endif

struct emi_region_info_t {
	unsigned long long start;
	unsigned long long end;
	unsigned int region;
	unsigned int lock_status;
	unsigned int apc[EMI_MPU_DGROUP_NUM];
};

struct mst_tbl_entry {
	u32 master;
	u32 port;
	u32 id_mask;
	u32 id_val;
	const char *note;
	const char *name;
};

extern int is_md_master(unsigned int master_id);
extern void set_ap_region_permission(struct emi_region_info_t *p_region_info);
extern int emi_mpu_set_protection(struct emi_region_info_t *region_info);
extern void clear_md_violation(void);

#endif				/* __MPU_H__ */
