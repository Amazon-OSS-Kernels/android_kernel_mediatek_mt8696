// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/console.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/spinlock.h>

#include <mtk_dbg_common_v1.h>
#include <mtk_lpm_module.h>
#include <mtk_resource_constraint_v1.h>

#include <mt8696_pwr_ctrl.h>
#include <mt8696_dbg_fs_common.h>
#include <mt8696_cond.h>
#include <mt8696_spm_comm.h>

#include <mtk_lpm_sysfs.h>
#include <mtk_lp_sysfs.h>

#define SPM_RC_UPDATE_COND_ID_MASK	0xffff
#define SPM_RC_UPDATE_COND_RC_ID_MASK	0xffff
#define SPM_RC_UPDATE_COND_RC_ID_SHIFT	(16)

#define SPM_RC_UPDATE_COND_CTRL_ID(rc, cond)\
	(((rc & SPM_RC_UPDATE_COND_RC_ID_MASK)\
		<< SPM_RC_UPDATE_COND_RC_ID_SHIFT)\
	| (cond & SPM_RC_UPDATE_COND_ID_MASK))


#define MT8696_LPM_RC_RATIO_DEFAULT		10000
#define mt8696_rc_log(buf, sz, len, fmt, args...) ({\
	if (len < sz)\
		len += scnprintf(buf + len, sz - len,\
				 fmt, ##args); })

#define MT8696_DBG_SMC(_id, _act, _rc, _param) ({\
	(unsigned long)mtk_lpm_smc_spm_dbg(_id, _act, _rc, _param); })


enum MT8696_RC_RATIO_TYPE {
	MT8696_RC_RATIO_UNKNOWN,
	MT8696_RC_RATIO_ENABLE,
	MT8696_RC_RATIO_DISABLE,
	MT8696_RC_RATIO_ENABLE_INFO,
	MT8696_RC_RATIO_INFO,
};

enum MT8696_RC_NODE_TYPE {
	MT8696_RC_NODE_STATE,
	MT8696_RC_NODE_RC_ENABLE,
	MT8696_RC_NODE_RC_STATE_SIMPLE,
	MT8696_RC_NODE_RC_STATE,
	MT8696_RC_NODE_COND_ENABLE,
	MT8696_RC_NODE_COND_STATE,
	MT8696_RC_NODE_COND_SET,
	MT8696_RC_NODE_COND_CLR,
	MT8696_RC_NODE_RATIO_ENABLE,
	MT8696_RC_NODE_RATIO_INTERVAL,
	MT8696_RC_NODE_VALID_BBLPM,
	MT8696_RC_NODE_VALID_TRACE,
	MT8696_RC_NODE_MAX
};

struct MT8696_RC_NODE {
	const char *name;
	int rc_id;
	int type;
	struct mtk_lp_sysfs_handle handle;
	struct mtk_lp_sysfs_op op;
};

struct MT8696_RC_ENTERY {
	const char *name;
	struct mtk_lp_sysfs_handle handle;
};

struct MT8696_RC_COND_HANDLES {
	struct MT8696_RC_ENTERY root;
	struct MT8696_RC_NODE hSet;
	struct MT8696_RC_NODE hClr;
	struct MT8696_RC_NODE hState;
	struct MT8696_RC_NODE hEnable;
};

struct MT8696_RC_VALID_HANDLES {
	struct MT8696_RC_ENTERY root;
	struct MT8696_RC_NODE hBblpm;
	struct MT8696_RC_NODE hTrace;
};

struct MT8696_RC_RATIO_HANDLES {
	struct MT8696_RC_ENTERY root;
	struct MT8696_RC_NODE hInterval;
	struct MT8696_RC_NODE hEnable;
};

struct MT8696_RC_HANDLE_BASIC {
	struct MT8696_RC_ENTERY root;
	struct MT8696_RC_NODE hEnable;
	struct MT8696_RC_NODE hState;
};

struct MT8696_RC_HANDLE {
	struct MT8696_RC_HANDLE_BASIC basic;
	struct MT8696_RC_COND_HANDLES hCond;
	struct MT8696_RC_VALID_HANDLES valid;
};


