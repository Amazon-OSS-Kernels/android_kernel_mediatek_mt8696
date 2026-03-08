// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Fish Wu <fish.wu@mediatek.com>
 *
 */

#include "mtk_aie.h"
#include <linux/delay.h>
#include <linux/firmware.h>

static const unsigned int fd_wdma_en[fd_loop_num][output_WDMA_WRA_num] = {
	{1, 0, 0, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 0, 0}, {1, 1, 1, 1},
	{1, 1, 1, 1}, {1, 0, 0, 0}, {1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1},
	{1, 0, 0, 0}, {1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 1, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 1, 0},
	{1, 0, 1, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 0, 0, 0},
	{1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 1, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0} };

static const unsigned int out_stride_size[fd_loop_num][output_WDMA_WRA_num] = {
	{1, 0, 0, 0}, {1, 0, 2, 0}, {1, 0, 2, 0}, {1, 0, 0, 0}, {1, 1, 2, 2},
	{1, 1, 2, 2}, {1, 0, 0, 0}, {1, 0, 2, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 0, 2, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {3, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 2, 0}, {1, 0, 2, 0}, {1, 0, 0, 0}, {1, 1, 2, 2}, {1, 1, 2, 2},
	{1, 0, 0, 0}, {1, 0, 2, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 2, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {3, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 2, 0},
	{1, 0, 2, 0}, {1, 0, 0, 0}, {1, 1, 2, 2}, {1, 1, 2, 2}, {1, 0, 0, 0},
	{1, 0, 2, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 2, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {3, 0, 0, 0} };

static const unsigned int fd_ker_rdma_size[fd_loop_num][kernel_RDMA_RA_num] = {
	{240, 240},   {1168, 1168}, {1168, 1168}, {272, 272},   {2320, 2320},
	{2080, 2080}, {1040, 1040}, {4624, 4624}, {3104, 3104}, {9232, 9232},
	{4624, 4624}, {4128, 4128}, {1040, 1040}, {4624, 4624}, {4624, 4624},
	{1552, 1552}, {4624, 4624}, {4624, 4624}, {4128, 4128}, {1040, 1040},
	{1040, 1040}, {528, 528},   {4160, 4160}, {4160, 4160}, {2080, 2080},
	{2080, 2080}, {2080, 2080}, {1040, 1040}, {0, 0},       {240, 240},
	{1168, 1168}, {1168, 1168}, {272, 272},   {2320, 2320}, {2080, 2080},
	{1040, 1040}, {4624, 4624}, {3104, 3104}, {9232, 9232}, {4624, 4624},
	{4128, 4128}, {1040, 1040}, {4624, 4624}, {4624, 4624}, {1552, 1552},
	{4624, 4624}, {4624, 4624}, {4128, 4128}, {1040, 1040}, {1040, 1040},
	{528, 528},   {4160, 4160}, {4160, 4160}, {2080, 2080}, {2080, 2080},
	{2080, 2080}, {1040, 1040}, {0, 0},       {240, 240},   {1168, 1168},
	{1168, 1168}, {272, 272},   {2320, 2320}, {2080, 2080}, {1040, 1040},
	{4624, 4624}, {3104, 3104}, {9232, 9232}, {4624, 4624}, {4128, 4128},
	{1040, 1040}, {4624, 4624}, {4624, 4624}, {1552, 1552}, {4624, 4624},
	{4624, 4624}, {4128, 4128}, {1040, 1040}, {1040, 1040}, {528, 528},
	{4160, 4160}, {4160, 4160}, {2080, 2080}, {2080, 2080}, {2080, 2080},
	{1040, 1040}, {0, 0} };

static const unsigned int fd_out_stride2_in[fd_loop_num] = {
	0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const unsigned int fd_stride[fd_loop_num] = {
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

static const unsigned int fd_maxpool[fd_loop_num] = {
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const unsigned int out_2size[fd_loop_num] = {
	0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const unsigned int in_ch_pack[fd_loop_num] = {
	1, 16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 0,  1,  16, 16, 16, 16, 16, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 0,  1,  16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 0};

static const unsigned int outlayer[fd_loop_num] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0};
static const unsigned int out_ch_pack[fd_loop_num] = {
	16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 16, 16, 16, 32, 32, 32, 32, 32, 32, 0,  16,
	16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 16, 16, 16, 32, 32, 32, 32, 32, 32, 0,  16, 16,
	16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 16, 16, 16, 32, 32, 32, 32, 32, 32, 0};

static const unsigned int anchor_en_num[fd_loop_num] = {
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5,  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

/* [loop][ch][output_index] */
static const signed int fd_rdma_en[fd_loop_num][input_WDMA_WRA_num][2] = {
	{{99, 99}, {99, 99}, {99, 99}, {-1, -1} },
	{{0, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{1, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{1, 0}, {2, 0}, {-1, -1}, {-1, -1} },
	{{3, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{1, 2}, {2, 2}, {4, 2}, {4, 3} },
	{{5, 0}, {5, 1}, {-1, -1}, {-1, -1} },
	{{6, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{5, 0}, {5, 1}, {7, 0}, {-1, -1} },
	{{8, 0}, {8, 1}, {-1, -1}, {-1, -1} },
	{{9, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{5, 2}, {5, 3}, {7, 2}, {10, 2} },
	{{11, 0}, {11, 1}, {-1, -1}, {-1, -1} },
	{{12, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{13, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{11, 0}, {11, 1}, {14, 0}, {-1, -1} },
	{{15, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{16, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{11, 0}, {11, 1}, {14, 0}, {17, 0} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{19, 0}, {22, 0}, {22, 1}, {25, 0} },
	{{99, 99}, {99, 99}, {99, 99}, {-1, -1} },
	{{29, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{30, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{30, 0}, {31, 0}, {-1, -1}, {-1, -1} },
	{{32, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{30, 2}, {31, 2}, {33, 2}, {33, 3} },
	{{34, 0}, {34, 1}, {-1, -1}, {-1, -1} },
	{{35, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{34, 0}, {34, 1}, {36, 0}, {-1, -1} },
	{{37, 0}, {37, 1}, {-1, -1}, {-1, -1} },
	{{38, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{34, 2}, {34, 3}, {36, 2}, {39, 2} },
	{{40, 0}, {40, 1}, {-1, -1}, {-1, -1} },
	{{41, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{42, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{40, 0}, {40, 1}, {43, 0}, {-1, -1} },
	{{44, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{45, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{40, 0}, {40, 1}, {43, 0}, {46, 0} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{48, 0}, {51, 0}, {51, 1}, {54, 0} },
	{{99, 99}, {99, 99}, {99, 99}, {-1, -1} },
	{{58, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{59, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{59, 0}, {60, 0}, {-1, -1}, {-1, -1} },
	{{61, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{59, 2}, {60, 2}, {62, 2}, {62, 3} },
	{{63, 0}, {63, 1}, {-1, -1}, {-1, -1} },
	{{64, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{63, 0}, {63, 1}, {65, 0}, {-1, -1} },
	{{66, 0}, {66, 1}, {-1, -1}, {-1, -1} },
	{{67, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{63, 2}, {63, 3}, {65, 2}, {68, 2} },
	{{69, 0}, {69, 1}, {-1, -1}, {-1, -1} },
	{{70, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{71, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{69, 0}, {69, 1}, {72, 0}, {-1, -1} },
	{{73, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{74, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{69, 0}, {69, 1}, {72, 0}, {75, 0} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{77, 0}, {80, 0}, {80, 1}, {83, 0} } };

static const unsigned int attr_wdma_en[attr_loop_num][output_WDMA_WRA_num] = {
	{1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1},
	{1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0} };
static const unsigned int
	attr_ker_rdma_size[attr_loop_num][kernel_RDMA_RA_num] = {
		{240, 240},   {1168, 1168}, {272, 272},   {2320, 2320},
		{2080, 2080}, {9232, 9232}, {3104, 3104}, {9232, 9232},
		{4128, 4128}, {1040, 1040}, {4624, 4624}, {4624, 4624},
		{1552, 1552}, {4624, 4624}, {4624, 4624}, {4128, 4128},
		{9232, 9232}, {272, 272},   {9232, 9232}, {2320, 2320},
		{144, 144},   {9232, 9232}, {272, 272},   {9232, 9232},
		{2320, 2320}, {144, 144} };
static const unsigned int attr_out_stride2_as_in[attr_loop_num] = {
	0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const unsigned int attr_fd_stride[attr_loop_num] = {/* H */
							   2, 1, 1, 1, 1, 1, 1,
							   1, 1, 1, 1, 1, 1, 1,
							   1, 1, 1, 1, 1, 1, 1,
							   1, 1, 1, 1, 1};
static const unsigned int attr_fd_maxpool[attr_loop_num] = {/* L */
							    1, 0, 0, 0, 0, 0, 0,
							    0, 0, 0, 0, 0, 0, 0,
							    0, 0, 0, 0, 0, 0, 0,
							    0, 0, 0, 0, 0};
static const unsigned int attr_out_2size[attr_loop_num] = {/* O */
							   1, 1, 0, 1, 1, 1, 0,
							   1, 0, 0, 0, 0, 0, 0,
							   0, 0, 0, 0, 0, 0, 0,
							   0, 0, 0, 0, 0};
static const unsigned int attr_input_ch_pack[attr_loop_num] = {
	1,  16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 16, 32, 32, 32, 32, 16};
/* [loop][ch][output_index] */
static const signed int attr_rdma_en[attr_loop_num][input_WDMA_WRA_num][2] = {
	{{99, 99}, {99, 99}, {99, 99}, {-1, -1} },
	{{0, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{0, 0}, {1, 0}, {-1, -1}, {-1, -1} },
	{{2, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{0, 2}, {1, 2}, {3, 2}, {3, 3} },
	{{4, 0}, {4, 1}, {-1, -1}, {-1, -1} },
	{{4, 0}, {4, 1}, {5, 0}, {-1, -1} },
	{{6, 0}, {6, 1}, {-1, -1}, {-1, -1} },
	{{4, 2}, {4, 3}, {5, 2}, {7, 2} },
	{{8, 0}, {8, 1}, {-1, -1}, {-1, -1} },
	{{9, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{10, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{8, 0}, {8, 1}, {11, 0}, {-1, -1} },
	{{12, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{13, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{8, 0}, {8, 1}, {11, 0}, {14, 0} },
	{{15, 0}, {15, 1}, {-1, -1}, {-1, -1} },
	{{16, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{15, 0}, {15, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{19, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{15, 0}, {15, 1}, {-1, -1}, {-1, -1} },
	{{21, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{15, 0}, {15, 1}, {-1, -1}, {-1, -1} },
	{{23, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{24, 0}, {-1, -1}, {-1, -1}, {-1, -1} } };

static const unsigned int attr_wdma_size[attr_loop_num][output_WDMA_WRA_num] = {
	{16384, 0, 4096, 0},
	{16384, 0, 4096, 0},
	{16384, 0, 0, 0},
	{16384, 16384, 4096, 4096},
	{8192, 8192, 2048, 2048},
	{8192, 0, 2048, 0},
	{8192, 8192, 0, 0},
	{8192, 0, 2048, 0},
	{2048, 2048, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 2048, 0, 0},
	{2048, 0, 0, 0},
	{0, 0, 0, 0},
	{2048, 0, 0, 0},
	{1024, 0, 0, 0},
	{0, 0, 0, 0},
	{2048, 0, 0, 0},
	{0, 0, 0, 0},
	{2048, 0, 0, 0},
	{1024, 0, 0, 0},
	{0, 0, 0, 0} };

static const unsigned int fld_step_align_size[fld_step_num][fld_max_frame] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6528},
	{1536, 1280, 1280, 1280, 1280, 1280, 1280, 1280,
	 1280, 1280, 1280, 1280, 1280, 1280, 1280},
	{5376, 5376, 5376, 5376, 5376, 5376, 5376, 5376,
	 5376, 5376, 5376, 5376, 5376, 5376, 5376},
	{307200, 307200, 307200, 307200, 307200, 307200, 307200, 307200,
	 307200, 307200, 307200, 307200, 307200, 307200, 307200},
	{8064, 8064, 8064, 8064, 8064, 8064, 8064, 8064,
	 8064, 8064, 8064, 8064, 8064, 8064, 8064},
	{8064, 8064, 8064, 8064, 8064, 8064, 8064, 8064,
	 8064, 8064, 8064, 8064, 8064, 8064, 8064}
};

static const unsigned int fld_face_info_0[fld_max_frame] = {
	FLD_INFO_0_FACE_0, FLD_INFO_0_FACE_1, FLD_INFO_0_FACE_2,
	FLD_INFO_0_FACE_3, FLD_INFO_0_FACE_4, FLD_INFO_0_FACE_5,
	FLD_INFO_0_FACE_6, FLD_INFO_0_FACE_7, FLD_INFO_0_FACE_8,
	FLD_INFO_0_FACE_9, FLD_INFO_0_FACE_10, FLD_INFO_0_FACE_11,
	FLD_INFO_0_FACE_12, FLD_INFO_0_FACE_13, FLD_INFO_0_FACE_14
};

static const unsigned int fld_face_info_1[fld_max_frame] = {
	FLD_INFO_1_FACE_0, FLD_INFO_1_FACE_1, FLD_INFO_1_FACE_2,
	FLD_INFO_1_FACE_3, FLD_INFO_1_FACE_4, FLD_INFO_1_FACE_5,
	FLD_INFO_1_FACE_6, FLD_INFO_1_FACE_7, FLD_INFO_1_FACE_8,
	FLD_INFO_1_FACE_9, FLD_INFO_1_FACE_10, FLD_INFO_1_FACE_11,
	FLD_INFO_1_FACE_12, FLD_INFO_1_FACE_13, FLD_INFO_1_FACE_14
};

static const unsigned int fld_face_info_2[fld_max_frame] = {
	FLD_INFO_2_FACE_0, FLD_INFO_2_FACE_1, FLD_INFO_2_FACE_2,
	FLD_INFO_2_FACE_3, FLD_INFO_2_FACE_4, FLD_INFO_2_FACE_5,
	FLD_INFO_2_FACE_6, FLD_INFO_2_FACE_7, FLD_INFO_2_FACE_8,
	FLD_INFO_2_FACE_9, FLD_INFO_2_FACE_10, FLD_INFO_2_FACE_11,
	FLD_INFO_2_FACE_12, FLD_INFO_2_FACE_13, FLD_INFO_2_FACE_14
};

static int aie_imem_alloc(struct mtk_aie_dev *fd, u32 size,
			  struct imem_buf_info *bufinfo)
{
	struct device *dev = fd->dev;
	void *va;
	dma_addr_t dma_handle;

	fd->fd_mem_size += size;

	va = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);
	if (!va)
		return -ENOMEM;

	bufinfo->va = va;
	bufinfo->pa = dma_handle;
	bufinfo->size = size;

	dev_dbg(fd->dev, "%s: vAddr(0x%p), pAddr(0x%pad), size(%d)\n",
		 __func__, va, &dma_handle, size);

	return 0;
}

static void aie_imem_free(struct mtk_aie_dev *fd, struct imem_buf_info *bufinfo)
{
	dev_dbg(fd->dev, "%s: vAddr(0x%p), pAddr(0x%pad), size(%d)\n",
		 __func__, bufinfo->va, &(bufinfo->pa),
		 bufinfo->size);

	dma_free_coherent(fd->dev, bufinfo->size, bufinfo->va, bufinfo->pa);
}

static void aie_init_table(struct mtk_aie_dev *fd, u16 pym_width,
			   u16 pym_height)
{
	int i;
	struct aie_static_info *pstv;

	pstv = &fd->st_info;

	pstv->img_width[pym2_start_loop] = pym_width / 4;
	pstv->img_height[pym2_start_loop] = pym_height / 4;

	pstv->img_width[pym1_start_loop] = pym_width / 2;
	pstv->img_height[pym1_start_loop] = pym_height / 2;

	pstv->img_width[pym0_start_loop] = pym_width;
	pstv->img_height[pym0_start_loop] = pym_height;

	for (i = 0; i < fd_loop_num; i++) {
		if (i != pym2_start_loop && i != pym1_start_loop &&
		    i != pym0_start_loop) {
			if (fd_out_stride2_in[i] == 1) {
				pstv->img_width[i] =
					pstv->stride2_out_width[i - 1];
				pstv->img_height[i] =
					pstv->stride2_out_height[i - 1];
			} else {
				pstv->img_width[i] = pstv->out_width[i - 1];
				pstv->img_height[i] = pstv->out_height[i - 1];
			}
		}

		if (fd_maxpool[i] == 1 && fd_stride[i] == 1) {
			pstv->out_width[i] =
				(pstv->img_width[i] - 1) /
				(2 * fd_maxpool[i]) + 1;
			pstv->out_height[i] = (pstv->img_height[i] - 1) /
					      (2 * fd_maxpool[i]) + 1;
		} else {
			pstv->out_width[i] =
				(pstv->img_width[i] - 1) /
					(fd_stride[i] + 2 * fd_maxpool[i]) + 1;
			pstv->out_height[i] =
				(pstv->img_height[i] - 1) /
					(fd_stride[i] + 2 * fd_maxpool[i]) + 1;
		}

		pstv->stride2_out_width[i] =
			((pstv->out_width[i] - 1) / 2 + 1) * out_2size[i];
		pstv->stride2_out_height[i] =
			((pstv->out_height[i] - 1) / 2 + 1) * out_2size[i];

		if (outlayer[i] == 1) {
			pstv->out_xsize_plus_1[i] =
				pstv->out_width[i] * out_ch_pack[i] * 2;
			pstv->out_stride[i] = round_up(
				pstv->out_xsize_plus_1[i] * anchor_en_num[i],
				16);
			pstv->out_xsize_plus_1_stride2[i] =
				((pstv->out_width[i] - 1) / 2 + 1) *
				out_ch_pack[i] * 2 * out_2size[i];
		} else {
			pstv->out_xsize_plus_1[i] =
				pstv->out_width[i] * out_ch_pack[i];
			pstv->out_stride[i] =
				round_up(pstv->out_xsize_plus_1[i], 16);
			pstv->out_xsize_plus_1_stride2[i] =
				((pstv->out_width[i] - 1) / 2 + 1) *
				out_ch_pack[i] * out_2size[i];
		}

		pstv->out_stride_stride2[i] =
			round_up(pstv->out_xsize_plus_1_stride2[i], 16);

		if (out_2size[i] == 1)
			pstv->out_ysize_plus_1_stride2[i] =
				(pstv->out_height[i] - 1) / 2 + 1;
		else
			pstv->out_ysize_plus_1_stride2[i] = pstv->out_height[i];

		if (fd_wdma_en[i][0]) {
			if (i == rpn2_loop_num || i == rpn1_loop_num ||
			    i == rpn0_loop_num) {
				pstv->fd_wdma_size[i][0] = result_size;
			} else {
				pstv->fd_wdma_size[i][0] = pstv->out_height[i] *
							   pstv->out_stride[i];
			}
		}

		if (outlayer[i] == 1) {
			if (fd_wdma_en[i][1])
				pstv->fd_wdma_size[i][1] =
					pstv->fd_wdma_size[i][0];
			if (fd_wdma_en[i][2])
				pstv->fd_wdma_size[i][2] =
					pstv->fd_wdma_size[i][0];
			if (fd_wdma_en[i][3])
				pstv->fd_wdma_size[i][3] =
					pstv->fd_wdma_size[i][0];
		} else if (i == rpn2_loop_num || i == rpn1_loop_num ||
			   i == rpn0_loop_num) {
			pstv->fd_wdma_size[i][0] = result_size;
		} else {
			if (fd_wdma_en[i][1])
				pstv->fd_wdma_size[i][1] = pstv->out_height[i] *
							   pstv->out_stride[i];
			if (fd_wdma_en[i][2])
				pstv->fd_wdma_size[i][2] =
					pstv->out_ysize_plus_1_stride2[i] *
					pstv->out_stride_stride2[i];
			if (fd_wdma_en[i][3])
				pstv->fd_wdma_size[i][3] =
					pstv->out_ysize_plus_1_stride2[i] *
					pstv->out_stride_stride2[i];
		}

		if (in_ch_pack[i] == 1)
			pstv->input_xsize_plus_1[i] =
				round_up(pstv->img_width[i], 8);
		else
			pstv->input_xsize_plus_1[i] =
				pstv->img_width[i] * in_ch_pack[i];
	}
}

static void aie_update_table(struct mtk_aie_dev *fd, u16 pym_width,
			   u16 pym_height)
{
	int i;
	struct aie_static_info *pstv;

	pstv = &fd->st_info;

	pstv->img_width[pym2_start_loop] = pym_width / 4;
	pstv->img_height[pym2_start_loop] = pym_height / 4;

	pstv->img_width[pym1_start_loop] = pym_width / 2;
	pstv->img_height[pym1_start_loop] = pym_height / 2;

	pstv->img_width[pym0_start_loop] = pym_width;
	pstv->img_height[pym0_start_loop] = pym_height;

	for (i = 0; i < fd_loop_num; i++) {
		if (i != pym2_start_loop && i != pym1_start_loop &&
		    i != pym0_start_loop) {
			if (fd_out_stride2_in[i] == 1) {
				pstv->img_width[i] =
					pstv->stride2_out_width[i - 1];
				pstv->img_height[i] =
					pstv->stride2_out_height[i - 1];
			} else {
				pstv->img_width[i] = pstv->out_width[i - 1];
				pstv->img_height[i] = pstv->out_height[i - 1];
			}
		}

		if (fd_maxpool[i] == 1 && fd_stride[i] == 1) {
			pstv->out_width[i] =
				(pstv->img_width[i] - 1) /
				(2 * fd_maxpool[i]) + 1;
			pstv->out_height[i] = (pstv->img_height[i] - 1) /
					      (2 * fd_maxpool[i]) + 1;
		} else {
			pstv->out_width[i] =
				(pstv->img_width[i] - 1) /
					(fd_stride[i] + 2 * fd_maxpool[i]) + 1;
			pstv->out_height[i] =
				(pstv->img_height[i] - 1) /
					(fd_stride[i] + 2 * fd_maxpool[i]) + 1;
		}

		pstv->stride2_out_width[i] =
			((pstv->out_width[i] - 1) / 2 + 1) * out_2size[i];
		pstv->stride2_out_height[i] =
			((pstv->out_height[i] - 1) / 2 + 1) * out_2size[i];

		if (outlayer[i] == 1) {
			pstv->out_xsize_plus_1[i] =
				pstv->out_width[i] * out_ch_pack[i] * 2;
			pstv->out_stride[i] = round_up(
				pstv->out_xsize_plus_1[i] * anchor_en_num[i],
				16);
			pstv->out_xsize_plus_1_stride2[i] =
				((pstv->out_width[i] - 1) / 2 + 1) *
				out_ch_pack[i] * 2 * out_2size[i];
		} else {
			pstv->out_xsize_plus_1[i] =
				pstv->out_width[i] * out_ch_pack[i];
			pstv->out_stride[i] =
				round_up(pstv->out_xsize_plus_1[i], 16);
			pstv->out_xsize_plus_1_stride2[i] =
				((pstv->out_width[i] - 1) / 2 + 1) *
				out_ch_pack[i] * out_2size[i];
		}

		pstv->out_stride_stride2[i] =
			round_up(pstv->out_xsize_plus_1_stride2[i], 16);

		if (out_2size[i] == 1)
			pstv->out_ysize_plus_1_stride2[i] =
				(pstv->out_height[i] - 1) / 2 + 1;
		else
			pstv->out_ysize_plus_1_stride2[i] = pstv->out_height[i];

		if (in_ch_pack[i] == 1)
			pstv->input_xsize_plus_1[i] =
				round_up(pstv->img_width[i], 8);
		else
			pstv->input_xsize_plus_1[i] =
				pstv->img_width[i] * in_ch_pack[i];
	}
}

static void aie_get_data_size(struct mtk_aie_dev *fd, u16 max_img_width,
			      u16 max_img_height)
{
	u8 i, j;
	struct aie_static_info *pstv;

	pstv = &fd->st_info;

	fd->base_para->max_img_width = max_img_width;
	fd->base_para->max_img_height = max_img_height;
	fd->fd_dma_max_size = 0;
	fd->fd_dma_rst_max_size = 0;
	fd->fd_fd_kernel_size = 0;
	fd->fd_attr_kernel_size = 0;
	fd->fd_attr_dma_max_size = 0;
	fd->fd_attr_dma_rst_max_size = 0;

	/* FDMODE Dram Buffer Size */
	fd->fd_rs_cfg_size = 4 * fd->variant->rs_cfg_size * 2;
	fd->fd_fd_cfg_size = 4 * fd->variant->fd_cfg_size * fd_loop_num;
	fd->fd_yuv2rgb_cfg_size = 4 * fd->variant->y2r_cfg_size;

	/* ATTRMODE Dram Buffer Size */
	fd->attr_fd_cfg_size = 4 * fd->variant->fd_cfg_size * attr_loop_num;
	fd->attr_yuv2rgb_cfg_size = 4 * fd->variant->y2r_cfg_size;

	/* HW Output Buffer Size */
	fd->rs_pym_out_size[0] = fd->base_para->max_pyramid_width *
				 fd->base_para->max_pyramid_height;
	fd->rs_pym_out_size[1] = fd->rs_pym_out_size[0] / 2;
	fd->rs_pym_out_size[2] = fd->rs_pym_out_size[0] / 4;

	/* FDMODE Dram Buffer Size */
	for (i = 0; i < fd_loop_num; i++) {
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (fd_wdma_en[i][j]) {
				if ((i == rpn2_loop_num ||
				     i == rpn1_loop_num ||
				     i == rpn0_loop_num) && (j == 0)) {
					fd->fd_dma_rst_max_size +=
						pstv->fd_wdma_size[i][j];
				} else {
					fd->fd_dma_max_size +=
						pstv->fd_wdma_size[i][j];
				}
			}
		}
	}

	for (i = 0; i < fd_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			if (fd_ker_rdma_size[i][j])
				fd->fd_fd_kernel_size += fd_ker_rdma_size[i][j];
		}
	}

	/* ATTRMODE Dram Buffer Size */
	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (attr_wdma_en[i][j]) {
				if ((i == age_out_rgs || i == gender_out_rgs ||
				     i == indian_out_rgs || i == race_out_rgs) &&
				    (j == 0)) {
					fd->fd_attr_dma_rst_max_size +=
						ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
				} else {
					fd->fd_attr_dma_max_size += attr_wdma_size[i][j];
				}
			}
		}
	}

	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++)
			fd->fd_attr_kernel_size += attr_ker_rdma_size[i][j];
	}

	/* FD Pose secure result output buffer: result size * 3 loops */
	fd->fd_dma_rst_max_size += result_size * 3;

	if (fd->variant->fld_enable) {
		/* fld size */
		fd->fld_step_size = 0;
		for (i = 0; i < fld_step_num; i++)
			for (j = 0; j < fld_max_frame; j++)
				fd->fld_step_size += fld_step_align_size[i][j];

		fd->fld_out_size = fld_output_size * fld_max_frame;
	}
}

static int aie_alloc_dram_buf(struct mtk_aie_dev *fd)
{
	int ret;
	u8 i;
	u32 alloc_size;

	/* RS DRAM */
	alloc_size = fd->fd_rs_cfg_size;
	ret = aie_imem_alloc(fd, alloc_size, &fd->rs_cfg_data);
	if (ret)
		goto free_rs;
	/* FD MODE */
	fd->base_para->fd_rs_cfg_pa = fd->rs_cfg_data.pa;
	fd->base_para->fd_rs_cfg_va = fd->rs_cfg_data.va;

	/* FD DRAM */
	alloc_size =
		fd->fd_fd_cfg_size + fd->attr_fd_cfg_size * MAX_ENQUE_FRAME_NUM;
	ret = aie_imem_alloc(fd, alloc_size, &fd->fd_cfg_data);
	if (ret)
		goto free_fd;
	/* FD MODE */
	fd->base_para->fd_fd_cfg_pa = fd->fd_cfg_data.pa;
	fd->base_para->fd_fd_cfg_va = fd->fd_cfg_data.va;
	/* ATTR MODE */
	fd->base_para->attr_fd_cfg_pa[0] =
		fd->base_para->fd_fd_cfg_pa + fd->fd_fd_cfg_size;
	fd->base_para->attr_fd_cfg_va[0] =
		fd->base_para->fd_fd_cfg_va + fd->fd_fd_cfg_size;

	for (i = 1; i < MAX_ENQUE_FRAME_NUM; i++) {
		fd->base_para->attr_fd_cfg_pa[i] =
			fd->base_para->attr_fd_cfg_pa[i - 1] +
			fd->attr_fd_cfg_size;
		fd->base_para->attr_fd_cfg_va[i] =
			fd->base_para->attr_fd_cfg_va[i - 1] +
			fd->attr_fd_cfg_size;
	}

	/* YUV2RGB DRAM */
	alloc_size = fd->fd_yuv2rgb_cfg_size +
		     fd->attr_yuv2rgb_cfg_size * MAX_ENQUE_FRAME_NUM;
	ret = aie_imem_alloc(fd, alloc_size, &fd->yuv2rgb_cfg_data);
	if (ret)
		goto free_yuv2rgb;
	/* FD MODE */
	fd->base_para->fd_yuv2rgb_cfg_pa = fd->yuv2rgb_cfg_data.pa;
	fd->base_para->fd_yuv2rgb_cfg_va = fd->yuv2rgb_cfg_data.va;

	/* ATTR MODE */
	fd->base_para->attr_yuv2rgb_cfg_pa[0] =
		fd->base_para->fd_yuv2rgb_cfg_pa + fd->fd_yuv2rgb_cfg_size;
	fd->base_para->attr_yuv2rgb_cfg_va[0] =
		fd->base_para->fd_yuv2rgb_cfg_va + fd->fd_yuv2rgb_cfg_size;

	for (i = 1; i < MAX_ENQUE_FRAME_NUM; i++) {
		fd->base_para->attr_yuv2rgb_cfg_pa[i] =
			fd->base_para->attr_yuv2rgb_cfg_pa[i - 1] +
			fd->attr_yuv2rgb_cfg_size;
		fd->base_para->attr_yuv2rgb_cfg_va[i] =
			fd->base_para->attr_yuv2rgb_cfg_va[i - 1] +
			fd->attr_yuv2rgb_cfg_size;
	}

	return ret;

free_yuv2rgb:
	aie_imem_free(fd, &fd->fd_cfg_data);
free_fd:
	aie_imem_free(fd, &fd->rs_cfg_data);
free_rs:
	return ret;
}

static int aie_alloc_output_buf(struct mtk_aie_dev *fd)
{
	int ret;
	u32 alloc_size = 0;
	int i, j, pa_off = 0, va_off = 0;

	for (i = 0; i < PYM_NUM; i++)
		alloc_size += fd->rs_pym_out_size[i] * 3;

	ret = aie_imem_alloc(fd, alloc_size, &fd->rs_output_hw);
	if (ret)
		return ret;

	for (i = 0; i < PYM_NUM; i++) {
		for (j = 0; j < COLOR_NUM; j++) {
			fd->base_para->rs_pym_rst_pa[i][j] =
				fd->rs_output_hw.pa + pa_off;
			pa_off += fd->rs_pym_out_size[i];

			fd->base_para->rs_pym_rst_va[i][j] =
				fd->rs_output_hw.va + va_off;
			va_off += fd->rs_pym_out_size[i];
		}
	}

	return ret;
}

static void aie_alloc_normal(struct mtk_aie_dev *fd, unsigned int start,
			     unsigned int end)
{
	unsigned int i, j;
	unsigned int pi, pj;
	struct aie_static_info *pstv;

	pstv = &fd->st_info;
	if (start <= 0 || end <= start)
		return;

	pi = start - 1;
	pj = 0;
	for (i = start; i < end + 1; i++) {
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (fd_wdma_en[i][j]) {
				fd->dma_para->fd_out_hw_pa[i][j] =
					fd->dma_para->fd_out_hw_pa[pi][pj] +
					pstv->fd_wdma_size[pi][pj];
				pi = i;
				pj = j;
			}
		}
	}
}

static int aie_alloc_fddma_buf(struct mtk_aie_dev *fd)
{
	int ret;
	u32 alloc_size;

	alloc_size = fd->fd_dma_max_size;
	ret = aie_imem_alloc(fd, alloc_size, &fd->fd_dma_hw);
	if (ret)
		return ret;

	alloc_size = fd->fd_fd_kernel_size + fd->fd_attr_kernel_size;
	ret = aie_imem_alloc(fd, alloc_size, &fd->fd_kernel_hw);
	if (ret)
		goto free_kernel;

	alloc_size = fd->fd_attr_dma_max_size;
	ret = aie_imem_alloc(fd, alloc_size, &fd->fd_attr_dma_hw);
	if (ret)
		goto free_attr_dma;

	alloc_size = fd->fd_dma_rst_max_size + fd->fd_attr_dma_rst_max_size;
	ret = aie_imem_alloc(fd, alloc_size, &fd->fd_dma_result_hw);
	if (ret)
		goto free_dma_result_hw;

	return 0;

free_dma_result_hw:
	aie_imem_free(fd, &fd->fd_attr_dma_hw);

free_attr_dma:
	aie_imem_free(fd, &fd->fd_kernel_hw);

free_kernel:
	aie_imem_free(fd, &fd->fd_dma_hw);

	return ret;
}

static int aie_alloc_fld_buf(struct mtk_aie_dev *fd)
{
	int ret;
	u32 alloc_size;

	alloc_size = fd->fld_step_size;
	ret = aie_imem_alloc(fd, alloc_size, &fd->fd_fld_step_data);
	if (ret)
		return ret;

	alloc_size = fd->fld_out_size;
	ret = aie_imem_alloc(fd, alloc_size, &fd->fd_fld_out_hw);
	if (ret)
		goto fld_step;

	return 0;
fld_step:
	aie_imem_free(fd, &fd->fd_fld_step_data);

	return ret;
}

static void aie_arrange_fddma_buf(struct mtk_aie_dev *fd)
{
	void *currentVA = NULL;
	dma_addr_t currentPA;
	struct aie_static_info *pstv;
	u8 i, j;

	pstv = &fd->st_info;

	/* 0~18 */
	fd->dma_para->fd_out_hw_pa[0][0] = fd->fd_dma_hw.pa;
	aie_alloc_normal(fd, 1, 18);

	/* 19~27 */
	fd->dma_para->fd_out_hw_pa[19][0] =
		fd->dma_para->fd_out_hw_pa[18][1] + pstv->fd_wdma_size[18][1];
	fd->dma_para->fd_out_hw_pa[19][1] =
		fd->dma_para->fd_out_hw_pa[19][0] + pstv->out_xsize_plus_1[19];
	fd->dma_para->fd_out_hw_pa[20][0] = fd->dma_para->fd_out_hw_pa[19][0] +
					    2 * pstv->out_xsize_plus_1[20];
	fd->dma_para->fd_out_hw_pa[20][1] = fd->dma_para->fd_out_hw_pa[19][0] +
					    3 * pstv->out_xsize_plus_1[20];
	fd->dma_para->fd_out_hw_pa[21][0] = fd->dma_para->fd_out_hw_pa[19][0] +
					    4 * pstv->out_xsize_plus_1[21];
	fd->dma_para->fd_out_hw_pa[22][0] =
		fd->dma_para->fd_out_hw_pa[19][0] + pstv->fd_wdma_size[19][0] +
		pstv->fd_wdma_size[19][1] + pstv->fd_wdma_size[20][0] +
		pstv->fd_wdma_size[20][1] + pstv->fd_wdma_size[21][0];
	fd->dma_para->fd_out_hw_pa[22][1] =
		fd->dma_para->fd_out_hw_pa[22][0] + pstv->fd_wdma_size[22][0] +
		pstv->fd_wdma_size[22][2] + pstv->fd_wdma_size[23][0] +
		pstv->fd_wdma_size[23][2] + pstv->fd_wdma_size[24][0];
	fd->dma_para->fd_out_hw_pa[22][2] =
		fd->dma_para->fd_out_hw_pa[22][0] + pstv->out_xsize_plus_1[22];
	fd->dma_para->fd_out_hw_pa[22][3] =
		fd->dma_para->fd_out_hw_pa[22][1] + pstv->out_xsize_plus_1[22];
	fd->dma_para->fd_out_hw_pa[23][0] = fd->dma_para->fd_out_hw_pa[22][0] +
					    2 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][1] = fd->dma_para->fd_out_hw_pa[22][1] +
					    2 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][2] = fd->dma_para->fd_out_hw_pa[22][0] +
					    3 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][3] = fd->dma_para->fd_out_hw_pa[22][1] +
					    3 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[24][0] = fd->dma_para->fd_out_hw_pa[22][0] +
					    4 * pstv->out_xsize_plus_1[24];
	fd->dma_para->fd_out_hw_pa[24][1] = fd->dma_para->fd_out_hw_pa[22][1] +
					    4 * pstv->out_xsize_plus_1[24];
	fd->dma_para->fd_out_hw_pa[25][0] =
		fd->dma_para->fd_out_hw_pa[22][1] + pstv->fd_wdma_size[22][1] +
		pstv->fd_wdma_size[22][3] + pstv->fd_wdma_size[23][1] +
		pstv->fd_wdma_size[23][3] + pstv->fd_wdma_size[24][1];
	fd->dma_para->fd_out_hw_pa[25][1] =
		fd->dma_para->fd_out_hw_pa[25][0] + pstv->out_xsize_plus_1[25];
	fd->dma_para->fd_out_hw_pa[26][0] = fd->dma_para->fd_out_hw_pa[25][0] +
					    2 * pstv->out_xsize_plus_1[26];
	fd->dma_para->fd_out_hw_pa[26][1] = fd->dma_para->fd_out_hw_pa[25][0] +
					    3 * pstv->out_xsize_plus_1[26];
	fd->dma_para->fd_out_hw_pa[27][0] = fd->dma_para->fd_out_hw_pa[25][0] +
					    4 * pstv->out_xsize_plus_1[27];

	/* 29~47 */
	fd->dma_para->fd_out_hw_pa[29][0] =
		fd->dma_para->fd_out_hw_pa[25][0] + pstv->fd_wdma_size[25][0] +
		pstv->fd_wdma_size[25][1] + pstv->fd_wdma_size[26][0] +
		pstv->fd_wdma_size[26][1] + pstv->fd_wdma_size[27][0];
	aie_alloc_normal(fd, 30, 47);

	/* 48~56 */
	fd->dma_para->fd_out_hw_pa[48][0] =
		fd->dma_para->fd_out_hw_pa[47][1] + pstv->fd_wdma_size[47][1];
	fd->dma_para->fd_out_hw_pa[48][1] =
		fd->dma_para->fd_out_hw_pa[48][0] + pstv->out_xsize_plus_1[48];
	fd->dma_para->fd_out_hw_pa[49][0] = fd->dma_para->fd_out_hw_pa[48][0] +
					    2 * pstv->out_xsize_plus_1[49];
	fd->dma_para->fd_out_hw_pa[49][1] = fd->dma_para->fd_out_hw_pa[48][0] +
					    3 * pstv->out_xsize_plus_1[49];
	fd->dma_para->fd_out_hw_pa[50][0] = fd->dma_para->fd_out_hw_pa[48][0] +
					    4 * pstv->out_xsize_plus_1[50];
	fd->dma_para->fd_out_hw_pa[51][0] =
		fd->dma_para->fd_out_hw_pa[48][0] + pstv->fd_wdma_size[48][0] +
		pstv->fd_wdma_size[48][1] + pstv->fd_wdma_size[49][0] +
		pstv->fd_wdma_size[49][1] + pstv->fd_wdma_size[50][0];
	fd->dma_para->fd_out_hw_pa[51][1] =
		fd->dma_para->fd_out_hw_pa[51][0] + pstv->fd_wdma_size[51][0] +
		pstv->fd_wdma_size[51][2] + pstv->fd_wdma_size[52][0] +
		pstv->fd_wdma_size[52][2] + pstv->fd_wdma_size[53][0];
	fd->dma_para->fd_out_hw_pa[51][2] =
		fd->dma_para->fd_out_hw_pa[51][0] + pstv->out_xsize_plus_1[51];
	fd->dma_para->fd_out_hw_pa[51][3] =
		fd->dma_para->fd_out_hw_pa[51][1] + pstv->out_xsize_plus_1[51];
	fd->dma_para->fd_out_hw_pa[52][0] = fd->dma_para->fd_out_hw_pa[51][0] +
					    2 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][1] = fd->dma_para->fd_out_hw_pa[51][1] +
					    2 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][2] = fd->dma_para->fd_out_hw_pa[51][0] +
					    3 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][3] = fd->dma_para->fd_out_hw_pa[51][1] +
					    3 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[53][0] = fd->dma_para->fd_out_hw_pa[51][0] +
					    4 * pstv->out_xsize_plus_1[53];
	fd->dma_para->fd_out_hw_pa[53][1] = fd->dma_para->fd_out_hw_pa[51][1] +
					    4 * pstv->out_xsize_plus_1[53];
	fd->dma_para->fd_out_hw_pa[54][0] =
		fd->dma_para->fd_out_hw_pa[51][1] + pstv->fd_wdma_size[51][1] +
		pstv->fd_wdma_size[51][3] + pstv->fd_wdma_size[52][1] +
		pstv->fd_wdma_size[52][3] + pstv->fd_wdma_size[53][1];
	fd->dma_para->fd_out_hw_pa[54][1] =
		fd->dma_para->fd_out_hw_pa[54][0] + pstv->out_xsize_plus_1[54];
	fd->dma_para->fd_out_hw_pa[55][0] = fd->dma_para->fd_out_hw_pa[54][0] +
					    2 * pstv->out_xsize_plus_1[55];
	fd->dma_para->fd_out_hw_pa[55][1] = fd->dma_para->fd_out_hw_pa[54][0] +
					    3 * pstv->out_xsize_plus_1[55];
	fd->dma_para->fd_out_hw_pa[56][0] = fd->dma_para->fd_out_hw_pa[54][0] +
					    4 * pstv->out_xsize_plus_1[56];

	/* 58~76 */
	fd->dma_para->fd_out_hw_pa[58][0] =
		fd->dma_para->fd_out_hw_pa[54][0] + pstv->fd_wdma_size[54][0] +
		pstv->fd_wdma_size[54][1] + pstv->fd_wdma_size[55][0] +
		pstv->fd_wdma_size[55][1] + pstv->fd_wdma_size[56][0];
	aie_alloc_normal(fd, 59, 76);

	/* 77~85 */
	fd->dma_para->fd_out_hw_pa[77][0] =
		fd->dma_para->fd_out_hw_pa[76][1] + pstv->fd_wdma_size[76][1];
	fd->dma_para->fd_out_hw_pa[77][1] =
		fd->dma_para->fd_out_hw_pa[77][0] + pstv->out_xsize_plus_1[77];
	fd->dma_para->fd_out_hw_pa[78][0] = fd->dma_para->fd_out_hw_pa[77][0] +
					    2 * pstv->out_xsize_plus_1[78];
	fd->dma_para->fd_out_hw_pa[78][1] = fd->dma_para->fd_out_hw_pa[77][0] +
					    3 * pstv->out_xsize_plus_1[78];
	fd->dma_para->fd_out_hw_pa[79][0] = fd->dma_para->fd_out_hw_pa[77][0] +
					    4 * pstv->out_xsize_plus_1[79];
	fd->dma_para->fd_out_hw_pa[80][0] =
		fd->dma_para->fd_out_hw_pa[77][0] + pstv->fd_wdma_size[77][0] +
		pstv->fd_wdma_size[77][1] + pstv->fd_wdma_size[78][0] +
		pstv->fd_wdma_size[78][1] + pstv->fd_wdma_size[79][0];
	fd->dma_para->fd_out_hw_pa[80][1] =
		fd->dma_para->fd_out_hw_pa[80][0] + pstv->fd_wdma_size[80][0] +
		pstv->fd_wdma_size[80][2] + pstv->fd_wdma_size[81][0] +
		pstv->fd_wdma_size[81][2] + pstv->fd_wdma_size[82][0];
	fd->dma_para->fd_out_hw_pa[80][2] =
		fd->dma_para->fd_out_hw_pa[80][0] + pstv->out_xsize_plus_1[80];
	fd->dma_para->fd_out_hw_pa[80][3] =
		fd->dma_para->fd_out_hw_pa[80][1] + pstv->out_xsize_plus_1[80];
	fd->dma_para->fd_out_hw_pa[81][0] = fd->dma_para->fd_out_hw_pa[80][0] +
					    2 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][1] = fd->dma_para->fd_out_hw_pa[80][1] +
					    2 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][2] = fd->dma_para->fd_out_hw_pa[80][0] +
					    3 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][3] = fd->dma_para->fd_out_hw_pa[80][1] +
					    3 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[82][0] = fd->dma_para->fd_out_hw_pa[80][0] +
					    4 * pstv->out_xsize_plus_1[82];
	fd->dma_para->fd_out_hw_pa[82][1] = fd->dma_para->fd_out_hw_pa[80][1] +
					    4 * pstv->out_xsize_plus_1[82];
	fd->dma_para->fd_out_hw_pa[83][0] =
		fd->dma_para->fd_out_hw_pa[80][1] + pstv->fd_wdma_size[80][1] +
		pstv->fd_wdma_size[80][3] + pstv->fd_wdma_size[81][1] +
		pstv->fd_wdma_size[81][3] + pstv->fd_wdma_size[82][1];
	fd->dma_para->fd_out_hw_pa[83][1] =
		fd->dma_para->fd_out_hw_pa[83][0] + pstv->out_xsize_plus_1[83];
	fd->dma_para->fd_out_hw_pa[84][0] = fd->dma_para->fd_out_hw_pa[83][0] +
					    2 * pstv->out_xsize_plus_1[84];
	fd->dma_para->fd_out_hw_pa[84][1] = fd->dma_para->fd_out_hw_pa[83][0] +
					    3 * pstv->out_xsize_plus_1[84];
	fd->dma_para->fd_out_hw_pa[85][0] = fd->dma_para->fd_out_hw_pa[83][0] +
					    4 * pstv->out_xsize_plus_1[85];

	/* VA : except 28, 57, 86 */
	/* 0~86 */
	fd->dma_para->fd_out_hw_va[0][0] = fd->fd_dma_hw.va;
	for (i = 1; i < fd_loop_num; i++) {
		if (i == rpn2_loop_num || i == rpn1_loop_num ||
		    i == rpn0_loop_num)
			continue;
		for (j = 0; j < 4; j++) {
			if (fd_wdma_en[i][j]) {
				fd->dma_para->fd_out_hw_va[i][j] =
					fd->fd_dma_hw.va +
					fd->dma_para->fd_out_hw_pa[i][j] -
					fd->fd_dma_hw.pa;
			}
		}
	}

	currentPA = fd->dma_para->fd_out_hw_pa[83][0] +
		    pstv->fd_wdma_size[83][0] + pstv->fd_wdma_size[83][1] +
		    pstv->fd_wdma_size[84][0] + pstv->fd_wdma_size[84][1] +
		    pstv->fd_wdma_size[85][0];
	currentVA = fd->dma_para->fd_out_hw_va[83][0] +
		    pstv->fd_wdma_size[83][0] + pstv->fd_wdma_size[83][1] +
		    pstv->fd_wdma_size[84][0] + pstv->fd_wdma_size[84][1] +
		    pstv->fd_wdma_size[85][0];

}

static void aie_arrange_kernel_buf(struct mtk_aie_dev *fd)
{
	void *currentVA = NULL;
	dma_addr_t currentPA;
	u8 i, j;

	currentPA = fd->fd_kernel_hw.pa;
	currentVA = fd->fd_kernel_hw.va;

	for (i = 0; i < fd_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			if (fd_ker_rdma_size[i][j]) {
				fd->dma_para->fd_kernel_pa[i][j] = currentPA;
				fd->dma_para->fd_kernel_va[i][j] = currentVA;
				currentPA += fd_ker_rdma_size[i][j];
				currentVA += fd_ker_rdma_size[i][j];
			}
		}
	}

	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			fd->dma_para->attr_kernel_pa[i][j] = currentPA;
			fd->dma_para->attr_kernel_va[i][j] = currentVA;
			currentPA += attr_ker_rdma_size[i][j];
			currentVA += attr_ker_rdma_size[i][j];
		}
	}
}

static void aie_arrange_attrdma_buf(struct mtk_aie_dev *fd)
{
	void *currentVA = NULL;
	dma_addr_t currentPA;
	u8 i, j;

	currentPA = fd->fd_attr_dma_hw.pa;
	currentVA = fd->fd_attr_dma_hw.va;

	/* attribute mode */
	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (attr_wdma_en[i][j]) {
				fd->dma_para->attr_out_hw_pa[i][j] = currentPA;
				fd->dma_para->attr_out_hw_va[i][j] = currentVA;
				currentPA += attr_wdma_size[i][j];
				currentVA += attr_wdma_size[i][j];
			}
		}
	}
}

static void aie_arrange_result_dma_buf(struct mtk_aie_dev *fd)
{
	void *currentResultVA = NULL;
	dma_addr_t currentResultPA;
	u8 i;
	struct aie_static_info *pstv;

	pstv = &fd->st_info;

	currentResultPA = fd->fd_dma_result_hw.pa;
	currentResultVA = fd->fd_dma_result_hw.va;

	fd->dma_para->fd_out_hw_pa[rpn2_loop_num][0] = currentResultPA;
	fd->dma_para->fd_out_hw_va[rpn2_loop_num][0] = currentResultVA;
	currentResultPA += pstv->fd_wdma_size[rpn2_loop_num][0];
	currentResultVA += pstv->fd_wdma_size[rpn2_loop_num][0];
	fd->dma_para->fd_out_hw_pa[rpn1_loop_num][0] = currentResultPA;
	fd->dma_para->fd_out_hw_va[rpn1_loop_num][0] = currentResultVA;
	currentResultPA += pstv->fd_wdma_size[rpn1_loop_num][0];
	currentResultVA += pstv->fd_wdma_size[rpn1_loop_num][0];
	fd->dma_para->fd_out_hw_pa[rpn0_loop_num][0] = currentResultPA;
	fd->dma_para->fd_out_hw_va[rpn0_loop_num][0] = currentResultVA;
	currentResultPA += pstv->fd_wdma_size[rpn0_loop_num][0];
	currentResultVA += pstv->fd_wdma_size[rpn0_loop_num][0];

	fd->dma_para->attr_out_hw_pa[age_out_rgs][0] = currentResultPA;
	fd->dma_para->attr_out_hw_va[age_out_rgs][0] = currentResultVA;
	currentResultPA += ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
	currentResultVA += ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
	fd->dma_para->attr_out_hw_pa[gender_out_rgs][0] = currentResultPA;
	fd->dma_para->attr_out_hw_va[gender_out_rgs][0] = currentResultVA;
	currentResultPA += ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
	currentResultVA += ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
	fd->dma_para->attr_out_hw_pa[indian_out_rgs][0] = currentResultPA;
	fd->dma_para->attr_out_hw_va[indian_out_rgs][0] = currentResultVA;
	currentResultPA += ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
	currentResultVA += ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
	fd->dma_para->attr_out_hw_pa[race_out_rgs][0] = currentResultPA;
	fd->dma_para->attr_out_hw_va[race_out_rgs][0] = currentResultVA;
	currentResultPA += ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
	currentResultVA += ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;

	/* need to prepare 10 buffers to store 10 times result */
	fd->dma_para->age_out_hw_pa[0] =
		fd->dma_para->attr_out_hw_pa[age_out_rgs][0];
	fd->dma_para->age_out_hw_va[0] =
		fd->dma_para->attr_out_hw_va[age_out_rgs][0];
	fd->dma_para->gender_out_hw_pa[0] =
		fd->dma_para->attr_out_hw_pa[gender_out_rgs][0];
	fd->dma_para->gender_out_hw_va[0] =
		fd->dma_para->attr_out_hw_va[gender_out_rgs][0];
	fd->dma_para->isIndian_out_hw_pa[0] =
		fd->dma_para->attr_out_hw_pa[indian_out_rgs][0];
	fd->dma_para->isIndian_out_hw_va[0] =
		fd->dma_para->attr_out_hw_va[indian_out_rgs][0];
	fd->dma_para->race_out_hw_pa[0] =
		fd->dma_para->attr_out_hw_pa[race_out_rgs][0];
	fd->dma_para->race_out_hw_va[0] =
		fd->dma_para->attr_out_hw_va[race_out_rgs][0];

	for (i = 1; i < MAX_ENQUE_FRAME_NUM; i++) {
		fd->dma_para->age_out_hw_pa[i] =
			fd->dma_para->age_out_hw_pa[i - 1] + ATTR_OUT_SIZE;
		fd->dma_para->age_out_hw_va[i] =
			fd->dma_para->age_out_hw_va[i - 1] + ATTR_OUT_SIZE;
		fd->dma_para->gender_out_hw_pa[i] =
			fd->dma_para->gender_out_hw_pa[i - 1] + ATTR_OUT_SIZE;
		fd->dma_para->gender_out_hw_va[i] =
			fd->dma_para->gender_out_hw_va[i - 1] + ATTR_OUT_SIZE;
		fd->dma_para->isIndian_out_hw_pa[i] =
			fd->dma_para->isIndian_out_hw_pa[i - 1] + ATTR_OUT_SIZE;
		fd->dma_para->isIndian_out_hw_va[i] =
			fd->dma_para->isIndian_out_hw_va[i - 1] + ATTR_OUT_SIZE;
		fd->dma_para->race_out_hw_pa[i] =
			fd->dma_para->race_out_hw_pa[i - 1] + ATTR_OUT_SIZE;
		fd->dma_para->race_out_hw_va[i] =
			fd->dma_para->race_out_hw_va[i - 1] + ATTR_OUT_SIZE;
	}

	memset(fd->fd_dma_result_hw.va, 0, fd->fd_dma_result_hw.size);

}

static void aie_arrange_fld_buf(struct mtk_aie_dev *fd)
{
	u8 i, j;
	unsigned int offset = 0;

	for (i = 0; i < fld_step_num; i++) {
		for (j = 0; j < fld_max_frame; j++) {
			fd->fld_para->fld_step_va[i][j] = fd->fd_fld_step_data.va + offset;
			fd->fld_para->fld_step_pa[i][j] = fd->fd_fld_step_data.pa + offset;
			offset += fld_step_align_size[i][j];
		}
	}

	for (i = 0, offset = 0; i < fld_max_frame; i++) {
		fd->fld_para->fld_output_va[i] = fd->fd_fld_out_hw.va + offset;
		fd->fld_para->fld_output_pa[i] = fd->fd_fld_out_hw.pa + offset;
		offset += fld_output_size;
	}
}

static void aie_update_fddma_buf(struct mtk_aie_dev *fd)
{
	struct aie_static_info *pstv;
	u8 i, j;

	pstv = &fd->st_info;

	/* 19~27 */
	fd->dma_para->fd_out_hw_pa[19][0] =
		fd->dma_para->fd_out_hw_pa[18][1] + pstv->fd_wdma_size[18][1];
	fd->dma_para->fd_out_hw_pa[19][1] =
		fd->dma_para->fd_out_hw_pa[19][0] + pstv->out_xsize_plus_1[19];
	fd->dma_para->fd_out_hw_pa[20][0] = fd->dma_para->fd_out_hw_pa[19][0] +
					    2 * pstv->out_xsize_plus_1[20];
	fd->dma_para->fd_out_hw_pa[20][1] = fd->dma_para->fd_out_hw_pa[19][0] +
					    3 * pstv->out_xsize_plus_1[20];
	fd->dma_para->fd_out_hw_pa[21][0] = fd->dma_para->fd_out_hw_pa[19][0] +
					    4 * pstv->out_xsize_plus_1[21];
	fd->dma_para->fd_out_hw_pa[22][0] =
		fd->dma_para->fd_out_hw_pa[19][0] + pstv->fd_wdma_size[19][0] +
		pstv->fd_wdma_size[19][1] + pstv->fd_wdma_size[20][0] +
		pstv->fd_wdma_size[20][1] + pstv->fd_wdma_size[21][0];
	fd->dma_para->fd_out_hw_pa[22][1] =
		fd->dma_para->fd_out_hw_pa[22][0] + pstv->fd_wdma_size[22][0] +
		pstv->fd_wdma_size[22][2] + pstv->fd_wdma_size[23][0] +
		pstv->fd_wdma_size[23][2] + pstv->fd_wdma_size[24][0];
	fd->dma_para->fd_out_hw_pa[22][2] =
		fd->dma_para->fd_out_hw_pa[22][0] + pstv->out_xsize_plus_1[22];
	fd->dma_para->fd_out_hw_pa[22][3] =
		fd->dma_para->fd_out_hw_pa[22][1] + pstv->out_xsize_plus_1[22];
	fd->dma_para->fd_out_hw_pa[23][0] = fd->dma_para->fd_out_hw_pa[22][0] +
					    2 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][1] = fd->dma_para->fd_out_hw_pa[22][1] +
					    2 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][2] = fd->dma_para->fd_out_hw_pa[22][0] +
					    3 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][3] = fd->dma_para->fd_out_hw_pa[22][1] +
					    3 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[24][0] = fd->dma_para->fd_out_hw_pa[22][0] +
					    4 * pstv->out_xsize_plus_1[24];
	fd->dma_para->fd_out_hw_pa[24][1] = fd->dma_para->fd_out_hw_pa[22][1] +
					    4 * pstv->out_xsize_plus_1[24];
	fd->dma_para->fd_out_hw_pa[25][0] =
		fd->dma_para->fd_out_hw_pa[22][1] + pstv->fd_wdma_size[22][1] +
		pstv->fd_wdma_size[22][3] + pstv->fd_wdma_size[23][1] +
		pstv->fd_wdma_size[23][3] + pstv->fd_wdma_size[24][1];
	fd->dma_para->fd_out_hw_pa[25][1] =
		fd->dma_para->fd_out_hw_pa[25][0] + pstv->out_xsize_plus_1[25];
	fd->dma_para->fd_out_hw_pa[26][0] = fd->dma_para->fd_out_hw_pa[25][0] +
					    2 * pstv->out_xsize_plus_1[26];
	fd->dma_para->fd_out_hw_pa[26][1] = fd->dma_para->fd_out_hw_pa[25][0] +
					    3 * pstv->out_xsize_plus_1[26];
	fd->dma_para->fd_out_hw_pa[27][0] = fd->dma_para->fd_out_hw_pa[25][0] +
					    4 * pstv->out_xsize_plus_1[27];

	/* 48~56 */
	fd->dma_para->fd_out_hw_pa[48][0] =
		fd->dma_para->fd_out_hw_pa[47][1] + pstv->fd_wdma_size[47][1];
	fd->dma_para->fd_out_hw_pa[48][1] =
		fd->dma_para->fd_out_hw_pa[48][0] + pstv->out_xsize_plus_1[48];
	fd->dma_para->fd_out_hw_pa[49][0] = fd->dma_para->fd_out_hw_pa[48][0] +
					    2 * pstv->out_xsize_plus_1[49];
	fd->dma_para->fd_out_hw_pa[49][1] = fd->dma_para->fd_out_hw_pa[48][0] +
					    3 * pstv->out_xsize_plus_1[49];
	fd->dma_para->fd_out_hw_pa[50][0] = fd->dma_para->fd_out_hw_pa[48][0] +
					    4 * pstv->out_xsize_plus_1[50];
	fd->dma_para->fd_out_hw_pa[51][0] =
		fd->dma_para->fd_out_hw_pa[48][0] + pstv->fd_wdma_size[48][0] +
		pstv->fd_wdma_size[48][1] + pstv->fd_wdma_size[49][0] +
		pstv->fd_wdma_size[49][1] + pstv->fd_wdma_size[50][0];
	fd->dma_para->fd_out_hw_pa[51][1] =
		fd->dma_para->fd_out_hw_pa[51][0] + pstv->fd_wdma_size[51][0] +
		pstv->fd_wdma_size[51][2] + pstv->fd_wdma_size[52][0] +
		pstv->fd_wdma_size[52][2] + pstv->fd_wdma_size[53][0];
	fd->dma_para->fd_out_hw_pa[51][2] =
		fd->dma_para->fd_out_hw_pa[51][0] + pstv->out_xsize_plus_1[51];
	fd->dma_para->fd_out_hw_pa[51][3] =
		fd->dma_para->fd_out_hw_pa[51][1] + pstv->out_xsize_plus_1[51];
	fd->dma_para->fd_out_hw_pa[52][0] = fd->dma_para->fd_out_hw_pa[51][0] +
					    2 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][1] = fd->dma_para->fd_out_hw_pa[51][1] +
					    2 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][2] = fd->dma_para->fd_out_hw_pa[51][0] +
					    3 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][3] = fd->dma_para->fd_out_hw_pa[51][1] +
					    3 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[53][0] = fd->dma_para->fd_out_hw_pa[51][0] +
					    4 * pstv->out_xsize_plus_1[53];
	fd->dma_para->fd_out_hw_pa[53][1] = fd->dma_para->fd_out_hw_pa[51][1] +
					    4 * pstv->out_xsize_plus_1[53];
	fd->dma_para->fd_out_hw_pa[54][0] =
		fd->dma_para->fd_out_hw_pa[51][1] + pstv->fd_wdma_size[51][1] +
		pstv->fd_wdma_size[51][3] + pstv->fd_wdma_size[52][1] +
		pstv->fd_wdma_size[52][3] + pstv->fd_wdma_size[53][1];
	fd->dma_para->fd_out_hw_pa[54][1] =
		fd->dma_para->fd_out_hw_pa[54][0] + pstv->out_xsize_plus_1[54];
	fd->dma_para->fd_out_hw_pa[55][0] = fd->dma_para->fd_out_hw_pa[54][0] +
					    2 * pstv->out_xsize_plus_1[55];
	fd->dma_para->fd_out_hw_pa[55][1] = fd->dma_para->fd_out_hw_pa[54][0] +
					    3 * pstv->out_xsize_plus_1[55];
	fd->dma_para->fd_out_hw_pa[56][0] = fd->dma_para->fd_out_hw_pa[54][0] +
					    4 * pstv->out_xsize_plus_1[56];
	/* 77~85 */
	fd->dma_para->fd_out_hw_pa[77][0] =
		fd->dma_para->fd_out_hw_pa[76][1] + pstv->fd_wdma_size[76][1];
	fd->dma_para->fd_out_hw_pa[77][1] =
		fd->dma_para->fd_out_hw_pa[77][0] + pstv->out_xsize_plus_1[77];
	fd->dma_para->fd_out_hw_pa[78][0] = fd->dma_para->fd_out_hw_pa[77][0] +
					    2 * pstv->out_xsize_plus_1[78];
	fd->dma_para->fd_out_hw_pa[78][1] = fd->dma_para->fd_out_hw_pa[77][0] +
					    3 * pstv->out_xsize_plus_1[78];
	fd->dma_para->fd_out_hw_pa[79][0] = fd->dma_para->fd_out_hw_pa[77][0] +
					    4 * pstv->out_xsize_plus_1[79];
	fd->dma_para->fd_out_hw_pa[80][0] =
		fd->dma_para->fd_out_hw_pa[77][0] + pstv->fd_wdma_size[77][0] +
		pstv->fd_wdma_size[77][1] + pstv->fd_wdma_size[78][0] +
		pstv->fd_wdma_size[78][1] + pstv->fd_wdma_size[79][0];
	fd->dma_para->fd_out_hw_pa[80][1] =
		fd->dma_para->fd_out_hw_pa[80][0] + pstv->fd_wdma_size[80][0] +
		pstv->fd_wdma_size[80][2] + pstv->fd_wdma_size[81][0] +
		pstv->fd_wdma_size[81][2] + pstv->fd_wdma_size[82][0];
	fd->dma_para->fd_out_hw_pa[80][2] =
		fd->dma_para->fd_out_hw_pa[80][0] + pstv->out_xsize_plus_1[80];
	fd->dma_para->fd_out_hw_pa[80][3] =
		fd->dma_para->fd_out_hw_pa[80][1] + pstv->out_xsize_plus_1[80];
	fd->dma_para->fd_out_hw_pa[81][0] = fd->dma_para->fd_out_hw_pa[80][0] +
					    2 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][1] = fd->dma_para->fd_out_hw_pa[80][1] +
					    2 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][2] = fd->dma_para->fd_out_hw_pa[80][0] +
					    3 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][3] = fd->dma_para->fd_out_hw_pa[80][1] +
					    3 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[82][0] = fd->dma_para->fd_out_hw_pa[80][0] +
					    4 * pstv->out_xsize_plus_1[82];
	fd->dma_para->fd_out_hw_pa[82][1] = fd->dma_para->fd_out_hw_pa[80][1] +
					    4 * pstv->out_xsize_plus_1[82];
	fd->dma_para->fd_out_hw_pa[83][0] =
		fd->dma_para->fd_out_hw_pa[80][1] + pstv->fd_wdma_size[80][1] +
		pstv->fd_wdma_size[80][3] + pstv->fd_wdma_size[81][1] +
		pstv->fd_wdma_size[81][3] + pstv->fd_wdma_size[82][1];
	fd->dma_para->fd_out_hw_pa[83][1] =
		fd->dma_para->fd_out_hw_pa[83][0] + pstv->out_xsize_plus_1[83];
	fd->dma_para->fd_out_hw_pa[84][0] = fd->dma_para->fd_out_hw_pa[83][0] +
					    2 * pstv->out_xsize_plus_1[84];
	fd->dma_para->fd_out_hw_pa[84][1] = fd->dma_para->fd_out_hw_pa[83][0] +
					    3 * pstv->out_xsize_plus_1[84];
	fd->dma_para->fd_out_hw_pa[85][0] = fd->dma_para->fd_out_hw_pa[83][0] +
					    4 * pstv->out_xsize_plus_1[85];

	/* VA : except 28, 57, 86 */
	/* 0~86 */
	fd->dma_para->fd_out_hw_va[0][0] = fd->fd_dma_hw.va;
	for (i = 1; i < fd_loop_num; i++) {
		if (i == rpn2_loop_num || i == rpn1_loop_num ||
		    i == rpn0_loop_num)
			continue;
		for (j = 0; j < 4; j++) {
			if (fd_wdma_en[i][j]) {
				fd->dma_para->fd_out_hw_va[i][j] =
					fd->fd_dma_hw.va +
					fd->dma_para->fd_out_hw_pa[i][j] -
					fd->fd_dma_hw.pa;
			}
		}
	}
}

static void aie_free_dram_buf(struct mtk_aie_dev *fd)
{
	aie_imem_free(fd, &fd->rs_cfg_data);
	aie_imem_free(fd, &fd->fd_cfg_data);
	aie_imem_free(fd, &fd->yuv2rgb_cfg_data);
}

static void aie_free_output_buf(struct mtk_aie_dev *fd)
{
	aie_imem_free(fd, &fd->rs_output_hw);
}

static void aie_free_fddma_buf(struct mtk_aie_dev *fd)
{
	aie_imem_free(fd, &fd->fd_dma_hw);
	aie_imem_free(fd, &fd->fd_kernel_hw);
	aie_imem_free(fd, &fd->fd_attr_dma_hw);
	aie_imem_free(fd, &fd->fd_dma_result_hw);
}

static void aie_free_fld_buf(struct mtk_aie_dev *fd)
{
	aie_imem_free(fd, &fd->fd_fld_step_data);
	aie_imem_free(fd, &fd->fd_fld_out_hw);
}

static int aie_copy_fw(struct mtk_aie_dev *fd, const char *name, void *buf,
		       unsigned int size)
{
	int ret;
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, name, fd->dev);
	if (ret) {
		dev_info(fd->dev, "%s: fail to load firmware %s\n", __func__,
			 name);
		goto fw_error;
	}

	if (size < fw->size) {
		ret = -EINVAL;
		goto fw_error;
	}

	memcpy(buf, fw->data, fw->size);

fw_error:
	if (fw != NULL) {
		release_firmware(fw);
		fw = NULL;
	}
	return ret;
}

static int aie_load_fw(struct mtk_aie_dev *fd)
{
	u8 i, j;
	int ret;
	char name[128];
	char *sel_folder;
	char *mp_fw30_folder = "aie_mp_fw";
	char *mp_fw31_folder = "aie_mp_fw31";

	if (fd->variant->hw_version == 30)
		sel_folder = mp_fw30_folder;
	else
		sel_folder = mp_fw31_folder;

	ret = sprintf(name, "%s/config/aie_fd_fd_config.bin", sel_folder);
	if (ret < 0)
		return ret;

	ret = aie_copy_fw(fd, name, fd->base_para->fd_fd_cfg_va,
			  fd->fd_fd_cfg_size);
	if (ret)
		return ret;

	ret = sprintf(name, "%s/config/aie_fd_rs_config.bin", sel_folder);
	if (ret < 0)
		return ret;

	ret = aie_copy_fw(fd, name, fd->base_para->fd_rs_cfg_va,
			  fd->fd_rs_cfg_size);
	if (ret)
		return ret;

	ret = sprintf(name, "%s/config/aie_fd_yuv2rgb_config.bin", sel_folder);
	if (ret < 0)
		return ret;

	ret = aie_copy_fw(fd, name, fd->base_para->fd_yuv2rgb_cfg_va,
			  fd->fd_yuv2rgb_cfg_size);
	if (ret)
		return ret;

	ret = sprintf(name, "%s/config/aie_attr_fd_config.bin", sel_folder);
	if (ret < 0)
		return ret;

	ret = aie_copy_fw(fd, name, fd->base_para->attr_fd_cfg_va[0],
			  fd->attr_fd_cfg_size);
	if (ret)
		return ret;

	ret = sprintf(name, "%s/config/aie_attr_yuv2rgb_config.bin", sel_folder);
	if (ret < 0)
		return ret;

	ret = aie_copy_fw(fd, name, fd->base_para->attr_yuv2rgb_cfg_va[0],
			  fd->attr_yuv2rgb_cfg_size);
	if (ret)
		return ret;

	for (i = 1; i < MAX_ENQUE_FRAME_NUM; i++) {
		memcpy(fd->base_para->attr_fd_cfg_va[i],
		       fd->base_para->attr_fd_cfg_va[0], fd->attr_fd_cfg_size);
		memcpy(fd->base_para->attr_yuv2rgb_cfg_va[i],
		       fd->base_para->attr_yuv2rgb_cfg_va[0],
		       fd->attr_yuv2rgb_cfg_size);
	}

	for (i = 0; i < fd_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			if (fd_ker_rdma_size[i][j]) {
				ret = sprintf(name,
					"%s/kernel/aie_fd_kernel_bias_loop%02d_%d.bin",
					sel_folder, i, j);
				if (ret < 0)
					return ret;

				ret = aie_copy_fw(
					fd, name,
					fd->dma_para->fd_kernel_va[i][j],
					fd_ker_rdma_size[i][j]);
				if (ret)
					return ret;
			}
		}
	}

	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			ret = sprintf(name,
				"%s/kernel/aie_attr_kernel_bias_loop%02d_%d.bin",
				sel_folder, i, j);
			if (ret < 0)
				return ret;

			ret = aie_copy_fw(fd, name,
					  fd->dma_para->attr_kernel_va[i][j],
					  attr_ker_rdma_size[i][j]);
			if (ret)
				return ret;
		}
	}

	if (fd->variant->fld_enable) {
		ret = sprintf(name, "%s/config/aie_fld_blink_weight_forest14.bin",
			sel_folder);
		if (ret < 0)
			return ret;

		ret = aie_copy_fw(fd, name,
				  fd->fld_para->fld_step_va[fld_step_blink][14],
				  fld_step_align_size[fld_step_blink][14]);
		if (ret)
			return ret;

		for (j = 0; j < fld_max_frame; j++) {
			ret = sprintf(name, "%s/config/aie_fld_cv_forest%02d_iom3.bin",
				sel_folder, j);
			if (ret < 0)
				return ret;

			ret = aie_copy_fw(fd, name,
					  fd->fld_para->fld_step_va[fld_step_cv][j],
					  fld_step_align_size[fld_step_cv][j]);
			if (ret)
				return ret;
		}

		for (j = 0; j < fld_max_frame; j++) {
			ret = sprintf(name, "%s/config/aie_fld_fp_forest%02d_om45.bin",
				sel_folder, j);
			if (ret < 0)
				return ret;

			ret = aie_copy_fw(fd, name,
					  fd->fld_para->fld_step_va[fld_step_fp][j],
					  fld_step_align_size[fld_step_fp][j]);
			if (ret)
				return ret;
		}

		for (j = 0; j < fld_max_frame; j++) {
			ret = sprintf(name, "%s/config/aie_fld_leafnode_forest%02d.bin",
				sel_folder, j);
			if (ret < 0)
				return ret;

			ret = aie_copy_fw(fd, name,
					  fd->fld_para->fld_step_va[fld_step_leaf][j],
					  fld_step_align_size[fld_step_leaf][j]);
			if (ret)
				return ret;
		}

		for (j = 0; j < fld_max_frame; j++) {
			ret = sprintf(name, "%s/config/aie_fld_tree_forest%02d_km02.bin",
				sel_folder, j);
			if (ret < 0)
				return ret;
			ret = aie_copy_fw(fd, name,
					  fd->fld_para->fld_step_va[fld_step_km02][j],
					  fld_step_align_size[fld_step_km02][j]);
			if (ret)
				return ret;
		}

		for (j = 0; j < fld_max_frame; j++) {
			ret = sprintf(name, "%s/config/aie_fld_tree_forest%02d_km13.bin",
				sel_folder, j);
			if (ret < 0)
				return ret;
			ret = aie_copy_fw(fd, name,
					  fd->fld_para->fld_step_va[fld_step_km13][j],
					  fld_step_align_size[fld_step_km13][j]);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static void aie_reset_output_buf(struct mtk_aie_dev *fd,
				 struct aie_enq_info *aie_cfg)
{
	if (aie_cfg->sel_mode == 0) {
		memset(fd->rs_output_hw.va, 0, fd->rs_output_hw.size);

		memset(fd->dma_para->fd_out_hw_va[rpn0_loop_num][0], 0,
		       result_size);
		memset(fd->dma_para->fd_out_hw_va[rpn1_loop_num][0], 0,
		       result_size);
		memset(fd->dma_para->fd_out_hw_va[rpn2_loop_num][0], 0,
		       result_size);
	} else if (aie_cfg->sel_mode == 1) {
		memset(fd->base_para->rs_pym_rst_va[0][0], 0,
		       fd->rs_pym_out_size[0]);
		memset(fd->base_para->rs_pym_rst_va[0][1], 0,
		       fd->rs_pym_out_size[0]);
		memset(fd->base_para->rs_pym_rst_va[0][2], 0,
		       fd->rs_pym_out_size[0]);
	} else if (aie_cfg->sel_mode == 2) {
		if (fd->variant->fld_enable)
			memset(fd->fld_para->fld_output_va[0], 0,
			       fld_max_frame * fld_output_size);
	}
}

static int aie_update_cfg(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	int crop_width;
	int crop_height;

	crop_width = aie_cfg->src_img_width;
	crop_height = aie_cfg->src_img_height;

	if (aie_cfg->en_roi) {
		crop_width = aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1 + 1;
		crop_height = aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1 + 1;
	}

	if (crop_width == 0 || crop_height == 0) {
		dev_info(fd->dev, "AIE error:crop size is wrong");
		return -EINVAL;
	}

	if (aie_cfg->en_padding) {
		crop_width = crop_width + aie_cfg->src_padding.right +
			     aie_cfg->src_padding.left;
		crop_height = crop_height + aie_cfg->src_padding.up +
			      aie_cfg->src_padding.down;
	}

	if (aie_cfg->sel_mode == 0) {
		fd->base_para->sel_mode = aie_cfg->sel_mode;
		fd->base_para->crop_width = crop_width;
		fd->base_para->crop_height = crop_height;
		fd->base_para->src_img_addr = aie_cfg->src_img_addr;
		fd->base_para->src_img_addr_uv = aie_cfg->src_img_addr_uv;
		fd->base_para->img_width = aie_cfg->src_img_width;
		fd->base_para->img_height = aie_cfg->src_img_height;
		fd->base_para->src_img_fmt = aie_cfg->src_img_fmt;
		fd->base_para->rotate_degree = aie_cfg->rotate_degree;
	} else if (aie_cfg->sel_mode == 1) {
		fd->attr_para->sel_mode[fd->attr_para->w_idx] =
			aie_cfg->sel_mode;
		fd->attr_para->crop_width[fd->attr_para->w_idx] = crop_width;
		fd->attr_para->crop_height[fd->attr_para->w_idx] = crop_height;
		fd->attr_para->src_img_addr[fd->attr_para->w_idx] =
			aie_cfg->src_img_addr;
		fd->attr_para->src_img_addr_uv[fd->attr_para->w_idx] =
			aie_cfg->src_img_addr_uv;
		fd->attr_para->img_width[fd->attr_para->w_idx] =
			aie_cfg->src_img_width;
		fd->attr_para->img_height[fd->attr_para->w_idx] =
			aie_cfg->src_img_height;
		fd->attr_para->src_img_fmt[fd->attr_para->w_idx] =
			aie_cfg->src_img_fmt;
		fd->attr_para->rotate_degree[fd->attr_para->w_idx] =
			aie_cfg->rotate_degree;
	}

	return 0;
}

static u32 aie_combine_u16(u16 low, u16 high)
{
	return ((u32)high << 16) | low;
}

static u32 aie_combine_stride(u16 low, u16 high)
{
	return ((u32)high << 16) | (low & 0x000F);
}

static int aie_config_y2r(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg,
			  int mode)
{
	u32 img_addr;
	u32 img_addr_UV;
	u32 img_off;
	u32 img_off_uv;
	u32 *yuv2rgb_cfg;
	u32 srcbuf, srcbuf_UV;
	u16 xmag_0, ymag_0;
	u16 pym0_out_w;
	u16 pym0_out_h;
	u16 stride_pym0_out_w;
	u16 src_crop_w;
	u16 src_crop_h;

	if (aie_cfg->en_roi == false) {
		img_off = 0;
		img_off_uv = 0;
	} else {
		if (aie_cfg->src_img_fmt == FMT_MONO ||
		    aie_cfg->src_img_fmt == FMT_YUV_2P ||
		    aie_cfg->src_img_fmt == FMT_YVU_2P) {
			img_off =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1;
			img_off_uv =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1;
		} else if (aie_cfg->src_img_fmt == FMT_YUV420_2P ||
			   aie_cfg->src_img_fmt == FMT_YUV420_1P) {
			img_off =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1;
			img_off_uv = aie_cfg->src_img_stride *
				     aie_cfg->src_roi.y1 / 2 +
				     aie_cfg->src_roi.x1;
		} else if (aie_cfg->src_img_fmt == FMT_YUYV ||
			   aie_cfg->src_img_fmt == FMT_YVYU ||
			   aie_cfg->src_img_fmt == FMT_UYVY ||
			   aie_cfg->src_img_fmt == FMT_VYUY) {
			img_off =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1 * 2;
			img_off_uv =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1 * 2;
		} else {
			dev_info(fd->dev,
				 "AIE error: Unsupport input format %d",
				 aie_cfg->src_img_fmt);
			return -EINVAL;
		}
	}

	img_addr = aie_cfg->src_img_addr + img_off;
	img_addr_UV = aie_cfg->src_img_addr_uv + img_off_uv;

	srcbuf = img_addr;
	if (aie_cfg->src_img_fmt == FMT_YUV420_2P ||
	    aie_cfg->src_img_fmt == FMT_YUV420_1P ||
	    aie_cfg->src_img_fmt == FMT_YUV_2P ||
	    aie_cfg->src_img_fmt == FMT_YVU_2P)
		srcbuf_UV = img_addr_UV;
	else
		srcbuf_UV = 0;

	if (mode == 0) {
		src_crop_w = fd->base_para->crop_width;
		src_crop_h = fd->base_para->crop_height;
		yuv2rgb_cfg = (u32 *)fd->base_para->fd_yuv2rgb_cfg_va;
		pym0_out_w = fd->base_para->pyramid_width;
	} else {
		src_crop_w = fd->attr_para->crop_width[fd->attr_para->w_idx];
		src_crop_h = fd->attr_para->crop_height[fd->attr_para->w_idx];
		yuv2rgb_cfg =
			(u32 *)fd->base_para
				->attr_yuv2rgb_cfg_va[fd->attr_para->w_idx];
		pym0_out_w = ATTR_MODE_PYRAMID_WIDTH;
	}

	pym0_out_h = pym0_out_w * src_crop_h / src_crop_w;

	if (pym0_out_w != 0) {
		xmag_0 = 512 * src_crop_w / pym0_out_w;
		ymag_0 = xmag_0;
	} else {
		xmag_0 = 0;
		ymag_0 = 0;
	}

	yuv2rgb_cfg[Y2R_SRC_DST_FORMAT] =
		(yuv2rgb_cfg[Y2R_SRC_DST_FORMAT] & 0xFFFFFFF8) |
		((aie_cfg->src_img_fmt) & 0x7);
	if (aie_cfg->src_img_fmt == FMT_YUV420_2P ||
	    aie_cfg->src_img_fmt == FMT_YUV420_1P) { /* for match patten */
		yuv2rgb_cfg[Y2R_SRC_DST_FORMAT] =
			(yuv2rgb_cfg[Y2R_SRC_DST_FORMAT] & 0xFFFFFFF8) |
			((0x3) & 0x7);
	}
	yuv2rgb_cfg[Y2R_IN_W_H] = (yuv2rgb_cfg[Y2R_IN_W_H] & 0xF800F800) |
				  ((src_crop_w << 16) & 0x7FF0000) |
				  (src_crop_h & 0x7FF);
	yuv2rgb_cfg[Y2R_OUT_W_H] = (yuv2rgb_cfg[Y2R_OUT_W_H] & 0xF800F800) |
				   ((pym0_out_w << 16) & 0x7FF0000) |
				   (pym0_out_h & 0x7FF);

	if (aie_cfg->src_img_fmt == FMT_YUV_2P ||
	    aie_cfg->src_img_fmt == FMT_YVU_2P) { /* 2 plane */
		yuv2rgb_cfg[Y2R_RA0_RA1_EN] =
			(yuv2rgb_cfg[Y2R_RA0_RA1_EN] & 0xFFFFFFEE) | 0x11;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
		}
		yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x1;
		yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x1;
	} else if (aie_cfg->src_img_fmt == FMT_MONO) {
		yuv2rgb_cfg[Y2R_RA0_RA1_EN] =
			(yuv2rgb_cfg[Y2R_RA0_RA1_EN] & 0xFFFFFFEE) | 0x01;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
		}
		yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x0;
		yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x0;
	} else if (aie_cfg->src_img_fmt == FMT_YUYV ||
		   aie_cfg->src_img_fmt == FMT_YVYU ||
		   aie_cfg->src_img_fmt == FMT_UYVY ||
		   aie_cfg->src_img_fmt == FMT_VYUY) { /* 1 plane */
		yuv2rgb_cfg[Y2R_RA0_RA1_EN] =
			(yuv2rgb_cfg[Y2R_RA0_RA1_EN] & 0xFFFFFFEE) | 0x1;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				2 * (aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1 +
				     1) -
					1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				2 * (aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1 +
				     1) -
					1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				2 * src_crop_w - 1, src_crop_h - 1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				2 * src_crop_w - 1, src_crop_h - 1);
		}
		yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x3;
		yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x3;
	}

	/* AIE3.0 */
	if (aie_cfg->src_img_fmt == FMT_YUV420_2P ||
	    aie_cfg->src_img_fmt == FMT_YUV420_1P) {
		yuv2rgb_cfg[Y2R_RA0_RA1_EN] =
			(yuv2rgb_cfg[Y2R_RA0_RA1_EN] & 0xFFFFFFEE) | 0x11;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				(aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1) /
					2);
		} else {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				src_crop_w - 1, src_crop_h / 2 - 1);
		}
		yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x0;
		yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x0;

		yuv2rgb_cfg[Y2R_CO2_FMT_MODE_EN] =
			(yuv2rgb_cfg[Y2R_CO2_FMT_MODE_EN] & 0xFFFFFFFE) | 0x01;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_CO2_CROP_X] = aie_combine_u16(
				0, aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1);
			yuv2rgb_cfg[Y2R_CO2_CROP_Y] = aie_combine_u16(
				0, aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_CO2_CROP_X] =
				aie_combine_u16(0, src_crop_w - 1);
			yuv2rgb_cfg[Y2R_CO2_CROP_Y] =
				aie_combine_u16(0, src_crop_h - 1);
		}
	} else {
		yuv2rgb_cfg[Y2R_CO2_FMT_MODE_EN] =
			(yuv2rgb_cfg[Y2R_CO2_FMT_MODE_EN] & 0xFFFFFFFE);

		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_CO2_CROP_X] = aie_combine_u16(
				0, aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1);
			yuv2rgb_cfg[Y2R_CO2_CROP_Y] = aie_combine_u16(
				0, aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_CO2_CROP_X] =
				aie_combine_u16(0, src_crop_w - 1);
			yuv2rgb_cfg[Y2R_CO2_CROP_Y] =
				aie_combine_u16(0, src_crop_h - 1);
		}
	}

	stride_pym0_out_w = round_up(pym0_out_w, 8);

	yuv2rgb_cfg[Y2R_OUT_X_Y_SIZE0] =
		aie_combine_u16(pym0_out_w - 1, pym0_out_h - 1);
	yuv2rgb_cfg[Y2R_OUT_STRIDE0_BUS_SIZE0] = aie_combine_u16(
		yuv2rgb_cfg[Y2R_OUT_STRIDE0_BUS_SIZE0], stride_pym0_out_w);
	yuv2rgb_cfg[Y2R_OUT_X_Y_SIZE1] =
		aie_combine_u16(pym0_out_w - 1, pym0_out_h - 1);
	yuv2rgb_cfg[Y2R_OUT_STRIDE1_BUS_SIZE1] = aie_combine_u16(
		yuv2rgb_cfg[Y2R_OUT_STRIDE1_BUS_SIZE1], stride_pym0_out_w);
	yuv2rgb_cfg[Y2R_OUT_X_Y_SIZE2] =
		aie_combine_u16(pym0_out_w - 1, pym0_out_h - 1);
	yuv2rgb_cfg[Y2R_OUT_STRIDE2_BUS_SIZE2] = aie_combine_u16(
		yuv2rgb_cfg[Y2R_OUT_STRIDE2_BUS_SIZE2], stride_pym0_out_w);

	if (aie_cfg->en_padding) {
		yuv2rgb_cfg[Y2R_PADDING_EN_UP_DOWN] =
			1 | ((aie_cfg->src_padding.up << 4) & 0x1FF0) |
			((aie_cfg->src_padding.down << 16) & 0x01FF0000);
		yuv2rgb_cfg[Y2R_PADDING_RIGHT_LEFT] =
			(aie_cfg->src_padding.right & 0x01FF) |
			((aie_cfg->src_padding.left << 16) & 0x01FF0000);
	} else {
		yuv2rgb_cfg[Y2R_PADDING_EN_UP_DOWN] = 0;
		yuv2rgb_cfg[Y2R_PADDING_RIGHT_LEFT] = 0;
	}

	yuv2rgb_cfg[Y2R_IN_0] = srcbuf;
	yuv2rgb_cfg[Y2R_IN_1] = srcbuf_UV;

	yuv2rgb_cfg[Y2R_OUT_0] = (u32)fd->base_para->rs_pym_rst_pa[0][0];
	yuv2rgb_cfg[Y2R_OUT_1] = (u32)fd->base_para->rs_pym_rst_pa[0][1];
	yuv2rgb_cfg[Y2R_OUT_2] = (u32)fd->base_para->rs_pym_rst_pa[0][2];

	yuv2rgb_cfg[Y2R_X_Y_MAG] =
		(xmag_0 & 0x3FFF) | ((ymag_0 << 16) & 0x3FFF0000);

	if (src_crop_w >= pym0_out_w) { /* down scale AIE1.0 by FRZ */
		yuv2rgb_cfg[Y2R_RS_SEL_SRZ_EN] =
			(yuv2rgb_cfg[Y2R_RS_SEL_SRZ_EN] & 0x00100070) |
			FDRZ_BIT;
		yuv2rgb_cfg[Y2R_SRZ_HORI_STEP] = 0;
		yuv2rgb_cfg[Y2R_SRZ_VERT_STEP] = 0;
	} else { /* SRZ */
		/* 0: FDRZ for down scaling */
		/* 1: SRZ for up scaling */
		yuv2rgb_cfg[Y2R_RS_SEL_SRZ_EN] =
			(yuv2rgb_cfg[Y2R_RS_SEL_SRZ_EN] & 0x00100070) | SRZ_BIT;
		yuv2rgb_cfg[Y2R_SRZ_HORI_STEP] =
			((src_crop_w - 1) << 15) / (pym0_out_w - 1);
		yuv2rgb_cfg[Y2R_SRZ_VERT_STEP] =
			((src_crop_h - 1) << 15) / (pym0_out_h - 1);
	}

	if (fd->variant->hw_version == 31) {
		yuv2rgb_cfg[Y2R_CON_IN_BA_MSB] = (u32)0x02020202;
		yuv2rgb_cfg[Y2R_CON_OUT_BA_MSB] = (u32)0x02020202;
	}

	return 0;
}

static int aie_config_rs(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	u32 *rs_cfg;
	u32 *rs_tbl[2];
	u16 xmag_0, ymag_0;
	u16 pym_out_w[3];
	u16 pym_out_h[3];
	u16 round_w;
	u16 src_crop_w;
	u16 src_crop_h;
	int i;

	if (aie_cfg->sel_mode == 0) {
		src_crop_w = fd->base_para->crop_width;
		src_crop_h = fd->base_para->crop_height;
	} else {
		src_crop_w = fd->attr_para->crop_width[fd->attr_para->w_idx];
		src_crop_h = fd->attr_para->crop_height[fd->attr_para->w_idx];
	}

	rs_cfg = (u32 *)fd->base_para->fd_rs_cfg_va;

	pym_out_w[0] = fd->base_para->pyramid_width;
	pym_out_w[1] = pym_out_w[0] >> 1;
	pym_out_w[2] = pym_out_w[1] >> 1;

	pym_out_h[0] = pym_out_w[0] * src_crop_h / src_crop_w;
	pym_out_h[1] = pym_out_h[0] >> 1;
	pym_out_h[2] = pym_out_h[1] >> 1;

	for (i = 0; i < 2; i++) {
		rs_tbl[i] = rs_cfg + fd->variant->rs_cfg_size * i;

		rs_tbl[i][RS_IN_0] = (u32)fd->base_para->rs_pym_rst_pa[i][0];
		rs_tbl[i][RS_IN_1] = (u32)fd->base_para->rs_pym_rst_pa[i][1];
		rs_tbl[i][RS_IN_2] = (u32)fd->base_para->rs_pym_rst_pa[i][2];

		rs_tbl[i][RS_OUT_0] =
			(u32)fd->base_para->rs_pym_rst_pa[i + 1][0];
		rs_tbl[i][RS_OUT_1] =
			(u32)fd->base_para->rs_pym_rst_pa[i + 1][1];
		rs_tbl[i][RS_OUT_2] =
			(u32)fd->base_para->rs_pym_rst_pa[i + 1][2];

		rs_tbl[i][RS_INPUT_W_H] =
			(rs_tbl[i][RS_INPUT_W_H] & 0xF800F800) |
			(pym_out_h[i] & 0x7FF) |
			((pym_out_w[i] << 16) & 0x7FF0000);
		rs_tbl[i][RS_OUTPUT_W_H] =
			(rs_tbl[i][RS_OUTPUT_W_H] & 0xF800F800) |
			(pym_out_h[i + 1] & 0x7FF) |
			((pym_out_w[i + 1] << 16) & 0x7FF0000);
		rs_tbl[i][RS_IN_X_Y_SIZE0] =
			aie_combine_u16(pym_out_w[i] - 1, pym_out_h[i] - 1);
		rs_tbl[i][RS_IN_X_Y_SIZE1] =
			aie_combine_u16(pym_out_w[i] - 1, pym_out_h[i] - 1);
		rs_tbl[i][RS_IN_X_Y_SIZE2] =
			aie_combine_u16(pym_out_w[i] - 1, pym_out_h[i] - 1);
		rs_tbl[i][RS_IN_STRIDE0] =
			aie_combine_u16(rs_tbl[i][RS_IN_STRIDE0], pym_out_w[i]);
		rs_tbl[i][RS_IN_STRIDE1] =
			aie_combine_u16(rs_tbl[i][RS_IN_STRIDE1], pym_out_w[i]);
		rs_tbl[i][RS_IN_STRIDE2] =
			aie_combine_u16(rs_tbl[i][RS_IN_STRIDE2], pym_out_w[i]);
		rs_tbl[i][RS_OUT_X_Y_SIZE0] = aie_combine_u16(
			pym_out_w[i + 1] - 1, pym_out_h[i + 1] - 1);
		rs_tbl[i][RS_OUT_X_Y_SIZE1] = aie_combine_u16(
			pym_out_w[i + 1] - 1, pym_out_h[i + 1] - 1);
		rs_tbl[i][RS_OUT_X_Y_SIZE2] = aie_combine_u16(
			pym_out_w[i + 1] - 1, pym_out_h[i + 1] - 1);

		if (i == 0)
			round_w = pym_out_w[i + 1];
		else
			round_w = round_up(pym_out_w[i + 1], 8);

		rs_tbl[i][RS_OUT_STRIDE0] =
			aie_combine_u16(rs_tbl[i][RS_OUT_STRIDE0], round_w);
		rs_tbl[i][RS_OUT_STRIDE1] =
			aie_combine_u16(rs_tbl[i][RS_OUT_STRIDE1], round_w);
		rs_tbl[i][RS_OUT_STRIDE2] =
			aie_combine_u16(rs_tbl[i][RS_OUT_STRIDE2], round_w);

		xmag_0 = 512 * pym_out_w[i] / pym_out_w[i + 1];
		ymag_0 = xmag_0;

		rs_tbl[i][RS_X_Y_MAG] =
			(xmag_0 & 0x3FFF) | ((ymag_0 << 16) & 0x3FFF0000);

		if (fd->variant->hw_version == 31) {
			rs_tbl[i][RS_CON_IN_BA_MSB] = (u32)0x02020202;
			rs_tbl[i][RS_CON_OUT_BA_MSB] = (u32)0x02020202;
		}
	}

	return 0;
}

static int aie_config_network(struct mtk_aie_dev *fd,
			      struct aie_enq_info *aie_cfg)
{
	u16 conv_width;
	u16 conv_height;
	u8 i;
	u8 j;
	u8 uch;
	u8 uloop;
	u16 fd_xsize[4];
	void *fd_cfg;
	u32 *fd_cur_cfg;
	u32 *fd_cur_set;
	u16 pyramid0_out_w;
	u16 pyramid0_out_h;
	u16 pyramid1_out_h;
	u16 pyramid2_out_h;
	u16 input_height;
	u16 out_height = 0;
	u16 out_ysize_plus_1;
	u16 out_ysize_plus_1_stride2;
	u32 src_crop_w;
	u32 src_crop_h;
	struct aie_static_info *pstv;
	u32 cal_x;
	u32 cal_y;

	pstv = &fd->st_info;

	if (aie_cfg->sel_mode == 0) {
		src_crop_w = fd->base_para->crop_width;
		src_crop_h = fd->base_para->crop_height;
	} else {
		src_crop_w = fd->attr_para->crop_width[fd->attr_para->w_idx];
		src_crop_h = fd->attr_para->crop_height[fd->attr_para->w_idx];
	}

	pyramid0_out_w = fd->base_para->pyramid_width;
	pyramid0_out_h = pyramid0_out_w * src_crop_h / src_crop_w;

	pyramid1_out_h = pyramid0_out_h / 2;
	pyramid2_out_h = pyramid1_out_h / 2;

	fd_cfg = fd->base_para->fd_fd_cfg_va;

	for (i = 0; i < fd_loop_num; i++) {
		fd_cur_cfg = (u32 *)fd_cfg + fd->variant->fd_cfg_size * i;
		fd_cur_cfg[FD_INPUT_ROTATE] =
			(fd_cur_cfg[FD_INPUT_ROTATE] & 0xFFFF0FFF) |
			((aie_cfg->rotate_degree << 12) & 0x3000);

		if (i == 0)
			input_height = pyramid2_out_h;
		else if (i == (rpn2_loop_num + 1))
			input_height = pyramid1_out_h;
		else if (i == (rpn1_loop_num + 1))
			input_height = pyramid0_out_h;
		else {
			if (fd_out_stride2_in[i] == 0)
				input_height = out_height;
			else
				input_height = (out_height + 1) / 2;
		}
		if (i == rpn0_loop_num)
			fd->pose_height = input_height;

		if (fd_maxpool[i] == 1 && fd_stride[i] == 1)
			out_height =
				DIV_ROUND_UP(input_height, 2 * fd_maxpool[i]);
		else
			out_height = DIV_ROUND_UP(
				input_height, fd_stride[i] + 2 * fd_maxpool[i]);

		if (i == rpn0_loop_num || i == rpn1_loop_num ||
		    i == rpn2_loop_num) {
			conv_width = fd->base_para->img_width;
			conv_height = fd->base_para->img_height;
			fd_xsize[0] =
				pstv->img_width[i] * 2 * 16 * anchor_en_num[i] -
				1;
			fd_xsize[1] = fd_xsize[2] = fd_xsize[3] =
				pstv->img_width[i] * 2 * 32 * anchor_en_num[i] -
				1;
		} else {
			conv_width =
				DIV_ROUND_UP(pstv->img_width[i], fd_stride[i]);
			conv_height = DIV_ROUND_UP(input_height, fd_stride[i]);

			fd_xsize[0] = fd_xsize[1] = fd_xsize[2] = fd_xsize[3] =
				pstv->input_xsize_plus_1[i] - 1;
		}

		fd_cur_cfg[FD_CONV_WIDTH_MOD6] =
			(fd_cur_cfg[FD_CONV_WIDTH_MOD6] & 0xFF8FFFFF) |
			(((conv_width % 6) << 20) & 0x00700000);
		fd_cur_cfg[FD_CONV_IMG_W_H] =
			aie_combine_u16(conv_height, conv_width);

		fd_cur_cfg[FD_IN_IMG_W_H] =
			aie_combine_u16(input_height, pstv->img_width[i]);
		fd_cur_cfg[FD_OUT_IMG_W_H] =
			aie_combine_u16(out_height, pstv->out_width[i]);

		if (fd_rdma_en[i][0][0] != -1) {
			for (j = 0; j < 4; j++) {
				fd_cur_cfg[FD_IN_X_Y_SIZE0 + 2 * j] =
					aie_combine_u16(fd_xsize[j],
							input_height - 1);

				fd_cur_cfg[FD_IN_STRIDE0_BUS_SIZE0 + 2 * j] =
					aie_combine_stride(
						fd_cur_cfg
							[FD_IN_STRIDE0_BUS_SIZE0 +
							 2 * j],
						fd_xsize[j] + 1);
			}
		}

		out_ysize_plus_1 = out_height - 1;
		out_ysize_plus_1_stride2 = (out_height + 1) / 2 - 1;

		for (j = 0; j < output_WDMA_WRA_num; j++) {
			fd_cur_set = fd_cur_cfg + 2 * j;
			if (!fd_wdma_en[i][j])
				continue;

			if (out_stride_size[i][j] == 1) {
				fd_cur_set[FD_OUT_X_Y_SIZE0] = aie_combine_u16(
					pstv->out_xsize_plus_1[i] - 1,
					out_ysize_plus_1);
				fd_cur_set[FD_OUT_STRIDE0_BUS_SIZE0] =
					aie_combine_stride(
						fd_cur_set
							[FD_OUT_STRIDE0_BUS_SIZE0],
						pstv->out_stride[i]);
			} else if (out_stride_size[i][j] == 2) {
				fd_cur_set[FD_OUT_X_Y_SIZE0] = aie_combine_u16(
					pstv->out_xsize_plus_1_stride2[i] - 1,
					out_ysize_plus_1_stride2);
				fd_cur_set[FD_OUT_STRIDE0_BUS_SIZE0] =
					aie_combine_stride(
						fd_cur_set
							[FD_OUT_STRIDE0_BUS_SIZE0],
						pstv->out_stride_stride2[i]);
			}
		}

		if (i == rpn0_loop_num || i == rpn1_loop_num ||
		    i == rpn2_loop_num) {
			fd_cur_cfg[FD_RPN_SET] =
				aie_combine_u16(fd_cur_cfg[FD_RPN_SET],
						fd->base_para->rpn_anchor_thrd);
		}

		if (i == rpn0_loop_num) {
			cal_x = ((src_crop_w << 10) * 100 /
				(int)fd->base_para->pyramid_width) >> 10;
			cal_y = cal_x * 512 / 100;
			fd_cur_cfg[FD_IMAGE_COORD] =
				(fd_cur_cfg[FD_IMAGE_COORD] & 0xF) |
				((cal_y << 4) & 0x7FFF0);
			fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] = 0;
			if (aie_cfg->en_roi) {
				fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] =
				(aie_cfg->src_roi.x1 - aie_cfg->src_padding.left) |
				(aie_cfg->src_roi.y1 - aie_cfg->src_padding.up) << 16;
			}
		} else if (i == rpn1_loop_num) {
			cal_x = ((src_crop_w << 10) * 100 /
				(int)fd->base_para->pyramid_width) >> 10;
			cal_y = cal_x * 2 * 512 / 100;
			fd_cur_cfg[FD_IMAGE_COORD] =
				(fd_cur_cfg[FD_IMAGE_COORD] & 0xF) |
				((cal_y << 4) & 0x7FFF0);
			fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] = 0;
			if (aie_cfg->en_roi) {
				fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] =
				(aie_cfg->src_roi.x1 - aie_cfg->src_padding.left) |
				(aie_cfg->src_roi.y1 - aie_cfg->src_padding.up) << 16;
			}
		} else if (i == rpn2_loop_num) {
			cal_x = ((src_crop_w << 10) * 100 /
				(int)fd->base_para->pyramid_width) >> 10;
			cal_y = cal_x * 4 * 512 / 100;
			fd_cur_cfg[FD_IMAGE_COORD] =
				(fd_cur_cfg[FD_IMAGE_COORD] & 0xF) |
				((cal_y << 4) & 0x7FFF0);
			fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] = 0;
			if (aie_cfg->en_roi) {
				fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] =
				(aie_cfg->src_roi.x1 - aie_cfg->src_padding.left) |
				(aie_cfg->src_roi.y1 - aie_cfg->src_padding.up) << 16;
			}
		}

		/* IN_FM_BASE_ADR */
		if (i == 0) {
			fd_cur_cfg[FD_IN_0] =
				(u32)(fd->base_para->rs_pym_rst_pa[2][0]);
			fd_cur_cfg[FD_IN_1] =
				(u32)(fd->base_para->rs_pym_rst_pa[2][1]);
			fd_cur_cfg[FD_IN_2] =
				(u32)(fd->base_para->rs_pym_rst_pa[2][2]);
		} else if (i == (rpn2_loop_num + 1)) {
			fd_cur_cfg[FD_IN_0] =
				(u32)(fd->base_para->rs_pym_rst_pa[1][0]);
			fd_cur_cfg[FD_IN_1] =
				(u32)(fd->base_para->rs_pym_rst_pa[1][1]);
			fd_cur_cfg[FD_IN_2] =
				(u32)(fd->base_para->rs_pym_rst_pa[1][2]);
		} else if (i == (rpn1_loop_num + 1)) {
			fd_cur_cfg[FD_IN_0] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][0]);
			fd_cur_cfg[FD_IN_1] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][1]);
			fd_cur_cfg[FD_IN_2] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][2]);
		} else {
			for (j = 0; j < input_WDMA_WRA_num; j++) {
				if (fd_rdma_en[i][j][0] != -1) {
					uloop = fd_rdma_en[i][j][0];
					uch = fd_rdma_en[i][j][1];
					fd_cur_cfg[FD_IN_0 + j] = (u32)(
						fd->dma_para
							->fd_out_hw_pa[uloop]
								      [uch]);
				}
			}
		}

		/* OUT_FM_BASE_ADR */
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (fd_wdma_en[i][j]) {
				fd_cur_cfg[FD_OUT_0 + j] =
					(u32)(fd->dma_para->fd_out_hw_pa[i][j]);
			}
		}

		/* KERNEL_BASE_ADR */
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			if (fd_ker_rdma_size[i][j]) {
				fd_cur_cfg[FD_KERNEL_0 + j] =
					(u32)(fd->dma_para->fd_kernel_pa[i][j]);
			}
		}

		if (fd->variant->hw_version == 31) {
			fd_cur_cfg[FD_CON_IN_BA_MSB] = (u32)0x02020202;
			fd_cur_cfg[FD_CON_OUT_BA_MSB] = (u32)0x02020202;
			fd_cur_cfg[FD_CON_KERNEL_BA_MSB] = (u32)0x00000202;
		}
	}

	return 0;
}

