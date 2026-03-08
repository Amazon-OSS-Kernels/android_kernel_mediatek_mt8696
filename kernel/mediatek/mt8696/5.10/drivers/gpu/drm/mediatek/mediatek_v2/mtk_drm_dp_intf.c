// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
 */

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_graph.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>

#include <video/videomode.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_bridge_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_of.h>
#include <drm/drm_simple_kms_helper.h>

#include "mtk_dpi_regs.h"
#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp_comp.h"

enum mtk_dpintf_out_bit_num {
	MTK_DPINTF_OUT_BIT_NUM_8BITS,
	MTK_DPINTF_OUT_BIT_NUM_10BITS,
	MTK_DPINTF_OUT_BIT_NUM_12BITS,
	MTK_DPINTF_OUT_BIT_NUM_16BITS
};

enum mtk_dpintf_out_yc_map {
	MTK_DPINTF_OUT_YC_MAP_RGB,
	MTK_DPINTF_OUT_YC_MAP_CYCY,
	MTK_DPINTF_OUT_YC_MAP_YCYC,
	MTK_DPINTF_OUT_YC_MAP_CY,
	MTK_DPINTF_OUT_YC_MAP_YC
};

enum mtk_dpintf_out_channel_swap {
	MTK_DPINTF_OUT_CHANNEL_SWAP_RGB,
	MTK_DPINTF_OUT_CHANNEL_SWAP_GBR,
	MTK_DPINTF_OUT_CHANNEL_SWAP_BRG,
	MTK_DPINTF_OUT_CHANNEL_SWAP_RBG,
	MTK_DPINTF_OUT_CHANNEL_SWAP_GRB,
	MTK_DPINTF_OUT_CHANNEL_SWAP_BGR
};

enum mtk_dpintf_out_color_format {
	MTK_DPINTF_COLOR_FORMAT_RGB,
	MTK_DPINTF_COLOR_FORMAT_RGB_FULL,
	MTK_DPINTF_COLOR_FORMAT_YCBCR_444,
	MTK_DPINTF_COLOR_FORMAT_YCBCR_422,
	MTK_DPINTF_COLOR_FORMAT_XV_YCC,
	MTK_DPINTF_COLOR_FORMAT_YCBCR_444_FULL,
	MTK_DPINTF_COLOR_FORMAT_YCBCR_422_FULL
};

enum TVDPLL_CLK {
	TVDPLL_PLL = 0,
	TVDPLL_D2 = 2,
	TVDPLL_D4 = 4,
	TVDPLL_D8 = 8,
	TVDPLL_D16 = 16,
};

struct mtk_dpintf {
	struct mtk_ddp_comp ddp_comp;
	struct drm_encoder encoder;
	struct drm_bridge bridge;
	struct drm_bridge *next_bridge;
	struct drm_connector *connector;
	void __iomem *regs;
	struct device *dev;
	struct clk *engine_clk;
	struct clk *dpi_ck_cg;
	struct clk *pixel_clk;
	struct clk *tvd_clk;
	struct clk *pclk_src[5];
	int irq;
	struct drm_display_mode mode;
	const struct mtk_dpintf_conf *conf;
	enum mtk_dpintf_out_color_format color_format;
	enum mtk_dpintf_out_yc_map yc_map;
	enum mtk_dpintf_out_bit_num bit_num;
	enum mtk_dpintf_out_channel_swap channel_swap;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_gpio;
	struct pinctrl_state *pins_dpi;
	u32 output_fmt;
	int refcount;
};

static inline struct mtk_dpintf *bridge_to_dpi(struct drm_bridge *b)
{
	return container_of(b, struct mtk_dpintf, bridge);
}

enum mtk_dpintf_polarity {
	MTK_DPINTF_POLARITY_RISING,
	MTK_DPINTF_POLARITY_FALLING,
};

struct mtk_dpintf_polarities {
	enum mtk_dpintf_polarity de_pol;
	enum mtk_dpintf_polarity ck_pol;
	enum mtk_dpintf_polarity hsync_pol;
	enum mtk_dpintf_polarity vsync_pol;
};

struct mtk_dpintf_sync_param {
	u32 sync_width;
	u32 front_porch;
	u32 back_porch;
	bool shift_half_line;
};

struct mtk_dpintf_yc_limit {
	u16 y_top;
	u16 y_bottom;
	u16 c_top;
	u16 c_bottom;
};

struct mtk_dpintf_conf {
	unsigned int (*cal_factor)(int clock);
	u32 reg_h_fre_con;
	u32 max_clock_khz;
	bool edge_sel_en;
	const u32 *output_fmts;
	u32 num_output_fmts;
	const u32 *input_fmts;
	u32 num_input_fmts;
	bool is_ck_de_pol;
	bool is_dpintf;
	bool csc_support;
	bool swap_input_support;
	// Mask used for HWIDTH, HPORCH, VSYNC_WIDTH and VSYNC_PORCH (no shift)
	u32 dimension_mask;
	// Mask used for HSIZE and VSIZE (no shift)
	u32 hvsize_mask;
	u32 channel_swap_shift;
	u32 yuv422_en_bit;
	u32 csc_enable_bit;
	const struct mtk_dpintf_yc_limit *limit;
};

static void mtk_dpintf_mask(struct mtk_dpintf *dpintf, u32 offset, u32 val, u32 mask)
{
	u32 tmp = readl(dpintf->regs + offset) & ~mask;

	tmp |= (val & mask);
	writel(tmp, dpintf->regs + offset);
}

static void mtk_dpintf_sw_reset(struct mtk_dpintf *dpintf, bool reset)
{
	mtk_dpintf_mask(dpintf, DPI_RET, reset ? RST : 0, RST);
}

static void mtk_dpintf_enable(struct mtk_dpintf *dpintf)
{
	mtk_dpintf_mask(dpintf, DPI_EN, EN, EN);
}

