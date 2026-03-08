// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "pvrsrv_device.h"
#include "rgxdevice.h"
#include "rgxinit.h"
#include "pvrsrv.h"
#include "syscommon.h"
#include "sysconfig.h"
#include "physheap.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif
#include "mtk_mfg.h"
#include "mtk_mfg_counter.h"
#include "mtk_gpu_misc.h"

#if defined(CONFIG_OF)
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <linux/platform_device.h>
#endif
#include <linux/proc_fs.h>
#include <linux/devfreq.h>
#include <linux/seq_file.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/devfreq.h>
#include "process_stats.h"

static PVRSRV_DEVICE_CONFIG * pvr_cfg;
static struct device *pvr_os_dev;
static PVRSRV_DVFS *pvr_dvfs;

static unsigned int gpu_loading;
static unsigned int gpu_block;
static unsigned int gpu_idle;
static unsigned int limited_freq = 0;

static PVRSRV_DEVICE_NODE *get_pvr_dev_node(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	IMG_UINT32 i;
	/* This may be null when caller using too early..*/
	if (!psPVRSRVData)
		return NULL;
	for (i = 0; i < psPVRSRVData->ui32RegisteredDevices; i++) {
		PVRSRV_DEVICE_NODE *psDeviceNode = &psPVRSRVData->psDeviceNodeList[i];

		if (psDeviceNode && psDeviceNode->psDevConfig)
			return psDeviceNode;

	}
	return NULL;
}


#ifdef CONFIG_PROC_FS
static int freq_seq_show(struct seq_file *m, void *v)
{
	u32 freq = mtk_mfg_get_freq();
	int volt = mtk_mfg_get_volt();
	int volt_sram = mtk_mfg_get_volt_sram();
	unsigned int mem;

	mtk_gpu_misc_get_util(&gpu_loading, &gpu_block, &gpu_idle);

	mem = mtk_get_gpu_memory_usage();

	seq_printf(m,
		"volt:%d, volt_sram:%d, freq:%d, loading:%d, blocked:%d, idle:%d, mem:%d\n",
		volt, volt_sram, freq, gpu_loading, gpu_block, gpu_idle, mem);

	return 0;
}

/*
 * To ensure that voltage and frequency are not modified from dvfs,
 * even devfreq is suspended (e.g. Thermal Cooling).
 */
static bool lock_volt_freq;
static void set_lock_volt_freq(bool lock)
{
	lock_volt_freq = lock;
}
bool mtk_gpu_is_volt_freq_locked(void)
{
	return lock_volt_freq;
}

static int freq_seq_open(struct inode *in, struct file *file)
{
	return single_open(file, freq_seq_show, NULL);
}

static ssize_t freq_seq_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	char desc[32];
	unsigned int len = 0;
	long volt;
	unsigned long freq;
	PVRSRV_DEVICE_NODE *pvr_node;
	RGX_DATA		*psRGXData;
	RGX_TIMING_INFORMATION	*psRGXTimingInfo;
#if defined(SUPPORT_LINUX_DVFS)
	struct devfreq *df = NULL;
#endif

	pvr_node = get_pvr_dev_node();
	if (pvr_node) {
		psRGXData = (RGX_DATA *)pvr_node->psDevConfig->hDevData;
		psRGXTimingInfo = psRGXData->psRGXTimingInfo;

#if defined(SUPPORT_LINUX_DVFS)
		df = pvr_dvfs->sDVFSDevice.psDevFreq;
#endif

		len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
		if (copy_from_user(desc, buffer, len))
			return 0;

		desc[len] = '\0';

		if (sscanf(desc, "%ld %ld", &volt, &freq) == 2) {
			/* Suspend DVFS first and set volt and freq. */
#if defined(SUPPORT_LINUX_DVFS)
			devfreq_suspend_device(df);
#endif
			set_lock_volt_freq(true);
			if (freq > mtk_mfg_get_freq()) {
				mtk_mfg_set_volt(volt);
				mtk_mfg_set_freq(freq, true);
			} else {
				mtk_mfg_set_freq(freq, true);
				mtk_mfg_set_volt(volt);
			}
			psRGXTimingInfo->ui32CoreClockSpeed = freq;
		}
	}

	return count;
}

