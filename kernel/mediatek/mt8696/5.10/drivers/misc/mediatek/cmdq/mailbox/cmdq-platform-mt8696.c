// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <dt-bindings/gce/mt8696-gce.h>
#include "cmdq-util.h"

#define GCE_M_PA	0x10228000

const char *cmdq_thread_module_dispatch(phys_addr_t gce_pa, s32 thread)
{
	switch (thread) {
	case 0 ... 6:
	case 8 ... 9:
		return "DISP";
	case 7:
		return "VDEC";
	case 10:
	case 19 ... 22:
		return "MDP";
	case 11:
	case 13 ... 14:
	case 16 ... 18:
		return "ISP";
	case 12:
		return "VENC";
	case 15:
		return "CMDQ";
	case 23:
		return "VFMT";
	default:
		return "CMDQ";
	}
}
EXPORT_SYMBOL(cmdq_thread_module_dispatch);

const char *cmdq_event_module_dispatch(phys_addr_t gce_pa, const u16 event,
	s32 thread)
{
	return "CMDQ";
}
EXPORT_SYMBOL(cmdq_event_module_dispatch);

u32 cmdq_get_backup_event_list(phys_addr_t gce_pa, struct cmdq_backup_event_list **list_out)
{
	u32 array_size = 0;

	static struct cmdq_backup_event_list backup_event_list_m = {
		0,
		{ {0, 0} }
	};

	if (gce_pa == GCE_M_PA) {
		/* config the sw token need to backup and restore in GCE-M */
		static struct cmdq_backup_event backup_event_m[] = {
			{0, 0}
		};

		backup_event_list_m.size = ARRAY_SIZE(backup_event_m);
		memcpy(backup_event_list_m.backup_event, backup_event_m,
			sizeof(backup_event_m[0])*backup_event_list_m.size);

		*list_out = &backup_event_list_m;
		array_size = backup_event_list_m.size;
		/* no backup event in currently 8696 platform */
		if ((array_size == 1) &&
			(backup_event_list_m.backup_event[0].event_id == 0) &&
			(backup_event_list_m.backup_event[0].value == 0))
			array_size = 0;
	} else {
		static struct cmdq_backup_event_list backup_event_list_null = {
			0,
			{ {0, 0} }
		};

		*list_out = &backup_event_list_null;
		array_size = 0;

		cmdq_err(" unknown pa:0x%llx ", gce_pa);
	}

	return array_size;
}

u32 cmdq_util_hw_id(u32 pa)
{
	return 0;
}
EXPORT_SYMBOL(cmdq_util_hw_id);

u16 cmdq_util_get_event_id(u16 event_id)
{
	switch (event_id) {
	/* event id is 10 bit */
	case CMDQ_TOKEN_MAX:
		return CMDQ_EVENT_MAX;

	default:
		cmdq_log("unknown event num:%d", event_id);
	break;
	}

	return CMDQ_SYNC_TOKEN_INVALID;
}

u32 cmdq_test_get_subsys_list(struct cmdq_util_subsys **regs_out)
{
	*regs_out = NULL;
	return 0;
}

const char *cmdq_util_hw_name(void *chan)
{
	return "GCE";
}

bool cmdq_thread_ddr_user_check(const s32 thread)
{
	switch (thread) {
	case 0 ... 6:
	case 8 ... 9:
	case 23:
		return false;
	default:
		return true;
	}
}
EXPORT_SYMBOL(cmdq_thread_ddr_user_check);

struct cmdq_util_platform_fp platform_fp = {
	.thread_module_dispatch = cmdq_thread_module_dispatch,
	.event_module_dispatch = cmdq_event_module_dispatch,
	.util_hw_id = cmdq_util_hw_id,
	.util_get_event_id = cmdq_util_get_event_id,
	.test_get_subsys_list = cmdq_test_get_subsys_list,
	.get_backup_event_list = cmdq_get_backup_event_list,
	.util_hw_name = cmdq_util_hw_name,
	.thread_ddr_module = cmdq_thread_ddr_user_check,
};

static int __init cmdq_platform_init(void)
{
	cmdq_util_set_fp(&platform_fp);
	return 0;
}
module_init(cmdq_platform_init);

MODULE_LICENSE("GPL v2");

