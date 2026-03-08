/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef _MENULOAD_HW_H_
#define _MENULOAD_HW_H_

#define ML_DEPTH  0x4
#define ML_EN     0x8
#define ML_ADDR0  0xC
#define ML_ADDR1  0x10
#define ML_DS_MSK 0x1C
#define ML_DS_WE  0x40
#define ML_TRIG_MD 0x74
#define ML_64B_EN 0x1C0
#define ML_MSK_EN 0x85C
#define ML_ST     0x6C
#define ML_RST    0x4C

#define ML_MSYS_REG_BASE 0x14013000
#define ML_DSYS_REG_BASE 0x15013000



/* 0x15013004-0x150131fc */
/* 0x14013004-0x140131fc */
/*0x1501385c or 0x1401385c*/

#define	SC3_reg_0004_DYN_SCL_BK1F   0x004

#define	SC3_reg_0008_DYN_SCL_BK1F   0x008

#define	SC3_reg_000C_DYN_SCL_BK1F   0x00C

#define	SC3_reg_0010_DYN_SCL_BK1F   0x010

#define	SC3_reg_0014_DYN_SCL_BK1F   0x014

#define	SC3_reg_0018_DYN_SCL_BK1F   0x018

#define	SC3_reg_001C_DYN_SCL_BK1F   0x01C

#define	SC3_reg_0020_DYN_SCL_BK1F   0x020

#define	SC3_reg_0024_DYN_SCL_BK1F   0x024

#define	SC3_reg_0028_DYN_SCL_BK1F   0x028

#define	SC3_reg_002C_DYN_SCL_BK1F   0x02C

#define	SC3_reg_0030_DYN_SCL_BK1F   0x030

#define	SC3_reg_0034_DYN_SCL_BK1F   0x034

#define	SC3_reg_0038_DYN_SCL_BK1F   0x038

#define	SC3_reg_003C_DYN_SCL_BK1F   0x03C

#define	SC3_reg_0040_DYN_SCL_BK1F   0x040

#define	SC3_reg_0044_DYN_SCL_BK1F   0x044

#define	SC3_reg_0048_DYN_SCL_BK1F   0x048

#define	SC3_reg_004C_DYN_SCL_BK1F   0x04C

#define	SC3_reg_0050_DYN_SCL_BK1F   0x050

#define	SC3_reg_0054_DYN_SCL_BK1F   0x054

#define	SC3_reg_0058_DYN_SCL_BK1F   0x058

#define	SC3_reg_005C_DYN_SCL_BK1F   0x05C

#define	SC3_reg_0060_DYN_SCL_BK1F   0x060

#define	SC3_reg_0064_DYN_SCL_BK1F   0x064

#define	SC3_reg_0068_DYN_SCL_BK1F   0x068

#define	SC3_reg_006C_DYN_SCL_BK1F   0x06C

#define	SC3_reg_0070_DYN_SCL_BK1F   0x070

#define	SC3_reg_0074_DYN_SCL_BK1F   0x074

#define	SC3_reg_0078_DYN_SCL_BK1F   0x078

#define	SC3_reg_007C_DYN_SCL_BK1F   0x07C

#define	SC3_reg_0080_DYN_SCL_BK1F   0x080

#define	SC3_reg_0088_DYN_SCL_BK1F   0x088

#define	SC3_reg_0094_DYN_SCL_BK1F   0x094

#define	SC3_reg_0120_DYN_SCL_BK1F   0x120

#define	SC3_reg_0140_DYN_SCL_BK1F   0x140

#define	SC3_reg_014C_DYN_SCL_BK1F   0x14C

#define	SC3_reg_0150_DYN_SCL_BK1F   0x150

#define	SC3_reg_0158_DYN_SCL_BK1F   0x158

#define	SC3_reg_015C_DYN_SCL_BK1F   0x15C

#define	SC3_reg_0160_DYN_SCL_BK1F   0x160

