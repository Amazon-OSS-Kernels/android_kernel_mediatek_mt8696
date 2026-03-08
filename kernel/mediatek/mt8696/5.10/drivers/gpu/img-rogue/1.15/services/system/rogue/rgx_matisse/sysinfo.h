/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#if !defined(__SYSINFO_H__)
#define __SYSINFO_H__



/*!< System specific poll/timeout details */
#if defined(PVR_LINUX_USING_WORKQUEUES)
#define MAX_HW_TIME_US				(1000000)
#define DEVICES_WATCHDOG_POWER_ON_SLEEP_TIMEOUT  (10000)
#define DEVICES_WATCHDOG_POWER_OFF_SLEEP_TIMEOUT (3600000)
#define WAIT_TRY_COUNT				(20000)
#else
#define MAX_HW_TIME_US				(5000000)
#define DEVICES_WATCHDOG_POWER_ON_SLEEP_TIMEOUT  (10000)
#define DEVICES_WATCHDOG_POWER_OFF_SLEEP_TIMEOUT (3600000)
#define WAIT_TRY_COUNT				(100000)
#endif

#define SYS_DEVICE_COUNT 3 /* RGX, DISPLAY (external), BUFFER (external) */

#define SYS_PHYS_HEAP_COUNT		1

#define SYS_RGX_OF_COMPATIBLE "mediatek,mt8696-clovelly"

#if defined(__linux__)
#define SYS_RGX_DEV_NAME    "pvrsrvkm"
/* #if defined(SUPPORT_DRM) */
/*
 * Use the static bus ID for the platform DRM device.
 */
#if defined(PVR_DRM_DEV_BUS_ID)
#define	SYS_RGX_DEV_DRM_BUS_ID	PVR_DRM_DEV_BUS_ID
#else
#define SYS_RGX_DEV_DRM_BUS_ID	"platform:pvrsrvkm"
#endif	/* defined(PVR_DRM_DEV_BUS_ID) */
/* #endif */	/* defined(SUPPORT_DRM) */
#endif

#endif	/* !defined(__SYSINFO_H__) */
