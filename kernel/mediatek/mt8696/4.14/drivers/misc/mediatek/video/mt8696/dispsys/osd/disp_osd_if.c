/*
 * Copyright (C) 2017 MediaTek Inc.
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

#define LOG_TAG "drv_osd_if"

#include <linux/kthread.h>

#include <linux/cdev.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/file.h>
#include <linux/interrupt.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <mt-plat/sync_write.h>
#include <uapi/linux/sched/types.h>

#include "disp_osd_fence.h"
#include "disp_osd_if.h"
#include "osd_hw.h"
#include "osd_sw.h"

#include "disp_osd_ion.h"

#include "disp_osd_log.h"
#include "mtk_sync.h"
#include "sync_file.h"
#include "disp_clk.h"
#include "disp_irq.h"

#include "disp_hw_log.h"
#include "disp_hw_mgr.h"
#include "disp_path.h"
#include "disp_mix_hal.h"

#include <linux/soc/mediatek/mtk-cmdq.h>
#include "cmdq_record.h"
#include "cmdq_virtual.h"

#include "disp_hdr_if.h"
#include "disp_fefifo_drv.h"
#include "disp_fefifo_if.h"
/*elvis temp notes
 *#include "disp_pmx_if.h"
 *#include "disp_dovi_common_if.h"
 *#include "hdr_config_if.h"
 */

atomic_t irq_fmt_vsync_start_flag = ATOMIC_INIT(0);
wait_queue_head_t vsync_start_flag;

struct task_struct *osd_irq_task[MAX_OSD_INPUT_CONFIG];
wait_queue_head_t osd_irq_wq[MAX_OSD_INPUT_CONFIG];
atomic_t osd_irq_event[MAX_OSD_INPUT_CONFIG] = {ATOMIC_INIT(0)};

struct task_struct *osd_config_task;
wait_queue_head_t osd_config_wq;
atomic_t osd_config_event = ATOMIC_INIT(0);

uint32_t osd_layer[MAX_OSD_INPUT_CONFIG] = {OSD_PLANE_1, OSD_PLANE_2};
unsigned long vsync_cnt, vsync_cnt1;
bool find_current[MAX_OSD_INPUT_CONFIG];
unsigned long config_cnt[MAX_OSD_INPUT_CONFIG];
unsigned long release_cnt[MAX_OSD_INPUT_CONFIG];

static struct list_head OSD_Buffer_Head[MAX_OSD_INPUT_CONFIG];
static struct list_head OSD_Configed_Head[MAX_OSD_INPUT_CONFIG];

struct Osd_buffer_list *pview[MAX_OSD_INPUT_CONFIG] = {NULL};

static BOOL fgupdate[MAX_OSD_INPUT_CONFIG];
static BOOL fg_config_update[MAX_OSD_INPUT_CONFIG];
static BOOL fgupdate_ex[MAX_OSD_INPUT_CONFIG];
static BOOL fg_alpha_det_update[MAX_OSD_INPUT_CONFIG];
bool is_stop_plane_done = true;

/*for enable faker function*/
BOOL fg_enable_faker_hdr;
/*for hdr10 plus video play*/
static int hdr10p_delay_done_flag;

bool fg_dovi_idk_test;
bool fg_debug_update;
UINT32 m_uptimes;
bool fg_osd_alpha_detect_en;
bool fg_osd_debug_dump_en[MAX_OSD_INPUT_CONFIG];
struct osd_context_t osd;
bool trigger[MAX_OSD_INPUT_CONFIG];
struct mtk_disp_buffer *osd_stop_config;

struct cmdq_update_pkt osd_update_pkt[2][UPDATE_PKT_COUNT];
#if OSD_SUPPORT_GCE
struct cmdq_base *osd_clt_base;
struct cmdq_client *osd_trigger_loop_cl;
struct cmdq_client *osd_update_cl;
unsigned int osd_frame_end_event_id;
struct cmdq_update_pkt osd_trigger_loop_pkt;
#endif

/* ----------------------plane config---------------------
 */
static int _osd_plane_map(unsigned int lay_id)
{
	switch (lay_id) {
	case 0:
		return OSD_PLANE_2; /*osd 3*/
	case 1:
		return OSD_PLANE_1; /*osd 2*/
	default:
		return OSD_PLANE_1;
	}
}
static phys_addr_t _get_osd_node_PA(struct device_node *node, int index)
{
	struct resource res;
	phys_addr_t regBasePA = 0L;

	do {
		if (of_address_to_resource(node, index, &res) < 0)
			break;

		regBasePA = (0L | res.start);
	} while (0);

	return regBasePA;
}
static int _osd_parse_dev_node(void)
{
	struct device_node *np;
	unsigned int reg_value;
	unsigned int irq_value;
	struct disp_hw *osd_drv = disp_osd_get_drv();
#if OSD_SUPPORT_GCE
	struct device *dev = disp_hw_mgr_get_dev();
	struct of_phandle_args spec;
#endif
//#if !OSD_IOMMU_SUPPORT
	/*for fpga test*/
	uintptr_t mmu_reg;
//#endif
	/*unsigned int irq_value; */

	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8696-osd");
	if (np == NULL) {
		OSDERR("dts error, no osd device node.\n");
		return OSD_RET_DTS_FAIL;
	}

	of_property_read_u32_index(np, "reg", 1, &reg_value);
	of_property_read_u32_index(np, "interrupts", 1, &irq_value);

	/*todo: osd register for 8695*/
	osd.osd_reg.osd_fmt_reg_base[0] =
		(uintptr_t)of_iomap(np, 0); /*osd2: 0x14004000*/
	osd.osd_reg.osd_pln_reg_base[0] =
		osd.osd_reg.osd_fmt_reg_base[0] + 0x100;
	osd.osd_reg.osd_scl_reg_base[0] =
		osd.osd_reg.osd_fmt_reg_base[0] + 0x200;
	osd.osd_reg.osd_reg_base_pa[0] =
		_get_osd_node_PA(np, 0);
	osd.osd_reg.osd_fmt_reg_base_pa[0] =
		osd.osd_reg.osd_reg_base_pa[0];
	osd.osd_reg.osd_pln_reg_base_pa[0] =
		osd.osd_reg.osd_reg_base_pa[0] + 0x100;
	osd.osd_reg.osd_scl_reg_base_pa[0] =
		osd.osd_reg.osd_reg_base_pa[0] + 0x200;

	osd.osd_reg.osd_fmt_reg_base[1] =
		(uintptr_t)of_iomap(np, 1); /*osd3: 0x14003000 */
	osd.osd_reg.osd_pln_reg_base[1] =
		osd.osd_reg.osd_fmt_reg_base[1] + 0x100;
	osd.osd_reg.osd_scl_reg_base[1] =
		osd.osd_reg.osd_fmt_reg_base[1] + 0x200;
	osd.osd_reg.osd_reg_base_pa[1] =
		_get_osd_node_PA(np, 1);
	osd.osd_reg.osd_fmt_reg_base_pa[1] =
		osd.osd_reg.osd_reg_base_pa[1];
	osd.osd_reg.osd_pln_reg_base_pa[1] =
		osd.osd_reg.osd_reg_base_pa[1] + 0x100;
	osd.osd_reg.osd_scl_reg_base_pa[1] =
		osd.osd_reg.osd_reg_base_pa[1] + 0x200;

#if !OSD_IOMMU_SUPPORT
	mmu_reg = (uintptr_t)of_iomap(np, 2); /*0x1400cf80 */
	mt_reg_sync_writew(0x0, mmu_reg);
	mt_reg_sync_writew(0x0, mmu_reg + 4);
	mmu_reg = (uintptr_t)of_iomap(np, 3); /*0x1400ef80 */
	mt_reg_sync_writew(0x0, mmu_reg);
	mt_reg_sync_writew(0x0, mmu_reg + 4);
#else
	mmu_reg = (uintptr_t)of_iomap(np, 2); /*0x1400cf80 */
	mt_reg_sync_writew(0x1, mmu_reg);
	mt_reg_sync_writew(0x1, mmu_reg + 4);
	mmu_reg = (uintptr_t)of_iomap(np, 3); /*0x1400ef80 */
	mt_reg_sync_writew(0x1, mmu_reg);
	mt_reg_sync_writew(0x1, mmu_reg + 4);
#endif

	osd_drv->irq[0].value = irq_of_parse_and_map(np, 0); /* osd3 irq*/
	if (osd_drv->irq[0].value == 0)
		OSDDBG("OSD get irq from dts fail 1\n");
	else {
		osd_drv->irq[0].irq = DISP_IRQ_OSD3_VSYNC;
		osd_drv->irq_num = 1;
	}
	osd_drv->irq[1].value = irq_of_parse_and_map(np, 1);
	if (osd_drv->irq[1].value == 0)
		OSDDBG("OSD get irq from dts fail 1\n");
	else {
		osd_drv->irq[1].irq = DISP_IRQ_OSD3_FRAME_START;
		osd_drv->irq_num = 2;
	}
	/* osd3 frame end irq*/
	osd_drv->irq[2].value = irq_of_parse_and_map(np, 2);
	if (osd_drv->irq[2].value == 0)
		OSDDBG("OSD get irq from dts fail 1\n");
	else {
		osd_drv->irq[2].irq = DISP_IRQ_OSD3_FRAME_END;
		osd_drv->irq_num = 3;
	}
#if OSD_SUPPORT_GCE
	/*register device by node, now the dev is mtkfb,
	 *gce not support subsysy, so the clt_base will null
	 */
	osd_clt_base = cmdq_register_device(dev);
	/*request thread by index(in dts)*/
	if (of_parse_phandle_with_args(np, "mboxes",
			"#mbox-cells", 0, &spec)) {
		OSD_LOG_E("can't parse mboxes 0 property");
		return -OSD_RET_ERR_INTERNAL;
	}
	OSDDBG("trigger_loop_thread_id=%d\n", spec.args[0]);
	osd_trigger_loop_cl = cmdq_mbox_create(dev, 0);
	if (of_parse_phandle_with_args(np, "mboxes",
			"#mbox-cells", 1, &spec)) {
		OSD_LOG_E("can't parse mboxes 1 property");
		return -OSD_RET_ERR_INTERNAL;
	}
	OSDDBG("update_thread_id=%d\n", spec.args[0]);
	osd_update_cl = cmdq_mbox_create(dev, 1);
	//of_property_read_u32_index(np, "gce-events", 1, &osd_frame_end_event);

	/*get osd frame end gce event id*/
	if (of_parse_phandle_with_args(np, "gce-events",
		"#gce-event-cells", 0, &spec)) {
		OSD_LOG_E("can't parse gce-events property");
		return -OSD_RET_ERR_INTERNAL;
	}
	//69:be_fifo_frame_end 51:osd_frame_end
	osd_frame_end_event_id = 69;
	OSDDBG("osd_frame_end_event_id=%d\n", osd_frame_end_event_id);
#endif

	OSDDBG("osd reg base:0x%p,0x%p\n",
		  (void *)osd.osd_reg.osd_fmt_reg_base[0],
		  (void *)osd.osd_reg.osd_fmt_reg_base[1]);
	return OSD_RET_OK;
}


long int got_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}

#if OSD_SUPPORT_GCE
static void _reclaim_cmdq_pkt_buf_work(struct work_struct *work)
{
	struct cmdq_update_pkt *update_pkt =
		container_of(work, struct cmdq_update_pkt, reclaim_work);
	int i = 0;
	int index = 0;
	i = update_pkt->plane;
	index = update_pkt->index;
	cmdq_pkt_destroy(update_pkt->pkt);
	update_pkt->state = false;
	OSDDBG("%s %d plane=%d index=%d vsync_cnt=%ld\n",
		__func__,
		__LINE__,
		i,
		index,
		vsync_cnt);
	/*release fence handler list node*/
	mutex_lock(&(osd.osd_config_queue_lock[i]));

	if (!list_empty(&(OSD_Configed_Head[i]))) {
		struct Osd_buffer_list *pBuffList = NULL;

		list_for_each_entry(pBuffList,
		&OSD_Configed_Head[i],
		list) {
			if (pBuffList->list_buf_state ==
				list_configed && index >=
					pBuffList
					->buffer_info
					.config
					.index ) {
				pBuffList
				->list_buf_state
				= list_updated;

				osd.update_tl_idx[i] =
					pBuffList
					->buffer_info
					.config
					.index;
				osd.osd_current_frame[i] = pBuffList;
				OSDDBG("%d:[%ld]cur idx:%d cv:%ld\n",
					i, vsync_cnt,
					osd
					.update_tl_idx
					[i],
					pBuffList
					->vsync_cnt
				);

				if (pBuffList
					->buffer_info
					.config
					.pre_index
					!= -1)
					osd_signal_pre_fence(
						i,
						pBuffList
						->buffer_info
						.config
						.pre_index);
				OSDDBG
				("free pre-fence idx[%ld]:%d",
				vsync_cnt,
				pBuffList
				->buffer_info
				.config
				.pre_index);
			} else
				OSD_PRINTF(OSD_FENCE_LOG,
					"have released pre_fence buffer_index=%d\n",
				pBuffList->buffer_info.config.index);
		}
	} else
		OSD_PRINTF(OSD_FENCE_LOG,
				"OSD_Configed_Head is empty\n");
	mutex_unlock(&(osd.osd_config_queue_lock[i]));
	osd_release_buffer(i);
}
static void cmdq_pkt_flush_cb_handler(struct cmdq_cb_data data)
{
	struct cmdq_update_pkt *update_pkt = NULL;

	update_pkt = (struct cmdq_update_pkt *)data.data;
	INIT_WORK(&update_pkt->reclaim_work,
				  _reclaim_cmdq_pkt_buf_work);
	queue_work(osd.reclaim_cmdq_pkt_workq,
				   &update_pkt->reclaim_work);
}
static int cmdq_trigger_loop_cl_event(void)
{
	INT32 ret = OSD_RET_OK;
	/*create osd trigger loop pkt for clear EOF event*/
	osd_trigger_loop_pkt.pkt =
		cmdq_pkt_create(osd_trigger_loop_cl);
	if (osd_trigger_loop_pkt.pkt == NULL)
		return OSD_RET_OUT_OF_MEM;
	ret = cmdq_pkt_wfe(
		osd_trigger_loop_pkt.pkt,
		osd_frame_end_event_id);
	if (ret != 0) {
		OSD_LOG_I("cmdq_pkt_wfe fail ret=%d\n", ret);
		return ret;
	}
	ret = cmdq_pkt_jump(osd_trigger_loop_pkt.pkt,
		-1*osd_trigger_loop_pkt.pkt->cmd_buf_size);
	if (ret != 0) {
		OSD_LOG_I("cmdq_pkt_jump fail ret=%d\n", ret);
		return ret;
	}
	ret = cmdq_pkt_flush_async(
		osd_trigger_loop_pkt.pkt,
		cmdq_pkt_flush_cb_handler,
		(void *)(&osd_trigger_loop_pkt));
	if (ret != 0) {
		OSD_LOG_I("cmdq_pkt_jump fail ret=%d\n", ret);
		return ret;
	}
	return ret;
}
#endif

