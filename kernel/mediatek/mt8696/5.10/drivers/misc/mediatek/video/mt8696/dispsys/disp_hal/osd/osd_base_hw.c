/*
 * Copyright (C) 2015-2016 MediaTek Inc.
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
#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/file.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include "disp_osd_if.h"
#include "disp_osd_log.h"
#include "osd_hw.h"

/*todo:change support to plane counter support*/
union OSD_BASE_UNION_T _rOsdBaseReg[OSD_FMT_MAX_NUM];
static union OSD_BASE_UNION_T *_prHwOsdBaseReg[OSD_FMT_MAX_NUM];
phys_addr_t _osd_fmt_base_reg_pa[OSD_FMT_MAX_NUM];

void _OSD_BASE_GET_REG_BASE(uintptr_t *base_reg,
	phys_addr_t *base_reg_pa)
{
	_prHwOsdBaseReg[OSD_FMT_1] = (union OSD_BASE_UNION_T *)base_reg[0];
	_prHwOsdBaseReg[OSD_FMT_2] = (union OSD_BASE_UNION_T *)base_reg[1];
	_osd_fmt_base_reg_pa[OSD_FMT_1] = base_reg_pa[0];
	_osd_fmt_base_reg_pa[OSD_FMT_2] = base_reg_pa[1];
	OSDDBG("2:hw base register =0x%p,0x%p", _prHwOsdBaseReg[OSD_FMT_1],
		   _prHwOsdBaseReg[OSD_FMT_2]);
}

void _OSD_AlwaysUpdateReg(unsigned int FMT_ID, BOOL fgEnable)
{
	UINT32 u4Val;

	if (fgEnable) {
		u4Val = IO_OSD_REG32((uintptr_t)_prHwOsdBaseReg[FMT_ID], 0);
		u4Val |= 0x02;
		IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[FMT_ID], 0, u4Val);
	} else {
		u4Val = IO_OSD_REG32((uintptr_t)_prHwOsdBaseReg[FMT_ID], 0);
		u4Val &= (~0x02);
		IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[FMT_ID], 0, u4Val);
	}
	IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[FMT_ID], 0x70, 0x40);
}

INT32 _OSD_BASE_SetForceUnupdate(unsigned int FMT_ID, UINT32 u4Value)
{
	return _OSD_BASE_SetAlwaysUpdate(FMT_ID, u4Value);
}

void _OSD_UpdateReg(unsigned int FMT_ID,
	bool fgEnable,
	struct cmdq_pkt *pkt)
{
	UINT32 u4Val;

	if (fgEnable) {
		u4Val = IO_OSD_REG32((uintptr_t)_prHwOsdBaseReg[FMT_ID], 0);
		u4Val |= 0x01;
		#if OSD_SUPPORT_GCE
			cmdq_pkt_write_value_addr(pkt,
				_osd_fmt_base_reg_pa[FMT_ID],
				u4Val,
				~0);
		#else
			IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[FMT_ID],
				0,
				u4Val);
		#endif
	} else {
		u4Val = IO_OSD_REG32((uintptr_t)_prHwOsdBaseReg[FMT_ID], 0);
		u4Val &= (~0x01);
		#if OSD_SUPPORT_GCE
			cmdq_pkt_write_value_addr(pkt,
				_osd_fmt_base_reg_pa[FMT_ID],
				u4Val,
				~0);
		#else
			IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[FMT_ID],
				0,
				u4Val);
		#endif
	}
}

void _Osd_PustReset_Plane(UINT32 u4Plane)
{
	UINT32 u4Value;

	u4Value = IO_OSD_REG32((uintptr_t)_prHwOsdBaseReg[u4Plane], 4);

	switch (u4Plane) {
	case OSD_PLANE_1:
		u4Value &= 0xFFFFFFFF;
		break;

	case OSD_PLANE_2:
		u4Value &= 0xFFFFFFFF;
		break;

	default:
		u4Value |= 0xFFFFFFFF;
		break;
	}

	IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[u4Plane], 4, u4Value);
}

