/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MDP_BUFFER_H__
#define __MDP_BUFFER_H__
#include <ion/ion.h>
#include "mdp_def.h"
#include "mdp_param.h"

/* 3 temp buffer */
#define MAX_TEMP_BUFFER_NODE 3

/* temp buffer size */
#define TEMP_BUFFER_SIZE (1920 * 1080 * 4)

/*
 * mdp_buffer global info internal use structure
 */
struct mdp_temp_buffer_global_struct {
	struct mdp_temp_buffer_struct buffer_pool[MAX_TEMP_BUFFER_NODE];
	struct ion_client *client;

	/* waitqueue: sleep wait for get a idle imgresz hardware,
	 * wait on imgresz hw management thread
	 */
	wait_queue_head_t mdp_temp_buffer_wait_buffer_free_queue;
};

/*
 * get temp buffer from buffer pool
 */
enum MDP_TASK_STATUS mdp_temp_buffer_get_buffer(
	struct mdp_temp_buffer_struct **ppBuffer);

/*
 * release temp buffer to buffer pool
 */
enum MDP_TASK_STATUS mdp_temp_buffer_put_buffer(
	struct mdp_temp_buffer_struct *pBuffer);


void mdp_temp_buffer_init(void);


enum MDP_TASK_STATUS mdp_buffer_convert_ion_fd(
	struct mdp_buffer_struct *pBuffer,
	struct mdp_ion_struct *pIonHandle);
enum MDP_TASK_STATUS mdp_buffer_release_ion_handle(
	struct mdp_buffer_struct *pBuffer,
	struct mdp_ion_struct *pIonHandle);
enum MDP_TASK_STATUS mdp_buffer_share_ion_fd(
	struct mdp_buffer_struct *pBuffer,
	struct mdp_ion_struct *pIonHandle);


#endif /* endof __MDP_BUFFER_H__ */
