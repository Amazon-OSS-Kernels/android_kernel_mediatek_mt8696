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

#ifndef __DISP_CFD_DEBUG_H__
#define __DISP_CFD_DEBUG_H__

#include "disp_info.h"
#include <linux/kernel.h>
#include <linux/printk.h>

#define CFD_RET_OK               0
#define CFD_RET_ERROR            1
#define CFD_RET_INV_ARG          2
#define CFD_RET_UNINIT           3
#define CFD_RET_INV_CMD          4
#define CFD_RET_UPDATE_FAIL      5
#define CFD_RET_READ_ERR         6

extern unsigned int cfd_dbg_level;
extern unsigned int cfd_sof_start;
extern unsigned int cfd_sof_end;

#define CFD_ERROR_LOG (1 << 0)
#define CFD_FUNC_LOG (2)
#define CFD_INFO_LOG (1 << 2)
#define CFD_METADATA_LOG (1 << 3)
#define CFD_VSVDB_LOG (1 << 4)
#define CFD_HDMI_LOG (1 << 5)
#define CFD_OUTPUT_LOG (1 << 6)


#define cfd_printf(fmt, args...) pr_info("[cfd] "fmt, ##args)

#define cfd_error(format, ...) do { \
	if (1) { \
		cfd_printf("error : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define cfd_func_start() do { \
	if (cfd_dbg_level & (CFD_FUNC_LOG)) { \
		cfd_printf("func| %s start\n", __func__); \
	} \
} while (0)

#define cfd_func_end() do { \
	if (cfd_dbg_level & (CFD_FUNC_LOG)) { \
		cfd_printf("func| %s end\n", __func__); \
	} \
} while (0)

#define cfd_func() do { \
	if (cfd_dbg_level & (CFD_FUNC_LOG)) { \
		cfd_printf("func| %s\n", __func__); \
	} \
} while (0)

#define cfd_func_default() do { \
	if (CFD_FUNC_LOG) { \
		cfd_printf("func| %s\n", __func__); \
	} \
} while (0)

#define cfd_info(format, ...) do { \
	if (cfd_dbg_level & (CFD_INFO_LOG)) { \
		cfd_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define cfd_md_info(format, ...) do { \
	if (cfd_dbg_level & (CFD_METADATA_LOG)) { \
		cfd_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define cfd_vsvdb_info(format, ...) do { \
	if (cfd_dbg_level & (CFD_VSVDB_LOG)) { \
		cfd_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define cfd_hdmi_info(format, ...) do { \
	if (cfd_dbg_level & (CFD_HDMI_LOG)) { \
		cfd_printf("info : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define cfd_output_info(format, ...) do { \
	if (cfd_dbg_level & (CFD_OUTPUT_LOG)) { \
		cfd_printf("output : "format, ##__VA_ARGS__); \
	} \
} while (0)

#define	cfd_default(format, ...) \
{ \
	cfd_printf(""format, ##__VA_ARGS__); \
}

#define printf(format, ...) cfd_printf(format, ##__VA_ARGS__)

#define assert(value) \
{ \
	if ((value) == 0) { \
		cfd_error("%s,%d value %d\n", __func__, __LINE__, value); \
	} \
	ASSERT(value); \
}

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
			cfd_error("[ERROR]return error: %d\n" \
				"  file : %s, line : %d\n",		\
				ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif

void cfd_debug_init(void);
void cfd_debug_deinit(void);


#endif /* __DISP_cfd_DEBUG_H__ */
