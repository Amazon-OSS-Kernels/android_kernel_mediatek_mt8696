// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include "imgresz_priv.h"
#include "imgresz_hal.h"

#define IMGRESZ_EP 0
#define IMGRESZ_PA2VA(x) ((x)+(imgresz_test_buffer_va - imgresz_test_buffer_pa))
#if IMGRESZ_TEE_ENABLE
KREE_SECUREMEM_HANDLE imgresz_test_sec_src_mem;
KREE_SECUREMEM_HANDLE imgresz_test_sec_dst_mem;
#endif
dma_addr_t imgresz_test_buffer_pa;
unsigned int imgresz_test_buffer_size;
unsigned long imgresz_test_buffer_va;
static struct imgresz_src_buf_info src;
static struct imgresz_dst_buf_info dst;
static enum imgresz_ticket_fun_type type;

static char dbg_buf[2048];
static char STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/imgresz\n"
	"ACTION:\n";
static int debug_init;
static struct dentry *debugfs;

int mdp_get_resz_callback(IMGRESZ_TICKET ti,
	enum imgresz_cb_state hw_state, void *privdata)
{
	pr_info("hw done %u, priv 0x%p", hw_state, privdata);
	/* Send successful event. */
	/* Don't call imgresz_ticket_put() here.*/
	return 0;
}


void imgresz_source_init_ep(unsigned char *mem,
			size_t wid, size_t hei)
{
	unsigned int h = 0;
	unsigned char *cur_line = NULL;
	unsigned char byte = 0xfe;

	for (h = 0; h < hei; h++) {
		cur_line = mem + h * 1920;
		memset(cur_line, byte--, 1920);
		if (byte == 0)
			byte = 0xfe;
	}
}

static void imgresz_prepare_buf(void)
{
	memset((void *)imgresz_test_buffer_va, 0, imgresz_test_buffer_size);
	src.ufo_ybuf_len = src.buf_width * src.buf_height;
	if (src.ufo_type >= IMGRESZ_UFO_10BIT_COMPACT &&
	    src.ufo_type <= IMGRESZ_UFO_10BIT_COMPACT_UNCOMPRESS)
		src.ufo_ybuf_len = src.ufo_ybuf_len / 4 * 5;
	src.y_buf_addr = IMGALIGN(imgresz_test_buffer_pa, 1024);
	src.cb_buf_addr = IMGALIGN(src.y_buf_addr + src.ufo_ybuf_len, 1024);

	if (imgresz_src_is_ufo(src.ufo_type) &&
	    src.ufo_type != IMGRESZ_UFO_10BIT_COMPACT_UNCOMPRESS) {
		src.ufo_ylen_buf_len = src.buf_width * src.buf_height / 256;
		src.ufo_ylen_buf =
			IMGALIGN(src.cb_buf_addr + src.ufo_ybuf_len / 2, 1024);
		src.ufo_clen_buf =
			IMGALIGN(src.ufo_ylen_buf + src.ufo_ylen_buf_len, 1024);
	}

	dst.y_buf_addr =
		IMGALIGN(imgresz_test_buffer_pa
		+ imgresz_test_buffer_size / 2, 1024);
	dst.c_buf_addr =
		IMGALIGN(dst.y_buf_addr + dst.buf_width * dst.buf_height, 1024);
	if (dst.bit10)
		dst.c_buf_addr =
		IMGALIGN(dst.y_buf_addr +
		((dst.buf_width * dst.buf_height) / 4 * 5), 1024);
}

static void imgresz_prepare_test_case(unsigned int test_case,
		struct imgresz_src_buf_info *src,
		struct imgresz_dst_buf_info *dst)
{
}

