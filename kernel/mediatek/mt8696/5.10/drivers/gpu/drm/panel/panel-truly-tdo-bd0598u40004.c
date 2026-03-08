// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015 MediaTek Inc.
 */

#include <linux/backlight.h>
#include <linux/delay.h>
/* #include <drm/drmP.h> */
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/backlight.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_log.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#endif

/* enable this to check panel self -bist pattern */
/* #define PANEL_BIST_PATTERN */

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int bpc;

	/**
	 * @width_mm: width of the panel's active display area
	 * @height_mm: height of the panel's active display area
	 */
	struct {
		unsigned int width_mm;
		unsigned int height_mm;
	} size;

	unsigned long mode_flags;
	enum mipi_dsi_pixel_format format;
	const struct panel_init_cmd *init_cmds;
	unsigned int lanes;
};

struct truly {
	struct device *dev;
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	enum drm_panel_orientation orientation;
	const struct panel_desc *desc;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;

	bool prepared;
	bool enabled;

	int error;
};

enum dsi_cmd_type {
	INIT_DCS_CMD,
	DELAY_CMD,
};

struct panel_init_cmd {
	enum dsi_cmd_type type;
	size_t len;
	const char *data;
};

#define _INIT_DCS_CMD(...) { \
	.type = INIT_DCS_CMD, \
	.len = sizeof((char[]){__VA_ARGS__}), \
	.data = (char[]){__VA_ARGS__} }

#define _INIT_DELAY_CMD(...) { \
	.type = DELAY_CMD,\
	.len = sizeof((char[]){__VA_ARGS__}), \
	.data = (char[]){__VA_ARGS__} }

