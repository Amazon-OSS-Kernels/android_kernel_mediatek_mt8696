/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef __MTK_TS_CPU_H__
#define __MTK_TS_CPU_H__

#ifdef CONFIG_ROGUE_GE9215
#ifndef CONFIG_MTK_GPU_SUPPORT
#define CONFIG_MTK_GPU_SUPPORT
#endif
#endif

#ifdef CONFIG_MTK_GPU_SUPPORT
#include "mtk_gpufreq.h"
#endif


extern int bts_cur_temp;
extern u32 get_devinfo_with_index(u32 index);

extern int get_immediate_temp2_wrap(void);
extern void mtkts_dump_cali_info(void);

int mtktsbattery_register_thermal(void);
void mtktsbattery_unregister_thermal(void);

int tsallts_register_thermal(void);
void tsallts_unregister_thermal(void);


extern int IMM_IsAdcInitReady(void);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);


/*mtk_ts_pa_thput.c*/
extern struct proc_dir_entry *mtk_thermal_get_proc_drv_therm_dir_entry(void);
extern bool is_meta_mode(void);
extern bool is_advanced_meta_mode(void);

/* mtk_ts_pmic6323.c */
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
int mtktspmic_register_thermal(void);
void mtktspmic_unregister_thermal(void);

/* mtk_thermal_platform.c */
extern int mtktscpu_limited_dmips;
#if defined(CONFIG_MTK_SMART_BATTERY)
/* global variable from battery driver... */
extern bool gFG_Is_Charging;
#endif
extern unsigned int mt_gpufreq_get_cur_freq(void);
void thermal_buffer_turn_on(void);
void thermal_buffer_turn_off(void);

struct proc_dir_entry *mtk_thermal_get_proc_drv_therm_dir_entry(void);

/* 1: turn on adaptive AP cooler; 0: turn off */

#define CPT_ADAPTIVE_AP_COOLER (0)

#if CPT_ADAPTIVE_AP_COOLER
extern char *adaptive_cooler_name;
extern int TARGET_TJS[];
#endif

#ifdef CONFIG_MTK_GPU_SUPPORT
extern int Num_of_GPU_OPP;
/* #define GPU_Default_POWER     456 */
extern struct mtk_gpu_power_info *mtk_gpu_power;
#endif

extern int mtktscpu_debug_log;
extern int Num_of_GPU_OPP;
extern int final_cpu_limit;
extern int final_gpu_limit;
extern unsigned int adaptive_cpu_power_limit;
extern unsigned int static_cpu_power_limit;

#ifdef CONFIG_MTK_GPU_SUPPORT
extern unsigned int adaptive_gpu_power_limit;
extern unsigned int static_gpu_power_limit;
#endif

typedef void (*met_thermalsampler_funcMET)(void);


#endif				/* __MTK_TS_CPU_H__ */
