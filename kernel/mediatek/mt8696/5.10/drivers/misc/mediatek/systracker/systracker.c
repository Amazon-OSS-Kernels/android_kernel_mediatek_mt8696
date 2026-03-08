// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/signal.h>
#include <linux/cpu.h>
#include <linux/io.h>
#include "systracker_v2.h"

void __iomem *BUS_DBG_BASE;
void __iomem *BUS_DBG_INFRA_BASE;
int systracker_irq;
struct systracker_config_t track_config;
struct systracker_entry_t track_entry;

static const struct of_device_id systracker_of_ids[] = {
	{ .compatible = "mediatek,bus_dbg-v2", },
	{}
};

static struct mt_systracker_driver mt_systracker_drv = {
	.driver = {
		.driver = {
			.name = "systracker",
			.bus = &platform_bus_type,
			.owner = THIS_MODULE,
			.of_match_table = systracker_of_ids,
		},
		.probe = systracker_probe,
		.remove = systracker_remove,
		.suspend = systracker_suspend,
		.resume = systracker_resume,
	},
	.device = {
		.name = "systracker",
	},
};

int systracker_probe(struct platform_device *pdev)
{
	void __iomem *infra_ao_base;
	unsigned int bus_dbg_con_offset;

	pr_notice("systracker probe\n");

	/* iomap register */
	BUS_DBG_BASE = of_iomap(pdev->dev.of_node, 0);
	if (!BUS_DBG_BASE) {
		pr_notice("can't of_iomap for systracker!!\n");
		return -ENOMEM;
	}

	pr_notice("of_iomap for systracker @ 0x%p\n", BUS_DBG_BASE);

	infra_ao_base = of_iomap(pdev->dev.of_node, 1);
	if (!infra_ao_base) {
		pr_notice("[systracker] bus_dbg_con is in infra\n");
		BUS_DBG_INFRA_BASE = BUS_DBG_BASE;
	} else {
		pr_notice("[systracker] bus_dbg_con is in infra_ao\n");
		if (of_property_read_u32
			(pdev->dev.of_node, "mediatek,bus_dbg_con_offset",
			&bus_dbg_con_offset)) {
			pr_notice
			("[systracker] cannot get bus_dbg_con_offset\n");
			return -ENODEV;
		}
		BUS_DBG_INFRA_BASE = infra_ao_base + bus_dbg_con_offset;
	}

	/* get irq #  */
	systracker_irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	pr_notice("%s:%d: irq # %d\n", __func__, __LINE__, systracker_irq);

#if IS_ENABLED(CONFIG_MTK_BUS_TRACER)
	pr_notice("[systracker] register isr for bus tracer\n");
	if (request_irq(systracker_irq, (irq_handler_t)systracker_isr,
				IRQF_TRIGGER_NONE, "SYSTRACKER", NULL)) {
		pr_notice("SYSTRACKER IRQ LINE NOT AVAILABLE!!\n");
		return -1;
	}
#endif

	/* save entry info */
	save_entry();
	memset(&track_config, 0, sizeof(struct systracker_config_t));

	track_config.enable_timeout = 1;
	track_config.enable_slave_err = 1;
	track_config.enable_irq = 1;
	track_config.timeout_ms = 100;
	track_config.timeout2_ms = 2000;

	systracker_reset();
	systracker_enable();

	return 0;
}

int systracker_remove(struct platform_device *pdev)
{
	return 0;
}

int systracker_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int systracker_resume_default(struct platform_device *pdev)
{
	if (track_config.state || track_config.enable_wp)
		systracker_enable();

	return 0;
}

int systracker_resume(struct platform_device *pdev)
{
	return systracker_resume_default(pdev);
}

