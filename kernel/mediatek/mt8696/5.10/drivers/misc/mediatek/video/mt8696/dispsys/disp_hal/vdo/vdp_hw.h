/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef _VDP_HW_H
#define _VDP_HW_H

#include <linux/types.h>
#include "disp_hw_mgr.h"

#define HAL_VDO1_OFST 0x400
#define HAL_VDO2_OFST 0x400
#define HAL_VDO3_UFO_OFST 0x200
#define HAL_VDO3_OFST 0x400
#define HAL_VDO4_UFO_OFST 0x200
#define HAL_VDO4_OFST 0x400
#define HAL_VDO3_SCL_OFST 0x100
#define HAL_VDO4_SCL_OFST 0x100

#define HAL_VDO_REG_NUM (0x100 / 4)

#define HAL_VDO_UFOD_REG_NUM (0x100 / 4)

#define HAL_VDO_ADV_REG_NUM (0x100 / 4)

#define HAL_VDO_SCL_REG_NUM (0xA4 / 4)

#define HAL_VDO_APQ_REG_NUM (0x100 / 4)

#define HAL_VDO_FUSION_REG_NUM (0x100 / 4)

#define HAL_VDO_CS_REG_NUM (0x100 / 4)

#define HAL_MVDO_RESET_ALL 0xFF
#define HAL_MVDO_RESET_CLEAR 0x0

#define VDO_LAYER_COUNT 2 /*hw video layer base count */

#if 1
/* 0x1100, 0x8100 */
struct vdo_hal_scl_field {
	/* DWORD - 000 */
	uint32_t YCoef00 : 32;

	/* DWORD - 004 */
	uint32_t YCoef01 : 32;

	/* DWORD - 008 */
	uint32_t YCoef02 : 32;

	/* DWORD - 00C */
	uint32_t YCoef03 : 32;

	/* DWORD - 010 */
	uint32_t YCoef04 : 32;

	/* DWORD - 014 */
	uint32_t YCoef05 : 32;

	/* DWORD - 018 */
	uint32_t YCoef06 : 32;

	/* DWORD - 01C */
	uint32_t YCoef07 : 32;

	/* DWORD - 020 */
	uint32_t YCoef08 : 32;

	/* DWORD - 024 */
	uint32_t YCoef09 : 32;

	/* DWORD - 028 */
	uint32_t YCoef0A : 32;

	/* DWORD - 02C */
	uint32_t YCoef0B : 32;

	/* DWORD - 030 */
uint32_t: 32;

	/* DWORD - 034 */
uint32_t: 32;

	/* DWORD - 038 */
uint32_t: 32;

	/* DWORD - 03C */
uint32_t: 32;

	/* DWORD - 040 */
	uint32_t YCoef10 : 32;

	/* DWORD - 044 */
uint32_t: 32;

	/* DWORD - 048 */
uint32_t: 32;

	/* DWORD - 04C */
uint32_t: 28;
	uint32_t MSB_Padding : 1;
uint32_t: 3;

	/* DWORD - 050 */
	uint32_t CCoef00 : 32;

	/* DWORD - 054 */
	uint32_t CCoef01 : 32;

	/* DWORD - 058 */
	uint32_t CCoef02 : 32;

	/* DWORD - 05C */
	uint32_t CCoef03 : 32;

	/* DWORD - 060 */
	uint32_t CCoef04 : 32;

	/* DWORD - 064 */
	uint32_t CCoef05 : 32;

	/* DWORD - 068 */
	uint32_t CCoef06 : 32;

	/* DWORD - 06C */
	uint32_t CCoef07 : 32;

	/* DWORD - 070 */
	uint32_t CCoef08 : 32;

	/* DWORD - 074 */
	uint32_t CCoef09 : 32;

	/* DWORD - 078 */
	uint32_t CCoef0A : 32;

	/* DWORD - 07C */
	uint32_t CCoef0B : 32;

	/* DWORD - 080 */
	uint32_t CCoef0C : 32;

	/* DWORD - 084 */
	uint32_t CCoef0D : 32;

	/* DWORD - 088 */
	uint32_t CCoef0E : 32;

	/* DWORD - 08C */
	uint32_t CCoef0F : 32;

	/* DWORD - 090 */
uint32_t: 32;

	/* DWORD - 094 */
uint32_t: 32;

