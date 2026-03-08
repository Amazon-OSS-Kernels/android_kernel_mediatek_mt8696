ccflags-y += -I$(srctree)/drivers/staging/android/uapi/ \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/videox/	    \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/vdp/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/osd/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/adl/	 \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/ml/	 \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/pmx/	 \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/hdr/	 \
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/hdr/dovi_hdr/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/vdo/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/osd/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/fmt	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/videoin	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/vdout_sys/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/disp_sys/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/disp_mix/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/ml/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/adl/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/disp_hal/hdr/dovi/	\
			-I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/$(CONFIG_MTK_PLATFORM)/inc/	\
			-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys/fefifo/	 \
			-I$(srctree)/include/trustzone \
			-I$(srctree)/drivers/misc/mediatek/include/mt-plat/trustzone \
			-I$(srctree)/drivers/misc/mediatek/mmp \
			-I$(srctree)/drivers/misc/mediatek/sync/      \
			-I$(srctree)/drivers/misc/mediatek/mmp \
			-I$(srctree)/drivers/staging/android/mtk_ion \
			-I$(srctree)/drivers/staging/android/mtk_ion/mtk \
			-I$(srctree)/include/linux/soc/mediatek \
#common
obj-y += disp_dovi_tz_client.o
obj-y += disp_dovi_cmd.o
obj-y += disp_dovi_common_if.o
obj-y += disp_dovi_vdo_fe_if.o
obj-y += disp_dovi_gfx_fe_if.o
obj-y += disp_dovi_be_if.o
obj-y += disp_dovi_main.o
obj-y += disp_dovi_io.o
obj-y += disp_dovi_unit_test.o

ccflags-y += -DMET_USER_EVENT_SUPPORT

ifeq ($(CONFIG_MTK_ALPS_BOX_SUPPORT), y)
ccflags-y += -DMTK_ALPS_BOX_SUPPORT
endif

# EOF
