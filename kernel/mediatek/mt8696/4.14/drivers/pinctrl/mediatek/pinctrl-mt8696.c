// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 * Author: Zhiyong Tao <zhiyong.tao@mediatek.com>
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/regmap.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <dt-bindings/pinctrl/mt65xx.h>

#include "pinctrl-mtk-common.h"
#include "pinctrl-mtk-mt8696.h"

#define MAX_GPIO_MODE_PER_REG 8
#define GPIO_MODE_BITS        4

static const struct mtk_pin_info mt8696_pin_info_eh[] = {
	MTK_PIN_INFO(71, 0x050, 9, 3, 4),
	MTK_PIN_INFO(72, 0x050, 3, 3, 4),
	MTK_PIN_INFO(73, 0x050, 0, 3, 4),
	MTK_PIN_INFO(74, 0x050, 6, 3, 4),

	MTK_PIN_INFO(104, 0x040, 0, 4, 2),
	MTK_PIN_INFO(105, 0x040, 12, 4, 2),
	MTK_PIN_INFO(106, 0x040, 16, 4, 2),
	MTK_PIN_INFO(107, 0x040, 4, 4, 2),
	MTK_PIN_INFO(108, 0x040, 20, 4, 2),
	MTK_PIN_INFO(109, 0x040, 8, 4, 2),

	MTK_PIN_INFO(114, 0x050, 0, 3, 3),
	MTK_PIN_INFO(115, 0x050, 3, 3, 3),
};

static const struct mtk_pin_info mt8696_pin_info_rsel[] = {
	MTK_PIN_INFO(71, 0x140, 6, 2, 4),
	MTK_PIN_INFO(72, 0x140, 2, 2, 4),
	MTK_PIN_INFO(73, 0x140, 0, 2, 4),
	MTK_PIN_INFO(74, 0x140, 4, 2, 4),

	MTK_PIN_INFO(104, 0x0e0, 0, 2, 2),
	MTK_PIN_INFO(105, 0x0e0, 6, 2, 2),
	MTK_PIN_INFO(106, 0x0e0, 8, 2, 2),
	MTK_PIN_INFO(107, 0x0e0, 2, 2, 2),
	MTK_PIN_INFO(108, 0x0e0, 10, 2, 2),
	MTK_PIN_INFO(109, 0x0e0, 4, 2, 2),

	MTK_PIN_INFO(114, 0x120, 0, 2, 3),
	MTK_PIN_INFO(115, 0x120, 2, 2, 3),
};

static const struct mtk_pin_info *mtk_find_i2c_drv_grp_by_pin(
		struct mtk_pinctrl *pctl,  unsigned long pin)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt8696_pin_info_eh); i++) {
		const struct mtk_pin_info *pin_drv =
				mt8696_pin_info_eh + i;
		if (pin == pin_drv->pin)
			return pin_drv;
	}

	return NULL;
}

/* i2c mode pin set pull r1r0 resistance in rsel register */
static void mtk_rsel_r1r0_set_samereg(const struct mtk_pin_info *rsel_infos,
		unsigned int info_num, unsigned int pin,
		unsigned int r1r0)
{
	switch (r1r0) {
	case MTK_RSEL_SET_R1R0_00:
		mtk_pinctrl_update_gpio_value(pctl_alt, pin, 0,
			info_num, rsel_infos);
		break;
	case MTK_RSEL_SET_R1R0_01:
		mtk_pinctrl_update_gpio_value(pctl_alt, pin, 1,
			info_num, rsel_infos);
		break;
	case MTK_RSEL_SET_R1R0_10:
		mtk_pinctrl_update_gpio_value(pctl_alt, pin, 2,
			info_num, rsel_infos);
		break;
	case MTK_RSEL_SET_R1R0_11:
		mtk_pinctrl_update_gpio_value(pctl_alt, pin, 3,
			info_num, rsel_infos);
		break;
	default:
		break;
	}
}