static int aie_config_attr_network(struct mtk_aie_dev *fd,
				   struct aie_enq_info *aie_cfg)
{
	bool isRegressionLoop = false;
	void *fd_cfg;
	u32 *fd_cur_cfg;
	u16 fd_input_ht, fd_output_ht = 0x0;
	u16 fd_out_y[4];
	u8 i, j;
	u8 uloop, uch, uidx;
	u16 pyramid0_out_w, pyramid0_out_h;
	int fd_conv_ht;
	u16 src_crop_w;
	u16 src_crop_h;

	src_crop_w = fd->attr_para->crop_width[fd->attr_para->w_idx];
	src_crop_h = fd->attr_para->crop_height[fd->attr_para->w_idx];

	pyramid0_out_w = ATTR_MODE_PYRAMID_WIDTH;
	pyramid0_out_h = pyramid0_out_w * src_crop_h / src_crop_w;

	fd_cfg = fd->base_para->attr_fd_cfg_va[fd->attr_para->w_idx];

	for (i = 0; i < attr_loop_num; i++) {
		fd_cur_cfg = (u32 *)fd_cfg + fd->variant->fd_cfg_size * i;
		fd_cur_cfg[FD_INPUT_ROTATE] =
			(fd_cur_cfg[FD_INPUT_ROTATE] & 0xFFFF0FFF) |
			((aie_cfg->rotate_degree << 12) & 0x3000);
		if (i == 0)
			fd_input_ht = pyramid0_out_h;
		else {
			if (attr_out_stride2_as_in[i] == 0)
				fd_input_ht = fd_output_ht;
			else if (attr_out_stride2_as_in[i] == 1)
				fd_input_ht = (fd_output_ht + 1) / 2;
		}
		fd_output_ht = DIV_ROUND_UP(fd_input_ht,
					    attr_fd_stride[i] +
						    2 * attr_fd_maxpool[i]);
		fd_conv_ht = DIV_ROUND_UP(fd_input_ht, attr_fd_stride[i]);

		fd_cur_cfg[FD_CONV_IMG_W_H] =
			(fd_cur_cfg[FD_CONV_IMG_W_H] & 0xFFFF0000) |
			(fd_conv_ht & 0xFFFF);
		fd_cur_cfg[FD_IN_IMG_W_H] =
			(fd_cur_cfg[FD_IN_IMG_W_H] & 0xFFFF0000) |
			(fd_input_ht & 0xFFFF);
		fd_cur_cfg[FD_OUT_IMG_W_H] =
			(fd_cur_cfg[FD_OUT_IMG_W_H] & 0xFFFF0000) |
			(fd_output_ht & 0xFFFF);
		fd_cur_cfg[FD_IN_X_Y_SIZE0] = aie_combine_u16(
			fd_cur_cfg[FD_IN_X_Y_SIZE0], fd_input_ht - 1);
		fd_cur_cfg[FD_IN_X_Y_SIZE1] = aie_combine_u16(
			fd_cur_cfg[FD_IN_X_Y_SIZE1], fd_input_ht - 1);
		fd_cur_cfg[FD_IN_X_Y_SIZE2] = aie_combine_u16(
			fd_cur_cfg[FD_IN_X_Y_SIZE2], fd_input_ht - 1);
		fd_cur_cfg[FD_IN_X_Y_SIZE3] = aie_combine_u16(
			fd_cur_cfg[FD_IN_X_Y_SIZE3], fd_input_ht - 1);

		isRegressionLoop = (i == age_out_rgs || i == gender_out_rgs ||
				    i == indian_out_rgs || i == race_out_rgs);

		if (isRegressionLoop) {
			fd_out_y[0] = 0;
			fd_out_y[1] = 0;
			fd_out_y[2] = 0;
			fd_out_y[3] = 0;
		} else {
			fd_out_y[0] = fd_output_ht - 1;
			fd_out_y[1] = fd_output_ht - 1;
			if (attr_out_2size[i] == 0) {
				fd_out_y[2] = fd_output_ht - 1;
				fd_out_y[3] = fd_output_ht - 1;
			} else {
				fd_out_y[2] = (fd_output_ht + 1) / 2 - 1;
				fd_out_y[3] = (fd_output_ht + 1) / 2 - 1;
			}
		}

		for (j = 0; j < 4; j++)
			fd_cur_cfg[FD_OUT_X_Y_SIZE0 + 2 * j] = aie_combine_u16(
				fd_cur_cfg[FD_OUT_X_Y_SIZE0 + 2 * j],
				fd_out_y[j]);

		/* IN_FM_BASE_ADR */
		if (i == 0) {
			fd_cur_cfg[FD_IN_0] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][0]);
			fd_cur_cfg[FD_IN_1] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][1]);
			fd_cur_cfg[FD_IN_2] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][2]);
		} else {
			for (j = 0; j < input_WDMA_WRA_num; j++) {
				if (attr_rdma_en[i][j][0] != -1) {
					uloop = attr_rdma_en[i][j][0];
					uch = attr_rdma_en[i][j][1];
					fd_cur_cfg[FD_IN_0 + j] = (u32)(
						fd->dma_para
							->attr_out_hw_pa[uloop]
									[uch]);
				}
			}
		}

		/* OUT_FM_BASE_ADR */
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (attr_wdma_en[i][j]) {
				uidx = fd->attr_para->w_idx;
				if (i == age_out_rgs && j == 0)
					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para
							->age_out_hw_pa[uidx]);
				else if (i == gender_out_rgs && j == 0)
					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para->gender_out_hw_pa
							[uidx]);
				else if (i == indian_out_rgs && j == 0)
					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para->isIndian_out_hw_pa
							[uidx]);
				else if (i == race_out_rgs && j == 0)
					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para
							->race_out_hw_pa[uidx]);
				else
					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para
							->attr_out_hw_pa[i][j]);
			}
		}

		/* KERNEL_BASE_ADR */
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			fd_cur_cfg[FD_KERNEL_0 + j] =
				(u32)(fd->dma_para->attr_kernel_pa[i][j]);
		}

		if (fd->variant->hw_version == 31) {
			fd_cur_cfg[FD_CON_IN_BA_MSB] = (u32)0x02020202;
			fd_cur_cfg[FD_CON_OUT_BA_MSB] = (u32)0x02020202;
			fd_cur_cfg[FD_CON_KERNEL_BA_MSB] = (u32)0x00000202;
		}
	}
	return 0;
}

