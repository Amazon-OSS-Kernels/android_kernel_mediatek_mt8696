/*
 * mt8696-afe-pcm.c  --  Mediatek 8696 ALSA SoC AFE platform driver
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Sail Yang <sail.yang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/mfd/syscon.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "mt8696-afe-common.h"
#include "mt8696-afe-utils.h"
#include "mt8696-reg.h"
#include "mt8696-afe-debug.h"
#include "mt8696-afe-controls.h"
#include "../common/mtk-base-afe.h"
#include "../common/mtk-afe-platform-driver.h"
#include "../common/mtk-afe-fe-dai.h"

#define MT8696_ETDM1_OUT_MCLK_MULTIPLIER 256
#define MT8696_ETDM2_OUT_MCLK_MULTIPLIER 256
#define MT8696_ETDM2_IN_MCLK_MULTIPLIER 256
#define MT8696_ETDM_NORMAL_MAX_BCK_RATE 24576000

// TODO: revise registers to backup
static const unsigned int mt8696_afe_backup_list[] = {
	AUDIO_TOP_CON0,
	AUDIO_TOP_CON2,
	AUDIO_TOP_CON4,
	AUDIO_TOP_CON5,
	ASYS_TOP_CON,
	AFE_DL5_BASE,
	AFE_DL5_END,
	AFE_DL8_BASE,
	AFE_DL8_END,
	AFE_UL1_BASE,
	AFE_UL1_END,
	AFE_DAC_CON0,
	AFE_IRQ_MASK,
	ETDM_OUT1_CON0,
	ETDM_IN2_CON0,
	ETDM_OUT2_CON0,
};

static const struct snd_pcm_hardware mt8696_afe_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.buffer_bytes_max = 256 * 1024,
	.period_bytes_min = 64,
	.period_bytes_max = 128 * 1024,
	.periods_min = 2,
	.periods_max = 256,
	.fifo_size = 0,
};

struct mt8696_afe_rate {
	unsigned int rate;
	unsigned int reg_val;
};

static const struct mt8696_afe_rate mt8696_afe_fs_rates[] = {
	{ .rate = 8000, .reg_val = MT8696_FS_8K },
	{ .rate = 12000, .reg_val = MT8696_FS_12K },
	{ .rate = 16000, .reg_val = MT8696_FS_16K },
	{ .rate = 24000, .reg_val = MT8696_FS_24K },
	{ .rate = 32000, .reg_val = MT8696_FS_32K },
	{ .rate = 48000, .reg_val = MT8696_FS_48K },
	{ .rate = 96000, .reg_val = MT8696_FS_96K },
	{ .rate = 192000, .reg_val = MT8696_FS_192K },
	{ .rate = 384000, .reg_val = MT8696_FS_384K },
	{ .rate = 7350, .reg_val = MT8696_FS_7P35K },
	{ .rate = 11025, .reg_val = MT8696_FS_11P025K },
	{ .rate = 14700, .reg_val = MT8696_FS_14P7K },
	{ .rate = 22050, .reg_val = MT8696_FS_22P05K },
	{ .rate = 29400, .reg_val = MT8696_FS_29P4K },
	{ .rate = 44100, .reg_val = MT8696_FS_44P1K },
	{ .rate = 88200, .reg_val = MT8696_FS_88P2K },
	{ .rate = 176400, .reg_val = MT8696_FS_176P4K },
	{ .rate = 352800, .reg_val = MT8696_FS_352P8K },
};

static const struct mt8696_afe_rate mt8696_etdm_fs_rates[] = {
	{ .rate = 8000, .reg_val = MT8696_ETDM_FS_8K },
	{ .rate = 12000, .reg_val = MT8696_ETDM_FS_12K },
	{ .rate = 16000, .reg_val = MT8696_ETDM_FS_16K },
	{ .rate = 24000, .reg_val = MT8696_ETDM_FS_24K },
	{ .rate = 32000, .reg_val = MT8696_ETDM_FS_32K },
	{ .rate = 48000, .reg_val = MT8696_ETDM_FS_48K },
	{ .rate = 64000, .reg_val = MT8696_ETDM_FS_64K },
	{ .rate = 96000, .reg_val = MT8696_ETDM_FS_96K },
	{ .rate = 192000, .reg_val = MT8696_ETDM_FS_192K },
	{ .rate = 384000, .reg_val = MT8696_ETDM_FS_384K },
	{ .rate = 11025, .reg_val = MT8696_ETDM_FS_11P025K },
	{ .rate = 22050, .reg_val = MT8696_ETDM_FS_22P05K },
	{ .rate = 44100, .reg_val = MT8696_ETDM_FS_44P1K },
	{ .rate = 88200, .reg_val = MT8696_ETDM_FS_88P2K },
	{ .rate = 176400, .reg_val = MT8696_ETDM_FS_176P4K },
	{ .rate = 352800, .reg_val = MT8696_ETDM_FS_352P8K },
};

static int mt8696_afe_fs_timing(unsigned int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt8696_afe_fs_rates); i++)
		if (mt8696_afe_fs_rates[i].rate == rate)
			return mt8696_afe_fs_rates[i].reg_val;

	return -EINVAL;
}

static int mt8696_etdm_fs_timing(unsigned int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt8696_etdm_fs_rates); i++)
		if (mt8696_etdm_fs_rates[i].rate == rate)
			return mt8696_etdm_fs_rates[i].reg_val;

	return -EINVAL;
}

static int mt8696_afe_irq_direction_enable(struct mtk_base_afe *afe,
	int irq_id,
	int direction)
{
	struct mtk_base_afe_irq *irq;

	if (irq_id >= MT8696_AFE_IRQ_NUM)
		return -1;

	irq = &afe->irqs[irq_id];

	if (direction == MT8696_AFE_IRQ_DIR_MCU) {
		regmap_update_bits(afe->regmap, ASYS_IRQ_MASK,
		       (1 << irq->irq_data->irq_clr_shift),
		       0);
		regmap_update_bits(afe->regmap, AFE_IRQ_MASK,
		       (1 << irq->irq_data->irq_clr_shift),
		       (1 << irq->irq_data->irq_clr_shift));
	} else if (direction == MT8696_AFE_IRQ_DIR_DSP) {
		regmap_update_bits(afe->regmap, ASYS_IRQ_MASK,
		       (1 << irq->irq_data->irq_clr_shift),
		       (1 << irq->irq_data->irq_clr_shift));
		regmap_update_bits(afe->regmap, AFE_IRQ_MASK,
		       (1 << irq->irq_data->irq_clr_shift),
		       0);
	} else {
		regmap_update_bits(afe->regmap, ASYS_IRQ_MASK,
		       (1 << irq->irq_data->irq_clr_shift),
		       (1 << irq->irq_data->irq_clr_shift));
		regmap_update_bits(afe->regmap, AFE_IRQ_MASK,
		       (1 << irq->irq_data->irq_clr_shift),
		       (1 << irq->irq_data->irq_clr_shift));
	}
	return 0;
}

bool mt8696_afe_rate_supported(unsigned int rate, unsigned int id)
{
	switch (id) {
	case MT8696_AFE_IO_ETDM1_OUT:
		/* FALLTHROUGH */
	case MT8696_AFE_IO_ETDM2_OUT:
		/* FALLTHROUGH */
	case MT8696_AFE_IO_ETDM2_IN:
		if (rate >= 8000 && rate <= 384000)
			return true;
		break;
	default:
		break;
	}

	return false;
}

bool mt8696_afe_channel_supported(unsigned int channel, unsigned int id)
{
	switch (id) {
	case MT8696_AFE_IO_ETDM1_OUT:
		if (channel >= 1 && channel <= 16)
			return true;
		break;
	case MT8696_AFE_IO_ETDM2_OUT:
		/* FALLTHROUGH */
	case MT8696_AFE_IO_ETDM2_IN:
		if (channel >= 1 && channel <= 8)
			return true;
		break;
	default:
		break;
	}

	return false;
}

static void mt8696_afe_inc_etdm_occupy(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int stream)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned long flags;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	etdm_data->occupied[stream]++;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);
}

static void mt8696_afe_dec_etdm_occupy(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int stream)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned long flags;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	etdm_data->occupied[stream]--;
	if (etdm_data->occupied[stream] < 0)
		etdm_data->occupied[stream] = 0;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);
}

static unsigned int mt8696_afe_tdm_ch_fixup(unsigned int channels)
{
	if (channels > 8)
		return 16;
	else if (channels > 4)
		return 8;
	else if (channels > 2)
		return 4;
	else
		return 2;
}

static const struct mt8696_etdm_ctrl_reg etdm_ctrl_reg[MT8696_ETDM_SETS][2] = {
	{
		{
			.con0 = ETDM_OUT1_CON0,
			.con1 = ETDM_OUT1_CON1,
			.con2 = ETDM_OUT1_CON2,
			.con3 = ETDM_OUT1_CON3,
			.con4 = ETDM_OUT1_CON4,
		},
		{
			.con0 = -1,
			.con1 = -1,
			.con2 = -1,
			.con3 = -1,
			.con4 = -1,
		},
	},
	{
		{
			.con0 = ETDM_OUT2_CON0,
			.con1 = ETDM_OUT2_CON1,
			.con2 = ETDM_OUT2_CON2,
			.con3 = ETDM_OUT2_CON3,
			.con4 = ETDM_OUT2_CON4,
		},
		{
			.con0 = ETDM_IN2_CON0,
			.con1 = ETDM_IN2_CON1,
			.con2 = ETDM_IN2_CON2,
			.con3 = ETDM_IN2_CON3,
			.con4 = ETDM_IN2_CON4,
		},
	},
};

