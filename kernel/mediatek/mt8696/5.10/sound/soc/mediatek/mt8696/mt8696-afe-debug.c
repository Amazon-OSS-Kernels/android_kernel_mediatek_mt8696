// SPDX-License-Identifier: GPL-2.0
/*
 * mt8696-afe-debug.c  --  Mediatek 8696 audio debugfs
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Sail Yang <sail.yang@mediatek.com>
 *
 */

#include "mt8696-afe-debug.h"
#include "mt8696-reg.h"
#include "mt8696-afe-utils.h"
#include "mt8696-afe-common.h"
#include "../common/mtk-base-afe.h"
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/device.h>


#ifdef CONFIG_DEBUG_FS

struct mt8696_afe_debug_fs {
	char *fs_name;
	const struct file_operations *fops;
};

struct afe_dump_reg_attr {
	uint32_t offset;
	char *name;
};

#define DUMP_REG_ENTRY(reg) {reg, #reg}

static const struct afe_dump_reg_attr all_dump_regs[] = {
	DUMP_REG_ENTRY(AUDIO_TOP_CON0),
	DUMP_REG_ENTRY(AUDIO_TOP_CON4),
	DUMP_REG_ENTRY(AUDIO_TOP_CON5),
	DUMP_REG_ENTRY(ASYS_TOP_CON),
	DUMP_REG_ENTRY(AFE_DAC_CON0),
	DUMP_REG_ENTRY(ETDM_OUT1_CON0),
	DUMP_REG_ENTRY(ETDM_OUT1_CON1),
	DUMP_REG_ENTRY(ETDM_OUT1_CON2),
	DUMP_REG_ENTRY(ETDM_OUT1_CON3),
	DUMP_REG_ENTRY(ETDM_OUT1_CON4),
	DUMP_REG_ENTRY(ETDM_IN2_CON0),
	DUMP_REG_ENTRY(ETDM_IN2_CON1),
	DUMP_REG_ENTRY(ETDM_IN2_CON2),
	DUMP_REG_ENTRY(ETDM_IN2_CON3),
	DUMP_REG_ENTRY(ETDM_IN2_CON4),
	DUMP_REG_ENTRY(ETDM_OUT2_CON0),
	DUMP_REG_ENTRY(ETDM_OUT2_CON1),
	DUMP_REG_ENTRY(ETDM_OUT2_CON2),
	DUMP_REG_ENTRY(ETDM_OUT2_CON3),
	DUMP_REG_ENTRY(ETDM_OUT2_CON4),
	DUMP_REG_ENTRY(ETDM_COWORK_CON0),
	DUMP_REG_ENTRY(ETDM_COWORK_CON1),
	DUMP_REG_ENTRY(ETDM_COWORK_CON2),
	DUMP_REG_ENTRY(ETDM_COWORK_CON3),
	DUMP_REG_ENTRY(AFE_DAC_CON1),
	DUMP_REG_ENTRY(AFE_DL5_BASE),
	DUMP_REG_ENTRY(AFE_DL5_CUR),
	DUMP_REG_ENTRY(AFE_DL5_END),
	DUMP_REG_ENTRY(AFE_DL5_CON0),
	DUMP_REG_ENTRY(AFE_DL8_BASE),
	DUMP_REG_ENTRY(AFE_DL8_CUR),
	DUMP_REG_ENTRY(AFE_DL8_END),
	DUMP_REG_ENTRY(AFE_DL8_CON0),
	DUMP_REG_ENTRY(AFE_UL1_BASE),
	DUMP_REG_ENTRY(AFE_UL1_CUR),
	DUMP_REG_ENTRY(AFE_UL1_END),
	DUMP_REG_ENTRY(AFE_UL1_CON0),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON0),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON1),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON2),
	DUMP_REG_ENTRY(ASYS_IRQ1_CON),
	DUMP_REG_ENTRY(ASYS_IRQ2_CON),
	DUMP_REG_ENTRY(ASYS_IRQ3_CON),
	DUMP_REG_ENTRY(ASYS_IRQ_CLR),
	DUMP_REG_ENTRY(ASYS_IRQ_STATUS),
	DUMP_REG_ENTRY(AFE_IRQ_STATUS),
	DUMP_REG_ENTRY(AFE_IRQ_MASK),
	DUMP_REG_ENTRY(ASYS_IRQ_MASK),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON0),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON1),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON2),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON3),
	DUMP_REG_ENTRY(AFE_BUS_MON1),
	DUMP_REG_ENTRY(ETDM_OUT1_MONITOR),
	DUMP_REG_ENTRY(ETDM_IN2_MONITOR),
	DUMP_REG_ENTRY(ETDM_OUT2_MONITOR),
	DUMP_REG_ENTRY(AFE_LOOPBACK_CFG0),
};

