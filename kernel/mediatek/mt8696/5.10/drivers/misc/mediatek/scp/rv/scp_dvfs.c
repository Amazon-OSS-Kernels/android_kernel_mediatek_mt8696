// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/suspend.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/pm_qos.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include "scp_ipi_pin.h"
#include "scp_mbox_layout.h"
#include "scp_helper.h"
#include "scp_excep.h"
#include "scp_dvfs.h"

#include <linux/pm_qos.h>

struct ipi_tx_data_t {
	unsigned int arg1;
	unsigned int arg2;
};

/* -1:SCP DVFS OFF, 1:SCP DVFS ON */
int scp_dvfs_flag = 1;

/*
 * -1: SCP Debug CMD: off,
 * 0-4: SCP DVFS Debug OPP.
 */
static int scp_dvfs_debug_flag = -1;

static struct wakeup_source *scp_suspend_lock;
static int g_scp_dvfs_init_flag = -1;

static struct regulator *dvfsrc_vscp_power;
static struct regulator *vgpu11_sshub; /* Vcore */

static void __iomem *gpio_base;

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SCP_MT8188)
#define ADR_GPIO_MODE_OF_SCP_VREQ	(gpio_base + 0x3c0)
#define BIT_GPIO_MODE_OF_SCP_VREQ	16
#define MSK_GPIO_MODE_OF_SCP_VREQ	0x7
#endif

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SCP_MT8195)
#define ADR_GPIO_MODE_OF_SCP_VREQ	(gpio_base + 0x390)
#define BIT_GPIO_MODE_OF_SCP_VREQ	16
#define MSK_GPIO_MODE_OF_SCP_VREQ	0x7
#endif

unsigned int slp_ipi_ackdata0, slp_ipi_ackdata1;
int slp_ipi_init_done;

void scp_slp_ipi_init(void)
{
	int ret;

	ret = mtk_ipi_register(&scp_ipidev, IPI_OUT_C_SLEEP_0,
			       NULL, NULL, &slp_ipi_ackdata0);
	if (ret)
		pr_notice("scp0 sleep ipi_register fail, ret %d\n", ret);

	ret = mtk_ipi_register(&scp_ipidev, IPI_OUT_C_SLEEP_1,
			       NULL, NULL, &slp_ipi_ackdata1);
	if (ret)
		pr_notice("scp1 sleep ipi_register fail, ret %d\n", ret);

	slp_ipi_init_done = 1;
}

static u32 _mt_scp_dvfs_set_test_freq(u32 sum)
{
	u32 added_freq = 0;

	if (scp_dvfs_debug_flag == -1)
		return 0;

	pr_info("manually set opp = %d\n", scp_dvfs_debug_flag);

	/* calculate test feature freq to meet fixed opp level. */
	if (scp_dvfs_debug_flag == 0 && sum < CLK_OPP0)
		added_freq = CLK_OPP0 - sum;
	else if (scp_dvfs_debug_flag == 1 && sum < CLK_OPP1)
		added_freq = CLK_OPP1 - sum;
	else if (scp_dvfs_debug_flag == 2 && sum < CLK_OPP2)
		added_freq = CLK_OPP2 - sum;
	else if (scp_dvfs_debug_flag == 3 && sum < CLK_OPP3)
		added_freq = CLK_OPP3 - sum;

	feature_table[VCORE_TEST_FEATURE_ID].freq = added_freq;
	pr_info("request freq: %d + %d = %d (MHz)\n",
		sum, added_freq, sum + added_freq);

	return added_freq;
}

