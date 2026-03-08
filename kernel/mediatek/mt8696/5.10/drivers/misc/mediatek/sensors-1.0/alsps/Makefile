# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
obj-$(CONFIG_CUSTOM_KERNEL_ALSPS)   += alsps-drv.o
alsps-drv-objs   :=  alsps.o alsps_factory.o
obj-$(CONFIG_CUSTOM_KERNEL_ALSPS)   +=  aal_control.o

obj-$(CONFIG_MTK_ALSPSHUB) += alspshub/
obj-y	+=  ltr559/
#obj-$(CONFIG_MTK_CM36558) += cm36558/
#obj-$(CONFIG_MTK_LTR303) += ltr303/
