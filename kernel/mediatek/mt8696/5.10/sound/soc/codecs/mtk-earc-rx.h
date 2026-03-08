/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8532_earc.h  --  Mediatek 8532 earc driver
 *
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __EARC_RX_H__
#define __EARC_RX_H__



#define EARC_EVENT_FLAGS__ASSERT		(1 << 7)

/* Channel status size in bytes */
#define EARC_CHANNEL_STATUS_SIZE	5

/* Offsets for frame rate to sampling frequency conversion -  */
#define	EARC_LAYOUT_2CH_SHIFT_OFFSET			0
#define	EARC_LAYOUT_8CH_SHIFT_OFFSET			2
#define	EARC_LAYOUT_16CH_SHIFT_OFFSET			3
#define	EARC_LAYOUT_32CH_SHIFT_OFFSET			4

#define	EARC_LAYOUT_A_SHIFT_OFFSET			0
#define	EARC_LAYOUT_B_SHIFT_OFFSET			2

/* Channel status masks - Bits 0,1,3,4,5 */
#define	EARC_CHANSTATUS_PATTERN__2CH_LPCM		0x00
#define	EARC_CHANSTATUS_PATTERN__MULTI_LPCM		0x20
#define	EARC_CHANSTATUS_PATTERN__NON_PCM			0x02

#define	EARC_CHANSTATUS_PATTERN__MASK			0x3B

/* Channel status masks - Bits 0,1,3,4,5 */
#define	EARC_CHANSTATUS_BITDEPTH__MAX_20BIT		0x00
#define	EARC_CHANSTATUS_BITDEPTH__16BIT			0x01
#define	EARC_CHANSTATUS_BITDEPTH__17BIT			0x06
#define	EARC_CHANSTATUS_BITDEPTH__18BIT			0x02
#define	EARC_CHANSTATUS_BITDEPTH__19BIT			0x04
#define	EARC_CHANSTATUS_BITDEPTH__20BIT_AT20		0x05

#define	EARC_CHANSTATUS_BITDEPTH__MAX_24BIT		0x01
#define	EARC_CHANSTATUS_BITDEPTH__20BIT_AT24		0x01
#define	EARC_CHANSTATUS_BITDEPTH__21BIT			0x06
#define	EARC_CHANSTATUS_BITDEPTH__22BIT			0x02
#define	EARC_CHANSTATUS_BITDEPTH__23BIT			0x04
#define	EARC_CHANSTATUS_BITDEPTH__24BIT			0x05


/* Channel status layouts - Bits 44, 45, 46, 47: Byte 5 */
#define	EARC_CHANSTATUS_LAYOUT__2CH			0x00
#define	EARC_CHANSTATUS_LAYOUT__8CH			0x70
#define	EARC_CHANSTATUS_LAYOUT__16CH			0xB0
#define	EARC_CHANSTATUS_LAYOUT__32CH			0x30

/* Channel status layouts for compressed audio - Bits 44, 45, 46, 47: Byte 5 */
#define	EARC_CHANSTATUS_COMPR_LAYOUT__A			0x00
#define	EARC_CHANSTATUS_COMPR_LAYOUT__B			0x70

/* Channel layout mask for byte 5 */
#define	EARC_CHANSTATUS_LAYOUT__MASK			0xF0

/* eARC Rx Capabilities Data Structure Maximum Length */
#define EARC_CAPS_DS_MAX_LENGTH			256

/* eARC Rx Capability data structure Version */
#define EARC_CAP_DATA_STRUCT_VERSION		0x01

/* eARC Rx Capability data structure End Marker */
#define EARC_CAP_DATA_STRUCT_END_MARKER		0x00


/***************EARC ANALOG REGISTER START*****************/

#define EARC_RX_CFG_0		0x05c

#define RG_EARCRX_TERM50_EN			(0x1 << 31)
#define RG_EARCRX_TERM50_SEL		(0x1f << 26)
#define RG_EARCRX_VCM_TERM_SEL		(0x3 << 24)
#define RG_EARCRX_BIAS_EN			(0x1 << 22)
#define RG_EARCRX_BIAS_INTR_CAL		(0x1F << 17)
#define RG_EARCRX_REFGEN_MONEN		(0x1 << 16)
#define RG_EARCRX_DM_LDO09_SEL		(0x1 << 15)
#define RG_EARCRX_DM_LDO_LVROD		(0x3 << 13)
#define RG_EARCRX_DM_LDO_MONEN		(0x1 << 12)
#define RG_EARCRX_PWM_CDR_EN		(0x1 << 11)
#define RG_EARCRX_PWM_CDR_TOBDET_EN		(0x1 << 10)
#define RG_EARCRX_PWM_GEAR_SEL		(0x3 << 8)
#define RG_EARCRX_CMTX_EN			(0x1 << 6)
#define RG_EARCRX_CM_VREFT_SEL		(0x1 << 5)
#define RG_EARCRX_CM_LDO_PWD		(0x1 << 4)
#define RG_EARCRX_CM_REG_PWD		(0x1 << 3)
#define RG_EARCRX_CM_LDO_LVROD		(0x3 << 1)
#define RG_EARCRX_CM_LDO_MONEN		(0x1 << 0)

