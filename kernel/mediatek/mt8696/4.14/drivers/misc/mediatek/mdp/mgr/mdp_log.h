/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

