/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8532_afe_common.h  --  Mediatek 8532 audio driver common definitions
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
 *
 */

#ifndef _MT8532_AFE_COMMON_H_
#define _MT8532_AFE_COMMON_H_

#define COMMON_CLOCK_FRAMEWORK_API
/* #define DEBUG_AFE_REGISTER_RW */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regmap.h>
#include <linux/hrtimer.h>
#include <linux/of_gpio.h>
#include <sound/asound.h>

#define SPDIF_CHSTS_NUM			6
#define SPDIF_USERCODE_NUM		12
#define MT8532_ETDM_MAX_CHANNELS	16
/* #define CALI_RESULT_CHECK */

enum {
	MT8532_AFE_MEMIF_DL_START,
	MT8532_AFE_MEMIF_DLM = MT8532_AFE_MEMIF_DL_START,  //dl8 24ch
	MT8532_AFE_MEMIF_DL11, //48ch
	MT8532_AFE_MEMIF_DL2,
	MT8532_AFE_MEMIF_DL3,
	MT8532_AFE_MEMIF_DL6,  //pcmo
	MT8532_AFE_MEMIF_DL7,  //spdif out
	MT8532_AFE_MEMIF_DL10, //hdmitx tdmout3
	MT8532_AFE_MEMIF_DL_END = MT8532_AFE_MEMIF_DL10,
	MT8532_AFE_MEMIF_UL_START,
	MT8532_AFE_MEMIF_UL1 = MT8532_AFE_MEMIF_UL_START,  //spdifin_mphone_multi1
	MT8532_AFE_MEMIF_UL2,  //cm1 8ch
	MT8532_AFE_MEMIF_UL3,  //etdmin2
	MT8532_AFE_MEMIF_UL4,
	MT8532_AFE_MEMIF_UL5,   //pcmi
	MT8532_AFE_MEMIF_UL6,   //hdmirx/dsd mphone_multi2
	MT8532_AFE_MEMIF_UL8,   //etdmin1
	MT8532_AFE_MEMIF_UL9,   //cm9 32ch
	MT8532_AFE_MEMIF_UL10,
	MT8532_AFE_MEMIF_UL_END = MT8532_AFE_MEMIF_UL10,
	MT8532_AFE_MEMIF_NUM,
	MT8532_AFE_BACKEND_BASE = MT8532_AFE_MEMIF_NUM,
	//master 24ch 12pin zone1/dsdout 6ch 2.8m/2ch 5.6m(11.2m)
	MT8532_AFE_IO_ETDM1_OUT = MT8532_AFE_BACKEND_BASE,
	MT8532_AFE_IO_ETDM1_IN,  //master 24ch for aec lpbk or amic 1pin
	//master 24ch 4pin zone2 2ch-i2sout or 16/24ch-TDM AEC ext lpbk
	MT8532_AFE_IO_ETDM2_OUT,
	MT8532_AFE_IO_ETDM2_IN,  //master/slave 8ch linein 4pin
	//master 8ch /dsdout 6ch 2.8m/2ch 5.6m(11.2m)
	MT8532_AFE_IO_ETDM3_OUT,
	MT8532_AFE_IO_PCM1,
	MT8532_AFE_IO_DMIC,
	MT8532_AFE_IO_GASRC0,
	MT8532_AFE_IO_GASRC1,
	MT8532_AFE_IO_GASRC2,
	MT8532_AFE_IO_GASRC3,
	MT8532_AFE_IO_GASRC4,
	MT8532_AFE_IO_GASRC5,
	MT8532_AFE_IO_GASRC6,
	MT8532_AFE_IO_GASRC7,
	MT8532_AFE_IO_GASRC8,
	MT8532_AFE_IO_GASRC9,
	MT8532_AFE_IO_GASRC10,
	MT8532_AFE_IO_GASRC11,
	MT8532_AFE_IO_GASRC12,
	MT8532_AFE_IO_GASRC13,
	MT8532_AFE_IO_GASRC14,
	MT8532_AFE_IO_GASRC15,
	MT8532_AFE_IO_GASRC16,
	MT8532_AFE_IO_GASRC17,
	MT8532_AFE_IO_GASRC18,
	MT8532_AFE_IO_GASRC19,
	MT8532_AFE_IO_SPDIF_OUT,
	MT8532_AFE_IO_SPDIF_IN,
	MT8532_AFE_IO_MULTI_IN,
	MT8532_AFE_IO_VIRTUAL_IN,
	MT8532_AFE_BACKEND_END,
	MT8532_AFE_BACKEND_NUM = (MT8532_AFE_BACKEND_END -
				  MT8532_AFE_BACKEND_BASE),
};

