#
# Copyright (C) 2016 MediaTek Inc.
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

# assign value
UTIL_DIR = util

# declare apusys version
MTK_PLATFORM = mt8188
APUSYS_RV_ENABLE = y
APUSYS_VERSION_MIDWARE = 2.0
APUSYS_VERSION_SAMPLE = 2.0
APUSYS_VERSION_DEBUG = 1.0
APUSYS_VERSION_MNOC = 1.5
APUSYS_VERSION_REVISER = 2.0
APUSYS_VERSION_VPU = 5.0
subdir-ccflags-y += -DRV_COMP

# target object
obj-$(CONFIG_MTK_APUSYS_SUPPORT) += apusys.o

# common include path
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/include/
ccflags-y += -I$(srctree)/drivers/iommu/

# core makefile
apusys-objs += core/apusys_core.o

# middleware makefile
ccflags-y += -I$(srctree)/drivers/staging/android/mtk_ion
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/slbc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/debug/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/midware/$(APUSYS_VERSION_MIDWARE)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/$(UTIL_DIR)/

ifeq ($(APUSYS_VERSION_MIDWARE), 1.1)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/midware/$(MTK_PLATFORM)/

apusys-$(CONFIG_FTRACE) += $(UTIL_DIR)/apusys_trace.o

apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_drv.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_usr.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_dbg.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_rsc.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_cmn.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_cmd.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_sched.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_queue.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_queue_norm.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_queue_deadline.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_dispr.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_mem.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_mem_ion.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_fence.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_sysfs.o
apusys-$(CONFIG_MTK_APUSYS_DEBUG) += midware/$(APUSYS_VERSION_MIDWARE)/mdw_tag.o

apusys-objs += midware/$(MTK_PLATFORM)/dump_reg.o
apusys-objs += midware/$(MTK_PLATFORM)/secure_perf.o
else
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/midware/$(APUSYS_VERSION_MIDWARE)/ap/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/midware/$(APUSYS_VERSION_MIDWARE)/rv/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/midware/$(APUSYS_VERSION_MIDWARE)/ap/dummy/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/$(UTIL_DIR)/

apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_mem_drv.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_mem_rsc.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_drv.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_ctx.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_dev.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_ioctl.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_mem.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_mem_dma.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_mem_aram.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_mem_pool.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_cmd.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_cmn.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_hs.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_sysfs.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_dbg.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_import.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/mdw_util.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap_cmd.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap_sched.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap_dispr.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap_queue.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap_queue_deadline.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap_queue_norm.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap_rsc.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/mdw_ap_tag.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/ap/dummy/mdw_dmy.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/rv/mdw_rv.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/rv/mdw_rv_cmd.o
apusys-objs += midware/$(APUSYS_VERSION_MIDWARE)/rv/mdw_rv_dev.o
apusys-$(CONFIG_MTK_APUSYS_DEBUG) += midware/$(APUSYS_VERSION_MIDWARE)/rv/mdw_rv_tag.o

apusys-$(CONFIG_FTRACE) += $(UTIL_DIR)/apusys_trace.o
endif

# sample driver makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/sample/$(APUSYS_VERSION_SAMPLE)/
ifeq ($(APUSYS_RV_ENABLE), n)
apusys-objs += sample/$(APUSYS_VERSION_SAMPLE)/sample_drv.o
apusys-objs += sample/$(APUSYS_VERSION_SAMPLE)/sample_inf.o
endif

#edma makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/edma/
ifeq ($(APUSYS_RV_ENABLE), n)
apusys-objs += edma/edma_driver.o
apusys-objs += edma/edma_dbgfs.o
apusys-objs += edma/edma_queue.o
apusys-objs += edma/edma_cmd_hnd.o
else
apusys-objs += edma/edma_rv.o
endif

# reviser driver makefile
REVISER=$(srctree)/drivers/misc/mediatek/apusys/
ifeq ($(APUSYS_RV_ENABLE), n)
REVISER_PLAT=reviser/$(MTK_PLATFORM)
REVISER_COMN=reviser/$(APUSYS_VERSION_REVISER)

