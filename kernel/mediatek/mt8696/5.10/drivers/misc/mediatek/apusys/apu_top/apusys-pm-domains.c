// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_clk.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

#include "apusys-pm-domains.h"
#include "apusys_pm_ctl_reg.h"

#define MTK_POLL_DELAY_US		10
#define MTK_POLL_TIMEOUT		USEC_PER_SEC

struct apusys {
	struct device *dev;

	struct regmap *rpc;
	struct regmap *acc;
	struct regmap *vcore;
	struct regmap *conn;
	struct regmap *conn1;
	struct regmap *ao_ctrl;
	struct regmap *infracfg;

	struct regulator *reg_vsram;

	const struct apusys_data *data;
	struct genpd_onecell_data pd_data;
	struct generic_pm_domain *domains[];
};

struct apusys_domain {
	struct generic_pm_domain genpd;
	const struct apusys_domain_data *data;
	struct apusys *apusys;
	int num_clks;
	struct clk_bulk_data *clks;
	struct regulator *supply;
};

#define to_apusys_domain(gpd) container_of(gpd, struct apusys_domain, genpd)

static bool apusys_domain_is_on(struct apusys_domain *pd)
{

	/* A domain is on when both status bits are set. */
	return true;
}

static int apusys_power_on(struct generic_pm_domain *genpd)
{
	return 0;
}

static int apusys_power_off(struct generic_pm_domain *genpd)
{
	return 0;
}

static struct
generic_pm_domain *apusys_add_one_domain(struct apusys *apusys,
					 struct device_node *node)
{
	const struct apusys_domain_data *domain_data;
	struct apusys_domain *pd;
	int i, ret;
	struct clk *clk;
	u32 id;

	ret = of_property_read_u32(node, "reg", &id);
	if (ret) {
		dev_dbg(apusys->dev, "%pOF: failed to retrieve domain id from reg: %d\n",
			node, ret);
		return ERR_PTR(-EINVAL);
	}

	if (id >= apusys->data->num_domains) {
		dev_dbg(apusys->dev, "%pOF: invalid domain id %d\n", node, id);
		return ERR_PTR(-EINVAL);
	}

	domain_data = &apusys->data->domains_data[id];

	pd = devm_kzalloc(apusys->dev, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return ERR_PTR(-ENOMEM);

	pd->data   = domain_data;
	pd->apusys = apusys;

	pd->num_clks = of_clk_get_parent_count(node);
	if (pd->num_clks > 0) {
		pd->clks = devm_kcalloc(apusys->dev, pd->num_clks,
					sizeof(*pd->clks), GFP_KERNEL);
		if (!pd->clks)
			return ERR_PTR(-ENOMEM);
	}

	for (i = 0; i < pd->num_clks; i++) {
		clk = of_clk_get(node, i);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			dev_dbg(apusys->dev,
				"%pOF: failed to get clk at index %d: %d\n",
				node, i, ret);
			goto err_put_clocks;
		}
		pd->clks[i].clk = clk;
	}

	dev_info(apusys->dev,
		"try devm_regulator_get on %s\n", node->name);
	pd->supply = devm_regulator_get_optional(apusys->dev, node->name);
	if (IS_ERR(pd->supply))
		pd->supply = NULL;

	if (apusys->domains[id]) {
		ret = -EINVAL;
		dev_dbg(apusys->dev,
			"power domain with id %d already exists\n", id);
		goto err_put_clocks;
	}

	pd->genpd.name = node->name;
	pd->genpd.power_off = apusys_power_off;
	pd->genpd.power_on = apusys_power_on;

	/*
	 * Initially turn on all domains to make the domains usable
	 * with !CONFIG_PM and to get the hardware in sync with the
	 * software.  The unused domains will be switched off during
	 * late_init time.
	 */

	ret = pd->genpd.power_on(&pd->genpd);
	if (ret < 0) {
		dev_dbg(apusys->dev, "%pOF: failed to power on domain: %d\n", node, ret);
		goto err_put_clocks;
	}

	pm_genpd_init(&pd->genpd, NULL, false);

	apusys->domains[id] = &pd->genpd;

	dev_info(apusys->dev, "genpd add domains %s\n", pd->genpd.name);

	return apusys->pd_data.domains[id];

err_put_clocks:
	clk_bulk_put(pd->num_clks, pd->clks);
	return ERR_PTR(ret);
}

static void apusys_remove_one_domain(struct apusys_domain *pd)
{
	int ret;

	if (apusys_domain_is_on(pd) && pd->genpd.power_off)
		pd->genpd.power_off(&pd->genpd);

	/*
	 * We're in the error cleanup already, so we only complain,
	 * but won't emit another error on top of the original one.
	 */
	ret = pm_genpd_remove(&pd->genpd);
	if (ret < 0)
		dev_dbg(pd->apusys->dev,
			"failed to remove domain '%s' : %d - state may be inconsistent\n",
			pd->genpd.name, ret);

	clk_bulk_put(pd->num_clks, pd->clks);
}

