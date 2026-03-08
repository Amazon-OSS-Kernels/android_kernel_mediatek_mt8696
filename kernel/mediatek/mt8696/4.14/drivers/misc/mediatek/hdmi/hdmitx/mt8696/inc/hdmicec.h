/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __hdmicec_h__
#define __hdmicec_h__
#include "hdmitx.h"
//#include <linux/wakelock.h>

enum HDMI_CEC_RX_MODE {
	CEC_NORMAL_MODE = 0,
	CEC_CTS_MODE,
	CEC_FAC_MODE,
	CEC_SLT_MODE,
	CEC_KER_HANDLE_MODE
};

enum CEC_TX_FAIL {
	FAIL_NONE = 0x00,
	FAIL_DIR = 0x01,
	FAIL_MODE = 0x02,
	FAIL_ID = 0x04,
	FAIL_SOURCE = 0x08,
	FAIL_HEADER = 0x10,
	FAIL_CMD = 0x20,
	FAIL_DATA = 0x40,
	FAIL_MAX = 0x80
};

enum CEC_SW_STATE {
	STATE_WAIT_TX_DATA_TAKEN = 0x0001,
	STATE_TX_NOACK = 0x0002,	/* CYJ.NOTE */
	STATE_TXING_FRAME = 0x0004,
	STATE_TX_FRAME_SUCCESS = 0x0008,
	STATE_HW_RETX = 0x0010,

	STATE_WAIT_RX_FRAME_COMPLETE = 0x0100,
	STATE_RX_COMPLETE_NEW_FRAME = 0x0200,
	STATE_HW_RX_OVERFLOW = 0x0400,
	STATE_RX_GET_NEW_HEADER = 0x0800,

	STATE_WAIT_TX_CHECK_RESULT = 0x1000,

	STATE_TXFAIL_DNAK = 0x10000,
	STATE_TXFAIL_HNAK = 0x20000,
	STATE_TXFAIL_RETR = 0x40000,
	STATE_TXFAIL_DATA = 0x80000,
	STATE_TXFAIL_HEAD = 0x100000,
	STATE_TXFAIL_SRC = 0x200000,
	STATE_TXFAIL_LOW = 0x400000
};

enum CEC_ERR_STATUS {
	ERR_TX_BUFFER_LOW = 0x0001,
	ERR_TX_UNDERRUN = 0x0002,
	ERR_TX_MISALARM = 0x0004,
	ERR_RX_LOST_EOM = 0x0100,
	ERR_RXQ_OVERFLOW = 0x0200,
	ERR_RX_LOST_HEADER = 0x0400
};