static void mt8696_afe_enable_etdm(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int stream)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	unsigned int en_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	unsigned long flags;
	bool need_update = false;

	dev_info(afe->dev, "%s, etdm_set %u\n", __func__, etdm_set);

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	etdm_data->active[stream]++;
	if (etdm_data->active[stream] == 1)
		need_update = true;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	if (need_update) {
		regmap_update_bits(afe->regmap, en_reg, 0x1, 0x1);
	}
}

static void mt8696_afe_disable_etdm(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int stream)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	bool slave_mode = etdm_data->slave_mode[stream];
	unsigned int reg;
	unsigned long flags;
	bool need_update = false;

	dev_info(afe->dev, "%s, etdm_set %u\n", __func__, etdm_set);

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	etdm_data->active[stream]--;
	if (etdm_data->active[stream] == 0)
		need_update = true;
	else if (etdm_data->active[stream] < 0)
		etdm_data->active[stream] = 0;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	if (need_update) {
		reg = etdm_ctrl_reg[etdm_set][stream].con0;
		regmap_update_bits(afe->regmap, reg, 0x1, 0x0);

		if (slave_mode) {
			reg = etdm_ctrl_reg[etdm_set][stream].con4;
			regmap_update_bits(afe->regmap, reg,
					   ETDM_CON4_ASYNC_RESET,
					   ETDM_CON4_ASYNC_RESET);
			regmap_update_bits(afe->regmap, reg,
					   ETDM_CON4_ASYNC_RESET,
					   0);
		}
	}
}

static int mt8696_afe_prepare_etdm_out(struct mtk_base_afe *afe,
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai,
	unsigned int etdm_set)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	unsigned int rate = dai->rate;
	unsigned int channels = dai->channels;
	unsigned int tdm_channels;
	int bit_width = dai->sample_bits;
	unsigned int stream = SNDRV_PCM_STREAM_PLAYBACK;
	unsigned int data_mode = etdm_data->data_mode[stream];
	unsigned int lrck_width = etdm_data->lrck_width[stream];
	bool slave_mode = etdm_data->slave_mode[stream];
	bool lrck_inv = etdm_data->lrck_inv[stream];
	bool bck_inv = etdm_data->bck_inv[stream];
	unsigned int fmt = etdm_data->format[stream];
	unsigned int ctrl_reg;
	unsigned int ctrl_mask;
	unsigned int val = 0;
	unsigned int bck;

	dev_info(dai->dev, "%s#%u rate:%u ch:%u bits:%u slave:%d data:%u\n",
		__func__, etdm_set + 1, rate, channels, bit_width,
		slave_mode, data_mode);

	tdm_channels = (data_mode == MT8696_ETDM_DATA_ONE_PIN) ?
		mt8696_afe_tdm_ch_fixup(channels) : 2;

	bck = rate * tdm_channels * bit_width;

	val |= ETDM_CON0_BIT_LEN(bit_width);
	val |= ETDM_CON0_WORD_LEN(bit_width);
	val |= ETDM_CON0_FORMAT(fmt);
	val |= ETDM_CON0_CH_NUM(tdm_channels);

	if (slave_mode) {
		val |= ETDM_CON0_SLAVE_MODE;
		if (lrck_inv)
			val |= ETDM_CON0_SLAVE_LRCK_INV;
		if (bck_inv)
			val |= ETDM_CON0_SLAVE_BCK_INV;
	} else {
		if (lrck_inv)
			val |= ETDM_CON0_MASTER_LRCK_INV;
		if (bck_inv)
			val |= ETDM_CON0_MASTER_BCK_INV;
	}

	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	ctrl_mask = ETDM_OUT_CON0_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	val = 0;

	val |= ETDM_CON1_LRCK_MANUAL_MODE;

	if (!slave_mode) {
		val |= ETDM_CON1_MCLK_OUTPUT;

		if (bck > MT8696_ETDM_NORMAL_MAX_BCK_RATE)
			val |= ETDM_CON1_BCK_FROM_DIVIDER;
	}

	if (lrck_width > 0)
		val |= ETDM_OUT_CON1_LRCK_WIDTH(lrck_width);
	else
		val |= ETDM_OUT_CON1_LRCK_WIDTH(bit_width);

	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con1;
	ctrl_mask = ETDM_OUT_CON1_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	val = ETDM_OUT_CON4_FS(mt8696_etdm_fs_timing(rate));
	ctrl_mask = ETDM_OUT_CON4_CTRL_MASK;

	if (etdm_set == MT8696_ETDM2) {
		if (slave_mode)
			val |= ETDM_OUT_CON4_CONN_FS(MT8696_FS_ETDMOUT2_1X_EN);
		else
			val |= ETDM_OUT_CON4_CONN_FS(
				mt8696_etdm_fs_timing(rate));
		ctrl_mask |= ETDM_OUT_CON4_INTERCONN_EN_SEL_MASK;
	}

	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	return 0;
}

static int mt8696_afe_prepare_etdm_in(struct mtk_base_afe *afe,
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai,
	unsigned int etdm_set)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	unsigned int rate = dai->rate;
	unsigned int channels = dai->channels;
	unsigned int tdm_channels;
	unsigned int bit_width = dai->sample_bits;
	unsigned int stream = SNDRV_PCM_STREAM_CAPTURE;
	unsigned int data_mode = etdm_data->data_mode[stream];
	unsigned int lrck_width = etdm_data->lrck_width[stream];
	bool slave_mode = etdm_data->slave_mode[stream];
	unsigned int clk_mode = etdm_data->clock_mode;
	bool lrck_inv = etdm_data->lrck_inv[stream];
	bool bck_inv = etdm_data->bck_inv[stream];
	unsigned int fmt = etdm_data->format[stream];
	unsigned int ctrl_reg;
	unsigned int ctrl_mask;
	unsigned int val = 0;
	unsigned int bck;

	dev_info(dai->dev, "%s#%u rate:%u ch:%u bits:%u slave:%d data:%u\n",
		__func__, etdm_set + 1, rate, channels, bit_width,
		slave_mode, data_mode);

	tdm_channels = (data_mode == MT8696_ETDM_DATA_ONE_PIN) ?
		mt8696_afe_tdm_ch_fixup(channels) : 2;

	val |= ETDM_CON0_BIT_LEN(bit_width);
	val |= ETDM_CON0_WORD_LEN(bit_width);
	val |= ETDM_CON0_FORMAT(fmt);
	val |= ETDM_CON0_CH_NUM(tdm_channels);

	if (clk_mode == MT8696_ETDM_SHARED_CLOCK)
		val |= ETDM_CON0_SYNC_MODE;

	bck = rate * tdm_channels * bit_width;

	if (slave_mode) {
		val |= ETDM_CON0_SLAVE_MODE;
		if (lrck_inv)
			val |= ETDM_CON0_SLAVE_LRCK_INV;
		if (bck_inv)
			val |= ETDM_CON0_SLAVE_BCK_INV;
	} else {
		if (lrck_inv)
			val |= ETDM_CON0_MASTER_LRCK_INV;
		if (bck_inv)
			val |= ETDM_CON0_MASTER_BCK_INV;
	}

	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	ctrl_mask = ETDM_IN_CON0_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	val = 0;

	val |= ETDM_CON1_LRCK_MANUAL_MODE;

	if (!slave_mode)
		val |= ETDM_CON1_MCLK_OUTPUT;
	else if (bck > MT8696_ETDM_NORMAL_MAX_BCK_RATE)
		val |= ETDM_CON1_BCK_FROM_DIVIDER;

	if (lrck_width > 0)
		val |= ETDM_IN_CON1_LRCK_WIDTH(lrck_width);
	else
		val |= ETDM_IN_CON1_LRCK_WIDTH(bit_width);

	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con1;
	ctrl_mask = ETDM_IN_CON1_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	val = ETDM_IN_CON3_FS(mt8696_etdm_fs_timing(rate));

	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con3;
	ctrl_mask = ETDM_IN_CON3_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	val = 0;
	if (data_mode == MT8696_ETDM_DATA_MULTI_PIN) {
		val |= ETDM_IN_CON2_MULTI_IP_2CH_MODE |
		       ETDM_IN_CON2_MULTI_IP_CH(tdm_channels);
	}

	val |= ETDM_IN_CON2_UPDATE_POINT_AUTO_DIS |
	       ETDM_IN_CON2_UPDATE_POINT(1);

	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con2;
	ctrl_mask = ETDM_IN_CON2_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	if (clk_mode == MT8696_ETDM_SHARED_CLOCK) {
		regmap_update_bits(afe->regmap, ETDM_COWORK_CON2,
				   ETDM_COWORK_CON2_IN2_SYNC_SEL_MASK,
				   ETDM_COWORK_CON2_IN2_SYNC_SEL_OUT2);
	}

	return 0;
}

