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

#include <linux/platform_device.h>

#include "mt_emi.h"

enum {
	MASTER_ARM0 = 0,
	MASTER_ARM1 = 1,
	MASTER_MM0 = 2,
	MASTER_MDMCU = 3,
	MASTER_MODEM = 4,
	MASTER_MM1 = 5,
	MASTER_GPU_0 = 6,
	MASTER_GPU_1 = 7
};
int is_md_master(unsigned int master_id)
{
#if 0
	if ((master_id & 0x7) == MASTER_MDMCU)
		return 1;

	if ((master_id & 0x7) == MASTER_MD)
		return 1;
#endif
	return 0;
}

void set_ap_region_permission(struct emi_region_info_t *p_region_info)
{
#if 0
#if (EMI_MPU_DGROUP_NUM == 1)
	SET_ACCESS_PERMISSION(apc, LOCK,
		FORBIDDEN, SEC_R_NSEC_RW, FORBIDDEN, NO_PROTECTION,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION);
#elif (EMI_MPU_DGROUP_NUM == 2)

	SET_ACCESS_PERMISSION(apc, LOCK,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
		FORBIDDEN, FORBIDDEN, NO_PROTECTION, NO_PROTECTION,
		FORBIDDEN, SEC_R_NSEC_RW, FORBIDDEN, NO_PROTECTION,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION);
#endif
#else
#if (EMI_MPU_DGROUP_NUM == 1)
	SET_ACCESS_PERMISSION(p_region_info->apc,
		NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
		NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION);
	p_region_info->lock_status = UNLOCK;
#elif (EMI_MPU_DGROUP_NUM == 2)
	SET_ACCESS_PERMISSION(p_region_info->apc,
		ALL_FORBIDDEN, ALL_FORBIDDEN, ALL_FORBIDDEN, ALL_FORBIDDEN,
		ALL_FORBIDDEN, ALL_FORBIDDEN, ALL_FORBIDDEN, ALL_FORBIDDEN,
		ALL_FORBIDDEN, ALL_FORBIDDEN, ALL_FORBIDDEN, ALL_FORBIDDEN,
		ALL_FORBIDDEN, ALL_FORBIDDEN, ALL_FORBIDDEN,
		SEC_W_NSEC_W_FORBIDDEN);
	p_region_info->lock_status = LOCK;
#endif

#endif
}

