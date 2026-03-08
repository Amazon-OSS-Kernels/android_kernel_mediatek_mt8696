/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>

#include "disp_hw_mgr.h"
#include "disp_irq.h"
#include "disp_ml_cmd.h"
#include "ml_hal.h"
#include "ml_hw.h"
#include "disp_ml_if.h"



struct mutex disp_ml_mutex;
struct mutex disp_ml_dispsys_mutex;
struct mutex disp_ml_mmsys_mutex;
struct device *ml_dev;
char ml_dts_nd_name[ML_DTS_NAME_SIZE] = "mediatek,mt8696-adl-ml";
uintptr_t ml_reg_base[ML_REG_ID_MAX];

static bool ml_init_done;
bool b_ml_recv_cmd = true;
bool disp_ml_clk_en[2];
uint32_t size_change_cnt;
uint32_t size_depth[2];

struct disp_ml_clt_context ml_clt_ctx[ML_IP_MAX];
struct ml_hw_reg_tbl ml_clt_reg_tbl[ML_IP_MAX];


static int ml_write2regtable_2bytes(uint32_t addr,
	uint16_t value, uint16_t mask, struct ml_hw_reg_tbl *preg_tbl)
{
	if (mask) {
		if (preg_tbl->depth < ML_REG_COUNT) {
			preg_tbl->address[preg_tbl->depth] = addr;
			preg_tbl->value[preg_tbl->depth] = value;
			preg_tbl->mask[preg_tbl->depth] = mask;
			preg_tbl->depth++;
			ml_reg("update reg 0x%x 0x%x 0x%x %d\n",
				addr, value, mask, preg_tbl->depth);
		} else {
			ml_error("overflow %d\n", preg_tbl->depth);
			return -1;
		}
	}
	return 0;
}

static int ml_parse_dev_node(void)
{
	struct device_node *np;
	uint32_t reg_value;
	char nd_name[ML_DTS_NAME_SIZE];

	sprintf(nd_name, "%s", ml_dts_nd_name);

	np = of_find_compatible_node(NULL, NULL, nd_name);
	if (np == NULL) {
		ml_error("dts error, no device node %s.\n", nd_name);
		return ML_ST_ERR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);

	/* mmsys ml regbase 0x14013000 */
	ml_reg_base[ML_MSYS] = (uintptr_t)of_iomap(np, 0);
	/* dispsys ml regbase 0x15013000 */
	ml_reg_base[ML_DSYS] = (uintptr_t)of_iomap(np, 1);

	ml_info("msys reg 0x%p,dsys 0x%p\n",
		(void *)ml_reg_base[ML_MSYS], (void *)ml_reg_base[ML_DSYS]);

	return ML_ST_OK;
}

void disp_ml_clk_on_off(uint8_t ml_ip, bool en)
{
	if (en != disp_ml_clk_en[ml_ip]) {
		if (ml_ip == ML_DSYS_IP) {
			disp_clock_enable(DISP_CLK_M_VDO_FE_ADL, en);
			disp_clock_smi_larb_en(DISP_SMI_LARB5, en);
		} else {
			disp_clock_enable(DISP_CLK_HDR_ADL, en);
			disp_clock_smi_larb_en(DISP_SMI_LARB4, en);
		}
		disp_ml_clk_en[ml_ip] = en;
		ml_printf("ml[%d] clk on/off %d\n", ml_ip, en);
	}
}

int ml_client_ctx_free(void)
{
	uint32_t i = 0;
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	struct device *ml_dev = NULL;

	ml_dev = disp_hw_mgr_get_dev();
	for (i = 0; i < ML_IP_MAX; i++) {
		p_ml_ctx = &ml_clt_ctx[i];
		if ((p_ml_ctx->lut_addr_va != NULL)
			&& (p_ml_ctx->state != ML_DISABLE)) {
			dma_free_coherent(ml_dev,
				ML_DSYS_CMD_MAX_SIZE * sizeof(uint64_t),
				p_ml_ctx->lut_addr_va,
				p_ml_ctx->lut_addr_mva);
			ml_printf("clt[%d] free\n", i);
		}
	}
	return 0;
}