u32 scp_get_freq(void)
{
	u32 i, sum_core0 = 0, sum_core1 = 0, return_freq = 0, sum = 0;

	/* calculate scp frequence */
	for (i = 0; i < NUM_FEATURE_ID; i++) {
		if (i != VCORE_TEST_FEATURE_ID &&
		    feature_table[i].enable == 1) {
			if (feature_table[i].sys_id == SCPSYS_CORE0)
				sum_core0 += feature_table[i].freq;
			else
				sum_core1 += feature_table[i].freq;
		}
	}

	/* calculate scp sensor frequence */
	for (i = 0; i < NUM_SENSOR_TYPE; i++) {
		if (sensor_type_table[i].enable == 1)
			sum += sensor_type_table[i].freq;
	}

	sum += sum_core0;
	feature_table[VCORE_TEST_FEATURE_ID].sys_id = SCPSYS_CORE0;
	if (sum_core1 > sum) {
		sum = sum_core1;
		feature_table[VCORE_TEST_FEATURE_ID].sys_id = SCPSYS_CORE1;
	}

	/* added up scp test cmd frequence */
	sum += _mt_scp_dvfs_set_test_freq(sum);

	if (sum <= CLK_OPP0) {
		return_freq = CLK_OPP0;
	} else if (sum <= CLK_OPP1) {
		return_freq = CLK_OPP1;
	} else if (sum <= CLK_OPP2) {
		return_freq = CLK_OPP2;
	} else if (sum <= CLK_OPP3) {
		return_freq = CLK_OPP3;
	} else {
		return_freq = CLK_OPP3;
		pr_debug("warning: request freq %d > max opp %d\n",
			 sum, CLK_OPP3);
	}

	return return_freq;
}

static int scp_set_regulator_voltage(int min_uV, int max_uv)
{
	int ret;

	/* Vcore voltage when pmic enter low power mode */
	ret = regulator_set_voltage(vgpu11_sshub, min_uV, max_uv);
	if (ret) {
		pr_notice("set vgpu11_sshub %d fail, ret = %d\n", min_uV, ret);
		return ret;
	}

	/* Vcore voltage when AP system runs at normal mode */
	ret = regulator_set_voltage(dvfsrc_vscp_power, min_uV, min_uV);
	if (ret) {
		pr_notice("set dvfsrc %d fail, ret = %d\n", min_uV, ret);
		return ret;
	}

	return 0;
}

int scp_vcore_request(unsigned int clk_opp)
{
	int ret, min_uV, max_uV;

	pr_debug("%s(%d)\n", __func__, clk_opp);

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SCP_MT8188)
	if (clk_opp == CLK_OPP0) {
		min_uV = 550000;
		max_uV = 725000;
		writel(0x8, SCP_SCP2SPM_VOL_LV);
	} else if (clk_opp == CLK_OPP1) {
		min_uV = 600000;
		max_uV = 725000;
		writel(0x104, SCP_SCP2SPM_VOL_LV);
	} else if (clk_opp == CLK_OPP2) {
		min_uV = 650000;
		max_uV = 725000;
		writel(0x202, SCP_SCP2SPM_VOL_LV);
	} else {
		min_uV = 725000;
		max_uV = 725000;
		writel(0x301, SCP_SCP2SPM_VOL_LV);
	}
#endif

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SCP_MT8195)
	if (clk_opp == CLK_OPP0) {
		min_uV = 550000;
		max_uV = 750000;
		writel(0x8, SCP_SCP2SPM_VOL_LV);
	} else if (clk_opp == CLK_OPP1) {
		min_uV = 600000;
		max_uV = 750000;
		writel(0x104, SCP_SCP2SPM_VOL_LV);
	} else if (clk_opp == CLK_OPP2) {
		min_uV = 650000;
		max_uV = 750000;
		writel(0x202, SCP_SCP2SPM_VOL_LV);
	} else {
		min_uV = 750000;
		max_uV = 750000;
		writel(0x301, SCP_SCP2SPM_VOL_LV);
	}
#endif

	ret = scp_set_regulator_voltage(min_uV, max_uV);
	if (ret)
		return ret;

	return 0;
}

/* scp_request_freq
 * return :-1 means the scp request freq. error
 * return :0  means the request freq. finished
 */
