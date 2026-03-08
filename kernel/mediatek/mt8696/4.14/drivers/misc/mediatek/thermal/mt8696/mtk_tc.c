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
#include <linux/time.h>
#include <linux/uidgid.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/sched.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif

#include "mtk_thermal_typedefs.h"
#include "inc/mtk_tc.h"
#include "mtk_thermal_reg.h"
#include "mtk_thermal_ipi.h"


#define TIMER_V1 (0)

#define THERM_CTRL_IRQ_BIT_ID (258)

#define MTK_TS_CPU_RT (0)

#define THERMAL_TURNOFF_AUXADC_BEFORE_DEEPIDLE (1)
#define THERMAL_PERFORMANCE_PROFILE (0)

#define NORMAL_TEMP_POLL (400)
static int temp_update_interval = NORMAL_TEMP_POLL;
static unsigned int g_golden_temp;


/* 1: thermal driver update temp to MET directly,
 * use hrtimer; 0: turn off.
 */

#define mtktc_dprintk(fmt, args...)                      \
	do {                                                 \
		if (mtktc_debug_log & 0x2) {                  \
			pr_info("Power/CPU_Thermal" fmt, ##args);    \
		}                                                \
	} while (0)

#define mtktc_printk(fmt, args...)                       \
	do {                                                 \
		if (mtktc_debug_log & 0x1) {                  \
			pr_info("Power/CPU_Thermal" fmt, ##args);    \
		}                                                \
	} while (0)

#define THERMAL_NAME "mt8696-thermal"

#ifdef CONFIG_OF

static u32 thermal_irq_number;
void __iomem *thermal_base;
void __iomem *auxadc_ts_base;
void __iomem *apmixed_base;
void __iomem *INFRACFG_AO_BASE;
void __iomem *PERICFG_AO_BASE;

int thermal_phy_base;
int auxadc_ts_phy_base;
int apmixed_phy_base;

static struct clk *clk_peri_therm;
static struct clk *clk_auxadc;
#endif

static struct hrtimer ts_tempinfo_hrtimer;
static struct timer_list normal_timer;
static int hrtimer_flag = -1;

static u32 calefuse1;
static u32 calefuse2;
static u32 calefuse3;
static u32 calefuse4;

static int mtktc_debug_log;
static int read_curr_temp;


#if MTK_TS_CPU_RT
static struct task_struct *ktp_thread_handle;
#endif

static int g_tc_resume; /* default=0,read temp */

static s32 g_adc_ge_t;
static s32 g_adc_oe_t;
static s32 g_o_vts[TS_NUM_MAX] = {260, 260, 260,
	260, 260, 260, 260};

static s32 g_degc_cali;
static s32 g_adc_cali_en_t;
static s32 g_o_slope;
static s32 g_o_slope_sign;
static s32 g_id;
static s32 g_ge = 1;
static s32 g_oe = 1;
static s32 g_gain = 1;

static s32 g_x_roomt[TS_NUM_MAX] = {0};

static bool talking_flag;

static int SOC_TS_TEMP_T[TS_NUM_MAX] = {0};
static int SOC_TS_TEMP_R[TS_NUM_MAX] = {THERMAL_INIT_VALUE,
	THERMAL_INIT_VALUE,	THERMAL_INIT_VALUE,
	THERMAL_INIT_VALUE,	THERMAL_INIT_VALUE,
	THERMAL_INIT_VALUE,	THERMAL_INIT_VALUE};

//static int g_is_temp_valid;

struct thermal_controller tscpu_g_tc[THERMAL_CONTROLLER_NUM] = {
	[0] = {
		.ts = {TS_MCU1, TS_MCU4, TS_MCU5, TS_MCU6},
		.ts_number = 4,
		.tc_offset = 0x0,
		.tc_speed = {
			0x00000004,
			0x0001000A,
			0x0000030D
		}
	},
	[1] = {
		.ts = {TS_MCU2, TS_MCU3},
		.ts_number = 2,
		.tc_offset = 0x100,
		.tc_speed = {
			0x00000004,
			0x0001000A,
			0x0000030D
		}
	}
};


static DEFINE_SPINLOCK(tempinfo_timer_lock);
static DEFINE_SPINLOCK(thermal_spinlock);

static void mt_thermal_lock(unsigned long *x)
{
	spin_lock_irqsave(&thermal_spinlock, *x);
	/*return 0;*/
};

static void mt_thermal_unlock(unsigned long *x)
{
	spin_unlock_irqrestore(&thermal_spinlock, *x);
	/*return 0;*/
};


void set_taklking_flag(bool flag)
{
	talking_flag = flag;
	mtktc_printk("%s=%d\n", __func__, talking_flag);
}

static void mtktc_thermal_clock_on(void)
{
	mtktc_printk("%s\n", __func__);

	if (clk_prepare_enable(clk_auxadc))
		pr_info("%s enable auxadc clk fail.", __func__);
	if (clk_prepare_enable(clk_peri_therm))
		pr_info("%s enable thermal clk fail.", __func__);
}

static void mtktc_thermal_clock_off(void)
{
	mtktc_printk("%s\n", __func__);
	clk_disable_unprepare(clk_peri_therm);
	clk_disable_unprepare(clk_auxadc);
}

/*
 * if you don't want to enter int,
 * you need write bakc value to TEMPMONINTSTS
 */
static void thermal_interrupt_handler(int tc_id)
{
	u32 ret = 0;
	unsigned long flags;
	int offset;

	offset = tscpu_g_tc[tc_id].tc_offset;

	mt_thermal_lock(&flags);

	ret = DRV_Reg32((offset + TEMPMONINTSTS));
	mtktc_printk("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
	mtktc_printk("%s,ret=0x%08x\n", __func__, ret);
	mtktc_printk("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");

	if (ret & THERMAL_MON_CINTSTS0)
		mtktc_dprintk(
			"thermal_isr: thermal sensor point 0 - cold interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS0)
		mtktc_dprintk(
			"<<<thermal_isr>>>: thermal sensor point 0 - hot interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS1)
		mtktc_dprintk(
			"<<<thermal_isr>>>: thermal sensor point 1 - hot interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS2)
		mtktc_dprintk(
			"<<<thermal_isr>>>: thermal sensor point 2 - hot interrupt trigger\n");

	if (ret & THERMAL_tri_SPM_State0)
		mtktc_dprintk(
			"thermal_isr: Thermal state0 to trigger SPM state0\n");
	if (ret & THERMAL_tri_SPM_State1) {
/* mtktc_dprintk("thermal_isr: Thermal state1 to trigger SPM state1\n"); */
#if MTK_TS_CPU_RT

		//mtktc_dprintk("THERMAL_tri_SPM_State1, T=%d,%d,%d\n",
		//	      SOC_TS_MCU1_T, SOC_TS_MCU2_T, SOC_TS_MCU2_T);

		wake_up_process(ktp_thread_handle);
#endif
	}
	if (ret & THERMAL_tri_SPM_State2)
		mtktc_dprintk(
			"thermal_isr: Thermal state2 to trigger SPM state2\n");

	mt_thermal_unlock(&flags);

}


static irqreturn_t
tscpu_thermal_all_tc_interrupt_handler(
	int irq, void *dev_id)
{
	unsigned int ret = 0, i  = 0, mask = 1;
	unsigned long flags;

	mt_thermal_lock(&flags);
	ret = DRV_Reg32(THERMINTST);
	ret = ret & 0xF;
	mtktc_printk("thermal_interrupt_handler : THERMINTST = 0x%x\n", ret);
	mt_thermal_unlock(&flags);
	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		mask = 1 << i;

		if ((ret & mask) == 0)
			thermal_interrupt_handler(i);
	}
	return IRQ_HANDLED;
}


static void thermal_reset_and_initial(int tc_num)
{
	u32 offset, tempMonCtl1, tempMonCtl2, tempAhbPoll;

	offset = tscpu_g_tc[tc_num].tc_offset;
	tempMonCtl1 = tscpu_g_tc[tc_num].tc_speed.tempMonCtl1;
	tempMonCtl2 = tscpu_g_tc[tc_num].tc_speed.tempMonCtl2;
	tempAhbPoll = tscpu_g_tc[tc_num].tc_speed.tempAhbPoll;


	/* Calculating period unit in Module clock x 256, and the Module clock
	 */
	/* will be changed to 26M when Infrasys enters Sleep mode. */
	/* THERMAL_WRAP_WR32(0x000003FF, TEMPMONCTL1);     counting unit is 1023
	 * * 15.15ns ~ 15.5us
	 */
	/* bus clock 66M counting unit is 4*15.15ns* 256 = 15513.6 ms=15.5us */
	THERMAL_WRAP_WR32(tempMonCtl1, (offset + TEMPMONCTL1));
	/* THERMAL_WRAP_WR32(0x000001FF, TEMPMONCTL1);*/

	THERMAL_WRAP_WR32(tempMonCtl2, (offset + TEMPMONCTL2));
	/* filter interval is 1023 * 15.5us ~ 15.86ms */
	/* filt interval is 1 * 46.540us = 46.54us, sen interval is 429 *
	 * 46.540us = 19.96ms
	 */
	/* filt interval is 1 * 46.540us = 46.54us, sen interval is 858 *
	 * 46.540us = 39.93ms
	 */
	/* THERMAL_WRAP_WR32(0x0001035A, TEMPMONCTL2); */
	/* filt interval is 1 * 46.540us = 46.54us, sen interval is 1287 *
	 * 46.540us = 59.89 ms
	 */
	/* THERMAL_WRAP_WR32(0x00010507, TEMPMONCTL2); */
	/*  poll is set to 1 * 46.540us = 46.540us */
	/* THERMAL_WRAP_WR32(0x00000001, TEMPAHBPOLL); */
	THERMAL_WRAP_WR32(tempAhbPoll, (offset + TEMPAHBPOLL));
	/* poll is set to 10u */
	/* temperature sampling control, 1 sample */
	/* THERMAL_WRAP_WR32(0x00000000, TEMPMSRCTL0);*/
	/* temperature sampling control, 2sample */
	/* THERMAL_WRAP_WR32(0x00000249, TEMPMSRCTL0);*/
	/* temperature sampling control, 4 sample */
	/* THERMAL_WRAP_WR32(0x00000492, TEMPMSRCTL0);*/
	/* temperature sampling control, 6 sample */
	THERMAL_WRAP_WR32(0x000006DB, (offset + TEMPMSRCTL0));
	/* temperature sampling control, 10 sample */
	/* THERMAL_WRAP_WR32(0x0000924, TEMPMSRCTL0);*/
	/* temperature sampling control, 18 sample */
	/* THERMAL_WRAP_WR32(0x0000B6D, TEMPMSRCTL0);*/

	THERMAL_WRAP_WR32(0xFFFFFFFF, (offset + TEMPAHBTO));
	/* exceed this polling time, IRQ would be inserted */

	THERMAL_WRAP_WR32(0x00000000, (offset + TEMPMONIDET0));
	/* times for interrupt occurrance */
	THERMAL_WRAP_WR32(0x00000000, (offset + TEMPMONIDET1));
	/* times for interrupt occurrance */
	THERMAL_WRAP_WR32(0x00000000, (offset + TEMPMONIDET2));
	/* times for interrupt occurrance */

	/* this value will be stored to TEMPPNPMUXADDR (TEMPSPARE0)
	 * automatically by hw
	 */
	THERMAL_WRAP_WR32(0x800, (offset + TEMPADCMUX));
	THERMAL_WRAP_WR32((u32)AUXADC_CON1_CLR_P, (offset + TEMPADCMUXADDR));

	THERMAL_WRAP_WR32(0x800, (offset + TEMPADCEN));
	/* AHB value for auxadc enable */
	/* AHB address for auxadc enable (channel 0 immediate mode selected) */
	THERMAL_WRAP_WR32((u32)AUXADC_CON1_SET_P, (offset + TEMPADCENADDR));

	THERMAL_WRAP_WR32((u32)AUXADC_DAT11_P, (offset + TEMPADCVALIDADDR));
	/* AHB address for auxadc valid bit */
	THERMAL_WRAP_WR32((u32)AUXADC_DAT11_P, (offset + TEMPADCVOLTADDR));
	/* AHB address for auxadc voltage output */

	THERMAL_WRAP_WR32(0x0, (offset + TEMPRDCTRL));
	/* read valid & voltage are at the same register */
	/* indicate where the valid bit is (the 12th bit is valid bit and 1 is
	 * valid)
	 */
	THERMAL_WRAP_WR32(0x0000002C, (offset + TEMPADCVALIDMASK));
	THERMAL_WRAP_WR32(0x0, (offset + TEMPADCVOLTAGESHIFT));
	/* do not need to shift */

#if 0
	THERMAL_WRAP_WR32(0x0, TEMPADCPNP0);
	THERMAL_WRAP_WR32(0x1, TEMPADCPNP1);
	THERMAL_WRAP_WR32(0x2, TEMPADCPNP2);
	THERMAL_WRAP_WR32(0x3, TEMPADCPNP3);
	THERMAL_WRAP_WR32(TS_CON1_P, TEMPPNPMUXADDR);
	/* AHB address for pnp sensor mux selection */
	THERMAL_WRAP_WR32(0x3, TEMPADCWRITECTRL);
#endif
}

/**
 * temperature2 to set the middle threshold
 * for interrupting CPU. -275000 to
 * disable it.
 */

static void set_tc_trigger_hw_protect(
	int temperature, int temperature2, int tc_id)
{
	int temp = 0, temp_high = 0, temp_middle = 0, temp_low = 0;
	int raw_high = 0, raw_middle = 0, raw_low = 0;
	u32 offset;
	int i;

	offset = tscpu_g_tc[tc_id].tc_offset;

	for (i = 0; i < tscpu_g_tc[tc_id].ts_number; i++) {

		temp_high = temperature_to_raw_room(temperature,
			tscpu_g_tc[tc_id].ts[i]);
		temp_middle = temperature_to_raw_room(temperature2,
						tscpu_g_tc[tc_id].ts[i]);
		temp_low = temperature_to_raw_room(5000,
			tscpu_g_tc[tc_id].ts[i]);

		mtktc_dprintk("thermal controller id : %d, sensor id: %d\n",
					tc_id, tscpu_g_tc[tc_id].ts[i]);
		mtktc_dprintk("temperature_high: %d, raw_high: %d\n",
					temperature, temp_high);
		mtktc_dprintk("temperature_midle: %d, raw_middle: %d\n",
				temperature2, temp_middle);
		mtktc_dprintk("temperature_low: 50000, raw_low: %d\n",
				temp_low);

		raw_high = raw_high > temp_high ? raw_high : temp_high;
		raw_middle = raw_middle > temp_middle ?
			raw_middle : temp_middle;
		raw_low = raw_low > temp_low ? raw_low : temp_low;

	}

	temp = DRV_Reg32((offset + TEMPMONINT));
	/* disable trigger SPM interrupt */
	THERMAL_WRAP_WR32(temp & 0x00000000, (offset + TEMPMONINT));
	/* disable trigger SPM interrupt */


	mtktc_printk("final temperature_high: %d, raw_high: %d\n",
				temperature, raw_high);
	mtktc_printk("final temperature_midle: %d, raw_middle: %d\n",
			temperature2, raw_middle);

	/* set protection event: maximum */
	THERMAL_WRAP_WR32(0x10000, (offset + TEMPPROTCTL));

	THERMAL_WRAP_WR32(raw_low, (offset + TEMPPROTTA));
	if (temperature2 > -275000)
		THERMAL_WRAP_WR32(raw_middle, (offset + TEMPPROTTB));
	/* register will remain unchanged if -275000... */

	/* set hot to HOT wakeup event */
	THERMAL_WRAP_WR32(raw_high, (offset + TEMPPROTTC));

	/*trigger cold ,normal and hot interrupt */
	/*Only trigger hot interrupt */
	if (temperature2 > -275000)
		/* enable trigger middle & Hot SPM interrupt */
		THERMAL_WRAP_WR32(temp | 0xC0000000, (offset + TEMPMONINT));
	else
		/* enable trigger Hot SPM interrupt */
		THERMAL_WRAP_WR32(temp | 0x80000000, (offset + TEMPMONINT));
}


static void mtkts_dump_cali_info(void)
{
	pr_info("[cal] g_adc_ge_t      = 0x%x\n", g_adc_ge_t);
	pr_info("[cal] g_adc_oe_t      = 0x%x\n", g_adc_oe_t);
	pr_info("[cal] g_degc_cali     = 0x%x\n", g_degc_cali);
	pr_info("[cal] g_adc_cali_en_t = 0x%x\n", g_adc_cali_en_t);
	pr_info("[cal] g_o_slope       = 0x%x\n", g_o_slope);
	pr_info("[cal] g_o_slope_sign  = 0x%x\n", g_o_slope_sign);
	pr_info("[cal] g_id            = 0x%x\n", g_id);

	pr_info("[cal] g_o_vtsmcu1	= 0x%x\n", g_o_vts[TS_MCU1]);
	pr_info("[cal] g_o_vtsmcu2	= 0x%x\n", g_o_vts[TS_MCU2]);
	pr_info("[cal] g_o_vtsmcu3	= 0x%x\n", g_o_vts[TS_MCU3]);
	pr_info("[cal] g_o_vtsmcu4	= 0x%x\n", g_o_vts[TS_MCU4]);
	pr_info("[cal] g_o_vtsmcu5	= 0x%x\n", g_o_vts[TS_MCU5]);
	pr_info("[cal] g_o_vtsmcu6	= 0x%x\n", g_o_vts[TS_MCU6]);
	pr_info("[cal] g_o_vtsabb		= 0x%x\n", g_o_vts[TS_ABB]);

}

static void thermal_cal_prepare(struct device *dev)
{
	g_adc_ge_t = 512;
	g_adc_oe_t = 512;
	g_degc_cali = 40;//44.75
	g_o_slope = 0;
	g_o_slope_sign = 0;

#if 0
	u32 *buf;
	struct nvmem_cell *cell;
	size_t len;

	cell = nvmem_cell_get(dev, "calibration-data");
	if (IS_ERR(cell)) {
		pr_info("%s, not find calibration-data!!", __func__);
		return;
	}
	buf = (u32 *)nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);
	if (IS_ERR(buf)) {
		mtktc_dprintk("read calibration err!\n");
		return;
	}
	if (len < 5 * sizeof(u32)) {
		dev_err(dev, "invalid calibration data\n");
		goto out;
	}
#endif

	calefuse1 = get_devinfo_with_index(63);
	calefuse2 = get_devinfo_with_index(64);
	calefuse3 = get_devinfo_with_index(65);
	calefuse4 = get_devinfo_with_index(66);

	/* ADC_CALI_EN_T(1b) *(0x11C10184)[0]*/
	g_adc_cali_en_t = (calefuse1 & _BIT_(29)) >> 29;

	if (g_adc_cali_en_t) {
		/* ADC_GE_T [9:0] *(0x11C10184)[9:0]*/
		g_adc_ge_t = (calefuse1 & _BITMASK_(9:0));
			/* ADC_OE_T [9:0] *(0x11C10184)[19:10]*/
		g_adc_oe_t = (calefuse1 & _BITMASK_(19:10)) >> 10;

		g_o_vts[TS_MCU1] = (calefuse2 & _BITMASK_(8:0));
		g_o_vts[TS_MCU2] = (calefuse2 & _BITMASK_(17:9)) >> 9;
		g_o_vts[TS_MCU3] = (calefuse2 & _BITMASK_(26:18)) >> 18;

		g_o_vts[TS_MCU4] = (calefuse4 & _BITMASK_(8:0));
		g_o_vts[TS_MCU5] = (calefuse4 & _BITMASK_(17:9)) >> 9;
		g_o_vts[TS_MCU6] = (calefuse4 & _BITMASK_(26:18)) >> 18;

		g_o_vts[TS_ABB] = (calefuse1 & _BITMASK_(28:20)) >> 20;

		/* DEGC_cali (6b) *(0x11C1018C)[22:27]*/
		g_degc_cali = (calefuse3 & _BITMASK_(5:0));

		/* O_SLOPE_SIGN (1b) *(0x11C10184)[7]*/
		g_o_slope_sign = (calefuse1 & _BIT_(30)) >> 30;

		/* O_SLOPE (6b) *(0x11C1018C)[16:21]*/
		g_o_slope = (calefuse3 & _BITMASK_(11:6)) >> 6;

		/* ID (1b) *(0x11C10184)[10]*/
		g_id = (calefuse1 & _BIT_(31)) >> 31;

		if (g_id == 0)
			g_o_slope = 0;

	} else
		dev_info(dev,
			"Device not calibrated, using default calibration values\n");

	mtkts_dump_cali_info();
}

static void thermal_cal_prepare_2(u32 ret)
{
	s32 format[TS_NUM_MAX] = { 0 };
	int i = 0;

	g_ge = ((g_adc_ge_t - 512) * 10000) / 4096; /* ge * 10000 */
	g_oe = (g_adc_oe_t - 512);
	g_gain = (10000 + g_ge);

	for (i = 0; i < TS_NUM_MAX; i++) {
		format[i] =  (g_o_vts[i] + 3350 - g_oe);
		g_x_roomt[i] = (((format[i] * 10000) / 4096) * 10000)
			/ g_gain;	/* x_roomt * 10000 */
		mtktc_dprintk("[cal] g_x_roomt%d   = 0x%x\n", i, g_x_roomt[i]);
	}

	mtktc_dprintk("[cal] g_ge         = 0x%x\n", g_ge);
	mtktc_dprintk("[cal] g_gain       = 0x%x\n", g_gain);
}


static s32 temperature_to_raw_room(
	u32 ret, enum thermal_sensor sensor_id)
{
	/* Ycurr = [(Tcurr - DEGC_cali/2)*
	 * (1650+O_slope*10)/10*(18/15)*
	 * (1/10000)+X_roomtabb]*Gain*4096 + OE
	 */
	unsigned int raw;
	int temp1, temp2;

	temp1 = 100000 * 15/18 * 1000 * 10;
	temp1 /= 1470 + g_o_slope * 10;//(1470 = 0.001470 * 100000 * 10);
	temp1 /= 4096 - 512 + g_adc_ge_t;
	temp2 = g_degc_cali * 500 - ret;
	raw = (temp2 / temp1) + g_o_vts[sensor_id] + 3350;
	raw &= 0xfff;
	mtktc_dprintk("%s,temp = %d, ts_raw = %d\n",
		__func__, ret, raw);

	return raw;

}


static s32 raw_to_temperature_roomt(u32 ret, enum thermal_sensor sensor_id)
{
	int temp = 0;

	temp = 100000 * 15/18 * 1000 * 10;
	temp /= 4096 - 512 + g_adc_ge_t;
	temp /= 1470 + g_o_slope * 10;//(1470 = 0.001470 * 100000 * 10)
	temp *= ret - g_o_vts[sensor_id] - 3350;
	temp = g_degc_cali * 500 - temp;

	return temp;
}

static void thermal_calibration(void)
{
	if (g_adc_cali_en_t == 0)
		mtktc_printk("#####  Not Calibration  ######\n");
	thermal_cal_prepare_2(0);
}


int get_sensor_tc_temp(int id)
{

	if (id >= TS_MCU1 &&  id <= TS_ABB)
		return SOC_TS_TEMP_T[id];
	else
		return -275000;
}

int get_all_tc_max_temp(void)
{
	int i = 0, j = 0, max_temp = -275000;
	int temp = -275000;

	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++)
		for (j = 0; j < tscpu_g_tc[i].ts_number; j++) {
			temp = SOC_TS_TEMP_T[tscpu_g_tc[i].ts[j]];
			max_temp = max_temp > temp ? max_temp : temp;
		}
	mtktc_printk("%s: max_temp: %d\n", __func__, max_temp);
	return max_temp;

}

int get_immediate_soc_wrap(void)
{
	int bank[] = {TS_MCU1, TS_MCU2, TS_MCU4, TS_MCU5, TS_MCU6};
	int temp = -275000, i;

	for (i = 0; i < ARRAY_SIZE(bank); i++) {
		temp = temp > SOC_TS_TEMP_T[bank[i]] ? temp :
			SOC_TS_TEMP_T[bank[i]];
	}

	return temp;
}

int get_immediate_cpuL_wrap(void)
{
	int bank[] = {TS_MCU1, TS_MCU4, TS_MCU5, TS_MCU6};
	int temp = bank[0], i;

	for (i = 0; i < ARRAY_SIZE(bank); i++) {
		temp = temp > SOC_TS_TEMP_T[bank[i]] ? temp :
			SOC_TS_TEMP_T[bank[i]];
	}

	return temp;
}

int get_immediate_gpu_wrap(void)
{
	return SOC_TS_TEMP_T[TS_MCU2];
}

int get_immediate_cpuB_wrap(void)
{
	return get_all_tc_max_temp();
}


int get_immediate_abb_temp_wrap(void)
{
	return SOC_TS_TEMP_T[TS_ABB];
}

int get_immediate_ts1_wrap(void)
{
	return SOC_TS_TEMP_T[TS_MCU1];
}

int get_immediate_ts2_wrap(void)
{
	return SOC_TS_TEMP_T[TS_MCU2];
}

int get_immediate_ts3_wrap(void)
{
	return SOC_TS_TEMP_T[TS_MCU3];
}

int get_immediate_ts4_wrap(void)
{
	return SOC_TS_TEMP_T[TS_MCU4];
}

int get_immediate_ts5_wrap(void)
{
	return SOC_TS_TEMP_T[TS_MCU5];
}

int get_immediate_ts6_wrap(void)
{
	return SOC_TS_TEMP_T[TS_MCU6];
}

int get_immediate_ts7_wrap(void)
{
	return SOC_TS_TEMP_T[TS_ABB];
}


static int read_tc_raw_and_temp(u32 *tempmsr_name,
				enum thermal_sensor sensor_id)
{
	int temp = 0, raw = 0;

	mtktc_dprintk("%s,tempmsr_name=0x%x,ts_num=%d\n",
		      __func__, *tempmsr_name, sensor_id);

	raw = (tempmsr_name != 0) ?
			(DRV_Reg32((tempmsr_name)) & 0x0fff) : 0;
	temp = (tempmsr_name != 0) ?
		raw_to_temperature_roomt(raw, sensor_id) : 0;

	SOC_TS_TEMP_R[sensor_id] = raw;

	mtktc_dprintk("%s,sensor_id: %d, ts_raw = %d, temp = %d\n",
			__func__, sensor_id, raw, temp);

	return temp;
}

#if 0
static void check_all_temp_valid(void)
{
	int i, j, raw;

	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		for (j = 0; j < tscpu_g_tc[i].ts_number; j++) {
			raw = SOC_TS_TEMP_R[tscpu_g_tc[i].ts[j]];

			if (raw == THERMAL_INIT_VALUE)
				return;	/* The temperature is not valid. */
		}
	}

	g_is_temp_valid = 1;
}

static int mtktc_is_temp_valid(void)
{
	int is_valid = 0;
	unsigned long flags;

	mt_thermal_lock(&flags);
	if (g_is_temp_valid == 0) {
		check_all_temp_valid();
		if (g_is_temp_valid == 0)
			mtktc_dprintk("%s: invalid\n", __func__);
	}

	is_valid = g_is_temp_valid;
	mt_thermal_unlock(&flags);

	return is_valid;
}

#endif

static void mtktc_thermal_read_tc_temp(
	int tc_id, enum thermal_sensor sensor_id, int pnp)
{
	u32 offset;

	mtktc_dprintk("%s tc_num %d type %d order %d\n",
		__func__, tc_id, sensor_id, pnp);
	if (tc_id < THERMAL_CONTROLLER0 ||
		tc_id >= THERMAL_CONTROLLER_NUM)
		return;

	offset = tscpu_g_tc[tc_id].tc_offset;

	switch (pnp) {
	case 0:
		SOC_TS_TEMP_T[sensor_id] =
		    read_tc_raw_and_temp((u32 *)(offset + TEMPMSR0), sensor_id);
		break;
	case 1:
		SOC_TS_TEMP_T[sensor_id] =
		    read_tc_raw_and_temp((u32 *)(offset + TEMPMSR1), sensor_id);
		break;
	case 2:
		SOC_TS_TEMP_T[sensor_id] =
		    read_tc_raw_and_temp((u32 *)(offset + TEMPMSR2), sensor_id);
		break;
	case 3:
		SOC_TS_TEMP_T[sensor_id] =
		    read_tc_raw_and_temp((u32 *)(offset + TEMPMSR3), sensor_id);
		break;
	default:
		SOC_TS_TEMP_T[sensor_id] =
		    read_tc_raw_and_temp((u32 *)(offset + TEMPMSR0), sensor_id);
		break;
	}

	mtktc_dprintk("%s order %d tc_num %d type %d temp %d\n",
			  __func__, pnp, tc_id,
			  sensor_id, SOC_TS_TEMP_T[sensor_id]);
}


static void read_all_temperature(void)
{
	unsigned long flags;
	int i = 0, j = 0;

	mtktc_printk("%s\n", __func__);
	mt_thermal_lock(&flags);
	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++)
		for (j = 0; j < tscpu_g_tc[i].ts_number; j++)
			mtktc_thermal_read_tc_temp(i,
				tscpu_g_tc[i].ts[j], j);
	mt_thermal_unlock(&flags);

	//mtktc_is_temp_valid();// for eem module use
}


/* pause ALL periodoc temperature sensing point */
static void thermal_pause_all_periodoc_temp_sensing(void)
{
	unsigned long flags;
	int i, temp, offset;

	mtktc_printk("%s\n", __func__);
	mt_thermal_lock(&flags);
	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		if (tscpu_g_tc[i].ts_number == 0)
			continue;

		offset = tscpu_g_tc[i].tc_offset;
		temp = DRV_Reg32((offset + TEMPMSRCTL1));
		/* set bit8=bit1=bit2=bit3=1 to pause sensing point 0,1,2,3 */
		DRV_WriteReg32((offset + TEMPMSRCTL1), (temp | 0x10E));
	}
	mt_thermal_unlock(&flags);
}

