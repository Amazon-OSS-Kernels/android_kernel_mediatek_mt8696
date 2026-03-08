// SPDX-License-Identifier: GPL-2.0
/*
 * asrc-hw.c  -- asrc common resource
 *
 * Copyright (C) 2021 MediaTek Inc.
 *
 * Author: Ken.Tai <ken.tai@mediatek.com>
 */

#include <linux/kernel.h>
#include <linux/regmap.h>
#include "asrc-hw.h"

unsigned int afe_read(unsigned int addr, struct regmap *regmap)
{
	unsigned int val = 0;
	int ret;

	ret = regmap_read(regmap, addr, &val);
	return val;
}

void afe_set_bit(u32 addr, int bit, struct regmap *regmap)
{
	afe_msk_write(addr,
	      (u32)(0x1U) << (u32)bit, (u32)(0x1U) << (u32)bit, regmap);
}

void afe_clear_bit(u32 addr, int bit, struct regmap *regmap)
{
	afe_msk_write(addr, 0x0, (u32)(0x1U) << (u32)bit, regmap);
}

void afe_write_bits(u32 addr, u32 value, int bits, int len, struct regmap *regmap)
{
	u32 u4TargetBitField =
	      (((u32)(0x1U) << (u32)len) - (u32)(1U)) << (u32)bits;
	u32 u4TargetValue = (value << (u32)bits) & u4TargetBitField;
	u32 u4CurrValue;

	u4CurrValue = afe_read(addr, regmap);
	afe_write(addr, ((u4CurrValue & (~u4TargetBitField))
	      | u4TargetValue), regmap);
}

u32 afe_read_bits(u32 addr, int bits, int len, struct regmap *regmap)
{
	u32 u4TargetBitField =
	      (((u32)(0x1U) << (u32)len) - (u32)(1U)) << (u32)bits;
	u32 u4CurrValue = afe_read(addr, regmap);

	return (u4CurrValue & u4TargetBitField) >> (u32)bits;
}
