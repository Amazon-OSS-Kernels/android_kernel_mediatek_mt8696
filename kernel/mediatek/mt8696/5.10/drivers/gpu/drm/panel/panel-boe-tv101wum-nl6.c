// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Jitao Shi <jitao.shi@mediatek.com>
 */
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include "nt50358a_i2c.h"

#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_log.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#endif

#define NT50358A_REG_WRITE(add, data) nt50358a_write_byte(add, data)
#define NT50358A_REG_READ(add) nt50358a_read_byte(add)

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
	bool discharge_on_disable;
};

struct boe_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;
	struct device *dev;
	const struct panel_desc *desc;
	struct backlight_device *backlight;
	bool enabled;

	enum drm_panel_orientation orientation;
	struct regulator *pp1800;
	struct gpio_desc *avee;
	struct gpio_desc *avdd;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *leden_gpio;

	bool prepared_power;
	bool prepared;
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

static const struct panel_init_cmd boe_init_cmd[] = {
	_INIT_DCS_CMD(0xB0, 0x05),
	_INIT_DCS_CMD(0xB1, 0xE5),
	_INIT_DCS_CMD(0xB3, 0x52),
	_INIT_DCS_CMD(0xB0, 0x00),
	_INIT_DCS_CMD(0xB3, 0x88),
	_INIT_DCS_CMD(0xB0, 0x04),
	_INIT_DCS_CMD(0xB8, 0x00),
	_INIT_DCS_CMD(0xB0, 0x00),
	_INIT_DCS_CMD(0xB6, 0x03),
	_INIT_DCS_CMD(0xBA, 0x8B),
	_INIT_DCS_CMD(0xBF, 0x1A),
	_INIT_DCS_CMD(0xC0, 0x0F),
	_INIT_DCS_CMD(0xC2, 0x0C),
	_INIT_DCS_CMD(0xC3, 0x02),
	_INIT_DCS_CMD(0xC4, 0x0C),
	_INIT_DCS_CMD(0xC5, 0x02),
	_INIT_DCS_CMD(0xB0, 0x01),
	_INIT_DCS_CMD(0xE0, 0x26),
	_INIT_DCS_CMD(0xE1, 0x26),
	_INIT_DCS_CMD(0xDC, 0x00),
	_INIT_DCS_CMD(0xDD, 0x00),
	_INIT_DCS_CMD(0xCC, 0x26),
	_INIT_DCS_CMD(0xCD, 0x26),
	_INIT_DCS_CMD(0xC8, 0x00),
	_INIT_DCS_CMD(0xC9, 0x00),
	_INIT_DCS_CMD(0xD2, 0x03),
	_INIT_DCS_CMD(0xD3, 0x03),
	_INIT_DCS_CMD(0xE6, 0x04),
	_INIT_DCS_CMD(0xE7, 0x04),
	_INIT_DCS_CMD(0xC4, 0x09),
	_INIT_DCS_CMD(0xC5, 0x09),
	_INIT_DCS_CMD(0xD8, 0x0A),
	_INIT_DCS_CMD(0xD9, 0x0A),
	_INIT_DCS_CMD(0xC2, 0x0B),
	_INIT_DCS_CMD(0xC3, 0x0B),
	_INIT_DCS_CMD(0xD6, 0x0C),
	_INIT_DCS_CMD(0xD7, 0x0C),
	_INIT_DCS_CMD(0xC0, 0x05),
	_INIT_DCS_CMD(0xC1, 0x05),
	_INIT_DCS_CMD(0xD4, 0x06),
	_INIT_DCS_CMD(0xD5, 0x06),
	_INIT_DCS_CMD(0xCA, 0x07),
	_INIT_DCS_CMD(0xCB, 0x07),
	_INIT_DCS_CMD(0xDE, 0x08),
	_INIT_DCS_CMD(0xDF, 0x08),
	_INIT_DCS_CMD(0xB0, 0x02),
	_INIT_DCS_CMD(0xC0, 0x00),
	_INIT_DCS_CMD(0xC1, 0x0D),
	_INIT_DCS_CMD(0xC2, 0x17),
	_INIT_DCS_CMD(0xC3, 0x26),
	_INIT_DCS_CMD(0xC4, 0x31),
	_INIT_DCS_CMD(0xC5, 0x1C),
	_INIT_DCS_CMD(0xC6, 0x2C),
	_INIT_DCS_CMD(0xC7, 0x33),
	_INIT_DCS_CMD(0xC8, 0x31),
	_INIT_DCS_CMD(0xC9, 0x37),
	_INIT_DCS_CMD(0xCA, 0x37),
	_INIT_DCS_CMD(0xCB, 0x37),
	_INIT_DCS_CMD(0xCC, 0x39),
	_INIT_DCS_CMD(0xCD, 0x2E),
	_INIT_DCS_CMD(0xCE, 0x2F),
	_INIT_DCS_CMD(0xCF, 0x2F),
	_INIT_DCS_CMD(0xD0, 0x07),
	_INIT_DCS_CMD(0xD2, 0x00),
	_INIT_DCS_CMD(0xD3, 0x0D),
	_INIT_DCS_CMD(0xD4, 0x17),
	_INIT_DCS_CMD(0xD5, 0x26),
	_INIT_DCS_CMD(0xD6, 0x31),
	_INIT_DCS_CMD(0xD7, 0x3F),
	_INIT_DCS_CMD(0xD8, 0x3F),
	_INIT_DCS_CMD(0xD9, 0x3F),
	_INIT_DCS_CMD(0xDA, 0x3F),
	_INIT_DCS_CMD(0xDB, 0x37),
	_INIT_DCS_CMD(0xDC, 0x37),
	_INIT_DCS_CMD(0xDD, 0x37),
	_INIT_DCS_CMD(0xDE, 0x39),
	_INIT_DCS_CMD(0xDF, 0x2E),
	_INIT_DCS_CMD(0xE0, 0x2F),
	_INIT_DCS_CMD(0xE1, 0x2F),
	_INIT_DCS_CMD(0xE2, 0x07),
	_INIT_DCS_CMD(0xB0, 0x03),
	_INIT_DCS_CMD(0xC8, 0x0B),
	_INIT_DCS_CMD(0xC9, 0x07),
	_INIT_DCS_CMD(0xC3, 0x00),
	_INIT_DCS_CMD(0xE7, 0x00),
	_INIT_DCS_CMD(0xC5, 0x2A),
	_INIT_DCS_CMD(0xDE, 0x2A),
	_INIT_DCS_CMD(0xCA, 0x43),
	_INIT_DCS_CMD(0xC9, 0x07),
	_INIT_DCS_CMD(0xE4, 0xC0),
	_INIT_DCS_CMD(0xE5, 0x0D),
	_INIT_DCS_CMD(0xCB, 0x00),
	_INIT_DCS_CMD(0xB0, 0x06),
	_INIT_DCS_CMD(0xB8, 0xA5),
	_INIT_DCS_CMD(0xC0, 0xA5),
	_INIT_DCS_CMD(0xC7, 0x0F),
	_INIT_DCS_CMD(0xD5, 0x32),
	_INIT_DCS_CMD(0xB8, 0x00),
	_INIT_DCS_CMD(0xC0, 0x00),
	_INIT_DCS_CMD(0xBC, 0x00),
	_INIT_DCS_CMD(0xB0, 0x07),
	_INIT_DCS_CMD(0xB1, 0x00),
	_INIT_DCS_CMD(0xB2, 0x02),
	_INIT_DCS_CMD(0xB3, 0x0F),
	_INIT_DCS_CMD(0xB4, 0x25),
	_INIT_DCS_CMD(0xB5, 0x39),
	_INIT_DCS_CMD(0xB6, 0x4E),
	_INIT_DCS_CMD(0xB7, 0x72),
	_INIT_DCS_CMD(0xB8, 0x97),
	_INIT_DCS_CMD(0xB9, 0xDC),
	_INIT_DCS_CMD(0xBA, 0x22),
	_INIT_DCS_CMD(0xBB, 0xA4),
	_INIT_DCS_CMD(0xBC, 0x2B),
	_INIT_DCS_CMD(0xBD, 0x2F),
	_INIT_DCS_CMD(0xBE, 0xA9),
	_INIT_DCS_CMD(0xBF, 0x25),
	_INIT_DCS_CMD(0xC0, 0x61),
	_INIT_DCS_CMD(0xC1, 0x97),
	_INIT_DCS_CMD(0xC2, 0xB2),
	_INIT_DCS_CMD(0xC3, 0xCD),
	_INIT_DCS_CMD(0xC4, 0xD9),
	_INIT_DCS_CMD(0xC5, 0xE7),
	_INIT_DCS_CMD(0xC6, 0xF4),
	_INIT_DCS_CMD(0xC7, 0xFA),
	_INIT_DCS_CMD(0xC8, 0xFC),
	_INIT_DCS_CMD(0xC9, 0x00),
	_INIT_DCS_CMD(0xCA, 0x00),
	_INIT_DCS_CMD(0xCB, 0x16),
	_INIT_DCS_CMD(0xCC, 0xAF),
	_INIT_DCS_CMD(0xCD, 0xFF),
	_INIT_DCS_CMD(0xCE, 0xFF),
	_INIT_DCS_CMD(0xB0, 0x08),
	_INIT_DCS_CMD(0xB1, 0x04),
	_INIT_DCS_CMD(0xB2, 0x05),
	_INIT_DCS_CMD(0xB3, 0x11),
	_INIT_DCS_CMD(0xB4, 0x24),
	_INIT_DCS_CMD(0xB5, 0x39),
	_INIT_DCS_CMD(0xB6, 0x4F),
	_INIT_DCS_CMD(0xB7, 0x72),
	_INIT_DCS_CMD(0xB8, 0x98),
	_INIT_DCS_CMD(0xB9, 0xDC),
	_INIT_DCS_CMD(0xBA, 0x23),
	_INIT_DCS_CMD(0xBB, 0xA6),
	_INIT_DCS_CMD(0xBC, 0x2C),
	_INIT_DCS_CMD(0xBD, 0x30),
	_INIT_DCS_CMD(0xBE, 0xAA),
	_INIT_DCS_CMD(0xBF, 0x26),
	_INIT_DCS_CMD(0xC0, 0x62),
	_INIT_DCS_CMD(0xC1, 0x9B),
	_INIT_DCS_CMD(0xC2, 0xB5),
	_INIT_DCS_CMD(0xC3, 0xCF),
	_INIT_DCS_CMD(0xC4, 0xDB),
	_INIT_DCS_CMD(0xC5, 0xE8),
	_INIT_DCS_CMD(0xC6, 0xF5),
	_INIT_DCS_CMD(0xC7, 0xFA),
	_INIT_DCS_CMD(0xC8, 0xFC),
	_INIT_DCS_CMD(0xC9, 0x00),
	_INIT_DCS_CMD(0xCA, 0x00),
	_INIT_DCS_CMD(0xCB, 0x16),
	_INIT_DCS_CMD(0xCC, 0xAF),
	_INIT_DCS_CMD(0xCD, 0xFF),
	_INIT_DCS_CMD(0xCE, 0xFF),
	_INIT_DCS_CMD(0xB0, 0x09),
	_INIT_DCS_CMD(0xB1, 0x04),
	_INIT_DCS_CMD(0xB2, 0x02),
	_INIT_DCS_CMD(0xB3, 0x16),
	_INIT_DCS_CMD(0xB4, 0x24),
	_INIT_DCS_CMD(0xB5, 0x3B),
	_INIT_DCS_CMD(0xB6, 0x4F),
	_INIT_DCS_CMD(0xB7, 0x73),
	_INIT_DCS_CMD(0xB8, 0x99),
	_INIT_DCS_CMD(0xB9, 0xE0),
	_INIT_DCS_CMD(0xBA, 0x26),
	_INIT_DCS_CMD(0xBB, 0xAD),
	_INIT_DCS_CMD(0xBC, 0x36),
	_INIT_DCS_CMD(0xBD, 0x3A),
	_INIT_DCS_CMD(0xBE, 0xAE),
	_INIT_DCS_CMD(0xBF, 0x2A),
	_INIT_DCS_CMD(0xC0, 0x66),
	_INIT_DCS_CMD(0xC1, 0x9E),
	_INIT_DCS_CMD(0xC2, 0xB8),
	_INIT_DCS_CMD(0xC3, 0xD1),
	_INIT_DCS_CMD(0xC4, 0xDD),
	_INIT_DCS_CMD(0xC5, 0xE9),
	_INIT_DCS_CMD(0xC6, 0xF6),
	_INIT_DCS_CMD(0xC7, 0xFA),
	_INIT_DCS_CMD(0xC8, 0xFC),
	_INIT_DCS_CMD(0xC9, 0x00),
	_INIT_DCS_CMD(0xCA, 0x00),
	_INIT_DCS_CMD(0xCB, 0x16),
	_INIT_DCS_CMD(0xCC, 0xAF),
	_INIT_DCS_CMD(0xCD, 0xFF),
	_INIT_DCS_CMD(0xCE, 0xFF),
	_INIT_DCS_CMD(0xB0, 0x0A),
	_INIT_DCS_CMD(0xB1, 0x00),
	_INIT_DCS_CMD(0xB2, 0x02),
	_INIT_DCS_CMD(0xB3, 0x0F),
	_INIT_DCS_CMD(0xB4, 0x25),
	_INIT_DCS_CMD(0xB5, 0x39),
	_INIT_DCS_CMD(0xB6, 0x4E),
	_INIT_DCS_CMD(0xB7, 0x72),
	_INIT_DCS_CMD(0xB8, 0x97),
	_INIT_DCS_CMD(0xB9, 0xDC),
	_INIT_DCS_CMD(0xBA, 0x22),
	_INIT_DCS_CMD(0xBB, 0xA4),
	_INIT_DCS_CMD(0xBC, 0x2B),
	_INIT_DCS_CMD(0xBD, 0x2F),
	_INIT_DCS_CMD(0xBE, 0xA9),
	_INIT_DCS_CMD(0xBF, 0x25),
	_INIT_DCS_CMD(0xC0, 0x61),
	_INIT_DCS_CMD(0xC1, 0x97),
	_INIT_DCS_CMD(0xC2, 0xB2),
	_INIT_DCS_CMD(0xC3, 0xCD),
	_INIT_DCS_CMD(0xC4, 0xD9),
	_INIT_DCS_CMD(0xC5, 0xE7),
	_INIT_DCS_CMD(0xC6, 0xF4),
	_INIT_DCS_CMD(0xC7, 0xFA),
	_INIT_DCS_CMD(0xC8, 0xFC),
	_INIT_DCS_CMD(0xC9, 0x00),
	_INIT_DCS_CMD(0xCA, 0x00),
	_INIT_DCS_CMD(0xCB, 0x16),
	_INIT_DCS_CMD(0xCC, 0xAF),
	_INIT_DCS_CMD(0xCD, 0xFF),
	_INIT_DCS_CMD(0xCE, 0xFF),
	_INIT_DCS_CMD(0xB0, 0x0B),
	_INIT_DCS_CMD(0xB1, 0x04),
	_INIT_DCS_CMD(0xB2, 0x05),
	_INIT_DCS_CMD(0xB3, 0x11),
	_INIT_DCS_CMD(0xB4, 0x24),
	_INIT_DCS_CMD(0xB5, 0x39),
	_INIT_DCS_CMD(0xB6, 0x4F),
	_INIT_DCS_CMD(0xB7, 0x72),
	_INIT_DCS_CMD(0xB8, 0x98),
	_INIT_DCS_CMD(0xB9, 0xDC),
	_INIT_DCS_CMD(0xBA, 0x23),
	_INIT_DCS_CMD(0xBB, 0xA6),
	_INIT_DCS_CMD(0xBC, 0x2C),
	_INIT_DCS_CMD(0xBD, 0x30),
	_INIT_DCS_CMD(0xBE, 0xAA),
	_INIT_DCS_CMD(0xBF, 0x26),
	_INIT_DCS_CMD(0xC0, 0x62),
	_INIT_DCS_CMD(0xC1, 0x9B),
	_INIT_DCS_CMD(0xC2, 0xB5),
	_INIT_DCS_CMD(0xC3, 0xCF),
	_INIT_DCS_CMD(0xC4, 0xDB),
	_INIT_DCS_CMD(0xC5, 0xE8),
	_INIT_DCS_CMD(0xC6, 0xF5),
	_INIT_DCS_CMD(0xC7, 0xFA),
	_INIT_DCS_CMD(0xC8, 0xFC),
	_INIT_DCS_CMD(0xC9, 0x00),
	_INIT_DCS_CMD(0xCA, 0x00),
	_INIT_DCS_CMD(0xCB, 0x16),
	_INIT_DCS_CMD(0xCC, 0xAF),
	_INIT_DCS_CMD(0xCD, 0xFF),
	_INIT_DCS_CMD(0xCE, 0xFF),
	_INIT_DCS_CMD(0xB0, 0x0C),
	_INIT_DCS_CMD(0xB1, 0x04),
	_INIT_DCS_CMD(0xB2, 0x02),
	_INIT_DCS_CMD(0xB3, 0x16),
	_INIT_DCS_CMD(0xB4, 0x24),
	_INIT_DCS_CMD(0xB5, 0x3B),
	_INIT_DCS_CMD(0xB6, 0x4F),
	_INIT_DCS_CMD(0xB7, 0x73),
	_INIT_DCS_CMD(0xB8, 0x99),
	_INIT_DCS_CMD(0xB9, 0xE0),
	_INIT_DCS_CMD(0xBA, 0x26),
	_INIT_DCS_CMD(0xBB, 0xAD),
	_INIT_DCS_CMD(0xBC, 0x36),
	_INIT_DCS_CMD(0xBD, 0x3A),
	_INIT_DCS_CMD(0xBE, 0xAE),
	_INIT_DCS_CMD(0xBF, 0x2A),
	_INIT_DCS_CMD(0xC0, 0x66),
	_INIT_DCS_CMD(0xC1, 0x9E),
	_INIT_DCS_CMD(0xC2, 0xB8),
	_INIT_DCS_CMD(0xC3, 0xD1),
	_INIT_DCS_CMD(0xC4, 0xDD),
	_INIT_DCS_CMD(0xC5, 0xE9),
	_INIT_DCS_CMD(0xC6, 0xF6),
	_INIT_DCS_CMD(0xC7, 0xFA),
	_INIT_DCS_CMD(0xC8, 0xFC),
	_INIT_DCS_CMD(0xC9, 0x00),
	_INIT_DCS_CMD(0xCA, 0x00),
	_INIT_DCS_CMD(0xCB, 0x16),
	_INIT_DCS_CMD(0xCC, 0xAF),
	_INIT_DCS_CMD(0xCD, 0xFF),
	_INIT_DCS_CMD(0xCE, 0xFF),
	_INIT_DCS_CMD(0xB0, 0x00),
	_INIT_DCS_CMD(0xB3, 0x08),
	_INIT_DCS_CMD(0xB0, 0x04),
	_INIT_DCS_CMD(0xB8, 0x68),
	_INIT_DELAY_CMD(20),
	{},
};