enum CEC_OPCODE {
	OPCODE_FEATURE_ABORT = 0x00,	/* 4 */
	OPCODE_IMAGE_VIEW_ON = 0x04,	/* 2 */
	OPCODE_TUNER_STEP_INCREMENT = 0x05,	/* 2 */
	OPCODE_TUNER_STEP_DECREMENT = 0x06,	/* 2 */
	OPCODE_TUNER_DEVICE_STATUS = 0x07,	/* 7 or 10 */
	OPCODE_GIVE_TUNER_DEVICE_STATUS = 0x08,	/* 3 */
	OPCODE_RECORD_ON = 0x09,	/* 3~10 */
	OPCODE_RECORD_STATUS = 0x0A,	/* 3 */
	OPCODE_RECORD_OFF = 0x0B,	/* 2 */
	OPCODE_TEXT_VIEW_ON = 0x0D,	/* 2 */
	OPCODE_RECORD_TV_SCREEN = 0x0F,	/* 2 */
	OPCODE_GIVE_DECK_STATUS = 0x1A,	/* 3 */
	OPCODE_DECK_STATUS = 0x1B,	/* 3 */
	OPCODE_SET_MENU_LANGUAGE = 0x32,	/* 5 */
	OPCODE_CLEAR_ANALOGUE_TIMER = 0x33,	/* 13 */
	OPCODE_SET_ANALOGUE_TIMER = 0x34,	/* 13 */
	OPCODE_TIMER_STATUS = 0x35,	/* 3 or 5 */
	OPCODE_STANDBY = 0x36,	/* 2 */
	OPCODE_PLAY = 0x41,	/* 3 */
	OPCODE_DECK_CONTROL = 0x42,	/* 3 */
	OPCODE_TIMER_CLEARED_STATUS = 0x43,	/* 3 */
	OPCODE_USER_CONTROL_PRESSED = 0x44,	/* 3 */
	OPCODE_USER_CONTROL_RELEASED = 0x45,	/* 2 */
	OPCODE_GIVE_OSD_NAME = 0x46,	/* 2 */
	OPCODE_SET_OSD_NAME = 0x47,	/* 3~16 */
	OPCODE_SET_OSD_STRING = 0x64,	/* 4~16 */
	OPCODE_SET_TIMER_PROGRAM_TITLE = 0x67,	/* 3~16 */
	OPCODE_SYSTEM_AUDIO_MODE_REQUEST = 0x70,	/* 4 */
	OPCODE_GIVE_AUDIO_STATUS = 0x71,	/* 2 */
	OPCODE_SET_SYSTEM_AUDIO_MODE = 0x72,	/* 3 */
	OPCODE_REPORT_AUDIO_STATUS = 0x7A,	/* 3 */
	OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS = 0x7D,	/* 2 */
	OPCODE_SYSTEM_AUDIO_MODE_STATUS = 0x7E,	/* 3 */
	OPCODE_ROUTING_CHANGE = 0x80,	/* 6 */
	OPCODE_ROUTING_INFORMATION = 0x81,	/* 4 */
	OPCODE_ACTIVE_SOURCE = 0x82,	/* 4 */
	OPCODE_GIVE_PHYSICAL_ADDRESS = 0x83,	/* 2 */
	OPCODE_REPORT_PHYSICAL_ADDRESS = 0x84,	/* 5 */
	OPCODE_REQUEST_ACTIVE_SOURCE = 0x85,	/* 2 */
	OPCODE_SET_STREAM_PATH = 0x86,	/* 4 */
	OPCODE_DEVICE_VENDOR_ID = 0x87,	/* 5 */
	OPCODE_VENDOR_COMMAND = 0x89,	/* <= 16 */
	OPCODE_VENDOR_REMOTE_BUTTON_DOWN = 0x8A,	/* <= 16 */
	OPCODE_VENDOR_REMOTE_BUTTON_UP = 0x8B,	/* 2 */
	OPCODE_GIVE_DEVICE_VENDOR_ID = 0x8C,	/* 2 */
	OPCODE_MENU_REQUEST = 0x8D,	/* 3 */
	OPCODE_MENU_STATUS = 0x8E,	/* 3 */
	OPCODE_GIVE_DEVICE_POWER_STATUS = 0x8F,	/* 2 */
	OPCODE_REPORT_POWER_STATUS = 0x90,	/* 3 */
	OPCODE_GET_MENU_LANGUAGE = 0x91,	/* 2 */
	OPCODE_SELECT_ANALOGUE_SERVICE = 0x92,	/* 6 */
	OPCODE_SELECT_DIGITAL_SERVICE = 0x93,	/* 9 */
	OPCODE_SET_DIGITAL_TIMER = 0x97,	/* 16 */
	OPCODE_CLEAR_DIGITAL_TIMER = 0x99,	/* 16 */
	OPCODE_SET_AUDIO_RATE = 0x9A,	/* 3 */
	OPCODE_INACTIVE_SOURCE = 0x9D,	/* 4 */
	OPCODE_CEC_VERSION = 0x9E,	/* 3 */
	OPCODE_GET_CEC_VERSION = 0x9F,	/* 2 */
	OPCODE_VENDOR_COMMAND_WITH_ID = 0xA0,	/* <= 16 */
	OPCODE_CLEAR_EXTERNAL_TIMER = 0xA1,	/* 10 ~ 11 */
	OPCODE_SET_EXTERNAL_TIMER = 0xA2,	/* 10 ~ 11 */
	OPCODE_REQUEST_CURRENT_LATENCY = 0xA7,	/* 4 */
	OPCODE_GET_CURRENT_LATENCY = 0xA8,	/* 6-7 */
	OPCODE_ABORT = 0xFF,	/* 2 */
	OPCODE_NONE = 0xFFF
};

#define RX_Q_SIZE 32
#define TX_Q_SIZE 32
#define RETX_MAX_CNT 1
#define CEC_MAX_MESG_SIZE 16

