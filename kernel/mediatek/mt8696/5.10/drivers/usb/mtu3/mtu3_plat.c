// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/arm-smccc.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/phy/mtk_usb_phy.h>
#include <dt-bindings/phy/phy.h>

#include "mtu3.h"
#include "mtu3_dr.h"
#include "mtu3_debug.h"

#ifdef CONFIG_MTU3_SUPPORT_Y_CABLE
static bool lk_y_cable;
static int  lk_usb_extconn;
enum usb_extconn_type {
	MT_USB_EXTCONN_UNKNOWN = 0,
	MT_USB_EXTCONN_STANDARDHOST = 1,
	MT_USB_EXTCONN_CHARGINGHOST = 2,
	MT_USB_EXTCONN_NONSTANDARDCHARGER = 3,
	MT_USB_EXTCONN_STANDARDCHARGER = 4,
	MT_USB_EXTCONN_MAXIMUM = 5,
};

const char *szusb_extconn_type[] = {
	 "UNKNOWN LINE TYPE",
	 "STANDARD_HOST",
	 "CHARGING_HOST",
	 "NONSTANDARD_CHARGER",
	 "STANDARD_CHARGER",
	 "INVALID PARAMETER",
};
#endif

enum {
	MTU3_SMC_INFRA_REQUEST = 0,
	MTU3_SMC_INFRA_RELEASE,
	MTU3_SMC_INFRA_NUM,
};

void ssusb_set_txdeemph(struct ssusb_mtk *ssusb)
{
	u32 txdeemph;

	if (!ssusb->gen1_txdeemph)
		return;

	txdeemph = mtu3_readl(ssusb->mac_base, U3D_TXDEEMPH);
	txdeemph &= ~PIPE_TXDEEMPH_MASK;
	txdeemph |= PIPE_TXDEEMPH(0x1);
	mtu3_writel(ssusb->mac_base, U3D_TXDEEMPH, txdeemph);
}

static void ssusb_smc_request(struct ssusb_mtk *ssusb,
	enum mtu3_power_state state)
{
	struct arm_smccc_res res;
	int op;

	dev_info(ssusb->dev, "%s state = %d\n", __func__, state);

	switch (state) {
	case MTU3_STATE_POWER_OFF:
		op = MTU3_SMC_INFRA_RELEASE;
		break;
	case MTU3_STATE_POWER_ON:
		op = MTU3_SMC_INFRA_REQUEST;
		break;
	default:
		return;
	}

	arm_smccc_smc(MTK_SIP_KERNEL_USB_CONTROL,
		op, 0, 0, 0, 0, 0, 0, &res);
}

static void ssusb_hw_request(struct ssusb_mtk *ssusb,
	enum mtu3_power_state state)
{
	u32 req;
	u32 spm_ctrl;

	dev_info(ssusb->dev, "%s state = %d\n", __func__, state);

	switch (state) {
	case MTU3_STATE_POWER_OFF:
		req = 0x0;
		break;
	case MTU3_STATE_POWER_ON:
	case MTU3_STATE_RESUME:
		req = SSUSB_SPM_DDR_EN | SSUSB_SPM_VRF18_REQ |
			SSUSB_SPM_APSRC_REQ | SSUSB_SPM_INFRE_REQ |
			SSUSB_SPM_SRCCLKENA;
		break;
	case MTU3_STATE_SUSPEND:
		req = SSUSB_SPM_INFRE_REQ | SSUSB_SPM_SRCCLKENA;
		break;
	default:
		return;
	}

	spm_ctrl = mtu3_readl(ssusb->ippc_base, U3D_SSUSB_SPM_CTRL);
	spm_ctrl &= ~SSUSB_SPM_REQ_MSK;
	spm_ctrl |= req;
	mtu3_writel(ssusb->ippc_base, U3D_SSUSB_SPM_CTRL, spm_ctrl);
}

void ssusb_set_power_state(struct ssusb_mtk *ssusb,
	enum mtu3_power_state state)
{
	if (ssusb->plat_type == PLAT_FPGA || !ssusb->clk_mgr)
		return;

	if (ssusb->hw_req_ctrl)
		ssusb_hw_request(ssusb, state);
	else
		ssusb_smc_request(ssusb, state);
}

