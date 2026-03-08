// SPDX-License-Identifier: GPL-2.0
/*
 * mt8532-afe-debug.c  --  Mediatek 8532 audio debugfs
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Wen Cai <wen.cai@mediatek.com>
 */

#include "mt8532-afe-debug.h"
#include "mt8532-reg.h"
#include "mt8532-afe-utils.h"
#include "mt8532-afe-common.h"
#include "../common/mtk-base-afe.h"
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/device.h>


#ifdef CONFIG_DEBUG_FS

struct mt8532_afe_debug_fs {
	char *fs_name;
	const struct file_operations *fops;
};

struct afe_dump_reg_attr {
	uint32_t offset;
	char *name;
};

#define DUMP_REG_ENTRY(reg) {reg, #reg}

static const struct afe_dump_reg_attr etdm_dump_regs[] = {
	DUMP_REG_ENTRY(AUDIO_TOP_CON0),
	DUMP_REG_ENTRY(AUDIO_TOP_CON4),
	DUMP_REG_ENTRY(AUDIO_TOP_CON5),
	DUMP_REG_ENTRY(ASYS_TOP_CON),
	DUMP_REG_ENTRY(AFE_DAC_CON0),
	DUMP_REG_ENTRY(ETDM_IN1_CON0),
	DUMP_REG_ENTRY(ETDM_IN1_CON1),
	DUMP_REG_ENTRY(ETDM_IN1_CON2),
	DUMP_REG_ENTRY(ETDM_IN1_CON3),
	DUMP_REG_ENTRY(ETDM_IN1_CON4),
	DUMP_REG_ENTRY(ETDM_IN2_CON0),
	DUMP_REG_ENTRY(ETDM_IN2_CON1),
	DUMP_REG_ENTRY(ETDM_IN2_CON2),
	DUMP_REG_ENTRY(ETDM_IN2_CON3),
	DUMP_REG_ENTRY(ETDM_IN2_CON4),
	DUMP_REG_ENTRY(ETDM_OUT1_CON0),
	DUMP_REG_ENTRY(ETDM_OUT1_CON1),
	DUMP_REG_ENTRY(ETDM_OUT1_CON2),
	DUMP_REG_ENTRY(ETDM_OUT1_CON3),
	DUMP_REG_ENTRY(ETDM_OUT1_CON4),
	DUMP_REG_ENTRY(ETDM_OUT1_CON5),
	DUMP_REG_ENTRY(ETDM_OUT2_CON0),
	DUMP_REG_ENTRY(ETDM_OUT2_CON1),
	DUMP_REG_ENTRY(ETDM_OUT2_CON2),
	DUMP_REG_ENTRY(ETDM_OUT2_CON3),
	DUMP_REG_ENTRY(ETDM_OUT2_CON4),
	DUMP_REG_ENTRY(ETDM_OUT2_CON5),
	DUMP_REG_ENTRY(ETDM_OUT3_CON0),
	DUMP_REG_ENTRY(ETDM_OUT3_CON1),
	DUMP_REG_ENTRY(ETDM_OUT3_CON2),
	DUMP_REG_ENTRY(ETDM_OUT3_CON3),
	DUMP_REG_ENTRY(ETDM_OUT3_CON4),
	DUMP_REG_ENTRY(ETDM_COWORK_CON0),
	DUMP_REG_ENTRY(ETDM_COWORK_CON1),
	DUMP_REG_ENTRY(ETDM_COWORK_CON2),
	DUMP_REG_ENTRY(ETDM_COWORK_CON3),
};

