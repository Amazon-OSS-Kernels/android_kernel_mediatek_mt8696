// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#ifndef DRM_CMDQ_DISABLE
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#else
#include "mtk-cmdq-ext.h"
#endif

#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_dump.h"
#include "mtk_rect.h"
#include "mtk_ethdr.h"
#include "mtk_drm_drv.h"

#define VDO1_CONFIG_SW0_RST_B 0x1D0
#define MT8195_DISP_MERGE_ENABLE	0x000
#define MT8195_DISP_MERGE_RESET		0x004
#define MT8195_DISP_MERGE_CFG_0		0x010
#define MT8195_DISP_MERGE_CFG_1		0x014
#define MT8195_DISP_MERGE_CFG_4		0x020
#define MT8195_DISP_MERGE_CFG_5		0x024
#define MT8195_DISP_MERGE_CFG_8		0x030
#define MT8195_DISP_MERGE_CFG_9		0x034
#define MT8195_DISP_MERGE_CFG_10	0x038
#define MT8195_DISP_MERGE_CFG_11	0x03C
#define MT8195_DISP_MERGE_CFG_12	0x040
#define MT8195_DISP_MERGE_CFG_13	0x044
#define MT8195_DISP_MERGE_CFG_14	0x048
#define MT8195_DISP_MERGE_CFG_15	0x04C
#define MT8195_DISP_MERGE_CFG_17	0x054
#define MT8195_DISP_MERGE_CFG_18	0x058
#define MT8195_DISP_MERGE_CFG_19	0x05C
#define MT8195_DISP_MERGE_CFG_20	0x060
#define MT8195_DISP_MERGE_CFG_21	0x064
#define MT8195_DISP_MERGE_CFG_22	0x068
#define MT8195_DISP_MERGE_CFG_23	0x06C
#define MT8195_DISP_MERGE_CFG_24	0x070
#define MT8195_DISP_MERGE_CFG_25	0x074
#define MT8195_DISP_MERGE_CFG_26	0x078
#define MT8195_DISP_MERGE_CFG_27	0x07C
#define MT8195_DISP_MERGE_CFG_28	0x080
#define MT8195_DISP_MERGE_CFG_29	0x084
#define MT8195_DISP_MERGE_CFG_36	0x0A0
	#define MT8195_DISP_MERGE_CFG_36_FLD_ULTRA_EN	REG_FLD(1, 0)
	#define MT8195_DISP_MERGE_CFG_36_FLD_PREULTRA_EN REG_FLD(1, 4)
	#define MT8195_DISP_MERGE_CFG_36_FLD_HALT_FOR_DVFS_EN REG_FLD(1, 8)
	#define MT8195_DISP_MERGE_CFG_36_VAL_ULTRA_EN(val)         \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_36_FLD_ULTRA_EN, (val))
	#define MT8195_DISP_MERGE_CFG_36_VAL_PREULTRA_EN(val)		  \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_36_FLD_PREULTRA_EN, (val))
	#define MT8195_DISP_MERGE_CFG_36_VAL_HALT_FOR_DVFS_EN(val)		  \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_36_FLD_HALT_FOR_DVFS_EN, (val))
#define MT8195_DISP_MERGE_CFG_37	0x0A4
	#define MT8195_DISP_MERGE_CFG_37_FLD_BUFFER_MODE REG_FLD(2, 0)
	#define MT8195_DISP_MERGE_CFG_37_VAL_BUFFER_MODE(val)		  \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_37_FLD_BUFFER_MODE, (val))