static int aie_config_dram(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	int ret = 0;

	if (aie_cfg->sel_mode == 0) { /* FDMODE */
		ret = aie_config_y2r(fd, aie_cfg, aie_cfg->sel_mode);
		if (ret)
			return ret;

		ret = aie_config_rs(fd, aie_cfg);
		if (ret)
			return ret;

		ret = aie_config_network(fd, aie_cfg);
		if (ret)
			return ret;

	} else if (aie_cfg->sel_mode == 1) { /* ATTRIBUTEMODE */
		ret = aie_config_y2r(fd, aie_cfg, aie_cfg->sel_mode);
		if (ret)
			return ret;

		ret = aie_config_attr_network(fd, aie_cfg);
		if (ret)
			return ret;
	}

	return ret;
}

void aie_reset(struct mtk_aie_dev *fd)
{
	writel(0x30000, fd->fd_base + AIE_START_REG);
	writel(0x0, fd->fd_base + AIE_START_REG);
}

int aie_init(struct mtk_aie_dev *fd, struct aie_init_info init_info)
{
	int ret = -ENOMEM;
	int err_tag = 0;

	fd->fd_state = STATE_NA;
	fd->fd_mem_size = 0;

	fd->base_para = kmalloc(sizeof(struct aie_para), GFP_KERNEL);
	if (fd->base_para == NULL)
		return ret;

	fd->attr_para = kmalloc(sizeof(struct aie_attr_para), GFP_KERNEL);
	if (fd->attr_para == NULL)
		goto attr_para_fail;

	fd->dma_para = kmalloc(sizeof(struct aie_fd_dma_para), GFP_KERNEL);
	if (fd->dma_para == NULL)
		goto dma_para_fail;

	if (fd->variant->fld_enable) {
		fd->fld_para = kmalloc(sizeof(struct aie_fd_fld_para), GFP_KERNEL);
		if (fd->fld_para == NULL)
			goto fld_para_fail;
	}

	fd->base_para->rpn_anchor_thrd = init_info.feature_threshold;
	fd->base_para->pyramid_width = init_info.pyramid_width;
	fd->base_para->pyramid_height = init_info.pyramid_height;
	fd->base_para->max_pyramid_width = init_info.pyramid_width;
	fd->base_para->max_pyramid_height = init_info.pyramid_height;

	memset(&fd->st_info, 0, sizeof(fd->st_info));
	aie_init_table(fd, fd->base_para->max_pyramid_width,
		       fd->base_para->max_pyramid_height);
	aie_get_data_size(fd, init_info.max_img_width,
			  init_info.max_img_height);
	ret = aie_alloc_dram_buf(fd);
	if (ret)
		goto dram_fail;

	ret = aie_alloc_output_buf(fd);
	if (ret)
		goto output_fail;

	ret = aie_alloc_fddma_buf(fd);
	if (ret)
		goto fddma_fail;

	if (fd->variant->fld_enable) {
		ret = aie_alloc_fld_buf(fd);
		if (ret)
			goto fdfld_fail;
	}

	aie_arrange_fddma_buf(fd);
	aie_arrange_kernel_buf(fd);
	aie_arrange_attrdma_buf(fd);
	aie_arrange_result_dma_buf(fd);

	if (fd->variant->fld_enable)
		aie_arrange_fld_buf(fd);

	ret = aie_load_fw(fd);
	if (ret)
		goto load_fw_fail;

	fd->attr_para->r_idx = 0;
	fd->attr_para->w_idx = 0;

	fd->fd_state = STATE_INIT;

	dev_info(fd->dev, "%s: fd_mem_size(%d)\n", __func__, fd->fd_mem_size);

	return ret;

load_fw_fail:
	if (fd->variant->fld_enable) {
		aie_free_fld_buf(fd);
		err_tag++;
	}
fdfld_fail:
	aie_free_fddma_buf(fd);
	err_tag++;

fddma_fail:
	aie_free_output_buf(fd);
	err_tag++;

output_fail:
	aie_free_dram_buf(fd);
	err_tag++;

dram_fail:
	if (fd->variant->fld_enable) {
		kfree(fd->fld_para);
		err_tag++;
	}

fld_para_fail:
	kfree(fd->dma_para);
	err_tag++;

dma_para_fail:
	kfree(fd->attr_para);
	err_tag++;

attr_para_fail:
	kfree(fd->base_para);
	err_tag++;

	dev_info(fd->dev, "Failed to init aie: %d\n", err_tag);

	return ret;
}