static int disp_osd_irq_handle(uint32_t irq)
{
	if (irq == DISP_IRQ_FMT_VSYNC) {
		if (vsync_cnt == 0xFFFFFFFF)
			vsync_cnt = 0;
		else
			vsync_cnt++;
		OSD_PRINTF(OSD_VYSNC_LOG,
			"************* vsync [%ld] *************\n",
			vsync_cnt);
		if (osd.osd_in_suspend[OSD_PLANE_1] &&
		    osd.osd_in_suspend[OSD_PLANE_2])
			return OSD_RET_OK;

		if (!osd.osd_initial)
			return OSD_RET_OK;

		/*check trigger setting(atomic)*/
		/*set config vsync cnt according to trigger info*/
#if 0
		if (trigger[OSD_PLANE_1])
			osd_pla_non_shadow_update(OSD_PLANE_1);
		if (trigger[OSD_PLANE_2])
			osd_pla_non_shadow_update(OSD_PLANE_2);
#endif

		atomic_set(&osd_irq_event[OSD_PLANE_1], 1);
		wake_up_interruptible(&osd_irq_wq[OSD_PLANE_1]);

		atomic_set(&osd_irq_event[OSD_PLANE_2], 1);
		wake_up_interruptible(&osd_irq_wq[OSD_PLANE_2]);

		atomic_set(&irq_fmt_vsync_start_flag, 1);
		wake_up_interruptible(&vsync_start_flag);

		/*clear osd3 plane irq flag*/
		atomic_set(&osd.irq_res_flag, 0);
		atomic_set(&osd.config_cmd_count, 0);
		trigger[OSD_PLANE_1] = false;
		trigger[OSD_PLANE_2] = false;
	} else if (irq == DISP_IRQ_OSD3_VSYNC) {
		/*wake up config thread*/
		if (vsync_cnt1 != vsync_cnt) {
			atomic_set(&osd.irq_res_flag, 1);
#if 0
			wake_up_interruptible(&osd.irq_res);
#endif
		}
		vsync_cnt1 = vsync_cnt;
	}

	return OSD_RET_OK;
}

static void osd_remove_update_buffer_list(uint32_t layer)
{
	int i = layer;

	mutex_lock(&(osd.osd_config_queue_lock[i]));
	while (!list_empty(&(OSD_Configed_Head[i]))) {
		struct Osd_buffer_list *pBuffList = NULL;

		pBuffList = list_first_entry(&OSD_Configed_Head[i],
					     struct Osd_buffer_list, list);
		if (pBuffList) {
			if (pBuffList->fence_fd > -1) {
				if (pBuffList->fences != NULL)
					dma_fence_put(pBuffList->fences);
			}
			OSD_PRINTF(OSD_CONFIG_SW_LOG,
				"[%d]rel update fence fd:%d, idx:%d\n", i,
				pBuffList->fence_fd,
				pBuffList->buffer_info.config.index);
			release_cnt[i]++;
			osd_release_all_fence(
				i, pBuffList->buffer_info.config.index,
				pBuffList->buffer_info.config.pre_index);

			if (pBuffList->buffer_info.config.ion_handle != NULL)
				osd_ion_free_handle(osd_ion_client,
						    pBuffList->buffer_info
							    .config.ion_handle);

			list_del_init(&(pBuffList->list));
			vfree(pBuffList);
		}
	}
	mutex_unlock(&(osd.osd_config_queue_lock[i]));
}

static void osd_remove_config_buffer_list(uint32_t layer)
{
	int i = layer;

	mutex_lock(&(osd.osd_release_buf_lock[OSD_PLANE_2]));
	while (!list_empty(&(OSD_Buffer_Head[i]))) {
		struct Osd_buffer_list *pBuffList = NULL;

		pBuffList = list_first_entry(&OSD_Buffer_Head[i],
					     struct Osd_buffer_list, list);
		if (pBuffList) {
			if (pBuffList->list_buf_state != list_useless) {
				if (pBuffList->fence_fd > -1) {
					if (pBuffList->fences != NULL)
					dma_fence_put(pBuffList->fences);
				}

				OSD_PRINTF(OSD_CONFIG_SW_LOG,
					"[%d]rel config fence fd:%d, idx:%d\n",
					i, pBuffList->fence_fd,
					pBuffList->buffer_info
					.config
					.index);
				release_cnt[i]++;
				osd_release_all_fence(
					i, pBuffList->buffer_info.config.index,
					pBuffList->buffer_info.config
						.pre_index);

				if (pBuffList->buffer_info.config.ion_handle !=
				    NULL)
					osd_ion_free_handle(
						osd_ion_client,
						pBuffList->buffer_info.config
							.ion_handle);
			}
			list_del_init(&(pBuffList->list));
			vfree(pBuffList);
		}
	}

	mutex_unlock(&(osd.osd_release_buf_lock[OSD_PLANE_2]));
}

void osd_engine_clk_enable(enum HDMI_VIDEO_RESOLUTION res_mode, bool enable)
{
	if (enable) {
		/*disp_clock_set_pll(DISP_CLK_OSDPLL, 648000000);*/
		disp_clock_enable(DISP_CLK_OSDPLL, true);
		disp_clock_enable(DISP_CLK_OSD_FHD, true);
		disp_clock_enable(DISP_CLK_OSD_UHD, true);
		disp_clock_enable(DISP_CLK_OSD_SEL, true);
		disp_clock_select_pll(DISP_CLK_OSD_SEL, DISP_CLK_OSDPLL);
	} else {
		disp_clock_enable(DISP_CLK_OSDPLL, false);
		disp_clock_enable(DISP_CLK_OSD_SEL, false);
		disp_clock_enable(DISP_CLK_OSD_UHD, false);
		disp_clock_enable(DISP_CLK_OSD_FHD, false);
	}
}

void osd_clk_enable(enum HDMI_VIDEO_RESOLUTION res_mode, bool enable,
		    unsigned int layer_id)
{
	if (enable) {
		/* disp_clock_enable(DISP_CLK_SDR2HDR, true); */
		disp_clock_enable(DISP_CLK_OSDPLL, true);
		if (layer_id == OSD_PLANE_2) {
			disp_clock_smi_larb_en(DISP_SMI_LARB4, true);
			disp_clock_enable(DISP_CLK_OSD_FHD, true);
		} else {
			disp_clock_smi_larb_en(DISP_SMI_LARB0, true);
			disp_clock_enable(DISP_CLK_OSD_UHD, true);
		}
		disp_clock_enable(DISP_CLK_OSD_SEL, true);
#if 0
		if (res_mode <= HDMI_VIDEO_1920x1080p_50Hz)
			disp_clock_select_pll(DISP_CLK_OSD_SEL,
					DISP_CLK_OSDPLL_D2);
		else
			disp_clock_select_pll(DISP_CLK_OSD_SEL,
					DISP_CLK_OSDPLL);
#endif
		disp_clock_select_pll(DISP_CLK_OSD_SEL, DISP_CLK_OSDPLL);
	} else {
		disp_clock_enable(DISP_CLK_OSDPLL, false);
		disp_clock_enable(DISP_CLK_OSD_SEL, false);
		if (layer_id == OSD_PLANE_2) {
			disp_clock_smi_larb_en(DISP_SMI_LARB4, false);
			disp_clock_enable(DISP_CLK_OSD_FHD, false);
		} else {
			disp_clock_smi_larb_en(DISP_SMI_LARB0, false);
			disp_clock_enable(DISP_CLK_OSD_UHD, false);
		}
		/* disp_clock_enable(DISP_CLK_SDR2HDR, false); */
	}
}

