// SPDX-License-Identifier: GPL-2.0
/*
 * mt8532_earc.h  --  Mediatek 8532 earc driver
 *
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/string.h>
#include <linux/completion.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <linux/of.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/debugfs.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/fb.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include "mtk-earc-rx.h"

#define EARC_DEVNAME "earc"

static struct timer_list earc_timer;
static struct task_struct *earc_timer_task;
wait_queue_head_t earc_timer_wq;
static unsigned int gEARC_CHK_INTERVAL = 10;
static unsigned int earc_hb_count;
static unsigned int earc_test_value;
atomic_t earc_timer_event = ATOMIC_INIT(0);
bool b_assert_status;

struct task_struct *earc_irq_task;
wait_queue_head_t earc_irq_wq;
atomic_t earc_irq_event = ATOMIC_INIT(0);

struct device_node *earc_node;
struct device *earc_dev;
unsigned int audfmt_notify;
unsigned int earc_irq;
unsigned int earc_poweron;
unsigned int onebitado;
unsigned int samplefreq;
unsigned int oldsamplefreq;
unsigned int channelca;
unsigned int oldchannelca;
unsigned int hpdstate;
unsigned int audiofmt;
unsigned int oldaudiofmt;
unsigned int codectype;
unsigned int channelno;
unsigned int earcconnet;
unsigned int earcmute;
unsigned int info_change;
unsigned int rstcompressedB;
unsigned int oldfreq;
unsigned int csmute;
unsigned int caldone;
unsigned int hbdetdone;
unsigned int cschange;
unsigned int oldfs;
unsigned int oldfmeter;
unsigned int stablecnt;
unsigned int stableunmute;
unsigned int csbit98;
unsigned int csbit9c;
unsigned int csbita0;
unsigned int csbita4;
unsigned int csbita8;
unsigned int csbitac;
unsigned int csbitb0;
unsigned int csbitb4;
unsigned int csbitb8;
unsigned int csbitbc;
unsigned int csbitc0;
unsigned int csbitc4;



/* from DTS, for debug */
unsigned int earc_reg_base[EARC_REG_NUM] = {
	0x1a001000,		/*REG_EARC_DIGITAL */
	0x11f30000,		/*REG_EARC_ANALOG */
	0x1a000000,		/*REG_HDMITX_DIGITAL */
	0x10005000,		/*REG_EARC_Monitor */
};

/* Reigster of EARC , Include EARC digital,analog*/
unsigned long earc_reg[EARC_REG_NUM] = { 0 };

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef NULL
#define NULL	0
#endif

enum earc_sm_state {
	EARC_SM_STATE_IDLE1,
	EARC_SM_STATE_DISC1,
	EARC_SM_STATE_IDLE2,
	EARC_SM_STATE_DISC2,
	EARC_SM_STATE_ARC,
	EARC_SM_STATE_EARC
};
enum earc_pref_extraction_mode {
	EARC_PREF_EXTRACTION_MODE_SPDIF,
	EARC_PREF_EXTRACTION_MODE_I2S
};

enum earc_mclk_mode {
	EARC_MCLK_MODE_NONE,
	EARC_MCLK_MODE_FS128,
	EARC_MCLK_MODE_FS256,
	EARC_MCLK_MODE_FS512
};

struct earc_erx_latency_req {
	unsigned char data;
};

struct obj_t {

	/* Hardware device instantiation identification number */
	unsigned int dev_id;

	earc_event_callback_func event_cb_func;

	unsigned int log_fifo_size;

	/* Controls which events will cause notification handler */
	/* to be called */
	unsigned int event_flags_mask;

	/* Event Flags status*/
	unsigned int event_flags_status;

	bool b_earc_connected_status;

	/* eARC_HPD when in eARC mode Set by API*/
	bool b_earc_hpd;

	unsigned int earc_hpd_toggle_time;

	/* eARC_HPD to be updated on timer expiration */
	bool b_earc_hpd_pending;

	/* Flag for eARC capability data structure ready status */
	bool b_earc_caps_ds_ready;

	/* HPD to be updated on timer expiration */
	bool b_hpd_pending;

	bool b_hpd_in;

	/* Current HPD level HiGH/LOW*/
	bool b_hpd_out;

	/* ARC mode status (NONE, Legacy ARC, or EARC) */
	enum arc_mode arc_mode_status;

	/* Configured ARC mode */
	enum arc_mode arc_mode_configured;

	/* Current state in State machine */
	enum earc_sm_state sm_state;

	/* Configured preferred audio extraction mode (I2S, SPDIF) in case*/
	/* audio is received as PCM or compressed audio formats. Set by API */
	enum earc_pref_extraction_mode pref_extraction_mode;

	/* extraction mode status*/
	/* (NONE, SPDIF2, SPDIF8, I2S2, I2S8, DSD2, DSD6) */
	enum earc_extraction_mode extraction_mode_status;

	/* Configured mode of MCLK output signal.*/
	/* (NONE, FS128, FS256, FS512) Set by API */
	enum earc_mclk_mode mclock_mode;

	/* channel status data(only applicable to SPDIF2,SPDIF8,I2S2,I2S8*/
	/* extraction modes). */
	struct earc_channel_status channel_status;

	/* channel status data(only applicable to SPDIF2,SPDIF8,I2S2,I2S8*/
	/* extraction modes). */
	struct earc_channel_status channel_status_out;

	/* Mute Status for audio data. Set by API */
	bool b_audio_mute_user_set;

	/* GPIO Input/Output configuration */
	/*Each bit represents 1 GPIO pin based on the following assignment:*/
	/*	Bit 0   : GPIO0*/
	/*	Bit 1   : GPIO1*/
	/*	Bit 2   : GPIO2*/
	/*	Bit 3   : GPIO3*/
	/*	Bit 4   : GPIO4*/
	/*	Bit 5   : GPIO5*/
	/*	Bit 6..7: unused*/

	unsigned int mutex;

	unsigned char caps_ds[EARC_CAPS_DS_MAX_LENGTH];

	struct earc_erx_latency erx_latency;

	struct earc_erx_latency_req erx_latency_req;

	struct earc_audio_info audio_info;

	/* Config params from application */
	bool b_caps_ds_stress_test;

	/* Different sources of Mute*/
	unsigned char audio_mute_disable;

	bool earc_lock_detected;

	bool earc_clock_detected;

	bool ch_status_rcvd;

	bool is_input_ch_status_valid;

	bool mute_state;

	bool b_io_en_state;

	bool is_clk_det_intr_mask_enable;

	bool is_lock_det_intr_mask_enable;

	bool is_fs_chng_intr_mask_enable;

	bool is_cs_chng_intr_mask_enable;

	bool is_pkt_ecc_err_fix_intr_mask_en;

	bool is_pkt_ecc_err_intr_mask_en;

	bool is_ecc_err_intr_mask_en;
};

struct snd_soc_card *soc_card;

char const DefaultCapData[256] = {
	0x01,		/*  Capabilities Data Structure Version = 0x01  */
#ifdef _SL870ATC_
	0x01, 0x2C,		/*  BLOCK_ID=1, 44-byte  */
#else
	0x01, 0x26,
	/*  BLOCK_ID=1, 38-byte change by peter for SL-870 HFR5-2-36  */
#endif
	0x3B,	/*  Tag=1 (Audio Data Block), Length=27  */
	0x67, 0x7E, 0x03,	/*  HBR  : 8-ch, 44~192K (Dolby TrueHD) MAT  */
	0x57, 0x04, 0x03,	/*  HBR  : 8-ch, 44~48K (Dolby Digital)  */
	0x15, 0x07, 0x50,	/*  AC-3 : 6-ch, 32~48K  */
	0x0F, 0x7F, 0x07,	/*  LPCM : 8-ch, 32~192K  */
	0x09, 0x7F, 0x07,	/*  LPCM : 2-Ch, 32-192K  */
	0x35, 0x06, 0x3C,	/*  AAC  : 6-ch, 44~48K  */
	0x3E, 0x1E, 0xC0,	/*  DTS  : 7-ch, 44~96K  */
	0x4D, 0x02, 0x00,	/*  DSD  : 6-ch, 44K  */
	0x5F, 0x7E, 0x01,	/*  HBR  : 8-ch, 44~192K (DTS-HD)  */
	/* 0x20 offset  */
	0x83,
	/*  Tag=4 (Speaker Allocation Data Block), 3-bye  */
	0x6F, 0x0F, 0x0C,

#ifdef _SL870ATC_
	0xEB, 0x13,
	/*  Tag Code=7, Length=11,*/
	/*Extended Tag Code=0x13 (Room Configuration Data Block)  */
	0x00, 0x6F, 0x0F, 0x0C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* change by peter for SL-870 HFR5-2-36  */
#else
	/* 0x24 offset  */
	0xE5, 0x13,		/*  change by peter for SL-870 HFR5-2-36  */
	0x00, 0x6F, 0x0F, 0x0C,
#endif

#ifdef _QD980ATC_		/*  temp for QD980 HFR5-2-36   0x2A offset  */
	0x02, 0x0C,		/*  BLOCK_ID=2, 12-byte  */
	0xEB, 0x14,
	/*Tag Code=7, Length=11, Extended Tag Code=0x14*/
#else
	0x02, 0x0A,
	/*  BLOCK_ID=2, 10-byte*/
	/*  should be marked for QD980 HFR5-2-26*/
#endif

	0x20, 0x00, 0x00, 0x00, 0x00,
	0x21, 0x01, 0x01, 0x01, 0x01,

	0x03, 0x01,		/*  BLOCK_ID=3, 1-byte  */
	0x89,
	/*  Supports_AI=1, ONE_BIT_AUDIO_LAYOUT=1 (12-ch)*/
	/*MULTI_CH_LPCM_LAYOUT=1 (16-ch)  */

#ifndef _QD980ATC_
	0x00, 0x00,
	/*  should be marked for QD980 HFR5-2-26  */
#endif

#ifndef _SL870ATC_
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
#endif

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

char const DefaultCapData255[256] = {
	0x01,		/*  Capabilities Data Structure Version = 0x01  */

	0x01, 0x26,
	/*  BLOCK_ID=1, 38-byte change by peter for SL-870 HFR5-2-36  */
	0x3B,	/*  Tag=1 (Audio Data Block), Length=27  */
	0x67, 0x7E, 0x03,	/*  HBR  : 8-ch, 44~192K (Dolby TrueHD) MAT  */
	0x57, 0x04, 0x03,	/*  HBR  : 8-ch, 44~48K (Dolby Digital)  */
	0x15, 0x07, 0x50,	/*  AC-3 : 6-ch, 32~48K  */
	0x0F, 0x7F, 0x07,	/*  LPCM : 8-ch, 32~192K  */
	0x09, 0x7F, 0x07,	/*  LPCM : 2-Ch, 32-192K  */
	0x35, 0x06, 0x3C,	/*  AAC  : 6-ch, 44~48K  */
	0x3E, 0x1E, 0xC0,	/*  DTS  : 7-ch, 44~96K  */
	0x4D, 0x02, 0x00,	/*  DSD  : 6-ch, 44K  */
	0x5F, 0x7E, 0x01,	/*  HBR  : 8-ch, 44~192K (DTS-HD)  */
	/* 0x20 offset  */
	0x83,
	/*  Tag=4 (Speaker Allocation Data Block), 3-bye  */
	0x6F, 0x0F, 0x0C,


	/* 0x24 offset  */
	0xE5, 0x13,		/*  change by peter for SL-870 HFR5-2-36  */
	0x00, 0x6F, 0x0F, 0x0C,


	0x02, 0x0A,
	/*  BLOCK_ID=2, 10-byte*/
	/*  should be marked for QD980 HFR5-2-26*/

	0x20, 0x00, 0x00, 0x00, 0x00,
	0x21, 0x01, 0x01, 0x01, 0x01,

	0x03, 0x01,		/*  BLOCK_ID=3, 1-byte  */
	0x89,
	/*  Supports_AI=1, ONE_BIT_AUDIO_LAYOUT=1 (12-ch)*/
	/*MULTI_CH_LPCM_LAYOUT=1 (16-ch)  */

	0x00, 0x00,
	/*  should be marked for QD980 HFR5-2-26  */

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

char const DefaultCapData32255[256] = {
	0x01,		/*  Capabilities Data Structure Version = 0x01  */

	0x01, 0x1d,
	/*  BLOCK_ID=1, 38-byte change by peter for SL-870 HFR5-2-36  */
	0x38,	/*  Tag=1 (Audio Data Block), Length=27  */
	0x67, 0x7E, 0x03,	/*  HBR  : 8-ch, 44~192K (Dolby TrueHD) MAT  */
	0x57, 0x06, 0x03,	/*  HBR  : 8-ch, 44~48K (Dolby Digital)  */
	0x15, 0x07, 0x50,	/*  AC-3 : 6-ch, 32~48K  */
	0x09, 0x7F, 0x07,	/*	LPCM : 2-Ch, 32-192K  */
	0x0F, 0x7F, 0x07,	/*  LPCM : 8-ch, 32~192K  */
	0x3E, 0x1F, 0xC0,	/*	DTS  : 7-ch, 44~96K  */
	0x4D, 0x02, 0x00,	/*  DSD  : 6-ch, 44K  */
	0x5F, 0x54, 0x01,	/*  HBR  : 8-ch, 44~192K (DTS-HD)  */
	0x83,
	0x5F, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

char const DefaultCapData32[32] = {
	0x01, 0x01, 0x1d, 0x38, 0x67, 0x7e, 0x03,
	0x57, 0x06, 0x03, 0x15, 0x07, 0x50, 0x09, 0x7f, 0x07,
	0x0f, 0x7f, 0x07, 0x3e, 0x1f, 0xc0, 0x4d, 0x02, 0x00,
	0x5f, 0x54, 0x01, 0x83, 0x5f, 0x00, 0x00};
static int earc_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai);

void internal_earc_read(unsigned long u4Reg, unsigned int *p4Data)
{
	*p4Data = readl((void *)u4Reg);
	/*if (earc_test_value == 1)*/
		/*EARC_LOG("[R]addr = 0x%x, data = 0x%08x\n", u4Reg, *p4Data);*/
}

void internal_earc_write(unsigned long u4Reg, unsigned int u4data)
{
	writel(u4data, (void *)u4Reg);
	/*if (earc_test_value == 2)*/
		/*EARC_LOG("[W]addr = 0x%x, data = 0x%08x\n", u4Reg, u4data);*/
}

unsigned int earc_hdmi_read(unsigned int u2Reg)
{
	unsigned int u4Data;

	internal_earc_read(earc_reg[REG_HDMITX_DIG] + u2Reg, &u4Data);

	/*EARC_LOG("[R]addr = 0x%04x, data = 0x%08x\n", u2Reg, u4Data);*/

	return u4Data;
}

void earc_hdmi_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_earc_write(earc_reg[REG_HDMITX_DIG] + u2Reg, u4Data);
}

unsigned int earc_drv_read(unsigned int u2Reg)
{
	unsigned int u4Data;

	internal_earc_read(earc_reg[REG_EARC_DIG] + u2Reg, &u4Data);

	/*EARC_LOG("[R]addr = 0x%04x, data = 0x%08x\n", u2Reg, u4Data);*/

	return u4Data;
}

void earc_drv_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_earc_write(earc_reg[REG_EARC_DIG] + u2Reg, u4Data);
}

