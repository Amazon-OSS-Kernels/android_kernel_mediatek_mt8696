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
#ifndef _AUTODOWNLOAD_HW_H_
#define _AUTODOWNLOAD_HW_H_

#define ADL_DSYS_REG_BASE 0x15013000
#define ADL_MSYS_REG_BASE 0x14013000
#define ADL_CLITA_OFFSET 0x798
#define ADL_CLITB_OFFSET 0x718
#define ADL_CLITC_OFFSET 0x740
#define ADL_CLITD_OFFSET 0x758
#define ADL_CLITE_OFFSET 0x780
#define ADL_CLITJ_OFFSET 0x500

#define ADL_SW_TRIGGER0_EN_OFFSET 0x6E0
#define ADL_SW_TRIGGER0_OFFSET 0x6E4
#define ADL_SW_TRIGGER1_EN_OFFSET 0x6E8
#define ADL_SW_TRIGGER1_OFFSET 0x6EC

#define ADL_CLI1_SW_TRIG_SHIF 0
#define ADL_CLI21_SW_TRIG_SHIF 1
#define ADL_CLI22_SW_TRIG_SHIF 2
#define ADL_CLI23_SW_TRIG_SHIF 3
#define ADL_CLI3_SW_TRIG_SHIF 4
#define ADL_CLI4_SW_TRIG_SHIF 5
#define ADL_CLI5_SW_TRIG_SHIF 6
#define ADL_CLI6_SW_TRIG_SHIF 7
#define ADL_CLI7_SW_TRIG_SHIF 8
#define ADL_CLI8_SW_TRIG_SHIF 9
#define ADL_CLI9_SW_TRIG_SHIF 10
#define ADL_CLIA_SW_TRIG_SHIF 11
#define ADL_CLIB_SW_TRIG_SHIF 12
#define ADL_CLIC_SW_TRIG_SHIF 13
#define ADL_CLID_SW_TRIG_SHIF 14
#define ADL_CLIE_SW_TRIG_SHIF 15
#define ADL_CLIF_SW_TRIG_SHIF 0
#define ADL_CLIG_SW_TRIG_SHIF 1
#define ADL_CLIH_SW_TRIG_SHIF 2
#define ADL_CLII_SW_TRIG_SHIF 3
#define ADL_CLIJ_SW_TRIG_SHIF 4



#define ADL_CLIT_RST_OFFSET 0x7DC
#define ADL_SW_TRG1_EN_OFFSET 0x4E0
#define ADL_SW_TRG1_OFFSET 0x4E4
#define ADL_SW_TRG2_EN_OFFSET 0x4E8
#define ADL_SW_TRG2_OFFSET 0x4EC
#define ADL_WG_OFFSET 0x7C0

#define EN 0x0
#define ADDR0 0x4
#define ADDR1 0x8
#define DEPTH 0xC
#define DMA_LEN 0x10
#define INIT_ADDR 0x14


/*client range
 *A 0x14013798-0x140137ac
 *B 0x14013718-0x1401372c
 *C 0x14013740-0x14013754
 *D 0x14013758-0x1401376c
 *E 0x14013780-0x14013794
 *J 0x14013500-0x14013514
 */

#define SC3_reg_0404_reg_auto_download_crc  0x004

#define SC3_reg_0408_reg_auto_download_crc  0x008

#define SC3_reg_0418_reg_auto_download_crc  0x018

#define SC3_reg_041C_reg_auto_download_crc  0x01C

#define SC3_reg_0500_reg_auto_download_crc  0x100

#define SC3_reg_0504_reg_auto_download_crc  0x104

#define SC3_reg_0508_reg_auto_download_crc  0x108

#define SC3_reg_050C_reg_auto_download_crc  0x10C

#define SC3_reg_0510_reg_auto_download_crc  0x110

#define SC3_reg_0514_reg_auto_download_crc  0x114

#define SC3_reg_05A8_reg_auto_download_crc  0x1A8