#define RG_EARCRX_TERM50_EN_SHIFT			31
#define RG_EARCRX_TERM50_SEL_SHIFT		26
#define RG_EARCRX_VCM_TERM_SEL_SHIFT		24
#define RG_EARCRX_BIAS_EN_SHIFT			22
#define RG_EARCRX_BIAS_INTR_CAL_SHIFT		17
#define RG_EARCRX_REFGEN_MONEN_SHIFT		16
#define RG_EARCRX_DM_LDO09_SEL_SHIFT		15
#define RG_EARCRX_DM_LDO_LVROD_SHIFT		13
#define RG_EARCRX_DM_LDO_MONEN_SHIFT		12
#define RG_EARCRX_PWM_CDR_EN_SHIFT		11
#define RG_EARCRX_PWM_CDR_TOBDET_EN_SHIFT		10
#define RG_EARCRX_PWM_GEAR_SEL_SHIFT		8
#define RG_EARCRX_CMTX_EN_SHIFT			6
#define RG_EARCRX_CM_VREFT_SEL_SHIFT		5
#define RG_EARCRX_CM_LDO_PWD_SHIFT		4
#define RG_EARCRX_CM_REG_PWD_SHIFT		3
#define RG_EARCRX_CM_LDO_LVROD_SHIFT		1
#define RG_EARCRX_CM_LDO_MONEN_SHIFT		0


#define EARC_RX_CFG_1		0x060

#define RG_EARCRX_CM_TRAN_SEL		(0x7 << 29)
#define RG_EARCRX_CM_DLY_SEL		(0x7 << 26)
#define RG_EARCRX_CM_SLEW_OSC_EN		(0x1 << 25)
#define RG_EARCRX_ARC_EN		(0x1 << 23)
#define RG_EARCRX_CMRX_EN		(0x1 << 22)
#define RG_EARCRX_CMRX_BUF_EN		(0x1 << 21)
#define RG_EARCRX_CMRX_HPF_BW_CTRL		(0x1 << 20)
#define RG_EARCRX_CMRX_HYST_AMP_EN		(0x1 << 19)
#define RG_EARCRX_CMRX_HYST_SEL		(0x1 << 18)
#define RG_EARCRX_CM_VREF_MONEN		(0x1 << 17)
#define RG_EARCRXPLL_EN		(0x1 << 16)
#define RG_EARCRXPLL_PREDIV		(0x3 << 14)
#define RG_EARCRXPLL_POSDIV		(0x7 << 11)
#define RG_EARCRXPLL_BLP		(0x1 << 10)
#define RG_EARCRXPLL_DIV3_EN		(0x1 << 9)
#define RG_EARCRXPLL_GLITCHFREE_EN		(0x1 << 8)
#define RG_EARCRXPLL_LVROD_EN		(0x1 << 7)
#define RG_EARCRXPLL_MONCK_EN		(0x1 << 6)
#define RG_EARCRXPLL_MONREF_EN		(0x1 << 5)
#define RG_EARCRXPLL_MONVC_EN		(0x1 << 4)
#define RG_EARCRXPLL_SDM_FRA_EN		(0x1 << 3)

#define RG_EARCRX_CM_TRAN_SEL_SHIFT		29
#define RG_EARCRX_CM_DLY_SEL_SHIFT		26
#define RG_EARCRX_CM_SLEW_OSC_EN_SHIFT		25
#define RG_EARCRX_ARC_EN_SHIFT		23
#define RG_EARCRX_CMRX_EN_SHIFT		22
#define RG_EARCRX_CMRX_BUF_EN_SHIFT		21
#define RG_EARCRX_CMRX_HPF_BW_CTRL_SHIFT		20
#define RG_EARCRX_CMRX_HYST_AMP_EN_SHIFT		19
#define RG_EARCRX_CMRX_HYST_SEL_SHIFT		18
#define RG_EARCRX_CM_VREF_MONEN_SHIFT		17
#define RG_EARCRXPLL_EN_SHIFT		16
#define RG_EARCRXPLL_PREDIV_SHIFT		14
#define RG_EARCRXPLL_POSDIV_SHIFT		11
#define RG_EARCRXPLL_BLP_SHIFT		10
#define RG_EARCRXPLL_DIV3_EN_SHIFT		9
#define RG_EARCRXPLL_GLITCHFREE_EN_SHIFT		8
#define RG_EARCRXPLL_LVROD_EN_SHIFT		7
#define RG_EARCRXPLL_MONCK_EN_SHIFT		6
#define RG_EARCRXPLL_MONREF_EN_SHIFT		5
#define RG_EARCRXPLL_MONVC_EN_SHIFT		4
#define RG_EARCRXPLL_SDM_FRA_EN_SHIFT		3