unsigned int earc_ana_read(unsigned int u2Reg)
{
	unsigned int u4Data;

	internal_earc_read(earc_reg[REG_EARC_ANA] + u2Reg, &u4Data);
	/*EARC_LOG("[R]addr = 0x%04x, data = 0x%08x\n", u2Reg, u4Data);*/
	return u4Data;
}

void earc_ana_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_earc_write(earc_reg[REG_EARC_ANA] + u2Reg, u4Data);
}

unsigned int earc_monitor_read(unsigned int u2Reg)
{
	unsigned int u4Data;

	internal_earc_read(earc_reg[REG_EARC_MONITOR] + u2Reg, &u4Data);
	/*EARC_LOG("[R]addr = 0x%04x, data = 0x%08x\n", u2Reg, u4Data);*/
	return u4Data;
}

void earc_monitor_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_earc_write(earc_reg[REG_EARC_MONITOR] + u2Reg, u4Data);
}

static const struct snd_soc_dapm_widget dir_widgets[] = {
	SND_SOC_DAPM_INPUT("earc-in"),
};

static const struct snd_soc_dapm_route dir_routes[] = {
	{ "Capture", NULL, "earc-in" },
};

static int earc_enable_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	/*todo: return earc enable or not*/
	ucontrol->value.integer.value[0] = earc_poweron;
	return 0;
}

static int earc_enable_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	int setting = ucontrol->value.integer.value[0];

	if (setting == earc_poweron)
		return 0;
	if (setting == 1) {
		earc_poweron = 1;
		earc_internal_power_on();
	} else if (setting == 2) {
		earc_poweron = 2;
		arc_internal_power_on();
	} else {
		if (earc_poweron == 1)
			earc_internal_power_off();
		else if (earc_poweron == 2)
			arc_internal_power_off();
		earc_poweron = 0;
	}
	/*ucontrol->value.integer.value[0] = earc_poweron;*/
	return 0;
}

static int earc_information_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = sizeof(struct earc_audio_info) /
		sizeof(unsigned int);

	return 0;
}

static int earc_information_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	/*todo: fill the audio_info here*/

	int index = 0;

	pr_debug("get fmt: %d; no: %d; type: %d, freq: %d\n",
		audiofmt, channelno, codectype, samplefreq);
	ucontrol->value.integer.value[index++] = audiofmt;
	ucontrol->value.integer.value[index++] = codectype;
	ucontrol->value.integer.value[index++] = channelno;
	ucontrol->value.integer.value[index++] = samplefreq;
	ucontrol->value.integer.value[index++] = earcconnet;
	ucontrol->value.integer.value[index++] = earcmute;
	ucontrol->value.integer.value[index++] = channelca;
	ucontrol->value.integer.value[index++] = hpdstate;
	return 0;
}

int cec_enable_arc(unsigned char u1EnArc)
{
	EARC_LOG("%s, u1EnArc:%d, earc_poweron:%d\n", __func__, u1EnArc, earc_poweron);
	if (u1EnArc == earc_poweron)
		return 0;
	if (u1EnArc == 1) {
		earc_poweron = 1;
		earc_internal_power_on();
	} else if (u1EnArc == 2) {
		earc_poweron = 2;
		arc_internal_power_on();
	} else {
		if (earc_poweron == 1) {
			/* earc on - power off earc */
			earc_internal_power_off();
		} else if (earc_poweron == 2) {
			/* arc on - power off arc */
			arc_internal_power_off();
		}
		earc_poweron = 0;
	}
	return 0;
}
EXPORT_SYMBOL(cec_enable_arc);

#define SND_SOC_CTL_RO(xname, xhandler_info, xhandler_get) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_READ | \
		  SNDRV_CTL_ELEM_ACCESS_VOLATILE, \
	.info = xhandler_info, .get = xhandler_get }

enum earc_enable_mode {
	EARC_MODE_OFF = 0,
	EARC_MODE_NORMAL,
	EARC_MODE_FORCE_ARC,
};

#define ENUM_TO_STR(enum) #enum
static const char *const earc_enable_modes[] = {
	ENUM_TO_STR(EARC_MODE_OFF),
	ENUM_TO_STR(EARC_MODE_NORMAL),
	ENUM_TO_STR(EARC_MODE_FORCE_ARC)
};

static const struct soc_enum earc_enalbe_mode_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(earc_enable_modes),
			earc_enable_modes);

static const struct snd_kcontrol_new dir_controls[] = {
	SOC_ENUM_EXT("Earc_Enable",
			earc_enalbe_mode_enum,
			earc_enable_get,
			earc_enable_put),
	SND_SOC_CTL_RO("Earc_Info",
			earc_information_info,
			earc_information_get),

};

#define STUB_RATES	SNDRV_PCM_RATE_8000_192000
#define STUB_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S20_3LE | \
			SNDRV_PCM_FMTBIT_S24_LE | \
			SNDRV_PCM_FMTBIT_S32_LE | \
			SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE)

struct snd_card *card;

static int earc_dir_probe(struct snd_soc_component *codec)
{
	int ret = 0;

	/*snd_ctl_notify will use the card.*/
	soc_card = codec->card;
	card = codec->card->snd_card;
	return ret;
}

static const struct snd_soc_component_driver earc_dir = {
	.probe = earc_dir_probe,
	.controls = dir_controls,
	.num_controls = ARRAY_SIZE(dir_controls),
	.dapm_widgets		= dir_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(dir_widgets),
	.dapm_routes		= dir_routes,
	.num_dapm_routes	= ARRAY_SIZE(dir_routes),
};

static const struct snd_soc_dai_ops earc_dai_ops = {
	.hw_params = earc_hw_params,
};

static struct snd_soc_dai_driver earc_dai = {
	.name		= "mtk-earc-rx-codec-dai",
	.capture	= {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= STUB_RATES,
		.formats	= STUB_FORMATS,
	},

	.ops = &earc_dai_ops,
};

struct clk *earcclksel;

struct dac_mute_struct {
	int mute;
	struct work_struct dac_mute_work;
	struct workqueue_struct *dac_mute_wq;
};

struct dac_mute_struct dac_mute_params;

void destroy_dac_mute_wq(void)
{
	if (dac_mute_params.dac_mute_wq) {
		destroy_workqueue(dac_mute_params.dac_mute_wq);
		dac_mute_params.dac_mute_wq = NULL;
	}
}

static int earc_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	return 0;
}
static int earc_timer_kthread(void *data)
{
	return 0;
}

void earc_hb_detect_hpd(int high)
{
	int	u32IntSta;

	if (high == TRUE) {
		/* Function:*/
		/* 048[28:17] = , */
		/* bit17 = 1: Idle;*/
		/* bit18 = 1: Common on;*/
		/* bit19 =1; Common off;*/
		/* bit20 =1: Timeout;*/
		/* bit21 = 1: EARC mode;*/
		/* bit22 =1: ARC mode; */
		u32IntSta = bReadEarc(RGS_DEBUG_FSM_3);
		if (((u32IntSta & 0x00020000) >> 17) == 0x1)
			EARC_LOG("EARC IDLE MODE!");
		else if (((u32IntSta & 0x00040000) >> 18) == 0x1)
			EARC_LOG("Comma On!");
		else if (((u32IntSta & 0x00080000) >> 19) == 0x1)
			EARC_LOG("Comma Off!");
		else if (((u32IntSta & 0x00100000) >> 20) == 0x1)
			EARC_LOG("EARC timeout!");
		else if (((u32IntSta & 0x00200000) >> 21) == 0x1)
			EARC_LOG("EARC MOde!");
		else if (((u32IntSta & 0x00400000) >> 22) == 0x1)
			EARC_LOG("ARC Mode!");

		u32IntSta = bReadEarc(RGS_DEBUG_FSM_2);
		EARC_LOG("HB 0xD0 = 0x%8x!\n", (u32IntSta >> 24));
		u32IntSta = bReadEarc(COMMA_HEARTBEAT);
		EARC_LOG("HB 0xD1 = 0x%8x!\n", (u32IntSta >> 24));

	} else if (high == FALSE) {
		EARC_LOG("hb detect hpd false!!");
		/* Function */
		/* 1, HB count =0; */
		/* 2, HB detect(Bit10) clr = 0; */
		earc_hb_count = 0;
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_detect_HB_clr_shift, int_detect_HB_clr);

		vWriteEarcMsk(COMMA_TOP_CONFIG_2,
			(0x0 << rg_soft_rstb_aud_shift), rg_soft_rstb_aud);
		vWriteEarcMsk(COMMA_TOP_CONFIG_2,
			(0x1 << rg_soft_rstb_aud_shift), rg_soft_rstb_aud);
		vWriteEarcMsk(COMMA_TOP_CONFIG_2,
			(0x0 << rg_soft_rstb_all_shift), rg_soft_rstb_all);
		vWriteEarcMsk(COMMA_TOP_CONFIG_2,
			(0xf << rg_soft_rstb_all_shift), rg_soft_rstb_all);
		if (earc_test_value == 0x10) {
			EARC_LOG("deglitch reset!!");
			vWriteEarcMsk(COMMA_TOP_CONFIG_2,
				(0x0 << rg_soft_rstb_deglitch_shift),
				rg_soft_rstb_deglitch);
			vWriteEarcMsk(COMMA_TOP_CONFIG_2,
				(0x1 << rg_soft_rstb_deglitch_shift),
				rg_soft_rstb_deglitch);
		}

		vWriteEarcMsk(COMMA_HEARTBEAT,
			rg_HB_d0_value_sel, rg_HB_d0_value_sel);
		vWriteEarcMsk(COMMA_HEARTBEAT,
			(0x18 << rg_HB_d0_value_shift), rg_HB_d0_value);
		vWriteEarcMsk(COMMA_HEARTBEAT,
			(0x0 << rg_HB_d0_value_sel_shift), rg_HB_d0_value_sel);

		vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x1 << DA_EARCRX_ARC_EN_SHIFT),
			DA_EARCRX_ARC_EN);
	}
}