#define SC3_reg_05AC_reg_auto_download_crc  0x1AC

#define SC3_reg_05B0_reg_auto_download_crc  0x1B0

#define SC3_reg_05B4_reg_auto_download_crc  0x1B4

#define SC3_reg_05B8_reg_auto_download_crc  0x1B8

#define SC3_reg_05BC_reg_auto_download_crc  0x1BC

#define SC3_reg_05C0_reg_auto_download_crc  0x1C0

#define SC3_reg_05C4_reg_auto_download_crc  0x1C4

#define SC3_reg_05C8_reg_auto_download_crc  0x1C8

#define SC3_reg_0604_ADL_BK67               0x204

#define SC3_reg_0608_ADL_BK67               0x208

#define SC3_reg_060C_ADL_BK67               0x20C

#define SC3_reg_0610_ADL_BK67               0x210

#define SC3_reg_0614_ADL_BK67               0x214

#define SC3_reg_0618_ADL_BK67               0x218

#define SC3_reg_061C_ADL_BK67               0x21C

#define SC3_reg_0620_ADL_BK67               0x220

#define SC3_reg_0624_ADL_BK67               0x224

#define SC3_reg_0628_ADL_BK67               0x228

#define SC3_reg_062C_ADL_BK67               0x22C

#define SC3_reg_0630_ADL_BK67               0x230

#define SC3_reg_0634_ADL_BK67               0x234

#define SC3_reg_0638_ADL_BK67               0x238

#define SC3_reg_063C_ADL_BK67               0x23C

#define SC3_reg_0640_ADL_BK67               0x240

#define SC3_reg_0644_ADL_BK67               0x244

#define SC3_reg_0648_ADL_BK67               0x248

#define SC3_reg_064C_ADL_BK67               0x24C

#define SC3_reg_0650_ADL_BK67               0x250

#define SC3_reg_0654_ADL_BK67               0x254

#define SC3_reg_0658_ADL_BK67               0x258

#define SC3_reg_065C_ADL_BK67               0x25C

#define SC3_reg_0660_ADL_BK67               0x260

#define SC3_reg_0664_ADL_BK67               0x264

#define SC3_reg_0668_ADL_BK67               0x268

#define SC3_reg_066C_ADL_BK67               0x26C

#define SC3_reg_0670_ADL_BK67               0x270

#define SC3_reg_0674_ADL_BK67               0x274

#define SC3_reg_0678_ADL_BK67               0x278

#define SC3_reg_067C_ADL_BK67               0x27C

#define SC3_reg_0680_ADL_BK67               0x280

#define SC3_reg_0684_ADL_BK67               0x284

#define SC3_reg_0688_ADL_BK67               0x288

#define SC3_reg_068C_ADL_BK67               0x28C

#define SC3_reg_0690_ADL_BK67               0x290

#define SC3_reg_0694_ADL_BK67               0x294

#define SC3_reg_0698_ADL_BK67               0x298

#define SC3_reg_069C_ADL_BK67               0x29C

#define SC3_reg_06A0_ADL_BK67               0x2A0

#define SC3_reg_06A4_ADL_BK67               0x2A4

#define SC3_reg_06A8_ADL_BK67               0x2A8

#define SC3_reg_06AC_ADL_BK67               0x2AC

#define SC3_reg_06B0_ADL_BK67               0x2B0

#define SC3_reg_06B4_ADL_BK67               0x2B4

#define SC3_reg_06B8_ADL_BK67               0x2B8

#define SC3_reg_06BC_ADL_BK67               0x2BC

#define SC3_reg_06C8_ADL_BK67               0x2C8

#define SC3_reg_06CC_ADL_BK67               0x2CC

#define SC3_reg_06D0_ADL_BK67               0x2D0

#define SC3_reg_06D4_ADL_BK67               0x2D4

#define SC3_reg_06D8_ADL_BK67               0x2D8

#define SC3_reg_06DC_ADL_BK67               0x2DC

