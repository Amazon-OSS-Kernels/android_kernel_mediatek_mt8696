/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include "disp_dovi_common_if.h"
#include "dovi_log.h"
#include "hdmitx.h"
#include "dovi_table.h"
#include "dovi_common_hal.h"

unsigned char av1_obu_data[] = {
	0xb5, 0x00, 0x3b, 0x00,
	0x00, 0x08, 0x00, 0x08,
	0x09, 0x00, 0x40, 0x61,
	0xb6, 0x50, 0x6e, 0x20,
	0x00, 0x00, 0x81, 0x52,
	0x24, 0xfe, 0x3b, 0x09,
	0x43, 0x10, 0x1d, 0x80,
	0x1f, 0xfc, 0x00, 0xff,
	0xfd, 0x01, 0x16, 0x01,
	0x00, 0x00, 0x01, 0x50,
	0x0f, 0xb0, 0xa4, 0x93,
	0xbb, 0x8c, 0x04, 0x79,
	0x10, 0xf8, 0x15, 0x04,
	0xb6, 0xac, 0x8f, 0xb7,
	0x77, 0x36, 0xf1, 0xce,
	0x55, 0x03, 0x35, 0xce,
	0x93, 0xca, 0x17, 0x34,
	0xbc, 0x99, 0x55, 0x2c,
	0x50, 0x11, 0x7d, 0x3f,
	0xb3, 0x59, 0x5e, 0x0d,
	0x4e, 0xb5, 0x72, 0x41,
	0x0d, 0xe8, 0x8f, 0x17,
	0x6d, 0xe7, 0x95, 0x68,
	0x74, 0x77, 0x3b, 0xf2,
	0x7b, 0x17, 0x53, 0xea,
	0x14, 0x31, 0x06, 0xa5,
	0x60, 0x64, 0x6b, 0x67,
	0x40, 0xc7, 0xbb, 0xa8,
	0x19, 0xdc, 0x3c, 0x71,
	0x56, 0x77, 0x25, 0x33,
	0xb1, 0x47, 0x18, 0xac,
	0x66, 0x34, 0x68, 0x80,
	0x00, 0x0c, 0x7c, 0x1a,
	0x44, 0x80, 0x03, 0xf1,
	0x6c, 0x11, 0x0c, 0x80,
	0x00, 0x04, 0x2f, 0xa9,
	0x5c, 0x00, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x00,
	0x01, 0x0a, 0xe7, 0xfa,
	0x8f, 0xfa, 0x8f, 0xfa,
	0x8d, 0x0a, 0xe7, 0xfa,
	0x8f, 0xfa, 0x8f, 0xda,
	0x8d, 0x0a, 0xe7, 0xff,
	0xfc, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x90, 0x81, 0xf7,
	0x38, 0x05, 0x45, 0x30,
	0x08, 0x23, 0xe9, 0x69,
	0xe1, 0x00, 0xc0, 0x28,
	0x21, 0x80, 0x08, 0x00,
	0x80, 0x08, 0x00, 0x80,
	0x04, 0x00, 0x02, 0x02,
	0x1c, 0x19, 0x16, 0x09,
	0x06, 0x0f, 0xa0, 0x00,
	0x32, 0x0a, 0x2e, 0x00,
	0x34, 0xf9, 0x7e, 0x21,
	0xdd, 0x80,
};

unsigned char rpu_test_data[] = {
	0x00, 0x00, 0x00, 0x01,
	0x19, 0x08, 0x09, 0x00,
	0x40, 0x61, 0xb6, 0x50,
	0x6e, 0x20, 0x00, 0x00,
	0x81, 0x52, 0x24, 0xfe,
	0x3b, 0x09, 0x43, 0x10,
	0x1d, 0x80, 0x1f, 0xfc,
	0x00, 0xff, 0xfd, 0x01,
	0x16, 0x01, 0x00, 0x00,
	0x03, 0x01, 0x50, 0x0f,
	0xb0, 0xa4, 0x93, 0xbb,
	0x8c, 0x04, 0x79, 0x10,
	0xf8, 0x15, 0x04, 0xb6,
	0xac, 0x8f, 0xb7, 0x77,
	0x36, 0xf1, 0xce, 0x55,
	0x03, 0x35, 0xce, 0x93,
	0xca, 0x17, 0x34, 0xbc,
	0x99, 0x55, 0x2c, 0x50,
	0x11, 0x7d, 0x3f, 0xb3,
	0x59, 0x5e, 0x0d, 0x4e,
	0xb5, 0x72, 0x41, 0x0d,
	0xe8, 0x8f, 0x17, 0x6d,
	0xe7, 0x95, 0x68, 0x74,
	0x77, 0x3b, 0xf2, 0x7b,
	0x17, 0x53, 0xea, 0x14,
	0x31, 0x06, 0xa5, 0x60,
	0x64, 0x6b, 0x67, 0x40,
	0xc7, 0xbb, 0xa8, 0x19,
	0xdc, 0x3c, 0x71, 0x56,
	0x77, 0x25, 0x33, 0xb1,
	0x47, 0x18, 0xac, 0x66,
	0x34, 0x68, 0x80, 0x00,
	0x0c, 0x7c, 0x1a, 0x44,
	0x80, 0x03, 0xf1, 0x6c,
	0x11, 0x0c, 0x80, 0x00,
	0x04, 0x2f, 0xa9, 0x5c,
	0x00, 0x00, 0x03, 0x00,
	0x00, 0x20, 0x00, 0x00,
	0x03, 0x00, 0x20, 0x00,
	0x00, 0x03, 0x01, 0x0a,
	0xe7, 0xfa, 0x8f, 0xfa,
	0x8f, 0xfa, 0x8d, 0x0a,
	0xe7, 0xfa, 0x8f, 0xfa,
	0x8f, 0xfa, 0x8d, 0x0a,
	0xe7, 0xff, 0xfc, 0x00,
	0x00, 0x03, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x03,
	0x00, 0x01, 0x90, 0x81,
	0xf7, 0x38, 0x05, 0x45,
	0x30, 0x08, 0x23, 0xe9,
	0x69, 0xe1, 0x00, 0xc0,
	0x28, 0x21, 0x80, 0x08,
	0x00, 0x80, 0x08, 0x00,
	0x80, 0x04, 0x00, 0x02,
	0x02, 0x1c, 0x19, 0x16,
	0x09, 0x06, 0x0f, 0xa0,
	0x00, 0x32, 0x0a, 0x2e,
	0x00, 0x34, 0xf9, 0x7e,
	0x21, 0xdd, 0x80, 0x00,
	0x00, 0x00, 0x01, 0x19,
};
unsigned char vsvdb_v1_15[] = {
	0xEE, 0x01, 0x46, 0xd0, 0x00, 0x24,
	0x17, 0x53, 0x00, 0xab, 0x55, 0x44,
	0xac, 0x24, 0x0d, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00,
};

unsigned char dv_std_tv_vsvdb[] = {
	0xeb, 0x1, 0x46, 0xd0, 0x0, 0x26,
	0xd, 0x39, 0xf1, 0x8f, 0x49, 0x5b,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
};

