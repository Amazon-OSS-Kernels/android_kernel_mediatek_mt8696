/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __APUSYS_MNOC_DBG_H__
#define __APUSYS_MNOC_DBG_H__

#include <aee.h>

#ifdef APU_AEE_ENABLE
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#define mnoc_aee_warn(key, format, args...) \
	do { \
		pr_info(format, ##args); \
		aee_kernel_warning("MNOC", \
			"\nCRDISPATCH_KEY:" key "\n" format, ##args); \
	} while (0)
#endif
#else
#define mnoc_aee_warn(key, format, args...)
#endif

int create_debugfs(struct dentry *root);
void remove_debugfs(void);

#endif