#define SC3_reg_06E0_ADL_BK67               0x2E0

#define SC3_reg_06E4_ADL_BK67               0x2E4

#define SC3_reg_06E8_ADL_BK67               0x2E8

#define SC3_reg_06EC_ADL_BK67               0x2EC

#define SC3_reg_06FC_ADL_BK67               0x2FC

#define SC3_reg_0700_ADL_BK67               0x300

#define SC3_reg_0704_ADL_BK67               0x304

#define SC3_reg_0708_ADL_BK67               0x308

#define SC3_reg_070C_ADL_BK67               0x30C

#define SC3_reg_0710_ADL_BK67               0x310

#define SC3_reg_0714_ADL_BK67               0x314

#define SC3_reg_0718_ADL_BK67               0x318

#define SC3_reg_071C_ADL_BK67               0x31C

#define SC3_reg_0720_ADL_BK67               0x320

#define SC3_reg_0724_ADL_BK67               0x324

#define SC3_reg_0728_ADL_BK67               0x328

#define SC3_reg_072C_ADL_BK67               0x32C

#define SC3_reg_0734_ADL_BK67               0x334

#define SC3_reg_0738_ADL_BK67               0x338

#define SC3_reg_073C_ADL_BK67               0x33C

#define SC3_reg_0740_ADL_BK67               0x340

#define SC3_reg_0744_ADL_BK67               0x344

#define SC3_reg_0748_ADL_BK67               0x348

#define SC3_reg_074C_ADL_BK67               0x34C

#define SC3_reg_0750_ADL_BK67               0x350

#define SC3_reg_0754_ADL_BK67               0x354

#define SC3_reg_0758_ADL_BK67               0x358

#define SC3_reg_075C_ADL_BK67               0x35C

#define SC3_reg_0760_ADL_BK67               0x360

#define SC3_reg_0764_ADL_BK67               0x364

#define SC3_reg_0768_ADL_BK67               0x368

#define SC3_reg_076C_ADL_BK67               0x36C

#define SC3_reg_0780_ADL_BK67               0x380

#define SC3_reg_0784_ADL_BK67               0x384

#define SC3_reg_0788_ADL_BK67               0x388

#define SC3_reg_078C_ADL_BK67               0x38C

#define SC3_reg_0790_ADL_BK67               0x390

#define SC3_reg_0794_ADL_BK67               0x394

#define SC3_reg_0798_ADL_BK67               0x398

#define SC3_reg_079C_ADL_BK67               0x39C

#define SC3_reg_07A0_ADL_BK67               0x3A0

#define SC3_reg_07A4_ADL_BK67               0x3A4

#define SC3_reg_07A8_ADL_BK67               0x3A8

#define SC3_reg_07AC_ADL_BK67               0x3AC

#define SC3_reg_07B8_ADL_BK67               0x3B8

#define SC3_reg_07BC_ADL_BK67               0x3BC

#define SC3_reg_07C0_ADL_BK67               0x3C0

#define SC3_reg_07C4_ADL_BK67               0x3C4

#define SC3_reg_07C8_ADL_BK67               0x3C8

#define SC3_reg_07CC_ADL_BK67               0x3CC

#define SC3_reg_07D0_ADL_BK67               0x3D0

#define SC3_reg_07D4_ADL_BK67               0x3D4

#define SC3_reg_07D8_ADL_BK67               0x3D8

#define SC3_reg_07DC_ADL_BK67               0x3DC

#define SC3_reg_07E0_ADL_BK67               0x3E0

#define SC3_reg_07E4_ADL_BK67               0x3E4

#define SC3_reg_07E8_ADL_BK67               0x3E8

#define SC3_reg_07EC_ADL_BK67               0x3EC

#define SC3_reg_07F0_ADL_BK67               0x3F0

#define SC3_reg_07F4_ADL_BK67               0x3F4

#define SC3_reg_07F8_ADL_BK67               0x3F8

#define SC3_reg_07FC_ADL_BK67               0x3FC

#endif
