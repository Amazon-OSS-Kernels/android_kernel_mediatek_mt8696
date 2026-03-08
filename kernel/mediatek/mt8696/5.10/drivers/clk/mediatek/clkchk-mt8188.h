/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Chun-Jie Chen <chun-jie.chen@mediatek.com>
 */

#ifndef __DRV_CLKDBG_MT8188_H
#define __DRV_CLKDBG_MT8188_H

enum chk_sys_id {
	spm,
	topckgen,
	infracfg_ao,
	apmixedsys,
	audsys_src,
	audsys,
	pericfg_ao,
	imp_iic_wrap_c,
	imp_iic_wrap_w,
	imp_iic_wrap_en,
	mfgcfg,
	vppsys0,
	wpesys,
	wpesys_vpp0,
	vppsys1,
	imgsys,
	imgsys1_dip_top,
	imgsys1_dip_nr,
	imgsys_wpe1,
	ipesys,
	imgsys_wpe2,
	imgsys_wpe3,
	camsys,
	camsys_rawa,
	camsys_yuva,
	camsys_rawb,
	camsys_yuvb,
	ccusys,
	vdecsys_soc,
	vdecsys,
	vencsys,
	vdosys0,
	vdosys1,
	chk_sys_num,
};

#endif	/* __DRV_CLKDBG_MT8188_H */
