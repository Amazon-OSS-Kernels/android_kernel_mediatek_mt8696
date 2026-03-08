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

#define LOG_TAG "HDR_MAIN"

#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/atomic.h>
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
#include <uapi/linux/sched/types.h>

#include "disp_hw_mgr.h"
#include "disp_dovi_main.h"
#include "hdmitx.h"
#include "disp_irq.h"
#include "disp_path.h"
#include "disp_hdr_if.h"
#include "disp_hw_log.h"
#include "disp_vdp_vsync.h"
#include "fmt_hal.h"
#include "fmt_def.h"
#include "vdout_sys_hal.h"
#include "disp_dovi_common_if.h"
#include "disp_hdr_main.h"
#include "disp_cfd_main.h"
#include "disp_hdr_cmd.h"

#define MULBASE 10
#define N 50

static struct task_struct *disp_hdr_thread0, *disp_hdr_thread1;
static struct task_struct *disp_hdr_thread2, *disp_hdr_thread3;
static struct task_struct *disp_hdr_thread4;

static wait_queue_head_t disp_hdr_thread_wq0, disp_hdr_thread_wq1;
static wait_queue_head_t disp_hdr_thread_wq2, disp_hdr_thread_wq3;
static wait_queue_head_t disp_hdr_thread_wq4;

static atomic_t gWakeupHdrThread0, gWakeupHdrThread1;
static atomic_t gWakeupHdrThread2, gWakeupHdrThread3;
static atomic_t gWakeupHdrThread4;
//static uint32_t disp_hdr_wakeup_thread0, disp_hdr_wakeup_thread1;
//static uint32_t disp_hdr_wakeup_thread2, disp_hdr_wakeup_thread3;
//static uint32_t disp_hdr_wakeup_thread4;

static struct mutex disp_hdr_thread_mutex;

struct mutex disp_hdr_mutex;
uint32_t vdp_start_st[V_G_LAYER_MAX];
uint32_t disp_hdr_event;
uint64_t disp_hdr_thread_cnt;
uint32_t befifo_irq_cnt;
bool hdr_init_done;
#ifdef CONFIG_HDMI_BLACK
bool fg_hdr_deep_suspend;
#endif
uint32_t hdr_res_width;
uint32_t hdr_res_height;


#if 0 // for hdr10+ parse md move to openhdr
int set_hdmi_signal(struct video_buffer_info *buf)
{
	VID_PLA_HDR_METADATA_INFO_T rHdr = { 0 };
	/* hdr10 plus metadata need to copy every frame. */
	if ((buf->hdr10_type == HDR10_TYPE_PLUS) &&
	    (disp_common_info.tv.is_support_hdr10_plus) &&
	    (!buf->hdr_info.metadata_info.dovi_metadata.svp)) {
		rHdr.is_enter_async_mode = buf->is_enter_async_mode;
		if (disp_common_info.tv.hdr10_plus_app_ver != 0xFF) {
			vdp_fill_metadata_info(VID_PLA_DR_TYPE_HDR10_PLUS_VSIF,
				&rHdr,
				buf->hdr_info.metadata_info.dovi_metadata.buff,
				buf->hdr_info.metadata_info.dovi_metadata.len);
		} else {
			vdp_fill_metadata_info(VID_PLA_DR_TYPE_HDR10_PLUS,
		       &rHdr,
		       buf->hdr_info.metadata_info.dovi_metadata.buff,
		       buf->hdr_info.metadata_info.dovi_metadata.len);
		}
		hdr_printf("metadata pts %lld,%lld, %d, %d, %d\n",
		     buf->hdr_info.metadata_info.dovi_metadata.pts,
		     buf->hdr_info.metadata_info.dovi_metadata.pts - buf->pts,
		     buf->is_enter_async_mode,
		     osd_enable_sdr2hdr,
		     graphic_overlay_for_hdr10plus);
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
		vVdpSetHdrMetadata(true, rHdr);
#endif
	}
	/* need to set HDR10 signal */
	if (buf->hdr10_type != hdr_enable) {
		/* disable former tv signal */
		vdp_config_hdmi_signal(hdr_enable, NULL, false);

		/* enable current tv signal */
		vdp_config_hdmi_signal(buf->hdr10_type,
		&buf->hdr10_info, true);

		hdr_enable = buf->hdr10_type;
	}
	hdr_set_HDMI_BT2020_signal(buf->is_bt2020);

	return 0;
}


uint16_t vsif_min(uint16_t sei_value, uint16_t mocecule_de,
		  uint32_t mocecule_max, uint16_t denominater,
		  uint16_t max_value)
{
	uint32_t sei_value_new;
	uint32_t temp = 0;
	uint16_t vsif_value = 0;

	temp = (sei_value * MULBASE) / mocecule_de;
	temp =
		(temp > mocecule_max * MULBASE) ? mocecule_max * MULBASE : temp;
	temp = temp / denominater;
	if ((temp % 10) >= 5)
		temp = 1;
	else
		temp = 0;

	sei_value_new = sei_value / mocecule_de;
	sei_value_new =
		(sei_value_new > mocecule_max) ? mocecule_max : sei_value_new;
	vsif_value = (uint16_t) (sei_value_new / denominater + temp);
	vsif_value = (vsif_value > max_value) ? max_value : vsif_value;
	return vsif_value;
}

