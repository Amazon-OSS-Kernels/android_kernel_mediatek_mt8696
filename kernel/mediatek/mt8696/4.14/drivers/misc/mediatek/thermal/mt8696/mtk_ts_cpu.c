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

#include "mt-plat/mtk_thermal_monitor.h"
#include <linux/acpi.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/thermal.h>
#include <linux/types.h>
#include <linux/version.h>
#include <mt-plat/aee.h>
#include <mt-plat/sync_write.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/uidgid.h>
#include <linux/thermal_framework.h>

#include "mtk_power_throttle.h"
#include "inc/mtk_thermal_timer.h"

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif
#define __MT_MTK_TS_CPU_C__


/* 1: turn on RT kthread for thermal protection in this sw module;
 * 0: turn off
 */
#define MTK_TS_CPU_RT (0)
#if MTK_TS_CPU_RT
#include <linux/kthread.h>
#include <linux/sched.h>
#endif

#include "inc/mtk_ts_cpu.h"
#include "mtk_tc_api.h"
#include "mach/mtk_cpufreq_api.h"
#include "mtk_thermal_typedefs.h"
#include "thermal_core.h"

#define THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET (0)

/* 1: turn on GPIO toggle monitor; 0: turn off */
#define THERMAL_GPIO_OUT_TOGGLE (0)

/* 1: turn on SW filtering in this sw module; 0: turn off */
#define MTK_TS_CPU_SW_FILTER (1)


#if CPT_ADAPTIVE_AP_COOLER
#define MAX_CPT_ADAPTIVE_COOLERS (3)
#endif
int mtktscpu_limited_dmips;

static kuid_t uid = KUIDT_INIT(0);
static kgid_t gid = KGIDT_INIT(1000);
static int temperature_switch;


/**
 * If curr_temp >= polling_trip_temp1, use interval
 * else if cur_temp >= polling_trip_temp2
 * && curr_temp < polling_trip_temp1, use
 * interval*polling_factor1
 * else, use interval*polling_factor2
 */

int final_cpu_limit = 0x7FFFFFFF;
int final_gpu_limit = 0x7FFFFFFF;
int fast_polling_trip_temp = 80000;
int fast_polling_factor = 5;
int polling_trip_temp0 = 60000;
int polling_trip_temp1 = 40000;
int polling_trip_temp2 = 20000;
int polling_factor0 = 2;
int polling_factor1 = 3;
int polling_factor2 = 5;

int tc_mid_trip = -275000;
int tc_high_trip = 117000;
static int g_max_temp = 50000; /* default=50 deg */
static unsigned int interval = 1000; /* mseconds, 0 : no auto polling */
/* trip_temp[0] must be initialized to the thermal HW protection point. */
static int trip_temp[10] = {117000, 90000, 85000, 75000, 65000,
			    55000,  45000,  35000, 25000, 15000};
static unsigned int cl_dev_sysrst_state;
static struct thermal_zone_device *thz_dev;
static struct thermal_cooling_device *cl_dev_sysrst;

int mtktscpu_debug_log;
//static int kernelmode;
static int g_THERMAL_TRIP[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int num_trip = 5;
//static int MA_len_temp;
#define CPU_COOLER_NUM 34

static DEFINE_MUTEX(TS_lock);

static char g_bind0[20] = "";
static char g_bind1[20] = "";
static char g_bind2[20] = "";
static char g_bind3[20] = "";
static char g_bind4[20] = "";
static char g_bind5[20] = "";
static char g_bind6[20] = "";
static char g_bind7[20] = "";
static char g_bind8[20] = "";
static char g_bind9[20] = "";

#define MTKTSCPU_TEMP_CRIT 120000 /* 120.000 degree Celsius */

struct mt_cpu_power_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_ncpu;
	unsigned int cpufreq_power;
};

static struct mt_cpu_power_info *mtk_cpu_power;
static int tscpu_num_opp;

#define THERMAL_NAME    "mt8696-thermal"

#ifdef CONFIG_MTK_GPU_SUPPORT
int Num_of_GPU_OPP;
/* #define GPU_Default_POWER     456 */
struct mtk_gpu_power_info *mtk_gpu_power;
#endif

static int tscpu_cpu_dmips[CPU_COOLER_NUM] = {0};