static inline struct truly *panel_to_truly(struct drm_panel *panel)
{
	return container_of(panel, struct truly, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int truly_dcs_read(struct truly *ctx, u8 cmd, void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return 0;

	ret = mipi_dsi_dcs_read(dsi, cmd, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %d reading dcs seq:(%#x)\n", ret, cmd);
		ctx->error = ret;
	}

	return ret;
}

static void truly_panel_get_data(struct truly *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = truly_dcs_read(ctx,  0x0A, buffer, 1);
		dev_err(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static int truly_panel_init_dcs_cmd(struct truly *truly)
{
	struct mipi_dsi_device *dsi = truly->dsi;
	struct drm_panel *panel = &truly->panel;
	int i, err = 0;

	if (truly->desc->init_cmds) {
		const struct panel_init_cmd *init_cmds = truly->desc->init_cmds;

		for (i = 0; init_cmds[i].len != 0; i++) {
			const struct panel_init_cmd *cmd = &init_cmds[i];

			switch (cmd->type) {
			case DELAY_CMD:
				msleep(cmd->data[0]);
				err = 0;
				break;

			case INIT_DCS_CMD:
				err = mipi_dsi_dcs_write(dsi, cmd->data[0],
							 cmd->len <= 1 ? NULL :
							 &cmd->data[1],
							 cmd->len - 1);
				break;

			default:
				err = -EINVAL;
			}

			if (err < 0) {
				dev_err(panel->dev,
					"failed to write command %u\n", i);
				return err;
			}
		}
	}
	return 0;
}

static const struct panel_init_cmd truly_init_cmd[] = {
	_INIT_DCS_CMD(0xFF, 0x10),
	_INIT_DELAY_CMD(2),

	_INIT_DCS_CMD(0xBB, 0x03),
	_INIT_DCS_CMD(0x3B, 0x03, 0x0A, 0x0A, 0x0A, 0x0A),
	_INIT_DCS_CMD(0x53, 0x24),
	_INIT_DCS_CMD(0x55, 0x00),
	_INIT_DCS_CMD(0x5E, 0x00),
#ifndef PANEL_BIST_PATTERN
	_INIT_DCS_CMD(MIPI_DCS_EXIT_SLEEP_MODE),
	_INIT_DELAY_CMD(150),
#endif
	_INIT_DCS_CMD(0xFF, 0x24),
	_INIT_DELAY_CMD(2),

	_INIT_DCS_CMD(0xFB, 0x01),
	_INIT_DCS_CMD(0x9D, 0xB0),
	_INIT_DCS_CMD(0x72, 0x00),
	_INIT_DCS_CMD(0x93, 0x04),
	_INIT_DCS_CMD(0x94, 0x04),
	_INIT_DCS_CMD(0x9B, 0x0F),
	_INIT_DCS_CMD(0x8A, 0x33),
	_INIT_DCS_CMD(0x86, 0x1B),
	_INIT_DCS_CMD(0x87, 0x39),
	_INIT_DCS_CMD(0x88, 0x1B),
	_INIT_DCS_CMD(0x89, 0x39),
	_INIT_DCS_CMD(0x8B, 0xF4),
	_INIT_DCS_CMD(0x8C, 0x01),
	_INIT_DCS_CMD(0x90, 0x79),
	_INIT_DCS_CMD(0x91, 0x4C),
	_INIT_DCS_CMD(0x92, 0x77),
	_INIT_DCS_CMD(0x95, 0xE4),
	_INIT_DCS_CMD(0xDE, 0xFF),
	_INIT_DCS_CMD(0xDF, 0x82),
	_INIT_DCS_CMD(0x00, 0x0F),
	_INIT_DCS_CMD(0x01, 0x00),
	_INIT_DCS_CMD(0x02, 0x00),
	_INIT_DCS_CMD(0x03, 0x00),
	_INIT_DCS_CMD(0x04, 0x0B),
	_INIT_DCS_CMD(0x05, 0x0C),
	_INIT_DCS_CMD(0x06, 0x00),
	_INIT_DCS_CMD(0x07, 0x00),
	_INIT_DCS_CMD(0x08, 0x00),
	_INIT_DCS_CMD(0x09, 0x00),
	_INIT_DCS_CMD(0x0A, 0x03),
	_INIT_DCS_CMD(0x0B, 0x04),
	_INIT_DCS_CMD(0x0C, 0x01),
	_INIT_DCS_CMD(0x0D, 0x13),
	_INIT_DCS_CMD(0x0E, 0x15),
	_INIT_DCS_CMD(0x0F, 0x17),
	_INIT_DCS_CMD(0x10, 0x0F),
	_INIT_DCS_CMD(0x11, 0x00),
	_INIT_DCS_CMD(0x12, 0x00),
	_INIT_DCS_CMD(0x13, 0x00),
	_INIT_DCS_CMD(0x14, 0x0B),
	_INIT_DCS_CMD(0x15, 0x0C),
	_INIT_DCS_CMD(0x16, 0x00),
	_INIT_DCS_CMD(0x17, 0x00),
	_INIT_DCS_CMD(0x18, 0x00),
	_INIT_DCS_CMD(0x19, 0x00),
	_INIT_DCS_CMD(0x1A, 0x03),
	_INIT_DCS_CMD(0x1B, 0x04),
	_INIT_DCS_CMD(0x1C, 0x01),
	_INIT_DCS_CMD(0x1D, 0x13),
	_INIT_DCS_CMD(0x1E, 0x15),
	_INIT_DCS_CMD(0x1F, 0x17),
	_INIT_DCS_CMD(0x20, 0x09),
	_INIT_DCS_CMD(0x21, 0x01),
	_INIT_DCS_CMD(0x22, 0x00),
	_INIT_DCS_CMD(0x23, 0x00),
	_INIT_DCS_CMD(0x24, 0x00),
	_INIT_DCS_CMD(0x25, 0x6D),
	_INIT_DCS_CMD(0x26, 0x00),
	_INIT_DCS_CMD(0x27, 0x00),
	_INIT_DCS_CMD(0x2F, 0x02),
	_INIT_DCS_CMD(0x30, 0x04),
	_INIT_DCS_CMD(0x31, 0x49),
	_INIT_DCS_CMD(0x32, 0x23),
	_INIT_DCS_CMD(0x33, 0x01),
	_INIT_DCS_CMD(0x34, 0x00),
	_INIT_DCS_CMD(0x35, 0x69),
	_INIT_DCS_CMD(0x36, 0x00),
	_INIT_DCS_CMD(0x37, 0x2D),
	_INIT_DCS_CMD(0x38, 0x08),
	_INIT_DCS_CMD(0x39, 0x00),
	_INIT_DCS_CMD(0x3A, 0x69),
	_INIT_DCS_CMD(0x29, 0x58),
	_INIT_DCS_CMD(0x2A, 0x16),
	_INIT_DCS_CMD(0x5B, 0x00),
	_INIT_DCS_CMD(0x5F, 0x75),
	_INIT_DCS_CMD(0x63, 0x00),
	_INIT_DCS_CMD(0x67, 0x04),
	_INIT_DCS_CMD(0x7B, 0x80),
	_INIT_DCS_CMD(0x7C, 0xD8),
	_INIT_DCS_CMD(0x7D, 0x60),
	_INIT_DCS_CMD(0x7E, 0x10),
	_INIT_DCS_CMD(0x7F, 0x19),
	_INIT_DCS_CMD(0x80, 0x00),
	_INIT_DCS_CMD(0x81, 0x06),
	_INIT_DCS_CMD(0x82, 0x03),
	_INIT_DCS_CMD(0x83, 0x00),
	_INIT_DCS_CMD(0x84, 0x03),
	_INIT_DCS_CMD(0x85, 0x07),
	_INIT_DCS_CMD(0x74, 0x10),
	_INIT_DCS_CMD(0x75, 0x19),
	_INIT_DCS_CMD(0x76, 0x06),
	_INIT_DCS_CMD(0x77, 0x03),
	_INIT_DCS_CMD(0x78, 0x00),
	_INIT_DCS_CMD(0x79, 0x00),
	_INIT_DCS_CMD(0x99, 0x33),
	_INIT_DCS_CMD(0x98, 0x00),
	_INIT_DCS_CMD(0xB3, 0x28),
	_INIT_DCS_CMD(0xB4, 0x05),
	_INIT_DCS_CMD(0xB5, 0x10),
	_INIT_DCS_CMD(0xFF, 0x20),
	_INIT_DELAY_CMD(2),

	_INIT_DCS_CMD(0x00, 0x01),
	_INIT_DCS_CMD(0x01, 0x55),
	_INIT_DCS_CMD(0x02, 0x45),
	_INIT_DCS_CMD(0x03, 0x55),
	_INIT_DCS_CMD(0x05, 0x50),
	_INIT_DCS_CMD(0x06, 0x9E),
	_INIT_DCS_CMD(0x07, 0xA8),
	_INIT_DCS_CMD(0x08, 0x0C),
	_INIT_DCS_CMD(0x0B, 0x96),
	_INIT_DCS_CMD(0x0C, 0x96),
	_INIT_DCS_CMD(0x0E, 0x00),
	_INIT_DCS_CMD(0x0F, 0x00),
	_INIT_DCS_CMD(0x11, 0x29),
	_INIT_DCS_CMD(0x12, 0x29),
	_INIT_DCS_CMD(0x13, 0x03),
	_INIT_DCS_CMD(0x14, 0x0A),
	_INIT_DCS_CMD(0x15, 0x99),
	_INIT_DCS_CMD(0x16, 0x99),
	_INIT_DCS_CMD(0x6D, 0x44),
	_INIT_DCS_CMD(0x58, 0x05),
	_INIT_DCS_CMD(0x59, 0x05),
	_INIT_DCS_CMD(0x5A, 0x05),
	_INIT_DCS_CMD(0x5B, 0x05),
	_INIT_DCS_CMD(0x5C, 0x00),
	_INIT_DCS_CMD(0x5D, 0x00),
	_INIT_DCS_CMD(0x5E, 0x00),
	_INIT_DCS_CMD(0x5F, 0x00),
	_INIT_DCS_CMD(0x1B, 0x39),
	_INIT_DCS_CMD(0x1C, 0x39),
	_INIT_DCS_CMD(0x1D, 0x47),
	_INIT_DCS_CMD(0xFF, 0x20),
	_INIT_DELAY_CMD(20),

	_INIT_DCS_CMD(0x75, 0x00),
	_INIT_DCS_CMD(0x76, 0x00),
	_INIT_DCS_CMD(0x77, 0x00),
	_INIT_DCS_CMD(0x78, 0x22),
	_INIT_DCS_CMD(0x79, 0x00),
	_INIT_DCS_CMD(0x7A, 0x46),
	_INIT_DCS_CMD(0x7B, 0x00),
	_INIT_DCS_CMD(0x7C, 0x5C),
	_INIT_DCS_CMD(0x7D, 0x00),
	_INIT_DCS_CMD(0x7E, 0x76),
	_INIT_DCS_CMD(0x7F, 0x00),
	_INIT_DCS_CMD(0x80, 0x8D),
	_INIT_DCS_CMD(0x81, 0x00),
	_INIT_DCS_CMD(0x82, 0xA6),
	_INIT_DCS_CMD(0x83, 0x00),
	_INIT_DCS_CMD(0x84, 0xB8),
	_INIT_DCS_CMD(0x85, 0x00),
	_INIT_DCS_CMD(0x86, 0xC7),
	_INIT_DCS_CMD(0x87, 0x00),
	_INIT_DCS_CMD(0x88, 0xF6),
	_INIT_DCS_CMD(0x89, 0x01),
	_INIT_DCS_CMD(0x8A, 0x1D),
	_INIT_DCS_CMD(0x8B, 0x01),
	_INIT_DCS_CMD(0x8C, 0x54),
	_INIT_DCS_CMD(0x8D, 0x01),
	_INIT_DCS_CMD(0x8E, 0x81),
	_INIT_DCS_CMD(0x8F, 0x01),
	_INIT_DCS_CMD(0x90, 0xCB),
	_INIT_DCS_CMD(0x91, 0x02),
	_INIT_DCS_CMD(0x92, 0x05),
	_INIT_DCS_CMD(0x93, 0x02),
	_INIT_DCS_CMD(0x94, 0x07),
	_INIT_DCS_CMD(0x95, 0x02),
	_INIT_DCS_CMD(0x96, 0x47),
	_INIT_DCS_CMD(0x97, 0x02),
	_INIT_DCS_CMD(0x98, 0x82),
	_INIT_DCS_CMD(0x99, 0x02),
	_INIT_DCS_CMD(0x9A, 0xAB),
	_INIT_DCS_CMD(0x9B, 0x02),
	_INIT_DCS_CMD(0x9C, 0xDC),
	_INIT_DCS_CMD(0x9D, 0x03),
	_INIT_DCS_CMD(0x9E, 0x01),
	_INIT_DCS_CMD(0x9F, 0x03),
	_INIT_DCS_CMD(0xA0, 0x3A),
	_INIT_DCS_CMD(0xA2, 0x03),
	_INIT_DCS_CMD(0xA3, 0x56),
	_INIT_DCS_CMD(0xA4, 0x03),
	_INIT_DCS_CMD(0xA5, 0x6D),
	_INIT_DCS_CMD(0xA6, 0x03),
	_INIT_DCS_CMD(0xA7, 0x89),
	_INIT_DCS_CMD(0xA9, 0x03),
	_INIT_DCS_CMD(0xAA, 0xA3),
	_INIT_DCS_CMD(0xAB, 0x03),
	_INIT_DCS_CMD(0xAC, 0xC9),
	_INIT_DCS_CMD(0xAD, 0x03),
	_INIT_DCS_CMD(0xAE, 0xDD),
	_INIT_DCS_CMD(0xAF, 0x03),
	_INIT_DCS_CMD(0xB0, 0xF5),
	_INIT_DCS_CMD(0xB1, 0x03),
	_INIT_DCS_CMD(0xB2, 0xFF),

	_INIT_DCS_CMD(0xB3, 0x00),
	_INIT_DCS_CMD(0xB4, 0x00),
	_INIT_DCS_CMD(0xB5, 0x00),
	_INIT_DCS_CMD(0xB6, 0x22),
	_INIT_DCS_CMD(0xB7, 0x00),
	_INIT_DCS_CMD(0xB8, 0x46),
	_INIT_DCS_CMD(0xB9, 0x00),
	_INIT_DCS_CMD(0xBA, 0x5C),
	_INIT_DCS_CMD(0xBB, 0x00),
	_INIT_DCS_CMD(0xBC, 0x76),
	_INIT_DCS_CMD(0xBD, 0x00),
	_INIT_DCS_CMD(0xBE, 0x8D),
	_INIT_DCS_CMD(0xBF, 0x00),
	_INIT_DCS_CMD(0xC0, 0xA6),
	_INIT_DCS_CMD(0xC1, 0x00),
	_INIT_DCS_CMD(0xC2, 0xB8),
	_INIT_DCS_CMD(0xC3, 0x00),
	_INIT_DCS_CMD(0xC4, 0xC7),
	_INIT_DCS_CMD(0xC5, 0x00),
	_INIT_DCS_CMD(0xC6, 0xF6),
	_INIT_DCS_CMD(0xC7, 0x01),
	_INIT_DCS_CMD(0xC8, 0x1D),
	_INIT_DCS_CMD(0xC9, 0x01),
	_INIT_DCS_CMD(0xCA, 0x54),
	_INIT_DCS_CMD(0xCB, 0x01),
	_INIT_DCS_CMD(0xCC, 0x81),
	_INIT_DCS_CMD(0xCD, 0x01),
	_INIT_DCS_CMD(0xCE, 0xCB),
	_INIT_DCS_CMD(0xCF, 0x02),
	_INIT_DCS_CMD(0xD0, 0x05),
	_INIT_DCS_CMD(0xD1, 0x02),
	_INIT_DCS_CMD(0xD2, 0x07),
	_INIT_DCS_CMD(0xD3, 0x02),
	_INIT_DCS_CMD(0xD4, 0x47),
	_INIT_DCS_CMD(0xD5, 0x02),
	_INIT_DCS_CMD(0xD6, 0x82),
	_INIT_DCS_CMD(0xD7, 0x02),
	_INIT_DCS_CMD(0xD8, 0xAB),
	_INIT_DCS_CMD(0xD9, 0x02),
	_INIT_DCS_CMD(0xDA, 0xDC),
	_INIT_DCS_CMD(0xDB, 0x03),
	_INIT_DCS_CMD(0xDC, 0x01),
	_INIT_DCS_CMD(0xDD, 0x03),
	_INIT_DCS_CMD(0xDE, 0x3A),
	_INIT_DCS_CMD(0xDF, 0x03),
	_INIT_DCS_CMD(0xE0, 0x56),
	_INIT_DCS_CMD(0xE1, 0x03),
	_INIT_DCS_CMD(0xE2, 0x6D),
	_INIT_DCS_CMD(0xE3, 0x03),
	_INIT_DCS_CMD(0xE4, 0x89),
	_INIT_DCS_CMD(0xE5, 0x03),
	_INIT_DCS_CMD(0xE6, 0xA3),
	_INIT_DCS_CMD(0xE7, 0x03),
	_INIT_DCS_CMD(0xE8, 0xC9),
	_INIT_DCS_CMD(0xE9, 0x03),
	_INIT_DCS_CMD(0xEA, 0xDD),
	_INIT_DCS_CMD(0xEB, 0x03),
	_INIT_DCS_CMD(0xEC, 0xF5),
	_INIT_DCS_CMD(0xED, 0x03),
	_INIT_DCS_CMD(0xEE, 0xFF),

	_INIT_DCS_CMD(0xEF, 0x00),
	_INIT_DCS_CMD(0xF0, 0x00),
	_INIT_DCS_CMD(0xF1, 0x00),
	_INIT_DCS_CMD(0xF2, 0x22),
	_INIT_DCS_CMD(0xF3, 0x00),
	_INIT_DCS_CMD(0xF4, 0x46),
	_INIT_DCS_CMD(0xF5, 0x00),
	_INIT_DCS_CMD(0xF6, 0x5C),
	_INIT_DCS_CMD(0xF7, 0x00),
	_INIT_DCS_CMD(0xF8, 0x76),
	_INIT_DCS_CMD(0xF9, 0x00),
	_INIT_DCS_CMD(0xFA, 0x8D),

	_INIT_DCS_CMD(0xFF, 0x21),
	_INIT_DELAY_CMD(20),

	_INIT_DCS_CMD(0x00, 0x00),
	_INIT_DCS_CMD(0x01, 0xA6),
	_INIT_DCS_CMD(0x02, 0x00),
	_INIT_DCS_CMD(0x03, 0xB8),
	_INIT_DCS_CMD(0x04, 0x00),
	_INIT_DCS_CMD(0x05, 0xC7),
	_INIT_DCS_CMD(0x06, 0x00),
	_INIT_DCS_CMD(0x07, 0xF6),
	_INIT_DCS_CMD(0x08, 0x01),
	_INIT_DCS_CMD(0x09, 0x1D),
	_INIT_DCS_CMD(0x0A, 0x01),
	_INIT_DCS_CMD(0x0B, 0x54),
	_INIT_DCS_CMD(0x0C, 0x01),
	_INIT_DCS_CMD(0x0D, 0x81),
	_INIT_DCS_CMD(0x0E, 0x01),
	_INIT_DCS_CMD(0x0F, 0xCB),
	_INIT_DCS_CMD(0x10, 0x02),
	_INIT_DCS_CMD(0x11, 0x05),
	_INIT_DCS_CMD(0x12, 0x02),
	_INIT_DCS_CMD(0x13, 0x07),
	_INIT_DCS_CMD(0x14, 0x02),
	_INIT_DCS_CMD(0x15, 0x47),
	_INIT_DCS_CMD(0x16, 0x02),
	_INIT_DCS_CMD(0x17, 0x82),
	_INIT_DCS_CMD(0x18, 0x02),
	_INIT_DCS_CMD(0x19, 0xAB),
	_INIT_DCS_CMD(0x1A, 0x02),
	_INIT_DCS_CMD(0x1B, 0xDC),
	_INIT_DCS_CMD(0x1C, 0x03),
	_INIT_DCS_CMD(0x1D, 0x01),
	_INIT_DCS_CMD(0x1E, 0x03),
	_INIT_DCS_CMD(0x1F, 0x3A),
	_INIT_DCS_CMD(0x20, 0x03),
	_INIT_DCS_CMD(0x21, 0x56),
	_INIT_DCS_CMD(0x22, 0x03),
	_INIT_DCS_CMD(0x23, 0x6D),
	_INIT_DCS_CMD(0x24, 0x03),
	_INIT_DCS_CMD(0x25, 0x89),
	_INIT_DCS_CMD(0x26, 0x03),
	_INIT_DCS_CMD(0x27, 0xA3),
	_INIT_DCS_CMD(0x28, 0x03),
	_INIT_DCS_CMD(0x29, 0xC9),
	_INIT_DCS_CMD(0x2A, 0x03),
	_INIT_DCS_CMD(0x2B, 0xDD),
	_INIT_DCS_CMD(0x2D, 0x03),
	_INIT_DCS_CMD(0x2F, 0xF5),
	_INIT_DCS_CMD(0x30, 0x03),
	_INIT_DCS_CMD(0x31, 0xFF),

	_INIT_DCS_CMD(0x32, 0x00),
	_INIT_DCS_CMD(0x33, 0x00),
	_INIT_DCS_CMD(0x34, 0x00),
	_INIT_DCS_CMD(0x35, 0x22),
	_INIT_DCS_CMD(0x36, 0x00),
	_INIT_DCS_CMD(0x37, 0x46),
	_INIT_DCS_CMD(0x38, 0x00),
	_INIT_DCS_CMD(0x39, 0x5C),
	_INIT_DCS_CMD(0x3A, 0x00),
	_INIT_DCS_CMD(0x3B, 0x76),
	_INIT_DCS_CMD(0x3D, 0x00),
	_INIT_DCS_CMD(0x3F, 0x8D),
	_INIT_DCS_CMD(0x40, 0x00),
	_INIT_DCS_CMD(0x41, 0xA6),
	_INIT_DCS_CMD(0x42, 0x00),
	_INIT_DCS_CMD(0x43, 0xB8),
	_INIT_DCS_CMD(0x44, 0x00),
	_INIT_DCS_CMD(0x45, 0xC7),
	_INIT_DCS_CMD(0x46, 0x00),
	_INIT_DCS_CMD(0x47, 0xF6),
	_INIT_DCS_CMD(0x48, 0x01),
	_INIT_DCS_CMD(0x49, 0x1D),
	_INIT_DCS_CMD(0x4A, 0x01),
	_INIT_DCS_CMD(0x4B, 0x54),
	_INIT_DCS_CMD(0x4C, 0x01),
	_INIT_DCS_CMD(0x4D, 0x81),
	_INIT_DCS_CMD(0x4E, 0x01),
	_INIT_DCS_CMD(0x4F, 0xCB),
	_INIT_DCS_CMD(0x50, 0x02),
	_INIT_DCS_CMD(0x51, 0x05),
	_INIT_DCS_CMD(0x52, 0x02),
	_INIT_DCS_CMD(0x53, 0x07),
	_INIT_DCS_CMD(0x54, 0x02),
	_INIT_DCS_CMD(0x55, 0x47),
	_INIT_DCS_CMD(0x56, 0x02),
	_INIT_DCS_CMD(0x58, 0x82),
	_INIT_DCS_CMD(0x59, 0x02),
	_INIT_DCS_CMD(0x5A, 0xAB),
	_INIT_DCS_CMD(0x5B, 0x02),
	_INIT_DCS_CMD(0x5C, 0xDC),
	_INIT_DCS_CMD(0x5D, 0x03),
	_INIT_DCS_CMD(0x5E, 0x01),
	_INIT_DCS_CMD(0x5F, 0x03),
	_INIT_DCS_CMD(0x60, 0x3A),
	_INIT_DCS_CMD(0x61, 0x03),
	_INIT_DCS_CMD(0x62, 0x56),
	_INIT_DCS_CMD(0x63, 0x03),
	_INIT_DCS_CMD(0x64, 0x6D),
	_INIT_DCS_CMD(0x65, 0x03),
	_INIT_DCS_CMD(0x66, 0x89),
	_INIT_DCS_CMD(0x67, 0x03),
	_INIT_DCS_CMD(0x68, 0xA3),
	_INIT_DCS_CMD(0x69, 0x03),
	_INIT_DCS_CMD(0x6A, 0xC9),
	_INIT_DCS_CMD(0x6B, 0x03),
	_INIT_DCS_CMD(0x6C, 0xDD),
	_INIT_DCS_CMD(0x6D, 0x03),
	_INIT_DCS_CMD(0x6E, 0xF5),
	_INIT_DCS_CMD(0x6F, 0x03),
	_INIT_DCS_CMD(0x70, 0xFF),

	_INIT_DCS_CMD(0x71, 0x00),
	_INIT_DCS_CMD(0x72, 0x00),
	_INIT_DCS_CMD(0x73, 0x00),
	_INIT_DCS_CMD(0x74, 0x22),
	_INIT_DCS_CMD(0x75, 0x00),
	_INIT_DCS_CMD(0x76, 0x46),
	_INIT_DCS_CMD(0x77, 0x00),
	_INIT_DCS_CMD(0x78, 0x5C),
	_INIT_DCS_CMD(0x79, 0x00),
	_INIT_DCS_CMD(0x7A, 0x76),
	_INIT_DCS_CMD(0x7B, 0x00),
	_INIT_DCS_CMD(0x7C, 0x8D),
	_INIT_DCS_CMD(0x7D, 0x00),
	_INIT_DCS_CMD(0x7E, 0xA6),
	_INIT_DCS_CMD(0x7F, 0x00),
	_INIT_DCS_CMD(0x80, 0xB8),
	_INIT_DCS_CMD(0x81, 0x00),
	_INIT_DCS_CMD(0x82, 0xC7),
	_INIT_DCS_CMD(0x83, 0x00),
	_INIT_DCS_CMD(0x84, 0xF6),
	_INIT_DCS_CMD(0x85, 0x01),
	_INIT_DCS_CMD(0x86, 0x1D),
	_INIT_DCS_CMD(0x87, 0x01),
	_INIT_DCS_CMD(0x88, 0x54),
	_INIT_DCS_CMD(0x89, 0x01),
	_INIT_DCS_CMD(0x8A, 0x81),
	_INIT_DCS_CMD(0x8B, 0x01),
	_INIT_DCS_CMD(0x8C, 0xCB),
	_INIT_DCS_CMD(0x8D, 0x02),
	_INIT_DCS_CMD(0x8E, 0x05),
	_INIT_DCS_CMD(0x8F, 0x02),
	_INIT_DCS_CMD(0x90, 0x07),
	_INIT_DCS_CMD(0x91, 0x02),
	_INIT_DCS_CMD(0x92, 0x47),
	_INIT_DCS_CMD(0x93, 0x02),
	_INIT_DCS_CMD(0x94, 0x82),
	_INIT_DCS_CMD(0x95, 0x02),
	_INIT_DCS_CMD(0x96, 0xAB),
	_INIT_DCS_CMD(0x97, 0x02),
	_INIT_DCS_CMD(0x98, 0xDC),
	_INIT_DCS_CMD(0x99, 0x03),
	_INIT_DCS_CMD(0x9A, 0x01),
	_INIT_DCS_CMD(0x9B, 0x03),
	_INIT_DCS_CMD(0x9C, 0x3A),
	_INIT_DCS_CMD(0x9D, 0x03),
	_INIT_DCS_CMD(0x9E, 0x56),
	_INIT_DCS_CMD(0x9F, 0x03),
	_INIT_DCS_CMD(0xA0, 0x6D),
	_INIT_DCS_CMD(0xA2, 0x03),
	_INIT_DCS_CMD(0xA3, 0x89),
	_INIT_DCS_CMD(0xA4, 0x03),
	_INIT_DCS_CMD(0xA5, 0xA3),
	_INIT_DCS_CMD(0xA6, 0x03),
	_INIT_DCS_CMD(0xA7, 0xC9),
	_INIT_DCS_CMD(0xA9, 0x03),
	_INIT_DCS_CMD(0xAA, 0xDD),
	_INIT_DCS_CMD(0xAB, 0x03),
	_INIT_DCS_CMD(0xAC, 0xF5),
	_INIT_DCS_CMD(0xAD, 0x03),
	_INIT_DCS_CMD(0xAE, 0xFF),

	_INIT_DCS_CMD(0xAF, 0x00),
	_INIT_DCS_CMD(0xB0, 0x00),
	_INIT_DCS_CMD(0xB1, 0x00),
	_INIT_DCS_CMD(0xB2, 0x22),
	_INIT_DCS_CMD(0xB3, 0x00),
	_INIT_DCS_CMD(0xB4, 0x46),
	_INIT_DCS_CMD(0xB5, 0x00),
	_INIT_DCS_CMD(0xB6, 0x5C),
	_INIT_DCS_CMD(0xB7, 0x00),
	_INIT_DCS_CMD(0xB8, 0x76),
	_INIT_DCS_CMD(0xB9, 0x00),
	_INIT_DCS_CMD(0xBA, 0x8D),
	_INIT_DCS_CMD(0xBB, 0x00),
	_INIT_DCS_CMD(0xBC, 0xA6),
	_INIT_DCS_CMD(0xBD, 0x00),
	_INIT_DCS_CMD(0xBE, 0xB8),
	_INIT_DCS_CMD(0xBF, 0x00),
	_INIT_DCS_CMD(0xC0, 0xC7),
	_INIT_DCS_CMD(0xC1, 0x00),
	_INIT_DCS_CMD(0xC2, 0xF6),
	_INIT_DCS_CMD(0xC3, 0x01),
	_INIT_DCS_CMD(0xC4, 0x1D),
	_INIT_DCS_CMD(0xC5, 0x01),
	_INIT_DCS_CMD(0xC6, 0x54),
	_INIT_DCS_CMD(0xC7, 0x01),
	_INIT_DCS_CMD(0xC8, 0x81),
	_INIT_DCS_CMD(0xC9, 0x01),
	_INIT_DCS_CMD(0xCA, 0xCB),
	_INIT_DCS_CMD(0xCB, 0x02),
	_INIT_DCS_CMD(0xCC, 0x05),
	_INIT_DCS_CMD(0xCD, 0x02),
	_INIT_DCS_CMD(0xCE, 0x07),
	_INIT_DCS_CMD(0xCF, 0x02),
	_INIT_DCS_CMD(0xD0, 0x47),
	_INIT_DCS_CMD(0xD1, 0x02),
	_INIT_DCS_CMD(0xD2, 0x82),
	_INIT_DCS_CMD(0xD3, 0x02),
	_INIT_DCS_CMD(0xD4, 0xAB),
	_INIT_DCS_CMD(0xD5, 0x02),
	_INIT_DCS_CMD(0xD6, 0xDC),
	_INIT_DCS_CMD(0xD7, 0x03),
	_INIT_DCS_CMD(0xD8, 0x01),
	_INIT_DCS_CMD(0xD9, 0x03),
	_INIT_DCS_CMD(0xDA, 0x3A),
	_INIT_DCS_CMD(0xDB, 0x03),
	_INIT_DCS_CMD(0xDC, 0x56),
	_INIT_DCS_CMD(0xDD, 0x03),
	_INIT_DCS_CMD(0xDE, 0x6D),
	_INIT_DCS_CMD(0xDF, 0x03),
	_INIT_DCS_CMD(0xE0, 0x89),
	_INIT_DCS_CMD(0xE1, 0x03),
	_INIT_DCS_CMD(0xE2, 0xA3),
	_INIT_DCS_CMD(0xE3, 0x03),
	_INIT_DCS_CMD(0xE4, 0xC9),
	_INIT_DCS_CMD(0xE5, 0x03),
	_INIT_DCS_CMD(0xE6, 0xDD),
	_INIT_DCS_CMD(0xE7, 0x03),
	_INIT_DCS_CMD(0xE8, 0xF5),
	_INIT_DCS_CMD(0xE9, 0x03),
	_INIT_DCS_CMD(0xEA, 0xFF),

	_INIT_DCS_CMD(0xFF, 0x21),
	_INIT_DELAY_CMD(2),

	_INIT_DCS_CMD(0xEB, 0x30),
	_INIT_DCS_CMD(0xEC, 0x17),
	_INIT_DCS_CMD(0xED, 0x20),
	_INIT_DCS_CMD(0xEE, 0x0F),
	_INIT_DCS_CMD(0xEF, 0x1F),
	_INIT_DCS_CMD(0xF0, 0x0F),
	_INIT_DCS_CMD(0xF1, 0x0F),
	_INIT_DCS_CMD(0xF2, 0x07),
	_INIT_DCS_CMD(0xFF, 0x23),
	_INIT_DELAY_CMD(2),

	_INIT_DCS_CMD(0x08, 0x04),
	_INIT_DCS_CMD(0xFF, 0x10),
	_INIT_DELAY_CMD(2),

#ifdef PANEL_BIST_PATTERN
	_INIT_DCS_CMD(0xFF, 0x24),
	_INIT_DELAY_CMD(1),
	_INIT_DCS_CMD(0xEC, 0x01),
#else
	_INIT_DCS_CMD(0x35, 0x00),
	_INIT_DCS_CMD(MIPI_DCS_SET_DISPLAY_ON),
	_INIT_DELAY_CMD(20),
#endif
	{},
};

static int truly_panel_enter_sleep_mode(struct truly *truly)
{
	struct mipi_dsi_device *dsi = truly->dsi;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0)
		return ret;

	return 0;
}

static int truly_disable(struct drm_panel *panel)
{
	struct truly *ctx = panel_to_truly(panel);

	pr_info("%s enabled %d backlight 0x%p\n", __func__,
		ctx->enabled, ctx->backlight);

	if (!ctx->enabled) {
		pr_err("panel is aready disabled\n");
		return 0;
	}

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = false;

	return 0;
}

static int truly_unprepare(struct drm_panel *panel)
{
	struct truly *ctx = panel_to_truly(panel);
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = truly_panel_enter_sleep_mode(ctx);
	if (ret < 0) {
		dev_err(panel->dev, "failed to set panel off: %d\n",
			ret);
		return ret;
	}

	if (ctx->reset_gpio)
		gpiod_set_value(ctx->reset_gpio, 0);

	ctx->error = 0;
	ctx->prepared = false;

	return 0;
}

static int truly_prepare(struct drm_panel *panel)
{
	struct truly *ctx = panel_to_truly(panel);
	int ret;

	if (ctx->prepared)
		return 0;

	DRM_DEBUG_DRIVER("start %d\n", __LINE__);

	if (ctx->reset_gpio)
		gpiod_set_value(ctx->reset_gpio, 1);

	msleep(100);

	ret = truly_panel_init_dcs_cmd(ctx);
	if (ret < 0) {
		dev_err(panel->dev, "failed to init panel: %d\n", ret);
		return ret;
	}

	ctx->prepared = true;

#ifdef PANEL_SUPPORT_READBACK
	truly_panel_get_data(ctx);
#endif
#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

	DRM_DEBUG_DRIVER("finish %d ret %d\n", __LINE__, ret);

	return ret;
}

static int truly_enable(struct drm_panel *panel)
{
	struct truly *ctx = panel_to_truly(panel);

	pr_info("%s enabled %d backlight 0x%p\n", __func__,
	ctx->enabled, ctx->backlight);

	if (ctx->enabled) {
		pr_err("panel is already enable\n");

		return 0;
	}

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	return 0;
}

static const struct drm_display_mode default_mode = {
#ifdef CONFIG_FPGA_EARLY_PORTING
	.clock = 4333,
#else
	.clock = 142858,
#endif
	.hdisplay = 1080,
	.hsync_start = 1080 + 40,
	.hsync_end = 1080 + 10 + 40,
	.htotal = 1080 + 10 + 20 + 40,
	.vdisplay = 1920,
	.vsync_start = 1920 + 10,
	.vsync_end = 1920 + 2 + 10,
	.vtotal = 1920 + 2 + 8 + 10,
};

static const struct panel_desc truly_tdo_bd0598u40004_desc = {
	.modes = &default_mode,
	.bpc = 8,
	.size = {
		.width_mm = 75,
		.height_mm = 133,
	},
	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
		      MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET
			  | MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.init_cmds = truly_init_cmd,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct truly *ctx = panel_to_truly(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int panel_ata_check(struct drm_panel *panel)
{
	struct truly *ctx = panel_to_truly(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	unsigned char data[3];
	unsigned char id[3] = {0x00, 0x00, 0x00};
	ssize_t ret;

	ret = mipi_dsi_dcs_read(dsi, 0x4, data, 3);
	if (ret < 0)
		pr_err("%s error\n", __func__);

	pr_info("ATA read data %x %x %x\n", data[0], data[1], data[2]);

	if (data[0] == id[0] &&
			data[1] == id[1] &&
			data[2] == id[2])
		return 1;

	DDPINFO("ATA expect read data is %x %x %x\n",
			id[0], id[1], id[2]);

	return 0;
}

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	char bl_tb0[] = {0x51, 0xFF};

	bl_tb0[1] = level;

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));

	return 0;
}

static struct mtk_panel_params ext_params = {
#ifdef CONFIG_FPGA_EARLY_PORTING
	.pll_clk = 13,
#else
	.pll_clk = 500,
#endif
	.vfp_low_power = 810,
	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x1c,
	},

};

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = panel_ata_check,
};
#endif

static int truly_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	struct truly *truly = panel_to_truly(panel);
	const struct drm_display_mode *m = truly->desc->modes;
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, m);
	if (!mode) {
		dev_err(panel->dev, "failed to add mode %ux%ux@%u\n",
			m->hdisplay, m->vdisplay, drm_mode_vrefresh(m));
		return -ENOMEM;
	}

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = truly->desc->size.width_mm;
	connector->display_info.height_mm = truly->desc->size.height_mm;
	connector->display_info.bpc = truly->desc->bpc;
	drm_connector_set_panel_orientation(connector, truly->orientation);

	return 1;
}

