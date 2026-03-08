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
#include <linux/types.h>

#include "disp_osd_log.h"
#include "disp_type.h"
#include "osd_hw.h"

union OSD_SC_UNION_T _rOsdScalerReg[OSD_SCALER_MAX_NUM];
static union OSD_SC_UNION_T *_prHwOsdScalerReg[OSD_SCALER_MAX_NUM];
phys_addr_t _osd_scl_base_reg_pa[OSD_SCALER_MAX_NUM];
void _OSD_SC_GET_REG_BASE(
	uintptr_t *osd_scl_reg_base,
	phys_addr_t *osd_scl_base_reg_pa)
{
	_prHwOsdScalerReg[OSD_SCALER_1] =
		(union OSD_SC_UNION_T *)osd_scl_reg_base[0];
	_prHwOsdScalerReg[OSD_SCALER_2] =
		(union OSD_SC_UNION_T *)osd_scl_reg_base[1];

	_osd_scl_base_reg_pa[OSD_SCALER_1] = osd_scl_base_reg_pa[0];
	_osd_scl_base_reg_pa[OSD_SCALER_2] = osd_scl_base_reg_pa[1];
}

INT32 _OSD_SC_GetReg(UINT32 u4Scaler, UINT32 *pOsdScalerReg)
{
	INT32 u4Idx = 0;

	OSD_VERIFY_SCALER(u4Scaler);

	if (pOsdScalerReg == NULL)
		return -(INT32)OSD_RET_INV_ARG;

	for (; u4Idx < OSD_SC_REG_NUM; u4Idx++)
		pOsdScalerReg[u4Idx] = _rOsdScalerReg[u4Scaler].au4Reg[u4Idx];

	return (INT32)OSD_RET_OK;
}

INT32 _OSD_SC_SetReg(UINT32 u4Scaler, const UINT32 *pOsdScalerReg)
{
	INT32 u4Idx = 0;

	OSD_VERIFY_SCALER(u4Scaler);

	if (pOsdScalerReg == NULL)
		for (; u4Idx < OSD_SC_REG_NUM; u4Idx++) {
			_rOsdScalerReg[u4Scaler].au4Reg[u4Idx] =
				_prHwOsdScalerReg[u4Scaler]->au4Reg[u4Idx];
		}
	else
		for (; u4Idx < OSD_SC_REG_NUM; u4Idx++)
			_rOsdScalerReg[u4Scaler].au4Reg[u4Idx] =
				pOsdScalerReg[u4Idx];

	return (INT32)OSD_RET_OK;
}

INT32 _OSD_SC_UpdateHwReg(UINT32 u4Scaler, struct cmdq_pkt *pkt)
{
	UINT32 u4Idx = 0;
	OSD_VERIFY_SCALER(u4Scaler);
	for (; u4Idx < OSD_SC_REG_NUM; u4Idx++) {
	#if OSD_SUPPORT_GCE
		cmdq_pkt_write_value_addr(pkt,
				_osd_scl_base_reg_pa[u4Scaler]+u4Idx*4,
				_rOsdScalerReg[u4Scaler].au4Reg[u4Idx],
				~0);
	#else
		_prHwOsdScalerReg[u4Scaler]->au4Reg[u4Idx] =
			_rOsdScalerReg[u4Scaler].au4Reg[u4Idx];
	#endif
		OSD_PRINTF(OSD_WRITE_HW_LOG, "SC_WRITE(%d + 'h%x, 32'h%x)\n",
			   u4Scaler, (u4Idx * 4),
			   _rOsdScalerReg[u4Scaler].au4Reg[u4Idx]);
	}
	return (INT32)OSD_RET_OK;
}

INT32 _OSD_SC_Update(UINT32 u4Scaler, struct cmdq_pkt *pkt)
{
	_OSD_SC_UpdateHwReg(u4Scaler, pkt);
	return (INT32)OSD_RET_OK;
}

INT32 _OSD_SC_SetUpdateStatus(UINT32 u4Scaler, UINT32 u4Update)
{
	UINT32 u4PlaneStatus = 0;

	_OSD_BASE_GetSclUpdate(u4Scaler, &u4PlaneStatus);
	u4PlaneStatus = u4Update ? (u4PlaneStatus | (1 << u4Scaler))
				 : (u4PlaneStatus & ~(1 << u4Scaler));
	_OSD_BASE_SetSclUpdate(u4Scaler, u4PlaneStatus);
	return (INT32)OSD_RET_OK;
}