void user_data_registered_inu_t_t35(uint8_t *data,
	uint32_t size, uint8_t *vsif)
{
	uint8_t country_code = 0;

	uint8_t application_identifier = 0;
	uint8_t application_version = 0;
	uint8_t num_windows = 0;

	uint8_t num_idx = 0;
	uint8_t idx = 0;
	uint32_t sel_tsdmaxl = 0;
	uint8_t vsif_tsdmaxl = 0;
	uint32_t sei_avgmaxrgb = 0;
	uint8_t vsif_avgmaxrgb = 0;
	uint8_t num_sei_dmaxrgb = 0;
	uint32_t sei_dmaxrgb[16];	/* distributtion */
	uint8_t sei_dmaxrgb_idx[16];	/* distributtion */
	uint8_t vsif_dmaxrgb[16];
	uint8_t tone_mapping_flag = 0;
	uint32_t sei_kpx = 0;
	uint8_t vsif_kpx = 0;
	uint32_t sei_kpy = 0;
	uint8_t vsif_kpy = 0;
	uint8_t num_sei_bz_a = 0;
	uint32_t sei_bz_a[9];
	uint8_t vsif_bz_a[9];
	int remained_bits;
	uint32_t value;
	uint8_t *metadata = NULL;

	if (data == NULL || vsif == NULL)
		return;

	metadata = data;
	country_code = *metadata;
	/* skip country code, privider_code,
	 *provider_oriented_code(u(8)+u(16)+u(16))
	 */
	metadata += 5;

	application_identifier = *metadata;
	application_version = *(metadata + 1);

	metadata += 2;
	num_windows = (((*metadata) & 0xC0) >> 6);

	/* 27 bit */
	sel_tsdmaxl = (((*(metadata)) << 24) |
		       ((*(metadata + 1)) << 16) |
		       ((*(metadata + 2)) << 8) |
		       (*(metadata + 3)));
	sel_tsdmaxl = (sel_tsdmaxl & 0x3FFFFFFF) >> 3;
	metadata += 3;		/* 2bit */

	remained_bits = 2;
	for (num_idx = 0; num_idx < num_windows; ++num_idx) {
		/*remained_bits +17bit(maxscl)*3 = min:51bit.max:58 */

		/* skip maxsel */

		if ((remained_bits == 0) || (remained_bits > 3)) {
			metadata += 6;
			remained_bits = 8 + 6 * 8 - (51 - remained_bits);
		} else {
			metadata += 7;
			remained_bits = 8 + 6 * 8 - (51 - remained_bits);
		}

		remained_bits = 8 - (17 - (remained_bits + 8));

		sei_avgmaxrgb = ((((*(metadata)) << 16) |
				  ((*(metadata + 1)) << 8) |
				  (*(metadata + 2))) >> remained_bits) & 0x1FF;

		metadata += 2;
		if (remained_bits >= 4) {
			remained_bits -= 4;
			num_sei_dmaxrgb = ((*metadata) >> remained_bits) & 0xF;
		} else {
			remained_bits = 8 - remained_bits;
			num_sei_dmaxrgb =
			    ((((*metadata) << 8) |
			    (*(metadata + 1))) >> remained_bits) & 0xF;
			metadata++;
		}
		if (remained_bits == 0)
			metadata++;

		memset(sei_dmaxrgb, 0, sizeof(sei_dmaxrgb));
		for (idx = 0; idx < num_sei_dmaxrgb; ++idx) {
			if (remained_bits)
				sei_dmaxrgb[idx] = ((((*metadata) << 24)
				| ((*(metadata + 1)) << 16)
				| ((*(metadata + 2)) << 8)
				| (*(metadata + 3))) >> remained_bits)
				& 0xFFFFFF;
			else
				sei_dmaxrgb[idx] = ((((*(metadata)) << 16)
				| ((*(metadata + 1)) << 8)
				| (*(metadata + 2)))) >> remained_bits;

			sei_dmaxrgb_idx[idx] =
				(sei_dmaxrgb[idx] & 0xFE0000) >> 17;
			sei_dmaxrgb[idx] = sei_dmaxrgb[idx] & 0x1FFFF;
			hdr_printf("rgb_idx:%d, value; %d (*md) = 0x%x\n",
				     sei_dmaxrgb_idx[idx],
				     sei_dmaxrgb[idx],
				     (*metadata));
			metadata += 3;
		}

		hdr_printf("afterrgb: rembits = %d, *md = 0x%x\n",
			     remained_bits, *metadata);
		/* 10bit */

		if (remained_bits == 0) {
			metadata += 1;
			remained_bits = 8 + 8 - 10;	/* 3 */
		} else if (remained_bits <= 2) {
			metadata += 2;
			if (remained_bits == 2)
				remained_bits = remained_bits + 8 - 10;
			else
				remained_bits = remained_bits + 8 - 10 + 8;
		} else {
			metadata += 1;
			remained_bits = remained_bits + 8 - 10;	/* 3 */
		}


	}

	hdr_printf("before: remained_bits = %d, *metadata = 0x%x\n",
		     remained_bits, *metadata);

	/*skip luminance flag 1bit */
	if (remained_bits == 0) {
		metadata += 1;
		remained_bits = 7;
	} else if (remained_bits == 1) {
		metadata += 1;
		remained_bits = 0;
	} else
		remained_bits -= 1;

	for (num_idx = 0; num_idx < num_windows; ++num_idx) {
		/*1bit */
		if (remained_bits) {
			remained_bits--;
			tone_mapping_flag =
				(((*metadata)) >> remained_bits) & 0x1;
			if (remained_bits == 0)
				metadata += 1;
		} else {
			remained_bits = 7;
			tone_mapping_flag =
				(((*metadata)) >> remained_bits) & 0x1;
		}

		hdr_printf("[%d, %d]after:%d *md = 0x%x rembits = %d\n",
			     num_idx, num_windows,
			     tone_mapping_flag, *metadata,
			     remained_bits);
		if (tone_mapping_flag) {

			if (remained_bits)
				value = (((*metadata << 24) |
					  (*(metadata + 1) << 16) |
					  (*(metadata + 2) << 8) |
					  *(metadata + 3)) >> remained_bits) &
					  0xFFFFFF;
			else
				value = ((*metadata << 16) |
					 ((*(metadata + 1)) << 8) |
					 ((*(metadata + 2)))) >> remained_bits;

			sei_kpx = (value & 0xFFF000) >> 12;
			sei_kpy = (value & 0xFFF);
			metadata += 3;

			if (remained_bits >= 4) {
				remained_bits -= 4;
				num_sei_bz_a =
					((*metadata) >> remained_bits) & 0xF;

				if (remained_bits == 0)
					metadata += 1;
			} else if (remained_bits == 0) {
				remained_bits = 4;
				num_sei_bz_a =
					((*metadata) >> remained_bits) & 0xF;
			} else {
				remained_bits = 8 - 4 - remained_bits;
				num_sei_bz_a =
				    (((*metadata << 8)
				    | (*(metadata + 1)))
				    >> remained_bits) & 0xF;
			}
			memset(sei_bz_a, 0, sizeof(sei_bz_a));

			for (idx = 0; idx < num_sei_bz_a; ++idx) {
				if (remained_bits == 1) {
					remained_bits = 16 - (1 + 8);
					sei_bz_a[idx] = (((*metadata << 16)
						| (*(metadata + 1) << 8
						| (*metadata)))
						>> remained_bits)
						& 0x3FF;
					metadata += 1;
				} else if (remained_bits == 2) {
					remained_bits = remained_bits + 8 - 10;
					sei_bz_a[idx] = (((*metadata << 8)
						| (*(metadata + 1)))
						>> remained_bits)
						& 0x3FF;
					metadata += 2;
				} else if (remained_bits == 0) {
					remained_bits = 8 + 8 - 10;
					sei_bz_a[idx] = (((*metadata << 8)
						| (*(metadata + 1)))
						>> remained_bits)
						& 0x3FF;
					metadata += 1;
				} else {
					remained_bits = remained_bits + 8 - 10;
					sei_bz_a[idx] = (((*metadata << 8)
						| (*(metadata + 1)))
						>> remained_bits)
						& 0x3FF;
					metadata += 1;
				}
			}
		}

		if (remained_bits == 0) {
			metadata += 1;
			remained_bits = 7;
		} else if (remained_bits == 1) {
			metadata += 1;
			remained_bits = 0;
		} else
			remained_bits -= 1;
	}


	vsif_tsdmaxl =
		vsif_min(sel_tsdmaxl, 1, 1024, 32, 31);
	vsif_avgmaxrgb =
		vsif_min(sei_avgmaxrgb, 10, 4096, 16, 255);
	vsif[0] = (0x40) | (vsif_tsdmaxl << 1);
	vsif[1] = vsif_avgmaxrgb;
	for (idx = 0; idx < num_sei_bz_a; ++idx) {
		if (idx != 2)
			vsif_dmaxrgb[idx] =
			vsif_min(sei_dmaxrgb[idx], 10,
			4096, 16, 255);
		else
			vsif_dmaxrgb[idx] = sei_dmaxrgb[idx];
		vsif[2 + idx] = vsif_dmaxrgb[idx];
	}

	vsif_kpx = vsif_min(sei_kpx, 1, 0xFFFF, 4, 1023);

	vsif_kpy = vsif_min(sei_kpy, 1, 0xFFFF, 4, 1023);

	vsif[11] = (num_sei_bz_a << 4)
		| ((vsif_kpx & 0x3C0) >> 6);
	vsif[12] = ((vsif_kpx & 0x3F) << 2)
		| ((vsif_kpy & 0x300) >> 8);
	vsif[13] = (vsif_kpy & 0xFF);
	for (idx = 0; idx < 9; ++idx) {
		vsif_bz_a[idx] =
			vsif_min(sei_bz_a[idx], 1, 0xFFFF, 4, 255);
		vsif[14 + idx] = vsif_bz_a[idx];
	}

	if (osd_enable)
		vsif[23] = 0x80;
	else
		vsif[23] = 0x00;
}