static const struct drm_panel_funcs truly_drm_funcs = {
	.disable = truly_disable,
	.unprepare = truly_unprepare,
	.prepare = truly_prepare,
	.enable = truly_enable,
	.get_modes = truly_get_modes,
};

static int truly_panel_add(struct truly *truly)
{
	struct device *dev = &truly->dsi->dev;
	struct device_node *backlight;
	int ret;

#ifndef CONFIG_FPGA_EARLY_PORTING
	truly->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_ASIS);
	if (IS_ERR(truly->reset_gpio)) {
		dev_err(dev, "cannot get reset-gpios %ld\n",
			PTR_ERR(truly->reset_gpio));
		return PTR_ERR(truly->reset_gpio);
	}
	ret = gpiod_direction_output(truly->reset_gpio, 1);
	if (ret < 0) {
		dev_err(dev, "cannot configure reset-gpios %d\n", ret);
		return ret;
	}
#endif

	drm_panel_init(&truly->panel, dev, &truly_drm_funcs,
			   DRM_MODE_CONNECTOR_DSI);
	ret = of_drm_get_panel_orientation(dev->of_node, &truly->orientation);
	if (ret < 0) {
		dev_err(dev, "%pOF: failed to get orientation %d\n", dev->of_node, ret);
		//return ret;
	}

	truly->prepared = false;
	truly->enabled = false;
	truly->panel.funcs = &truly_drm_funcs;
	truly->panel.dev = dev;

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		truly->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!truly->backlight) {
			dev_err(dev, "failed to find backlight device %d\n", ret);
			return -EPROBE_DEFER;
		}
	}

	drm_panel_add(&truly->panel);

	return 0;
	}