static const struct afe_dump_reg_attr memif_dump_regs[] = {
	DUMP_REG_ENTRY(ASYS_TOP_CON),
	DUMP_REG_ENTRY(AUDIO_TOP_CON5),
	DUMP_REG_ENTRY(AFE_DAC_CON0),
	DUMP_REG_ENTRY(AFE_DAC_CON1),
	DUMP_REG_ENTRY(AFE_DL2_BASE),
	DUMP_REG_ENTRY(AFE_DL2_CUR),
	DUMP_REG_ENTRY(AFE_DL2_END),
	DUMP_REG_ENTRY(AFE_DL2_CON0),
	DUMP_REG_ENTRY(AFE_DL3_BASE),
	DUMP_REG_ENTRY(AFE_DL3_CUR),
	DUMP_REG_ENTRY(AFE_DL3_END),
	DUMP_REG_ENTRY(AFE_DL3_CON0),
	DUMP_REG_ENTRY(AFE_DL6_BASE),
	DUMP_REG_ENTRY(AFE_DL6_CUR),
	DUMP_REG_ENTRY(AFE_DL6_END),
	DUMP_REG_ENTRY(AFE_DL6_CON0),
	DUMP_REG_ENTRY(AFE_DL7_BASE),
	DUMP_REG_ENTRY(AFE_DL7_CUR),
	DUMP_REG_ENTRY(AFE_DL7_END),
	DUMP_REG_ENTRY(AFE_DL7_CON0),
	DUMP_REG_ENTRY(AFE_DL8_BASE),
	DUMP_REG_ENTRY(AFE_DL8_CUR),
	DUMP_REG_ENTRY(AFE_DL8_END),
	DUMP_REG_ENTRY(AFE_DL8_CON0),
	DUMP_REG_ENTRY(AFE_DL10_BASE),
	DUMP_REG_ENTRY(AFE_DL10_CUR),
	DUMP_REG_ENTRY(AFE_DL10_END),
	DUMP_REG_ENTRY(AFE_DL10_CON0),
	DUMP_REG_ENTRY(AFE_UL1_BASE),
	DUMP_REG_ENTRY(AFE_UL1_CUR),
	DUMP_REG_ENTRY(AFE_UL1_END),
	DUMP_REG_ENTRY(AFE_UL1_CON0),
	DUMP_REG_ENTRY(AFE_UL2_BASE),
	DUMP_REG_ENTRY(AFE_UL2_CUR),
	DUMP_REG_ENTRY(AFE_UL2_END),
	DUMP_REG_ENTRY(AFE_UL2_CON0),
	DUMP_REG_ENTRY(AFE_UL3_BASE),
	DUMP_REG_ENTRY(AFE_UL3_CUR),
	DUMP_REG_ENTRY(AFE_UL3_END),
	DUMP_REG_ENTRY(AFE_UL3_CON0),
	DUMP_REG_ENTRY(AFE_UL4_BASE),
	DUMP_REG_ENTRY(AFE_UL4_CUR),
	DUMP_REG_ENTRY(AFE_UL4_END),
	DUMP_REG_ENTRY(AFE_UL4_CON0),
	DUMP_REG_ENTRY(AFE_UL5_BASE),
	DUMP_REG_ENTRY(AFE_UL5_CUR),
	DUMP_REG_ENTRY(AFE_UL5_END),
	DUMP_REG_ENTRY(AFE_UL5_CON0),
	DUMP_REG_ENTRY(AFE_UL8_BASE),
	DUMP_REG_ENTRY(AFE_UL8_CUR),
	DUMP_REG_ENTRY(AFE_UL8_END),
	DUMP_REG_ENTRY(AFE_UL8_CON0),
	DUMP_REG_ENTRY(AFE_UL9_BASE),
	DUMP_REG_ENTRY(AFE_UL9_CUR),
	DUMP_REG_ENTRY(AFE_UL9_END),
	DUMP_REG_ENTRY(AFE_UL9_CON0),
	DUMP_REG_ENTRY(AFE_UL10_BASE),
	DUMP_REG_ENTRY(AFE_UL10_CUR),
	DUMP_REG_ENTRY(AFE_UL10_END),
	DUMP_REG_ENTRY(AFE_UL10_CON0),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON0),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON1),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON2),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON3),
	DUMP_REG_ENTRY(AFE_CM0_CON),
	DUMP_REG_ENTRY(AFE_CM1_CON),
};

