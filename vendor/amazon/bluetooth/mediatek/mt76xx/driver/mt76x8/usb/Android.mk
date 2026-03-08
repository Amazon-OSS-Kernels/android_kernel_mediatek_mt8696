# Copyright Statement:
#
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

local_path_full := $(shell pwd)/$(LOCAL_PATH)
btusb_module_out_path := $(PRODUCT_OUT)$(BT_DRIVER_MODULE_PATH)
btusb_module_target := $(btusb_module_out_path)

LOCAL_MODULE := btmtk_usb
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(btusb_module_target)

include $(BUILD_PHONY_PACKAGE)

$(LOCAL_ADDITIONAL_DEPENDENCIES): PRIVATE_DRIVER_LOCAL_DIR := $(local_path_full)
$(LOCAL_ADDITIONAL_DEPENDENCIES): PRIVATE_DRIVER_OUT := $(btusb_module_out_path)
$(LOCAL_ADDITIONAL_DEPENDENCIES): $(INSTALLED_KERNEL_TARGET)
	$(hide) rm -rf $(PRIVATE_DRIVER_OUT)
	$(MAKE) -C $(KERNEL_OUT) M=$(PRIVATE_DRIVER_LOCAL_DIR) ARCH=$(TARGET_KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) modules
	$(hide) cp -f $(PRIVATE_DRIVER_LOCAL_DIR)/$(BT_DRIVER_MODULE_NAME).ko $(PRIVATE_DRIVER_OUT)
	$(MAKE) -C $(KERNEL_OUT) M=$(PRIVATE_DRIVER_LOCAL_DIR) ARCH=$(TARGET_KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) clean

local_path_full :=
btusb_module_out_path :=
btusb_module_target :=