void i2s2ch_enable(int enable)
{
	EARC_LOG("%s\n", __func__);
	if (enable == 1) {
		vWriteEarcMsk(COMMA_RX_AUD_CNTL,
			rg_audio_format_soft_en, rg_audio_format_soft_en);
		vWriteEarcMsk(COMMA_RX_AUD_CNTL,
			(0x1 << rg_data_268ch_en_shift), rg_data_268ch_en);
		vWriteEarcMsk(COMMA_RX_AUD_CNTL,
			(0x4 << bck_config_shift), bck_config);
		vWriteEarcMsk(COMMA_RX_AUD_CNTL,
			(0x4 << mclk_config_shift), mclk_config);
	} else if (enable == 0) {
		vWriteEarcMsk(COMMA_RX_AUD_CNTL,
			(0x0 << rg_audio_format_soft_en_shift),
			rg_audio_format_soft_en);
		vWriteEarcMsk(COMMA_RX_AUD_CNTL,
			(0x0 << rg_data_268ch_en_shift), rg_data_268ch_en);
		vWriteEarcMsk(COMMA_RX_AUD_CNTL,
			(0x0 << bck_config_shift), bck_config);
		vWriteEarcMsk(COMMA_RX_AUD_CNTL,
			(0x0 << mclk_config_shift), mclk_config);
	}
}

void earc_connected(int enable)
{
	int	u32IntSta, CaliValue;

	EARC_DRV_LOG("%s\n", __func__);
	if (enable == 1) {
		earcconnet = 0x1;
		EARC_LOG("earc connect!\n");
		EARC_LOG("0x1a001064: 0x%x\n", bReadEarc(0x064));
		EARC_LOG("0x11f30064: 0x%x\n", bReadAnaEarc(EARC_RX_CFG_2));
		EARC_LOG("0x11f30060: 0x%x\n", bReadAnaEarc(EARC_RX_CFG_1));

		EARC_LOG("set 0x11F30064=SW Calibration Value!!!!!\n");
		u32IntSta = bReadEarc(COMMA_RX_AUD_CNTL1);
		EARC_LOG("u32IntSta: 0x%x\n", u32IntSta);
		vWriteAnaEarcMsk(EARC_RX_CFG_2,
			(0x1 << RG_FORCE_CALIBRATEION_SHIFT),
			RG_FORCE_CALIBRATEION);
		CaliValue = (u32IntSta >> 26) & 0x1F;
		EARC_LOG("CaliValue: 0x%x\n", CaliValue);
		vWriteAnaEarcMsk(EARC_RX_CFG_2,
			(CaliValue << RG_EARCRX_REV_VALUE_SHIFT),
			RG_EARCRX_REV_VALUE);
		EARC_LOG("force SWCali 0x1a001064: 0x%x\n", bReadEarc(0x064));
		EARC_LOG("force SWCali 0x11f30064: 0x%x\n",
			bReadAnaEarc(EARC_RX_CFG_2));

	} else if (enable == 0) {
		earcconnet = 0x0;
		EARC_LOG("earc disconnect!\n");
		rstcompressedB = 0;
		EARC_LOG("L: %d rstcompressedB = %d\n",
		__LINE__, rstcompressedB);
	}
	info_change = 1;
}

void earc_audio_change(void)
{
	EARC_LOG("%s\n", __func__);

	info_change = 1;
}

void earc_audio_reset(void)
{
	if (oldaudiofmt != audiofmt) {
		EARC_DRV_LOG("audiofmt = 0x%x->0x%x\n", oldaudiofmt, audiofmt);
		hbdetdone = 1;
		oldaudiofmt = audiofmt;
		EARC_LOG("audio format reset flag!\n");
	}
}
void earc_audio_mute(int enable)
{
	EARC_LOG("%s enable=%d; premute=%d\n",
		__func__, enable, earcmute);

	if (enable != earcmute) {
		if (enable == 1) {
			info_change = 1;
			earcmute = 0x1;
			/*set_dac_mute(1);*/
			EARC_LOG("earc audio mute Notify!\n");
		} else if (enable == 0) {
			info_change = 1;
			earcmute = 0x0;
			/*set_dac_mute(0);*/
			EARC_LOG("earc audio unmute Notify!\n");
		} else
			EARC_LOG("earc mute change but not Notify!\n");
	}
}

static struct snd_kcontrol *snd_ctl_find_name(struct snd_card *card,
	unsigned char *name)
{
	struct snd_kcontrol *kctl;

	if (!card || !name) {
		pr_info("%s null handle\n", __func__);
		return NULL;
	}

	list_for_each_entry(kctl, &card->controls, list) {
		if (!strncmp(kctl->id.name, name, sizeof(kctl->id.name)))
			return kctl;
	}

	return NULL;
}

int mt8532_earc_notify(struct snd_card *card,
	unsigned char *ctl_name, unsigned int mask)
{

	struct snd_kcontrol *kctl;

	kctl = snd_ctl_find_name(card, ctl_name);
	if (!kctl) {
		pr_info("%s can not find ctl %s\n", __func__, ctl_name);
		return -1;
	}
	pr_info("%s ctl %s notify\n", __func__, ctl_name);
	snd_ctl_notify(card, mask, &kctl->id);

	return 0;
}

