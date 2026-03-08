/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __VDO1_MDP_RDMA0_REGS_H__
#define __VDO1_MDP_RDMA0_REGS_H__

// ---------------------------------------------------------------------------
//  Register Field Access
// ---------------------------------------------------------------------------
#if !defined(REG_FLD)
#define REG_FLD(width, shift) \
	((unsigned int)((((width) & 0xFF) << 16) | ((shift) & 0xFF)))
#endif
#if !defined(REG_FLD_WIDTH)
#define REG_FLD_WIDTH(field) \
	((unsigned int)(((field) >> 16) & 0xFF))
#endif
#if !defined(REG_FLD_SHIFT)
#define REG_FLD_SHIFT(field) \
	((unsigned int)((field) & 0xFF))
#endif
#if !defined(REG_FLD_MASK)
#define REG_FLD_MASK(field) \
((REG_FLD_WIDTH(field) == 32) \
? (0xFFFFFFFF) \
: ((((unsigned int)(1 << REG_FLD_WIDTH(field)) - 1) << REG_FLD_SHIFT(field))))
#endif

#if !defined(REG_FLD_VAL)
#define REG_FLD_VAL(field, val) \
	(((val) << REG_FLD_SHIFT(field)) & REG_FLD_MASK(field))
#endif

//#define REG_FLD_GET(field, reg32)

//#define REG_FLD_SET(field, reg32, val)

// ---------- VDO1_MDP_RDMA0 C Macro Definitions   ----------
//extern PVDO1_MDP_RDMA0_REGS g_VDO1_MDP_RDMA0_BASE;

#define VDO1_MDP_RDMA0_BASE                                    (0x1C104000)
#define VDO1_MDP_RDMA1_BASE                                    (0x1C105000)
#define VDO1_MDP_RDMA2_BASE                                    (0x1C106000)
#define VDO1_MDP_RDMA3_BASE                                    (0x1C107000)
#define VDO1_MDP_RDMA4_BASE                                    (0x1C108000)
#define VDO1_MDP_RDMA5_BASE                                    (0x1C109000)
#define VDO1_MDP_RDMA6_BASE                                    (0x1C10A000)
#define VDO1_MDP_RDMA7_BASE                                    (0x1C10B000)

#define MDP_RDMA_EN                                            0x000
#define MDP_RDMA_UFBDC_DCM_EN                                  0x004
#define MDP_RDMA_RESET                                         0x008
#define MDP_RDMA_INTERRUPT_ENABLE                              0x010
#define MDP_RDMA_INTERRUPT_STATUS                              0x018
#define MDP_RDMA_CON                                           0x020
#define MDP_RDMA_SHADOW_CTRL                                   0x024
#define MDP_RDMA_GMCIF_CON                                     0x028
#define MDP_RDMA_SRC_CON                                       0x030
#define MDP_RDMA_COMP_CON                                      0x038
#define MDP_RDMA_PVRIC_CRYUVAL_0                               0x040
#define MDP_RDMA_PVRIC_CRYUVAL_1                               0x048
#define MDP_RDMA_PVRIC_CRCH0123VAL_0                           0x050
#define MDP_RDMA_PVRIC_CRCH0123VAL_1                           0x058
#define MDP_RDMA_MF_BKGD_SIZE_IN_BYTE                          0x060
#define MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL                         0x068
#define MDP_RDMA_MF_SRC_SIZE                                   0x070
#define MDP_RDMA_MF_CLIP_SIZE                                  0x078
#define MDP_RDMA_MF_OFFSET_1                                   0x080
#define MDP_RDMA_MF_PAR                                        0x088
#define MDP_RDMA_SF_BKGD_SIZE_IN_BYTE                          0x090
#define MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL                       0x098
#define MDP_RDMA_TARGET_LINE                                   0x0A0
#define MDP_RDMA_SF_PAR                                        0x0B8
#define MDP_RDMA_MB_DEPTH                                      0x0C0
#define MDP_RDMA_MB_BASE                                       0x0C8
#define MDP_RDMA_MB_CON                                        0x0D0
#define MDP_RDMA_SB_DEPTH                                      0x0D8
#define MDP_RDMA_SB_BASE                                       0x0E0
#define MDP_RDMA_SB_CON                                        0x0E8
#define MDP_RDMA_VC1_RANGE                                     0x0F0
#define MDP_RDMA_SRC_END_0                                     0x100
#define MDP_RDMA_SRC_END_1                                     0x108
#define MDP_RDMA_SRC_END_2                                     0x110
#define MDP_RDMA_SRC_OFFSET_0                                  0x118
#define MDP_RDMA_SRC_OFFSET_1                                  0x120
#define MDP_RDMA_SRC_OFFSET_2                                  0x128
#define MDP_RDMA_SRC_OFFSET_W_0                                0x130
#define MDP_RDMA_SRC_OFFSET_W_1                                0x138
#define MDP_RDMA_SRC_OFFSET_W_2                                0x140
#define MDP_RDMA_SRC_OFFSET_WP                                 0x148
#define MDP_RDMA_SRC_OFFSET_HP                                 0x150
#define MDP_RDMA_C00                                           0x154
#define MDP_RDMA_C01                                           0x158
#define MDP_RDMA_C02                                           0x15C
#define MDP_RDMA_C10                                           0x160
#define MDP_RDMA_C11                                           0x164
#define MDP_RDMA_C12                                           0x168
#define MDP_RDMA_C20                                           0x16C
#define MDP_RDMA_C21                                           0x170
#define MDP_RDMA_C22                                           0x174
#define MDP_RDMA_PRE_ADD_0                                     0x178
#define MDP_RDMA_PRE_ADD_1                                     0x17C
#define MDP_RDMA_PRE_ADD_2                                     0x180
#define MDP_RDMA_POST_ADD_0                                    0x184
#define MDP_RDMA_POST_ADD_1                                    0x188
#define MDP_RDMA_POST_ADD_2                                    0x18C
#define MDP_RDMA_TRANSFORM_0                                   0x200
#define MDP_RDMA_DMABUF_CON_0                                  0x240
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0                           0x248
#define MDP_RDMA_ULTRA_TH_LOW_CON_0                            0x250
#define MDP_RDMA_DMABUF_CON_1                                  0x258
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1                           0x260
#define MDP_RDMA_ULTRA_TH_LOW_CON_1                            0x268
#define MDP_RDMA_DMABUF_CON_2                                  0x270
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2                           0x278
#define MDP_RDMA_ULTRA_TH_LOW_CON_2                            0x280
#define MDP_RDMA_DMABUF_CON_3                                  0x288
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3                           0x290
#define MDP_RDMA_ULTRA_TH_LOW_CON_3                            0x298
#define MDP_RDMA_DITHER_CON                                    0x2A0
#define MDP_RDMA_RESV_DUMMY_0                                  0x2A8
#define MDP_RDMA_UNCOMP_MON                                    0x2C0
#define MDP_RDMA_COMP_MON                                      0x2C8
#define MDP_RDMA_CHKS_EXTR                                     0x300
#define MDP_RDMA_CHKS_INTW                                     0x308
#define MDP_RDMA_CHKS_INTR                                     0x310
#define MDP_RDMA_CHKS_ROTO                                     0x318
#define MDP_RDMA_CHKS_SRIY                                     0x320
#define MDP_RDMA_CHKS_SRIU                                     0x328
#define MDP_RDMA_CHKS_SRIV                                     0x330
#define MDP_RDMA_CHKS_SROY                                     0x338
#define MDP_RDMA_CHKS_SROU                                     0x340
#define MDP_RDMA_CHKS_SROV                                     0x348
#define MDP_RDMA_CHKS_VUPI                                     0x350
#define MDP_RDMA_CHKS_VUPO                                     0x358
#define MDP_RDMA_DEBUG_CON                                     0x380
#define MDP_RDMA_MON_STA_0                                     0x400
#define MDP_RDMA_MON_STA_1                                     0x408
#define MDP_RDMA_MON_STA_2                                     0x410
#define MDP_RDMA_MON_STA_3                                     0x418
#define MDP_RDMA_MON_STA_4                                     0x420
#define MDP_RDMA_MON_STA_5                                     0x428
#define MDP_RDMA_MON_STA_6                                     0x430
#define MDP_RDMA_MON_STA_7                                     0x438
#define MDP_RDMA_MON_STA_8                                     0x440
#define MDP_RDMA_MON_STA_9                                     0x448
#define MDP_RDMA_MON_STA_10                                    0x450
#define MDP_RDMA_MON_STA_11                                    0x458
#define MDP_RDMA_MON_STA_12                                    0x460
#define MDP_RDMA_MON_STA_13                                    0x468
#define MDP_RDMA_MON_STA_14                                    0x470
#define MDP_RDMA_MON_STA_15                                    0x478
#define MDP_RDMA_MON_STA_16                                    0x480
#define MDP_RDMA_MON_STA_17                                    0x488
#define MDP_RDMA_MON_STA_18                                    0x490
#define MDP_RDMA_MON_STA_19                                    0x498
#define MDP_RDMA_MON_STA_20                                    0x4A0
#define MDP_RDMA_MON_STA_21                                    0x4A8
#define MDP_RDMA_MON_STA_22                                    0x4B0
#define MDP_RDMA_MON_STA_23                                    0x4B8
#define MDP_RDMA_MON_STA_24                                    0x4C0
#define MDP_RDMA_MON_STA_25                                    0x4C8
#define MDP_RDMA_MON_STA_26                                    0x4D0
#define MDP_RDMA_MON_STA_27                                    0x4D8
#define MDP_RDMA_MON_STA_28                                    0x4E0
#define MDP_RDMA_SRC_BASE_0                                    0xF00
#define MDP_RDMA_SRC_BASE_1                                    0xF08
#define MDP_RDMA_SRC_BASE_2                                    0xF10
#define MDP_RDMA_UFO_DEC_LENGTH_BASE_Y                         0xF20
#define MDP_RDMA_UFO_DEC_LENGTH_BASE_C                         0xF28



#define MDP_RDMA_EN_FLD_INTERNAL_DCM_EN                        REG_FLD(20, 4)
#define MDP_RDMA_EN_FLD_ROT_ENABLE                             REG_FLD(1, 0)

#define MDP_RDMA_UFBDC_DCM_EN_FLD_UFBDC_INTERNAL_DCM_EN        REG_FLD(32, 0)

#define MDP_RDMA_RESET_FLD_WARM_RESET                          REG_FLD(1, 0)

#define MDP_RDMA_INTERRUPT_ENABLE_FLD_UNDERRUN_INT_EN          REG_FLD(1, 2)
#define MDP_RDMA_INTERRUPT_ENABLE_FLD_REG_UPDATE_INT_EN        REG_FLD(1, 1)
#define MDP_RDMA_INTERRUPT_ENABLE_FLD_FRAME_COMPLETE_INT_EN    REG_FLD(1, 0)

#define MDP_RDMA_INTERRUPT_STATUS_FLD_UNDERRUN_INT             REG_FLD(1, 2)
#define MDP_RDMA_INTERRUPT_STATUS_FLD_REG_UPDATE_INT           REG_FLD(1, 1)
#define MDP_RDMA_INTERRUPT_STATUS_FLD_FRAME_COMPLETE_INT       REG_FLD(1, 0)

#define MDP_RDMA_CON_FLD_LB_2B_MODE                            REG_FLD(1, 12)
#define MDP_RDMA_CON_FLD_BUFFER_MODE                           REG_FLD(1, 8)
#define MDP_RDMA_CON_FLD_OUTPUT_10B                            REG_FLD(1, 5)
#define MDP_RDMA_CON_FLD_SIMPLE_MODE                           REG_FLD(1, 4)

#define MDP_RDMA_SHADOW_CTRL_FLD_READ_WRK_REG                  REG_FLD(1, 2)
#define MDP_RDMA_SHADOW_CTRL_FLD_BYPASS_SHADOW                 REG_FLD(1, 1)
#define MDP_RDMA_SHADOW_CTRL_FLD_FORCE_COMMIT                  REG_FLD(1, 0)

#define MDP_RDMA_GMCIF_CON_FLD_THROTTLE_PERIOD                 REG_FLD(12, 20)
#define MDP_RDMA_GMCIF_CON_FLD_THROTTLE_EN                     REG_FLD(1, 19)
#define MDP_RDMA_GMCIF_CON_FLD_EXT_ULTRA_EN                    REG_FLD(1, 18)
#define MDP_RDMA_GMCIF_CON_FLD_PRE_ULTRA_EN                    REG_FLD(2, 16)
#define MDP_RDMA_GMCIF_CON_FLD_URGENT_EN                       REG_FLD(2, 14)
#define MDP_RDMA_GMCIF_CON_FLD_ULTRA_EN                        REG_FLD(2, 12)
#define MDP_RDMA_GMCIF_CON_FLD_CHANNEL_BOUNDARY                REG_FLD(2, 10)
#define MDP_RDMA_GMCIF_CON_FLD_WRITE_REQUEST_TYPE              REG_FLD(2, 8)
#define MDP_RDMA_GMCIF_CON_FLD_READ_REQUEST_TYPE               REG_FLD(4, 4)
#define MDP_RDMA_GMCIF_CON_FLD_EXT_PREULTRA_EN                 REG_FLD(1, 3)
#define MDP_RDMA_GMCIF_CON_FLD_GC_LAST_DISABLE                 REG_FLD(1, 2)
#define MDP_RDMA_GMCIF_CON_FLD_HG_DISABLE                      REG_FLD(1, 1)
#define MDP_RDMA_GMCIF_CON_FLD_COMMAND_DIV                     REG_FLD(1, 0)

#define MDP_RDMA_SRC_CON_FLD_TOP_BOUNDARY                      REG_FLD(1, 31)
#define MDP_RDMA_SRC_CON_FLD_BOT_BOUNDARY                      REG_FLD(1, 30)
#define MDP_RDMA_SRC_CON_FLD_LEFT_BOUNDARY                     REG_FLD(1, 29)
#define MDP_RDMA_SRC_CON_FLD_RIGHT_BOUNDARY                    REG_FLD(1, 28)
#define MDP_RDMA_SRC_CON_FLD_AVOID_SRAM_DOUBLE_READ            REG_FLD(1, 27)
#define MDP_RDMA_SRC_CON_FLD_VUPSAMPLE_OFF                     REG_FLD(1, 26)
#define MDP_RDMA_SRC_CON_FLD_OUTPUT_ARGB                       REG_FLD(1, 25)
#define MDP_RDMA_SRC_CON_FLD_RING_BUF_READ                     REG_FLD(1, 24)
#define MDP_RDMA_SRC_CON_FLD_BLOCK_10BIT_TILE_MODE             REG_FLD(1, 23)
#define MDP_RDMA_SRC_CON_FLD_BYTE_SWAP                         REG_FLD(1, 20)
#define MDP_RDMA_SRC_CON_FLD_BIT_NUMBER                        REG_FLD(2, 18)
#define MDP_RDMA_SRC_CON_FLD_UNIFORM_CONFIG                    REG_FLD(1, 17)
#define MDP_RDMA_SRC_CON_FLD_IS_Y_LSB                          REG_FLD(1, 16)
#define MDP_RDMA_SRC_CON_FLD_BLOCK                             REG_FLD(1, 15)
#define MDP_RDMA_SRC_CON_FLD_SWAP                              REG_FLD(1, 14)
#define MDP_RDMA_SRC_CON_FLD_VDO_FIELD                         REG_FLD(1, 13)
#define MDP_RDMA_SRC_CON_FLD_VDO_MODE                          REG_FLD(1, 12)
#define MDP_RDMA_SRC_CON_FLD_LOOSE                             REG_FLD(1, 11)
#define MDP_RDMA_SRC_CON_FLD_CUS_REP                           REG_FLD(2, 9)
#define MDP_RDMA_SRC_CON_FLD_COSITE                            REG_FLD(1, 8)
#define MDP_RDMA_SRC_CON_FLD_RGB_PAD                           REG_FLD(1, 7)
#define MDP_RDMA_SRC_CON_FLD_SRC_SWAP                          REG_FLD(3, 4)
#define MDP_RDMA_SRC_CON_FLD_SRC_FORMAT                        REG_FLD(4, 0)

#define MDP_RDMA_COMP_CON_FLD_UFO_DEC_EN                       REG_FLD(1, 31)
#define MDP_RDMA_COMP_CON_FLD_UFO_2V2H_MODE                    REG_FLD(1, 30)
#define MDP_RDMA_COMP_CON_FLD_UFO_OFFSET_MODE                  REG_FLD(1, 29)
#define MDP_RDMA_COMP_CON_FLD_UFO_REMAP_10BIT                  REG_FLD(1, 28)
#define MDP_RDMA_COMP_CON_FLD_LENGTH_TABLE_NOT_REV             REG_FLD(1, 27)
#define MDP_RDMA_COMP_CON_FLD_UFO_DATA_IN_NOT_REV              REG_FLD(1, 26)
#define MDP_RDMA_COMP_CON_FLD_UFO_DATA_OUT_NOT_REV             REG_FLD(1, 25)
#define MDP_RDMA_COMP_CON_FLD_UFO_DCP_EN                       REG_FLD(1, 24)
#define MDP_RDMA_COMP_CON_FLD_UFO_DCP_10BIT                    REG_FLD(1, 23)
#define MDP_RDMA_COMP_CON_FLD_AFBC_EN                          REG_FLD(1, 22)
#define MDP_RDMA_COMP_CON_FLD_AFBC_YUV_TRANSFORM               REG_FLD(1, 21)
#define MDP_RDMA_COMP_CON_FLD_PVRIC_EN                         REG_FLD(1, 20)
#define MDP_RDMA_COMP_CON_FLD_SHORT_BURST                      REG_FLD(1, 19)
#define MDP_RDMA_COMP_CON_FLD_UFBDC_SECURE_MODE                REG_FLD(1, 18)
#define MDP_RDMA_COMP_CON_FLD_UFBDC_HG_DISABLE                 REG_FLD(4, 14)
#define MDP_RDMA_COMP_CON_FLD_HYAFBC_EN                        REG_FLD(1, 13)
#define MDP_RDMA_COMP_CON_FLD_UFBDC_EN                         REG_FLD(1, 12)

#define MDP_RDMA_PVRIC_CRYUVAL_0_FLD_PVRIC_CRYVAL0             REG_FLD(10, 10)
#define MDP_RDMA_PVRIC_CRYUVAL_0_FLD_PVRIC_CRUVVAL0            REG_FLD(10, 0)

#define MDP_RDMA_PVRIC_CRYUVAL_1_FLD_PVRIC_CRYVAL1             REG_FLD(10, 10)
#define MDP_RDMA_PVRIC_CRYUVAL_1_FLD_PVRIC_CRUVVAL1            REG_FLD(10, 0)

#define MDP_RDMA_PVRIC_CRCH0123VAL_0_FLD_PVRIC_CRCH0123VAL0    REG_FLD(32, 0)

#define MDP_RDMA_PVRIC_CRCH0123VAL_1_FLD_PVRIC_CRCH0123VAL1    REG_FLD(32, 0)

#define MDP_RDMA_MF_BKGD_SIZE_IN_BYTE_FLD_MF_BKGD_WB           REG_FLD(23, 0)

#define MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_FLD_MF_BKGD_WP          REG_FLD(23, 0)

#define MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_H                      REG_FLD(15, 16)
#define MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_W                      REG_FLD(15, 0)

#define MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_H                    REG_FLD(15, 16)
#define MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_W                    REG_FLD(15, 0)

#define MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_H_1                 REG_FLD(6, 16)
#define MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_W_1                 REG_FLD(5, 0)