/* release ALL periodoc temperature sensing point */
static void thermal_release_all_periodoc_temp_sensing(void)
{
	unsigned long flags;
	int i = 0, temp;
	__u32 offset;

	mtktc_printk("%s\n", __func__);
	mt_thermal_lock(&flags);
	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		if (tscpu_g_tc[i].ts_number == 0)
			continue;
		offset = tscpu_g_tc[i].tc_offset;
		temp = DRV_Reg32((offset + TEMPMSRCTL1));
		/* set bit1=bit2=bit3=bit8=0 to release sensing point 0,1,2,3*/
		DRV_WriteReg32((offset + TEMPMSRCTL1), ((temp & (~0x10E))));
	}
	mt_thermal_unlock(&flags);
}

/* disable ALL periodoc temperature sensing point */
static void thermal_disable_all_periodoc_temp_sensing(void)
{
	unsigned long flags;
	int i = 0, offset;

	mtktc_printk("%s\n", __func__);
	mt_thermal_lock(&flags);
	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		if (tscpu_g_tc[i].ts_number == 0)
			continue;
		offset = tscpu_g_tc[i].tc_offset;
		THERMAL_WRAP_WR32(0x00000000, (offset + TEMPMONCTL0));
	}
	mt_thermal_unlock(&flags);
}

