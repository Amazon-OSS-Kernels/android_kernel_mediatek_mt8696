// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <asm/arch_timer.h>
#include "stc.h"

static struct stc_instance instances[MAX_STC];
static struct stc_param g_stc_param;
static DEFINE_SPINLOCK(gpt_lock);
/*bitmap for allocated stc id*/
static DECLARE_BITMAP(stc_bitmap, MAX_STC);

#define COUNTER_27MHZ_TO_90KHZ(v)	(freq_cnt_convert(v))

static u64 read_gpt6_cnt(void)
{
	/*
	 * We use 64bit systimer(arch-counter) as stc counter source
	 * default 13M & always-on
	 * No suspend scene because video will be stop and also
	 * will stop stc by ioctl while the system going to suspend
	 */
	return arch_timer_read_counter();
}

static inline u64 get_27m_stc_cnt(void)
{
	u64 result;

	/*
	 * STC:90Khz, GPT6:13Mhz
	 * return 27MHz counter for MPEG specification.
	 */
	result = read_gpt6_cnt();
	result = (result) * 27;
	do_div(result, 13);

	return result;
}

static inline u64 freq_cnt_convert(u64 v)
{
	u64 result = v;

	do_div(result, 300);

	return result;
}

static int alloc_stc(unsigned int *id)
{
	unsigned long save_flags;

	if (g_stc_param.instance_cnt >= MAX_STC) {
		*id = -1;
		pr_notice("[STC] Error: No free stc hw.\n");
		return STC_FAIL;
	}

	/*
	 * [id = 0] <===> instances[0]
	 * [id = 1] <===> instances[1]
	 */
	spin_lock_irqsave(&gpt_lock, save_flags);
	*id = find_first_zero_bit(stc_bitmap, MAX_STC);
	if (*id >= MAX_STC) {
		pr_notice("[STC] Error: No free stc bit.\n");
		return STC_FAIL;
	}

	instances[*id].offset = 0;
	instances[*id].alloc = true;
	instances[*id].running = false;

	set_bit(*id, stc_bitmap);
	++g_stc_param.instance_cnt;
	spin_unlock_irqrestore(&gpt_lock, save_flags);

	pr_notice("[STC] an instance has been allocated, id = %d, cnt = %u\n",
			*id, g_stc_param.instance_cnt);

	return STC_OK;
}

static int free_stc(unsigned int id)
{
	unsigned long save_flags;

	if ((id > g_stc_param.instance_cnt) ||
			!instances[id].alloc) {
		pr_notice("[STC] Free wrong stc id: %u\n", id);
		return STC_FAIL;
	}

	spin_lock_irqsave(&gpt_lock, save_flags);
	instances[id].offset = 0;
	instances[id].alloc = false;
	instances[id].running = false;

	clear_bit(id, stc_bitmap);
	--g_stc_param.instance_cnt;
	spin_unlock_irqrestore(&gpt_lock, save_flags);

	pr_notice("[STC] an instance has been free, id = %d, cnt = %u\n",
			id, g_stc_param.instance_cnt);

	return STC_OK;
}

static void init_stc_hw(void)
{
	pr_notice("[%s] STC start...\n", __func__);
}

static int stop_stc(int id)
{
	if ((id < 0) || (id > g_stc_param.instance_cnt)
			|| !instances[id].alloc) {
		pr_notice("[STC] invalid stc id: %d in %s\n", id, __func__);
		return STC_FAIL;
	}

	if (instances[id].running) {
		instances[id].offset +=
			COUNTER_27MHZ_TO_90KHZ(get_27m_stc_cnt());
		instances[id].running = false;
	}

	if (enable_debug(g_stc_param.enable_flag))
		pr_notice("%s id = %d\n", __func__, id);

	return STC_OK;
}

static int start_stc(int id)
{
	if ((id < 0) || (id > g_stc_param.instance_cnt)
			|| !instances[id].alloc) {
		pr_notice("[STC] invalid stc id: %d in %s\n", id, __func__);
		return STC_FAIL;
	}

	if (!instances[id].running) {
		instances[id].offset -=
			COUNTER_27MHZ_TO_90KHZ(get_27m_stc_cnt());
		instances[id].running = true;
	}

	if (enable_debug(g_stc_param.enable_flag))
		pr_notice("%s id = %d\n", __func__, id);

	return STC_OK;
}