void ssusb_set_force_vbus(struct ssusb_mtk *ssusb, bool vbus_on)
{
	u32 u2ctl;
	u32 misc;

	if (!ssusb->force_vbus)
		return;

	u2ctl = mtu3_readl(ssusb->ippc_base, SSUSB_U2_CTRL(0));
	misc = mtu3_readl(ssusb->mac_base, U3D_MISC_CTRL);
	if (vbus_on) {
		u2ctl &= ~SSUSB_U2_PORT_OTG_SEL;
		misc |= VBUS_FRC_EN | VBUS_ON;
	} else {
		u2ctl |= SSUSB_U2_PORT_OTG_SEL;
		misc &= ~(VBUS_FRC_EN | VBUS_ON);
	}
	mtu3_writel(ssusb->ippc_base, SSUSB_U2_CTRL(0), u2ctl);
	mtu3_writel(ssusb->mac_base, U3D_MISC_CTRL, misc);
}

/* u2-port0 should be powered on and enabled; */
int ssusb_check_clocks(struct ssusb_mtk *ssusb, u32 ex_clks)
{
	void __iomem *ibase = ssusb->ippc_base;
	u32 value, check_val;
	int ret;

	check_val = ex_clks | SSUSB_SYS125_RST_B_STS | SSUSB_SYSPLL_STABLE |
			SSUSB_REF_RST_B_STS;

	ret = readl_poll_timeout(ibase + U3D_SSUSB_IP_PW_STS1, value,
			(check_val == (value & check_val)), 100, 20000);
	if (ret) {
		dev_err(ssusb->dev,
		"clks of sts1 are not stable, check_val:0x%08x, value:0x%08x!\n",
			check_val, value);
		return ret;
	}

	ret = readl_poll_timeout(ibase + U3D_SSUSB_IP_PW_STS2, value,
			(value & SSUSB_U2_MAC_SYS_RST_B_STS), 100, 10000);
	if (ret) {
		dev_err(ssusb->dev, "mac2 clock is not stable\n");
		return ret;
	}

	return 0;
}

static int ssusb_phy_init(struct ssusb_mtk *ssusb)
{
	int i;
	int ret;
	struct mtk_op_phy *op_phy;

	for (i = 0; i < ssusb->num_phys; i++) {
		op_phy = &ssusb->op_phys[i];
		if (op_phy->phy_disable)
			continue;

		ret = phy_init(op_phy->phy);
		if (ret)
			goto exit_phy;
	}

	return 0;

exit_phy:
	for (; i > 0; i--) {
		op_phy = &ssusb->op_phys[i-1];
		if (op_phy->phy_disable)
			continue;

		phy_exit(op_phy->phy);
	}

	return ret;
}

static int ssusb_phy_exit(struct ssusb_mtk *ssusb)
{
	int i;
	struct mtk_op_phy *op_phy;

	for (i = 0; i < ssusb->num_phys; i++) {
		op_phy = &ssusb->op_phys[i];
		if (op_phy->phy_disable)
			continue;

		phy_exit(op_phy->phy);
	}

	return 0;
}

int ssusb_phy_power_on(struct ssusb_mtk *ssusb)
{
	int i;
	int ret;
	struct mtk_op_phy *op_phy;

	if (ssusb->phy_on == true) {
		dev_info(ssusb->dev, "phy already enabled\n");
		return 0;
	}

	for (i = 0; i < ssusb->num_phys; i++) {
		op_phy = &ssusb->op_phys[i];
		if (op_phy->phy_disable)
			continue;

		ret = phy_power_on(op_phy->phy);
		if (ret)
			goto power_off_phy;

		ret = mtk_phy_regular_ctrl(op_phy->phy, true);
		if (ret)
			goto power_off_phy;
	}
	ssusb->phy_on = true;

	return 0;

power_off_phy:
	for (; i > 0; i--) {
		op_phy = &ssusb->op_phys[i-1];
		if (op_phy->phy_disable)
			continue;

		phy_power_off(op_phy->phy);
		mtk_phy_regular_ctrl(op_phy->phy, false);

	}
	ssusb->phy_on = false;
	return ret;
}

void ssusb_phy_power_off(struct ssusb_mtk *ssusb)
{
	unsigned int i;
	struct mtk_op_phy *op_phy;

	if (ssusb->phy_on == false) {
		dev_info(ssusb->dev, "phy already disabled\n");
		return;
	}

	for (i = 0; i < ssusb->num_phys; i++) {
		op_phy = &ssusb->op_phys[i];
		if (op_phy->phy_disable)
			continue;

		phy_power_off(op_phy->phy);
		mtk_phy_regular_ctrl(op_phy->phy, false);
	}
	ssusb->phy_on = false;
}