static inline struct boe_panel *to_boe_panel(struct drm_panel *panel)
{
	return container_of(panel, struct boe_panel, base);
}

static int boe_panel_init_dcs_cmd(struct boe_panel *boe)
{
	struct mipi_dsi_device *dsi = boe->dsi;
	struct drm_panel *panel = &boe->base;
	int i, err = 0;

	if (boe->desc->init_cmds) {
		const struct panel_init_cmd *init_cmds = boe->desc->init_cmds;

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

static int boe_panel_enter_sleep_mode(struct boe_panel *boe)
{
	struct mipi_dsi_device *dsi = boe->dsi;
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

static int boe_panel_unprepare_power(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);

	if (!boe->prepared_power)
		return 0;

	msleep(150);

	if (boe->desc->discharge_on_disable) {
		/* Enable AVDD & AVEE discharge when power off */
		NT50358A_REG_WRITE(0x03, 0x33);
		gpiod_set_value(boe->avee, 0);
		gpiod_set_value(boe->avdd, 0);
		usleep_range(5000, 7000);
		gpiod_set_value(boe->enable_gpio, 0);
		usleep_range(5000, 7000);
		if (boe->pp1800)
			regulator_disable(boe->pp1800);
	} else {
		gpiod_set_value(boe->enable_gpio, 0);
		usleep_range(500, 1000);
		/* Enable AVDD & AVEE discharge when power off */
		NT50358A_REG_WRITE(0x03, 0x33);
		gpiod_set_value(boe->avee, 0);
		gpiod_set_value(boe->avdd, 0);
		usleep_range(5000, 7000);
		if (boe->pp1800)
			regulator_disable(boe->pp1800);
	}

	boe->prepared_power = false;

	return 0;
}

static int boe_panel_unprepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	int ret;

	if (!boe->prepared)
		return 0;

	if (!boe->desc->discharge_on_disable) {
		ret = boe_panel_enter_sleep_mode(boe);
		if (ret < 0) {
			dev_err(panel->dev, "failed to set panel off: %d\n",
				ret);
			return ret;
		}
	}

	boe_panel_unprepare_power(panel);

	boe->prepared = false;

	return 0;
}

static int boe_panel_prepare_power(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);

	if (boe->prepared_power)
		return 0;

	gpiod_set_value(boe->enable_gpio, 0);
	usleep_range(1000, 1500);

	if (boe->pp1800) {
		int ret;

		ret = regulator_enable(boe->pp1800);
		if (ret < 0)
			dev_err(panel->dev, "Enable pp1800 fail, %ld\n", ret);
		 usleep_range(3000, 5000);
	}
	gpiod_set_value(boe->avdd, 1);
	gpiod_set_value(boe->avee, 1);
	usleep_range(6000, 7000);
	/* Set AVDD = 5.8V */
	NT50358A_REG_WRITE(0x00, 0x12);
	/* Set AVEE = -5.8V */
	NT50358A_REG_WRITE(0x01, 0x12);
	/* Disable AVDD & AVEE discharge when power on*/
	NT50358A_REG_WRITE(0x03, 0x30);

	gpiod_set_value(boe->enable_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value(boe->enable_gpio, 0);
	usleep_range(1000, 2000);
	gpiod_set_value(boe->enable_gpio, 1);
	usleep_range(6000, 10000);

	boe->prepared_power = true;

	return 0;
}

static int boe_panel_prepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	int ret;

	if (boe->prepared)
		return 0;

	boe_panel_prepare_power(panel);

	ret = boe_panel_init_dcs_cmd(boe);
	if (ret < 0) {
		dev_err(panel->dev, "failed to init panel: %d\n", ret);
		return ret;
	}

	boe->prepared = true;

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

	return 0;
}

