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

#ifndef __DISP_DOVI_CMD_H__
#define __DISP_DOVI_CMD_H__

extern bool dump_rpu_enable;
extern uint32_t dump_md_enable;
extern uint32_t reg_test_enable;
extern uint32_t dovi_idk_test;
extern uint32_t dovi_idk_file_id;
extern uint32_t dovi_sdk_test;
extern uint32_t dovi_sdk_file_id;
extern bool dovi_mute;
extern bool fhd_scale_to_uhd;
extern enum VIDEO_BIT_MODE idk_dump_bpp;
extern bool dovi_idk_dump;
extern bool dovi_force_output;
extern enum dovi_signal_format_t dovi_force_out_format;
extern uint32_t dovi_idk_disp_cnt;
extern unsigned int g_force_dovi;
extern unsigned int g_force_open_hdr;
extern unsigned int g_dovi_efuse;
extern unsigned int g_hdr_type;
extern unsigned int g_out_format;
extern bool b_comp_enable;
extern uint32_t _subv_type;
extern int no_mix_fhd;
extern int no_mix_uhd;
extern uint32_t f_graphic_off_cmd;
extern uint32_t idk_vsem;
extern uint32_t sdk_vsem;
extern uint32_t set_vsvdb;
extern uint32_t dovi_hdmi_brightness_en;
extern uint32_t dovi_hdmi_brightness;
extern uint32_t dovi_get_ext_md;

void dovi_debug_init(void);

int disp_dovi_status(void);
extern int disp_dovi_common_test(uint32_t option);
extern int dovi_unit_process_dbg_opt(const char *opt);
extern uint32_t disp_dovi_set_idk_info(void);
extern void disp_dovi_set_tz_test_info(uint32_t value1);
extern uint32_t disp_dovi_set_sdk_info(void);
extern void disp_dovi_set_hdr_enable(uint32_t enable, uint32_t outformat);

#ifndef STR_CVT
#define STR_CVT(p, val, base, action)\
	do {			\
		int ret = 0;	\
		const char *tmp;	\
		const char A[8] = "int32_t"; \
		const char B[9] = "uint32_t"; \
		const char C[9] = "uint64_t"; \
		tmp = strsep(p, ","); \
		if (tmp == NULL) \
			break; \
		if (strcmp(#base, A) == 0)\
			ret = kstrtoint(tmp, 0, (int *)val); \
		else if (strcmp(#base, B) == 0)\
			ret = kstrtouint(tmp, 0, (unsigned int *)val); \
		else if (strcmp(#base, C) == 0)\
			ret = kstrtoul(tmp, 0, (unsigned long *)val); \
		if (ret != 0) {\
			dovi_error("[ERROR]return error: %d\n" \
				"  file : %s, line : %d\n",		\
				ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif

#ifndef STR_CVT_U32
#define STR_CVT_U32(p, val, action)\
	do {			\
		int ret = 0;	\
		const char *tmp;	\
		tmp = strsep(p, ","); \
		if (tmp == NULL) \
			break; \
		ret = kstrtouint(tmp, 0, (unsigned int *)val); \
		if (ret != 0) {\
			dovi_error("[ERROR]return error: %d\n" \
				"  file : %s, line : %d\n",		\
				ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif


#endif