#define MT8195_DISP_MERGE_CFG_38	0x0A8
	#define MT8195_DISP_MERGE_CFG_38_FLD_VDE_BLOCK_ULTRA REG_FLD(1, 0)
	#define MT8195_DISP_MERGE_CFG_38_FLD_VALID_TH_BLOCK_ULTRA REG_FLD(1, 4)
	#define MT8195_DISP_MERGE_CFG_38_FLD_ULTRA_FIFO_VALID_TH REG_FLD(16, 16)
	#define MT8195_DISP_MERGE_CFG_38_VAL_VDE_BLOCK_ULTRA(val)		  \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_38_FLD_VDE_BLOCK_ULTRA, (val))
	#define MT8195_DISP_MERGE_CFG_38_VAL_VALID_TH_BLOCK_ULTRA(val)	\
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_38_FLD_VALID_TH_BLOCK_ULTRA, (val))
	#define MT8195_DISP_MERGE_CFG_38_VAL_ULTRA_FIFO_VALID_TH(val)	\
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_38_FLD_ULTRA_FIFO_VALID_TH, (val))
#define MT8195_DISP_MERGE_CFG_39	0x0AC
	#define MT8195_DISP_MERGE_CFG_39_FLD_NVDE_FORCE_PREULTRA REG_FLD(1, 8)
	#define MT8195_DISP_MERGE_CFG_39_FLD_NVALID_TH_FORCE_PREULTRA REG_FLD(1, 12)
	#define MT8195_DISP_MERGE_CFG_39_FLD_PREULTRA_FIFO_VALID_TH REG_FLD(16, 16)
	#define MT8195_DISP_MERGE_CFG_39_VAL_NVDE_FORCE_PREULTRA(val) \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_39_FLD_NVDE_FORCE_PREULTRA, (val))
	#define MT8195_DISP_MERGE_CFG_39_VAL_NVALID_TH_FORCE_PREULTRA(val)\
	REG_FLD_VAL(MT8195_DISP_MERGE_CFG_39_FLD_NVALID_TH_FORCE_PREULTRA, (val))
	#define MT8195_DISP_MERGE_CFG_39_VAL_PREULTRA_FIFO_VALID_TH(val)\
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_39_FLD_PREULTRA_FIFO_VALID_TH, (val))
#define MT8195_DISP_MERGE_CFG_40	0x0B0
	#define MT8195_DISP_MERGE_CFG_40_FLD_ULTRA_TH_LOW REG_FLD(16, 0)
	#define MT8195_DISP_MERGE_CFG_40_FLD_ULTRA_TH_HIGH REG_FLD(16, 16)
	#define MT8195_DISP_MERGE_CFG_40_VAL_ULTRA_TH_LOW(val)	\
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_40_FLD_ULTRA_TH_LOW, (val))
	#define MT8195_DISP_MERGE_CFG_40_VAL_ULTRA_TH_HIGH(val) \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_40_FLD_ULTRA_TH_HIGH, (val))
#define MT8195_DISP_MERGE_CFG_41	0x0B4
	#define MT8195_DISP_MERGE_CFG_41_FLD_PREULTRA_TH_LOW REG_FLD(16, 0)
	#define MT8195_DISP_MERGE_CFG_41_FLD_PREULTRA_TH_HIGH REG_FLD(16, 16)
	#define MT8195_DISP_MERGE_CFG_41_VAL_PREULTRA_TH_LOW(val) \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_41_FLD_PREULTRA_TH_LOW, (val))
	#define MT8195_DISP_MERGE_CFG_41_VAL_PREULTRA_TH_HIGH(val) \
		REG_FLD_VAL(MT8195_DISP_MERGE_CFG_41_FLD_PREULTRA_TH_HIGH, (val))

#define DISP_REG_MERGE_CTRL (0x000)
	#define FLD_MERGE_EN REG_FLD_MSB_LSB(0, 0)
	#define FLD_MERGE_RST REG_FLD_MSB_LSB(4, 4)
	#define FLD_MERGE_LR_SWAP REG_FLD_MSB_LSB(8, 8)
	#define FLD_MERGE_DCM_DIS REG_FLD_MSB_LSB(12, 12)
#define DISP_REG_MERGE_WIDTH (0x004)
	#define FLD_IN_WIDHT_L REG_FLD_MSB_LSB(15, 0)
	#define FLD_IN_WIDHT_R REG_FLD_MSB_LSB(31, 16)
#define DISP_REG_MERGE_HEIGHT (0x008)
	#define FLD_IN_HEIGHT REG_FLD_MSB_LSB(15, 0)

#define DISP_REG_MERGE_SHADOW_CRTL (0x00C)
#define DISP_REG_MERGE_DGB0 (0x010)
	#define FLD_PIXEL_CNT REG_FLD_MSB_LSB(15, 0)
	#define FLD_MERGE_STATE REG_FLD_MSB_LSB(17, 16)
