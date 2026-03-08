/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef ___PHY_MTK_TPHY_DEBUG_H___
#define ___PHY_MTK_TPHY_DEBUG_H___
#include <linux/kconfig.h>
#include <linux/kernel.h>

#if IS_ENABLED(CONFIG_DEBUG_FS)
int  tphy_phy_init_debugfs(void *handle);
void tphy_phy_exit_debugfs(void *handle);
#else
static inline int tphy_phy_init_debugfs(void *handle)
{
	return 0;
}
static inline void tphy_phy_exit_debugfs(void *handle)
{
}
#endif

#endif /* ___PHY_MTK_TPHY_DEBUG_H___ */