static int mt8696_afe_etdm1_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_etdm_data *etdm_data =
		&afe_priv->etdm_data[MT8696_ETDM1];
	unsigned int clk_mode = etdm_data->clock_mode;
	unsigned int stream = substream->stream;

	dev_dbg(dai->dev, "%s stream %u clk_mode %u\n",
		__func__, stream, clk_mode);

	mt8696_afe_enable_main_clk(afe);

	if (stream == SNDRV_PCM_STREAM_CAPTURE)
		dev_info(dai->dev, "%s stream %u is not supported!\n",
		__func__, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK ||
	    clk_mode == MT8696_ETDM_SHARED_CLOCK) {
		mt8696_afe_enable_clk(afe,
			afe_priv->clocks[MT8696_CLK_APLL12_DIV3]);
		mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_ETDM_OUT1);
	}

	return 0;
}

static void mt8696_afe_etdm1_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8696_ETDM1;
	struct mt8696_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int clk_mode = etdm_data->clock_mode;
	unsigned int stream = substream->stream;
	bool reset_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(clk_mode == MT8696_ETDM_SHARED_CLOCK);

	dev_info(dai->dev, "%s stream %u clk_mode %u occupied %u\n",
		__func__, stream, clk_mode, etdm_data->occupied[stream]);

	if (reset_out_change) {
		mt8696_afe_dec_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (!afe->force_clk_on) {
			if (etdm_data->occupied[stream] == 0) {
				mt8696_afe_disable_etdm(afe, etdm_set,
					SNDRV_PCM_STREAM_PLAYBACK);
			}

			mt8696_afe_disable_top_cg(afe,
				MT8696_TOP_CG_ETDM_OUT1);
			mt8696_afe_disable_clk(afe,
				afe_priv->clocks[MT8696_CLK_APLL12_DIV3]);
		}
	}
	mt8696_afe_disable_main_clk(afe);
}

static int mt8696_afe_etdm1_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8696_ETDM1;
	struct mt8696_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int clk_mode = etdm_data->clock_mode;
	unsigned int stream = substream->stream;
	bool apply_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(clk_mode == MT8696_ETDM_SHARED_CLOCK);
	unsigned int rate = dai->rate;
	unsigned int mclk = afe_priv->etdm_data[etdm_set].mclk_freq[stream];
	int ret;
#if defined(FORCE_CLK_SWITCH_FEATURE_ON)
	uint32_t format = afe->hdmi_audio_format;
	unsigned int channels = dai->channels;
	int bit_width = dai->sample_bits;
#endif

	dev_dbg(dai->dev, "%s stream %u clk_mode %u occupied %d\n",
		__func__, stream, clk_mode, etdm_data->occupied[stream]);

	if (etdm_data->occupied[stream])
		return 0;

	if (apply_out_change) {
		mt8696_afe_inc_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

#if defined(FORCE_CLK_SWITCH_FEATURE_ON)
		if (afe->force_clk_on) {
			if ((afe->cached_hdmi_audio_format == format) &&
			    (afe->cached_hdmi_audio_sample_rate == rate) &&
			    (afe->cached_hdmi_audio_channels == channels) &&
			    (afe->cached_hdmi_audio_bit_depth == bit_width)) {
				afe->need_hdmi_toggle = false;
				return 0;
			}

			mt8696_afe_disable_etdm(afe, etdm_set,
				SNDRV_PCM_STREAM_PLAYBACK);
			afe->need_hdmi_toggle = true;
			afe->cached_hdmi_audio_format = format;
			afe->cached_hdmi_audio_sample_rate = rate;
			afe->cached_hdmi_audio_channels = channels;
			afe->cached_hdmi_audio_bit_depth = bit_width;
		}
#endif

		ret = mt8696_afe_prepare_etdm_out(afe, substream, dai,
						  etdm_set);
		if (ret)
			return ret;

		if (mclk == 0)
			mclk = MT8696_ETDM1_OUT_MCLK_MULTIPLIER * rate;
		mt8696_afe_set_clk_parent(afe,
			afe_priv->clocks[MT8696_CLK_ETDM_OUT1_M_SEL],
			(rate % 8000) ?
			afe_priv->clocks[MT8696_CLK_FAPLL1] :
			afe_priv->clocks[MT8696_CLK_FAPLL2]);
		mt8696_afe_set_clk_rate(afe,
			afe_priv->clocks[MT8696_CLK_APLL12_DIV3],
			mclk);
		mt8696_afe_enable_etdm(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);
	}

	return 0;
}

static int mt8696_afe_etdm2_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_etdm_data *etdm_data =
		&afe_priv->etdm_data[MT8696_ETDM2];
	unsigned int clk_mode = etdm_data->clock_mode;
	unsigned int stream = substream->stream;

	dev_info(dai->dev, "%s stream %u clk_mode %u occupied %d\n",
		__func__, stream, clk_mode, etdm_data->occupied[stream]);

	mt8696_afe_enable_main_clk(afe);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK ||
	    clk_mode == MT8696_ETDM_SHARED_CLOCK) {
		mt8696_afe_enable_clk(afe,
			afe_priv->clocks[MT8696_CLK_APLL12_DIV2]);
		mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_ETDM_OUT2);
	}

	if (stream == SNDRV_PCM_STREAM_CAPTURE ||
	    clk_mode == MT8696_ETDM_SHARED_CLOCK) {
		mt8696_afe_enable_clk(afe,
			afe_priv->clocks[MT8696_CLK_APLL12_DIV0]);
		mt8696_afe_enable_top_cg(afe, MT8696_TOP_CG_ETDM_IN);
	}

	return 0;
}

static void mt8696_afe_etdm2_shutdown(struct snd_pcm_substream *substream,
				      struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8696_ETDM2;
	struct mt8696_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int clk_mode = etdm_data->clock_mode;
	unsigned int stream = substream->stream;
	bool reset_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(clk_mode == MT8696_ETDM_SHARED_CLOCK);
	bool reset_in_change = (stream == SNDRV_PCM_STREAM_CAPTURE) ||
		(clk_mode == MT8696_ETDM_SHARED_CLOCK);

	dev_info(dai->dev, "%s stream %u clk_mode %u occupied %d\n",
		__func__, stream, clk_mode, etdm_data->occupied[stream]);

	if (reset_out_change)
		mt8696_afe_dec_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

	if (reset_in_change)
		mt8696_afe_dec_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_CAPTURE);

	if (etdm_data->occupied[stream] == 0) {
		if (reset_out_change) {
			mt8696_afe_disable_etdm(afe, etdm_set,
				SNDRV_PCM_STREAM_PLAYBACK);
		}

		if (reset_in_change) {
			mt8696_afe_disable_etdm(afe, etdm_set,
				SNDRV_PCM_STREAM_CAPTURE);
		}
	}

	if (reset_out_change) {
		mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_ETDM_OUT2);
		mt8696_afe_disable_clk(afe,
			afe_priv->clocks[MT8696_CLK_APLL12_DIV2]);
	}

	if (reset_in_change) {
		mt8696_afe_disable_top_cg(afe, MT8696_TOP_CG_ETDM_IN);
		mt8696_afe_disable_clk(afe,
			afe_priv->clocks[MT8696_CLK_APLL12_DIV0]);
	}

	mt8696_afe_disable_main_clk(afe);
}

static int mt8696_afe_etdm2_prepare(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8696_ETDM2;
	struct mt8696_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	unsigned int clk_mode = etdm_data->clock_mode;
	unsigned int stream = substream->stream;
	bool apply_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(clk_mode == MT8696_ETDM_SHARED_CLOCK);
	bool apply_in_change = (stream == SNDRV_PCM_STREAM_CAPTURE) ||
		(clk_mode == MT8696_ETDM_SHARED_CLOCK);
	unsigned int rate = dai->rate;
	unsigned int mclk = afe_priv->etdm_data[etdm_set].mclk_freq[stream];
	int ret;

	dev_info(dai->dev, "%s stream %u clk_mode %u occupied %d\n",
		__func__, stream, clk_mode, etdm_data->occupied[stream]);

	if (etdm_data->occupied[stream])
		return 0;

	if (apply_out_change)
		mt8696_afe_inc_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

	if (apply_in_change)
		mt8696_afe_inc_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_CAPTURE);

	if (apply_out_change) {
		ret = mt8696_afe_prepare_etdm_out(afe, substream, dai,
						  etdm_set);
		if (ret)
			return ret;
	}

	if (apply_in_change) {
		ret = mt8696_afe_prepare_etdm_in(afe, substream, dai,
						 etdm_set);
		if (ret)
			return ret;
	}

	if (apply_out_change) {
		if (mclk == 0)
			mclk = MT8696_ETDM2_OUT_MCLK_MULTIPLIER * rate;
		mt8696_afe_set_clk_parent(afe,
			afe_priv->clocks[MT8696_CLK_ETDM_OUT2_M_SEL],
			(rate % 8000) ?
			afe_priv->clocks[MT8696_CLK_FAPLL1] :
			afe_priv->clocks[MT8696_CLK_FAPLL2]);
		mt8696_afe_set_clk_rate(afe,
			afe_priv->clocks[MT8696_CLK_APLL12_DIV2],
			mclk);
		mt8696_afe_enable_etdm(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);
	}

	if (apply_in_change) {
		if (mclk == 0)
			mclk = MT8696_ETDM2_IN_MCLK_MULTIPLIER * rate;
		mt8696_afe_set_clk_parent(afe,
			afe_priv->clocks[MT8696_CLK_ETDM_IN2_M_SEL],
			(rate % 8000) ?
			afe_priv->clocks[MT8696_CLK_FAPLL1] :
			afe_priv->clocks[MT8696_CLK_FAPLL2]);
		mt8696_afe_set_clk_rate(afe,
			afe_priv->clocks[MT8696_CLK_APLL12_DIV0],
			mclk);
		mt8696_afe_enable_etdm(afe, etdm_set,
			SNDRV_PCM_STREAM_CAPTURE);
	}

	return 0;
}

