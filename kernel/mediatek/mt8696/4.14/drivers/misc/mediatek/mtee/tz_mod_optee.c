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


#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/cma.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/pagemap.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>

#include <linux/clk.h>
#include <linux/irqdomain.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/completion.h>
#include <mt-plat/mtk_cmo.h>

#include "trustzone/kree/tz_mod.h"
#include "trustzone/kree/mem.h"
#include "trustzone/kree/system.h"
#include "trustzone/kree/tz_pm.h"
#include "trustzone/kree/tz_irq.h"
#include "trustzone/tz_cross/ta_mem.h"
#include "trustzone/tz_cross/ta_system.h"
#include "kree_int.h"
#include "tz_counter.h"
#include "tz_cross/ta_pm.h"
#include "tz_ndbg.h"

#include "tz_playready.h"

#include "tz_secure_clock.h"
#define MTEE_MOD_TAG "MTEE_MOD"

#define TZ_DEVNAME "mtk_tz"

/* Used for mapping user space address to physical memory
 */
struct MTIOMMU_PIN_RANGE_T {
	void *start;
	void *pageArray;
	uint32_t size;
	uint32_t nrPages;
	uint32_t isPage;
};

#ifdef CONFIG_OF
/*Used for clk management*/
#define CLK_NAME_LEN 16
struct mtee_clk {
	struct list_head list;
	char clk_name[CLK_NAME_LEN];
	struct clk *clk;
};
static LIST_HEAD(mtee_clk_list);

static void mtee_clks_init(struct platform_device *pdev)
{
	int clks_num;
	int idx;

	clks_num = of_property_count_strings(pdev->dev.of_node, "clock-names");
	for (idx = 0; idx < clks_num; idx++) {
		const char *clk_name;
		struct clk *clk;
		struct mtee_clk *mtee_clk;

		if (of_property_read_string_index(pdev->dev.of_node,
					"clock-names", idx, &clk_name)) {
			pr_warn("[%s] get clk_name failed, index:%d\n",
				MODULE_NAME,
				idx);
			continue;
		}
		if (strlen(clk_name) > CLK_NAME_LEN-1) {
			pr_warn("[%s] clk_name %s is longer than %d, trims to %d\n",
				MODULE_NAME,
				clk_name, CLK_NAME_LEN-1, CLK_NAME_LEN-1);
		}
		clk = devm_clk_get(&pdev->dev, clk_name);
		if (IS_ERR(clk)) {
			pr_warn("[%s] get devm_clk_get failed, clk_name:%s\n",
				MODULE_NAME,
				clk_name);
			continue;
		}

		mtee_clk = kzalloc(sizeof(struct mtee_clk), GFP_KERNEL);
		strncpy(mtee_clk->clk_name, clk_name, CLK_NAME_LEN-1);
		mtee_clk->clk = clk;

		list_add(&mtee_clk->list, &mtee_clk_list);
	}
}

struct clk *mtee_clk_get(const char *clk_name)
{
	struct mtee_clk *cur;
	struct mtee_clk *tmp;

	list_for_each_entry_safe(cur, tmp, &mtee_clk_list, list) {
		if (strncmp(cur->clk_name, clk_name,
			strlen(cur->clk_name)) == 0)
			return cur->clk;
	}
	return NULL;
}

/*Used for power management*/
#define PM_NAME_LEN 16
struct mtee_pm {
	struct list_head list;
	char pm_name[PM_NAME_LEN];
	struct device *dev;
};
static LIST_HEAD(mtee_pm_list);

static void mtee_pms_init(struct platform_device *pdev)
{
	int pms_num;
	int idx;

	pms_num = of_property_count_strings(pdev->dev.of_node, "pm-names");
	for (idx = 0; idx < pms_num; idx++) {
		const char *pm_name;
		struct platform_device *platdev;
		struct mtee_pm *mtee_pm;
		struct device_node *node;

		if (of_property_read_string_index(pdev->dev.of_node,
					"pm-names", idx, &pm_name)) {
			pr_warn("[%s] get pm_name failed, index:%d\n",
				MODULE_NAME,
				idx);
			continue;
		}
		if (strlen(pm_name) > PM_NAME_LEN-1) {
			pr_warn("[%s] pm_name %s is longer than %d, trims to %d\n",
				MODULE_NAME,
				pm_name, PM_NAME_LEN-1, PM_NAME_LEN-1);
		}
		node = of_parse_phandle(pdev->dev.of_node, "pm-devs", idx);
		if (!node)
			continue;

		platdev = of_find_device_by_node(node);
		of_node_put(node);
		if (!platdev)
			continue;

		mtee_pm = kzalloc(sizeof(struct mtee_pm), GFP_KERNEL);
		strncpy(mtee_pm->pm_name, pm_name, PM_NAME_LEN-1);
		mtee_pm->dev = &platdev->dev;

		list_add(&mtee_pm->list, &mtee_pm_list);
	}
}