#define MT8696_CONSTRAINT_GENERIC_OP(op, _priv) ({\
	op.fs_read = mt8696_generic_rc_read;\
	op.fs_write = mt8696_generic_rc_write;\
	op.priv = _priv; })


#define MT8696_GENERIC_RC_NODE_INIT(_n, _name, _id, _type) ({\
	_n.name = _name;\
	_n.type = _type;\
	_n.rc_id = _id;\
	MT8696_CONSTRAINT_GENERIC_OP(_n.op, &_n); })


struct mtk_lp_sysfs_handle mt8696_entry_rc;
struct MT8696_RC_HANDLE rc_dram;
struct MT8696_RC_HANDLE rc_syspll;
struct MT8696_RC_HANDLE rc_bus26m;
struct MT8696_RC_HANDLE_BASIC rc_cpu_buckldo;
struct MT8696_RC_RATIO_HANDLES rc_Ratio;
struct MT8696_RC_NODE rc_state;

static char *mt8696_spm_cond_cg_str[PLAT_SPM_COND_MAX] = {
	[PLAT_SPM_COND_MTCMOS1]	= "MTCMOS_1",
	[PLAT_INFRA_SW_CG0]	= "INFRA_0",
	[PLAT_INFRA_SW_CG1]	= "INFRA_1",
	[PLAT_MMSYS_CG_CON0]	= "MMSYS_CG_0",
	[PLAT_MMSYS_CG_CON1]	= "MMSYS_CG_1",
	[PLAT_AUDIO_IPSYS_CG0]      = "AUDIO_0",
	[PLAT_NNA_CG_0]	= "NNA_CG_0",
	[PLAT_NNA_CG_1]	= "NNA_CG_1",
	[PLAT_PERISYS_CG0]	= "PERISYS_0",
	[PLAT_PERISYS_CG1]	= "PERISYS_1",
	[PLAT_PERISYS_CG2]	= "PERISYS_2",
	[PLAT_RV33_CG]      = "RV33_CG",
	[PLAT_VDEC_CG_0]      = "VDEC_CG_0",
	[PLAT_VDEC_CG_1]	= "VDEC_CG_1",
	[PLAT_VDEC_CG_2]	= "VDEC_CG_2",
	[PLAT_VDEC_CG_3]	= "VDEC_CG_3",
	[PLAT_VDEC_CG_4]	= "VDEC_CG_4",
	[PLAT_VDEC_CG_5]	= "VDEC_CG_5",
	[PLAT_VENC_CG_0]	= "VENC_CG_0",
};

static char *mt8696_spm_cond_pll_str[PLAT_SPM_COND_PLL_MAX] = {
	[PLAT_SPM_COND_UNIVPLL]	= "UNIVPLL",
	[PLAT_SPM_COND_MFGPLL]	= "MFGPLL",
	[PLAT_SPM_COND_MSDCPLL]	= "MSDCPLL",
	[PLAT_SPM_COND_TVPLL]	= "TVPLL",
	[PLAT_SPM_COND_MMPLL]	= "MMPLL",
};


int mt8696_lpm_rc_cond_ctrl(int rc_id, unsigned int act,
			unsigned int cond_id, unsigned int value)
{
	unsigned int cond_ctrl_id;
	int res = 0;

	cond_ctrl_id = SPM_RC_UPDATE_COND_CTRL_ID(rc_id, cond_id);

	if (cond_id < PLAT_SPM_COND_MAX)
		MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_COND_CTRL,
				    act, cond_ctrl_id, value);
	else if ((cond_id - PLAT_SPM_COND_MAX) < PLAT_SPM_COND_PLL_MAX)
		MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_COND_CTRL,
				    act, cond_ctrl_id, !!value);
	else
		pr_info("[%s:%d] - unknown cond id = %u\n",
			__func__, __LINE__, cond_id);

	return res;
}

