// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include <dt-bindings/interconnect/mtk,mmqos.h>

#include <drm/drm_fourcc.h>

#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_drv.h"
#include "mtk_log.h"
#include "mtk_dump.h"
#include "mtk_mdp_rdma.h"
#include "mtk_ethdr.h"
#include "mtk_drm_fb.h"
#include "mtk-interconnect.h"

#include "vdo1/8188/mtk_vdo1_comp.h"

#define DISP_MERGE_ENABLE	0x000
#define DISP_MERGE_RESET	0x004
#define DISP_MERGE_CFG_0	0x010
#define DISP_MERGE_CFG_1	0x014
#define DISP_MERGE_CFG_4	0x020
#define DISP_MERGE_CFG_5	0x024
#define DISP_MERGE_CFG_8	0x030
#define DISP_MERGE_CFG_9	0x034
#define DISP_MERGE_CFG_10	0x038
	#define CFG_10_NO_SWAP 0
	#define CFG_10_RG_SWAP 1
	#define CFG_10_GB_SWAP 2
	#define CFG_10_BR_SWAP 4
	#define CFG_10_RG_GB_SWAP 8
	#define CFG_10_RB_GB_SWAP 10
#define DISP_MERGE_CFG_11	0x03C
#define DISP_MERGE_CFG_12	0x040
	#define CFG12_10_10_1PI_2PO_BUF_MODE 6
	#define CFG12_11_10_1PI_2PO_MERGE 18
#define DISP_MERGE_CFG_13	0x044
#define DISP_MERGE_CFG_14	0x048
#define DISP_MERGE_CFG_15	0x04C
#define DISP_MERGE_CFG_17	0x054
#define DISP_MERGE_CFG_18	0x058
#define DISP_MERGE_CFG_19	0x05C
#define DISP_MERGE_CFG_20	0x060
#define DISP_MERGE_CFG_21	0x064
#define DISP_MERGE_CFG_22	0x068
#define DISP_MERGE_CFG_23	0x06C
#define DISP_MERGE_CFG_24	0x070
#define DISP_MERGE_CFG_25	0x074
#define DISP_MERGE_CFG_26	0x078
#define DISP_MERGE_CFG_27	0x07C
#define DISP_MERGE_CFG_28	0x080
#define DISP_MERGE_CFG_29	0x084
#define DISP_MERGE_PATG_0	0x200
#define DISP_MERGE_PATG_5	0x214
#define DISP_MERGE_PATG_9	0x224
#define DISP_MERGE_PATG_13	0x234
#define DISP_MERGE_PATG_17	0x244
#define DISP_MERGE_PATG_33	0x284
#define DISP_MERGE_PATG_37	0x294
#define DISP_MERGE_PATG_41	0x2A4
#define DISP_MERGE_PATG_45	0x2B4
#define DISP_MERGE_MUTE_0	0xF00

/*VDO1 CONFIG*/
#define VDO1_CONFIG_SW0_RST_B 0x1D0
#define VDO1_CONFIG_MERGE0_ASYNC_CFG_WD 0xE30
#define VDO1_CONFIG_MERGE1_ASYNC_CFG_WD 0xE40
#define VDO1_CONFIG_MERGE2_ASYNC_CFG_WD 0xE50
#define VDO1_CONFIG_MERGE3_ASYNC_CFG_WD 0xE60
#define VDO1_CONFIG_VPP2_ASYNC_CFG_WD 0XEC0
#define VDO1_CONFIG_VPP3_ASYNC_CFG_WD 0XED0
#define VDO1_HDR_TOP_CFG 0xD00
	#define VDO1_HDR_TOP_CFG_HDR_ALPHA_SEL_MIXER_IN1 20
	#define VDO1_HDR_TOP_CFG_HDR_ALPHA_SEL_MIXER_IN2 21
	#define VDO1_HDR_TOP_CFG_HDR_ALPHA_SEL_MIXER_IN3 22
	#define VDO1_HDR_TOP_CFG_HDR_ALPHA_SEL_MIXER_IN4 23
#define VDO1_CONFIG_MIXER_IN1_ALPHA 0xD30
#define VDO1_CONFIG_MIXER_IN2_ALPHA 0xD34
#define VDO1_CONFIG_MIXER_IN3_ALPHA 0xD38
#define VDO1_CONFIG_MIXER_IN4_ALPHA 0xD3C

#define MTK_PSEUDO_OVL_LAYER_NUM 4
#define MTK_PSEUDO_OVL_SINGLE_PIPE_MAX_WIDTH 1920
#define VDO1_CONFIG_ALPHA_DEFAULT_VALUE 0x0100
#define MERGE_NUM (PSEUDO_OVL_DISP_MERGE3-PSEUDO_OVL_DISP_MERGE0+1)
#define RDMA_NUM (PSEUDO_OVL_MDP_RDMA7-PSEUDO_OVL_MDP_RDMA0+1)
#define MTK_PSEUDO_OVL_SEC_PORT_L0 (1UL << 59)
#define MTK_PSEUDO_OVL_SEC_PORT_L1 (1UL << 60)

#define MTK_PSEUDO_OVL_PRIMARY_LAYER 2

struct mtk_disp_pseudo_ovl_data {
	bool tbd; //no use
};

struct pseudo_ovl_larb_data {
	unsigned int	larb_id;
	struct device	*dev;
};

struct mtk_disp_pseudo_ovl_config_struct {
	unsigned int fmt;
	unsigned int merge_mode;
	unsigned int in_w[2];
	unsigned int out_w[2];
	unsigned int in_h;
	unsigned int out_swap;
	unsigned int spilt_ofs[2];
};

enum mtk_pseudo_ovl_larb {
	PSEUDO_OVL_LARB2,
	PSEUDO_OVL_LARB3,
	PSEUDO_OVL_LARB_NUM
};

enum mtk_pseudo_ovl_comp_id {
	PSEUDO_OVL_MDP_RDMA0,
	PSEUDO_OVL_MDP_RDMA1,
	PSEUDO_OVL_MDP_RDMA2,
	PSEUDO_OVL_MDP_RDMA3,
	PSEUDO_OVL_MDP_RDMA4,
	PSEUDO_OVL_MDP_RDMA5,
	PSEUDO_OVL_MDP_RDMA6,
	PSEUDO_OVL_MDP_RDMA7,
	PSEUDO_OVL_DISP_MERGE0,
	PSEUDO_OVL_DISP_MERGE1,
	PSEUDO_OVL_DISP_MERGE2,
	PSEUDO_OVL_DISP_MERGE3,
	PSEUDO_OVL_PAD0,
	PSEUDO_OVL_PAD1,
	PSEUDO_OVL_PAD2,
	PSEUDO_OVL_PAD3,
	PSEUDO_OVL_PAD4,
	PSEUDO_OVL_PAD5,
	PSEUDO_OVL_PAD6,
	PSEUDO_OVL_PAD7,
	PSEUDO_OVL_RSZ1,
	PSEUDO_OVL_RSZ2,
	PSEUDO_OVL_RSZ3,
	PSEUDO_OVL_RSZ4,
	PSEUDO_OVL_ID_MAX
};

enum mtk_pseudo_ovl_index {
	MTK_PSEUDO_OVL_RDMA_BASE = 0,
	MTK_PSEUDO_OVL_L0_LEFT = MTK_PSEUDO_OVL_RDMA_BASE,
	MTK_PSEUDO_OVL_L0_RIGHT,
	MTK_PSEUDO_OVL_L1_LEFT,
	MTK_PSEUDO_OVL_L1_RIGHT,
	MTK_PSEUDO_OVL_L2_LEFT,
	MTK_PSEUDO_OVL_L2_RIGHT,
	MTK_PSEUDO_OVL_L3_LEFT,
	MTK_PSEUDO_OVL_L3_RIGHT,
	MTK_PSEUDO_OVL_MERGE_BASE = PSEUDO_OVL_DISP_MERGE0,
	MTK_PSEUDO_OVL_L0_MERGE = MTK_PSEUDO_OVL_MERGE_BASE,
	MTK_PSEUDO_OVL_L1_MERGE,
	MTK_PSEUDO_OVL_L2_MERGE,
	MTK_PSEUDO_OVL_L3_MERGE
};