#define EARC_RX_CFG_2		0x064

#define RG_FORCE_CALIBRATEION		(0x1 << 23)
#define RG_EARCRX_REV_VALUE		(0x1F << 18)
#define RG_EARCRX_REV		(0xFF << 16)
#define rg_hdmi2_moni_sel		(0x1 << 13)
#define RG_EARCRXPLL_TSTDIV		(0x3 << 11)
#define RG_EARCRX_VDCTST_SEL		(0x7 << 8)
#define rg_sw_hpd_on		(0x1 << 7)
#define rg_hdmirx_pcw_sel		(0x1 << 6)
#define DA_EARCRXPLL_SDM_PWR_ON		(0x1 << 3)
#define DA_EARCRXPLL_SDM_ISO_EN		(0x1 << 2)
#define DA_EARCRX_ARC_EN		(0x1 << 1)
#define DA_EARCRXPLL_SDM_PCW_CHG		(0x1 << 0)

#define RG_FORCE_CALIBRATEION_SHIFT		23
#define RG_EARCRX_REV_VALUE_SHIFT		18
#define RG_EARCRX_REV_SHIFT		16
#define rg_hdmi2_moni_sel_SHIFT		13
#define RG_EARCRXPLL_TSTDIV_SHIFT		11
#define RG_EARCRX_VDCTST_SEL_SHIFT		8
#define rg_sw_hpd_on_SHIFT		7
#define rg_hdmirx_pcw_sel_SHIFT		6
#define DA_EARCRXPLL_SDM_PWR_ON_SHIFT		3
#define DA_EARCRXPLL_SDM_ISO_EN_SHIFT		2
#define DA_EARCRX_ARC_EN_SHIFT		1
#define DA_EARCRXPLL_SDM_PCW_CHG_SHIFT		0
#define EARC_RX_CFG_1_SHIFT		0x080

#define EARC_RX_CFG_3	0x80

#define DA_EARCRXPLL_SDM_PCW		(0xFFFFFFFF << 0)

#define RG_EARCRXPLL_SDM_PCW_SHIFT	0

#define EARC_RX_CFG_0C0		0xc0

#define RG_EARCRX_MUX		(0x1 << 5)
#define RG_EARCRX_MUX_SHIFT		5


/******************EARC ANALOG REGISTER END************/

/****************EARC DIGITAL REGISTER START****************/

#define COMMA_TIMER_SND_PKT_1	0x000

#define tx_timer_wait_slv		(0xFF << 24)
#define tx_timer_cm_turnover		(0xFF << 8)
#define tx_timer_cm_turnover_shift		8
#define tx_timer_eARC_tgl_cm		(0xFF << 0)
#define tx_timer_eARC_tgl_cm_shift		0

#define COMMA_TIMER_SND_PKT_2	0x004

#define tx_timer_cm_drive_stop		(0x7FF << 0)

#define COMMA_SND_SOFT	0x008

#define receive_data		(0xFF << 24)
#define receive_cd		(0x1 << 23)

#define COMMA_TIMER_RCV_PKT_1	0x010

#define tx_timer_eARC_tgl_cm_max		(0x7FF << 16)
#define tx_timer_eARC_tgl_cm_max_shift		16
#define tx_timer_eARC_tgl_cm_min		(0x7FF << 0)
#define tx_timer_eARC_tgl_cm_min_shift		0

#define COMMA_TIMER_RCV_PKT_2	0x014
#define tx_timer_eARC_bit_cm_max		(0x7FF << 16)
#define tx_timer_eARC_bit_cm_max_shift		16
#define tx_timer_eARC_bit_cm_min		(0x7FF << 0)
#define tx_timer_eARC_bit_cm_min_shift		0

#define COMMA_TIMER_DISC_1	0x018