unsigned char dv_ll_tv_vsvdb[] = {
	0xeb, 0x1, 0x46, 0xd0, 0x0, 0x45,
	0xb, 0x90, 0x87, 0x60, 0x76, 0x8f,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
};
unsigned char idk_5000_comp_md[] = {
	0x0c, 0x00, 0x00, 0x00,
	0x0a, 0x00, 0x00, 0x00,
	0x0a, 0x00, 0x00, 0x00,
	0x17, 0x00, 0x00, 0x00,
	0x09, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x17, 0x00, 0x00, 0x00,
	0xa0, 0x00, 0x00, 0x00,
	0x9e, 0x01, 0x00, 0x00,
	0x8a, 0x02, 0x00, 0x00,
	0x1e, 0x03, 0x00, 0x00,
	0xe2, 0x03, 0x00, 0x00,
	0xff, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0xdd, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0x02, 0x00, 0x00, 0x00,
	0xfe, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00,
	0xf4, 0xff, 0xff, 0xff,
	0x06, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x8b, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x85, 0x7d, 0x00, 0x00,
	0xc6, 0xdd, 0x49, 0x00,
	0xc0, 0x87, 0x48, 0x00,
	0x56, 0x5b, 0x02, 0x00,
	0xee, 0x6e, 0x1f, 0x00,
	0x72, 0x8e, 0x37, 0x00,
	0xe7, 0x9a, 0x01, 0x00,
	0x2e, 0x94, 0x27, 0x00,
	0xca, 0xe4, 0x25, 0x00,
	0x08, 0x28, 0x16, 0x00,
	0xd9, 0x9f, 0x3e, 0x00,
	0x06, 0xaf, 0x2c, 0x00,
	0x48, 0xae, 0x56, 0x00,
	0x1e, 0xd1, 0x1b, 0x00,
	0xf2, 0xbc, 0x6d, 0x00,
	0xee, 0xe8, 0x50, 0x00,
	0xd8, 0x93, 0x5f, 0x00,
	0x50, 0x9f, 0x3a, 0x00,
	0xac, 0xd4, 0x20, 0x00,
	0xe8, 0x6c, 0x0d, 0x00,
	0x40, 0xdd, 0x3d, 0x00,
	0x71, 0x3c, 0x5c, 0x00,
	0x94, 0xdc, 0x59, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x8c, 0xa3, 0x58, 0x00,
	0x68, 0xcc, 0x58, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

unsigned char idk_5000_dm_md[] = {
	0x00, 0x01, 0x20, 0x00,
	0x03, 0x1f, 0x06, 0x91,
	0x20, 0x00, 0xfc, 0x5b,
	0x04, 0x43, 0x20, 0x00,
	0x01, 0x0b, 0xea, 0x57,
	0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0x00, 0x00,
	0x08, 0x00, 0x00, 0x00,
	0x42, 0xb9, 0xfe, 0xa3,
	0xfe, 0xa3, 0xfe, 0xa3,
	0x42, 0xb9, 0xfe, 0xa3,
	0xfe, 0xa3, 0xfe, 0xa3,
	0x42, 0xb9, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x0c, 0x02, 0x01, 0x01,
	0x00, 0x3e, 0x0e, 0x70,
	0x00, 0x2a, 0x04, 0x00,
	0x00, 0x00, 0x06, 0x01,
	0x00, 0x47, 0x0d, 0x2d,
	0x03, 0xc2, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc, 0xcc,
	0xcc, 0xcc, 0xcc,
};

/**
 * htotal, vtotal, width , height,  freq, progressive,  hd,  resolution *
 **/
struct dobly_resolution dobly_resolution_table[] = {
{858, 525, 720, 480, 60, false, false, HDMI_VIDEO_720x480i_60Hz},
{864, 625, 720, 576, 50, false, false, HDMI_VIDEO_720x576i_50Hz},
{858, 525, 720, 480, 60, true, false, HDMI_VIDEO_720x480p_60Hz},
{864, 625, 720, 576, 50, true, false, HDMI_VIDEO_720x576p_50Hz},
{1650, 750, 1280, 720, 60, true, true, HDMI_VIDEO_1280x720p_60Hz},
{1980, 750, 1280, 720, 50, true, true, HDMI_VIDEO_1280x720p_50Hz},
{2200, 1125, 1920, 1080, 60, false, true, HDMI_VIDEO_1920x1080i_60Hz},
{2640, 1125, 1920, 1080, 50, false, true, HDMI_VIDEO_1920x1080i_50Hz},
{2200, 1125, 1920, 1080, 30, true, true, HDMI_VIDEO_1920x1080p_30Hz},
{2640, 1125, 1920, 1080, 25, true, true, HDMI_VIDEO_1920x1080p_25Hz},
{2750, 1125, 1920, 1080, 24, true, true, HDMI_VIDEO_1920x1080p_24Hz},
{2750, 1125, 1920, 1080, 23, true, true, HDMI_VIDEO_1920x1080p_23Hz},
{2200, 1125, 1920, 1080, 29, true, true, HDMI_VIDEO_1920x1080p_29Hz},
{2200, 1125, 1920, 1080, 60, true, true, HDMI_VIDEO_1920x1080p_60Hz},
{2640, 1125, 1920, 1080, 50, true, true, HDMI_VIDEO_1920x1080p_50Hz},
{1650, 750, 1280, 720, 60, true, true, HDMI_VIDEO_1280x720p3d_60Hz},
{1980, 750, 1280, 720, 50, true, true, HDMI_VIDEO_1280x720p3d_50Hz},
{2200, 1125, 1920, 1080, 60, false, true, HDMI_VIDEO_1920x1080i3d_60Hz},
{2640, 1125, 1920, 1080, 50, false, true, HDMI_VIDEO_1920x1080i3d_50Hz},
{2750, 1125, 1920, 1080, 24, true, true, HDMI_VIDEO_1920x1080p3d_24Hz},
{2750, 1125, 1920, 1080, 23, true, true, HDMI_VIDEO_1920x1080p3d_23Hz},
{5500, 2250, 3840, 2160, 23, true, true, HDMI_VIDEO_3840x2160P_23_976HZ},
{5500, 2250, 3840, 2160, 24, true, true, HDMI_VIDEO_3840x2160P_24HZ},
{5280, 2250, 3840, 2160, 25, true, true, HDMI_VIDEO_3840x2160P_25HZ},
{4400, 2250, 3840, 2160, 29, true, true, HDMI_VIDEO_3840x2160P_29_97HZ},
{4400, 2250, 3840, 2160, 30, true, true, HDMI_VIDEO_3840x2160P_30HZ},
{5500, 2250, 4096, 2160, 24, true, true, HDMI_VIDEO_4096x2160P_24HZ},
{4400, 2250, 3840, 2160, 60, true, true, HDMI_VIDEO_3840x2160P_60HZ},
{5280, 2250, 3840, 2160, 50, true, true, HDMI_VIDEO_3840x2160P_50HZ},
{4400, 2250, 4096, 2160, 60, true, true, HDMI_VIDEO_4096x2160P_60HZ},
{5280, 2250, 4096, 2160, 50, true, true, HDMI_VIDEO_4096x2160P_50HZ},
{1650, 750, 1280, 720, 60, true, true, HDMI_VIDEO_1280x720p_59_94Hz},
{2200, 1125, 1920, 1080, 60, true, true, HDMI_VIDEO_1920x1080p_59_94Hz},
{4400, 2250, 3840, 2160, 60, true, true, HDMI_VIDEO_3840x2160P_59_94HZ},
{4400, 2250, 4096, 2160, 60, true, true, HDMI_VIDEO_4096x2160P_59_94HZ},
};

const char *dobly_resstr[HDMI_VIDEO_RESOLUTION_NUM] = {
	"RES_480I",
	"RES_576I",
	"RES_480P",
	"RES_576P",
	"RES_720P60HZ",
	"RES_720P50HZ",
	"RES_1080I60HZ",
	"RES_1080I50HZ",
	"RES_1080P30HZ",
	"RES_1080P25HZ",
	"RES_1080P24HZ",
	"RES_1080P23_976HZ",
	"RES_1080P29_97HZ",
	"RES_1080P60HZ",
	"RES_1080P50HZ",
	"RES_720p3d60Hz",
	"RES_720p3d50Hz",
	"RES_1080i3d60Hz",
	"RES_1080i3d50Hz",
	"RES_1080P3d24Hz",
	"RES_1080P3d23Hz",
	"RES_2160P_23_976HZ",
	"RES_2160P_24HZ",
	"RES_2160P_25HZ",
	"RES_2160P_29_97HZ",
	"RES_2160P_30HZ",
	"RES_2161P_24HZ",
	"RES_2160P_60HZ",
	"RES_2160P_50HZ",
	"RES_2161P_60HZ",
	"RES_2161P_50HZ",
};

uint32_t force_priority_mode;
uint32_t priority_mode;
uint32_t force_g_format;
uint32_t g_format;

struct dovi_out_format_table_t dovi_out_format_tbl[MAX_DOVI_TEST_CASE_ID] = {
	//idk2.4
	{5000, OUT_FORMAT_DOVI},
	{5001, OUT_FORMAT_DOVI},
	{5002, OUT_FORMAT_DOVI},
	{5003, OUT_FORMAT_DOVI},
	//idk2.6
	{5005, OUT_FORMAT_SDR8},
	{50071, OUT_FORMAT_DOVI},
	{50072, OUT_FORMAT_DOVI},
	{5007, OUT_FORMAT_SDR8},
	{5009, OUT_FORMAT_SDR8},
	{5010, OUT_FORMAT_DOVI},
	{5011, OUT_FORMAT_DOVI},
	{5012, OUT_FORMAT_DOVI},
	{5013, OUT_FORMAT_DOVI},
	{5015, OUT_FORMAT_SDR8},
	{5020, OUT_FORMAT_SDR8},
	{5021, OUT_FORMAT_DOVI},
	{5022, OUT_FORMAT_SDR8},
	{5023, OUT_FORMAT_DOVI},
	{5024, OUT_FORMAT_DOVI},
	{5025, OUT_FORMAT_DOVI},
	{5026, OUT_FORMAT_SDR8},
	{5030, OUT_FORMAT_SDR8},
	{5031, OUT_FORMAT_SDR8},
	{5032, OUT_FORMAT_SDR8},
	{5033, OUT_FORMAT_SDR8},
	{5034, OUT_FORMAT_SDR8},
	{5035, OUT_FORMAT_SDR8},
	{5036, OUT_FORMAT_SDR8},
	{5040, OUT_FORMAT_DOVI},
	{5041, OUT_FORMAT_DOVI},
	{5042, OUT_FORMAT_DOVI},
	{5043, OUT_FORMAT_DOVI},
	{5044, OUT_FORMAT_DOVI},
	{5045, OUT_FORMAT_DOVI},
	{5046, OUT_FORMAT_DOVI},
	{5047, OUT_FORMAT_DOVI},
	{5048, OUT_FORMAT_DOVI},
	{5049, OUT_FORMAT_DOVI},
	{5051, OUT_FORMAT_DOVI},
	{50521, OUT_FORMAT_DOVI},
	{50522, OUT_FORMAT_DOVI},
	{5054, OUT_FORMAT_DOVI},
	{5060, OUT_FORMAT_DOVI},
	{5061, OUT_FORMAT_DOVI},
	{50621, OUT_FORMAT_DOVI},
	{50622, OUT_FORMAT_DOVI},
	{50623, OUT_FORMAT_DOVI},
	{50641, OUT_FORMAT_DOVI},
	{50642, OUT_FORMAT_DOVI},
	{50643, OUT_FORMAT_DOVI},
	{50644, OUT_FORMAT_DOVI},
	{50645, OUT_FORMAT_DOVI},
	{50646, OUT_FORMAT_DOVI},
	{5065, OUT_FORMAT_DOVI},
	{5067, OUT_FORMAT_DOVI},
	{5080, OUT_FORMAT_DOVI},
	{5081, OUT_FORMAT_DOVI},
	{5082, OUT_FORMAT_DOVI},
	{5083, OUT_FORMAT_DOVI},
	{51001, OUT_FORMAT_SDR8},
	{51002, OUT_FORMAT_SDR8},
	{51003, OUT_FORMAT_SDR8},
	{51004, OUT_FORMAT_SDR8},
	{51005, OUT_FORMAT_SDR8},
	{51006, OUT_FORMAT_SDR8},
	{51007, OUT_FORMAT_SDR8},
	{51008, OUT_FORMAT_SDR8},
	{51009, OUT_FORMAT_SDR8},
	{510010, OUT_FORMAT_SDR8},
	{51011, OUT_FORMAT_SDR8},
	{51012, OUT_FORMAT_SDR8},
	{51013, OUT_FORMAT_SDR8},
	{51014, OUT_FORMAT_SDR8},
	{51015, OUT_FORMAT_SDR8},
	{51016, OUT_FORMAT_SDR8},
	{51017, OUT_FORMAT_SDR8},
	{51018, OUT_FORMAT_SDR8},
	{51019, OUT_FORMAT_SDR8},
	{510110, OUT_FORMAT_SDR8},
	{51021, OUT_FORMAT_SDR8},
	{51022, OUT_FORMAT_SDR8},
	{51023, OUT_FORMAT_SDR8},
	{51024, OUT_FORMAT_SDR8},
	{51025, OUT_FORMAT_SDR8},
	{51026, OUT_FORMAT_SDR8},
	{51027, OUT_FORMAT_SDR8},
	{51028, OUT_FORMAT_SDR8},
	{51029, OUT_FORMAT_SDR8},
	{510210, OUT_FORMAT_SDR8},
	{5106, OUT_FORMAT_SDR8},
	{5200, OUT_FORMAT_SDR8},
	{5201, OUT_FORMAT_DOVI},
	{5203, OUT_FORMAT_DOVI},
	{5204, OUT_FORMAT_SDR8},
	{5205, OUT_FORMAT_HDR10},
	{5207, OUT_FORMAT_HDR10},
	{5210, OUT_FORMAT_SDR8},
	{5211, OUT_FORMAT_DOVI},
	{5213, OUT_FORMAT_DOVI},
	{5214, OUT_FORMAT_SDR8},
	{5215, OUT_FORMAT_HDR10},
	{5217, OUT_FORMAT_HDR10},
	{5220, OUT_FORMAT_SDR8},
	{5221, OUT_FORMAT_DOVI},
	{5223, OUT_FORMAT_DOVI},
	{5224, OUT_FORMAT_SDR8},
	{5225, OUT_FORMAT_HDR10},
	{5227, OUT_FORMAT_HDR10},
	{5230, OUT_FORMAT_SDR8},
	{5231, OUT_FORMAT_DOVI},
	{52321, OUT_FORMAT_DOVI},
	{52322, OUT_FORMAT_DOVI},
	{52323, OUT_FORMAT_DOVI},
	{52324, OUT_FORMAT_HDR10},
	{52325, OUT_FORMAT_SDR8},
	{5233, OUT_FORMAT_DOVI},
	{5234, OUT_FORMAT_SDR8},
	{5235, OUT_FORMAT_HDR10},
	{5237, OUT_FORMAT_HDR10},
	{5240, OUT_FORMAT_SDR8},
	{5241, OUT_FORMAT_DOVI},
	{5243, OUT_FORMAT_DOVI},
	{5244, OUT_FORMAT_SDR8},
	{5245, OUT_FORMAT_HDR10},
	{5247, OUT_FORMAT_HDR10},
	{5250, OUT_FORMAT_SDR8},
	{5251, OUT_FORMAT_DOVI},
	{5253, OUT_FORMAT_DOVI},
	{5254, OUT_FORMAT_SDR8},
	{5255, OUT_FORMAT_HDR10},
	{5257, OUT_FORMAT_HDR10},
	{5260, OUT_FORMAT_SDR8},
	{5261, OUT_FORMAT_DOVI},
	{5263, OUT_FORMAT_DOVI},
	{5264, OUT_FORMAT_SDR8},
	{5265, OUT_FORMAT_HDR10},
	{5267, OUT_FORMAT_HDR10},
	{5270, OUT_FORMAT_SDR8},
	{5271, OUT_FORMAT_DOVI},
	{52721, OUT_FORMAT_DOVI},
	{52722, OUT_FORMAT_DOVI},
	{52723, OUT_FORMAT_DOVI},
	{52724, OUT_FORMAT_HDR10},
	{52725, OUT_FORMAT_SDR8},
	{5273, OUT_FORMAT_DOVI},
	{5274, OUT_FORMAT_SDR8},
	{5275, OUT_FORMAT_HDR10},
	{5277, OUT_FORMAT_HDR10},
	{5280, OUT_FORMAT_SDR8},
	{5281, OUT_FORMAT_DOVI},
	{5283, OUT_FORMAT_DOVI},
	{5284, OUT_FORMAT_SDR8},
	{5285, OUT_FORMAT_HDR10},
	{5287, OUT_FORMAT_HDR10},
	{5290, OUT_FORMAT_SDR8},
	{5291, OUT_FORMAT_DOVI},
	{5293, OUT_FORMAT_DOVI},
	{5294, OUT_FORMAT_SDR8},
	{5295, OUT_FORMAT_HDR10},
	{5297, OUT_FORMAT_HDR10},
	{5320, OUT_FORMAT_SDR8},
	{5321, OUT_FORMAT_DOVI},
	{53221, OUT_FORMAT_DOVI},
	{53222, OUT_FORMAT_DOVI},
	{53223, OUT_FORMAT_DOVI},
	{53224, OUT_FORMAT_DOVI},
	{5323, OUT_FORMAT_DOVI},
	{5324, OUT_FORMAT_SDR8},
	{5325, OUT_FORMAT_HDR10},
	{5327, OUT_FORMAT_HDR10},
	{5330, OUT_FORMAT_SDR8},
	{5331, OUT_FORMAT_DOVI},
	{53321, OUT_FORMAT_DOVI},
	{53322, OUT_FORMAT_DOVI},
	{53323, OUT_FORMAT_DOVI},
	{53324, OUT_FORMAT_HDR10},
	{53325, OUT_FORMAT_SDR8},
	{5333, OUT_FORMAT_DOVI},
	{5334, OUT_FORMAT_SDR8},
	{5335, OUT_FORMAT_HDR10},
	{5337, OUT_FORMAT_HDR10},
	{5400, OUT_FORMAT_DOVI},
	{5401, OUT_FORMAT_DOVI},
	{5402, OUT_FORMAT_DOVI},
	{5403, OUT_FORMAT_DOVI},
	{5404, OUT_FORMAT_DOVI},
	{5405, OUT_FORMAT_DOVI},
	{5406, OUT_FORMAT_DOVI},
	{5407, OUT_FORMAT_DOVI},
	{5408, OUT_FORMAT_DOVI},
	{5409, OUT_FORMAT_DOVI},
	{5410, OUT_FORMAT_DOVI},
	{5411, OUT_FORMAT_DOVI},
	{54121, OUT_FORMAT_DOVI},
	{54122, OUT_FORMAT_DOVI},
	{5413, OUT_FORMAT_DOVI},
	{5414, OUT_FORMAT_DOVI},
	{5415, OUT_FORMAT_DOVI},
	{5416, OUT_FORMAT_DOVI},
	{5417, OUT_FORMAT_DOVI},
	{5418, OUT_FORMAT_DOVI},
	{5419, OUT_FORMAT_DOVI},
	{5420, OUT_FORMAT_DOVI},
	{5421, OUT_FORMAT_DOVI},
	{5422, OUT_FORMAT_DOVI},
	{5423, OUT_FORMAT_DOVI},
	{5424, OUT_FORMAT_DOVI},
	{5425, OUT_FORMAT_DOVI},
	{5426, OUT_FORMAT_DOVI},
	{5427, OUT_FORMAT_DOVI},
	{5428, OUT_FORMAT_DOVI},
	{5429, OUT_FORMAT_DOVI},
	{5430, OUT_FORMAT_DOVI},
	{5431, OUT_FORMAT_DOVI},
	{5432, OUT_FORMAT_DOVI},
	{5433, OUT_FORMAT_DOVI},
	{5434, OUT_FORMAT_DOVI},
	{5435, OUT_FORMAT_DOVI},
	{5436, OUT_FORMAT_HDR10},
	{5437, OUT_FORMAT_HDR10},
	{5439, OUT_FORMAT_HDR10},
	{5440, OUT_FORMAT_HDR10},
	{5442, OUT_FORMAT_HDR10},
	{5443, OUT_FORMAT_HDR10},
	{5448, OUT_FORMAT_SDR8},
	{5449, OUT_FORMAT_SDR8},
};

// case ID£¬ vsvdb, use_ll, use_ll_rgb
struct dovi_case_info_t dovi_case_info[MAX_DOVI_TEST_CASE_ID] = {
	{5000, "vsvdb_v1-15b.bin"},
	{5001, "vsvdb_v1-15b.bin"},
	{5002, "vsvdb_v1-15b.bin"},
	{5003, "vsvdb_v1-15b.bin"},
	//idk2.6
	{5005, ""},
	{50071, "vsvdb_v2-if3.bin"},
	{50072, "vsvdb_v2-if3-dmver4.bin"},
	{5007, ""},
	{5009, ""},
	{5010, "vsvdb_v0.bin"},
	{5011, "vsvdb_v1-15b.bin"},
	{5012, "vsvdb_v0.bin"},
	{5013, "vsvdb_v2-if3.bin"},
	{5015, ""},
	{5020, ""},
	{5021, "vsvdb_v2-if3.bin"},
	{5022, ""},
	{5023, "vsvdb_v2-if3.bin"},
	{5024, "vsvdb_v2-if3.bin"},
	{5025, "vsvdb_v2-if3.bin"},
	{5026, ""},
	{5030, ""},
	{5031, ""},
	{5032, ""},
	{5033, ""},
	{5034, ""},
	{5035, ""},
	{5036, ""},
	{5040, "vsvdb_v1-12b-ll.bin"},
	{5041, "vsvdb_v1-12b-ll.bin", 1},
	{5042, "vsvdb_v2-if0.bin", 1},
	{5043, "vsvdb_v2-if1.bin", 1},
	{5044, "vsvdb_v2-if1.bin", 1, 1},
	{5045, "vsvdb_v2-if2.bin"},
	{5046, "vsvdb_v2-if2.bin", 1},
	{5047, "vsvdb_v2-if3.bin"},
	{5048, "vsvdb_v2-if3.bin", 1},
	{5049, "vsvdb_v2-if3.bin", 1, 1},
	{5051, "vsvdb_v2-if3.bin"},
	{50521, "vsvdb_v2-if3.bin", 1},
	{50522, "vsvdb_v2-if0-dmver3.bin", 1},
	{5054, "vsvdb_v2-if3.bin"},
	{5060, "vsvdb_v2-if3.bin"},
	{5061, "vsvdb_v2-if3.bin"},
	{50621, "vsvdb_v2-if3.bin"},
	{50622, "vsvdb_v2-if3.bin", 1},
	{50623, "vsvdb_v2-if0-dmver3.bin", 1},
	{50641, "vsvdb_v2-if3.bin"},
	{50642, "vsvdb_v2-if3.bin", 1},
	{50643, "vsvdb_v2-if0-dmver3.bin", 1},
	{50644, "vsvdb_v2-if0-dmver3.bin", 1},
	{50645, "vsvdb_v2-if3.bin"},
	{50646, "vsvdb_v2-if3.bin", 1},
	{5065, "vsvdb_v2-if3.bin"},
	{5067, "vsvdb_v2-if3.bin"},
	{5080, "vsvdb_v2-if3.bin", 1},
	{5081, "vsvdb_v2-if3.bin", 1},
	{5082, "vsvdb_v2-if0-dmver3.bin", 1},
	{5083, "vsvdb_v2-if0-dmver3.bin", 1, 1},
	{51001, ""},
	{51002, ""},
	{51003, ""},
	{51004, ""},
	{51005, ""},
	{51006, ""},
	{51007, ""},
	{51008, ""},
	{51009, ""},
	{510010, ""},
	{51011, ""},
	{51012, ""},
	{51013, ""},
	{51014, ""},
	{51015, ""},
	{51016, ""},
	{51017, ""},
	{51018, ""},
	{51019, ""},
	{510110, ""},
	{51021, ""},
	{51022, ""},
	{51023, ""},
	{51024, ""},
	{51025, ""},
	{51026, ""},
	{51027, ""},
	{51028, ""},
	{51029, ""},
	{510210, ""},
	{5106, ""},
	{5200, ""},
	{5201, "vsvdb_v2-if2.bin"},
	{5203, "vsvdb_v2-if2.bin"},
	{5204, ""},
	{5205, ""},
	{5207, ""},
	{5210, ""},
	{5211, "vsvdb_v2-if2.bin"},
	{5213, "vsvdb_v2-if2.bin"},
	{5214, ""},
	{5215, ""},
	{5217, ""},
	{5220, ""},
	{5221, "vsvdb_v2-if2.bin"},
	{5223, "vsvdb_v2-if2.bin"},
	{5224, ""},
	{5225, ""},
	{5227, ""},
	{5230, ""},
	{5231, "vsvdb_v2-if2.bin"},
	{52321, "vsvdb_v2-if3.bin"},
	{52322, "vsvdb_v2-if0.bin"},
	{52323, "vsvdb_v2-if0-dmver3.bin"},
	{52324, ""},
	{52325, ""},
	{5233, "vsvdb_v2-if2.bin"},
	{5234, ""},
	{5235, ""},
	{5237, ""},
	{5240, ""},
	{5241, "vsvdb_v2-if2.bin"},
	{5243, "vsvdb_v2-if2.bin"},
	{5244, ""},
	{5245, ""},
	{5247, ""},
	{5250, ""},
	{5251, "vsvdb_v2-if2.bin"},
	{5253, "vsvdb_v2-if2.bin"},
	{5254, ""},
	{5255, ""},
	{5257, ""},
	{5260, ""},
	{5261, "vsvdb_v2-if2.bin"},
	{5263, "vsvdb_v2-if2.bin"},
	{5264, ""},
	{5265, ""},
	{5267, ""},
	{5270, ""},
	{5271, "vsvdb_v2-if2.bin"},
	{52721, "vsvdb_v2-if3.bin"},
	{52722, "vsvdb_v2-if0.bin"},
	{52723, "vsvdb_v2-if0-dmver3.bin"},
	{52724, ""},
	{52725, ""},
	{5273, "vsvdb_v2-if2.bin"},
	{5274, ""},
	{5275, ""},
	{5277, ""},
	{5280, ""},
	{5281, "vsvdb_v2-if2.bin"},
	{5283, "vsvdb_v2-if2.bin"},
	{5284, ""},
	{5285, ""},
	{5287, ""},
	{5290, ""},
	{5291, "vsvdb_v2-if2.bin"},
	{5293, "vsvdb_v2-if2.bin"},
	{5294, ""},
	{5295, ""},
	{5297, ""},
	{5320, ""},
	{5321, "vsvdb_v2-if2.bin"},
	{53221, "vsvdb_v2-if3.bin", 1},
	{53222, "vsvdb_v2-if0-dmver3.bin"},
	{53223, "vsvdb_v2-if3.bin", 1},
	{53224, "vsvdb_v2-if0-dmver3.bin"},
	{5323, "vsvdb_v2-if2.bin"},
	{5324, ""},
	{5325, ""},
	{5327, ""},
	{5330, ""},
	{5331, "vsvdb_v2-if2.bin"},
	{53321, "vsvdb_v2-if3.bin"},
	{53322, "vsvdb_v2-if0.bin"},
	{53323, "vsvdb_v2-if0-dmver3.bin"},
	{53324, ""},
	{53325, ""},
	{5333, "vsvdb_v2-if2.bin"},
	{5334, ""},
	{5335, ""},
	{5337, ""},
	{5400, "vsvdb_v2-if3.bin"},
	{5401, "vsvdb_v2-if3.bin"},
	{5402, "vsvdb_v2-if3.bin"},
	{5403, "vsvdb_v2-if3.bin"},
	{5404, "vsvdb_v2-if3.bin"},
	{5405, "vsvdb_v2-if3.bin"},
	{5406, "vsvdb_v2-if3.bin"},
	{5407, "vsvdb_v2-if3.bin"},
	{5408, "vsvdb_v2-if3.bin"},
	{5409, "vsvdb_v2-if3.bin"},
	{5410, "vsvdb_v2-if3.bin"},
	{5411, "vsvdb_v2-if3.bin"},
	{54121, "vsvdb_v2-if3.bin"},
	{54122, "vsvdb_v2-if3.bin"},
	{5413, "vsvdb_v2-if3.bin"},
	{5414, "vsvdb_v2-if3.bin"},
	{5415, "vsvdb_v2-if3.bin"},
	{5416, "vsvdb_v2-if3.bin"},
	{5417, "vsvdb_v2-if3.bin"},
	{5418, "vsvdb_v2-if3.bin"},
	{5419, "vsvdb_v2-if3.bin"},
	{5420, "vsvdb_v2-if3.bin"},
	{5421, "vsvdb_v2-if3.bin", 1},
	{5422, "vsvdb_v2-if3.bin", 1},
	{5423, "vsvdb_v2-if3.bin", 1},
	{5424, "vsvdb_v2-if3.bin", 1},
	{5425, "vsvdb_v2-if3.bin", 1},
	{5426, "vsvdb_v2-if3.bin", 1},
	{5427, "vsvdb_v2-if3.bin", 1},
	{5428, "vsvdb_v2-if3.bin", 1},
	{5429, "vsvdb_v2-if3.bin", 1},
	{5430, "vsvdb_v2-if3.bin", 1},
	{5431, "vsvdb_v2-if3.bin", 1},
	{5432, "vsvdb_v2-if3.bin", 1},
	{5433, "vsvdb_v2-if3.bin", 1},
	{5434, "vsvdb_v2-if3.bin", 1},
	{5435, "vsvdb_v2-if3.bin", 1},
	{5436, ""},
	{5437, ""},
	{5439, ""},
	{5440, ""},
	{5442, ""},
	{5443, ""},
	{5448, ""},
	{5449, ""},
};

//case id, normal/pip/sbs mode, v/g priority
struct dovi_mode_priority_t dovi_mode_priority[MAX_DOVI_TEST_CASE_ID] = {
	{5000, NORMAL_MODE, V_PRIORITY},
	{5001, NORMAL_MODE, V_PRIORITY},
	{5002, NORMAL_MODE, V_PRIORITY},
	{5003, NORMAL_MODE, V_PRIORITY},
	//idk2.6
	{5005, NORMAL_MODE, G_PRIORITY},
	{50071, NORMAL_MODE, V_PRIORITY},
	{50072, NORMAL_MODE, V_PRIORITY},
	{5007, NORMAL_MODE, G_PRIORITY},
	{5009, NORMAL_MODE, G_PRIORITY},
	{5010, NORMAL_MODE, G_PRIORITY},
	{5011, NORMAL_MODE, V_PRIORITY},
	{5012, NORMAL_MODE, G_PRIORITY},
	{5013, NORMAL_MODE, V_PRIORITY},
	{5015, NORMAL_MODE, G_PRIORITY},
	{5020, NORMAL_MODE, G_PRIORITY},
	{5021, NORMAL_MODE, G_PRIORITY},
	{5022, NORMAL_MODE, G_PRIORITY},
	{5023, NORMAL_MODE, G_PRIORITY},
	{5024, NORMAL_MODE, G_PRIORITY},
	{5025, NORMAL_MODE, G_PRIORITY},
	{5026, NORMAL_MODE, G_PRIORITY},
	{5030, NORMAL_MODE, G_PRIORITY},
	{5031, NORMAL_MODE, G_PRIORITY},
	{5032, NORMAL_MODE, G_PRIORITY},
	{5033, NORMAL_MODE, G_PRIORITY},
	{5034, NORMAL_MODE, G_PRIORITY},
	{5035, NORMAL_MODE, G_PRIORITY},
	{5036, NORMAL_MODE, G_PRIORITY},
	{5040, NORMAL_MODE, G_PRIORITY},
	{5041, NORMAL_MODE, G_PRIORITY},
	{5042, NORMAL_MODE, G_PRIORITY},
	{5043, NORMAL_MODE, G_PRIORITY},
	{5044, NORMAL_MODE, G_PRIORITY},
	{5045, NORMAL_MODE, G_PRIORITY},
	{5046, NORMAL_MODE, G_PRIORITY},
	{5047, NORMAL_MODE, G_PRIORITY},
	{5048, NORMAL_MODE, G_PRIORITY},
	{5049, NORMAL_MODE, G_PRIORITY},
	{5051, NORMAL_MODE, G_PRIORITY},
	{50521, NORMAL_MODE, G_PRIORITY},
	{50522, NORMAL_MODE, G_PRIORITY},
	{5054, NORMAL_MODE, G_PRIORITY},
	{5060, NORMAL_MODE, G_PRIORITY},
	{5061, NORMAL_MODE, G_PRIORITY},
	{50621, NORMAL_MODE, G_PRIORITY},
	{50622, NORMAL_MODE, G_PRIORITY},
	{50623, NORMAL_MODE, G_PRIORITY},
	{50641, NORMAL_MODE, G_PRIORITY},
	{50642, NORMAL_MODE, G_PRIORITY},
	{50643, NORMAL_MODE, G_PRIORITY},
	{50644, NORMAL_MODE, G_PRIORITY},
	{50645, NORMAL_MODE, G_PRIORITY},
	{50646, NORMAL_MODE, G_PRIORITY},
	{5065, NORMAL_MODE, G_PRIORITY},
	{5067, NORMAL_MODE, G_PRIORITY},
	{5080, NORMAL_MODE, G_PRIORITY},
	{5081, NORMAL_MODE, G_PRIORITY},
	{5082, NORMAL_MODE, G_PRIORITY},
	{5083, NORMAL_MODE, G_PRIORITY},
	{51001, NORMAL_MODE, G_PRIORITY},
	{51002, NORMAL_MODE, G_PRIORITY},
	{51003, NORMAL_MODE, G_PRIORITY},
	{51004, NORMAL_MODE, G_PRIORITY},
	{51005, NORMAL_MODE, G_PRIORITY},
	{51006, NORMAL_MODE, G_PRIORITY},
	{51007, NORMAL_MODE, G_PRIORITY},
	{51008, NORMAL_MODE, G_PRIORITY},
	{51009, NORMAL_MODE, G_PRIORITY},
	{510010, NORMAL_MODE, G_PRIORITY},
	{51011, NORMAL_MODE, G_PRIORITY},
	{51012, NORMAL_MODE, G_PRIORITY},
	{51013, NORMAL_MODE, G_PRIORITY},
	{51014, NORMAL_MODE, G_PRIORITY},
	{51015, NORMAL_MODE, G_PRIORITY},
	{51016, NORMAL_MODE, G_PRIORITY},
	{51017, NORMAL_MODE, G_PRIORITY},
	{51018, NORMAL_MODE, G_PRIORITY},
	{51019, NORMAL_MODE, G_PRIORITY},
	{510110, NORMAL_MODE, G_PRIORITY},
	{51021, NORMAL_MODE, G_PRIORITY},
	{51022, NORMAL_MODE, G_PRIORITY},
	{51023, NORMAL_MODE, G_PRIORITY},
	{51024, NORMAL_MODE, G_PRIORITY},
	{51025, NORMAL_MODE, G_PRIORITY},
	{51026, NORMAL_MODE, G_PRIORITY},
	{51027, NORMAL_MODE, G_PRIORITY},
	{51028, NORMAL_MODE, G_PRIORITY},
	{51029, NORMAL_MODE, G_PRIORITY},
	{510210, NORMAL_MODE, G_PRIORITY},
	{5106, NORMAL_MODE, G_PRIORITY},
	{5200, NORMAL_MODE, G_PRIORITY},
	{5201, NORMAL_MODE, V_PRIORITY},
	{5203, NORMAL_MODE, V_PRIORITY},
	{5204, NORMAL_MODE, G_PRIORITY},
	{5205, NORMAL_MODE, G_PRIORITY},
	{5207, NORMAL_MODE, G_PRIORITY},
	{5210, NORMAL_MODE, G_PRIORITY},
	{5211, NORMAL_MODE, G_PRIORITY},
	{5213, NORMAL_MODE, G_PRIORITY},
	{5214, NORMAL_MODE, G_PRIORITY},
	{5215, NORMAL_MODE, G_PRIORITY},
	{5217, NORMAL_MODE, G_PRIORITY},
	{5220, NORMAL_MODE, G_PRIORITY},
	{5221, NORMAL_MODE, V_PRIORITY},
	{5223, NORMAL_MODE, V_PRIORITY},
	{5224, NORMAL_MODE, G_PRIORITY},
	{5225, NORMAL_MODE, G_PRIORITY},
	{5227, NORMAL_MODE, G_PRIORITY},
	{5230, NORMAL_MODE, G_PRIORITY},
	{5231, NORMAL_MODE, V_PRIORITY},
	{52321, NORMAL_MODE, G_PRIORITY},
	{52322, NORMAL_MODE, G_PRIORITY},
	{52323, NORMAL_MODE, G_PRIORITY},
	{52324, NORMAL_MODE, G_PRIORITY},
	{52325, NORMAL_MODE, G_PRIORITY},
	{5233, NORMAL_MODE, V_PRIORITY},
	{5234, NORMAL_MODE, G_PRIORITY},
	{5235, NORMAL_MODE, G_PRIORITY},
	{5237, NORMAL_MODE, G_PRIORITY},
	{5240, NORMAL_MODE, V_PRIORITY},
	{5241, NORMAL_MODE, V_PRIORITY},
	{5243, NORMAL_MODE, V_PRIORITY},
	{5244, NORMAL_MODE, V_PRIORITY},
	{5245, NORMAL_MODE, G_PRIORITY},
	{5247, NORMAL_MODE, G_PRIORITY},
	{5250, NORMAL_MODE, G_PRIORITY},
	{5251, NORMAL_MODE, V_PRIORITY},
	{5253, NORMAL_MODE, V_PRIORITY},
	{5254, NORMAL_MODE, G_PRIORITY},
	{5255, NORMAL_MODE, G_PRIORITY},
	{5257, NORMAL_MODE, G_PRIORITY},
	{5260, NORMAL_MODE, G_PRIORITY},
	{5261, NORMAL_MODE, V_PRIORITY},
	{5263, NORMAL_MODE, V_PRIORITY},
	{5264, NORMAL_MODE, G_PRIORITY},
	{5265, NORMAL_MODE, G_PRIORITY},
	{5267, NORMAL_MODE, G_PRIORITY},
	{5270, NORMAL_MODE, G_PRIORITY},
	{5271, NORMAL_MODE, V_PRIORITY},
	{52721, NORMAL_MODE, G_PRIORITY},
	{52722, NORMAL_MODE, G_PRIORITY},
	{52723, NORMAL_MODE, G_PRIORITY},
	{52724, NORMAL_MODE, G_PRIORITY},
	{52725, NORMAL_MODE, G_PRIORITY},
	{5273, NORMAL_MODE, V_PRIORITY},
	{5274, NORMAL_MODE, G_PRIORITY},
	{5275, NORMAL_MODE, G_PRIORITY},
	{5277, NORMAL_MODE, G_PRIORITY},
	{5280, NORMAL_MODE, G_PRIORITY},
	{5281, NORMAL_MODE, V_PRIORITY},
	{5283, NORMAL_MODE, V_PRIORITY},
	{5284, NORMAL_MODE, G_PRIORITY},
	{5285, NORMAL_MODE, G_PRIORITY},
	{5287, NORMAL_MODE, G_PRIORITY},
	{5290, NORMAL_MODE, G_PRIORITY},
	{5291, NORMAL_MODE, V_PRIORITY},
	{5293, NORMAL_MODE, V_PRIORITY},
	{5294, NORMAL_MODE, G_PRIORITY},
	{5295, NORMAL_MODE, G_PRIORITY},
	{5297, NORMAL_MODE, G_PRIORITY},
	{5320, NORMAL_MODE, G_PRIORITY},
	{5321, NORMAL_MODE, V_PRIORITY},
	{53221, NORMAL_MODE, G_PRIORITY},
	{53222, NORMAL_MODE, G_PRIORITY},
	{53223, NORMAL_MODE, G_PRIORITY},
	{53224, NORMAL_MODE, G_PRIORITY},
	{5323, NORMAL_MODE, V_PRIORITY},
	{5324, NORMAL_MODE, G_PRIORITY},
	{5325, NORMAL_MODE, G_PRIORITY},
	{5327, NORMAL_MODE, G_PRIORITY},
	{5330, NORMAL_MODE, G_PRIORITY},
	{5331, NORMAL_MODE, V_PRIORITY},
	{53321, NORMAL_MODE, G_PRIORITY},
	{53322, NORMAL_MODE, G_PRIORITY},
	{53323, NORMAL_MODE, G_PRIORITY},
	{53324, NORMAL_MODE, G_PRIORITY},
	{53325, NORMAL_MODE, G_PRIORITY},
	{5333, NORMAL_MODE, V_PRIORITY},
	{5334, NORMAL_MODE, G_PRIORITY},
	{5335, NORMAL_MODE, G_PRIORITY},
	{5337, NORMAL_MODE, G_PRIORITY},
	{5400, PIP_MODE, V_PRIORITY},
	{5401, PIP_MODE, V_PRIORITY},
	{5402, PIP_MODE, V_PRIORITY},
	{5403, PIP_MODE, V_PRIORITY},
	{5404, SBS_MODE, V_PRIORITY},
	{5405, PIP_MODE, V_PRIORITY},
	{5406, SBS_MODE, G_PRIORITY},
	{5407, PIP_MODE, G_PRIORITY},
	{5408, PIP_MODE, G_PRIORITY},
	{5409, PIP_MODE, G_PRIORITY},
	{5410, PIP_MODE, G_PRIORITY},
	{5411, SBS_MODE, G_PRIORITY},
	{54121, PIP_MODE, V_PRIORITY},
	{54122, PIP_MODE, G_PRIORITY},
	{5413, PIP_MODE, G_PRIORITY},
	{5414, SBS_MODE, G_PRIORITY},
	{5415, PIP_MODE, G_PRIORITY},
	{5416, PIP_MODE, G_PRIORITY},
	{5417, PIP_MODE, G_PRIORITY},
	{5418, SBS_MODE, G_PRIORITY},
	{5419, PIP_MODE, G_PRIORITY},
	{5420, PIP_MODE, G_PRIORITY},
	{5421, PIP_MODE, G_PRIORITY},
	{5422, SBS_MODE, G_PRIORITY},
	{5423, PIP_MODE, G_PRIORITY},
	{5424, PIP_MODE, G_PRIORITY},
	{5425, PIP_MODE, G_PRIORITY},
	{5426, PIP_MODE, G_PRIORITY},
	{5427, SBS_MODE, G_PRIORITY},
	{5428, PIP_MODE, G_PRIORITY},
	{5429, PIP_MODE, G_PRIORITY},
	{5430, SBS_MODE, G_PRIORITY},
	{5431, PIP_MODE, G_PRIORITY},
	{5432, PIP_MODE, G_PRIORITY},
	{5433, PIP_MODE, G_PRIORITY},
	{5434, SBS_MODE, G_PRIORITY},
	{5435, PIP_MODE, G_PRIORITY},
	{5436, PIP_MODE, G_PRIORITY},
	{5437, SBS_MODE, G_PRIORITY},
	{5439, PIP_MODE, G_PRIORITY},
	{5440, PIP_MODE, G_PRIORITY},
	{5442, PIP_MODE, G_PRIORITY},
	{5443, SBS_MODE, G_PRIORITY},
	{5448, PIP_MODE, G_PRIORITY},
	{5449, SBS_MODE, G_PRIORITY},
};

//case id, in type, out type
struct dovi_vdo_inout_type_t dovi_vdo_inout_type[MAX_DOVI_TEST_CASE_ID] = {
	{5005, VDO_TYPE_OTT},
	{50071, VDO_TYPE_OTT},
	{50072, VDO_TYPE_OTT},
	{5007, VDO_TYPE_OTT},
	{5009, VDO_TYPE_OTT},
	{5010, VDO_TYPE_HDMI2P0},
	{5011, VDO_TYPE_HDMI2P0},
	{5012, VDO_TYPE_HDMI2P0},
	{5013, VDO_TYPE_HDMI2P1},
	{5015, VDO_TYPE_HDMI2P0},
	{5020, VDO_TYPE_OTT},
	{5021, VDO_TYPE_OTT},
	{5022, VDO_TYPE_OTT},
	{5023, VDO_TYPE_OTT},
	{5024, VDO_TYPE_OTT},
	{5025, VDO_TYPE_OTT},
	{5026, VDO_TYPE_OTT},
	{5030, VDO_TYPE_OTT},
	{5031, VDO_TYPE_OTT},
	{5032, VDO_TYPE_OTT},
	{5033, VDO_TYPE_OTT},
	{5034, VDO_TYPE_OTT},
	{5035, VDO_TYPE_OTT},
	{5036, VDO_TYPE_OTT},
	{5040, VDO_TYPE_OTT},
	{5041, VDO_TYPE_OTT},
	{5042, VDO_TYPE_OTT},
	{5043, VDO_TYPE_OTT},
	{5044, VDO_TYPE_OTT},
	{5045, VDO_TYPE_OTT},
	{5046, VDO_TYPE_OTT},
	{5047, VDO_TYPE_OTT},
	{5048, VDO_TYPE_OTT},
	{5049, VDO_TYPE_OTT},
	{5051, VDO_TYPE_HDMI2P0},
	{50521, VDO_TYPE_HDMI2P0},
	{50522, VDO_TYPE_HDMI2P0, VDO_TYPE_HDMI2P1},
	{5054, VDO_TYPE_HDMI2P0},
	{5060, VDO_TYPE_HDMI2P0},
	{5061, VDO_TYPE_HDMI2P0},
	{50621, VDO_TYPE_HDMI2P1},
	{50622, VDO_TYPE_HDMI2P1},
	{50623, VDO_TYPE_HDMI2P1, VDO_TYPE_HDMI2P1},
	{50641, VDO_TYPE_HDMI1P4},
	{50642, VDO_TYPE_HDMI1P4},
	{50643, VDO_TYPE_HDMI1P4, VDO_TYPE_HDMI2P1},
	{50644, VDO_TYPE_HDMI1P4, VDO_TYPE_HDMI2P1},
	{50645, VDO_TYPE_HDMI1P4},
	{50646, VDO_TYPE_HDMI1P4},
	{5065, VDO_TYPE_HDMI1P4},
	{5067, VDO_TYPE_HDMI2P0},
	{5080, VDO_TYPE_OTT},
	{5081, VDO_TYPE_OTT},
	{5082, VDO_TYPE_OTT, VDO_TYPE_HDMI2P1},
	{5083, VDO_TYPE_OTT, VDO_TYPE_HDMI2P1},
	{51001, VDO_TYPE_OTT},
	{51002, VDO_TYPE_OTT},
	{51003, VDO_TYPE_OTT},
	{51004, VDO_TYPE_OTT},
	{51005, VDO_TYPE_OTT},
	{51006, VDO_TYPE_OTT},
	{51007, VDO_TYPE_OTT},
	{51008, VDO_TYPE_OTT},
	{51009, VDO_TYPE_OTT},
	{510010, VDO_TYPE_OTT},
	{51011, VDO_TYPE_OTT},
	{51012, VDO_TYPE_OTT},
	{51013, VDO_TYPE_OTT},
	{51014, VDO_TYPE_OTT},
	{51015, VDO_TYPE_OTT},
	{51016, VDO_TYPE_OTT},
	{51017, VDO_TYPE_OTT},
	{51018, VDO_TYPE_OTT},
	{51019, VDO_TYPE_OTT},
	{510110, VDO_TYPE_OTT},
	{51021, VDO_TYPE_OTT},
	{51022, VDO_TYPE_OTT},
	{51023, VDO_TYPE_OTT},
	{51024, VDO_TYPE_OTT},
	{51025, VDO_TYPE_OTT},
	{51026, VDO_TYPE_OTT},
	{51027, VDO_TYPE_OTT},
	{51028, VDO_TYPE_OTT},
	{51029, VDO_TYPE_OTT},
	{510210, VDO_TYPE_OTT},
	{5106, VDO_TYPE_OTT},
	{5200, VDO_TYPE_OTT},
	{5201, VDO_TYPE_OTT},
	{5203, VDO_TYPE_OTT},
	{5204, VDO_TYPE_OTT},
	{5205, VDO_TYPE_OTT},
	{5207, VDO_TYPE_OTT},
	{5210, VDO_TYPE_OTT},
	{5211, VDO_TYPE_OTT},
	{5213, VDO_TYPE_OTT},
	{5214, VDO_TYPE_OTT},
	{5215, VDO_TYPE_OTT},
	{5217, VDO_TYPE_OTT},
	{5220, VDO_TYPE_OTT},
	{5221, VDO_TYPE_OTT},
	{5223, VDO_TYPE_OTT},
	{5224, VDO_TYPE_OTT},
	{5225, VDO_TYPE_OTT},
	{5227, VDO_TYPE_OTT},
	{5230, VDO_TYPE_OTT},
	{5231, VDO_TYPE_OTT},
	{52321, VDO_TYPE_HDMI1P4},
	{52322, VDO_TYPE_HDMI1P4},
	{52323, VDO_TYPE_HDMI1P4, VDO_TYPE_HDMI2P1},
	{52324, VDO_TYPE_HDMI1P4},
	{52325, VDO_TYPE_HDMI1P4},
	{5233, VDO_TYPE_OTT},
	{5234, VDO_TYPE_OTT},
	{5235, VDO_TYPE_OTT},
	{5237, VDO_TYPE_OTT},
	{5240, VDO_TYPE_OTT},
	{5241, VDO_TYPE_OTT},
	{5243, VDO_TYPE_OTT},
	{5244, VDO_TYPE_OTT},
	{5245, VDO_TYPE_OTT},
	{5247, VDO_TYPE_OTT},
	{5250, VDO_TYPE_OTT},
	{5251, VDO_TYPE_OTT},
	{5253, VDO_TYPE_OTT},
	{5254, VDO_TYPE_OTT},
	{5255, VDO_TYPE_OTT},
	{5257, VDO_TYPE_OTT},
	{5260, VDO_TYPE_OTT},
	{5261, VDO_TYPE_OTT},
	{5263, VDO_TYPE_OTT},
	{5264, VDO_TYPE_OTT},
	{5265, VDO_TYPE_OTT},
	{5267, VDO_TYPE_OTT},
	{5270, VDO_TYPE_OTT},
	{5271, VDO_TYPE_OTT},
	{52721, VDO_TYPE_HDMI1P4},
	{52722, VDO_TYPE_HDMI1P4},
	{52723, VDO_TYPE_HDMI1P4, VDO_TYPE_HDMI2P1},
	{52724, VDO_TYPE_HDMI1P4},
	{52725, VDO_TYPE_HDMI1P4},
	{5273, VDO_TYPE_OTT},
	{5274, VDO_TYPE_OTT},
	{5275, VDO_TYPE_OTT},
	{5277, VDO_TYPE_OTT},
	{5280, VDO_TYPE_OTT},
	{5281, VDO_TYPE_OTT},
	{5283, VDO_TYPE_OTT},
	{5284, VDO_TYPE_OTT},
	{5285, VDO_TYPE_OTT},
	{5287, VDO_TYPE_OTT},
	{5290, VDO_TYPE_OTT},
	{5291, VDO_TYPE_OTT},
	{5293, VDO_TYPE_OTT},
	{5294, VDO_TYPE_OTT},
	{5295, VDO_TYPE_OTT},
	{5297, VDO_TYPE_OTT},
	{5320, VDO_TYPE_OTT},
	{5321, VDO_TYPE_OTT},
	{53221, VDO_TYPE_HDMI2P0},
	{53222, VDO_TYPE_HDMI2P0, VDO_TYPE_HDMI2P1},
	{53223, VDO_TYPE_HDMI2P1},
	{53224, VDO_TYPE_HDMI2P1, VDO_TYPE_HDMI2P1},
	{5323, VDO_TYPE_OTT},
	{5324, VDO_TYPE_OTT},
	{5325, VDO_TYPE_OTT},
	{5327, VDO_TYPE_OTT},
	{5330, VDO_TYPE_OTT},
	{5331, VDO_TYPE_OTT},
	{53321, VDO_TYPE_HDMI1P4},
	{53322, VDO_TYPE_HDMI1P4},
	{53323, VDO_TYPE_HDMI1P4, VDO_TYPE_HDMI2P1},
	{53324, VDO_TYPE_HDMI1P4},
	{53325, VDO_TYPE_HDMI1P4},
	{5333, VDO_TYPE_OTT},
	{5334, VDO_TYPE_OTT},
	{5335, VDO_TYPE_OTT},
	{5337, VDO_TYPE_OTT},
};

//case id, in hdmi1.4, drm file
struct dovi_drm_type_t dovi_drm_type[MID_DOVI_TEST_CASE_ID] = {
	{50641, VDO_TYPE_HDMI1P4,
	"Dovi_DynamicRangeandMasteringInfoFrame.bin"},
	{50642, VDO_TYPE_HDMI1P4,
	"Dovi_DynamicRangeandMasteringInfoFrame.bin"},
	{50643, VDO_TYPE_HDMI1P4,
	"Dovi_DynamicRangeandMasteringInfoFrame.bin"},
	{50644, VDO_TYPE_HDMI1P4,
	"Dovi_DynamicRangeandMasteringInfoFrame.bin"},
	{50645, VDO_TYPE_HDMI1P4,
	"Dovi_DynamicRangeandMasteringInfoFrame.bin"},
	{50646, VDO_TYPE_HDMI1P4,
	"Dovi_DynamicRangeandMasteringInfoFrame.bin"},
	{5065, VDO_TYPE_HDMI1P4,
	"Dovi_DynamicRangeandMasteringInfoFrame.bin"},
	{52321, VDO_TYPE_HDMI1P4, ""},
	{52322, VDO_TYPE_HDMI1P4, ""},
	{52323, VDO_TYPE_HDMI1P4, ""},
	{52324, VDO_TYPE_HDMI1P4, ""},
	{52325, VDO_TYPE_HDMI1P4, ""},
	{52721, VDO_TYPE_HDMI1P4, ""},
	{52722, VDO_TYPE_HDMI1P4, ""},
	{52723, VDO_TYPE_HDMI1P4, ""},
	{52724, VDO_TYPE_HDMI1P4, ""},
	{52725, VDO_TYPE_HDMI1P4, ""},
	{53321, VDO_TYPE_HDMI1P4, ""},
	{53322, VDO_TYPE_HDMI1P4, ""},
	{53323, VDO_TYPE_HDMI1P4, ""},
	{53324, VDO_TYPE_HDMI1P4, ""},
	{53325, VDO_TYPE_HDMI1P4, ""},
};

//case id, in hdmi2.0, vsif file
struct dovi_vsif_type_t dovi_vsif_type[MID_DOVI_TEST_CASE_ID] = {
	{5010, VDO_TYPE_HDMI2P0, "Dolby_VSIF_STD_DoVi.bin"},
	{5011, VDO_TYPE_HDMI2P0, "Dolby_VSIF_STD_DoVi.bin"},
	{5012, VDO_TYPE_HDMI2P0, "Dolby_VSIF_STD_DoVi.bin"},
	{5015, VDO_TYPE_HDMI2P0, "Dolby_VSIF_STD_DoVi.bin"},
	{5051, VDO_TYPE_HDMI2P0, "Dolby_VSIF_LL_DoVi.bin"},
	{50521, VDO_TYPE_HDMI2P0, "Dolby_VSIF_LL_DoVi.bin"},
	{50522, VDO_TYPE_HDMI2P0, "Dolby_VSIF_LL_DoVi.bin"},
	{5054, VDO_TYPE_HDMI2P0, "Dolby_VSIF_LL_DoVi.bin"},
	{5060, VDO_TYPE_HDMI2P0, "Dolby_VSIF_LL_DoVi.bin"},
	{5061, VDO_TYPE_HDMI2P0, "Dolby_VSIF_LL_BT2020_DoVi.bin"},
	{5067, VDO_TYPE_HDMI2P0, "Dolby_VSIF_LL_BT2020_DoVi.bin"},
	{53221, VDO_TYPE_HDMI2P0, "Dolby_VSIF_STD_DoVi.bin"},
	{53222, VDO_TYPE_HDMI2P0, "Dolby_VSIF_STD_DoVi.bin"},
};

//case id, in hdmi2.1, vdif file, vsem file
struct dovi_vsif_vsem_type_t dovi_vsif_vsem_type[MID_DOVI_TEST_CASE_ID] = {
	{5013, VDO_TYPE_HDMI2P1,
	"", "vsem.%02d.bin"},
	{50621, VDO_TYPE_HDMI2P1,
	"", "Dolby_VSEM_LL_BT2020_DoVi.bin"},
	{50622, VDO_TYPE_HDMI2P1,
	"", "Dolby_VSEM_LL_BT2020_DoVi.bin"},
	{50623, VDO_TYPE_HDMI2P1,
	"", "Dolby_VSEM_LL_BT2020_DoVi.bin"},
	{53223, VDO_TYPE_HDMI2P1,
	"Dolby_VSIF_STD_DoVi.bin", "vsem.%02d.bin"},
	{53224, VDO_TYPE_HDMI2P1,
	"Dolby_VSIF_STD_DoVi.bin", "vsem.%02d.bin"},
};

//54**, 1 in type, 1 vsif file, 2 in type, 2 vsif file
struct dovi_multi_vsif_t dovi_multi_vsif[MID_DOVI_TEST_CASE_ID] = {
	{5400, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5401, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5402, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5403, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_OTT, ""},
	{5404, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_HDMI2P0, "Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5405, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_OTT, ""},
	{5406, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5407, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5408, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5409, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_OTT, ""},
	{5410, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_HDMI2P0, "Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5411, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_OTT, ""},
	{54121, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{54122, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5413, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5414, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5415, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5416, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_2.vsif"},
	{5417, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5418, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5419, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5420, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5421, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5422, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5423, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5424, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_OTT, ""},
	{5425, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_HDMI2P0, "Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5426, VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_1.vsif",
	VDO_TYPE_OTT, ""},
	{5427, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5428, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5429, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5430, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5431, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"ArtGlass_1920x1080_24fps_0_23_Video_2.vsif"},
	{5432, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5433, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5434, VDO_TYPE_OTT, "", VDO_TYPE_HDMI2P0,
	"Zion_1920x1080_24fps_500_523_Video_2.vsif"},
	{5435, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5436, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5437, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5439, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5440, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5442, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5443, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5448, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
	{5449, VDO_TYPE_OTT, "", VDO_TYPE_OTT, ""},
};


//case id, rpu file
struct dovi_rpu_type_t dovi_rpu_type[MID_DOVI_TEST_CASE_ID] = {
	{5021, "5021_Zion_23-976fps_3840x2160_90mbps_dav1_10_0.rpu"},
	{5023, "5023_Graffiti_23-976fps_1920x1080_5800kbps_dav1-10-1.rpu"},
	{5024, "5024_ArtGlass_60fps_3840x2160_30mbps_dav1_10_4.rpu"},
	{5025, "5025_Graffiti_23-976fps_1920x1080_10mbps_dav1-10-2.rpu"},
};

//case id, out css
struct dovi_output_css_t dovi_output_css_type[MID_DOVI_TEST_CASE_ID] = {
	{5080, CHROMA_FORMAT_P444},
	{5082, CHROMA_FORMAT_P444},
};

//case id, have gfx, gfx file(same settings :
//--cs rgba --bd 8 --min 50 --max 5000000 --format SDR8 --css p444)
struct dovi_gfx_info_table_t dovi_gfx_info_table[MAX_DOVI_TEST_CASE_ID] = {
	{5000, 0,},
	{5001, 1,
	"/sdcard/dovi/03_colorSquare_FHD_Rec709_Gamma2.2.bgra"},
	{5002, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{5003, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	//idk2.6
	{5005, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{50071, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{50072, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{5007, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{5009, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5010, 0},
	{5011, 0},
	{5012, 0},
	{5013, 0},
	{5015, 0},
	{5020, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{5021, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5022, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{5023, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{5024, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5025, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{5026, 0},
	{5030, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5031, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5032, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5033, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5034, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5035, 1,
	"/sdcard/dovi/DolbyLogo_3840x2160_P444_8b_argb_interleaved.bin"},
	{5036, 1,
	"/sdcard/dovi/DolbyLogo_1920x1080_P444_8b_argb_interleaved.bin"},
	{5040, 0},
	{5041, 0},
	{5042, 0},
	{5043, 0},
	{5044, 0},
	{5045, 0},
	{5046, 0},
	{5047, 0},
	{5048, 0},
	{5049, 0},
	{5051, 0},
	{50521, 0},
	{50522, 0},
	{5054, 0},
	{5060, 0},
	{5061, 0},
	{50621, 0},
	{50622, 0},
	{50623, 0},
	{50641, 0},
	{50642, 0},
	{50643, 0},
	{50644, 0},
	{50645, 0},
	{50646, 0},
	{5065, 0},
	{5067, 0},
	{5080, 0},
	{5081, 0},
	{5082, 0},
	{5083, 0},
	{51001, 0},
	{51002, 0},
	{51003, 0},
	{51004, 0},
	{51005, 0},
	{51006, 0},
	{51007, 0},
	{51008, 0},
	{51009, 0},
	{510010, 0},
	{51011, 0},
	{51012, 0},
	{51013, 0},
	{51014, 0},
	{51015, 0},
	{51016, 0},
	{51017, 0},
	{51018, 0},
	{51019, 0},
	{510110, 0},
	{51021, 0},
	{51022, 0},
	{51023, 0},
	{51024, 0},
	{51025, 0},
	{51026, 0},
	{51027, 0},
	{51028, 0},
	{51029, 0},
	{510210, 0},
	{5106, 0},
	{5200, 0},
	{5201, 0},
	{5203, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5204, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5205, 0},
	{5207, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5210, 0},
	{5211, 0},
	{5213, 1,
	"/sdcard/dovi/Graphic_720x480_P444_8b_SDR_argb_interleaved.bin"},
	{5214, 1,
	"/sdcard/dovi/Graphic_720x480_P444_8b_SDR_argb_interleaved.bin"},
	{5215, 0},
	{5217, 1,
	"/sdcard/dovi/Graphic_720x480_P444_8b_SDR_argb_interleaved.bin"},
	{5220, 0},
	{5221, 0},
	{5223, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5224, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5225, 0},
	{5227, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5230, 0},
	{5231, 0},
	{52321, 0},
	{52322, 0},
	{52323, 0},
	{52324, 0},
	{52325, 0},
	{5233, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5234, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5235, 0},
	{5237, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5240, 0},
	{5241, 0},
	{5243, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5244, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5245, 0},
	{5247, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5250, 0},
	{5251, 0},
	{5253, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5254, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5255, 0},
	{5257, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5260, 0},
	{5261, 0},
	{5263, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5264, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5265, 0},
	{5267, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5270, 0},
	{5271, 0},
	{52721, 0},
	{52722, 0},
	{52723, 0},
	{52724, 0},
	{52725, 0},
	{5273, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5274, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5275, 0},
	{5277, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5280, 0},
	{5281, 0},
	{5283, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5284, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5285, 0},
	{5287, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5290, 0},
	{5291, 0},
	{5293, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5294, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5295, 0},
	{5297, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5320, 0},
	{5321, 0},
	{53221, 0},
	{53222, 0},
	{53223, 0},
	{53224, 0},
	{5323, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5324, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5325, 0},
	{5327, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5330, 0},
	{5331, 0},
	{53321, 0},
	{53322, 0},
	{53323, 0},
	{53324, 0},
	{53325, 0},
	{5333, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5334, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
	{5335, 0},
	{5337, 1,
	"/sdcard/dovi/Graphic_1920x1080_P444_8b_SDR_argb_interleaved.bin"},
};

//have vdo in it, need change
struct dovi_only_gfx_info_t dovi_only_gfx_info[MID_DOVI_TEST_CASE_ID] = {
	{5010, 1,
	"/sdcard/dovi/ArtGlass_scrambled_1920x1080_UYVY_12b.0086756.ipt",
	FORMAT_DOVI, BIT_DEPTH_12, CSS_UYVY},
	{5011, 1,
	"/sdcard/dovi/Teststream_scrambled_3840x2160_UYVY_12b.00000.ipt",
	FORMAT_DOVI, BIT_DEPTH_12, CSS_UYVY},
	{5012, 1,
	"/sdcard/dovi/DolbyVisionModeTest_1920x1080_I444_8b.rgb",
	FORMAT_DOVI, BIT_DEPTH_8, CSS_I444},
	{5013, 1,
	"/sdcard/dovi/Teststream_scrambled_3840x2160_UYVY_12b.00000.ipt",
	FORMAT_DOVI, BIT_DEPTH_12, CSS_UYVY},
	{5015, 1,
	"/sdcard/dovi/TestSet_scrambled_1920x1080_UYVY_12b.00000.ipt",
	FORMAT_DOVI, BIT_DEPTH_12, CSS_UYVY},
	{5051, 1, "/sdcard/dovi/Testset_1920x1080_UYVY_12b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_UYVY},
	{50521, 1, "/sdcard/dovi/Testset_1920x1080_UYVY_12b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_UYVY},
	{50522, 1, "/sdcard/dovi/Testset_1920x1080_UYVY_12b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_UYVY},
	{5054, 1, "/sdcard/dovi/Testset_1920x1080_I444_12b.rgb",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_I444},
	{5060, 1, "/sdcard/dovi/Testset_1920x1080_I444_10b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_10, CSS_I444},
	{5061, 1, "/sdcard/dovi/Testset_1920x1080_UYVY_12b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_UYVY},
	{50621, 1, "/sdcard/dovi/Testset_1920x1080_I444_12b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_I444},
	{50622, 1, "/sdcard/dovi/Testset_1920x1080_I444_12b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_I444},
	{50623, 1, "/sdcard/dovi/Testset_1920x1080_I444_12b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_I444},
	{50641, 1, "/sdcard/dovi/Testset_1920x1080_I444_8b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_8, CSS_I444},
	{50642, 1, "/sdcard/dovi/Testset_1920x1080_I444_8b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_8, CSS_I444},
	{50643, 1, "/sdcard/dovi/Testset_1920x1080_I444_8b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_8, CSS_I444},
	{50644, 1, "/sdcard/dovi/Testset_1920x1080_I444_8b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_8, CSS_I444},
	{50645, 1, "/sdcard/dovi/Testset_1920x1080_I444_8b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_8, CSS_I444},
	{50646, 1, "/sdcard/dovi/Testset_1920x1080_I444_8b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_8, CSS_I444},
	{5065, 1, "/sdcard/dovi/Testset_1920x1080_I444_8b.rgb",
	FORMAT_DOVI_LL, BIT_DEPTH_8, CSS_I444},
	{5067, 1, "/sdcard/dovi/Testset_1920x1080_P420_12b.yuv",
	FORMAT_DOVI_LL, BIT_DEPTH_12, CSS_P420},
	{52321, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HDR.0000.yuv",
	FORMAT_HDR, BIT_DEPTH_10, CSS_UYVY},
	{52322, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HDR.0000.yuv",
	FORMAT_HDR, BIT_DEPTH_10, CSS_UYVY},
	{52323, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HDR.0000.yuv",
	FORMAT_HDR, BIT_DEPTH_10, CSS_UYVY},
	{52324, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HDR.0000.yuv",
	FORMAT_HDR, BIT_DEPTH_10, CSS_UYVY},
	{52325, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HDR.0000.yuv",
	FORMAT_HDR, BIT_DEPTH_10, CSS_UYVY},
	{52721, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_SDR.0000.yuv",
	FORMAT_SDR, BIT_DEPTH_10, CSS_UYVY},
	{52722, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_SDR.0000.yuv",
	FORMAT_SDR, BIT_DEPTH_10, CSS_UYVY},
	{52723, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_SDR.0000.yuv",
	FORMAT_SDR, BIT_DEPTH_10, CSS_UYVY},
	{52724, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_SDR.0000.yuv",
	FORMAT_SDR, BIT_DEPTH_10, CSS_UYVY},
	{52725, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_SDR.0000.yuv",
	FORMAT_SDR, BIT_DEPTH_10, CSS_UYVY},
	{53221, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_12b_scrambled.0000.ipt",
	FORMAT_DOVI, BIT_DEPTH_12, CSS_UYVY},
	{53222, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_12b_scrambled.0000.ipt",
	FORMAT_DOVI, BIT_DEPTH_12, CSS_UYVY},
	{53223, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_12b_scrambled.0000.ipt",
	FORMAT_DOVI, BIT_DEPTH_12, CSS_UYVY},
	{53224, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_12b_scrambled.0000.ipt",
	FORMAT_DOVI, BIT_DEPTH_12, CSS_UYVY},
	{53321, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HLG.0000.yuv",
	FORMAT_HLG, BIT_DEPTH_10, CSS_UYVY},
	{53322, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HLG.0000.yuv",
	FORMAT_HLG, BIT_DEPTH_10, CSS_UYVY},
	{53323, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HLG.0000.yuv",
	FORMAT_HLG, BIT_DEPTH_10, CSS_UYVY},
	{53324, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HLG.0000.yuv",
	FORMAT_HLG, BIT_DEPTH_10, CSS_UYVY},
	{53325, 1,
	"/sdcard/dovi/TestSet_1920x1080_UYVY_10b_HLG.0000.yuv",
	FORMAT_HLG, BIT_DEPTH_10, CSS_UYVY},
};

//54 case : video layer 0, gfx layer 0, gfx layer 1
//gfx1: case id, gfx file, alpha file, in format, md file
//sdr: --bd 8 --cs rgba; hdr: --css p444 --bd 10 --cs yuv; dovi: --css p444
struct dovi_pip_gfx_info_t dovi_pip_gfx1_info[MID_DOVI_TEST_CASE_ID] = {
	{5400, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5401, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5402, "/sdcard/dovi/Graphic_1920x1080_P444_8b_vuya.bin",
	FORMAT_DOVI,
	"/sdcard/dovi/Graphic_1920x1080_P444_12b_PQ_Graphic_1.md"},
	{5403, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5404, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5405, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5406, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5407, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5408, "/sdcard/dovi/Graphic_1920x1080_P444_8b_vuya.bin",
	FORMAT_DOVI,
	"/sdcard/dovi/Graphic_1920x1080_P444_12b_PQ_Graphic_1.md"},
	{5409, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5410, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5411, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{54121, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{54122, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5413, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5414, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5415, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5416, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5417, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5418, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5419, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5420, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5421, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5422, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5423, "/sdcard/dovi/Graphic_1920x1080_P444_8b_vuya.bin",
	FORMAT_DOVI,
	"/sdcard/dovi/Graphic_1920x1080_P444_12b_PQ_Graphic_1.md"},
	{5424, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5425, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5426, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5427, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5428, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5429, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5430, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5431, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5432, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5433, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5434, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5435, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5436, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5437, "/sdcard/dovi/Graphic_1920x1080_P444_8b_vuya.bin",
	FORMAT_DOVI,
	"/sdcard/dovi/Graphic_1920x1080_P444_12b_PQ_Graphic_1.md"},
	{5439, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5440, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5442, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5443, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5448, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5449, "/sdcard/dovi/Graphic_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
};

//gfx2: case id, gfx file, alpha file, in format, md file
//sdr8: --bd 8 --cs rgba; hdr10: --css p444 --bd 10 --cs yuv; dovi: --css p444
struct dovi_pip_gfx_info_t dovi_pip_gfx2_info[MID_DOVI_TEST_CASE_ID] = {
	{5400, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5401, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5402, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_vuya.bin",
	FORMAT_HDR, ""},
	{5403, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5404, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5405, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5406, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5407, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5408, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_vuya.bin",
	FORMAT_HDR, ""},
	{5409, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5410, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5411, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{54121, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{54122, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5413, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5414, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5415, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5416, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5417, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5418, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5419, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5420, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5421, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5422, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5423, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_vuya.bin",
	FORMAT_HDR, ""},
	{5424, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5425, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5426, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5427, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5428, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5429, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5430, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5431, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5432, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5433, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5434, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5435, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5436, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5437, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_vuya.bin",
	FORMAT_HDR, ""},
	{5439, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5440, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5442, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5443, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5448, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
	{5449, "/sdcard/dovi/TransportBar_1920x1080_P444_8b_bgra.bin",
	FORMAT_SDR, ""},
};

//not need
struct dovi2hdr10_out_map_table_t dovi2hdr10_out_mapping[150] = {
	{1, 0},
	{2, 1},
};
struct dovi_graphic_info_table_t dovi_graphic_info[MAX_DOVI_TEST_CASE_ID] = {
	{1, "/sdcard/dovi/01_colorBar_FHD_Rec709_Gamma2.2.rgba",
	1, V_PRIORITY, GRAPHIC_SDR_RGB},
	{2, "/sdcard/dovi/03_colorSquare_FHD_Rec709_Gamma2.2.rgba",
	1, G_PRIORITY, GRAPHIC_SDR_RGB},
};
char dovi_graphic_name[][MAX_FILE_NAME_LEN] = {
	"/sdcard/dovi/03_colorSquare_FHD_Rec709_Gamma2.2.bgra",
	"/sdcard/dovi/04_colorSquare_UHD_Rec709_Gamma2.2.bgra",
};
uint32_t get_dovi2hdr10_mapping_type(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < 150; idx++) {
		if (test_case_id
			== dovi2hdr10_out_mapping[idx].test_case_id)
			return dovi2hdr10_out_mapping[idx].dovi2hdr10_mapping;
	}

	return 0;
}
uint32_t get_dovi2hdr10_mapping_type_Status(void)
{
	uint32_t idx = 0;

	for (idx = 0; idx < 150; idx++) {
		dovi_default("%d, %d\n",
			     dovi2hdr10_out_mapping[idx].test_case_id,
			     dovi2hdr10_out_mapping[idx].dovi2hdr10_mapping);
	}

	return 0;
}
uint32_t set_dovi2hdr10_mapping_type(uint32_t test_case_id,
	uint32_t dovi2hdr10_mapping)
{
	uint32_t idx = 0;

	for (idx = 0; idx < 150; idx++) {
		if (test_case_id == dovi_out_format_tbl[idx].test_case_id) {
			dovi2hdr10_out_mapping[idx].dovi2hdr10_mapping =
				dovi2hdr10_mapping;
			return dovi2hdr10_out_mapping[idx].dovi2hdr10_mapping;
		}
	}

	return 0;
}
int get_dovi_graphic_on(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_graphic_info[idx].test_case_id)
			return dovi_graphic_info[idx].f_graphic_on;
	}

	return 0;
}
enum graphic_format_t get_dovi_g_format(uint32_t test_case_id)
{
	uint32_t idx = 0;

	if (force_g_format)
		return (enum graphic_format_t)g_format;
	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_graphic_info[idx].test_case_id)
			return dovi_graphic_info[idx].g_format;
	}

	return GRAPHIC_SDR_RGB;
}


uint32_t get_dovi_out_format(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_out_format_tbl[idx].test_case_id)
			return dovi_out_format_tbl[idx].out_format;
	}

	return OUT_FORMAT_SDR8;
}

uint32_t get_dovi_out_format_status(void)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		dovi_default("%d, %d\n",
			     dovi_out_format_tbl[idx].test_case_id,
			     dovi_out_format_tbl[idx].out_format);
	}

	return 0;
}

uint32_t set_dovi_out_format(uint32_t test_case_id,
	uint32_t out_format)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_out_format_tbl[idx].test_case_id) {
			dovi_out_format_tbl[idx].out_format = out_format;

			return dovi_out_format_tbl[idx].out_format;
		}
	}

	return OUT_FORMAT_SDR8;
}

char *get_dovi_vsvdb_file_name(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_case_info[idx].test_case_id)
			return dovi_case_info[idx].vsvdb_file_name;
	}

	return 0;
}

uint32_t get_dovi_use_ll(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_case_info[idx].test_case_id)
			return dovi_case_info[idx].use_ll;
	}

	return 0;
}

uint32_t get_dovi_ll_rgb_desired(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_case_info[idx].test_case_id)
			return dovi_case_info[idx].ll_rgb_desired;
	}

	return 0;
}

uint32_t set_dovi_ll_mode(uint32_t test_case_id,
	uint32_t use_ll, uint32_t ll_rgb_desired)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_case_info[idx].test_case_id) {
			dovi_case_info[idx].use_ll = use_ll;
			dovi_case_info[idx].ll_rgb_desired =
				ll_rgb_desired;

			return dovi_case_info[idx].use_ll;
		}
	}

	return 0;
}

uint32_t get_dovi_ll_mode_status(void)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		dovi_default("[%4d]  %d %d\n",
			     dovi_case_info[idx].test_case_id,
			     dovi_case_info[idx].use_ll,
			     dovi_case_info[idx].ll_rgb_desired);
	}

	return 0;
}

void set_dovi_priority_mode(uint32_t force_pri_mode,
	uint32_t pri_mode)
{
	force_priority_mode = force_pri_mode;
	priority_mode = pri_mode;
}

void set_dovi_g_format(uint32_t force_gformat,
	uint32_t gformat)
{
	force_g_format = force_gformat;
	g_format = gformat;
}

enum disp_mode_t get_dovi_disp_mode(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_mode_priority[idx].test_case_id)
			return dovi_mode_priority[idx].disp_mode;
	}

	return NORMAL_MODE;
}

enum pri_mode_t get_dovi_priority_mode(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_mode_priority[idx].test_case_id)
			return dovi_mode_priority[idx].priority_mode;
	}

	return V_PRIORITY;
}

enum vdo_inout_type_t get_dovi_input_type(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_vdo_inout_type[idx].test_case_id)
			return dovi_vdo_inout_type[idx].vdo_in_type;
	}

	return VDO_TYPE_OTT;
}

enum vdo_inout_type_t get_dovi_output_type(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_vdo_inout_type[idx].test_case_id)
			return dovi_vdo_inout_type[idx].vdo_out_type;
	}

	return VDO_TYPE_OTT;
}

char *get_dovi_drm_file_name(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_drm_type[idx].test_case_id)
			return dovi_drm_type[idx].drm_file_name;
	}

	return 0;
}

char *get_dovi_vsif_file_name(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_vsif_type[idx].test_case_id)
			return dovi_vsif_type[idx].vsif_file_name;
	}

	return 0;
}

char *get_dovi_vsif_vsem_name1(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_vsif_vsem_type[idx].test_case_id)
			return dovi_vsif_vsem_type[idx].vsif_file_name;
	}

	return 0;
}

