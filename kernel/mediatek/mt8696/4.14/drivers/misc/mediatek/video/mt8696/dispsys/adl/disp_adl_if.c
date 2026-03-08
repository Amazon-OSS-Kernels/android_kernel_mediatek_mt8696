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
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>
#include <uapi/linux/sched/types.h>

#include "disp_hw_mgr.h"
#include "disp_irq.h"
#include "disp_adl_cmd.h"
#include "adl_hal.h"
#include "adl_hw.h"
#include "disp_path.h"
#include "fmt_hal.h"
#include "disp_adl_if.h"

struct mutex disp_adl_mutex;
struct device *adl_dev;
char adl_dts_nd_name[ADL_DTS_MAX_ND_SIZE] = "mediatek,mt8696-adl-ml";
uintptr_t adl_reg_base[ADL_MAX];
static bool adl_init_done;
bool disp_adl_clk_en[2];

struct adl_reg_tbl adl_clt_reg_tbl[ADL_CLIT_REG_MAX];
struct disp_adl_clt_context adl_clt_ctx[ADL_CLIT_REG_MAX];

static struct task_struct *disp_adl_thread;
static wait_queue_head_t disp_adl_thread_wq;
static uint32_t disp_adl_wakeup_thread;
static struct mutex disp_adl_thread_mutex;
bool gfx_lut_update;
uint32_t *gfx_lut_addr;

void disp_adl_thread_wakeup(uint8_t path)
{
	uint8_t hdr_path = 0;

	disp_hdr_get_cur_path(&hdr_path);
	if (((hdr_path == ADL_DOVI) || (hdr_path == ADL_OPENHDR))
		&& (hdr_path != path)) {
		adl_printf("wk path not match %d %d\n", hdr_path, path);
		return;
	}

	disp_adl_wakeup_thread = 1;
	wake_up(&disp_adl_thread_wq);

}

static int disp_adl_routine(void *data)
{
	while (1) {
		wait_event_interruptible(
			disp_adl_thread_wq,
			disp_adl_wakeup_thread);
		disp_adl_wakeup_thread = 0;
		disp_adl_main();
	}
	return 0;
}

int disp_adl_thread_init(void)
{
	struct sched_param adl_param = {.sched_priority = 2};

	if (!(disp_adl_thread)) {
		mutex_init(&disp_adl_thread_mutex);
		adl_info("mutex init\n");

		init_waitqueue_head(&disp_adl_thread_wq);
		disp_adl_wakeup_thread = 0;
		adl_info("wq init\n");

		disp_adl_thread =
			kthread_create(disp_adl_routine,
			NULL, "disp_adl_thread");

		sched_setscheduler(disp_adl_thread, SCHED_RR, &adl_param);

		wake_up_process(disp_adl_thread);

		adl_info("thread init done %p\n", disp_adl_thread);
	}

	return 0;
}


static int adl_parse_dev_node(void)
{
	struct device_node *np;
	unsigned int reg_value;
	char nd_name[ADL_DTS_MAX_ND_SIZE];

	sprintf(nd_name, "%s", adl_dts_nd_name);

	np = of_find_compatible_node(NULL, NULL, nd_name);
	if (np == NULL) {
		adl_error("dts error, no device node %s.\n", nd_name);
		return ADL_ERR;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);

	/* mmsys adl regbase 0x14013000 -0x14014000 */
	adl_reg_base[ADL_MSYS] = (uintptr_t)of_iomap(np, 0);
	/* dispsys adl regbase 0x15013000 -0x15014000 */
	adl_reg_base[ADL_DSYS] = (uintptr_t)of_iomap(np, 1);

	adl_info("adl reg base 0x%p 0x%p\n",
		(void *)adl_reg_base[ADL_MSYS],
		(void *)adl_reg_base[ADL_DSYS]);

	return ADL_OK;
}

int disp_adl_clock_on_off(uint8_t layer_id, bool en)
{
	enum DISP_PATH_HW_ID hw_id = 0;
	enum FMT_SOF_TYPE hw_sof_start = 0;
	enum FMT_SOF_TYPE hw_sof_end = 0;
	int sof_start = 0x00010002;
	int sof_end = 0x00010033;

	if (en != disp_adl_clk_en[layer_id]) {
		if (layer_id == DISPSYS_ADL) {
			hw_id = DISP_PATH_DISP_DISPSYS_MUNULOAD;
			hw_sof_start = FMT_SOF_23_DISP_MUNULOAD_STA;
			hw_sof_end = FMT_SOF_23_DISP_MUNULOAD_END;
			disp_clock_enable(DISP_CLK_M_VDO_FE_ADL, en);
			disp_clock_smi_larb_en(DISP_SMI_LARB5, en);
			fmt_hal_set_sof(hw_sof_start, hw_sof_end,
				sof_start, sof_end);
		} else {
			hw_id = DISP_PATH_DISP_MMSYS_MENULOAD;
			hw_sof_start = FMT_SOF_24_MMSYS_MUNULOAD_STA;
			hw_sof_end = FMT_SOF_24_MMSYS_MUNULOAD_END;
			disp_clock_enable(DISP_CLK_HDR_ADL, en);
			disp_clock_smi_larb_en(DISP_SMI_LARB4, en);
			fmt_hal_set_sof(hw_sof_start, hw_sof_end,
				sof_start, sof_end);
		}
		disp_adl_clk_en[layer_id] = en;
		adl_printf("adl[%d] clk on/off %d\n", layer_id, en);
	}
	return 0;
}

int adl_client_ctx_free(void)
{
	uint32_t i = 0;
	struct disp_adl_clt_context *p_adl_ctx = NULL;
	struct device *adl_dev = NULL;

	adl_dev = disp_hw_mgr_get_dev();
	for (i = 0; i < ADL_CLIT_REG_MAX; i++) {
		p_adl_ctx = &adl_clt_ctx[i];
		if ((p_adl_ctx->lut_addr_va != NULL)
			&& (p_adl_ctx->inited == 1)) {
			dma_free_coherent(adl_dev,
				ADL_LUT_BUFFER_SIZE,
				p_adl_ctx->lut_addr_va,
				p_adl_ctx->lut_addr_mva);
			adl_printf("clt[%d] free\n", i);
		}
		if (p_adl_ctx->src_tbl.hdr_b0103.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.hdr_b0103.p_data);
			p_adl_ctx->src_tbl.hdr_b0103.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.hdr_b0202ss.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.hdr_b0202ss.p_data);
			p_adl_ctx->src_tbl.hdr_b0202ss.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.hdr_b0202si.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.hdr_b0202si.p_data);
			p_adl_ctx->src_tbl.hdr_b0202si.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.hdr_b0202ts.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.hdr_b0202ts.p_data);
			p_adl_ctx->src_tbl.hdr_b0202ts.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.hdr_b0202ti.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.hdr_b0202ti.p_data);
			p_adl_ctx->src_tbl.hdr_b0202ti.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.hdr_b0105.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.hdr_b0105.p_data);
			p_adl_ctx->src_tbl.hdr_b0105.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.hdr_ootf.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.hdr_ootf.p_data);
			p_adl_ctx->src_tbl.hdr_ootf.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.tosd_degam.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.tosd_degam.p_data);
			p_adl_ctx->src_tbl.tosd_degam.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.scmb.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.scmb.p_data);
			p_adl_ctx->src_tbl.scmb.p_data = NULL;
		}

		if (p_adl_ctx->src_tbl.fgrain.p_data != NULL) {
			kfree(p_adl_ctx->src_tbl.fgrain.p_data);
			p_adl_ctx->src_tbl.fgrain.p_data = NULL;
		}

	}
	return 0;
}