static const char * const pseudo_ovl_comp_str[] = {
	"PSEUDO_OVL_MDP_RDMA0",
	"PSEUDO_OVL_MDP_RDMA1",
	"PSEUDO_OVL_MDP_RDMA2",
	"PSEUDO_OVL_MDP_RDMA3",
	"PSEUDO_OVL_MDP_RDMA4",
	"PSEUDO_OVL_MDP_RDMA5",
	"PSEUDO_OVL_MDP_RDMA6",
	"PSEUDO_OVL_MDP_RDMA7",
	"PSEUDO_OVL_DISP_MERGE0",
	"PSEUDO_OVL_DISP_MERGE1",
	"PSEUDO_OVL_DISP_MERGE2",
	"PSEUDO_OVL_DISP_MERGE3",
	"PSEUDO_OVL_PAD0",
	"PSEUDO_OVL_PAD1",
	"PSEUDO_OVL_PAD2",
	"PSEUDO_OVL_PAD3",
	"PSEUDO_OVL_PAD4",
	"PSEUDO_OVL_PAD5",
	"PSEUDO_OVL_PAD6",
	"PSEUDO_OVL_PAD7",
	"PSEUDO_OVL_RSZ1",
	"PSEUDO_OVL_RSZ2",
	"PSEUDO_OVL_RSZ3",
	"PSEUDO_OVL_RSZ4",
	"PSEUDO_OVL_ID_MAX"
};

static unsigned int merge_async_offset[] = {
	VDO1_CONFIG_MERGE0_ASYNC_CFG_WD,
	VDO1_CONFIG_MERGE1_ASYNC_CFG_WD,
	VDO1_CONFIG_MERGE2_ASYNC_CFG_WD,
	VDO1_CONFIG_MERGE3_ASYNC_CFG_WD,
};

static unsigned int merge_async_reset_bit[] = {
	25, 26, 27, 28
};

static unsigned int alpha_source_sel_from_prev_reg[] = {
	VDO1_HDR_TOP_CFG_HDR_ALPHA_SEL_MIXER_IN1,
	VDO1_HDR_TOP_CFG_HDR_ALPHA_SEL_MIXER_IN2,
	VDO1_HDR_TOP_CFG_HDR_ALPHA_SEL_MIXER_IN3,
	VDO1_HDR_TOP_CFG_HDR_ALPHA_SEL_MIXER_IN4,
};

static unsigned int mixer_alpha_value[] = {
	VDO1_CONFIG_MIXER_IN1_ALPHA,
	VDO1_CONFIG_MIXER_IN2_ALPHA,
	VDO1_CONFIG_MIXER_IN3_ALPHA,
	VDO1_CONFIG_MIXER_IN4_ALPHA,
};

static u64 pseudo_ovl_sec_port[] = {
	MTK_PSEUDO_OVL_SEC_PORT_L0,
	MTK_PSEUDO_OVL_SEC_PORT_L1,
	0,
	0,
};

static u32 layer_sec_status;

struct mtk_disp_pseudo_ovl {
	struct mtk_ddp_comp ddp_comp;
	bool rdma_memory_mode;
	struct drm_crtc *crtc;
	struct mtk_ddp_comp pseudo_ovl_comp[PSEUDO_OVL_ID_MAX];
	struct device *larb_dev0;
	u32 larb_id0;
	struct device *larb_dev1;
	u32 larb_id1;
	struct clk *async_clk[MERGE_NUM];
	const struct mtk_disp_pseudo_ovl_data *data;
};

static struct platform_device *pseudo_ovl_dev[PSEUDO_OVL_LARB_NUM] = {NULL};
static struct device *pseudo_ovl_larb_dev[PSEUDO_OVL_LARB_NUM] = {NULL};

static void mtk_pseudo_ovl_layer_off(struct mtk_ddp_comp *comp,
	unsigned int idx, unsigned int ext_idx, struct cmdq_pkt *handle);

static inline struct mtk_disp_pseudo_ovl *comp_to_pseudo_ovl(
	struct mtk_ddp_comp *comp)
{
	return container_of(comp, struct mtk_disp_pseudo_ovl, ddp_comp);
}

static bool is_yuv(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_P010:
		return true;
	default:
		return false;
	}
}

static int mtk_pseudo_ovl_golden_setting(struct mtk_ddp_comp *comp,
				  struct mtk_ddp_config *cfg,
				  struct cmdq_pkt *handle)
{
	struct mtk_ddp_comp *priv_comp = NULL;
	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);
	int i;

	/*config rdma gs*/
	for (i = PSEUDO_OVL_MDP_RDMA0; i <= PSEUDO_OVL_MDP_RDMA7; i++) {
		priv_comp =
			&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+i]);
		mtk_mdp_rdma_golden_setting(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base);
	}

	return 0;
}

static u64 mtk_pseudo_ovl_get_sec_port(struct mtk_ddp_comp *comp)
{
	struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
	struct drm_framebuffer *fb;
	struct drm_plane *plane;
	struct mtk_plane_comp_state comp_state;
	u64 sec_port = 0;
	int i;

	for (i = 0; i < mtk_crtc->layer_nr; i++) {
		plane = &mtk_crtc->planes[i].base;
		fb = plane->state->fb;

		if (mtk_drm_fb_is_secure(fb)) {
			mtk_plane_get_comp_state(plane, &comp_state,
				&(mtk_crtc->base), 0);
			sec_port |= pseudo_ovl_sec_port[comp_state.lye_id];
			DDPDBG("%s fb is secure for layer %d(%d)\n", __func__, i,
				comp_state.lye_id);
		}
	}

	DDPDBG("%s get secure port:0x%lx\n", __func__, sec_port);
	return sec_port;
}

static u64 mtk_pseudo_ovl_get_disable_sec_port(struct mtk_ddp_comp *comp,
	struct cmdq_pkt *handle)
{
	struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
	struct drm_framebuffer *fb;
	struct drm_plane *plane;
	struct mtk_plane_state *plane_state;
	u64 sec_port = 0;
	int i;

	for (i = 0; i < mtk_crtc->layer_nr; i++) {
		plane = &mtk_crtc->planes[i].base;
		plane_state = to_mtk_plane_state(plane->state);
		fb = plane->state->fb;
		if (!mtk_drm_fb_is_secure(fb) &&
			(layer_sec_status & (1<<plane_state->comp_state.lye_id))) {
			sec_port |= pseudo_ovl_sec_port[plane_state->comp_state.lye_id];
			mtk_pseudo_ovl_layer_off(comp, plane_state->comp_state.lye_id,
				0, handle);
		}
	}

	DDPDBG("%s get disable secure port:0x%lx\n", __func__, sec_port);
	return sec_port;
}

static int mtk_pseudo_ovl_replace_bootup_mva(struct mtk_ddp_comp *comp,
				      struct cmdq_pkt *handle, void *params,
				      struct mtk_ddp_fb_info *fb_info)
{
	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);
	unsigned int layer_addr, layer_mva;
	struct mtk_ddp_comp *priv_comp = NULL;
	bool enable = false;
	int i;
	struct mtk_ddp_comp fake_comp = {0};

	DDPINFO("%s", __func__);

	for (i = 0 ; i < MTK_PSEUDO_OVL_LAYER_NUM; i++) {
		priv_comp =
			&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*i]);
		enable = mtk_mdp_rdma_is_enable(priv_comp->regs);
		if (enable) {
			layer_addr = mtk_mdp_rdma_get_addr(priv_comp->regs);
			break;
		}
	}

	if (enable) {
		layer_mva = layer_addr - fb_info->fb_pa + fb_info->fb_gem->dma_addr;
		DDPINFO("%s replace layer %d pa:0x%x with mva: 0x%x, size:0x%x",
			__func__, i, fb_info->fb_pa, layer_mva, fb_info->fb_gem->size);

		fake_comp.id = DDP_COMPONENT_ID_MAX;
		fake_comp.dev = &pseudo_ovl_dev[PSEUDO_OVL_LARB2]->dev;
		fake_comp.larb_dev = pseudo_ovl_larb_dev[PSEUDO_OVL_LARB2];
		mtk_ddp_comp_iommu_enable(&fake_comp, handle);

		fake_comp.dev = &pseudo_ovl_dev[PSEUDO_OVL_LARB3]->dev;
		fake_comp.larb_dev = pseudo_ovl_larb_dev[PSEUDO_OVL_LARB3];
		mtk_ddp_comp_iommu_enable(&fake_comp, handle);

		mtk_mdp_rdma_set_addr(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base, layer_mva);
		priv_comp =
			&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*i+1]);
		mtk_mdp_rdma_set_addr(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base, layer_mva);
	}

	return 0;
}