static int osd_init(struct disp_hw_common_info *info)
{
	UINT32 u4Plane = 0;
	#if OSD_SUPPORT_GCE
	UINT32 u4Index = 0;
	#endif
	struct sched_param param = {.sched_priority = 2};
	struct sched_param irq_param = {.sched_priority = (MAX_RT_PRIO - 2)};
	INT32 ret = OSD_RET_OK;

	_osd_parse_dev_node();
	_OSD_BASE_GET_REG_BASE(osd.osd_reg.osd_fmt_reg_base,
			       osd.osd_reg.osd_fmt_reg_base_pa);
	_OSD_PLA_GET_REG_BASE(osd.osd_reg.osd_pln_reg_base,
					osd.osd_reg.osd_pln_reg_base_pa);
	_OSD_SC_GET_REG_BASE(osd.osd_reg.osd_scl_reg_base,
					osd.osd_reg.osd_scl_reg_base_pa);

	for (u4Plane = OSD_PLANE_1; u4Plane < OSD_PLANE_MAX_NUM; u4Plane++) {
		osd.res_chg[u4Plane].osd_res_mode = info->resolution->res_mode;
		osd.res_chg[u4Plane].cur_res_mode = info->resolution->res_mode;
		osd.res_chg[u4Plane].fg_need_change_res = false;
		osd.res_chg[u4Plane].height = info->resolution->height;
		osd.res_chg[u4Plane].width = info->resolution->width;
		osd.res_chg[u4Plane].htotal = info->resolution->htotal;
		osd.res_chg[u4Plane].vtotal = info->resolution->vtotal;
		osd.res_chg[u4Plane].is_hd = info->resolution->is_hd;
		osd.res_chg[u4Plane].is_progressive =
			info->resolution->is_progressive;
		osd.update_tl_idx[u4Plane] = -1;

		init_waitqueue_head(&osd.res_chg[u4Plane].event_res);
		atomic_set(&osd.res_chg[u4Plane].event_res_flag, 0);

		/*init res change event*/
		init_waitqueue_head(&osd.suspend_event_res[u4Plane]);
		atomic_set(&osd.suspend_event_res_flag[u4Plane], 0);

		mutex_init(&(osd.osd_queue_lock[u4Plane]));
		mutex_init(&(osd.osd_config_queue_lock[u4Plane]));
		mutex_init(&(osd.osd_release_buf_lock[u4Plane]));

		mutex_init(&osd.stop_sync_lock[u4Plane]);
		mutex_init(&osd.config_setting_lock[u4Plane]);

		osd.osd_stop_ctl.osd_stop_pts[u4Plane] = 0;
		osd.update_tl_idx[u4Plane] = 0;
		osd.plug_out[u4Plane].fg_hdmi_plug_out = false;
	}
	osd.aee_enable = false;
	init_waitqueue_head(&osd.irq_res);
	atomic_set(&osd.irq_res_flag, 0);
	/* clk manager*/
	/*osd_engine_clk_enable(info->resolution->res_mode, true);*/
	/*default use FHD-OSD*/
	osd_clk_enable(info->resolution->res_mode, true, OSD_PLANE_2);
	osd_clk_enable(info->resolution->res_mode, true, OSD_PLANE_1);

	osd.osd_stop_ctl.osd_stop_status[OSD_PLANE_1] = OSD_LAYER_START;
	osd.osd_stop_ctl.osd_stop_status[OSD_PLANE_2] = OSD_LAYER_START;

/*todo: base0, base1, fastlogo layer get reg from hw setting,
 *the other layer still need reset
 */
#if CONFIG_DRV_FAST_LOGO
	/*enter driver ,default not change resolution,
	 * res should be same as fastlogo res
	 * osd1:get osd info from hw setting
	 */
	for (u4Plane = OSD_PLANE_1; u4Plane < OSD_PLANE_MAX_NUM; u4Plane++) {
		_OSD_BASE_SetReg(u4Plane, NULL);
		_OSD_PLA_SetReg(u4Plane, NULL);
		OSD_PLA_Reset(u4Plane);
	}

	/*todo: fastlogo layer(UHD do not need res change.)*/
	/*todo: FHD layer need be set to the same res info,*/
	/*incase this layer been used without any res change */
	i4Osd_BaseSetFmt(OSD_PLANE_1, OSD_MAIN_PATH,
			 &(osd.res_chg[OSD_PLANE_1]), true);
#else
	for (u4Plane = OSD_PLANE_1; u4Plane < OSD_PLANE_MAX_NUM; u4Plane++) {
		_OSD_BASE_SetReg(u4Plane, NULL);
		_OSD_PLA_SetReg(u4Plane, NULL);
		i4Osd_BaseSetFmt(u4Plane, OSD_MAIN_PATH,
				 &(osd.res_chg[u4Plane]), true);
		OSD_PLA_Reset(u4Plane);
		OSDDBG("OSD_PLA_Reset %d\n", u4Plane);
	}
#endif
	_OSD_PLA_SetUpdateStatus(OSD_PLANE_1, 1);
	_OSD_PLA_SetUpdateStatus(OSD_PLANE_2, 1);
	osd_plane_set_extend_update(OSD_PLANE_1, 1);
	osd_plane_set_extend_update(OSD_PLANE_2, 1);
	fgupdate[OSD_PLANE_1] = true;
	fgupdate[OSD_PLANE_2] = true;

	osd_region_init(); /*todo: new region */
	OSDDBG("osd_region_init done\n");

	osd_ion_init();

	for (u4Plane = 0; u4Plane < MAX_OSD_INPUT_CONFIG; u4Plane++) {
		osd_sync_init(u4Plane);
		INIT_LIST_HEAD(&(OSD_Buffer_Head[u4Plane]));
		INIT_LIST_HEAD(&(OSD_Configed_Head[u4Plane]));
	}

	disp_osd_debug_init();
	disp_osd_dim_layer_buffer_init();

#if OSD_SUPPORT_GCE
	mutex_init(&osd.cmdq_client_handle_lock);

	osd.reclaim_cmdq_pkt_workq =
		alloc_workqueue("osd_reclaim_cmdq_pkt_wq",
				WQ_HIGHPRI | WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (IS_ERR_OR_NULL(osd.reclaim_cmdq_pkt_workq)) {
		OSD_LOG_E("alloc workqueue osd_reclaim_cmdq_pkt_wq fail\n");
		return -EFAULT;
	}

	for (u4Index = 0; u4Index < UPDATE_PKT_COUNT; u4Index++) {
		memset(&osd_update_pkt[OSD_PLANE_1][u4Index],
			0,
			sizeof(struct cmdq_update_pkt));
		memset(&osd_update_pkt[OSD_PLANE_2][u4Index],
			0,
			sizeof(struct cmdq_update_pkt));
	}

	cmdq_trigger_loop_cl_event();
#endif

	init_waitqueue_head(&osd_config_wq);
	if (!osd_config_task) {
		osd_config_task =
			kthread_create(disp_osd_config_kthread,
				       (void *)&(osd_layer[OSD_PLANE_2]),
				       "disp_osd2_config_kthread");
		wake_up_process(osd_config_task);
		OSDDBG("kthread_create disp_osd2_engine_kthread\n");
	}
	sched_setscheduler(osd_config_task, SCHED_RR, &param);

	init_waitqueue_head(&osd_irq_wq[OSD_PLANE_1]);
	if (!osd_irq_task[OSD_PLANE_1]) {
		osd_irq_task[OSD_PLANE_1] = kthread_create(
			disp_osd_irq_kthread, (void *)&(osd_layer[OSD_PLANE_1]),
			"disp_osd1_irq_kthread");
		wake_up_process(osd_irq_task[OSD_PLANE_1]);
		OSDDBG("kthread_create disp_osd1_irq_kthread\n");
	}
	sched_setscheduler(osd_irq_task[OSD_PLANE_1], SCHED_RR, &irq_param);

	init_waitqueue_head(&osd_irq_wq[OSD_PLANE_2]);
	if (!osd_irq_task[OSD_PLANE_2]) {
		osd_irq_task[OSD_PLANE_2] = kthread_create(
			disp_osd_irq_kthread, (void *)&(osd_layer[OSD_PLANE_2]),
			"disp_osd2_irq_kthread");
		wake_up_process(osd_irq_task[OSD_PLANE_2]);
		OSDDBG("kthread_create disp_osd2_irq_kthread\n");
	}
	sched_setscheduler(osd_irq_task[OSD_PLANE_2], SCHED_RR, &irq_param);

	osd_stop_config = vmalloc(sizeof(struct mtk_disp_buffer));
	memset(osd_stop_config, 0, sizeof(struct mtk_disp_buffer));

	osd.osd_in_suspend[OSD_PLANE_1] = false;
	osd.osd_in_suspend[OSD_PLANE_2] = false;
	fg_osd_alpha_detect_en = false;
	fg_enable_faker_hdr = false;
	osd.osd_initial = true;
	osd.osd_swap = false;

	init_waitqueue_head(&vsync_start_flag);

	osd_clk_enable(info->resolution->res_mode, false, OSD_PLANE_1);
	osd.osd_stop_ctl.osd_stop_status[OSD_PLANE_1] = OSD_LAYER_STOPPED;
	OSD_LOG_I("%s done\n", __func__);
	return ret;
}

static int osd_uninit(void)
{
	unsigned int u4Plane = 0;

	OSD_LOG_I("disp_osd_uninit\n");
	disp_osd_debug_deinit();

	_osd_region_uninit();
	osd_dim_layer_uninit();
	vfree(osd_stop_config);

	/*make sure all command be done!*/

	for (u4Plane = 0; u4Plane < MAX_OSD_INPUT_CONFIG; u4Plane++) {
		osd_sync_destroy(u4Plane);
		osd.res_chg[u4Plane].fg_need_change_res = false;
		osd_clk_enable(osd.res_chg[u4Plane].osd_res_mode, false,
			       u4Plane);
		fgupdate[u4Plane] = false;
	}
	return OSD_RET_OK;
}

static uint32_t osd_config_cnt[2];
static uint32_t osd_update_cnt[2];
unsigned long osd_start_vsync;
void disp_osd_vsync_stastic(unsigned int idx)
{
	if (idx == 1) {
		osd_config_cnt[0] = 0;
		osd_update_cnt[0] = 0;
		osd_config_cnt[1] = 0;
		osd_update_cnt[1] = 0;
		osd_start_vsync = vsync_cnt;
	}

	if (idx == 2) {
		OSD_LOG_I("osd[%ld-%ld]:%d,%d\n", osd_start_vsync, vsync_cnt,
			  osd_config_cnt[0], osd_update_cnt[0]);

		OSD_LOG_I("osd[%ld-%ld]:%d,%d\n", osd_start_vsync, vsync_cnt,
			  osd_config_cnt[1], osd_update_cnt[1]);
	}
}

void osd_release_buffer(unsigned int layer_id)
{
	/*new buffer comes, free last buffer*/
	/*stop free last buffer*/
	/*no new buffer, keep last buffer*/
	uint32_t i = layer_id;

	mutex_lock(&(osd.osd_config_queue_lock[i]));

	if (!list_empty(&(OSD_Configed_Head[i]))) {
		struct Osd_buffer_list *pBuffList = NULL;
		struct Osd_buffer_list *pBuffList_Temp = NULL;

		list_for_each_entry_safe(pBuffList, pBuffList_Temp,
					 &(OSD_Configed_Head[i]), list) {
			if (pBuffList->list_buf_state == list_updated) {
				if ((pBuffList->buffer_info.config.index <
				     (osd.update_tl_idx[i])) ||
				    (osd.update_tl_idx[i] == -1)) {
					if (pBuffList->buffer_info.config
						    .index != -1)
						osd_signal_fence(
							i,
							pBuffList->buffer_info
								.config.index);
					release_cnt[i]++;
					OSDDBG("free idx[%ld]:%d;cnt:%ld:%ld\n",
					       vsync_cnt,
					       pBuffList->buffer_info
								  .config.index,
					       config_cnt[i],
					       release_cnt[i]);

					if (pBuffList->buffer_info.config
						    .ion_handle != NULL)
						osd_ion_free_handle(
							osd_ion_client,
							pBuffList->buffer_info
							.config
							.ion_handle);

				if (pBuffList->fence_fd > -1) {
					if (pBuffList->fences != NULL)
					dma_fence_put(pBuffList->fences);

				}

					list_del_init(&(pBuffList->list));
					vfree(pBuffList);
					pBuffList = NULL;
				}
			}
		}
	}
	mutex_unlock(&(osd.osd_config_queue_lock[i]));
}

int disp_osd_irq_kthread(void *data)
{
	int wait_ret = 0;
	unsigned int i = 0;
	unsigned int layer_id = 0;
	uint32_t plane_id = *(uint32_t *)data;
	enum DISP_PATH_HW_ID path_id;
	struct disp_hw *osd_drv = NULL;
	struct DISP_PATH_LAYER_INFO mix_layer_info = {0};
#if 0
bool is_last_cmd;
is_need_force_free_cmd;
#endif

	while (1) {
		if (plane_id >= OSD_PLANE_MAX_WINDOW) {
			OSD_LOG_E("osd irq kthread lay_id %d error\n",
				plane_id);
			continue;
		}
		wait_ret = wait_event_interruptible(
			osd_irq_wq[plane_id],
			atomic_read(&osd_irq_event[plane_id]));
		if (wait_ret)
			OSD_LOG_E("wait osd_irq_event error");
		atomic_set(&osd_irq_event[plane_id], 0);

		if (osd.osd_in_suspend[plane_id]) {
			/*not handle cmd  before
			 *resume
			 */
			continue;
		}

		i = plane_id;

		if (osd.res_chg[plane_id].fg_need_change_res) {
			if (osd.res_chg[plane_id].fg_res_vsync3) {
				_Osd_ReleaseReset_Plane(plane_id);
				osd.res_chg[plane_id].cur_res_mode =
					osd.res_chg[plane_id].osd_res_mode;
				/*inform pmx/hdmi: osd resolution change done!
				 */
				osd_remove_config_buffer_list(plane_id);
				osd_remove_update_buffer_list(plane_id);
				osd.res_chg[plane_id].fg_need_change_res =
					false;
				atomic_set(
					&osd.res_chg[plane_id].event_res_flag,
					1);
				wake_up(&osd.res_chg[plane_id].event_res);
				osd.res_chg[plane_id].fg_res_vsync3 = false;
				osd.osd_current_frame[i] = NULL;
			}

			if (osd.res_chg[plane_id].fg_res_vsync2) {
				osd_clk_enable(
					osd.res_chg[plane_id].osd_res_mode,
					true, plane_id);
				osd_clk_enable(
					osd.res_chg[plane_id].cur_res_mode,
					false, plane_id);
				/*todo: osd base change*/
				i4Osd_BaseSetFmt(plane_id, OSD_MAIN_PATH,
						 &(osd.res_chg[plane_id]),
						 true);
				osd.res_chg[plane_id].fg_res_vsync3 = true;
				osd.res_chg[plane_id].fg_res_vsync2 = false;
				fgupdate[plane_id] = true;
			}

			if (osd.res_chg[plane_id].fg_res_vsync1) {
				OSD_PLA_DisableAllPlane(plane_id);
				osd.res_chg[plane_id].fg_res_vsync2 = true;
				osd.res_chg[plane_id].fg_res_vsync1 = false;
				fgupdate[plane_id] = true;
			}

			/*goto update;*/
		}

#if !OSD_SUPPORT_GCE
		find_current[i] = false;
		/*get next and current buffer*/
		mutex_lock(&(osd.osd_config_queue_lock[i]));

		if (!list_empty(&(OSD_Configed_Head[i]))) {
			struct Osd_buffer_list *pBuffList = NULL;

			list_for_each_entry(pBuffList,
			&OSD_Configed_Head[i],
			list) {

				if (pBuffList->list_buf_state ==
				    list_configed) {

					if (pBuffList->vsync_cnt
					< vsync_cnt) {
						pBuffList
						->list_buf_state
						= list_updated;
						/*if
						 * (pBuffList
						 ->buffer_info
						 .config
						 .index
						 * != -1)
						 */
						osd.update_tl_idx[i] =
							pBuffList
							->buffer_info
							.config
							.index;
						find_current[i] = true;
						osd.osd_current_frame
							[i] = pBuffList;
						OSDDBG
						("%d:[%ld]cur idx:%d cv:%ld\n",
							i, vsync_cnt,
							osd
							.update_tl_idx
							[i],
							pBuffList
							->vsync_cnt
						);

					if (pBuffList
						->buffer_info
						.config
						.pre_index
						!= -1)
						osd_signal_pre_fence(
							i,
							pBuffList
							->buffer_info
							.config
							.pre_index);
					OSDDBG
					("free pre-fence idx[%ld]:%d",
					vsync_cnt,
					pBuffList
					->buffer_info
					.config
					.pre_index);
				} else {
					OSDDBG
					("%d:[%ld] idx:%d cfg vsync:%ld\n",
					i, vsync_cnt,
					pBuffList
					->buffer_info
					.config
					.index,
					pBuffList
					->vsync_cnt);
				}
				}
			}
		}
		mutex_unlock(&(osd.osd_config_queue_lock[i]));

		if (find_current[i])
			osd_release_buffer(i);
#endif
		if (true) {
			mutex_lock(&osd.stop_sync_lock[i]);
			if (osd.osd_stop_ctl.osd_stop_status[i] ==
			    OSD_LAYER_STOPPING) {
					osd_clk_enable(
						osd.res_chg[i]
							.cur_res_mode,
						false, i);
					osd.osd_stop_ctl
						.osd_stop_status[i] =
						OSD_LAYER_STOPPED;
					OSD_LOG_D(
						"[%ld]osd clk disable\n",
						vsync_cnt);
				//}
				osd.osd_stop_ctl.osd_stop_pts[i] = 0;
				osd_drv = disp_osd_get_drv();
				layer_id = _osd_plane_map(i);
				osd_drv->drv_call(DISP_CMD_OSD_STOP,
					&layer_id);
				disp_fefifo_stop(layer_id + 2);
				if (i == 1)
					path_id = DISP_PATH_FHD_OSD;
				else
					path_id = DISP_PATH_UHD_OSD;
				disp_path_set_hw_path(path_id, false);
				osd.osd_stop_ctl.osd_stop_pts[i] = 0;
			}
			if (osd.osd_stop_ctl.osd_stop_status[i] ==
				OSD_LAYER_DISABLE &&
				osd.osd_stop_ctl.osd_stop_pts[i]++ > 30)
				osd.osd_stop_ctl.osd_stop_status[i] =
					OSD_LAYER_STOPPING;
			if (osd.osd_stop_ctl.osd_stop_status[i] ==
			    OSD_LAYER_STOP) {
				osd.osd_stop_ctl.osd_stop_status[i] =
					OSD_LAYER_DISABLE;
				osd.osd_stop_ctl.osd_stop_pts[i] = 1;

			}

			mutex_unlock(&osd.stop_sync_lock[i]);
		}

		if (find_current[i] == false) {
			if (osd.osd_stop_ctl.osd_stop_status[i]
				== OSD_LAYER_START)
				osd_reassemble_multi_region(i);
		}


		if (osd.osd_try_in_suspend[i] == 2) {
			atomic_set(&osd.suspend_event_res_flag[i], 1);
			wake_up(&osd.suspend_event_res[i]);
			osd.osd_try_in_suspend[i]++;
		}

		if (osd.osd_try_in_suspend[i] == 1) {
			osd_plane_enable(i, false);
			if (i == OSD_PLANE_2)
				mix_layer_info.type = DISP_OSD_FHD;
			else
				mix_layer_info.type = DISP_OSD_UHD;
			mix_layer_info.enable = true;
			disp_mix_hal_layer_control(&mix_layer_info);
			fgupdate[i] = true;
			OSD_LOG_D
			("osd_try_in_suspend close osd plane %d\n", i);
			osd.osd_try_in_suspend[i]++;
			osd.osd_current_frame[i] = NULL;
		}

		if (osd.plug_out[i].fg_hdmi_plug_out) {
			osd_remove_config_buffer_list(i);
			osd_remove_update_buffer_list(i);
			osd.osd_current_frame[i] = NULL;
			if (OSD_Is_PLA_Enabled(i)) {
				osd_plane_enable(i, false);
				OSD_LOG_D("hdmi plug out release buf %d", i);
				fgupdate[i] = true;
			} else {
				fgupdate[i] = false;
			}
		}

		mutex_lock(&osd.config_setting_lock[plane_id]);
		disp_osd_update_register(plane_id, -1);
		mutex_unlock(&osd.config_setting_lock[plane_id]);

		if (kthread_should_stop())
			break;
	}
	return 0;
}

int disp_osd_config_kthread(void *data)
{
	int wait_ret = 0;
#if 0
	int wait_plane_ret = 0;
#endif
	int i = 0;
	struct disp_hw *osd_drv = disp_osd_get_drv();
	int hdr10p_delay_flag = 0;

	while (1) {
		wait_ret = wait_event_interruptible(
			osd_config_wq, atomic_read(&osd_config_event));
		if (wait_ret)
			OSD_LOG_E("wait osd_config_event error");
		atomic_set(&osd_config_event, 0);

		/*wait 2 layer fence*/
		while ((((!osd.osd_swap && !list_empty(&(OSD_Buffer_Head[OSD_PLANE_2]))) ||
				(osd.osd_swap && !list_empty(&(OSD_Buffer_Head[OSD_PLANE_1])))) &&
				!(osd.res_chg[OSD_PLANE_2].fg_need_change_res)) ||
	       		osd.aee_enable) {

			mutex_lock(&(osd.osd_release_buf_lock[OSD_PLANE_2]));

		for (i = OSD_PLANE_1; i <= OSD_PLANE_2; i++) {

			if (!list_empty(&(OSD_Buffer_Head[i]))) {
				struct Osd_buffer_list *pBuffList
					= NULL;

				pBuffList = list_first_entry(
					&(OSD_Buffer_Head[i]),
					struct Osd_buffer_list, list);

			if (pBuffList) {

				if (pBuffList->list_buf_state ==
					   list_new) {
					OSDDBG(
						"vsync[%ld] rel_fence_idx=%d\n",
						vsync_cnt,
						pBuffList
						->buffer_info
						.config
						.index);

					if (osd_config_sw_register(&(
						pBuffList
						->buffer_info
						.config)) <
						0) {
						OSD_LOG_E(
						"osd_config_sw_register Failed\n");
						osd_reset_plane_region_state(
						pBuffList
						->buffer_info
						.config
						.layer_id);

					if (pBuffList
						->buffer_info
						.config
						.sbs !=
						0)
						osd_reset_plane_region_state(
						pBuffList
						->buffer_info
						.config
						.layer_id
						+ 1);
					}

			if ((pBuffList
				->fence_fd) >
				-1) {

				if (pBuffList
					->fences !=
					NULL) {
					/*osd wait acquire fence */
					if ( dma_fence_wait_timeout(
							pBuffList->fences,
							false,
							msecs_to_jiffies(50)) < 0) {
						OSD_LOG_I
						("%d wait fence nxt vsync\n",
						i);
					}
				} else
					OSDERR
					("get fence fail\n");
				}
					pview[i] = pBuffList;
					}
				} /*pBufferList*/
			} else {
				pview[i] = NULL;
			}
		}

		if (hdr10p_delay_done_flag == 0) {
			osd_drv->drv_call(DISP_CMD_GET_HDR10PLUS_INFO,
					  &hdr10p_delay_flag);
			if ((hdr10p_delay_flag == 1) &&
			    (hdr10p_delay_done_flag == 0)) {
				OSDDBG("start to delay a vsync\n");
				osd_drv->drv_call(
				DISP_CMD_SET_HDR10PLUS_GRAPHIC_OVERLAY,
				&hdr10p_delay_flag);
				atomic_set(&irq_fmt_vsync_start_flag,
					   0);
				wait_ret = wait_event_interruptible(
					vsync_start_flag,
					atomic_read(
					&irq_fmt_vsync_start_flag));
				if (wait_ret)
					OSD_LOG_E("wait fmt vsync flag err");
				atomic_set(&irq_fmt_vsync_start_flag,
					   0);
				hdr10p_delay_flag = 0;
				hdr10p_delay_done_flag = 1;
				OSDDBG("delay a vsync done\n");
			}
		}
		#if !OSD_SUPPORT_GCE
		if ((atomic_read(&osd.config_cmd_count) > 0) &&
		    atomic_read(&osd.irq_res_flag) == 1) {
			pr_info("***************** sleep *************\n");
			usleep_range(400, 700);
		}
		#endif
		for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
			if (pview[i] != NULL) {
				if ((pview[i]->buffer_info.config.layer_enable)
					&& ((pview[i]->buffer_info
					.config.tgt_height
					!= pview[i]->buffer_info
					.osd_disp_buffer.tgt.height)
					|| (pview[i]->buffer_info
					.config.tgt_width
					!= pview[i]->buffer_info
					.osd_disp_buffer.tgt.width))) {

					pview[i]->buffer_info
					.osd_disp_buffer.tgt.height =
					pview[i]->buffer_info.config.tgt_height;

					pview[i]->buffer_info
					.osd_disp_buffer.tgt.width =
					pview[i]->buffer_info.config.tgt_width;

				}
				if (pview[i]->buffer_info.config.layer_enable)
					disp_hdr_config_osd_info(
					&(pview[i]
					->buffer_info
					.osd_disp_buffer));
				mutex_lock(&osd.config_setting_lock[i]);
				fg_config_update[i] = true;
				disp_osd_update_register(i,
					pview[i]->buffer_info.config.index);
				mutex_unlock(
					&osd.config_setting_lock[i]);
			}
		}

		for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
			if (pview[i] != NULL) {
				#if !OSD_SUPPORT_GCE
				disp_osd_trigger_hw(i);
				#endif
				pview[i]->list_buf_state =
					list_configed;
				pview[i]->vsync_cnt = vsync_cnt;
				config_cnt[i]++;
				trigger[i] = true;
			}
		}
		OSDDBG("vsync[%ld] trigger end\n", vsync_cnt);

		for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
			if (pview[i] != NULL) {
				mutex_lock(&(osd.osd_queue_lock[i]));
				list_del_init(&(pview[i]->list));
				mutex_unlock(&(osd.osd_queue_lock[i]));

				mutex_lock(&(
					osd.osd_config_queue_lock[i]));
				list_add_tail(&(pview[i]->list),
					      &(OSD_Configed_Head[i]));
				mutex_unlock(&(
					osd.osd_config_queue_lock[i]));
				pview[i] = NULL;
			}
		}
		OSDDBG("vsync[%ld] configed end\n", vsync_cnt);

		/*trigger and record this vsync's record info*/
		mutex_unlock(&(osd.osd_release_buf_lock[OSD_PLANE_2]));
		atomic_add(1, &osd.config_cmd_count);
	}

		if (kthread_should_stop())
			break;
	}

	return 0;
}

