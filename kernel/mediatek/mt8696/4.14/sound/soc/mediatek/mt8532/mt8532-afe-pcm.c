/*
 * mt8532-afe-pcm.c  --  Mediatek 8532 ALSA SoC AFE platform driver
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
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
#include <linux/atomic.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "mt8532-afe-common.h"
#include "mt8532-afe-utils.h"
#include "mt8532-reg.h"
#include "mt8532-afe-debug.h"
#include "mt8532-afe-controls.h"
#include "../common/mtk-base-afe.h"
#include "../common/mtk-afe-platform-driver.h"
#include "../common/mtk-afe-fe-dai.h"
#include "mt8532-snd-utils.h"
#ifdef CONFIG_SND_SOC_MT8570
#include "mt8570-adsp-utils.h"
#endif
#ifdef CONFIG_MTK_AUDIO_ASRC
#include "mtk_asrc_export_api.h"
#endif

#define MT8532_ETDM1_OUT_MCLK_MULTIPLIER 256
#define MT8532_ETDM1_IN_MCLK_MULTIPLIER 256
#define MT8532_ETDM2_OUT_MCLK_MULTIPLIER 256
#define MT8532_ETDM2_IN_MCLK_MULTIPLIER 256
#define MT8532_ETDM3_OUT_MCLK_MULTIPLIER 256
#define MT8532_ETDM3_IN_MCLK_MULTIPLIER 256
#define MT8532_ETDM_NORMAL_MAX_BCK_RATE 24576000

#define WAIT_OUTPUT_ACTIVE_DELAY_NS (NSEC_PER_MSEC * 20)
#define CLK_TUNE_CALI_WARNING_TIME_OUT_NS (NSEC_PER_SEC * 1)
#define MAX_MPHONE_MULTI_PERIOD_BYTES (1024)

#define ARCIN_FEATURE_SUPPORT 1
#if ARCIN_FEATURE_SUPPORT
static struct mtk_base_afe *g_afe = NULL;
struct work_struct uevent_work;
enum {
	NOTIFY_UEVENT_KEY_LOCK = 0,
	NOTIFY_UEVENT_KEY_NUM,
};
static const char *const key_str[] = {
	"LOCK=%d",
};
static char env_ext[NOTIFY_UEVENT_KEY_NUM][64];
#endif


#define PCM_STREAM_STR(x) \
	(((x) == SNDRV_PCM_STREAM_CAPTURE) ? "Capture" : "Playback")

// TODO: revise registers to backup
static const unsigned int mt8532_afe_backup_list[] = {
	AUDIO_TOP_CON0,
	AUDIO_TOP_CON2,
	AUDIO_TOP_CON4,
	AUDIO_TOP_CON5,
	ASYS_TOP_CON,
	AFE_CONN1,
	AFE_CONN2,
	AFE_CONN3,
	AFE_CONN4,
	AFE_CONN5,
	AFE_CONN6,
	AFE_CONN7,
	AFE_CONN8,
	AFE_CONN9,
	AFE_CONN10,
	AFE_CONN11,
	AFE_CONN12,
	AFE_CONN13,
	AFE_CONN14,
	AFE_CONN15,
	AFE_CONN16,
	AFE_CONN17,
	AFE_CONN18,
	AFE_CONN19,
	AFE_CONN20,
	AFE_CONN21,
	AFE_CONN26,
	AFE_CONN27,
	AFE_CONN28,
	AFE_CONN29,
	AFE_CONN30,
	AFE_CONN31,
	AFE_CONN32,
	AFE_CONN33,
	AFE_CONN34,
	AFE_CONN35,
	AFE_CONN36,
	AFE_CONN37,
	AFE_CONN38,
	AFE_CONN39,
	AFE_CONN40,
	AFE_CONN41,
	AFE_CONN42,
	AFE_CONN43,
	AFE_CONN44,
	AFE_CONN45,
	AFE_CONN46,
	AFE_CONN47,
	AFE_CONN48,
	AFE_CONN49,
	AFE_CONN53,
	AFE_CONN54,
	AFE_CONN55,
	AFE_CONN56,
	AFE_CONN57,
	AFE_CONN58,
	AFE_CONN59,
	AFE_CONN60,
	AFE_CONN61,
	AFE_CONN64,
	AFE_CONN65,
	AFE_CONN66,
	AFE_CONN67,
	AFE_CONN68,
	AFE_CONN69,
	AFE_CONN70,
	AFE_CONN71,
	AFE_CONN72,
	AFE_CONN73,
	AFE_CONN74,
	AFE_CONN75,
	AFE_CONN76,
	AFE_CONN_16BIT,
	AFE_CONN_24BIT,
	AFE_TDMOUT_CONN0,
	AFE_TDMOUT_CONN1,
	AFE_DL2_BASE,
	AFE_DL2_END,
	AFE_DL2_CON0,
	AFE_DL3_BASE,
	AFE_DL3_END,
	AFE_DL3_CON0,
	AFE_DL6_BASE,
	AFE_DL6_END,
	AFE_DL6_CON0,
	AFE_DL7_BASE,
	AFE_DL7_END,
	AFE_DL7_CON0,
	AFE_DL8_BASE,
	AFE_DL8_END,
	AFE_DL8_CON0,
	AFE_DL10_BASE,
	AFE_DL10_END,
	AFE_DL10_CON0,
	AFE_UL1_BASE,
	AFE_UL1_END,
	AFE_UL1_CON0,
	AFE_UL2_BASE,
	AFE_UL2_END,
	AFE_UL2_CON0,
	AFE_UL3_BASE,
	AFE_UL3_END,
	AFE_UL3_CON0,
	AFE_UL4_BASE,
	AFE_UL4_END,
	AFE_UL4_CON0,
	AFE_UL5_BASE,
	AFE_UL5_END,
	AFE_UL5_CON0,
	AFE_UL8_BASE,
	AFE_UL8_END,
	AFE_UL8_CON0,
	AFE_UL9_BASE,
	AFE_UL9_END,
	AFE_UL9_CON0,
	AFE_UL10_BASE,
	AFE_UL10_END,
	AFE_UL10_CON0,
	AFE_MEMIF_AGENT_FS_CON0,
	AFE_MEMIF_AGENT_FS_CON1,
	AFE_MEMIF_AGENT_FS_CON2,
	AFE_MEMIF_AGENT_FS_CON3,
	AFE_DAC_CON0,
	AFE_IRQ_MASK,
	ETDM_OUT1_CON0,
	ETDM_OUT1_CON1,
	ETDM_OUT1_CON2,
	ETDM_OUT1_CON3,
	ETDM_OUT1_CON4,
	ETDM_IN1_CON0,
	ETDM_IN1_CON1,
	ETDM_IN1_CON2,
	ETDM_IN1_CON3,
	ETDM_IN1_CON4,
	ETDM_OUT2_CON0,
	ETDM_OUT2_CON1,
	ETDM_OUT2_CON2,
	ETDM_OUT2_CON3,
	ETDM_OUT2_CON4,
	ETDM_IN2_CON0,
	ETDM_IN2_CON1,
	ETDM_IN2_CON2,
	ETDM_IN2_CON3,
	ETDM_IN2_CON4,
	AFE_NORMAL_BASE_ADR_MSB,
	AFE_NORMAL_END_ADR_MSB,
	AFE_CM1_CON,
	AFE_CM0_CON,
	AFE_SINEGEN_CON0,
	PWR2_TOP_CON0,
	PWR2_TOP_CON1,
};

static const struct snd_pcm_hardware mt8532_afe_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.buffer_bytes_max = 512 * 1024,
	.period_bytes_min = 64,
	.period_bytes_max = 256 * 1024,
	.periods_min = 2,
	.periods_max = 256,
	.fifo_size = 0,
};

struct mt8532_afe_rate {
	unsigned int rate;
	unsigned int reg_val;
};

static int tdm_fs(unsigned int fs)
{
	switch (fs) {
	case MT8532_FS_8K:
		return MT8532_TDM_8K;
	case MT8532_FS_12K:
		return MT8532_TDM_12K;
	case MT8532_FS_16K:
		return MT8532_TDM_16K;
	case MT8532_FS_24K:
		return MT8532_TDM_24K;
	case MT8532_FS_32K:
		return MT8532_TDM_32K;
	case MT8532_FS_48K:
		return MT8532_TDM_48K;
	case MT8532_FS_96K:
		return MT8532_TDM_96K;
	case MT8532_FS_192K:
		return MT8532_TDM_192K;
	case MT8532_FS_384K:
		return MT8532_TDM_384K;
	case MT8532_FS_11D025K:
		return MT8532_TDM_11D025K;
	case MT8532_FS_22D05K:
		return MT8532_TDM_22D05K;
	case MT8532_FS_44D1K:
		return MT8532_TDM_44D1K;
	case MT8532_FS_88D2K:
		return MT8532_TDM_88D2K;
	case MT8532_FS_176D4K:
		return MT8532_TDM_176D4K;
	case MT8532_FS_352D8K:
		return MT8532_TDM_352D8K;
	default:
		return -EINVAL;
	}
}


static const struct mt8532_afe_rate mt8532_afe_fs_rates[] = {
	{ .rate = 8000, .reg_val = MT8532_FS_8K },
	{ .rate = 12000, .reg_val = MT8532_FS_12K },
	{ .rate = 16000, .reg_val = MT8532_FS_16K },
	{ .rate = 24000, .reg_val = MT8532_FS_24K },
	{ .rate = 32000, .reg_val = MT8532_FS_32K },
	{ .rate = 48000, .reg_val = MT8532_FS_48K },
	{ .rate = 96000, .reg_val = MT8532_FS_96K },
	{ .rate = 192000, .reg_val = MT8532_FS_192K },
	{ .rate = 384000, .reg_val = MT8532_FS_384K },
	{ .rate = 7350, .reg_val = MT8532_FS_7D35K },
	{ .rate = 11025, .reg_val = MT8532_FS_11D025K },
	{ .rate = 14700, .reg_val = MT8532_FS_14D7K },
	{ .rate = 22050, .reg_val = MT8532_FS_22D05K },
	{ .rate = 29400, .reg_val = MT8532_FS_29D4K },
	{ .rate = 44100, .reg_val = MT8532_FS_44D1K },
	{ .rate = 88200, .reg_val = MT8532_FS_88D2K },
	{ .rate = 176400, .reg_val = MT8532_FS_176D4K },
	{ .rate = 352800, .reg_val = MT8532_FS_352D8K },
};

static int mt8532_afe_emu_to_rate(unsigned int emu)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt8532_afe_fs_rates); i++)
		if (mt8532_afe_fs_rates[i].reg_val == emu)
			return mt8532_afe_fs_rates[i].rate;

	return -EINVAL;
}

static int mt8532_afe_fs_timing(unsigned int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt8532_afe_fs_rates); i++)
		if (mt8532_afe_fs_rates[i].rate == rate)
			return mt8532_afe_fs_rates[i].reg_val;

	return -EINVAL;
}

bool mt8532_afe_rate_supported(unsigned int rate, unsigned int id)
{
	switch (id) {
	case MT8532_AFE_IO_ETDM1_OUT:
		/* FALLTHROUGH */
	case MT8532_AFE_IO_ETDM1_IN:
		/* FALLTHROUGH */
	case MT8532_AFE_IO_ETDM2_OUT:
		/* FALLTHROUGH */
	case MT8532_AFE_IO_ETDM2_IN:
		if (rate >= 8000 && rate <= 384000)
			return true;
		break;
	case MT8532_AFE_IO_DMIC:
		/* FALLTHROUGH */
	case MT8532_AFE_IO_PCM1:
		/* FALLTHROUGH */
	default:
		break;
	}

	return false;
}
EXPORT_SYMBOL_GPL(mt8532_afe_rate_supported);

#if ARCIN_FEATURE_SUPPORT
static void uevent_notify_init(void)
{
	scnprintf(env_ext[NOTIFY_UEVENT_KEY_LOCK], 64,
		key_str[NOTIFY_UEVENT_KEY_LOCK], 0);
}

static void uevent_notify_work(struct work_struct *wq)
{
	char *envp_ext[NOTIFY_UEVENT_KEY_NUM + 1] = {NULL};

	envp_ext[NOTIFY_UEVENT_KEY_LOCK] = env_ext[NOTIFY_UEVENT_KEY_LOCK];

	kobject_uevent_env(&g_afe->dev->kobj, KOBJ_CHANGE, envp_ext);
}

void ARCIN_Enable(bool enable)
{
	unsigned int port = enable?SPDIF_IN_PORT_ARC:SPDIF_IN_PORT_NONE;
	pr_info("%s() enable:%d\n", __func__, enable);
	if (g_afe) {
		mt8532_afe_handle_spdif_in_port_change(g_afe, port);
	} else {
		pr_info("%s() afe is null for enable:%d\n", __func__, enable);
	}
}
EXPORT_SYMBOL(ARCIN_Enable);
#endif

bool mt8532_afe_channel_supported(unsigned int channel, unsigned int id)
{
	switch (id) {
	case MT8532_AFE_IO_ETDM1_OUT:
		/* FALLTHROUGH */
	case MT8532_AFE_IO_ETDM1_IN:
		if (channel >= 1 && channel <= 16)
			return true;
		break;
	case MT8532_AFE_IO_ETDM2_OUT:
		/* FALLTHROUGH */
	case MT8532_AFE_IO_ETDM2_IN:
		if (channel >= 1 && channel <= 8)
			return true;
		break;
	case MT8532_AFE_IO_DMIC:
		if (channel >= 1 && channel <= 8)
			return true;
		break;
	case MT8532_AFE_IO_PCM1:
		/* FALLTHROUGH */
		break;
	default:
		break;
	}

	return false;
}
EXPORT_SYMBOL_GPL(mt8532_afe_channel_supported);

#if 0
static bool is_ul3_in_direct_mode(struct mtk_base_afe *afe)
{
	unsigned int val = 0;

	regmap_read(afe->regmap, ETDM_COWORK_CON1, &val);

	if (val & ETDM_COWORK_CON1_TDM_IN2_BYPASS_INTERCONN)
		return true;
	else
		return false;
}
#endif

static const char * const mt8532_afe_pin_str[AFE_PIN_STATE_MAX] = {
	[AFE_PIN_STATE_DEFAULT] = "default",
	[AFE_PIN_STATE_ETDM1_OUT_ON] = "etdm1_out_on",
	[AFE_PIN_STATE_ETDM1_OUT_OFF] = "etdm1_out_off",
	[AFE_PIN_STATE_ETDM1_IN_ON] = "etdm1_in_on",
	[AFE_PIN_STATE_ETDM1_IN_OFF] = "etdm1_in_off",
	[AFE_PIN_STATE_ETDM2_OUT_ON] = "etdm2_out_on",
	[AFE_PIN_STATE_ETDM2_OUT_OFF] = "etdm2_out_off",
	[AFE_PIN_STATE_ETDM2_IN_ON] = "etdm2_in_on",
	[AFE_PIN_STATE_ETDM2_IN_OFF] = "etdm2_in_off",
};

static int mt8532_afe_select_pin_state(struct mtk_base_afe *afe,
	unsigned int state)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int ret;

	if (state >= AFE_PIN_STATE_MAX)
		return -EINVAL;

	if (afe_priv->pin_states[state]) {
		ret = pinctrl_select_state(afe_priv->pinctrl,
			afe_priv->pin_states[state]);
		if (ret) {
			dev_info(afe->dev,
				"%s failed to select state '%s' %d\n",
				__func__, mt8532_afe_pin_str[state], ret);
			return ret;
		}
	}

	return 0;
}

#ifdef CONFIG_MTK_AUDIO_ASRC
static int mt8532_afe_asrc_cali_reset(
	struct mt8532_afe_ext_clk_tune_data *clk_tune)
{
	if (clk_tune->start == true) {
		mtk_asrc_cali_stop(clk_tune->asrc_id);
		mtk_asrc_cali_release(clk_tune->asrc_id);
		clk_tune->start = false;
	}

	return 0;
}
#endif
#ifdef CALI_RESULT_CHECK
#define CALI_RES_DELAY_MS	(1000)
#define CALI_MEMIF_DL_PATH	(MT8532_AFE_MEMIF_DLM)
#define CALI_CHK_PRT_FREQ	10
#define FRAMES_TO_MS(f, sr)	(MSEC_PER_SEC * f / sr)

static void mt8532_afe_stop_cali_res_hrt(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_cali_result *cali_res = &afe_priv->cali_res;

	hrtimer_cancel(&cali_res->cali_hrt);
}

static void mt8532_afe_trigger_cali_res_hrt(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_cali_result *cali_res = &afe_priv->cali_res;

	/* initialization */
	cali_res->avg = 0;
	cali_res->cur = 0;
	cali_res->max = 0;
	cali_res->min = 0;
	cali_res->cnt = 0;
	cali_res->sum = 0;
	hrtimer_start(&cali_res->cali_hrt, ns_to_ktime(0), HRTIMER_MODE_REL);
}

static enum hrtimer_restart cali_res_hrt_func(struct hrtimer *hrt)
{
	struct mt8532_cali_result *cali_res = container_of(hrt,
		struct mt8532_cali_result, cali_hrt);
	struct mtk_base_afe *afe = cali_res->afe;
	struct mtk_base_afe_memif *memif = &afe->memif[CALI_MEMIF_DL_PATH];
	struct snd_pcm_substream *substream = memif->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;
	unsigned int output_rate = clk_tune->output_rate;
	u64 tmp;

	if (!substream) {
		dev_info(afe->dev, "%s() substream is null!\n", __func__);
		goto CALI_RES_HRT_RESTART;
	}

	if (!snd_pcm_running(substream)) {
		dev_info(afe->dev, "%s() stream is not running!\n", __func__);
		goto CALI_RES_HRT_RESTART;
	}

	cali_res->cur = snd_pcm_playback_hw_avail(runtime);
	if (cali_res->cur < 0) {
		dev_info(afe->dev, "%s() hw buf is not available!\n", __func__);
		goto CALI_RES_HRT_RESTART;
	}

	if (cali_res->min == 0)
		cali_res->min = cali_res->cur;
	else
		cali_res->min = cali_res->min < cali_res->cur
				? cali_res->min : cali_res->cur;

	if (cali_res->max == 0)
		cali_res->max = cali_res->cur;
	else
		cali_res->max = cali_res->max < cali_res->cur
				? cali_res->cur : cali_res->max;

	cali_res->sum += (u64)cali_res->cur;
	cali_res->cnt++;

	if (cali_res->cnt % CALI_CHK_PRT_FREQ == 0) {
		tmp = cali_res->sum;
		do_div(tmp, cali_res->cnt);
		cali_res->avg = (snd_pcm_sframes_t)tmp;
		dev_info(afe->dev,
			 "%s() #%d cur:%ldms min:%ld max:%ld avg:%ld\n",
			 __func__, cali_res->cnt,
			 FRAMES_TO_MS(cali_res->cur, output_rate),
			 FRAMES_TO_MS(cali_res->min, output_rate),
			 FRAMES_TO_MS(cali_res->max, output_rate),
			 FRAMES_TO_MS(cali_res->avg, output_rate));
	}

CALI_RES_HRT_RESTART:
	hrtimer_forward_now(hrt, ms_to_ktime(CALI_RES_DELAY_MS));
	return HRTIMER_RESTART;
}
#endif

static void mt8532_afe_configure_clk_tune(struct mtk_base_afe *afe,
	unsigned int tune_id,
	unsigned int rate)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;

	dev_dbg(afe->dev, "%s id %u rate %u\n", __func__, tune_id, rate);

	clk_tune->input_rate = rate;
	clk_tune->working = &clk_tune->props[tune_id];
}

static void mt8532_afe_trigger_clk_tune(struct mtk_base_afe *afe,
	unsigned int period_size)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;
	u64 period_ns;

	dev_dbg(afe->dev, "%s period_size %u\n", __func__, period_size);

	if (!clk_tune->input_rate) {
		dev_info(afe->dev, "%s invalid input rate\n", __func__);
		return;
	}

	period_ns = (u64)period_size * NSEC_PER_SEC;

	do_div(period_ns, clk_tune->input_rate);

	clk_tune->working_phase = TUNE_PHASE_INIT;
	clk_tune->cali_retry_time = period_ns;

	queue_delayed_work(system_power_efficient_wq,
			   &clk_tune->clk_tune_work,
			   nsecs_to_jiffies(period_ns));
}

static void mt8532_afe_stop_clk_tune_hrt(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;

	cancel_delayed_work_sync(&clk_tune->clk_tune_work);
#ifdef CONFIG_MTK_AUDIO_ASRC
	mt8532_afe_asrc_cali_reset(clk_tune);
#endif
}

static void mt8532_afe_reset_clk_tune(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;

	dev_dbg(afe->dev, "%s\n", __func__);

	clk_tune->input_rate = 0;
	clk_tune->working = NULL;
}

static unsigned long mt8532_adjust_clk_rate_by_step(struct mtk_base_afe *afe,
	struct clk *clk,
	unsigned long now,
	unsigned long target,
	unsigned long step)
{
	unsigned long next;
	int adj;

	if (now > target) {
		adj = now - target;
		if (step && (adj > step))
			adj = step;

		next = now - adj;
	} else if (now < target) {
		adj = target - now;
		if (step && (adj > step))
			adj = step;

		next = now + adj;
	} else {
		next = now;
		adj = 0;
	}

	if (adj != 0)
		mt8532_afe_set_clk_rate(afe, clk, next);

	return next;
}

static void mt8532_afe_reset_apll_rate(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;
	unsigned long current_rate;
	unsigned long target_rate;
	unsigned long step_hz;
	unsigned long step_delay_us;

	if (!clk_tune->apll_clk) {
		dev_info(afe->dev, "%s invalid apll_clk\n", __func__);
		return;
	}

	if (!clk_tune->working) {
		dev_info(afe->dev, "%s invalid working\n", __func__);
		return;
	}

	current_rate = clk_get_rate(clk_tune->apll_clk);
	target_rate = clk_tune->apll_restore_rate;
	step_hz = clk_tune->apll_step_hz;
	step_delay_us = clk_tune->working->adj_step_us;

	while (current_rate != target_rate) {
		current_rate = mt8532_adjust_clk_rate_by_step(afe,
			clk_tune->apll_clk,
			current_rate,
			target_rate,
			step_hz);

		if (current_rate != target_rate)
			usleep_range(step_delay_us, step_delay_us + 1);
	}
}

static bool mt8532_afe_is_clk_tune_output(struct mtk_base_afe *afe,
	int id)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;
	int index;

	index = mt8532_afe_get_be_idx(id);
	if (index < 0)
		return false;

	if (clk_tune->outputs & BIT(index))
		return true;

	return false;
}

static void mt8532_afe_set_clk_tune_output_rate(struct mtk_base_afe *afe,
	unsigned int rate)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;

	clk_tune->output_rate = rate;
}

#ifdef CONFIG_SND_SOC_MT8570
static int mt8532_afe_get_scene_by_dai_id(int id)
{
	switch (id) {
	case MT8532_AFE_IO_ETDM1_OUT:
		/* FALLTHROUGH */
	case MT8532_AFE_IO_ETDM2_OUT:
		return TASK_SCENE_INTERLINK_HOST_TO_DSP;
	case MT8532_AFE_IO_ETDM1_IN:
		/* FALLTHROUGH */
	case MT8532_AFE_IO_ETDM2_IN:
		return TASK_SCENE_TDM_RECORD;
	default:
		break;
	}

	return -1;
}
#endif

static int mt8532_afe_gasrc_get_period_val(unsigned int rate,
	bool op_freq_45m, unsigned int cali_cycles);

static void mt8532_afe_inc_etdm_occupy(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned long flags;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	etdm_data->occupied[stream]++;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);
}

static void mt8532_afe_dec_etdm_occupy(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned long flags;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	etdm_data->occupied[stream]--;
	if (etdm_data->occupied[stream] < 0)
		etdm_data->occupied[stream] = 0;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);
}

static unsigned int mt8532_afe_tdm_ch_fixup(unsigned int channels)
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

static const struct mt8532_etdm_ctrl_reg etdm_ctrl_reg[MT8532_ETDM_SETS][2] = {
	{
		{
			.con0 = ETDM_OUT1_CON0,
			.con1 = ETDM_OUT1_CON1,
			.con2 = ETDM_OUT1_CON2,
			.con3 = ETDM_OUT1_CON3,
			.con4 = ETDM_OUT1_CON4,
			.con5 = ETDM_OUT1_CON5,
		},
		{
			.con0 = ETDM_IN1_CON0,
			.con1 = ETDM_IN1_CON1,
			.con2 = ETDM_IN1_CON2,
			.con3 = ETDM_IN1_CON3,
			.con4 = ETDM_IN1_CON4,
			.con5 = ETDM_IN1_CON5,
		},
	},
	{
		{
			.con0 = ETDM_OUT2_CON0,
			.con1 = ETDM_OUT2_CON1,
			.con2 = ETDM_OUT2_CON2,
			.con3 = ETDM_OUT2_CON3,
			.con4 = ETDM_OUT2_CON4,
			.con5 = ETDM_OUT2_CON5,
		},
		{
			.con0 = ETDM_IN2_CON0,
			.con1 = ETDM_IN2_CON1,
			.con2 = ETDM_IN2_CON2,
			.con3 = ETDM_IN2_CON3,
			.con4 = ETDM_IN2_CON4,
			.con5 = ETDM_IN2_CON5,
		},
	},
	{
		{
			.con0 = ETDM_OUT3_CON0,
			.con1 = ETDM_OUT3_CON1,
			.con2 = ETDM_OUT3_CON2,
			.con3 = ETDM_OUT3_CON3,
			.con4 = ETDM_OUT3_CON4,
			.con5 = ETDM_OUT3_CON5,
		},
		{
			.con0 = -1,
			.con1 = -1,
			.con2 = -1,
			.con3 = -1,
			.con4 = -1,
			.con5 = -1,
		},
	},
};

static const unsigned int etdm_on_pin_state[MT8532_ETDM_SETS][2] = {
	{AFE_PIN_STATE_ETDM1_OUT_ON, AFE_PIN_STATE_ETDM1_IN_ON},
	{AFE_PIN_STATE_ETDM2_OUT_ON, AFE_PIN_STATE_ETDM2_IN_ON},
};

static const unsigned int etdm_off_pin_state[MT8532_ETDM_SETS][2] = {
	{AFE_PIN_STATE_ETDM1_OUT_OFF, AFE_PIN_STATE_ETDM1_IN_OFF},
	{AFE_PIN_STATE_ETDM2_OUT_OFF, AFE_PIN_STATE_ETDM2_IN_OFF},
};

static void mt8532_afe_enable_etdm(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	unsigned int en_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	unsigned long flags;
	bool need_update = false;

	spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

	etdm_data->active[stream]++;
	if (etdm_data->active[stream] == 1)
		need_update = true;

	spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

	if (need_update) {
		mt8532_afe_select_pin_state(afe,
			etdm_on_pin_state[etdm_set][stream]);

		regmap_update_bits(afe->regmap, en_reg, 0x1, 0x1);

		dev_dbg(afe->dev, "%s#%d '%s'\n",
			__func__, etdm_set + 1, PCM_STREAM_STR(stream));
	}
}

static void mt8532_afe_disable_etdm(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	bool slave_mode = etdm_data->slave_mode[stream];
	unsigned int reg;
	unsigned long flags;
	bool need_update = false;

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

		mt8532_afe_select_pin_state(afe,
			etdm_off_pin_state[etdm_set][stream]);

		dev_dbg(afe->dev, "%s#%d '%s'\n",
			__func__, etdm_set + 1, PCM_STREAM_STR(stream));
	}
}

static unsigned int mt8532_afe_derive_sync_source(unsigned int etdm_set,
	unsigned int stream)
{
	if ((etdm_set == MT8532_ETDM1) &&
	    (stream == SNDRV_PCM_STREAM_PLAYBACK))
		return MT8532_ETDM_SYNC_FROM_OUT1;
	else if ((etdm_set == MT8532_ETDM1) &&
		 (stream == SNDRV_PCM_STREAM_CAPTURE))
		return MT8532_ETDM_SYNC_FROM_IN1;
	else if ((etdm_set == MT8532_ETDM2) &&
		 (stream == SNDRV_PCM_STREAM_PLAYBACK))
		return MT8532_ETDM_SYNC_FROM_OUT2;
	else if ((etdm_set == MT8532_ETDM2) &&
		 (stream == SNDRV_PCM_STREAM_CAPTURE))
		return MT8532_ETDM_SYNC_FROM_IN2;
	else if ((etdm_set == MT8532_ETDM3) &&
		 (stream == SNDRV_PCM_STREAM_PLAYBACK))
		return MT8532_ETDM_SYNC_FROM_OUT3;
	else
		return MT8532_ETDM_SYNC_NONE;
}

static int mt8532_afe_derive_etdm_info(unsigned int sync_source,
	unsigned int *etdm_set,
	unsigned int *stream)
{
	int ret = 0;

	if (sync_source == MT8532_ETDM_SYNC_FROM_IN1) {
		if (etdm_set)
			*etdm_set = MT8532_ETDM1;
		if (stream)
			*stream = SNDRV_PCM_STREAM_CAPTURE;
	} else if (sync_source == MT8532_ETDM_SYNC_FROM_IN2) {
		if (etdm_set)
			*etdm_set = MT8532_ETDM2;
		if (stream)
			*stream = SNDRV_PCM_STREAM_CAPTURE;
	} else if (sync_source == MT8532_ETDM_SYNC_FROM_OUT1) {
		if (etdm_set)
			*etdm_set = MT8532_ETDM1;
		if (stream)
			*stream = SNDRV_PCM_STREAM_PLAYBACK;
	} else if (sync_source == MT8532_ETDM_SYNC_FROM_OUT2) {
		if (etdm_set)
			*etdm_set = MT8532_ETDM2;
		if (stream)
			*stream = SNDRV_PCM_STREAM_PLAYBACK;
	} else if (sync_source == MT8532_ETDM_SYNC_FROM_OUT3) {
		if (etdm_set)
			*etdm_set = MT8532_ETDM3;
		if (stream)
			*stream = SNDRV_PCM_STREAM_PLAYBACK;
	} else
		ret = -EINVAL;

	return ret;
}


static void mt8532_afe_enable_etdm_sync_clients(struct mtk_base_afe *afe,
	unsigned int src_set,
	unsigned int src_stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm;
	unsigned int sync_source;
	unsigned int i, j;

	sync_source = mt8532_afe_derive_sync_source(src_set, src_stream);
	if (sync_source == MT8532_ETDM_SYNC_NONE)
		return;

	for (i = 0; i < MT8532_ETDM_SETS; i++) {
		for (j = 0; j < (SNDRV_PCM_STREAM_LAST + 1); j++) {
			if (i == src_set && j == src_stream)
				continue;

			etdm = &afe_priv->etdm_data[i];
			if (etdm->sync_source[j] == sync_source)
				mt8532_afe_enable_etdm(afe, i, j);
		}
	}
}

static void mt8532_afe_disable_etdm_sync_clients(struct mtk_base_afe *afe,
	unsigned int src_set,
	unsigned int src_stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm;
	unsigned int sync_source;
	unsigned int i, j;

	sync_source = mt8532_afe_derive_sync_source(src_set, src_stream);
	if (sync_source == MT8532_ETDM_SYNC_NONE)
		return;

	for (i = 0; i < MT8532_ETDM_SETS; i++) {
		for (j = 0; j < (SNDRV_PCM_STREAM_LAST + 1); j++) {
			if (i == src_set && j == src_stream)
				continue;

			etdm = &afe_priv->etdm_data[i];
			if (etdm->sync_source[j] == sync_source)
				mt8532_afe_disable_etdm(afe, i, j);
		}
	}
}

static void mt8532_afe_enable_etdm_sync_sources(struct mtk_base_afe *afe,
	unsigned int etdm_set,
	unsigned int etdm_stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm;
	unsigned int sync_source;
	unsigned int i, j, ret;

	etdm = &afe_priv->etdm_data[etdm_set];

	sync_source = etdm->sync_source[etdm_stream];
	if (sync_source == MT8532_ETDM_SYNC_NONE)
		return;

	ret = mt8532_afe_derive_etdm_info(sync_source, &i, &j);
	if (ret == 0)
		mt8532_afe_enable_etdm(afe, i, j);
}

static void mt8532_afe_disable_etdm_sync_sources(struct mtk_base_afe *afe,
	unsigned int etdm_set,
	unsigned int etdm_stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm;
	unsigned int sync_source;
	unsigned int i, j, ret;

	etdm = &afe_priv->etdm_data[etdm_set];

	sync_source = etdm->sync_source[etdm_stream];
	if (sync_source == MT8532_ETDM_SYNC_NONE)
		return;

	ret = mt8532_afe_derive_etdm_info(sync_source, &i, &j);
	if (ret == 0)
		mt8532_afe_disable_etdm(afe, i, j);
}

static int mt8532_afe_configure_etdm_out_dsd(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int dsd_bit, unsigned int dsd_clk,
	unsigned int dsd_ch)
{
	unsigned int stream = SNDRV_PCM_STREAM_PLAYBACK;
	unsigned int ctrl_reg;

	// set format dsd
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	regmap_update_bits(afe->regmap, ctrl_reg, 0x7 << 6, 0x6 << 6);

	// set ch
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	regmap_update_bits(afe->regmap, ctrl_reg, 0x1 << 23, 0x1 << 23);

	// set dsd ch
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con5;
	regmap_update_bits(afe->regmap, ctrl_reg, 0xf << 2, (dsd_ch-1) << 2);

	// set dsd bitnum
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con5;
	regmap_update_bits(afe->regmap, ctrl_reg, 0x3 << 0, dsd_bit << 0);

	if (dsd_bit == DSD_16_BIT || dsd_bit == DSD_32_BIT) {
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 11, 0xf << 11);
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 16, 0xf << 16);
	} else if (dsd_bit == DSD_8_BIT) {
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 11, 0xf << 11);
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 16, 0xf << 16);
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con1;
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 0, 0x1f << 0);
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 5, 0x1f << 5);
	} else if (dsd_bit == DSD_24_BIT) {
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con5;
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1 << 11, 0x1 << 11);
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 11, 0xb << 11);
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 16, 0xb << 16);
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con1;
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 0, 0x1f << 0);
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f << 5, 0x1f << 5);
	}
	// disable sync mod & set master
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	regmap_update_bits(afe->regmap, ctrl_reg, 0x1 << 1, 0);
	regmap_update_bits(afe->regmap, ctrl_reg, 0x1 << 5, 0);

	if (dsd_clk == DSD_2P8M) {
		// manual mod & bck 2.8244m = 44.1k * 64
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
		regmap_update_bits(afe->regmap, ctrl_reg, 0x1<<10, 0);
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f, 0x12);//44.1k
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x7<<6, 0x1<<6); //a2sys
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x3ff<<14, 0xe7<<14); //8*8
	} else if (dsd_clk == DSD_5P6M) {
		// manual mod & bck 2.8244m = 44.1k * 128
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
		regmap_update_bits(afe->regmap, ctrl_reg, 0x1<<10, 0);
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f, 0x12); //44.1k
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x7<<6, 0x1<<6); //a2sys
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x3ff<<14, 0xef<<14); //8*16
	} else if (dsd_clk == DSD_11P2M) {
		// manual mod & bck 2.8244m = 44.1k * 256
		ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
		regmap_update_bits(afe->regmap, ctrl_reg, 0x1<<10, 0);
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x1f, 0x12); //44.1k
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x7<<6, 0x1<<6); //a2sys
		regmap_update_bits(afe->regmap, ctrl_reg,
				0x3ff<<14, 0x1ef<<14); //16*16
	}
	// relatch fs
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	regmap_update_bits(afe->regmap, ctrl_reg, 0x3<<28, 0); //a1&a2 timing
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
	regmap_update_bits(afe->regmap, ctrl_reg,
			0x1f<<24, 0x9<<24); //etdmout1 1x en
	return 0;
}

static int mt8532_afe_configure_etdm_out(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int rate,
	unsigned int channels, unsigned int bit_width)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	unsigned int tdm_channels;
	unsigned int stream = SNDRV_PCM_STREAM_PLAYBACK;
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int data_mode = etdm_data->data_mode[stream];
	unsigned int lrck_width = etdm_data->lrck_width[stream];
	bool slave_mode = etdm_data->slave_mode[stream];
	bool lrck_inv;
	bool bck_inv;
	unsigned int fmt = etdm_data->format[stream];
	unsigned int ctrl_reg;
	unsigned int ctrl_mask;
	unsigned int val = 0;
	unsigned int bck;
	unsigned int apll_sel = 1;
	unsigned int con_rel_fs, con_rel_apll_sel;

	unsigned int dai_id = (etdm_set == MT8532_ETDM1) ?
			MT8532_AFE_IO_ETDM1_OUT : (etdm_set == MT8532_ETDM2 ?
			MT8532_AFE_IO_ETDM2_OUT : MT8532_AFE_IO_ETDM3_OUT);
	struct mt8532_be_dai_data *be_data = &afe_priv->be_data[dai_id];

	tdm_channels = (data_mode == MT8532_ETDM_DATA_ONE_PIN) ?
		mt8532_afe_tdm_ch_fixup(channels) : 2;

	// set con0
	bck = rate * tdm_channels * bit_width;
	lrck_inv = etdm_data->lrck_inv[stream] ^
		etdm_data->int_lrck_inv[stream];
	bck_inv = etdm_data->bck_inv[stream] ^
		etdm_data->int_bck_inv[stream];
	val |= ETDM_CON0_BIT_LEN(bit_width);
	val |= ETDM_CON0_WORD_LEN(bit_width);
	val |= ETDM_CON0_FORMAT(fmt);
	val |= ETDM_CON0_CH_NUM(tdm_channels);
	if (slave_mode) {
		val |= ETDM_CON0_SLAVE_MODE;
	} else {
		con_rel_apll_sel = be_data->fs_1xen_sys == FS_ENGEN_A3SYS ? 1 :
			(be_data->fs_1xen_sys == FS_ENGEN_A4SYS ? 2 : 0);
		val |= ETDM_OUT_CON0_REL_APLL_SEL(con_rel_apll_sel);
	}
	if (out_sync_source != MT8532_ETDM_SYNC_NONE)
		val |= ETDM_CON0_SYNC_MODE;
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	ctrl_mask = ETDM_OUT_CON0_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	// set con5
	val = 0;
	if (slave_mode) {
		if (lrck_inv)
			val |= ETDM_OUT_CON5_SLAVE_LRCK_INV;
		if (bck_inv)
			val |= ETDM_OUT_CON5_SLAVE_BCK_INV;
	} else {
		if (lrck_inv)
			val |= ETDM_OUT_CON5_MASTER_LRCK_INV;
		if (bck_inv)
			val |= ETDM_OUT_CON5_MASTER_BCK_INV;
	}
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con5;
	ctrl_mask = ETDM_OUT_CON5_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	// set con1
	val = 0;
	val |= ETDM_CON1_LRCK_MANUAL_MODE;
	if (!slave_mode) {
		val |= ETDM_CON1_MCLK_OUTPUT;
		if (bck > MT8532_ETDM_NORMAL_MAX_BCK_RATE)
			val |= ETDM_CON1_BCK_FROM_DIVIDER;
	}
	if (lrck_width > 0)
		val |= ETDM_OUT_CON1_LRCK_WIDTH(lrck_width);
	else
		val |= ETDM_OUT_CON1_LRCK_WIDTH(bit_width);
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con1;
	ctrl_mask = ETDM_OUT_CON1_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	// set con4
	val = 0;
	if (!slave_mode) {
		apll_sel =  be_data->fs_1xen_sys == FS_ENGEN_26M ? 0 :
			(be_data->fs_1xen_sys == FS_ENGEN_A3SYS ?
			2 : (be_data->fs_1xen_sys == FS_ENGEN_A4SYS ? 3 : 1));
		val |= ETDM_OUT_CON4_APLL_SEL(apll_sel);
		val |= ETDM_OUT_CON4_FS(tdm_fs(mt8532_afe_fs_timing(rate)));
		con_rel_fs = rate;
		val |= ETDM_OUT_CON4_CONN_FS(mt8532_afe_fs_timing(con_rel_fs));
	} else {
		con_rel_fs = etdm_set == MT8532_ETDM1 ?
			MT8532_FS_ETDMOUT1_1X_EN : (etdm_set == MT8532_ETDM2 ?
			MT8532_FS_ETDMOUT2_1X_EN : MT8532_FS_ETDMOUT3_1X_EN);
		val |= ETDM_OUT_CON4_CONN_FS(con_rel_fs);
	}
	ctrl_mask = ETDM_OUT_CON4_CTRL_MASK;
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	if (etdm_set == MT8532_ETDM1) {
		val = 0;
		ctrl_reg = ETDM_COWORK_CON0;
		ctrl_mask = ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_MASK;
		if (out_sync_source == MT8532_ETDM_SYNC_FROM_IN1)
			val |= ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_IN1;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_IN2)
			val |= ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_IN2;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_OUT2)
			val |= ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_OUT2;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_OUT3)
			val |= ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_OUT3;
		if (out_sync_source != MT8532_ETDM_SYNC_NONE)
			regmap_update_bits(afe->regmap,
				ctrl_reg, ctrl_mask, val);
	} else if (etdm_set == MT8532_ETDM2) {
		val = 0;
		ctrl_reg = ETDM_COWORK_CON2;
		ctrl_mask |= ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_MASK;
		if (out_sync_source == MT8532_ETDM_SYNC_FROM_IN1)
			val |= ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_IN1;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_IN2)
			val |= ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_IN2;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_OUT1)
			val |= ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_OUT1;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_OUT3)
			val |= ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_OUT3;
		if (out_sync_source != MT8532_ETDM_SYNC_NONE)
			regmap_update_bits(afe->regmap,
				ctrl_reg, ctrl_mask, val);
	} else if (etdm_set == MT8532_ETDM3) {
		val = 0;
		ctrl_reg = ETDM_COWORK_CON2;
		ctrl_mask = ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_MASK;
		if (out_sync_source == MT8532_ETDM_SYNC_FROM_IN1)
			val |= ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_IN1;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_IN2)
			val |= ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_IN2;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_OUT1)
			val |= ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_OUT1;
		else if (out_sync_source == MT8532_ETDM_SYNC_FROM_OUT2)
			val |= ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_OUT2;
		if (out_sync_source != MT8532_ETDM_SYNC_NONE)
			regmap_update_bits(afe->regmap,
				ctrl_reg, ctrl_mask, val);
	}
	return 0;
}

static int mt8532_afe_configure_etdm_in(struct mtk_base_afe *afe,
	unsigned int etdm_set, unsigned int rate,
	unsigned int channels, unsigned int bit_width)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data =
		&afe_priv->etdm_data[etdm_set];
	unsigned int tdm_channels;
	unsigned int stream = SNDRV_PCM_STREAM_CAPTURE;
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int data_mode = etdm_data->data_mode[stream];
	unsigned int lrck_width = etdm_data->lrck_width[stream];
	bool slave_mode = etdm_data->slave_mode[stream];
	bool lrck_inv;
	bool bck_inv;
	unsigned int fmt = etdm_data->format[stream];
	unsigned int ctrl_reg;
	unsigned int ctrl_mask;
	unsigned int val = 0;
	unsigned int bck;
	unsigned int i;
	unsigned int apll_sel = 1;
	unsigned int con_rel_fs, con_rel_apll_sel;
	unsigned int dai_id = (etdm_set == MT8532_ETDM1) ?
		MT8532_AFE_IO_ETDM1_IN : MT8532_AFE_IO_ETDM2_IN;
	struct mt8532_be_dai_data *be_data = &afe_priv->be_data[dai_id];

	tdm_channels = (data_mode == MT8532_ETDM_DATA_ONE_PIN) ?
		mt8532_afe_tdm_ch_fixup(channels) : 2;
	// set con0
	val |= ETDM_CON0_BIT_LEN(bit_width);
	val |= ETDM_CON0_WORD_LEN(bit_width);
	val |= ETDM_CON0_FORMAT(fmt);
	val |= ETDM_CON0_CH_NUM(tdm_channels);
	if (slave_mode) {
		val |= ETDM_CON0_SLAVE_MODE;
	} else {
		con_rel_apll_sel = be_data->fs_1xen_sys == FS_ENGEN_A3SYS ? 1 :
			(be_data->fs_1xen_sys == FS_ENGEN_A4SYS) ? 2 : 0;
		val |= ETDM_IN_CON0_REL_APLL_SEL(con_rel_apll_sel);
	}
	if (in_sync_source != MT8532_ETDM_SYNC_NONE)
		val |= ETDM_CON0_SYNC_MODE;
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
	ctrl_mask = ETDM_IN_CON0_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	// set con4
	bck = rate * tdm_channels * bit_width;
	lrck_inv = etdm_data->lrck_inv[stream] ^
		etdm_data->int_lrck_inv[stream];
	bck_inv = etdm_data->bck_inv[stream] ^
		etdm_data->int_bck_inv[stream];
	val = 0;
	if (slave_mode) {
		con_rel_fs = (etdm_set == MT8532_ETDM1) ?
			MT8532_FS_ETDMIN1_1X_EN : MT8532_FS_ETDMIN2_1X_EN;
		val |= ETDM_IN_CON4_CONN_FS(con_rel_fs);
		if (lrck_inv)
			val |= ETDM_IN_CON4_SLAVE_LRCK_INV;
		if (bck_inv)
			val |= ETDM_IN_CON4_SLAVE_BCK_INV;
	} else {
		con_rel_fs = rate;
		val |= ETDM_IN_CON4_CONN_FS(mt8532_afe_fs_timing(con_rel_fs));
		if (lrck_inv)
			val |= ETDM_IN_CON4_MASTER_LRCK_INV;
		if (bck_inv)
			val |= ETDM_IN_CON4_MASTER_BCK_INV;
	}
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
	ctrl_mask = ETDM_IN_CON4_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	// set con1
	val = 0;
	val |= ETDM_IN_CON1_LRCK_MANUAL_MODE;
	if (!slave_mode)
		val |= ETDM_CON1_MCLK_OUTPUT;
	else if (bck > MT8532_ETDM_NORMAL_MAX_BCK_RATE)
		val |= ETDM_CON1_BCK_FROM_DIVIDER;
	if (lrck_width > 0)
		val |= ETDM_IN_CON1_LRCK_WIDTH(lrck_width);
	else
		val |= ETDM_IN_CON1_LRCK_WIDTH(bit_width);
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con1;
	ctrl_mask = ETDM_IN_CON1_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	// set con3
	val = 0;
	if (!slave_mode) {
		val = mt8532_afe_fs_timing(rate);
		val = tdm_fs(val);
		val = ETDM_IN_CON3_FS(val);
	}
	for (i = 0; i < channels; i += 2) {
		if (etdm_data->in_disable_ch[i] &&
			etdm_data->in_disable_ch[i+1])
			val |= ETDM_IN_CON3_DISABLE_OUT(i >> 1);
	}
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con3;
	ctrl_mask = ETDM_IN_CON3_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	// set con2
	val = 0;
	if (!slave_mode) {
		apll_sel = (be_data->fs_1xen_sys == FS_ENGEN_26M) ?
			0 : (be_data->fs_1xen_sys == FS_ENGEN_A3SYS ?
			2 : (be_data->fs_1xen_sys == FS_ENGEN_A4SYS ? 3 : 1));
		val |= ETDM_IN_CON2_APLL_SEL(apll_sel);
	}
	if (data_mode == MT8532_ETDM_DATA_MULTI_PIN) {
		val |= ETDM_IN_CON2_MULTI_IP_2CH_MODE |
		       ETDM_IN_CON2_MULTI_IP_TOTAL_CH(channels);
	}
	val |= ETDM_IN_CON2_UPDATE_POINT_AUTO_DIS |
		ETDM_IN_CON2_UPDATE_POINT(1);
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con2;
	ctrl_mask = ETDM_IN_CON2_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	// set con5
	val = 0;
	for (i = 0; i < channels; i += 2) {
		if (etdm_data->in_disable_ch[i] &&
		    !etdm_data->in_disable_ch[i+1])
			val |= ETDM_IN_CON5_LR_SWAP(i >> 1);
		else if (etdm_data->in_disable_ch[i] ||
				 etdm_data->in_disable_ch[i+1])
			val |= ETDM_IN_CON5_ENABLE_ODD(i >> 1);
	}
	ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con5;
	ctrl_mask = ETDM_IN_CON5_CTRL_MASK;
	regmap_update_bits(afe->regmap, ctrl_reg, ctrl_mask, val);

	if (etdm_set == MT8532_ETDM1) {
		val = 0;
		ctrl_reg = ETDM_COWORK_CON1;
		ctrl_mask = ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_MASK;
		if (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1)
			val |= ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_OUT1;
		else if (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2)
			val |= ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_OUT2;
		else if (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT3)
			val |= ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_OUT3;
		else if (in_sync_source == MT8532_ETDM_SYNC_FROM_IN2)
			val |= ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_IN2;
		if (in_sync_source != MT8532_ETDM_SYNC_NONE)
			regmap_update_bits(afe->regmap,
				ctrl_reg, ctrl_mask, val);
	} else if (etdm_set == MT8532_ETDM2) {
		val = 0;
		ctrl_reg = ETDM_COWORK_CON2;
		ctrl_mask = ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_MASK;
		if (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1)
			val |= ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_OUT1;
		else if (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2)
			val |= ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_OUT2;
		else if (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT3)
			val |= ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_OUT3;
		else if (in_sync_source == MT8532_ETDM_SYNC_FROM_IN1)
			val |= ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_IN1;
		if (in_sync_source != MT8532_ETDM_SYNC_NONE)
			regmap_update_bits(afe->regmap,
				ctrl_reg, ctrl_mask, val);
	}
	return 0;
}

static void mt8532_afe_etdm1_out_force_on(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm1 = &priv->etdm_data[MT8532_ETDM1];
	unsigned int mclk, rate;

	mclk = etdm1->mclk_freq[SNDRV_PCM_STREAM_PLAYBACK];
	rate = etdm1->force_rate[SNDRV_PCM_STREAM_PLAYBACK];

	if (mclk == 0)
		mclk = MT8532_ETDM1_OUT_MCLK_MULTIPLIER * rate;

	mt8532_afe_enable_clk(afe, priv->clocks[MT8532_CLK_TOP_APLL12_DIV3]);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_TDM_OUT);

	mt8532_afe_configure_etdm_out(afe,
		MT8532_ETDM1,
		rate,
		etdm1->force_channels[SNDRV_PCM_STREAM_PLAYBACK],
		etdm1->force_bit_width[SNDRV_PCM_STREAM_PLAYBACK]);

	mt8532_afe_set_clk_parent(afe,
		priv->clocks[MT8532_CLK_TOP_I2SO2_SEL],
		(rate % 8000) ?
		priv->clocks[MT8532_CLK_APMIXED_APLL2] :
		priv->clocks[MT8532_CLK_APMIXED_APLL1]);

	mt8532_afe_set_clk_rate(afe,
		priv->clocks[MT8532_CLK_TOP_I2SO2_M],
		mclk);

	mt8532_afe_enable_etdm(afe, MT8532_ETDM1, SNDRV_PCM_STREAM_PLAYBACK);

	etdm1->force_on_status[SNDRV_PCM_STREAM_PLAYBACK] = true;
}

static void mt8532_afe_etdm1_in_force_on(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm1 = &priv->etdm_data[MT8532_ETDM1];
	unsigned int mclk, rate;

	mclk = etdm1->mclk_freq[SNDRV_PCM_STREAM_CAPTURE];
	rate = etdm1->force_rate[SNDRV_PCM_STREAM_CAPTURE];

	if (mclk == 0)
		mclk = MT8532_ETDM1_IN_MCLK_MULTIPLIER * rate;

	mt8532_afe_enable_clk(afe, priv->clocks[MT8532_CLK_TOP_APLL12_DIV1]);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_TDM_IN);

	mt8532_afe_configure_etdm_in(afe,
		MT8532_ETDM1,
		rate,
		etdm1->force_channels[SNDRV_PCM_STREAM_CAPTURE],
		etdm1->force_bit_width[SNDRV_PCM_STREAM_CAPTURE]);

	mt8532_afe_set_clk_parent(afe,
		priv->clocks[MT8532_CLK_TOP_I2SI2_SEL],
		(rate % 8000) ?
		priv->clocks[MT8532_CLK_APMIXED_APLL2] :
		priv->clocks[MT8532_CLK_APMIXED_APLL1]);

	mt8532_afe_set_clk_rate(afe,
		priv->clocks[MT8532_CLK_TOP_I2SI2_M],
		mclk);

	mt8532_afe_enable_etdm(afe, MT8532_ETDM1, SNDRV_PCM_STREAM_CAPTURE);

	etdm1->force_on_status[SNDRV_PCM_STREAM_CAPTURE] = true;
}

static void mt8532_afe_etdm2_out_force_on(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm2 = &priv->etdm_data[MT8532_ETDM2];
	unsigned int mclk, rate;

	mclk = etdm2->mclk_freq[SNDRV_PCM_STREAM_PLAYBACK];
	rate = etdm2->force_rate[SNDRV_PCM_STREAM_PLAYBACK];

	if (mclk == 0)
		mclk = MT8532_ETDM2_OUT_MCLK_MULTIPLIER * rate;

	mt8532_afe_enable_clk(afe, priv->clocks[MT8532_CLK_TOP_APLL12_DIV2]);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_I2S_OUT);

	mt8532_afe_configure_etdm_out(afe,
		MT8532_ETDM2,
		rate,
		etdm2->force_channels[SNDRV_PCM_STREAM_PLAYBACK],
		etdm2->force_bit_width[SNDRV_PCM_STREAM_PLAYBACK]);

	mt8532_afe_set_clk_parent(afe,
		priv->clocks[MT8532_CLK_TOP_I2SO1_SEL],
		(rate % 8000) ?
		priv->clocks[MT8532_CLK_APMIXED_APLL2] :
		priv->clocks[MT8532_CLK_APMIXED_APLL1]);

	mt8532_afe_set_clk_rate(afe,
		priv->clocks[MT8532_CLK_TOP_I2SO1_M],
		mclk);

	mt8532_afe_enable_etdm(afe, MT8532_ETDM2, SNDRV_PCM_STREAM_PLAYBACK);

	etdm2->force_on_status[SNDRV_PCM_STREAM_PLAYBACK] = true;
}

static void mt8532_afe_etdm2_in_force_on(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm2 = &priv->etdm_data[MT8532_ETDM2];
	unsigned int mclk, rate;

	mclk = etdm2->mclk_freq[SNDRV_PCM_STREAM_CAPTURE];
	rate = etdm2->force_rate[SNDRV_PCM_STREAM_CAPTURE];

	if (mclk == 0)
		mclk = MT8532_ETDM2_IN_MCLK_MULTIPLIER * rate;

	mt8532_afe_enable_clk(afe, priv->clocks[MT8532_CLK_TOP_APLL12_DIV0]);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_I2S_IN);

	mt8532_afe_configure_etdm_in(afe,
		MT8532_ETDM2,
		rate,
		etdm2->force_channels[SNDRV_PCM_STREAM_CAPTURE],
		etdm2->force_bit_width[SNDRV_PCM_STREAM_CAPTURE]);

	mt8532_afe_set_clk_parent(afe,
		priv->clocks[MT8532_CLK_TOP_I2SI1_SEL],
		(rate % 8000) ?
		priv->clocks[MT8532_CLK_APMIXED_APLL2] :
		priv->clocks[MT8532_CLK_APMIXED_APLL1]);

	mt8532_afe_set_clk_rate(afe,
		priv->clocks[MT8532_CLK_TOP_I2SI1_M],
		mclk);

	mt8532_afe_enable_etdm(afe, MT8532_ETDM2, SNDRV_PCM_STREAM_CAPTURE);

	etdm2->force_on_status[SNDRV_PCM_STREAM_CAPTURE] = true;
}

static int mt8532_afe_etdm1_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data =
		&afe_priv->etdm_data[MT8532_ETDM1];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u force %d interlink %d\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source,
		etdm_data->force_on[stream],
		etdm_data->enable_interlink[stream]);

	mt8532_afe_enable_main_clk(afe);

	if ((stream == SNDRV_PCM_STREAM_PLAYBACK) ||
	    (out_sync_source == MT8532_ETDM_SYNC_FROM_IN1) ||
	    (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1)) {
		mt8532_afe_enable_clk(afe,
			afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV3]);
		mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_TDM_OUT);
	}

	if ((stream == SNDRV_PCM_STREAM_CAPTURE) ||
	    (out_sync_source == MT8532_ETDM_SYNC_FROM_IN1) ||
	    (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1)) {
		mt8532_afe_enable_clk(afe,
			afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV1]);
		mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_TDM_IN);
	}

	return 0;
}

static void mt8532_afe_etdm1_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM1;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool reset_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN1) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1);
	bool reset_in_change = (stream == SNDRV_PCM_STREAM_CAPTURE) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN1) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1);

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source);

	if (reset_out_change)
		mt8532_afe_dec_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

	if (reset_in_change)
		mt8532_afe_dec_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_CAPTURE);


	if (etdm_data->force_on[stream])
		return;

	if (etdm_data->occupied[stream] == 0) {
		if (etdm_data->enable_seq[stream] == EN_IN_PREP_DIS_IN_SD) {
			mt8532_afe_disable_etdm_sync_sources(afe,
				etdm_set, stream);

			mt8532_afe_disable_etdm(afe, etdm_set, stream);

			mt8532_afe_disable_etdm_sync_clients(afe,
				etdm_set, stream);
		}
	}

	if (reset_out_change) {
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_TDM_OUT);
		mt8532_afe_disable_clk(afe,
			afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV3]);
	}

	if (reset_in_change) {
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_TDM_IN);
		mt8532_afe_disable_clk(afe,
			afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV1]);
	}

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_etdm1_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM1;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool apply_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN1) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1);
	bool apply_in_change = (stream == SNDRV_PCM_STREAM_CAPTURE) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN1) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1);
	unsigned int rate = params_rate(params);
	unsigned int bit_width = params_width(params);
	unsigned int channels = params_channels(params);

	unsigned int etdm_mod = etdm_data->dsd_mod[stream];
	unsigned int dsd_bit = etdm_data->dsd_bit[stream];
	unsigned int dsd_clk = etdm_data->dsd_clk[stream];

	dev_dbg(afe->dev,
		"%s '%s' rate %u ch %u bit %u period %u-%u clock %u slv %u\n",
		__func__, snd_pcm_stream_str(substream),
		rate, channels, bit_width, params_period_size(params),
		params_periods(params), etdm_data->clock_mode,
		etdm_data->slave_mode[stream]);

	dev_dbg(afe->dev,
		"%s '%s' fmt %u data %u lrck %d-%d-%u bck %d-%d\n",
		__func__, snd_pcm_stream_str(substream),
		etdm_data->format[stream], etdm_data->data_mode[stream],
		etdm_data->lrck_inv[stream], etdm_data->int_lrck_inv[stream],
		etdm_data->lrck_width[stream], etdm_data->bck_inv[stream],
		etdm_data->int_bck_inv[stream]);

	if (apply_out_change) {
		if (etdm_mod == PCM_MOD)
			ret = mt8532_afe_configure_etdm_out(afe, etdm_set,
				rate, channels, bit_width);
		else if (etdm_mod == DSD_MOD)
			ret = mt8532_afe_configure_etdm_out_dsd(afe, etdm_set,
				dsd_bit, dsd_clk, channels);
		else
			return -1;

		if (ret != 0)
			return ret;
	}

	if (apply_in_change) {
		ret = mt8532_afe_configure_etdm_in(afe, etdm_set,
			rate, channels, bit_width);
		if (ret != 0)
			return ret;
	}

	return ret;
}

int mt8532_afe_etdm1_hw_free(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{

	return 0;
}

static int mt8532_afe_etdm1_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM1;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool apply_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN1) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1);
	bool apply_in_change = (stream == SNDRV_PCM_STREAM_CAPTURE) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN1) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT1);
	unsigned int rate = dai->rate;
	unsigned int mclk = afe_priv->etdm_data[etdm_set].mclk_freq[stream];

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u seq %u occupied %d\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source,
		etdm_data->enable_seq[stream],
		etdm_data->occupied[stream]);

	if (etdm_data->occupied[stream])
		return 0;

	if (apply_out_change)
		mt8532_afe_inc_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

	if (apply_in_change)
		mt8532_afe_inc_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_CAPTURE);

	if (apply_out_change) {
		if (mclk == 0) {
			if (etdm_data->dsd_mod[stream] == DSD_MOD)
				mclk = MT8532_ETDM1_IN_MCLK_MULTIPLIER *
					2 * rate;
			else
				mclk = MT8532_ETDM1_IN_MCLK_MULTIPLIER * rate;
		}
		mt8532_afe_set_clk_parent(afe,
			afe_priv->clocks[MT8532_CLK_TOP_I2SO2_SEL],
			(rate % 8000) ?
			afe_priv->clocks[MT8532_CLK_APMIXED_APLL2] :
			afe_priv->clocks[MT8532_CLK_APMIXED_APLL1]);
		mt8532_afe_set_clk_rate(afe,
			afe_priv->clocks[MT8532_CLK_TOP_I2SO2_M],
			mclk);
	}

	if (apply_in_change) {
		if (mclk == 0)
			mclk = MT8532_ETDM1_IN_MCLK_MULTIPLIER * rate;

		mt8532_afe_set_clk_parent(afe,
			afe_priv->clocks[MT8532_CLK_TOP_I2SI2_SEL],
			(rate % 8000) ?
			afe_priv->clocks[MT8532_CLK_APMIXED_APLL2] :
			afe_priv->clocks[MT8532_CLK_APMIXED_APLL1]);
		mt8532_afe_set_clk_rate(afe,
			afe_priv->clocks[MT8532_CLK_TOP_I2SI2_M],
			mclk);
	}

	if (etdm_data->enable_seq[stream] == EN_IN_PREP_DIS_IN_SD) {
		mt8532_afe_enable_etdm_sync_clients(afe, etdm_set, stream);

		mt8532_afe_enable_etdm(afe, etdm_set, stream);

		mt8532_afe_enable_etdm_sync_sources(afe, etdm_set, stream);
	}

	return 0;
}

static void mt8532_afe_check_and_wait_dl8_dlm_sync(struct mtk_base_afe *afe)
{
	struct mtk_base_afe_memif *memif = &afe->memif[MT8532_AFE_MEMIF_DL10];
	struct snd_pcm_runtime *runtime = memif->substream->runtime;
	unsigned int dl8_base = 0, dl8_cur = 0, dlm_base = 0, dlm_cur = 0;
	unsigned int aux_frame_bytes = 0, main_frame_bytes = 0;
	int check_count = 20;

	if (memif->aux_channels == 0)
		return;

	aux_frame_bytes = memif->aux_channels * samples_to_bytes(runtime, 1);

	main_frame_bytes = (runtime->channels - memif->aux_channels) *
		samples_to_bytes(runtime, 1);

	regmap_read(afe->regmap, AFE_DL8_BASE, &dl8_base);
	regmap_read(afe->regmap, AFE_DL10_BASE, &dlm_base);

	while (check_count--) {
		regmap_read(afe->regmap, AFE_DL8_CUR, &dl8_cur);
		regmap_read(afe->regmap, AFE_DL10_CUR, &dlm_cur);

		if (((dl8_cur - dl8_base) >= main_frame_bytes) &&
		    ((dlm_cur - dlm_base) >= aux_frame_bytes))
			return;

		udelay(10);
	};
}

static int mt8532_afe_etdm1_trigger(struct snd_pcm_substream *substream,
	int cmd,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM1;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int stream = substream->stream;
	unsigned int sync_source = etdm_data->sync_source[stream];
	unsigned int enable_seq = etdm_data->enable_seq[stream];
	int ret = 0;

	dev_dbg(dai->dev, "%s '%s' cmd %d\n", __func__,
		snd_pcm_stream_str(substream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (!etdm_data->force_on[stream] &&
		    (enable_seq == EN_DIS_IN_TRIGGER) &&
		    (sync_source == MT8532_ETDM_SYNC_NONE)) {
			if ((stream == SNDRV_PCM_STREAM_PLAYBACK) &&
			    afe_priv->dl8_enable_24ch_output)
				mt8532_afe_check_and_wait_dl8_dlm_sync(afe);

			mt8532_afe_enable_etdm_sync_clients(afe,
				etdm_set, stream);

			mt8532_afe_enable_etdm(afe, etdm_set, stream);
		}
		if (mt8532_afe_is_clk_tune_output(afe, dai->id)) {
			mt8532_afe_set_be_active(afe, dai->id);
			mt8532_afe_set_clk_tune_output_rate(afe, dai->rate);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (!etdm_data->force_on[stream] &&
		    (enable_seq == EN_DIS_IN_TRIGGER) &&
		    (sync_source == MT8532_ETDM_SYNC_NONE)) {
			mt8532_afe_disable_etdm(afe, etdm_set, stream);

			mt8532_afe_disable_etdm_sync_clients(afe,
				etdm_set, stream);
		}

		if (mt8532_afe_is_clk_tune_output(afe, dai->id)) {
			mt8532_afe_clear_be_active(afe, dai->id);
			mt8532_afe_set_clk_tune_output_rate(afe, 0);
		}
		break;
	default:
		break;
	}

	return ret;
}

static int mt8532_afe_etdm2_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data =
		&afe_priv->etdm_data[MT8532_ETDM2];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u force %d interlink %d\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source,
		etdm_data->force_on[stream],
		etdm_data->enable_interlink[stream]);


	mt8532_afe_enable_main_clk(afe);

	if ((stream == SNDRV_PCM_STREAM_PLAYBACK) ||
	    (out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
	    (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2)) {
		mt8532_afe_enable_clk(afe,
			afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV2]);
		mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_I2S_OUT);
	}

	if ((stream == SNDRV_PCM_STREAM_CAPTURE) ||
	    (out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
	    (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2)) {
		mt8532_afe_enable_clk(afe,
			afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV0]);
		mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_I2S_IN);
	}

	return 0;
}

static void mt8532_afe_etdm2_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM2;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool reset_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);
	bool reset_in_change = (stream == SNDRV_PCM_STREAM_CAPTURE) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source);

	if (reset_out_change)
		mt8532_afe_dec_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

	if (reset_in_change)
		mt8532_afe_dec_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_CAPTURE);

	if (etdm_data->occupied[stream] == 0) {
		if (etdm_data->enable_seq[stream] == EN_IN_PREP_DIS_IN_SD) {
			mt8532_afe_disable_etdm_sync_sources(afe,
				etdm_set, stream);

			mt8532_afe_disable_etdm(afe, etdm_set, stream);

			mt8532_afe_disable_etdm_sync_clients(afe,
				etdm_set, stream);
		}
	}

	if (reset_out_change) {
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_I2S_OUT);
		mt8532_afe_disable_clk(afe,
			afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV2]);
	}

	if (reset_in_change) {
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_I2S_IN);
		mt8532_afe_disable_clk(afe,
			afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV0]);
	}

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_etdm2_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM2;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool apply_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);
	bool apply_in_change = (stream == SNDRV_PCM_STREAM_CAPTURE) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);
	unsigned int rate = params_rate(params);
	unsigned int bit_width = params_width(params);
	unsigned int channels = params_channels(params);

	dev_dbg(afe->dev,
		"%s '%s' rate %u ch %u bit %u period %u-%u clock %u slv %u\n",
		__func__, snd_pcm_stream_str(substream),
		rate, channels, bit_width, params_period_size(params),
		params_periods(params), etdm_data->clock_mode,
		etdm_data->slave_mode[stream]);

	dev_dbg(afe->dev,
		"%s '%s' fmt %u data %u lrck %d-%d-%u bck %d-%d\n",
		__func__, snd_pcm_stream_str(substream),
		etdm_data->format[stream], etdm_data->data_mode[stream],
		etdm_data->lrck_inv[stream], etdm_data->int_lrck_inv[stream],
		etdm_data->lrck_width[stream], etdm_data->bck_inv[stream],
		etdm_data->int_bck_inv[stream]);

	if (apply_out_change) {
		ret = mt8532_afe_configure_etdm_out(afe, etdm_set,
			rate, channels, bit_width);
		if (ret != 0)
			return ret;
	}

	if (apply_in_change) {
		ret = mt8532_afe_configure_etdm_in(afe, etdm_set,
			rate, channels, bit_width);
		if (ret != 0)
			return ret;
	}

	return ret;
}

int mt8532_afe_etdm2_hw_free(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "%s\n", __func__);
	return 0;
}

static int mt8532_afe_etdm2_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM2;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool apply_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);
	bool apply_in_change = (stream == SNDRV_PCM_STREAM_CAPTURE) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);
	unsigned int rate = dai->rate;
	unsigned int mclk = afe_priv->etdm_data[etdm_set].mclk_freq[stream];

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u seq %u occupied %d\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source,
		etdm_data->enable_seq[stream],
		etdm_data->occupied[stream]);

	if (etdm_data->occupied[stream])
		return 0;

	if (apply_out_change)
		mt8532_afe_inc_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

	if (apply_in_change)
		mt8532_afe_inc_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_CAPTURE);


	if (apply_out_change) {
		if (mclk == 0)
			mclk = MT8532_ETDM2_OUT_MCLK_MULTIPLIER * rate;

		mt8532_afe_set_clk_parent(afe,
			afe_priv->clocks[MT8532_CLK_TOP_I2SO1_SEL],
			(rate % 8000) ?
			afe_priv->clocks[MT8532_CLK_APMIXED_APLL2] :
			afe_priv->clocks[MT8532_CLK_APMIXED_APLL1]);
		mt8532_afe_set_clk_rate(afe,
			afe_priv->clocks[MT8532_CLK_TOP_I2SO1_M],
			mclk);
	}

	if (apply_in_change) {
		if (mclk == 0)
			mclk = MT8532_ETDM2_IN_MCLK_MULTIPLIER * rate;

		mt8532_afe_set_clk_parent(afe,
			afe_priv->clocks[MT8532_CLK_TOP_I2SI1_SEL],
			(rate % 8000) ?
			afe_priv->clocks[MT8532_CLK_APMIXED_APLL2] :
			afe_priv->clocks[MT8532_CLK_APMIXED_APLL1]);
		mt8532_afe_set_clk_rate(afe,
			afe_priv->clocks[MT8532_CLK_TOP_I2SI1_M],
			mclk);
	}

	if (etdm_data->enable_seq[stream] == EN_IN_PREP_DIS_IN_SD) {
		mt8532_afe_enable_etdm_sync_clients(afe, etdm_set, stream);

		mt8532_afe_enable_etdm(afe, etdm_set, stream);

		mt8532_afe_enable_etdm_sync_sources(afe, etdm_set, stream);
	}


	return 0;
}

static int mt8532_afe_etdm2_trigger(struct snd_pcm_substream *substream,
	int cmd,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM2;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int stream = substream->stream;
	unsigned int sync_source = etdm_data->sync_source[stream];
	unsigned int enable_seq = etdm_data->enable_seq[stream];
	int ret = 0;

	dev_dbg(dai->dev, "%s '%s' cmd %d\n", __func__,
		snd_pcm_stream_str(substream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (!etdm_data->force_on[stream] &&
		    (enable_seq == EN_DIS_IN_TRIGGER) &&
		    (sync_source == MT8532_ETDM_SYNC_NONE)) {
			mt8532_afe_enable_etdm_sync_clients(afe,
				etdm_set, stream);

			mt8532_afe_enable_etdm(afe, etdm_set, stream);
		}

		if (mt8532_afe_is_clk_tune_output(afe, dai->id)) {
			mt8532_afe_set_be_active(afe, dai->id);
			mt8532_afe_set_clk_tune_output_rate(afe, dai->rate);
#ifdef CALI_RESULT_CHECK
			mt8532_afe_trigger_cali_res_hrt(afe);
#endif
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:

		if (!etdm_data->force_on[stream] &&
		    (enable_seq == EN_DIS_IN_TRIGGER) &&
		    (sync_source == MT8532_ETDM_SYNC_NONE)) {
			mt8532_afe_disable_etdm(afe, etdm_set, stream);

			mt8532_afe_disable_etdm_sync_clients(afe,
				etdm_set, stream);
		}

		if (mt8532_afe_is_clk_tune_output(afe, dai->id)) {
#ifdef CALI_RESULT_CHECK
			mt8532_afe_stop_cali_res_hrt(afe);
#endif
			mt8532_afe_clear_be_active(afe, dai->id);
			mt8532_afe_set_clk_tune_output_rate(afe, 0);
		}
		break;
	default:
		break;
	}

	return ret;
}

static int mt8532_afe_etdm3_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm_data =
		&afe_priv->etdm_data[MT8532_ETDM3];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u force %d interlink %d\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source,
		etdm_data->force_on[stream],
		etdm_data->enable_interlink[stream]);

	mt8532_afe_enable_main_clk(afe);

	if ((stream == SNDRV_PCM_STREAM_PLAYBACK) ||
	    (in_sync_source == MT8532_ETDM_SYNC_FROM_OUT3)) {
		mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_HDMI_OUT);
	}

	return 0;
}

static void mt8532_afe_etdm3_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM3;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool reset_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source);

	if (reset_out_change)
		mt8532_afe_dec_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

	if (etdm_data->force_on[stream])
		return;

	if (etdm_data->occupied[stream] == 0) {
		if (etdm_data->enable_seq[stream] == EN_IN_PREP_DIS_IN_SD) {
			mt8532_afe_disable_etdm_sync_sources(afe,
				etdm_set, stream);

			mt8532_afe_disable_etdm(afe, etdm_set, stream);

			mt8532_afe_disable_etdm_sync_clients(afe,
				etdm_set, stream);
		}
	}

	if (reset_out_change)
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_HDMI_OUT);

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_etdm3_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM3;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool apply_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);
	unsigned int rate = params_rate(params);
	unsigned int bit_width = params_width(params);
	unsigned int channels = params_channels(params);

	dev_dbg(afe->dev,
		"%s '%s' rate %u ch %u bit %u period %u-%u clock %u slv %u\n",
		__func__, snd_pcm_stream_str(substream),
		rate, channels, bit_width, params_period_size(params),
		params_periods(params), etdm_data->clock_mode,
		etdm_data->slave_mode[stream]);

	dev_dbg(afe->dev,
		"%s '%s' fmt %u data %u lrck %d-%d-%u bck %d-%d\n",
		__func__, snd_pcm_stream_str(substream),
		etdm_data->format[stream], etdm_data->data_mode[stream],
		etdm_data->lrck_inv[stream], etdm_data->int_lrck_inv[stream],
		etdm_data->lrck_width[stream], etdm_data->bck_inv[stream],
		etdm_data->int_bck_inv[stream]);

	if (apply_out_change) {
		ret = mt8532_afe_configure_etdm_out(afe, etdm_set,
			rate, channels, bit_width);
		if (ret != 0)
			return ret;
	}

	return ret;
}

int mt8532_afe_etdm3_hw_free(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static int mt8532_afe_etdm3_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM3;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int out_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_PLAYBACK];
	unsigned int in_sync_source =
		etdm_data->sync_source[SNDRV_PCM_STREAM_CAPTURE];
	unsigned int stream = substream->stream;
	bool apply_out_change = (stream == SNDRV_PCM_STREAM_PLAYBACK) ||
		(out_sync_source == MT8532_ETDM_SYNC_FROM_IN2) ||
		(in_sync_source == MT8532_ETDM_SYNC_FROM_OUT2);

	dev_dbg(dai->dev, "%s '%s' sync_src %u-%u seq %u occupied %d\n",
		__func__, snd_pcm_stream_str(substream),
		out_sync_source, in_sync_source,
		etdm_data->enable_seq[stream],
		etdm_data->occupied[stream]);

	if (etdm_data->occupied[stream])
		return 0;

	if (apply_out_change)
		mt8532_afe_inc_etdm_occupy(afe, etdm_set,
			SNDRV_PCM_STREAM_PLAYBACK);

	if (etdm_data->enable_seq[stream] == EN_IN_PREP_DIS_IN_SD) {
		mt8532_afe_enable_etdm_sync_clients(afe, etdm_set, stream);

		mt8532_afe_enable_etdm(afe, etdm_set, stream);

		mt8532_afe_enable_etdm_sync_sources(afe, etdm_set, stream);
	}

	return 0;
}

static int mt8532_afe_etdm3_trigger(struct snd_pcm_substream *substream,
	int cmd,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = MT8532_ETDM3;
	struct mt8532_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
	unsigned int stream = substream->stream;
	unsigned int sync_source = etdm_data->sync_source[stream];
	unsigned int enable_seq = etdm_data->enable_seq[stream];
	int ret = 0;

	dev_dbg(dai->dev, "%s '%s' cmd %d\n", __func__,
		snd_pcm_stream_str(substream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (!etdm_data->force_on[stream] &&
		    (enable_seq == EN_DIS_IN_TRIGGER) &&
		    (sync_source == MT8532_ETDM_SYNC_NONE)) {
			mt8532_afe_enable_etdm_sync_clients(afe,
				etdm_set, stream);

			mt8532_afe_enable_etdm(afe, etdm_set, stream);
		}

		if (mt8532_afe_is_clk_tune_output(afe, dai->id)) {
			mt8532_afe_set_be_active(afe, dai->id);
			mt8532_afe_set_clk_tune_output_rate(afe, dai->rate);
#ifdef CALI_RESULT_CHECK
			mt8532_afe_trigger_cali_res_hrt(afe);
#endif
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:

		if (!etdm_data->force_on[stream] &&
		    (enable_seq == EN_DIS_IN_TRIGGER) &&
		    (sync_source == MT8532_ETDM_SYNC_NONE)) {
			mt8532_afe_disable_etdm(afe, etdm_set, stream);

			mt8532_afe_disable_etdm_sync_clients(afe,
				etdm_set, stream);
		}

		if (mt8532_afe_is_clk_tune_output(afe, dai->id)) {
#ifdef CALI_RESULT_CHECK
			mt8532_afe_stop_cali_res_hrt(afe);
#endif
			mt8532_afe_clear_be_active(afe, dai->id);
			mt8532_afe_set_clk_tune_output_rate(afe, 0);
		}
		break;
	default:
		break;
	}

	return ret;
}

static int mt8532_afe_handle_etdm_force_on(struct mtk_base_afe *afe,
	bool probe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm1 = &priv->etdm_data[MT8532_ETDM1];
	struct mt8532_etdm_data *etdm2 = &priv->etdm_data[MT8532_ETDM2];
	bool force_apply_etdm1_out;
	bool force_apply_etdm1_in;
	bool force_apply_etdm2_out;
	bool force_apply_etdm2_in;
	bool co_clock;

	co_clock = (etdm1->sync_source[SNDRV_PCM_STREAM_PLAYBACK] ==
		    MT8532_ETDM_SYNC_FROM_IN1) ||
		   (etdm1->sync_source[SNDRV_PCM_STREAM_CAPTURE] ==
		    MT8532_ETDM_SYNC_FROM_OUT1);

	force_apply_etdm1_out =
		(etdm1->force_on[SNDRV_PCM_STREAM_PLAYBACK] &&
		 !etdm1->force_on_status[SNDRV_PCM_STREAM_PLAYBACK] &&
		 (!probe ||
		  etdm1->force_on_policy[SNDRV_PCM_STREAM_PLAYBACK] ==
		  MT8532_ETDM_FORCE_ON_DEFAULT)) ||
		(etdm1->force_on[SNDRV_PCM_STREAM_CAPTURE] &&
		 !etdm1->force_on_status[SNDRV_PCM_STREAM_CAPTURE] &&
		 (!probe ||
		  etdm1->force_on_policy[SNDRV_PCM_STREAM_CAPTURE] ==
		  MT8532_ETDM_FORCE_ON_DEFAULT) && co_clock);

	force_apply_etdm1_in =
		(etdm1->force_on[SNDRV_PCM_STREAM_CAPTURE] &&
		 !etdm1->force_on_status[SNDRV_PCM_STREAM_CAPTURE] &&
		 (!probe ||
		  etdm1->force_on_policy[SNDRV_PCM_STREAM_CAPTURE] ==
		  MT8532_ETDM_FORCE_ON_DEFAULT)) ||
		(etdm1->force_on[SNDRV_PCM_STREAM_PLAYBACK] &&
		 !etdm1->force_on_status[SNDRV_PCM_STREAM_PLAYBACK] &&
		 (!probe ||
		  etdm1->force_on_policy[SNDRV_PCM_STREAM_PLAYBACK] ==
		  MT8532_ETDM_FORCE_ON_DEFAULT) && co_clock);

	co_clock = (etdm2->sync_source[SNDRV_PCM_STREAM_PLAYBACK] ==
		    MT8532_ETDM_SYNC_FROM_IN2) ||
		   (etdm2->sync_source[SNDRV_PCM_STREAM_CAPTURE] ==
		    MT8532_ETDM_SYNC_FROM_OUT2);

	force_apply_etdm2_out =
		(etdm2->force_on[SNDRV_PCM_STREAM_PLAYBACK] &&
		 !etdm2->force_on_status[SNDRV_PCM_STREAM_PLAYBACK] &&
		 (!probe ||
		  etdm2->force_on_policy[SNDRV_PCM_STREAM_PLAYBACK] ==
		  MT8532_ETDM_FORCE_ON_DEFAULT)) ||
		(etdm2->force_on[SNDRV_PCM_STREAM_CAPTURE] &&
		 !etdm2->force_on_status[SNDRV_PCM_STREAM_CAPTURE] &&
		 (!probe ||
		  etdm2->force_on_policy[SNDRV_PCM_STREAM_CAPTURE] ==
		  MT8532_ETDM_FORCE_ON_DEFAULT) && co_clock);

	force_apply_etdm2_in =
		(etdm2->force_on[SNDRV_PCM_STREAM_CAPTURE] &&
		 !etdm2->force_on_status[SNDRV_PCM_STREAM_CAPTURE] &&
		 (!probe ||
		  etdm2->force_on_policy[SNDRV_PCM_STREAM_CAPTURE] ==
		  MT8532_ETDM_FORCE_ON_DEFAULT)) ||
		(etdm2->force_on[SNDRV_PCM_STREAM_PLAYBACK] &&
		 !etdm2->force_on_status[SNDRV_PCM_STREAM_PLAYBACK] &&
		 (!probe ||
		  etdm2->force_on_policy[SNDRV_PCM_STREAM_PLAYBACK] ==
		  MT8532_ETDM_FORCE_ON_DEFAULT) && co_clock);

	if (!force_apply_etdm1_out &&
	    !force_apply_etdm1_in &&
	    !force_apply_etdm2_out &&
	    !force_apply_etdm2_in)
		return 0;

	mt8532_afe_enable_main_clk(afe);

	if (force_apply_etdm1_out)
		mt8532_afe_etdm1_out_force_on(afe);

	if (force_apply_etdm1_in)
		mt8532_afe_etdm1_in_force_on(afe);

	if (force_apply_etdm2_out)
		mt8532_afe_etdm2_out_force_on(afe);

	if (force_apply_etdm2_in)
		mt8532_afe_etdm2_in_force_on(afe);

	return 0;
}

static int mt8532_afe_handle_etdm_force_off(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_etdm_data *etdm1 = &priv->etdm_data[MT8532_ETDM1];
	struct mt8532_etdm_data *etdm2 = &priv->etdm_data[MT8532_ETDM2];
	bool force_apply_etdm1_out;
	bool force_apply_etdm1_in;
	bool force_apply_etdm2_out;
	bool force_apply_etdm2_in;
	bool co_clock;

	co_clock = (etdm1->sync_source[SNDRV_PCM_STREAM_PLAYBACK] ==
		    MT8532_ETDM_SYNC_FROM_IN1) ||
		   (etdm1->sync_source[SNDRV_PCM_STREAM_CAPTURE] ==
		    MT8532_ETDM_SYNC_FROM_OUT1);

	force_apply_etdm1_out = etdm1->force_on[SNDRV_PCM_STREAM_PLAYBACK] ||
		(etdm1->force_on[SNDRV_PCM_STREAM_CAPTURE] && co_clock);

	force_apply_etdm1_in = etdm1->force_on[SNDRV_PCM_STREAM_CAPTURE] ||
		(etdm1->force_on[SNDRV_PCM_STREAM_PLAYBACK] && co_clock);

	co_clock = (etdm2->sync_source[SNDRV_PCM_STREAM_PLAYBACK] ==
		    MT8532_ETDM_SYNC_FROM_IN2) ||
		   (etdm2->sync_source[SNDRV_PCM_STREAM_CAPTURE] ==
		    MT8532_ETDM_SYNC_FROM_OUT2);

	force_apply_etdm2_out = etdm2->force_on[SNDRV_PCM_STREAM_PLAYBACK] ||
		(etdm2->force_on[SNDRV_PCM_STREAM_CAPTURE] && co_clock);

	force_apply_etdm2_in = etdm2->force_on[SNDRV_PCM_STREAM_CAPTURE] ||
		(etdm2->force_on[SNDRV_PCM_STREAM_PLAYBACK] && co_clock);

	if (!force_apply_etdm1_out &&
	    !force_apply_etdm1_in &&
	    !force_apply_etdm2_out &&
	    !force_apply_etdm2_in)
		return 0;

	if (force_apply_etdm1_out) {
		mt8532_afe_disable_etdm(afe, MT8532_ETDM1,
			SNDRV_PCM_STREAM_PLAYBACK);

		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_TDM_OUT);
		mt8532_afe_disable_clk(afe,
			priv->clocks[MT8532_CLK_TOP_APLL12_DIV3]);

		etdm1->force_on_status[SNDRV_PCM_STREAM_PLAYBACK] = false;
	}

	if (force_apply_etdm1_in) {
		mt8532_afe_disable_etdm(afe, MT8532_ETDM1,
			SNDRV_PCM_STREAM_CAPTURE);

		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_TDM_IN);
		mt8532_afe_disable_clk(afe,
			priv->clocks[MT8532_CLK_TOP_APLL12_DIV1]);

		etdm1->force_on_status[SNDRV_PCM_STREAM_CAPTURE] = false;
	}

	if (force_apply_etdm2_out) {
		mt8532_afe_disable_etdm(afe, MT8532_ETDM2,
			SNDRV_PCM_STREAM_PLAYBACK);

		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_I2S_OUT);
		mt8532_afe_disable_clk(afe,
			priv->clocks[MT8532_CLK_TOP_APLL12_DIV2]);

		etdm2->force_on_status[SNDRV_PCM_STREAM_PLAYBACK] = false;
	}

	if (force_apply_etdm2_in) {
		mt8532_afe_disable_etdm(afe, MT8532_ETDM2,
			SNDRV_PCM_STREAM_CAPTURE);

		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_I2S_IN);
		mt8532_afe_disable_clk(afe,
			priv->clocks[MT8532_CLK_TOP_APLL12_DIV0]);

		etdm2->force_on_status[SNDRV_PCM_STREAM_CAPTURE] = false;
	}

	mt8532_afe_disable_main_clk(afe);

	return 0;
}


static int mt8532_afe_suspend(struct device *dev, struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct regmap *regmap = afe->regmap;
	int i;

	dev_dbg(afe->dev, "%s suspend %d %d >>\n", __func__,
		pm_runtime_status_suspended(dev), afe->suspended);

	if (pm_runtime_status_suspended(dev) || afe->suspended)
		return 0;

	if (priv->handle_etdm_force_in_suspend_resume)
		mt8532_afe_handle_etdm_force_off(afe);

	mt8532_afe_enable_main_clk(afe);

	if (!afe->reg_back_up)
		afe->reg_back_up =
			devm_kcalloc(dev, afe->reg_back_up_list_num,
				     sizeof(unsigned int), GFP_KERNEL);

	for (i = 0; i < afe->reg_back_up_list_num; i++)
		regmap_read(regmap, afe->reg_back_up_list[i],
			    &afe->reg_back_up[i]);

	mt8532_afe_disable_main_clk(afe);

	//mt8532_afe_disable_reg_rw_clk(afe);

	afe->suspended = true;

	dev_dbg(afe->dev, "%s <<\n", __func__);

	return 0;
}

static int mt8532_afe_resume(struct device *dev, struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct regmap *regmap = afe->regmap;
	int i = 0;

	dev_dbg(afe->dev, "%s suspend %d %d >>\n", __func__,
		pm_runtime_status_suspended(dev), afe->suspended);

	if (pm_runtime_status_suspended(dev) || !afe->suspended)
		return 0;

	if (afe->reg_back_up) {
		mt8532_afe_enable_main_clk(afe);

		for (i = 0; i < afe->reg_back_up_list_num; i++)
			regmap_write(regmap,
				afe->reg_back_up_list[i],
				afe->reg_back_up[i]);

		mt8532_afe_disable_main_clk(afe);
	} else
		dev_dbg(dev, "%s no reg_backup\n", __func__);


	if (priv->handle_etdm_force_in_suspend_resume)
		mt8532_afe_handle_etdm_force_on(afe, false);

	//mt8532_afe_enable_reg_rw_clk(afe);

	afe->suspended = false;

	dev_dbg(afe->dev, "%s <<\n", __func__);

	return 0;
}


static int mt8532_afe_dai_suspend(struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dai->dev);

	dev_dbg(afe->dev, "%s '%s' >>\n", __func__, dai->name);

	mt8532_afe_suspend(afe->dev, afe);

	dev_dbg(afe->dev, "%s '%s' <<\n", __func__, dai->name);

	return 0;
}

static int mt8532_afe_dai_resume(struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dai->dev);

	dev_dbg(afe->dev, "%s '%s' >>\n", __func__, dai->name);

	mt8532_afe_resume(afe->dev, afe);

	dev_dbg(afe->dev, "%s '%s' <<\n", __func__, dai->name);

	return 0;
}

// be_clients will get connected after startup callback
static bool mt8532_match_1st_be_cpu_dai(struct snd_pcm_substream *substream,
	struct snd_soc_pcm_runtime **rbe, char *dai_name)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	int stream = substream->stream;
	struct snd_soc_dpcm *dpcm;
	int i = 0;

	list_for_each_entry(dpcm, &fe->dpcm[stream].be_clients, list_be) {
		struct snd_soc_pcm_runtime *be = dpcm->be;

		if (i > 0)
			break;

		if (!strcmp(be->cpu_dai->name, dai_name)) {
			*rbe = be;
			return true;
		}
		i++;
	}

	return false;
}

static int mt8532_irq_engen_apll_sel(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mt8532_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
	struct mtk_base_afe_irq *irqs = &afe->irqs[memif->irq_usage];
	const struct mtk_base_irq_data *irq_data = irqs->irq_data;
	unsigned int val = 0;

	if (fe_data->slave_mode)
		val = 0;
	else {
		if (fe_data->fs_1xen_sys == FS_ENGEN_A1SYS ||
		    fe_data->fs_1xen_sys == FS_ENGEN_A2SYS) {
			val = 0;
			//mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_LP_MOD);
		} else if (fe_data->fs_1xen_sys == FS_ENGEN_A3SYS)
			val = 1;
		else if (fe_data->fs_1xen_sys == FS_ENGEN_A4SYS)
			val = 2;
		else if (fe_data->fs_1xen_sys == FS_ENGEN_26M) {
			val = 0;
			mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_LP_MOD);
		}
	}
	regmap_update_bits(afe->regmap, irq_data->engen_domain_reg,
			irq_data->engen_domain_maskbit <<
			irq_data->engen_domain_shift,
			val << irq_data->engen_domain_shift);

	return 0;
}

static int mt8532_memif_engen_apll_sel(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mt8532_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
	const struct mtk_base_memif_data *data = memif->data;
	unsigned int val = 0;

	if (fe_data->slave_mode)
		val = 0;
	else {
		if (fe_data->fs_1xen_sys == FS_ENGEN_A1SYS ||
		    fe_data->fs_1xen_sys == FS_ENGEN_A2SYS) {
			val = 0;
			mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_LP_MOD);
		} else if (fe_data->fs_1xen_sys == FS_ENGEN_A3SYS)
			val = 1;
		else if (fe_data->fs_1xen_sys == FS_ENGEN_A4SYS)
			val = 2;
		else if (fe_data->fs_1xen_sys == FS_ENGEN_26M) {
			val = 0;
			mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_LP_MOD);
		}
	}
	regmap_update_bits(afe->regmap, data->engen_domain_reg,
			data->engen_domain_maskbit << data->engen_domain_shift,
			val << data->engen_domain_shift);

	return 0;
}

static int mt8532_memif_fs(struct snd_pcm_substream *substream,
	unsigned int rate)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mt8532_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	struct snd_soc_pcm_runtime *bertd;
	struct mt8532_etdm_data *etdm_data;
	unsigned int dsd_mod;
	unsigned int dir;
	int fs;

	if (dai_id == MT8532_AFE_MEMIF_UL8)
		return MT8532_FS_ETDMIN1_NX_EN;

	if (dai_id == MT8532_AFE_MEMIF_UL3 /*&& is_ul3_in_direct_mode(afe)*/)
		return MT8532_FS_ETDMIN2_NX_EN;

	fs = mt8532_afe_fs_timing(rate);
	if (!fe_data->slave_mode) {
		switch (dai_id) {
		case MT8532_AFE_MEMIF_DLM:
		case MT8532_AFE_MEMIF_DL11:
			if (mt8532_match_1st_be_cpu_dai(substream,
					&bertd, "ETDM1_OUT")) {
				afe = snd_soc_platform_get_drvdata(
					bertd->platform);
				afe_priv = afe->platform_priv;
				etdm_data = &afe_priv->etdm_data[MT8532_ETDM1];
				dir = SNDRV_PCM_STREAM_PLAYBACK;
				dsd_mod = etdm_data->dsd_mod[dir];
				if (dsd_mod == DSD_MOD)
					fs = MT8532_FS_ETDMOUT1_1X_EN;
			}
			break;
		default:
			break;
		}
		return fs;
	}
	// slave mode
	switch (dai_id) {
	case MT8532_AFE_MEMIF_DL10:
		fs = MT8532_FS_ETDMOUT3_1X_EN;
		break;
	case MT8532_AFE_MEMIF_UL2:
	//case MT8532_AFE_MEMIF_UL3:
	case MT8532_AFE_MEMIF_UL4:
//	case MT8532_AFE_MEMIF_UL5:
	case MT8532_AFE_MEMIF_UL9:
	case MT8532_AFE_MEMIF_UL10:
		if (mt8532_match_1st_be_cpu_dai(substream, &bertd,
			"ETDM1_IN"))
			fs = MT8532_FS_ETDMIN1_1X_EN;
		else if (mt8532_match_1st_be_cpu_dai(substream, &bertd,
			"ETDM2_IN"))
			fs = MT8532_FS_ETDMIN2_1X_EN;
		break;
	case MT8532_AFE_MEMIF_DLM:
	case MT8532_AFE_MEMIF_DL11:
	case MT8532_AFE_MEMIF_DL2:
	case MT8532_AFE_MEMIF_DL3:
		if (mt8532_match_1st_be_cpu_dai(substream, &bertd,
			"ETDM1_OUT"))
			fs = MT8532_FS_ETDMOUT1_1X_EN;
		else if (mt8532_match_1st_be_cpu_dai(substream, &bertd,
			"ETDM2_OUT"))
			fs = MT8532_FS_ETDMOUT2_1X_EN;
		break;
	default:
		break;
	}

	return fs;
}

static int mt8532_irq_fs(struct snd_pcm_substream *substream,
	unsigned int rate)
{
	int irq_fs = mt8532_memif_fs(substream, rate);

	if (irq_fs == MT8532_FS_ETDMIN1_NX_EN)
		irq_fs = MT8532_FS_ETDMIN1_1X_EN;
	else if (irq_fs == MT8532_FS_ETDMIN2_NX_EN)
		irq_fs = MT8532_FS_ETDMIN2_1X_EN;
	return irq_fs;
}

static int mt8532_alloc_dmabuf(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
	const struct mtk_base_memif_data *data = memif->data;
	struct mt8532_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	const size_t request_size = params_buffer_bytes(params);
	int ret;

	dev_dbg(afe->dev, "%s %s request_size=%d sram_size=%d\n",
		__func__, data->name, request_size, fe_data->sram_size);
	if (request_size > fe_data->sram_size) {
		ret = snd_pcm_lib_malloc_pages(substream, request_size);
		if (ret < 0) {
			dev_err(afe->dev,
				"%s %s malloc pages %zu bytes failed %d\n",
				__func__, data->name, request_size, ret);
			return ret;
		}

		fe_data->use_sram = false;
		dev_dbg(afe->dev, "%s dma.area=%p, dma.addr=0x%x ,dma.bytes=%d\n",
			__func__, substream->dma_buffer.area,
			substream->dma_buffer.addr,
			substream->dma_buffer.bytes);
		dev_dbg(afe->dev, "%s dma.area=%p, dma.addr=0x%x ,dma.bytes=%d\n",
			__func__, substream->runtime->dma_area,
			substream->runtime->dma_addr,
			substream->runtime->dma_bytes);
		mt8532_afe_block_dpidle(afe);
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

static int mt8532_free_dmabuf(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mt8532_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	int ret = 0;

	if (fe_data->use_sram) {
		snd_pcm_set_runtime_buffer(substream, NULL);
	} else {
		ret = snd_pcm_lib_free_pages(substream);

		mt8532_afe_unblock_dpidle(afe);
	}

	return ret;
}

static enum hrtimer_restart mt8532_afe_dma_hrt_callback(struct hrtimer *hrt)
{
	struct mtk_base_afe_memif *memif = container_of(hrt,
		struct mtk_base_afe_memif, dma_hrt);
	struct mtk_base_afe *afe = memif->afe;
	struct snd_pcm_substream *substream = memif->substream;
	const struct mtk_base_memif_data *data = memif->data;
	unsigned int hw_ptr = 0, hw_base = 0;
	unsigned int hw_cur, prev_hw_cur, increment;
	u64 delay_ns;
	int ret;

	if (!substream) {
		dev_dbg(afe->dev, "%s [%s] invalid substream\n",
			__func__, data->name);
		return HRTIMER_NORESTART;
	}

dma_hrt_check_hw_cur:
	if (!snd_pcm_running(substream)) {
		dev_dbg(afe->dev, "%s [%s] stop running\n",
			__func__, data->name);
		return HRTIMER_NORESTART;
	}

	ret = regmap_read(afe->regmap, data->reg_ofs_base, &hw_base);
	if (ret || hw_base == 0) {
		dev_info(afe->dev, "%s hw_base err\n", __func__);
		return HRTIMER_NORESTART;
	}

	ret = regmap_read(afe->regmap, data->reg_ofs_cur, &hw_ptr);
	if (ret || hw_ptr == 0) {
		dev_info(afe->dev, "%s hw_ptr err\n", __func__);
		return HRTIMER_NORESTART;
	}

	hw_cur = hw_ptr - hw_base;

	prev_hw_cur = memif->prev_hw_cur;

	if (hw_cur >= prev_hw_cur)
		increment = hw_cur - prev_hw_cur;
	else
		increment = memif->buffer_size - prev_hw_cur + hw_cur;

	if (increment >= memif->period_bytes) {
		snd_pcm_period_elapsed(substream);

		memif->prev_hw_cur += memif->period_bytes;
		memif->prev_hw_cur %= memif->buffer_size;

		goto dma_hrt_check_hw_cur;
	} else {
		delay_ns = (u64)(memif->period_bytes - increment) *
			NSEC_PER_SEC;
		do_div(delay_ns, memif->byte_rate);
	}

	hrtimer_forward_now(hrt, ns_to_ktime(delay_ns));

	return HRTIMER_RESTART;
}

int mt8532_afe_get_tv_input_path(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int src = TV_INPUT_PATH_SEL_NONE;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;
	unsigned int val = 0;

	mt8532_afe_enable_main_clk(afe);

	regmap_read(afe->regmap, AFE_LOOPBACK_CFG0, &val);

	mt8532_afe_disable_main_clk(afe);

	val = val & MULTI_IN_DAT0_SEL_MASK;

	if (val == MULTI_IN_DAT0_SEL_SPLIN &&
	    spdif_in->port != SPDIF_IN_PORT_NONE)
		src = PATH_SEL_INTERNAL_ARC;
	else if (val == MULTI_IN_DAT0_SEL_EARC)
		src = PATH_SEL_INTERNAL_EARC;
	else {
		src = TV_INPUT_PATH_SEL_NONE;
		dev_dbg(afe->dev, "%s it's not ARC or EARC path.\n", __func__);
	}

	return src;
}

static int clear_multi_in_monitor_info(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_multi_in_mon_info *mon_info;

	mon_info = &priv->multi_in_mon_info;

	mon_info->rough_type = MT8532_MULTI_IN_UNKNOWN;

	return 0;
}

static int check_multi_in_monitor_status(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_multi_in_mon_info mon_info;
	struct mt8532_multi_in_mon_info *cur_mon_info;
	unsigned int val = 0;
	bool notify = false;

	regmap_read(afe->regmap, AFE_MPHONE_MULTI_MON, &val);
	if (!(val & AFE_MPHONE_MULTI_MON_DEC))
		return 0;

	cur_mon_info = &priv->multi_in_mon_info;

	memset(&mon_info, 0, sizeof(struct mt8532_multi_in_mon_info));

	mon_info.rough_type = AFE_MPHONE_MULTI_MON_ROUGH_TYPE(val);

	if (mon_info.rough_type != MT8532_MULTI_IN_ROUGH_PCM) {
		mon_info.data_type = AFE_MPHONE_MULTI_MON_DATA_TYPE(val);
		mon_info.bitstream_number = AFE_MPHONE_MULTI_MON_BS_NUM(val);
		mon_info.pc_b12_to_b5 = AFE_MPHONE_MULTI_MON_PC_B12_B5(val);
	}

	if ((cur_mon_info->rough_type != mon_info.rough_type) ||
	    ((mon_info.rough_type == MT8532_MULTI_IN_ROUGH_RAW) &&
	     (cur_mon_info->data_type != mon_info.data_type)))
		notify = true;

	memcpy(cur_mon_info, &mon_info,
		sizeof(struct mt8532_multi_in_mon_info));

	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_MON,
		AFE_MPHONE_MULTI_MON_DEC, 0x0);

	if (notify) {
		dev_info(afe->dev,
			"%s rough(%d) data(%d) bsnum(%d) pc12_5(0x%x)\n",
			__func__, mon_info.rough_type, mon_info.data_type,
			mon_info.bitstream_number, mon_info.pc_b12_to_b5);

		mt8532_snd_ctl_notify(priv->card,
			"Multi_In_Info",
			SNDRV_CTL_EVENT_MASK_VALUE);
	}

	return 0;
}

int mt8532_afe_fe_startup(struct snd_pcm_substream *substream,
			  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int id = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[id];
	const struct mtk_base_memif_data *data = memif->data;
	const struct mtk_base_memif_data *aux_data = memif->aux_data;
	const struct snd_pcm_hardware *mtk_afe_hardware = afe->mtk_afe_hardware;
	int ret;

	dev_dbg(dai->dev, "%s %s\n", __func__, data->name);

	memif->substream = substream;

	if (data->buffer_bytes_align > 0)
		snd_pcm_hw_constraint_step(substream->runtime, 0,
				   SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
				   data->buffer_bytes_align);
	else
		snd_pcm_hw_constraint_step(substream->runtime, 0,
				   SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 16);

	mt8532_afe_enable_main_clk(afe);

	/* enable agent */
	regmap_update_bits(afe->regmap, data->agent_disable_reg,
			   1 << data->agent_disable_shift,
			   0 << data->agent_disable_shift);

	if (aux_data)
		regmap_update_bits(afe->regmap,
			aux_data->agent_disable_reg,
			1 << aux_data->agent_disable_shift,
			0 << aux_data->agent_disable_shift);

	snd_soc_set_runtime_hwparams(substream, mtk_afe_hardware);

	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		dev_err(afe->dev, "snd_pcm_hw_constraint_integer failed\n");
		return ret;
	}

	dev_dbg(dai->dev, "%s %s end\n", __func__, data->name);
	return ret;
}

static void mt8532_afe_fe_shutdown(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int id = dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[id];
	const struct mtk_base_memif_data *data = memif->data;
	const struct mtk_base_memif_data *aux_data = memif->aux_data;

	dev_dbg(dai->dev, "%s %s\n", __func__, data->name);

	if (aux_data)
		regmap_update_bits(afe->regmap,
			aux_data->agent_disable_reg,
			1 << aux_data->agent_disable_shift,
			1 << aux_data->agent_disable_shift);

	mtk_afe_fe_shutdown(substream, dai);

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_fe_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int dai_id = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
	const struct mtk_base_memif_data *data = memif->data;
	const struct mtk_base_memif_data *aux_data = memif->aux_data;
	struct mt8532_control_data *ctrl_data = &afe_priv->ctrl_data;
	struct mt8532_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
	unsigned int rate = params_rate(params);
	unsigned int channels = params_channels(params);
	unsigned int bits = params_width(params);
	unsigned int period_size = params_period_size(params);
	unsigned int aux_channels = 0;
	int ret, fs;

	if ((dai_id == MT8532_AFE_MEMIF_DL10) &&
	    afe_priv->dl8_enable_24ch_output &&
	    (channels > afe_priv->dl8_max_main_channels)) {
		aux_channels = channels - afe_priv->dl8_max_main_channels;
		if (aux_channels > 8) {
			dev_info(afe->dev,
				 "%s [%s] unsupported aux_channels %u\n",
				 __func__, data->name, aux_channels);
			return -EINVAL;
		}

		channels -= aux_channels;
	}

	dev_dbg(afe->dev,
		"%s [%s] rate %u ch %u (aux %u) bit %u period %u-%u\n",
		__func__, data->name, rate, channels, aux_channels,
		bits, period_size, params_periods(params));

	if (data->ch_config_shift >= 0)
		regmap_update_bits(afe->regmap,
			data->ch_config_reg,
			0x3f << data->ch_config_shift,  //bit0~5
			channels << data->ch_config_shift);

	if (data->int_odd_shift >= 0) {
		unsigned int odd_en = (channels == 1) ? 1 : 0;

		regmap_update_bits(afe->regmap, data->int_odd_reg,
				   1 << data->int_odd_shift,
				   odd_en << data->int_odd_shift);
	}

	if (aux_data && (aux_channels > 0)) {
		if (aux_data->ch_config_shift >= 0)
			regmap_update_bits(afe->regmap,
				aux_data->ch_config_reg,
				0x3f << aux_data->ch_config_shift,  //bit0~5
				aux_channels << aux_data->ch_config_shift);

		if (aux_data->int_odd_shift >= 0) {
			unsigned int odd_en = (aux_channels == 1) ? 1 : 0;

			regmap_update_bits(afe->regmap,
				aux_data->int_odd_reg,
				1 << aux_data->int_odd_shift,
				odd_en << aux_data->int_odd_shift);
		}
	}

	if (dai_id == MT8532_AFE_MEMIF_UL2) {
		unsigned int val = 0;

		if (!ctrl_data->bypass_cm1) {
			#if 0
			regmap_update_bits(afe->regmap, ASYS_TOP_CON,
				   ASYS_TCON_O34_O41_1X_EN_MASK,
				   ASYS_TCON_O34_O41_1X_EN_UL2);
			#endif
			val |= /*UL_REORDER_START_DATA(8) |*/
				UL_REORDER_UPDATE_CNT(3) |
				UL_REORDER_CHANNEL(channels) |
				UL_REORDER_NO_BYPASS;
		}
		regmap_update_bits(afe->regmap, AFE_CM1_CON,
				UL_REORDER_CTRL_MASK, val);
	} else if (dai_id == MT8532_AFE_MEMIF_UL9) {
		unsigned int val = 0;

		if (!ctrl_data->bypass_cm0) {
			#if 0
			regmap_update_bits(afe->regmap, ASYS_TOP_CON,
				   ASYS_TCON_O26_O33_1X_EN_MASK,
				   ASYS_TCON_O26_O33_1X_EN_UL9);
			if (channels > 8)
				regmap_update_bits(afe->regmap, ASYS_TOP_CON,
					   ASYS_TCON_O34_O41_1X_EN_MASK,
					   ASYS_TCON_O34_O41_1X_EN_UL9);
			#endif
			val |= /*UL_REORDER_START_DATA(0) |*/
				UL_REORDER_UPDATE_CNT(3) |
				UL_REORDER_CHANNEL(channels) |
				UL_REORDER_NO_BYPASS;
		}

		regmap_update_bits(afe->regmap, AFE_CM0_CON,
				UL_REORDER_CTRL_MASK, val);
	}

	memif->aux_channels = aux_channels;

	if (afe->alloc_dmabuf)
		ret = afe->alloc_dmabuf(substream, params, dai);
	else
		ret = snd_pcm_lib_malloc_pages(substream,
				params_buffer_bytes(params));

	if (ret < 0)
		return ret;

	memif->phys_buf_addr = lower_32_bits(runtime->dma_addr);
	memif->buffer_size = runtime->dma_bytes;

	if (aux_data && (aux_channels > 0)) {
		memif->aux_buffer_size = memif->buffer_size * aux_channels /
			params_channels(params);
		memif->buffer_size -= memif->aux_buffer_size;
		memif->aux_phys_addr = memif->phys_buf_addr +
			memif->buffer_size;

		mtk_afe_configure_dma(afe,
			aux_data,
			runtime->dma_addr + memif->buffer_size,
			memif->aux_buffer_size,
			MT8532_FS_ETDMOUT2_1X_EN,
			aux_channels);
	}

	if (afe->memif_engen_apll_sel)
		afe->memif_engen_apll_sel(substream);

	fs = afe->memif_fs(substream, rate);
#if 0
	/* set engen apll domain */
	if (data->engen_domain_shift >= 0) {
		unsigned int fs1xen_sel = 0;

		if (fe_data->fs_1xen_sys == FS_ENGEN_A1SYS ||
		    fe_data->fs_1xen_sys == FS_ENGEN_A2SYS) {
			fs1xen_sel = 0;
			mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_LP_MOD);
		} else if (fe_data->fs_1xen_sys == FS_ENGEN_A3SYS)
			fs1xen_sel = 1;
		else if (fe_data->fs_1xen_sys == FS_ENGEN_A4SYS)
			fs1xen_sel = 2;
		else if (fe_data->fs_1xen_sys == FS_ENGEN_26M) {
			fs1xen_sel = 0;
			mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_LP_MOD);
		}
		regmap_update_bits(afe->regmap, data->engen_domain_reg,
				data->engen_domain_maskbit <<
				data->engen_domain_shift,
				fs1xen_sel << data->engen_domain_shift);
	}
#endif
	dev_dbg(afe->dev,
		"%s [%s] dma_area=%p, dma_addr=0x%x buffer_size=%d fs=%d channels=%d\n",
		__func__, data->name, runtime->dma_area, runtime->dma_addr,
		memif->buffer_size, fs, channels);

	ret = mtk_afe_configure_dma(afe,
		data,
		runtime->dma_addr,
		memif->buffer_size,
		fs, channels);

	if (ret)
		return ret;

	if (dai_id == MT8532_AFE_MEMIF_UL1) {
		u64 request_period_ns, hw_support_irq_period_ns;
		unsigned int byte_rate;

		request_period_ns = (u64)period_size * NSEC_PER_SEC;
		do_div(request_period_ns, rate);

		byte_rate = (rate * channels * bits) >> 3;

		hw_support_irq_period_ns = (u64)MAX_MPHONE_MULTI_PERIOD_BYTES *
			NSEC_PER_SEC;
		do_div(hw_support_irq_period_ns, byte_rate);

		if ((request_period_ns > hw_support_irq_period_ns) &&
		    (((u64)fe_data->min_hw_irq_period_us * NSEC_PER_USEC) >
		     hw_support_irq_period_ns)) {
			memif->period_time_ns = request_period_ns;
			memif->period_bytes = params_period_bytes(params);
			memif->byte_rate = byte_rate;

			hrtimer_init(&memif->dma_hrt,
				CLOCK_MONOTONIC,
				HRTIMER_MODE_REL);

			memif->dma_hrt.function = mt8532_afe_dma_hrt_callback;

			memif->use_sw_irq = true;
		} else
			memif->use_sw_irq = false;
	}

	return ret;
}

static int mt8532_afe_fe_prepare(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int id = dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[id];
	const struct mtk_base_memif_data *data = memif->data;
	const struct mtk_base_memif_data *aux_data = memif->aux_data;
	int hd_audio = 0;

	switch (runtime->format) {
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
		dev_info(afe->dev, "%s unsupported format %d\n",
			 __func__, runtime->format);
		return -EINVAL;
	}
	dev_dbg(dai->dev, "%s %s fmt:%d, hd:%d, reg=0x%x\n", __func__,
		data->name, runtime->format, hd_audio, data->hd_reg);
	if ((memif->aux_channels > 0) && aux_data && (aux_data->hd_reg >= 0))
		regmap_update_bits(afe->regmap, aux_data->hd_reg,
			1 << aux_data->hd_shift,
			hd_audio << aux_data->hd_shift);

	if (data->hd_reg >= 0)
		regmap_update_bits(afe->regmap, data->hd_reg,
			1 << data->hd_shift,
			hd_audio << data->hd_shift);
	dev_dbg(dai->dev, "%s %s end\n", __func__, data->name);
	return 0;
}

int mt8532_afe_fe_trigger(struct snd_pcm_substream *substream, int cmd,
			  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	const int id = dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[id];
	const struct mtk_base_memif_data *data = memif->data;
	const struct mtk_base_memif_data *aux_data = memif->aux_data;

	dev_dbg(dai->dev, "%s %s cmd %d\n", __func__, data->name, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		/* enable channel merge */
		if (id == MT8532_AFE_MEMIF_UL2) {
			regmap_update_bits(afe->regmap,
				AFE_CM1_CON,
				UL_REORDER_EN, UL_REORDER_EN);
		} else if (id == MT8532_AFE_MEMIF_UL9) {
			regmap_update_bits(afe->regmap,
				AFE_CM0_CON,
				UL_REORDER_EN, UL_REORDER_EN);
		}

		if ((memif->aux_channels > 0) && aux_data &&
		    (aux_data->enable_shift >= 0))
			regmap_update_bits(afe->regmap,
				aux_data->enable_reg,
				1 << aux_data->enable_shift,
				1 << aux_data->enable_shift);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		/* disable channel merge */
		if (id == MT8532_AFE_MEMIF_UL2) {
			regmap_update_bits(afe->regmap,
				AFE_CM1_CON,
				UL_REORDER_EN, 0x0);
		} else if (id == MT8532_AFE_MEMIF_UL9) {
			regmap_update_bits(afe->regmap,
				AFE_CM0_CON,
				UL_REORDER_EN, 0x0);
		}

		if ((memif->aux_channels > 0) && aux_data &&
		    (aux_data->enable_shift >= 0))
			regmap_update_bits(afe->regmap,
				aux_data->enable_reg,
				1 << aux_data->enable_shift,
				0);
		break;
	default:
		break;
	}
	dev_dbg(dai->dev, "%s %s cmd %d end\n", __func__, data->name, cmd);
	return mtk_afe_fe_trigger(substream, cmd, dai);
}

static int mt8532_afe_fe_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_fe_dai_data *fe_data = &afe_priv->fe_data[dai->id];

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

static int mt8532_afe_etdm_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = (dai->id == MT8532_AFE_IO_ETDM1_OUT ||
				 dai->id == MT8532_AFE_IO_ETDM1_IN) ?
				 MT8532_ETDM1 : MT8532_ETDM2;
	unsigned int stream;
	int ret;

	if (dai->id == MT8532_AFE_IO_ETDM3_OUT)
		etdm_set = MT8532_ETDM3;
	stream = (dai->id == MT8532_AFE_IO_ETDM1_OUT ||
			dai->id == MT8532_AFE_IO_ETDM2_OUT ||
			dai->id == MT8532_AFE_IO_ETDM3_OUT) ?
			SNDRV_PCM_STREAM_PLAYBACK :
			SNDRV_PCM_STREAM_CAPTURE;

	dev_dbg(dai->dev, "%s etdm_set=%d stream=%d force_on=%d\n",
			__func__, etdm_set, stream,
			afe_priv->etdm_data[etdm_set].force_on[stream]);

	if (afe_priv->etdm_data[etdm_set].force_on[stream])
		return 0;

	ret = mt8532_snd_get_etdm_format(fmt & SND_SOC_DAIFMT_FORMAT_MASK);
	if (ret >= 0)
		afe_priv->etdm_data[etdm_set].format[stream] = ret;
	else
		return -EINVAL;

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

static int mt8532_afe_etdm_set_tdm_slot(struct snd_soc_dai *dai,
					unsigned int tx_mask,
					unsigned int rx_mask,
					int slots,
					int slot_width)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = (dai->id == MT8532_AFE_IO_ETDM1_OUT ||
				 dai->id == MT8532_AFE_IO_ETDM1_IN) ?
				 MT8532_ETDM1 : MT8532_ETDM2;
	unsigned int stream;

	if (dai->id == MT8532_AFE_IO_ETDM3_OUT)
		etdm_set = MT8532_ETDM3;
	stream = (dai->id == MT8532_AFE_IO_ETDM1_OUT ||
			dai->id == MT8532_AFE_IO_ETDM2_OUT ||
			dai->id == MT8532_AFE_IO_ETDM3_OUT) ?
			SNDRV_PCM_STREAM_PLAYBACK :
			SNDRV_PCM_STREAM_CAPTURE;

	dev_dbg(dai->dev, "%s etdm#%u '%s' slot_width %d\n",
		__func__, etdm_set + 1, PCM_STREAM_STR(stream), slot_width);

	if (afe_priv->etdm_data[etdm_set].force_on[stream])
		return 0;

	afe_priv->etdm_data[etdm_set].lrck_width[stream] = slot_width;
	return 0;
}

static int mt8532_afe_etdm_set_sysclk(struct snd_soc_dai *dai,
				      int clk_id,
				      unsigned int freq,
				      int dir)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	unsigned int etdm_set = (dai->id == MT8532_AFE_IO_ETDM1_OUT ||
				 dai->id == MT8532_AFE_IO_ETDM1_IN) ?
				 MT8532_ETDM1 : MT8532_ETDM2;
	unsigned int stream;

	if (dai->id == MT8532_AFE_IO_ETDM3_OUT)
		etdm_set = MT8532_ETDM3;
	stream = (dai->id == MT8532_AFE_IO_ETDM1_OUT ||
			       dai->id == MT8532_AFE_IO_ETDM2_OUT ||
			       dai->id == MT8532_AFE_IO_ETDM3_OUT) ?
			       SNDRV_PCM_STREAM_PLAYBACK :
			       SNDRV_PCM_STREAM_CAPTURE;

	dev_dbg(dai->dev, "%s etdm#%u '%s' freq %u\n",
		__func__, etdm_set + 1, PCM_STREAM_STR(stream), freq);

	if (afe_priv->etdm_data[etdm_set].force_on[stream])
		return 0;

	afe_priv->etdm_data[etdm_set].mclk_freq[stream] = freq;
	return 0;
}

static void mt8532_afe_enable_pcm1(struct mtk_base_afe *afe)
{
	regmap_update_bits(afe->regmap, PCM_INTF_CON1,
			   PCM_INTF_CON1_EN, PCM_INTF_CON1_EN);
}

static void mt8532_afe_disable_pcm1(struct mtk_base_afe *afe)
{
	regmap_update_bits(afe->regmap, PCM_INTF_CON1,
			   PCM_INTF_CON1_EN, 0x0);
}

static int mt8532_afe_configure_pcm1(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	bool slave_mode = afe_priv->pcm_intf_data.slave_mode;
	bool lrck_inv = afe_priv->pcm_intf_data.lrck_inv;
	bool bck_inv = afe_priv->pcm_intf_data.bck_inv;
	unsigned int fmt = afe_priv->pcm_intf_data.format;
	unsigned int bit_width = dai->sample_bits;
	unsigned int val = 0;

	if (!slave_mode) {
		val |= PCM_INTF_CON1_MASTER_MODE |
		       PCM_INTF_CON1_BYPASS_ASRC;

		if (lrck_inv)
			val |= PCM_INTF_CON1_SYNC_OUT_INV;
		if (bck_inv)
			val |= PCM_INTF_CON1_BCLK_OUT_INV;
	} else {
		val |= PCM_INTF_CON1_SLAVE_MODE;

		if (lrck_inv)
			val |= PCM_INTF_CON1_SYNC_IN_INV;
		if (bck_inv)
			val |= PCM_INTF_CON1_BCLK_IN_INV;

		// TODO: add asrc setting
	}

	val |= PCM_INTF_CON1_FORMAT(fmt);

	if (fmt == MT8532_PCM_FORMAT_PCMA ||
	    fmt == MT8532_PCM_FORMAT_PCMB)
		val |= PCM_INTF_CON1_SYNC_LEN(1);
	else
		val |= PCM_INTF_CON1_SYNC_LEN(bit_width);

	switch (runtime->rate) {
	case 48000:
		val |= PCM_INTF_CON1_FS_48K;
		break;
	case 32000:
		val |= PCM_INTF_CON1_FS_32K;
		break;
	case 16000:
		val |= PCM_INTF_CON1_FS_16K;
		break;
	case 8000:
		val |= PCM_INTF_CON1_FS_8K;
		break;
	default:
		return -EINVAL;
	}

	if (bit_width > 16)
		val |= PCM_INTF_CON1_24BIT | PCM_INTF_CON1_64BCK;
	else
		val |= PCM_INTF_CON1_16BIT | PCM_INTF_CON1_32BCK;

	regmap_update_bits(afe->regmap, PCM_INTF_CON1,
			   PCM_INTF_CON1_CONFIG_MASK, val);

	return 0;
}

static int mt8532_afe_pcm1_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	dev_dbg(dai->dev, "%s active %u\n", __func__, dai->active);

	if (dai->active)
		return 0;

	mt8532_afe_enable_main_clk(afe);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_PCMIF);

	return 0;
}

static void mt8532_afe_pcm1_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	dev_dbg(dai->dev, "%s active %u\n", __func__, dai->active);

	if (dai->active)
		return;

	mt8532_afe_disable_pcm1(afe);

	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_PCMIF);

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_pcm1_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int ret;

	dev_dbg(dai->dev, "%s\n", __func__);

	if ((dai->playback_active + dai->capture_active) > 1) {
		dev_dbg(afe->dev, "%s '%s' active(%u-%u) already\n",
			__func__, snd_pcm_stream_str(substream),
			dai->playback_active,
			dai->capture_active);
		return 0;
	}

	ret = mt8532_afe_configure_pcm1(substream, dai);
	if (ret)
		return ret;

	mt8532_afe_enable_pcm1(afe);

	return 0;
}

static int mt8532_afe_pcm1_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	dev_dbg(dai->dev, "%s fmt 0x%x\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		afe_priv->pcm_intf_data.format = MT8532_PCM_FORMAT_I2S;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		afe_priv->pcm_intf_data.bck_inv = false;
		afe_priv->pcm_intf_data.lrck_inv = false;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		afe_priv->pcm_intf_data.bck_inv = false;
		afe_priv->pcm_intf_data.lrck_inv = true;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		afe_priv->pcm_intf_data.bck_inv = true;
		afe_priv->pcm_intf_data.lrck_inv = false;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		afe_priv->pcm_intf_data.bck_inv = true;
		afe_priv->pcm_intf_data.lrck_inv = true;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		afe_priv->pcm_intf_data.slave_mode = true;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		afe_priv->pcm_intf_data.slave_mode = false;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void mt8532_afe_enable_dmic(struct mtk_base_afe *afe,
					struct snd_soc_dai *dai)
{
	unsigned int channels = dai->channels;
	unsigned int val = 0;
	unsigned int msk = 0;

	msk |= DMIC_UL_CON0_MODE_3P25M_CH1_CTL;
	msk |= DMIC_UL_CON0_MODE_3P25M_CH2_CTL;
	msk |= DMIC_UL_CON0_SRC_ON_TMP_CTL;
	val = msk;

	if (channels > 6)
		regmap_update_bits(afe->regmap, AFE_DMIC3_UL_SRC_CON0,
			msk, val);

	if (channels > 4)
		regmap_update_bits(afe->regmap, AFE_DMIC2_UL_SRC_CON0,
			msk, val);

	if (channels > 2)
		regmap_update_bits(afe->regmap, AFE_DMIC1_UL_SRC_CON0,
			msk, val);

	if (channels > 0) {
		regmap_update_bits(afe->regmap, AFE_DMIC0_UL_SRC_CON0,
			msk, val);
	}

#if 0
	/*DE said 8512 dmic no need set this,8168 need */
	/* adda afe on */
	regmap_update_bits(afe->regmap, AFE_ADDA_UL_DL_CON0,
					ADDA_UL_DL_CON0_ADDA_INTF_ON,
					ADDA_UL_DL_CON0_ADDA_INTF_ON);

	/* dmic clkdiv on */
	regmap_update_bits(afe->regmap, AFE_ADDA_UL_DL_CON0,
					ADDA_UL_DL_CON0_DMIC_CLKDIV_ON,
					ADDA_UL_DL_CON0_DMIC_CLKDIV_ON);
#endif

	/* dmic ck div on */
	regmap_update_bits(afe->regmap, PWR2_TOP_CON1,
					PWR2_TOP_CON1_DMIC_PDM_INTF_ON,
					PWR2_TOP_CON1_DMIC_PDM_INTF_ON);
}


static void mt8532_afe_disable_dmic(struct mtk_base_afe *afe)
{
	unsigned int msk = 0;

	msk |= DMIC_UL_CON0_MODE_3P25M_CH1_CTL;
	msk |= DMIC_UL_CON0_MODE_3P25M_CH2_CTL;
	msk |= DMIC_UL_CON0_SRC_ON_TMP_CTL;

	regmap_update_bits(afe->regmap, AFE_DMIC3_UL_SRC_CON0,
		msk, 0x0);
	regmap_update_bits(afe->regmap, AFE_DMIC2_UL_SRC_CON0,
		msk, 0x0);
	regmap_update_bits(afe->regmap, AFE_DMIC1_UL_SRC_CON0,
		msk, 0x0);
	regmap_update_bits(afe->regmap, AFE_DMIC0_UL_SRC_CON0,
		msk, 0x0);

#if 0
	/*DE said 8512 dmic no need set this,8168 need */
	regmap_update_bits(afe->regmap, AFE_ADDA_UL_DL_CON0,
			ADDA_UL_DL_CON0_ADDA_INTF_ON, 0x0);

	regmap_update_bits(afe->regmap, AFE_ADDA_UL_DL_CON0,
			ADDA_UL_DL_CON0_DMIC_CLKDIV_ON, 0x0);
#endif

	regmap_update_bits(afe->regmap, PWR2_TOP_CON1,
		PWR2_TOP_CON1_DMIC_PDM_INTF_ON, 0x0);
}


static const struct reg_sequence mt8532_afe_dmic_iir_coeff_reg_defaults[] = {
	{ AFE_DMIC0_IIR_COEF_02_01, 0x00000000 },
	{ AFE_DMIC0_IIR_COEF_04_03, 0x00003FB8 },
	{ AFE_DMIC0_IIR_COEF_06_05, 0x3FB80000 },
	{ AFE_DMIC0_IIR_COEF_08_07, 0x3FB80000 },
	{ AFE_DMIC0_IIR_COEF_10_09, 0x0000C048 },
	{ AFE_DMIC1_IIR_COEF_02_01, 0x00000000 },
	{ AFE_DMIC1_IIR_COEF_04_03, 0x00003FB8 },
	{ AFE_DMIC1_IIR_COEF_06_05, 0x3FB80000 },
	{ AFE_DMIC1_IIR_COEF_08_07, 0x3FB80000 },
	{ AFE_DMIC1_IIR_COEF_10_09, 0x0000C048 },
	{ AFE_DMIC2_IIR_COEF_02_01, 0x00000000 },
	{ AFE_DMIC2_IIR_COEF_04_03, 0x00003FB8 },
	{ AFE_DMIC2_IIR_COEF_06_05, 0x3FB80000 },
	{ AFE_DMIC2_IIR_COEF_08_07, 0x3FB80000 },
	{ AFE_DMIC2_IIR_COEF_10_09, 0x0000C048 },
	{ AFE_DMIC3_IIR_COEF_02_01, 0x00000000 },
	{ AFE_DMIC3_IIR_COEF_04_03, 0x00003FB8 },
	{ AFE_DMIC3_IIR_COEF_06_05, 0x3FB80000 },
	{ AFE_DMIC3_IIR_COEF_08_07, 0x3FB80000 },
	{ AFE_DMIC3_IIR_COEF_10_09, 0x0000C048 },
};

static int mt8532_afe_load_dmic_iir_coeff_table(struct mtk_base_afe *afe)
{
	return regmap_multi_reg_write(afe->regmap,
			mt8532_afe_dmic_iir_coeff_reg_defaults,
			ARRAY_SIZE(mt8532_afe_dmic_iir_coeff_reg_defaults));
}

static int mt8532_afe_configure_dmic_array(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_dmic_data *dmic_data = &afe_priv->dmic_data;
	unsigned int *dmic_src_sel = dmic_data->dmic_src_sel;
	unsigned int mask =
			PWR2_TOP_CON_DMIC8_SRC_SEL_MASK |
			PWR2_TOP_CON_DMIC7_SRC_SEL_MASK |
			PWR2_TOP_CON_DMIC6_SRC_SEL_MASK |
			PWR2_TOP_CON_DMIC5_SRC_SEL_MASK |
			PWR2_TOP_CON_DMIC4_SRC_SEL_MASK |
			PWR2_TOP_CON_DMIC3_SRC_SEL_MASK |
			PWR2_TOP_CON_DMIC2_SRC_SEL_MASK |
			PWR2_TOP_CON_DMIC1_SRC_SEL_MASK;
	unsigned int val =
			PWR2_TOP_CON_DMIC8_SRC_SEL_VAL(dmic_src_sel[7]) |
			PWR2_TOP_CON_DMIC7_SRC_SEL_VAL(dmic_src_sel[6]) |
			PWR2_TOP_CON_DMIC6_SRC_SEL_VAL(dmic_src_sel[5]) |
			PWR2_TOP_CON_DMIC5_SRC_SEL_VAL(dmic_src_sel[4]) |
			PWR2_TOP_CON_DMIC4_SRC_SEL_VAL(dmic_src_sel[3]) |
			PWR2_TOP_CON_DMIC3_SRC_SEL_VAL(dmic_src_sel[2]) |
			PWR2_TOP_CON_DMIC2_SRC_SEL_VAL(dmic_src_sel[1]) |
			PWR2_TOP_CON_DMIC1_SRC_SEL_VAL(dmic_src_sel[0]);

	regmap_update_bits(afe->regmap, PWR2_TOP_CON0, mask, val);

	return 0;
}

static int mt8532_afe_configure_dmic(struct mtk_base_afe *afe,
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_dmic_data *dmic_data = &afe_priv->dmic_data;
	unsigned int rate = dai->rate;
	unsigned int channels = dai->channels;
	bool two_wire_mode = dmic_data->two_wire_mode;
	unsigned int clk_phase_sel_ch1 = dmic_data->clk_phase_sel_ch1;
	unsigned int clk_phase_sel_ch2 = dmic_data->clk_phase_sel_ch2;
	bool iir_on = dmic_data->iir_on;
	unsigned int dmic_ul_mode = dmic_data->ul_mode;
	unsigned int val = 0;

	val |= DMIC_UL_CON0_SDM_3_LEVEL_CTL;

	if (two_wire_mode) {
		val |= DMIC_UL_CON0_TWO_WIRE_MODE_CTL;
		val |= DMIC_UL_CON0_PHASE_SEL_CH1(clk_phase_sel_ch1);
		val |= DMIC_UL_CON0_PHASE_SEL_CH2(clk_phase_sel_ch2);
	} else {
		val |= DMIC_UL_CON0_PHASE_SEL_CH1(clk_phase_sel_ch1);
		val |= DMIC_UL_CON0_PHASE_SEL_CH2((clk_phase_sel_ch1 + 4)
						  & 0x7);
	}

	mt8532_afe_configure_dmic_array(afe);

	switch (rate) {
	case 48000:
		val |= DMIC_UL_CON0_VOCIE_MODE_48K;
		break;
	case 32000:
		val |= DMIC_UL_CON0_VOCIE_MODE_32K;
		break;
	case 16000:
		val |= DMIC_UL_CON0_VOCIE_MODE_16K;
		break;
	case 8000:
		val |= DMIC_UL_CON0_VOCIE_MODE_8K;
		break;
	default:
		return -EINVAL;
	}

	val |= DMIC_UL_CON0_3P25M_1P625M_SEL(dmic_ul_mode & 0x1);
	val |= DMIC_UL_CON0_LOW_POWER_MODE_SEL(dmic_ul_mode);

	if (iir_on) {
		mt8532_afe_load_dmic_iir_coeff_table(afe);
		val |= DMIC_UL_CON0_IIR_MODE_SEL(0); /* SW mode */
		val |= DMIC_UL_CON0_IIR_ON_TMP_CTL;
	}

	if (channels > 6)
		regmap_update_bits(afe->regmap, AFE_DMIC3_UL_SRC_CON0,
			DMIC_UL_CON0_CONFIG_MASK, val);

	if (channels > 4)
		regmap_update_bits(afe->regmap, AFE_DMIC2_UL_SRC_CON0,
			DMIC_UL_CON0_CONFIG_MASK, val);

	if (channels > 2)
		regmap_update_bits(afe->regmap, AFE_DMIC1_UL_SRC_CON0,
			DMIC_UL_CON0_CONFIG_MASK, val);

	if (channels > 0) {
		regmap_update_bits(afe->regmap, AFE_DMIC0_UL_SRC_CON0,
			DMIC_UL_CON0_CONFIG_MASK, val);
	}

	return 0;
}


static int mt8532_afe_dmic_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	dev_dbg(dai->dev, "%s\n", __func__);

	mt8532_afe_enable_main_clk(afe);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_26M_DMIC1);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_26M_DMIC2);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_26M_DMIC3);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_26M_DMIC4);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_DMIC0);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_DMIC1);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_DMIC2);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_DMIC3);

	return 0;
}

static void mt8532_afe_dmic_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	dev_dbg(dai->dev, "%s\n", __func__);

	mt8532_afe_disable_dmic(afe);

	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_DMIC3);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_DMIC2);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_DMIC1);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_DMIC0);

	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_26M_DMIC4);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_26M_DMIC3);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_26M_DMIC2);
	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_26M_DMIC1);

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_dmic_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	dev_dbg(dai->dev, "%s\n", __func__);

	mt8532_afe_configure_dmic(afe, substream, dai);
	mt8532_afe_enable_dmic(afe, dai);

	return 0;
}

static const struct mt8532_gasrc_ctrl_reg gasrc_ctrl_reg[MT8532_GASRC_NUM] = {
	[MT8532_GASRC0] = {
		.con0 = AFE_GASRC0_NEW_CON0,
		.con1 = AFE_GASRC0_NEW_CON1,
		.con2 = AFE_GASRC0_NEW_CON2,
		.con3 = AFE_GASRC0_NEW_CON3,
		.con4 = AFE_GASRC0_NEW_CON4,
		.con5 = AFE_GASRC0_NEW_CON5,
		.con6 = AFE_GASRC0_NEW_CON6,
		.con7 = AFE_GASRC0_NEW_CON7,
		.con10 = AFE_GASRC0_NEW_CON10,
		.con11 = AFE_GASRC0_NEW_CON11,
		.con13 = AFE_GASRC0_NEW_CON13,
		.con14 = AFE_GASRC0_NEW_CON14,
	},
	[MT8532_GASRC1] = {
		.con0 = AFE_GASRC1_NEW_CON0,
		.con1 = AFE_GASRC1_NEW_CON1,
		.con2 = AFE_GASRC1_NEW_CON2,
		.con3 = AFE_GASRC1_NEW_CON3,
		.con4 = AFE_GASRC1_NEW_CON4,
		.con5 = AFE_GASRC1_NEW_CON5,
		.con6 = AFE_GASRC1_NEW_CON6,
		.con7 = AFE_GASRC1_NEW_CON7,
		.con10 = AFE_GASRC1_NEW_CON10,
		.con11 = AFE_GASRC1_NEW_CON11,
		.con13 = AFE_GASRC1_NEW_CON13,
		.con14 = AFE_GASRC1_NEW_CON14,
	},
	[MT8532_GASRC2] = {
		.con0 = AFE_GASRC2_NEW_CON0,
		.con1 = AFE_GASRC2_NEW_CON1,
		.con2 = AFE_GASRC2_NEW_CON2,
		.con3 = AFE_GASRC2_NEW_CON3,
		.con4 = AFE_GASRC2_NEW_CON4,
		.con5 = AFE_GASRC2_NEW_CON5,
		.con6 = AFE_GASRC2_NEW_CON6,
		.con7 = AFE_GASRC2_NEW_CON7,
		.con10 = AFE_GASRC2_NEW_CON10,
		.con11 = AFE_GASRC2_NEW_CON11,
		.con13 = AFE_GASRC2_NEW_CON13,
		.con14 = AFE_GASRC2_NEW_CON14,
	},
	[MT8532_GASRC3] = {
		.con0 = AFE_GASRC3_NEW_CON0,
		.con1 = AFE_GASRC3_NEW_CON1,
		.con2 = AFE_GASRC3_NEW_CON2,
		.con3 = AFE_GASRC3_NEW_CON3,
		.con4 = AFE_GASRC3_NEW_CON4,
		.con5 = AFE_GASRC3_NEW_CON5,
		.con6 = AFE_GASRC3_NEW_CON6,
		.con7 = AFE_GASRC3_NEW_CON7,
		.con10 = AFE_GASRC3_NEW_CON10,
		.con11 = AFE_GASRC3_NEW_CON11,
		.con13 = AFE_GASRC3_NEW_CON13,
		.con14 = AFE_GASRC3_NEW_CON14,
	},
	[MT8532_GASRC4] = {
		.con0 = AFE_GASRC4_NEW_CON0,
		.con1 = AFE_GASRC4_NEW_CON1,
		.con2 = AFE_GASRC4_NEW_CON2,
		.con3 = AFE_GASRC4_NEW_CON3,
		.con4 = AFE_GASRC4_NEW_CON4,
		.con5 = AFE_GASRC4_NEW_CON5,
		.con6 = AFE_GASRC4_NEW_CON6,
		.con7 = AFE_GASRC4_NEW_CON7,
		.con10 = AFE_GASRC4_NEW_CON10,
		.con11 = AFE_GASRC4_NEW_CON11,
		.con13 = AFE_GASRC4_NEW_CON13,
		.con14 = AFE_GASRC4_NEW_CON14,
	},
	[MT8532_GASRC5] = {
		.con0 = AFE_GASRC5_NEW_CON0,
		.con1 = AFE_GASRC5_NEW_CON1,
		.con2 = AFE_GASRC5_NEW_CON2,
		.con3 = AFE_GASRC5_NEW_CON3,
		.con4 = AFE_GASRC5_NEW_CON4,
		.con5 = AFE_GASRC5_NEW_CON5,
		.con6 = AFE_GASRC5_NEW_CON6,
		.con7 = AFE_GASRC5_NEW_CON7,
		.con10 = AFE_GASRC5_NEW_CON10,
		.con11 = AFE_GASRC5_NEW_CON11,
		.con13 = AFE_GASRC5_NEW_CON13,
		.con14 = AFE_GASRC5_NEW_CON14,
	},
	[MT8532_GASRC6] = {
		.con0 = AFE_GASRC6_NEW_CON0,
		.con1 = AFE_GASRC6_NEW_CON1,
		.con2 = AFE_GASRC6_NEW_CON2,
		.con3 = AFE_GASRC6_NEW_CON3,
		.con4 = AFE_GASRC6_NEW_CON4,
		.con5 = AFE_GASRC6_NEW_CON5,
		.con6 = AFE_GASRC6_NEW_CON6,
		.con7 = AFE_GASRC6_NEW_CON7,
		.con10 = AFE_GASRC6_NEW_CON10,
		.con11 = AFE_GASRC6_NEW_CON11,
		.con13 = AFE_GASRC6_NEW_CON13,
		.con14 = AFE_GASRC6_NEW_CON14,
	},
	[MT8532_GASRC7] = {
		.con0 = AFE_GASRC7_NEW_CON0,
		.con1 = AFE_GASRC7_NEW_CON1,
		.con2 = AFE_GASRC7_NEW_CON2,
		.con3 = AFE_GASRC7_NEW_CON3,
		.con4 = AFE_GASRC7_NEW_CON4,
		.con5 = AFE_GASRC7_NEW_CON5,
		.con6 = AFE_GASRC7_NEW_CON6,
		.con7 = AFE_GASRC7_NEW_CON7,
		.con10 = AFE_GASRC7_NEW_CON10,
		.con11 = AFE_GASRC7_NEW_CON11,
		.con13 = AFE_GASRC7_NEW_CON13,
		.con14 = AFE_GASRC7_NEW_CON14,
	},
	[MT8532_GASRC8] = {
		.con0 = AFE_GASRC8_NEW_CON0,
		.con1 = AFE_GASRC8_NEW_CON1,
		.con2 = AFE_GASRC8_NEW_CON2,
		.con3 = AFE_GASRC8_NEW_CON3,
		.con4 = AFE_GASRC8_NEW_CON4,
		.con5 = AFE_GASRC8_NEW_CON5,
		.con6 = AFE_GASRC8_NEW_CON6,
		.con7 = AFE_GASRC8_NEW_CON7,
		.con10 = AFE_GASRC8_NEW_CON10,
		.con11 = AFE_GASRC8_NEW_CON11,
		.con13 = AFE_GASRC8_NEW_CON13,
		.con14 = AFE_GASRC8_NEW_CON14,
	},
	[MT8532_GASRC9] = {
		.con0 = AFE_GASRC9_NEW_CON0,
		.con1 = AFE_GASRC9_NEW_CON1,
		.con2 = AFE_GASRC9_NEW_CON2,
		.con3 = AFE_GASRC9_NEW_CON3,
		.con4 = AFE_GASRC9_NEW_CON4,
		.con5 = AFE_GASRC9_NEW_CON5,
		.con6 = AFE_GASRC9_NEW_CON6,
		.con7 = AFE_GASRC9_NEW_CON7,
		.con10 = AFE_GASRC9_NEW_CON10,
		.con11 = AFE_GASRC9_NEW_CON11,
		.con13 = AFE_GASRC9_NEW_CON13,
		.con14 = AFE_GASRC9_NEW_CON14,
	},
	[MT8532_GASRC10] = {
		.con0 = AFE_GASRC10_NEW_CON0,
		.con1 = AFE_GASRC10_NEW_CON1,
		.con2 = AFE_GASRC10_NEW_CON2,
		.con3 = AFE_GASRC10_NEW_CON3,
		.con4 = AFE_GASRC10_NEW_CON4,
		.con5 = AFE_GASRC10_NEW_CON5,
		.con6 = AFE_GASRC10_NEW_CON6,
		.con7 = AFE_GASRC10_NEW_CON7,
		.con10 = AFE_GASRC10_NEW_CON10,
		.con11 = AFE_GASRC10_NEW_CON11,
		.con13 = AFE_GASRC10_NEW_CON13,
		.con14 = AFE_GASRC10_NEW_CON14,
	},
	[MT8532_GASRC11] = {
		.con0 = AFE_GASRC11_NEW_CON0,
		.con1 = AFE_GASRC11_NEW_CON1,
		.con2 = AFE_GASRC11_NEW_CON2,
		.con3 = AFE_GASRC11_NEW_CON3,
		.con4 = AFE_GASRC11_NEW_CON4,
		.con5 = AFE_GASRC11_NEW_CON5,
		.con6 = AFE_GASRC11_NEW_CON6,
		.con7 = AFE_GASRC11_NEW_CON7,
		.con10 = AFE_GASRC11_NEW_CON10,
		.con11 = AFE_GASRC11_NEW_CON11,
		.con13 = AFE_GASRC11_NEW_CON13,
		.con14 = AFE_GASRC11_NEW_CON14,
	},
	[MT8532_GASRC12] = {
		.con0 = AFE_GASRC12_NEW_CON0,
		.con1 = AFE_GASRC12_NEW_CON1,
		.con2 = AFE_GASRC12_NEW_CON2,
		.con3 = AFE_GASRC12_NEW_CON3,
		.con4 = AFE_GASRC12_NEW_CON4,
		.con5 = AFE_GASRC12_NEW_CON5,
		.con6 = AFE_GASRC12_NEW_CON6,
		.con7 = AFE_GASRC12_NEW_CON7,
		.con10 = AFE_GASRC12_NEW_CON10,
		.con11 = AFE_GASRC12_NEW_CON11,
		.con13 = AFE_GASRC12_NEW_CON13,
		.con14 = AFE_GASRC12_NEW_CON14,
	},
	[MT8532_GASRC13] = {
		.con0 = AFE_GASRC13_NEW_CON0,
		.con1 = AFE_GASRC13_NEW_CON1,
		.con2 = AFE_GASRC13_NEW_CON2,
		.con3 = AFE_GASRC13_NEW_CON3,
		.con4 = AFE_GASRC13_NEW_CON4,
		.con5 = AFE_GASRC13_NEW_CON5,
		.con6 = AFE_GASRC13_NEW_CON6,
		.con7 = AFE_GASRC13_NEW_CON7,
		.con10 = AFE_GASRC13_NEW_CON10,
		.con11 = AFE_GASRC13_NEW_CON11,
		.con13 = AFE_GASRC13_NEW_CON13,
		.con14 = AFE_GASRC13_NEW_CON14,
	},
	[MT8532_GASRC14] = {
		.con0 = AFE_GASRC14_NEW_CON0,
		.con1 = AFE_GASRC14_NEW_CON1,
		.con2 = AFE_GASRC14_NEW_CON2,
		.con3 = AFE_GASRC14_NEW_CON3,
		.con4 = AFE_GASRC14_NEW_CON4,
		.con5 = AFE_GASRC14_NEW_CON5,
		.con6 = AFE_GASRC14_NEW_CON6,
		.con7 = AFE_GASRC14_NEW_CON7,
		.con10 = AFE_GASRC14_NEW_CON10,
		.con11 = AFE_GASRC14_NEW_CON11,
		.con13 = AFE_GASRC14_NEW_CON13,
		.con14 = AFE_GASRC14_NEW_CON14,
	},
	[MT8532_GASRC15] = {
		.con0 = AFE_GASRC15_NEW_CON0,
		.con1 = AFE_GASRC15_NEW_CON1,
		.con2 = AFE_GASRC15_NEW_CON2,
		.con3 = AFE_GASRC15_NEW_CON3,
		.con4 = AFE_GASRC15_NEW_CON4,
		.con5 = AFE_GASRC15_NEW_CON5,
		.con6 = AFE_GASRC15_NEW_CON6,
		.con7 = AFE_GASRC15_NEW_CON7,
		.con10 = AFE_GASRC15_NEW_CON10,
		.con11 = AFE_GASRC15_NEW_CON11,
		.con13 = AFE_GASRC15_NEW_CON13,
		.con14 = AFE_GASRC15_NEW_CON14,
	},
	[MT8532_GASRC16] = {
		.con0 = AFE_GASRC16_NEW_CON0,
		.con1 = AFE_GASRC16_NEW_CON1,
		.con2 = AFE_GASRC16_NEW_CON2,
		.con3 = AFE_GASRC16_NEW_CON3,
		.con4 = AFE_GASRC16_NEW_CON4,
		.con5 = AFE_GASRC16_NEW_CON5,
		.con6 = AFE_GASRC16_NEW_CON6,
		.con7 = AFE_GASRC16_NEW_CON7,
		.con10 = AFE_GASRC16_NEW_CON10,
		.con11 = AFE_GASRC16_NEW_CON11,
		.con13 = AFE_GASRC16_NEW_CON13,
		.con14 = AFE_GASRC16_NEW_CON14,
	},
	[MT8532_GASRC17] = {
		.con0 = AFE_GASRC17_NEW_CON0,
		.con1 = AFE_GASRC17_NEW_CON1,
		.con2 = AFE_GASRC17_NEW_CON2,
		.con3 = AFE_GASRC17_NEW_CON3,
		.con4 = AFE_GASRC17_NEW_CON4,
		.con5 = AFE_GASRC17_NEW_CON5,
		.con6 = AFE_GASRC17_NEW_CON6,
		.con7 = AFE_GASRC17_NEW_CON7,
		.con10 = AFE_GASRC17_NEW_CON10,
		.con11 = AFE_GASRC17_NEW_CON11,
		.con13 = AFE_GASRC17_NEW_CON13,
		.con14 = AFE_GASRC17_NEW_CON14,
	},
	[MT8532_GASRC18] = {
		.con0 = AFE_GASRC18_NEW_CON0,
		.con1 = AFE_GASRC18_NEW_CON1,
		.con2 = AFE_GASRC18_NEW_CON2,
		.con3 = AFE_GASRC18_NEW_CON3,
		.con4 = AFE_GASRC18_NEW_CON4,
		.con5 = AFE_GASRC18_NEW_CON5,
		.con6 = AFE_GASRC18_NEW_CON6,
		.con7 = AFE_GASRC18_NEW_CON7,
		.con10 = AFE_GASRC18_NEW_CON10,
		.con11 = AFE_GASRC18_NEW_CON11,
		.con13 = AFE_GASRC18_NEW_CON13,
		.con14 = AFE_GASRC18_NEW_CON14,
	},
	[MT8532_GASRC19] = {
		.con0 = AFE_GASRC19_NEW_CON0,
		.con1 = AFE_GASRC19_NEW_CON1,
		.con2 = AFE_GASRC19_NEW_CON2,
		.con3 = AFE_GASRC19_NEW_CON3,
		.con4 = AFE_GASRC19_NEW_CON4,
		.con5 = AFE_GASRC19_NEW_CON5,
		.con6 = AFE_GASRC19_NEW_CON6,
		.con7 = AFE_GASRC19_NEW_CON7,
		.con10 = AFE_GASRC19_NEW_CON10,
		.con11 = AFE_GASRC19_NEW_CON11,
		.con13 = AFE_GASRC19_NEW_CON13,
		.con14 = AFE_GASRC19_NEW_CON14,
	},
};

static const struct mt8532_afe_gasrc_mux_map gasrc_mux_maps[] = {
	{ MT8532_GASRC0, 0, MUX_GASRC_8CH, },
	{ MT8532_GASRC0, 1, MUX_GASRC_6CH, },
	{ MT8532_GASRC0, 2, MUX_GASRC_4CH, },
	{ MT8532_GASRC0, 3, MUX_GASRC_2CH, },
	{ MT8532_GASRC1, 0, MUX_GASRC_8CH, },
	{ MT8532_GASRC1, 1, MUX_GASRC_6CH, },
	{ MT8532_GASRC1, 2, MUX_GASRC_4CH, },
	{ MT8532_GASRC1, 3, MUX_GASRC_2CH, },
	{ MT8532_GASRC2, 0, MUX_GASRC_8CH, },
	{ MT8532_GASRC2, 1, MUX_GASRC_6CH, },
	{ MT8532_GASRC2, 2, MUX_GASRC_2CH, },
	{ MT8532_GASRC3, 0, MUX_GASRC_8CH, },
	{ MT8532_GASRC3, 1, MUX_GASRC_2CH, },
};

static int mt8532_afe_query_gasrc_mux(int gasrc_id, int idx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(gasrc_mux_maps); i++)
		if ((gasrc_mux_maps[i].gasrc_id == gasrc_id) &&
			(gasrc_mux_maps[i].idx == idx))
			return gasrc_mux_maps[i].mux;

	return -EINVAL;
}

static int mt8532_afe_get_gasrc_mux_config(struct mtk_base_afe *afe,
	int gasrc_id, unsigned int stream)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int input_mux;
	int output_mux;

	if (gasrc_id < 0)
		return -EINVAL;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		input_mux = mt8532_afe_query_gasrc_mux(gasrc_id,
			afe_priv->gasrc_data[gasrc_id].input_mux);
		if (input_mux < 0)
			return -EINVAL;
		else
			return input_mux;
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		output_mux = mt8532_afe_query_gasrc_mux(gasrc_id,
			afe_priv->gasrc_data[gasrc_id].output_mux);
		if (output_mux < 0)
			return -EINVAL;
		else
			return output_mux;
	}

	return -EINVAL;
}

static int mt8532_dai_num_to_gasrc(int num)
{
	int val = num - MT8532_AFE_IO_GASRC0;

	if (val < 0 || val >= MT8532_GASRC_NUM)
		return -EINVAL;

	return val;
}

static void mt8532_afe_reset_gasrc(struct mtk_base_afe *afe,
	struct snd_soc_dai *dai)
{


	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int ctrl_reg = 0;

	if (gasrc_id < 0)
		return;
	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON5_SOFT_RESET, GASRC_NEW_CON5_SOFT_RESET);
	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON5_SOFT_RESET, 0);
}

static void mt8532_afe_clear_gasrc(struct mtk_base_afe *afe,
	struct snd_soc_dai *dai)
{
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int ctrl_reg = 0;

	if (gasrc_id < 0)
		return;

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET_STR_CLR, GASRC_NEW_CON0_CHSET_STR_CLR);
}

static struct snd_soc_dai *mt8532_get_be_cpu_dai(
	struct snd_pcm_substream *be_substream,
	char *dai_name)
{
	struct snd_soc_dpcm *dpcm_be;
	struct snd_soc_dpcm *dpcm_fe;
	struct snd_soc_pcm_runtime *be = be_substream->private_data;
	struct snd_soc_pcm_runtime *fe;
	int stream = be_substream->stream;

	list_for_each_entry(dpcm_fe, &be->dpcm[stream].fe_clients, list_fe) {
		fe = dpcm_fe->fe;

		list_for_each_entry(dpcm_be,
			&fe->dpcm[stream].be_clients, list_be) {
			if (!strcmp(dpcm_be->be->cpu_dai->name, dai_name))
				return dpcm_be->be->cpu_dai;
		}
	}

	return NULL;
}

static bool mt8532_be_cpu_dai_matched(struct snd_soc_dapm_widget *w,
	int dai_id, int stream)
{
	struct snd_soc_dapm_path *path;
	struct snd_soc_dai *dai;
	bool ret = false;
	bool playback = (stream == SNDRV_PCM_STREAM_PLAYBACK) ? true : false;

	if (!w)
		return false;

	if (w->is_ep)
		return false;

	if (playback) {
		snd_soc_dapm_widget_for_each_source_path(w, path) {
			if (path && path->source) {
				if (!path->connect)
					continue;

				dai = path->source->priv;
				if (dai && dai_id == dai->id)
					return true;

				ret = mt8532_be_cpu_dai_matched(path->source,
						dai_id, stream);
				if (ret)
					return ret;
			}
		}
	} else {
		snd_soc_dapm_widget_for_each_sink_path(w, path) {
			if (path && path->sink) {
				if (!path->connect)
					continue;

				dai = path->sink->priv;
				if (dai && dai_id == dai->id)
					return true;

				ret = mt8532_be_cpu_dai_matched(path->sink,
						dai_id, stream);
				if (ret)
					return ret;
			}
		}
	}

	return false;
}

static bool mt8532_be_cpu_dai_connected(
	struct snd_pcm_substream *be_substream,
	char *dai_name, int stream, int dai_id)
{
	struct snd_soc_dai *dai =
		mt8532_get_be_cpu_dai(be_substream, dai_name);
	struct snd_soc_dapm_widget *w = NULL;

	if (dai == NULL)
		return false;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		w = dai->playback_widget;
	else if (stream == SNDRV_PCM_STREAM_CAPTURE)
		w = dai->capture_widget;

	return mt8532_be_cpu_dai_matched(w, dai_id, stream);
}

static bool mt8532_get_be_cpu_dai_rate(struct snd_pcm_substream *be_substream,
	char *dai_name, int stream, int dai_id, unsigned int *rate)
{
	struct snd_soc_dai *dai = mt8532_get_be_cpu_dai(be_substream, dai_name);
	bool ret = false;

	if (!rate)
		return ret;

	ret = mt8532_be_cpu_dai_connected(be_substream,
			dai_name, stream, dai_id);
	if (!ret)
		return false;

	*rate = dai->rate;

	return ret;
}



static int mt8532_afe_gasrc_get_input_rate(
	struct snd_pcm_substream *substream, int dai_id, unsigned int rate)
{
	unsigned int input_rate;

	if (mt8532_get_be_cpu_dai_rate(substream, "ETDM1_IN",
			SNDRV_PCM_STREAM_CAPTURE, dai_id, &input_rate))
		return input_rate;
	else if (mt8532_get_be_cpu_dai_rate(substream, "ETDM2_IN",
			SNDRV_PCM_STREAM_CAPTURE, dai_id, &input_rate))
		return input_rate;
	else if (mt8532_get_be_cpu_dai_rate(substream, "DMIC",
			SNDRV_PCM_STREAM_CAPTURE, dai_id, &input_rate))
		return input_rate;

	return rate;
}

static int mt8532_afe_gasrc_get_output_rate(
	struct snd_pcm_substream *substream, int dai_id, unsigned int rate)
{
	unsigned int output_rate;

	if (mt8532_get_be_cpu_dai_rate(substream, "ETDM1_OUT",
			SNDRV_PCM_STREAM_PLAYBACK, dai_id, &output_rate))
		return output_rate;
	else if (mt8532_get_be_cpu_dai_rate(substream, "ETDM2_OUT",
			SNDRV_PCM_STREAM_PLAYBACK, dai_id, &output_rate))
		return output_rate;
	return rate;
}

static int mt8532_afe_gasrc_get_input_fs(struct snd_pcm_substream *substream,
	int dai_id, unsigned int rate)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int fs;
	struct mt8532_etdm_data *etdm_data;
	int be_rate;

	fs = mt8532_afe_fs_timing(rate);


	if (mt8532_be_cpu_dai_connected(substream, "ETDM1_IN",
			SNDRV_PCM_STREAM_CAPTURE, dai_id)) {
		etdm_data = &afe_priv->etdm_data[MT8532_ETDM1];
		if (etdm_data->slave_mode[substream->stream]) {
			fs = MT8532_FS_ETDMIN1_1X_EN;
		} else {
			//be_rate = mt8532_afe_gasrc_get_input_rate(substream,
			//	dai_id, rate);
			struct snd_soc_dai *dai =
				mt8532_get_be_cpu_dai(substream, "ETDM1_IN");

			be_rate = dai->rate;
			fs = mt8532_afe_fs_timing(be_rate);
		}
	} else if (mt8532_be_cpu_dai_connected(substream, "ETDM2_IN",
			SNDRV_PCM_STREAM_CAPTURE, dai_id)) {
		etdm_data = &afe_priv->etdm_data[MT8532_ETDM2];
		if (etdm_data->slave_mode[substream->stream]) {
			fs = MT8532_FS_ETDMIN2_1X_EN;
		} else {
			struct snd_soc_dai *dai =
				mt8532_get_be_cpu_dai(substream, "ETDM2_IN");

			be_rate = dai->rate;
			fs = mt8532_afe_fs_timing(be_rate);
		}
	}

	if (mt8532_be_cpu_dai_connected(substream, "DMIC",
			SNDRV_PCM_STREAM_CAPTURE, dai_id)) {
		be_rate = mt8532_afe_gasrc_get_input_rate(substream,
			dai_id, rate);
		fs = mt8532_afe_fs_timing(be_rate);
	}

	return fs;
}

static int mt8532_afe_gasrc_get_output_fs(struct snd_pcm_substream *substream,
	int dai_id, unsigned int rate)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int fs;
	struct mt8532_etdm_data *etdm_data;
	int be_rate;

	fs = mt8532_afe_fs_timing(rate);

	if (mt8532_be_cpu_dai_connected(substream, "ETDM1_OUT",
		SNDRV_PCM_STREAM_PLAYBACK, dai_id)) {
		etdm_data = &afe_priv->etdm_data[MT8532_ETDM1];
		if (etdm_data->slave_mode[substream->stream]) {
			fs = MT8532_FS_ETDMOUT1_1X_EN;
		} else {
			be_rate = mt8532_afe_gasrc_get_output_rate(substream,
				dai_id, rate);
			fs = mt8532_afe_fs_timing(be_rate);
		}
	} else if (mt8532_be_cpu_dai_connected(substream, "ETDM2_OUT",
		SNDRV_PCM_STREAM_PLAYBACK, dai_id)) {
		etdm_data = &afe_priv->etdm_data[MT8532_ETDM2];
		if (etdm_data->slave_mode[substream->stream]) {
			fs = MT8532_FS_ETDMOUT2_1X_EN;
		} else {
			be_rate = mt8532_afe_gasrc_get_output_rate(substream,
				dai_id, rate);
			fs = mt8532_afe_fs_timing(be_rate);
		}
	}

	return fs;
}

static void mt8532_afe_gasrc_set_input_fs(struct mtk_base_afe *afe,
	struct snd_soc_dai *dai, int fs_timing)
{
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int val = 0;
	unsigned int mask = 0;
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_be_dai_data *be_data = &afe_priv->be_data[dai->id];

	/*set timing sys*/
	if (be_data->fs_1xen_sys == FS_ENGEN_A1SYS ||
	    be_data->fs_1xen_sys == FS_ENGEN_A2SYS)
		val = 0;
	else if (be_data->fs_1xen_sys == FS_ENGEN_A3SYS)
		val = 1;
	else if (be_data->fs_1xen_sys == FS_ENGEN_A4SYS)
		val = 2;

	if (fs_timing >= MT8532_FS_ETDMOUT1_1X_EN &&
	    fs_timing <= MT8532_FS_ETDMIN2_1X_EN)
		val = 0;
	switch (gasrc_id) {
	case MT8532_GASRC0 ... MT8532_GASRC15:
		val = (val << ((gasrc_id-MT8532_GASRC0) * 2));
		mask = (0x3 << ((gasrc_id-MT8532_GASRC0) * 2));
		//afe_msk_write(A3_A4_TIMING_SEL2, val, mask);
		regmap_update_bits(afe->regmap, A3_A4_TIMING_SEL2, mask, val);
		break;
	case MT8532_GASRC16 ... MT8532_GASRC19:
		val = (val << ((gasrc_id-MT8532_GASRC16) * 2));
		mask = (0x3 << ((gasrc_id-MT8532_GASRC16) * 2));
		//afe_msk_write(A3_A4_TIMING_SEL3, val, mask);
		regmap_update_bits(afe->regmap, A3_A4_TIMING_SEL3, mask, val);
		break;
	default:
		break;
	}
	switch (gasrc_id) {
	case MT8532_GASRC0 ... MT8532_GASRC5:
		val = (fs_timing << ((gasrc_id-MT8532_GASRC0) * 5));
		mask = (0x1F << ((gasrc_id-MT8532_GASRC0) * 5));
		//afe_msk_write(GASRC_TIMING_CON0, val, mask);
		regmap_update_bits(afe->regmap, GASRC_TIMING_CON0, mask, val);
		break;
	case MT8532_GASRC6 ... MT8532_GASRC11:
		val = (fs_timing << ((gasrc_id-MT8532_GASRC6) * 5));
		mask = (0x1F << ((gasrc_id-MT8532_GASRC6) * 5));
		//afe_msk_write(GASRC_TIMING_CON1, val, mask);
		regmap_update_bits(afe->regmap, GASRC_TIMING_CON1, mask, val);
		break;
	case MT8532_GASRC12 ... MT8532_GASRC17:
		val = (fs_timing << ((gasrc_id-MT8532_GASRC12) * 5));
		mask = (0x1F << ((gasrc_id-MT8532_GASRC12) * 5));
		//afe_msk_write(GASRC_TIMING_CON2, val, mask);
		regmap_update_bits(afe->regmap, GASRC_TIMING_CON2, mask, val);
		break;
	case MT8532_GASRC18 ... MT8532_GASRC19:
		val = (fs_timing << ((gasrc_id-MT8532_GASRC18) * 5));
		mask = (0x1F << ((gasrc_id-MT8532_GASRC18) * 5));
		//afe_msk_write(GASRC_TIMING_CON3, val, mask);
		regmap_update_bits(afe->regmap, GASRC_TIMING_CON3, mask, val);
		break;
	default:
		break;
	}

	#if 0
	switch (gasrc_id) {
	case MT8532_GASRC0:
		mask = GASRC_TIMING_CON0_GASRC0_IN_MODE_MASK;
		val = GASRC_TIMING_CON0_GASRC0_IN_MODE(fs_timing);
		break;
	case MT8532_GASRC1:
		mask = GASRC_TIMING_CON0_GASRC1_IN_MODE_MASK;
		val = GASRC_TIMING_CON0_GASRC1_IN_MODE(fs_timing);
		break;
	case MT8532_GASRC2:
		mask = GASRC_TIMING_CON0_GASRC2_IN_MODE_MASK;
		val = GASRC_TIMING_CON0_GASRC2_IN_MODE(fs_timing);
		break;
	case MT8532_GASRC3:
		mask = GASRC_TIMING_CON0_GASRC3_IN_MODE_MASK;
		val = GASRC_TIMING_CON0_GASRC3_IN_MODE(fs_timing);
		break;
	default:
		return;
	}
	regmap_update_bits(afe->regmap, GASRC_TIMING_CON0, mask, val);
	#endif
}

static void mt8532_afe_gasrc_set_output_fs(struct mtk_base_afe *afe,
	struct snd_soc_dai *dai, int fs_timing)
{
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int val = 0;
	unsigned int mask = 0;
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_be_dai_data *be_data;

	if (gasrc_id < 0)
		return;

	be_data = &afe_priv->be_data[gasrc_id -
			MT8532_AFE_IO_GASRC0 + MT8532_AFE_IO_GASRC0];

	/*set timing sys*/
	if (be_data->fs_1xen_sys == FS_ENGEN_A1SYS ||
	    be_data->fs_1xen_sys == FS_ENGEN_A2SYS)
		val = 0;
	else if (be_data->fs_1xen_sys == FS_ENGEN_A3SYS)
		val = 1;
	else if (be_data->fs_1xen_sys == FS_ENGEN_A4SYS)
		val = 2;

	if (fs_timing >= MT8532_FS_ETDMOUT1_1X_EN &&
	    fs_timing <= MT8532_FS_ETDMIN2_1X_EN)
		val = 0;
	switch (gasrc_id) {
	case MT8532_GASRC0 ... MT8532_GASRC15:
		val = (val << ((gasrc_id-MT8532_GASRC0) * 2));
		mask = (0x3 << ((gasrc_id-MT8532_GASRC0) * 2));
		regmap_update_bits(afe->regmap, A3_A4_TIMING_SEL4, mask, val);
		break;
	case MT8532_GASRC16 ... MT8532_GASRC19:
		val = (val << ((gasrc_id-MT8532_GASRC16) * 2));
		mask = (0x3 << ((gasrc_id-MT8532_GASRC16) * 2));
		regmap_update_bits(afe->regmap, A3_A4_TIMING_SEL5, mask, val);
		break;
	default:
		break;
	}
	switch (gasrc_id) {
	case MT8532_GASRC0 ... MT8532_GASRC5:
		val = (fs_timing << ((gasrc_id-MT8532_GASRC0) * 5));
		mask = (0x1F << ((gasrc_id-MT8532_GASRC0) * 5));
		regmap_update_bits(afe->regmap, GASRC_TIMING_CON4, mask, val);
		break;
	case MT8532_GASRC6 ... MT8532_GASRC11:
		val = (fs_timing << ((gasrc_id-MT8532_GASRC6) * 5));
		mask = (0x1F << ((gasrc_id-MT8532_GASRC6) * 5));
		regmap_update_bits(afe->regmap, GASRC_TIMING_CON5, mask, val);
		break;
	case MT8532_GASRC12 ... MT8532_GASRC17:
		val = (fs_timing << ((gasrc_id-MT8532_GASRC12) * 5));
		mask = (0x1F << ((gasrc_id-MT8532_GASRC12) * 5));
		regmap_update_bits(afe->regmap, GASRC_TIMING_CON6, mask, val);
		break;
	case MT8532_GASRC18 ... MT8532_GASRC19:
		val = (fs_timing << ((gasrc_id-MT8532_GASRC18) * 5));
		mask = (0x1F << ((gasrc_id-MT8532_GASRC18) * 5));
		regmap_update_bits(afe->regmap, GASRC_TIMING_CON7, mask, val);
		break;
	default:
		break;
	}
	#if 0
	switch (gasrc_id) {
	case MT8532_GASRC0:
		mask = GASRC_TIMING_CON1_GASRC0_OUT_MODE_MASK;
		val = GASRC_TIMING_CON1_GASRC0_OUT_MODE(fs_timing);
		break;
	case MT8532_GASRC1:
		mask = GASRC_TIMING_CON1_GASRC1_OUT_MODE_MASK;
		val = GASRC_TIMING_CON1_GASRC1_OUT_MODE(fs_timing);
		break;
	case MT8532_GASRC2:
		mask = GASRC_TIMING_CON1_GASRC2_OUT_MODE_MASK;
		val = GASRC_TIMING_CON1_GASRC2_OUT_MODE(fs_timing);
		break;
	case MT8532_GASRC3:
		mask = GASRC_TIMING_CON1_GASRC3_OUT_MODE_MASK;
		val = GASRC_TIMING_CON1_GASRC3_OUT_MODE(fs_timing);
		break;
	default:
		return;
	}
	regmap_update_bits(afe->regmap, GASRC_TIMING_CON1, mask, val);
	#endif
}

struct mt8532_afe_gasrc_freq {
	unsigned int rate;
	unsigned int freq_val;
};

static const
	struct mt8532_afe_gasrc_freq
		mt8532_afe_gasrc_freq_palette_49m_45m_64_cycles[] = {
	{ .rate = 8000, .freq_val = 0x050000 },
	{ .rate = 12000, .freq_val = 0x078000 },
	{ .rate = 16000, .freq_val = 0x0A0000 },
	{ .rate = 24000, .freq_val = 0x0F0000 },
	{ .rate = 32000, .freq_val = 0x140000 },
	{ .rate = 48000, .freq_val = 0x1E0000 },
	{ .rate = 96000, .freq_val = 0x3C0000 },
	{ .rate = 192000, .freq_val = 0x780000 },
	{ .rate = 384000, .freq_val = 0xF00000 },
	{ .rate = 7350, .freq_val = 0x049800 },
	{ .rate = 11025, .freq_val = 0x06E400 },
	{ .rate = 14700, .freq_val = 0x093000 },
	{ .rate = 22050, .freq_val = 0x0DC800 },
	{ .rate = 29400, .freq_val = 0x126000 },
	{ .rate = 44100, .freq_val = 0x1B9000 },
	{ .rate = 88200, .freq_val = 0x372000 },
	{ .rate = 176400, .freq_val = 0x6E4000 },
	{ .rate = 352800, .freq_val = 0xDC8000 },
};

static const
	struct mt8532_afe_gasrc_freq
		mt8532_afe_gasrc_period_palette_49m_64_cycles[] = {
	{ .rate = 8000, .freq_val = 0x060000 },
	{ .rate = 12000, .freq_val = 0x040000 },
	{ .rate = 16000, .freq_val = 0x030000 },
	{ .rate = 24000, .freq_val = 0x020000 },
	{ .rate = 32000, .freq_val = 0x018000 },
	{ .rate = 48000, .freq_val = 0x010000 },
	{ .rate = 96000, .freq_val = 0x008000 },
	{ .rate = 192000, .freq_val = 0x004000 },
	{ .rate = 384000, .freq_val = 0x002000 },
	{ .rate = 7350, .freq_val = 0x0687D8 },
	{ .rate = 11025, .freq_val = 0x045A90 },
	{ .rate = 14700, .freq_val = 0x0343EC },
	{ .rate = 22050, .freq_val = 0x022D48 },
	{ .rate = 29400, .freq_val = 0x01A1F6 },
	{ .rate = 44100, .freq_val = 0x0116A4 },
	{ .rate = 88200, .freq_val = 0x008B52 },
	{ .rate = 176400, .freq_val = 0x0045A9 },
	{ .rate = 352800, .freq_val = 0x0022D4 },
};

static const
	struct mt8532_afe_gasrc_freq
		mt8532_afe_gasrc_period_palette_45m_64_cycles[] = {
	{ .rate = 8000, .freq_val = 0x058332 },
	{ .rate = 12000, .freq_val = 0x03ACCC },
	{ .rate = 16000, .freq_val = 0x02C199 },
	{ .rate = 24000, .freq_val = 0x01D666 },
	{ .rate = 32000, .freq_val = 0x0160CC },
	{ .rate = 48000, .freq_val = 0x00EB33 },
	{ .rate = 96000, .freq_val = 0x007599 },
	{ .rate = 192000, .freq_val = 0x003ACD },
	{ .rate = 384000, .freq_val = 0x001D66 },
	{ .rate = 7350, .freq_val = 0x060000 },
	{ .rate = 11025, .freq_val = 0x040000 },
	{ .rate = 14700, .freq_val = 0x030000 },
	{ .rate = 22050, .freq_val = 0x020000 },
	{ .rate = 29400, .freq_val = 0x018000 },
	{ .rate = 44100, .freq_val = 0x010000 },
	{ .rate = 88200, .freq_val = 0x008000 },
	{ .rate = 176400, .freq_val = 0x004000 },
	{ .rate = 352800, .freq_val = 0x002000 },
};

/* INT_ADDA ADC (RX Tracking of 26m) */
static const
	struct mt8532_afe_gasrc_freq
		mt8532_afe_gasrc_freq_palette_49m_45m_48_cycles[] = {
	{ .rate = 8000, .freq_val = 0x06AAAA },
	{ .rate = 12000, .freq_val = 0x0A0000 },
	{ .rate = 16000, .freq_val = 0x0D5555 },
	{ .rate = 24000, .freq_val = 0x140000 },
	{ .rate = 32000, .freq_val = 0x1AAAAA },
	{ .rate = 48000, .freq_val = 0x280000 },
	{ .rate = 96000, .freq_val = 0x500000 },
	{ .rate = 192000, .freq_val = 0xA00000 },
	{ .rate = 384000, .freq_val = 0x1400000 },
	{ .rate = 11025, .freq_val = 0x093000 },
	{ .rate = 22050, .freq_val = 0x126000 },
	{ .rate = 44100, .freq_val = 0x24C000 },
	{ .rate = 88200, .freq_val = 0x498000 },
	{ .rate = 176400, .freq_val = 0x930000 },
	{ .rate = 352800, .freq_val = 0x1260000 },
};

/* INT_ADDA DAC (TX Tracking of 26m) */
static const
	struct mt8532_afe_gasrc_freq
		mt8532_afe_gasrc_period_palette_49m_48_cycles[] = {
	{ .rate = 8000, .freq_val = 0x048000 },
	{ .rate = 12000, .freq_val = 0x030000 },
	{ .rate = 16000, .freq_val = 0x024000 },
	{ .rate = 24000, .freq_val = 0x018000 },
	{ .rate = 32000, .freq_val = 0x012000 },
	{ .rate = 48000, .freq_val = 0x00C000 },
	{ .rate = 96000, .freq_val = 0x006000 },
	{ .rate = 192000, .freq_val = 0x003000 },
	{ .rate = 384000, .freq_val = 0x001800 },
	{ .rate = 11025, .freq_val = 0x0343EB },
	{ .rate = 22050, .freq_val = 0x01A1F6 },
	{ .rate = 44100, .freq_val = 0x00D0FB },
	{ .rate = 88200, .freq_val = 0x00687D },
	{ .rate = 176400, .freq_val = 0x00343F },
	{ .rate = 352800, .freq_val = 0x001A1F },
};

/* INT_ADDA DAC (TX Tracking of 26m) */
static const
	struct mt8532_afe_gasrc_freq
		mt8532_afe_gasrc_period_palette_45m_441_cycles[] = {
	{ .rate = 8000, .freq_val = 0x25FC0D },
	{ .rate = 12000, .freq_val = 0x1952B3 },
	{ .rate = 16000, .freq_val = 0x12FE06 },
	{ .rate = 24000, .freq_val = 0x0CA95A },
	{ .rate = 32000, .freq_val = 0x097F03 },
	{ .rate = 48000, .freq_val = 0x0654AD },
	{ .rate = 96000, .freq_val = 0x032A56 },
	{ .rate = 192000, .freq_val = 0x01952B },
	{ .rate = 384000, .freq_val = 0x00CA96 },
	{ .rate = 11025, .freq_val = 0x1B9000 },
	{ .rate = 22050, .freq_val = 0x0DC800 },
	{ .rate = 44100, .freq_val = 0x06E400 },
	{ .rate = 88200, .freq_val = 0x037200 },
	{ .rate = 176400, .freq_val = 0x01B900 },
	{ .rate = 352800, .freq_val = 0x00DC80 },
};

static int mt8532_afe_gasrc_get_freq_val(unsigned int rate,
	unsigned int cali_cycles)
{
	int i;
	const struct mt8532_afe_gasrc_freq *freq_palette = NULL;
	int tbl_size = 0;

	if (cali_cycles == 48) {
		freq_palette =
			mt8532_afe_gasrc_freq_palette_49m_45m_48_cycles;
		tbl_size = ARRAY_SIZE(
			mt8532_afe_gasrc_freq_palette_49m_45m_48_cycles);
	} else {
		freq_palette =
			mt8532_afe_gasrc_freq_palette_49m_45m_64_cycles;
		tbl_size = ARRAY_SIZE(
			mt8532_afe_gasrc_freq_palette_49m_45m_64_cycles);
	}

	for (i = 0; i < tbl_size; i++)
		if (freq_palette[i].rate == rate)
			return freq_palette[i].freq_val;

	return -EINVAL;
}

static int mt8532_afe_gasrc_get_period_val(unsigned int rate,
	bool op_freq_45m, unsigned int cali_cycles)
{
	int i;
	const struct mt8532_afe_gasrc_freq *period_palette = NULL;
	int tbl_size = 0;

	if (cali_cycles == 48) {
		period_palette =
			mt8532_afe_gasrc_period_palette_49m_48_cycles;
		tbl_size = ARRAY_SIZE(
			mt8532_afe_gasrc_period_palette_49m_48_cycles);
	} else if (cali_cycles == 441) {
		period_palette =
			mt8532_afe_gasrc_period_palette_45m_441_cycles;
		tbl_size = ARRAY_SIZE(
			mt8532_afe_gasrc_period_palette_45m_441_cycles);
	} else {
		if (op_freq_45m) {
			period_palette =
				mt8532_afe_gasrc_period_palette_45m_64_cycles;
			tbl_size = ARRAY_SIZE(
				mt8532_afe_gasrc_period_palette_45m_64_cycles);
		} else {
			period_palette =
				mt8532_afe_gasrc_period_palette_49m_64_cycles;
			tbl_size = ARRAY_SIZE(
				mt8532_afe_gasrc_period_palette_49m_64_cycles);
		}
	}

	for (i = 0; i < tbl_size; i++) {
		if (period_palette[i].rate == rate)
			return period_palette[i].freq_val;
	}

	return -EINVAL;
}

static void mt8532_afe_gasrc_set_rx_mode_fs(struct mtk_base_afe *afe,
	struct snd_soc_dai *dai, int input_rate, int output_rate)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int cali_cycles;
	unsigned int ctrl_reg = 0;
	int val = 0;

	if (gasrc_id < 0)
		return;

	cali_cycles = afe_priv->gasrc_data[gasrc_id].cali_cycles;

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;
	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET0_OFS_SEL_MASK,
		GASRC_NEW_CON0_CHSET0_OFS_SEL_RX);
	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET0_IFS_SEL_MASK,
		GASRC_NEW_CON0_CHSET0_IFS_SEL_RX);

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con2;
	val = mt8532_afe_gasrc_get_freq_val(output_rate, cali_cycles);
	if (val > 0)
		regmap_write(afe->regmap, ctrl_reg, val);

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con3;
	val = mt8532_afe_gasrc_get_freq_val(input_rate, cali_cycles);
	if (val > 0)
		regmap_write(afe->regmap, ctrl_reg, val);
}

static void mt8532_afe_gasrc_set_tx_mode_fs(struct mtk_base_afe *afe,
	struct snd_soc_dai *dai, int input_rate, int output_rate)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	bool gasrc_op_freq_45m;
	unsigned int cali_cycles;
	unsigned int ctrl_reg = 0;
	int val = 0;

	if (gasrc_id < 0)
		return;

	gasrc_op_freq_45m = afe_priv->gasrc_data[gasrc_id].op_freq_45m;
	cali_cycles = afe_priv->gasrc_data[gasrc_id].cali_cycles;

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;
	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET0_OFS_SEL_MASK,
		GASRC_NEW_CON0_CHSET0_OFS_SEL_TX);
	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET0_IFS_SEL_MASK,
		GASRC_NEW_CON0_CHSET0_IFS_SEL_TX);

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con4;
	val = mt8532_afe_gasrc_get_period_val(output_rate,
		gasrc_op_freq_45m, cali_cycles);
	if (val > 0)
		regmap_write(afe->regmap, ctrl_reg, val);

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con1;
	val = mt8532_afe_gasrc_get_period_val(input_rate,
		gasrc_op_freq_45m, cali_cycles);
	if (val > 0)
		regmap_write(afe->regmap, ctrl_reg, val);
}
#if 0
static void mt8532_afe_gasrc_use_sel(struct mtk_base_afe *afe,
	struct snd_soc_dai *dai, bool no_bypass)
{
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int mask = 0;
	unsigned int val = 0;

	switch (gasrc_id) {
	case MT8532_GASRC0:
		mask = GASRC_CFG0_GASRC0_USE_SEL_MASK;
		val = GASRC_CFG0_GASRC0_USE_SEL(no_bypass);
		break;
	case MT8532_GASRC1:
		mask = GASRC_CFG0_GASRC1_USE_SEL_MASK;
		val = GASRC_CFG0_GASRC1_USE_SEL(no_bypass);
		break;
	case MT8532_GASRC2:
		mask = GASRC_CFG0_GASRC2_USE_SEL_MASK;
		val = GASRC_CFG0_GASRC2_USE_SEL(no_bypass);
		break;
	case MT8532_GASRC3:
		mask = GASRC_CFG0_GASRC3_USE_SEL_MASK;
		val = GASRC_CFG0_GASRC3_USE_SEL(no_bypass);
		break;
	default:
		break;
	}

	regmap_update_bits(afe->regmap, GASRC_CFG0, mask, val);
}
#endif
struct mt8532_afe_gasrc_lrck_sel {
	int fs_timing;
	unsigned int lrck_sel_val;
};

static const
	struct mt8532_afe_gasrc_lrck_sel mt8532_afe_gasrc_lrck_sels[] = {
	{
		.fs_timing = MT8532_FS_ETDMIN2_1X_EN,
		.lrck_sel_val = MT8532_AFE_GASRC_LRCK_SEL_ETDM_IN2,
	},
	#if 0
	{
		.fs_timing = MT8532_FS_ETDMIN1_1X_EN,
		.lrck_sel_val = MT8532_AFE_GASRC_LRCK_SEL_ETDM_IN1,
	},
	{
		.fs_timing = MT8532_FS_ETDMOUT2_1X_EN,
		.lrck_sel_val = MT8532_AFE_GASRC_LRCK_SEL_ETDM_OUT2,
	},
	{
		.fs_timing = MT8532_FS_ETDMOUT1_1X_EN,
		.lrck_sel_val = MT8532_AFE_GASRC_LRCK_SEL_ETDM_OUT1,
	},
	#endif
};

static int mt8532_afe_gasrc_get_lrck_sel_val(int fs_timing)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt8532_afe_gasrc_lrck_sels); i++)
		if (mt8532_afe_gasrc_lrck_sels[i].fs_timing == fs_timing)
			return mt8532_afe_gasrc_lrck_sels[i].lrck_sel_val;

	return -EINVAL;
}


static void mt8532_afe_gasrc_sel_lrck(
	struct mtk_base_afe *afe,
	struct snd_soc_dai *dai, int fs_timing)
{
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int val = 0;

	val = mt8532_afe_gasrc_get_lrck_sel_val(fs_timing);

	if (val < 0 || gasrc_id < 0)
		return;

	regmap_update_bits(afe->regmap, gasrc_ctrl_reg[gasrc_id].con5,
			GASRC_CON5_LRCK_SEL_MASK,
			GASRC_CON5_LRCK_SEL(val));

	regmap_update_bits(afe->regmap, gasrc_ctrl_reg[gasrc_id].con6,
			GASRC_CON6_USE_SEL_MASK,
			GASRC_CON6_USE_SEL(1)); //use master

}

static void mt8532_afe_gasrc_sel_cali_clk(
	struct mtk_base_afe *afe, struct snd_soc_dai *dai)
{
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int mask = 0;
	unsigned int cali_ck_sel_val = 0;
	unsigned int cali_ck_26m_sel_val = 0;
	unsigned int val = 0;
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_be_dai_data *be_data = &afe_priv->be_data[dai->id];

	/*set timing sys*/
	switch (be_data->fs_1xen_sys) {
	case FS_ENGEN_A1SYS:
		cali_ck_sel_val = 0;
		cali_ck_26m_sel_val = 0;
		break;
	case FS_ENGEN_A2SYS:
		cali_ck_sel_val = 1;
		cali_ck_26m_sel_val = 0;
		break;
	case FS_ENGEN_A3SYS:
		cali_ck_sel_val = 0;
		cali_ck_26m_sel_val = 2;
		break;
	case FS_ENGEN_A4SYS:
		cali_ck_sel_val = 0;
		cali_ck_26m_sel_val = 3;
		break;
	case FS_ENGEN_26M:
		cali_ck_sel_val = 0;
		cali_ck_26m_sel_val = 1;
		break;
	default:
		return;
	}

	switch (gasrc_id) {
	case MT8532_GASRC0:
		val = (cali_ck_sel_val << 2);
		val |= (cali_ck_26m_sel_val << 24);
		mask = 0x03000004;
		regmap_update_bits(afe->regmap, PWR1_ASM_CON1, mask, val);
		break;
	case MT8532_GASRC1:
		val = (cali_ck_sel_val << 5);
		val |= (cali_ck_26m_sel_val << 26);
		mask = 0x0c000020;
		regmap_update_bits(afe->regmap, PWR1_ASM_CON1, mask, val);
		break;
	case MT8532_GASRC2:
		val = (cali_ck_sel_val << 20);
		val |= (cali_ck_26m_sel_val << 28);
		mask = 0x30100000;
		regmap_update_bits(afe->regmap, PWR1_ASM_CON1, mask, val);
		break;
	case MT8532_GASRC3:
		val = (cali_ck_sel_val << 23);
		val |= (cali_ck_26m_sel_val << 30);
		mask = 0xc0800000;
		regmap_update_bits(afe->regmap, PWR1_ASM_CON1, mask, val);
		break;
	case MT8532_GASRC4 ... MT8532_GASRC9:
		val = (cali_ck_sel_val << ((gasrc_id-MT8532_GASRC4) * 5 + 2));
		val |= (cali_ck_26m_sel_val <<
			((gasrc_id-MT8532_GASRC4) * 5 + 3));
		mask = (0x7 << ((gasrc_id-MT8532_GASRC4) * 5 + 2));
		regmap_update_bits(afe->regmap, PWR1_ASM_CON2, mask, val);
		break;
	case MT8532_GASRC10 ... MT8532_GASRC15:
		val = (cali_ck_sel_val << ((gasrc_id-MT8532_GASRC10) * 5 + 2));
		val |= (cali_ck_26m_sel_val <<
			((gasrc_id-MT8532_GASRC10) * 5 + 3));
		mask = (0x7 << ((gasrc_id-MT8532_GASRC10) * 5 + 2));
		regmap_update_bits(afe->regmap, PWR1_ASM_CON3, mask, val);
		break;
	case MT8532_GASRC16 ... MT8532_GASRC19:
		val = (cali_ck_sel_val << ((gasrc_id-MT8532_GASRC16) * 5 + 2));
		val |= (cali_ck_26m_sel_val <<
			((gasrc_id-MT8532_GASRC16) * 5 + 3));
		mask = (0x7 << ((gasrc_id-MT8532_GASRC16) * 5 + 2));
		regmap_update_bits(afe->regmap, PWR1_ASM_CON4, mask, val);
		break;
	default:
		break;
	}
}

static bool mt8532_afe_gasrc_is_tx_tracking(int fs_timing)
{
	if (fs_timing == MT8532_FS_ETDMOUT1_1X_EN ||
		fs_timing == MT8532_FS_ETDMOUT2_1X_EN)
		return true;
	else
		return false;
}

static bool mt8532_afe_gasrc_is_rx_tracking(int fs_timing)
{
	if (/*fs_timing == MT8532_FS_ETDMIN1_1X_EN ||*/
		fs_timing == MT8532_FS_ETDMIN2_1X_EN)
		return true;
	else
		return false;
}

static void mt8532_afe_gasrc_enable_iir(struct mtk_base_afe *afe,
	int gasrc_id)
{
	unsigned int ctrl_reg = 0;

	if (gasrc_id < 0)
		return;

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET0_IIR_STAGE_MASK,
		GASRC_NEW_CON0_CHSET0_IIR_STAGE(8));

	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET0_CLR_IIR_HISTORY,
		GASRC_NEW_CON0_CHSET0_CLR_IIR_HISTORY);

	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET0_IIR_EN,
		GASRC_NEW_CON0_CHSET0_IIR_EN);
}

static void mt8532_afe_gasrc_disable_iir(struct mtk_base_afe *afe,
	int gasrc_id)
{
	unsigned int ctrl_reg = 0;

	if (gasrc_id < 0)
		return;

	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_CHSET0_IIR_EN, 0);
}

enum afe_gasrc_iir_coeff_table_id {
	MT8532_AFE_GASRC_IIR_COEFF_384_to_352 = 0,
	MT8532_AFE_GASRC_IIR_COEFF_256_to_192,
	MT8532_AFE_GASRC_IIR_COEFF_352_to_256,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_256,
	MT8532_AFE_GASRC_IIR_COEFF_352_to_192,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_176,
	MT8532_AFE_GASRC_IIR_COEFF_256_to_96,
	MT8532_AFE_GASRC_IIR_COEFF_352_to_128,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_128,
	MT8532_AFE_GASRC_IIR_COEFF_352_to_96,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_88,
	MT8532_AFE_GASRC_IIR_COEFF_256_to_48,
	MT8532_AFE_GASRC_IIR_COEFF_352_to_64,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_64,
	MT8532_AFE_GASRC_IIR_COEFF_352_to_48,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_48,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_44,
	MT8532_AFE_GASRC_IIR_COEFF_352_to_32,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_32,
	MT8532_AFE_GASRC_IIR_COEFF_352_to_24,
	MT8532_AFE_GASRC_IIR_COEFF_384_to_24,
	MT8532_AFE_GASRC_IIR_TABLES,
};

struct mt8532_afe_gasrc_iir_coeff_table_id {
	int input_rate;
	int output_rate;
	int table_id;
};

static const struct mt8532_afe_gasrc_iir_coeff_table_id
	mt8532_afe_gasrc_iir_coeff_table_ids[] = {
	{
		.input_rate = 8000,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 12000,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_256,
	},
	{
		.input_rate = 12000,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 16000,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 16000,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_256_to_192,
	},
	{
		.input_rate = 16000,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_176,
	},
	{
		.input_rate = 16000,
		.output_rate = 14700,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 24000,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_128,
	},
	{
		.input_rate = 24000,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 24000,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_256,
	},
	{
		.input_rate = 24000,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_176,
	},
	{
		.input_rate = 24000,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 32000,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 32000,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_256_to_96,
	},
	{
		.input_rate = 32000,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 32000,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_256_to_192,
	},
	{
		.input_rate = 32000,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_88,
	},
	{
		.input_rate = 32000,
		.output_rate = 14700,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_176,
	},
	{
		.input_rate = 32000,
		.output_rate = 29400,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 48000,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_64,
	},
	{
		.input_rate = 48000,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 48000,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_128,
	},
	{
		.input_rate = 48000,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 48000,
		.output_rate = 32000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_256,
	},
	{
		.input_rate = 48000,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_88,
	},
	{
		.input_rate = 48000,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_176,
	},
	{
		.input_rate = 48000,
		.output_rate = 44100,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 96000,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_32,
	},
	{
		.input_rate = 96000,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_48,
	},
	{
		.input_rate = 96000,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_64,
	},
	{
		.input_rate = 96000,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 96000,
		.output_rate = 32000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_128,
	},
	{
		.input_rate = 96000,
		.output_rate = 48000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 96000,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_44,
	},
	{
		.input_rate = 96000,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_88,
	},
	{
		.input_rate = 96000,
		.output_rate = 44100,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_176,
	},
	{
		.input_rate = 96000,
		.output_rate = 88200,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 192000,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_24,
	},
	{
		.input_rate = 192000,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_32,
	},
	{
		.input_rate = 192000,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_48,
	},
	{
		.input_rate = 192000,
		.output_rate = 32000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_64,
	},
	{
		.input_rate = 192000,
		.output_rate = 48000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 192000,
		.output_rate = 96000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 192000,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_44,
	},
	{
		.input_rate = 192000,
		.output_rate = 44100,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_88,
	},
	{
		.input_rate = 192000,
		.output_rate = 88200,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_176,
	},
	{
		.input_rate = 192000,
		.output_rate = 176400,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 384000,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_24,
	},
	{
		.input_rate = 384000,
		.output_rate = 32000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_32,
	},
	{
		.input_rate = 384000,
		.output_rate = 48000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_48,
	},
	{
		.input_rate = 384000,
		.output_rate = 96000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 384000,
		.output_rate = 192000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 384000,
		.output_rate = 44100,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_44,
	},
	{
		.input_rate = 384000,
		.output_rate = 88200,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_88,
	},
	{
		.input_rate = 384000,
		.output_rate = 176400,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_176,
	},
	{
		.input_rate = 384000,
		.output_rate = 352800,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_352,
	},
	{
		.input_rate = 11025,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_256,
	},
	{
		.input_rate = 11025,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_256,
	},
	{
		.input_rate = 14700,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_192,
	},
	{
		.input_rate = 14700,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 14700,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_256_to_192,
	},
	{
		.input_rate = 22050,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_128,
	},
	{
		.input_rate = 22050,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_192,
	},
	{
		.input_rate = 22050,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_256,
	},
	{
		.input_rate = 22050,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_128,
	},
	{
		.input_rate = 22050,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 22050,
		.output_rate = 14700,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_256,
	},
	{
		.input_rate = 29400,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_96,
	},
	{
		.input_rate = 29400,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_192,
	},
	{
		.input_rate = 29400,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 29400,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_256_to_96,
	},
	{
		.input_rate = 29400,
		.output_rate = 14700,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 29400,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_256_to_192,
	},
	{
		.input_rate = 44100,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_64,
	},
	{
		.input_rate = 44100,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_96,
	},
	{
		.input_rate = 44100,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_128,
	},
	{
		.input_rate = 44100,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_192,
	},
	{
		.input_rate = 44100,
		.output_rate = 32000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_256,
	},
	{
		.input_rate = 44100,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_64,
	},
	{
		.input_rate = 44100,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 44100,
		.output_rate = 14700,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_128,
	},
	{
		.input_rate = 44100,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 44100,
		.output_rate = 29400,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_256,
	},
	{
		.input_rate = 88200,
		.output_rate = 8000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_32,
	},
	{
		.input_rate = 88200,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_48,
	},
	{
		.input_rate = 88200,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_64,
	},
	{
		.input_rate = 88200,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_96,
	},
	{
		.input_rate = 88200,
		.output_rate = 32000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_128,
	},
	{
		.input_rate = 88200,
		.output_rate = 48000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_192,
	},
	{
		.input_rate = 88200,
		.output_rate = 7350,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_32,
	},
	{
		.input_rate = 88200,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_48,
	},
	{
		.input_rate = 88200,
		.output_rate = 14700,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_64,
	},
	{
		.input_rate = 88200,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 88200,
		.output_rate = 29400,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_128,
	},
	{
		.input_rate = 88200,
		.output_rate = 44100,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 176400,
		.output_rate = 12000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_24,
	},
	{
		.input_rate = 176400,
		.output_rate = 16000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_32,
	},
	{
		.input_rate = 176400,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_48,
	},
	{
		.input_rate = 176400,
		.output_rate = 32000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_64,
	},
	{
		.input_rate = 176400,
		.output_rate = 48000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_96,
	},
	{
		.input_rate = 176400,
		.output_rate = 96000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_192,
	},
	{
		.input_rate = 176400,
		.output_rate = 11025,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_24,
	},
	{
		.input_rate = 176400,
		.output_rate = 14700,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_32,
	},
	{
		.input_rate = 176400,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_48,
	},
	{
		.input_rate = 176400,
		.output_rate = 29400,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_64,
	},
	{
		.input_rate = 176400,
		.output_rate = 44100,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 176400,
		.output_rate = 88200,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
	{
		.input_rate = 352800,
		.output_rate = 24000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_24,
	},
	{
		.input_rate = 352800,
		.output_rate = 32000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_32,
	},
	{
		.input_rate = 352800,
		.output_rate = 48000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_48,
	},
	{
		.input_rate = 352800,
		.output_rate = 96000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_96,
	},
	{
		.input_rate = 352800,
		.output_rate = 192000,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_352_to_192,
	},
	{
		.input_rate = 352800,
		.output_rate = 22050,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_24,
	},
	{
		.input_rate = 352800,
		.output_rate = 29400,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_32,
	},
	{
		.input_rate = 352800,
		.output_rate = 44100,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_48,
	},
	{
		.input_rate = 352800,
		.output_rate = 88200,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_96,
	},
	{
		.input_rate = 352800,
		.output_rate = 176400,
		.table_id = MT8532_AFE_GASRC_IIR_COEFF_384_to_192,
	},
};

#define IIR_NUMS (48)

static const unsigned int
	mt8532_afe_gasrc_iir_coeffs[MT8532_AFE_GASRC_IIR_TABLES][IIR_NUMS] = {
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_352] = {
		0x10bea3af, 0x2007e9be, 0x10bea3af,
		0xe2821372, 0xf0848d58, 0x00000003,
		0x08f9d435, 0x113d1a1f, 0x08f9d435,
		0xe31a73c5, 0xf1030af1, 0x00000003,
		0x09dd37b9, 0x13106967, 0x09dd37b9,
		0xe41398e1, 0xf1c98ae5, 0x00000003,
		0x0b55c74b, 0x16182d46, 0x0b55c74b,
		0xe5bce8cb, 0xf316f594, 0x00000003,
		0x0e02cb05, 0x1b950f07, 0x0e02cb05,
		0xf44d829a, 0xfaa9876b, 0x00000004,
		0x13e0e18e, 0x277f6d77, 0x13e0e18e,
		0xf695efae, 0xfc700da4, 0x00000004,
		0x0db3df0d, 0x1b6240b3, 0x0db3df0d,
		0xf201ce8e, 0xfca24567, 0x00000003,
		0x06b31e0f, 0x0cca96d1, 0x06b31e0f,
		0xc43a9021, 0xe051c370, 0x00000002,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_256_to_192] = {
		0x0de3c667, 0x137bf0e3, 0x0de3c667,
		0xd9575388, 0xe0d4770d, 0x00000002,
		0x0e54ed46, 0x1474090f, 0x0e54ed46,
		0xdb1c8213, 0xe2a7b6b7, 0x00000002,
		0x0d58713b, 0x13bde364, 0x05d8713b,
		0xde0a3770, 0xe5183cde, 0x00000002,
		0x0bdcfce3, 0x128ef355, 0x0bdcfce3,
		0xe2be28af, 0xe8affd19, 0x00000002,
		0x139091b3, 0x20f20a8e, 0x139091b3,
		0xe9ed58af, 0xedff795d, 0x00000002,
		0x0e68e9cd, 0x1a4cb00b, 0x0e68e9cd,
		0xf3ba2b24, 0xf5275137, 0x00000002,
		0x13079595, 0x251713f9, 0x13079595,
		0xf78c204d, 0xf227616a, 0x00000000,
		0x00000000, 0x2111eb8f, 0x2111eb8f,
		0x0014ac5b, 0x00000000, 0x00000006,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_352_to_256] = {
		0x0db45c84, 0x1113e68a, 0x0db45c84,
		0xdf58fbd3, 0xe0e51ba2, 0x00000002,
		0x0e0c4d8f, 0x11eaf5ef, 0x0e0c4d8f,
		0xe11e9264, 0xe2da4b80, 0x00000002,
		0x0cf2558c, 0x1154c11a, 0x0cf2558c,
		0xe41c6288, 0xe570c517, 0x00000002,
		0x0b5132d7, 0x10545ecd, 0x0b5132d7,
		0xe8e2e944, 0xe92f8dc6, 0x00000002,
		0x1234ffbb, 0x1cfba5c7, 0x1234ffbb,
		0xf00653e0, 0xee9406e3, 0x00000002,
		0x0cfd073a, 0x170277ad, 0x0cfd073a,
		0xf96e16e7, 0xf59562f9, 0x00000002,
		0x08506c2b, 0x1011cd72, 0x08506c2b,
		0x164a9eae, 0xe4203311, 0xffffffff,
		0x00000000, 0x3d58af1e, 0x3d58af1e,
		0x001bee13, 0x00000000, 0x00000007,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_256] = {
		0x0eca2fa9, 0x0f2b0cd3, 0x0eca2fa9,
		0xf50313ef, 0xf15857a7, 0x00000003,
		0x0ee239a9, 0x1045115c, 0x0ee239a9,
		0xec9f2976, 0xe5090807, 0x00000002,
		0x0ec57a45, 0x11d000f7, 0x0ec57a45,
		0xf0bb67bb, 0xe84c86de, 0x00000002,
		0x0e85ba7e, 0x13ee7e9a, 0x0e85ba7e,
		0xf6c74ebb, 0xecdba82c, 0x00000002,
		0x1cba1ac9, 0x2da90ada, 0x1cba1ac9,
		0xfecba589, 0xf2c756e1, 0x00000002,
		0x0f79dec4, 0x1c27f5e0, 0x0f79dec4,
		0x03c44399, 0xfc96c6aa, 0x00000003,
		0x1104a702, 0x21a72c89, 0x1104a702,
		0x1b6a6fb8, 0xfb5ee0f2, 0x00000001,
		0x0622fc30, 0x061a0c67, 0x0622fc30,
		0xe88911f2, 0xe0da327a, 0x00000002,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_352_to_192] = {
		0x1c012b9a, 0x09302bd9, 0x1c012b9a,
		0x0056c6d0, 0xe2b7f35c, 0x00000002,
		0x1b60cee5, 0x0b59639b, 0x1b60cee5,
		0x045dc965, 0xca2264a0, 0x00000001,
		0x19ec96ad, 0x0eb20aa9, 0x19ec96ad,
		0x0a6789cd, 0xd08944ba, 0x00000001,
		0x17c243aa, 0x1347e7fc, 0x17c243aa,
		0x131e03a8, 0xd9241dd4, 0x00000001,
		0x1563b168, 0x1904032f, 0x1563b168,
		0x0f0d206b, 0xf1d7f8e1, 0x00000002,
		0x14cd0206, 0x2169e2af, 0x14cd0206,
		0x14a5d991, 0xf7279caf, 0x00000002,
		0x0aac4c7f, 0x14cb084b, 0x0aac4c7f,
		0x30bc41c6, 0xf5565720, 0x00000001,
		0x0cea20d5, 0x03bc5f00, 0x0cea20d5,
		0xfeec800a, 0xc1b99664, 0x00000001,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_192] = {
		0x1bd356f3, 0x012e014f, 0x1bd356f3,
		0x081be0a6, 0xe28e2407, 0x00000002,
		0x0d7d8ee8, 0x01b9274d, 0x0d7d8ee8,
		0x09857a7b, 0xe4cae309, 0x00000002,
		0x0c999cbe, 0x038e89c5, 0x0c999cbe,
		0x0beae5bc, 0xe7ded2a4, 0x00000002,
		0x0b4b6e2c, 0x061cd206, 0x0b4b6e2c,
		0x0f6a2551, 0xec069422, 0x00000002,
		0x13ad5974, 0x129397e7, 0x13ad5974,
		0x13d3c166, 0xf11cacb8, 0x00000002,
		0x126195d4, 0x1b259a6c, 0x126195d4,
		0x184cdd94, 0xf634a151, 0x00000002,
		0x092aa1ea, 0x11add077, 0x092aa1ea,
		0x3682199e, 0xf31b28fc, 0x00000001,
		0x0e09b91b, 0x0010b76f, 0x0e09b91b,
		0x0f0e2575, 0xc19d364a, 0x00000001,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_176] = {
		0x1b4feb25, 0xfa1874df, 0x1b4feb25,
		0x0fc84364, 0xe27e7427, 0x00000002,
		0x0d22ad1f, 0xfe465ea8, 0x0d22ad1f,
		0x10d89ab2, 0xe4aa760e, 0x00000002,
		0x0c17b497, 0x004c9a14, 0x0c17b497,
		0x12ba36ef, 0xe7a11513, 0x00000002,
		0x0a968b87, 0x031b65c2, 0x0a968b87,
		0x157c39d1, 0xeb9561ce, 0x00000002,
		0x11cea26a, 0x0d025bcc, 0x11cea26a,
		0x18ef4a32, 0xf05a2342, 0x00000002,
		0x0fe5d188, 0x156af55c, 0x0fe5d188,
		0x1c6234df, 0xf50cd288, 0x00000002,
		0x07a1ea25, 0x0e900dd7, 0x07a1ea25,
		0x3d441ae6, 0xf0314c15, 0x00000001,
		0x0dd3517a, 0xfc7f1621, 0x0dd3517a,
		0x1ee4972a, 0xc193ad77, 0x00000001,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_256_to_96] = {
		0x0bad1c6d, 0xf7125e39, 0x0bad1c6d,
		0x200d2195, 0xe0e69a20, 0x00000002,
		0x0b7cc85d, 0xf7b2aa2b, 0x0b7cc85d,
		0x1fd4a137, 0xe2d2e8fc, 0x00000002,
		0x09ad4898, 0xf9f3edb1, 0x09ad4898,
		0x202ffee3, 0xe533035b, 0x00000002,
		0x073ebe31, 0xfcd552f2, 0x073ebe31,
		0x2110eb62, 0xe84975f6, 0x00000002,
		0x092af7cc, 0xff2b1fc9, 0x092af7cc,
		0x2262052a, 0xec1ceb75, 0x00000002,
		0x09655d3e, 0x04f0939d, 0x09655d3e,
		0x47cf219d, 0xe075904a, 0x00000001,
		0x021b3ca5, 0x03057f44, 0x021b3ca5,
		0x4a5c8f68, 0xe72b7f7b, 0x00000001,
		0x00000000, 0x389ecf53, 0x358ecf53,
		0x04b60049, 0x00000000, 0x00000004,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_352_to_128] = {
		0x0c4deacd, 0xf5b3be35, 0x0c4deacd,
		0x20349d1f, 0xe0b9a80d, 0x00000002,
		0x0c5dbbaa, 0xf6157998, 0x0c5dbbaa,
		0x200c143d, 0xe25209ea, 0x00000002,
		0x0a9de1bd, 0xf85ee460, 0x0a9de1bd,
		0x206099de, 0xe46a166c, 0x00000002,
		0x081f9a34, 0xfb7ffe47, 0x081f9a34,
		0x212dd0f7, 0xe753c9ab, 0x00000002,
		0x0a6f9ddb, 0xfd863e9e, 0x0a6f9ddb,
		0x226bd8a2, 0xeb2ead0b, 0x00000002,
		0x05497d0e, 0x01ebd7f0, 0x05497d0e,
		0x23eba2f6, 0xef958aff, 0x00000002,
		0x008e7c5f, 0x00be6aad, 0x008e7c5f,
		0x4a74b30a, 0xe6b0319a, 0x00000001,
		0x00000000, 0x38f3c5aa, 0x38f3c5aa,
		0x012e1306, 0x00000000, 0x00000006,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_128] = {
		0x0cf188aa, 0xf37845cc, 0x0cf188aa,
		0x126b5cbc, 0xf10e5785, 0x00000003,
		0x0c32c481, 0xf503c49b, 0x0c32c481,
		0x24e5a686, 0xe3edcb35, 0x00000002,
		0x0accda0f, 0xf7ad602d, 0x0accda0f,
		0x2547ad4f, 0xe65c4390, 0x00000002,
		0x08d6d7fb, 0xfb56b002, 0x08d6d7fb,
		0x25f3f39f, 0xe9860165, 0x00000002,
		0x0d4b1ceb, 0xff189a5d, 0x0d4b1ceb,
		0x26d3a3a5, 0xed391db5, 0x00000002,
		0x0a060fcf, 0x07a2d23a, 0x0a060fcf,
		0x27b2168e, 0xf0c10173, 0x00000002,
		0x040b6e8c, 0x0742638c, 0x040b6e8c,
		0x5082165c, 0xe5f8f032, 0x00000001,
		0x067a1ae1, 0xf98acf04, 0x067a1ae1,
		0x2526b255, 0xe0ab23e6, 0x00000002,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_352_to_96] = {
		0x0ba3aaf1, 0xf0c12941, 0x0ba3aaf1,
		0x2d8fe4ae, 0xe097f1ad, 0x00000002,
		0x0be92064, 0xf0b1f1a9, 0x0be92064,
		0x2d119d04, 0xe1e5fe1b, 0x00000002,
		0x0a1220de, 0xf3a9aff8, 0x0a1220de,
		0x2ccb18cb, 0xe39903cf, 0x00000002,
		0x07794a30, 0xf7c2c155, 0x07794a30,
		0x2ca647c8, 0xe5ef0ccd, 0x00000002,
		0x0910b1c4, 0xf84c9886, 0x0910b1c4,
		0x2c963877, 0xe8fbcb7a, 0x00000002,
		0x041d6154, 0xfec82c8a, 0x041d6154,
		0x2c926893, 0xec6aa839, 0x00000002,
		0x005b2676, 0x0050bb1f, 0x005b2676,
		0x5927e9f4, 0xde9fd5bc, 0x00000001,
		0x00000000, 0x2b1e5dc1, 0x2b1e5dc1,
		0x0164aa09, 0x00000000, 0x00000006,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_96] = {
		0x0481f41d, 0xf9c1b194, 0x0481f41d,
		0x31c66864, 0xe0581a1d, 0x00000002,
		0x0a3e5a4c, 0xf216665d, 0x0a3e5a4c,
		0x31c3de69, 0xe115ebae, 0x00000002,
		0x0855f15c, 0xf5369aef, 0x0855f15c,
		0x323c17ad, 0xe1feed04, 0x00000002,
		0x05caeeeb, 0xf940c54b, 0x05caeeeb,
		0x33295d2b, 0xe3295c94, 0x00000002,
		0x0651a46a, 0xfa4d6542, 0x0651a46a,
		0x3479d138, 0xe49580b2, 0x00000002,
		0x025e0ccb, 0xff36a412, 0x025e0ccb,
		0x35f517d7, 0xe6182a82, 0x00000002,
		0x0085eff3, 0x0074e0ca, 0x0085eff3,
		0x372ef0de, 0xe7504e71, 0x00000002,
		0x00000000, 0x29b76685, 0x29b76685,
		0x0deab1c3, 0x00000000, 0x00000003,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_88] = {
		0x0c95e01f, 0xed56f8fc, 0x0c95e01f,
		0x191b8467, 0xf0c99b0e, 0x00000003,
		0x0bbee41a, 0xef0e8160, 0x0bbee41a,
		0x31c02b41, 0xe2ef4cd9, 0x00000002,
		0x0a2d258f, 0xf2225b96, 0x0a2d258f,
		0x314c8bd2, 0xe4c10e08, 0x00000002,
		0x07f9e42a, 0xf668315f, 0x07f9e42a,
		0x30cf47d4, 0xe71e3418, 0x00000002,
		0x0afd6fa9, 0xf68f867d, 0x0afd6fa9,
		0x3049674d, 0xe9e0cf4b, 0x00000002,
		0x06ebc830, 0xffaa9acd, 0x06ebc830,
		0x2fcee1bf, 0xec81ee52, 0x00000002,
		0x010de038, 0x01a27806, 0x010de038,
		0x2f82d453, 0xee2ade9b, 0x00000002,
		0x064f0462, 0xf68a0d30, 0x064f0462,
		0x32c81742, 0xe07f3a37, 0x00000002,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_256_to_48] = {
		0x02b72fb4, 0xfb7c5152, 0x02b72fb4,
		0x374ab8ef, 0xe039095c, 0x00000002,
		0x05ca62de, 0xf673171b, 0x05ca62de,
		0x1b94186a, 0xf05c2de7, 0x00000003,
		0x09a9656a, 0xf05ffe29, 0x09a9656a,
		0x37394e81, 0xe1611f87, 0x00000002,
		0x06e86c29, 0xf54bf713, 0x06e86c29,
		0x37797f41, 0xe24ce1f6, 0x00000002,
		0x07a6b7c2, 0xf5491ea7, 0x07a6b7c2,
		0x37e40444, 0xe3856d91, 0x00000002,
		0x02bf8a3e, 0xfd2f5fa6, 0x02bf8a3e,
		0x38673190, 0xe4ea5a4d, 0x00000002,
		0x007e1bd5, 0x000e76ca, 0x007e1bd5,
		0x38da5414, 0xe61afd77, 0x00000002,
		0x00000000, 0x2038247b, 0x2038247b,
		0x07212644, 0x00000000, 0x00000004,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_352_to_64] = {
		0x05c89f29, 0xf6443184, 0x05c89f29,
		0x1bbe0f00, 0xf034bf19, 0x00000003,
		0x05e47be3, 0xf6284bfe, 0x05e47be3,
		0x1b73d610, 0xf0a9a268, 0x00000003,
		0x09eb6c29, 0xefbc8df5, 0x09eb6c29,
		0x365264ff, 0xe286ce76, 0x00000002,
		0x0741f28e, 0xf492d155, 0x0741f28e,
		0x35a08621, 0xe4320cfe, 0x00000002,
		0x087cdc22, 0xf3daa1c7, 0x087cdc22,
		0x34c55ef0, 0xe6664705, 0x00000002,
		0x038022af, 0xfc43da62, 0x038022af,
		0x33d2b188, 0xe8e92eb8, 0x00000002,
		0x001de8ed, 0x0001bd74, 0x001de8ed,
		0x33061aa8, 0xeb0d6ae7, 0x00000002,
		0x00000000, 0x3abd8743, 0x3abd8743,
		0x032b3f7f, 0x00000000, 0x00000005,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_64] = {
		0x05690759, 0xf69bdff3, 0x05690759,
		0x392fbdf5, 0xe032c3cc, 0x00000002,
		0x05c3ff7a, 0xf60d6b05, 0x05c3ff7a,
		0x1c831a72, 0xf052119a, 0x00000003,
		0x0999efb9, 0xefae71b0, 0x0999efb9,
		0x3900fd02, 0xe13a60b9, 0x00000002,
		0x06d5aa46, 0xf4c1d0ea, 0x06d5aa46,
		0x39199f34, 0xe20c15e1, 0x00000002,
		0x077f7d1d, 0xf49411e4, 0x077f7d1d,
		0x394b3591, 0xe321be50, 0x00000002,
		0x02a14b6b, 0xfcd3c8a5, 0x02a14b6b,
		0x398b4c12, 0xe45e5473, 0x00000002,
		0x00702155, 0xffef326c, 0x00702155,
		0x39c46c90, 0xe56c1e59, 0x00000002,
		0x00000000, 0x1c69d66c, 0x1c69d66c,
		0x0e76f270, 0x00000000, 0x00000003,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_352_to_48] = {
		0x05be8a21, 0xf589fb98, 0x05be8a21,
		0x1d8de063, 0xf026c3d8, 0x00000003,
		0x05ee4f4f, 0xf53df2e5, 0x05ee4f4f,
		0x1d4d87e2, 0xf07d5518, 0x00000003,
		0x0a015402, 0xee079bc7, 0x0a015402,
		0x3a0a0c2b, 0xe1e16c40, 0x00000002,
		0x07512c6a, 0xf322f651, 0x07512c6a,
		0x394e82c2, 0xe326def2, 0x00000002,
		0x087a5316, 0xf1d3ba1f, 0x087a5316,
		0x385bbd4a, 0xe4dbe26b, 0x00000002,
		0x035bd161, 0xfb2b7588, 0x035bd161,
		0x37464782, 0xe6d6a034, 0x00000002,
		0x00186dd8, 0xfff28830, 0x00186dd8,
		0x365746b9, 0xe88d9a4a, 0x00000002,
		0x00000000, 0x2cd02ed1, 0x2cd02ed1,
		0x035f6308, 0x00000000, 0x00000005,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_48] = {
		0x0c68c88c, 0xe9266466, 0x0c68c88c,
		0x1db3d4c3, 0xf0739c07, 0x00000003,
		0x05c69407, 0xf571a70a, 0x05c69407,
		0x1d6f1d3b, 0xf0d89718, 0x00000003,
		0x09e8d133, 0xee2a68df, 0x09e8d133,
		0x3a32d61b, 0xe2c2246a, 0x00000002,
		0x079233b7, 0xf2d17252, 0x079233b7,
		0x3959a2c3, 0xe4295381, 0x00000002,
		0x09c2822e, 0xf0613d7b, 0x09c2822e,
		0x385c3c48, 0xe5d3476b, 0x00000002,
		0x050e0b2c, 0xfa200d5d, 0x050e0b2c,
		0x37688f21, 0xe76fc030, 0x00000002,
		0x006ddb6e, 0x00523f01, 0x006ddb6e,
		0x36cd234d, 0xe8779510, 0x00000002,
		0x0635039f, 0xf488f773, 0x0635039f,
		0x3be42508, 0xe0488e99, 0x00000002,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_44] = {
		0x0c670696, 0xe8dc1ef2, 0x0c670696,
		0x1e05c266, 0xf06a9f0d, 0x00000003,
		0x05c60160, 0xf54b9f4a, 0x05c60160,
		0x1dc3811d, 0xf0c7e4db, 0x00000003,
		0x09e74455, 0xeddfc92a, 0x09e74455,
		0x3adfddda, 0xe28c4ae3, 0x00000002,
		0x078ea9ae, 0xf28c3ba7, 0x078ea9ae,
		0x3a0a98e8, 0xe3d93541, 0x00000002,
		0x09b32647, 0xefe954c5, 0x09b32647,
		0x3910a244, 0xe564f781, 0x00000002,
		0x04f0e9e4, 0xf9b7e8d5, 0x04f0e9e4,
		0x381f6928, 0xe6e5316c, 0x00000002,
		0x006303ee, 0x003ae836, 0x006303ee,
		0x37852c0e, 0xe7db78c1, 0x00000002,
		0x06337ac0, 0xf46665c5, 0x06337ac0,
		0x3c818406, 0xe042df81, 0x00000002,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_352_to_32] = {
		0x07d25973, 0xf0fd68ae, 0x07d25973,
		0x3dd9d640, 0xe02aaf11, 0x00000002,
		0x05a0521d, 0xf5390cc4, 0x05a0521d,
		0x1ec7dff7, 0xf044be0d, 0x00000003,
		0x04a961e1, 0xf71c730b, 0x04a961e1,
		0x1e9edeee, 0xf082b378, 0x00000003,
		0x06974728, 0xf38e3bf1, 0x06974728,
		0x3cd69b60, 0xe1afd01c, 0x00000002,
		0x072d4553, 0xf2c1e0e2, 0x072d4553,
		0x3c54fdc3, 0xe28e96b6, 0x00000002,
		0x02802de3, 0xfbb07dd5, 0x02802de3,
		0x3bc4f40f, 0xe38a3256, 0x00000002,
		0x000ce31b, 0xfff0d7a8, 0x000ce31b,
		0x3b4bbb40, 0xe45f55d6, 0x00000002,
		0x00000000, 0x1ea1b887, 0x1ea1b887,
		0x03b1b27d, 0x00000000, 0x00000005,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_32] = {
		0x0c5074a7, 0xe83ee090, 0x0c5074a7,
		0x1edf8fe7, 0xf04ec5d0, 0x00000003,
		0x05bbb01f, 0xf4fa20a7, 0x05bbb01f,
		0x1ea87e16, 0xf093b881, 0x00000003,
		0x04e8e57f, 0xf69fc31d, 0x04e8e57f,
		0x1e614210, 0xf0f1139e, 0x00000003,
		0x07756686, 0xf1f67c0b, 0x07756686,
		0x3c0a3b55, 0xe2d8c5a6, 0x00000002,
		0x097212e8, 0xeede0608, 0x097212e8,
		0x3b305555, 0xe3ff02e3, 0x00000002,
		0x0495d6c0, 0xf8bf1399, 0x0495d6c0,
		0x3a5c93a1, 0xe51e0d14, 0x00000002,
		0x00458b2d, 0xfffdc761, 0x00458b2d,
		0x39d4793b, 0xe5d6d407, 0x00000002,
		0x0609587b, 0xf456ed0f, 0x0609587b,
		0x3e1d20e1, 0xe0315c96, 0x00000002,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_352_to_24] = {
		0x062002ee, 0xf4075ac9, 0x062002ee,
		0x1f577599, 0xf0166280, 0x00000003,
		0x05cdb68c, 0xf4ab2e81, 0x05cdb68c,
		0x1f2a7a17, 0xf0484eb7, 0x00000003,
		0x04e3078b, 0xf67b954a, 0x04e3078b,
		0x1ef25b71, 0xf08a5bcf, 0x00000003,
		0x071fc81e, 0xf23391f6, 0x071fc81e,
		0x3d4bc51b, 0xe1cdf67e, 0x00000002,
		0x08359c1c, 0xf04d3910, 0x08359c1c,
		0x3c80bf1e, 0xe2c6cf99, 0x00000002,
		0x0331888d, 0xfa1ebde6, 0x0331888d,
		0x3b94c153, 0xe3e96fad, 0x00000002,
		0x00143063, 0xffe1d1af, 0x00143063,
		0x3ac672e3, 0xe4e7f96f, 0x00000002,
		0x00000000, 0x2d7cf831, 0x2d7cf831,
		0x074e3a4f, 0x00000000, 0x00000004,
	},
	[MT8532_AFE_GASRC_IIR_COEFF_384_to_24] = {
		0x0c513993, 0xe7dbde26, 0x0c513993,
		0x1f4e3b98, 0xf03b6bee, 0x00000003,
		0x05bd9980, 0xf4c4fb19, 0x05bd9980,
		0x1f21aa2b, 0xf06fa0e5, 0x00000003,
		0x04eb9c21, 0xf6692328, 0x04eb9c21,
		0x1ee6fb2f, 0xf0b6982c, 0x00000003,
		0x07795c9e, 0xf18d56cf, 0x07795c9e,
		0x3d345c1a, 0xe229a2a1, 0x00000002,
		0x096d3d11, 0xee265518, 0x096d3d11,
		0x3c7d096a, 0xe30bee74, 0x00000002,
		0x0478f0db, 0xf8270d5a, 0x0478f0db,
		0x3bc96998, 0xe3ea3cf8, 0x00000002,
		0x0037d4b8, 0xffdedcf0, 0x0037d4b8,
		0x3b553ec9, 0xe47a2910, 0x00000002,
		0x0607e296, 0xf42bc1d7, 0x0607e296,
		0x3ee67cb9, 0xe0252e31, 0x00000002,
	},
};

static bool mt8532_afe_gasrc_found_iir_coeff_table_id(int input_rate,
	int output_rate, int *table_id)
{
	int i;
	const struct mt8532_afe_gasrc_iir_coeff_table_id *table =
		mt8532_afe_gasrc_iir_coeff_table_ids;

	if (!table_id)
		return false;

	/* no need to apply iir for up-sample */
	if (input_rate <= output_rate)
		return false;

	for (i = 0; i < ARRAY_SIZE(mt8532_afe_gasrc_iir_coeff_table_ids); i++) {
		if ((table[i].input_rate == input_rate) &&
			(table[i].output_rate == output_rate)) {
			*table_id = table[i].table_id;
			return true;
		}
	}

	return false;
}

static bool mt8532_afe_gasrc_fill_iir_coeff_table(struct mtk_base_afe *afe,
	int gasrc_id, int table_id)
{
	const unsigned int *table;
	unsigned int ctrl_reg;
	int i;

	if ((table_id < 0) ||
		(table_id >= MT8532_AFE_GASRC_IIR_TABLES))
		return false;

	if (gasrc_id < 0)
		return false;

	dev_dbg(afe->dev, "%s [%d] table_id %d\n",
		__func__, gasrc_id, table_id);

	table = &mt8532_afe_gasrc_iir_coeffs[table_id][0];

	/* enable access for iir sram */
	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;
	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_COEFF_SRAM_CTRL,
		GASRC_NEW_CON0_COEFF_SRAM_CTRL);

	/* fill coeffs from addr 0 */
	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con11;
	regmap_write(afe->regmap, ctrl_reg, 0);

	/* fill all coeffs */
	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con10;
	for (i = 0; i < IIR_NUMS; i++)
		regmap_write(afe->regmap, ctrl_reg, table[i]);

	/* disable access for iir sram */
	ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;
	regmap_update_bits(afe->regmap, ctrl_reg,
		GASRC_NEW_CON0_COEFF_SRAM_CTRL, 0);

	return true;
}

static bool mt8532_afe_load_gasrc_iir_coeff_table(struct mtk_base_afe *afe,
	int gasrc_id, int input_rate, int output_rate)
{
	int table_id;

	if (mt8532_afe_gasrc_found_iir_coeff_table_id(input_rate,
			output_rate, &table_id)) {
		return mt8532_afe_gasrc_fill_iir_coeff_table(afe,
			gasrc_id, table_id);
	}

	return false;
}

static void mt8532_afe_adjust_gasrc_cali_cycles(struct mtk_base_afe *afe,
	struct snd_soc_dai *dai, int fs_timing, unsigned int rate)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	unsigned int *cali_cycles;

	if (gasrc_id < 0)
		return;

	cali_cycles = &(afe_priv->gasrc_data[gasrc_id].cali_cycles);

	if (fs_timing == MT8532_FS_AMIC_1X_EN_ASYNC) {
		switch (rate) {
		case 8000:
			/* FALLTHROUGH */
		case 16000:
			/* FALLTHROUGH */
		case 32000:
			*cali_cycles = 64;
			break;
		case 48000:
			*cali_cycles = 48;
			break;
		default:
			*cali_cycles = 64;
			break;
		}
	}

	dev_dbg(afe->dev, "%s [%d] cali_cycles %u\n",
		__func__, gasrc_id, *cali_cycles);
}

static int mt8532_afe_configure_gasrc(struct mtk_base_afe *afe,
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	unsigned int stream = substream->stream;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	struct mt8532_gasrc_data *gasrc_data;
	int mux;
	int input_fs = 0, output_fs = 0;
	int input_rate = 0, output_rate = 0;
	unsigned int val = 0;
	unsigned int mask = 0;
	struct snd_soc_pcm_runtime *be = substream->private_data;
	struct snd_pcm_substream *paired_substream = NULL;
	bool duplex;
	bool *gasrc_op_freq_45m;
	unsigned int *cali_cycles;


	if (gasrc_id < 0)
		return -EINVAL;

	gasrc_data = &afe_priv->gasrc_data[gasrc_id];

	mux = mt8532_afe_get_gasrc_mux_config(afe, gasrc_id, stream);
	duplex = gasrc_data->duplex;
	gasrc_op_freq_45m = &(gasrc_data->op_freq_45m);
	cali_cycles = &(gasrc_data->cali_cycles);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		input_fs = mt8532_afe_fs_timing(runtime->rate);
		input_rate = runtime->rate;
		*gasrc_op_freq_45m = (input_rate % 8000);

		if (duplex) {
			paired_substream = snd_soc_dpcm_get_substream(be,
				SNDRV_PCM_STREAM_CAPTURE);
			output_fs = mt8532_afe_gasrc_get_output_fs(
				paired_substream, dai->id,
				paired_substream->runtime->rate);
			output_rate = mt8532_afe_gasrc_get_output_rate(
				paired_substream, dai->id,
				paired_substream->runtime->rate);
		} else {
			output_fs = mt8532_afe_gasrc_get_output_fs(
				substream, dai->id, runtime->rate);
			output_rate = mt8532_afe_gasrc_get_output_rate(
				substream, dai->id, runtime->rate);
		}
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		output_fs = mt8532_afe_fs_timing(runtime->rate);
		output_rate = runtime->rate;
		*gasrc_op_freq_45m = (output_rate % 8000);

		if (duplex) {
			paired_substream =
				snd_soc_dpcm_get_substream(be,
					SNDRV_PCM_STREAM_PLAYBACK);
			input_fs = mt8532_afe_gasrc_get_input_fs(
				paired_substream, dai->id,
				paired_substream->runtime->rate);
			input_rate = mt8532_afe_gasrc_get_input_rate(
				paired_substream, dai->id,
				paired_substream->runtime->rate);
		} else {
			input_fs = mt8532_afe_gasrc_get_input_fs(
				substream, dai->id, runtime->rate);
			//input_rate = mt8532_afe_gasrc_get_input_rate(
			//	substream, dai->id, runtime->rate);
			input_rate = mt8532_afe_emu_to_rate(input_fs);
		}
	}

	dev_dbg(dai->dev,
		"%s [%d] '%s' in fs-rate 0x%x-%d out fs-rate 0x%x-%d 45m %d,duplex=%d,stream=%d\n",
		__func__, gasrc_id, snd_pcm_stream_str(substream),
		input_fs, input_rate, output_fs, output_rate,
		*gasrc_op_freq_45m, duplex, stream);

	if (mt8532_afe_load_gasrc_iir_coeff_table(afe, gasrc_id,
			input_rate, output_rate)) {
		mt8532_afe_gasrc_enable_iir(afe, gasrc_id);
		gasrc_data->iir_on = true;
	} else {
		mt8532_afe_gasrc_disable_iir(afe, gasrc_id);
		gasrc_data->iir_on = false;
	}

	/* INT_ADDA ADC (RX Tracking of 26m) */
	if (stream == SNDRV_PCM_STREAM_CAPTURE)
		mt8532_afe_adjust_gasrc_cali_cycles(afe, dai,
			input_fs, output_rate);

	mt8532_afe_gasrc_set_input_fs(afe, dai, input_fs);
	mt8532_afe_gasrc_set_output_fs(afe, dai, output_fs);

	gasrc_data->cali_tx = false;
	gasrc_data->cali_rx = false;
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (mt8532_afe_gasrc_is_tx_tracking(output_fs))
			gasrc_data->cali_tx = true;
		else
			gasrc_data->cali_tx = false;
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (mt8532_afe_gasrc_is_rx_tracking(input_fs))
			gasrc_data->cali_rx = true;
		else
			gasrc_data->cali_rx = false;

		gasrc_data->cali_tx = false;
	}

	switch (mux) {
	case MUX_GASRC_8CH:
	case MUX_GASRC_6CH:
	case MUX_GASRC_4CH:
		gasrc_data->one_heart = true;
		break;
	case MUX_GASRC_2CH:
		gasrc_data->one_heart = false;
		break;
	default:
		gasrc_data->one_heart = false;
		break;
	}
	gasrc_data->one_heart = false;

	dev_dbg(dai->dev, "%s [%d] '%s' cali_tx %d, cali_rx %d oh:%d, iir_on=%d\n",
			__func__, gasrc_id, snd_pcm_stream_str(substream),
			gasrc_data->cali_tx, gasrc_data->cali_rx,
			gasrc_data->one_heart, gasrc_data->iir_on);

	if (gasrc_data->one_heart &&
		(gasrc_id != MT8532_GASRC0))
		regmap_update_bits(afe->regmap, gasrc_ctrl_reg[gasrc_id].con0,
			GASRC_NEW_CON0_ONE_HEART, GASRC_NEW_CON0_ONE_HEART);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		mt8532_afe_gasrc_set_tx_mode_fs(afe,
			dai, input_rate, output_rate);
		if (gasrc_data->cali_tx) {
			mt8532_afe_gasrc_sel_cali_clk(afe, dai);
			mt8532_afe_gasrc_sel_lrck(afe, dai, output_fs);
		}
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		mt8532_afe_gasrc_set_rx_mode_fs(afe,
			dai, input_rate, output_rate);
		if (gasrc_data->cali_rx) {
			mt8532_afe_gasrc_sel_cali_clk(afe,	dai);
			mt8532_afe_gasrc_sel_lrck(afe, dai, input_fs);
		}
	}

	if (gasrc_data->cali_tx || gasrc_data->cali_rx) {
		val = (*gasrc_op_freq_45m) ?
			GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_45M :
			GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_49M;
		mask = GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_MASK;
		regmap_update_bits(afe->regmap, gasrc_ctrl_reg[gasrc_id].con7,
			mask, val);

		val = GASRC_NEW_CON6_FREQ_CALI_CYCLE(*cali_cycles) |
				GASRC_NEW_CON6_COMP_FREQ_RES_EN |
				GASRC_NEW_CON6_FREQ_CALI_BP_DGL |
				GASRC_NEW_CON6_CALI_USE_FREQ_OUT |
				GASRC_NEW_CON6_FREQ_CALI_AUTO_RESTART;

		mask = GASRC_NEW_CON6_FREQ_CALI_CYCLE_MASK |
				GASRC_NEW_CON6_COMP_FREQ_RES_EN |
				GASRC_NEW_CON6_FREQ_CALI_BP_DGL |
				GASRC_NEW_CON6_CALI_USE_FREQ_OUT |
				GASRC_NEW_CON6_FREQ_CALI_AUTO_RESTART;

		regmap_update_bits(afe->regmap, gasrc_ctrl_reg[gasrc_id].con6,
			mask, val);
	}

	return 0;
}

static int mt8532_afe_enable_gasrc(struct snd_soc_dai *dai, int stream)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	struct mt8532_gasrc_data *gasrc_data;
	unsigned int ctrl_reg = 0;
	unsigned int val = 0;
	int ret = 0, counter;
	bool re_enable = false;

	if (gasrc_id < 0)
		return -EINVAL;

	gasrc_data = &afe_priv->gasrc_data[gasrc_id];

	if (gasrc_data->re_enable[stream]) {
		re_enable = true;
		gasrc_data->re_enable[stream] = false;
	}

	counter = atomic_add_return(1, &gasrc_data->ref_cnt);
	if (counter != 1 && !re_enable)
		return 0;

	dev_dbg(dai->dev, "%s [%d] one_heart %d re_enable %d\n",
		__func__, gasrc_id, gasrc_data->one_heart, re_enable);

	if (gasrc_data->cali_tx || gasrc_data->cali_rx) {
		if (gasrc_data->one_heart)
			ctrl_reg = gasrc_ctrl_reg[MT8532_GASRC0].con6;
		else
			ctrl_reg = gasrc_ctrl_reg[gasrc_id].con6;

		val = GASRC_NEW_CON6_CALI_EN;
		regmap_update_bits(afe->regmap, ctrl_reg, val, val);

		val = GASRC_NEW_CON6_AUTO_TUNE_FREQ2 |
				GASRC_NEW_CON6_AUTO_TUNE_FREQ3;
		regmap_update_bits(afe->regmap, ctrl_reg, val, val);
	}

	if (gasrc_data->one_heart)
		ctrl_reg = gasrc_ctrl_reg[MT8532_GASRC0].con0;
	else
		ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

	val = GASRC_NEW_CON0_ASM_ON;
	regmap_update_bits(afe->regmap, ctrl_reg, val, val);

	return ret;
}

static int mt8532_afe_disable_gasrc(struct snd_soc_dai *dai, bool directly)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	struct mt8532_gasrc_data *gasrc_data;
	unsigned int ctrl_reg = 0;
	unsigned int val = 0;
	int ret = 0, counter;

	if (gasrc_id < 0)
		return -EINVAL;

	gasrc_data = &afe_priv->gasrc_data[gasrc_id];

	if (!directly) {
		counter = atomic_sub_return(1, &gasrc_data->ref_cnt);
		if (counter < 0) {
			atomic_add(1, &gasrc_data->ref_cnt);
			return 0;
		} else if (counter > 0)
			return 0;
	}

	dev_dbg(dai->dev, "%s [%d] one_heart %d directly %d\n",
		__func__, gasrc_id, gasrc_data->one_heart, directly);

	if (gasrc_data->one_heart)
		ctrl_reg = gasrc_ctrl_reg[MT8532_GASRC0].con0;
	else
		ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

	val = GASRC_NEW_CON0_ASM_ON;
	regmap_update_bits(afe->regmap, ctrl_reg, val, 0);

	if (gasrc_data->cali_tx || gasrc_data->cali_rx) {
		if (gasrc_data->one_heart)
			ctrl_reg = gasrc_ctrl_reg[MT8532_GASRC0].con6;
		else
			ctrl_reg = gasrc_ctrl_reg[gasrc_id].con6;

		val = GASRC_NEW_CON6_CALI_EN;
		regmap_update_bits(afe->regmap, ctrl_reg, val, 0);
	}

	if (gasrc_data->iir_on)
		mt8532_afe_gasrc_disable_iir(afe, gasrc_id);

	return ret;
}

static int mt8532_afe_gasrc_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);

	dev_dbg(dai->dev, "%s [%d] '%s'\n", __func__,
		gasrc_id, snd_pcm_stream_str(substream));

	mt8532_afe_enable_main_clk(afe);

	mt8532_afe_enable_clk(afe, afe_priv->clocks[MT8532_CLK_TOP_ASML_SEL]);
	mt8532_afe_enable_clk(afe, afe_priv->clocks[MT8532_CLK_TOP_ASMM_SEL]);
	mt8532_afe_enable_clk(afe, afe_priv->clocks[MT8532_CLK_TOP_ASMH_SEL]);

	if (gasrc_id >= MT8532_GASRC0 && gasrc_id <= MT8532_GASRC19)
		mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_GASRC0 +
			gasrc_id - MT8532_GASRC0);

	return 0;
}

static void mt8532_afe_gasrc_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	struct mt8532_gasrc_data *gasrc_data;

	if (gasrc_id < 0)
		return;

	gasrc_data = &afe_priv->gasrc_data[gasrc_id];

	dev_dbg(dai->dev, "%s [%d] '%s'\n", __func__,
		gasrc_id, snd_pcm_stream_str(substream));

	gasrc_data->re_enable[substream->stream] = false;

	if (gasrc_id >= MT8532_GASRC0 && gasrc_id <= MT8532_GASRC19)
		mt8532_afe_disable_top_cg(afe,
			MT8532_TOP_CG_GASRC0 + gasrc_id - MT8532_GASRC0);
#if 0
	switch (gasrc_id) {
	case MT8532_GASRC0:
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_GASRC0);
		break;
	case MT8532_GASRC1:
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_GASRC1);
		break;
	case MT8532_GASRC2:
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_GASRC2);
		break;
	case MT8532_GASRC3:
		mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_GASRC3);
		break;
	default:
		break;
	}
#endif
	mt8532_afe_disable_clk(afe, afe_priv->clocks[MT8532_CLK_TOP_ASMH_SEL]);
	mt8532_afe_disable_clk(afe, afe_priv->clocks[MT8532_CLK_TOP_ASMM_SEL]);
	mt8532_afe_disable_clk(afe, afe_priv->clocks[MT8532_CLK_TOP_ASML_SEL]);

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_gasrc_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const int gasrc_id = mt8532_dai_num_to_gasrc(dai->id);
	struct mt8532_gasrc_data *gasrc_data;
	int counter;

	if (gasrc_id < 0)
		return -EINVAL;

	gasrc_data = &afe_priv->gasrc_data[gasrc_id];

	gasrc_data->duplex = false;
	gasrc_data->op_freq_45m = false;
	gasrc_data->cali_cycles = 64;
	gasrc_data->re_enable[substream->stream] = false;

	counter = atomic_read(&gasrc_data->ref_cnt);

	if (dai->capture_active && dai->playback_active)
		gasrc_data->duplex = true;

	dev_dbg(dai->dev, "%s [%d] '%s' duplex %d\n", __func__,
		gasrc_id, snd_pcm_stream_str(substream),
		gasrc_data->duplex);

	if (gasrc_data->duplex && counter > 0) {
		mt8532_afe_disable_gasrc(dai, true);

		gasrc_data->re_enable[substream->stream] = true;
	}

	mt8532_afe_reset_gasrc(afe, dai);
	mt8532_afe_clear_gasrc(afe, dai);
	//mt8532_afe_gasrc_use_sel(afe, dai, true); //bypass not use in 8532
	mt8532_afe_configure_gasrc(afe, substream, dai);

	return 0;
}

static int mt8532_afe_gasrc_trigger(struct snd_pcm_substream *substream,
	int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	dev_dbg(dai->dev, "%s [%d] '%s' cmd %d\n", __func__,
		mt8532_dai_num_to_gasrc(dai->id),
		snd_pcm_stream_str(substream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		ret = mt8532_afe_enable_gasrc(dai, substream->stream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		ret = mt8532_afe_disable_gasrc(dai, false);
		break;
	default:
		break;
	}

	return ret;
}

static void mt8532_afe_spdif_out_set_128fs(struct mtk_base_afe *afe,
	unsigned int rate)
{
#if 0
	unsigned int spdif_out_src_ck;
	unsigned int spdif_div;
	unsigned int spdif_out_src_apll;

	if (rate % 8000) {
		spdif_out_src_ck = 45158400;
		spdif_out_src_apll = AUD_TCON0_SPDF_OUT_SRC_APLL1;
	} else {
		spdif_out_src_ck = 49152000;
		spdif_out_src_apll = AUD_TCON0_SPDF_OUT_SRC_APLL2;
	}

	spdif_div = spdif_out_src_ck / 128 / rate;

	regmap_update_bits(afe->regmap, AUDIO_TOP_CON0,
			   AUD_TCON0_SPDF_OUT_PLL_SEL_MASK,
			   spdif_out_src_apll);

	regmap_update_bits(afe->regmap, AUDIO_TOP_CON2,
			   AUD_TCON0_CON2_SPDF_DIV_MASK,
			   AUD_TCON0_CON2_SPDF_DIV(spdif_div));
#endif
}

static void mt8532_afe_spdif_out_set_channel_status(struct mtk_base_afe *afe,
	unsigned int rate, unsigned int bit_width)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_control_data *ctrl_data = &afe_priv->ctrl_data;
	unsigned int ch_status_lsb, ch_status_msb;

	ch_status_lsb = 0;

	/* Byte 0, Bit 1 */
	if (ctrl_data->spdif_output_iec61937)
		ch_status_lsb |= 0x2;

	/* Byte 1, category code */
	ch_status_lsb |= (0x19 << 8);

	/* Byte 2, source and channel number */
	/* Do not take into account */
	ch_status_lsb |= (0x0 << 16);

	/* Byte 3, sampling frequency */
	switch (rate) {
	case 32000:
		ch_status_lsb |= (0x3 << 24);
		break;
	case 44100:
		ch_status_lsb |= (0x0 << 24);
		break;
	case 48000:
		ch_status_lsb |= (0x2 << 24);
		break;
	case 88200:
		ch_status_lsb |= (0x8 << 24);
		break;
	case 96000:
		ch_status_lsb |= (0xa << 24);
		break;
	case 176400:
		ch_status_lsb |= (0xc << 24);
		break;
	case 192000:
		ch_status_lsb |= (0xe << 24);
		break;
	default:
		/* use 48K Hz */
		ch_status_lsb |= (0x2 << 24);
		dev_warn(afe->dev, "%s invalid rate %u\n", __func__, rate);
		break;
	}

	ch_status_msb = 0;

	/* Byte 4, word length */
	if (bit_width > 16)
		ch_status_msb |= 0xb; /* 24-bits */
	else
		ch_status_msb |= 0x2; /* 16-bits */

	regmap_update_bits(afe->regmap, AFE_IEC_CHL_STAT0,
			   AFE_IEC_CH_STAT0_SET_MASK, ch_status_lsb);
	regmap_update_bits(afe->regmap, AFE_IEC_CHL_STAT1,
			   AFE_IEC_CH_STAT1_SET_MASK, ch_status_msb);
	regmap_update_bits(afe->regmap, AFE_IEC_CHR_STAT0,
			   AFE_IEC_CH_STAT0_SET_MASK, ch_status_lsb);
	regmap_update_bits(afe->regmap, AFE_IEC_CHR_STAT1,
			   AFE_IEC_CH_STAT1_SET_MASK, ch_status_msb);
}

static int mt8532_afe_spdif_out_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	mt8532_afe_enable_main_clk(afe);

	mt8532_afe_enable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV4]);

	//mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_APLL);
	//mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_APLL2);
	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_SPDIF_OUT);

	return 0;
}

static void mt8532_afe_spdif_out_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	regmap_update_bits(afe->regmap, AFE_IEC_CFG,
			   AFE_IEC_CFG_SW_RST_MASK,
			   AFE_IEC_CFG_SW_RST);

	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_SPDIF_OUT);
	//mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_APLL);
	//mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_APLL2);

	mt8532_afe_disable_clk(afe,
		afe_priv->clocks[MT8532_CLK_TOP_APLL12_DIV4]);

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_spdif_out_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	unsigned int rate = dai->rate;
	unsigned int bit_width = dai->sample_bits;
	snd_pcm_uframes_t period_size = runtime->period_size;
	unsigned int val;

	mt8532_afe_spdif_out_set_128fs(afe, rate);

	mt8532_afe_spdif_out_set_channel_status(afe, rate, bit_width);

	regmap_update_bits(afe->regmap, AFE_IEC_NSNUM,
			   AFE_IEC_NSNUM_SET_MASK,
			   AFE_IEC_NSNUM_SAM_NUM(period_size) |
			   AFE_IEC_NSNUM_INTR_NUM(period_size / 2));

	regmap_update_bits(afe->regmap, AFE_IEC_BURST_INFO,
			   AFE_IEC_BURST_INFO_SET_MASK,
			   0x0);

	regmap_update_bits(afe->regmap, AFE_IEC_BURST_LEN,
			   AFE_IEC_BURST_LEN_SET_MASK,
			   frames_to_bytes(runtime, period_size) * 8);

	val = AFE_IEC_CFG_DATA_SRC_DRAM |
	      AFE_IEC_CFG_PCM_DATA |
	      AFE_IEC_CFG_NO_SW_RST |
	      AFE_IEC_CFG_VALID_DATA |
	      AFE_IEC_CFG_FORCE_UPDATE |
	      AFE_IEC_CFG_FORCE_UPDATE_SIZE(2);

	if (bit_width > 16)
		val |= AFE_IEC_CFG_RAW_24BIT |
		       AFE_IEC_CFG_RAW_24BIT_SWITCH;

	regmap_update_bits(afe->regmap, AFE_IEC_CFG,
			   AFE_IEC_CFG_SET_MASK,
			   val);

	return 0;
}

static int mt8532_afe_spdif_out_trigger(struct snd_pcm_substream *substream,
	int cmd, struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	unsigned int val;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		regmap_write(afe->regmap, AFE_IEC_NSADR, runtime->dma_addr);

		regmap_update_bits(afe->regmap, AFE_SPDIF_OUT_CON0,
				   AFE_SPDIF_OUT_CON0_TIMING_MASK,
				   AFE_SPDIF_OUT_CON0_TIMING_ON);

		regmap_update_bits(afe->regmap, AFE_IEC_BURST_INFO,
				   AFE_IEC_BURST_INFO_READY_MASK,
				   AFE_IEC_BURST_INFO_NOT_READY);

		udelay(2000);

		regmap_update_bits(afe->regmap, AFE_IEC_CFG,
				   AFE_IEC_CFG_EN_MASK,
				   AFE_IEC_CFG_ENABLE_CTRL);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		regmap_update_bits(afe->regmap, AFE_IEC_CFG,
				   AFE_IEC_CFG_EN_MASK,
				   AFE_IEC_CFG_DISABLE_CTRL);

		do {
			regmap_read(afe->regmap, AFE_IEC_BURST_INFO, &val);
		} while (val & AFE_IEC_BURST_INFO_READY_MASK);

		regmap_update_bits(afe->regmap, AFE_SPDIF_OUT_CON0,
				   AFE_SPDIF_OUT_CON0_TIMING_MASK,
				   AFE_SPDIF_OUT_CON0_TIMING_OFF);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mt8532_afe_configure_multi_in(struct mtk_base_afe *afe,
	unsigned int channels,
	unsigned int bit_width,
	unsigned int period_size,
	bool spdif_input)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_multi_in_data *multi_in = &afe_priv->multi_in_data;
	unsigned int val;
	unsigned int period_bytes;

	val = AFE_MPHONE_MULTI_CON0_SDATA0_SEL(0) |
	      AFE_MPHONE_MULTI_CON0_SDATA1_SEL(1) |
	      AFE_MPHONE_MULTI_CON0_SDATA2_SEL(2) |
	      AFE_MPHONE_MULTI_CON0_SDATA3_SEL(3);

	if (spdif_input) {
		if (bit_width == 32) {
			val |= AFE_MPHONE_MULTI_CON0_24BIT_DATA;

			period_bytes = period_size * 4 * channels;
		} else if (bit_width == 24) {
			val |= AFE_MPHONE_MULTI_CON0_24BIT_DATA;

			period_bytes = period_size * 3 * channels;
		} else {
			val |= AFE_MPHONE_MULTI_CON0_16BIT_DATA |
			       AFE_MPHONE_MULTI_CON0_16BIT_SWAP;

			period_bytes = period_size * 4 * channels;
		}
	} else {
		if (bit_width == 24) {
			val |= AFE_MPHONE_MULTI_CON0_24BIT_DATA;

			period_bytes = period_size * 3 * channels;
		} else {
			val |= AFE_MPHONE_MULTI_CON0_16BIT_DATA;

			period_bytes = period_size * 4 * channels;
		}
	}

	if (period_bytes >= 1024) {
		val |= AFE_MPHONE_MULTI_CON0_256DWORD_PERIOD;

		multi_in->period_update_bytes = 1024;
	} else if (period_bytes >= 512) {
		val |= AFE_MPHONE_MULTI_CON0_128DWORD_PERIOD;

		multi_in->period_update_bytes = 512;
	} else if (period_bytes >= 256) {
		val |= AFE_MPHONE_MULTI_CON0_64DWORD_PERIOD;

		multi_in->period_update_bytes = 256;
	} else {
		val |= AFE_MPHONE_MULTI_CON0_32DWORD_PERIOD;

		multi_in->period_update_bytes = 128;
	}

	multi_in->notify_irq_count = period_bytes /
		multi_in->period_update_bytes;

	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON0,
			   AFE_MPHONE_MULTI_CON0_SET_MASK, val);

	val = AFE_MPHONE_MULTI_CON1_CH_NUM(channels) |
	      AFE_MPHONE_MULTI_CON1_BIT_NUM(bit_width) |
	      AFE_MPHONE_MULTI_CON1_SYNC_ON |
	      AFE_MPHONE_MULTI_CON1_NO_RESET |
	      AFE_MPHONE_MULTI_CON1_DET_LONG_PERIOD;

	if (spdif_input) {
		if (bit_width == 32) {
			val |= AFE_MPHONE_MULTI_CON1_NON_COMPACT_MODE |
			       AFE_MPHONE_MULTI_CON1_24BIT_SWAP_BYPASS |
			       AFE_MPHONE_MULTI_CON1_LRCK_32_CYCLE;
		} else if (bit_width == 24) {
			val |= AFE_MPHONE_MULTI_CON1_COMPACT_MODE |
			       AFE_MPHONE_MULTI_CON1_24BIT_SWAP_BYPASS |
			       AFE_MPHONE_MULTI_CON1_LRCK_32_CYCLE;
		} else {
			val |= AFE_MPHONE_MULTI_CON1_NON_COMPACT_MODE |
			       AFE_MPHONE_MULTI_CON1_LRCK_32_CYCLE;
		}
	} else {
		if (channels > 2)
			val |= AFE_MPHONE_MULTI_CON1_HBR_MODE;

		if (bit_width == 32) {
			val |= AFE_MPHONE_MULTI_CON1_NON_COMPACT_MODE |
			       AFE_MPHONE_MULTI_CON1_LRCK_32_CYCLE;
		} else if (bit_width == 24) {
			val |= AFE_MPHONE_MULTI_CON1_COMPACT_MODE |
			       AFE_MPHONE_MULTI_CON1_24BIT_SWAP_BYPASS |
			       AFE_MPHONE_MULTI_CON1_LRCK_32_CYCLE;
		} else {
			val |= AFE_MPHONE_MULTI_CON1_NON_COMPACT_MODE |
			       AFE_MPHONE_MULTI_CON1_LRCK_32_CYCLE;
		}

		if (multi_in->format == MT8532_MULTI_IN_FORMAT_I2S) {
			val |= AFE_MPHONE_MULTI_CON1_DELAY_DATA |
			       AFE_MPHONE_MULTI_CON1_LEFT_ALIGN;
		} else if (multi_in->format == MT8532_MULTI_IN_FORMAT_LJ) {
			val |= AFE_MPHONE_MULTI_CON1_LEFT_ALIGN |
			       AFE_MPHONE_MULTI_CON1_LRCK_INV;
		} else if (multi_in->format == MT8532_MULTI_IN_FORMAT_RJ) {
			val |= AFE_MPHONE_MULTI_CON1_LRCK_INV;
		}

		if (multi_in->bck_inv)
			val ^= AFE_MPHONE_MULTI_CON1_BCK_INV;

		if (multi_in->lrck_inv)
			val ^= AFE_MPHONE_MULTI_CON1_LRCK_INV;
	}

	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON1,
			   AFE_MPHONE_MULTI_CON1_SET_MASK, val);

	val = 0;
	if (spdif_input)
		val |= AFE_MPHONE_MULTI_CON2_SEL_SPDIFIN;
	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON2,
			   AFE_MPHONE_MULTI_CON2_SET_MASK, val);

	if (spdif_input) {
		regmap_update_bits(afe->regmap,
				   AFE_MPHONE_MULTI_DET_REG_CON2,
				   0xFFFFFFFF, 0x4040);
		regmap_update_bits(afe->regmap,
				   AFE_MPHONE_MULTI_DET_REG_CON3,
				   0xFFFFFFFF, 0x5080);
	} else {
		if ((channels == 8) && (bit_width == 16)) {
			regmap_update_bits(afe->regmap,
					   AFE_MPHONE_MULTI_DET_REG_CON2,
					   0xFFFFFFFF, 0x7840);
			regmap_update_bits(afe->regmap,
					   AFE_MPHONE_MULTI_DET_REG_CON3,
					   0xFFFFFFFF, 0x8880);
		} else {
			regmap_update_bits(afe->regmap,
					   AFE_MPHONE_MULTI_DET_REG_CON2,
					   0xFFFFFFFF, 0x4040);
			regmap_update_bits(afe->regmap,
					   AFE_MPHONE_MULTI_DET_REG_CON3,
					   0xFFFFFFFF, 0x5080);
		}
	}

	multi_in->current_irq_count = 0;

	return 0;
}

static int mt8532_afe_enable_multi_in(struct mtk_base_afe *afe,
	bool spdif_input)
{
	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_MON,
		AFE_MPHONE_MULTI_MON_DEC, 0x0);

	/* CLK/DATA REG should enable/disable spdif-in/multi-in. */
	regmap_update_bits(afe->regmap, AFE_SPDIFIN_CFG1,
			AFE_SPDIFIN_CFG1_SEL_DEC0_DATA_EN |
			AFE_SPDIFIN_CFG1_SEL_DEC0_CLK_EN,
			spdif_input ?
			AFE_SPDIFIN_CFG1_SPDIF_IN_CLK_DATA_ENABLE :
			AFE_SPDIFIN_CFG1_MULTI_IN_CLK_DATA_ENABLE);

	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON0,
			   AFE_MPHONE_MULTI_CON0_EN,
			   AFE_MPHONE_MULTI_CON0_EN);

	return 0;
}

static int mt8532_afe_disable_multi_in(struct mtk_base_afe *afe)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	regmap_update_bits(afe->regmap, AFE_MPHONE_MULTI_CON0,
			   AFE_MPHONE_MULTI_CON0_EN, 0x0);

	afe_priv->multi_in_data.current_irq_count = 0;

	clear_multi_in_monitor_info(afe);

	return 0;
}

static int mt8532_afe_spdif_in_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	dev_dbg(dai->dev, "%s\n", __func__);

	mt8532_afe_enable_main_clk(afe);

	return 0;
}

static void mt8532_afe_spdif_in_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;

	dev_dbg(dai->dev, "%s\n", __func__);

	if (clk_tune->props[CLK_TUNE_SPDIF_IN].do_tune) {
		mt8532_afe_stop_clk_tune_hrt(afe);
		mt8532_afe_reset_apll_rate(afe);
		mt8532_afe_reset_clk_tune(afe);
	}

	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_spdif_in_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;
	int ret = 0;

	dev_info(dai->dev, "%s ch:%d bits:%d period_size:%lu\n",
		__func__, dai->channels, dai->sample_bits,
		runtime->period_size);

	if (clk_tune->props[CLK_TUNE_SPDIF_IN].do_tune) {
		mt8532_afe_stop_clk_tune_hrt(afe);
		mt8532_afe_reset_clk_tune(afe);
	}

	ret = mt8532_afe_configure_multi_in(afe,
					    dai->channels,
					    dai->sample_bits,
					    runtime->period_size,
					    true);

	return ret;
}

static int mt8532_afe_spdif_in_trigger(struct snd_pcm_substream *substream,
	int cmd, struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;

	dev_info(dai->dev, "%s cmd %d\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		mt8532_afe_enable_multi_in(afe, true);

		if (clk_tune->props[CLK_TUNE_SPDIF_IN].do_tune) {
			mt8532_afe_configure_clk_tune(afe, CLK_TUNE_SPDIF_IN,
				runtime->rate);

			mt8532_afe_trigger_clk_tune(afe, runtime->period_size);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		mt8532_afe_disable_multi_in(afe);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mt8532_afe_multi_in_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);

	dev_dbg(dai->dev, "%s\n", __func__);

	mt8532_afe_enable_main_clk(afe);

	snd_pcm_hw_constraint_step(substream->runtime, 0,
				   SNDRV_PCM_HW_PARAM_PERIOD_BYTES, 128);

	mt8532_afe_enable_top_cg(afe, MT8532_TOP_CG_MULTI_IN);

	return 0;
}

static void mt8532_afe_multi_in_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;

	dev_dbg(dai->dev, "%s\n", __func__);

	if (clk_tune->props[CLK_TUNE_MULTI_IN].do_tune) {
		mt8532_afe_stop_clk_tune_hrt(afe);
		mt8532_afe_reset_apll_rate(afe);
		mt8532_afe_reset_clk_tune(afe);
	}

	mt8532_afe_disable_top_cg(afe, MT8532_TOP_CG_MULTI_IN);
	mt8532_afe_disable_main_clk(afe);
}

static int mt8532_afe_multi_in_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;
	int ret = 0;

	dev_dbg(dai->dev, "%s ch:%d bits:%d period_size:%lu\n",
		__func__, dai->channels, dai->sample_bits,
		runtime->period_size);

	if (clk_tune->props[CLK_TUNE_MULTI_IN].do_tune) {
		mt8532_afe_stop_clk_tune_hrt(afe);
		mt8532_afe_reset_clk_tune(afe);
	}

	ret = mt8532_afe_configure_multi_in(afe,
					    dai->channels,
					    dai->sample_bits,
					    runtime->period_size,
					    false);

	return ret;
}

static int mt8532_afe_multi_in_set_fmt(struct snd_soc_dai *dai,
	unsigned int fmt)
{
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;

	dev_dbg(dai->dev, "%s fmt 0x%x\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		afe_priv->multi_in_data.format = MT8532_MULTI_IN_FORMAT_I2S;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		afe_priv->multi_in_data.format = MT8532_MULTI_IN_FORMAT_LJ;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		afe_priv->multi_in_data.format = MT8532_MULTI_IN_FORMAT_RJ;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		afe_priv->multi_in_data.bck_inv = false;
		afe_priv->multi_in_data.lrck_inv = false;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		afe_priv->multi_in_data.bck_inv = false;
		afe_priv->multi_in_data.lrck_inv = true;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		afe_priv->multi_in_data.bck_inv = true;
		afe_priv->multi_in_data.lrck_inv = false;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		afe_priv->multi_in_data.bck_inv = true;
		afe_priv->multi_in_data.lrck_inv = true;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mt8532_afe_multi_in_trigger(struct snd_pcm_substream *substream,
	int cmd, struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime * const runtime = substream->runtime;
	struct mtk_base_afe *afe = snd_soc_dai_get_drvdata(dai);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &afe_priv->clk_tune;

	dev_dbg(dai->dev, "%s cmd %d\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		mt8532_afe_enable_multi_in(afe, false);

		if (clk_tune->props[CLK_TUNE_MULTI_IN].do_tune) {
			mt8532_afe_configure_clk_tune(afe, CLK_TUNE_MULTI_IN,
				runtime->rate);

			mt8532_afe_trigger_clk_tune(afe, runtime->period_size);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		mt8532_afe_disable_multi_in(afe);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* FE DAIs */
static const struct snd_soc_dai_ops mt8532_afe_fe_dai_ops = {
	.startup	= mt8532_afe_fe_startup,
	.shutdown	= mt8532_afe_fe_shutdown,
	.hw_params	= mt8532_afe_fe_hw_params,
	.hw_free	= mtk_afe_fe_hw_free,
	.prepare	= mt8532_afe_fe_prepare,
	.trigger	= mt8532_afe_fe_trigger,
	.set_fmt	= mt8532_afe_fe_set_fmt,
};

/* BE DAIs */
static const struct snd_soc_dai_ops mt8532_afe_etdm1_ops = {
	.startup	= mt8532_afe_etdm1_startup,
	.shutdown	= mt8532_afe_etdm1_shutdown,
	.hw_params	= mt8532_afe_etdm1_hw_params,
	.hw_free	= mt8532_afe_etdm1_hw_free,
	.prepare	= mt8532_afe_etdm1_prepare,
	.trigger	= mt8532_afe_etdm1_trigger,
	.set_fmt	= mt8532_afe_etdm_set_fmt,
	.set_tdm_slot	= mt8532_afe_etdm_set_tdm_slot,
	.set_sysclk	= mt8532_afe_etdm_set_sysclk,
};

static const struct snd_soc_dai_ops mt8532_afe_etdm2_ops = {
	.startup	= mt8532_afe_etdm2_startup,
	.shutdown	= mt8532_afe_etdm2_shutdown,
	.hw_params	= mt8532_afe_etdm2_hw_params,
	.hw_free	= mt8532_afe_etdm2_hw_free,
	.prepare	= mt8532_afe_etdm2_prepare,
	.trigger	= mt8532_afe_etdm2_trigger,
	.set_fmt	= mt8532_afe_etdm_set_fmt,
	.set_tdm_slot	= mt8532_afe_etdm_set_tdm_slot,
	.set_sysclk	= mt8532_afe_etdm_set_sysclk,
};

static const struct snd_soc_dai_ops mt8532_afe_etdm3_ops = {
	.startup	= mt8532_afe_etdm3_startup,
	.shutdown	= mt8532_afe_etdm3_shutdown,
	.hw_params	= mt8532_afe_etdm3_hw_params,
	.hw_free	= mt8532_afe_etdm3_hw_free,
	.prepare	= mt8532_afe_etdm3_prepare,
	.trigger	= mt8532_afe_etdm3_trigger,
	.set_fmt	= mt8532_afe_etdm_set_fmt,
	.set_tdm_slot	= mt8532_afe_etdm_set_tdm_slot,
	.set_sysclk	= mt8532_afe_etdm_set_sysclk,
};

static const struct snd_soc_dai_ops mt8532_afe_pcm1_ops = {
	.startup	= mt8532_afe_pcm1_startup,
	.shutdown	= mt8532_afe_pcm1_shutdown,
	.prepare	= mt8532_afe_pcm1_prepare,
	.set_fmt	= mt8532_afe_pcm1_set_fmt,
};

static const struct snd_soc_dai_ops mt8532_afe_dmic_ops = {
	.startup	= mt8532_afe_dmic_startup,
	.shutdown	= mt8532_afe_dmic_shutdown,
	.prepare	= mt8532_afe_dmic_prepare,
};


static const struct snd_soc_dai_ops mt8532_afe_gasrc_ops = {
	.startup	= mt8532_afe_gasrc_startup,
	.shutdown	= mt8532_afe_gasrc_shutdown,
	.prepare	= mt8532_afe_gasrc_prepare,
	.trigger	= mt8532_afe_gasrc_trigger,
};

static const struct snd_soc_dai_ops mt8532_afe_spdif_out_ops = {
	.startup	= mt8532_afe_spdif_out_startup,
	.shutdown	= mt8532_afe_spdif_out_shutdown,
	.prepare	= mt8532_afe_spdif_out_prepare,
	.trigger	= mt8532_afe_spdif_out_trigger,
};

static const struct snd_soc_dai_ops mt8532_afe_spdif_in_ops = {
	.startup	= mt8532_afe_spdif_in_startup,
	.shutdown	= mt8532_afe_spdif_in_shutdown,
	.prepare	= mt8532_afe_spdif_in_prepare,
	.trigger	= mt8532_afe_spdif_in_trigger,
};

static const struct snd_soc_dai_ops mt8532_afe_multi_in_ops = {
	.startup	= mt8532_afe_multi_in_startup,
	.shutdown	= mt8532_afe_multi_in_shutdown,
	.prepare	= mt8532_afe_multi_in_prepare,
	.set_fmt	= mt8532_afe_multi_in_set_fmt,
	.trigger	= mt8532_afe_multi_in_trigger,
};

static struct snd_soc_dai_driver mt8532_afe_pcm_dais[] = {
	/* FE DAIs: memory intefaces to CPU */
	{
		.name = "DLM",  //means dl8
		.id = MT8532_AFE_MEMIF_DLM,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.playback = {
			.stream_name = "DLM",
			.channels_min = 1,
			.channels_max = 24,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
#if 0
	{
		.name = "DL11",
		.id = MT8532_AFE_MEMIF_DL11,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.playback = {
			.stream_name = "DL11",
			.channels_min = 1,
			.channels_max = 48,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
#endif
	{
		.name = "DL2",
		.id = MT8532_AFE_MEMIF_DL2,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.playback = {
			.stream_name = "DL2",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
#if 0
	{
		.name = "DL3",
		.id = MT8532_AFE_MEMIF_DL3,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.playback = {
			.stream_name = "DL3",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	}, {
		.name = "DL6",
		.id = MT8532_AFE_MEMIF_DL6,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.playback = {
			.stream_name = "DL6",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	}, {
		.name = "DL7",
		.id = MT8532_AFE_MEMIF_DL7,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.playback = {
			.stream_name = "DL7",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 |
				 SNDRV_PCM_RATE_88200 |
				 SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_176400 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
#endif
	{
		.name = "DL10",
		.id = MT8532_AFE_MEMIF_DL10,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.playback = {
			.stream_name = "DL10",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	}, {
		.name = "UL1",
		.id = MT8532_AFE_MEMIF_UL1,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL1",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
	{
		.name = "UL2",
		.id = MT8532_AFE_MEMIF_UL2,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL2",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
#if 0

	{
		.name = "UL3",
		.id = MT8532_AFE_MEMIF_UL3,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL3",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
#endif
		{
		.name = "UL4",
		.id = MT8532_AFE_MEMIF_UL4,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL4",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
#if 0
	{
		.name = "UL5",
		.id = MT8532_AFE_MEMIF_UL5,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL5",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	}, {
		.name = "UL6",
		.id = MT8532_AFE_MEMIF_UL6,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL6",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	}, {
		.name = "UL8",
		.id = MT8532_AFE_MEMIF_UL8,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL8",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	}, {
		.name = "UL9",
		.id = MT8532_AFE_MEMIF_UL9,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL9",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	}, {
		.name = "UL10",
		.id = MT8532_AFE_MEMIF_UL10,
		.suspend = mt8532_afe_dai_suspend,
		.resume = mt8532_afe_dai_resume,
		.capture = {
			.stream_name = "UL10",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_fe_dai_ops,
		.bus_control = true,
	},
#endif

	{
	/* BE DAIs */
		.name = "ETDM1_OUT",
		.id = MT8532_AFE_IO_ETDM1_OUT,
		.playback = {
			.stream_name = "ETDM1 Playback",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_etdm1_ops,
	}, {
	/* BE DAIs */
		.name = "ETDM1_IN",
		.id = MT8532_AFE_IO_ETDM1_IN,
		.capture = {
			.stream_name = "ETDM1 Capture",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_etdm1_ops,
	},
	{
	/* BE DAIs */
		.name = "ETDM2_OUT",
		.id = MT8532_AFE_IO_ETDM2_OUT,
		.playback = {
			.stream_name = "ETDM2 Playback",
			.channels_min = 1,
			.channels_max = 24,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_etdm2_ops,
	}, {
	/* BE DAIs */
		.name = "ETDM2_IN",
		.id = MT8532_AFE_IO_ETDM2_IN,
		.capture = {
			.stream_name = "ETDM2 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_etdm2_ops,
	}, {
	/* BE DAIs */
		.name = "ETDM3_OUT",
		.id = MT8532_AFE_IO_ETDM3_OUT,
		.playback = {
			.stream_name = "ETDM3 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_etdm3_ops,
	},
#if 0
	{
	/* BE DAIs */
		.name = "PCM1",
		.id = MT8532_AFE_IO_PCM1,
		.playback = {
			.stream_name = "PCM1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 |
				 SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "PCM1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 |
				 SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_pcm1_ops,
		.symmetric_rates = 1,
		.symmetric_samplebits = 1,
	}, {
	/* BE DAIs */
		.name = "DMIC",
		.id = MT8532_AFE_IO_DMIC,
		.capture = {
			.stream_name = "DMIC Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000 |
				 SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_dmic_ops,
	},
#endif

	{
	/* BE DAIs */
		.name = "GASRC0",
		.id = MT8532_AFE_IO_GASRC0,
		.playback = {
			.stream_name = "GASRC0 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "GASRC0 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_gasrc_ops,
	},
#if 1
	{
	/* BE DAIs */
		.name = "GASRC1",
		.id = MT8532_AFE_IO_GASRC1,
		.playback = {
			.stream_name = "GASRC1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "GASRC1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_gasrc_ops,
	}, {
	/* BE DAIs */
		.name = "GASRC2",
		.id = MT8532_AFE_IO_GASRC2,
		.playback = {
			.stream_name = "GASRC2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "GASRC2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_gasrc_ops,
	}, {
	/* BE DAIs */
		.name = "GASRC3",
		.id = MT8532_AFE_IO_GASRC3,
		.playback = {
			.stream_name = "GASRC3 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "GASRC3 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_gasrc_ops,
	},
#endif

#if 0
	{
	/* BE DAIs */
		.name = "SPDIF_OUT",
		.id = MT8532_AFE_IO_SPDIF_OUT,
		.playback = {
			.stream_name = "SPDIF Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 |
				 SNDRV_PCM_RATE_88200 |
				 SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_176400 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE,
		},
		.ops = &mt8532_afe_spdif_out_ops,
	},
#endif
	{
	/* BE DAIs */
		.name = "SPDIF_IN",
		.id = MT8532_AFE_IO_SPDIF_IN,
		.capture = {
			.stream_name = "SPDIF Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 |
				 SNDRV_PCM_RATE_88200 |
				 SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_176400 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_spdif_in_ops,
	},
	{
	/* BE DAIs */
		.name = "MULTI_IN",
		.id = MT8532_AFE_IO_MULTI_IN,
		.capture = {
			.stream_name = "MULTI Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &mt8532_afe_multi_in_ops,
	},
	{
	/* BE DAIs */
		.name = "VIRTUAL_IN_OUT",
		.id = MT8532_AFE_IO_VIRTUAL_IN,
		.playback = {
			.stream_name = "VIRTUAL Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "VIRTUAL Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S24_3LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		//.ops = &mt8532_afe_multi_in_ops,
	},
};
#if 0
static struct snd_soc_dai_driver *mt8532_get_dai_drv(int id)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(mt8532_afe_pcm_dais); i++) {
		if (mt8532_afe_pcm_dais[i].id == id)
			return &mt8532_afe_pcm_dais[i];
	}

	return NULL;
}
#endif
static int mt8532_snd_soc_dapm_get_enum_double(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm =
		snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_component *comp = snd_soc_dapm_to_component(dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(comp);
	int ret;

	mt8532_afe_enable_reg_rw_clk(afe);

	ret = snd_soc_dapm_get_enum_double(kcontrol, ucontrol);

	mt8532_afe_disable_reg_rw_clk(afe);

	return ret;
}

static int mt8532_snd_soc_dapm_put_enum_double(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm =
		snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_component *comp = snd_soc_dapm_to_component(dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(comp);
	int ret;

	mt8532_afe_enable_reg_rw_clk(afe);

	ret = snd_soc_dapm_put_enum_double(kcontrol, ucontrol);

	mt8532_afe_disable_reg_rw_clk(afe);

	return ret;
}

static const struct snd_kcontrol_new mt8532_afe_o34_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I70 Switch", AFE_CONN34_2, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN34, 12, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I96 Switch", AFE_CONN34_3, 96-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o35_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I71 Switch", AFE_CONN35_2, 7, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN35, 13, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I97 Switch", AFE_CONN35_3, 97-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o00_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I00 Switch", AFE_CONN0, 0, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o01_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I01 Switch", AFE_CONN1, 1, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o02_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I02 Switch", AFE_CONN2, 2, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o03_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I03 Switch", AFE_CONN3, 3, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o04_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I20 Switch", AFE_CONN4, 20, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN4, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I24 Switch", AFE_CONN4, 24, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I26 Switch", AFE_CONN4, 26, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I28 Switch", AFE_CONN4, 28, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN53, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I40 Switch", AFE_CONN53, 8, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o05_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I21 Switch", AFE_CONN5, 21, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN5, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I25 Switch", AFE_CONN5, 25, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I27 Switch", AFE_CONN5, 27, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I29 Switch", AFE_CONN5, 29, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN53, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I41 Switch", AFE_CONN53, 25, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o06_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I24 Switch", AFE_CONN6, 24, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I34 Switch", AFE_CONN54, 2, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o07_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I25 Switch", AFE_CONN7, 25, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I35 Switch", AFE_CONN54, 19, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o08_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I26 Switch", AFE_CONN8, 26, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I36 Switch", AFE_CONN55, 4, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o09_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I27 Switch", AFE_CONN9, 27, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I37 Switch", AFE_CONN55, 21, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o10_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I28 Switch", AFE_CONN10, 28, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN56, 6, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o11_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I29 Switch", AFE_CONN11, 29, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN56, 23, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o12_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I10 Switch", AFE_CONN12, 10, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I18 Switch", AFE_CONN12, 18, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN12, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN57, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN57, 6, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o13_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I11 Switch", AFE_CONN13, 11, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I19 Switch", AFE_CONN13, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN13, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN57, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN57, 23, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o14_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I04 Switch", AFE_CONN14, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I10 Switch", AFE_CONN14, 10, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN14, 12, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I18 Switch", AFE_CONN14, 18, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN14, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN58, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN58, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I42 Switch", AFE_CONN58, 10, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o15_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I05 Switch", AFE_CONN15, 5, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I11 Switch", AFE_CONN15, 11, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN15, 13, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I19 Switch", AFE_CONN15, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN15, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN58, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN58, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I43 Switch", AFE_CONN58, 27, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o16_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I04 Switch", AFE_CONN16, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN16, 12, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN59, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I42 Switch", AFE_CONN59, 10, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o17_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I05 Switch", AFE_CONN17, 5, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN17, 13, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN59, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I43 Switch", AFE_CONN59, 27, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o18_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I04 Switch", AFE_CONN18, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN18, 12, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I42 Switch", AFE_CONN60, 10, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o19_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I05 Switch", AFE_CONN19, 5, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN19, 13, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I43 Switch", AFE_CONN60, 27, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o20_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I20 Switch", AFE_CONN20, 20, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I21 Switch", AFE_CONN20, 21, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN20, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN20, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I24 Switch", AFE_CONN20, 24, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I26 Switch", AFE_CONN20, 26, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I28 Switch", AFE_CONN20, 28, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I40 Switch", AFE_CONN61, 8, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I41 Switch", AFE_CONN61, 9, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o21_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I20 Switch", AFE_CONN21, 20, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I21 Switch", AFE_CONN21, 21, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN21, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN21, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I25 Switch", AFE_CONN21, 25, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I27 Switch", AFE_CONN21, 27, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I29 Switch", AFE_CONN21, 29, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I40 Switch", AFE_CONN61, 24, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I41 Switch", AFE_CONN61, 25, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o26_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I04 Switch", AFE_CONN26, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN26, 12, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN64, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I34 Switch", AFE_CONN64, 2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I42 Switch", AFE_CONN64, 10, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o27_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I05 Switch", AFE_CONN27, 5, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN27, 13, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN64, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I35 Switch", AFE_CONN64, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I43 Switch", AFE_CONN64, 27, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o28_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I06 Switch", AFE_CONN28, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I14 Switch", AFE_CONN28, 14, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN28, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN65, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I34 Switch", AFE_CONN65, 2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN65, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I44 Switch", AFE_CONN65, 12, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o29_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I07 Switch", AFE_CONN29, 7, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I15 Switch", AFE_CONN29, 15, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN29, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN65, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I35 Switch", AFE_CONN65, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN65, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I45 Switch", AFE_CONN65, 29, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o30_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I08 Switch", AFE_CONN30, 8, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I16 Switch", AFE_CONN30, 16, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN30, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN66, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I34 Switch", AFE_CONN66, 2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I36 Switch", AFE_CONN66, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN66, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I46 Switch", AFE_CONN66, 14, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o31_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I09 Switch", AFE_CONN31, 9, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I17 Switch", AFE_CONN31, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN31, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN66, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I35 Switch", AFE_CONN66, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I37 Switch", AFE_CONN66, 21, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN66, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I47 Switch", AFE_CONN66, 31, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o32_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I10 Switch", AFE_CONN32, 10, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I18 Switch", AFE_CONN32, 18, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN32, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN67, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I34 Switch", AFE_CONN67, 2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I36 Switch", AFE_CONN67, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN67, 6, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o33_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I11 Switch", AFE_CONN33, 11, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I19 Switch", AFE_CONN33, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN33, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN67, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I35 Switch", AFE_CONN67, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I37 Switch", AFE_CONN67, 21, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN67, 23, 1, 0),
};
#if 0
static const struct snd_kcontrol_new mt8532_afe_o34_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I04 Switch", AFE_CONN34, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN34, 12, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I22 Switch", AFE_CONN34, 22, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I32 Switch", AFE_CONN68, 0, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I34 Switch", AFE_CONN68, 2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I36 Switch", AFE_CONN68, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN68, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I42 Switch", AFE_CONN68, 10, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o35_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I05 Switch", AFE_CONN35, 5, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN35, 13, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I23 Switch", AFE_CONN35, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I33 Switch", AFE_CONN68, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I35 Switch", AFE_CONN68, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I37 Switch", AFE_CONN68, 21, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN68, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I43 Switch", AFE_CONN68, 27, 1, 0),
};
#endif

static const struct snd_kcontrol_new mt8532_afe_o36_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I06 Switch", AFE_CONN36, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I14 Switch", AFE_CONN36, 14, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I34 Switch", AFE_CONN69, 2, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I36 Switch", AFE_CONN69, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN69, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I44 Switch", AFE_CONN69, 12, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o37_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I07 Switch", AFE_CONN37, 7, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I15 Switch", AFE_CONN37, 15, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I35 Switch", AFE_CONN69, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I37 Switch", AFE_CONN69, 21, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN69, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I45 Switch", AFE_CONN69, 29, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o38_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I08 Switch", AFE_CONN38, 8, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I16 Switch", AFE_CONN38, 16, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I36 Switch", AFE_CONN70, 4, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I38 Switch", AFE_CONN70, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I46 Switch", AFE_CONN70, 14, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o39_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I09 Switch", AFE_CONN39, 9, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I17 Switch", AFE_CONN39, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I37 Switch", AFE_CONN70, 21, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I39 Switch", AFE_CONN70, 23, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I47 Switch", AFE_CONN70, 31, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o96_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN96, 12, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o97_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN97, 13, 1, 0),

};
static const struct snd_kcontrol_new mt8532_afe_o98_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I14 Switch", AFE_CONN98, 14, 1, 0),

};
static const struct snd_kcontrol_new mt8532_afe_o99_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I15 Switch", AFE_CONN99, 15, 1, 0),

};
static const struct snd_kcontrol_new mt8532_afe_o100_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I16 Switch", AFE_CONN100, 16, 1, 0),

};
static const struct snd_kcontrol_new mt8532_afe_o101_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I17 Switch", AFE_CONN101, 17, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o102_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I18 Switch", AFE_CONN102, 18, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o103_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I19 Switch", AFE_CONN103, 19, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o40_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I12 Switch", AFE_CONN40, 12, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I72 Switch", AFE_CONN40_2, 72-64, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I96 Switch", AFE_CONN40_3, 96-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o41_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I13 Switch", AFE_CONN41, 13, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I73 Switch", AFE_CONN41_2, 73-64, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I97 Switch", AFE_CONN41_3, 97-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o42_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I14 Switch", AFE_CONN42, 14, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I74 Switch", AFE_CONN42_2, 74-64, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I98 Switch", AFE_CONN42_3, 98-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o43_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I15 Switch", AFE_CONN43, 15, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I75 Switch", AFE_CONN43_2, 75-64, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I99 Switch", AFE_CONN43_3, 99-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o44_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I16 Switch", AFE_CONN44, 16, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I76 Switch", AFE_CONN44_2, 76-64, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I100 Switch", AFE_CONN44_3, 100-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o45_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I17 Switch", AFE_CONN45, 17, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I77 Switch", AFE_CONN45_2, 77-64, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I101 Switch", AFE_CONN45_3, 101-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o46_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I18 Switch", AFE_CONN46, 18, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I78 Switch", AFE_CONN46_2, 78-64, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I102 Switch", AFE_CONN46_3, 102-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o47_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I19 Switch", AFE_CONN47, 19, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I79 Switch", AFE_CONN47_2, 79-64, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I103 Switch", AFE_CONN47_3, 103-96, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o48_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I70 Switch", AFE_CONN48_2, 6, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I46 Switch", AFE_CONN48_1, 46-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o49_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I71 Switch", AFE_CONN49_2, 7, 1, 0),
	SOC_DAPM_SINGLE_AUTODISABLE("I47 Switch", AFE_CONN49_1, 47-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o50_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I48 Switch", AFE_CONN50_1, 48-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o51_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I49 Switch", AFE_CONN51_1, 49-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o52_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I50 Switch", AFE_CONN52_1, 50-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o53_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I51 Switch", AFE_CONN53_1, 51-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o54_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I52 Switch", AFE_CONN54_1, 52-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o55_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I53 Switch", AFE_CONN55_1, 53-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o56_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I54 Switch", AFE_CONN56_1, 54-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o57_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I55 Switch", AFE_CONN57_1, 55-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o58_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I56 Switch", AFE_CONN58_2, 56-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o59_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I57 Switch", AFE_CONN59_1, 57-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o60_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I58 Switch", AFE_CONN60_1, 58-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o61_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I59 Switch", AFE_CONN61_1, 59-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o62_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I60 Switch", AFE_CONN62_1, 60-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o63_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I61 Switch", AFE_CONN63_1, 61-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o64_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I62 Switch", AFE_CONN64_1, 62-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o65_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I63 Switch", AFE_CONN65_1, 63-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o66_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I64 Switch", AFE_CONN66_2, 64-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o67_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I65 Switch", AFE_CONN67_2, 65-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o68_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I66 Switch", AFE_CONN68_2, 66-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o69_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I67 Switch", AFE_CONN69_2, 67-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o70_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I68 Switch", AFE_CONN70_2, 68-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o71_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I69 Switch", AFE_CONN71_2, 69-64, 1, 0),
};



////
#if 1
static const struct snd_kcontrol_new mt8532_afe_o72_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I46 Switch", AFE_CONN72_1, 46-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o73_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I47 Switch", AFE_CONN73_1, 47-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o74_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I48 Switch", AFE_CONN74_1, 48-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o75_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I49 Switch", AFE_CONN75_1, 49-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o76_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I50 Switch", AFE_CONN76_1, 50-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o77_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I51 Switch", AFE_CONN77_1, 51-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o78_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I52 Switch", AFE_CONN78_1, 52-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o79_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I53 Switch", AFE_CONN79_1, 53-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o80_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I54 Switch", AFE_CONN80_1, 54-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o81_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I55 Switch", AFE_CONN81_1, 55-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o82_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I56 Switch", AFE_CONN82_2, 56-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o83_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I57 Switch", AFE_CONN83_1, 57-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o84_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I58 Switch", AFE_CONN84_1, 58-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o85_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I59 Switch", AFE_CONN85_1, 59-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o86_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I60 Switch", AFE_CONN86_1, 60-32, 1, 0),
};

static const struct snd_kcontrol_new mt8532_afe_o87_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I61 Switch", AFE_CONN87_1, 61-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o88_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I62 Switch", AFE_CONN88_1, 62-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o89_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I63 Switch", AFE_CONN89_1, 63-32, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o90_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I64 Switch", AFE_CONN90_2, 64-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o91_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I65 Switch", AFE_CONN91_2, 65-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o92_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I66 Switch", AFE_CONN92_2, 66-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o93_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I67 Switch", AFE_CONN93_2, 67-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o94_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I68 Switch", AFE_CONN94_2, 68-64, 1, 0),
};
static const struct snd_kcontrol_new mt8532_afe_o95_mix[] = {
	SOC_DAPM_SINGLE_AUTODISABLE("I69 Switch", AFE_CONN95_2, 69-64, 1, 0),
};
#endif



static const char * const i10_i11_mux_text[] = {
	"Dmic3", "Aadc_latch", "Etdm_in1", "None", "Aadc_fifo",
};

static SOC_ENUM_SINGLE_DECL(i10_i11_mux_enum,
	AFE_CONN76, 29, i10_i11_mux_text);

static const struct snd_kcontrol_new i10_i11_mux =
	SOC_DAPM_ENUM_EXT("I10_I11 Source", i10_i11_mux_enum,
		mt8532_snd_soc_dapm_get_enum_double,
		mt8532_snd_soc_dapm_put_enum_double);

static const char * const i18_i19_mux_text[] = {
	"Etdm_in2", "Aadc_latch", "Aadc_fifo",
};

static SOC_ENUM_SINGLE_DECL(i18_i19_mux_enum,
	AFE_CONN76, 27, i18_i19_mux_text);

static const struct snd_kcontrol_new i18_i19_mux =
	SOC_DAPM_ENUM_EXT("I18_I19 Source", i18_i19_mux_enum,
		mt8532_snd_soc_dapm_get_enum_double,
		mt8532_snd_soc_dapm_put_enum_double);

static const char * const ul3_mux_text[] = {
	"Interconn", "Direct",
};

static SOC_ENUM_SINGLE_DECL(ul3_mux_enum,
	ETDM_COWORK_CON1, 24, ul3_mux_text);

static const struct snd_kcontrol_new ul3_mux =
	SOC_DAPM_ENUM_EXT("UL3 Source", ul3_mux_enum,
		mt8532_snd_soc_dapm_get_enum_double,
		mt8532_snd_soc_dapm_put_enum_double);

static const char * const ul1_mux_text[] = {
	"Spdif_In", "Multi_In",
};

static SOC_ENUM_SINGLE_VIRT_DECL(ul1_mux_enum, ul1_mux_text);

static const struct snd_kcontrol_new ul1_mux =
	SOC_DAPM_ENUM("UL1 Source", ul1_mux_enum);

static const struct snd_kcontrol_new dl8_gasrc0_connection_control =
	SOC_DAPM_SINGLE_VIRT("Switch", 1);

static const struct snd_kcontrol_new dl8_gasrc1_connection_control =
	SOC_DAPM_SINGLE_VIRT("Switch", 1);

static const struct snd_kcontrol_new dl8_gasrc2_connection_control =
	SOC_DAPM_SINGLE_VIRT("Switch", 1);

static const struct snd_kcontrol_new dl8_gasrc3_connection_control =
	SOC_DAPM_SINGLE_VIRT("Switch", 1);

static const char * const mt8532_afe_gasrc_text_1[] = {
//	"Gasrc_8ch", "Gasrc_6ch", "Gasrc_4ch", "Gasrc_2ch",
	"Gasrc_8ch", "Gasrc_2ch",
};

static const char * const mt8532_afe_gasrc_text_2[] = {
	"Gasrc_8ch", "Gasrc_6ch", "Gasrc_2ch",
};

static const char * const mt8532_afe_gasrc_text_3[] = {
	"Gasrc_8ch", "Gasrc_2ch",
};

static SOC_ENUM_SINGLE_VIRT_DECL(mt8532_afe_gasrc0_input_enum,
	mt8532_afe_gasrc_text_1);
static SOC_ENUM_SINGLE_VIRT_DECL(mt8532_afe_gasrc1_input_enum,
	mt8532_afe_gasrc_text_1);
static SOC_ENUM_SINGLE_VIRT_DECL(mt8532_afe_gasrc2_input_enum,
	mt8532_afe_gasrc_text_2);
static SOC_ENUM_SINGLE_VIRT_DECL(mt8532_afe_gasrc3_input_enum,
	mt8532_afe_gasrc_text_3);

static SOC_ENUM_SINGLE_VIRT_DECL(mt8532_afe_gasrc0_output_enum,
	mt8532_afe_gasrc_text_1);
static SOC_ENUM_SINGLE_VIRT_DECL(mt8532_afe_gasrc1_output_enum,
	mt8532_afe_gasrc_text_1);
static SOC_ENUM_SINGLE_VIRT_DECL(mt8532_afe_gasrc2_output_enum,
	mt8532_afe_gasrc_text_2);
static SOC_ENUM_SINGLE_VIRT_DECL(mt8532_afe_gasrc3_output_enum,
	mt8532_afe_gasrc_text_3);

static int mt8532_afe_gasrc_string_to_id(const char *name)
{
	int gasrc_id = -1;

	if (strstr(name, "GASRC0"))
		gasrc_id = MT8532_GASRC0;
	else if (strstr(name, "GASRC1"))
		gasrc_id = MT8532_GASRC1;
	else if (strstr(name, "GASRC2"))
		gasrc_id = MT8532_GASRC2;
	else if (strstr(name, "GASRC3"))
		gasrc_id = MT8532_GASRC3;

	return gasrc_id;
}

static int mt8532_afe_gasrc_source_mux_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm =
		snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_component *comp = snd_soc_dapm_to_component(dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(comp);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	int gasrc_id = mt8532_afe_gasrc_string_to_id(name);

	if (gasrc_id < 0)
		return -EINVAL;

	ucontrol->value.enumerated.item[0] =
		afe_priv->gasrc_data[gasrc_id].input_mux;

	return 0;
}

static int mt8532_afe_gasrc_source_mux_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm =
		snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_component *comp = snd_soc_dapm_to_component(dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(comp);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	const char *name = kcontrol->id.name;
	int gasrc_id = mt8532_afe_gasrc_string_to_id(name);

	if (gasrc_id < 0)
		return -EINVAL;

	if (ucontrol->value.enumerated.item[0] >= e->items)
		return -EINVAL;

	afe_priv->gasrc_data[gasrc_id].input_mux =
		ucontrol->value.enumerated.item[0];

	return 0;
}

static int mt8532_afe_gasrc_sink_mux_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm =
		snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_component *comp = snd_soc_dapm_to_component(dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(comp);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	const char *name = kcontrol->id.name;
	int gasrc_id = mt8532_afe_gasrc_string_to_id(name);

	if (gasrc_id < 0)
		return -EINVAL;

	ucontrol->value.enumerated.item[0] =
		afe_priv->gasrc_data[gasrc_id].output_mux;

	return 0;
}

static int mt8532_afe_gasrc_sink_mux_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm =
		snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_component *comp = snd_soc_dapm_to_component(dapm);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(comp);
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	const char *name = kcontrol->id.name;
	int gasrc_id = mt8532_afe_gasrc_string_to_id(name);

	if (gasrc_id < 0)
		return -EINVAL;

	if (ucontrol->value.enumerated.item[0] >= e->items)
		return -EINVAL;

	afe_priv->gasrc_data[gasrc_id].output_mux =
		ucontrol->value.enumerated.item[0];

	return 0;
}

static const struct snd_kcontrol_new mt8532_afe_gasrc0_input_mux =
	SOC_DAPM_ENUM_EXT("GASRC0 Source", mt8532_afe_gasrc0_input_enum,
		mt8532_afe_gasrc_source_mux_get,
		mt8532_afe_gasrc_source_mux_put);
static const struct snd_kcontrol_new mt8532_afe_gasrc1_input_mux =
	SOC_DAPM_ENUM_EXT("GASRC1 Source", mt8532_afe_gasrc1_input_enum,
		mt8532_afe_gasrc_source_mux_get,
		mt8532_afe_gasrc_source_mux_put);
static const struct snd_kcontrol_new mt8532_afe_gasrc2_input_mux =
	SOC_DAPM_ENUM_EXT("GASRC2 Source", mt8532_afe_gasrc2_input_enum,
		mt8532_afe_gasrc_source_mux_get,
		mt8532_afe_gasrc_source_mux_put);
static const struct snd_kcontrol_new mt8532_afe_gasrc3_input_mux =
	SOC_DAPM_ENUM_EXT("GASRC3 Source", mt8532_afe_gasrc3_input_enum,
		mt8532_afe_gasrc_source_mux_get,
		mt8532_afe_gasrc_source_mux_put);

static const struct snd_kcontrol_new mt8532_afe_gasrc0_output_mux =
	SOC_DAPM_ENUM_EXT("GASRC0 Sink", mt8532_afe_gasrc0_output_enum,
		mt8532_afe_gasrc_sink_mux_get,
		mt8532_afe_gasrc_sink_mux_put);
static const struct snd_kcontrol_new mt8532_afe_gasrc1_output_mux =
	SOC_DAPM_ENUM_EXT("GASRC1 Sink", mt8532_afe_gasrc1_output_enum,
		mt8532_afe_gasrc_sink_mux_get,
		mt8532_afe_gasrc_sink_mux_put);
static const struct snd_kcontrol_new mt8532_afe_gasrc2_output_mux =
	SOC_DAPM_ENUM_EXT("GASRC2 Sink", mt8532_afe_gasrc2_output_enum,
		mt8532_afe_gasrc_sink_mux_get,
		mt8532_afe_gasrc_sink_mux_put);
static const struct snd_kcontrol_new mt8532_afe_gasrc3_output_mux =
	SOC_DAPM_ENUM_EXT("GASRC3 Sink", mt8532_afe_gasrc3_output_enum,
		mt8532_afe_gasrc_sink_mux_get,
		mt8532_afe_gasrc_sink_mux_put);






static const struct snd_soc_dapm_widget mt8532_afe_pcm_widgets[] = {
#if 0
	SND_SOC_DAPM_MIXER("I70L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I71L", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_OUTPUT("Virtual_DL_Out"),
	SND_SOC_DAPM_INPUT("Virtual_UL_In"),
#endif

	SND_SOC_DAPM_MIXER("O34", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o34_mix, ARRAY_SIZE(mt8532_afe_o34_mix)), //ul4
	SND_SOC_DAPM_MIXER("O35", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o35_mix, ARRAY_SIZE(mt8532_afe_o35_mix)),

	SND_SOC_DAPM_MIXER("I70", SND_SOC_NOPM, 0, 0, NULL, 0),//dl2
	SND_SOC_DAPM_MIXER("I71", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("I46", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I47", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I48", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I49", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I50", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I51", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I52", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I53", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I54", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I55", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I56", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I57", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I58", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I59", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I60", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I61", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I62", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I63", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I64", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I65", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I66", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I67", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I68", SND_SOC_NOPM, 0, 0, NULL, 0),//dl8 dlm
	SND_SOC_DAPM_MIXER("I69", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("I96", SND_SOC_NOPM, 0, 0, NULL, 0),//gasrc0in
	SND_SOC_DAPM_MIXER("I97", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I98", SND_SOC_NOPM, 0, 0, NULL, 0),//gasrc1in
	SND_SOC_DAPM_MIXER("I99", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I100", SND_SOC_NOPM, 0, 0, NULL, 0),//gasrc2in
	SND_SOC_DAPM_MIXER("I101", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I102", SND_SOC_NOPM, 0, 0, NULL, 0),//gasrc3in
	SND_SOC_DAPM_MIXER("I103", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("O96", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o96_mix, ARRAY_SIZE(mt8532_afe_o96_mix)), //gasrc0out
	SND_SOC_DAPM_MIXER("O97", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o97_mix, ARRAY_SIZE(mt8532_afe_o97_mix)),
	SND_SOC_DAPM_MIXER("O98", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o98_mix, ARRAY_SIZE(mt8532_afe_o98_mix)), //gasrc1out
	SND_SOC_DAPM_MIXER("O99", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o99_mix, ARRAY_SIZE(mt8532_afe_o99_mix)),
	SND_SOC_DAPM_MIXER("O100", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o100_mix,
		ARRAY_SIZE(mt8532_afe_o100_mix)), //gasrc2out
	SND_SOC_DAPM_MIXER("O101", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o101_mix, ARRAY_SIZE(mt8532_afe_o101_mix)),
	SND_SOC_DAPM_MIXER("O102", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o102_mix,
		ARRAY_SIZE(mt8532_afe_o102_mix)), //gasrc3out
	SND_SOC_DAPM_MIXER("O103", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o103_mix, ARRAY_SIZE(mt8532_afe_o103_mix)),

	SND_SOC_DAPM_MIXER("O40", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o40_mix, ARRAY_SIZE(mt8532_afe_o40_mix)), //ul2
	SND_SOC_DAPM_MIXER("O41", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o41_mix, ARRAY_SIZE(mt8532_afe_o41_mix)),
	SND_SOC_DAPM_MIXER("O42", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o42_mix, ARRAY_SIZE(mt8532_afe_o42_mix)), //ul2
	SND_SOC_DAPM_MIXER("O43", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o43_mix, ARRAY_SIZE(mt8532_afe_o43_mix)),
	SND_SOC_DAPM_MIXER("O44", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o44_mix, ARRAY_SIZE(mt8532_afe_o44_mix)), //ul2
	SND_SOC_DAPM_MIXER("O45", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o45_mix, ARRAY_SIZE(mt8532_afe_o45_mix)),
	SND_SOC_DAPM_MIXER("O46", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o46_mix, ARRAY_SIZE(mt8532_afe_o46_mix)), //ul2
	SND_SOC_DAPM_MIXER("O47", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o47_mix, ARRAY_SIZE(mt8532_afe_o47_mix)),

	SND_SOC_DAPM_MIXER("I12", SND_SOC_NOPM, 0, 0, NULL, 0), //etdmin2
	SND_SOC_DAPM_MIXER("I13", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I14", SND_SOC_NOPM, 0, 0, NULL, 0), //etdmin2
	SND_SOC_DAPM_MIXER("I15", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I16", SND_SOC_NOPM, 0, 0, NULL, 0), //etdmin2
	SND_SOC_DAPM_MIXER("I17", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I18", SND_SOC_NOPM, 0, 0, NULL, 0), //etdmin2
	SND_SOC_DAPM_MIXER("I19", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("I72", SND_SOC_NOPM, 0, 0, NULL, 0), //etdmin1
	SND_SOC_DAPM_MIXER("I73", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I74", SND_SOC_NOPM, 0, 0, NULL, 0), //etdmin1
	SND_SOC_DAPM_MIXER("I75", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I76", SND_SOC_NOPM, 0, 0, NULL, 0), //etdmin1
	SND_SOC_DAPM_MIXER("I77", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("I78", SND_SOC_NOPM, 0, 0, NULL, 0), //etdmin1
	SND_SOC_DAPM_MIXER("I79", SND_SOC_NOPM, 0, 0, NULL, 0),


	SND_SOC_DAPM_MIXER("O48", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o48_mix, ARRAY_SIZE(mt8532_afe_o48_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O49", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o49_mix, ARRAY_SIZE(mt8532_afe_o49_mix)),
	SND_SOC_DAPM_MIXER("O50", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o50_mix, ARRAY_SIZE(mt8532_afe_o50_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O51", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o51_mix, ARRAY_SIZE(mt8532_afe_o51_mix)),
	SND_SOC_DAPM_MIXER("O52", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o52_mix, ARRAY_SIZE(mt8532_afe_o52_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O53", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o53_mix, ARRAY_SIZE(mt8532_afe_o53_mix)),
	SND_SOC_DAPM_MIXER("O54", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o54_mix, ARRAY_SIZE(mt8532_afe_o54_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O55", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o55_mix, ARRAY_SIZE(mt8532_afe_o55_mix)),
	SND_SOC_DAPM_MIXER("O56", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o56_mix, ARRAY_SIZE(mt8532_afe_o56_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O57", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o57_mix, ARRAY_SIZE(mt8532_afe_o57_mix)),
	SND_SOC_DAPM_MIXER("O58", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o58_mix, ARRAY_SIZE(mt8532_afe_o58_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O59", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o59_mix, ARRAY_SIZE(mt8532_afe_o59_mix)),
	SND_SOC_DAPM_MIXER("O60", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o60_mix, ARRAY_SIZE(mt8532_afe_o60_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O61", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o61_mix, ARRAY_SIZE(mt8532_afe_o61_mix)),
	SND_SOC_DAPM_MIXER("O62", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o62_mix, ARRAY_SIZE(mt8532_afe_o62_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O63", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o63_mix, ARRAY_SIZE(mt8532_afe_o63_mix)),
	SND_SOC_DAPM_MIXER("O64", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o64_mix, ARRAY_SIZE(mt8532_afe_o64_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O65", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o65_mix, ARRAY_SIZE(mt8532_afe_o65_mix)),
	SND_SOC_DAPM_MIXER("O66", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o66_mix, ARRAY_SIZE(mt8532_afe_o66_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O67", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o67_mix, ARRAY_SIZE(mt8532_afe_o67_mix)),
	SND_SOC_DAPM_MIXER("O68", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o68_mix, ARRAY_SIZE(mt8532_afe_o68_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O69", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o69_mix, ARRAY_SIZE(mt8532_afe_o69_mix)),
	SND_SOC_DAPM_MIXER("O70", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o70_mix, ARRAY_SIZE(mt8532_afe_o70_mix)), //etdmout2
	SND_SOC_DAPM_MIXER("O71", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o71_mix, ARRAY_SIZE(mt8532_afe_o71_mix)),
#if 1
	SND_SOC_DAPM_MIXER("O72", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o72_mix, ARRAY_SIZE(mt8532_afe_o72_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O73", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o73_mix, ARRAY_SIZE(mt8532_afe_o73_mix)),
	SND_SOC_DAPM_MIXER("O74", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o74_mix, ARRAY_SIZE(mt8532_afe_o74_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O75", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o75_mix, ARRAY_SIZE(mt8532_afe_o75_mix)),
	SND_SOC_DAPM_MIXER("O76", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o76_mix, ARRAY_SIZE(mt8532_afe_o76_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O77", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o77_mix, ARRAY_SIZE(mt8532_afe_o77_mix)),
	SND_SOC_DAPM_MIXER("O78", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o78_mix, ARRAY_SIZE(mt8532_afe_o78_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O79", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o79_mix, ARRAY_SIZE(mt8532_afe_o79_mix)),
	SND_SOC_DAPM_MIXER("O80", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o80_mix, ARRAY_SIZE(mt8532_afe_o80_mix)), //etdmou1
	SND_SOC_DAPM_MIXER("O81", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o81_mix, ARRAY_SIZE(mt8532_afe_o81_mix)),
	SND_SOC_DAPM_MIXER("O82", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o82_mix, ARRAY_SIZE(mt8532_afe_o82_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O83", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o83_mix, ARRAY_SIZE(mt8532_afe_o83_mix)),
	SND_SOC_DAPM_MIXER("O84", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o84_mix, ARRAY_SIZE(mt8532_afe_o84_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O85", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o85_mix, ARRAY_SIZE(mt8532_afe_o85_mix)),
	SND_SOC_DAPM_MIXER("O86", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o86_mix, ARRAY_SIZE(mt8532_afe_o86_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O87", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o87_mix, ARRAY_SIZE(mt8532_afe_o87_mix)),
	SND_SOC_DAPM_MIXER("O88", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o88_mix, ARRAY_SIZE(mt8532_afe_o88_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O89", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o89_mix, ARRAY_SIZE(mt8532_afe_o89_mix)),
	SND_SOC_DAPM_MIXER("O90", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o90_mix, ARRAY_SIZE(mt8532_afe_o90_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O91", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o91_mix, ARRAY_SIZE(mt8532_afe_o91_mix)),
	SND_SOC_DAPM_MIXER("O92", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o92_mix, ARRAY_SIZE(mt8532_afe_o92_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O93", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o93_mix, ARRAY_SIZE(mt8532_afe_o93_mix)),
	SND_SOC_DAPM_MIXER("O94", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o94_mix, ARRAY_SIZE(mt8532_afe_o94_mix)), //etdmout1
	SND_SOC_DAPM_MIXER("O95", SND_SOC_NOPM, 0, 0,
		mt8532_afe_o95_mix, ARRAY_SIZE(mt8532_afe_o95_mix)),
#endif
	SND_SOC_DAPM_MUX("UL1 Mux", SND_SOC_NOPM, 0, 0, &ul1_mux),

	SND_SOC_DAPM_MUX("GASRC0 Input Mux", SND_SOC_NOPM, 0, 0,
		&mt8532_afe_gasrc0_input_mux),
	SND_SOC_DAPM_MUX("GASRC0 Output Mux", SND_SOC_NOPM, 0, 0,
		&mt8532_afe_gasrc0_output_mux),
};

static const struct snd_soc_dapm_route mt8532_afe_pcm_routes[] = {
	{"O34", "I12 Switch", "I12"}, //etdmin2->ul4
	{"O35", "I13 Switch", "I13"},
	{"O34", "I96 Switch", "I96"}, //garcout0->ul4
	{"O35", "I97 Switch", "I97"},
	{"UL4", NULL, "O34"},
	{"UL4", NULL, "O35"},

	{"I46", NULL, "DLM"},
	{"I47", NULL, "DLM"},
#if 1
	{"I48", NULL, "DLM"},
	{"I49", NULL, "DLM"},
	{"I50", NULL, "DLM"},
	{"I51", NULL, "DLM"},
	{"I52", NULL, "DLM"},
	{"I53", NULL, "DLM"},
	{"I54", NULL, "DLM"},
	{"I55", NULL, "DLM"},
	{"I56", NULL, "DLM"},
	{"I57", NULL, "DLM"},
	{"I58", NULL, "DLM"},
	{"I59", NULL, "DLM"},
	{"I60", NULL, "DLM"},
	{"I61", NULL, "DLM"},
	{"I62", NULL, "DLM"},
	{"I63", NULL, "DLM"},
	{"I64", NULL, "DLM"},
	{"I65", NULL, "DLM"},
	{"I66", NULL, "DLM"},
	{"I67", NULL, "DLM"},
	{"I68", NULL, "DLM"},
	{"I69", NULL, "DLM"},
#endif
	{"O48", "I46 Switch", "I46"},  //dl8->etdmout2
	{"O49", "I47 Switch", "I47"},
#if 1
	{"O50", "I48 Switch", "I48"},
	{"O51", "I49 Switch", "I49"},
	{"O52", "I50 Switch", "I50"},
	{"O53", "I51 Switch", "I51"},
	{"O54", "I52 Switch", "I52"},
	{"O55", "I53 Switch", "I53"},
	{"O56", "I54 Switch", "I54"},
	{"O57", "I55 Switch", "I55"},
	{"O58", "I56 Switch", "I56"},
	{"O59", "I57 Switch", "I57"},
	{"O60", "I58 Switch", "I58"},
	{"O61", "I59 Switch", "I59"},
	{"O62", "I60 Switch", "I60"},
	{"O63", "I61 Switch", "I61"},
	{"O64", "I62 Switch", "I62"},
	{"O65", "I63 Switch", "I63"},
	{"O66", "I64 Switch", "I64"},
	{"O67", "I65 Switch", "I65"},
	{"O68", "I66 Switch", "I66"},
	{"O69", "I67 Switch", "I67"},
	{"O70", "I68 Switch", "I68"},
	{"O71", "I69 Switch", "I69"},  //dl8->etdmout2
#endif

	{"ETDM2 Playback", NULL, "O48"},
	{"ETDM2 Playback", NULL, "O49"},
#if 1
	{"ETDM2 Playback", NULL, "O50"},
	{"ETDM2 Playback", NULL, "O51"},
	{"ETDM2 Playback", NULL, "O52"},
	{"ETDM2 Playback", NULL, "O53"},
	{"ETDM2 Playback", NULL, "O54"},
	{"ETDM2 Playback", NULL, "O55"},
	{"ETDM2 Playback", NULL, "O56"},
	{"ETDM2 Playback", NULL, "O57"},
	{"ETDM2 Playback", NULL, "O58"},
	{"ETDM2 Playback", NULL, "O59"},
	{"ETDM2 Playback", NULL, "O60"},
	{"ETDM2 Playback", NULL, "O61"},
	{"ETDM2 Playback", NULL, "O62"},
	{"ETDM2 Playback", NULL, "O63"},
	{"ETDM2 Playback", NULL, "O64"},
	{"ETDM2 Playback", NULL, "O65"},
	{"ETDM2 Playback", NULL, "O66"},
	{"ETDM2 Playback", NULL, "O67"},
	{"ETDM2 Playback", NULL, "O68"},
	{"ETDM2 Playback", NULL, "O69"},
	{"ETDM2 Playback", NULL, "O70"},
	{"ETDM2 Playback", NULL, "O71"},
#endif

#if 1
	{"O72", "I46 Switch", "I46"},  //dl8->etdmout1
	{"O73", "I47 Switch", "I47"},
	{"O74", "I48 Switch", "I48"},
	{"O75", "I49 Switch", "I49"},
	{"O76", "I50 Switch", "I50"},
	{"O77", "I51 Switch", "I51"},
	{"O78", "I52 Switch", "I52"},
	{"O79", "I53 Switch", "I53"},
	{"O80", "I54 Switch", "I54"},
	{"O81", "I55 Switch", "I55"},
	{"O82", "I56 Switch", "I56"},
	{"O83", "I57 Switch", "I57"},
	{"O84", "I58 Switch", "I58"},
	{"O85", "I59 Switch", "I59"},
	{"O86", "I60 Switch", "I60"},
	{"O87", "I61 Switch", "I61"},
	{"O88", "I62 Switch", "I62"},
	{"O89", "I63 Switch", "I63"},
	{"O90", "I64 Switch", "I64"},
	{"O91", "I65 Switch", "I65"},
	{"O92", "I66 Switch", "I66"},
	{"O93", "I67 Switch", "I67"},
	{"O94", "I68 Switch", "I68"},
	{"O95", "I69 Switch", "I69"},  //dl8->etdmout1

	{"ETDM1 Playback", NULL, "O72"},
	{"ETDM1 Playback", NULL, "O73"},
	{"ETDM1 Playback", NULL, "O74"},
	{"ETDM1 Playback", NULL, "O75"},
	{"ETDM1 Playback", NULL, "O76"},
	{"ETDM1 Playback", NULL, "O77"},
	{"ETDM1 Playback", NULL, "O78"},
	{"ETDM1 Playback", NULL, "O79"},
	{"ETDM1 Playback", NULL, "O80"},
	{"ETDM1 Playback", NULL, "O81"},
	{"ETDM1 Playback", NULL, "O82"},
	{"ETDM1 Playback", NULL, "O83"},
	{"ETDM1 Playback", NULL, "O84"},
	{"ETDM1 Playback", NULL, "O85"},
	{"ETDM1 Playback", NULL, "O86"},
	{"ETDM1 Playback", NULL, "O87"},
	{"ETDM1 Playback", NULL, "O88"},
	{"ETDM1 Playback", NULL, "O89"},
	{"ETDM1 Playback", NULL, "O90"},
	{"ETDM1 Playback", NULL, "O91"},
	{"ETDM1 Playback", NULL, "O92"},
	{"ETDM1 Playback", NULL, "O93"},
	{"ETDM1 Playback", NULL, "O94"},
	{"ETDM1 Playback", NULL, "O95"},
#endif

	//{"GASRC0 Capture", NULL, "ETDM2 Capture"}, //etdm in2

	{"I12", NULL, "ETDM2 Capture"}, //etdm in2
	{"I13", NULL, "ETDM2 Capture"},
	//loopback from etdm out3 to mphone multi
	//{"MULTI Capture", NULL, "Virtual_UL_In"},
	//{"UL1", NULL, "MULTI Capture"},
	{"UL1 Mux", "Spdif_In", "SPDIF Capture"},
	{"UL1 Mux", "Multi_In", "MULTI Capture"},
	{"UL1", NULL, "UL1 Mux"},
	//{"Virtual_DL_Out", NULL, "ETDM3 Playback"},
	{"ETDM3 Playback", NULL, "DL10"},
#if 1
	{"I14", NULL, "ETDM2 Capture"},
	{"I15", NULL, "ETDM2 Capture"},
	{"I16", NULL, "ETDM2 Capture"},
	{"I17", NULL, "ETDM2 Capture"},
	{"I18", NULL, "ETDM2 Capture"},
	{"I19", NULL, "ETDM2 Capture"},
#endif
	{"I72", NULL, "ETDM1 Capture"}, //etdm in1
	{"I73", NULL, "ETDM1 Capture"},
	{"I74", NULL, "ETDM1 Capture"},
	{"I75", NULL, "ETDM1 Capture"},
	{"I76", NULL, "ETDM1 Capture"},
	{"I77", NULL, "ETDM1 Capture"},
	{"I78", NULL, "ETDM1 Capture"},
	{"I79", NULL, "ETDM1 Capture"},

	{"O96", "I12 Switch", "I12"}, //etdmin2->gasrc0in
	{"O97", "I13 Switch", "I13"},
#if 1
	{"O98", "I14 Switch", "I14"},
	{"O99", "I15 Switch", "I15"},
	{"O100", "I16 Switch", "I16"},
	{"O101", "I17 Switch", "I17"},
	{"O102", "I18 Switch", "I18"},
	{"O103", "I19 Switch", "I19"},  //etdmin2->gasrc3in
#endif
	//{"GASRC0 Input Mux", "Gasrc_8ch", "O96"},
	//{"GASRC0 Input Mux", "Gasrc_8ch", "O97"},
	//{"GASRC0 Input Mux", "Gasrc_6ch", "O96"},
	//{"GASRC0 Input Mux", "Gasrc_6ch", "O97"},
	//{"GASRC0 Input Mux", "Gasrc_4ch", "O96"},
	//{"GASRC0 Input Mux", "Gasrc_4ch", "O97"},
	//{"GASRC0 Input Mux", "Gasrc_2ch", "O96"},
	//{"GASRC0 Input Mux", "Gasrc_2ch", "O97"},
	//{"GASRC0 Playback", NULL, "GASRC0 Input Mux"},
	//{"GASRC0 Capture", NULL, "GASRC0 Input Mux"}, //gasrc0in->gasrc0
	//{"GASRC0 Capture", NULL, "O97"},

	{"GASRC0 Capture", NULL, "O96"},
	{"GASRC0 Capture", NULL, "O97"},
	{"GASRC1 Capture", NULL, "O98"},
	{"GASRC1 Capture", NULL, "O99"},
	{"GASRC2 Capture", NULL, "O100"},
	{"GASRC2 Capture", NULL, "O101"},
	{"GASRC3 Capture", NULL, "O102"},
	{"GASRC3 Capture", NULL, "O103"},

	{"GASRC0 Playback", NULL, "O96"},
	{"GASRC0 Playback", NULL, "O97"},
	{"GASRC1 Playback", NULL, "O98"},
	{"GASRC1 Playback", NULL, "O99"},
	{"GASRC2 Playback", NULL, "O100"},
	{"GASRC2 Playback", NULL, "O101"},
	{"GASRC3 Playback", NULL, "O102"},
	{"GASRC3 Playback", NULL, "O103"},

	{"I96", NULL, "GASRC0 Capture"},
	{"I97", NULL, "GASRC0 Capture"},
	{"I98", NULL, "GASRC1 Capture"},
	{"I99", NULL, "GASRC1 Capture"},
	{"I100", NULL, "GASRC2 Capture"},
	{"I101", NULL, "GASRC2 Capture"},
	{"I102", NULL, "GASRC3 Capture"},
	{"I103", NULL, "GASRC3 Capture"},

	{"I96", NULL, "GASRC0 Playback"},
	{"I97", NULL, "GASRC0 Playback"},
	{"I98", NULL, "GASRC1 Playback"},
	{"I99", NULL, "GASRC1 Playback"},
	{"I100", NULL, "GASRC2 Playback"},
	{"I101", NULL, "GASRC2 Playback"},
	{"I102", NULL, "GASRC3 Playback"},
	{"I103", NULL, "GASRC3 Playback"},

	{"O40", "I96 Switch", "I96"}, //gasrc0out->ul2
	{"O41", "I97 Switch", "I97"},
#if 1
	{"O42", "I98 Switch", "I98"},
	{"O43", "I99 Switch", "I99"},
	{"O44", "I100 Switch", "I100"},
	{"O45", "I101 Switch", "I101"},
	{"O46", "I102 Switch", "I102"},
	{"O47", "I103 Switch", "I103"},  //gasrc3out->ul2
#endif

#if 1
	{"O40", "I12 Switch", "I12"},  //etdm in2->UL2
	{"O41", "I13 Switch", "I13"},
	{"O42", "I14 Switch", "I14"},  //etdm in2
	{"O43", "I15 Switch", "I15"},
	{"O44", "I16 Switch", "I16"},  //etdm in2
	{"O45", "I17 Switch", "I17"},
	{"O46", "I18 Switch", "I18"},  //etdm in2
	{"O47", "I19 Switch", "I19"},
#endif

	{"O40", "I72 Switch", "I72"},  //etdm in1->UL2
	{"O41", "I73 Switch", "I73"},
	{"O42", "I74 Switch", "I74"},  //etdm in1
	{"O43", "I75 Switch", "I75"},
	{"O44", "I76 Switch", "I76"},  //etdm in1
	{"O45", "I77 Switch", "I77"},
	{"O46", "I78 Switch", "I78"},  //etdm in1
	{"O47", "I79 Switch", "I79"},

	{"UL2", NULL, "O40"},
	{"UL2", NULL, "O41"},
#if 1
	{"UL2", NULL, "O42"},
	{"UL2", NULL, "O43"},
	{"UL2", NULL, "O44"},
	{"UL2", NULL, "O45"},
	{"UL2", NULL, "O46"},
	{"UL2", NULL, "O47"},
#endif

};

static const struct snd_soc_dapm_route mt8532_afe_24ch_output_routes[] = {
	{"ETDM2 Playback", NULL, "DL10"},
};

static const struct snd_soc_component_driver mt8532_afe_pcm_dai_component = {
	.name = "mt8532-afe-pcm-dai",
	.dapm_widgets = mt8532_afe_pcm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt8532_afe_pcm_widgets),
	.dapm_routes = mt8532_afe_pcm_routes,
	.num_dapm_routes = ARRAY_SIZE(mt8532_afe_pcm_routes),
};

static const struct mtk_base_memif_data memif_data[MT8532_AFE_MEMIF_NUM] = {
	{
		.name = "DLM",
		.id = MT8532_AFE_MEMIF_DLM,
		.reg_ofs_base = AFE_DL8_BASE,
		.reg_ofs_cur = AFE_DL8_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON1,
		.fs_shift = 10,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = AFE_DL8_CON0,
		.hd_shift = 6,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 24,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 24,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 24,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 24,
		.ch_config_reg = AFE_DL8_CON0,
		.ch_config_shift = 0,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 28,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "DL11",
		.id = MT8532_AFE_MEMIF_DL11,
		.reg_ofs_base = AFE_DL11_BASE,
		.reg_ofs_cur = AFE_DL11_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON1,
		.fs_shift = 25,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = AFE_DL11_CON0,
		.hd_shift = 6,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 27,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 27,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 27,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 27,
		.ch_config_reg = AFE_DL11_CON0,
		.ch_config_shift = 0,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 30,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "DL2",
		.id = MT8532_AFE_MEMIF_DL2,
		.reg_ofs_base = AFE_DL2_BASE,
		.reg_ofs_cur = AFE_DL2_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON0,
		.fs_shift = 10,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = AFE_DL2_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 18,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 18,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 18,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 18,
		.ch_config_reg = AFE_DL2_CON0,
		.ch_config_shift = 0,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 18,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "DL3",
		.id = MT8532_AFE_MEMIF_DL3,
		.reg_ofs_base = AFE_DL3_BASE,
		.reg_ofs_cur = AFE_DL3_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON0,
		.fs_shift = 15,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = AFE_DL3_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 19,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 19,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 19,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 19,
		.ch_config_reg = AFE_DL3_CON0,
		.ch_config_shift = 0,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 20,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "DL6",
		.id = MT8532_AFE_MEMIF_DL6,
		.reg_ofs_base = AFE_DL6_BASE,
		.reg_ofs_cur = AFE_DL6_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON1,
		.fs_shift = 0,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = AFE_DL6_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 22,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 22,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 22,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 22,
		.ch_config_reg = AFE_DL6_CON0,
		.ch_config_shift = 0,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 22,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "DL7",
		.id = MT8532_AFE_MEMIF_DL7,
		.reg_ofs_base = AFE_DL7_BASE,
		.reg_ofs_cur = AFE_DL7_CUR,
		.fs_reg = -1,
		.fs_shift = -1,
		.fs_maskbit = -1,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = -1,
		.hd_shift = -1,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 23,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 23,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 23,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 23,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
		.buffer_end_shift = 1,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 24,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "DL10",
		.id = MT8532_AFE_MEMIF_DL10,
		.reg_ofs_base = AFE_DL10_BASE,
		.reg_ofs_cur = AFE_DL10_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON1,
		.fs_shift = 20,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
		.hd_reg = AFE_DL10_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 26,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 26,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 26,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 26,
		.ch_config_reg = AFE_DL10_CON0,
		.ch_config_shift = 0,
		.int_odd_reg = -1,
		.int_odd_shift = -1,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 28,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "UL1",
		.id = MT8532_AFE_MEMIF_UL1,
		.reg_ofs_base = AFE_UL1_BASE,
		.reg_ofs_cur = AFE_UL1_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 0,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = -1,
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
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL1_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 0,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "UL2",
		.id = MT8532_AFE_MEMIF_UL2,
		.reg_ofs_base = AFE_UL2_BASE,
		.reg_ofs_cur = AFE_UL2_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 5,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL2_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL2_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 2,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 1,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 1,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 1,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL2_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 2,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "UL3",
		.id = MT8532_AFE_MEMIF_UL3,
		.reg_ofs_base = AFE_UL3_BASE,
		.reg_ofs_cur = AFE_UL3_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 10,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL3_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL3_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 3,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 2,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 2,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 2,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL3_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 4,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "UL4",
		.id = MT8532_AFE_MEMIF_UL4,
		.reg_ofs_base = AFE_UL4_BASE,
		.reg_ofs_cur = AFE_UL4_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 15,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL4_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL4_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 4,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 3,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 3,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 3,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL4_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 6,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "UL5",
		.id = MT8532_AFE_MEMIF_UL5,
		.reg_ofs_base = AFE_UL5_BASE,
		.reg_ofs_cur = AFE_UL5_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 20,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL5_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL5_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 5,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 4,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 4,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 4,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL5_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 8,
		.engen_domain_maskbit = 0x3,
	},  {
		.name = "UL6",
		.id = MT8532_AFE_MEMIF_UL6,
		.reg_ofs_base = AFE_UL6_BASE,
		.reg_ofs_cur = AFE_UL6_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 25,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL6_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL6_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 6,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 5,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 5,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 5,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL6_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 10,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "UL8",
		.id = MT8532_AFE_MEMIF_UL8,
		.reg_ofs_base = AFE_UL8_BASE,
		.reg_ofs_cur = AFE_UL8_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON3,
		.fs_shift = 5,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL8_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL8_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 8,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 7,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 7,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 7,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL8_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 12,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "UL9",
		.id = MT8532_AFE_MEMIF_UL9,
		.reg_ofs_base = AFE_UL9_BASE,
		.reg_ofs_cur = AFE_UL9_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON3,
		.fs_shift = 10,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL9_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL9_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 9,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 8,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 8,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 8,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL9_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 14,
		.engen_domain_maskbit = 0x3,
	}, {
		.name = "UL10",
		.id = MT8532_AFE_MEMIF_UL10,
		.reg_ofs_base = AFE_UL10_BASE,
		.reg_ofs_cur = AFE_UL10_CUR,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON3,
		.fs_shift = 15,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL10_CON0,
		.mono_shift = 1,
		.hd_reg = AFE_UL10_CON0,
		.hd_shift = 5,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 10,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 9,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 9,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 9,
		.ch_config_reg = -1,
		.ch_config_shift = -1,
		.int_odd_reg = AFE_UL10_CON0,
		.int_odd_shift = 0,
		.buffer_bytes_align = 64,
		.engen_domain_reg = A3_A4_TIMING_SEL1,
		.engen_domain_shift = 16,
		.engen_domain_maskbit = 0x3,
	},
};

static int spdif_out_irq_handler(int irq, void *dev_id)
{
	struct mtk_base_afe *afe = dev_id;
	struct mtk_base_afe_memif *memif = &afe->memif[MT8532_AFE_MEMIF_DL7];
	unsigned int val = 0;
	unsigned int ns_adr = 0;
	unsigned int burst_len = 0;
	unsigned int end_addr = 0;

	regmap_read(afe->regmap, AFE_IEC_BURST_INFO, &val);
	if ((val & AFE_IEC_BURST_INFO_READY_MASK) !=
	    AFE_IEC_BURST_INFO_READY) {
		dev_warn(afe->dev, "%s burst info is not ready\n", __func__);
		return 0;
	}

	regmap_read(afe->regmap, AFE_IEC_NSADR, &ns_adr);
	regmap_read(afe->regmap, AFE_IEC_BURST_LEN, &burst_len);
	regmap_read(afe->regmap, AFE_DL7_END, &end_addr);

	ns_adr += (burst_len & 0x0007ffff) >> 3;
	if (ns_adr >= end_addr) {
		regmap_read(afe->regmap, AFE_DL7_BASE, &val);
		ns_adr = val;
	}

	regmap_write(afe->regmap, AFE_IEC_NSADR, ns_adr);

	regmap_update_bits(afe->regmap, AFE_IEC_BURST_INFO,
			   AFE_IEC_BURST_INFO_READY_MASK,
			   AFE_IEC_BURST_INFO_NOT_READY);

	snd_pcm_period_elapsed(memif->substream);

	return 0;
}

static int spdif_in_irq_handler(int irq, void *dev_id)
{
	struct mtk_base_afe *afe = dev_id;
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mtk_base_afe_memif *memif = &afe->memif[MT8532_AFE_MEMIF_UL1];
	struct mt8532_multi_in_data *multi_in = &afe_priv->multi_in_data;

	multi_in->current_irq_count++;
	if (multi_in->current_irq_count >= multi_in->notify_irq_count) {
		check_multi_in_monitor_status(afe);
		snd_pcm_period_elapsed(memif->substream);
		multi_in->current_irq_count = 0;
	}

	return 0;
}

static unsigned int get_spdif_in_clear_bits(unsigned int v)
{
	unsigned int bits = 0;

	if (v & AFE_SPDIFIN_DEBUG3_PRE_ERR_NON_STS)
		bits |= AFE_SPDIFIN_EC_PRE_ERR_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG3_PRE_ERR_B_STS)
		bits |= AFE_SPDIFIN_EC_PRE_ERR_B_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG3_PRE_ERR_M_STS)
		bits |= AFE_SPDIFIN_EC_PRE_ERR_M_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG3_PRE_ERR_W_STS)
		bits |= AFE_SPDIFIN_EC_PRE_ERR_W_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG3_PRE_ERR_BITCNT_STS)
		bits |= AFE_SPDIFIN_EC_PRE_ERR_BITCNT_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG3_PRE_ERR_PARITY_STS)
		bits |= AFE_SPDIFIN_EC_PRE_ERR_PARITY_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG3_TIMEOUT_ERR_STS)
		bits |= AFE_SPDIFIN_EC_TIMEOUT_INT_CLEAR;
	if (v & AFE_SPDIFIN_INT_EXT2_LRCK_CHANGE)
		bits |= AFE_SPDIFIN_EC_DATA_LRCK_CHANGE_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG1_DATALAT_ERR)
		bits |= AFE_SPDIFIN_EC_DATA_LATCH_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG3_CHSTS_PREAMPHASIS_STS)
		bits |= AFE_SPDIFIN_EC_CHSTS_PREAMPHASIS_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG2_FIFO_ERR)
		bits |= AFE_SPDIFIN_EC_FIFO_ERR_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG2_CHSTS_INT_FLAG)
		bits |= AFE_SPDIFIN_EC_CHSTS_COLLECTION_CLEAR;
	if (v & AFE_SPDIFIN_DEBUG2_PERR_9TIMES_FLAG)
		bits |= AFE_SPDIFIN_EC_USECODE_COLLECTION_CLEAR;

	return bits;
}

static unsigned int get_spdif_in_sample_rate(struct mtk_base_afe *afe)
{
	unsigned int val = 0, fs = 0;

	regmap_read(afe->regmap, AFE_SPDIFIN_INT_EXT2, &val);

	val &= AFE_SPDIFIN_INT_EXT2_ROUGH_FS_MASK;

	switch (val) {
	case AFE_SPDIFIN_INT_EXT2_FS_32K:
		fs = 32000;
		break;
	case AFE_SPDIFIN_INT_EXT2_FS_44D1K:
		fs = 44100;
		break;
	case AFE_SPDIFIN_INT_EXT2_FS_48K:
		fs = 48000;
		break;
	case AFE_SPDIFIN_INT_EXT2_FS_88D2K:
		fs = 88200;
		break;
	case AFE_SPDIFIN_INT_EXT2_FS_96K:
		fs = 96000;
		break;
	case AFE_SPDIFIN_INT_EXT2_FS_176D4K:
		fs = 176400;
		break;
	case AFE_SPDIFIN_INT_EXT2_FS_192K:
		fs = 192000;
		break;
	default:
		fs = 0;
		break;
	}

	return fs;
}

static void spdif_in_reset_and_clear_error(struct mtk_base_afe *afe,
	unsigned int err_bits)
{
	regmap_update_bits(afe->regmap,
			   AFE_SPDIFIN_CFG0,
			   AFE_SPDIFIN_CFG0_INT_EN |
			   AFE_SPDIFIN_CFG0_EN,
			   0x0);

	regmap_write(afe->regmap, AFE_SPDIFIN_EC, err_bits);

	regmap_update_bits(afe->regmap,
			   AFE_SPDIFIN_CFG0,
			   AFE_SPDIFIN_CFG0_INT_EN |
			   AFE_SPDIFIN_CFG0_EN,
			   AFE_SPDIFIN_CFG0_INT_EN |
			   AFE_SPDIFIN_CFG0_EN);
}

static void spdif_in_info_update(struct mtk_base_afe *afe,
	unsigned int rate,
	unsigned int data_type_chg)
{
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_spdif_in_data *spdif_in = &afe_priv->spdif_in_data;
	unsigned int stream_type = 0;
	bool info_change = false;
	unsigned int user_code[SPDIF_USERCODE_NUM] = {0};
	unsigned int ch_status[SPDIF_CHSTS_NUM] = {0};
	int i = 0;

	do {
		//update rate, user code, channel status, stream type.
		if (spdif_in->subdata.rate != rate) {
			spdif_in->subdata.rate = rate;
			info_change = true;
#if ARCIN_FEATURE_SUPPORT
			if (rate == 0) {
				dev_info(afe->dev, "%s ARC in unlock.\n",
					__func__);
				scnprintf(env_ext[NOTIFY_UEVENT_KEY_LOCK], 64,
					key_str[NOTIFY_UEVENT_KEY_LOCK], 0);
			} else {
				dev_info(afe->dev, "%s ARC in lock\n",
					__func__);
				scnprintf(env_ext[NOTIFY_UEVENT_KEY_LOCK], 64,
					key_str[NOTIFY_UEVENT_KEY_LOCK], 1);
			}
			schedule_work(&uevent_work);
#endif
			if (rate == 0) {
				//sample rate
				spdif_in->subdata.rate = 0;

				//usercode
				memset((void *)spdif_in->subdata.user_code,
				0xff, sizeof(spdif_in->subdata.user_code));

				//channel status
				memset((void *)spdif_in->subdata.ch_status,
				0xff, sizeof(spdif_in->subdata.ch_status));

				//stream type
				spdif_in->subdata.stream_type = 0xff;
				break;
			}
		}

		//usercode
		for (i = 0; i < SPDIF_USERCODE_NUM; i++) {
			regmap_read(afe->regmap,
				SPDIFIN_USERCODE1 + i * 4,
				&user_code[i]);
			if (spdif_in->subdata.user_code[i] != user_code[i]) {
				spdif_in->subdata.user_code[i] = user_code[i];
				info_change = true;
			}
		}

		//channel status
		for (i = 0; i < SPDIF_CHSTS_NUM; i++) {
			regmap_read(afe->regmap,
				AFE_SPDIFIN_CHSTS1 + i * 4,
				&ch_status[i]);
			if (spdif_in->subdata.ch_status[i] != ch_status[i])
				spdif_in->subdata.ch_status[i] = ch_status[i];
		}

		//channel status -data type change
		if (data_type_chg)
			info_change = true;

		//stream type
		regmap_read(afe->regmap, AFE_MPHONE_MULTI_MON, &stream_type);
		if (stream_type & AFE_MPHONE_MULTI_MON_DEC) {
			stream_type = stream_type &
				AFE_MPHONE_MULTI_MON_ROUGH;

			if (spdif_in->subdata.stream_type != stream_type) {
				spdif_in->subdata.stream_type = stream_type;
				info_change = true;
			}
		}
	} while (0);

	dev_dbg(afe->dev, "%s info_change:%d data_type_chg:%u rate:%u\n",
		__func__, info_change, data_type_chg, rate);

	if (info_change) {
		mt8532_snd_ctl_notify(afe_priv->card, "Spdif_In_Info",
				SNDRV_CTL_EVENT_MASK_VALUE);
	}
}

static int spdif_in_detect_irq_handler(int irq, void *dev_id)
{
	struct mtk_base_afe *afe = dev_id;
	struct mt8532_afe_private *priv = afe->platform_priv;
	unsigned int debug1 = 0, debug2 = 0, debug3 = 0, int_ext2 = 0, data_type_chg = 0;
	unsigned int err = 0;
	unsigned int rate = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&priv->spdifin_ctrl_lock, flags);

	regmap_read(afe->regmap, AFE_SPDIFIN_INT_EXT2, &int_ext2);
	regmap_read(afe->regmap, AFE_SPDIFIN_DEBUG1, &debug1);
	regmap_read(afe->regmap, AFE_SPDIFIN_DEBUG2, &debug2);
	regmap_read(afe->regmap, AFE_SPDIFIN_DEBUG3, &debug3);

	err = (int_ext2 & AFE_SPDIFIN_INT_EXT2_LRCK_CHANGE) |
	      (debug1 & AFE_SPDIFIN_DEBUG1_DATALAT_ERR) |
	      (debug2 & AFE_SPDIFIN_DEBUG2_FIFO_ERR) |
	      (debug3 & AFE_SPDIFIN_DEBUG3_ALL_ERR);

	data_type_chg = (debug3 &
		AFE_SPDIFIN_DEBUG3_CHSTS_PREAMPHASIS_STS) >> 7;

	dev_dbg(afe->dev,
		"%s int_ext2(0x%x) debug(0x%x,0x%x,0x%x) err(0x%x)\n",
		__func__, int_ext2, debug1, debug2, debug3, err);

	if (err != 0) {
		spdif_in_reset_and_clear_error(afe,
			get_spdif_in_clear_bits(err));

		rate = 0;
		spdif_in_info_update(afe, rate, data_type_chg);

		goto spdif_in_detect_irq_end;
	}

	rate = get_spdif_in_sample_rate(afe);
	if (rate == 0) {
		spdif_in_reset_and_clear_error(afe, AFE_SPDIFIN_EC_CLEAR_ALL);
		spdif_in_info_update(afe, rate, data_type_chg);

		goto spdif_in_detect_irq_end;
	}

	spdif_in_info_update(afe, rate, data_type_chg);

	/* clear all interrupt bits */
	regmap_write(afe->regmap,
		     AFE_SPDIFIN_EC,
		     AFE_SPDIFIN_EC_CLEAR_ALL);

spdif_in_detect_irq_end:
	spin_unlock_irqrestore(&priv->spdifin_ctrl_lock, flags);

	return 0;
}

static const struct mtk_base_irq_data irq_data[MT8532_AFE_IRQ_NUM] = {
	{
		.id = MT8532_AFE_IRQ1,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ1_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 0,
		.irq_status_shift = 16,
		.custom_handler = spdif_out_irq_handler,
	}, {
		.id = MT8532_AFE_IRQ2,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ2_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 17,
		.custom_handler = spdif_in_detect_irq_handler,
	}, {
		.id = MT8532_AFE_IRQ3,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ3_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 2,
		.irq_status_shift = 18,
		.custom_handler = spdif_in_irq_handler,
	}, {
		.id = MT8532_AFE_IRQ4,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ4_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 17,
	}, {
		.id = MT8532_AFE_IRQ5,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ5_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 17,
	}, {
		.id = MT8532_AFE_IRQ6,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ6_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 17,
	}, {
		.id = MT8532_AFE_IRQ7,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ7_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 17,
	}, {
		.id = MT8532_AFE_IRQ8,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ8_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 17,
	}, {
		.id = MT8532_AFE_IRQ9,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ9_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 17,
		.custom_handler = spdif_in_detect_irq_handler,
	}, {
		.id = MT8532_AFE_IRQ10,
		.irq_cnt_reg = -1,
		.irq_cnt_shift = -1,
		.irq_cnt_maskbit = -1,
		.irq_en_reg = AFE_IRQ10_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = -1,
		.irq_fs_shift = -1,
		.irq_fs_maskbit = -1,
		.irq_clr_reg = AFE_IRQ_MCU_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 17,
	}, {
		.id = MT8532_AFE_IRQ11,
		.irq_cnt_reg = ASYS_IRQ1_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ1_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ1_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 0,
		.irq_status_shift = 0,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 0,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ12,
		.irq_cnt_reg = ASYS_IRQ2_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ2_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ2_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 1,
		.irq_status_shift = 1,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 2,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ13,
		.irq_cnt_reg = ASYS_IRQ3_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ3_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ3_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 2,
		.irq_status_shift = 2,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 4,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ14,
		.irq_cnt_reg = ASYS_IRQ4_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ4_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ4_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 3,
		.irq_status_shift = 3,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 6,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ15,
		.irq_cnt_reg = ASYS_IRQ5_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ5_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ5_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 4,
		.irq_status_shift = 4,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 8,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ16,
		.irq_cnt_reg = ASYS_IRQ6_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ6_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ6_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 5,
		.irq_status_shift = 5,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 10,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ17,
		.irq_cnt_reg = ASYS_IRQ7_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ7_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ7_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 6,
		.irq_status_shift = 6,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 12,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ18,
		.irq_cnt_reg = ASYS_IRQ8_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ8_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ8_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 7,
		.irq_status_shift = 7,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 14,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ19,
		.irq_cnt_reg = ASYS_IRQ9_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ9_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ9_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 8,
		.irq_status_shift = 8,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 16,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ20,
		.irq_cnt_reg = ASYS_IRQ10_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ10_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ10_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 9,
		.irq_status_shift = 9,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 18,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ21,
		.irq_cnt_reg = ASYS_IRQ11_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ11_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ11_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 10,
		.irq_status_shift = 10,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 20,
		.engen_domain_maskbit = 0x3,
	}, {
		.id = MT8532_AFE_IRQ22,
		.irq_cnt_reg = ASYS_IRQ12_CON,
		.irq_cnt_shift = 0,
		.irq_cnt_maskbit = 0xffffff,
		.irq_en_reg = ASYS_IRQ12_CON,
		.irq_en_shift = 31,
		.irq_fs_reg = ASYS_IRQ12_CON,
		.irq_fs_shift = 24,
		.irq_fs_maskbit = 0x3f,
		.irq_clr_reg = ASYS_IRQ_CLR,
		.irq_clr_shift = 11,
		.irq_status_shift = 11,
		.engen_domain_reg = A3_A4_TIMING_SEL6,
		.engen_domain_shift = 22,
		.engen_domain_maskbit = 0x3,
	},
};

static const int memif_specified_irqs[MT8532_AFE_MEMIF_NUM] = {
	[MT8532_AFE_MEMIF_DLM] = MT8532_AFE_IRQ11,
	[MT8532_AFE_MEMIF_DL11] = MT8532_AFE_IRQ12,
	[MT8532_AFE_MEMIF_DL2] = MT8532_AFE_IRQ13,
	[MT8532_AFE_MEMIF_DL3] = MT8532_AFE_IRQ14,
	[MT8532_AFE_MEMIF_DL6] = MT8532_AFE_IRQ15,
	[MT8532_AFE_MEMIF_DL7] = MT8532_AFE_IRQ1,  //spdif out
	[MT8532_AFE_MEMIF_DL10] = MT8532_AFE_IRQ16,
	[MT8532_AFE_MEMIF_UL1] = MT8532_AFE_IRQ3,  //mphone_mulit1
	[MT8532_AFE_MEMIF_UL2] = MT8532_AFE_IRQ17,
	[MT8532_AFE_MEMIF_UL3] = MT8532_AFE_IRQ18,
	[MT8532_AFE_MEMIF_UL4] = MT8532_AFE_IRQ19,
	[MT8532_AFE_MEMIF_UL5] = MT8532_AFE_IRQ20,
	[MT8532_AFE_MEMIF_UL6] = MT8532_AFE_IRQ9, //mphone_mulit2
	[MT8532_AFE_MEMIF_UL8] = MT8532_AFE_IRQ21,
	[MT8532_AFE_MEMIF_UL9] = MT8532_AFE_IRQ20,
	[MT8532_AFE_MEMIF_UL10] = MT8532_AFE_IRQ22,
};

static const int aux_irqs[] = {
	MT8532_AFE_IRQ2,
};

#ifdef DEBUG_AFE_REGISTER_RW
int mt8532_reg_read(void *context, unsigned int reg, unsigned int *val)
{
	struct mtk_base_afe *afe = context;

	mt8532_afe_enable_reg_rw_clk(afe);

	dev_notice(afe->dev, "%s reg 0x%x >>\n", __func__, reg);

	*val = readl(afe->base_addr + reg);

	dev_notice(afe->dev, "%s reg 0x%x val 0x%x <<\n", __func__, reg, *val);

	mt8532_afe_disable_reg_rw_clk(afe);

	return 0;
}

int mt8532_reg_write(void *context, unsigned int reg, unsigned int val)
{
	struct mtk_base_afe *afe = context;

	mt8532_afe_enable_reg_rw_clk(afe);

	dev_notice(afe->dev, "%s reg 0x%x val 0x%x >>\n", __func__, reg, val);

	writel(val, afe->base_addr + reg);

	dev_notice(afe->dev, "%s reg 0x%x val 0x%x <<\n", __func__, reg, val);

	mt8532_afe_disable_reg_rw_clk(afe);

	return 0;
}

static const struct regmap_bus mt8532_afe_regmap_bus = {
	.fast_io = true,
	.reg_write = mt8532_reg_write,
	.reg_read = mt8532_reg_read,
	.val_format_endian_default = REGMAP_ENDIAN_LITTLE,
};
#endif

static const struct regmap_config mt8532_afe_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = MAX_REGISTER,
	.cache_type = REGCACHE_NONE,
};

static irqreturn_t mt8532_afe_irq_handler(int irq, void *dev_id)
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
		dev_dbg(afe->dev, "%s irq status err\n", __func__);
		afe_irq_clr_bits = AFE_IRQ_MCU_CLR_BITS;
		asys_irq_clr_bits = ASYS_IRQ_CLR_BITS;
		goto err_irq;
	}

	for (i = 0; i < MT8532_AFE_MEMIF_NUM; i++) {
		struct mtk_base_afe_memif *memif = &afe->memif[i];
		struct mtk_base_irq_data const *irq_data;

		if (memif->irq_usage < 0)
			continue;

		irq_data = afe->irqs[memif->irq_usage].irq_data;

		irq_status_bits = BIT(irq_data->irq_status_shift);
		irq_clr_bits = BIT(irq_data->irq_clr_shift);

		if (!(val & irq_status_bits))
			continue;

		//dev_dbg(afe->dev, "%s ii=%d\n", __func__,i);

		if (irq_data->irq_clr_reg == ASYS_IRQ_CLR)
			asys_irq_clr_bits |= irq_clr_bits;
		else
			afe_irq_clr_bits |= irq_clr_bits;

		if (irq_data->custom_handler)
			irq_data->custom_handler(irq, dev_id);
		else
			snd_pcm_period_elapsed(memif->substream);
	}

	for (i = 0; i < ARRAY_SIZE(aux_irqs); i++) {
		struct mtk_base_irq_data const *irq_data;
		unsigned int irq_id = aux_irqs[i];

		irq_data = afe->irqs[irq_id].irq_data;

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
	}

err_irq:
	/* clear irq */
	if (asys_irq_clr_bits)
		regmap_write(afe->regmap, ASYS_IRQ_CLR, asys_irq_clr_bits);
	if (afe_irq_clr_bits)
		regmap_write(afe->regmap, AFE_IRQ_MCU_CLR, afe_irq_clr_bits);

	return IRQ_HANDLED;
}

static int mt8532_afe_runtime_suspend(struct device *dev)
{
	return 0;
}

static int mt8532_afe_runtime_resume(struct device *dev)
{
	return 0;
}

static int mt8532_afe_dev_runtime_suspend(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	dev_dbg(afe->dev, "%s >>\n", __func__);

	mt8532_afe_suspend(afe->dev, afe);

	dev_dbg(afe->dev, "%s <<\n", __func__);

	return 0;
}

static int mt8532_afe_dev_runtime_resume(struct device *dev)
{
	struct mtk_base_afe *afe = dev_get_drvdata(dev);

	dev_dbg(afe->dev, "%s >>\n", __func__);

	mt8532_afe_resume(afe->dev, afe);

	dev_dbg(afe->dev, "%s <<\n", __func__);

	return 0;
}

static int mt8532_afe_init_registers(struct mtk_base_afe *afe)
{
	size_t i;
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	struct mt8532_fe_dai_data *fe_data;

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

	static struct {
		unsigned int id;
		unsigned int pbuf_reg;
	} dl_memif_regs[] = {
		{MT8532_AFE_MEMIF_DLM, AFE_DL8_CON0},
		{MT8532_AFE_MEMIF_DL11, AFE_DL11_CON0},
		{MT8532_AFE_MEMIF_DL2, AFE_DL2_CON0},
		{MT8532_AFE_MEMIF_DL3, AFE_DL3_CON0},
		{MT8532_AFE_MEMIF_DL6, AFE_DL6_CON0},
		{MT8532_AFE_MEMIF_DL7, AFE_DL7_CON0},
		{MT8532_AFE_MEMIF_DL10, AFE_DL10_CON0},
	};

	mt8532_afe_enable_main_clk(afe);

	for (i = 0; i < ARRAY_SIZE(init_regs); i++)
		regmap_update_bits(afe->regmap, init_regs[i].reg,
				   init_regs[i].mask, init_regs[i].val);

	for (i = 0; i < ARRAY_SIZE(dl_memif_regs); i++) {
		fe_data = &afe_priv->fe_data[dl_memif_regs[i].id];

		regmap_update_bits(afe->regmap,
			dl_memif_regs[i].pbuf_reg,
			GENMASK(17, 16),
			(0x3 - fe_data->pbuf_size_conf) << 16);
	}

	if (afe_priv->use_bypass_afe_pinmux)
		regmap_update_bits(afe->regmap, ETDM_IN2_CON0,
				   ETDM_CON0_SLAVE_MODE,
				   ETDM_CON0_SLAVE_MODE);

	mt8532_afe_disable_main_clk(afe);

	return 0;
}

static bool mt8532_afe_validate_sram(struct mtk_base_afe *afe,
	u32 pa, u32 size)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	u32 pa_base = pa;
	u32 pa_end = pa + size;

	if (priv->afe_sram_pa &&
	    (pa_base >= priv->afe_sram_pa) &&
	    (pa_end <= (priv->afe_sram_pa + priv->afe_sram_size))) {
		return true;
	}

	return false;
}

static void __iomem *mt8532_afe_sram_pa_to_va(struct mtk_base_afe *afe,
	u32 pa)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	u32 off = pa - priv->afe_sram_pa;

	if (priv->afe_sram_pa &&
	    (off >= 0) && (off < priv->afe_sram_size)) {
		return ((unsigned char *)priv->afe_sram_va + off);
	}

	return NULL;
}

static const struct {
	unsigned int rate;
	unsigned int cycles;
} rate_cali_cycles[] = {
	{.rate = 8000, .cycles = 64},
	{.rate = 12000, .cycles = 64},
	{.rate = 16000, .cycles = 64},
	{.rate = 24000, .cycles = 64},
	{.rate = 32000, .cycles = 64},
	{.rate = 48000, .cycles = 64},
	{.rate = 96000, .cycles = 64},
	{.rate = 192000, .cycles = 64},
	{.rate = 384000, .cycles = 64},
	{.rate = 7350, .cycles = 64},
	{.rate = 11025, .cycles = 64},
	{.rate = 14700, .cycles = 64},
	{.rate = 22050, .cycles = 64},
	{.rate = 29400, .cycles = 64},
	{.rate = 44100, .cycles = 64},
	{.rate = 88200, .cycles = 64},
	{.rate = 176400, .cycles = 64},
	{.rate = 352800, .cycles = 64},
};

static unsigned int get_cali_cycles_by_rate(unsigned int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rate_cali_cycles); i++)
		if (rate_cali_cycles[i].rate == rate)
			return rate_cali_cycles[i].cycles;

	return 0;
}

static void clk_tune_adjust(struct work_struct *work)
{
	struct mt8532_afe_ext_clk_tune_data *tune = container_of(work,
		struct mt8532_afe_ext_clk_tune_data, clk_tune_work.work);
	struct mtk_base_afe *afe = tune->afe;
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct ext_clk_tune_property *prop;
	int phase;
	u64 res_delay_ns = 0;
	unsigned int asrc_id = 0;

	prop = tune->working;
	if (!prop) {
		dev_info(afe->dev, "%s invalid working\n", __func__);
		return;
	}

	do {
		phase = tune->working_phase;

		if (phase == TUNE_PHASE_INIT) {
			unsigned int in_rate = 0;

			in_rate = tune->input_rate;
			if (!in_rate) {
				phase = TUNE_PHASE_ABORT;
				goto clk_tune_hrt_abort;
			}

			tune->cali_cycles = get_cali_cycles_by_rate(in_rate);
			tune->asrc_id = asrc_id;
			tune->cali_retry_cnt = 0;
			phase = TUNE_PHASE_START_CALI_PROC;
		}

		if (phase == TUNE_PHASE_START_CALI_PROC) {
#ifdef CONFIG_MTK_AUDIO_ASRC
			int ret;
			enum afe_mem_asrc_tracking_mode tracking_mode;
			enum afe_mem_asrc_tracking_source tracking_src;
			enum afe_mem_asrc_cali_clk cali_clk;
			u32 cali_cycle = tune->cali_cycles;
			u32 freq = tune->input_rate;

			tracking_mode = MEM_ASRC_TRACKING_RX;
			tracking_src = (prop->tune_id == CLK_TUNE_SPDIF_IN)
					? CALI_SIG_SEL_MPHONE
					: CALI_SIG_SEL_SPLIN_SLAVE;
			cali_clk = MEM_ASRC_CALI_CLK_A1SYS_FIX;
			ret = mtk_asrc_cali_open(asrc_id);
			if (ret < 0) {
				dev_info(afe->dev, "%s() cali open fail!\n",
					__func__);
				phase = TUNE_PHASE_ABORT;
				goto clk_tune_hrt_abort;
			}
			ret = mtk_asrc_cali_start(asrc_id,
					tracking_mode,
					tracking_src,
					cali_clk,
					cali_cycle,
					freq,
					freq);
			if (ret < 0) {
				dev_info(afe->dev, "%s() cali start fail!\n",
					__func__);
				phase = TUNE_PHASE_ABORT;
				goto clk_tune_hrt_abort;
			}
			tune->start = true;

			res_delay_ns = (u64)cali_cycle * NSEC_PER_SEC * 2;
			do_div(res_delay_ns, tune->input_rate);
			dev_dbg(afe->dev, "%s() phase:%d, delay_ns: %llu\n",
				__func__, phase, res_delay_ns);
#endif
			phase = TUNE_PHASE_GET_CALI_RESULT;
			goto clk_tune_hrt_restart;
		}

		if (phase == TUNE_PHASE_GET_CALI_RESULT) {
#ifdef CONFIG_MTK_AUDIO_ASRC
			u64 valid_range =  (u64)tune->input_rate * 100;
			u64 up_limit_rate = (u64)tune->input_rate * 100000;
			u64 low_limit_rate = (u64)tune->input_rate * 100000;

			up_limit_rate += valid_range;
			low_limit_rate -= valid_range;

			tune->cali_input_rate_10uhz =
				mtk_asrc_cali_get_result(asrc_id);
			if (tune->cali_input_rate_10uhz > up_limit_rate ||
			    tune->cali_input_rate_10uhz < low_limit_rate) {
				phase = TUNE_PHASE_GET_CALI_RESULT;
				res_delay_ns = tune->cali_retry_time;
				tune->cali_retry_cnt++;
				dev_dbg(afe->dev,
					"%s cali rate: %llu, delay_ns: %llu\n",
					__func__, tune->cali_input_rate_10uhz,
					res_delay_ns);
				goto clk_tune_hrt_restart;
			}

			dev_dbg(afe->dev,
				"%s cali rate:%llu [%llu:%llu]\n",
				__func__, tune->cali_input_rate_10uhz,
				low_limit_rate, up_limit_rate);
#endif
			phase = TUNE_PHASE_CHECK_OUTPUT_ACTIVE;
		}

		if (phase == TUNE_PHASE_CHECK_OUTPUT_ACTIVE) {
			unsigned int out_rate = 0;
			unsigned int in_rate = 0;
			u64 cali_rate_10uhz = 0;
			struct clk *clk;
			u64 tmp, retry_time;

			retry_time = tune->cali_retry_time *
				(u64)tune->cali_retry_cnt;

			if (retry_time > CLK_TUNE_CALI_WARNING_TIME_OUT_NS)
				dev_info(afe->dev,
				"%s cali rate:%llu retry time:%llu cnt:%d\n",
				__func__, tune->cali_input_rate_10uhz,
				retry_time, tune->cali_retry_cnt);

			tune->cali_retry_cnt = 0;

			if (!(priv->be_active_status & tune->outputs)) {
				res_delay_ns = WAIT_OUTPUT_ACTIVE_DELAY_NS;
				goto clk_tune_hrt_restart;
			}

			out_rate = tune->output_rate;
			if (!out_rate) {
				res_delay_ns = WAIT_OUTPUT_ACTIVE_DELAY_NS;
				goto clk_tune_hrt_restart;
			}

			in_rate = tune->input_rate;
			cali_rate_10uhz = tune->cali_input_rate_10uhz;
			if (!in_rate || !cali_rate_10uhz) {
				phase = TUNE_PHASE_ABORT;
				goto clk_tune_hrt_abort;
			}

			if ((in_rate % 8000) && (out_rate % 8000)) {
				tmp = (u64)MT8532_APLL1_RATE * cali_rate_10uhz;
				do_div(tmp, in_rate);
			} else if (!(in_rate % 8000) && !(out_rate % 8000)) {
				tmp = (u64)MT8532_APLL2_RATE * cali_rate_10uhz;
				do_div(tmp, in_rate);
			} else if (!(in_rate % 8000) && (out_rate % 8000)) {
				tmp = (u64)MT8532_APLL2_RATE * cali_rate_10uhz;
				do_div(tmp, in_rate);
				/* APLL2 -> APLL1 */
				do_div(tmp, 480);
				tmp *= 441;
			} else {
				tmp = (u64)MT8532_APLL1_RATE * cali_rate_10uhz;
				do_div(tmp, in_rate);
				/* APLL1 -> APLL2 */
				do_div(tmp, 441);
				tmp *= 480;
			}

			do_div(tmp, 100000);
			tune->apll_target_rate = (unsigned long)tmp;
			dev_dbg(afe->dev, "%s() tmp:%llu, target:%lu\n",
				__func__, tmp, tune->apll_target_rate);

			if (out_rate % 8000) {

				clk = priv->clocks[MT8532_CLK_APMIXED_APLL1];
				tune->apll_restore_rate = tune->apll1_rate;

				tmp = (u64)MT8532_APLL1_RATE *
					prop->adj_step_ppm;
				do_div(tmp, 1000000);
				tune->apll_step_hz = (unsigned long)tmp;
			} else {
				clk = priv->clocks[MT8532_CLK_APMIXED_APLL2];
				tune->apll_restore_rate = tune->apll2_rate;

				tmp = (u64)MT8532_APLL2_RATE *
					prop->adj_step_ppm;
				do_div(tmp, 1000000);
				tune->apll_step_hz = (unsigned long)tmp;
			}
			tune->apll_clk = clk;
			tune->apll_current_rate = clk_get_rate(clk);

			if (tune->apll_target_rate == tune->apll_current_rate)
				phase = TUNE_PHASE_ADJUST_DONE;
			else
				phase = TUNE_PHASE_ADJUST_APLL_RATE;
		}

		if (phase == TUNE_PHASE_ADJUST_APLL_RATE) {
			tune->apll_current_rate =
				mt8532_adjust_clk_rate_by_step(afe,
						tune->apll_clk,
						tune->apll_current_rate,
						tune->apll_target_rate,
						tune->apll_step_hz);
			if (tune->apll_current_rate !=
			    tune->apll_target_rate) {
				res_delay_ns = (u64)prop->adj_step_us *
					NSEC_PER_USEC;
				goto clk_tune_hrt_restart;
			}

			dev_dbg(afe->dev,
				"%s adjust clk %s to %lu done\n",
				__func__,
				__clk_get_name(tune->apll_clk),
				tune->apll_target_rate);

			phase = TUNE_PHASE_ADJUST_DONE;
		}

		if (phase == TUNE_PHASE_ADJUST_DONE) {
			if (prop->period_ms > 0) {
				res_delay_ns = (u64)prop->period_ms *
					NSEC_PER_MSEC;
				phase = TUNE_PHASE_GET_CALI_RESULT;
				goto clk_tune_hrt_restart;
			} else {
				phase = TUNE_PHASE_ALL_DONE;
			}
		}

		if (phase == TUNE_PHASE_ALL_DONE)
			break;
	} while (1);

	dev_dbg(afe->dev, "%s done phase %d\n", __func__, phase);

clk_tune_hrt_abort:
#ifdef CONFIG_MTK_AUDIO_ASRC
	mt8532_afe_asrc_cali_reset(tune);
#endif
	return;

clk_tune_hrt_restart:
	tune->working_phase = phase;
	queue_delayed_work(system_power_efficient_wq,
			   &tune->clk_tune_work,
			   nsecs_to_jiffies(res_delay_ns));
}

static void mt8532_afe_init_private(struct mt8532_afe_private *priv)
{
	size_t i;

	spin_lock_init(&priv->afe_ctrl_lock);
	spin_lock_init(&priv->spdifin_ctrl_lock);

	mutex_init(&priv->block_dpidle_mutex);

	priv->dl8_max_main_channels = 16;

	for (i = 0; i < MT8532_AFE_TDMOUT_CONN_CFG_NUM; i++)
		priv->tdmo_conn.out_cfg[i] = i;

	priv->multi_in_mon_info.rough_type = MT8532_MULTI_IN_UNKNOWN;
}

static int mt8532_afe_parse_of(struct mtk_base_afe *afe,
	struct platform_device *pdev)
{
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct device_node *np = pdev->dev.of_node;
	size_t i, j;
	int ret;
	struct resource *res;
	unsigned int irq_id;
	unsigned int stream;
	unsigned int temps[4];
	char prop[128];
	struct mt8532_etdm_data *etdm_data;
	struct {
		char *name;
		unsigned int val;
	} of_fe_table[] = {
		{ "dlm",	MT8532_AFE_MEMIF_DLM },
		{ "dl2",	MT8532_AFE_MEMIF_DL2 },
		{ "dl3",	MT8532_AFE_MEMIF_DL3 },
		{ "dl6",	MT8532_AFE_MEMIF_DL6 },
		{ "dl7",	MT8532_AFE_MEMIF_DL7 },
		{ "dl10",	MT8532_AFE_MEMIF_DL10 },
		{ "ul1",	MT8532_AFE_MEMIF_UL1 },
		{ "ul2",	MT8532_AFE_MEMIF_UL2 },
		{ "ul3",	MT8532_AFE_MEMIF_UL3 },
		{ "ul4",	MT8532_AFE_MEMIF_UL4 },
		{ "ul5",	MT8532_AFE_MEMIF_UL5 },
		{ "ul8",	MT8532_AFE_MEMIF_UL8 },
		{ "ul9",	MT8532_AFE_MEMIF_UL9 },
		{ "ul10",	MT8532_AFE_MEMIF_UL10 },
	};
	struct {
		char *name;
		unsigned int set;
		unsigned int stream;
	} of_afe_etdms[] = {
		{"etdm1-out", MT8532_ETDM1, SNDRV_PCM_STREAM_PLAYBACK},
		{"etdm1-in", MT8532_ETDM1, SNDRV_PCM_STREAM_CAPTURE},
		{"etdm2-out", MT8532_ETDM2, SNDRV_PCM_STREAM_PLAYBACK},
		{"etdm2-in", MT8532_ETDM2, SNDRV_PCM_STREAM_CAPTURE},
	};
	struct {
		char *name;
		unsigned int val;
	} of_tune_slave_inputs[] = {
		{"spdif-in", CLK_TUNE_SPDIF_IN},
		{"multi-in", CLK_TUNE_MULTI_IN},
	};
	struct {
		char *name;
		unsigned int val;
	} of_tune_outputs[] = {
		{"etdm1-out", MT8532_AFE_IO_ETDM1_OUT},
		{"etdm2-out", MT8532_AFE_IO_ETDM2_OUT},
	};

	irq_id = platform_get_irq(pdev, 0);
	dev_dbg(afe->dev, "irq_id=%d\n", irq_id);
	if (!irq_id) {
		dev_err(afe->dev, "np %s no irq\n", np->name);
		return -ENXIO;
	}

	ret = devm_request_irq(afe->dev, irq_id, mt8532_afe_irq_handler,
			       0, "Afe1_ISR_Handle", (void *)afe);
	if (ret) {
		dev_err(afe->dev, "could not request_irq\n");
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev_dbg(afe->dev, "reg start=0x%x,end=0x%x\n", res->start, res->end);
	afe->base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(afe->base_addr))
		return PTR_ERR(afe->base_addr);

	dev_dbg(afe->dev, "base_addr_va=%p\n", afe->base_addr);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	dev_dbg(afe->dev, "sram start=0x%x,end=0x%x\n", res->start, res->end);
	if (res) {
		priv->afe_sram_va =
			devm_ioremap_resource(&pdev->dev, res);
		dev_dbg(afe->dev, "afe_sram_va=%p\n", priv->afe_sram_va);
		if (!IS_ERR(priv->afe_sram_va)) {
			priv->afe_sram_pa = res->start;
			priv->afe_sram_size = resource_size(res);
			dev_dbg(afe->dev, "afe_sram_pa=0x%x sz=0x%x\n",
				priv->afe_sram_pa, priv->afe_sram_size);
		}
	}

	priv->topckgen = syscon_regmap_lookup_by_phandle(np,
		"mediatek,topckgen-regmap");

	priv->scpsys = syscon_regmap_lookup_by_phandle(np,
		"mediatek,scpsys-regmap");

	ret = of_property_read_u32_array(np, "mediatek,etdm-clock-modes",
					 &temps[0],
					 MT8532_ETDM_SETS);
	if (ret == 0) {
		for (i = 0; i < MT8532_ETDM_SETS; i++)
			priv->etdm_data[i].clock_mode = temps[i];

		stream = SNDRV_PCM_STREAM_PLAYBACK;

		etdm_data = &priv->etdm_data[MT8532_ETDM1];
		if (etdm_data->clock_mode == MT8532_ETDM_SHARED_CLOCK)
			etdm_data->sync_source[stream] =
				MT8532_ETDM_SYNC_FROM_IN1;

		etdm_data = &priv->etdm_data[MT8532_ETDM2];
		if (etdm_data->clock_mode == MT8532_ETDM_SHARED_CLOCK)
			etdm_data->sync_source[stream] =
				MT8532_ETDM_SYNC_FROM_IN2;
	}

	ret = of_property_read_u32_array(np, "mediatek,etdm-out-data-modes",
					 &temps[0],
					 MT8532_ETDM_SETS);
	dev_dbg(afe->dev, "%s etdm-out-data-modes ret=%d\n", __func__, ret);
	if (ret == 0) {
		stream = SNDRV_PCM_STREAM_PLAYBACK;
		for (i = 0; i < MT8532_ETDM_SETS; i++) {
			dev_dbg(afe->dev, "%s i=%d,stream=%d,tdmoutdata_mod=%d\n",
				__func__, i, stream, temps[i]);
			priv->etdm_data[i].data_mode[stream] = temps[i];
		}
	}

	ret = of_property_read_u32_array(np, "mediatek,etdm-in-data-modes",
					 &temps[0],
					 MT8532_ETDM_SETS);
	if (ret == 0) {
		stream = SNDRV_PCM_STREAM_CAPTURE;
		for (i = 0; i < MT8532_ETDM_SETS; i++) {
			dev_dbg(afe->dev, "%s i=%d,stream=%d,tdmindata_mod=%d\n",
				__func__, i, stream, temps[i]);
			priv->etdm_data[i].data_mode[stream] = temps[i];
		}
	}

	for (i = 0; i < ARRAY_SIZE(of_afe_etdms); i++) {
		unsigned int stream = of_afe_etdms[i].stream;
		const char *str;
		bool force_on = false;
		bool force_on_1st_trigger = false;

		etdm_data = &priv->etdm_data[of_afe_etdms[i].set];

		if (stream == SNDRV_PCM_STREAM_CAPTURE) {
			for (j = 0; j < MT8532_ETDM_MAX_CHANNELS; j++) {
				snprintf(prop, sizeof(prop),
					 "mediatek,%s-ch%zu-disabled",
					 of_afe_etdms[i].name, j);

				etdm_data->in_disable_ch[j] =
					of_property_read_bool(np, prop);
			}
		}

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-enable-sequence",
			 of_afe_etdms[i].name);
		ret = of_property_read_u32(np, prop, &temps[0]);
		dev_dbg(afe->dev, "%s %s ret=%d\n", __func__, prop, ret);
		if (ret == 0)
			etdm_data->enable_seq[stream] = temps[0];

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-sync-source",
			 of_afe_etdms[i].name);
		ret = of_property_read_u32(np, prop, &temps[0]);
		dev_dbg(afe->dev, "%s %s ret=%d\n", __func__, prop, ret);
		if (ret == 0)
			etdm_data->sync_source[stream] = temps[0];

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-force-on",
			 of_afe_etdms[i].name);
		force_on = of_property_read_bool(np, prop);

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-force-on-1st-trigger",
			 of_afe_etdms[i].name);
		force_on_1st_trigger = of_property_read_bool(np, prop);
		dev_dbg(afe->dev, "%s force_on=%d\n", __func__, force_on);

		if (force_on) {
			etdm_data->force_on[stream] = true;
			etdm_data->force_on_policy[stream] =
				MT8532_ETDM_FORCE_ON_DEFAULT;
		} else if (force_on_1st_trigger) {
			etdm_data->force_on[stream] = true;
			etdm_data->force_on_policy[stream] =
				MT8532_ETDM_FORCE_ON_1ST_TRIGGER;
		}

		if (etdm_data->force_on[stream]) {
			snprintf(prop, sizeof(prop),
				 "mediatek,%s-force-format",
				 of_afe_etdms[i].name);
			dev_dbg(afe->dev, "%s %s ret=%d\n",
				__func__, prop, ret);
			ret = of_property_read_string(np, prop, &str);
			if (ret == 0) {
				temps[0] = mt8532_snd_get_dai_format(str);

				ret = mt8532_snd_get_etdm_format(temps[0]);
				if (ret >= 0)
					etdm_data->format[stream] = ret;
			}

			snprintf(prop, sizeof(prop),
				 "mediatek,%s-force-mclk-freq",
				 of_afe_etdms[i].name);
			dev_dbg(afe->dev, "%s %s ret=%d\n",
				__func__, prop, ret);
			ret = of_property_read_u32(np, prop, &temps[0]);
			if (ret == 0)
				etdm_data->mclk_freq[stream] = temps[0];

			snprintf(prop, sizeof(prop),
				 "mediatek,%s-force-lrck-inverse",
				 of_afe_etdms[i].name);
			etdm_data->lrck_inv[stream] =
				of_property_read_bool(np, prop);

			snprintf(prop, sizeof(prop),
				 "mediatek,%s-force-bck-inverse",
				 of_afe_etdms[i].name);
			etdm_data->bck_inv[stream] =
				of_property_read_bool(np, prop);

			snprintf(prop, sizeof(prop),
				 "mediatek,%s-force-lrck-width",
				 of_afe_etdms[i].name);
			ret = of_property_read_u32(np, prop, &temps[0]);
			dev_dbg(afe->dev, "%s %s ret=%d\n",
				__func__, prop, ret);
			if (ret == 0)
				etdm_data->lrck_width[stream] = temps[0];

			snprintf(prop, sizeof(prop),
				 "mediatek,%s-force-rate",
				 of_afe_etdms[i].name);
			ret = of_property_read_u32(np, prop, &temps[0]);
			dev_dbg(afe->dev, "%s %s ret=%d\n",
				__func__, prop, ret);
			if (ret == 0)
				etdm_data->force_rate[stream] = temps[0];

			snprintf(prop, sizeof(prop),
				 "mediatek,%s-force-channels",
				 of_afe_etdms[i].name);
			ret = of_property_read_u32(np, prop, &temps[0]);
			if (ret == 0)
				etdm_data->force_channels[stream] = temps[0];

			snprintf(prop, sizeof(prop),
				 "mediatek,%s-force-bit-width",
				 of_afe_etdms[i].name);
			ret = of_property_read_u32(np, prop, &temps[0]);
			if (ret == 0)
				etdm_data->force_bit_width[stream] = temps[0];
		}

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-int-lrck-inverse",
			 of_afe_etdms[i].name);
		etdm_data->int_lrck_inv[stream] =
			of_property_read_bool(np, prop);

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-int-bck-inverse",
			 of_afe_etdms[i].name);
		etdm_data->int_bck_inv[stream] =
			of_property_read_bool(np, prop);
	}

	ret = of_property_read_u32_array(np, "mediatek,dmic-two-wire-mode",
					 &temps[0],
					 1);
	dev_dbg(afe->dev, "%s dmic-two-wire-mode ret=%d\n", __func__, ret);
	if (ret == 0)
		priv->dmic_data.two_wire_mode = temps[0];

	ret = of_property_read_u32_array(np, "mediatek,dmic-clk-phases",
					 &temps[0],
					 2);
	dev_dbg(afe->dev, "%s dmic-clk-phases ret=%d\n", __func__, ret);
	if (ret == 0) {
		priv->dmic_data.clk_phase_sel_ch1 = temps[0];
		priv->dmic_data.clk_phase_sel_ch2 = temps[1];
	} else if (!priv->dmic_data.two_wire_mode) {
		priv->dmic_data.clk_phase_sel_ch1 = 0;
		priv->dmic_data.clk_phase_sel_ch2 = 4;
	}

	ret = of_property_read_u32_array(np, "mediatek,dmic-src-sels",
					 &priv->dmic_data.dmic_src_sel[0],
					 DMIC_MAX_CH);
	dev_dbg(afe->dev, "%s dmic-src-sels ret=%d\n", __func__, ret);
	if (ret != 0) {
		if (priv->dmic_data.two_wire_mode) {
			priv->dmic_data.dmic_src_sel[0] = 0;
			priv->dmic_data.dmic_src_sel[1] = 1;
			priv->dmic_data.dmic_src_sel[2] = 2;
			priv->dmic_data.dmic_src_sel[3] = 3;
			priv->dmic_data.dmic_src_sel[4] = 4;
			priv->dmic_data.dmic_src_sel[5] = 5;
			priv->dmic_data.dmic_src_sel[6] = 6;
			priv->dmic_data.dmic_src_sel[7] = 7;
		} else {
			priv->dmic_data.dmic_src_sel[0] = 0;
			priv->dmic_data.dmic_src_sel[2] = 1;
			priv->dmic_data.dmic_src_sel[4] = 2;
			priv->dmic_data.dmic_src_sel[6] = 3;
		}
	}

	priv->dmic_data.iir_on = of_property_read_bool(np,
		"mediatek,dmic-iir-on");

	of_property_read_u32(np,
		"mediatek,dmic-setup-time-us",
		&priv->dmic_data.setup_time_us);

	for (i = 0; i < ARRAY_SIZE(of_fe_table); i++) {
		bool valid_sram;
		struct mt8532_fe_dai_data *fe_data;

		fe_data = &priv->fe_data[of_fe_table[i].val];

		memset(temps, 0, sizeof(temps));

		snprintf(prop, sizeof(prop), "mediatek,%s-use-sram",
			 of_fe_table[i].name);
		ret = of_property_read_u32_array(np, prop, &temps[0], 2);
		dev_dbg(afe->dev, "%s %s ret=%d\n", __func__, prop, ret);
		if (ret == 0) {
			valid_sram = mt8532_afe_validate_sram(afe, temps[0],
				temps[1]);
			if (valid_sram) {
				fe_data->sram_phy_addr = temps[0];
				fe_data->sram_size = temps[1];
				fe_data->sram_vir_addr =
					mt8532_afe_sram_pa_to_va(afe,
						fe_data->sram_phy_addr);
			} else {
				dev_info(afe->dev,
					 "%s %s validate 0x%x 0x%x fail\n",
					 __func__, of_fe_table[i].name,
					 temps[0], temps[1]);
			}
		}

		snprintf(prop, sizeof(prop), "mediatek,%s-prealloc-size",
			 of_fe_table[i].name);
		ret = of_property_read_u32(np, prop, &temps[0]);
		dev_dbg(afe->dev, "%s %s ret=%d\n", __func__, prop, ret);
		if (ret == 0)
			fe_data->prealloc_size = temps[0];

		snprintf(prop, sizeof(prop), "mediatek,%s-pbuf-size-conf",
			 of_fe_table[i].name);
		ret = of_property_read_u32(np, prop, &temps[0]);

		dev_dbg(afe->dev, "%s %s ret=%d\n", __func__, prop, ret);

		if ((ret == 0) && (temps[0] < PBUF_SIZE_CONF_NUM))
			fe_data->pbuf_size_conf = temps[0];

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-min-hw-irq-period-us",
			 of_fe_table[i].name);
		ret = of_property_read_u32(np, prop, &temps[0]);
		if (ret == 0)
			fe_data->min_hw_irq_period_us = temps[0];
	}

	for (i = 0; i < MT8532_AFE_TDMOUT_CONN_CFG_NUM; i++) {
		struct mt8532_afe_tdmout_conn_data *tdmo_conn;

		tdmo_conn = &priv->tdmo_conn;

		snprintf(prop, sizeof(prop), "mediatek,etdm1-out-O_%zu-cfg",
			 i);
		ret = of_property_read_u32(np, prop, &temps[0]);
		if ((ret == 0) && (temps[0] >= MT8532_AFE_TDMOUT_CONN_I0) &&
		    (temps[0] <= MT8532_AFE_TDMOUT_CONN_I15))
			tdmo_conn->out_cfg[i] = temps[0];
	}

	for (i = 0; i < ARRAY_SIZE(of_tune_slave_inputs); i++) {
		struct ext_clk_tune_property *tune;

		tune = &priv->clk_tune.props[of_tune_slave_inputs[i].val];

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-clk-tuning-enabled",
			 of_tune_slave_inputs[i].name);
		tune->do_tune = of_property_read_bool(np, prop);
		if (!tune->do_tune)
			continue;

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-clk-tuning-period-ms",
			 of_tune_slave_inputs[i].name);
		ret = of_property_read_u32(np, prop, &temps[0]);
		if (ret == 0)
			tune->period_ms = temps[0];

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-clk-step-ppm",
			 of_tune_slave_inputs[i].name);
		ret = of_property_read_u32(np, prop, &temps[0]);
		if (ret == 0)
			tune->adj_step_ppm = temps[0];

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-clk-step-interval-us",
			 of_tune_slave_inputs[i].name);
		ret = of_property_read_u32(np, prop, &temps[0]);
		if (ret == 0)
			tune->adj_step_us = temps[0];
	}

	for (i = 0; i < ARRAY_SIZE(of_tune_outputs); i++) {
		struct mt8532_afe_ext_clk_tune_data *clk_tune;

		clk_tune = &priv->clk_tune;

		snprintf(prop, sizeof(prop),
			 "mediatek,%s-clk-tuning-enabled",
			 of_tune_outputs[i].name);
		if (of_property_read_bool(np, prop)) {
			temps[0] =
				mt8532_afe_get_be_idx(of_tune_outputs[i].val);

			clk_tune->outputs |= BIT(temps[0]);
		}
	}

	priv->use_bypass_afe_pinmux = of_property_read_bool(np,
		"mediatek,use-bypass-afe-pinmux");

	of_property_read_u32(np,
		"mediatek,spdif-in-opt-mux",
		&priv->spdif_in_data.ports_mux[SPDIF_IN_PORT_OPT]);

	of_property_read_u32(np,
		"mediatek,spdif-in-coa-mux",
		&priv->spdif_in_data.ports_mux[SPDIF_IN_PORT_COAXIAL]);

	of_property_read_u32(np,
		"mediatek,spdif-in-arc-mux",
		&priv->spdif_in_data.ports_mux[SPDIF_IN_PORT_ARC]);

	priv->dl8_enable_24ch_output = of_property_read_bool(np,
		"mediatek,dl8-enable-24ch-output");

	ret = of_property_read_u32(np,
		"mediatek,dl8-max-main-channels",
		&temps[0]);
	if ((ret == 0) && (temps[0] <= 16))
		priv->dl8_max_main_channels = temps[0];

	priv->pinctrl = devm_pinctrl_get(afe->dev);
	if (!IS_ERR(priv->pinctrl)) {
		for (i = 0; i < AFE_PIN_STATE_MAX; i++) {
			priv->pin_states[i] =
				pinctrl_lookup_state(priv->pinctrl,
					mt8532_afe_pin_str[i]);
			if (IS_ERR(priv->pin_states[i])) {
				priv->pin_states[i] = NULL;
				continue;
			}
		}
	} else
		priv->pinctrl = NULL;

	priv->handle_etdm_force_in_suspend_resume = of_property_read_bool(np,
			"mediatek,handle-etdm-force-in-suspend-resume");

	return 0;
}

static int mt8532_afe_pcm_probe(struct snd_soc_platform *platform)
{
	int ret = 0;
	int i = 0;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(platform);
	struct mt8532_afe_private *priv = afe->platform_priv;
	struct mt8532_afe_ext_clk_tune_data *clk_tune = &priv->clk_tune;
	bool need_clk_tune = false;
#ifdef CALI_RESULT_CHECK
	struct mt8532_cali_result *cali_res = &priv->cali_res;
#endif
	static const unsigned int default_select_pins[] = {
		AFE_PIN_STATE_DEFAULT,
		AFE_PIN_STATE_ETDM1_OUT_OFF,
		AFE_PIN_STATE_ETDM1_IN_OFF,
		AFE_PIN_STATE_ETDM2_OUT_OFF,
		AFE_PIN_STATE_ETDM2_IN_OFF,
	};

#if ARCIN_FEATURE_SUPPORT
	INIT_WORK(&uevent_work, uevent_notify_work);
	uevent_notify_init();
#endif
	ret = mt8532_afe_add_controls(platform);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(default_select_pins); i++)
		mt8532_afe_select_pin_state(afe, default_select_pins[i]);

	ret = mt8532_afe_handle_etdm_force_on(afe, true);

	//use fix 26M as hp clock first
	regmap_update_bits(afe->regmap, AUDIO_TOP_CON1,
			AUD_TCON1_A1SYS_HP_SEL_MASK,
			AUD_TCON1_A1SYS_HP_SEL_VAL(2));

	dev_dbg(afe->dev, "%s %d\n", __func__, __LINE__);

	if (ret)
		return ret;


	/* initialization for clock tuning */
	for (i = 0; i < CLK_TUNE_SOURCE_NUM; i++) {
		if (clk_tune->props[i].do_tune) {
			need_clk_tune = true;
			clk_tune->props[i].tune_id = i;
			break;
		}
	}

	if (need_clk_tune) {
		struct clk *clk;

		clk_tune->afe = afe;

		clk = priv->clocks[MT8532_CLK_APMIXED_APLL1];

		clk_tune->apll1_rate = clk_get_rate(clk);

		clk = priv->clocks[MT8532_CLK_APMIXED_APLL2];

		clk_tune->apll2_rate = clk_get_rate(clk);

		INIT_DELAYED_WORK(&clk_tune->clk_tune_work, clk_tune_adjust);
	}

#ifdef CALI_RESULT_CHECK
	cali_res->afe = afe;
	hrtimer_init(&cali_res->cali_hrt, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	cali_res->cali_hrt.function = cali_res_hrt_func;
#endif

	return 0;
}

static int mt8532_afe_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mt8532_afe_private *priv = afe->platform_priv;
	int id = rtd->cpu_dai->id;
	struct mt8532_fe_dai_data *fe_data = &priv->fe_data[id];
	struct snd_pcm_substream *substream;
	int stream;
	int ret = 0;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (substream) {
			struct snd_dma_buffer *buf = &substream->dma_buffer;

			buf->dev.type = SNDRV_DMA_TYPE_DEV;
			buf->dev.dev = card->dev;
			buf->private_data = NULL;
		}
	}

	if (fe_data->prealloc_size > 0) {
		unsigned int max = afe->mtk_afe_hardware->buffer_bytes_max;

		ret = snd_pcm_lib_preallocate_pages_for_all(pcm,
			SNDRV_DMA_TYPE_DEV, card->dev,
			min(fe_data->prealloc_size, max),
			max);
	}

	return ret;
}

static snd_pcm_uframes_t mt8532_afe_pcm_pointer
	(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int id = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[id];
	const struct mtk_base_memif_data *memif_data = memif->data;
	struct device *dev = afe->dev;
	int reg_ofs_base = memif_data->reg_ofs_base;
	int reg_ofs_cur = memif_data->reg_ofs_cur;
	unsigned int hw_ptr = 0, hw_base = 0;
	int ret, pcm_ptr_bytes;

	ret = regmap_read(afe->regmap, reg_ofs_cur, &hw_ptr);
	if (ret || hw_ptr == 0) {
		dev_info(dev, "%s hw_ptr err\n", __func__);
		pcm_ptr_bytes = 0;
		goto POINTER_RETURN_FRAMES;
	}

	ret = regmap_read(afe->regmap, reg_ofs_base, &hw_base);
	if (ret || hw_base == 0) {
		dev_info(dev, "%s hw_base err\n", __func__);
		pcm_ptr_bytes = 0;
		goto POINTER_RETURN_FRAMES;
	}

	pcm_ptr_bytes = hw_ptr - hw_base;

	if (memif->aux_channels > 0) {
		struct snd_pcm_runtime *runtime = substream->runtime;
		unsigned int main_channels;
		unsigned int frame_bytes;

		main_channels = runtime->channels - memif->aux_channels;

		frame_bytes = frames_to_bytes(runtime, 1);

		pcm_ptr_bytes = pcm_ptr_bytes * runtime->channels /
				main_channels;

		pcm_ptr_bytes = pcm_ptr_bytes / frame_bytes * frame_bytes;
	}

POINTER_RETURN_FRAMES:
	return bytes_to_frames(substream->runtime, pcm_ptr_bytes);
}

static int mt8532_afe_pcm_copy_to_main_and_aux(
	struct mtk_base_afe_memif *memif,
	snd_pcm_uframes_t pos,
	void __user *buf,
	snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = memif->substream->runtime;
	char *main_hwbuf;
	char *aux_hwbuf;
	unsigned int offset, main_off, aux_off;
	unsigned int main_channels, aux_channels;
	size_t i, main_frame_bytes, aux_frame_bytes;
	char __user *from = buf;

	aux_channels = memif->aux_channels;
	main_channels = runtime->channels - aux_channels;

	aux_frame_bytes = aux_channels * samples_to_bytes(runtime, 1);
	main_frame_bytes = main_channels * samples_to_bytes(runtime, 1);

	offset = frames_to_bytes(runtime, pos);

	main_off = offset * main_channels / runtime->channels;
	aux_off = offset * aux_channels / runtime->channels;

	main_hwbuf = runtime->dma_area + main_off;
	aux_hwbuf = runtime->dma_area + memif->buffer_size + aux_off;

	for (i = 0; i < count; i++) {
		copy_from_user_toio(main_hwbuf, from, main_frame_bytes);
		main_hwbuf += main_frame_bytes;
		from += main_frame_bytes;
		copy_from_user_toio(aux_hwbuf, from, aux_frame_bytes);
		aux_hwbuf += aux_frame_bytes;
		from += aux_frame_bytes;
	}

	return 0;
}

static int mt8532_afe_pcm_copy(struct snd_pcm_substream *substream,
	int channel,
	snd_pcm_uframes_t pos,
	void __user *buf,
	snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	int id = rtd->cpu_dai->id;
	struct mtk_base_afe_memif *memif = &afe->memif[id];
	int stream = substream->stream;
	ssize_t copy_bytes = count; //frames_to_bytes(runtime, count);
	char *hwbuf = runtime->dma_area + pos ; //frames_to_bytes(runtime, pos);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (memif->aux_channels > 0)
			return mt8532_afe_pcm_copy_to_main_and_aux(
				memif, pos, buf, count);

		if (copy_from_user_toio(hwbuf, buf, copy_bytes))
			return -EFAULT;
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (copy_to_user_fromio(buf, hwbuf, copy_bytes))
			return -EFAULT;
	}

	return 0;
}

const struct snd_pcm_ops mt8532_afe_pcm_ops = {
	.ioctl = snd_pcm_lib_ioctl,
	.pointer = mt8532_afe_pcm_pointer,
	.copy_user = mt8532_afe_pcm_copy,
};

const struct snd_soc_platform_driver mt8532_afe_pcm_platform = {
	.probe = mt8532_afe_pcm_probe,
	.pcm_new = mt8532_afe_pcm_new,
	.pcm_free = mtk_afe_pcm_free,
	.ops = &mt8532_afe_pcm_ops,
};

static int mt8532_afe_pcm_dev_probe(struct platform_device *pdev)
{
	int ret, i, sel_irq;
	struct mtk_base_afe *afe;
	struct mt8532_afe_private *afe_priv;
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "%s enter\n", __func__);

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(33));
	if (ret)
		return ret;

	afe = devm_kzalloc(dev, sizeof(*afe), GFP_KERNEL);
	if (!afe)
		return -ENOMEM;

	afe->platform_priv = devm_kzalloc(dev, sizeof(*afe_priv), GFP_KERNEL);
	afe_priv = afe->platform_priv;
	if (!afe_priv)
		return -ENOMEM;

	mt8532_afe_init_private(afe_priv);

	dev_dbg(afe->dev, "%s %d\n", __func__, __LINE__);

	afe->dev = dev;

	ret = mt8532_afe_parse_of(afe, pdev);

	dev_dbg(afe->dev, "%s %d\n", __func__, __LINE__);

	if (ret)
		return ret;

	/* initial audio related clock */
	ret = mt8532_afe_init_audio_clk(afe);
	if (ret) {
		dev_err(afe->dev, "mt8532_afe_init_audio_clk fail\n");
		return ret;
	}
	dev_dbg(afe->dev, "%s %d\n", __func__, __LINE__);

#ifdef DEBUG_AFE_REGISTER_RW
	afe->regmap = devm_regmap_init(dev,
		&mt8532_afe_regmap_bus, afe,
		&mt8532_afe_regmap_config);
#else
	afe->regmap = devm_regmap_init_mmio_clk(dev, "aud_intbussel",
		afe->base_addr, &mt8532_afe_regmap_config);
	//afe->regmap = devm_regmap_init_mmio(dev,
	//	afe->base_addr, &mt8532_afe_regmap_config);
#endif
	if (IS_ERR(afe->regmap))
		return PTR_ERR(afe->regmap);

	/* memif % irq initialize*/
	afe->memif_size = MT8532_AFE_MEMIF_NUM;
	afe->memif = devm_kcalloc(afe->dev, afe->memif_size,
				  sizeof(*afe->memif), GFP_KERNEL);
	if (!afe->memif)
		return -ENOMEM;

	afe->irqs_size = MT8532_AFE_IRQ_NUM;
	afe->irqs = devm_kcalloc(afe->dev, afe->irqs_size,
				 sizeof(*afe->irqs), GFP_KERNEL);
	if (!afe->irqs)
		return -ENOMEM;
	for (i = 0; i < afe->irqs_size; i++)
		afe->irqs[i].irq_data = &irq_data[i];

	for (i = 0; i < afe->memif_size; i++) {
		afe->memif[i].afe = afe;
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
	dev_dbg(afe->dev, "%s %d\n", __func__, __LINE__);

	afe->mtk_afe_hardware = &mt8532_afe_hardware;
	afe->memif_fs = mt8532_memif_fs;
	afe->memif_engen_apll_sel = mt8532_memif_engen_apll_sel;
	afe->irq_fs = mt8532_irq_fs;
	afe->irq_engen_apll_sel = mt8532_irq_engen_apll_sel;
	afe->alloc_dmabuf = mt8532_alloc_dmabuf;
	afe->free_dmabuf = mt8532_free_dmabuf;

	platform_set_drvdata(pdev, afe);
	pm_runtime_enable(dev);
	if (!pm_runtime_enabled(dev)) {
		dev_warn(afe->dev, "%s pm_runtime not enabled\n", __func__);
		ret = mt8532_afe_runtime_resume(dev);
		if (ret) {
			dev_err(afe->dev, "4444\n");
			goto err_pm_disable;
		}
	}

	pm_runtime_get_sync(dev);

	afe->reg_back_up_list = mt8532_afe_backup_list;
	afe->reg_back_up_list_num = ARRAY_SIZE(mt8532_afe_backup_list);
	afe->runtime_resume = mt8532_afe_runtime_resume;
	afe->runtime_suspend = mt8532_afe_runtime_suspend;

	//mt8532_afe_enable_reg_rw_clk(afe);

	ret = snd_soc_register_platform(dev, &mt8532_afe_pcm_platform);
	if (ret)
		goto err_platform;

	ret = snd_soc_register_component(dev,
					 &mt8532_afe_pcm_dai_component,
					 mt8532_afe_pcm_dais,
					 ARRAY_SIZE(mt8532_afe_pcm_dais));

	//mt8532_afe_disable_reg_rw_clk(afe);

	if (ret)
		goto err_component;

	mt8532_afe_init_registers(afe);

	mt8532_afe_init_debugfs(afe);
#if ARCIN_FEATURE_SUPPORT
	g_afe = afe;
#endif
	dev_dbg(dev, "MT8532 AFE driver initialized.\n");
	return 0;

err_component:
	snd_soc_unregister_platform(dev);
err_platform:
	pm_runtime_put_sync(dev);
err_pm_disable:
	pm_runtime_disable(dev);
	return ret;
}

static int mt8532_afe_pcm_dev_remove(struct platform_device *pdev)
{
	struct mtk_base_afe *afe = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	//mt8532_afe_disable_reg_rw_clk(afe);

	mt8532_afe_cleanup_debugfs(afe);
	if (!pm_runtime_status_suspended(dev))
		mt8532_afe_runtime_suspend(dev);

	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	snd_soc_unregister_component(dev);
	snd_soc_unregister_platform(dev);
	return 0;
}

static const struct of_device_id mt8532_afe_pcm_dt_match[] = {
	{ .compatible = "mediatek,mt8532-afe-pcm", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt8532_afe_pcm_dt_match);

static const struct dev_pm_ops mt8532_afe_pm_ops = {
	SET_RUNTIME_PM_OPS(mt8532_afe_dev_runtime_suspend,
			   mt8532_afe_dev_runtime_resume, NULL)
};

static struct platform_driver mt8532_afe_pcm_driver = {
	.driver = {
		   .name = "mt8532-afe-pcm",
		   .of_match_table = mt8532_afe_pcm_dt_match,
		   .pm = &mt8532_afe_pm_ops,
	},
	.probe = mt8532_afe_pcm_dev_probe,
	.remove = mt8532_afe_pcm_dev_remove,
};

module_platform_driver(mt8532_afe_pcm_driver);

MODULE_DESCRIPTION("Mediatek ALSA SoC AFE platform driver");
MODULE_AUTHOR("Wen Cai <wen.cai@mediatek.com>");
MODULE_LICENSE("GPL v2");
