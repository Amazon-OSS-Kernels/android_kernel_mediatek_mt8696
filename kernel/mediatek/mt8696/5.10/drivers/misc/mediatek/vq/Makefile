ccflags-y += -I$(srctree)/include/linux/                        \
             -I$(srctree)/include/soc/mediatek/                 \
             -I$(srctree)/drivers/misc/mediatek/sync/           \
             -I$(srctree)/drivers/staging/android               \
             -I$(srctree)/drivers/staging/android/uapi/         \
             -I$(srctree)/include/trustzone/

obj-y += mtk_vq_mgr.o mtk_vq_debug.o di_hal.o

obj-$(CONFIG_MTK_NR) += nr_hal.o