ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

snd-soc-mt8696-pcm-objs := \
    mt8696-afe-pcm.o mt8696-afe-utils.o mt8696-afe-controls.o mt8696-afe-debug.o

# MTK Platform Support
obj-$(CONFIG_SND_SOC_MT8696) += snd-soc-mt8696-pcm.o
# Machine support
obj-$(CONFIG_SND_SOC_MT8696_EVB) += mt8696-evb.o
obj-$(CONFIG_SND_SOC_MT8696_FPGA) += mt8696-evb.o