#define tscpu_dprintk(fmt, args...)                      \
	do {                                                 \
		if (mtktscpu_debug_log & 0x4) {                  \
			pr_info("Power/CPU_Thermal" fmt, ##args);    \
		}                                                \
	} while (0)

#define tscpu_printk(fmt, args...)                       \
	do {                                                 \
		if (mtktscpu_debug_log & 0x2) {                  \
			pr_info("Power/CPU_Thermal" fmt, ##args);    \
		}                                                \
	} while (0)

//static int tscpu_register_thermal(void);
//static void tscpu_unregister_thermal(void);
#if THERMAL_GPIO_OUT_TOGGLE
static void tscpu_set_GPIO_toggle_for_monitor(void);
#endif

#ifdef CONFIG_MTK_GPU_SUPPORT
bool __attribute__((weak))
	mtk_get_gpu_loading(unsigned int *pLoading)
{
	pr_info("######E_WF: %s doesn't exist!!\n", __func__);
	return 0;
}
void __attribute__((weak))
	mt_gpufreq_thermal_protect(unsigned int limited_power)
{
	pr_info("######E_WF: %s doesn't exist!!\n", __func__);
}

unsigned int __attribute__((weak))
	mt_gpufreq_get_cur_freq(void)
{
	pr_info("######E_WF: %s doesn't exist!!\n", __func__);
	return 0;
}
#endif

void __attribute__((weak))
	mt_cpufreq_thermal_protect(unsigned int limited_power)
{
	pr_info("######E_WF: %s doesn't exist!!\n", __func__);
}

struct tscpu_thermal_zone {
	struct thermal_zone_device *tscpu_tzd;
};

static struct tscpu_thermal_zone *g_tscpu_tz;
#if THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET

enum mtk_thermal_sensor_cpu_id_met {
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

static met_thermalsampler_funcMET g_pThermalSampler;
static DEFINE_MUTEX(MET_GET_TEMP_LOCK);

void mt_thermalsampler_registerCB(met_thermalsampler_funcMET pCB)
{
	g_pThermalSampler = pCB;
}
EXPORT_SYMBOL(mt_thermalsampler_registerCB);

static DEFINE_SPINLOCK(tscpu_met_spinlock);

void tscpu_met_lock(unsigned long *flags)
{
	spin_lock_irqsave(&tscpu_met_spinlock, *flags);
}
EXPORT_SYMBOL(tscpu_met_lock);

void tscpu_met_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&tscpu_met_spinlock, *flags);
}
EXPORT_SYMBOL(tscpu_met_unlock);


int tscpu_get_cpu_temp_met(enum mtk_thermal_sensor_cpu_id_met id)
{
	unsigned long flags;
	int ret;

	if (id < 0 || id >= MTK_THERMAL_SENSOR_CPU_COUNT) {
		pr_info("%s: met_id is %d\n", __func__, id);
		return -127000;
	}

	if (id == ATM_CPU_LIMIT)
		return (adaptive_cpu_power_limit != 0x7FFFFFFF) ?
						adaptive_cpu_power_limit : 0;

#ifdef CONFIG_MTK_GPU_SUPPORT
	if (id == ATM_GPU_LIMIT)
		return (adaptive_gpu_power_limit != 0x7FFFFFFF) ?
						adaptive_gpu_power_limit : 0;
#endif

#ifndef CONFIG_MTK_GPU_SUPPORT
	if (id == ATM_GPU_LIMIT)
		return -127000;
#endif

	tscpu_met_lock(&flags);
	ret = get_sensor_tc_temp(id);
	tscpu_met_unlock(&flags);
	return ret;
}
EXPORT_SYMBOL(tscpu_get_cpu_temp_met);

#endif

int mtk_cpufreq_register(struct mt_cpu_power_info *freqs, int num)
{
	int i = 0;
#ifdef CONFIG_MTK_GPU_SUPPORT
	int gpu_power = 0;
#endif

	tscpu_dprintk("%s\n", __func__);
	tscpu_num_opp = num;

	mtk_cpu_power =
		kzalloc((num) * sizeof(struct mt_cpu_power_info), GFP_KERNEL);
	if (mtk_cpu_power == NULL)
		return -ENOMEM;

#ifdef CONFIG_MTK_GPU_SUPPORT
	if (0 != Num_of_GPU_OPP && NULL != mtk_gpu_power)
		gpu_power = mtk_gpu_power[Num_of_GPU_OPP - 1].gpufreq_power;
	else
		tscpu_printk("Num_of_GPU_OPP is 0!\n");
#endif

	for (i = 0; i < num; i++) {

#if 0
		int dmips = freqs[i].cpufreq_khz * freqs[i].cpufreq_ncpu / 1000;
		/* TODO: this line must be modified every time cooler mapping
		 * table changes
		 */

		int cl_id =
			(((freqs[i].cpufreq_power + gpu_power) + 99) / 100) - 7;

#endif

		mtk_cpu_power[i].cpufreq_khz = freqs[i].cpufreq_khz;
		mtk_cpu_power[i].cpufreq_ncpu = freqs[i].cpufreq_ncpu;
		mtk_cpu_power[i].cpufreq_power = freqs[i].cpufreq_power;

#if 0
		if (cl_id < CPU_COOLER_NUM) {
			if (tscpu_cpu_dmips[cl_id] < dmips)
				tscpu_cpu_dmips[cl_id] = dmips;
		}
#endif


	}

	{
		int base = (mtk_cpu_power[num - 1].cpufreq_khz *
			    mtk_cpu_power[num - 1].cpufreq_ncpu) /
			   1000;
		for (i = 0; i < CPU_COOLER_NUM; i++) {
			if (tscpu_cpu_dmips[i] == 0 ||
			    tscpu_cpu_dmips[i] < base)
				tscpu_cpu_dmips[i] = base;
			else
				base = tscpu_cpu_dmips[i];
		}
		mtktscpu_limited_dmips = base;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_cpufreq_register);

/* Init local structure for AP coolers */


#if 0
int mtk_gpufreq_register(struct mtk_gpu_power_info *freqs, int num)
{
	int i = 0;

	tscpu_dprintk("%s\n", __func__);
	mtk_gpu_power =
		kzalloc((num) * sizeof(struct mtk_gpu_power_info), GFP_KERNEL);
	if (mtk_gpu_power == NULL)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		mtk_gpu_power[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mtk_gpu_power[i].gpufreq_power = freqs[i].gpufreq_power;

		pr_info("[%d].gpufreq_khz=%u, .gpufreq_power=%u\n", i,
			freqs[i].gpufreq_khz, freqs[i].gpufreq_power);
	}

	Num_of_GPU_OPP = num; /* GPU OPP count */
	return 0;
}
EXPORT_SYMBOL(mtk_gpufreq_register);
#endif


#ifdef CONFIG_MTK_GPU_SUPPORT

int _get_current_gpu_power(void)
{
	unsigned int cur_gpu_freq = mt_gpufreq_get_cur_freq();
	unsigned int cur_gpu_power = 0;
	int i = 0;

	if (mtk_gpu_power != NULL) {
		for (; i < Num_of_GPU_OPP; i++)
			if (mtk_gpu_power[i].gpufreq_khz == cur_gpu_freq)
				cur_gpu_power = mtk_gpu_power[i].gpufreq_power;
	}

	return (int) cur_gpu_power;
}

#endif


#define CLUSTER_NUM  (1)

int _get_current_cpu_power(void)
{
	unsigned int cur_cpu_freq = 0;
	unsigned int cur_cpu_num = 0;
	int i = 0, j = 0;
	unsigned int cur_cpu_power = 0;
	struct cpumask cluster_cpu, online_cpu;

	for (i = 0; i < CLUSTER_NUM; i++) {
		arch_get_cluster_cpus(&cluster_cpu, i);
		cpumask_and(&online_cpu, &cluster_cpu, cpu_online_mask);

		cur_cpu_num = cpumask_weight(&online_cpu);

		cur_cpu_freq = mt_cpufreq_get_cur_freq(i);
		for (j = 0; j < tscpu_num_opp; j++)
			if (mtk_cpu_power[j].cpufreq_ncpu == cur_cpu_num &&
				mtk_cpu_power[j].cpufreq_khz == cur_cpu_freq)
				cur_cpu_power += mtk_cpu_power[j].cpufreq_power;
	}

	return (int) cur_cpu_power;
}

#if 0
unsigned int mt_ppm_thermal_get_max_power(void)
{
	int i = 0;

	for (i = 0; i < CLUSTER_NUM; i++)
		cur_cpu_power += mtk_cpu_power[0].cpufreq_power;

}
#endif

static int tscpu_get_temp(struct thermal_zone_device *thermal, int *t)
{
#if THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET
	unsigned long flags;
#endif
#if MTK_TS_CPU_SW_FILTER == 1
	int ret = 0;
	int curr_temp;
	int temp_temp;
	static int last_cpu_real_temp;

	curr_temp = get_immediate_soc_wrap();
	tscpu_printk("\n %s: T=%d\n", __func__, curr_temp);

	if ((curr_temp > (trip_temp[0] - 15000)) || (curr_temp < -30000) ||
	    (curr_temp > 85000))
		tscpu_printk("CPU T=%d\n", curr_temp);

	temp_temp = curr_temp;
	/* not resumed from suspensio... */
	if (curr_temp != 0) {
		/* invalid range */
		if ((curr_temp > 150000) || (curr_temp < -20000)) {
			tscpu_dprintk("CPU temp invalid=%d\n", curr_temp);
			temp_temp = 50000;
			ret = -1;
		} else if (last_cpu_real_temp != 0) {
			/* delta 40C, invalid change */
			if ((curr_temp - last_cpu_real_temp > 40000) ||
			    (last_cpu_real_temp - curr_temp > 40000)) {
				tscpu_dprintk(
					"CPU temp float hugely temp=%d, lasttemp=%d\n",
					curr_temp, last_cpu_real_temp);
				/* tscpu_dprintk("RAW_TS2 = %d,RAW_TS3 =
				 * %d,RAW_TS4 = %d\n",RAW_TS2,RAW_TS3,RAW_TS4);
				 */
				temp_temp = 50000;
				ret = -1;
			}
		}
	}

	last_cpu_real_temp = curr_temp;
	curr_temp = temp_temp;
/* tscpu_dprintk("TS2 = %d,TS3 = %d,TS4 = %d\n", Temp_TS2,Temp_TS3,Temp_TS4); */
#else
	int ret = 0;
	int curr_temp;

	curr_temp = get_immediate_soc_wrap();

	tscpu_dprintk("%s CPU T1=%d\n", __func__, curr_temp);
	if ((curr_temp > (trip_temp[0] - 15000)) || (curr_temp < -30000))
		tscpu_dprintk("[Power/CPU_Thermal] CPU T=%d\n", curr_temp);
#endif

	*t = (unsigned long)curr_temp;
	if ((int)curr_temp < polling_trip_temp2) {
		thermal->polling_delay = interval * polling_factor2;
	} else if ((int)curr_temp < polling_trip_temp1) {
		thermal->polling_delay = interval * polling_factor1;
	} else if (curr_temp >= fast_polling_trip_temp) {
		/* it means next timeout will be in interval/fast_polling_factor
		 */
		thermal->polling_delay = interval / fast_polling_factor;
	} else if (curr_temp >= polling_trip_temp0) {
		thermal->polling_delay = interval / polling_factor0;
	} else {
		thermal->polling_delay = interval;
	}

#if 0
#if CPT_ADAPTIVE_AP_COOLER
	tscpu_prev_cpu_temp = tscpu_curr_cpu_temp;
	tscpu_curr_cpu_temp = curr_temp;
#endif
#endif

#if THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET
	tscpu_met_lock(&flags);
	//a_tscpu_all_temp[0] = SOC_TS_TEMP_T[TS_MCU1]; /* temp of TS_MCU1 */
	//a_tscpu_all_temp[1] = SOC_TS_TEMP_T[TS_MCU2]; /* temp of TS_MCU2 */
	//a_tscpu_all_temp[2] = SOC_TS_MCU3_T; /* temp of TS_MCU3 */
	tscpu_met_unlock(&flags);

	if (g_pThermalSampler != NULL)
		g_pThermalSampler();
#endif


#if THERMAL_GPIO_OUT_TOGGLE
	/*for output signal monitor */
	tscpu_set_GPIO_toggle_for_monitor();
#endif
	g_max_temp = curr_temp;
	tscpu_dprintk("%s, current temp =%d\n", __func__, curr_temp);

	return ret;
}

static int mtktscpu_of_get_temp(void *data, int *t)
{
       struct tscpu_thermal_zone *tscpu_tz;
       int ret = 0;

       if (!data) {
               pr_err("%s: data not valid\n", __func__);
               return -EINVAL;
       }

       tscpu_tz = (struct tscpu_thermal_zone *)data;

       if(!tscpu_tz->tscpu_tzd) {
               pr_err("%s: tscpu_tzd not valid\n", __func__);
               return -EINVAL;
       }
       ret = tscpu_get_temp(tscpu_tz->tscpu_tzd, t);
       if (ret)
               pr_err("%s: failed\n", __func__);

       return ret;
}

#if 0
static int tscpu_bind(struct thermal_zone_device *thermal,
		      struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind0)) {
		table_val = 0;
		mtktc_config_all_tc_hw_protect(tc_high_trip, tc_mid_trip);
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind1)) {
		table_val = 1;
		/* only when a valid cooler is tried to bind here, we set
		 * tc_mid_trip to trip_temp[1];
		 */
		tc_mid_trip = trip_temp[1];
		mtktc_config_all_tc_hw_protect(tc_high_trip, tc_mid_trip);
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind2)) {
		table_val = 2;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind3)) {
		table_val = 3;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind4)) {
		table_val = 4;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind5)) {
		table_val = 5;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind6)) {
		table_val = 6;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind7)) {
		table_val = 7;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind8)) {
		table_val = 8;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind9)) {
		table_val = 9;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else {
		return 0;
	}

	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		tscpu_printk("%s error binding cooling dev\n", __func__);
		return -EINVAL;
	}
	tscpu_dprintk("%s binding OK, %d\n", __func__, table_val);

	return 0;
}

