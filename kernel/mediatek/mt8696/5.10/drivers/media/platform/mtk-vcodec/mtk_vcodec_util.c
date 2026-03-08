// SPDX-License-Identifier: GPL-2.0
/*
* Copyright (c) 2016 MediaTek Inc.
* Author: PC Chen <pc.chen@mediatek.com>
*       Tiffany Lin <tiffany.lin@mediatek.com>
*
*/

#include <linux/module.h>
#include <linux/module.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>

#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_util.h"
#include "mtk_vcu.h"
#include "mtk_vcodec_dec.h"
#include "mtk_vcodec_enc.h"

/* For encoder, this will enable logs in venc/*/
bool mtk_vcodec_dbg;
EXPORT_SYMBOL(mtk_vcodec_dbg);

/* For vcodec performance measure */
bool mtk_vcodec_perf;
EXPORT_SYMBOL(mtk_vcodec_perf);

/* The log level of v4l2 encoder or decoder driver.
 * That is, files under mtk-vcodec/.
 */
int mtk_v4l2_dbg_level;
EXPORT_SYMBOL(mtk_v4l2_dbg_level);

void __iomem *mtk_vcodec_get_dec_reg_addr(struct mtk_vcodec_ctx *data,
	unsigned int reg_idx)
{
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)data;

	if (!data || reg_idx >= NUM_MAX_VDEC_REG_BASE) {
		mtk_v4l2_err("Invalid arguments, reg_idx=%d", reg_idx);
		return NULL;
	}
	return ctx->dev->dec_reg_base[reg_idx];
}
EXPORT_SYMBOL(mtk_vcodec_get_dec_reg_addr);

void __iomem *mtk_vcodec_get_enc_reg_addr(struct mtk_vcodec_ctx *data,
	unsigned int reg_idx)
{
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)data;

	if (!data || reg_idx >= NUM_MAX_VENC_REG_BASE) {
		mtk_v4l2_err("Invalid arguments, reg_idx=%d", reg_idx);
		return NULL;
	}
	return ctx->dev->enc_reg_base[reg_idx];
}
EXPORT_SYMBOL(mtk_vcodec_get_enc_reg_addr);


int mtk_vcodec_mem_alloc(struct mtk_vcodec_ctx *data,
						 struct mtk_vcodec_mem *mem)
{
	unsigned long size = mem->size;
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)data;
	struct device *dev = &ctx->dev->plat_dev->dev;

	mem->va = dma_alloc_coherent(dev, size, &mem->dma_addr, GFP_KERNEL);

	if (!mem->va) {
		mtk_v4l2_err("%s dma_alloc size=%ld failed!", dev_name(dev),
					 size);
		return -ENOMEM;
	}

	memset(mem->va, 0, size);

	mtk_v4l2_debug(4, "[%d]  - va      = %p", ctx->id, mem->va);
	mtk_v4l2_debug(4, "[%d]  - dma     = 0x%lx", ctx->id,
				   (unsigned long)mem->dma_addr);
	mtk_v4l2_debug(4, "[%d]    size = 0x%lx", ctx->id, size);

	return 0;
}
EXPORT_SYMBOL(mtk_vcodec_mem_alloc);

void mtk_vcodec_mem_free(struct mtk_vcodec_ctx *data,
						 struct mtk_vcodec_mem *mem)
{
	unsigned long size = mem->size;
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)data;
	struct device *dev = &ctx->dev->plat_dev->dev;

	if (!mem->va) {
		mtk_v4l2_err("%s dma_free size=%ld failed!", dev_name(dev),
					 size);
		return;
	}

	mtk_v4l2_debug(4, "[%d]  - va      = %p", ctx->id, mem->va);
	mtk_v4l2_debug(4, "[%d]  - dma     = 0x%lx", ctx->id,
				   (unsigned long)mem->dma_addr);
	mtk_v4l2_debug(4, "[%d]    size = 0x%lx", ctx->id, size);

	dma_free_coherent(dev, size, mem->va, mem->dma_addr);
	mem->va = NULL;
	mem->dma_addr = 0;
	mem->size = 0;
}
EXPORT_SYMBOL(mtk_vcodec_mem_free);

void mtk_vcodec_set_curr_ctx(struct mtk_vcodec_dev *dev,
	struct mtk_vcodec_ctx *ctx, int hw_id)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->irqlock, flags);
	dev->curr_dec_ctx[hw_id] = ctx;
	spin_unlock_irqrestore(&dev->irqlock, flags);
}
EXPORT_SYMBOL(mtk_vcodec_set_curr_ctx);

