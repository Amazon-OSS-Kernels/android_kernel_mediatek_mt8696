/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "pvrsrv_device.h"
#include "rgxdevice.h"

#if !defined(__SYSCCONFIG_H__)
#define __SYSCCONFIG_H__


#define RGX_HW_SYSTEM_NAME "RGX HW"

#define RGX_HW_CORE_CLOCK_SPEED			(650000000)
#define SYS_RGX_ACTIVE_POWER_LATENCY_MS (50)


/* if *CONFIG_OF is not set, please makesure the following address and IRQ number are right */
/* #error RGX_GPU_please_fill_the_following_defines */
#define SYS_MTK_RGX_REGS_SYS_PHYS_BASE      0x13000000
#define SYS_MTK_RGX_REGS_SIZE               0x80000

#define MTK_PM_SUPPORT 1


/*****************************************************************************
 * system specific data structures
 *****************************************************************************/

#endif	/* __SYSCCONFIG_H__ */