static int tscpu_unbind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind0)) {
		table_val = 0;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind1)) {
		table_val = 1;
		/* only when a valid cooler is tried to bind here, we set
		 * tc_mid_trip to trip_temp[1];
		 */
		tc_mid_trip = -275000;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind2)) {
		table_val = 2;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind3)) {
		table_val = 3;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind4)) {
		table_val = 4;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind5)) {
		table_val = 5;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind6)) {
		table_val = 6;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind7)) {
		table_val = 7;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind8)) {
		table_val = 8;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else if (!strcmp(cdev->type, g_bind9)) {
		table_val = 9;
		/* tscpu_dprintk("%s %s\n", __func__, cdev->type); */
	} else
		return 0;

	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		tscpu_printk("%s error unbinding cooling dev\n", __func__);
		return -EINVAL;
	}
	tscpu_dprintk("%s unbinding OK\n", __func__);

	return 0;
}

static int tscpu_get_mode(struct thermal_zone_device *thermal,
			  enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED : THERMAL_DEVICE_DISABLED;
	return 0;
}

static int tscpu_set_mode(struct thermal_zone_device *thermal,
			  enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}

static int tscpu_get_trip_type(struct thermal_zone_device *thermal, int trip,
			       enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int tscpu_get_trip_temp(struct thermal_zone_device *thermal, int trip,
			       int *temp)
{
	*temp = trip_temp[trip];
	return 0;
}

static int tscpu_set_trip_temp(struct thermal_zone_device *thermal, int trip,
	int temp)
{
	trip_temp[trip] = temp;
	return 0;
}

static int tscpu_get_crit_temp(struct thermal_zone_device *thermal,
			       int *temperature)
{
	*temperature = MTKTSCPU_TEMP_CRIT;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktscpu_dev_ops = {
	.bind = tscpu_bind,
	.unbind = tscpu_unbind,
	.get_temp = tscpu_get_temp,
	.get_mode = tscpu_get_mode,
	.set_mode = tscpu_set_mode,
	.get_trip_type = tscpu_get_trip_type,
	.get_trip_temp = tscpu_get_trip_temp,
	.set_trip_temp = tscpu_set_trip_temp,
	.get_crit_temp = tscpu_get_crit_temp,
};
#endif

/* Add this functio so that thermal-hal will not throw error while parsing
       thermal.policy.conf and disabled thermal-zone. Don't want user to dynamially
       configure the trip point number. nstrip is the number of trip points */
static int mtktscpu_of_set_trips(void *data, int low, int high ) {
               pr_debug("%s: ntrip won't change \n", __func__);
               return 0;
}

static struct thermal_zone_of_device_ops mtktscpu_of_dev_ops = {
		.get_temp = mtktscpu_of_get_temp,
		.set_trips = mtktscpu_of_set_trips,
};

/*
 * cooling device callback functions (tscpu_cooling_sysrst_ops)
 * 1 : ON and 0 : OFF
 */
static int sysrst_cpu_get_max_state(struct thermal_cooling_device *cdev,
				    unsigned long *state)
{
	/* tscpu_dprintk("sysrst_cpu_get_max_state\n"); */
	*state = 1;
	return 0;
}

static int sysrst_cpu_get_cur_state(struct thermal_cooling_device *cdev,
				    unsigned long *state)
{
	/* tscpu_dprintk("sysrst_cpu_get_cur_state\n"); */
	*state = cl_dev_sysrst_state;
	return 0;
}


static int sysrst_cpu_set_cur_state(struct thermal_cooling_device *cdev,
				    unsigned long state)
{
	cl_dev_sysrst_state = state;

	if (cl_dev_sysrst_state == 1) {
		tscpu_dprintk("%s = 1\n", __func__);
		tscpu_dprintk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		tscpu_dprintk("*****************************************\n");
		tscpu_dprintk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

		BUG();
	}
	return 0;
}


static struct thermal_cooling_device_ops mtktscpu_cooling_sysrst_ops = {
	.get_max_state = sysrst_cpu_get_max_state,
	.get_cur_state = sysrst_cpu_get_cur_state,
	.set_cur_state = sysrst_cpu_set_cur_state,
};

#if THERMAL_GPIO_OUT_TOGGLE
static int g_trigger_temp = 95000; /* default 95 deg */
static int g_GPIO_out_enable;      /* 0:disable */
static int g_GPIO_already_set;

#define GPIO_BASE (0xF0005000)
#define GPIO118_MODE (GPIO_BASE + 0x0770)
#define GPIO118_DIR (GPIO_BASE + 0x0070)
#define GPIO118_DOUT (GPIO_BASE + 0x0470)

static void tscpu_set_GPIO_toggle_for_monitor(void)
{
	int lv_GPIO118_MODE, lv_GPIO118_DIR, lv_GPIO118_DOUT;

	tscpu_dprintk(
		"%s,g_GPIO_out_enable=%d\n",
		__func__, g_GPIO_out_enable);

	if (g_GPIO_out_enable == 1) {
		if (g_max_temp > g_trigger_temp) {

			tscpu_printk("g_max_temp %d > g_trigger_temp %d\n",
				     g_max_temp, g_trigger_temp);

			g_GPIO_out_enable = 0; /* only can enter once */
			g_GPIO_already_set = 1;

			lv_GPIO118_MODE = thermal_readl(GPIO118_MODE);
			lv_GPIO118_DIR = thermal_readl(GPIO118_DIR);
			lv_GPIO118_DOUT = thermal_readl(GPIO118_DOUT);

			tscpu_printk(
				"%s:lv_GPIO118_MODE=0x%x,",
				__func__, lv_GPIO118_MODE);
			tscpu_printk(
				"lv_GPIO118_DIR=0x%x,lv_GPIO118_DOUT=0x%x,\n",
				lv_GPIO118_DIR, lv_GPIO118_DOUT);

			/* thermal_clrl(GPIO118_MODE,0x00000E00);clear
			 * GPIO118_MODE[11:9]
			 */
			/* thermal_setl(GPIO118_DIR, 0x00000040);set
			 * GPIO118_DIR[6]=1
			 */
			thermal_clrl(
				GPIO118_DOUT,
				0x00000040); /* set GPIO118_DOUT[6]=0 Low */
			udelay(200);
			thermal_setl(
				GPIO118_DOUT,
				0x00000040); /* set GPIO118_DOUT[6]=1 Hiht */
		} else {
			if (g_GPIO_already_set == 1) {
				/* restore */
				g_GPIO_already_set = 0;
				/* thermal_writel(GPIO118_MODE,lv_GPIO118_MODE);
				 */
				/* thermal_writel(GPIO118_DIR, lv_GPIO118_DIR);
				 */
				/* thermal_writel(GPIO118_DOUT,lv_GPIO118_DOUT);
				 */
				thermal_clrl(GPIO118_DOUT, 0x00000040);
				/* set GPIO118_DOUT[6]=0 Low */
			}
		}
	}
}

static int tscpu_read_GPIO_out(struct seq_file *m, void *v)
{

	seq_printf(m, "GPIO out enable:%d, trigger temperature=%d\n",
		   g_GPIO_out_enable, g_trigger_temp);

	return 0;
}

static ssize_t tscpu_write_GPIO_out(struct file *file,
				    const char __user *buffer, size_t count,
				    loff_t *data)
{
	char desc[512];
	char TEMP[10], ENABLE[10];
	unsigned int valTEMP, valENABLE;

	int len = 0;

	int lv_GPIO118_MODE, lv_GPIO118_DIR;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%9s %d %9s %d ", TEMP, &valTEMP, ENABLE,
		   &valENABLE) == 4) {
		/* tscpu_printk("XXXXXXXXX\n"); */

		if (!strcmp(TEMP, "TEMP")) {
			g_trigger_temp = valTEMP;
			tscpu_printk("g_trigger_temp=%d\n", valTEMP);
		} else {
			tscpu_printk(
				"%s TEMP bad argument\n", __func__);
			return -EINVAL;
		}

		if (!strcmp(ENABLE, "ENABLE")) {
			g_GPIO_out_enable = valENABLE;
			tscpu_printk(
				"g_GPIO_out_enable=%d,g_GPIO_already_set=%d\n",
				valENABLE, g_GPIO_already_set);
		} else {
			tscpu_printk(
				"%s ENABLE bad argument\n", __func__);
			return -EINVAL;
		}

		lv_GPIO118_MODE = thermal_readl(GPIO118_MODE);
		lv_GPIO118_DIR = thermal_readl(GPIO118_DIR);
		/* clear GPIO118_MODE[11:9],GPIO118_MODE = 0 (change to GPIO
		 * mode)
		 */
		thermal_clrl(GPIO118_MODE, 0x00000E00);
		thermal_setl(GPIO118_DIR, 0x00000040);
		/* set GPIO118_DIR[6]=1,GPIO118_DIR =1 (output) */
		thermal_clrl(GPIO118_DOUT, 0x00000040);
		/* set GPIO118_DOUT[6]=0 Low */
		return count;
	}
	tscpu_printk("%s bad argument\n", __func__);

	return -EINVAL;
}
#endif
static int tscpu_read_log(struct seq_file *m, void *v)
{
	seq_printf(m, "[ %s] log = %d\n",
		__func__, mtktscpu_debug_log);
	return 0;
}


