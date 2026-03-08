// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/arm-smccc.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <mt-plat/aee.h>
#include <soc/mediatek/emi.h>

struct emi_mpu {
	unsigned long long dram_base;
	unsigned int region_cnt;
	unsigned int domain_cnt;

	unsigned int dump_cnt;
	struct reg_info_t *dump_reg;

	unsigned int emi_cen_cnt;
	void __iomem **emi_cen_base;
	void __iomem **emi_mpu_base;

	/* interrupt id */
	unsigned int irq;

	/* hook functions in ISR */
	emimpu_pre_handler pre_handler;
	emimpu_post_clear post_clear;
	emimpu_md_handler md_handler;

	/* debugging log for EMI MPU violation */
	char *vio_msg;
	unsigned int in_msg_dump;

	/* hook functions in worker thread */
	struct emimpu_dbg_cb *dbg_cb_list;
};

/* global pointer for exported functions */
static struct emi_mpu *global_emi_mpu;

static unsigned int emi_read_vio_info(
	unsigned int emi_id, unsigned int reg_offset)
{
	struct arm_smccc_res smc_res;

	arm_smccc_smc(MTK_SIP_EMIMPU_CONTROL, MTK_EMI_READ,
		emi_id, reg_offset, 0, 0, 0, 0, &smc_res);

	return (unsigned int)smc_res.a0;
}

static void clear_violation(
	struct emi_mpu *mpu, unsigned int emi_id)
{
	struct arm_smccc_res smc_res;

	arm_smccc_smc(MTK_SIP_EMIMPU_CONTROL, MTK_EMI_VIO_CLR,
		EMI_VIO_CLR_MPU, 0, 0, 0, 0, 0, &smc_res);

	if (mpu->post_clear)
		mpu->post_clear(emi_id);
}

static void emimpu_vio_dump(struct work_struct *work)
{
	struct emi_mpu *mpu;
	struct emimpu_dbg_cb *curr_dbg_cb;

	mpu = global_emi_mpu;
	if (!mpu)
		return;

	for (curr_dbg_cb = mpu->dbg_cb_list; curr_dbg_cb;
		curr_dbg_cb = curr_dbg_cb->next_dbg_cb)
		curr_dbg_cb->func();

	if (mpu->vio_msg)
		aee_kernel_exception("EMIMPU", mpu->vio_msg);

	mpu->in_msg_dump = 0;
}
static DECLARE_WORK(emimpu_work, emimpu_vio_dump);

