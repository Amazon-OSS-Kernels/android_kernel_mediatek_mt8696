// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#define LOG_TAG "VDP"

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched/clock.h>
#include <uapi/linux/time.h>
#include <linux/time64.h>

#include "disp_hw_mgr.h"
#include "disp_adl_if.h"
#include "disp_fg_if.h"
#include "fg_hal.h"
#include "disp_info.h"
#include "disp_fg_drv.h"

static MS_U8 frame_num;
static const int gauss_bits = 11;
static MS_U16 random_register;  // random number generator register

static bool fg_force_bypass;
static bool fg_sw_auto_reg_filter = true;

static const int gaussian_sequence[2048] = {
	56,    568,   -180,  172,   124,   -84,   172,   -64,   -900,  24,   820,
	224,   1248,  996,   272,   -8,    -916,  -388,  -732,  -104,  -188, 800,
	112,   -652,  -320,  -376,  140,   -252,  492,   -168,  44,    -788, 588,
	-584,  500,   -228,  12,    680,   272,   -476,  972,   -100,  652,  368,
	432,   -196,  -720,  -192,  1000,  -332,  652,   -136,  -552,  -604, -4,
	192,   -220,  -136,  1000,  -52,   372,   -96,   -624,  124,   -24,  396,
	540,   -12,   -104,  640,   464,   244,   -208,  -84,   368,   -528, -740,
	248,   -968,  -848,  608,   376,   -60,   -292,  -40,   -156,  252,  -292,
	248,   224,   -280,  400,   -244,  244,   -60,   76,    -80,   212,  532,
	340,   128,   -36,   824,   -352,  -60,   -264,  -96,   -612,  416,  -704,
	220,   -204,  640,   -160,  1220,  -408,  900,   336,   20,    -336, -96,
	-792,  304,   48,    -28,   -1232, -1172, -448,  104,   -292,  -520, 244,
	60,    -948,  0,     -708,  268,   108,   356,   -548,  488,   -344, -136,
	488,   -196,  -224,  656,   -236,  -1128, 60,    4,     140,   276,  -676,
	-376,  168,   -108,  464,   8,     564,   64,    240,   308,   -300, -400,
	-456,  -136,  56,    120,   -408,  -116,  436,   504,   -232,  328,  844,
	-164,  -84,   784,   -168,  232,   -224,  348,   -376,  128,   568,  96,
	-1244, -288,  276,   848,   832,   -360,  656,   464,   -384,  -332, -356,
	728,   -388,  160,   -192,  468,   296,   224,   140,   -776,  -100, 280,
	4,     196,   44,    -36,   -648,  932,   16,    1428,  28,    528,  808,
	772,   20,    268,   88,    -332,  -284,  124,   -384,  -448,  208,  -228,
	-1044, -328,  660,   380,   -148,  -300,  588,   240,   540,   28,   136,
	-88,   -436,  256,   296,   -1000, 1400,  0,     -48,   1056,  -136, 264,
	-528,  -1108, 632,   -484,  -592,  -344,  796,   124,   -668,  -768, 388,
	1296,  -232,  -188,  -200,  -288,  -4,    308,   100,   -168,  256,  -500,
	204,   -508,  648,   -136,  372,   -272,  -120,  -1004, -552,  -548, -384,
	548,   -296,  428,   -108,  -8,    -912,  -324,  -224,  -88,   -112, -220,
	-100,  996,   -796,  548,   360,   -216,  180,   428,   -200,  -212, 148,
	96,    148,   284,   216,   -412,  -320,  120,   -300,  -384,  -604, -572,
	-332,  -8,    -180,  -176,  696,   116,   -88,   628,   76,    44,   -516,
	240,   -208,  -40,   100,   -592,  344,   -308,  -452,  -228,  20,   916,
	-1752, -136,  -340,  -804,  140,   40,    512,   340,   248,   184,  -492,
	896,   -156,  932,   -628,  328,   -688,  -448,  -616,  -752,  -100, 560,
	-1020, 180,   -800,  -64,   76,    576,   1068,  396,   660,   552,  -108,
	-28,   320,   -628,  312,   -92,   -92,   -472,  268,   16,    560,  516,
	-672,  -52,   492,   -100,  260,   384,   284,   292,   304,   -148, 88,
	-152,  1012,  1064,  -228,  164,   -376,  -684,  592,   -392,  156,  196,
	-524,  -64,   -884,  160,   -176,  636,   648,   404,   -396,  -436, 864,
	424,   -728,  988,   -604,  904,   -592,  296,   -224,  536,   -176, -920,
	436,   -48,   1176,  -884,  416,   -776,  -824,  -884,  524,   -548, -564,
	-68,   -164,  -96,   692,   364,   -692,  -1012, -68,   260,   -480, 876,
	-1116, 452,   -332,  -352,  892,   -1088, 1220,  -676,  12,    -292, 244,
	496,   372,   -32,   280,   200,   112,   -440,  -96,   24,    -644, -184,
	56,    -432,  224,   -980,  272,   -260,  144,   -436,  420,   356,  364,
	-528,  76,    172,   -744,  -368,  404,   -752,  -416,  684,   -688, 72,
	540,   416,   92,    444,   480,   -72,   -1416, 164,   -1172, -68,  24,
	424,   264,   1040,  128,   -912,  -524,  -356,  64,    876,   -12,  4,
	-88,   532,   272,   -524,  320,   276,   -508,  940,   24,    -400, -120,
	756,   60,    236,   -412,  100,   376,   -484,  400,   -100,  -740, -108,
	-260,  328,   -268,  224,   -200,  -416,  184,   -604,  -564,  -20,  296,
	60,    892,   -888,  60,    164,   68,    -760,  216,   -296,  904,  -336,
	-28,   404,   -356,  -568,  -208,  -1480, -512,  296,   328,   -360, -164,
	-1560, -776,  1156,  -428,  164,   -504,  -112,  120,   -216,  -148, -264,
	308,   32,    64,    -72,   72,    116,   176,   -64,   -272,  460,  -536,
	-784,  -280,  348,   108,   -752,  -132,  524,   -540,  -776,  116,  -296,
	-1196, -288,  -560,  1040,  -472,  116,   -848,  -1116, 116,   636,  696,
	284,   -176,  1016,  204,   -864,  -648,  -248,  356,   972,   -584, -204,
	264,   880,   528,   -24,   -184,  116,   448,   -144,  828,   524,  212,
	-212,  52,    12,    200,   268,   -488,  -404,  -880,  824,   -672, -40,
	908,   -248,  500,   716,   -576,  492,   -576,  16,    720,   -108, 384,
	124,   344,   280,   576,   -500,  252,   104,   -308,  196,   -188, -8,
	1268,  296,   1032,  -1196, 436,   316,   372,   -432,  -200,  -660, 704,
	-224,  596,   -132,  268,   32,    -452,  884,   104,   -1008, 424,  -1348,
	-280,  4,     -1168, 368,   476,   696,   300,   -8,    24,    180,  -592,
	-196,  388,   304,   500,   724,   -160,  244,   -84,   272,   -256, -420,
	320,   208,   -144,  -156,  156,   364,   452,   28,    540,   316,  220,
	-644,  -248,  464,   72,    360,   32,    -388,  496,   -680,  -48,  208,
	-116,  -408,  60,    -604,  -392,  548,   -840,  784,   -460,  656,  -544,
	-388,  -264,  908,   -800,  -628,  -612,  -568,  572,   -220,  164,  288,
	-16,   -308,  308,   -112,  -636,  -760,  280,   -668,  432,   364,  240,
	-196,  604,   340,   384,   196,   592,   -44,   -500,  432,   -580, -132,
	636,   -76,   392,   4,     -412,  540,   508,   328,   -356,  -36,  16,
	-220,  -64,   -248,  -60,   24,    -192,  368,   1040,  92,    -24,  -1044,
	-32,   40,    104,   148,   192,   -136,  -520,  56,    -816,  -224, 732,
	392,   356,   212,   -80,   -424,  -1008, -324,  588,   -1496, 576,  460,
	-816,  -848,  56,    -580,  -92,   -1372, -112,  -496,  200,   364,  52,
	-140,  48,    -48,   -60,   84,    72,    40,    132,   -356,  -268, -104,
	-284,  -404,  732,   -520,  164,   -304,  -540,  120,   328,   -76,  -460,
	756,   388,   588,   236,   -436,  -72,   -176,  -404,  -316,  -148, 716,
	-604,  404,   -72,   -88,   -888,  -68,   944,   88,    -220,  -344, 960,
	472,   460,   -232,  704,   120,   832,   -228,  692,   -508,  132,  -476,
	844,   -748,  -364,  -44,   1116,  -1104, -1056, 76,    428,   552,  -692,
	60,    356,   96,    -384,  -188,  -612,  -576,  736,   508,   892,  352,
	-1132, 504,   -24,   -352,  324,   332,   -600,  -312,  292,   508,  -144,
	-8,    484,   48,    284,   -260,  -240,  256,   -100,  -292,  -204, -44,
	472,   -204,  908,   -188,  -1000, -256,  92,    1164,  -392,  564,  356,
	652,   -28,   -884,  256,   484,   -192,  760,   -176,  376,   -524, -452,
	-436,  860,   -736,  212,   124,   504,   -476,  468,   76,    -472, 552,
	-692,  -944,  -620,  740,   -240,  400,   132,   20,    192,   -196, 264,
	-668,  -1012, -60,   296,   -316,  -828,  76,    -156,  284,   -768, -448,
	-832,  148,   248,   652,   616,   1236,  288,   -328,  -400,  -124, 588,
	220,   520,   -696,  1032,  768,   -740,  -92,   -272,  296,   448,  -464,
	412,   -200,  392,   440,   -200,  264,   -152,  -260,  320,   1032, 216,
	320,   -8,    -64,   156,   -1016, 1084,  1172,  536,   484,   -432, 132,
	372,   -52,   -256,  84,    116,   -352,  48,    116,   304,   -384, 412,
	924,   -300,  528,   628,   180,   648,   44,    -980,  -220,  1320, 48,
	332,   748,   524,   -268,  -720,  540,   -276,  564,   -344,  -208, -196,
	436,   896,   88,    -392,  132,   80,    -964,  -288,  568,   56,   -48,
	-456,  888,   8,     552,   -156,  -292,  948,   288,   128,   -716, -292,
	1192,  -152,  876,   352,   -600,  -260,  -812,  -468,  -28,   -120, -32,
	-44,   1284,  496,   192,   464,   312,   -76,   -516,  -380,  -456, -1012,
	-48,   308,   -156,  36,    492,   -156,  -808,  188,   1652,  68,   -120,
	-116,  316,   160,   -140,  352,   808,   -416,  592,   316,   -480, 56,
	528,   -204,  -568,  372,   -232,  752,   -344,  744,   -4,    324,  -416,
	-600,  768,   268,   -248,  -88,   -132,  -420,  -432,  80,    -288, 404,
	-316,  -1216, -588,  520,   -108,  92,    -320,  368,   -480,  -216, -92,
	1688,  -300,  180,   1020,  -176,  820,   -68,   -228,  -260,  436,  -904,
	20,    40,    -508,  440,   -736,  312,   332,   204,   760,   -372, 728,
	96,    -20,   -632,  -520,  -560,  336,   1076,  -64,   -532,  776,  584,
	192,   396,   -728,  -520,  276,   -188,  80,    -52,   -612,  -252, -48,
	648,   212,   -688,  228,   -52,   -260,  428,   -412,  -272,  -404, 180,
	816,   -796,  48,    152,   484,   -88,   -216,  988,   696,   188,  -528,
	648,   -116,  -180,  316,   476,   12,    -564,  96,    476,   -252, -364,
	-376,  -392,  556,   -256,  -576,  260,   -352,  120,   -16,   -136, -260,
	-492,  72,    556,   660,   580,   616,   772,   436,   424,   -32,  -324,
	-1268, 416,   -324,  -80,   920,   160,   228,   724,   32,    -516, 64,
	384,   68,    -128,  136,   240,   248,   -204,  -68,   252,   -932, -120,
	-480,  -628,  -84,   192,   852,   -404,  -288,  -132,  204,   100,  168,
	-68,   -196,  -868,  460,   1080,  380,   -80,   244,   0,     484,  -888,
	64,    184,   352,   600,   460,   164,   604,   -196,  320,   -64,  588,
	-184,  228,   12,    372,   48,    -848,  -344,  224,   208,   -200, 484,
	128,   -20,   272,   -468,  -840,  384,   256,   -720,  -520,  -464, -580,
	112,   -120,  644,   -356,  -208,  -608,  -528,  704,   560,   -424, 392,
	828,   40,    84,    200,   -152,  0,     -144,  584,   280,   -120, 80,
	-556,  -972,  -196,  -472,  724,   80,    168,   -32,   88,    160,  -688,
	0,     160,   356,   372,   -776,  740,   -128,  676,   -248,  -480, 4,
	-364,  96,    544,   232,   -1032, 956,   236,   356,   20,    -40,  300,
	24,    -676,  -596,  132,   1120,  -104,  532,   -1096, 568,   648,  444,
	508,   380,   188,   -376,  -604,  1488,  424,   24,    756,   -220, -192,
	716,   120,   920,   688,   168,   44,    -460,  568,   284,   1144, 1160,
	600,   424,   888,   656,   -356,  -320,  220,   316,   -176,  -724, -188,
	-816,  -628,  -348,  -228,  -380,  1012,  -452,  -660,  736,   928,  404,
	-696,  -72,   -268,  -892,  128,   184,   -344,  -780,  360,   336,  400,
	344,   428,   548,   -112,  136,   -228,  -216,  -820,  -516,  340,  92,
	-136,  116,   -300,  376,   -244,  100,   -316,  -520,  -284,  -12,  824,
	164,   -548,  -180,  -128,  116,   -924,  -828,  268,   -368,  -580, 620,
	192,   160,   0,     -1676, 1068,  424,   -56,   -360,  468,   -156, 720,
	288,   -528,  556,   -364,  548,   -148,  504,   316,   152,   -648, -620,
	-684,  -24,   -376,  -384,  -108,  -920,  -1032, 768,   180,   -264, -508,
	-1268, -260,  -60,   300,   -240,  988,   724,   -376,  -576,  -212, -736,
	556,   192,   1092,  -620,  -880,  376,   -56,   -4,    -216,  -32,  836,
	268,   396,   1332,  864,   -600,  100,   56,    -412,  -92,   356,  180,
	884,   -468,  -436,  292,   -388,  -804,  -704,  -840,  368,   -348, 140,
	-724,  1536,  940,   372,   112,   -372,  436,   -480,  1136,  296,  -32,
	-228,  132,   -48,   -220,  868,   -1016, -60,   -1044, -464,  328,  916,
	244,   12,    -736,  -296,  360,   468,   -376,  -108,  -92,   788,  368,
	-56,   544,   400,   -672,  -420,  728,   16,    320,   44,    -284, -380,
	-796,  488,   132,   204,   -596,  -372,  88,    -152,  -908,  -636, -572,
	-624,  -116,  -692,  -200,  -56,   276,   -88,   484,   -324,  948,  864,
	1000,  -456,  -184,  -276,  292,   -296,  156,   676,   320,   160,  908,
	-84,   -1236, -288,  -116,  260,   -372,  -644,  732,   -756,  -96,  84,
	344,   -520,  348,   -688,  240,   -84,   216,   -1044, -136,  -676, -396,
	-1500, 960,   -40,   176,   168,   1516,  420,   -504,  -344,  -364, -360,
	1216,  -940,  -380,  -212,  252,   -660,  -708,  484,   -444,  -152, 928,
	-120,  1112,  476,   -260,  560,   -148,  -344,  108,   -196,  228,  -288,
	504,   560,   -328,  -88,   288,   -1008, 460,   -228,  468,   -836, -196,
	76,    388,   232,   412,   -1168, -716,  -644,  756,   -172,  -356, -504,
	116,   432,   528,   48,    476,   -168,  -608,  448,   160,   -532, -272,
	28,    -676,  -12,   828,   980,   456,   520,   104,   -104,  256,  -344,
	-4,    -28,   -368,  -52,   -524,  -572,  -556,  -200,  768,   1124, -208,
	-512,  176,   232,   248,   -148,  -888,  604,   -600,  -304,  804,  -156,
	-212,  488,   -192,  -804,  -256,  368,   -360,  -916,  -328,  228,  -240,
	-448,  -472,  856,   -556,  -364,  572,   -12,   -156,  -368,  -340, 432,
	252,   -752,  -152,  288,   268,   -580,  -848,  -592,  108,   -76,  244,
	312,   -716,  592,   -80,   436,   360,   4,     -248,  160,   516,  584,
	732,   44,    -468,  -280,  -292,  -156,  -588,  28,    308,   912,  24,
	124,   156,   180,   -252,  944,   -924,  -772,  -520,  -428,  -624, 300,
	-212,  -1144, 32,    -724,  800,   -1128, -212,  -1288, -848,  180,  -416,
	440,   192,   -576,  -792,  -76,   -1080, 80,    -532,  -352,  -132, 380,
	-820,  148,   1112,  128,   164,   456,   700,   -924,  144,   -668, -384,
	648,   -832,  508,   552,   -52,   -100,  -656,  208,   -568,  748,  -88,
	680,   232,   300,   192,   -408,  -1012, -152,  -252,  -268,  272,  -876,
	-664,  -648,  -332,  -136,  16,    12,    1152,  -28,   332,   -536, 320,
	-672,  -460,  -316,  532,   -260,  228,   -40,   1052,  -816,  180,  88,
	-496,  -556,  -672,  -368,  428,   92,    356,   404,   -408,  252,  196,
	-176,  -556,  792,   268,   32,    372,   40,    96,    -332,  328,  120,
	372,   -900,  -40,   472,   -264,  -592,  952,   128,   656,   112,  664,
	-232,  420,   4,     -344,  -464,  556,   244,   -416,  -32,   252,  0,
	-412,  188,   -696,  508,   -476,  324,   -1096, 656,   -312,  560,  264,
	-136,  304,   160,   -64,   -580,  248,   336,   -720,  560,   -348, -288,
	-276,  -196,  -500,  852,   -544,  -236,  -1128, -992,  -776,  116,  56,
	52,    860,   884,   212,   -12,   168,   1020,  512,   -552,  924,  -148,
	716,   188,   164,   -340,  -520,  -184,  880,   -152,  -680,  -208, -1156,
	-300,  -528,  -472,  364,   100,   -744,  -1056, -32,   540,   280,  144,
	-676,  -32,   -232,  -280,  -224,  96,    568,   -76,   172,   148,  148,
	104,   32,    -296,  -32,   788,   -80,   32,    -16,   280,   288,  944,
	428,   -484
};