void aie_uninit(struct mtk_aie_dev *fd)
{
	fd->fd_state = STATE_NA;

	aie_free_dram_buf(fd);
	aie_free_output_buf(fd);
	aie_free_fddma_buf(fd);

	if (fd->variant->fld_enable)
		aie_free_fld_buf(fd);

	kfree(fd->base_para);
	kfree(fd->attr_para);
	kfree(fd->dma_para);

	if (fd->variant->fld_enable)
		kfree(fd->fld_para);
}

int aie_prepare(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	int ret = 0;

	if (fd->fd_state != STATE_INIT) {
		dev_info(fd->dev, "%s fd state fail: %d\n",
			 __func__, fd->fd_state);
		return -EINVAL;
	}

	if (fd->variant->fld_enable) {
		if (aie_cfg->sel_mode == 2) { /* FLD don't need to prepare buf */
			dev_info(fd->dev, "FLD, Mode: %d", aie_cfg->sel_mode);
			return ret;
		}
	}

	memset(&fd->reg_cfg, 0, sizeof(fd->reg_cfg));

	if (aie_cfg->pyramid_base_width == 0) {
		fd->base_para->pyramid_width =
			fd->base_para->max_pyramid_width;
		fd->base_para->pyramid_height =
			fd->base_para->max_pyramid_height;
		fd->base_para->number_of_pyramid = 3;
	} else {
		if (aie_cfg->pyramid_base_width >
			fd->base_para->max_pyramid_width ||
		    aie_cfg->pyramid_base_height >
			fd->base_para->max_pyramid_height ||
		    aie_cfg->number_of_pyramid > 3 ||
		    aie_cfg->number_of_pyramid <= 0) {
			dev_info(fd->dev, "err: base w: %d, h: %d, num: %d\n",
			    aie_cfg->pyramid_base_width,
			    aie_cfg->pyramid_base_height,
			    aie_cfg->number_of_pyramid);
			dev_info(fd->dev, "err: max w: %d, h: %d\n",
			    fd->base_para->max_pyramid_width,
			    fd->base_para->max_pyramid_height);

			return -EINVAL;
		}

		fd->base_para->pyramid_height =
			fd->base_para->max_pyramid_height;
		fd->base_para->number_of_pyramid =
			aie_cfg->number_of_pyramid;
		if (aie_cfg->pyramid_base_width !=
			fd->base_para->pyramid_width) {
			dev_dbg(fd->dev, "pre: %d, cur: %d, num: %d\n",
				fd->base_para->pyramid_width,
				aie_cfg->pyramid_base_width,
				fd->base_para->number_of_pyramid);
			fd->base_para->pyramid_width =
				aie_cfg->pyramid_base_width;
			aie_update_table(
				fd, fd->base_para->pyramid_width,
				fd->base_para->pyramid_height);
			aie_update_fddma_buf(fd);
		}
	}

	if ((aie_cfg->src_img_width > fd->base_para->max_img_width) ||
	    (aie_cfg->src_img_height > fd->base_para->max_img_height)) {
		dev_info(
			fd->dev,
			"AIE error: Enque Size error, Src_WD: %d, Src_HT: %d\n",
			aie_cfg->src_img_width, aie_cfg->src_img_height);

		dev_info(fd->dev, "AIE error: MAX_Src_WD: %d, MAX_Src_HT: %d\n",
			 fd->base_para->max_img_width,
			 fd->base_para->max_img_height);
		return -EINVAL;
	}

	aie_reset_output_buf(fd, aie_cfg);

	fd->reg_cfg.fd_mode = aie_cfg->sel_mode;
	if (aie_cfg->sel_mode == 0) { /* FDMODE */
		fd->reg_cfg.rs_adr = (u32)fd->base_para->fd_rs_cfg_pa;
		fd->reg_cfg.yuv2rgb_adr = (u32)fd->base_para->fd_yuv2rgb_cfg_pa;
		fd->reg_cfg.fd_adr = (u32)fd->base_para->fd_fd_cfg_pa +
			fd->variant->fd_cfg_size * 4 * fd_loop_num /
			3 * (3 - aie_cfg->number_of_pyramid);

	} else if (aie_cfg->sel_mode == 1) { /* ATTRMODE */
		fd->reg_cfg.yuv2rgb_adr =
			(u32)fd->base_para
				->attr_yuv2rgb_cfg_pa[fd->attr_para->w_idx];
		fd->reg_cfg.fd_adr =
			(u32)fd->base_para
				->attr_fd_cfg_pa[fd->attr_para->w_idx];
	} else {
		dev_info(fd->dev, "AIE error, Mode: %d", aie_cfg->sel_mode);
		return -EINVAL;
	}

	ret = aie_update_cfg(fd, aie_cfg);
	if (ret)
		return ret;

	ret = aie_config_dram(fd, aie_cfg);
	if (ret)
		return ret;

	if (aie_cfg->sel_mode == 1) { /* ATTRMODE */
		fd->attr_para->w_idx =
			(fd->attr_para->w_idx + 1) % MAX_ENQUE_FRAME_NUM;
	}

	return ret;
}

