/*
 * Copyright (C) 2015 MediaTek Inc.
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


#ifndef _MTK_THERMAL_H
#define _MTK_THERMAL_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <linux/io.h>
#include <linux/uaccess.h>

#include "sync_write.h"


enum thermal_sensor_name {
	THERMAL_SENSOR1     = 0,/*TS_MCU1*/
	THERMAL_SENSOR2     = 1,/*TS_MCU2*/
/*	THERMAL_SENSOR3     = 2,*/

	THERMAL_SENSOR_NUM
};

enum thermal_bank_name {
	THERMAL_BANK0     = 0, /*CPU (TS_MCU1) (TS1)*/
	THERMAL_BANK1     = 1, /*MFG (TS_MCU2) (TS2)*/
	THERMAL_BANK2     = 2, /*GPU (TS_MCU3) (TS3)*/
	THERMAL_BANK_NUM
};

struct TS_SVS {
	unsigned int ts_MTS;
	unsigned int ts_BTS;
};

struct mtk_gpu_power_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_power;
};

/* ptp driver need this function */
extern void get_thermal_slope_intercept(struct TS_SVS *ts_info,
	enum thermal_bank_name ts_bank);

/* mtk_thermal_platform.c need this */
extern void set_taklking_flag(bool flag);

#define THERMAL_WRAP_WR32(val, addr)  mt_reg_sync_writel((val), ((void *)addr))

#if 1
/*4 thermal sensors*/
enum MTK_THERMAL_SENSOR_CPU_ID_MET {
	MTK_THERMAL_SENSOR_TS1 = 0,
	MTK_THERMAL_SENSOR_TS2,
	MTK_THERMAL_SENSOR_TS3,
	MTK_THERMAL_SENSOR_TS4,
	MTK_THERMAL_SENSOR_TS5,
	MTK_THERMAL_SENSOR_TS6,
	MTK_THERMAL_SENSOR_TSABB,

	ATM_CPU_LIMIT,
	ATM_GPU_LIMIT,

	MTK_THERMAL_SENSOR_CPU_COUNT
};
#else
/*5 thermal sensors*/
enum MTK_THERMAL_SENSOR_CPU_ID_MET {
	MTK_THERMAL_SENSOR_TS1 = 0,
	MTK_THERMAL_SENSOR_TS2,
	MTK_THERMAL_SENSOR_TS3,
	MTK_THERMAL_SENSOR_TS4,
	MTK_THERMAL_SENSOR_TSABB,

	ATM_CPU_LIMIT,
	ATM_GPU_LIMIT,

	MTK_THERMAL_SENSOR_CPU_COUNT
};
#endif

extern int tscpu_get_cpu_temp_met(enum MTK_THERMAL_SENSOR_CPU_ID_MET id);
extern int mtk_gpufreq_register(struct mtk_gpu_power_info *freqs, int num);


typedef void (*met_thermalsampler_funcMET)(void);
void mt_thermalsampler_registerCB(met_thermalsampler_funcMET pCB);

void tscpu_start_thermal(void);
void tscpu_stop_thermal(void);
void tscpu_cancel_thermal_timer(void);
void tscpu_start_thermal_timer(void);
int mtkts_bts_get_hw_temp(void);

extern int get_immediate_ts1_wrap(void);
extern int get_immediate_ts2_wrap(void);
extern int get_immediate_ts3_wrap(void);

extern int is_cpu_power_unlimit(void);	/* in mtk_ts_cpu.c */
extern int is_cpu_power_min(void);	/* in mtk_ts_cpu.c */
extern int get_cpu_target_tj(void);
extern int get_cpu_target_offset(void);

extern int mtktscpu_debug_log;
extern int hpt_set_cpu_num_limit(
		unsigned int little_cpu, unsigned int big_cpu);

#ifdef CONFIG_MTK_GPU_SUPPORT
extern int _get_current_gpu_power(void);
#endif

extern int _get_current_cpu_power(void);

#endif