static int boe_panel_enable(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);

	if (boe->enabled)
		return 0;

	msleep(130);

	if (boe->backlight) {
		boe->backlight->props.state &= ~BL_CORE_FBBLANK;
		boe->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(boe->backlight);
	}

	gpiod_set_value(boe->leden_gpio, 1);
	boe->enabled = true;

	return 0;
}

static int boe_panel_disable(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);

	if (!boe->enabled)
		return 0;

	if (boe->backlight) {
		boe->backlight->props.power = FB_BLANK_POWERDOWN;
		boe->backlight->props.state |= BL_CORE_FBBLANK;
		backlight_update_status(boe->backlight);
	}

	gpiod_set_value(boe->leden_gpio, 0);

	boe->enabled = false;

	return 0;
}

static const struct drm_display_mode boe_tv101wum_nl6_default_mode = {
	.clock = 159425,
	.hdisplay = 1200,
	.hsync_start = 1200 + 100,
	.hsync_end = 1200 + 100 + 40,
	.htotal = 1200 + 100 + 40 + 24,
	.vdisplay = 1920,
	.vsync_start = 1920 + 10,
	.vsync_end = 1920 + 10 + 14,
	.vtotal = 1920 + 10 + 14 + 4,
};

static const struct panel_desc boe_tv101wum_nl6_desc = {
	.modes = &boe_tv101wum_nl6_default_mode,
	.bpc = 8,
	.size = {
		.width_mm = 135,
		.height_mm = 216,
	},
	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
		      MIPI_DSI_MODE_LPM,
	.init_cmds = boe_init_cmd,
	.discharge_on_disable = false,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct boe_panel *boe = to_boe_panel(panel);

	gpiod_set_value(boe->enable_gpio, on);
	devm_gpiod_put(boe->dev, boe->enable_gpio);

	return 0;
}