int scp_request_freq(void)
{
	int value = 0, ret = 0;
	unsigned long spin_flags;
	int is_increasing_freq = 0;
	struct pm_qos_request *qos_request;

	pr_debug("%s()\n", __func__);

	if (scp_dvfs_flag != 1 || g_scp_dvfs_init_flag != 1) {
		pr_notice("warning: SCP DVFS is OFF\n");
		return -1;
	}

	if (scp_current_freq == scp_expected_freq)
		return 0;

	qos_request = kzalloc(sizeof(struct pm_qos_request), GFP_KERNEL);
	if (!qos_request)
		return -1;

	/* Let CPUs leave idle-off state for SCP controlling */
	cpu_latency_qos_add_request(qos_request, PM_QOS_DEFAULT_VALUE);

	/*
	 * because we are waiting for scp to update register(scp_current_freq)
	 * use wake lock to prevent AP from entering suspend state
	 */
	__pm_stay_awake(scp_suspend_lock);

	/* keep scp alive before raise vcore up */
	scp_awake_lock((void *)SCP_A_ID);

	/* do DVS before DFS if increasing frequency */
	if (scp_current_freq < scp_expected_freq ||
	    scp_current_freq == CLK_UNINIT) {
		if (scp_vcore_request(scp_expected_freq)) {
			ret = -1;
			goto scp_request_freq_done;
		}

		is_increasing_freq = 1;
	}

	value = scp_expected_freq;

	do {
		ret = mtk_ipi_send(&scp_ipidev,	IPI_OUT_DVFS_SET_FREQ_0,
				   IPI_SEND_WAIT, &value,
				   PIN_OUT_SIZE_DVFS_SET_FREQ_0, 500);
		if (ret != IPI_ACTION_DONE) {
			pr_notice("set freq fail, c(%u) != e(%u), %d\n",
			       scp_current_freq, scp_expected_freq,
			       ret);
			ret = -1;
			goto scp_request_freq_done;
		}

		/* wait RV33 to set register(CURRENT_FREQ_REG) */
		mdelay(5);

		/* read scp_current_freq again */
		spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
		scp_current_freq = readl(CURRENT_FREQ_REG);
		spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
	} while (scp_current_freq != scp_expected_freq);

	/* do DVS after DFS if decreasing frequency */
	if (is_increasing_freq == 0) {
		if (scp_vcore_request(scp_expected_freq)) {
			ret = -1;
			goto scp_request_freq_done;
		}
	}

	pr_debug("[SCP] succeed to set freq, expect=%u, cur=%u\n",
		 scp_expected_freq, scp_current_freq);

scp_request_freq_done:
	/* release scp to sleep after ap freq drop request */
	scp_awake_unlock((void *)SCP_A_ID);
	__pm_relax(scp_suspend_lock);
	cpu_latency_qos_remove_request(qos_request);
	kfree(qos_request);

	return ret;
}

void wait_scp_dvfs_init_done(void)
{
	u32 count = 1;

	while (g_scp_dvfs_init_flag != 1) {
		if ((count % 3000) == 0) {
			pr_notice("SCP dvfs driver init fail\n");
			WARN_ON(1);
		}

		mdelay(1);
		count++;
	}
}

#if IS_ENABLED(CONFIG_PROC_FS)
/*
 * PROC
 */

/****************************
 * show SCP state
 *****************************/
static int mt_scp_dvfs_state_proc_show(struct seq_file *m, void *v)
{
	unsigned int scp_state;
	char *scp_status;

	scp_state = readl(SCP_A_SLEEP_DEBUG_REG);
	if ((scp_state & IN_DEBUG_IDLE) == IN_DEBUG_IDLE)
		scp_status = "idle mode";
	else if ((scp_state & ENTERING_SLEEP) == ENTERING_SLEEP)
		scp_status = "enter sleep";
	else if ((scp_state & IN_SLEEP) == IN_SLEEP)
		scp_status = "sleep mode";
	else if ((scp_state & ENTERING_ACTIVE) == ENTERING_ACTIVE)
		scp_status = "enter active";
	else if ((scp_state & IN_ACTIVE) == IN_ACTIVE)
		scp_status = "active mode";
	else
		scp_status = "none of state";

	seq_printf(m, "scp status: %s\n", scp_status);

	return 0;
}

/****************************
 * show scp dvfs ctrl
 *****************************/