void earc_poll_isr(struct timer_list *n)
{
	int	u32IntSta;
	bool b_hpd_in;
	bool EarcValid;
	int /*u32AudFormat, */u32ChanStatus78;

	if (earc_test_value == 0xEEEE)
		return;

	if (earc_poweron == 0) {
		EARC_LOG("earc_poweron == 0!\n");
		return;
	}

	u32IntSta = bReadEarc(COMMA_RX_INT_READ);
	/*EARC_LOG("u32IntSta = 0x%8x!\n", u32IntSta);*/

#if 0
	if (u32IntSta & 0x1000000) {
		/* bit24 audio format change interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_aud_fmt_chg_clr_shift, int_aud_fmt_chg_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_aud_fmt_chg_clr_shift, int_aud_fmt_chg_clr);

		/*read audio format and notify*/
		u32AudFormat = (bReadEarc(COMMA_AUDIO_READ)&0xFF00)>>8;
		EARC_LOG("audio format change! 0x%x\n", u32AudFormat);
		Earc_Audio_Format();
	}
#endif
	if (u32IntSta & 0x2000000) {
		/*bit25 audio channel status/user bits change interrupt enable*/
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_aud_msg_chg_clr_shift, int_aud_msg_chg_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_aud_msg_chg_clr_shift, int_aud_msg_chg_clr);

		EARC_LOG("audio channel status start!\n");
		info_change = 1;
		earc_audio_mute(1);

		stablecnt = 0;
		Earc_Channel_Status();
		/*Earc_Audio_Rst();*/
		Earc_Audio_information();
		Earc_mute_audio();
		/*Earc_pll_meter();*/

		u32ChanStatus78 = bReadEarc(AUDIO_RX_CHAN_LEFT_5);
		if (u32ChanStatus78 & 0x40000) {
			csmute = 1;
			hbdetdone = 1;
			EARC_LOG("channel mute reset flag!\n");
			vWriteEarcMsk(COMMA_RX_AUD_CNTL,
				(0x1 << rg_mute_soft_shift), rg_mute_soft);
			EARC_LOG("set earc audio mute!\n");
			earc_audio_mute(1);
		} else {
			csmute = 0;
		}

		EARC_DRV_LOG("audiofmt: %d; channo: %d; codectype: %d, freq: %d\n",
			audiofmt, channelno, codectype, samplefreq);
		/*Earc_statble_reg();*/
		if (info_change) {
			mt8532_earc_notify(card, "Earc_Info",
					SNDRV_CTL_EVENT_MASK_VALUE);
			info_change = 0;
		}

		EARC_LOG("audio channel status end!\n");

		/*read audio channel status and user bits then log.*/
	}

	if (u32IntSta & 0x01) {
		/* bit0 physical HPD raising interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_hpd_raise_cap_clr_shift, int_hpd_raise_cap_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_hpd_raise_cap_clr_shift,
			int_hpd_raise_cap_clr);

		hpdstate = 1;
		info_change = 1;
		EARC_LOG("HPD is high + notify!\n");
		if (earc_poweron == 2) {
			EARC_LOG("Force ARC Mode!\n");
			ARC_RX_Enable(1);
			earc_connected(0);
		} else {
			ARC_RX_Enable(0);
			eARC_RX_CM_Enable(1);
		}
	}
	if (u32IntSta & 0x02) {
		/* bit1 physical HPD falling interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		(1 << int_hpd_fall_cap_clr_shift), int_hpd_fall_cap_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			(0 << int_hpd_fall_cap_clr_shift),
		int_hpd_fall_cap_clr);

		csmute = 1;

		vWriteEarcMsk(COMMA_RX_INT_CLR,
			1 << int_aud_msg_chg_clr_shift, int_aud_msg_chg_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			1 << int_aud_fmt_chg_clr_shift, int_aud_fmt_chg_clr);
		EARC_LOG("aduio format & channel status disable/////////\n");

		hpdstate = 0;
		info_change = 1;
		EARC_LOG("HPD is low + notify!\n");
		earc_hb_detect_hpd(FALSE);
		earc_connected(0);

		/*reset EARC RX and change to IDLE*/

		/*EARC audio disconnect*/
		/*earc_audio_disconnected();*/

	}
	if (u32IntSta & 0x04) {
		/* bit2 got packet in comma on periode interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_comma_on_running_cmd_clr_shift,
		int_comma_on_running_cmd_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_comma_on_running_cmd_clr_shift,
			int_comma_on_running_cmd_clr);

		EARC_LOG("got packet in comma on periode!\n");

	}
	if (u32IntSta & 0x08) {
		/* bit3 audio ecc error interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_ecc_err_clr_shift, int_ecc_err_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_ecc_err_clr_shift, int_ecc_err_clr);

		EARC_POLL_LOG("audio ecc error!\n");

	}
	if (u32IntSta & 0x10) {
		/* bit4 audio ecc fix interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_ecc_fix_clr_shift, int_ecc_fix_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_ecc_fix_clr_shift, int_ecc_fix_clr);

		EARC_POLL_LOG("audio ecc fix!\n");

	}

	if (u32IntSta & 0x20) {
		/* bit5 write sequence into Nack interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_write_noack_clr_shift, int_write_noack_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_write_noack_clr_shift, int_write_noack_clr);

		EARC_LOG("write sequence into Nack!\n");

	}
	if (u32IntSta & 0x40) {
		/* bit6 read sequence into Nack interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_read_noack_clr_shift, int_read_noack_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_read_noack_clr_shift, int_read_noack_clr);

		EARC_LOG("read sequence into Nack!\n");

	}
	if (u32IntSta & 0x80) {
		/* bit7 internal HPD low interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_hpd_phy_low_clr_shift, int_hpd_phy_low_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_hpd_phy_low_clr_shift, int_hpd_phy_low_clr);

		EARC_LOG("internal HPD low!\n");

	}
	if (u32IntSta & 0x100) {
		/* bit8 total timeout interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		(1 << int_timeout_clr_shift), int_timeout_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			(0 << int_timeout_clr_shift), int_timeout_clr);

		ARC_RX_Enable(1);
		earc_connected(0);
		EARC_LOG("250ms total timeout!\n");

	}
	if (u32IntSta & 0x200) {
		/* bit9 HB timeout interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_HB_timeout_clr_shift, int_HB_timeout_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_HB_timeout_clr_shift, int_HB_timeout_clr);

		eARC_RX_update_cap();
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_detect_HB_clr_shift, int_detect_HB_clr);

		EARC_LOG("HB timeout!\n");

	}
	if (u32IntSta & 0x400) {
		/* bit10 detect HB interrupt enable, for debug */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_detect_HB_clr_shift, int_detect_HB_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_detect_HB_clr_shift, int_detect_HB_clr);

		/* 1, count = 2, clr =1; */
		/* 2, Count = 0/1, log EARC mode */
		earc_hb_count++;
		if (earc_hb_count == 1) {
			caldone = 1;
			hbdetdone = 1;
			EARC_LOG("rg 0x1a001060: 0x%x\n",
				bReadEarc(EARC_RX_CFG_1));
			EARC_LOG("rg 0x1a001064: 0x%x\n", bReadEarc(0x064));
		}
		if (earc_hb_count == 3) {
			vWriteEarcMsk(COMMA_RX_INT_CLR,
				0 << int_aud_msg_chg_clr_shift,
				int_aud_msg_chg_clr);
			vWriteEarcMsk(COMMA_RX_INT_CLR,
				0 << int_aud_fmt_chg_clr_shift,
				int_aud_fmt_chg_clr);
			EARC_LOG("aduio format & channel status enable////\n");
			vWriteEarcMsk(COMMA_TOP_CONFIG_2,
				(0x0 << rg_soft_rstb_aud_shift),
				rg_soft_rstb_aud);
			vWriteEarcMsk(COMMA_TOP_CONFIG_2,
				(0x1 << rg_soft_rstb_aud_shift),
				rg_soft_rstb_aud);
			EARC_LOG("reset audio!\n");
		}
		EARC_LOG("count = %d\n", earc_hb_count);
		if (earc_hb_count >= 5) {
			vWriteEarcMsk(COMMA_RX_INT_CLR,
			1 << int_detect_HB_clr_shift, int_detect_HB_clr);
			earc_connected(1);
		} else
			earc_hb_detect_hpd(TRUE);
		EARC_LOG("detect HB enable! count = %d\n", earc_hb_count);

	}
	if (u32IntSta & 0x800) {
		/* bit11 fsm abort interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_abort_clr_shift, int_abort_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_abort_clr_shift, int_abort_clr);

		EARC_LOG("fsm abort interrupt enable\n");

	}
	if (u32IntSta & 0x1000) {
		/* bit12 WRITE sequence finish interrupt enable, for debug */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_write_finish_clr_shift, int_write_finish_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_write_finish_clr_shift, int_write_finish_clr);

		/*EARC_LOG("W");*/

	}
	if (u32IntSta & 0x2000) {
		/* bit13 READ sequence finish interrupt enable , for debug */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_read_finish_clr_shift, int_read_finish_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_read_finish_clr_shift, int_read_finish_clr);

		/*EARC_LOG("R");*/

	}
	if (u32IntSta & 0x4000) {
		/* bit14 receive packet done interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_rscv_ok_clr_shift, int_rscv_ok_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_rscv_ok_clr_shift, int_rscv_ok_clr);

		/*EARC_LOG("receive packet done interrupt!\n");*/

	}
	if (u32IntSta & 0x8000) {
		/* bit15 send packet done interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_send_ok_clr_shift, int_send_ok_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_send_ok_clr_shift, int_send_ok_clr);

		EARC_LOG("send packet done interrupt!\n");

	}
	if (u32IntSta & 0x20000) {
		/* bit17 CM receive packet abort interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_rscv_abort_clr_shift, int_rscv_abort_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_rscv_abort_clr_shift, int_rscv_abort_clr);

		EARC_LOG("CM receive packet abort!\n");

	}
	if (u32IntSta & 0x40000) {
		/* bit18 CM packet ecc error interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_err_ecc_clr_shift, int_err_ecc_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_err_ecc_clr_shift, int_err_ecc_clr);

		EARC_LOG("CM packet ecc error!\n");

	}
	if (u32IntSta & 0x80000) {
		/*bit19 ealy hb detect, deponds on the interrupt enable*/
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_early_hb_clr_shift, int_early_hb_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_early_hb_clr_shift, int_early_hb_clr);

		EARC_LOG("ealy hb detect!\n");

	}
	if (u32IntSta & 0x100000) {
		/* bit20 EARC_VALID change interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		(1 << int_EARC_VALID_clr_shift), int_EARC_VALID_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			(0 << int_EARC_VALID_clr_shift), int_EARC_VALID_clr);

		EARC_LOG("EARC_VALID change!\n");
		EarcValid = (bReadEarc(0x030)&0x80000000)>>31;
		if (EarcValid == 1) {
		/*reset earc rx*/
		/*change to idle*/
		}

	}
	if (u32IntSta & 0x200000) {
		/* bit21 HDMI_HPD change interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_HDMI_HPD_clr_shift, int_HDMI_HPD_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_HDMI_HPD_clr_shift, int_HDMI_HPD_clr);


		EARC_LOG("HDMI_HPD change!\n");

		/*read HDMI_HPD from register*/
		/*set HPD to HW*/
		b_hpd_in = (bReadEarc(0x030)&0x01000000)>>24;
		if (b_hpd_in == 1)
			vWriteEarcMsk(0x064, 0x1<<15, 0x1<<15);
		else
			vWriteEarcMsk(0x064, 0x0<<15, 0x1<<15);

	}
	if (u32IntSta & 0x400000) {
		/* bit22 write full in memory interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_write_full_clr_shift, int_write_full_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_write_full_clr_shift, int_write_full_clr);

		EARC_LOG("capability write full in memory!\n");

	}
	if (u32IntSta & 0x800000) {
		/* bit23 read address equal to 8'hff interrupt enable */
		EARC_LOG("capability read address empty! 1\n");
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			1 << int_read_empty_clr_shift, int_read_empty_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_read_empty_clr_shift, int_read_empty_clr);

		EARC_LOG("capability read address empty!\n");

	}

	if (u32IntSta & 0x4000000) {
		/* bit26 EXT_LATENCY_REQ Change interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_exr_latency_req_clr_shift, int_exr_latency_req_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_exr_latency_req_clr_shift,
			int_exr_latency_req_clr);

		EARC_LOG("EXT_LATENCY_REQ Change!\n");

	}

	Earc_valid_signal();

	if ((csmute == 0) && (caldone == 1) && (stableunmute == 1) && (audfmt_notify == 0)) {
		csmute = 1;
		if (hbdetdone == 1) {
			hbdetdone = 0;
			vWriteEarcMsk(COMMA_TOP_CONFIG_2,
				(0x0 << rg_soft_rstb_aud_shift),
				rg_soft_rstb_aud);
			vWriteEarcMsk(COMMA_TOP_CONFIG_2,
				(0x1 << rg_soft_rstb_aud_shift),
				rg_soft_rstb_aud);
			EARC_LOG("reset audio!\n");
		} else {
			vWriteEarcMsk(COMMA_RX_AUD_CNTL,
				(0x0 << rg_mute_soft_shift), rg_mute_soft);
			EARC_LOG("set earc audio unmute!\n");
			earc_audio_mute(0);
		}
	}

	if (info_change)
		mt8532_earc_notify(card, "Earc_Info",
				SNDRV_CTL_EVENT_MASK_VALUE);
	info_change = 0;
	audfmt_notify = 0;

	atomic_set(&earc_timer_event, 1);
	wake_up_interruptible(&earc_timer_wq);
	mod_timer(&earc_timer, jiffies + gEARC_CHK_INTERVAL / (1000 / HZ));

}

int Earc_StartTimer(void)
{
	EARC_LOG("%s\n", __func__);

	init_waitqueue_head(&earc_timer_wq);
	earc_timer_task = kthread_create(earc_timer_kthread,
		NULL, "earc_timer_kthread");
	wake_up_process(earc_timer_task);

	EARC_LOG("cmd earc Earc_StartTimer1\n");

	memset((void *)&earc_timer, 0, sizeof(earc_timer));
	earc_timer.expires = jiffies + 1 / (10000 / HZ);
	earc_timer.function = earc_poll_isr;
	//earc_timer.data = 0;
	//init_timer(&earc_timer);
	add_timer(&earc_timer);

	return 0;
}

void Earc_Mode_state(void)
{
	EARC_LOG("Earc Mode reg: 0x%x\n", bReadEarc(RGS_DEBUG_FSM_3));

	if (((bReadEarc(RGS_DEBUG_FSM_3) & 0x00020000) >> 17) == 0x1) {
		if (earc_poweron == 2)
			EARC_LOG("Force ARC Mode!\n");
		else
			EARC_LOG("EARC IDLE MODE!");
	} else if (((bReadEarc(RGS_DEBUG_FSM_3) & 0x00040000) >> 18) == 0x1)
		EARC_LOG("Comma On!");
	else if (((bReadEarc(RGS_DEBUG_FSM_3) & 0x00080000) >> 19) == 0x1)
		EARC_LOG("Comma Off!");
	else if (((bReadEarc(RGS_DEBUG_FSM_3) & 0x00100000) >> 20) == 0x1)
		EARC_LOG("EARC timeout!");
	else if (((bReadEarc(RGS_DEBUG_FSM_3) & 0x00200000) >> 21) == 0x1)
		EARC_LOG("EARC MOde!");
	else if (((bReadEarc(RGS_DEBUG_FSM_3) & 0x00400000) >> 22) == 0x1)
		EARC_LOG("ARC Mode!");
}

void Earc_Audio_Format(void)
{
	int u32AudFormat;

	EARC_LOG("audio format reg: 0x%x\n", bReadEarc(COMMA_AUDIO_READ));
	u32AudFormat = (bReadEarc(COMMA_AUDIO_READ)&0xFF00)>>8;

	if (u32AudFormat == 0x01) {
		/*PCM_2;*/
		EARC_LOG("PCM_2!\n");
		audiofmt = PCM_2;
	} else if (u32AudFormat == 0x02) {
		/*PCM_8;*/
		EARC_LOG("PCM_8!\n");
		audiofmt = PCM_8;
	} else if (u32AudFormat == 0x04) {
		/*PCM_16;*/
		EARC_LOG("PCM_16!\n");
		audiofmt = PCM_16;
	} else if (u32AudFormat == 0x08) {
		/*PCM_32;*/
		EARC_LOG("PCM_32!\n");
		audiofmt = PCM_32;
	} else if (u32AudFormat == 0x10) {
		/*CMPS_a;*/
		EARC_LOG("CMPS_a!\n");
		audiofmt = CMPS_a;
	} else if (u32AudFormat == 0x20) {
		/*CMPS_b;*/
		EARC_LOG("CMPS_b!\n");
		audiofmt = CMPS_b;
	} else if (u32AudFormat == 0x40) {
		/*DSD_6;*/
		EARC_LOG("DSD_6!\n");
		audiofmt = DSD_6;
	} else if (u32AudFormat == 0x80) {
		/*DSD_12;*/
		EARC_LOG("DSD_12!\n");
		audiofmt = DSD_12;
	}

	EARC_LOG("AC 0x1a001064: 0x%x\n", bReadEarc(0x064));
	EARC_LOG("AC 0x11f30064: 0x%x\n", bReadAnaEarc(EARC_RX_CFG_2));

	/*if (u32AudFormat)*/
		earc_audio_change();
}

void Earc_Audio_Status(void)
{
	int u32AudFormat;

	EARC_LOG("audio format reg: 0x%x\n", bReadEarc(COMMA_AUDIO_READ));
	EARC_LOG("earcconnet: %d\n", earcconnet);
	u32AudFormat = (bReadEarc(COMMA_AUDIO_READ) & 0xFF00) >> 8;

	if (u32AudFormat == 0x01) {
		/*PCM_2;*/
		EARC_LOG("PCM_2!\n");
		audiofmt = PCM_2;
	} else if (u32AudFormat == 0x02) {
		/*PCM_8;*/
		EARC_LOG("PCM_8!\n");
		audiofmt = PCM_8;
	} else if (u32AudFormat == 0x04) {
		/*PCM_16;*/
		EARC_LOG("PCM_16!\n");
		audiofmt = PCM_16;
	} else if (u32AudFormat == 0x08) {
		/*PCM_32;*/
		EARC_LOG("PCM_32!\n");
		audiofmt = PCM_32;
	} else if (u32AudFormat == 0x10) {
		/*CMPS_a;*/
		EARC_LOG("CMPS_a!\n");
		audiofmt = CMPS_a;
	} else if (u32AudFormat == 0x20) {
		/*CMPS_b;*/
		EARC_LOG("CMPS_b!\n");
		audiofmt = CMPS_b;
	} else if (u32AudFormat == 0x40) {
		/*DSD_6;*/
		EARC_LOG("DSD_6!\n");
		audiofmt = DSD_6;
	} else if (u32AudFormat == 0x80) {
		/*DSD_12;*/
		EARC_LOG("DSD_12!\n");
		audiofmt = DSD_12;
	}
	EARC_LOG("earcmute: %d\n", earcmute);
	EARC_LOG("audiofmt: %d\n", audiofmt);
	EARC_LOG("samplefreq: %d\n", samplefreq);
	EARC_LOG("codectype: %d\n", codectype);
	EARC_LOG("channelca: %d\n", channelca);
	EARC_LOG("hpdstate: %d\n", hpdstate);
	EARC_LOG("channelno: %d\n", channelno);
	EARC_LOG("onebitado: %d\n", onebitado);
	EARC_LOG("rstcompressedB: %d\n", rstcompressedB);
}

void Earc_Audio_information(void)
{
	if (audiofmt == CMPS_a) {
		if ((samplefreq == 768) || (samplefreq == 705)) {
			i2s2ch_enable(0);
			codectype = 3;/*hbr*/
			channelno = 8;
		} else {
			i2s2ch_enable(1);
			codectype = 2;/*ac3*/
			channelno = 2;
		}
		rstcompressedB = 0;
		EARC_LOG("L: %d rstcompressedB = %d\n",
			__LINE__, rstcompressedB);
	} else if ((audiofmt == PCM_2) || (audiofmt == PCM_8)) {
		i2s2ch_enable(0);
		codectype = 1;/*pcm*/
		if (audiofmt == PCM_2)
			channelno = 2;
		else if (audiofmt == PCM_8) {
			channelno = 8;
			if ((samplefreq == 768) || (samplefreq == 384) ||
			(samplefreq == 192))
				samplefreq = samplefreq/4;
		} else
			channelno = 2;
		rstcompressedB = 0;
		EARC_POLL_LOG("L: %d rstcompressedB = %d\n",
			__LINE__, rstcompressedB);
	} else if (audiofmt == CMPS_b) {
		i2s2ch_enable(0);
		codectype = 2;/*ac3*/
		channelno = 2;
		EARC_LOG("L: %d samplefreq = %d\n",
			__LINE__, samplefreq);
		samplefreq = samplefreq/4;
		EARC_LOG("L: %d samplefreq = %d\n",
			__LINE__, samplefreq);
		if (rstcompressedB == 0) {
			vWriteEarcMsk(COMMA_TOP_CONFIG_2,
				0 << rg_soft_rstb_aud_shift,
				rg_soft_rstb_aud);
			vWriteEarcMsk(COMMA_TOP_CONFIG_2,
				1 << rg_soft_rstb_aud_shift,
				rg_soft_rstb_aud);
			rstcompressedB = 1;
			EARC_LOG("L: %d rstcompressedB = %d\n",
				__LINE__, rstcompressedB);
		}
		EARC_LOG("L: %d rstcompressedB = %d\n",
			__LINE__, rstcompressedB);
	} else {
		rstcompressedB = 0;
		EARC_LOG("L: %d rstcompressedB = %d\n",
			__LINE__, rstcompressedB);
		channelno = 0xff;
		EARC_LOG("invalid audiofmt: %d; channo: %d!\n",
			audiofmt, channelno);
	}
}