#define rg_ms_counter		(0x3FF << 16)
#define rg_ms_counter_shift				16
#define rg_comma_count_max		(0xF << 12)
#define rg_comma_count_max_shift			12
#define rg_comma_num_min		(0xF << 8)
#define rg_comma_num_min_shift			8
#define rg_us_counter		(0xFF << 0)
#define rg_us_counter_shift				0

#define COMMA_TIMER_DISC_2	0x01c

#define rx_timer_timeout		(0x1FF << 16)
#define rg_timer_comma_on		(0xFF << 8)
#define rx_timer_eARC_lost_HB		(0xFF << 0)
#define rx_timer_eARC_lost_HB_shift		0

#define COMMA_TOP_CONTROL	0x020

#define rg_data_ready		(0x1 << 26)
#define rg_rcv_ecc_bypass		(0x1 << 25)
#define rg_rcv_ecc_bypass_shift		25
#define rg_htplg_phy_soft		(0x1 << 17)
#define rg_htplg_phy_soft_mode		(0x1 << 8)
#define rg_capa_data_unread		(0x1 << 7)
#define rg_send_rsvd		(0x1 << 4)
#define rg_cm_en		(0x1 << 3)
#define rg_cm		(0x1 << 2)
#define rg_cm_en_select		(0x1 << 1)
#define rg_cm_select		(0x1 << 0)

#define COMMA_DISC_CNTL	0x024

#define rg_eARC_valid_max		(0xF << 28)
#define rg_eARC_valid_min		(0xF << 24)
#define rg_hpd_down		(0x1 << 17)
#define rg_v_comma_byp		(0x1 << 7)
#define rg_hpd_up		(0x1 << 5)
#define rg_detect_en		(0x1 << 4)
#define rg_detect_en_shift		4
#define rg_lost_heartbeat		(0x1 << 3)
#define rg_got_heartbeat		(0x1 << 2)
#define rg_lost_heartbeat_sel		(0x1 << 1)
#define rg_got_heartbeat_sel		(0x1 << 0)

#define COMMA_FSM_CNTL	0x028

#define rg_offset_sel		(0x1 << 17)
#define rg_full_detect_HB		(0x1 << 4)
#define rg_write_not_allow		(0x1 << 3)
#define rg_write_not_allow_sel		(0x1 << 2)
#define rg_right_id		(0x1 << 0)

#define COMMA_PULSE_CNTL	0x02c

#define begin_cal				(0x1 << 2)
#define comma_pulse_cntl		(0xFFFFFFFF << 0)

#define COMMA_HEARTBEAT	0x030

#define rgs_heartbeat		(0xFF << 24)
#define rg_heartbeat_clr		(0x1 << 15)
#define rg_HB_d0_value_sel		(0x1 << 8)
#define rg_HB_d0_value_sel_shift		8
#define rg_HB_d0_value		(0xFF << 0)
#define rg_HB_d0_value_shift		0

#define COMMA_AUDIO	0x034

#define rdata_rg_config		(0xFF << 16)
#define rg_audio_config_clr		(0x1 << 15)
#define rg_audio_config_en_p		(0x1 << 8)
#define rg_audio_config		(0xFF << 0)

#define rdata_rg_config_shift		16
#define rg_audio_config_clr_shift		15
#define rg_audio_config_en_p_shift		8
#define rg_audio_config_shift		0


#define COMMA_CAPA_ADDR	0x038

#define capa_read		(0xFF << 16)
#define capa_address		(0xFF << 0)
#define capa_address_shift		0

#define COMMA_CAPA_DATA	0x03c

#define capa_data_ff		(0xFF << 0)
#define capa_data_ff_shift		0
#define capa_data		(0xFFFFFFFF << 0)

#define RGS_DEBUG_FSM_1	0x040

#define HDMI_HPD		(0x1 << 31)
#define EARC_VALID		(0x1 << 30)

#define RGS_DEBUG_FSM_2	0x044

#define rgs_HB0_value		(0xFF << 15)

#define RGS_DEBUG_FSM_3	0x048

#define RGS_DEBUG_TOP	0x04c

#define mem_wr_addr		(0xFF << 24)
#define mem_rd_addr		(0xFF << 16)

#define COMMA_RX_INT_MASK	0x050

