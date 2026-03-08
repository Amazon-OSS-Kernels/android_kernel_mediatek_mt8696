/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __DRTX_API_H__
#define __DRTX_API_H__


void mtk_dp_sw_interrupt_set(u8 status);
void mtk_dp_dsc_func_en(bool dsc_en, u8 *pps_buf, u8 size);
int mtk_drm_dp_get_dev_info(struct drm_device *dev,
			void *data, struct drm_file *file_priv);
int mtk_drm_dp_audio_enable(struct drm_device *dev,
			void *data, struct drm_file *file_priv);
int mtk_drm_dp_audio_config(struct drm_device *dev,
			void *data, struct drm_file *file_priv);
int mtk_drm_dp_get_cap(struct drm_device *dev,
			void *data, struct drm_file *file_priv);
int mtk_drm_dp_get_info(struct drm_device *dev,
			struct drm_mtk_session_info *info);
void mtk_dp_get_dsc_capability(u8 *dsc_cap);
int mtk_drm_ioctl_enable_dp(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
#endif