ccflags-y += -I$(REVISER)/$(REVISER_PLAT)
ccflags-y += -I$(REVISER)/$(REVISER_COMN)

apusys-objs += $(REVISER_COMN)/reviser_drv.o
apusys-objs += $(REVISER_COMN)/reviser_hw.o
apusys-objs += $(REVISER_COMN)/reviser_mem.o
apusys-objs += $(REVISER_COMN)/reviser_mem_mgt.o
apusys-objs += $(REVISER_COMN)/reviser_dbg.o
apusys-objs += $(REVISER_COMN)/reviser_export.o
apusys-objs += $(REVISER_PLAT)/reviser_reg.o
else
REVISER_PLAT=reviser/$(APUSYS_VERSION_REVISER)/platform
REVISER_COMN=reviser/$(APUSYS_VERSION_REVISER)/common

ccflags-y += -I$(REVISER)/$(REVISER_PLAT)
ccflags-y += -I$(REVISER)/$(REVISER_COMN)
ccflags-y += -I$(REVISER)/$(REVISER_PLAT)/v_1_0
ccflags-y += -I$(REVISER)/$(REVISER_PLAT)/v_rv

apusys-objs += $(REVISER_COMN)/reviser_drv.o
apusys-objs += $(REVISER_COMN)/reviser_dbg.o
apusys-objs += $(REVISER_COMN)/reviser_export.o
apusys-objs += $(REVISER_COMN)/reviser_import.o
apusys-objs += $(REVISER_COMN)/reviser_hw_mgt.o
apusys-objs += $(REVISER_COMN)/reviser_mem.o
apusys-objs += $(REVISER_COMN)/reviser_power.o
apusys-objs += $(REVISER_COMN)/reviser_table_mgt.o
apusys-objs += $(REVISER_COMN)/reviser_remote.o
apusys-objs += $(REVISER_COMN)/reviser_remote_cmd.o

apusys-objs += $(REVISER_PLAT)/reviser_device.o
apusys-objs += $(REVISER_PLAT)/reviser_plat.o

apusys-objs += $(REVISER_PLAT)/v_1_0/reviser_hw_v1_0.o
apusys-objs += $(REVISER_PLAT)/v_1_0/reviser_plat_v1_0.o
apusys-objs += $(REVISER_PLAT)/v_1_0/reviser_reg_v1_0.o
apusys-objs += $(REVISER_PLAT)/v_rv/reviser_hw_vrv.o
apusys-objs += $(REVISER_PLAT)/v_rv/reviser_plat_vrv.o
endif

# mnoc makefile
ccflags-y += -DMNOC_MET_PMU_FTRACE
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/mnoc/$(APUSYS_VERSION_MNOC)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/mnoc/$(MTK_PLATFORM)

ifeq ($(APUSYS_RV_ENABLE), n)
apusys-objs += mnoc/$(APUSYS_VERSION_MNOC)/mnoc_drv.o
apusys-objs += mnoc/$(APUSYS_VERSION_MNOC)/mnoc_dbg.o
apusys-objs += mnoc/$(APUSYS_VERSION_MNOC)/mnoc_qos.o
apusys-objs += mnoc/$(APUSYS_VERSION_MNOC)/mnoc_pmu.o
apusys-objs += mnoc/$(APUSYS_VERSION_MNOC)/mnoc_qos_sys.o
apusys-objs += mnoc/$(MTK_PLATFORM)/mnoc_hw.o
else
apusys-objs += mnoc/$(APUSYS_VERSION_MNOC)/common/mnoc_rv.o
endif

# debug makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/debug/$(APUSYS_VERSION_DEBUG)/common/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/debug/$(APUSYS_VERSION_DEBUG)/interface/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/debug/$(APUSYS_VERSION_DEBUG)/platform/

ifeq ($(CONFIG_MTK_APUSYS_DEBUG),y)
apusys-objs += debug/apu_tp.o
apusys-objs += debug/apu_tags.o