static void mtk_dpintf_disable(struct mtk_dpintf *dpintf)
{
	mtk_dpintf_mask(dpintf, DPI_EN, 0, EN);
}

static void mtk_dpintf_config_hsync(struct mtk_dpintf *dpintf,
				 struct mtk_dpintf_sync_param *sync)
{
	mtk_dpintf_mask(dpintf, DPI_TGEN_HWIDTH, sync->sync_width << HPW,
		     dpintf->conf->dimension_mask << HPW);
	mtk_dpintf_mask(dpintf, DPI_TGEN_HPORCH, sync->back_porch << HBP,
		     dpintf->conf->dimension_mask << HBP);
	mtk_dpintf_mask(dpintf, DPI_TGEN_HPORCH, sync->front_porch << HFP,
		     dpintf->conf->dimension_mask << HFP);
}

static void mtk_dpintf_config_vsync(struct mtk_dpintf *dpintf,
				 struct mtk_dpintf_sync_param *sync,
				 u32 width_addr, u32 porch_addr)
{
	mtk_dpintf_mask(dpintf, width_addr,
		     sync->shift_half_line << VSYNC_HALF_LINE_SHIFT,
		     VSYNC_HALF_LINE_MASK);
	mtk_dpintf_mask(dpintf, width_addr,
		     sync->sync_width << VSYNC_WIDTH_SHIFT,
		     dpintf->conf->dimension_mask << VSYNC_WIDTH_SHIFT);
	mtk_dpintf_mask(dpintf, porch_addr,
		     sync->back_porch << VSYNC_BACK_PORCH_SHIFT,
		     dpintf->conf->dimension_mask << VSYNC_BACK_PORCH_SHIFT);
	mtk_dpintf_mask(dpintf, porch_addr,
		     sync->front_porch << VSYNC_FRONT_PORCH_SHIFT,
		     dpintf->conf->dimension_mask << VSYNC_FRONT_PORCH_SHIFT);
}

static void mtk_dpintf_config_vsync_lodd(struct mtk_dpintf *dpintf,
				      struct mtk_dpintf_sync_param *sync)
{
	mtk_dpintf_config_vsync(dpintf, sync, DPI_TGEN_VWIDTH, DPI_TGEN_VPORCH);
}

static void mtk_dpintf_config_vsync_leven(struct mtk_dpintf *dpintf,
				       struct mtk_dpintf_sync_param *sync)
{
	mtk_dpintf_config_vsync(dpintf, sync, DPI_TGEN_VWIDTH_LEVEN,
			     DPI_TGEN_VPORCH_LEVEN);
}

static void mtk_dpintf_config_vsync_rodd(struct mtk_dpintf *dpintf,
				      struct mtk_dpintf_sync_param *sync)
{
	mtk_dpintf_config_vsync(dpintf, sync, DPI_TGEN_VWIDTH_RODD,
			     DPI_TGEN_VPORCH_RODD);
}

static void mtk_dpintf_config_vsync_reven(struct mtk_dpintf *dpintf,
				       struct mtk_dpintf_sync_param *sync)
{
	mtk_dpintf_config_vsync(dpintf, sync, DPI_TGEN_VWIDTH_REVEN,
			     DPI_TGEN_VPORCH_REVEN);
}

static void mtk_dpintf_config_pol(struct mtk_dpintf *dpintf,
			       struct mtk_dpintf_polarities *dpi_pol)
{
	unsigned int pol;
	unsigned int mask;

	mask = HSYNC_POL | VSYNC_POL;
	pol = (dpi_pol->hsync_pol == MTK_DPINTF_POLARITY_RISING ? 0 : HSYNC_POL) |
	      (dpi_pol->vsync_pol == MTK_DPINTF_POLARITY_RISING ? 0 : VSYNC_POL);
	if (dpintf->conf->is_ck_de_pol) {
		mask |= CK_POL | DE_POL;
		pol |= (dpi_pol->ck_pol == MTK_DPINTF_POLARITY_RISING ?
			0 : CK_POL) |
		       (dpi_pol->de_pol == MTK_DPINTF_POLARITY_RISING ?
			0 : DE_POL);
	}

	mtk_dpintf_mask(dpintf, DPI_OUTPUT_SETTING, pol, mask);
}

static void mtk_dpintf_config_3d(struct mtk_dpintf *dpintf, bool en_3d)
{
	mtk_dpintf_mask(dpintf, DPI_CON, en_3d ? TDFP_EN : 0, TDFP_EN);
}

static void mtk_dpintf_config_interface(struct mtk_dpintf *dpintf, bool inter)
{
	mtk_dpintf_mask(dpintf, DPI_CON, inter ? INTL_EN : 0, INTL_EN);
}

static void mtk_dpintf_config_fb_size(struct mtk_dpintf *dpintf, u32 width, u32 height)
{
	mtk_dpintf_mask(dpintf, DPI_SIZE, width << HSIZE,
		     dpintf->conf->hvsize_mask << HSIZE);
	mtk_dpintf_mask(dpintf, DPI_SIZE, height << VSIZE,
		     dpintf->conf->hvsize_mask << VSIZE);
}

static void mtk_dpintf_config_channel_limit(struct mtk_dpintf *dpintf)
{
	const struct mtk_dpintf_yc_limit *limit = dpintf->conf->limit;

	mtk_dpintf_mask(dpintf, DPI_Y_LIMIT, limit->y_bottom << Y_LIMINT_BOT,
		     Y_LIMINT_BOT_MASK);
	mtk_dpintf_mask(dpintf, DPI_Y_LIMIT, limit->y_top << Y_LIMINT_TOP,
		     Y_LIMINT_TOP_MASK);
	mtk_dpintf_mask(dpintf, DPI_C_LIMIT, limit->c_bottom << C_LIMIT_BOT,
		     C_LIMIT_BOT_MASK);
	mtk_dpintf_mask(dpintf, DPI_C_LIMIT, limit->c_top << C_LIMIT_TOP,
		     C_LIMIT_TOP_MASK);
}