/* i2c mode pin set driving resistance in eh register */
static int mtk_i2c_drive_set_samereg(struct mtk_pinctrl *pctl,
		const struct mtk_pin_info *eh_infos,
		unsigned int info_num, unsigned int pin,
		unsigned int ehdrive, bool enable)
{
	const struct mtk_pin_info *pin_drv;
	unsigned char tmp_value = 0;

	if (!enable) {
		mtk_pinctrl_update_gpio_value(pctl, pin, 0, info_num, eh_infos);
		return 0;
	}

	pin_drv = mtk_find_i2c_drv_grp_by_pin(pctl, pin);

	if (pin_drv->width == 3 && ehdrive >= MTK_I2C_DRIVE_100) {
		pr_info("invalid i2c driving arg %d on pin %d, width = %d.\n",
			ehdrive, pin, pin_drv->width);
		return -EINVAL;
	}
	tmp_value = ehdrive - MTK_I2C_DRIVE_000;
	mtk_pinctrl_update_gpio_value(pctl, pin,
				tmp_value * 2 + 1, info_num, eh_infos);
	return 0;
}

static const struct mtk_drv_group_desc mt8696_drv_grp[] =  {
	/* 0E4E8SR 4/8/12/16 */
	MTK_DRV_GRP(4, 16, 0, 1, 4),
	/* 0E2E4SR  2/4/6/8 */
	MTK_DRV_GRP(2, 8, 0, 1, 2),
	/* E8E4E2  2/4/6/8/10/12/14/16 */
	MTK_DRV_GRP(2, 16, 0, 2, 2)
};