struct device *mtee_pmdev_get(const char *pm_name)
{
	struct mtee_pm *cur;
	struct mtee_pm *tmp;

	list_for_each_entry_safe(cur, tmp, &mtee_pm_list, list) {
		if (strncmp(cur->pm_name, pm_name,
			strlen(cur->pm_name)) == 0)
			return cur->dev;
	}
	return NULL;
}

#endif

/*****************************************************************************
 * FUNCTION DEFINITION
 *****************************************************************************
 */

/* pm op funcstions */
#ifdef TZ_PLAYREADY_SECURETIME_SUPPORT
#ifdef CONFIG_HAS_EARLYSUSPEND
static int securetime_savefile(void)
{
	int ret = 0;

	KREE_SESSION_HANDLE securetime_session = 0;

	ret = KREE_CreateSession(TZ_TA_PLAYREADY_UUID, &securetime_session);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("[securetime]CreateSession error %d\n", ret);

	TEE_update_pr_time_infile(securetime_session);

	ret = KREE_CloseSession(securetime_session);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("CloseSession error %d\n", ret);


	return ret;
}
#endif /* EARLYSUSPEND */

static void st_shutdown(struct platform_device *pdev)
{
	pr_warn("[securetime]%s: kickoff\n", __func__);
}
#endif

#ifdef CONFIG_PM_SLEEP
static int tz_suspend(struct device *pdev)
{
	int tzret;

	tzret = kree_pm_device_ops(MTEE_SUSPEND);
	return (tzret != TZ_RESULT_SUCCESS) ? (-EBUSY) : (0);
}

static int tz_resume(struct device *pdev)
{
	int tzret;

	tzret = kree_pm_device_ops(MTEE_RESUME);
	return (tzret != TZ_RESULT_SUCCESS) ? (-EBUSY) : (0);
}
#endif

static int tz_suspend_late(struct device *pdev)
{
	int tzret;

	tzret = kree_pm_device_ops(MTEE_SUSPEND_LATE);
	return (tzret != TZ_RESULT_SUCCESS) ? (-EBUSY) : (0);
}

static int tz_resume_early(struct device *pdev)
{
	int tzret;

	tzret = kree_pm_device_ops(MTEE_RESUME_EARLY);
	return (tzret != TZ_RESULT_SUCCESS) ? (-EBUSY) : (0);
}

static const struct dev_pm_ops tz_pm_ops = {
	.suspend_late = tz_suspend_late,
	.freeze_late = tz_suspend_late,
	.resume_early = tz_resume_early,
	.thaw_early = tz_resume_early,
	SET_SYSTEM_SLEEP_PM_OPS(tz_suspend, tz_resume)
};

#ifdef CONFIG_CMA
static struct cma *tz_cma;
#endif  /* CONFIG_CMA */

#ifdef CONFIG_MTEE_CMA_SECURE_MEMORY
static struct page *secure_pages;
static size_t secure_size;

/* TEE chunk memory allocate by REE service
 */