// be_clients will get connected after startup callback
static bool mt8696_match_1st_be_cpu_dai(struct snd_pcm_substream *substream,
	char *dai_name)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	int stream = substream->stream;
	struct snd_soc_dpcm *dpcm;
	int i = 0;

	list_for_each_entry(dpcm, &fe->dpcm[stream].be_clients, list_be) {
		struct snd_soc_pcm_runtime *be = dpcm->be;

		if (i > 0)
			break;

		if (!strcmp(be->cpu_dai->name, dai_name))
			return true;
		i++;
	}

	return false;
}

static int mt8696_memif_fs(struct snd_pcm_substream *substream,
	unsigned int rate)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mt8696_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	int fs;

	if (dai_id == MT8696_AFE_MEMIF_UL1)
		return MT8696_FS_ETDMIN2_NX_EN;

	fs = mt8696_afe_fs_timing(rate);

	if (!fe_data->slave_mode)
		return fs;

	// slave mode
	switch (dai_id) {
	case MT8696_AFE_MEMIF_DL8:
	case MT8696_AFE_MEMIF_DL5:
		if (mt8696_match_1st_be_cpu_dai(substream, "ETDM1_OUT"))
			fs = MT8696_FS_ETDMOUT1_1X_EN;
		else if (mt8696_match_1st_be_cpu_dai(substream, "ETDM2_OUT"))
			fs = MT8696_FS_ETDMOUT2_1X_EN;
		break;
	default:
		break;
	}

	return fs;
}

static int mt8696_irq_fs(struct snd_pcm_substream *substream,
	unsigned int rate)
{
	int irq_fs = mt8696_memif_fs(substream, rate);

	if (irq_fs == MT8696_FS_ETDMIN1_NX_EN)
		irq_fs = MT8696_FS_ETDMIN1_1X_EN;
	else if (irq_fs == MT8696_FS_ETDMIN2_NX_EN)
		irq_fs = MT8696_FS_ETDMIN2_1X_EN;

	return irq_fs;
}

static int mt8696_alloc_dmabuf(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
	const struct mtk_base_memif_data *data = memif->data;
	struct mt8696_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	const size_t request_size = params_buffer_bytes(params);
	int ret;

	if (request_size > fe_data->sram_size) {
		ret = snd_pcm_lib_malloc_pages(substream, request_size);
		if (ret < 0) {
			dev_info(afe->dev,
				"%s %s malloc pages %zu bytes failed %d\n",
				__func__, data->name, request_size, ret);
			return ret;
		}

		fe_data->use_sram = false;

		mt8696_afe_block_dpidle(afe);
	} else {
		struct snd_dma_buffer *dma_buf = &substream->dma_buffer;

		dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
		dma_buf->dev.dev = substream->pcm->card->dev;
		dma_buf->area = (unsigned char *)fe_data->sram_vir_addr;
		dma_buf->addr = fe_data->sram_phy_addr;
		dma_buf->bytes = request_size;
		snd_pcm_set_runtime_buffer(substream, dma_buf);

		fe_data->use_sram = true;
	}

	return 0;
}

static int mt8696_free_dmabuf(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mt8696_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	int ret = 0;

	if (fe_data->use_sram) {
		snd_pcm_set_runtime_buffer(substream, NULL);
	} else {
		ret = snd_pcm_lib_free_pages(substream);

		mt8696_afe_unblock_dpidle(afe);
	}

	return ret;
}

int mt8696_afe_fe_startup(struct snd_pcm_substream *substream,
			  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int memif_num = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[memif_num];
	const struct mtk_base_memif_data *data = memif->data;
	const struct snd_pcm_hardware *mtk_afe_hardware = afe->mtk_afe_hardware;
	int ret;

	dev_info(dai->dev, "%s %s\n", __func__, data->name);

	memif->substream = substream;

	if (memif->data->buffer_bytes_align > 0)
		snd_pcm_hw_constraint_step(substream->runtime, 0,
				   SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
				   memif->data->buffer_bytes_align);
	else
		snd_pcm_hw_constraint_step(substream->runtime, 0,
				   SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 16);

	/* enable agent */
	regmap_update_bits(afe->regmap, memif->data->agent_disable_reg,
			   1 << memif->data->agent_disable_shift,
			   0 << memif->data->agent_disable_shift);

	snd_soc_set_runtime_hwparams(substream, mtk_afe_hardware);

	mt8696_afe_irq_direction_enable(afe,
		memif->irq_usage,
		MT8696_AFE_IRQ_DIR_MCU);

	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		dev_info(afe->dev, "snd_pcm_hw_constraint_integer failed\n");
		return ret;
	}

	mt8696_afe_enable_main_clk(afe);

	return ret;
}

static void mt8696_afe_fe_shutdown(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mtk_base_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	const struct mtk_base_memif_data *data = memif->data;

	dev_dbg(dai->dev, "%s %s\n", __func__, data->name);

	mtk_afe_fe_shutdown(substream, dai);
	mt8696_afe_disable_main_clk(afe);

#if defined(FORCE_CLK_SWITCH_FEATURE_ON)
	memif->prepared = false;
	memif->avsync_mode = false;
#endif
}

int mt8696_afe_configure_dma(struct mtk_base_afe *afe,
	const struct mtk_base_memif_data *data,
	dma_addr_t dma_addr,
	size_t dma_size,
	unsigned int fs,
	unsigned int channels)
{
	int msb_at_bit33 = 0;
	int msb2_at_bit33 = 0;
	unsigned int phys_addr;

#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	msb_at_bit33 = upper_32_bits(dma_addr) ? 1 : 0;
	msb2_at_bit33 = upper_32_bits(dma_addr + dma_size - 1) ? 1 : 0;
#endif
	phys_addr = lower_32_bits(dma_addr);

	/* start */
	regmap_write(afe->regmap, data->reg_ofs_base, phys_addr);

	/* end */
	regmap_write(afe->regmap,
			 data->reg_ofs_base + 8/*AFE_BASE_END_OFFSET*/,
			 phys_addr + dma_size - 1 + data->buffer_end_shift);

	/* set MSB to 33-bit */
	regmap_update_bits(afe->regmap, data->msb_reg,
			       1 << data->msb_shift,
			       msb_at_bit33 << data->msb_shift);

	/* set buf end MSB to 33-bit */
	regmap_update_bits(afe->regmap, data->msb2_reg,
			       1 << data->msb2_shift,
			       msb2_at_bit33 << data->msb2_shift);

	/* set channel */
	if (data->mono_shift >= 0) {
		unsigned int mono = (channels == 1) ? 1 : 0;

		regmap_update_bits(afe->regmap, data->mono_reg,
				       1 << data->mono_shift,
				       mono << data->mono_shift);
	}

	/* set rate */
	if (data->fs_shift >= 0) {
		regmap_update_bits(afe->regmap, data->fs_reg,
				       data->fs_maskbit << data->fs_shift,
				       fs << data->fs_shift);
	}

	return 0;
}

static int mt8696_afe_fe_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	const int dai_id = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
	const struct mtk_base_memif_data *data = memif->data;
	unsigned int channels = params_channels(params);
	unsigned int rate = params_rate(params);
	int ret, fs = 0;

	dev_info(afe->dev, "%s [%s] rate %u ch %u bit %u period %u-%u\n",
		__func__, data->name, params_rate(params), channels,
		params_width(params), params_period_size(params),
		params_periods(params));

	if (afe->alloc_dmabuf)
		ret = afe->alloc_dmabuf(substream, params, dai);
	else
		ret = snd_pcm_lib_malloc_pages(substream,
					       params_buffer_bytes(params));

	if (ret < 0)
		return ret;

	memif->phys_buf_addr = lower_32_bits(substream->runtime->dma_addr);
	memif->buffer_size = substream->runtime->dma_bytes;

	fs = afe->memif_fs(substream, rate);
	if (fs < 0)
		return -EINVAL;
#if !defined(FORCE_CLK_SWITCH_FEATURE_ON)
	ret = mtk_afe_configure_dma(afe, data,
		substream->runtime->dma_addr,
		substream->runtime->dma_bytes,
		fs, channels);
#endif
	return 0;
}