	/* DWORD - 098 */
	uint32_t crc_result_8tap : 24;
uint32_t: 8;

	/* DWORD - 09C */
	uint32_t V3TAP_THRESHOLD : 6;
uint32_t: 2;
	uint32_t V3TAP_EN : 1;
	uint32_t V3TAP_DEMO_EN : 1;
uint32_t: 22;

	/* DWORD - 0A0 */ /* 0x11A0, 0x81A0 */
	uint32_t LINE_BUF_LEFT_RIGHT_START : 7;
uint32_t: 8;
	uint32_t SKIP_AFTER_MIRROR : 1;
uint32_t: 15;
	uint32_t BUFFER_SP_USE_REG_OFFSET : 1;

	/* DWORD - 0A4 */
	uint32_t OW_RIGHT_START : 9;
uint32_t: 23;

};

union vdo_hal_scl_union {
	uint32_t reg[HAL_VDO_SCL_REG_NUM];
	struct vdo_hal_scl_field field;
};

/* 0x1400, 0x8400 */
struct vdo_hal_field {
	/* DWORD - 000 */
uint32_t: 32;

	/* DWORD - 004 */
uint32_t: 32;

	/* DWORD - 008 */
uint32_t: 32;

	/* DWORD - 00C */
uint32_t: 32;

	/* DWORD - 010  0x1410, 0x8410 */
uint32_t: 8;
	uint32_t DW_NEED : 8;     /* pixel width / 4 */
	uint32_t PIC_HEIGHT : 12; /*for MT8580 4kx2k */
uint32_t: 2;
	uint32_t DW_NEED_BIT10 : 1;
uint32_t: 1;

	/* DWORD - 014 */
	uint32_t VSCALE : 16;
	uint32_t P_SKIP : 12; /*for MT8580 4kx2k */
uint32_t: 2;
	uint32_t SHADOW_MODE : 1;
	uint32_t ENABLE_TO_LAYER2 : 1;

	/* DWORD - 018 */
uint32_t: 32;

	/* DWORD - 01C  0x141c */
	uint32_t VDOEN : 1;
uint32_t: 1;
	uint32_t YLAVG : 1;
	uint32_t CLAVG : 1;
uint32_t: 1;
uint32_t: 1;
	uint32_t YLR : 1;
	uint32_t CLR : 1;
uint32_t: 16;
	uint32_t Tst0_7 : 8;    /*[31:24] */

	/* DWORD - 020 */
	uint32_t YSLTT : 12;
uint32_t: 20;

	/* DWORD - 024 */
	uint32_t CSLTT : 12;
uint32_t: 20;

	/* DWORD - 028 */
	uint32_t YSSLTT : 8;
uint32_t: 24;

	/* DWORD - 02C */
	uint32_t CSSLTT : 8;
uint32_t: 24;

	/* DWORD - 030  0x1430 */
uint32_t: 9;
	uint32_t CR80 : 1;   /*[9] */
uint32_t: 8;	/* 17:10 */
	uint32_t c_extend_1_line : 1;	/*[18] */
	uint32_t y_extend_1_line : 1;	/*[19] */
uint32_t: 1;   /*[20] */
	uint32_t YUV422 : 1; /*[21] */
uint32_t: 10;

	/* DWORD - 034  0x42434 */
uint32_t: 16;		  /*[15:0] */
	uint32_t DITHER_MODE : 3; /* [18:16]*/
	uint32_t DITHER_EN : 1;   /*[19]*/
uint32_t: 12;		  /*31:20*/

	/* DWORD - 038 */
uint32_t: 32;

	/* DWORD - 03C */
	uint32_t SW_RST : 8;
uint32_t: 24;

	/* DWORD - 040 */
uint32_t: 32;

	/* DWORD - 044 */
uint32_t: 32;

	/* DWORD - 048 */
uint32_t: 32;

	/* DWORD - 04C */
uint32_t: 32;

	/* DWORD - 050 */
uint32_t: 32;

	/* DWORD - 054 */
uint32_t: 32;

	/* DWORD - 058 */
uint32_t: 32;

	/* DWORD - 05C */
uint32_t: 32;

	/* DWORD - 060 */
uint32_t: 15;
	uint32_t SP_LD : 1;
uint32_t: 4;
	uint32_t Y_SRC_LINE_NUM : 12;