uint32_t vdp_bitshift(int32_t needbits)
{
	uint32_t retValue = 1;
	int32_t temp = 0;

	temp = needbits;
	while (needbits) {
		retValue *= 2;
		needbits--;
	}
	return (retValue - 1);
}


UINT32 vdp_bitvalue_get(uint8_t *hdr_metadata,
	int32_t needbits, int32_t *remained_bits,
			uint32_t *addBytes)
{
	int32_t diff = 0;
	int32_t i = 0;
	uint32_t retValue = 0;
	*addBytes = 0;
	diff = needbits - (*remained_bits);
	if (diff < 0) {
		(*remained_bits) -= needbits;
		retValue = ((*hdr_metadata)
			>> (*remained_bits))
			& vdp_bitshift(needbits);
	} else {
		if (diff == 0) {
			retValue = ((*hdr_metadata) >> 0)
				& vdp_bitshift(needbits);
			*addBytes = 1;
			*remained_bits = 8;
		} else {
			if (diff - (diff / 8 * 8) > 0) {
				*addBytes = diff / 8 + 1;
				while (i <= (*addBytes)) {
					retValue +=
					    (*(hdr_metadata + i))
					    << (8 * ((*addBytes) - i));
					i++;
				}
				*remained_bits = (*addBytes) * 8 - diff;
				retValue = (retValue >> (*remained_bits))
					& vdp_bitshift(needbits);
			} else {
				(*addBytes) = diff / 8;
				while (i <= (*addBytes)) {
					retValue +=
					    (*(hdr_metadata + i))
					    << (8 * ((*addBytes) - i));
					i++;
				}

				retValue = (retValue >> 0)
					& vdp_bitshift(needbits);
				*remained_bits = 8;
				(*addBytes)++;
			}
		}
	}
	HDR_LOG("[vdp] bytes is %d value 0x%x, rembits %d\n",
		 (*addBytes), retValue, *remained_bits);
	return retValue;
}