static ssize_t mt8696_lpm_rc_block_info(int rc_id,
					      char *ToUserBuf,
					      size_t sz)
{
	uint32_t block, b;
	int i;
	ssize_t len = 0;

	block = (uint32_t)
		MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_COND_BLOCK,
				MT_LPM_SMC_ACT_GET, rc_id, 0);
	b = (uint32_t)
		MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_COND_CHECK,
				MT_LPM_SMC_ACT_GET, rc_id, 0);

	mt8696_rc_log(ToUserBuf, sz, len,
			"blocked=%u, blocked_cond=0x%08x\n",
			b, block);

	for (i = 0, b = block >> SPM_COND_BLOCKED_CG_IDX;
	     i < PLAT_SPM_COND_MAX; i++)
		mt8696_rc_log(ToUserBuf, sz, len,
				"[%2d] %8s=0x%08lx\n", i,
				mt8696_spm_cond_cg_str[i],
				((b >> i) & 0x1) ?
		    MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_BLOCK_DETAIL,
				   MT_LPM_SMC_ACT_GET, rc_id, i) : 0);
	for (i = 0, b = block >> SPM_COND_BLOCKED_PLL_IDX;
	     i < PLAT_SPM_COND_PLL_MAX; i++)
		mt8696_rc_log(ToUserBuf, sz, len,
				"[%2d] %8s=%d\n",
				(i + PLAT_SPM_COND_MAX),
				mt8696_spm_cond_pll_str[i],
				((b >> i) & 0x1));
	return len;
}

static ssize_t mt8696_rc_state(int rc_id, char *ToUserBuf, size_t sz)
{
	ssize_t len = 0;

	if (rc_id < 0)
		return 0;

	mt8696_rc_log(ToUserBuf, sz, len,
		"enable=%lu, count=%lu, rc-id=%d\n",
		MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_SWITCH,
				MT_LPM_SMC_ACT_GET, rc_id, 0)
				& MT_SPM_RC_VALID_SW,
		MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_CNT,
				MT_LPM_SMC_ACT_GET, rc_id, 0),
		rc_id);
	return len;
}

static ssize_t mt8696_generic_rc_read(char *ToUserBuf,
					    size_t sz, void *priv)
{
	ssize_t len = 0;
	struct MT8696_RC_NODE *node = (struct MT8696_RC_NODE *)priv;

	if (!node)
		return -EINVAL;

	switch (node->type) {
	case MT8696_RC_NODE_STATE:
		mt8696_rc_log(ToUserBuf, sz, len, "count:%lu\n",
			MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_IDLE_CNT,
				    MT_LPM_SMC_ACT_GET, DBG_CTRL_COUNT, 0));
		break;
	case MT8696_RC_NODE_RC_STATE_SIMPLE:
		len += mt8696_rc_state(node->rc_id, ToUserBuf, sz);
		break;
	case MT8696_RC_NODE_RC_STATE:
		len += mt8696_rc_state(node->rc_id, ToUserBuf, sz);
		len += mt8696_lpm_rc_block_info(node->rc_id,
					ToUserBuf + len, sz - len);
		break;
	case MT8696_RC_NODE_RC_ENABLE:
		mt8696_rc_log(ToUserBuf, sz, len, "%lu\n",
			MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_SWITCH,
				    MT_LPM_SMC_ACT_GET, node->rc_id, 0));
		break;
	case MT8696_RC_NODE_COND_ENABLE:
		mt8696_rc_log(ToUserBuf, sz, len, "%lu\n",
			MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_COND_CHECK,
				    MT_LPM_SMC_ACT_GET, node->rc_id, 0));
		break;
	case MT8696_RC_NODE_COND_STATE:
		len += mt8696_lpm_rc_block_info(node->rc_id, ToUserBuf, sz);
		break;
	case MT8696_RC_NODE_RATIO_ENABLE:
		mt8696_rc_log(ToUserBuf, sz, len, "%lu\n",
			MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_RES_CTRL,
				    MT_LPM_SMC_ACT_GET, node->rc_id, 0));
		break;

	case MT8696_RC_NODE_VALID_BBLPM:
		mt8696_rc_log(ToUserBuf, sz, len, "%lu\n",
			MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_BBLPM,
				    MT_LPM_SMC_ACT_GET, node->rc_id, 0));
		break;
	case MT8696_RC_NODE_VALID_TRACE:
		mt8696_rc_log(ToUserBuf, sz, len, "%lu\n",
			MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_TRACE,
				    MT_LPM_SMC_ACT_GET, node->rc_id, 0));
		break;
	}

	return len;
}

