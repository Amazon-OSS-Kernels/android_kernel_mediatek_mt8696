/*
 * mt8532-reg.h  --  Mediatek 8532 audio driver reg definition
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

#ifndef _MT8532_REG_H_
#define _MT8532_REG_H_

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/
#define AUDIO_TOP_CON0			0x0000
#define AUDIO_TOP_CON1			0x0004
#define AUDIO_TOP_CON2			0x0008
#define AUDIO_TOP_CON3			0x000c
#define AUDIO_TOP_CON4			0x0010
#define AUDIO_TOP_CON5			0x0014
#define AUDIO_TOP_CON6			0x0018

#define ASMO_TIMING_CON0		0x0100 //not used

#define PWR1_ASM_CON1			0x0108 //diff gasrc cali/asm clk
#define PWR1_ASM_CON2			0x03b0 //diff
#define PWR1_ASM_CON3			0x03b4 //diff
#define PWR1_ASM_CON4			0x03b8 //diff


#define ASYS_IRQ1_CON			0x0114
#define ASYS_IRQ2_CON			0x0118
#define ASYS_IRQ3_CON			0x011c
#define ASYS_IRQ4_CON			0x0120
#define ASYS_IRQ5_CON			0x0124
#define ASYS_IRQ6_CON			0x0128
#define ASYS_IRQ7_CON			0x012c
#define ASYS_IRQ8_CON			0x0130
#define ASYS_IRQ9_CON			0x0134
#define ASYS_IRQ10_CON			0x0138
#define ASYS_IRQ11_CON			0x013c
#define ASYS_IRQ12_CON			0x0140
#define ASYS_IRQ_CLR			0x0154
#define ASYS_IRQ_STATUS			0x0158

#define AFE_IRQ1_CON			0x0164  //spdif out
#define AFE_IRQ2_CON			0x0168  //spdif in detect
//SPDIF IN DATA(Mphone_multi counter mode/direct mode UL1)
#define AFE_IRQ3_CON			0x016c
#define AFE_IRQ4_CON			0x01a0
#define AFE_IRQ5_CON			0x01a4
#define AFE_IRQ6_CON			0x01a8
#define AFE_IRQ7_CON			0x01ac
//Mphone_mulit_spdif_iecdata_dec
#define AFE_IRQ8_CON			0x01b8
//Mphone_multi2 counter mode/direct mode UL6
#define AFE_IRQ9_CON			0x01bc
//Mphone_multi2_spdif_iecdata_dec
#define AFE_IRQ10_CON			0x01c0

#define AFE_IRQ_MCU_CLR			0x0170
#define AFE_IRQ_STATUS			0x0174
#define AFE_IRQ_MASK			0x0178
#define ASYS_IRQ_MASK			0x017c

#define AFE_SINEGEN_CON0		0x01f0
#define AFE_SINEGEN_CON1		0x01f4
#define AFE_SINEGEN_CON2		0x01f8
#define AFE_SINEGEN_CON3		0x01fc

#define AFE_SPDIF_OUT_CON0		0x0380

#define AFE_TDMOUT_CONN0		0x0390
#define AFE_TDMOUT_CONN1		0x0398
#define AFE_TDMOUT_CONN2		0x039c

#define AFE_APLL_TUNER_CFG      0x03f8
#define AFE_APLL_TUNER_CFG1     0x03fc

#define AFE_IEC_CFG			0x0480
#define AFE_IEC_NSNUM			0x0484
#define AFE_IEC_BURST_INFO		0x0488
#define AFE_IEC_BURST_LEN		0x048c
#define AFE_IEC_NSADR			0x0490
#define AFE_IEC_CHL_STAT0		0x04a0
#define AFE_IEC_CHL_STAT1		0x04a4
#define AFE_IEC_CHR_STAT0		0x04a8
#define AFE_IEC_CHR_STAT1		0x04ac

#define AFE_SPDIFIN_CFG0		0x0500
#define AFE_SPDIFIN_CFG1		0x0504
#define AFE_SPDIFIN_CHSTS1		0x0508
#define AFE_SPDIFIN_CHSTS2		0x050c
#define AFE_SPDIFIN_CHSTS3		0x0510
#define AFE_SPDIFIN_CHSTS4		0x0514
#define AFE_SPDIFIN_CHSTS5		0x0518
#define AFE_SPDIFIN_CHSTS6		0x051c
#define AFE_SPDIFIN_DEBUG1		0x0520
#define AFE_SPDIFIN_DEBUG2		0x0524
#define AFE_SPDIFIN_DEBUG3		0x0528
#define AFE_SPDIFIN_DEBUG4		0x052c
#define AFE_SPDIFIN_EC			0x0530
#define AFE_SPDIFIN_CKLOCK_CFG		0x0534
#define AFE_SPDIFIN_BR			0x053c
#define AFE_SPDIFIN_BR_DBG1		0x0540
#define AFE_SPDIFIN_CKFBDIV		0x0544
#define AFE_SPDIFIN_INT_EXT		0x0548
#define AFE_SPDIFIN_INT_EXT2		0x054c
#define SPDIFIN_FREQ_INFO		0x0550
#define SPDIFIN_FREQ_INFO_2		0x0554
#define SPDIFIN_FREQ_INFO_3		0x0558
#define SPDIFIN_FREQ_STATUS		0x055c
#define SPDIFIN_USERCODE1		0x0560
#define SPDIFIN_USERCODE2		0x0564
#define SPDIFIN_USERCODE3		0x0568
#define SPDIFIN_USERCODE4		0x056c
#define SPDIFIN_USERCODE5		0x0570
#define SPDIFIN_USERCODE6		0x0574
#define SPDIFIN_USERCODE7		0x0578
#define SPDIFIN_USERCODE8		0x057c
#define SPDIFIN_USERCODE9		0x0580
#define SPDIFIN_USERCODE10		0x0584
#define SPDIFIN_USERCODE11		0x0588
#define SPDIFIN_USERCODE12		0x058c
#define AFE_SPDIFIN_APLL_TUNER_CFG	0x0594
#define AFE_SPDIFIN_APLL_TUNER_CFG1	0x0598

#define ASYS_TOP_CON			0x0600

#define AFE_LINEIN_APLL_TUNER_CFG		0x0610
#define AFE_LINEIN_APLL_TUNER_MOI		0x0614
#define AFE_EARC_APLL_TUNER_CFG		0x0618
#define AFE_EARC_APLL_TUNER_MON		0x061c



#define PWR2_TOP_CON0			0x0634
#define PWR2_TOP_CON1			0x0638

#define PCM_INTF_CON1			0x063c
#define PCM_INTF_CON2			0x0640

#define AFE_CM0_CON		0x0660
#define AFE_CM1_CON		0x0664

#define AFE_MPHONE_MULTI_CON0		0x06a4
#define AFE_MPHONE_MULTI_CON1		0x06a8
#define AFE_MPHONE_MULTI_CON2		0x06ac
#define AFE_MPHONE_MULTI_MON		0x06b0
#define AFE_MPHONE_MULTI_DET_REG_CON0		0x06b4
#define AFE_MPHONE_MULTI_DET_REG_CON1		0x06b8
#define AFE_MPHONE_MULTI_DET_REG_CON2		0x06bc
#define AFE_MPHONE_MULTI_DET_REG_CON3		0x06c0
#define AFE_MPHONE_MULTI_DET_MON0		0x06c4
#define AFE_MPHONE_MULTI_DET_MON1		0x06c8
#define AFE_MPHONE_MULTI_DET_MON2		0x06d0

#define AFE_MPHONE_MULTI2_CON0		0x06d4
#define AFE_MPHONE_MULTI2_CON1		0x06d8
#define AFE_MPHONE_MULTI2_CON2		0x06dc
#define AFE_MPHONE_MULTI2_MON		0x06e0
#define AFE_MPHONE_MULTI2_DET_REG_CON0		0x06e4
#define AFE_MPHONE_MULTI2_DET_REG_CON1		0x06e8
#define AFE_MPHONE_MULTI2_DET_REG_CON2		0x06ec
#define AFE_MPHONE_MULTI2_DET_REG_CON3		0x06f0
#define AFE_MPHONE_MULTI2_DET_MON0		0x06f4
#define AFE_MPHONE_MULTI2_DET_MON1		0x06f8
#define AFE_MPHONE_MULTI2_DET_MON2		0x06fc


#define AFE_CONN0                0x3000
#define AFE_CONN0_1              0x3004
#define AFE_CONN0_2              0x3008
#define AFE_CONN0_3              0x300c
#define AFE_CONN0_4              0x3010
#define AFE_CONN1                0x3014
#define AFE_CONN1_1              0x3018
#define AFE_CONN1_2              0x301c
#define AFE_CONN1_3              0x3020
#define AFE_CONN1_4              0x3024
#define AFE_CONN2                0x3028
#define AFE_CONN2_1              0x302c
#define AFE_CONN2_2              0x3030
#define AFE_CONN2_3              0x3034
#define AFE_CONN2_4              0x3038
#define AFE_CONN3                0x303c
#define AFE_CONN3_1              0x3040
#define AFE_CONN3_2              0x3044
#define AFE_CONN3_3              0x3048
#define AFE_CONN3_4              0x304c
#define AFE_CONN4                0x3050
#define AFE_CONN4_1              0x3054
#define AFE_CONN4_2              0x3058
#define AFE_CONN4_3              0x305c
#define AFE_CONN4_4              0x3060
#define AFE_CONN5                0x3064
#define AFE_CONN5_1              0x3068
#define AFE_CONN5_2              0x306c
#define AFE_CONN5_3              0x3070
#define AFE_CONN5_4              0x3074
#define AFE_CONN6                0x3078
#define AFE_CONN6_1              0x307c
#define AFE_CONN6_2              0x3080
#define AFE_CONN6_3              0x3084
#define AFE_CONN6_4              0x3088
#define AFE_CONN7                0x308c
#define AFE_CONN7_1              0x3090
#define AFE_CONN7_2              0x3094
#define AFE_CONN7_3              0x3098
#define AFE_CONN7_4              0x309c
#define AFE_CONN8                0x30a0
#define AFE_CONN8_1              0x30a4
#define AFE_CONN8_2              0x30a8
#define AFE_CONN8_3              0x30ac
#define AFE_CONN8_4              0x30b0
#define AFE_CONN9                0x30b4
#define AFE_CONN9_1              0x30b8
#define AFE_CONN9_2              0x30bc
#define AFE_CONN9_3              0x30c0
#define AFE_CONN9_4              0x30c4
#define AFE_CONN10               0x30c8
#define AFE_CONN10_1             0x30cc
#define AFE_CONN10_2             0x30d0
#define AFE_CONN10_3             0x30d4
#define AFE_CONN10_4             0x30d8
#define AFE_CONN11               0x30dc
#define AFE_CONN11_1             0x30e0
#define AFE_CONN11_2             0x30e4
#define AFE_CONN11_3             0x30e8
#define AFE_CONN11_4             0x30ec
#define AFE_CONN12               0x30f0
#define AFE_CONN12_1             0x30f4
#define AFE_CONN12_2             0x30f8
#define AFE_CONN12_3             0x30fc
#define AFE_CONN12_4             0x3100
#define AFE_CONN13               0x3104
#define AFE_CONN13_1             0x3108
#define AFE_CONN13_2             0x310c
#define AFE_CONN13_3             0x3110
#define AFE_CONN13_4             0x3114
#define AFE_CONN14               0x3118
#define AFE_CONN14_1             0x311c
#define AFE_CONN14_2             0x3120
#define AFE_CONN14_3             0x3124
#define AFE_CONN14_4             0x3128
#define AFE_CONN15               0x312c
#define AFE_CONN15_1             0x3130
#define AFE_CONN15_2             0x3134
#define AFE_CONN15_3             0x3138
#define AFE_CONN15_4             0x313c
#define AFE_CONN16               0x3140
#define AFE_CONN16_1             0x3144
#define AFE_CONN16_2             0x3148
#define AFE_CONN16_3             0x314c
#define AFE_CONN16_4             0x3150
#define AFE_CONN17               0x3154
#define AFE_CONN17_1             0x3158
#define AFE_CONN17_2             0x315c
#define AFE_CONN17_3             0x3160
#define AFE_CONN17_4             0x3164
#define AFE_CONN18               0x3168
#define AFE_CONN18_1             0x316c
#define AFE_CONN18_2             0x3170
#define AFE_CONN18_3             0x3174
#define AFE_CONN18_4             0x3178
#define AFE_CONN19               0x317c
#define AFE_CONN19_1             0x3180
#define AFE_CONN19_2             0x3184
#define AFE_CONN19_3             0x3188
#define AFE_CONN19_4             0x318c
#define AFE_CONN20               0x3190
#define AFE_CONN20_1             0x3194
#define AFE_CONN20_2             0x3198
#define AFE_CONN20_3             0x319c
#define AFE_CONN20_4             0x31a0
#define AFE_CONN21               0x31a4
#define AFE_CONN21_1             0x31a8
#define AFE_CONN21_2             0x31ac
#define AFE_CONN21_3             0x31b0
#define AFE_CONN21_4             0x31b4
#define AFE_CONN22               0x31b8
#define AFE_CONN22_1             0x31bc
#define AFE_CONN22_2             0x31c0
#define AFE_CONN22_3             0x31c4
#define AFE_CONN22_4             0x31c8
#define AFE_CONN23               0x31cc
#define AFE_CONN23_1             0x31d0
#define AFE_CONN23_2             0x31d4
#define AFE_CONN23_3             0x31d8
#define AFE_CONN23_4             0x31dc
#define AFE_CONN24               0x31e0
#define AFE_CONN24_1             0x31e4
#define AFE_CONN24_2             0x31e8
#define AFE_CONN24_3             0x31ec
#define AFE_CONN24_4             0x31f0
#define AFE_CONN25               0x31f4
#define AFE_CONN25_1             0x31f8
#define AFE_CONN25_2             0x31fc
#define AFE_CONN25_3             0x3200
#define AFE_CONN25_4             0x3204
#define AFE_CONN26               0x3208
#define AFE_CONN26_1             0x320c
#define AFE_CONN26_2             0x3210
#define AFE_CONN26_3             0x3214
#define AFE_CONN26_4             0x3218
#define AFE_CONN27               0x321c
#define AFE_CONN27_1             0x3220
#define AFE_CONN27_2             0x3224
#define AFE_CONN27_3             0x3228
#define AFE_CONN27_4             0x322c
#define AFE_CONN28               0x3230
#define AFE_CONN28_1             0x3234
#define AFE_CONN28_2             0x3238
#define AFE_CONN28_3             0x323c
#define AFE_CONN28_4             0x3240
#define AFE_CONN29               0x3244
#define AFE_CONN29_1             0x3248
#define AFE_CONN29_2             0x324c
#define AFE_CONN29_3             0x3250
#define AFE_CONN29_4             0x3254
#define AFE_CONN30               0x3258
#define AFE_CONN30_1             0x325c
#define AFE_CONN30_2             0x3260
#define AFE_CONN30_3             0x3264
#define AFE_CONN30_4             0x3268
#define AFE_CONN31               0x326c
#define AFE_CONN31_1             0x3270
#define AFE_CONN31_2             0x3274
#define AFE_CONN31_3             0x3278
#define AFE_CONN31_4             0x327c
#define AFE_CONN32               0x3280
#define AFE_CONN32_1             0x3284
#define AFE_CONN32_2             0x3288
#define AFE_CONN32_3             0x328c
#define AFE_CONN32_4             0x3290
#define AFE_CONN33               0x3294
#define AFE_CONN33_1             0x3298
#define AFE_CONN33_2             0x329c
#define AFE_CONN33_3             0x32a0
#define AFE_CONN33_4             0x32a4
#define AFE_CONN34               0x32a8
#define AFE_CONN34_1             0x32ac
#define AFE_CONN34_2             0x32b0
#define AFE_CONN34_3             0x32b4
#define AFE_CONN34_4             0x32b8
#define AFE_CONN35               0x32bc
#define AFE_CONN35_1             0x32c0
#define AFE_CONN35_2             0x32c4
#define AFE_CONN35_3             0x32c8
#define AFE_CONN35_4             0x32cc
#define AFE_CONN36               0x32d0
#define AFE_CONN36_1             0x32d4
#define AFE_CONN36_2             0x32d8
#define AFE_CONN36_3             0x32dc
#define AFE_CONN36_4             0x32e0
#define AFE_CONN37               0x32e4
#define AFE_CONN37_1             0x32e8
#define AFE_CONN37_2             0x32ec
#define AFE_CONN37_3             0x32f0
#define AFE_CONN37_4             0x32f4
#define AFE_CONN38               0x32f8
#define AFE_CONN38_1             0x32fc
#define AFE_CONN38_2             0x3300
#define AFE_CONN38_3             0x3304
#define AFE_CONN38_4             0x3308
#define AFE_CONN39               0x330c
#define AFE_CONN39_1             0x3310
#define AFE_CONN39_2             0x3314
#define AFE_CONN39_3             0x3318
#define AFE_CONN39_4             0x331c
#define AFE_CONN40               0x3320
#define AFE_CONN40_1             0x3324
#define AFE_CONN40_2             0x3328
#define AFE_CONN40_3             0x332c
#define AFE_CONN40_4             0x3330
#define AFE_CONN41               0x3334
#define AFE_CONN41_1             0x3338
#define AFE_CONN41_2             0x333c
#define AFE_CONN41_3             0x3340
#define AFE_CONN41_4             0x3344
#define AFE_CONN42               0x3348
#define AFE_CONN42_1             0x334c
#define AFE_CONN42_2             0x3350
#define AFE_CONN42_3             0x3354
#define AFE_CONN42_4             0x3358
#define AFE_CONN43               0x335c
#define AFE_CONN43_1             0x3360
#define AFE_CONN43_2             0x3364
#define AFE_CONN43_3             0x3368
#define AFE_CONN43_4             0x336c
#define AFE_CONN44               0x3370
#define AFE_CONN44_1             0x3374
#define AFE_CONN44_2             0x3378
#define AFE_CONN44_3             0x337c
#define AFE_CONN44_4             0x3380
#define AFE_CONN45               0x3384
#define AFE_CONN45_1             0x3388
#define AFE_CONN45_2             0x338c
#define AFE_CONN45_3             0x3390
#define AFE_CONN45_4             0x3394
#define AFE_CONN46               0x3398
#define AFE_CONN46_1             0x339c
#define AFE_CONN46_2             0x33a0
#define AFE_CONN46_3             0x33a4
#define AFE_CONN46_4             0x33a8
#define AFE_CONN47               0x33ac
#define AFE_CONN47_1             0x33b0
#define AFE_CONN47_2             0x33b4
#define AFE_CONN47_3             0x33b8
#define AFE_CONN47_4             0x33bc
#define AFE_CONN48               0x33c0
#define AFE_CONN48_1             0x33c4
#define AFE_CONN48_2             0x33c8
#define AFE_CONN48_3             0x33cc
#define AFE_CONN48_4             0x33d0
#define AFE_CONN49               0x33d4
#define AFE_CONN49_1             0x33d8
#define AFE_CONN49_2             0x33dc
#define AFE_CONN49_3             0x33e0
#define AFE_CONN49_4             0x33e4
#define AFE_CONN50               0x33e8
#define AFE_CONN50_1             0x33ec
#define AFE_CONN50_2             0x33f0
#define AFE_CONN50_3             0x33f4
#define AFE_CONN50_4             0x33f8
#define AFE_CONN51               0x33fc
#define AFE_CONN51_1             0x3400
#define AFE_CONN51_2             0x3404
#define AFE_CONN51_3             0x3408
#define AFE_CONN51_4             0x340c
#define AFE_CONN52               0x3410
#define AFE_CONN52_1             0x3414
#define AFE_CONN52_2             0x3418
#define AFE_CONN52_3             0x341c
#define AFE_CONN52_4             0x3420
#define AFE_CONN53               0x3424
#define AFE_CONN53_1             0x3428
#define AFE_CONN53_2             0x342c
#define AFE_CONN53_3             0x3430
#define AFE_CONN53_4             0x3434
#define AFE_CONN54               0x3438
#define AFE_CONN54_1             0x343c
#define AFE_CONN54_2             0x3440
#define AFE_CONN54_3             0x3444
#define AFE_CONN54_4             0x3448
#define AFE_CONN55               0x344c
#define AFE_CONN55_1             0x3450
#define AFE_CONN55_2             0x3454
#define AFE_CONN55_3             0x3458
#define AFE_CONN55_4             0x345c
#define AFE_CONN56               0x3460
#define AFE_CONN56_1             0x3464
#define AFE_CONN56_2             0x3468
#define AFE_CONN56_3             0x346c
#define AFE_CONN56_4             0x3470
#define AFE_CONN57               0x3474
#define AFE_CONN57_1             0x3478
#define AFE_CONN57_2             0x347c
#define AFE_CONN57_3             0x3480
#define AFE_CONN57_4             0x3484
#define AFE_CONN58               0x3488
#define AFE_CONN58_1             0x348c
#define AFE_CONN58_2             0x3490
#define AFE_CONN58_3             0x3494
#define AFE_CONN58_4             0x3498
#define AFE_CONN59               0x349c
#define AFE_CONN59_1             0x34a0
#define AFE_CONN59_2             0x34a4
#define AFE_CONN59_3             0x34a8
#define AFE_CONN59_4             0x34ac
#define AFE_CONN60               0x34b0
#define AFE_CONN60_1             0x34b4
#define AFE_CONN60_2             0x34b8
#define AFE_CONN60_3             0x34bc
#define AFE_CONN60_4             0x34c0
#define AFE_CONN61               0x34c4
#define AFE_CONN61_1             0x34c8
#define AFE_CONN61_2             0x34cc
#define AFE_CONN61_3             0x34d0
#define AFE_CONN61_4             0x34d4
#define AFE_CONN62               0x34d8
#define AFE_CONN62_1             0x34dc
#define AFE_CONN62_2             0x34e0
#define AFE_CONN62_3             0x34e4
#define AFE_CONN62_4             0x34e8
#define AFE_CONN63               0x34ec
#define AFE_CONN63_1             0x34f0
#define AFE_CONN63_2             0x34f4
#define AFE_CONN63_3             0x34f8
#define AFE_CONN63_4             0x34fc
#define AFE_CONN64               0x3500
#define AFE_CONN64_1             0x3504
#define AFE_CONN64_2             0x3508
#define AFE_CONN64_3             0x350c
#define AFE_CONN64_4             0x3510
#define AFE_CONN65               0x3514
#define AFE_CONN65_1             0x3518
#define AFE_CONN65_2             0x351c
#define AFE_CONN65_3             0x3520
#define AFE_CONN65_4             0x3524
#define AFE_CONN66               0x3528
#define AFE_CONN66_1             0x352c
#define AFE_CONN66_2             0x3530
#define AFE_CONN66_3             0x3534
#define AFE_CONN66_4             0x3538
#define AFE_CONN67               0x353c
#define AFE_CONN67_1             0x3540
#define AFE_CONN67_2             0x3544
#define AFE_CONN67_3             0x3548
#define AFE_CONN67_4             0x354c
#define AFE_CONN68               0x3550
#define AFE_CONN68_1             0x3554
#define AFE_CONN68_2             0x3558
#define AFE_CONN68_3             0x355c
#define AFE_CONN68_4             0x3560
#define AFE_CONN69               0x3564
#define AFE_CONN69_1             0x3568
#define AFE_CONN69_2             0x356c
#define AFE_CONN69_3             0x3570
#define AFE_CONN69_4             0x3574
#define AFE_CONN70               0x3578
#define AFE_CONN70_1             0x357c
#define AFE_CONN70_2             0x3580
#define AFE_CONN70_3             0x3584
#define AFE_CONN70_4             0x3588
#define AFE_CONN71               0x358c
#define AFE_CONN71_1             0x3590
#define AFE_CONN71_2             0x3594
#define AFE_CONN71_3             0x3598
#define AFE_CONN71_4             0x359c
#define AFE_CONN72               0x35a0
#define AFE_CONN72_1             0x35a4
#define AFE_CONN72_2             0x35a8
#define AFE_CONN72_3             0x35ac
#define AFE_CONN72_4             0x35b0
#define AFE_CONN73               0x35b4
#define AFE_CONN73_1             0x35b8
#define AFE_CONN73_2             0x35bc
#define AFE_CONN73_3             0x35c0
#define AFE_CONN73_4             0x35c4
#define AFE_CONN74               0x35c8
#define AFE_CONN74_1             0x35cc
#define AFE_CONN74_2             0x35d0
#define AFE_CONN74_3             0x35d4
#define AFE_CONN74_4             0x35d8
#define AFE_CONN75               0x35dc
#define AFE_CONN75_1             0x35e0
#define AFE_CONN75_2             0x35e4
#define AFE_CONN75_3             0x35e8
#define AFE_CONN75_4             0x35ec
#define AFE_CONN76               0x35f0
#define AFE_CONN76_1             0x35f4
#define AFE_CONN76_2             0x35f8
#define AFE_CONN76_3             0x35fc
#define AFE_CONN76_4             0x3600
#define AFE_CONN77               0x3604
#define AFE_CONN77_1             0x3608
#define AFE_CONN77_2             0x360c
#define AFE_CONN77_3             0x3610
#define AFE_CONN77_4             0x3614
#define AFE_CONN78               0x3618
#define AFE_CONN78_1             0x361c
#define AFE_CONN78_2             0x3620
#define AFE_CONN78_3             0x3624
#define AFE_CONN78_4             0x3628
#define AFE_CONN79               0x362c
#define AFE_CONN79_1             0x3630
#define AFE_CONN79_2             0x3634
#define AFE_CONN79_3             0x3638
#define AFE_CONN79_4             0x363c
#define AFE_CONN80               0x3640
#define AFE_CONN80_1             0x3644
#define AFE_CONN80_2             0x3648
#define AFE_CONN80_3             0x364c
#define AFE_CONN80_4             0x3650
#define AFE_CONN81               0x3654
#define AFE_CONN81_1             0x3658
#define AFE_CONN81_2             0x365c
#define AFE_CONN81_3             0x3660
#define AFE_CONN81_4             0x3664
#define AFE_CONN82               0x3668
#define AFE_CONN82_1             0x366c
#define AFE_CONN82_2             0x3670
#define AFE_CONN82_3             0x3674
#define AFE_CONN82_4             0x3678
#define AFE_CONN83               0x367c
#define AFE_CONN83_1             0x3680
#define AFE_CONN83_2             0x3684
#define AFE_CONN83_3             0x3688
#define AFE_CONN83_4             0x368c
#define AFE_CONN84               0x3690
#define AFE_CONN84_1             0x3694
#define AFE_CONN84_2             0x3698
#define AFE_CONN84_3             0x369c
#define AFE_CONN84_4             0x36a0
#define AFE_CONN85               0x36a4
#define AFE_CONN85_1             0x36a8
#define AFE_CONN85_2             0x36ac
#define AFE_CONN85_3             0x36b0
#define AFE_CONN85_4             0x36b4
#define AFE_CONN86               0x36b8
#define AFE_CONN86_1             0x36bc
#define AFE_CONN86_2             0x36c0
#define AFE_CONN86_3             0x36c4
#define AFE_CONN86_4             0x36c8
#define AFE_CONN87               0x36cc
#define AFE_CONN87_1             0x36d0
#define AFE_CONN87_2             0x36d4
#define AFE_CONN87_3             0x36d8
#define AFE_CONN87_4             0x36dc
#define AFE_CONN88               0x36e0
#define AFE_CONN88_1             0x36e4
#define AFE_CONN88_2             0x36e8
#define AFE_CONN88_3             0x36ec
#define AFE_CONN88_4             0x36f0
#define AFE_CONN89               0x36f4
#define AFE_CONN89_1             0x36f8
#define AFE_CONN89_2             0x36fc
#define AFE_CONN89_3             0x3700
#define AFE_CONN89_4             0x3704
#define AFE_CONN90               0x3708
#define AFE_CONN90_1             0x370c
#define AFE_CONN90_2             0x3710
#define AFE_CONN90_3             0x3714
#define AFE_CONN90_4             0x3718
#define AFE_CONN91               0x371c
#define AFE_CONN91_1             0x3720
#define AFE_CONN91_2             0x3724
#define AFE_CONN91_3             0x3728
#define AFE_CONN91_4             0x372c
#define AFE_CONN92               0x3730
#define AFE_CONN92_1             0x3734
#define AFE_CONN92_2             0x3738
#define AFE_CONN92_3             0x373c
#define AFE_CONN92_4             0x3740
#define AFE_CONN93               0x3744
#define AFE_CONN93_1             0x3748
#define AFE_CONN93_2             0x374c
#define AFE_CONN93_3             0x3750
#define AFE_CONN93_4             0x3754
#define AFE_CONN94               0x3758
#define AFE_CONN94_1             0x375c
#define AFE_CONN94_2             0x3760
#define AFE_CONN94_3             0x3764
#define AFE_CONN94_4             0x3768
#define AFE_CONN95               0x376c
#define AFE_CONN95_1             0x3770
#define AFE_CONN95_2             0x3774
#define AFE_CONN95_3             0x3778
#define AFE_CONN95_4             0x377c
#define AFE_CONN96               0x3780
#define AFE_CONN96_1             0x3784
#define AFE_CONN96_2             0x3788
#define AFE_CONN96_3             0x378c
#define AFE_CONN96_4             0x3790
#define AFE_CONN97               0x3794
#define AFE_CONN97_1             0x3798
#define AFE_CONN97_2             0x379c
#define AFE_CONN97_3             0x37a0
#define AFE_CONN97_4             0x37a4
#define AFE_CONN98               0x37a8
#define AFE_CONN98_1             0x37ac
#define AFE_CONN98_2             0x37b0
#define AFE_CONN98_3             0x37b4
#define AFE_CONN98_4             0x37b8
#define AFE_CONN99               0x37bc
#define AFE_CONN99_1             0x37c0
#define AFE_CONN99_2             0x37c4
#define AFE_CONN99_3             0x37c8
#define AFE_CONN99_4             0x37cc
#define AFE_CONN100              0x37d0
#define AFE_CONN100_1            0x37d4
#define AFE_CONN100_2            0x37d8
#define AFE_CONN100_3            0x37dc
#define AFE_CONN100_4            0x37e0
#define AFE_CONN101              0x37e4
#define AFE_CONN101_1            0x37e8
#define AFE_CONN101_2            0x37ec
#define AFE_CONN101_3            0x37f0
#define AFE_CONN101_4            0x37f4
#define AFE_CONN102              0x37f8
#define AFE_CONN102_1            0x37fc
#define AFE_CONN102_2            0x3800
#define AFE_CONN102_3            0x3804
#define AFE_CONN102_4            0x3808
#define AFE_CONN103              0x380c
#define AFE_CONN103_1            0x3810
#define AFE_CONN103_2            0x3814
#define AFE_CONN103_3            0x3818
#define AFE_CONN103_4            0x381c
#define AFE_CONN104              0x3820
#define AFE_CONN104_1            0x3824
#define AFE_CONN104_2            0x3828
#define AFE_CONN104_3            0x382c
#define AFE_CONN104_4            0x3830
#define AFE_CONN105              0x3834
#define AFE_CONN105_1            0x3838
#define AFE_CONN105_2            0x383c
#define AFE_CONN105_3            0x3840
#define AFE_CONN105_4            0x3844
#define AFE_CONN106              0x3848
#define AFE_CONN106_1            0x384c
#define AFE_CONN106_2            0x3850
#define AFE_CONN106_3            0x3854
#define AFE_CONN106_4            0x3858
#define AFE_CONN107              0x385c
#define AFE_CONN107_1            0x3860
#define AFE_CONN107_2            0x3864
#define AFE_CONN107_3            0x3868
#define AFE_CONN107_4            0x386c
#define AFE_CONN108              0x3870
#define AFE_CONN108_1            0x3874
#define AFE_CONN108_2            0x3878
#define AFE_CONN108_3            0x387c
#define AFE_CONN108_4            0x3880
#define AFE_CONN109              0x3884
#define AFE_CONN109_1            0x3888
#define AFE_CONN109_2            0x388c
#define AFE_CONN109_3            0x3890
#define AFE_CONN109_4            0x3894
#define AFE_CONN110              0x3898
#define AFE_CONN110_1            0x389c
#define AFE_CONN110_2            0x38a0
#define AFE_CONN110_3            0x38a4
#define AFE_CONN110_4            0x38a8
#define AFE_CONN111              0x38ac
#define AFE_CONN111_1            0x38b0
#define AFE_CONN111_2            0x38b4
#define AFE_CONN111_3            0x38b8
#define AFE_CONN111_4            0x38bc
#define AFE_CONN112              0x38c0
#define AFE_CONN112_1            0x38c4
#define AFE_CONN112_2            0x38c8
#define AFE_CONN112_3            0x38cc
#define AFE_CONN112_4            0x38d0
#define AFE_CONN113              0x38d4
#define AFE_CONN113_1            0x38d8
#define AFE_CONN113_2            0x38dc
#define AFE_CONN113_3            0x38e0
#define AFE_CONN113_4            0x38e4
#define AFE_CONN114              0x38e8
#define AFE_CONN114_1            0x38ec
#define AFE_CONN114_2            0x38f0
#define AFE_CONN114_3            0x38f4
#define AFE_CONN114_4            0x38f8
#define AFE_CONN115              0x38fc
#define AFE_CONN115_1            0x3900
#define AFE_CONN115_2            0x3904
#define AFE_CONN115_3            0x3908
#define AFE_CONN115_4            0x390c
#define AFE_CONN116              0x3910
#define AFE_CONN116_1            0x3914
#define AFE_CONN116_2            0x3918
#define AFE_CONN116_3            0x391c
#define AFE_CONN116_4            0x3920
#define AFE_CONN117              0x3924
#define AFE_CONN117_1            0x3928
#define AFE_CONN117_2            0x392c
#define AFE_CONN117_3            0x3930
#define AFE_CONN117_4            0x3934
#define AFE_CONN118              0x3938
#define AFE_CONN118_1            0x393c
#define AFE_CONN118_2            0x3940
#define AFE_CONN118_3            0x3944
#define AFE_CONN118_4            0x3948
#define AFE_CONN119              0x394c
#define AFE_CONN119_1            0x3950
#define AFE_CONN119_2            0x3954
#define AFE_CONN119_3            0x3958
#define AFE_CONN119_4            0x395c
#define AFE_CONN120              0x3960
#define AFE_CONN120_1            0x3964
#define AFE_CONN120_2            0x3968
#define AFE_CONN120_3            0x396c
#define AFE_CONN120_4            0x3970
#define AFE_CONN121              0x3974
#define AFE_CONN121_1            0x3978
#define AFE_CONN121_2            0x397c
#define AFE_CONN121_3            0x3980
#define AFE_CONN121_4            0x3984
#define AFE_CONN122              0x3988
#define AFE_CONN122_1            0x398c
#define AFE_CONN122_2            0x3990
#define AFE_CONN122_3            0x3994
#define AFE_CONN122_4            0x3998
#define AFE_CONN123              0x399c
#define AFE_CONN123_1            0x39a0
#define AFE_CONN123_2            0x39a4
#define AFE_CONN123_3            0x39a8
#define AFE_CONN123_4            0x39ac
#define AFE_CONN124              0x39b0
#define AFE_CONN124_1            0x39b4
#define AFE_CONN124_2            0x39b8
#define AFE_CONN124_3            0x39bc
#define AFE_CONN124_4            0x39c0
#define AFE_CONN125              0x39c4
#define AFE_CONN125_1            0x39c8
#define AFE_CONN125_2            0x39cc
#define AFE_CONN125_3            0x39d0
#define AFE_CONN125_4            0x39d4
#define AFE_CONN126              0x39d8
#define AFE_CONN126_1            0x39dc
#define AFE_CONN126_2            0x39e0
#define AFE_CONN126_3            0x39e4
#define AFE_CONN126_4            0x39e8
#define AFE_CONN127              0x39ec
#define AFE_CONN127_1            0x39f0
#define AFE_CONN127_2            0x39f4
#define AFE_CONN127_3            0x39f8
#define AFE_CONN127_4            0x39fc
#define AFE_CONN128              0x3a00
#define AFE_CONN128_1            0x3a04
#define AFE_CONN128_2            0x3a08
#define AFE_CONN128_3            0x3a0c
#define AFE_CONN128_4            0x3a10
#define AFE_CONN129              0x3a14
#define AFE_CONN129_1            0x3a18
#define AFE_CONN129_2            0x3a1c
#define AFE_CONN129_3            0x3a20
#define AFE_CONN129_4            0x3a24
#define AFE_CONN130              0x3a28
#define AFE_CONN130_1            0x3a2c
#define AFE_CONN130_2            0x3a30
#define AFE_CONN130_3            0x3a34
#define AFE_CONN130_4            0x3a38
#define AFE_CONN131              0x3a3c
#define AFE_CONN131_1            0x3a40
#define AFE_CONN131_2            0x3a44
#define AFE_CONN131_3            0x3a48
#define AFE_CONN131_4            0x3a4c
#define AFE_CONN132              0x3a50
#define AFE_CONN132_1            0x3a54
#define AFE_CONN132_2            0x3a58
#define AFE_CONN132_3            0x3a5c
#define AFE_CONN132_4            0x3a60
#define AFE_CONN133              0x3a64
#define AFE_CONN133_1            0x3a68
#define AFE_CONN133_2            0x3a6c
#define AFE_CONN133_3            0x3a70
#define AFE_CONN133_4            0x3a74
#define AFE_CONN134              0x3a78
#define AFE_CONN134_1            0x3a7c
#define AFE_CONN134_2            0x3a80
#define AFE_CONN134_3            0x3a84
#define AFE_CONN134_4            0x3a88
#define AFE_CONN135              0x3a8c
#define AFE_CONN135_1            0x3a90
#define AFE_CONN135_2            0x3a94
#define AFE_CONN135_3            0x3a98
#define AFE_CONN135_4            0x3a9c
#define AFE_CONN136              0x3aa0
#define AFE_CONN136_1            0x3aa4
#define AFE_CONN136_2            0x3aa8
#define AFE_CONN136_3            0x3aac
#define AFE_CONN136_4            0x3ab0
#define AFE_CONN137              0x3ab4
#define AFE_CONN137_1            0x3ab8
#define AFE_CONN137_2            0x3abc
#define AFE_CONN137_3            0x3ac0
#define AFE_CONN137_4            0x3ac4
#define AFE_CONN138              0x3ac8
#define AFE_CONN138_1            0x3acc
#define AFE_CONN138_2            0x3ad0
#define AFE_CONN138_3            0x3ad4
#define AFE_CONN138_4            0x3ad8
#define AFE_CONN139              0x3adc
#define AFE_CONN139_1            0x3ae0
#define AFE_CONN139_2            0x3ae4
#define AFE_CONN139_3            0x3ae8
#define AFE_CONN139_4            0x3aec
#define AFE_CONN_RS              0x3af0
#define AFE_CONN_RS_1            0x3af4
#define AFE_CONN_RS_2            0x3af8
#define AFE_CONN_RS_3            0x3afc
#define AFE_CONN_RS_4            0x3b00
#define AFE_CONN_16BIT           0x3b04
#define AFE_CONN_16BIT_1         0x3b08
#define AFE_CONN_16BIT_2         0x3b0c
#define AFE_CONN_16BIT_3         0x3b10
#define AFE_CONN_16BIT_4         0x3b14
#define AFE_CONN_24BIT           0x3b18
#define AFE_CONN_24BIT_1         0x3b1c
#define AFE_CONN_24BIT_2         0x3b20
#define AFE_CONN_24BIT_3         0x3b24
#define AFE_CONN_24BIT_4         0x3b28


#define AFE_GASRC0_NEW_CON0               0x4c40
#define AFE_GASRC0_NEW_CON1               0x4c44
#define AFE_GASRC0_NEW_CON2               0x4c48
#define AFE_GASRC0_NEW_CON3               0x4c4c
#define AFE_GASRC0_NEW_CON4               0x4c50
#define AFE_GASRC0_NEW_CON5               0x4c54
#define AFE_GASRC0_NEW_CON6               0x4c58
#define AFE_GASRC0_NEW_CON7               0x4c5c
#define AFE_GASRC0_NEW_CON8               0x4c60
#define AFE_GASRC0_NEW_CON9               0x4c64
#define AFE_GASRC0_NEW_CON10              0x4c68
#define AFE_GASRC0_NEW_CON11              0x4c6c
#define AFE_GASRC0_NEW_CON12              0x4c70
#define AFE_GASRC0_NEW_CON13              0x4c74
#define AFE_GASRC0_NEW_CON14              0x4c78
#define AFE_GASRC1_NEW_CON0               0x4c80
#define AFE_GASRC1_NEW_CON1               0x4c84
#define AFE_GASRC1_NEW_CON2               0x4c88
#define AFE_GASRC1_NEW_CON3               0x4c8c
#define AFE_GASRC1_NEW_CON4               0x4c90
#define AFE_GASRC1_NEW_CON5               0x4c94
#define AFE_GASRC1_NEW_CON6               0x4c98
#define AFE_GASRC1_NEW_CON7               0x4c9c
#define AFE_GASRC1_NEW_CON8               0x4ca0
#define AFE_GASRC1_NEW_CON9               0x4ca4
#define AFE_GASRC1_NEW_CON10              0x4ca8
#define AFE_GASRC1_NEW_CON11              0x4cac
#define AFE_GASRC1_NEW_CON12              0x4cb0
#define AFE_GASRC1_NEW_CON13              0x4cb4
#define AFE_GASRC1_NEW_CON14              0x4cb8
#define AFE_GASRC2_NEW_CON0               0x4cc0
#define AFE_GASRC2_NEW_CON1               0x4cc4
#define AFE_GASRC2_NEW_CON2               0x4cc8
#define AFE_GASRC2_NEW_CON3               0x4ccc
#define AFE_GASRC2_NEW_CON4               0x4cd0
#define AFE_GASRC2_NEW_CON5               0x4cd4
#define AFE_GASRC2_NEW_CON6               0x4cd8
#define AFE_GASRC2_NEW_CON7               0x4cdc
#define AFE_GASRC2_NEW_CON8               0x4ce0
#define AFE_GASRC2_NEW_CON9               0x4ce4
#define AFE_GASRC2_NEW_CON10              0x4ce8
#define AFE_GASRC2_NEW_CON11              0x4cec
#define AFE_GASRC2_NEW_CON12              0x4cf0
#define AFE_GASRC2_NEW_CON13              0x4cf4
#define AFE_GASRC2_NEW_CON14              0x4cf8
#define AFE_GASRC3_NEW_CON0               0x4d00
#define AFE_GASRC3_NEW_CON1               0x4d04
#define AFE_GASRC3_NEW_CON2               0x4d08
#define AFE_GASRC3_NEW_CON3               0x4d0c
#define AFE_GASRC3_NEW_CON4               0x4d10
#define AFE_GASRC3_NEW_CON5               0x4d14
#define AFE_GASRC3_NEW_CON6               0x4d18
#define AFE_GASRC3_NEW_CON7               0x4d1c
#define AFE_GASRC3_NEW_CON8               0x4d20
#define AFE_GASRC3_NEW_CON9               0x4d24
#define AFE_GASRC3_NEW_CON10              0x4d28
#define AFE_GASRC3_NEW_CON11              0x4d2c
#define AFE_GASRC3_NEW_CON12              0x4d30
#define AFE_GASRC3_NEW_CON13              0x4d34
#define AFE_GASRC3_NEW_CON14              0x4d38
#define AFE_GASRC4_NEW_CON0               0x4d40
#define AFE_GASRC4_NEW_CON1               0x4d44
#define AFE_GASRC4_NEW_CON2               0x4d48
#define AFE_GASRC4_NEW_CON3               0x4d4c
#define AFE_GASRC4_NEW_CON4               0x4d50
#define AFE_GASRC4_NEW_CON5               0x4d54
#define AFE_GASRC4_NEW_CON6               0x4d58
#define AFE_GASRC4_NEW_CON7               0x4d5c
#define AFE_GASRC4_NEW_CON8               0x4d60
#define AFE_GASRC4_NEW_CON9               0x4d64
#define AFE_GASRC4_NEW_CON10              0x4d68
#define AFE_GASRC4_NEW_CON11              0x4d6c
#define AFE_GASRC4_NEW_CON12              0x4d70
#define AFE_GASRC4_NEW_CON13              0x4d74
#define AFE_GASRC4_NEW_CON14              0x4d78
#define AFE_GASRC5_NEW_CON0               0x4d80
#define AFE_GASRC5_NEW_CON1               0x4d84
#define AFE_GASRC5_NEW_CON2               0x4d88
#define AFE_GASRC5_NEW_CON3               0x4d8c
#define AFE_GASRC5_NEW_CON4               0x4d90
#define AFE_GASRC5_NEW_CON5               0x4d94
#define AFE_GASRC5_NEW_CON6               0x4d98
#define AFE_GASRC5_NEW_CON7               0x4d9c
#define AFE_GASRC5_NEW_CON8               0x4da0
#define AFE_GASRC5_NEW_CON9               0x4da4
#define AFE_GASRC5_NEW_CON10              0x4da8
#define AFE_GASRC5_NEW_CON11              0x4dac
#define AFE_GASRC5_NEW_CON12              0x4db0
#define AFE_GASRC5_NEW_CON13              0x4db4
#define AFE_GASRC5_NEW_CON14              0x4db8
#define AFE_GASRC6_NEW_CON0               0x4dc0
#define AFE_GASRC6_NEW_CON1               0x4dc4
#define AFE_GASRC6_NEW_CON2               0x4dc8
#define AFE_GASRC6_NEW_CON3               0x4dcc
#define AFE_GASRC6_NEW_CON4               0x4dd0
#define AFE_GASRC6_NEW_CON5               0x4dd4
#define AFE_GASRC6_NEW_CON6               0x4dd8
#define AFE_GASRC6_NEW_CON7               0x4ddc
#define AFE_GASRC6_NEW_CON8               0x4de0
#define AFE_GASRC6_NEW_CON9               0x4de4
#define AFE_GASRC6_NEW_CON10              0x4de8
#define AFE_GASRC6_NEW_CON11              0x4dec
#define AFE_GASRC6_NEW_CON12              0x4df0
#define AFE_GASRC6_NEW_CON13              0x4df4
#define AFE_GASRC6_NEW_CON14              0x4df8
#define AFE_GASRC7_NEW_CON0               0x4e00
#define AFE_GASRC7_NEW_CON1               0x4e04
#define AFE_GASRC7_NEW_CON2               0x4e08
#define AFE_GASRC7_NEW_CON3               0x4e0c
#define AFE_GASRC7_NEW_CON4               0x4e10
#define AFE_GASRC7_NEW_CON5               0x4e14
#define AFE_GASRC7_NEW_CON6               0x4e18
#define AFE_GASRC7_NEW_CON7               0x4e1c
#define AFE_GASRC7_NEW_CON8               0x4e20
#define AFE_GASRC7_NEW_CON9               0x4e24
#define AFE_GASRC7_NEW_CON10              0x4e28
#define AFE_GASRC7_NEW_CON11              0x4e2c
#define AFE_GASRC7_NEW_CON12              0x4e30
#define AFE_GASRC7_NEW_CON13              0x4e34
#define AFE_GASRC7_NEW_CON14              0x4e38
#define AFE_GASRC8_NEW_CON0               0x4e40
#define AFE_GASRC8_NEW_CON1               0x4e44
#define AFE_GASRC8_NEW_CON2               0x4e48
#define AFE_GASRC8_NEW_CON3               0x4e4c
#define AFE_GASRC8_NEW_CON4               0x4e50
#define AFE_GASRC8_NEW_CON5               0x4e54
#define AFE_GASRC8_NEW_CON6               0x4e58
#define AFE_GASRC8_NEW_CON7               0x4e5c
#define AFE_GASRC8_NEW_CON8               0x4e60
#define AFE_GASRC8_NEW_CON9               0x4e64
#define AFE_GASRC8_NEW_CON10              0x4e68
#define AFE_GASRC8_NEW_CON11              0x4e6c
#define AFE_GASRC8_NEW_CON12              0x4e70
#define AFE_GASRC8_NEW_CON13              0x4e74
#define AFE_GASRC8_NEW_CON14              0x4e78
#define AFE_GASRC9_NEW_CON0               0x4e80
#define AFE_GASRC9_NEW_CON1               0x4e84
#define AFE_GASRC9_NEW_CON2               0x4e88
#define AFE_GASRC9_NEW_CON3               0x4e8c
#define AFE_GASRC9_NEW_CON4               0x4e90
#define AFE_GASRC9_NEW_CON5               0x4e94
#define AFE_GASRC9_NEW_CON6               0x4e98
#define AFE_GASRC9_NEW_CON7               0x4e9c
#define AFE_GASRC9_NEW_CON8               0x4ea0
#define AFE_GASRC9_NEW_CON9               0x4ea4
#define AFE_GASRC9_NEW_CON10              0x4ea8
#define AFE_GASRC9_NEW_CON11              0x4eac
#define AFE_GASRC9_NEW_CON12              0x4eb0
#define AFE_GASRC9_NEW_CON13              0x4eb4
#define AFE_GASRC9_NEW_CON14              0x4eb8
#define AFE_GASRC10_NEW_CON0              0x4ec0
#define AFE_GASRC10_NEW_CON1              0x4ec4
#define AFE_GASRC10_NEW_CON2              0x4ec8
#define AFE_GASRC10_NEW_CON3              0x4ecc
#define AFE_GASRC10_NEW_CON4              0x4ed0
#define AFE_GASRC10_NEW_CON5              0x4ed4
#define AFE_GASRC10_NEW_CON6              0x4ed8
#define AFE_GASRC10_NEW_CON7              0x4edc
#define AFE_GASRC10_NEW_CON8              0x4ee0
#define AFE_GASRC10_NEW_CON9              0x4ee4
#define AFE_GASRC10_NEW_CON10             0x4ee8
#define AFE_GASRC10_NEW_CON11             0x4eec
#define AFE_GASRC10_NEW_CON12             0x4ef0
#define AFE_GASRC10_NEW_CON13             0x4ef4
#define AFE_GASRC10_NEW_CON14             0x4ef8
#define AFE_GASRC11_NEW_CON0              0x4f00
#define AFE_GASRC11_NEW_CON1              0x4f04
#define AFE_GASRC11_NEW_CON2              0x4f08
#define AFE_GASRC11_NEW_CON3              0x4f0c
#define AFE_GASRC11_NEW_CON4              0x4f10
#define AFE_GASRC11_NEW_CON5              0x4f14
#define AFE_GASRC11_NEW_CON6              0x4f18
#define AFE_GASRC11_NEW_CON7              0x4f1c
#define AFE_GASRC11_NEW_CON8              0x4f20
#define AFE_GASRC11_NEW_CON9              0x4f24
#define AFE_GASRC11_NEW_CON10             0x4f28
#define AFE_GASRC11_NEW_CON11             0x4f2c
#define AFE_GASRC11_NEW_CON12             0x4f30
#define AFE_GASRC11_NEW_CON13             0x4f34
#define AFE_GASRC11_NEW_CON14             0x4f38
#define AFE_GASRC12_NEW_CON0              0x4f40
#define AFE_GASRC12_NEW_CON1              0x4f44
#define AFE_GASRC12_NEW_CON2              0x4f48
#define AFE_GASRC12_NEW_CON3              0x4f4c
#define AFE_GASRC12_NEW_CON4              0x4f50
#define AFE_GASRC12_NEW_CON5              0x4f54
#define AFE_GASRC12_NEW_CON6              0x4f58
#define AFE_GASRC12_NEW_CON7              0x4f5c
#define AFE_GASRC12_NEW_CON8              0x4f60
#define AFE_GASRC12_NEW_CON9              0x4f64
#define AFE_GASRC12_NEW_CON10             0x4f68
#define AFE_GASRC12_NEW_CON11             0x4f6c
#define AFE_GASRC12_NEW_CON12             0x4f70
#define AFE_GASRC12_NEW_CON13             0x4f74
#define AFE_GASRC12_NEW_CON14             0x4f78
#define AFE_GASRC13_NEW_CON0              0x4f80
#define AFE_GASRC13_NEW_CON1              0x4f84
#define AFE_GASRC13_NEW_CON2              0x4f88
#define AFE_GASRC13_NEW_CON3              0x4f8c
#define AFE_GASRC13_NEW_CON4              0x4f90
#define AFE_GASRC13_NEW_CON5              0x4f94
#define AFE_GASRC13_NEW_CON6              0x4f98
#define AFE_GASRC13_NEW_CON7              0x4f9c
#define AFE_GASRC13_NEW_CON8              0x4fa0
#define AFE_GASRC13_NEW_CON9              0x4fa4
#define AFE_GASRC13_NEW_CON10             0x4fa8
#define AFE_GASRC13_NEW_CON11             0x4fac
#define AFE_GASRC13_NEW_CON12             0x4fb0
#define AFE_GASRC13_NEW_CON13             0x4fb4
#define AFE_GASRC13_NEW_CON14             0x4fb8
#define AFE_GASRC14_NEW_CON0              0x4fc0
#define AFE_GASRC14_NEW_CON1              0x4fc4
#define AFE_GASRC14_NEW_CON2              0x4fc8
#define AFE_GASRC14_NEW_CON3              0x4fcc
#define AFE_GASRC14_NEW_CON4              0x4fd0
#define AFE_GASRC14_NEW_CON5              0x4fd4
#define AFE_GASRC14_NEW_CON6              0x4fd8
#define AFE_GASRC14_NEW_CON7              0x4fdc
#define AFE_GASRC14_NEW_CON8              0x4fe0
#define AFE_GASRC14_NEW_CON9              0x4fe4
#define AFE_GASRC14_NEW_CON10             0x4fe8
#define AFE_GASRC14_NEW_CON11             0x4fec
#define AFE_GASRC14_NEW_CON12             0x4ff0
#define AFE_GASRC14_NEW_CON13             0x4ff4
#define AFE_GASRC14_NEW_CON14             0x4ff8
#define AFE_GASRC15_NEW_CON0              0x5000
#define AFE_GASRC15_NEW_CON1              0x5004
#define AFE_GASRC15_NEW_CON2              0x5008
#define AFE_GASRC15_NEW_CON3              0x500c
#define AFE_GASRC15_NEW_CON4              0x5010
#define AFE_GASRC15_NEW_CON5              0x5014
#define AFE_GASRC15_NEW_CON6              0x5018
#define AFE_GASRC15_NEW_CON7              0x501c
#define AFE_GASRC15_NEW_CON8              0x5020
#define AFE_GASRC15_NEW_CON9              0x5024
#define AFE_GASRC15_NEW_CON10             0x5028
#define AFE_GASRC15_NEW_CON11             0x502c
#define AFE_GASRC15_NEW_CON12             0x5030
#define AFE_GASRC15_NEW_CON13             0x5034
#define AFE_GASRC15_NEW_CON14             0x5038
#define AFE_GASRC16_NEW_CON0              0x5040
#define AFE_GASRC16_NEW_CON1              0x5044
#define AFE_GASRC16_NEW_CON2              0x5048
#define AFE_GASRC16_NEW_CON3              0x504c
#define AFE_GASRC16_NEW_CON4              0x5050
#define AFE_GASRC16_NEW_CON5              0x5054
#define AFE_GASRC16_NEW_CON6              0x5058
#define AFE_GASRC16_NEW_CON7              0x505c
#define AFE_GASRC16_NEW_CON8              0x5060
#define AFE_GASRC16_NEW_CON9              0x5064
#define AFE_GASRC16_NEW_CON10             0x5068
#define AFE_GASRC16_NEW_CON11             0x506c
#define AFE_GASRC16_NEW_CON12             0x5070
#define AFE_GASRC16_NEW_CON13             0x5074
#define AFE_GASRC16_NEW_CON14             0x5078
#define AFE_GASRC17_NEW_CON0              0x5080
#define AFE_GASRC17_NEW_CON1              0x5084
#define AFE_GASRC17_NEW_CON2              0x5088
#define AFE_GASRC17_NEW_CON3              0x508c
#define AFE_GASRC17_NEW_CON4              0x5090
#define AFE_GASRC17_NEW_CON5              0x5094
#define AFE_GASRC17_NEW_CON6              0x5098
#define AFE_GASRC17_NEW_CON7              0x509c
#define AFE_GASRC17_NEW_CON8              0x50a0
#define AFE_GASRC17_NEW_CON9              0x50a4
#define AFE_GASRC17_NEW_CON10             0x50a8
#define AFE_GASRC17_NEW_CON11             0x50ac
#define AFE_GASRC17_NEW_CON12             0x50b0
#define AFE_GASRC17_NEW_CON13             0x50b4
#define AFE_GASRC17_NEW_CON14             0x50b8
#define AFE_GASRC18_NEW_CON0              0x50c0
#define AFE_GASRC18_NEW_CON1              0x50c4
#define AFE_GASRC18_NEW_CON2              0x50c8
#define AFE_GASRC18_NEW_CON3              0x50cc
#define AFE_GASRC18_NEW_CON4              0x50d0
#define AFE_GASRC18_NEW_CON5              0x50d4
#define AFE_GASRC18_NEW_CON6              0x50d8
#define AFE_GASRC18_NEW_CON7              0x50dc
#define AFE_GASRC18_NEW_CON8              0x50e0
#define AFE_GASRC18_NEW_CON9              0x50e4
#define AFE_GASRC18_NEW_CON10             0x50e8
#define AFE_GASRC18_NEW_CON11             0x50ec
#define AFE_GASRC18_NEW_CON12             0x50f0
#define AFE_GASRC18_NEW_CON13             0x50f4
#define AFE_GASRC18_NEW_CON14             0x50f8
#define AFE_GASRC19_NEW_CON0              0x5100
#define AFE_GASRC19_NEW_CON1              0x5104
#define AFE_GASRC19_NEW_CON2              0x5108
#define AFE_GASRC19_NEW_CON3              0x510c
#define AFE_GASRC19_NEW_CON4              0x5110
#define AFE_GASRC19_NEW_CON5              0x5114
#define AFE_GASRC19_NEW_CON6              0x5118
#define AFE_GASRC19_NEW_CON7              0x511c
#define AFE_GASRC19_NEW_CON8              0x5120
#define AFE_GASRC19_NEW_CON9              0x5124
#define AFE_GASRC19_NEW_CON10             0x5128
#define AFE_GASRC19_NEW_CON11             0x512c
#define AFE_GASRC19_NEW_CON12             0x5130
#define AFE_GASRC19_NEW_CON13             0x5134
#define AFE_GASRC19_NEW_CON14             0x5138

#define AFE_ASRC11_NEW_CON0               0x0d80
#define AFE_ASRC11_NEW_CON1               0x0d84
#define AFE_ASRC11_NEW_CON2               0x0d88
#define AFE_ASRC11_NEW_CON3               0x0d8c
#define AFE_ASRC11_NEW_CON4               0x0d90
#define AFE_ASRC11_NEW_CON5               0x0d94
#define AFE_ASRC11_NEW_CON6               0x0d98
#define AFE_ASRC11_NEW_CON7               0x0d9c
#define AFE_ASRC11_NEW_CON8               0x0da0
#define AFE_ASRC11_NEW_CON9               0x0da4
#define AFE_ASRC11_NEW_CON10              0x0da8
#define AFE_ASRC11_NEW_CON11              0x0dac
#define AFE_ASRC11_NEW_CON13              0x0db4
#define AFE_ASRC11_NEW_CON14              0x0db8
#define AFE_ASRC12_NEW_CON0               0x0dc0
#define AFE_ASRC12_NEW_CON1               0x0dc4
#define AFE_ASRC12_NEW_CON2               0x0dc8
#define AFE_ASRC12_NEW_CON3               0x0dcc
#define AFE_ASRC12_NEW_CON4               0x0dd0
#define AFE_ASRC12_NEW_CON5               0x0dd4
#define AFE_ASRC12_NEW_CON6               0x0dd8
#define AFE_ASRC12_NEW_CON7               0x0ddc
#define AFE_ASRC12_NEW_CON8               0x0de0
#define AFE_ASRC12_NEW_CON9               0x0de4
#define AFE_ASRC12_NEW_CON10              0x0de8
#define AFE_ASRC12_NEW_CON11              0x0dec
#define AFE_ASRC12_NEW_CON13              0x0df4
#define AFE_ASRC12_NEW_CON14              0x0df8


#define AFE_ASRCO5_NEW_CON0		0x0a40
#define AFE_ASRCO5_NEW_CON1		0x0a44
#define AFE_ASRCO5_NEW_CON4		0x0a50
#define AFE_ASRCO5_NEW_CON6		0x0a58
#define AFE_ASRCO5_NEW_CON7		0x0a5c
#define AFE_ASRCO5_NEW_CON8		0x0a60
#define AFE_ASRCO5_NEW_CON9		0x0a64

#define AFE_DAC_CON0			0x1200
#define AFE_DAC_CON1			0x1204
#define AFE_DAC_CON2			0x1208

#define AFE_DL2_BASE			0x1250
#define AFE_DL2_CUR			0x1254
#define AFE_DL2_END			0x1258
#define AFE_DL2_CON0			0x125c

#define AFE_DL3_BASE			0x1260
#define AFE_DL3_CUR			0x1264
#define AFE_DL3_END			0x1268
#define AFE_DL3_CON0			0x126c

#define AFE_DL6_BASE			0x1290
#define AFE_DL6_CUR			0x1294
#define AFE_DL6_END			0x1298
#define AFE_DL6_CON0			0x129c

#define AFE_DL7_BASE			0x12a0
#define AFE_DL7_CUR			0x12a4
#define AFE_DL7_END			0x12a8
#define AFE_DL7_CON0			0x12ac

#define AFE_DL8_BASE			0x12b0
#define AFE_DL8_CUR			0x12b4
#define AFE_DL8_END			0x12b8
#define AFE_DL8_CON0			0x12bc

#define AFE_DL10_BASE			0x12d0
#define AFE_DL10_CUR			0x12d4
#define AFE_DL10_END			0x12d8
#define AFE_DL10_CON0			0x12dc

#define AFE_DL11_BASE			0x12e0
#define AFE_DL11_CUR			0x12e4
#define AFE_DL11_END			0x12e8
#define AFE_DL11_CON0			0x12ec


#define AFE_UL1_BASE			0x1300
#define AFE_UL1_CUR			0x1304
#define AFE_UL1_END			0x1308
#define AFE_UL1_CON0			0x130c

#define AFE_UL2_BASE			0x1310
#define AFE_UL2_CUR			0x1314
#define AFE_UL2_END			0x1318
#define AFE_UL2_CON0			0x131c

#define AFE_UL3_BASE			0x1320
#define AFE_UL3_CUR			0x1324
#define AFE_UL3_END			0x1328
#define AFE_UL3_CON0			0x132c

#define AFE_UL4_BASE			0x1330
#define AFE_UL4_CUR			0x1334
#define AFE_UL4_END			0x1338
#define AFE_UL4_CON0			0x133c

#define AFE_UL5_BASE			0x1340
#define AFE_UL5_CUR			0x1344
#define AFE_UL5_END			0x1348
#define AFE_UL5_CON0			0x134c

#define AFE_UL6_BASE			0x1350
#define AFE_UL6_CUR			0x1354
#define AFE_UL6_END			0x1358
#define AFE_UL6_CON0			0x135c

#define AFE_UL8_BASE			0x1370
#define AFE_UL8_CUR			0x1374
#define AFE_UL8_END			0x1378
#define AFE_UL8_CON0			0x137c

#define AFE_UL9_BASE			0x1380
#define AFE_UL9_CUR			0x1384
#define AFE_UL9_END			0x1388
#define AFE_UL9_CON0			0x138c

#define AFE_UL10_BASE			0x13d0
#define AFE_UL10_CUR			0x13d4
#define AFE_UL10_END			0x13d8
#define AFE_UL10_CON0			0x13dc

#define AFE_DL8_CHK_SUM1		0x1400
#define AFE_DL8_CHK_SUM2		0x1404
#define AFE_DL8_CHK_SUM3		0x1408
#define AFE_DL8_CHK_SUM4		0x140c
#define AFE_DL10_CHK_SUM1		0x1418
#define AFE_DL10_CHK_SUM2		0x141c
#define AFE_DL10_CHK_SUM3		0x1420
#define AFE_DL10_CHK_SUM4		0x1424
#define AFE_DL10_CHK_SUM5		0x1428
#define AFE_DL10_CHK_SUM6		0x142c

#define AFE_DL11_CHK_SUM1		0x1430
#define AFE_DL11_CHK_SUM2		0x1434
#define AFE_DL11_CHK_SUM3		0x1438
#define AFE_DL11_CHK_SUM4		0x143c
#define AFE_DL11_CHK_SUM5		0x1440
#define AFE_DL11_CHK_SUM6		0x1444

#define AFE_UL1_CHK_SUM1		0x1450
#define AFE_UL1_CHK_SUM2		0x1454
#define AFE_UL2_CHK_SUM1		0x1458
#define AFE_UL2_CHK_SUM2		0x145c
#define AFE_UL3_CHK_SUM1		0x1460
#define AFE_UL3_CHK_SUM2		0x1464
#define AFE_UL4_CHK_SUM1		0x1468
#define AFE_UL4_CHK_SUM2		0x146c
#define AFE_UL5_CHK_SUM1		0x1470
#define AFE_UL5_CHK_SUM2		0x1474

#define AFE_UL6_CHK_SUM1		0x1478
#define AFE_UL6_CHK_SUM2		0x147c

#define AFE_UL8_CHK_SUM1		0x1488
#define AFE_UL8_CHK_SUM2		0x148c
#define AFE_DL2_CHK_SUM1		0x14a0
#define AFE_DL2_CHK_SUM2		0x14a4
#define AFE_DL3_CHK_SUM1		0x14b0
#define AFE_DL3_CHK_SUM2		0x14b4
#define AFE_DL6_CHK_SUM1		0x14e0
#define AFE_DL6_CHK_SUM2		0x14e4
#define AFE_DL7_CHK_SUM1		0x14f0
#define AFE_DL7_CHK_SUM2		0x14f4
#define AFE_UL9_CHK_SUM1		0x1528
#define AFE_UL9_CHK_SUM2		0x152c

#define AFE_BUS_MON1			0x1540

#define AFE_MEMIF_AGENT_FS_CON0		0x15a0
#define AFE_MEMIF_AGENT_FS_CON1		0x15a4
#define AFE_MEMIF_AGENT_FS_CON2		0x15a8
#define AFE_MEMIF_AGENT_FS_CON3		0x15ac

#define AFE_NORMAL_BASE_ADR_MSB		0x192c
#define AFE_NORMAL_END_ADR_MSB		0x1930

#define AFE_LOOPBACK_CFG0			0x1950
#define MULTI_IN_BCK_SEL_MASK			GENMASK(2, 0)
#define MULTI_IN_BCK_SEL_SPLIN			(0x0 << 0)
#define MULTI_IN_BCK_SEL_ETDM_IN2		(0x1 << 0)
#define MULTI_IN_BCK_SEL_NONE2		(0x2 << 0)
#define MULTI_IN_BCK_SEL_EARC			(0x3 << 0)
#define MULTI_IN_BCK_SEL_NONE4		(0x4 << 0)
#define MULTI_IN_BCK_SEL_ETDM_OUT3		(0x4 << 0)
#define MULTI_IN_BCK_SEL_HDMIRX		(0x5 << 0)
#define MULTI_IN_BCK_SEL_AUD2HDMI		(0x6 << 0)

#define MULTI_IN_LRCK_SEL_MASK		GENMASK(5, 3)
#define MULTI_IN_LRCK_SEL_SPLIN		(0x0 << 3)
#define MULTI_IN_LRCK_SEL_ETDM_IN2		(0x1 << 3)
#define MULTI_IN_LRCK_SEL_AUD2HDMI		(0x2 << 3)
#define MULTI_IN_LRCK_SEL_EARC		(0x3 << 3)
#define MULTI_IN_LRCK_SEL_AUD2HD4		(0x4 << 3)
#define MULTI_IN_LRCK_SEL_ETDM_OUT3		(0x4 << 3)
#define MULTI_IN_LRCK_SEL_HDMIRX		(0x5 << 3)
#define MULTI_IN_LRCK_SEL_EO3D4		(0x6 << 3)

#define MULTI_IN_DAT0_SEL_MASK		GENMASK(8, 6)
#define MULTI_IN_DAT0_SEL_SPLIN		(0x0 << 6)
#define MULTI_IN_DAT0_SEL_ETDM_OUT3		(0x1 << 6)
#define MULTI_IN_DAT0_SEL_ETDM_IN2		(0x2 << 6)
#define MULTI_IN_DAT0_SEL_EARC		(0x3 << 6)
#define MULTI_IN_DAT0_SEL_HDMIRX		(0x4 << 6)
#define MULTI_IN_DAT0_SEL_AUD2_HDMI		(0x6 << 6)

#define MULTI_IN_DAT1_SEL_MASK		GENMASK(11, 9)
#define MULTI_IN_DAT1_SEL_SPLIN		(0x0 << 9)
#define MULTI_IN_DAT1_SEL_ETDM_OUT3		(0x1 << 9)
#define MULTI_IN_DAT1_SEL_ETDM_IN2		(0x2 << 9)
#define MULTI_IN_DAT1_SEL_EARC		(0x3 << 9)
#define MULTI_IN_DAT1_SEL_HDMIRX		(0x4 << 9)
#define MULTI_IN_DAT1_SEL_AUD2_HDMI		(0x6 << 9)

#define MULTI_IN_DAT2_SEL_MASK		GENMASK(14, 12)
#define MULTI_IN_DAT2_SEL_SPLIN		(0x0 << 12)
#define MULTI_IN_DAT2_SEL_ETDM_OUT3		(0x1 << 12)
#define MULTI_IN_DAT2_SEL_ETDM_IN2		(0x2 << 12)
#define MULTI_IN_DAT2_SEL_EARC		(0x3 << 12)
#define MULTI_IN_DAT2_SEL_HDMIRX		(0x4 << 12)
#define MULTI_IN_DAT2_SEL_AUD2_HDMI		(0x6 << 12)

#define MULTI_IN_DAT3_SEL_MASK		GENMASK(17, 15)
#define MULTI_IN_DAT3_SEL_SPLIN		(0x0 << 15)
#define MULTI_IN_DAT3_SEL_ETDM_OUT3		(0x1 << 15)
#define MULTI_IN_DAT3_SEL_ETDM_IN2		(0x2 << 15)
#define MULTI_IN_DAT3_SEL_EARC		(0x3 << 15)
#define MULTI_IN_DAT3_SEL_HDMIRX		(0x4 << 15)
#define MULTI_IN_DAT3_SEL_AUD2_HDMI		(0x6 << 15)

#define AFE_LOOPBACK_CFG1		0x1954
#define AFE_LOOPBACK_CFG2		0x1958
#define AFE_LOOPBACK_CFG3		0x195c


#define DMIC_TOP_CON			0x1A00
#define DMIC_IIR_ULCF_COEF_CON1		0x1A04
#define DMIC_IIR_ULCF_COEF_CON2		0x1A08
#define DMIC_IIR_ULCF_COEF_CON3		0x1A0C
#define DMIC_IIR_ULCF_COEF_CON4		0x1A10
#define DMIC_IIR_ULCF_COEF_CON5		0x1A14
#define DMIC2_TOP_CON			0x1A58
#define DMIC2_IIR_ULCF_COEF_CON1	0x1A5C
#define DMIC2_IIR_ULCF_COEF_CON2	0x1A60
#define DMIC2_IIR_ULCF_COEF_CON3	0x1A64
#define DMIC2_IIR_ULCF_COEF_CON4	0x1A68
#define DMIC2_IIR_ULCF_COEF_CON5	0x1A6C
#define DMIC3_TOP_CON			0x1AB0
#define DMIC3_IIR_ULCF_COEF_CON1	0x1AB4
#define DMIC3_IIR_ULCF_COEF_CON2	0x1AB8
#define DMIC3_IIR_ULCF_COEF_CON3	0x1ABC
#define DMIC3_IIR_ULCF_COEF_CON4	0x1AC0
#define DMIC3_IIR_ULCF_COEF_CON5	0x1AC4
#define DMIC4_TOP_CON			0x1B08
#define DMIC4_IIR_ULCF_COEF_CON1	0x1B0C
#define DMIC4_IIR_ULCF_COEF_CON2	0x1B10
#define DMIC4_IIR_ULCF_COEF_CON3	0x1B14
#define DMIC4_IIR_ULCF_COEF_CON4	0x1B18
#define DMIC4_IIR_ULCF_COEF_CON5	0x1B1C


//for 8532 dmic
#define AFE_DMIC0_UL_SRC_CON0             0x1a00
#define AFE_DMIC0_UL_SRC_CON1             0x1a04
#define AFE_DMIC0_SRC_DEBUG               0x1a08
#define AFE_DMIC0_SRC_DEBUG_MON0          0x1a0c
#define AFE_DMIC0_UL_SRC_MON0             0x1a10
#define AFE_DMIC0_UL_SRC_MON1             0x1a14
#define AFE_DMIC0_IIR_COEF_02_01          0x1a18
#define AFE_DMIC0_IIR_COEF_04_03          0x1a1c
#define AFE_DMIC0_IIR_COEF_06_05          0x1a20
#define AFE_DMIC0_IIR_COEF_08_07          0x1a24
#define AFE_DMIC0_IIR_COEF_10_09          0x1a28
#define AFE_DMIC1_UL_SRC_CON0             0x1a68
#define AFE_DMIC1_UL_SRC_CON1             0x1a6c
#define AFE_DMIC1_SRC_DEBUG               0x1a70
#define AFE_DMIC1_SRC_DEBUG_MON0          0x1a74
#define AFE_DMIC1_UL_SRC_MON0             0x1a78
#define AFE_DMIC1_UL_SRC_MON1             0x1a7c
#define AFE_DMIC1_IIR_COEF_02_01          0x1a80
#define AFE_DMIC1_IIR_COEF_04_03          0x1a84
#define AFE_DMIC1_IIR_COEF_06_05          0x1a88
#define AFE_DMIC1_IIR_COEF_08_07          0x1a8c
#define AFE_DMIC1_IIR_COEF_10_09          0x1a90
#define AFE_DMIC2_UL_SRC_CON0             0x1ad0
#define AFE_DMIC2_UL_SRC_CON1             0x1ad4
#define AFE_DMIC2_SRC_DEBUG               0x1ad8
#define AFE_DMIC2_SRC_DEBUG_MON0          0x1adc
#define AFE_DMIC2_UL_SRC_MON0             0x1ae0
#define AFE_DMIC2_UL_SRC_MON1             0x1ae4
#define AFE_DMIC2_IIR_COEF_02_01          0x1ae8
#define AFE_DMIC2_IIR_COEF_04_03          0x1aec
#define AFE_DMIC2_IIR_COEF_06_05          0x1af0
#define AFE_DMIC2_IIR_COEF_08_07          0x1af4
#define AFE_DMIC2_IIR_COEF_10_09          0x1af8
#define AFE_DMIC3_UL_SRC_CON0             0x1b38
#define AFE_DMIC3_UL_SRC_CON1             0x1b3c
#define AFE_DMIC3_SRC_DEBUG               0x1b40
#define AFE_DMIC3_SRC_DEBUG_MON0          0x1b44
#define AFE_DMIC3_UL_SRC_MON0             0x1b48
#define AFE_DMIC3_UL_SRC_MON1             0x1b4c
#define AFE_DMIC3_IIR_COEF_02_01          0x1b50
#define AFE_DMIC3_IIR_COEF_04_03          0x1b54
#define AFE_DMIC3_IIR_COEF_06_05          0x1b58
#define AFE_DMIC3_IIR_COEF_08_07          0x1b5c
#define AFE_DMIC3_IIR_COEF_10_09          0x1b60
#define DMIC_BYPASS_HW_GAIN               0x1bf0
#define DMIC_GAIN1_CON0                   0x1c00
#define DMIC_GAIN1_CON1                   0x1c04
#define DMIC_GAIN1_CON2                   0x1c08
#define DMIC_GAIN1_CON3                   0x1c0c
#define DMIC_GAIN1_CUR                    0x1c10
#define DMIC_GAIN2_CON0                   0x1c20
#define DMIC_GAIN2_CON1                   0x1c24
#define DMIC_GAIN2_CON2                   0x1c28
#define DMIC_GAIN2_CON3                   0x1c2c
#define DMIC_GAIN2_CUR                    0x1c30
#define DMIC_GAIN3_CON0                   0x1c40
#define DMIC_GAIN3_CON1                   0x1c44
#define DMIC_GAIN3_CON2                   0x1c48
#define DMIC_GAIN3_CON3                   0x1c4c
#define DMIC_GAIN3_CUR                    0x1c50
#define DMIC_GAIN4_CON0                   0x1c60
#define DMIC_GAIN4_CON1                   0x1c64
#define DMIC_GAIN4_CON2                   0x1c68
#define DMIC_GAIN4_CON3                   0x1c6c
#define DMIC_GAIN4_CUR                    0x1c70


#define ETDM_OUT1_DSD_FADE_CON            0x2260
#define ETDM_OUT1_DSD_FADE_CON1           0x2264
#define ETDM_OUT3_DSD_FADE_CON            0x2280
#define ETDM_OUT3_DSD_FADE_CON1           0x2284
#define ETDM_IN1_AFIFO_CON                0x2294
#define ETDM_IN2_AFIFO_CON                0x2298

#define ETDM_IN1_MONITOR		0x22c0
#define ETDM_IN2_MONITOR		0x22c4
#define ETDM_OUT1_MONITOR		0x22d0
#define ETDM_OUT2_MONITOR		0x22d4
#define ETDM_OUT3_MONITOR       0x22d8

#define ETDM_COWORK_CON0		0x22f0
#define ETDM_COWORK_CON1		0x22f4
#define ETDM_COWORK_CON2        0x22f8
#define ETDM_COWORK_CON3		0x22fc

#define ETDM_IN1_CON0			0x2300
#define ETDM_IN1_CON1			0x2304
#define ETDM_IN1_CON2			0x2308
#define ETDM_IN1_CON3			0x230c
#define ETDM_IN1_CON4			0x2310
#define ETDM_IN1_CON5           0x2314
#define ETDM_IN1_CON6           0x2318
#define ETDM_IN1_CON7           0x231c

#define ETDM_IN2_CON0			0x2320
#define ETDM_IN2_CON1			0x2324
#define ETDM_IN2_CON2			0x2328
#define ETDM_IN2_CON3			0x232c
#define ETDM_IN2_CON4			0x2330
#define ETDM_IN2_CON5           0x2334
#define ETDM_IN2_CON6           0x2338
#define ETDM_IN2_CON7           0x233c


#define ETDM_OUT1_CON0			0x2380
#define ETDM_OUT1_CON1			0x2384
#define ETDM_OUT1_CON2			0x2388
#define ETDM_OUT1_CON3			0x238c
#define ETDM_OUT1_CON4			0x2390
#define ETDM_OUT1_CON5          0x2394
#define ETDM_OUT1_CON6          0x2398
#define ETDM_OUT1_CON7          0x239c


#define ETDM_OUT2_CON0			0x23a0
#define ETDM_OUT2_CON1			0x23a4
#define ETDM_OUT2_CON2			0x23a8
#define ETDM_OUT2_CON3			0x23ac
#define ETDM_OUT2_CON4			0x23b0
#define ETDM_OUT2_CON5          0x23b4
#define ETDM_OUT2_CON6          0x23b8
#define ETDM_OUT2_CON7          0x23bc

#define ETDM_OUT3_CON0			0x23c0
#define ETDM_OUT3_CON1			0x23c4
#define ETDM_OUT3_CON2			0x23c8
#define ETDM_OUT3_CON3			0x23cc
#define ETDM_OUT3_CON4			0x23d0
#define ETDM_OUT3_CON5          0x23d4
#define ETDM_OUT3_CON6          0x23d8
#define ETDM_OUT3_CON7          0x23dc


#define GASRC_TIMING_CON0      0x2414
#define GASRC_TIMING_CON1      0x2418
#define GASRC_TIMING_CON2      0x241c
#define GASRC_TIMING_CON3      0x2420
#define GASRC_TIMING_CON4      0x2424
#define GASRC_TIMING_CON5      0x2428
#define GASRC_TIMING_CON6      0x242c
#define GASRC_TIMING_CON7      0x2430
#define A3_A4_TIMING_SEL0      0x2440
#define A3_A4_TIMING_SEL1      0x2444
#define A3_A4_TIMING_SEL2      0x2448
#define A3_A4_TIMING_SEL3      0x244c
#define A3_A4_TIMING_SEL4      0x2450
#define A3_A4_TIMING_SEL5      0x2454
#define A3_A4_TIMING_SEL6      0x2458
#define A3_A4_TIMING_SEL7      0x245c

#define GASRC_CFG0			0x2400

//#define GASRC_TIMING_CON0		0x2408
//#define GASRC_TIMING_CON1		0x240c
#define MAX_REGISTER			AFE_GASRC19_NEW_CON14

#define AFE_IRQ_STATUS_BITS		0x1ffff
#define AFE_IRQ_MCU_CLR_BITS		0x1ff
#define ASYS_IRQ_CLR_BITS		0xffff

/* AUDIO_TOP_CON0 (0x0000) */
#define AUD_TCON0_PDN_TML			BIT(27)
#define AUD_TCON0_PDN_APLL2			BIT(24)//for apll_tuner
#define AUD_TCON0_PDN_APLL			BIT(23)//for apll2_tuner
#define AUD_TCON0_PDN_SPDIF_OUT		BIT(21)
#define AUD_TCON0_PDN_APLL2_TUNER	BIT(20)
#define AUD_TCON0_PDN_APLL_TUNER	BIT(19)
#define AUD_TCON0_SPDF_OUT_PLL_SEL_MASK		BIT(15)
#define AUD_TCON0_PDN_SPDIFIN_TUNER_DBG			BIT(11)
#define AUD_TCON0_PDN_SPDIFIN_TUNER_APLL		BIT(10)
#define AUD_TCON0_PDN_AFE			BIT(2)