enum {
	MT8532_AFE_IRQ1, /* SPDIF OUT */
	MT8532_AFE_IRQ2, /* SPDIF IN DETECT */
	/* SPDIF IN DATA (Mphone_multi counter mode/direct mode UL1)*/
	MT8532_AFE_IRQ3,
	MT8532_AFE_IRQ4, /*DL10*/
	MT8532_AFE_IRQ5, /*DL2*/
	MT8532_AFE_IRQ6, /*DL3*/
	MT8532_AFE_IRQ7, /*TDMOUT*/
	MT8532_AFE_IRQ8, /* Mphone_mulit_spdif_iecdata_dec */
	MT8532_AFE_IRQ9, /* Mphone_multi2 counter mode/direct mode UL6*/
	MT8532_AFE_IRQ10, /* Mphone_multi2_spdif_iecdata_dec */
	MT8532_AFE_IRQ11, /* common 1*/
	MT8532_AFE_IRQ12, /* common 2*/
	MT8532_AFE_IRQ13, /* common 3*/
	MT8532_AFE_IRQ14, /* common 4*/
	MT8532_AFE_IRQ15, /* common 5*/
	MT8532_AFE_IRQ16, /* common 6*/
	MT8532_AFE_IRQ17, /* common 7*/
	MT8532_AFE_IRQ18, /* common 8*/
	MT8532_AFE_IRQ19, /* common 9*/
	MT8532_AFE_IRQ20, /* common 10*/
	MT8532_AFE_IRQ21, /* common 11*/
	MT8532_AFE_IRQ22, /* common 12*/
	MT8532_AFE_IRQ_NUM,
};

enum {
	MT8532_TOP_CG_AFE,
	MT8532_TOP_CG_SPDIFIN_TUNER_APLL,
	MT8532_TOP_CG_SPDIFIN_TUNER_DBG,
	MT8532_TOP_CG_APLL_TUNER,
	MT8532_TOP_CG_APLL2_TUNER,
	MT8532_TOP_CG_SPDIF_OUT,
	MT8532_TOP_CG_APLL,
	MT8532_TOP_CG_APLL2,
	MT8532_TOP_CG_TML,
	MT8532_TOP_CG_26M_DMIC_TM,
	MT8532_TOP_CG_26M_DMIC4,
	MT8532_TOP_CG_26M_DMIC3,
	MT8532_TOP_CG_26M_DMIC2,
	MT8532_TOP_CG_26M_DMIC1,
	MT8532_TOP_CG_A1SYS_HP,
	MT8532_TOP_CG_LINEIN_TUNER,
	MT8532_TOP_CG_EARC_TUNER,
	MT8532_TOP_CG_I2S_IN,
	MT8532_TOP_CG_TDM_IN,
	MT8532_TOP_CG_I2S_OUT,
	MT8532_TOP_CG_TDM_OUT,
	MT8532_TOP_CG_HDMI_OUT, //21
	MT8532_TOP_CG_ASRC11,
	MT8532_TOP_CG_ASRC12,
	MT8532_TOP_CG_MULTI_IN,
	MT8532_TOP_CG_INTDIR,
	MT8532_TOP_CG_A1SYS,
	MT8532_TOP_CG_A2SYS,
	MT8532_TOP_CG_AFE_CONN,
	MT8532_TOP_CG_PCMIF,
	MT8532_TOP_CG_A3SYS,
	MT8532_TOP_CG_A4SYS,
	MT8532_TOP_CG_GASRC0,
	MT8532_TOP_CG_GASRC1,
	MT8532_TOP_CG_GASRC2,
	MT8532_TOP_CG_GASRC3,
	MT8532_TOP_CG_GASRC4,
	MT8532_TOP_CG_GASRC5,
	MT8532_TOP_CG_GASRC6,
	MT8532_TOP_CG_GASRC7,
	MT8532_TOP_CG_GASRC8,
	MT8532_TOP_CG_GASRC9,
	MT8532_TOP_CG_GASRC10,
	MT8532_TOP_CG_GASRC11,
	MT8532_TOP_CG_GASRC12,
	MT8532_TOP_CG_GASRC13,
	MT8532_TOP_CG_GASRC14,
	MT8532_TOP_CG_GASRC15,
	MT8532_TOP_CG_GASRC16,
	MT8532_TOP_CG_GASRC17,
	MT8532_TOP_CG_GASRC18,
	MT8532_TOP_CG_GASRC19,
	MT8532_TOP_CG_DMIC0,
	MT8532_TOP_CG_DMIC1,
	MT8532_TOP_CG_DMIC2,
	MT8532_TOP_CG_DMIC3,
	MT8532_TOP_CG_A1SYS_TIMING,
	MT8532_TOP_CG_A2SYS_TIMING,
	MT8532_TOP_CG_A3SYS_TIMING,
	MT8532_TOP_CG_A4SYS_TIMING,
	MT8532_TOP_CG_LP_MOD, //56
	MT8532_TOP_CG_LP_26M_ENGEN,
	MT8532_TOP_CG_NUM
};