static void tscpu_clear_all_temp(void)
{
	int i = 0;

	for (i = 0; i < TS_NUM_MAX; i++) {
		SOC_TS_TEMP_T[i] = 0;
		SOC_TS_TEMP_R[i] = 0;
	}
}

void tc_ipi_send_sspm_thermal_suspend_resume(int is_suspend)
{
	struct thermal_ipi_data thermal_data;

	thermal_data.u.data.arg[0] = is_suspend;
	thermal_data.u.data.arg[1] = 0;
	thermal_data.u.data.arg[2] = 0;
	while (thermal_to_mcupm(THERMAL_IPI_SUSPEND_RESUME_NOTIFY,
		&thermal_data) != 0)
		udelay(100);
}


/*mtktc_thermal_suspend spend 1000us~1310us*/
static int mtktc_thermal_suspend(struct platform_device *dev,
				 pm_message_t state)
{
	int cnt = 0;
	int temp = 0;
#if THERMAL_PERFORMANCE_PROFILE
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtktc_printk("%s, talking_flag=%d\n",
			__func__, talking_flag);

	g_tc_resume = 1; /* set "1", don't read temp during suspend */

	if (talking_flag == false) {
		mtktc_dprintk("%s no talking\n", __func__);

		//tc_ipi_send_sspm_thermal_suspend_resume(1);
		/* disable periodic temp measurement on sensor 0~3 */
		thermal_disable_all_periodoc_temp_sensing(); /* TEMPMONCTL0 */
		while (cnt < 50) {
			temp = (DRV_Reg32(THAHBST0) >> 16);
			if (cnt > 10)
				mtktc_printk(KERN_CRIT
					     "THAHBST0 = 0x%x,cnt=%d, %d\n",
					     temp, cnt, __LINE__);
			if (temp == 0x0)
				break;
			udelay(2);
			cnt++;
		}
		mtktc_thermal_clock_off();
		/*TSCON1[7:6]=2'b11, Buffer off */
		/* turn off the sensor buffer to save power */
		thermal_buffer_turn_off();
	}
#if THERMAL_PERFORMANCE_PROFILE
	do_gettimeofday(&end);
	/* Get milliseconds */
	mtktc_printk("suspend time spent, sec : %lu , usec : %lu\n",
		     (end.tv_sec - begin.tv_sec),
		     (end.tv_usec - begin.tv_usec));
#endif

	return 0;
}


