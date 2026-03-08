/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mdw_events
#if !defined(__MDW_EVENTS_H__) || defined(TRACE_HEADER_MULTI_READ)
#define __MDW_EVENTS_H__
#include <linux/tracepoint.h>
#include "mdw_rsc.h"
#define MDW_TAG_CMD_PRINT "%s,pid=%d,tgid=%d,cmd_uid=0x%llx,cmd_id=0x%llx,"\
	"sc_idx=%d,total_sc=%u,dev_type=%d,dev_name=%s,dev_idx=%d,"\
	"pack_id=0x%x,mc_idx=%u,mc_num=%u,mc_bitmap=0x%llx,priority=%d,"\
	"soft_limit=%u,hard_limit=%u,exec_time=%u,suggest_time=%u,"\
	"power_save=%d,mem_ctx=%u,tcm_force=%d,tcm_usage=0x%x,"\
	"tcm_real_usage=0x%x,boost=%u,ip_time=%u,ret=%d\n"\

TRACE_EVENT(mdw_cmd,
	TP_PROTO(struct mdw_tag_cmd *cmd),
	TP_ARGS(cmd),
	TP_STRUCT__entry(
		__field(uint32_t, done)
		__field(pid_t, pid)
		__field(pid_t, tgid)
		__field(uint64_t, uid)
		__field(uint64_t, cmd_id)
		__field(int, sc_idx)
		__field(uint32_t, num_sc)
		__field(int, type)
		__array(char, dev_name, MDW_DEV_NAME_SIZE)
		__field(int, dev_idx)
		__field(uint32_t, pack_id)
		__field(uint32_t, multicore_idx)
		__field(uint32_t, exec_core_num)
		__field(uint64_t, exec_core_bitmap)
		__field(unsigned char, priority)
		__field(uint32_t, soft_limit)
		__field(uint32_t, hard_limit)
		__field(uint32_t, exec_time)
		__field(uint32_t, suggest_time)
		__field(unsigned char, power_save)
		__field(uint32_t, ctx_id)
		__field(unsigned char, tcm_force)
		__field(uint32_t, tcm_usage)
		__field(uint32_t, tcm_real_usage)
		__field(uint32_t, boost)
		__field(uint32_t, ip_time)
		__field(uint32_t, ret)
	),
	TP_fast_assign(
		__entry->done = cmd->done;
		__entry->pid = cmd->pid;
		__entry->tgid = cmd->tgid;
		__entry->uid = cmd->uid;
		__entry->cmd_id = cmd->cmd_id;
		__entry->sc_idx = cmd->sc_idx;
		__entry->num_sc = cmd->num_sc;
		__entry->type = cmd->type;
		if (snprintf(__entry->dev_name, MDW_DEV_NAME_SIZE,
			"%s", cmd->dev_name) < 0)
			return;
		__entry->dev_idx = cmd->dev_idx;
		__entry->pack_id = cmd->pack_id;
		__entry->multicore_idx = cmd->multicore_idx;
		__entry->exec_core_num = cmd->exec_core_num;
		__entry->exec_core_bitmap = cmd->exec_core_bitmap;
		__entry->priority = cmd->priority;
		__entry->soft_limit = cmd->soft_limit;
		__entry->hard_limit = cmd->hard_limit;
		__entry->exec_time = cmd->exec_time;
		__entry->suggest_time = cmd->suggest_time;
		__entry->power_save = cmd->power_save;
		__entry->ctx_id = cmd->ctx_id;
		__entry->tcm_force = cmd->tcm_force;
		__entry->tcm_usage = cmd->tcm_usage;
		__entry->tcm_real_usage = cmd->tcm_real_usage;
		__entry->boost = cmd->boost;
		__entry->done = cmd->ip_time;
		__entry->ret = cmd->ret;
	),
	TP_printk(
		MDW_TAG_CMD_PRINT,
		__entry->done == 0 ? "start":"end",
		__entry->pid,
		__entry->tgid,
		__entry->uid,
		__entry->cmd_id,
		__entry->sc_idx,
		__entry->num_sc,
		__entry->type,
		__entry->dev_name,
		__entry->dev_idx,
		__entry->pack_id,
		__entry->multicore_idx,
		__entry->exec_core_num,
		__entry->exec_core_bitmap,
		__entry->priority,
		__entry->soft_limit,
		__entry->hard_limit,
		__entry->exec_time,
		__entry->suggest_time,
		__entry->power_save,
		__entry->ctx_id,
		__entry->tcm_force,
		__entry->tcm_usage,
		__entry->tcm_real_usage,
		__entry->boost,
		__entry->ip_time,
		__entry->ret
	)
);
#undef MDW_TAG_CMD_PRINT

#endif /* #if !defined(_MDW_EVENTS_H__) || defined(TRACE_HEADER_MULTI_READ) */


/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE mdw_events
#include <trace/define_trace.h>