static irqreturn_t emimpu_violation_irq(int irq, void *dev_id)
{
	struct emi_mpu *mpu = (struct emi_mpu *)dev_id;
	struct reg_info_t *dump_reg = mpu->dump_reg;
	void __iomem *emi_cen_base;
	unsigned int emi_id, i;
	ssize_t msg_len;
	int n, nr_vio;
	bool violation;

	if (mpu->in_msg_dump)
		goto ignore_violation;

	n = snprintf(mpu->vio_msg, MTK_EMI_MAX_CMD_LEN, "violation\n");
	msg_len = (n < 0) ? 0 : (ssize_t)n;

	nr_vio = 0;
	for (emi_id = 0; emi_id < mpu->emi_cen_cnt; emi_id++) {
		violation = false;
		emi_cen_base = mpu->emi_cen_base[emi_id];

		for (i = 0; i < mpu->dump_cnt; i++) {
			dump_reg[i].value = emi_read_vio_info(
				emi_id, dump_reg[i].offset);

			if (msg_len < MTK_EMI_MAX_CMD_LEN) {
				n = snprintf(mpu->vio_msg + msg_len,
					MTK_EMI_MAX_CMD_LEN - msg_len,
					"%s(%d),%s(%x),%s(%x);\n",
					"emi", emi_id,
					"off", dump_reg[i].offset,
					"val", dump_reg[i].value);
				msg_len += (n < 0) ? 0 : (ssize_t)n;
			}

			if (dump_reg[i].value)
				violation = true;
		}

		if (!violation)
			continue;

		/*
		 * The DEVAPC module used the EMI MPU interrupt on some
		 * old smart-phone SoC. For these SoC, the DEVAPC driver
		 * will register a handler for processing its interrupt.
		 * If the handler has processed DEVAPC interrupt (and
		 * returns IRQ_HANDLED), just skip dumping and exit.
		 */
		if (mpu->pre_handler)
			if (mpu->pre_handler(emi_id, dump_reg,
					mpu->dump_cnt) == IRQ_HANDLED) {
				clear_violation(mpu, emi_id);
				mtk_clear_md_violation();
				continue;
			}

		nr_vio++;

		/*
		 * Whenever there is an EMI MPU violation, the Modem
		 * software would like to be notified immediately.
		 * This is because the Modem software wants to do
		 * its coredump as earlier as possible for debugging
		 * and analysis.
		 * (Even if the violated master is not Modem, it
		 *  may still need coredump for clarification.)
		 * Have a hook function in the EMI MPU ISR for this
		 * purpose.
		 */
		if (mpu->md_handler) {
			mpu->md_handler(emi_id,
				dump_reg, mpu->dump_cnt);
		}
	}

	if (nr_vio) {
		pr_info("%s: %s", __func__, mpu->vio_msg);
		mpu->in_msg_dump = 1;
		schedule_work(&emimpu_work);
	}

ignore_violation:
	for (emi_id = 0; emi_id < mpu->emi_cen_cnt; emi_id++)
		clear_violation(mpu, emi_id);

	return IRQ_HANDLED;
}


/*
 * mtk_emimpu_prehandle_register - register callback for irq prehandler
 * @bypass_func:	function point for prehandler
 *
 * Return 0 for success, -EINVAL for fail
 */
int mtk_emimpu_prehandle_register(emimpu_pre_handler bypass_func)
{
	struct emi_mpu *mpu;

	mpu = global_emi_mpu;
	if (!mpu)
		return -EINVAL;

	if (!bypass_func) {
		pr_info("%s: bypass_func is NULL\n", __func__);
		return -EINVAL;
	}

	mpu->pre_handler = bypass_func;

	return 0;
}
EXPORT_SYMBOL(mtk_emimpu_prehandle_register);

/*
 * mtk_emimpu_postclear_register - register callback for clear posthandler
 * @clear_func:	function point for posthandler
 *
 * Return 0 for success, -EINVAL for fail
 */
int mtk_emimpu_postclear_register(emimpu_post_clear clear_func)
{
	struct emi_mpu *mpu;

	mpu = global_emi_mpu;
	if (!mpu)
		return -EINVAL;

	if (!clear_func) {
		pr_info("%s: clear_func is NULL\n", __func__);
		return -EINVAL;
	}

	mpu->post_clear = clear_func;

	return 0;
}
EXPORT_SYMBOL(mtk_emimpu_postclear_register);

/*
 * mtk_emimpu_md_handling_register - register callback for md handling
 * @md_handling_func:	function point for md handling
 *
 * Return 0 for success, -EINVAL for fail
 */
int mtk_emimpu_md_handling_register(emimpu_md_handler md_handling_func)
{
	struct emi_mpu *mpu;

	mpu = global_emi_mpu;
	if (!mpu)
		return -EINVAL;

	if (!md_handling_func) {
		pr_info("%s: md_handling_func is NULL\n", __func__);
		return -EINVAL;
	}

	mpu->md_handler = md_handling_func;

	return 0;
}
EXPORT_SYMBOL(mtk_emimpu_md_handling_register);

/*
 * mtk_clear_md_violation - clear irq for md violation
 *
 * No return
 */
void mtk_clear_md_violation(void)
{
	struct arm_smccc_res smc_res;

	arm_smccc_smc(MTK_SIP_EMIMPU_CONTROL, MTK_EMI_VIO_CLR,
		EMI_VIO_CLR_MD, 0, 0, 0, 0, 0, &smc_res);

}
EXPORT_SYMBOL(mtk_clear_md_violation);