/* AUDIO_TOP_CON1 (0x0004) */
#define AUD_TCON1_PDN_26M_DMIC_TM			BIT(14)
#define AUD_TCON1_PDN_DMIC4_26M_UL_HOP_BCLK			BIT(13)
#define AUD_TCON1_PDN_DMIC3_26M_UL_HOP_BCLK			BIT(12)
#define AUD_TCON1_PDN_DMIC2_26M_UL_HOP_BCLK			BIT(11)
#define AUD_TCON1_PDN_DMIC1_26M_UL_HOP_BCLK			BIT(10)
#define AUD_TCON1_PDN_A1SYS_HP			BIT(2)
#define AUD_TCON1_A1SYS_HP_SEL_MASK		0x3
#define AUD_TCON1_A1SYS_HP_SEL_VAL(x)	(x)

#if 1
/* AUDIO_TOP_CON2 (0x0008) use the topckgen*/
#define AUD_TCON0_CON2_SPDF_DIV_MASK		GENMASK(7, 0)
#define AUD_TCON0_CON2_SPDF_DIV(x)		(((x-1) & 0xff))
#endif

/* AUDIO_TOP_CON3 (0x000c) */
#define AUD_TCON3_PDN_EARC_TUNER			BIT(7)
#define AUD_TCON3_PDN_LINEIN_TUNER			BIT(5)

