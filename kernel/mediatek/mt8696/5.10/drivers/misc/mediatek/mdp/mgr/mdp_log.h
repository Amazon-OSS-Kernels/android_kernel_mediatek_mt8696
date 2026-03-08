/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef __MDP_LOG_H__
#define __MDP_LOG_H__

#include <linux/types.h>
#include <linux/printk.h>
#include "mdp_proc_print.h"

#define LOG_TAG "MDP"

#include "disp_hw_log.h"

bool mdp_core_should_print_log(void);

#define MDP_ERR(string, args...) \
	pr_notice("MDP_ERR: "string, ##args)

#define MDP_LOG(string, args...) do { \
	if (mdp_core_should_print_log()) { \
		pr_notice("MDP_LOG: "string, ##args); \
	} \
} while (0)

#endif /* endof __MDP_LOG_H__ */

