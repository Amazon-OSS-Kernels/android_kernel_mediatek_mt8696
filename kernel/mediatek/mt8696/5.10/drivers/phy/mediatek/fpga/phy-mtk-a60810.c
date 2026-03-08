// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

/*
 * A60810/A60931 usb3 phy board for FPGA (MD1122, MD1191 etc)
 * bank 0x00: u2phy com
 *      0x10: u3phyd + u3phyd_bank2
 *      0x30: u3phya + u3phya_da
 *      0x50: chip
 *      0x60: spllc
 *      0xf0: fm
 */

#include <dt-bindings/phy/phy.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include "ext_i2c.h"

#define A60810_I2C_ADDR 0x60
#define U3P_REF_CLK		26	/* MHZ */
#define U3P_SLEW_RATE_COEF	28

struct device *u3phy_dev;
static void *g_ippc_port_addr;

enum mtk_phy_version {
	MTK_PHY_V1 = 1,
	MTK_PHY_V2,
};

struct a60810_phy_pdata {
	/* avoid RX sensitivity level degradation only for mt8173 */
	bool avoid_rx_sen_degradation;
	enum mtk_phy_version version;
};

struct u2phy_banks {
	void __iomem *misc;
	void __iomem *fmreg;
	void __iomem *com;
};

struct u3phy_banks {
	void __iomem *spllc;
	void __iomem *chip;
	void __iomem *phyd; /* include u3phyd_bank2 */
	void __iomem *phya; /* include u3phya_da */
};

struct mtk_phy_instance {
	struct phy *phy;
	void __iomem *port_base;
	union {
		struct u2phy_banks u2_banks;
		struct u3phy_banks u3_banks;
	};
	struct clk *ref_clk;	/* reference clock of anolog phy */
	u32 index;
	u8 type;
};

struct mtk_tphy {
	struct device *dev;
	void __iomem *sif_base;	/* only shared sif */
	/* deprecated, use @ref_clk instead in phy instance */
	struct clk *u3phya_ref;	/* reference clock of usb3 anolog phy */
	const struct a60810_phy_pdata *pdata;
	struct mtk_phy_instance **phys;
	int nphys;
	int src_ref_clk; /* MHZ, reference clock for slew rate calibrate */
	int src_coef; /* coefficient for slew rate calibrate */
};

static int phy_write8(unsigned char data, unsigned char addr)
{
	phy_writeb(g_ippc_port_addr, A60810_I2C_ADDR, addr, data);

	return 0;
}

static unsigned char __maybe_unused phy_read8(unsigned char addr)
{
	unsigned char data;

	data = phy_readb(g_ippc_port_addr, A60810_I2C_ADDR, addr);

	return data;
}