/* AUDIO_TOP_CON4 (0x0010) */
#define AUD_TCON4_PDN_A4SYS			BIT(31)
#define AUD_TCON4_PDN_A3SYS			BIT(30)
#define AUD_TCON4_PDN_PCMIF			BIT(24)
#define AUD_TCON4_PDN_AFE_CONN			BIT(23)
#define AUD_TCON4_PDN_A2SYS			BIT(22)
#define AUD_TCON4_PDN_A1SYS			BIT(21)
#define AUD_TCON4_PDN_INTDIR			BIT(20)
#define AUD_TCON4_PDN_MULTI_IN			BIT(19)
#define AUD_TCON4_PDN_ASRC12			BIT(17)
#define AUD_TCON4_PDN_ASRC11			BIT(16)
#define AUD_TCON4_PDN_HDMI_OUT			BIT(8)
#define AUD_TCON4_PDN_TDM_OUT			BIT(7)
#define AUD_TCON4_PDN_I2S_OUT			BIT(6)
#define AUD_TCON4_PDN_TDM_IN			BIT(1)
#define AUD_TCON4_PDN_I2S_IN			BIT(0)


/* AUDIO_TOP_CON6 (0x0018) */
#define AUD_TCON6_PDN_GASRC19			BIT(19)
#define AUD_TCON6_PDN_GASRC18			BIT(18)
#define AUD_TCON6_PDN_GASRC17			BIT(17)
#define AUD_TCON6_PDN_GASRC16			BIT(16)
#define AUD_TCON6_PDN_GASRC15			BIT(15)
#define AUD_TCON6_PDN_GASRC14			BIT(14)
#define AUD_TCON6_PDN_GASRC13			BIT(13)
#define AUD_TCON6_PDN_GASRC12			BIT(12)
#define AUD_TCON6_PDN_GASRC11			BIT(11)
#define AUD_TCON6_PDN_GASRC10			BIT(10)
#define AUD_TCON6_PDN_GASRC9			BIT(9)
#define AUD_TCON6_PDN_GASRC8			BIT(8)
#define AUD_TCON6_PDN_GASRC7			BIT(7)
#define AUD_TCON6_PDN_GASRC6			BIT(6)
#define AUD_TCON6_PDN_GASRC5			BIT(5)
#define AUD_TCON6_PDN_GASRC4			BIT(4)
#define AUD_TCON6_PDN_GASRC3			BIT(3)
#define AUD_TCON6_PDN_GASRC2			BIT(2)
#define AUD_TCON6_PDN_GASRC1			BIT(1)
#define AUD_TCON6_PDN_GASRC0			BIT(0)