static void mtk_dpintf_config_bit_num(struct mtk_dpintf *dpintf,
				   enum mtk_dpintf_out_bit_num num)
{
	u32 val;

	switch (num) {
	case MTK_DPINTF_OUT_BIT_NUM_8BITS:
		val = OUT_BIT_8;
		break;
	case MTK_DPINTF_OUT_BIT_NUM_10BITS:
		val = OUT_BIT_10;
		break;
	case MTK_DPINTF_OUT_BIT_NUM_12BITS:
		val = OUT_BIT_12;
		break;
	case MTK_DPINTF_OUT_BIT_NUM_16BITS:
		val = OUT_BIT_16;
		break;
	default:
		val = OUT_BIT_8;
		break;
	}
	mtk_dpintf_mask(dpintf, DPI_OUTPUT_SETTING, val << OUT_BIT,
		     OUT_BIT_MASK);
}

static void mtk_dpintf_config_yc_map(struct mtk_dpintf *dpintf,
				  enum mtk_dpintf_out_yc_map map)
{
	u32 val;

	switch (map) {
	case MTK_DPINTF_OUT_YC_MAP_RGB:
		val = YC_MAP_RGB;
		break;
	case MTK_DPINTF_OUT_YC_MAP_CYCY:
		val = YC_MAP_CYCY;
		break;
	case MTK_DPINTF_OUT_YC_MAP_YCYC:
		val = YC_MAP_YCYC;
		break;
	case MTK_DPINTF_OUT_YC_MAP_CY:
		val = YC_MAP_CY;
		break;
	case MTK_DPINTF_OUT_YC_MAP_YC:
		val = YC_MAP_YC;
		break;
	default:
		val = YC_MAP_RGB;
		break;
	}

	mtk_dpintf_mask(dpintf, DPI_OUTPUT_SETTING, val << YC_MAP, YC_MAP_MASK);
}

static void mtk_dpintf_config_channel_swap(struct mtk_dpintf *dpintf,
					enum mtk_dpintf_out_channel_swap swap)
{
	u32 val;

	switch (swap) {
	case MTK_DPINTF_OUT_CHANNEL_SWAP_RGB:
		val = SWAP_RGB;
		break;
	case MTK_DPINTF_OUT_CHANNEL_SWAP_GBR:
		val = SWAP_GBR;
		break;
	case MTK_DPINTF_OUT_CHANNEL_SWAP_BRG:
		val = SWAP_BRG;
		break;
	case MTK_DPINTF_OUT_CHANNEL_SWAP_RBG:
		val = SWAP_RBG;
		break;
	case MTK_DPINTF_OUT_CHANNEL_SWAP_GRB:
		val = SWAP_GRB;
		break;
	case MTK_DPINTF_OUT_CHANNEL_SWAP_BGR:
		val = SWAP_BGR;
		break;
	default:
		val = SWAP_RGB;
		break;
	}

	mtk_dpintf_mask(dpintf, DPI_OUTPUT_SETTING, val << dpintf->conf->channel_swap_shift,
		     CH_SWAP_MASK << dpintf->conf->channel_swap_shift);
}

static void mtk_dpintf_config_yuv422_enable(struct mtk_dpintf *dpintf, bool enable)
{
	mtk_dpintf_mask(dpintf, DPI_CON, enable ? dpintf->conf->yuv422_en_bit : 0,
		     dpintf->conf->yuv422_en_bit);
}

static void mtk_dpintf_config_csc_enable(struct mtk_dpintf *dpintf, bool enable)
{
	mtk_dpintf_mask(dpintf, DPI_CON, enable ? dpintf->conf->csc_enable_bit : 0,
		dpintf->conf->csc_enable_bit);
}

static void mtk_dpintf_config_swap_input(struct mtk_dpintf *dpintf, bool enable)
{
	mtk_dpintf_mask(dpintf, DPI_CON, enable ? IN_RB_SWAP : 0, IN_RB_SWAP);
}

static void mtk_dpintf_config_2n_h_fre(struct mtk_dpintf *dpintf)
{
	mtk_dpintf_mask(dpintf, dpintf->conf->reg_h_fre_con, H_FRE_2N, H_FRE_2N);
}

static void mtk_dpintf_config_disable_edge(struct mtk_dpintf *dpintf)
{
	if (dpintf->conf->edge_sel_en)
		mtk_dpintf_mask(dpintf, dpintf->conf->reg_h_fre_con, 0, EDGE_SEL_EN);
}

static void mtk_dpintf_matrix_sel(struct mtk_dpintf *dpintf,
	enum mtk_dpintf_out_color_format format)
{
	u32 matrix_sel = 0;

	if (dpintf->conf->input_fmts &&
	    (dpintf->conf->input_fmts[0] == MEDIA_BUS_FMT_YUV8_1X24)) {
		switch (format) {
		case MTK_DPINTF_COLOR_FORMAT_RGB:
		case MTK_DPINTF_COLOR_FORMAT_RGB_FULL:
			matrix_sel = 0x7;
			break;
		default:
			break;
		}
	} else {
		switch (format) {
		case MTK_DPINTF_COLOR_FORMAT_YCBCR_422:
		case MTK_DPINTF_COLOR_FORMAT_YCBCR_422_FULL:
		case MTK_DPINTF_COLOR_FORMAT_YCBCR_444:
		case MTK_DPINTF_COLOR_FORMAT_YCBCR_444_FULL:
		case MTK_DPINTF_COLOR_FORMAT_XV_YCC:
			if (dpintf->mode.hdisplay <= 720)
				matrix_sel = 0x2;
			break;
		default:
			break;
		}
	}
	mtk_dpintf_mask(dpintf, DPI_MATRIX_SET, matrix_sel, INT_MATRIX_SEL_MASK);
}