INT32 _OSD_SC_GetUpdateStatus(UINT32 u4Scaler)
{
	UINT32 u4PlaneStatus;

	_OSD_BASE_GetSclUpdate(u4Scaler, &u4PlaneStatus);
	return ((u4PlaneStatus & (1 << u4Scaler)) >> u4Scaler);
}

inline INT32 _OSD_SC_SetVuscAlphaEdgeRcvr(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgVuscAlphaEdgeRcvr = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVuscAlphaEdgeRcvr(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgVuscAlphaEdgeRcvr;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVuscAlphaEdgeElt(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgVuscAlphaEdgeElt = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVuscAlphaEdgeElt(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgVuscAlphaEdgeElt;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVdscAlphaEdgeRcvr(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgVdscAlphaEdgeRcvr = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVdscAlphaEdgeRcvr(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgVdscAlphaEdgeRcvr;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVdscAlphaEdgeElt(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgVdscAlphaEdgeElt = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVdscAlphaEdgeElt(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgVdscAlphaEdgeElt;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHuscAlphaEdgeRcvr(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgHuscAlphaEdgeRcvr = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHuscAlphaEdgeRcvr(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgHuscAlphaEdgeRcvr;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHuscAlphaEdgeElt(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgHuscAlphaEdgeElt = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHuscAlphaEdgeElt(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgHuscAlphaEdgeElt;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHdscAlphaEdgeRcvr(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgHdscAlphaEdgeRcvr = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHdscAlphaEdgeRcvr(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgHdscAlphaEdgeRcvr;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHdscAlphaEdgeElt(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgHdscAlphaEdgeElt = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHdscAlphaEdgeElt(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgHdscAlphaEdgeElt;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVuscEn(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgVuscEn = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVuscEn(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgVuscEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVdscEn(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgVdscEn = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVdscEn(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgVdscEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHuscEn(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgHuscEn = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHuscEn(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgHuscEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHdscEn(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgHdscEn = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHdscEn(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgHdscEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetScLpfEn(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgScLpfEn = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetScLpfEn(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgScLpfEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetScEn(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.fgScEn = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetScEn(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.fgScEn;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetSrcVSize(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4SrcVSize = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetSrcVSize(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4SrcVSize;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetSrcHSize(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4SrcHSize = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetSrcHSize(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4SrcHSize;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetDstVSize(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4DstVSize = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetDstVSize(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4DstVSize;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetDstHSize(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4DstHSize = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetDstHSize(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4DstHSize;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVscHSize(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4VscHSize = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVscHSize(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4VscHSize;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHdscStep(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4HdscStep = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHdscStep(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4HdscStep;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHdscOfst(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4HdscOfst = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHdscOfst(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4HdscOfst;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHuscStep(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4HuscStep = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHuscStep(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4HuscStep;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetHuscOfst(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4HuscOfst = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetHuscOfst(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4HuscOfst;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVscOfstTop(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4VscOfstTop = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVscOfstTop(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4VscOfstTop;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVscOfstBot(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4VscOfstBot = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVscOfstBot(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4VscOfstBot;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetVscStep(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4VscStep = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetVscStep(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4VscStep;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetScLpfC3(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4ScLpfC3 = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetScLpfC3(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4ScLpfC3;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetScLpfC4(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4ScLpfC4 = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetScLpfC4(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4ScLpfC4;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_SetScLpfC5(UINT32 u4Scaler, UINT32 u4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	_rOsdScalerReg[u4Scaler].rField.u4ScLpfC5 = u4Value;
	return (INT32)OSD_RET_OK;
}

inline INT32 _OSD_SC_GetScLpfC5(UINT32 u4Scaler, UINT32 *pu4Value)
{
	OSD_VERIFY_SCALER(u4Scaler);
	if (pu4Value == NULL)
		return -(INT32)OSD_RET_INV_ARG;
	*pu4Value = _rOsdScalerReg[u4Scaler].rField.u4ScLpfC5;
	return (INT32)OSD_RET_OK;
}