void ssusb_phy_set_mode(struct ssusb_mtk *ssusb, enum phy_mode mode)
{
	unsigned int i;
	struct mtk_op_phy *op_phy;

	for (i = 0; i < ssusb->num_phys; i++) {
		op_phy = &ssusb->op_phys[i];
		if (op_phy->phy_disable)
			continue;

		phy_set_mode(op_phy->phy, mode);
	}
}

int ssusb_clks_enable(struct ssusb_mtk *ssusb)
{
	int ret;

	if (ssusb->clk_on == true) {
		dev_info(ssusb->dev, "clk already enabled\n");
		return 0;
	}

	ret = clk_prepare_enable(ssusb->sys_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable sys_clk\n");
		goto sys_clk_err;
	}

	ret = clk_prepare_enable(ssusb->frmcnt_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable frmcnt_clk\n");
		goto frmcnt_clk_err;
	}

	ret = clk_prepare_enable(ssusb->ref_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable ref_clk\n");
		goto ref_clk_err;
	}

	ret = clk_prepare_enable(ssusb->host_clk);
	if (ret) {
		dev_info(ssusb->dev, "failed to enable host_clk\n");
		goto host_clk_err;
	}

	ret = clk_prepare_enable(ssusb->mcu_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable mcu_clk\n");
		goto mcu_clk_err;
	}

	ret = clk_prepare_enable(ssusb->dma_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable dma_clk\n");
		goto dma_clk_err;
	}
	ssusb->clk_on = true;

	return 0;

dma_clk_err:
	clk_disable_unprepare(ssusb->mcu_clk);
mcu_clk_err:
	clk_disable_unprepare(ssusb->host_clk);
host_clk_err:
	clk_disable_unprepare(ssusb->ref_clk);
ref_clk_err:
	clk_disable_unprepare(ssusb->frmcnt_clk);
frmcnt_clk_err:
	clk_disable_unprepare(ssusb->sys_clk);
sys_clk_err:
	ssusb->clk_on = false;
	return ret;
}

enum usb_dr_mode usb_get_default_mode(struct device *dev)
{
	const char *default_mode = NULL;
	int err;

	err = device_property_read_string(dev, "default_mode", &default_mode);
	if (err < 0)
		return USB_DR_MODE_UNKNOWN;

	if (default_mode != NULL  && !strcmp(default_mode, "peripheral"))
		return USB_DR_MODE_PERIPHERAL;
	else if (default_mode != NULL  && !strcmp(default_mode, "host"))
		return USB_DR_MODE_HOST;
	else
		return USB_DR_MODE_UNKNOWN;
}

void ssusb_clks_disable(struct ssusb_mtk *ssusb)
{
	if (ssusb->clk_on == false) {
		dev_info(ssusb->dev, "clk already disabled\n");
		return;
	}

	clk_disable_unprepare(ssusb->dma_clk);
	clk_disable_unprepare(ssusb->mcu_clk);
	clk_disable_unprepare(ssusb->host_clk);
	clk_disable_unprepare(ssusb->ref_clk);
	clk_disable_unprepare(ssusb->frmcnt_clk);
	clk_disable_unprepare(ssusb->sys_clk);
	ssusb->clk_on = false;
}

static int ssusb_rscs_init(struct ssusb_mtk *ssusb)
{
	int ret = 0;

	if (ssusb->plat_type == PLAT_FPGA)
		goto phy_init;

	ret = regulator_enable(ssusb->vusb33);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable vusb33\n");
		goto vusb33_err;
	}

	ret = ssusb_clks_enable(ssusb);
	if (ret)
		goto clks_err;

phy_init:
	ret = ssusb_phy_init(ssusb);
	if (ret) {
		dev_err(ssusb->dev, "failed to init phy\n");
		goto phy_init_err;
	}

	ret = ssusb_phy_power_on(ssusb);
	if (ret) {
		dev_err(ssusb->dev, "failed to power on phy\n");
		goto phy_err;
	}

	return 0;

phy_err:
	ssusb_phy_exit(ssusb);
phy_init_err:
	if (ssusb->plat_type == PLAT_FPGA)
		return ret;

	ssusb_clks_disable(ssusb);
clks_err:
	regulator_disable(ssusb->vusb33);
vusb33_err:
	return ret;
}