int KREE_ServGetChunkmemPool(u32 op,
			u8 uparam[REE_SERVICE_BUFFER_SIZE])
{
	struct ree_service_chunk_mem *chunkmem;

	if (!tz_cma)
		return TZ_RESULT_ERROR_OUT_OF_MEMORY;

	/* get parameters */
	chunkmem = (struct ree_service_chunk_mem *)uparam;

	if (secure_pages != NULL) {
		pr_warn("%s() already allocated!\n", __func__);
		return TZ_RESULT_ERROR_OUT_OF_MEMORY;
	}

	secure_size = cma_get_size(tz_cma);
	secure_pages = cma_alloc_large(tz_cma, secure_size/SZ_4K,
					get_order(SZ_1M));
	if (secure_pages == NULL) {
		pr_warn("%s() cma_alloc() failed!\n", __func__);
		return TZ_RESULT_ERROR_OUT_OF_MEMORY;
	}
	chunkmem->size = secure_size;
	chunkmem->chunkmem_pa = (uint64_t)page_to_phys(secure_pages);

	pr_warn("%s() get @%llx [0x%zx]\n", __func__,
			chunkmem->chunkmem_pa, secure_size);

	/* flush cache to avoid writing secure memory after allocation. */
	smp_inner_dcache_flush_all();

	return TZ_RESULT_SUCCESS;
}

int KREE_ServReleaseChunkmemPool(u32 op,
			u8 uparam[REE_SERVICE_BUFFER_SIZE])
{
	if (secure_pages != NULL) {
		phys_addr_t addr = page_to_phys(secure_pages);

		cma_release(tz_cma, secure_pages,
				cma_get_size(tz_cma)>>PAGE_SHIFT);
		pr_warn("%s() release @%pax [0x%zx]\n", __func__,
				&addr, secure_size);
		secure_pages = NULL;
		secure_size = 0;
		return TZ_RESULT_SUCCESS;
	}
	return TZ_RESULT_ERROR_GENERIC;
}

#ifndef NO_CMA_RELEASE_THROUGH_SHRINKER_FOR_EARLY_STAGE

int KREE_TeeReleseChunkmemPool(void)
{
	int ret;
	KREE_SESSION_HANDLE mem_session;

	ret = KREE_CreateSession(TZ_TA_MEM_UUID, &mem_session);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("Create memory session error %d\n", ret);
		return ret;
	}

	ret = KREE_TeeServiceCall(mem_session, TZCMD_MEM_RELEASE_SECURECM,
					TZPT_NONE, NULL);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("Release Secure CM failed, ret = %d\n", ret);

	KREE_CloseSession(mem_session);

	return ret;
}

DECLARE_COMPLETION(tz_cm_shrinker_work);
DECLARE_COMPLETION(tz_cm_shrinker_finish_work);

int tz_cm_shrinker_thread(void *data)
{
	do {
		if (wait_for_completion_interruptible(
						&tz_cm_shrinker_work) != 0)
			continue;

		/* TeeReleaseChunkmemPool will call     */
		/* ree-service call to do cma_release() */
		if (KREE_TeeReleseChunkmemPool() != TZ_RESULT_SUCCESS)
			pr_warn("Can't free tz chunk memory\n");
		else
			pr_debug("free tz chunk memory successfully\n");

		complete(&tz_cm_shrinker_finish_work);
	} while (1);
}

static int KREE_IsTeeChunkmemPoolReleasable(int *releasable)
{
	int ret;
	KREE_SESSION_HANDLE mem_session;
	union MTEEC_PARAM param[4];

	if (releasable == NULL)
		return TZ_RESULT_ERROR_BAD_FORMAT;

	ret = KREE_CreateSession(TZ_TA_MEM_UUID, &mem_session);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("Create memory session error %d\n", ret);
		return ret;
	}

	ret = KREE_TeeServiceCall(mem_session, TZCMD_MEM_USAGE_SECURECM,
					TZPT_VALUE_OUTPUT, param);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("Is Secure CM releasable failed, ret = %d\n", ret);

	*releasable = param[0].value.a;

	KREE_CloseSession(mem_session);

	return ret;
}

static unsigned long tz_cm_count(struct shrinker *s,
						struct shrink_control *sc)
{
	int releasable;

	if (secure_pages == NULL)
		return 0;

	if (KREE_IsTeeChunkmemPoolReleasable(&releasable) != TZ_RESULT_SUCCESS)
		return 0;

	if (releasable == 0)
		return 0;

	return secure_size / SZ_4K;
}
static unsigned long tz_cm_scan(struct shrinker *s,
						struct shrink_control *sc)
{
	unsigned long release_objects_count = secure_size;

	if (secure_pages == NULL)
		return SHRINK_STOP;

	complete(&tz_cm_shrinker_work);
	wait_for_completion(&tz_cm_shrinker_finish_work);

