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
#include "mdp_nr.h"

/*
 * fill mdp task for nr use.
 */
enum MDP_TASK_STATUS mdp_nr_fill_task(struct mdp_task_struct *pTask)
{
	/* TODO */
	return MDP_TASK_STATUS_OK;
}

/*
 * wait nr hardware idle
 */
enum MDP_TASK_STATUS mdp_nr_wait_hw_ready(struct mdp_task_struct *pTask)
{
	/* we don't need to wait nr hardware ready, for nr driver has deal it */
	return MDP_TASK_STATUS_OK;
}

/* trigger nr hardware work */
enum MDP_TASK_STATUS mdp_nr_trigger_hw_work(struct mdp_task_struct *pTask)
{
	/* TODO */
	return MDP_TASK_STATUS_OK;
}



/* handle nr task done */
void mdp_nr_handle_task_done(struct mdp_task_struct *pTask)
{

}

/* handle nr task timeout */
void mdp_nr_handle_task_error(struct mdp_task_struct *pTask)
{

}

/* dump nr task */
void mdp_nr_dump_task(struct mdp_task_struct *pTask)
{

}