static void ssusb_rscs_exit(struct ssusb_mtk *ssusb)
{
	ssusb_clks_disable(ssusb);
	regulator_disable(ssusb->vusb33);
	ssusb_phy_power_off(ssusb);
	ssusb_phy_exit(ssusb);
}

void ssusb_ip_sw_reset(struct ssusb_mtk *ssusb)
{
	/* reset whole ip (xhci & u3d) */
	mtu3_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
	udelay(1);
	mtu3_clrbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);

	/*
	 * device ip may be powered on in firmware/BROM stage before entering
	 * kernel stage;
	 * power down device ip, otherwise ip-sleep will fail when working as
	 * host only mode
	 */
	mtu3_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL2,
				SSUSB_IP_DEV_PDN);
}

static int get_ssusb_rscs(struct platform_device *pdev, struct ssusb_mtk *ssusb)
{
	struct device_node *node = pdev->dev.of_node;
	struct otg_switch_mtk *otg_sx = &ssusb->otg_switch;
	struct device *dev = &pdev->dev;
	struct regulator *vbus;
	struct of_phandle_args args;
	u32 u2port = 0;
	u32 u3port = 0;
	int i;
	int ret;

	ret = of_property_read_u32(node, "plat_type", &ssusb->plat_type);
	if (!ret && ssusb->plat_type == PLAT_FPGA) {
		dev_info(ssusb->dev, "platform is fpga\n");

		of_property_read_u32(node, "fpga_phy",
				&ssusb->fpga_phy);

		dev_info(ssusb->dev, "fpga phy is %d\n", ssusb->fpga_phy);
		goto get_phy;
	}

	ssusb->vusb33 = devm_regulator_get(dev, "vusb33");
	if (IS_ERR(ssusb->vusb33)) {
		dev_err(dev, "failed to get vusb33\n");
		return PTR_ERR(ssusb->vusb33);
	}

	ssusb->sys_clk = devm_clk_get(dev, "sys_ck");
	if (IS_ERR(ssusb->sys_clk)) {
		dev_err(dev, "failed to get sys clock\n");
		return PTR_ERR(ssusb->sys_clk);
	}

	ssusb->frmcnt_clk = devm_clk_get_optional(dev, "frmcnt_ck");
	if (IS_ERR(ssusb->frmcnt_clk))
		return PTR_ERR(ssusb->frmcnt_clk);

	ssusb->ref_clk = devm_clk_get_optional(dev, "ref_ck");
	if (IS_ERR(ssusb->ref_clk))
		return PTR_ERR(ssusb->ref_clk);

	ssusb->host_clk = devm_clk_get_optional(dev, "host_ck");
	if (IS_ERR(ssusb->host_clk))
		return PTR_ERR(ssusb->host_clk);

	ssusb->mcu_clk = devm_clk_get_optional(dev, "mcu_ck");
	if (IS_ERR(ssusb->mcu_clk))
		return PTR_ERR(ssusb->mcu_clk);

	ssusb->dma_clk = devm_clk_get_optional(dev, "dma_ck");
	if (IS_ERR(ssusb->dma_clk))
		return PTR_ERR(ssusb->dma_clk);

get_phy:
	ssusb->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	if (ssusb->num_phys > 0) {
		ssusb->op_phys = devm_kcalloc(dev, ssusb->num_phys,
					sizeof(*ssusb->op_phys), GFP_KERNEL);
		if (!ssusb->op_phys)
			return -ENOMEM;
	} else
		ssusb->num_phys = 0;

	for (i = 0; i < ssusb->num_phys; i++) {
		int err;

		ssusb->op_phys[i].phy_index = i;
		ssusb->op_phys[i].phy = devm_of_phy_get_by_index(dev, node, i);
		if (ssusb->op_phys[i].phy == ERR_PTR(-ENODEV))
			ssusb->op_phys[i].phy_disable	= true;
		else if (IS_ERR(ssusb->op_phys[i].phy)) {
			ret = PTR_ERR(ssusb->op_phys[i].phy);
			dev_err(dev,
				"get phy-%d failed since ret:%i\n", i, ret);
			return ret;
		}

		err = of_parse_phandle_with_args(node,
			"phys", "#phy-cells",
			i, &args);
		if (err) {
			dev_info(dev,
				"Failed to get phys phandle at index %d: %d\n",
				i, err);
			continue;
		}

		if (u2port < USB_SS_MAXPORTS
			&& args.args[0] == PHY_TYPE_USB2) {
			if (of_device_is_available(args.np))
				ssusb->u2port_power[u2port] = true;

			u2port++;
		}

		if (u3port < USB_MAXCHILDREN
			 && args.args[0] == PHY_TYPE_USB3) {
			if (of_device_is_available(args.np))
				ssusb->u3port_power[u3port] = true;

			u3port++;
		}

		dev_info(dev, "USB%i PHY has %sabled now.\n", i,
				ssusb->op_phys[i].phy_disable ? "dis":"en");
	}

#ifdef CONFIG_FPGA_EARLY_PORTING
	ssusb->ippc_base = of_iomap(node, 1);
#else
	ssusb->ippc_base = devm_platform_ioremap_resource_byname(pdev, "ippc");
	if (IS_ERR(ssusb->ippc_base))
		return PTR_ERR(ssusb->ippc_base);
#endif

	ssusb->force_vbus = of_property_read_bool(node, "mediatek,force-vbus");
	ssusb->clk_mgr = of_property_read_bool(node, "mediatek,clk-mgr");
	ssusb->hw_req_ctrl = of_property_read_bool(node, "mediatek,hw-req-ctrl");
	ssusb->keep_ao = of_property_read_bool(node, "mediatek,keep-host-on");
	ssusb->noise_still_tr =
		of_property_read_bool(node, "mediatek,noise-still-tr");
	ssusb->gen1_txdeemph =
		of_property_read_bool(node, "mediatek,gen1-txdeemph");

	ssusb->dr_mode = usb_get_dr_mode(dev);
	if (ssusb->dr_mode == USB_DR_MODE_UNKNOWN)
		ssusb->dr_mode = USB_DR_MODE_OTG;

	if (ssusb->dr_mode == USB_DR_MODE_PERIPHERAL)
		goto out;

	/* if host role is supported */
	ret = ssusb_wakeup_of_property_parse(ssusb, node);
	if (ret) {
		dev_err(dev, "failed to parse uwk property\n");
		return ret;
	}

	/* optional property, ignore the error if it does not exist */
	of_property_read_u32(node, "mediatek,u3p-dis-msk",
			     &ssusb->u3p_dis_msk);

	vbus = devm_regulator_get(dev, "vbus");
	if (IS_ERR(vbus)) {
		dev_err(dev, "failed to get vbus\n");
		return PTR_ERR(vbus);
	}
	otg_sx->vbus = vbus;

	if (ssusb->dr_mode == USB_DR_MODE_HOST)
		goto out;

	/* if dual-role mode is supported */
	otg_sx->is_u3_drd = of_property_read_bool(node, "mediatek,usb3-drd");
#ifndef CONFIG_MTU3_SUPPORT_Y_CABLE
	otg_sx->default_mode = usb_get_default_mode(dev);
	otg_sx->manual_drd_enabled =
		of_property_read_bool(node, "enable-manual-drd");
#else
		otg_sx->manual_drd_enabled = true;
		if (lk_y_cable) {
			dev_info(dev, "Pull down IDDIG function\n");
			otg_sx->default_mode = USB_DR_MODE_HOST;
		} else if (lk_usb_extconn == MT_USB_EXTCONN_STANDARDHOST) {
			dev_info(dev, "Pull up IDDIG function\n");
			otg_sx->default_mode = USB_DR_MODE_PERIPHERAL;
		} else {
			int line_type = lk_usb_extconn;

			if (lk_usb_extconn > MT_USB_EXTCONN_MAXIMUM)
				line_type = MT_USB_EXTCONN_MAXIMUM;

			dev_info(dev,
				"Can't support extended connector line type: %s (%i)\n",
				szusb_extconn_type[line_type],
				lk_usb_extconn);
			return -EPERM;
		}
#endif

	otg_sx->role_sw_used = of_property_read_bool(node, "usb-role-switch");

	if (!otg_sx->role_sw_used && of_property_read_bool(node, "extcon")) {
		otg_sx->edev = extcon_get_edev_by_phandle(ssusb->dev, 0);
		if (otg_sx->edev == ERR_PTR(-EPROBE_DEFER)) {
			return -EPROBE_DEFER;
		} else if (IS_ERR(otg_sx->edev)) {
			dev_err(ssusb->dev, "couldn't get extcon device\n");
			ret = PTR_ERR(otg_sx->edev);
			dev_err(dev,
				"couldn't get extcon device since %i\n", ret);
			return ret;
		}
	}

out:
	dev_info(dev, "dr_mode: %d, is_u3_dr: %d, u3p_dis_msk: %x, drd: %s\n",
		ssusb->dr_mode, otg_sx->is_u3_drd, ssusb->u3p_dis_msk,
		otg_sx->manual_drd_enabled ? "manual" : "auto");

	return 0;
}