static int mtktc_thermal_resume(struct platform_device *dev)
{
	int temp = 0;
	int cnt = 0;
#if THERMAL_PERFORMANCE_PROFILE
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtktc_printk("%s,talking_flag=%d\n", __func__, talking_flag);
	g_tc_resume = 1; /* set "1", don't read temp during start resume */

	if (talking_flag == false) {
		mtktc_reset_thermal();
		mtktc_thermal_clock_on();

		thermal_buffer_turn_on();

		mtktc_fast_initial_sw_workaround();
		thermal_disable_all_periodoc_temp_sensing(); /* TEMPMONCTL0 */
		while (cnt < 50) {
			temp = (DRV_Reg32(THAHBST0) >> 16);
			if (cnt > 10)
				mtktc_printk(KERN_CRIT
					     "THAHBST0 = 0x%x,cnt=%d, %d\n",
					     temp, cnt, __LINE__);
			if (temp == 0x0)
				break;
			udelay(2);
			cnt++;
		}
		thermal_initial();
		tscpu_clear_all_temp();
		read_all_temperature();
		/*mtktc_config_all_tc_hw_protect(tc_high_trip, tc_mid_trip);*/
		//tc_ipi_send_sspm_thermal_suspend_resume(0);
	}

	g_tc_resume = 2; /* set "2", resume finish,can read temp */

#if THERMAL_PERFORMANCE_PROFILE
	do_gettimeofday(&end);

	/* Get milliseconds */
	mtktc_printk("resume time spent, sec : %lu , usec : %lu\n",
				 (end.tv_sec - begin.tv_sec),
				 (end.tv_usec - begin.tv_usec));
#endif

	return 0;
}