static void mtk_pseudo_ovl_all_layer_off(struct mtk_ddp_comp *comp,
	struct cmdq_pkt *handle, int keep_first_layer)
{
	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);
	struct mtk_ddp_comp *priv_comp;
	int i = 0;
	int j;

	if (keep_first_layer) {
		for (i = 0 ; i < MTK_PSEUDO_OVL_LAYER_NUM; i++) {
			priv_comp =
				&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*i]);
			if (mtk_mdp_rdma_is_enable(priv_comp->regs))
				break;
		}
	}

	for (j = ++i; j < MTK_PSEUDO_OVL_LAYER_NUM; j++)
		mtk_pseudo_ovl_layer_off(comp, j, LYE_NORMAL, handle);
}

static int mtk_pseudo_ovl_io_cmd(struct mtk_ddp_comp *comp,
			struct cmdq_pkt *handle,
			enum mtk_ddp_io_cmd io_cmd, void *params)
{
	int ret = 0;

	switch (io_cmd) {
	case SECURE_PORT_SETTING_ENABLE: {
		u64 *sec_port;

		sec_port = (u64 *)params;
		*sec_port = mtk_pseudo_ovl_get_sec_port(comp);
		break;
	}
	case SECURE_PORT_SETTING_DISABLE: {
		u64 *sec_port;

		sec_port = (u64 *)params;
		*sec_port = mtk_pseudo_ovl_get_disable_sec_port(comp, handle);
		break;
	}
	case OVL_REPLACE_BOOTUP_MVA: {
		struct mtk_ddp_fb_info *fb_info =
			(struct mtk_ddp_fb_info *)params;

		mtk_pseudo_ovl_replace_bootup_mva(comp, handle, params, fb_info);
		break;
	}
	case OVL_ALL_LAYER_OFF:
	{
		int *keep_first_layer = params;

		mtk_pseudo_ovl_all_layer_off(comp, handle, *keep_first_layer);
		break;
	}
	case OVL_GET_PRIMARY_LAYER:
	{
		int *primary_layer = params;

		*primary_layer = MTK_PSEUDO_OVL_PRIMARY_LAYER;
		break;
	}
#ifdef MTK_DISP_MMQOS_SUPPORT
	case PMQOS_SET_BW: {
		/*Set BW*/
		struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);
		u32 bw_val = 0;
		int i;

		for (i = 0; i < RDMA_NUM; i++) {
			bw_val = pseudo_ovl->pseudo_ovl_comp[i].qos_bw;
			__mtk_disp_set_module_bw(pseudo_ovl->pseudo_ovl_comp[i].qos_req,
				comp->id, bw_val, DISP_BW_NORMAL_MODE);
		}
		break;
	}
#endif
	default:
		break;
	}

	return ret;
}

static void mtk_pseudo_ovl_merge_config(struct mtk_ddp_comp *comp,
		struct mtk_disp_pseudo_ovl_config_struct *pseudo_ovl_cfg,
		struct cmdq_pkt *handle)
{
	/* 8'd00 : Use external configure
	 * 8'd01 : CFG_11_11_1PI_1PO_BYPASS
	 * 8'd02 : CFG_11_11_2PI_2PO_BYPASS
	 * 8'd03 : CFG_10_10_2PI_1PO_BYPASS
	 * 8'd04 : CFG_10_10_2PI_2PO_BYPASS
	 * 8'd05 : CFG_10_10_1PI_1PO_BUF_MODE
	 * 8'd06 : CFG_10_10_1PI_2PO_BUF_MODE
	 * 8'd07 : CFG_10_10_2PI_1PO_BUF_MODE
	 * 8'd08 : CFG_10_10_2PI_2PO_BUF_MODE
	 * 8'd09 : CFG_10_01_1PI_1PO_BUF_MODE
	 * 8'd10 : CFG_10_01_2PI_1PO_BUF_MODE
	 * 8'd11 : CFG_01_10_1PI_1PO_BUF_MODE
	 * 8'd12 : CFG_01_10_1PI_2PO_BUF_MODE
	 * 8'd13 : CFG_01_01_1PI_1PO_BUF_MODE
	 * 8'd14 : CFG_10_11_1PI_1PO_SPLIT
	 * 8'd15 : CFG_10_11_2PI_1PO_SPLIT
	 * 8'd16 : CFG_01_11_1PI_1PO_SPLIT
	 * 8'd17 : CFG_11_10_1PI_1PO_MERGE
	 * 8'd18 : CFG_11_10_1PI_2PO_MERGE
	 * 8'd19 : CFG_10_10_1PI_1PO_TO422
	 * 8'd20 : CFG_10_10_1PI_2PO_TO444
	 * 8'd21 : CFG_10_10_2PI_2PO_TO444
	 * else  : CFG_11_11_1PI_1PO_BYPASS
	 */
	switch (pseudo_ovl_cfg->merge_mode) {
	case 1:
	case 2:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_1,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_5,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		break;
	case 3:
	case 4:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		break;
	case 5:
	case 6:
	case 7:
	case 8:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		break;
	case 9:
	case 10:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_5,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		break;
	case 11:
	case 12:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_1,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		break;
	case 13:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_1,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_5,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
			cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		break;
	case 14:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_5,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_20,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_21,
			0,
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_22,
			(pseudo_ovl_cfg->spilt_ofs[0] << 16 |
			pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_23,
			(pseudo_ovl_cfg->spilt_ofs[1] << 16 |
			pseudo_ovl_cfg->out_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		break;
	case 15:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_5,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_20,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_21,
			1,
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_22,
			(pseudo_ovl_cfg->spilt_ofs[0] << 16 |
			pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_23,
			(pseudo_ovl_cfg->spilt_ofs[1] << 16 |
			pseudo_ovl_cfg->out_w[1]),
			~0);
			cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		break;
	case 16:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_1,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_5,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_20,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
			cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_21,
			0,
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_22,
			(pseudo_ovl_cfg->spilt_ofs[0] << 16 |
			pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_23,
			(pseudo_ovl_cfg->spilt_ofs[1] << 16 |
			pseudo_ovl_cfg->out_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
			cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		break;
	case 17:
	case 18:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_1,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_26,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_27,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		break;
	case 19:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_28,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_29,
			0x2,
			~0);
		break;
	case 20:
	case 21:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_24,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_25,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		break;
	default:
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_0,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_1,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->in_w[1]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_4,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[0]),
			~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
			comp->regs_pa + DISP_MERGE_CFG_5,
			(pseudo_ovl_cfg->in_h << 16 | pseudo_ovl_cfg->out_w[1]),
			~0);
		break;
	}
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + DISP_MERGE_CFG_12,
		pseudo_ovl_cfg->merge_mode, ~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + DISP_MERGE_CFG_10,
		pseudo_ovl_cfg->out_swap, ~0);

	cmdq_pkt_write(handle, comp->cmdq_base,
		comp->regs_pa + DISP_MERGE_ENABLE,
		1, 0x1);

}

static void mtk_pseudo_ovl_layer_on(struct mtk_ddp_comp *comp, unsigned int idx,
		unsigned int ext_idx, struct cmdq_pkt *handle)
{
	if (ext_idx != LYE_NORMAL) {
		DDPDBG("%s- not support extension layer ext_idx:%d\n",
		__func__, ext_idx);
		return;
	}
	DDPINFO("%s-%d\n", __func__, idx);

	mtk_ethdr_layer_on(idx, handle);
}

