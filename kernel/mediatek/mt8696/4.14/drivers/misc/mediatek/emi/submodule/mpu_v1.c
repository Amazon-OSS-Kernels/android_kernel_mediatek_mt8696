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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/printk.h>
#include <linux/memblock.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/mtk_meminfo.h>
#include <mt-plat/mtk_ccci_common.h>
#include <mt-plat/mtk_secure_api.h>
#ifdef CONFIG_MTK_AEE_FEATURE
#include <mt-plat/aee.h>
#endif

#include <mt_emi.h>
#include "mpu_v1.h"
#include <mpu_platform.h>

#ifdef CONFIG_MTK_DEVMPU
#include <devmpu.h>
#endif

_Static_assert(EMI_MPU_DOMAIN_NUM <= 2048, "EMI_MPU_DOMAIN_NUM is over 2048");
_Static_assert(EMI_MPU_REGION_NUM <= 256, "EMI_MPU_REGION_NUM is over 256");

static void __iomem *CEN_EMI_BASE;

static void (*check_violation_cb)(void);
static const char *UNKNOWN_MASTER = "unknown";

static unsigned int match_id(unsigned int axi_id, unsigned int tbl_idx,
			     unsigned int port_id)
{
	if ((axi_id & mst_tbl[tbl_idx].id_mask) == mst_tbl[tbl_idx].id_val) {
		if (port_id == mst_tbl[tbl_idx].port)
			return 1;
	}

	return 0;
}

static const char *id2name(unsigned int axi_id, unsigned int port_id)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (match_id(axi_id, i, port_id))
			return mst_tbl[i].name;
	}

	return (char *)UNKNOWN_MASTER;
}

static void clear_violation(void)
{
#if 1
	unsigned int mpus = 0;
	unsigned int mput = 0;
	unsigned int i = 0;
#ifdef CONFIG_MTK_DEVMPU
	unsigned int mput_2nd = 0;
#endif

	/* clear violation status */
	for (i = 0; i < EMI_MPU_DOMAIN_NUM; i++) {
		/* clear region abort violation */
		mt_reg_sync_writel(0xFFFFFFFF, EMI_MPUD_ST(i));
		/* clear out-of-range violation */
		mt_reg_sync_writel(0x3, EMI_MPUD_ST2(i));
	}

	/* clear debug info */
	mt_reg_sync_writel(0x80000000, EMI_MPUS);
	mt_reg_sync_writel(0x40000000, EMI_MPUT_2ND);

	mpus = readl(IOMEM(EMI_MPUS));
	mput = readl(IOMEM(EMI_MPUT));

	if (mpus) {
		pr_info("[MPU] fail to clear violation\n");
		pr_info("[MPU] EMI_MPUS: %x, EMI_MPUT: %x\n", mpus, mput);
	}
#ifdef CONFIG_MTK_DEVMPU
	/* clear hyp violation status */
	mt_reg_sync_writel(0x40000000, EMI_MPUT_2ND);

	mput_2nd = readl(IOMEM(EMI_MPUT_2ND));
	if ((mput_2nd >> 21) & 0x3) {
		pr_info("[MPU] fail to clear hypervisor violation\n");
		pr_info("[MPU] EMI_MPT_2ND: %x\n", mput_2nd);
	}
#endif
#endif
}