static uint8_t user_data_registered_inu_t_t35_new(
	uint8_t *hdr_metadata,
	uint32_t size,
	uint8_t *vsif,
	uint32_t ctl_info)
{
	uint8_t country_code = 0;
	uint8_t application_identifier = 0;
	uint8_t application_version = 0;
	uint8_t num_windows = 0;
	uint8_t num_idx = 0;
	uint8_t idx = 0;
	uint32_t sel_tsdmaxl = 0;
	uint8_t vsif_tsdmaxl = 0;
	uint32_t sei_avgmaxrgb = 0;
	uint8_t vsif_avgmaxrgb = 0;
	uint8_t num_sei_dmaxrgb = 0;
	uint32_t sei_dmaxrgb[16];
	uint8_t sei_dmaxrgb_idx[16];
	uint8_t vsif_dmaxrgb[16];
	uint8_t tone_mapping_flag = 0;
	uint32_t sei_kpx = 0;
	uint16_t vsif_kpx = 0;
	uint32_t sei_kpy = 0;
	uint16_t vsif_kpy = 0;
	uint8_t num_sei_bz_a = 0;
	uint32_t sei_bz_a[9];
	uint8_t vsif_bz_a[9];
	int32_t remained_bits;
	uint32_t addBytes;
	uint32_t value;
	uint8_t *metadata = NULL;
	uint8_t graphic_overlay_flag = 0;
	uint8_t vsif_timing_mode = 0;

	if (hdr_metadata == NULL || vsif == NULL)
		return 0;
	metadata = hdr_metadata;
	country_code = *metadata;
	metadata += 5;
	application_identifier = *metadata;
	application_version = *(metadata + 1);
	metadata += 2;
	num_windows = (((*metadata) & 0xC0) >> 6);
	remained_bits = 6;

	/* 27 bit */
	sel_tsdmaxl = vdp_bitvalue_get(metadata, 27,
	&remained_bits, &addBytes);
	metadata += addBytes;	/* 2bit */
	HDR_LOG("[VDP]:0x%x num:%d tsdmaxl:0x%x *md=0x%x\n",
		 country_code, num_windows,
		 sel_tsdmaxl, *metadata);
	remained_bits = 2;
	for (num_idx = 0; num_idx < num_windows; ++num_idx) {
		HDR_LOG("[VDP]idx[%d] *(md)=0x%x rembits=%d\n",
			 num_idx, *(metadata), remained_bits);

		/* skip maxsel */
		(void)vdp_bitvalue_get(metadata, 51,
		&remained_bits, &addBytes);
		metadata += addBytes;

		HDR_LOG("[VDP]after *(md)=0x%x rembits=%d\n",
			 *(metadata), remained_bits);

		sei_avgmaxrgb = vdp_bitvalue_get(metadata,
			17, &remained_bits,
			&addBytes);
		metadata += addBytes;
		HDR_LOG("[VDP]rgb:%d,%d *md=0x%x, 0x%x, 0x%x\n",
			 sei_avgmaxrgb, remained_bits, *metadata,
			 *(metadata + 1), *(metadata + 2));

		num_sei_dmaxrgb = vdp_bitvalue_get(metadata, 4,
			&remained_bits, &addBytes);
		metadata += addBytes;
		HDR_LOG("[VDP]:%d *(md)=0x%x remained_bits=%d\n",
			 num_sei_dmaxrgb,
			 *(metadata),
			 remained_bits);
		memset(sei_dmaxrgb, 0, sizeof(sei_dmaxrgb));
		for (idx = 0; idx < num_sei_dmaxrgb; ++idx) {
			sei_dmaxrgb[idx] =
			    vdp_bitvalue_get(metadata, 24,
			    &remained_bits, &addBytes);
			metadata += addBytes;
			sei_dmaxrgb_idx[idx] =
				(sei_dmaxrgb[idx] & 0xFE0000) >> 17;
			sei_dmaxrgb[idx] =
				sei_dmaxrgb[idx] & 0x1FFFF;
			HDR_LOG("[VDP]:%d, value;%d (*md)=0x%x\n",
				 sei_dmaxrgb_idx[idx],
				 sei_dmaxrgb[idx],
				 (*metadata));
		}

		HDR_LOG("[VDP]after dmxrgb:  =%d, *md=0x%x\n",
			 remained_bits, *metadata);

		/* 10bit */
		(void)vdp_bitvalue_get(metadata, 10,
		&remained_bits, &addBytes);
		metadata += addBytes;

		HDR_LOG("[VDP]after bright_pixel:%d, *md=0x%x\n",
			 remained_bits, *metadata);
	}

	HDR_LOG("[VDP]before tone_mapping_flag:%d,*md=0x%x\n",
		 remained_bits, *metadata);

	/* skip luminance flag 1bit */
	(void)vdp_bitvalue_get(metadata, 1,
	&remained_bits, &addBytes);
	metadata += addBytes;

	for (num_idx = 0; num_idx < num_windows; ++num_idx) {
		/*1bit */
		tone_mapping_flag =
		vdp_bitvalue_get(metadata, 1, &remained_bits,
		&addBytes);
		metadata += addBytes;
		HDR_LOG("[VDP][%d, %d]:%d *md=0x%x remits=%d\n",
			 num_idx, num_windows,
			 tone_mapping_flag,
			 *metadata, remained_bits);
		memset(sei_bz_a, 0, sizeof(sei_bz_a));
		if (tone_mapping_flag) {
			value =
				vdp_bitvalue_get(metadata, 24,
				&remained_bits, &addBytes);
			metadata += addBytes;
			sei_kpx = (value & 0xFFF000) >> 12;
			sei_kpy = (value & 0xFFF);
			HDR_LOG("[VDP]sei_kpx:%d :%d *md=0x%x rembits=%d\n",
				 sei_kpx, sei_kpy, *metadata,
				 remained_bits);
			num_sei_bz_a =
				vdp_bitvalue_get(metadata, 4,
				&remained_bits, &addBytes);
			metadata += addBytes;
			HDR_LOG("[VDP]num_sei_bz_a:%d *md=0x%x rembits=%d\n",
				 num_sei_bz_a, *metadata, remained_bits);

			for (idx = 0; idx < num_sei_bz_a; ++idx) {
				HDR_LOG("[VDP]*md=0x%x rembits=%d\n",
					 *metadata, remained_bits);
				sei_bz_a[idx] =
				    vdp_bitvalue_get(metadata, 10,
				    &remained_bits, &addBytes);
				metadata += addBytes;
				HDR_LOG("[VDP]sei_bz_a[%d]%d 0x%x rb=%d\n",
					 idx, sei_bz_a[idx],
					 *metadata,
					 remained_bits);
			}
		}
		vdp_bitvalue_get(metadata, 1,
			&remained_bits, &addBytes);
		metadata += addBytes;
	}
	vsif_tsdmaxl =
		(uint8_t) vsif_min(sel_tsdmaxl,
		1, 1024, 32, 31);
	vsif_avgmaxrgb =
		(uint8_t) vsif_min(sei_avgmaxrgb,
		10, 4096, 16, 255);
	HDR_LOG("[VDP]vsif_tsdmaxl=%d =%d\n",
		vsif_tsdmaxl, vsif_avgmaxrgb);

	vsif[0] = (application_version << 6)
		| (vsif_tsdmaxl << 1);
	vsif[1] = vsif_avgmaxrgb;
	for (idx = 0; idx < 9; ++idx) {
		if ((idx != 2)
			|| (application_version == 0)) {
			vsif_dmaxrgb[idx] =
				(uint8_t) vsif_min(sei_dmaxrgb[idx],
				10, 4096, 16, 255);
			if ((idx == 8)
				&& (application_version == 0)
				&& (num_sei_dmaxrgb == 10))
				vsif_dmaxrgb[idx] =
				(uint8_t) vsif_min(
				sei_dmaxrgb[idx + 1],
				10, 4096, 16, 255);
		} else
			vsif_dmaxrgb[idx] =
			(uint8_t) sei_dmaxrgb[idx];
		vsif[2 + idx] = vsif_dmaxrgb[idx];
	}
	vsif_kpx = vsif_min(sei_kpx, 1, 0xFFFF, 4, 1023);
	vsif_kpy = vsif_min(sei_kpy, 1, 0xFFFF, 4, 1023);
	HDR_LOG("[VDP]sei_kpx=%d, vsif=%d, sei=%d, vsif=%d\n",
		 sei_kpx, vsif_kpx, sei_kpy, vsif_kpy);
	vsif[11] = (num_sei_bz_a << 4)
		| ((vsif_kpx & 0x3C0) >> 6);
	vsif[12] = ((vsif_kpx & 0x3F) << 2)
		| ((vsif_kpy & 0x300) >> 8);
	vsif[13] = (vsif_kpy & 0xFF);
	for (idx = 0; idx < 9; ++idx) {
		vsif_bz_a[idx] =
			(uint8_t) vsif_min(sei_bz_a[idx],
			1, 0xFFFF, 4, 255);
		vsif[14 + idx] = vsif_bz_a[idx];
		HDR_LOG("[VDP]vsif_bz_a=%d, sei_bz_a=%d\n",
			sei_bz_a[idx], vsif_bz_a[idx]);
	}

	if (ctl_info & VDP_HDR10PLUS_GRAPHIC_OVERLAY_FLAG)
		graphic_overlay_flag = 1;
	if (ctl_info & VDP_HDR10PLUS_VISF_TIMING_MODE)
		vsif_timing_mode = 1;
	vsif[23] = (graphic_overlay_flag << 7)
		| (vsif_timing_mode << 6);

	return 1;
}