#define FG_CHECk_VALUE(para, over, handle)		\
do {							\
	if ((para) > (over)) {				\
		FG_ERR("%s %d overflow %d!!!\n", #para, (para), (over));		\
		(para) = (handle);			\
	}						\
} while (0)

/* previous and current fg params
 * if update_grain == 0, use previous params
 */
struct disp_fg_info fg_info[MAX_FG];

int get_random_number(int bits)
{
	MS_U16 bit;

	bit = ((random_register >> 0) ^ (random_register >> 1) ^
	(random_register >> 3) ^ (random_register >> 12)) & 1;

	random_register = (random_register >> 1) | (bit << 15);

	return (random_register >> (16 - bits)) & ((1 << bits) - 1);
}

static void init_random_generator(int luma_line, MS_U16 seed)
{
	// same for the picture
	int luma_num;
	MS_U16 msb = (seed >> 8) & 255;
	MS_U16 lsb = seed & 255;

	random_register = (msb << 8) + lsb;

	// changes for each row
	luma_num = luma_line >> 5;

	random_register ^= ((luma_num * 37 + 178) & 255) << 8;
	random_register ^= ((luma_num * 173 + 105) & 255);
}

static void init_scaling_function(MS_U8 scaling_points[][2], MS_U8 num_points, MS_U8 scaling_lut[])
{
	int i;
	int point;
	int x;
	int delta_y;
	int delta_x;
	MS_S64 delta;

	if (num_points == 0)
		return;

	i = 0;
	for (i = 0; i < scaling_points[0][0]; i++)
		scaling_lut[i] = scaling_points[0][1];

	point = 0;
	x = 0;
	for (point = 0; point < num_points - 1; point++) {
		delta_y = scaling_points[point + 1][1] - scaling_points[point][1];
		delta_x = scaling_points[point + 1][0] - scaling_points[point][0];
		if (delta_x != 0) {
			delta = delta_y * ((65536 + (delta_x >> 1)) / delta_x);
		} else {
			delta_x = 1;
			delta = delta_y * ((65536 + (delta_x >> 1)) / delta_x);
		}

		for (x = 0; x < delta_x; x++) {
			scaling_lut[scaling_points[point][0] + x] =
			scaling_points[point][1] + (int)((x * delta + 32768) >> 16);
		}
	}

	i = 0;
	for (i = scaling_points[num_points - 1][0]; i < 256; i++)
		scaling_lut[i] = scaling_points[num_points - 1][1];
}


static void disp_fg_check_vdec_md(struct mtk_av1_film_grain_params *fg_param)
{
	frame_num = frame_num + 1;

	FG_FUNC();

	FG_CHECk_VALUE(fg_param->num_y_points, 14, 0);

	FG_CHECk_VALUE(fg_param->num_cb_points, 10, 0);

	FG_CHECk_VALUE(fg_param->num_cr_points, 10, 0);

	FG_CHECk_VALUE(fg_param->grain_scaling, 11, 11);

	FG_CHECk_VALUE(fg_param->ar_coeff_lag, 3, 0);

	FG_CHECk_VALUE(fg_param->ar_coeff_shift, 9, 9);

	FG_CHECk_VALUE(fg_param->overlap_flag, 1, 1);

	FG_CHECk_VALUE(fg_param->clip_to_restricted_range, 1, 0);

	FG_CHECk_VALUE(fg_param->chroma_scaling_from_luma, 1, 0);
}

static void disp_fg_parser_lut(struct mtk_av1_film_grain_params *fg_param,
			       struct fg_hw_reg_output *params_hw_reg)
{
	MS_U8 num;

	FG_FUNC();

	params_hw_reg->num_y_points = fg_param->num_y_points;
	if (params_hw_reg->num_y_points) {
		for (num = 0; num < params_hw_reg->num_y_points; num++) {
			params_hw_reg->scaling_points_y[num][0] = fg_param->point_y_value[num];
			params_hw_reg->scaling_points_y[num][1] = fg_param->point_y_scaling[num];
		}
	}

	params_hw_reg->chroma_scaling_from_luma = fg_param->chroma_scaling_from_luma;

	params_hw_reg->num_cb_points = fg_param->num_cb_points;
	if (params_hw_reg->num_cb_points) {
		for (num = 0; num < params_hw_reg->num_cb_points; num++) {
			params_hw_reg->scaling_points_cb[num][0] = fg_param->point_cb_value[num];
			params_hw_reg->scaling_points_cb[num][1] = fg_param->point_cb_scaling[num];
		}
	}

	params_hw_reg->num_cr_points = fg_param->num_cr_points;
	if (params_hw_reg->num_cr_points) {
		for (num = 0; num < params_hw_reg->num_cr_points; num++) {
			params_hw_reg->scaling_points_cr[num][0] = fg_param->point_cb_value[num];
			params_hw_reg->scaling_points_cr[num][1] = fg_param->point_cr_scaling[num];
		}
	}

	params_hw_reg->grain_scaling = fg_param->grain_scaling - 8;

	params_hw_reg->cb_mult = fg_param->cb_mult;
	params_hw_reg->cb_luma_mult = fg_param->cb_luma_mult;
	params_hw_reg->cb_offset = fg_param->cb_offset;

	params_hw_reg->cr_mult = fg_param->cr_mult;
	params_hw_reg->cr_luma_mult = fg_param->cr_luma_mult;
	params_hw_reg->cr_offset = fg_param->cr_offset;

	params_hw_reg->clip_to_restricted_range = fg_param->clip_to_restricted_range;
}

/* parser grain noise */
static void disp_fg_parser_gns(struct mtk_av1_film_grain_params *params,
			       struct fg_hw_reg_output *params_hw_reg)
{
	MS_U8 num = 0;
	MS_U8 NumPosLuma = 0;
	MS_U8 NumPosLuma_PlusOne = 0;
	MS_U8 NumPosChroma = 0;

	MS_U8 ar_coeffs_y[AV1_MAX_AR_COEFFS_CNT] = {0};
	MS_U8 ar_coeffs_cb[AV1_MAX_AR_COEFFS_CNT] = {0};
	MS_U8 ar_coeffs_cr[AV1_MAX_AR_COEFFS_CNT] = {0};
	MS_U8 ar_coeff_lag = 0;

	FG_FUNC();

	params_hw_reg->grain_seed = params->grain_seed;
	params_hw_reg->grain_scale_shift = params->grain_scale_shift;

	ar_coeff_lag = params->ar_coeff_lag;
	NumPosLuma = FG_GRAIN_MARGIN_NS * ar_coeff_lag * (ar_coeff_lag + 1);

	if (params_hw_reg->num_y_points) {
		NumPosLuma_PlusOne = 1;
		for (num = 0; num < NumPosLuma; num++)
			ar_coeffs_y[num] =
				(MS_U8)((params->ar_coeffs_y[num]) &
					FG_AR_COEFF_MASK);
	}

	NumPosChroma = NumPosLuma_PlusOne + NumPosLuma;
	if (params_hw_reg->chroma_scaling_from_luma ||
	    params_hw_reg->num_cb_points) {
		for (num = 0; num < NumPosChroma; num++)
			ar_coeffs_cb[num] =
				(MS_U8)((params->ar_coeffs_cb[num]) &
					FG_AR_COEFF_MASK);
	}

	if (params_hw_reg->chroma_scaling_from_luma ||
	    params_hw_reg->num_cr_points) {
		for (num = 0; num < NumPosChroma; num++)
			ar_coeffs_cr[num] =
				(MS_U8)((params->ar_coeffs_cr[num]) &
					FG_AR_COEFF_MASK);
	}

	/* ar_coeff_shift minus 6 */
	params_hw_reg->ar_coeff_shift = params->ar_coeff_shift -
					FG_AR_COEFF_SHIT_MINUS;
	params_hw_reg->overlap_flag = params->overlap_flag;

	memset(params_hw_reg->ar_coeffs_y, 0,
	       sizeof(params_hw_reg->ar_coeffs_y));
	memset(params_hw_reg->ar_coeffs_cb, 0,
	       sizeof(params_hw_reg->ar_coeffs_cb));
	memset(params_hw_reg->ar_coeffs_cr, 0,
	       sizeof(params_hw_reg->ar_coeffs_cr));

	/* copy ar_coeffs_y[] */
	if (params_hw_reg->num_y_points) {
		NumPosLuma_PlusOne = 1;

		if (ar_coeff_lag == 3) {
			memcpy(params_hw_reg->ar_coeffs_y, ar_coeffs_y,
			       sizeof(params_hw_reg->ar_coeffs_y));
		} else if (ar_coeff_lag == 2) {
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_8] =
				ar_coeffs_y[FG_AR_COEFF_IDX_0];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_9] =
				ar_coeffs_y[FG_AR_COEFF_IDX_1];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_10] =
				ar_coeffs_y[FG_AR_COEFF_IDX_2];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_11] =
				ar_coeffs_y[FG_AR_COEFF_IDX_3];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_12] =
				ar_coeffs_y[FG_AR_COEFF_IDX_4];

			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_15] =
				ar_coeffs_y[FG_AR_COEFF_IDX_5];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_16] =
				ar_coeffs_y[FG_AR_COEFF_IDX_6];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_17] =
				ar_coeffs_y[FG_AR_COEFF_IDX_7];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_18] =
				ar_coeffs_y[FG_AR_COEFF_IDX_8];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_19] =
				ar_coeffs_y[FG_AR_COEFF_IDX_9];

			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_22] =
				ar_coeffs_y[FG_AR_COEFF_IDX_10];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_23] =
				ar_coeffs_y[FG_AR_COEFF_IDX_11];
		} else if (ar_coeff_lag == 1) {
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_16] =
				ar_coeffs_y[FG_AR_COEFF_IDX_0];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_17] =
				ar_coeffs_y[FG_AR_COEFF_IDX_1];
			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_18] =
				ar_coeffs_y[FG_AR_COEFF_IDX_2];

			params_hw_reg->ar_coeffs_y[FG_AR_COEFF_IDX_23] =
				ar_coeffs_y[FG_AR_COEFF_IDX_3];
		}
	} else {
		NumPosLuma_PlusOne = 0;
	}

	NumPosChroma = NumPosLuma_PlusOne + NumPosLuma;
	/* copy ar_coeffs_cb[] */
	if (params_hw_reg->chroma_scaling_from_luma ||
	    params_hw_reg->num_cb_points) {
		if (ar_coeff_lag == 3) {
			memcpy(params_hw_reg->ar_coeffs_cb, ar_coeffs_cb,
			       sizeof(params_hw_reg->ar_coeffs_cb));
		} else if (ar_coeff_lag == 2) {
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_8] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_0];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_9] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_1];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_10] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_2];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_11] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_3];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_12] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_4];

			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_15] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_5];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_16] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_6];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_17] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_7];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_18] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_8];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_19] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_9];

			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_22] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_10];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_23] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_11];
			if (NumPosLuma_PlusOne == 1)
				params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_24]
					= ar_coeffs_cb[FG_AR_COEFF_IDX_12];
		} else if (ar_coeff_lag == 1) {
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_16] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_0];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_17] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_1];
			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_18] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_2];

			params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_23] =
				ar_coeffs_cb[FG_AR_COEFF_IDX_3];
			if (NumPosLuma_PlusOne == 1)
				params_hw_reg->ar_coeffs_cb[FG_AR_COEFF_IDX_24]
					= ar_coeffs_cb[FG_AR_COEFF_IDX_4];
		}
	}

	/* copy ar_coeffs_cr[] */
	if (params_hw_reg->chroma_scaling_from_luma ||
	    params_hw_reg->num_cr_points) {
		if (ar_coeff_lag == 3) {
			memcpy(params_hw_reg->ar_coeffs_cr, ar_coeffs_cr,
			       sizeof(params_hw_reg->ar_coeffs_cr));
		} else if (ar_coeff_lag == 2) {
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_8] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_0];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_9] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_1];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_10] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_2];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_11] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_3];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_12] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_4];

			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_15] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_5];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_16] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_6];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_17] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_7];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_18] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_8];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_19] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_9];

			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_22] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_10];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_23] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_11];
			if (NumPosLuma_PlusOne == 1)
				params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_24]
					= ar_coeffs_cr[FG_AR_COEFF_IDX_12];
		} else if (ar_coeff_lag == 1) {
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_16] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_0];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_17] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_1];
			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_18] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_2];

			params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_23] =
				ar_coeffs_cr[FG_AR_COEFF_IDX_3];
			if (NumPosLuma_PlusOne == 1)
				params_hw_reg->ar_coeffs_cr[FG_AR_COEFF_IDX_24]
					= ar_coeffs_cr[FG_AR_COEFF_IDX_4];
		}
	}
}