	/* DWORD - 064 */
uint32_t: 32;

	/* DWORD - 068 */
uint32_t: 32;

	/* DWORD - 06C */
uint32_t: 32;

	/* DWORD - 070 */
	uint32_t FLT_TW : 1;
	uint32_t CF_TW : 1;
uint32_t: 6;
	uint32_t VDO_TST : 2;
uint32_t: 22;

	/* DWORD - 074 */
uint32_t: 20;
	uint32_t C_SRC_LINE_NUM : 12;

	/* DWORD - 078   0x1478 */
	uint32_t YFIR_ON : 1;       /* 0 */
	uint32_t YFIR_LNR : 1;      /* 1 */
	uint32_t GAU62_15_0575 : 1; /* 2 */
	uint32_t GAU62_15_0675 : 1; /* 3 */
	uint32_t YFIR_CF_PRG : 1;   /* 4 */
	uint32_t PH16 : 1;	  /* 5 */
	uint32_t EVN_FIR : 1;       /* 6 */
	uint32_t MIRROR_EN : 1;     /* 7 */
uint32_t: 24;

	/* DWORD - 07C  0x147c */
uint32_t: 8;
	uint32_t Video_Opt8 : 1;
uint32_t: 23;

	/* DWORD - 080 */
uint32_t: 32;

	/* DWORD - 084 */
uint32_t: 32;

	/* DWORD - 088 */
uint32_t: 28;
	uint32_t BP_YC : 1;
uint32_t: 3;

	/* DWORD - 08C */
uint32_t: 24;
	uint32_t MA_Video_Mode : 8;

	/* DWORD - 090 */
uint32_t: 32;

	/* DWORD - 094 */
uint32_t: 32;

	/* DWORD - 098 */
uint32_t: 32;

	/* DWORD - 09C */
uint32_t: 32;

	/* DWORD - 0A0 */
uint32_t: 32;

	/* DWORD - 0A4 */
uint32_t: 32;

	/* DWORD - 0A8 */
uint32_t: 32;

	/* DWORD - 0AC */
uint32_t: 32;

	/* DWORD - 0B0 */
uint32_t: 32;

	/* DWORD - 0B4 */
uint32_t: 32;

	/* DWORD - 0B8  */
uint32_t: 32;

	/* DWORD - 0BC */
uint32_t: 32;

	/* DWORD - 0C0   0x424c0 motion detection advance */
uint32_t: 32;

	/* DWORD - 0C4   0x424c4 pull down field like */
uint32_t: 32;

	/* DWORD - 0C8  0x424c8 pull down band pass filter */
	uint32_t LUMA_KEY_TH : 8; /* [7:0] luma key threshold */
uint32_t: 17;
	uint32_t LM_KEY : 1; /* [25] luma key enable */
uint32_t: 6;

	/* DWORD - 0CC */
uint32_t: 32;

	/* DWORD - 0D0  */
uint32_t: 32;

	/* DWORD - 0D4 */
uint32_t: 32;

	/* DWORD - 0D8 */
uint32_t: 32;

	/* DWORD - 0DC */
uint32_t: 32;

	/* DWORD - 0E0  */
	uint32_t DW_NEED_HD : 9;   /*[8:0] */
uint32_t: 15;
	uint32_t HD_EN : 1; /*24 */
uint32_t: 2;
	uint32_t DW_NEED_BIT9 : 1; /*for MT8580 4kx2k */
uint32_t: 3;
	uint32_t F_PRGS : 1;
};

union vdo_hal_union {
	uint32_t reg[HAL_VDO_REG_NUM];
	struct vdo_hal_field field;
};

/* 0x1200, 0x8200 */
struct vdo_hal_ufo_field {
	/*DWORD - 000 */
	uint32_t PTR_TO_Y : 32;

	/*DWORD - 004 */
	uint32_t PTR_TO_C : 32;

	/*DWORD - 008 */
	uint32_t H_WIDTH : 12;
uint32_t: 8;
	uint32_t V_HEIGTH : 12;

	/*DWORD - 00C */
	uint32_t LINE_PITCH : 10;
uint32_t: 22;

	/*DWORD - 010 */
	uint32_t PTR_TO_Y_LENGTH : 32;

	/*DWORD - 014 */
	uint32_t PTR_TO_C_LENGTH : 32;

