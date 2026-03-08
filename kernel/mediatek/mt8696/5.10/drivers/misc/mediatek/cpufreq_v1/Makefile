# SPDX-License-Identifier: GPL-2.0

ccflags-y += \
	-I$(srctree)/include/ \

obj-$(CONFIG_MEDIATEK_CPUFREQ_DEBUG) += cpufreq-dbg.o cpufreq_qos-dbg.o
subdir-ccflags-$(CONFIG_MEDIATEK_EEM_DEBUG) += -DEEM_DBG
obj-$(CONFIG_MEDIATEK_EEM_DEBUG) += eem-dbg.o
