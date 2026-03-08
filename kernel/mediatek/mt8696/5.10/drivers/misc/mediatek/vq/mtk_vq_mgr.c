// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/pm_runtime.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/sched/clock.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/iommu.h>
#include <linux/slab.h>

#include "mtk_vq_mgr.h"
#if IS_ENABLED(CONFIG_MTK_NR)
#include "nr_hal.h"
#endif
#include "smi.h"
#include "di_hal.h"

static struct vq_data *vq_data_info;
static dev_t vq_devno;
static struct cdev *vq_cdev;
static struct class *vq_class;

struct vq_data *mtk_vq_get_data(void)
{
	return vq_data_info;
}

void mtk_vq_get_log_help(void)
{
	u32 i;
	static char *di_log_help[DI_LOG_MAX] = {
		[DI_LOG_ERROR] = "error log, default on",
		[DI_LOG_WARN] = "warning log",
		[DI_LOG_VQ] = "vq software flow",
		[DI_LOG_FLOW] = "di software flow",
		[DI_LOG_TIME] = "vq performance debug log",
		[DI_LOG_IRQ] = "vq irq log",
	};

	pr_info("vq log helper, current status 0x%x\n",
		di_dbg_level);

	for (i = 0; i < DI_LOG_MAX; i++)
		pr_info("[%d] %s\n", i, di_log_help[i]);

	for (i = 0; i < DI_LOG_MAX; i++) {
		if (di_dbg_level & (1 << i))
			pr_info("[%d] enable %s\n", i, di_log_help[i]);
	}
}

void mtk_vq_set_log_enable(u32 level, u32 en)
{
	if (en)
		di_dbg_level |= (1 << level);
	else
		di_dbg_level &= ~(1 << level);

	pr_info("[VQ] vq_log_enable is level %d %d 0x%x\n",
		level, en, di_dbg_level);
}