int disp_ml_clt_set_mode(enum ML_SYS_IP clit, uint8_t clt_update_mode,
	uint8_t reg_cfg_mode)
{
	struct disp_ml_clt_context *p_ml_ctx = NULL;

	p_ml_ctx = &ml_clt_ctx[clit];
	p_ml_ctx->clt_update_mode = clt_update_mode;
	p_ml_ctx->reg_conf_mode = reg_cfg_mode;

	return 0;
}
static int disp_ml_clt_ctx_init(enum ML_SYS_IP clit)
{
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	uintptr_t remap_reg = 0;
	uint32_t hw_reg = 0;
	dma_addr_t mva_addr = 0;
	uint64_t *va = NULL;
	struct device *ml_dev = NULL;
	uint32_t size = 0;

	p_ml_ctx = &ml_clt_ctx[clit];
	ml_dev = disp_hw_mgr_get_dev();

	switch (clit) {
	case ML_MSYS_IP:
		remap_reg = ml_reg_base[ML_MSYS_IP];
		hw_reg = ML_MSYS_REG_BASE;
		size = ML_MSYS_CMD_MAX_SIZE;
		break;
	case ML_DSYS_IP:
		remap_reg = ml_reg_base[ML_DSYS_IP];
		hw_reg = ML_DSYS_REG_BASE;
		size = ML_DSYS_CMD_MAX_SIZE;
		break;
	default:
		ml_error("%s error clt\n", __func__);
		return ML_ST_ERR;
	}

	va =
	(uint64_t *)dma_alloc_coherent(ml_dev,
	size * sizeof(uint64_t), &mva_addr, GFP_KERNEL);

	if (va == NULL) {
		ml_error("ml alloc mem failed\n");
		return ML_ST_ERR;
	}
	memset(va, 0, size * sizeof(uint64_t));

	p_ml_ctx->state = ML_INIT;
	p_ml_ctx->cmd_depth = 0;
	p_ml_ctx->remap_reg_base = remap_reg;
	p_ml_ctx->hw_reg_base = hw_reg;
	p_ml_ctx->clt_update_mode = 1;//default framedone trigger mode
	p_ml_ctx->reg_conf_mode = 1; //default cpu updatte reg
	p_ml_ctx->preg_tbl = &ml_clt_reg_tbl[clit];

	p_ml_ctx->lut_addr_va = va;
	p_ml_ctx->lut_addr_mva = mva_addr;
	p_ml_ctx->lut_addr_pa = 0;

	ml_info("ml clt[%d] 0x%x 0x%p 0x%x 0x%p 0x%x\n", clit,
		p_ml_ctx->hw_reg_base, (void *)p_ml_ctx->remap_reg_base,
		p_ml_ctx->lut_addr_mva, p_ml_ctx->lut_addr_va,
		p_ml_ctx->lut_addr_pa);
	return ML_ST_OK;

}

int disp_ml_init(struct disp_hw_common_info *info)
{
	uint8_t i = 0;

	if (info == NULL) {
		ml_error("hwg info is null\n");
		return ML_ST_ERR;
	}
	mutex_init(&disp_ml_mutex);
	mutex_init(&disp_ml_dispsys_mutex);
	mutex_init(&disp_ml_mmsys_mutex);

	/*mmsys ml clk share clk with adl*/
	//disp_ml_clk_on_off(ML_MSYS_IP, true);
	ml_dev = disp_hw_mgr_get_dev();

	/*parser dts file for register base */
	ml_parse_dev_node();
	for (i = 0; i < ML_IP_MAX; i++) {
		if (disp_ml_clt_ctx_init(i) == ML_ST_ERR) {
			ml_client_ctx_free();
			ml_error("init failed\n");
			return -1;
		}
	}

	//ml_alloc_mem();
	ml_debug_init();
	ml_hal_init();

	b_ml_recv_cmd = 1;
	ml_init_done = 1;
	ml_printf("%s done\n", __func__);

	return ML_ST_OK;
}

int disp_ml_deinit(void)
{
	if (ml_init_done == 0)
		return 0;

	ml_client_ctx_free();

	ml_init_done = 0;

	return 0;
}