#define MDP_RDMA_MF_PAR_FLD_MF_SB                              REG_FLD(19, 12)
#define MDP_RDMA_MF_PAR_FLD_MF_JUMP                            REG_FLD(11, 0)

#define MDP_RDMA_SF_BKGD_SIZE_IN_BYTE_FLD_SF_BKGD_WB           REG_FLD(23, 0)

#define MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_FLD_MF_BKGD_HP        REG_FLD(23, 0)

#define MDP_RDMA_TARGET_LINE_FLD_LINE_THRESHOLD                REG_FLD(15, 17)
#define MDP_RDMA_TARGET_LINE_FLD_TARGET_LINE_EN                REG_FLD(1, 16)

#define MDP_RDMA_SF_PAR_FLD_SF_SB                              REG_FLD(19, 12)
#define MDP_RDMA_SF_PAR_FLD_SF_JUMP                            REG_FLD(11, 0)

#define MDP_RDMA_MB_DEPTH_FLD_MB_DEPTH                         REG_FLD(7, 0)

#define MDP_RDMA_MB_BASE_FLD_MB_BASE                           REG_FLD(16, 0)

#define MDP_RDMA_MB_CON_FLD_MB_LP                              REG_FLD(16, 16)
#define MDP_RDMA_MB_CON_FLD_MB_PPS                             REG_FLD(15, 0)

#define MDP_RDMA_SB_DEPTH_FLD_SB_DEPTH                         REG_FLD(7, 0)

#define MDP_RDMA_SB_BASE_FLD_SB_BASE                           REG_FLD(16, 0)

#define MDP_RDMA_SB_CON_FLD_SB_LP                              REG_FLD(16, 16)
#define MDP_RDMA_SB_CON_FLD_SB_PPS                             REG_FLD(15, 0)

#define MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_PARA_C                  REG_FLD(5, 16)
#define MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_PARA_Y                  REG_FLD(5, 8)
#define MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_EN                      REG_FLD(1, 4)
#define MDP_RDMA_VC1_RANGE_FLD_VC1_RED_EN                      REG_FLD(1, 0)

#define MDP_RDMA_SRC_END_0_FLD_SRC_END_0                       REG_FLD(32, 0)

#define MDP_RDMA_SRC_END_1_FLD_SRC_END_1                       REG_FLD(32, 0)

#define MDP_RDMA_SRC_END_2_FLD_SRC_END_2                       REG_FLD(32, 0)

#define MDP_RDMA_SRC_OFFSET_0_FLD_SRC_OFFSET_0                 REG_FLD(32, 0)

#define MDP_RDMA_SRC_OFFSET_1_FLD_SRC_OFFSET_1                 REG_FLD(32, 0)

#define MDP_RDMA_SRC_OFFSET_2_FLD_SRC_OFFSET_2                 REG_FLD(32, 0)

#define MDP_RDMA_SRC_OFFSET_W_0_FLD_SRC_OFFSET_W_0             REG_FLD(16, 0)

#define MDP_RDMA_SRC_OFFSET_W_1_FLD_SRC_OFFSET_W_1             REG_FLD(16, 0)

#define MDP_RDMA_SRC_OFFSET_W_2_FLD_SRC_OFFSET_W_2             REG_FLD(16, 0)

#define MDP_RDMA_SRC_OFFSET_WP_FLD_SRC_OFFSET_WP               REG_FLD(32, 0)

#define MDP_RDMA_SRC_OFFSET_HP_FLD_SRC_OFFSET_HP               REG_FLD(32, 0)

#define MDP_RDMA_C00_FLD_MDP_RDMA_C00                          REG_FLD(15, 0)

#define MDP_RDMA_C01_FLD_MDP_RDMA_C01                          REG_FLD(15, 0)

#define MDP_RDMA_C02_FLD_MDP_RDMA_C02                          REG_FLD(15, 0)

#define MDP_RDMA_C10_FLD_MDP_RDMA_C10                          REG_FLD(15, 0)

#define MDP_RDMA_C11_FLD_MDP_RDMA_C11                          REG_FLD(15, 0)

#define MDP_RDMA_C12_FLD_MDP_RDMA_C12                          REG_FLD(15, 0)

#define MDP_RDMA_C20_FLD_MDP_RDMA_C20                          REG_FLD(15, 0)

#define MDP_RDMA_C21_FLD_MDP_RDMA_C21                          REG_FLD(15, 0)

#define MDP_RDMA_C22_FLD_MDP_RDMA_C22                          REG_FLD(15, 0)

#define MDP_RDMA_PRE_ADD_0_FLD_MDP_RDMA_PRE_ADD_0              REG_FLD(9, 0)

#define MDP_RDMA_PRE_ADD_1_FLD_MDP_RDMA_PRE_ADD_1              REG_FLD(9, 0)

#define MDP_RDMA_PRE_ADD_2_FLD_MDP_RDMA_PRE_ADD_2              REG_FLD(9, 0)

#define MDP_RDMA_POST_ADD_0_FLD_MDP_RDMA_POST_ADD_0            REG_FLD(9, 0)

#define MDP_RDMA_POST_ADD_1_FLD_MDP_RDMA_POST_ADD_1            REG_FLD(9, 0)

#define MDP_RDMA_POST_ADD_2_FLD_MDP_RDMA_POST_ADD_2            REG_FLD(9, 0)

#define MDP_RDMA_TRANSFORM_0_FLD_CSC_ENABLE                    REG_FLD(1, 31)
#define MDP_RDMA_TRANSFORM_0_FLD_CSC_FORMAT                    REG_FLD(3, 28)
#define MDP_RDMA_TRANSFORM_0_FLD_int_matrix_sel                REG_FLD(5, 23)
#define MDP_RDMA_TRANSFORM_0_FLD_ext_matrix_en                 REG_FLD(1, 22)
#define MDP_RDMA_TRANSFORM_0_FLD_TRANS_EN                      REG_FLD(1, 16)
#define MDP_RDMA_TRANSFORM_0_FLD_BITEXTEND_WITH_ZERO           REG_FLD(1, 15)

#define MDP_RDMA_DMABUF_CON_0_FLD_EXTRD_ARB_MAX_0              REG_FLD(4, 24)
#define MDP_RDMA_DMABUF_CON_0_FLD_BUF_RESV_SIZE_0              REG_FLD(8, 16)
#define MDP_RDMA_DMABUF_CON_0_FLD_ISSUE_REQ_TH_0               REG_FLD(8, 0)

#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_URGENT_TH_HIGH_OFS_0  REG_FLD(10, 20)
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_PRE_ULTRA_TH_HIGH_OFS_0 REG_FLD(10, 10)
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_ULTRA_TH_HIGH_OFS_0   REG_FLD(10, 0)

#define MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_URGENT_TH_LOW_OFS_0    REG_FLD(10, 20)
#define MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_PRE_ULTRA_TH_LOW_OFS_0 REG_FLD(10, 10)
#define MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_ULTRA_TH_LOW_OFS_0     REG_FLD(10, 0)

#define MDP_RDMA_DMABUF_CON_1_FLD_EXTRD_ARB_MAX_1              REG_FLD(4, 24)
#define MDP_RDMA_DMABUF_CON_1_FLD_BUF_RESV_SIZE_1              REG_FLD(7, 16)
#define MDP_RDMA_DMABUF_CON_1_FLD_ISSUE_REQ_TH_1               REG_FLD(7, 0)

#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_URGENT_TH_HIGH_OFS_1  REG_FLD(10, 20)
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_PRE_ULTRA_TH_HIGH_OFS_1 REG_FLD(10, 10)
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_ULTRA_TH_HIGH_OFS_1   REG_FLD(10, 0)

#define MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_URGENT_TH_LOW_OFS_1    REG_FLD(10, 20)
#define MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_PRE_ULTRA_TH_LOW_OFS_1 REG_FLD(10, 10)
#define MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_ULTRA_TH_LOW_OFS_1     REG_FLD(10, 0)

#define MDP_RDMA_DMABUF_CON_2_FLD_EXTRD_ARB_MAX_2              REG_FLD(4, 24)
#define MDP_RDMA_DMABUF_CON_2_FLD_BUF_RESV_SIZE_2              REG_FLD(6, 16)
#define MDP_RDMA_DMABUF_CON_2_FLD_ISSUE_REQ_TH_2               REG_FLD(6, 0)

#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_URGENT_TH_HIGH_OFS_2  REG_FLD(10, 20)
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_PRE_ULTRA_TH_HIGH_OFS_2 REG_FLD(10, 10)
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_ULTRA_TH_HIGH_OFS_2   REG_FLD(10, 0)

#define MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_URGENT_TH_LOW_OFS_2    REG_FLD(10, 20)
#define MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_PRE_ULTRA_TH_LOW_OFS_2 REG_FLD(10, 10)
#define MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_ULTRA_TH_LOW_OFS_2     REG_FLD(10, 0)

#define MDP_RDMA_DMABUF_CON_3_FLD_EXTRD_ARB_MAX_3              REG_FLD(4, 24)
#define MDP_RDMA_DMABUF_CON_3_FLD_BUF_RESV_SIZE_3              REG_FLD(6, 16)
#define MDP_RDMA_DMABUF_CON_3_FLD_ISSUE_REQ_TH_3               REG_FLD(6, 0)

#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_URGENT_TH_HIGH_OFS_3  REG_FLD(10, 20)
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_PRE_ULTRA_TH_HIGH_OFS_3 REG_FLD(10, 10)
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_ULTRA_TH_HIGH_OFS_3   REG_FLD(10, 0)

#define MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_URGENT_TH_LOW_OFS_3    REG_FLD(10, 20)
#define MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_PRE_ULTRA_TH_LOW_OFS_3 REG_FLD(10, 10)
#define MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_ULTRA_TH_LOW_OFS_3     REG_FLD(10, 0)

#define MDP_RDMA_DITHER_CON_FLD_DITHER_INIT_H_POS              REG_FLD(2, 8)
#define MDP_RDMA_DITHER_CON_FLD_DITHER_INIT_V_POS              REG_FLD(2, 4)
#define MDP_RDMA_DITHER_CON_FLD_DITHER_EN                      REG_FLD(3, 0)

#define MDP_RDMA_RESV_DUMMY_0_FLD_RESV_DUMMY_0                 REG_FLD(32, 0)

#define MDP_RDMA_UNCOMP_MON_FLD_COMP_MON_RAW                   REG_FLD(25, 1)
#define MDP_RDMA_UNCOMP_MON_FLD_COMP_MON_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_COMP_MON_FLD_COMP_MON_COMP                    REG_FLD(25, 1)

#define MDP_RDMA_CHKS_EXTR_FLD_CHKS_EXTR_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_EXTR_FLD_CHKS_EXTR_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_INTW_FLD_CHKS_INTW_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_INTW_FLD_CHKS_INTW_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_INTR_FLD_CHKS_INTR_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_INTR_FLD_CHKS_INTR_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_ROTO_FLD_CHKS_ROTO_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_ROTO_FLD_CHKS_ROTO_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_SRIY_FLD_CHKS_SRIY_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_SRIY_FLD_CHKS_SRIY_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_SRIU_FLD_CHKS_SRIU_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_SRIU_FLD_CHKS_SRIU_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_SRIV_FLD_CHKS_SRIV_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_SRIV_FLD_CHKS_SRIV_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_SROY_FLD_CHKS_SROY_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_SROY_FLD_CHKS_SROY_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_SROU_FLD_CHKS_SROU_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_SROU_FLD_CHKS_SROU_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_SROV_FLD_CHKS_SROV_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_SROV_FLD_CHKS_SROV_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_VUPI_FLD_CHKS_VUPI_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_VUPI_FLD_CHKS_VUPI_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_CHKS_VUPO_FLD_CHKS_VUPO_CRC                   REG_FLD(24, 8)
#define MDP_RDMA_CHKS_VUPO_FLD_CHKS_VUPO_CLR                   REG_FLD(1, 0)

#define MDP_RDMA_DEBUG_CON_FLD_ufbdc_debug_out_sel             REG_FLD(6, 13)
#define MDP_RDMA_DEBUG_CON_FLD_debug_out_sel                   REG_FLD(5, 8)
#define MDP_RDMA_DEBUG_CON_FLD_CHKS_TRB_SEL                    REG_FLD(1, 5)
#define MDP_RDMA_DEBUG_CON_FLD_CHKS_SEL                        REG_FLD(4, 1)
#define MDP_RDMA_DEBUG_CON_FLD_CHKS_CRC_EN                     REG_FLD(1, 0)

#define MDP_RDMA_MON_STA_0_FLD_MON_STA_0                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_1_FLD_MON_STA_1                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_2_FLD_MON_STA_2                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_3_FLD_MON_STA_3                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_4_FLD_MON_STA_4                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_5_FLD_MON_STA_5                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_6_FLD_MON_STA_6                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_7_FLD_MON_STA_7                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_8_FLD_MON_STA_8                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_9_FLD_MON_STA_9                       REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_10_FLD_MON_STA_10                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_11_FLD_MON_STA_11                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_12_FLD_MON_STA_12                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_13_FLD_MON_STA_13                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_14_FLD_MON_STA_14                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_15_FLD_MON_STA_15                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_16_FLD_MON_STA_16                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_17_FLD_MON_STA_17                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_18_FLD_MON_STA_18                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_19_FLD_MON_STA_19                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_20_FLD_MON_STA_20                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_21_FLD_MON_STA_21                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_22_FLD_MON_STA_22                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_23_FLD_MON_STA_23                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_24_FLD_MON_STA_24                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_25_FLD_MON_STA_25                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_26_FLD_MON_STA_26                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_27_FLD_MON_STA_27                     REG_FLD(32, 0)

#define MDP_RDMA_MON_STA_28_FLD_MON_STA_28                     REG_FLD(32, 0)

#define MDP_RDMA_SRC_BASE_0_FLD_SRC_BASE_0                     REG_FLD(32, 0)

#define MDP_RDMA_SRC_BASE_1_FLD_SRC_BASE_1                     REG_FLD(32, 0)

#define MDP_RDMA_SRC_BASE_2_FLD_SRC_BASE_2                     REG_FLD(32, 0)

#define MDP_RDMA_UFO_DEC_LENGTH_BASE_Y_FLD_UFO_DEC_Y_LEN_BASE  REG_FLD(32, 0)

#define MDP_RDMA_UFO_DEC_LENGTH_BASE_C_FLD_UFO_DEC_C_LEN_BASE  REG_FLD(32, 0)

#if defined(REG_FLD_GET)
#define MDP_RDMA_EN_GET_INTERNAL_DCM_EN(reg32)                 \
	REG_FLD_GET(MDP_RDMA_EN_FLD_INTERNAL_DCM_EN, (reg32))
#define MDP_RDMA_EN_GET_ROT_ENABLE(reg32)                      \
	REG_FLD_GET(MDP_RDMA_EN_FLD_ROT_ENABLE, (reg32))

#define MDP_RDMA_UFBDC_DCM_EN_GET_UFBDC_INTERNAL_DCM_EN(reg32) \
	REG_FLD_GET(MDP_RDMA_UFBDC_DCM_EN_FLD_UFBDC_INTERNAL_DCM_EN, (reg32))

#define MDP_RDMA_RESET_GET_WARM_RESET(reg32)                   \
	REG_FLD_GET(MDP_RDMA_RESET_FLD_WARM_RESET, (reg32))

#define MDP_RDMA_INTERRUPT_ENABLE_GET_UNDERRUN_INT_EN(reg32)   \
	REG_FLD_GET(MDP_RDMA_INTERRUPT_ENABLE_FLD_UNDERRUN_INT_EN, (reg32))
#define MDP_RDMA_INTERRUPT_ENABLE_GET_REG_UPDATE_INT_EN(reg32) \
	REG_FLD_GET(MDP_RDMA_INTERRUPT_ENABLE_FLD_REG_UPDATE_INT_EN, (reg32))
#define MDP_RDMA_INTERRUPT_ENABLE_GET_FRAME_COMPLETE_INT_EN(reg32) \
	REG_FLD_GET(MDP_RDMA_INTERRUPT_ENABLE_FLD_FRAME_COMPLETE_INT_EN, \
			(reg32))

#define MDP_RDMA_INTERRUPT_STATUS_GET_UNDERRUN_INT(reg32)      \
	REG_FLD_GET(MDP_RDMA_INTERRUPT_STATUS_FLD_UNDERRUN_INT, (reg32))
#define MDP_RDMA_INTERRUPT_STATUS_GET_REG_UPDATE_INT(reg32)    \
	REG_FLD_GET(MDP_RDMA_INTERRUPT_STATUS_FLD_REG_UPDATE_INT, (reg32))
#define MDP_RDMA_INTERRUPT_STATUS_GET_FRAME_COMPLETE_INT(reg32) \
	REG_FLD_GET(MDP_RDMA_INTERRUPT_STATUS_FLD_FRAME_COMPLETE_INT, (reg32))

#define MDP_RDMA_CON_GET_LB_2B_MODE(reg32)                     \
	REG_FLD_GET(MDP_RDMA_CON_FLD_LB_2B_MODE, (reg32))\

#define MDP_RDMA_CON_GET_BUFFER_MODE(reg32)                    \
	REG_FLD_GET(MDP_RDMA_CON_FLD_BUFFER_MODE, (reg32))
#define MDP_RDMA_CON_GET_OUTPUT_10B(reg32)                     \
	REG_FLD_GET(MDP_RDMA_CON_FLD_OUTPUT_10B, (reg32))
#define MDP_RDMA_CON_GET_SIMPLE_MODE(reg32)                    \
	REG_FLD_GET(MDP_RDMA_CON_FLD_SIMPLE_MODE, (reg32))

#define MDP_RDMA_SHADOW_CTRL_GET_READ_WRK_REG(reg32)           \
	REG_FLD_GET(MDP_RDMA_SHADOW_CTRL_FLD_READ_WRK_REG, (reg32))
#define MDP_RDMA_SHADOW_CTRL_GET_BYPASS_SHADOW(reg32)          \
	REG_FLD_GET(MDP_RDMA_SHADOW_CTRL_FLD_BYPASS_SHADOW, (reg32))
#define MDP_RDMA_SHADOW_CTRL_GET_FORCE_COMMIT(reg32)           \
	REG_FLD_GET(MDP_RDMA_SHADOW_CTRL_FLD_FORCE_COMMIT, (reg32))

#define MDP_RDMA_GMCIF_CON_GET_THROTTLE_PERIOD(reg32)          \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_THROTTLE_PERIOD, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_THROTTLE_EN(reg32)              \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_THROTTLE_EN, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_EXT_ULTRA_EN(reg32)             \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_EXT_ULTRA_EN, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_PRE_ULTRA_EN(reg32)             \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_PRE_ULTRA_EN, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_URGENT_EN(reg32)                \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_URGENT_EN, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_ULTRA_EN(reg32)                 \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_ULTRA_EN, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_CHANNEL_BOUNDARY(reg32)         \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_CHANNEL_BOUNDARY, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_WRITE_REQUEST_TYPE(reg32)       \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_WRITE_REQUEST_TYPE, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_READ_REQUEST_TYPE(reg32)        \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_READ_REQUEST_TYPE, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_EXT_PREULTRA_EN(reg32)          \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_EXT_PREULTRA_EN, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_GC_LAST_DISABLE(reg32)          \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_GC_LAST_DISABLE, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_HG_DISABLE(reg32)               \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_HG_DISABLE, (reg32))
#define MDP_RDMA_GMCIF_CON_GET_COMMAND_DIV(reg32)              \
	REG_FLD_GET(MDP_RDMA_GMCIF_CON_FLD_COMMAND_DIV, (reg32))

