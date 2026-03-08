/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _MTK_HIFIXDSP_COMMON_
#define _MTK_HIFIXDSP_COMMON_

/*
 * ADSP reserve memory ID definition
 */
enum ADSP_RESERVE_MEM_ID {
	ADSP_A_AUDIO_MEM_ID,
	ADSP_LOGGER_BUF_MEM_ID,
	ADSP_MET_MEM_ID,
	ADSP_NUMS_SYSRAM_ID
};

/*
 * ADSP hardware semaphore ID definition
 *
 * Here, add SEMA-ID definition MUST be
 * sync with the SEMA-ID definition in FreeRTOS DSP project.
 */
enum ADSP_HW_SEMAPHORE_ID {
	ADSP_HW_SEMA_EXAMPLE = 0,
	ADSP_HW_SEMA_IPI = 1,
	/* add semaphore id here */
	ADSP_HW_SEMA_MAX = 10
};

/* Callback type define */
typedef void (*callback_fn)(void *arg);

/*
 * Public function API for Audio system
 */
extern int hifixdsp_run_status(void);
extern int async_load_hifixdsp_bin_and_run(callback_fn callback, void *param);
extern int hifixdsp_stop_run(void);
extern int hw_semaphore_get(enum ADSP_HW_SEMAPHORE_ID sema_id,
			unsigned int timeout);
extern int hw_semaphore_release(enum ADSP_HW_SEMAPHORE_ID sema_id);
extern unsigned long adsp_get_reserve_sysram_phys(enum ADSP_RESERVE_MEM_ID id);
extern unsigned long adsp_get_reserve_sysram_virt(enum ADSP_RESERVE_MEM_ID id);
extern unsigned long adsp_get_reserve_sysram_size(enum ADSP_RESERVE_MEM_ID id);
extern unsigned long adsp_get_shared_dtcm_virt_for_logger(void);
extern unsigned long adsp_get_shared_dtcm_virt_for_ipc(void);
extern unsigned long adsp_get_shared_timesync_virt(void);
extern void __iomem *adsp_get_shared_sysram_phys2virt(phys_addr_t addr);
extern phys_addr_t adsp_get_shared_sysram_virt2phys(void __iomem *addr);
extern phys_addr_t adsp_hal_phys_addr_cpu2dsp(phys_addr_t addr);
extern phys_addr_t adsp_hal_phys_addr_dsp2cpu(phys_addr_t addr);
extern int register_adsp_wdt_notifier(struct notifier_block *nb);
extern int unregister_adsp_wdt_notifier(struct notifier_block *nb);
extern int adsp_get_bus_emi_offset(void);

#endif /*_MTK_HIFIXDSP_COMMON_*/
