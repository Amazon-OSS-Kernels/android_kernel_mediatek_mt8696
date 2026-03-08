ccflags-y += -I$(srctree)/include/                              \
             -I$(srctree)/drivers/staging/android/mtk_ion/mtk/  \
             -I$(srctree)/include/soc/mediatek/                 \
             -I$(srctree)/drivers/staging/android/ion/          \
             -I$(srctree)/drivers/misc/mediatek/video/mt8696/dispsys \
             -I$(srctree)/drivers/misc/mediatek/mmp \
             -I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/mt8696/inc \
             -I$(srctree)/drivers/misc/mediatek/include/mt-plat \
             -I$(srctree)/drivers/misc/mediatek/include/mt-plat/trustzone/   \

obj-y += imgresz_hal.o imgresz.o
obj-$(CONFIG_MTK_IMGRZ_TEST) += ./test/imgresz_test.o