#define MDP_RDMA_SRC_CON_GET_TOP_BOUNDARY(reg32)               \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_TOP_BOUNDARY, (reg32))
#define MDP_RDMA_SRC_CON_GET_BOT_BOUNDARY(reg32)               \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_BOT_BOUNDARY, (reg32))
#define MDP_RDMA_SRC_CON_GET_LEFT_BOUNDARY(reg32)              \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_LEFT_BOUNDARY, (reg32))
#define MDP_RDMA_SRC_CON_GET_RIGHT_BOUNDARY(reg32)             \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_RIGHT_BOUNDARY, (reg32))
#define MDP_RDMA_SRC_CON_GET_AVOID_SRAM_DOUBLE_READ(reg32)     \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_AVOID_SRAM_DOUBLE_READ, (reg32))
#define MDP_RDMA_SRC_CON_GET_VUPSAMPLE_OFF(reg32)              \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_VUPSAMPLE_OFF, (reg32))
#define MDP_RDMA_SRC_CON_GET_OUTPUT_ARGB(reg32)                \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_OUTPUT_ARGB, (reg32))
#define MDP_RDMA_SRC_CON_GET_RING_BUF_READ(reg32)              \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_RING_BUF_READ, (reg32))
#define MDP_RDMA_SRC_CON_GET_BLOCK_10BIT_TILE_MODE(reg32)      \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_BLOCK_10BIT_TILE_MODE, (reg32))
#define MDP_RDMA_SRC_CON_GET_BYTE_SWAP(reg32)                  \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_BYTE_SWAP, (reg32))
#define MDP_RDMA_SRC_CON_GET_BIT_NUMBER(reg32)                 \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_BIT_NUMBER, (reg32))
#define MDP_RDMA_SRC_CON_GET_UNIFORM_CONFIG(reg32)             \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_UNIFORM_CONFIG, (reg32))
#define MDP_RDMA_SRC_CON_GET_IS_Y_LSB(reg32)                   \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_IS_Y_LSB, (reg32))
#define MDP_RDMA_SRC_CON_GET_BLOCK(reg32)                      \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_BLOCK, (reg32))
#define MDP_RDMA_SRC_CON_GET_SWAP(reg32)                       \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_SWAP, (reg32))
#define MDP_RDMA_SRC_CON_GET_VDO_FIELD(reg32)                  \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_VDO_FIELD, (reg32))
#define MDP_RDMA_SRC_CON_GET_VDO_MODE(reg32)                   \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_VDO_MODE, (reg32))
#define MDP_RDMA_SRC_CON_GET_LOOSE(reg32)                      \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_LOOSE, (reg32))
#define MDP_RDMA_SRC_CON_GET_CUS_REP(reg32)                    \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_CUS_REP, (reg32))
#define MDP_RDMA_SRC_CON_GET_COSITE(reg32)                     \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_COSITE, (reg32))
#define MDP_RDMA_SRC_CON_GET_RGB_PAD(reg32)                    \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_RGB_PAD, (reg32))
#define MDP_RDMA_SRC_CON_GET_SRC_SWAP(reg32)                   \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_SRC_SWAP, (reg32))
#define MDP_RDMA_SRC_CON_GET_SRC_FORMAT(reg32)                 \
	REG_FLD_GET(MDP_RDMA_SRC_CON_FLD_SRC_FORMAT, (reg32))

#define MDP_RDMA_COMP_CON_GET_UFO_DEC_EN(reg32)                \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFO_DEC_EN, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFO_2V2H_MODE(reg32)             \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFO_2V2H_MODE, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFO_OFFSET_MODE(reg32)           \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFO_OFFSET_MODE, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFO_REMAP_10BIT(reg32)           \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFO_REMAP_10BIT, (reg32))
#define MDP_RDMA_COMP_CON_GET_LENGTH_TABLE_NOT_REV(reg32)      \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_LENGTH_TABLE_NOT_REV, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFO_DATA_IN_NOT_REV(reg32)       \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFO_DATA_IN_NOT_REV, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFO_DATA_OUT_NOT_REV(reg32)      \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFO_DATA_OUT_NOT_REV, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFO_DCP_EN(reg32)                \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFO_DCP_EN, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFO_DCP_10BIT(reg32)             \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFO_DCP_10BIT, (reg32))
#define MDP_RDMA_COMP_CON_GET_AFBC_EN(reg32)                   \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_AFBC_EN, (reg32))
#define MDP_RDMA_COMP_CON_GET_AFBC_YUV_TRANSFORM(reg32)        \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_AFBC_YUV_TRANSFORM, (reg32))
#define MDP_RDMA_COMP_CON_GET_PVRIC_EN(reg32)                  \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_PVRIC_EN, (reg32))
#define MDP_RDMA_COMP_CON_GET_SHORT_BURST(reg32)               \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_SHORT_BURST, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFBDC_SECURE_MODE(reg32)         \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFBDC_SECURE_MODE, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFBDC_HG_DISABLE(reg32)          \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFBDC_HG_DISABLE, (reg32))
#define MDP_RDMA_COMP_CON_GET_HYAFBC_EN(reg32)                 \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_HYAFBC_EN, (reg32))
#define MDP_RDMA_COMP_CON_GET_UFBDC_EN(reg32)                  \
	REG_FLD_GET(MDP_RDMA_COMP_CON_FLD_UFBDC_EN, (reg32))

#define MDP_RDMA_PVRIC_CRYUVAL_0_GET_PVRIC_CRYVAL0(reg32)      \
	REG_FLD_GET(MDP_RDMA_PVRIC_CRYUVAL_0_FLD_PVRIC_CRYVAL0, (reg32))
#define MDP_RDMA_PVRIC_CRYUVAL_0_GET_PVRIC_CRUVVAL0(reg32)     \
	REG_FLD_GET(MDP_RDMA_PVRIC_CRYUVAL_0_FLD_PVRIC_CRUVVAL0, (reg32))

#define MDP_RDMA_PVRIC_CRYUVAL_1_GET_PVRIC_CRYVAL1(reg32)      \
	REG_FLD_GET(MDP_RDMA_PVRIC_CRYUVAL_1_FLD_PVRIC_CRYVAL1, (reg32))
#define MDP_RDMA_PVRIC_CRYUVAL_1_GET_PVRIC_CRUVVAL1(reg32)     \
	REG_FLD_GET(MDP_RDMA_PVRIC_CRYUVAL_1_FLD_PVRIC_CRUVVAL1, (reg32))

#define MDP_RDMA_PVRIC_CRCH0123VAL_0_GET_PVRIC_CRCH0123VAL0(reg32) \
	REG_FLD_GET(MDP_RDMA_PVRIC_CRCH0123VAL_0_FLD_PVRIC_CRCH0123VAL0, \
			(reg32))

#define MDP_RDMA_PVRIC_CRCH0123VAL_1_GET_PVRIC_CRCH0123VAL1(reg32) \
	REG_FLD_GET(MDP_RDMA_PVRIC_CRCH0123VAL_1_FLD_PVRIC_CRCH0123VAL1, \
			(reg32))

#define MDP_RDMA_MF_BKGD_SIZE_IN_BYTE_GET_MF_BKGD_WB(reg32)    \
	REG_FLD_GET(MDP_RDMA_MF_BKGD_SIZE_IN_BYTE_FLD_MF_BKGD_WB, (reg32))

#define MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_GET_MF_BKGD_WP(reg32)   \
	REG_FLD_GET(MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_FLD_MF_BKGD_WP, (reg32))

#define MDP_RDMA_MF_SRC_SIZE_GET_MF_SRC_H(reg32)               \
	REG_FLD_GET(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_H, (reg32))
#define MDP_RDMA_MF_SRC_SIZE_GET_MF_SRC_W(reg32)               \
	REG_FLD_GET(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_W, (reg32))

#define MDP_RDMA_MF_CLIP_SIZE_GET_MF_CLIP_H(reg32)             \
	REG_FLD_GET(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_H, (reg32))
#define MDP_RDMA_MF_CLIP_SIZE_GET_MF_CLIP_W(reg32)             \
	REG_FLD_GET(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_W, (reg32))

#define MDP_RDMA_MF_OFFSET_1_GET_MF_OFFSET_H_1(reg32)          \
	REG_FLD_GET(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_H_1, (reg32))
#define MDP_RDMA_MF_OFFSET_1_GET_MF_OFFSET_W_1(reg32)          \
	REG_FLD_GET(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_W_1, (reg32))

#define MDP_RDMA_MF_PAR_GET_MF_SB(reg32)                       \
	REG_FLD_GET(MDP_RDMA_MF_PAR_FLD_MF_SB, (reg32))
#define MDP_RDMA_MF_PAR_GET_MF_JUMP(reg32)                     \
	REG_FLD_GET(MDP_RDMA_MF_PAR_FLD_MF_JUMP, (reg32))

#define MDP_RDMA_SF_BKGD_SIZE_IN_BYTE_GET_SF_BKGD_WB(reg32)    \
	REG_FLD_GET(MDP_RDMA_SF_BKGD_SIZE_IN_BYTE_FLD_SF_BKGD_WB, (reg32))

#define MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_GET_MF_BKGD_HP(reg32) \
	REG_FLD_GET(MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_FLD_MF_BKGD_HP, (reg32))

#define MDP_RDMA_TARGET_LINE_GET_LINE_THRESHOLD(reg32)         \
	REG_FLD_GET(MDP_RDMA_TARGET_LINE_FLD_LINE_THRESHOLD, (reg32))
#define MDP_RDMA_TARGET_LINE_GET_TARGET_LINE_EN(reg32)         \
	REG_FLD_GET(MDP_RDMA_TARGET_LINE_FLD_TARGET_LINE_EN, (reg32))

#define MDP_RDMA_SF_PAR_GET_SF_SB(reg32)                       \
	REG_FLD_GET(MDP_RDMA_SF_PAR_FLD_SF_SB, (reg32))
#define MDP_RDMA_SF_PAR_GET_SF_JUMP(reg32)                     \
	REG_FLD_GET(MDP_RDMA_SF_PAR_FLD_SF_JUMP, (reg32))

#define MDP_RDMA_MB_DEPTH_GET_MB_DEPTH(reg32)                  \
	REG_FLD_GET(MDP_RDMA_MB_DEPTH_FLD_MB_DEPTH, (reg32))

#define MDP_RDMA_MB_BASE_GET_MB_BASE(reg32)                    \
	REG_FLD_GET(MDP_RDMA_MB_BASE_FLD_MB_BASE, (reg32))

#define MDP_RDMA_MB_CON_GET_MB_LP(reg32)                       \
	REG_FLD_GET(MDP_RDMA_MB_CON_FLD_MB_LP, (reg32))
#define MDP_RDMA_MB_CON_GET_MB_PPS(reg32)                      \
	REG_FLD_GET(MDP_RDMA_MB_CON_FLD_MB_PPS, (reg32))

#define MDP_RDMA_SB_DEPTH_GET_SB_DEPTH(reg32)                  \
	REG_FLD_GET(MDP_RDMA_SB_DEPTH_FLD_SB_DEPTH, (reg32))

#define MDP_RDMA_SB_BASE_GET_SB_BASE(reg32)                    \
	REG_FLD_GET(MDP_RDMA_SB_BASE_FLD_SB_BASE, (reg32))

#define MDP_RDMA_SB_CON_GET_SB_LP(reg32)                       \
	REG_FLD_GET(MDP_RDMA_SB_CON_FLD_SB_LP, (reg32))
#define MDP_RDMA_SB_CON_GET_SB_PPS(reg32)                      \
	REG_FLD_GET(MDP_RDMA_SB_CON_FLD_SB_PPS, (reg32))

#define MDP_RDMA_VC1_RANGE_GET_VC1_MAP_PARA_C(reg32)           \
	REG_FLD_GET(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_PARA_C, (reg32))
#define MDP_RDMA_VC1_RANGE_GET_VC1_MAP_PARA_Y(reg32)           \
	REG_FLD_GET(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_PARA_Y, (reg32))
#define MDP_RDMA_VC1_RANGE_GET_VC1_MAP_EN(reg32)               \
	REG_FLD_GET(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_EN, (reg32))
#define MDP_RDMA_VC1_RANGE_GET_VC1_RED_EN(reg32)               \
	REG_FLD_GET(MDP_RDMA_VC1_RANGE_FLD_VC1_RED_EN, (reg32))

#define MDP_RDMA_SRC_END_0_GET_SRC_END_0(reg32)                \
	REG_FLD_GET(MDP_RDMA_SRC_END_0_FLD_SRC_END_0, (reg32))

#define MDP_RDMA_SRC_END_1_GET_SRC_END_1(reg32)                \
	REG_FLD_GET(MDP_RDMA_SRC_END_1_FLD_SRC_END_1, (reg32))

#define MDP_RDMA_SRC_END_2_GET_SRC_END_2(reg32)                \
	REG_FLD_GET(MDP_RDMA_SRC_END_2_FLD_SRC_END_2, (reg32))

#define MDP_RDMA_SRC_OFFSET_0_GET_SRC_OFFSET_0(reg32)          \
	REG_FLD_GET(MDP_RDMA_SRC_OFFSET_0_FLD_SRC_OFFSET_0, (reg32))

#define MDP_RDMA_SRC_OFFSET_1_GET_SRC_OFFSET_1(reg32)          \
	REG_FLD_GET(MDP_RDMA_SRC_OFFSET_1_FLD_SRC_OFFSET_1, (reg32))

#define MDP_RDMA_SRC_OFFSET_2_GET_SRC_OFFSET_2(reg32)          \
	REG_FLD_GET(MDP_RDMA_SRC_OFFSET_2_FLD_SRC_OFFSET_2, (reg32))

#define MDP_RDMA_SRC_OFFSET_W_0_GET_SRC_OFFSET_W_0(reg32)      \
	REG_FLD_GET(MDP_RDMA_SRC_OFFSET_W_0_FLD_SRC_OFFSET_W_0, (reg32))

#define MDP_RDMA_SRC_OFFSET_W_1_GET_SRC_OFFSET_W_1(reg32)      \
	REG_FLD_GET(MDP_RDMA_SRC_OFFSET_W_1_FLD_SRC_OFFSET_W_1, (reg32))

#define MDP_RDMA_SRC_OFFSET_W_2_GET_SRC_OFFSET_W_2(reg32)      \
	REG_FLD_GET(MDP_RDMA_SRC_OFFSET_W_2_FLD_SRC_OFFSET_W_2, (reg32))

#define MDP_RDMA_SRC_OFFSET_WP_GET_SRC_OFFSET_WP(reg32)        \
	REG_FLD_GET(MDP_RDMA_SRC_OFFSET_WP_FLD_SRC_OFFSET_WP, (reg32))

#define MDP_RDMA_SRC_OFFSET_HP_GET_SRC_OFFSET_HP(reg32)        \
	REG_FLD_GET(MDP_RDMA_SRC_OFFSET_HP_FLD_SRC_OFFSET_HP, (reg32))

#define MDP_RDMA_C00_GET_MDP_RDMA_C00(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C00_FLD_MDP_RDMA_C00, (reg32))

#define MDP_RDMA_C01_GET_MDP_RDMA_C01(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C01_FLD_MDP_RDMA_C01, (reg32))

#define MDP_RDMA_C02_GET_MDP_RDMA_C02(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C02_FLD_MDP_RDMA_C02, (reg32))

#define MDP_RDMA_C10_GET_MDP_RDMA_C10(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C10_FLD_MDP_RDMA_C10, (reg32))

#define MDP_RDMA_C11_GET_MDP_RDMA_C11(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C11_FLD_MDP_RDMA_C11, (reg32))

#define MDP_RDMA_C12_GET_MDP_RDMA_C12(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C12_FLD_MDP_RDMA_C12, (reg32))

#define MDP_RDMA_C20_GET_MDP_RDMA_C20(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C20_FLD_MDP_RDMA_C20, (reg32))

#define MDP_RDMA_C21_GET_MDP_RDMA_C21(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C21_FLD_MDP_RDMA_C21, (reg32))

#define MDP_RDMA_C22_GET_MDP_RDMA_C22(reg32)                   \
	REG_FLD_GET(MDP_RDMA_C22_FLD_MDP_RDMA_C22, (reg32))

#define MDP_RDMA_PRE_ADD_0_GET_MDP_RDMA_PRE_ADD_0(reg32)       \
	REG_FLD_GET(MDP_RDMA_PRE_ADD_0_FLD_MDP_RDMA_PRE_ADD_0, (reg32))

#define MDP_RDMA_PRE_ADD_1_GET_MDP_RDMA_PRE_ADD_1(reg32)       \
	REG_FLD_GET(MDP_RDMA_PRE_ADD_1_FLD_MDP_RDMA_PRE_ADD_1, (reg32))

#define MDP_RDMA_PRE_ADD_2_GET_MDP_RDMA_PRE_ADD_2(reg32)       \
	REG_FLD_GET(MDP_RDMA_PRE_ADD_2_FLD_MDP_RDMA_PRE_ADD_2, (reg32))

#define MDP_RDMA_POST_ADD_0_GET_MDP_RDMA_POST_ADD_0(reg32)     \
	REG_FLD_GET(MDP_RDMA_POST_ADD_0_FLD_MDP_RDMA_POST_ADD_0, (reg32))

#define MDP_RDMA_POST_ADD_1_GET_MDP_RDMA_POST_ADD_1(reg32)     \
	REG_FLD_GET(MDP_RDMA_POST_ADD_1_FLD_MDP_RDMA_POST_ADD_1, (reg32))

#define MDP_RDMA_POST_ADD_2_GET_MDP_RDMA_POST_ADD_2(reg32)     \
	REG_FLD_GET(MDP_RDMA_POST_ADD_2_FLD_MDP_RDMA_POST_ADD_2, (reg32))

#define MDP_RDMA_TRANSFORM_0_GET_CSC_ENABLE(reg32)             \
	REG_FLD_GET(MDP_RDMA_TRANSFORM_0_FLD_CSC_ENABLE, (reg32))
#define MDP_RDMA_TRANSFORM_0_GET_CSC_FORMAT(reg32)             \
	REG_FLD_GET(MDP_RDMA_TRANSFORM_0_FLD_CSC_FORMAT, (reg32))
#define MDP_RDMA_TRANSFORM_0_GET_int_matrix_sel(reg32)         \
	REG_FLD_GET(MDP_RDMA_TRANSFORM_0_FLD_int_matrix_sel, (reg32))
#define MDP_RDMA_TRANSFORM_0_GET_ext_matrix_en(reg32)          \
	REG_FLD_GET(MDP_RDMA_TRANSFORM_0_FLD_ext_matrix_en, (reg32))
#define MDP_RDMA_TRANSFORM_0_GET_TRANS_EN(reg32)               \
	REG_FLD_GET(MDP_RDMA_TRANSFORM_0_FLD_TRANS_EN, (reg32))
#define MDP_RDMA_TRANSFORM_0_GET_BITEXTEND_WITH_ZERO(reg32)    \
	REG_FLD_GET(MDP_RDMA_TRANSFORM_0_FLD_BITEXTEND_WITH_ZERO, (reg32))