int disp_ml_write_dispsys_reg_multi(struct ml_reg_table *reg_tbl)
{
	uint32_t depth = 0;
	int i = 0;
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	uint64_t *ml_addr = NULL;

	mutex_lock(&disp_ml_dispsys_mutex);

	p_ml_ctx = &ml_clt_ctx[ML_DSYS_IP];
	if ((p_ml_ctx->reset == 0) &&
		(reg_tbl->reg_type != 1)) {
		memset(p_ml_ctx->lut_addr_va, 0,
			ML_DSYS_CMD_MAX_SIZE * sizeof(uint64_t));
		p_ml_ctx->cmd_depth = 0;
		p_ml_ctx->reset = 1;
		ml_info("reset ml clt[%d] mem\n",
			ML_DSYS_IP);
	}

	ml_addr = p_ml_ctx->lut_addr_va;
	if (p_ml_ctx->cmd_depth >= ML_DSYS_CMD_MAX_SIZE) {
		ml_error("dsys cmd alreay full\n");
		mutex_unlock(&disp_ml_dispsys_mutex);
		return ML_ST_ERR;
	}

	/* record the depth of size when continue size change
	 * then replace the old size by the reduce ml table depth
	 */
	if (reg_tbl->reg_type == 1 && reg_tbl->depth == 4)
		size_change_cnt++;
	else {
		size_change_cnt = 0;
		size_depth[0] = 0;
		size_depth[1] = 0;
	}

	if ((size_change_cnt > 1) && (p_ml_ctx->cmd_depth > 4)) {
		if ((reg_tbl->p_reg_addr[0] & 0xFFFFF000) == M_HDR_FE_BASE) {
			if (size_depth[0] == 0) {
				size_depth[0] = p_ml_ctx->cmd_depth;
				ml_printf("video0 size depth %d\n", size_depth[0]);
			} else {
				for (i = 0; i < 4; i++) {
					ML_UPDATE_REG_TABLE_SIZE(ml_addr,
						size_depth[0] + i,
						reg_tbl->p_reg_value[i],
						reg_tbl->p_reg_mask[i],
						reg_tbl->p_reg_addr[i]);
				}
				reg_tbl->depth = 0;
			}

		} else if ((reg_tbl->p_reg_addr[0] & 0xFFFFF000) == S_HDR_FE_BASE) {
			if (size_depth[1] == 0) {
				size_depth[1] = p_ml_ctx->cmd_depth;
				ml_printf("video1 size depth %d\n", size_depth[1]);
			} else {
				for (i = 0; i < 4; i++)
					ML_UPDATE_REG_TABLE_SIZE(ml_addr,
						size_depth[1] + i,
						reg_tbl->p_reg_value[i],
						reg_tbl->p_reg_mask[i],
						reg_tbl->p_reg_addr[i]);
				reg_tbl->depth = 0;
			}
		}
	}

	depth = reg_tbl->depth;

	for (i = 0; i < depth; i++) {
		ml_reg("dsys 0x%x\n", reg_tbl->p_reg_addr[i]);
		ML_UPDATE_REG_TABLE(ml_addr,
			p_ml_ctx->cmd_depth,
			reg_tbl->p_reg_value[i],
			reg_tbl->p_reg_mask[i],
			reg_tbl->p_reg_addr[i]);

		p_ml_ctx->cmd_depth++;
		if (p_ml_ctx->cmd_depth >= ML_DSYS_CMD_MAX_SIZE) {
			ml_error("during update mldsys cmd overflow\n");
			//mutex_unlock(&disp_ml_dispsys_mutex);
			//return ML_ST_ERR;
			break;
		}
	}
	p_ml_ctx->state = ML_ENABLE;

	disp_ml_get_setting(reg_tbl->ml_ip);
	disp_ml_client_flush(reg_tbl->ml_ip);

	ml_info("%s clt[%d] depth %d addr 0x%p\n", __func__,
		reg_tbl->ml_ip,
		p_ml_ctx->cmd_depth, ml_addr);
	if ((p_ml_ctx->reset == 0) && (reg_tbl->reg_type != 1))
		ml_printf("dispsys ml not finish write\n");
	mutex_unlock(&disp_ml_dispsys_mutex);

	return ML_ST_OK;
}