static void disp_fg_process_lut(struct fg_hw_reg_output *params_hw_reg,
				struct fg_hw_adl_output *params_hw_adl)
{
	memset(params_hw_adl->u8scaling_lut_y256, 0, sizeof(params_hw_adl->u8scaling_lut_y256));
	memset(params_hw_adl->u8scaling_lut_cb256, 0, sizeof(params_hw_adl->u8scaling_lut_cb256));
	memset(params_hw_adl->u8scaling_lut_cr256, 0, sizeof(params_hw_adl->u8scaling_lut_cr256));

	FG_FUNC();

	init_scaling_function(params_hw_reg->scaling_points_y,
			      params_hw_reg->num_y_points,
			      params_hw_adl->u8scaling_lut_y256);

	if (params_hw_reg->chroma_scaling_from_luma) {
		memcpy(params_hw_adl->u8scaling_lut_cb256,
		       params_hw_adl->u8scaling_lut_y256,
		       sizeof(params_hw_adl->u8scaling_lut_y256));
		memcpy(params_hw_adl->u8scaling_lut_cr256,
		       params_hw_adl->u8scaling_lut_y256,
		       sizeof(params_hw_adl->u8scaling_lut_y256));
	} else {
		init_scaling_function(params_hw_reg->scaling_points_cb,
				      params_hw_reg->num_cb_points,
				      params_hw_adl->u8scaling_lut_cb256);
		init_scaling_function(params_hw_reg->scaling_points_cr,
				      params_hw_reg->num_cr_points,
				      params_hw_adl->u8scaling_lut_cr256);
	}
}