static const struct afe_dump_reg_attr irq_dump_regs[] = {
	DUMP_REG_ENTRY(ASYS_TOP_CON),
	DUMP_REG_ENTRY(AFE_IRQ1_CON),
	DUMP_REG_ENTRY(AFE_IRQ2_CON),
	DUMP_REG_ENTRY(AFE_IRQ3_CON),
	DUMP_REG_ENTRY(ASYS_IRQ1_CON),
	DUMP_REG_ENTRY(ASYS_IRQ2_CON),
	DUMP_REG_ENTRY(ASYS_IRQ3_CON),
	DUMP_REG_ENTRY(ASYS_IRQ4_CON),
	DUMP_REG_ENTRY(ASYS_IRQ5_CON),
	DUMP_REG_ENTRY(ASYS_IRQ6_CON),
	DUMP_REG_ENTRY(ASYS_IRQ7_CON),
	DUMP_REG_ENTRY(ASYS_IRQ8_CON),
	DUMP_REG_ENTRY(ASYS_IRQ9_CON),
	DUMP_REG_ENTRY(ASYS_IRQ10_CON),
	DUMP_REG_ENTRY(ASYS_IRQ11_CON),
	DUMP_REG_ENTRY(ASYS_IRQ_CLR),
	DUMP_REG_ENTRY(ASYS_IRQ_STATUS),
	DUMP_REG_ENTRY(AFE_IRQ_MCU_CLR),
	DUMP_REG_ENTRY(AFE_IRQ_STATUS),
	DUMP_REG_ENTRY(AFE_IRQ_MASK),
	DUMP_REG_ENTRY(ASYS_IRQ_MASK),
};