#define MDP_RDMA_DMABUF_CON_0_GET_EXTRD_ARB_MAX_0(reg32)       \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_0_FLD_EXTRD_ARB_MAX_0, (reg32))
#define MDP_RDMA_DMABUF_CON_0_GET_BUF_RESV_SIZE_0(reg32)       \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_0_FLD_BUF_RESV_SIZE_0, (reg32))
#define MDP_RDMA_DMABUF_CON_0_GET_ISSUE_REQ_TH_0(reg32)        \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_0_FLD_ISSUE_REQ_TH_0, (reg32))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_GET_URGENT_TH_HIGH_OFS_0(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_URGENT_TH_HIGH_OFS_0, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_GET_PRE_ULTRA_TH_HIGH_OFS_0(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_PRE_ULTRA_TH_HIGH_OFS_0, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_GET_ULTRA_TH_HIGH_OFS_0(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_ULTRA_TH_HIGH_OFS_0, \
			(reg32))

#define MDP_RDMA_ULTRA_TH_LOW_CON_0_GET_URGENT_TH_LOW_OFS_0(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_URGENT_TH_LOW_OFS_0, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_LOW_CON_0_GET_PRE_ULTRA_TH_LOW_OFS_0(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_PRE_ULTRA_TH_LOW_OFS_0,\
			 (reg32))
#define MDP_RDMA_ULTRA_TH_LOW_CON_0_GET_ULTRA_TH_LOW_OFS_0(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_ULTRA_TH_LOW_OFS_0, \
			(reg32))

#define MDP_RDMA_DMABUF_CON_1_GET_EXTRD_ARB_MAX_1(reg32)       \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_1_FLD_EXTRD_ARB_MAX_1, (reg32))
#define MDP_RDMA_DMABUF_CON_1_GET_BUF_RESV_SIZE_1(reg32)       \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_1_FLD_BUF_RESV_SIZE_1, (reg32))
#define MDP_RDMA_DMABUF_CON_1_GET_ISSUE_REQ_TH_1(reg32)        \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_1_FLD_ISSUE_REQ_TH_1, (reg32))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_GET_URGENT_TH_HIGH_OFS_1(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_URGENT_TH_HIGH_OFS_1, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_GET_PRE_ULTRA_TH_HIGH_OFS_1(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_PRE_ULTRA_TH_HIGH_OFS_1, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_GET_ULTRA_TH_HIGH_OFS_1(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_ULTRA_TH_HIGH_OFS_1, \
			(reg32))

#define MDP_RDMA_ULTRA_TH_LOW_CON_1_GET_URGENT_TH_LOW_OFS_1(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_URGENT_TH_LOW_OFS_1, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_LOW_CON_1_GET_PRE_ULTRA_TH_LOW_OFS_1(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_PRE_ULTRA_TH_LOW_OFS_1, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_LOW_CON_1_GET_ULTRA_TH_LOW_OFS_1(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_ULTRA_TH_LOW_OFS_1, \
			(reg32))

#define MDP_RDMA_DMABUF_CON_2_GET_EXTRD_ARB_MAX_2(reg32)       \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_2_FLD_EXTRD_ARB_MAX_2, (reg32))
#define MDP_RDMA_DMABUF_CON_2_GET_BUF_RESV_SIZE_2(reg32)       \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_2_FLD_BUF_RESV_SIZE_2, (reg32))
#define MDP_RDMA_DMABUF_CON_2_GET_ISSUE_REQ_TH_2(reg32)        \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_2_FLD_ISSUE_REQ_TH_2, (reg32))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_GET_URGENT_TH_HIGH_OFS_2(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_URGENT_TH_HIGH_OFS_2, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_GET_PRE_ULTRA_TH_HIGH_OFS_2(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_PRE_ULTRA_TH_HIGH_OFS_2, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_GET_ULTRA_TH_HIGH_OFS_2(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_ULTRA_TH_HIGH_OFS_2, \
			(reg32))

#define MDP_RDMA_ULTRA_TH_LOW_CON_2_GET_URGENT_TH_LOW_OFS_2(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_URGENT_TH_LOW_OFS_2, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_LOW_CON_2_GET_PRE_ULTRA_TH_LOW_OFS_2(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_PRE_ULTRA_TH_LOW_OFS_2, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_LOW_CON_2_GET_ULTRA_TH_LOW_OFS_2(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_ULTRA_TH_LOW_OFS_2, \
			(reg32))

#define MDP_RDMA_DMABUF_CON_3_GET_EXTRD_ARB_MAX_3(reg32)       \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_3_FLD_EXTRD_ARB_MAX_3, (reg32))
#define MDP_RDMA_DMABUF_CON_3_GET_BUF_RESV_SIZE_3(reg32)       \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_3_FLD_BUF_RESV_SIZE_3, (reg32))
#define MDP_RDMA_DMABUF_CON_3_GET_ISSUE_REQ_TH_3(reg32)        \
	REG_FLD_GET(MDP_RDMA_DMABUF_CON_3_FLD_ISSUE_REQ_TH_3, (reg32))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_GET_URGENT_TH_HIGH_OFS_3(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_URGENT_TH_HIGH_OFS_3, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_GET_PRE_ULTRA_TH_HIGH_OFS_3(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_PRE_ULTRA_TH_HIGH_OFS_3, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_GET_ULTRA_TH_HIGH_OFS_3(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_ULTRA_TH_HIGH_OFS_3, \
			(reg32))

#define MDP_RDMA_ULTRA_TH_LOW_CON_3_GET_URGENT_TH_LOW_OFS_3(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_URGENT_TH_LOW_OFS_3, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_LOW_CON_3_GET_PRE_ULTRA_TH_LOW_OFS_3(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_PRE_ULTRA_TH_LOW_OFS_3, \
			(reg32))
#define MDP_RDMA_ULTRA_TH_LOW_CON_3_GET_ULTRA_TH_LOW_OFS_3(reg32) \
	REG_FLD_GET(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_ULTRA_TH_LOW_OFS_3, \
			(reg32))

#define MDP_RDMA_DITHER_CON_GET_DITHER_INIT_H_POS(reg32)       \
	REG_FLD_GET(MDP_RDMA_DITHER_CON_FLD_DITHER_INIT_H_POS, (reg32))
#define MDP_RDMA_DITHER_CON_GET_DITHER_INIT_V_POS(reg32)       \
	REG_FLD_GET(MDP_RDMA_DITHER_CON_FLD_DITHER_INIT_V_POS, (reg32))
#define MDP_RDMA_DITHER_CON_GET_DITHER_EN(reg32)               \
	REG_FLD_GET(MDP_RDMA_DITHER_CON_FLD_DITHER_EN, (reg32))

#define MDP_RDMA_RESV_DUMMY_0_GET_RESV_DUMMY_0(reg32)          \
	REG_FLD_GET(MDP_RDMA_RESV_DUMMY_0_FLD_RESV_DUMMY_0, (reg32))

#define MDP_RDMA_UNCOMP_MON_GET_COMP_MON_RAW(reg32)            \
	REG_FLD_GET(MDP_RDMA_UNCOMP_MON_FLD_COMP_MON_RAW, (reg32))
#define MDP_RDMA_UNCOMP_MON_GET_COMP_MON_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_UNCOMP_MON_FLD_COMP_MON_CLR, (reg32))

#define MDP_RDMA_COMP_MON_GET_COMP_MON_COMP(reg32)             \
	REG_FLD_GET(MDP_RDMA_COMP_MON_FLD_COMP_MON_COMP, (reg32))

#define MDP_RDMA_CHKS_EXTR_GET_CHKS_EXTR_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_EXTR_FLD_CHKS_EXTR_CRC, (reg32))
#define MDP_RDMA_CHKS_EXTR_GET_CHKS_EXTR_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_EXTR_FLD_CHKS_EXTR_CLR, (reg32))

#define MDP_RDMA_CHKS_INTW_GET_CHKS_INTW_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_INTW_FLD_CHKS_INTW_CRC, (reg32))
#define MDP_RDMA_CHKS_INTW_GET_CHKS_INTW_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_INTW_FLD_CHKS_INTW_CLR, (reg32))

#define MDP_RDMA_CHKS_INTR_GET_CHKS_INTR_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_INTR_FLD_CHKS_INTR_CRC, (reg32))
#define MDP_RDMA_CHKS_INTR_GET_CHKS_INTR_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_INTR_FLD_CHKS_INTR_CLR, (reg32))

#define MDP_RDMA_CHKS_ROTO_GET_CHKS_ROTO_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_ROTO_FLD_CHKS_ROTO_CRC, (reg32))
#define MDP_RDMA_CHKS_ROTO_GET_CHKS_ROTO_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_ROTO_FLD_CHKS_ROTO_CLR, (reg32))

#define MDP_RDMA_CHKS_SRIY_GET_CHKS_SRIY_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SRIY_FLD_CHKS_SRIY_CRC, (reg32))
#define MDP_RDMA_CHKS_SRIY_GET_CHKS_SRIY_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SRIY_FLD_CHKS_SRIY_CLR, (reg32))

#define MDP_RDMA_CHKS_SRIU_GET_CHKS_SRIU_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SRIU_FLD_CHKS_SRIU_CRC, (reg32))
#define MDP_RDMA_CHKS_SRIU_GET_CHKS_SRIU_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SRIU_FLD_CHKS_SRIU_CLR, (reg32))

#define MDP_RDMA_CHKS_SRIV_GET_CHKS_SRIV_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SRIV_FLD_CHKS_SRIV_CRC, (reg32))
#define MDP_RDMA_CHKS_SRIV_GET_CHKS_SRIV_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SRIV_FLD_CHKS_SRIV_CLR, (reg32))

#define MDP_RDMA_CHKS_SROY_GET_CHKS_SROY_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SROY_FLD_CHKS_SROY_CRC, (reg32))
#define MDP_RDMA_CHKS_SROY_GET_CHKS_SROY_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SROY_FLD_CHKS_SROY_CLR, (reg32))

#define MDP_RDMA_CHKS_SROU_GET_CHKS_SROU_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SROU_FLD_CHKS_SROU_CRC, (reg32))
#define MDP_RDMA_CHKS_SROU_GET_CHKS_SROU_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SROU_FLD_CHKS_SROU_CLR, (reg32))

#define MDP_RDMA_CHKS_SROV_GET_CHKS_SROV_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SROV_FLD_CHKS_SROV_CRC, (reg32))
#define MDP_RDMA_CHKS_SROV_GET_CHKS_SROV_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_SROV_FLD_CHKS_SROV_CLR, (reg32))

#define MDP_RDMA_CHKS_VUPI_GET_CHKS_VUPI_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_VUPI_FLD_CHKS_VUPI_CRC, (reg32))
#define MDP_RDMA_CHKS_VUPI_GET_CHKS_VUPI_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_VUPI_FLD_CHKS_VUPI_CLR, (reg32))

#define MDP_RDMA_CHKS_VUPO_GET_CHKS_VUPO_CRC(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_VUPO_FLD_CHKS_VUPO_CRC, (reg32))
#define MDP_RDMA_CHKS_VUPO_GET_CHKS_VUPO_CLR(reg32)            \
	REG_FLD_GET(MDP_RDMA_CHKS_VUPO_FLD_CHKS_VUPO_CLR, (reg32))

#define MDP_RDMA_DEBUG_CON_GET_ufbdc_debug_out_sel(reg32)      \
	REG_FLD_GET(MDP_RDMA_DEBUG_CON_FLD_ufbdc_debug_out_sel, (reg32))
#define MDP_RDMA_DEBUG_CON_GET_debug_out_sel(reg32)            \
	REG_FLD_GET(MDP_RDMA_DEBUG_CON_FLD_debug_out_sel, (reg32))
#define MDP_RDMA_DEBUG_CON_GET_CHKS_TRB_SEL(reg32)             \
	REG_FLD_GET(MDP_RDMA_DEBUG_CON_FLD_CHKS_TRB_SEL, (reg32))
#define MDP_RDMA_DEBUG_CON_GET_CHKS_SEL(reg32)                 \
	REG_FLD_GET(MDP_RDMA_DEBUG_CON_FLD_CHKS_SEL, (reg32))
#define MDP_RDMA_DEBUG_CON_GET_CHKS_CRC_EN(reg32)              \
	REG_FLD_GET(MDP_RDMA_DEBUG_CON_FLD_CHKS_CRC_EN, (reg32))

#define MDP_RDMA_MON_STA_0_GET_MON_STA_0(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_0_FLD_MON_STA_0, (reg32))

#define MDP_RDMA_MON_STA_1_GET_MON_STA_1(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_1_FLD_MON_STA_1, (reg32))

#define MDP_RDMA_MON_STA_2_GET_MON_STA_2(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_2_FLD_MON_STA_2, (reg32))

#define MDP_RDMA_MON_STA_3_GET_MON_STA_3(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_3_FLD_MON_STA_3, (reg32))

#define MDP_RDMA_MON_STA_4_GET_MON_STA_4(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_4_FLD_MON_STA_4, (reg32))

#define MDP_RDMA_MON_STA_5_GET_MON_STA_5(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_5_FLD_MON_STA_5, (reg32))

#define MDP_RDMA_MON_STA_6_GET_MON_STA_6(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_6_FLD_MON_STA_6, (reg32))

#define MDP_RDMA_MON_STA_7_GET_MON_STA_7(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_7_FLD_MON_STA_7, (reg32))

#define MDP_RDMA_MON_STA_8_GET_MON_STA_8(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_8_FLD_MON_STA_8, (reg32))

#define MDP_RDMA_MON_STA_9_GET_MON_STA_9(reg32)                \
	REG_FLD_GET(MDP_RDMA_MON_STA_9_FLD_MON_STA_9, (reg32))

#define MDP_RDMA_MON_STA_10_GET_MON_STA_10(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_10_FLD_MON_STA_10, (reg32))

#define MDP_RDMA_MON_STA_11_GET_MON_STA_11(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_11_FLD_MON_STA_11, (reg32))

#define MDP_RDMA_MON_STA_12_GET_MON_STA_12(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_12_FLD_MON_STA_12, (reg32))

#define MDP_RDMA_MON_STA_13_GET_MON_STA_13(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_13_FLD_MON_STA_13, (reg32))

#define MDP_RDMA_MON_STA_14_GET_MON_STA_14(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_14_FLD_MON_STA_14, (reg32))

#define MDP_RDMA_MON_STA_15_GET_MON_STA_15(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_15_FLD_MON_STA_15, (reg32))

#define MDP_RDMA_MON_STA_16_GET_MON_STA_16(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_16_FLD_MON_STA_16, (reg32))

#define MDP_RDMA_MON_STA_17_GET_MON_STA_17(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_17_FLD_MON_STA_17, (reg32))

#define MDP_RDMA_MON_STA_18_GET_MON_STA_18(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_18_FLD_MON_STA_18, (reg32))

#define MDP_RDMA_MON_STA_19_GET_MON_STA_19(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_19_FLD_MON_STA_19, (reg32))

#define MDP_RDMA_MON_STA_20_GET_MON_STA_20(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_20_FLD_MON_STA_20, (reg32))

#define MDP_RDMA_MON_STA_21_GET_MON_STA_21(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_21_FLD_MON_STA_21, (reg32))

#define MDP_RDMA_MON_STA_22_GET_MON_STA_22(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_22_FLD_MON_STA_22, (reg32))

#define MDP_RDMA_MON_STA_23_GET_MON_STA_23(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_23_FLD_MON_STA_23, (reg32))

#define MDP_RDMA_MON_STA_24_GET_MON_STA_24(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_24_FLD_MON_STA_24, (reg32))

#define MDP_RDMA_MON_STA_25_GET_MON_STA_25(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_25_FLD_MON_STA_25, (reg32))

#define MDP_RDMA_MON_STA_26_GET_MON_STA_26(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_26_FLD_MON_STA_26, (reg32))

#define MDP_RDMA_MON_STA_27_GET_MON_STA_27(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_27_FLD_MON_STA_27, (reg32))

#define MDP_RDMA_MON_STA_28_GET_MON_STA_28(reg32)              \
	REG_FLD_GET(MDP_RDMA_MON_STA_28_FLD_MON_STA_28, (reg32))

#define MDP_RDMA_SRC_BASE_0_GET_SRC_BASE_0(reg32)              \
	REG_FLD_GET(MDP_RDMA_SRC_BASE_0_FLD_SRC_BASE_0, (reg32))

#define MDP_RDMA_SRC_BASE_1_GET_SRC_BASE_1(reg32)              \
	REG_FLD_GET(MDP_RDMA_SRC_BASE_1_FLD_SRC_BASE_1, (reg32))

#define MDP_RDMA_SRC_BASE_2_GET_SRC_BASE_2(reg32)              \
	REG_FLD_GET(MDP_RDMA_SRC_BASE_2_FLD_SRC_BASE_2, (reg32))

#define MDP_RDMA_UFO_DEC_LENGTH_BASE_Y_GET_UFO_DEC_Y_LEN_BASE(reg32) \
	REG_FLD_GET(MDP_RDMA_UFO_DEC_LENGTH_BASE_Y_FLD_UFO_DEC_Y_LEN_BASE, \
			(reg32))

#define MDP_RDMA_UFO_DEC_LENGTH_BASE_C_GET_UFO_DEC_C_LEN_BASE(reg32) \
	REG_FLD_GET(MDP_RDMA_UFO_DEC_LENGTH_BASE_C_FLD_UFO_DEC_C_LEN_BASE, \
			(reg32))
#endif

#if defined(REG_FLD_SET)
#define MDP_RDMA_EN_SET_INTERNAL_DCM_EN(reg32, val)            \
	REG_FLD_SET(MDP_RDMA_EN_FLD_INTERNAL_DCM_EN, (reg32), (val))
#define MDP_RDMA_EN_SET_ROT_ENABLE(reg32, val)                 \
	REG_FLD_SET(MDP_RDMA_EN_FLD_ROT_ENABLE, (reg32), (val))

#define MDP_RDMA_UFBDC_DCM_EN_SET_UFBDC_INTERNAL_DCM_EN(reg32, val) \
	REG_FLD_SET(MDP_RDMA_UFBDC_DCM_EN_FLD_UFBDC_INTERNAL_DCM_EN, (reg32), \
			(val))

#define MDP_RDMA_RESET_SET_WARM_RESET(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_RESET_FLD_WARM_RESET, (reg32), (val))

#define MDP_RDMA_INTERRUPT_ENABLE_SET_UNDERRUN_INT_EN(reg32, val) \
	REG_FLD_SET(MDP_RDMA_INTERRUPT_ENABLE_FLD_UNDERRUN_INT_EN, (reg32), \
			(val))
#define MDP_RDMA_INTERRUPT_ENABLE_SET_REG_UPDATE_INT_EN(reg32, val) \
	REG_FLD_SET(MDP_RDMA_INTERRUPT_ENABLE_FLD_REG_UPDATE_INT_EN, (reg32), \
			(val))
#define MDP_RDMA_INTERRUPT_ENABLE_SET_FRAME_COMPLETE_INT_EN(reg32, val) \
	REG_FLD_SET(MDP_RDMA_INTERRUPT_ENABLE_FLD_FRAME_COMPLETE_INT_EN, \
			(reg32), (val))

#define MDP_RDMA_INTERRUPT_STATUS_SET_UNDERRUN_INT(reg32, val) \
	REG_FLD_SET(MDP_RDMA_INTERRUPT_STATUS_FLD_UNDERRUN_INT, (reg32), (val))
#define MDP_RDMA_INTERRUPT_STATUS_SET_REG_UPDATE_INT(reg32, val) \
	REG_FLD_SET(MDP_RDMA_INTERRUPT_STATUS_FLD_REG_UPDATE_INT, (reg32), \
			(val))
#define MDP_RDMA_INTERRUPT_STATUS_SET_FRAME_COMPLETE_INT(reg32, val) \
	REG_FLD_SET(MDP_RDMA_INTERRUPT_STATUS_FLD_FRAME_COMPLETE_INT, (reg32), \
			(val))

#define MDP_RDMA_CON_SET_LB_2B_MODE(reg32, val)                \
	REG_FLD_SET(MDP_RDMA_CON_FLD_LB_2B_MODE, (reg32), (val))
#define MDP_RDMA_CON_SET_BUFFER_MODE(reg32, val)               \
	REG_FLD_SET(MDP_RDMA_CON_FLD_BUFFER_MODE, (reg32), (val))
#define MDP_RDMA_CON_SET_OUTPUT_10B(reg32, val)                \
	REG_FLD_SET(MDP_RDMA_CON_FLD_OUTPUT_10B, (reg32), (val))
#define MDP_RDMA_CON_SET_SIMPLE_MODE(reg32, val)               \
	REG_FLD_SET(MDP_RDMA_CON_FLD_SIMPLE_MODE, (reg32), (val))

#define MDP_RDMA_SHADOW_CTRL_SET_READ_WRK_REG(reg32, val)      \
	REG_FLD_SET(MDP_RDMA_SHADOW_CTRL_FLD_READ_WRK_REG, (reg32), (val))
#define MDP_RDMA_SHADOW_CTRL_SET_BYPASS_SHADOW(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_SHADOW_CTRL_FLD_BYPASS_SHADOW, (reg32), (val))
#define MDP_RDMA_SHADOW_CTRL_SET_FORCE_COMMIT(reg32, val)      \
	REG_FLD_SET(MDP_RDMA_SHADOW_CTRL_FLD_FORCE_COMMIT, (reg32), (val))

#define MDP_RDMA_GMCIF_CON_SET_THROTTLE_PERIOD(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_THROTTLE_PERIOD, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_THROTTLE_EN(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_THROTTLE_EN, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_EXT_ULTRA_EN(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_EXT_ULTRA_EN, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_PRE_ULTRA_EN(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_PRE_ULTRA_EN, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_URGENT_EN(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_URGENT_EN, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_ULTRA_EN(reg32, val)            \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_ULTRA_EN, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_CHANNEL_BOUNDARY(reg32, val)    \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_CHANNEL_BOUNDARY, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_WRITE_REQUEST_TYPE(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_WRITE_REQUEST_TYPE, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_READ_REQUEST_TYPE(reg32, val)   \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_READ_REQUEST_TYPE, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_EXT_PREULTRA_EN(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_EXT_PREULTRA_EN, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_GC_LAST_DISABLE(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_GC_LAST_DISABLE, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_HG_DISABLE(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_HG_DISABLE, (reg32), (val))
#define MDP_RDMA_GMCIF_CON_SET_COMMAND_DIV(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_GMCIF_CON_FLD_COMMAND_DIV, (reg32), (val))

#define MDP_RDMA_SRC_CON_SET_TOP_BOUNDARY(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_TOP_BOUNDARY, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_BOT_BOUNDARY(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_BOT_BOUNDARY, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_LEFT_BOUNDARY(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_LEFT_BOUNDARY, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_RIGHT_BOUNDARY(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_RIGHT_BOUNDARY, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_AVOID_SRAM_DOUBLE_READ(reg32, val) \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_AVOID_SRAM_DOUBLE_READ, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_VUPSAMPLE_OFF(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_VUPSAMPLE_OFF, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_OUTPUT_ARGB(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_OUTPUT_ARGB, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_RING_BUF_READ(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_RING_BUF_READ, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_BLOCK_10BIT_TILE_MODE(reg32, val) \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_BLOCK_10BIT_TILE_MODE, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_BYTE_SWAP(reg32, val)             \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_BYTE_SWAP, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_BIT_NUMBER(reg32, val)            \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_BIT_NUMBER, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_UNIFORM_CONFIG(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_UNIFORM_CONFIG, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_IS_Y_LSB(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_IS_Y_LSB, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_BLOCK(reg32, val)                 \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_BLOCK, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_SWAP(reg32, val)                  \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_SWAP, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_VDO_FIELD(reg32, val)             \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_VDO_FIELD, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_VDO_MODE(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_VDO_MODE, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_LOOSE(reg32, val)                 \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_LOOSE, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_CUS_REP(reg32, val)               \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_CUS_REP, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_COSITE(reg32, val)                \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_COSITE, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_RGB_PAD(reg32, val)               \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_RGB_PAD, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_SRC_SWAP(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_SRC_SWAP, (reg32), (val))
#define MDP_RDMA_SRC_CON_SET_SRC_FORMAT(reg32, val)            \
	REG_FLD_SET(MDP_RDMA_SRC_CON_FLD_SRC_FORMAT, (reg32), (val))

#define MDP_RDMA_COMP_CON_SET_UFO_DEC_EN(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFO_DEC_EN, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFO_2V2H_MODE(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFO_2V2H_MODE, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFO_OFFSET_MODE(reg32, val)      \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFO_OFFSET_MODE, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFO_REMAP_10BIT(reg32, val)      \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFO_REMAP_10BIT, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_LENGTH_TABLE_NOT_REV(reg32, val) \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_LENGTH_TABLE_NOT_REV, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFO_DATA_IN_NOT_REV(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFO_DATA_IN_NOT_REV, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFO_DATA_OUT_NOT_REV(reg32, val) \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFO_DATA_OUT_NOT_REV, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFO_DCP_EN(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFO_DCP_EN, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFO_DCP_10BIT(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFO_DCP_10BIT, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_AFBC_EN(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_AFBC_EN, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_AFBC_YUV_TRANSFORM(reg32, val)   \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_AFBC_YUV_TRANSFORM, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_PVRIC_EN(reg32, val)             \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_PVRIC_EN, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_SHORT_BURST(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_SHORT_BURST, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFBDC_SECURE_MODE(reg32, val)    \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFBDC_SECURE_MODE, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFBDC_HG_DISABLE(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFBDC_HG_DISABLE, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_HYAFBC_EN(reg32, val)            \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_HYAFBC_EN, (reg32), (val))
#define MDP_RDMA_COMP_CON_SET_UFBDC_EN(reg32, val)             \
	REG_FLD_SET(MDP_RDMA_COMP_CON_FLD_UFBDC_EN, (reg32), (val))

#define MDP_RDMA_PVRIC_CRYUVAL_0_SET_PVRIC_CRYVAL0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_PVRIC_CRYUVAL_0_FLD_PVRIC_CRYVAL0, (reg32), (val))
#define MDP_RDMA_PVRIC_CRYUVAL_0_SET_PVRIC_CRUVVAL0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_PVRIC_CRYUVAL_0_FLD_PVRIC_CRUVVAL0, (reg32), (val))

#define MDP_RDMA_PVRIC_CRYUVAL_1_SET_PVRIC_CRYVAL1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_PVRIC_CRYUVAL_1_FLD_PVRIC_CRYVAL1, (reg32), (val))
#define MDP_RDMA_PVRIC_CRYUVAL_1_SET_PVRIC_CRUVVAL1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_PVRIC_CRYUVAL_1_FLD_PVRIC_CRUVVAL1, (reg32), (val))

#define MDP_RDMA_PVRIC_CRCH0123VAL_0_SET_PVRIC_CRCH0123VAL0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_PVRIC_CRCH0123VAL_0_FLD_PVRIC_CRCH0123VAL0, \
			(reg32), (val))