static void disp_fg_process_gns(struct fg_hw_reg_output *params_hw_reg,
				struct fg_hw_adl_output *params_hw_adl)
{
	MS_U8 luma_block_size_y = FG_LUMA_BLOCK_SIZE_Y;
	MS_U8 luma_block_size_x = FG_LUMA_BLOCK_SIZE_X;
	MS_U8 chroma_block_size_y = FG_CHROMA_BLOCK_SIZE_Y;
	MS_U8 chroma_block_size_x = FG_CHROMA_BLOCK_SIZE_X;
	MS_U8 luma_grain_stride = 0;
	MS_U8 chroma_grain_stride = 0;


	int gauss_sec_shift;
	int i;
	int j;

	FG_FUNC();

	random_register = params_hw_reg->grain_seed;

	luma_grain_stride = luma_block_size_x; /* 82 */
	chroma_grain_stride = chroma_block_size_x; /* 44 */

	gauss_sec_shift = FG_MAX_BITS - params_hw_reg->bit_depth +
		params_hw_reg->grain_scale_shift;

	FG_LOG_D("random_register %u\n", random_register);
	FG_LOG_D("bit_depth %u grain_scale_shift %u gauss_sec_shift %d\n",
		params_hw_reg->bit_depth,
		params_hw_reg->grain_scale_shift,
		gauss_sec_shift);