static int panel_ata_check(struct drm_panel *panel)
{
	return 1;
}

static struct mtk_panel_params ext_params = {
	.pll_clk = 506,
	.vfp_low_power = 400,
	.cust_esd_check = 0,
	.esd_check_enable = 0,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x1c,
	},
};

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.ata_check = panel_ata_check,
};
#endif

static int boe_panel_get_modes(struct drm_panel *panel,
			       struct drm_connector *connector)
{
	struct boe_panel *boe = to_boe_panel(panel);
	const struct drm_display_mode *m = boe->desc->modes;
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, m);
	if (!mode) {
		dev_err(panel->dev, "failed to add mode %ux%u@%u\n",
			m->hdisplay, m->vdisplay, drm_mode_vrefresh(m));
		return -ENOMEM;
	}

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = boe->desc->size.width_mm;
	connector->display_info.height_mm = boe->desc->size.height_mm;
	connector->display_info.bpc = boe->desc->bpc;
	drm_connector_set_panel_orientation(connector, boe->orientation);

	return 1;
}

static const struct drm_panel_funcs boe_panel_funcs = {
	.unprepare = boe_panel_unprepare,
	.prepare = boe_panel_prepare,
	.disable = boe_panel_disable,
	.enable = boe_panel_enable,
	.get_modes = boe_panel_get_modes,
};