/* ASMO_TIMING_CON0 (0x0100) */
#define ASMO_TIMING_CON0_ASMO0_MODE_MASK	GENMASK(4, 0)
#define ASMO_TIMING_CON0_ASMO0_MODE_VAL(x)	((x & 0x1f) << 0)

/* PWR1_ASM_CON1 (0x0108) */
#define PWR1_ASM_CON1_GASRC0_CALI_CK_SEL_MASK	BIT(2)
#define PWR1_ASM_CON1_GASRC0_CALI_CK_SEL(x)	(x << 2)
#define PWR1_ASM_CON1_GASRC1_CALI_CK_SEL_MASK	BIT(5)
#define PWR1_ASM_CON1_GASRC1_CALI_CK_SEL(x)	(x << 5)
#define PWR1_ASM_CON1_GASRC2_CALI_CK_SEL_MASK	BIT(20)
#define PWR1_ASM_CON1_GASRC2_CALI_CK_SEL(x)	(x << 20)
#define PWR1_ASM_CON1_GASRC3_CALI_CK_SEL_MASK	BIT(23)
#define PWR1_ASM_CON1_GASRC3_CALI_CK_SEL(x)	(x << 23)
#define PWR1_ASM_CON1_DL_ASRC_CALI_CK_SEL_MASK	BIT(26)
#define PWR1_ASM_CON1_DL_ASRC_CALI_CK_SEL(x)	(x << 26)