static const struct afe_dump_reg_attr conn_dump_regs[] = {
	DUMP_REG_ENTRY(AFE_CONN34),
	DUMP_REG_ENTRY(AFE_CONN34_1),
	DUMP_REG_ENTRY(AFE_CONN34_2),
	DUMP_REG_ENTRY(AFE_CONN34_3),
	DUMP_REG_ENTRY(AFE_CONN34_4),
	DUMP_REG_ENTRY(AFE_CONN35),
	DUMP_REG_ENTRY(AFE_CONN35_1),
	DUMP_REG_ENTRY(AFE_CONN35_2),
	DUMP_REG_ENTRY(AFE_CONN35_3),
	DUMP_REG_ENTRY(AFE_CONN35_4),

	DUMP_REG_ENTRY(AFE_CONN48),
	DUMP_REG_ENTRY(AFE_CONN48_1),
	DUMP_REG_ENTRY(AFE_CONN48_2),
	DUMP_REG_ENTRY(AFE_CONN48_3),
	DUMP_REG_ENTRY(AFE_CONN48_4),
	DUMP_REG_ENTRY(AFE_CONN49),
	DUMP_REG_ENTRY(AFE_CONN49_1),
	DUMP_REG_ENTRY(AFE_CONN49_2),
	DUMP_REG_ENTRY(AFE_CONN49_3),
	DUMP_REG_ENTRY(AFE_CONN49_4),

	DUMP_REG_ENTRY(AFE_CONN50),
	DUMP_REG_ENTRY(AFE_CONN50_1),
	DUMP_REG_ENTRY(AFE_CONN50_2),
	DUMP_REG_ENTRY(AFE_CONN50_3),
	DUMP_REG_ENTRY(AFE_CONN50_4),
	DUMP_REG_ENTRY(AFE_CONN51),
	DUMP_REG_ENTRY(AFE_CONN51_1),
	DUMP_REG_ENTRY(AFE_CONN51_2),
	DUMP_REG_ENTRY(AFE_CONN51_3),
	DUMP_REG_ENTRY(AFE_CONN51_4),
	DUMP_REG_ENTRY(AFE_CONN52),
	DUMP_REG_ENTRY(AFE_CONN52_1),
	DUMP_REG_ENTRY(AFE_CONN52_2),
	DUMP_REG_ENTRY(AFE_CONN52_3),
	DUMP_REG_ENTRY(AFE_CONN52_4),
	DUMP_REG_ENTRY(AFE_CONN53),
	DUMP_REG_ENTRY(AFE_CONN53_1),
	DUMP_REG_ENTRY(AFE_CONN53_2),
	DUMP_REG_ENTRY(AFE_CONN53_3),
	DUMP_REG_ENTRY(AFE_CONN53_4),
	DUMP_REG_ENTRY(AFE_CONN54),
	DUMP_REG_ENTRY(AFE_CONN54_1),
	DUMP_REG_ENTRY(AFE_CONN54_2),
	DUMP_REG_ENTRY(AFE_CONN54_3),
	DUMP_REG_ENTRY(AFE_CONN54_4),
	DUMP_REG_ENTRY(AFE_CONN55),
	DUMP_REG_ENTRY(AFE_CONN55_1),
	DUMP_REG_ENTRY(AFE_CONN55_2),
	DUMP_REG_ENTRY(AFE_CONN55_3),
	DUMP_REG_ENTRY(AFE_CONN55_4),
	DUMP_REG_ENTRY(AFE_CONN56),
	DUMP_REG_ENTRY(AFE_CONN56_1),
	DUMP_REG_ENTRY(AFE_CONN56_2),
	DUMP_REG_ENTRY(AFE_CONN56_3),
	DUMP_REG_ENTRY(AFE_CONN56_4),
	DUMP_REG_ENTRY(AFE_CONN57),
	DUMP_REG_ENTRY(AFE_CONN57_1),
	DUMP_REG_ENTRY(AFE_CONN57_2),
	DUMP_REG_ENTRY(AFE_CONN57_3),
	DUMP_REG_ENTRY(AFE_CONN57_4),
	DUMP_REG_ENTRY(AFE_CONN58),
	DUMP_REG_ENTRY(AFE_CONN58_1),
	DUMP_REG_ENTRY(AFE_CONN58_2),
	DUMP_REG_ENTRY(AFE_CONN58_3),
	DUMP_REG_ENTRY(AFE_CONN58_4),
	DUMP_REG_ENTRY(AFE_CONN59),
	DUMP_REG_ENTRY(AFE_CONN59_1),
	DUMP_REG_ENTRY(AFE_CONN59_2),
	DUMP_REG_ENTRY(AFE_CONN59_3),
	DUMP_REG_ENTRY(AFE_CONN59_4),
	DUMP_REG_ENTRY(AFE_CONN60),
	DUMP_REG_ENTRY(AFE_CONN60_1),
	DUMP_REG_ENTRY(AFE_CONN60_2),
	DUMP_REG_ENTRY(AFE_CONN60_3),
	DUMP_REG_ENTRY(AFE_CONN60_4),
	DUMP_REG_ENTRY(AFE_CONN61),
	DUMP_REG_ENTRY(AFE_CONN61_1),
	DUMP_REG_ENTRY(AFE_CONN61_2),
	DUMP_REG_ENTRY(AFE_CONN61_3),
	DUMP_REG_ENTRY(AFE_CONN61_4),
	DUMP_REG_ENTRY(AFE_CONN62),
	DUMP_REG_ENTRY(AFE_CONN62_1),
	DUMP_REG_ENTRY(AFE_CONN62_2),
	DUMP_REG_ENTRY(AFE_CONN62_3),
	DUMP_REG_ENTRY(AFE_CONN62_4),
	DUMP_REG_ENTRY(AFE_CONN63),
	DUMP_REG_ENTRY(AFE_CONN63_1),
	DUMP_REG_ENTRY(AFE_CONN63_2),
	DUMP_REG_ENTRY(AFE_CONN63_3),
	DUMP_REG_ENTRY(AFE_CONN63_4),
	DUMP_REG_ENTRY(AFE_CONN64),
	DUMP_REG_ENTRY(AFE_CONN64_1),
	DUMP_REG_ENTRY(AFE_CONN64_2),
	DUMP_REG_ENTRY(AFE_CONN64_3),
	DUMP_REG_ENTRY(AFE_CONN64_4),
	DUMP_REG_ENTRY(AFE_CONN65),
	DUMP_REG_ENTRY(AFE_CONN65_1),
	DUMP_REG_ENTRY(AFE_CONN65_2),
	DUMP_REG_ENTRY(AFE_CONN65_3),
	DUMP_REG_ENTRY(AFE_CONN65_4),
	DUMP_REG_ENTRY(AFE_CONN66),
	DUMP_REG_ENTRY(AFE_CONN66_1),
	DUMP_REG_ENTRY(AFE_CONN66_2),
	DUMP_REG_ENTRY(AFE_CONN66_3),
	DUMP_REG_ENTRY(AFE_CONN66_4),
	DUMP_REG_ENTRY(AFE_CONN67),
	DUMP_REG_ENTRY(AFE_CONN67_1),
	DUMP_REG_ENTRY(AFE_CONN67_2),
	DUMP_REG_ENTRY(AFE_CONN67_3),
	DUMP_REG_ENTRY(AFE_CONN67_4),
	DUMP_REG_ENTRY(AFE_CONN68),
	DUMP_REG_ENTRY(AFE_CONN68_1),
	DUMP_REG_ENTRY(AFE_CONN68_2),
	DUMP_REG_ENTRY(AFE_CONN68_3),
	DUMP_REG_ENTRY(AFE_CONN68_4),
	DUMP_REG_ENTRY(AFE_CONN69),
	DUMP_REG_ENTRY(AFE_CONN69_1),
	DUMP_REG_ENTRY(AFE_CONN69_2),
	DUMP_REG_ENTRY(AFE_CONN69_3),
	DUMP_REG_ENTRY(AFE_CONN69_4),
	DUMP_REG_ENTRY(AFE_CONN70),
	DUMP_REG_ENTRY(AFE_CONN70_1),
	DUMP_REG_ENTRY(AFE_CONN70_2),
	DUMP_REG_ENTRY(AFE_CONN70_3),
	DUMP_REG_ENTRY(AFE_CONN70_4),
	DUMP_REG_ENTRY(AFE_CONN71),
	DUMP_REG_ENTRY(AFE_CONN71_1),
	DUMP_REG_ENTRY(AFE_CONN71_2),
	DUMP_REG_ENTRY(AFE_CONN71_3),
	DUMP_REG_ENTRY(AFE_CONN71_4),
};

