/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef __DISP_CLI_H__
#define __DISP_CLI_H__
#include <linux/list.h>
#include <linux/ratelimit.h>
#include <linux/printk.h>
#include "disp_cli_def.h"


enum CLI_STATUS {
	CLI_STATUS_OK = 0,
	CLI_STATUS_CREATE_PROC_CLI_FAIL = 1,
	CLI_STATUS_UNRECOGNIZED_IOCTL = 2,
	CLI_STATUS_COPY_TO_USER_FAIL = 3,
	CLI_STATUS_COPY_FROM_USER_FAIL = 4,
	CLI_STATUS_ITEM_NOT_FOUND = 5,
	CLI_STATUS_CALLBACK_ERROR = 6,
	CLI_STATUS_TOO_MANY_MODULE = 7,
	CLI_STATUS_PARSE_CMD_FAIL = 8,
	CLI_STATUS_ALLOCATE_BUFFER_FAIL = 9,

};

#define CLI_ERR(string, args...) \
	pr_notice_ratelimited("CLI_ERR: "string, ##args)


#define CLI_LOG(string, args...) do { \
	if (1) \
		pr_notice_ratelimited("CLI_LOG: "string, ##args); \
} while (0)

typedef int (*pFunc)(int i4Argc, const char **aszArgv);


enum CLI_ACCESS_RIGHT_T {
	CLI_SUPERVISOR = 0,
	CLI_ADMIN,
	CLI_GUEST,
	CLI_HIDDEN,
};


/* one item */
struct CLI_EXEC_T {
	char *name;	/* command string */
	char *pszCmdAbbrStr;	/* command abbreviation */
	pFunc function;	/* execution function */
	struct CLI_EXEC_T *prCmdNextLevel;	/* next level command table */
	char *detail;	/* command description string */
	enum CLI_ACCESS_RIGHT_T	eAccessRight;	/* command access right */
};




struct module_item_struct {
	struct CLI_EXEC_T module_case;
	char module_name[MAX_SHORT_STRING_LEN];
	struct list_head list;
};

int32_t cli_register(char *module_name, struct CLI_EXEC_T item[]);


#endif /* endof __DISP_CLI_H__ */
