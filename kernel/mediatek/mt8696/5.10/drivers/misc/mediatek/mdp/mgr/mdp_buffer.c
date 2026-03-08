// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/dma-buf.h>
#include "mdp_buffer.h"
#include "mdp_log.h"
struct device *img_dev;

void mdp_temp_buffer_init(void)
{
}

void mdp_temp_buffer_destroy(void)
{
}

bool mdp_temp_buffer_find_free_buffer(int32_t *p_index)
{
	return 0;//buffer_finded;
}

enum MDP_TASK_STATUS mdp_temp_buffer_allocate_buffer(void)
{
	return 0;//status;
}

/*
 * get temp buffer from buffer pool
 */
enum MDP_TASK_STATUS mdp_temp_buffer_get_buffer(
	struct mdp_temp_buffer_struct **ppBuffer)
{
	enum MDP_TASK_STATUS status = MDP_TASK_STATUS_OK;
	return status;
}

/*
 * release buffer to buffer pool
 */
enum MDP_TASK_STATUS mdp_temp_buffer_put_buffer(
	struct mdp_temp_buffer_struct *pBuffer)
{
	enum MDP_TASK_STATUS status = MDP_TASK_STATUS_OK;
	return status;
}

static bool mdp_ion_get_dma_buf(struct device *dev, int fd,
	struct dma_buf **buf_out, struct dma_buf_attachment **attach_out,
	struct sg_table **sgt_out)
{
	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct sg_table *sgt = NULL;

	if (fd <= 0) {
		MDP_ERR("ion error fd %d\n", fd);
		goto err;
	}

	if (dev == NULL) {
		MDP_ERR("mdp device null\n");
		goto err;
	}

	buf = dma_buf_get(fd);
	if (IS_ERR(buf)) {
		MDP_ERR("ion buf get fail %ld\n", PTR_ERR(buf));
		goto err;
	}

	attach = dma_buf_attach(buf, dev);
	if (IS_ERR(attach)) {
		MDP_ERR("ion buf attach fail %ld", PTR_ERR(attach));
		goto err_attach;
	}

	sgt =  dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		MDP_ERR("ion buf map fail %ld", PTR_ERR(sgt));
		goto err_map;
	}

	*buf_out = buf;
	*attach_out = attach;
	*sgt_out = sgt;

	return true;

err_map:
	dma_buf_detach(buf, attach);

err_attach:
	dma_buf_put(buf);
err:
	return false;
}

static void mdp_ion_free_dma_buf(struct device *dev,
	struct dma_buf *buf,
	struct dma_buf_attachment *attach, struct sg_table *sgt)
{
	if (sgt == NULL || attach == NULL || buf == NULL)
		return;

	if (dev && (sg_page(sgt->sgl) != 0))
		dma_sync_sg_for_cpu(dev, sgt->sgl, sgt->nents, DMA_BIDIRECTIONAL);
	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(buf, attach);
	dma_buf_put(buf);
}

/* convert fd to PA */
enum MDP_TASK_STATUS mdp_buffer_convert_fd(
	struct mdp_buffer_struct *pBuffer,
	struct mdp_ion_struct *pIonHandle)
{

	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct sg_table *sgt = NULL;
	dma_addr_t ion_addr = 0, ion_addr2 = 0;
	bool isOneChannel = false;
	int fd;

	isOneChannel = ((pBuffer->buffer_info.fd) & 0xFFFF0000) == 0;
	if (img_dev == NULL)
		img_dev = imgresz_get_dev();

	fd = (pBuffer->buffer_info.fd & 0x0000FFFF);
	/* need to map dma fd to iova */
	if (!mdp_ion_get_dma_buf(img_dev, fd, &buf, &attach, &sgt))
		return MDP_TASK_STATUS_IMPORT_ION_FD_FAIL;

	ion_addr = sg_dma_address(sgt->sgl);
	if (ion_addr) {
		pIonHandle->buf[0] = buf;
		pIonHandle->attach[0] = attach;
		pIonHandle->sgt[0] = sgt;
		pBuffer->buffer_info.buffersize[0] = pIonHandle->buf[0]->size;
		MDP_LOG("%s fd:%d buffersize:0x%x -> iova:%#llx\n",
			__func__, pBuffer->buffer_info.fd,
			pBuffer->buffer_info.buffersize[0], (u64)ion_addr);
	} else {
		MDP_LOG("%s fail to get iova for fd:%d\n",
			__func__, pBuffer->buffer_info.fd);
		mdp_ion_free_dma_buf(img_dev, buf, attach, sgt);
		return 0;
	}