/***** New CEC IP HW START*******/
#define CEC2_TR_CONFIG		0x00
#define CEC2_RX_CHK_DST			BIT(29)
#define CEC2_BYPASS				BIT(28)
#define CEC2_DEVICE_ADDR3			(0xf << 16)
#define CEC2_LA3_SHIFT			16
#define CEC2_DEVICE_ADDR2			(0xf << 20)
#define CEC2_LA2_SHIFT			20
#define CEC2_DEVICE_ADDR1			(0xf << 24)
#define CEC2_LA1_SHIFT			24
#define CEC2_RX_ACK_ERROR_BYPASS		BIT(13)
#define CEC2_RX_ACK_ERROR_HANDLE		BIT(12)
#define CEC2_RX_DATA_ACK			BIT(11)
#define CEC2_RX_HEADER_ACK			BIT(10)
#define CEC2_RX_HEADER_TRIG_INT_EN		BIT(9)
#define CEC2_RX_ACK_SEL			BIT(8)
#define CEC2_TX_RESET_WRITE				BIT(1)
#define CEC2_TX_RESET_READ				BIT(0)
#define CEC2_RX_RESET_WRITE				BIT(0)
#define CEC2_RX_RESET_READ				BIT(1)

#define CEC2_CKGEN		0x04
#define CEC2_CLK_TX_EN			BIT(20)
#define CEC2_CLK_32K_EN			BIT(19)
#define CEC2_CLK_27M_EN			BIT(18)
#define CEC2_CLK_SEL_DIV			BIT(17)
#define CEC2_CLK_PDN				BIT(16)
#define CEC2_CLK_DIV				(0xffff << 0)
#define CEC2_DIV_SEL_100K		0x82

#define CEC2_RX_TIMER_START_R	0x08
#define CEC2_RX_TIMER_START_R_MAX		(0x7ff << 16)
#define CEC2_RX_TIMER_START_R_MIN		(0x7ff)

#define CEC2_RX_TIMER_START_F	0x0c
#define CEC2_RX_TIMER_START_F_MAX		(0x7ff << 16)
#define CEC2_RX_TIMER_START_F_MIN		(0x7ff)

#define CEC2_RX_TIMER_DATA		0x10
#define CEC2_RX_TIMER_DATA_F_MAX		(0x7ff << 16)
#define CEC2_RX_TIMER_DATA_F_MIN		(0x7ff)

#define CEC2_RX_TIMER_ACK		0x14
#define CEC2_RX_TIMER_DATA_SAMPLE		(0x7ff << 16)
#define CEC2_RX_TIMER_ACK_R			0x7ff

#define CEC2_RX_TIMER_ERROR		0x18
#define CEC2_RX_TIMER_ERROR_D		(0x7ff << 16)

#define CEC2_TX_TIMER_START		0x1c
#define CEC2_TX_TIMER_START_F		(0x7ff << 16)
#define CEC2_TX_TIMER_START_F_SHIFT	16
#define CEC2_TX_TIMER_START_R		(0x7ff)

#define CEC2_TX_TIMER_DATA_R		0x20
#define CEC2_TX_TIMER_BIT1_R			(0x7ff << 16)
#define CEC2_TX_TIMER_BIT1_R_SHIFT	16
#define CEC2_TX_TIMER_BIT0_R			(0x7ff)

#define CEC2_TX_T_DATA_F			0x24
#define CEC2_TX_TIMER_DATA_BIT		(0x7ff << 16)
#define CEC2_TX_TIMER_DATA_BIT_SHIFT	16
#define CEC2_TX_TIMER_DATA_F			(0x7ff)

#define TX_TIMER_DATA_S		0x28
#define CEC2_TX_COMP_CNT					(0x7ff << 16)
#define CEC2_TX_TIMER_DATA_SAMPLE		(0x7ff)

#define CEC2_TX_ARB		0x2c
#define CEC2_TX_MAX_RETRANSMIT_NUM_ARB		GENMASK(29, 24)
#define CEC2_TX_MAX_RETRANSMIT_NUM_COL		GENMASK(23, 20)
#define CEC2_TX_MAX_RETRANSMIT_NUM_NAK		GENMASK(19, 16)
#define CEC2_TX_BCNT_RETRANSMIT				GENMASK(11, 8)
#define CEC2_TX_BCNT_NEW_MSG				GENMASK(7, 4)
#define CEC2_TX_BCNT_NEW_INIT				GENMASK(3, 0)