#define MDP_RDMA_PVRIC_CRCH0123VAL_1_SET_PVRIC_CRCH0123VAL1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_PVRIC_CRCH0123VAL_1_FLD_PVRIC_CRCH0123VAL1, \
			(reg32), (val))

#define MDP_RDMA_MF_BKGD_SIZE_IN_BYTE_SET_MF_BKGD_WB(reg32, val) \
	REG_FLD_SET(MDP_RDMA_MF_BKGD_SIZE_IN_BYTE_FLD_MF_BKGD_WB, (reg32), \
			(val))

#define MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_SET_MF_BKGD_WP(reg32, val) \
	REG_FLD_SET(MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_FLD_MF_BKGD_WP, (reg32), \
			(val))

#define MDP_RDMA_MF_SRC_SIZE_SET_MF_SRC_H(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_H, (reg32), (val))
#define MDP_RDMA_MF_SRC_SIZE_SET_MF_SRC_W(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_W, (reg32), (val))

#define MDP_RDMA_MF_CLIP_SIZE_SET_MF_CLIP_H(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_H, (reg32), (val))
#define MDP_RDMA_MF_CLIP_SIZE_SET_MF_CLIP_W(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_W, (reg32), (val))

#define MDP_RDMA_MF_OFFSET_1_SET_MF_OFFSET_H_1(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_H_1, (reg32), (val))
#define MDP_RDMA_MF_OFFSET_1_SET_MF_OFFSET_W_1(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_W_1, (reg32), (val))

#define MDP_RDMA_MF_PAR_SET_MF_SB(reg32, val)                  \
	REG_FLD_SET(MDP_RDMA_MF_PAR_FLD_MF_SB, (reg32), (val))
#define MDP_RDMA_MF_PAR_SET_MF_JUMP(reg32, val)                \
	REG_FLD_SET(MDP_RDMA_MF_PAR_FLD_MF_JUMP, (reg32), (val))

#define MDP_RDMA_SF_BKGD_SIZE_IN_BYTE_SET_SF_BKGD_WB(reg32, val) \
	REG_FLD_SET(MDP_RDMA_SF_BKGD_SIZE_IN_BYTE_FLD_SF_BKGD_WB, (reg32), \
			(val))

#define MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_SET_MF_BKGD_HP(reg32, val) \
	REG_FLD_SET(MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_FLD_MF_BKGD_HP, (reg32), \
			(val))

#define MDP_RDMA_TARGET_LINE_SET_LINE_THRESHOLD(reg32, val)    \
	REG_FLD_SET(MDP_RDMA_TARGET_LINE_FLD_LINE_THRESHOLD, (reg32), (val))
#define MDP_RDMA_TARGET_LINE_SET_TARGET_LINE_EN(reg32, val)    \
	REG_FLD_SET(MDP_RDMA_TARGET_LINE_FLD_TARGET_LINE_EN, (reg32), (val))

#define MDP_RDMA_SF_PAR_SET_SF_SB(reg32, val)                  \
	REG_FLD_SET(MDP_RDMA_SF_PAR_FLD_SF_SB, (reg32), (val))
#define MDP_RDMA_SF_PAR_SET_SF_JUMP(reg32, val)                \
	REG_FLD_SET(MDP_RDMA_SF_PAR_FLD_SF_JUMP, (reg32), (val))

#define MDP_RDMA_MB_DEPTH_SET_MB_DEPTH(reg32, val)             \
	REG_FLD_SET(MDP_RDMA_MB_DEPTH_FLD_MB_DEPTH, (reg32), (val))

#define MDP_RDMA_MB_BASE_SET_MB_BASE(reg32, val)               \
	REG_FLD_SET(MDP_RDMA_MB_BASE_FLD_MB_BASE, (reg32), (val))

#define MDP_RDMA_MB_CON_SET_MB_LP(reg32, val)                  \
	REG_FLD_SET(MDP_RDMA_MB_CON_FLD_MB_LP, (reg32), (val))
#define MDP_RDMA_MB_CON_SET_MB_PPS(reg32, val)                 \
	REG_FLD_SET(MDP_RDMA_MB_CON_FLD_MB_PPS, (reg32), (val))

#define MDP_RDMA_SB_DEPTH_SET_SB_DEPTH(reg32, val)             \
	REG_FLD_SET(MDP_RDMA_SB_DEPTH_FLD_SB_DEPTH, (reg32), (val))

#define MDP_RDMA_SB_BASE_SET_SB_BASE(reg32, val)               \
	REG_FLD_SET(MDP_RDMA_SB_BASE_FLD_SB_BASE, (reg32), (val))

#define MDP_RDMA_SB_CON_SET_SB_LP(reg32, val)                  \
	REG_FLD_SET(MDP_RDMA_SB_CON_FLD_SB_LP, (reg32), (val))
#define MDP_RDMA_SB_CON_SET_SB_PPS(reg32, val)                 \
	REG_FLD_SET(MDP_RDMA_SB_CON_FLD_SB_PPS, (reg32), (val))

#define MDP_RDMA_VC1_RANGE_SET_VC1_MAP_PARA_C(reg32, val)      \
	REG_FLD_SET(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_PARA_C, (reg32), (val))
#define MDP_RDMA_VC1_RANGE_SET_VC1_MAP_PARA_Y(reg32, val)      \
	REG_FLD_SET(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_PARA_Y, (reg32), (val))
#define MDP_RDMA_VC1_RANGE_SET_VC1_MAP_EN(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_EN, (reg32), (val))
#define MDP_RDMA_VC1_RANGE_SET_VC1_RED_EN(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_VC1_RANGE_FLD_VC1_RED_EN, (reg32), (val))

#define MDP_RDMA_SRC_END_0_SET_SRC_END_0(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_SRC_END_0_FLD_SRC_END_0, (reg32), (val))

#define MDP_RDMA_SRC_END_1_SET_SRC_END_1(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_SRC_END_1_FLD_SRC_END_1, (reg32), (val))

#define MDP_RDMA_SRC_END_2_SET_SRC_END_2(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_SRC_END_2_FLD_SRC_END_2, (reg32), (val))

#define MDP_RDMA_SRC_OFFSET_0_SET_SRC_OFFSET_0(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_SRC_OFFSET_0_FLD_SRC_OFFSET_0, (reg32), (val))

#define MDP_RDMA_SRC_OFFSET_1_SET_SRC_OFFSET_1(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_SRC_OFFSET_1_FLD_SRC_OFFSET_1, (reg32), (val))

#define MDP_RDMA_SRC_OFFSET_2_SET_SRC_OFFSET_2(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_SRC_OFFSET_2_FLD_SRC_OFFSET_2, (reg32), (val))

#define MDP_RDMA_SRC_OFFSET_W_0_SET_SRC_OFFSET_W_0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_SRC_OFFSET_W_0_FLD_SRC_OFFSET_W_0, (reg32), (val))

#define MDP_RDMA_SRC_OFFSET_W_1_SET_SRC_OFFSET_W_1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_SRC_OFFSET_W_1_FLD_SRC_OFFSET_W_1, (reg32), (val))

#define MDP_RDMA_SRC_OFFSET_W_2_SET_SRC_OFFSET_W_2(reg32, val) \
	REG_FLD_SET(MDP_RDMA_SRC_OFFSET_W_2_FLD_SRC_OFFSET_W_2, (reg32), (val))

#define MDP_RDMA_SRC_OFFSET_WP_SET_SRC_OFFSET_WP(reg32, val)   \
	REG_FLD_SET(MDP_RDMA_SRC_OFFSET_WP_FLD_SRC_OFFSET_WP, (reg32), (val))

#define MDP_RDMA_SRC_OFFSET_HP_SET_SRC_OFFSET_HP(reg32, val)   \
	REG_FLD_SET(MDP_RDMA_SRC_OFFSET_HP_FLD_SRC_OFFSET_HP, (reg32), (val))

#define MDP_RDMA_C00_SET_MDP_RDMA_C00(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C00_FLD_MDP_RDMA_C00, (reg32), (val))

#define MDP_RDMA_C01_SET_MDP_RDMA_C01(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C01_FLD_MDP_RDMA_C01, (reg32), (val))

#define MDP_RDMA_C02_SET_MDP_RDMA_C02(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C02_FLD_MDP_RDMA_C02, (reg32), (val))

#define MDP_RDMA_C10_SET_MDP_RDMA_C10(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C10_FLD_MDP_RDMA_C10, (reg32), (val))

#define MDP_RDMA_C11_SET_MDP_RDMA_C11(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C11_FLD_MDP_RDMA_C11, (reg32), (val))

#define MDP_RDMA_C12_SET_MDP_RDMA_C12(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C12_FLD_MDP_RDMA_C12, (reg32), (val))

#define MDP_RDMA_C20_SET_MDP_RDMA_C20(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C20_FLD_MDP_RDMA_C20, (reg32), (val))

#define MDP_RDMA_C21_SET_MDP_RDMA_C21(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C21_FLD_MDP_RDMA_C21, (reg32), (val))

#define MDP_RDMA_C22_SET_MDP_RDMA_C22(reg32, val)              \
	REG_FLD_SET(MDP_RDMA_C22_FLD_MDP_RDMA_C22, (reg32), (val))

#define MDP_RDMA_PRE_ADD_0_SET_MDP_RDMA_PRE_ADD_0(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_PRE_ADD_0_FLD_MDP_RDMA_PRE_ADD_0, (reg32), (val))

#define MDP_RDMA_PRE_ADD_1_SET_MDP_RDMA_PRE_ADD_1(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_PRE_ADD_1_FLD_MDP_RDMA_PRE_ADD_1, (reg32), (val))

#define MDP_RDMA_PRE_ADD_2_SET_MDP_RDMA_PRE_ADD_2(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_PRE_ADD_2_FLD_MDP_RDMA_PRE_ADD_2, (reg32), (val))

#define MDP_RDMA_POST_ADD_0_SET_MDP_RDMA_POST_ADD_0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_POST_ADD_0_FLD_MDP_RDMA_POST_ADD_0, (reg32), (val))

#define MDP_RDMA_POST_ADD_1_SET_MDP_RDMA_POST_ADD_1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_POST_ADD_1_FLD_MDP_RDMA_POST_ADD_1, (reg32), (val))

#define MDP_RDMA_POST_ADD_2_SET_MDP_RDMA_POST_ADD_2(reg32, val) \
	REG_FLD_SET(MDP_RDMA_POST_ADD_2_FLD_MDP_RDMA_POST_ADD_2, (reg32), (val))

#define MDP_RDMA_TRANSFORM_0_SET_CSC_ENABLE(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_TRANSFORM_0_FLD_CSC_ENABLE, (reg32), (val))
#define MDP_RDMA_TRANSFORM_0_SET_CSC_FORMAT(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_TRANSFORM_0_FLD_CSC_FORMAT, (reg32), (val))
#define MDP_RDMA_TRANSFORM_0_SET_int_matrix_sel(reg32, val)    \
	REG_FLD_SET(MDP_RDMA_TRANSFORM_0_FLD_int_matrix_sel, (reg32), (val))
#define MDP_RDMA_TRANSFORM_0_SET_ext_matrix_en(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_TRANSFORM_0_FLD_ext_matrix_en, (reg32), (val))
#define MDP_RDMA_TRANSFORM_0_SET_TRANS_EN(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_TRANSFORM_0_FLD_TRANS_EN, (reg32), (val))
#define MDP_RDMA_TRANSFORM_0_SET_BITEXTEND_WITH_ZERO(reg32, val) \
	REG_FLD_SET(MDP_RDMA_TRANSFORM_0_FLD_BITEXTEND_WITH_ZERO, (reg32), \
			(val))

#define MDP_RDMA_DMABUF_CON_0_SET_EXTRD_ARB_MAX_0(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_0_FLD_EXTRD_ARB_MAX_0, (reg32), (val))
#define MDP_RDMA_DMABUF_CON_0_SET_BUF_RESV_SIZE_0(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_0_FLD_BUF_RESV_SIZE_0, (reg32), (val))
#define MDP_RDMA_DMABUF_CON_0_SET_ISSUE_REQ_TH_0(reg32, val)   \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_0_FLD_ISSUE_REQ_TH_0, (reg32), (val))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_SET_URGENT_TH_HIGH_OFS_0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_URGENT_TH_HIGH_OFS_0, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_SET_PRE_ULTRA_TH_HIGH_OFS_0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_PRE_ULTRA_TH_HIGH_OFS_0, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_SET_ULTRA_TH_HIGH_OFS_0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_ULTRA_TH_HIGH_OFS_0, \
			(reg32), (val))