static int mt8696_afe_fe_prepare(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
#if defined(FORCE_CLK_SWITCH_FEATURE_ON)
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mtk_base_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	const struct mtk_base_memif_data *data = memif->data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	unsigned int counter = runtime->period_size;
	int fs;
	int hd_audio = 0;
	struct mtk_base_afe_irq *irqs = &afe->irqs[memif->irq_usage];
	const struct mtk_base_irq_data *irq_data = irqs->irq_data;

	dev_dbg(dai->dev, "%s %s\n", __func__, data->name);

	if (!memif->prepared) {
		/* set irq counter */
		if (irq_data->irq_cnt_reg >= 0)
			regmap_update_bits(afe->regmap,
				irq_data->irq_cnt_reg,
				irq_data->irq_cnt_maskbit
				<< irq_data->irq_cnt_shift,
				counter << irq_data->irq_cnt_shift);

		if (afe->irq_engen_apll_sel)
			afe->irq_engen_apll_sel(substream);
		/* set irq fs */
		fs = afe->irq_fs(substream, runtime->rate);

		if (fs < 0)
			return -EINVAL;

		mt8696_afe_configure_dma(afe, data,
			substream->runtime->dma_addr,
			substream->runtime->dma_bytes,
			fs, runtime->channels);

		if (irq_data->irq_fs_reg >= 0)
			regmap_update_bits(afe->regmap,
				irq_data->irq_fs_reg,
				irq_data->irq_fs_maskbit
				<< irq_data->irq_fs_shift,
				fs << irq_data->irq_fs_shift);

		if (data->ch_config_shift >= 0)
			regmap_update_bits(afe->regmap,
				data->ch_config_reg,
				0x1f << data->ch_config_shift,
				runtime->channels << data->ch_config_shift);

		if (data->int_odd_shift >= 0) {
			unsigned int odd_en = (runtime->channels == 1) ? 1 : 0;

			regmap_update_bits(afe->regmap, data->int_odd_reg,
					   1 << data->int_odd_shift,
					   odd_en << data->int_odd_shift);
		}

		/* set hd mode */
		switch (substream->runtime->format) {
		case SNDRV_PCM_FORMAT_S16_LE:
			hd_audio = 0;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			hd_audio = 1;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			hd_audio = 1;
			break;
		case SNDRV_PCM_FORMAT_S24_3LE:
			hd_audio = 1;
			break;
		default:
			dev_info(afe->dev, "%s() error: unsupported format %d\n",
				__func__, substream->runtime->format);
			break;
		}

		if (data->hd_reg >= 0)
			regmap_update_bits(afe->regmap, data->hd_reg,
					       1 << data->hd_shift,
					       hd_audio << data->hd_shift);

		if (memif->use_sw_irq) {
			memif->prev_hw_cur = 0;
			hrtimer_cancel(&memif->dma_hrt);
		}
		memif->prepared = true;
		memif->buf_frame_size = runtime->frame_bits / 8;
		afe->cached_sample_rate = runtime->rate;
		afe->cached_channels = runtime->channels;
	}
	return 0;
#else
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mtk_base_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	const struct mtk_base_memif_data *data = memif->data;
	int hd_audio = 0;

	dev_info(dai->dev, "%s %s\n", __func__, data->name);

	/* set hd mode */
	switch (substream->runtime->format) {
	case SNDRV_PCM_FORMAT_S16_LE:
		hd_audio = 0;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		hd_audio = 1;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		hd_audio = 1;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		hd_audio = 1;
		break;
	default:
		dev_info(afe->dev, "%s() error: unsupported format %d\n",
			__func__, substream->runtime->format);
		break;
	}

	if (data->hd_reg >= 0)
		regmap_update_bits(afe->regmap, data->hd_reg,
				       1 << data->hd_shift,
				       hd_audio << data->hd_shift);

	if (memif->use_sw_irq) {
		memif->prev_hw_cur = 0;
		hrtimer_cancel(&memif->dma_hrt);
	}
	memif->prepared = true;
	memif->buf_frame_size = runtime->frame_bits / 8;
	afe->cached_sample_rate = runtime->rate;
	afe->cached_channels = runtime->channels;
	return 0;
#endif
}

int mt8696_afe_fe_trigger(struct snd_pcm_substream *substream, int cmd,
			  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mtk_base_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	const struct mtk_base_memif_data *data = memif->data;
	struct mtk_base_afe_irq *irqs = &afe->irqs[memif->irq_usage];
	const struct mtk_base_irq_data *irq_data = irqs->irq_data;
#if !defined(FORCE_CLK_SWITCH_FEATURE_ON)
	struct snd_pcm_runtime * const runtime = substream->runtime;
	unsigned int counter = runtime->period_size;
	int fs;
#endif
	unsigned long flags;
	struct timespec ts;

	dev_dbg(afe->dev, "%s %s cmd=%d\n", __func__, data->name, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (data->enable_shift >= 0)
			regmap_update_bits(afe->regmap,
					       data->enable_reg,
					       1 << data->enable_shift,
					       1 << data->enable_shift);
#if !defined(FORCE_CLK_SWITCH_FEATURE_ON)
		/* set irq counter */
		if (irq_data->irq_cnt_reg >= 0)
			regmap_update_bits(afe->regmap,
				irq_data->irq_cnt_reg,
				irq_data->irq_cnt_maskbit
				<< irq_data->irq_cnt_shift,
				counter << irq_data->irq_cnt_shift);

		if (afe->irq_engen_apll_sel)
			afe->irq_engen_apll_sel(substream);
		/* set irq fs */
		fs = afe->irq_fs(substream, runtime->rate);

		if (fs < 0)
			return -EINVAL;

		if (irq_data->irq_fs_reg >= 0)
			regmap_update_bits(afe->regmap,
				irq_data->irq_fs_reg,
				irq_data->irq_fs_maskbit
				<< irq_data->irq_fs_shift,
				fs << irq_data->irq_fs_shift);
#endif
		if (!memif->use_sw_irq) {
			/* enable interrupt */
			regmap_update_bits(afe->regmap,
				irq_data->irq_en_reg,
				1 << irq_data->irq_en_shift,
				1 << irq_data->irq_en_shift);
		} else {
			hrtimer_start(&memif->dma_hrt,
				ns_to_ktime(memif->period_time_ns),
				HRTIMER_MODE_REL);
		}
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			getrawmonotonic(&ts);
			afe->hdmi_dac_timestamp = timespec_to_ns(&ts);
			afe->hdmi_dac_state = 1;
		}
		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		regmap_update_bits(afe->regmap, data->enable_reg,
				       1 << data->enable_shift, 0);
		/* disable interrupt */
		regmap_update_bits(afe->regmap, irq_data->irq_en_reg,
				       1 << irq_data->irq_en_shift,
				       0 << irq_data->irq_en_shift);
		/* and clear pending IRQ */
		regmap_write(afe->regmap, irq_data->irq_clr_reg,
				 1 << irq_data->irq_clr_shift);
		if (memif->avsync_mode) {
			spin_lock_irqsave(&memif->buf_info_lock, flags);
			memif->buf_read = memif->buf_write = 0;
			memif->buf_reset = true;
			spin_unlock_irqrestore(&memif->buf_info_lock, flags);
		}
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			getrawmonotonic(&ts);
			afe->hdmi_dac_timestamp = timespec_to_ns(&ts);
			afe->hdmi_dac_state = 0;
		}
		return 0;
	default:
		return -EINVAL;
	}
}

static int mt8696_afe_fe_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mt8696_fe_dai_data *fe_data = &afe_priv->fe_data[dai->id];

	dev_dbg(dai->dev, "%s fmt 0x%x\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		fe_data->slave_mode = true;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		fe_data->slave_mode = false;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mt8696_afe_etdm_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = (dai->id == MT8696_AFE_IO_ETDM1_OUT) ?
				 MT8696_ETDM1 : MT8696_ETDM2;
	unsigned int stream = (dai->id == MT8696_AFE_IO_ETDM2_IN) ?
			       SNDRV_PCM_STREAM_CAPTURE :
			       SNDRV_PCM_STREAM_PLAYBACK;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		afe_priv->etdm_data[etdm_set].format[stream] =
			MT8696_ETDM_FORMAT_I2S;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		afe_priv->etdm_data[etdm_set].format[stream] =
			MT8696_ETDM_FORMAT_DSPA;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		afe_priv->etdm_data[etdm_set].format[stream] =
			MT8696_ETDM_FORMAT_DSPB;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		afe_priv->etdm_data[etdm_set].format[stream] =
			MT8696_ETDM_FORMAT_LJ;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		afe_priv->etdm_data[etdm_set].format[stream] =
			MT8696_ETDM_FORMAT_RJ;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		afe_priv->etdm_data[etdm_set].bck_inv[stream] = false;
		afe_priv->etdm_data[etdm_set].lrck_inv[stream] = false;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		afe_priv->etdm_data[etdm_set].bck_inv[stream] = false;
		afe_priv->etdm_data[etdm_set].lrck_inv[stream] = true;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		afe_priv->etdm_data[etdm_set].bck_inv[stream] = true;
		afe_priv->etdm_data[etdm_set].lrck_inv[stream] = false;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		afe_priv->etdm_data[etdm_set].bck_inv[stream] = true;
		afe_priv->etdm_data[etdm_set].lrck_inv[stream] = true;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		afe_priv->etdm_data[etdm_set].slave_mode[stream] = true;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		afe_priv->etdm_data[etdm_set].slave_mode[stream] = false;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mt8696_afe_etdm_set_tdm_slot(struct snd_soc_dai *dai,
					unsigned int tx_mask,
					unsigned int rx_mask,
					int slots,
					int slot_width)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mtk_base_afe_memif *memif = &afe->memif[dai->id];
	const struct mtk_base_memif_data *data = memif->data;
	unsigned int etdm_set = (dai->id == MT8696_AFE_IO_ETDM1_OUT) ?
				 MT8696_ETDM1 : MT8696_ETDM2;
	unsigned int stream = (dai->id == MT8696_AFE_IO_ETDM2_IN) ?
			       SNDRV_PCM_STREAM_CAPTURE :
			       SNDRV_PCM_STREAM_PLAYBACK;

	dev_dbg(dai->dev, "%s %s etdm %u stream %u slot_width %d\n",
		__func__, data->name, etdm_set, stream, slot_width);

	afe_priv->etdm_data[etdm_set].lrck_width[stream] = slot_width;
	return 0;
}

