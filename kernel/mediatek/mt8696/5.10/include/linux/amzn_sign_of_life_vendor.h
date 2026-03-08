/*
 * amzn_sign_of_life_rtc.h
 *
 * Copyright 2022 Amazon Technologies, Inc. All Rights Reserved.
 *
 * The code contained herein is licensed under the GNU General Public
 * License Version 2.
 * You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __AMZN_SIGN_OF_LIFE_VENDOR_H
#define __AMZN_SIGN_OF_LIFE_VENDOR_H

/*
 * android boot reason charaters.
 */
#define POWER_KEY_BOOT		"=PowerKey"
#define USB_BOOT		"=usb"
#define WATCHDOG_SW		"=Watchdog"
#define WATCHDOG_HW		"=wdt_hw"
#define TWO_SEC_REBOOT		"=2sec_reboot"
#define POWER_LOSS		"=power_loss"
#define OVER_CUR_PRO		"=ocp"
#define KERNEL_PANIC		"=kernel_panic"


#define lcr_rtc_lock		mtk_rtc_acquire_lock
#define lcr_rtc_unlock		mtk_rtc_release_lock

#define lcr_rtc_write		mtk_rtc_write
#define lcr_rtc_read		mtk_rtc_read
#define lcr_rtc_write_trigger	mtk_rtc_write_trigger_out

#endif