#define CEC2_TX_HEADER		0x30
#define CEC2_TX_READY			BIT(16)
#define CEC2_TX_SEND_CNT			(0xf << 12)
#define CEC2_TX_HEADER_EOM			BIT(8)
#define CEC2_TX_HEADER_DST			(0x0f)
#define CEC2_TX_HEADER_SRC			(0xf0)

#define CEC2_TX_DATA0		0x34
#define CEC2_TX_DATA_B3			(0xff << 24)
#define CEC2_TX_DATA_B2			(0xff << 16)
#define CEC2_TX_DATA_B1			(0xff << 8)
#define CEC2_TX_DATA_B0			(0xff)

#define CEC2_TX_DATA1		0x38
#define CEC2_TX_DATA_B7			(0xff << 24)
#define CEC2_TX_DATA_B6			(0xff << 16)
#define CEC2_TX_DATA_B5			(0xff << 8)
#define CEC2_TX_DATA_B4			(0xff)

#define CEC2_TX_DATA2		0x3c
#define CEC2_TX_DATA_B11			(0xff << 24)
#define CEC2_TX_DATA_B10			(0xff << 16)
#define CEC2_TX_DATA_B9			(0xff << 8)
#define CEC2_TX_DATA_B8			(0xff)

#define CEC2_TX_DATA3		0x40
#define CEC2_TX_DATA_B15			(0xff << 24)
#define CEC2_TX_DATA_B14			(0xff << 16)
#define CEC2_TX_DATA_B13			(0xff << 8)
#define CEC2_TX_DATA_B12			(0xff)

#define CEC2_TX_TIMER_ACK		0x44
#define CEC2_TX_TIMER_ACK_MAX		GENMASK(26, 16)
#define CEC2_TX_TIMER_ACK_MIN		GENMASK(15, 0)

#define CEC2_LINE_DET		0x48
#define CEC2_TIMER_LOW_LONG			GENMASK(31, 16)
#define CEC2_TIMER_HIGH_LONG		GENMASK(15, 0)

#define CEC2_RX_BUF_HEADER		0x4c
#define CEC2_RX_BUF_RISC_ACK			BIT(16)
#define CEC2_RX_BUF_CNT			(0xf << 12)
#define CEC2_RX_HEADER_EOM			BIT(8)
#define	CEC2_RX_HEADER_SRC			(0xf << 4)
#define CEC2_RX_HEADER_DST			(0xf)

#define CEC2_RX_DATA0		0x50
#define CEC2_RX_DATA_B3			(0xff << 24)
#define CEC2_RX_DATA_B2			(0xff << 16)
#define CEC2_RX_DATA_B1			(0xff << 8)
#define CEC2_RX_DATA_B0			(0xff)

#define CEC2_RX_DATA1		0x54
#define CEC2_RX_DATA_B7			(0xff << 24)
#define CEC2_RX_DATA_B6			(0xff << 16)
#define CEC2_RX_DATA_B5			(0xff << 8)
#define CEC2_RX_DATA_B4			(0xff)

#define CEC2_RX_DATA2		0x58
#define CEC2_RX_DATA_B11			(0xff << 24)
#define CEC2_RX_DATA_B10			(0xff << 16)
#define CEC2_RX_DATA_B9			(0xff << 8)
#define CEC2_RX_DATA_B8			(0xff)

#define CEC2_RX_DATA3		0x5c
#define CEC2_RX_DATA_B15			(0xff << 24)
#define CEC2_RX_DATA_B14			(0xff << 16)
#define CEC2_RX_DATA_B13			(0xff << 8)
#define CEC2_RX_DATA_B12			(0xff)

#define CEC2_RX_STATUS		0x60
#define CEC2_CEC_INPUT			BIT(31)
#define CEC2_RX_FSM				GENMASK(22, 16)
#define CEC2_RX_BIT_COUNTER		(0xf << 12)
#define CEC2_RX_TIMER			(0x7ff)

#define CEC2_TX_STATUS		0x64
#define CEC2_TX_NUM_RETRANSMIT	GENMASK(31, 26)
#define CEC2_TX_FSM				GENMASK(24, 16)
#define CEC2_TX_BIT_COUNTER		(0xf << 12)
#define CEC2_TX_TIMER			(0x7ff)

