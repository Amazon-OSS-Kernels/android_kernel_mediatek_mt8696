// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <../fs/proc/internal.h>


#include "debugfs_creator.h"

#define CREATOR_ERR(string, args...) \
	pr_notice("[DEBUGFS_CREATOR ERR]"string" @%s,%u\n", \
		##args, __func__, __LINE__)

#define MAX_WORD_COUNT 10
#define MAX_CHAR_COUNT 256

static LIST_HEAD(creator_header);

static bool is_char(char p)
{
	if (p >= '0' && p <= '9')
		return true;
	if (p >= 'a' && p <= 'z')
		return true;
	if (p >= 'A' && p <= 'Z')
		return true;
	if (p == '_' || p == '-' || p == '=')
		return true;
	if (p == '/' || p == '.')
		return true;

	return false;
}

static int debugfs_creator_process_string(char *str,
	const struct write_operation_data *operation)
{
	unsigned int word_count = 0;
	int find_word = 0;
	int i = 0;
	char *pbeg = str;
	char *pend = str;
	char *buffer[MAX_WORD_COUNT] = {NULL};

	for (i = 0; i < ARRAY_SIZE(buffer); i++) {
		buffer[i] = vmalloc(MAX_CHAR_COUNT);
		memset(buffer[i], 0, MAX_CHAR_COUNT);
	}

	/* split string to word , store in buffer[] */
	while (*pend != '\0') {
		if (find_word == 0 && is_char(*pend)) {
			/*find a word begin */
			find_word = 1;
			pbeg = pend;
		} else if (find_word == 1 && !is_char(*pend)) {
			/* find a word end */
			find_word = 0;
			/* copy pbeg ~ pend to buffer */
			memcpy(buffer[word_count++], pbeg,
				(pend - pbeg) > (MAX_CHAR_COUNT - 1) ?
				(MAX_CHAR_COUNT - 1) : (pend - pbeg));
			if (word_count >= MAX_WORD_COUNT)
				break;
		}
		pend++;
	}
	if (find_word == 1)
		memcpy(buffer[word_count++], pbeg,
			(pend - pbeg) > (MAX_CHAR_COUNT - 1) ?
			(MAX_CHAR_COUNT - 1) : (pend - pbeg));

	/* search item, call item related callback function.
	 * item name store in buffer[0]
	 * item param store in buffer[1] ~ buffer[word_count -1]
	 */

	for (i = 0; i < word_count; i++)
		CREATOR_ERR("buffer[%d]:%s", i, buffer[i]);

	for (i = 0; i < operation->cmd_item_cnt; i++) {
		if ((strlen(operation->item[i].item_name) ==
			strlen(buffer[0])) &&
			strncmp(operation->item[i].item_name,
				buffer[0],
				strlen(operation->item[i].item_name)) == 0) {
			operation->item[i].item_cb(word_count - 1,
				&buffer[1]);
			break;
		}
	}
	if (i == operation->cmd_item_cnt)
		CREATOR_ERR("invalid cmd:%s", str);

	for (i = 0; i < ARRAY_SIZE(buffer); i++)
		vfree(buffer[i]);

	return 0;

}

static ssize_t debugfs_creator_write_cmd(struct file *f,
	const char __user *user_buffer,
	size_t size, loff_t *offset)
{
	size_t copied_size = 0;
	char *buffer = NULL;

	buffer = vmalloc(size + 1);
	if (buffer == NULL) {
		CREATOR_ERR("allocate buffer fail");
		return 0;
	}
	memset(buffer, 0, size + 1);
	do {
		struct file_device *fd;

		if (copy_from_user(buffer, user_buffer, size) != 0) {
			CREATOR_ERR("copy write cmd from user fail");
			copied_size = 0;
			break;
		}
		copied_size = size;
		buffer[size] = '\0';

		CREATOR_ERR("cmd: %s", buffer);

		list_for_each_entry(fd, &creator_header, list) {
			/* compare struct file & proc_dir_entry */
			struct proc_inode *node = container_of(f->f_inode,
				struct proc_inode, vfs_inode);

			if (fd->file_ptr == node->pde) {
				if (fd->type & FILE_TYPE_WRITE)
					debugfs_creator_process_string(buffer,
						&fd->write_op);
				else
					CREATOR_ERR("%s not support write op",
						fd->device_name);
				break;
			}
		}
	} while (0);
	vfree(buffer);
	return copied_size;
}

static int debugfs_creator_read_callback(struct seq_file *m, void *v)
{
	struct file_device *fd = NULL;

	list_for_each_entry(fd, &creator_header, list) {
		struct proc_inode *node = container_of(m->file->f_inode,
				struct proc_inode, vfs_inode);

		if (fd->file_ptr == node->pde) {
			if (fd->type & FILE_TYPE_READ)
				seq_printf(m, "%s\n", fd->read_op.buffer);
			else
				CREATOR_ERR("file %s not support read op",
						fd->device_name);
			return 0;
		}
	}

	return 0;
}

static int debugfs_creator_read(struct inode *inode, struct file *file)
{
	return single_open(file, debugfs_creator_read_callback,
		inode->i_private);
}

static const struct proc_ops file_ops = {
	.proc_open = debugfs_creator_read,
	.proc_read = seq_read,
	.proc_write = debugfs_creator_write_cmd,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int debugfs_creator_create_file(struct file_device *file)
{
	int status = 0;

	if (file->device_name == NULL) {
		CREATOR_ERR("device_name NULL");
		return CREATOR_ERR_INVALID_DEVICE_NAME;
	}
	INIT_LIST_HEAD(&file->list);

	/* default support read operation */
	memset(&file->read_op, 0, sizeof(struct read_operation_data));

	if (file->type & FILE_TYPE_WRITE)
		if (file->write_op.cmd_item_cnt == 0 ||
			file->write_op.item == NULL) {
			CREATOR_ERR("write operation not linked");
			return CREATOR_ERR_INVALID_FILE_TYPE;
	}

	file->file_ptr = proc_create(file->device_name, 0660,
		file->folder_ptr, &file_ops);

	if (file->file_ptr == NULL) {
		CREATOR_ERR("create file %s fail", file->device_name);
		return CREATOR_ERR_CREATE_FS_FAIL;
	}

	if (status == CREATOR_OK)
		list_add_tail(&file->list, &creator_header);

	return status;
}


void debugfs_creator_destroy_file(struct file_device *file)
{
	struct read_operation_data *op = &file->read_op;

	list_del_init(&file->list);
	proc_remove(file->file_ptr);

	if (op->buffer) {
		vfree(op->buffer);
		memset(op, 0, sizeof(struct read_operation_data));
	}
}

/* print message to read file */
void debugfs_creator_printf_args(struct file_device *fd,
	enum PRINT_TYPE_ENUM print_type,
	const char *msg, va_list args)
{
	struct read_operation_data *buffer_info = NULL;
	char print_msg[512];
	int ret = 0;

	/* print buffer function enable ?
	 * not allow to write to memory if not enable.
	 */
	if (!(fd->type & FILE_TYPE_READ))
		return;

	buffer_info = &fd->read_op;

	/* convert msg to print_msg */
	ret = vsnprintf(print_msg, sizeof(print_msg), msg, args);
	if (ret < 0)
		return;

	if (print_type == PRINT_TYPE_TRUNC)
		buffer_info->used_size = 0;

	/* allocate buffer if not exist. */
	if (buffer_info->buffer == NULL) {
		buffer_info->buffer = vmalloc(PAGE_SIZE);
		if (buffer_info->buffer == NULL) {
			CREATOR_ERR("vmalloc buffer fail, size:%lu",
				PAGE_SIZE);
			return;
		}
		buffer_info->buffer_size = PAGE_SIZE;
		buffer_info->used_size = 0;
	}

	/* check if buffer sitll enough. */
	if (buffer_info->buffer_size - buffer_info->used_size <
		strlen(print_msg) + 1) {
		/* allocate a larger one. */
		char *new_buffer = vmalloc(
			PAGE_SIZE + buffer_info->buffer_size);

		if (new_buffer == NULL) {
			CREATOR_ERR("allocate buffer fail, size:%lu",
				PAGE_SIZE + buffer_info->buffer_size);
			return;
		}

		/* copy the old buffer to new buffer */
		memcpy(new_buffer, buffer_info->buffer,
			buffer_info->used_size);

		/* free old buffer */
		vfree(buffer_info->buffer);

		/* update the read_operation_data info */
		buffer_info->buffer = new_buffer;
		buffer_info->buffer_size =
			PAGE_SIZE + buffer_info->buffer_size;
	}

	/* write print_buffer to read_operation_data.buffer */
	buffer_info->used_size += snprintf(
		buffer_info->buffer + buffer_info->used_size,
		buffer_info->buffer_size - buffer_info->used_size - 1,
		"%s", print_msg);
}

void debugfs_creator_printf(struct file_device *fd,
	enum PRINT_TYPE_ENUM print_type,
	const char *print_msg, ...)
{
	va_list args;

	va_start(args, print_msg);
	debugfs_creator_printf_args(fd, print_type, print_msg, args);
	va_end(args);
}

void debugfd_creator_reset_file(struct file_device *fd)
{
	struct read_operation_data *buffer_info = NULL;

	buffer_info = &fd->read_op;

	buffer_info->used_size = 0;
	buffer_info->buffer_size = 0;
	buffer_info->used_size = 0;
	/* allocate buffer if not exist. */
	if (buffer_info->buffer != NULL) {
		vfree(buffer_info->buffer);
		buffer_info->buffer = NULL;
	}
}