#define int_exr_latency_req_mask		(0x1 << 26)
#define int_aud_msg_chg_mask		(0x1 << 25)
#define int_aud_fmt_chg_mask		(0x1 << 24)
#define int_read_empty_mask		(0x1 << 23)
#define int_write_full_mask		(0x1 << 22)
#define int_HDMI_HPD_mask		(0x1 << 21)
#define int_EARC_VALID_mask		(0x1 << 20)
#define int_early_hb_mask		(0x1 << 19)
#define int_err_ecc_mask		(0x1 << 18)
#define int_rscv_abort_mask		(0x1 << 17)
#define int_send_ok_mask		(0x1 << 15)
#define int_rscv_ok_mask		(0x1 << 14)
#define int_read_finish_mask		(0x1 << 13)
#define int_write_finish_mask		(0x1 << 12)
#define int_abort_mask		(0x1 << 11)
#define int_detect_HB_mask		(0x1 << 10)
#define int_detect_HB_mask_shift		10
#define int_HB_timeout_mask		(0x1 << 9)
#define int_timeout_mask		(0x1 << 8)
#define int_hpd_phy_low_mask		(0x1 << 7)
#define int_read_noack_mask		(0x1 << 6)
#define int_write_noack_mask		(0x1 << 5)
#define int_ecc_fix_mask		(0x1 << 4)
#define int_ecc_err_mask		(0x1 << 3)
#define int_comma_on_running_cmd_mask		(0x1 << 2)
#define int_hpd_fall_cap		(0x1 << 1)
#define int_hpd_raise_cap		(0x1 << 0)

#define COMMA_RX_INT_READ	0x054

#define comma_rx_int_read		(0xFFFFFFFF << 0)

#define COMMA_RX_INT_CLR	0x058

#define comma_rx_int_clr		(0xFFFFFFFF << 0)


#define int_16		(0x1 << 16)
#define int_16_shift		16
#define int_27_31		(0x1F << 27)
#define int_27_31_shift		27



#define int_exr_latency_req_clr		(0x1 << 26)
#define int_exr_latency_req_clr_shift		26
#define int_aud_msg_chg_clr		(0x1 << 25)
#define int_aud_msg_chg_clr_shift		25
#define int_aud_fmt_chg_clr		(0x1 << 24)
#define int_aud_fmt_chg_clr_shift		24
#define int_read_empty_clr		(0x1 << 23)
#define int_read_empty_clr_shift		23
#define int_write_full_clr		(0x1 << 22)
#define int_write_full_clr_shift		22
#define int_HDMI_HPD_clr		(0x1 << 21)
#define int_HDMI_HPD_clr_shift		21
#define int_EARC_VALID_clr		(0x1 << 20)
#define int_EARC_VALID_clr_shift		20
#define int_early_hb_clr		(0x1 << 19)
#define int_early_hb_clr_shift		19
#define int_err_ecc_clr		(0x1 << 18)
#define int_err_ecc_clr_shift		18
#define int_rscv_abort_clr		(0x1 << 17)
#define int_rscv_abort_clr_shift		17
#define int_send_ok_clr		(0x1 << 15)
#define int_send_ok_clr_shift		15
#define int_rscv_ok_clr		(0x1 << 14)
#define int_rscv_ok_clr_shift		14
#define int_read_finish_clr		(0x1 << 13)
#define int_read_finish_clr_shift		13
#define int_write_finish_clr		(0x1 << 12)
#define int_write_finish_clr_shift		12
#define int_abort_clr		(0x1 << 11)
#define int_abort_clr_shift		11
#define int_detect_HB_clr		(0x1 << 10)
#define int_detect_HB_clr_shift		10
#define int_HB_timeout_clr		(0x1 << 9)
#define int_HB_timeout_clr_shift		9
#define int_timeout_clr		(0x1 << 8)
#define int_timeout_clr_shift		8
#define int_hpd_phy_low_clr		(0x1 << 7)
#define int_hpd_phy_low_clr_shift		7
#define int_read_noack_clr		(0x1 << 6)
#define int_read_noack_clr_shift		6
#define int_write_noack_clr		(0x1 << 5)
#define int_write_noack_clr_shift		5
#define int_ecc_fix_clr		(0x1 << 4)
#define int_ecc_fix_clr_shift		4
#define int_ecc_err_clr		(0x1 << 3)
#define int_ecc_err_clr_shift		3
#define int_comma_on_running_cmd_clr		(0x1 << 2)
#define int_comma_on_running_cmd_clr_shift		2
#define int_hpd_fall_cap_clr		(0x1 << 1)
#define int_hpd_fall_cap_clr_shift		1
#define int_hpd_raise_cap_clr		(0x1 << 0)
#define int_hpd_raise_cap_clr_shift		0

#define COMMA_FREQ_METER	0x05c

#define freq_meter_value		(0xFFFF << 16)
#define freq_meter_value_shift		16
#define freq_clock_select		(0x3 << 14)
#define freq_clock_select_shift		14
#define dm_freq_sel		(0x1 << 11)
#define dm_freq_sel_shift		11
#define freq_meter_enable		(0x1 << 10)
#define freq_meter_enable_shift		10
#define freq_meter_cnt		(0x3FF << 0)
#define freq_meter_cnt_shift		0

