/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#ifndef _MTK_VQ_MGR_H_
#define _MTK_VQ_MGR_H_

#include "vq_def.h"
#include "mtk_vq_info.h"

#define VQ_SESSION_DEVICE "mtk_vq_mgr"
#define VQ_DEVICE_NODE "/dev/mtk_vq_mgr"

struct vq_data *mtk_vq_get_data(void);
void mtk_vq_set_log_enable(u32 level, u32 en);
void mtk_vq_get_log_help(void);
int mtk_vq_mgr_set_input_buffer(struct vq_data *data,
				struct mtk_vq_config *config);
int mtk_vq_mgr_prepare_buffer(struct vq_data *data,
				     struct mtk_vq_config_info *config_info);

int mtk_vq_power_off(struct vq_data *data, enum VQ_PATH_MODE vq_mode);
int mtk_vq_power_on(struct vq_data *data, enum VQ_PATH_MODE vq_mode);


#endif				/* _MTK_VQ_MGR_H_ */