static ssize_t mt8696_generic_rc_write(char *FromUserBuf,
					  size_t sz, void *priv)
{
	struct MT8696_RC_NODE *node = (struct MT8696_RC_NODE *)priv;

	if (!node)
		return -EINVAL;

	if ((node->type == MT8696_RC_NODE_RC_ENABLE) ||
		(node->type == MT8696_RC_NODE_COND_ENABLE) ||
		(node->type == MT8696_RC_NODE_VALID_BBLPM) ||
		(node->type == MT8696_RC_NODE_VALID_TRACE)) {
		unsigned int parm;
		int cmd;

		if ((!kstrtoint(FromUserBuf, 10, &parm)) == 1) {
			cmd = (node->type == MT8696_RC_NODE_RC_ENABLE) ?
				MT_SPM_DBG_SMC_UID_RC_SWITCH :
				(node->type == MT8696_RC_NODE_COND_ENABLE) ?
				MT_SPM_DBG_SMC_UID_COND_CHECK :
				(node->type == MT8696_RC_NODE_VALID_BBLPM) ?
				MT_SPM_DBG_SMC_UID_RC_BBLPM :
				(node->type == MT8696_RC_NODE_VALID_TRACE) ?
				MT_SPM_DBG_SMC_UID_RC_TRACE : -1;

			if (cmd < 0)
				return -EINVAL;

			if (!!parm)
				parm = MT_LPM_SMC_ACT_SET;
			else
				parm = MT_LPM_SMC_ACT_CLR;
			MT8696_DBG_SMC(cmd, parm, node->rc_id, 0);
		}
	} else if ((node->type == MT8696_RC_NODE_COND_SET)
		|| (node->type == MT8696_RC_NODE_COND_CLR)) {
		unsigned int parm1, parm2, act;

		if (sscanf(FromUserBuf, "%u %x", &parm1, &parm2) == 2) {
			act = (node->type == MT8696_RC_NODE_COND_SET) ?
				MT_LPM_SMC_ACT_SET :
				(node->type == MT8696_RC_NODE_COND_CLR) ?
				MT_LPM_SMC_ACT_CLR : 0;

			if (act != 0)
				mt8696_lpm_rc_cond_ctrl(node->rc_id, act,
							parm1, parm2);
		}
	} else if ((node->type == MT8696_RC_NODE_RATIO_ENABLE)
		|| (node->type == MT8696_RC_NODE_RATIO_INTERVAL)) {
		unsigned int parm;

		if ((!kstrtoint(FromUserBuf, 10, &parm)) == 1) {
			if (node->type == MT8696_RC_NODE_RATIO_ENABLE) {
				parm = (!!parm) ? MT_LPM_SMC_ACT_SET :
						MT_LPM_SMC_ACT_CLR;
				MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_RES_CTRL,
					parm, MT_RM_CONSTRAINT_ID_DRAM, 0);
				MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_RES_CTRL,
					parm, MT_RM_CONSTRAINT_ID_SYSPLL, 0);
				MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_RES_CTRL,
					parm, MT_RM_CONSTRAINT_ID_BUS26M, 0);
			}
		}
	}
	return sz;
}

static int mt8696_lpm_rc_node_add(struct MT8696_RC_NODE *n,
				int mode, struct MT8696_RC_ENTERY *p)
{
	return mtk_lpm_sysfs_sub_entry_node_add(n->name, mode,
					&n->op, &p->handle, &n->handle);
}

static int mt8696_lpm_rc_entry_add(struct MT8696_RC_ENTERY *n,
					int mode, struct MT8696_RC_ENTERY *p)
{
	return mtk_lpm_sysfs_sub_entry_add(n->name, mode,
					   &p->handle, &n->handle);
}

