// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Chun-Jie Chen <chun-jie.chen@mediatek.com>
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>

#include "clk-mtk.h"
#include "clk-mux.h"

struct mtk_clk_dummy_mux {
	struct clk_hw hw;
	spinlock_t *lock;
	u8 is_enable;
	u8 select;
};

static inline struct mtk_clk_dummy_mux *to_mtk_clk_dummy_mux(struct clk_hw *hw)
{
	return container_of(hw, struct mtk_clk_dummy_mux, hw);
}

static int mtk_clk_mux_enable(struct clk_hw *hw)
{
	struct mtk_clk_dummy_mux *mux = to_mtk_clk_dummy_mux(hw);

	mux->is_enable = 1;

	return 0;
}

static void mtk_clk_mux_disable(struct clk_hw *hw)
{
	struct mtk_clk_dummy_mux *mux = to_mtk_clk_dummy_mux(hw);

	mux->is_enable = 0;
}

static int mtk_clk_mux_enable_setclr(struct clk_hw *hw)
{
	struct mtk_clk_dummy_mux *mux = to_mtk_clk_dummy_mux(hw);

	mux->is_enable = 1;

	return 0;
}

static void mtk_clk_mux_disable_setclr(struct clk_hw *hw)
{
	struct mtk_clk_dummy_mux *mux = to_mtk_clk_dummy_mux(hw);

	mux->is_enable = 0;
}

static int mtk_clk_mux_is_enabled(struct clk_hw *hw)
{
	struct mtk_clk_dummy_mux *mux = to_mtk_clk_dummy_mux(hw);

	return mux->is_enable;
}


static u8 mtk_clk_mux_get_parent(struct clk_hw *hw)
{
	struct mtk_clk_dummy_mux *mux = to_mtk_clk_dummy_mux(hw);

	return mux->select;
}


static int mtk_clk_mux_set_parent_lock(struct clk_hw *hw, u8 index)
{
	struct mtk_clk_dummy_mux *mux = to_mtk_clk_dummy_mux(hw);
	unsigned long flags = 0;

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);
	else
		__acquire(mux->lock);

	mux->select = index;

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);
	else
		__release(mux->lock);

	return 0;
}


static int mtk_clk_mux_set_parent_setclr_lock(struct clk_hw *hw, u8 index)
{
	struct mtk_clk_dummy_mux *mux = to_mtk_clk_dummy_mux(hw);
	unsigned long flags = 0;

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);
	else
		__acquire(mux->lock);

	mux->select = index;

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);
	else
		__release(mux->lock);

	return 0;
}

const struct clk_ops mtk_mux_ops = {
	.get_parent = mtk_clk_mux_get_parent,
	.set_parent = mtk_clk_mux_set_parent_lock,
};
EXPORT_SYMBOL(mtk_mux_ops);

const struct clk_ops mtk_mux_clr_set_upd_ops = {
	.get_parent = mtk_clk_mux_get_parent,
	.set_parent = mtk_clk_mux_set_parent_setclr_lock,
};
EXPORT_SYMBOL(mtk_mux_clr_set_upd_ops);

const struct clk_ops mtk_mux_gate_ops = {
	.enable = mtk_clk_mux_enable,
	.disable = mtk_clk_mux_disable,
	.is_enabled = mtk_clk_mux_is_enabled,
	.get_parent = mtk_clk_mux_get_parent,
	.set_parent = mtk_clk_mux_set_parent_lock,
};
EXPORT_SYMBOL(mtk_mux_gate_ops);

const struct clk_ops mtk_mux_gate_clr_set_upd_ops = {
	.enable = mtk_clk_mux_enable_setclr,
	.disable = mtk_clk_mux_disable_setclr,
	.is_enabled = mtk_clk_mux_is_enabled,
	.get_parent = mtk_clk_mux_get_parent,
	.set_parent = mtk_clk_mux_set_parent_setclr_lock,
};
EXPORT_SYMBOL(mtk_mux_gate_clr_set_upd_ops);

struct clk *mtk_clk_register_mux(const struct mtk_mux *mux,
				 struct regmap *regmap,
				 spinlock_t *lock)
{
	struct mtk_clk_dummy_mux *clk_mux;
	struct clk_init_data init = {};
	struct clk *clk;

	clk_mux = kzalloc(sizeof(*clk_mux), GFP_KERNEL);
	if (!clk_mux)
		return ERR_PTR(-ENOMEM);

	init.name = mux->name;
	init.flags = mux->flags | CLK_SET_RATE_PARENT;
	init.parent_names = mux->parent_names;
	init.num_parents = mux->num_parents;
	init.ops = mux->ops;

	clk_mux->lock = lock;
	clk_mux->is_enable = 0;
	clk_mux->select = 0;
	clk_mux->hw.init = &init;

	clk = clk_register(NULL, &clk_mux->hw);
	if (IS_ERR(clk)) {
		kfree(clk_mux);
		return clk;
	}

	return clk;
}
EXPORT_SYMBOL(mtk_clk_register_mux);

int mtk_clk_register_muxes(const struct mtk_mux *muxes,
			   int num, struct device_node *node,
			   spinlock_t *lock,
			   struct clk_onecell_data *clk_data)
{
	struct regmap *regmap;
	struct clk *clk;
	int i;

	regmap = device_node_to_regmap(node);
	if (IS_ERR(regmap)) {
		pr_err("Cannot find regmap for %pOF: %ld\n", node,
		       PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	for (i = 0; i < num; i++) {
		const struct mtk_mux *mux = &muxes[i];

		if (IS_ERR_OR_NULL(clk_data->clks[mux->id])) {
			clk = mtk_clk_register_mux(mux, regmap, lock);

			if (IS_ERR(clk)) {
				pr_err("Failed to register clk %s: %ld\n",
				       mux->name, PTR_ERR(clk));
				continue;
			}

			clk_data->clks[mux->id] = clk;
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_clk_register_muxes);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Dummy MUX");
MODULE_AUTHOR("MediaTek Inc.");