static int get_stc(struct mtk_stc_info *info)
{
	int id;

	if (info)
		id = info->stc_id;
	else
		goto err_out;

	if ((id < 0) || (id > g_stc_param.instance_cnt)
			|| !instances[id].alloc) {
		pr_notice("[%s] id%d err, please call alloc first\n",
				__func__, id);
		goto err_out;
	}

	info->stc_value = instances[id].offset + (instances[id].running ?
			COUNTER_27MHZ_TO_90KHZ(get_27m_stc_cnt()) : 0);

	if (enable_debug(g_stc_param.enable_flag))
		pr_notice("%s id = %d, stc_value = %lld\n",
				__func__, id, info->stc_value);

	return STC_OK;

err_out:
	WARN_ON(1);
	return STC_FAIL;
}

static int set_stc(struct mtk_stc_info *info)
{
	int id;

	if (info)
		id = info->stc_id;
	else
		goto err_out;

	if ((id < 0) || (id > g_stc_param.instance_cnt)
			|| !instances[id].alloc) {
		pr_notice("[%s] id%d err, please call alloc first\n",
				__func__, id);
		goto err_out;
	}

	instances[id].offset = info->stc_value - (instances[id].running ?
			COUNTER_27MHZ_TO_90KHZ(get_27m_stc_cnt()) : 0);

	if (enable_debug(g_stc_param.enable_flag))
		pr_notice("%s id = %d, stc_value = %lld\n",
				__func__, id, info->stc_value);

	return STC_OK;

err_out:
	WARN_ON(1);
	return STC_FAIL;
}

static int adjust_stc(struct mtk_stc_info *info)
{
	int64_t StcAdjustValue;
	int id;

	if (info)
		id = info->stc_id;
	else
		goto err_out;

	if ((id < 0) || (id > g_stc_param.instance_cnt)
			|| !instances[id].alloc) {
		pr_notice("[%s] id%d err, please call alloc first\n",
				__func__, id);
		goto err_out;
	}

	if (enable_debug(g_stc_param.enable_flag))
		pr_notice("%s id = %d, stc_value = %lld\n",
				__func__, id, info->stc_value);

	StcAdjustValue = info->stc_value;
	get_stc(info);
	info->stc_value += StcAdjustValue;
	set_stc(info);

	return STC_OK;

err_out:
	WARN_ON(1);
	return STC_FAIL;
}

static long mtk_stc_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;
	unsigned int stc_id = 0;
	struct mtk_stc_info info = {0};

	switch (cmd) {
	case MTK_STC_IOCTL_ALLOC:
		ret = alloc_stc(&stc_id);
		if (ret || put_user(stc_id, (int __user *)arg))
			ret = -EFAULT;
		break;

	case MTK_STC_IOCTL_FREE:
		if (get_user(stc_id, (int __user *)arg))
			return -EFAULT;
		ret = free_stc(stc_id);
		break;

	case MTK_STC_IOCTL_START:
		if (get_user(stc_id, (int __user *)arg))
			return -EFAULT;
		ret = start_stc(stc_id);
		break;

	case MTK_STC_IOCTL_STOP:
		if (get_user(stc_id, (int __user *)arg))
			return -EFAULT;
		ret = stop_stc(stc_id);
		break;

	case MTK_STC_IOCTL_SET:
		if (copy_from_user(&info, argp, sizeof(info)))
			return -EFAULT;
		ret = set_stc(&info);
		break;

	case MTK_STC_IOCTL_GET:
		if (copy_from_user(&info, argp, sizeof(info)))
			return -EFAULT;
		ret = get_stc(&info);
		if (ret || copy_to_user(argp, &info, sizeof(info)))
			ret = -EFAULT;
		break;

	case MTK_STC_IOCTL_ADJUST:
		if (copy_from_user(&info, argp, sizeof(info)))
			return -EFAULT;
		ret = adjust_stc(&info);
		break;

	default:
		break;
	}

	return ret;
}

static int mtk_stc_open(struct inode *inode, struct file *file)
{
	return STC_OK;
}

static int mtk_stc_release(struct inode *inode, struct file *file)
{
	return STC_OK;
}

static void stc_test_alloc(struct mtk_stc_info info1,
		struct mtk_stc_info info2)
{
	alloc_stc(&info1.stc_id);
	free_stc(info1.stc_id);
	alloc_stc(&info2.stc_id);
	free_stc(info2.stc_id);

	alloc_stc(&info1.stc_id);
	alloc_stc(&info2.stc_id);
	free_stc(info1.stc_id);
	free_stc(info2.stc_id);

	alloc_stc(&info1.stc_id);
	alloc_stc(&info2.stc_id);
	free_stc(info1.stc_id);
	alloc_stc(&info1.stc_id);
	free_stc(info2.stc_id);
	free_stc(info1.stc_id);
}

