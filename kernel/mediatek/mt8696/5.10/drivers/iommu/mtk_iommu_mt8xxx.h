/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
 */

#ifndef _MTK_IOMMU_H_
#define _MTK_IOMMU_H_

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/io-pgtable.h>
#include <linux/iommu.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <soc/mediatek/smi.h>
#include <dt-bindings/memory/mtk-memory-port.h>

#define MTK_LARB_COM_MAX	8
#define MTK_LARB_SUBCOM_MAX	8

#define MTK_IOMMU_GROUP_MAX	12

#define MTK_IOMMU_BANK_MAX	5

struct mtk_iommu_suspend_reg {
	union {
		u32			standard_axi_mode;/* v1 */
		u32			misc_ctrl;/* v2 */
	};
	u32				dcm_dis;
	u32				ctrl_reg;
	u32				vld_pa_rng;
	u32				wr_len_ctrl;
	union {
		struct { /* only for gen1 */
			u32		int_control0;
		};

		struct { /* only for gen2 that support multi-banks */
			u32		int_control[MTK_IOMMU_BANK_MAX];
			u32		int_main_control[MTK_IOMMU_BANK_MAX];
			u32		ivrp_paddr[MTK_IOMMU_BANK_MAX];
		};
	};
};

enum mtk_iommu_plat {
	M4U_MT2701,
	M4U_MT2712,
	M4U_MT6779,
	M4U_MT8167,
	M4U_MT8173,
	M4U_MT8183,
	M4U_MT8188,
	M4U_MT8192,
	M4U_MT8195,
	M4U_MT8696
};

struct mtk_iommu_iova_region;

struct mtk_iommu_plat_data {
	enum mtk_iommu_plat m4u_plat;
	u32                 flags;
	u32                 inv_sel_reg;

	char					*pericfg_comp_str;
	struct list_head			*hw_list;
	unsigned int				iova_region_nr;
	const struct mtk_iommu_iova_region	*iova_region;

	/* reserve/dir-mapping iova region data */
	const unsigned int			resv_cnt;
	const struct mtk_iommu_iova_region	*resv_region;

	/* bank_info:
	 *   bit[7:0]: bank_nr
	 *   bit[15:8]: used_bank_bitmap
	 *     the order of interrupts at dts node must be consistent with here.
	 *   bit[23:16]: normal_bank_bitmap
	 *     the order of reg at dts node must be consistent with here.
	 *   bit[31:24]: base bank id
	 */
	unsigned int        bank_info;
	unsigned int        bank_portmsk[MTK_IOMMU_BANK_MAX];
	unsigned char       larbid_remap[MTK_LARB_COM_MAX][MTK_LARB_SUBCOM_MAX];
};

struct mtk_iommu_domain;

struct mtk_iommu_bank_data {
	void __iomem			*base;
	int				irq;
	unsigned int			id;
	struct device			*pdev;
	struct mtk_iommu_data		*pdata;   /* parent data */
	spinlock_t			tlb_lock; /* lock for tlb range flush */
	struct mtk_iommu_domain		*m4u_dom; /* Each bank has a domain */
};

struct mtk_iommu_data {
	struct mtk_iommu_bank_data	bank[MTK_IOMMU_BANK_MAX];
	struct device			*dev;
	struct clk			*bclk;
	struct clk			*apb;
	phys_addr_t			protect_base; /* protect memory base */
	struct mtk_iommu_suspend_reg	reg;
#ifdef CONFIG_ARM64
	struct iommu_group		*m4u_group[MTK_IOMMU_GROUP_MAX];
#else
	struct dma_iommu_mapping	*mapping[MTK_IOMMU_GROUP_MAX];
#endif
	struct device			*resv_iova_dev[MTK_IOMMU_GROUP_MAX];
	bool                            enable_4GB;

	struct iommu_device		iommu;
	const struct mtk_iommu_plat_data *plat_data;
	struct device			*smicomm_dev;

	struct regmap			*pericfg;

	/*
	 * In the sharing pgtable case, list data->list to the global list like m4ulist.
	 * In the non-sharing pgtable case, list data->list to the itself hw_list_head.
	 */
	struct list_head		*hw_list;
	struct list_head		hw_list_head;
	struct list_head		list;
	struct mtk_smi_larb_iommu	larb_imu[MTK_LARB_NR_MAX];
};

static inline int compare_of(struct device *dev, void *data)
{
	return dev->of_node == data;
}

static inline void release_of(struct device *dev, void *data)
{
	of_node_put(data);
}

static inline int mtk_iommu_bind(struct device *dev)
{
	struct mtk_iommu_data *data = dev_get_drvdata(dev);

	return component_bind_all(dev, &data->larb_imu);
}

static inline void mtk_iommu_unbind(struct device *dev)
{
	struct mtk_iommu_data *data = dev_get_drvdata(dev);

	component_unbind_all(dev, &data->larb_imu);
}

#ifdef CONFIG_MTK_FB
extern phys_addr_t mtkfb_get_fb_base(void);
extern size_t mtkfb_get_fb_size(void);
#endif

#endif
