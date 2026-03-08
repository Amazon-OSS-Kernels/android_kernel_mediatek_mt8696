#
# Copyright (C) 2019 MediaTek Inc.
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

ifeq ($(CONFIG_MTK_IOMMU_MT8XXX), y)
# MT8XXX
ccflags-y += -I$(srctree)/drivers/misc/mediatek/iommu
ifneq ($(filter y m, $(CONFIG_OPTEE)),)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/trustzone/
endif

obj-$(CONFIG_MTK_IOMMU_MISC) += mtk_dma_buf_helper.o
obj-$(CONFIG_MTK_IOMMU_MISC_SECURE) += mtk_iommu_sec.o

else
# Smart phone
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/iommu
ccflags-y += -I$(srctree)/drivers/dma-buf/heaps

obj-$(CONFIG_MTK_IOMMU_MISC_DBG)    += iommu_debug.o
obj-$(CONFIG_MTK_IOMMU_MISC_SECURE) += iommu_secure.o
obj-$(CONFIG_MTK_IOMMU_MISC_TEST)   += iommu_test.o
endif