/* AFE_IRQ_MASK (0x0178) */
#define AFE_IRQ_MASK_EN_BITS			(0x3870fff)
#define AFE_IRQ_MASK_EN_MASK			GENMASK(25, 0)

/* AFE_SINEGEN_CON0 (0x01f0) */
#define AFE_SINEGEN_CON0_INIT_MASK		(0xff0ff)
#define AFE_SINEGEN_CON0_INIT_VAL		(0x21021)
#define AFE_SINEGEN_CON0_EN			BIT(26)
#define AFE_SINEGEN_CON0_FREQ_DIV_CH2_MASK	GENMASK(16, 12)
#define AFE_SINEGEN_CON0_FREQ_DIV_CH1_MASK	GENMASK(4, 0)
#define AFE_SINEGEN_CON0_FREQ_DIV_CH2(x)	(((x) & 0x1f) << 12)
#define AFE_SINEGEN_CON0_FREQ_DIV_CH1(x)	(((x) & 0x1f) << 0)

/* AFE_SINEGEN_CON1 (0x01f4) */
#define AFE_SINEGEN_CON1_TIMING_CH2_MASK	GENMASK(25, 21)
#define AFE_SINEGEN_CON1_TIMING_CH1_MASK	GENMASK(20, 16)
#define AFE_SINEGEN_CON1_TIMING_CH2(x)		(((x) & 0x1f) << 21)
#define AFE_SINEGEN_CON1_TIMING_CH1(x)		(((x) & 0x1f) << 16)
#define AFE_SINEGEN_CON1_TIMING_8K		(0)
#define AFE_SINEGEN_CON1_TIMING_12K		(1)
#define AFE_SINEGEN_CON1_TIMING_16K		(2)
#define AFE_SINEGEN_CON1_TIMING_24K		(3)
#define AFE_SINEGEN_CON1_TIMING_32K		(4)
#define AFE_SINEGEN_CON1_TIMING_48K		(5)
#define AFE_SINEGEN_CON1_TIMING_96K		(6)
#define AFE_SINEGEN_CON1_TIMING_192K		(7)
#define AFE_SINEGEN_CON1_TIMING_384K		(8)
#define AFE_SINEGEN_CON1_TIMING_7D35K		(16)
#define AFE_SINEGEN_CON1_TIMING_11D025K		(17)
#define AFE_SINEGEN_CON1_TIMING_14D7K		(18)
#define AFE_SINEGEN_CON1_TIMING_22D05K		(19)
#define AFE_SINEGEN_CON1_TIMING_29D4K		(20)
#define AFE_SINEGEN_CON1_TIMING_44D1K		(21)
#define AFE_SINEGEN_CON1_TIMING_88D2K		(22)
#define AFE_SINEGEN_CON1_TIMING_176D4K		(23)
#define AFE_SINEGEN_CON1_TIMING_352D8K		(24)
#define AFE_SINEGEN_CON1_TIMING_DL_1X_EN	(30)
#define AFE_SINEGEN_CON1_TIMING_SGEN_EN		(31)
#define AFE_SINEGEN_CON1_GASRC_IN_SGEN		BIT(13)
#define AFE_SINEGEN_CON1_GASRC_OUT_SGEN		BIT(12)