char *vdp_print_hdr_type(enum HDR10_TYPE_ENUM hdr_type)
{
	switch (hdr_type) {
	case HDR10_TYPE_NONE:
		return "HDR10_TYPE_NONE";
	case HDR10_TYPE_ST2084:
		return "HDR10_TYPE_ST2084";
	case HDR10_TYPE_HLG:
		return "HDR10_TYPE_HLG";
	case HDR10_TYPE_PLUS:
		return "HDR10_TYPE_PLUS";
	default:
		return NULL;
	}
	return NULL;
}

void vdp_config_hdmi_signal(enum HDR10_TYPE_ENUM hdr_type,
	struct VID_STATIC_HDMI_MD_T *metadata,
			    bool enable)
{
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT

	VID_PLA_HDR_METADATA_INFO_T rHdr = { 0 };
	static bool hdr_enabled;
	static bool hlg_enabled;

	switch (hdr_type) {
	case HDR10_TYPE_ST2084:
		if (enable
			&& disp_common_info.tv.is_support_hdr) {
			/* enable ST2084 */
			if (!hdr_enabled) {
				rHdr.fgIsMetadata = true;
				rHdr.e_DynamicRangeType =
					VID_PLA_DR_TYPE_HDR10;
				memcpy(&rHdr.metadata_info.hdr10_metadata,
				       metadata,
				       sizeof(struct VID_STATIC_HDMI_MD_T));
				vSetStaticHdrType(GAMMA_ST2084);
				vVdpSetHdrMetadata(true, rHdr);
				vHdrEnable(true);
				hdr_printf("hdmi : %s enable:%d\n",
					     vdp_print_hdr_type(hdr_type),
					     true);
			}
			hdr_enabled = true;
		} else {
			/* disable ST2084 */
			if (hdr_enabled) {
				vHdrEnable(false);
				hdr_printf("hdmi: %s enable:%d\n",
					     vdp_print_hdr_type(hdr_type),
					     false);
			}
			hdr_enabled = false;
		}
		break;
	case HDR10_TYPE_HLG:
		if (enable
			&& disp_common_info.tv.is_support_hlg) {
			/* enable hlg */
			if (!hlg_enabled) {
				rHdr.fgIsMetadata = true;
				rHdr.e_DynamicRangeType =
					VID_PLA_DR_TYPE_HDR10;
				memcpy(&rHdr.metadata_info.hdr10_metadata,
				       metadata,
				       sizeof(struct VID_STATIC_HDMI_MD_T));
				vSetStaticHdrType(GAMMA_HLG);
				vVdpSetHdrMetadata(true, rHdr);
				vHdrEnable(true);
				hdr_printf("hdmi: %s enable:%d\n",
					     vdp_print_hdr_type(hdr_type),
					     true);
			}
			hlg_enabled = true;
		} else {
			/* disable hlg */
			if (hlg_enabled) {
				vHdrEnable(false);
				hdr_printf("hdmi : %s enable:%d\n",
					     vdp_print_hdr_type(hdr_type),
					     false);
			}
			hlg_enabled = false;
		}
		break;
	case HDR10_TYPE_PLUS:
		if (disp_common_info.tv.is_support_hdr10_plus) {
			hdr_printf("hdmi: %s apver:0x%x enable:%d\n",
				     vdp_print_hdr_type(hdr_type),
				     disp_common_info.tv.hdr10_plus_app_ver,
				     enable);
			hdr_printf("tv HDR10+,HDRst2084 in parallel\n");

			if (enable) {
				rHdr.fgIsMetadata = true;
				rHdr.e_DynamicRangeType =
					VID_PLA_DR_TYPE_HDR10;
				memcpy(&rHdr.metadata_info.hdr10_metadata,
				       metadata,
				       sizeof(struct VID_STATIC_HDMI_MD_T));
				vSetStaticHdrType(GAMMA_ST2084);
				vVdpSetHdrMetadata(true, rHdr);
			}
			if (disp_common_info.tv.hdr10_plus_app_ver
				!= 0xFF)
				vHdr10PlusVSIFEnable(enable);
			else
				vHdr10PlusEnable(enable);
		} else {
			hdr_printf("tv HDR10+, use st2084 instead\n");
			vdp_config_hdmi_signal(HDR10_TYPE_ST2084,
				metadata, enable);
		}
		break;
	default:
		break;
	}
#endif
}


