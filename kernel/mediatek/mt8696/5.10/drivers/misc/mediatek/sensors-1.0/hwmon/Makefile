ifneq ($(filter y m, $(CONFIG_MTK_SENSOR_SUPPORT)),)
obj-$(CONFIG_MTK_HWMON) += hwmsen/
obj-$(CONFIG_MTK_HWMON) += sensor_dts/
obj-$(CONFIG_MTK_HWMON) += sensor_attributes/
obj-$(CONFIG_MTK_HWMON) += sensor_event/
obj-$(CONFIG_MTK_HWMON) += sensor_performance/
obj-$(CONFIG_MTK_HWMON) += sensor_list/
else
obj-n := hwmsen/
endif