static int mt8696_afe_etdm_set_sysclk(struct snd_soc_dai *dai,
				      int clk_id,
				      unsigned int freq,
				      int dir)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	struct mtk_base_afe_memif *memif = &afe->memif[dai->id];
	const struct mtk_base_memif_data *data = memif->data;
	unsigned int etdm_set = (dai->id == MT8696_AFE_IO_ETDM1_OUT) ?
				 MT8696_ETDM1 : MT8696_ETDM2;
	unsigned int stream = (dai->id == MT8696_AFE_IO_ETDM2_IN) ?
			       SNDRV_PCM_STREAM_CAPTURE :
			       SNDRV_PCM_STREAM_PLAYBACK;

	dev_dbg(dai->dev, "%s %s etdm %u stream %u freq %u\n",
		__func__, data->name, etdm_set, stream, freq);

	afe_priv->etdm_data[etdm_set].mclk_freq[stream] = freq;
	return 0;
}

static int mt8696_afe_dai_suspend(struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dai->dev);

	if (afe->suspended)
		return 0;

	/* do afe suspend */
	mtk_afe_dai_suspend(dai);

	return 0;
}

static int mt8696_afe_dai_resume(struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dai->dev);
	struct device *dev = afe->dev;
	struct regmap *regmap = afe->regmap;
	int i = 0;

	if (pm_runtime_status_suspended(dev) || !afe->suspended)
		return 0;

	afe->runtime_resume(dev);

	if (dai->id == MT8696_AFE_MEMIF_DL8) {
		afe->cached_hdmi_audio_channels = 0;
		afe->cached_hdmi_audio_format = 0;
		afe->cached_hdmi_audio_sample_rate = 0;
		afe->cached_hdmi_audio_bit_depth = 0;
		dev_dbg(afe->dev, "%s afe->need_hdmi_toggle %d\n",
			__func__, afe->need_hdmi_toggle);
	}

	if (afe->reg_back_up) {
		for (i = 0; i < afe->reg_back_up_list_num; i++)
			regmap_write(regmap,
				afe->reg_back_up_list[i],
				afe->reg_back_up[i]);
	} else
		dev_dbg(dev, "%s no reg_backup\n", __func__);

	afe->suspended = false;
	return 0;
}

/* FE DAIs */
static const struct snd_soc_dai_ops mt8696_afe_fe_dai_ops = {
	.startup	= mt8696_afe_fe_startup,
	.shutdown	= mt8696_afe_fe_shutdown,
	.hw_params	= mt8696_afe_fe_hw_params,
	.hw_free	= mtk_afe_fe_hw_free,
	.prepare	= mt8696_afe_fe_prepare,
	.trigger	= mt8696_afe_fe_trigger,
	.set_fmt	= mt8696_afe_fe_set_fmt,
};

/* BE DAIs */
static const struct snd_soc_dai_ops mt8696_afe_etdm1_ops = {
	.startup	= mt8696_afe_etdm1_startup,
	.shutdown	= mt8696_afe_etdm1_shutdown,
	.prepare	= mt8696_afe_etdm1_prepare,
	.set_fmt	= mt8696_afe_etdm_set_fmt,
	.set_tdm_slot	= mt8696_afe_etdm_set_tdm_slot,
	.set_sysclk	= mt8696_afe_etdm_set_sysclk,
};

static const struct snd_soc_dai_ops mt8696_afe_etdm2_ops = {
	.startup	= mt8696_afe_etdm2_startup,
	.shutdown	= mt8696_afe_etdm2_shutdown,
	.prepare	= mt8696_afe_etdm2_prepare,
	.set_fmt	= mt8696_afe_etdm_set_fmt,
	.set_tdm_slot	= mt8696_afe_etdm_set_tdm_slot,
	.set_sysclk	= mt8696_afe_etdm_set_sysclk,
};