char *get_dovi_vsif_vsem_name2(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_vsif_vsem_type[idx].test_case_id)
			return dovi_vsif_vsem_type[idx].vsem_file_name;
	}

	return 0;
}

enum vdo_inout_type_t get_multi_input_type1(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_multi_vsif[idx].test_case_id)
			return dovi_multi_vsif[idx].vdo_in_type1;
	}

	return VDO_TYPE_OTT;
}

enum vdo_inout_type_t get_multi_input_type2(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_multi_vsif[idx].test_case_id)
			return dovi_multi_vsif[idx].vdo_in_type2;
	}

	return VDO_TYPE_OTT;
}

char *get_dovi_multi_vsif1(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_multi_vsif[idx].test_case_id)
			return dovi_multi_vsif[idx].vsif_file_name1;
	}

	return 0;
}

char *get_dovi_multi_vsif2(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_multi_vsif[idx].test_case_id)
			return dovi_multi_vsif[idx].vsif_file_name2;
	}

	return 0;
}

char *get_dovi_rpu_name(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id == dovi_rpu_type[idx].test_case_id)
			return dovi_rpu_type[idx].rpu_file_name;
	}

	return 0;
}

enum chroma_format_t get_dovi_out_css(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_output_css_type[idx].test_case_id)
			return dovi_output_css_type[idx].out_css;
	}

	return CHROMA_FORMAT_UYVY;
}