static int mt8696_lpm_rc_valid_node_add(int rc_id,
					struct MT8696_RC_ENTERY *parent,
					struct MT8696_RC_VALID_HANDLES *valid)
{
	int bRet = 0;

	if (!valid || !parent)
		return -EINVAL;

	valid->root.name = "valid";
	bRet = mt8696_lpm_rc_entry_add(&valid->root, 0644, parent);

	if (!bRet) {
		MT8696_GENERIC_RC_NODE_INIT(valid->hBblpm, "bblpm",
				    rc_id, MT8696_RC_NODE_VALID_BBLPM);
		mt8696_lpm_rc_node_add(&valid->hBblpm, 0200, &valid->root);
		MT8696_GENERIC_RC_NODE_INIT(valid->hTrace, "trace",
				    rc_id, MT8696_RC_NODE_VALID_TRACE);
		mt8696_lpm_rc_node_add(&valid->hTrace, 0200, &valid->root);
	}
	return bRet;
}

static int mt8696_lpm_rc_ratio_entry(struct mtk_lp_sysfs_handle *parent,
					struct MT8696_RC_RATIO_HANDLES *ratio)
{
	int bRet = 0;

	if (!ratio || !parent)
		return -EINVAL;

	ratio->root.name = "ratio";
	bRet = mtk_lpm_sysfs_sub_entry_add(ratio->root.name, 0644,
					parent, &ratio->root.handle);
	if (!bRet) {
		MT8696_GENERIC_RC_NODE_INIT(ratio->hEnable, "enable",
				    0, MT8696_RC_NODE_RATIO_ENABLE);
		mt8696_lpm_rc_node_add(&ratio->hEnable, 0200, &ratio->root);
		MT8696_GENERIC_RC_NODE_INIT(ratio->hInterval, "interval",
				    0, MT8696_RC_NODE_RATIO_INTERVAL);
		mt8696_lpm_rc_node_add(&ratio->hInterval, 0200, &ratio->root);
	}
	return 0;
}

static int mt8696_lpm_rc_cond_node_add(int rc_id,
					struct MT8696_RC_ENTERY *parent,
					struct MT8696_RC_COND_HANDLES *cond)
{
	int bRet = 0;

	if (!cond || !parent)
		return -EINVAL;

	cond->root.name = "cond";
	bRet = mt8696_lpm_rc_entry_add(&cond->root, 0644, parent);

	if (!bRet) {
		MT8696_GENERIC_RC_NODE_INIT(cond->hSet, "set",
				    rc_id, MT8696_RC_NODE_COND_SET);
		mt8696_lpm_rc_node_add(&cond->hSet, 0200, &cond->root);

		MT8696_GENERIC_RC_NODE_INIT(cond->hClr, "clr",
				    rc_id, MT8696_RC_NODE_COND_CLR);
		mt8696_lpm_rc_node_add(&cond->hClr, 0200, &cond->root);

		MT8696_GENERIC_RC_NODE_INIT(cond->hEnable, "enable",
				    rc_id, MT8696_RC_NODE_COND_ENABLE);
		mt8696_lpm_rc_node_add(&cond->hEnable, 0644, &cond->root);

		MT8696_GENERIC_RC_NODE_INIT(cond->hState, "state",
				    rc_id, MT8696_RC_NODE_COND_STATE);
		mt8696_lpm_rc_node_add(&cond->hState, 0444, &cond->root);
	}
	return bRet;
}


static int mt8696_lpm_rc_entry_nodes_basic(int IsSimple,
					const char *name, int rc_id,
					struct mtk_lp_sysfs_handle *parent,
					struct MT8696_RC_HANDLE_BASIC *rc)
{
	int bRet = 0;

	if (!parent || !rc)
		return -EINVAL;

	rc->root.name = name;
	bRet = mtk_lpm_sysfs_sub_entry_add(rc->root.name, 0644,
					parent, &rc->root.handle);
	if (bRet)
		return -EINVAL;

	MT8696_GENERIC_RC_NODE_INIT(rc->hState, "state", rc_id,
				    (IsSimple) ? MT8696_RC_NODE_RC_STATE_SIMPLE
						: MT8696_RC_NODE_RC_STATE);
	mt8696_lpm_rc_node_add(&rc->hState, 0444, &rc->root);