static void vq_dma_buffer_release(struct mtk_vq_dma *vq_dma)
{
	if (!vq_dma->dma_buf || !vq_dma->attach || !vq_dma->sgt) {
		VQ_ERR("invalid dma buffer\n");
		return;
	}
	dma_buf_unmap_attachment(vq_dma->attach,
		vq_dma->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(vq_dma->dma_buf, vq_dma->attach);
	dma_buf_put(vq_dma->dma_buf);

	vq_dma->dma_buf = NULL;
	vq_dma->attach = NULL;
	vq_dma->sgt = NULL;
}

static int vq_dma_fd_to_buffer(int fd, struct device *dev, struct mtk_vq_dma *vq_dma)
{
	int ret = 0;

	vq_dma->dma_buf = dma_buf_get(fd);
	if (IS_ERR(vq_dma->dma_buf)) {
		ret = (int)PTR_ERR(vq_dma->dma_buf);
		VQ_ERR("dma buf get failed, ret:%d\n", ret);
		return ret;
	}
	vq_dma->attach = dma_buf_attach(vq_dma->dma_buf, dev);
	if (IS_ERR(vq_dma->attach)) {
		ret = (int)PTR_ERR(vq_dma->attach);
		dma_buf_put(vq_dma->dma_buf);
		VQ_ERR("dma buf attach failed, ret:%d\n", ret);
		return ret;
	}

	vq_dma->sgt = dma_buf_map_attachment(vq_dma->attach, DMA_BIDIRECTIONAL);

	if (IS_ERR(vq_dma->sgt)) {
		ret = (int)PTR_ERR(vq_dma->sgt);
		VQ_ERR("dma buf map attachment failed, ret:%d\n", ret);
		dma_buf_detach(vq_dma->dma_buf, vq_dma->attach);
		dma_buf_put(vq_dma->dma_buf);
		return ret;
	}

	vq_dma->dma_addr = (unsigned int)sg_dma_address(vq_dma->sgt->sgl);

	VQ_INFO("dma fd to buffer, fd:%d, dma_add:0x%x\n", fd, vq_dma->dma_addr);

	return vq_dma->dma_addr;
}

int mtk_vq_mgr_prepare_buffer(struct vq_data *data,
				     struct mtk_vq_config_info *config_info)
{
	int ret = 0;
	int i = 0;
	unsigned int pre_buf_num = 0;
	unsigned int mva = 0;

	struct mtk_vq_config *config = config_info->vq_config;

	if (config->vq_mode == VQ_NR_STANDALONE)
		pre_buf_num = 1;
	else
		pre_buf_num = MTK_VQ_DI_INPUTBUFFER;

	for (i = 0; i < pre_buf_num; i++) {
		mva = vq_dma_fd_to_buffer(config->src_fd[i],
			data->dev, &config_info->src_vq_dma[i]);
		if (mva == 0) {
			VQ_ERR("[VQ]the %d fd:%d, src mva is invalid %d\n",
				i, config->src_fd[i], mva);
			ret = -EFAULT;
			goto error;
		}

		config_info->src_mva[i] = mva;

		VQ_INFO("[VQ]the %d src mva is 0x%x\n", i,
			config_info->src_mva[i]);

		mva = 0;
	}

	mva = vq_dma_fd_to_buffer(config->dst_fd, data->dev, &config_info->dst_vq_dma);
	if (mva == 0) {
		VQ_ERR("[VQ] fd:%d, dst mva is invalid %d\n",
			config->dst_fd, mva);

		ret = -EFAULT;
		goto error;
	}

	config_info->dst_mva = mva;

	VQ_INFO("[VQ] dst mva is 0x%x\n", config_info->dst_mva);

	return 0;

error:
	return ret;
}

static int mtk_vq_mgr_free_buffer_handle(struct mtk_vq_config_info *config_info)
{
	int i = 0;
	unsigned int pre_buf_num = 0;

	if (config_info->vq_config->vq_mode == VQ_NR_STANDALONE)
		pre_buf_num = 1;
	else
		pre_buf_num = MTK_VQ_DI_INPUTBUFFER;

	for (i = 0; i < pre_buf_num; i++)
		vq_dma_buffer_release(&config_info->src_vq_dma[i]);

	vq_dma_buffer_release(&config_info->dst_vq_dma);

	return 0;
}

static void mtk_vq_smi_larb_en(struct vq_data *data, bool en)
{
	VQ_INFO("enable %d\n", en);

	if (en)
		pm_runtime_get_sync(data->larb_dev);
	else
		pm_runtime_put_sync(data->larb_dev);
}

static void mtk_vq_pm_en(struct vq_data *data, bool en)
{
	if (en)
		pm_runtime_get_sync(data->dev);
	else
		pm_runtime_put_sync(data->dev);
}

int mtk_vq_power_on(struct vq_data *data, enum VQ_PATH_MODE vq_mode)
{
	int ret;

	mtk_vq_pm_en(data, true);

	if (vq_mode == VQ_NR_STANDALONE) {
#if IS_ENABLED(CONFIG_MTK_NR)
		ret = nr_hal_power_on(&data->nr);
		if (ret) {
			mtk_vq_pm_en(data, false);
			return ret;
		}
#else
		VQ_ERR("nr config not enable\n");

#endif
	} else if (vq_mode == VQ_DI_STANDALONE) {
		ret = di_hal_power_on(&data->di);
		if (ret) {
			mtk_vq_pm_en(data, false);
			return ret;
		}
	} else {
#if IS_ENABLED(CONFIG_MTK_NR)
		ret = nr_hal_power_on(&data->nr);
		if (ret) {
			mtk_vq_pm_en(data, false);
			return ret;
		}
#else
		VQ_ERR("nr config not enable\n");
#endif
		ret = di_hal_power_on(&data->di);
		if (ret) {
			mtk_vq_pm_en(data, false);
			return ret;
		}
	}

	mtk_vq_smi_larb_en(data, true);

	VQ_INFO("end\n");

	return ret;
}

int mtk_vq_power_off(struct vq_data *data, enum VQ_PATH_MODE vq_mode)
{
	if (vq_mode == VQ_NR_STANDALONE) {
#if IS_ENABLED(CONFIG_MTK_NR)
		nr_hal_power_off(&data->nr);
#else
		VQ_ERR("nr config not enable\n");
#endif
	} else if (vq_mode == VQ_DI_STANDALONE) {
		di_hal_power_off(&data->di);
	} else {
#if IS_ENABLED(CONFIG_MTK_NR)
		nr_hal_power_off(&data->nr);
#else
		VQ_ERR("nr config not enable\n");
#endif
		di_hal_power_off(&data->di);
	}

	mtk_vq_pm_en(data, false);

	mtk_vq_smi_larb_en(data, false);

	VQ_INFO("end\n");

	return 0;
}

static int mtk_vq_mgr_set_input_buffer_normal(struct vq_data *data,
					      struct mtk_vq_config_info
								*vq_config_info)
{
	int ret = 0;
	unsigned long start_timer, end_timer;

	switch (vq_config_info->vq_config->vq_mode) {
	case VQ_DI_NR_DIRECTLINK_ALL_ENABLE:
	case VQ_DI_NR_DIRECTLINK_DI_BYPASS:
	case VQ_DI_NR_DIRECTLINK_NR_BYPASS:
		start_timer = sched_clock();
#if IS_ENABLED(CONFIG_MTK_NR)
		ret = nr_hal_set_info(data->nr.nr_reg_base, vq_config_info);
#else
		VQ_ERR("nr config not enable\n");
#endif
		ret = di_hal_config(vq_config_info);
		end_timer = sched_clock();
		VQ_TIME("hw time %ld di+nr s %ld end %ld\n",
			(end_timer - start_timer) / 1000000,
			start_timer / 1000000,
			end_timer / 1000000);
		/* set di config */
		break;

	case VQ_DI_STANDALONE:
		start_timer = sched_clock();
		ret = di_hal_config(vq_config_info);
		end_timer = sched_clock();
		VQ_TIME("hw time is %ld di s:%ld,end:%ld\n",
			(end_timer - start_timer) / 1000000,
			start_timer / 1000000,
			end_timer / 1000000);
		break;

	case VQ_NR_STANDALONE:
		start_timer = sched_clock();
#if IS_ENABLED(CONFIG_MTK_NR)
		ret = nr_hal_set_info(data->nr.nr_reg_base, vq_config_info);
		ret = nr_hal_wait_complete_timeout(&data->nr, 1000);
#else
		VQ_ERR("nr config not enable\n");
#endif
		end_timer = sched_clock();
		VQ_TIME("[NR] hw timer is %ld\n",
			end_timer - start_timer);
		break;
	default:
		VQ_ERR("unknown vq mode %d\n",
			vq_config_info->vq_config->vq_mode);
		break;
	}

	return ret;
}

int mtk_vq_mgr_set_input_buffer(struct vq_data *data,
				struct mtk_vq_config *config)
{
	int ret = 0;
	unsigned long start_timer, end_timer;

	struct mtk_vq_config_info vq_config_info = {0};

	vq_config_info.vq_config = config;
	start_timer = sched_clock();

	ret = mtk_vq_mgr_prepare_buffer(data, &vq_config_info);
	if (ret) {
		VQ_ERR("prepare buffer error\n");
		return -EFAULT;
	}
	end_timer = sched_clock();
	VQ_TIME("prepare total time is %ld,s:%ld,end:%ld\n",
		(end_timer - start_timer) / 1000000,
		start_timer / 1000000,
		end_timer / 1000000);

	start_timer = sched_clock();
	ret = mtk_vq_power_on(data, config->vq_mode);
	if (ret) {
		VQ_ERR("power on clk error\n");
		return -EFAULT;
	}

#if IS_ENABLED(CONFIG_MTK_NR)
	nr_hal_switch_mode(data->disp_top_reg_base, config->vq_mode);
#endif

	VQ_INFO("the size (%d, %d), align size (%d, %d), mode %d\n",
		config->src_width,
		config->src_height,
		config->src_align_width,
		config->src_align_height,
		config->vq_mode);

	VQ_INFO("src offset(%d, %d), dst offset(%d %d), level(%d %d)\n",
		config->src_ofset_y_len[0], config->src_ofset_c_len[0],
		config->dst_ofset_y_len, config->dst_ofset_c_len,
		config->bnr_level, config->mnr_level);
	end_timer = sched_clock();
	VQ_TIME("power total time is %ld,s:%ld,end:%ld\n",
		(end_timer - start_timer) / 1000000,
		start_timer / 1000000,
		end_timer / 1000000);

	start_timer = sched_clock();

	ret = mtk_vq_mgr_set_input_buffer_normal(data, &vq_config_info);

	end_timer = sched_clock();
	VQ_TIME("%s end  %ld,s:%ld,end:%ld\n", __func__,
		(end_timer - start_timer) / 1000000,
		start_timer / 1000000,
		end_timer / 1000000);

	start_timer = sched_clock();
	mtk_vq_power_off(data, config->vq_mode);
	end_timer = sched_clock();
	VQ_TIME("mtk_vq_power_off end  %ld,s:%ld,end:%ld\n",
		(end_timer - start_timer) / 1000000,
		start_timer / 1000000,
		end_timer / 1000000);

	start_timer = sched_clock();
	ret = mtk_vq_mgr_free_buffer_handle(&vq_config_info);
	end_timer = sched_clock();
	VQ_TIME("%s end %ld,s:%ld,end:%ld\n", __func__,
		(end_timer - start_timer) / 1000000,
		start_timer / 1000000,
		end_timer / 1000000);

	return ret;
}


static long mtk_vq_mgr_ioctl(struct file *file, unsigned int cmd,
			     unsigned long arg)
{
	unsigned long start_timer, end_timer;
	void __user *argp = (void __user *)arg;
	long ret = 0;
	struct mtk_vq_config config;
	struct vq_data *data = mtk_vq_get_data();

	start_timer = sched_clock();

	VQ_INFO("%s:%d, ioctl cmd %d\n", __func__, __LINE__, cmd);

	switch (cmd) {
	case MTK_VQ_IOCTL_SET_INPUT_CONFIG:
		{
			VQ_INFO("%s:%d, ioctl cmd %d\n", __func__, __LINE__, cmd);
			if (copy_from_user(&config, argp, sizeof(config)))
				return -EFAULT;

			ret = mtk_vq_mgr_set_input_buffer(data, &config);

			if (!ret && copy_to_user(argp, &config, sizeof(config)))
				ret = -EFAULT;
			VQ_INFO("%s:%d, ioctl cmd %d\n", __func__, __LINE__, cmd);
			break;
		}
	default:
		VQ_ERR("error ioctl cmd %d\n", cmd);
		break;
	}

	end_timer = sched_clock();

	VQ_TIME("[VQ] %s total time is %ld\n", __func__,
		(end_timer - start_timer) / 1000000);

	return 0;
}



#if IS_ENABLED(CONFIG_COMPAT)
static long mtk_vq_mgr_compat_ioctl(struct file *file, unsigned int cmd,
				    unsigned long arg)
{
	return mtk_vq_mgr_ioctl(file, cmd, arg);
}
#endif

static int mtk_vq_mgr_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int mtk_vq_mgr_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations mtk_vq_mgr_fops = {
	.owner = THIS_MODULE,
	.open = mtk_vq_mgr_open,
	.unlocked_ioctl = mtk_vq_mgr_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = mtk_vq_mgr_compat_ioctl,
#endif
	.release = mtk_vq_mgr_release,
};

static int mtk_vq_create_device(struct platform_device *pdev)
{
	int ret = 0;

	VQ_INFO("\n");

	ret = alloc_chrdev_region(&vq_devno, 0, 1, VQ_SESSION_DEVICE);
	if (ret < 0) {
		VQ_ERR("alloc_chrdev_region failed, %d\n", ret);
		return ret;
	}

	vq_cdev = cdev_alloc();
	vq_cdev->owner = THIS_MODULE;
	vq_cdev->ops = &mtk_vq_mgr_fops;
	ret = cdev_add(vq_cdev, vq_devno, 1);
	if (ret < 0) {
		VQ_ERR("Attach file operation failed, %d\n", ret);
		goto out;
	}

	vq_class = class_create(THIS_MODULE, VQ_SESSION_DEVICE);
	device_create(vq_class, NULL, vq_devno, NULL, VQ_SESSION_DEVICE);

	return 0;
out:
	/* Release char driver */
	if (vq_cdev != NULL) {
		cdev_del(vq_cdev);
		vq_cdev = NULL;
	}

	unregister_chrdev_region(vq_devno, 1);
	return ret;
}

static int mtk_vq_mgr_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct vq_data *data;
	struct device_node *larb_node;
	struct resource *res;
	struct platform_device *larb_pdev;

	VQ_INFO("++\n");

	larb_node = of_parse_phandle(pdev->dev.of_node, "mediatek,larb", 0);
	if (!larb_node) {
		VQ_ERR("get smi larb node fail\n");
		return -EINVAL;
	}

	larb_pdev = of_find_device_by_node(larb_node);
	of_node_put(larb_node);
	if ((!larb_pdev) || (!larb_pdev->dev.driver)) {
		VQ_INFO("earlier than SMI\n");
		return -EPROBE_DEFER;
	}

	ret = mtk_vq_create_device(pdev);
	if (ret < 0) {
		VQ_ERR("vq device create fail, %d\n", ret);
		return -EINVAL;
	}

	data = devm_kzalloc(dev, sizeof(struct vq_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->larb_dev = &larb_pdev->dev;
	data->dev = dev;

#if IS_ENABLED(CONFIG_MTK_NR)
	ret = nr_hal_hw_init(pdev, &data->nr);
	if (ret == -EPROBE_DEFER) {
		VQ_INFO("get nr clk earlier than dispsys\n");
		return -EPROBE_DEFER;
	} else if (ret < 0) {
		VQ_ERR("get nr info fail\n");
		return -EINVAL;
	}
#endif

	ret = di_hal_hw_init(pdev, &data->di);
	if (ret == -EPROBE_DEFER) {
		VQ_INFO("get di clk earlier than dispsys\n");
		return -EPROBE_DEFER;
	} else if (ret < 0) {
		VQ_ERR("get di info fail\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, MTK_VQ_REG_DISP_TOP);
	if (!res) {
		VQ_ERR("failed to get disp top resource\n");
		return -ENXIO;
	}
	data->disp_top_reg_base =
		devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(data->disp_top_reg_base)) {
		VQ_ERR("get disp top reg base err\n");
		return PTR_ERR(data->disp_top_reg_base);
	}
	VQ_INFO("disp top reg base is 0x%lx\n",
		(unsigned long)data->disp_top_reg_base);

	dev_set_drvdata(dev, data);

	pm_runtime_enable(dev);

	vq_data_info = data;

	VQ_INFO("--\n");

	return 0;
}

static int mtk_vq_mgr_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;

	pm_runtime_disable(dev);

	return ret;
}

int mtk_vq_suspend(void)
{
	struct vq_data *data = mtk_vq_get_data();

	VQ_INFO("start\n");

	mtk_vq_power_off(data, VQ_DI_NR_DIRECTLINK_ALL_ENABLE);

	VQ_INFO("finish\n");

	return 0;
}

int mtk_vq_resume(void)
{
	struct vq_data *data = mtk_vq_get_data();

	VQ_INFO("start\n");

	mtk_vq_power_on(data, VQ_DI_NR_DIRECTLINK_ALL_ENABLE);

	VQ_INFO("finish\n");

	return 0;
}

static const struct of_device_id mgr_of_ids[] = {
	{.compatible = VQ_COMPATIBLE_NAME,},
	{}
};

static struct platform_driver mtk_vq_mgr_driver = {
	.probe = mtk_vq_mgr_probe,
	.remove = mtk_vq_mgr_remove,
	.driver = {
		   .name = VQ_SESSION_DEVICE,
		   .owner = THIS_MODULE,
		   .of_match_table = mgr_of_ids,
		   },
};

static int __init mtk_vq_mgr_init(void)
{
	VQ_INFO("in\n");

	if (platform_driver_register(&mtk_vq_mgr_driver))
		return -ENODEV;

	VQ_INFO("out\n");
	return 0;
}

static void __exit mtk_vq_mgr_exit(void)
{
	device_destroy(vq_class, vq_devno);
	class_destroy(vq_class);
	cdev_del(vq_cdev);
	unregister_chrdev_region(vq_devno, 1);

	platform_driver_unregister(&mtk_vq_mgr_driver);
}

module_init(mtk_vq_mgr_init);
module_exit(mtk_vq_mgr_exit);
MODULE_DESCRIPTION("mediatek vq manager");
MODULE_LICENSE("GPL");