	if (!isOneChannel) {
		fd = (pBuffer->buffer_info.fd & 0xFFFF0000) >> 16;

		/* need to map dma fd to iova */
		if (!mdp_ion_get_dma_buf(img_dev, fd, &buf, &attach, &sgt))
			return MDP_TASK_STATUS_IMPORT_ION_FD_FAIL;

		ion_addr2 = sg_dma_address(sgt->sgl);
		if (ion_addr2) {
			pIonHandle->buf[1] = buf;
			pIonHandle->attach[1] = attach;
			pIonHandle->sgt[1] = sgt;
			pBuffer->buffer_info.buffersize[1] = pIonHandle->buf[1]->size;
			MDP_LOG("%s fd:%d buffersize:0x%x -> iova2:%#llx\n",
				__func__, pBuffer->buffer_info.fd,
				pBuffer->buffer_info.buffersize[1], (u64)ion_addr2);
		} else {
			MDP_LOG("%s fail to get iova2 for fd:%d\n",
				__func__, pBuffer->buffer_info.fd);
			mdp_ion_free_dma_buf(img_dev, buf, attach, sgt);
			return 0;
		}
	}
	if (pBuffer->buffer_info.memory_type != DP_MEMORY_SECURE) {
		/* normal buffer: convert mva */
		if (isOneChannel) {
			pBuffer->y_buffer_address =
				ion_addr + pBuffer->buffer_info.y_offset;
			if (pBuffer->buffer_info.cb_offset)
				pBuffer->u_buffer_address =
					ion_addr + pBuffer->buffer_info.cb_offset;
			if (pBuffer->buffer_info.cr_offset)
				pBuffer->v_buffer_address =
					ion_addr + pBuffer->buffer_info.cr_offset;
			if (pBuffer->buffer_info.ylen_offset)
				pBuffer->ufo_ylen_buffer_address =
					ion_addr + pBuffer->buffer_info.ylen_offset;
			if (pBuffer->buffer_info.clen_offset)
				pBuffer->ufo_clen_buffer_address =
					ion_addr + pBuffer->buffer_info.clen_offset;
		} else {
			pBuffer->y_buffer_address =
				ion_addr + pBuffer->buffer_info.y_offset;
			pBuffer->u_buffer_address =
				ion_addr2 + pBuffer->buffer_info.cb_offset;
			if (pBuffer->buffer_info.cr_offset)
				pBuffer->v_buffer_address =
					ion_addr2 + pBuffer->buffer_info.cr_offset;
			if (pBuffer->buffer_info.ylen_offset)
				pBuffer->ufo_ylen_buffer_address =
					ion_addr + pBuffer->buffer_info.ylen_offset;
			if (pBuffer->buffer_info.clen_offset)
				pBuffer->ufo_clen_buffer_address =
					ion_addr2 + pBuffer->buffer_info.clen_offset;
		}
	} else {
		/* secure buffer */
		pBuffer->buffer_info.secureHandle = (uint64_t)ion_addr;
		pBuffer->y_buffer_address = pBuffer->buffer_info.y_offset;
		pBuffer->u_buffer_address = pBuffer->buffer_info.cb_offset;
		pBuffer->v_buffer_address = pBuffer->buffer_info.cr_offset;
		pBuffer->ufo_ylen_buffer_address =
			pBuffer->buffer_info.ylen_offset;
		pBuffer->ufo_clen_buffer_address =
			pBuffer->buffer_info.clen_offset;
	}

	return MDP_TASK_STATUS_OK;
}

enum MDP_TASK_STATUS mdp_buffer_release_ion_handle(
	struct mdp_buffer_struct *pBuffer,
	struct mdp_ion_struct *pIonHandle)
{
	mdp_ion_free_dma_buf(img_dev, pIonHandle->buf[0],
			pIonHandle->attach[0], pIonHandle->sgt[0]);
	if (((pBuffer->buffer_info.fd) & 0xFFFF0000) != 0)
		mdp_ion_free_dma_buf(img_dev, pIonHandle->buf[1],
				pIonHandle->attach[1], pIonHandle->sgt[1]);

	return MDP_TASK_STATUS_OK;
}

enum MDP_TASK_STATUS mdp_buffer_share_ion_fd(
	struct mdp_buffer_struct *pBuffer,
	struct mdp_ion_struct *pIonHandle)
{
	return MDP_TASK_STATUS_OK;

}

void *mdp_buffer_get_va(struct mdp_ion_struct *pIonHandle)
{
	void *va;

	if (!pIonHandle->buf[0])
		return NULL;

	va = dma_buf_vmap(pIonHandle->buf[0]);
	dma_sync_sg_for_cpu(img_dev, pIonHandle->sgt[0]->sgl,
			    pIonHandle->sgt[0]->nents, DMA_BIDIRECTIONAL);
	return va;
}

void mdp_buffer_free_va(
	struct mdp_ion_struct *pIonHandle,
	void *va)
{
	if (!pIonHandle->buf[0] || !va)
		return;

	dma_sync_sg_for_device(img_dev, pIonHandle->sgt[0]->sgl,
			       pIonHandle->sgt[0]->nents, DMA_BIDIRECTIONAL);
	dma_buf_vunmap(pIonHandle->buf[0], va);
}