	if (params_hw_reg->num_y_points) {
		/* 73 x 82 = 5986 */
		for (i = 0; i < luma_block_size_y; i++) {	/* 73 */
			for (j = 0; j < luma_block_size_x; j++)	/* 82 */
				params_hw_adl->y_grain_block[i *
				luma_grain_stride + j] =
				(((gaussian_sequence[get_random_number
				(gauss_bits)] +
				((1 << gauss_sec_shift) >> 1)) >>
				gauss_sec_shift));
		}
	} else {
		memset(params_hw_adl->y_grain_block, 0,
			sizeof(params_hw_adl->y_grain_block));
	}

	if (params_hw_reg->chroma_scaling_from_luma ||
	    params_hw_reg->num_cb_points) {
		init_random_generator(FG_CB_LINE, params_hw_reg->grain_seed);
		/* 38 x 44 = 1762 */
		for (i = 0; i < chroma_block_size_y; i++) {
			for (j = 0; j < chroma_block_size_x; j++)
				params_hw_adl->cb_grain_block[i *
				chroma_grain_stride + j] =
				(((gaussian_sequence[get_random_number(
				gauss_bits)] +
				((1 << gauss_sec_shift) >> 1)) >>
				gauss_sec_shift));
		}
	} else {
		memset(params_hw_adl->cb_grain_block, 0,
			sizeof(params_hw_adl->cb_grain_block));
	}