static int boe_panel_add(struct boe_panel *boe)
{
	struct device *dev = &boe->dsi->dev;
	struct device_node *backlight;
	int ret;

	boe->avdd = devm_gpiod_get(dev, "avdd", GPIOD_OUT_HIGH);
	if (IS_ERR(boe->avdd)) {
		dev_err(dev, "cannot get avdd-gpios %ld\n",
			PTR_ERR(boe->avdd));
		return PTR_ERR(boe->avdd);
	}

	boe->avee = devm_gpiod_get(dev, "avee", GPIOD_OUT_HIGH);
	if (IS_ERR(boe->avee)) {
		dev_err(dev, "cannot get avee-gpios %ld\n",
			PTR_ERR(boe->avee));
		return PTR_ERR(boe->avee);
	}

	boe->pp1800 = devm_regulator_get(dev, "pp1800");
	if (IS_ERR(boe->pp1800)) {
		dev_err(dev, "cannot get pp1800 %ld\n",
			PTR_ERR(boe->pp1800));
		boe->pp1800 = NULL;
	} else {
		ret = regulator_enable(boe->pp1800);
		if (ret < 0)
			dev_err(dev, "Enable pp1800 fail, %ld\n", ret);
	}

	boe->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(boe->enable_gpio)) {
		dev_err(dev, "cannot get reset-gpios %ld\n",
			PTR_ERR(boe->enable_gpio));
		return PTR_ERR(boe->enable_gpio);
	}

	boe->leden_gpio = devm_gpiod_get(dev, "leden", GPIOD_OUT_HIGH);
	if (IS_ERR(boe->leden_gpio)) {
		dev_err(dev, "cannot get lcden-gpios %ld\n",
			PTR_ERR(boe->leden_gpio));
		return PTR_ERR(boe->leden_gpio);
	}

	drm_panel_init(&boe->base, dev, &boe_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	ret = of_drm_get_panel_orientation(dev->of_node, &boe->orientation);
	if (ret < 0) {
		dev_err(dev, "%pOF: failed to get orientation %d\n", dev->of_node, ret);
		return ret;
	}

#ifndef CONFIG_MTK_DISP_NO_LK
	boe->prepared = true;
	boe->enabled = true;
	boe->prepared_power = true;
#endif

	boe->base.funcs = &boe_panel_funcs;
	boe->base.dev = &boe->dsi->dev;

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		boe->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!boe->backlight)
			return -EPROBE_DEFER;
	}

	drm_panel_add(&boe->base);

	return 0;
}

