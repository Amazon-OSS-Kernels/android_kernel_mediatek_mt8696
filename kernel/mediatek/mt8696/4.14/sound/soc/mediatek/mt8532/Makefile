ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ifeq ($(CONFIG_SND_SOC_MT8570),y)
ccflags-y += \
    -I$(srctree)/sound/soc/mediatek/mt8570 \
    -I$(srctree)/drivers/misc/mediatek/hifi4dsp/hifi4dsp_audio_ipi/common/framework \
    -I$(srctree)/drivers/misc/mediatek/hifi4dsp/
endif
ifeq ($(CONFIG_MTK_AUDIO_ASRC),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/asrc \
             -I$(srctree)/drivers/misc/mediatek/asrc/common
endif
snd-soc-mt8532-pcm-objs := \
    mt8532-afe-pcm.o mt8532-afe-utils.o mt8532-snd-utils.o \
    mt8532-afe-controls.o mt8532-afe-debug.o

# MTK Platform Support
obj-$(CONFIG_SND_SOC_MT8532) += snd-soc-mt8532-pcm.o

# Machine support

ifeq ($(CONFIG_SND_SOC_MT8696_EVB)_$(CONFIG_SND_SOC_MT8532_EVB),y_y)
obj-y += mt8696-sound1.o
else ifeq ($(CONFIG_SND_SOC_MT8532_EVB),y)
obj-y += mt8532-evb.o
endif

#obj-$(CONFIG_SND_SOC_MT8532_EVB) += mt8532-evb.o
#obj-$(CONFIG_SND_SOC_MT8532A_P1) += mt8532-evb.o
#obj-$(CONFIG_SND_SOC_MT8532B_P1) += mt8532-evb.o
#obj-$(CONFIG_SND_SOC_MT8532S_P2) += mt8532-evb.o