static const struct afe_dump_reg_attr gasrc_dump_regs[] = {
	DUMP_REG_ENTRY(AUDIO_TOP_CON0),
	DUMP_REG_ENTRY(AUDIO_TOP_CON4),
	DUMP_REG_ENTRY(ASMO_TIMING_CON0),
	DUMP_REG_ENTRY(PWR1_ASM_CON1),
	DUMP_REG_ENTRY(GASRC_CFG0),
	DUMP_REG_ENTRY(GASRC_TIMING_CON0),
	DUMP_REG_ENTRY(GASRC_TIMING_CON1),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON0),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON1),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON2),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON3),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON4),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON6),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON7),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON8),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON9),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON10),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON11),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON13),
	DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON14),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON0),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON1),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON2),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON3),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON4),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON6),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON7),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON8),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON9),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON10),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON11),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON13),
	DUMP_REG_ENTRY(AFE_GASRC1_NEW_CON14),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON0),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON1),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON2),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON3),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON4),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON6),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON7),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON8),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON9),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON10),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON11),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON13),
	DUMP_REG_ENTRY(AFE_GASRC2_NEW_CON14),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON0),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON1),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON2),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON3),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON4),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON6),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON7),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON8),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON9),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON10),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON11),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON13),
	DUMP_REG_ENTRY(AFE_GASRC3_NEW_CON14),
};