static int mt_scp_dvfs_ctrl_proc_show(struct seq_file *m, void *v)
{
	unsigned long spin_flags;
	bool scp_dvfs_status;
	int i;

	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	scp_current_freq = readl(CURRENT_FREQ_REG);
	scp_expected_freq = readl(EXPECTED_FREQ_REG);
	scp_dvfs_status = (scp_dvfs_flag == 1) && (g_scp_dvfs_init_flag == 1);
	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);

	seq_printf(m, "SCP DVFS: %s\n", scp_dvfs_status ? "ON" : "OFF");
	seq_printf(m, "SCP frequency: cur=%uMHz, expect=%uMHz\n",
		   scp_current_freq, scp_expected_freq);

	for (i = 0; i < NUM_FEATURE_ID; i++)
		seq_printf(m, "feature=%d, freq=%d, enable=%d\n",
			   feature_table[i].feature, feature_table[i].freq,
			   feature_table[i].enable);

	for (i = 0; i < NUM_SENSOR_TYPE; i++)
		seq_printf(m, "sensor id=%d, freq=%d, enable=%d\n",
			   sensor_type_table[i].feature,
			   sensor_type_table[i].freq,
			   sensor_type_table[i].enable);

	return 0;
}

/**********************************
 * write scp dvfs ctrl
 ***********************************/
static ssize_t mt_scp_dvfs_ctrl_proc_write(struct file *file,
					   const char __user *buffer,
					   size_t count, loff_t *data)
{
	char *desc = NULL, cmd[32];
	int dvfs_opp;
	int n;

	if (count >= PAGE_SIZE)
		return -EINVAL;

	desc = (char *)memdup_user_nul(buffer, count);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	n = sscanf(desc, "%31s %d", cmd, &dvfs_opp);
	if (n == 1 || n == 2) {
		if (!strcmp(cmd, "on")) {
			scp_dvfs_flag = 1;
			pr_info("SCP DVFS: ON\n");
		} else if (!strcmp(cmd, "off")) {
			scp_dvfs_flag = -1;
			pr_info("SCP DVFS: OFF\n");
		} else if (!strcmp(cmd, "opp")) {
			if (dvfs_opp == -1) {
				pr_info("remove the opp setting of command\n");

				feature_table[VCORE_TEST_FEATURE_ID].freq = 0;

				scp_deregister_feature(VCORE_TEST_FEATURE_ID);

				scp_dvfs_debug_flag = dvfs_opp;
			} else if (dvfs_opp >= 0 && dvfs_opp <= 3) {
				scp_dvfs_debug_flag = dvfs_opp;

				scp_register_feature(VCORE_TEST_FEATURE_ID);
			} else {
				pr_info("invalid opp value %d\n", dvfs_opp);
			}
		} else {
			pr_info("invalid command %s\n", cmd);
		}
	} else {
		pr_info("invalid length %d\n", n);
	}

	kfree(desc);

	return count;
}

/****************************
 * show scp sleep ctrl0
 *****************************/
static int mt_scp_sleep_ctrl0_proc_show(struct seq_file *m, void *v)
{
	int ret;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	ipi_data.arg1 = SLP_DBG_CMD_GET_FLAG;
	ret = mtk_ipi_send_compl(&scp_ipidev, IPI_OUT_C_SLEEP_0, IPI_SEND_WAIT,
				 &ipi_data, PIN_OUT_C_SIZE_SLEEP_0, 500);
	if (ret != IPI_ACTION_DONE) {
		seq_printf(m, "ipi fail, ret = %d\n", ret);
	} else {
		if (slp_ipi_ackdata0 >= SCP_SLEEP_OFF &&
		    slp_ipi_ackdata0 <= SLP_DBG_CMD_SET_NO_CONDITION)
			seq_printf(m, "SCP Sleep flag = %d\n",
				   slp_ipi_ackdata0);
		else
			seq_printf(m, "invalid SCP Sleep flag = %d\n",
				   slp_ipi_ackdata0);
	}

	return 0;
}

/**********************************
 * write scp sleep ctrl0
 ***********************************/
static ssize_t mt_scp_sleep_ctrl0_proc_write(struct file *file,
					     const char __user *buffer,
					     size_t count, loff_t *data)
{
	char *desc = NULL;
	unsigned int val = 0;
	int ret = 0;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	if (count >= PAGE_SIZE)
		return -EINVAL;

	desc = (char *)memdup_user_nul(buffer, count);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	if (kstrtouint(desc, 10, &val) == 0) {
		if (val >= SCP_SLEEP_OFF && val <= SCP_SLEEP_NO_CONDITION) {
			ipi_data.arg1 = val;
			ret = mtk_ipi_send_compl(&scp_ipidev,
						 IPI_OUT_C_SLEEP_0,
						 IPI_SEND_WAIT,
						 &ipi_data,
						 PIN_OUT_C_SIZE_SLEEP_0,
						 500);
			if (ret)
				pr_notice("%s: mtk_ipi_send_compl fail, ret=%d\n",
				       __func__, ret);
		} else {
			pr_info("Warning: invalid input value %d\n", val);
		}
	} else {
		pr_info("Warning: invalid input command, val=%d\n", val);
	}

	kfree(desc);

	return count;
}

