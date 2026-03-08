/*
 * mt8696_afe_common.h  --  Mediatek 8696 audio driver common definitions
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

#ifndef _MT8696_AFE_COMMON_H_
#define _MT8696_AFE_COMMON_H_

#define COMMON_CLOCK_FRAMEWORK_API
//#define DEBUG_AFE_REGISTER_RW
#define FORCE_CLK_SWITCH_FEATURE_ON

#include <linux/clk.h>
#include <linux/regmap.h>
#include <sound/asound.h>

enum {
	MT8696_AFE_MEMIF_DL8,
	MT8696_AFE_MEMIF_DL5,
	MT8696_AFE_MEMIF_UL1,
	MT8696_AFE_MEMIF_NUM,
	MT8696_AFE_BACKEND_BASE = MT8696_AFE_MEMIF_NUM,
	MT8696_AFE_IO_ETDM1_OUT = MT8696_AFE_BACKEND_BASE,
	MT8696_AFE_IO_ETDM2_IN,
	MT8696_AFE_IO_ETDM2_OUT,
	MT8696_AFE_BACKEND_END,
	MT8696_AFE_BACKEND_NUM = (MT8696_AFE_BACKEND_END -
				  MT8696_AFE_BACKEND_BASE),
};

enum {
	MT8696_AFE_IRQ1, /* HDMI OUT */
	MT8696_AFE_IRQ2, /* BTSCO OUT */
	MT8696_AFE_IRQ3, /* BTSCO IN */
	MT8696_AFE_IRQ_NUM,
};

enum {
	MT8696_TOP_CG_AFE,
	MT8696_TOP_CG_TML,
	MT8696_TOP_CG_ETDM_IN,
	MT8696_TOP_CG_ETDM_OUT1,
	MT8696_TOP_CG_ETDM_OUT2,
	MT8696_TOP_CG_A1SYS,
	MT8696_TOP_CG_A2SYS,
	MT8696_TOP_CG_A1SYS_TIMING,
	MT8696_TOP_CG_A2SYS_TIMING,
	MT8696_TOP_CG_NUM
};

enum {
	MT8696_CLK_TOP_AUD_26M,
	MT8696_CLK_TOP_AUD_BUS,
	MT8696_CLK_INFRA_AUD,
	MT8696_CLK_FA1SYS,
	MT8696_CLK_FA2SYS,
	MT8696_CLK_FAPLL1,
	MT8696_CLK_FAPLL2,
	MT8696_CLK_APLL_SEL,
	MT8696_CLK_APLL2_SEL,
	MT8696_CLK_APLL12_DIV0, //eTDM in2
	MT8696_CLK_APLL12_DIV2, //eTDM out2
	MT8696_CLK_APLL12_DIV3, //eTDM out1
	MT8696_CLK_ETDM_OUT1_M_SEL,
	MT8696_CLK_ETDM_OUT2_M_SEL,
	MT8696_CLK_ETDM_IN2_M_SEL,
	MT8696_CLK_NUM
};

enum {
	MT8696_AFE_APLL1 = 0,
	MT8696_AFE_APLL2,
	MT8696_AFE_APLL_NUM,
};

enum {
	MT8696_ETDM1 = 0,
	MT8696_ETDM2,
	MT8696_ETDM_SETS,
};

enum {
	MT8696_ETDM_DATA_ONE_PIN = 0,
	MT8696_ETDM_DATA_MULTI_PIN,
};

enum {
	MT8696_ETDM_SEPARATE_CLOCK = 0,
	MT8696_ETDM_SHARED_CLOCK,
};

enum {
	MT8696_ETDM_FORMAT_I2S = 0,
	MT8696_ETDM_FORMAT_LJ,
	MT8696_ETDM_FORMAT_RJ,
	MT8696_ETDM_FORMAT_EIAJ,
	MT8696_ETDM_FORMAT_DSPA,
	MT8696_ETDM_FORMAT_DSPB,
};

enum {
	MT8696_PCM_FORMAT_I2S = 0,
	MT8696_PCM_FORMAT_EIAJ,
	MT8696_PCM_FORMAT_PCMA,
	MT8696_PCM_FORMAT_PCMB,
};