static const struct afe_dump_reg_attr etdm_dump_regs[] = {
	DUMP_REG_ENTRY(AUDIO_TOP_CON0),
	DUMP_REG_ENTRY(AUDIO_TOP_CON4),
	DUMP_REG_ENTRY(AUDIO_TOP_CON5),
	DUMP_REG_ENTRY(ASYS_TOP_CON),
	DUMP_REG_ENTRY(AFE_DAC_CON0),
	DUMP_REG_ENTRY(ETDM_OUT1_CON0),
	DUMP_REG_ENTRY(ETDM_OUT1_CON1),
	DUMP_REG_ENTRY(ETDM_OUT1_CON2),
	DUMP_REG_ENTRY(ETDM_OUT1_CON3),
	DUMP_REG_ENTRY(ETDM_OUT1_CON4),
	DUMP_REG_ENTRY(ETDM_IN2_CON0),
	DUMP_REG_ENTRY(ETDM_IN2_CON1),
	DUMP_REG_ENTRY(ETDM_IN2_CON2),
	DUMP_REG_ENTRY(ETDM_IN2_CON3),
	DUMP_REG_ENTRY(ETDM_IN2_CON4),
	DUMP_REG_ENTRY(ETDM_OUT2_CON0),
	DUMP_REG_ENTRY(ETDM_OUT2_CON1),
	DUMP_REG_ENTRY(ETDM_OUT2_CON2),
	DUMP_REG_ENTRY(ETDM_OUT2_CON3),
	DUMP_REG_ENTRY(ETDM_OUT2_CON4),
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
	DUMP_REG_ENTRY(AFE_DL5_BASE),
	DUMP_REG_ENTRY(AFE_DL5_CUR),
	DUMP_REG_ENTRY(AFE_DL5_END),
	DUMP_REG_ENTRY(AFE_DL5_CON0),
	DUMP_REG_ENTRY(AFE_DL8_BASE),
	DUMP_REG_ENTRY(AFE_DL8_CUR),
	DUMP_REG_ENTRY(AFE_DL8_END),
	DUMP_REG_ENTRY(AFE_DL8_CON0),
	DUMP_REG_ENTRY(AFE_UL1_BASE),
	DUMP_REG_ENTRY(AFE_UL1_CUR),
	DUMP_REG_ENTRY(AFE_UL1_END),
	DUMP_REG_ENTRY(AFE_UL1_CON0),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON0),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON1),
	DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON2),
};

static const struct afe_dump_reg_attr irq_dump_regs[] = {
	DUMP_REG_ENTRY(ASYS_TOP_CON),
	DUMP_REG_ENTRY(ASYS_IRQ1_CON),
	DUMP_REG_ENTRY(ASYS_IRQ2_CON),
	DUMP_REG_ENTRY(ASYS_IRQ3_CON),
	DUMP_REG_ENTRY(ASYS_IRQ_CLR),
	DUMP_REG_ENTRY(ASYS_IRQ_STATUS),
	DUMP_REG_ENTRY(AFE_IRQ_STATUS),
	DUMP_REG_ENTRY(AFE_IRQ_MASK),
	DUMP_REG_ENTRY(ASYS_IRQ_MASK),
};

static const struct afe_dump_reg_attr dbg_dump_regs[] = {
	DUMP_REG_ENTRY(AFE_SINEGEN_CON0),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON1),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON2),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON3),
	DUMP_REG_ENTRY(AFE_BUS_MON1),
	DUMP_REG_ENTRY(ETDM_OUT1_MONITOR),
	DUMP_REG_ENTRY(ETDM_IN2_MONITOR),
	DUMP_REG_ENTRY(ETDM_OUT2_MONITOR),
	DUMP_REG_ENTRY(AFE_LOOPBACK_CFG0),
};

static ssize_t mt8696_afe_dump_registers(char __user *user_buf,
					 size_t count,
					 loff_t *pos,
					 struct mtk_base_afe *afe,
					 const struct afe_dump_reg_attr *regs,
					 size_t regs_len)
{
	ssize_t ret = 0, i = 0;
	char *buf;
	unsigned int reg_value = 0;
	int n = 0;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	mt8696_afe_enable_main_clk(afe);

	for (i = 0; i < regs_len; i++) {
		if (regmap_read(afe->regmap, regs[i].offset, &reg_value))
			n += scnprintf(buf + n, count - n, "%s = N/A\n",
				       regs[i].name);
		else
			n += scnprintf(buf + n, count - n, "%s = 0x%x\n",
				       regs[i].name, reg_value);
	}

	mt8696_afe_disable_main_clk(afe);

	ret = simple_read_from_buffer(user_buf, count, pos, buf, n);
	kfree(buf);

	return ret;
}