#define	SC3_reg_0164_DYN_SCL_BK1F   0x164

#define	SC3_reg_0178_DYN_SCL_BK1F   0x178

#define	SC3_reg_017C_DYN_SCL_BK1F   0x17C

#define	SC3_reg_0144_DYN_SCL_BK1F   0x144

#define	SC3_reg_0148_DYN_SCL_BK1F   0x148

#define	SC3_reg_0180_DYN_SCL_BK1F   0x180

#define	SC3_reg_0184_DYN_SCL_BK1F   0x184

#define	SC3_reg_0188_DYN_SCL_BK1F   0x188

#define	SC3_reg_018C_DYN_SCL_BK1F   0x18C

#define	SC3_reg_0190_DYN_SCL_BK1F   0x190

#define	SC3_reg_0194_DYN_SCL_BK1F   0x194

#define	SC3_reg_01A0_DYN_SCL_BK1F   0x1A0

#define	SC3_reg_01BC_DYN_SCL_BK1F   0x1BC

#define	SC3_reg_01C0_DYN_SCL_BK1F   0x1C0

#define	SC3_reg_01C4_DYN_SCL_BK1F   0x1C4

#define	SC3_reg_01CC_DYN_SCL_BK1F   0x1CC

#define	SC3_reg_01D0_DYN_SCL_BK1F   0x1D0

#define	SC3_reg_01D4_DYN_SCL_BK1F   0x1D4

#define	SC3_reg_01D8_DYN_SCL_BK1F   0x1D8

#define	SC3_reg_01DC_DYN_SCL_BK1F   0x1DC

#define	SC3_reg_01E0_DYN_SCL_BK1F   0x1E0

#define	SC3_reg_01E4_DYN_SCL_BK1F   0x1E4

#define	SC3_reg_01E8_DYN_SCL_BK1F   0x1E8

#define	SC3_reg_01EC_DYN_SCL_BK1F   0x1EC

#define	SC3_reg_01F4_DYN_SCL_BK1F   0x1F4

#define	SC3_reg_01F8_DYN_SCL_BK1F   0x1F8

#define	SC3_reg_01FC_DYN_SCL_BK1F   0x1FC

#define	SC3_reg_0218_dyn_scl        0x218

#define	SC3_reg_021C_dyn_scl        0x21C

#define	SC3_reg_0220_dyn_scl        0x220

#define	SC3_reg_0224_dyn_scl        0x224

#define	SC3_reg_0228_dyn_scl        0x228

#define	SC3_reg_022C_dyn_scl        0x22C

#define	SC3_reg_0230_dyn_scl        0x230

#define	SC3_reg_0240_dyn_scl        0x240

#define	SC3_reg_0244_dyn_scl        0x244

#define	SC3_reg_0248_dyn_scl        0x248

#define	SC3_reg_024C_dyn_scl        0x24C

#define	SC3_reg_0250_dyn_scl        0x250

#define	SC3_reg_0254_dyn_scl        0x254

#define	SC3_reg_0260_dyn_scl        0x260

#define	SC3_reg_0264_dyn_scl        0x264

#define	SC3_reg_0268_dyn_scl        0x268

#define	SC3_reg_026C_dyn_scl        0x26C

#define	SC3_reg_0270_dyn_scl        0x270

#define	SC3_reg_0274_dyn_scl        0x274

#define	SC3_reg_0280_dyn_scl        0x280

#define	SC3_reg_0284_dyn_scl        0x284

#define	SC3_reg_0288_dyn_scl        0x288

#define	SC3_reg_028C_dyn_scl        0x28C

#define	SC3_reg_0290_dyn_scl        0x290

#define	SC3_reg_0294_dyn_scl        0x294

#define	SC3_reg_02A0_dyn_scl        0x2A0

#define	SC3_reg_02A4_dyn_scl        0x2A4

#define	SC3_reg_02A8_dyn_scl        0x2A8