enum {
	//MT8532_CLK_TOP_SYSPLL1_D8,
	//MT8532_CLK_TOP_AXI_SEL,
	MT8532_CLK_TOP_AUDIO_26MSEL,
	MT8532_CLK_TOP_AUDIO_SEL,
	MT8532_CLK_TOP_AUD_INTBUSSEL,
	MT8532_CLK_INFRA_IPSYS,
	MT8532_CLK_TOP_A1SYSHP_SEL,
	MT8532_CLK_TOP_INTDIR_SEL,
	MT8532_CLK_TOP_A2SYSHP_SEL,
	MT8532_CLK_TOP_A3SYSHP_SEL,
	MT8532_CLK_TOP_A4SYSHP_SEL,
	MT8532_CLK_TOP_APLL3_D4,
	MT8532_CLK_TOP_APLL4_D4,
	MT8532_CLK_TOP_APLL5_D4,
	MT8532_CLK_TOP_HDMIRX_APLL_D6,
	MT8532_CLK_TOP_APLL_SEL,
	MT8532_CLK_TOP_APLL2_SEL,
	MT8532_CLK_TOP_APLL3_SEL,
	MT8532_CLK_TOP_APLL4_SEL,
	MT8532_CLK_TOP_APLL5_SEL,
	MT8532_CLK_TOP_APLL1_D8,
	MT8532_CLK_TOP_APLL2_D8,
	MT8532_CLK_TOP_APLL3_D8,
	MT8532_CLK_TOP_APLL4_D8,
	MT8532_CLK_TOP_APLL5_D8,
	MT8532_CLK_TOP_ASML_SEL,
	MT8532_CLK_TOP_ASMM_SEL,
	MT8532_CLK_TOP_ASMH_SEL,
	MT8532_CLK_TOP_APLL12_DIV4, //for iec div pdn
	MT8532_CLK_TOP_APLL12_DIV0, //for mclk div pdn
	MT8532_CLK_TOP_APLL12_DIV1,
	MT8532_CLK_TOP_APLL12_DIV2,
	MT8532_CLK_TOP_APLL12_DIV3,
	MT8532_CLK_TOP_AUD_IEC, // for iec div
	MT8532_CLK_TOP_I2SI1_M,  //for mclk div
	MT8532_CLK_TOP_I2SI2_M,
	MT8532_CLK_TOP_I2SO1_M,
	MT8532_CLK_TOP_I2SO2_M,
	MT8532_CLK_TOP_I2SI1_SEL,
	MT8532_CLK_TOP_I2SI2_SEL,
	MT8532_CLK_TOP_I2SO1_SEL,
	MT8532_CLK_TOP_I2SO2_SEL,
	MT8532_CLK_APMIXED_APLL1,
	MT8532_CLK_APMIXED_APLL2,
	MT8532_CLK_APMIXED_APLL3,
	MT8532_CLK_APMIXED_APLL4,
	MT8532_CLK_APMIXED_APLL5,
	MT8532_CLK_NUM
};