int disp_ml_write_mmsys_reg_multi(struct ml_reg_table *reg_tbl)
{
	uint32_t depth = 0;
	int i = 0;
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	uint64_t *ml_addr = NULL;

	mutex_lock(&disp_ml_mmsys_mutex);

	p_ml_ctx = &ml_clt_ctx[ML_MSYS_IP];
	if (p_ml_ctx->reset == 0) {
		memset(p_ml_ctx->lut_addr_va, 0,
			ML_MSYS_CMD_MAX_SIZE * sizeof(uint64_t));
		p_ml_ctx->cmd_depth = 0;
		p_ml_ctx->reset = 1;
		ml_info("reset ml clt[%d] mem\n",
			ML_MSYS_IP);
	}
	ml_addr = p_ml_ctx->lut_addr_va;

	if (p_ml_ctx->cmd_depth >= ML_MSYS_CMD_MAX_SIZE) {
		ml_error("msys cmd is alreay full\n");
		mutex_unlock(&disp_ml_mmsys_mutex);
		return ML_ST_ERR;
	}

	depth = reg_tbl->depth;

	for (i = 0; i < depth; i++) {
		ml_reg("msys 0x%x\n", reg_tbl->p_reg_addr[i]);
		ML_UPDATE_REG_TABLE(ml_addr,
			p_ml_ctx->cmd_depth,
			reg_tbl->p_reg_value[i],
			reg_tbl->p_reg_mask[i],
			reg_tbl->p_reg_addr[i]);

		p_ml_ctx->cmd_depth++;
		if (p_ml_ctx->cmd_depth >= ML_MSYS_CMD_MAX_SIZE) {
			ml_error("afterupd mlmsys cmd overflow\n");
			//mutex_unlock(&disp_ml_mmsys_mutex);
			//return ML_ST_ERR;
			break;
		}
	}
	p_ml_ctx->state = ML_ENABLE;

	disp_ml_get_setting(reg_tbl->ml_ip);
	disp_ml_client_flush(reg_tbl->ml_ip);

	ml_info("%s clt[%d] depth %d addr 0x%p\n", __func__,
		reg_tbl->ml_ip,
		p_ml_ctx->cmd_depth, ml_addr);
	if ((p_ml_ctx->reset == 0) && (reg_tbl->reg_type != 1))
		ml_printf("mmsys ml not finish write\n");
	mutex_unlock(&disp_ml_mmsys_mutex);

	return ML_ST_OK;
}

int disp_ml_write_reg_multi(struct ml_reg_table *reg_tbl)
{
	uint8_t path = 0;

	if (b_ml_recv_cmd == false) {
		ml_printf("irq period,cannot recv cmd\n");
		return ML_ST_ERR;
	}

	if (reg_tbl == NULL) {
		ml_error("reg tbl null\n");
		return ML_ST_ERR;
	}

	if (reg_tbl->ml_ip >= ML_IP_MAX
		|| reg_tbl->depth == 0
		|| (reg_tbl->depth % 4 != 0)) {
		ml_error("reg_table[%d] not support %d\n",
			reg_tbl->ml_ip,
			reg_tbl->depth);
		return ML_ST_ERR;
	}

	disp_hdr_get_cur_path(&path);
	if (((path == ML_DOVI) || (path == ML_OPENHDR))
		&& (path != reg_tbl->path)) {
		ml_printf("%d path not match %d %d\n", reg_tbl->ml_ip,
			path, reg_tbl->path);
		return ML_ST_ERR;
	}
	if (reg_tbl->ml_ip == ML_DSYS_IP)
		disp_ml_write_dispsys_reg_multi(reg_tbl);
	else
		disp_ml_write_mmsys_reg_multi(reg_tbl);

	return ML_ST_OK;
}

