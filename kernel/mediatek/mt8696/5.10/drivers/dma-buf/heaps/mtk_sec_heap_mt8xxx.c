// SPDX-License-Identifier: GPL-2.0
/*
 * DMABUF mtk_sec heap exporter
 *
 * Copyright (C) 2021 MediaTek Inc.
 *
 */

#define pr_fmt(fmt) "dma_heap: mtk_sec "fmt

#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/dma-heap.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "mtk_heap_priv.h"
#include "mtk_heap.h"

struct mtk_sec_heap_buffer {
	struct dma_heap		*heap;
	struct list_head	attachments;
	bool			uncached;
	struct mutex		lock;
	/* struct sg_table	table; */
	unsigned long		len;
	pid_t			pid;
	pid_t			tid;
	char			pid_name[TASK_COMM_LEN];
	char			tid_name[TASK_COMM_LEN];
	u32			sec_handle;
};

#define SEC_HEAP_FLAG_UNCACHED		(1 << 0)
#define SEC_HEAP_FLAG_ZALLOC		(1 << 1)
#define SEC_HEAP_FLAG_INITED		(1 << 31)

struct mtk_sec_heap {
	const char		*heap_name;
	struct dma_heap		*heap;
	unsigned int		align;
	unsigned int		flags;
	int (*allocate)(struct mtk_sec_heap *sec_heap,
			struct mtk_sec_heap_buffer *sec_buf);
	void (*free)(struct mtk_sec_heap *sec_heap,
		     struct mtk_sec_heap_buffer *sec_buf);
	atomic64_t		total_size;
};

#if defined(MTK_IN_HOUSE_SEC_HEAP_SUPPORT)
#include "tz_cross/trustzone.h"
#include "tz_cross/ta_mem.h"
#include "kree/system.h"
#include "kree/mem.h"

static KREE_SESSION_HANDLE
mtk_kree_session_handle(struct mtk_sec_heap *sec_heap)
{
	static KREE_SESSION_HANDLE ta_mem_session;
	int ret;

	if (!(sec_heap->flags & SEC_HEAP_FLAG_INITED)) {
		ret = KREE_CreateSession(TZ_TA_MEM_UUID, &ta_mem_session);
		if (ret != TZ_RESULT_SUCCESS) {
			ta_mem_session = KREE_SESSION_HANDLE_NULL;
			pr_err("KREE_CreateSession fail, ret=%d\n", ret);
		} else {
			sec_heap->flags |= SEC_HEAP_FLAG_INITED;
		}
	}

	return ta_mem_session;
}

static int mtk_kree_buffer_allocate(struct mtk_sec_heap *sec_heap,
				    struct mtk_sec_heap_buffer *sec_buf)
{
	u32 sec_handle = 0;
	int ret;

	if (sec_heap->flags & SEC_HEAP_FLAG_ZALLOC)
		ret = KREE_ZallocSecurechunkmemWithTag(
			mtk_kree_session_handle(sec_heap), &sec_handle,
			sec_heap->align, sec_buf->len, sec_heap->heap_name);
	else
		ret = KREE_AllocSecurechunkmemWithTag(
			mtk_kree_session_handle(sec_heap), &sec_handle,
			sec_heap->align, sec_buf->len, sec_heap->heap_name);

	if (ret != TZ_RESULT_SUCCESS) {
		pr_err("KREE_AllocSecurechunkmemWithTag failed, ret %d\n",
		       ret);
		return -ENOMEM;
	}

	sec_buf->sec_handle = sec_handle;

	return 0;
}

static void mtk_kree_buffer_free(struct mtk_sec_heap *sec_heap,
				 struct mtk_sec_heap_buffer *sec_buf)
{
	int ret = KREE_UnreferenceSecurechunkmem(
			mtk_kree_session_handle(sec_heap),
			sec_buf->sec_handle);

	if (ret != TZ_RESULT_SUCCESS)
		pr_err("KREE_UnreferenceSecurechunkmem failed, ret %d\n", ret);
}
#endif

static struct mtk_sec_heap mtk_sec_heap_info = {
	.heap_name = "mtk_svp",
	.align = SZ_4K,
	/* A weak UNCACHED flag in here for dma_buf attachment recording */
	.flags = (SEC_HEAP_FLAG_UNCACHED | SEC_HEAP_FLAG_ZALLOC),
#if defined(MTK_IN_HOUSE_SEC_HEAP_SUPPORT)
	.allocate = mtk_kree_buffer_allocate,
	.free = mtk_kree_buffer_free,
#endif
};

static inline struct mtk_sec_heap *mtk_sec_heap_get(const struct dma_heap *heap)
{
	if (heap == mtk_sec_heap_info.heap)
		return &mtk_sec_heap_info;

	return NULL;
}

static const struct dma_buf_ops mtk_sec_heap_buf_ops;

static struct dma_buf *alloc_dmabuf(struct mtk_sec_heap_buffer *sec_buf,
				    unsigned long fd_flags)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(sec_buf->heap);
	exp_info.ops = &mtk_sec_heap_buf_ops;
	exp_info.size = sec_buf->len;
	exp_info.flags = fd_flags;
	exp_info.priv = sec_buf;

	return dma_buf_export(&exp_info);
}