void Earc_valid_signal(void)
{
	int freqValue;
	int	u32ChsL1, rxchstfs;

	/*EARC_LOG("%s\n", __func__);*/

	vWriteEarcMsk(COMMA_FREQ_METER, freq_meter_enable, freq_meter_enable);
	vWriteEarcMsk(COMMA_PULSE_CNTL, begin_cal, begin_cal);
	HAL_Delay_us(50);
	freqValue = (bReadEarc(COMMA_FREQ_METER) >> 16) & 0xFFFF;

	u32ChsL1 = bReadEarc(AUDIO_RX_CHAN_LEFT_1);
	rxchstfs = (u32ChsL1 >> 24) & 0x0F;
	EARC_POLL_LOG("rxchstfs = %d ; freqValue = 0x%x!\n",
		rxchstfs, freqValue);
	EARC_POLL_LOG("oldfs = %d ; oldfmeter = 0x%x!\n", oldfs, oldfmeter);

	if (((freqValue >= 0x4F) && (freqValue <= 0x794)) &&
		((freqValue >= (oldfmeter - 3)) &&
		(freqValue <= (oldfmeter + 3)))
		&& (rxchstfs == oldfs))
		stablecnt++;
	else
		stablecnt = 0;

	oldfmeter = freqValue;
	oldfs = rxchstfs;
	if (stablecnt >= 5)
		stableunmute = 1;
	else
		stableunmute = 0;
	EARC_POLL_LOG("stablecnt = %d ; stableunmute = 0x%x!\n",
		stablecnt, stableunmute);
}

void Earc_mute_audio(void)
{
	if ((bReadEarc(AUDIO_RX_USER_1) == 0) &&
		(bReadEarc(AUDIO_RX_USER_2) == 0) &&
		(bReadEarc(AUDIO_RX_USER_3) == 0) &&
		(bReadEarc(AUDIO_RX_USER_4) == 0) &&
		(bReadEarc(AUDIO_RX_USER_5) == 0) &&
		(bReadEarc(AUDIO_RX_USER_6) == 0) &&
		(bReadEarc(AUDIO_RX_USER_7) == 0) &&
		(bReadEarc(AUDIO_RX_USER_8) == 0) &&
		(bReadEarc(AUDIO_RX_USER_9) == 0) &&
		(bReadEarc(AUDIO_RX_USER_10) == 0) &&
		(bReadEarc(AUDIO_RX_USER_11) == 0) &&
		(bReadEarc(AUDIO_RX_USER_12) == 0)) {
		cschange = 1;
		EARC_LOG("channel status change valid!\n");
	} else
		cschange = 0;
}

void Earc_Channel_Status(void)
{
	int	u32ChsL1, u32ChsL2, u32ChsL5;
	int u32ChanStatus, rxchstfs, u32OneBitAdo, u32ChanNo, u32ChanCa;
	int tmp = 0;

	EARC_LOG("%s\n", __func__);

	u32ChsL1 = bReadEarc(AUDIO_RX_CHAN_LEFT_1);
	u32ChanStatus = ((u32ChsL1 & 0x38) >> 1) | (u32ChsL1 & 0x03);
	if (u32ChanStatus == UN2CHLPCM)
		EARC_LOG("Un-enc 2Ch LPCM");
	else if (u32ChanStatus == UNMCHLPCM)
		EARC_LOG("Un-enc Multi-Ch LPCM");
	else if (u32ChanStatus == UNXCHDSD)
		EARC_LOG("Un-enc X-Ch DSD(1-bit audio)");
	else if (u32ChanStatus == UN2CHNLPCM)
		EARC_LOG("Un-enc 2Ch NLPCM");
	else if (u32ChanStatus == EN2CHNLPCM)
		EARC_LOG("Enc 2Ch NLPCM");
	else if (u32ChanStatus == ENMCHNLPCM)
		EARC_LOG("Enc Multi-Ch NLPCM");
	else if (u32ChanStatus == ENXCHDSD)
		EARC_LOG("Enc X-Ch DSD(1-bit audio)");
	else
		EARC_LOG("Err");

	rxchstfs = (u32ChsL1 >> 24) & 0x3F;
	switch (rxchstfs) {
	case AUD32K:
		tmp = 32;
		break;
	case AUD44K:
		tmp = 44;
		break;
	case AUD48K:
		tmp = 48;
		break;
	case AUD64K:
		tmp = 64;
		break;
	case AUD88K:
		tmp = 88;
		break;
	case AUD96K:
		tmp = 96;
		break;
	case AUD128K:
		tmp = 128;
		break;
	case AUD176K:
		tmp = 176;
		break;
	case AUD192K:
		tmp = 192;
		break;
	case AUD256K:
		tmp = 256;
		break;
	case AUD352K:
		tmp = 352;
		break;
	case AUD384K:
		tmp = 384;
		break;
	case AUD512K:
		tmp = 512;
		break;
	case AUD705K:
		tmp = 705;
		break;
	case AUD768K:
		tmp = 768;
		break;
	case AUD1024K:
		tmp = 1024;
		break;
	case AUD1411K:
		tmp = 1411;
		break;
	case AUD1536K:
		tmp = 1536;
		break;
	default:
		break;
	}
	samplefreq = tmp;
	EARC_LOG("ChStFS %dK\r\n", tmp);

	u32ChsL2 = bReadEarc(AUDIO_RX_CHAN_LEFT_2);
	u32OneBitAdo = u32ChsL2 & 0xF;
	if ((u32OneBitAdo == 0) &&
		((u32ChanStatus == UNXCHDSD) || (u32ChanStatus == ENXCHDSD))) {
		EARC_LOG("One Bit Audio\n");
		onebitado = 1;
	} else {
		EARC_LOG("No One Bit Audio\n");
		onebitado = 0;
	}

	u32ChanNo = (u32ChsL2 >> 12) & 0x0F;
	if ((u32ChanStatus == UNMCHLPCM) || (u32ChanStatus == ENMCHNLPCM)) {
		switch (u32ChanNo) {
		case ADO2CHPCM:
			tmp = 2;
			break;
		case ADO8CHPCM:
			tmp = 8;
			break;
		case ADO16CHPCM:
			tmp = 16;
			break;
		case ADO32CHPCM:
			tmp = 32;
			break;
		default:
			break;
		}
		EARC_LOG("%dCH PCM\n", tmp);
	} else if (onebitado == 1) {
		switch (u32ChanNo) {
		case ADO6CHDSD:
			tmp = 6;
			break;
		case ADO12CHDSD:
			tmp = 12;
			break;
		default:
			break;
		}
		EARC_LOG("%dCH DSD\n", tmp);
	} else if ((u32ChanStatus == UN2CHNLPCM) ||
	(u32ChanStatus == EN2CHNLPCM)) {
		switch (u32ChanNo) {
		case CompLayoutA:
			tmp = 0xA;
			break;
		case CompLayoutB:
			tmp = 0xB;
			break;
		default:
			break;
		}
		EARC_LOG("Compressed Layout %X\n", tmp);
	}

	u32ChsL5 = bReadEarc(AUDIO_RX_CHAN_LEFT_5);
	u32ChanCa = (u32ChsL5 >> 8) & 0xFF;
	channelca = u32ChanCa;
	EARC_LOG("valid channelca 0x%X\n", channelca);
	if ((audfmt_notify == 0) &&
		((oldsamplefreq != samplefreq) || (oldchannelca != channelca))) {
		earc_audio_change();
		EARC_LOG("CS change! freq:0x%x->0x%x; ca:0x%x->0x%x\n",
			oldsamplefreq, samplefreq, oldchannelca, channelca);
		oldsamplefreq = samplefreq;
		oldchannelca = channelca;
	}

	EARC_LOG("CS 0x1a001064: 0x%x\n", bReadEarc(0x064));
	EARC_LOG("CS 0x11f30064: 0x%x\n", bReadAnaEarc(EARC_RX_CFG_2));

}