	if (params_hw_reg->chroma_scaling_from_luma ||
	    params_hw_reg->num_cr_points) {
		init_random_generator(FG_CR_LINE, params_hw_reg->grain_seed);
		for (i = 0; i < chroma_block_size_y; i++) {
			for (j = 0; j < chroma_block_size_x; j++)
				params_hw_adl->cr_grain_block[i *
				chroma_grain_stride + j] =
				(((gaussian_sequence[get_random_number(
				gauss_bits)] +
				((1 << gauss_sec_shift) >> 1)) >>
				gauss_sec_shift));
		}
	} else {
		memset(params_hw_adl->cr_grain_block, 0,
			sizeof(params_hw_adl->cr_grain_block));
	}
}

/* vdec metada data -> fg hw reg setting & adl raw data */
static void disp_fg_parser_meta(struct mtk_av1_film_grain_params *fg_param,
				struct fg_hw_reg_output *params_hw_reg,
				struct fg_hw_adl_output *params_hw_adl)
{
	FG_FUNC();

	params_hw_reg->bit_depth = FG_DEFAULT_BIT;

	disp_fg_check_vdec_md(fg_param);

	/* vdec meta data -> fg hw reg setting */
	disp_fg_parser_lut(fg_param, params_hw_reg);
	disp_fg_parser_gns(fg_param, params_hw_reg);

