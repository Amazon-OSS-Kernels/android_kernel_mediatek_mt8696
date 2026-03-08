/*
 * Copyright (C) 2017 MediaTek Inc.
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
#define LOG_TAG "osd_region"

/* #include <mach/mt85xx.h> */
/* #include <mach/cache_operation.h> */
/* #include <mach/mt_typedefs.h> */

#include <linux/types.h>

#include "disp_osd_if.h"
#include "osd_hw.h"
#include "osd_sw.h"
#include <linux/io.h>
#include <linux/slab.h>

#include "disp_osd_log.h"
#include <linux/dma-mapping.h>
#include <linux/memory.h>

static union OSD_RGN_UNION_T
	*_prOsdRegionReg1[OSD_RGN_REG_NUM][OSD_RGN_CLUSTER_NUM];
static VOID *_prOsdRegionReg1Free[OSD_RGN_REG_NUM];

#if 1

/* AMYFP_ADD for test */
#define OSD_REGION_FLUSH(X)

#else
#define OSD_REGION_FLUSH(X) HalFlushInvalidateDCache()
#endif

int _osd_region_init(void)
{
	unsigned int index = 0;
	unsigned int size = 0;
#if OSD_IOMMU_SUPPORT
	dma_addr_t mva_address = 0;
#endif
	struct device *dev;
	unsigned int i;

	size = sizeof(union OSD_RGN_UNION_T) * OSD_RGN_CLUSTER_NUM;
	/*todo: get memory through mmu*/
	dev = disp_hw_mgr_get_dev();
	for (index = 0; index < OSD_RGN_REG_NUM; index++) {
#if OSD_IOMMU_SUPPORT
		_prOsdRegionReg1Free[index] =
			dma_alloc_coherent(dev, size, &mva_address, GFP_KERNEL);
#else
		_prOsdRegionReg1Free[index] = kmalloc(size, GFP_KERNEL);
#endif
		if (_prOsdRegionReg1Free[index] == NULL) {
			OSD_LOG_E("osd_region_init kmalloc failed\n");
			goto err;
		}
		memset(_prOsdRegionReg1Free[index], 0, size);
		/*todo: first region is the whole pic region as before*/
		_prOsdRegionReg1[index][0] =
			(union OSD_RGN_UNION_T *)_prOsdRegionReg1Free[index];
		OSDDBG("[Osd] %s _prOsdRegionReg1Free = 0x%p size 0x%x\n",
		       __func__, _prOsdRegionReg1Free[index], size);

		for (i = 1; i < OSD_RGN_CLUSTER_NUM; i++) {
			_prOsdRegionReg1[index][i] =
				(union OSD_RGN_UNION_T
					 *)(_prOsdRegionReg1Free[index] +
					    i * sizeof(union OSD_RGN_UNION_T));

			OSDDBG(
				"[Osd] %s _prOsdRegionReg1Free[%d][%d] = 0x%p\n",
				__func__, index, i, _prOsdRegionReg1[index][i]);
		}

		_rAllRgnNode[index].pu4_vAddr =
			((unsigned long)_prOsdRegionReg1Free[index]);
#if OSD_IOMMU_SUPPORT
		_rAllRgnNode[index].pu4_pAddr = mva_address;
#else
		_rAllRgnNode[index].pu4_pAddr =
			virt_to_phys(((void *)_rAllRgnNode[index].pu4_vAddr));
#endif

		_rAllRgnNode[index].eOsdRgnState = OSD_RGN_STATE_IDLE;
		OSDDBG("[Osd]osd_region_init vAddr=0x%lx,pAddr bus=0x%x\n",
		       _rAllRgnNode[index].pu4_vAddr,
		       _rAllRgnNode[index].pu4_pAddr);
	}
	return (INT32)OSD_RET_OK;

err:
	for (index = 0; index < OSD_RGN_REG_NUM; index++) {
		if (_prOsdRegionReg1Free[index] != NULL) {
#if OSD_IOMMU_SUPPORT
			dma_free_coherent(dev, size,
					  _prOsdRegionReg1Free[index],
					  _rAllRgnNode[index].pu4_pAddr);
#else
			kfree(_prOsdRegionReg1Free[index]);
#endif
			_prOsdRegionReg1Free[index] = NULL;
		}
	}
	return (INT32)(-OSD_RET_UNINIT);
}

