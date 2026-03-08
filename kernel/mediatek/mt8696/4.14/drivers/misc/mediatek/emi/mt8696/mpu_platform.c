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