void vdp_fill_metadata_info(enum VID_PLA_DR_TYPE_T type,
			    VID_PLA_HDR_METADATA_INFO_T *rHdr,
			    void *buf_add, uint32_t len)
{
	uint8_t vsif[32];
	uint32_t vsiflen = 0;
	uint32_t hdr10plusctlbit = 0;
	VID_HDR10_PLUS_METADATA_INFO_T *hdr10pmdifo;

	hdr10pmdifo = NULL;
	rHdr->fgIsMetadata = true;
	rHdr->e_DynamicRangeType = type;

	hdr10pmdifo =
		&rHdr->metadata_info.hdr10_plus_metadata.hdr10p_metadata_info;

	if (type == VID_PLA_DR_TYPE_HDR10_PLUS_VSIF) {
		memset(vsif, 0, sizeof(vsif));
		vsif[0] = 0x8B;
		vsif[1] = 0x84;
		vsif[2] = 0x90;

		/* add vsif control bit   */
		/*hdr10plusctlbit |= VDP_HDR10PLUS_VISF_TIMING_MODE; */
		if (!rHdr->is_enter_async_mode)
			hdr10plusctlbit |= VDP_HDR10PLUS_VISF_TIMING_MODE;
		if (osd_enable_sdr2hdr && graphic_overlay_for_hdr10plus)
			hdr10plusctlbit |= VDP_HDR10PLUS_GRAPHIC_OVERLAY_FLAG;
		user_data_registered_inu_t_t35_new(buf_add, len, &(vsif[3]),
			hdr10plusctlbit);
		vsiflen = 27;
		memcpy(buf_add, vsif, vsiflen);
		hdr10pmdifo->ui4_Hdr10PlusAddr = (uint64_t)buf_add;
		hdr10pmdifo->ui4_Hdr10PlusSize = vsiflen;
			} else {
				hdr10pmdifo->ui4_Hdr10PlusAddr =
					(uint64_t)buf_add;
				hdr10pmdifo->ui4_Hdr10PlusSize =
					len;
			}
}

void pass_osd_buffer_info(
	struct mtk_disp_buffer *osd_disp_buffer,
	struct disp_hw_common_info *common_info)
{
	enum HDR_PATH_ENUM path;

	if ((osd_disp_buffer == NULL)
		|| (common_info == NULL)) {
		DISP_LOG_E("vdp routine pass err param at %s\n",
			__func__);
		return;
	}

	path = _hdr_core_handle_disp_config(
		osd_disp_buffer,
		common_info);
	_hdr_core_handle_disp_update(path);
}
#endif

void disp_hdr_set_event(uint32_t event)
{
	mutex_lock(&disp_hdr_thread_mutex);
	//hdr_printf("dovi set event 0x%X %lu\n",
	//	event, disp_hdr_thread_cnt);
	disp_hdr_event |= event;
	mutex_unlock(&disp_hdr_thread_mutex);
}

void disp_hdr_wakeup_routine(uint32_t thread_id)
{
	//hdr_printf("%s %d\n", __func__, thread_id);
	switch (thread_id) {
	case 0:
		atomic_set(&gWakeupHdrThread0, 1);
		wake_up(&disp_hdr_thread_wq0);
		break;
	case 1:
		atomic_set(&gWakeupHdrThread1, 1);
		wake_up(&disp_hdr_thread_wq1);
		break;
	case 2:
		atomic_set(&gWakeupHdrThread2, 1);
		wake_up(&disp_hdr_thread_wq2);
		break;
	case 3:
		atomic_set(&gWakeupHdrThread3, 1);
		wake_up(&disp_hdr_thread_wq3);
		break;
	case 4:
		atomic_set(&gWakeupHdrThread4, 1);
		wake_up(&disp_hdr_thread_wq4);
		break;
	}
}

static int disp_hdr_routine0(void *data)
{
	int wait_ret = 0;

	while (1) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq0,
			atomic_read(&gWakeupHdrThread0));
		if (wait_ret)
			hdr_error("wait hdr routine0 error");
		atomic_set(&gWakeupHdrThread0, 0);

		++disp_hdr_thread_cnt;

		if ((disp_hdr_event & HDR_EVENT_VLAYER0_CFG_DONE)
			&& (disp_hdr_event & HDR_EVENT_VLAYER1_CFG_DONE)) {
			disp_hdr_event &= ~HDR_EVENT_VLAYER0_CFG_DONE;
			disp_hdr_event &= ~HDR_EVENT_VLAYER1_CFG_DONE;

			disp_hdr_path_judge();
		}
	}
	return 0;
}

static int disp_hdr_routine1(void *data)
{
	struct video_buffer_info *buf_sub = NULL;
	int wait_ret = 0;

	while (1) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq1,
			atomic_read(&gWakeupHdrThread1));
		if (wait_ret)
			hdr_error("wait hdr routine1 error");
		atomic_set(&gWakeupHdrThread1, 0);

		//hdr_printf("hdrthread1 wakeup\n");
		if (bsub_exist) {
			buf_sub = &hdr_video_layer[LAYER1];
			disp_cfd_config_video_frame(1, buf_sub,
				&(dovi_common_info.tv));
		}
	}
	return 0;
}

static int disp_hdr_routine2(void *data)
{
	struct mtk_disp_buffer *disp_buf_osd = NULL;
	int wait_ret = 0;

	disp_buf_osd = &hdr_osd_layer[LAYER0];

	while (1) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq2,
			atomic_read(&gWakeupHdrThread2));
		if (wait_ret)
			hdr_error("wait hdr routine2 error");
		atomic_set(&gWakeupHdrThread2, 0);

		if (disp_buf_osd == NULL) {
			pr_err("disp_buf_osd NULL\n");
			return 0;
		}

		if (!tv_info_set_by_cmd)
			disp_hw_mgr_get_info(&dovi_common_info);

		//pass_osd_buffer_info(disp_buf_osd, &disp_common_info);
		disp_cfd_config_graphic_frame(2, disp_buf_osd,
			&(dovi_common_info.tv));

		/* generate cfg setting */

		/* gce update */
	}
	return 0;
}