	/*DWORD - 018 */
	uint32_t UFO_PATTERN : 1;
uint32_t: 22;
	uint32_t TILE_MODE : 1;
	uint32_t DATA_PACK_MODE : 3;
	uint32_t UHD4K_MODE_ENABLE : 1;
	uint32_t flip_en : 1;
	uint32_t YUV422_MODE : 1;
	uint32_t ADD_MODE : 2;

	/*DWORD - 01C */
	uint32_t HD_REG_TH_H : 4;
	uint32_t FLIP_SKIP_LINE : 5;
uint32_t: 18;
	uint32_t JMP_BURST4 : 1;
uint32_t: 3;
	uint32_t UFO_DRAM_NEW_MODE : 1;

	/*DWORD - 020 */
	uint32_t PRE_ULTRA_TH : 7;
	uint32_t PRE_ULTRA_EN : 1;
	uint32_t ULTRA_TH : 7;
	uint32_t ULTRA_EN : 1;
	uint32_t C_PRE_ULTRA_TH : 7;
	uint32_t C_PRE_ULTRA_EN : 1;
	uint32_t C_ULTRA_TH : 7;
	uint32_t C_ULTRA_EN : 1;

	/*DWORD - 024 */
uint32_t: 32;

	/*DWORD - 028 */
uint32_t: 32;

	/*DWORD - 02C */
uint32_t: 32;

	/*DWORD - 030 */
uint32_t: 32;

	/*DWORD - 034 */
uint32_t: 32;

	/*DWORD - 038 */
uint32_t: 32;

	/*DWORD - 03C */
uint32_t: 32;

	/*DWORD - 040 */
	uint32_t UFOD_ENABLE : 1;
	uint32_t SIMULATION_MODE : 1;
	uint32_t SHADOW_ENABLE : 1;
uint32_t: 29;

	/*DWORD - 044 */
	uint32_t DRAM_CHECK_SUM : 32;

	/*DWORD - 048 */
	uint32_t FIFO_FULL_ERROR : 1;
uint32_t: 31;

	/*DWORD - 04C */
uint32_t: 32;

	/*DWORD - 050 */
uint32_t: 32;

	/*DWORD - 054 */
uint32_t: 32;

	/*DWORD - 058 */
uint32_t: 32;

	/*DWORD - 05C */
uint32_t: 32;

	/*DWORD - 060 */
uint32_t: 32;

	/*DWORD - 064 */
uint32_t: 32;

	/*DWORD - 068 */
uint32_t: 32;

	/*DWORD - 06C */
uint32_t: 32;

	/*DWORD - 070 */
uint32_t: 32;

	/*DWORD - 074 */
uint32_t: 32;

	/*DWORD - 078 */
uint32_t: 32;

	/*DWORD - 07C */
uint32_t: 32;

	/*DWORD - 080 */
uint32_t: 32;

	/*DWORD - 084 */
uint32_t: 32;

	/*DWORD - 088 */
uint32_t: 32;

	/*DWORD - 08C */
uint32_t: 32;

	/*DWORD - 090 */
uint32_t: 32;

	/*DWORD - 094 */
uint32_t: 32;

	/*DWORD - 098 */
uint32_t: 32;

	/*DWORD - 09C */
uint32_t: 32;

	/*DWORD - 0A0 */
uint32_t: 32;

	/*DWORD - 0A4 */
uint32_t: 32;

	/*DWORD - 0A8 */
uint32_t: 32;

	/*DWORD - 0AC */
uint32_t: 32;

	/*DWORD - 0B0 */
uint32_t: 32;

	/*DWORD - 0B4 */
uint32_t: 32;

	/*DWORD - 0B8 */
uint32_t: 32;

	/*DWORD - 0BC */
uint32_t: 32;

	/*DWORD - 0C0 */
uint32_t: 32;

	/*DWORD - 0C4 */
uint32_t: 32;

	/*DWORD - 0C8 */
uint32_t: 32;

	/*DWORD - 0CC */
uint32_t: 32;

	/*DWORD - 0D0 */
uint32_t: 32;

	/*DWORD - 0D4 */
uint32_t: 32;

	/*DWORD - 0D8 */
uint32_t: 32;

	/*DWORD - 0DC */
uint32_t: 32;

	/*DWORD - 0E0 */
uint32_t: 32;

