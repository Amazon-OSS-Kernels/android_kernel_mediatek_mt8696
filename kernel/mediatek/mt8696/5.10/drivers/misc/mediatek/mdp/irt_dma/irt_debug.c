// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/debugfs.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>

#include "irt_if.h"

/* ------------------------------------------- */
/* Debug Options */
/* -------------------------------------------- */
static const char STR_HELP[] =
	"USAGE\n"
		"echo [ACTION]... > /d/mtkirt\n"
	"ACTION\n";

/* ---------------------------------------------- */
/* Command Processor */
/* ---------------------------------------------- */

int irt_hw_done_callback(enum irt_cb_state hw_state, void *privdata)
{
	pr_info("%s %d\n", __func__, hw_state);

	if (hw_state == IRT_CB_TIMEOUT)
		pr_info("[IRT] error framedone timeout\n");

	return 0;
}

static int irtAllocDmaBuffer(unsigned int size, bool en, struct dma_buf **dma_buf, void **va)
{
	struct dma_heap *dma_heap = NULL;
	unsigned int i;
	void *debug_va = NULL;
	int fd;

	dma_heap = dma_heap_find("mtk_mm");

	if (!dma_heap) {
		pr_info("[IRT]%s, dma heap find fail\n", __func__);
		return -1;
	}

	*dma_buf = dma_heap_buffer_alloc(dma_heap, size,
		O_RDWR | O_CLOEXEC, DMA_HEAP_VALID_HEAP_FLAGS);
	dma_heap_put(dma_heap);
	if (IS_ERR(*dma_buf)) {
		pr_info("[IRT]%s, dma buffer alloc fail\n", __func__);
		return -1;
	}

	debug_va = dma_buf_vmap(*dma_buf);

	if (!debug_va) {
		pr_info("[IRT]dma map va fail.\n");
		dma_heap_buffer_free(*dma_buf);
		return -1;
	}

	*va = debug_va;

	if (en) {
		for (i = 0; i < 4; i++) {
			memset(debug_va, (i+1) * 60, 720 * 120);
			debug_va += 720 * 120;
		}
	}

	fd = dma_buf_fd(*dma_buf, O_RDWR | O_CLOEXEC);

	//dma_buf_vunmap(dma_buf, debug_va); /* need confirm with ion owner */

	return fd;
}

void irtStartTest(void)
{
	struct irt_dma_info irt_info;
	static struct dma_buf *s_dma_buf;
	static struct dma_buf *d_dma_buf;
	unsigned int debug_size = 1920 * 1088 * 2;
	int src_fd, dst_fd;
	void *src_va = NULL;
	void *dst_va = NULL;

	memset((void *)&irt_info, 0x0, sizeof(irt_info));

	src_fd = irtAllocDmaBuffer(debug_size, true, &s_dma_buf, &src_va);
	if (src_fd != -1) {
		dst_fd = irtAllocDmaBuffer(debug_size, false, &d_dma_buf, &dst_va);
		if (dst_fd == -1) {
			pr_info("[IRT] dst dma buf alloc fail\n");
			if (s_dma_buf) {
				if (src_va)
					dma_buf_vunmap(s_dma_buf, src_va);

				dma_heap_buffer_free(s_dma_buf);
			}

			if (d_dma_buf) {
				if (dst_va)
					dma_buf_vunmap(d_dma_buf, dst_va);

				dma_heap_buffer_free(d_dma_buf);
			}
			return;
		}

	} else {
		pr_info("[IRT]src dma buf alloc fail\n");
		if (s_dma_buf) {
			if (src_va)
				dma_buf_vunmap(s_dma_buf, src_va);

			dma_heap_buffer_free(s_dma_buf);
		}
		return;
	}

	irt_info.rotate_mode = IRT_DMA_MODE_ROTATE_90;
	irt_info.dither_mode = 0;
	irt_info.src_color_fmt = IRT_DMA_SRC_COL_MD_YC420_8BIT_SCL;
	irt_info.dst_color_fmt = IRT_DMA_DST_COL_MD_YC420_8BIT_SCL;

	irt_info.src_width_align = 720;
	irt_info.src_height_align = 480;

	irt_info.src_fd = src_fd;
	irt_info.dst_fd = dst_fd;

	irt_info.src_offset_y_len = 0;
	irt_info.src_offset_c_len = 1920 * 1088;
	irt_info.dst_offset_y_len = 0;
	irt_info.dst_offset_c_len = 1920 * 1088;

	irt_ticket_get();

	irt_dma_trigger_sync(&irt_info);

	irt_ticket_put();

	if (s_dma_buf) {
		if (src_va)
			dma_buf_vunmap(s_dma_buf, src_va);

		dma_heap_buffer_free(s_dma_buf);
	}

	if (d_dma_buf) {
		if (dst_va)
			dma_buf_vunmap(d_dma_buf, dst_va);

		dma_heap_buffer_free(d_dma_buf);
	}

	pr_info("[IRT] %s end\n", __func__);
}

static void process_dbg_opt(const char *opt)
{
	pr_info("[IRT] %s\n", __func__);

	if (strncmp(opt, "logen:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int logen = 0;

		if (kstrtouint(p, 16, (unsigned int *)&logen))
			goto error;

		irt_set_log_enable(logen);

		pr_info("[IRT] log level is %d\n", logen);
	} else if (strncmp(opt, "irttest", 7) == 0) {
		irtStartTest();
	} else
		goto error;

	return;

error:
	pr_info("[IRT]Parse command error!\n\n%s", STR_HELP);
}

static void process_dbg_cmd(char *cmd)
{
	char *tok;

	pr_info("[mtkirt_dbg] %s\n", cmd);
	while ((tok = strsep(&cmd, " ")) != NULL)
		process_dbg_opt(tok);
}

static int irt_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t irt_debug_read(struct file *file,
	char __user *ubuf, size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(ubuf,
		count, ppos, STR_HELP, strlen(STR_HELP));
}

static ssize_t irt_debug_write(struct file *file, const char __user *ubuf,
			       size_t count, loff_t *ppos)
{
	static char irt_cmd_buf[512];
	const int debug_bufmax = sizeof(irt_cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&irt_cmd_buf, ubuf, count))
		return -EFAULT;

	irt_cmd_buf[count] = 0;

	process_dbg_cmd(irt_cmd_buf);

	return ret;
}

struct dentry *mtkirt_dbg;
static const struct file_operations debug_fops = {
	.read = irt_debug_read,
	.write = irt_debug_write,
	.open = irt_debug_open,
};

static int __init mtk_irt_debug_init(void)
{
	/*pr_info("[IRT] %s\n", __func__);*/
	mtkirt_dbg = debugfs_create_file("mtkirt", 0644,
					NULL, (void *)0, &debug_fops);

	return 0;
}

static void __exit mtk_irt_debug_deinit(void)
{
	debugfs_remove(mtkirt_dbg);
}
module_init(mtk_irt_debug_init);
module_exit(mtk_irt_debug_deinit);