static const struct afe_dump_reg_attr spdif_dump_regs[] = {
	DUMP_REG_ENTRY(AUDIO_TOP_CON0),
	DUMP_REG_ENTRY(AUDIO_TOP_CON2),
	DUMP_REG_ENTRY(AUDIO_TOP_CON4),
	DUMP_REG_ENTRY(AFE_SPDIF_OUT_CON0),
	DUMP_REG_ENTRY(AFE_IEC_CFG),
	DUMP_REG_ENTRY(AFE_IEC_NSNUM),
	DUMP_REG_ENTRY(AFE_IEC_BURST_INFO),
	DUMP_REG_ENTRY(AFE_IEC_BURST_LEN),
	DUMP_REG_ENTRY(AFE_IEC_NSADR),
	DUMP_REG_ENTRY(AFE_IEC_CHL_STAT0),
	DUMP_REG_ENTRY(AFE_IEC_CHL_STAT1),
	DUMP_REG_ENTRY(AFE_IEC_CHR_STAT0),
	DUMP_REG_ENTRY(AFE_IEC_CHR_STAT1),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CFG0),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CFG1),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CHSTS1),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CHSTS2),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CHSTS3),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CHSTS4),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CHSTS5),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CHSTS6),
	DUMP_REG_ENTRY(AFE_SPDIFIN_DEBUG1),
	DUMP_REG_ENTRY(AFE_SPDIFIN_DEBUG2),
	DUMP_REG_ENTRY(AFE_SPDIFIN_DEBUG3),
	DUMP_REG_ENTRY(AFE_SPDIFIN_DEBUG4),
	DUMP_REG_ENTRY(AFE_SPDIFIN_EC),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CKLOCK_CFG),
	DUMP_REG_ENTRY(AFE_SPDIFIN_BR),
	DUMP_REG_ENTRY(AFE_SPDIFIN_BR_DBG1),
	DUMP_REG_ENTRY(AFE_SPDIFIN_CKFBDIV),
	DUMP_REG_ENTRY(AFE_SPDIFIN_INT_EXT),
	DUMP_REG_ENTRY(AFE_SPDIFIN_INT_EXT2),
	DUMP_REG_ENTRY(SPDIFIN_FREQ_INFO),
	DUMP_REG_ENTRY(SPDIFIN_FREQ_INFO_2),
	DUMP_REG_ENTRY(SPDIFIN_FREQ_INFO_3),
	DUMP_REG_ENTRY(SPDIFIN_FREQ_STATUS),
	DUMP_REG_ENTRY(AFE_MPHONE_MULTI_CON0),
	DUMP_REG_ENTRY(AFE_MPHONE_MULTI_CON1),
	DUMP_REG_ENTRY(AFE_MPHONE_MULTI_MON),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE1),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE2),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE3),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE4),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE5),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE6),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE7),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE8),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE9),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE10),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE11),
	DUMP_REG_ENTRY(SPDIFIN_USERCODE12),
};

static const struct afe_dump_reg_attr dbg_dump_regs[] = {
	DUMP_REG_ENTRY(AFE_SINEGEN_CON0),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON1),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON2),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON3),
	DUMP_REG_ENTRY(AFE_BUS_MON1),
	DUMP_REG_ENTRY(ETDM_IN1_MONITOR),
	DUMP_REG_ENTRY(ETDM_IN2_MONITOR),
	DUMP_REG_ENTRY(ETDM_OUT1_MONITOR),
	DUMP_REG_ENTRY(ETDM_OUT2_MONITOR),
	DUMP_REG_ENTRY(PCM_INTF_CON1),
	DUMP_REG_ENTRY(PCM_INTF_CON2),
	DUMP_REG_ENTRY(AFE_LOOPBACK_CFG0),
	DUMP_REG_ENTRY(AFE_UL1_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_UL1_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_UL2_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_UL2_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_UL3_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_UL3_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_UL4_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_UL4_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_UL5_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_UL5_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_UL8_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_UL8_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_UL9_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_UL9_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_DL2_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_DL2_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_DL3_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_DL3_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_DL6_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_DL6_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_DL7_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_DL7_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_DL8_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_DL8_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_DL8_CHK_SUM3),
	DUMP_REG_ENTRY(AFE_DL8_CHK_SUM4),
	DUMP_REG_ENTRY(AFE_DL10_CHK_SUM1),
	DUMP_REG_ENTRY(AFE_DL10_CHK_SUM2),
	DUMP_REG_ENTRY(AFE_DL10_CHK_SUM3),
	DUMP_REG_ENTRY(AFE_DL10_CHK_SUM4),
	DUMP_REG_ENTRY(AFE_DL10_CHK_SUM5),
	DUMP_REG_ENTRY(AFE_DL10_CHK_SUM6),
};