#define	SC3_reg_02AC_dyn_scl        0x2AC

#define	SC3_reg_02B0_dyn_scl        0x2B0

#define	SC3_reg_02B4_dyn_scl        0x2B4

#define	SC3_reg_02C8_dyn_scl        0x2C8

#define	SC3_reg_02CC_dyn_scl        0x2CC

#define	SC3_reg_02D0_dyn_scl        0x2D0

#define	SC3_reg_02D4_dyn_scl        0x2D4

#define	SC3_reg_02D8_dyn_scl        0x2D8

#define	SC3_reg_02DC_dyn_scl        0x2DC

#define	SC3_reg_02E0_dyn_scl        0x2E0

#define	SC3_reg_02E4_dyn_scl        0x2E4

#define	SC3_reg_02E8_dyn_scl        0x2E8

#define	SC3_reg_02EC_dyn_scl        0x2EC

#define	SC3_reg_0300_dyn_scl        0x300

#define	SC3_reg_0304_dyn_scl        0x304

#define	SC3_reg_0308_dyn_scl        0x308

#define	SC3_reg_030C_dyn_scl        0x30C

#define	SC3_reg_0310_dyn_scl        0x310

#define	SC3_reg_0314_dyn_scl        0x314

#define	SC3_reg_0318_dyn_scl        0x318

#define	SC3_reg_031C_dyn_scl        0x31C

#define	SC3_reg_0320_dyn_scl        0x320

#define	SC3_reg_0324_dyn_scl        0x324

#define	SC3_reg_0328_dyn_scl        0x328

#define	SC3_reg_032C_dyn_scl        0x32C

#define	SC3_reg_0340_dyn_scl        0x340

#define	SC3_reg_0344_dyn_scl        0x344

#define	SC3_reg_0348_dyn_scl        0x348

#define	SC3_reg_034C_dyn_scl        0x34C

#define	SC3_reg_0350_dyn_scl        0x350

#define	SC3_reg_0354_dyn_scl        0x354

#define	SC3_reg_0358_dyn_scl        0x358

#define	SC3_reg_035C_dyn_scl        0x35C

#define	SC3_reg_0360_dyn_scl        0x360

#define	SC3_reg_0364_dyn_scl        0x364

#define	SC3_reg_0368_dyn_scl        0x368

#define	SC3_reg_036C_dyn_scl        0x36C

#define	SC3_reg_0380_dyn_scl        0x380

#define	SC3_reg_0384_dyn_scl        0x384

#define	SC3_reg_0388_dyn_scl        0x388

#define	SC3_reg_038C_dyn_scl        0x38C

#define	SC3_reg_0390_dyn_scl        0x390

#define	SC3_reg_0394_dyn_scl        0x394

#define	SC3_reg_0398_dyn_scl        0x398

#define	SC3_reg_039C_dyn_scl        0x39C

#define	SC3_reg_03A0_dyn_scl        0x3A0

#define	SC3_reg_03A4_dyn_scl        0x3A4

#define	SC3_reg_03A8_dyn_scl        0x3A8

#define	SC3_reg_03AC_dyn_scl        0x3AC

#define	SC3_reg_03C0_dyn_scl        0x3C0

#define	SC3_reg_03C4_dyn_scl        0x3C4

#define	SC3_reg_03C8_dyn_scl        0x3C8

#define	SC3_reg_03CC_dyn_scl        0x3CC

#define	SC3_reg_03D0_dyn_scl        0x3D0

#define	SC3_reg_03D4_dyn_scl        0x3D4

#define	SC3_reg_03D8_dyn_scl        0x3D8

#define	SC3_reg_03DC_dyn_scl        0x3DC

#define	SC3_reg_03E0_dyn_scl        0x3E0

#define	SC3_reg_03E4_dyn_scl        0x3E4

#define	SC3_reg_03E8_dyn_scl        0x3E8

#define	SC3_reg_03EC_dyn_scl        0x3EC


#endif