static int a60810_u3phy_init(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	u32 version;

	g_ippc_port_addr = instance->port_base + 0x00D0;

	version = get_phy_version(g_ippc_port_addr);
	if (version != 0xa60810a && version != 0xa60931a) {
		PHY_LOG("get phy version failed\n");
		return -1;
	}

	/* usb phy initial sequence */
	phy_write8(0x00, 0xFF);
	PHY_LOG("*********** before bank 0x00 ***********\n");
	PHY_LOG("[U2P]addr: 0xFF, value: %x\n", phy_read8(0xFF));
	PHY_LOG("[U2P]addr: 0x00, value: %x\n", phy_read8(0x00));
	PHY_LOG("[U2P]addr: 0x05, value: %x\n", phy_read8(0x05));
	PHY_LOG("[U2P]addr: 0x18, value: %x\n", phy_read8(0x18));
	PHY_LOG("[U2P]addr: 0x1d, value: %x\n", phy_read8(0x1d));
	PHY_LOG("[U2P]addr: 0x6A, value: %x\n", phy_read8(0x6A));
	PHY_LOG("[U2P]addr: 0x68, value: %x\n", phy_read8(0x68));
	PHY_LOG("[U2P]addr: 0x6C, value: %x\n", phy_read8(0x6C));
	PHY_LOG("[U2P]addr: 0x6D, value: %x\n", phy_read8(0x6D));
	PHY_LOG("*********** after ***********\n");
	/* phy_write8(phy_read8(0x0) | 0x20, 0x00);*/ /* INTR_EN */
	phy_write8(0x55, 0x05); /* U2PHYACR1: TERM, VRT */
	phy_write8(0x40, 0x15); /* SLEW RATE */
	phy_write8(0x84, 0x18); /* U2PHYACR6: SQTH, DISCTH */
	phy_write8(0x00, 0x1a); /* disable BC11 */
	phy_write8(0x04, 0x6A); /* U2PHYDTM0,1: force suspendm */
	phy_write8(0x08, 0x68);
	phy_write8(0x26, 0x6C); /* force iddig, avalid, sessend, vbusvalid */
	phy_write8(0x36, 0x6D);

	PHY_LOG("[U2P]addr: 0xFF, value: %x\n", phy_read8(0xFF));
	PHY_LOG("[U2P]addr: 0x00, value: %x\n", phy_read8(0x00));
	PHY_LOG("[U2P]addr: 0x05, value: %x\n", phy_read8(0x05));
	PHY_LOG("[U2P]addr: 0x18, value: %x\n", phy_read8(0x18));
	PHY_LOG("[U2P]addr: 0x1d, value: %x\n", phy_read8(0x1d));
	PHY_LOG("[U2P]addr: 0x6A, value: %x\n", phy_read8(0x6A));
	PHY_LOG("[U2P]addr: 0x68, value: %x\n", phy_read8(0x68));
	PHY_LOG("[U2P]addr: 0x6C, value: %x\n", phy_read8(0x6C));
	PHY_LOG("[U2P]addr: 0x6D, value: %x\n", phy_read8(0x6D));

	PHY_LOG("*********** before bank 0x10 ***********\n");
	phy_write8(0x10, 0xFF);
	PHY_LOG("[U2P]addr: 0xFF, value: %x\n", phy_read8(0xFF));
	PHY_LOG("[U2P]addr: 0x0A, value: %x\n", phy_read8(0x0A));
	PHY_LOG("*********** after ***********\n");

	phy_write8(0x84, 0x0A);

	PHY_LOG("[U2P]addr: 0xFF, value: %x\n", phy_read8(0xFF));
	PHY_LOG("[U2P]addr: 0x0A, value: %x\n", phy_read8(0x0A));
	PHY_LOG("*********** before bank 0x40 ***********\n");
	phy_write8(0x40, 0xFF);
	PHY_LOG("[U2P]addr: 0xFF, value: %x\n", phy_read8(0xFF));
	PHY_LOG("[U2P]addr: 0x38, value: %x\n", phy_read8(0x38));
	PHY_LOG("[U2P]addr: 0x42, value: %x\n", phy_read8(0x42));
	PHY_LOG("[U2P]addr: 0x08, value: %x\n", phy_read8(0x08));
	PHY_LOG("[U2P]addr: 0x09, value: %x\n", phy_read8(0x09));
	PHY_LOG("[U2P]addr: 0x0C, value: %x\n", phy_read8(0x0C));
	PHY_LOG("[U2P]addr: 0x0E, value: %x\n", phy_read8(0x0E));
	PHY_LOG("[U2P]addr: 0x10, value: %x\n", phy_read8(0x10));
	PHY_LOG("[U2P]addr: 0x14, value: %x\n", phy_read8(0x14));
	PHY_LOG("*********** after ***********\n");

	phy_write8(0x46, 0x38);
	phy_write8(0x40, 0x42);
	phy_write8(0xAB, 0x08);
	phy_write8(0x0C, 0x09);
	phy_write8(0x71, 0x0C);
	phy_write8(0x4F, 0x0E);
	phy_write8(0xE1, 0x10);
	phy_write8(0x5F, 0x14);
	PHY_LOG("[U2P]addr: 0xFF, value: %x\n", phy_read8(0xFF));
	PHY_LOG("[U2P]addr: 0x38, value: %x\n", phy_read8(0x38));
	PHY_LOG("[U2P]addr: 0x42, value: %x\n", phy_read8(0x42));
	PHY_LOG("[U2P]addr: 0x08, value: %x\n", phy_read8(0x08));
	PHY_LOG("[U2P]addr: 0x09, value: %x\n", phy_read8(0x09));
	PHY_LOG("[U2P]addr: 0x0C, value: %x\n", phy_read8(0x0C));
	PHY_LOG("[U2P]addr: 0x0E, value: %x\n", phy_read8(0x0E));
	PHY_LOG("[U2P]addr: 0x10, value: %x\n", phy_read8(0x10));
	PHY_LOG("[U2P]addr: 0x14, value: %x\n", phy_read8(0x14));
	PHY_LOG("*********** before bank 0x60 ***********\n");
	phy_write8(0x60, 0xFF);
	PHY_LOG("[U2P]addr: 0xFF, value: %x\n", phy_read8(0xFF));
	PHY_LOG("[U2P]addr: 0x10, value: %x\n", phy_read8(0x14));
	PHY_LOG("*********** after ***********\n");

	phy_write8(0x03, 0x14);
	PHY_LOG("[U2P]addr: 0xFF, value: %x\n", phy_read8(0xFF));
	PHY_LOG("[U2P]addr: 0x10, value: %x\n", phy_read8(0x14));

	PHY_LOG("[U2P]%s, end\n", __func__);
	return 0;
}

