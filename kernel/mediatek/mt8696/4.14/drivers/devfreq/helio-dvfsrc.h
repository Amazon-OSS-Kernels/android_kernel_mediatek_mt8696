/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __HELIO_DVFSRC_H
#define __HELIO_DVFSRC_H

#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/io.h>
#include <linux/pm_qos.h>
#include "helio-dvfsrc-opp.h"

struct helio_dvfsrc {
	struct devfreq		*devfreq;
	struct device *dev;
	bool dvfsrc_enabled;
	int type;

	void __iomem		*regs;
	struct regmap		*spm_map;

	struct notifier_block	pm_qos_ddr_opp_nb;
	struct notifier_block	pm_qos_vcore_opp_nb;
	struct notifier_block	pm_qos_vcore_dvfs_force_opp_nb;

	bool opp_forced;
	int (*suspend)(struct helio_dvfsrc *dvfsrc_dev);
	int (*resume)(struct helio_dvfsrc *dvfsrc_dev);
};

struct opp_profile {
	int vcore_uv;
	int ddr_Mbps;
};

#define DVFSRC_TIMEOUT		1000
#define SPM_DVFS_LEVEL                 (0x4A4)
#define SPM_DVFS_MISC                  (0x4AC)
#define SPM_DVS_DFS_LEVEL              (0x4F8)
#define SPM_FORCE_DVFS                 (0x4FC)
#define RC_SW_LEVEL0                   (0x510)
#define RC_INFO                        (0x514)
#define RC_RECORD_DVS                  (0x518)
#define RC_RECORD_DFS                  (0x51C)
#define RC_TARGET_LEVEL                (0x520)
#define RC_SW_LEVEL1                   (0x524)
#define RC_SW_LEVEL2                   (0x528)

#define RC_EMI_BW_LSB                  (0x3F << 0)     /* 6b */
#define RC_DFS_LSB                     (0x3 << 6)      /* 2b */
#define RC_DVS_LSB                     (0x3 << 8)      /* 2b*/
#define RC_REQ_LSB                     (1U << 10)      /* 1b */
#define SPM_DVFS_TYPE_SHIFT            0
#define SPM_DVFS_FORCE_ENABLE_SHIFT    2
#define SPM_DVFSRC_ENABLE_SHIFT        4
#define RC_EMI_BW_SHIFT                0
#define RC_DFS_SHIFT                   6
#define RC_DVS_SHIFT                   8
#define RC_REQ_SHIFT                   10

#define dvfsrc_wait_for_completion(condition, timeout)			\
({								\
	int ret = 0;						\
	if (is_dvfsrc_enabled())				\
		ret = 1;					\
	while (!(condition) && ret > 0) {			\
		if (ret++ >= timeout) {				\
			pr_info("timeout %x\n", spm_get_dvfs_level());\
			ret = -EBUSY;				\
		}						\
		udelay(1);					\
	}							\
	ret;							\
})

extern int is_dvfsrc_enabled(void);
extern void helio_dvfsrc_enable(int dvfsrc_en);
extern char *dvfsrc_dump_reg(char *ptr);
extern char *dvfsrc_dump_opp_table(char *ptr);

extern int helio_dvfsrc_add_interface(struct device *dev);
extern void helio_dvfsrc_remove_interface(struct device *dev);

#endif /* __HELIO_DVFSRC_H */
