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

#ifndef __MDP_CLI_H__
#define __MDP_CLI_H__

struct mdp_cli_setting_struct {
	bool dump_imgresz_setting;
	bool enable_log;
	bool enable_mmp_debug;
	bool print_record;
	bool print_message;
};

void mdp_cli_init(void);
struct mdp_cli_setting_struct *mdp_cli_get(void);

#endif /* endof __MDP_CLI_H__ */