static int a60810_phy_init(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_tphy *u3phy = dev_get_drvdata(phy->dev.parent);
	int ret;

	ret = clk_prepare_enable(u3phy->u3phya_ref);
	if (ret) {
		dev_err(u3phy->dev, "failed to enable u3phya_ref\n");
		return ret;
	}

	ret = clk_prepare_enable(instance->ref_clk);
	if (ret) {
		dev_err(u3phy->dev, "failed to enable ref_clk\n");
		return ret;
	}

	switch (instance->type) {
	case PHY_TYPE_USB2:
	case PHY_TYPE_USB3:
		a60810_u3phy_init(phy);
		break;
	default:
		dev_err(u3phy->dev,
			"unsupported device type: %d\n", instance->type);
		break;
	}

	return 0;
}

static int a60810_phy_exit(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_tphy *u3phy = dev_get_drvdata(phy->dev.parent);

	switch (instance->type) {
	case PHY_TYPE_USB2:
	case PHY_TYPE_USB3:
		/* phy_instance_exit(u3phy, instance); */
		break;
	default:
		dev_err(u3phy->dev,
			"unsupported device type: %d\n", instance->type);
		break;
	}

	clk_disable_unprepare(instance->ref_clk);
	clk_disable_unprepare(u3phy->u3phya_ref);
	return 0;
}

static void phy_v1_banks_init(struct mtk_tphy *u3phy,
	struct mtk_phy_instance *instance)
{
	struct u2phy_banks *u2_banks = &instance->u2_banks;
	struct u3phy_banks *u3_banks = &instance->u3_banks;

	switch (instance->type) {
	case PHY_TYPE_USB2:
		u2_banks->misc	= NULL;
		u2_banks->fmreg = u3phy->sif_base;
		u2_banks->com	= instance->port_base;
		break;
	case PHY_TYPE_USB3:
		u3_banks->spllc	= u3phy->sif_base;
		u3_banks->chip	= NULL;
		u3_banks->phyd	= instance->port_base;
		u3_banks->phya	= instance->port_base;
		break;
	default:
		dev_err(u3phy->dev,
			"unsupported device type: %d\n", instance->type);
		break;
	}

}

static void phy_v2_banks_init(struct mtk_tphy *u3phy,
	struct mtk_phy_instance *instance)
{
	struct u2phy_banks *u2_banks = &instance->u2_banks;
	struct u3phy_banks *u3_banks = &instance->u3_banks;