struct mtk_vcodec_ctx *mtk_vcodec_get_curr_ctx(struct mtk_vcodec_dev *dev,
	int hw_id)
{
	unsigned long flags;
	struct mtk_vcodec_ctx *ctx;

	spin_lock_irqsave(&dev->irqlock, flags);
	ctx = dev->curr_dec_ctx[hw_id];
	spin_unlock_irqrestore(&dev->irqlock, flags);
	return ctx;
}
EXPORT_SYMBOL(mtk_vcodec_get_curr_ctx);

void mtk_vcodec_add_ctx_list(struct mtk_vcodec_ctx *ctx)
{
	mutex_lock(&ctx->dev->ctx_mutex);
	list_add(&ctx->list, &ctx->dev->ctx_list);
	mutex_unlock(&ctx->dev->ctx_mutex);
}
EXPORT_SYMBOL_GPL(mtk_vcodec_add_ctx_list);

void mtk_vcodec_del_ctx_list(struct mtk_vcodec_ctx *ctx)
{
	mutex_lock(&ctx->dev->ctx_mutex);
	list_del_init(&ctx->list);
	mutex_unlock(&ctx->dev->ctx_mutex);
}
EXPORT_SYMBOL_GPL(mtk_vcodec_del_ctx_list);


struct vdec_fb *mtk_vcodec_get_fb(struct mtk_vcodec_ctx *ctx)
{
	struct vb2_buffer *dst_buf, *src_buf;
	struct vdec_fb *pfb;
	struct mtk_video_dec_buf *dst_buf_info;
	struct vb2_v4l2_buffer *dst_vb2_v4l2, *src_vb2_v4l2;
	int i;

	if (!ctx) {
		mtk_v4l2_err("Ctx is NULL!");
		return NULL;
	}

	/* for getting timestamp*/
	src_vb2_v4l2 = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	if (src_vb2_v4l2 == NULL) {
		mtk_v4l2_err("[%s][%d]src_buf empty!!", __func__, ctx->id);
		return NULL;
	}
	src_buf = &src_vb2_v4l2->vb2_buf;

	mtk_v4l2_debug_enter();
	dst_vb2_v4l2 = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (dst_vb2_v4l2 == NULL) {
		mtk_v4l2_err("[%s][%d]dst_buf empty!!", __func__, ctx->id);
		return NULL;
	}
	dst_buf = &dst_vb2_v4l2->vb2_buf;
	if (dst_buf != NULL) {
		dst_buf_info = container_of(
			dst_vb2_v4l2, struct mtk_video_dec_buf, vb);
		pfb = &dst_buf_info->frame_buffer;
		pfb->num_planes = dst_vb2_v4l2->vb2_buf.num_planes;
		pfb->index = dst_buf->index;

		for (i = 0; i < dst_vb2_v4l2->vb2_buf.num_planes; i++) {
			//pfb->fb_base[i].va = vb2_plane_vaddr(dst_buf, i);
#ifdef CONFIG_VB2_MEDIATEK_DMA_SG
			pfb->fb_base[i].dma_addr =
				mtk_vb2_dma_contig_plane_dma_addr(dst_buf, i);
#else
			pfb->fb_base[i].dma_addr =
				vb2_dma_contig_plane_dma_addr(dst_buf, i);
#endif
			pfb->fb_base[i].size = ctx->picinfo.fb_sz[i];
			pfb->fb_base[i].length = dst_buf->planes[i].length;
			pfb->fb_base[i].dmabuf = dst_buf->planes[i].dbuf;
		}

		pfb->status = 0;
		mtk_v4l2_debug(1, "[%d] idx=%d pfb=0x%p VA=%p dma_addr[0]=%p dma_addr[1]=%p Size=%zx fd:%x, dma_general_buf = %p, general_buf_fd = %d, dma_meta_buf = %p, meta_buf_fd = %d",
				ctx->id, dst_buf->index, pfb,
				pfb->fb_base[0].va,
				&pfb->fb_base[0].dma_addr,
				&pfb->fb_base[1].dma_addr,
				pfb->fb_base[0].size,
				dst_buf->planes[0].m.fd,
				pfb->dma_general_buf,
				pfb->general_buf_fd,
				pfb->dma_meta_buf,
				pfb->meta_buf_fd);

		mutex_lock(&ctx->buf_lock);
		dst_buf_info->used = true;
		mutex_unlock(&ctx->buf_lock);
		dst_vb2_v4l2 = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		if (dst_vb2_v4l2 == NULL) {
			mtk_v4l2_err("v4l2_m2m_dst_buf_remove return NULL\n");
			return NULL;
		}

		dst_buf = &dst_vb2_v4l2->vb2_buf;
		mtk_v4l2_debug(8, "[%d] index=%d, num_rdy_bufs=%d\n", ctx->id,
			dst_buf->index,
			v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx));

	} else {
		mtk_v4l2_err("[%d] No free framebuffer in v4l2!!\n", ctx->id);
		pfb = NULL;
	}
	mtk_v4l2_debug_leave();

	return pfb;
}
EXPORT_SYMBOL(mtk_vcodec_get_fb);