static int truly_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct panel_desc *desc;
	struct truly *ctx;
	int ret;

	DRM_DEBUG_DRIVER("start\n");

	ctx = devm_kzalloc(dev, sizeof(struct truly), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dev = dev;
	desc = of_device_get_match_data(&dsi->dev);
	dsi->lanes = desc->lanes;
	dsi->format = desc->format;
	dsi->mode_flags = desc->mode_flags;
	ctx->desc = desc;
	ctx->dsi = dsi;
	ret = truly_panel_add(ctx);
	if (ret < 0) {
		dev_err(dev, "failed to add panel %d\n", ret);
		return ret;
	}

	mipi_dsi_set_drvdata(dsi, ctx);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "failed to attach panel %d\n", ret);
		drm_panel_remove(&ctx->panel);
	}

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;
#endif

	DRM_DEBUG_DRIVER("finish\n");

	return ret;
}

static int truly_remove(struct mipi_dsi_device *dsi)
{
	struct truly *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id truly_of_match[] = {
	{ .compatible = "truly,bd0598u4004",
	  .data = &truly_tdo_bd0598u40004_desc
	},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, truly_of_match);

static struct mipi_dsi_driver truly_driver = {
	.probe = truly_probe,
	.remove = truly_remove,
	.driver = {
		.name = "panel-bd0598u4004",
		.owner = THIS_MODULE,
		.of_match_table = truly_of_match,
	},
};

module_mipi_dsi_driver(truly_driver);

MODULE_AUTHOR("Shaoming Chen <shaoming.chen@mediatek.com>");
MODULE_DESCRIPTION("TRULY TDO-BD0598U40004 LCD Panel Driver");
MODULE_LICENSE("GPL v2");