static struct snd_soc_dai_driver mt8696_afe_pcm_dais[] = {
	/* FE DAIs: memory intefaces to CPU */
	{
		.name = "DL8",
		.id = MT8696_AFE_MEMIF_DL8,
		.suspend = mt8696_afe_dai_suspend,
		.resume = mt8696_afe_dai_resume,
		.playback = {
			.stream_name = "DL8 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8696_afe_fe_dai_ops,
	}, {
		.name = "DL5",
		.id = MT8696_AFE_MEMIF_DL5,
		.suspend = mt8696_afe_dai_suspend,
		.resume = mt8696_afe_dai_resume,
		.playback = {
			.stream_name = "DL5 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8696_afe_fe_dai_ops,
	}, {
		.name = "UL1",
		.id = MT8696_AFE_MEMIF_UL1,
		.suspend = mt8696_afe_dai_suspend,
		.resume = mt8696_afe_dai_resume,
		.capture = {
			.stream_name = "UL1 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8696_afe_fe_dai_ops,
	}, {
	/* BE DAIs */
		.name = "ETDM1_OUT",
		.id = MT8696_AFE_IO_ETDM1_OUT,
		.playback = {
			.stream_name = "ETDM1 Playback",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8696_afe_etdm1_ops,
	}, {
	/* BE DAIs */
		.name = "ETDM2_IN",
		.id = MT8696_AFE_IO_ETDM2_IN,
		.capture = {
			.stream_name = "ETDM2 Capture",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8696_afe_etdm2_ops,
	}, {
	/* BE DAIs */
		.name = "ETDM2_OUT",
		.id = MT8696_AFE_IO_ETDM2_OUT,
		.playback = {
			.stream_name = "ETDM2 Playback",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8696_afe_etdm2_ops,
	},
};

static const struct snd_soc_dapm_widget mt8696_afe_pcm_widgets[] = {
};

static const struct snd_soc_dapm_route mt8696_afe_pcm_routes[] = {
	{"ETDM1 Playback", NULL, "DL8 Playback"},
	{"ETDM2 Playback", NULL, "DL5 Playback"},
	{"UL1 Capture", NULL, "ETDM2 Capture"},
};

static const struct snd_soc_component_driver mt8696_afe_pcm_dai_component = {
	.name = "mt8696-afe-pcm-dai",
	.dapm_widgets = mt8696_afe_pcm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt8696_afe_pcm_widgets),
	.dapm_routes = mt8696_afe_pcm_routes,
	.num_dapm_routes = ARRAY_SIZE(mt8696_afe_pcm_routes),
};

static const struct mtk_base_memif_data memif_data[MT8696_AFE_MEMIF_NUM] = {
	{
		.name = "DL8",
		.id = MT8696_AFE_MEMIF_DL8,
		.reg_ofs_base = AFE_DL8_BASE,
		.reg_ofs_cur = AFE_DL8_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON1,
		.fs_shift = 10,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = AFE_DL8_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 24,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 24,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 24,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 13,
		.ch_config_reg = AFE_DL8_CON0,
		.ch_config_shift = 0,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
	}, {
		.name = "DL5",
		.id = MT8696_AFE_MEMIF_DL5,
		.reg_ofs_base = AFE_DL5_BASE,
		.reg_ofs_cur = AFE_DL5_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON0,
		.fs_shift = 25,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = AFE_DL5_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 21,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 21,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 21,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 10,
		.ch_config_reg = AFE_DL5_CON0,
		.ch_config_shift = 0,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
	}, {
		.name = "UL1",
		.id = MT8696_AFE_MEMIF_UL1,
		.reg_ofs_base = AFE_UL1_BASE,
		.reg_ofs_cur = AFE_UL1_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 0,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL1_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL1_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 1,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 0,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 0,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 0,
		.ch_config_reg = -1,
		.ch_config_shift = 0,
		.int_odd_reg = AFE_UL1_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
	},
};

static const struct mtk_base_irq_data irq_data[MT8696_AFE_IRQ_NUM] = {
	{
		.id = MT8696_AFE_IRQ1,
		.irq_cnt_reg = ASYS_IRQ1_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ1_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ1_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 0,
		.irq_status_shift = 0,
	}, {
		.id = MT8696_AFE_IRQ2,
		.irq_cnt_reg = ASYS_IRQ2_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ2_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ2_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 1,
	}, {
		.id = MT8696_AFE_IRQ3,
		.irq_cnt_reg = ASYS_IRQ3_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ3_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ3_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x1f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 2,
		.irq_status_shift = 2,
	},
};

static const int memif_specified_irqs[MT8696_AFE_MEMIF_NUM] = {
	[MT8696_AFE_MEMIF_DL8] = MT8696_AFE_IRQ1,
	[MT8696_AFE_MEMIF_DL5] = MT8696_AFE_IRQ2,
	[MT8696_AFE_MEMIF_UL1] = MT8696_AFE_IRQ3,
};

static const int aux_irqs[] = {
	MT8696_AFE_IRQ2, //no aux irq now
};

#ifdef DEBUG_AFE_REGISTER_RW
int mt8696_reg_read(void *context, unsigned int reg, unsigned int *val)
{
	struct mtk_base_afe *afe = context;

	mt8696_afe_enable_reg_rw_clk(afe);

	dev_notice(afe->dev, "%s reg 0x%x >>\n", __func__, reg);

	*val = readl(afe->base_addr + reg);

	dev_notice(afe->dev, "%s reg 0x%x val 0x%x <<\n", __func__, reg, *val);

	mt8696_afe_disable_reg_rw_clk(afe);

	return 0;
}

int mt8696_reg_write(void *context, unsigned int reg, unsigned int val)
{
	struct mtk_base_afe *afe = context;

	mt8696_afe_enable_reg_rw_clk(afe);

	dev_notice(afe->dev, "%s reg 0x%x val 0x%x >>\n", __func__, reg, val);

	writel(val, afe->base_addr + reg);

	dev_notice(afe->dev, "%s reg 0x%x val 0x%x <<\n", __func__, reg, val);

	mt8696_afe_disable_reg_rw_clk(afe);

	return 0;
}

static const struct regmap_bus mt8696_afe_regmap_bus = {
	.fast_io = true,
	.reg_write = mt8696_reg_write,
	.reg_read = mt8696_reg_read,
	.val_format_endian_default = REGMAP_ENDIAN_LITTLE,
};
#endif

static const struct regmap_config mt8696_afe_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = MAX_REGISTER,
	.cache_type = REGCACHE_NONE,
};

static irqreturn_t mt8696_afe_irq_handler(int irq, void *dev_id)
{
	struct mtk_base_afe *afe = dev_id;
	unsigned int val;
	unsigned int asys_irq_clr_bits = 0;
	unsigned int afe_irq_clr_bits = 0;
	unsigned int irq_status_bits;
	unsigned int irq_clr_bits;
	int i, ret;

	ret = regmap_read(afe->regmap, AFE_IRQ_STATUS, &val);
	if (ret) {
		dev_info(afe->dev, "%s irq status err\n", __func__);
		afe_irq_clr_bits = AFE_IRQ_MCU_CLR_BITS;
		asys_irq_clr_bits = ASYS_IRQ_CLR_BITS;
		goto err_irq;
	}

	for (i = 0; i < MT8696_AFE_MEMIF_NUM; i++) {
		struct mtk_base_afe_memif *memif = &afe->memif[i];
		struct mtk_base_irq_data const *irq_data;

		if (memif->irq_usage < 0)
			continue;

		irq_data = afe->irqs[memif->irq_usage].irq_data;

		irq_status_bits = BIT(irq_data->irq_status_shift);
		irq_clr_bits = BIT(irq_data->irq_clr_shift);

		if (!(val & irq_status_bits))
			continue;

		if (irq_data->irq_clr_reg == ASYS_IRQ_CLR)
			asys_irq_clr_bits |= irq_clr_bits;
		else
			afe_irq_clr_bits |= irq_clr_bits;

		if (irq_data->custom_handler)
			irq_data->custom_handler(irq, dev_id);
		else
			snd_pcm_period_elapsed(memif->substream);
	}
err_irq:
	/* clear irq */
	if (asys_irq_clr_bits)
		regmap_write(afe->regmap, ASYS_IRQ_CLR, asys_irq_clr_bits);
	if (afe_irq_clr_bits)
		dev_err(afe->dev, "%s irq status err\n", __func__);

	return IRQ_HANDLED;
}

static int mt8696_afe_runtime_suspend(struct device *dev)
{
	return 0;
}

static int mt8696_afe_runtime_resume(struct device *dev)
{
	return 0;
}

static int mt8696_afe_dev_runtime_suspend(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);
	struct regmap *regmap = afe->regmap;
	int i;

	dev_dbg(afe->dev, "%s suspend %d %d >>\n", __func__,
		pm_runtime_status_suspended(dev), afe->suspended);

	if (pm_runtime_status_suspended(dev) || afe->suspended)
		return 0;

	mt8696_afe_enable_main_clk(afe);

	if (!afe->reg_back_up)
		afe->reg_back_up =
			devm_kcalloc(dev, afe->reg_back_up_list_num,
				     sizeof(unsigned int), GFP_KERNEL);

	for (i = 0; i < afe->reg_back_up_list_num; i++)
		regmap_read(regmap, afe->reg_back_up_list[i],
			    &afe->reg_back_up[i]);

	mt8696_afe_disable_main_clk(afe);

	afe->suspended = true;

	dev_dbg(afe->dev, "%s <<\n", __func__);

	return 0;
}

static int mt8696_afe_dev_runtime_resume(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);
	struct regmap *regmap = afe->regmap;
	int i = 0;

	dev_dbg(afe->dev, "%s suspend %d %d >>\n", __func__,
		pm_runtime_status_suspended(dev), afe->suspended);

	if (pm_runtime_status_suspended(dev) || !afe->suspended)
		return 0;

	if (!afe->reg_back_up) {
		dev_dbg(dev, "%s no reg_backup\n", __func__);
		return -EINVAL;
	}

	mt8696_afe_enable_main_clk(afe);

	for (i = 0; i < afe->reg_back_up_list_num; i++)
		regmap_write(regmap, afe->reg_back_up_list[i],
				 afe->reg_back_up[i]);

	mt8696_afe_disable_main_clk(afe);

	afe->suspended = false;

	dev_dbg(afe->dev, "%s <<\n", __func__);

	return 0;
}

static int mt8696_afe_init_registers(struct mtk_base_afe *afe)
{
	size_t i;
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	static struct {
		unsigned int reg;
		unsigned int mask;
		unsigned int val;
	} init_regs[] = {
		{ AFE_IRQ_MASK, AFE_IRQ_MASK_EN_MASK, AFE_IRQ_MASK_EN_BITS },
		{ AFE_CONN_24BIT, GENMASK(31, 0), 0x0 },
		{ AFE_CONN_16BIT, GENMASK(31, 0), 0x0 },
		{ AFE_SINEGEN_CON0, AFE_SINEGEN_CON0_INIT_MASK,
		  AFE_SINEGEN_CON0_INIT_VAL },
	};

	mt8696_afe_enable_main_clk(afe);

	for (i = 0; i < ARRAY_SIZE(init_regs); i++)
		regmap_update_bits(afe->regmap, init_regs[i].reg,
				   init_regs[i].mask, init_regs[i].val);

	if (afe_priv->use_bypass_afe_pinmux)
		regmap_update_bits(afe->regmap, ETDM_IN2_CON0,
				   ETDM_CON0_SLAVE_MODE,
				   ETDM_CON0_SLAVE_MODE);

	mt8696_afe_disable_main_clk(afe);

	return 0;
}

static bool mt8696_afe_validate_sram(struct mtk_base_afe *afe,
	u32 phy_addr, u32 size)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;

	if (afe_priv->afe_sram_phy_addr &&
	    (phy_addr >= afe_priv->afe_sram_phy_addr) &&
	    ((phy_addr + size) <=
	     (afe_priv->afe_sram_phy_addr + afe_priv->afe_sram_size))) {
		return true;
	}

	return false;
}

static void __iomem *mt8696_afe_sram_pa_to_va(struct mtk_base_afe *afe,
	u32 phy_addr)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	u32 off = phy_addr - afe_priv->afe_sram_phy_addr;

	if (afe_priv->afe_sram_phy_addr &&
	    (off >= 0) && (off < afe_priv->afe_sram_size)) {
		return ((unsigned char *)afe_priv->afe_sram_vir_addr + off);
	}

	return NULL;
}

static void mt8696_afe_parse_of(struct mtk_base_afe *afe,
				struct device_node *np)
{
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	size_t i;
	int ret;
	unsigned int stream;
	unsigned int temps[4];
	char prop[128];
	unsigned int val[2];
	struct {
		char *name;
		unsigned int val;
	} of_fe_table[] = {
		{ "dl8",	MT8696_AFE_MEMIF_DL8 },
		{ "dl5",	MT8696_AFE_MEMIF_DL5 },
		{ "ul1",	MT8696_AFE_MEMIF_UL1 },
	};

	ret = of_property_read_u32_array(np, "mediatek,hdmi-clk-force-on",
					 &temps[0],
					 1);
	if (ret == 0 && temps[0]) {
		dev_info(afe->dev, "hdmi clock force on enable\n");

		mt8696_afe_enable_main_clk(afe);

		/* force audio clock on*/
		afe->cached_sample_rate = 48000;
		afe->cached_channels = 2;
		afe->force_clk_on = true;
		afe->hdmi_force_clk_switch = 1;
		afe->hdmi_audio_format = 0;
		afe->cached_hdmi_audio_format = 0;
		afe->cached_hdmi_audio_sample_rate = 0;
		afe->cached_hdmi_audio_channels = 0;
		afe->cached_hdmi_audio_bit_depth = 0;
		afe->need_hdmi_toggle = true;
	} else {
		dev_info(afe->dev, "hdmi clock force on disable\n");

		/* force audio clock off*/
		afe->cached_sample_rate = 0;
		afe->cached_channels = 0;
		afe->force_clk_on = false;
		afe->hdmi_force_clk_switch = 0;
		afe->hdmi_audio_format = 0;
		afe->cached_hdmi_audio_format = 0;
		afe->cached_hdmi_audio_sample_rate = 0;
		afe->cached_hdmi_audio_channels = 0;
		afe->cached_hdmi_audio_bit_depth = 0;
		afe->need_hdmi_toggle = true;
	}

	ret = of_property_read_u32_array(np, "mediatek,etdm-clock-modes",
					 &temps[0],
					 MT8696_ETDM_SETS);
	if (ret == 0) {
		for (i = 0; i < MT8696_ETDM_SETS; i++)
			afe_priv->etdm_data[i].clock_mode = temps[i];
	}

	ret = of_property_read_u32_array(np, "mediatek,etdm-out-data-modes",
					 &temps[0],
					 MT8696_ETDM_SETS);
	if (ret == 0) {
		stream = SNDRV_PCM_STREAM_PLAYBACK;
		for (i = 0; i < MT8696_ETDM_SETS; i++)
			afe_priv->etdm_data[i].data_mode[stream] = temps[i];
	}

