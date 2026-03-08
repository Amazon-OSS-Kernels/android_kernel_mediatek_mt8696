// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <helio-dvfsrc.h>

static struct pm_qos_request dvfsrc_ddr_opp_req;
static struct pm_qos_request dvfsrc_vcore_opp_req;
static struct pm_qos_request dvfsrc_vcore_dvfs_opp_force;

static ssize_t dvfsrc_req_ddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	pm_qos_update_request(&dvfsrc_ddr_opp_req, val);

	return count;
}
static DEVICE_ATTR(dvfsrc_req_ddr, 0200,
		NULL, dvfsrc_req_ddr_store);

static ssize_t dvfsrc_req_vcore_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	pm_qos_update_request(&dvfsrc_vcore_opp_req, val);

	return count;
}
static DEVICE_ATTR(dvfsrc_req_vcore, 0200,
		NULL, dvfsrc_req_vcore_store);

static ssize_t dvfsrc_force_opp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	pm_qos_update_request(&dvfsrc_vcore_dvfs_opp_force, val);

	return count;
}
static DEVICE_ATTR(dvfsrc_force_opp, 0200,
		NULL, dvfsrc_force_opp_store);

static ssize_t dvfsrc_opp_table_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;

	p = dvfsrc_dump_opp_table(p);

	return p - buf;
}

static DEVICE_ATTR(dvfsrc_opp_table, 0444, dvfsrc_opp_table_show, NULL);

static ssize_t dvfsrc_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;

	p = dvfsrc_dump_reg(p);

	return p - buf;
}

static DEVICE_ATTR(dvfsrc_dump, 0444, dvfsrc_dump_show, NULL);

static struct attribute *helio_dvfsrc_attrs[] = {
	&dev_attr_dvfsrc_req_ddr.attr,
	&dev_attr_dvfsrc_req_vcore.attr,
	&dev_attr_dvfsrc_force_opp.attr,
	&dev_attr_dvfsrc_opp_table.attr,
	&dev_attr_dvfsrc_dump.attr,
	NULL,
};

static struct attribute_group helio_dvfsrc_attr_group = {
	.name = "helio-dvfsrc",
	.attrs = helio_dvfsrc_attrs,
};

int helio_dvfsrc_add_interface(struct device *dev)
{
	pm_qos_add_request(&dvfsrc_ddr_opp_req, PM_QOS_EMI_OPP,
			PM_QOS_EMI_OPP_DEFAULT_VALUE);
	pm_qos_add_request(&dvfsrc_vcore_opp_req, PM_QOS_VCORE_OPP,
			PM_QOS_VCORE_OPP_DEFAULT_VALUE);
	pm_qos_add_request(&dvfsrc_vcore_dvfs_opp_force,
			PM_QOS_VCORE_DVFS_FIXED_OPP,
			PM_QOS_VCORE_DVFS_FIXED_OPP_DEFAULT_VALUE);

	return sysfs_create_group(&dev->kobj, &helio_dvfsrc_attr_group);
}

void helio_dvfsrc_remove_interface(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &helio_dvfsrc_attr_group);
}
