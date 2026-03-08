ccflags-y += -I$(srctree)/drivers/staging/android/uapi/ \
			-I$(srctree)/include/trustzone \
			-I$(srctree)/drivers/misc/mediatek/mmp \
			-I$(srctree)/drivers/misc/mediatek/sync/      \
			-I$(srctree)/drivers/staging/android/mtk_ion \
			-I$(srctree)/drivers/staging/android/ion \
			-I$(srctree)/arch/arm/mach-mt8580/include/mach/inc/ \
			-I$(srctree)/drivers/misc/mediatek/mdp/cli/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/videox/	    \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/vdp/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/osd/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/adl/	 \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/ml/	 \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/pmx/	 \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/hdr/dovi_hdr/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/hdr/open_hdr/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/vdo/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/osd/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/fmt	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/videoin	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/vdout_sys/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/disp_sys/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/disp_mix/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/ml/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/adl/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/hdr/dovi/ \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_VIDEO_PLATFORM)/dispsys/disp_hal/hdr/cfd/ \
			-I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/$(CONFIG_MTK_VIDEO_PLATFORM)/inc/	\
			-I$(srctree)/include/linux/soc/mediatek \

obj-y += disp_hdr_if.o
obj-y += disp_hdr_main.o
obj-y += disp_hdr_cmd.o
obj-y += dovi_hdr/

ifeq ($(CONFIG_MMPROFILE),y)
ccflags-y += -DDEFAULT_MMP_ENABLE
endif

obj-y += open_hdr/
# EOF