	if (secure_pages != NULL)
		return SHRINK_STOP;

	return release_objects_count;
}

static struct shrinker tz_cm_shrinker = {
	.scan_objects = tz_cm_scan,
	.count_objects = tz_cm_count,
	.seeks = DEFAULT_SEEKS
};

#else

static int KREE_TeeTriggerChunkmemAllocation(void)
{
	int ret;
	KREE_SESSION_HANDLE mem_session;
	KREE_SECUREMEM_HANDLE cm_handle;

	ret = KREE_CreateSession(TZ_TA_MEM_UUID, &mem_session);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("Create memory session error %d\n", ret);
		return ret;
	}

	ret = KREE_AllocSecurechunkmem(mem_session, &cm_handle, 1, 4096);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("Allocate securechunkmem error %d\n", ret);
		return ret;
	}

	ret = KREE_UnreferenceSecurechunkmem(mem_session, cm_handle);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("Unreference securechunkmem error %d\n", ret);
		return ret;
	}

	KREE_CloseSession(mem_session);

	return ret;
}
#endif  /* NO_CMA_RELEASE_THROUGH_SHRINKER_FOR_EARLY_STAGE */

#endif

/* TEE Kthread create by REE service, TEE service call body
 */
static int kree_thread_function(void *arg)
{
	union MTEEC_PARAM param[4];
	int ret;
	struct ree_service_thrd *info = (struct ree_service_thrd *)arg;
	KREE_SESSION_HANDLE local_syssess;

	ret = KREE_CreateSession(PTA_SYSTEM_UUID_STRING, &local_syssess);

	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("Create session for system failed!\n");
		return -EINVAL;
	}

	param[0].value.a = (uint32_t)info->handle;

	/* free parameter resource */
	kfree(info);

	/* create TEE kthread */
	ret = KREE_TeeServiceCall(local_syssess,
				  TZCMD_SYS_THREAD_CREATE,
				  TZ_ParamTypes1(TZPT_VALUE_INPUT),
				  param);

	KREE_CloseSession(local_syssess);
	return ret;
}

/* TEE Kthread create by REE service
 */
static int KREE_ServThreadCreate(u32 op,
			u8 uparam[REE_SERVICE_BUFFER_SIZE])
{
	struct ree_service_thrd *info;

	/* get parameters */
	/* the resource will be freed when the thread starts */
	info = (struct ree_service_thrd *)
		kmalloc(sizeof(struct ree_service_thrd), GFP_KERNEL);
	if (info == NULL)
		return TZ_RESULT_ERROR_OUT_OF_MEMORY;

	memcpy(info, uparam, sizeof(struct ree_service_thrd));

	/* create thread and run */
	if (!kthread_run(kree_thread_function, info, info->name))
		return TZ_RESULT_ERROR_GENERIC;

	return TZ_RESULT_SUCCESS;
}

static int ree_service(void *data)
{
	int ret;
	union MTEEC_PARAM param[4];
	KREE_SHAREDMEM_HANDLE shmh;
	struct KREE_SHAREDMEM_PARAM shmparam;
	KREE_SESSION_HANDLE local_syssess;

	shmparam.size = 512;
	shmparam.buffer = kmalloc(shmparam.size, GFP_KERNEL);
	if (shmparam.buffer == NULL) {
		pr_warn("%s: out of memory!\n", __func__);
		return -ENOMEM;
	}

	ret = KREE_RegisterSharedmem(0 /* optee: dont need mem session */,
				     &shmh,
				     &shmparam);

	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("Register shared memory failed!\n");
		return -EINVAL;
	}

	ret = KREE_CreateSession(PTA_SYSTEM_UUID_STRING, &local_syssess);

	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("Create session for system failed!\n");
		return -EINVAL;
	}

	param[1].memref.handle = shmh;
	param[1].memref.offset = 0;
	param[1].memref.size = shmparam.size;

	while (true) {
		ret = KREE_TeeServiceCall(local_syssess,
					  TZCMD_SYS_REE_CALLBACK,
					  TZ_ParamTypes2(
						TZPT_VALUE_OUTPUT,
						TZPT_MEMREF_OUTPUT),
					  param);
		if (ret != TZ_RESULT_SUCCESS) {
			pr_warn("ree service call failed!\n");
			return -EINVAL;
		}
		switch (param[0].value.a) {
		case REE_SERV_PUTS:
			pr_warn("%s", (char *)shmparam.buffer);
		break;
		case REE_SERV_REQUEST_IRQ:
			KREE_ServRequestIrq(REE_SERV_REQUEST_IRQ,
					    shmparam.buffer);
		break;
		case REE_SERV_ENABLE_IRQ:
			KREE_ServEnableIrq(REE_SERV_REQUEST_IRQ,
					    shmparam.buffer);
		break;
		case REE_SERV_ENABLE_CLOCK:
			KREE_ServEnableClock(REE_SERV_ENABLE_CLOCK,
					     shmparam.buffer);
		break;
		case REE_SERV_DISABLE_CLOCK:
			KREE_ServDisableClock(REE_SERV_DISABLE_CLOCK,
					     shmparam.buffer);
		break;
		case REE_SERV_THREAD_CREATE:
			KREE_ServThreadCreate(REE_SERV_THREAD_CREATE,
					      shmparam.buffer);
		break;
		}
	}

	return 0;
}


