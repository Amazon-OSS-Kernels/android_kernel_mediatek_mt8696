ifeq ($(or $(strip $(CONFIG_AMAZON_METRICS_LOG)), $(strip $(CONFIG_AMAZON_MINERVA_METRICS_LOG))),y)
	obj-y += amazon_logger.o
endif
obj-$(CONFIG_USB_CH_TYPE_SYSFS) += usb_charge_type_sysfs.o
obj-$(CONFIG_AMAZON_METRICS_LOG_TEST)	+= amazon_logger_test.o
obj-$(CONFIG_AMAZON_SIGN_OF_LIFE) += amzn_sign_of_life.o
amzn_sign_of_life-objs += amzn_sign_of_life_v3.o amzn_sign_of_life_vendor.o