static int mtu3_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct ssusb_mtk *ssusb;
	int ret = -ENOMEM;

	/* all elements are set to ZERO as default value */
	ssusb = devm_kzalloc(dev, sizeof(*ssusb), GFP_KERNEL);
	if (!ssusb)
		return -ENOMEM;

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "No suitable DMA config available\n");
		return -ENOTSUPP;
	}

	platform_set_drvdata(pdev, ssusb);
	ssusb->dev = dev;

	ret = get_ssusb_rscs(pdev, ssusb);
	if (ret)
		return ret;

	ssusb_debugfs_create_root(ssusb);

	/* enable power domain */
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	device_enable_async_suspend(dev);

	ret = ssusb_rscs_init(ssusb);
	if (ret)
		goto comm_init_err;

	if (IS_ENABLED(CONFIG_USB_MTU3_HOST))
		ssusb->dr_mode = USB_DR_MODE_HOST;
	else if (IS_ENABLED(CONFIG_USB_MTU3_GADGET))
		ssusb->dr_mode = USB_DR_MODE_PERIPHERAL;

	ssusb_ip_sw_reset(ssusb);

	/* default as host */
	ssusb->is_host = !(ssusb->dr_mode == USB_DR_MODE_PERIPHERAL);

	switch (ssusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		ret = ssusb_gadget_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto comm_exit;
		}
		ssusb_set_force_mode(ssusb, MTU3_DR_FORCE_DEVICE);
		break;
	case USB_DR_MODE_HOST:
		ret = ssusb_host_init(ssusb, node);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto comm_exit;
		}
		ssusb_set_force_mode(ssusb, MTU3_DR_FORCE_HOST);
		break;
	case USB_DR_MODE_OTG:
		if (ssusb->num_phys && !ssusb->op_phys[0].phy_disable) {
			ret = ssusb_gadget_init(ssusb);
			if (ret) {
				dev_err(dev, "failed to initialize gadget\n");
				goto comm_exit;
			}
		} else
			dev_info(dev,
			 "Port0 USB PHY had been required to disabled.\n");

		if (ssusb->keep_ao) {
			ret = ssusb_host_init(ssusb, node);
			if (ret) {
				dev_err(dev, "failed to initialize host\n");
				goto gadget_exit;
			}
		}

		if (!ssusb->keep_ao) {
			ssusb_phy_power_off(ssusb);
			ssusb_clks_disable(ssusb);
		}

		if (ssusb->num_phys && !ssusb->op_phys[0].phy_disable) {
			ret = ssusb_otg_switch_init(ssusb);
			if (ret) {
				dev_err(dev, "failed to initialize switch\n");
				goto host_exit;
			}
		}
		break;
	default:
		dev_err(dev, "unsupported mode: %d\n", ssusb->dr_mode);
		ret = -EINVAL;
		goto comm_exit;
	}

	ssusb_set_power_state(ssusb, MTU3_STATE_POWER_ON);

