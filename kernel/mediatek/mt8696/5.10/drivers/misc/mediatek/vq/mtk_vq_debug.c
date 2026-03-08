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
#include <linux/fs.h>
#include <linux/uaccess.h>

#if IS_ENABLED(CONFIG_MTK_NR)
#include "nr_hal.h"
#endif
#include "di_hal.h"

#include "mtk_vq_mgr.h"
#include "vq_def.h"

#ifndef STR_CONVERT
#define STR_CONVERT(p, val, base, action)                 \
	do {                                                  \
		int ret = 0;                                      \
		const char *tmp;                                  \
		tmp = strsep(p, ",");                             \
		if (tmp == NULL)                                  \
			break;                                        \
		if (strcmp(#base, "int") == 0)                    \
			ret = kstrtoint(tmp, 0, (int *)val);          \
		else if (strcmp(#base, "uint") == 0)              \
			ret = kstrtouint(tmp, 0, (unsigned int *)val);\
		else if (strcmp(#base, "ul") == 0)                \
			ret = kstrtoul(tmp, 0, (unsigned long *)val); \
		if (ret != 0) {                                   \
			pr_info(                                   \
	"[ERROR]kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n", \
				ret, __FILE__, __LINE__); \
			action; \
			} \
	} while (0)
#endif

/* ------------------------------------------- */
/* Debug Options */
/* -------------------------------------------- */
static const char STR_HELP[] =
	"USAGE\n"
	"echo [ACTION]... > mtkvq\n"
	"ACTION:\n"
	"logen:level,en\n"
	"nrtest\n"
	"di\n";

/* ---------------------------------------------- */
/* Command Processor */
/* ---------------------------------------------- */

void vq_write_file(char *file_name, char *buff, uint32_t len)
{
	mm_segment_t fs;
	struct file *fp = NULL;
	ssize_t file_size;
	int ret;

	VQ_INFO("write %s buff 0x%p len %d\n", file_name, buff, len);

	fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_name,
		O_RDWR | O_CREAT | O_TRUNC, 0644);

	if (IS_ERR(fp)) {
		VQ_INFO("open file %s fail\n", file_name);
		set_fs(fs);
		return;
	}

	/* write data */
	file_size = vfs_write(fp, buff, len, &fp->f_pos);

	ret = filp_close(fp, NULL);

	VQ_INFO("write %s len %d %ld %d\n",
		file_name, len, file_size, ret);

	set_fs(fs);
}

void vq_read_file(char *file_name, char *buff, uint32_t len)
{
	mm_segment_t fs;
	struct file *fp = NULL;
	ssize_t file_size;
	int ret;

	VQ_INFO("read %s buff 0x%p len %d\n", file_name, buff, len);

	fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_name, O_RDONLY, 0x0);

	if (IS_ERR(fp)) {
		VQ_INFO("open file %s fail\n", file_name);
		set_fs(fs);
		return;
	}

	/* read date */
	file_size = vfs_read(fp, buff, len, &fp->f_pos);

	ret = filp_close(fp, NULL);

	VQ_INFO("read %s len %d %ld %d done\n",
		file_name, len, file_size, ret);

	set_fs(fs);
}

static int vqAllocDmaBuffer(unsigned int size, bool en, struct dma_buf **dma_buf, void **va)
{
	struct dma_heap *dma_heap = NULL;
	unsigned int i;
	void *debug_va = NULL;
	int fd;

	dma_heap = dma_heap_find("mtk_mm");

	if (!dma_heap) {
		pr_info("[VQ]%s, dma heap find fail\n", __func__);
		return -1;
	}

	*dma_buf = dma_heap_buffer_alloc(dma_heap, size,
		O_RDWR | O_CLOEXEC, DMA_HEAP_VALID_HEAP_FLAGS);
	dma_heap_put(dma_heap);
	if (IS_ERR(*dma_buf)) {
		pr_info("[VQ]%s, dma buffer alloc fail\n", __func__);
		return -1;
	}

	debug_va = dma_buf_vmap(*dma_buf);

	if (!debug_va) {
		pr_info("[VQ]dma map va fail.\n");
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

void nrStartTest(void)
{
	struct mtk_vq_config config;
	struct vq_data *data = mtk_vq_get_data();
	unsigned int debug_size = 1920 * 1088 * 2;
	static struct dma_buf *s_dma_buf;
	static struct dma_buf *d_dma_buf;
	void *src_va = NULL;
	void *dst_va = NULL;
	int src_fd, dst_fd;

	memset((void *)&config, 0, sizeof(config));

	src_fd = vqAllocDmaBuffer(debug_size, true, &s_dma_buf, &src_va);
	if (src_fd != -1) {
		dst_fd = vqAllocDmaBuffer(debug_size, false, &d_dma_buf, &dst_va);
		if (dst_fd == -1) {
			pr_info("[VQ]dst dma buf alloc fail\n");
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
		pr_info("[VQ]src dma buf alloc fail\n");
		if (s_dma_buf) {
			if (src_va)
				dma_buf_vunmap(s_dma_buf, src_va);

			dma_heap_buffer_free(s_dma_buf);
		}
		return;
	}

	config.dst_fmt = VQ_COLOR_FMT_420BLK;
	config.src_fmt = VQ_COLOR_FMT_420BLK;

	config.src_fd[0] = src_fd;
	config.dst_fd = dst_fd;

	config.bnr_level = 3;
	config.mnr_level = 3;
	config.vq_mode = VQ_NR_STANDALONE;

	config.src_width = 720;
	config.src_height = 480;
	config.src_align_width = 720;
	config.src_align_height = 480;

	config.src_ofset_y_len[0] = 0;
	config.src_ofset_c_len[0] = 1920 * 1088;

	config.dst_ofset_y_len = 0;
	config.dst_ofset_c_len = 1920 * 1088;

	config.vq_mode = VQ_NR_STANDALONE;

	mtk_vq_mgr_set_input_buffer(data, &config);

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

	pr_info("[NR] %s end\n", __func__);
}

/* di stand alone ut test */
void diTest(const char *opt)
{
	unsigned int vq_mode = 0;
	unsigned int di_mode = 0;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int h265_enable = 0;
	int i = 0;
	unsigned int top_first = 0;
	unsigned int top_current = 0;
	int ret = 0;
	struct mtk_vq_config_info vq_config_info;
	struct mtk_vq_config config;
	u32 buf_offset = 1920 * 1088;
	unsigned int debug_size = 1920 * 1088 * 2;
	struct vq_data *data = mtk_vq_get_data();
	static struct dma_buf *s_dma_buf;
	static struct dma_buf *d_dma_buf;
	int src_fd, dst_fd;
	void *src_va = NULL;
	void *dst_va = NULL;

	char in_y_file[100] =
		"/data/vq/720x480/OK/pic_0_Y.out";
	char in_c_file[100] =
		"/data/vq/720x480/OK/pic_0_CbCr.out";

	char out_y_file[100] =
		"/data/vq/720x480/OK/wrch_y.out";
	char out_c_file[100] =
		"/data/vq/720x480/OK/wrch_c.out";
	uint32_t len = 720*480;

	char *p = (char *)opt + 3;

	VQ_INFO("di ut test\n");
	ret =
	    sscanf(p, "%d %d %d %d %d %d %d",
		   &vq_mode,
		   &di_mode,
		   &width,
		   &height,
		   &h265_enable,
		   &top_first,
		   &top_current);
	if (ret != 7 && ret != 6) {
		VQ_INFO
		    ("vq %d di %d %dx%d h265 %d tff %d f %d\n",
		     vq_mode, di_mode, width, height, h265_enable, top_first,
		     top_current);
		pr_info("error to parse cmd %s, ret=%d\n", opt, ret);
		goto error;
	}
	VQ_INFO("vq %d di %d %dx%d h265 %d tff %d f %d\n",
		vq_mode, di_mode, width, height, h265_enable, top_first,
		top_current);

	memset(&config, 0, sizeof(struct mtk_vq_config));
	memset(&vq_config_info, 0, sizeof(struct mtk_vq_config_info));

	config.vq_mode = vq_mode;
	config.di_mode = di_mode;
	config.h265_enable = h265_enable;
	config.src_width = width;
	config.src_height = height;
	config.topfield_first_enable = top_first;
	config.cur_field = top_current;
	config.src_align_width = width;
	config.src_align_height = height;

	for (i = 0; i < MTK_VQ_BUFFER_COUINT; ++i) {
		config.src_ofset_y_len[i] = 0;
		config.src_ofset_c_len[i] = buf_offset;
	}

	config.bnr_level = 0;
	config.mnr_level = 0;
	config.dst_ofset_y_len = 0;
	config.dst_ofset_c_len = buf_offset;

	config.src_fmt = VQ_COLOR_FMT_420BLK;
	config.dst_fmt = VQ_COLOR_FMT_422SCL;

	src_fd = vqAllocDmaBuffer(debug_size, true, &s_dma_buf, &src_va);
	if (src_fd != -1) {
		dst_fd = vqAllocDmaBuffer(debug_size, false, &d_dma_buf, &dst_va);
		if (dst_fd == -1) {
			if (s_dma_buf) {
				if (src_va)
					dma_buf_vunmap(s_dma_buf, src_va);

				dma_heap_buffer_free(s_dma_buf);
			}

			pr_info("[VQ]dst dma buf alloc fail\n");
			if (d_dma_buf) {
				if (dst_va)
					dma_buf_vunmap(d_dma_buf, dst_va);

				dma_heap_buffer_free(d_dma_buf);
			}
			return;
		}
	} else {
		pr_info("[VQ]src dma buf alloc fail\n");
		if (s_dma_buf) {
			if (src_va)
				dma_buf_vunmap(s_dma_buf, src_va);

			dma_heap_buffer_free(s_dma_buf);
		}
		return;
	}

	config.dst_fd = dst_fd;

	for (i = 0; i < MTK_VQ_DI_INPUTBUFFER; ++i)
		config.src_fd[i] = src_fd;

	vq_read_file(in_y_file, (char *)src_va, len);
	vq_read_file(in_c_file, (char *)(src_va + buf_offset), len);

	vq_config_info.vq_config = &config;

	ret = mtk_vq_mgr_prepare_buffer(data, &vq_config_info);

	mtk_vq_power_on(data, VQ_DI_STANDALONE);
	//mtk_vq_mgr_set_input_buffer(data, &config);
	ret = di_hal_config(&vq_config_info);

	//mtk_vq_power_off(mtk_vq_get_data(), VQ_DI_STANDALONE);

	vq_write_file(out_y_file, (char *)dst_va, len);
	vq_write_file(out_c_file, (char *)(dst_va + buf_offset), len);

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
error:
	VQ_ERR("Parse command error %d !\n\n%s", __LINE__, STR_HELP);

}

static void process_dbg_opt(const char *opt)
{
	VQ_INFO("\n");

	if (strncmp(opt, "logen:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int level = 0;
		unsigned int logen = 0;

		if (kstrtouint(p, 16, (unsigned int *)&level))
			goto error;

		if (kstrtouint(p, 16, (unsigned int *)&logen))
			goto error;

		mtk_vq_set_log_enable(level, logen);

		pr_info("[VQ] set level %d enable %d\n", level, logen);
	} else if (strncmp(opt, "nrtest", 6) == 0) {
		nrStartTest();
	} else if (strncmp(opt, "di", 2) == 0) {
		diTest(opt);
	} else
		goto error;

	return;

error:
	mtk_vq_get_log_help();
	pr_info("[VQ] Parse command error %d!\n\n%s", __LINE__, STR_HELP);
}

static void process_dbg_cmd(char *cmd)
{
	char *tok;

	VQ_INFO("[mtkvq_dbg] %s\n", cmd);
	while ((tok = strsep(&cmd, "&&")) != NULL)
		process_dbg_opt(tok);
}

static int vq_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t vq_debug_read(struct file *file, char __user *ubuf,
			     size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(ubuf, count, ppos, STR_HELP,
				       strlen(STR_HELP));
}

static ssize_t vq_debug_write(struct file *file, const char __user *ubuf,
			      size_t count, loff_t *ppos)
{
	static char vq_cmd_buf[512];
	const int debug_bufmax = sizeof(vq_cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&vq_cmd_buf, ubuf, count))
		return -EFAULT;

	vq_cmd_buf[count] = 0;

	process_dbg_cmd(vq_cmd_buf);

	return ret;
}

struct dentry *mtkvq_dbg;
static const struct file_operations debug_fops = {
	.read = vq_debug_read,
	.write = vq_debug_write,
	.open = vq_debug_open,
};

static int __init mtk_vq_debug_init(void)
{
	VQ_INFO("\n");
	mtkvq_dbg = debugfs_create_file("mtkvq", S_IFREG | 0444, NULL,
					(void *)0,
					&debug_fops);

	VQ_INFO("mtkvq_dbg %p\n", mtkvq_dbg);

	return 0;
}

static void __exit mtk_vq_debug_deinit(void)
{
	debugfs_remove(mtkvq_dbg);
}

module_init(mtk_vq_debug_init);
module_exit(mtk_vq_debug_deinit);
