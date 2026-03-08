#export LINUX_SRC := /home/luke.hsieh/abc123_VSB_518_1/out/target/product/apple/obj/KERNEL_OBJ
###############################################################################
# USB
###############################################################################
USB_MOD_NAME = btmtk_usb
USB_CFILES := \
	btmtk_usb_main.c \
	btmtk_usb_fifo.c
$(USB_MOD_NAME)-objs := $(USB_CFILES:.c=.o)

###############################################################################
# SDIO
###############################################################################
#SDIO_MOD_NAME = btmtk_sdio
#SDIO_CFILES := \
#	btmtk_sdio_main.c
#$(SDIO_MOD_NAME)-objs := $(SDIO_CFILES:.c=.o)

###############################################################################
# Common
###############################################################################
obj-m := $(USB_MOD_NAME).o \
#	$(SDIO_MOD_NAME).o

PWD     := $(shell pwd)

all:
	make -C $(LINUX_SRC) M=$(PWD) modules

usb:
	make -C $(LINUX_SRC) M=$(PWD) $(USB_MOD_NAME).ko

#sdio:
#	make -C $(LINUX_SRC) M=$(PWD) $(SDIO_MOD_NAME).ko

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions bin2h
	rm -f Module.markers
	rm -f Module.symvers
	rm -f modules.order

###############################################################################
# Check coding style
###############################################################################
#ccs:
#	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_main.c
#	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_main.h
#	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_config.h
#	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_define.h
#	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_dbgfs.c
#	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_dbgfs.h
#	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_fifo.c
#	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 -f btmtk_usb_fifo.h