void aie_execute(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	unsigned int loop_num = 0;
	unsigned int loop_reg_val = 0;
	unsigned int i = 0;

	if (aie_cfg->sel_mode == 0) {
		writel(0x0, fd->fd_base + AIE_START_REG);
		writel(0x00000111, fd->fd_base + AIE_ENABLE_REG);
		loop_num = fd_loop_num / 3 * (aie_cfg->number_of_pyramid);
		loop_reg_val = (loop_num << 8) |
			(aie_cfg->number_of_pyramid - 1);
		writel(loop_reg_val, fd->fd_base + AIE_LOOP_REG);
		writel(0x1, fd->fd_base + AIE_INT_EN_REG);
		writel(fd->reg_cfg.rs_adr,
		       fd->fd_base + AIE_RS_CON_BASE_ADR_REG);
		writel(fd->reg_cfg.fd_adr,
		       fd->fd_base + AIE_FD_CON_BASE_ADR_REG);
		writel(fd->reg_cfg.yuv2rgb_adr,
		       fd->fd_base + AIE_YUV2RGB_CON_BASE_ADR_REG);

		if (fd->variant->hw_version == 31) {
			writel(0x00000002,
			       fd->fd_base + AIE_YUV2RGB_CON_BASE_ADR_MSB);
			writel(0x00000002,
			       fd->fd_base + AIE_RS_CON_BASE_ADR_MSB);
			writel(0x00000002,
			       fd->fd_base + AIE_FD_CON_BASE_ADR_MSB);
		}

		writel(0x1, fd->fd_base + AIE_START_REG);
	} else if (aie_cfg->sel_mode == 1) {
		writel(0x0, fd->fd_base + AIE_START_REG);
		writel(0x00000101, fd->fd_base + AIE_ENABLE_REG);
		writel(0x00001A00, fd->fd_base + AIE_LOOP_REG);
		writel(0x1, fd->fd_base + AIE_INT_EN_REG);
		writel(fd->reg_cfg.rs_adr,
		       fd->fd_base + AIE_RS_CON_BASE_ADR_REG);
		writel(fd->reg_cfg.fd_adr,
		       fd->fd_base + AIE_FD_CON_BASE_ADR_REG);
		writel(fd->reg_cfg.yuv2rgb_adr,
		       fd->fd_base + AIE_YUV2RGB_CON_BASE_ADR_REG);

		if (fd->variant->hw_version == 31) {
			writel(0x00000002,
			       fd->fd_base + AIE_YUV2RGB_CON_BASE_ADR_MSB);
			writel(0x00000002,
			       fd->fd_base + AIE_RS_CON_BASE_ADR_MSB);
			writel(0x00000002,
			       fd->fd_base + AIE_FD_CON_BASE_ADR_MSB);
		}

		writel(0x1, fd->fd_base + AIE_START_REG);
	} else if (aie_cfg->sel_mode == 2) {
		if (fd->variant->fld_enable) {
			writel(0x10, fd->fd_base + AIE_START_REG);
			writel(0x00011111, fd->fd_base + AIE_DMA_CTL_REG);
			writel(0x01111111, fd->fd_base + FLD_EN);
			writel(0x1, fd->fd_base + AIE_INT_EN_REG);
			for (i = 0; i < aie_cfg->fld_face_num; i++) {
				writel(aie_cfg->src_img_addr,
				       fd->fd_base + FLD_BASE_ADDR_FACE_0 + i * 0x4);
				writel(aie_cfg->fld_input[i].fld_in_crop_x1 << 16 |
				       aie_cfg->fld_input[i].fld_in_crop_y1,
				       fd->fd_base + fld_face_info_0[i]);
				writel(aie_cfg->fld_input[i].fld_in_crop_x2 << 16 |
				       aie_cfg->fld_input[i].fld_in_crop_y2,
				       fd->fd_base + fld_face_info_1[i]);
				writel(aie_cfg->fld_input[i].fld_in_rip << 4 |
				       aie_cfg->fld_input[i].fld_in_rop,
				       fd->fd_base + fld_face_info_2[i]);
			}

			writel(aie_cfg->fld_face_num << 28 | fld_forest << 16 | fld_point,
			       fd->fd_base + FLD_MODEL_PARA1);
			writel(13 << 16 | 0xfe9, fd->fd_base + FLD_MODEL_PARA14);

			writel(aie_cfg->src_img_width << 16 | aie_cfg->src_img_height,
			       fd->fd_base + FLD_SRC_WD_HT);

			/*input settings*/
			writel(0x007c003f, fd->fd_base + FLD_PL_IN_SIZE_0);
			writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_0);
			writel(0x007c003f, fd->fd_base + FLD_PL_IN_SIZE_1);
			writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_1);
			writel(0x0016003f, fd->fd_base + FLD_PL_IN_SIZE_2_0);
			writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_2_0);
			writel(0x0013003f, fd->fd_base + FLD_PL_IN_SIZE_2_1);
			writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_2_1);
			writel(0x0013003f, fd->fd_base + FLD_PL_IN_SIZE_2_2);
			writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_2_2);
			writel(0x00a6001f, fd->fd_base + FLD_PL_IN_SIZE_3);
			writel(0x0020000f, fd->fd_base + FLD_PL_IN_STRIDE_3);

			/*output setting*/
			writel((2400 * aie_cfg->fld_face_num - 1) << 16 | 127,
			       fd->fd_base + FLD_SH_IN_SIZE_0);
			writel(0x0010000f, fd->fd_base + FLD_SH_IN_STRIDE_0);
			writel(fd->fld_para->fld_output_pa[0],
			       fd->fd_base + FLD_TR_OUT_BASE_ADDR_0);
			writel((aie_cfg->fld_face_num - 1) << 16 | 0x6f,
			       fd->fd_base + FLD_TR_OUT_SIZE_0);
			writel(0x0070000f, fd->fd_base + FLD_TR_OUT_STRIDE_0);
			writel(fd->fld_para->fld_output_pa[0],
			       fd->fd_base + FLD_PP_OUT_BASE_ADDR_0);
			writel((aie_cfg->fld_face_num - 1) << 16 | 0x6f,
			       fd->fd_base + FLD_PP_OUT_SIZE_0);
			writel(0x0070000f, fd->fd_base + FLD_PP_OUT_STRIDE_0);

			/*cv score*/
			writel(0x00000001, fd->fd_base + FLD_BS_BIAS);
			writel(0x0000b835, fd->fd_base + FLD_CV_FM_RANGE_0); //8E8
			writel(0xffff5cba, fd->fd_base + FLD_CV_FM_RANGE_1); //8EC
			writel(0x00005ed5, fd->fd_base + FLD_CV_PM_RANGE_0); //8F0
			writel(0xffff910d, fd->fd_base + FLD_CV_PM_RANGE_1); //8F4
			writel(0x0000031e, fd->fd_base + FLD_BS_RANGE_0); //8F8
			writel(0xfffffcae, fd->fd_base + FLD_BS_RANGE_1); //8FC

			/* 6 steps */
			writel(fd->fld_para->fld_step_pa[fld_step_blink][14],
			       fd->fd_base + FLD_BS_IN_BASE_ADDR_14);

			writel(fd->fld_para->fld_step_pa[fld_step_cv][0],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_0);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][1],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_1);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][2],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_2);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][3],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_3);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][4],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_4);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][5],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_5);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][6],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_6);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][7],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_7);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][8],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_8);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][9],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_9);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][10],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_10);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][11],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_11);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][12],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_12);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][13],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_13);
			writel(fd->fld_para->fld_step_pa[fld_step_cv][14],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_2_14);

			writel(fd->fld_para->fld_step_pa[fld_step_fp][0],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_0);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][1],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_1);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][2],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_2);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][3],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_3);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][4],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_4);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][5],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_5);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][6],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_6);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][7],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_7);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][8],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_8);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][9],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_9);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][10],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_10);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][11],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_11);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][12],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_12);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][13],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_13);
			writel(fd->fld_para->fld_step_pa[fld_step_fp][14],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_3_14);

			writel(fd->fld_para->fld_step_pa[fld_step_leaf][0],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_0);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][1],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_1);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][2],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_2);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][3],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_3);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][4],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_4);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][5],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_5);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][6],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_6);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][7],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_7);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][8],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_8);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][9],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_9);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][10],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_10);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][11],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_11);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][12],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_12);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][13],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_13);
			writel(fd->fld_para->fld_step_pa[fld_step_leaf][14],
			       fd->fd_base + FLD_SH_IN_BASE_ADDR_14);

			writel(fd->fld_para->fld_step_pa[fld_step_km02][0],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_0);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][1],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_1);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][2],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_2);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][3],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_3);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][4],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_4);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][5],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_5);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][6],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_6);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][7],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_7);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][8],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_8);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][9],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_9);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][10],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_10);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][11],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_11);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][12],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_12);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][13],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_13);
			writel(fd->fld_para->fld_step_pa[fld_step_km02][14],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_0_14);

			writel(fd->fld_para->fld_step_pa[fld_step_km13][0],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_0);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][1],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_1);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][2],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_2);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][3],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_3);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][4],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_4);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][5],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_5);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][6],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_6);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][7],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_7);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][8],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_8);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][9],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_9);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][10],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_10);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][11],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_11);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][12],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_12);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][13],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_13);
			writel(fd->fld_para->fld_step_pa[fld_step_km13][14],
			       fd->fd_base + FLD_PL_IN_BASE_ADDR_1_14);

			/* */
			writel(0x22222222, fd->fd_base + FLD_PL_IN_BASE_ADDR_0_0_7_MSB);
			writel(0x02222222, fd->fd_base + FLD_PL_IN_BASE_ADDR_0_8_15_MSB);

			writel(0x22222222, fd->fd_base + FLD_PL_IN_BASE_ADDR_1_0_7_MSB);
			writel(0x02222222, fd->fd_base + FLD_PL_IN_BASE_ADDR_1_8_15_MSB);

			writel(0x22222222, fd->fd_base + FLD_PL_IN_BASE_ADDR_2_0_7_MSB);
			writel(0x02222222, fd->fd_base + FLD_PL_IN_BASE_ADDR_2_8_15_MSB);

			writel(0x22222222, fd->fd_base + FLD_PL_IN_BASE_ADDR_3_0_7_MSB);
			writel(0x02222222, fd->fd_base + FLD_PL_IN_BASE_ADDR_3_8_15_MSB);

			writel(0x22222222, fd->fd_base + FLD_SH_IN_BASE_ADDR_0_7_MSB);
			writel(0x02222222, fd->fd_base + FLD_SH_IN_BASE_ADDR_8_15_MSB);

			writel(0x02000000, fd->fd_base + FLD_BS_IN_BASE_ADDR_8_15_MSB);

			writel(0x22222222, fd->fd_base + FLD_BASE_ADDR_FACE_0_7_MSB);
			writel(0x02222222, fd->fd_base + FLD_BASE_ADDR_FACE_8_14_MSB);
			writel(0x00000002, fd->fd_base + FLD_TR_OUT_BASE_ADDR_0_MSB);
			writel(0x00000002, fd->fd_base + FLD_PP_OUT_BASE_ADDR_0_MSB);

			/*fld mode + trigger start*/
			writel(0x11, fd->fd_base + AIE_START_REG);
		}
	}
}