static ssize_t mt8532_afe_dump_registers(char __user *user_buf,
					 size_t count,
					 loff_t *pos,
					 struct mtk_base_afe *afe,
					 const struct afe_dump_reg_attr *regs,
					 size_t regs_len)
{
	ssize_t ret, i;
	char *buf;
	unsigned int reg_value = 0;
	int n = 0;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	mt8532_afe_enable_reg_rw_clk(afe);

	for (i = 0; i < regs_len; i++) {
		if (regmap_read(afe->regmap, regs[i].offset, &reg_value))
			n += scnprintf(buf + n, count - n, "%s(0x%x) = N/A\n",
				       regs[i].name, regs[i].offset);
		else
			n += scnprintf(buf + n, count - n, "%s(0x%x) = 0x%x\n",
				       regs[i].name, regs[i].offset, reg_value);
	}

	mt8532_afe_disable_reg_rw_clk(afe);

	ret = simple_read_from_buffer(user_buf, count, pos, buf, n);
	kfree(buf);

	return ret;
}

static ssize_t mt8532_afe_etdm_read_file(struct file *file,
				    char __user *user_buf,
				    size_t count,
				    loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8532_afe_dump_registers(user_buf, count, pos, afe,
					etdm_dump_regs,
					ARRAY_SIZE(etdm_dump_regs));

	return ret;
}

static ssize_t mt8532_afe_write_file(struct file *file,
				     const char __user *user_buf,
				     size_t count,
				     loff_t *pos)
{
	char buf[64];
	size_t buf_size;
	char *start = buf;
	char *reg_str;
	char *value_str;
	static const char delim[] = " ,";
	unsigned long reg, value;
	struct mtk_base_afe *afe = file->private_data;

	buf_size = min(count, (sizeof(buf) - 1));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;

	buf[buf_size] = 0;

	reg_str = strsep(&start, delim);
	if (!reg_str || !strlen(reg_str))
		return -EINVAL;

	value_str = strsep(&start, delim);
	if (!value_str || !strlen(value_str))
		return -EINVAL;

	if (kstrtoul(reg_str, 16, &reg))
		return -EINVAL;

	if (kstrtoul(value_str, 16, &value))
		return -EINVAL;

	mt8532_afe_enable_reg_rw_clk(afe);

	regmap_write(afe->regmap, reg, value);

	mt8532_afe_disable_reg_rw_clk(afe);

	return buf_size;
}

static ssize_t mt8532_afe_memif_read_file(struct file *file,
					  char __user *user_buf,
					  size_t count,
					  loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8532_afe_dump_registers(user_buf, count, pos, afe,
					memif_dump_regs,
					ARRAY_SIZE(memif_dump_regs));

	return ret;
}

static ssize_t mt8532_afe_irq_read_file(struct file *file,
					char __user *user_buf,
					size_t count,
					loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8532_afe_dump_registers(user_buf, count, pos, afe,
					irq_dump_regs,
					ARRAY_SIZE(irq_dump_regs));

	return ret;
}

