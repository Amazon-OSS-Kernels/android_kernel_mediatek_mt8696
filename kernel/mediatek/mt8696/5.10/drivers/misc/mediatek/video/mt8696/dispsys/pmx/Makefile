# drivers/dispsys
#include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys \
             -I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/fmt \
             -I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/vdout_sys \
             -I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/disp_sys \
			 -I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/disp_mix \
             -I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/$(CONFIG_MTK_VIDEO_PLATFORM)/inc	\
             -I$(srctree)/include	\
             -I$(srctree)/include/linux/soc/mediatek \

#common
obj-y += disp_pmx_if.o

# EOF
