/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_MFG_COUNTER__
#define __MTK_MFG_COUNTER__

#include <linux/seq_file.h>

extern int mfg_counter_init(void);
extern void mfg_counter_deinit(void);

void mfg_counter_start_trace(void);
void mfg_counter_stop_trace(void);

void mfg_counter_show_trace(struct seq_file *m);
int mfg_counter_set_hist_size(int period, int len);

void mfg_counter_power_on(void);
void mfg_counter_power_off(void);
#endif