	switch (instance->type) {
	case PHY_TYPE_USB2:
		u2_banks->misc	= instance->port_base;
		u2_banks->fmreg	= instance->port_base;
		u2_banks->com	= instance->port_base;
		break;
	case PHY_TYPE_USB3:
	case PHY_TYPE_PCIE:
		u3_banks->spllc	= instance->port_base;
		u3_banks->chip	= instance->port_base;
		u3_banks->phyd	= instance->port_base;
		u3_banks->phya	= instance->port_base;
		break;
	default:
		dev_err(u3phy->dev,
			"unsupported device type: %d\n", instance->type);
		break;
	}

}

static int a60810_phy_power_on(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_tphy *u3phy = dev_get_drvdata(phy->dev.parent);

	switch (instance->type) {
	case PHY_TYPE_USB2:
	case PHY_TYPE_USB3:
		/* phy_instance_power_on(u3phy, instance); */
		/* hs_slew_rate_calibrate(u3phy, instance); */
		break;
	default:
		dev_err(u3phy->dev,
			"unsupported device type: %d\n", instance->type);
		break;
	}

	return 0;
}

static int a60810_phy_power_off(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_tphy *u3phy = dev_get_drvdata(phy->dev.parent);

	switch (instance->type) {
	case PHY_TYPE_USB2:
	case PHY_TYPE_USB3:
		/* phy_instance_power_off(u3phy, instance); */
		break;
	default:
		dev_err(u3phy->dev,
			"unsupported device type: %d\n", instance->type);
		break;
	}

	return 0;
}

static struct phy *mtk_phy_xlate(struct device *dev,
					struct of_phandle_args *args)
{
	struct mtk_tphy *tphy = dev_get_drvdata(dev);
	struct mtk_phy_instance *instance = NULL;
	struct device_node *phy_np = args->np;
	int index;

	if (args->args_count != 1) {
		dev_err(dev, "invalid number of cells in 'phy' property\n");
		return ERR_PTR(-EINVAL);
	}

	for (index = 0; index < tphy->nphys; index++)
		if (phy_np == tphy->phys[index]->phy->dev.of_node) {
			instance = tphy->phys[index];
			break;
		}

	if (!instance) {
		dev_err(dev, "failed to find appropriate phy\n");
		return ERR_PTR(-EINVAL);
	}

	instance->type = args->args[0];
	if (!(instance->type == PHY_TYPE_USB2 ||
	      instance->type == PHY_TYPE_USB3 ||
	      instance->type == PHY_TYPE_PCIE ||
	      instance->type == PHY_TYPE_SATA)) {
		dev_err(dev, "unsupported device type: %d\n", instance->type);
		return ERR_PTR(-EINVAL);
	}

	if (tphy->pdata->version == MTK_PHY_V1) {
		phy_v1_banks_init(tphy, instance);
	} else if (tphy->pdata->version == MTK_PHY_V2) {
		phy_v2_banks_init(tphy, instance);
	} else {
		dev_err(dev, "phy version is not supported\n");
		return ERR_PTR(-EINVAL);
	}

	return instance->phy;
}

static const struct phy_ops a60810_u3phy_ops = {
	.init		= a60810_phy_init,
	.exit		= a60810_phy_exit,
	.power_on	= a60810_phy_power_on,
	.power_off	= a60810_phy_power_off,
	.owner		= THIS_MODULE,
};

static const struct a60810_phy_pdata a60810_pdata = {
	.avoid_rx_sen_degradation = false,
	.version = MTK_PHY_V2,
};

static const struct of_device_id a60810_u3phy_id_table[] = {
	{
	 .compatible = "mediatek,u3phy-a60810",
	 .data = &a60810_pdata
	},
	{ },
};