/* AFE_SINEGEN_CON2 (0x01f8) */
#define AFE_SINEGEN_CON2_MODE_MASK		GENMASK(31, 24)

/* A3_A4_TIMING_SEL0 (0x2440) */
#define AFE_A3A4_TIMING_SEL0_SGEN_CH2_MASK	GENMASK(5, 4)
#define AFE_A3A4_TIMING_SEL0_SGEN_CH1_MASK	GENMASK(3, 2)
#define AFE_A3A4_TIMING_SEL0_CH2(x)		(((x) & 0x3) << 4)
#define AFE_A3A4_TIMING_SEL0_CH1(x)		(((x) & 0x3) << 2)
#define AFE_A3A4_TIMING_SEL0_A1A2		(0)
#define AFE_A3A4_TIMING_SEL0_A3		(1)
#define AFE_A3A4_TIMING_SEL0_A4		(2)


/* AFE_SPDIF_OUT_CON0 (0x0380) */
#define AFE_SPDIF_OUT_CON0_TIMING_MASK		BIT(1)
#define AFE_SPDIF_OUT_CON0_TIMING_ON		(1 << 1)
#define AFE_SPDIF_OUT_CON0_TIMING_OFF		(0 << 1)

/* AFE_IEC_CFG (0x0480) */
#define AFE_IEC_CFG_SET_MASK			(0xff9e0073)
#define AFE_IEC_CFG_SW_RST_MASK			BIT(23)
#define AFE_IEC_CFG_FORCE_UPDATE_SIZE(x)	(((x) & 0xff) << 24)
#define AFE_IEC_CFG_FORCE_UPDATE		BIT(20)
#define AFE_IEC_CFG_SWAP_IEC_BYTE		BIT(17)
#define AFE_IEC_CFG_EN_MASK			BIT(16)
#define AFE_IEC_CFG_RAW_24BIT_SWITCH		BIT(6)
#define AFE_IEC_CFG_RAW_24BIT			BIT(5)
#define AFE_IEC_CFG_VALID_DATA			BIT(4)
#define AFE_IEC_CFG_NO_SW_RST			(1 << 23)
#define AFE_IEC_CFG_SW_RST			(0 << 23)
#define AFE_IEC_CFG_ENABLE_CTRL			(1 << 16)
#define AFE_IEC_CFG_DISABLE_CTRL		(0 << 16)
#define AFE_IEC_CFG_MUTE_DATA			(1 << 3)
#define AFE_IEC_CFG_UNMUTE_DATA			(0 << 3)
#define AFE_IEC_CFG_ENCODED_DATA		(1 << 1)
#define AFE_IEC_CFG_PCM_DATA			(0 << 1)
#define AFE_IEC_CFG_DATA_SRC_DRAM		(1 << 0)

/* AFE_IEC_NSNUM (0x0484) */
#define AFE_IEC_NSNUM_SET_MASK			(0x3fff3fff)
#define AFE_IEC_NSNUM_INTR_NUM(x)		(((x) & 0x3fff) << 16)
#define AFE_IEC_NSNUM_SAM_NUM(x)		(((x) & 0x3fff) << 0)

/* AFE_IEC_BURST_INFO (0x0488) */
#define AFE_IEC_BURST_INFO_READY_MASK		BIT(16)
#define AFE_IEC_BURST_INFO_NOT_READY		(1 << 16)
#define AFE_IEC_BURST_INFO_READY		(0 << 16)
#define AFE_IEC_BURST_INFO_SET_MASK		GENMASK(15, 0)

/* AFE_IEC_BURST_LEN (0x048c) */
#define AFE_IEC_BURST_LEN_SET_MASK		GENMASK(18, 0)

/* AFE_IEC_CHL_STAT0 (0x04a0)
 * AFE_IEC_CHL_STAT1 (0x04a4)
 * AFE_IEC_CHR_STAT0 (0x04a8)
 * AFE_IEC_CHR_STAT1 (0x04ac)
 */
#define AFE_IEC_CH_STAT0_SET_MASK		GENMASK(31, 0)
#define AFE_IEC_CH_STAT1_SET_MASK		GENMASK(15, 0)

/* AFE_SPDIFIN_CFG0 (0x0500) */
#define AFE_SPDIFIN_CFG0_SET_MASK		(0x3fffffa)
#define AFE_SPDIFIN_CFG0_MAX_LEN_NUM(x)		(((x) & 0xff) << 16)
#define AFE_SPDIFIN_CFG0_GMAT_BC_256_CYCLES	(3 << 24)
#define AFE_SPDIFIN_CFG0_DE_SEL_3_SAMPLES	(0 << 13)
#define AFE_SPDIFIN_CFG0_DE_SEL_14_SAMPLES	(1 << 13)
#define AFE_SPDIFIN_CFG0_DE_SEL_30_SAMPLES	(2 << 13)
#define AFE_SPDIFIN_CFG0_DE_SEL_CNT		(3 << 13)
#define AFE_SPDIFIN_CFG0_DE_CNT(x)		(((x) & 0x1f) << 8)
#define AFE_SPDIFIN_CFG0_TIMEOUT2IDLE_EN	BIT(7)
#define AFE_SPDIFIN_CFG0_INT_EN			BIT(6)
#define AFE_SPDIFIN_CFG0_DERR2IDLE_EN		(BIT(5)|BIT(4))
#define AFE_SPDIFIN_CFG0_DPERR2IDLE_EN		BIT(3)
#define AFE_SPDIFIN_CFG0_FLIP			BIT(1)
#define AFE_SPDIFIN_CFG0_EN			BIT(0)

/* AFE_SPDIFIN_CFG1 (0x0504) */
#define AFE_SPDIFIN_CFG1_SET_MASK		(0xfff10073)
#define AFE_SPDIFIN_CFG1_FIFOSTART_MASK		GENMASK(6, 4)
#define AFE_SPDIFIN_CFG1_CHSTS_INT		BIT(30)
#define AFE_SPDIFIN_CFG1_CHSTS_CHANGE_INT	BIT(29)
#define AFE_SPDIFIN_CFG1_TIMEOUT_INT		BIT(28)
#define AFE_SPDIFIN_CFG1_PREAMBLE_ERR		BIT(20)
#define AFE_SPDIFIN_CFG1_SEL_DEC0_CLK_EN	BIT(1)
#define AFE_SPDIFIN_CFG1_SEL_DEC0_DATA_EN	BIT(0)
#define AFE_SPDIFIN_CFG1_SEL_BCK_SPDIFIN	(0x1 << 16)
#define AFE_SPDIFIN_CFG1_FIFOSTART_5POINTS	(0x1 << 4)
#define AFE_SPDIFIN_CFG1_INT_BITS		(0x7ff << 20)
#define AFE_SPDIFIN_CFG1_SPDIF_IN_CLK_DATA_ENABLE 0x3
#define AFE_SPDIFIN_CFG1_MULTI_IN_CLK_DATA_ENABLE 0x0

/* AFE_SPDIFIN_DEBUG1 (0x0520) */
#define AFE_SPDIFIN_DEBUG1_DATALAT_ERR		BIT(10)
#define AFE_SPDIFIN_DEBUG1_CS_MASK		GENMASK(28, 24)

/* AFE_SPDIFIN_DEBUG2 (0x0524) */
#define AFE_SPDIFIN_DEBUG2_FIFO_ERR		(BIT(31)|BIT(30))
#define AFE_SPDIFIN_DEBUG2_CHSTS_INT_FLAG	BIT(26)
#define AFE_SPDIFIN_DEBUG2_PERR_9TIMES_FLAG	BIT(25)

/* AFE_SPDIFIN_DEBUG3 (0x0528) */
#define AFE_SPDIFIN_DEBUG3_ALL_ERR			GENMASK(6, 0)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_NON_STS		BIT(0)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_B_STS		BIT(1)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_M_STS		BIT(2)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_W_STS		BIT(3)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_BITCNT_STS		BIT(4)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_PARITY_STS		BIT(5)
#define AFE_SPDIFIN_DEBUG3_TIMEOUT_ERR_STS		BIT(6)
#define AFE_SPDIFIN_DEBUG3_CHSTS_PREAMPHASIS_STS	BIT(7)

