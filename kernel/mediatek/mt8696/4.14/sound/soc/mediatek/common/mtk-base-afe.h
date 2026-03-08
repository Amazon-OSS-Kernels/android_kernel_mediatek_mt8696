/*
 * mtk-base-afe.h  --  Mediatek base afe structure
 *
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Garlic Tseng <garlic.tseng@mediatek.com>
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

#ifndef _MTK_BASE_AFE_H_
#define _MTK_BASE_AFE_H_

#include <linux/hrtimer.h>

struct mtk_base_memif_data {
	int id;
	const char *name;
	int reg_ofs_base;
	int reg_ofs_cur;
	int fs_reg;
	int fs_shift;
	int fs_maskbit;
	int mono_reg;
	int mono_shift;
	int enable_reg;
	int enable_shift;
	int hd_reg;
	int hd_shift;
	int msb_reg;
	int msb_shift;
	int msb2_reg;
	int msb2_shift;
	int agent_disable_reg;
	int agent_disable_shift;
	int ch_config_reg;
	int ch_config_shift;
	int hd_align_reg;
	int hd_align_shfit;
	int int_odd_reg;
	int int_odd_shift;
	int buffer_bytes_align;
	int buffer_end_shift;
	int engen_domain_reg;
	int engen_domain_shift;
	int engen_domain_maskbit;
};

struct mtk_base_irq_data {
	int id;
	int irq_cnt_reg;
	int irq_cnt_shift;
	int irq_cnt_maskbit;
	int irq_fs_reg;
	int irq_fs_shift;
	int irq_fs_maskbit;
	int irq_en_reg;
	int irq_en_shift;
	int irq_clr_reg;
	int irq_clr_shift;
	int irq_status_shift;
	int engen_domain_reg;
	int engen_domain_shift;
	int engen_domain_maskbit;
	int (*custom_handler)(int irq, void *dev_id);
};

struct device;
struct list_head;
struct mtk_base_afe_memif;
struct mtk_base_afe_irq;
struct mtk_base_afe_dai;
struct regmap;
struct snd_pcm_substream;
struct snd_soc_dai;

#if defined(CONFIG_SND_SOC_MT8696)
struct mtk_iec_ch_status {
	unsigned int chl_stat0;
	unsigned int chl_stat1;
	unsigned int chr_stat0;
	unsigned int chr_stat1;
};

struct mtk_iec_buf_ctrl {
	unsigned int buf_sa;
	unsigned int buf_ea;
	unsigned int buf_nsadr;
};

struct mtk_iec_config {
	unsigned int bit_width;
	unsigned int nsnum;
	unsigned int period_bytes;
	unsigned int force_update_size;
	struct mtk_iec_buf_ctrl buf;
	struct mtk_iec_ch_status ch_status;
};
#endif

struct mtk_base_afe {
	void __iomem *base_addr;
	struct device *dev;
	struct regmap *regmap;
	struct mutex irq_alloc_lock; /* dynamic alloc irq lock */

	unsigned int const *reg_back_up_list;
	unsigned int *reg_back_up;
	unsigned int reg_back_up_list_num;

	int (*runtime_suspend)(struct device *dev);
	int (*runtime_resume)(struct device *dev);
	bool suspended;

	struct mtk_base_afe_memif *memif;
	int memif_size;
	struct mtk_base_afe_irq *irqs;
	int irqs_size;

	struct list_head sub_dais;
	struct snd_soc_dai_driver *dai_drivers;
	unsigned int num_dai_drivers;

	const struct snd_pcm_hardware *mtk_afe_hardware;
	int (*memif_fs)(struct snd_pcm_substream *substream,
			unsigned int rate);

	int (*memif_engen_apll_sel)(struct snd_pcm_substream *substream);

	int (*irq_fs)(struct snd_pcm_substream *substream,
		      unsigned int rate);

	int (*irq_engen_apll_sel)(struct snd_pcm_substream *substream);

	int (*alloc_dmabuf)(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai);
	int (*free_dmabuf)(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai);

	void *platform_priv;
#if defined(CONFIG_SND_SOC_MT8532)
	uint32_t mini_clk_enable;
#endif
#if defined(CONFIG_SND_SOC_MT8696)
	bool force_clk_on;
	uint32_t hdmi_audio_format;
	uint32_t cached_hdmi_audio_format;
	uint32_t cached_hdmi_audio_sample_rate;
	uint32_t cached_hdmi_audio_channels;
	uint32_t cached_hdmi_audio_bit_depth;
	bool need_hdmi_toggle;
	struct mtk_iec_config iec;
	uint32_t hdmi_force_clk_switch;
	uint32_t cached_sample_rate;
	uint32_t cached_channels;
	int hdmi_dac_state;
	int64_t hdmi_dac_timestamp;
#endif
};

struct mtk_base_afe_memif {
	unsigned int phys_buf_addr;
	int buffer_size;
	struct snd_pcm_substream *substream;
	const struct mtk_base_memif_data *data;
	int irq_usage;
	int const_irq;
	const struct mtk_base_memif_data *aux_data;
	unsigned int aux_channels;
	unsigned int aux_phys_addr;
	int aux_buffer_size;
	bool use_sw_irq;
	struct hrtimer dma_hrt;
	unsigned int byte_rate;
	u64 period_time_ns;
	unsigned int period_bytes;
	unsigned int prev_hw_cur;
	void *afe;
#if defined(CONFIG_SND_SOC_MT8696)
	bool prepared;
	bool avsync_mode;
	bool buf_reset; /* for avsync*/
	unsigned long long buf_read;	/* for avsync unit is bytes */
	unsigned long long buf_write;	/* for avsync unit is bytes */
	unsigned int buf_frame_size;
	spinlock_t buf_info_lock;
#endif
};

struct mtk_base_afe_irq {
	const struct mtk_base_irq_data *irq_data;
	int irq_occupyed;
};

struct mtk_base_afe_dai {
	struct snd_soc_dai_driver *dai_drivers;
	unsigned int num_dai_drivers;

	const struct snd_kcontrol_new *controls;
	unsigned int num_controls;
	const struct snd_soc_dapm_widget *dapm_widgets;
	unsigned int num_dapm_widgets;
	const struct snd_soc_dapm_route *dapm_routes;
	unsigned int num_dapm_routes;

	struct list_head list;
};

#endif

