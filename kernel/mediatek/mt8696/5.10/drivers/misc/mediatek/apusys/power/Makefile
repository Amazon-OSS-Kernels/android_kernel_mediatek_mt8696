obj-$(CONFIG_MTK_APUSYS_SUPPORT) += apu_top.o

# 2.0
PDIR = $(MTK_PLATFORM)

ccflags-y += -I$(srctree)/drivers/devfreq
ccflags-y += -I$(srctree)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(PDIR)/include/mach/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/debug/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/test/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/$(PDIR)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/midware/1.1/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/freqhopping/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/freqhopping/$(MTK_PLATFORM)/

# FPGA environment
ifeq ($(CONFIG_MTK_FPGA),y)
subdir-ccflags-y += -DMTK_FPGA_PORTING
else ifeq ($(CONFIG_FPGA_EARLY_PORTING),y)
subdir-ccflags-y += -DMTK_FPGA_PORTING
endif

apu_top-objs += 2.0/apusys_power_driver.o
apu_top-objs += 2.0/apusys_power_ctl.o
apu_top-objs += 2.0/apusys_power_debug.o

ifneq ($(PDIR),)
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/apusys/power/2.0/$(PDIR)),)


ifeq ($(CONFIG_DEVFREQ_THERMAL),y)
apu_top-objs += 2.0/$(PDIR)/apu_power_table.o
endif

ifeq (,$(filter $(strip $(MTK_PLATFORM)), mt6885))
apu_top-objs += 2.0/$(PDIR)/apu_power_tag.o
endif

APUPW_PLATFORM_DIR = $(PDIR)
apu_top-objs += 2.0/$(APUPW_PLATFORM_DIR)/apu_power_regulator.o
apu_top-objs += 2.0/$(APUPW_PLATFORM_DIR)/apu_power_clock.o

else
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/dummy/
APUPW_PLATFORM_DIR = dummy
endif
else
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/dummy/
APUPW_PLATFORM_DIR = dummy
endif

apu_top-objs += 2.0/$(APUPW_PLATFORM_DIR)/hal_config_power.o
apu_top-objs += 2.0/$(APUPW_PLATFORM_DIR)/apusys_power_cust.o
apu_top-objs += 2.0/$(APUPW_PLATFORM_DIR)/apu_platform_resource.o
apu_top-objs += 2.0/$(APUPW_PLATFORM_DIR)/apu_platform_debug.o
apu_top-objs += 2.0/$(APUPW_PLATFORM_DIR)/apusys_power_rule_check.o


ifneq (,$(findstring aging,$(PROJECT_DTB_NAMES)))
subdir-ccflags-y += -DAGING_MARGIN
endif


# 3.0
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/midware/2.0/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/3.0/inc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/3.0/inc/platform/

#src = $(srctree)/drivers/misc/mediatek/apusys/power/3.0
#apu_top-objs += $(shell cd $(src) && find . -name "*.c" | sed 's/\.c/\.o/g')
apu_top-objs += apu_top_entry.o
apu_top-objs += 3.0/aputop_drv.o
apu_top-objs += 3.0/aputop_rpmsg.o
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/apusys/power/3.0/platform/mt8188_aputop.c),)
apu_top-objs += 3.0/platform/mt8188_aputop.o
apu_top-objs += 3.0/platform/mt8188_pwr_func.o
endif
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/apusys/power/3.0/platform/mt8195_aputop.c),)
apu_top-objs += 3.0/platform/mt8195_aputop.o
apu_top-objs += 3.0/platform/mt8195_pwr_func.o
endif

ifeq ($(CONFIG_DEVFREQ_THERMAL),y)
apu_top-objs += 3.0/apu_devfreq_cooling.o
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/apusys/power/3.0/platform/mt8188_apu_devfreq_cooling.c),)
apu_top-objs += 3.0/platform/mt8188_apu_devfreq_cooling.o
endif
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/apusys/power/3.0/platform/mt8195_apu_devfreq_cooling.c),)
apu_top-objs += 3.0/platform/mt8195_apu_devfreq_cooling.o
endif
endif