static void mtk_dpintf_config_color_format(struct mtk_dpintf *dpintf,
					enum mtk_dpintf_out_color_format format)
{
	if (dpintf->conf->input_fmts &&
	    (dpintf->conf->input_fmts[0] == MEDIA_BUS_FMT_YUV8_1X24)) {
		if ((format == MTK_DPINTF_COLOR_FORMAT_RGB) ||
		    (format == MTK_DPINTF_COLOR_FORMAT_RGB_FULL)) {
			mtk_dpintf_config_yuv422_enable(dpintf, false);
			if (dpintf->conf->csc_support) {
				mtk_dpintf_config_csc_enable(dpintf, true);
				mtk_dpintf_matrix_sel(dpintf, format);
			}
			if (dpintf->conf->swap_input_support)
				mtk_dpintf_config_swap_input(dpintf, false);
			mtk_dpintf_config_channel_swap(dpintf, MTK_DPINTF_OUT_CHANNEL_SWAP_GBR);
		} else {
			mtk_dpintf_config_yuv422_enable(dpintf, false);
			if (dpintf->conf->csc_support)
				mtk_dpintf_config_csc_enable(dpintf, false);
			if (dpintf->conf->swap_input_support)
				mtk_dpintf_config_swap_input(dpintf, false);
			mtk_dpintf_config_channel_swap(dpintf, MTK_DPINTF_OUT_CHANNEL_SWAP_RGB);
		}

	} else {
		if ((format == MTK_DPINTF_COLOR_FORMAT_YCBCR_444) ||
		    (format == MTK_DPINTF_COLOR_FORMAT_YCBCR_444_FULL)) {
			mtk_dpintf_config_yuv422_enable(dpintf, false);
			if (dpintf->conf->csc_support) {
				mtk_dpintf_config_csc_enable(dpintf, true);
				mtk_dpintf_matrix_sel(dpintf, format);
			}
			if (dpintf->conf->swap_input_support)
				mtk_dpintf_config_swap_input(dpintf, false);
			mtk_dpintf_config_channel_swap(dpintf, MTK_DPINTF_OUT_CHANNEL_SWAP_BGR);
		} else if ((format == MTK_DPINTF_COLOR_FORMAT_YCBCR_422) ||
			   (format == MTK_DPINTF_COLOR_FORMAT_YCBCR_422_FULL)) {
			mtk_dpintf_config_yuv422_enable(dpintf, true);
			if (dpintf->conf->csc_support) {
				mtk_dpintf_config_csc_enable(dpintf, true);
				mtk_dpintf_matrix_sel(dpintf, format);
			}
			if (dpintf->conf->swap_input_support)
				mtk_dpintf_config_swap_input(dpintf, true);
			mtk_dpintf_config_channel_swap(dpintf, MTK_DPINTF_OUT_CHANNEL_SWAP_RGB);
		} else {
			mtk_dpintf_config_yuv422_enable(dpintf, false);
			if (dpintf->conf->csc_support)
				mtk_dpintf_config_csc_enable(dpintf, false);
			if (dpintf->conf->swap_input_support)
				mtk_dpintf_config_swap_input(dpintf, false);
			mtk_dpintf_config_channel_swap(dpintf, MTK_DPINTF_OUT_CHANNEL_SWAP_RGB);
		}
	}
}

static void mtk_dpintf_dual_edge(struct mtk_dpintf *dpintf)
{
	if ((dpintf->output_fmt == MEDIA_BUS_FMT_RGB888_2X12_LE) ||
	    (dpintf->output_fmt == MEDIA_BUS_FMT_RGB888_2X12_BE)) {
		mtk_dpintf_mask(dpintf, DPI_DDR_SETTING, DDR_EN | DDR_4PHASE,
			     DDR_EN | DDR_4PHASE);
		mtk_dpintf_mask(dpintf, DPI_OUTPUT_SETTING,
			     dpintf->output_fmt == MEDIA_BUS_FMT_RGB888_2X12_LE ?
			     EDGE_SEL : 0, EDGE_SEL);
	} else {
		mtk_dpintf_mask(dpintf, DPI_DDR_SETTING, DDR_EN | DDR_4PHASE, 0);
	}
}

static void mtk_dpintf_power_off(struct mtk_dpintf *dpintf)
{
	if (WARN_ON(dpintf->refcount == 0))
		return;

	if (--dpintf->refcount != 0)
		return;

	if (dpintf->pinctrl && dpintf->pins_gpio)
		pinctrl_select_state(dpintf->pinctrl, dpintf->pins_gpio);

	mtk_dpintf_disable(dpintf);
	clk_disable_unprepare(dpintf->pixel_clk);
	clk_disable_unprepare(dpintf->engine_clk);
	clk_disable_unprepare(dpintf->dpi_ck_cg);
	clk_disable_unprepare(dpintf->tvd_clk);
}

static int mtk_dpintf_power_on(struct mtk_dpintf *dpintf)
{
	int ret;

	if (++dpintf->refcount != 1)
		return 0;

	ret = clk_prepare_enable(dpintf->tvd_clk);
	if (ret) {
		dev_err(dpintf->dev, "Failed to tvd pll: %d\n", ret);
		goto err_pixel;
	}

	ret = clk_prepare_enable(dpintf->engine_clk);
	if (ret) {
		dev_err(dpintf->dev, "Failed to enable engine clock: %d\n", ret);
		goto err_refcount;
	}

	ret = clk_prepare_enable(dpintf->dpi_ck_cg);
	if (ret) {
		dev_err(dpintf->dev, "Failed to enable dpi_ck_cg clock: %d\n", ret);
		goto err_ck_cg;
	}

	ret = clk_prepare_enable(dpintf->pixel_clk);
	if (ret) {
		dev_err(dpintf->dev, "Failed to enable pixel clock: %d\n", ret);
		goto err_pixel;
	}

	if (dpintf->pinctrl && dpintf->pins_dpi)
		pinctrl_select_state(dpintf->pinctrl, dpintf->pins_dpi);

	return 0;

err_pixel:
	clk_disable_unprepare(dpintf->dpi_ck_cg);
err_ck_cg:
	clk_disable_unprepare(dpintf->engine_clk);
err_refcount:
	dpintf->refcount--;
	return ret;
}