void _Osd_ReleaseReset_Plane(UINT32 u4Plane)
{
	UINT32 u4Value;

	u4Value = IO_OSD_REG32((uintptr_t)_prHwOsdBaseReg[u4Plane], 4);
	u4Value &= ~OSD_RESET_PLANE_MASK;
	IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[u4Plane], 4, u4Value);
}

void _OSD_BASE_Reset(UINT32 u4Plane)
{
	UINT32 u4Val;

	u4Val = 0x03 << (u4Plane * 2 + 4);
	IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[u4Plane], 0x4, u4Val);
	IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[u4Plane], 0x4, 0);
}

INT32 _OSD_BASE_GetReg(unsigned int u4Plane, UINT32 *pOsdBaseReg)
{
	INT32 u4Idx = OSD_BASE_SKIP;

	if (pOsdBaseReg == NULL)
		return -(INT32)OSD_RET_INV_ARG;

	for (; u4Idx < OSD_BASE_REG_NUM; u4Idx++)
		pOsdBaseReg[u4Idx] = _rOsdBaseReg[u4Plane].au4Reg[u4Idx];

	return (INT32)OSD_RET_OK;
}

INT32 _OSD_BASE_SetReg(unsigned int u4Plane, const UINT32 *pOsdBaseReg)
{
	UINT32 u4Idx;

	if (pOsdBaseReg == NULL) {
		OSDDBG("_prHwOsdBaseReg[0]=0x%p\n", _prHwOsdBaseReg[u4Plane]);
		u4Idx = IO_OSD_REG32((uintptr_t)_prHwOsdBaseReg[u4Plane], 4);
#if !CONFIG_DRV_FAST_LOGO
		u4Idx |= OSD_RESET_PLANE_MASK;
		IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[u4Plane], 4, u4Idx);

		u4Idx &= (~OSD_RESET_PLANE_MASK);
		u4Idx |= 0xc00000c0;
		IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[u4Plane], 4, u4Idx);

		//IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[u4Plane], 0x38,
				   //0x80008000);
		//IO_OSD_WRITE32((uintptr_t)_prHwOsdBaseReg[u4Plane], 0x3c,
				  // 0x80008000);
#endif
		//_prHwOsdBaseReg[u4Plane]->rField.fg_source_sync_select = 1;
		//_prHwOsdBaseReg[u4Plane]->rField.fgAlwaysUpdate = 0;
		//_prHwOsdBaseReg[u4Plane]->rField.fgUpdate = 1;
		//_prHwOsdBaseReg[u4Plane]->rField.u4Value0ColorCh3 = 1;

		for (u4Idx = OSD_BASE_SKIP; u4Idx < OSD_BASE_REG_NUM; u4Idx++)
			_rOsdBaseReg[u4Plane].au4Reg[u4Idx] =
				_prHwOsdBaseReg[u4Plane]->au4Reg[u4Idx];

		//_rOsdBaseReg[u4Plane].au4Reg[1] &= (~OSD_RESET_PLANE_MASK);
	} else {
		for (u4Idx = OSD_BASE_SKIP; u4Idx < OSD_BASE_REG_NUM; u4Idx++)
			_rOsdBaseReg[u4Plane].au4Reg[u4Idx] =
				pOsdBaseReg[u4Idx];
	}
	return (INT32)OSD_RET_OK;
}

INT32 _OSD_BASE_UpdateHwReg(void)
{
	uint32_t i;

	for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++)
		_OSD_BASE_SetUpdate(i, true);
	return (INT32)OSD_RET_OK;
}

