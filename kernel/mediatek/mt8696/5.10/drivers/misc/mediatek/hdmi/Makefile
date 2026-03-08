# SPDX-License-Identifier:GPL-2.0
#
# Makefile for the hdmi device drivers
#

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_HDMI_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_HDMI_PLATFORM)/videox/ \
             -I$(srctree)/drivers/staging/android \
             -I$(srctree)/drivers/misc/mediatek/dispsys/$(CONFIG_MTK_HDMI_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/lcm/inc \
             -I$(srctree)/drivers/misc/mediatek/include/mt-plat/

ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/$(CONFIG_MTK_HDMI_PLATFORM)
endif

#obj-$(CONFIG_MTK_HDMI_SUPPORT) += Sii8348/
#obj-y += Sii8348/
ifeq ($(CONFIG_MTK_HDMI_PLATFORM),"mt8135")
obj-y	:= hdmitx.o debug.o  hdmitx_test.o
endif

ifeq ($(CONFIG_CUSTOM_KERNEL_HDMI), "ITE66121")
obj-y	+= it66121/
endif

ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
obj-y  += hdmitx/$(subst ",,$(CONFIG_MTK_HDMI_PLATFORM))/
endif

ifneq ($(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT),)
ccflags-y += -I$(srctree)/include/trustzone/
endif
