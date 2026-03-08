// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "interrupt_support.h"
#include "pvrsrv_device.h"
#include "syscommon.h"
#include "sysconfig.h"
#include "physheap.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif
#include "mtk_mfg.h"
#include "mtk_gpu_misc.h"

#if defined(CONFIG_OF)
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <linux/platform_device.h>
struct platform_device *gpsPVRLDMDev;
#endif
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/syscalls.h>
#include <mtk_devinfo.h>
#define RGX_CR_ISP_GRIDOFFSET	(0x0FA0U)


static RGX_TIMING_INFORMATION	gsRGXTimingInfo;
static RGX_DATA			gsRGXData;
static PVRSRV_DEVICE_CONFIG	gsDevices[1];

static PHYS_HEAP_FUNCTIONS      gsPhysHeapFuncs;
static PHYS_HEAP_CONFIG         gsPhysHeapConfig;

#if defined(SUPPORT_LINUX_DVFS)
static IMG_OPP asOPPTable[] = {
	{ 700000, 275000000},
	{ 700000, 400000000},
	{ 800000, 560000000},
	{ 800000, 650000000},
	{ 900000, 750000000},
	{ 900000, 850000000}
};

#define LEVEL_COUNT (sizeof(asOPPTable) / sizeof(IMG_OPP))
#endif


/* CPU to Device physcial address translation */
static
void UMAPhysHeapCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
				   IMG_UINT32 ui32NumOfAddr,
				   IMG_DEV_PHYADDR *psDevPAddr,
				   IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psDevPAddr[0].uiAddr = psCpuPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1) {
		IMG_UINT32 ui32Idx;

		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr;
	}
}

/* Device to CPU physcial address translation */
static
void UMAPhysHeapDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
				   IMG_UINT32 ui32NumOfAddr,
				   IMG_CPU_PHYADDR *psCpuPAddr,
				   IMG_DEV_PHYADDR *psDevPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psCpuPAddr[0].uiAddr = psDevPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1) {
		IMG_UINT32 ui32Idx;

		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
			psCpuPAddr[ui32Idx].uiAddr = psDevPAddr[ui32Idx].uiAddr;
	}
}

static PVRSRV_ERROR MTKSysPrePowerState(IMG_HANDLE hSysData, PVRSRV_SYS_POWER_STATE eNewPowerState,
				 PVRSRV_SYS_POWER_STATE eCurrentPowerState,
				 PVRSRV_POWER_FLAGS ePwrFlags)
{
	if ((eNewPowerState == PVRSRV_SYS_POWER_STATE_OFF) &&
	    (eCurrentPowerState == PVRSRV_SYS_POWER_STATE_ON)) {
		mtk_mfg_disable_gpu();
		/*mtk_gpu_misc_met_nfy_power_state(0);*/
	}
	return PVRSRV_OK;
}

static PVRSRV_ERROR MTKSysPostPowerState(IMG_HANDLE hSysData, PVRSRV_SYS_POWER_STATE eNewPowerState,
				  PVRSRV_SYS_POWER_STATE eCurrentPowerState,
				  PVRSRV_POWER_FLAGS ePwrFlags)
{
	if ((eCurrentPowerState == PVRSRV_SYS_POWER_STATE_OFF) &&
	    (eNewPowerState == PVRSRV_SYS_POWER_STATE_ON)) {
		mtk_mfg_enable_gpu();
		/*mtk_gpu_misc_met_nfy_power_state(1);*/
	}
	return PVRSRV_OK;
}

#if defined(CONFIG_DEVFREQ_THERMAL)

/* TODO: Recauculate. */
#define MFG_POWER_DYNAMIC 484 /* mw */
#define MFG_POWER_REF_FREQ 8000000  /* hz / 100 */
#define MFG_POWER_REF_VOLT 9 /* mV / 100 */
#define MFG_POWER_LEAK  4

static unsigned long mtk_mfg_get_static_power(struct devfreq *df,
							unsigned long voltage)
{
	return MFG_POWER_LEAK;
}

static unsigned long mtk_mfg_get_dynamic_power(struct devfreq *df,
							unsigned long freq,
							unsigned long voltage)
{
	return MFG_POWER_DYNAMIC * (freq / MFG_POWER_REF_FREQ) *
		(voltage / MFG_POWER_REF_VOLT) * (voltage / MFG_POWER_REF_VOLT) /
		(100 * 10000);
}

