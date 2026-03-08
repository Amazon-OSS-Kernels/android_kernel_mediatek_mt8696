// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/iommu.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>

#include "mtk_drm_drv.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_mdp_reg_rdma.h"
#include "mtk_mdp_rdma.h"
#include "cmdq-sec.h"

#define IRQ_LEVEL_IDLE		(0)
#define IRQ_LEVEL_ALL		(1)

#define IRQ_INT_EN_ALL	\
	(REG_FLD_MASK(MDP_RDMA_INTERRUPT_ENABLE_FLD_UNDERRUN_INT_EN) \
	| REG_FLD_MASK(MDP_RDMA_INTERRUPT_ENABLE_FLD_REG_UPDATE_INT_EN) \
	| REG_FLD_MASK(MDP_RDMA_INTERRUPT_ENABLE_FLD_FRAME_COMPLETE_INT_EN))

#define RDMA_INFO(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define RDMA_DEBUG(fmt, ...)	pr_debug(fmt, ##__VA_ARGS__)

void mtk_mdp_write(void __iomem *base, unsigned int value,
		   unsigned int offset, struct cmdq_pkt *handle,
		   void __iomem *cmdq_base, resource_size_t regs_pa)
{
#ifdef CONFIG_MTK_DISPLAY_CMDQ
	cmdq_pkt_write(handle, cmdq_base, regs_pa + offset, value, ~0);
#else
	writel(value, base + offset);
#endif
}

void mtk_mdp_write_relaxed(void __iomem *base, unsigned int value,
			   unsigned int offset, struct cmdq_pkt *handle,
			   void __iomem *cmdq_base, resource_size_t regs_pa)
{
#ifdef CONFIG_MTK_DISPLAY_CMDQ
	cmdq_pkt_write(handle, cmdq_base, regs_pa + offset, value, ~0);
#else
	writel_relaxed(value, base + offset);
#endif
}

void mtk_mdp_write_mask(void __iomem *base, unsigned int value,
			unsigned int offset, unsigned int mask,
			struct cmdq_pkt *handle,
			void __iomem *cmdq_base,
			resource_size_t regs_pa)
{
#ifdef CONFIG_MTK_DISPLAY_CMDQ
	cmdq_pkt_write(handle, cmdq_base, regs_pa + offset, value, mask);
#else

	unsigned int tmp = readl(base + offset);

	tmp = (tmp & ~mask) | (value & mask);
	writel(tmp, base + offset);
#endif
}

void mtk_mdp_write_mask_cpu(void __iomem *base, unsigned int value,
			    unsigned int offset, unsigned int mask)
{
	unsigned int tmp = readl(base + offset);

	tmp = (tmp & ~mask) | (value & mask);
	writel(tmp, base + offset);
}

struct mtk_mdp_rdma_data {
	unsigned int id;
	void __iomem *base;
	unsigned int fifo_size;
	bool support_shadow;
	unsigned int underflow_cnt;
};

/**
 * struct mtk_mdp_rdma - MDP_RDMA driver structure
 * @data - private data
 */
struct mtk_mdp_rdma {
	struct mtk_mdp_rdma_data *data;
};

static unsigned int rdma_get_y_pitch(unsigned int fmt, unsigned int width)
{
	switch (fmt) {
	default:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
		return 2 * width;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 3 * width;
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
		return 4 * width;
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_YUYV:
		return 2 * width;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return 1 * width;
	case DRM_FORMAT_P010:
		return 2 * width;
	}
}

static unsigned int rdma_get_uv_pitch(unsigned int fmt, unsigned int width)
{
	switch (fmt) {
	default:
		return 0;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return 1 * width;
	case DRM_FORMAT_P010:
		return 2 * width;
	}
}

static unsigned int rdma_get_block_w(unsigned int mode)
{
	switch (mode) {
	default:
		return 0;
	case RDMA_BLOCK_8x8:
	case RDMA_BLOCK_8x16:
	case RDMA_BLOCK_8x32:
		return 8;
	case RDMA_BLOCK_16x8:
	case RDMA_BLOCK_16x16:
	case RDMA_BLOCK_16x32:
		return 16;
	case RDMA_BLOCK_32x8:
	case RDMA_BLOCK_32x16:
	case RDMA_BLOCK_32x32:
		return 32;
	}
}

static unsigned int rdma_get_block_h(unsigned int mode)
{
	switch (mode) {
	default:
		return 0;
	case RDMA_BLOCK_8x8:
	case RDMA_BLOCK_16x8:
	case RDMA_BLOCK_32x8:
		return 8;
	case RDMA_BLOCK_8x16:
	case RDMA_BLOCK_16x16:
	case RDMA_BLOCK_32x16:
		return 16;
	case RDMA_BLOCK_8x32:
	case RDMA_BLOCK_16x32:
	case RDMA_BLOCK_32x32:
		return 32;
	}
}


static unsigned int rdma_get_horizontal_shift_uv(unsigned int fmt)
{
	switch (fmt) {
	default:
		return 0;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return 1;
	}
}

static unsigned int rdma_get_vertical_shift_uv(unsigned int fmt)
{
	switch (fmt) {
	default:
		return 0;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return 2;
	case DRM_FORMAT_P010:
		return 1;
	}
}

static unsigned int rdma_get_bits_per_pixel_y(unsigned int fmt)
{
	switch (fmt) {
	default:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
		return 16;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
		return 32;
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_YUYV:
		return 16;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return 8;
	case DRM_FORMAT_P010:
		return 16;
	}
}

static unsigned int rdma_get_bits_per_pixel_uv(unsigned int fmt)
{
	switch (fmt) {
	default:
		return 0;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return 8;
	case DRM_FORMAT_P010:
		return 16;
	}
}

static bool with_alpha(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_ABGRFP16:
		return true;
	default:
		return false;
	}
}