static int mtk_dpintf_set_display_mode(struct mtk_dpintf *dpintf,
				    struct drm_display_mode *mode)
{
	struct mtk_dpintf_polarities dpi_pol;
	struct mtk_dpintf_sync_param hsync;
	struct mtk_dpintf_sync_param vsync_lodd = { 0 };
	struct mtk_dpintf_sync_param vsync_leven = { 0 };
	struct mtk_dpintf_sync_param vsync_rodd = { 0 };
	struct mtk_dpintf_sync_param vsync_reven = { 0 };
	struct videomode vm = { 0 };
	unsigned long pll_rate;
	unsigned int factor;

	/* let pll_rate can fix the valid range of tvdpll (1G~2GHz) */
	factor = dpintf->conf->cal_factor(mode->clock);
	drm_display_mode_to_videomode(mode, &vm);
	pll_rate = vm.pixelclock * factor;

	dev_dbg(dpintf->dev, "Want PLL %lu Hz, pixel clock %lu Hz\n",
		pll_rate, vm.pixelclock);

	clk_set_rate(dpintf->tvd_clk, pll_rate);
	pll_rate = clk_get_rate(dpintf->tvd_clk);

	vm.pixelclock = pll_rate / factor;
	if (dpintf->conf->is_dpintf) {
		//clk_set_rate(dpintf->pixel_clk, vm.pixelclock / 4);
		if (factor == 1)
			clk_set_parent(dpintf->pixel_clk, dpintf->pclk_src[2]);
		else if (factor == 2)
			clk_set_parent(dpintf->pixel_clk, dpintf->pclk_src[3]);
		else if (factor == 4)
			clk_set_parent(dpintf->pixel_clk, dpintf->pclk_src[4]);
		else
			clk_set_parent(dpintf->pixel_clk, dpintf->pclk_src[2]);
	} else if ((dpintf->output_fmt == MEDIA_BUS_FMT_RGB888_2X12_LE) ||
		 (dpintf->output_fmt == MEDIA_BUS_FMT_RGB888_2X12_BE))
		clk_set_rate(dpintf->pixel_clk, vm.pixelclock * 2);
	else
		clk_set_rate(dpintf->pixel_clk, vm.pixelclock);

	vm.pixelclock = clk_get_rate(dpintf->pixel_clk);

	dev_dbg(dpintf->dev, "Got  PLL %lu Hz, pixel clock %lu Hz\n",
		pll_rate, vm.pixelclock);

	dpi_pol.ck_pol = MTK_DPINTF_POLARITY_FALLING;
	dpi_pol.de_pol = MTK_DPINTF_POLARITY_RISING;
	dpi_pol.hsync_pol = vm.flags & DISPLAY_FLAGS_HSYNC_HIGH ?
			    MTK_DPINTF_POLARITY_FALLING : MTK_DPINTF_POLARITY_RISING;
	dpi_pol.vsync_pol = vm.flags & DISPLAY_FLAGS_VSYNC_HIGH ?
			    MTK_DPINTF_POLARITY_FALLING : MTK_DPINTF_POLARITY_RISING;
	if (dpintf->conf->is_dpintf) {
		hsync.sync_width = vm.hsync_len / 4;
		hsync.back_porch = vm.hback_porch / 4;
		hsync.front_porch = vm.hfront_porch / 4;
	} else {
		hsync.sync_width = vm.hsync_len;
		hsync.back_porch = vm.hback_porch;
		hsync.front_porch = vm.hfront_porch;
	}
	hsync.shift_half_line = false;
	vsync_lodd.sync_width = vm.vsync_len;
	vsync_lodd.back_porch = vm.vback_porch;
	vsync_lodd.front_porch = vm.vfront_porch;
	vsync_lodd.shift_half_line = false;

	if (vm.flags & DISPLAY_FLAGS_INTERLACED &&
	    mode->flags & DRM_MODE_FLAG_3D_MASK) {
		vsync_leven = vsync_lodd;
		vsync_rodd = vsync_lodd;
		vsync_reven = vsync_lodd;
		vsync_leven.shift_half_line = true;
		vsync_reven.shift_half_line = true;
	} else if (vm.flags & DISPLAY_FLAGS_INTERLACED &&
		   !(mode->flags & DRM_MODE_FLAG_3D_MASK)) {
		vsync_leven = vsync_lodd;
		vsync_leven.shift_half_line = true;
	} else if (!(vm.flags & DISPLAY_FLAGS_INTERLACED) &&
		   mode->flags & DRM_MODE_FLAG_3D_MASK) {
		vsync_rodd = vsync_lodd;
	}
	mtk_dpintf_sw_reset(dpintf, true);
	mtk_dpintf_config_pol(dpintf, &dpi_pol);

	mtk_dpintf_config_hsync(dpintf, &hsync);
	mtk_dpintf_config_vsync_lodd(dpintf, &vsync_lodd);
	mtk_dpintf_config_vsync_rodd(dpintf, &vsync_rodd);
	mtk_dpintf_config_vsync_leven(dpintf, &vsync_leven);
	mtk_dpintf_config_vsync_reven(dpintf, &vsync_reven);

