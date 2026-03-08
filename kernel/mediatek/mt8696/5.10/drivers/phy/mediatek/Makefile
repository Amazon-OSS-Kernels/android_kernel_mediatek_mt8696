# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the phy drivers.
#

obj-$(CONFIG_PHY_MTK_TPHY)		+= tphy.o
tphy-y							:= phy-mtk-tphy.o
ifneq ($(CONFIG_DEBUG_FS),)
tphy-y				   			+= phy-mtk-tphy-debug.o
endif

# FPGA USB Daughter-Board Drivers
ifneq ($(CONFIG_PHY_MTK_A60810),)
obj-$(CONFIG_PHY_MTK_A60810)	+= tphy.o
tphy-y							:= phy-mtk-a60810.o
ifneq ($(CONFIG_PHY_MTK_USB3_I2C_FPGA),)
tphy-y							+= phy-mtk-u3phy-i2c.o
endif
endif

ifneq ($(CONFIG_PHY_MTK_A60931),)
obj-$(CONFIG_PHY_MTK_A60931)	+= tphy.o
tphy-y							:= phy-mtk-a60931.o
ifneq ($(CONFIG_PHY_MTK_USB3_I2C_FPGA),)
tphy-y							+= phy-mtk-u3phy-i2c.o
endif
endif

ifneq ($(CONFIG_PHY_MTK_A60XXX),)
obj-$(CONFIG_PHY_MTK_A60XXX)	+= tphy.o
tphy-y							:= phy-mtk-a60xxx.o
ifneq ($(CONFIG_PHY_MTK_USB3_I2C_FPGA),)
tphy-y							+= phy-mtk-u3phy-i2c.o
endif
endif

obj-$(CONFIG_PHY_MTK_A60810)		+= phy-mtk-a60810.o
obj-$(CONFIG_PHY_MTK_A60931)		+= phy-mtk-a60931.o
obj-$(CONFIG_PHY_MTK_A60XXX)		+= phy-mtk-a60xxx.o
obj-$(CONFIG_PHY_MTK_DP)		+= phy-mtk-dp.o
obj-$(CONFIG_PHY_MTK_UFS)		+= phy-mtk-ufs.o
obj-$(CONFIG_PHY_MTK_XSPHY)		+= phy-mtk-xsphy.o
obj-$(CONFIG_PHY_MTK_FPGAPHY)		+= phy-mtk-fpgaphy.o
obj-$(CONFIG_FPGA_EARLY_PORTING)	+= fpga/
phy-mtk-hdmi-drv-y			:= phy-mtk-hdmi.o
phy-mtk-hdmi-drv-y			+= phy-mtk-hdmi-mt2701.o
phy-mtk-hdmi-drv-y			+= phy-mtk-hdmi-mt8173.o
obj-$(CONFIG_PHY_MTK_HDMI)		+= phy-mtk-hdmi-drv.o
