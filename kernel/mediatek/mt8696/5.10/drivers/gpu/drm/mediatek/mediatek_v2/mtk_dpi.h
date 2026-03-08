/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_DPI__H__
#define __MTK_DPI__H__

#include "mtk_dpi_regs.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_crtc.h"
#include <drm/drm_bridge.h>
#include <drm/drm_bridge_connector.h>

enum mtk_dpi_out_bit_num {
	MTK_DPI_OUT_BIT_NUM_8BITS,
	MTK_DPI_OUT_BIT_NUM_10BITS,
	MTK_DPI_OUT_BIT_NUM_12BITS,
	MTK_DPI_OUT_BIT_NUM_16BITS
};

enum mtk_dpi_out_yc_map {
	MTK_DPI_OUT_YC_MAP_RGB,
	MTK_DPI_OUT_YC_MAP_CYCY,
	MTK_DPI_OUT_YC_MAP_YCYC,
	MTK_DPI_OUT_YC_MAP_CY,
	MTK_DPI_OUT_YC_MAP_YC
};

enum mtk_dpi_out_channel_swap {
	MTK_DPI_OUT_CHANNEL_SWAP_RGB,
	MTK_DPI_OUT_CHANNEL_SWAP_GBR,
	MTK_DPI_OUT_CHANNEL_SWAP_BRG,
	MTK_DPI_OUT_CHANNEL_SWAP_RBG,
	MTK_DPI_OUT_CHANNEL_SWAP_GRB,
	MTK_DPI_OUT_CHANNEL_SWAP_BGR
};

enum mtk_dpi_internal_matrix {
	MTK_DPI_INT_MATRIX_COLOR_FORMAT_RGB, /* full-range sRGB */
	MTK_DPI_INT_MATRIX_COLOR_FORMAT_JPEG, /* full-range BT601 */
	MTK_DPI_INT_MATRIX_COLOR_FORMAT_FULL709, /* full-range BT709 */
	MTK_DPI_INT_MATRIX_COLOR_FORMAT_BT601, /* limit-range BT601 */
	MTK_DPI_INT_MATRIX_COLOR_FORMAT_BT709, /* limit-range BT709 */
	MTK_DPI_INT_MATRIX_COLOR_FORMAT_2020LIMITED,
	MTK_DPI_INT_MATRIX_COLOR_FORMAT_2020FULL,
	MTK_DPI_INT_MATRIX_COLOR_FORMAT_CERGB /* limit-range sRGB */
};

enum mtk_dpi_out_color_format {
	MTK_DPI_COLOR_FORMAT_RGB,
	MTK_DPI_COLOR_FORMAT_RGB_FULL,
	MTK_DPI_COLOR_FORMAT_YCBCR_444,
	MTK_DPI_COLOR_FORMAT_YCBCR_422,
	MTK_DPI_COLOR_FORMAT_XV_YCC,
	MTK_DPI_COLOR_FORMAT_YCBCR_444_FULL,
	MTK_DPI_COLOR_FORMAT_YCBCR_422_FULL
};

struct mtk_dpi {
	struct mtk_ddp_comp ddp_comp;
	struct drm_encoder encoder;
	struct drm_bridge *bridge;
	void __iomem *regs;
	struct regmap *vdosys1_regmap;
	unsigned int vdosys1_offset;
	struct device *dev;
	struct clk *engine_clk;
	struct clk *pixel_clk;
	struct clk *tvd_clk;
	int irq;
	struct drm_display_mode mode;
	const struct mtk_dpi_conf *conf;
	enum mtk_dpi_out_color_format color_format;
	enum mtk_dpi_out_yc_map yc_map;
	enum mtk_dpi_out_bit_num bit_num;
	enum mtk_dpi_out_channel_swap channel_swap;
	enum mtk_dpi_internal_matrix in_format;
	enum mtk_dpi_internal_matrix out_format;
	bool power_sta;
	u8 power_ctl;
	bool bypass_matrix;
#ifdef CONFIG_LK_FASTLOGO
	bool lk_fastlogo;
#endif
};

enum mtk_dpi_polarity {
	MTK_DPI_POLARITY_RISING,
	MTK_DPI_POLARITY_FALLING,
};

enum mtk_dpi_power_ctl {
	DPI_POWER_START = BIT(0),
	DPI_POWER_ENABLE = BIT(1),
};