int osd_color_fmt_remap(enum DISP_HW_COLOR_FORMAT disp_clr_fmt,
			uint32_t *osd_fmt, uint32_t *fmt_order)
{
	if (disp_clr_fmt == DISP_HW_COLOR_FORMAT_ARGB8888) {
		*osd_fmt = OSD_CM_ARGB8888_DIRECT32;
		*fmt_order = 0;
	} else if (disp_clr_fmt == DISP_HW_COLOR_FORMAT_ABGR8888) {
		*osd_fmt = OSD_CM_ARGB8888_DIRECT32;
		*fmt_order = 1;
	} else if (disp_clr_fmt == DISP_HW_COLOR_FORMAT_RGBA8888) {
		*osd_fmt = OSD_CM_ARGB8888_DIRECT32;
		*fmt_order = 2;
	} else if (disp_clr_fmt == DISP_HW_COLOR_FORMAT_BGRA8888) {
		*osd_fmt = OSD_CM_ARGB8888_DIRECT32;
		*fmt_order = 3;
	} else if (disp_clr_fmt == DISP_HW_COLOR_FORMAT_RGB565) {
		*osd_fmt = OSD_CM_RGB565_DIRECT16;
		*fmt_order = 1;
	} else {
		return -1;
	}
	return 0;
}

int osd_config_sw_register(
	struct disp_osd_layer_config *osd_overlay_buffer_info)
{
	INT32 i4Ret = 0;
	UINT32 u4Region = 0;
	union OSD_RGN_UNION_T *prRgn;
	struct DISP_PATH_LAYER_INFO mix_layer_info = {0};
	struct DISP_PATH_LOCATION_INFO location_info;
	BOOL fgValidReg = osd_overlay_buffer_info->layer_enable;
	unsigned int pvBitmap = osd_overlay_buffer_info->src_phy_addr;
	UINT32 u4SrcBufWidth = osd_overlay_buffer_info->src_buffer_width;
	UINT32 u4SrcBufHeight = osd_overlay_buffer_info->src_buffer_height;
	UINT32 u4SrcHeight = osd_overlay_buffer_info->src_height;
	UINT32 u4SrcWidth = osd_overlay_buffer_info->src_width;
	UINT32 u4SrcDispX = osd_overlay_buffer_info->src_offset_x;
	UINT32 u4SrcDispY = osd_overlay_buffer_info->src_offset_y;
	UINT32 u4tgtDispX = osd_overlay_buffer_info->tgt_offset_x;
	UINT32 u4tgtDispY = osd_overlay_buffer_info->tgt_offset_y;
	UINT32 u4tgtWidth = osd_overlay_buffer_info->tgt_width;
	UINT32 u4tgtheight = osd_overlay_buffer_info->tgt_height;
	UINT32 u4src_pitch = osd_overlay_buffer_info->src_pitch;
	/*UINT32 u4RgnWidth =osd_overlay_buffer_info->tgt_width; */
	UINT32 u4Plane = osd_overlay_buffer_info->layer_id; /*osd2, osd3*/
	UINT32 u4Scaler = u4Plane;
	UINT32 u4AlphaEn = osd_overlay_buffer_info->alpha_enable;
	bool   is_pvric = osd_overlay_buffer_info->is_pvric;
	uint32_t alpha = osd_overlay_buffer_info->alpha;
	bool osd_swap = osd_overlay_buffer_info->osd_swap;

	//struct ion_handle *ion_handle_1 = osd_overlay_buffer_info->ion_handle;
	//unsigned long va;
	unsigned int u4src_fmt, fmt_order;
	enum DISP_PATH_HW_ID mix_layer_id;
	uint32_t osd_hstart = 0;
	uint32_t osd_vodd = 0;
	uint32_t osd_veven = 0;

	unsigned int u4FdbcHeaderStartAddr = 0;
	unsigned int u4FdbcHeaderPitch = 0;
	OSD_PRINTF(OSD_CONFIG_SW_LOG,
		"%s u4Plane=%d enable=%d fmt=%x alpha:%d alpha_en:%d\n",
		__func__, u4Plane, fgValidReg,
		osd_overlay_buffer_info->src_fmt,
		alpha,
		u4AlphaEn);

