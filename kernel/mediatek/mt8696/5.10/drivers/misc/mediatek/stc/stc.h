/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#define STC_SESSION_DEVICE	"mtk_stc"


/*******************************************************************************
 * Struct and Macro definition
 ******************************************************************************/
struct mtk_stc_info {
	int     stc_id;
	int64_t stc_value;
};

struct stc_instance {
	u64	offset;
	bool	alloc;
	bool	running; /* false: stop, true: running */
};

struct stc_param {
	dev_t			stc_devno;
	struct cdev		*stc_cdev;
	struct class	*stc_class;
	void __iomem	*gpt_base_addr;
	/*alloc for user id = instance_cnt = 0 1 2...MAX_STC-1*/
	unsigned int	instance_cnt;
	unsigned int	enable_flag;
};

enum DEBUG_ENABLE {
	ENABLE_STC_TEST = 0,
	ENABLE_STC_DEBUG,
	ENABLE_STC_TEST_DEBUG
};

#define enable_debug(flag)	((flag == ENABLE_STC_DEBUG) || \
		(flag == ENABLE_STC_TEST_DEBUG))
#define enable_test(flag)	((flag == ENABLE_STC_TEST) || \
		(flag == ENABLE_STC_TEST_DEBUG))

#define MAX_STC				(2)
#define STC_OK				(0)
#define STC_FAIL				(-1)

#define GPT_MODE_BIT			(0x60)
#define GPT_CLKSRC_BIT			(0xC)
#define GPT_EN_BIT			(0x1)

#define GPT6_ADDR				(0xA0)
#define GPT6_CNT_L			(GPT6_ADDR + 0x8)
#define GPT6_CNT_H			(GPT6_ADDR + 0x10)

#define MTK_STC_IOW(num, dtype)     _IOW('B', num, dtype)
#define MTK_STC_IOR(num, dtype)     _IOR('B', num, dtype)
#define MTK_STC_IOWR(num, dtype)    _IOWR('B', num, dtype)
#define MTK_STC_IO(num)             _IO('B', num)

#define MTK_STC_IOCTL_ALLOC         MTK_STC_IO(0x0)
#define MTK_STC_IOCTL_FREE          MTK_STC_IO(0x1)
#define MTK_STC_IOCTL_START         MTK_STC_IOW(0x2, int)
#define MTK_STC_IOCTL_STOP          MTK_STC_IOW(0x3, int)
#define MTK_STC_IOCTL_SET           MTK_STC_IOW(0x4, struct mtk_stc_info)
#define MTK_STC_IOCTL_GET           MTK_STC_IOR(0x5, struct mtk_stc_info)
#define MTK_STC_IOCTL_ADJUST        MTK_STC_IOW(0x6, struct mtk_stc_info)