static int mtee_probe(struct platform_device *pdev)
{
	int tzret;
	struct task_struct *ree_thread;
#ifdef ENABLE_INC_ONLY_COUNTER
	struct task_struct *thread;
#endif
#ifdef TZ_SECURETIME_SUPPORT
	struct task_struct *thread_securetime_gb;
#endif
#ifdef TZ_PLAYREADY_SECURETIME_SUPPORT
	struct task_struct *thread_securetime;
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&securetime_early_suspend);
#endif
#endif

#ifdef CONFIG_OF
	struct device_node *parent_node;

	if (pdev->dev.of_node) {
		parent_node = of_irq_find_parent(pdev->dev.of_node);
		if (parent_node)
			kree_set_sysirq_node(parent_node);
		else
			pr_warn("can't find interrupt-parent device node from mtee\n");
	} else
		pr_warn("No mtee device node\n");

	mtee_clks_init(pdev);
	mtee_pms_init(pdev);
#endif /* CONFIG_OF */

	tzret = KREE_InitTZ();

	if (tzret != TZ_RESULT_SUCCESS) {
		pr_warn("tz_client_init: TZ Failed %d\n", (int)tzret);
		WARN_ON(tzret != TZ_RESULT_SUCCESS);
	}

	ree_thread = kthread_run(ree_service, NULL, "ree_service");

	tzret = KREE_TeeServiceCall(MTEE_SESSION_HANDLE_SYSTEM,
				  TZCMD_SYS_INIT,
				  TZPT_NONE,
				  NULL);

	if (tzret != TZ_RESULT_SUCCESS) {
		pr_warn("TZ Sys Init failed!\n");
		return -EINVAL;
	}

	pr_debug("tz_client_init: successfully\n");
	//kree_irq_init();
	//kree_pm_init();

#ifdef ENABLE_INC_ONLY_COUNTER
	thread = kthread_run(update_counter_thread, NULL, "update_tz_counter");
#endif

	/* tz_test(); */

#ifdef TZ_PLAYREADY_SECURETIME_SUPPORT
	thread_securetime = kthread_run(update_securetime_thread, NULL,
						"update_securetime");
#endif

#ifdef TZ_SECURETIME_SUPPORT
	thread_securetime_gb = kthread_run(update_securetime_thread_gb, NULL,
						"update_securetime_gb");
#endif

#ifdef CONFIG_MTEE_CMA_SECURE_MEMORY
#ifndef NO_CMA_RELEASE_THROUGH_SHRINKER_FOR_EARLY_STAGE
	thread_tz_cm_shrinker = kthread_run(tz_cm_shrinker_thread, NULL,
						"tz_cm_shrinker");
	register_shrinker(&tz_cm_shrinker);
#endif  /* NO_CMA_RELEASE_THROUGH_SHRINKER_FOR_EARLY_STAGE */
#endif  /* CONFIG_MTEE_CMA_SECURE_MEMORY */

#if defined(CONFIG_MTEE_CMA_SECURE_MEMORY) && \
	!defined(NO_CMA_RELEASE_THROUGH_SHRINKER_FOR_EARLY_STAGE)
	struct task_struct *thread_tz_cm_shrinker;