static unsigned int rdma_fmt_convert(unsigned int fmt)
{
	switch (fmt) {
	default:
	case DRM_FORMAT_RGB565:
		return RDMA_INPUT_FORMAT_RGB565;
	case DRM_FORMAT_BGR565:
		return RDMA_INPUT_FORMAT_RGB565 | RDMA_INPUT_SWAP;
	case DRM_FORMAT_RGB888:
		return RDMA_INPUT_FORMAT_RGB888;
	case DRM_FORMAT_BGR888:
		return RDMA_INPUT_FORMAT_RGB888 | RDMA_INPUT_SWAP;
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_RGBA8888:
		return RDMA_INPUT_FORMAT_ARGB8888;
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_BGRA8888:
		return RDMA_INPUT_FORMAT_ARGB8888 | RDMA_INPUT_SWAP;
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
		return RDMA_INPUT_FORMAT_RGBA8888;
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
		return RDMA_INPUT_FORMAT_RGBA8888 | RDMA_INPUT_SWAP;
	case DRM_FORMAT_ABGR2101010:
		return RDMA_INPUT_FORMAT_RGBA8888 | RDMA_INPUT_SWAP | RDMA_INPUT_10BIT;
	case DRM_FORMAT_ARGB2101010:
		return RDMA_INPUT_FORMAT_RGBA8888 | RDMA_INPUT_10BIT;
	case DRM_FORMAT_RGBA1010102:
		return RDMA_INPUT_FORMAT_ARGB8888 | RDMA_INPUT_SWAP | RDMA_INPUT_10BIT;
	case DRM_FORMAT_BGRA1010102:
		return RDMA_INPUT_FORMAT_ARGB8888 | RDMA_INPUT_10BIT;
	case DRM_FORMAT_UYVY:
		return RDMA_INPUT_FORMAT_UYVY;
	case DRM_FORMAT_YUYV:
		return RDMA_INPUT_FORMAT_YUY2;
	case DRM_FORMAT_P010:
		return RDMA_INPUT_FORMAT_NV12 | RDMA_INPUT_10BIT |
				MDP_RDMA_SRC_CON_VAL_LOOSE(1);
	}
}

#if defined(ENABLE_IRQ_HANDLER)
irqreturn_t mtk_mdp_rdma_irq_handler(int irq, void *dev_id)
{
	struct mtk_mdp_rdma *priv = dev_id;
	unsigned int val = readl(priv->data->base + MDP_RDMA_INTERRUPT_STATUS);

	//DRM_MMP_MARK(IRQ, irq, val);

	RDMA_INFO("rdma%d, irq, val:0x%x\n", priv->data->id, val);

	writel(~val, priv->data->base + MDP_RDMA_INTERRUPT_STATUS);

	if (val & BIT(2)) {
		RDMA_INFO("[IRQ] %s: underflow! cnt=%d\n",
			  priv->data->id, priv->data->underflow_cnt);

		priv->data->underflow_cnt++;
	}

	if (val & BIT(1))
		RDMA_INFO("[IRQ] rdma%d: reg update done!\n", priv->data->id);

	if (val & BIT(0))
		RDMA_INFO("[IRQ] rdma%d: frame done!\n", priv->data->id);

	return IRQ_HANDLED;
}
#endif

#ifdef IF_ZERO
static void mtk_mdp_rdma_enable_vblank(void __iomem *base,
				       struct cmdq_pkt *handle,
				       unsigned int cmdq_base)
{
	unsigned int inten;

	inten = REG_FLD_MASK(
		MDP_RDMA_INTERRUPT_ENABLE_FLD_FRAME_COMPLETE_INT_EN);

	cmdq_pkt_write(handle, cmdq_base, base + MDP_RDMA_INTERRUPT_ENABLE,
		inten, 1);
}

static void mtk_mdp_rdma_disable_vblank(void __iomem *base,
					struct cmdq_pkt *handle,
					unsigned int cmdq_base)
{
	unsigned int inten;

	inten = 0;

	cmdq_pkt_write(handle, cmdq_base, base + MDP_RDMA_INTERRUPT_ENABLE,
				   inten, 0);
}
#endif

static int mtk_mdp_rdma_io_cmd(void __iomem *base, struct cmdq_pkt *handle,
			       unsigned int io_cmd, void *params,
			       void __iomem *cmdq_base, resource_size_t regs_pa)
{
	int ret = 0;
	unsigned int inten = IRQ_INT_EN_ALL;

	switch (io_cmd) {
	case IRQ_LEVEL_ALL:
#ifdef CONFIG_MTK_DISPLAY_CMDQ
		cmdq_pkt_write(handle, cmdq_base,
			regs_pa + MDP_RDMA_INTERRUPT_ENABLE, inten, inten);
#else
		writel(inten, base + MDP_RDMA_INTERRUPT_ENABLE);
#endif
		break;
	case IRQ_LEVEL_IDLE:
#ifdef CONFIG_MTK_DISPLAY_CMDQ
		cmdq_pkt_write(handle, cmdq_base,
			regs_pa + MDP_RDMA_INTERRUPT_ENABLE, 0, inten);
#else
		writel(0, base + MDP_RDMA_INTERRUPT_ENABLE);
#endif
		break;
	default:
		break;
	}

	return ret;
}

void mtk_mdp_rdma_start(void __iomem *base,
	resource_size_t regs_pa,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base)
{
#if defined(ENABLE_PM_FLOW)
	//int ret;
#endif
	unsigned int inten = IRQ_INT_EN_ALL;

#if defined(ENABLE_PM_FLOW)
	//ret = pm_runtime_get_sync(comp->dev);
	//if (ret < 0)
	//	DRM_ERROR("Failed to enable power domain: %d\n", ret);
#endif

	mtk_mdp_write(base, inten, MDP_RDMA_INTERRUPT_ENABLE,
		handle, cmdq_base, regs_pa);

	mtk_mdp_rdma_io_cmd(base, handle, IRQ_LEVEL_ALL, NULL, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base, 1<<REG_FLD_SHIFT(MDP_RDMA_EN_FLD_ROT_ENABLE),
		MDP_RDMA_EN, REG_FLD_MASK(MDP_RDMA_EN_FLD_ROT_ENABLE),
		handle, cmdq_base, regs_pa);
}