#define DISP_REG_MERGE_DGB1 (0x014)
	#define FLD_LINE_CNT REG_FLD_MSB_LSB(15, 0)

struct mtk_disp_merge_data {
	bool need_golden_setting;
	enum mtk_ddp_comp_id gs_comp_id;
};

struct mtk_merge_config_struct {
	unsigned short width_right;
	unsigned short width_left;
	unsigned int height;
	unsigned int fmt;
	unsigned int mode;
	unsigned int swap;
};


struct mtk_disp_merge {
	struct mtk_ddp_comp ddp_comp;
	struct drm_crtc *crtc;
	struct clk *async_clk;
	const struct mtk_disp_merge_data *data;
};

static struct mtk_ddp_comp *_Merge5Comp;

static inline struct mtk_disp_merge *comp_to_merge(struct mtk_ddp_comp *comp)
{
	return container_of(comp, struct mtk_disp_merge, ddp_comp);
}

static void mtk_merge_start(struct mtk_ddp_comp *comp, struct cmdq_pkt *handle)
{
	int ret;

	DDPDBG("%s(comp-%d): regs = %lx, regs_pa = %lx\n",
		__func__, comp->id, comp->regs, comp->regs_pa);

	ret = pm_runtime_get_sync(comp->dev);
	if (ret < 0)
		DRM_ERROR("%s(comp-%d): failed to enable power domain %s: %d\n",
			__func__, comp->id, comp->dev->init_name, ret);
	cmdq_pkt_write(handle, comp->cmdq_base,
		       comp->regs_pa + DISP_REG_MERGE_CTRL, 0x1, ~0);
}

static void mtk_merge_stop(struct mtk_ddp_comp *comp, struct cmdq_pkt *handle)
{
	int ret;

	DDPMSG("%s\n", __func__);

	cmdq_pkt_write(handle, comp->cmdq_base,
		       comp->regs_pa + DISP_REG_MERGE_CTRL, 0x0, ~0);

	ret = pm_runtime_put_sync(comp->dev);
	if (ret < 0)
		DRM_ERROR("Failed to disable power domain: %d\n", ret);
}

static int mtk_merge_check_params(struct mtk_merge_config_struct *merge_config)
{
	if (!merge_config->height || !merge_config->width_left
		|| !merge_config->width_right) {
		DDPPR_ERR("%s:merge input width l(%u) w(%u) h(%u)\n",
			  __func__, merge_config->width_left,
			  merge_config->width_right, merge_config->height);
		return -EINVAL;
	}
	DDPMSG("%s:merge input width l(%u) r(%u) height(%u)\n",
			  __func__, merge_config->width_left,
			  merge_config->width_right, merge_config->height);
	return 0;
}

static void mtk_merge_YUV422_12Bit(struct mtk_ddp_comp *comp,
				   struct mtk_ddp_config *cfg,
				   struct cmdq_pkt *handle)
{
	struct mtk_merge_config_struct merge_config;

	merge_config.mode = 0;
	merge_config.width_left = cfg->w / 2;
	merge_config.width_right = cfg->w / 2;
	merge_config.height = cfg->h;

	mtk_merge_check_params(&merge_config);

	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_12,
		merge_config.mode, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_13,
		0x11110011, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_14,
		0x11100001, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_15,
		0x03010022, ~0);

	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_0,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_1,
		(merge_config.height << 16 | merge_config.width_right),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_4,
		(merge_config.height << 16 | cfg->w),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_24,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_25,
		(merge_config.height << 16 | merge_config.width_right),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_26,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_27,
		(merge_config.height << 16 | merge_config.width_right),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_ENABLE,
		0x1, 0x1);
}

static int mtk_merge_golden_setting(struct mtk_ddp_comp *comp,
				  struct cmdq_pkt *handle)
{
	int ultra_en = 1;
	int preultra_en = 1;
	int halt_for_dvfs_en = 0;
	int buffer_mode = 3;
	int vde_block_ultra = 0;
	int valid_th_block_ultra = 0;
	int ultra_fifo_valid_th = 0;
	int nvde_force_preultra = 0;
	int nvalid_th_force_preultra = 0;
	int preultra_fifo_valid_th = 0;
	int ultra_th_low = 0xE10;
	int ultra_th_high = 0x12C0;
	int preultra_th_low = 0x12C0;
	int preultra_th_high = 0x1518;