/****************************
 * show scp sleep ctrl1
 *****************************/
static int mt_scp_sleep_ctrl1_proc_show(struct seq_file *m, void *v)
{
	int ret;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	ipi_data.arg1 = SLP_DBG_CMD_GET_FLAG;
	ret = mtk_ipi_send_compl(&scp_ipidev, IPI_OUT_C_SLEEP_1, IPI_SEND_WAIT,
				 &ipi_data, PIN_OUT_C_SIZE_SLEEP_1, 500);
	if (ret != IPI_ACTION_DONE) {
		seq_printf(m, "ipi fail, ret = %d\n", ret);
	} else {
		if (slp_ipi_ackdata1 >= SCP_SLEEP_OFF &&
		    slp_ipi_ackdata1 <= SLP_DBG_CMD_SET_NO_CONDITION)
			seq_printf(m, "SCP Sleep flag = %d\n",
				   slp_ipi_ackdata1);
		else
			seq_printf(m, "invalid SCP Sleep flag = %d\n",
				   slp_ipi_ackdata1);
	}

	return 0;
}

/**********************************
 * write scp sleep ctrl1
 ***********************************/
static ssize_t mt_scp_sleep_ctrl1_proc_write(struct file *file,
					     const char __user *buffer,
					     size_t count, loff_t *data)
{
	char *desc = NULL;
	unsigned int val = 0;
	int ret = 0;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	if (count >= PAGE_SIZE)
		return -EINVAL;

	desc = (char *)memdup_user_nul(buffer, count);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	if (kstrtouint(desc, 10, &val) == 0) {
		if (val >= SCP_SLEEP_OFF && val <= SCP_SLEEP_NO_CONDITION) {
			ipi_data.arg1 = val;
			ret = mtk_ipi_send_compl(&scp_ipidev,
						 IPI_OUT_C_SLEEP_1,
						 IPI_SEND_WAIT,
						 &ipi_data,
						 PIN_OUT_C_SIZE_SLEEP_1,
						 500);
			if (ret != IPI_ACTION_DONE)
				pr_notice("%s: mtk_ipi_send_compl fail, ret=%d\n",
				       __func__, ret);
		} else {
			pr_info("Warning: invalid input value %d\n", val);
		}
	} else {
		pr_info("Warning: invalid input command, val=%d\n", val);
	}

	kfree(desc);

	return count;
}

/****************************
 * show scp sleep cnt0
 *****************************/
static int mt_scp_sleep_cnt0_proc_show(struct seq_file *m, void *v)
{
	int ret;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	ipi_data.arg1 = SLP_DBG_CMD_GET_CNT;
	ret = mtk_ipi_send_compl(&scp_ipidev, IPI_OUT_C_SLEEP_0, IPI_SEND_WAIT,
				 &ipi_data, PIN_OUT_C_SIZE_SLEEP_0, 500);
	if (ret != IPI_ACTION_DONE)
		seq_printf(m, "ipi fail, ret = %d\n", ret);
	else
		seq_printf(m, "scp_sleep_cnt = %d\n", slp_ipi_ackdata0);

	return 0;
}

/**********************************
 * write scp sleep cnt0
 ***********************************/
static ssize_t mt_scp_sleep_cnt0_proc_write(struct file *file,
					    const char __user *buffer,
					    size_t count, loff_t *data)
{
	char *desc = NULL;
	unsigned int val = 0;
	int ret = 0;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	if (count >= PAGE_SIZE)
		return -EINVAL;

	desc = (char *)memdup_user_nul(buffer, count);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	if (kstrtouint(desc, 10, &val) == 0) {
		ipi_data.arg1 = SLP_DBG_CMD_RESET;

		ret = mtk_ipi_send_compl(&scp_ipidev,
					 IPI_OUT_C_SLEEP_0,
					 IPI_SEND_WAIT,
					 &ipi_data,
					 PIN_OUT_C_SIZE_SLEEP_0,
					 500);
		if (ret != IPI_ACTION_DONE)
			pr_notice("%s: mtk_ipi_send_compl fail, ret=%d\n",
			       __func__, ret);
	} else {
		pr_info("Warning: invalid input command, val=%d\n", val);
	}

	kfree(desc);

	return count;
}

