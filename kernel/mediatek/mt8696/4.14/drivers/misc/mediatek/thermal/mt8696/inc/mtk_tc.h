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

#ifndef __MTK_THERMAL_CONTROLLER_H__
#define __MTK_THERMAL_CONTROLLER_H__

#define _BIT_(_bit_)		(unsigned int)(1 << (_bit_))
#define _BITMASK_(_bits_)	(((unsigned int) -1 >> (31 - ((1) ? _bits_)))\
	& ~((1U << ((0) ? _bits_)) - 1))

#define THERMAL_INIT_VALUE		(0xDA1)


enum thermal_sensor {
	TS_MCU1 = 0,
	TS_MCU2,
	TS_MCU3,
	TS_MCU4,
	TS_MCU5,
	TS_MCU6,
	TS_ABB, /* TODO: double check if TS_ABB exists. */
	TS_NUM_MAX,
};


enum thermal_controller_name {
	THERMAL_CONTROLLER0 = 0,
	THERMAL_CONTROLLER1,
	THERMAL_CONTROLLER_NUM
};

struct thermal_controller_speed {
	unsigned int tempMonCtl1;
	unsigned int tempMonCtl2;
	unsigned int tempAhbPoll;
};

struct thermal_controller {
	enum thermal_sensor ts[4];
	int ts_number;
	void __iomem *addr;
	int tc_offset;
	struct thermal_controller_speed tc_speed;
};

extern u32 get_devinfo_with_index(u32 index);
extern int mtkTTimer_register(const char *name,
	void (*start_timer)(void), void (*cancel_timer)(void));
extern int mtkTTimer_unregister(const char *name);

static void mtktc_fast_initial_sw_workaround(void);
static void mtktc_reset_thermal(void);
static s32 temperature_to_raw_room(u32 ret,
	enum thermal_sensor sensor_id);
static void set_tc_trigger_hw_protect(int temperature,
	int temperature2, int tc_id);
void mtktc_config_all_tc_hw_protect(int temperature,
	int temperature2);
static void thermal_initial(void);
static int thermal_fast_init(int tc_num);

static void mtktc_update_temperature_timer_init(void);
static s32 raw_to_temperature_roomt(u32 ret,
	enum thermal_sensor sensor_id);

static void thermal_buffer_turn_on(void);
static void thermal_buffer_turn_off(void);

extern struct proc_dir_entry
	*mtk_thermal_get_proc_drv_therm_dir_entry(void);
extern int tscpu_register_of_thermal(struct platform_device *pdev);
extern int tscpu_unregister_of_thermal(struct platform_device *pdev);

extern int fast_polling_trip_temp;
extern int fast_polling_factor;

extern int polling_trip_temp0;
extern int polling_trip_temp1;
extern int polling_trip_temp2;
extern int polling_factor0;
extern int polling_factor1;
extern int polling_factor2;

extern int tc_mid_trip;
extern int tc_high_trip;

#ifdef CONFIG_MTK_GPU_SUPPORT
extern int _get_current_gpu_power(void);
#endif

extern int _get_current_cpu_power(void);
extern unsigned int sched_get_percpu_load(int cpu, bool reset,
					  bool use_maxfreq);

#endif				/* __MTK_TS_CPU_H__ */