	DDPINFO("%s\n", __func__);

	cmdq_pkt_write(handle, comp->cmdq_base,
	comp->regs_pa + MT8195_DISP_MERGE_CFG_36,
	MT8195_DISP_MERGE_CFG_36_VAL_ULTRA_EN(ultra_en)|
		MT8195_DISP_MERGE_CFG_36_VAL_PREULTRA_EN(preultra_en)|
		MT8195_DISP_MERGE_CFG_36_VAL_HALT_FOR_DVFS_EN(halt_for_dvfs_en),
	REG_FLD_MASK(MT8195_DISP_MERGE_CFG_36_FLD_ULTRA_EN)|
		REG_FLD_MASK(MT8195_DISP_MERGE_CFG_36_FLD_PREULTRA_EN)|
		REG_FLD_MASK(MT8195_DISP_MERGE_CFG_36_FLD_HALT_FOR_DVFS_EN));

	cmdq_pkt_write(handle, comp->cmdq_base,
	comp->regs_pa + MT8195_DISP_MERGE_CFG_37,
	MT8195_DISP_MERGE_CFG_37_VAL_BUFFER_MODE(buffer_mode),
	REG_FLD_MASK(MT8195_DISP_MERGE_CFG_37_FLD_BUFFER_MODE));

	cmdq_pkt_write(handle, comp->cmdq_base,
	comp->regs_pa + MT8195_DISP_MERGE_CFG_38,
	MT8195_DISP_MERGE_CFG_38_VAL_VDE_BLOCK_ULTRA(vde_block_ultra)|
		MT8195_DISP_MERGE_CFG_38_VAL_VALID_TH_BLOCK_ULTRA(valid_th_block_ultra)|
		MT8195_DISP_MERGE_CFG_38_VAL_ULTRA_FIFO_VALID_TH(ultra_fifo_valid_th),
	REG_FLD_MASK(MT8195_DISP_MERGE_CFG_38_FLD_VDE_BLOCK_ULTRA)|
		REG_FLD_MASK(MT8195_DISP_MERGE_CFG_38_FLD_VALID_TH_BLOCK_ULTRA)|
		REG_FLD_MASK(MT8195_DISP_MERGE_CFG_38_FLD_ULTRA_FIFO_VALID_TH));


	cmdq_pkt_write(handle, comp->cmdq_base,
	comp->regs_pa + MT8195_DISP_MERGE_CFG_39,
	MT8195_DISP_MERGE_CFG_39_VAL_NVDE_FORCE_PREULTRA(nvde_force_preultra)|
		MT8195_DISP_MERGE_CFG_39_VAL_NVALID_TH_FORCE_PREULTRA
			(nvalid_th_force_preultra)|
		MT8195_DISP_MERGE_CFG_39_VAL_PREULTRA_FIFO_VALID_TH
			(preultra_fifo_valid_th),
	REG_FLD_MASK(MT8195_DISP_MERGE_CFG_39_FLD_NVDE_FORCE_PREULTRA)|
		REG_FLD_MASK(MT8195_DISP_MERGE_CFG_39_FLD_NVALID_TH_FORCE_PREULTRA)|
		REG_FLD_MASK(MT8195_DISP_MERGE_CFG_39_FLD_PREULTRA_FIFO_VALID_TH));

	cmdq_pkt_write(handle, comp->cmdq_base,
	comp->regs_pa + MT8195_DISP_MERGE_CFG_40,
	MT8195_DISP_MERGE_CFG_40_VAL_ULTRA_TH_LOW(ultra_th_low)|
		MT8195_DISP_MERGE_CFG_40_VAL_ULTRA_TH_HIGH(ultra_th_high),
	REG_FLD_MASK(MT8195_DISP_MERGE_CFG_40_FLD_ULTRA_TH_LOW)|
		REG_FLD_MASK(MT8195_DISP_MERGE_CFG_40_FLD_ULTRA_TH_HIGH));