static int tscpu_read(struct seq_file *m, void *v)
{
	int i;

	seq_printf(
		m,
		"[%s]%d\ntrip_0=%d %d %s\ntrip_1=%d %d %s\ntrip_2=%d %d %s\ntrip_3=%d %d %s\ntrip_4=%d %d %s\ntrip_5=%d %d %s\ntrip_6=%d %d %s\ntrip_7=%d %d %s\ntrip_8=%d %d %s\ntrip_9=%d %d %s\ninterval=%d\n",
		__func__, num_trip, trip_temp[0], g_THERMAL_TRIP[0], g_bind0,
		trip_temp[1], g_THERMAL_TRIP[1], g_bind1, trip_temp[2],
		g_THERMAL_TRIP[2], g_bind2, trip_temp[3], g_THERMAL_TRIP[3],
		g_bind3, trip_temp[4], g_THERMAL_TRIP[4], g_bind4, trip_temp[5],
		g_THERMAL_TRIP[5], g_bind5, trip_temp[6], g_THERMAL_TRIP[6],
		g_bind6, trip_temp[7], g_THERMAL_TRIP[7], g_bind7, trip_temp[8],
		g_THERMAL_TRIP[8], g_bind8, trip_temp[9], g_THERMAL_TRIP[9],
		g_bind9, interval);

#ifdef CONFIG_MTK_GPU_SUPPORT
	for (i = 0; i < Num_of_GPU_OPP; i++)
		seq_printf(m, "g %d %d %d\n", i, mtk_gpu_power[i].gpufreq_khz,
			   mtk_gpu_power[i].gpufreq_power);
#endif

	for (i = 0; i < tscpu_num_opp; i++)
		seq_printf(m, "c %d %d %d %d\n", i,
			   mtk_cpu_power[i].cpufreq_khz,
			   mtk_cpu_power[i].cpufreq_ncpu,
			   mtk_cpu_power[i].cpufreq_power);

	for (i = 0; i < CPU_COOLER_NUM; i++)
		seq_printf(m, "d %d %d\n", i, tscpu_cpu_dmips[i]);

	return 0;
}


