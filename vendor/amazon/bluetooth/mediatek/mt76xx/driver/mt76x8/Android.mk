ifeq ($(BOARD_BLUETOOTH_DEVICE),mediatek)
LOCAL_PATH := $(call my-dir)

    ifeq ($(BOARD_MTK_ONE_IMAGE),true)
        include $(LOCAL_PATH)/usb/Android.mk
    else ifeq ($(BT_DRIVER_MODULE_NAME),btmtk_usb)
        include $(LOCAL_PATH)/usb/Android.mk
    else ifeq ($(BT_DRIVER_MODULE_NAME),btmtksdio)
        include $(LOCAL_PATH)/sdio/Android.mk
    endif
endif