#endif

	return 0;
}

static const struct of_device_id mtee_of_match[] = {
	{ .compatible = "mediatek,mtee", },
	{}
};
MODULE_DEVICE_TABLE(of, mtee_of_match);

/* add tz virtual driver for suspend/resume support */
static struct platform_driver tz_driver = {
	.probe = mtee_probe,
	.driver = {
		.name = TZ_DEVNAME,
		.pm = &tz_pm_ops,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtee_of_match,
#endif
	},
#ifdef TZ_PLAYREADY_SECURETIME_SUPPORT
	.shutdown   = st_shutdown,
#endif
};

/* CMA */
#ifdef CONFIG_CMA
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_fdt.h>

static int __init rmem_tz_sec_setup(struct reserved_mem *rmem)
{
	unsigned long node = rmem->fdt_node;
	struct cma *cma;
	int err;

	if (!of_get_flat_dt_prop(node, "reusable", NULL) ||
	    of_get_flat_dt_prop(node, "no-map", NULL))
		return -EINVAL;

	if (!rmem->size) {
		pr_err("%s: size == 0\n", __func__);
		return -EINVAL;
	}

	err = cma_init_reserved_mem(rmem->base, rmem->size, 0, "tz_cma", &cma);
	if (err) {
		pr_err("Reserved memory: unable to setup CMA region\n");
		return err;
	}

	tz_cma = cma;
	pr_info("Reserved memory: created CMA memory pool at %pa, size %ld MiB\n",
		&rmem->base, (unsigned long)rmem->size / SZ_1M);

	return 0;
}
RESERVEDMEM_OF_DECLARE(tz_sec, "shared-secure-pool", rmem_tz_sec_setup);
#endif  /* CONFIG_CMA */

/******************************************************************************
 * register_tz_driver
 *
 * DESCRIPTION:
 *   register the device driver !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   0 for success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static int __init register_tz_driver(void)
{
	int ret = 0;

#ifndef CONFIG_OF
	if (platform_device_register(&tz_device)) {
		ret = -ENODEV;
		pr_warn("[%s] could not register device for the device, ret:%d\n",
			MODULE_NAME,
			ret);
		return ret;
	}
#endif

	if (platform_driver_register(&tz_driver)) {
		ret = -ENODEV;
		pr_warn("[%s] could not register device for the device, ret:%d\n",
			MODULE_NAME,
			ret);
#ifndef CONFIG_OF
		platform_device_unregister(&tz_device);
#endif
		return ret;
	}

	return ret;
}
#ifdef TZ_PLAYREADY_SECURETIME_SUPPORT

#ifdef CONFIG_HAS_EARLYSUSPEND

static void st_early_suspend(struct early_suspend *h)
{
	pr_debug("%s: start\n", __func__);
	securetime_savefile();
}

static void st_late_resume(struct early_suspend *h)
{
	int ret = 0;
	KREE_SESSION_HANDLE securetime_session = 0;

	ret = KREE_CreateSession(TZ_TA_PLAYREADY_UUID, &securetime_session);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("[securetime]CreateSession error %d\n", ret);

	TEE_update_pr_time_intee(securetime_session);
	ret = KREE_CloseSession(securetime_session);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("[securetime]CloseSession error %d\n", ret);
}

static struct early_suspend securetime_early_suspend = {
	.level  = 258,
	.suspend = st_early_suspend,
	.resume  = st_late_resume,
};
#endif
#endif

/******************************************************************************
 * tz_client_init
 *
 * DESCRIPTION:
 *   Init the device driver !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   0 for success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static int __init tz_client_init(void)
{
	int ret = 0;

	ret = register_tz_driver();
	if (ret) {
		pr_warn("[%s] register device/driver failed, ret:%d\n",
			MODULE_NAME,
			ret);
		return ret;
	}

	return 0;
}
module_init(tz_client_init);

#ifdef NO_CMA_RELEASE_THROUGH_SHRINKER_FOR_EARLY_STAGE
static int __init tz_client_cma_alloc_init(void)
{
	/* trigger allocation chunk memory in initialization flow */
	KREE_TeeTriggerChunkmemAllocation();
	return 0;
}
/* use late_initcall to be triggered after M4U init is done. */
late_initcall(tz_client_cma_alloc_init);
#endif