static ssize_t tscpu_write_log(struct file *file, const char __user *buffer,
			       size_t count, loff_t *data)
{
	char desc[32];
	int log_switch;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	/*if (sscanf(desc, "%d", &log_switch) == 1)*/
	if (kstrtoint(desc, 10, &log_switch) == 0) {
		mtktscpu_debug_log = log_switch;
		return count;
	}
	tscpu_printk("%s bad argument\n", __func__);

	return -EINVAL;
}

#if 0
static ssize_t tscpu_write(struct file *file, const char __user *buffer,
			   size_t count, loff_t *data)
{
	int len = 0, time_msec = 0;
	int trip[10] = {0};
	int t_type[10] = {0};
	int i;
	char bind0[20], bind1[20], bind2[20], bind3[20], bind4[20];
	char bind5[20], bind6[20], bind7[20], bind8[20], bind9[20];
	char desc[512];

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc,
		   "%d %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d",
		   &num_trip, &trip[0], &t_type[0], bind0, &trip[1], &t_type[1],
		   bind1, &trip[2], &t_type[2], bind2, &trip[3], &t_type[3],
		   bind3, &trip[4], &t_type[4], bind4, &trip[5], &t_type[5],
		   bind5, &trip[6], &t_type[6], bind6, &trip[7], &t_type[7],
		   bind7, &trip[8], &t_type[8], bind8, &trip[9], &t_type[9],
		   bind9, &time_msec, &MA_len_temp) == 33) {

		tscpu_dprintk(
			"%s tscpu_unregister_thermal MA_len_temp=%d\n",
			__func__, MA_len_temp);

		//tscpu_unregister_thermal();

		if (num_trip < 0 || num_trip > 10)
			return -EINVAL;

		for (i = 0; i < num_trip; i++)
			g_THERMAL_TRIP[i] = t_type[i];

		g_bind0[0] = g_bind1[0] = g_bind2[0] =
		g_bind3[0] = g_bind4[0] = g_bind5[0] =
		g_bind6[0] = g_bind7[0] = g_bind8[0] =
		g_bind9[0] = '\0';

		for (i = 0; i < 20; i++) {
			g_bind0[i] = bind0[i];
			g_bind1[i] = bind1[i];
			g_bind2[i] = bind2[i];
			g_bind3[i] = bind3[i];
			g_bind4[i] = bind4[i];
			g_bind5[i] = bind5[i];
			g_bind6[i] = bind6[i];
			g_bind7[i] = bind7[i];
			g_bind8[i] = bind8[i];
			g_bind9[i] = bind9[i];
		}

#if CPT_ADAPTIVE_AP_COOLER
		/* initialize... */
		for (i = 0; i < MAX_CPT_ADAPTIVE_COOLERS; i++)
			TARGET_TJS[i] = 117000;

		if (!strncmp(bind0, adaptive_cooler_name, 13)
			&& bind0[13] - '0' >= 0
			&& bind0[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind0[13] - '0')] = trip[0];

		if (!strncmp(bind1, adaptive_cooler_name, 13)
			&& bind1[13] - '0' >= 0
			&& bind1[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind1[13] - '0')] = trip[1];

		if (!strncmp(bind2, adaptive_cooler_name, 13)
			&& bind2[13] - '0' >= 0
			&& bind2[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind2[13] - '0')] = trip[2];

		if (!strncmp(bind3, adaptive_cooler_name, 13)
			&& bind3[13] - '0' >= 0
			&& bind3[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind3[13] - '0')] = trip[3];

		if (!strncmp(bind4, adaptive_cooler_name, 13)
			&& bind4[13] - '0' >= 0
			&& bind4[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind4[13] - '0')] = trip[4];

		if (!strncmp(bind5, adaptive_cooler_name, 13)
			&& bind5[13] - '0' >= 0
			&& bind5[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind5[13] - '0')] = trip[5];

		if (!strncmp(bind6, adaptive_cooler_name, 13)
			&& bind6[13] - '0' >= 0
			&& bind6[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind6[13] - '0')] = trip[6];

		if (!strncmp(bind7, adaptive_cooler_name, 13)
			&& bind7[13] - '0' >= 0
			&& bind7[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind7[13] - '0')] = trip[7];

		if (!strncmp(bind8, adaptive_cooler_name, 13)
			&& bind8[13] - '0' >= 0
			&& bind8[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind8[13] - '0')] = trip[8];

		if (!strncmp(bind9, adaptive_cooler_name, 13)
			&& bind9[13] - '0' >= 0
			&& bind9[13] - '0' <= MAX_CPT_ADAPTIVE_COOLERS)
			TARGET_TJS[(bind9[13] - '0')] = trip[9];

		tscpu_dprintk("%s TTJ0=%d, TTJ1=%d, TTJ2=%d\n",
			      __func__, TARGET_TJS[0],
				  TARGET_TJS[1], TARGET_TJS[2]);
#endif

		tscpu_dprintk(
			"%s g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,",
			__func__, g_THERMAL_TRIP[0], g_THERMAL_TRIP[1],
			g_THERMAL_TRIP[2]);
		tscpu_dprintk(
			"g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,g_THERMAL_TRIP_5=%d,",
			g_THERMAL_TRIP[3], g_THERMAL_TRIP[4],
			g_THERMAL_TRIP[5]);
		tscpu_dprintk(
			"g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
			g_THERMAL_TRIP[6], g_THERMAL_TRIP[7], g_THERMAL_TRIP[8],
			g_THERMAL_TRIP[9]);
		tscpu_dprintk(
			"%s cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,",
			__func__, g_bind0, g_bind1, g_bind2, g_bind3, g_bind4);
		tscpu_dprintk(
			"cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
			g_bind5, g_bind6, g_bind7, g_bind8, g_bind9);

		tc_high_trip = trip[0];
		tc_mid_trip = trip[1];

		for (i = 0; i < num_trip; i++)
			trip_temp[i] = trip[i];

		interval = time_msec;

		tscpu_dprintk(
			"%s trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,",
			__func__, trip_temp[0], trip_temp[1], trip_temp[2]);
		tscpu_dprintk("trip_3_temp=%d,trip_4_temp=%d,trip_5_temp=%d,",
			      trip_temp[3], trip_temp[4], trip_temp[5]);
		tscpu_dprintk("trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,",
			      trip_temp[6], trip_temp[7], trip_temp[8]);
		tscpu_dprintk("trip_9_temp=%d,time_ms=%d, num_trip=%d\n",
			      trip_temp[9], interval, num_trip);

		tscpu_dprintk("%s tscpu_register_thermal\n", __func__);
		//tscpu_register_thermal();

		return count;
	}
	tscpu_dprintk("%s bad argument\n", __func__);

	return -EINVAL;
}
#endif