void mtk_mdp_rdma_stop(void __iomem *base,
	resource_size_t regs_pa,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base)
{
#if defined(ENABLE_PM_FLOW)
	//int ret;
#endif

	mtk_mdp_write_mask(base, 0<<REG_FLD_SHIFT(MDP_RDMA_EN_FLD_ROT_ENABLE),
		MDP_RDMA_EN, REG_FLD_MASK(MDP_RDMA_EN_FLD_ROT_ENABLE),
		handle, cmdq_base, regs_pa);
	mtk_mdp_write(base, 0x0, MDP_RDMA_INTERRUPT_ENABLE, handle, cmdq_base,
		regs_pa);
	mtk_mdp_write(base, 0x0, MDP_RDMA_INTERRUPT_STATUS, handle, cmdq_base,
		regs_pa);
	mtk_mdp_write(base, 0x1, MDP_RDMA_RESET, handle, cmdq_base, regs_pa);
	mtk_mdp_write(base, 0x0, MDP_RDMA_RESET, handle, cmdq_base, regs_pa);

#if defined(ENABLE_PM_FLOW)
	//ret = pm_runtime_put(comp->dev);
	//if (ret < 0)
	//	DRM_ERROR("Failed to disable power domain: %d\n", ret);
#endif
}

void mtk_mdp_rdma_golden_setting(void __iomem *base,
	resource_size_t regs_pa,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base)
{
	int read_request_type = 7;
	int command_div = 1;
	int ext_preultra_en = 1;
	int ultra_en = 0;
	int pre_ultra_en = 1;
	int ext_ultra_en = 1;

	int extrd_arb_max_0 = 3;
	int buf_resv_size_0 = 0;
	int issue_req_th_0 = 0;

	int ultra_th_high_con_0 = 156;
	int ultra_th_low_con_0 = 104;

	mtk_mdp_write_mask(base,
		MDP_RDMA_GMCIF_CON_VAL_READ_REQUEST_TYPE(read_request_type)|
			MDP_RDMA_GMCIF_CON_VAL_COMMAND_DIV(command_div)|
			MDP_RDMA_GMCIF_CON_VAL_EXT_PREULTRA_EN(ext_preultra_en)|
			MDP_RDMA_GMCIF_CON_VAL_ULTRA_EN(ultra_en)|
			MDP_RDMA_GMCIF_CON_VAL_PRE_ULTRA_EN(pre_ultra_en)|
			MDP_RDMA_GMCIF_CON_VAL_EXT_ULTRA_EN(ext_ultra_en),
		MDP_RDMA_GMCIF_CON,
		REG_FLD_MASK(MDP_RDMA_GMCIF_CON_FLD_READ_REQUEST_TYPE)|
			REG_FLD_MASK(MDP_RDMA_GMCIF_CON_FLD_COMMAND_DIV)|
			REG_FLD_MASK(MDP_RDMA_GMCIF_CON_FLD_EXT_PREULTRA_EN)|
			REG_FLD_MASK(MDP_RDMA_GMCIF_CON_FLD_ULTRA_EN)|
			REG_FLD_MASK(MDP_RDMA_GMCIF_CON_FLD_PRE_ULTRA_EN)|
			REG_FLD_MASK(MDP_RDMA_GMCIF_CON_FLD_EXT_ULTRA_EN),
		handle, cmdq_base, regs_pa);

	/*dma buf 0*/
	mtk_mdp_write_mask(base,
		MDP_RDMA_DMABUF_CON_0_VAL_EXTRD_ARB_MAX_0(extrd_arb_max_0)|
			MDP_RDMA_DMABUF_CON_0_VAL_BUF_RESV_SIZE_0(buf_resv_size_0)|
			MDP_RDMA_DMABUF_CON_0_VAL_ISSUE_REQ_TH_0(issue_req_th_0),
		MDP_RDMA_DMABUF_CON_0,
		REG_FLD_MASK(MDP_RDMA_DMABUF_CON_0_FLD_EXTRD_ARB_MAX_0)|
			REG_FLD_MASK(MDP_RDMA_DMABUF_CON_0_FLD_BUF_RESV_SIZE_0)|
			REG_FLD_MASK(MDP_RDMA_DMABUF_CON_0_FLD_ISSUE_REQ_TH_0),
		handle, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base,
		MDP_RDMA_ULTRA_TH_HIGH_CON_0_VAL_PRE_ULTRA_TH_HIGH_OFS_0
			(ultra_th_high_con_0),
		MDP_RDMA_ULTRA_TH_HIGH_CON_0,
		REG_FLD_MASK(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_PRE_ULTRA_TH_HIGH_OFS_0),
		handle, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base,
		MDP_RDMA_ULTRA_TH_LOW_CON_0_VAL_PRE_ULTRA_TH_LOW_OFS_0
			(ultra_th_low_con_0),
		MDP_RDMA_ULTRA_TH_LOW_CON_0,
		REG_FLD_MASK(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_PRE_ULTRA_TH_LOW_OFS_0),
		handle, cmdq_base, regs_pa);

	/*dma buf 1*/
	mtk_mdp_write_mask(base,
		MDP_RDMA_DMABUF_CON_1_VAL_EXTRD_ARB_MAX_1(0)|
			MDP_RDMA_DMABUF_CON_1_VAL_BUF_RESV_SIZE_1(0)|
			MDP_RDMA_DMABUF_CON_1_VAL_ISSUE_REQ_TH_1(0),
		MDP_RDMA_DMABUF_CON_1,
		REG_FLD_MASK(MDP_RDMA_DMABUF_CON_1_FLD_EXTRD_ARB_MAX_1)|
			REG_FLD_MASK(MDP_RDMA_DMABUF_CON_1_FLD_BUF_RESV_SIZE_1)|
			REG_FLD_MASK(MDP_RDMA_DMABUF_CON_1_FLD_ISSUE_REQ_TH_1),
		handle, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base,
		MDP_RDMA_ULTRA_TH_HIGH_CON_1_VAL_PRE_ULTRA_TH_HIGH_OFS_1(0),
		MDP_RDMA_ULTRA_TH_HIGH_CON_1,
		REG_FLD_MASK(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_PRE_ULTRA_TH_HIGH_OFS_1),
		handle, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base,
		MDP_RDMA_ULTRA_TH_LOW_CON_1_VAL_PRE_ULTRA_TH_LOW_OFS_1(0),
		MDP_RDMA_ULTRA_TH_LOW_CON_1,
		REG_FLD_MASK(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_PRE_ULTRA_TH_LOW_OFS_1),
		handle, cmdq_base, regs_pa);

	/*dma buf 2*/
	mtk_mdp_write_mask(base,
		MDP_RDMA_DMABUF_CON_2_VAL_EXTRD_ARB_MAX_2(0)|
			MDP_RDMA_DMABUF_CON_2_VAL_BUF_RESV_SIZE_2(0)|
			MDP_RDMA_DMABUF_CON_2_VAL_ISSUE_REQ_TH_2(0),
		MDP_RDMA_DMABUF_CON_2,
		REG_FLD_MASK(MDP_RDMA_DMABUF_CON_2_FLD_EXTRD_ARB_MAX_2)|
			REG_FLD_MASK(MDP_RDMA_DMABUF_CON_2_FLD_BUF_RESV_SIZE_2)|
			REG_FLD_MASK(MDP_RDMA_DMABUF_CON_2_FLD_ISSUE_REQ_TH_2),
		handle, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base,
		MDP_RDMA_ULTRA_TH_HIGH_CON_2_VAL_PRE_ULTRA_TH_HIGH_OFS_2(0),
		MDP_RDMA_ULTRA_TH_HIGH_CON_2,
		REG_FLD_MASK(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_PRE_ULTRA_TH_HIGH_OFS_2),
		handle, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base,
		MDP_RDMA_ULTRA_TH_LOW_CON_2_VAL_PRE_ULTRA_TH_LOW_OFS_2(0),
		MDP_RDMA_ULTRA_TH_LOW_CON_2,
		REG_FLD_MASK(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_PRE_ULTRA_TH_LOW_OFS_2),
		handle, cmdq_base, regs_pa);

	/*dma buf 3*/
	mtk_mdp_write_mask(base,
		MDP_RDMA_DMABUF_CON_3_VAL_EXTRD_ARB_MAX_3(0)|
			MDP_RDMA_DMABUF_CON_3_VAL_BUF_RESV_SIZE_3(0)|
			MDP_RDMA_DMABUF_CON_3_VAL_ISSUE_REQ_TH_3(0),
		MDP_RDMA_DMABUF_CON_3,
		REG_FLD_MASK(MDP_RDMA_DMABUF_CON_3_FLD_EXTRD_ARB_MAX_3)|
			REG_FLD_MASK(MDP_RDMA_DMABUF_CON_3_FLD_BUF_RESV_SIZE_3)|
			REG_FLD_MASK(MDP_RDMA_DMABUF_CON_3_FLD_ISSUE_REQ_TH_3),
		handle, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base,
		MDP_RDMA_ULTRA_TH_HIGH_CON_3_VAL_PRE_ULTRA_TH_HIGH_OFS_3(0),
		MDP_RDMA_ULTRA_TH_HIGH_CON_3,
		REG_FLD_MASK(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_PRE_ULTRA_TH_HIGH_OFS_3),
		handle, cmdq_base, regs_pa);

	mtk_mdp_write_mask(base,
		MDP_RDMA_ULTRA_TH_LOW_CON_3_VAL_PRE_ULTRA_TH_LOW_OFS_3(0),
		MDP_RDMA_ULTRA_TH_LOW_CON_3,
		REG_FLD_MASK(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_PRE_ULTRA_TH_LOW_OFS_3),
		handle, cmdq_base, regs_pa);
}