/* AFE_SPDIFIN_EC (0x0530) */
#define AFE_SPDIFIN_EC_CLEAR_ALL		(0x30fff)
#define AFE_SPDIFIN_EC_PRE_ERR_CLEAR		BIT(0)
#define AFE_SPDIFIN_EC_PRE_ERR_B_CLEAR		BIT(1)
#define AFE_SPDIFIN_EC_PRE_ERR_M_CLEAR		BIT(2)
#define AFE_SPDIFIN_EC_PRE_ERR_W_CLEAR		BIT(3)
#define AFE_SPDIFIN_EC_PRE_ERR_BITCNT_CLEAR	BIT(4)
#define AFE_SPDIFIN_EC_PRE_ERR_PARITY_CLEAR	BIT(5)
#define AFE_SPDIFIN_EC_FIFO_ERR_CLEAR		(BIT(7)|BIT(6))
#define AFE_SPDIFIN_EC_TIMEOUT_INT_CLEAR	BIT(8)
#define AFE_SPDIFIN_EC_CHSTS_PREAMPHASIS_CLEAR	BIT(9)
#define AFE_SPDIFIN_EC_USECODE_COLLECTION_CLEAR	BIT(10)
#define AFE_SPDIFIN_EC_CHSTS_COLLECTION_CLEAR	BIT(11)
#define AFE_SPDIFIN_EC_DATA_LRCK_CHANGE_CLEAR	BIT(16)
#define AFE_SPDIFIN_EC_DATA_LATCH_CLEAR		BIT(17)

/* AFE_SPDIFIN_INT_EXT (0x0548) */
#define AFE_SPDIFIN_INT_EXT_INPUT_SEL_MASK	GENMASK(15, 14)
#define AFE_SPDIFIN_INT_EXT_SET_MASK		(0xeff00)
#define AFE_SPDIFIN_INT_EXT_DERR_NEW_RETEN	BIT(19)
#define AFE_SPDIFIN_INT_EXT_DATALAT_ERR_EN	BIT(17)
#define AFE_SPDIFIN_INT_EXT_SEL_OPTICAL		(0 << 14)
#define AFE_SPDIFIN_INT_EXT_SEL_COAXIAL		(1 << 14)
#define AFE_SPDIFIN_INT_EXT_SEL_ARC		(2 << 14)
#define AFE_SPDIFIN_INT_EXT_SEL_TIED_LOW	(3 << 14)

/* AFE_SPDIFIN_INT_EXT2 (0x054c) */
#define AFE_SPDIFIN_INT_EXT2_ROUGH_FS_MASK	GENMASK(31, 28)
#define AFE_SPDIFIN_INT_EXT2_FS_NOT_DEFINED	(0 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_32K		(1 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_44D1K		(2 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_48K		(3 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_64K		(4 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_88D2K		(5 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_96K		(6 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_128K		(7 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_144K		(8 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_176D4K		(9 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_192K		(10 << 28)
#define AFE_SPDIFIN_INT_EXT2_LRCK_CHANGE	BIT(27)
#define SPDIFIN_594MODE_MASK			(1 << 17)
#define SPDIFIN_594MODE_EN			(1 << 17)

/* ASYS_TOP_CON (0x0600) */
#define ASYS_TCON_A2SYS_TIMING_ON		BIT(1)
#define ASYS_TCON_A1SYS_TIMING_ON		BIT(0)
#if 0
#define ASYS_TCON_O34_O41_1X_EN_MASK		BIT(15)
#define ASYS_TCON_O34_O41_1X_EN_UL9		(0 << 15)
#define ASYS_TCON_O34_O41_1X_EN_UL2		(1 << 15)
#define ASYS_TCON_O26_O33_1X_EN_MASK		BIT(14)
#define ASYS_TCON_O26_O33_1X_EN_UL9		(0 << 14)
#define ASYS_TCON_O26_O33_1X_EN_UL2		(1 << 14)
#endif
#define ASYS_TCON_UL8_USE_SINEGEN		BIT(8)
#define ASYS_TCON_A4SYS_TIMING_ON		BIT(5)
#define ASYS_TCON_A3SYS_TIMING_ON		BIT(4)
#define ASYS_TCON_LP_MOD_ON		BIT(3)
#define ASYS_TCON_LP_26M_ENGEN_ON		BIT(2)


/* PWR2_TOP_CON0 (0x0634) */
#define PWR2_TOP_CON_PDN_DMIC0			BIT(0)
#define PWR2_TOP_CON_PDN_DMIC1			BIT(1)
#define PWR2_TOP_CON_PDN_DMIC2			BIT(2)
#define PWR2_TOP_CON_PDN_DMIC3			BIT(3)
#define PWR2_TOP_CON_DMIC8_SRC_SEL_MASK		GENMASK(31, 29)
#define PWR2_TOP_CON_DMIC7_SRC_SEL_MASK		GENMASK(28, 26)
#define PWR2_TOP_CON_DMIC6_SRC_SEL_MASK		GENMASK(25, 23)
#define PWR2_TOP_CON_DMIC5_SRC_SEL_MASK		GENMASK(22, 20)
#define PWR2_TOP_CON_DMIC4_SRC_SEL_MASK		GENMASK(19, 17)
#define PWR2_TOP_CON_DMIC3_SRC_SEL_MASK		GENMASK(16, 14)
#define PWR2_TOP_CON_DMIC2_SRC_SEL_MASK		GENMASK(13, 11)
#define PWR2_TOP_CON_DMIC1_SRC_SEL_MASK		GENMASK(10, 8)
#define PWR2_TOP_CON_DMIC8_SRC_SEL_VAL(x)	(x << 29)
#define PWR2_TOP_CON_DMIC7_SRC_SEL_VAL(x)	(x << 26)
#define PWR2_TOP_CON_DMIC6_SRC_SEL_VAL(x)	(x << 23)
#define PWR2_TOP_CON_DMIC5_SRC_SEL_VAL(x)	(x << 20)
#define PWR2_TOP_CON_DMIC4_SRC_SEL_VAL(x)	(x << 17)
#define PWR2_TOP_CON_DMIC3_SRC_SEL_VAL(x)	(x << 14)
#define PWR2_TOP_CON_DMIC2_SRC_SEL_VAL(x)	(x << 11)
#define PWR2_TOP_CON_DMIC1_SRC_SEL_VAL(x)	(x << 8)

/* PWR2_TOP_CON1 (0x0638) */
#define PWR2_TOP_CON1_DMIC_PDM_INTF_ON		BIT(1)


/* PCM_INTF_CON1 (0x063c) */
#define PCM_INTF_CON1_16BIT			(0 << 16)
#define PCM_INTF_CON1_24BIT			(1 << 16)
#define PCM_INTF_CON1_32BCK			(0 << 14)
#define PCM_INTF_CON1_64BCK			(1 << 14)
#define PCM_INTF_CON1_MASTER_MODE		(0 << 5)
#define PCM_INTF_CON1_SLAVE_MODE		(1 << 5)
#define PCM_INTF_CON1_FS_8K			(0 << 3)
#define PCM_INTF_CON1_FS_16K			(1 << 3)
#define PCM_INTF_CON1_FS_32K			(2 << 3)
#define PCM_INTF_CON1_FS_48K			(3 << 3)
#define PCM_INTF_CON1_SYNC_LEN(x)		(((x) - 1) << 9)
#define PCM_INTF_CON1_FORMAT(x)			((x) << 1)
#define PCM_INTF_CON1_SYNC_OUT_INV		BIT(23)
#define PCM_INTF_CON1_BCLK_OUT_INV		BIT(22)
#define PCM_INTF_CON1_SYNC_IN_INV		BIT(21)
#define PCM_INTF_CON1_BCLK_IN_INV		BIT(20)
#define PCM_INTF_CON1_BYPASS_ASRC		BIT(6)
#define PCM_INTF_CON1_EN			BIT(0)
#define PCM_INTF_CON1_CONFIG_MASK		(0xf1fffe)

/* PCM_INTF_CON2 (0x0640) */
#define PCM_INTF_CON2_LPBK_EN			BIT(8)

/* AFE_MPHONE_MULTI_MON (0x06B0)*/
#define AFE_MPHONE_MULTI_MON_DEC		BIT(10)
#define AFE_MPHONE_MULTI_MON_ROUGH		(0x3 << 8)
#define AFE_MPHONE_MULTI_MON_PC_B12_B5(x)	(((x) >> 11) & 0xff)
#define AFE_MPHONE_MULTI_MON_ROUGH_TYPE(x)	(((x) >> 8) & 0x3)
#define AFE_MPHONE_MULTI_MON_BS_NUM(x)		(((x) >> 5) & 0x7)
#define AFE_MPHONE_MULTI_MON_DATA_TYPE(x)	((x) & 0x1f)

/* AFE_MPHONE_MULTI_CON0 (0x06a4) */
#define AFE_MPHONE_MULTI_CON0_SET_MASK		(0x3ffc07e)
#define AFE_MPHONE_MULTI_CON0_SDATA3_SEL(x)	(((x) & 0x7) << 23)
#define AFE_MPHONE_MULTI_CON0_SDATA2_SEL(x)	(((x) & 0x7) << 20)
#define AFE_MPHONE_MULTI_CON0_SDATA1_SEL(x)	(((x) & 0x7) << 17)
#define AFE_MPHONE_MULTI_CON0_SDATA0_SEL(x)	(((x) & 0x7) << 14)
#define AFE_MPHONE_MULTI_CON0_256DWORD_PERIOD	(0x3 << 4)
#define AFE_MPHONE_MULTI_CON0_128DWORD_PERIOD	(0x2 << 4)
#define AFE_MPHONE_MULTI_CON0_64DWORD_PERIOD	(0x1 << 4)
#define AFE_MPHONE_MULTI_CON0_32DWORD_PERIOD	(0x0 << 4)
#define AFE_MPHONE_MULTI_CON0_16BIT_SWAP	BIT(3)
#define AFE_MPHONE_MULTI_CON0_24BIT_DATA	(0x1 << 1)
#define AFE_MPHONE_MULTI_CON0_16BIT_DATA	(0x0 << 1)
#define AFE_MPHONE_MULTI_CON0_EN		BIT(0)

/* AFE_MPHONE_MULTI_CON1 (0x06a8) */
#define AFE_MPHONE_MULTI_CON1_SET_MASK		(0xfffff6f)
#define AFE_MPHONE_MULTI_CON1_NO_RESET		BIT(27)
#define AFE_MPHONE_MULTI_CON1_DET_LONG_PERIOD	BIT(25)
#define AFE_MPHONE_MULTI_CON1_SYNC_ON		BIT(24)
#define AFE_MPHONE_MULTI_CON1_24BIT_SWAP_BYPASS	BIT(22)
#define AFE_MPHONE_MULTI_CON1_NON_COMPACT_MODE	(0x1 << 19)
#define AFE_MPHONE_MULTI_CON1_COMPACT_MODE	(0x0 << 19)
#define AFE_MPHONE_MULTI_CON1_HBR_MODE		BIT(18)
#define AFE_MPHONE_MULTI_CON1_LRCK_32_CYCLE	(0x2 << 16)
#define AFE_MPHONE_MULTI_CON1_LRCK_24_CYCLE	(0x1 << 16)
#define AFE_MPHONE_MULTI_CON1_LRCK_16_CYCLE	(0x0 << 16)
#define AFE_MPHONE_MULTI_CON1_LRCK_INV		BIT(15)
#define AFE_MPHONE_MULTI_CON1_DELAY_DATA	BIT(14)
#define AFE_MPHONE_MULTI_CON1_LEFT_ALIGN	BIT(13)
#define AFE_MPHONE_MULTI_CON1_BCK_INV		BIT(6)
#define AFE_MPHONE_MULTI_CON1_BIT_NUM(x)	((((x) - 1) & 0x1f) << 8)
#define AFE_MPHONE_MULTI_CON1_CH_NUM(x)		((((x) >> 1) - 1) & 0x3)

/* AFE_MPHONE_MULTI_CON2 (0x06ac) */
#define AFE_MPHONE_MULTI_CON2_SET_MASK		(0x80080000)
#define AFE_MPHONE_MULTI_CON2_SEL_SPDIFIN	BIT(19)

/* AFE_CONN76 (0x07f0) */
#define AFE_CONN76_I10_I11_SEL_MASK		GENMASK(31, 29)
#define AFE_CONN76_I18_I19_SEL_MASK		GENMASK(28, 27)
#define AFE_CONN76_I10_I11_SEL_DMIC		(0 << 29)
#define AFE_CONN76_I10_I11_SEL_AMIC		(1 << 29)
#define AFE_CONN76_I10_I11_SEL_ETDM_IN1		(2 << 29)
#define AFE_CONN76_I10_I11_SEL_AMIC_FIFO	(4 << 29)
#define AFE_CONN76_I18_I19_SEL_ETDM_IN2		(0 << 27)
#define AFE_CONN76_I18_I19_SEL_AMIC		(1 << 27)
#define AFE_CONN76_I18_I19_SEL_AMIC_FIFO	(2 << 27)

/* AFE_GASRC0_NEW_CON0 (0x0800)
 * AFE_GASRC1_NEW_CON0 (0x0840)
 * AFE_GASRC2_NEW_CON0 (0x0880)
 * AFE_GASRC3_NEW_CON0 (0x08c0)
 */
#define GASRC_NEW_CON0_ONE_HEART			BIT(31)
#define GASRC_NEW_CON0_CHSET0_CLR_IIR_HISTORY	BIT(17)
#define GASRC_NEW_CON0_CHSET0_OFS_SEL_MASK	GENMASK(15, 14)
#define GASRC_NEW_CON0_CHSET0_OFS_SEL_TX		(0 << 14)
#define GASRC_NEW_CON0_CHSET0_OFS_SEL_RX		(1 << 14)
#define GASRC_NEW_CON0_CHSET0_IFS_SEL_MASK	GENMASK(13, 12)
#define GASRC_NEW_CON0_CHSET0_IFS_SEL_TX		(3 << 12)
#define GASRC_NEW_CON0_CHSET0_IFS_SEL_RX		(2 << 12)
#define GASRC_NEW_CON0_CHSET0_IIR_EN		BIT(11)
#define GASRC_NEW_CON0_CHSET0_IIR_STAGE(x)	(((x) - 1) << 8)
#define GASRC_NEW_CON0_CHSET0_IIR_STAGE_MASK	GENMASK(10, 8)
#define GASRC_NEW_CON0_CHSET_STR_CLR		BIT(4)
#define GASRC_NEW_CON0_COEFF_SRAM_CTRL		BIT(1)
#define GASRC_NEW_CON0_ASM_ON				BIT(0)


#define GASRC_NEW_CON5_SOFT_RESET		BIT(0)


/* AFE_GASRC0_NEW_CON6 (0x0818)
 * AFE_GASRC1_NEW_CON6 (0x0858)
 * AFE_GASRC2_NEW_CON6 (0x0898)
 * AFE_GASRC3_NEW_CON6 (0x08d8)
 */
#define GASRC_NEW_CON6_FREQ_CALI_CYCLE_MASK	GENMASK(31, 16)
#define GASRC_NEW_CON6_FREQ_CALI_CYCLE(x)	(((x - 1) & 0xffff) << 16)
#define GASRC_NEW_CON6_AUTO_TUNE_FREQ3	BIT(12)
#define GASRC_NEW_CON6_COMP_FREQ_RES_EN	BIT(11)
#define GASRC_NEW_CON6_FREQ_CALI_BP_DGL	BIT(7)
#define GASRC_NEW_CON6_AUTO_TUNE_FREQ2	BIT(3)
#define GASRC_NEW_CON6_FREQ_CALI_AUTO_RESTART	BIT(2)
#define GASRC_NEW_CON6_CALI_USE_FREQ_OUT	BIT(1)
#define GASRC_NEW_CON6_CALI_EN				BIT(0)

/* AFE_GASRC0_NEW_CON7 (0x081c)
 * AFE_GASRC1_NEW_CON7 (0x085c)
 * AFE_GASRC2_NEW_CON7 (0x089c)
 * AFE_GASRC3_NEW_CON7 (0x08dc)
 */
#define GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_MASK	GENMASK(23, 0)
#define GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_49M	(0x3C00)
#define GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_45M	(0x3720)

/* AFE_GASRC0_NEW_CON13 (0x0834)
 * AFE_GASRC0_NEW_CON14 (0x0838)
 * AFE_GASRC1_NEW_CON13 (0x0874)
 * AFE_GASRC1_NEW_CON14 (0x0878)
 * AFE_GASRC2_NEW_CON13 (0x08b4)
 * AFE_GASRC2_NEW_CON14 (0x08b8)
 * AFE_GASRC3_NEW_CON13 (0x08f4)
 * AFE_GASRC3_NEW_CON14 (0x08f8)
 */
#define GASRC_NEW_CON_FREQ_CALI_AUTORST_TH_MASK	GENMASK(23, 0)

/* AFE_DMIC0_UL_SRC_CON0  (0x1A00)
 * AFE_DMIC1_UL_SRC_CON0 (0x1A68)
 * AFE_DMIC2_UL_SRC_CON0 (0x1AD0)
 * AFE_DMIC3_UL_SRC_CON0 (0x1B38)
 */