	cmdq_pkt_write(handle, comp->cmdq_base,
	comp->regs_pa + MT8195_DISP_MERGE_CFG_41,
	MT8195_DISP_MERGE_CFG_41_VAL_PREULTRA_TH_LOW(preultra_th_low)|
		MT8195_DISP_MERGE_CFG_41_VAL_PREULTRA_TH_HIGH(preultra_th_high),
	REG_FLD_MASK(MT8195_DISP_MERGE_CFG_41_FLD_PREULTRA_TH_LOW)|
		REG_FLD_MASK(MT8195_DISP_MERGE_CFG_41_FLD_PREULTRA_TH_HIGH));

	return 0;
}

void mtk_merge_layer_config(struct mtk_plane_state *state,
					  struct cmdq_pkt *handle)
{
	enum EN_ETHDR_HDR_TYPE eOutputHdrType = 0;
	unsigned int swap;

	mtk_ethdr_get_output_hdr_type(&eOutputHdrType);
	swap = (eOutputHdrType == EN_ETHDR_HDR_TYPE_DV) ? 0x08:0x10;

	cmdq_pkt_write(handle, _Merge5Comp->cmdq_base,
			_Merge5Comp->regs_pa + MT8195_DISP_MERGE_CFG_10,
			swap, ~0);
}


static void mtk_merge_config(struct mtk_ddp_comp *comp,
				struct mtk_ddp_config *cfg,
		       struct cmdq_pkt *handle)
{
	struct mtk_merge_config_struct merge_config;
	enum EN_ETHDR_HDR_TYPE eOutputHdrType = 0;
	struct mtk_disp_merge *priv = dev_get_drvdata(comp->dev);

	DDPMSG("%s(comp-%d): regs = %lx, regs_pa = %lx\n",
		__func__, comp->id, comp->regs, comp->regs_pa);

	/*golden setting*/
	if (priv->data != NULL) {
		if (priv->data->need_golden_setting &&
			priv->data->gs_comp_id == comp->id)
			mtk_merge_golden_setting(comp, handle);
	}

	if ((comp->fb) && (comp->fb->format))
		merge_config.fmt = comp->fb->format->format;
	else
		merge_config.fmt = DRM_FORMAT_ARGB8888;

	switch (comp->id) {
	case DDP_COMPONENT_MERGE0:
		merge_config.mode = 5;
		merge_config.width_left = cfg->w;
		merge_config.width_right = cfg->w;
		merge_config.height = cfg->h;
		merge_config.swap = 0;
		break;
	case DDP_COMPONENT_MERGE1:
	case DDP_COMPONENT_MERGE2:
	case DDP_COMPONENT_MERGE3:
	case DDP_COMPONENT_MERGE4:
		if (merge_config.fmt == DRM_FORMAT_ARGB8888 ||
		    merge_config.fmt == DRM_FORMAT_YVU444) {
			merge_config.mode = 18;
			merge_config.width_left = cfg->w / 2;
			merge_config.width_right = cfg->w / 2;
			merge_config.height = cfg->h;
		} else if (merge_config.fmt == DRM_FORMAT_RGB888) {
			mtk_merge_YUV422_12Bit(comp, cfg, handle);
			return;
		}
		if (merge_config.fmt == DRM_FORMAT_YVU444)
			merge_config.swap = 4;
		else
			merge_config.swap = 0;
		break;
	case DDP_COMPONENT_MERGE5:
		mtk_ethdr_get_output_hdr_type(&eOutputHdrType);
		merge_config.mode = 8;
		merge_config.width_left = cfg->w;
		merge_config.width_right = cfg->w;
		merge_config.height = cfg->h;
		merge_config.swap = (eOutputHdrType == EN_ETHDR_HDR_TYPE_DV) ? 0x08:0x10;
		break;
	default:
		DDPDBG("No find component merge %d\n", comp->id);
		return;
	}

	mtk_merge_check_params(&merge_config);

	switch (merge_config.mode) {
	case 5:
	case 8:
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_0,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_4,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_24,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_25,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_10,
		merge_config.swap, 0x1f);

	break;
	case 18:
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_0,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_1,
		(merge_config.height << 16 | merge_config.width_right),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_4,
		(merge_config.height << 16 | cfg->w),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_24,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_25,
		(merge_config.height << 16 | merge_config.width_right),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_26,
		(merge_config.height << 16 | merge_config.width_left),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_27,
		(merge_config.height << 16 | merge_config.width_right),
		~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_10,
		merge_config.swap, 0x1f);
	break;
	default:
	break;
	}
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_CFG_12,
		merge_config.mode, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + MT8195_DISP_MERGE_ENABLE,
		0x1, 0x1);