	mtk_dpintf_config_3d(dpintf, !!(mode->flags & DRM_MODE_FLAG_3D_MASK));
	mtk_dpintf_config_interface(dpintf, !!(vm.flags &
					 DISPLAY_FLAGS_INTERLACED));
	if (vm.flags & DISPLAY_FLAGS_INTERLACED)
		mtk_dpintf_config_fb_size(dpintf, vm.hactive, vm.vactive >> 1);
	else
		mtk_dpintf_config_fb_size(dpintf, vm.hactive, vm.vactive);

	mtk_dpintf_config_channel_limit(dpintf);
	mtk_dpintf_config_bit_num(dpintf, dpintf->bit_num);
	mtk_dpintf_config_channel_swap(dpintf, dpintf->channel_swap);
	mtk_dpintf_config_color_format(dpintf, dpintf->color_format);
	if (dpintf->conf->is_dpintf) {
		mtk_dpintf_mask(dpintf, DPI_CON, DPINTF_INPUT_2P_EN,
			     DPINTF_INPUT_2P_EN);
	} else {
		mtk_dpintf_config_yc_map(dpintf, dpintf->yc_map);
		mtk_dpintf_config_2n_h_fre(dpintf);
		mtk_dpintf_dual_edge(dpintf);
		mtk_dpintf_config_disable_edge(dpintf);
	}
	mtk_dpintf_sw_reset(dpintf, false);

	mtk_dpintf_enable(dpintf);

	return 0;
}

static u32 *mtk_dpintf_bridge_atomic_get_output_bus_fmts(struct drm_bridge *bridge,
						      struct drm_bridge_state *bridge_state,
						      struct drm_crtc_state *crtc_state,
						      struct drm_connector_state *conn_state,
						      unsigned int *num_output_fmts)
{
	struct mtk_dpintf *dpintf = bridge_to_dpi(bridge);
	u32 *output_fmts;

	*num_output_fmts = 0;

	if (!dpintf->conf->output_fmts) {
		dev_err(dpintf->dev, "output_fmts should not be null\n");
		return NULL;
	}

	output_fmts = kcalloc(dpintf->conf->num_output_fmts, sizeof(*output_fmts),
			     GFP_KERNEL);
	if (!output_fmts)
		return NULL;

	*num_output_fmts = dpintf->conf->num_output_fmts;

	memcpy(output_fmts, dpintf->conf->output_fmts,
	       sizeof(*output_fmts) * dpintf->conf->num_output_fmts);

	return output_fmts;
}

static u32 *mtk_dpintf_bridge_atomic_get_input_bus_fmts(struct drm_bridge *bridge,
						     struct drm_bridge_state *bridge_state,
						     struct drm_crtc_state *crtc_state,
						     struct drm_connector_state *conn_state,
						     u32 output_fmt,
						     unsigned int *num_input_fmts)
{
	u32 *input_fmts;
	struct mtk_dpintf *dpintf = bridge_to_dpi(bridge);

	*num_input_fmts = 0;
	input_fmts = kcalloc(1, sizeof(*input_fmts),
			     GFP_KERNEL);
	if (!input_fmts)
		return NULL;

	*num_input_fmts = 1;
	input_fmts[0] = dpintf->conf->input_fmts[0];

	return input_fmts;
}

static int mtk_dpintf_bridge_atomic_check(struct drm_bridge *bridge,
				       struct drm_bridge_state *bridge_state,
				       struct drm_crtc_state *crtc_state,
				       struct drm_connector_state *conn_state)
{
	struct mtk_dpintf *dpintf = bridge_to_dpi(bridge);
	unsigned int out_bus_format;

	out_bus_format = bridge_state->output_bus_cfg.format;

	if (out_bus_format == MEDIA_BUS_FMT_FIXED)
		if (dpintf->conf->num_output_fmts)
			out_bus_format = dpintf->conf->output_fmts[0];

	dev_info(dpintf->dev, "input format 0x%04x, output format 0x%04x\n",
		bridge_state->input_bus_cfg.format,
		bridge_state->output_bus_cfg.format);

	dpintf->output_fmt = out_bus_format;
	dpintf->bit_num = MTK_DPINTF_OUT_BIT_NUM_8BITS;
	dpintf->channel_swap = MTK_DPINTF_OUT_CHANNEL_SWAP_RGB;
	dpintf->yc_map = MTK_DPINTF_OUT_YC_MAP_RGB;
	if (out_bus_format == MEDIA_BUS_FMT_YUYV8_1X16)
		dpintf->color_format = MTK_DPINTF_COLOR_FORMAT_YCBCR_422_FULL;
	else
		dpintf->color_format = MTK_DPINTF_COLOR_FORMAT_RGB;

	return 0;
}

static int mtk_dpintf_bridge_attach(struct drm_bridge *bridge,
				 enum drm_bridge_attach_flags flags)
{
	struct mtk_dpintf *dpintf = bridge_to_dpi(bridge);

	return drm_bridge_attach(bridge->encoder, dpintf->next_bridge,
				 &dpintf->bridge, flags);
}

static void mtk_dpintf_bridge_mode_set(struct drm_bridge *bridge,
				const struct drm_display_mode *mode,
				const struct drm_display_mode *adjusted_mode)
{
	struct mtk_dpintf *dpintf = bridge_to_dpi(bridge);

	drm_mode_copy(&dpintf->mode, adjusted_mode);
}

static void mtk_dpintf_bridge_disable(struct drm_bridge *bridge)
{
	struct mtk_dpintf *dpintf = bridge_to_dpi(bridge);

	mtk_dpintf_power_off(dpintf);
}

static void mtk_dpintf_bridge_enable(struct drm_bridge *bridge)
{
	struct mtk_dpintf *dpintf = bridge_to_dpi(bridge);

	mtk_dpintf_power_on(dpintf);
	mtk_dpintf_set_display_mode(dpintf, &dpintf->mode);
}