/*
 * mtk_emimpu_debugdump_register - register callback for debug info dump
 * @debug_func:	function point for debug info dump
 *
 * Return 0 for success, -EINVAL for fail
 */
int mtk_emimpu_debugdump_register(emimpu_debug_dump debug_func)
{
	struct emimpu_dbg_cb *targ_dbg_cb;
	struct emimpu_dbg_cb *curr_dbg_cb;
	struct emi_mpu *mpu;

	mpu = global_emi_mpu;
	if (!mpu)
		return -EINVAL;

	if (!debug_func) {
		pr_info("%s: debug_func is NULL\n", __func__);
		return -EINVAL;
	}

	targ_dbg_cb = kmalloc(sizeof(struct emimpu_dbg_cb), GFP_KERNEL);
	if (!targ_dbg_cb)
		return -ENOMEM;

	targ_dbg_cb->func = debug_func;
	targ_dbg_cb->next_dbg_cb = NULL;

	if (!(mpu->dbg_cb_list)) {
		mpu->dbg_cb_list = targ_dbg_cb;
		return 0;
	}

	for (curr_dbg_cb = mpu->dbg_cb_list; curr_dbg_cb;
		curr_dbg_cb = curr_dbg_cb->next_dbg_cb) {
		if (!(curr_dbg_cb->next_dbg_cb)) {
			curr_dbg_cb->next_dbg_cb = targ_dbg_cb;
			break;
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_emimpu_debugdump_register);

static const struct of_device_id emimpu_of_ids[] = {
	{.compatible = "mediatek,common-emimpu",},
	{}
};
MODULE_DEVICE_TABLE(of, emimpu_of_ids);

static int emimpu_probe(struct platform_device *pdev)
{
	struct device_node *emimpu_node = pdev->dev.of_node;
	struct device_node *emicen_node =
		of_parse_phandle(emimpu_node, "mediatek,emi-reg", 0);
	struct emi_mpu *mpu;
	int ret, size, i;
	struct resource *res;
	unsigned int *dump_list;

	dev_info(&pdev->dev, "driver probed\n");

	if (!emicen_node) {
		dev_err(&pdev->dev, "No emi-reg\n");
		return -ENXIO;
	}

	mpu = devm_kzalloc(&pdev->dev,
		sizeof(struct emi_mpu), GFP_KERNEL);
	if (!mpu)
		return -ENOMEM;

	ret = of_property_read_u32(emimpu_node,
		"region_cnt", &(mpu->region_cnt));
	if (ret) {
		dev_err(&pdev->dev, "No region_cnt\n");
		return -ENXIO;
	}

	ret = of_property_read_u32(emimpu_node,
		"domain_cnt", &(mpu->domain_cnt));
	if (ret) {
		dev_err(&pdev->dev, "No domain_cnt\n");
		return -ENXIO;
	}

	ret = of_property_read_u64(emimpu_node,
		"dram_base", &(mpu->dram_base));
	if (ret) {
		dev_err(&pdev->dev, "No dram_base\n");
		return -ENXIO;
	}

	size = of_property_count_elems_of_size(emimpu_node,
		"dump", sizeof(char));
	if (size <= 0) {
		dev_err(&pdev->dev, "No dump\n");
		return -ENXIO;
	}
	dump_list = devm_kmalloc(&pdev->dev, size, GFP_KERNEL);
	if (!dump_list)
		return -ENOMEM;
	size >>= 2;
	mpu->dump_cnt = size;
	ret = of_property_read_u32_array(emimpu_node, "dump",
		dump_list, size);
	if (ret) {
		dev_err(&pdev->dev, "No dump\n");
		return -ENXIO;
	}
	mpu->dump_reg = devm_kmalloc(&pdev->dev,
		size * sizeof(struct reg_info_t), GFP_KERNEL);
	if (!(mpu->dump_reg))
		return -ENOMEM;
	for (i = 0; i < mpu->dump_cnt; i++) {
		mpu->dump_reg[i].offset = dump_list[i];
		mpu->dump_reg[i].value = 0;
		mpu->dump_reg[i].leng = 0;
	}

	mpu->emi_cen_cnt = of_property_count_elems_of_size(
			emicen_node, "reg", sizeof(unsigned int) * 4);
	if (mpu->emi_cen_cnt <= 0) {
		dev_err(&pdev->dev, "No reg\n");
		return -ENXIO;
	}

	mpu->emi_cen_base = devm_kmalloc_array(&pdev->dev,
		mpu->emi_cen_cnt, sizeof(phys_addr_t), GFP_KERNEL);
	if (!(mpu->emi_cen_base))
		return -ENOMEM;

	mpu->emi_mpu_base = devm_kmalloc_array(&pdev->dev,
		mpu->emi_cen_cnt, sizeof(phys_addr_t), GFP_KERNEL);
	if (!(mpu->emi_mpu_base))
		return -ENOMEM;

	for (i = 0; i < mpu->emi_cen_cnt; i++) {
		mpu->emi_cen_base[i] = of_iomap(emicen_node, i);
		if (IS_ERR(mpu->emi_cen_base[i])) {
			dev_err(&pdev->dev, "Failed to map EMI%d CEN base\n",
				i);
			return -EIO;
		}

		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		mpu->emi_mpu_base[i] =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(mpu->emi_mpu_base[i])) {
			dev_err(&pdev->dev, "Failed to map EMI%d MPU base\n",
				i);
			return -EIO;
		}
	}

	mpu->vio_msg = devm_kmalloc(&pdev->dev,
		MTK_EMI_MAX_CMD_LEN, GFP_KERNEL);
	if (!(mpu->vio_msg))
		return -ENOMEM;

	global_emi_mpu = mpu;
	platform_set_drvdata(pdev, mpu);

	mpu->irq = irq_of_parse_and_map(emimpu_node, 0);
	if (mpu->irq == 0) {
		dev_err(&pdev->dev, "Failed to get irq resource\n");
		ret = -ENXIO;
		goto emimpu_probe_end;
	}
	ret = request_irq(mpu->irq, (irq_handler_t)emimpu_violation_irq,
		IRQF_TRIGGER_NONE, "emimpu", mpu);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq");
		ret = -EINVAL;
		goto emimpu_probe_end;
	}

	devm_kfree(&pdev->dev, dump_list);

	dev_info(&pdev->dev, "%s(%d),%s(%d),%s(%llx)\n",
		"region_cnt", mpu->region_cnt,
		"domain_cnt", mpu->domain_cnt,
		"dram_base", mpu->dram_base);

	return 0;

emimpu_probe_end:

	return ret;
}

static int emimpu_remove(struct platform_device *pdev)
{
	struct emi_mpu *mpu = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "driver removed\n");

	free_irq(mpu->irq, mpu);

	flush_work(&emimpu_work);

	global_emi_mpu = NULL;

	return 0;
}

static struct platform_driver emimpu_driver = {
	.probe = emimpu_probe,
	.remove = emimpu_remove,
	.driver = {
		.name = "emimpu_driver",
		.owner = THIS_MODULE,
		.of_match_table = emimpu_of_ids,
	},
};

static __init int emimpu_init(void)
{
	int ret;

	pr_info("emimpu was loaded\n");

	ret = platform_driver_register(&emimpu_driver);
	if (ret) {
		pr_err("emimpu: failed to register driver\n");
		return ret;
	}

	return 0;
}

static void __exit emimpu_exit(void)
{
	platform_driver_unregister(&emimpu_driver);
}

module_init(emimpu_init);
module_exit(emimpu_exit);

MODULE_DESCRIPTION("MediaTek EMI MPU Driver");
MODULE_LICENSE("GPL v2");