void save_entry(void)
{
	int i = 0;

	track_entry.dbg_con =  readl(BUS_DBG_CON);
	track_entry.dbg_con_infra =  readl(BUS_DBG_CON_INFRA);

	for (i = 0; i < BUS_DBG_NUM_TRACKER; i++) {
		track_entry.ar_track_l[i]   =
			readl(BUS_DBG_AR_TRACK_L(i));
		track_entry.ar_track_h[i]   =
			readl(BUS_DBG_AR_TRACK_H(i));
		track_entry.ar_trans_tid[i] =
			readl(BUS_DBG_AR_TRANS_TID(i));
		track_entry.aw_track_l[i]   =
			readl(BUS_DBG_AW_TRACK_L(i));
		track_entry.aw_track_h[i]   =
			readl(BUS_DBG_AW_TRACK_H(i));
		track_entry.aw_trans_tid[i] =
			readl(BUS_DBG_AW_TRANS_TID(i));
	}

	track_entry.w_track_data6 = readl(BUS_DBG_W_TRACK_DATA6);
	track_entry.w_track_data7 = readl(BUS_DBG_W_TRACK_DATA7);
	track_entry.w_track_data_valid =
		readl(BUS_DBG_W_TRACK_DATA_VALID);
}

irqreturn_t systracker_isr(void)
{
	return IRQ_HANDLED;
}

static int systracker_watchpoint_enable_default(void)
{
	unsigned int con;

	track_config.enable_wp = 1;

	writel(track_config.wp_phy_address, BUS_DBG_WP);
	writel(0x00000000, BUS_DBG_WP_MASK);

	con = readl(BUS_DBG_CON_INFRA) | BUS_DBG_CON_WP_EN;
	writel(con, BUS_DBG_CON_INFRA);
	/* ensure access complete */
	mb();

	return 0;
}

int systracker_watchpoint_enable(void)
{
	return systracker_watchpoint_enable_default();
}

static int systracker_watchpoint_disable_default(void)
{
	track_config.enable_wp = 0;
	writel(readl(BUS_DBG_CON_INFRA) & (~BUS_DBG_CON_WP_EN),
		BUS_DBG_CON_INFRA);
	/* ensure access complete */
	mb();

	return 0;
}

int systracker_watchpoint_disable(void)
{
	return systracker_watchpoint_disable_default();
}

void systracker_reset_default(void)
{
	writel(readl(BUS_DBG_CON) |
		BUS_DBG_CON_SW_RST, BUS_DBG_CON);
	writel(readl(BUS_DBG_CON) |
		BUS_DBG_CON_IRQ_CLR, BUS_DBG_CON);
	writel(readl(BUS_DBG_CON) |
		BUS_DBG_CON_TIMEOUT_CLR, BUS_DBG_CON);
	/* ensure access complete */
	mb();
}

void systracker_reset(void)
{
	systracker_reset_default();
}

unsigned int systracker_timeout_value_default(void)
{
	/* prescale = (133 * (10 ^ 6)) / 16 = 8312500/s */
	return (BUS_DBG_BUS_MHZ * 1000 / 16) * track_config.timeout_ms;
}

unsigned int systracker_timeout2_value_default(void)
{
	/* prescale = (133 * (10 ^ 6)) / 16 = 8312500/s */
	return (BUS_DBG_BUS_MHZ * 1000 / 16) * track_config.timeout2_ms;
}

unsigned int systracker_timeout_value(void)
{
	return systracker_timeout_value_default();
}

unsigned int systracker_timeout2_value(void)
{
	return systracker_timeout2_value_default();
}

void systracker_enable_default(void)
{
	unsigned int con;
	unsigned int timer_control_value;

	timer_control_value = systracker_timeout_value();
	writel(timer_control_value, BUS_DBG_TIMER_CON0);

	timer_control_value = systracker_timeout2_value();
	writel(timer_control_value, BUS_DBG_TIMER_CON1);

	track_config.state = 1;
	con = BUS_DBG_CON_BUS_DBG_EN | BUS_DBG_CON_BUS_OT_EN;
	if (track_config.enable_timeout)
		con |= BUS_DBG_CON_TIMEOUT_EN;

	if (track_config.enable_slave_err)
		con |= BUS_DBG_CON_SLV_ERR_EN;

	if (track_config.enable_irq) {
		con |= BUS_DBG_CON_IRQ_EN;
		con &= ~BUS_DBG_CON_IRQ_WP_EN;
	}

	con |= BUS_DBG_CON_HALT_ON_EN;
	writel(con, BUS_DBG_CON_INFRA);
	/* ensure access complete */
	mb();
}