static const struct proc_ops freq_proc_ops = {
	.proc_open = freq_seq_open,
	.proc_read = seq_read,
	.proc_write = freq_seq_write,
	.proc_release = single_release,
};


static int regs_seq_show(struct seq_file *m, void *v)
{
	/*TODO: Enable power first?? */
	mfg_dump_regs_seq(m, "MFG ");
	return 0;
}

static int regs_seq_open(struct inode *in, struct file *file)
{
	return single_open(file, regs_seq_show, NULL);
}

static ssize_t regs_seq_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	char desc[32];
	unsigned int len = 0;
	u32 off;
	u32 val;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%x %x", &off, &val) == 2)
		mfg_write(off, val);

	return count;

}

static const struct proc_ops regs_proc_ops = {
	.proc_open = regs_seq_open,
	.proc_read = seq_read,
	.proc_write = regs_seq_write,
	.proc_release = single_release,
};


static int dvfs_seq_show(struct seq_file *m, void *v)
{
#if defined(SUPPORT_LINUX_DVFS)
	int i;
	int count;
	struct dev_pm_opp *opp;
	unsigned long freq = 0;
	unsigned long volt;

	IMG_DVFS_DEVICE *dvfs_dev = &pvr_dvfs->sDVFSDevice;

	struct devfreq_dev_status *stat;

	stat = &dvfs_dev->psDevFreq->last_status;

	if (lock_volt_freq)
		seq_printf(m, "Disabled and Fixed, current freq %ld, volt %d\n",
		mtk_mfg_get_freq(), mtk_mfg_get_volt());
	else if (dvfs_dev->psDevFreq->stop_polling)
		seq_printf(m, "Disabled by devfreq, current freq %ld, volt %d\n",
		mtk_mfg_get_freq(), mtk_mfg_get_volt());
	else if (!dvfs_dev->bEnabled)
		seq_printf(m, "Disabled by PVR DVFS, current freq %ld, volt %d\n",
		mtk_mfg_get_freq(), mtk_mfg_get_volt());
	else
		seq_puts(m, "Enabled\n");

	seq_puts(m, "    volt \t freq\n");

	/*rcu_read_lock();*/

	count = dev_pm_opp_get_opp_count(pvr_os_dev);
	for (i = 0; i < count; i++) {
		opp = dev_pm_opp_find_freq_ceil(pvr_os_dev, &freq);
		freq = dev_pm_opp_get_freq(opp);
		volt =	dev_pm_opp_get_voltage(opp);

		if (freq > dvfs_dev->psDevFreq->scaling_max_freq)
			seq_puts(m, " X ");
		else
			seq_puts(m, "   ");

		if (!lock_volt_freq && freq == stat->current_frequency)
			seq_puts(m, " * ");
		else
			seq_puts(m, "   ");

		seq_printf(m, "%ld \t %ld\n", volt, freq);
		freq++;/* next freq greater*/
	}
	/*rcu_read_unlock();*/

	/*
	 * We show the loading bounds instead of original data structure.
	 */
	seq_printf(m, "up:%d, down:%d, last:%ld\n", dvfs_dev->data.upthreshold,
		dvfs_dev->data.upthreshold - dvfs_dev->data.downdifferential,
		(unsigned long)(
			div_u64((u64)stat->busy_time * 100UL,
			(u64)stat->total_time)));

#else
	seq_puts(m, "NO DVFS\n");
#endif

	return 0;
}

static int dvfs_seq_open(struct inode *in, struct file *file)
{
	return single_open(file, dvfs_seq_show, NULL);
}

static ssize_t dvfs_seq_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{

	char desc[32];
	unsigned int len = 0;
	IMG_DVFS_DEVICE *psDVFSDev = &pvr_dvfs->sDVFSDevice;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';
	if (strncmp(desc, "disable", 7) == 0) {
		devfreq_suspend_device(psDVFSDev->psDevFreq);
		set_lock_volt_freq(true);
	} else if (strncmp(desc, "enable", 6) == 0) {
		devfreq_resume_device(psDVFSDev->psDevFreq);
		set_lock_volt_freq(false);
	}

	return count;
}

static const struct proc_ops dvfs_proc_ops = {
	.proc_open = dvfs_seq_open,
	.proc_read = seq_read,
	.proc_write = dvfs_seq_write,
	.proc_release = single_release,
};