#ifdef CONFIG_DEBUG_FS
	dev_info(dev,
		 "===%s u2:u3 ports: %u:%u in ssusb 0x%p ===\n",
		 __func__, ssusb->u2_ports, ssusb->u3_ports, ssusb);
	mtk_ssusb_init_debugfs(ssusb);
#endif

	return 0;

host_exit:
	ssusb_host_exit(ssusb);
gadget_exit:
	ssusb_gadget_exit(ssusb);
comm_exit:
	ssusb_rscs_exit(ssusb);
comm_init_err:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	ssusb_debugfs_remove_root(ssusb);

	return ret;
}

static int mtu3_remove(struct platform_device *pdev)
{
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);

#ifdef CONFIG_DEBUG_FS
	mtk_ssusb_exit_debugfs(ssusb);
#endif

	switch (ssusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		ssusb_gadget_exit(ssusb);
		break;
	case USB_DR_MODE_HOST:
		ssusb_host_exit(ssusb);
		break;
	case USB_DR_MODE_OTG:
		ssusb_otg_switch_exit(ssusb);
		ssusb_gadget_exit(ssusb);
		ssusb_host_exit(ssusb);
		break;
	default:
		return -EINVAL;
	}

	ssusb_set_power_state(ssusb, MTU3_STATE_POWER_OFF);

	ssusb_rscs_exit(ssusb);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	ssusb_debugfs_remove_root(ssusb);

	return 0;
}