int tscpu_register_sysrst_cooler(void)
{
	cl_dev_sysrst = mtk_thermal_cooling_device_register(
		"mtktscpu-sysrst", NULL, &mtktscpu_cooling_sysrst_ops);

	return 0;
}

#ifdef CONFIG_THERMAL_VIRTUAL_SENSOR
static int tscpu_read_temp(struct device *dev, int index, int *temp)
{
	int ret = 0;

	if (g_tscpu_tz->tscpu_tzd)
		ret = tscpu_get_temp(g_tscpu_tz->tscpu_tzd, temp);
	else {
		pr_err("tscpu_tzd does not exist");
		ret = -EINVAL;
	}

	return ret;
}

static struct thermal_dev_ops tscpu_fops = {
        .get_temp = tscpu_read_temp,
};
#endif


int tscpu_register_of_thermal(struct platform_device *pdev)
{
       int ret = 0;

       tscpu_dprintk("tscpu_register_of_thermal\n");

       g_tscpu_tz = kzalloc(sizeof(struct tscpu_thermal_zone), GFP_KERNEL);
       if (!g_tscpu_tz) {
               pr_err("%s: kzalloc failed\n", __func__);
               return -ENOMEM;
       }

       g_tscpu_tz->tscpu_tzd = kzalloc(sizeof(struct thermal_zone_device), GFP_KERNEL);
       if (!g_tscpu_tz->tscpu_tzd) {
               pr_err("%s: kzalloc failed\n", __func__);
               ret = -ENOMEM;
               goto exit;
       }

       g_tscpu_tz->tscpu_tzd = thermal_zone_of_sensor_register(&pdev->dev,
                                               0,
                                               g_tscpu_tz,
                                               &mtktscpu_of_dev_ops);
       if (IS_ERR(g_tscpu_tz->tscpu_tzd)) {
               pr_err("%s: failed to register sensor\n", __func__);
               ret = -EINVAL;
               goto exit0;
       }

       dev_set_drvdata(&pdev->dev, g_tscpu_tz);

#ifdef CONFIG_THERMAL_VIRTUAL_SENSOR
       ret = virtual_sensor_dev_register(&pdev->dev, &tscpu_fops, 0);
       if (ret) {
	       pr_err("%s: Error registering thermal device for tscpu\n", __func__);
	       return -EINVAL;
       }
#endif


       pr_info("%s: success\n", __func__);

       return 0;

exit0:
       kfree(g_tscpu_tz->tscpu_tzd);
exit:
       kfree(g_tscpu_tz);
       return ret;
}
EXPORT_SYMBOL(tscpu_register_of_thermal);