static int install_cmds(void)
{
	struct proc_dir_entry *pentry = proc_mkdir("mtk-gpu", NULL);

	if (pentry) {
		proc_create_data("freq", 0600, pentry, &freq_proc_ops, NULL);
		proc_create_data("regs", 0600, pentry, &regs_proc_ops, NULL);
		proc_create_data("dvfs", 0600, pentry, &dvfs_proc_ops, NULL);
	}

	return 0;
}
#else
static int install_cmds(void)
{
	return 0;
}
#endif

/*
 * TODO: Helper functions for MET..
 */
static IMG_HANDLE util_user;

void mtk_gpu_misc_get_util(unsigned int *pui32Loading,
	unsigned int *pui32Block, unsigned int *pui32Idle)
{
	PVRSRV_DEVICE_NODE *pvr_node;
	PVRSRV_RGXDEV_INFO *pvr_info;

	if (pui32Loading)
		*pui32Loading = 0;
	if (pui32Block)
		*pui32Block = 0;
	if (pui32Idle)
		*pui32Idle = 0;

	pvr_node = get_pvr_dev_node();
	if (!util_user)
		return;

	if (pvr_node) {
		pvr_info = (PVRSRV_RGXDEV_INFO *)pvr_node->pvDevice;

		if (pvr_info && pvr_info->pfnGetGpuUtilStats) {
			RGXFWIF_GPU_UTIL_STATS util_stats = {0};

			pvr_info->pfnGetGpuUtilStats(pvr_info->psDeviceNode,
			    util_user, &util_stats);

			if (util_stats.bValid) {
#if defined(__arm64__) || defined(__aarch64__)
				if (pui32Loading)
					*pui32Loading =
						(100*(util_stats.ui64GpuStatActive)) /
						util_stats.ui64GpuStatCumulative;
				if (pui32Block)
					*pui32Block =
						(100*(util_stats.ui64GpuStatBlocked)) /
						util_stats.ui64GpuStatCumulative;
				if (pui32Idle)
					*pui32Idle =
						(100*(util_stats.ui64GpuStatIdle)) /
						util_stats.ui64GpuStatCumulative;
#else
				if (pui32Loading)
					*pui32Loading =
						(unsigned long)(100*
						(util_stats.ui64GpuStatActive)) /
						(unsigned long)util_stats.ui64GpuStatCumulative;
				if (pui32Block)
					*pui32Block =
						(unsigned long)(100*
						(util_stats.ui64GpuStatBlocked)) /
						(unsigned long)util_stats.ui64GpuStatCumulative;
				if (pui32Idle)
					*pui32Idle =
						(unsigned long)(100*
						(util_stats.ui64GpuStatIdle)) /
						(unsigned long)util_stats.ui64GpuStatCumulative;
#endif
				return;
			}
		}
	}
}

/*
 * For MET and other drivers which has 'static' reference to
 * gpu utils.
 */
unsigned int mtk_get_gpu_loading(void)
{
	return gpu_loading;
}
EXPORT_SYMBOL(mtk_get_gpu_loading);

unsigned int mtk_get_gpu_block(void)
{
	return gpu_block;
}
EXPORT_SYMBOL(mtk_get_gpu_block);

unsigned int mtk_get_gpu_idle(void)
{
	return gpu_idle;
}
EXPORT_SYMBOL(mtk_get_gpu_idle);

unsigned int mtk_get_gpu_memory_usage(void)
{
	return PVRMTKGetTotalMemory();
}
EXPORT_SYMBOL(mtk_get_gpu_memory_usage);

unsigned int mt_gpufreq_get_cur_freq(void)
{
	return mtk_mfg_get_snap_freq();
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq);

unsigned int mt_gpufreq_get_cur_volt(void)
{
	return mtk_mfg_get_snap_volt();
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_volt);

unsigned int mt_gpufreq_get_thermal_limit_freq_Khz(void)
{
	return limited_freq;
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_freq_Khz);

