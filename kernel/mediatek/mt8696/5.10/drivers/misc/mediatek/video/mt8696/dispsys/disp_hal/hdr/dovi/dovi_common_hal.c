/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include <linux/types.h>

#include "disp_dovi_common_if.h"
#include "dovi_vdo_fe_hal.h"
#include "dovi_gfx_fe_hal.h"
#include "dovi_be_hal.h"
#include "dovi_common_hal.h"

struct dv_all_reg_tab dv_dsys_all_reg;
struct dv_all_reg_tab dv_msys_all_reg;
bool b_comp_enable;
uint32_t _subv_type; // 0: only sdr, 1: repeat mvdo
uint32_t cur_ml_cfg_st[4];

uint32_t sub_sdr_ipt_reg[722] = {
	0x15015204, 0x00000000,
	0x15015220, 0x00000000,
	0x15015224, 0x000003FF,
	0x15015228, 0x000003FF,
	0x1501522C, 0x000003FF,
	0x15015230, 0x000003FF,
	0x15015234, 0x000003FF,
	0x15015238, 0x000003FF,
	0x1501523C, 0x000003FF,
	0x15015240, 0x000003FF,
	0x15015244, 0x00000000,
	0x15015248, 0x00000000,
	0x1501524C, 0x00000000,
	0x15015250, 0x00001806,
	0x15015254, 0x00000080,
	0x15015258, 0x00000000,
	0x1501525C, 0x00000000,
	0x15015260, 0x00000000,
	0x15015264, 0x00000000,
	0x15015268, 0x00001806,
	0x1501526C, 0x00000080,
	0x15015270, 0x00000000,
	0x15015274, 0x00000000,
	0x15015278, 0x00000000,
	0x1501527C, 0x00000000,
	0x15015280, 0x00001806,
	0x15015284, 0x00000080,
	0x15015288, 0x00000000,
	0x1501528C, 0x00000000,
	0x15015290, 0x00000000,
	0x15015294, 0x00000000,
	0x15015298, 0x00001806,
	0x1501529C, 0x00000080,
	0x150152A0, 0x00000000,
	0x150152A4, 0x00000000,
	0x150152A8, 0x00000000,
	0x150152AC, 0x00000000,
	0x150152B0, 0x00001806,
	0x150152B4, 0x00000080,
	0x150152B8, 0x00000000,
	0x150152BC, 0x00000000,
	0x150152C0, 0x00000000,
	0x150152C4, 0x00000000,
	0x150152C8, 0x00001806,
	0x150152CC, 0x00000080,
	0x150152D0, 0x00000000,
	0x150152D4, 0x00000000,
	0x150152D8, 0x00000000,
	0x150152DC, 0x00000000,
	0x150152E0, 0x00001806,
	0x150152E4, 0x00000080,
	0x150152E8, 0x00000000,
	0x150152EC, 0x00000000,
	0x150152F0, 0x00000000,
	0x150152F4, 0x00000000,
	0x150152F8, 0x00001806,
	0x150152FC, 0x00000080,
	0x15015300, 0x00000000,
	0x15015304, 0x00000000,
	0x15015308, 0x00000000,
	0x1501530C, 0x000003FF,
	0x15015310, 0x000003FF,
	0x15015314, 0x000003FF,
	0x15015318, 0x000003FF,
	0x1501531C, 0x00000000,
	0x15015320, 0x00000000,
	0x15015324, 0x00000000,
	0x15015328, 0x00001806,
	0x1501532C, 0x00000080,
	0x15015330, 0x00000000,
	0x15015334, 0x00000000,
	0x15015338, 0x00000000,
	0x1501533C, 0x00000000,
	0x15015340, 0x00001806,
	0x15015344, 0x00000080,
	0x15015348, 0x00000000,
	0x1501534C, 0x00000000,
	0x15015350, 0x00000000,
	0x15015354, 0x00000000,
	0x15015358, 0x00001806,
	0x1501535C, 0x00000080,
	0x15015360, 0x00000000,
	0x15015364, 0x00000000,
	0x15015368, 0x00000000,
	0x1501536C, 0x00000000,
	0x15015370, 0x00001806,
	0x15015374, 0x00000080,
	0x15015378, 0x00000000,
	0x1501537C, 0x00000000,
	0x15015380, 0x00000000,
	0x15015384, 0x000003FF,
	0x15015388, 0x000003FF,
	0x1501538C, 0x000003FF,
	0x15015390, 0x000003FF,
	0x15015394, 0x00000000,
	0x15015398, 0x00000000,
	0x1501539C, 0x00000000,
	0x150153A0, 0x00001806,
	0x150153A4, 0x00000080,
	0x150153A8, 0x00000000,
	0x150153AC, 0x00000000,
	0x150153B0, 0x00000000,
	0x150153B4, 0x00000000,
	0x150153B8, 0x00001806,
	0x150153BC, 0x00000080,
	0x150153C0, 0x00000000,
	0x150153C4, 0x00000000,
	0x150153C8, 0x00000000,
	0x150153CC, 0x00000000,
	0x150153D0, 0x00001806,
	0x150153D4, 0x00000080,
	0x150153D8, 0x00000000,
	0x150153DC, 0x00000000,
	0x150153E0, 0x00000000,
	0x150153E4, 0x00000000,
	0x150153E8, 0x00001806,
	0x150153EC, 0x00000080,
	0x150153F0, 0x00000000,
	0x150153F4, 0x00000000,
	0x150153F8, 0x00000000,
	0x150153FC, 0x00000000,
	0x15015420, 0x00000000,
	0x15015424, 0x00000000,
	0x15015428, 0x00000000,
	0x1501542C, 0x00000000,
	0x15015430, 0x00000000,
	0x15015434, 0x00000000,
	0x15015438, 0x00000000,
	0x1501543C, 0x00000000,
	0x15015440, 0x00000000,
	0x15015444, 0x00000000,
	0x15015448, 0x00000000,
	0x1501544C, 0x00000000,
	0x15015450, 0x00000000,
	0x15015454, 0x00000000,
	0x15015458, 0x00000000,
	0x1501545C, 0x00000000,
	0x15015460, 0x00000000,
	0x15015464, 0x00000000,
	0x15015468, 0x00000000,
	0x1501546C, 0x00000000,
	0x15015470, 0x00000000,
	0x15015474, 0x00000000,
	0x15015478, 0x00000000,
	0x1501547C, 0x00000000,
	0x15015480, 0x00000000,
	0x15015484, 0x00000000,
	0x15015488, 0x00000000,
	0x1501548C, 0x00000000,
	0x15015490, 0x00000000,
	0x15015494, 0x00000000,
	0x15015498, 0x00000000,
	0x1501549C, 0x00000000,
	0x150154A0, 0x00000000,
	0x150154A4, 0x00000000,
	0x150154A8, 0x00000000,
	0x150154AC, 0x00000000,
	0x150154B0, 0x00000000,
	0x150154B4, 0x00000000,
	0x150154B8, 0x00000000,
	0x150154BC, 0x00000000,
	0x150154C0, 0x00000000,
	0x150154C4, 0x00000000,
	0x150154C8, 0x00000000,
	0x150154CC, 0x00000000,
	0x150154D0, 0x00000000,
	0x150154D4, 0x00000000,
	0x150154D8, 0x00000000,
	0x150154DC, 0x00000000,
	0x150154E0, 0x00000000,
	0x150154E4, 0x00000000,
	0x150154E8, 0x00000000,
	0x150154EC, 0x00000000,
	0x150154F0, 0x00000000,
	0x150154F4, 0x00000000,
	0x150154F8, 0x00000000,
	0x150154FC, 0x00000000,
	0x15015500, 0x00000000,
	0x15015504, 0x00000000,
	0x15015508, 0x00000000,
	0x1501550C, 0x00000000,
	0x15015510, 0x00000000,
	0x15015514, 0x00000000,
	0x15015518, 0x00000000,
	0x1501551C, 0x00000000,
	0x15015520, 0x00000000,
	0x15015524, 0x00000000,
	0x15015528, 0x00000000,
	0x1501552C, 0x00000000,
	0x15015530, 0x00000000,
	0x15015534, 0x00000000,
	0x15015538, 0x00000000,
	0x1501553C, 0x00000000,
	0x15015540, 0x00000000,
	0x15015544, 0x00000000,
	0x15015548, 0x00000000,
	0x1501554C, 0x00000000,
	0x15015550, 0x00000000,
	0x15015554, 0x00000000,
	0x15015558, 0x00000000,
	0x1501555C, 0x00000000,
	0x15015560, 0x00000000,
	0x15015564, 0x00000000,
	0x15015568, 0x00000000,
	0x1501556C, 0x00000000,
	0x15015570, 0x00000000,
	0x15015574, 0x00000000,
	0x15015578, 0x00000000,
	0x1501557C, 0x00000000,
	0x15015580, 0x00000000,
	0x15015584, 0x00000000,
	0x15015588, 0x00000000,
	0x1501558C, 0x00000000,
	0x15015590, 0x00000000,
	0x15015594, 0x00000000,
	0x15015598, 0x00000000,
	0x1501559C, 0x00000000,
	0x150155A0, 0x00000000,
	0x150155A4, 0x00000000,
	0x150155A8, 0x00000000,
	0x150155AC, 0x00000000,
	0x150155B0, 0x00000000,
	0x150155B4, 0x00000000,
	0x150155B8, 0x00000000,
	0x150155BC, 0x00000000,
	0x150155C0, 0x00000000,
	0x150155C4, 0x00000000,
	0x150155C8, 0x00000000,
	0x150155CC, 0x00000000,
	0x150155D0, 0x00000000,
	0x150155D4, 0x00000000,
	0x150155D8, 0x00000000,
	0x150155DC, 0x00000000,
	0x150155E0, 0x00000000,
	0x150155E4, 0x00000000,
	0x150155E8, 0x00000000,
	0x150155EC, 0x00000000,
	0x150155F0, 0x00000000,
	0x150155F4, 0x00000000,
	0x15015618, 0x00000000,
	0x1501561C, 0x00000002,
	0x15015634, 0x00008000,
	0x150156D0, 0x00008001,
	0x15015710, 0x00005000,
	0x15015714, 0x00000000,
	0x15015718, 0x00000000,
	0x1501571C, 0x00000000,
	0x15015804, 0x00000000,
	0x15015808, 0x00000005,
	0x1501580C, 0x000001E0,
	0x15015810, 0x00000000,
	0x15015814, 0x00000000,
	0x15015818, 0x00000000,
	0x1501581C, 0x00000020,
	0x15015820, 0x00002565,
	0x15015824, 0x00000000,
	0x15015828, 0x00003993,
	0x1501582C, 0x00002565,
	0x15015830, 0x0000F926,
	0x15015834, 0x0000EEE1,
	0x15015838, 0x00002565,
	0x1501583C, 0x000043D8,
	0x15015840, 0x00000000,
	0x15015844, 0x0000F400,
	0x15015848, 0x000007C7,
	0x1501584C, 0x00007400,
	0x15015850, 0x0000FD96,
	0x15015854, 0x00009400,
	0x15015858, 0x00000910,
	0x1501585C, 0x00000000,
	0x15015860, 0x0000FFF0,
	0x15015864, 0x00000010,
	0x15015868, 0x00000001,
	0x1501586C, 0x0000FFFC,
	0x15015870, 0x000007FF,
	0x15015874, 0x00000000,
	0x15015878, 0x00000000,
	0x1501587C, 0x00000005,
	0x15015880, 0x00002705,
	0x15015884, 0x000050CE,
	0x15015888, 0x0000082B,
	0x1501588C, 0x000013EF,
	0x15015890, 0x00005F0D,
	0x15015894, 0x00000D01,
	0x15015898, 0x00000361,
	0x1501589C, 0x00001106,
	0x150158A0, 0x00006B97,
	0x150158A4, 0x00000666,
	0x150158A8, 0x00000666,
	0x150158AC, 0x00000333,
	0x150158B0, 0x00004748,
	0x150158B4, 0x0000B262,
	0x150158B8, 0x00000656,
	0x150158BC, 0x00000CE4,
	0x150158C0, 0x000005B7,
	0x150158C4, 0x0000ED65,
	0x150158C8, 0x00000001,
	0x150158CC, 0x00000000,
	0x150158D0, 0x00000000,
	0x150158D4, 0x00000000,
	0x150158D8, 0x0000FFFF,
	0x150158DC, 0x0000FFFF,
	0x150158E0, 0x00000000,
	0x150158E4, 0x0000FFFF,
	0x150158E8, 0x00000000,
	0x150158EC, 0x0000FFFF,
	0x150158F0, 0x00000000,
	0x150158F4, 0x0000FFFF,
	0x150158F8, 0x00002108,
	0x150158FC, 0x00003333,
	0x15015974, 0x00000000,
	0x150159C0, 0x00008000,
	0x150159C4, 0x00000800,
	0x150159C8, 0x00000000,
	0x150159E4, 0x00000001,
	0x150159E8, 0x00001000,
	0x150159EC, 0x00000180,
	0x150159F0, 0x00000000,
	0x15015A04, 0x00000000,
	0x15015A20, 0x00000000,
	0x15015A24, 0x00000000,
	0x15015A28, 0x00000000,
	0x15015A2C, 0x00000000,
	0x15015A30, 0x00000000,
	0x15015A34, 0x00000000,
	0x15015A38, 0x00000000,
	0x15015A3C, 0x00000000,
	0x15015A40, 0x00000000,
	0x15015A44, 0x00000000,
	0x15015A48, 0x00000000,
	0x15015A4C, 0x00000000,
	0x15015A50, 0x00000000,
	0x15015A54, 0x00000000,
	0x15015A58, 0x00000000,
	0x15015A5C, 0x00000000,
	0x15015A60, 0x00000300,
	0x15015A64, 0x00000000,
	0x15015B00, 0x00000001,
	0x15015B4C, 0x00000000,
	0x15015C10, 0x00000000,
	0x15015C14, 0x00000000,
	0x15015C18, 0x00000000,
	0x15015C1C, 0x00000000,
	0x15015C20, 0x00000000,
	0x15015C24, 0x00000000,
	0x15015C28, 0x00000000,
	0x15015C2C, 0x00000000,
	0x15015C30, 0x00000000,
	0x15015C34, 0x00000000,
	0x15015C38, 0x00000000,
	0x15015C3C, 0x00000000,
	0x15015C40, 0x00000000,
	0x15015C44, 0x00000000,
	0x15015C48, 0x00000000,
	0x15015C4C, 0x00000000,
	0x15015C50, 0x00000000,
	0x15015C54, 0x00000000,
	0x15015C58, 0x00000000,
	0x15015C5C, 0x00000000,
	0x15015C60, 0x00000001,
	0x15015EAC, 0x00000000,
};