struct mtk_dpi_polarities {
	enum mtk_dpi_polarity de_pol;
	enum mtk_dpi_polarity ck_pol;
	enum mtk_dpi_polarity hsync_pol;
	enum mtk_dpi_polarity vsync_pol;
};

struct mtk_dpi_sync_param {
	u32 sync_width;
	u32 front_porch;
	u32 back_porch;
	bool shift_half_line;
};

struct mtk_dpi_yc_limit {
	u16 y_top;
	u16 y_bottom;
	u16 c_top;
	u16 c_bottom;
};

struct mtk_dpi_conf {
	unsigned int (*cal_factor)(int clock);
	const u32 reg_h_fre_con;
	bool vdosys1_sw_rst;
};

void mtk_dpi_mask(struct mtk_dpi *dpi, u32 offset, u32 val, u32 mask);
void mtk_dpi_sw_reset(struct mtk_dpi *dpi, bool reset);
void mtk_dpi_enable(struct mtk_dpi *dpi);
void mtk_dpi_disable(struct mtk_dpi *dpi);
void mtk_dpi_set_interrupt_enable(struct mtk_dpi *dpi);
void mtk_dpi_config_hsync(struct mtk_dpi *dpi,
			struct mtk_dpi_sync_param *sync);
void mtk_dpi_config_vsync(struct mtk_dpi *dpi,
			struct mtk_dpi_sync_param *sync,
			u32 width_addr, u32 porch_addr);
void mtk_dpi_config_vsync_lodd(struct mtk_dpi *dpi,
			struct mtk_dpi_sync_param *sync);
void mtk_dpi_config_vsync_leven(struct mtk_dpi *dpi,
			struct mtk_dpi_sync_param *sync);
void mtk_dpi_config_vsync_rodd(struct mtk_dpi *dpi,
			struct mtk_dpi_sync_param *sync);
void mtk_dpi_config_vsync_reven(struct mtk_dpi *dpi,
			struct mtk_dpi_sync_param *sync);
void mtk_dpi_config_pol(struct mtk_dpi *dpi,
			struct mtk_dpi_polarities *dpi_pol);
void mtk_dpi_config_3d(struct mtk_dpi *dpi, bool en_3d);
void mtk_dpi_config_interface(struct mtk_dpi *dpi, bool inter);
void mtk_dpi_config_input_2p(struct mtk_dpi *dpi, bool input_2p);
void mtk_dpi_config_output_1t1p(struct mtk_dpi *dpi, bool output_1t1p);
void mtk_dpi_config_fb_size(struct mtk_dpi *dpi, u32 width, u32 height);
void mtk_dpi_config_channel_limit(struct mtk_dpi *dpi,
			struct mtk_dpi_yc_limit *limit);
void mtk_dpi_config_bit_num(struct mtk_dpi *dpi,
			enum mtk_dpi_out_bit_num num);
void mtk_dpi_config_yc_map(struct mtk_dpi *dpi,
			enum mtk_dpi_out_yc_map map);
void mtk_dpi_config_channel_swap(struct mtk_dpi *dpi,
			enum mtk_dpi_out_channel_swap swap);
void mtk_dpi_config_yuv422_enable(struct mtk_dpi *dpi, bool enable);
void mtk_dpi_config_csc_enable(struct mtk_dpi *dpi, bool enable);
void mtk_dpi_config_swap_input(struct mtk_dpi *dpi, bool enable);
void mtk_dpi_config_2n_h_fre(struct mtk_dpi *dpi);
void mtk_dpi_config_ext_matrix_enable(struct mtk_dpi *dpi, bool enable);
void mtk_dpi_config_ext_matrix(struct mtk_dpi *dpi);
void mtk_dpi_config_matrix_in_offset(struct mtk_dpi *dpi);
void mtk_dpi_config_matrix_out_offset(struct mtk_dpi *dpi);
void mtk_dpi_config_color_format(struct mtk_dpi *dpi,
			enum mtk_dpi_out_color_format format);
void mtk_dpi_set_input_format(bool bypass_matrix,
			enum mtk_dpi_internal_matrix in_format);
void mtk_dpi_internal_matrix_sel(struct mtk_dpi *dpi);
void mtk_dpi_power_off(struct mtk_dpi *dpi, enum mtk_dpi_power_ctl pctl);
int mtk_dpi_power_on(struct mtk_dpi *dpi, enum mtk_dpi_power_ctl pctl);
int mtk_dpi_set_display_mode(struct mtk_dpi *dpi,
			struct drm_display_mode *mode);

#endif //__MTK_DPI__H__
