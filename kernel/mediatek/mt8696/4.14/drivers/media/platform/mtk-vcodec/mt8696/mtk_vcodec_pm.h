/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Tiffany Lin <tiffany.lin@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MTK_VCODEC_PM_H_
#define _MTK_VCODEC_PM_H_

#include "ion_drv.h"
//#include "mach/mt_iommu.h"

extern struct ion_device *g_ion_device;

#define MTK_PLATFORM_STR        "platform:mt8696"
#define MTK_VDEC_RACING_INFO_OFFSET  0x100
#define MTK_VDEC_RACING_INFO_SIZE 68

/**
 * struct mtk_vcodec_pm - Power management data structure
 */
struct mtk_vcodec_pm {
	struct clk      *vdec_bus_clk_src;
	struct clk      *vencpll;

	struct clk      *vcodecpll;
	struct clk      *univpll_d2;
	struct clk      *clk_cci400_sel;
	struct clk      *vdecpll;
	struct clk      *vdec_sel;
	struct clk      *vencpll_d2;
	struct clk      *venc_sel;
	struct clk      *univpll1_d2;
	struct clk      *venc_lt_sel;
	struct clk      *img_resz;
	struct device   *larbvdec[MTK_VDEC_HW_NUM];
	struct device   *larbvenc;
	struct device   *larbvenclt;
	struct device   *dev;
	struct device   *dev_soc;
	struct device_node      *chip_node;
	struct mtk_vcodec_dev   *mtkdev;

	struct clk *MT_VDECEN;               /* VDEC core clk*/
	struct clk *MT_VDEC_L1EN;            /* VDEC core larb*/
	struct clk *MT_VDSOC_VSEN;           /* VDEC soc clk*/
	struct clk *MT_VDSOC_L1_SOCEN;       /* VDEC soc larb*/
	struct clk *MT_VDSOC_LSEN;           /* VDEC lat clk*/
	struct clk *MT_TOPCK_SLOW_SELEN;     /* VDEC slow clk*/

	struct clk *MT_VENC_SMI;//CLK_VENC_SMI_CON;          /* VENC core 0*/
	struct clk *MT_VENC_CON;//CLK_VENC_CON;          /* VENC core 0*/
	//struct clk *clk_MT_SCP_SYS_VEN;      /* SCP_SYS_VEN */

	atomic_t dec_active_cnt;
	__u32 vdec_racing_info[MTK_VDEC_RACING_INFO_SIZE];
	struct mutex dec_racing_info_mutex;
};

enum mtk_dec_dtsi_reg_idx {
	VDEC_SYS,
	VDEC_VLD,
	VDEC_MISC,
	VDEC_LAT_MISC,
	VDEC_RACING_CTRL,
	NUM_MAX_VDEC_REG_BASE,
};

enum mtk_enc_dtsi_reg_idx {
	VENC_SYS,
	NUM_MAX_VENC_REG_BASE
};

#endif /* _MTK_VCODEC_PM_H_ */