static enum drm_mode_status
mtk_dpintf_bridge_mode_valid(struct drm_bridge *bridge,
			  const struct drm_display_info *info,
			  const struct drm_display_mode *mode)
{
	struct mtk_dpintf *dpintf = bridge_to_dpi(bridge);

	if (dpintf->conf->max_clock_khz && mode->clock > dpintf->conf->max_clock_khz)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static const struct drm_bridge_funcs mtk_dpintf_bridge_funcs = {
	.attach = mtk_dpintf_bridge_attach,
	.mode_set = mtk_dpintf_bridge_mode_set,
	.mode_valid = mtk_dpintf_bridge_mode_valid,
	.disable = mtk_dpintf_bridge_disable,
	.enable = mtk_dpintf_bridge_enable,
	.atomic_check = mtk_dpintf_bridge_atomic_check,
	.atomic_get_output_bus_fmts = mtk_dpintf_bridge_atomic_get_output_bus_fmts,
	.atomic_get_input_bus_fmts = mtk_dpintf_bridge_atomic_get_input_bus_fmts,
	.atomic_duplicate_state = drm_atomic_helper_bridge_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_bridge_destroy_state,
	.atomic_reset = drm_atomic_helper_bridge_reset,
};

void mtk_dpintf_start(struct mtk_ddp_comp *comp, struct cmdq_pkt *handle)
{
	struct mtk_dpintf *dpintf = container_of(comp, struct mtk_dpintf, ddp_comp);

	mtk_dpintf_power_on(dpintf);
}

void mtk_dpintf_stop(struct mtk_ddp_comp *comp, struct cmdq_pkt *handle)
{
	struct mtk_dpintf *dpintf = container_of(comp, struct mtk_dpintf, ddp_comp);

	mtk_dpintf_power_off(dpintf);
}

static int mtk_dpintf_io_cmd(struct mtk_ddp_comp *comp, struct cmdq_pkt *handle,
			  enum mtk_ddp_io_cmd cmd, void *params)
{
	struct mtk_dpintf *dpintf = container_of(comp, struct mtk_dpintf, ddp_comp);
	struct videomode vm = { 0 };

	drm_display_mode_to_videomode(&dpintf->mode, &vm);

	switch (cmd) {
	case SET_MMCLK_BY_DATARATE:
	{
#ifndef CONFIG_FPGA_EARLY_PORTING
		struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
		unsigned int pixelclock = vm.pixelclock / 1000000;

		mtk_drm_set_mmclk_by_pixclk(&(mtk_crtc->base),
			pixelclock, __func__);
#endif
	}
		break;
	default:
		break;
	}
	return 0;
}

static const struct mtk_ddp_comp_funcs mtk_dpintf_funcs = {
	.start = mtk_dpintf_start,
	.stop = mtk_dpintf_stop,
	.io_cmd = mtk_dpintf_io_cmd,
};

static int mtk_dpintf_bind(struct device *dev, struct device *master, void *data)
{
	struct mtk_dpintf *dpintf = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	int ret;

	ret = mtk_ddp_comp_register(drm_dev, &dpintf->ddp_comp);
	if (ret < 0) {
		dev_err(dev, "Failed to register component %s: %d\n",
			dev->of_node->full_name, ret);
		return ret;
	}

	ret = drm_simple_encoder_init(drm_dev, &dpintf->encoder,
				      DRM_MODE_ENCODER_TMDS);
	if (ret) {
		dev_err(dev, "Failed to initialize decoder: %d\n", ret);
		return ret;
	}

	dpintf->encoder.possible_crtcs =
		mtk_drm_find_possible_crtc_by_comp(drm_dev, dpintf->ddp_comp);

	ret = drm_bridge_attach(&dpintf->encoder, &dpintf->bridge, NULL,
				DRM_BRIDGE_ATTACH_NO_CONNECTOR);
	if (ret) {
		dev_err(dev, "Failed to attach bridge: %d\n", ret);
		goto err_cleanup;
	}

	dpintf->connector = drm_bridge_connector_init(drm_dev, &dpintf->encoder);
	if (IS_ERR(dpintf->connector)) {
		dev_err(dev, "Unable to create bridge connector\n");
		ret = PTR_ERR(dpintf->connector);
		goto err_cleanup;
	}
	drm_connector_attach_encoder(dpintf->connector, &dpintf->encoder);

	return 0;

err_cleanup:
	drm_encoder_cleanup(&dpintf->encoder);
	return ret;
}

static void mtk_dpintf_unbind(struct device *dev, struct device *master,
			   void *data)
{
	struct mtk_dpintf *dpintf = dev_get_drvdata(dev);

	drm_encoder_cleanup(&dpintf->encoder);
}

static const struct component_ops mtk_dpintf_component_ops = {
	.bind = mtk_dpintf_bind,
	.unbind = mtk_dpintf_unbind,
};

static unsigned int mt8195_dpintf_calculate_factor(int clock)
{
	if (clock < 70000)
		return 4;
	else if (clock < 200000)
		return 2;
	else
		return 1;
}

static const u32 mt8195_output_fmts[] = {
	MEDIA_BUS_FMT_RGB888_1X24,
	MEDIA_BUS_FMT_YUV8_1X24,
	MEDIA_BUS_FMT_YUYV8_1X16,
};

static const u32 mt8195_input_fmts[] = {
	MEDIA_BUS_FMT_YUV8_1X24,
};
static const struct mtk_dpintf_yc_limit mtk_dpintf_limit = {
	.c_bottom = 0x0010,
	.c_top = 0x0FE0,
	.y_bottom = 0x0010,
	.y_top = 0x0FE0,
};

static const struct mtk_dpintf_yc_limit mtk_dpintfntf_limit = {
	.c_bottom = 0x0000,
	.c_top = 0xFFF,
	.y_bottom = 0x0000,
	.y_top = 0xFFF,
};

static const struct mtk_dpintf_conf mt8195_dpintf_conf = {
	.cal_factor = mt8195_dpintf_calculate_factor,
	.output_fmts = mt8195_output_fmts,
	.num_output_fmts = ARRAY_SIZE(mt8195_output_fmts),
	.input_fmts = mt8195_input_fmts,
	.num_input_fmts = ARRAY_SIZE(mt8195_input_fmts),
	.is_dpintf = true,
	.csc_support = true,
	.dimension_mask = DPINTF_HPW_MASK,
	.hvsize_mask = DPINTF_HSIZE_MASK,
	.channel_swap_shift = DPINTF_CH_SWAP,
	.yuv422_en_bit = DPINTF_YUV422_EN,
	.csc_enable_bit = DPINTF_CSC_ENABLE,
	.limit = &mtk_dpintfntf_limit,
};

static int mtk_dpintf_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_dpintf *dpintf;
	struct resource *mem;
	int ret;
	int comp_id;

	dpintf = devm_kzalloc(dev, sizeof(*dpintf), GFP_KERNEL);
	if (!dpintf)
		return -ENOMEM;

	dpintf->dev = dev;
	dpintf->conf = (struct mtk_dpintf_conf *)of_device_get_match_data(dev);
	dpintf->output_fmt = MEDIA_BUS_FMT_RGB888_1X24;

	dpintf->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(dpintf->pinctrl)) {
		dpintf->pinctrl = NULL;
		dev_dbg(&pdev->dev, "Cannot find pinctrl!\n");
	}
	if (dpintf->pinctrl) {
		dpintf->pins_gpio = pinctrl_lookup_state(dpintf->pinctrl, "sleep");
		if (IS_ERR(dpintf->pins_gpio)) {
			dpintf->pins_gpio = NULL;
			dev_dbg(&pdev->dev, "Cannot find pinctrl idle!\n");
		}
		if (dpintf->pins_gpio)
			pinctrl_select_state(dpintf->pinctrl, dpintf->pins_gpio);

		dpintf->pins_dpi = pinctrl_lookup_state(dpintf->pinctrl, "default");
		if (IS_ERR(dpintf->pins_dpi)) {
			dpintf->pins_dpi = NULL;
			dev_dbg(&pdev->dev, "Cannot find pinctrl active!\n");
		}
	}
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dpintf->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(dpintf->regs)) {
		ret = PTR_ERR(dpintf->regs);
		dev_err(dev, "Failed to ioremap mem resource: %d\n", ret);
		return ret;
	}

	dpintf->engine_clk = devm_clk_get(dev, "engine");
	if (IS_ERR(dpintf->engine_clk)) {
		ret = PTR_ERR(dpintf->engine_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get engine clock: %d\n",
				ret);

		return ret;
	}

	dpintf->dpi_ck_cg = devm_clk_get(dev, "ck_cg");
	if (IS_ERR(dpintf->dpi_ck_cg)) {
		ret = PTR_ERR(dpintf->dpi_ck_cg);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get dpintf ck cg clock: %d\n",
				ret);

		return ret;
	}

	dpintf->pixel_clk = devm_clk_get(dev, "pixel");
	if (IS_ERR(dpintf->pixel_clk)) {
		ret = PTR_ERR(dpintf->pixel_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get pixel clock: %d\n", ret);

		return ret;
	}

	dpintf->tvd_clk = devm_clk_get(dev, "pll");
	if (IS_ERR(dpintf->tvd_clk)) {
		ret = PTR_ERR(dpintf->tvd_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get tvdpll clock: %d\n", ret);

		return ret;
	}

	dpintf->pclk_src[1] = devm_clk_get(dev, "TVDPLL_D2");
	dpintf->pclk_src[2] = devm_clk_get(dev, "TVDPLL_D4");
	dpintf->pclk_src[3] = devm_clk_get(dev, "TVDPLL_D8");
	dpintf->pclk_src[4] = devm_clk_get(dev, "TVDPLL_D16");

	dpintf->irq = platform_get_irq(pdev, 0);
	if (dpintf->irq <= 0)
		return -EINVAL;

	ret = drm_of_find_panel_or_bridge(dev->of_node, 0, 0,
					  NULL, &dpintf->next_bridge);
	if (ret)
		return ret;

	dev_info(dev, "Found bridge node: %pOF\n", dpintf->next_bridge->of_node);

	comp_id = mtk_ddp_comp_get_id(dev->of_node, MTK_DP_INTF);
	if (comp_id < 0) {
		dev_err(dev, "Failed to identify by alias: %d\n", comp_id);
		return comp_id;
	}

	ret = mtk_ddp_comp_init(dev, dev->of_node, &dpintf->ddp_comp, comp_id,
				&mtk_dpintf_funcs);
	if (ret) {
		dev_err(dev, "Failed to initialize component: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, dpintf);

	dpintf->bridge.funcs = &mtk_dpintf_bridge_funcs;
	dpintf->bridge.of_node = dev->of_node;
	dpintf->bridge.type = DRM_MODE_CONNECTOR_DPI;

	drm_bridge_add(&dpintf->bridge);

	ret = component_add(dev, &mtk_dpintf_component_ops);
	if (ret) {
		drm_bridge_remove(&dpintf->bridge);
		dev_err(dev, "Failed to add component: %d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_dpintf_remove(struct platform_device *pdev)
{
	struct mtk_dpintf *dpintf = platform_get_drvdata(pdev);

	component_del(&pdev->dev, &mtk_dpintf_component_ops);
	drm_bridge_remove(&dpintf->bridge);

	return 0;
}

static const struct of_device_id mtk_dpintf_of_ids[] = {
	{ .compatible = "mediatek,mt8188-dp_intf",
	  .data = &mt8195_dpintf_conf,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_dpintf_of_ids);

struct platform_driver mtk_dpintf_driver = {
	.probe = mtk_dpintf_probe,
	.remove = mtk_dpintf_remove,
	.driver = {
		.name = "mediatek-dpintf",
		.of_match_table = mtk_dpintf_of_ids,
	},
};