void Earc_statble_reg(void)
{
	EARC_LOG("%s\n", __func__);

	EARC_LOG("0x11f30060: 0x%x\n", bReadAnaEarc(EARC_RX_CFG_1));
	EARC_LOG("0x11f30064: 0x%x\n", bReadAnaEarc(EARC_RX_CFG_2));

	EARC_LOG("0x1a001000: 0x%x\n", bReadEarc(COMMA_TIMER_SND_PKT_1));
	EARC_LOG("0x1a001004: 0x%x\n", bReadEarc(COMMA_TIMER_SND_PKT_2));
	EARC_LOG("0x1a001008: 0x%x\n", bReadEarc(COMMA_SND_SOFT));
	EARC_LOG("0x1a00100C: 0x%x\n", bReadEarc(0x00c));
	EARC_LOG("0x1a001010: 0x%x\n", bReadEarc(COMMA_TIMER_RCV_PKT_1));
	EARC_LOG("0x1a001014: 0x%x\n", bReadEarc(COMMA_TIMER_RCV_PKT_2));
	EARC_LOG("0x1a001018: 0x%x\n", bReadEarc(COMMA_TIMER_DISC_1));
	EARC_LOG("0x1a00101C: 0x%x\n", bReadEarc(COMMA_TIMER_DISC_2));

	EARC_LOG("0x1a001060: 0x%x\n", bReadEarc(COMMA_RX_AUD_CNTL));
	EARC_LOG("0x1a001064: 0x%x\n", bReadEarc(0x064));
	EARC_LOG("0x1a001068: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_LEFT_1));
	EARC_LOG("0x1a00106C: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_LEFT_2));
	EARC_LOG("0x1a001070: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_LEFT_3));
	EARC_LOG("0x1a001074: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_LEFT_4));
	EARC_LOG("0x1a001078: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_LEFT_5));
	EARC_LOG("0x1a00107C: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_LEFT_6));

	EARC_LOG("0x1a001080: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_RIGHT_1));
	EARC_LOG("0x1a001084: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_RIGHT_2));
	EARC_LOG("0x1a001088: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_RIGHT_3));
	EARC_LOG("0x1a00108C: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_RIGHT_4));
	EARC_LOG("0x1a001090: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_RIGHT_5));
	EARC_LOG("0x1a001094: 0x%x\n", bReadEarc(AUDIO_RX_CHAN_RIGHT_6));

	EARC_LOG("0x1a001098: 0x%x\n", bReadEarc(AUDIO_RX_USER_1));
	EARC_LOG("0x1a00109C: 0x%x\n", bReadEarc(AUDIO_RX_USER_2));
	EARC_LOG("0x1a0010A0: 0x%x\n", bReadEarc(AUDIO_RX_USER_3));
	EARC_LOG("0x1a0010A4: 0x%x\n", bReadEarc(AUDIO_RX_USER_4));
	EARC_LOG("0x1a0010A8: 0x%x\n", bReadEarc(AUDIO_RX_USER_5));
	EARC_LOG("0x1a0010AC: 0x%x\n", bReadEarc(AUDIO_RX_USER_6));
}

void Earc_Status(void)
{
	Earc_Mode_state();
	Earc_Audio_Status();
	Earc_statble_reg();
}

void eARC_RX_update_cap(void)
{
	EARC_LOG("%s\n", __func__);
	vWriteEarcMsk(COMMA_HEARTBEAT, rg_HB_d0_value_sel, rg_HB_d0_value_sel);
	vWriteEarcMsk(COMMA_HEARTBEAT,
		(0x18 << rg_HB_d0_value_shift), rg_HB_d0_value);
	vWriteEarcMsk(COMMA_HEARTBEAT,
		(0x0 << rg_HB_d0_value_sel_shift), rg_HB_d0_value_sel);
}

void eARC_RX_Write_Audiocap(char const *ptr)
{
	int i;

	EARC_LOG("%s\n", __func__);
	vWriteEarcMsk(COMMA_CAPA_ADDR,
		(0x00 << capa_address_shift), capa_address);

	for (i = 0; i < 256; i++) {
		/*vWriteEarcMsk(COMMA_CAPA_DATA, */
		/*(*ptr << capa_data_ff_shift), capa_data_ff);*/
		vWriteEarc(COMMA_CAPA_DATA, *ptr);
		/*EARC_LOG("[W][%d] 0x%x\n", i, *ptr);*/
		ptr++;
	}

	vWriteEarcMsk(COMMA_HEARTBEAT, rg_HB_d0_value_sel, rg_HB_d0_value_sel);
	vWriteEarcMsk(COMMA_HEARTBEAT,
		(0x18 << rg_HB_d0_value_shift), rg_HB_d0_value);
	vWriteEarcMsk(COMMA_HEARTBEAT,
		(0x0 << rg_HB_d0_value_sel_shift), rg_HB_d0_value_sel);
}

void Write_Audiocap(void)
{
	EARC_LOG("%s\n", __func__);
	if (earc_test_value == 1) {
		//eARC_RX_Write_Audiocap(DefaultCapData32);
		EARC_LOG("write capability same as MT8531\n");
	} else if (earc_test_value == 0) {
		eARC_RX_Write_Audiocap(DefaultCapData32255);
		EARC_LOG("write capability same as MT8531 and 000\n");
	} else if (earc_test_value == 3) {
		eARC_RX_Write_Audiocap(DefaultCapData255);
		EARC_LOG("write capability same as MT8532 no 870\n");
	} else if (earc_test_value == 4) {
		eARC_RX_Write_Audiocap(DefaultCapData);
		EARC_LOG("write capability same as MT8532 default\n");
	}
}

void eARC_RX_Read_Audiocap(void)
{
	int i, capvalue;

	EARC_LOG("%s\n", __func__);
	vWriteEarcMsk(0x038, 0x00, 0xFF);

	for (i = 0; i < 255; i++) {
		bReadEarc(0x03c);
		capvalue = bReadEarc(0x038);
		EARC_LOG("[R3c1]%d 0x%8x\n", i, capvalue);
	}
}

void eARC_RX_Set_AudLatency(int LatValue)
{
	int u32IntSta;

	u32IntSta = bReadEarc(COMMA_AUDIO);
	EARC_LOG("audio latency1: 0x%x\n", u32IntSta);

	vWriteEarcMsk(COMMA_AUDIO, rg_audio_config_en_p,
		rg_audio_config_en_p);
	vWriteEarcMsk(COMMA_AUDIO, LatValue, rg_audio_config);

	u32IntSta = bReadEarc(COMMA_AUDIO);
	EARC_LOG("audio latency2: 0x%x\n", u32IntSta);
}

void eARC_analog_Init(void)
{
	EARC_LOG("%s\n", __func__);

	caldone = 0;

	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRXPLL_EN_SHIFT),
		RG_EARCRXPLL_EN);

	vWriteAnaEarcMsk(EARC_RX_CFG_0, RG_EARCRX_BIAS_EN, RG_EARCRX_BIAS_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x10 << RG_EARCRX_BIAS_INTR_CAL_SHIFT),
		RG_EARCRX_BIAS_INTR_CAL);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_DM_LDO09_SEL_SHIFT),
		RG_EARCRX_DM_LDO09_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_DM_LDO_LVROD_SHIFT),
		RG_EARCRX_DM_LDO_LVROD);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_LDO_PWD_SHIFT),
		RG_EARCRX_CM_LDO_PWD);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_LDO_LVROD_SHIFT),
		RG_EARCRX_CM_LDO_LVROD);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_REFGEN_MONEN_SHIFT),
		RG_EARCRX_REFGEN_MONEN);

	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x1 << RG_EARCRX_TERM50_EN_SHIFT),
		RG_EARCRX_TERM50_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x10 << RG_EARCRX_TERM50_SEL_SHIFT),
		RG_EARCRX_TERM50_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_VCM_TERM_SEL_SHIFT),
		RG_EARCRX_VCM_TERM_SEL);

	/*vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x0 << DA_EARCRX_ARC_EN_SHIFT),*/
	/*	DA_EARCRX_ARC_EN);*/
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x1 << RG_EARCRX_CMTX_EN_SHIFT),
		RG_EARCRX_CMTX_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_DM_LDO_MONEN_SHIFT),
		RG_EARCRX_DM_LDO_MONEN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x1 << RG_EARCRX_PWM_CDR_EN_SHIFT),
		RG_EARCRX_PWM_CDR_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0,
		0x1 << RG_EARCRX_PWM_CDR_TOBDET_EN_SHIFT,
		RG_EARCRX_PWM_CDR_TOBDET_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_PWM_GEAR_SEL_SHIFT),
		RG_EARCRX_PWM_GEAR_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_ARC_EN_SHIFT),
		RG_EARCRX_ARC_EN);
	/*vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x0 << RG_EARCRX_REV_SHIFT),*/
	/*	RG_EARCRX_REV);*/
	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x1 << RG_EARCRX_REV_SHIFT),
		RG_EARCRX_REV);

	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_VREFT_SEL_SHIFT),
		RG_EARCRX_CM_VREFT_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_REG_PWD_SHIFT),
		RG_EARCRX_CM_REG_PWD);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_LDO_MONEN_SHIFT),
		RG_EARCRX_CM_LDO_MONEN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_CM_TRAN_SEL_SHIFT),
	RG_EARCRX_CM_TRAN_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x4 << RG_EARCRX_CM_DLY_SEL_SHIFT),
		RG_EARCRX_CM_DLY_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_CM_SLEW_OSC_EN_SHIFT),
		RG_EARCRX_CM_SLEW_OSC_EN);

	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x1 << RG_EARCRX_CMRX_EN_SHIFT),
		RG_EARCRX_CMRX_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x1 << RG_EARCRX_CMRX_BUF_EN_SHIFT),
		RG_EARCRX_CMRX_BUF_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1,
		(0x0 << RG_EARCRX_CMRX_HPF_BW_CTRL_SHIFT),
		RG_EARCRX_CMRX_HPF_BW_CTRL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1,
		(0x1 << RG_EARCRX_CMRX_HYST_AMP_EN_SHIFT),
		RG_EARCRX_CMRX_HYST_AMP_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_CMRX_HYST_SEL_SHIFT),
		RG_EARCRX_CMRX_HYST_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_CM_VREF_MONEN_SHIFT),
		RG_EARCRX_CM_VREF_MONEN);
	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x0 << RG_EARCRX_VDCTST_SEL_SHIFT),
		RG_EARCRX_VDCTST_SEL);

	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x1 << DA_EARCRXPLL_SDM_PWR_ON_SHIFT),
		DA_EARCRXPLL_SDM_PWR_ON);
	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x0 << DA_EARCRXPLL_SDM_ISO_EN_SHIFT),
		DA_EARCRXPLL_SDM_ISO_EN);

	if (earc_test_value == 0x0) {
		vWriteAnaEarcMsk(EARC_RX_CFG_3, 0x4D000000,
			DA_EARCRXPLL_SDM_PCW);
	} else {
		vWriteAnaEarcMsk(EARC_RX_CFG_3, earc_test_value << 24,
			DA_EARCRXPLL_SDM_PCW);
	}
	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x1 << DA_EARCRXPLL_SDM_PCW_CHG_SHIFT),
		DA_EARCRXPLL_SDM_PCW_CHG);

	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x1 << RG_EARCRXPLL_EN_SHIFT),
		RG_EARCRXPLL_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_PREDIV_SHIFT,
		RG_EARCRXPLL_PREDIV);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_POSDIV_SHIFT,
		RG_EARCRXPLL_POSDIV);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x1 << RG_EARCRXPLL_BLP_SHIFT,
		RG_EARCRXPLL_BLP);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_DIV3_EN_SHIFT,
		RG_EARCRXPLL_DIV3_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_GLITCHFREE_EN_SHIFT,
		RG_EARCRXPLL_GLITCHFREE_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_LVROD_EN_SHIFT,
		RG_EARCRXPLL_LVROD_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_MONCK_EN_SHIFT,
		RG_EARCRXPLL_MONCK_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_MONREF_EN_SHIFT,
		RG_EARCRXPLL_MONREF_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_MONVC_EN_SHIFT,
		RG_EARCRXPLL_MONVC_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_SDM_FRA_EN_SHIFT,
		RG_EARCRXPLL_SDM_FRA_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_2, 0x0 << RG_EARCRXPLL_TSTDIV_SHIFT,
		RG_EARCRXPLL_TSTDIV);

	vWriteAnaEarcMsk(EARC_RX_CFG_0C0, RG_EARCRX_MUX,
		RG_EARCRX_MUX);
}

void ARC_analog_Init(void)
{
	EARC_LOG("%s\n", __func__);

	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRXPLL_EN_SHIFT),
		RG_EARCRXPLL_EN);

	vWriteAnaEarcMsk(EARC_RX_CFG_0, RG_EARCRX_BIAS_EN, RG_EARCRX_BIAS_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x10 << RG_EARCRX_BIAS_INTR_CAL_SHIFT),
		RG_EARCRX_BIAS_INTR_CAL);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_DM_LDO09_SEL_SHIFT),
		RG_EARCRX_DM_LDO09_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_DM_LDO_LVROD_SHIFT),
		RG_EARCRX_DM_LDO_LVROD);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_LDO_PWD_SHIFT),
		RG_EARCRX_CM_LDO_PWD);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_LDO_LVROD_SHIFT),
		RG_EARCRX_CM_LDO_LVROD);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_REFGEN_MONEN_SHIFT),
		RG_EARCRX_REFGEN_MONEN);

	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x1 << RG_EARCRX_TERM50_EN_SHIFT),
		RG_EARCRX_TERM50_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x10 << RG_EARCRX_TERM50_SEL_SHIFT),
		RG_EARCRX_TERM50_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_VCM_TERM_SEL_SHIFT),
		RG_EARCRX_VCM_TERM_SEL);

	/*vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x0 << DA_EARCRX_ARC_EN_SHIFT),*/
	/*	DA_EARCRX_ARC_EN);*/
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x1 << RG_EARCRX_CMTX_EN_SHIFT),
		RG_EARCRX_CMTX_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_DM_LDO_MONEN_SHIFT),
		RG_EARCRX_DM_LDO_MONEN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x1 << RG_EARCRX_PWM_CDR_EN_SHIFT),
		RG_EARCRX_PWM_CDR_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0,
		0x1 << RG_EARCRX_PWM_CDR_TOBDET_EN_SHIFT,
		RG_EARCRX_PWM_CDR_TOBDET_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_PWM_GEAR_SEL_SHIFT),
		RG_EARCRX_PWM_GEAR_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_ARC_EN_SHIFT),
		RG_EARCRX_ARC_EN);
	/*vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x0 << RG_EARCRX_REV_SHIFT),*/
	/*	RG_EARCRX_REV);*/
	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x1 << RG_EARCRX_REV_SHIFT),
		RG_EARCRX_REV);

	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_VREFT_SEL_SHIFT),
		RG_EARCRX_CM_VREFT_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_REG_PWD_SHIFT),
		RG_EARCRX_CM_REG_PWD);
	vWriteAnaEarcMsk(EARC_RX_CFG_0, (0x0 << RG_EARCRX_CM_LDO_MONEN_SHIFT),
		RG_EARCRX_CM_LDO_MONEN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_CM_TRAN_SEL_SHIFT),
	RG_EARCRX_CM_TRAN_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x4 << RG_EARCRX_CM_DLY_SEL_SHIFT),
		RG_EARCRX_CM_DLY_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_CM_SLEW_OSC_EN_SHIFT),
		RG_EARCRX_CM_SLEW_OSC_EN);

	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x1 << RG_EARCRX_CMRX_EN_SHIFT),
		RG_EARCRX_CMRX_EN);
	/*vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x1 << RG_EARCRX_CMRX_BUF_EN_SHIFT),*/
	/*	RG_EARCRX_CMRX_BUF_EN);*/

	/*force ARC mode*/
	vWriteAnaEarcMsk(EARC_RX_CFG_1,
		(0x0 << RG_EARCRX_CMRX_BUF_EN_SHIFT),
		RG_EARCRX_CMRX_BUF_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_2,
		(0x1 << DA_EARCRX_ARC_EN_SHIFT), DA_EARCRX_ARC_EN);

	vWriteAnaEarcMsk(EARC_RX_CFG_1,
		(0x0 << RG_EARCRX_CMRX_HPF_BW_CTRL_SHIFT),
		RG_EARCRX_CMRX_HPF_BW_CTRL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1,
		(0x1 << RG_EARCRX_CMRX_HYST_AMP_EN_SHIFT),
		RG_EARCRX_CMRX_HYST_AMP_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_CMRX_HYST_SEL_SHIFT),
		RG_EARCRX_CMRX_HYST_SEL);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x0 << RG_EARCRX_CM_VREF_MONEN_SHIFT),
		RG_EARCRX_CM_VREF_MONEN);
	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x0 << RG_EARCRX_VDCTST_SEL_SHIFT),
		RG_EARCRX_VDCTST_SEL);

	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x1 << DA_EARCRXPLL_SDM_PWR_ON_SHIFT),
		DA_EARCRXPLL_SDM_PWR_ON);
	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x0 << DA_EARCRXPLL_SDM_ISO_EN_SHIFT),
		DA_EARCRXPLL_SDM_ISO_EN);

	if (earc_test_value == 0x0) {
		vWriteAnaEarcMsk(EARC_RX_CFG_3, 0x4D000000,
			DA_EARCRXPLL_SDM_PCW);
	} else {
		vWriteAnaEarcMsk(EARC_RX_CFG_3, earc_test_value << 24,
			DA_EARCRXPLL_SDM_PCW);
	}
	vWriteAnaEarcMsk(EARC_RX_CFG_2, (0x1 << DA_EARCRXPLL_SDM_PCW_CHG_SHIFT),
		DA_EARCRXPLL_SDM_PCW_CHG);

	vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x1 << RG_EARCRXPLL_EN_SHIFT),
		RG_EARCRXPLL_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_PREDIV_SHIFT,
		RG_EARCRXPLL_PREDIV);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_POSDIV_SHIFT,
		RG_EARCRXPLL_POSDIV);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x1 << RG_EARCRXPLL_BLP_SHIFT,
		RG_EARCRXPLL_BLP);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_DIV3_EN_SHIFT,
		RG_EARCRXPLL_DIV3_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_GLITCHFREE_EN_SHIFT,
		RG_EARCRXPLL_GLITCHFREE_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_LVROD_EN_SHIFT,
		RG_EARCRXPLL_LVROD_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_MONCK_EN_SHIFT,
		RG_EARCRXPLL_MONCK_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_MONREF_EN_SHIFT,
		RG_EARCRXPLL_MONREF_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_MONVC_EN_SHIFT,
		RG_EARCRXPLL_MONVC_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_1, 0x0 << RG_EARCRXPLL_SDM_FRA_EN_SHIFT,
		RG_EARCRXPLL_SDM_FRA_EN);
	vWriteAnaEarcMsk(EARC_RX_CFG_2, 0x0 << RG_EARCRXPLL_TSTDIV_SHIFT,
		RG_EARCRXPLL_TSTDIV);

	vWriteAnaEarcMsk(EARC_RX_CFG_0C0, RG_EARCRX_MUX,
		RG_EARCRX_MUX);
}

