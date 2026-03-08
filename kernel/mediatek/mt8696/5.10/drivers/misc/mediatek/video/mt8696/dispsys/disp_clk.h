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

#ifndef __DISP_CLK_H__
#define __DISP_CLK_H__

#include <linux/device.h>
#include <linux/types.h>

enum DISP_HW_CLK {
	/*MMsys TOP PM*/
	DISP_CLK_TVDPLL, /* 0 */
	DISP_CLK_TVDPLL_D2,
	DISP_CLK_OSDPLL,
	DISP_CLK_OSDPLL_D2,
	DISP_CLK_OSDPLL_D4,
	DISP_CLK_HD_SEL,
	DISP_CLK_SD_SEL,
	DISP_CLK_OSD_SEL,
	DISP_CLK_VDO3_SEL,
	DISP_CLK_VDO4_SEL,
	DISP_CLK_26M,
	DISP_CLK_VDOUT_SYS,
	DISP_CLK_DGI,
	DISP_CLK_FAKE_ENG,
	DISP_CLK_FMT_TG,
	DISP_CLK_OSD_FHD,
	DISP_CLK_OSD_FBDC,
	DISP_CLK_OSD_FHD_SIM2MIU,
	DISP_CLK_FHD_GFX_FE_FIFO,
	DISP_CLK_FHD_HDR_GFX_FE,
	DISP_CLK_HDR_ADL,
	DISP_CLK_DISP_MIX,
	DISP_CLK_HDR_VDO_BE,
	DISP_CLK_HDR_BE_FIFO,
	DISP_CLK_HDMI_TX_PIXEL,
	DISP_CLK_HDMI_TX_PIXEL_D2,

	/*MMsys common  PM*/
	DISP_CLK_HDMI_METADATA,
	DISP_CLK_W2D,
	DISP_CLK_P2I,
	DISP_CLK_SDPPF,
	DISP_CLK_VDOIN,
	DISP_CLK_VM,

	/*OSD UHD PM*/
	DISP_CLK_OSD_UHD,
	DISP_CLK_UHD_GFX_FE_FIFO,
	DISP_CLK_UHD_HDR_GFX_FE,

	/*DISP MVDO PM*/
	DISP_CLK_VDO3,
	DISP_CLK_DISPFMT3,
	DISP_CLK_M_VDO_FE_FIFO,
	DISP_CLK_M_FILM_GRAIN,
	DISP_CLK_M_HDR_VDO_FE,
	DISP_CLK_M_VDO_FE_ADL,
	DISP_CLK_R2R,

	/*DISP SVDO PM*/
	DISP_CLK_VDO4,
	DISP_CLK_DISPFMT4,
	DISP_CLK_S_VDO_FE_FIFO,
	DISP_CLK_S_FILM_GRAIN,
	DISP_CLK_S_HDR_VDO_FE,

	DISP_CLK_NUM,
};

enum DISP_PM_TYPE {
	DISP_PM_MMSYS_TOP,
	DISP_PM_MMSYS_COM,
	DISP_PM_OSD_UHD,
	DISP_PM_DISP_MAIN,
	DISP_PM_DISP_SUB,
	DISP_PM_UNKNOWN,
};

struct DISP_PM_INFO {
	enum DISP_PM_TYPE type;
	int pm_clk_cnt;
	struct device *pm_dev;
};

enum DISP_SMI_LARB_ID {
	DISP_SMI_LARB0,
	DISP_SMI_LARB4,
	DISP_SMI_LARB5,
	DISP_SMI_LARB6,
	DISP_SMI_LARB_NUM,
};

int disp_get_clk(struct device *dev);
int disp_clock_set_pm_dev(struct device *dev, enum DISP_PM_TYPE type);
int disp_clock_enable(enum DISP_HW_CLK hw_clk, bool enable);
int disp_clock_dump(enum DISP_HW_CLK hw_clk);
int disp_clock_set_pll(enum DISP_HW_CLK hw_clk, unsigned int rate);
int disp_clock_select_pll(enum DISP_HW_CLK hw_clk, enum DISP_HW_CLK sel_clk);
bool disp_clock_register_success(void);
int disp_clock_set_larb_dev(struct device *dev, int larb_id);
int disp_clock_smi_larb_en(enum DISP_SMI_LARB_ID larb_id, bool en);

#endif
