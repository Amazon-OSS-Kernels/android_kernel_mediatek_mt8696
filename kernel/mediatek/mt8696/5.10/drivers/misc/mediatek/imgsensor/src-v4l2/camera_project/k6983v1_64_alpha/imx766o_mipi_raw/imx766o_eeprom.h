/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 imx766o_eeprom.h
 *
 * Project:
 * --------
 * Description:
 * ------------
 *	 Add APIs to read from EEPROM
 *
 ****************************************************************************/
#ifndef __IMX766O_EEPROM_H__
#define __IMX766O_EEPROM_H__

#include "kd_camera_typedef.h"

#include "adaptor-subdrv.h"

/*
 * LRC
 *
 * @param data Buffer
 * @return size of data
 */
unsigned int read_imx766o_LRC(struct subdrv_ctx *ctx, BYTE *data);

/*
 * DCC
 *
 * @param data Buffer
 * @return size of data
 */
unsigned int read_imx766o_DCC(struct subdrv_ctx *ctx, BYTE *data);

#endif