static struct dma_buf *mtk_sec_heap_allocate(struct dma_heap *heap,
					     unsigned long len,
					     unsigned long fd_flags,
					     unsigned long heap_flags)
{
	struct dma_buf *dmabuf;
	struct mtk_sec_heap_buffer *sec_buf;
	struct mtk_sec_heap *sec_heap = mtk_sec_heap_get(heap);
	struct task_struct *task = current->group_leader;
	int ret;

	if (!sec_heap || !sec_heap->allocate) {
		pr_err("%s: heap <%s> can not do allcate\n", __func__,
		       (sec_heap) ? sec_heap->heap_name : "null");
		return ERR_PTR(-EFAULT);
	}

	pr_debug("%s: %s: size:0x%lx\n", __func__, sec_heap->heap_name, len);

	sec_buf = kzalloc(sizeof(*sec_buf), GFP_KERNEL);
	if (!sec_buf)
		return ERR_PTR(-ENOMEM);

	sec_buf->len = len;
	sec_buf->heap = heap;
	sec_buf->uncached = !!(sec_heap->flags & SEC_HEAP_FLAG_UNCACHED);

	ret = sec_heap->allocate(sec_heap, sec_buf);
	if (ret)
		goto free_buf;

	/* ret = sg_alloc_table(sec_buf->table, 1, GFP_KERNEL);
	 * if (ret)
	 *	return ERR_PTR(ret);
	 * sg_set_page(sec_buf->table->sgl, 0, sec_buf->len, 0);
	 */

	INIT_LIST_HEAD(&sec_buf->attachments);
	mutex_init(&sec_buf->lock);

	sec_buf->pid = task_pid_nr(task);
	sec_buf->tid = task_pid_nr(current);
	get_task_comm(sec_buf->pid_name, task);
	get_task_comm(sec_buf->tid_name, current);

	dmabuf = alloc_dmabuf(sec_buf, fd_flags);
	if (IS_ERR(dmabuf)) {
		pr_err("%s: alloc_dmabuf fail\n", __func__);
		ret = PTR_ERR(dmabuf);
		goto free_sec_mem;
	}

	atomic64_add(sec_buf->len, &sec_heap->total_size);

	pr_debug("%s: %s: size:0x%lx(total_size:0x%lx), sec_hdl:%u (%s)\n",
		 __func__, sec_heap->heap_name, sec_buf->len,
		 atomic64_read(&sec_heap->total_size),
		 sec_buf->sec_handle, sec_buf->tid_name);

	return dmabuf;

free_sec_mem:
	if (sec_heap->free)
		sec_heap->free(sec_heap, sec_buf);
free_buf:
	kfree(sec_buf);

	return ERR_PTR(ret);
}

static void mtk_sec_heap_free(struct dma_buf *dmabuf)
{
	struct mtk_sec_heap_buffer *sec_buf = dmabuf->priv;
	struct mtk_sec_heap *sec_heap = mtk_sec_heap_get(sec_buf->heap);

	if (!sec_heap) {
		pr_err("%s: heap <%s> can not do free\n", __func__,
		       (sec_heap) ? sec_heap->heap_name : "null");
		return;
	}

	if (atomic64_sub_return(sec_buf->len, &sec_heap->total_size) < 0)
		pr_warn("%s: %s: memory overflow 0x%lx\n", __func__,
			sec_heap->heap_name,
			atomic64_read(&sec_heap->total_size));

	pr_debug("%s: %s: size:0x%lx, sec_hdl:0x%x\n",
		 __func__, sec_heap->heap_name, sec_buf->len,
		 sec_buf->sec_handle);

	if (sec_heap->free)
		sec_heap->free(sec_heap, sec_buf);

	kfree(sec_buf);
}

static int mtk_sec_heap_attach(struct dma_buf *dmabuf,
			       struct dma_buf_attachment *attachment)
{
	struct mtk_sec_heap_buffer *sec_buf = dmabuf->priv;
	struct dma_heap_attachment *a;
	struct sg_table *table;
	int ret = 0;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table) {
		ret = -ENOMEM;
		goto free_attach;
	}

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto free_sgt;
	sg_set_page(table->sgl, 0, sec_buf->len, 0);

	a->table = table;
	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);
	a->mapped = false;
	a->uncached = sec_buf->uncached;
	attachment->priv = a;

	mutex_lock(&sec_buf->lock);
	list_add(&a->list, &sec_buf->attachments);
	mutex_unlock(&sec_buf->lock);

	return 0;

free_sgt:
	kfree(table);
free_attach:
	kfree(a);

	return ret;
}

static void mtk_sec_heap_detach(struct dma_buf *dmabuf,
				struct dma_buf_attachment *attachment)
{
	struct mtk_sec_heap_buffer *sec_buf = dmabuf->priv;
	struct dma_heap_attachment *a = attachment->priv;