#define MDP_RDMA_ULTRA_TH_LOW_CON_0_SET_URGENT_TH_LOW_OFS_0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_URGENT_TH_LOW_OFS_0, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_0_SET_PRE_ULTRA_TH_LOW_OFS_0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_PRE_ULTRA_TH_LOW_OFS_0, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_0_SET_ULTRA_TH_LOW_OFS_0(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_ULTRA_TH_LOW_OFS_0, \
			(reg32), (val))

#define MDP_RDMA_DMABUF_CON_1_SET_EXTRD_ARB_MAX_1(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_1_FLD_EXTRD_ARB_MAX_1, (reg32), (val))
#define MDP_RDMA_DMABUF_CON_1_SET_BUF_RESV_SIZE_1(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_1_FLD_BUF_RESV_SIZE_1, (reg32), (val))
#define MDP_RDMA_DMABUF_CON_1_SET_ISSUE_REQ_TH_1(reg32, val)   \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_1_FLD_ISSUE_REQ_TH_1, (reg32), (val))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_SET_URGENT_TH_HIGH_OFS_1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_URGENT_TH_HIGH_OFS_1, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_SET_PRE_ULTRA_TH_HIGH_OFS_1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_PRE_ULTRA_TH_HIGH_OFS_1, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_SET_ULTRA_TH_HIGH_OFS_1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_ULTRA_TH_HIGH_OFS_1, \
			(reg32), (val))

#define MDP_RDMA_ULTRA_TH_LOW_CON_1_SET_URGENT_TH_LOW_OFS_1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_URGENT_TH_LOW_OFS_1, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_1_SET_PRE_ULTRA_TH_LOW_OFS_1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_PRE_ULTRA_TH_LOW_OFS_1, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_1_SET_ULTRA_TH_LOW_OFS_1(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_ULTRA_TH_LOW_OFS_1, \
			(reg32), (val))

#define MDP_RDMA_DMABUF_CON_2_SET_EXTRD_ARB_MAX_2(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_2_FLD_EXTRD_ARB_MAX_2, (reg32), (val))
#define MDP_RDMA_DMABUF_CON_2_SET_BUF_RESV_SIZE_2(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_2_FLD_BUF_RESV_SIZE_2, (reg32), (val))
#define MDP_RDMA_DMABUF_CON_2_SET_ISSUE_REQ_TH_2(reg32, val)   \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_2_FLD_ISSUE_REQ_TH_2, (reg32), (val))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_SET_URGENT_TH_HIGH_OFS_2(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_URGENT_TH_HIGH_OFS_2, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_SET_PRE_ULTRA_TH_HIGH_OFS_2(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_PRE_ULTRA_TH_HIGH_OFS_2, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_SET_ULTRA_TH_HIGH_OFS_2(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_ULTRA_TH_HIGH_OFS_2, \
			(reg32), (val))

#define MDP_RDMA_ULTRA_TH_LOW_CON_2_SET_URGENT_TH_LOW_OFS_2(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_URGENT_TH_LOW_OFS_2, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_2_SET_PRE_ULTRA_TH_LOW_OFS_2(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_PRE_ULTRA_TH_LOW_OFS_2, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_2_SET_ULTRA_TH_LOW_OFS_2(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_ULTRA_TH_LOW_OFS_2, \
			(reg32), (val))