enum {
	MT8532_ETDM1 = 0,
	MT8532_ETDM2,
	MT8532_ETDM3,
	MT8532_ETDM_SETS,
};

enum {
	MT8532_ETDM_DATA_ONE_PIN = 0,
	MT8532_ETDM_DATA_MULTI_PIN,
};

enum {
	MT8532_ETDM_SEPARATE_CLOCK = 0,
	MT8532_ETDM_SHARED_CLOCK,
};

enum {
	MT8532_ETDM_SYNC_NONE = 0,
	MT8532_ETDM_SYNC_FROM_IN1,
	MT8532_ETDM_SYNC_FROM_IN2,
	MT8532_ETDM_SYNC_FROM_OUT1,
	MT8532_ETDM_SYNC_FROM_OUT2,
	MT8532_ETDM_SYNC_FROM_OUT3,
};

enum {
	MT8532_ETDM_FORMAT_I2S = 0,
	MT8532_ETDM_FORMAT_LJ,
	MT8532_ETDM_FORMAT_RJ,
	MT8532_ETDM_FORMAT_EIAJ,
	MT8532_ETDM_FORMAT_DSPA,
	MT8532_ETDM_FORMAT_DSPB,
};

enum {
	MT8532_PCM_FORMAT_I2S = 0,
	MT8532_PCM_FORMAT_EIAJ,
	MT8532_PCM_FORMAT_PCMA,
	MT8532_PCM_FORMAT_PCMB,
};

enum {
	MT8532_MULTI_IN_FORMAT_I2S = 0,
	MT8532_MULTI_IN_FORMAT_LJ,
	MT8532_MULTI_IN_FORMAT_RJ,
};

enum {
	MT8532_MULTI_IN_UNKNOWN = -1,
	MT8532_MULTI_IN_ROUGH_PCM = 0,
	MT8532_MULTI_IN_ROUGH_RAW,
	MT8532_MULTI_IN_ROUGH_DTSCD16,
	MT8532_MULTI_IN_ROUGH_DTSCD14,
};

enum {
	MT8532_FS_8K = 0,
	MT8532_FS_12K,
	MT8532_FS_16K,
	MT8532_FS_24K,
	MT8532_FS_32K,
	MT8532_FS_48K,
	MT8532_FS_96K,
	MT8532_FS_192K,
	MT8532_FS_384K,
	MT8532_FS_ETDMOUT1_1X_EN,
	MT8532_FS_ETDMOUT2_1X_EN,
	MT8532_FS_ETDMOUT3_1X_EN,
	MT8532_FS_ETDMIN1_1X_EN = 12,
	MT8532_FS_ETDMIN2_1X_EN,
	MT8532_FS_EXT_PCM_1X_EN = 15,
	MT8532_FS_7D35K,
	MT8532_FS_11D025K,
	MT8532_FS_14D7K,
	MT8532_FS_22D05K,
	MT8532_FS_29D4K,
	MT8532_FS_44D1K,
	MT8532_FS_88D2K,
	MT8532_FS_176D4K,
	MT8532_FS_352D8K,
	MT8532_FS_ETDMIN1_NX_EN,
	MT8532_FS_ETDMIN2_NX_EN = 26,
	MT8532_FS_AMIC_1X_EN_ASYNC = 28,
};

enum {
	MT8532_TDM_8K = 0,
	MT8532_TDM_12K,
	MT8532_TDM_16K,
	MT8532_TDM_24K,
	MT8532_TDM_32K,
	MT8532_TDM_48K,
	MT8532_TDM_96K = 7,
	MT8532_TDM_192K = 9,
	MT8532_TDM_384K = 11,

	MT8532_TDM_11D025K = 16,
	MT8532_TDM_22D05K,
	MT8532_TDM_44D1K,
	MT8532_TDM_88D2K,
	MT8532_TDM_176D4K,
	MT8532_TDM_352D8K,

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
	MT8532_ETDM_FORCE_ON_DEFAULT = 0,
	MT8532_ETDM_FORCE_ON_1ST_TRIGGER,
};

