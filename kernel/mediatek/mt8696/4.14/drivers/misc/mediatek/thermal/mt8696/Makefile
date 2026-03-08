#
# Copyright (C) 2018 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/$(MTK_PLATFORM)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/video/include/
ccflags-y  += -I$(srctree)/drivers/thermal/
ccflags-y  += -I$(srctree)/drivers/gpu/img-rogue/services/system/rgx_mt8696/
ccflags-y  += -I$(srctree)/include/linux/soc/mediatek/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/mcupm/$(MTK_PLATFORM)

obj-  := dummy.o

ifeq ($(CONFIG_THERMAL), y)
else
    obj-y += mtk_thermal_dummy.o
endif

obj-y += mtk_thermal_platform.o
obj-y += mtk_thermal_timer.o

ifneq ($(CONFIG_MTK_COMBO_WIFI),)
ifneq ($(filter "MT76%", $(CONFIG_MTK_COMBO_CHIP)),)
    obj-$(CONFIG_THERMAL) += mtk_ts_wmt_766x.o
else
#    obj-y += mtk_ts_wmt.o
endif
endif

# Only included in non-EVB projects
ifeq ($(CONFIG_CHARGER_MTK), y)
    obj-y += mtk_cooler_bcct.o
    obj-y += mtk_ts_battery.o
endif

#obj-y += mtk_ts_bts.o
#obj-y += mtk_ts_cpu.o
obj-$(CONFIG_BTS_THERMISTOR) += lab126_ts_bts.o
obj-$(CONFIG_AMZN_THERMAL_COOLING) += amzn_thermal_cooling.o
ifeq ($(CONFIG_MTK_PMIC), y)
 obj-y += mtk_ts_pmic.o
endif
#obj-y += mtk_cooler_hpt.o
obj-y += mtk_tc.o
#obj-y += mtk_cooler_atm.o
obj-y += mtk_ts_cpu.o
#obj-y += mtk_cooler_dtm.o
obj-y += mtk_thermal_ipi.o
