// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Chun-Jie Chen <chun-jie.chen@mediatek.com>
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/module.h>
#include "clk-mtk.h"

#define REG_CON0		0
#define REG_CON1		4

#define CON0_BASE_EN		BIT(0)
#define CON0_PWR_ON		BIT(0)
#define CON0_ISO_EN		BIT(1)
#define PCW_CHG_MASK		BIT(31)

#define AUDPLL_TUNER_EN		BIT(31)

#define POSTDIV_MASK		0x7

/* default 7 bits integer, can be overridden with pcwibits. */
#define INTEGER_BITS		7

/*
 * MediaTek PLLs are configured through their pcw value. The pcw value describes
 * a divider in the PLL feedback loop which consists of 7 bits for the integer
 * part and the remaining bits (if present) for the fractional part. Also they
 * have a 3 bit power-of-two post divider.
 */

bool (*mtk_fh_set_rate)(const char *name, unsigned long dds, int postdiv) = NULL;
EXPORT_SYMBOL(mtk_fh_set_rate);

struct mtk_clk_dummy_pll {
	struct clk_hw   hw;
	u8 is_prepared;
	unsigned long rate;
};

static inline struct mtk_clk_dummy_pll *to_mtk_clk_dummy_pll(struct clk_hw *hw)
{
	return container_of(hw, struct mtk_clk_dummy_pll, hw);
}

static void mtk_pll_unprepare(struct clk_hw *hw)
{
	struct mtk_clk_dummy_pll *pll = to_mtk_clk_dummy_pll(hw);

	pll->is_prepared = 0;
}

static int mtk_pll_prepare(struct clk_hw *hw)
{
	struct mtk_clk_dummy_pll *pll = to_mtk_clk_dummy_pll(hw);

	pll->is_prepared = 1;

	return 0;
}

static int mtk_pll_is_prepared(struct clk_hw *hw)
{
	struct mtk_clk_dummy_pll *pll = to_mtk_clk_dummy_pll(hw);

	return pll->is_prepared;
}

static unsigned long mtk_pll_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct mtk_clk_dummy_pll *pll = to_mtk_clk_dummy_pll(hw);

	if (pll->rate)
		return pll->rate;
	else
		return parent_rate;
}

static int mtk_pll_set_rate(struct clk_hw *hw, unsigned long rate,
			    unsigned long parent_rate)
{
	struct mtk_clk_dummy_pll *pll = to_mtk_clk_dummy_pll(hw);

	pll->rate = rate;

	return 0;
}

static long mtk_pll_round_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long *prate)
{
	return rate;
}

static const struct clk_ops mtk_pll_ops = {
	.is_prepared	= mtk_pll_is_prepared,
	.prepare	= mtk_pll_prepare,
	.unprepare	= mtk_pll_unprepare,
	.recalc_rate	= mtk_pll_recalc_rate,
	.round_rate	= mtk_pll_round_rate,
	.set_rate	= mtk_pll_set_rate,
};

static struct clk *mtk_clk_register_pll(const struct mtk_pll_data *data,
					void __iomem *base)
{
	struct mtk_clk_dummy_pll *pll;
	struct clk_init_data init = {};
	struct clk *clk;
	const char *parent_name = "clk26m";

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	pll->hw.init = &init;
	pll->is_prepared = 0;
	pll->rate = 26000000;

	init.name = data->name;
	init.flags = (data->flags & PLL_AO) ? CLK_IS_CRITICAL : 0;
	init.ops = &mtk_pll_ops;
	if (data->parent_name)
		init.parent_names = &data->parent_name;
	else
		init.parent_names = &parent_name;
	init.num_parents = 1;

	clk = clk_register(NULL, &pll->hw);

	if (IS_ERR(clk))
		kfree(pll);

	return clk;
}

void mtk_clk_register_plls(struct device_node *node,
			   const struct mtk_pll_data *plls, int num_plls,
			   struct clk_onecell_data *clk_data)
{
	void __iomem *base;
	int i;
	struct clk *clk;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	for (i = 0; i < num_plls; i++) {
		const struct mtk_pll_data *pll = &plls[i];

		clk = mtk_clk_register_pll(pll, base);

		if (IS_ERR(clk)) {
			pr_err("Failed to register clk %s: %ld\n",
					pll->name, PTR_ERR(clk));
			continue;
		}

		clk_data->clks[pll->id] = clk;
	}
}
EXPORT_SYMBOL(mtk_clk_register_plls);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Dummy PLL");
MODULE_AUTHOR("MediaTek Inc.");