enum {
	MT8532_AFE_DEBUGFS_ETDM,
	MT8532_AFE_DEBUGFS_MEMIF,
	MT8532_AFE_DEBUGFS_IRQ,
	MT8532_AFE_DEBUGFS_CONN,
	MT8532_AFE_DEBUGFS_GASRC,
	MT8532_AFE_DEBUGFS_SPDIF,
	MT8532_AFE_DEBUGFS_DBG,
	MT8532_AFE_DEBUGFS_NUM,
};

enum tv_input_path_sel {
	TV_INPUT_PATH_SEL_NONE = 0,
	PATH_SEL_INTERNAL_EARC,
	PATH_SEL_INTERNAL_ARC,
};

struct mt8532_fe_dai_data {
	bool slave_mode;
	bool use_sram;
	unsigned int sram_phy_addr;
	void __iomem *sram_vir_addr;
	unsigned int sram_size;
	unsigned int prealloc_size;
	unsigned int pbuf_size_conf;
	unsigned int min_hw_irq_period_us;
	unsigned int fs_1xen_sys;
};

struct mt8532_be_dai_data {
	bool prepared[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int fmt_mode;
	unsigned int fs_1xen_sys;
};

struct mt8532_etdm_data {
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
	bool in_disable_ch[MT8532_ETDM_MAX_CHANNELS];
	bool force_on[SNDRV_PCM_STREAM_LAST + 1];
	bool force_on_status[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int force_on_policy[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int force_rate[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int force_channels[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int force_bit_width[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int sync_source[SNDRV_PCM_STREAM_LAST + 1];
	bool int_lrck_inv[SNDRV_PCM_STREAM_LAST + 1];
	bool int_bck_inv[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int enable_seq[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int dsd_mod[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int dsd_bit[SNDRV_PCM_STREAM_LAST + 1];
	unsigned int dsd_clk[SNDRV_PCM_STREAM_LAST + 1];
};

struct mt8532_pcm_intf_data {
	bool slave_mode;
	bool lrck_inv;
	bool bck_inv;
	unsigned int format;
};

struct mt8532_multi_in_data {
	bool lrck_inv;
	bool bck_inv;
	unsigned int format;
	unsigned int period_update_bytes;
	unsigned int notify_irq_count;
	unsigned int current_irq_count;
};

struct mt8532_spdif_in_subdata {
	unsigned int rate;
	unsigned int user_code[SPDIF_USERCODE_NUM];
	unsigned int ch_status[SPDIF_CHSTS_NUM];
	unsigned int stream_type;
};

struct mt8532_spdif_in_data {
	unsigned int port;
	unsigned int ports_mux[SPDIF_IN_PORT_NUM];
	struct mt8532_spdif_in_subdata subdata;
};

struct mt8532_multi_in_mon_info {
	int rough_type;
	int data_type;
	int bitstream_number;
	int pc_b12_to_b5;
};

struct mt8532_etdm_ctrl_reg {
	unsigned int con0;
	unsigned int con1;
	unsigned int con2;
	unsigned int con3;
	unsigned int con4;
	unsigned int con5;
};

struct mt8532_control_data {
	bool bypass_cm0;
	bool bypass_cm1;
	bool spdif_output_iec61937;
};

#define DMIC_MAX_CH (8)

struct mt8532_dmic_data {
	bool two_wire_mode;
	unsigned int clk_phase_sel_ch1;
	unsigned int clk_phase_sel_ch2;
	unsigned int dmic_src_sel[DMIC_MAX_CH];
	bool iir_on;
	unsigned int setup_time_us;
	unsigned int ul_mode;
};

enum {
	MT8532_GASRC0 = 0,
	MT8532_GASRC1,
	MT8532_GASRC2,
	MT8532_GASRC3,
	MT8532_GASRC4,
	MT8532_GASRC5,
	MT8532_GASRC6,
	MT8532_GASRC7,
	MT8532_GASRC8,
	MT8532_GASRC9,
	MT8532_GASRC10,
	MT8532_GASRC11,
	MT8532_GASRC12,
	MT8532_GASRC13,
	MT8532_GASRC14,
	MT8532_GASRC15,
	MT8532_GASRC16,
	MT8532_GASRC17,
	MT8532_GASRC18,
	MT8532_GASRC19,
	MT8532_GASRC_NUM,
};



struct mt8532_gasrc_ctrl_reg {
	unsigned int con0;
	unsigned int con1;
	unsigned int con2;
	unsigned int con3;
	unsigned int con4;
	unsigned int con5;
	unsigned int con6;
	unsigned int con7;
	unsigned int con10;
	unsigned int con11;
	unsigned int con13;
	unsigned int con14;
};

struct mt8532_gasrc_data {
	unsigned int input_mux;
	unsigned int output_mux;
	bool cali_tx;
	bool cali_rx;
	bool one_heart;
	bool iir_on;
	bool duplex;
	bool op_freq_45m;
	unsigned int cali_cycles;
	bool re_enable[SNDRV_PCM_STREAM_LAST + 1];
	atomic_t ref_cnt;
};

enum mt8532_afe_gasrc_mux {
	MUX_GASRC_8CH = 0,
	MUX_GASRC_6CH,
	MUX_GASRC_4CH,
	MUX_GASRC_2CH,
};

struct mt8532_afe_gasrc_mux_map {
	int gasrc_id;
	int idx;
	int mux;
};

enum mt8532_afe_gasrc_lrck_sel_src {
	MT8532_AFE_GASRC_LRCK_SEL_ETDM_IN2 = 0,  //i2s in slave
	MT8532_AFE_GASRC_LRCK_SEL_ETDM_IN1,
	MT8532_AFE_GASRC_LRCK_SEL_ETDM_OUT2,
	MT8532_AFE_GASRC_LRCK_SEL_ETDM_OUT1,
	MT8532_AFE_GASRC_LRCK_SEL_PCM_IF,
	MT8532_AFE_GASRC_LRCK_SEL_UL_VIRTUAL,
};

enum {
	MT8532_APLL1_RATE = 180633600,
	MT8532_APLL2_RATE = 196608000,
};

enum {
	CLK_TUNE_SPDIF_IN = 0,
	CLK_TUNE_MULTI_IN,
	CLK_TUNE_SOURCE_NUM,
};

enum {
	TUNE_PHASE_INIT = 0,
	TUNE_PHASE_START_CALI_PROC,
	TUNE_PHASE_GET_CALI_RESULT,
	TUNE_PHASE_CHECK_OUTPUT_ACTIVE,
	TUNE_PHASE_ADJUST_APLL_RATE,
	TUNE_PHASE_ADJUST_DONE,
	TUNE_PHASE_ALL_DONE,
	TUNE_PHASE_ABORT,
};

enum {
	PBUF_SIZE_FULL = 0,
	PBUF_SIZE_HALF,
	PBUF_SIZE_QUARTER,
	PBUF_SIZE_EIGHTH,
	PBUF_SIZE_CONF_NUM,
};

enum {
	EN_IN_PREP_DIS_IN_SD = 0,
	EN_DIS_IN_TRIGGER,
};

struct ext_clk_tune_property {
	bool do_tune;
	unsigned int tune_id;
	unsigned int period_ms;
	unsigned int adj_step_ppm;
	unsigned int adj_step_us;
};

struct mt8532_afe_ext_clk_tune_data {
	struct ext_clk_tune_property props[CLK_TUNE_SOURCE_NUM];
	struct ext_clk_tune_property *working;
	struct delayed_work clk_tune_work;
	int working_phase;
	bool start;
	unsigned int asrc_id;
	unsigned int cali_cycles;
	unsigned int input_rate;
	u64 cali_retry_time;
	int cali_retry_cnt;
	u64 cali_input_rate_10uhz;
	unsigned long apll1_rate;
	unsigned long apll2_rate;
	unsigned long apll_target_rate;
	unsigned long apll_current_rate;
	unsigned long apll_restore_rate;
	unsigned long apll_step_hz;
	unsigned int outputs;
	unsigned int output_rate;
	void *apll_clk;
	void *afe;
};

enum {
	MT8532_AFE_TDMOUT_CONN_I0 = 0,
	MT8532_AFE_TDMOUT_CONN_I15 = 15,
	MT8532_AFE_TDMOUT_CONN_CFG_NUM = 16,
};

struct mt8532_afe_tdmout_conn_data {
	unsigned int out_cfg[MT8532_AFE_TDMOUT_CONN_CFG_NUM];
};

#ifdef CALI_RESULT_CHECK
struct mt8532_cali_result {
	struct hrtimer cali_hrt;
	int cnt;
	snd_pcm_sframes_t max;
	snd_pcm_sframes_t min;
	snd_pcm_sframes_t avg;
	snd_pcm_sframes_t cur;
	u64 sum;
	void *afe;
};
#endif

enum mt8532_afe_engen_timing {
	FS_ENGEN_A1SYS = 0,
	FS_ENGEN_A2SYS,
	FS_ENGEN_A3SYS,
	FS_ENGEN_A4SYS,
	FS_ENGEN_26M,
};

enum mt8532_afe_etdm_mod {
	PCM_MOD = 0,
	DSD_MOD,
};

enum mt8532_afe_etdm_dsd_bit {
	DSD_8_BIT = 0,
	DSD_16_BIT,
	DSD_24_BIT,
	DSD_32_BIT,
};

enum mt8532_afe_etdm_dsd_clk {
	DSD_2P8M = 0,
	DSD_5P6M,
	DSD_11P2M,
};

enum {
	AFE_PIN_STATE_DEFAULT = 0,
	AFE_PIN_STATE_ETDM1_OUT_ON,
	AFE_PIN_STATE_ETDM1_OUT_OFF,
	AFE_PIN_STATE_ETDM1_IN_ON,
	AFE_PIN_STATE_ETDM1_IN_OFF,
	AFE_PIN_STATE_ETDM2_OUT_ON,
	AFE_PIN_STATE_ETDM2_OUT_OFF,
	AFE_PIN_STATE_ETDM2_IN_ON,
	AFE_PIN_STATE_ETDM2_IN_OFF,
	AFE_PIN_STATE_MAX
};

struct mt8532_afe_private {
	struct clk *clocks[MT8532_CLK_NUM];
	struct mt8532_fe_dai_data fe_data[MT8532_AFE_MEMIF_NUM];
	struct mt8532_be_dai_data be_data[MT8532_AFE_BACKEND_NUM];
	struct mt8532_etdm_data etdm_data[MT8532_ETDM_SETS];
	struct mt8532_pcm_intf_data pcm_intf_data;
	struct mt8532_multi_in_data multi_in_data;
	struct mt8532_spdif_in_data spdif_in_data;
	struct mt8532_control_data ctrl_data;
	struct mt8532_dmic_data dmic_data;
	struct mt8532_afe_tdmout_conn_data tdmo_conn;
	struct mt8532_gasrc_data gasrc_data[MT8532_GASRC_NUM];
	struct mt8532_afe_ext_clk_tune_data clk_tune;
	struct mt8532_multi_in_mon_info multi_in_mon_info;
#ifdef CALI_RESULT_CHECK
	struct mt8532_cali_result cali_res;
#endif

	int afe_on_ref_cnt;
	int top_cg_ref_cnt[MT8532_TOP_CG_NUM];
	void __iomem *afe_sram_va;
	u32 afe_sram_pa;
	u32 afe_sram_size;
	bool use_bypass_afe_pinmux;
	u32 be_active_status;
	bool dl8_enable_24ch_output;
	u32 dl8_max_main_channels;
	/* locks */
	spinlock_t afe_ctrl_lock;
	spinlock_t spdifin_ctrl_lock;
	struct regmap *topckgen;
	struct regmap *scpsys;
	int block_dpidle_ref_cnt;
	struct mutex block_dpidle_mutex;
	struct snd_card *card;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_states[AFE_PIN_STATE_MAX];
	bool handle_etdm_force_in_suspend_resume;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dentry[MT8532_AFE_DEBUGFS_NUM];
#endif

	int mmap_playback_state;
	int mmap_record_state;
	int mmap_ion_mem_ready;
	int mmap_dl_memif_id;
	int mmap_ul_memif_id;

};

bool mt8532_afe_rate_supported(unsigned int rate, unsigned int id);
bool mt8532_afe_channel_supported(unsigned int channel, unsigned int id);

#endif
