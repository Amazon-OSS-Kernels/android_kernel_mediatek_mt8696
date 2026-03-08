/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Tiffany Lin <tiffany.lin@mediatek.com>
 *
 */

#ifndef _MTK_VCODEC_PM_H_
#define _MTK_VCODEC_PM_H_
#include "mtk_vcodec_util.h"
#define MTK_PLATFORM_STR        "platform:mt8195"
#define MTK_VDEC_RACING_INFO_OFFSET  0x100
#define DEC_MAX_UBE_INDEX         32
#define MTK_VDEC_RACING_INFO_SIZE (DEC_MAX_UBE_INDEX*4)

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
	struct clk      *vdecpll_ck;
	struct clk      *vencpll_d6;
	struct clk      *venc_sel;
	struct clk      *univpll1_d2;
	struct clk      *venc_lt_sel;
	struct clk      *img_resz;
	struct device   *larbvdec;
	struct device   *larbvenc;
	struct device   *larbvenclt;
	struct device   *dev;
	struct device_node      *chip_node;
	struct mtk_vcodec_dev   *mtkdev;
	/*soc clk*/
	struct clk *clk_MT_CG_SOC_LARB1;
	struct clk *clk_MT_CG_SOC_LAT;
	struct clk *clk_MT_CG_SOC_VDEC;
	/*core0 clk*/
	struct clk *clk_MT_CG_LARB1;
	struct clk *clk_MT_CG_LAT;
	struct clk *clk_MT_CG_VDEC;
	/*core1 clk*/
	struct clk *clk_MT_CG_CORE1_LARB1;
	struct clk *clk_MT_CG_CORE1_LAT;
	struct clk *clk_MT_CG_CORE1_VDEC;

    /* VENC core 0*/
	struct clk *clk_MT_CG_VENC0;

    /* VENC core 1*/
	struct clk *clk_MT_CG_VENC1;

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
	VDEC_CORE1_MISC,
	VDEC_LAT1_MISC,
	VDEC_SOC_GLOBAL,
	VDEC_LAT_WDMA,
	VDEC_LAT1_WDMA,
	VDEC_LAT_TOP,
	VDEC_UFO_ENC,
	NUM_MAX_VDEC_REG_BASE,
};

enum mtk_enc_dtsi_reg_idx {
	VENC_SYS,
	VENC_C1_SYS,
	NUM_MAX_VENC_REG_BASE
};

#endif /* _MTK_VCODEC_PM_H_ */