static void mtktc_thermal_ADCPNP(
	int tc_id, enum thermal_sensor sensor_id, int pnp)
{
	u32 offset;

	if (tc_id < THERMAL_CONTROLLER0 ||
		tc_id >= THERMAL_CONTROLLER_NUM)
		return;

	offset = tscpu_g_tc[tc_id].tc_offset;

	mtktc_dprintk("%s adc %x, order %d\n", __func__, sensor_id, pnp);

	switch (pnp) {
	case 0:
		THERMAL_WRAP_WR32(sensor_id, (offset + TEMPADCPNP0));
		break;
	case 1:
		THERMAL_WRAP_WR32(sensor_id, (offset + TEMPADCPNP1));
		break;
	case 2:
		THERMAL_WRAP_WR32(sensor_id, (offset + TEMPADCPNP2));
		break;
	case 3:
		THERMAL_WRAP_WR32(sensor_id, (offset + TEMPADCPNP3));
		break;
	default:
		THERMAL_WRAP_WR32(sensor_id, (offset + TEMPADCPNP0));
		break;
	}

}


static void thermal_initial(void)
{
	unsigned long flags;
	u32 temp = 0, offset;
	int i = 0, j = 0;

	mtktc_thermal_clock_on();

	mt_thermal_lock(&flags);

	/* AuxADC Initialization,ref MT6592_AUXADC.doc  TODO: check this line */
	temp = DRV_Reg32(AUXADC_CON0_V);
	/* Auto set enable for CH11 */
	temp &= 0xFFFFF7FF;
	/* 0: Not AUTOSET mode */
	THERMAL_WRAP_WR32(temp, AUXADC_CON0_V);
	/* disable auxadc channel 11 synchronous mode */
	THERMAL_WRAP_WR32(0x800, AUXADC_CON1_CLR_V);
	/* disable auxadc channel 11 immediate mode */

	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		if (tscpu_g_tc[i].ts_number == 0)
			continue;

		offset = tscpu_g_tc[i].tc_offset;
		thermal_reset_and_initial(i);

		for (j = 0; j < tscpu_g_tc[i].ts_number; j++)
			mtktc_thermal_ADCPNP(i, tscpu_g_tc[i].ts[j], j);

		THERMAL_WRAP_WR32(TS_CON1_P, (offset + TEMPPNPMUXADDR));
		/* AHB address for pnp sensor mux selection */
		THERMAL_WRAP_WR32(0x3, (offset + TEMPADCWRITECTRL));
	}

	/* enable auxadc channel 11 immediate mode */
	THERMAL_WRAP_WR32(0x800, AUXADC_CON1_SET_V);

	/* enable and start sample */
	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		if (tscpu_g_tc[i].ts_number == 0)
			continue;
		offset = tscpu_g_tc[i].tc_offset;
		temp = _BITMASK_((tscpu_g_tc[i].ts_number - 1) : 0);
		THERMAL_WRAP_WR32(temp, (offset + TEMPMONCTL0));
	}

	mt_thermal_unlock(&flags);
}

void mtktc_config_all_tc_hw_protect(int temperature, int temperature2)
{
	unsigned long flags;
	int i;
#if THERMAL_PERFORMANCE_PROFILE
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtktc_dprintk(
		"%s,temperature=%d,temperature2=%d,\n",
		__func__, temperature, temperature2);

/* this api maybe called by other module,
 * so we need update high and mid trip for next suspend and resume.
 */

	tc_high_trip = temperature;
	tc_mid_trip = temperature2;

	mt_thermal_lock(&flags);

	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		if (tscpu_g_tc[i].ts_number == 0)
			continue;
		set_tc_trigger_hw_protect(temperature,
			temperature2, i);
		/* Move thermal HW protection ahead... */
	}

	mt_thermal_unlock(&flags);

#if THERMAL_PERFORMANCE_PROFILE
	do_gettimeofday(&end);

	/* Get milliseconds */
	mtktc_printk("resume time spent, sec : %lu , usec : %lu\n",
		     (end.tv_sec - begin.tv_sec),
		     (end.tv_usec - begin.tv_usec));
#endif

}

static void mtktc_reset_thermal(void)
{
	int temp = 0;

	mtktc_printk("%s\n", __func__);
	/* reset thremal ctrl */

	temp = DRV_Reg32(PERI_GLOBALCON_RST0);
	/* 1: Enables thermal control software reset */
	temp |= 0x00200000;
	DRV_WriteReg32(PERI_GLOBALCON_RST0, temp);

	udelay(10);//need >=10us

	//un reset
	temp = DRV_Reg32(PERI_GLOBALCON_RST0);
	/* 0: clear thermal control software reset */
	temp &= ~(unsigned int)(0x00200000);
	DRV_WriteReg32(PERI_GLOBALCON_RST0, temp);

}

static void mtktc_fast_initial_sw_workaround(void)
{
	unsigned long flags;
	unsigned int i = 0;
	/* mtktc_printk("%s\n", __func__); */

	/* mtktc_thermal_clock_on(); */
	mt_thermal_lock(&flags);
	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++) {
		if (tscpu_g_tc[i].ts_number == 0)
			continue;
		thermal_fast_init(i);
	}
	//g_is_temp_valid = 0;
	mt_thermal_unlock(&flags);
}

#if TIMER_V1

static void start_tempinfo_update_timer(void)
{

	if (hrtimer_flag != -1)
		return;

	if (read_curr_temp > fast_polling_trip_temp) {
		hrtimer_start(&ts_tempinfo_hrtimer,
			ktime_set(0, 50 * 1000000), HRTIMER_MODE_REL);
		hrtimer_flag = 4;
	} else {
		add_timer(&normal_timer);
		hrtimer_flag = 0;
	}
	mtktc_dprintk("%s: hrtimer_flag:%d, curr_temp:%d\n",
		__func__, hrtimer_flag, read_curr_temp);
}

static void cancel_tempinfo_update_timer(void)
{
	unsigned long flags;


	spin_lock_irqsave(&tempinfo_timer_lock, flags);

	if (hrtimer_flag == 4)
		hrtimer_cancel(&ts_tempinfo_hrtimer);
	else if (hrtimer_flag == 0)
		del_timer(&normal_timer);
	else {
		hrtimer_cancel(&ts_tempinfo_hrtimer);
		del_timer(&normal_timer);
	}
	hrtimer_flag = -1;
	mtktc_dprintk("########### %s\n", __func__);
	spin_unlock_irqrestore(&tempinfo_timer_lock, flags);
}

#else

static void start_tempinfo_update_timer(void)
{

	if (hrtimer_flag != -1)
		return;

	if (read_curr_temp > fast_polling_trip_temp) {
		hrtimer_start(&ts_tempinfo_hrtimer,
			ktime_set(0, 50 * 1000000), HRTIMER_MODE_REL);
		hrtimer_flag = 2;
	} else {
		add_timer(&normal_timer);
		hrtimer_flag = 1;
	}
	mtktc_dprintk("%s: hrtimer_flag:%d, curr_temp:%d\n",
		__func__, hrtimer_flag, read_curr_temp);
}

static void cancel_tempinfo_update_timer(void)
{
	unsigned long flags;

	if (hrtimer_flag == -1)
		return;

	spin_lock_irqsave(&tempinfo_timer_lock, flags);
	if (hrtimer_flag == 2)
		hrtimer_cancel(&ts_tempinfo_hrtimer);
	else if (hrtimer_flag == 1)
		del_timer(&normal_timer);

	hrtimer_flag = -1;
	mtktc_dprintk("########### %s\n", __func__);
	spin_unlock_irqrestore(&tempinfo_timer_lock, flags);
}

#endif


static enum hrtimer_restart update_tempinfo_timer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&tempinfo_timer_lock, flags);

	if (hrtimer_flag == -1) {
		spin_unlock_irqrestore(&tempinfo_timer_lock, flags);
		return HRTIMER_NORESTART;
	}