	/*DWORD - 0E4 */
uint32_t: 32;

	/*DWORD - 0E8 */
uint32_t: 32;

	/*DWORD - 0EC */
uint32_t: 32;

	/*DWORD - 0F0 */
uint32_t: 32;

	/*DWORD - 0F4 */
uint32_t: 32;

	/*DWORD - 0F8 */
uint32_t: 32;

	/*DWORD - 0FC */
uint32_t: 32;
};

union vdo_hal_ufo_union {
	uint32_t reg[HAL_VDO_UFOD_REG_NUM];
	struct vdo_hal_ufo_field field;
};

struct vdo_sw_shadow {
	union vdo_hal_union vdo_reg;
	union vdo_hal_scl_union vdo_scl_reg;
	union vdo_hal_ufo_union vdo_ufo_reg;
	uint64_t vdo_reg_mode;
	uint64_t vdo_scl_reg_mode;
	uint64_t vdo_ufo_reg_mode;
};

struct vdo_hw_register {
	union vdo_hal_union *vdo_reg;
	union vdo_hal_ufo_union *vdo_ufo_reg;
	union vdo_hal_scl_union *vdo_scl_reg;
};
#endif

#ifdef DISP_GCE_SUPPORT
struct vdo_phy_register {
	phys_addr_t vdo_base;
	phys_addr_t ufo_base;
	phys_addr_t scl_base;
};
#endif
/*************************************************************************/
/* VDP HAL API */
/************************************************************************/
/*#define VDP_1					0	*/
/*#define VDP_2					1	*/

#if 0
extern union vdo_hal_union vdp1_sw_reg;
extern union vdo_hal_union vdp2_sw_reg;

extern union vdo_hal_scl_union vdp1_scl_sw_reg;
extern union vdo_hal_scl_union vdp2_scl_sw_reg;

extern union vdo_hal_ufo_union vdo3_ufo_sw_reg;
extern union vdo_hal_ufo_union vdo4_ufo_sw_reg;
#endif

extern uint64_t vdp3_reg_mode;
extern uint64_t vdp4_reg_mode;

extern uint64_t vdp3_scl_reg_mode;
extern uint64_t vdp4_scl_reg_mode;

extern uint64_t vdo3_ufo_reg_mode;
extern uint64_t vdo4_ufo_reg_mode;
extern char *disp_vdo_reg_base[VDO_LAYER_COUNT];

#define REG_MASK(idx) ((uint64_t)1LL << (idx))

#define IS_REG_SET(r, mask) ((r) & (mask))
#define REG_SET(r, mask) ((r) |= (mask))
#define REG_RESET(r, mask) ((r) &= ~(mask))

#define GET_VDP_PTR(id, reg, mode)                                             \
	do {                                                                   \
		if (id == VDP_1) {                                             \
			reg = &vdo_sw_reg[0].vdo_reg;                          \
			mode = &vdo_sw_reg[0].vdo_reg_mode;                    \
		} else if (id == VDP_2) {                                      \
			reg = &vdo_sw_reg[1].vdo_reg;                          \
			mode = &vdo_sw_reg[1].vdo_reg_mode;                    \
		}                                                              \
	} while (0)

#define GET_UFO_PTR(id, reg, mode)                                             \
	do {                                                                   \
		if (id == VDP_1) {                                             \
			reg = &vdo_sw_reg[0].vdo_ufo_reg;                      \
			mode = &vdo_sw_reg[0].vdo_ufo_reg_mode;                \
		} else if (id == VDP_2) {                                      \
			reg = &vdo_sw_reg[1].vdo_ufo_reg;                      \
			mode = &vdo_sw_reg[1].vdo_ufo_reg_mode;                \
		}                                                              \
	} while (0)

#define GET_VDP_SCL_PTR(id, reg, mode)                                         \
	do {                                                                   \
		if (id == VDP_1) {                                             \
			reg = &vdo_sw_reg[0].vdo_scl_reg;                      \
			mode = &vdo_sw_reg[0].vdo_scl_reg_mode;                \
		} else if (id == VDP_2) {                                      \
			reg = &vdo_sw_reg[1].vdo_scl_reg;                      \
			mode = &vdo_sw_reg[1].vdo_scl_reg_mode;                \
		}                                                              \
	} while (0)

#endif