void mtk_mdp_rdma_config(void __iomem *base,
	resource_size_t regs_pa,
	struct mtk_mdp_rdma_cfg *cfg,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base,
	bool sec_on)
{
#ifdef IF_ZERO
	unsigned long long threshold;
	unsigned int reg;
#endif
	unsigned int sourceImagePitchY =
			rdma_get_y_pitch(cfg->fmt, cfg->source_width);
	unsigned int sourceImagePitchUV =
			rdma_get_uv_pitch(cfg->fmt, cfg->source_width);
	unsigned int ringYStartLine = 0;
	unsigned int ringYEndLine = cfg->height - 1;
	unsigned int videoBlockWidth = rdma_get_block_w(cfg->block_size);
	unsigned int videoBlockHeight = rdma_get_block_h(cfg->block_size);
	unsigned int frameModeShift = 0;
	unsigned int horizontalShiftUV = rdma_get_horizontal_shift_uv(cfg->fmt);
	unsigned int verticalShiftUV = rdma_get_vertical_shift_uv(cfg->fmt);
	unsigned int bitsPerPixelY = rdma_get_bits_per_pixel_y(cfg->fmt);
	unsigned int bitsPerPixelUV = rdma_get_bits_per_pixel_uv(cfg->fmt);
	unsigned int videoBlockShiftW = 0;
	unsigned int videoBlockShiftH = 0;

	unsigned int tile_x_left = cfg->x_left;
	unsigned int tile_y_top = cfg->y_top;
	unsigned int offset_y = 0;
	unsigned int offset_u = 0;
	unsigned int offset_v = 0;

	RDMA_DEBUG("sourceImagePitchY = %d\n", sourceImagePitchY);
	RDMA_DEBUG("sourceImagePitchUV = %d\n", sourceImagePitchUV);
	RDMA_DEBUG("ringYStartLine = %d\n", ringYStartLine);
	RDMA_DEBUG("ringYEndLine = %d\n", ringYEndLine);
	RDMA_DEBUG("videoBlockWidth = %d\n", videoBlockWidth);
	RDMA_DEBUG("videoBlockHeight = %d\n", videoBlockHeight);
	RDMA_DEBUG("frameModeShift = %d\n", frameModeShift);
	RDMA_DEBUG("horizontalShiftUV = %d\n", horizontalShiftUV);
	RDMA_DEBUG("verticalShiftUV = %d\n", verticalShiftUV);
	RDMA_DEBUG("bitsPerPixelY = %d\n", bitsPerPixelY);
	RDMA_DEBUG("bitsPerPixelUV = %d\n", bitsPerPixelUV);
	RDMA_DEBUG("videoBlockShiftW = %d\n", videoBlockShiftW);
	RDMA_DEBUG("videoBlockShiftH = %d\n", videoBlockShiftH);

	/******** Frame ********/
	mtk_mdp_write_mask(base,
		1<<REG_FLD_SHIFT(MDP_RDMA_SRC_CON_FLD_UNIFORM_CONFIG), MDP_RDMA_SRC_CON,
		REG_FLD_MASK(MDP_RDMA_SRC_CON_FLD_UNIFORM_CONFIG), handle,
		cmdq_base, regs_pa);

	// Setup format and swap
	mtk_mdp_write_mask(base, rdma_fmt_convert(cfg->fmt), MDP_RDMA_SRC_CON,
		REG_FLD_MASK(MDP_RDMA_SRC_CON_FLD_SWAP)|
		REG_FLD_MASK(MDP_RDMA_SRC_CON_FLD_SRC_FORMAT)|
		REG_FLD_MASK(MDP_RDMA_SRC_CON_FLD_LOOSE)|
		REG_FLD_MASK(MDP_RDMA_SRC_CON_FLD_BIT_NUMBER), handle,
		cmdq_base, regs_pa);

	if ((!cfg->csc_enable) && with_alpha(cfg->fmt)) {
		mtk_mdp_write_mask(base,
		1<<REG_FLD_SHIFT(MDP_RDMA_SRC_CON_FLD_OUTPUT_ARGB),
		MDP_RDMA_SRC_CON,
		REG_FLD_MASK(MDP_RDMA_SRC_CON_FLD_OUTPUT_ARGB),
		handle, cmdq_base, regs_pa);
	} else {
		mtk_mdp_write_mask(base,
		0<<REG_FLD_SHIFT(MDP_RDMA_SRC_CON_FLD_OUTPUT_ARGB),
		MDP_RDMA_SRC_CON,
		REG_FLD_MASK(MDP_RDMA_SRC_CON_FLD_OUTPUT_ARGB),
		handle, cmdq_base, regs_pa);
	}

	// Setup start address

#if defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
	if (sec_on) {
		u32 meta_type, regs_addr, offset;

		offset = 0;
		regs_addr = regs_pa + MDP_RDMA_SRC_BASE_0;
		meta_type = CMDQ_IWC_H_2_MVA;
		cmdq_sec_pkt_write_reg(handle, regs_addr,
			cfg->addr0, meta_type,
			offset, cfg->source_size, 0);

		if (cfg->addr1) {
			regs_addr = regs_pa + MDP_RDMA_SRC_BASE_1;
			meta_type = CMDQ_IWC_H_2_MVA;
			offset = cfg->addr1 - cfg->addr0;
			cmdq_sec_pkt_write_reg(handle, regs_addr,
				cfg->addr0, meta_type,
				offset, cfg->source_size, 0);
		}

		if (cfg->addr2) {
			regs_addr = regs_pa + MDP_RDMA_SRC_BASE_2;
			meta_type = CMDQ_IWC_H_2_MVA;
			offset = cfg->addr2 - cfg->addr0;
			cmdq_sec_pkt_write_reg(handle, regs_addr,
				cfg->addr0, meta_type,
				offset, cfg->source_size, 0);
		}
	} else
#endif
	{
		mtk_mdp_write_mask(base, cfg->addr0, MDP_RDMA_SRC_BASE_0,
			REG_FLD_MASK(MDP_RDMA_SRC_BASE_0_FLD_SRC_BASE_0),
			handle, cmdq_base, regs_pa);
		mtk_mdp_write_mask(base, cfg->addr1, MDP_RDMA_SRC_BASE_1,
			REG_FLD_MASK(MDP_RDMA_SRC_BASE_1_FLD_SRC_BASE_1),
			handle, cmdq_base, regs_pa);
		mtk_mdp_write_mask(base, cfg->addr2, MDP_RDMA_SRC_BASE_2,
			REG_FLD_MASK(MDP_RDMA_SRC_BASE_2_FLD_SRC_BASE_2),
			handle, cmdq_base, regs_pa);
	}

	// Setup source frame pitch
	mtk_mdp_write_mask(base, sourceImagePitchY,
		MDP_RDMA_MF_BKGD_SIZE_IN_BYTE,
		REG_FLD_MASK(MDP_RDMA_MF_BKGD_SIZE_IN_BYTE_FLD_MF_BKGD_WB),
		handle, cmdq_base, regs_pa);
	mtk_mdp_write_mask(base, sourceImagePitchUV,
		MDP_RDMA_SF_BKGD_SIZE_IN_BYTE,
		REG_FLD_MASK(MDP_RDMA_SF_BKGD_SIZE_IN_BYTE_FLD_SF_BKGD_WB),
		handle, cmdq_base, regs_pa);

	// Setup AFBC
	if (cfg->encode_type == RDMA_ENCODE_AFBC) {
		mtk_mdp_write_mask(base,
		    (cfg->source_width)<<REG_FLD_SHIFT(
		    MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_FLD_MF_BKGD_WP),
		    MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL,
		    REG_FLD_MASK(MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_FLD_MF_BKGD_WP),
		    handle, cmdq_base, regs_pa);
		mtk_mdp_write_mask(base,
		    (cfg->height)<<REG_FLD_SHIFT(
			MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_FLD_MF_BKGD_HP),
			MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL,
			REG_FLD_MASK(
			MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_FLD_MF_BKGD_HP),
			handle, cmdq_base, regs_pa);

		mtk_mdp_write_mask(base,
			1<<REG_FLD_SHIFT(MDP_RDMA_COMP_CON_FLD_AFBC_YUV_TRANSFORM),
			MDP_RDMA_COMP_CON,
			REG_FLD_MASK(MDP_RDMA_COMP_CON_FLD_AFBC_YUV_TRANSFORM),
			handle, cmdq_base, regs_pa);

		mtk_mdp_write_mask(base,
			1<<REG_FLD_SHIFT(MDP_RDMA_COMP_CON_FLD_UFBDC_EN),
			MDP_RDMA_COMP_CON,
			REG_FLD_MASK(MDP_RDMA_COMP_CON_FLD_UFBDC_EN),
			handle, cmdq_base, regs_pa);
		mtk_mdp_write_mask(base,
			1<<REG_FLD_SHIFT(MDP_RDMA_COMP_CON_FLD_AFBC_EN),
			MDP_RDMA_COMP_CON,
			REG_FLD_MASK(MDP_RDMA_COMP_CON_FLD_AFBC_EN),
			handle, cmdq_base, regs_pa);
	} else {
		mtk_mdp_write_mask(base,
			0<<REG_FLD_SHIFT(MDP_RDMA_COMP_CON_FLD_AFBC_YUV_TRANSFORM),
			MDP_RDMA_COMP_CON,
			REG_FLD_MASK(MDP_RDMA_COMP_CON_FLD_AFBC_YUV_TRANSFORM),
			handle, cmdq_base, regs_pa);

		mtk_mdp_write_mask(base,
			0<<REG_FLD_SHIFT(MDP_RDMA_COMP_CON_FLD_UFBDC_EN),
			MDP_RDMA_COMP_CON,
			REG_FLD_MASK(MDP_RDMA_COMP_CON_FLD_UFBDC_EN),
			handle, cmdq_base, regs_pa);
		mtk_mdp_write_mask(base,
			0<<REG_FLD_SHIFT(MDP_RDMA_COMP_CON_FLD_AFBC_EN),
			MDP_RDMA_COMP_CON,
			REG_FLD_MASK(MDP_RDMA_COMP_CON_FLD_AFBC_EN),
			handle, cmdq_base, regs_pa);

	}

	// Setup Ouptut 10b (csc, 8.2)
	mtk_mdp_write_mask(base, 1<<REG_FLD_SHIFT(MDP_RDMA_CON_FLD_OUTPUT_10B),
		MDP_RDMA_CON, REG_FLD_MASK(MDP_RDMA_CON_FLD_OUTPUT_10B), handle,
		cmdq_base, regs_pa);

	// Setup Simple Mode
	mtk_mdp_write_mask(base, 1<<REG_FLD_SHIFT(MDP_RDMA_CON_FLD_SIMPLE_MODE),
		MDP_RDMA_CON, REG_FLD_MASK(MDP_RDMA_CON_FLD_SIMPLE_MODE),
		handle, cmdq_base, regs_pa);

	// Setup Color Space Conversion
	mtk_mdp_write_mask(base,
	(cfg->csc_enable)<<REG_FLD_SHIFT(MDP_RDMA_TRANSFORM_0_FLD_TRANS_EN),
	MDP_RDMA_TRANSFORM_0, REG_FLD_MASK(MDP_RDMA_TRANSFORM_0_FLD_TRANS_EN),
	handle, cmdq_base, regs_pa);
	mtk_mdp_write_mask(base,
	(cfg->profile)<<REG_FLD_SHIFT(MDP_RDMA_TRANSFORM_0_FLD_int_matrix_sel),
	MDP_RDMA_TRANSFORM_0,
	REG_FLD_MASK(MDP_RDMA_TRANSFORM_0_FLD_int_matrix_sel),
	handle, cmdq_base, regs_pa);

	/******** Tile ********/

	if (cfg->block_size == RDMA_BLOCK_NONE) {
		ringYStartLine = tile_y_top;

		offset_y  = (tile_x_left * bitsPerPixelY >> 3)
			+ ringYStartLine * sourceImagePitchY;
		offset_u  = ((tile_x_left >> horizontalShiftUV)
			* bitsPerPixelUV >> 3)
			+ (ringYStartLine >> verticalShiftUV)
			* sourceImagePitchUV;
		offset_v  = ((tile_x_left >> horizontalShiftUV)
			* bitsPerPixelUV >> 3)
			+ (ringYStartLine >> verticalShiftUV)
			* sourceImagePitchUV;
	} else {
		// Alignment X left in block boundary
		tile_x_left = ((tile_x_left >> videoBlockShiftW)
			<< videoBlockShiftW);
		// Alignment Y top  in block boundary
		tile_y_top  = ((tile_y_top  >> videoBlockShiftH)
			<< videoBlockShiftH);

		offset_y = (tile_x_left * (videoBlockHeight << frameModeShift)
			* bitsPerPixelY >> 3)
			+ (tile_y_top >> videoBlockShiftH) * sourceImagePitchY;
		offset_u = ((tile_x_left >> horizontalShiftUV)
			* ((videoBlockHeight >> verticalShiftUV)
			<< frameModeShift) * bitsPerPixelUV >> 3)
			+ (tile_y_top >> videoBlockShiftH) * sourceImagePitchUV;
		offset_v = ((tile_x_left >> horizontalShiftUV)
			* ((videoBlockHeight >> verticalShiftUV)
			<< frameModeShift) * bitsPerPixelUV >> 3)
			+ (tile_y_top >> videoBlockShiftH) * sourceImagePitchUV;
	}

	// Setup AFBC
	if (cfg->encode_type == RDMA_ENCODE_AFBC) {
		// tile x left
		mtk_mdp_write_mask(base,
			tile_x_left<<
			REG_FLD_SHIFT(MDP_RDMA_SRC_OFFSET_WP_FLD_SRC_OFFSET_WP),
			MDP_RDMA_SRC_OFFSET_WP,
			REG_FLD_MASK(MDP_RDMA_SRC_OFFSET_WP_FLD_SRC_OFFSET_WP),
			handle, cmdq_base, regs_pa);
		// tile y top
		mtk_mdp_write_mask(base,
			tile_y_top<<
			REG_FLD_SHIFT(MDP_RDMA_SRC_OFFSET_HP_FLD_SRC_OFFSET_HP),
			MDP_RDMA_SRC_OFFSET_HP,
			REG_FLD_MASK(MDP_RDMA_SRC_OFFSET_HP_FLD_SRC_OFFSET_HP),
			handle, cmdq_base, regs_pa);
	}

	// Set Y pixel offset
	mtk_mdp_write_mask(base,
	    offset_y<<REG_FLD_SHIFT(MDP_RDMA_SRC_OFFSET_0_FLD_SRC_OFFSET_0),
	    MDP_RDMA_SRC_OFFSET_0,
		REG_FLD_MASK(MDP_RDMA_SRC_OFFSET_0_FLD_SRC_OFFSET_0),
		handle, cmdq_base, regs_pa);
	// Set U pixel offset
	mtk_mdp_write_mask(base,
		offset_u<<REG_FLD_SHIFT(MDP_RDMA_SRC_OFFSET_1_FLD_SRC_OFFSET_1),
		MDP_RDMA_SRC_OFFSET_1,
		REG_FLD_MASK(MDP_RDMA_SRC_OFFSET_1_FLD_SRC_OFFSET_1),
		handle, cmdq_base, regs_pa);
	// Set V pixel offset
	mtk_mdp_write_mask(base,
		offset_v<<REG_FLD_SHIFT(MDP_RDMA_SRC_OFFSET_2_FLD_SRC_OFFSET_2),
		MDP_RDMA_SRC_OFFSET_2,
		REG_FLD_MASK(MDP_RDMA_SRC_OFFSET_2_FLD_SRC_OFFSET_2),
		handle, cmdq_base, regs_pa);

	// Set source size
	mtk_mdp_write_mask(base,
		(cfg->width)<<REG_FLD_SHIFT(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_W),
		MDP_RDMA_MF_SRC_SIZE,
		REG_FLD_MASK(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_W),
		handle, cmdq_base, regs_pa);
	mtk_mdp_write_mask(base,
	    cfg->height<<REG_FLD_SHIFT(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_H),
	    MDP_RDMA_MF_SRC_SIZE,
	    REG_FLD_MASK(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_H),
	    handle, cmdq_base, regs_pa);

	// Set target (clip) size
	mtk_mdp_write_mask(base,
		cfg->width<<REG_FLD_SHIFT(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_W),
		MDP_RDMA_MF_CLIP_SIZE,
		REG_FLD_MASK(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_W),
		handle, cmdq_base, regs_pa);
	mtk_mdp_write_mask(base,
		cfg->height<<REG_FLD_SHIFT(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_H),
		MDP_RDMA_MF_CLIP_SIZE,
		REG_FLD_MASK(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_H),
		handle, cmdq_base, regs_pa);
	// Set crop offset
	mtk_mdp_write_mask(base,
		0<<REG_FLD_SHIFT(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_W_1),
		MDP_RDMA_MF_OFFSET_1,
		REG_FLD_MASK(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_W_1),
		handle, cmdq_base, regs_pa);
	mtk_mdp_write_mask(base,
		0<<REG_FLD_SHIFT(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_H_1),
		MDP_RDMA_MF_OFFSET_1,
		REG_FLD_MASK(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_H_1),
		handle, cmdq_base, regs_pa);

	// Target Line
	mtk_mdp_write_mask(base,
		(cfg->height)<<
		REG_FLD_SHIFT(MDP_RDMA_TARGET_LINE_FLD_LINE_THRESHOLD),
		MDP_RDMA_TARGET_LINE,
		REG_FLD_MASK(MDP_RDMA_TARGET_LINE_FLD_LINE_THRESHOLD),
		handle, cmdq_base, regs_pa);
	mtk_mdp_write_mask(base,
		1<<REG_FLD_SHIFT(MDP_RDMA_TARGET_LINE_FLD_TARGET_LINE_EN),
		MDP_RDMA_TARGET_LINE,
		REG_FLD_MASK(MDP_RDMA_TARGET_LINE_FLD_TARGET_LINE_EN),
		handle, cmdq_base, regs_pa);
}

