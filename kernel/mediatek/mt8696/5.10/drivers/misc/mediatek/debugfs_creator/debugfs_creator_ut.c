// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <../fs/proc/internal.h>
#include <linux/proc_fs.h>


#include "debugfs_creator.h"

static struct file_device read_file = {
	.device_name = "creator_read",
	.type = FILE_TYPE_READ,
	.folder_ptr = NULL,
	};

int ut_test(int argc, char *argv[])
{

	int i = 0;

	for (i = 0; i < argc; i++)
		pr_notice("argv[%d]: %s\n", i, argv[i]);

	return 0;
}


int ut_read_file_append_mode(int argc, char *argv[])
{
	int i = 0;

	for (i = 0; i < argc; i++)
		debugfs_creator_printf(&read_file, PRINT_TYPE_APPEND,
			"%s argv[%d]:%s\n", __func__, i, argv[i]);

	return 0;
}

int ut_read_file_trunc_mode(int argc, char *argv[])
{
	int i = 0;

	for (i = 0; i < argc; i++)
		debugfs_creator_printf(&read_file, PRINT_TYPE_TRUNC,
			"%s argv[%d]:%s\n", __func__, i, argv[i]);

	return 0;
}

struct proc_dir_entry *dir_ptr;
struct cmd_item items[] = {
	{"test", ut_test},
	{"read_app", ut_read_file_append_mode},
	{"read_trunc", ut_read_file_trunc_mode},
};

static struct file_device write_file = {
	.device_name = "creator_write",
	.type = FILE_TYPE_READ | FILE_TYPE_WRITE,
	.folder_ptr = NULL,
	.write_op = {items, ARRAY_SIZE(items)},
	};


static int __init debugfs_creator_init(void)
{
	int status = 0;

	dir_ptr = proc_mkdir("debugfs_creator_ut", NULL);

	write_file.folder_ptr = dir_ptr;
	read_file.folder_ptr = dir_ptr;

	debugfs_creator_create_file(&write_file);
	debugfs_creator_create_file(&read_file);

	return status;
}

static void __exit debugfs_creator_exit(void)
{
	debugfs_creator_destroy_file(&write_file);
	debugfs_creator_destroy_file(&read_file);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DebugFs Creator");
late_initcall_sync(debugfs_creator_init);
module_exit(debugfs_creator_exit);