int disp_ml_write_reg_single(uint8_t ml_ip, uint32_t reg,
	uint16_t val, uint16_t mask)
{
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	uint64_t *ml_addr = NULL;

	if (b_ml_recv_cmd == false) {
		ml_printf("irq period,cannot recv cmd\n");
		return ML_ST_ERR;
	}

	if (ml_ip >= ML_IP_MAX) {
		ml_error("client not support\n");
		return ML_ST_ERR;
	}

	mutex_lock(&disp_ml_mutex);

	if (ml_ip == ML_DSYS_IP) {
		p_ml_ctx = &ml_clt_ctx[ML_DSYS_IP];
		ml_addr = p_ml_ctx->lut_addr_va;

		if (p_ml_ctx->cmd_depth >= ML_DSYS_CMD_MAX_SIZE) {
			ml_error("dsys cmd size alreay full\n");
			mutex_unlock(&disp_ml_mutex);
			return ML_ST_ERR;
		}

		ML_UPDATE_REG_TABLE(ml_addr, p_ml_ctx->cmd_depth,
			val, mask, reg);

		p_ml_ctx->cmd_depth++;
		if (p_ml_ctx->cmd_depth > ML_DSYS_CMD_MAX_SIZE) {
			ml_error("afterup dsys cmd size full\n");
			mutex_unlock(&disp_ml_mutex);
			return ML_ST_ERR;
		}
		p_ml_ctx->state = ML_ENABLE;
		ml_printf("update dsyscmddpt %d\n", p_ml_ctx->cmd_depth);
	} else {
		p_ml_ctx = &ml_clt_ctx[ML_MSYS_IP];
		ml_addr = p_ml_ctx->lut_addr_va;

		if (p_ml_ctx->cmd_depth >= ML_MSYS_CMD_MAX_SIZE) {
			ml_error("msys cmd size alreay full\n");
			mutex_unlock(&disp_ml_mutex);
			return ML_ST_ERR;
		}

		ML_UPDATE_REG_TABLE(ml_addr, p_ml_ctx->cmd_depth,
			val, mask, reg);

		p_ml_ctx->cmd_depth++;
		if (p_ml_ctx->cmd_depth > ML_MSYS_CMD_MAX_SIZE) {
			ml_error("afterup msys cmd size full\n");
			mutex_unlock(&disp_ml_mutex);
			return ML_ST_ERR;
		}
		p_ml_ctx->state = ML_ENABLE;
		ml_printf("update msyscmddpt %d\n", p_ml_ctx->cmd_depth);
	}
	ml_info("%s clt[%d] depth %d addr 0x%p\n", __func__,
		ml_ip,
		p_ml_ctx->cmd_depth, ml_addr);
	mutex_unlock(&disp_ml_mutex);

	return ML_ST_OK;
}

int disp_ml_set_buffer(uint8_t ml_ip, void *va, uint32_t len)
{
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	uint32_t size = 0;

	mutex_lock(&disp_ml_mutex);
	p_ml_ctx = &ml_clt_ctx[ml_ip];

	if (ml_ip == ML_MSYS_IP)
		size = ML_MSYS_CMD_MAX_SIZE;
	else
		size = ML_DSYS_CMD_MAX_SIZE;

	if (va == NULL || ml_ip >= ML_IP_MAX ||
		len >= size) {
		ml_error("%s input error\n", __func__);
		return ML_ST_ERR;
	}

	memset(p_ml_ctx->lut_addr_va, 0, size);
	memcpy(p_ml_ctx->lut_addr_va, va, len * sizeof(uint64_t));

	p_ml_ctx->cmd_depth = len;
	p_ml_ctx->state = ML_ENABLE;
	disp_ml_get_setting(ml_ip);
	disp_ml_client_flush(ml_ip);
	mutex_unlock(&disp_ml_mutex);

	ml_printf("%s ml[%d] 0x%x 0x%p 0x%x len %d\n", __func__,
		ml_ip, p_ml_ctx->hw_reg_base,
		p_ml_ctx->lut_addr_va, p_ml_ctx->lut_addr_mva,
		p_ml_ctx->cmd_depth);
	return 0;
}
int disp_ml_get_setting(uint8_t ml_ip)
{
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	struct ml_hw_reg_tbl *p_reg_tbl = NULL;
	dma_addr_t mva_msb = 0;
	dma_addr_t mva_lsb = 0;
	uint32_t value = 0;

	p_ml_ctx = &ml_clt_ctx[ml_ip];
	p_reg_tbl = p_ml_ctx->preg_tbl;

	mva_msb = ((0xffff0000 & ((p_ml_ctx->lut_addr_mva) >> 5)) >> 16);
	mva_lsb = ((0x0000ffff & ((p_ml_ctx->lut_addr_mva) >> 5)));


	/*dsys bmask enable*/
	ml_write2regtable_2bytes(ML_MSK_EN, 0x1, 0x1, p_reg_tbl);

	if (p_ml_ctx->clt_update_mode == 0) {
		/*74 ml sw trigge mode*/
		ml_write2regtable_2bytes(ML_TRIG_MD, 0x1000, 0x1000, p_reg_tbl);
	} else {
		/*74 ml vs trigge mode*/
		ml_write2regtable_2bytes(ML_TRIG_MD, 0x0, 0x1000, p_reg_tbl);
	}

	/*08 set en */
	value = 0x8000 | (p_ml_ctx->cmd_depth / 4);
	ml_write2regtable_2bytes(ML_EN, value, 0xffff, p_reg_tbl);

	/*04 ml read length*/
	value = p_ml_ctx->cmd_depth / 4;
	ml_write2regtable_2bytes(ML_DEPTH, value, 0xffff, p_reg_tbl);

	/*0c ml addr0*/
	ml_write2regtable_2bytes(ML_ADDR0, mva_lsb, 0xffff, p_reg_tbl);

	/*10 ml addr1*/
	ml_write2regtable_2bytes(ML_ADDR1, mva_msb, 0x1fff, p_reg_tbl);

	/*1c ml ds msk*/
	ml_write2regtable_2bytes(ML_DS_MSK, 0xc008, 0xffff, p_reg_tbl);

	/*40 ml ds write en*/
	ml_write2regtable_2bytes(ML_DS_WE, 0x1004, 0xffff, p_reg_tbl);

	/*1c0 ml 64bit cmd en*/
	ml_write2regtable_2bytes(ML_64B_EN, 0x8003, 0xffff, p_reg_tbl);

	/*reset*/
	ml_write2regtable_2bytes(ML_RST, 0xf000, 0xf000, p_reg_tbl);
	ml_write2regtable_2bytes(ML_RST, 0xe000, 0xf000, p_reg_tbl);



	if (p_ml_ctx->clt_update_mode == 0) {
		/*74 ml sw trigge */
		ml_write2regtable_2bytes(ML_TRIG_MD, 0x9000, 0x9000, p_reg_tbl);
	}

	return 0;
}