static int disp_hdr_routine3(void *data)
{
	struct mtk_disp_buffer *disp_buf_osd = NULL;
	int wait_ret = 0;

	disp_buf_osd = &hdr_osd_layer[LAYER1];

	while (1) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq3,
			atomic_read(&gWakeupHdrThread3));
		if (wait_ret)
			hdr_error("wait hdr routine3 error");
		atomic_set(&gWakeupHdrThread3, 0);

		if (disp_buf_osd == NULL) {
			hdr_printf("disp_buf_osd NULL\n");
			return 0;
		}

		if (!tv_info_set_by_cmd)
			disp_hw_mgr_get_info(&dovi_common_info);
		//pass_osd_buffer_info(disp_buf_osd, &disp_common_info);
		disp_cfd_config_graphic_frame(3, disp_buf_osd,
			&(dovi_common_info.tv));

		/* generate cfg setting */

		/* gce update */
	}
	return 0;
}

static int disp_hdr_routine4(void *data)
{
	uint32_t i = 0;
	int wait_ret = 0;

	while (1) {
		wait_ret = wait_event_interruptible(disp_hdr_thread_wq4,
			atomic_read(&gWakeupHdrThread4));
		if (wait_ret)
			hdr_error("wait hdr routine4 error");
		atomic_set(&gWakeupHdrThread4, 0);

		if (disp_hdr_irq_event == 0) {
			hdr_printf("no relate irq event\n");
			return 0;
		}

		for (i = CLK_CHG_FE0; i < CLK_CHG_BE; i++) {
			if (disp_hdr_irq_event & (1 << i)) {
				disp_hdr_fe_set_clk(i, false);
				disp_hdr_irq_event &=
					~((1 << i) & 0xff);
			}
		}

		if (disp_hdr_irq_event & (1 << CLK_CHG_BE)) {
			disp_hdr_set_vdo_be_clk(false);
			disp_hdr_irq_event &=
				~((1 << CLK_CHG_BE) & 0xff);
		}

		if (disp_hdr_irq_event & (1 << FORCE_HDR_CHG)) {
			disp_hdr_irq_event &= ~((1 << FORCE_HDR_CHG) & 0xff);
			disp_hdr_handle_forcehdr(DISP_CMD_FORCE_HDR,
			(void *)(&ui_force_hdr_type));
		}
	}
	return 0;
}



int disp_hdr_thread_init(void)
{
	struct sched_param hdr_param = {.sched_priority = 2};

	if (!(disp_hdr_thread0 && disp_hdr_thread1
		&& disp_hdr_thread2 && disp_hdr_thread3
		&& disp_hdr_thread4)) {
		mutex_init(&disp_hdr_thread_mutex);
		hdr_info("mutex init\n");

		init_waitqueue_head(&disp_hdr_thread_wq0);
		init_waitqueue_head(&disp_hdr_thread_wq1);
		init_waitqueue_head(&disp_hdr_thread_wq2);
		init_waitqueue_head(&disp_hdr_thread_wq3);
		init_waitqueue_head(&disp_hdr_thread_wq4);

		atomic_set(&gWakeupHdrThread0, 0);
		atomic_set(&gWakeupHdrThread1, 0);
		atomic_set(&gWakeupHdrThread2, 0);
		atomic_set(&gWakeupHdrThread3, 0);
		atomic_set(&gWakeupHdrThread4, 0);
		hdr_info("wq init\n");

		disp_hdr_thread0 =
			kthread_create(disp_hdr_routine0,
			NULL, "disp_hdr_thread0");

		disp_hdr_thread1 =
			kthread_create(disp_hdr_routine1,
			NULL, "disp_hdr_thread1");

		disp_hdr_thread2 =
			kthread_create(disp_hdr_routine2,
			NULL, "disp_hdr_thread2");

		disp_hdr_thread3 =
			kthread_create(disp_hdr_routine3,
			NULL, "disp_hdr_thread3");

		disp_hdr_thread4 =
			kthread_create(disp_hdr_routine4,
			NULL, "disp_hdr_thread4");

		wake_up_process(disp_hdr_thread0);
		wake_up_process(disp_hdr_thread1);
		wake_up_process(disp_hdr_thread2);
		wake_up_process(disp_hdr_thread3);
		wake_up_process(disp_hdr_thread4);

		hdr_info("thread %p %p %p %p %p init\n",
			disp_hdr_thread0,
			disp_hdr_thread1,
			disp_hdr_thread2,
			disp_hdr_thread3,
			disp_hdr_thread4);
	}
	sched_setscheduler(disp_hdr_thread0, SCHED_RR, &hdr_param);
	return 0;
}


int disp_hdr_init(struct disp_hw_common_info *info)
{
	if (info == NULL) {
		hdr_printf("%s err\n", __func__);
		return -1;
	}

	mutex_init(&disp_hdr_mutex);
	disp_hdr_thread_init();
	/*enable hdr clk and path
	 * fhd hdr always enable when init(rgb2yuv for osd)
	 * hdr be enable when forcedolby path
	 */
	disp_hdr_fe_start_stop(LAYER2, true);
	disp_hdr_fe_start_stop(LAYER3, true);
	if (g_force_dolby)
		disp_hdr_vdo_be_start_stop(true);

	//if (g_dovi_efuse) {
	if (disp_dovi_init(info) != 0)
		hdr_printf("dovi_hdr init fail\n");
	//}
	if (disp_cfd_init(info) != 0)
		hdr_printf("open_hdr init fail\n");


	if (g_hdmi_res == HDMI_VIDEO_1280x720p_59_94Hz)
		old_resolution = HDMI_VIDEO_1280x720p_60Hz;
	else if (g_hdmi_res == HDMI_VIDEO_1920x1080p_59_94Hz)
		old_resolution = HDMI_VIDEO_1920x1080p_60Hz;
	else if (g_hdmi_res == HDMI_VIDEO_3840x2160P_59_94HZ)
		old_resolution = HDMI_VIDEO_3840x2160P_60HZ;
	else
		old_resolution = g_hdmi_res;

	hdr_output_signal_type = g_out_format;

	hdr_info("%s lk info:res = %d, force(%d %d %d %d %d) ver %d\n",
		__func__, old_resolution, g_force_hdr,
		g_force_dolby, g_dovi_efuse,
		g_force_open_hdr, g_out_format, g_ic_version);

	if (g_force_dolby) {
		// force dolby vs10
		disp_dovi_force_gfx_vs10();
	} else if (g_force_open_hdr) {
		// defaults open cfd from lk and just clk on keep
		disp_cfd_enable(HDR_CFD_LAYER_FHD_GFX, true);
	} else {
		// defaults open r2y
		disp_cfd_enable(HDR_CFD_LAYER_FHD_GFX, true);
		disp_cfd_drv_set_bypass(HDR_CFD_LAYER_FHD_GFX);
		disp_cfd_drv_set_r2y(HDR_CFD_LAYER_FHD_GFX, 0);
	}

	vdp_start_st[0] = 0;
	vdp_start_st[1] = 0;
	adl_mode = 0x2800;
	hdr_init_done = true;
	hdr_debug_init();

	return 0;
}