static int u3phy_a60810_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct phy_provider *provider;
	struct resource *sif_res;
	struct mtk_tphy *tphy;
	int port, retval;
	u32 ippc;

	tphy = devm_kzalloc(dev, sizeof(*tphy), GFP_KERNEL);
	if (!tphy)
		return -ENOMEM;

	tphy->pdata = of_device_get_match_data(dev);
	if (!tphy->pdata)
		return -EINVAL;

	tphy->nphys = of_get_child_count(np);
	tphy->phys = devm_kcalloc(dev, tphy->nphys,
					   sizeof(*tphy->phys), GFP_KERNEL);
	if (!tphy->phys)
		return -ENOMEM;

	retval = device_rename(dev, np->name);
	if (retval)
		dev_info(&pdev->dev, "failed to rename\n");
	/* fix uaf(use after free) issue: backup pdev->name,
	 * device_rename will free pdev->name
	 */
	pdev->name = pdev->dev.kobj.name;
	u3phy_dev = dev;

	tphy->dev = dev;
	platform_set_drvdata(pdev, tphy);

	sif_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	/* SATA phy of V1 needn't it if not shared with PCIe or USB */
	if (sif_res && tphy->pdata->version == MTK_PHY_V1) {
		/* get banks shared by multiple phys */
		tphy->sif_base = devm_ioremap_resource(dev, sif_res);
		if (IS_ERR(tphy->sif_base)) {
			dev_err(dev, "failed to remap sif regs\n");
			return PTR_ERR(tphy->sif_base);
		}
	}

	/* it's deprecated, make it optional for backward compatibility */
	tphy->u3phya_ref = devm_clk_get_optional(dev, "u3phya_ref");
	if (IS_ERR(tphy->u3phya_ref))
		return PTR_ERR(tphy->u3phya_ref);

	tphy->src_ref_clk = U3P_REF_CLK;
	tphy->src_coef = U3P_SLEW_RATE_COEF;
	/* update parameters of slew rate calibrate if exist */
	device_property_read_u32(dev, "mediatek,src-ref-clk-mhz",
		&tphy->src_ref_clk);
	device_property_read_u32(dev, "mediatek,src-coef", &tphy->src_coef);

	port = 0;
	for_each_child_of_node(np, child_np) {
		struct mtk_phy_instance *instance;
		struct phy *phy;

		instance = devm_kzalloc(dev, sizeof(*instance), GFP_KERNEL);
		if (!instance) {
			retval = -ENOMEM;
			goto put_child;
		}

		tphy->phys[port] = instance;

		phy = devm_phy_create(dev, child_np, &a60810_u3phy_ops);
		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create phy\n");
			retval = PTR_ERR(phy);
			goto put_child;
		}

		retval = of_property_read_u32(np, "mediatek,ippc", &ippc);
		if (retval) {
			dev_err(dev, "Failed to parse ippc value\n");
			goto put_child;
		}

		instance->port_base = ioremap(ippc, 0x100);
		if (!instance->port_base) {
			dev_err(dev, "could not ioremap ippc regs\n");
			goto put_child;
		}

		instance->phy = phy;
		instance->index = port;
		phy_set_drvdata(phy, instance);
		port++;

		/* if deprecated clock is provided, ignore instance's one */
		if (tphy->u3phya_ref)
			continue;

		instance->ref_clk = devm_clk_get(&phy->dev, "ref");
		if (IS_ERR(instance->ref_clk)) {
			dev_err(dev, "failed to get ref_clk(id-%d)\n", port);
			retval = PTR_ERR(instance->ref_clk);
			goto put_child;
		}
	}

	provider = devm_of_phy_provider_register(dev, mtk_phy_xlate);

	return PTR_ERR_OR_ZERO(provider);
put_child:
	of_node_put(child_np);
	return retval;

}

MODULE_DEVICE_TABLE(of, a60810_u3phy_id_table);

static struct platform_driver a60810_u3phy_driver = {
	.probe		= u3phy_a60810_probe,
	.driver		= {
		.name	= "a60810-u3phy",
		.of_match_table = a60810_u3phy_id_table,
	},
};

module_platform_driver(a60810_u3phy_driver);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_DESCRIPTION("A60810 U3PHY driver");
MODULE_LICENSE("GPL v2");