int adl_client_ctx_init(enum ADL_CLIT_REG_ID clit)
{
	struct disp_adl_clt_context *p_adl_ctx = NULL;
	uintptr_t remap_reg = 0;
	uint32_t hw_reg = 0;
	dma_addr_t mva_addr = 0;
	void *va = NULL;
	struct device *adl_dev = NULL;

	p_adl_ctx = &adl_clt_ctx[clit];
	memset(p_adl_ctx, 0, sizeof(struct disp_adl_clt_context));
	adl_dev = disp_hw_mgr_get_dev();

	switch (clit) {
	case ADL_DSYS_CLITA:
		remap_reg = adl_reg_base[ADL_DSYS];
		hw_reg = ADL_DSYS_REG_BASE;
		break;
	case ADL_DSYS_CLITB:
		remap_reg = adl_reg_base[ADL_DSYS];
		hw_reg = ADL_DSYS_REG_BASE;
		break;
	case ADL_DSYS_CLITE:
		remap_reg = adl_reg_base[ADL_DSYS];
		hw_reg = ADL_DSYS_REG_BASE;
		break;
	case ADL_DSYS_CLITJ:
		remap_reg = adl_reg_base[ADL_DSYS];
		hw_reg = ADL_DSYS_REG_BASE;
		break;
	case ADL_MSYS_CLITA:
		remap_reg = adl_reg_base[ADL_MSYS];
		hw_reg = ADL_MSYS_REG_BASE;
		break;
	case ADL_MSYS_CLITB:
		remap_reg = adl_reg_base[ADL_MSYS];
		hw_reg = ADL_MSYS_REG_BASE;
		break;
	case ADL_MSYS_CLITC:
		remap_reg = adl_reg_base[ADL_MSYS];
		hw_reg = ADL_MSYS_REG_BASE;
		break;
	case ADL_MSYS_CLITD:
		remap_reg = adl_reg_base[ADL_MSYS];
		hw_reg = ADL_MSYS_REG_BASE;
		break;
	case ADL_MSYS_CLITE:
		remap_reg = adl_reg_base[ADL_MSYS];
		hw_reg = ADL_MSYS_REG_BASE;
		break;
	default:
		adl_error("%s error clt\n", __func__);
		return -1;
	}

	va = dma_alloc_coherent(adl_dev,
		ADL_LUT_BUFFER_SIZE, &mva_addr, GFP_KERNEL);
	if (va == NULL) {
		adl_error("adl alloc mem failed\n");
		return ADL_ERR;
	}
	memset(va, 0, ADL_LUT_BUFFER_SIZE);

	p_adl_ctx->inited = 1;
	p_adl_ctx->enabled = 0;
	p_adl_ctx->remap_reg_base = remap_reg;
	p_adl_ctx->hw_reg_base = hw_reg;
	p_adl_ctx->clt_update_mode = 1;//default frame done mode
	p_adl_ctx->reg_conf_mode = 2; //default cpu updatte reg
	p_adl_ctx->preg_tbl = &adl_clt_reg_tbl[clit];

	p_adl_ctx->lut_addr_va = va;
	p_adl_ctx->lut_addr_mva = mva_addr;
	p_adl_ctx->lut_addr_pa = 0;

	p_adl_ctx->src_tbl.hdr_b0103.p_data =
		kmalloc(FE_ADL_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.hdr_b0103.used_size = FE_ADL_SIZE;
	p_adl_ctx->src_tbl.hdr_b0202ss.p_data =
		kmalloc(FE_ADL_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.hdr_b0202ss.used_size = FE_ADL_SIZE;
	p_adl_ctx->src_tbl.hdr_b0202si.p_data =
		kmalloc(FE_ADL_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.hdr_b0202si.used_size = FE_ADL_SIZE;
	p_adl_ctx->src_tbl.hdr_b0202ts.p_data =
		kmalloc(FE_ADL_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.hdr_b0202ts.used_size = FE_ADL_SIZE;
	p_adl_ctx->src_tbl.hdr_b0202ti.p_data =
		kmalloc(FE_ADL_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.hdr_b0202ti.used_size = FE_ADL_SIZE;
	p_adl_ctx->src_tbl.hdr_b0105.p_data =
		kmalloc(FE_ADL_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.hdr_b0105.used_size = FE_ADL_SIZE;
	p_adl_ctx->src_tbl.hdr_ootf.p_data =
		kmalloc(FE_ADL_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.hdr_ootf.used_size = FE_ADL_SIZE;
	p_adl_ctx->src_tbl.tosd_degam.p_data =
		kmalloc(FE_ADL_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.tosd_degam.used_size = FE_ADL_SIZE;
	p_adl_ctx->src_tbl.fgrain.p_data =
		kmalloc(FILMG_SRC_LEN, GFP_KERNEL);
	p_adl_ctx->src_tbl.fgrain.used_size = FILMG_SRC_LEN;
	p_adl_ctx->src_tbl.scmb.p_data =
		kmalloc(MD_MAX_SIZE, GFP_KERNEL);
	p_adl_ctx->src_tbl.scmb.used_size = MD_MAX_SIZE;

	adl_info("adl clt[%d] 0x%x 0x%p 0x%x 0x%p 0x%x\n", clit,
		p_adl_ctx->hw_reg_base, (void *)(p_adl_ctx->remap_reg_base),
		p_adl_ctx->lut_addr_mva, p_adl_ctx->lut_addr_va,
		p_adl_ctx->lut_addr_pa);
	return 0;

}
int disp_adl_init(struct disp_hw_common_info *info)
{
	uint8_t i = 0;

	if (info == NULL) {
		adl_error("hwmgrinfo is null\n");
		return ADL_ERR;
	}
	mutex_init(&disp_adl_mutex);
	/*default mmsys adl clk on*/
	disp_adl_clock_on_off(MMSYS_ADL, true);

	adl_dev = disp_hw_mgr_get_dev();

	/*parser dts file for register base */
	adl_parse_dev_node();

	adl_debug_init();
	adl_hal_init();

	for (i = 0; i < ADL_CLIT_REG_MAX; i++) {
		if (adl_client_ctx_init(i) == ADL_ERR) {
			adl_client_ctx_free();
			adl_error("init failed\n");
			return ADL_ERR;
		}
	}

	disp_adl_thread_init();

	adl_init_done = 1;
	adl_info("init done\n");
	return ADL_OK;
}

int disp_adl_get_clt_buf(enum ADL_CLIENT clit,
	void **va, dma_addr_t *pa)
{
	struct disp_adl_clt_context *p_adl_ctx = NULL;

	switch (clit) {
	case DV_ADL_V_MAIN:
		p_adl_ctx = &adl_clt_ctx[ADL_DSYS_CLITA];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case DV_ADL_V_SUB:
		p_adl_ctx = &adl_clt_ctx[ADL_DSYS_CLITE];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case DV_ADL_G_FHD:
		p_adl_ctx = &adl_clt_ctx[ADL_MSYS_CLITA];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case DV_ADL_G_UHD:
		p_adl_ctx = &adl_clt_ctx[ADL_MSYS_CLITE];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case DV_SCRM:
		p_adl_ctx = &adl_clt_ctx[ADL_MSYS_CLITB];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case THDR_ADL_V_MAIN:
		p_adl_ctx = &adl_clt_ctx[ADL_DSYS_CLITA];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case THDR_ADL_V_SUB:
		p_adl_ctx = &adl_clt_ctx[ADL_DSYS_CLITE];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case THDR_ADL_G_FHD:
		p_adl_ctx = &adl_clt_ctx[ADL_MSYS_CLITC];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case THDR_ADL_G_UHD:
		p_adl_ctx = &adl_clt_ctx[ADL_MSYS_CLITD];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case FILM_GRAIN_MAIN:
		p_adl_ctx = &adl_clt_ctx[ADL_DSYS_CLITB];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	case FILM_GRAIN_SUB:
		p_adl_ctx = &adl_clt_ctx[ADL_DSYS_CLITJ];
		*va = p_adl_ctx->lut_addr_va;
		*pa = p_adl_ctx->lut_addr_mva;
		break;
	default:
		return ADL_ERR;
	}
	adl_info("clt[%d] get buffer 0x%p 0x%x",
		clit, *va, *pa);
	return 0;
}
int disp_adl_deinit(void)
{
	disp_adl_clock_on_off(MMSYS_ADL, false);
	disp_adl_clock_on_off(DISPSYS_ADL, false);

	adl_client_ctx_free();

	adl_printf("%s done\n", __func__);
	return ADL_OK;
}

int disp_adl_isr(void)
{
	adl_irq("irq coming\n");//unitest
	disp_adl_client_flush();
	return ADL_OK;
}

int disp_adl_irq_handler(uint32_t irq)
{
	if (!adl_init_done)
		return ADL_ERR;
	switch (irq) {
	case DISP_IRQ_BEFIFO: //frame done irq
		//disp_adl_isr();
		break;
	default:
		break;
	}
	return ADL_OK;
}

#ifdef CONFIG_HDMI_BLACK
int disp_adl_deep_suspend(void)
{
	disp_adl_clock_on_off(DISPSYS_ADL, false);
	disp_adl_clock_on_off(MMSYS_ADL, false);
	adl_printf("%s done\n", __func__);
	return ADL_OK;
}
#endif
int disp_adl_suspend(void)
{
	if (!disp_common_info.low_energy_dozing_mode_enable)
		disp_adl_clock_on_off(MMSYS_ADL, false);
	disp_adl_clock_on_off(DISPSYS_ADL, false);
	adl_printf("%s done\n", __func__);

	return ADL_OK;
}


int disp_adl_resume(void)
{
	adl_printf("%s start\n", __func__);
	disp_adl_clock_on_off(MMSYS_ADL, true);
	return ADL_OK;
}


int disp_adl_process_cmd(enum DISP_CMD cmd, void *data)
{
	return ADL_OK;
}

int disp_adl_set_client_mode(uint8_t client, uint8_t clt_update_mode,
	uint8_t reg_update_mode)
{
	uint8_t hw_client = 0;
	struct disp_adl_clt_context *p_adl_ctx = NULL;

	switch (client) {
	case DV_ADL_V_MAIN:
	case THDR_ADL_V_MAIN:
		hw_client = ADL_DSYS_CLITA;
		break;
	case DV_ADL_V_SUB:
	case THDR_ADL_V_SUB:
		hw_client = ADL_DSYS_CLITE;
		break;
	case DV_ADL_G_FHD:
		hw_client = ADL_MSYS_CLITA;
		break;
	case DV_ADL_G_UHD:
		hw_client = ADL_MSYS_CLITE;
		break;
	case DV_SCRM:
		hw_client = ADL_MSYS_CLITB;
		break;
	case THDR_ADL_G_FHD:
		hw_client = ADL_MSYS_CLITC;
		break;
	case THDR_ADL_G_UHD:
		hw_client = ADL_MSYS_CLITD;
		break;
	case FILM_GRAIN_MAIN:
		hw_client = ADL_DSYS_CLITB;
		break;
	case FILM_GRAIN_SUB:
		hw_client = ADL_DSYS_CLITJ;
		break;
	default:
		adl_error("%s client error\n", __func__);
		break;
	}

	p_adl_ctx = &adl_clt_ctx[hw_client];
	p_adl_ctx->clt_update_mode = clt_update_mode;
	p_adl_ctx->reg_conf_mode = reg_update_mode;

	adl_info("%s[%d] %d %d\n", __func__, hw_client,
		clt_update_mode, reg_update_mode);
	return 0;
}
int disp_adl_cfg_client_en(uint8_t client, bool en, uint8_t en_mode)
{
	adl_hal_cfg_client_enable(client, en, en_mode);
	return ADL_OK;
}
int disp_adl_cfg_client(uint8_t client, dma_addr_t dst_mva, uint32_t max_len)
{
	adl_hal_cfg_client(client, dst_mva, max_len);
	return ADL_OK;
}

int disp_adl_update_uhd_lut(bool en, uint32_t *addr)
{
	if (addr == NULL)
		return ADL_ERR;

	gfx_lut_update = en;
	gfx_lut_addr = addr;
	adl_info("%s[%d] %p\n", __func__, gfx_lut_update,
		gfx_lut_addr);

	return ADL_OK;
}
int adl_cfg_thdr_v_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint32_t bytes_size = 0;
	uint32_t idx = 0;
	uint8_t *temp_adl_buf;
	uint16_t value2 = 0;
	uint32_t max_len = 0;
	uint8_t *pb0103 = NULL;
	uint8_t *pb0105 = NULL;
	uint8_t *pb02ss = NULL;
	uint8_t *pb02si = NULL;
	uint8_t *pb02ts = NULL;
	uint8_t *pb02ti = NULL;
	uint32_t v_b0103 = 0;
	uint16_t v_b0105 = 0;
	uint16_t v_b02ss = 0;
	uint16_t v_b02si = 0;
	uint16_t v_b02ts = 0;
	uint16_t v_b02ti = 0;

	if (adl_tbl == NULL || dst_buf == NULL) {
		adl_printf("input is null\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video dv main lut*/
	pb0103 = (uint8_t *)(adl_tbl->hdr_b0103.p_data);
	pb0105 = (uint8_t *)(adl_tbl->hdr_b0105.p_data);
	pb02ss = (uint8_t *)(adl_tbl->hdr_b0202ss.p_data);
	pb02si = (uint8_t *)(adl_tbl->hdr_b0202si.p_data);
	pb02ts = (uint8_t *)(adl_tbl->hdr_b0202ts.p_data);
	pb02ti = (uint8_t *)(adl_tbl->hdr_b0202ti.p_data);
	size = FE_ADL_SIZE;
	adl_info("cfd vdo adl 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2 (256x4)
		if (bytes_size > 512) {
			adl_printf("cfd vdo lut size error %d\n", bytes_size);
			return ADL_ERR;
		}

		for (idx = 0; idx < bytes_size; idx++) {
			if (idx < B0103_ADL_SIZE)
				v_b0103 = *(((uint32_t *)pb0103) + idx);
			else
				v_b0103 = 0;
			v_b0105 = *(((uint16_t *)pb0105) + idx);
			v_b02ss = *(((uint16_t *)pb02ss) + idx);
			v_b02si = *(((uint16_t *)pb02si) + idx);
			v_b02ts = *(((uint16_t *)pb02ts) + idx);
			v_b02ti = *(((uint16_t *)pb02ti) + idx);
			WRITE_CFD_VDO_LUTS_DATA(temp_adl_buf, idx, v_b0103,
				v_b0105, v_b02ss, v_b02si, v_b02ts, v_b02ti);

			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	//ootf offset is max_lenxBYTE_PER_LINE
	temp_adl_buf = dst_buf->dst_va;
	temp_adl_buf += max_len * BYTE_PER_LINE;
	/*video ootf*/
	pdata = (uint8_t *)(adl_tbl->hdr_ootf.p_data);
	size = adl_tbl->hdr_ootf.used_size;
	adl_info("thdv ootf 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_OOTF_DATA_FASTMODE(temp_adl_buf, idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}

			max_len += bytes_size;
	}

	//check buffer size
	if (max_len > (dst_buf->size/BYTE_PER_LINE)) {
		adl_printf("dst buffer size err%d %d\n",
			max_len, dst_buf->size);
		return ADL_ERR;
	}

	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}

#if 0
int adl_cfg_thdr_v_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint32_t bytes_size = 0;
	uint32_t idx = 0;
	uint8_t *temp_adl_buf;
	uint32_t value1 = 0;
	uint16_t value2 = 0;
	uint32_t max_len = 0;

	if (adl_tbl == NULL || dst_buf == NULL) {
		adl_printf("input is null\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0103 lut*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0103.p_data);
	size = adl_tbl->hdr_b0103.used_size;
	adl_info("thdvb0103 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint32_t);
		// max is 256 because lut size is 256x4
		if (bytes_size > 256) {
			adl_printf("b0103 lut size error %d\n", bytes_size);
			return ADL_ERR;
		}

		for (idx = 0; idx < bytes_size; idx++) {
			value1 = *(((uint32_t *)pdata) + idx);
			WRITE_B0103_SMLUTS_DATA_FASTMODE(temp_adl_buf,
				idx, value1);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0105 lut*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0105.p_data);
	size = adl_tbl->hdr_b0105.used_size;
	adl_info("thdvb0105 0x%p %d\n", pdata, size);

	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0105 lut size error %d\n", bytes_size);
			return ADL_ERR;
		}

		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0105_DATA_FASTMODE(temp_adl_buf, idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202SS*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ss.p_data);
	size = adl_tbl->hdr_b0202ss.used_size;
	adl_info("thdvb0202ss 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0202_SMLUTS_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202SI*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202si.p_data);
	size = adl_tbl->hdr_b0202si.used_size;
	adl_info("thdvb0202si 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202si lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0202_SMLUTI_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202TS*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ts.p_data);
	size = adl_tbl->hdr_b0202ts.used_size;
	adl_info("thdvb0202ts 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0202_TMLUTS_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202TI*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ti.p_data);
	size = adl_tbl->hdr_b0202ti.used_size;
	adl_info("thdvb0202ti 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0202_TMLUTI_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	//ootf offset is max_lenxBYTE_PER_LINE
	temp_adl_buf = dst_buf->dst_va;
	temp_adl_buf += max_len * BYTE_PER_LINE;
	/*video ootf*/
	pdata = (uint8_t *)(adl_tbl->hdr_ootf.p_data);
	size = adl_tbl->hdr_ootf.used_size;
	adl_info("thdv ootf 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_OOTF_DATA_FASTMODE(temp_adl_buf, idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}

			max_len += bytes_size;
	}

	//check buffer size
	if (max_len > (dst_buf->size/BYTE_PER_LINE)) {
		adl_printf("dst buffer size err%d %d\n",
			max_len, dst_buf->size);
		return ADL_ERR;
	}

	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}
#endif

int adl_cfg_thdr_g_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint32_t bytes_size = 0;
	uint32_t idx = 0;
	uint8_t *temp_adl_buf;

	uint16_t value = 0;
	uint32_t max_len = 0;

	if (adl_tbl == NULL || dst_buf == NULL) {
		adl_printf("input is null\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*gop degamma*/
	pdata = (uint8_t *)(adl_tbl->tosd_degam.p_data);
	size = adl_tbl->tosd_degam.used_size;
	adl_info("tgop dgma 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512
		if (bytes_size > 512) {
			adl_printf("degamma lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		//ADL cmd to enable R/G/B
		//*temp_adl_buf = (BIT(2) | BIT(1) | BIT(0));
		*temp_adl_buf = BIT(0);
		max_len++;
		temp_adl_buf += BYTE_PER_LINE;

		for (idx = 0; idx < bytes_size; idx++) {
			value = *(((uint16_t *)pdata) + idx);
			WRITE_TOSD_DEGAMMA_DATA_FASTMODE(temp_adl_buf,
				idx, value);
		}
		max_len = max_len + (size / 32);
	}

	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}


int adl_cfg_fgrain_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint8_t *temp_adl_buf;
	uint8_t valuey = 0;
	uint8_t valuecb = 0;
	uint8_t valuecr = 0;
	uint32_t max_len = 0;
	uint32_t i = 0;
	uint32_t j = 0;


	if (adl_tbl == NULL || dst_buf == NULL
		|| ((adl_tbl->fgrain.used_size) != FILMG_SRC_LEN)) {
		adl_printf("va is null or len error\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*film grain scale factor*/
	pdata = (uint8_t *)(adl_tbl->fgrain.p_data);
	size = adl_tbl->fgrain.used_size;
	adl_info("fgrain 0x%p %d\n", pdata, size);
	if (size != 0) {
		for (i = 0; i < FILMG_SF_LEN; i++) {
			valuey = *(pdata + 0);
			valuecb = *(pdata + 1);
			valuecr = *(pdata + 2);
			WRITE_FILMG_SF_DATA_FASTMODE(temp_adl_buf,
			valuey, valuecb, valuecr);
			pdata += FILMG_SF_CMD;
			temp_adl_buf += BYTE_PER_LINE;
			max_len++;
		}

		for (i = 0; i < FILMG_YN_LEN; i++) {
			for (j = 0; j < FILMG_YN_CMD; j++) {
				valuey = *(pdata + j);
				WRITE_FILMG_YN_DATA_FASTMODE(temp_adl_buf + j,
					valuey);
			}
			pdata += FILMG_YN_CMD;
			temp_adl_buf += BYTE_PER_LINE;
			max_len++;
		}

		for (i = 0; i < FILMG_CBCRN_LEN; i++) {
			for (j = 0; j < FILMG_CBCRN_CMD; j++) {
				valuey = *(pdata + j);
				WRITE_FILMG_YN_DATA_FASTMODE(temp_adl_buf + j,
					valuey);
			}
			pdata += FILMG_CBCRN_CMD;
			temp_adl_buf += BYTE_PER_LINE;
			max_len++;
		}

	}
	//check buffer size
	if (max_len > FILMG_MAX_CMD_LEN) {
		adl_printf("dst buffer size err%d %d\n",
			max_len, dst_buf->size);
		return ADL_ERR;
	}

	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}




int adl_cfg_scrmble_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint32_t bytes_size = 0;
	uint32_t idx = 0;
	uint8_t *temp_adl_buf;
	uint8_t value = 0;
	uint32_t max_len = 0;

	if (adl_tbl == NULL || dst_buf == NULL) {
		adl_printf("va is null\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*be scm*/
	pdata = (uint8_t *)(adl_tbl->scmb.p_data);
	size = adl_tbl->scmb.used_size;
	adl_info("be scm 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint8_t);
		// max is 2048,but current buffer max cmd is 2048
		// scm md size must be 124 bytes align
		if ((bytes_size > MD_MAX_SIZE) ||
			(bytes_size % MD_PKT_SIZE != 0)) {
			adl_printf("scramble lut size error %d\n", bytes_size);
			return ADL_ERR;
		}

		for (idx = 0; idx < bytes_size; idx++) {
			value = *(((uint8_t *)pdata) + idx);
			WRITE_SCRAMBLE_DATA_FASTMODE(temp_adl_buf, idx, value);
			temp_adl_buf += BYTE_PER_LINE;
		}

		if (max_len < bytes_size)
			max_len = bytes_size;
	}
	//check buffer size
	if (max_len > (dst_buf->size/BYTE_PER_LINE)) {
		adl_printf("dst buffer size err%d %d\n",
			max_len, dst_buf->size);
		return ADL_ERR;
	}

	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}

int adl_cfg_dv_g_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint32_t bytes_size = 0;
	uint32_t idx = 0;
	uint8_t *temp_adl_buf;
	uint32_t max_len = 0;
	uint8_t *pb0103 = NULL;
	uint8_t *pb02ss = NULL;
	uint8_t *pb02si = NULL;
	uint8_t *pb02ts = NULL;
	uint8_t *pb02ti = NULL;
	uint32_t v_b0103 = 0;
	uint16_t v_b02ss = 0;
	uint16_t v_b02si = 0;
	uint16_t v_b02ts = 0;
	uint16_t v_b02ti = 0;

	if (adl_tbl == NULL || dst_buf == NULL) {
		adl_printf("input is null\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*dv gfx lut*/
	pb0103 = (uint8_t *)(adl_tbl->hdr_b0103.p_data);
	pb02ss = (uint8_t *)(adl_tbl->hdr_b0202ss.p_data);
	pb02si = (uint8_t *)(adl_tbl->hdr_b0202si.p_data);
	pb02ts = (uint8_t *)(adl_tbl->hdr_b0202ts.p_data);
	pb02ti = (uint8_t *)(adl_tbl->hdr_b0202ti.p_data);
	size = adl_tbl->hdr_b0103.used_size;
	adl_info("dv gfx adl 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2 (256x4)
		if (bytes_size > 512) {
			adl_printf("dv gfx lut size error %d\n", bytes_size);
			return ADL_ERR;
		}

		for (idx = 0; idx < bytes_size; idx++) {
			if (idx < B0103_ADL_SIZE)
				v_b0103 = *(((uint32_t *)pb0103) + idx);
			else
				v_b0103 = 0;
			v_b02ss = *(((uint16_t *)pb02ss) + idx);
			v_b02si = *(((uint16_t *)pb02si) + idx);
			v_b02ts = *(((uint16_t *)pb02ts) + idx);
			v_b02ti = *(((uint16_t *)pb02ti) + idx);
			WRITE_DV_LUTS_DATA(temp_adl_buf, idx, v_b0103, v_b02ss,
				v_b02si, v_b02ts, v_b02ti);

			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	//check buffer size
	if (max_len > ((dst_buf->size) / BYTE_PER_LINE)) {
		adl_printf("dst buffer size err %d %d\n",
			max_len, dst_buf->size);
		return ADL_ERR;
	}

	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}

int adl_cfg_dv_v_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint32_t bytes_size = 0;
	uint32_t idx = 0;
	uint8_t *temp_adl_buf;
	uint32_t max_len = 0;
	uint8_t *pb0103 = NULL;
	uint8_t *pb02ss = NULL;
	uint8_t *pb02si = NULL;
	uint8_t *pb02ts = NULL;
	uint8_t *pb02ti = NULL;
	uint32_t v_b0103 = 0;
	uint16_t v_b02ss = 0;
	uint16_t v_b02si = 0;
	uint16_t v_b02ts = 0;
	uint16_t v_b02ti = 0;

	if (adl_tbl == NULL || dst_buf == NULL) {
		adl_printf("input is null\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;

	/*dv gfx lut*/
	pb0103 = (uint8_t *)(adl_tbl->hdr_b0103.p_data);
	pb02ss = (uint8_t *)(adl_tbl->hdr_b0202ss.p_data);
	pb02si = (uint8_t *)(adl_tbl->hdr_b0202si.p_data);
	pb02ts = (uint8_t *)(adl_tbl->hdr_b0202ts.p_data);
	pb02ti = (uint8_t *)(adl_tbl->hdr_b0202ti.p_data);
	size = adl_tbl->hdr_b0103.used_size;
	adl_info("dv gfx adl 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2 (256x4)
		if (bytes_size > 512) {
			adl_printf("dv gfx lut size error %d\n", bytes_size);
			return ADL_ERR;
		}

		for (idx = 0; idx < bytes_size; idx++) {
			if (idx < B0103_ADL_SIZE)
				v_b0103 = *(((uint32_t *)pb0103) + idx);
			else
				v_b0103 = 0;
			v_b02ss = *(((uint16_t *)pb02ss) + idx);
			v_b02si = *(((uint16_t *)pb02si) + idx);
			v_b02ts = *(((uint16_t *)pb02ts) + idx);
			v_b02ti = *(((uint16_t *)pb02ti) + idx);
			WRITE_DV_LUTS_DATA(temp_adl_buf, idx, v_b0103, v_b02ss,
				v_b02si, v_b02ts, v_b02ti);

			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	//check buffer size
	if (max_len > ((dst_buf->size) / BYTE_PER_LINE)) {
		adl_printf("dst buffer size err%d %d\n",
			max_len, dst_buf->size);
		return ADL_ERR;
	}

	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}

#if 0
int adl_cfg_dv_g_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint32_t bytes_size = 0;
	uint32_t idx = 0;
	uint8_t *temp_adl_buf;
	uint32_t value1 = 0;
	uint16_t value2 = 0;
	uint32_t max_len = 0;

	if (adl_tbl == NULL || dst_buf == NULL) {
		adl_printf("input is null\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;

	/*gfx b0103 lut*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0103.p_data);
	size = adl_tbl->hdr_b0103.used_size;
	adl_info("gfxb0103 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint32_t);
		// max is 256 because lut size is 256x4
		if (bytes_size > 256) {
			adl_printf("b0103 lut size error %d\n", bytes_size);
			return ADL_ERR;
		}

		for (idx = 0; idx < bytes_size; idx++) {
			value1 = *(((uint32_t *)pdata) + idx);
			WRITE_OSD_B0103_DATA_FASTMODE(temp_adl_buf,
				idx, value1);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202SS*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ss.p_data);
	size = adl_tbl->hdr_b0202ss.used_size;
	adl_info("gfxb02ss 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_OSD_B0202_SMLUTS_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202SI*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202si.p_data);
	size = adl_tbl->hdr_b0202si.used_size;
	adl_info("gfxb02si 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202si lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_OSD_B0202_SMLUTI_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202TS*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ts.p_data);
	size = adl_tbl->hdr_b0202ts.used_size;
	adl_info("gfxb02ts 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_OSD_B0202_TMLUTS_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202TI*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ti.p_data);
	size = adl_tbl->hdr_b0202ti.used_size;
	adl_info("gfxb02ti 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_OSD_B0202_TMLUTI_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}
	//check buffer size
	if (max_len > ((dst_buf->size) / BYTE_PER_LINE)) {
		adl_printf("dst buffer size err %d %d\n",
			max_len, dst_buf->size);
		return ADL_ERR;
	}

	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}

int adl_cfg_dv_v_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf)
{
	uint8_t *pdata = NULL;
	uint32_t size = 0;
	uint32_t bytes_size = 0;
	uint32_t idx = 0;
	uint8_t *temp_adl_buf;
	uint32_t value1 = 0;
	uint16_t value2 = 0;
	uint32_t max_len = 0;

	if (adl_tbl == NULL || dst_buf == NULL) {
		adl_printf("input is null\n");
		return ADL_ERR;
	}

	temp_adl_buf = dst_buf->dst_va;

	/*video b0103 lut*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0103.p_data);
	size = adl_tbl->hdr_b0103.used_size;
	adl_info("b0103 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint32_t);
		// max is 256 because lut size is 256x4
		if (bytes_size > 256) {
			adl_printf("b0103 lut size error %d\n", bytes_size);
			return ADL_ERR;
		}

		for (idx = 0; idx < bytes_size; idx++) {
			value1 = *(((uint32_t *)pdata) + idx);
			WRITE_B0103_SMLUTS_DATA_FASTMODE(temp_adl_buf,
				idx, value1);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202SS*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ss.p_data);
	size = adl_tbl->hdr_b0202ss.used_size;
	adl_info("b0202ss 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512X2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0202_SMLUTS_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202SI*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202si.p_data);
	size = adl_tbl->hdr_b0202si.used_size;
	adl_info("b0202si 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202si lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0202_SMLUTI_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202TS*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ts.p_data);
	size = adl_tbl->hdr_b0202ts.used_size;
	adl_info("b0202ts 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0202_TMLUTS_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}

	temp_adl_buf = dst_buf->dst_va;
	/*video b0202TI*/
	pdata = (uint8_t *)(adl_tbl->hdr_b0202ti.p_data);
	size = adl_tbl->hdr_b0202ti.used_size;
	adl_info("b0202ti 0x%p %d\n", pdata, size);
	if (size != 0) {
		bytes_size = size / sizeof(uint16_t);
		// max is 512 because lut size is 512x2
		if (bytes_size > 512) {
			adl_printf("b0202ss lut size error %d\n", bytes_size);
			return ADL_ERR;
		}
		for (idx = 0; idx < bytes_size; idx++) {
			value2 = *(((uint16_t *)pdata) + idx);
			WRITE_B0202_TMLUTI_DATA_FASTMODE(temp_adl_buf,
				idx, value2);
			temp_adl_buf += BYTE_PER_LINE;
		}
		if (max_len < bytes_size)
			max_len = bytes_size;
	}
	//check buffer size
	if (max_len > ((dst_buf->size) / BYTE_PER_LINE)) {
		adl_printf("dst buffer size err%d %d\n",
			max_len, dst_buf->size);
		return ADL_ERR;
	}


	adl_hal_cfg_client(adl_tbl->client, dst_buf->dst_mva, max_len);

	return ADL_OK;
}
#endif
uint8_t disp_adl_get_hwclt_id(enum ADL_CLIENT clit)
{
	uint8_t hw_client = 0;

	switch (clit) {
	case DV_ADL_V_MAIN:
	case THDR_ADL_V_MAIN:
		hw_client = ADL_DSYS_CLITA;
		break;
	case DV_ADL_V_SUB:
	case THDR_ADL_V_SUB:
		hw_client = ADL_DSYS_CLITE;
		break;
	case DV_ADL_G_FHD:
		hw_client = ADL_MSYS_CLITA;
		break;
	case DV_ADL_G_UHD:
		hw_client = ADL_MSYS_CLITE;
		break;
	case DV_SCRM:
		hw_client = ADL_MSYS_CLITB;
		break;
	case THDR_ADL_G_FHD:
		hw_client = ADL_MSYS_CLITC;
		break;
	case THDR_ADL_G_UHD:
		hw_client = ADL_MSYS_CLITD;
		break;
	case FILM_GRAIN_MAIN:
		hw_client = ADL_DSYS_CLITB;
		break;
	case FILM_GRAIN_SUB:
		hw_client = ADL_DSYS_CLITJ;
		break;
	default:
		hw_client = ADL_DSYS_CLITA;
		adl_error("adl client id err\n");
	}

	return hw_client;
}

int disp_filmg_config_adl_table(struct adl_src_tbl *adl_tbl)
{
	enum ADL_CLIENT clit = ADL_CLIENT_MAX;
	struct disp_adl_clt_context *p_adl_ctx = NULL;
	struct adl_dst_buf dst_buf = { 0 };
	uint8_t hw_client = 0;
	uint32_t src_size = 0;

	if (adl_tbl == NULL) {
		adl_error("adl_tbl null\n");
		return ADL_ERR;
	}

	clit = (enum ADL_CLIENT)(adl_tbl->client);

	if (clit >= ADL_CLIENT_MAX) {
		adl_error("client id or dstaddr\n");
		return ADL_ERR;
	}

	hw_client = disp_adl_get_hwclt_id(clit);

	p_adl_ctx = &adl_clt_ctx[hw_client];

	adl_info("clt[%d][%d] va:0x%p mva:0x%x\n", clit, hw_client,
		p_adl_ctx->lut_addr_va, p_adl_ctx->lut_addr_mva);

	src_size = adl_tbl->fgrain.used_size;
	if (src_size > FILMG_SRC_LEN) {
		adl_error("adl[%d] fg table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}

	switch (clit) {
	case FILM_GRAIN_MAIN:
		//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
		dst_buf.dst_va = p_adl_ctx->lut_addr_va;
		dst_buf.dst_mva = p_adl_ctx->lut_addr_mva;
		dst_buf.size = ADL_LUT_BUFFER_SIZE;
		adl_cfg_fgrain_adl_table(adl_tbl, &dst_buf);
		break;
	case FILM_GRAIN_SUB:
		//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
		dst_buf.dst_va = p_adl_ctx->lut_addr_va;
		dst_buf.dst_mva = p_adl_ctx->lut_addr_mva;
		dst_buf.size = ADL_LUT_BUFFER_SIZE;
		adl_cfg_fgrain_adl_table(adl_tbl, &dst_buf);
		break;
	default:
		adl_printf("filmg clit error\n");
		break;
	}

	return ADL_OK;

}

int disp_config_adl_table(struct adl_src_tbl *adl_tbl)
{
	enum ADL_CLIENT clit = ADL_CLIENT_MAX;
	struct disp_adl_clt_context *p_adl_ctx = NULL;
	uint8_t hw_client = 0;
	uint32_t src_size = 0;
	uint8_t cur_path = 0;

	if (adl_tbl == NULL) {
		adl_error("adl_tbl null\n");
		return ADL_ERR;
	}

	clit = (enum ADL_CLIENT)(adl_tbl->client);

	if (clit >= ADL_CLIENT_MAX) {
		adl_error("client id or dstaddr\n");
		return ADL_ERR;
	}

	disp_hdr_get_cur_path(&cur_path);
	if (((cur_path == ADL_DOVI) || (cur_path == ADL_OPENHDR))
		&& (cur_path != adl_tbl->path)) {
		adl_printf("%d path not match %d %d\n", clit,
			cur_path, adl_tbl->path);
		return ADL_ERR;
	}

	hw_client = disp_adl_get_hwclt_id(clit);

	p_adl_ctx = &adl_clt_ctx[hw_client];

	adl_info("clt[%d][%d] va:0x%p mva:0x%x\n", clit, hw_client,
		p_adl_ctx->lut_addr_va, p_adl_ctx->lut_addr_mva);

	// dv cfd fg need keep src buffer,here only copy buffer addr
	//memcpy(&p_adl_ctx->src_tbl, adl_tbl, sizeof(struct adl_src_tbl));

	src_size = adl_tbl->hdr_b0103.used_size;
	if (src_size > FE_ADL_SIZE) {
		adl_error("adl[%d] b0103 table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.hdr_b0103.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.hdr_b0103.p_data,
			adl_tbl->hdr_b0103.p_data, src_size);
		p_adl_ctx->src_tbl.hdr_b0103.used_size = src_size;

	}

	src_size = adl_tbl->hdr_b0202ss.used_size;
	if (src_size > FE_ADL_SIZE) {
		adl_error("adl[%d] b02ss table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.hdr_b0202ss.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.hdr_b0202ss.p_data,
			adl_tbl->hdr_b0202ss.p_data, src_size);
		p_adl_ctx->src_tbl.hdr_b0202ss.used_size = src_size;
	}

	src_size = adl_tbl->hdr_b0202si.used_size;
	if (src_size > FE_ADL_SIZE) {
		adl_error("adl[%d] b02si table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.hdr_b0202si.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.hdr_b0202si.p_data,
			adl_tbl->hdr_b0202si.p_data, src_size);
		p_adl_ctx->src_tbl.hdr_b0202si.used_size = src_size;
	}

	src_size = adl_tbl->hdr_b0202ts.used_size;
	if (src_size > FE_ADL_SIZE) {
		adl_error("adl[%d] b02ts table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.hdr_b0202ts.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.hdr_b0202ts.p_data,
			adl_tbl->hdr_b0202ts.p_data, src_size);
		p_adl_ctx->src_tbl.hdr_b0202ts.used_size = src_size;
	}

	src_size = adl_tbl->hdr_b0202ti.used_size;
	if (src_size > FE_ADL_SIZE) {
		adl_error("adl[%d] b02si table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.hdr_b0202ti.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.hdr_b0202ti.p_data,
			adl_tbl->hdr_b0202ti.p_data, src_size);
		p_adl_ctx->src_tbl.hdr_b0202ti.used_size = src_size;
	}

	src_size = adl_tbl->hdr_b0105.used_size;
	if (src_size > FE_ADL_SIZE) {
		adl_error("adl[%d] b0105 table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.hdr_b0105.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.hdr_b0105.p_data,
			adl_tbl->hdr_b0105.p_data, src_size);
		p_adl_ctx->src_tbl.hdr_b0105.used_size = src_size;
	}

	src_size = adl_tbl->hdr_ootf.used_size;
	if (src_size > FE_ADL_SIZE) {
		adl_error("adl[%d] ootf table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.hdr_ootf.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.hdr_ootf.p_data,
			adl_tbl->hdr_ootf.p_data, src_size);
		p_adl_ctx->src_tbl.hdr_ootf.used_size = src_size;
	}

	src_size = adl_tbl->tosd_degam.used_size;
	if (src_size > FE_ADL_SIZE) {
		adl_error("adl[%d] degam table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.tosd_degam.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.tosd_degam.p_data,
			adl_tbl->tosd_degam.p_data, src_size);
		p_adl_ctx->src_tbl.tosd_degam.used_size = src_size;
	}

	#if 0
	src_size = adl_tbl->fgrain.used_size;
	if (src_size > FILMG_SRC_LEN) {
		adl_error("adl[%d] fg table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.fgrain.p_data, 0, FE_ADL_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.fgrain.p_data,
			adl_tbl->fgrain.p_data, src_size);
		p_adl_ctx->src_tbl.fgrain.used_size = src_size;
	}
	#endif

	src_size = adl_tbl->scmb.used_size;
	if (src_size > MD_MAX_SIZE) {
		adl_error("adl[%d] scm table oversize %d\n", clit, src_size);
		return ADL_ERR;
	}
	//memset(p_adl_ctx->src_tbl.scmb.p_data, 0, MD_MAX_SIZE);
	if (src_size > 0) {
		memcpy(p_adl_ctx->src_tbl.scmb.p_data,
			adl_tbl->scmb.p_data, src_size);
		p_adl_ctx->src_tbl.scmb.used_size = src_size;
	}
	p_adl_ctx->src_tbl.client = adl_tbl->client;
	p_adl_ctx->src_tbl.update = true;

	return ADL_OK;

}

int disp_adl_main(void)
{
	enum ADL_CLIENT clit = ADL_CLIENT_MAX;
	struct adl_dst_buf dst_buf = { 0 };
	struct adl_src_tbl *adl_tbl = NULL;
	struct disp_adl_clt_context *p_adl_ctx = NULL;
	void *dst_va = NULL;
	dma_addr_t dst_mva = 0;
	uint8_t hw_clt = 0;

	mutex_lock(&disp_adl_thread_mutex);

	for (hw_clt = ADL_DSYS_CLITA; hw_clt < ADL_CLIT_REG_MAX; hw_clt++) {
		p_adl_ctx = &adl_clt_ctx[hw_clt];

		if (p_adl_ctx->src_tbl.update == false)
			continue;
		clit = p_adl_ctx->src_tbl.client;
		if (clit >= ADL_CLIENT_MAX) {
			adl_error("%s clit error\n", __func__);
			return ADL_ERR;
		}
		adl_tbl = &p_adl_ctx->src_tbl;
		dst_va = p_adl_ctx->lut_addr_va;
		dst_mva = p_adl_ctx->lut_addr_mva;
		adl_dbg("clt[%d][%d] va:0x%p mva:0x%x\n", clit, hw_clt,
			dst_va, dst_mva);
		switch (clit) {
		case DV_ADL_V_MAIN:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_dv_v_adl_table(adl_tbl, &dst_buf);
			break;
		case DV_ADL_V_SUB:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_dv_v_adl_table(adl_tbl, &dst_buf);
			break;
		case DV_ADL_G_FHD:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_dv_g_adl_table(adl_tbl, &dst_buf);

			if (gfx_lut_update) {
				memcpy((void *)gfx_lut_addr, dst_va, GFX_FE_LUT_SIZE);
				gfx_lut_update = false;
				adl_printf("uhd lut update done\n");
			}
			break;
		case DV_ADL_G_UHD:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_dv_g_adl_table(adl_tbl, &dst_buf);
			break;
		case DV_SCRM:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_scrmble_adl_table(adl_tbl, &dst_buf);
			break;
		case THDR_ADL_V_MAIN:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_thdr_v_adl_table(adl_tbl, &dst_buf);
			break;
		case THDR_ADL_V_SUB:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_thdr_v_adl_table(adl_tbl, &dst_buf);
			break;
		case THDR_ADL_G_FHD:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_thdr_g_adl_table(adl_tbl, &dst_buf);
			break;
		case THDR_ADL_G_UHD:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_thdr_g_adl_table(adl_tbl, &dst_buf);
			break;
		case FILM_GRAIN_MAIN:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_fgrain_adl_table(adl_tbl, &dst_buf);
			break;
		case FILM_GRAIN_SUB:
			//memset(dst_va, 0, ADL_LUT_BUFFER_SIZE);
			dst_buf.dst_va = dst_va;
			dst_buf.dst_mva = dst_mva;
			dst_buf.size = ADL_LUT_BUFFER_SIZE;
			adl_cfg_fgrain_adl_table(adl_tbl, &dst_buf);
			break;
		default:
			mutex_unlock(&disp_adl_thread_mutex);
			return ADL_ERR;
		}
		p_adl_ctx->src_tbl.update = false;
	}

	mutex_unlock(&disp_adl_thread_mutex);
	return ADL_OK;
}

int disp_adl_client_flush(void)
{
	uint32_t i = 0;
	struct disp_adl_clt_context *p_adl_ctx = NULL;

	for (i = 0 ; i < ADL_CLIT_REG_MAX; i++) {
		p_adl_ctx = &adl_clt_ctx[i];
		if (p_adl_ctx->enabled && p_adl_ctx->inited
			&& (p_adl_ctx->preg_tbl->depth > 0)) {
			adl_dbg("flush clt[%d] %d\n", i,
				p_adl_ctx->preg_tbl->depth);
			adl_hal_client_flush(p_adl_ctx);
			p_adl_ctx->enabled = 0;
		}
	}

	return 0;
}

int disp_adl_trigger_by_gce(void)
{
	mutex_lock(&disp_adl_mutex);
	disp_adl_client_flush();
	mutex_unlock(&disp_adl_mutex);
	return 0;
}
int disp_adl_client_status(void)
{
	struct disp_adl_clt_context *p_adl_clt_ctx = NULL;
	uint8_t i = 0;

	for (i = 0 ; i < ADL_CLIT_REG_MAX; i++) {
		p_adl_clt_ctx = &adl_clt_ctx[i];
		adl_printf("clt[%d] %d %d %d %d\n", i, p_adl_clt_ctx->inited,
			p_adl_clt_ctx->enabled,
			p_adl_clt_ctx->clt_update_mode,
			p_adl_clt_ctx->reg_conf_mode);
		adl_printf("reg 0x%p 0x%x\n",
			(void *)p_adl_clt_ctx->remap_reg_base,
			p_adl_clt_ctx->hw_reg_base);
		adl_printf("mem 0x%p 0x%x 0x%x 0x%p\n",
			p_adl_clt_ctx->lut_addr_va,
			p_adl_clt_ctx->lut_addr_mva,
			p_adl_clt_ctx->lut_addr_pa,
			p_adl_clt_ctx->preg_tbl);

	}
	return 0;
}
/*****define autodownload driver*****/
struct disp_hw disp_adl_drv = {
	.name = ADL_DRV_NAME,
	.init = disp_adl_init,
	.deinit = disp_adl_deinit,
	.start = NULL,
	.stop = NULL,
#ifdef CONFIG_HDMI_BLACK
	.deep_suspend = disp_adl_deep_suspend,
	.deep_resume = NULL,
#endif
	.suspend = disp_adl_suspend,
	.resume = disp_adl_resume,
	.get_info = NULL,
	.change_resolution = NULL,
	.config = NULL,
	.irq_handler = disp_adl_irq_handler,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = NULL,
	.set_cmd = disp_adl_process_cmd,
};

struct disp_hw *disp_adl_get_drv(void)
{
	return &disp_adl_drv;
}