#ifdef IFZERO
	merge_config.height = cfg->h;
	merge_config.width_left = cfg->w / 2;
	merge_config.width_right = cfg->w / 2;

	mtk_merge_check_params(&merge_config);

	DDPMSG("%s\n", __func__);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + DISP_REG_MERGE_WIDTH,
		merge_config.width_left | (merge_config.width_right << 16),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		       comp->regs_pa + DISP_REG_MERGE_HEIGHT,
		       merge_config.height, ~0);
#endif
}

void mtk_merge_dump(struct mtk_ddp_comp *comp)
{
	void __iomem *baddr = comp->regs;
	int i = 0;

	DDPDUMP("== DISP %s REGS ==\n", mtk_dump_comp_str(comp));
	for (i = 0; i < 1; i++) {
		DDPDUMP("0x%03X: 0x%08x 0x%08x 0x%08x 0x%08x\n", i * 0x10,
			readl(baddr + i * 0x10), readl(baddr + i * 0x10 + 0x4),
			readl(baddr + i * 0x10 + 0x8),
			readl(baddr + i * 0x10 + 0xC));
	}
	DDPDUMP("0x010: 0x%08x 0x%08x;\n", readl(baddr + 0x10),
		readl(baddr + 0x14));
}

int mtk_merge_analysis(struct mtk_ddp_comp *comp)
{
#define LEN 100
	void __iomem *baddr = comp->regs;
	u32 width = 0;
	u32 height = 0;
	u32 enable = 0;
	u32 dbg0 = 0;
	u32 dbg1 = 0;

	int ret;
	char msg[LEN];

	enable = readl(baddr + DISP_REG_MERGE_CTRL);
	width = readl(baddr + DISP_REG_MERGE_WIDTH);
	height = readl(baddr + DISP_REG_MERGE_HEIGHT);
	dbg0 = readl(baddr + DISP_REG_MERGE_DGB0);
	dbg1 = readl(baddr + DISP_REG_MERGE_DGB1);

	DDPDUMP("== DISP %s ANALYSIS ==\n", mtk_dump_comp_str(comp));

	ret = snprintf(msg, LEN,
		"en:%d,swap:%d,dcm_dis:%d,width_L:%d,width_R:%d,h:%d,pix_cnt:%d,line_cnt:%d\n",
		REG_FLD_VAL_GET(FLD_MERGE_EN, enable),
		REG_FLD_VAL_GET(FLD_MERGE_LR_SWAP, enable),
		REG_FLD_VAL_GET(FLD_MERGE_DCM_DIS, enable),
		REG_FLD_VAL_GET(FLD_IN_WIDHT_L, width),
		REG_FLD_VAL_GET(FLD_IN_WIDHT_R, width),
		REG_FLD_VAL_GET(FLD_IN_HEIGHT, height),
		REG_FLD_VAL_GET(FLD_PIXEL_CNT, dbg0),
		REG_FLD_VAL_GET(FLD_MERGE_STATE, dbg0),
		REG_FLD_VAL_GET(FLD_LINE_CNT, dbg1));

	if (ret >= 0)
		DDPDUMP("%s", msg);

	return 0;
}

static void mtk_merge_prepare(struct mtk_ddp_comp *comp)
{
	struct mtk_disp_merge *priv = dev_get_drvdata(comp->dev);
	int ret;

	DDPINFO("%s\n", __func__);
	mtk_ddp_comp_clk_prepare(comp);

	if (priv->async_clk != NULL) {
		ret = clk_prepare_enable(priv->async_clk);
		if (ret)
			DDPDBG("async clk prepare enable failed:%s\n",
				mtk_dump_comp_str(comp));
	}
}

static void mtk_merge_unprepare(struct mtk_ddp_comp *comp)
{
	struct mtk_disp_merge *priv = dev_get_drvdata(comp->dev);

	DDPINFO("%s\n", __func__);
	if (priv->async_clk != NULL)
		clk_disable_unprepare(priv->async_clk);
	mtk_ddp_comp_clk_unprepare(comp);
}