#ifdef TIMER_V1
	if (read_curr_temp > fast_polling_trip_temp) {
		if (hrtimer_flag == 3) {
			hrtimer_forward_now(&ts_tempinfo_hrtimer,
				ktime_set(0, temp_update_interval * 1000000));
			del_timer(&normal_timer);
			hrtimer_flag = 4;
			mtktc_dprintk(" %s: hrtimer_flag:%d, curr_temp:%d\n",
				__func__, hrtimer_flag, read_curr_temp);
		} else if (hrtimer_flag == 4) {
			hrtimer_forward_now(&ts_tempinfo_hrtimer,
				ktime_set(0, temp_update_interval * 1000000));
			mtktc_dprintk(" %s: hrtimer_flag:%d, curr_temp:%d\n",
				__func__, hrtimer_flag, read_curr_temp);
		} else if (hrtimer_flag < 3) {
			hrtimer_start(&ts_tempinfo_hrtimer,
				ktime_set(0, temp_update_interval * 1000000),
				HRTIMER_MODE_REL);
			hrtimer_flag = 3;
			mtktc_dprintk(" %s: hrtimer_flag:%d, curr_temp:%d\n",
				__func__, hrtimer_flag, read_curr_temp);
		}
		spin_unlock_irqrestore(&tempinfo_timer_lock, flags);
	} else {
		mtktc_dprintk(" %s: hrtimer_flag:%d, curr_temp:%d\n",
			__func__, hrtimer_flag, read_curr_temp);
		mod_timer(&normal_timer,
			jiffies + msecs_to_jiffies(temp_update_interval));
		if (hrtimer_flag > 2) {
			hrtimer_flag = 2;
			mtktc_dprintk(" %s: hrtimer_flag:%d, curr_temp:%d\n",
				__func__, hrtimer_flag, read_curr_temp);
		} else if (hrtimer_flag == 2) {
			hrtimer_cancel(&ts_tempinfo_hrtimer);
			hrtimer_flag = 0;
			mtktc_dprintk(" %s: hrtimer_flag:%d, curr_temp:%d\n",
				__func__, hrtimer_flag, read_curr_temp);
		}
		spin_unlock_irqrestore(&tempinfo_timer_lock, flags);
		return HRTIMER_NORESTART;
	}
	return HRTIMER_RESTART;

#else

	if (read_curr_temp > fast_polling_trip_temp) {
		if (hrtimer_flag == 1) {
			hrtimer_start(&ts_tempinfo_hrtimer,
				ktime_set(0, temp_update_interval * 1000000),
				HRTIMER_MODE_REL);
			del_timer(&normal_timer);
			hrtimer_flag = 2;
		} else if (hrtimer_flag == 2)
			hrtimer_forward_now(&ts_tempinfo_hrtimer,
				ktime_set(0, temp_update_interval * 1000000));

		mtktc_dprintk(" %s: hrtimer_flag:%d, curr_temp:%d\n",
				__func__, hrtimer_flag, read_curr_temp);
		spin_unlock_irqrestore(&tempinfo_timer_lock, flags);
	} else {
		if (hrtimer_flag == 2) {
			hrtimer_cancel(&ts_tempinfo_hrtimer);
			add_timer(&normal_timer);
			hrtimer_flag = 1;
		} else if (hrtimer_flag == 1)
			mod_timer(&normal_timer,
				jiffies +
				msecs_to_jiffies(temp_update_interval));

		mtktc_dprintk(" %s: hrtimer_flag:%d, curr_temp:%d\n",
				__func__, hrtimer_flag, read_curr_temp);
		spin_unlock_irqrestore(&tempinfo_timer_lock, flags);
		return HRTIMER_NORESTART;
	}
	return HRTIMER_RESTART;

#endif
}


DEFINE_PER_CPU(int, thermal_percpu_load);

static unsigned int _get_percpu_load(int cpu)
{
#ifdef CONFIG_MTK_SCHED_RQAVG_US
	return sched_get_percpu_load(cpu, 1, 0);
#else
	return 100;
#endif
}

//num_possible_cpus()

int _get_avg_loading(void)
{
	unsigned int cpu;
	int total_loading = 0;
	int avg_loading = 0;

	for_each_possible_cpu(cpu) {
		per_cpu(thermal_percpu_load, cpu) =	_get_percpu_load(cpu);
		total_loading += per_cpu(thermal_percpu_load, cpu);
	}

	if (num_possible_cpus() <= 0)
		return avg_loading;
	//avg_loading = (total_loading * 100) / num_possible_cpus();
	avg_loading = total_loading  / num_possible_cpus();
	pr_info("current system loading: %d	total_core : %d\n",
		avg_loading, num_possible_cpus());

	return avg_loading;
}


static enum hrtimer_restart mtktc_update_tempinfo(struct hrtimer *timer)
{
	static int resume_ok;
	//int curr_temp;

	//int total_power  = 0;
	//int avg_loading = 0;

	mtktc_printk("mtktc_update_tempinfo\n");
	if (g_tc_resume == 0) {
		read_all_temperature();
		if (resume_ok) {
			mtktc_config_all_tc_hw_protect(tc_high_trip,
						       tc_mid_trip);
			resume_ok = 0;
		}
	} else if (g_tc_resume == 2) { /* resume ready */

		/* mtktc_printk("%s g_tc_resume==2\n", __func__); */
		g_tc_resume = 0;
		resume_ok = 1;
	}

	read_curr_temp = get_all_tc_max_temp();

#if 0 //maybe  should in mtktscpu
	//######## total_power = _get_current_cpu_power();

#ifdef CONFIG_MTK_GPU_SUPPORT
	//########## total_power = _get_current_cpu_power()
				+ _get_current_gpu_power(void);
#endif
	avg_loading = _get_avg_loading();
#endif

	if (read_curr_temp < polling_trip_temp2)
		temp_update_interval =
			NORMAL_TEMP_POLL * polling_factor2;
	else if (read_curr_temp < polling_trip_temp1)
		temp_update_interval =
			NORMAL_TEMP_POLL * polling_factor1;

	else if (read_curr_temp >= fast_polling_trip_temp)
		temp_update_interval =
			NORMAL_TEMP_POLL / fast_polling_factor;

	else if (read_curr_temp >= polling_trip_temp0)
		temp_update_interval =
			NORMAL_TEMP_POLL / polling_factor0;
	else
		temp_update_interval = NORMAL_TEMP_POLL;

	return update_tempinfo_timer();
}