static ssize_t mt8532_afe_conn_read_file(struct file *file,
					 char __user *user_buf,
					 size_t count,
					 loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8532_afe_dump_registers(user_buf, count, pos, afe,
					conn_dump_regs,
					ARRAY_SIZE(conn_dump_regs));

	return ret;
}

static ssize_t mt8532_afe_gasrc_read_file(struct file *file,
					  char __user *user_buf,
					  size_t count,
					  loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8532_afe_dump_registers(user_buf, count, pos, afe,
					gasrc_dump_regs,
					ARRAY_SIZE(gasrc_dump_regs));

	return ret;
}

static ssize_t mt8532_afe_spdif_read_file(struct file *file,
					  char __user *user_buf,
					  size_t count,
					  loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8532_afe_dump_registers(user_buf, count, pos, afe,
					spdif_dump_regs,
					ARRAY_SIZE(spdif_dump_regs));

	return ret;
}

static ssize_t mt8532_afe_dbg_read_file(struct file *file,
					char __user *user_buf,
					size_t count,
					loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8532_afe_dump_registers(user_buf, count, pos, afe,
					dbg_dump_regs,
					ARRAY_SIZE(dbg_dump_regs));

	return ret;
}

static const struct file_operations mt8532_afe_etdm_fops = {
	.open = simple_open,
	.read = mt8532_afe_etdm_read_file,
	.write = mt8532_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8532_afe_memif_fops = {
	.open = simple_open,
	.read = mt8532_afe_memif_read_file,
	.write = mt8532_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8532_afe_irq_fops = {
	.open = simple_open,
	.read = mt8532_afe_irq_read_file,
	.write = mt8532_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8532_afe_conn_fops = {
	.open = simple_open,
	.read = mt8532_afe_conn_read_file,
	.write = mt8532_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8532_afe_gasrc_fops = {
	.open = simple_open,
	.read = mt8532_afe_gasrc_read_file,
	.write = mt8532_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8532_afe_spdif_fops = {
	.open = simple_open,
	.read = mt8532_afe_spdif_read_file,
	.write = mt8532_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8532_afe_dbg_fops = {
	.open = simple_open,
	.read = mt8532_afe_dbg_read_file,
	.write = mt8532_afe_write_file,
	.llseek = default_llseek,
};

static const
struct mt8532_afe_debug_fs afe_debug_fs[MT8532_AFE_DEBUGFS_NUM] = {
	{"mt8532socaudioetdm", &mt8532_afe_etdm_fops},
	{"mt8532socaudiomemif", &mt8532_afe_memif_fops},
	{"mt8532socaudioirq", &mt8532_afe_irq_fops},
	{"mt8532socaudioconn", &mt8532_afe_conn_fops},
	{"mt8532socaudiogasrc", &mt8532_afe_gasrc_fops},
	{"mt8532socaudiospdif", &mt8532_afe_spdif_fops},
	{"mt8532socaudiodbg", &mt8532_afe_dbg_fops},
};

#endif

void mt8532_afe_init_debugfs(struct mtk_base_afe *afe)
{
#ifdef CONFIG_DEBUG_FS
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int i;

	for (i = 0; i < ARRAY_SIZE(afe_debug_fs); i++) {
		afe_priv->debugfs_dentry[i] =
			debugfs_create_file(afe_debug_fs[i].fs_name,
				0644, NULL, afe, afe_debug_fs[i].fops);
		if (!afe_priv->debugfs_dentry[i])
			dev_dbg(afe->dev, "%s create %s debugfs failed\n",
				 __func__, afe_debug_fs[i].fs_name);
	}
#endif
}

void mt8532_afe_cleanup_debugfs(struct mtk_base_afe *afe)
{
#ifdef CONFIG_DEBUG_FS
	struct mt8532_afe_private *afe_priv = afe->platform_priv;
	int i;

	if (!afe_priv)
		return;

	for (i = 0; i < MT8532_AFE_DEBUGFS_NUM; i++)
		debugfs_remove(afe_priv->debugfs_dentry[i]);
#endif
}