/*Notice: All test should check by log*/
static int stc_test(void *data)
{
	//int i;
	struct mtk_stc_info info1 = {0}, info2 = {0};

	//for (i = 0; i < 50; i++) {
		stc_test_alloc(info1, info2);

		alloc_stc(&info1.stc_id);
		alloc_stc(&info2.stc_id);

		get_stc(&info1);
		get_stc(&info2);
		pr_notice("[STC]T1 stc: %lld, id: %d.\n",
				info1.stc_value, info1.stc_id);
		pr_notice("[STC]T1 stc: %lld, id: %d.\n",
				info2.stc_value, info2.stc_id);

		start_stc(info1.stc_id);
		start_stc(info2.stc_id);

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ);
		stop_stc(info1.stc_id);
		stop_stc(info2.stc_id);
		get_stc(&info1);
		get_stc(&info2);
		pr_notice("[STC]T2 stc: %lld, id: %d.\n",
				info1.stc_value, info1.stc_id);
		pr_notice("[STC]T2 stc: %lld, id: %d.\n",
				info2.stc_value, info2.stc_id);

		start_stc(info1.stc_id);
		start_stc(info2.stc_id);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(3*HZ);
		pr_notice("[STC]T3 stc: %lld, id: %d.\n",
				info1.stc_value, info1.stc_id);
		pr_notice("[STC]T3 stc: %lld, id: %d.\n",
				info2.stc_value, info2.stc_id);

		info1.stc_value = 90000*100;
		info2.stc_value = 90000*100;
		set_stc(&info1);
		get_stc(&info1);
		set_stc(&info2);
		get_stc(&info2);
		pr_notice("[STC]T4 stc: %lld, id: %d.\n",
				info1.stc_value, info1.stc_id);
		pr_notice("[STC]T4 stc: %lld, id: %d.\n",
				info2.stc_value, info2.stc_id);

		info1.stc_value = 90000*500;
		info2.stc_value = 90000*500;
		adjust_stc(&info1);
		adjust_stc(&info2);
		get_stc(&info1);
		get_stc(&info2);
		pr_notice("[STC]T5 stc: %lld, id: %d.\n",
				info1.stc_value, info1.stc_id);
		pr_notice("[STC]T5 stc: %lld, id: %d.\n",
				info2.stc_value, info2.stc_id);

		stop_stc(info1.stc_id);
		stop_stc(info2.stc_id);
		start_stc(info1.stc_id);
		start_stc(info2.stc_id);

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(5*HZ);
		get_stc(&info1);
		get_stc(&info2);
		pr_notice("[STC]T6 stc: %lld, id: %d.\n",
				info1.stc_value, info1.stc_id);
		pr_notice("[STC]T6 stc: %lld, id: %d.\n",
				info2.stc_value, info2.stc_id);

		free_stc(info1.stc_id);
		free_stc(info2.stc_id);
	//}
	return STC_OK;
}

static ssize_t gpt_stc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return STC_OK;
}

static ssize_t gpt_stc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;

	/*just get debug flag*/
	if ((buf != NULL) && (count != 0)) {
		ret = kstrtouint(buf, 0, &g_stc_param.enable_flag);
		if (ret)
			pr_notice("%s get debug flags err!!!\n", __func__);
		else
			pr_notice("%s:enable_flag = %u\n",
					__func__, g_stc_param.enable_flag);
	}

	if (enable_test(g_stc_param.enable_flag)) {
		pr_notice("%s: start to run test thread...\n", __func__);
		kthread_run(stc_test, NULL, "stc_test");
	}
	return count;
}

/*
 * echo 0 > /sys/devices/platform/soc/...stc/gpt_stc
 *	==> just enable test flow
 * echo 1 > /sys/devices/platform/soc/...stc/gpt_stc
 *	==> just enable print debug info
 * echo 2 > /sys/devices/platform/soc/...stc/gpt_stc
 *	==> enable test flow & print debug info
 **/
static DEVICE_ATTR_RW(gpt_stc);

static const struct file_operations mtk_stc_fops = {
	.owner = THIS_MODULE,
	.open = mtk_stc_open,
	.unlocked_ioctl = mtk_stc_ioctl,
	.compat_ioctl = mtk_stc_ioctl,
	.release = mtk_stc_release,
};