static void tempinfo_hrtimer_init(void)
{
	mtktc_dprintk("%s\n", __func__);

	hrtimer_init(&ts_tempinfo_hrtimer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	ts_tempinfo_hrtimer.function = mtktc_update_tempinfo;
}

static void tempinfo_normal_timer_init(void)
{
	mtktc_printk("%s\n", __func__);

	init_timer_deferrable(&normal_timer);
	normal_timer.function = (void *)&mtktc_update_tempinfo;
	normal_timer.data = (unsigned long)&normal_timer;
	normal_timer.expires = jiffies + msecs_to_jiffies(temp_update_interval);
}

static void mtktc_update_temperature_timer_init(void)
{
	tempinfo_hrtimer_init();
	tempinfo_normal_timer_init();
	start_tempinfo_update_timer();
}

#if THERMAL_TURNOFF_AUXADC_BEFORE_DEEPIDLE
/*&#include <mach/mt_clkmgr.h>*/
/* #define MT_PDN_PERI_AUXADC MT_CG_INFRA_AUXADC */

static void mtktc_pause_tc(void)
{
	int cnt = 0;
	int temp = 0;

	mtktc_printk("%s\n", __func__);
	g_tc_resume = 1; /* set "1", don't read temp during suspend */

	if (talking_flag == false)
	{
		thermal_pause_all_periodoc_temp_sensing();
		while (cnt < 50) {
			temp = (DRV_Reg32(THAHBST0) >> 16);
			if (cnt > 10)
				mtktc_printk(KERN_CRIT
					     "THAHBST0 = 0x%x,cnt=%d, %d\n",
					     temp, cnt, __LINE__);
			if (temp == 0x0)
				break;

			udelay(2);
			cnt++;
		}
	}
}

static void mtktc_release_tc(void)
{
	int temp = 0;
	int cnt = 0;

	mtktc_printk("%s\n", __func__);
	g_tc_resume = 1; /* set "1", don't read temp during start resume */
	if (talking_flag == false)
	{
		thermal_release_all_periodoc_temp_sensing();
		while (cnt < 50) {
			temp = (DRV_Reg32(THAHBST0) >> 16);
			if (cnt > 10)
				mtktc_printk(KERN_CRIT
					     "THAHBST0 = 0x%x,cnt=%d, %d\n",
					     temp, cnt, __LINE__);
			if (temp == 0x1)
				break;
			udelay(2);
			cnt++;
		}
	}
	g_tc_resume = 2; /* set "2", resume finish,can read temp */
}
#endif


static void mtktc_start_thermal_timer(void)
{
#if THERMAL_TURNOFF_AUXADC_BEFORE_DEEPIDLE
	mtktc_thermal_clock_on();
	udelay(15);
#if defined(CONFIG_MTK_CLKMGR)
	if ((clock_is_on(MT_CG_INFRA_AUXADC) == 0x0))
		mtktc_printk("hwEnableClock AUXADC failed\n");
#endif
	mtktc_release_tc();
#endif
	start_tempinfo_update_timer();
}

static void mtktc_cancel_thermal_timer(void)
{
	cancel_tempinfo_update_timer();
#if THERMAL_TURNOFF_AUXADC_BEFORE_DEEPIDLE
	mtktc_pause_tc();
	mtktc_thermal_clock_off();
#if defined(CONFIG_MTK_CLKMGR)
	if (clock_is_on(MT_CG_INFRA_AUXADC))
		mtktc_printk("hwEnableClock AUXADC still on\n");
#endif

#endif

}

static int thermal_fast_init(int tc_num)
{
	u32 temp = 0, cunt = 0, offset = 0;

	mtktc_printk("%s\n", __func__);
	if (tc_num < THERMAL_CONTROLLER0 ||
		tc_num >= THERMAL_CONTROLLER_NUM)
		return -1;

	offset = tscpu_g_tc[tc_num].tc_offset;

	temp = THERMAL_INIT_VALUE;
	DRV_WriteReg32(PTPSPARE2, (0x00001000 + temp));
	/* write temp to spare register */

	DRV_WriteReg32((offset + TEMPMONCTL1), 1);
	/* counting unit is 320 * 31.25us = 10ms */
	DRV_WriteReg32((offset + TEMPMONCTL2), 1);
	/* sensing interval is 200 * 10ms = 2000ms */
	DRV_WriteReg32((offset + TEMPAHBPOLL), 1);
	/* polling interval to check if temperature sense is ready */

	DRV_WriteReg32((offset + TEMPAHBTO), 0x000000FF);
	/* exceed this polling time, IRQ would be inserted */
	//DRV_WriteReg32(TEMPMONIDET0, 0x00000000);
	/* times for interrupt occurrance */
	DRV_WriteReg32((offset + TEMPMONIDET1), 0x00000000);
	/* times for interrupt occurrance */

	DRV_WriteReg32((offset + TEMPMSRCTL0), 0x0000000);
	/* temperature measurement sampling control */
	/* this value will be stored to TEMPPNPMUXADDR (TEMPSPARE0)
	 * automatically by hw
	 */
	DRV_WriteReg32((offset + TEMPADCPNP0), 0x0);
	DRV_WriteReg32((offset + TEMPADCPNP1), 0x1);
	DRV_WriteReg32((offset + TEMPADCPNP2), 0x2);
	DRV_WriteReg32((offset + TEMPADCPNP3), 0x3);

	DRV_WriteReg32((offset + TEMPPNPMUXADDR), (u32)PTPSPARE0_P);
	/* AHB address for pnp sensor mux selection */
	DRV_WriteReg32((offset + TEMPADCMUXADDR), (u32)PTPSPARE0_P);
	/* AHB address for auxadc mux selection */
	DRV_WriteReg32((offset + TEMPADCENADDR), (u32)PTPSPARE1_P);
	/* AHB address for auxadc enable */
	DRV_WriteReg32((offset + TEMPADCVALIDADDR), (u32)PTPSPARE2_P);
	/* AHB address for auxadc valid bit */
	DRV_WriteReg32((offset + TEMPADCVOLTADDR), (u32)PTPSPARE2_P);
	/* AHB address for auxadc voltage output */
	DRV_WriteReg32((offset + TEMPRDCTRL), 0x0);
	/* read valid & voltage are at the same register */
	/* indicate where the valid bit is (the 12th bit is valid bit and 1 is
	 * valid)
	 */
	DRV_WriteReg32((offset + TEMPADCVALIDMASK), 0x0000002C);
	DRV_WriteReg32((offset + TEMPADCVOLTAGESHIFT), 0x0);
	/* do not need to shift */

	DRV_WriteReg32((offset + TEMPADCWRITECTRL),	0x3);
	/* enable auxadc mux & pnp write transaction */

	/* enable all interrupt except filter sense and immediate sense
	 * interrupt
	 */
	DRV_WriteReg32((offset + TEMPMONINT), 0x00000000);

	DRV_WriteReg32((offset + TEMPMONCTL0), 0x0000000F);
	/* enable all sensing point */

	cunt = 0;
	temp = DRV_Reg32((offset + TEMPMSR0)) & 0x0fff;
	while (temp != THERMAL_INIT_VALUE && cunt < 20) {
		cunt++;
		mtktc_dprintk("[Power/CPU_Thermal]0 temp=%d,cunt=%d\n", temp,
			      cunt);
		temp = DRV_Reg32((offset + TEMPMSR0)) & 0x0fff;
	}

	cunt = 0;
	temp = DRV_Reg32((offset + TEMPMSR1)) & 0x0fff;
	while (temp != THERMAL_INIT_VALUE && cunt < 20) {
		cunt++;
		mtktc_dprintk("[Power/CPU_Thermal]1 temp=%d,cunt=%d\n", temp,
			      cunt);
		temp = DRV_Reg32((offset + TEMPMSR1)) & 0x0fff;
	}

	cunt = 0;
	temp = DRV_Reg32((offset + TEMPMSR2)) & 0x0fff;
	while (temp != THERMAL_INIT_VALUE && cunt < 20) {
		cunt++;
		mtktc_dprintk("[Power/CPU_Thermal]2 temp=%d,cunt=%d\n", temp,
			      cunt);
		temp = DRV_Reg32((offset + TEMPMSR2)) & 0x0fff;
	}

	cunt = 0;
	temp = DRV_Reg32((offset + TEMPMSR3)) & 0x0fff;
	while (temp != THERMAL_INIT_VALUE && cunt < 20) {
		cunt++;
		mtktc_dprintk("[Power/CPU_Thermal]3 temp=%d,cunt=%d\n", temp,
			      cunt);
		temp = DRV_Reg32((offset + TEMPMSR3)) & 0x0fff;
	}

	return 0;
}

#ifdef CONFIG_OF

static u64 of_get_phys_base(struct device_node *np)
{
	u64 size64;
	const __be32 *regaddr_p;

	regaddr_p = of_get_address(np, 0, &size64, NULL);
	if (!regaddr_p)
		return OF_BAD_ADDR;

	return of_translate_address(np, regaddr_p);
}
#endif

#ifdef CONFIG_OF
const long tscpu_dev_alloc_module_base_by_name(const char *name)
{
	unsigned long VA;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	if (!node) {
		mtktc_printk("find node failed\n");
		return 0;
	}
	VA = (unsigned long)of_iomap(node, 0);
	mtktc_printk("DEV: VA(%s): 0x%lx\n", name, VA);

	return VA;
}
#endif

static int get_io_reg_base(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;

	/* Setup IO addresses */
	thermal_base = of_iomap(node, 0);
	mtktc_printk("[THERM_CTRL] thermal_base=0x%lx\n",
		     (unsigned long)thermal_base);

	/* get thermal phy base */
	thermal_phy_base = of_get_phys_base(node);
	mtktc_printk("[THERM_CTRL] thermal_phy_base=0x%lx\n",
		     (unsigned long)thermal_phy_base);

	/* get thermal irq num */
	thermal_irq_number = irq_of_parse_and_map(node, 0);
	mtktc_printk("[THERM_CTRL] thermal_irq_number=%d\n",
		     thermal_irq_number);

	node = of_find_compatible_node(NULL, NULL,
		"mediatek,mt8696-apmixedsys");
	apmixed_base = of_iomap(node, 0);
	mtktc_printk("[THERM_CTRL] apmixed_base=0x%lx\n",
		     (unsigned long)apmixed_base);

	apmixed_phy_base = of_get_phys_base(node);
	mtktc_printk("[THERM_CTRL] apmixed_phy_base=0x%lx\n",
		     (unsigned long)apmixed_phy_base);

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-auxadc");

	auxadc_ts_base = of_iomap(node, 0);

	mtktc_printk("auxadc_base=0x%lx\n", (unsigned long)auxadc_ts_base);

	auxadc_ts_phy_base = of_get_phys_base(node);
	mtktc_printk("[THERM_CTRL] auxadc_ts_phy_base=0x%lx\n",
		     (unsigned long)auxadc_ts_phy_base);

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-infrasys");

	INFRACFG_AO_BASE = of_iomap(node, 0);

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-pericfg");

	PERICFG_AO_BASE = of_iomap(node, 0);

	mtktc_printk("[THERM_CTRL] INFRACFG_AO_BASE=0x%lx\n",
		     (unsigned long)INFRACFG_AO_BASE);

	return 1;
}

void thermal_buffer_turn_on(void)
{
	int temp = 0;

	temp = DRV_Reg32(TS_CON1);
	pr_info("Before write the register TS_CON1: 0x%x\n",
		DRV_Reg32(TS_CON1));

	temp &= ~(0x000000030);
	/* TS_CON0[29:28]=2'b00,   00: Buffer on, TSMCU to AUXADC */
	THERMAL_WRAP_WR32(temp, TS_CON1);
	udelay(200);
	pr_info("After write the register TS_CON1: 0x%x\n",
		DRV_Reg32(TS_CON1));
}

void thermal_buffer_turn_off(void)
{
	int temp;

	temp = DRV_Reg32(TS_CON1);
	//temp &= 0xFFF03FFF;

	temp |= 0x000000030;
	/*00: Buffer on, TSMCU to AUXADC */
	THERMAL_WRAP_WR32(temp, TS_CON1);
	udelay(200);
	mtktc_dprintk("%s :TS_CON1=0x%x\n", __func__, temp);
}


static int mtktc_read_cal(struct seq_file *m, void *v)
{

	int i = 0;

	seq_printf(m, "calibration calefuse1=0x%x, calefuse2=0x%x\n",
		      calefuse1, calefuse2);
	seq_printf(m, "calibration calefuse3=0x%x, calefuse4=0x%x\n",
		      calefuse3, calefuse4);

	seq_printf(m, "[cal] g_adc_ge_t      = 0x%x\n", g_adc_ge_t);
	seq_printf(m, "[cal] g_adc_oe_t      = 0x%x\n", g_adc_oe_t);
	seq_printf(m, "[cal] g_degc_cali     = 0x%x\n", g_degc_cali);
	seq_printf(m, "[cal] g_adc_cali_en_t = 0x%x\n", g_adc_cali_en_t);
	seq_printf(m, "[cal] g_o_slope       = 0x%x\n", g_o_slope);
	seq_printf(m, "[cal] g_o_slope_sign  = 0x%x\n", g_o_slope_sign);
	seq_printf(m, "[cal] g_id            = 0x%x\n", g_id);

	seq_printf(m, "[cal] g_o_vtsmcu1	= 0x%x\n", g_o_vts[TS_MCU1]);
	seq_printf(m, "[cal] g_o_vtsmcu2	= 0x%x\n", g_o_vts[TS_MCU2]);
	seq_printf(m, "[cal] g_o_vtsmcu3	= 0x%x\n", g_o_vts[TS_MCU3]);
	seq_printf(m, "[cal] g_o_vtsmcu4	= 0x%x\n", g_o_vts[TS_MCU4]);
	seq_printf(m, "[cal] g_o_vtsmcu5	= 0x%x\n", g_o_vts[TS_MCU5]);
	seq_printf(m, "[cal] g_o_vtsmcu6	= 0x%x\n", g_o_vts[TS_MCU6]);
	seq_printf(m, "[cal] g_o_vtsabb		= 0x%x\n", g_o_vts[TS_ABB]);

	seq_printf(m, "[cal] g_ge         = 0x%x\n", g_ge);
	seq_printf(m, "[cal] g_gain       = 0x%x\n", g_gain);

	for (i = 0; i < TS_NUM_MAX; i++)
		seq_printf(m, "[cal] g_x_roomt%d = 0x%x\n", i, g_x_roomt[i]);
	seq_puts(m, "\n");

	for (i = 0; i < TS_NUM_MAX; i++)
		seq_printf(m, "SOC_TS_TEMP_T[%d] = %d ", i, SOC_TS_TEMP_T[i]);
	seq_puts(m, "\n");

	for (i = 0; i < TS_NUM_MAX; i++)
		seq_printf(m, "SOC_TS_TEMP_R[%d] = %d ", i, SOC_TS_TEMP_R[i]);
	seq_puts(m, "\n");

	return 0;
}

static int mtktc_cal_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtktc_read_cal, NULL);
}