ifeq ($(APUSYS_RV_ENABLE), n)
apusys-objs += debug/debug_driver.o
else
apusys-objs += debug/$(APUSYS_VERSION_DEBUG)/common/debug_drv.o
apusys-objs += debug/$(APUSYS_VERSION_DEBUG)/platform/debug_plat_device.o
apusys-objs += debug/$(APUSYS_VERSION_DEBUG)/platform/debug_platform.o
endif
endif

# power
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/apusys/power/2.0/$(MTK_PLATFORM)),)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/$(MTK_PLATFORM)/
else
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/dummy/
endif
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/3.0/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/3.0/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/3.0/include/platform/
obj-$(CONFIG_MTK_APUSYS_SUPPORT) += apu_top/
obj-$(CONFIG_MTK_APUSYS_SUPPORT) += power/

# mdla makefile
subdir-ccflags-y += -DLEGACY_PROJ

src=$(srctree)/drivers/misc/mediatek/apusys
ccflags-y += -I$(src)/mdla/
apusys-objs += $(shell cd $(src) && find mdla -name "*.c" | sed 's/\.c/\.o/g')

# vpu makefile
VPU = vpu/$(APUSYS_VERSION_VPU)
VPU_IP = vpu/p1
VPU_PLAT = vpu/$(MTK_PLATFORM)
# RemoteRPC
ccflags-y += -I$(srctree)/drivers/remoteproc/
# VPU driver
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/$(VPU)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/$(VPU_PLAT)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/$(VPU_IP)
ccflags-y += -I$(srctree)/drivers/base

ifeq ($(CONFIG_MTK_APUSYS_VPU),y)
apusys-objs += $(VPU)/vpu_debug.o
apusys-objs += $(VPU)/vpu_main.o
apusys-objs += $(VPU)/vpu_mem.o
apusys-objs += $(VPU)/vpu_cfg.o
apusys-objs += $(VPU)/vpu_algo.o
apusys-objs += $(VPU)/vpu_hw.o
apusys-objs += $(VPU)/vpu_loadimage.o
endif

# devapc makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/devapc/
apusys-objs += devapc/dapc.o
apusys-objs += devapc/dapc_cfg.o

# apusys_rv makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/apusys_rv
apusys-objs += apusys_rv/apu_config.o
apusys-objs += apusys_rv/apu_coredump.o
apusys-objs += apusys_rv/apu_excep.o
apusys-objs += apusys_rv/apu_ipi.o
apusys-objs += apusys_rv/apu_mbox.o
apusys-objs += apusys_rv/apu_mem.o
apusys-objs += apusys_rv/apu_rproc.o
apusys-objs += apusys_rv/apu_sysfs.o
apusys-objs += apusys_rv/apu_timesync.o
apusys-objs += apusys_rv/apusys_rv.o
apusys-objs += apusys_rv/apu_deepidle.o
apusys-objs += apusys_rv/apu_regdump.o
apusys-objs += apusys_rv/apu_debug.o
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/apusys/apusys_rv/2.0/mt8188_plat.c),)
subdir-ccflags-y += -DMT8188_APUSYS_RV_PLAT_DATA
apusys-objs += apusys_rv/2.0/mt8188_plat.o
endif
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/apusys/apusys_rv/2.0/mt8195_plat.c),)
subdir-ccflags-y += -DMT8195_APUSYS_RV_PLAT_DATA
apusys-objs += apusys_rv/2.0/mt8195_plat.o
endif
# ipi makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/3.0/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/3.0/include/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/midware/2.0/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/mdla/rv/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/mvpu/1.0/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/reviser/2.0/common
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/ipi
ccflags-y += -I$(srctree)/drivers/staging/android/ion
ccflags-y += -I$(srctree)/drivers/rpmsg/
apusys-objs += ipi/mtk_apu_rpmsg.o


# hw_logger makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/hw_logger/
apusys-objs += hw_logger/hw_logger.o

# sw_logger makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/sw_logger/
apusys-objs += sw_logger/sw_logger.o