/****************************
 * show scp sleep cnt1
 *****************************/
static int mt_scp_sleep_cnt1_proc_show(struct seq_file *m, void *v)
{
	int ret;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	ipi_data.arg1 = SLP_DBG_CMD_GET_CNT;
	ret = mtk_ipi_send_compl(&scp_ipidev, IPI_OUT_C_SLEEP_1, IPI_SEND_WAIT,
				 &ipi_data, PIN_OUT_C_SIZE_SLEEP_1, 500);
	if (ret != IPI_ACTION_DONE)
		seq_printf(m, "ipi fail, ret = %d\n", ret);
	else
		seq_printf(m, "scp_sleep_cnt = %d\n", slp_ipi_ackdata1);

	return 0;
}

/**********************************
 * write scp sleep cnt1
 ***********************************/
static ssize_t mt_scp_sleep_cnt1_proc_write(struct file *file,
					    const char __user *buffer,
					    size_t count, loff_t *data)
{
	char *desc = NULL;
	unsigned int val = 0;
	int ret = 0;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	if (count >= PAGE_SIZE)
		return -EINVAL;

	desc = (char *)memdup_user_nul(buffer, count);
	if (IS_ERR(desc))
		return PTR_ERR(desc);


	if (kstrtouint(desc, 10, &val) == 0) {
		ipi_data.arg1 = SLP_DBG_CMD_RESET;

		ret = mtk_ipi_send_compl(&scp_ipidev,
					 IPI_OUT_C_SLEEP_1,
					 IPI_SEND_WAIT,
					 &ipi_data,
					 PIN_OUT_C_SIZE_SLEEP_1,
					 500);
		if (ret != IPI_ACTION_DONE)
			pr_notice("%s: mtk_ipi_send_compl fail, ret=%d\n",
			       __func__, ret);
	} else {
		pr_info("Warning: invalid input command, val=%d\n", val);
	}

	kfree(desc);

	return count;
}

#define PROC_FOPS_RW(name) \
static int mt_ ## name ## _proc_open(\
				struct inode *inode, \
				struct file *file) \
{ \
	return single_open(file, mt_ ## name ## _proc_show, \
			   PDE_DATA(inode)); \
} \
static const struct proc_ops \
	mt_ ## name ## _proc_fops = {\
	.proc_open		= mt_ ## name ## _proc_open, \
	.proc_read		= seq_read, \
	.proc_lseek		= seq_lseek, \
	.proc_release	= single_release, \
	.proc_write		= mt_ ## name ## _proc_write, \
}

#define PROC_FOPS_RO(name) \
static int mt_ ## name ## _proc_open(\
				struct inode *inode,\
				struct file *file)\
{ \
	return single_open(file, mt_ ## name ## _proc_show, \
			   PDE_DATA(inode)); \
} \
static const struct proc_ops mt_ ## name ## _proc_fops = {\
	.proc_open		= mt_ ## name ## _proc_open,\
	.proc_read		= seq_read,\
	.proc_lseek		= seq_lseek,\
	.proc_release	= single_release,\
}

#define PROC_ENTRY(name) {__stringify(name), &mt_ ## name ## _proc_fops}

PROC_FOPS_RO(scp_dvfs_state);
PROC_FOPS_RW(scp_dvfs_ctrl);
PROC_FOPS_RW(scp_sleep_ctrl0);
PROC_FOPS_RW(scp_sleep_ctrl1);
PROC_FOPS_RW(scp_sleep_cnt0);
PROC_FOPS_RW(scp_sleep_cnt1);

