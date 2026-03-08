/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_MDP_RDMA_H__
#define __MTK_MDP_RDMA_H__

#define ENABLE_PM_FLOW		0 // TODO: need to check pm flow
#define ENABLE_CLK_FLOW		0 // TODO: need to check clk flow
#define ENABLE_IRQ_HANDLER	0 // TODO: need to check irq handler

enum rdma_format {
	RDMA_INPUT_FORMAT_RGB565 = 0,
	RDMA_INPUT_FORMAT_RGB888 = 1,
	RDMA_INPUT_FORMAT_RGBA8888 = 2,
	RDMA_INPUT_FORMAT_ARGB8888 = 3,
	RDMA_INPUT_FORMAT_UYVY = 4,
	RDMA_INPUT_FORMAT_YUY2 = 5,
	RDMA_INPUT_FORMAT_Y8 = 7,
	RDMA_INPUT_FORMAT_YV12 = 8,
	RDMA_INPUT_FORMAT_UYVY_3PL = 9,
	RDMA_INPUT_FORMAT_NV12 = 12,
	RDMA_INPUT_FORMAT_UYVY_2PL = 13,
	RDMA_INPUT_FORMAT_Y410 = 14
};

#define RDMA_INPUT_SWAP		(1 << 14)
#define RDMA_INPUT_10BIT    (1 << 18)

enum rdma_profile {
	RDMA_CSC_RGB_TO_JPEG = 0,
	RDMA_CSC_RGB_TO_FULL709 = 1,
	RDMA_CSC_RGB_TO_BT601 = 2,
	RDMA_CSC_RGB_TO_BT709 = 3,
	RDMA_CSC_JPEG_TO_RGB = 4,
	RDMA_CSC_FULL709_TO_RGB = 5,
	RDMA_CSC_BT601_TO_RGB = 6,
	RDMA_CSC_BT709_TO_RGB = 7,
	RDMA_CSC_JPEG_TO_BT601 = 8,
	RDMA_CSC_JPEG_TO_BT709 = 9,
	RDMA_CSC_BT601_TO_JPEG = 10,
	RDMA_CSC_BT709_TO_BT601 = 11,
	RDMA_CSC_BT601_TO_BT709 = 12
};

enum rdma_encode {
	RDMA_ENCODE_NONE = 0,
	RDMA_ENCODE_AFBC = 1,
	RDMA_ENCODE_HYFBC = 2,
	RDMA_ENCODE_UFO_DCP = 3
};

enum rdma_block {
	RDMA_BLOCK_NONE = 0,
	RDMA_BLOCK_8x8 = 1,
	RDMA_BLOCK_8x16 = 2,
	RDMA_BLOCK_8x32 = 3,
	RDMA_BLOCK_16x8 = 4,
	RDMA_BLOCK_16x16 = 5,
	RDMA_BLOCK_16x32 = 6,
	RDMA_BLOCK_32x8 = 7,
	RDMA_BLOCK_32x16 = 8,
	RDMA_BLOCK_32x32 = 9
};

struct mtk_mdp_rdma_cfg {
	unsigned int addr0; /* Y plane */
	unsigned int addr1; /* U plane */
	unsigned int addr2; /* V plane */
	unsigned int source_width;  /* source original tile */
	unsigned int source_size;  /* source buffer size */
	unsigned int width;  /* tile */
	unsigned int height; /* tile */
	unsigned int x_left; /* tile */
	unsigned int y_top; /* tile */
	enum rdma_format fmt; /* input format */
	bool csc_enable;
	enum rdma_profile profile; /* csc */
	enum rdma_encode encode_type; /* compression */
	enum rdma_block block_size; /* block mode */
};

void mtk_mdp_rdma_start(void __iomem *base,
	resource_size_t regs_pa,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base);

void mtk_mdp_rdma_stop(void __iomem *base,
	resource_size_t regs_pa,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base);

void mtk_mdp_rdma_golden_setting(void __iomem *base,
	resource_size_t regs_pa,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base);

void mtk_mdp_rdma_config(void __iomem *base,
	resource_size_t regs_pa,
	struct mtk_mdp_rdma_cfg *cfg,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base,
	bool sec_on);

int mtk_mdp_rdma_dump(void __iomem *base);

bool mtk_mdp_rdma_is_enable(void __iomem *base);

unsigned int mtk_mdp_rdma_get_addr(void __iomem *base);

void mtk_mdp_rdma_set_addr(void __iomem *base,
	resource_size_t regs_pa,
	struct cmdq_pkt *handle,
	struct cmdq_base *cmdq_base,
	unsigned int rdma_addr);

#if defined(ENABLE_CLK_FLOW)
void mtk_mdp_rdma_prepare(void __iomem *base, bool shadow);

void mtk_mdp_rdma_unprepare(void __iomem *base);
#endif

#endif // __MTK_MDP_RDMA_H__