	/* generate adl raw data */
	disp_fg_process_lut(params_hw_reg, params_hw_adl);
	disp_fg_process_gns(params_hw_reg, params_hw_adl);
}

static uint64_t disp_fg_gettimeofday(void)
{
	struct timespec64 t;

	ktime_get_ts64(&t);

	return (t.tv_sec & FG_SEC_MASK) * FG_ONE_SEC_PER_NS + t.tv_nsec;
}

static void disp_fg_sw_auto_reg(struct mtk_av1_film_grain_params *fg_param,
				struct disp_fg_info *info)
{
	uint64_t start_timer, end_timer;

	FG_FUNC();

	start_timer = disp_fg_gettimeofday();

	info->hw_reg->force_write = 1;
	disp_fg_init_gns_grain_info(fg_param,
				    info->hw_reg,
				    &info->gns_info);
	disp_fg_pre_process_gns(fg_param, &info->gns_info, info->hw_adl,
				&info->gns_ar_info);
	disp_fg_pre_process_ar_coeffs(info->hw_reg);

	end_timer = disp_fg_gettimeofday();

	FG_SW_PER("sw filter %llu ns start %llu %llu\n",
		  (end_timer - start_timer),
		  start_timer,
		  end_timer);
}

static void disp_fg_hw_auto_reg(struct mtk_av1_film_grain_params *fg_param,
				struct disp_fg_info *info)
{
	uint32_t i = 0;
	MS_S32 *y_grain_block = info->hw_adl->y_grain_block;
	MS_S32 *cb_grain_block = info->hw_adl->cb_grain_block;
	MS_S32 *cr_grain_block = info->hw_adl->cr_grain_block;

