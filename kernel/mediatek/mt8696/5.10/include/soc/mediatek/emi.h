/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __EMI_H__
#define __EMI_H__

#include <linux/irqreturn.h>

#define MTK_EMIMPU_NO_PROTECTION           0
#define MTK_EMIMPU_NSEC_W_FORBIDDEN        1
#define MTK_EMIMPU_NSEC_R_FORBIDDEN        2
#define MTK_EMIMPU_NSEC_RW_FORBIDDEN       3
#define MTK_EMIMPU_SEC_W_FORBIDDEN         4
#define MTK_EMIMPU_SEC_W_NSEC_W_FORBIDDEN  5
#define MTK_EMIMPU_SEC_W_NSEC_R_FORBIDDEN  6
#define MTK_EMIMPU_SEC_W_NSEC_RW_FORBIDDEN 7
#define MTK_EMIMPU_SEC_R_FORBIDDEN         8
#define MTK_EMIMPU_SEC_R_NSEC_W_FORBIDDEN  9
#define MTK_EMIMPU_SEC_R_NSEC_R_FORBIDDEN  10
#define MTK_EMIMPU_SEC_R_NSEC_RW_FORBIDDEN 11
#define MTK_EMIMPU_SEC_RW_FORBIDDEN        12
#define MTK_EMIMPU_SEC_RW_NSEC_W_FORBIDDEN 13
#define MTK_EMIMPU_SEC_RW_NSEC_R_FORBIDDEN 14
#define MTK_EMIMPU_ALL_FORBIDDEN           15

#define MTK_EMIMPU_UNLOCK		false
#define MTK_EMIMPU_LOCK			true

#define MTK_EMIMPU_SET			0
#define MTK_EMIMPU_CLEAR		1
#define MTK_EMIMPU_READ			2
#define MTK_EMIMPU_SLVERR		3
#define MTK_EMIDBG_DUMP			4
#define MTK_EMIDBG_MSG			5
#define MTK_EMI_VIO_CLR			6
#define MTK_EMI_READ			7

#define MTK_EMIMPU_READ_SA		0
#define MTK_EMIMPU_READ_EA		1
#define MTK_EMIMPU_READ_APC		2

#define MTK_EMI_MAX_TOKEN		4
#define MTK_EMI_MAX_CMD_LEN		4096

enum {
	EMI_VIO_CLR_MPU = 0,
	EMI_VIO_CLR_MD,
};

struct emi_addr_map {
	int emi;
	int channel;
	int rank;
	int bank;
	int row;
	int column;
};

struct reg_info_t {
	unsigned int offset;
	unsigned int value;
	unsigned int leng;
};

struct emimpu_region_t {
	unsigned long long start;
	unsigned long long end;
	unsigned int rg_num;
	bool lock;
	unsigned int *apc;
};

typedef irqreturn_t (*emimpu_pre_handler)(
	unsigned int emi_id, struct reg_info_t *dump, unsigned int leng);
typedef void (*emimpu_post_clear)(unsigned int emi_id);
typedef void (*emimpu_md_handler)(
	unsigned int emi_id, struct reg_info_t *dump, unsigned int leng);
typedef void (*emimpu_debug_dump)(void);

struct emimpu_dbg_cb {
	emimpu_debug_dump func;
	struct emimpu_dbg_cb *next_dbg_cb;
};

/* mtk emicen api */
unsigned int mtk_emicen_get_ch_cnt(void);
unsigned int mtk_emicen_get_rk_cnt(void);
unsigned int mtk_emicen_get_rk_size(unsigned int rk_id);


/* mtk emimpu api */
int mtk_emimpu_prehandle_register(emimpu_pre_handler bypass_func);
int mtk_emimpu_postclear_register(emimpu_post_clear clear_func);
int mtk_emimpu_md_handling_register(emimpu_md_handler md_handling_func);
int mtk_emimpu_debugdump_register(emimpu_debug_dump debug_func);
void mtk_clear_md_violation(void);

#endif /* __EMI_H__ */