static const struct mtk_pin_multi_base_drv_grp mt8696_pin_drv[] = {
	MTK_PIN_MULTI_BASE_DRV_GRP(0, 0x0000, 0, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(1, 0x0000, 3, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(2, 0x0030, 6, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(3, 0x0000, 18, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(4, 0x0020, 21, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(5, 0x0010, 0, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(6, 0x0000, 22, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(7, 0x0000, 20, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(8, 0x0000, 30, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(9, 0x0010, 12, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(10, 0x0010, 14, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(11, 0x0010, 16, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(12, 0x0010, 18, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(13, 0x0010, 20, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(14, 0x0010, 22, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(15, 0x0010, 24, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(16, 0x0010, 26, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(17, 0x0010, 6, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(18, 0x0010, 8, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(19, 0x0000, 24, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(20, 0x0000, 26, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(21, 0x0000, 28, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(22, 0x0020, 21, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(23, 0x0020, 23, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(24, 0x0020, 25, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(25, 0x0020, 27, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(26, 0x0010, 9, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(27, 0x0010, 9, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(28, 0x0000, 0, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(29, 0x0000, 2, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(30, 0x0000, 4, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(31, 0x0010, 2, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(32, 0x0010, 28, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(33, 0x0010, 30, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(34, 0x0010, 4, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(35, 0x0010, 6, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(36, 0x0000, 27, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(37, 0x0010, 0, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(38, 0x0010, 3, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(39, 0x0000, 14, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(40, 0x0000, 6, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(41, 0x0000, 24, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(42, 0x0000, 21, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(43, 0x0000, 18, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(44, 0x0000, 15, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(45, 0x0000, 12, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(46, 0x0000, 9, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(47, 0x0010, 0, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(48, 0x0000, 0, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(49, 0x0000, 27, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(50, 0x0000, 24, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(51, 0x0000, 21, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(52, 0x0000, 18, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(53, 0x0000, 15, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(54, 0x0000, 12, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(55, 0x0000, 9, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(56, 0x0000, 6, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(57, 0x0000, 3, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(58, 0x0010, 3, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(59, 0x0020, 9, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(60, 0x0020, 12, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(61, 0x0020, 3, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(62, 0x0020, 6, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(63, 0x0020, 0, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(64, 0x0030, 9, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(65, 0x0030, 9, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(66, 0x0030, 9, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(67, 0x0030, 9, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(68, 0x0030, 12, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(69, 0x0030, 12, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(70, 0x0030, 12, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(71, 0x0030, 3, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(72, 0x0020, 27, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(73, 0x0020, 24, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(74, 0x0030, 0, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(75, 0x0020, 15, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(76, 0x0020, 17, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(77, 0x0020, 19, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(78, 0x0020, 21, 0, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(79, 0x0010, 10, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(80, 0x0010, 12, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(81, 0x0010, 15, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(82, 0x0010, 18, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(83, 0x0010, 21, 2, 1),
	MTK_PIN_MULTI_BASE_DRV_GRP(84, 0x0030, 6, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(85, 0x0030, 9, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(86, 0x0010, 6, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(87, 0x0010, 18, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(88, 0x0010, 21, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(89, 0x0010, 24, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(90, 0x0010, 27, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(91, 0x0020, 0, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(92, 0x0020, 3, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(93, 0x0020, 6, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(94, 0x0020, 9, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(95, 0x0020, 18, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(96, 0x0020, 15, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(97, 0x0020, 12, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(98, 0x0010, 15, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(99, 0x0010, 12, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(100, 0x0010, 9, 2, 4),
	MTK_PIN_MULTI_BASE_DRV_GRP(104, 0x0020, 0, 2, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(105, 0x0020, 0, 2, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(106, 0x0020, 9, 2, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(107, 0x0020, 3, 2, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(108, 0x0020, 12, 2, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(109, 0x0020, 6, 2, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(114, 0x0030, 15, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(115, 0x0030, 18, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(117, 0x0030, 21, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(118, 0x0030, 24, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(119, 0x0000, 0, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(120, 0x0000, 6, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(121, 0x0000, 9, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(122, 0x0000, 3, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(123, 0x0000, 12, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(124, 0x0000, 15, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(125, 0x0000, 16, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(126, 0x0000, 6, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(127, 0x0000, 18, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(128, 0x0000, 8, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(129, 0x0000, 10, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(130, 0x0000, 12, 0, 2),
	MTK_PIN_MULTI_BASE_DRV_GRP(131, 0x0030, 6, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(132, 0x0030, 6, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(133, 0x0010, 24, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(134, 0x0010, 21, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(135, 0x0010, 18, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(136, 0x0010, 15, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(137, 0x0010, 12, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(138, 0x0010, 27, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(139, 0x0010, 3, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(140, 0x0010, 0, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(141, 0x0000, 27, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(142, 0x0000, 24, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(143, 0x0010, 6, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(144, 0x0010, 9, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(145, 0x0000, 21, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(146, 0x0030, 3, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(147, 0x0030, 3, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(148, 0x0030, 3, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(149, 0x0030, 6, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(150, 0x0030, 3, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(151, 0x0020, 15, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(152, 0x0030, 0, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(153, 0x0020, 24, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(154, 0x0020, 27, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(155, 0x0020, 21, 2, 3),
	MTK_PIN_MULTI_BASE_DRV_GRP(156, 0x0020, 18, 2, 3),
};

static const struct mtk_pin_multi_base_drv_grp *mtk_find_drv_grp_by_pin(
		struct mtk_pinctrl *pctl,  unsigned long pin)
{
	int i;

	for (i = 0; i < pctl->devdata->n_pin_drv_grps; i++) {
		const struct mtk_pin_multi_base_drv_grp *pin_drv =
				pctl->devdata->pin_multi_base_drv_grp + i;
		if (pin == pin_drv->pin)
			return pin_drv;
	}

	return NULL;
}

static int mtk_pctl_set_gpio_drving(struct mtk_pinctrl *pctl,
		unsigned int pin, unsigned char driving)
{
	const struct mtk_pin_multi_base_drv_grp *pin_drv;
	unsigned int val;
	unsigned int bits, mask, shift;
	const struct mtk_drv_group_desc *drv_grp;
	struct regmap *regmap;



	if (pin >= pctl->devdata->npins)
		return -EINVAL;
	if ((driving >= MTK_I2C_DRIVE_000) &&
		(driving <= MTK_I2C_DRIVE_111))
		return mtk_i2c_drive_set_samereg(pctl, mt8696_pin_info_eh,
			ARRAY_SIZE(mt8696_pin_info_eh), pin, driving, true);
	/* if i2c pins use MTK_DRIVE_2mA/MTK_DRIVE_4mA/.../MTK_DRIVE_16mA,
	 * it defaults to eh = 0, using generic driving.
	 */
	if ((pin >= 71 && pin <= 74) || (pin >= 104 && pin <= 109) ||
		(pin >= 114 && pin <= 115))
		mtk_i2c_drive_set_samereg(pctl, mt8696_pin_info_eh,
			ARRAY_SIZE(mt8696_pin_info_eh), pin, 0, false);

	pin_drv = mtk_find_drv_grp_by_pin(pctl, pin);
	if (!pin_drv || pin_drv->grp > pctl->devdata->n_grp_cls)
		return -EINVAL;

	regmap = pctl->regmap[pin_drv->ip_num];
	drv_grp = pctl->devdata->grp_desc + pin_drv->grp;
	if (driving >= drv_grp->min_drv && driving <= drv_grp->max_drv
		&& !(driving % drv_grp->step)) {
		val = driving / drv_grp->step - 1;
		bits = drv_grp->high_bit - drv_grp->low_bit + 1;
		mask = BIT(bits) - 1;
		shift = pin_drv->bit + drv_grp->low_bit;
		mask <<= shift;
		val <<= shift;
		return regmap_update_bits(regmap,
				pin_drv->offset, mask, val);
	}

	return -EINVAL;
}

static int mtk_pctl_get_gpio_drving(struct mtk_pinctrl *pctl,
		unsigned int pin)
{
	const struct mtk_pin_multi_base_drv_grp *pin_drv;
	unsigned int val = 0;
	unsigned int bits, mask, shift;
	const struct mtk_drv_group_desc *drv_grp;
	struct regmap *regmap;

	pin_drv = mtk_find_drv_grp_by_pin(pctl, pin);
	if (!pin_drv || pin_drv->grp > pctl->devdata->n_grp_cls)
		return -1;

	drv_grp = pctl->devdata->grp_desc + pin_drv->grp;
	bits = drv_grp->high_bit - drv_grp->low_bit + 1;
	mask = BIT(bits) - 1;
	shift = pin_drv->bit + drv_grp->low_bit;
	mask <<= shift;
	regmap = pctl->regmap[pin_drv->ip_num];
	regmap_read(regmap, pin_drv->offset, &val);
	return ((val & mask) >> shift);
}

static int mtk_pinctrl_set_gpio_pupd_r1r0(struct mtk_pinctrl *pctl,
	unsigned int pin, bool enable, bool isup, unsigned int r1r0)
{
	unsigned int r0, r1, ret;

	if (r1r0 >= MTK_RSEL_SET_R1R0_00 && r1r0 <= MTK_RSEL_SET_R1R0_11)
		mtk_rsel_r1r0_set_samereg(mt8696_pin_info_rsel,
			ARRAY_SIZE(mt8696_pin_info_rsel), pin, r1r0);

	ret = mtk_pinctrl_set_gpio_value(pctl, pin, !isup,
		pctl->devdata->n_pin_pupd, pctl->devdata->pin_pupd_grps);
	if (ret == 0) {
		r0 = r1r0 & 0x1;
		r1 = (r1r0 & 0x2) >> 1;
		mtk_pinctrl_set_gpio_value(pctl, pin, r0,
			pctl->devdata->n_pin_r0, pctl->devdata->pin_r0_grps);
		mtk_pinctrl_set_gpio_value(pctl, pin, r1,
			pctl->devdata->n_pin_r1, pctl->devdata->pin_r1_grps);
		ret = 0;
	}
	return ret;
}

static int mtk_pinctrl_get_gpio_pupd_r1r0(struct mtk_pinctrl *pctl,
	unsigned int pin)
{
	int bit_pupd, bit_r0, bit_r1;

	bit_pupd = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_pupd, pctl->devdata->pin_pupd_grps);
	if (bit_pupd != -EPERM) {
		bit_r1 = mtk_pinctrl_get_gpio_value(pctl, pin,
			pctl->devdata->n_pin_r1, pctl->devdata->pin_r1_grps);
		bit_r0 = mtk_pinctrl_get_gpio_value(pctl, pin,
			pctl->devdata->n_pin_r0, pctl->devdata->pin_r0_grps);
		return (!bit_pupd)|(bit_r0<<1)|(bit_r1<<2)|(1<<3);
	}
	return -EPERM;
}

static int mtk_pinctrl_get_gpio_pu_pd(struct mtk_pinctrl *pctl,
	unsigned int pin)
{
	unsigned int bit_pu = 0, bit_pd = 0;

	bit_pu = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_pu, pctl->devdata->pin_pu_grps);
	bit_pd = mtk_pinctrl_get_gpio_value(pctl, pin,
		pctl->devdata->n_pin_pd, pctl->devdata->pin_pd_grps);
	if ((bit_pd != -EPERM) && (bit_pu != -EPERM))
		return (bit_pd)|(bit_pu<<1);
	else if ((bit_pd == -EPERM) && (bit_pu != -EPERM))
		return bit_pu<<1;
	else if ((bit_pd != -EPERM) && (bit_pu == -EPERM))
		return bit_pd;
	else
		return -EPERM;
}

static int mtk_pinctrl_set_gpio_pu_pd(struct mtk_pinctrl *pctl,
	unsigned int pin, bool enable, bool isup, unsigned int r1r0)
{
	mtk_pinctrl_set_gpio_value(pctl, pin, isup,
		pctl->devdata->n_pin_pu, pctl->devdata->pin_pu_grps);
	if (isup == 1)
		mtk_pinctrl_set_gpio_value(pctl, pin, !enable,
		pctl->devdata->n_pin_pd, pctl->devdata->pin_pd_grps);
	else
		mtk_pinctrl_set_gpio_value(pctl, pin, enable,
		pctl->devdata->n_pin_pd, pctl->devdata->pin_pd_grps);
	return 0;
}

static int mtk_pinctrl_get_gpio_pullen(struct mtk_pinctrl *pctl,
	unsigned int pin)
{
	unsigned int pull_en = 0;

	pull_en = mtk_pinctrl_get_gpio_pupd_r1r0(pctl, pin);
	if (pull_en == -EPERM) {
		pull_en = mtk_pinctrl_get_gpio_pu_pd(pctl, pin);
	/*pull_en = [pu,pd], 10,01 pull enabel, others pull disable*/
		if ((pull_en == 0x1) || (pull_en == 0x2))
			pull_en = GPIO_PULL_ENABLE;
		else if (pull_en == -EPERM)
			pull_en = GPIO_PULL_UNSUPPORTED;
		else
			pull_en = GPIO_PULL_DISABLE;
	} else {
	/*pull_en = [r1,r0,pupd], pull disabel 000,001, others enable*/
		if ((pull_en == 0x8) || (pull_en == 0x9))
			pull_en = GPIO_PULL_DISABLE;
		else
			pull_en = GPIO_PULL_ENABLE;
	}
	return pull_en;

}

static int mtk_pinctrl_get_gpio_pullsel(struct mtk_pinctrl *pctl,
	unsigned int pin)
{
	unsigned int pull_sel = 0;

	pull_sel = mtk_pinctrl_get_gpio_pupd_r1r0(pctl, pin);
	if (pull_sel == -EPERM) {
		pull_sel = mtk_pinctrl_get_gpio_pu_pd(pctl, pin);
		/*pull_sel = [pu,pd], 10 is pull up, 01 is pull down*/
		if (pull_sel == 0x02)
			pull_sel = GPIO_PULL_UP;
		else if (pull_sel == 0x01)
			pull_sel = GPIO_PULL_DOWN;
		else if (pull_sel == -EPERM)
			pull_sel = GPIO_PULL_UNSUPPORTED;
		else
			pull_sel = GPIO_NO_PULL;
	}
	return pull_sel;
}

static int mtk_pinctrl_set_gpio_pullsel(struct mtk_pinctrl *pctl,
		unsigned int pin, bool enable, bool isup, unsigned int arg)
{
	int ret = 0;
#ifndef GPIO_DEBUG
	ret = mtk_pinctrl_set_gpio_pupd_r1r0(pctl, pin, enable, isup, arg);
	if (ret != 0)
		mtk_pinctrl_set_gpio_pu_pd(pctl, pin, enable, isup, arg);
#else
	pr_debug("mtk_pinctrl_set_gpio_pull,pin=%d,enab=%d,sel=%d\n",
		pin, enable, isup);
	ret = mtk_pinctrl_set_gpio_pupd_r1r0(pctl, pin, enable, isup, arg);
	if (ret != 0)
		mtk_pinctrl_set_gpio_pu_pd(pctl, pin, enable, isup, arg);
	pr_debug("mtk_pinctrl_get_gpio_pull,pin=%d,enab=%d,sel=%d\n",
		pin, mtk_pinctrl_get_gpio_pullen(pctl, pin),
		mtk_pinctrl_get_gpio_pullsel(pctl, pin));
#endif
	return 0;
}

static void mt8696_spec_pinmux_set(struct regmap *reg, unsigned int pin,
			unsigned int mode)
{
	unsigned int reg_addr;
	unsigned char bit;
	unsigned int val;
	unsigned int mask = (1L << GPIO_MODE_BITS) - 1;

	reg_addr = ((pin / MAX_GPIO_MODE_PER_REG) << 4) + 0x0300;
	mode &= mask;
	bit = pin % MAX_GPIO_MODE_PER_REG;
	mask <<= (GPIO_MODE_BITS * bit);
	val = (mode << (GPIO_MODE_BITS * bit));

	regmap_update_bits(reg, reg_addr, mask, val);
}



static const struct mtk_pinctrl_devdata mt8696_pinctrl_data = {
	.pins = mtk_pins_mt8696,
	.npins = ARRAY_SIZE(mtk_pins_mt8696),
	.pin_mode_grps = mtk_pin_info_mode,
	.n_pin_mode = ARRAY_SIZE(mtk_pin_info_mode),
	.grp_desc = mt8696_drv_grp,
	.n_grp_cls = ARRAY_SIZE(mt8696_drv_grp),
	.pin_multi_base_drv_grp = mt8696_pin_drv,
	.n_pin_drv_grps = ARRAY_SIZE(mt8696_pin_drv),
	.pin_ies_grps = mtk_pin_info_ies,
	.n_pin_ies = ARRAY_SIZE(mtk_pin_info_ies),
	.pin_smt_grps = mtk_pin_info_smt,
	.n_pin_smt = ARRAY_SIZE(mtk_pin_info_smt),
	.pin_pu_grps = mtk_pin_info_pu,
	.n_pin_pu = ARRAY_SIZE(mtk_pin_info_pu),
	.pin_pd_grps = mtk_pin_info_pd,
	.n_pin_pd = ARRAY_SIZE(mtk_pin_info_pd),
	.pin_pupd_grps = mtk_pin_info_pupd,
	.n_pin_pupd = ARRAY_SIZE(mtk_pin_info_pupd),
	.pin_r0_grps = mtk_pin_info_r0,
	.n_pin_r0 = ARRAY_SIZE(mtk_pin_info_r0),
	.pin_r1_grps = mtk_pin_info_r1,
	.n_pin_r1 = ARRAY_SIZE(mtk_pin_info_r1),
	.mtk_pctl_set_pull_sel = mtk_pinctrl_set_gpio_pullsel,
	.mtk_pctl_get_pull_sel = mtk_pinctrl_get_gpio_pullsel,
	.mtk_pctl_get_pull_en = mtk_pinctrl_get_gpio_pullen,
	.spec_pinmux_set = mt8696_spec_pinmux_set,
	.mtk_pctl_set_gpio_drv = mtk_pctl_set_gpio_drving,
	.mtk_pctl_get_gpio_drv = mtk_pctl_get_gpio_drving,
	.dir_offset = 0x0000,
	.dout_offset = 0x0100,
	.din_offset = 0x0200,
	.pinmux_offset = 0x0300,
	.type1_start = 157,
	.type1_end = 157,
	.regmap_num = 6,
	.port_shf = 4,
	.port_mask = 0x1f,
	.port_align = 4,
	.port_pin_shf = 5,
	.eint_offsets = {
		.name = "mt8696_eint",
		.stat      = 0x000,
		.ack       = 0x040,
		.mask      = 0x080,
		.mask_set  = 0x0c0,
		.mask_clr  = 0x100,
		.sens      = 0x140,
		.sens_set  = 0x180,
		.sens_clr  = 0x1c0,
		.soft      = 0x200,
		.soft_set  = 0x240,
		.soft_clr  = 0x280,
		.pol       = 0x300,
		.pol_set   = 0x340,
		.pol_clr   = 0x380,
		.dom_en    = 0x400,
		.dbnc_ctrl = 0x500,
		.dbnc_set  = 0x600,
		.dbnc_clr  = 0x700,
		.port_mask = 0xf,
		.ports     = 8,
	},
	.ap_num = 232,
	.db_cnt = 232,
};

static int mtk_pinctrl_probe(struct platform_device *pdev)
{
	return mtk_pctrl_init(pdev, &mt8696_pinctrl_data, NULL);
}

static const struct of_device_id mt8696_pctrl_match[] = {
	{
		.compatible = "mediatek,mt8696-pinctrl",
	},
	{ }
};

static struct platform_driver mtk_pinctrl_driver = {
	.probe = mtk_pinctrl_probe,
	.driver = {
		.name = "mediatek-mt8696-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = mt8696_pctrl_match,
		.pm = &mtk_eint_pm_ops,
	},
};

static int __init mtk_pinctrl_init(void)
{
	return platform_driver_register(&mtk_pinctrl_driver);
}

arch_initcall(mtk_pinctrl_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Pinctrl Driver");
MODULE_AUTHOR("Zhiyong Tao <zhiyong.tao@mediatek.com>");
