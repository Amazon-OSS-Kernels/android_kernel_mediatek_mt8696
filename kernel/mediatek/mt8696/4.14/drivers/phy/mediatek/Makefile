# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the phy drivers.
#

obj-$(CONFIG_PHY_MTK_TPHY)		+= phy-mtk-tphy.o
ifneq ($(CONFIG_PHY_MTK_TPHY),)
obj-$(CONFIG_DEBUG_FS)		+= phy-mtk-tphy-debug.o
endif

# FPGA USB Daughter-Board Drivers
obj-$(CONFIG_PHY_MTK_USB3_I2C_FPGA)	+= phy-mtk-u3phy-i2c.o
obj-$(CONFIG_PHY_MTK_A60810)		+= phy-mtk-a60810.o
obj-$(CONFIG_PHY_MTK_A60931)		+= phy-mtk-a60931.o
obj-$(CONFIG_PHY_MTK_A60XXX)		+= phy-mtk-a60xxx.o