static void mtk_pseudo_ovl_layer_off(struct mtk_ddp_comp *comp,
	unsigned int idx, unsigned int ext_idx, struct cmdq_pkt *handle)
{
	struct mtk_disp_pseudo_ovl *pseudo_ovl = NULL;
	struct mtk_ddp_comp *priv_comp = NULL;

	DDPDBG("%s(layer=%d, ext_id=%d)+\n", __func__, idx, ext_idx);

	if (idx < 0 || idx >= EXTERNAL_INPUT_LAYER_NR) {
		DDPDBG("%s(): invalid layer id %ds\n", __func__, idx);
		return;
	}

	if (!comp)
		DDPDBG("%s(id=%d, ext_id=%d): NULL component\n",
			__func__, idx, ext_idx);

	pseudo_ovl = comp_to_pseudo_ovl(comp);
	if (!pseudo_ovl)
		DDPDBG("%s(id=%d, ext_id=%d): NULL pseudo_ovl\n",
			__func__, idx, ext_idx);

	if (ext_idx != LYE_NORMAL) {
		DDPDBG("%s- not support extension layer ext_idx:%d\n",
		__func__, ext_idx);
		return;
	}

	mtk_ethdr_layer_off(idx, handle);

	priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_MERGE_BASE+idx]);
	cmdq_pkt_write(handle, priv_comp->cmdq_base,
		priv_comp->regs_pa + DISP_MERGE_ENABLE, 0x0, 0x1);
	cmdq_pkt_write(handle, priv_comp->cmdq_base,
		priv_comp->regs_pa + DISP_MERGE_PATG_0, 0x0, 0x1);

	priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*idx]);
	mtk_mdp_rdma_stop(priv_comp->regs, priv_comp->regs_pa,
		handle, priv_comp->cmdq_base);

	priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*idx+1]);
	mtk_mdp_rdma_stop(priv_comp->regs, priv_comp->regs_pa,
		handle, priv_comp->cmdq_base);

#ifdef MTK_DISP_MMQOS_SUPPORT
	{
		u32 i;
		/*Set BW to 0*/
		for (i = 0; i < RDMA_NUM; i++) {
			pseudo_ovl->pseudo_ovl_comp[i].qos_bw = 0;
			DDPDBG("%s pseudo_ovl->pseudo_ovl_comp[%d].qos_bw = 0\n", __func__, i);
		}
	}
#endif

	DDPDBG("%s(id=%d, ext_id=%d)-\n", __func__, idx, ext_idx);
}

static void mtk_pseudo_ovl_resize_reset(
	struct mtk_ddp_comp *comp,
	struct cmdq_pkt *handle,
	int layer_id,
	int rsz_id)
{
	int i;

	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);
	unsigned int vdo1_base = mtk_crtc_get_regs_pa(comp->mtk_crtc, comp->id);
	unsigned int rdma_in  = VDO1_RDMA0_SEL_IN   + 8 * layer_id;
	unsigned int rdma_out = VDO1_RDMA0_SOUT_SEL + 8 * layer_id;
	unsigned int rsz_base = pseudo_ovl->pseudo_ovl_comp[PSEUDO_OVL_RSZ1].regs_pa;

	rsz_base += 0x2000 * rsz_id;

	DDPDBG("%s(layer-%d, rsz-%d): vdo1_base=0x%x, rdma_in=0x%x, rdma_out=0x%x, rsz_base=0x%x\n",
		__func__, layer_id, rsz_id, vdo1_base, rdma_in, rdma_out, rsz_base);

	for (i = 0; i < 2; i++) {
		cmdq_pkt_write(handle, comp->cmdq_base,
					rsz_base + 0x1000 * i, 0, ~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
					vdo1_base | (rdma_out + 4 * i), 0, ~0);
		cmdq_pkt_write(handle, comp->cmdq_base,
					vdo1_base | (rdma_in + 4 * i), 0, ~0);
	}
}

static int mtk_pseudo_ovl_resize_route(
	struct mtk_ddp_comp *comp,
	struct cmdq_pkt *handle,
	int layer_id,
	unsigned int ctrl)
{
	static int occupied[2];

	int i, j, rsz_id = -1;
	unsigned int value;

	unsigned int vdo1_base = mtk_crtc_get_regs_pa(comp->mtk_crtc, comp->id);
	unsigned int rdma_in  = VDO1_RDMA0_SEL_IN   + 8 * layer_id;
	unsigned int rdma_out = VDO1_RDMA0_SOUT_SEL + 8 * layer_id;
	unsigned int rsz_in   = VDO1_RSZ0_SEL_IN;
	unsigned int rsz_out  = VDO1_RSZ0_SOUT_SEL;

	if (!ctrl) {

		DDPDBG("%s(layer-%d): no need to resize\n",
			   __func__, layer_id);

		for (i = 0; i < 2; i++) {
			if (occupied[i] == layer_id + 1) {
				DDPDBG("%s(layer-%d): reset occupied rsz-%d\n",
					__func__, layer_id, i);
				mtk_pseudo_ovl_resize_reset(comp, handle, occupied[i]-1, i);
				occupied[i] = 0;
			}
		}
		goto end;
	}

	for (i = 0; i < 2; i++) {
		DDPDBG("%s(layer-%d): occupied[%d] = %d\n",
			   __func__, layer_id, i, occupied[i]);
		if (occupied[i] == layer_id + 1) {
			rsz_id = i;
			DDPDBG("%s(layer-%d): already occupied rsz-%d\n",
			   __func__, layer_id, i);
			goto end;
		}
	}

	switch (layer_id) {
	case 0:
		rsz_id = 0;
		break;
	case 3:
		rsz_id = 1;
		break;
	default:
		if (occupied[0] && occupied[1])
			rsz_id = (layer_id == 1 ? 1 : 0);
		else
			rsz_id = occupied[0] ? 1 : 0;
	}

	if (occupied[rsz_id])
		mtk_pseudo_ovl_resize_reset(comp, handle, occupied[rsz_id]-1, rsz_id);

	DDPDBG("%s(layer-%d): set occupied[%d] to %d\n",
			   __func__, layer_id, rsz_id, layer_id + 1);
	occupied[rsz_id] = layer_id + 1;

	///rsz_base += 0x2000 * rsz_id;
	rsz_in   += 8 * rsz_id;
	rsz_out  += 8 * rsz_id;

	//mtk_disp_mutex_add_comp(comp->mtk_crtc,
	//	comp->mtk_crtc->mutex[0],
	//	pseudo_ovl->pseudo_ovl_comp->id);

	for (i = 0; i < 2; i++) {

		value = 1;

		DDPDBG("%s(layer-%d): set RDMA_OUT+%d %x = %d\n",
			__func__, layer_id, i, vdo1_base | (rdma_out + 4 * i), 1);
		cmdq_pkt_write(handle, comp->cmdq_base,
					vdo1_base | (rdma_out + 4 * i), 1, ~0);

		if (layer_id == 1) {
			DDPDBG("%s(layer-%d): set RDMA2_RSZ_SOUT_SEL+%d %x = %d\n",
				__func__, layer_id, i,
				vdo1_base | (VDO1_RDMA2_RSZ_SOUT_SEL + 4 * i),
				1 - rsz_id);
			cmdq_pkt_write(handle, comp->cmdq_base,
						vdo1_base | (VDO1_RDMA2_RSZ_SOUT_SEL + 4 * i),
						1 - rsz_id, ~0);
			if (i == 0)
				value = 2 - rsz_id;
		} else if (layer_id == 2) {
			DDPDBG("%s(layer-%d): set RDMA4_RSZ_SOUT_SEL+%d %x = %d\n",
				__func__, layer_id, i,
				vdo1_base | (VDO1_RDMA4_RSZ_SOUT_SEL + 4 * i),
				1 - rsz_id);
			cmdq_pkt_write(handle, comp->cmdq_base,
						vdo1_base | (VDO1_RDMA4_RSZ_SOUT_SEL + 4 * i),
						1 - rsz_id, ~0);
			value = 2 - rsz_id;
		}

		DDPDBG("%s(layer-%d): set RDMA_IN +%d %x = %d\n",
			__func__, layer_id, i, vdo1_base | (rdma_in + 4 * i), value);
		cmdq_pkt_write(handle, comp->cmdq_base,
			vdo1_base | (rdma_in + 4 * i), value, ~0);

		if (occupied[i] != layer_id + 1)
			continue;

		value = 2 - layer_id + i;

		for (j = 0; j < 2; j++) {
			DDPDBG("%s(layer-%d): set RSZ_IN +%d %x = %d\n",
				__func__, layer_id, j, vdo1_base | (rsz_in  + 4 * j), value);
			cmdq_pkt_write(handle, comp->cmdq_base,
						vdo1_base | (rsz_in  + 4 * j), value, ~0);
			DDPDBG("%s(layer-%d): set RSZ_OUT+%d %x = %d\n",
				__func__, layer_id, j, vdo1_base | (rsz_out + 4 * j), value);
			cmdq_pkt_write(handle, comp->cmdq_base,
						vdo1_base | (rsz_out + 4 * j), value, ~0);
		}
	}

end:
	return rsz_id;
}