static void apusys_domain_cleanup(struct apusys *apusys)
{
	struct generic_pm_domain *genpd;
	struct apusys_domain *pd;
	int i;

	for (i = apusys->pd_data.num_domains - 1; i >= 0; i--) {
		genpd = apusys->pd_data.domains[i];
		if (genpd) {
			pd = to_apusys_domain(genpd);
			apusys_remove_one_domain(pd);
		}
	}
}

static const struct of_device_id apusys_of_match[] = {
	{
		.compatible = "mediatek,apusys-power-controller",
	},
	{ }
};

static int apusys_syscon_regmap(struct regmap **base,
				struct device *dev,
				const char *phandle_name)
{
	*base = syscon_regmap_lookup_by_phandle(dev->of_node, phandle_name);

	if (IS_ERR(*base)) {
		dev_dbg(dev, "Cannot find %s: %ld\n",
			phandle_name, PTR_ERR(*base));
		return PTR_ERR(*base);
	}
	return 0;
}

static int apusys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct genpd_onecell_data *pd_data;
	const struct apusys_data *data;
	struct device_node *node;
	struct apusys *apusys;
	int i, ret;

	data = of_device_get_match_data(&pdev->dev);
	if (!data) {
		dev_dbg(dev, "no power controller data\n");
		return -EINVAL;
	}

	apusys = devm_kzalloc(dev, struct_size(apusys, domains, data->num_domains), GFP_KERNEL);
	if (!apusys)
		return -ENOMEM;

	apusys->dev = dev;
	apusys->data = data;

	apusys->pd_data.domains = apusys->domains;
	apusys->pd_data.num_domains = data->num_domains;

	//Vsram
	apusys->reg_vsram = devm_regulator_get_optional(&pdev->dev, "vsram");
	if (IS_ERR(apusys->reg_vsram)) {
		apusys->reg_vsram = NULL;
		return -EPROBE_DEFER;
	}

	apusys->infracfg = syscon_regmap_lookup_by_phandle(np, "mediatek,scpsys");
	if (IS_ERR(apusys->infracfg)) {
		dev_dbg(dev, "mediatek,scpsys available\n");
		return PTR_ERR(apusys->infracfg);
	}

	ret  = apusys_syscon_regmap(&apusys->rpc, dev, "mediatek,apu_rpc");
	if (ret)
		return ret;

	ret  = apusys_syscon_regmap(&apusys->conn, dev, "mediatek,apu_conn");
	if (ret)
		return ret;

	ret = apusys_syscon_regmap(&apusys->conn1, dev, "mediatek,apu_conn1");
	if (ret)
		return ret;

	ret = apusys_syscon_regmap(&apusys->vcore, dev, "mediatek,apu_vcore");
	if (ret)
		return ret;

	ret = apusys_syscon_regmap(&apusys->acc, dev, "mediatek,apu_acc");
	if (ret)
		return ret;

	ret = apusys_syscon_regmap(&apusys->ao_ctrl, dev, "mediatek,apu_ao_ctrl");
	if (ret)
		return ret;

	for_each_available_child_of_node(np, node) {
		struct generic_pm_domain *domain;

		domain = apusys_add_one_domain(apusys, node);
		if (IS_ERR(domain)) {
			ret = PTR_ERR(domain);
			of_node_put(node);
			goto err_cleanup_domains;
		}
	}

	ret = of_genpd_add_provider_onecell(np, &apusys->pd_data);
	if (ret) {
		dev_dbg(dev, "failed to add provider: %d\n", ret);
		goto err_cleanup_domains;
	}

	pd_data = &apusys->pd_data;
	for (i = 0; i < data->num_subdomains; i++) {
		const struct apusys_subdomain *sd = &data->subdomains[i];

		ret = pm_genpd_add_subdomain(pd_data->domains[sd->origin],
					     pd_data->domains[sd->subdomain]);
		if (ret && IS_ENABLED(CONFIG_PM))
			dev_dbg(&pdev->dev, "Failed to add subdomain: %d\n",
				ret);
	}

	return 0;

err_cleanup_domains:
	dev_dbg(dev, "err_cleanup_domains: %d\n", ret);
	apusys_domain_cleanup(apusys);
	return ret;
}

static struct platform_driver apusys_pm_domain_driver = {
	.probe = apusys_probe,
	.driver = {
		.name = "apusys-power-controller",
		.suppress_bind_attrs = true,
		.of_match_table = apusys_of_match,
	},
};

module_platform_driver(apusys_pm_domain_driver);
MODULE_LICENSE("GPL");