void systracker_enable(void)
{
	systracker_enable_default();
}

void enable_systracker(void)
{
	systracker_enable();
}

static void systracker_disable_default(void)
{
	track_config.state = 0;
	writel(readl(BUS_DBG_CON_INFRA) &
		~BUS_DBG_CON_BUS_DBG_EN, BUS_DBG_CON_INFRA);
	/* ensure access complete */
	mb();
}

void systracker_disable(void)
{
	systracker_disable_default();
}

int systracker_test_init(void)
{
	if (mt_systracker_drv.systracker_test_init)
		return mt_systracker_drv.systracker_test_init();

	pr_notice("mt_systracker_drv.%s is NULL", __func__);
	return -1;
}

struct mt_systracker_driver *get_mt_systracker_drv(void)
{
	return &mt_systracker_drv;
}

static ssize_t tracker_run_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE,
		"BUS_DBG_CON=0x%x, BUS_DBG_CON_INFRA=0x%x\n",
			readl(BUS_DBG_CON),
			readl(BUS_DBG_CON_INFRA));
}

static ssize_t tracker_run_store(struct device_driver *driver,
		const char *buf, size_t count)
{
	unsigned int value;

	if (kstrtou32(buf, 10, &value))
		return -EINVAL;

	if (value == 1)
		systracker_enable();
	else if (value == 0)
		systracker_disable();
	else
		return -EINVAL;

	return count;
}

DRIVER_ATTR_RW(tracker_run);

static ssize_t enable_wp_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%x\n", track_config.enable_wp);
}

static ssize_t enable_wp_store(struct device_driver *driver,
		const char *buf, size_t count)
{
	unsigned int value;

	if (kstrtou32(buf, 10, &value))
		return -EINVAL;

	if (value == 1)
		systracker_watchpoint_enable();
	else if (value == 0)
		systracker_watchpoint_disable();
	else
		return -EINVAL;

	return count;
}

static DRIVER_ATTR_RW(enable_wp);

static ssize_t set_wp_address_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%x\n", track_config.wp_phy_address);
}

int systracker_set_watchpoint_addr(unsigned int addr)
{
	if (mt_systracker_drv.set_watchpoint_address)
		return mt_systracker_drv.set_watchpoint_address(addr);

	track_config.wp_phy_address = addr;

	return 0;
}

static ssize_t set_wp_address_store
	(struct device_driver *driver, const char *buf, size_t count)
{
	unsigned int value;
	int ret;

	ret = kstrtou32(buf, 16, &value);
	pr_debug("watch address:0x%x, ret = %d\n", value, ret);
	systracker_set_watchpoint_addr(value);

	return count;
}

static DRIVER_ATTR_RW(set_wp_address);

static ssize_t tracker_swtrst_show(struct device_driver *driver, char *buf)
{
	return 0;
}

static ssize_t tracker_swtrst_store
	(struct device_driver *driver, const char *buf, size_t count)
{
	writel(readl(BUS_DBG_CON) |
		BUS_DBG_CON_SW_RST, BUS_DBG_CON);
	return count;
}

static DRIVER_ATTR_RW(tracker_swtrst);

#ifdef SYSTRACKER_TEST_SUIT
/*top axi protect module control register*/
void __iomem *ctrl_bus_reg;
/*read/write this addr make bus timeout*/
void __iomem *test_area;

#if IS_ENABLED(CONFIG_ARM64)
#define IOMEM(a)	((void __force __iomem *)((a)))
#endif

