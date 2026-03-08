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


#ifndef _MTK_TC_API_H_
#define _MTK_TC_API_H_

extern void mtktc_config_all_tc_hw_protect(
	int temperature, int temperature2);
extern void set_taklking_flag(bool flag);

extern int get_sensor_tc_temp(int id);
extern int get_all_tc_max_temp(void);
extern int get_immediate_soc_wrap(void);
extern int get_immediate_cpuL_wrap(void);
extern int get_immediate_gpu_wrap(void);
extern int get_immediate_cpuB_wrap(void);
extern int get_immediate_abb_temp_wrap(void);
extern int get_immediate_ts1_wrap(void);
extern int get_immediate_ts2_wrap(void);
extern int get_immediate_ts3_wrap(void);
extern int get_immediate_ts4_wrap(void);
extern int get_immediate_ts5_wrap(void);
extern int get_immediate_ts6_wrap(void);
extern int get_immediate_ts7_wrap(void);


#endif