#define DMIC_UL_CON0_SRC_ON_TMP_CTL		BIT(0)
#define DMIC_UL_CON0_SDM_3_LEVEL_CTL		BIT(1)
#define DMIC_UL_CON0_LOOPBACK_MODE_CTL		BIT(2)
#define DMIC_UL_CON0_3P25M_1P625M_SEL(x)	((x) << 5)
#define DMIC_UL_CON0_IIR_MODE_SEL(x)		((x) << 7)
#define DMIC_UL_CON0_IIR_ON_TMP_CTL		BIT(10)
#define DMIC_UL_CON0_DISABLE_HW_CG_CTL		BIT(12)
#define DMIC_UL_CON0_LOW_POWER_MODE_SEL(x)	((x) << 14)
#define DMIC_UL_CON0_VOCIE_MODE_8K		(0 << 17)
#define DMIC_UL_CON0_VOCIE_MODE_16K		(1 << 17)
#define DMIC_UL_CON0_VOCIE_MODE_32K		(2 << 17)
#define DMIC_UL_CON0_VOCIE_MODE_48K		(3 << 17)
#define DMIC_UL_CON0_MODE_3P25M_CH1_CTL		BIT(21)
#define DMIC_UL_CON0_MODE_3P25M_CH2_CTL		BIT(22)
#define DMIC_UL_CON0_TWO_WIRE_MODE_CTL		BIT(23)
#define DMIC_UL_CON0_PHASE_SEL_CH2(x)		((x) << 24)
#define DMIC_UL_CON0_PHASE_SEL_CH1(x)		((x) << 27)
#define DMIC_UL_CON0_ULCF_CFG_EN_CTL		BIT(31)
#define DMIC_UL_CON0_CONFIG_MASK		(0xBF8ED7A6)

/* AFE_DMIC0_UL_SRC_CON1  (0x1A04)
 * AFE_DMIC1_UL_SRC_CON1 (0x1A6C)
 * AFE_DMIC2_UL_SRC_CON1 (0x1AD4)
 * AFE_DMIC3_UL_SRC_CON1 (0x1B3C)
 */

#define DMIC_UL_CON1_SGEN_EN			BIT(27)
#define DMIC_UL_CON1_SGEN_MUTE			BIT(26)
#define DMIC_UL_CON1_TRIANGULAR_TONE		BIT(25)
#define DMIC_UL_CON1_SGEN_CH2_AMP_DIV(x)	((x) << 21)
#define DMIC_UL_CON1_SGEN_CH2_FREQ_DIV(x)	((x) << 16)
#define DMIC_UL_CON1_SGEN_CH2_SINE_MODE(x)	((x) << 12)
#define DMIC_UL_CON1_SGEN_CH1_AMP_DIV(x)	((x) << 9)
#define DMIC_UL_CON1_SGEN_CH1_FREQ_DIV(x)	((x) << 4)
#define DMIC_UL_CON1_SGEN_CH1_SINE_MODE(x)	((x) << 0)


/* ETDM_COWORK_CON0 (0x22f0) */
#define ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_MASK		GENMASK(19, 16)
#define ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_IN1		(0x2 << 16)
#define ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_IN2		(0x4 << 16)
#define ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_OUT2		(0xc << 16)
#define ETDM_COWORK_CON0_TDM_OUT1_SYNC_SEL_OUT3		(0xe << 16)

/* ETDM_COWORK_CON1 (0x22f4) */
#define ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_MASK		GENMASK(11, 8)
#define ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_SELF		(0x3 << 8)
#define ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT1_MAS	(0xa << 8)
#define ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT2_MAS	(0xc << 8)
#define ETDM_COWORK_CON1_TDM_IN1_SLV_SEL_OUT3_MAS	(0xe << 8)
#define ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_MASK		GENMASK(15, 12)
#define ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_IN2		(0x4 << 12)
#define ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_OUT1		(0xa << 12)
#define ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_OUT2		(0xc << 12)
#define ETDM_COWORK_CON1_TDM_IN1_SYNC_SEL_OUT3		(0xe << 12)
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_MASK		GENMASK(19, 16)
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_PAD		(0x2 << 16)
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT1		(0xa << 16)
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT2		(0xc << 16)
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT3		(0xe << 16)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_MASK	GENMASK(23, 20)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_PAD		(0x2 << 20)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_OUT1	(0xa << 20)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_OUT2	(0xc << 20)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_x_SEL_OUT3	(0xe << 20)

/* ETDM_COWORK_CON2 (0x22f8) */
#define ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_MASK		GENMASK(7, 4)
#define ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_IN1		(0x2 << 4)
#define ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_IN2		(0x4 << 4)
#define ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_OUT1		(0xa << 4)
#define ETDM_COWORK_CON2_TDM_OUT2_SYNC_SEL_OUT3		(0xe << 4)
#define ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_MASK		GENMASK(19, 16)
#define ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_IN1		(0x2 << 16)
#define ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_IN2		(0x4 << 16)
#define ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_OUT1		(0xa << 16)
#define ETDM_COWORK_CON2_TDM_OUT3_SYNC_SEL_OUT2		(0xc << 16)
#define ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_MASK		GENMASK(27, 24)
#define ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_SELF		(0x5 << 24)
#define ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT1_MAS	(0xa << 24)
#define ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT2_MAS	(0xc << 24)
#define ETDM_COWORK_CON2_TDM_IN2_SLV_SEL_OUT3_MAS	(0xe << 24)
#define ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_MASK		GENMASK(31, 28)
#define ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_IN1		(0x2 << 28)
#define ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_OUT1		(0xa << 28)
#define ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_OUT2		(0xc << 28)
#define ETDM_COWORK_CON2_TDM_IN2_SYNC_SEL_OUT3		(0xe << 28)


/* ETDM_COWORK_CON3 (0x22fc) */
#define ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_MASK		GENMASK(3, 0)
#define ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_PAD		(0x4)
#define ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT1		(0xa)
#define ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT2		(0xc)
#define ETDM_COWORK_CON3_TDM_IN2_DAT0_SEL_OUT3		(0xe)
#define ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_MASK	GENMASK(7, 4)
#define ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_PAD			(0x4 << 4)
#define ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_OUT1		(0xa << 4)
#define ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_OUT2		(0xc << 4)
#define ETDM_COWORK_CON3_TDM_IN2_DAT1_x_SEL_OUT3		(0xe << 4)
#define ETDM_COWORK_CON3_OUT1_USE_SINEGEN	BIT(29)
#define ETDM_COWORK_CON3_OUT2_USE_SINEGEN	BIT(30)
#define ETDM_COWORK_CON3_OUT3_USE_SINEGEN	BIT(31)


//#define UL_REORDER_START_DATA(x)		(((x) & 0xf) << 8)
#define UL_REORDER_NO_BYPASS			BIT(27)
#define UL_REORDER_EN				BIT(0)
// a1sys_hoping_ck/fs/(N+1) > (ch/2), * N+1 < 26m/384k/(32/2)=4 N<3
#define UL_REORDER_UPDATE_CNT(x)		(((x) & 0x3ff) << 16)
#define UL_REORDER_CHANNEL(x)			((((x) & 0x1f) - 1) << 3)
#define UL_REORDER_CTRL_MASK			(0x1bff00fa)

#define ETDM_CON0_CH_NUM(x)			(((x) - 1) << 23)
#define ETDM_CON0_WORD_LEN(x)			(((x) - 1) << 16)
#define ETDM_CON0_BIT_LEN(x)			(((x) - 1) << 11)
#define ETDM_CON0_FORMAT(x)			((x) << 6)
#define ETDM_CON0_SLAVE_MODE			BIT(5)
#define ETDM_CON0_SYNC_MODE			BIT(1)

#define ETDM_CON1_MCLK_OUTPUT			BIT(16)
#define ETDM_CON1_LRCK_MANUAL_MODE		(0 << 29)
#define ETDM_CON1_LRCK_AUTO_MODE		(1 << 29)
#define ETDM_CON1_BCK_FROM_DIVIDER		BIT(30)


#define ETDM_CON4_ASYNC_RESET			BIT(11)

#define ETDM_IN_CON0_CTRL_MASK			(0x3f9ff9e2)
#define ETDM_IN_CON1_CTRL_MASK			(0xfff10000)
#define ETDM_IN_CON2_CTRL_MASK			(0x840f9c1f)
#define ETDM_IN_CON3_CTRL_MASK			(0x7c00ffff)
#define ETDM_IN_CON4_CTRL_MASK			(0x01ff0000)
#define ETDM_IN_CON5_CTRL_MASK			(0xffffffff)

#define ETDM_IN_CON0_REL_APLL_SEL(x)			(((x) & 0x3) << 28)
#define ETDM_IN_CON1_LRCK_WIDTH(x)		(((x) - 1) << 20)
#define ETDM_IN_CON1_LRCK_MANUAL_MODE		(0 << 31)
#define ETDM_IN_CON1_LRCK_AUTO_MODE		(1 << 31)

#define ETDM_IN_CON2_MULTI_IP_TOTAL_CH(x)	(((x) - 1) << 15)
#define ETDM_IN_CON2_MULTI_IP_2CH_MODE		BIT(31)
#define ETDM_IN_CON2_MULTI_IP_ONE_DATA		BIT(15)
#define ETDM_IN_CON2_UPDATE_POINT_AUTO_DIS	(0 << 26)
#define ETDM_IN_CON2_UPDATE_POINT_AUTO_EN	(1 << 26)
#define ETDM_IN_CON2_UPDATE_POINT(x)		((x) & 0x1f)
#define ETDM_IN_CON2_APLL_SEL(x)			(((x) & 0x7) << 10)

#define ETDM_IN_CON3_FS(x)			(((x) & 0x1f) << 26)
#define ETDM_IN_CON3_DISABLE_OUT(x)		BIT((x) & 0xffff)

#define ETDM_IN_CON4_MASTER_LRCK_INV		BIT(19)  //etdmin con4
#define ETDM_IN_CON4_MASTER_BCK_INV		BIT(18)
#define ETDM_IN_CON4_SLAVE_LRCK_INV		BIT(17)
#define ETDM_IN_CON4_SLAVE_BCK_INV			BIT(16)
#define ETDM_IN_CON4_CONN_FS(x)		(((x) & 0x1f) << 20)

#define ETDM_IN_CON5_ENABLE_ODD(x)		BIT((x) & 0xffff)
#define ETDM_IN_CON5_LR_SWAP(x)			BIT(((x) & 0xffff) + 16)

///
#define ETDM_OUT_CON0_CTRL_MASK			(0x3f9ff9e2)
#define ETDM_OUT_CON1_CTRL_MASK			(0x7ff90000)
#define ETDM_OUT_CON4_CTRL_MASK			(0x1f0001df)
#define ETDM_OUT_CON5_CTRL_MASK			(0x7c0)

#define ETDM_OUT_CON0_REL_APLL_SEL(x)			(((x) & 0x3) << 28)

#define ETDM_OUT_CON1_LRCK_WIDTH(x)		(((x) - 1) << 19)

#define ETDM_OUT_CON4_FS(x)			(((x) & 0x1f) << 0)
#define ETDM_OUT_CON4_APLL_SEL(x)			(((x) & 0x7) << 6)
#define ETDM_OUT_CON4_CONN_FS(x)		(((x) & 0x1f) << 24)


#define ETDM_OUT_CON5_MASTER_LRCK_INV		BIT(10)  //out con5
#define ETDM_OUT_CON5_MASTER_BCK_INV		BIT(9)
#define ETDM_OUT_CON5_SLAVE_LRCK_INV		BIT(8)
#define ETDM_OUT_CON5_SLAVE_BCK_INV			BIT(7)


/* ETDM_OUT2_CON4 (0x23b0) */
#define ETDM_OUT_CON4_INTERCONN_EN_SEL_MASK	(0x1f000000)


/*AFE_GASRCx_NEW_CON5*/
#define GASRC_CON5_LRCK_SEL_MASK	GENMASK(3, 1)
#define GASRC_CON5_LRCK_SEL(x)	(((x) & 0x7) << 1)

/*AFE_GASRCx_NEW_CON6*/
#define GASRC_CON6_USE_SEL_MASK	GENMASK(9, 8)
#define GASRC_CON6_USE_SEL(x)	(((x) & 0x3) << 8)


/* GASRC_CFG0 (0x2400) */
#define GASRC_CFG0_GASRC0_SOFT_RST		BIT(0)
#define GASRC_CFG0_GASRC1_SOFT_RST		BIT(8)
#define GASRC_CFG0_GASRC2_SOFT_RST		BIT(16)
#define GASRC_CFG0_GASRC3_SOFT_RST		BIT(24)
#define GASRC_CFG0_GASRC0_LRCK_SEL_MASK	GENMASK(6, 4)
#define GASRC_CFG0_GASRC0_LRCK_SEL(x)	(((x) & 0x7) << 4)
#define GASRC_CFG0_GASRC1_LRCK_SEL_MASK	GENMASK(14, 12)
#define GASRC_CFG0_GASRC1_LRCK_SEL(x)	(((x) & 0x7) << 12)
#define GASRC_CFG0_GASRC2_LRCK_SEL_MASK	GENMASK(22, 20)
#define GASRC_CFG0_GASRC2_LRCK_SEL(x)	(((x) & 0x7) << 20)
#define GASRC_CFG0_GASRC3_LRCK_SEL_MASK	GENMASK(30, 28)
#define GASRC_CFG0_GASRC3_LRCK_SEL(x)	(((x) & 0x7) << 28)
#define GASRC_CFG0_GASRC0_USE_SEL_MASK	BIT(1)
#define GASRC_CFG0_GASRC0_USE_SEL(x)	((x) << 1)
#define GASRC_CFG0_GASRC1_USE_SEL_MASK	BIT(9)
#define GASRC_CFG0_GASRC1_USE_SEL(x)	((x) << 9)
#define GASRC_CFG0_GASRC2_USE_SEL_MASK	BIT(17)
#define GASRC_CFG0_GASRC2_USE_SEL(x)	((x) << 17)
#define GASRC_CFG0_GASRC3_USE_SEL_MASK	BIT(25)
#define GASRC_CFG0_GASRC3_USE_SEL(x)	((x) << 25)

#if 0
/* GASRC_TIMING_CON0 (0x2408) */
#define GASRC_TIMING_CON0_GASRC0_IN_MODE(x)	(((x) & 0x1f) << 0)
#define GASRC_TIMING_CON0_GASRC1_IN_MODE(x)	(((x) & 0x1f) << 5)
#define GASRC_TIMING_CON0_GASRC2_IN_MODE(x)	(((x) & 0x1f) << 10)
#define GASRC_TIMING_CON0_GASRC3_IN_MODE(x)	(((x) & 0x1f) << 15)
#define GASRC_TIMING_CON0_GASRC0_IN_MODE_MASK	GENMASK(4, 0)
#define GASRC_TIMING_CON0_GASRC1_IN_MODE_MASK	GENMASK(9, 5)
#define GASRC_TIMING_CON0_GASRC2_IN_MODE_MASK	GENMASK(14, 10)
#define GASRC_TIMING_CON0_GASRC3_IN_MODE_MASK	GENMASK(19, 15)

/* GASRC_TIMING_CON1 (0x240c) */
#define GASRC_TIMING_CON1_GASRC0_OUT_MODE(x)	(((x) & 0x1f) << 0)
#define GASRC_TIMING_CON1_GASRC1_OUT_MODE(x)	(((x) & 0x1f) << 5)
#define GASRC_TIMING_CON1_GASRC2_OUT_MODE(x)	(((x) & 0x1f) << 10)
#define GASRC_TIMING_CON1_GASRC3_OUT_MODE(x)	(((x) & 0x1f) << 15)
#define GASRC_TIMING_CON1_GASRC0_OUT_MODE_MASK	GENMASK(4, 0)
#define GASRC_TIMING_CON1_GASRC1_OUT_MODE_MASK	GENMASK(9, 5)
#define GASRC_TIMING_CON1_GASRC2_OUT_MODE_MASK	GENMASK(14, 10)
#define GASRC_TIMING_CON1_GASRC3_OUT_MODE_MASK	GENMASK(19, 15)
#endif

#define BLOCK_DPIDLE_REG 0xb6c
#define BLOCK_DPIDLE_REG_BIT 31
#define BLOCK_DPIDLE_REG_MASK (1 << BLOCK_DPIDLE_REG_BIT)
#define BLOCK_DPIDLE_REG_BIT_ON  (1 << BLOCK_DPIDLE_REG_BIT)
#define BLOCK_DPIDLE_REG_BIT_OFF (0 << BLOCK_DPIDLE_REG_BIT)

#endif
