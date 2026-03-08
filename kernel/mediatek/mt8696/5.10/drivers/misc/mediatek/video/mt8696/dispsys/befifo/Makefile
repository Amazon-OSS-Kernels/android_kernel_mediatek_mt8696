# drivers/dispsys
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -DMET_USER_EVENT_SUPPORT
ccflags-y +=  -I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/inc	\
							-I$(srctree)/drivers/misc/mediatek/sync/ \
							-I$(srctree)/drivers/misc/mediatek/mmp \
							-I$(srctree)/drivers/staging/android/uapi/ \
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/videox/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/fmt	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/vdout_sys	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/disp_sys \
							-I$(srctree)/drivers/misc/mediatek/mdp/cli/	\
							-I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/$(CONFIG_MTK_VIDEO_PLATFORM)/inc/ \
							-I$(srctree)/include/linux/soc/mediatek \

#common
obj-y += disp_befifo_debug.o
obj-y += disp_befifo_if.o
obj-y += disp_befifo_drv.o
obj-y += disp_befifo_hal.o

# EOF