static int mt_scp_dvfs_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i, ret = 0;

	struct pentry {
		const char *name;
		const struct proc_ops *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(scp_dvfs_state),
		PROC_ENTRY(scp_dvfs_ctrl),
		PROC_ENTRY(scp_sleep_ctrl0),
		PROC_ENTRY(scp_sleep_ctrl1),
		PROC_ENTRY(scp_sleep_cnt0),
		PROC_ENTRY(scp_sleep_cnt1),
	};

	dir = proc_mkdir("scp_dvfs", NULL);
	if (!dir) {
		pr_notice("fail to create /proc/scp_dvfs @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, 0664, dir,
				 entries[i].fops)) {
			pr_notice("ERROR: %s: create /proc/scp_dvfs/%s failed\n",
			       __func__, entries[i].name);
			ret = -ENOMEM;
		}
	}

	return ret;
}
#endif /* CONFIG_PROC_FS */

#if IS_ENABLED(CONFIG_PM)
static int mt_scp_dump_sleep_count(void)
{
	int ret;
	struct ipi_tx_data_t ipi_data;

	if (!slp_ipi_init_done)
		scp_slp_ipi_init();

	ipi_data.arg1 = SLP_DBG_CMD_GET_CNT;
	ret = mtk_ipi_send_compl(&scp_ipidev, IPI_OUT_C_SLEEP_0, IPI_SEND_WAIT,
				 &ipi_data, PIN_OUT_C_SIZE_SLEEP_0, 500);
	if (ret != IPI_ACTION_DONE)
		pr_notice("[%s:%d] - scp ipi fail, ret = %d\\n",
				__func__, __LINE__, ret);
	else
		pr_info("[%s:%d] - scp_sleep_cnt_0 = %d\n",
			__func__, __LINE__, slp_ipi_ackdata0);

	ret = mtk_ipi_send_compl(&scp_ipidev, IPI_OUT_C_SLEEP_1, IPI_SEND_WAIT,
				 &ipi_data, PIN_OUT_C_SIZE_SLEEP_1, 500);
	if (ret != IPI_ACTION_DONE)
		pr_notice("[%s:%d] - scp ipi fail, ret = %d\\n",
				__func__, __LINE__, ret);
	else
		pr_info("[%s:%d] - scp_sleep_cnt_1 = %d\n",
			__func__, __LINE__, slp_ipi_ackdata1);

	return 0;
}

static int mtk_scp_pm_event(struct notifier_block *notifier,
			       unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		return NOTIFY_DONE;
	case PM_RESTORE_PREPARE:
		return NOTIFY_DONE;
	case PM_POST_HIBERNATION:
		return NOTIFY_DONE;
	case PM_SUSPEND_PREPARE:
	case PM_POST_SUSPEND:
		mt_scp_dump_sleep_count();
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block mtk_scp_pm_notifier_func = {
	.notifier_call = mtk_scp_pm_event,
};
#endif

int mt_pmic_sshub_init(void)
{
	int ret;

	ret = regulator_enable(vgpu11_sshub);
	if (ret) {
		pr_notice("Enable vcore failed!!!\n");
		return ret;
	}

	ret = scp_vcore_request(CLK_OPP0);
	if (ret)
		return ret;

	return 0;
}

static const struct of_device_id scpdvfs_of_ids[] = {
	{.compatible = "mediatek,scp-dvfs",},
	{}
};

static int scp_dvfs_pdrv_probe(struct platform_device *pdev)
{
	struct device_node *gpio_node;
	unsigned int gpio_mode;
	int ret;

	pr_notice("%s()\n", __func__);

	dvfsrc_vscp_power = devm_regulator_get_optional(&pdev->dev,
							"dvfsrc-vscp");
	if (IS_ERR(dvfsrc_vscp_power)) {
		pr_notice("dvfsrc-vscp-supply is not available: %d\n",
		       PTR_ERR(dvfsrc_vscp_power));
		return PTR_ERR(dvfsrc_vscp_power);
	}

	vgpu11_sshub = devm_regulator_get_optional(&pdev->dev, "sshub-vcore");
	if (IS_ERR(vgpu11_sshub)) {
		pr_notice("sshub-vcore-supply is not available: %d\n",
			  PTR_ERR(vgpu11_sshub));
		return PTR_ERR(vgpu11_sshub);
	}

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SCP_MT8188)
	gpio_node = of_find_compatible_node(NULL, NULL,
					    "mediatek,mt8188-pinctrl");
#endif

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SCP_MT8195)
	gpio_node = of_find_compatible_node(NULL, NULL,
					    "mediatek,mt8195-pinctrl");