	if (fgValidReg) { /*plane enable*/
		if (osd_color_fmt_remap(osd_overlay_buffer_info->src_fmt,
					&u4src_fmt, &fmt_order) < 0)
			return -2;

		/*cal pitch*/
		if (u4src_pitch == 0) {
			if ((u4src_fmt == (UINT32)OSD_CM_AYCBCR8888_DIRECT32) ||
			    (u4src_fmt == (UINT32)OSD_CM_ARGB8888_DIRECT32))
				u4src_pitch = u4SrcBufWidth << 2;
			else
				u4src_pitch = u4SrcBufWidth << 1;
		} else {
			if ((u4src_fmt == (UINT32)OSD_CM_AYCBCR8888_DIRECT32) ||
			    (u4src_fmt == (UINT32)OSD_CM_ARGB8888_DIRECT32))
				u4src_pitch = u4src_pitch << 2;
			else
				u4src_pitch = u4src_pitch << 1;
		}

		i4Ret = osd_get_free_region(u4Plane, &u4Region);
		i4Ret = _osd_region_reset(u4Region);
		if (i4Ret < 0)
			return i4Ret;

		osd_overlay_buffer_info->region_id = u4Region;

		/*todo: rgn alpha detect control*/
		i4Ret = OSD_RGN_Create_EX(u4Region, u4SrcBufWidth,
					  u4SrcBufHeight,
					  u4SrcDispX, u4SrcDispY,
					  u4SrcWidth, u4SrcHeight, pvBitmap,
					  u4src_fmt, u4src_pitch, 0, 0,
					  u4SrcWidth, u4SrcHeight, u4Plane, 0,
					  u4AlphaEn, alpha, fmt_order,
					  is_pvric);
		if (fg_osd_alpha_detect_en) {
			if (u4SrcHeight > 127) {
				osd_alpha_detect_enable(u4Plane, true,
							u4SrcHeight);
				osd.rgn.osd_rgn_idx[u4Plane] = u4Region;
			} else {
				osd_alpha_detect_enable(u4Plane, false,
							u4SrcHeight);
				osd.rgn.osd_rgn_idx[u4Plane] = u4Region;
			}
		}
		osd.rgn.osd_rgn_idx[u4Plane] = u4Region;
		osd_set_region_state(u4Plane);
		if (i4Ret < 0)
			return i4Ret;

		if (is_pvric) {
			u4FdbcHeaderStartAddr =
			pvBitmap + ((((((u4SrcBufWidth * u4SrcBufHeight) >> 6)
			+ 255) >> 8) << 7) - 1);
			u4FdbcHeaderPitch = u4SrcBufWidth >> 5;
			_OSD_PLA_SetBurst8_Enable(u4Plane, false);
			osd_plane_set_pvric_update(u4Plane, true);
		} else
			_OSD_PLA_SetBurst8_Enable(u4Plane, true);
		osd_plane_set_burst8_update(u4Plane, true);
		osd_pla_set_fbdc_header_start_addr(
			u4Plane, u4FdbcHeaderStartAddr);
		osd_pla_set_fbdc_header_src_pitch(
			u4Plane, u4FdbcHeaderPitch);

		//elvis added for big timing for small photo
		if ((u4tgtWidth < osd.res_chg[u4Plane].width) ||
			(u4tgtheight < osd.res_chg[u4Plane].height)) {
			osd_base_set_big_timing_update(u4Plane, true);
			_OSD_BASE_SetBigTiming(u4Plane, true);
		} else {
			osd_base_set_big_timing_update(u4Plane, true);
			_OSD_BASE_SetBigTiming(u4Plane, false);
		}
		i4Ret = OSD_SC_Scale(u4Scaler, fgValidReg, u4SrcWidth,
				     u4SrcHeight, u4tgtDispX, u4tgtDispY,
				     u4tgtWidth, u4tgtheight);
		if (i4Ret < 0)
			return i4Ret;
		if (osd_swap) {
			if (u4Plane == OSD_PLANE_1) {
				mix_layer_id = DISP_PATH_UHD_OSD;
				location_info.layer_type = DISP_OSD_UHD;
				location_info.layer_location_id = DISP_OSD_FHD;
			} else {
				mix_layer_id = DISP_PATH_FHD_OSD;
				location_info.layer_type = DISP_OSD_FHD;
				location_info.layer_location_id = DISP_OSD_UHD;
			}
		} else {
			if (u4Plane == OSD_PLANE_1) {
				mix_layer_id = DISP_PATH_UHD_OSD;
				location_info.layer_type = DISP_OSD_UHD;
				location_info.layer_location_id = DISP_OSD_UHD;
			} else {
				mix_layer_id = DISP_PATH_FHD_OSD;
				location_info.layer_type = DISP_OSD_FHD;
				location_info.layer_location_id = DISP_OSD_FHD;
			}
		}
		/*get active info*/
		i4Ret = disp_path_get_active_zone(
			mix_layer_id, osd.res_chg[u4Plane].cur_res_mode,
			&osd_hstart, &osd_vodd, &osd_veven);
		if (i4Ret < 0)
			OSD_PRINTF(OSD_CONFIG_SW_LOG, "get active zone fail\n");
		/*set active info*/
		_OSD_BASE_SetScrnHStartOsd(u4Plane,
						    osd_hstart);

		//elvis premultiply src need confirm with gpu
		//premulti need config
		if (u4AlphaEn) {
			_OSD_PLA_SetPreMulti(u4Plane, u4AlphaEn);
			//_OSD_PLA_SetPreMultiMode(u4Plane, 0x3);
			_OSD_PLA_SetFading(u4Plane, alpha);
		} else {
			_OSD_PLA_SetPreMulti(u4Plane, u4AlphaEn);
			//_OSD_PLA_SetPreMultiMode(u4Plane, 0x0);
			_OSD_PLA_SetFading(u4Plane, 0xff);
		}

		/* Add those for when debug to use the faker header function*/
		if (fg_enable_faker_hdr) {
			IGNORE_RET(_OSD_PLA_SetFakeHdr(u4Plane, 1));
			prRgn = OSD_RGN_GetAdd(u4Region);
			osd_pla_set_faker_header_register(u4Plane, prRgn);
			osd_plane_set_faker_header_update(u4Plane, true);
		} else
			IGNORE_RET(_OSD_PLA_SetFakeHdr(u4Plane, 0));
		i4Ret = OsdFlipPlaneRightNow(u4Plane, fgValidReg, u4Region, 0);

		mix_layer_info.alpha_blending = true;
		mix_layer_info.enable = fgValidReg;
		if (u4Plane == OSD_PLANE_2) {
			mix_layer_info.type = DISP_OSD_FHD;
			if (not_mix_fhd)
				mix_layer_info.enable = false;
		} else {
			mix_layer_info.type = DISP_OSD_UHD;
			if (not_mix_uhd)
				mix_layer_info.enable = false;
		}
		mix_layer_info.alpha_value = 0xff;
		mix_layer_info.pre_multi = false;
		mix_layer_info.x_offset = u4tgtDispX;
		mix_layer_info.y_offset = u4tgtDispY;
		mix_layer_info.src_width = u4tgtWidth;
		mix_layer_info.src_height = u4tgtheight;
		mix_layer_info.large_timing_small_region = false;
		//elvis added for big timing for small photo
		if ((u4tgtWidth < osd.res_chg[u4Plane].width) ||
			(u4tgtheight < osd.res_chg[u4Plane].height)) {

			mix_layer_info.x_offset = 0;
			mix_layer_info.y_offset = 0;
			mix_layer_info.src_width = osd.res_chg[u4Plane].width;
			mix_layer_info.src_height = osd.res_chg[u4Plane].height;
			u4tgtWidth = osd.res_chg[u4Plane].width;
			u4tgtheight = osd.res_chg[u4Plane].height;
			osd_overlay_buffer_info->tgt_width = u4tgtWidth;
			osd_overlay_buffer_info->tgt_height = u4tgtheight;
		}
		disp_mix_hal_layer_control(&mix_layer_info);
		disp_mix_hal_for_layer_swap(&location_info);
		if (mix_layer_info.type == DISP_OSD_FHD)
			disp_fefifo_set_src_res(2,
				u4tgtWidth, u4tgtheight, 0);
		else if (mix_layer_info.type == DISP_OSD_UHD)
			disp_fefifo_set_src_res(3,
				u4tgtWidth, u4tgtheight, 0);
#if 0
		if (ion_handle_1 != NULL) {
			va = (unsigned long)ion_map_kernel(osd_ion_client,
							   ion_handle_1);
			OSD_PRINTF(OSD_CONFIG_SW_LOG,
				"plane=%d src_va=%p****\n",
				u4Plane, (char *)va);
			if (u4Plane == OSD_PLANE_1)
				DISP_MMP_DUMP(MMP_DISP_BITMAP_OSD1, va,
					      u4src_pitch * u4SrcHeight,
					      osd_overlay_buffer_info->src_fmt,
					      u4SrcWidth, u4SrcHeight);
			else
				DISP_MMP_DUMP(MMP_DISP_BITMAP_OSD2, va,
					      u4src_pitch * u4SrcHeight,
					      osd_overlay_buffer_info->src_fmt,
					      u4SrcWidth, u4SrcHeight);
		}
#endif
	} else { /*plane disable*/
		i4Ret = OsdFlipPlaneRightNow(u4Plane, fgValidReg, 0, 0);
		if (u4Plane == OSD_PLANE_2) {
			mix_layer_info.type = DISP_OSD_FHD;
			mix_layer_info.enable = true;
		} else {
			mix_layer_info.type = DISP_OSD_UHD;
			mix_layer_info.enable = fgValidReg;
		}
		disp_mix_hal_layer_control(&mix_layer_info);
	}
	return i4Ret;
}

int osd_reconfig_sw_register(
	uint32_t plane_id, uint32_t region_id,
	struct disp_osd_layer_config *osd_overlay_buffer_info)
{
	INT32 i4Ret = 0;
	UINT32 u4Region;
	BOOL fgValidReg = osd_overlay_buffer_info->layer_enable;
	unsigned int pvBitmap = osd_overlay_buffer_info->src_phy_addr;
	UINT32 u4SrcBufWidth = osd_overlay_buffer_info->src_buffer_width;
	UINT32 u4SrcBufHeight = osd_overlay_buffer_info->src_buffer_height;
	UINT32 u4SrcHeight = osd_overlay_buffer_info->src_height;
	UINT32 u4SrcWidth = osd_overlay_buffer_info->src_width;
	UINT32 u4SrcDispX = osd_overlay_buffer_info->src_offset_x;
	UINT32 u4SrcDispY = osd_overlay_buffer_info->src_offset_y;
	UINT32 u4src_pitch = osd_overlay_buffer_info->src_pitch;
	/*UINT32 u4RgnWidth =osd_overlay_buffer_info->tgt_width; */
	UINT32 u4Plane = osd_overlay_buffer_info->layer_id; /*osd2, osd3*/
	UINT32 u4AlphaEn = osd_overlay_buffer_info->alpha_enable;
	uint32_t alpha = osd_overlay_buffer_info->alpha;
	bool   is_pvric = osd_overlay_buffer_info->is_pvric;
	unsigned int u4src_fmt, fmt_order, region_height0, region_height1;
	unsigned int i;

	OSD_PRINTF(
		OSD_CONFIG_SW_LOG,
		"osd_config_sw_register u4Plane=%d ,layer_enable =%d, src_fmt=%x\n",
		u4Plane, fgValidReg, osd_overlay_buffer_info->src_fmt);
	if (osd_color_fmt_remap(osd_overlay_buffer_info->src_fmt, &u4src_fmt,
				&fmt_order) < 0)
		return -2;

	/*cal pitch*/
	if (u4src_pitch == 0) {
		if ((u4src_fmt == (UINT32)OSD_CM_AYCBCR8888_DIRECT32) ||
		    (u4src_fmt == (UINT32)OSD_CM_ARGB8888_DIRECT32))
			u4src_pitch = u4SrcWidth << 2;
		else
			u4src_pitch = u4SrcWidth << 1;
	} else {
		if ((u4src_fmt == (UINT32)OSD_CM_AYCBCR8888_DIRECT32) ||
		    (u4src_fmt == (UINT32)OSD_CM_ARGB8888_DIRECT32))
			u4src_pitch = u4src_pitch << 2;
		else
			u4src_pitch = u4src_pitch << 1;
	}

	u4Region = region_id;

	/*todo: rgn alpha detect control*/
	if (u4SrcHeight > 127) {
		for (i = 0; i < OSD_RGN_REG_NUM - 1; i++) {
			region_height0 =
				u4SrcHeight /
				OSD_RGN_REG_NUM; /*there may be a problem*/
			i4Ret = OSD_RGN_Create_EX(
				u4Region, u4SrcBufWidth,
				u4SrcBufHeight, u4SrcDispX,
				(u4SrcDispY + i * region_height0), u4SrcWidth,
				region_height0, pvBitmap, u4src_fmt,
				u4src_pitch, 0, (i * region_height0),
				u4SrcWidth, region_height0, u4Plane, (i + 1),
				u4AlphaEn, alpha, fmt_order, is_pvric);
		}
		region_height1 = (u4SrcHeight - i * region_height0);
		i4Ret = OSD_RGN_Create_EX(
			u4Region, u4SrcBufWidth, u4SrcBufHeight,
			u4SrcDispX, (u4SrcDispY + i * region_height0),
			u4SrcWidth, region_height1, pvBitmap, u4src_fmt,
			u4src_pitch, 0, (i * region_height0), u4SrcWidth,
			region_height1, u4Plane, (i + 1), u4AlphaEn, alpha,
			fmt_order, is_pvric);

		osd.rgn.osd_multi_region_rdy[u4Plane] = 1;
	} else {
		osd.rgn.osd_multi_region_rdy[u4Plane] = 0;
	}
	return i4Ret;
}

int osd_reassemble_multi_region(uint32_t plane)
{
	unsigned int region_alpha = 0xffffffff;
	unsigned int region_id;
	unsigned int header_id = 0;
	unsigned int sub_header_addr = 0;
	unsigned int i = plane;

	if (fg_osd_alpha_detect_en) {
		/*make sure alpha value is right for current frame*/
		if (osd.osd_current_frame[i] != NULL) {
			/*get  plane region alpha info*/
			osd_pla_get_alpha_region(plane, &region_alpha);
			/* plane multi-region setting*/
			if ((region_alpha & 0xff) == 0xff) {
				return 0;
			} else if ((region_alpha & 0xff) == 0x0) {
				OSDDBG("no region is visible\n");
				/*osd_plane_enable(plane, false);*/
			} else {
				region_id =
					osd.osd_current_frame[i]
						->buffer_info.config.region_id;

				osd_reconfig_sw_register(
					plane, region_id,
					&(osd.osd_current_frame[i]
						  ->buffer_info.config));

				if (osd.rgn.osd_multi_region_rdy[i] == 1) {
					OSDDBG("region id is:%x,%d\n", plane,
					       region_id);

			for (i = OSD_RGN_CLUSTER_NUM - 1;
				i > 0;
				i--) {

				if (region_alpha &
					(0x1 << (i - 1))) {
					header_id = i;

					if (sub_header_addr !=
						0) {
						_OSD_RGN_SetNextRegion(
							region_id,
							i,
							sub_header_addr);
							_OSD_RGN_SetNextEnable(
							region_id,
							i, 1);
						}
					OSDDBG("region is visible:0x%x\n",
							sub_header_addr);
						_OSD_RGN_GetAddress(
							region_id, i,
							&sub_header_addr);
						}
					}
					/*reflip new rgn to osd plane*/
					OsdFlipPlaneRightNow(plane, true,
							     region_id,
							     header_id);
					fg_alpha_det_update[plane] = true;
				}
			}
		}
	}
	return 0;
}