static int mtk_pseudo_ovl_resize(
	struct mtk_ddp_comp *comp,
	struct cmdq_pkt *handle,
	int layer_id,
	struct mtk_plane_pending_state *pending)
{
	int i, rsz_id = -1;

	unsigned int old_w, old_h, old_size;
	unsigned int new_w, new_h, new_size;
	unsigned int coeff_w, coeff_h;
	unsigned int ctrl = 0;

	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);
	unsigned int rsz_base = pseudo_ovl->pseudo_ovl_comp[PSEUDO_OVL_RSZ1].regs_pa;

	old_w = pending->src_w / 2;
	old_h = pending->src_h;
	old_size = old_h << 16 | old_w;

	new_w = pending->width / 2;
	new_h = pending->height;
	new_size = new_h << 16 | new_w;

	if (!old_w || !old_h || !new_w || !new_h)
		goto end;

	if (new_w > old_w) {
		ctrl |= 0b1; // bit[0]: resize horizontally
		coeff_w = old_w * 0x8000 / new_w;
	}

	if (new_h > old_h) {
		ctrl |= 0b10; // bit[1]: resize vertically
		coeff_h = old_h * 0x8000 / new_h;
	}

	DDPDBG("%s(layer-%d): src=%dx%d(%d,%d), dst=%dx%d(%d,%d), ctrl=%x\n",
		   __func__, layer_id,
		   old_w, old_h, pending->src_x, pending->src_y,
		   new_w, new_h, pending->dst_x, pending->dst_y,
		   ctrl);

	rsz_id = mtk_pseudo_ovl_resize_route(comp, handle, layer_id, ctrl);

	if (rsz_id < 0)
		goto end;

	rsz_base += 0x2000 * rsz_id;

	for (i = 0; i < 2; i++) {
		rsz_base += 0x1000 * i;
		DDPDBG("%s(layer-%d): configure rsz-%d (%x)\n",
			__func__, layer_id, rsz_id, rsz_base);
		DDPDBG("%s(layer-%d): set ctrl     %x to %x\n",
			__func__, layer_id, rsz_base | 0x04, ctrl);
		DDPDBG("%s(layer-%d): set old_size %x to %x\n",
			__func__, layer_id, rsz_base | 0x10, old_size);
		DDPDBG("%s(layer-%d): set new_size %x to %x\n",
			__func__, layer_id, rsz_base | 0x14, new_size);
		DDPDBG("%s(layer-%d): set coeff_w  %x to %x\n",
			__func__, layer_id, rsz_base | 0x18, coeff_w);
		DDPDBG("%s(layer-%d): set coeff_h  %x to %x\n",
			__func__, layer_id, rsz_base | 0x1c, coeff_h);
		cmdq_pkt_write(handle, comp->cmdq_base, rsz_base | 0x04, ctrl, ~0);
		cmdq_pkt_write(handle, comp->cmdq_base, rsz_base | 0x10, old_size, ~0);
		cmdq_pkt_write(handle, comp->cmdq_base, rsz_base | 0x14, new_size, ~0);
		cmdq_pkt_write(handle, comp->cmdq_base, rsz_base | 0x18, coeff_w, ~0);
		cmdq_pkt_write(handle, comp->cmdq_base, rsz_base | 0x1c, coeff_h, ~0);
		cmdq_pkt_write(handle, comp->cmdq_base, rsz_base, 1, ~0);
	}
end:
	return rsz_id >= 0;
}

static void mtk_pseudo_ovl_layer_config(struct mtk_ddp_comp *comp,
	unsigned int idx,
	struct mtk_plane_state *state,
	struct cmdq_pkt *handle)
{
	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);
	struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
	struct mtk_drm_private *priv = mtk_crtc->base.dev->dev_private;
	struct mtk_plane_pending_state *pending = &state->pending;
	struct mtk_disp_pseudo_ovl_config_struct pseudo_ovl_cfg = {0};
	struct mtk_mdp_rdma_cfg rdma_config = {0};
	struct mtk_ddp_comp *priv_comp = NULL;
	int i, layer_id = 0;
	bool temp_force_output_yuv = false;
	bool output_const_color = false;
	bool from_layering_rule = true;
	bool resized = false;
	const struct drm_format_info *fmt_info = drm_format_info(pending->format);
	resource_size_t regs_pa = mtk_crtc_get_regs_pa(mtk_crtc, comp->id);
#ifdef CONFIG_LK_FASTLOGO
	static bool BootlogoDisabled;
#endif

#ifdef MTK_DISP_MMQOS_SUPPORT
	unsigned long long emi_bw = 0;
	uint fps;
	u32 rdma_path = 0;
#endif
	if (priv->data->mmsys_id == MMSYS_MT8188) {
		for (i = PSEUDO_OVL_PAD0; i <= PSEUDO_OVL_PAD7; i++) {
			if (pseudo_ovl->pseudo_ovl_comp[i].regs_pa)
				cmdq_pkt_write(handle, comp->cmdq_base,
					pseudo_ovl->pseudo_ovl_comp[i].regs_pa, 3, 3);
		}
	}

	if (state->comp_state.comp_id) {
		layer_id = state->comp_state.lye_id;
	} else {
		from_layering_rule = false;
		layer_id = idx;
	}

	DDPDBG("%s(): id %d, idx:%d, enable:%d, fmt:0x%x, sec:%d , modifier:%d",
		__func__, comp->id, layer_id, pending->enable, pending->format,
		state->pending.is_sec, state->pending.modifier);

	DDPDBG("%s(): addr 0x%lx, fb w:%d, {%d,%d,%d,%d}->{%d,%d,%d,%d}, compr %d\n", __func__,
		pending->addr, pending->pitch/mtk_drm_format_plane_cpp(pending->format, 0),
		pending->src_x, pending->src_y, pending->src_w, pending->src_h,
		pending->dst_x, pending->dst_y, pending->width, pending->height,
		pending->prop_val[PLANE_PROP_COMPRESS]);

	if (priv->data->mmsys_id == MMSYS_MT8188)
		resized = mtk_pseudo_ovl_resize(comp, handle, layer_id, pending);

	DDPDBG("fmt_info->num_planes = %d\n", fmt_info->num_planes);
	if (fmt_info && (fmt_info->num_planes > 1))
		DDPINFO("plane 1 offset 0x%x\n", pending->offset1);

	/* handle dim layer for compression flag & color dim*/
	if (pending->format == DRM_FORMAT_C8) {
		output_const_color = true;
		DDPINFO("output constant color\n");
	}

	if (!pending->enable) {
		mtk_pseudo_ovl_layer_off(comp, layer_id, LYE_NORMAL, handle);
		return;
	}

	if (pending->addr == 0 && !output_const_color) {
		DDPINFO("addr not valid, ignore this layer config\n");
		return;
	}

#ifdef CONFIG_LK_FASTLOGO
	if (!BootlogoDisabled) {
		priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+4]);
		mtk_mdp_rdma_stop(priv_comp->regs, priv_comp->regs_pa,
		handle, priv_comp->cmdq_base);
		priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+5]);
		mtk_mdp_rdma_stop(priv_comp->regs, priv_comp->regs_pa,
		handle, priv_comp->cmdq_base);
		BootlogoDisabled = true;
	}