void eARC_digital_Init(void)
{
	EARC_LOG("%s\n", __func__);

	vWriteEarcMsk(COMMA_FREQ_METER, freq_meter_cnt, freq_meter_cnt);
	vWriteEarcMsk(COMMA_FREQ_METER, 0x1 << freq_clock_select_shift,
		freq_clock_select);

	vWriteEarcMsk(COMMA_TIMER_DISC_1,
		(0xD0 << rg_us_counter_shift), rg_us_counter);
	/*208M*/
	vWriteEarcMsk(COMMA_TIMER_RCV_PKT_1,
	(0x7D << tx_timer_eARC_tgl_cm_max_shift), tx_timer_eARC_tgl_cm_max);
	vWriteEarcMsk(COMMA_TIMER_RCV_PKT_1,
		(0x53 << tx_timer_eARC_tgl_cm_min_shift),
		tx_timer_eARC_tgl_cm_min);
	/*0x6D=109=208*0.525; 0x63=99=208*0.475*/

	vWriteEarcMsk(COMMA_TIMER_RCV_PKT_2,
	(0xFA << tx_timer_eARC_bit_cm_max_shift), tx_timer_eARC_bit_cm_max);
	vWriteEarcMsk(COMMA_TIMER_RCV_PKT_2,
		(0xA6 << tx_timer_eARC_bit_cm_min_shift),
		tx_timer_eARC_bit_cm_min);
	/*0xDA=218=208*1.05; 0xC6=198=208*0.95*/

	vWriteEarcMsk(COMMA_TIMER_SND_PKT_1,
	(0x68 << tx_timer_eARC_tgl_cm_shift), tx_timer_eARC_tgl_cm);
	/*208M*/

	vWriteEarcMsk(COMMA_TOP_CONFIG_1,
	(0xFA << rg_delitch_htplg_shift), rg_delitch_htplg);
	vWriteEarcMsk(COMMA_TOP_CONFIG_1,
		(0x32 << rg_deglitch_cm_shift), rg_deglitch_cm);
	vWriteEarcMsk(COMMA_TIMER_SND_PKT_1,
		(0x01 << tx_timer_cm_turnover_shift), tx_timer_cm_turnover);

	vWriteEarcMsk(COMMA_TIMER_DISC_2,
	(0x7D << rx_timer_eARC_lost_HB_shift), rx_timer_eARC_lost_HB);

	vWriteEarcMsk(COMMA_TOP_CONTROL,
	(0x0 << rg_rcv_ecc_bypass_shift), rg_rcv_ecc_bypass);

	vWriteEarcMsk(COMMA_RX_AUD_CNTL,
		(0x1 << rg_mute_soft_sel_shift), rg_mute_soft_sel);
	vWriteEarcMsk(COMMA_RX_AUD_CNTL,
		(0x1 << rg_mute_soft_shift), rg_mute_soft);
	vWriteEarcMsk(COMMA_TOP_CONFIG_2,
		(0x0 << rg_fmt_chg_detect_en_shift), rg_fmt_chg_detect_en);
	vWriteEarcMsk(COMMA_TOP_CONFIG_2,
		(0x1 << rg_soft_rstb_dishpd_shift), rg_soft_rstb_dishpd);
}

void eARC_RX_CM_Enable(int enable)
{
	vWriteEarcMsk(COMMA_DISC_CNTL,
		(enable << rg_detect_en_shift), rg_detect_en);
}

void ARC_RX_Enable(unsigned int enable)
{
	EARC_LOG("%s\n", __func__);

	if (enable == 1) {
		EARC_LOG("ARC Mode!!!\n");
		vWriteAnaEarcMsk(EARC_RX_CFG_1,
			(0x0 << RG_EARCRX_CMRX_BUF_EN_SHIFT),
			RG_EARCRX_CMRX_BUF_EN);
		vWriteAnaEarcMsk(EARC_RX_CFG_2,
			(0x1 << DA_EARCRX_ARC_EN_SHIFT), DA_EARCRX_ARC_EN);
	} else if (enable == 0) {
		EARC_LOG("EARC Mode!!!\n");
		vWriteAnaEarcMsk(EARC_RX_CFG_1,
			(0x1 << RG_EARCRX_CMRX_BUF_EN_SHIFT),
			RG_EARCRX_CMRX_BUF_EN);
		vWriteAnaEarcMsk(EARC_RX_CFG_2,
			(0x0 << DA_EARCRX_ARC_EN_SHIFT), DA_EARCRX_ARC_EN);

		HAL_Delay_us(1000);
		vWriteAnaEarcMsk(EARC_RX_CFG_1, (0x1 << RG_EARCRX_ARC_EN_SHIFT),
			RG_EARCRX_ARC_EN);
	}
}

void eARC_RX_Freq_Meter(char freqtype)
{
	int freqValue, freq;

	if (freqtype == 0)
		vWriteEarcMsk(COMMA_FREQ_METER,
		0x0 << freq_clock_select_shift, freq_clock_select);
	else if (freqtype == 1)
		vWriteEarcMsk(COMMA_FREQ_METER,
		0x01 << freq_clock_select_shift, freq_clock_select);
	else if (freqtype == 2)
		vWriteEarcMsk(COMMA_FREQ_METER,
		0x02 << freq_clock_select_shift, freq_clock_select);
	else if (freqtype == 3)
		vWriteEarcMsk(COMMA_FREQ_METER,
		0x03 << freq_clock_select_shift, freq_clock_select);

	vWriteEarcMsk(COMMA_FREQ_METER, freq_meter_cnt, freq_meter_cnt);
	vWriteEarcMsk(COMMA_FREQ_METER,
		freq_meter_enable, freq_meter_enable);
	vWriteEarcMsk(COMMA_PULSE_CNTL, begin_cal, begin_cal);
	/*delay 100us*/

	freqValue = (bReadEarc(COMMA_FREQ_METER) >> 16) & 0xFFFF;
	freq = ((freqValue / 1024) * 24) / 1000;
	EARC_LOG("freq = %d ; freqValue = 0x%x!\n", freq, freqValue);
}

void eARC_RX_AudioIn_Enable(int enable)
{
	EARC_LOG("%s\n", __func__);

	vWriteEarcMsk(AUDIO_RX_USER_11,
		enable << audioin_enable_shift, audioin_enable);
}

static irqreturn_t earc_irq_handler(int irq, void *dev_id)
{
	int	u32IntSta, u32AudFormat;

	u32IntSta = bReadEarc(COMMA_RX_INT_READ);
	EARC_LOG("irq handle u32IntSta = 0x%8x!\n", u32IntSta);

	if (u32IntSta & 0x1000000) {
		/* bit24 audio format change interrupt enable */
		vWriteEarcMsk(COMMA_RX_INT_CLR,
		1 << int_aud_fmt_chg_clr_shift, int_aud_fmt_chg_clr);
		vWriteEarcMsk(COMMA_RX_INT_CLR,
			0 << int_aud_fmt_chg_clr_shift, int_aud_fmt_chg_clr);

		/*read audio format and notify*/
		u32AudFormat = (bReadEarc(COMMA_AUDIO_READ)&0xFF00)>>8;
		EARC_LOG("audio format change! 0x%x\n", u32AudFormat);

		Earc_Audio_Format();
	}

	/*vWriteEarcMsk(COMMA_RX_INT_CLR, 0x1 >>\*/
	/*int_hpd_raise_cap_clr_shift, int_hpd_raise_cap_clr);*/
	/*vWriteEarcMsk(COMMA_RX_INT_CLR, 0x1 >>\*/
	/*int_hpd_fall_cap_clr_shift, int_hpd_fall_cap_clr);*/
	/*vWriteEarcMsk(COMMA_RX_INT_CLR, 0x0 >>\*/
	/*int_hpd_raise_cap_clr_shift, int_hpd_raise_cap_clr);*/
	/*vWriteEarcMsk(COMMA_RX_INT_CLR, 0x0 >>*/
	/*int_hpd_fall_cap_clr_shift, int_hpd_fall_cap_clr);*/

	return 0;
}

void earc_5vset(int enable)
{
	int earc5v_gpio;

	earc5v_gpio = of_get_named_gpio(earc_node, "earcrx-gpios", 0);
	EARC_LOG("earc5v_gpio = %d\n", earc5v_gpio);
	gpio_request(earc5v_gpio, "earcrx_pin");
	gpio_direction_output(earc5v_gpio, 1);
}

int earc_internal_power_on(void)
{
	int ret = 0;

	ret = clk_prepare_enable(earcclksel);
	if (ret) {
		EARC_LOG("[RX]Failed to enable earcclksel\n");
		return FALSE;
	}

	vWriteEarc(COMMA_RX_INT_MASK, 0x01000000);

	if (request_irq(earc_irq, earc_irq_handler,
		IRQF_TRIGGER_HIGH, "earcirq", NULL) < 0)
		EARC_LOG("request earc interrupt failed.\n");
	else
		EARC_LOG("request earc interrupt success\n");

	eARC_analog_Init();
	eARC_digital_Init();
	eARC_RX_Set_AudLatency(0x46);
	eARC_RX_AudioIn_Enable(1);

	Write_Audiocap();
	earc_5vset(1);

	Earc_StartTimer();

	return 0;
}

int arc_internal_power_on(void)
{
	int ret = 0;

	ret = clk_prepare_enable(earcclksel);
	if (ret) {
		EARC_LOG("[RX]Failed to enable earcclksel\n");
		return FALSE;
	}
	ARC_analog_Init();

	return 0;
}

void earc_internal_power_off(void)
{
	/*printf("[RX]disable power EARC clock\n");*/
	clk_disable_unprepare(earcclksel);

	free_irq(earc_irq, NULL);
	EARC_LOG("Free earc interrupt\n");
}

void arc_internal_power_off(void)
{
	/*printf("[RX]disable power ARC/EARC clock\n");*/
	clk_disable_unprepare(earcclksel);
	EARC_LOG("Disables ARC clock\n");
}

static int earc_probe(struct platform_device *pdev)
{
	int ret;
	int i;

	EARC_LOG("%s\n", __func__);

	earc_node = pdev->dev.of_node;

	earcclksel = devm_clk_get(&pdev->dev, "hdmi_apb_sel");
	if (IS_ERR(earcclksel)) {
		EARC_LOG("[RX]Failed to get earcclksel clk\n");
		return PTR_ERR(earcclksel);
	}

	if (pdev->dev.of_node == NULL) {
		EARC_LOG("[%s] Device Node Error\n", __func__);
		return -1;
	}
	/* iomap registers of EARC Module */
	for (i = 0; i < EARC_REG_NUM; i++) {
		earc_reg[i] = (unsigned long)of_iomap(pdev->dev.of_node, i);
		if (!earc_reg[i]) {
			EARC_LOG(
				"Unable to ioremap registers, of_iomap fail, i=%d\n",
				i);
			return -ENOMEM;
		}
		/*EARC_LOG("DT, i=%d, map_addr=0x%x, reg_pa=0x%x\n",*/
				 /*i, earc_reg[i], earc_reg_base[i]);*/
	}

	/*get IRQ ID and request IRQ */
	EARC_LOG("get IRQ ID and request IRQ\n");
	earc_irq = irq_of_parse_and_map(pdev->dev.of_node, 0);

	/*   debugfs init   */
	ret = earcrx_debug_init();

	/*atomic_set(&earc_irq_event, 1);*/
	/*wake_up_interruptible(&earc_irq_wq);*/

	/*earc_internal_power_on();*/

	return snd_soc_register_component(&pdev->dev, &earc_dir,
			&earc_dai, 1);
}