void mt_gpufreq_set_thermal_limit_freq(unsigned long freq)
{
	struct dev_pm_opp *opp;
	unsigned long freq_hz;

	if (!pvr_dvfs)
		return;

	freq_hz = freq * 1000;
	/*rcu_read_lock();*/
	opp = dev_pm_opp_find_freq_ceil(pvr_os_dev, &freq_hz);
	limited_freq = freq_hz;
	/*rcu_read_unlock();*/

	if (freq_hz) {
		mutex_lock(&pvr_dvfs->sDVFSDevice.psDevFreq->lock);
		pvr_dvfs->sDVFSDevice.psDevFreq->scaling_max_freq = freq_hz;
		update_devfreq(pvr_dvfs->sDVFSDevice.psDevFreq);
		mutex_unlock(&pvr_dvfs->sDVFSDevice.psDevFreq->lock);
	} else {
		pr_err("%s: cannot find any limited freq %ld\n", __func__, freq_hz);
	}
}
EXPORT_SYMBOL(mt_gpufreq_set_thermal_limit_freq);

void mtk_gpu_thermal_setup(void)
{
	int i;
	int count;
	struct dev_pm_opp *opp;
	unsigned long freq = 0;
	unsigned long volt;
	struct mtk_gpu_power_info *info;

	/*rcu_read_lock();*/
	count = dev_pm_opp_get_opp_count(pvr_os_dev);
	info = kcalloc(count, sizeof(struct mtk_gpu_power_info), GFP_NOWAIT);
	for (i = 0; i < count; i++) {
		opp = dev_pm_opp_find_freq_ceil(pvr_os_dev, &freq);
		freq = dev_pm_opp_get_freq(opp);
		volt =	dev_pm_opp_get_voltage(opp);

		info[count - i - 1].gpufreq_khz = freq/1000;
		info[count - i - 1].gpufreq_power = 0; //not used


		freq++;/* next freq greater*/
	}
	/*rcu_read_unlock();*/

	mtk_gpufreq_register(info, count);

	kfree(info);
}


#define MFG_POWER_NOTIFY_COUNT 5
met_gpu_power_change_notify_fp met_power_nfy[MFG_POWER_NOTIFY_COUNT];
char met_power_nfy_name[MFG_POWER_NOTIFY_COUNT][64];

static DEFINE_MUTEX(nfy_lock);

bool mtk_register_gpu_power_change(const char *name, met_gpu_power_change_notify_fp callback)
{
	int i;

	mutex_lock(&nfy_lock);
	for (i = 0; i < MFG_POWER_NOTIFY_COUNT; i++) {
		if (met_power_nfy[i] == NULL) {
			met_power_nfy[i] = callback;
			strncpy(met_power_nfy_name[i], name, 63);
			mutex_unlock(&nfy_lock);
			return true;
		}
	}
	mutex_unlock(&nfy_lock);
	return false;
}
EXPORT_SYMBOL(mtk_register_gpu_power_change);

bool mtk_unregister_gpu_power_change(const char *name)
{
	int i;

	mutex_lock(&nfy_lock);
	for (i = 0; i < MFG_POWER_NOTIFY_COUNT; i++) {
		if (strncmp(met_power_nfy_name[i], name, 63)) {
			met_power_nfy[i] = NULL;
			met_power_nfy_name[i][0] = '\0';
			mutex_unlock(&nfy_lock);
			return true;
		}
	}
	mutex_unlock(&nfy_lock);
	return false;
}
EXPORT_SYMBOL(mtk_unregister_gpu_power_change);

void mtk_gpu_misc_met_nfy_power_state(int power_on)
{
	int i;

	mutex_lock(&nfy_lock);
	for (i = 0; i < MFG_POWER_NOTIFY_COUNT; i++) {
		if (met_power_nfy[i] != NULL)
			met_power_nfy[i](power_on);
	}
	mutex_unlock(&nfy_lock);
}

static void met_nfy_init(void)
{
	int i;

	for (i = 0; i < MFG_POWER_NOTIFY_COUNT; i++) {
		met_power_nfy[i] = NULL;
		met_power_nfy_name[i][0] = '\0';
	}
}

void mtk_gpu_misc_init(PVRSRV_DEVICE_CONFIG *cfg)
{
	pvr_cfg = cfg;
	pvr_os_dev = cfg->pvOSDevice;
	pvr_dvfs = &cfg->sDVFS;

	SORgxGpuUtilStatsRegister(&util_user);

	met_nfy_init();

	install_cmds();
}