void aie_execute_pose(struct mtk_aie_dev *fd)
{
	writel(0x00000100, fd->fd_base + AIE_ENABLE_REG);
	writel(0x00000300, fd->fd_base + AIE_LOOP_REG);
	writel(0x1, fd->fd_base + AIE_INT_EN_REG);
	writel(fd->reg_cfg.fd_pose_adr, fd->fd_base + AIE_FD_CON_BASE_ADR_REG);
	writel(0x1, fd->fd_base + AIE_START_REG);
}

void aie_irqhandle(struct mtk_aie_dev *fd)
{
	int status;

	writel(0x0, fd->fd_base + AIE_START_REG);

	/* interrupt read clear */
	status = readl(fd->fd_base + AIE_INT_REG);
}

/* return aie_cfg to user space */
void aie_get_fd_result(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	void *fd_pym_result[PYM_NUM];
	u32 fd_result_hw, fd_result_1_hw;
	u32 fd_total_num;
	u32 fd_pyramid_num[PYM_NUM];

	aie_cfg->sel_mode = fd->base_para->sel_mode;
	aie_cfg->rotate_degree = fd->base_para->rotate_degree;
	aie_cfg->src_img_addr = fd->base_para->src_img_addr;
	aie_cfg->src_img_addr_uv = fd->base_para->src_img_addr_uv;
	aie_cfg->src_img_width = fd->base_para->img_width;
	aie_cfg->src_img_height = fd->base_para->img_height;
	aie_cfg->src_img_fmt = fd->base_para->src_img_fmt;
	aie_cfg->fd_version = FD_VERSION;
	aie_cfg->attr_version = ATTR_VERSION;

	aie_cfg->irq_status = readl(fd->fd_base + AIE_INT_EN_REG);

	fd_pym_result[0] = fd->dma_para->fd_out_hw_va[rpn0_loop_num][0];
	fd_pym_result[1] = fd->dma_para->fd_out_hw_va[rpn1_loop_num][0];
	fd_pym_result[2] = fd->dma_para->fd_out_hw_va[rpn2_loop_num][0];

	fd_result_hw = fd->reg_cfg.hw_result;
	fd_result_1_hw = fd->reg_cfg.hw_result1;
	fd_total_num = fd_result_hw & 0xFFF;
	fd_pyramid_num[0] = (fd_result_hw & 0xFFF0000) >> 16;
	fd_pyramid_num[1] = fd_result_1_hw & 0xFFF;
	fd_pyramid_num[2] = (fd_result_1_hw & 0xFFF0000) >> 16;

	aie_cfg->fd_out.fd_total_num = fd_total_num;
	aie_cfg->fd_out.fd_pyramid0_num = fd_pyramid_num[0];
	aie_cfg->fd_out.fd_pyramid1_num = fd_pyramid_num[1];
	aie_cfg->fd_out.fd_pyramid2_num = fd_pyramid_num[2];

	memcpy(aie_cfg->fd_out.rpn31_rlt,
	       fd->dma_para->fd_out_hw_va[rpn2_loop_num][0],
	       sizeof(aie_cfg->fd_out.rpn31_rlt));
	memcpy(aie_cfg->fd_out.rpn63_rlt,
	       fd->dma_para->fd_out_hw_va[rpn1_loop_num][0],
	       sizeof(aie_cfg->fd_out.rpn63_rlt));
	memcpy(aie_cfg->fd_out.rpn95_rlt,
	       fd->dma_para->fd_out_hw_va[rpn0_loop_num][0],
	       sizeof(aie_cfg->fd_out.rpn95_rlt));
}