#ifdef CONFIG_MTU3_SUPPORT_Y_CABLE
static int __init usb_y_cable_setup(char *str)
{
	if (!strcmp(str, "1"))
		lk_y_cable = true;
	else if (!strcmp(str, "0"))
		lk_y_cable = false;
	else
		lk_y_cable = false;

	return 1;
}
early_param("y_cable", usb_y_cable_setup);

static int __init usb_line_type_setup(char *str)
{
	int ret = 0;

	ret = get_option(&str, &lk_usb_extconn);
	if (ret != 1) {
		pr_info("usb_extconn = %i, Defaulting to device since ret: %i\n",
				 lk_usb_extconn, ret);
		lk_usb_extconn = MT_USB_EXTCONN_STANDARDHOST;
	}

	return 1;
}
early_param("usb_extconn", usb_line_type_setup);
#endif

/*
 * when support dual-role mode, we reject suspend when
 * it works as device mode;
 */
static int __maybe_unused mtu3_suspend(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	/* REVISIT: disconnect it for only device mode? */
	if (!ssusb->is_host)
		return 0;

	ssusb_set_power_state(ssusb, MTU3_STATE_SUSPEND);

	ssusb_host_disable(ssusb, true);
	ssusb_phy_power_off(ssusb);
	ssusb_clks_disable(ssusb);
	ssusb_wakeup_set(ssusb, true);

	return 0;
}

static int __maybe_unused mtu3_resume(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	if (!ssusb->is_host)
		return 0;

	ssusb_wakeup_set(ssusb, false);
	ret = ssusb_clks_enable(ssusb);
	if (ret)
		goto clks_err;

	ret = ssusb_phy_power_on(ssusb);
	if (ret)
		goto phy_err;

	ssusb_host_enable(ssusb);

	ssusb_set_power_state(ssusb, MTU3_STATE_RESUME);

	return 0;

phy_err:
	ssusb_clks_disable(ssusb);
clks_err:
	return ret;
}

static int __maybe_unused mtu3_runtime_suspend(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);
	int ret = 0;

	dev_info(ssusb->dev, "%s\n", __func__);
	if (dev->power.runtime_status == RPM_SUSPENDED) {
		dev_info(ssusb->dev, "%s: already suspend\n", __func__);
		return 0;
	}

	ret = mtu3_suspend(ssusb->dev);
	if (!ret) {
		dev_info(ssusb->dev, "%s: successfully!\n", __func__);
		dev->power.runtime_status = RPM_SUSPENDED;
	}
	return 0;
}

static int __maybe_unused mtu3_runtime_resume(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);
	int ret = 0;

	dev_info(ssusb->dev, "%s\n", __func__);
	if (dev->power.runtime_status == RPM_ACTIVE) {
		dev_info(ssusb->dev, "%s: already resume\n", __func__);
		return 0;
	}

	ret = mtu3_resume(ssusb->dev);
	if (!ret) {
		dev_info(ssusb->dev, "%s: successfully!\n", __func__);
		dev->power.runtime_status = RPM_ACTIVE;
	}
	return 0;
}

static const struct dev_pm_ops mtu3_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtu3_suspend, mtu3_resume)
	SET_RUNTIME_PM_OPS(mtu3_runtime_suspend,
				mtu3_runtime_resume,
				NULL)
};

#define DEV_PM_OPS (IS_ENABLED(CONFIG_PM) ? &mtu3_pm_ops : NULL)

#ifdef CONFIG_OF

static const struct of_device_id mtu3_of_match[] = {
	{.compatible = "mediatek,mt8173-mtu3",},
	{.compatible = "mediatek,mtu3",},
	{},
};

MODULE_DEVICE_TABLE(of, mtu3_of_match);

#endif

static struct platform_driver mtu3_driver = {
	.probe = mtu3_probe,
	.remove = mtu3_remove,
	.driver = {
		.name = MTU3_DRIVER_NAME,
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(mtu3_of_match),
	},
};
module_platform_driver(mtu3_driver);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek USB3 DRD Controller Driver");