#define CEC2_BACK		0x70
#define CEC2_RGS_BACK			GENMASK(31, 21)
#define CEC2_TX_LAST_ERR_STA		GENMASK(24, 16)
#define CEC2_REG_BACK			GENMASK(15, 0)

#define CEC2_INT_CLR			0x74
#define CEC2_INT_CLR_ALL			GENMASK(23, 0)
#define CEC2_RX_INT_ALL_CLR			GENMASK(23, 16)
#define CEC2_TX_INT_FAIL_CLR		(GENMASK(14, 9) | BIT(7))
#define CEC2_TX_INT_ALL_CLR			(CEC2_TX_INT_FAIL_CLR | \
	BIT(15) | BIT(8))
#define CEC2_RX_FSM_CHG_INT_CLR		BIT(23)
#define CEC2_RX_ACK_FAIL_INT_CLR		BIT(22)
#define CEC2_RX_ERROR_HANDLING_INT_CLR	BIT(21)
#define CEC2_RX_BUF_FULL_INT_CLR		BIT(20)
#define CEC2_RX_STAGE_READY_INT_CLR		BIT(19)
#define CEC2_RX_DATA_RCVD_INT_CLR		BIT(18)
#define CEC2_RX_HEADER_RCVD_INT_CLR		BIT(17)
#define CEC2_TX_FSM_CHG_INT_CLR		BIT(15)
#define CEC2_TX_FAIL_DATA_ACK_INT_CLR	BIT(14)
#define CEC2_TX_FAIL_HEADER_ACK_INT_CLR	BIT(13)
#define CEC2_TX_FAIL_RETRANSMIT_INT_CLR	BIT(12)
#define CEC2_TX_FAIL_DATA_INT_CLR		BIT(11)
#define CEC2_TX_FAIL_HEADER_INT_CLR		BIT(10)
#define CEC2_TX_FAIL_SRC_INT_CLR		BIT(9)
#define CEC2_TX_DATA_FINISH_INT_CLR		BIT(8)
#define CEC2_LINE_lOW_LONG_INT_CLR		BIT(7)
#define CEC2_LINE_HIGH_LONG_INT_CLR		BIT(6)
#define CEC2_PORD_FAIL_INT_CLR		BIT(5)
#define CEC2_PORD_RISE_INT_CLR		BIT(4)
#define CEC2_HTPLG_FAIL_INT_CLR		BIT(3)
#define CEC2_HTPLG_RISE_INT_CLR		BIT(2)
#define CEC2_FAIL_INT_CLR		BIT(1)
#define CEC2_RISE_INT_CLR		BIT(0)

#define CEC2_INT_EN			0x78
#define CEC2_INT_ALL_EN		GENMASK(23, 0)
#define CEC2_RX_INT_ALL_EN		GENMASK(23, 16)
#define CEC2_TX_INT_FAIL_EN		(GENMASK(14, 9) | BIT(7))
#define CEC2_TX_INT_ALL_EN		(CEC2_TX_INT_FAIL_EN | BIT(15) | BIT(8))
#define CEC2_RX_FSM_CHG_INT_EN		BIT(23)
#define CEC2_RX_ACK_FAIL_INT_EN		BIT(22)
#define CEC2_RX_ERROR_HANDLING_INT_EN	BIT(21)
#define CEC2_RX_BUF_FULL_INT_EN		BIT(20)
#define CEC2_RX_STAGE_READY_INT_EN		BIT(19)
#define CEC2_RX_DATA_RCVD_INT_EN		BIT(18)
#define CEC2_RX_HEADER_RCVD_INT_EN		BIT(17)
#define CEC2_RX_BUF_READY_INT_EN		BIT(16)
#define CEC2_TX_FSM_CHG_INT_EN		BIT(15)
#define CEC2_TX_FAIL_DATA_ACK_INT_EN		BIT(14)
#define CEC2_TX_FAIL_HEADER_ACK_INT_EN	BIT(13)
#define CEC2_TX_FAIL_RETRANSMIT_INT_EN	BIT(12)
#define CEC2_TX_FAIL_DATA_INT_EN		BIT(11)
#define CEC2_TX_FAIL_HEADER_INT_EN		BIT(10)
#define CEC2_TX_FAIL_SRC_INT_EN		BIT(9)
#define CEC2_TX_DATA_FINISH_INT_EN		BIT(8)
#define CEC2_LINE_lOW_LONG_INT_EN		BIT(7)
#define CEC2_LINE_HIGH_LONG_INT_EN		BIT(6)
#define CEC2_PORD_FAIL_INT_EN		BIT(5)
#define CEC2_PORD_RISE_INT_EN		BIT(4)
#define CEC2_HTPLG_FAIL_INT_EN		BIT(3)
#define CEC2_HTPLG_RISE_INT_EN		BIT(2)
#define CEC2_FALL_INT_EN			BIT(1)
#define CEC2_RISE_INT_EN			BIT(0)