#endif

	if (!is_yuv(pending->format) && (layer_id <= 1)) {
		if (from_layering_rule) {
			DDPINFO("Received RGB data to video layer\n");
			mtk_ethdr_layer_off(layer_id, handle);
			return;
		}
		temp_force_output_yuv = true;
		DDPDBG("force yuv\n");
	}

#ifdef MTK_DISP_MMQOS_SUPPORT
	/*Calculate BW*/
	fps = drm_mode_vrefresh(&mtk_crtc->base.state->adjusted_mode);

	if (resized)
		emi_bw = pending->src_w * pending->src_h;
	else
		emi_bw = pending->width * pending->height;

	emi_bw *= mtk_drm_format_plane_cpp(pending->format, 0) * 121 / 100;

	do_div(emi_bw, 1000);
	emi_bw = emi_bw * fps;
	do_div(emi_bw, 1000);

	DDPDBG("%s : Get vrefresh rate:%d, emi_bw:%llu for layer:%d", __func__,
			fps, emi_bw, layer_id);

	rdma_path = MTK_PSEUDO_OVL_RDMA_BASE + 2 * layer_id;

	pseudo_ovl->pseudo_ovl_comp[rdma_path].qos_bw = emi_bw;
	pseudo_ovl->pseudo_ovl_comp[rdma_path+1].qos_bw = emi_bw;

	DDPDBG("%s pseudo_ovl->pseudo_ovl_comp[%d].qos_bw = %u\n", __func__,
			rdma_path, emi_bw);
#endif

	/*Config merge*/
	pseudo_ovl_cfg.fmt = pending->format;
	pseudo_ovl_cfg.out_w[0] = pending->width;
	pseudo_ovl_cfg.in_h = pending->height;
	if (is_yuv(pending->format) || temp_force_output_yuv)
		pseudo_ovl_cfg.out_swap = CFG_10_RG_GB_SWAP;
	else
		pseudo_ovl_cfg.out_swap = mtk_ethdr_isDvMode() ?
							CFG_10_RG_GB_SWAP : CFG_10_NO_SWAP;

	pseudo_ovl_cfg.merge_mode = CFG12_11_10_1PI_2PO_MERGE;

	pseudo_ovl_cfg.in_w[0] = pending->width/2 + ((pending->width/2) % 2);
	pseudo_ovl_cfg.in_w[1] = pending->width/2 - ((pending->width/2) % 2);

	priv_comp =
	&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_MERGE_BASE+layer_id]);
	mtk_pseudo_ovl_merge_config(priv_comp, &pseudo_ovl_cfg, handle);

	/*Config merge ASYNC WD : W/2*/
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + merge_async_offset[layer_id],
		(pending->height << 16 | (pending->width/2)), ~0);

	/*Config RDMA left tile*/
	rdma_config.addr0 = pending->addr;
	rdma_config.addr1 = pending->addr + pending->offset1;
	rdma_config.addr2 = 0;
	rdma_config.source_size = pending->size;
	rdma_config.x_left = pending->src_x;
	rdma_config.y_top = pending->src_y;
	/*Temp force convert to YUV for bypass ETHDR*/
	if (!is_yuv(pending->format) && temp_force_output_yuv)
		rdma_config.csc_enable = true;
	else
		rdma_config.csc_enable = false;
	rdma_config.profile = RDMA_CSC_RGB_TO_BT709;
	rdma_config.fmt = pending->format;
	rdma_config.source_width =
				pending->pitch/mtk_drm_format_plane_cpp(pending->format, 0);
	rdma_config.width  = resized ? pending->src_w / 2 : pseudo_ovl_cfg.in_w[0];
	rdma_config.height = resized ? pending->src_h : pending->height;

	if (pending->prop_val[PLANE_PROP_COMPRESS]) {
		rdma_config.encode_type = RDMA_ENCODE_AFBC;
		rdma_config.block_size = RDMA_BLOCK_32x8;
	} else {
		rdma_config.encode_type = RDMA_ENCODE_NONE;
		rdma_config.block_size = RDMA_BLOCK_NONE;
	}

	if (state->pending.is_sec)
		layer_sec_status |= 1<<layer_id;
	else
		layer_sec_status &= ~(1<<layer_id);

	priv_comp =
	&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*layer_id]);
	mtk_mdp_rdma_config(priv_comp->regs, priv_comp->regs_pa,
		&rdma_config, handle, priv_comp->cmdq_base, state->pending.is_sec);

	/*Config RDMA right tile*/
	rdma_config.x_left = pending->src_x + rdma_config.width;
	rdma_config.width = resized ? pending->src_w / 2 : pseudo_ovl_cfg.in_w[1];

	priv_comp =
	&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*layer_id+1]);

	mtk_mdp_rdma_config(priv_comp->regs, priv_comp->regs_pa,
		&rdma_config, handle, priv_comp->cmdq_base, state->pending.is_sec);

	/*Config VDO1 alpha source*/
	if (is_yuv(pending->format) || temp_force_output_yuv) {
		cmdq_pkt_write(handle, comp->cmdq_base,
			regs_pa + VDO1_HDR_TOP_CFG,
			1<<alpha_source_sel_from_prev_reg[layer_id],
			1<<alpha_source_sel_from_prev_reg[layer_id]);
	} else {
		cmdq_pkt_write(handle, comp->cmdq_base,
			regs_pa + VDO1_HDR_TOP_CFG,
			0, 1<<alpha_source_sel_from_prev_reg[layer_id]);
	}
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + mixer_alpha_value[layer_id],
		VDO1_CONFIG_ALPHA_DEFAULT_VALUE<<16|VDO1_CONFIG_ALPHA_DEFAULT_VALUE,
		~0);

	/*enable merge*/
	priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_MERGE_BASE+layer_id]);

	if (pending->enable) {
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_ENABLE, 0x1, 0x1);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_MUTE_0, 0x0, 0x1);
	} else {
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_MUTE_0, 0x1, 0x1);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_ENABLE, 0x0, 0x1);
	}

	if (output_const_color) {
		unsigned int dim_color = state->pending.prop_val[PLANE_PROP_DIM_COLOR];
		unsigned int r = (dim_color & 0xFF0000) >> 16;
		unsigned int g = (dim_color & 0xFF00) >> 8;
		unsigned int b = (dim_color & 0xFF);

		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_0, 0x1, 0x1);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_5, 0xFF, 0xFF);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_9, (r << 2), 0x3FF);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_13, (g << 2), 0x3FF);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_17, (b << 2), 0x3FF);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_33, 0xFF, 0xFF);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_37, (r << 2), 0x3FF);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_41, (g << 2), 0x3FF);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_PATG_45, (b << 2), 0x3FF);
		goto ethdr_layer_config_only;
	}

	/*Enable left tile RDMA*/
	priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*layer_id]);
	if (pending->enable)
		mtk_mdp_rdma_start(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base);
	else
		mtk_mdp_rdma_stop(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base);

	/*Enable right tile RDMA*/
	priv_comp =
	&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*layer_id+1]);
	if (pending->enable)
		mtk_mdp_rdma_start(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base);
	else
		mtk_mdp_rdma_stop(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base);

ethdr_layer_config_only:
	mtk_ethdr_layer_config(layer_id, state, handle);
	if (pending->enable)
		mtk_pseudo_ovl_layer_on(comp, layer_id, 0, handle);
}

static void mtk_pseudo_ovl_config(struct mtk_ddp_comp *comp,
	struct mtk_ddp_config *cfg,
	struct cmdq_pkt *handle)
{
	struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
	resource_size_t regs_pa = mtk_crtc_get_regs_pa(mtk_crtc, comp->id);

	DDPINFO("%s\n", __func__);