void _osd_region_uninit(void)
{
	unsigned int index;
	unsigned int size = 0;
	struct device *dev;

	size = sizeof(union OSD_RGN_UNION_T) * OSD_RGN_CLUSTER_NUM;
	dev = disp_hw_mgr_get_dev();
	for (index = 0; index < OSD_RGN_REG_NUM; index++) {
		if (_prOsdRegionReg1Free[index] != NULL) {
#if OSD_IOMMU_SUPPORT
			dma_free_coherent(dev, size,
					  _prOsdRegionReg1Free[index],
					  _rAllRgnNode[index].pu4_pAddr);
#else
			kfree(_prOsdRegionReg1Free[index]);
#endif
			_prOsdRegionReg1Free[index] = NULL;
		}
	}
	OSD_LOG_I("osd_region_uninit\n");
}

INT32 _osd_region_reset(UINT32 u4Region)
{
	unsigned int size = 0;

	OSD_VERIFY_REGION(u4Region);
	if (_prOsdRegionReg1Free[u4Region] == NULL) {
		OSD_LOG_E("osd_region_init kmalloc failed\n");
		return (INT32)-OSD_RET_UNINIT;
	}

	/*todo: reset size change*/
	size = sizeof(union OSD_RGN_UNION_T) * OSD_RGN_CLUSTER_NUM;
	memset(_prOsdRegionReg1Free[u4Region],
		0, sizeof(union OSD_RGN_UNION_T) * 1);

	/*time for memset?*/

	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetFbdcEnable(
	UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.fgFbdcEn = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetFbdcEnable(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.fgFbdcEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetPixelFormat(
	UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4PixelFormat = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetPixelFormat(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4PixelFormat;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetNextRegion(UINT32 u4Region, uint32_t index,
				    UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4NextOsdAddr =
		(u4Value == 0) ? 0 : (u4Value) >> 4;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetNextRegion(UINT32 u4Region, uint32_t index,
				    UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = (UINT32)VIRTUAL(prRgn[index]->rField.u4NextOsdAddr << 4);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetFifoEx(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.fgFifoEx = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetFifoEx(UINT32 u4Region, uint32_t index,
				UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.fgFifoEx;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetNextEnable(UINT32 u4Region, uint32_t index,
				    UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.fgNextOsdEn = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetNextEnable(UINT32 u4Region, uint32_t index,
				    UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.fgNextOsdEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetAlpha(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4MixWeight = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetAlpha(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4MixWeight;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetHClip(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4HClip = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetHClip(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4HClip;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetVClip(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4VClip = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetVClip(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4VClip;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetLineSize(UINT32 u4Region, uint32_t index,
				  UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4LineSize = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetLineSize(UINT32 u4Region, uint32_t index,
				  UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4LineSize;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetVbSel(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4VbSel = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetVbSel(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4VbSel;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetUgSel(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4UgSel = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetUgSel(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4UgSel;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetYrSel(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4YrSel = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetYrSel(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4YrSel;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetASel(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4AlphaSel = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetASel(UINT32 u4Region, uint32_t index, UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4AlphaSel;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetInputWidth(UINT32 u4Region, uint32_t index,
				    UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4Ihw = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetInputWidth(UINT32 u4Region, uint32_t index,
				    UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4Ihw;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetInputHeight(UINT32 u4Region, uint32_t index,
				     UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4Ivw = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetInputHeight(UINT32 u4Region, uint32_t index,
				     UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4Ivw;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetHStep(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4HStep = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetHStep(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4HStep;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetVStep(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4VStep = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetVStep(UINT32 u4Region, uint32_t index,
			       UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4VStep;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetOutputHeight(UINT32 u4Region, uint32_t index,
				      UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4Ovw = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetOutputHeight(UINT32 u4Region, uint32_t index,
				      UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4Ovw;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetOutputPosY(UINT32 u4Region, uint32_t index,
				    UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4Ovs = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetOutputPosY(UINT32 u4Region, uint32_t index,
				    UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4Ovs;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetOutputWidth(UINT32 u4Region, uint32_t index,
				     UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4Ohw = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetOutputWidth(UINT32 u4Region, uint32_t index,
				     UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4Ohw;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetOutputPosX(UINT32 u4Region, uint32_t index,
				    UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4Ohs = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetOutputPosX(UINT32 u4Region, uint32_t index,
				    UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4Ohs;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetDataAddrHI(UINT32 u4Region, uint32_t index,
				    UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4DataAddrHI = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetDataAddrHI(UINT32 u4Region, uint32_t index,
				    UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4DataAddrHI;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetColorKeyEnable(UINT32 u4Region, uint32_t index,
					UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.fgColorKeyEn = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetColorKeyEnable(UINT32 u4Region, uint32_t index,
					UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.fgColorKeyEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetMixSel(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4MixSel = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetMixSel(UINT32 u4Region, uint32_t index,
				UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4MixSel;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetFrameMode(UINT32 u4Region, uint32_t index,
				   UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.fgAcsFrame = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetFrameMode(UINT32 u4Region, uint32_t index,
				   UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.fgAcsFrame;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetAutoMode(UINT32 u4Region, uint32_t index,
				  UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.fgAcsAuto = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetAutoMode(UINT32 u4Region, uint32_t index,
				  UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.fgAcsAuto;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetTopField(UINT32 u4Region, uint32_t index,
				  UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.fgAcsTop = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetTopField(UINT32 u4Region, uint32_t index,
				  UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.fgAcsTop;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetBlendMode(UINT32 u4Region, uint32_t index,
				   UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.u4MixSel = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetBlendMode(UINT32 u4Region, uint32_t index,
				   UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.u4MixSel;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_SetSelectByteEn(UINT32 u4Region, uint32_t index,
				      UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	prRgn[index]->rField.fgSelectByteEn = u4Value;
	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_RGN_GetSelectByteEn(UINT32 u4Region, uint32_t index,
				      UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn = NULL;

	OSD_VERIFY_REGION(u4Region);
	prRgn = _prOsdRegionReg1[u4Region];
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = prRgn[index]->rField.fgSelectByteEn;
	return (INT32)OSD_RET_OK;
}

/* -----------------------------------------------------------------------------
 */
/** Brief
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------------
 */
INT32 _OSD_RGN_SetColorMode(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn;

	OSD_VERIFY_REGION(u4Region);

	if ((u4Value == 0) || (u4Value == 1) || (u4Value == 2) ||
	    (u4Value == 8) || (u4Value == 9) || (u4Value == 10))
		IGNORE_RET(_OSD_RGN_SetFifoEx(u4Region, index, 0));
	else
		IGNORE_RET(_OSD_RGN_SetFifoEx(u4Region, index, 1));

	prRgn = _prOsdRegionReg1[u4Region];

	prRgn[index]->rField.u4ColorMode = u4Value;

	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

/* -----------------------------------------------------------------------------
 */
/** Brief
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------------
 */
INT32 _OSD_RGN_GetColorMode(UINT32 u4Region, uint32_t index, UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn;

	OSD_VERIFY_REGION(u4Region);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;

	prRgn = _prOsdRegionReg1[u4Region];
	*pu4Value = prRgn[index]->rField.u4ColorMode;
	return (INT32)OSD_RET_OK;
}

/* -----------------------------------------------------------------------------
 */
/** Brief
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------------
 */
INT32 _OSD_RGN_SetColorKey(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn;
	UINT32 u4ColorMode;

	OSD_VERIFY_REGION(u4Region);

	prRgn = _prOsdRegionReg1[u4Region];
	u4ColorMode = prRgn[index]->rField.u4ColorMode;
	if (((u4ColorMode >= (UINT32)OSD_CM_CBYCRY422_DIRECT16) &&
	     (u4ColorMode <= (UINT32)OSD_CM_AYCBCR8888_DIRECT32)) ||
	    ((u4ColorMode >= (UINT32)OSD_CM_RGB565_DIRECT16) &&
	     (u4ColorMode <= (UINT32)OSD_CM_ARGB8888_DIRECT32))) {
		/* _prOsdRegionReg[u4Region].rField.u4PaletteAddr
		 * = u4Value >>8;
		 */
		prRgn[index]->rField.u4PaletteAddr = u4Value >> 8;
	}
	/* _prOsdRegionReg[u4Region].rField.u4ColorKey = u4Value & 0xff; */
	prRgn[index]->rField.u4ColorKey = u4Value & 0xff;

	OSD_REGION_FLUSH(u4Region);
	return (INT32)OSD_RET_OK;
}

/* -----------------------------------------------------------------------------
 */
/** Brief
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------------
 */
INT32 _OSD_RGN_GetColorKey(UINT32 u4Region, uint32_t index, UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn;

	OSD_VERIFY_REGION(u4Region);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;

	prRgn = _prOsdRegionReg1[u4Region];
	/* *pu4Value = (_prOsdRegionReg[u4Region].rField.u4PaletteAddr << 8) |
	 */
	/* _prOsdRegionReg[u4Region].rField.u4ColorKey; */
	*pu4Value = (prRgn[index]->rField.u4PaletteAddr << 8) |
		    prRgn[index]->rField.u4ColorKey;
	return (INT32)OSD_RET_OK;
}

/* -----------------------------------------------------------------------------
 */
/** Brief
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------------
 */
INT32 _OSD_RGN_SetDataAddr(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn;

	OSD_VERIFY_REGION(u4Region);

	prRgn = _prOsdRegionReg1[u4Region];

	prRgn[index]->rField.u4DataAddr = (u4Value ? (u4Value) : 0) >> 4;
	prRgn[index]->rField.u4DataAddrMD = (u4Value ? (u4Value) : 0) >> 28;
	prRgn[index]->rField.u4DataAddrHI = (u4Value ? (u4Value) : 0) >> 30;
	OSD_REGION_FLUSH(u4Region);

	return (INT32)OSD_RET_OK;
}

/* -----------------------------------------------------------------------------
 */
/** Brief
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------------
 */
INT32 _OSD_RGN_GetDataAddr(UINT32 u4Region, uint32_t index, UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn;

	OSD_VERIFY_REGION(u4Region);

	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;

	prRgn = _prOsdRegionReg1[u4Region];

	*pu4Value = (prRgn[index]->rField.u4DataAddr << 4);
	*pu4Value |= (prRgn[index]->rField.u4DataAddrMD << 28);
	/* MT8580 DATA_ADDR[31:30] not used */
	*pu4Value |= 0xC0000000;
	/* *pu4Value |= (prRgn[index]->rField.u4DataAddrHI << 30); */
	return (INT32)OSD_RET_OK;
}

/* -----------------------------------------------------------------------------
 */
/** Brief :
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------------
 */
INT32 _OSD_RGN_SetPaletteAddr(UINT32 u4Region, uint32_t index, UINT32 u4Value)
{
	union OSD_RGN_UNION_T **prRgn;

	OSD_VERIFY_REGION(u4Region);

	prRgn = _prOsdRegionReg1[u4Region];

	/* prRgn[index]->rField.u4PaletteAddr   = (u4Value ? PHYSICAL(u4Value) :
	 * 0) >> 4;
	 */
	/* prRgn[index]->rField.u4PaletteAddrHI = (u4Value ? PHYSICAL(u4Value) :
	 * 0) >> 30;
	 */
	prRgn[index]->rField.u4PaletteAddr = (u4Value ? (u4Value) : 0) >> 4;
	/* prRgn[index]->rField.u4PaletteAddrHI = (u4Value ? (u4Value) : 0) >>
	 * 30;
	 */
	/* MT8580 if config the [31:30]bit the memory will be point to channel
	 * 2, so the high bit must be 00b
	 */
	prRgn[index]->rField.u4PaletteAddrHI = 0;
	return (INT32)OSD_RET_OK;
}

/* -----------------------------------------------------------------------------
 */
/** Brief
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------------
 */
INT32 _OSD_RGN_GetPaletteAddr(UINT32 u4Region, uint32_t index, UINT32 *pu4Value)
{
	union OSD_RGN_UNION_T **prRgn;

	OSD_VERIFY_REGION(u4Region);

	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;

	prRgn = _prOsdRegionReg1[u4Region];

	*pu4Value = (prRgn[index]->rField.u4PaletteAddr << 4);
	*pu4Value |= (prRgn[index]->rField.u4PaletteAddrHI << 30);

	return (INT32)OSD_RET_OK;
}

/* ----------------------------------------------------------------------
 */
/** Brief
 *  @param
 *  @return
 */
/* -----------------------------------------------------------------------
 */

INT32 _OSD_RGN_GetAddress(UINT32 u4Region, uint32_t index, UINT32 *pu4Addr)
{
	union OSD_RGN_UNION_T *prRgn;
	ulong region_va;
	size_t size = 0;

	OSD_VERIFY_REGION(u4Region);

	prRgn = _prOsdRegionReg1[u4Region]
				[index]; /* OSD_RGN_GetAdd(u4Region); */

	/*need a region flush*/
	size = (size_t)sizeof(union OSD_RGN_UNION_T);

	/**pu4Addr = (uintptr_t) &prRgn[u4Region]; */
	region_va = (ulong)prRgn;

	/*todo: rgn node change to multi-region*/
	if ((region_va & 0xf) == 0)
		*pu4Addr = _rAllRgnNode[u4Region].pu4_pAddr +
			   index * sizeof(union OSD_RGN_UNION_T);
	else
		OSD_LOG_E("invalid region addr\n");
	/*todo: return issue*/
	return (INT32)OSD_RET_OK;
}

union OSD_RGN_UNION_T *OSD_RGN_GetAdd(UINT32 u4Region)
{
	/* OSD_LOG_I("[Osd]osd_region_init pu4Addr =
	 * %x\n",&_prOsdRegionReg1[u4Region]);
	 */
	return _prOsdRegionReg1[u4Region][0];
}