#define COMMA_RX_AUD_CNTL	0x060

#define rg_channel_swap		(0x1 << 22)
#define rg_bclk_xor		(0x1 << 21)
#define rg_i2s_en		(0x1 << 20)
#define rg_spdif_en		(0x1 << 17)
#define mclk_config		(0x7 << 13)
#define mclk_config_shift		13
#define bck_config		(0x7 << 10)
#define bck_config_shift		10
#define rg_data_6ch_en		(0x1 << 9)
#define rg_data_8ch_en		(0x1 << 8)
#define rg_data_2ch_en		(0x1 << 7)
#define rg_data_268ch_en		(0x7 << 7)
#define rg_data_268ch_en_shift		7
#define rg_audio_format_soft_en		(0x1 << 6)
#define rg_audio_format_soft_en_shift		6
#define rg_mute_soft		(0x1 << 3)
#define rg_mute_soft_shift		3
#define rg_mute_soft_sel		(0x1 << 2)
#define rg_mute_soft_sel_shift		2
#define rg_bypass_ecc_correct		(0x1 << 0)

#define COMMA_RX_AUD_CNTL1	0x064

#define calibration_value		(0x1F << 26)
#define calibration_value_shift		26


#define AUDIO_RX_CHAN_LEFT_1	0x068

#define audio_rx_chan_left_1		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_LEFT_2	0x06C

#define audio_rx_chan_left_2		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_LEFT_3	0x070

#define audio_rx_chan_left_3		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_LEFT_4	0x074

#define audio_rx_chan_left_4		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_LEFT_5	0x078

#define audio_mute_bit		(0x1 << 18)
#define audio_mute_bit_shift		18
#define audio_rx_chan_left_5		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_LEFT_6	0x07C

#define audio_rx_chan_left_6		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_RIGHT_1	0x080

#define audio_rx_chan_right_1		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_RIGHT_2	0x084

#define audio_rx_chan_right_2		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_RIGHT_3	0x088

#define audio_rx_chan_right_3		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_RIGHT_4	0x08c

#define audio_rx_chan_right_4		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_RIGHT_5	0x090

#define audio_rx_chan_right_5		(0xFFFFFFFF << 0)

#define AUDIO_RX_CHAN_RIGHT_6	0x094

#define audio_rx_chan_right_6		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_1	0x098

#define audio_rx_user_1		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_2	0x09C

#define audio_rx_user_2		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_3	0x0A0

#define audio_rx_user_3		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_4	0x0A4

#define audio_rx_user_4		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_5	0x0A8

#define audio_rx_user_5		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_6	0x0AC

#define audio_rx_user_6		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_7	0x0B0

#define audio_rx_user_7		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_8	0x0B4

#define audio_rx_user_8		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_9	0x0B8

#define audio_rx_user_9		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_10	0x0BC

#define audio_rx_user_10		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_11	0x0C0

#define audioin_enable			(0x1 << 5)
#define audioin_enable_shift			5
#define audio_rx_user_11		(0xFFFFFFFF << 0)

#define AUDIO_RX_USER_12	0x0C4

#define audio_rx_user_12		(0xFFFFFFFF << 0)

#define COMMA_RX_SOFT_ECC_1	0x0C8

#define soft_ecc_1		(0xFFFFFFFF << 0)

#define COMMA_RX_SOFT_ECC_2	0x0CC

#define soft_ecc_2		(0xFFFFFFFF << 0)

#define COMMA_TOP_CONFIG_1	0x0D0

#define rg_delitch_htplg		(0xFF << 16)
#define rg_delitch_htplg_shift		16
#define rg_sample_step		(0xFF << 8)
#define rg_deglitch_cm		(0xFF << 0)
#define rg_deglitch_cm_shift		0

#define COMMA_TOP_CONFIG_2	0x0D4

#define rg_fmt_chg_detect_en		(0x1 << 31)
#define rg_fmt_chg_detect_en_shift		31
#define rg_soft_rstb_aud		(0x1 << 21)
#define rg_soft_rstb_aud_shift		21
#define rg_soft_rstb_deglitch		(0x1 << 20)
#define rg_soft_rstb_deglitch_shift		20
#define rg_soft_rstb_disc		(0x1 << 19)
#define rg_soft_rstb_disc_shift		19
#define rg_soft_rstb_fsm		(0x1 << 18)
#define rg_soft_rstb_fsm_shift		18
#define rg_soft_rstb_snd		(0x1 << 17)
#define rg_soft_rstb_snd_shift		17
#define rg_soft_rstb_rscv		(0x1 << 16)
#define rg_soft_rstb_rscv_shift		16
#define rg_soft_rstb_dishpd		(0x1 << 0)
#define rg_soft_rstb_dishpd_shift		0

