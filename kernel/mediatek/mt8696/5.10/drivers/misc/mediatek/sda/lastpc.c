// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

struct boot_tag_lastpc {
	u32 tag_size;
	u32 tag_magic;
	u32 lastpc[4][8];
};

static ssize_t lastpc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i;
	unsigned long tmp;
	unsigned long elr, lr, sp, pc;
	unsigned long size = 0, offset = 0;
	char *ptr = buf;
	const char *name;
	char str[KSYM_SYMBOL_LEN];
	struct boot_tag_lastpc *tags = dev_get_drvdata(dev);

	if (!tags)
		return snprintf(buf, PAGE_SIZE, "lastpc is NULL\n");

	for (i = 0; i < 4; i++) {
		tmp = 0;
		/*
		 * 0x06 : elr_latch[31:0]
		 * 0x07 : elr_latch[63:32]
		 * 0x08 : lr_latch[31:0]
		 * 0x09 : lr_latch[63:32]
		 * 0x0A : sp_latch[31:0]
		 * 0x0B : sp_latch[63:32]
		 * 0x0C : pc_latch[31:0]
		 * 0x0D : pc_latch[63:32]
		 */
		elr = tags->lastpc[i][0];
		lr  = tags->lastpc[i][2];
		sp  = tags->lastpc[i][4];
		pc  = tags->lastpc[i][6];
#if IS_ENABLED(CONFIG_ARM64)
		tmp = tags->lastpc[i][1];
		elr |= (tmp << 32);
		tmp = tags->lastpc[i][3];
		lr  |= (tmp << 32);
		tmp = tags->lastpc[i][5];
		sp  |= (tmp << 32);
		tmp = tags->lastpc[i][7];
		pc  |= (tmp << 32);
#endif
		name = kallsyms_lookup(pc, &size, &offset, NULL, str);
		if (!name)
			strncpy(str, "UNKNOWN", KSYM_SYMBOL_LEN);
		ptr += sprintf(ptr, "CPU%d: PC = 0x%lx(%s + 0x%lx/0x%lx)\n",
					i, pc, str, offset, size);
		name = kallsyms_lookup(lr, &size, &offset, NULL, str);
		if (!name)
			strncpy(str, "UNKNOWN", KSYM_SYMBOL_LEN);
		ptr += sprintf(ptr, "CPU%d: LR = 0x%lx(%s + 0x%lx/0x%lx)\n",
					i, lr, str, offset, size);
		name = kallsyms_lookup(elr, &size, &offset, NULL, str);
		if (!name)
			strncpy(str, "UNKNOWN", KSYM_SYMBOL_LEN);
		ptr += sprintf(ptr, "CPU%d: ELR = 0x%lx(%s + 0x%lx/0x%lx)\n",
					i, elr, str, offset, size);
		ptr += sprintf(ptr, "CPU%d: SP = 0x%lx\n", i, sp);
	}

	return strlen(buf);
}

static DEVICE_ATTR_RO(lastpc);

static int lastpc_probe(struct platform_device *pdev)
{
	int ret;
#if IS_ENABLED(CONFIG_OF)
	struct boot_tag_lastpc *tags;

	if (of_chosen) {
		tags = (struct boot_tag_lastpc *)of_get_property(of_chosen,
				"atag,lastpc", NULL);
		if (tags)
			platform_set_drvdata(pdev, tags);
		else {
			dev_info(&pdev->dev, "atag,lastpc not found!\n");
			return -ENODEV;
		}
	} else {
		dev_notice(&pdev->dev, "of_chosen is NULL!\n");
		return -ENODEV;
	}
#endif

	ret = device_create_file(&pdev->dev, &dev_attr_lastpc);
	if (ret) {
		dev_notice(&pdev->dev, "cannot create sys file, err=%d\n", ret);
		return ret;
	}

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id lastpc_of_ids[] = {
	{ .compatible = "mediatek,lastpc", },
	{ /* sentinel */ }
};
#endif

static struct platform_driver lastpc_drv = {
	.probe	= lastpc_probe,
	.driver	= {
		.name			= "lastpc",
		.bus			= &platform_bus_type,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table	= lastpc_of_ids,
#endif
		.owner			= THIS_MODULE,
	},
};

module_platform_driver(lastpc_drv);