int get_dovi_gfx_on_info(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_gfx_info_table[idx].test_case_id)
			return dovi_gfx_info_table[idx].is_graphic_on;
	}

	return 0;
}

char *get_dovi_gfx_file_info(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MAX_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_gfx_info_table[idx].test_case_id)
			return dovi_gfx_info_table[idx].graphic_file_name;
	}

	return 0;
}


int get_dovi_is_only_gfx_test(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_only_gfx_info[idx].test_case_id)
			return dovi_only_gfx_info[idx].is_only_gfx;
	}

	return 0;
}

char *get_dovi_only_gfx_file_info(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_only_gfx_info[idx].test_case_id)
			return dovi_only_gfx_info[idx].gfx_file_name;
	}

	return 0;
}

enum input_format_t get_dovi_only_gfx_input_format(
	uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_only_gfx_info[idx].test_case_id)
			return dovi_only_gfx_info[idx].input_format;
	}

	return 0;
}

enum input_BD_t get_dovi_only_gfx_input_bd(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_only_gfx_info[idx].test_case_id)
			return dovi_only_gfx_info[idx].input_BD;
	}

	return 0;
}

enum input_css_t get_dovi_only_gfx_in_css(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_only_gfx_info[idx].test_case_id)
			return dovi_only_gfx_info[idx].in_css;
	}

	return 0;
}

