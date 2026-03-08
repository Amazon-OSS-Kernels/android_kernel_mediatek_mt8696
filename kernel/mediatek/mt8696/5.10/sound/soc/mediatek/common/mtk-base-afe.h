/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk-base-afe.h  --  Mediatek base afe structure
 *
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Garlic Tseng <garlic.tseng@mediatek.com>
 */

#ifndef _MTK_BASE_AFE_H_
#define _MTK_BASE_AFE_H_

#if defined(CONFIG_SND_SOC_MT8696)
#include <linux/hrtimer.h>
#endif

#define MTK_STREAM_NUM (SNDRV_PCM_STREAM_LAST + 1)

enum {
	MTK_AFE_RATE_8K,
	MTK_AFE_RATE_11K,
	MTK_AFE_RATE_12K,
	MTK_AFE_RATE_384K,
	MTK_AFE_RATE_16K,
	MTK_AFE_RATE_22K,
	MTK_AFE_RATE_24K,
	MTK_AFE_RATE_352K,
	MTK_AFE_RATE_32K,
	MTK_AFE_RATE_44K,
	MTK_AFE_RATE_48K,
	MTK_AFE_RATE_88K,
	MTK_AFE_RATE_96K,
	MTK_AFE_RATE_176K,
	MTK_AFE_RATE_192K,
	MTK_AFE_RATE_260K,
};

enum {
	MTK_AFE_DAI_MEMIF_RATE_8K,
	MTK_AFE_DAI_MEMIF_RATE_16K,
	MTK_AFE_DAI_MEMIF_RATE_32K,
	MTK_AFE_DAI_MEMIF_RATE_48K,
};

enum {
	MTK_AFE_PCM_RATE_8K,
	MTK_AFE_PCM_RATE_16K,
	MTK_AFE_PCM_RATE_32K,
	MTK_AFE_PCM_RATE_48K,
};

enum {
	MTKAIF_PROTOCOL_1 = 0,
	MTKAIF_PROTOCOL_2,
	MTKAIF_PROTOCOL_2_CLK_P2,
};

enum {
	MTK_AFE_ADDA_DL_GAIN_MUTE = 0,
	MTK_AFE_ADDA_DL_GAIN_NORMAL = 0xf74f,
	/* SA suggest apply -0.3db to audio/speech path */
};

/* SMC CALL Operations */
enum mtk_audio_smc_call_op {
	MTK_AUDIO_SMC_OP_INIT = 0,
	MTK_AUDIO_SMC_OP_DRAM_REQUEST,
	MTK_AUDIO_SMC_OP_DRAM_RELEASE,
	MTK_AUDIO_SMC_OP_FM_REQUEST,
	MTK_AUDIO_SMC_OP_FM_RELEASE,
	MTK_AUDIO_SMC_OP_ADSP_REQUEST,
	MTK_AUDIO_SMC_OP_ADSP_RELEASE,
	MTK_AUDIO_SMC_OP_NUM
};

struct mtk_base_memif_data {
	int id;
	const char *name;
	int reg_ofs_base;
	int reg_ofs_cur;
	int reg_ofs_end;
	int reg_ofs_base_msb;
	int reg_ofs_cur_msb;
	int reg_ofs_end_msb;
	int fs_reg;
	int fs_shift;
	int fs_maskbit;
	int mono_reg;
	int mono_shift;
	int mono_invert;
	int quad_ch_reg;
	int quad_ch_mask;
	int quad_ch_shift;
	int int_odd_flag_reg;
	int int_odd_flag_shift;
	int enable_reg;
	int enable_shift;
	int hd_reg;
	int hd_shift;
	int hd_align_reg;
	int hd_align_mshift;
	int msb_reg;
	int msb_shift;
	int msb2_reg;
	int msb2_shift;
	int agent_disable_reg;
	int agent_disable_shift;
	int ch_num_reg;
	int ch_num_shift;
	int ch_num_maskbit;
	/* playback memif only */
	int pbuf_reg;
	int pbuf_mask;
	int pbuf_shift;
	int minlen_reg;
	int minlen_mask;
	int minlen_shift;
#if defined(CONFIG_SND_SOC_MT8696)
	int buffer_bytes_align;
	int ch_config_reg;
	int ch_config_shift;
	int buffer_end_shift;
	int int_odd_shift;
	int int_odd_reg;
#endif
#if defined(CONFIG_SND_SOC_MT8532)
	int engen_domain_reg;
	int engen_domain_shift;
	int engen_domain_maskbit;
#endif
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
	int irq_ap_en_reg;
	int irq_ap_en_shift;
	int irq_scp_en_reg;
	int irq_scp_en_shift;
	int irq_status_shift;
#if defined(CONFIG_SND_SOC_MT8696)
	int (*custom_handler)(int irq, void *dev_id);
#endif
#if defined(CONFIG_SND_SOC_MT8532)
	int engen_domain_reg;
	int engen_domain_shift;
	int engen_domain_maskbit;
#endif
};