int disp_hdr_deinit(void)
{
	if (g_dovi_efuse)
		disp_dovi_deinit();

	if (disp_cfd_deinit() != 0)
		hdr_printf("open_hdr deinit fail\n");

	hdr_init_done = false;

	return 0;
}

#ifdef CONFIG_HDMI_BLACK
int disp_hdr_deep_suspend(void)
{
	if (dolby_path_enable) {
		disp_hdr_vdo_be_start_stop(false);
		fg_hdr_deep_suspend = true;
		disp_hdr_wakeup_routine(4);
		hdr_printf("hdr deep suspend\n");
	}
	return 0;
}

#endif

int disp_hdr_suspend(void)
{

	if (disp_dovi_suspend() == 0)
		hdr_printf("dovi_hdr suspend\n");

	disp_hdr_fe_start_stop(LAYER0, false);
	disp_hdr_fe_start_stop(LAYER1, false);
	disp_hdr_fe_start_stop(LAYER2, false);
	disp_hdr_fe_start_stop(LAYER3, false);
#ifndef CONFIG_HDMI_BLACK
	disp_hdr_vdo_be_start_stop(false);
#endif
	hdr_init_done = false;
	hdr_printf("%s done\n", __func__);
	return 0;
}

int disp_hdr_resume(void)
{
	#ifdef CONFIG_HDMI_BLACK
	if (fg_hdr_deep_suspend) {
		disp_hdr_vdo_be_start_stop(true);
		disp_dovi_force_gfx_vs10();
		fg_hdr_deep_suspend = false;
		hdr_printf("hdr deep resume\n");
	}
	#endif

	if (g_dovi_efuse)
		dolby_path_ready2start = 1;

	hdr_printf("%s %d done\n", __func__, dolby_path_enable);
	return 0;
}

int disp_hdr_change_resolution(
	const struct disp_hw_resolution *info)
{
	struct disp_hw_tv_capbility tv_cap;
	struct disp_hw *hdr_drv = disp_hdr_get_drv();
	enum HDR_PATH hdr_path = DEFAULT_PATH;
	struct video_buffer_info *cur_buf = NULL;
	uint32_t res_mod = 0;

	if (info == NULL)
		return -1;

	hdr_res_width = info->width;
	hdr_res_height = info->height;
	hdr_input_width[0] = hdr_res_width;
	hdr_input_height[0] = hdr_res_height;
	hdr_input_width[1] = hdr_res_width;
	hdr_input_height[1] = hdr_res_height;

	res_mod = info->res_mode;
	disp_dovi_resolution_change(info);
	hdr_drv->drv_call(DISP_CMD_GET_HDMI_CAP, &tv_cap);
	if (dolby_path_enable) {
		dovi_update_res_change(&tv_cap, info);
		hdr_path = DOVI_PATH;
	} else {
		//opendhr change res
		cur_buf = &hdr_video_layer[LAYER0];
		disp_cfd_chg_output(cur_buf, info, &tv_cap);
		hdr_path = OPENHDR_PATH;
	}
	disp_hdr_config_hdmi_signal(hdr_path);
	hdr_printf("path[%d] chg res[%d] done\n",
		hdr_path, res_mod);

	return 0;
}

int disp_hdr_irq_handler(uint32_t irq)
{
	switch (irq) {
	case DISP_IRQ_BEFIFO:
		befifo_irq_cnt++;
		if (disp_hdr_irq_event)
			disp_hdr_irq_handle();
		break;
	case DISP_IRQ_FMT_VSYNC:
		if (g_dovi_efuse)
			disp_dovi_irq_handler(irq);
		break;
	default:
		break;
	}
	return 0;
}

int disp_hdr_cmd(enum DISP_CMD cmd, void *data)
{
	switch (cmd) {
	case DISP_CMD_FORCE_HDR:
		ui_force_hdr_type = *((uint32_t *) data);
		disp_hdr_irq_event |= 1 << FORCE_HDR_CHG;
		//disp_hdr_handle_forcehdr(cmd, data);
		break;
	case DISP_CMD_OSD_START:
		disp_hdr_handle_osd_start(cmd, data);
		break;
	case DISP_CMD_OSD_STOP:
		disp_hdr_handle_osd_stop(cmd, data);
		break;
	case DISP_CMD_VDP_START:
		disp_hdr_handle_vdp_start(cmd, data);
		break;
	case DISP_CMD_VDP_STOP:
		disp_hdr_handle_vdp_stop(cmd, data);
		break;
	case DISP_CMD_GCE_SET_HDR_ID:
		/* save hdr gce memory */
		break;
	default:
		if (g_dovi_efuse)
			disp_dovi_process_cmd(LAYER0, cmd, data);
		//hdr_core_handle_other_moudule_call(cmd, data);
		break;
	}
	return 0;
}


struct disp_hw disp_hdr_driver = {
	.name = HDR_DRV_NAME,
	.init = disp_hdr_init,
	.deinit = disp_hdr_deinit,
	.start = NULL,
	.stop = NULL,

#ifdef CONFIG_HDMI_BLACK
	.deep_suspend = disp_hdr_deep_suspend,
	.deep_resume = NULL,
#endif
	.suspend = disp_hdr_suspend,
	.resume = disp_hdr_resume,
	.get_info = NULL,
	.change_resolution = disp_hdr_change_resolution,
	.config = NULL,
	.irq_handler = disp_hdr_irq_handler,
	.set_listener = NULL,
	.wait_event = NULL,
	.dump = NULL,
	.set_cmd = disp_hdr_cmd,
};

struct disp_hw *disp_hdr_get_drv(void)
{
	return &disp_hdr_driver;
}