#define MDP_RDMA_DMABUF_CON_3_SET_EXTRD_ARB_MAX_3(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_3_FLD_EXTRD_ARB_MAX_3, (reg32), (val))
#define MDP_RDMA_DMABUF_CON_3_SET_BUF_RESV_SIZE_3(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_3_FLD_BUF_RESV_SIZE_3, (reg32), (val))
#define MDP_RDMA_DMABUF_CON_3_SET_ISSUE_REQ_TH_3(reg32, val)   \
	REG_FLD_SET(MDP_RDMA_DMABUF_CON_3_FLD_ISSUE_REQ_TH_3, (reg32), (val))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_SET_URGENT_TH_HIGH_OFS_3(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_URGENT_TH_HIGH_OFS_3, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_SET_PRE_ULTRA_TH_HIGH_OFS_3(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_PRE_ULTRA_TH_HIGH_OFS_3, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_SET_ULTRA_TH_HIGH_OFS_3(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_ULTRA_TH_HIGH_OFS_3, \
			(reg32), (val))

#define MDP_RDMA_ULTRA_TH_LOW_CON_3_SET_URGENT_TH_LOW_OFS_3(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_URGENT_TH_LOW_OFS_3, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_3_SET_PRE_ULTRA_TH_LOW_OFS_3(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_PRE_ULTRA_TH_LOW_OFS_3, \
			(reg32), (val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_3_SET_ULTRA_TH_LOW_OFS_3(reg32, val) \
	REG_FLD_SET(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_ULTRA_TH_LOW_OFS_3, \
			(reg32), (val))

#define MDP_RDMA_DITHER_CON_SET_DITHER_INIT_H_POS(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DITHER_CON_FLD_DITHER_INIT_H_POS, (reg32), (val))
#define MDP_RDMA_DITHER_CON_SET_DITHER_INIT_V_POS(reg32, val)  \
	REG_FLD_SET(MDP_RDMA_DITHER_CON_FLD_DITHER_INIT_V_POS, (reg32), (val))
#define MDP_RDMA_DITHER_CON_SET_DITHER_EN(reg32, val)          \
	REG_FLD_SET(MDP_RDMA_DITHER_CON_FLD_DITHER_EN, (reg32), (val))

#define MDP_RDMA_RESV_DUMMY_0_SET_RESV_DUMMY_0(reg32, val)     \
	REG_FLD_SET(MDP_RDMA_RESV_DUMMY_0_FLD_RESV_DUMMY_0, (reg32), (val))

#define MDP_RDMA_UNCOMP_MON_SET_COMP_MON_RAW(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_UNCOMP_MON_FLD_COMP_MON_RAW, (reg32), (val))
#define MDP_RDMA_UNCOMP_MON_SET_COMP_MON_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_UNCOMP_MON_FLD_COMP_MON_CLR, (reg32), (val))

#define MDP_RDMA_COMP_MON_SET_COMP_MON_COMP(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_COMP_MON_FLD_COMP_MON_COMP, (reg32), (val))

#define MDP_RDMA_CHKS_EXTR_SET_CHKS_EXTR_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_EXTR_FLD_CHKS_EXTR_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_EXTR_SET_CHKS_EXTR_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_EXTR_FLD_CHKS_EXTR_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_INTW_SET_CHKS_INTW_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_INTW_FLD_CHKS_INTW_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_INTW_SET_CHKS_INTW_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_INTW_FLD_CHKS_INTW_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_INTR_SET_CHKS_INTR_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_INTR_FLD_CHKS_INTR_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_INTR_SET_CHKS_INTR_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_INTR_FLD_CHKS_INTR_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_ROTO_SET_CHKS_ROTO_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_ROTO_FLD_CHKS_ROTO_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_ROTO_SET_CHKS_ROTO_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_ROTO_FLD_CHKS_ROTO_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_SRIY_SET_CHKS_SRIY_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SRIY_FLD_CHKS_SRIY_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_SRIY_SET_CHKS_SRIY_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SRIY_FLD_CHKS_SRIY_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_SRIU_SET_CHKS_SRIU_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SRIU_FLD_CHKS_SRIU_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_SRIU_SET_CHKS_SRIU_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SRIU_FLD_CHKS_SRIU_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_SRIV_SET_CHKS_SRIV_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SRIV_FLD_CHKS_SRIV_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_SRIV_SET_CHKS_SRIV_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SRIV_FLD_CHKS_SRIV_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_SROY_SET_CHKS_SROY_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SROY_FLD_CHKS_SROY_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_SROY_SET_CHKS_SROY_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SROY_FLD_CHKS_SROY_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_SROU_SET_CHKS_SROU_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SROU_FLD_CHKS_SROU_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_SROU_SET_CHKS_SROU_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SROU_FLD_CHKS_SROU_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_SROV_SET_CHKS_SROV_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SROV_FLD_CHKS_SROV_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_SROV_SET_CHKS_SROV_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_SROV_FLD_CHKS_SROV_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_VUPI_SET_CHKS_VUPI_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_VUPI_FLD_CHKS_VUPI_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_VUPI_SET_CHKS_VUPI_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_VUPI_FLD_CHKS_VUPI_CLR, (reg32), (val))

#define MDP_RDMA_CHKS_VUPO_SET_CHKS_VUPO_CRC(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_VUPO_FLD_CHKS_VUPO_CRC, (reg32), (val))
#define MDP_RDMA_CHKS_VUPO_SET_CHKS_VUPO_CLR(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_CHKS_VUPO_FLD_CHKS_VUPO_CLR, (reg32), (val))

#define MDP_RDMA_DEBUG_CON_SET_ufbdc_debug_out_sel(reg32, val) \
	REG_FLD_SET(MDP_RDMA_DEBUG_CON_FLD_ufbdc_debug_out_sel, (reg32), (val))
#define MDP_RDMA_DEBUG_CON_SET_debug_out_sel(reg32, val)       \
	REG_FLD_SET(MDP_RDMA_DEBUG_CON_FLD_debug_out_sel, (reg32), (val))
#define MDP_RDMA_DEBUG_CON_SET_CHKS_TRB_SEL(reg32, val)        \
	REG_FLD_SET(MDP_RDMA_DEBUG_CON_FLD_CHKS_TRB_SEL, (reg32), (val))
#define MDP_RDMA_DEBUG_CON_SET_CHKS_SEL(reg32, val)            \
	REG_FLD_SET(MDP_RDMA_DEBUG_CON_FLD_CHKS_SEL, (reg32), (val))
#define MDP_RDMA_DEBUG_CON_SET_CHKS_CRC_EN(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_DEBUG_CON_FLD_CHKS_CRC_EN, (reg32), (val))

#define MDP_RDMA_MON_STA_0_SET_MON_STA_0(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_0_FLD_MON_STA_0, (reg32), (val))

#define MDP_RDMA_MON_STA_1_SET_MON_STA_1(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_1_FLD_MON_STA_1, (reg32), (val))

#define MDP_RDMA_MON_STA_2_SET_MON_STA_2(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_2_FLD_MON_STA_2, (reg32), (val))

#define MDP_RDMA_MON_STA_3_SET_MON_STA_3(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_3_FLD_MON_STA_3, (reg32), (val))

#define MDP_RDMA_MON_STA_4_SET_MON_STA_4(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_4_FLD_MON_STA_4, (reg32), (val))

#define MDP_RDMA_MON_STA_5_SET_MON_STA_5(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_5_FLD_MON_STA_5, (reg32), (val))

#define MDP_RDMA_MON_STA_6_SET_MON_STA_6(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_6_FLD_MON_STA_6, (reg32), (val))

#define MDP_RDMA_MON_STA_7_SET_MON_STA_7(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_7_FLD_MON_STA_7, (reg32), (val))

#define MDP_RDMA_MON_STA_8_SET_MON_STA_8(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_8_FLD_MON_STA_8, (reg32), (val))

#define MDP_RDMA_MON_STA_9_SET_MON_STA_9(reg32, val)           \
	REG_FLD_SET(MDP_RDMA_MON_STA_9_FLD_MON_STA_9, (reg32), (val))

#define MDP_RDMA_MON_STA_10_SET_MON_STA_10(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_10_FLD_MON_STA_10, (reg32), (val))

#define MDP_RDMA_MON_STA_11_SET_MON_STA_11(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_11_FLD_MON_STA_11, (reg32), (val))

#define MDP_RDMA_MON_STA_12_SET_MON_STA_12(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_12_FLD_MON_STA_12, (reg32), (val))

#define MDP_RDMA_MON_STA_13_SET_MON_STA_13(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_13_FLD_MON_STA_13, (reg32), (val))

#define MDP_RDMA_MON_STA_14_SET_MON_STA_14(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_14_FLD_MON_STA_14, (reg32), (val))

#define MDP_RDMA_MON_STA_15_SET_MON_STA_15(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_15_FLD_MON_STA_15, (reg32), (val))

#define MDP_RDMA_MON_STA_16_SET_MON_STA_16(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_16_FLD_MON_STA_16, (reg32), (val))

#define MDP_RDMA_MON_STA_17_SET_MON_STA_17(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_17_FLD_MON_STA_17, (reg32), (val))

#define MDP_RDMA_MON_STA_18_SET_MON_STA_18(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_18_FLD_MON_STA_18, (reg32), (val))

#define MDP_RDMA_MON_STA_19_SET_MON_STA_19(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_19_FLD_MON_STA_19, (reg32), (val))

#define MDP_RDMA_MON_STA_20_SET_MON_STA_20(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_20_FLD_MON_STA_20, (reg32), (val))

#define MDP_RDMA_MON_STA_21_SET_MON_STA_21(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_21_FLD_MON_STA_21, (reg32), (val))

#define MDP_RDMA_MON_STA_22_SET_MON_STA_22(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_22_FLD_MON_STA_22, (reg32), (val))

#define MDP_RDMA_MON_STA_23_SET_MON_STA_23(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_23_FLD_MON_STA_23, (reg32), (val))

#define MDP_RDMA_MON_STA_24_SET_MON_STA_24(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_24_FLD_MON_STA_24, (reg32), (val))

#define MDP_RDMA_MON_STA_25_SET_MON_STA_25(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_25_FLD_MON_STA_25, (reg32), (val))

#define MDP_RDMA_MON_STA_26_SET_MON_STA_26(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_26_FLD_MON_STA_26, (reg32), (val))

#define MDP_RDMA_MON_STA_27_SET_MON_STA_27(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_27_FLD_MON_STA_27, (reg32), (val))

#define MDP_RDMA_MON_STA_28_SET_MON_STA_28(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_MON_STA_28_FLD_MON_STA_28, (reg32), (val))

#define MDP_RDMA_SRC_BASE_0_SET_SRC_BASE_0(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_SRC_BASE_0_FLD_SRC_BASE_0, (reg32), (val))

#define MDP_RDMA_SRC_BASE_1_SET_SRC_BASE_1(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_SRC_BASE_1_FLD_SRC_BASE_1, (reg32), (val))

#define MDP_RDMA_SRC_BASE_2_SET_SRC_BASE_2(reg32, val)         \
	REG_FLD_SET(MDP_RDMA_SRC_BASE_2_FLD_SRC_BASE_2, (reg32), (val))

#define MDP_RDMA_UFO_DEC_LENGTH_BASE_Y_SET_UFO_DEC_Y_LEN_BASE(reg32, val) \
	REG_FLD_SET(MDP_RDMA_UFO_DEC_LENGTH_BASE_Y_FLD_UFO_DEC_Y_LEN_BASE, \
			(reg32), (val))

#define MDP_RDMA_UFO_DEC_LENGTH_BASE_C_SET_UFO_DEC_C_LEN_BASE(reg32, val) \
	REG_FLD_SET(MDP_RDMA_UFO_DEC_LENGTH_BASE_C_FLD_UFO_DEC_C_LEN_BASE, \
			(reg32), (val))
#endif

#if defined(REG_FLD_VAL)
#define MDP_RDMA_EN_VAL_INTERNAL_DCM_EN(val)                   \
	REG_FLD_VAL(MDP_RDMA_EN_FLD_INTERNAL_DCM_EN, (val))
#define MDP_RDMA_EN_VAL_ROT_ENABLE(val)                        \
	REG_FLD_VAL(MDP_RDMA_EN_FLD_ROT_ENABLE, (val))

#define MDP_RDMA_UFBDC_DCM_EN_VAL_UFBDC_INTERNAL_DCM_EN(val)   \
	REG_FLD_VAL(MDP_RDMA_UFBDC_DCM_EN_FLD_UFBDC_INTERNAL_DCM_EN, (val))

#define MDP_RDMA_RESET_VAL_WARM_RESET(val)                     \
	REG_FLD_VAL(MDP_RDMA_RESET_FLD_WARM_RESET, (val))

#define MDP_RDMA_INTERRUPT_ENABLE_VAL_UNDERRUN_INT_EN(val)     \
	REG_FLD_VAL(MDP_RDMA_INTERRUPT_ENABLE_FLD_UNDERRUN_INT_EN, (val))
#define MDP_RDMA_INTERRUPT_ENABLE_VAL_REG_UPDATE_INT_EN(val)   \
	REG_FLD_VAL(MDP_RDMA_INTERRUPT_ENABLE_FLD_REG_UPDATE_INT_EN, (val))
#define MDP_RDMA_INTERRUPT_ENABLE_VAL_FRAME_COMPLETE_INT_EN(val) \
	REG_FLD_VAL(MDP_RDMA_INTERRUPT_ENABLE_FLD_FRAME_COMPLETE_INT_EN, (val))

#define MDP_RDMA_INTERRUPT_STATUS_VAL_UNDERRUN_INT(val)        \
	REG_FLD_VAL(MDP_RDMA_INTERRUPT_STATUS_FLD_UNDERRUN_INT, (val))
#define MDP_RDMA_INTERRUPT_STATUS_VAL_REG_UPDATE_INT(val)      \
	REG_FLD_VAL(MDP_RDMA_INTERRUPT_STATUS_FLD_REG_UPDATE_INT, (val))
#define MDP_RDMA_INTERRUPT_STATUS_VAL_FRAME_COMPLETE_INT(val)  \
	REG_FLD_VAL(MDP_RDMA_INTERRUPT_STATUS_FLD_FRAME_COMPLETE_INT, (val))

#define MDP_RDMA_CON_VAL_LB_2B_MODE(val)                       \
	REG_FLD_VAL(MDP_RDMA_CON_FLD_LB_2B_MODE, (val))
#define MDP_RDMA_CON_VAL_BUFFER_MODE(val)                      \
	REG_FLD_VAL(MDP_RDMA_CON_FLD_BUFFER_MODE, (val))
#define MDP_RDMA_CON_VAL_OUTPUT_10B(val)                       \
	REG_FLD_VAL(MDP_RDMA_CON_FLD_OUTPUT_10B, (val))
#define MDP_RDMA_CON_VAL_SIMPLE_MODE(val)                      \
	REG_FLD_VAL(MDP_RDMA_CON_FLD_SIMPLE_MODE, (val))

#define MDP_RDMA_SHADOW_CTRL_VAL_READ_WRK_REG(val)             \
	REG_FLD_VAL(MDP_RDMA_SHADOW_CTRL_FLD_READ_WRK_REG, (val))
#define MDP_RDMA_SHADOW_CTRL_VAL_BYPASS_SHADOW(val)            \
	REG_FLD_VAL(MDP_RDMA_SHADOW_CTRL_FLD_BYPASS_SHADOW, (val))
#define MDP_RDMA_SHADOW_CTRL_VAL_FORCE_COMMIT(val)             \
	REG_FLD_VAL(MDP_RDMA_SHADOW_CTRL_FLD_FORCE_COMMIT, (val))

#define MDP_RDMA_GMCIF_CON_VAL_THROTTLE_PERIOD(val)            \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_THROTTLE_PERIOD, (val))
#define MDP_RDMA_GMCIF_CON_VAL_THROTTLE_EN(val)                \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_THROTTLE_EN, (val))
#define MDP_RDMA_GMCIF_CON_VAL_EXT_ULTRA_EN(val)               \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_EXT_ULTRA_EN, (val))
#define MDP_RDMA_GMCIF_CON_VAL_PRE_ULTRA_EN(val)               \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_PRE_ULTRA_EN, (val))
#define MDP_RDMA_GMCIF_CON_VAL_URGENT_EN(val)                  \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_URGENT_EN, (val))
#define MDP_RDMA_GMCIF_CON_VAL_ULTRA_EN(val)                   \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_ULTRA_EN, (val))
#define MDP_RDMA_GMCIF_CON_VAL_CHANNEL_BOUNDARY(val)           \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_CHANNEL_BOUNDARY, (val))
#define MDP_RDMA_GMCIF_CON_VAL_WRITE_REQUEST_TYPE(val)         \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_WRITE_REQUEST_TYPE, (val))
#define MDP_RDMA_GMCIF_CON_VAL_READ_REQUEST_TYPE(val)          \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_READ_REQUEST_TYPE, (val))
#define MDP_RDMA_GMCIF_CON_VAL_EXT_PREULTRA_EN(val)            \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_EXT_PREULTRA_EN, (val))
#define MDP_RDMA_GMCIF_CON_VAL_GC_LAST_DISABLE(val)            \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_GC_LAST_DISABLE, (val))
#define MDP_RDMA_GMCIF_CON_VAL_HG_DISABLE(val)                 \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_HG_DISABLE, (val))
#define MDP_RDMA_GMCIF_CON_VAL_COMMAND_DIV(val)                \
	REG_FLD_VAL(MDP_RDMA_GMCIF_CON_FLD_COMMAND_DIV, (val))

#define MDP_RDMA_SRC_CON_VAL_TOP_BOUNDARY(val)                 \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_TOP_BOUNDARY, (val))
#define MDP_RDMA_SRC_CON_VAL_BOT_BOUNDARY(val)                 \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_BOT_BOUNDARY, (val))
#define MDP_RDMA_SRC_CON_VAL_LEFT_BOUNDARY(val)                \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_LEFT_BOUNDARY, (val))
#define MDP_RDMA_SRC_CON_VAL_RIGHT_BOUNDARY(val)               \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_RIGHT_BOUNDARY, (val))
#define MDP_RDMA_SRC_CON_VAL_AVOID_SRAM_DOUBLE_READ(val)       \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_AVOID_SRAM_DOUBLE_READ, (val))
#define MDP_RDMA_SRC_CON_VAL_VUPSAMPLE_OFF(val)                \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_VUPSAMPLE_OFF, (val))
#define MDP_RDMA_SRC_CON_VAL_OUTPUT_ARGB(val)                  \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_OUTPUT_ARGB, (val))
#define MDP_RDMA_SRC_CON_VAL_RING_BUF_READ(val)                \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_RING_BUF_READ, (val))
#define MDP_RDMA_SRC_CON_VAL_BLOCK_10BIT_TILE_MODE(val)        \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_BLOCK_10BIT_TILE_MODE, (val))
#define MDP_RDMA_SRC_CON_VAL_BYTE_SWAP(val)                    \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_BYTE_SWAP, (val))
#define MDP_RDMA_SRC_CON_VAL_BIT_NUMBER(val)                   \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_BIT_NUMBER, (val))
#define MDP_RDMA_SRC_CON_VAL_UNIFORM_CONFIG(val)               \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_UNIFORM_CONFIG, (val))
#define MDP_RDMA_SRC_CON_VAL_IS_Y_LSB(val)                     \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_IS_Y_LSB, (val))
#define MDP_RDMA_SRC_CON_VAL_BLOCK(val)                        \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_BLOCK, (val))
#define MDP_RDMA_SRC_CON_VAL_SWAP(val)                         \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_SWAP, (val))
#define MDP_RDMA_SRC_CON_VAL_VDO_FIELD(val)                    \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_VDO_FIELD, (val))
#define MDP_RDMA_SRC_CON_VAL_VDO_MODE(val)                     \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_VDO_MODE, (val))
#define MDP_RDMA_SRC_CON_VAL_LOOSE(val)                        \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_LOOSE, (val))
#define MDP_RDMA_SRC_CON_VAL_CUS_REP(val)                      \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_CUS_REP, (val))
#define MDP_RDMA_SRC_CON_VAL_COSITE(val)                       \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_COSITE, (val))
#define MDP_RDMA_SRC_CON_VAL_RGB_PAD(val)                      \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_RGB_PAD, (val))
#define MDP_RDMA_SRC_CON_VAL_SRC_SWAP(val)                     \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_SRC_SWAP, (val))
#define MDP_RDMA_SRC_CON_VAL_SRC_FORMAT(val)                   \
	REG_FLD_VAL(MDP_RDMA_SRC_CON_FLD_SRC_FORMAT, (val))

#define MDP_RDMA_COMP_CON_VAL_UFO_DEC_EN(val)                  \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFO_DEC_EN, (val))
#define MDP_RDMA_COMP_CON_VAL_UFO_2V2H_MODE(val)               \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFO_2V2H_MODE, (val))
#define MDP_RDMA_COMP_CON_VAL_UFO_OFFSET_MODE(val)             \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFO_OFFSET_MODE, (val))
#define MDP_RDMA_COMP_CON_VAL_UFO_REMAP_10BIT(val)             \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFO_REMAP_10BIT, (val))
#define MDP_RDMA_COMP_CON_VAL_LENGTH_TABLE_NOT_REV(val)        \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_LENGTH_TABLE_NOT_REV, (val))
#define MDP_RDMA_COMP_CON_VAL_UFO_DATA_IN_NOT_REV(val)         \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFO_DATA_IN_NOT_REV, (val))
#define MDP_RDMA_COMP_CON_VAL_UFO_DATA_OUT_NOT_REV(val)        \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFO_DATA_OUT_NOT_REV, (val))
#define MDP_RDMA_COMP_CON_VAL_UFO_DCP_EN(val)                  \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFO_DCP_EN, (val))
#define MDP_RDMA_COMP_CON_VAL_UFO_DCP_10BIT(val)               \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFO_DCP_10BIT, (val))
#define MDP_RDMA_COMP_CON_VAL_AFBC_EN(val)                     \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_AFBC_EN, (val))
#define MDP_RDMA_COMP_CON_VAL_AFBC_YUV_TRANSFORM(val)          \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_AFBC_YUV_TRANSFORM, (val))
#define MDP_RDMA_COMP_CON_VAL_PVRIC_EN(val)                    \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_PVRIC_EN, (val))
#define MDP_RDMA_COMP_CON_VAL_SHORT_BURST(val)                 \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_SHORT_BURST, (val))
#define MDP_RDMA_COMP_CON_VAL_UFBDC_SECURE_MODE(val)           \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFBDC_SECURE_MODE, (val))
#define MDP_RDMA_COMP_CON_VAL_UFBDC_HG_DISABLE(val)            \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFBDC_HG_DISABLE, (val))
#define MDP_RDMA_COMP_CON_VAL_HYAFBC_EN(val)                   \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_HYAFBC_EN, (val))
#define MDP_RDMA_COMP_CON_VAL_UFBDC_EN(val)                    \
	REG_FLD_VAL(MDP_RDMA_COMP_CON_FLD_UFBDC_EN, (val))

#define MDP_RDMA_PVRIC_CRYUVAL_0_VAL_PVRIC_CRYVAL0(val)        \
	REG_FLD_VAL(MDP_RDMA_PVRIC_CRYUVAL_0_FLD_PVRIC_CRYVAL0, (val))
#define MDP_RDMA_PVRIC_CRYUVAL_0_VAL_PVRIC_CRUVVAL0(val)       \
	REG_FLD_VAL(MDP_RDMA_PVRIC_CRYUVAL_0_FLD_PVRIC_CRUVVAL0, (val))

#define MDP_RDMA_PVRIC_CRYUVAL_1_VAL_PVRIC_CRYVAL1(val)        \
	REG_FLD_VAL(MDP_RDMA_PVRIC_CRYUVAL_1_FLD_PVRIC_CRYVAL1, (val))
#define MDP_RDMA_PVRIC_CRYUVAL_1_VAL_PVRIC_CRUVVAL1(val)       \
	REG_FLD_VAL(MDP_RDMA_PVRIC_CRYUVAL_1_FLD_PVRIC_CRUVVAL1, (val))

#define MDP_RDMA_PVRIC_CRCH0123VAL_0_VAL_PVRIC_CRCH0123VAL0(val) \
	REG_FLD_VAL(MDP_RDMA_PVRIC_CRCH0123VAL_0_FLD_PVRIC_CRCH0123VAL0, (val))

#define MDP_RDMA_PVRIC_CRCH0123VAL_1_VAL_PVRIC_CRCH0123VAL1(val) \
	REG_FLD_VAL(MDP_RDMA_PVRIC_CRCH0123VAL_1_FLD_PVRIC_CRCH0123VAL1, (val))

#define MDP_RDMA_MF_BKGD_SIZE_IN_BYTE_VAL_MF_BKGD_WB(val)      \
	REG_FLD_VAL(MDP_RDMA_MF_BKGD_SIZE_IN_BYTE_FLD_MF_BKGD_WB, (val))

#define MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_VAL_MF_BKGD_WP(val)     \
	REG_FLD_VAL(MDP_RDMA_MF_BKGD_SIZE_IN_PIXEL_FLD_MF_BKGD_WP, (val))

#define MDP_RDMA_MF_SRC_SIZE_VAL_MF_SRC_H(val)                 \
	REG_FLD_VAL(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_H, (val))
#define MDP_RDMA_MF_SRC_SIZE_VAL_MF_SRC_W(val)                 \
	REG_FLD_VAL(MDP_RDMA_MF_SRC_SIZE_FLD_MF_SRC_W, (val))

#define MDP_RDMA_MF_CLIP_SIZE_VAL_MF_CLIP_H(val)               \
	REG_FLD_VAL(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_H, (val))
#define MDP_RDMA_MF_CLIP_SIZE_VAL_MF_CLIP_W(val)               \
	REG_FLD_VAL(MDP_RDMA_MF_CLIP_SIZE_FLD_MF_CLIP_W, (val))

#define MDP_RDMA_MF_OFFSET_1_VAL_MF_OFFSET_H_1(val)            \
	REG_FLD_VAL(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_H_1, (val))
#define MDP_RDMA_MF_OFFSET_1_VAL_MF_OFFSET_W_1(val)            \
	REG_FLD_VAL(MDP_RDMA_MF_OFFSET_1_FLD_MF_OFFSET_W_1, (val))

#define MDP_RDMA_MF_PAR_VAL_MF_SB(val)                         \
	REG_FLD_VAL(MDP_RDMA_MF_PAR_FLD_MF_SB, (val))
#define MDP_RDMA_MF_PAR_VAL_MF_JUMP(val)                       \
	REG_FLD_VAL(MDP_RDMA_MF_PAR_FLD_MF_JUMP, (val))

#define MDP_RDMA_SF_BKGD_SIZE_IN_BYTE_VAL_SF_BKGD_WB(val)      \
	REG_FLD_VAL(MDP_RDMA_SF_BKGD_SIZE_IN_BYTE_FLD_SF_BKGD_WB, (val))

#define MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_VAL_MF_BKGD_HP(val)   \
	REG_FLD_VAL(MDP_RDMA_MF_BKGD_H_SIZE_IN_PIXEL_FLD_MF_BKGD_HP, (val))

#define MDP_RDMA_TARGET_LINE_VAL_LINE_THRESHOLD(val)           \
	REG_FLD_VAL(MDP_RDMA_TARGET_LINE_FLD_LINE_THRESHOLD, (val))
#define MDP_RDMA_TARGET_LINE_VAL_TARGET_LINE_EN(val)           \
	REG_FLD_VAL(MDP_RDMA_TARGET_LINE_FLD_TARGET_LINE_EN, (val))

#define MDP_RDMA_SF_PAR_VAL_SF_SB(val)                         \
	REG_FLD_VAL(MDP_RDMA_SF_PAR_FLD_SF_SB, (val))
#define MDP_RDMA_SF_PAR_VAL_SF_JUMP(val)                       \
	REG_FLD_VAL(MDP_RDMA_SF_PAR_FLD_SF_JUMP, (val))

#define MDP_RDMA_MB_DEPTH_VAL_MB_DEPTH(val)                    \
	REG_FLD_VAL(MDP_RDMA_MB_DEPTH_FLD_MB_DEPTH, (val))

#define MDP_RDMA_MB_BASE_VAL_MB_BASE(val)                      \
	REG_FLD_VAL(MDP_RDMA_MB_BASE_FLD_MB_BASE, (val))

#define MDP_RDMA_MB_CON_VAL_MB_LP(val)                         \
	REG_FLD_VAL(MDP_RDMA_MB_CON_FLD_MB_LP, (val))
#define MDP_RDMA_MB_CON_VAL_MB_PPS(val)                        \
	REG_FLD_VAL(MDP_RDMA_MB_CON_FLD_MB_PPS, (val))

#define MDP_RDMA_SB_DEPTH_VAL_SB_DEPTH(val)                    \
	REG_FLD_VAL(MDP_RDMA_SB_DEPTH_FLD_SB_DEPTH, (val))

#define MDP_RDMA_SB_BASE_VAL_SB_BASE(val)                      \
	REG_FLD_VAL(MDP_RDMA_SB_BASE_FLD_SB_BASE, (val))

#define MDP_RDMA_SB_CON_VAL_SB_LP(val)                         \
	REG_FLD_VAL(MDP_RDMA_SB_CON_FLD_SB_LP, (val))
#define MDP_RDMA_SB_CON_VAL_SB_PPS(val)                        \
	REG_FLD_VAL(MDP_RDMA_SB_CON_FLD_SB_PPS, (val))

#define MDP_RDMA_VC1_RANGE_VAL_VC1_MAP_PARA_C(val)             \
	REG_FLD_VAL(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_PARA_C, (val))
#define MDP_RDMA_VC1_RANGE_VAL_VC1_MAP_PARA_Y(val)             \
	REG_FLD_VAL(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_PARA_Y, (val))
#define MDP_RDMA_VC1_RANGE_VAL_VC1_MAP_EN(val)                 \
	REG_FLD_VAL(MDP_RDMA_VC1_RANGE_FLD_VC1_MAP_EN, (val))
#define MDP_RDMA_VC1_RANGE_VAL_VC1_RED_EN(val)                 \
	REG_FLD_VAL(MDP_RDMA_VC1_RANGE_FLD_VC1_RED_EN, (val))

#define MDP_RDMA_SRC_END_0_VAL_SRC_END_0(val)                  \
	REG_FLD_VAL(MDP_RDMA_SRC_END_0_FLD_SRC_END_0, (val))

#define MDP_RDMA_SRC_END_1_VAL_SRC_END_1(val)                  \
	REG_FLD_VAL(MDP_RDMA_SRC_END_1_FLD_SRC_END_1, (val))

#define MDP_RDMA_SRC_END_2_VAL_SRC_END_2(val)                  \
	REG_FLD_VAL(MDP_RDMA_SRC_END_2_FLD_SRC_END_2, (val))

#define MDP_RDMA_SRC_OFFSET_0_VAL_SRC_OFFSET_0(val)            \
	REG_FLD_VAL(MDP_RDMA_SRC_OFFSET_0_FLD_SRC_OFFSET_0, (val))

#define MDP_RDMA_SRC_OFFSET_1_VAL_SRC_OFFSET_1(val)            \
	REG_FLD_VAL(MDP_RDMA_SRC_OFFSET_1_FLD_SRC_OFFSET_1, (val))

#define MDP_RDMA_SRC_OFFSET_2_VAL_SRC_OFFSET_2(val)            \
	REG_FLD_VAL(MDP_RDMA_SRC_OFFSET_2_FLD_SRC_OFFSET_2, (val))

#define MDP_RDMA_SRC_OFFSET_W_0_VAL_SRC_OFFSET_W_0(val)        \
	REG_FLD_VAL(MDP_RDMA_SRC_OFFSET_W_0_FLD_SRC_OFFSET_W_0, (val))

#define MDP_RDMA_SRC_OFFSET_W_1_VAL_SRC_OFFSET_W_1(val)        \
	REG_FLD_VAL(MDP_RDMA_SRC_OFFSET_W_1_FLD_SRC_OFFSET_W_1, (val))

#define MDP_RDMA_SRC_OFFSET_W_2_VAL_SRC_OFFSET_W_2(val)        \
	REG_FLD_VAL(MDP_RDMA_SRC_OFFSET_W_2_FLD_SRC_OFFSET_W_2, (val))

#define MDP_RDMA_SRC_OFFSET_WP_VAL_SRC_OFFSET_WP(val)          \
	REG_FLD_VAL(MDP_RDMA_SRC_OFFSET_WP_FLD_SRC_OFFSET_WP, (val))

#define MDP_RDMA_SRC_OFFSET_HP_VAL_SRC_OFFSET_HP(val)          \
	REG_FLD_VAL(MDP_RDMA_SRC_OFFSET_HP_FLD_SRC_OFFSET_HP, (val))

#define MDP_RDMA_C00_VAL_MDP_RDMA_C00(val)                     \
	REG_FLD_VAL(MDP_RDMA_C00_FLD_MDP_RDMA_C00, (val))

#define MDP_RDMA_C01_VAL_MDP_RDMA_C01(val)                     \
	REG_FLD_VAL(MDP_RDMA_C01_FLD_MDP_RDMA_C01, (val))

#define MDP_RDMA_C02_VAL_MDP_RDMA_C02(val)                     \
	REG_FLD_VAL(MDP_RDMA_C02_FLD_MDP_RDMA_C02, (val))

#define MDP_RDMA_C10_VAL_MDP_RDMA_C10(val)                     \
	REG_FLD_VAL(MDP_RDMA_C10_FLD_MDP_RDMA_C10, (val))

#define MDP_RDMA_C11_VAL_MDP_RDMA_C11(val)                     \
	REG_FLD_VAL(MDP_RDMA_C11_FLD_MDP_RDMA_C11, (val))

#define MDP_RDMA_C12_VAL_MDP_RDMA_C12(val)                     \
	REG_FLD_VAL(MDP_RDMA_C12_FLD_MDP_RDMA_C12, (val))

#define MDP_RDMA_C20_VAL_MDP_RDMA_C20(val)                     \
	REG_FLD_VAL(MDP_RDMA_C20_FLD_MDP_RDMA_C20, (val))

#define MDP_RDMA_C21_VAL_MDP_RDMA_C21(val)                     \
	REG_FLD_VAL(MDP_RDMA_C21_FLD_MDP_RDMA_C21, (val))

#define MDP_RDMA_C22_VAL_MDP_RDMA_C22(val)                     \
	REG_FLD_VAL(MDP_RDMA_C22_FLD_MDP_RDMA_C22, (val))

#define MDP_RDMA_PRE_ADD_0_VAL_MDP_RDMA_PRE_ADD_0(val)         \
	REG_FLD_VAL(MDP_RDMA_PRE_ADD_0_FLD_MDP_RDMA_PRE_ADD_0, (val))

#define MDP_RDMA_PRE_ADD_1_VAL_MDP_RDMA_PRE_ADD_1(val)         \
	REG_FLD_VAL(MDP_RDMA_PRE_ADD_1_FLD_MDP_RDMA_PRE_ADD_1, (val))

#define MDP_RDMA_PRE_ADD_2_VAL_MDP_RDMA_PRE_ADD_2(val)         \
	REG_FLD_VAL(MDP_RDMA_PRE_ADD_2_FLD_MDP_RDMA_PRE_ADD_2, (val))

#define MDP_RDMA_POST_ADD_0_VAL_MDP_RDMA_POST_ADD_0(val)       \
	REG_FLD_VAL(MDP_RDMA_POST_ADD_0_FLD_MDP_RDMA_POST_ADD_0, (val))

#define MDP_RDMA_POST_ADD_1_VAL_MDP_RDMA_POST_ADD_1(val)       \
	REG_FLD_VAL(MDP_RDMA_POST_ADD_1_FLD_MDP_RDMA_POST_ADD_1, (val))

#define MDP_RDMA_POST_ADD_2_VAL_MDP_RDMA_POST_ADD_2(val)       \
	REG_FLD_VAL(MDP_RDMA_POST_ADD_2_FLD_MDP_RDMA_POST_ADD_2, (val))

#define MDP_RDMA_TRANSFORM_0_VAL_CSC_ENABLE(val)               \
	REG_FLD_VAL(MDP_RDMA_TRANSFORM_0_FLD_CSC_ENABLE, (val))
#define MDP_RDMA_TRANSFORM_0_VAL_CSC_FORMAT(val)               \
	REG_FLD_VAL(MDP_RDMA_TRANSFORM_0_FLD_CSC_FORMAT, (val))
#define MDP_RDMA_TRANSFORM_0_VAL_int_matrix_sel(val)           \
	REG_FLD_VAL(MDP_RDMA_TRANSFORM_0_FLD_int_matrix_sel, (val))
#define MDP_RDMA_TRANSFORM_0_VAL_ext_matrix_en(val)            \
	REG_FLD_VAL(MDP_RDMA_TRANSFORM_0_FLD_ext_matrix_en, (val))
#define MDP_RDMA_TRANSFORM_0_VAL_TRANS_EN(val)                 \
	REG_FLD_VAL(MDP_RDMA_TRANSFORM_0_FLD_TRANS_EN, (val))
#define MDP_RDMA_TRANSFORM_0_VAL_BITEXTEND_WITH_ZERO(val)      \
	REG_FLD_VAL(MDP_RDMA_TRANSFORM_0_FLD_BITEXTEND_WITH_ZERO, (val))

#define MDP_RDMA_DMABUF_CON_0_VAL_EXTRD_ARB_MAX_0(val)         \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_0_FLD_EXTRD_ARB_MAX_0, (val))
#define MDP_RDMA_DMABUF_CON_0_VAL_BUF_RESV_SIZE_0(val)         \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_0_FLD_BUF_RESV_SIZE_0, (val))
#define MDP_RDMA_DMABUF_CON_0_VAL_ISSUE_REQ_TH_0(val)          \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_0_FLD_ISSUE_REQ_TH_0, (val))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_VAL_URGENT_TH_HIGH_OFS_0(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_URGENT_TH_HIGH_OFS_0, \
			(val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_VAL_PRE_ULTRA_TH_HIGH_OFS_0(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_PRE_ULTRA_TH_HIGH_OFS_0, \
			(val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_0_VAL_ULTRA_TH_HIGH_OFS_0(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_0_FLD_ULTRA_TH_HIGH_OFS_0, \
			(val))

#define MDP_RDMA_ULTRA_TH_LOW_CON_0_VAL_URGENT_TH_LOW_OFS_0(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_URGENT_TH_LOW_OFS_0, \
			(val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_0_VAL_PRE_ULTRA_TH_LOW_OFS_0(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_PRE_ULTRA_TH_LOW_OFS_0, \
			(val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_0_VAL_ULTRA_TH_LOW_OFS_0(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_0_FLD_ULTRA_TH_LOW_OFS_0, \
			(val))

#define MDP_RDMA_DMABUF_CON_1_VAL_EXTRD_ARB_MAX_1(val)         \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_1_FLD_EXTRD_ARB_MAX_1, (val))
#define MDP_RDMA_DMABUF_CON_1_VAL_BUF_RESV_SIZE_1(val)         \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_1_FLD_BUF_RESV_SIZE_1, (val))
#define MDP_RDMA_DMABUF_CON_1_VAL_ISSUE_REQ_TH_1(val)          \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_1_FLD_ISSUE_REQ_TH_1, (val))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_VAL_URGENT_TH_HIGH_OFS_1(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_URGENT_TH_HIGH_OFS_1, \
			(val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_VAL_PRE_ULTRA_TH_HIGH_OFS_1(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_PRE_ULTRA_TH_HIGH_OFS_1, \
			(val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_1_VAL_ULTRA_TH_HIGH_OFS_1(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_1_FLD_ULTRA_TH_HIGH_OFS_1, \
			(val))

#define MDP_RDMA_ULTRA_TH_LOW_CON_1_VAL_URGENT_TH_LOW_OFS_1(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_URGENT_TH_LOW_OFS_1, \
			(val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_1_VAL_PRE_ULTRA_TH_LOW_OFS_1(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_PRE_ULTRA_TH_LOW_OFS_1, \
			(val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_1_VAL_ULTRA_TH_LOW_OFS_1(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_1_FLD_ULTRA_TH_LOW_OFS_1, \
			(val))

#define MDP_RDMA_DMABUF_CON_2_VAL_EXTRD_ARB_MAX_2(val)         \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_2_FLD_EXTRD_ARB_MAX_2, (val))
#define MDP_RDMA_DMABUF_CON_2_VAL_BUF_RESV_SIZE_2(val)         \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_2_FLD_BUF_RESV_SIZE_2, (val))
#define MDP_RDMA_DMABUF_CON_2_VAL_ISSUE_REQ_TH_2(val)          \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_2_FLD_ISSUE_REQ_TH_2, (val))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_VAL_URGENT_TH_HIGH_OFS_2(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_URGENT_TH_HIGH_OFS_2, \
			(val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_VAL_PRE_ULTRA_TH_HIGH_OFS_2(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_PRE_ULTRA_TH_HIGH_OFS_2, \
			(val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_2_VAL_ULTRA_TH_HIGH_OFS_2(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_2_FLD_ULTRA_TH_HIGH_OFS_2, \
			(val))

#define MDP_RDMA_ULTRA_TH_LOW_CON_2_VAL_URGENT_TH_LOW_OFS_2(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_URGENT_TH_LOW_OFS_2, \
			(val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_2_VAL_PRE_ULTRA_TH_LOW_OFS_2(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_PRE_ULTRA_TH_LOW_OFS_2, \
			(val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_2_VAL_ULTRA_TH_LOW_OFS_2(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_2_FLD_ULTRA_TH_LOW_OFS_2, \
			(val))

#define MDP_RDMA_DMABUF_CON_3_VAL_EXTRD_ARB_MAX_3(val)         \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_3_FLD_EXTRD_ARB_MAX_3, (val))
#define MDP_RDMA_DMABUF_CON_3_VAL_BUF_RESV_SIZE_3(val)         \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_3_FLD_BUF_RESV_SIZE_3, (val))
#define MDP_RDMA_DMABUF_CON_3_VAL_ISSUE_REQ_TH_3(val)          \
	REG_FLD_VAL(MDP_RDMA_DMABUF_CON_3_FLD_ISSUE_REQ_TH_3, (val))

#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_VAL_URGENT_TH_HIGH_OFS_3(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_URGENT_TH_HIGH_OFS_3, \
			(val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_VAL_PRE_ULTRA_TH_HIGH_OFS_3(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_PRE_ULTRA_TH_HIGH_OFS_3, \
			(val))
#define MDP_RDMA_ULTRA_TH_HIGH_CON_3_VAL_ULTRA_TH_HIGH_OFS_3(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_HIGH_CON_3_FLD_ULTRA_TH_HIGH_OFS_3, \
			(val))

#define MDP_RDMA_ULTRA_TH_LOW_CON_3_VAL_URGENT_TH_LOW_OFS_3(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_URGENT_TH_LOW_OFS_3, \
			(val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_3_VAL_PRE_ULTRA_TH_LOW_OFS_3(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_PRE_ULTRA_TH_LOW_OFS_3, \
			(val))
#define MDP_RDMA_ULTRA_TH_LOW_CON_3_VAL_ULTRA_TH_LOW_OFS_3(val) \
	REG_FLD_VAL(MDP_RDMA_ULTRA_TH_LOW_CON_3_FLD_ULTRA_TH_LOW_OFS_3, \
			(val))

#define MDP_RDMA_DITHER_CON_VAL_DITHER_INIT_H_POS(val)         \
	REG_FLD_VAL(MDP_RDMA_DITHER_CON_FLD_DITHER_INIT_H_POS, (val))
#define MDP_RDMA_DITHER_CON_VAL_DITHER_INIT_V_POS(val)         \
	REG_FLD_VAL(MDP_RDMA_DITHER_CON_FLD_DITHER_INIT_V_POS, (val))
#define MDP_RDMA_DITHER_CON_VAL_DITHER_EN(val)                 \
	REG_FLD_VAL(MDP_RDMA_DITHER_CON_FLD_DITHER_EN, (val))

#define MDP_RDMA_RESV_DUMMY_0_VAL_RESV_DUMMY_0(val)            \
	REG_FLD_VAL(MDP_RDMA_RESV_DUMMY_0_FLD_RESV_DUMMY_0, (val))

#define MDP_RDMA_UNCOMP_MON_VAL_COMP_MON_RAW(val)              \
	REG_FLD_VAL(MDP_RDMA_UNCOMP_MON_FLD_COMP_MON_RAW, (val))
#define MDP_RDMA_UNCOMP_MON_VAL_COMP_MON_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_UNCOMP_MON_FLD_COMP_MON_CLR, (val))

#define MDP_RDMA_COMP_MON_VAL_COMP_MON_COMP(val)               \
	REG_FLD_VAL(MDP_RDMA_COMP_MON_FLD_COMP_MON_COMP, (val))

#define MDP_RDMA_CHKS_EXTR_VAL_CHKS_EXTR_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_EXTR_FLD_CHKS_EXTR_CRC, (val))
#define MDP_RDMA_CHKS_EXTR_VAL_CHKS_EXTR_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_EXTR_FLD_CHKS_EXTR_CLR, (val))

#define MDP_RDMA_CHKS_INTW_VAL_CHKS_INTW_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_INTW_FLD_CHKS_INTW_CRC, (val))
#define MDP_RDMA_CHKS_INTW_VAL_CHKS_INTW_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_INTW_FLD_CHKS_INTW_CLR, (val))

#define MDP_RDMA_CHKS_INTR_VAL_CHKS_INTR_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_INTR_FLD_CHKS_INTR_CRC, (val))
#define MDP_RDMA_CHKS_INTR_VAL_CHKS_INTR_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_INTR_FLD_CHKS_INTR_CLR, (val))

#define MDP_RDMA_CHKS_ROTO_VAL_CHKS_ROTO_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_ROTO_FLD_CHKS_ROTO_CRC, (val))
#define MDP_RDMA_CHKS_ROTO_VAL_CHKS_ROTO_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_ROTO_FLD_CHKS_ROTO_CLR, (val))

#define MDP_RDMA_CHKS_SRIY_VAL_CHKS_SRIY_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SRIY_FLD_CHKS_SRIY_CRC, (val))
#define MDP_RDMA_CHKS_SRIY_VAL_CHKS_SRIY_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SRIY_FLD_CHKS_SRIY_CLR, (val))

#define MDP_RDMA_CHKS_SRIU_VAL_CHKS_SRIU_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SRIU_FLD_CHKS_SRIU_CRC, (val))
#define MDP_RDMA_CHKS_SRIU_VAL_CHKS_SRIU_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SRIU_FLD_CHKS_SRIU_CLR, (val))

#define MDP_RDMA_CHKS_SRIV_VAL_CHKS_SRIV_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SRIV_FLD_CHKS_SRIV_CRC, (val))
#define MDP_RDMA_CHKS_SRIV_VAL_CHKS_SRIV_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SRIV_FLD_CHKS_SRIV_CLR, (val))

#define MDP_RDMA_CHKS_SROY_VAL_CHKS_SROY_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SROY_FLD_CHKS_SROY_CRC, (val))
#define MDP_RDMA_CHKS_SROY_VAL_CHKS_SROY_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SROY_FLD_CHKS_SROY_CLR, (val))

#define MDP_RDMA_CHKS_SROU_VAL_CHKS_SROU_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SROU_FLD_CHKS_SROU_CRC, (val))
#define MDP_RDMA_CHKS_SROU_VAL_CHKS_SROU_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SROU_FLD_CHKS_SROU_CLR, (val))

#define MDP_RDMA_CHKS_SROV_VAL_CHKS_SROV_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SROV_FLD_CHKS_SROV_CRC, (val))
#define MDP_RDMA_CHKS_SROV_VAL_CHKS_SROV_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_SROV_FLD_CHKS_SROV_CLR, (val))

#define MDP_RDMA_CHKS_VUPI_VAL_CHKS_VUPI_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_VUPI_FLD_CHKS_VUPI_CRC, (val))
#define MDP_RDMA_CHKS_VUPI_VAL_CHKS_VUPI_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_VUPI_FLD_CHKS_VUPI_CLR, (val))

#define MDP_RDMA_CHKS_VUPO_VAL_CHKS_VUPO_CRC(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_VUPO_FLD_CHKS_VUPO_CRC, (val))
#define MDP_RDMA_CHKS_VUPO_VAL_CHKS_VUPO_CLR(val)              \
	REG_FLD_VAL(MDP_RDMA_CHKS_VUPO_FLD_CHKS_VUPO_CLR, (val))

#define MDP_RDMA_DEBUG_CON_VAL_ufbdc_debug_out_sel(val)        \
	REG_FLD_VAL(MDP_RDMA_DEBUG_CON_FLD_ufbdc_debug_out_sel, (val))
#define MDP_RDMA_DEBUG_CON_VAL_debug_out_sel(val)              \
	REG_FLD_VAL(MDP_RDMA_DEBUG_CON_FLD_debug_out_sel, (val))
#define MDP_RDMA_DEBUG_CON_VAL_CHKS_TRB_SEL(val)               \
	REG_FLD_VAL(MDP_RDMA_DEBUG_CON_FLD_CHKS_TRB_SEL, (val))
#define MDP_RDMA_DEBUG_CON_VAL_CHKS_SEL(val)                   \
	REG_FLD_VAL(MDP_RDMA_DEBUG_CON_FLD_CHKS_SEL, (val))
#define MDP_RDMA_DEBUG_CON_VAL_CHKS_CRC_EN(val)                \
	REG_FLD_VAL(MDP_RDMA_DEBUG_CON_FLD_CHKS_CRC_EN, (val))

#define MDP_RDMA_MON_STA_0_VAL_MON_STA_0(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_0_FLD_MON_STA_0, (val))

#define MDP_RDMA_MON_STA_1_VAL_MON_STA_1(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_1_FLD_MON_STA_1, (val))

#define MDP_RDMA_MON_STA_2_VAL_MON_STA_2(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_2_FLD_MON_STA_2, (val))

#define MDP_RDMA_MON_STA_3_VAL_MON_STA_3(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_3_FLD_MON_STA_3, (val))

#define MDP_RDMA_MON_STA_4_VAL_MON_STA_4(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_4_FLD_MON_STA_4, (val))

#define MDP_RDMA_MON_STA_5_VAL_MON_STA_5(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_5_FLD_MON_STA_5, (val))

#define MDP_RDMA_MON_STA_6_VAL_MON_STA_6(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_6_FLD_MON_STA_6, (val))

#define MDP_RDMA_MON_STA_7_VAL_MON_STA_7(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_7_FLD_MON_STA_7, (val))

#define MDP_RDMA_MON_STA_8_VAL_MON_STA_8(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_8_FLD_MON_STA_8, (val))

#define MDP_RDMA_MON_STA_9_VAL_MON_STA_9(val)                  \
	REG_FLD_VAL(MDP_RDMA_MON_STA_9_FLD_MON_STA_9, (val))

#define MDP_RDMA_MON_STA_10_VAL_MON_STA_10(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_10_FLD_MON_STA_10, (val))

#define MDP_RDMA_MON_STA_11_VAL_MON_STA_11(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_11_FLD_MON_STA_11, (val))

#define MDP_RDMA_MON_STA_12_VAL_MON_STA_12(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_12_FLD_MON_STA_12, (val))

#define MDP_RDMA_MON_STA_13_VAL_MON_STA_13(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_13_FLD_MON_STA_13, (val))

#define MDP_RDMA_MON_STA_14_VAL_MON_STA_14(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_14_FLD_MON_STA_14, (val))

#define MDP_RDMA_MON_STA_15_VAL_MON_STA_15(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_15_FLD_MON_STA_15, (val))

#define MDP_RDMA_MON_STA_16_VAL_MON_STA_16(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_16_FLD_MON_STA_16, (val))

#define MDP_RDMA_MON_STA_17_VAL_MON_STA_17(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_17_FLD_MON_STA_17, (val))

#define MDP_RDMA_MON_STA_18_VAL_MON_STA_18(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_18_FLD_MON_STA_18, (val))

#define MDP_RDMA_MON_STA_19_VAL_MON_STA_19(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_19_FLD_MON_STA_19, (val))

#define MDP_RDMA_MON_STA_20_VAL_MON_STA_20(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_20_FLD_MON_STA_20, (val))

#define MDP_RDMA_MON_STA_21_VAL_MON_STA_21(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_21_FLD_MON_STA_21, (val))

#define MDP_RDMA_MON_STA_22_VAL_MON_STA_22(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_22_FLD_MON_STA_22, (val))

#define MDP_RDMA_MON_STA_23_VAL_MON_STA_23(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_23_FLD_MON_STA_23, (val))

#define MDP_RDMA_MON_STA_24_VAL_MON_STA_24(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_24_FLD_MON_STA_24, (val))

#define MDP_RDMA_MON_STA_25_VAL_MON_STA_25(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_25_FLD_MON_STA_25, (val))

#define MDP_RDMA_MON_STA_26_VAL_MON_STA_26(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_26_FLD_MON_STA_26, (val))

#define MDP_RDMA_MON_STA_27_VAL_MON_STA_27(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_27_FLD_MON_STA_27, (val))

#define MDP_RDMA_MON_STA_28_VAL_MON_STA_28(val)                \
	REG_FLD_VAL(MDP_RDMA_MON_STA_28_FLD_MON_STA_28, (val))

#define MDP_RDMA_SRC_BASE_0_VAL_SRC_BASE_0(val)                \
	REG_FLD_VAL(MDP_RDMA_SRC_BASE_0_FLD_SRC_BASE_0, (val))

#define MDP_RDMA_SRC_BASE_1_VAL_SRC_BASE_1(val)                \
	REG_FLD_VAL(MDP_RDMA_SRC_BASE_1_FLD_SRC_BASE_1, (val))

#define MDP_RDMA_SRC_BASE_2_VAL_SRC_BASE_2(val)                \
	REG_FLD_VAL(MDP_RDMA_SRC_BASE_2_FLD_SRC_BASE_2, (val))

#define MDP_RDMA_UFO_DEC_LENGTH_BASE_Y_VAL_UFO_DEC_Y_LEN_BASE(val) \
	REG_FLD_VAL(MDP_RDMA_UFO_DEC_LENGTH_BASE_Y_FLD_UFO_DEC_Y_LEN_BASE,\
			 (val))

#define MDP_RDMA_UFO_DEC_LENGTH_BASE_C_VAL_UFO_DEC_C_LEN_BASE(val) \
	REG_FLD_VAL(MDP_RDMA_UFO_DEC_LENGTH_BASE_C_FLD_UFO_DEC_C_LEN_BASE, \
			(val))
#endif

#endif // __VDO1_MDP_RDMA0_REGS_H__