static struct devfreq_cooling_power sPowerOps = {
	.get_static_power = mtk_mfg_get_static_power,
	.get_dynamic_power = mtk_mfg_get_dynamic_power,
};
#endif

static void MTKSetFreq(IMG_UINT32 ui32Freq)
{
	if (!mtk_gpu_is_volt_freq_locked())
		mtk_mfg_set_freq(ui32Freq, true);
}

static void MTKSetVolt(IMG_UINT32 ui32Volt)
{
	if (!mtk_gpu_is_volt_freq_locked())
		mtk_mfg_set_volt(ui32Volt);
}

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	PVRSRV_ERROR err = PVRSRV_OK;

	if (!mtk_mfg_is_ready()) {
		pr_info("mfg not ready - defer");
		return PVRSRV_ERROR_PROBE_DEFER;
	}

	gsPhysHeapFuncs.pfnCpuPAddrToDevPAddr = UMAPhysHeapCpuPAddrToDevPAddr;
	gsPhysHeapFuncs.pfnDevPAddrToCpuPAddr = UMAPhysHeapDevPAddrToCpuPAddr;

	gsPhysHeapConfig.pszPDumpMemspaceName = "SYSMEM";
	gsPhysHeapConfig.eType = PHYS_HEAP_TYPE_UMA;
	gsPhysHeapConfig.psMemFuncs = &gsPhysHeapFuncs;
	gsPhysHeapConfig.hPrivData = (IMG_HANDLE)&gsDevices[0];
	// [TIM] hack
	gsPhysHeapConfig.ui32UsageFlags = PHYS_HEAP_USAGE_GPU_LOCAL;

	gsDevices[0].pvOSDevice = pvOSDevice;
	gsDevices[0].pasPhysHeaps = &gsPhysHeapConfig;
	gsDevices[0].ui32PhysHeapCount = sizeof(gsPhysHeapConfig) / sizeof(PHYS_HEAP_CONFIG);

	/* Setup RGX specific timing data */
	gsRGXTimingInfo.ui32CoreClockSpeed = RGX_HW_CORE_CLOCK_SPEED;

#if MTK_PM_SUPPORT
	gsRGXTimingInfo.bEnableActivePM           = true;
	gsRGXTimingInfo.ui32ActivePMLatencyms     = SYS_RGX_ACTIVE_POWER_LATENCY_MS,
#else
	gsRGXTimingInfo.bEnableActivePM           = false;
#endif

	/*  define HW APM */
	gsRGXTimingInfo.bEnableRDPowIsland        = mtk_mfg_is_hwapm();

	/*
	 * Setup RGX specific data
	 */
	gsRGXData.psRGXTimingInfo = &gsRGXTimingInfo;

	/*
	 * Setup RGX device
	 */
	gsDevices[0].pszName	= "RGX";
	gsDevices[0].pszVersion = NULL;

	/* Device setup information */
#if defined(CONFIG_OF)
	/* MTK: using device tree */
	{
		struct resource *irq_res;
		struct resource *reg_res;
		const void *default_freq;

		gpsPVRLDMDev = to_platform_device((struct device *)pvOSDevice);
		irq_res = platform_get_resource(gpsPVRLDMDev, IORESOURCE_IRQ, 0);

		if (irq_res) {
			gsDevices[0].ui32IRQ				= irq_res->start;
		} else {
			PVR_DPF((PVR_DBG_ERROR, "irq_res = NULL!"));
err_out:
			return PVRSRV_ERROR_INIT_FAILURE;
		}

		reg_res = platform_get_resource(gpsPVRLDMDev, IORESOURCE_MEM, 0);

		if (reg_res) {
			gsDevices[0].sRegsCpuPBase.uiAddr	= reg_res->start;
			gsDevices[0].ui32RegsSize			= resource_size(reg_res);
		} else {
			PVR_DPF((PVR_DBG_ERROR, "reg_res = NULL!"));
			goto err_out;
		}

		default_freq = of_get_property(
				gpsPVRLDMDev->dev.of_node, "clock-frequency",
				NULL);
		if (default_freq)
			gsRGXTimingInfo.ui32CoreClockSpeed =
				be32_to_cpup(default_freq);

	}