#define rg_soft_rstb_all		(0xf << 16)
#define rg_soft_rstb_all_shift		16


#define COMMA_TOP_CONFIG_3	0x0D8

#define comma_top_config_3		(0xFFFFFFFF << 0)

#define COMMA_TOP_CONFIG_4	0x0DC

#define comma_top_config_4		(0xFFFFFFFF << 0)

#define COMMA_RX_RSCV_PKT	0x0E0

#define rg_bypass_rsv_err		(0x1 << 17)
#define rscv_ecc_invert		(0x1 << 1)
#define rscv_ecc_revert		(0x1 << 0)

#define COMMA_RX_SND_PKT	0x0E4

#define snd_ecc_invert		(0x1 << 1)
#define snd_ecc_revert		(0x1 << 0)

#define COMMA_MONI_CNTL	0x0E8

#define comma_moni_cntl		(0xFFFFFFFF << 0)

#define COMMA_AUDIO_READ	0x0EC

#define audio_format		(0xFF << 8)
#define exr_latency_req		(0xFF << 0)

/***********EARC DIGITAL REGISTER END*************/

/***********EARC Audio REGISTER START*************/
#define earc_audio_clock	0x010

#define rg_clock_gated_en		(0x1 << 19)
#define rg_clock_gated_en_shift		19

#define earc_audio_mphone_multi	0x6a4

#define rg_mphone_multi		(0x1 << 0)
#define rg_mphone_multi_shift		0

/***********EARC Audio REGISTER END*************/


#define UN2CHLPCM       (0x00)
#define UNMCHLPCM       (0x10)
#define UNXCHDSD        (0x18)
#define UN2CHNLPCM      (0x02)
#define EN2CHNLPCM      (0x06)
#define ENMCHNLPCM      (0x16)
#define ENXCHDSD        (0x1E)

#define LPCM2CH         (0x0)
#define LPCM8CH         (0x7)
#define LPCM16CH        (0xB)
#define LPCM32CH        (0x3)
#define NLPCM2CH        (0x0)
#define NLPCM8CH        (0x7)
#define DSD6CH          (0x5)
#define DSD12CH         (0x9)

#define PCM_2         (0x0)
#define PCM_8         (0x1)
#define PCM_16         (0x2)
#define PCM_32         (0x3)
#define CMPS_a         (0x4)
#define CMPS_b         (0x5)
#define DSD_6         (0x6)
#define DSD_12         (0x7)

#define AUD32K         (0x03)
#define AUD44K         (0x00)
#define AUD48K         (0x02)
#define AUD64K         (0x0B)
#define AUD88K         (0x08)
#define AUD96K         (0x0A)
#define AUD128K         (0x2B)
#define AUD176K         (0x0C)
#define AUD192K         (0x0E)
#define AUD256K         (0x1B)
#define AUD352K         (0x0D)
#define AUD384K         (0x05)
#define AUD512K         (0x3B)
#define AUD705K         (0x2D)
#define AUD768K         (0x09)
#define AUD1024K         (0x35)
#define AUD1411K         (0x1D)
#define AUD1536K         (0x15)

#define ADO2CHPCM			(0x00)
#define ADO8CHPCM			(0x07)
#define ADO16CHPCM			(0x0B)
#define ADO32CHPCM			(0x03)

#define ADO6CHDSD			(0x05)
#define ADO12CHDSD			(0x09)

#define CompLayoutA			(0x00)
#define CompLayoutB			(0x07)


enum EARC_REG_ENUM {
	REG_EARC_DIG,
	REG_EARC_ANA,
	REG_HDMITX_DIG,
	REG_EARC_MONITOR,
	EARC_REG_NUM,
};


typedef void (*earc_event_callback_func)(unsigned long *callback_param);

#define EARC_LOG(fmt, arg...) pr_info("[EARC]"fmt, ##arg)
#define HAL_Delay_us(delay) udelay(delay)

#define earcpolllog         (0x1)
#define earcdrvlog			(0x2)

#define earcalllog   earcdrvlog

unsigned int earc_log_on = earcalllog;