int mtk_mdp_rdma_dump(void __iomem *base)
{
	void __iomem *baddr = base;

	RDMA_INFO("(0x010)INTERRUPT_ENABLE=0x%x\n",
		readl(MDP_RDMA_INTERRUPT_ENABLE + baddr));
	RDMA_INFO("(0x018)INTERRUPT_STATUS=0x%x\n",
		readl(MDP_RDMA_INTERRUPT_STATUS + baddr));
	RDMA_INFO("(0x020)CON=0x%x\n",
		readl(MDP_RDMA_CON + baddr));
	RDMA_INFO("(0x024)SHADOW_CTRL=0x%x\n",
		readl(MDP_RDMA_SHADOW_CTRL + baddr));
	RDMA_INFO("(0x030)SRC_CON=0x%x\n",
		readl(MDP_RDMA_SRC_CON + baddr));
	RDMA_INFO("(0x038)COMP_CON=0x%x\n",
		readl(MDP_RDMA_COMP_CON + baddr));
	RDMA_INFO("(0x060)MF_BKGD_SIZE_IN_BYTE=0x%x\n",
		readl(MDP_RDMA_MF_BKGD_SIZE_IN_BYTE + baddr));
	RDMA_INFO("(0x068)MF_BKGD_SIZE_IN_PIXEL=0x%x\n",
		readl(MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL + baddr));
	RDMA_INFO("(0x070)MF_SRC_SIZE=0x%x\n",
		readl(MDP_RDMA_MF_SRC_SIZE + baddr));
	RDMA_INFO("(0x078)MF_CLIP_SIZE=0x%x\n",
		readl(MDP_RDMA_MF_CLIP_SIZE + baddr));
	RDMA_INFO("(0x080)MF_OFFSET_1=0x%x\n",
		readl(MDP_RDMA_MF_OFFSET_1 + baddr));
	RDMA_INFO("(0x090)SF_BKGD_SIZE_IN_BYTE=0x%x\n",
		readl(MDP_RDMA_SF_BKGD_SIZE_IN_BYTE + baddr));
	RDMA_INFO("(0x098)MF_BKGD_H_SIZE_IN_PIXEL=0x%x\n",
		readl(MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL + baddr));
	RDMA_INFO("(0x0a0)TARGET_LINE=0x%x\n",
		readl(MDP_RDMA_TARGET_LINE + baddr));
	RDMA_INFO("(0x118)SRC_OFFSET_0=0x%x\n",
		readl(MDP_RDMA_SRC_OFFSET_0 + baddr));
	RDMA_INFO("(0x120)SRC_OFFSET_1=0x%x\n",
		readl(MDP_RDMA_SRC_OFFSET_1 + baddr));
	RDMA_INFO("(0x128)SRC_OFFSET_2=0x%x\n",
		readl(MDP_RDMA_SRC_OFFSET_2 + baddr));
	RDMA_INFO("(0x148)SRC_OFFSET_WP=0x%x\n",
		readl(MDP_RDMA_SRC_OFFSET_WP + baddr));
	RDMA_INFO("(0x150)SRC_OFFSET_HP=0x%x\n",
		readl(MDP_RDMA_SRC_OFFSET_HP + baddr));
	RDMA_INFO("(0x154)C00=0x%x\n",
		readl(MDP_RDMA_C00 + baddr));
	RDMA_INFO("(0x158)C01=0x%x\n",
		readl(MDP_RDMA_C01 + baddr));
	RDMA_INFO("(0x15c)C02=0x%x\n",
		readl(MDP_RDMA_C02 + baddr));
	RDMA_INFO("(0x160)C10=0x%x\n",
		readl(MDP_RDMA_C10 + baddr));
	RDMA_INFO("(0x164)C11=0x%x\n",
		readl(MDP_RDMA_C11 + baddr));
	RDMA_INFO("(0x168)C12=0x%x\n",
		readl(MDP_RDMA_C12 + baddr));
	RDMA_INFO("(0x170)C20=0x%x\n",
		readl(MDP_RDMA_C20 + baddr));
	RDMA_INFO("(0x174)C21=0x%x\n",
		readl(MDP_RDMA_C21 + baddr));
	RDMA_INFO("(0x178)C22=0x%x\n",
		readl(MDP_RDMA_C22 + baddr));
	RDMA_INFO("(0x178)PRE_ADD_0=0x%x\n",
		readl(MDP_RDMA_PRE_ADD_0 + baddr));
	RDMA_INFO("(0x17c)PRE_ADD_1=0x%x\n",
		readl(MDP_RDMA_PRE_ADD_1 + baddr));
	RDMA_INFO("(0x180)PRE_ADD_2=0x%x\n",
		readl(MDP_RDMA_PRE_ADD_2 + baddr));
	RDMA_INFO("(0x184)POST_ADD_0=0x%x\n",
		readl(MDP_RDMA_POST_ADD_0 + baddr));
	RDMA_INFO("(0x188)POST_ADD_1=0x%x\n",
		readl(MDP_RDMA_POST_ADD_1 + baddr));
	RDMA_INFO("(0x18c)POST_ADD_2=0x%x\n",
		readl(MDP_RDMA_POST_ADD_2 + baddr));
	RDMA_INFO("(0xf00)SRC_BASE_0=0x%x\n",
		readl(MDP_RDMA_SRC_BASE_0 + baddr));

	return 0;
}