	mutex_lock(&sec_buf->lock);
	list_del(&a->list);
	mutex_unlock(&sec_buf->lock);

	sg_free_table(a->table);
	kfree(a->table);
	kfree(a);
}

static struct sg_table *
mtk_sec_heap_map_dma_buf(struct dma_buf_attachment *attachment,
			 enum dma_data_direction direction)
{
	struct dma_buf *dmabuf = attachment->dmabuf;
	struct mtk_sec_heap_buffer *sec_buf = dmabuf->priv;
	struct dma_heap_attachment *a = attachment->priv;
	struct sg_table *table = a->table;

	sg_dma_address(table->sgl) = sec_buf->sec_handle;
	/* sg_dma_len(table->sgl) = sec_buf->len; */
	a->mapped = true;

	return table;
}

static void mtk_sec_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				       struct sg_table *table,
				       enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;

	a->mapped = false;
	sg_dma_address(table->sgl) = 0;

	WARN_ON(a->table != table);
}

static int mtk_sec_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
					enum dma_data_direction direction)
{

	pr_debug("%s: fake ops\n", __func__);

	return -EFAULT;
}

static int mtk_sec_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
					enum dma_data_direction direction)
{

	pr_debug("%s: fake ops\n", __func__);

	return -EFAULT;
}

static int mtk_sec_heap_dma_buf_get_flags(struct dma_buf *dmabuf,
					  unsigned long *flags)
{
	struct mtk_sec_heap_buffer *sec_buf = dmabuf->priv;

	*flags = sec_buf->uncached;

	return 0;
}

static const struct dma_heap_ops mtk_sec_heap_ops = {
	.allocate = mtk_sec_heap_allocate,
};

static const struct dma_buf_ops mtk_sec_heap_buf_ops = {
	/* one attachment can only map once */
	.cache_sgt_mapping = 1,
	.attach = mtk_sec_heap_attach,
	.detach = mtk_sec_heap_detach,
	.map_dma_buf = mtk_sec_heap_map_dma_buf,
	.unmap_dma_buf = mtk_sec_heap_unmap_dma_buf,
	.begin_cpu_access = mtk_sec_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = mtk_sec_heap_dma_buf_end_cpu_access,
	.release = mtk_sec_heap_free,
	.get_flags = mtk_sec_heap_dma_buf_get_flags,
};

static int mtk_sec_heap_buf_priv_dump(const struct dma_buf *dmabuf,
				      struct dma_heap *heap,
				      void *priv)
{
	struct mtk_sec_heap_buffer *sec_buf = dmabuf->priv;
	struct seq_file *s = priv;

	if ((heap != sec_buf->heap) || !is_mtk_sec_heap_dmabuf(dmabuf))
		return -EINVAL;

	dmabuf_dump(s, "\t\tbuf_priv: uncache:%d alloc-pid:%d[%s]-tid:%d[%s]\n",
		    !!sec_buf->uncached,
		    sec_buf->pid, sec_buf->pid_name,
		    sec_buf->tid, sec_buf->tid_name);

	dmabuf_dump(s, "\t\tbuf_priv: heap[%s] sec_hdl:%u size:%lu\n",
		    dma_heap_get_name(sec_buf->heap), sec_buf->sec_handle,
		    sec_buf->len);

	return 0;
}

static struct mtk_heap_priv_info mtk_sec_heap_priv = {
	.buf_priv_dump = mtk_sec_heap_buf_priv_dump,
};

int is_mtk_sec_heap_dmabuf(const struct dma_buf *dmabuf)
{
	if (!dmabuf || (dmabuf->ops != &mtk_sec_heap_buf_ops))
		return 0;

	return 1;
}
EXPORT_SYMBOL_GPL(is_mtk_sec_heap_dmabuf);

u32 dmabuf_to_secure_handle(const struct dma_buf *dmabuf)
{
	struct mtk_sec_heap_buffer *sec_buf;

	if (!is_mtk_sec_heap_dmabuf(dmabuf)) {
		pr_err("%s: dmabuf is not secure\n", __func__);
		return 0;
	}
	sec_buf = dmabuf->priv;

	pr_debug("%s: sec_handle:%u\n", __func__, sec_buf->sec_handle);

	return sec_buf->sec_handle;
}
EXPORT_SYMBOL_GPL(dmabuf_to_secure_handle);

static int mtk_sec_heap_init(void)
{
	struct dma_heap_export_info exp_info;

	exp_info.ops = &mtk_sec_heap_ops;
	exp_info.priv = (void *)&mtk_sec_heap_priv;

	exp_info.name = mtk_sec_heap_info.heap_name;
	mtk_sec_heap_info.heap = dma_heap_add(&exp_info);
	if (IS_ERR(mtk_sec_heap_info.heap))
		return PTR_ERR(mtk_sec_heap_info.heap);

	pr_info("%s add heap[%s] success\n", __func__, exp_info.name);

	return 0;
}

module_init(mtk_sec_heap_init);

MODULE_DESCRIPTION("MediaTek DMA-SVP-HEAP Driver");
MODULE_LICENSE("GPL v2");