int disp_osd_update_register(uint32_t plane, int index)
{
	INT32 ret = OSD_RET_OK;
	UINT32 u4Index = 0;

	if (fg_config_update[plane]) {
		if (osd.osd_irq_thread_update[plane] == vsync_cnt)
			OSD_LOG_D(
				"[%ld]osd%d can't rewrite irq setting\n",
				vsync_cnt, plane);
		osd_update_cnt[plane]++;
	}

	if (fgupdate[plane])
		osd.osd_irq_thread_update[plane] = vsync_cnt;

	if (fg_config_update[plane])
		osd.osd_config_thread_update[plane] = vsync_cnt;

	/*if alpha detect update, could rewite by config thread*/
	if ((fgupdate[plane] || fg_config_update[plane] ||
		fg_debug_update) ||
		fgupdate_ex[plane]) {
		//if (osd.osd_stop_ctl.osd_stop_status[plane] !=
			//OSD_LAYER_STOPPED)
		{
			#if OSD_SUPPORT_GCE
			for (; u4Index < UPDATE_PKT_COUNT; u4Index++) {
				if (osd_update_pkt[plane][u4Index].state
					== false) {
					osd_update_pkt[plane][u4Index]
					.state = true;
					break;
				}
			}
			if (u4Index == UPDATE_PKT_COUNT) {
				OSD_LOG_E("get free update pkt fail\n");
				goto out;
			}
			mutex_lock(&osd.cmdq_client_handle_lock);

			if (osd_update_cl != NULL) {
				osd_update_pkt[plane][u4Index].pkt
					= cmdq_pkt_create(osd_update_cl);
				if (osd_update_pkt[plane][u4Index].pkt
					== NULL) {
					OSD_LOG_E("get free update pkt fail\n");
					goto err;
				}
				osd_update_pkt[plane][u4Index].plane = plane;
				osd_update_pkt[plane][u4Index].index = index;
				ret = cmdq_pkt_wait_no_clear(
					osd_update_pkt[plane][u4Index].pkt,
					osd_frame_end_event_id);
				if (ret != 0) {
					OSD_LOG_I("%s fail ret=%d\n",
						__func__,
						ret);
					goto err;
				}
				#else
				_OSD_UpdateReg(plane, 0, NULL);
				#endif
				_OSD_BASE_Update(plane,
					osd_update_pkt[plane][u4Index].pkt);
				OSD_PLA_Update(plane,
					osd_update_pkt[plane][u4Index].pkt);
				OSD_SC_Update(plane,
					osd_update_pkt[plane][u4Index].pkt);
				#if OSD_SUPPORT_GCE
				_OSD_UpdateReg(plane, 1,
					osd_update_pkt[plane][u4Index].pkt);
				//reset pvric
				cmdq_pkt_write_value_addr(
					osd_update_pkt[plane][u4Index].pkt,
					0x14000140,
					0xfffc7fff,
					~0);
				cmdq_pkt_write_value_addr(
					osd_update_pkt[plane][u4Index].pkt,
					0x14000140,
					0xffffffff,
					~0);
				ret = cmdq_pkt_flush_async(
					osd_update_pkt[plane][u4Index].pkt,
					cmdq_pkt_flush_cb_handler,
					(void *)
					(&osd_update_pkt[plane][u4Index]));
				if (ret < 0) {
					OSD_LOG_I("cmdq flush fail=%d\n",
						ret);
					goto err;
				}
			} else
				OSD_LOG_I("osd_update_cl is NULL\n");

			mutex_unlock(&osd.cmdq_client_handle_lock);
			#else
			if (!fg_config_update[plane])
				_OSD_UpdateReg(plane,
				1,
				NULL);
			#endif
		}
	}
	goto out;

#if OSD_SUPPORT_GCE
err:
	mutex_unlock(&osd.cmdq_client_handle_lock);
#endif

out:
	fgupdate[plane] = false;
	fgupdate_ex[plane] = false;
	fg_config_update[plane] = false;
	fg_debug_update = false;

	return ret;
}

void disp_osd_trigger_hw(uint32_t plane)
{
	_OSD_UpdateReg(plane, 1, NULL);
}


bool osd_drop_command_check(uint32_t plane_id,
			    struct disp_osd_input_config *buffer_info)
{
	if (osd.aee_enable)
		return false;
	if (buffer_info->config.layer_enable) {
		if (buffer_info->config.res_mode !=
		    osd.res_chg[plane_id].cur_res_mode)
			return true;
	}
	return false;
}

static int disp_mgr_post_osd_buffer(struct disp_osd_input_config *buffer_info)
{
	int ret = OSD_RET_OK;
	struct Osd_buffer_list *pBuffList = NULL;
	int value = MTK_OSD_NO_FENCE_FD;
	int value_present = MTK_OSD_NO_FENCE_FD;
	int fenceFd = MTK_OSD_NO_FENCE_FD;
	int pre_fencefd = MTK_OSD_NO_FENCE_FD;
	unsigned int i = buffer_info->config.layer_id;

	if (buffer_info->config.layer_id >= MAX_OSD_INPUT_CONFIG) {
		OSD_LOG_E("disp osd config lay id error\n");
		ret = -OSD_RET_INV_ARG;
		goto err;
	}

	if (fg_dovi_idk_test) {
		buffer_info->fence_fd = MTK_OSD_NO_FENCE_FD;
		buffer_info->config.release_fence_fd = MTK_OSD_NO_FENCE_FD;
		buffer_info->config.present_fence_fd = MTK_OSD_NO_FENCE_FD;
		vfree(pBuffList);
		return ret;
	}

	if (osd_drop_command_check(i, buffer_info)) {
		if (buffer_info->config.ion_handle != NULL)
			osd_ion_free_handle(osd_ion_client,
					    buffer_info->config.ion_handle);
		buffer_info->config.ion_handle = NULL;
		buffer_info->config.layer_enable = false;
		buffer_info->config.acquire_fence_fd = -1;
		OSD_LOG_I(
		"invalid layer res info buffer_res=%x cur_res=%x\n",
		buffer_info->config.res_mode,
		osd.res_chg[i].cur_res_mode);
	}

	pBuffList = vmalloc(sizeof(struct Osd_buffer_list));
	if (!pBuffList) {
		ret = -OSD_RET_INV_ARG;
		OSD_LOG_E("could not allocate buffer_list\n");
		goto err;
	}

	memcpy(&pBuffList->buffer_info, buffer_info,
	       sizeof(struct disp_osd_input_config));

	if (buffer_info->config.ion_handle != NULL) {
		ret = osd_create_fence(&pre_fencefd, &fenceFd, &value_present,
				       &value, i);
		if (ret < 0)
			goto err;
	}

	OSD_PRINTF(OSD_CONFIG_SW_LOG,
		"create fence at vsync=%ld fence_value=%d,%d\n",
		vsync_cnt, value, value_present);

	pBuffList->list_buf_state = list_new;
	pBuffList->buffer_info.config.release_fence_fd = fenceFd;
	pBuffList->buffer_info.config.present_fence_fd = pre_fencefd;
	pBuffList->buffer_info.config.index = value;
	pBuffList->buffer_info.config.pre_index = value_present;
	osd.last_cmd_fence_idx[i] = value;

	if (buffer_info->config.acquire_fence_fd > -1)
		pBuffList->fences = sync_file_get_fence(
							buffer_info->config
							.acquire_fence_fd);

	pBuffList->fence_fd = buffer_info->config.acquire_fence_fd;

	buffer_info->config.release_fence_fd = fenceFd;
	buffer_info->config.present_fence_fd = pre_fencefd;

	INIT_LIST_HEAD(&pBuffList->list);
	mutex_lock(&(osd.osd_queue_lock[i]));
	list_add_tail(&pBuffList->list, &(OSD_Buffer_Head[i]));
	mutex_unlock(&(osd.osd_queue_lock[i]));
	OSD_PRINTF(OSD_CONFIG_SW_LOG,
		"end post buffer vsync=%ld\n",
		vsync_cnt);

	if ((osd.osd_try_in_suspend[i] != 0) ||
	    (osd.res_chg[i].fg_need_change_res)) {
		OSD_LOG_I("osd enter suspend or resolution change :%d\n",
			  (osd.res_chg[i].fg_need_change_res));
		goto out;
	}
	goto out;

err:
	OSD_LOG_E("fence_fd=%d failed\n", buffer_info->fence_fd);
	buffer_info->fence_fd = MTK_OSD_NO_FENCE_FD;
	buffer_info->config.release_fence_fd = MTK_OSD_NO_FENCE_FD;
	buffer_info->config.present_fence_fd = MTK_OSD_NO_FENCE_FD;
	vfree(pBuffList);

out:

	return ret;
}

static int osd_set_displaymode(const struct disp_hw_resolution *info)
{
	int ret = -1;
	int i = 0;
	OSD_LOG_I("change resolution pre_res=0x%x current_res=0x%x\n",
		osd.res_chg[OSD_PLANE_2].osd_res_mode,
		info->res_mode);
	if (osd.res_chg[OSD_PLANE_2].osd_res_mode != info->res_mode) {
		for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
			osd.res_chg[i].fg_need_change_res = true;
			osd.res_chg[i].fg_res_vsync1 = true;
			osd.res_chg[i].fg_res_vsync2 = false;
			osd.res_chg[i].fg_res_vsync3 = false;
			osd.res_chg[i].osd_res_mode = info->res_mode;
			osd.res_chg[i].cur_res_mode = HDMI_VIDEO_RESOLUTION_NUM;
			osd.res_chg[i].frequency = info->frequency;
			osd.res_chg[i].height = info->height;
			osd.res_chg[i].width = info->width;
			osd.res_chg[i].htotal = info->htotal;
			osd.res_chg[i].vtotal = info->vtotal;
			osd.res_chg[i].is_hd = info->is_hd;
			osd.res_chg[i].is_progressive = info->is_progressive;
		}
		ret = (int)wait_event_timeout(
			osd.res_chg[OSD_PLANE_1].event_res,
			atomic_read(&osd.res_chg[OSD_PLANE_1].event_res_flag),
			msecs_to_jiffies(150));
		atomic_set(&osd.res_chg[OSD_PLANE_1].event_res_flag, 0);

		/*wait res change event*/
		if (ret == 0)
			OSD_LOG_E("wait osd1 reg change event timeout\n");

		ret = (int)wait_event_timeout(
			osd.res_chg[OSD_PLANE_2].event_res,
			atomic_read(&osd.res_chg[OSD_PLANE_2].event_res_flag),
			msecs_to_jiffies(150));
		atomic_set(&osd.res_chg[OSD_PLANE_2].event_res_flag, 0);

		/*wait res change event*/
		if (ret == 0)
			OSD_LOG_E("wait osd2 reg change event timeout\n");
	}

	return OSD_RET_OK;
}

/* test code */
int test_osd_buffer(struct disp_osd_input_config *buffer_info)
{
	int ret = OSD_RET_OK;

	return ret;
}

void osd_enable_faker_header(BOOL enable)
{
	fg_enable_faker_hdr = enable;
}

void disp_osd_dim_layer_buffer_init(void)
{
	struct device *dev;
	unsigned int size;
	unsigned int width, height;
	dma_addr_t mva_address = 0;
	unsigned long temp_address = 0;
	int i = 0, j = 0;

	dev = disp_hw_mgr_get_dev();
	width = 8;
	height = 4;
	size = width * height * 4;

	osd.dim_layer_virt_addr =
		dma_alloc_coherent(dev, size, &mva_address, GFP_KERNEL);
	osd.dim_layer_phy_addr = mva_address;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			temp_address = (unsigned long)(osd.dim_layer_virt_addr +
						       i * width * 4 + j * 4);
			*(uint8_t *)temp_address = 0x00;
			*(uint8_t *)(temp_address + 1) = 0x00;
			*(uint8_t *)(temp_address + 2) = 0x00;
			*(uint8_t *)(temp_address + 3) = 0xFF;
		}
	}

	OSDDBG("dim layer address 0x%p, 0x%x\n", osd.dim_layer_virt_addr,
		  osd.dim_layer_phy_addr);
}

void osd_dim_layer_uninit(void)
{
	unsigned int size = 0;
	struct device *dev;

	size = 8 * 4 * 4;
	dev = disp_hw_mgr_get_dev();
	if (osd.dim_layer_phy_addr)
		dma_free_coherent(dev, size, osd.dim_layer_virt_addr,
				  osd.dim_layer_phy_addr);
}

#ifdef min
#undef min
#define min(x, y) (x < y ? x : y)
#endif
#ifdef max
#undef max
#define max(x, y) (x > y ? x : y)
#endif

/*need release fd before return*/
static int disp_osd_config(struct mtk_disp_buffer *config,
			   struct disp_hw_common_info *info)
{
	struct disp_osd_input_config osd_plane_config = {0};
	struct ion_handle *osd_buf_ion_handle = NULL;
	unsigned int mva = 0;
	int ret = OSD_RET_OK;