#define CEC2_INT_STA			0x7c
#define CEC2_TRX_INT_STA		GENMASK(23, 0)
#define CEC2_TX_INT_FAIL		(GENMASK(14, 9) | BIT(7))
#define CEC2_RX_FSM_CHG_INT_STA		BIT(23)
#define CEC2_RX_ACK_FAIL_INT_STA		BIT(22)
#define CEC2_RX_ERROR_HANDLING_INT_STA	BIT(21)
#define CEC2_RX_BUF_FULL_INT_STA		BIT(20)
#define CEC2_RX_STAGE_READY_INT_STA		BIT(19)
#define CEC2_RX_DATA_RCVD_INT_STA		BIT(18)
#define CEC2_RX_HEADER_RCVD_INT_STA		BIT(17)
#define CEC2_RX_BUF_READY_INT_STA		BIT(16)
#define CEC2_TX_FSM_CHG_INT_STA		BIT(15)
#define CEC2_TX_FAIL_DATA_ACK_INT_STA	BIT(14)
#define CEC2_TX_FAIL_HEADER_ACK_INT_STA	BIT(13)
#define CEC2_TX_FAIL_RETRANSMIT_INT_STA	BIT(12)
#define CEC2_TX_FAIL_DATA_INT_STA		BIT(11)
#define CEC2_TX_FAIL_HEADER_INT_STA		BIT(10)
#define CEC2_TX_FAIL_SRC_INT_STA		BIT(9)
#define CEC2_TX_DATA_FINISH_INT_STA		BIT(8)
#define CEC2_LINE_lOW_LONG_INT_STA		BIT(7)
#define CEC2_LINE_HIGH_LONG_INT_STA		BIT(6)
#define CEC2_PORD_FAIL_INT_STA		BIT(5)
#define CEC2_PORD_RISE_INT_STA		BIT(4)
#define CEC2_HTPLG_FAIL_INT_STA		BIT(3)
#define CEC2_HTPLG_RISE_INT_STA		BIT(2)
#define CEC2_FAlL_INT_STA		BIT(1)
#define CEC2_RISE_INT_STA		BIT(0)
/***** New CEC IP HW END*******/

struct CEC_LA_ADDRESS {
	unsigned char ui1_num;
	unsigned char aui1_la[3];
	unsigned short ui2_pa;
};

struct cec_saved_parameters {
	unsigned char version;
	unsigned short *pa;
	unsigned char device_type;
	unsigned char vendor_id[3];
	unsigned char osd_name[14];
	unsigned char osd_namelen;
};

struct cec_kernel_handle {
	struct work_struct cec_work;
	wait_queue_head_t waitq_tx;
	struct APK_CEC_ACK_INFO tx_result;
	struct CEC_FRAME_DESCRIPTION_IO txing_frame;
	struct CEC_FRAME_DESCRIPTION_IO rx_frame;
	struct cec_saved_parameters saved_arg;
	char max_retry;
};

struct mtk_cec {
	struct platform_device *pdev;
	void __iomem *cec_base;
	//struct wake_lock wakelock;
	wait_queue_head_t waitq_tx;
	struct CEC_SEND_MSG txmsg_usr;
	struct CEC_FRAME_DESCRIPTION_IO txmsg_ker;
	struct cec_kernel_handle kh;
	struct clk *clock;
	int irq;

	struct mtk_hdmi *hdmi;

	bool (*cec_readbit)(struct mtk_cec *cec,
		unsigned short reg, unsigned int offset);
	unsigned int (*cec_read)(struct mtk_cec *cec, unsigned short reg);
	void (*cec_write)(struct mtk_cec *cec,
		unsigned short reg, unsigned int val);
	void (*cec_mask)(struct mtk_cec *cec,
		unsigned int reg, unsigned int val, unsigned int mask);
	void (*common_mask)(struct mtk_cec *cec,
		void __iomem *addr, unsigned int val, unsigned int mask);
};