int disp_ml_set_disable(uint8_t ml_ip)
{
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	uintptr_t rmap_reg_base = 0;

	p_ml_ctx = &ml_clt_ctx[ml_ip];
	rmap_reg_base = p_ml_ctx->remap_reg_base;
	MlWriteREGMsk((rmap_reg_base + ML_EN), 0x0, 0xFFFF);
	MlWriteREGMsk((rmap_reg_base + ML_DEPTH), 0x0, 0xFFFF);
	if (ml_ip == ML_DSYS_IP) {
		memset(p_ml_ctx->lut_addr_va, 0,
			ML_DSYS_CMD_MAX_SIZE * sizeof(uint64_t));
	} else {
		memset(p_ml_ctx->lut_addr_va, 0,
			ML_MSYS_CMD_MAX_SIZE * sizeof(uint64_t));
	}
	p_ml_ctx->cmd_depth = 0;
	return 0;
}

int disp_ml_cmd_align(uint8_t ip)
{
	struct disp_ml_clt_context *p_ml_ctx = NULL;
	uint32_t depth = 0;
	uint32_t depth_max = 0;
	uint32_t padding = 0;
	uint64_t *ml_addr = NULL;

	p_ml_ctx = &ml_clt_ctx[ip];

	if (ip == ML_MSYS_IP)
		depth_max = ML_MSYS_CMD_MAX_SIZE;
	else
		depth_max = ML_DSYS_CMD_MAX_SIZE;

	memset(p_ml_ctx->preg_tbl, 0, sizeof(struct ml_hw_reg_tbl));

	if ((p_ml_ctx->state == ML_ENABLE)
		&& (p_ml_ctx->cmd_depth > 0)) {
		ml_info("ml[%d] before align %d\n", ip, p_ml_ctx->cmd_depth);
		depth = p_ml_ctx->cmd_depth;
		ml_addr = p_ml_ctx->lut_addr_va;
		padding = (4 - (depth % 4)) % 4;
		while (padding) {
			ml_addr[depth] = ml_addr[depth-1];
			depth++;
			if (depth > depth_max) {
				ml_info("msys cmd size full,keep max depth\n");
				//return ML_ST_ERR;
			}
			padding--;
		}
		p_ml_ctx->cmd_depth = depth;
		ml_info("ml[%d] after align %d\n", ip, p_ml_ctx->cmd_depth);
		//disp_ml_get_setting(ip);
	}

	return 0;
}

