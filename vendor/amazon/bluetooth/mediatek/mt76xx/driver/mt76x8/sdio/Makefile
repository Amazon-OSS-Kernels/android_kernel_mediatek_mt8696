export KBUILD_VERBOSE=1
#export KERNEL_SRC := /lib/modules/$(shell uname -r)/build

###############################################################################
# SDIO
###############################################################################
#ccflags-y += -DDBG=0
#ccflags-y += -I$(src)/os -I$(src)/os/linux/include
ifeq ($(MTK_CHIP_PCM), TRUE)
    ccflags-y += -DMTK_CHIP_PCM
endif

ifeq ($(MTK_WOBLE_ENABLE), TRUE)
    ccflags-y += -DMTK_WOBLE_ENABLE
endif

#VPATH = /opt/toolchains/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux
SDIO_MOD_NAME = btmtksdio
SDIO_CFILES := \
	btmtk_sdio.c \
        btmtk_main.c
ifeq ($(CONFIG_AMAZON_A2DP_TS),y)
    SDIO_CFILES += btif_audio_ts.c
endif

#ifneq ($(TARGET_BUILD_VARIANT), user)
#    ccflags-y += -DMTK_KERNEL_DEBUG
#endif

        ccflags-y += -DMTK_FULL_COREDUMP

$(SDIO_MOD_NAME)-objs := $(SDIO_CFILES:.c=.o)

###############################################################################
# Common
###############################################################################
obj-m := $(SDIO_MOD_NAME).o

PWD   := $(shell pwd)

all:
	make -C $(LINUX_SRC) M=$(PWD) modules

sdio:
	make -C $(LINUX_SRC) M=$(PWD) $(SDIO_MOD_NAME).ko
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions bin2h
	rm -f Module.markers
	rm -f Module.symvers
	rm -f modules.order

# Check coding style
export IGNORE_CODING_STYLE_RULES := NEW_TYPEDEFS,LEADING_SPACE,CODE_INDENT,SUSPECT_CODE_INDENT
ccs:
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_main.c
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_sdio.c
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_sdio.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_config.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_define.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_drv.h

