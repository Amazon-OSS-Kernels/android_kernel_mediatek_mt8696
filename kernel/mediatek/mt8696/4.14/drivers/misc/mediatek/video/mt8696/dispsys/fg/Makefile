# drivers/dispsys
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y +=  -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/inc	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/sync/      \
							-I$(srctree)/drivers/misc/mediatek/mmp \
							-I$(srctree)/drivers/staging/android/ion \
							-I$(srctree)/drivers/staging/android/ion/mtk \
							-I$(srctree)/drivers/staging/android/uapi/ \
							-I$(srctree)/arch/arm/mach-mt8580/include/mach/inc/ \
							-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/fefifo/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/fg	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/fmt	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/vdout_sys	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/disp_sys \
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/videox/	\
							-I$(srctree)/drivers/misc/mediatek/mdp/cli/	\
							-I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/$(CONFIG_MTK_PLATFORM)/inc/	\
							-I$(srctree)/include/trustzone \
							-I$(srctree)/include/linux \
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/hdr/dovi_hdr/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/hdr/open_hdr/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/dovi/inc   \
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/hdr/cfd   \
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/hdr/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/adl/	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/videoin	\
							-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/disp_mix	\
							-I$(srctree)/drivers/misc/mediatek/include/mt-plat/trustzone	\
							-I$(srctree)/include/linux/soc/mediatek \
							-I$(srctree)/drivers/misc/mediatek/cmdq/mailbox	\


#common
obj-y += disp_fg_if.o disp_fg_drv.o disp_fg_debug.o disp_fg_tz_client.o


# EOF