	/*Config VPP ASYNC WD: W*/
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_VPP2_ASYNC_CFG_WD,
		(cfg->h << 16 | (cfg->w/2)),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_VPP3_ASYNC_CFG_WD,
		(cfg->h << 16 | (cfg->w/2)),
		~0);

	/*Config merge 0~4 ASYNC WD : W/2*/
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_MERGE0_ASYNC_CFG_WD,
		(cfg->h << 16 | (cfg->w/2)),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_MERGE1_ASYNC_CFG_WD,
		(cfg->h << 16 | (cfg->w/2)),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_MERGE2_ASYNC_CFG_WD,
		(cfg->h << 16 | (cfg->w/2)),
		~0);
	cmdq_pkt_write(handle, comp->cmdq_base,
		regs_pa + VDO1_CONFIG_MERGE3_ASYNC_CFG_WD,
		(cfg->h << 16 | (cfg->w/2)),
		~0);

	/*golden setting*/
	mtk_pseudo_ovl_golden_setting(comp, cfg, handle);
}

static void mtk_pseudo_ovl_start(struct mtk_ddp_comp *comp,
	struct cmdq_pkt *handle)
{
	DDPINFO("%s\n", __func__);
}

static void mtk_pseudo_ovl_stop(struct mtk_ddp_comp *comp,
	struct cmdq_pkt *handle)
{
	u32 i;
	struct mtk_ddp_comp *priv_comp = NULL;
	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);
	struct mtk_drm_crtc *mtk_crtc = comp->mtk_crtc;
	resource_size_t regs_pa = mtk_crtc_get_regs_pa(mtk_crtc, comp->id);

	DDPINFO("%s\n", __func__);

	for (i = 0; i < MTK_PSEUDO_OVL_LAYER_NUM; i++) {
		/*Disable left tile RDMA*/
		priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*i]);
		mtk_mdp_rdma_stop(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base);

		/*Disable right tile RDMA*/
		priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_RDMA_BASE+2*i+1]);
		mtk_mdp_rdma_stop(priv_comp->regs, priv_comp->regs_pa,
			handle, priv_comp->cmdq_base);

		/*disable merge*/
		priv_comp =
		&(pseudo_ovl->pseudo_ovl_comp[MTK_PSEUDO_OVL_MERGE_BASE+i]);
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			priv_comp->regs_pa + DISP_MERGE_ENABLE, 0x0, 0x1);
		/*reset async*/
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			regs_pa + VDO1_CONFIG_SW0_RST_B,
			0,
			BIT(merge_async_reset_bit[i]));
		cmdq_pkt_write(handle, priv_comp->cmdq_base,
			regs_pa + VDO1_CONFIG_SW0_RST_B,
			BIT(merge_async_reset_bit[i]),
			BIT(merge_async_reset_bit[i]));
	}

#ifdef MTK_DISP_MMQOS_SUPPORT
	/*Set BW to 0*/
	for (i = 0; i < RDMA_NUM; i++) {
		pseudo_ovl->pseudo_ovl_comp[i].qos_bw = 0;
		DDPDBG("%s pseudo_ovl->pseudo_ovl_comp[%d].qos_bw = 0\n", __func__, i);
	}
#endif

}

static void mtk_pseudo_ovl_prepare(struct mtk_ddp_comp *comp)
{
	u32 i;
	int ret;
	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);

	DDPINFO("%s\n", __func__);

	ret = pm_runtime_get_sync(&pseudo_ovl_dev[PSEUDO_OVL_LARB2]->dev);
	if (ret < 0)
		DRM_ERROR("Failed to enable power domain: %d\n", ret);

	ret = pm_runtime_get_sync(&pseudo_ovl_dev[PSEUDO_OVL_LARB3]->dev);
	if (ret < 0)
		DRM_ERROR("Failed to enable power domain: %d\n", ret);

	ret = pm_runtime_get_sync(comp->dev);
	if (ret < 0)
		DRM_ERROR("Failed to enable power domain: %d\n", ret);

	for (i = PSEUDO_OVL_MDP_RDMA0; i < PSEUDO_OVL_ID_MAX; i++)
		mtk_ddp_comp_clk_prepare(&(pseudo_ovl->pseudo_ovl_comp[i]));

	/*ASYNC clk prepare*/
	for (i = 0; i < 4; i++) {
		if (pseudo_ovl->async_clk[i]) {
			ret = clk_prepare_enable(pseudo_ovl->async_clk[i]);
			if (ret)
				DDPDBG("clk prepare failed for merge%d async\n", i);
		}
	}
}

static void mtk_pseudo_ovl_unprepare(struct mtk_ddp_comp *comp)
{
	u32 i;
	int ret;
	struct mtk_disp_pseudo_ovl *pseudo_ovl = comp_to_pseudo_ovl(comp);

	DDPINFO("%s\n", __func__);

	for (i = PSEUDO_OVL_MDP_RDMA0 ; i < PSEUDO_OVL_ID_MAX ; i++)
		mtk_ddp_comp_clk_unprepare(&(pseudo_ovl->pseudo_ovl_comp[i]));

	/*ASYNC clk unprepare*/
	for (i = 0; i < 4; i++) {
		if (pseudo_ovl->async_clk[i])
			clk_disable_unprepare(pseudo_ovl->async_clk[i]);
	}

	ret = pm_runtime_put_sync(comp->dev);
	if (ret < 0)
		DRM_ERROR("Failed to disable power domain: %d\n", ret);
	ret = pm_runtime_put_sync(&pseudo_ovl_dev[PSEUDO_OVL_LARB2]->dev);
	if (ret < 0)
		DRM_ERROR("Failed to disable power domain: %d\n", ret);
	ret = pm_runtime_put_sync(&pseudo_ovl_dev[PSEUDO_OVL_LARB3]->dev);
	if (ret < 0)
		DRM_ERROR("Failed to disable power domain: %d\n", ret);
}

static const struct mtk_ddp_comp_funcs mtk_disp_pseudo_ovl_funcs = {
	.config = mtk_pseudo_ovl_config,
	.start = mtk_pseudo_ovl_start,
	.stop = mtk_pseudo_ovl_stop,
	.layer_on = mtk_pseudo_ovl_layer_on,
	.layer_off = mtk_pseudo_ovl_layer_off,
	.layer_config = mtk_pseudo_ovl_layer_config,
	.io_cmd = mtk_pseudo_ovl_io_cmd,
	.prepare = mtk_pseudo_ovl_prepare,
	.unprepare = mtk_pseudo_ovl_unprepare,
};

static int mtk_disp_pseudo_ovl_bind(struct device *dev, struct device *master,
	void *data)
{
	struct mtk_disp_pseudo_ovl *priv = dev_get_drvdata(dev);
	struct mtk_ddp_comp *comp = &priv->ddp_comp;
	struct drm_device *drm_dev = data;
	int ret;
#ifdef MTK_DISP_MMQOS_SUPPORT
	int i;
	char buf[50];
	int len;
#endif

	DDPINFO("%s\n", __func__);

	ret = mtk_ddp_comp_register(drm_dev, &priv->ddp_comp);
	if (ret < 0) {
		dev_notice(dev, "Failed to register component %s: %d\n",
			dev->of_node->full_name, ret);
		return ret;
	}

#ifdef MTK_DISP_MMQOS_SUPPORT
	/*Get ICC path*/
	for (i = PSEUDO_OVL_MDP_RDMA0; i <= PSEUDO_OVL_MDP_RDMA7; i++) {
		len = snprintf(buf, sizeof(buf), "%s%d", "mdp_rdma", i);
		if (len < 0) {
			/* Handle snprintf() error */
			DDPDBG("%s:snprintf error\n", __func__);
		}
		priv->pseudo_ovl_comp[i].qos_req = of_mtk_icc_get(dev, buf);
		DDPINFO("pseudo_ovl qos request[%d]:%x", i, priv->pseudo_ovl_comp[i].qos_req);
	}
#endif

	comp->comp_mode = &priv->rdma_memory_mode;
	return 0;
}

static void mtk_disp_pseudo_ovl_unbind(struct device *dev,
	struct device *master,
	void *data)
{
	struct mtk_disp_pseudo_ovl *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;

	mtk_ddp_comp_unregister(drm_dev, &(priv->ddp_comp));
}

static const struct component_ops mtk_disp_pseudo_ovl_component_ops = {
	.bind	= mtk_disp_pseudo_ovl_bind,
	.unbind = mtk_disp_pseudo_ovl_unbind,
};