	MT8696_GENERIC_RC_NODE_INIT(rc->hEnable, "enable", rc_id,
				    MT8696_RC_NODE_RC_ENABLE);
	mt8696_lpm_rc_node_add(&rc->hEnable, 0644, &rc->root);

	return bRet;
}

int mt8696_lpm_rc_entry_nodes(const char *name, int rc_id,
				    struct mtk_lp_sysfs_handle *parent,
				    struct MT8696_RC_HANDLE *rc)
{
	int bRet = 0;

	if (!parent || !rc)
		return -EINVAL;

	memset(rc, 0, sizeof(struct MT8696_RC_HANDLE));

	bRet = mt8696_lpm_rc_entry_nodes_basic(0, name, rc_id,
						parent, &rc->basic);

	bRet = mt8696_lpm_rc_cond_node_add(rc_id, &rc->basic.root,
					   &rc->hCond);

	bRet = mt8696_lpm_rc_valid_node_add(rc_id, &rc->basic.root,
					   &rc->valid);

	return 0;
}

int mt8696_dbg_lpm_fs_init(void)
{
	/* enable resource constraint condition block latch */
	mtk_lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_BLOCK_LATCH,
				    MT_LPM_SMC_ACT_SET, 0, 0);
	mtk_lpm_sysfs_root_entry_create();

	mtk_lpm_sysfs_sub_entry_add("rc", 0644, NULL,
				    &mt8696_entry_rc);

	MT8696_GENERIC_RC_NODE_INIT(rc_state, "state",
				    0, MT8696_RC_NODE_STATE);
	mtk_lpm_sysfs_sub_entry_node_add(rc_state.name, 0444,
					&rc_state.op, &mt8696_entry_rc,
					&rc_state.handle);

	mt8696_lpm_rc_ratio_entry(&mt8696_entry_rc, &rc_Ratio);

	mt8696_lpm_rc_entry_nodes_basic(1, "cpu-buck-ldo",
				MT_RM_CONSTRAINT_ID_CPU_BUCK_LDO,
				&mt8696_entry_rc, &rc_cpu_buckldo);
	mt8696_lpm_rc_entry_nodes("dram", MT_RM_CONSTRAINT_ID_DRAM,
				  &mt8696_entry_rc, &rc_dram);
	mt8696_lpm_rc_entry_nodes("syspll", MT_RM_CONSTRAINT_ID_SYSPLL,
				  &mt8696_entry_rc, &rc_syspll);
	mt8696_lpm_rc_entry_nodes("bus26m", MT_RM_CONSTRAINT_ID_BUS26M,
				  &mt8696_entry_rc, &rc_bus26m);

	/* enable constraint tracing */
	MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_TRACE,
			MT_LPM_SMC_ACT_SET, MT_RM_CONSTRAINT_ID_BUS26M, 0);
	MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_TRACE,
			MT_LPM_SMC_ACT_SET, MT_RM_CONSTRAINT_ID_SYSPLL, 0);
	MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_TRACE,
			MT_LPM_SMC_ACT_SET, MT_RM_CONSTRAINT_ID_DRAM, 0);

	return 0;
}

int mt8696_dbg_lpm_fs_deinit(void)
{
	/* disable resource contraint condition block latch */
	mtk_lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_BLOCK_LATCH,
			    MT_LPM_SMC_ACT_CLR, 0, 0);

	/* disable constraint tracing */
	MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_TRACE,
			MT_LPM_SMC_ACT_CLR, MT_RM_CONSTRAINT_ID_BUS26M, 0);
	MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_TRACE,
			MT_LPM_SMC_ACT_CLR, MT_RM_CONSTRAINT_ID_SYSPLL, 0);
	MT8696_DBG_SMC(MT_SPM_DBG_SMC_UID_RC_TRACE,
			MT_LPM_SMC_ACT_CLR, MT_RM_CONSTRAINT_ID_DRAM, 0);

	return 0;
}