static const struct mtk_ddp_comp_funcs mtk_disp_merge_funcs = {
	.start = mtk_merge_start,
	.stop = mtk_merge_stop,
	.config = mtk_merge_config,
	.prepare = mtk_merge_prepare,
	.unprepare = mtk_merge_unprepare,
};

static int mtk_disp_merge_bind(struct device *dev, struct device *master,
			     void *data)
{
	struct mtk_disp_merge *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	//struct mtk_drm_private *private = drm_dev->dev_private;
	int ret;

	pr_info("%s\n", __func__);
	ret = mtk_ddp_comp_register(drm_dev, &priv->ddp_comp);
	if (ret < 0) {
		dev_err(dev, "Failed to register component %s: %d\n",
			dev->of_node->full_name, ret);
		return ret;
	}
	pr_info("%s end\n", __func__);
	return 0;
}

static void mtk_disp_merge_unbind(struct device *dev, struct device *master,
				void *data)
{
	struct mtk_disp_merge *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;

	mtk_ddp_comp_unregister(drm_dev, &priv->ddp_comp);
}

static const struct component_ops mtk_disp_merge_component_ops = {
	.bind = mtk_disp_merge_bind, .unbind = mtk_disp_merge_unbind,
};

static int mtk_disp_merge_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_disp_merge *priv;
	enum mtk_ddp_comp_id comp_id;
	int ret;

	pr_info("%s+\n", __func__);
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	comp_id = mtk_ddp_comp_get_id(dev->of_node, MTK_DISP_MERGE);
	if ((int)comp_id < 0) {
		dev_err(dev, "Failed to identify by alias: %d\n", comp_id);
		return comp_id;
	}

	ret = mtk_ddp_comp_init(dev, dev->of_node, &priv->ddp_comp, comp_id,
				&mtk_disp_merge_funcs);
	if (ret) {
		dev_err(dev, "Failed to initialize component: %d\n", ret);
		return ret;
	}

	if (comp_id == DDP_COMPONENT_MERGE5)
		_Merge5Comp = &priv->ddp_comp;

	priv->async_clk = of_clk_get(dev->of_node, 1);
	if (IS_ERR(priv->async_clk)) {
		ret = PTR_ERR(priv->async_clk);
		DDPINFO("No merge async clock: %d\n", ret);
		priv->async_clk = NULL;
	}

	priv->data = of_device_get_match_data(dev);

	platform_set_drvdata(pdev, priv);

	mtk_ddp_comp_pm_enable(&priv->ddp_comp);

	ret = component_add(dev, &mtk_disp_merge_component_ops);
	if (ret != 0) {
		dev_err(dev, "Failed to add component: %d\n", ret);
		mtk_ddp_comp_pm_disable(&priv->ddp_comp);
	}

	pr_info("%s-\n", __func__);

	return ret;
}

static int mtk_disp_merge_remove(struct platform_device *pdev)
{
	struct mtk_disp_merge *priv = dev_get_drvdata(&pdev->dev);

	component_del(&pdev->dev, &mtk_disp_merge_component_ops);
	mtk_ddp_comp_pm_disable(&priv->ddp_comp);

	return 0;
}

static const struct mtk_disp_merge_data mt8195_merge_driver_data = {
	.need_golden_setting = true,
	.gs_comp_id = DDP_COMPONENT_MERGE5,
};

static const struct of_device_id mtk_disp_merge_driver_dt_match[] = {
	{.compatible = "mediatek,mt6779-disp-merge", },
	{.compatible = "mediatek,mt6885-disp-merge", },
	{.compatible = "mediatek,mt6983-disp-merge", },
	{.compatible = "mediatek,mt8195-disp-merge",
	 .data = &mt8195_merge_driver_data},
	{.compatible = "mediatek,mt8188-disp-merge",
	 .data = &mt8195_merge_driver_data}, // same as 8195
	{},
};
MODULE_DEVICE_TABLE(of, mtk_disp_merge_driver_dt_match);

struct platform_driver mtk_disp_merge_driver = {
	.probe = mtk_disp_merge_probe,
	.remove = mtk_disp_merge_remove,
	.driver = {
		.name = "mediatek-disp-merge",
		.owner = THIS_MODULE,
		.of_match_table = mtk_disp_merge_driver_dt_match,
	},
};
