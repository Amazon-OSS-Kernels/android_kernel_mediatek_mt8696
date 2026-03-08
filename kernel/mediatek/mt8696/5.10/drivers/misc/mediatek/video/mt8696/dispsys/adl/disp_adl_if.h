/*
 * Copyright (C) 2020 MediaTek Inc.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __DISP_ADL_IF_H__
#define __DISP_ADL_IF_H__

#include "disp_hw_mgr.h"

#define ADL_DRV_NAME  "disp_drv_adl"

#define ADL_DTS_MAX_ND_SIZE 30

#define ADL_IOMMU_SUPPORT 1

#define BYTE_PER_LINE (32)

#define ADL_LUT_BUFFER_SIZE 41184 //1287 line x 32 bytes perline

#define ADL_REG_COUNT (50)

#define MD_PKT_SIZE (124)
#define MD_MAX_SIZE (1280)
#define FE_ADL_SIZE (1024)
#define B0103_ADL_SIZE (256)
#define GFX_FE_LUT_SIZE (0x4000)


enum ADL_STATUS {
	ADL_OK = 0,
	ADL_ERR = 1,
};
enum ADL_CLK_ID {
	DISPSYS_ADL = 0,
	MMSYS_ADL = 1
};

enum ADL_CLIT_REG_ID {
	//main v dv/mtk hdrfe
	ADL_DSYS_CLITA = 0,
	//main fgrain
	ADL_DSYS_CLITB = 1,
	//sub v dv.mtk hdrfe
	ADL_DSYS_CLITE = 2,
	//sub fgrain
	ADL_DSYS_CLITJ = 3,
	//fhd gfx dv
	ADL_MSYS_CLITA = 4,
	//be scramble
	ADL_MSYS_CLITB = 5,
	//fhd gfx thdr
	ADL_MSYS_CLITC = 6,
	//uhd gfx thdr
	ADL_MSYS_CLITD = 7,
	//uhd gfx dv
	ADL_MSYS_CLITE = 8,
	ADL_CLIT_REG_MAX,
};

enum ADL_CLIENT {
	DV_ADL_V_MAIN = 0,
	DV_ADL_V_SUB = 1,
	DV_ADL_G_FHD = 2,
	DV_ADL_G_UHD = 3,
	DV_SCRM = 4,
	THDR_ADL_V_MAIN = 5,
	THDR_ADL_V_SUB = 6,
	THDR_ADL_G_FHD = 7,
	THDR_ADL_G_UHD = 8,
	FILM_GRAIN_MAIN = 9,
	FILM_GRAIN_SUB = 10,
	ADL_CLIENT_MAX,
};

enum ADL_REG_BASE {
	ADL_DSYS = 0,
	ADL_MSYS = 1,
	ADL_MAX,
};

struct adl_tbl_elemt {
	uint8_t *p_data; //src lut table
	uint32_t used_size; //lut table size (bytes)
};

struct adl_dst_buf {
	uint8_t *dst_va;
	dma_addr_t dst_mva;
	uint32_t size;
};

enum ADL_PATH {
	ADL_DEF = 0,
	ADL_DOVI = 1,
	ADL_OPENHDR = 2,
	ADL_PATH_MAX,
};

struct adl_src_tbl {
	enum ADL_CLIENT client;
	bool update;
	uint8_t path;
	struct adl_tbl_elemt hdr_b0103;
	struct adl_tbl_elemt hdr_b0202ss;
	struct adl_tbl_elemt hdr_b0202si;
	struct adl_tbl_elemt hdr_b0202ts;
	struct adl_tbl_elemt hdr_b0202ti;
	struct adl_tbl_elemt hdr_b0105;
	struct adl_tbl_elemt hdr_ootf;
	struct adl_tbl_elemt tosd_degam;
	struct adl_tbl_elemt scmb;
	struct adl_tbl_elemt fgrain;
};

struct adl_reg_tbl {
	uint32_t depth;
	uint32_t address[ADL_REG_COUNT];
	uint16_t value[ADL_REG_COUNT];
	uint16_t mask[ADL_REG_COUNT];
};

struct disp_adl_clt_context {
	bool inited;
	bool enabled;
	uintptr_t remap_reg_base;
	uint32_t hw_reg_base;
	uint32_t clt_update_mode; //sw tirg, vfd, vsyncdelay(0,1,2)
	uint32_t reg_conf_mode; //gce, ml, cpu(0,1,2)
	void *lut_addr_va;
	dma_addr_t lut_addr_mva;
	uint32_t lut_addr_pa;
	struct adl_reg_tbl *preg_tbl;
	struct cmdqRecStruct *gce_handle;
	struct adl_src_tbl src_tbl;
};


#define WRITE_B0105_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x00) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = ((((*(baseaddr + 13)) & 0xB0) | 0xC0) \
	| (((value) >> 12) & 0x0F)); \
	*(baseaddr + 12) = (((value) >> 4) & 0xFF); \
	*(baseaddr + 11) = (((*(baseaddr + 11)) & 0x0F) \
	| (((value) << 4) & 0xF0)); \
} while (0)

#define WRITE_B0202_SMLUTS_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x00) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((*(baseaddr + 13) & 0xDF) | 0xA0)); \
	*(baseaddr + 11) = (((*(baseaddr + 11)) & 0xF0) \
	| (((value) >> 11) & 0x0F)); \
	*(baseaddr + 10) = (((value) >> 3) & 0xFF); \
	*(baseaddr + 9) = (((*(baseaddr + 9)) & 0x1F) \
	| ((value << 5) & 0xE0)); \
} while (0)

#define WRITE_B0202_SMLUTI_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x00) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF);\
	*(baseaddr + 13) = (((*(baseaddr + 13) & 0xDF) | 0xA0)); \
	*(baseaddr + 9) = (((*(baseaddr + 9)) & 0xE0) \
	| (((value) >> 10) & 0x1F)); \
	*(baseaddr + 8) = (((value) >> 2) & 0xFF); \
	*(baseaddr + 7) = (((*(baseaddr + 7)) & 0x3F) \
	| (((value) << 6) & 0xC0)); \
} while (0)

#define WRITE_B0202_TMLUTS_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x00) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF);\
	*(baseaddr + 13) = (((*(baseaddr + 13) & 0xDF) | 0xA0));\
	*(baseaddr + 7) = (((*(baseaddr + 7)) & 0xC0) \
	| (((value) >> 9) & 0x3F)); \
	*(baseaddr + 6) = (((value) >> 1) & 0xFF);\
	*(baseaddr + 5) = (((*(baseaddr + 5)) & 0x7F) \
	| (((value) << 7) & 0x80)); \
} while (0)

#define WRITE_B0202_TMLUTI_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x00) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((*(baseaddr + 13) & 0xDF) | 0xA0)); \
	*(baseaddr + 5) = (((*(baseaddr + 5)) & 0x80) \
	| (((value) >> 8) & 0x7F)); \
	*(baseaddr + 4) = ((value) & 0xFF); \
} while (0)

#define WRITE_B0103_SMLUTS_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x00) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((*(baseaddr + 13) & 0xEF) | 0x90)); \
	*(baseaddr + 3) = (((value) >> 24) & 0xFF); \
	*(baseaddr + 2) = (((value) >> 16) & 0xFF); \
	*(baseaddr + 1) = (((value) >> 8) & 0xFF); \
	*(baseaddr)      = ((value) & 0xFF); \
} while (0)

#define WRITE_CFD_VDO_LUTS_DATA(baseaddr, index, b0103, b0105, b02ss, \
b02si, b02ts, b02ti) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x00) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((index < B0103_ADL_SIZE) ? \
	( 0xF0) : \
	((*(baseaddr + 13) & 0xF0) | 0xE0)) \
	| (((b0105) >> 12) & 0x0F)); \
	*(baseaddr + 12) = (((b0105) >> 4) & 0xFF); \
	*(baseaddr + 11) = ((((b0105) << 4) & 0xF0) \
	| (((b02ss) >> 11) & 0x0F)); \
	*(baseaddr + 10) = (((b02ss) >> 3) & 0xFF); \
	*(baseaddr + 9) = (((b02ss << 5) & 0xE0) \
	| (((b02si) >> 10) & 0x1F)); \
	*(baseaddr + 8) = (((b02si) >> 2) & 0xFF); \
	*(baseaddr + 7) = ((((b02si) << 6) & 0xC0) \
	| (((b02ts) >> 9) & 0x3F)); \
	*(baseaddr + 6) = (((b02ts) >> 1) & 0xFF);\
	*(baseaddr + 5) = ((((b02ts) << 7) & 0x80) \
	| (((b02ti) >> 8) & 0x7F)); \
	*(baseaddr + 4) = ((b02ti) & 0xFF); \
	*(baseaddr + 3) = (((b0103) >> 24) & 0xFF); \
	*(baseaddr + 2) = (((b0103) >> 16) & 0xFF); \
	*(baseaddr + 1) = (((b0103) >> 8) & 0xFF); \
	*(baseaddr)      = ((b0103) & 0xFF); \
} while (0)

#define WRITE_DV_LUTS_DATA(baseaddr, index, b0103, b02ss, \
b02si, b02ts, b02ti) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x00) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = ((index < B0103_ADL_SIZE) ? \
	((*(baseaddr + 13) & 0xEF) | 0xB0) : \
	((*(baseaddr + 13) & 0xDF) | 0xA0)); \
	*(baseaddr + 11) = (((*(baseaddr + 11)) & 0xF0) \
	| (((b02ss) >> 11) & 0x0F)); \
	*(baseaddr + 10) = (((b02ss) >> 3) & 0xFF); \
	*(baseaddr + 9) = (((b02ss << 5) & 0xE0) \
	| (((b02si) >> 10) & 0x1F)); \
	*(baseaddr + 8) = (((b02si) >> 2) & 0xFF); \
	*(baseaddr + 7) = ((((b02si) << 6) & 0xC0) \
	| (((b02ts) >> 9) & 0x3F)); \
	*(baseaddr + 6) = (((b02ts) >> 1) & 0xFF);\
	*(baseaddr + 5) = ((((b02ts) << 7) & 0x80) \
	| (((b02ti) >> 8) & 0x7F)); \
	*(baseaddr + 4) = ((b02ti) & 0xFF); \
	*(baseaddr + 3) = (((b0103) >> 24) & 0xFF); \
	*(baseaddr + 2) = (((b0103) >> 16) & 0xFF); \
	*(baseaddr + 1) = (((b0103) >> 8) & 0xFF); \
	*(baseaddr)      = ((b0103) & 0xFF); \
} while (0)

#define WRITE_OOTF_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = ((((*(baseaddr + 15)) & 0x00) | 0x80) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 1) = (((value) >> 8) & 0xFF); \
	*(baseaddr)      = ((value) & 0xFF); \
} while (0)

#define WRITE_SCRAMBLE_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = (((*(baseaddr + 15)) & 0xF8) \
	| (((index) >> 8) & 0x07)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr)      = ((value) & 0xFF); \
} while (0)

#define WRITE_OSD_B0202_SMLUTS_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = (((*(baseaddr + 15)) & 0xFE) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((*(baseaddr + 13)) & 0xDF) | 0xA0); \
	*(baseaddr + 11) = (((*(baseaddr + 11)) & 0xF0) \
	| (((value) >> 11) & 0x0F)); \
	*(baseaddr + 10) = (((value) >> 3) & 0xFF);\
	*(baseaddr + 9) = (((*(baseaddr + 9)) & 0x1F) \
	| (((value) << 5) & 0xE0)); \
} while (0)

#define WRITE_OSD_B0202_SMLUTI_DATA_FASTMODE(baseaddr, index, value) do {\
	*(baseaddr + 15) = (((*(baseaddr + 15)) & 0xFE) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((*(baseaddr + 13)) & 0xDF) | 0xA0); \
	*(baseaddr + 9) = (((*(baseaddr + 9)) & 0xE0) \
	| (((value) >> 10) & 0x1F)); \
	*(baseaddr + 8) = (((value) >> 2) & 0xFF); \
	*(baseaddr + 7) = (((*(baseaddr + 7)) & 0x3F) \
	| (((value) << 6) & 0xC0)); \
} while (0)

#define WRITE_OSD_B0202_TMLUTS_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = (((*(baseaddr + 15)) & 0xFE) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((*(baseaddr + 13)) & 0xDF) | 0xA0); \
	*(baseaddr + 7) = (((*(baseaddr + 7)) & 0xC0) \
	| (((value) >> 9) & 0x3F)); \
	*(baseaddr + 6) = (((value) >> 1) & 0xFF); \
	*(baseaddr + 5) = (((*(baseaddr + 5)) & 0x7F) \
	| (((value) << 7) & 0x80)); \
} while (0)

#define WRITE_OSD_B0202_TMLUTI_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = (((*(baseaddr + 15)) & 0xFE) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((*(baseaddr + 13)) & 0xDF) | 0xA0); \
	*(baseaddr + 5) = (((*(baseaddr + 5)) & 0x80) \
	| (((value) >> 8) & 0x7F)); \
	*(baseaddr + 4) = ((value) & 0xFF); \
} while (0)

#define WRITE_OSD_B0103_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + 15) = (((*(baseaddr + 15)) & 0xFE) \
	| (((index) >> 8) & 0x01)); \
	*(baseaddr + 14) = ((index) & 0xFF); \
	*(baseaddr + 13) = (((*(baseaddr + 13)) & 0xEF) | 0x90); \
	*(baseaddr + 3) = (((value) >> 24) & 0xFF); \
	*(baseaddr + 2) = (((value) >> 16) & 0xFF); \
	*(baseaddr + 1) = (((value) >> 8) & 0xFF); \
	*(baseaddr)      = ((value) & 0xFF); \
} while (0)

#define TOSD_ENTRY_PER_CMD (16)
#define WRITE_TOSD_DEGAMMA_DATA_FASTMODE(baseaddr, index, value) do { \
	*(baseaddr + index*2 + 1) = ((value>>8) & 0x0F); \
	*(baseaddr + index*2) = (value & 0xFF); \
} while (0)

#define FILMG_MAX_CMD_LEN (1287) //256 + 803 + 228
#define FILMG_SRC_LEN (13358) // 256x3 + 803x10 + 228x20
#define FILMG_SF_CMD (3)
#define FILMG_SF_LEN (256)
#define WRITE_FILMG_SF_DATA_FASTMODE(baseaddr, y, cb, cr) do { \
	*(baseaddr + 2) = ((cr) & 0xFF); \
	*(baseaddr + 1) = ((cb) & 0xFF); \
	*(baseaddr) = ((y) & 0xFF); \
} while (0)

#define FILMG_YN_CMD (10)
#define FILMG_YN_LEN (803)
#define WRITE_FILMG_YN_DATA_FASTMODE(baseaddr, y) \
	(*(baseaddr) = ((y) & 0xFF))

#define FILMG_CBCRN_CMD (20)
#define FILMG_CBCRN_LEN (228)
#define WRITE_FILMG_CBCRN_DATA_FASTMODE(baseaddr, y) \
	(*(baseaddr) = ((y) & 0xFF))



extern uintptr_t adl_reg_base[ADL_MAX];


int disp_adl_cfg_client_en(uint8_t client, bool en, uint8_t en_mode);
int disp_config_adl_table(struct adl_src_tbl *adl_tbl);
int disp_filmg_config_adl_table(struct adl_src_tbl *adl_tbl);

int adl_cfg_dv_v_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf);
int adl_cfg_dv_g_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf);
int adl_cfg_scrmble_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf);
int adl_cfg_thdr_v_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf);
int adl_cfg_thdr_g_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf);
int adl_cfg_fgrain_adl_table(struct adl_src_tbl *adl_tbl,
	struct adl_dst_buf *dst_buf);
int disp_adl_cfg_client(uint8_t client,
	dma_addr_t dst_mva, uint32_t max_len);

int disp_adl_init(struct disp_hw_common_info *info);
int disp_adl_deinit(void);
int disp_adl_suspend(void);
int disp_adl_resume(void);
int disp_adl_irq_handler(uint32_t irq);
int disp_adl_process_cmd(enum DISP_CMD cmd,
	 void *data);
int disp_adl_clock_on_off(uint8_t id, bool en);
int disp_adl_client_status(void);
int disp_adl_get_clt_buf(enum ADL_CLIENT clit,
	void **va, dma_addr_t *pa);
int disp_adl_set_client_mode(uint8_t client,
	uint8_t clt_update_mode,
	uint8_t reg_update_mode);
int disp_adl_client_flush(void);
int disp_adl_isr(void);
int disp_adl_main(void);
void disp_adl_thread_wakeup(uint8_t path);
extern void disp_hdr_get_cur_path(uint8_t *path);
int disp_adl_update_uhd_lut(bool en, uint32_t *addr);

#endif