struct dentry;
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

typedef int (*mtk_sp_copy_f)(struct snd_pcm_substream *substream,
				 int channel, unsigned long hwoff,
				 void *buf, unsigned long bytes);
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
	int memif_32bit_supported;
	struct mtk_base_afe_irq *irqs;
	int irqs_size;

	/* Bit banding of memif use AFE_AGEN_ON_SET/CLR
	 * to control memif enable bit.
	 */
	int is_memif_bit_banding;

	struct list_head sub_dais;
	struct snd_soc_dai_driver *dai_drivers;
	unsigned int num_dai_drivers;

	const struct snd_pcm_hardware *mtk_afe_hardware;
	int (*memif_fs)(struct snd_pcm_substream *substream,
			unsigned int rate);
	int (*irq_fs)(struct snd_pcm_substream *substream,
		      unsigned int rate);
	int (*get_dai_fs)(struct mtk_base_afe *afe,
			  int dai_id, unsigned int rate);
	int (*get_memif_pbuf_size)(struct snd_pcm_substream *substream);

	void *sram;
	int (*request_dram_resource)(struct device *dev);
	int (*release_dram_resource)(struct device *dev);

	struct dentry *debugfs;
	const struct mtk_afe_debug_cmd *debug_cmds;

	void *platform_priv;

	int (*copy)(struct snd_pcm_substream *substream,
		    int channel, unsigned long hwoff,
		    void *buf, unsigned long bytes,
		    mtk_sp_copy_f sp_copy);
#if defined(CONFIG_SND_SOC_MT8696)
	bool etdm1_mclk_is_on;
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
	bool suspend_toggle;
	int (*alloc_dmabuf)(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai);
	int (*irq_engen_apll_sel)(struct snd_pcm_substream *substream);
	int (*free_dmabuf)(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai);
	int hdmi_dac_state;
	int64_t hdmi_dac_timestamp;
#if defined(CONFIG_SND_SOC_MT8532)
	int (*memif_engen_apll_sel)(struct snd_pcm_substream *substream);
	uint32_t mini_clk_enable;
#endif
#endif
};

struct mtk_base_afe_memif {
	unsigned int phys_buf_addr;
	int buffer_size;
	struct snd_pcm_substream *substream;
	const struct mtk_base_memif_data *data;
	int irq_usage;
	int const_irq;

	int using_sram;
	int use_dram_only;
	unsigned char *dma_area;
	dma_addr_t dma_addr;
	size_t dma_bytes;
	int use_adsp_share_mem;
	bool ack_enable;
	int (*ack)(struct snd_pcm_substream *substream);
	int use_mmap_share_mem;  // 1: dl, 2: ul
	bool vow_barge_in_enable;
#if IS_ENABLED(CONFIG_MTK_ULTRASND_PROXIMITY)
	bool scp_ultra_enable;
#endif
#if defined(CONFIG_SND_SOC_MT8696)
	bool prepared;
	bool avsync_mode;
	bool buf_reset; /* for avsync*/
	unsigned long long buf_read;	/* for avsync unit is bytes */
	unsigned long long buf_write;	/* for avsync unit is bytes */
	unsigned int buf_frame_size;
	spinlock_t buf_info_lock;
	struct hrtimer dma_hrt;
	u64 period_time_ns;
	bool use_sw_irq;
	unsigned int prev_hw_cur;
#endif
#if defined(CONFIG_SND_SOC_MT8532)
	unsigned int aux_channels;
	void *afe;
	unsigned int period_bytes;
	unsigned int byte_rate;
	const struct mtk_base_memif_data *aux_data;
	int aux_buffer_size;
	unsigned int aux_phys_addr;
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