int dovi_get_sub_video_setting(enum dovi_signal_format_t out_fmt)
{
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	uint32_t *p_reg = NULL;
	uint32_t len = 0;
	uint32_t i = 0;
	uint32_t addr = 0;
	uint16_t value = 0;

	p_reg_tbl = &dv_dsys_all_reg;

	p_reg = sub_sdr_ipt_reg;
	len = 722;

	for (i = 0; i < len; i += 2) {
		addr = p_reg[i];
		value = p_reg[i+1] & 0xffff;
		dv_w_regtbl_2bytes(addr, value, 0xffff, p_reg_tbl);
	}

	reg_hdr_h_size(hdr_input_width[1], SVDO_HDR_FE_BASE, p_reg_tbl);
	reg_hdr_v_size(hdr_input_height[1], SVDO_HDR_FE_BASE, p_reg_tbl);

	return 0;
}
int dovi_get_cp1_setting(uint32_t id, struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_comp_reg *p_dv_comp_reg;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	p_reg_tbl = &dv_dsys_all_reg;
	p_dv_comp_reg = &p_dv_reg->dv_comp[id];

	if (id == 0)
		reg_base = MVDO_HDR_FE_BASE;
	else
		reg_base = SVDO_HDR_FE_BASE;

	reg_mmr_coeff_u0(p_dv_comp_reg->coeff_mmr_cb[0], reg_base, p_reg_tbl);
	reg_mmr_coeff_u1(p_dv_comp_reg->coeff_mmr_cb[1], reg_base, p_reg_tbl);
	reg_mmr_coeff_u2(p_dv_comp_reg->coeff_mmr_cb[2], reg_base, p_reg_tbl);
	reg_mmr_coeff_u3(p_dv_comp_reg->coeff_mmr_cb[3], reg_base, p_reg_tbl);
	reg_mmr_coeff_u4(p_dv_comp_reg->coeff_mmr_cb[4], reg_base, p_reg_tbl);
	reg_mmr_coeff_u5(p_dv_comp_reg->coeff_mmr_cb[5], reg_base, p_reg_tbl);
	reg_mmr_coeff_u6(p_dv_comp_reg->coeff_mmr_cb[6], reg_base, p_reg_tbl);
	reg_mmr_coeff_u7(p_dv_comp_reg->coeff_mmr_cb[7], reg_base, p_reg_tbl);
	reg_mmr_coeff_u8(p_dv_comp_reg->coeff_mmr_cb[8], reg_base, p_reg_tbl);
	reg_mmr_coeff_u9(p_dv_comp_reg->coeff_mmr_cb[9], reg_base, p_reg_tbl);
	reg_mmr_coeff_u10(p_dv_comp_reg->coeff_mmr_cb[10], reg_base, p_reg_tbl);
	reg_mmr_coeff_u11(p_dv_comp_reg->coeff_mmr_cb[11], reg_base, p_reg_tbl);
	reg_mmr_coeff_u12(p_dv_comp_reg->coeff_mmr_cb[12], reg_base, p_reg_tbl);
	reg_mmr_coeff_u13(p_dv_comp_reg->coeff_mmr_cb[13], reg_base, p_reg_tbl);
	reg_mmr_coeff_u14(p_dv_comp_reg->coeff_mmr_cb[14], reg_base, p_reg_tbl);
	reg_mmr_coeff_u15(p_dv_comp_reg->coeff_mmr_cb[15], reg_base, p_reg_tbl);
	reg_mmr_coeff_u16(p_dv_comp_reg->coeff_mmr_cb[16], reg_base, p_reg_tbl);
	reg_mmr_coeff_u17(p_dv_comp_reg->coeff_mmr_cb[17], reg_base, p_reg_tbl);
	reg_mmr_coeff_u18(p_dv_comp_reg->coeff_mmr_cb[18], reg_base, p_reg_tbl);
	reg_mmr_coeff_u19(p_dv_comp_reg->coeff_mmr_cb[19], reg_base, p_reg_tbl);
	reg_mmr_coeff_u20(p_dv_comp_reg->coeff_mmr_cb[20], reg_base, p_reg_tbl);
	reg_mmr_coeff_u21(p_dv_comp_reg->coeff_mmr_cb[21], reg_base, p_reg_tbl);


	reg_mmr_coeff_v0(p_dv_comp_reg->coeff_mmr_cr[0], reg_base, p_reg_tbl);
	reg_mmr_coeff_v1(p_dv_comp_reg->coeff_mmr_cr[1], reg_base, p_reg_tbl);
	reg_mmr_coeff_v2(p_dv_comp_reg->coeff_mmr_cr[2], reg_base, p_reg_tbl);
	reg_mmr_coeff_v3(p_dv_comp_reg->coeff_mmr_cr[3], reg_base, p_reg_tbl);
	reg_mmr_coeff_v4(p_dv_comp_reg->coeff_mmr_cr[4], reg_base, p_reg_tbl);
	reg_mmr_coeff_v5(p_dv_comp_reg->coeff_mmr_cr[5], reg_base, p_reg_tbl);
	reg_mmr_coeff_v6(p_dv_comp_reg->coeff_mmr_cr[6], reg_base, p_reg_tbl);
	reg_mmr_coeff_v7(p_dv_comp_reg->coeff_mmr_cr[7], reg_base, p_reg_tbl);
	reg_mmr_coeff_v8(p_dv_comp_reg->coeff_mmr_cr[8], reg_base, p_reg_tbl);
	reg_mmr_coeff_v9(p_dv_comp_reg->coeff_mmr_cr[9], reg_base, p_reg_tbl);
	reg_mmr_coeff_v10(p_dv_comp_reg->coeff_mmr_cr[10], reg_base, p_reg_tbl);
	reg_mmr_coeff_v11(p_dv_comp_reg->coeff_mmr_cr[11], reg_base, p_reg_tbl);
	reg_mmr_coeff_v12(p_dv_comp_reg->coeff_mmr_cr[12], reg_base, p_reg_tbl);
	reg_mmr_coeff_v13(p_dv_comp_reg->coeff_mmr_cr[13], reg_base, p_reg_tbl);
	reg_mmr_coeff_v14(p_dv_comp_reg->coeff_mmr_cr[14], reg_base, p_reg_tbl);
	reg_mmr_coeff_v15(p_dv_comp_reg->coeff_mmr_cr[15], reg_base, p_reg_tbl);
	reg_mmr_coeff_v16(p_dv_comp_reg->coeff_mmr_cr[16], reg_base, p_reg_tbl);
	reg_mmr_coeff_v17(p_dv_comp_reg->coeff_mmr_cr[17], reg_base, p_reg_tbl);
	reg_mmr_coeff_v18(p_dv_comp_reg->coeff_mmr_cr[18], reg_base, p_reg_tbl);
	reg_mmr_coeff_v19(p_dv_comp_reg->coeff_mmr_cr[19], reg_base, p_reg_tbl);
	reg_mmr_coeff_v20(p_dv_comp_reg->coeff_mmr_cr[20], reg_base, p_reg_tbl);
	reg_mmr_coeff_v21(p_dv_comp_reg->coeff_mmr_cr[21], reg_base, p_reg_tbl);

	reg_spatial_resampling_mode_hor_idc(
		p_dv_comp_reg->spatial_resampling_mode_hor_idc,
		reg_base, p_reg_tbl);
	reg_spatial_resampling_mode_ver_idc(
		p_dv_comp_reg->spatial_resampling_mode_ver_idc,
		reg_base, p_reg_tbl);

	reg_spatial_filter_coeff_hor_prog(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_prog,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_prog(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_prog,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_hor_00(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_00,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_hor_01(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_01,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_hor_02(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_02,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_hor_03(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_03,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_hor_04(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_04,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_hor_05(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_05,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_hor_06(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_06,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_hor_07(
		p_dv_comp_reg->reg_spatial_filter_coeff_hor_07,
		reg_base, p_reg_tbl);

	reg_spatial_filter_coeff_ver_001(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_001,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_002(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_002,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_003(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_003,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_004(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_004,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_101(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_101,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_102(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_102,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_103(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_103,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_104(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_104,
		reg_base, p_reg_tbl);


	reg_spatial_filter_coeff_ver_002_uv(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_002_uv,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_102_uv(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_102_uv,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_103_uv(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_103_uv,
		reg_base, p_reg_tbl);
	reg_spatial_filter_coeff_ver_prog_uv(
		p_dv_comp_reg->reg_spatial_filter_coeff_ver_prog_uv,
		reg_base, p_reg_tbl);

	reg_mapping_idc_u(p_dv_comp_reg->mapping_idc_cb, reg_base, p_reg_tbl);
	reg_mapping_idc_v(p_dv_comp_reg->mapping_idc_cr, reg_base, p_reg_tbl);
	reg_vdr_bit_depth(p_dv_comp_reg->vdr_bit_depth, reg_base, p_reg_tbl);

	return 0;
}
int dovi_get_cp0_setting(uint32_t id, struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_comp_reg *p_dv_comp_reg;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	p_reg_tbl = &dv_dsys_all_reg;
	p_dv_comp_reg = &p_dv_reg->dv_comp[id];

	if (id == 0)
		reg_base = MVDO_HDR_FE_BASE;
	else
		reg_base = SVDO_HDR_FE_BASE;

	reg_el_coeff_y0(p_dv_comp_reg->NLQ_coeff_y[0], reg_base, p_reg_tbl);
	reg_el_coeff_y1(p_dv_comp_reg->NLQ_coeff_y[1], reg_base, p_reg_tbl);
	reg_el_coeff_y2(p_dv_comp_reg->NLQ_coeff_y[2], reg_base, p_reg_tbl);

	reg_el_coeff_u0(p_dv_comp_reg->NLQ_coeff_cb[0], reg_base, p_reg_tbl);
	reg_el_coeff_u1(p_dv_comp_reg->NLQ_coeff_cb[1], reg_base, p_reg_tbl);
	reg_el_coeff_u2(p_dv_comp_reg->NLQ_coeff_cb[2], reg_base, p_reg_tbl);

	reg_el_coeff_v0(p_dv_comp_reg->NLQ_coeff_cr[0], reg_base, p_reg_tbl);
	reg_el_coeff_v1(p_dv_comp_reg->NLQ_coeff_cr[1], reg_base, p_reg_tbl);
	reg_el_coeff_v2(p_dv_comp_reg->NLQ_coeff_cr[2], reg_base, p_reg_tbl);

	reg_el_nlq_offset_y(p_dv_comp_reg->NLQ_offset_y, reg_base, p_reg_tbl);
	reg_el_nlq_offset_u(p_dv_comp_reg->NLQ_offset_cb, reg_base, p_reg_tbl);
	reg_el_nlq_offset_v(p_dv_comp_reg->NLQ_offset_cr, reg_base, p_reg_tbl);

	reg_mode(1-p_dv_comp_reg->disable_EL_flag, reg_base, p_reg_tbl);

	reg_bl_dat_8b((p_dv_comp_reg->rpu_BL_bit_depth == 8) ?
		1 : 0, reg_base, p_reg_tbl);
	reg_el_dat_8b((p_dv_comp_reg->rpu_EL_bit_depth == 8) ?
		1 : 0, reg_base, p_reg_tbl);

	reg_el_spatial_resampling_filter_flag(
		p_dv_comp_reg->el_spatial_resampling_filter_flag,
		reg_base, p_reg_tbl);

	reg_bl_pivot_y0(p_dv_comp_reg->pivot_value_y[0], reg_base, p_reg_tbl);
	reg_bl_pivot_y1(p_dv_comp_reg->pivot_value_y[1], reg_base, p_reg_tbl);
	reg_bl_pivot_y2(p_dv_comp_reg->pivot_value_y[2], reg_base, p_reg_tbl);
	reg_bl_pivot_y3(p_dv_comp_reg->pivot_value_y[3], reg_base, p_reg_tbl);
	reg_bl_pivot_y4(p_dv_comp_reg->pivot_value_y[4], reg_base, p_reg_tbl);
	reg_bl_pivot_y5(p_dv_comp_reg->pivot_value_y[5], reg_base, p_reg_tbl);
	reg_bl_pivot_y6(p_dv_comp_reg->pivot_value_y[6], reg_base, p_reg_tbl);
	reg_bl_pivot_y7(p_dv_comp_reg->pivot_value_y[7], reg_base, p_reg_tbl);
	reg_bl_pivot_y8(p_dv_comp_reg->pivot_value_y[8], reg_base, p_reg_tbl);

	reg_bl_coeff_y00(p_dv_comp_reg->coeff_y[0], reg_base, p_reg_tbl);
	reg_bl_coeff_y01(p_dv_comp_reg->coeff_y[1], reg_base, p_reg_tbl);
	reg_bl_coeff_y02(p_dv_comp_reg->coeff_y[2], reg_base, p_reg_tbl);
	reg_bl_coeff_y10(p_dv_comp_reg->coeff_y[3], reg_base, p_reg_tbl);
	reg_bl_coeff_y11(p_dv_comp_reg->coeff_y[4], reg_base, p_reg_tbl);
	reg_bl_coeff_y12(p_dv_comp_reg->coeff_y[5], reg_base, p_reg_tbl);
	reg_bl_coeff_y20(p_dv_comp_reg->coeff_y[6], reg_base, p_reg_tbl);
	reg_bl_coeff_y21(p_dv_comp_reg->coeff_y[7], reg_base, p_reg_tbl);
	reg_bl_coeff_y22(p_dv_comp_reg->coeff_y[8], reg_base, p_reg_tbl);
	reg_bl_coeff_y30(p_dv_comp_reg->coeff_y[9], reg_base, p_reg_tbl);
	reg_bl_coeff_y31(p_dv_comp_reg->coeff_y[10], reg_base, p_reg_tbl);
	reg_bl_coeff_y32(p_dv_comp_reg->coeff_y[11], reg_base, p_reg_tbl);
	reg_bl_coeff_y40(p_dv_comp_reg->coeff_y[12], reg_base, p_reg_tbl);
	reg_bl_coeff_y41(p_dv_comp_reg->coeff_y[13], reg_base, p_reg_tbl);
	reg_bl_coeff_y42(p_dv_comp_reg->coeff_y[14], reg_base, p_reg_tbl);
	reg_bl_coeff_y50(p_dv_comp_reg->coeff_y[15], reg_base, p_reg_tbl);
	reg_bl_coeff_y51(p_dv_comp_reg->coeff_y[16], reg_base, p_reg_tbl);
	reg_bl_coeff_y52(p_dv_comp_reg->coeff_y[17], reg_base, p_reg_tbl);
	reg_bl_coeff_y60(p_dv_comp_reg->coeff_y[18], reg_base, p_reg_tbl);
	reg_bl_coeff_y61(p_dv_comp_reg->coeff_y[19], reg_base, p_reg_tbl);
	reg_bl_coeff_y62(p_dv_comp_reg->coeff_y[20], reg_base, p_reg_tbl);
	reg_bl_coeff_y70(p_dv_comp_reg->coeff_y[21], reg_base, p_reg_tbl);
	reg_bl_coeff_y71(p_dv_comp_reg->coeff_y[22], reg_base, p_reg_tbl);
	reg_bl_coeff_y72(p_dv_comp_reg->coeff_y[23], reg_base, p_reg_tbl);


	reg_bl_pivot_u0(p_dv_comp_reg->pivot_value_cb[0], reg_base, p_reg_tbl);
	reg_bl_pivot_u1(p_dv_comp_reg->pivot_value_cb[1], reg_base, p_reg_tbl);
	reg_bl_pivot_u2(p_dv_comp_reg->pivot_value_cb[2], reg_base, p_reg_tbl);
	reg_bl_pivot_u3(p_dv_comp_reg->pivot_value_cb[3], reg_base, p_reg_tbl);
	reg_bl_pivot_u4(p_dv_comp_reg->pivot_value_cb[4], reg_base, p_reg_tbl);


	reg_bl_coeff_u00(p_dv_comp_reg->coeff_cb[0], reg_base, p_reg_tbl);
	reg_bl_coeff_u01(p_dv_comp_reg->coeff_cb[1], reg_base, p_reg_tbl);
	reg_bl_coeff_u02(p_dv_comp_reg->coeff_cb[2], reg_base, p_reg_tbl);
	reg_bl_coeff_u10(p_dv_comp_reg->coeff_cb[3], reg_base, p_reg_tbl);
	reg_bl_coeff_u11(p_dv_comp_reg->coeff_cb[4], reg_base, p_reg_tbl);
	reg_bl_coeff_u12(p_dv_comp_reg->coeff_cb[5], reg_base, p_reg_tbl);
	reg_bl_coeff_u20(p_dv_comp_reg->coeff_cb[6], reg_base, p_reg_tbl);
	reg_bl_coeff_u21(p_dv_comp_reg->coeff_cb[7], reg_base, p_reg_tbl);
	reg_bl_coeff_u22(p_dv_comp_reg->coeff_cb[8], reg_base, p_reg_tbl);
	reg_bl_coeff_u30(p_dv_comp_reg->coeff_cb[9], reg_base, p_reg_tbl);
	reg_bl_coeff_u31(p_dv_comp_reg->coeff_cb[10], reg_base, p_reg_tbl);
	reg_bl_coeff_u32(p_dv_comp_reg->coeff_cb[11], reg_base, p_reg_tbl);


	reg_bl_pivot_v0(p_dv_comp_reg->pivot_value_cr[0], reg_base, p_reg_tbl);
	reg_bl_pivot_v1(p_dv_comp_reg->pivot_value_cr[1], reg_base, p_reg_tbl);
	reg_bl_pivot_v2(p_dv_comp_reg->pivot_value_cr[2], reg_base, p_reg_tbl);
	reg_bl_pivot_v3(p_dv_comp_reg->pivot_value_cr[3], reg_base, p_reg_tbl);
	reg_bl_pivot_v4(p_dv_comp_reg->pivot_value_cr[4], reg_base, p_reg_tbl);


	reg_bl_coeff_v00(p_dv_comp_reg->coeff_cr[0], reg_base, p_reg_tbl);
	dovi_info("2398 0x%x", p_dv_comp_reg->coeff_cr[0]);
	reg_bl_coeff_v01(p_dv_comp_reg->coeff_cr[1], reg_base, p_reg_tbl);
	reg_bl_coeff_v02(p_dv_comp_reg->coeff_cr[2], reg_base, p_reg_tbl);
	reg_bl_coeff_v10(p_dv_comp_reg->coeff_cr[3], reg_base, p_reg_tbl);
	reg_bl_coeff_v11(p_dv_comp_reg->coeff_cr[4], reg_base, p_reg_tbl);
	reg_bl_coeff_v12(p_dv_comp_reg->coeff_cr[5], reg_base, p_reg_tbl);
	reg_bl_coeff_v20(p_dv_comp_reg->coeff_cr[6], reg_base, p_reg_tbl);
	reg_bl_coeff_v21(p_dv_comp_reg->coeff_cr[7], reg_base, p_reg_tbl);
	reg_bl_coeff_v22(p_dv_comp_reg->coeff_cr[8], reg_base, p_reg_tbl);
	reg_bl_coeff_v30(p_dv_comp_reg->coeff_cr[9], reg_base, p_reg_tbl);
	reg_bl_coeff_v31(p_dv_comp_reg->coeff_cr[10], reg_base, p_reg_tbl);
	reg_bl_coeff_v32(p_dv_comp_reg->coeff_cr[11], reg_base, p_reg_tbl);
	return 0;
}

int dovi_get_ctrl_setting(uint32_t id, struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_hdr_ctrl_reg *p_dv_ctrl_reg = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	p_reg_tbl = &dv_dsys_all_reg;
	p_dv_ctrl_reg = &p_dv_reg->dv_ctrl[id];

	if (id == 0)
		reg_base = MVDO_HDR_FE_BASE;
	else
		reg_base = SVDO_HDR_FE_BASE;

	//defalut path internal bypass setting
	reg_0618_default(0, reg_base, p_reg_tbl);
	reg_061c_default(0x2, reg_base, p_reg_tbl);
	reg_0634_default(0x8000, reg_base, p_reg_tbl);
	reg_0804_default(0xfd, reg_base, p_reg_tbl);
	reg_081c_default(0x12e, reg_base, p_reg_tbl);
	reg_09ec_default(0x80, reg_base, p_reg_tbl);

	reg_dm_src_sel(p_dv_ctrl_reg->dm_src_sel, reg_base, p_reg_tbl);
	reg_hdrin_shift(p_dv_ctrl_reg->dm_src_sel, reg_base, p_reg_tbl);
	reg_hdr2ip_force_ack(0, reg_base, p_reg_tbl);
	reg_hdr2ip_path_en(0x2, reg_base, p_reg_tbl);
	reg_hdr12b_compose_in(0x8001, reg_base, p_reg_tbl);

	reg_dm_msb_align_en(p_dv_ctrl_reg->dm_msb_align_en,
		reg_base, p_reg_tbl);

	//reg_hdr_h_size(p_dv_ctrl_reg->hdr_h_size, reg_base, p_reg_tbl);
	//reg_hdr_v_size(p_dv_ctrl_reg->hdr_v_size, reg_base, p_reg_tbl);

	reg_hdr2ip_force_ack(p_dv_ctrl_reg->hdr2ip_force_ack,
		reg_base, p_reg_tbl);
	reg_aoi_en(p_dv_ctrl_reg->aoi_en, reg_base, p_reg_tbl);
	reg_edclk_en(p_dv_ctrl_reg->edclk_en, reg_base, p_reg_tbl);

	reg_hdr_h_size(hdr_input_width[id], reg_base, p_reg_tbl);
	reg_hdr_v_size(hdr_input_height[id], reg_base, p_reg_tbl);

	//add need reset openhdr register
	reg_oot_shift(0, reg_base, p_reg_tbl);
	reg_oot_3x1_m0(0, reg_base, p_reg_tbl);
	reg_oot_3x1_m1(0, reg_base, p_reg_tbl);
	reg_oot_3x1_m2(0, reg_base, p_reg_tbl);
	reg_mst_hdr_gamma_output_limit_1(0xffff, reg_base, p_reg_tbl);
	reg_dc0_to_dst_sel(0, reg_base, p_reg_tbl);
	reg_dm_dither_sel(0, reg_base, p_reg_tbl);
	reg_uvc_new_en(1, reg_base, p_reg_tbl);
	reg_low_y_close_to_ori_en(0, reg_base, p_reg_tbl);
	reg_close_to_one_en(0, reg_base, p_reg_tbl);
	reg_low_y_sat_prot_en(0, reg_base, p_reg_tbl);
	reg_dm_b0202_open_uvc_en(0, reg_base, p_reg_tbl);
	reg_dm_b0202_tmo_i_only_en(0, reg_base, p_reg_tbl);
	reg_thdr_r2y_en(0, reg_base, p_reg_tbl);

	return 0;
}


int dovi_get_vdo_dm_setting(uint32_t id, struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_dm_reg *p_dv_dm_reg = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	p_reg_tbl = &dv_dsys_all_reg;
	p_dv_dm_reg = &p_dv_reg->dv_dm[id];

	if (id == 0)
		reg_base = MVDO_HDR_FE_BASE;
	else
		reg_base = SVDO_HDR_FE_BASE;

	/* DM top */
	reg_csc2ipt_pq_12bits_en(p_dv_dm_reg->b01.reg_b01_out_12bits_en,
	reg_base, p_reg_tbl);

	/* B0101 */
	reg_cup420_en(p_dv_dm_reg->b0101.cup420_en, reg_base, p_reg_tbl);
	reg_420repeat_en(p_dv_dm_reg->b0101.repeat_en, reg_base, p_reg_tbl);
	reg_422to444_en(p_dv_dm_reg->b0101._422to444_en, reg_base, p_reg_tbl);
	reg_cup420_43mode(p_dv_dm_reg->b0101.cup420_43mode, reg_base,
		p_reg_tbl);
	reg_cup420_cbcr_cross_en(p_dv_dm_reg->b0101.cup420_cbcr_cross_en,
		reg_base, p_reg_tbl);
	reg_cup420_0101_mode(p_dv_dm_reg->b0101.cup420_0101_mode, reg_base,
		p_reg_tbl);
	reg_cup420_la_md(p_dv_dm_reg->b0101.cup420_la_md, reg_base, p_reg_tbl);
	reg_cup420_tb_md(p_dv_dm_reg->b0101.cup420_tb_md, reg_base, p_reg_tbl);
	reg_cup420_c_in_r_ch(p_dv_dm_reg->b0101.cup420_c_in_r_ch, reg_base,
		p_reg_tbl);
	reg_b0101_clp_sel(p_dv_dm_reg->b0101.b0101_clp_sel, reg_base,
		p_reg_tbl);

	/* B0102 */
	reg_b0102_byp_en(p_dv_dm_reg->b0102.reg_b0102_byp_en, reg_base,
	p_reg_tbl);
	reg_ycbcr_m0(p_dv_dm_reg->b0102.reg_ycbcr_m0, reg_base, p_reg_tbl);
	reg_ycbcr_m1(p_dv_dm_reg->b0102.reg_ycbcr_m1, reg_base, p_reg_tbl);
	reg_ycbcr_m2(p_dv_dm_reg->b0102.reg_ycbcr_m2, reg_base, p_reg_tbl);
	reg_ycbcr_m3(p_dv_dm_reg->b0102.reg_ycbcr_m3, reg_base, p_reg_tbl);
	reg_ycbcr_m4(p_dv_dm_reg->b0102.reg_ycbcr_m4, reg_base, p_reg_tbl);
	reg_ycbcr_m5(p_dv_dm_reg->b0102.reg_ycbcr_m5, reg_base, p_reg_tbl);
	reg_ycbcr_m6(p_dv_dm_reg->b0102.reg_ycbcr_m6, reg_base, p_reg_tbl);
	reg_ycbcr_m7(p_dv_dm_reg->b0102.reg_ycbcr_m7, reg_base, p_reg_tbl);
	reg_ycbcr_m8(p_dv_dm_reg->b0102.reg_ycbcr_m8, reg_base, p_reg_tbl);
	reg_ycbcr_offset_0(p_dv_dm_reg->b0102.reg_ycbcr_offset_0,
		reg_base, p_reg_tbl);
	reg_ycbcr_offset_1(p_dv_dm_reg->b0102.reg_ycbcr_offset_1,
		reg_base, p_reg_tbl);
	reg_ycbcr_offset_2(p_dv_dm_reg->b0102.reg_ycbcr_offset_2,
		reg_base, p_reg_tbl);
	reg_clip_max(p_dv_dm_reg->b0102.reg_range_clip, reg_base, p_reg_tbl);
	reg_ycbcr_shift(p_dv_dm_reg->b0102.reg_ycbcr_shift,
		reg_base, p_reg_tbl);
	reg_range_min_7a(p_dv_dm_reg->b0102.reg_range_min,
		reg_base, p_reg_tbl);
	reg_range_max(p_dv_dm_reg->b0102.reg_range_max, reg_base, p_reg_tbl);
	reg_range_inv_7a(p_dv_dm_reg->b0102.reg_range_inv,
		reg_base, p_reg_tbl);
	reg_y2r_byp_en(p_dv_dm_reg->b0102.reg_b0102_y2r_byp_en,
		reg_base, p_reg_tbl);
	reg_y2r_byp_shift(p_dv_dm_reg->b0102.reg_b0102_y2r_byp_shift,
		reg_base, p_reg_tbl);

	/* B0103 */
	reg_b0103_byp_en(p_dv_dm_reg->b0103.reg_b0103_byp_en,
	reg_base, p_reg_tbl);
	reg_b0103_eotf_mode(p_dv_dm_reg->b0103.reg_b0103_eotf_mode,
		reg_base, p_reg_tbl);

	/* B0104 */
	reg_b0104_byp_en(p_dv_dm_reg->b0104.reg_b0104_byp_en,
	reg_base, p_reg_tbl);
	reg_csa2csb_m0(p_dv_dm_reg->b0104.reg_csa2csb_m0, reg_base, p_reg_tbl);
	reg_csa2csb_m1(p_dv_dm_reg->b0104.reg_csa2csb_m1, reg_base, p_reg_tbl);
	reg_csa2csb_m2(p_dv_dm_reg->b0104.reg_csa2csb_m2, reg_base, p_reg_tbl);
	reg_csa2csb_m3(p_dv_dm_reg->b0104.reg_csa2csb_m3, reg_base, p_reg_tbl);
	reg_csa2csb_m4(p_dv_dm_reg->b0104.reg_csa2csb_m4, reg_base, p_reg_tbl);
	reg_csa2csb_m5(p_dv_dm_reg->b0104.reg_csa2csb_m5, reg_base, p_reg_tbl);
	reg_csa2csb_m6(p_dv_dm_reg->b0104.reg_csa2csb_m6, reg_base, p_reg_tbl);
	reg_csa2csb_m7(p_dv_dm_reg->b0104.reg_csa2csb_m7, reg_base, p_reg_tbl);
	reg_csa2csb_m8(p_dv_dm_reg->b0104.reg_csa2csb_m8, reg_base, p_reg_tbl);
	reg_csa2csb_shift(p_dv_dm_reg->b0104.reg_csa2csb_shift,
		reg_base, p_reg_tbl);

	/* B0105 */
	reg_b0105_byp_en(p_dv_dm_reg->b0105.reg_b0105_byp_en,
	reg_base, p_reg_tbl);
	reg_b0105_gam_en(p_dv_dm_reg->b0105.reg_hdr_gamma_select_en,
		reg_base, p_reg_tbl);

	/* OOTF */
	reg_ootf_en(p_dv_dm_reg->ootf.reg_ootf_en, reg_base, p_reg_tbl);

	/* B0106 */
	reg_b0106_byp_en(p_dv_dm_reg->b0106.reg_b0106_byp_en,
	reg_base, p_reg_tbl);
	reg_csc2ipt_shift(p_dv_dm_reg->b0106.reg_csc2ipt_shift,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m0(p_dv_dm_reg->b0106.reg_csc2ipt_m0, reg_base, p_reg_tbl);
	reg_csc2ipt_m1(p_dv_dm_reg->b0106.reg_csc2ipt_m1, reg_base, p_reg_tbl);
	reg_csc2ipt_m2(p_dv_dm_reg->b0106.reg_csc2ipt_m2, reg_base, p_reg_tbl);
	reg_csc2ipt_m3(p_dv_dm_reg->b0106.reg_csc2ipt_m3, reg_base, p_reg_tbl);
	reg_csc2ipt_m4(p_dv_dm_reg->b0106.reg_csc2ipt_m4, reg_base, p_reg_tbl);
	reg_csc2ipt_m5(p_dv_dm_reg->b0106.reg_csc2ipt_m5, reg_base, p_reg_tbl);
	reg_csc2ipt_m6(p_dv_dm_reg->b0106.reg_csc2ipt_m6, reg_base, p_reg_tbl);
	reg_csc2ipt_m7(p_dv_dm_reg->b0106.reg_csc2ipt_m7, reg_base, p_reg_tbl);
	reg_csc2ipt_m8(p_dv_dm_reg->b0106.reg_csc2ipt_m8, reg_base, p_reg_tbl);

	/* B02 */
	reg_b02_byp_en(p_dv_dm_reg->b02.reg_b02_byp_en, reg_base, p_reg_tbl);
	reg_b0202_byp_clamp_en(p_dv_dm_reg->b02.reg_b0202_byp_clamp_en,
		reg_base, p_reg_tbl);
	reg_si_byp_en(p_dv_dm_reg->b02.reg_si_byp_en, reg_base, p_reg_tbl);
	reg_ss_byp_en(p_dv_dm_reg->b02.reg_ss_byp_en, reg_base, p_reg_tbl);
	reg_ti_byp_en(p_dv_dm_reg->b02.reg_ti_byp_en, reg_base, p_reg_tbl);
	reg_ts_byp_en(p_dv_dm_reg->b02.reg_ts_byp_en, reg_base, p_reg_tbl);
	reg_dm_b0202_shift(p_dv_dm_reg->b02.reg_dm_b0202_shift,
		reg_base, p_reg_tbl);

	return 0;
}

int dovi_get_vdo_lut_setting(uint32_t id, struct dv_hw_reg *p_dv_reg)
{

	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	p_reg_tbl = &dv_dsys_all_reg;

	if (id == 0)
		reg_base = MVDO_HDR_FE_BASE;
	else
		reg_base = SVDO_HDR_FE_BASE;

	reg_autod_lut_md_7a(1, reg_base, p_reg_tbl); // use autodownload
	reg_autod_trigger_md_7a(adl_mode, reg_base, p_reg_tbl);
	//reg_autod_protect_7a(0, reg_base, p_reg_tbl); //close protect bits

	return 0;
}

int dovi_get_gop_setting(uint32_t id, struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_gop_reg *p_dv_gop_reg = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	p_reg_tbl = &dv_msys_all_reg;
	p_dv_gop_reg = &p_dv_reg->dv_gop[id];

	if (id == 0)
		reg_base = FHD_HDR_FE_BASE;
	else
		reg_base = UHD_HDR_FE_BASE;

	//default gop internal by pass setting
	reg_9100_gop(0x8001, reg_base, p_reg_tbl);
	reg_9204_gop(0xfd, reg_base, p_reg_tbl);
	reg_921c_gop(0x20, reg_base, p_reg_tbl);
	reg_93ec_gop(0x80, reg_base, p_reg_tbl);

	/* B0102 */
	reg_b0102_byp_en_gop(p_dv_gop_reg->b0102.reg_b0102_byp_en,
	reg_base, p_reg_tbl);
	reg_ycbcr_m0_gop(p_dv_gop_reg->b0102.reg_ycbcr_m0, reg_base, p_reg_tbl);
	reg_ycbcr_m1_gop(p_dv_gop_reg->b0102.reg_ycbcr_m1, reg_base, p_reg_tbl);
	reg_ycbcr_m2_gop(p_dv_gop_reg->b0102.reg_ycbcr_m2, reg_base, p_reg_tbl);
	reg_ycbcr_m3_gop(p_dv_gop_reg->b0102.reg_ycbcr_m3, reg_base, p_reg_tbl);
	reg_ycbcr_m4_gop(p_dv_gop_reg->b0102.reg_ycbcr_m4, reg_base, p_reg_tbl);
	reg_ycbcr_m5_gop(p_dv_gop_reg->b0102.reg_ycbcr_m5, reg_base, p_reg_tbl);
	reg_ycbcr_m6_gop(p_dv_gop_reg->b0102.reg_ycbcr_m6, reg_base, p_reg_tbl);
	reg_ycbcr_m7_gop(p_dv_gop_reg->b0102.reg_ycbcr_m7, reg_base, p_reg_tbl);
	reg_ycbcr_m8_gop(p_dv_gop_reg->b0102.reg_ycbcr_m8, reg_base, p_reg_tbl);
	reg_ycbcr_offset_0_gop(p_dv_gop_reg->b0102.reg_ycbcr_offset_0,
		reg_base, p_reg_tbl);
	reg_ycbcr_offset_1_gop(p_dv_gop_reg->b0102.reg_ycbcr_offset_1,
		reg_base, p_reg_tbl);
	reg_ycbcr_offset_2_gop(p_dv_gop_reg->b0102.reg_ycbcr_offset_2,
		reg_base, p_reg_tbl);
	reg_clip_max_gop(p_dv_gop_reg->b0102.reg_range_clip, reg_base,
		p_reg_tbl);
	reg_ycbcr_shift_gop(p_dv_gop_reg->b0102.reg_ycbcr_shift,
		reg_base, p_reg_tbl);
	reg_range_min_gop(p_dv_gop_reg->b0102.reg_range_min,
		reg_base, p_reg_tbl);
	reg_range_max_gop(p_dv_gop_reg->b0102.reg_range_max,
		reg_base, p_reg_tbl);
	reg_range_inv_gop(p_dv_gop_reg->b0102.reg_range_inv,
		reg_base, p_reg_tbl);
	reg_b0102_y2r_byp_en_gop(p_dv_gop_reg->b0102.reg_b0102_y2r_byp_en,
		reg_base, p_reg_tbl);
	reg_b0102_y2r_byp_shift_gop(p_dv_gop_reg->b0102.reg_b0102_y2r_byp_shift,
		reg_base, p_reg_tbl);

	/* B0103 */
	reg_b0103_byp_en_gop(p_dv_gop_reg->b0103.reg_b0103_byp_en, reg_base,
	p_reg_tbl);
	reg_b0103_eotf_mode_gop(p_dv_gop_reg->b0103.reg_b0103_eotf_mode,
		reg_base, p_reg_tbl);

	/* B0104 */
	reg_b0104_byp_en_gop(p_dv_gop_reg->b0104.reg_b0104_byp_en,
	reg_base, p_reg_tbl);
	reg_csa2csb_m0_gop(p_dv_gop_reg->b0104.reg_csa2csb_m0,
		reg_base, p_reg_tbl);
	reg_csa2csb_m1_gop(p_dv_gop_reg->b0104.reg_csa2csb_m1,
		reg_base, p_reg_tbl);
	reg_csa2csb_m2_gop(p_dv_gop_reg->b0104.reg_csa2csb_m2,
		reg_base, p_reg_tbl);
	reg_csa2csb_m3_gop(p_dv_gop_reg->b0104.reg_csa2csb_m3,
		reg_base, p_reg_tbl);
	reg_csa2csb_m4_gop(p_dv_gop_reg->b0104.reg_csa2csb_m4,
		reg_base, p_reg_tbl);
	reg_csa2csb_m5_gop(p_dv_gop_reg->b0104.reg_csa2csb_m5,
		reg_base, p_reg_tbl);
	reg_csa2csb_m6_gop(p_dv_gop_reg->b0104.reg_csa2csb_m6,
		reg_base, p_reg_tbl);
	reg_csa2csb_m7_gop(p_dv_gop_reg->b0104.reg_csa2csb_m7,
		reg_base, p_reg_tbl);
	reg_csa2csb_m8_gop(p_dv_gop_reg->b0104.reg_csa2csb_m8,
		reg_base, p_reg_tbl);
	reg_csa2csb_shift_gop(p_dv_gop_reg->b0104.reg_csa2csb_shift,
		reg_base, p_reg_tbl);

	/* B0105 */
	reg_b0105_byp_en_gop(0, reg_base, p_reg_tbl);

	/* B0106 */
	reg_b0106_byp_en_gop(p_dv_gop_reg->b0106.reg_b0106_byp_en,
	reg_base, p_reg_tbl);
	reg_csc2ipt_shift_gop(p_dv_gop_reg->b0106.reg_csc2ipt_shift,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m0_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m0,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m1_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m1,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m2_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m2,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m3_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m3,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m4_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m4,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m5_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m5,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m6_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m6,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m7_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m7,
		reg_base, p_reg_tbl);
	reg_csc2ipt_m8_gop(p_dv_gop_reg->b0106.reg_csc2ipt_m8,
		reg_base, p_reg_tbl);

	/* B02 */
	reg_b02_byp_en_gop(p_dv_gop_reg->b02.reg_b02_byp_en,
	reg_base, p_reg_tbl);
	reg_b0202_byp_clamp_en_gop(p_dv_gop_reg->b02.reg_b0202_byp_clamp_en,
		reg_base, p_reg_tbl);
	reg_dm_b0202_shift_gop(p_dv_gop_reg->b02.reg_dm_b0202_shift,
		reg_base, p_reg_tbl);
	reg_si_byp_en_gop(p_dv_gop_reg->b02.reg_si_byp_en,
		reg_base, p_reg_tbl);
	reg_ss_byp_en_gop(p_dv_gop_reg->b02.reg_ss_byp_en,
		reg_base, p_reg_tbl);
	reg_ti_byp_en_gop(p_dv_gop_reg->b02.reg_ti_byp_en,
		reg_base, p_reg_tbl);
	reg_ts_byp_en_gop(p_dv_gop_reg->b02.reg_ts_byp_en,
		reg_base, p_reg_tbl);

	reg_r2y_gop(0, reg_base, p_reg_tbl);
	reg_edclk_en_gop(1, reg_base, p_reg_tbl);
	reg_hdr_gfx_sw_rst_gop(0, reg_base, p_reg_tbl);
	reg_hdr_gfx_sram_pd_en_gop(0, reg_base, p_reg_tbl);
	reg_csc2ipt_pq_12bits_en_gop(1, reg_base, p_reg_tbl);
	reg_hdr_v_size_gop(dovi_out_height, reg_base, p_reg_tbl);
	reg_dm_msb_align_en_gop(0, reg_base, p_reg_tbl);

	return 0;
}


int dovi_get_gfx_luts_setting(uint32_t id, struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	p_reg_tbl = &dv_msys_all_reg;

	if (id == 0)
		reg_base = FHD_HDR_FE_BASE;
	else
		reg_base = UHD_HDR_FE_BASE;

	reg_autod_lut_md_gop(1, reg_base, p_reg_tbl); // use autodownload
	reg_autod_trigger_md_gop(adl_mode, reg_base, p_reg_tbl);
	//reg_autod_protect_gop(0, reg_base, p_reg_tbl);//close protect mode

	return 0;
}

int dovi_get_dith_setting(struct dv_hw_reg *p_dv_reg)
{

	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_dither_reg *p_dv_dither_reg = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	reg_base = VDO_BE_REG_BASE;

	p_reg_tbl = &dv_msys_all_reg;
	p_dv_dither_reg = &p_dv_reg->dv_dither;

	if (dump_bit_depth == 12) {
		reg_hdr_dith_en(0x0, reg_base, p_reg_tbl);
		reg_hdr_dith_444md(0x0, reg_base, p_reg_tbl);
	} else {
		reg_hdr_dith_en(p_dv_dither_reg->hdr_dith_en,
			reg_base, p_reg_tbl);
		reg_hdr_dith_444md(p_dv_dither_reg->hdr_dith_444md,
			reg_base, p_reg_tbl);
	}
	reg_hdr_dith_8b_md(p_dv_dither_reg->hdr_dith_8b_md,
		reg_base, p_reg_tbl);
	reg_hdr_pseudo_dith_stop(p_dv_dither_reg->hdr_byp_dith_reorder,
		reg_base, p_reg_tbl);
	reg_hdr_dith_force_window(p_dv_dither_reg->hdr_dith_force_window,
		reg_base, p_reg_tbl);

	return 0;
}


int dovi_get_rord_setting(struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_reorder_reg *p_dv_reorder_reg = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	reg_base = VDO_BE_REG_BASE;

	p_reg_tbl = &dv_msys_all_reg;
	p_dv_reorder_reg = &p_dv_reg->dv_reorder;

	reg_reorder_en(p_dv_reorder_reg->reg_reorder_en,
		reg_base, p_reg_tbl);
	reg_byp_y2r_reorder_disable(
		p_dv_reorder_reg->reg_byp_y2r_reorder_disable,
		reg_base, p_reg_tbl);

	return 0;
}


int dovi_get_scm_setting(struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_scramble_reg *p_dv_scm_reg = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	reg_base = VDO_BE_REG_BASE;

	p_reg_tbl = &dv_msys_all_reg;
	p_dv_scm_reg = &p_dv_reg->dv_scm;

	if (p_dv_scm_reg->reg_meta_pkt_num) {
		reg_autod_lut_md_7b(1, reg_base, p_reg_tbl);
		reg_autod_trigger_md_7b(0x8000, reg_base, p_reg_tbl);
		reg_scamble_v_size_7b(dovi_out_height, reg_base, p_reg_tbl);
		reg_scramble_byp_en(0, reg_base, p_reg_tbl);
		reg_meta_pkt_num(p_dv_scm_reg->reg_meta_pkt_num,
			reg_base, p_reg_tbl);
		reg_meta_pkt_repeat_num(p_dv_scm_reg->reg_meta_pkt_repeat_num,
			reg_base, p_reg_tbl);
		reg_meta_len_per_pkt(p_dv_scm_reg->reg_meta_len_per_pkt,
			reg_base, p_reg_tbl);

	} else {
		reg_scramble_byp_en(1, reg_base, p_reg_tbl);
		//reg_autod_lut_md_7b(0, reg_base, p_reg_tbl);
		//reg_autod_trigger_md_7b(0x0, reg_base, p_reg_tbl);
	}

	return 0;
}


int dovi_get_be_dm_setting(struct dv_hw_reg *p_dv_reg)
{
	uint32_t reg_base = 0;
	struct dv_all_reg_tab *p_reg_tbl = NULL;
	struct dv_be_dm_reg *p_dv_dm_reg = NULL;

	if (p_dv_reg == NULL) {
		dovi_error("%s null params\n", __func__);
		return -1;
	}

	reg_base = VDO_BE_REG_BASE;
	p_reg_tbl = &dv_msys_all_reg;
	p_dv_dm_reg = &p_dv_reg->dv_be_dm;

	//be dm default internal bypass setting
	reg_b204_defalut(0x7e, reg_base, p_reg_tbl);
	reg_b320_defalut(0x00, reg_base, p_reg_tbl);
	reg_b3c8_defalut(0x1, reg_base, p_reg_tbl);

	/* B04 */
	reg_hdr_vdo_be_en(p_dv_dm_reg->b04.reg_hdr_vdo_be_en,
	reg_base, p_reg_tbl);

	/*B0401 */
	reg_b0401_byp_en(p_dv_dm_reg->b0401.reg_b0401_byp_en,
	reg_base, p_reg_tbl);
	reg_b0401_ipt_m0(p_dv_dm_reg->b0401.reg_b0401_ipt_m0,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_m1(p_dv_dm_reg->b0401.reg_b0401_ipt_m1,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_m2(p_dv_dm_reg->b0401.reg_b0401_ipt_m2,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_m3(p_dv_dm_reg->b0401.reg_b0401_ipt_m3,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_m4(p_dv_dm_reg->b0401.reg_b0401_ipt_m4,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_m5(p_dv_dm_reg->b0401.reg_b0401_ipt_m5,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_m6(p_dv_dm_reg->b0401.reg_b0401_ipt_m6,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_m7(p_dv_dm_reg->b0401.reg_b0401_ipt_m7,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_m8(p_dv_dm_reg->b0401.reg_b0401_ipt_m8,
		reg_base, p_reg_tbl);
	reg_b0401_ipt_shift(p_dv_dm_reg->b0401.reg_b0401_ipt_shift,
		reg_base, p_reg_tbl);

	/*B0402 */
	reg_b0402_byp_en(p_dv_dm_reg->b0402.reg_b0402_byp_en,
	reg_base, p_reg_tbl);

	/*B0403 */
	reg_b0403_byp_en(p_dv_dm_reg->b0403.reg_b0403_byp_en,
	reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m0(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m0,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m1(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m1,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m2(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m2,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m3(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m3,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m4(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m4,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m5(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m5,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m6(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m6,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m7(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m7,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_m8(p_dv_dm_reg->b0403.reg_b0403_csa2csb_m8,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_shift(p_dv_dm_reg->b0403.reg_b0403_csa2csb_shift,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_clp_max_0(
		p_dv_dm_reg->b0403.reg_b0403_csa2csb_clp_max,
		reg_base, p_reg_tbl);
	reg_b0403_csa2csb_clp_min_0(
		p_dv_dm_reg->b0403.reg_b0403_csa2csb_clp_min,
		reg_base, p_reg_tbl);

	/*B0404 */
	reg_b0404_byp_en(p_dv_dm_reg->b0404.reg_b0404_byp_en,
	reg_base, p_reg_tbl);
	reg_b0404_oetf_mode(p_dv_dm_reg->b0404.reg_b0404_oetf_mode,
		reg_base, p_reg_tbl);

	/*B0405 */
	reg_b0405_byp_en(p_dv_dm_reg->b0405.reg_b0405_byp_en,
	reg_base, p_reg_tbl);
	reg_b0405_rgb_m0(p_dv_dm_reg->b0405.reg_b0405_rgb_m0,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_m1(p_dv_dm_reg->b0405.reg_b0405_rgb_m1,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_m2(p_dv_dm_reg->b0405.reg_b0405_rgb_m2,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_m3(p_dv_dm_reg->b0405.reg_b0405_rgb_m3,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_m4(p_dv_dm_reg->b0405.reg_b0405_rgb_m4,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_m5(p_dv_dm_reg->b0405.reg_b0405_rgb_m5,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_m6(p_dv_dm_reg->b0405.reg_b0405_rgb_m6,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_m7(p_dv_dm_reg->b0405.reg_b0405_rgb_m7,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_m8(p_dv_dm_reg->b0405.reg_b0405_rgb_m8,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_offset_0(p_dv_dm_reg->b0405.reg_b0405_rgb_offset_0,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_offset_1(p_dv_dm_reg->b0405.reg_b0405_rgb_offset_1,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_offset_2(p_dv_dm_reg->b0405.reg_b0405_rgb_offset_2,
		reg_base, p_reg_tbl);
	reg_b0405_rgb_shift(p_dv_dm_reg->b0405.reg_b0405_rgb_shift,
		reg_base, p_reg_tbl);
	reg_range_min_7b(p_dv_dm_reg->b0405.reg_b0405_rgb_min,
		reg_base, p_reg_tbl);
	reg_range_inv_7b(p_dv_dm_reg->b0405.reg_b0405_rgb_inv,
		reg_base, p_reg_tbl);

	/*B0406 */
	reg_b0406_444to422_byp_en(p_dv_dm_reg->b0406.reg_b0406_byp_en,
	reg_base, p_reg_tbl);
	reg_b0406_444to422_cbcr_swap(
		p_dv_dm_reg->b0406.reg_b0406_444to422_cbcr_swap,
		reg_base, p_reg_tbl);
	reg_b0406_clp_sel(p_dv_dm_reg->b0406.reg_b0406_clp_sel,
		reg_base, p_reg_tbl);

	return 0;
}

int dovi_set_comp_bypass(uint8_t is_dovi, struct dv_hw_reg *p_hw_reg)
{
	if (dv_vdo_fe_en[0] && !((is_dovi & 1) || b_comp_enable)) {
		p_hw_reg->dv_ctrl[0].dm_src_sel = 1;
		p_hw_reg->dv_ctrl[0].dm_msb_align_en = 1;

		p_hw_reg->dv_dm[0].b0101.cup420_en = 0;
		p_hw_reg->dv_dm[0].b0101.repeat_en = 0;
		p_hw_reg->dv_dm[0].b0101._422to444_en = 0;

		p_hw_reg->dv_dm[0].b0102.reg_ycbcr_offset_0 =
			p_hw_reg->dv_dm[0].b0102.reg_ycbcr_offset_0 << 2;
		p_hw_reg->dv_dm[0].b0102.reg_ycbcr_offset_1 =
			p_hw_reg->dv_dm[0].b0102.reg_ycbcr_offset_1 << 2;
		p_hw_reg->dv_dm[0].b0102.reg_ycbcr_offset_2 =
			p_hw_reg->dv_dm[0].b0102.reg_ycbcr_offset_2 << 2;
		p_hw_reg->dv_dm[0].b0102.reg_range_clip =
			p_hw_reg->dv_dm[0].b0102.reg_range_clip << 2;

		p_hw_reg->dv_dm[0].b0102.reg_ycbcr_shift = 0;
	}

	if (dv_vdo_fe_en[1] && !((is_dovi & 2) || b_comp_enable)) {
		p_hw_reg->dv_ctrl[1].dm_src_sel = 1;
		p_hw_reg->dv_ctrl[1].dm_msb_align_en = 1;

		p_hw_reg->dv_dm[1].b0101.cup420_en = 0;
		p_hw_reg->dv_dm[1].b0101.repeat_en = 0;
		p_hw_reg->dv_dm[1].b0101._422to444_en = 0;

		p_hw_reg->dv_dm[1].b0102.reg_ycbcr_offset_0 =
			p_hw_reg->dv_dm[1].b0102.reg_ycbcr_offset_0 << 2;
		p_hw_reg->dv_dm[1].b0102.reg_ycbcr_offset_1 =
			p_hw_reg->dv_dm[1].b0102.reg_ycbcr_offset_1 << 2;
		p_hw_reg->dv_dm[1].b0102.reg_ycbcr_offset_2 =
			p_hw_reg->dv_dm[1].b0102.reg_ycbcr_offset_2 << 2;
		p_hw_reg->dv_dm[1].b0102.reg_range_clip =
			p_hw_reg->dv_dm[1].b0102.reg_range_clip << 2;

		p_hw_reg->dv_dm[1].b0102.reg_ycbcr_shift = 0;
	}

	return 0;
}
int dovi_get_output_setting(struct dv_hw_reg *p_hw_reg, uint8_t is_dovi_src)
{
	if (p_hw_reg == NULL) {
		dovi_printf("%s null params\n",  __func__);
		return -1;
	}

	dovi_flow("dv hw en (%d %d)(%d %d)(%d) is_dovi %d\n",
		dv_vdo_fe_en[0], dv_vdo_fe_en[1],
		dv_gfx_fe_en[0], dv_gfx_fe_en[1],
		dv_vdo_be_en, is_dovi_src);

	memset(&dv_dsys_all_reg, 0, sizeof(struct dv_all_reg_tab));
	memset(&dv_msys_all_reg, 0, sizeof(struct dv_all_reg_tab));

	dovi_set_comp_bypass(is_dovi_src, p_hw_reg);

	if (dv_vdo_fe_en[0]) {
		/* get composer hw output */
		dovi_get_cp0_setting(0, p_hw_reg);
		dovi_get_cp1_setting(0, p_hw_reg);
		/* get control path hw output */
		dovi_get_ctrl_setting(0, p_hw_reg);
		/* get dm hw output*/
		dovi_get_vdo_dm_setting(0, p_hw_reg);
		/* get vdo fe lut setting*/
		dovi_get_vdo_lut_setting(0, p_hw_reg);
		cur_ml_cfg_st[0] = 1;
	} else
		cur_ml_cfg_st[0] = 0;

	//temp use vdofe[0] setting because of idk limit
	if (dv_vdo_fe_en[1]) {
		/* get composer hw output */
		dovi_get_cp0_setting(1, p_hw_reg);
		dovi_get_cp1_setting(1, p_hw_reg);
		/* get control path hw output */
		dovi_get_ctrl_setting(1, p_hw_reg);
		/* get dm hw output*/
		dovi_get_vdo_dm_setting(1, p_hw_reg);
		/* get vdo fe lut setting*/
		dovi_get_vdo_lut_setting(1, p_hw_reg);
		cur_ml_cfg_st[1] = 1;
	} else
		cur_ml_cfg_st[1] = 0;

	if (dv_gfx_fe_en[0]) {
		/* get gop hw output*/
		dovi_get_gop_setting(0, p_hw_reg);
		/*get gfxlut setting*/
		dovi_get_gfx_luts_setting(0, p_hw_reg);
		cur_ml_cfg_st[2] = 1;
	} else
		cur_ml_cfg_st[2] = 0;

	if (dv_gfx_fe_en[1]) {
		/* get gop hw output*/
		dovi_get_gop_setting(1, p_hw_reg);

		/*get gfxlut setting*/
		dovi_get_gfx_luts_setting(1, p_hw_reg);
		cur_ml_cfg_st[3] = 1;
	} else
		cur_ml_cfg_st[3] = 0;

	if (dv_vdo_be_en) {
		/*get be dm hw output*/
		dovi_get_be_dm_setting(p_hw_reg);
		/* get dither hw output */
		dovi_get_dith_setting(p_hw_reg);
		/* get reorder hw output */
		dovi_get_rord_setting(p_hw_reg);
		/* get scramble hw output */
		dovi_get_scm_setting(p_hw_reg);
	}
	return 0;
}