#if 0
static int tscpu_register_thermal(void)
{
	pr_err("tscpu_register_thermal does not work\n");
        /* prevent someone accidentally call this function, and create mtk default thermal zone,
           exit early here as well */
        return 0;


	/* trips : trip 0~3 */
	thz_dev = mtk_thermal_zone_device_register("mtktscpu", num_trip, NULL,
						   &mtktscpu_dev_ops, 0, 0, 0,
						   interval);

	return 0;
}
#endif

void tscpu_unregister_sysrst_cooler(void)
{
	if (cl_dev_sysrst) {
		mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
		cl_dev_sysrst = NULL;
	}
}

int tscpu_unregister_of_thermal(struct platform_device *pdev)
{
	tscpu_dprintk("%s\n", __func__);
	if (g_tscpu_tz->tscpu_tzd) {
		thermal_zone_of_sensor_unregister(&pdev->dev, g_tscpu_tz->tscpu_tzd);
	}

	kfree(g_tscpu_tz->tscpu_tzd);
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(g_tscpu_tz);

	return 0;
}
EXPORT_SYMBOL(tscpu_unregister_of_thermal);

#if 0
static void tscpu_unregister_thermal(void)
{

	tscpu_dprintk("%s\n", __func__);
	if (thz_dev) {
		mtk_thermal_zone_device_unregister(thz_dev);
		thz_dev = NULL;
	}
}
#endif

static int tscpu_set_temperature_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", temperature_switch);
	return 0;
}
#if 0
static ssize_t tscpu_set_temperature_write(struct file *file,
					   const char __user *buffer,
					   size_t count, loff_t *data)
{
	char desc[32];
	int lv_tempe_switch;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	tscpu_dprintk("%s\n", __func__);

	/*if (sscanf(desc, "%d", &lv_tempe_switch) == 1) {*/
	if (kstrtoint(desc, 10, &lv_tempe_switch) == 0) {
		temperature_switch = lv_tempe_switch;

#if 0
		tscpu_config_all_tc_hw_protect(
			temperature_switch, tc_mid_trip);
#endif
		tscpu_dprintk(
			"%s temperature_switch=%d\n",
			__func__, temperature_switch);
		return count;
	}
	tscpu_printk("%s bad argument\n", __func__);

	return -EINVAL;
}
#endif

static int tscpu_set_temperature_open(
	struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_set_temperature_read, NULL);
}

static const struct file_operations mtktscpu_set_temperature_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_set_temperature_open,
	.read = seq_read,
	.llseek = seq_lseek,
	//.write = tscpu_set_temperature_write,
	.release = single_release,
};


static int tscpu_read_opp(struct seq_file *m, void *v)
{

	unsigned int cpu_power;
#ifdef CONFIG_MTK_GPU_SUPPORT
	unsigned int gpu_power;
	unsigned int gpu_loading = 0;

	gpu_power = final_gpu_limit;
#endif
	cpu_power = final_cpu_limit;


#ifdef CONFIG_MTK_GPU_SUPPORT
	if (!mtk_get_gpu_loading(&gpu_loading))
		gpu_loading = 0;

	seq_printf(m, "%d,%d,%d,%d\n",
		   (int)((cpu_power != 0x7FFFFFFF) ? cpu_power : 0),
		   (int)((gpu_power != 0x7FFFFFFF) ? gpu_power : 0),
		   (int)gpu_loading,
		   (int)mt_gpufreq_get_cur_freq());

#if CPT_ADAPTIVE_AP_COOLER
	seq_printf(m, "%d,%d,%d,%d\n",
			adaptive_cpu_power_limit,
			static_cpu_power_limit,
			adaptive_gpu_power_limit,
			static_gpu_power_limit);
#else
/*	seq_printf(m, "%d,%d,%d,%d\n",
			adaptive_cpu_power_limit,
			static_cpu_power_limit,
			0,
			0); */
#endif

#else
	seq_printf(m, "%d,0,0,0\n",
	   (int)((cpu_power != 0x7FFFFFFF) ? cpu_power : 0));

#if CPT_ADAPTIVE_AP_COOLER
	seq_printf(m, "%d,%d,%d,%d\n",
			adaptive_cpu_power_limit,
			static_cpu_power_limit,
			0,
			0);
#else
/*	seq_printf(m, "%d,%d,%d,%d\n",
			0,
			static_cpu_power_limit,
			0,
			0); */
#endif

#endif

	return 0;
}