bool mtk_mdp_rdma_is_enable(void __iomem *base)
{
	unsigned int regval;

	regval = readl(base + MDP_RDMA_EN);
	return REG_FLD_VAL_GET(MDP_RDMA_EN_FLD_ROT_ENABLE, regval);
}

unsigned int mtk_mdp_rdma_get_addr(void __iomem *base)
{
	return readl(base + MDP_RDMA_SRC_BASE_0);
}

void mtk_mdp_rdma_set_addr(void __iomem *base,
	resource_size_t regs_pa,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base,
	unsigned int rdma_addr)
{
	mtk_mdp_write(base, rdma_addr, MDP_RDMA_SRC_BASE_0,
		handle, cmdq_base, regs_pa);
}

#if defined(ENABLE_CLK_FLOW)
void mtk_mdp_rdma_prepare(void __iomem *base, bool shadow)
{
	//mtk_ddp_comp_clk_prepare(comp);

#if defined(CONFIG_DRM_MTK_SHADOW_REGISTER_SUPPORT)
	if (shadow) { // TODO: check support_shadow
		/* Enable shadow register and read shadow register */
		mtk_mdp_write_mask_cpu(base, 0x0,
			MDP_RDMA_SHADOW_CTRL,
			REG_FLD_MASK(MDP_RDMA_SHADOW_CTRL_FLD_BYPASS_SHADOW));
	} else {
		/* Bypass shadow register and read shadow register */
		mtk_mdp_write_mask_cpu(base, 0x1,
			MDP_RDMA_SHADOW_CTRL,
			REG_FLD_MASK(MDP_RDMA_SHADOW_CTRL_FLD_BYPASS_SHADOW));
	}
#endif
}

void mtk_mdp_rdma_unprepare(void __iomem *base)
{
	//mtk_ddp_comp_clk_unprepare(comp);
}
#endif
