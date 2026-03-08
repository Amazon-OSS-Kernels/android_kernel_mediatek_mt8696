# SPDX-License-Identifier: GPL-2.0
#
# Copyright (c) 2019 MediaTek Inc.
#

ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include \
             -I$(srctree)/drivers/misc/mediatek/smi/ \
             -I$(srctree)/drivers/misc/mediatek/mdp_cmdq/$(MTK_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/mmp/ \
             -I$(srctree)/drivers/misc/mediatek/tee_gp/include/ \
             -I$(srctree)/drivers/misc/mediatek/include/

obj-y += cmdq-util.o
obj-y += $(subst ",,$(MTK_PLATFORM))/cmdq-platform.o
obj-y += cmdq-bw-mon.o