static const struct file_operations mtktc_cal_fops = {
	.owner = THIS_MODULE,
	.open = mtktc_cal_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t mtktc_write_log(
	struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	char desc[32];
	int log_switch;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (len < 0 || len >= 32)
		return -EINVAL;

	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	/*if (sscanf(desc, "%d", &log_switch) == 1)*/
	if (kstrtoint(desc, 10, &log_switch) == 0) {
		mtktc_debug_log = log_switch;
		return count;
	}
	mtktc_printk("%s bad argument\n", __func__);

	return -EINVAL;
}


static int mtktc_read_log(struct seq_file *m, void *v)
{
	seq_printf(m, "[ %s] log = %d\n", __func__, mtktc_debug_log);
	return 0;
}

static int mtktc_open_log(struct inode *inode, struct file *file)
{
	return single_open(file, mtktc_read_log, NULL);
}

static const struct file_operations mtktc_log_fops = {
	.owner = THIS_MODULE,
	.open = mtktc_open_log,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtktc_write_log,
	.release = single_release,
};


#if THERMAL_ENABLE_TINYSYS_SSPM || THERMAL_ENABLE_ONLY_TZ_SSPM
void tc_ipi_send_efuse_data(void)
{
	struct thermal_ipi_data thermal_data;

	mtktc_printk("%s\n", __func__);

	thermal_data.u.data.arg[0] = g_golden_temp;
	thermal_data.u.data.arg[1] = 0;
	thermal_data.u.data.arg[2] = 0;
	while (thermal_to_mcupm(THERMAL_IPI_LVTS_INIT_GRP1, &thermal_data) != 0)
		udelay(100);
}
#endif


static int mtktc_thermal_probe(struct platform_device *pdev)
/*static int __init mtktc_init(void)*/
{
	int err = 0;
	int temp = 0;
	int cnt = 0;
	int i = 0;
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktscpu_dir = NULL;

	mtktc_printk("mtktc_thermal_probe\n");

	clk_peri_therm = devm_clk_get(&pdev->dev, "therm");
	WARN_ON(IS_ERR(clk_peri_therm));

	clk_auxadc = devm_clk_get(&pdev->dev, "auxadc");
	WARN_ON(IS_ERR(clk_auxadc));

#ifdef CONFIG_OF
	if (get_io_reg_base(pdev) == 0)
		return 0;
#endif

	for (i = 0; i < ARRAY_SIZE(tscpu_g_tc); i++)
		tscpu_g_tc[i].addr = thermal_base + tscpu_g_tc[i].tc_offset;

	thermal_cal_prepare(&pdev->dev);
	thermal_calibration();

	mtktc_thermal_clock_on();
	mtktc_reset_thermal();

	thermal_buffer_turn_on();

/* ####################################### */
/* for thermal controller setting start*/

	mtktc_fast_initial_sw_workaround();
	thermal_disable_all_periodoc_temp_sensing();

	while (cnt < 50) {
		temp = (DRV_Reg32(THAHBST0) >> 16);
		if (cnt > 10)
			mtktc_dprintk(KERN_CRIT "THAHBST0 = 0x%x,cnt=%d, %d\n",
				      temp, cnt, __LINE__);
		if (temp == 0x0)
			break;
		udelay(2);
		cnt++;
	}

	/*Normal initial */
	thermal_initial();

/* for thermal controller setting end*/
/* ####################################### */

	read_all_temperature();
	mtktc_update_temperature_timer_init();

#ifdef CONFIG_OF
	err = request_irq(thermal_irq_number,
			tscpu_thermal_all_tc_interrupt_handler,
			  IRQF_TRIGGER_HIGH, THERMAL_NAME, NULL);
	if (err)
		mtktc_printk("mtktc_init IRQ register fail\n");
#else
	err = request_irq(THERM_CTRL_IRQ_BIT_ID,
			tscpu_thermal_all_tc_interrupt_handler,
			  IRQF_TRIGGER_HIGH, THERMAL_NAME, NULL);
	if (err)
		mtktc_printk("mtktc_init IRQ register fail\n");
#endif

	mtktc_config_all_tc_hw_protect(tc_high_trip, tc_mid_trip);

	mtktscpu_dir = mtk_thermal_get_proc_drv_therm_dir_entry();
	if (!mtktscpu_dir) {
		mtktc_printk("[%s]: mkdir /proc/driver/thermal failed\n",
			     __func__);
	} else {
		entry = proc_create("mtktc_cal", 0400, mtktscpu_dir,
				&mtktc_cal_fops);

		entry = proc_create("mtktc_log", 0644, mtktscpu_dir,
				    &mtktc_log_fops);
	}

	mtkTTimer_register("mtk_thermal_tc",
		mtktc_start_thermal_timer,
		mtktc_cancel_thermal_timer);
	start_tempinfo_update_timer();

#if THERMAL_ENABLE_TINYSYS_SSPM || THERMAL_ENABLE_ONLY_TZ_SSPM
	//tc_ipi_send_efuse_data();
#endif

        err = tscpu_register_of_thermal(pdev);
        if (err) {
                pr_err("%s: tscpu_register_of_thermal fail\n", __func__);
                goto err_unreg;
        }

	return 0;

err_unreg:
	return err;

}

static int mtktc_thermal_remove(struct platform_device *pdev)
{

	pr_info("%s\n", __func__);
	mtkTTimer_unregister("mtk_thermal_tc");

	tscpu_unregister_of_thermal(pdev);

	cancel_tempinfo_update_timer();
	mtktc_thermal_clock_off();
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mtktc_thermal_of_match[] = {
	{
		.compatible = "mediatek,mt8696-thermal",
	},
	{},
};
#endif


static struct platform_driver mtktc_thermal_driver = {
	.remove = mtktc_thermal_remove,
	.shutdown = NULL,
	.probe = mtktc_thermal_probe,
	.suspend = mtktc_thermal_suspend,
	.resume = mtktc_thermal_resume,
	.driver = {
			.name = THERMAL_NAME,
#ifdef CONFIG_OF
			.of_match_table = mtktc_thermal_of_match,
#endif
		},
};

static int __init mtktc_init(void)
{
	return platform_driver_register(&mtktc_thermal_driver);
}

static void __exit mtktc_exit(void)
{
	mtktc_dprintk("%s\n", __func__);
}
module_init(mtktc_init);
module_exit(mtktc_exit);

/* late_initcall(mtktc_init); */
/* device_initcall(mtktc_init); */