	FG_FUNC();

	info->hw_reg->force_write = 0;

	for (i = 0; i < FG_LUAM_BLOCK_SIZE; i++)
		y_grain_block[i] = REVISE_VAL(y_grain_block[i],
					      FG_GRAIN_VALUE_MAX);

	for (i = 0; i < FG_CHROMA_BLOCK_SIZE; i++) {
		cb_grain_block[i] = REVISE_VAL(cb_grain_block[i],
					       FG_GRAIN_VALUE_MAX);
		cr_grain_block[i] = REVISE_VAL(cr_grain_block[i],
					       FG_GRAIN_VALUE_MAX);
	}
}

MS_BOOL disp_fg_handler(u32 fg_hw_id, struct mtk_av1_film_grain_params *fg_param,
			struct adl_src_tbl *adl_tbl)
{
	MS_U8 apply_grain;
	MS_U8 update_grain;
	struct disp_fg_info *info;

	if (!fg_param || fg_force_bypass || !fg_param->apply_grain)
		goto bypass_fg;

	FG_FUNC();

	info = &fg_info[fg_hw_id];

	apply_grain = fg_param->apply_grain;
	update_grain = fg_param->update_grain;

	info->fg_params_idx = (info->fg_params_idx + 1) % FG_MAX_PARAM_NS;
	info->hw_reg = &info->fg_params_hw_reg[info->fg_params_idx];
	info->hw_adl = &info->fg_params_hw_adl[info->fg_params_idx];
	disp_fg_parser_meta(fg_param, info->hw_reg, info->hw_adl);

	if (fg_sw_auto_reg_filter && info->gns_ar_info.ar_block_alloc)
		disp_fg_sw_auto_reg(fg_param, info);
	else
		disp_fg_hw_auto_reg(fg_param, info);
	fg_hal_update_process(fg_hw_id, info->hw_reg, info->hw_adl, adl_tbl);

	return true;

bypass_fg:
	fg_hal_bypass(fg_hw_id, true);

	return false;
}

void disp_fg_force_bypass(u32 fg_hw_id, bool bypass)
{
	FG_FUNC();

	fg_force_bypass = bypass;
	fg_hal_bypass(fg_hw_id, bypass);
}

void disp_fg_sw_auto_reg_filter_enable(bool enable)
{
	FG_FUNC();

	fg_sw_auto_reg_filter = enable;
}

void disp_fg_free_gns_ar_scale_info(u32 fg_hw_id)
{
	struct disp_fg_info *info = NULL;

	FG_FUNC();

	if (fg_hw_id >= MAX_FG) {
		FG_ERR("invalid hw id %u\n", fg_hw_id);
		return;
	}

	info = &fg_info[fg_hw_id];

	disp_fg_free_gns_ar_block(&info->gns_ar_info);

	info->scale_info.src_w = 0;
	info->scale_info.src_h = 0;
	info->scale_info.dst_w = 0;
	info->scale_info.dst_h = 0;
}

void disp_fg_update_scale_info(u32 hw_id, struct video_scale_info *scale_info)
{
	struct disp_fg_info *info = NULL;

	FG_FUNC();

	if (hw_id >= MAX_FG || !scale_info) {
		FG_ERR("invalid hw id %u or scale_info is NULL\n", hw_id);
		return;
	}

	if (!fg_sw_auto_reg_filter)
		return;

	info = &fg_info[hw_id];

	if ((scale_info->src_w != info->scale_info.src_w) ||
	    (scale_info->src_h != info->scale_info.src_h) ||
	    (scale_info->dst_w != info->scale_info.dst_w) ||
	    (scale_info->dst_h != info->scale_info.dst_h)) {

		FG_LOG_D("scale_info [%ux%u -> %ux%u] [%ux%u -> %ux%u]\n",
			 info->scale_info.src_w,
			 info->scale_info.src_h,
			 info->scale_info.dst_w,
			 info->scale_info.dst_h,
			 scale_info->src_w,
			 scale_info->src_h,
			 scale_info->dst_w,
			 scale_info->dst_h);

		memcpy(&info->scale_info, scale_info,
		       sizeof(struct video_scale_info));
		disp_fg_init_gns_basic_info(&info->gns_info);
		disp_fg_update_gns_ar_block_info(&info->gns_info,
						 &info->scale_info,
						 &info->gns_ar_info);
	}
}
