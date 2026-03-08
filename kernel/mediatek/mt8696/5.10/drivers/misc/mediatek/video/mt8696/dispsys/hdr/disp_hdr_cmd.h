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

#ifndef __DISP_HDR_CMD_H__
#define __DISP_HDR_CMD_H__

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
			hdr_error("[ERROR]return error: %d\n" \
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
			hdr_error("[ERROR]return error: %d\n" \
				"  file : %s, line : %d\n",		\
				ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif

extern unsigned int hdr_dbg_level;
extern uint32_t time_check;
extern uint32_t adl_mode;
extern uint32_t hdr_sof_start;
extern uint32_t hdr_sof_end;
extern uint32_t delay_hdr_num;
extern uint32_t delay_hdr_mute_num;
extern uint32_t force_sdr_output;
extern bool hdr_allm_ctl_by_cmd;
extern uint32_t hdr_allm_type;
extern uint32_t use_dv_s_type;
extern uint32_t osd_force_allm;

#define HDR_INFO_LOG (1 << 0)
#define HDR_OSD_INFO_LOG (1 << 1)
#define HDR_VIDEO_INFO_LOG (1 << 2)

#define hdr_printf(fmt, args...) pr_info("[hdr] "fmt, ##args)

#define hdr_error(format, ...) \
	hdr_printf("error : "format, ##__VA_ARGS__)


#define hdr_info(format, ...) do { \
	if (hdr_dbg_level & (HDR_INFO_LOG)) { \
		hdr_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define hdr_osd_info(format, ...) do { \
		if (hdr_dbg_level & (HDR_OSD_INFO_LOG)) { \
			hdr_printf("info : "format, ##__VA_ARGS__); \
		} \
} while (0)

#define hdr_video_info(format, ...) do { \
		if (hdr_dbg_level & (HDR_VIDEO_INFO_LOG)) { \
			hdr_printf("info : "format, ##__VA_ARGS__); \
		} \
} while (0)
void hdr_debug_init(void);

#endif
