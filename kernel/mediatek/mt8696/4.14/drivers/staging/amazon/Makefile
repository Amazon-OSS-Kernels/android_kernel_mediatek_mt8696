ifeq ($(or $(strip $(CONFIG_AMAZON_METRICS_LOG)), $(strip $(CONFIG_AMAZON_MINERVA_METRICS_LOG))),y)
	obj-y += amazon_logger.o
endif
obj-$(CONFIG_AMAZON_METRICS_LOG_TEST)	+= amazon_logger_test.o
obj-$(CONFIG_AMAZON_SIGN_OF_LIFE)	+= sign_of_life.o sign_of_life_vendor.o