unsigned int prot_reg, test_addr, bits;

static void systracker_platform_wp_test(void)
{
	void __iomem *ptr;
	/* use eint reg base as our watchpoint */
	ptr = ioremap(test_addr, 0x4);
	pr_debug("%s:%d: we got p = 0x%p\n", __func__, __LINE__, ptr);
	systracker_set_watchpoint_addr(test_addr);
	systracker_watchpoint_enable();
	/* touch it */
	writel(0, ptr);
	pr_debug("after we touched watchpoint\n");
	iounmap(ptr);
}

static void systracker_platform_read_timeout_test(void)
{
	/* FIXME: testing
	 * track_config.enable_slave_err = 0;
	 * systracker_enable();
	 */
	pr_notice("we are going to have read timeout\n");
	writel(readl(ctrl_bus_reg) | (0x1 << bits), ctrl_bus_reg);
	readl(test_area);
	writel(readl(ctrl_bus_reg) & ~(0x1 << bits), ctrl_bus_reg);
}

static void systracker_platform_write_timeout_test(void)
{
	pr_notice("we are going to have write timeout\n");
	writel(readl(ctrl_bus_reg) | (0x1 << bits), ctrl_bus_reg);
	writel(0x0, test_area);
	writel(readl(ctrl_bus_reg) & ~(0x1 << bits), ctrl_bus_reg);
	pr_debug("out write timeout:%s:%d\n", __func__, __LINE__);
}

static void systracker_platform_withrecord_test(void)
{
	writel(readl(IOMEM(BUS_DBG_CON)) |
	       BUS_DBG_CON_HALT_ON_EN, IOMEM(BUS_DBG_CON));
	writel(readl(ctrl_bus_reg) | (0x1 << bits), ctrl_bus_reg);
	readl(test_area);
}

static void systracker_platform_notimeout_test(void)
{
	writel(readl(ctrl_bus_reg) | (0x1 << bits), ctrl_bus_reg);
	/* disable timeout */
	writel(readl(IOMEM(BUS_DBG_CON_INFRA)) &
		~(BUS_DBG_CON_TIMEOUT_EN), IOMEM(BUS_DBG_CON_INFRA));
	/* read it, should cause bus hang */
	readl(test_area);
	/* never come back */
	pr_notice("failed??\n");
}

/*Before test bus timeout ,must first test_suit init*/
static ssize_t test_suit_init_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "==Systracker test init==\n"
		"echo prot_reg test_addr bits >/sys/bus/platform/drivers/systracker/test_suit_init\n");
}

static ssize_t test_suit_init_store
	(struct device_driver *driver, const char *buf, size_t count)
{
	char *p = (char *)buf, *arg;
	int i = 0, j = 0;
	unsigned long input;

	while ((arg = strsep(&p, " ")) && (i <= 2)) {
		if (i <= 1)
			j = 16;
		else
			j = 10;
		if (kstrtoul(arg, j, &input) != 0) {
			pr_notice("%s: kstrtoul fail for %s\n", __func__, p);
			return 0;
		}
		switch (i) {
		case 0:
			prot_reg = input;
			break;
		case 1:
			test_addr = input;
			break;
		case 2:
			bits = input;
			break;
		default:
			break;
		}
		i++;
	}
	ctrl_bus_reg = ioremap(prot_reg, 0x4);

	test_area = ioremap(test_addr, 0x4);
	return count;
}

DRIVER_ATTR_RW(test_suit_init);

static ssize_t test_suit_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "==Systracker test==\n"
			"%d: Systracker watchpoint test\n"
			"%d: Systracker read timeout test\n"
			"%d: Systracker write timeout test\n"
			"%d: Systracker timeout with record test\n"
			"%d: Systracker no timeout test\n",
			TRACKER_WP_TEST,
			READ_TIMEOUT_TEST,
			WRITE_TIMEOUT_TEST,
			TRACKER_RECORD_TEST,
			NOTIMEOUT_TEST);
}