	ret = of_property_read_u32_array(np, "mediatek,etdm-in-data-modes",
					 &temps[0],
					 MT8696_ETDM_SETS);
	if (ret == 0) {
		stream = SNDRV_PCM_STREAM_CAPTURE;
		for (i = 0; i < MT8696_ETDM_SETS; i++)
			afe_priv->etdm_data[i].data_mode[stream] = temps[i];
	}

	for (i = 0; i < ARRAY_SIZE(of_fe_table); i++) {
		bool valid_sram;
		struct mt8696_fe_dai_data *fe_data;

		memset(val, 0, sizeof(val));

		snprintf(prop, sizeof(prop), "mediatek,%s-use-sram",
			 of_fe_table[i].name);
		ret = of_property_read_u32_array(np, prop, &val[0], 2);
		if (ret)
			continue;

		valid_sram = mt8696_afe_validate_sram(afe, val[0], val[1]);
		if (!valid_sram) {
			dev_info(afe->dev, "%s %s validate 0x%x 0x%x fail\n",
				 __func__, of_fe_table[i].name,
				 val[0], val[1]);
			continue;
		}

		fe_data = &afe_priv->fe_data[of_fe_table[i].val];

		fe_data->sram_phy_addr = val[0];
		fe_data->sram_size = val[1];
		fe_data->sram_vir_addr = mt8696_afe_sram_pa_to_va(afe,
			fe_data->sram_phy_addr);
	}
}

static int mt8696_afe_pcm_probe(struct snd_soc_platform *platform)
{
	return mt8696_afe_add_controls(platform);
}

const struct snd_soc_platform_driver mt8696_afe_pcm_platform = {
	.probe = mt8696_afe_pcm_probe,
	.pcm_new = mtk_afe_pcm_new,
	.pcm_free = mtk_afe_pcm_free,
	.ops = &mtk_afe_pcm_ops,
};

static int mt8696_afe_pcm_dev_probe(struct platform_device *pdev)
{
	int ret, i, sel_irq;
	unsigned int irq_id;
	struct mtk_base_afe *afe;
	struct mt8696_afe_private *afe_priv;
	struct resource *res;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(33));
	if (ret)
		return ret;

	afe = devm_kzalloc(&pdev->dev, sizeof(*afe), GFP_KERNEL);
	if (!afe)
		return -ENOMEM;

	afe->platform_priv = devm_kzalloc(&pdev->dev, sizeof(*afe_priv),
					  GFP_KERNEL);
	afe_priv = afe->platform_priv;
	if (!afe_priv)
		return -ENOMEM;

	spin_lock_init(&afe_priv->afe_ctrl_lock);

	mutex_init(&afe_priv->afe_clk_mutex);

	mutex_init(&afe_priv->block_dpidle_mutex);

	afe->dev = &pdev->dev;

	irq_id = platform_get_irq(pdev, 0);
	if (!irq_id) {
		dev_info(afe->dev, "np %s no irq\n", afe->dev->of_node->name);
		return -ENXIO;
	}
	ret = devm_request_irq(afe->dev, irq_id, mt8696_afe_irq_handler,
			       0, "Afe_ISR_Handle", (void *)afe);
	if (ret) {
		dev_info(afe->dev, "could not request_irq\n");
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	afe->base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(afe->base_addr))
		return PTR_ERR(afe->base_addr);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res) {
		afe_priv->afe_sram_vir_addr =
			devm_ioremap_resource(&pdev->dev, res);
		if (!IS_ERR(afe_priv->afe_sram_vir_addr)) {
			afe_priv->afe_sram_phy_addr = res->start;
			afe_priv->afe_sram_size = resource_size(res);
		}
	}

	/* initial audio related clock */
	ret = mt8696_afe_init_audio_clk(afe);
	if (ret) {
		dev_info(afe->dev, "mt8696_afe_init_audio_clk fail\n");
		return ret;
	}

#ifdef DEBUG_AFE_REGISTER_RW
	afe->regmap = devm_regmap_init(&pdev->dev,
		&mt8696_afe_regmap_bus, afe,
		&mt8696_afe_regmap_config);
#else
	/* afe->regmap = devm_regmap_init_mmio_clk(&pdev->dev, */
	/*	"top_aud_intbus", afe->base_addr, */
	/*	&mt8696_afe_regmap_config); */
	afe->regmap = devm_regmap_init_mmio(&pdev->dev, afe->base_addr,
		&mt8696_afe_regmap_config);
#endif
	if (IS_ERR(afe->regmap))
		return PTR_ERR(afe->regmap);

	/* memif % irq initialize*/
	afe->memif_size = MT8696_AFE_MEMIF_NUM;
	afe->memif = devm_kcalloc(afe->dev, afe->memif_size,
				  sizeof(*afe->memif), GFP_KERNEL);
	if (!afe->memif)
		return -ENOMEM;

	afe->irqs_size = MT8696_AFE_IRQ_NUM;
	afe->irqs = devm_kcalloc(afe->dev, afe->irqs_size,
				 sizeof(*afe->irqs), GFP_KERNEL);
	if (!afe->irqs)
		return -ENOMEM;

	for (i = 0; i < afe->irqs_size; i++)
		afe->irqs[i].irq_data = &irq_data[i];

	for (i = 0; i < afe->memif_size; i++) {
		afe->memif[i].data = &memif_data[i];
		sel_irq = memif_specified_irqs[i];
		if (sel_irq >= 0) {
			afe->memif[i].irq_usage = sel_irq;
			afe->memif[i].const_irq = 1;
			afe->irqs[sel_irq].irq_occupyed = true;
		} else {
			afe->memif[i].irq_usage = -1;
		}
	}

	for (i = 0; i < MT8696_AFE_MEMIF_NUM; i++) {
		afe->memif[i].data = &memif_data[i];
		spin_lock_init(&afe->memif[i].buf_info_lock);
	}

	afe->mtk_afe_hardware = &mt8696_afe_hardware;
	afe->memif_fs = mt8696_memif_fs;
	afe->irq_fs = mt8696_irq_fs;
	afe->alloc_dmabuf = mt8696_alloc_dmabuf;
	afe->free_dmabuf = mt8696_free_dmabuf;

	platform_set_drvdata(pdev, afe);

	pm_runtime_enable(&pdev->dev);
	if (!pm_runtime_enabled(&pdev->dev)) {
		dev_info(afe->dev, "%s pm_runtime not enabled\n", __func__);
		ret = mt8696_afe_runtime_resume(&pdev->dev);
		if (ret)
			goto err_pm_disable;
	}

	pm_runtime_get_sync(&pdev->dev);

	afe->reg_back_up_list = mt8696_afe_backup_list;
	afe->reg_back_up_list_num = ARRAY_SIZE(mt8696_afe_backup_list);
	afe->runtime_resume = mt8696_afe_runtime_resume;
	afe->runtime_suspend = mt8696_afe_runtime_suspend;

	mt8696_afe_parse_of(afe, pdev->dev.of_node);

	ret = snd_soc_register_platform(&pdev->dev, &mt8696_afe_pcm_platform);
	if (ret)
		goto err_platform;

	ret = snd_soc_register_component(&pdev->dev,
					 &mt8696_afe_pcm_dai_component,
					 mt8696_afe_pcm_dais,
					 ARRAY_SIZE(mt8696_afe_pcm_dais));
	if (ret)
		goto err_component;

	mt8696_afe_init_registers(afe);

	mt8696_afe_init_debugfs(afe);

	dev_info(&pdev->dev, "MT8696 AFE driver initialized.\n");
	return 0;

err_component:
	snd_soc_unregister_platform(&pdev->dev);
err_platform:
	pm_runtime_put_sync(&pdev->dev);
err_pm_disable:
	pm_runtime_disable(&pdev->dev);
	return ret;
}

static int mt8696_afe_pcm_dev_remove(struct platform_device *pdev)
{
	struct mtk_base_afe *afe = platform_get_drvdata(pdev);

	mt8696_afe_cleanup_debugfs(afe);

	pm_runtime_disable(&pdev->dev);
	if (!pm_runtime_status_suspended(&pdev->dev))
		mt8696_afe_runtime_suspend(&pdev->dev);

	pm_runtime_put_sync(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct of_device_id mt8696_afe_pcm_dt_match[] = {
	{ .compatible = "mediatek,mt8696-afe-pcm", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt8696_afe_pcm_dt_match);

static const struct dev_pm_ops mt8696_afe_pm_ops = {
	SET_RUNTIME_PM_OPS(mt8696_afe_dev_runtime_suspend,
			   mt8696_afe_dev_runtime_resume, NULL)
};

static struct platform_driver mt8696_afe_pcm_driver = {
	.driver = {
		   .name = "mt8696-afe-pcm",
		   .of_match_table = mt8696_afe_pcm_dt_match,
		   .pm = &mt8696_afe_pm_ops,
	},
	.probe = mt8696_afe_pcm_dev_probe,
	.remove = mt8696_afe_pcm_dev_remove,
};

module_platform_driver(mt8696_afe_pcm_driver);

MODULE_DESCRIPTION("Mediatek ALSA SoC AFE platform driver");
MODULE_AUTHOR("Sail Yang <sail.yang@mediatek.com>");
MODULE_LICENSE("GPL v2");