static int boe_panel_probe(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe;
	struct device *dev = &dsi->dev;
	int ret;
	const struct panel_desc *desc;

	boe = devm_kzalloc(&dsi->dev, sizeof(*boe), GFP_KERNEL);
	if (!boe)
		return -ENOMEM;

	boe->dev = dev;
	desc = of_device_get_match_data(&dsi->dev);
	dsi->lanes = desc->lanes;
	dsi->format = desc->format;
	dsi->mode_flags = desc->mode_flags;
	boe->desc = desc;
	boe->dsi = dsi;
	ret = boe_panel_add(boe);
	if (ret < 0)
		return ret;

	mipi_dsi_set_drvdata(dsi, boe);

	ret = mipi_dsi_attach(dsi);
	if (ret)
		drm_panel_remove(&boe->base);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&boe->base);
	ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &boe->base);
	if (ret < 0)
		return ret;
#endif

	return ret;
}

static void boe_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);

	drm_panel_disable(&boe->base);
	drm_panel_unprepare(&boe->base);
}

static int boe_panel_remove(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);
	int ret;

	boe_panel_shutdown(dsi);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to detach from DSI host: %d\n", ret);

	if (boe->base.dev)
		drm_panel_remove(&boe->base);

	return 0;
}

static const struct of_device_id boe_of_match[] = {
	{ .compatible = "boe,tv101wum-nl6",
	  .data = &boe_tv101wum_nl6_desc
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, boe_of_match);

static struct mipi_dsi_driver boe_panel_driver = {
	.driver = {
		.name = "panel-boe-tv101wum-nl6",
		.of_match_table = boe_of_match,
	},
	.probe = boe_panel_probe,
	.remove = boe_panel_remove,
	.shutdown = boe_panel_shutdown,
};
module_mipi_dsi_driver(boe_panel_driver);

MODULE_AUTHOR("Jitao Shi <jitao.shi@mediatek.com>");
MODULE_DESCRIPTION("BOE tv101wum-nl6 1200x1920 video mode panel driver");
MODULE_LICENSE("GPL v2");