	/*insert  buffer to queue */
	if (config->type == DISP_LAYER_OSD || config->type == DISP_LAYER_AEE) {
		if (osd.plug_out[OSD_PLANE_1].fg_hdmi_plug_out ||
		    osd.plug_out[OSD_PLANE_2].fg_hdmi_plug_out ||
		    osd.osd_in_suspend[OSD_PLANE_1] ||
		    osd.osd_in_suspend[OSD_PLANE_2]) {

			OSD_PRINTF(OSD_CONFIG_SW_LOG,
				"drop during hdmi plug out layer_id=%d,enable=%d\n",
				config->layer_id, config->layer_enable);

			config->release_fence_fd = MTK_OSD_NO_FENCE_FD;
			config->present_fence_fd = MTK_OSD_NO_FENCE_FD;
			goto error;
		}

		if (config->layer_id > MAX_OSD_INPUT_CONFIG) {
			OSD_LOG_E("disp osd config lay id error\n");
			ret = -OSD_RET_INV_ARG;
			goto error;
		}

		osd_plane_config.config.layer_id =
			_osd_plane_map(config->layer_id);
		osd_plane_config.config.layer_enable = config->layer_enable;
		if (config->layer_enable) {
			if (config->buffer_source != DISP_BUFFER_ALPHA) {
				osd_plane_config.config.src_fmt =
					config->src_fmt;
				osd_plane_config.config.src_pitch =
					config->src.pitch;
				osd_plane_config.config.src_buffer_width =
					config->src.width;
				osd_plane_config.config.src_buffer_height =
					config->src.height;
				osd_plane_config.config.src_offset_x =
					config->crop.x;
				osd_plane_config.config.src_offset_y =
					config->crop.y;
				osd_plane_config.config.src_width =
					config->crop.width;
				osd_plane_config.config.src_height =
					config->crop.height;
				osd_plane_config.config.src_phy_addr =
					(unsigned int)((unsigned long)config
							       ->src_phy_addr);

				if (config->ion_fd > 0) {
				//if (0) {
					osd_buf_ion_handle =
						osd_ion_import_handle(
							osd_ion_client,
							config->ion_fd);
					if (osd_buf_ion_handle == NULL) {
						OSD_LOG_I(
						"disp osd config get ion handle err\n");
						ret = -OSD_RET_INV_ARG;
						goto error;
					}
					osd_ion_phys_mmu_addr(
						osd_ion_client,
						osd_buf_ion_handle, &mva);
					if (mva == 0) {
						OSD_LOG_E(
						"disp osd config ion_phys err\n");
						ret = -OSD_RET_INV_ARG;
						goto error;
					}
				} else {
					if (osd_plane_config.config
						    .src_phy_addr < 0) {
						OSD_LOG_E(
							"disp osd config src_phy_addr err\n");
						ret = -OSD_RET_INV_ARG;
						goto error;
					}
					mva = osd_plane_config.config
						      .src_phy_addr;
				}

			} else {
				osd_plane_config.config.src_fmt =
					DISP_HW_COLOR_FORMAT_RGBA8888;
				osd_plane_config.config.src_pitch = 0;
				osd_plane_config.config.src_buffer_width = 0;
				osd_plane_config.config.src_buffer_height = 0;
				osd_plane_config.config.src_offset_x = 0;
				osd_plane_config.config.src_offset_y = 0;
				osd_plane_config.config.src_width = 8;
				osd_plane_config.config.src_height = 4;
				osd_plane_config.config.src_phy_addr =
					osd.dim_layer_phy_addr;
				mva = osd_plane_config.config.src_phy_addr;
			}

			if (info->osd_swap == 1) {
				osd_plane_config.config.osd_swap = true;
				osd.osd_swap = true;
			} else
				osd.osd_swap = false;

			osd_plane_config.config.ion_handle = osd_buf_ion_handle;
			osd_plane_config.config.src_phy_addr = mva;
			osd_plane_config.config.tgt_offset_x = config->tgt.x;
			osd_plane_config.config.tgt_offset_y = config->tgt.y;
			osd_plane_config.config.tgt_width = config->tgt.width;
			osd_plane_config.config.tgt_height = config->tgt.height;
			osd_plane_config.config.alpha_enable = config->alpha_en;
			osd_plane_config.config.alpha = config->alpha;
			osd_plane_config.config.res_mode = config->res_mode;
			osd_plane_config.config.is_pvric = config->is_pvric;

			osd_plane_config.config.acquire_fence_fd =
				config->acquire_fence_fd;

			/*
			 *the following info is for sdr2hdr,
			 *saving osd buffer info and tv info at vdp config,
			 *and used them at vdp routine.
			 */
			memcpy(&osd_plane_config.osd_disp_buffer, config,
			       sizeof(struct mtk_disp_buffer));
			memcpy(&osd_plane_config.common_info, info,
			       sizeof(struct disp_hw_common_info));

		} else {
			/*layer disable*/
			osd_plane_config.config.ion_handle = NULL;
			osd_plane_config.config.acquire_fence_fd = -1;
		}
		/* when set aee config buffer(no matter printf or clean) to
		 * osd,
		 */
		/* this para will be true, for config thread can handle this aee
		 * buffer
		 */
		if (config->type == DISP_LAYER_AEE &&
		    osd_plane_config.config.layer_id == OSD_PLANE_1)
			osd.aee_enable = true;
		else
			osd.aee_enable = false;

		OSD_PRINTF(OSD_CONFIG_SW_LOG,
			"src=%d type=%d layer_id=%d res=0x%x mva=0x%x\n",
			config->buffer_source,
			config->type,
			osd_plane_config.config.layer_id,
			config->res_mode,
			osd_plane_config.config.src_phy_addr);

		disp_mgr_post_osd_buffer(&osd_plane_config);

		config->release_fence_fd =
			osd_plane_config.config.release_fence_fd;
		config->present_fence_fd =
			osd_plane_config.config.present_fence_fd;

	} else if (config->type == DISP_LAYER_VDP && config->layer_order > 0) {
		/*record win info*/
		if (osd.win_cnt > (OSD_PLANE_MAX_WINDOW - 1)) {
			OSD_LOG_E("osd not support win cnt > 2\n");
			ret = -OSD_RET_INV_ARG;
			goto error;
		}

		osd.win[osd.win_cnt].x = config->tgt.x;
		osd.win[osd.win_cnt].y = config->tgt.y;
		osd.win[osd.win_cnt].width = config->tgt.width;
		osd.win[osd.win_cnt].height = config->tgt.height;
		osd.win[osd.win_cnt].active = false;
		osd.win_cnt++;
		OSD_PRINTF(OSD_FLOW_LOG, "vdp layer:%d\n", osd.win_cnt);
	}

error:
	return ret;
}

static int disp_osd_config_ex(struct mtk_disp_config *config,
			      struct disp_hw_common_info *info)
{
	int i = 0;
	int ret = 0;
	int ui_layer_cnt = 0;

	if (config->user == DISP_USER_AVSYNC) {
		OSD_PRINTF(OSD_FLOW_LOG, "osd config ex fake = %d %d\n",
			osd.osd_stop_ctl.osd_stop_status[OSD_PLANE_1],
			osd.osd_stop_ctl.osd_stop_status[OSD_PLANE_2]);
		if (osd.osd_stop_ctl.osd_stop_status[OSD_PLANE_2] ==
			OSD_LAYER_START)
			disp_hdr_config_osd_info_fake(0);
		if (osd.osd_stop_ctl.osd_stop_status[OSD_PLANE_1] ==
			OSD_LAYER_START)
			disp_hdr_config_osd_info_fake(1);
		return 0;
	}
	/*insert stop info when UI layer stopped*/
	for (i = 0; i < 4; i++) {
		if (config->buffer_info[i].layer_enable &&
		    (config->buffer_info[i].type == DISP_LAYER_OSD ||
		     config->buffer_info[i].type == DISP_LAYER_AEE))
			ui_layer_cnt++;
	}
	for (i = 3; i >= 0; i--) {
		if (ui_layer_cnt == 2) {
			if (config->buffer_info[i].layer_enable)
				disp_osd_config(&config->buffer_info[i], info);
		} else if (ui_layer_cnt == 1) {
			if (config->buffer_info[i].layer_enable) {
				if (config->buffer_info[i].type ==
				    DISP_LAYER_OSD) {
					if (config->buffer_info[i].layer_id ==
					    0) {
						/*insert layer 1 stop buffer*/
						osd_stop_config->type =
							DISP_LAYER_OSD;
						osd_stop_config->layer_enable =
							false;
						osd_stop_config->layer_id = 1;
						disp_osd_config(osd_stop_config,
								info);
						/*insert layer 0 buffer*/
						disp_osd_config(
							&config->buffer_info[i],
							info);
					} else if (config->buffer_info[i]
							   .layer_id == 1) {
						/*insert layer 1 buffer*/
						disp_osd_config(
							&config->buffer_info[i],
							info);

						/*insert layer 0 stop buffer*/
						osd_stop_config->type =
							DISP_LAYER_OSD;
						osd_stop_config->layer_enable =
							false;
						osd_stop_config->layer_id = 0;
						disp_osd_config(osd_stop_config,
								info);

					} else {
						OSD_LOG_E("err layer id:%d\n",
							  config->buffer_info[i]
								  .layer_id);
					}
				} else {
					disp_osd_config(&config->buffer_info[i],
							info);
				}
			}
		} else if (ui_layer_cnt == 0) {
			if (osd.osd_stop_ctl.osd_stop_status[0] ==
				    OSD_LAYER_STOPPED &&
			    osd.osd_stop_ctl.osd_stop_status[1] ==
				    OSD_LAYER_STOPPED) {
				goto OUT;
			} else {
				if (config->user == DISP_USER_AEE) {
					osd_stop_config->type = DISP_LAYER_AEE;
					osd_stop_config->layer_enable = false;
					osd_stop_config->layer_id = 1;
					disp_osd_config(osd_stop_config, info);
				} else {
					/*insert stop buffer for UI*/
					osd_stop_config->type = DISP_LAYER_OSD;
					osd_stop_config->layer_enable = false;
					osd_stop_config->layer_id = 1;
					disp_osd_config(osd_stop_config, info);

					osd_stop_config->type = DISP_LAYER_OSD;
					osd_stop_config->layer_enable = false;
					osd_stop_config->layer_id = 0;
					disp_osd_config(osd_stop_config, info);
				}
				break;
			}
		} else {
			OSD_LOG_I("ui_layer_cnt = %d error\n", ui_layer_cnt);
		}
	}

	atomic_set(&osd_config_event, 1);
	wake_up_interruptible(&osd_config_wq);

	osd.win_cnt = 0; /*clear win info*/

OUT:
	return ret;
}

int disp_osd_start(struct disp_hw_common_info *info, unsigned int layer_id)
{
	unsigned int osd_layer_id;
	enum DISP_PATH_HW_ID path_id;
	struct disp_hw *osd_drv = disp_osd_get_drv();

	osd_layer_id = _osd_plane_map(layer_id);

	if (osd_layer_id == 1)
		path_id = DISP_PATH_FHD_OSD;
	else
		path_id = DISP_PATH_UHD_OSD;
	disp_path_set_hw_path(path_id, true);

	OSD_PRINTF(OSD_FLOW_LOG, "[%ld]%s\n",
		vsync_cnt, __func__);

	mutex_lock(&osd.stop_sync_lock[osd_layer_id]);
	if (osd.osd_stop_ctl.osd_stop_status[osd_layer_id]
		!= OSD_LAYER_START) {
		if (osd.osd_stop_ctl.osd_stop_status[osd_layer_id]
			== OSD_LAYER_STOPPED) {
			osd_clk_enable(
				osd.res_chg[osd_layer_id].cur_res_mode,
				true,
				osd_layer_id);

			/*reset full screen info*/
			memset(
			(void *)osd.osd_reg.osd_fmt_reg_base[osd_layer_id],
			9,
			sizeof(uintptr_t));
			/*reset plane RgbMode*/
			OSD_PLA_Reset(osd_layer_id);
			osd_plane_set_extend_update(osd_layer_id, 1);
			i4Osd_BaseSetFmt(osd_layer_id, OSD_MAIN_PATH,
				&(osd.res_chg[osd_layer_id]),
				true);
			_OSD_BASE_SetUpdate(osd_layer_id, true);
			_OSD_AlwaysUpdateReg(osd_layer_id, false);
			#if !OSD_SUPPORT_GCE
			_OSD_UpdateReg(osd_layer_id, true, NULL);
			#endif

		}
		osd.osd_stop_ctl.osd_stop_status[osd_layer_id]
			= OSD_LAYER_START;
	}
	disp_fefifo_start(info, layer_id + 2);
	osd_drv->drv_call(DISP_CMD_OSD_START,
						&layer_id);

	hdr10p_delay_done_flag = 0;
	osd.osd_stop_ctl.osd_stop_pts[osd_layer_id] = 0;
	mutex_unlock(&osd.stop_sync_lock[osd_layer_id]);

	return OSD_RET_OK;
}

int disp_osd_stop(unsigned int layer_id)
{
	unsigned int osd_layer_id;
	int ret = OSD_RET_OK;

	OSD_PRINTF(OSD_FLOW_LOG, "%s\n",
		__func__);

	if (fg_dovi_idk_test)
		return ret;

	osd_layer_id = _osd_plane_map(layer_id);
	mutex_lock(&osd.stop_sync_lock[osd_layer_id]);
	if (osd.osd_stop_ctl.osd_stop_status[osd_layer_id] == OSD_LAYER_START) {
		OSD_PRINTF(OSD_FLOW_LOG, "%s\n", __func__);
		osd.osd_stop_ctl.osd_stop_status[osd_layer_id] = OSD_LAYER_STOP;
		hdr10p_delay_done_flag = 0;
		ret = OSD_RET_OK;
	} else {
		OSD_LOG_I("err stop status: %d\n",
			  osd.osd_stop_ctl.osd_stop_status[osd_layer_id]);
		ret = -OSD_RET_INV_ARG;
	}
	mutex_unlock(&osd.stop_sync_lock[osd_layer_id]);

	osd.osd_stop_ctl.osd_stop_rel_fence_idx[osd_layer_id] =
		osd.last_cmd_fence_idx[osd_layer_id];
	is_stop_plane_done = false;

	osd_update_cnt[osd_layer_id] = 0;
	return ret;
}

#define ALIGN_TO_256(x) ((x + 0x255) & ~(0x255))

static int disp_osd_suspend(void)
{
	int i = 0;
	int wait_ret = 0;
	int layer_id = 0;
	char *osd_suspend_reg_info = NULL;
	unsigned int osd_suspend_reg_info_size = 0;
	struct disp_hw *osd_drv = NULL;
	#ifdef CONFIG_HDMI_BLACK
	struct DISP_PATH_LAYER_INFO mix_layer_info = {0};
	#endif

	osd_suspend_reg_info_size = 0x700;

	osd_suspend_reg_info = vmalloc(osd_suspend_reg_info_size);
	if (osd_suspend_reg_info == NULL) {
		OSD_LOG_I("osd suspend malloc mem fail\n");
		return OSD_RET_OUT_OF_MEM;
	}
	osd.osd_suspend_save_info = osd_suspend_reg_info;

	for (i = 0; i < MAX_OSD_INPUT_CONFIG; i++) {
		memcpy(osd_suspend_reg_info,
		       (void *)osd.osd_reg.osd_fmt_reg_base[i],
		       sizeof(union OSD_BASE_UNION_T));
		osd_suspend_reg_info += 0x100;
	}
	for (i = 0; i < MAX_OSD_INPUT_CONFIG; i++)
		osd.osd_try_in_suspend[i] = 1;
	/*osd push reset*/
	for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
		wait_ret = wait_event_timeout(osd.suspend_event_res[i],
				atomic_read(&osd.suspend_event_res_flag[i]),
				msecs_to_jiffies(100));
		if (wait_ret == 0) {
			OSD_LOG_I("wait irq thread suspend flow timeout\n");
			continue;
		}
		atomic_set(&osd.suspend_event_res_flag[i], 0);
	}

	/*osd_engine_clk_enable(osd.res_chg.cur_res_mode, false);*/
	for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
		mutex_lock(&osd.stop_sync_lock[i]);
		if (osd.osd_stop_ctl.osd_stop_status[i] !=
			OSD_LAYER_STOPPED) {
			osd_clk_enable(osd.res_chg[i].cur_res_mode,
				false, i);
			osd_drv = disp_osd_get_drv();
			layer_id = _osd_plane_map(i);
			osd_drv->drv_call(DISP_CMD_OSD_STOP,
					&layer_id);
			disp_fefifo_stop(layer_id + 2);
			osd.osd_stop_ctl.osd_stop_status[i]
				= OSD_LAYER_STOPPED;
		}
		mutex_unlock(&osd.stop_sync_lock[i]);

		osd.osd_in_suspend[i] = true;
		osd.osd_try_in_suspend[i] = 0;
		#ifdef CONFIG_HDMI_BLACK
		if (i == OSD_PLANE_1) {
			mix_layer_info.type = DISP_OSD_UHD;
			mix_layer_info.enable = false;
			disp_mix_hal_layer_control(&mix_layer_info);
		}
		#endif
	}

	/*remove all buffer*/
	for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
		osd_remove_config_buffer_list(i);
		osd_remove_update_buffer_list(i);
	}

	#if OSD_SUPPORT_GCE
	mutex_lock(&osd.cmdq_client_handle_lock);
	cmdq_mbox_stop(osd_trigger_loop_cl);
	cmdq_mbox_stop(osd_update_cl);
	mutex_unlock(&osd.cmdq_client_handle_lock);
	#endif

	OSD_LOG_I("osd suspend 0x%p\n", (void *)osd.osd_suspend_save_info);
	return OSD_RET_OK;
}