static ssize_t mt8696_afe_all_read_file(struct file *file,
				    char __user *user_buf,
				    size_t count,
				    loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8696_afe_dump_registers(user_buf, count, pos, afe,
					all_dump_regs,
					ARRAY_SIZE(all_dump_regs));

	return ret;
}

static ssize_t mt8696_afe_etdm_read_file(struct file *file,
				    char __user *user_buf,
				    size_t count,
				    loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8696_afe_dump_registers(user_buf, count, pos, afe,
					etdm_dump_regs,
					ARRAY_SIZE(etdm_dump_regs));

	return ret;
}

static ssize_t mt8696_afe_write_file(struct file *file,
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

	mt8696_afe_enable_main_clk(afe);

	regmap_write(afe->regmap, reg, value);

	mt8696_afe_disable_main_clk(afe);

	return buf_size;
}

static ssize_t mt8696_afe_memif_read_file(struct file *file,
					  char __user *user_buf,
					  size_t count,
					  loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8696_afe_dump_registers(user_buf, count, pos, afe,
					memif_dump_regs,
					ARRAY_SIZE(memif_dump_regs));

	return ret;
}

static ssize_t mt8696_afe_irq_read_file(struct file *file,
					char __user *user_buf,
					size_t count,
					loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8696_afe_dump_registers(user_buf, count, pos, afe,
					irq_dump_regs,
					ARRAY_SIZE(irq_dump_regs));

	return ret;
}

static ssize_t mt8696_afe_dbg_read_file(struct file *file,
					char __user *user_buf,
					size_t count,
					loff_t *pos)
{
	struct mtk_base_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt8696_afe_dump_registers(user_buf, count, pos, afe,
					dbg_dump_regs,
					ARRAY_SIZE(dbg_dump_regs));

	return ret;
}

static const struct file_operations mt8696_afe_all_fops = {
	.open = simple_open,
	.read = mt8696_afe_all_read_file,
	.write = mt8696_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8696_afe_etdm_fops = {
	.open = simple_open,
	.read = mt8696_afe_etdm_read_file,
	.write = mt8696_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8696_afe_memif_fops = {
	.open = simple_open,
	.read = mt8696_afe_memif_read_file,
	.write = mt8696_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8696_afe_irq_fops = {
	.open = simple_open,
	.read = mt8696_afe_irq_read_file,
	.write = mt8696_afe_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt8696_afe_dbg_fops = {
	.open = simple_open,
	.read = mt8696_afe_dbg_read_file,
	.write = mt8696_afe_write_file,
	.llseek = default_llseek,
};

static const
struct mt8696_afe_debug_fs afe_debug_fs[MT8696_AFE_DEBUGFS_NUM] = {
	{"mtksocaudio", &mt8696_afe_all_fops},
	{"mtksocaudioetdm", &mt8696_afe_etdm_fops},
	{"mtksocaudiomemif", &mt8696_afe_memif_fops},
	{"mtksocaudioirq", &mt8696_afe_irq_fops},
	{"mtksocaudiodbg", &mt8696_afe_dbg_fops},
};

#endif

void mt8696_afe_init_debugfs(struct mtk_base_afe *afe)
{
#ifdef CONFIG_DEBUG_FS
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	int i;

	for (i = 0; i < ARRAY_SIZE(afe_debug_fs); i++) {
		afe_priv->debugfs_dentry[i] =
			debugfs_create_file(afe_debug_fs[i].fs_name,
				0644, NULL, afe, afe_debug_fs[i].fops);
		if (!afe_priv->debugfs_dentry[i])
			dev_info(afe->dev, "%s create %s debugfs failed\n",
				 __func__, afe_debug_fs[i].fs_name);
	}
#endif
}

void mt8696_afe_cleanup_debugfs(struct mtk_base_afe *afe)
{
#ifdef CONFIG_DEBUG_FS
	struct mt8696_afe_private *afe_priv = afe->platform_priv;
	int i;

	if (!afe_priv)
		return;

	for (i = 0; i < MT8696_AFE_DEBUGFS_NUM; i++)
		debugfs_remove(afe_priv->debugfs_dentry[i]);
#endif
}