int disp_ml_client_reset(uint8_t ip)
{
	struct disp_ml_clt_context *p_clt_ctx = NULL;

	p_clt_ctx = &ml_clt_ctx[ip];
	p_clt_ctx->state = ML_INIT;
	p_clt_ctx->reset = 0;

	return 0;
}
int disp_ml_client_flush(uint8_t ip)
{
	struct disp_ml_clt_context *p_clt_ctx = NULL;
	struct ml_hw_reg_tbl *preg_tbl = NULL;
	uintptr_t rmap_reg_base = 0;

	uint32_t hw_reg_base = 0;

	uint8_t i = 0;
	uint8_t mode = 0;
	uint32_t depth = 0;

	p_clt_ctx = &ml_clt_ctx[ip];
	preg_tbl = p_clt_ctx->preg_tbl;
	mode = p_clt_ctx->reg_conf_mode;
	rmap_reg_base = p_clt_ctx->remap_reg_base;
	hw_reg_base = p_clt_ctx->hw_reg_base;
	depth = preg_tbl->depth;

	if (!((p_clt_ctx->state == ML_ENABLE)
		&& (p_clt_ctx->cmd_depth > 0) && (depth > 0))) {
		ml_irq("not reg to flush\n");
		return 0;
	}

	switch (mode) {
	case 0: //gce update
		for (i = 0; i < depth; i++) {
			//call gce api,use hw regbase
			//write_reg(hw_reg_base + preg_tbl->address[i],
			//	preg_tbl->value[i],
			//	preg_tbl->mask[i]);
		}
		break;
	case 1: // cpu update
		for (i = 0; i < depth; i++) {
			//call cpu api,use remap regbase
			MlWriteREGMsk(rmap_reg_base + preg_tbl->address[i],
				preg_tbl->value[i],
				preg_tbl->mask[i]);

			ml_reg("ml setting 0x%p 0x%x 0x%x\n",
				(void *)(rmap_reg_base + preg_tbl->address[i]),
				preg_tbl->value[i],
				preg_tbl->mask[i]);
		}
		break;
	default:
		ml_info("update mode error\n");
		break;
	}
	//disp_ml_client_reset(ip);
	memset(p_clt_ctx->preg_tbl, 0, sizeof(struct ml_hw_reg_tbl));
	return 0;

}

int disp_ml_isr(void)
{
	uint8_t i = 0;

	ml_irq("irq coming\n");//unitest
	b_ml_recv_cmd = false;
	for (i = 0; i < ML_IP_MAX; i++) {
		//disp_ml_cmd_align(i);
		//disp_ml_client_flush(i);
		disp_ml_client_reset(i);
	}
	b_ml_recv_cmd = true;
	return ML_ST_OK;
}

int disp_ml_irq_handler(uint32_t irq)
{
	if (!ml_init_done)
		return ML_ST_OK;

	switch (irq) {
	case DISP_IRQ_BEFIFO: //frame done irq
		disp_ml_isr();
		break;
	default:
		break;
	}
	return ML_ST_OK;
}

int disp_ml_suspend(void)
{
	return ML_ST_OK;
}


int disp_ml_resume(void)
{
	b_ml_recv_cmd = 1;
	return ML_ST_OK;
}


int disp_ml_process_cmd(enum DISP_CMD cmd,
	 void *data)
{
	return ML_ST_OK;
}

/*****define menuload driver*****/
struct disp_hw disp_ml_drv = {
	.name = ML_DRV_NAME,
	.init = disp_ml_init,
	.deinit = disp_ml_deinit,
	.start = NULL,
	.stop = NULL,
	.suspend = disp_ml_suspend,
	.resume = disp_ml_resume,
	.get_info = NULL,
	.change_resolution = NULL,
	.config = NULL,
	.irq_handler = disp_ml_irq_handler,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = NULL,
	.set_cmd = disp_ml_process_cmd,
};

struct disp_hw *disp_ml_get_drv(void)
{
	return &disp_ml_drv;
}