enum CEC_DRV_LOG_ADDR_T {
	CEC_LOG_ADDR_TV = 0,
	CEC_LOG_ADDR_REC_DEV_1,
	CEC_LOG_ADDR_REC_DEV_2,
	CEC_LOG_ADDR_TUNER_1,
	CEC_LOG_ADDR_PLAYBACK_DEV_1,
	CEC_LOG_ADDR_AUD_SYS,
	CEC_LOG_ADDR_TUNER_2,
	CEC_LOG_ADDR_TUNER_3,
	CEC_LOG_ADDR_PLAYBACK_DEV_2,
	CEC_LOG_ADDR_REC_DEV_3,
	CEC_LOG_ADDR_TUNER_4,
	CEC_LOG_ADDR_PLAYBACK_DEV_3,
	CEC_LOG_ADDR_RESERVED_1,
	CEC_LOG_ADDR_RESERVED_2,
	CEC_LOG_ADDR_FREE_USE,
	CEC_LOG_ADDR_UNREGED_BRDCST,
	CEC_LOG_ADDR_MAX
};

enum CEC_CMD_STATE {
	GET_CMD_ERR = 0x11,
	GET_CMD_EMPTY = 0x33,
	GET_CMD_UNEMPTY = 0x55
};


enum HDMI_NFY_CEC_STATE_T {
	HDMI_CEC_PLUG_OUT = 0,
	HDMI_CEC_TX_STATUS,
	HDMI_CEC_GET_CMD,
};

extern unsigned char hdmi_cec_on;
extern void hdmi_cec_init(struct mtk_cec *cec);
extern unsigned char hdmi_cec_isrprocess(
	struct mtk_cec *cec, unsigned char u1rxmode);
extern	void hdmi_cec_mainloop(unsigned char u1rxmode);
extern void hdmi_CECMWSetLA(struct CEC_DRV_ADDR_CFG *prAddr);
extern void hdmi_u4CecSendSLTData(unsigned char *pu1Data);
extern void hdmi_CECMWGet(struct CEC_FRAME_DESCRIPTION_IO *frame);
extern void hdmi_GetSLTData(struct CEC_SLT_DATA *rCecSltData);
extern void hdmi_CECMWSend(struct CEC_SEND_MSG *msg);
extern void hdmi_CECMWSetEnableCEC(unsigned char u1EnCec);
extern void hdmi_CECMWSetEnableARC(unsigned char u1EnArc);
extern void cec_enable_arc(unsigned char u1EnArc);
extern void ARCIN_Enable(bool enable);
extern void hdmi_NotifyApiCECAddress(struct CEC_ADDRESS_IO *cecaddr);
extern void vNotifyAppHdmiCecState(enum HDMI_NFY_CEC_STATE_T u1hdmicecstate);
extern void hdmi_SetPhysicCECAddress(unsigned short u2pa, unsigned char u1la);
extern void hdmi_cec_api_get_txsts(struct APK_CEC_ACK_INFO *pt);
extern void hdmi_cec_api_get_cmd(struct CEC_FRAME_DESCRIPTION_IO *frame);
extern void hdmi_cec_usr_cmd(unsigned int cmd, unsigned int *result);
extern void hdmi_cec_power_on(struct mtk_cec *cec, bool pwr);
extern unsigned int hdmi_cec_read(unsigned short u2Reg);
extern void cec_timer_sleep(void);
extern void cec_timer_wakeup(void);
extern unsigned char cec_clock;
extern bool is_cec_irq(void);
extern void enable_cec_ip_irq(bool enable);
extern void vClear_cec_irq(void);
extern int report_virtual_hdmikey(void);
extern unsigned char hdmi_rxcecmode;
extern bool hdmi_cec_factory_test(void);
void CEC_KHandle_TXNotify(enum HDMI_NFY_CEC_STATE_T u1hdmicecstate);
void hdmi_CECSendNotify(enum HDMI_NFY_CEC_STATE_T u1hdmicecstate);
int hdmi_cec_status_dump(char *str);
int hdmi_cec_probe(struct platform_device *pdev);
#endif