#define EARC_POLL_LOG(fmt, arg...) \
	do {	if (earc_log_on & earcpolllog)	\
		EARC_LOG("[poll] "fmt, ##arg);	\
	} while (0)

#define EARC_DRV_LOG(fmt, arg...) \
	do {	if (earc_log_on & earcdrvlog)	\
		EARC_LOG("[drv] "fmt, ##arg);	\
	} while (0)

#define EARC_DRV_FUNC()	\
	do {	if (earc_log_on & earcdrvlog) \
		EARC_LOG("[DRV] %s\n", __func__); \
	} while (0)

/* * Return value for successful API execution*/
#define EARC_RETURN_VALUE__SUCCESS		0

/* The current audio latency in milliseconds from*/
/* the eARC RX audio input to the speakers*/
struct earc_erx_latency {
	unsigned char data;
};

enum arc_mode {
	ARC_MODE__NONE,
	ARC_MODE__ARC,
	ARC_MODE__EARC
};

enum earc_chan_layout {
	earc_CHAN_LAYOUT__A,
	earc_CHAN_LAYOUT__B
};

enum earc_cap_block_id {
	EARC_CAP_BLOCK_ID__0 = 0,
	EARC_CAP_BLOCK_ID__1 = 1,
	EARC_CAP_BLOCK_ID__2 = 2,
	EARC_CAP_BLOCK_ID__3 = 3
};
enum earc_extraction_mode {
	EARC_EXTRACTION_MODE__NONE,
	EARC_EXTRACTION_MODE__SPDIF2,
	EARC_EXTRACTION_MODE__SPDIF8,
	EARC_EXTRACTION_MODE__I2S2,
	EARC_EXTRACTION_MODE__I2S8
};

struct earc_channel_status {
	unsigned char data[EARC_CHANNEL_STATUS_SIZE];
};

struct earc_audio_info {
	/* earc hw audio format reading from register*/
	unsigned int earc_audiofmt;
	/* earc audio format notify to app*/
	unsigned int earc_appadofmt;
	unsigned int earc_channelno;
	unsigned int earc_samplefreq;
	unsigned int earc_connected;
	unsigned int earc_mute;
	unsigned int earc_channelca;
};

struct earc_config {
	earc_event_callback_func callback_func;

	/* Logging FIFO Queue size in bytes */
	unsigned long log_fifo_size;

	/* Current audio latency in milliseconds from the eARC RX audio */
	/* input to the speakers*/
	struct earc_erx_latency erx_latency;
};

struct mtk_earc {
	struct device *dev;
};

struct earc_driver {
	int (*init)(void);
	};

int earcrx_debug_init(void);
void eARC_analog_Init(void);
void ARC_analog_Init(void);
void eARC_RX_Write_Audiocap(char const *ptr);
void eARC_RX_Read_Audiocap(void);
void eARC_RX_Set_AudLatency(int LatValue);
void eARC_digital_Init(void);
void eARC_RX_CM_Enable(int enable);
void eARC_RX_Freq_Meter(char freqtype);
void eARC_RX_AudioIn_Enable(int enable);
void eARC_RX_update_cap(void);
void eARC_analog_Init_test(void);
void eARC_analog_Init_test1(void);
void ARC_RX_Enable(unsigned int enable);
void i2s2ch_enable(int enable);
void Earc_Channel_Status(void);
void Earc_Audio_Format(void);
void earc_internal_power_off(void);
int earc_internal_power_on(void);
void arc_internal_power_off(void);
int arc_internal_power_on(void);
void Earc_Audio_Rst(void);
void Earc_Audio_information(void);
void Earc_statble_reg(void);
void Earc_valid_signal(void);
void Earc_mute_audio(void);

#define vWriteAnaEarc(dAddr, dVal)  (earc_ana_write(dAddr, dVal))
#define bReadAnaEarc(bAddr)         (earc_ana_read(bAddr))
#define vWriteAnaEarcMsk(dAddr, dVal, dMsk) \
	(vWriteAnaEarc((dAddr), (bReadAnaEarc(dAddr) & (~(dMsk)))\
	| (dVal & dMsk)))

#define vWriteEarc(dAddr, dVal)  (earc_drv_write(dAddr, dVal))
#define bReadEarc(bAddr)         (earc_drv_read(bAddr))
#define vWriteEarcMsk(dAddr, dVal, dMsk) \
	(vWriteEarc((dAddr), (bReadEarc(dAddr) & (~(dMsk))) | (dVal & dMsk)))

#define vWriteMonitorEarc(dAddr, dVal)  (earc_monitor_write(dAddr, dVal))
#define bReadMonitorEarc(bAddr)         (earc_monitor_read(bAddr))
#define vWriteMonitorEarcMsk(dAddr, dVal, dMsk) \
	(vWriteMonitorEarc((dAddr), (bReadMonitorEarc(dAddr) & (~(dMsk)))\
	| (dVal & dMsk)))

#endif
