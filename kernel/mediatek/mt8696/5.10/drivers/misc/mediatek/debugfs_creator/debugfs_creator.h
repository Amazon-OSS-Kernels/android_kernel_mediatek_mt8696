/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#ifndef __DEBUGFS_CREATOR_H__
#define __DEBUGFS_CREATOR_H__

#include <linux/types.h>
#include <linux/fs.h>


/* data structure */
#define MAX_NAME_LEN 20

typedef int (*print_seq_func)(struct seq_file *m, void *v);
typedef int (*ITEM_CALLBACK)(int argc, char *argv[]);

struct cmd_item {
	char item_name[MAX_NAME_LEN];
	ITEM_CALLBACK item_cb;
};

struct write_operation_data {
	/* user fill */
	struct cmd_item *item;
	int cmd_item_cnt;
};

enum PRINT_TYPE_ENUM {
	PRINT_TYPE_APPEND,
	PRINT_TYPE_TRUNC,
};

struct read_operation_data {
	/* private */
	char *buffer;
	u32 used_size;
	u32 buffer_size;
};

enum FILE_TYPE_ENUM {
	FILE_TYPE_READ = BIT_MASK(0),
	FILE_TYPE_WRITE = BIT_MASK(1),
};

struct file_device {
	/* user fill */
	char *device_name;	/* create procfs file name */
	/* create file is located in which folder */
	struct proc_dir_entry *folder_ptr;
	struct write_operation_data write_op; /* support write operation ? */
	enum FILE_TYPE_ENUM type;

	/* private */
	struct list_head list;
	struct proc_dir_entry *file_ptr;
	/* default support read operation */
	struct read_operation_data read_op;
};

/* error code */
enum CREATOR_ERR_CODE_ENUM {
	CREATOR_OK = 0,
	CREATOR_ERR_INVALID_DEVICE_NAME = -1,
	CREATOR_ERR_INVALID_FILE_TYPE = -2,
	CREATOR_ERR_ALLOC_FAIL = -3,
	CREATOR_ERR_CREATE_FS_FAIL = -4,
};

/* public API */
int debugfs_creator_create_file(struct file_device *file);
void debugfs_creator_destroy_file(struct file_device *file);
/* print message to read file */
void debugfs_creator_printf(struct file_device *fd,
	enum PRINT_TYPE_ENUM print_type,
	const char *print_msg, ...);
void debugfs_creator_printf_args(struct file_device *fd,
	enum PRINT_TYPE_ENUM print_type,
	const char *msg, va_list args);
void debugfd_creator_reset_file(struct file_device *fd);



#endif /* __DEBUGFS_CREATOR_H__ */