enum {
	MT8696_MULTI_IN_FORMAT_I2S = 0,
	MT8696_MULTI_IN_FORMAT_LJ,
	MT8696_MULTI_IN_FORMAT_RJ,
};

enum {
	MT8696_FS_8K = 0,
	MT8696_FS_12K,
	MT8696_FS_16K,
	MT8696_FS_24K,
	MT8696_FS_32K,
	MT8696_FS_48K,
	MT8696_FS_96K,
	MT8696_FS_192K,
	MT8696_FS_384K,
	MT8696_FS_ETDMOUT1_1X_EN,
	MT8696_FS_ETDMOUT2_1X_EN,
	MT8696_FS_ETDMIN1_1X_EN = 12,
	MT8696_FS_ETDMIN2_1X_EN,
	MT8696_FS_EXT_PCM_1X_EN = 15,
	MT8696_FS_7P35K,
	MT8696_FS_11P025K,
	MT8696_FS_14P7K,
	MT8696_FS_22P05K,
	MT8696_FS_29P4K,
	MT8696_FS_44P1K,
	MT8696_FS_88P2K,
	MT8696_FS_176P4K,
	MT8696_FS_352P8K,
	MT8696_FS_ETDMIN1_NX_EN,
	MT8696_FS_ETDMIN2_NX_EN,
	MT8696_FS_AMIC_1X_EN_ASYNC = 28,
	MT8696_FS_DL_1X_EN = 30,
};

enum {
	MT8696_ETDM_FS_8K = 0,
	MT8696_ETDM_FS_12K,
	MT8696_ETDM_FS_16K,
	MT8696_ETDM_FS_24K,
	MT8696_ETDM_FS_32K,
	MT8696_ETDM_FS_48K,
	MT8696_ETDM_FS_64K,
	MT8696_ETDM_FS_96K,
	MT8696_ETDM_FS_128K,
	MT8696_ETDM_FS_192K,
	MT8696_ETDM_FS_256K,
	MT8696_ETDM_FS_384K = 11,
	MT8696_ETDM_FS_11P025K = 16,
	MT8696_ETDM_FS_22P05K,
	MT8696_ETDM_FS_44P1K,
	MT8696_ETDM_FS_88P2K,
	MT8696_ETDM_FS_176P4K,
	MT8696_ETDM_FS_352P8K = 21,
};

enum {
	SPDIF_IN_PORT_NONE = 0,
	SPDIF_IN_PORT_OPT,
	SPDIF_IN_PORT_COAXIAL,
	SPDIF_IN_PORT_ARC,
	SPDIF_IN_PORT_NUM
};

enum {
	SPDIF_IN_MUX_0 = 0,
	SPDIF_IN_MUX_1,
	SPDIF_IN_MUX_2,
};

enum {
	MT8696_AFE_DEBUGFS_ALL,
	MT8696_AFE_DEBUGFS_ETDM,
	MT8696_AFE_DEBUGFS_MEMIF,
	MT8696_AFE_DEBUGFS_IRQ,
	MT8696_AFE_DEBUGFS_DBG,
	MT8696_AFE_DEBUGFS_NUM,
};

enum {
	MT8696_AFE_IRQ_DIR_MCU = 0,
	MT8696_AFE_IRQ_DIR_DSP,
	MT8696_AFE_IRQ_DIR_BOTH,
};

struct mt8696_fe_dai_data {
	bool slave_mode;
	bool use_sram;
	unsigned int sram_phy_addr;
	void __iomem *sram_vir_addr;
	unsigned int sram_size;
};