#if !IMGRESZ_EP
static int
imgresz_rw_file(uint64_t op, char *path, long buf, uint32_t size)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	ssize_t len;
	int flag = O_RDWR;

	if (op == WRITE)
		flag |= O_CREAT;
	fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(path, flag, 0644);
	if (IS_ERR(fp)) {
		logwarn("open %s error: 0x%p\n", path, fp);
		return PTR_ERR(fp);
	}
	pos = 0;
	if (op == READ)
		len = vfs_read(fp, (void *)buf, size, &pos);
	else if (op == WRITE)
		len = vfs_write(fp, (void *)buf, size, &pos);

	logwarn("%s: %s %s bytes: %zd(expect %u), buf: 0x%lx\n",
		__func__, path, (op == READ) ? "read" : "write",
		len, size, buf);
	filp_close(fp, NULL);
	set_fs(fs);

	return 0;
}

static int imgresz_write_buffer(void)
{
	int ret = 0;
	int ybuf_len = dst.buf_width * dst.buf_height;

	if (dst.dst_mode >= IMGRESZ_DST_COL_MD_420_BLK &&
	    dst.dst_mode <= IMGRESZ_DST_COL_MD_422_RS) {
		int c_buf_ratio = 1;

		if (dst.dst_mode == IMGRESZ_DST_COL_MD_420_BLK ||
		    dst.dst_mode == IMGRESZ_DST_COL_MD_420_RS)
			c_buf_ratio = 2;

		imgresz_rw_file(WRITE, "/data/dst_y.bin",
			IMGRESZ_PA2VA(dst.y_buf_addr), ybuf_len);
		imgresz_rw_file(WRITE, "/data/dst_c.bin",
			IMGRESZ_PA2VA(dst.c_buf_addr), ybuf_len/c_buf_ratio);

	} else if (dst.dst_mode >= IMGRESZ_DST_COL_MD_AYUV) {
		imgresz_rw_file(WRITE, "/data/dst.bin",
			IMGRESZ_PA2VA(dst.y_buf_addr), ybuf_len);
	}
	return ret;
}

static int imgresz_read_buffer(void)
{
	int ret = 0;

	if (src.src_mode >= IMGRESZ_SRC_COL_MD_420_BLK &&
	    src.src_mode <= IMGRESZ_SRC_COL_MD_422_RS) {
		int c_buf_ratio = 1;

		if (src.src_mode == IMGRESZ_SRC_COL_MD_420_BLK ||
		    src.src_mode == IMGRESZ_SRC_COL_MD_420_RS)
			c_buf_ratio = 2;

		imgresz_rw_file(READ, "/data/bits_y.bin",
			IMGRESZ_PA2VA(src.y_buf_addr), src.ufo_ybuf_len);
		imgresz_rw_file(READ, "/data/bits_c.bin",
			IMGRESZ_PA2VA(src.cb_buf_addr),
			src.ufo_ybuf_len/c_buf_ratio);

		if (imgresz_src_is_ufo(src.ufo_type) &&
		    src.ufo_type != IMGRESZ_UFO_10BIT_COMPACT_UNCOMPRESS) {
			imgresz_rw_file(READ, "/data/len_y.bin",
				IMGRESZ_PA2VA(src.ufo_ylen_buf),
				src.ufo_ylen_buf_len);
			imgresz_rw_file(READ, "/data/len_c.bin",
				IMGRESZ_PA2VA(src.ufo_clen_buf),
				src.ufo_ylen_buf_len/2);
		}
	} else if (src.src_mode >= IMGRESZ_SRC_COL_MD_AYUV) {
		imgresz_rw_file(READ, "/data/bits.bin",
			IMGRESZ_PA2VA(src.y_buf_addr), src.ufo_ybuf_len);
	}
	return ret;
}
#endif
static int imgresz_test(void)
{
	IMGRESZ_TICKET ti;
	int ret;

	ti = imgresz_ticket_get(type);
	if (ti < 0)
		return ret;

	imgresz_clk_on(0);
	imgresz_clk_on(1);
	ret = imgresz_set_scale_mode(ti, IMGRESZ_FRAME_SCALE);
	ret = imgresz_set_src_bufinfo(ti, &src);/* set src info */
	ret = imgresz_set_dst_bufinfo(ti, &dst);/* set src info */

	imgresz_trigger_scale_async(ti, false);

	msleep(1000);
	imgresz_clk_off(1);
	imgresz_clk_off(0);
	imgresz_ticket_put(ti);

	return ret;
}