static int mtk_disp_pseudo_ovl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_disp_pseudo_ovl *priv;
	enum mtk_ddp_comp_id comp_id;
	int ret;
	int i;
	struct resource res;
	struct device_node *larb_node = NULL;
	struct platform_device *larb_pdev = NULL;
	unsigned int larb_id;
	char clk_name[30] = {'\0'};

	DDPINFO("%s+\n", __func__);

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	comp_id = mtk_ddp_comp_get_id(dev->of_node, MTK_DISP_PSEUDO_OVL);
	if ((int)comp_id < 0) {
		DDPDBG("Failed to identify by alias: %d\n", comp_id);
		return comp_id;
	}

	ret = mtk_ddp_comp_init(dev, dev->of_node, &priv->ddp_comp, comp_id,
				&mtk_disp_pseudo_ovl_funcs);
	if (ret != 0) {
		DDPDBG("Failed to initialize component: %d\n", ret);
		return ret;
	}

	/* handle larb resources */
	priv->larb_dev0 = NULL;
	larb_node = of_parse_phandle(dev->of_node, "mediatek,larb", 0);
	if (larb_node) {
		larb_pdev = of_find_device_by_node(larb_node);
		if (larb_pdev)
			priv->larb_dev0 = &larb_pdev->dev;
		of_node_put(larb_node);
	}

	if (!priv->larb_dev0)
		return 0;

	ret = of_property_read_u32_index(dev->of_node,
		"mediatek,smi-id", 0, &larb_id);
	if (ret) {
		dev_notice(priv->larb_dev0, "read smi-id failed:%d\n", ret);
		return 0;
	}
	priv->larb_id0 = larb_id;

	DDPINFO("%s- component %s use larb smi-id:%d\n", __func__,
		mtk_dump_comp_str(&(priv->ddp_comp)), priv->larb_id0);

	//set private comp info
	for (i = 0; i < PSEUDO_OVL_ID_MAX; i++) {

		if (of_address_to_resource(dev->of_node, i, &res) != 0) {
			dev_notice(dev, "warning: missing reg in for component %s\n",
				pseudo_ovl_comp_str[i]);
			continue;
		}

		priv->pseudo_ovl_comp[i].id = DDP_COMPONENT_PSEUDO_OVL;
		priv->pseudo_ovl_comp[i].dev = dev;

		/* get the clk for each pseudo ovl comp in the device node */
		priv->pseudo_ovl_comp[i].clk = of_clk_get(dev->of_node, i);
		if (IS_ERR(priv->pseudo_ovl_comp[i].clk)) {
			priv->pseudo_ovl_comp[i].clk = NULL;
			DDPDBG("comp:%d get priv comp %s clock %d fail!\n",
			comp_id, pseudo_ovl_comp_str[i]);
		}

		priv->pseudo_ovl_comp[i].regs_pa = res.start;
		priv->pseudo_ovl_comp[i].regs = of_iomap(dev->of_node, i);
		priv->pseudo_ovl_comp[i].irq = of_irq_get(dev->of_node, i);
		priv->pseudo_ovl_comp[i].cmdq_base = priv->ddp_comp.cmdq_base;
		DDPINFO("[DRM]regs_pa:0x%lx, regs:0x%p, node:%s\n",
			(unsigned long)priv->pseudo_ovl_comp[i].regs_pa,
			priv->pseudo_ovl_comp[i].regs, pseudo_ovl_comp_str[i]);
	}

	for (i = 0 ; i < MERGE_NUM; i++) {
		sprintf(clk_name, "vdo1_merge%d_async", i);
		priv->async_clk[i] = devm_clk_get(dev, clk_name);

		if (IS_ERR(priv->async_clk[i])) {
			ret = PTR_ERR(priv->async_clk[i]);
			DDPINFO("No merge %d async clock: %d\n", i, ret);
			priv->async_clk[i] = NULL;
		}
	}

	priv->data = of_device_get_match_data(dev);

	platform_set_drvdata(pdev, priv);

	pm_runtime_enable(dev);

	ret = component_add(dev, &mtk_disp_pseudo_ovl_component_ops);
	if (ret != 0) {
		dev_notice(dev, "Failed to add component: %d\n", ret);
		pm_runtime_disable(dev);
	}
	DDPINFO("%s-\n", __func__);
	return ret;
}

static int mtk_disp_pseudo_ovl_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &mtk_disp_pseudo_ovl_component_ops);

	pm_runtime_disable(&pdev->dev);
	return 0;
}

static const struct mtk_disp_pseudo_ovl_data mt8195_pseudo_ovl_driver_data = {
	.tbd = false,
};

static const struct mtk_disp_pseudo_ovl_data mt8188_pseudo_ovl_driver_data = {
	.tbd = false,
};

static const struct of_device_id mtk_disp_pseudo_ovl_driver_dt_match[] = {
	{ .compatible = "mediatek,mt8195-disp-pseudo-ovl",
	  .data = &mt8195_pseudo_ovl_driver_data},
	{ .compatible = "mediatek,mt8188-disp-pseudo-ovl",
	  .data = &mt8188_pseudo_ovl_driver_data},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_disp_pseudo_ovl_driver_dt_match);

struct platform_driver mtk_disp_pseudo_ovl_driver = {
	.probe = mtk_disp_pseudo_ovl_probe,
	.remove = mtk_disp_pseudo_ovl_remove,
	.driver = {
		.name = "mediatek-disp-pseudo-ovl",
		.owner = THIS_MODULE,
		.of_match_table = mtk_disp_pseudo_ovl_driver_dt_match,
	},
};

static int mtk_pseudo_ovl_larb_probe(struct platform_device *pdev)
{
	struct device           *dev = &pdev->dev;
	struct pseudo_ovl_larb_data   *data;
	int ret;
	struct device_node *larb_node = NULL;
	struct platform_device *larb_pdev = NULL;
	enum mtk_pseudo_ovl_larb pseudo_ovl_larb = PSEUDO_OVL_LARB2;

	data = devm_kzalloc(dev, sizeof(*dev), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret =
	of_property_read_u32(dev->of_node, "mediatek,larb-id", &data->larb_id);
	if (ret != 0) {
		dev_notice(dev, "[%s] failed to find larb-id\n", __func__);
		return ret;
	}
	platform_set_drvdata(pdev, data);

	if (data->larb_id == 2) {
		pseudo_ovl_dev[PSEUDO_OVL_LARB2] = pdev;
		pseudo_ovl_larb = PSEUDO_OVL_LARB2;
	} else if (data->larb_id == 3) {
		pseudo_ovl_dev[PSEUDO_OVL_LARB3] = pdev;
		pseudo_ovl_larb = PSEUDO_OVL_LARB3;
	}

	larb_node = of_parse_phandle(dev->of_node, "mediatek,larb", 0);

	if (larb_node) {
		DDPINFO("Get larb node %d", data->larb_id);
		larb_pdev = of_find_device_by_node(larb_node);
		if (larb_pdev)
			pseudo_ovl_larb_dev[pseudo_ovl_larb] = &larb_pdev->dev;
		of_node_put(larb_node);
	}

	pm_runtime_enable(dev);
	DDPINFO("%s enable larb %d\n", __func__, data->larb_id);
	return ret;
}
static int mtk_pseudo_ovl_larb_remove(struct platform_device *pdev)
{
	DDPINFO("%s disable larb\n", __func__);
	return 0;
}

static const struct of_device_id mtk_pseudo_ovl_larb_match[] = {
	{.compatible = "mediatek,mt8195-pseudo-ovl-larb",},
	{.compatible = "mediatek,mt8188-pseudo-ovl-larb",},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_pseudo_ovl_larb_match);

static struct platform_driver mtk_pseudo_ovl_larb_driver = {
	.probe  = mtk_pseudo_ovl_larb_probe,
	.remove = mtk_pseudo_ovl_larb_remove,
	.driver = {
		.name   = "mtk-pseudo-ovl-larb",
		.of_match_table = mtk_pseudo_ovl_larb_match,
	},
};
module_platform_driver(mtk_pseudo_ovl_larb_driver);