static int disp_osd_resume(void)
{
	int i;
	char *osd_suspend_reg_info = NULL;
	struct DISP_PATH_LAYER_INFO mix_layer_info = {0};

	#if OSD_SUPPORT_GCE
	struct device *dev = disp_hw_mgr_get_dev();

	mutex_lock(&osd.cmdq_client_handle_lock);
	if (osd_trigger_loop_cl == NULL)
		osd_trigger_loop_cl = cmdq_mbox_create(dev, 0);
	if (osd_update_cl == NULL)
		osd_update_cl = cmdq_mbox_create(dev, 1);
	mutex_unlock(&osd.cmdq_client_handle_lock);
	cmdq_trigger_loop_cl_event();
	#endif

	OSD_LOG_I("osd resume 0x%p\n", (void *)osd.osd_suspend_save_info);
	osd_suspend_reg_info = osd.osd_suspend_save_info;

	/*restore osd hw  register*/
	for (i = 0; i < MAX_OSD_INPUT_CONFIG; i++) {
		memcpy((void *)(osd.osd_reg.osd_fmt_reg_base[i] + 8),
		       osd_suspend_reg_info + 8,
		       sizeof(union OSD_BASE_UNION_T) - 8);
		osd_suspend_reg_info += 0x100;
	}

	/*todo: resume osd setting recovery*/

	for (i = 0; i < MAX_OSD_INPUT_CONFIG; i++) {
		/*reset full screen info*/
		memset((void *)osd.osd_reg.osd_fmt_reg_base[i], 9,
		       sizeof(uintptr_t));
		/*reset plane RgbMode*/
		OSD_PLA_Reset(i);
		osd_plane_set_extend_update(i, 1);
		i4Osd_BaseSetFmt(i, OSD_MAIN_PATH, &(osd.res_chg[i]), true);
		_OSD_BASE_SetUpdate(i, true);
		_OSD_AlwaysUpdateReg(i, false);
		#if !OSD_SUPPORT_GCE
		_OSD_UpdateReg(i, true, NULL);
		#endif
	}

	for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
		mutex_lock(&osd.stop_sync_lock[i]);
		if (osd.osd_stop_ctl.osd_stop_status[i] ==
			OSD_LAYER_STOPPED) {
			osd_clk_enable(osd.res_chg[i].cur_res_mode,
				true, i);
			osd.osd_stop_ctl.osd_stop_status[i] =
				OSD_LAYER_START;
		}
		mutex_unlock(&osd.stop_sync_lock[i]);

		if (i == OSD_PLANE_1) {
			mix_layer_info.type = DISP_OSD_UHD;
			mix_layer_info.enable = false;
		} else {
			mix_layer_info.type = DISP_OSD_FHD;
			mix_layer_info.enable = true;
		}
		disp_mix_hal_layer_control(&mix_layer_info);

		osd.osd_in_suspend[i] = false;
	}

	vfree(osd.osd_suspend_save_info);
	osd.osd_suspend_save_info = NULL;

	return OSD_RET_OK;
}

static int disp_osd_get_info(struct disp_hw_common_info *info)
{
	return OSD_RET_OK;
}

int disp_osd_dump(uint32_t level)
{
	UINT32 u4OsdNum;
	UINT32 src_x, src_y, src_width, src_height;
	UINT32 dst_x, dst_y, dst_width, dst_height;
	UINT32 osd_color_fmt;
	UINT32 src_Asel;
	UINT32 src_Yrsel;
	UINT32 dst_color_mode;
	UINT32 u4PlaneChecksum = 0;
	UINT32 u4SclChecksum = 0;
	BOOL status;
	UINT32 u4Region;
	union OSD_BASE_UNION_T *prHwOsdFmtReg_dump;
	union OSD_PLA_CORE_UNION_T *prHwOsdPlaneReg_dump;
	union OSD_SC_UNION_T *prHwOsdSclReg_dump;
	UINT32 current_hdmi_resolution;
	UINT32 timeline_cnt, pre_timeline_cnt;
	UINT32 fence_cnt, pre_fence_cnt;
	UINT32 current_fence_index;
	char *src_color_format = NULL;
	char *OSD_LAYER[OSD_PLANE_MAX_NUM] = {"UHD", "FHD"};
	char *LAYER_STATUS[2] = {"Disable", "Enable"};
	/*base ARGB sel*/
	char *SRC_COLOR_FORMAT[4][4] = {{"", "ARGB8888", "", "ABGR8888"},
					{"", "", "", ""},
					{"", "", "", ""},
					{"RGBA8888", "", "BGRA8888", ""} };
	char *DST_COLOR_MODE[2] = {"YUV_MODE", "RGB_MODE"};
	char *HDMI_RESOLUTION_MODE[HDMI_VIDEO_RESOLUTION_NUM + 1] = {
		"720x480i_60Hz",      "720x576i_50Hz",
		"720x480p_60Hz",      "720x576p_50Hz",
		"1280x720p_60Hz",     "1280x720p_50Hz",
		"1920x1080i_60Hz",    "1920x1080i_50Hz",
		"1920x1080p_30Hz",    "1920x1080p_25Hz",
		"1920x1080p_24Hz",    "1920x1080p_23Hz",
		"1920x1080p_29Hz",    "1920x1080p_60Hz",
		"1920x1080p_50Hz",    "1280x720p3d_60Hz",
		"1280x720p3d_50Hz",   "1920x1080i3d_60Hz",
		"1920x1080i3d_50Hz",  "1920x1080p3d_24Hz",
		"1920x1080p3d_23Hz",  "3840x2160P_23_976HZ",
		"3840x2160P_24HZ",    "3840x2160P_25HZ",
		"3840x2160P_29_97HZ", "3840x2160P_30HZ",
		"4096x2160P_24HZ",    "3840x2160P_60HZ",
		"3840x2160P_50HZ",    "4096x2160P_60HZ",
		"4096x2160P_50HZ",    "1280x720p_59_94Hz",
		"1920x1080p_59_94Hz", "3840x2160P_59_94HZ",
		"4096x2160P_59_94HZ", "Reserved"};

	OSD_LOG_I("<<<<<<<<<<< OSD INFO BEGIN >>>>>>>>>>\n");
	for (u4OsdNum = 0; u4OsdNum < OSD_PLANE_MAX_NUM; u4OsdNum++) {
		OSD_LOG_I("**********OSD-%s layer **********\n",
			  OSD_LAYER[u4OsdNum]);
		prHwOsdFmtReg_dump =
			(union OSD_BASE_UNION_T *)
				osd.osd_reg.osd_fmt_reg_base[u4OsdNum];
		prHwOsdPlaneReg_dump =
			(union OSD_PLA_CORE_UNION_T *)
				osd.osd_reg.osd_pln_reg_base[u4OsdNum];
		prHwOsdSclReg_dump =
			(union OSD_SC_UNION_T *)
				osd.osd_reg.osd_scl_reg_base[u4OsdNum];
		status = prHwOsdPlaneReg_dump->rField.fgOsdEn;
		u4Region = osd.rgn.osd_rgn_idx[u4OsdNum];
		_OSD_RGN_GetHClip(u4Region, 0, &src_x);
		_OSD_RGN_GetVClip(u4Region, 0, &src_y);
		_OSD_RGN_GetASel(u4Region, 0, &src_Asel);
		_OSD_RGN_GetYrSel(u4Region, 0, &src_Yrsel);
		_OSD_RGN_GetColorMode(u4Region, 0, &osd_color_fmt);
		if (osd_color_fmt == OSD_CM_RGB565_DIRECT16)
			src_color_format = "RGB565";
		else if (osd_color_fmt == OSD_CM_ARGB8888_DIRECT32)
			src_color_format =
				SRC_COLOR_FORMAT[src_Asel][src_Yrsel];
		dst_color_mode = prHwOsdPlaneReg_dump->rField.fgRgbMode;
		src_width = prHwOsdSclReg_dump->rField.u4SrcHSize;
		src_height = prHwOsdSclReg_dump->rField.u4SrcVSize;
		dst_x = prHwOsdFmtReg_dump->rField.u4OsdHStart;
		dst_y = prHwOsdFmtReg_dump->rField.u4OsdVStart;
		dst_width = prHwOsdSclReg_dump->rField.u4DstHSize;
		dst_height = prHwOsdSclReg_dump->rField.u4DstVSize;
		u4PlaneChecksum =
				prHwOsdFmtReg_dump->rField.u4OsdCheckSum;
		u4SclChecksum =
				prHwOsdFmtReg_dump->rField.u4OsdScCheckSum;
		fence_cnt = osd_read_fence_counter(u4OsdNum);
		pre_fence_cnt = osd_read_pre_fence_counter(u4OsdNum);
		timeline_cnt = osd_read_timeline_counter(u4OsdNum);
		pre_timeline_cnt = osd_read_pre_timeline_counter(u4OsdNum);
		if (osd.plug_out[OSD_PLANE_1].fg_hdmi_plug_out) {
			current_fence_index = osd.update_tl_idx[u4OsdNum];
			current_hdmi_resolution =
				osd.res_chg[u4OsdNum].cur_res_mode;
			OSD_LOG_I(
				"** now hdmi status is plug_out, current HDMI resolution is %s\n",
				HDMI_RESOLUTION_MODE[current_hdmi_resolution]);
		} else {
			current_hdmi_resolution =
				osd.res_chg[u4OsdNum].osd_res_mode;
			OSD_LOG_I(
				"** HDMI resolution is %s\n",
				HDMI_RESOLUTION_MODE[current_hdmi_resolution]);
			if (status)
				current_fence_index =
					osd.osd_current_frame[u4OsdNum]
						->buffer_info.config.index;
		}
		OSD_LOG_I("** Layer status: %s\n", LAYER_STATUS[status]);
		if (status) {
			OSD_LOG_I(
				"** src_rec :(%4d,%4d,%4d,%4d) src_format:%s\n",
				src_x, src_y, src_width, src_height,
				src_color_format);
			OSD_LOG_I(
				"** dst_rec :(%4d,%4d,%4d,%4d) dst_color_mode:%s\n",
				dst_x, dst_y, dst_width, dst_height,
				DST_COLOR_MODE[dst_color_mode]);
			if (level >= 1) {
				OSD_LOG_I("** now osd playing fence index=%d\n",
					  current_fence_index);
				OSD_LOG_I(
					"** now still have %d buffer in configed queue not play\n",
					pre_fence_cnt - current_fence_index);
				OSD_LOG_I(
					"** fence_cnt=%d,pre_fence_cnt=%d,timeline_cnt=%d pre_timeline_cnt=%d\n",
					fence_cnt, pre_fence_cnt, timeline_cnt,
					pre_timeline_cnt);
				OSD_LOG_I("** config_cnt=%ld,release_cnt=%ld\n",
					  config_cnt[u4OsdNum],
					  release_cnt[u4OsdNum]);
			}
			if (level >= 2)
				OSD_LOG_I(
					"** PlaneChecksum:0x%x, SclChecksum:0x%x\n",
					u4PlaneChecksum, u4SclChecksum);
		}
		OSD_LOG_I("******************************\n");
	}
	OSD_LOG_I("<<<<<<<<<<< OSD INFO END >>>>>>>>>>\n");
	return OSD_RET_OK;
}

static int disp_osd_hdmi_plug_out(void)
{
	int i;

	for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
		if (osd.plug_out[i].fg_hdmi_plug_out == false) {
			osd.plug_out[i].fg_hdmi_plug_out = true;
			osd.plug_out[i].fg_vsync1 = true;
			osd.plug_out[i].fg_vsync2 = false;
		}
	}
	OSD_LOG_I("%s:%d\n", __func__,
		  osd.plug_out[OSD_PLANE_1].fg_hdmi_plug_out);

	return OSD_RET_OK;
}

static int disp_osd_hdmi_plug_in(void)
{
	int i;

	for (i = OSD_PLANE_1; i < OSD_PLANE_MAX_NUM; i++) {
		if (osd.plug_out[i].fg_hdmi_plug_out == true)
			osd.plug_out[i].fg_hdmi_plug_out = false;
	}

	OSD_LOG_I("%s:%d\n", __func__,
		  osd.plug_out[OSD_PLANE_1].fg_hdmi_plug_out);

	return OSD_RET_OK;
}

static int disp_osd_handle_event(enum DISP_CMD cmd, void *data)
{
	uint32_t rgb2bgr;

	switch (cmd) {
	case DISP_CMD_OSD_PREMIX_ENABLE_VDO4:
		OSD_LOG_I("DISP_CMD_OSD_PREMIX_ENABLE_VDO4\n");
		break;
	case DISP_CMD_OSD_UPDATE:
		fg_dovi_idk_test = *((bool *)data);
		OSD_LOG_I("fg_dovi_idk_test\n");
		break;
	case DISP_CMD_HDMITX_PLUG_OUT:
		disp_osd_hdmi_plug_out();
		break;
	case DISP_CMD_HDMITX_PLUG_IN:
		disp_osd_hdmi_plug_in();
		break;
	case DISP_CMD_OSD_RGB_TO_BGR:
		rgb2bgr = *((uint32_t *)data);
		OSD_LOG_I("RGB_TO_BGR: %d\n", rgb2bgr);
		break;
	case DISP_CMD_STOP_SDR2HDR_BT2020:
		hdr10p_delay_done_flag = 0;
		break;
	default:
		break;
	}
	return OSD_RET_OK;
}

/*****************osd driver****************/
struct disp_hw disp_osd_driver = {
	.name = OSD_DRV_NAME,
	.init = osd_init,
	.deinit = osd_uninit,
	.start = disp_osd_start,
	.stop = disp_osd_stop,
	.suspend = disp_osd_suspend,
	.resume = disp_osd_resume,
	.get_info = disp_osd_get_info,
	.change_resolution = osd_set_displaymode,
	.config = NULL,
	.config_ex = disp_osd_config_ex,
	.irq_handler = disp_osd_irq_handle,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = disp_osd_dump,
	.set_cmd = disp_osd_handle_event,
};

struct disp_hw *disp_osd_get_drv(void)
{
	return &disp_osd_driver;
}
