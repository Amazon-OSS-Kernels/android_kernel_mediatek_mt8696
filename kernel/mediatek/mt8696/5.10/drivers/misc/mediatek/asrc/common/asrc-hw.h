/* SPDX-License-Identifier: GPL-2.0 */
/*
 * asrc-hw.h  -- asrc common definition
 *
 * Copyright (C) 2021 MediaTek Inc.
 *
 * Author: Ken.Tai <ken.tai@mediatek.com>
 */

#ifndef ASRC_HW_H_
#define ASRC_HW_H_

#include <linux/regmap.h>
#include <linux/fs.h>

#define INPUT  (0)
#define OUTPUT (1)

#define ASRC_IRQ_ENABLE (1)
#define ASRC_IRQ_DISABLE (!ASRC_IRQ_ENABLE)

#define ASRC_ENABLE (1)
#define ASRC_DISABLE (!ASRC_ENABLE)

#define ASRC_CALI_ENABLE (1)
#define ASRC_CALI_DISABLE (!ASRC_CALI_ENABLE)

#define DENOMINATOR_48K 0x3C00
#define DENOMINATOR_44K 0x3720

#define ASRC_ALIGN_VALUE (16U)
#define ASRC_ALIGN_VALUE_L (16UL)

#define QUERY_STRING_LENGTH_32 (32)
#define QUERY_STRING_LENGTH_256 (256)
#define QUERY_STRING_LENGTH_512 (512)

#define ASRC_SIGNAL_REASON_IBUF_EMPTY  (0U)
#define ASRC_SIGNAL_REASON_OBUF_FULL   (1U)

#define ASSIGN_SIZE(str) (sizeof(str) - strlen(str) - 1)
#define DUMP_REG_ENTRY(reg) {reg, #reg}

struct CALI_FS {
	int integer;
	int decimal;
};

struct CALI_CLK_GET {
	char clk_string[QUERY_STRING_LENGTH_256];
};

struct CALI_SIG_GET {
	char sig_string[QUERY_STRING_LENGTH_512];
};

struct ASRC_QUANTITY_GET {
	char quantity_string[QUERY_STRING_LENGTH_32];
};

#define afe_write(addr, val, regmap) \
	regmap_write(regmap, addr, val)

#define afe_msk_write(addr, val, msk, regmap) \
	regmap_update_bits(regmap, addr, msk, val)

struct asrc_debug_fs {
	char *fs_name;
	const struct file_operations *fops;
};

struct asrc_dump_reg_attr {
	uint32_t offset;
	char *name;
};

struct cali_map_dump {
	char *name;
	int num;
};

struct asrc_buf {
	unsigned char *area;	/* virtual pointer */
	dma_addr_t addr;		/* physical address */
	size_t bytes;		/* buffer size in bytes */
};

struct afe_mem_asrc_buffer {
	u32 base;		/* physical */
	u32 size;
	u32 freq;
	u32 bitwidth;
};

unsigned int afe_read(unsigned int addr, struct regmap *regmap);
void afe_set_bit(u32 addr, int bit, struct regmap *regmap);
void afe_clear_bit(u32 addr, int bit, struct regmap *regmap);
void afe_write_bits(u32 addr, u32 value, int bits, int len, struct regmap *regmap);
u32 afe_read_bits(u32 addr, int bits, int len, struct regmap *regmap);

#endif