static void imgresz_process_dbg_opt(const char *opt)
{
	int ret = 0;
	unsigned int temp = 0;

	if (strncmp(opt, "src: ", 5) == 0) {
		memset(&src, 0, sizeof(src));
		ret = sscanf(opt, "src: %dx%d %dx%d %dx%d %u,%u,%u",
			&src.buf_width, &src.buf_height,
			&src.pic_width, &src.pic_height,
			&src.pic_x_offset, &src.pic_y_offset,
			&src.src_mode, &src.ufo_type, &temp);
		src.ufo_jump = temp;
		if (ret != 9) {
			logwarn("error: cmd parsed %d param", ret);
			goto Error;
		}

	} else if (strncmp(opt, "dst: ", 5) == 0) {
		memset(&dst, 0, sizeof(dst));
		ret = sscanf(opt, "dst: %dx%d %dx%d %dx%d %u %u",
			&dst.buf_width, &dst.buf_height,
			&dst.pic_width, &dst.pic_height,
			&dst.pic_x_offset, &dst.pic_y_offset,
			&dst.dst_mode, &temp);
		dst.bit10 = temp;
		if (ret != 8) {
			logwarn("error: cmd parsed %d param", ret);
			goto Error;
		}
	} else if (strncmp(opt, "go ", 3) == 0) {
		ret = sscanf(opt, "go %u", &type);
		if (ret != 1) {
			logwarn("error: cmd parsed %d param", ret);
			goto Error;
		}
		imgresz_prepare_buf();
#if !IMGRESZ_EP
		imgresz_read_buffer();
#endif
		imgresz_test();
#if !IMGRESZ_EP
		imgresz_write_buffer();
#endif
	} else if (strncmp(opt, "test ", 4) == 0) {
		unsigned int test_case = 0;

		ret = sscanf(opt, "test %d", &test_case);
		if (ret != 1) {
			pr_info("error to parse cmd %s, ret=%d\n", opt, ret);
			goto Error;
		}
		pr_info("test_case=%d\n", test_case);
		imgresz_prepare_test_case(test_case, &src, &dst);
		imgresz_test();
	} else
		goto Error;

	return;
Error:
	pr_info("%s\n", STR_HELP);
}


static void imgresz_process_dbg_cmd(char *cmd)
{
	pr_info("cmd: %s\n", cmd);
	memset(dbg_buf, 0, sizeof(dbg_buf));
	imgresz_process_dbg_opt(cmd);
}

/* --------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------- */

static int imgresz_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static char cmd_buf[512];

static ssize_t imgresz_debug_read(struct file *file,
	char __user *ubuf, size_t count, loff_t *ppos)
{
	if (strlen(dbg_buf))
		return simple_read_from_buffer(ubuf, count,
		ppos, dbg_buf, strlen(dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count,
		ppos, STR_HELP, strlen(STR_HELP));

}

static ssize_t imgresz_debug_write(struct file *file,
	const char __user *ubuf, size_t count, loff_t *ppos)
{
	size_t debug_bufmax = sizeof(cmd_buf) - 1;

	count = min(count, debug_bufmax);
	if (copy_from_user(&cmd_buf, ubuf, count))
		return -EFAULT;
	cmd_buf[count] = 0;

	imgresz_process_dbg_cmd(cmd_buf);

	return count;
}

static const struct file_operations debug_fops = {
	.read = imgresz_debug_read,
	.write = imgresz_debug_write,
	.open = imgresz_debug_open,
};

void imgresz_debug_init(struct device *dev)
{
	if (!debug_init) {
		debug_init = 1;
		debugfs = debugfs_create_file("imgresz",
					      0444,
					      NULL, (void *)0, &debug_fops);

		pr_info("imgresz debug init, fs= %p\n", debugfs);
	}
}

void imgresz_debug_deinit(void)
{
	debugfs_remove(debugfs);
}