struct mtk_vcodec_mem *mtk_vcodec_get_bs(struct mtk_vcodec_ctx *ctx)
{
	struct vb2_buffer *dst_buf;
	struct mtk_vcodec_mem *pbs_buf;
	struct mtk_video_enc_buf *dst_buf_info;
	struct vb2_v4l2_buffer *dst_vb2_v4l2;

	if (!ctx) {
		mtk_v4l2_err("Ctx is NULL!");
		return NULL;
	}

	mtk_v4l2_debug_enter();
	dst_vb2_v4l2 = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (dst_vb2_v4l2 == NULL) {
		mtk_v4l2_err("[%s][%d]dst_buf empty!!", __func__, ctx->id);
		return NULL;
	}
	dst_buf = &dst_vb2_v4l2->vb2_buf;
	if (dst_buf != NULL) {
		dst_buf_info = container_of(dst_vb2_v4l2, struct mtk_video_enc_buf, vb);
		pbs_buf = &dst_buf_info->bs_buf;

		if (!ctx->enc_params.svp_mode)
			pbs_buf->va = vb2_plane_vaddr(dst_buf, 0);
		pbs_buf->dma_addr = vb2_dma_contig_plane_dma_addr(dst_buf, 0);
		pbs_buf->size = (size_t)dst_buf->planes[0].length;
		pbs_buf->dmabuf = dst_buf->planes[0].dbuf;

		dst_vb2_v4l2 = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		dst_buf = &dst_vb2_v4l2->vb2_buf;
		if (dst_buf != NULL) {
			mtk_v4l2_debug(8, "[%d] index=%d, num_rdy_bufs=%d\n", ctx->id,
				dst_buf->index,
				v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx));
		}

	} else {
		mtk_v4l2_err("[%d] No free framebuffer in v4l2!!\n", ctx->id);
		pbs_buf = NULL;
	}
	mtk_v4l2_debug_leave();

	return pbs_buf;
}
EXPORT_SYMBOL(mtk_vcodec_get_bs);


void v4l2_m2m_buf_queue_check(struct v4l2_m2m_ctx *m2m_ctx,
		void *vbuf)
{
	struct v4l2_m2m_buffer *b = container_of(vbuf,
				struct v4l2_m2m_buffer, vb);
	mtk_v4l2_debug(8, "[Debug] b %p b->list.next %p prev %p %p %p\n",
		b, b->list.next, b->list.prev,
		LIST_POISON1, LIST_POISON2);

	if (WARN_ON(IS_ERR_OR_NULL(m2m_ctx) ||
		(b->list.next != LIST_POISON1 && b->list.next) ||
		(b->list.prev != LIST_POISON2 && b->list.prev))) {
		v4l2_aee_print("b %p next %p prev %p already in rdyq %p %p\n",
			b, b->list.next, b->list.prev,
			LIST_POISON1, LIST_POISON2);
		return;
	}
	v4l2_m2m_buf_queue(m2m_ctx, vbuf);
}
EXPORT_SYMBOL(v4l2_m2m_buf_queue_check);

struct sg_table *mtk_dma_dup_sg_table_by_range(const struct sg_table *table,
	unsigned int offset, unsigned int len)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;
	unsigned int sg_offset = 0, first_sg_require_len = 0;
	unsigned int contig_size = 0, found_start = 0;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_table, table->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	new_table->nents = 0;

	for_each_sg(table->sgl, sg, table->orig_nents, i) {
		if (!found_start) {
			sg_offset += sg->length;
			if (sg_offset <= offset)
				continue;
			found_start = 1;

			/*copy first sg and cal first sg off and length*/
			memcpy(new_sg, sg, sizeof(*sg));
			first_sg_require_len = sg_offset - offset;
			new_sg->offset += sg->length - first_sg_require_len;
			if (len <= first_sg_require_len)
				new_sg->length = len;
			else
				new_sg->length = first_sg_require_len;
			contig_size += new_sg->length;
		} else {
			memcpy(new_sg, sg, sizeof(*sg));
			contig_size += sg->length;
		}

		new_table->nents++;
		if (contig_size >= len) {
			/*adjust the last sg length*/
			new_sg->length -= (contig_size - len);
			break;
		}
		new_sg = sg_next(new_sg);
	}
	return new_table;
}
EXPORT_SYMBOL(mtk_dma_dup_sg_table_by_range);