static void check_violation(void)
{
	unsigned int mpus = 0, mput = 0, mput_2nd = 0;
	unsigned int master_id = 0, domain_id = 0;
	unsigned int port_id = 0, axi_id = 0;
	unsigned int region = 0;
	unsigned int wr_vio = 0, wr_oo_vio = 0;
	unsigned long long vio_addr = 0;
	const char *master_name;
#ifdef CONFIG_MTK_DEVMPU
	unsigned int hp_wr_vio = 0;
#endif

	mpus = readl(IOMEM(EMI_MPUS));
	mput = readl(IOMEM(EMI_MPUT));
	mput_2nd = readl(IOMEM(EMI_MPUT_2ND));
	vio_addr =
	    ((((unsigned long long)(mput_2nd & 0xF)) << 32) + mput +
	     DRAM_OFFSET);

	/* decode EMI_MPUS */
	master_id = (mpus & 0xFFFF) | ((mput_2nd & 0x000000F0) << 12);
	domain_id = ((mpus >> 21) & 0xF) | ((mput_2nd & 0x00010000) >> 12);
	region = (mpus >> 16) & 0x1F;
	wr_vio = (mpus >> 29) & 0x3;
	wr_oo_vio = (mpus >> 27) & 0x3;
	port_id = master_id & 0x7;
	axi_id = (master_id >> 3) & 0x1FFFF;
	master_name = id2name(axi_id, port_id);

#ifdef CONFIG_MTK_DEVMPU
	/* if is hyperviosr MPU violation, deliver to DevMPU */
	hp_wr_vio = (mput_2nd >> 21) & 0x3;
	if (hp_wr_vio) {
		devmpu_print_violation(vio_addr, master_id, domain_id,
		hp_wr_vio, true);
		clear_violation();
		return;
	}
#endif

	pr_info("[MPU] EMI MPU violation\n");
	pr_info("[MPU] MPUS: %x, MPUT: %x, MPUT_2ND: %x.\n", mpus, mput,
		mput_2nd);
	pr_info("[MPU] current process is \"%s \" (pid: %i)\n", current->comm,
		current->pid);
	pr_info("[MPU] corrupted address is 0x%llx, in region %d\n", vio_addr,
		region);
	pr_info("[MPU] master ID: 0x%x, AXI ID: 0x%x, port ID: 0x%x\n",
		master_id, axi_id, port_id);
	pr_info("[MPU] violation master is %s, from domain 0x%x\n", master_name,
		domain_id);

	if (wr_vio == 1)
		pr_info("[MPU] write violation\n");
	else if (wr_vio == 2)
		pr_info("[MPU] read violation\n");
	else
		pr_info("[MPU] strange write/read violation (%d)\n", wr_vio);
	if (wr_oo_vio == 1)
		pr_info("[MPU] write out-of-range violation\n");
	else if (wr_oo_vio == 2)
		pr_info("[MPU] read out-of-range violation\n");

#ifdef CONFIG_MTK_AEE_FEATURE
	if (wr_vio == 1) {
#if 0
		if (is_md_master(master_id)) {
			char str[CCCI_STR_MAX_LEN] = "0";

			snprintf(str, CCCI_STR_MAX_LEN,
				 "EMI_MPUS = 0x%x, ADDR = 0x%llx", mpus,
				 vio_addr);
			exec_ccci_kern_func_by_md_id(0, ID_MD_MPU_ASSERT, str,
						     strlen(str));

			pr_info("[MPU] violation trigger MD, ");
			pr_info("str=%s strlen(str)=%d\n", str,
				(int)strlen(str));
		}
#endif
		aee_kernel_exception("EMI MPU",
				     "%s%s = 0x%x,%s = 0x%x,%s = 0x%x,%s = 0x%llx\n%s%s\n",
				     "EMI MPU violation.\n",
				     "EMI_MPUS", mpus,
				     "EMI_MPUT", mput,
				     "EMI_MPUT_2ND", mput_2nd,
				     "vio_addr", vio_addr,
				     "CRDISPATCH_KEY:EMI MPU Violation Issue/",
				     master_name);
	}
#endif

	clear_violation();
}

static irqreturn_t violation_irq(int irq, void *dev_id)
{
	check_violation_cb();
	return IRQ_HANDLED;
}

#ifdef ENABLE_MPU_SLVERR
static void enable_slverr(void)
{
	unsigned int value = 0;
	unsigned int domain = 0;

	for (domain = 0; domain < EMI_MPU_DOMAIN_NUM; domain++) {
		value = emi_mpu_smc_read(EMI_MPU_CTRL_D(domain));
		emi_mpu_smc_write(EMI_MPU_CTRL_D(domain), value | 0x2);
	}
}
#endif

void mpu_init(struct platform_driver *emi_ctrl, struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	unsigned int mpu_irq = 0;
	int ret = 0;

	pr_info("[MPU] initialize EMI MPU\n");

	CEN_EMI_BASE = mt_cen_emi_base_get();

	if (!check_violation_cb)
		check_violation_cb = check_violation;
	if (readl(IOMEM(EMI_MPUS))) {
		pr_info("[MPU] detect violation in driver init\n");
		check_violation_cb();
	} else
		clear_violation();

	if (node) {
		mpu_irq = irq_of_parse_and_map(node, MPU_IRQ_INDEX);
		pr_info("[MPU] get MPU IRQ: %d\n", mpu_irq);

		ret = request_irq(mpu_irq, (irq_handler_t) violation_irq,
				  IRQF_TRIGGER_NONE, "mpu", emi_ctrl);
		if (ret != 0) {
			pr_info("[MPU] fail to request IRQ (%d)\n", ret);
			return;
		}
	}

#ifdef ENABLE_MPU_SLVERR
	enable_slverr();
#endif
}

int emi_mpu_check_register(void (*cb_func) (void))
{
	if (!cb_func) {
		pr_info("%s%d: cb_func is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	check_violation_cb = cb_func;
	return 0;
}

EXPORT_SYMBOL(emi_mpu_check_register);

void clear_md_violation(void)
{
	mt_reg_sync_writel(0x80000000, EMI_MPUT_2ND);
}

EXPORT_SYMBOL(clear_md_violation);