char *get_dovi_pip_gfx1_gfx_file(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_pip_gfx1_info[idx].test_case_id)
			return dovi_pip_gfx1_info[idx].gfx_file_name;
	}

	return 0;
}

char *get_dovi_pip_gfx1_md(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_pip_gfx1_info[idx].test_case_id)
			return dovi_pip_gfx1_info[idx].md_file_name;
	}

	return 0;
}

enum input_format_t get_dovi_pip_gfx1_input_format(
	uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_pip_gfx1_info[idx].test_case_id)
			return dovi_pip_gfx1_info[idx].input_format;
	}

	return 0;
}

char *get_dovi_pip_gfx1_md_file(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_pip_gfx1_info[idx].test_case_id)
			return dovi_pip_gfx1_info[idx].md_file_name;
	}

	return 0;
}

char *get_dovi_pip_gfx2_gfx_file(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_pip_gfx2_info[idx].test_case_id)
			return dovi_pip_gfx2_info[idx].gfx_file_name;
	}

	return 0;
}

enum input_format_t get_dovi_pip_gfx2_input_format(
	uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_pip_gfx2_info[idx].test_case_id)
			return dovi_pip_gfx2_info[idx].input_format;
	}

	return 0;
}

char *get_dovi_pip_gfx2_md_file(uint32_t test_case_id)
{
	uint32_t idx = 0;

	for (idx = 0; idx < MID_DOVI_TEST_CASE_ID; idx++) {
		if (test_case_id ==
			dovi_pip_gfx2_info[idx].test_case_id)
			return dovi_pip_gfx2_info[idx].md_file_name;
	}

	return 0;
}