static int earc_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	destroy_dac_mute_wq();
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id earc_rx_dt_ids[] = {
	{ .compatible = "mediatek,earc-rx", },
	{ }
};
MODULE_DEVICE_TABLE(of, earc_rx_dt_ids);
#endif

static struct platform_driver earc_driver = {
	.probe		= earc_probe,
	.remove		= earc_remove,
	.driver		= {
		.name	= "earc-rx",
		.of_match_table = of_match_ptr(earc_rx_dt_ids),
	},
};

static char debug_buffer[4095] = {0};
static unsigned int temp_len, buf_offset;

#define EARC_ATTR_SPRINTF(fmt, arg...)  \
do { \
	EARC_LOG(fmt, ##arg); \
	if (buf_offset < (sizeof(debug_buffer) - 1)) { \
		temp_len = \
		snprintf(debug_buffer + buf_offset, \
			sizeof(debug_buffer) - buf_offset, fmt, ##arg); \
		if (temp_len > 0) \
			buf_offset += temp_len; \
		debug_buffer[buf_offset] = 0;\
	} \
} while (0)

static unsigned int _ReadReg(char *str_p)
{
	unsigned int u4Data;
	unsigned int u4Addr;
	int ret;

	if (strncmp(str_p, "help", 4) == 0) {
		EARC_ATTR_SPRINTF("rg:0xPPPPPPPP\n");
		return 0;
	}

	ret = sscanf(str_p, "0x%x", &u4Addr);
	if (ret != 1)
		return 0;

	EARC_LOG("%s 0x%x\n", __func__, u4Addr);
	if (u4Addr >= 0x1a001000)
		u4Data = earc_drv_read(u4Addr - 0x1a001000);
	else if (u4Addr >= 0x1a000000)
		u4Data = earc_hdmi_read(u4Addr - 0x1a000000);
	else if (u4Addr >= 0x11f30000)
		u4Data = earc_ana_read(u4Addr - 0x11f30000);
	else
		u4Data = earc_monitor_read(u4Addr - 0x10005000);
	/*u4Data = *(unsigned int *)u4Addr;*/

	EARC_ATTR_SPRINTF("0x%x = 0x%08x\n", u4Addr, u4Data);
	return TRUE;
}

static unsigned int _WriteReg(char *str_p)
{
	unsigned int u4Data;
	unsigned int u4Addr;
	int ret;

	if (strncmp(str_p, "help", 4) == 0) {
		EARC_ATTR_SPRINTF("wg:0xPPPPPPPP/0xPPPPPPPP\n");
		return 0;
	}
	ret = sscanf(str_p, "0x%x=0x%x", &u4Addr, &u4Data);
	if (ret != 2)
		return 0;

	EARC_LOG("%s addr = 0x%x; data = 0x%x\n", __func__, u4Addr, u4Data);

	if (u4Addr >= 0x1a001000)
		earc_drv_write(u4Addr - 0x1a001000, u4Data);
	else if (u4Addr >= 0x1a000000)
		earc_hdmi_write(u4Addr - 0x1a000000, u4Data);
	else if (u4Addr >= 0x11f30000)
		earc_ana_write(u4Addr - 0x11f30000, u4Data);
	else
		earc_monitor_write(u4Addr - 0x10005000, u4Data);
	/*earc_drv_write(u4Addr, u4Data);*/
	/**(unsigned int *)u4Addr = u4Data;*/
	/*u4Data = *(unsigned int *)u4Addr;*/
	EARC_ATTR_SPRINTF("0x%x = 0x%08x\n", u4Addr, u4Data);
	return TRUE;
}

static unsigned int _ReadReg_Range(char *str_p)
{
	unsigned int u4Data;
	unsigned int u4Addr;
	unsigned int u4Count;
	int ret;

	if (strncmp(str_p, "help", 4) == 0) {
		EARC_ATTR_SPRINTF("rg:0xPPPPPPPP/0xPPP\n");
		return 0;
	}

	ret = sscanf(str_p, "0x%x/0x%x", &u4Addr, &u4Count);
	if (ret != 2)
		return 0;

	while (u4Count >= 4) {
		/*u4Data = *(unsigned int *)u4Addr;*/

		if (u4Addr >= 0x1a001000)
			u4Data = earc_drv_read(u4Addr - 0x1a001000);
		else if (u4Addr >= 0x1a000000)
			u4Data = earc_hdmi_read(u4Addr - 0x1a000000);
		else if (u4Addr >= 0x11f30000)
			u4Data = earc_ana_read(u4Addr - 0x11f30000);
		else
			u4Data = earc_monitor_read(u4Addr - 0x10005000);

		EARC_ATTR_SPRINTF("0x%x = 0x%08x\n", u4Addr, u4Data);
		HAL_Delay_us(2);
		u4Addr = u4Addr + 4;
		u4Count = u4Count - 4;
	}

	return 0;
}

static unsigned int earc_setlatency(char *str_p)
{
	unsigned int u4Value;
	int ret;

	if (strncmp(str_p, "help", 4) == 0) {
		EARC_ATTR_SPRINTF("setlat:0xPP\n");
		return 0;
	}

	ret = sscanf(str_p, "0x%x", &u4Value);
	if (ret != 1)
		return 0;

	eARC_RX_Set_AudLatency(u4Value);

	return TRUE;
}

static unsigned int earc_freqmeter(char *str_p)
{
	unsigned int u4Value;
	int ret;

	if (strncmp(str_p, "help", 4) == 0) {
		EARC_ATTR_SPRINTF("fmeter:0xPP\n");
		return 0;
	}

	ret = sscanf(str_p, "0x%x", &u4Value);
	if (ret != 1)
		return 0;

	eARC_RX_Freq_Meter(u4Value);

	return TRUE;
}

static unsigned int earc_test(char *str_p)
{
	unsigned int u4Value;
	int ret;

	if (strncmp(str_p, "help", 4) == 0) {
		EARC_ATTR_SPRINTF("test:0xPP\n");
		return 0;
	}

	ret = sscanf(str_p, "0x%x", &u4Value);
	if (ret != 1)
		return 0;
	earc_test_value = u4Value;

	return TRUE;
}

static unsigned int arc_enable(char *str_p)
{
	unsigned int u4Value;
	int ret;

	if (strncmp(str_p, "help", 4) == 0) {
		EARC_ATTR_SPRINTF("arcen:0xPP\n");
		return 0;
	}

	ret = sscanf(str_p, "0x%x", &u4Value);
	if (ret != 1)
		return 0;
	ARC_RX_Enable(u4Value);

	return TRUE;
}

static unsigned int earc_gpio_setting(void)
{
	EARC_LOG("%s\n", __func__);
	vWriteMonitorEarc(0x1000, 0x0B160001);
	vWriteMonitorEarc(0x1314, 0x00001116);
	vWriteMonitorEarc(0x1314, 0x0000111E);
	vWriteMonitorEarc(0x1314, 0x0000110E);
	vWriteMonitorEarc(0x1314, 0x0000110C);
	vWriteMonitorEarc(0x1314, 0x0000110D);
	vWriteMonitorEarc(0x1314, 0x0000100D);
	vWriteMonitorEarc(0x1314, 0x0000000D);

	vWriteEarc(0x05C, 0x00008FFF);
	vWriteEarc(0x02C, 0x00000004);
	vWriteAnaEarc(0x064, 0x00003809);
	vWriteMonitorEarc(0x6D0, 0x0000000A);
	vWriteMonitorEarc(0x354, 0x00007000);

	return TRUE;
}

static unsigned int set_earcdebuglevel(char *str_p)
{
	unsigned int debuglevel;
	int ret;

	if (strncmp(str_p, "help", 4) == 0) {
		EARC_ATTR_SPRINTF("dbglevel:0xPPP\n");
		return 0;
	}

	ret = sscanf(str_p, "0x%x", &debuglevel);
	if (ret != 1)
		return 0;

	EARC_ATTR_SPRINTF("dbglevel = %d\n", debuglevel);

	EARC_LOG("earcpolllog =   0x1\n");
	EARC_LOG("earcdrvlog =   0x2\n");

	earc_log_on = debuglevel;
	return 0;
}

/* ------------------------------------------ */
/* Debug FileSystem Routines */
/* ------------------------------------------ */
static void process_dbg_cmd(char *opt)
{
	if (strncmp(opt, "r:", 2) == 0)
		_ReadReg(opt + 2);
	else if (strncmp(opt, "w:", 2) == 0)
		_WriteReg(opt + 2);
	else if (strncmp(opt, "rr:", 3) == 0)
		_ReadReg_Range(opt + 3);
	else if (strncmp(opt, "poweron", 7) == 0)
		earc_internal_power_on();
	else if (strncmp(opt, "poweroff", 8) == 0)
		earc_internal_power_off();
	else if (strncmp(opt, "setlat:", 7) == 0)
		earc_setlatency(opt + 7);
	else if (strncmp(opt, "test:", 5) == 0)
		earc_test(opt + 5);
	else if (strncmp(opt, "init", 4) == 0)
		eARC_digital_Init();
	else if (strncmp(opt, "anainit", 7) == 0)
		eARC_analog_Init();
	else if (strncmp(opt, "wcap", 4) == 0)
		Write_Audiocap();
	else if (strncmp(opt, "rcap", 4) == 0)
		eARC_RX_Read_Audiocap();
	else if (strncmp(opt, "updatecap", 9) == 0)
		eARC_RX_update_cap();
	else if (strncmp(opt, "status", 6) == 0)
		Earc_Status();
	else if (strncmp(opt, "fmeter:", 7) == 0)
		earc_freqmeter(opt + 7);
	else if (strncmp(opt, "debug:", 6) == 0)
		set_earcdebuglevel(opt + 6);
	else if (strncmp(opt, "arcen:", 6) == 0)
		arc_enable(opt + 6);
	else if (strncmp(opt, "gpioset:", 8) == 0)
		earc_gpio_setting();
}

struct dentry *earcrx_debugfs;

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t debug_read(struct file *file,
	char __user *ubuf, size_t count, loff_t *ppos)
{
	int n = 0;

	n = strlen(debug_buffer);
	if ((n >= 0) && (n < sizeof(debug_buffer)))
		debug_buffer[n++] = 0;

	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

static ssize_t debug_write(struct file *file,
	const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(debug_buffer) - 1;
	ssize_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&debug_buffer, ubuf, count))
		return -EFAULT;

	debug_buffer[count] = 0;
	buf_offset = 0;

	process_dbg_cmd(debug_buffer);

	return ret;
}

static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};

int earcrx_debug_init(void)
{
	EARC_LOG("%s\n", __func__);
	earcrx_debugfs = debugfs_create_file("earcrx",
		S_IFREG | 0444, NULL, (void *)0, &debug_fops);

	if (IS_ERR(earcrx_debugfs))
		return PTR_ERR(earcrx_debugfs);

	return 0;
}

void earcrx_debug_uninit(void)
{
	debugfs_remove(earcrx_debugfs);
}

void earc_irq_impl(void)
{
	int	u32IntSta;

	u32IntSta = bReadEarc(COMMA_RX_INT_READ);
	EARC_LOG("u32IntSta = 0x%8x!\n", u32IntSta);

}

int earc_irq_kthread(void *data)
{
#if 0
	struct sched_param param = {.sched_priority = RTPM_PRIO_SCRN_UPDATE };

	sched_setscheduler(current, SCHED_RR, &param);
#endif

	for (;;) {
		wait_event_interruptible(earc_irq_wq,
			atomic_read(&earc_irq_event));
		atomic_set(&earc_irq_event, 0);
		earc_irq_impl();
		if (kthread_should_stop())
			break;
	}

	return 0;
}

static int __init earc_init(void)
{
	EARC_LOG("%s\n", __func__);

#if 0
	init_waitqueue_head(&earc_irq_wq);
	earc_irq_task = kthread_create(earc_irq_kthread,
					NULL, "earc_irq_kthread");
	wake_up_process(earc_irq_task);
#endif

	if (platform_driver_register(&earc_driver)) {
		EARC_LOG("failed to register earc driver\n");
		return -1;
	}

	return 0;
}
module_init(earc_init);

/*module_platform_driver(earc_driver);*/

MODULE_DESCRIPTION("MTK EARC SoC codec driver");
MODULE_AUTHOR("Yulan Zhang <yulan.zhang@mediatek.com>");
MODULE_LICENSE("GPL");