struct mt8696_be_dai_data {
	bool prepared[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int fmt_mode;
};

struct mt8696_etdm_data {
	int occupied[SNDRV_PCM_STREAM_LAST + 1];
	int active[SNDRV_PCM_STREAM_LAST + 1];
	bool slave_mode[SNDRV_PCM_STREAM_LAST + 1];
	bool lrck_inv[SNDRV_PCM_STREAM_LAST + 1];
	bool bck_inv[SNDRV_PCM_STREAM_LAST + 1];
	bool enable_interlink[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int lrck_width[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int data_mode[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int format[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int mclk_freq[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int clock_mode;
};

struct mt8696_pcm_intf_data {
	bool slave_mode;
	bool lrck_inv;
	bool bck_inv;
	unsigned int format;
};

struct mt8696_multi_in_data {
	bool lrck_inv;
	bool bck_inv;
	unsigned int format;
	unsigned int period_update_bytes;
	unsigned int notify_irq_count;
	unsigned int current_irq_count;
};

struct mt8696_spdif_in_data {
	unsigned int port;
	unsigned int rate;
	unsigned int ch_status[6];
	unsigned int ports_mux[SPDIF_IN_PORT_NUM];
};

struct mt8696_etdm_ctrl_reg {
	unsigned int con0;
	unsigned int con1;
	unsigned int con2;
	unsigned int con3;
	unsigned int con4;
};

struct mt8696_control_data {
	bool bypass_cm0;
	bool bypass_cm1;
	bool spdif_output_iec61937;
};

#define DMIC_MAX_CH (8)

enum {
	DMIC_3M25M = 0,
	DMIC_1P625M = 1,
	DMIC_812P5K = 2,
	DMIC_406P25K = 3
};

struct mt8696_dmic_data {
	bool two_wire_mode;
	unsigned int clk_phase_sel_ch1;
	unsigned int clk_phase_sel_ch2;
	unsigned int dmic_src_sel[DMIC_MAX_CH];
	bool iir_on;
	unsigned int setup_time_us;
	unsigned int ul_mode;
};

enum {
	MT8696_GASRC0 = 0,
	MT8696_GASRC1,
	MT8696_GASRC2,
	MT8696_GASRC3,
	MT8696_GASRC_NUM,
};

struct mt8696_gasrc_ctrl_reg {
	unsigned int con0;
	unsigned int con1;
	unsigned int con2;
	unsigned int con3;
	unsigned int con4;
	unsigned int con6;
	unsigned int con7;
	unsigned int con10;
	unsigned int con11;
	unsigned int con13;
	unsigned int con14;
};

struct mt8696_gasrc_data {
	unsigned int input_mux;
	unsigned int output_mux;
	bool cali_tx;
	bool cali_rx;
	bool one_heart;
	bool iir_on;
	bool duplex;
	bool op_freq_45m;
	unsigned int cali_cycles;
};

enum mt8696_afe_gasrc_mux {
	MUX_GASRC_8CH = 0,
	MUX_GASRC_6CH,
	MUX_GASRC_4CH,
	MUX_GASRC_2CH,
};

struct mt8696_afe_gasrc_mux_map {
	int gasrc_id;
	int idx;
	int mux;
};

enum mt8696_afe_gasrc_lrck_sel_src {
	MT8696_AFE_GASRC_LRCK_SEL_ETDM_IN2 = 0,
	MT8696_AFE_GASRC_LRCK_SEL_ETDM_IN1,
	MT8696_AFE_GASRC_LRCK_SEL_ETDM_OUT2,
	MT8696_AFE_GASRC_LRCK_SEL_ETDM_OUT1,
	MT8696_AFE_GASRC_LRCK_SEL_PCM_IF,
	MT8696_AFE_GASRC_LRCK_SEL_UL_VIRTUAL,
};

struct mt8696_afe_private {
	struct clk *clocks[MT8696_CLK_NUM];
	struct mt8696_fe_dai_data fe_data[MT8696_AFE_MEMIF_NUM];
	struct mt8696_be_dai_data be_data[MT8696_AFE_BACKEND_NUM];
	struct mt8696_etdm_data etdm_data[MT8696_ETDM_SETS];

	int afe_on_ref_cnt;
	int top_cg_ref_cnt[MT8696_TOP_CG_NUM];
	void __iomem *afe_sram_vir_addr;
	u32 afe_sram_phy_addr;
	u32 afe_sram_size;
	bool use_bypass_afe_pinmux;
	/* locks */
	spinlock_t afe_ctrl_lock;
	struct mutex afe_clk_mutex;
	struct regmap *topckgen;
	struct regmap *scpsys;
	int block_dpidle_ref_cnt;
	struct mutex block_dpidle_mutex;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dentry[MT8696_AFE_DEBUGFS_NUM];
#endif
	int apll_tuner_ref_cnt[MT8696_AFE_APLL_NUM];
};

bool mt8696_afe_rate_supported(unsigned int rate, unsigned int id);
bool mt8696_afe_channel_supported(unsigned int channel, unsigned int id);

#endif