#endif

	if (IS_ERR_OR_NULL(gpio_node)) {
		pr_notice("error: can't find GPIO node\n");
		WARN_ON(1);
		return -ENODEV;
	}

	gpio_base = of_iomap(gpio_node, 0);
	if (IS_ERR_OR_NULL(gpio_base)) {
		pr_notice("error: iomap fail for GPIO\n");
		WARN_ON(1);
		return -ENODEV;
	}

	/* check if GPIO is configured correctly for SCP VREQ */
	gpio_mode = (readl(ADR_GPIO_MODE_OF_SCP_VREQ) >>
		     BIT_GPIO_MODE_OF_SCP_VREQ) & MSK_GPIO_MODE_OF_SCP_VREQ;
	if (gpio_mode == 1) {
		pr_notice("SCP_VREQ_VAO pinmux setting is correct - 0x%x\n",
			  readl(ADR_GPIO_MODE_OF_SCP_VREQ));
	} else {
		pr_notice("wrong SCP_VREQ_VAO pinmux setting - 0x%x\n",
		       readl(ADR_GPIO_MODE_OF_SCP_VREQ));
		WARN_ON(1);
	}

	of_node_put(gpio_node);

	scp_suspend_lock = wakeup_source_register(&pdev->dev, "scp wakelock");
	if (!scp_suspend_lock) {
		pr_notice("cannot register wakelock\n");
		return -EPERM;
	}

	ret = mt_pmic_sshub_init();
	if (ret)
		return ret;

#if IS_ENABLED(CONFIG_PM)
	ret = register_pm_notifier(&mtk_scp_pm_notifier_func);
	if (ret) {
		pr_notice("[SCP DVFS] Failed to register PM notifier.\n");
		return ret;
	}
#endif /* CONFIG_PM */

#if IS_ENABLED(CONFIG_PROC_FS)
	if (mt_scp_dvfs_create_procfs()) {
		pr_notice("mt_scp_dvfs_create_procfs fail..\n");
		WARN_ON(1);
		return -1;
	}
#endif /* CONFIG_PROC_FS */

	g_scp_dvfs_init_flag = 1;

	return 0;
}

/***************************************
 * this function should never be called
 ****************************************/
static int scp_dvfs_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver scp_dvfs_pdrv = {
	.probe = scp_dvfs_pdrv_probe,
	.remove = scp_dvfs_pdrv_remove,
	.driver = {
		.name = "scp_dvfs",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(scpdvfs_of_ids),
	},
};

int __init scp_dvfs_init(void)
{
	struct device_node *scp_np, *scp_dvfs_np;
	struct platform_device *scp_dvfs_pdev;
	int ret = 0;

	pr_notice("%s\n", __func__);

	scp_np = of_find_node_by_name(NULL, "scp");
	if (!scp_np) {
		pr_notice("no node mediatek,scp\n");
		return -ENODEV;
	}

	scp_dvfs_pdev = platform_device_alloc("scp-dvfs", 0);
	if (!scp_dvfs_pdev) {
		pr_notice("cannot alloc scp_dvfs_pdev?\n");
		return -EPERM;
	}

	for_each_child_of_node(scp_np, scp_dvfs_np) {
		if (of_device_is_compatible(scp_dvfs_np,
					    "mediatek,scp-dvfs")) {
			scp_dvfs_pdev->dev.of_node = scp_dvfs_np;
			break;
		}
	}

	of_node_put(scp_np);

	ret = platform_device_add(scp_dvfs_pdev);
	if (ret) {
		pr_notice("fail to register scp dvfs device @ %s()\n", __func__);
		WARN_ON(1);
		return ret;
	}

	ret = platform_driver_register(&scp_dvfs_pdrv);
	if (ret) {
		pr_notice("fail to register scp dvfs driver @ %s()\n", __func__);
		WARN_ON(1);
		return ret;
	}

	return ret;
}

void __exit scp_dvfs_exit(void)
{
	platform_driver_unregister(&scp_dvfs_pdrv);
}