#else
	gsDevices[0].sRegsCpuPBase.uiAddr   = SYS_MTK_RGX_REGS_SYS_PHYS_BASE;
	gsDevices[0].ui32RegsSize           = SYS_MTK_RGX_REGS_SIZE;
	gsDevices[0].ui32IRQ                = SYS_MTK_RGX_IRQ;
#endif

	/*  power management on  HW system */
#if MTK_PM_SUPPORT
	gsDevices[0].pfnPrePowerState       = MTKSysPrePowerState;
	gsDevices[0].pfnPostPowerState      = MTKSysPostPowerState;
#else
	gsDevices[0].pfnPrePowerState       = NULL;
	gsDevices[0].pfnPostPowerState      = NULL;
#endif

	/*  clock frequency  */
	gsDevices[0].pfnClockFreqGet        = NULL;
	gsDevices[0].hDevData               = &gsRGXData;
	gsDevices[0].eCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_NONE;

#if defined(SUPPORT_LINUX_DVFS)
	{
		int oppSize = ARRAY_SIZE(asOPPTable);
		int oppIdx;
		/* T:0x9 or 0xB; D:0x8 or 0xA */
		int segment = get_devinfo_with_index(18);

		switch (segment) {
		case 0x9:
		case 0xB:
			break;
		default:
			while (oppSize > 0
				&& asOPPTable[oppSize - 1].ui32Volt == 900000)
				oppSize--;
			break;
		}

		if (!mtk_mfg_can_set_volt(700000))
			for (oppIdx = 0; oppIdx < oppSize; oppIdx++)
				if (asOPPTable[oppIdx].ui32Volt == 700000)
					asOPPTable[oppIdx].ui32Volt = 800000;

		gsDevices[0].sDVFS.sDVFSDeviceCfg.pasOPPTable =	asOPPTable;
		gsDevices[0].sDVFS.sDVFSDeviceCfg.ui32OPPTableSize = oppSize;
	}
	/* Default freq as min for performance */
	gsDevices[0].sDVFS.sDVFSDeviceCfg.ui32MinFreq =
		gsRGXTimingInfo.ui32CoreClockSpeed;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.pfnSetFrequency = MTKSetFreq;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.pfnSetVoltage = MTKSetVolt;
	/* Use false after hooping ready */
	gsDevices[0].sDVFS.sDVFSDeviceCfg.bIdleReq = mtk_mfg_dvfs_idle();
	gsDevices[0].sDVFS.sDVFSDeviceCfg.ui32PollMs = 100;

	gsDevices[0].sDVFS.sDVFSGovernorCfg.ui32UpThreshold = 65;
	gsDevices[0].sDVFS.sDVFSGovernorCfg.ui32DownDifferential = 40;

#if defined(CONFIG_DEVFREQ_THERMAL)
	gsDevices[0].sDVFS.sDVFSDeviceCfg.psPowerOps = &sPowerOps;
#endif
#endif

	/* Setup other system specific stuff */
#if defined(SUPPORT_ION)
	IonInit(NULL);
#endif

	gsDevices[0].pvOSDevice = pvOSDevice;

	*ppsDevConfig = &gsDevices[0];

	mtk_gpu_misc_init(&gsDevices[0]);

	return err;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
#if defined(SUPPORT_ION)
	IonDeinit();
#endif
	psDevConfig->pvOSDevice = NULL;
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
				  IMG_UINT32 ui32IRQ,
				  const IMG_CHAR *pszName,
				  PFN_LISR pfnLISR,
				  void *pvData,
				  IMG_HANDLE *phLISRData)
{
	IMG_UINT32 ui32IRQFlags = SYS_IRQ_FLAG_TRIGGER_HIGH;

	PVR_UNREFERENCED_PARAMETER(hSysData);

#if defined(PVRSRV_GPUVIRT_MULTIDRV_MODEL)
	ui32IRQFlags |= SYS_IRQ_FLAG_SHARED;
#endif

	return OSInstallSystemLISR(phLISRData, ui32IRQ, pszName, pfnLISR, pvData,
							   ui32IRQFlags);
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	return OSUninstallSystemLISR(hLISRData);
}


PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevConfig,
			  DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf, void *pvDumpDebugFile)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_UNREFERENCED_PARAMETER(pfnDumpDebugPrintf);
	return PVRSRV_OK;
}




/******************************************************************************
 * End of file (sysconfig.c)
 ******************************************************************************/