INT32 _OSD_BASE_Update(uint32_t plane, struct cmdq_pkt *pkt)
{
	UINT32 u4Update;
	UINT32 u4Idx = OSD_BASE_SKIP;

	_OSD_BASE_GetUpdate(plane, &u4Update);
	if (u4Update) {
		for (; u4Idx < OSD_BASE_COMMON_REG_NUM; u4Idx++) {
		#if OSD_SUPPORT_GCE
			cmdq_pkt_write_value_addr(pkt,
				_osd_fmt_base_reg_pa[plane]+4*u4Idx,
				_rOsdBaseReg[plane].au4Reg[u4Idx],
				~0);
		#else
			_prHwOsdBaseReg[plane]->au4Reg[u4Idx] =
				_rOsdBaseReg[plane].au4Reg[u4Idx];
		#endif
			OSD_PRINTF(OSD_WRITE_HW_LOG,
				   "FMT_WRITE(%d + 'h%x, 32'h%x)\n", plane,
				   (u4Idx * 4),
				   _rOsdBaseReg[plane].au4Reg[u4Idx]);
		}
		_OSD_BASE_SetUpdate(plane, false);
	}

	osd_base_get_start_update(plane, &u4Update);
	if (u4Update) {
	#if OSD_SUPPORT_GCE
		cmdq_pkt_write_value_addr(pkt,
				_osd_fmt_base_reg_pa[plane]+4*4,
				_rOsdBaseReg[plane].au4Reg[4],
				~0);
		cmdq_pkt_write_value_addr(pkt,
				_osd_fmt_base_reg_pa[plane]+4*5,
				_rOsdBaseReg[plane].au4Reg[5],
				~0);

		cmdq_pkt_write_value_addr(pkt,
				_osd_fmt_base_reg_pa[plane]+4*9,
				_rOsdBaseReg[plane].au4Reg[9],
				~0);

	#else
		_prHwOsdBaseReg[plane]->au4Reg[4] =
			_rOsdBaseReg[plane].au4Reg[4];
		_prHwOsdBaseReg[plane]->au4Reg[5] =
			_rOsdBaseReg[plane].au4Reg[5];
		_prHwOsdBaseReg[plane]->au4Reg[9] =
			_rOsdBaseReg[plane].au4Reg[9];
	#endif
		osd_base_set_start_update(plane, false);
	}

	osd_base_get_big_timing_update(plane, &u4Update);
	if (u4Update) {
	#if OSD_SUPPORT_GCE
		cmdq_pkt_write_value_addr(pkt,
				_osd_fmt_base_reg_pa[plane]+4*3,
				_rOsdBaseReg[plane].au4Reg[3],
				~0);
		cmdq_pkt_write_value_addr(pkt,
				_osd_fmt_base_reg_pa[plane]+4*52,
				_rOsdBaseReg[plane].au4Reg[52],
				~0);
	#else
		_prHwOsdBaseReg[plane]->au4Reg[3] =
			_rOsdBaseReg[plane].au4Reg[3];
		_prHwOsdBaseReg[plane]->au4Reg[52] =
			_rOsdBaseReg[plane].au4Reg[52];
	#endif
		osd_base_set_big_timing_update(plane, false);
	}

	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetUpdate(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgUpdate = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetUpdate(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgUpdate;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetPlaneUpdate(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgPlaneUpdate = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetPlaneUpdate(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgPlaneUpdate;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetSclUpdate(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgSclUpdate = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetSclUpdate(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgSclUpdate;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetPlaneReflip(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgPlaneReflip = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetPlaneReflip(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgPlaneReflip;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetScalerReCfg(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgScalerReCfg = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetScalerReCfg(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgScalerReCfg;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetAlwaysUpdate(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgAlwaysUpdate = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetAlwaysUpdate(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _prHwOsdBaseReg[u4Plane]->rField.fgAlwaysUpdate;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetResetMainPath(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgRstMainFmt = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetResetMainPath(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgRstMainFmt;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetResetOsd1(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgRstOsd1 = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetResetOsd1(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgRstOsd1;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetResetOsd2(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgRstOsd2 = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetResetOsd2(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgRstOsd2;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetIOMonSel(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4IOMonSel = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetIOMonSel(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4IOMonSel;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetAlphaSel(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4AlphaSel = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetAlphaSel(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4AlphaSel;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetSRamType(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4SRamType = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetSRamType(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4SRamType;
	return (INT32)OSD_RET_OK;
}

/* 08h OSD mode configuration register */
inline INT32 _OSD_BASE_SetHsEdge(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgHsEdge = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetHsEdge(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgHsEdge;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetVsEdge(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgVsEdge = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetVsEdge(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgVsEdge;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetFldPol(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgFldPol = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetFldPol(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgFldPol;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsdPrgs(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgOsdPrgs = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsdPrgs(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgOsdPrgs;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsdFrameEndEvent(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgFrameEndEvent = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsdFrameEndEvent(
	unsigned int u4Plane,
	UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgFrameEndEvent;
	return (INT32)OSD_RET_OK;
}
inline INT32 _OSD_BASE_SetOsdActiveEndEvent(
	unsigned int u4Plane,
	UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgActiveEndEvent = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsdActiveEndEvent(
	unsigned int u4Plane,
	UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgActiveEndEvent;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsd1Path(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgOsd1Aux = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsd1Path(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgOsd1Aux;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsd2Path(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgOsd2Aux = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsd2Path(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgOsd2Aux;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsd1Dotctl(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4Osd1Dotctl = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsd1Dotctl(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4Osd1Dotctl;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsd2Dotctl(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4Osd2Dotctl = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsd2Dotctl(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4Osd2Dotctl;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetAutoSwEn(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgAutoSwEn = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetAutoSwEn(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgAutoSwEn;
	return (INT32)OSD_RET_OK;
}

/* 0Ch Main FMT Vsync Timing Configuration Register */
inline INT32 _OSD_BASE_SetOvtMain(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4OvtMain = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOvtMain(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4OvtMain;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetVsWidthMain(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4VsWidthMain = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetVsWidthMain(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4VsWidthMain;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetHsWidthMain(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4HsWidthMain = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetHsWidthMain(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4HsWidthMain;
	return (INT32)OSD_RET_OK;
}
inline INT32 _OSD_BASE_SetBigTiming(unsigned int u4Plane, bool u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgBigTiming = u4Value;
	_rOsdBaseReg[u4Plane].rField.rg_screen_de_sel = u4Value;
	_rOsdBaseReg[u4Plane].rField.rg_de_cnt_sel = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetBigTiming(unsigned int u4Plane, bool *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgBigTiming;
	return (INT32)OSD_RET_OK;
}


/* 10h FMT H-Timing Configuration Register#1 */
inline INT32 _OSD_BASE_SetScrnHStartOsd(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4ScrnHStartOsd = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetScrnHStartOsd(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4ScrnHStartOsd;
	return (INT32)OSD_RET_OK;
}

/* 18h Main FMT V-Timing Configuration Register #1 */
inline INT32 _OSD_BASE_SetScrnVStartBotMain(unsigned int u4Plane,
						UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4ScrnVStartBotMain = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetScrnVStartBotMain(unsigned int u4Plane,
						UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4ScrnVStartBotMain;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetScrnVStartTopMain(unsigned int u4Plane,
						UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4ScrnVStartTopMain = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetScrnVStartTopMain(unsigned int u4Plane,
						UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4ScrnVStartTopMain;
	return (INT32)OSD_RET_OK;
}

/* 1Ch Main FMT Timing Configuration Register #2 */
inline INT32 _OSD_BASE_SetScrnVSizeMain(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4ScrnVSizeMain = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetScrnVSizeMain(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4ScrnVSizeMain;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetScrnHSizeMain(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4ScrnHSizeMain = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetScrnHSizeMain(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4ScrnHSizeMain;
	return (INT32)OSD_RET_OK;
}

/* 24h OSD2 Window Position Configuration Register	*/
inline INT32 _OSD_BASE_SetOsdVStart(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4OsdVStart = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsdVStart(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4OsdVStart;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsdHStart(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4OsdHStart = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsdHStart(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4OsdHStart;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetPvricValue0ColorCh3(
	unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4Value0ColorCh3 = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetPvricValue0ColorCh3(
	unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4Value0ColorCh3 = u4Value;
	return (INT32)OSD_RET_OK;
}

/* DWORD - 064 OSD_FMT_64 */
inline INT32 _OSD_BASE_SetOhtMain(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.u4OhtMain = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOhtMain(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.u4OhtMain;
	return (INT32)OSD_RET_OK;
}

/* DWORD - 088 OSD_FMT_70 */
inline INT32 _OSD_BASE_SetIntTGen(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgIntTGen = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetIntTGen(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgIntTGen;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsdShareSram(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgOsdShareSram = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsdShareSram(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.fgOsdShareSram;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetOsdChkSumEn(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.fgCheckSumEn = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsdChkSum(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _prHwOsdBaseReg[u4Plane]->rField.u4OsdCheckSum;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetOsdScChkSum(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _prHwOsdBaseReg[u4Plane]->rField.u4OsdScCheckSum;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetFrameEndEventValueV(
	unsigned int u4Plane, UINT32 u4Value)
{
	_prHwOsdBaseReg[u4Plane]->rField.rg_frame_end_v = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetFrameEndEventValueV
	(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _prHwOsdBaseReg[u4Plane]->rField.rg_frame_end_v;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_SetFrameEndEventValueH(
	unsigned int u4Plane, UINT32 u4Value)
{
	_prHwOsdBaseReg[u4Plane]->rField.rg_frame_end_h = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_BASE_GetFrameEndEventValueH
	(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _prHwOsdBaseReg[u4Plane]->rField.rg_frame_end_h;
	return (INT32)OSD_RET_OK;
}

inline void osd_plane_set_burst8_update(
	unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.osd_burst8_update = u4Value;
}

inline int osd_plane_get_burst8_update(
	unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.osd_burst8_update;
	return (INT32)OSD_RET_OK;
}

inline void osd_plane_set_pvric_update(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.osd_pvric_update = u4Value;
}

inline int osd_plane_get_pvric_update(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.osd_pvric_update;
	return (INT32)OSD_RET_OK;
}

inline void osd_base_set_big_timing_update(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.osd_big_timing_update = u4Value;
}

inline int osd_base_get_big_timing_update(unsigned int u4Plane,
	UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.osd_big_timing_update;
	return (INT32)OSD_RET_OK;
}

inline void osd_plane_set_faker_header_update(unsigned int u4Plane,
	UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.osd_enable_shadow_update = u4Value;
}

inline int osd_plane_get_faker_header_update(unsigned int u4Plane,
	UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.osd_enable_shadow_update;
	return (INT32)OSD_RET_OK;
}
inline void osd_base_set_start_update(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.osd_start_update = u4Value;
}

inline int osd_base_get_start_update(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.osd_start_update;
	return (INT32)OSD_RET_OK;
}

inline void osd_pmx_set_window_update(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.osd_window_update = u4Value;
}

inline int osd_pmx_get_window_update(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.osd_window_update;
	return (INT32)OSD_RET_OK;
}

inline void osd_plane_set_extend_update(unsigned int u4Plane, UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.osd_plane_update = u4Value;
}

inline int osd_plane_get_extend_update(unsigned int u4Plane, UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.osd_plane_update;
	return (INT32)OSD_RET_OK;
}

inline void osd_plane_set_sync_threshold_update(unsigned int u4Plane,
						UINT32 u4Value)
{
	_rOsdBaseReg[u4Plane].rField.osd_sync_threshold_update = u4Value;
}

inline int osd_plane_get_sync_threshold_update(unsigned int u4Plane,
						   UINT32 *pu4Value)
{
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdBaseReg[u4Plane].rField.osd_sync_threshold_update;
	return (INT32)OSD_RET_OK;
}