void aie_get_attr_result(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	u32 *attr_race_result, *attr_gender_result;
	u32 *attr_age_result, *attr_isIndian_result;

	aie_cfg->sel_mode = fd->attr_para->sel_mode[fd->attr_para->r_idx];
	aie_cfg->rotate_degree =
		fd->attr_para->rotate_degree[fd->attr_para->r_idx];
	aie_cfg->src_img_addr =
		fd->attr_para->src_img_addr[fd->attr_para->r_idx];
	aie_cfg->src_img_addr_uv =
		fd->attr_para->src_img_addr_uv[fd->attr_para->r_idx];
	aie_cfg->src_img_width = fd->attr_para->img_width[fd->attr_para->r_idx];
	aie_cfg->src_img_height =
		fd->attr_para->img_height[fd->attr_para->r_idx];
	aie_cfg->src_img_fmt = fd->attr_para->src_img_fmt[fd->attr_para->r_idx];
	aie_cfg->fd_version = FD_VERSION;
	aie_cfg->attr_version = ATTR_VERSION;

	aie_cfg->irq_status = readl(fd->fd_base + AIE_INT_EN_REG);

	/* 64 feature * 32 bytes */
	attr_age_result =
		(u32 *)fd->dma_para->age_out_hw_va[fd->attr_para->r_idx];
	attr_gender_result =
		(u32 *)fd->dma_para->gender_out_hw_va[fd->attr_para->r_idx];
	attr_isIndian_result =
		(u32 *)fd->dma_para->isIndian_out_hw_va[fd->attr_para->r_idx];
	attr_race_result =
		(u32 *)fd->dma_para->race_out_hw_va[fd->attr_para->r_idx];

	memcpy(aie_cfg->attr_out.rpn17_rlt, attr_age_result,
	       sizeof(aie_cfg->attr_out.rpn17_rlt));
	memcpy(aie_cfg->attr_out.rpn20_rlt, attr_gender_result,
	       sizeof(aie_cfg->attr_out.rpn20_rlt));
	memcpy(aie_cfg->attr_out.rpn22_rlt, attr_isIndian_result,
	       sizeof(aie_cfg->attr_out.rpn22_rlt));
	memcpy(aie_cfg->attr_out.rpn25_rlt, attr_race_result,
	       sizeof(aie_cfg->attr_out.rpn25_rlt));

	fd->attr_para->r_idx = (fd->attr_para->r_idx + 1) % MAX_ENQUE_FRAME_NUM;
}

void aie_get_fld_result(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	aie_cfg->irq_status = readl(fd->fd_base + AIE_INT_EN_REG);

	memcpy(aie_cfg->fld_out.fld_rlt,
	       fd->fld_para->fld_output_va[0],
	       sizeof(aie_cfg->fld_out.fld_rlt));
}