static int stc_parse_dev_node(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	g_stc_param.gpt_base_addr = of_iomap(np, 0);
	if (!g_stc_param.gpt_base_addr) {
		pr_notice("%s: Can't find STC(GPT6) Base!\n",
				STC_SESSION_DEVICE);
		return STC_FAIL;
	}
	/*
	 * Here we use GPT6 hardware instead of STC hardware.
	 * Because GPT 13Mhz clocksource is initialized earlier,
	 * it is no longer necessary to continue initializatin now.
	 */
	return STC_OK;
}

static int mtk_stc_probe(struct platform_device *pdev)
{
	int i = 0, ret = 0;
	struct device *dev;

	ret = alloc_chrdev_region(&g_stc_param.stc_devno,
				0, 1, STC_SESSION_DEVICE);
	if (ret < 0) {
		pr_info("%s region alloc failed\n", __func__);
		return  ret;
	}

	g_stc_param.stc_cdev = cdev_alloc();
	g_stc_param.stc_cdev->owner = THIS_MODULE;
	g_stc_param.stc_cdev->ops = &mtk_stc_fops;

	ret = cdev_add(g_stc_param.stc_cdev, g_stc_param.stc_devno, 1);
	if (ret) {
		pr_info("%s cdev add failed\n", __func__);
		goto cdev_add_fail;
	}

	g_stc_param.stc_class = class_create(THIS_MODULE, STC_SESSION_DEVICE);
	if (IS_ERR(g_stc_param.stc_class)) {
		pr_info("%s class create failed\n", __func__);
		ret = PTR_ERR(g_stc_param.stc_class);
		goto class_create_fail;
	}

	dev = device_create(g_stc_param.stc_class, NULL, g_stc_param.stc_devno,
				NULL, STC_SESSION_DEVICE);
	if (IS_ERR(dev)) {
		pr_info("%s device create failed\n", __func__);
		ret = PTR_ERR(dev);
		goto dev_create_fail;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_gpt_stc);
	if (ret) {
		pr_info("%s device file create failed\n", __func__);
		goto dev_file_create_fail;
	}

	g_stc_param.instance_cnt = 0;
	g_stc_param.enable_flag = 0;

	for (i = 0; i < MAX_STC; ++i)
		clear_bit(i, stc_bitmap);

	ret = stc_parse_dev_node(pdev);
	if (ret != STC_OK) {
		pr_info("%s parse node failed\n", __func__);
		goto out;
	}

	init_stc_hw();  /* HW reset maybe need here */

	return STC_OK;

out:
	device_remove_file(&pdev->dev, &dev_attr_gpt_stc);
dev_file_create_fail:
	device_destroy(g_stc_param.stc_class, g_stc_param.stc_devno);
dev_create_fail:
	class_destroy(g_stc_param.stc_class);
class_create_fail:
	cdev_del(g_stc_param.stc_cdev);
cdev_add_fail:
	unregister_chrdev_region(g_stc_param.stc_devno, 1);

	return ret;
}

static int mtk_stc_remove(struct platform_device *pdev)
{
	int ret = 0;

	iounmap(g_stc_param.gpt_base_addr);

	device_remove_file(&pdev->dev, &dev_attr_gpt_stc);

	cdev_del(g_stc_param.stc_cdev);
	unregister_chrdev_region(g_stc_param.stc_devno, 1);
	device_destroy(g_stc_param.stc_class, g_stc_param.stc_devno);
	class_destroy(g_stc_param.stc_class);

	return ret;
}

static void mtk_stc_shutdown(struct platform_device *pdev)
{

}

static int __maybe_unused stc_suspend(struct device *dev)
{
	return STC_OK;
}

static int __maybe_unused stc_resume(struct device *dev)
{
	/*
	 * do nothing for GPT6 resume,
	 * it is assumed that CLK26M/CLK13M is always open,
	 * so GPT's clock is not closed.
	 */
	return STC_OK;
}

static const struct dev_pm_ops stc_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(stc_suspend, stc_resume)
};

static const struct of_device_id mediatek_stc_of_match[] = {
	{ .compatible = "mediatek,gpt-stc", },
	{ }
};
MODULE_DEVICE_TABLE(of, mediatek_stc_of_match);

static struct platform_driver mtk_stc_driver = {
	.driver = {
		.name = STC_SESSION_DEVICE,
		.of_match_table = mediatek_stc_of_match,
		.owner = THIS_MODULE,
		.pm = &stc_dev_pm_ops,
	},
	.probe = mtk_stc_probe,
	.remove = mtk_stc_remove,
	.shutdown = mtk_stc_shutdown,
};

module_platform_driver(mtk_stc_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Fengquan Chen <Fengquan.Chen@mediatek.com>");
MODULE_DESCRIPTION("Misc Driver for MediaTek GPT-STC");
