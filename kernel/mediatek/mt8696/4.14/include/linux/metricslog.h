/*
 * Copyright 2011-2017 Amazon Technologies, Inc. All Rights Reserved.
 * Portions Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _LINUX_METRICSLOG_H
#define _LINUX_METRICSLOG_H

#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
/* Add required keys here */
/* Add GROUP_ID */
#define METRICS_THERMAL_GROUP_ID "o3cc69de"

#define METRICS_THERMISTOR_SCHEMA_ID "sk77/2/03330430"

#define MINERVA_PREDEFINED_REQUIRED_FIELDS "_deviceId=;SY,_deviceType=;SY,_softwareVersion=;SY,_platform=;SY,_marketPlaceId=;SY,_countryOfResidence=;SY,_otaGroupName=;SY,_osFileTag=;SY"

/* Add SCHEMA_ID */
#endif /* CONFIG_AMAZON_MINERVA_METRICS_LOG */

typedef enum {
	VITALS_NORMAL = 0,
	VITALS_FGTRACKING,
	VITALS_TIME_BUCKET,
} vitals_type;

/*
 * Android log priority values, in ascending priority order.
 */
typedef enum android_log_priority {
	ANDROID_LOG_UNKNOWN = 0,
	ANDROID_LOG_DEFAULT,	/* only for SetMinPriority() */
	ANDROID_LOG_VERBOSE,
	ANDROID_LOG_DEBUG,
	ANDROID_LOG_INFO,
	ANDROID_LOG_WARN,
	ANDROID_LOG_ERROR,
	ANDROID_LOG_FATAL,
	ANDROID_LOG_SILENT,	/* only for SetMinPriority(); must be last */
} android_LogPriority;

int log_to_metrics(enum android_log_priority priority,
	const char *domain, char *logmsg);

int log_counter_to_vitals(enum android_log_priority priority,
	const char *domain, const char *program,
	const char *source, const char *key,
	long counter_value, const char *unit,
	const char *metadata, vitals_type type);
int log_timer_to_vitals(enum android_log_priority priority,
	const char *domain, const char *program,
	const char *source, const char *key,
	long timer_value, const char *unit, vitals_type type);

int log_counter_to_vitals_v2(enum android_log_priority priority,
	const char *group_id, const char *schema_id,
	const char *domain, const char *program,
	const char *source, const char *key,
	long counter_value, const char *unit,
	const char *metadata, vitals_type type,
	const char *dimensions, const char *annotations);

int log_timer_to_vitals_v2(enum android_log_priority priority,
	const char *group_id, const char *schema_id,
	const char *domain, const char *program,
	const char *source, const char *key,
	long timer_value, const char *unit, vitals_type type,
	const char *dimensions, const char *annotations);

#endif /* _LINUX_METRICSLOG_H */