static int tscpu_open_opp(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_opp, NULL);
}

static const struct file_operations mtktscpu_opp_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_open_opp,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


#if defined(THERMAL_SSPM_THERMAL_THROTTLE_SWITCH)

static int tscpu_sspm_thermal_throttle;

void tscpu_ipi_send_sspm_thermal_thtottle(void)
{
	struct thermal_ipi_data thermal_data;

	lvts_printk("%s\n", __func__);

	thermal_data.u.data.arg[0] = tscpu_sspm_thermal_throttle;
	thermal_data.u.data.arg[1] = 0;
	thermal_data.u.data.arg[2] = 0;
	while (thermal_to_mcupm(THERMAL_IPI_SET_DIS_THERMAL_THROTTLE,
		&thermal_data) != 0)
		udelay(100);
}

static int tscpu_read_sspm_thermal_throttle(
	struct seq_file *m, void *v)
{
	seq_printf(m, "tscpu_sspm_thermal_throttle:%d\n",
				tscpu_sspm_thermal_throttle);
	return 0;
}

static ssize_t tscpu_write_sspm_thermal_throttle
	(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	char desc[32];
	int sspm_thermal_throttle_switch;
	int len = 0;

	tscpu_warn("%s\n", __func__);

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtoint(desc, 10, &sspm_thermal_throttle_switch) == 0) {
		tscpu_sspm_thermal_throttle =
			sspm_thermal_throttle_switch;

		tscpu_warn("%s , %d\n", __func__,
			tscpu_sspm_thermal_throttle);

		tscpu_ipi_send_sspm_thermal_thtottle();

		return count;
	}

	tscpu_warn("%s bad argument\n", __func__);
	return -EINVAL;
}

static int tscpu_sspm_thermal_throttle_open
(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_sspm_thermal_throttle, NULL);
}

static const struct file_operations mtktscpu_sspm_thermal_throttle = {
	.owner = THIS_MODULE,
	.open = tscpu_sspm_thermal_throttle_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_sspm_thermal_throttle,
	.release = single_release,
};

#endif

#if THERMAL_GPIO_OUT_TOGGLE
static int tscpu_GPIO_out(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_GPIO_out, NULL);
}

static const struct file_operations mtktscpu_GPIO_out_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_GPIO_out,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_GPIO_out,
	.release = single_release,
};
#endif

static int tscpu_open_log(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_log, NULL);
}

static const struct file_operations mtktscpu_log_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_open_log,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_log,
	.release = single_release,
};

static int tscpu_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read, NULL);
}

static const struct file_operations mtktscpu_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_open,
	.read = seq_read,
	.llseek = seq_lseek,
	//.write = tscpu_write,
	.release = single_release,
};

static void mtkts_cpu_start_thermal_timer(void)
{

	/* resume thermal framework polling when leaving deep idle */
	if (thz_dev != NULL && interval != 0)
		/* 60ms */
		mod_delayed_work(system_freezable_wq, &(thz_dev->poll_queue),
				round_jiffies(msecs_to_jiffies(50)));
}

static void mtkts_cpu_cancel_thermal_timer(void)
{

	/* stop thermal framework polling when entering deep idle */
	if (thz_dev)
		cancel_delayed_work(&(thz_dev->poll_queue));
}

static int tscpu_thermal_init(void)
{
	int err = 0;
	//int temp = 0;

	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktscpu_dir = NULL;

	tscpu_printk("%s\n", __func__);

	/*err = tscpu_register_sysrst_cooler();
	if (err) {
		tscpu_printk("tscpu_register_sysrst_cooler fail\n");
		return err;
	}
	err = tscpu_register_thermal();
	if (err) {
		tscpu_printk("tscpu_register_thermal fail\n");
		goto err_unreg;
	}*/

	mtktscpu_dir = mtk_thermal_get_proc_drv_therm_dir_entry();
	if (!mtktscpu_dir) {
		tscpu_printk("[%s]: mkdir /proc/driver/thermal failed\n",
			     __func__);
	} else {
		entry = proc_create("tzcpu", 0664, mtktscpu_dir,
				    &mtktscpu_fops);
		if (entry)
			proc_set_user(entry, uid, gid);

		entry = proc_create("tzcpu_log", 0644, mtktscpu_dir,
				    &mtktscpu_log_fops);

#if THERMAL_GPIO_OUT_TOGGLE
		entry = proc_create("tzcpu_GPIO_out_monitor", 0644,
				    mtktscpu_dir, &mtktscpu_GPIO_out_fops);
		if (entry)
			proc_set_user(entry, uid, gid);
#endif

#if defined(THERMAL_SSPM_THERMAL_THROTTLE_SWITCH)
		entry = proc_create("sspm_thermal_throttle",
				0644, mtktscpu_dir,
				&mtktscpu_sspm_thermal_throttle);
		if (entry)
			proc_set_user(entry, uid, gid);
#endif

		entry = proc_create("tzcpu_set_temperature",
				0644, mtktscpu_dir,
				&mtktscpu_set_temperature_fops);
		if (entry)
			proc_set_user(entry, uid, gid);

		entry = proc_create("thermlmt", 0444,
				NULL, &mtktscpu_opp_fops);
		if (entry)
			proc_set_user(entry, uid, gid);

	mtkTTimer_register("mtktscpu",
		mtkts_cpu_start_thermal_timer,
		mtkts_cpu_cancel_thermal_timer);

	}

	return 0;

//err_unreg:
//	tscpu_unregister_sysrst_cooler();
	return err;
}

static int tscpu_thermal_remove(void)
{

	pr_info("%s\n", __func__);

#if THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET
		mt_thermalsampler_registerCB(NULL);
#endif
	mtkTTimer_unregister("mtktscpu");
	//tscpu_unregister_thermal();
	tscpu_unregister_sysrst_cooler();

	return 0;
}

static int __init tscpu_init(void)
{
	tscpu_thermal_init();
	return 0;
}

static void __exit tscpu_exit(void)
{
	tscpu_thermal_remove();
	tscpu_dprintk("%s\n", __func__);
}
module_init(tscpu_init);
module_exit(tscpu_exit);

/* late_initcall(thermal_late_init); */
/* device_initcall(thermal_late_init); */