static ssize_t test_suit_store
	(struct device_driver *driver, const char *buf, size_t count)
{
	char *p = (char *)buf;
	unsigned long num;

	if (kstrtoul(p, 10, &num) != 0) {
		pr_notice("[Tracker] kstrtoul fail for %s\n", p);
		return 0;
	}

	if ((prot_reg != 0) && (test_addr != 0)) {
		switch (num) {
		/* Test Systracker Function */
		case TRACKER_WP_TEST:
			systracker_platform_wp_test();
			break;
		case READ_TIMEOUT_TEST:
			systracker_platform_read_timeout_test();
			break;
		case WRITE_TIMEOUT_TEST:
			systracker_platform_write_timeout_test();
			break;
		case TRACKER_RECORD_TEST:
			systracker_platform_withrecord_test();
			break;
		case NOTIMEOUT_TEST:
			systracker_platform_notimeout_test();
			break;
		default:
			break;
		}
	} else {
		pr_notice("please first test_suit_init\n");
	}
	return count;
}
DRIVER_ATTR_RW(test_suit);
#endif

static ssize_t tracker_last_status_show
	(struct device_driver *driver, char *buf)
{

	if (track_entry.dbg_con & BUS_DBG_CON_TIMEOUT)
		return snprintf(buf, PAGE_SIZE, "1\n");
	else
		return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t tracker_last_status_store
	(struct device_driver *driver, const char *buf, size_t count)
{
	return count;
}

static DRIVER_ATTR_RW(tracker_last_status);

/*
 * driver initialization entry point
 */
static int __init systracker_init(void)
{
	int err = 0;
	int ret = 0;

	err = platform_driver_register(&mt_systracker_drv.driver);
	if (err)
		return err;

	/* Create sysfs entry */
	ret |= driver_create_file(&mt_systracker_drv.driver.driver,
		&driver_attr_tracker_run);
	ret |= driver_create_file(&mt_systracker_drv.driver.driver,
		&driver_attr_enable_wp);
	ret |= driver_create_file(&mt_systracker_drv.driver.driver,
		&driver_attr_set_wp_address);
	ret |= driver_create_file(&mt_systracker_drv.driver.driver,
		&driver_attr_tracker_swtrst);
	ret |= driver_create_file(&mt_systracker_drv.driver.driver,
		&driver_attr_tracker_last_status);
#ifdef SYSTRACKER_TEST_SUIT
	ret |= driver_create_file(&mt_systracker_drv.driver.driver,
		&driver_attr_test_suit_init);
	ret |= driver_create_file(&mt_systracker_drv.driver.driver,
		&driver_attr_test_suit);
#endif
	if (ret)
		pr_notice("Fail to create systracker_drv sysfs files");

	pr_debug("systracker init done\n");
	return 0;
}

/*
 * driver exit point
 */
static void __exit systracker_exit(void)
{
	platform_driver_unregister(&mt_systracker_drv.driver);
	/* remove sysfs entry */
	driver_remove_file(&mt_systracker_drv.driver.driver,
		&driver_attr_tracker_run);
	driver_remove_file(&mt_systracker_drv.driver.driver,
		&driver_attr_enable_wp);
	driver_remove_file(&mt_systracker_drv.driver.driver,
		&driver_attr_set_wp_address);
	driver_remove_file(&mt_systracker_drv.driver.driver,
		&driver_attr_tracker_swtrst);
	driver_remove_file(&mt_systracker_drv.driver.driver,
		&driver_attr_tracker_last_status);
#ifdef SYSTRACKER_TEST_SUIT
	driver_remove_file(&mt_systracker_drv.driver.driver,
		&driver_attr_test_suit_init);
	driver_remove_file(&mt_systracker_drv.driver.driver,
		&driver_attr_test_suit);
#endif

	pr_debug("systracker exit done\n");
}

module_init(systracker_init);
module_exit(systracker_exit);
MODULE_LICENSE("GPL v2");
