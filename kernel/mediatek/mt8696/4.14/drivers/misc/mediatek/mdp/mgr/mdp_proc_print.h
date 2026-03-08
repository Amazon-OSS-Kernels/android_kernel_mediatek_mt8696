/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __MDP_PROC_PRINT_H__
#define  __MDP_PROC_PRINT_H__
#include "mdp_def.h"

#define PAGESIZE 4096
#define SHOW_RECORD_COUNT 600

struct proc_debug {
	char name[MAX_ARRAY_SIZE];
	char *printBuffer;
	unsigned int bufferSize;
	unsigned int count;
};

struct mdp_proc_record_struct {
	struct list_head listHead;
	int record_count;
	struct mutex lock;
};

struct mdp_proc_print_struct {
	/* record task time */
	bool record_ready_to_use;
	struct mdp_proc_record_struct *record;


	/* record error log to proc */
	bool error_ready_to_use;
	struct proc_debug *error;
};

enum MDP_TASK_STATUS mdp_proc_add_error(char *fmt, ...);
enum MDP_TASK_STATUS mdp_proc_add_time_item(struct mdp_task_struct *pTask);

int mdp_proc_print_show_record(struct seq_file *seq, void *v);
int mdp_proc_print_show_error(struct seq_file *seq, void *v);

int mdp_proc_print_open_error(struct inode *node, struct file *file);
int mdp_proc_print_open_record(struct inode *node, struct file *file);

enum MDP_TASK_STATUS mdp_proc_print_init(void);
void mdp_proc_print_exit(void);

int64_t mdp_proc_get_time_in_us(uint64_t start, uint64_t end);

#endif /* endof __MDP_PROC_PRINT_H__ */

