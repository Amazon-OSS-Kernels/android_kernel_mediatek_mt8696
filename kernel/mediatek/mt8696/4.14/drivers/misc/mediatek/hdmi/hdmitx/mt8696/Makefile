
ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/mt8696/inc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/mt8696/dispsys

obj-y := hdmi_ioctl.o hdmi_drv.o hdmiavd.o hdmicec.o hdmiddc.o hdmiedid.o hdmihdcp.o hdmicmd.o hdmictrl.o hdmi_rpt_drv.o hdmi_tx_cod.o

ifneq ($(CONFIG_OPTEE)$(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT),)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/trustzone/
obj-y += hdmi_ca.o
endif

endif
