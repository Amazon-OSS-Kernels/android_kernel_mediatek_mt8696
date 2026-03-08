/*
 *  Copyright (c) 2016 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/completion.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/skbuff.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "btmtk_usb_main.h"
#include "btmtk_usb_fifo.h"

/*============================================================================*/
/* Local Configuration */
/*============================================================================*/
#define VERSION "6.0.23010601"
/*============================================================================*/
/* Function Prototype */
/*============================================================================*/
#define ___________________________________________________Function_Prototype
static int btmtk_usb_standby(void);
static void btmtk_usb_cap_init(void);
static int btmtk_usb_BT_init(void);
static void btmtk_usb_BT_exit(void);
static int btmtk_usb_send_assert_cmd(void);
static int btmtk_usb_send_assert_cmd_ctrl(void);
static int btmtk_usb_send_assert_cmd_bulk(void);
static int btmtk_usb_submit_intr_urb(void);
static int btmtk_usb_submit_bulk_in_urb(void);
static int btmtk_usb_send_hci_reset_cmd(void);
static int btmtk_usb_send_woble_suspend_cmd(void);
static void btmtk_usb_load_rom_patch_complete(const struct urb *urb);
static void btmtk_usb_bulk_in_complete(struct urb *urb);
static int btmtk_usb_submit_isoc_urb(void);
static void btmtk_usb_isoc_complete(struct urb *urb);
static void btmtk_usb_isoc_tx_complete(const struct urb *urb);
static void btmtk_usb_hci_snoop_print_to_log(void);
static int btmtk_usb_get_rom_patch_result(void);
static void btmtk_usb_tx_complete_meta(const struct urb *urb);
static int btmtk_usb_load_rom_patch(void);
static int btmtk_usb_send_wmt_reset_cmd(void);
static void btmtk_usb_early_suspend(void);
static void btmtk_usb_late_resume(void);
static void btmtk_usb_intr_complete(struct urb *urb);
static int btmtk_usb_send_wmt_cmd(const u8 *cmd, const int cmd_len, const u8 *event,
		const int event_len, u32 delay, u32 retry_count);
static int btmtk_usb_send_hci_cmd(const u8 *cmd, int cmd_len, const u8 *event, int event_len);
static int btmtk_usb_add_to_hci_log(const u8 *buf, int buf_len, int hci_type);
static int btmtk_usb_push_data_to_metabuffer(u8 *buf, int buf_len, int hci_type);
/** file_operations: stpbt/stpbtfwlog */
static int btmtk_usb_fops_open(struct inode *inode, struct file *file);
static int btmtk_usb_fops_close(struct inode *inode, struct file *file);
static ssize_t btmtk_usb_fops_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t btmtk_usb_fops_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int btmtk_usb_fops_poll(struct file *file, poll_table *wait);
static long btmtk_usb_fops_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int btmtk_usb_fops_openfwlog(struct inode *inode, struct file *file);
static int btmtk_usb_fops_closefwlog(struct inode *inode, struct file *file);
static ssize_t btmtk_usb_fops_readfwlog(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t btmtk_usb_fops_writefwlog(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int btmtk_usb_fops_pollfwlog(struct file *filp, poll_table *wait);
static long btmtk_usb_fops_unlocked_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg);

static int btmtk_usb_fops_sco_open(struct inode *inode, struct file *file);
static int btmtk_usb_fops_sco_close(struct inode *inode, struct file *file);
static ssize_t btmtk_usb_fops_sco_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t btmtk_usb_fops_sco_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int btmtk_usb_fops_sco_poll(struct file *file, poll_table *wait);
static long btmtk_usb_fops_sco_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/** read_write for proc */
static int btmtk_proc_show(struct seq_file *m, void *v);
static int btmtk_proc_open(struct inode *inode, struct  file *file);
static int btmtk_chip_reset_delay_proc_show(struct seq_file *m, void *v);
static int btmtk_chip_reset_delay_proc_open(struct inode *inode, struct  file *file);
static ssize_t btmtk_chip_reset_delay_proc_write(struct file *filp, const char __user *buffer,
                                    size_t count, loff_t *ppos);
static void btmtk_proc_create_new_entry(void);

#if SUPPORT_MT7662
static int btmtk_usb_load_rom_patch_7662(void);
static int btmtk_usb_io_read32_7662(u32 reg, u32 *val);
static int btmtk_usb_io_write32_7662(u32 reg, u32 val);
static int btmtk_usb_switch_iobase_7662(int base);
static int btmtk_usb_chk_crc_7662(u32 checksum_len);
static u16 btmtk_usb_get_crc_7662(void);
static u16 btmtk_usb_checksum16_7662(u8 *pData, int len);
static int btmtk_usb_send_hci_set_tx_power_cmd_7662(void);
static int btmtk_usb_send_hci_radio_on_cmd_7662(void);
static int btmtk_usb_send_hci_radio_off_cmd_7662(void);
static int btmtk_usb_send_hci_set_ce_cmd_7662(void);
static int btmtk_usb_send_hci_check_rom_patch_result_cmd_7662(void);
static int btmtk_usb_send_hci_low_power_cmd_7662(bool enable);
static void btmtk_usb_send_dummy_bulk_out_packet_7662(void);
#endif /* SUPPORT_MT7662 */

#if SUPPORT_MT7668
static int btmtk_usb_load_rom_patch_7668(void);
static int btmtk_usb_load_partial_rom_patch_7668(u32 patch_len, int offset);
static int btmtk_usb_io_read32_7668(u32 reg, u32 *val);
static int btmtk_usb_io_read32_7668(u32 reg, u32 *val);
static int btmtk_usb_send_hci_tci_set_sleep_cmd_7668(void);
static int btmtk_usb_send_wmt_power_on_cmd_7668(void);
static int btmtk_usb_send_wmt_power_off_cmd_7668(void);
static int btmtk_usb_reset_power_on(void);
#endif /* SUPPORT_MT7668 */

static int btmtk_usb_unify_woble_wake_up(void);
static int btmtk_usb_unify_woble_suspend(struct btmtk_usb_data *data);
static int btmtk_usb_send_unify_woble_suspend_cmd(void);
static int btmtk_usb_send_leave_woble_suspend_cmd(void);
static int btmtk_usb_send_get_vendor_cap(void);
static int btmtk_usb_L0_probe(struct usb_interface *intf, const struct usb_device_id *id);


typedef int (*usb_probe)(struct usb_interface *intf, const struct usb_device_id *id);


/*============================================================================*/
/* Global Variable */
/*============================================================================*/
#define _____________________________________________________Global_Variables
#define FWLOG_SPIN_LOCK(x)	spin_lock_irqsave(&g_data->fwlog_lock, x)
#define FWLOG_SPIN_UNLOCK(x)	spin_unlock_irqrestore(&g_data->fwlog_lock, x)
#define ISOC_SPIN_LOCK(x)	spin_lock_irqsave(&g_data->isoc_lock, x)
#define ISOC_SPIN_UNLOCK(x)	spin_unlock_irqrestore(&g_data->isoc_lock, x)

#define RETRY_TIMES 10

/* WoBLE LE scan parameters - 1056*0.625 = 660ms, 50*0.625 = 31.25ms */
#define BTMTK_WOBLE_SCAN_INTERVAL 1056
#define BTMTK_WOBLE_SCAN_WINDOW 50

/* stpbt character device for meta */
#ifdef FIXED_STPBT_MAJOR_DEV_ID
static int BT_major = FIXED_STPBT_MAJOR_DEV_ID;
static int BT_majorfwlog = FIXED_STPBT_MAJOR_DEV_ID + 1;
static int BT_major_sco = FIXED_STPBT_MAJOR_DEV_ID + 2;
#else
static int BT_major;
static int BT_majorfwlog;
static int BT_major_sco;
#endif /* FIXED_STPBT_MAJOR_DEV_ID */

static struct class *pBTClass;
static struct device *pBTDev;
static struct device *pBTDevfwlog;
static struct device *pBTDev_sco;

static struct cdev BT_cdev;
static struct cdev BT_cdevfwlog;
static struct cdev BT_cdev_sco;

static struct usb_driver btmtk_usb_driver;
static wait_queue_head_t inq;
static wait_queue_head_t fw_log_inq;
static wait_queue_head_t inq_isoc;
static struct btmtk_usb_data *g_data;
static int probe_counter;
static u8 need_reset_stack;
static u8 need_reset_stack_type;
static u8 need_reopen;
static int send_hw_err_event_count;

/* bluetooth KPI feautre, bperf */
static u8 btmtk_bluetooth_kpi;
static int leftACLSize;
static int btmtk_usb_state = BTMTK_USB_STATE_UNKNOWN;
static int btmtk_fops_state = BTMTK_FOPS_STATE_UNKNOWN;
static unsigned short sco_handle;	/* So far only support one SCO link */
static DECLARE_WAIT_QUEUE_HEAD(BT_wq);
static DECLARE_WAIT_QUEUE_HEAD(BT_sco_wq);
static DEFINE_MUTEX(btmtk_usb_state_mutex);
#define USB_MUTEX_LOCK()	mutex_lock(&btmtk_usb_state_mutex)
#define USB_MUTEX_UNLOCK()	mutex_unlock(&btmtk_usb_state_mutex)
static DEFINE_MUTEX(btmtk_fops_state_mutex);
#define FOPS_MUTEX_LOCK()	mutex_lock(&btmtk_fops_state_mutex)
#define FOPS_MUTEX_UNLOCK()	mutex_unlock(&btmtk_fops_state_mutex)

typedef void (*register_early_suspend) (void (*f) (void));
typedef void (*register_late_resume) (void (*f) (void));
register_early_suspend register_early_suspend_func;
register_late_resume register_late_resume_func;

/* Hci Snoop */
static u8 hci_cmd_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_cmd_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_cmd_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static u8 hci_event_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_event_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_event_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static u8 hci_acl_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_acl_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_acl_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static int hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
static int hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
static int hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;

static dev_t g_devIDfwlog;
static char fw_version_str[FW_VERSION_BUF_SIZE];
static struct proc_dir_entry *g_proc_dir;
static struct le_scan_parm_s host_le_scan;
static struct timer_list chip_reset_timer;
u8 btmtk_log_lvl = BTMTK_LOG_LEVEL_DEFAULT;
u32 btmtk_chip_reset_delay = RESET_PIN_SET_LOW_TIME; //delay in ms
u8 btmtk_fops_open_log = BTMTK_LOG_FULL_PRINT;

static atomic_t doing_reset = ATOMIC_INIT(0);

const struct file_operations BT_proc_fops = {
	.open = btmtk_proc_open,
	.read = seq_read,
	.release = single_release,
};

const struct file_operations BT_chip_reset_delay_proc_fops = {
	.open = btmtk_chip_reset_delay_proc_open,
	.read = seq_read,
	.write = btmtk_chip_reset_delay_proc_write,
	.release = single_release,
};

const struct file_operations BT_fops = {
	.open = btmtk_usb_fops_open,
	.release = btmtk_usb_fops_close,
	.read = btmtk_usb_fops_read,
	.write = btmtk_usb_fops_write,
	.poll = btmtk_usb_fops_poll,
	.unlocked_ioctl = btmtk_usb_fops_unlocked_ioctl,
};

const struct file_operations BT_fopsfwlog = {
	.open = btmtk_usb_fops_openfwlog,
	.release = btmtk_usb_fops_closefwlog,
	.read = btmtk_usb_fops_readfwlog,
	.write = btmtk_usb_fops_writefwlog,
	.poll = btmtk_usb_fops_pollfwlog,
	.unlocked_ioctl = btmtk_usb_fops_unlocked_ioctlfwlog
};

const struct file_operations BT_sco_fops = {
	.open = btmtk_usb_fops_sco_open,
	.release = btmtk_usb_fops_sco_close,
	.read = btmtk_usb_fops_sco_read,
	.write = btmtk_usb_fops_sco_write,
	.poll = btmtk_usb_fops_sco_poll,
	.unlocked_ioctl = btmtk_usb_fops_sco_unlocked_ioctl,
};

static inline void __fill_isoc_descriptor(struct urb *urb, int len, int mtu)
{
	int i, offset = 0;

	BTUSB_DBG("%s: len %d mtu %d", __func__, len, mtu);
	for (i = 0; i < BTUSB_MAX_ISOC_FRAMES && len >= mtu;
			i++, offset += mtu, len -= mtu) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = mtu;
	}

	if (len && i < BTUSB_MAX_ISOC_FRAMES) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = len;
		i++;
	}

	urb->number_of_packets = i;
}

/*============================================================================*/
/* Internal Functions */
/*============================================================================*/
#define ___________________________________________________Internal_Functions
static int btmtk_skb_enq_fwlog(void *src, u32 len, u8 type, struct sk_buff_head *queue)
{
	struct sk_buff *skb_tmp = NULL;
	ulong flags = 0;
	int retry = RETRY_TIMES;

	do {
		/* If we need hci type, len + 1 */
		skb_tmp = alloc_skb(type ? len + 1 : len, GFP_ATOMIC);
		if (skb_tmp != NULL) {
			break;
		} else if (retry <= 0) {
			BTUSB_ERR("%s: alloc_skb return 0, error", __func__);
			return -ENOMEM;
		}
		BTUSB_ERR("%s: alloc_skb return 0, error, retry = %d", __func__, retry);
	} while (retry-- > 0);

	if (type) {
		memcpy(&skb_tmp->data[0], &type, 1);
		memcpy(&skb_tmp->data[1], src, len);
		skb_tmp->len = len + 1;
	} else {
		memcpy(skb_tmp->data, src, len);
		skb_tmp->len = len;
	}

	FWLOG_SPIN_LOCK(flags);
	skb_queue_tail(queue, skb_tmp);
	FWLOG_SPIN_UNLOCK(flags);
	return 0;
}

static void btmtk_chip_rst_disc_timo_func(void *data)
{
	int ret;

	BTUSB_INFO("%s", __func__);

	/* workaround for after toggle reset pin but disconnect can't occur. */
	do {
		typedef void (*set_pin_state_func_ptr) (struct device * dev, int state);
		char *func_name = "btmtk_set_reset_pin_state";
		set_pin_state_func_ptr set_pin_state_func =
			(set_pin_state_func_ptr) kallsyms_lookup_name(func_name);

		if (set_pin_state_func) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 0);
			set_pin_state_func(&g_data->udev->dev, 0);
			mdelay(btmtk_chip_reset_delay);
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 1);
			set_pin_state_func(&g_data->udev->dev, 1);
		}  else
			BTUSB_INFO("%s: No Exported Func Found [%s]", __func__, func_name);
	} while (0);

	do {
		typedef int (*usb_logical_disconnect_ptr) (struct usb_device *udev);
		char *func_name = "btmtk_usb_logical_disconnect";
		usb_logical_disconnect_ptr usb_logical_disconnect_func =
			(usb_logical_disconnect_ptr) kallsyms_lookup_name(func_name);

		if (usb_logical_disconnect_func) {
			BTUSB_INFO("%s: Invoke %s", __func__, func_name);
			ret = usb_logical_disconnect_func(g_data->udev);
			BTUSB_INFO("%s: Invoke %s, ret %d", __func__, func_name, ret);
		} else
			BTUSB_INFO("%s: No Exported Func Found [%s]", __func__, func_name);
	} while (0);

	atomic_set(&doing_reset, 0);
}

static void btmtk_fw_dump_timo_func(void *data)
{
	BTUSB_INFO("%s", __func__);
	btmtk_usb_toggle_rst_pin();
}

static void btmtk_chip_reset_timo_func(void *data)
{
	BTUSB_INFO("%s", __func__);
	btmtk_skb_enq_fwlog(RESET_BT_TIMO, strlen(RESET_BT_TIMO), 0,
			&((struct btmtk_usb_data *)data)->fwlog_queue);
	wake_up_interruptible(&fw_log_inq);
}

static void btmtk_add_timer(struct timer_list *timer, void *fun, u16 sec, void *data)
{
	if (timer == NULL || sec == 0) {
		BTUSB_ERR("%s: Incorrect timer(%p, %d)", __func__, timer, sec);
		return;
	}
	BTUSB_INFO("%s", __func__);

	if (!timer_pending(timer)) {
		if (fun == NULL) {
			BTUSB_ERR("%s: Incorrect func for timer", __func__);
			return;
		}
		BTUSB_DBG("Add new timer");
#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
		timer->function = fun;
		timer->data = data ? (unsigned long)data : (unsigned long)NULL;
#else
		timer_setup(timer, fun, 0);
#endif
		timer->expires = jiffies + HZ * sec;
		add_timer(timer);
	} else {
		BTUSB_DBG("Modify the timer");
		mod_timer(timer, jiffies + HZ * sec);
	}
}

static void btmtk_del_timer(struct timer_list *timer)
{
	if (timer == NULL) {
		BTUSB_ERR("%s: Incorrect timer", __func__);
		return;
	}
	BTUSB_INFO("%s", __func__);
	del_timer_sync(timer);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0)
/**
 * kvmalloc_node - attempt to allocate physically contiguous memory, but upon
 * failure, fall back to non-contiguous (vmalloc) allocation.
 * @size: size of the request.
 * @flags: gfp mask for the allocation - must be compatible (superset) with GFP_KERNEL.
 * @node: numa node to allocate from
 *
 * Uses kmalloc to get the memory but if the allocation fails then falls back
 * to the vmalloc allocator. Use kvfree for freeing the memory.
 *
 * Reclaim modifiers - __GFP_NORETRY and __GFP_NOFAIL are not supported. __GFP_REPEAT
 * is supported only for large (>32kB) allocations, and it should be used only if
 * kmalloc is preferable to the vmalloc fallback, due to visible performance drawbacks.
 *
 * Any use of gfp flags outside of GFP_KERNEL should be consulted with mm people.
 */
static void *kvmalloc_node(size_t size, gfp_t flags, int node)
{
	gfp_t kmalloc_flags = flags;
	void *ret;

	/*
	 * vmalloc uses GFP_KERNEL for some internal allocations (e.g page tables)
	 * so the given set of flags has to be compatible.
	 */
	WARN_ON_ONCE((flags & GFP_KERNEL) != GFP_KERNEL);

	/*
	 * We want to attempt a large physically contiguous block first because
	 * it is less likely to fragment multiple larger blocks and therefore
	 * contribute to a long term fragmentation less than vmalloc fallback.
	 * However make sure that larger requests are not too disruptive - no
	 * OOM killer and no allocation failure warnings as we have a fallback.
	 */
	if (size > PAGE_SIZE) {
		kmalloc_flags |= __GFP_NOWARN;

		/*
		 * We have to override __GFP_REPEAT by __GFP_NORETRY for !costly
		 * requests because there is no other way to tell the allocator
		 * that we want to fail rather than retry endlessly.
		 */
		if (!(kmalloc_flags & __GFP_REPEAT) ||
				(size <= PAGE_SIZE << PAGE_ALLOC_COSTLY_ORDER))
			kmalloc_flags |= __GFP_NORETRY;
	}

	ret = kmalloc_node(size, kmalloc_flags, node);

	/*
	 * It doesn't really make sense to fallback to vmalloc for sub page
	 * requests
	 */
	if (ret || size <= PAGE_SIZE)
		return ret;

	return __vmalloc(size, flags, PAGE_KERNEL);
}

static inline void *kvmalloc(size_t size, gfp_t flags)
{
	return kvmalloc_node(size, flags, NUMA_NO_NODE);
}
static inline void *kvzalloc_node(size_t size, gfp_t flags, int node)
{
	return kvmalloc_node(size, flags | __GFP_ZERO, node);
}
static inline void *kvzalloc(size_t size, gfp_t flags)
{
	return kvmalloc(size, flags | __GFP_ZERO);
}

static inline void *kvmalloc_array(size_t n, size_t size, gfp_t flags)
{
	if (size != 0 && n > SIZE_MAX / size)
		return NULL;

	return kvmalloc(n * size, flags);
}
#endif

static int btmtk_usb_init_memory(void)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data is NULL !", __func__);
		return -1;
	}

	g_data->chip_id = 0;
	g_data->suspend_count = 0;

	if (g_data->metabuffer)
		memset(g_data->metabuffer->buffer, 0, META_BUFFER_SIZE);

	if (g_data->io_buf)
		memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);

	if (g_data->rom_patch_bin_file_name)
		memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);

	if (g_data->i_buf)
		memset(g_data->i_buf, 0, BUFFER_SIZE);

	if (g_data->o_buf)
		memset(g_data->o_buf, 0, BUFFER_SIZE);

	if (g_data->i_fwlog_buf)
		memset(g_data->i_fwlog_buf, 0, HCI_MAX_COMMAND_BUF_SIZE);

	if (g_data->o_fwlog_buf)
		memset(g_data->o_fwlog_buf, 0, HCI_MAX_COMMAND_SIZE);

	if (g_data->o_sco_buf)
		memset(g_data->o_sco_buf, 0, BUFFER_SIZE);

	if (g_data->o_usb_buf)
		memset(g_data->o_usb_buf, 0, HCI_MAX_COMMAND_SIZE);
	BTUSB_INFO("%s: Success", __func__);
	return 1;
}

static int btmtk_usb_allocate_memory(void)
{
	if (g_data == NULL) {
		g_data = kzalloc(sizeof(*g_data), GFP_KERNEL);
		if (!g_data) {
			BTUSB_ERR("%s: alloc memory fail (g_data)", __func__);
			return -1;
		}
	}

	if (g_data->metabuffer == NULL) {
		g_data->metabuffer = kvmalloc(sizeof(struct ring_buffer_struct), GFP_KERNEL);
		if (!g_data->metabuffer) {
			BTUSB_ERR("%s: alloc memory fail (g_data->metabuffer)",
				__func__);
			return -1;
		}
	}

	if (g_data->io_buf == NULL) {
		g_data->io_buf = kzalloc(USB_IO_BUF_SIZE, GFP_KERNEL);
		if (!g_data->io_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->io_buf)", __func__);
			return -1;
		}
	}

	if (g_data->rom_patch_bin_file_name == NULL) {
		g_data->rom_patch_bin_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!g_data->rom_patch_bin_file_name) {
			BTUSB_ERR("%s: alloc memory fail (g_data->rom_patch_bin_file_name)", __func__);
			return -1;
		}
	}

	if (g_data->woble_setting_file_name == NULL) {
		g_data->woble_setting_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!g_data->woble_setting_file_name) {
			BTUSB_ERR("%s: alloc memory fail (g_data->woble_setting_file_name)", __func__);
			return -1;
		}
	}

	if (g_data->i_buf == NULL) {
		g_data->i_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!g_data->i_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->i_buf)", __func__);
			return -1;
		}
	}

	if (g_data->o_buf == NULL) {
		g_data->o_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!g_data->o_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->o_buf)", __func__);
			return -1;
		}
	}

	if (g_data->bt_fifo == NULL) {
		g_data->bt_fifo = btmtk_fifo_init();
		if (!g_data->bt_fifo) {
			BTUSB_ERR("%s: alloc memory fail (g_data->bt_fifo)", __func__);
			return -1;
		}
	}

	if (g_data->i_fwlog_buf == NULL) {
		g_data->i_fwlog_buf = kzalloc(HCI_MAX_COMMAND_BUF_SIZE, GFP_KERNEL);
		if (!g_data->i_fwlog_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->i_fwlog_buf)", __func__);
			return -1;
		}
	}

	if (g_data->o_fwlog_buf == NULL) {
		g_data->o_fwlog_buf = kzalloc(HCI_MAX_COMMAND_SIZE, GFP_KERNEL);
		if (!g_data->o_fwlog_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->o_fwlog_buf)", __func__);
			return -1;
		}
	}

	if (g_data->o_sco_buf == NULL) {
		g_data->o_sco_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (g_data->o_sco_buf == NULL) {
			BTUSB_ERR("%s: alloc memory fail (g_data->o_sco_buf)",
				__func__);
			return -1;
		}
	}

	if (g_data->o_usb_buf == NULL) {
		g_data->o_usb_buf = kzalloc(HCI_MAX_COMMAND_SIZE, GFP_KERNEL);
		if (!g_data->o_usb_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->o_usb_buf)", __func__);
			return -1;
		}
}
	BTUSB_INFO("%s: Success", __func__);
	return 1;
}

static void btmtk_usb_free_memory(void)
{
	if (!g_data) {
		BTUSB_ERR("%s: g_data is NULL!", __func__);
		return;
	}

	kvfree(g_data->metabuffer);
	g_data->metabuffer = NULL;

	kfree(g_data->i_buf);
	g_data->i_buf = NULL;

	kfree(g_data->o_buf);
	g_data->o_buf = NULL;

	kfree(g_data->i_fwlog_buf);
	g_data->i_fwlog_buf = NULL;

	kfree(g_data->o_fwlog_buf);
	g_data->o_fwlog_buf = NULL;

	kfree(g_data->o_sco_buf);
	g_data->o_sco_buf = NULL;

	kfree(g_data->rom_patch_bin_file_name);
	g_data->rom_patch_bin_file_name = NULL;

	kfree(g_data->woble_setting_file_name);
	g_data->woble_setting_file_name = NULL;

	kfree(g_data->io_buf);
	g_data->io_buf = NULL;

	kvfree(g_data->rom_patch_image);
	g_data->rom_patch_image = NULL;

	kfree(g_data->o_usb_buf);
	g_data->o_usb_buf = NULL;

	btmtk_fifo_init();
	g_data->bt_fifo = NULL;

	/* free queues */
	skb_queue_purge(&g_data->fwlog_queue);
	skb_queue_purge(&g_data->isoc_in_queue);

	kfree(g_data);
	g_data = NULL;

	BTUSB_INFO("%s: Success", __func__);
}

static int btmtk_usb_get_state(void)
{
	return btmtk_usb_state;
}

static void btmtk_usb_set_state(int new_state)
{
	static const char * const state_msg[] = {
		"UNKNOWN", "INIT", "DISCONNECT", "PROBE", "WORKING", "EARLY_SUSPEND",
		"SUSPEND", "RESUME", "LATE_RESUME", "FW_DUMP", "SUSPEND_DISCONNECT",
		"SUSPEND_PROBE", "SUSPEND_FW_DUMP", "RESUME_DISCONNECT", "RESUME_PROBE",
		"RESUME_FW_DUMP",
	};

	BTUSB_INFO("%s: %s(%d) -> %s(%d)", __func__, state_msg[btmtk_usb_state],
			btmtk_usb_state, state_msg[new_state], new_state);
	btmtk_usb_state = new_state;
}

static int btmtk_fops_get_state(void)
{
	return btmtk_fops_state;
}

static void btmtk_fops_set_state(int new_state)
{
	static const char * const fstate_msg[] = {"UNKNOWN", "INIT", "OPENED", "CLOSING", "CLOSED"};

	BTUSB_INFO("%s: FOPS_%s(%d) -> FOPS_%s(%d)", __func__, fstate_msg[btmtk_fops_state],
			btmtk_fops_state, fstate_msg[new_state], new_state);
	btmtk_fops_state = new_state;
}

void btmtk_do_gettimeofday(struct timeval *tv)
{
#if (KERNEL_VERSION(4, 19, 85) > LINUX_VERSION_CODE)
	do_gettimeofday(tv);
#else
	struct timespec64 ts;
	ktime_get_real_ts64(&ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec/1000;
#endif
}


static unsigned int btmtk_usb_hci_snoop_get_microseconds(void)
{
	struct timeval now;

	btmtk_do_gettimeofday(&now);
	return now.tv_sec * 1000000 + now.tv_usec;
}

static void btmtk_usb_save_le_scan_parm(u8 type, u16 interval, u16 win, u8 addr_type, u8 policy)
{
	// Get from host
	BTUSB_INFO("%s: type:%02X, interval:%04X, window:%04X, addr_type:%02X, policy:%02X",
			__func__, type, interval, win, addr_type, policy);
	host_le_scan.type = type;
	host_le_scan.interval = interval;
	host_le_scan.window = win;
	host_le_scan.own_addr_type = addr_type;
	host_le_scan.filter_policy = policy;
}

static void btmtk_usb_save_le_scan_state(u8 enable, u8 duplicate)
{
	// Get from host
	BTUSB_INFO("%s: enable:%02X, filter dup:%02X", __func__, enable, duplicate);
	host_le_scan.enable = enable;
	host_le_scan.filter_dup = duplicate;
}

static int btmtk_usb_dispatch_data_bluetooth_kpi(u8 *buf, int len, u8 type)
{
	static u8 fwlog_blocking_warn;
	int ret = 0;

	if (btmtk_bluetooth_kpi &&
		skb_queue_len(&g_data->fwlog_queue) < FWLOG_BLUETOOTH_KPI_QUEUE_COUNT) {
		/* sent event to queue, picus tool will log it for bluetooth KPI feature */
		if (btmtk_skb_enq_fwlog(buf, len, type, &g_data->fwlog_queue) == 0) {
			wake_up_interruptible(&fw_log_inq);
			fwlog_blocking_warn = 0;
		}
	} else {
		if (fwlog_blocking_warn == 0) {
			fwlog_blocking_warn = 1;
			BTUSB_WARN("btmtk_usb fwlog queue size is full(bluetooth_kpi)");
		}
	}
	return ret;
}

static int btmtk_usb_dispatch_event(u8 *buf, int len)
{
	static u8 fwlog_blocking_warn;
	int ret = 0;

	/* picus or syslog */
	if (buf[0] == 0xFF && buf[1] + 2 == len && (buf[2] == 0x50 || buf[2] == 0x51)) {
		if (skb_queue_len(&g_data->fwlog_queue) < FWLOG_QUEUE_COUNT) {
			/* sent coredump data to queue, picus will log it */
			if (btmtk_skb_enq_fwlog(buf, len, 0, &g_data->fwlog_queue) == 0) {
				wake_up_interruptible(&fw_log_inq);
				fwlog_blocking_warn = 0;
			}
		} else {
			if (fwlog_blocking_warn == 0) {
				fwlog_blocking_warn = 1;
				BTUSB_WARN("btmtk_usb fwlog queue size is full(picus)");
			}
		}

#define HCE_WAKEUP_DEBUG_EVENT 0xE7
	/* Wakeup debugging event */
	} else if (buf[0] == HCE_WAKEUP_DEBUG_EVENT && buf[1] == 0x10) {
		BTUSB_INFO("passSCAN:0x%02X, enterAPCF:0x%02X, passAPCF:0x%02X, toggleGPIO:0x%02X",
				*(u32 *)(buf + 2), *(u32 *)(buf + 6),
				*(u32 *)(buf + 10), *(u32 *)(buf + 14));
		btmtk_usb_add_to_hci_log(buf, len, HCI_EVENT_PKT);

	/* Unexpected event */
	} else {
		btmtk_usb_push_data_to_metabuffer(buf, len, HCI_EVENT_PKT);
		ret = -1;
	}
	return ret;
}

/**
 * Do not remove this btmtk_usb_le_set_scan_parm() function, as it save and restores
 * BLE scan parameters for WoBLE. Even if the woble_setting.bin can configure the
 * BLE scan parameters, on existing Amazon Lab126 projects, we have not used the
 * woble_setting.bin to config these parameters. Even if we configure some products
 * to use woble_setting.bin for BLE scan parameters configuration, the values in
 * woble_setting.bin will be effective after this function is run, woble_setting.bin
 * has higher priority for these settings.
 */
static void btmtk_usb_le_set_scan_parm(bool restore)
{
	int ret = -1;
	u8 disa_scan[] = { 0x0C, 0x20, 0x02, 0x00, 0x01 };
	u8 disa_scan_event[] = { 0x0E, 0x04, 0x01, 0x0C, 0x20, 0x00 };
	u8 scan_parm[10] = { 0x0B, 0x20, 0x07, };
	u8 scan_parm_event[] = { 0x0E, 0x04, 0x01, 0x0B, 0x20, 0x00 };

	if (host_le_scan.interval == 0 || host_le_scan.window == 0) {
		BTUSB_INFO("%s: skip this because of no le scan parameters", __func__);
		return;
	}

	BTUSB_INFO("%s: %s[%04X/%04X]", __func__, restore == TRUE ? (host_le_scan.type ? "ACTIVE" : "PASSIVE")
			: "passive", host_le_scan.interval, host_le_scan.window);
	if (host_le_scan.enable == 1) {
		/* if LE scan is enabled, disable it firstly or FW wouldn't accept */
		ret = btmtk_usb_send_hci_cmd(disa_scan, sizeof(disa_scan), disa_scan_event,
				sizeof(disa_scan_event));
		if (ret <= 0)
			/* if controller already disabled the state would be COMMAND_DISALLOWED(0x0C) */
			BTUSB_ERR("%s: Disable scan fail %d, could disabled already", __func__, ret);
	}

	/** Set LE scan parameters.,
	 *  Because scan_resp is necessary if RC doesen't response it would incur WoBLE fail.
	 *  If controller is scanning, the state would be COMMAND_DISALLOWED(0x0C)
	 */
	scan_parm[3] = (restore == TRUE ? host_le_scan.type : 0x00);	/* 0: passive., 1: active */
	/**
	 * The host_le_scan can be initiated from host when it is doing device discovery,
	 * e.g, during OOBE, or during discovering/pairing of devices in Settings, in such
	 * cases the host_le_scan can be FULL BLE scan (window==interval). However, for
	 * WoBLE to work in low power mode, MT7668 should not use this Full scan, otherwise
	 * it may cause power consumptionm regulation issues for the whole device because
	 * full BLE scan will need more power than partial BLE scan.
	 *
	 * We could save the host requested LE scan parameters in host_le_scan before suspend,
	 * and set the default low power LE scan when entering STR (into MT7668), and restore
	 * the host requested LE scan when resuming from STR.
	 *
	 * The 'restore == TRUE' here will make sure it always restores the original host
	 * initiated BLE scanparameters, no matter if it was set to default parameters here,
	 * or it was overriden from woble_setting.bin.
	 */
	*(u16 *)(scan_parm + 4) = (restore == TRUE ? host_le_scan.interval : BTMTK_WOBLE_SCAN_INTERVAL);
	*(u16 *)(scan_parm + 6) = (restore == TRUE ? host_le_scan.window : BTMTK_WOBLE_SCAN_WINDOW);
	scan_parm[8] = host_le_scan.own_addr_type;
	scan_parm[9] = host_le_scan.filter_policy;
	BTUSB_DBG_RAW(scan_parm, sizeof(scan_parm), "scan_parm[%d]:", (int)sizeof(scan_parm));
	ret = btmtk_usb_send_hci_cmd(scan_parm, sizeof(scan_parm), scan_parm_event, sizeof(scan_parm_event));
	if (ret <= 0)
		BTUSB_ERR("%s: Set scan parm fail %d", __func__, ret);

	if (host_le_scan.enable == 1) {
		/* restore LE scan state, reuse disa_scan[] */
		disa_scan[3] = host_le_scan.enable;
		disa_scan[4] = host_le_scan.filter_dup;
		ret = btmtk_usb_send_hci_cmd(disa_scan, sizeof(disa_scan), disa_scan_event,
				sizeof(disa_scan_event));
		if (ret <= 0)
			BTUSB_ERR("%s: Enable scan fail %d", __func__, ret);
	}
}

static void btmtk_usb_hci_snoop_init(void)
{
	int i;

	hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	for (i = 0; i < HCI_SNOOP_ENTRY_NUM; i++) {
		hci_cmd_snoop_len[i] = 0;
		hci_event_snoop_len[i] = 0;
		hci_acl_snoop_len[i] = 0;
	}
}

static void btmtk_usb_hci_snoop_save_cmd(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_cmd_snoop_len[hci_cmd_snoop_index] = copy_len & 0xff;
		memset(hci_cmd_snoop_buf[hci_cmd_snoop_index], 0, HCI_SNOOP_BUF_SIZE);
		memcpy(hci_cmd_snoop_buf[hci_cmd_snoop_index], buf, copy_len & 0xff);
		hci_cmd_snoop_timestamp[hci_cmd_snoop_index] = btmtk_usb_hci_snoop_get_microseconds();

		hci_cmd_snoop_index--;
		if (hci_cmd_snoop_index < 0)
			hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}

static void btmtk_usb_hci_snoop_save_event(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_event_snoop_len[hci_event_snoop_index] = copy_len;
		memset(hci_event_snoop_buf[hci_event_snoop_index], 0,
			HCI_SNOOP_BUF_SIZE);
		memcpy(hci_event_snoop_buf[hci_event_snoop_index], buf, copy_len);
		hci_event_snoop_timestamp[hci_event_snoop_index] = btmtk_usb_hci_snoop_get_microseconds();

		hci_event_snoop_index--;
		if (hci_event_snoop_index < 0)
			hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}

static void btmtk_usb_hci_snoop_save_acl(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_acl_snoop_len[hci_acl_snoop_index] = copy_len & 0xff;
		memset(hci_acl_snoop_buf[hci_acl_snoop_index], 0, HCI_SNOOP_BUF_SIZE);
		memcpy(hci_acl_snoop_buf[hci_acl_snoop_index], buf, copy_len & 0xff);
		hci_acl_snoop_timestamp[hci_acl_snoop_index] = btmtk_usb_hci_snoop_get_microseconds();

		hci_acl_snoop_index--;
		if (hci_acl_snoop_index < 0)
			hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}

static void btmtk_usb_hci_snoop_print_to_log(void)
{
	int counter, index, j;

	BTUSB_INFO("HCI Command Dump");
	BTUSB_INFO("  index(len)(timestamp:us) :HCI Command");
	index = hci_cmd_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_cmd_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
							hci_cmd_snoop_len[index],
							hci_cmd_snoop_timestamp[index]);
			for (j = 0; j < hci_cmd_snoop_len[index]; j++)
				pr_cont("%02X ", hci_cmd_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}

	BTUSB_INFO("HCI Event Dump");
	BTUSB_INFO("  index(len)(timestamp:us) :HCI Event");
	index = hci_event_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_event_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
							hci_event_snoop_len[index],
							hci_event_snoop_timestamp[index]);
			for (j = 0; j < hci_event_snoop_len[index]; j++)
				pr_cont("%02X ", hci_event_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}

	BTUSB_INFO("HCI ACL Dump");
	BTUSB_INFO("  index(len)(timestamp:us) :ACL");
	index = hci_acl_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_acl_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
							hci_acl_snoop_len[index],
							hci_acl_snoop_timestamp[index]);
			for (j = 0; j < hci_acl_snoop_len[index]; j++)
				pr_cont("%02X ", hci_acl_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}
}

static int btmtk_usb_send_assert_cmd(void)
{
	int ret = 0;
	int state = btmtk_usb_get_state();

	if (state == BTMTK_USB_STATE_FW_DUMP || state == BTMTK_USB_STATE_SUSPEND_FW_DUMP
			|| state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
		BTUSB_WARN("%s: FW dumping already!!!", __func__);
		return ret;
	}

	BTUSB_INFO("%s: send assert cmd, dongle state %d", __func__, g_data->is_mt7668_dongle_state);

	ret = btmtk_usb_send_assert_cmd_ctrl();
	if (ret < 0) {
		ret = btmtk_usb_send_assert_cmd_bulk();
		if (ret < 0) {
			BTUSB_ERR("%s: send assert cmd fail, tigger hw reset only", __func__);
			btmtk_usb_toggle_rst_pin();
		} else {
			if (g_data->bulk_urb_submitted == 0) {
				ret = btmtk_usb_submit_bulk_in_urb();
				if (ret < 0) {
					BTUSB_ERR("%s: Submit bulk-in fail, tigger hw reset", __func__);
					btmtk_usb_toggle_rst_pin();
				}
			}
		}
	} else {
		if (g_data->bulk_urb_submitted == 0) {
			ret = btmtk_usb_submit_bulk_in_urb();
			if (ret < 0) {
				BTUSB_ERR("%s: Submit bulk in fail, tigger hw reset", __func__);
				btmtk_usb_toggle_rst_pin();
			}
		}
	}
	return ret;
}

static void btmtk_usb_L0_hook_new_probe(usb_probe pFn_Probe)
{
	if (pFn_Probe == NULL) {
		BTUSB_ERR("%s, new probe is NULL", __func__);
		return;
	}
	btmtk_usb_driver.probe = pFn_Probe;
}

void btmtk_usb_toggle_rst_pin(void)
{
	int cur;

	BTUSB_INFO("%s: begin", __func__);
	if (timer_pending(&g_data->fw_dump_timer)) {
		btmtk_del_timer(&g_data->fw_dump_timer);
	}
	/* Avoid multiple tasks try to toggle reset pin */
	if (timer_pending(&chip_reset_timer)) {
		BTUSB_INFO("%s: In diag reset, skip this request", __func__);
		return;
	}

	cur = atomic_cmpxchg(&doing_reset, 0, 1);
	if (cur == 1) {
		BTUSB_INFO("%s: reset in progress, return", __func__);
		return;
	}

	/* No Need print HCI records since trigger by WiFi */
	btmtk_usb_hci_snoop_print_to_log();

	btmtk_usb_L0_hook_new_probe(btmtk_usb_L0_probe);

	if (need_reset_stack_type == HW_ERR_NONE)
		need_reset_stack_type = HW_ERR_CODE_BT_DRIVER;

	/* start timer to monitor disconnect event happen or not*/
	btmtk_add_timer(&g_data->chip_rst_disc_timer, btmtk_chip_rst_disc_timo_func,
			RESET_TIMO, g_data);

	/* First interface - void btmtk_toggle_reset_pin(struct device * dev, int reset) */
	do {
		typedef void (*toggle_pin_func_ptr) (struct device * dev, int reset);
		char *func_name = "btmtk_toggle_reset_pin";
		toggle_pin_func_ptr toggle_pin_func =
			(toggle_pin_func_ptr) kallsyms_lookup_name(func_name);

		if (toggle_pin_func) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 1);
			toggle_pin_func(&g_data->udev->dev, 1);
		} else
			BTUSB_INFO("%s: No Exported Func Found [%s]", __func__, func_name);
	} while (0);

	/* Second interface - void btmtk_set_reset_pin_state(struct device * dev, int state) */
	do {
		typedef void (*set_pin_state_func_ptr) (struct device * dev, int state);
		char *func_name = "btmtk_set_reset_pin_state";
		set_pin_state_func_ptr set_pin_state_func =
			(set_pin_state_func_ptr) kallsyms_lookup_name(func_name);

		if (set_pin_state_func) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 0);
			set_pin_state_func(&g_data->udev->dev, 0);
			mdelay(btmtk_chip_reset_delay);
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 1);
			set_pin_state_func(&g_data->udev->dev, 1);
		}  else
			BTUSB_INFO("%s: No Exported Func Found [%s]", __func__, func_name);
	} while (0);

	BTUSB_INFO("%s: end", __func__);
}
EXPORT_SYMBOL(btmtk_usb_toggle_rst_pin);

static inline void btmtk_usb_lock_unsleepable_lock(struct OSAL_UNSLEEPABLE_LOCK *pUSL)
{
	spin_lock_irqsave(&(pUSL->lock), pUSL->flag);
}

static inline void btmtk_usb_unlock_unsleepable_lock(struct OSAL_UNSLEEPABLE_LOCK *pUSL)
{
	spin_unlock_irqrestore(&(pUSL->lock), pUSL->flag);
}

static void btmtk_usb_woble_free_setting_struct(struct woble_setting_struct *woble_struct, int count)
{
	int i = 0;

	for (i = 0; i < count; i++) {
		if (woble_struct[i].content) {
			BTUSB_INFO("%s:kfree %d", __func__, i);
			kfree(woble_struct[i].content);
			woble_struct[i].content = NULL;
			woble_struct[i].length = 0;
		} else
			woble_struct[i].length = 0;
	}
}

static void btmtk_usb_woble_free_setting(void)
{
	BTUSB_INFO("%s", __func__);
	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data == NULL", __func__);
		return;
	}

	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_apcf, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_apcf_fill_mac, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_apcf_fill_mac_location, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_radio_off, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_radio_off_status_event, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_radio_off_comp_event, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_radio_on, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_radio_on_status_event, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_radio_on_comp_event, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_apcf_resume, WOBLE_SETTING_COUNT);

	g_data->woble_setting_len = 0;
}

static int btmtk_usb_set_radio_off_cmd(struct woble_setting_struct *radiooff,
		u8 attr, void *value, u8 pos, u8 len)
{
	u8 cmd_len = 0;
	u8 attr_len = 0;
	int i = 5, j = 0; /* 5 is a start of attr group */

	if (radiooff == NULL || value == NULL || len == 0) {
		BTUSB_ERR("%s: Incorrect parameters - %s %s %s", __func__,
				radiooff ? "" : "radiooff",
				value ? "" : "value", len ? "" : "len");
		return -EINVAL;
	}
	cmd_len = radiooff->content[2];
	BTUSB_INFO_RAW(radiooff->content, radiooff->length, "Origin RADIOOFF: ");

	do {
		attr_len = radiooff->content[i];
		if (i < radiooff->length - 1 && radiooff->content[i + 1] == attr) {
			if (attr == WOBX_TYPE_KEYCODE_MAPPING) {
				if (len != attr_len - 1) {
					/* could rewrite length, this attr should local in the end */
					u8 *tmp = radiooff->content;
					int new_len = 3 + cmd_len - attr_len + (len + 1);

					BTUSB_DBG("%s: Re-allocate size(%d/%d) for mod", __func__,
							radiooff->length, new_len);
					radiooff->content = kzalloc(new_len, GFP_KERNEL);
					memcpy(radiooff->content, tmp, radiooff->length);
					kfree(tmp);
					tmp = NULL;
					radiooff->length = new_len;
					/* Update cmd len & attr len both are necessary */
					radiooff->content[i] = len + 1;		/* attr len */
					radiooff->content[2] = new_len - 3;	/* cmd len */
				}
			}
			/* assign value */
			for (j = 0; j < len; j++)
				radiooff->content[i + 1 + pos + j] = *((u8 *)value + j);
			break;
		}
		i += (attr_len + 1);
	} while (i + 1 < cmd_len);

	if (i + 1 < cmd_len) {
		BTUSB_INFO_RAW(radiooff->content, radiooff->length, "Update RADIOOFF: ");
		return 0;
	}

	if (attr == WOBX_TYPE_IR) {
		BTUSB_ERR("%s: Please add WOBX_TYPE_IR in woble_setting.bin first", __func__);
		return -ENOENT;
	} else if (attr == WOBX_TYPE_KEYCODE_MAPPING) {
		u8 *tmp = radiooff->content;
		int new_len = 3 + cmd_len + 2 + len;
		int old_len = radiooff->length;

		BTUSB_DBG("%s: Re-allocate size(%d/%d) for mod", __func__,
				radiooff->length, new_len);
		radiooff->content = kzalloc(new_len, GFP_KERNEL);
		memcpy(radiooff->content, tmp, radiooff->length);
		kfree(tmp);
		tmp = NULL;
		radiooff->length = new_len;
		/* Update cmd len is necessary */
		radiooff->content[2] = new_len - 3;	/* cmd len */

		radiooff->content[old_len] = len + 1;	/* attr len: attr + content */
		radiooff->content[old_len + 1] = attr;
		for (j = 0; j < len; j++)
			radiooff->content[old_len + 2 + j] = *((u8 *)value + j);
	}
	BTUSB_INFO_RAW(radiooff->content, radiooff->length, "Append RADIOOFF: ");
	return 0;
}

static int btmtk_usb_load_woble_ir_setting(struct woble_setting_struct *radiooff,
		u8 *setting)
{
#define CHAR42HEX_SIZE	10
#define CHAR2HEX_SIZE	4
#define CHAR2DEC_SIZE	2
	int i = 0;
	char *head = NULL, *tail = NULL;
	u16 mark = 0x0000;
	char ir_proto[] = IR_PROTOCOL"00:";	/* pattern */
	char ir_km_g[] = IR_KEYMAP_G"00:";	/* pattern */
	char ir_km[] = IR_KEYMAP"00:";		/* pattern */
	char ir[CHAR2HEX_SIZE + 1] = {"\n"};	/* 1 is for '\n' */
	char group[3] = {"\n"};			/* DEC: 2, 1 is for '\n' */
	struct woble_ir_km_char_s map[MAX_IRKMG] = {{{'\n'}, {'\n'}}};
	unsigned long ir_value = 0;
	unsigned long group_value = 0;
	struct woble_ir_km_value_s map_value[MAX_IRKMG] = {{{0}, {0}}};

	if (radiooff == NULL || setting == NULL) {
		BTUSB_ERR("%s: Incorrect parameters - %s %s", __func__,
				radiooff ? "" : "radiooff", setting ? "" : "setting");
		return -EINVAL;
	}

	/* confirm how many woble setting groups */
	for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
		if (radiooff[i].content != NULL && radiooff[i].length != 0)
			mark |= (1 << i);
	}
	BTUSB_DBG("%s: mark: 0x%04X", __func__, mark);

	for (i = 0; ((mark >> i) & 0x0001) && i < WOBLE_SETTING_COUNT; i++) {
		BTUSB_DBG("%s: The following is radiooff[%d]", __func__, i);
		/* IR protocol parsing */
		snprintf(ir_proto, sizeof(ir_proto), "%s%02d:", IR_PROTOCOL, i);
		head = strstr(setting, ir_proto);
		if (head) {
			head = strstr(head, "0x");	/* should be 0xAA */
			if (head) {
				tail = strstr(head, ",");
				if (!tail || tail - head != CHAR2HEX_SIZE) {
					BTUSB_ERR("%s: Has incorrect format: %p %p %s",
							__func__, tail, head, head);
					return -EINVAL;
				}
				memcpy(ir, head, CHAR2HEX_SIZE);
				if (kstrtoul(ir, 0, &ir_value) == 0) {
					BTUSB_INFO("%s: ir protocol is 0x%02X", __func__, (int)ir_value);
					/* update radio off command */
					btmtk_usb_set_radio_off_cmd(&radiooff[i], WOBX_TYPE_IR,
							(void *)&ir_value, 2, 1);
				} else {
					BTUSB_ERR("%s: %s convert fail", __func__, ir);
					return -EPERM;
				}
			} else {
				BTUSB_ERR("%s: %s has incorrect format",
						__func__, ir_proto);
				return -EINVAL;
			}
		} else {
			BTUSB_INFO("%s: No %s setting", __func__, ir_proto);
			return -ENOENT;
		}

		/* Keymap parsing */
		snprintf(ir_km_g, sizeof(ir_km_g), "%s%02d", IR_KEYMAP_G, i);
		head = strstr(setting, ir_km_g);
		if (head) {
			head = strstr(head, ":");
			if (head) {
				tail = strstr(head, ",");
				if (!tail || tail - (head + 1) != CHAR2DEC_SIZE) {
					BTUSB_ERR("%s: %s has incorrect format",
							__func__, head + 1);
					return -EINVAL;
				}
				memcpy(group, head + 1, CHAR2DEC_SIZE);
				BTUSB_DBG("%s: group: %s", __func__, group);
				if (kstrtoul(group, 0, &group_value) == 0) {
					int j = 0;

					if (group_value > MAX_IRKMG) {
						BTUSB_WARN("%s: Key mapping can't over than %d groups(%d), ignore surplus",
								__func__, MAX_IRKMG, (unsigned int)group_value);
						group_value = MAX_IRKMG; /* FW limitation */
					}
					for (j = 1; j <= group_value; j++) {
						snprintf(ir_km, sizeof(ir_km), "%s%02d:", IR_KEYMAP, j);
						head = strstr(setting, ir_km);
						if (head) {
							/* should be 0x027D5FA0 for NEC */
							head = strstr(head, "0x");
							if (head) {
								tail = strstr(head, ",");
								if (!tail || tail - head != CHAR42HEX_SIZE) {
									BTUSB_ERR("%s: %s has incorrect format",
											__func__, head);
									break;
								}
								memcpy(map[j - 1].nec, head, CHAR42HEX_SIZE);
								BTUSB_DBG("%s: %d:nec: %s", __func__, j, map[j - 1].nec);
								if (kstrtoul(map[j - 1].nec, 0, (unsigned long *)map_value[j - 1].nec) != 0) {
									BTUSB_ERR("%s: %s convert fail", __func__, map[j - 1].nec);
									break;
								}
							} else {
								BTUSB_ERR("%s: Incorrect format - %s",
										__func__, head);
								break;
							}
							/* should be 0x36B90000 for RC5 */
							head = strstr(tail, "0x");
							if (head) {
								tail = strstr(head, ",");
								if (!tail || tail - head != CHAR42HEX_SIZE) {
									BTUSB_ERR("%s: %s has incorrect format",
											__func__, head);
									break;
								}
								memcpy(map[j - 1].rc5, head, CHAR42HEX_SIZE);
								BTUSB_DBG("%s: %d:rc5: %s", __func__, j, map[j - 1].rc5);
								if (kstrtoul(map[j - 1].rc5, 0, (unsigned long *)map_value[j - 1].rc5) != 0) {
									BTUSB_ERR("%s: %s convert fail", __func__, map[j - 1].rc5);
									break;
								}
							} else {
								BTUSB_ERR("%s: Incorrect format - %s",
										__func__, head);
								break;
							}
						} else {
							BTUSB_ERR("%s: Can't find keymap[%d]", __func__, j);
							break;
						}
					}
					btmtk_usb_set_radio_off_cmd(&radiooff[i], WOBX_TYPE_KEYCODE_MAPPING,
							(void *)map_value, 1, group_value * 8);

				} else
					BTUSB_ERR("%s: %s convert fail", __func__, group);
			} else
				BTUSB_ERR("%s: Incorrect format", __func__);
		} else
			BTUSB_DBG("%s: No keymap[%d]", __func__, i);
	}
	return 0;
}

static int btmtk_usb_load_woble_block_setting(char *block_name,
	struct woble_setting_struct *save_content,
	int save_content_count, u8 *searchconetnt)
{
	int ret = 0;
	int i = 0;
	long parsing_result = 0;
	u8 *search_result = NULL;
	u8 *search_end = NULL;
	u8 search[32];
	u8 temp[260]; /* save for total hex number */
	u8 *next_number = NULL;
	u8 *next_block = NULL;
	u8 number[8];
	int temp_len;

	memset(search, 0, sizeof(search));
	memset(temp, 0, sizeof(temp));
	memset(number, 0, sizeof(number));

	/* search block name */
	for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
		temp_len = 0;
		snprintf(search, sizeof(search), "%s%02d:", block_name, i); /* ex APCF01 */
		search_result = strstr(searchconetnt, search);
		if (search_result) {
			memset(temp, 0, sizeof(temp));
			temp_len = 0;
			search_result += strlen(search); /* move to first number */

			do {
				next_number = NULL;
				search_end = strstr(search_result, ",");
				if ((search_end - search_result) <= 0) {
					BTUSB_INFO("%s: can not find search end, break", __func__);
					break;
				}

				if ((search_end - search_result) > sizeof(number))
					break;

				memset(number, 0, sizeof(number));
				memcpy(number, search_result, search_end - search_result);

				if (number[0] == 0x20) /* space */
					ret = kstrtol(number + 1, 0, &parsing_result);
				else
					ret = kstrtol(number, 0, &parsing_result);

				if (ret == 0) {
					if (temp_len >= sizeof(temp)) {
						BTUSB_ERR("%s: %s data over %zu", __func__, block_name, sizeof(temp));
						return -1;
					}
					temp[temp_len] = parsing_result;
					temp_len++;
					/* find next number */
					next_number = strstr(search_end, "0x");

					/* find next block */
					next_block = strstr(search_end, ":");
				} else {
					BTUSB_ERR("%s:kstrtol ret = %d(%s), could incorrect format",
							__func__, ret, number);
					break;
				}

				if (next_number == NULL) {
					BTUSB_DBG("%s: not find next apcf number temp_len %d, break",
						__func__, temp_len);
					break;
				}

				if ((next_number > next_block) && (next_block != 0)) {
					BTUSB_DBG("%s: find next apcf number is over to next block temp_len %d, break",
						__func__, temp_len);
					break;
				}

				search_result = search_end + 1;
			} while (1);
		} else
			BTUSB_DBG("%s: %s is not found", __func__, search);

		if (temp_len) {
			BTUSB_INFO("%s: %s found", __func__, search);
			BTUSB_DBG("%s: kzalloc i=%d temp_len=%d", __func__, i, temp_len);
			save_content[i].content = kzalloc(temp_len, GFP_KERNEL);
			memcpy(save_content[i].content, temp, temp_len);
			save_content[i].length = temp_len;
			BTUSB_DBG("%s: save_content[%d].length %d temp_len=%d",
				__func__, i, save_content[i].length, temp_len);
		}

	}
	return ret;
}

static int btmtk_usb_load_woble_setting(char *bin_name, struct device *dev,
		u32 *code_len, struct btmtk_usb_data *data)
{
	int err;
	const struct firmware *fw_entry = NULL;
	u8 *image;

	*code_len = 0;

	BTUSB_INFO("%s: woble_setting_file_name = %s", __func__, bin_name);
	err = request_firmware(&fw_entry, bin_name, dev);
	if (err != 0 || fw_entry == NULL) {
		BTUSB_ERR("%s: request_firmware function fail!! error code = %d, fw_entry = %p",
				__func__, err, fw_entry);
		if (fw_entry)
			release_firmware(fw_entry);
		return err;
	}

	BTUSB_INFO("%s: woble_setting request_firmware size %zu success", __func__, fw_entry->size);
	image = kzalloc(fw_entry->size + 1, GFP_KERNEL); /* w:move to btmtk_usb_free_memory */
	if (image == NULL) {
		BTUSB_ERR("%s: kzalloc size %zu failed!!", __func__, fw_entry->size);
		release_firmware(fw_entry);
		return err;
	}

	memcpy(image, fw_entry->data, fw_entry->size);
	image[fw_entry->size] = '\0';

	*code_len = fw_entry->size;
	BTUSB_INFO("%s: code_len (%d) assign done", __func__, *code_len);

	err = btmtk_usb_load_woble_block_setting("APCF",
			data->woble_setting_apcf, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("APCF_ADD_MAC",
			data->woble_setting_apcf_fill_mac, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("APCF_ADD_MAC_LOCATION",
			data->woble_setting_apcf_fill_mac_location, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOOFF",
			data->woble_setting_radio_off, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;
	err = btmtk_usb_load_woble_ir_setting(data->woble_setting_radio_off, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOOFF_STATUS_EVENT",
			data->woble_setting_radio_off_status_event, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOOFF_COMPLETE_EVENT",
			data->woble_setting_radio_off_comp_event, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOON",
			data->woble_setting_radio_on, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOON_STATUS_EVENT",
			data->woble_setting_radio_on_status_event, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOON_COMPLETE_EVENT",
			data->woble_setting_radio_on_comp_event, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("APCF_RESMUE",
			data->woble_setting_apcf_resume, WOBLE_SETTING_COUNT, image);

LOAD_END:
	kfree(image);
	release_firmware(fw_entry);
	if (err)
		BTUSB_ERR("%s: error return %d", __func__, err);

	return err;
}

static void btmtk_usb_load_code_from_bin(u8 **image, char *bin_name,
					 struct device *dev, u32 *code_len)
{
	const struct firmware *fw_entry;
	int err = 0;
	static u32 chip_id;
	int retry = RETRY_TIMES;

	if (g_data->rom_patch_image && g_data->rom_patch_image_len && g_data->chip_id == chip_id) {
		/* no need to request firmware again. */
		*image = g_data->rom_patch_image;
		*code_len = g_data->rom_patch_image_len;
		return;

	} else {
		chip_id = g_data->chip_id;
		kvfree(g_data->rom_patch_image);
		g_data->rom_patch_image = NULL;
		g_data->rom_patch_image_len = 0;
	}

	do {
		err = request_firmware(&fw_entry, bin_name, dev);
		if (err == 0) {
			break;
		} else if (retry <= 0) {
			*image = NULL;
			BTUSB_ERR("%s: request_firmware %d times fail!!! err = %d", __func__, RETRY_TIMES, err);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
			log_counter_to_vitals_v2(ANDROID_LOG_INFO,
					BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
					BT_OPERATION, BT_KEY_PROBE, 1, "count",
					"request-fw-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
			log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
					BT_OPERATION, BT_KEY_PROBE, 1, "count",
					"request-fw-fail", VITALS_NORMAL);
#endif
			return;
		}
		BTUSB_ERR("%s: request_firmware fail!!! err = %d, retry = %d", __func__, err, retry);
		msleep(100);
	} while (retry-- > 0);

	*image = kvzalloc(fw_entry->size, GFP_KERNEL);
	if (*image == NULL) {
		BTUSB_ERR("%s: kzalloc failed!! error code = %d, size = %zu", __func__, err, fw_entry->size);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"mem-alloc-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"mem-alloc-fail", VITALS_NORMAL);
#endif
		goto exit;
	}

	memcpy(*image, fw_entry->data, fw_entry->size);
	*code_len = fw_entry->size;

	g_data->rom_patch_image = *image;
	g_data->rom_patch_image_len = *code_len;

exit:
	release_firmware(fw_entry);
}

static void btmtk_usb_stop_traffic(void)
{
	if ((g_data->bulk_urb_submitted == 0) && (g_data->interrupt_urb_submitted == 0)
			&& (g_data->isoc_urb_submitted == 0))
		return;

	BTUSB_INFO("%s", __func__);
	usb_kill_anchored_urbs(&g_data->intr_in_anchor);
	usb_kill_anchored_urbs(&g_data->bulk_in_anchor);
	usb_kill_anchored_urbs(&g_data->isoc_in_anchor);

	g_data->bulk_urb_submitted = 0;
	g_data->interrupt_urb_submitted = 0;
	g_data->isoc_urb_submitted = 0;
}

static void btmtk_usb_waker(struct work_struct *work)
{
	int err;

	err = usb_autopm_get_interface(g_data->intf);
	if (err < 0)
		return;

	usb_autopm_put_interface(g_data->intf);
}

static int btmtk_usb_submit_intr_urb(void)
{
	struct urb *urb;
	u8 *buf;
	unsigned int pipe;
	int err, size;

	BTUSB_INFO("%s", __func__);

	if (g_data->interrupt_urb_submitted) {
		BTUSB_WARN("%s: already submitted", __func__);
		return 0;
	}
	g_data->interrupt_urb_submitted = 0;

	if (!g_data->intr_ep) {
		BTUSB_ERR("%s: error 1", __func__);
		return -ENODEV;
	}

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		BTUSB_ERR("%s: error 2", __func__);
		return -ENOMEM;
	}
	/* size = le16_to_cpu(g_data->intr_ep->wMaxPacketSize); */
	size = le16_to_cpu(HCI_MAX_EVENT_SIZE);
	BTUSB_INFO("%s: maximum packet size:%d", __func__, size);

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		usb_free_urb(urb);
		BTUSB_ERR("%s: error 3", __func__);
		return -ENOMEM;
	}

	pipe = usb_rcvintpipe(g_data->udev, g_data->intr_ep->bEndpointAddress);

	usb_fill_int_urb(urb, g_data->udev, pipe, buf, size,
			 (usb_complete_t)btmtk_usb_intr_complete, (void *)g_data,
			 g_data->intr_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &g_data->intr_in_anchor);

	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s: urb %p submission failed (%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	} else {
		g_data->interrupt_urb_submitted = 1;
	}

	usb_free_urb(urb);
	return err;
}

static int btmtk_usb_submit_bulk_in_urb(void)
{
	struct urb *urb = NULL;
	u8 *buf = NULL;
	unsigned int pipe = 0;
	int err = 0;
	int size = HCI_MAX_FRAME_SIZE;

	BTUSB_INFO("%s", __func__);

	if (g_data->bulk_urb_submitted) {
		BTUSB_WARN("%s: already submitted", __func__);
		return 0;
	}
	g_data->bulk_urb_submitted = 0;

	if (!g_data->bulk_rx_ep) {
		BTUSB_ERR("%s: end error 1", __func__);
		return -ENODEV;
	}

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		BTUSB_ERR("%s: end error 2", __func__);
		return -ENOMEM;
	}

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		usb_free_urb(urb);
		BTUSB_ERR("%s: end error 3", __func__);
		return -ENOMEM;
	}
	pipe = usb_rcvbulkpipe(g_data->udev, g_data->bulk_rx_ep->bEndpointAddress);
	usb_fill_bulk_urb(urb, g_data->udev, pipe, buf, size,
				btmtk_usb_bulk_in_complete, g_data);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_mark_last_busy(g_data->udev);
	usb_anchor_urb(urb, &g_data->bulk_in_anchor);

	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s: urb %p submission failed (%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	} else {
		g_data->bulk_urb_submitted = 1;
	}

	usb_free_urb(urb);
	return err;
}

static int btmtk_usb_send_wmt_cmd(const u8 *cmd, const int cmd_len,
		const u8 *event, const int event_len, u32 delay, u32 retry_count)
{
	int ret = -1;	/* if successful, read length */
	int i = 0;
	bool check = FALSE;

	if (g_data == NULL || g_data->udev == NULL || g_data->io_buf == NULL ||
		g_data->o_usb_buf == NULL || cmd == NULL || cmd_len > HCI_MAX_COMMAND_SIZE || cmd_len <= 0) {
		BTUSB_ERR("%s: incorrect cmd pointer", __func__);
		return ret;
	}
	if (event != NULL && event_len > 0)
		check = TRUE;

	/* send WMT command */
	memcpy(g_data->o_usb_buf, cmd, cmd_len);
	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				0x01, DEVICE_CLASS_REQUEST_OUT, 0x30, 0x00, (void *)g_data->o_usb_buf,
				cmd_len, USB_CTRL_IO_TIMO);
	if (ret < 0) {
		BTUSB_ERR("%s: command send failed(%d)", __func__, ret);
		return ret;
	}

get_response_again:
	/* ms delay */
	mdelay(delay);

	/* check WMT event */
	memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
	ret = usb_control_msg(g_data->udev, usb_rcvctrlpipe(g_data->udev, 0),
				0x01, DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00, g_data->io_buf,
				USB_IO_BUF_SIZE, USB_CTRL_IO_TIMO);
	if (ret < 0) {
		BTUSB_ERR("%s: event get failed(%d)", __func__, ret);
		if (check == TRUE)
			return ret;
		else
			return 0;	/* Do not ask read so return 0 */
	}

	if (check == TRUE) {
		if (ret >= event_len && !memcmp(event, g_data->io_buf, event_len)) {
			return ret; /* return read length */
		} else if (retry_count > 0) {
			BTUSB_WARN("%s: Trying to get response... (%d)", __func__, ret);
			retry_count--;
			goto get_response_again;
		} else {
			BTUSB_ERR("%s: got unknown event:(%d)", __func__, event_len);
			pr_cont("\t");
			for (i = 0; i < ret && i < 64; i++)
				pr_cont("%02X ", g_data->io_buf[i]);
			pr_cont("\n");
		}
	}
	return -1;
}

static int btmtk_usb_push_data_to_metabuffer(u8 *buf, int buf_len, int hci_type)
{
	int ret = -EFAULT;
	u32 roomLeft = 0, last_len = 0, length = buf_len;
	u32 leftHciSize = 0;
	static u32 queueFullTimes = 0;

	if (!g_data) {
		BTUSB_ERR("%s: g_data is NULL return", __func__);
		return ret;
	}

	if (hci_type == HCI_ACLDATA_PKT)
		leftHciSize = leftACLSize;
	else
		leftHciSize = 0;
	length += leftHciSize == 0 ? 1 : 0; /* Means 1st packet */

	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	/* roomleft means the usable space */
	if (g_data->metabuffer->read_p <= g_data->metabuffer->write_p)
		roomLeft = META_BUFFER_SIZE - g_data->metabuffer->write_p +
				g_data->metabuffer->read_p - 1;
	else
		roomLeft = g_data->metabuffer->read_p - g_data->metabuffer->write_p - 1;

	/* no enough space to store the received data */
	if (roomLeft < length) {
		queueFullTimes++;
		if (queueFullTimes >= FW_QUEUE_FULL_ERR_MAX_TIMES) {
			BTUSB_ERR("%s: Queue full reached 100 times, toggle reset ", __func__);
			btmtk_usb_toggle_rst_pin();
		}
		BTUSB_ERR("%s: Queue is full !!!", __func__);
		btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
		return -ENOMEM;
	}

	/* have space, reset queue full times */
	queueFullTimes = 0;

	if (g_data->metabuffer->write_p + length <= META_BUFFER_SIZE) {
		if (leftHciSize == 0) {
			/* copy HCI type */
			g_data->metabuffer->buffer[g_data->metabuffer->write_p] = hci_type;
			g_data->metabuffer->write_p += 1;
		}
		/* copy payload */
		memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
				buf, buf_len);
		g_data->metabuffer->write_p += buf_len;
	} else {
		/* last_len should not 0 at first */
		last_len = META_BUFFER_SIZE - g_data->metabuffer->write_p;
		if (leftHciSize == 0) {
			/* copy HCI type */
			g_data->metabuffer->buffer[g_data->metabuffer->write_p] = hci_type;
			g_data->metabuffer->write_p += 1;
			last_len--;
		}

		/* copy payload */
		if (last_len > 0) {
			memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
					buf, last_len);
			memcpy(g_data->metabuffer->buffer, buf + last_len,
					buf_len - last_len);
			g_data->metabuffer->write_p = buf_len - last_len;
		} else {
			memcpy(g_data->metabuffer->buffer, buf, buf_len);
			g_data->metabuffer->write_p = buf_len;
		}
	}
	if (g_data->metabuffer->write_p == META_BUFFER_SIZE)
		g_data->metabuffer->write_p = 0;
	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	return buf_len;
}

static int btmtk_usb_add_to_hci_log(const u8 *buf, int buf_len, int hci_type)
{
	u8 *alloc_buf = NULL;
	int ret = -1;
	u8 header[] = { 0xff, 0x00, 0xFE, 0x00 };
	int header_len = 0;

	header_len = sizeof(header);
	if (buf_len <= 0) {
		BTUSB_ERR("%s: buf_len = %d error", __func__, buf_len);
		return ret;
	}
	header[1] = buf_len + 2; /* 2 is include 0xfe, 0x00(hci type) */
	header[3] = hci_type;
	alloc_buf = kzalloc(buf_len + header_len, GFP_KERNEL);
	if (alloc_buf == NULL)
		return ret;

	memcpy(alloc_buf, header, header_len);
	memcpy(alloc_buf + header_len, buf, buf_len);
	ret = btmtk_usb_push_data_to_metabuffer(alloc_buf, buf_len + header_len, HCI_EVENT_PKT);
	kfree(alloc_buf);
	return ret;
}

static int btmtk_usb_send_hci_cmd(const u8 *cmd, const int cmd_len,
		const u8 *event, const int event_len)
{
	/** return length of even if compare successfully., 0 no need check event., < 0 error */
	int ret = -1;	/* if successful, read length */
	int len = 0;
	int i = 0;
	unsigned long timo = 0;
	bool check = FALSE;

	if (is_mt7668(g_data)) {
		if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON &&
				g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_WOBLE) {
			BTUSB_WARN("%s: chip power isn't on, ignore this command, state is %d", __func__,
					g_data->is_mt7668_dongle_state);
			return ret;
		}
	}

	/* parameter check */
	if (g_data == NULL || g_data->udev == NULL || g_data->io_buf == NULL ||
		g_data->o_usb_buf == NULL || cmd == NULL || cmd_len > HCI_MAX_COMMAND_SIZE || cmd_len <= 0) {
		BTUSB_ERR("%s: incorrect cmd pointer", __func__);
		return ret;
	}
	if (event != NULL && event_len > 0)
		check = TRUE;

	/* need get event by interrupt, stop traffic before cmd send */
	btmtk_usb_stop_traffic();

	/* send HCI command */
	memcpy(g_data->o_usb_buf, cmd, cmd_len);
	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				0, DEVICE_CLASS_REQUEST_OUT, 0, 0,
				(void *)g_data->o_usb_buf, cmd_len, USB_CTRL_IO_TIMO);
	if (ret < 0) {
		BTUSB_ERR("%s: command send failed(%d)", __func__, ret);
		return ret;
	}

	if (event_len == -1) {
		/**
		 * If event_len == -1, DO NOT read event, since FW wouldn't feedback
		 */
		return 0;
	}

	/* check HCI event */
	timo = jiffies + msecs_to_jiffies(USB_INTR_MSG_TIMO);
	do {
		memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
		ret = usb_interrupt_msg(g_data->udev, usb_rcvintpipe(g_data->udev, 1),
					g_data->io_buf, USB_IO_BUF_SIZE, &len, USB_INTR_MSG_TIMO);
		if (ret < 0) {
			BTUSB_ERR("%s: event get failed(%d)", __func__, ret);
			if (check == TRUE)
				return ret;
			else
				return 0;	/* Do not ask read so return 0 */
		}

		if (len >= 2 && g_data->io_buf[1] + 2 != len) {
			/* Incorrect packet format and length */
			BTUSB_WARN("%s: Ignore incorrect format packet:%02X %02X",
					__func__, g_data->io_buf[0], g_data->io_buf[1]);
			continue;
		}

		if (check == TRUE) {
			/* maybe don't care some returned parameters */
			if (len >= event_len) {
				for (i = 0; i < event_len; i++) {
					if (event[i] != g_data->io_buf[i])
						break;
				}
			} else {
				BTUSB_ERR("%s: event length is not match(%d/%d)", __func__, len, event_len);
			}
			if (i != event_len) {
				/* In case standard or picus event */
				if (btmtk_usb_dispatch_event(g_data->io_buf, len) < 0) {
					BTUSB_WARN("%s: got unexpected event:(%d/%d), actual len = %d",
							__func__, i, event_len, len);
					pr_cont("\t");
					for (i = 0; i < len && i < 64; i++)
						pr_cont("%02X ", g_data->io_buf[i]);
					pr_cont("\n");
				}
			} else
				return len; /* actually read length */
		}
	} while (time_before(jiffies, timo));

	BTUSB_ERR("%s: error, got event timeout, jiffies = %lu", __func__, jiffies);
	return -1;
}

int btmtk_usb_meta_send_data(const u8 *buffer, const unsigned int length)
{
	int ret = 0;

	if (buffer[0] != HCI_COMMAND_PKT) {
		BTUSB_WARN("the data from meta isn't HCI command, value: 0x%X", buffer[0]);
	} else {
		u8 *buf = (u8 *)buffer;

		ret = usb_control_msg(g_data->udev,
					usb_sndctrlpipe(g_data->udev, 0), 0x0,
					DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00,
					buf + 1, length - 1,
					USB_CTRL_IO_TIMO);
	}

	if (ret < 0) {
		BTUSB_ERR("%s: error1(%d)", __func__, ret);
		return ret;
	}

	return length;
}

int btmtk_usb_send_data(const u8 *buffer, const unsigned int length)
{
	struct urb *urb = NULL;
	unsigned int pipe;
	int err;
	int send_data_len = length - 1;
	char *buf = NULL;

	if (!g_data->bulk_tx_ep) {
		BTUSB_ERR("%s: No bulk_tx_ep", __func__);
		return -ENODEV;
	}

	if (buffer[0] == HCI_ACLDATA_PKT) {
		while (g_data->meta_tx != 0)
			/* msleep if less than 20ms is no accuracy */
			usleep_range(400, 500);

		g_data->meta_tx = 1;
		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			BTUSB_ERR("%s: No memory for ACL", __func__);
			return -ENOMEM;
		}
		buf = usb_alloc_coherent(g_data->udev, send_data_len, GFP_KERNEL, &urb->transfer_dma);

		urb->transfer_buffer = buf;
		urb->transfer_buffer_length = send_data_len;

		if (!buf) {
			BTUSB_ERR("%s: usb_alloc_coherent error", __func__);
			err = -ENOMEM;
			goto error_buffer;
		}
		/* remove the ACL header:0x02 */
		memcpy(buf, buffer + 1, send_data_len);

		pipe = usb_sndbulkpipe(g_data->udev, g_data->bulk_tx_ep->bEndpointAddress);

		usb_fill_bulk_urb(urb, g_data->udev, pipe, buf,
					length - 1, (usb_complete_t)btmtk_usb_tx_complete_meta,
					(void *)g_data);

		urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

		usb_anchor_urb(urb, &g_data->bulk_out_anchor);

		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err == 0)
			err = length;
		else
			BTUSB_ERR("send ACL data fail, error:%d", err);
		mdelay(1);

	} else if (buffer[0] == HCI_SCODATA_PKT) {
		BTUSB_DBG("%s: HCI_SCODATA_PKT start (%d/%d)", __func__,
				send_data_len, g_data->isoc_tx_ep->wMaxPacketSize);
		if (!g_data->isoc_tx_ep) {
			BTUSB_ERR("%s: No isoc_tx_ep", __func__);
			return -ENODEV;
		}
		urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_KERNEL);
		if (!urb) {
			BTUSB_ERR("%s: No memory for SCO", __func__);
			return -ENOMEM;
		}
		pipe = usb_sndisocpipe(g_data->udev, g_data->isoc_tx_ep->bEndpointAddress);
		buf = kzalloc(send_data_len, GFP_KERNEL);
		if (!buf) {
			BTUSB_ERR("%s: kzalloc error", __func__);
			err = -ENOMEM;
			goto error_buffer;
		}
		memcpy(buf, buffer + 1, send_data_len);
		usb_fill_int_urb(urb, g_data->udev, pipe, buf,
			send_data_len, (usb_complete_t)btmtk_usb_isoc_tx_complete,
			(void *)g_data, g_data->isoc_tx_ep->bInterval);

		urb->transfer_flags = URB_ISO_ASAP;

		__fill_isoc_descriptor(urb, send_data_len,
			le16_to_cpu(g_data->isoc_tx_ep->wMaxPacketSize));
		usb_anchor_urb(urb, &g_data->isoc_out_anchor);

		atomic_inc(&g_data->isoc_out_count);
		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err == 0) {
			err = length;
			BTUSB_DBG("%s: HCI_SCODATA_PKT end", __func__);
		} else {
			BTUSB_ERR("submit SCO urb fail, error:%d", err);
			err = -1;
		}
	} else {
		BTUSB_WARN("%s: unknown data", __func__);
		err = -1;
	}

error_buffer:
	if (err < 0) {
		if (urb) {
			if (buffer[0] == HCI_ACLDATA_PKT)
				usb_free_coherent(g_data->udev, urb->transfer_buffer_length,
						urb->transfer_buffer, urb->transfer_dma);
			else if (buffer[0] == HCI_SCODATA_PKT)
				kfree(urb->transfer_buffer);
			kfree(urb->setup_packet);
			usb_unanchor_urb(urb);
		}
	} else {
		usb_mark_last_busy(g_data->udev);
	}

	usb_free_urb(urb);
	return err;
}

static inline void btmtk_usb_woble_wake_lock(struct btmtk_usb_data *data)
{
#if SUPPORT_UNIFY_WOBLE
	BTUSB_INFO("%s: WoBLE WakeLock Enable", __func__);
	__pm_stay_awake(data->woble_wlock);
#endif
}

static inline void btmtk_usb_woble_wake_unlock(struct btmtk_usb_data *data)
{
#if SUPPORT_UNIFY_WOBLE
	BTUSB_INFO("%s: WoBLE WakeLock Disable", __func__);
	__pm_relax(data->woble_wlock);
#endif
}

static int btmtk_usb_BT_init(void)
{
	dev_t devID = MKDEV(BT_major, 0);
	dev_t devIDfwlog = MKDEV(BT_majorfwlog, 1);
	dev_t devID_sco = MKDEV(BT_major_sco, 0);
	int ret0 = -1, ret1 = -1, ret2 = -1;
	int cdevErr0 = 0, cdevErr1 = 0, cdevErr2 = 0;
	int major = 0;
	int majorfwlog = 0;
	int major_sco = 0;

	BTUSB_INFO("%s: devID %d, devID_fwlog %d, devID_sco %d", __func__, devID, devIDfwlog, devID_sco);

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_INIT);
	USB_MUTEX_UNLOCK();

	btmtk_proc_create_new_entry();

#ifdef FIXED_STPBT_MAJOR_DEV_ID
	ret0 = register_chrdev_region(devID, 1, "BT_chrdev");
	if (ret0) {
		BTUSB_ERR("fail to register_chrdev %d, allocate chrdev again", MAJOR(devID));
		ret0 = alloc_chrdev_region(&devID, 0, 1, "BT_chrdev");
		if (ret0) {
			BTUSB_ERR("fail to allocate chrdev");
			return ret0;
		}
	}

	ret1 = register_chrdev_region(devIDfwlog, 1, "BT_chrdevfwlog");
	if (ret1) {
		BTUSB_ERR("fail to register_chrdev %d, allocate chrdev again", MAJOR(devIDfwlog));
		ret1 = alloc_chrdev_region(&devIDfwlog, 0, 1, "BT_chrdevfwlog");
		if (ret1) {
			BTUSB_ERR("fail to allocate chrdev fwlog");
			goto err0;
		}
	}

	ret2 = register_chrdev_region(devID_sco, 1, "BT_chrdev_sco");
	if (ret2) {
		BTUSB_ERR("fail to register_chrdev %d, allocate chrdev again", MAJOR(devID_sco));
		ret2 = alloc_chrdev_region(&devID_sco, 0, 1, "BT_chrdev_sco");
		if (ret2) {
			BTUSB_ERR("fail to allocate chrdev sco");
			goto err0;
		}
	}
#else /* FIXED_STPBT_MAJOR_DEV_ID */
	ret0 = alloc_chrdev_region(&devID, 0, 1, "BT_chrdev");
	if (ret0) {
		BTUSB_ERR("fail to allocate chrdev");
		return ret0;
	}

	ret1 = alloc_chrdev_region(&devIDfwlog, 0, 1, "BT_chrdevfwlog");
	if (ret1) {
		BTUSB_ERR("fail to allocate chrdev");
		goto err0;
	}
	ret2 = alloc_chrdev_region(&devID_sco, 0, 1, "BT_chrdev_sco");
	if (ret2) {
		BTUSB_ERR("fail to allocate chrdev sco");
		goto err0;
	}
#endif /* FIXED_STPBT_MAJOR_DEV_ID */

	BT_major = major = MAJOR(devID);
	BTUSB_INFO("%s: major number: %d", __func__, BT_major);
	BT_majorfwlog = majorfwlog = MAJOR(devIDfwlog);
	BTUSB_INFO("%s: BT_majorfwlog number: %d", __func__, BT_majorfwlog);
	BT_major_sco = major_sco = MAJOR(devID_sco);
	BTUSB_INFO("%s: BT_major_sco number: %d", __func__, BT_major_sco);

	cdev_init(&BT_cdev, &BT_fops);
	BT_cdev.owner = THIS_MODULE;
	cdev_init(&BT_cdev_sco, &BT_sco_fops);
	BT_cdev_sco.owner = THIS_MODULE;

	cdev_init(&BT_cdevfwlog, &BT_fopsfwlog);
	BT_cdevfwlog.owner = THIS_MODULE;
	cdevErr0 = cdev_add(&BT_cdev, devID, 1);
	if (cdevErr0)
		goto err1;
	cdevErr1 = cdev_add(&BT_cdev_sco, devID_sco, 1);
	if (cdevErr1)
		goto err1;
	cdevErr2 = cdev_add(&BT_cdevfwlog, devIDfwlog, 1);
	if (cdevErr2)
		goto err1;
	BTUSB_INFO("%s: %s driver(major %d) installed.", __func__, "BT_chrdev", BT_major);
	BTUSB_INFO("%s: %s driver(major %d) installed.", __func__, "BT_chrdevfwlog", BT_majorfwlog);
	BTUSB_INFO("%s: %s driver(major %d) installed.", __func__, "BT_cdev_sco", BT_major_sco);
	pBTClass = class_create(THIS_MODULE, "BT_chrdev");
	if (IS_ERR(pBTClass)) {
		BTUSB_ERR("class create fail, error code(%ld)", PTR_ERR(pBTClass));
		goto err1;
	}

	pBTDev = device_create(pBTClass, NULL, devID, NULL, "stpbt");
	if (IS_ERR(pBTDev)) {
		BTUSB_ERR("device create fail, error code(%ld)", PTR_ERR(pBTDev));
		goto err2;
	}

	pBTDevfwlog = device_create(pBTClass, NULL, devIDfwlog, NULL, "stpbtfwlog");
	if (IS_ERR(pBTDevfwlog)) {
		BTUSB_ERR("device(stpbtfwlog) create fail, error code(%ld)", PTR_ERR(pBTDevfwlog));
		goto err2;
	}

	pBTDev_sco = device_create(pBTClass, NULL, devID_sco, NULL, "stpbt_sco");
	if (IS_ERR(pBTDev_sco)) {
		BTUSB_ERR("device(stpbt_sco) create fail, error code(%ld)", PTR_ERR(pBTDev_sco));
		goto err2;
	}
	BTUSB_INFO("%s: BT_major %d, BT_majorfwlog %d", __func__, BT_major, BT_majorfwlog);
	BTUSB_INFO("%s: devID %d, devIDfwlog %d", __func__, devID, devIDfwlog);
	BTUSB_INFO("%s: BT_major_sco %d, devID_sco %d", __func__, BT_major_sco, devID_sco);
	g_devIDfwlog = devIDfwlog;
	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_INIT);
	FOPS_MUTEX_UNLOCK();

	/* init wait queue */
	init_waitqueue_head(&(inq));
	init_waitqueue_head(&(fw_log_inq));
	init_waitqueue_head(&(inq_isoc));

	/* register system power off callback function. */
	do {
		typedef void (*func_ptr) (int (*f) (void));
		char *func_name = "RegisterPdwncCallback";
		func_ptr pFunc = (func_ptr) kallsyms_lookup_name(func_name);

		if (pFunc) {
			BTUSB_INFO("%s: Register Pdwnc callback success.", __func__);
			pFunc(&btmtk_usb_standby);
		} else
			BTUSB_WARN("%s: No Exported Func Found [%s], just skip!", __func__, func_name);
	} while (0);

	/* register early_suspend / late_reasume callback function. */
	do {
		char *register_early_suspend_func_name = "RegisterEarlySuspendNotification";
		char *register_late_resume_func_name = "RegisterLateResumeNotification";

		register_early_suspend_func = (register_early_suspend)
			kallsyms_lookup_name(register_early_suspend_func_name);
		register_late_resume_func = (register_late_resume)
			kallsyms_lookup_name(register_late_resume_func_name);

		if (register_early_suspend_func && register_late_resume_func) {
			BTUSB_INFO("%s: Register early suspend/late resume nitofication success.", __func__);
			register_early_suspend_func(&btmtk_usb_early_suspend);
			register_late_resume_func(&btmtk_usb_late_resume);
		} else {
			BTUSB_WARN("%s: No Exported Func Found [%s], just skip!", __func__,
					register_late_resume_func_name);
		}
	} while (0);

	/* allocate buffers. */
	if (btmtk_usb_allocate_memory() < 0) {
		BTUSB_ERR("%s: allocate memory failed!", __func__);
		goto err2;
	}

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
	USB_MUTEX_UNLOCK();
#if SUPPORT_UNIFY_WOBLE
#if (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE)
		g_data->woble_wlock = wakeup_source_register(NULL, "btmtk_woble_wakelock");
#else
		g_data->woble_wlock = wakeup_source_register("btmtk_woble_wakelock");
#endif
#endif
	spin_lock_init(&g_data->fwlog_lock);
	skb_queue_head_init(&g_data->fwlog_queue);
	spin_lock_init(&g_data->isoc_lock);
	skb_queue_head_init(&g_data->isoc_in_queue);
	spin_lock_init(&g_data->txlock);
	/* init meta buffer */
	spin_lock_init(&(g_data->metabuffer->spin_lock.lock));

#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
	init_timer(&chip_reset_timer);
	init_timer(&g_data->fw_dump_timer);
	init_timer(&g_data->chip_rst_disc_timer);
#endif

	BTUSB_INFO("%s: end", __func__);
	return 0;

err2:
	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

	btmtk_usb_free_memory();

err1:
	if (cdevErr0 < 0)
		cdev_del(&BT_cdev);
	if (cdevErr1 < 0)
		cdev_del(&BT_cdev_sco);
	if (cdevErr2 < 0)
		cdev_del(&BT_cdevfwlog);

err0:
	if (ret0 == 0)
		unregister_chrdev_region(devID, 1);
	if (ret1 == 0)
		unregister_chrdev_region(devIDfwlog, 1);
	if (ret2 == 0)
		unregister_chrdev_region(devID_sco, 1);
	BTUSB_ERR("%s: error", __func__);
	return -1;
}

static void btmtk_usb_BT_exit(void)
{
	dev_t dev = MKDEV(BT_major, 0);
	dev_t devIDfwlog = g_devIDfwlog; /* MKDEV(BT_majorfwlog, 0); */
	dev_t devID_sco = MKDEV(BT_major_sco, 0);

	BTUSB_INFO("%s: BT_major %d, BT_majorfwlog %d", __func__, BT_major, BT_majorfwlog);
	BTUSB_INFO("%s: dev %d, devIDfwlog %d", __func__, dev, devIDfwlog);

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_UNKNOWN);
	FOPS_MUTEX_UNLOCK();
#if SUPPORT_UNIFY_WOBLE
	if (g_data)
		wakeup_source_unregister(g_data->woble_wlock);
	else
		BTUSB_ERR("%s:g_data is NULL, no destroy woble_wlock", __func__);
#endif

	if (pBTDev_sco) {
		device_destroy(pBTClass, devID_sco);
		pBTDev_sco = NULL;
	}

	if (pBTDevfwlog) {
		device_destroy(pBTClass, devIDfwlog);
		pBTDevfwlog = NULL;
	}

	if (pBTDev) {
		device_destroy(pBTClass, dev);
		pBTDev = NULL;
	}

	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

	cdev_del(&BT_cdev_sco);
	unregister_chrdev_region(devID_sco, 1);

	cdev_del(&BT_cdevfwlog);
	unregister_chrdev_region(devIDfwlog, 1);

	cdev_del(&BT_cdev);
	unregister_chrdev_region(dev, 1);

	btmtk_usb_free_memory();

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_UNKNOWN);
	USB_MUTEX_UNLOCK();

	if (g_proc_dir != 0) {
		remove_proc_entry("bt_chip_reset_delay", g_proc_dir);
		remove_proc_entry("bt_fw_version", g_proc_dir);
		remove_proc_entry("stpbt", NULL);
		BTUSB_INFO("%s: BT_proc node removed.", __func__);
	}
	g_proc_dir = 0;

	BTUSB_INFO("%s: BT_chrdev driver removed.", __func__);
}

static int btmtk_usb_handle_resume(void)
{
	int ret = -1;

	BTUSB_INFO("%s", __func__);
	if (is_support_unify_woble(g_data)) {
		ret = btmtk_usb_unify_woble_wake_up();
		if (ret)
			return WOBLE_FAIL;
	}

	if (btmtk_usb_submit_intr_urb() < 0)
		return -1;

	if (btmtk_usb_submit_bulk_in_urb() < 0)
		return -1;

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
	USB_MUTEX_UNLOCK();

	BTUSB_DBG("%s: call inq", __func__);
	wake_up_interruptible(&inq);
	return 0;
}

static int btmtk_usb_wait_until_event(u8 *wait_event, int wait_event_len, int total_timeout,
		int event_interval_timeout, int hci)
{
	/** return 0 if compare successfully., < 0 if error
	 *  hci: 1 - Send the excepted event to stack
	 */
	int ret = -1, len = 0;
	unsigned long comp_event_timo = 0, start_time = 0;

	start_time = jiffies;
	/* check HCI event */
	comp_event_timo = jiffies + msecs_to_jiffies(total_timeout);
	do {
		memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
		ret = usb_interrupt_msg(g_data->udev, usb_rcvintpipe(g_data->udev, 1),
			g_data->io_buf, USB_IO_BUF_SIZE, &len, event_interval_timeout);
		if (ret == (-ETIMEDOUT)) {
			continue;
		} else if (ret < 0) {
			BTUSB_ERR("%s: ret(%d) error break", __func__, ret);
			break;
		}

		if (len >= 2 && g_data->io_buf[1] + 2 != len) {
			BTUSB_WARN("%s: Ignore incorrect format packet:%02X %02X",
					__func__, g_data->io_buf[0], g_data->io_buf[1]);
			continue;
		}

		/* check if receive expected event */
		if (len >= wait_event_len) {
			if (memcmp(wait_event, g_data->io_buf, wait_event_len) == 0) {
				if (hci) /* 1: save it as vendor event */
					btmtk_usb_add_to_hci_log(g_data->io_buf, len, HCI_EVENT_PKT);
				return 0;
			}
		}
		/* maybe this is a standard or picus event */
		btmtk_usb_dispatch_event(g_data->io_buf, len);
	} while (time_before(jiffies, comp_event_timo));

	BTUSB_ERR("%s: Get compelete event fail %d", __func__, ret);
	return ret;
}

static int btmtk_usb_set_Woble_APCF_filter_parameter(void)
{
	int ret = -1;
	u8 cmd[] = { 0x57, 0xfd, 0x0a, 0x01, 0x00, 0x5a, 0x20, 0x00, 0x20, 0x00, 0x01, 0x80, 0x00 };
	u8 event_complete[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00, 0x01/*, 00, 63*/ };

	BTUSB_INFO("%s", __func__);
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event_complete, sizeof(event_complete));
	if (ret < 0)
		BTUSB_ERR("%s: end ret %d", __func__, ret);
	else
		ret = 0;

	return ret;
}

static int btmtk_usb_send_read_BDADDR_cmd(void)
{
	u8 cmd[] = { 0x09, 0x10, 0x00 };
	int i;
	int retry_counter = 30;
	int ret = -1;
	int actual_length;
	u8 zero[BD_ADDRESS_SIZE];

	BTUSB_INFO("%s", __func__);
	memset(zero, 0, sizeof(zero));
	if (memcmp(g_data->bdaddr, zero, BD_ADDRESS_SIZE) != 0) {
		BTUSB_INFO("%s: already got bdaddr %02x%02x%02x%02x%02x%02x, return 0", __func__,
		g_data->bdaddr[0], g_data->bdaddr[1], g_data->bdaddr[2],
		g_data->bdaddr[3], g_data->bdaddr[4], g_data->bdaddr[5]);
		return 0;
	}

	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data == NULL!", __func__);
		return -1;
	}
	if (g_data->udev == NULL) {
		BTUSB_ERR("%s: g_data->udev == NULL!", __func__);
		return -1;
	}
	if (g_data->io_buf == NULL) {
		BTUSB_ERR("%s: g_data->io_buf == NULL!", __func__);
		return -1;
	}

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	/* Get response of Read BD_ADDR */
	while (1) {
		memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
		ret = usb_interrupt_msg(g_data->udev,
				usb_rcvintpipe(g_data->udev, 1),
				g_data->io_buf, USB_IO_BUF_SIZE,
				&actual_length, USB_INTR_MSG_TIMO);
		if (ret < 0) {
			BTUSB_ERR("%s: error2(%d)", __func__, ret);
			return ret;
		}

		if ((actual_length == 12) &&
			(g_data->io_buf[0] == 0x0e) &&
			(g_data->io_buf[1] == 0x0a) &&
			(g_data->io_buf[2] == 0x01) &&
			(g_data->io_buf[3] == 0x09) &&
			(g_data->io_buf[4] == 0x10) &&
			(g_data->io_buf[5] == 0x00)) {
			break;
		}
		BTUSB_WARN("%s: drop unknown event:", __func__);

		mdelay(1);
		retry_counter--;

		if (retry_counter < 0)
			return ret;
	}

	for (i = 0; i < BD_ADDRESS_SIZE; i++)
		g_data->bdaddr[i] = g_data->io_buf[6 + i];

	BTUSB_INFO("%s: ret = %d, TV BDADDR = %02X:%02X:%02X:%02X:%02X:%02X", __func__, ret,
		g_data->bdaddr[0], g_data->bdaddr[1], g_data->bdaddr[2],
		g_data->bdaddr[3], g_data->bdaddr[4], g_data->bdaddr[5]);
	return ret;
}

/**
 * Set APCF manufacturer data and filter parameter
 */
static int btmtk_usb_set_Woble_APCF(void)
{
	int ret = -1;
	int i = 0;
	u8 manufactur_data[] = { 0x57, 0xfd, 0x27, 0x06, 0x00, 0x5a,
		0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x43, 0x52, 0x4B, 0x54, 0x4D,
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	u8 event_complete[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00/*, 0x06 00 63*/ };

	BTUSB_INFO("%s: g_data->woble_setting_apcf[0].length %d",
		__func__, g_data->woble_setting_apcf[0].length);

	/* start to send apcf cmd from woble setting  file */
	if (g_data->woble_setting_apcf[0].length) {
		for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
			if (!g_data->woble_setting_apcf[i].length)
				continue;

			BTUSB_INFO("%s: g_data->woble_setting_apcf_fill_mac[%d].content[0] = 0x%02x",
				__func__, i,
				g_data->woble_setting_apcf_fill_mac[i].content[0]);
			BTUSB_INFO("%s: g_data->woble_setting_apcf_fill_mac_location[%d].length = %d",
				__func__, i,
				g_data->woble_setting_apcf_fill_mac_location[i].length);

			if ((g_data->woble_setting_apcf_fill_mac[i].content[0] == 1) &&
				g_data->woble_setting_apcf_fill_mac_location[i].length) {
				/* need add BD addr to apcf cmd */
				memcpy(g_data->woble_setting_apcf[i].content +
					(*g_data->woble_setting_apcf_fill_mac_location[i].content),
					g_data->bdaddr, BD_ADDRESS_SIZE);
				BTUSB_INFO("%s: apcf %d ,add mac to location %d",
					__func__, i,
					(*g_data->woble_setting_apcf_fill_mac_location[i].content));
			}

			BTUSB_INFO("%s: send APCF %d", __func__, i);
			BTUSB_INFO_RAW(g_data->woble_setting_apcf[i].content, g_data->woble_setting_apcf[i].length,
				"woble_setting_apcf ");

			ret = btmtk_usb_send_hci_cmd(
				g_data->woble_setting_apcf[i].content,
				g_data->woble_setting_apcf[i].length, NULL, -1);
			if (ret) {
				BTUSB_ERR("%s: manufactur_data error ret %d", __func__, ret);
				return ret;
			}

			ret = btmtk_usb_wait_until_event(event_complete, sizeof(event_complete),
				WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
			if (ret)
				BTUSB_ERR("%s: (from file) btmtk_usb_wait_until_event end ret %d",
						__func__, ret);
		}
	} else { /* use default */
		BTUSB_INFO("%s: use default manufactur data", __func__);
		memcpy(manufactur_data + 9, g_data->bdaddr, BD_ADDRESS_SIZE);
		ret = btmtk_usb_send_hci_cmd(
			manufactur_data, sizeof(manufactur_data), NULL, -1);
		if (ret) {
			BTUSB_ERR("%s: manufactur_data error ret %d", __func__, ret);
			return ret;
		}

		ret = btmtk_usb_wait_until_event(event_complete, sizeof(event_complete),
			WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
		if (ret)
			BTUSB_ERR("%s: btmtk_usb_wait_until_event end ret %d", __func__, ret);

		ret = btmtk_usb_set_Woble_APCF_filter_parameter();
	}
	BTUSB_INFO("%s: end ret=%d", __func__, ret);
	return ret;
}

static int btmtk_usb_handle_leaving_WoBLE_state(void)
{
	int ret = -1;
	u8 status_event[] = { 0x0F, 0x04, 0x00, 0x01, 0xC9, 0xFC };
	u8 comp_event[] = { 0xe6, 0x02, 0x08, 0x01 };
	int i = 0;

	if (!is_support_unify_woble(g_data)) {
		BTUSB_ERR("%s: do nothing", __func__);
		return 0;
	}

	BTUSB_INFO("%s: g_data->woble_setting_radio_on[0].length %d",
		__func__, g_data->woble_setting_radio_on[0].length);
	if (g_data->woble_setting_radio_on[0].length) {
		/* start to send radio off cmd from woble setting file */
		for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
			if (g_data->woble_setting_radio_on[i].length) {
				BTUSB_INFO_RAW(g_data->woble_setting_radio_on[i].content,
					g_data->woble_setting_radio_on[i].length, "%s: send radio on %d:", __func__, i);

				ret = btmtk_usb_send_hci_cmd(g_data->woble_setting_radio_on[i].content,
					g_data->woble_setting_radio_on[i].length,
					NULL, -1);
				if (ret != 0) {
					BTUSB_ERR("%s: btmtk_usb_send_hci_cmd return fail %d", __func__, ret);
					return ret;
				}

				if (g_data->woble_setting_radio_on_status_event[i].length) {
					BTUSB_INFO("%s: check %d woble_setting_radio_on_status_event", __func__, i);
					ret = btmtk_usb_wait_until_event(
						g_data->woble_setting_radio_on_status_event[i].content,
						g_data->woble_setting_radio_on_status_event[i].length,
						USB_INTR_MSG_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
					if (ret) {
						BTUSB_ERR("%s: woble_setting_radio_off_status_event %d return error",
							__func__, i);
						return ret;
					}
				}

				if (g_data->woble_setting_radio_on_comp_event[i].length) {
					BTUSB_INFO("%s: check %d woble_setting_radio_on_comp_event", __func__, i);
					ret = btmtk_usb_wait_until_event(
							g_data->woble_setting_radio_on_comp_event[i].content,
							g_data->woble_setting_radio_on_comp_event[i].length,
							USB_INTR_MSG_TIMO,
							WOBLE_EVENT_INTERVAL_TIMO, 1);
					if (ret) {
						BTUSB_ERR(
							"%s: woble_setting_radio_off_status_event %d return error",
							__func__, i);
						return ret;
					}
				}
			}
		}
	} else { /* use default */
		BTUSB_WARN("%s: use default radio on cmd", __func__);
		ret = btmtk_usb_send_leave_woble_suspend_cmd();
		if (ret) {
			BTUSB_ERR("%s: btmtk_usb_send_leave_woble_suspend_cmd return error", __func__);
			return ret;
		}

		ret = btmtk_usb_wait_until_event(status_event, sizeof(status_event),
			USB_INTR_MSG_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
		if (ret) {
			BTUSB_ERR("%s: btmtk_usb_wait_until_event status_event return error", __func__);
			return ret;
		}

		ret = btmtk_usb_wait_until_event(comp_event, sizeof(comp_event),
			WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 1);
		if (ret) {
			BTUSB_ERR("%s: btmtk_usb_wait_until_event comp_event return error", __func__);
			return ret;
		}
	}
	return ret;
}

static int btmtk_usb_del_Woble_APCF_index(void)
{
	int ret = -1;
	u8 cmd[] = { 0x57, 0xfd, 0x03, 0x01, 0x01, 0x5a };
	u8 event_complete[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00, 0x01/*, 00, 63*/ };

	BTUSB_INFO("%s", __func__);
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	if (ret) {
		BTUSB_ERR("%s: btmtk_usb_send_hci_cmd error %d", __func__, ret);
		return ret;
	}

	ret = btmtk_usb_wait_until_event(event_complete, sizeof(event_complete),
			WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
	if (ret)
		BTUSB_ERR("%s: btmtk_usb_wait_until_event error %d", __func__, ret);
	return ret;
}

/* Check power status, if power is off, try to set power on */
static int btmtk_usb_reset_power_on(void)
{
	if (is_mt7668(g_data)) {
		while (g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWERING_ON ||
			g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWERING_OFF) {
			BTUSB_INFO("%s: dongle state is POWERING ON or OFF.", __func__);
			msleep(100);
		}
		if (g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWER_OFF) {
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
			if (btmtk_usb_send_wmt_power_on_cmd_7668() < 0)
				return -1;
			if (btmtk_usb_send_hci_tci_set_sleep_cmd_7668() < 0)
				return -1;
			if (btmtk_usb_send_hci_reset_cmd() < 0)
				return -1;
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_ON;
		}

		if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON) {
			BTUSB_WARN("%s: end of Incorrect state:%d", __func__, g_data->is_mt7668_dongle_state);
			return -EBADFD;
		}
		BTUSB_INFO("%s: 7668 end success", __func__);
	}
	return 0;
}

static int btmtk_usb_send_unify_woble_suspend_cmd(void)
{
	int ret = 0;	/* if successful, 0 */
	u8 cmd[] = { 0xC9, 0xFC, 0x14, 0x01, 0x20, 0x02, 0x00, 0x01,
		0x02, 0x01, 0x00, 0x05, 0x10, 0x01, 0x00, 0x40, 0x06,
		0x02, 0x40, 0x5A, 0x02, 0x41, 0x0F };

	BTUSB_DBG("%s", __func__);
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	if (ret) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}
	return ret;
}

static int btmtk_usb_send_leave_woble_suspend_cmd(void)
{
	int ret = 0;	/* if successful, 0 */
	u8 cmd[] = { 0xC9, 0xFC, 0x05, 0x01, 0x21, 0x02, 0x00, 0x00 };

	BTUSB_INFO("%s", __func__);
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	if (ret) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}
	return ret;
}

static int btmtk_usb_handle_entering_WoBLE_state(void)
{
	int ret = -1;
	u8 status_event[] = { 0x0f, 0x04, 0x00, 0x01, 0xC9, 0xFC };
	u8 comp_event[] = { 0xe6, 0x02, 0x08, 0x00 };
	int i = 0;

	BTUSB_INFO("%s", __func__);

	/* Power on first if state is power off */
	ret = btmtk_usb_reset_power_on();
	if (ret) {
		BTUSB_INFO("%s: reset power_on fail return", __func__);
		goto Finish;
	}

	if (is_support_unify_woble(g_data)) {
		btmtk_usb_le_set_scan_parm(FALSE);

		ret = btmtk_usb_send_get_vendor_cap();
		if (ret)
			goto Finish;
		ret = btmtk_usb_send_read_BDADDR_cmd();
		if (ret)
			goto Finish;
		ret = btmtk_usb_set_Woble_APCF();
		if (ret)
			goto Finish;

		BTUSB_INFO("%s: woble_setting_radio_off[0].length %d", __func__,
				g_data->woble_setting_radio_off[0].length);
		if (g_data->woble_setting_radio_off[0].length && is_support_unify_woble(g_data)) {
			/* start to send radio off cmd from woble setting file */
			for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
				if (!g_data->woble_setting_radio_off[i].length)
					continue;

				BTUSB_INFO_RAW(g_data->woble_setting_radio_off[i].content,
					g_data->woble_setting_radio_off[i].length, "send radio off %d:", i);

				ret = btmtk_usb_send_hci_cmd(
					g_data->woble_setting_radio_off[i].content,
					g_data->woble_setting_radio_off[i].length,
					NULL, -1);
				if (ret != 0) {
					BTUSB_ERR("%s: btmtk_usb_send_hci_cmd return fail %d", __func__, ret);
					goto Finish;
				}

				if (g_data->woble_setting_radio_off_status_event[i].length) {
					BTUSB_INFO("%s: check %d adio_off_status_event", __func__, i);
					ret = btmtk_usb_wait_until_event(
							g_data->woble_setting_radio_off_status_event[i].content,
							g_data->woble_setting_radio_off_status_event[i].length,
							USB_INTR_MSG_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
					if (ret) {
						BTUSB_ERR("%s: radio_off_status_event %d return error",
							__func__, i);
						goto Finish;
					}
				}

				if (g_data->woble_setting_radio_off_comp_event[i].length) {
					BTUSB_INFO("%s: check %d radio_off_comp_event", __func__, i);
					ret = btmtk_usb_wait_until_event(
						g_data->woble_setting_radio_off_comp_event[i].content,
						g_data->woble_setting_radio_off_comp_event[i].length,
						USB_INTR_MSG_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 1);
					if (ret) {
						BTUSB_ERR("%s: radio_off_status_event %d return error",
							__func__, i);
						goto Finish;
					}
				}

			}
		} else if (is_support_unify_woble(g_data)) { /* use default */
			BTUSB_INFO("%s: use default radio off cmd", __func__);
			ret = btmtk_usb_send_unify_woble_suspend_cmd();
			if (ret) {
				BTUSB_ERR("%s: btmtk_usb_send_unify_woble_suspend_cmd return error", __func__);
				goto Finish;
			}

			ret = btmtk_usb_wait_until_event(status_event, sizeof(status_event),
				USB_INTR_MSG_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
			if (ret) {
				BTUSB_ERR("%s: btmtk_usb_wait_until_event status_event return error", __func__);
				goto Finish;
			}

			ret = btmtk_usb_wait_until_event(comp_event, sizeof(comp_event),
					WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 1);
			if (ret) {
				BTUSB_ERR("%s: btmtk_usb_wait_until_event comp_event return error", __func__);
				goto Finish;
			}
		} else
			BTUSB_ERR("%s: controller don't support unify woble, don't set radio off cmd",
					__func__);
	} else {
		ret = btmtk_usb_send_woble_suspend_cmd();
		BTUSB_INFO("%s: end", __func__);
		goto Finish;
	}

Finish:
	if (ret) {
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
		btmtk_usb_woble_wake_lock(g_data);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_WOBLE, 1, "count",
				"enter-woble-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_WOBLE, 1, "count",
				"enter-woble-fail", VITALS_NORMAL);
#endif

	} else
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_WOBLE;

	return ret;
}

/*============================================================================*/
/* Internal Functions : Chip Related */
/*============================================================================*/
#define ______________________________________Internal_Functions_Chip_Related
/**
 * Only for load rom patch function, tmp_str[15] is '\n'
 */
#define SHOW_FW_DETAILS(s)							\
	BTUSB_INFO("%s: %s = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", __func__, s,	\
			tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],		\
			tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],		\
			tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],	\
			tmp_str[12], tmp_str[13], tmp_str[14]/*, tmp_str[15]*/)

#if SUPPORT_MT7662
static int btmtk_usb_load_rom_patch_7662(void)
{
	u32 loop = 0;
	u32 value;
	s32 sent_len;
	int ret = 0;
	u16 total_checksum = 0;
	struct urb *urb;
	u32 patch_len = 0;
	u32 cur_len = 0;
	dma_addr_t data_dma;
	struct completion sent_to_mcu_done;
	int first_block = 1;
	u8 phase;
	void *buf;
	u8 *pos;
	char *tmp_str;
	unsigned int pipe = usb_sndbulkpipe(g_data->udev,
						g_data->
						bulk_tx_ep->bEndpointAddress);

load_patch_protect:
	btmtk_usb_switch_iobase_7662(WLAN);
	btmtk_usb_io_read32_7662(SEMAPHORE_03, &value);
	loop++;

	if ((value & 0x01) == 0x00) {
		if (loop < 1000) {
			mdelay(1);
			goto load_patch_protect;
		} else {
			BTUSB_WARN("%s: WARNING! Can't get semaphore! Continue", __func__);
		}
	}

	btmtk_usb_switch_iobase_7662(SYSCTL);

	btmtk_usb_io_write32_7662(0x1c, 0x30);

	btmtk_usb_switch_iobase_7662(WLAN);

	urb = usb_alloc_urb(0, GFP_KERNEL);

	if (!urb) {
		ret = -ENOMEM;
		goto error0;
	}

	buf = usb_alloc_coherent(g_data->udev, UPLOAD_PATCH_UNIT, GFP_KERNEL, &data_dma);
	if (!buf) {
		ret = -ENOMEM;
		goto error1;
	}

	pos = buf;
	btmtk_usb_load_code_from_bin(&g_data->rom_patch,
					 g_data->rom_patch_bin_file_name,
					 &g_data->udev->dev,
					 &g_data->rom_patch_len);

	if (!g_data->rom_patch) {
		BTUSB_ERR("%s: please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)",
				__func__, g_data->rom_patch_bin_file_name,
				 g_data->rom_patch_bin_file_name);

		ret = -1;
		goto error2;
	}

	tmp_str = g_data->rom_patch;
	memcpy(fw_version_str, g_data->rom_patch, FW_VERSION_BUF_SIZE);
	SHOW_FW_DETAILS("FW Version");
	SHOW_FW_DETAILS("build Time");

	/* check ROM patch if upgrade */
	btmtk_usb_io_read32_7662(CLOCK_CTL, &value);
	if ((value & 0x01) == 0x01) {
		BTUSB_INFO("%s: no need to load rom patch", __func__);
		if (!is_mt7662T(g_data))
			btmtk_usb_send_dummy_bulk_out_packet_7662();
		goto error2;
	}

	tmp_str = g_data->rom_patch + 16;
	BTUSB_INFO("%s: platform = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = g_data->rom_patch + 20;
	BTUSB_INFO("%s: HW/SW version = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = g_data->rom_patch + 24;
	BTUSB_INFO("%s: Patch version = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	do {
		typedef void (*pdwnc_func) (u8 fgReset);
		char *pdwnc_func_name = "PDWNC_SetBTInResetState";
		pdwnc_func pdwndFunc = NULL;

		pdwndFunc = (pdwnc_func) kallsyms_lookup_name(pdwnc_func_name);

		if (pdwndFunc) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, pdwnc_func_name, 1);
			pdwndFunc(1);
		} else {
			BTUSB_WARN("%s: No Exported Func Found [%s]", __func__, pdwnc_func_name);
		}
	} while (0);

	BTUSB_INFO("%s: rom patch %s loading...", __func__, g_data->rom_patch_bin_file_name);

	init_completion(&sent_to_mcu_done);

	cur_len = 0x00;
	patch_len = g_data->rom_patch_len - PATCH_INFO_SIZE;
	BTUSB_INFO("%s: patch_len = %d", __func__, patch_len);

	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;
		static u8 current_phase;

		sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				if (patch_len - cur_len == sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}
			if (phase != current_phase) {
				BTUSB_INFO("%s: cur_len = %d, phase = %d", __func__, cur_len, phase);
				current_phase = phase;
			}

			/* prepare HCI header */
			pos[0] = 0x6F;
			pos[1] = 0xFC;
			pos[2] = (sent_len + 5) & 0xFF;
			pos[3] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			pos[4] = 0x01;
			pos[5] = 0x01;
			pos[6] = (sent_len + 1) & 0xFF;
			pos[7] = ((sent_len + 1) >> 8) & 0xFF;

			pos[8] = phase;

			memcpy(&pos[9], g_data->rom_patch + PATCH_INFO_SIZE + cur_len, sent_len);

			BTUSB_DBG("%s: sent_len = %d, cur_len = %d, phase = %d", __func__, sent_len, cur_len, phase);

			usb_fill_bulk_urb(urb,
					g_data->udev,
					pipe,
					buf,
					sent_len + PATCH_HEADER_SIZE,
					(usb_complete_t)btmtk_usb_load_rom_patch_complete,
					&sent_to_mcu_done);

			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			ret = usb_submit_urb(urb, GFP_KERNEL);

			if (ret)
				goto error2;

			if (!wait_for_completion_timeout
				(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				BTUSB_ERR("%s: upload rom_patch timeout", __func__);
				ret = -ETIME;
				goto error2;
			}

			cur_len += sent_len;
			mdelay(1);

		} else {
			BTUSB_INFO("%s: loading rom patch... Done", __func__);
			break;
		}
	}

	mdelay(20);
	ret = btmtk_usb_get_rom_patch_result();
	mdelay(1);

	/* Send Checksum request */
	total_checksum = btmtk_usb_checksum16_7662(g_data->rom_patch + PATCH_INFO_SIZE, patch_len);
	btmtk_usb_chk_crc_7662(patch_len);

	mdelay(1);

	if (total_checksum != btmtk_usb_get_crc_7662()) {
		BTUSB_ERR("checksum fail!, local(0x%x) <> fw(0x%x)", total_checksum, btmtk_usb_get_crc_7662());
		ret = -1;
		goto error2;
	}

	/* send check rom patch result request */
	mdelay(1);
	btmtk_usb_send_hci_check_rom_patch_result_cmd_7662();

	/* CHIP_RESET */
	mdelay(1);
	ret = btmtk_usb_send_wmt_reset_cmd();

	/* BT_RESET */
	mdelay(1);
	btmtk_usb_send_hci_reset_cmd();

	/* Enable BT Low Power */
	mdelay(1);
	btmtk_usb_send_hci_low_power_cmd_7662(TRUE);

	/* for WoBLE/WoW low power */
	mdelay(1);
	btmtk_usb_send_hci_set_ce_cmd_7662();

	mdelay(1);
	btmtk_usb_send_hci_set_tx_power_cmd_7662();

error2:
	usb_free_coherent(g_data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
error1:
	usb_free_urb(urb);
error0:
	btmtk_usb_io_write32_7662(SEMAPHORE_03, 0x1);

	do {
		typedef void (*pdwnc_func) (u8 fgReset);
		char *pdwnc_func_name = "PDWNC_SetBTInResetState";
		pdwnc_func pdwndFunc = NULL;

		pdwndFunc = (pdwnc_func) kallsyms_lookup_name(pdwnc_func_name);

		if (pdwndFunc) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, pdwnc_func_name, 0);
			pdwndFunc(0);
		} else
			BTUSB_WARN("%s: No Exported Func Found [%s]", __func__, pdwnc_func_name);
	} while (0);

	return ret;
}

static int btmtk_usb_io_read32_7662(u32 reg, u32 *val)
{
	int ret = -1;
	u8 request = g_data->r_request;

	memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
	ret = usb_control_msg(g_data->udev, usb_rcvctrlpipe(g_data->udev, 0),
			request, DEVICE_VENDOR_REQUEST_IN, 0x0, reg,
			g_data->io_buf, sizeof(u32), USB_CTRL_IO_TIMO);

	if (ret < 0) {
		*val = 0xffffffff;
		BTUSB_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, *val);
		return ret;
	}

	memmove(val, g_data->io_buf, sizeof(u32));
	*val = le32_to_cpu(*val);

	return 0;
}

static int btmtk_usb_io_write32_7662(u32 reg, u32 val)
{
	int ret;
	u16 value, index;
	u8 request = g_data->w_request;

	index = (u16) reg;
	value = val & 0x0000ffff;

	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				request, DEVICE_VENDOR_REQUEST_OUT, value, index,
				NULL, 0, USB_CTRL_IO_TIMO);

	if (ret < 0) {
		BTUSB_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, val);
		return ret;
	}

	index = (u16) (reg + 2);
	value = (val & 0xffff0000) >> 16;

	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				request, DEVICE_VENDOR_REQUEST_OUT, value, index,
				NULL, 0, USB_CTRL_IO_TIMO);

	if (ret < 0) {
		BTUSB_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, val);
		return ret;
	}

	if (ret > 0)
		ret = 0;

	return ret;
}

static int btmtk_usb_switch_iobase_7662(int base)
{
	int ret = 0;

	switch (base) {
	case SYSCTL:
		g_data->w_request = 0x42;
		g_data->r_request = 0x47;
		break;
	case WLAN:
		g_data->w_request = 0x02;
		g_data->r_request = 0x07;
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

static u16 btmtk_usb_checksum16_7662(u8 *pData, int len)
{
	u32 sum = 0;

	while (len > 1) {
		sum += *((u16 *) pData);

		pData = pData + 2;

		if (sum & 0x80000000)
			sum = (sum & 0xFFFF) + (sum >> 16);

		len -= 2;
	}

	if (len)
		sum += *((u8 *) pData);

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return (u16)(~sum);
}

static int btmtk_usb_chk_crc_7662(u32 checksum_len)
{
	int ret = 0;

	BT_DBG("%s", __func__);

	memmove(g_data->io_buf, &g_data->rom_patch_offset, 4);
	memmove(&g_data->io_buf[4], &checksum_len, 4);

	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0), 0x1,
				DEVICE_VENDOR_REQUEST_OUT, 0x20, 0x00,
				g_data->io_buf, 8, USB_CTRL_IO_TIMO);

	if (ret < 0)
		BTUSB_ERR("%s: error(%d)", __func__, ret);

	return ret;
}

static u16 btmtk_usb_get_crc_7662(void)
{
	int ret = 0;
	u16 crc, count = 0;

	while (1) {
		ret = usb_control_msg(g_data->udev,
					usb_rcvctrlpipe(g_data->udev, 0), 0x01,
					DEVICE_VENDOR_REQUEST_IN, 0x21, 0x00,
					g_data->io_buf, USB_IO_BUF_SIZE,
					USB_CTRL_IO_TIMO);

		if (ret < 0) {
			crc = 0xFFFF;
			BTUSB_ERR("%s: error(%d)", __func__, ret);
		}

		memmove(&crc, g_data->io_buf, 2);

		crc = le16_to_cpu(crc);

		if (crc != 0xFFFF)
			break;

		mdelay(1);

		if (count++ > 100) {
			BTUSB_WARN("Query CRC over %d times", count);
			break;
		}
	}

	return crc;
}

static int btmtk_usb_send_hci_set_tx_power_cmd_7662(void)
{
	u8 cmd[] = { 0x79, 0xFC, 0x06, 0x07, 0x80, 0x00, 0x06, 0x07, 0x07 };
	u8 event[] = { 0x0E, 0x04, 0x01, 0x79, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_hci_check_rom_patch_result_cmd_7662(void)
{
	u8 cmd[] = { 0xD1, 0xFC, 0x04, 0x00, 0xE2, 0x40, 0x00 };
	u8 event[] = { 0x0E, 0x08, 0x01, 0xD1, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);

	} else if (ret == sizeof(event) + 4) {
		if (g_data->io_buf[6] == 0 &&
			g_data->io_buf[7] == 0 &&
			g_data->io_buf[8] == 0 &&
			g_data->io_buf[9] == 0) {
			BTUSB_WARN("Check rom patch result: NG");
			return -1;

		} else {
			BTUSB_INFO("Check rom patch result: OK");
			ret = 0;
		}

	} else {
		BTUSB_ERR("%s: failed, incorrect response length(%d)", __func__, ret);
		return -1;
	}

	return ret;
}

static int btmtk_usb_send_hci_radio_on_cmd_7662(void)
{
	u8 cmd[] = { 0xC9, 0xFC, 0x02, 0x01, 0x01 };
	u8 event[] = { 0xE6, 0x02, 0x08, 0x01 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_hci_set_ce_cmd_7662(void)
{
	u8 cmd[] = { 0xD1, 0xFC, 0x04, 0x0C, 0x07, 0x41, 0x00 };
	u8 event[] = { 0x0E, 0x08, 0x01, 0xD1, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);

	} else if (ret == sizeof(event) + 4) {
		if (g_data->io_buf[6] & 0x01) {
			BTUSB_WARN("warning, 0x41070c[0] is 1!");
			ret = 0;
		} else {
			/* write 0x41070C[0] to 1 */
			u8 cmd2[11] = { 0xD0, 0xFC, 0x08, 0x0C, 0x07, 0x41, 0x00 };

			cmd2[7] = g_data->io_buf[6] | 0x01;
			cmd2[8] = g_data->io_buf[7];
			cmd2[9] = g_data->io_buf[8];
			cmd2[10] = g_data->io_buf[9];

			ret = btmtk_usb_send_hci_cmd(cmd2, sizeof(cmd2), NULL, 0);
			if (ret < 0) {
				BTUSB_ERR("%s: write 0x41070C failed(%d)", __func__, ret);
			} else {
				BTUSB_INFO("%s: OK", __func__);
				ret = 0;
			}
		}
	} else {
		BTUSB_ERR("%s: failed, incorrect response length(%d)", __func__, ret);
		return -1;
	}

	return ret;
}

static int btmtk_usb_send_hci_radio_off_cmd_7662(void)
{
	u8 cmd[] = { 0xC9, 0xFC, 0x02, 0x01, 0x00 };
	u8 event[] = { 0xE6, 0x02, 0x08, 0x00 };	/* unexpected opcode */
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_hci_low_power_cmd_7662(bool enable)
{
	/* default for disable */
	u8 cmd[] = { 0x40, 0xFC, 0x00 };
	u8 event[] = { 0x0E, 0x04, 0x01, 0x40, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	/* for enable */
	if (enable == TRUE) {
		cmd[0] = 0x41;
		event[3] = 0x41;
	}
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static void btmtk_usb_send_dummy_bulk_out_packet_7662(void)
{
	int ret = 0;
	int actual_len;
	unsigned int pipe;

	pipe = usb_sndbulkpipe(g_data->udev, g_data->bulk_tx_ep->bEndpointAddress);
	memset(g_data->o_usb_buf, 0, 8);
	ret = usb_bulk_msg(g_data->udev, pipe, g_data->o_usb_buf, 8, &actual_len, 100);
	if (ret)
		BTUSB_ERR("%s: submit dummy bulk out failed!", __func__);
	else
		BTUSB_INFO("%s: 1. OK", __func__);

	memset(g_data->o_usb_buf, 0, 8);
	ret = usb_bulk_msg(g_data->udev, pipe, g_data->o_usb_buf, 8, &actual_len, 100);
	if (ret)
		BTUSB_ERR("%s: submit dummy bulk out failed!", __func__);
	else
		BTUSB_INFO("%s: 2. OK", __func__);
}
#endif /* SUPPORT_MT7662 */

#if SUPPORT_MT7668
static int btmtk_usb_io_read32_7668(u32 reg, u32 *val)
{
	int ret = -1;
	__le16 reg_high;
	__le16 reg_low;

	reg_high = ((reg >> 16) & 0xffff);
	reg_low = (reg & 0xffff);

	memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
	ret = usb_control_msg(g_data->udev, usb_rcvctrlpipe(g_data->udev, 0),
			0x63,				/* bRequest */
			DEVICE_VENDOR_REQUEST_IN,	/* bRequestType */
			reg_high,			/* wValue */
			reg_low,			/* wIndex */
			g_data->io_buf,
			sizeof(u32), USB_CTRL_IO_TIMO);

	if (ret < 0) {
		*val = 0xffffffff;
		BTUSB_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, *val);
		return ret;
	}

	memmove(val, g_data->io_buf, sizeof(u32));
	*val = le32_to_cpu(*val);

	return 0;
}

static int btmtk_usb_check_need_load_rom_patch_7668(void)
{
	/* TRUE: need load patch., FALSE: do not need */
	u8 cmd[] = { 0x6F, 0xFC, 0x05, 0x01, 0x17, 0x01, 0x00, 0x01 };
	u8 event[] = { 0xE4, 0x05, 0x02, 0x17, 0x01, 0x00, /* 0x02 */ };	/* event[6] is key */
	int ret = -1;

	BTUSB_DBG_RAW(cmd, sizeof(cmd), "%s: Send CMD:", __func__);
	ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20, 20);
	/* can't get correct event */
	if (ret < 0)
		return PATCH_ERR;

	if (ret == sizeof(event) + 1)
		return g_data->io_buf[6];

	return PATCH_ERR;
}

static int btmtk_usb_check_bt_power_status_7668(void)
{
	/* TRUE: ON, FALSE: OFF */
	u8 cmd[] = { 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x04, 0x00 };
	u8 event[] = { 0xE4, 0x07, 0x02, 0x06, 0x03, 0x00, 0x00, 0x04 };	/* event[8] is key */
	int ret = -1;
	int retry = 0;

	do {
		ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 100, 20);
		/* get bt power status failed */
		if (ret < 0)
			return ret;

		if (ret == sizeof(event) + 1) {
			BTUSB_INFO("%s: %02X", __func__, g_data->io_buf[8]);
			/* Check the bit5 firstly for calibration before power status., bit5 since Bora CL58715 */
			if (g_data->io_buf[8] & 0x20) {		/* bit5 1:calibrating, 0:ok*/
				msleep(200);
				BTUSB_WARN("%s: Calibrating check again", __func__);
				continue;
			} else if (g_data->io_buf[8] & 0x04) {	/* bit3 1:BT on, 0:BT off */
				return 1;
			} else {
				return 0;
			}
		} else {
			BTUSB_WARN("%s: got unknown result (%d)", __func__, ret);
			BTUSB_DBG_RAW(g_data->io_buf, ret, "%s:", __func__);
			return -1;
		}
	} while (retry++ < 5);
	BTUSB_ERR("%s: Calibrating check fail", __func__);
	return -1;
}

static int btmtk_usb_load_rom_patch_7668(void)
{
	int ret = 0;
	u32 patch_len = 0;
	char *tmp_str;
	bool load_sysram3 = false;
	int patch_status = 0;
	int retry = 20;

	BTUSB_INFO("%s: begin", __func__);

	btmtk_usb_load_code_from_bin(&g_data->rom_patch,
			g_data->rom_patch_bin_file_name, &g_data->udev->dev,
			&g_data->rom_patch_len);
	if (!g_data->rom_patch) {
		BTUSB_ERR("%s: please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)",
				 __func__, g_data->rom_patch_bin_file_name,
				 g_data->rom_patch_bin_file_name);
		ret = -1;
		goto patch_end;
	}

	tmp_str = g_data->rom_patch;
	SHOW_FW_DETAILS("FW Version");
	SHOW_FW_DETAILS("build Time");
	memset(fw_version_str, 0, FW_VERSION_BUF_SIZE);
	if (tmp_str[8] >= '0' && tmp_str[8] <= '9')
		memcpy(fw_version_str, tmp_str, FW_VERSION_SIZE - 1);
	else
		sprintf(fw_version_str, "%.4s-%.2s-%.2s.%.1s.%.2s.%.1s.%.1s.%.2s",
				tmp_str, tmp_str + 4, tmp_str + 6, tmp_str + 8,
				tmp_str + 9, tmp_str + 11, tmp_str + 12, tmp_str + 13);

#if SUPPORT_MT7668
	if (is_mt7668(g_data))
		load_sysram3 = (g_data->rom_patch_len > (PATCH_INFO_SIZE + PATCH_LEN_ILM)) ? true : false;
#endif

	do {
		patch_status = btmtk_usb_check_need_load_rom_patch_7668();
		BTUSB_DBG("%s: patch_status %d", __func__, patch_status);

		if (patch_status > PATCH_NEED_DOWNLOAD || patch_status == PATCH_ERR) {
			BTUSB_ERR("%s: patch_status error", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		    log_counter_to_vitals_v2(ANDROID_LOG_INFO,
					BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
					BT_OPERATION, BT_KEY_PROBE, 1, "count",
					"patch-status-error", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
			log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
					BT_OPERATION, BT_KEY_PROBE, 1, "count",
					"patch-status-error", VITALS_NORMAL);
#endif
			return -1;
		} else if (patch_status == PATCH_READY) {
			BTUSB_INFO("%s: no need to load rom patch", __func__);
			if (!load_sysram3)
				goto patch_end;
			else
				goto sysram3;
		} else if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
			msleep(100);
			retry--;
		} else if (patch_status == PATCH_NEED_DOWNLOAD) {
			break;  /* Download ROM patch directly */
		}
	} while (retry > 0);

	if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
		BTUSB_WARN("%s: Hold by another fun more than 2 seconds", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"hold-by-others", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"hold-by-others", VITALS_NORMAL);
#endif
		return -1;
	}

	tmp_str = g_data->rom_patch + 16;
	BTUSB_INFO("%s: platform = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = g_data->rom_patch + 20;
	BTUSB_INFO("%s: HW/SW version = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = g_data->rom_patch + 24;

	BTUSB_INFO("loading rom patch...");

	BTUSB_INFO("%s: loading ILM rom patch...", __func__);
	patch_len = load_sysram3 ? PATCH_LEN_ILM : (g_data->rom_patch_len - PATCH_INFO_SIZE);
	ret = btmtk_usb_load_partial_rom_patch_7668(patch_len, PATCH_INFO_SIZE);
	if (ret) {
		BTUSB_WARN("%s: loading ILM rom patch... Fail", __func__);
		goto patch_end;
	}
	BTUSB_INFO("%s: loading ILM rom patch... Done", __func__);

	/* CHIP_RESET, ROM patch would be reactivated.
	 * Currently, wmt reset is only for ILM rom patch, and there are also
	 * some preparations need to be done in FW for loading sysram3 patch...
	 */
	ret = btmtk_usb_send_wmt_reset_cmd();
	if (ret) {
		BTUSB_WARN("%s: send wmt reset cmd... Fail", __func__);
		goto patch_end;
	}
	g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_OFF;

sysram3:
	if (load_sysram3) {
		BTUSB_INFO("%s: loading sysram3 rom patch...", __func__);
		patch_len = g_data->rom_patch_len - PATCH_INFO_SIZE - PATCH_LEN_ILM - PATCH_INFO_SIZE;
		ret = btmtk_usb_load_partial_rom_patch_7668(patch_len,
				PATCH_INFO_SIZE + PATCH_LEN_ILM + PATCH_INFO_SIZE);
		BTUSB_INFO("%s: loading sysram3 rom patch... Done", __func__);
	}

patch_end:
	BTUSB_INFO("%s: end", __func__);
	return ret;
}

static int btmtk_usb_load_partial_rom_patch_7668(u32 patch_len, int offset)
{
	s32 sent_len;
	int ret = 0;
	struct urb *urb;
	u32 cur_len = 0;
	dma_addr_t data_dma;
	struct completion sent_to_mcu_done;
	int first_block = 1;
	u8 phase;
	void *buf;
	u8 *pos;
	unsigned int pipe = usb_sndbulkpipe(g_data->udev, g_data->bulk_tx_ep->bEndpointAddress);

	BTUSB_INFO("%s: begin", __func__);

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"urb-alloc-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"urb-alloc-fail", VITALS_NORMAL);
#endif
		ret = -ENOMEM;
		goto error0;
	}

	buf = usb_alloc_coherent(g_data->udev, UPLOAD_PATCH_UNIT, GFP_KERNEL, &data_dma);
	if (!buf) {
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"usb-buf-alloc-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"usb-buf-alloc-fail", VITALS_NORMAL);
#endif
		ret = -ENOMEM;
		goto error1;
	}

	pos = buf;

	init_completion(&sent_to_mcu_done);

	BTUSB_INFO("%s: patch_len = %d", __func__, patch_len);

	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;
		int status = -1;

		sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				if (patch_len - cur_len == sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}

			/* prepare HCI header */
			pos[0] = 0x6F;
			pos[1] = 0xFC;
			pos[2] = (sent_len + 5) & 0xFF;
			pos[3] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			pos[4] = 0x01;
			pos[5] = 0x01;
			pos[6] = (sent_len + 1) & 0xFF;
			pos[7] = ((sent_len + 1) >> 8) & 0xFF;

			pos[8] = phase;

			memcpy(&pos[9], g_data->rom_patch + offset + cur_len,
					sent_len);

			BTUSB_DBG("%s: sent_len = %d, cur_len = %d, phase = %d", __func__, sent_len, cur_len, phase);

			usb_fill_bulk_urb(urb,
					g_data->udev,
					pipe,
					buf,
					sent_len + PATCH_HEADER_SIZE,
					(usb_complete_t)btmtk_usb_load_rom_patch_complete,
					&sent_to_mcu_done);

			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			status = usb_submit_urb(urb, GFP_KERNEL);
			if (status) {
				BTUSB_ERR("%s: submit urb failed (%d)", __func__, status);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
				log_counter_to_vitals_v2(ANDROID_LOG_INFO,
						BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
						BT_OPERATION, BT_KEY_PROBE, 1, "count",
						"urb-submit-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
				log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
						BT_OPERATION, BT_KEY_PROBE, 1, "count",
						"urb-submit-fail", VITALS_NORMAL);
#endif
				ret = status;
				goto error2;
			}

			if (!wait_for_completion_timeout
				(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				BTUSB_ERR("%s: upload rom_patch timeout", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
				log_counter_to_vitals_v2(ANDROID_LOG_INFO,
						BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
						BT_OPERATION, BT_KEY_PROBE, 1, "count",
						"upload-patch-timeout", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
				log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
						BT_OPERATION, BT_KEY_PROBE, 1, "count",
						"upload-patch-timeout", VITALS_NORMAL);
#endif
				ret = -ETIME;
				goto error2;
			}

			cur_len += sent_len;

			mdelay(1);
			if (btmtk_usb_get_rom_patch_result() < 0)
				goto error2;
			mdelay(1);

		} else {
			break;
		}
	}
error2:
	usb_free_coherent(g_data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
error1:
	usb_free_urb(urb);
error0:
	BTUSB_INFO("%s: end", __func__);
	return ret;
}

static int btmtk_usb_send_hci_tci_set_sleep_cmd_7668(void)
{
	u8 cmd[] = { 0x7A, 0xFC, 0x07, 0x05, 0x40, 0x06, 0x40, 0x06, 0x00, 0x00 };
	u8 event[] = { 0x0E, 0x04, 0x01, 0x7A, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_wmt_power_on_cmd_7668(void)
{
	u8 count = 0;	/* retry 3 times */
	u8 cmd[] = { 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x01 };
	u8 event[] = { 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00 };	/* event[6] is key */
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_check_bt_power_status_7668();
	if (ret < 0) {
		BTUSB_ERR("%s: get bt power status failed!", __func__);
		return ret;
	} else if (ret == 1) {
		BTUSB_WARN("%s: dongle is already on, no need to send wmt power on again!", __func__);
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_ON;
		return 0;
	}

	g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWERING_ON;
	do {
		BTUSB_INFO("%s: begin", __func__);
		ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 100, 20);
		if (ret < 0) {
			BTUSB_ERR("%s: failed(%d)", __func__, ret);
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;

		} else if (ret == sizeof(event) + 1) {
			switch (g_data->io_buf[6]) {
			case 0:			 /* successful */
				BTUSB_INFO("%s: OK", __func__);
				g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_ON;
				ret = 0;
				break;
			case 2:			 /* retry */
				BTUSB_INFO("%s: Try again", __func__);
				msleep(100);
				continue;
			default:
				BTUSB_WARN("%s: Unknown result: %02X", __func__, g_data->io_buf[6]);
				g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
				return -1;
			}

		} else {
			BTUSB_ERR("%s: failed, incorrect response length(%d)", __func__, ret);
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
			return -1;
		}
	} while (++count < 5 && ret > 0);

	return ret;
}

static int btmtk_usb_send_wmt_power_off_cmd_7668(void)
{
	u8 cmd[] = { 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x00 };
	u8 event[] = { 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_check_bt_power_status_7668();
	if (ret < 0) {
		BTUSB_ERR("%s: get bt power status failed!", __func__);
		return ret;
	} else if (ret == 0) {
		BTUSB_WARN("%s: dongle is already off, no need to send wmt power off again!", __func__);
		return 0;
	}

	BTUSB_INFO("%s: begin", __func__);
	g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWERING_OFF;
	ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20, 20);
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
		return ret;
	}

	BTUSB_INFO("%s: OK", __func__);
	g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_OFF;
	return 0;
}
#endif /* SUPPORT_MT7668 */

static void btmtk_usb_cap_init(void)
{
#if SUPPORT_MT7662
	btmtk_usb_switch_iobase_7662(WLAN);
	btmtk_usb_io_read32_7662(0x00, &g_data->chip_id);

	if (g_data->chip_id) {
		BTUSB_INFO("%s: chip id = %x", __func__, g_data->chip_id);
		if (is_mt7662T(g_data)) {
			BTUSB_INFO("%s: This is MT7662T chip", __func__);
			if (g_data->rom_patch_bin_file_name) {
				memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
				memcpy(g_data->rom_patch_bin_file_name, "mt7662t_patch_e1_hdr.bin", 24);
			}
			g_data->rom_patch_offset = 0xBC000;
			g_data->rom_patch_len = 0;
			return;
		} else if (is_mt7662(g_data)) {
			BTUSB_INFO("%s: This is MT7662U chip", __func__);
			if (g_data->rom_patch_bin_file_name) {
				memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
				memcpy(g_data->rom_patch_bin_file_name, "mt7662_patch_e3_hdr.bin", 23);
			}
			g_data->rom_patch_offset = 0x90000;
			g_data->rom_patch_len = 0;
			return;
		}
	}
#endif /* SUPPORT_MT7662 */

	btmtk_usb_io_read32_7668(0x80000008, &g_data->chip_id);
	if (g_data->chip_id) {
		unsigned int fw_version = 0;

		btmtk_usb_io_read32_7668(0x80000004, &fw_version);
		BTUSB_INFO("%s: Chip.ID = 0x%04x, FW.Ver:0x%x", __func__, g_data->chip_id, fw_version);

		memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
		if ((fw_version & 0xff) == 0xff) {
			BTUSB_ERR("%s: failed, wrong FW version : 0x%x !", __func__, fw_version);
			return;
		}

		/* Bin filename format : "mt$$$$_patch_e%.bin" */
		/*     $$$$ : chip id */
		/*     % : fw version + 1 (in HEX) */
		snprintf(g_data->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN, "mt%04x_patch_e%x_hdr.bin",
				g_data->chip_id & 0xffff, (fw_version & 0x0ff) + 1);
		BTUSB_INFO("%s: rom patch file name is %s", __func__, g_data->rom_patch_bin_file_name);
		g_data->rom_patch_len = 0;

#if SUPPORT_MT7668
		if (is_mt7668(g_data)) {
			memcpy(g_data->woble_setting_file_name, WOBLE_SETTING_FILE_NAME,
					sizeof(WOBLE_SETTING_FILE_NAME));
			BTUSB_INFO("%s: woble setting file name is %s", __func__, g_data->woble_setting_file_name);
		}
#endif
	}
}

static int btmtk_usb_load_rom_patch(void)
{
	int err = -1;

	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data is NULL !", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"g-data-null", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"g-data-null", VITALS_NORMAL);
#endif
		return err;
	}

#if SUPPORT_MT7662
	if (is_mt7662(g_data))
		return btmtk_usb_load_rom_patch_7662();
#endif

#if SUPPORT_MT7668
	if (is_mt7668(g_data))
		return btmtk_usb_load_rom_patch_7668();
#endif

	BTUSB_WARN("%s: unknown chip id (%d)", __func__, g_data->chip_id);
	return err;
}

/*============================================================================*/
/* Internal Functions : Send HCI/WMT */
/*============================================================================*/
#define ______________________________________Internal_Functions_Send_HCI_WMT
static int btmtk_usb_send_woble_suspend_cmd(void)
{
	int ret = 0;	/* if successful, 0 */
#if SUPPORT_LEGACY_WOBLE
	if (need_reset_stack == HW_ERR_NONE) {
		BTUSB_INFO("%s: set need_reset_stack = %d",
				__func__, HW_ERR_CODE_LEGACY_WOBLE);
		need_reset_stack = HW_ERR_CODE_LEGACY_WOBLE;
	}
#if BT_RC_VENDOR_DEFAULT
	do {
		u8 cmd[] = { 0xC9, 0xFC, 0x0D, 0x01, 0x0E, 0x00, 0x05, 0x43,
				0x52, 0x4B, 0x54, 0x4D, 0x20, 0x04, 0x32, 0x00 };

		BTUSB_INFO("%s: BT_RC_VENDOR_T0 or Default", __func__);
		ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	} while (0);
#elif BT_RC_VENDOR_S0
	do {
		u8 cmd[] = { 0xC9, 0xFC, 0x02, 0x01, 0x0B };

		BTUSB_INFO("%s: BT_RC_VENDOR_S0", __func__);
		ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	} while (0);
#endif
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}
#endif /* SUPPORT_LEGACY_WOBLE */
	return ret;
}

static int btmtk_usb_send_wmt_reset_cmd(void)
{
	u8 cmd[] = { 0x6F, 0xFC, 0x05, 0x01, 0x07, 0x01, 0x00, 0x04 };
	u8 event[] = { 0xE4, 0x05, 0x02, 0x07, 0x01, 0x00, 0x00 };
	int ret = -1;	/* if successful, 0 */

	BTUSB_INFO("%s", __func__);
	BTUSB_DBG_RAW(cmd, sizeof(cmd), "%s: Send CMD:", __func__);
	ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20, 20);
	if (ret < 0) {
		BTUSB_ERR("%s: Check reset wmt result: NG", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"wmt-reset-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"wmt-reset-fail", VITALS_NORMAL);
#endif
	} else {
		BTUSB_INFO("%s: Check reset wmt result: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_get_rom_patch_result(void)
{
	u8 event[] = { 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00 };
	int ret = -1;	/* if successful, 0 */

	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data == NULL!", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"g-data-null", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"g-data-null", VITALS_NORMAL);
#endif
		return -1;
	}
	if (g_data->udev == NULL) {
		BTUSB_ERR("%s: g_data->udev == NULL!", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"g-udev-null", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"g-udev-null", VITALS_NORMAL);
#endif
		return -1;
	}
	if (g_data->io_buf == NULL) {
		BTUSB_ERR("%s: g_data->io_buf == NULL!", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"io-buf-null", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"io-buf-null", VITALS_NORMAL);
#endif
		return -1;
	}

	memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
	ret = usb_control_msg(g_data->udev, usb_rcvctrlpipe(g_data->udev, 0),
				0x01, DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00,
				g_data->io_buf, USB_IO_BUF_SIZE,
				USB_CTRL_IO_TIMO);

	if (ret < 0)
		BTUSB_ERR("%s: error(%d)", __func__, ret);

	/* ret should be 16 bytes */
	if (ret >= sizeof(event) && !memcmp(g_data->io_buf, event, sizeof(event))) {
		BTUSB_DBG("Get rom patch result: OK");
		return 0;

	} else {
		BTUSB_WARN("Get rom patch result: NG");
		BTUSB_DBG_RAW(g_data->io_buf, ret, "%s: Get unknown event is:", __func__);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"patch-result-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"patch-result-fail", VITALS_NORMAL);
#endif
		return -1;
	}

	return ret;
}

static int btmtk_usb_send_hci_reset_cmd(void)
{
	u8 cmd[] = { 0x03, 0x0C, 0x00 };
	u8 event[] = { 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_assert_cmd_ctrl(void)
{
	u8 cmd[] = { 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
	int ret = -1;	/* if successful, 0 */

	BTUSB_DBG_RAW(cmd, sizeof(cmd), "%s: Send CMD:", __func__);
	/* Ask DO NOT read event */
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	if (ret < 0) {
		BTUSB_ERR("%s: error(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_assert_cmd_bulk(void)
{
	int ret = 0;
	int actual_length = 9;
	u8 buf[] = { 0x6F, 0xFC, 0x05, 0x00, 0x01, 0x02, 0x01, 0x00, 0x08 };

	BTUSB_DBG_RAW(buf, sizeof(buf), "%s: Send CMD:", __func__);
	memcpy(g_data->o_usb_buf, buf, sizeof(buf));

	ret = usb_bulk_msg(g_data->udev, usb_sndbulkpipe(g_data->udev, 2),
			g_data->o_usb_buf, sizeof(buf), &actual_length, 100);


	if (ret < 0) {
		BTUSB_ERR("%s: error(%d)", __func__, ret);
		return ret;
	}
	BTUSB_INFO("%s: OK", __func__);
	return 0;
}


static int btmtk_usb_unify_woble_wake_up(void)
{
	int ret = -1;
	int i = 0;
	u8 event_complete[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00 };

	BTUSB_INFO("%s: handle leave woble from file", __func__);

	ret = btmtk_usb_handle_leaving_WoBLE_state();
	if (ret) {
		BTUSB_ERR("%s: btmtk_usb_handle_leaving_WoBLE_state return fail %d", __func__, ret);
		goto resume_woble_done;
	}

	if (g_data->woble_setting_len) {
		if (g_data->woble_setting_apcf_resume[0].length) {
			BTUSB_INFO("%s: handle leave woble apcf from file", __func__);
			for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
				if (!g_data->woble_setting_apcf_resume[i].length)
					continue;

				BTUSB_INFO_RAW(g_data->woble_setting_apcf_resume[i].content,
					g_data->woble_setting_apcf_resume[i].length, "send radio on apcf ");

				ret = btmtk_usb_send_hci_cmd(
					g_data->woble_setting_apcf_resume[i].content,
					g_data->woble_setting_apcf_resume[i].length, NULL, -1);
				if (ret < 0) {
					BTUSB_ERR("%s: Send command fail %d", __func__, ret);
					goto resume_woble_done;
				}

				ret = btmtk_usb_wait_until_event(event_complete, sizeof(event_complete),
						WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
				if (ret < 0) {
					BTUSB_ERR("%s: btmtk_usb_wait_until_event error %d", __func__, ret);
					goto resume_woble_done;
				}
			}
		}

	} else { /* use default */
		BTUSB_WARN("%s: use default leave woble", __func__);
		ret = btmtk_usb_del_Woble_APCF_index();
		if (ret) {
			BTUSB_ERR("%s: btmtk_usb_del_Woble_APCF_index return fail %d", __func__, ret);
			goto resume_woble_done;
		}
	}

	btmtk_usb_le_set_scan_parm(TRUE);
	BTUSB_INFO("%s: handle leave woble end", __func__);

resume_woble_done:
	if (ret) {
		BTUSB_INFO("%s: woble_resume_fail!!!", __func__);
		ret = WOBLE_FAIL;
	} else {
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
		USB_MUTEX_UNLOCK();
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_ON;
		BTUSB_INFO("%s: success", __func__);
	}
	return ret;
}

static int btmtk_usb_unify_woble_suspend(struct btmtk_usb_data *data)
{
	int ret = -1;

	BTUSB_INFO("%s", __func__);
	ret = btmtk_usb_handle_entering_WoBLE_state();
	if (ret)
		BTUSB_ERR("%s: suspend_woble_done may error!!!", __func__);

	return ret;
}

void btmtk_usb_trigger_core_dump(void)
{
	int state = BTMTK_USB_STATE_UNKNOWN;

	BTUSB_WARN("%s: Invoked by other module (WiFi).", __func__);
	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_FW_DUMP || state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
		BTUSB_WARN("%s: current in dump state, skip.", __func__);
		USB_MUTEX_UNLOCK();
		return;
	}
	USB_MUTEX_UNLOCK();

	if (need_reset_stack_type == HW_ERR_NONE)
		need_reset_stack_type = HW_ERR_CODE_WIFI;

	btmtk_usb_toggle_rst_pin();
}
EXPORT_SYMBOL(btmtk_usb_trigger_core_dump);

/*============================================================================*/
/* Callback Functions */
/*============================================================================*/
#define ___________________________________________________Callback_Functions
static void btmtk_usb_early_suspend(void)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	BTUSB_INFO("%s", __func__);
	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_EARLY_SUSPEND);
	USB_MUTEX_UNLOCK();

	if (is_support_unify_woble(g_data))
		btmtk_usb_unify_woble_suspend(g_data);
}

static void btmtk_usb_late_resume(void)
{
	int retry_counter = 10;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int ret = -1;

late_resume_again:
	BTUSB_INFO("%s", __func__);
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_EARLY_SUSPEND || state == BTMTK_USB_STATE_WORKING) {
		BTUSB_WARN("%s: invoked immediately after early suspend, ignore.", __func__);
		btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
		BTUSB_INFO("%s: end", __func__);
		USB_MUTEX_UNLOCK();
		return;
	} else if (state == BTMTK_USB_STATE_DISCONNECT || state == BTMTK_USB_STATE_SUSPEND_DISCONNECT
			|| state == BTMTK_USB_STATE_RESUME_DISCONNECT) {
		BTUSB_WARN("%s: previous state is disconnect(%d), ignore and set to disconnect state.",
			__func__, state);
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		BTUSB_INFO("%s: end", __func__);
		USB_MUTEX_UNLOCK();
		return;
	} else if (state == BTMTK_USB_STATE_FW_DUMP || state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
		BTUSB_WARN("%s: previous state is fw dump(%d), ignore and set to fw dump state.", __func__, state);
		btmtk_usb_set_state(BTMTK_USB_STATE_FW_DUMP);
		BTUSB_INFO("%s: end", __func__);
		USB_MUTEX_UNLOCK();
		return;
	} else if (state == BTMTK_USB_STATE_PROBE ||
			state == BTMTK_USB_STATE_SUSPEND_PROBE || state == BTMTK_USB_STATE_RESUME_PROBE) {
		BTUSB_WARN("%s: previous state is probe(%d), ignore and set to probe state.", __func__, state);
		btmtk_usb_set_state(BTMTK_USB_STATE_PROBE);
		BTUSB_INFO("%s: end", __func__);
		USB_MUTEX_UNLOCK();
		return;
	} else if (state != BTMTK_USB_STATE_RESUME && state != BTMTK_USB_STATE_SUSPEND) {
		BTUSB_WARN("%s: previous state is not suspend/resume(%d), ignore.", __func__, state);
		BTUSB_INFO("%s: end", __func__);
		USB_MUTEX_UNLOCK();
		return;
	}
	btmtk_usb_set_state(BTMTK_USB_STATE_LATE_RESUME);
	USB_MUTEX_UNLOCK();

	/* Since late_resume is running in another kernel thread, handle error case here to avoid race condition. */
	ret = btmtk_usb_handle_resume();
	if (ret < 0) {
		if (retry_counter > 0 && (ret != WOBLE_FAIL)) {
			retry_counter--;
			BTUSB_WARN("%s: failed to handle resume, wait 500ms and retry again.", __func__);
			BTUSB_INFO("%s: end", __func__);
			msleep(500);
			goto late_resume_again;
		} else {
			btmtk_usb_send_assert_cmd();
		}
	}
}

static void btmtk_usb_load_rom_patch_complete(const struct urb *urb)
{
	struct completion *sent_to_mcu_done = (struct completion *)urb->context;

	complete(sent_to_mcu_done);
}

static void btmtk_usb_intr_complete(struct urb *urb)
{
	u8 *event_buf = NULL;
	u8 ebf0 = 0, ebf1 = 0, ebf2 = 0;
	u32 length = 0;
	int err;
	static u8 intr_blocking_usb_warn;

	if (g_data == NULL || urb == NULL) {
		BTUSB_ERR("%s: ERROR, %s is NULL!", __func__,
				g_data == NULL ? "g_data" : "urb");
		return;
	}

	if (urb->status != 0 && intr_blocking_usb_warn < 10) {
		intr_blocking_usb_warn++;
		BTUSB_WARN("%s: urb:%p, urb->status:%d, len:%d", __func__,
			urb, urb->status, urb->actual_length);
	} else if (urb->status == 0 && urb->actual_length != 0) {
		intr_blocking_usb_warn = 0;
	}
	event_buf = urb->transfer_buffer;
	length = event_buf[1] + 2;

	if ((urb->status == 0 || urb->status == -EOVERFLOW) && urb->actual_length != 0) {
		/* -EOVERFLOW: Could babble issue, take even only, give up remainder */
		ebf0 = event_buf[0];	/* event code */
		ebf1 = event_buf[1];	/* length */
		ebf2 = event_buf[2];	/* status */

		if ((urb->actual_length >= 2 + ebf1) && ebf0 == 0xFF && (ebf2 == 0x50 || ebf2 == 0x51)) {
			btmtk_usb_dispatch_event(event_buf, length);
			/* should drop this packet */
			goto intr_resub;
#define HCE_DIS_CONN_COMPLETE  0x05
#define HCE_SYNC_CONN_COMPLETE 0x2C
		/* For synchronous connection & disconnection */
		} else if (ebf0 == HCE_SYNC_CONN_COMPLETE && ebf2 == 0x00) {
			if (sco_handle)
				BTUSB_WARN("More than ONE SCO link");

			sco_handle = event_buf[3] + (event_buf[4] << 8);
			BTUSB_INFO("Synchronous Connection Complete, 0x%04X", sco_handle);

		} else if (ebf0 == HCE_DIS_CONN_COMPLETE && ebf2 == 0x00) {
			if ((event_buf[3] + (event_buf[4] << 8)) == sco_handle) {
				BTUSB_INFO("Synchronous Disconnection Complete, 0x%04X", sco_handle);
				sco_handle = 0;
			}
		}

		btmtk_usb_dispatch_data_bluetooth_kpi(event_buf, urb->actual_length, HCI_EVENT_PKT);
		btmtk_usb_hci_snoop_save_event(length, event_buf);

		if (event_buf[1] == 0) {
			BTUSB_ERR("Incorrect Event format");
			BTUSB_INFO_RAW(event_buf, length, "Err:");
			goto intr_resub;
		} else if (urb->actual_length > length) {
			BTUSB_WARN("Data over flow(actLen:%d, len:%d)", urb->actual_length, length);
			BTUSB_INFO_RAW(event_buf, urb->actual_length, "RAW:");
		}

		btmtk_usb_push_data_to_metabuffer(event_buf, length, HCI_EVENT_PKT);

		if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING) {
			wake_up(&BT_wq);
			wake_up_interruptible(&inq);
		} else {
			BTUSB_WARN("%s: current is in suspend/resume (%d), Don't wake-up wait queue",
					__func__, btmtk_usb_get_state());
		}
	}
intr_resub:
	usb_mark_last_busy(g_data->udev);
	usb_anchor_urb(urb, &g_data->intr_in_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed; */
		/* -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s: urb %p failed to resubmit intr_in_urb(%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static void btmtk_usb_bulk_in_complete(struct urb *urb)
{
	/* actual_length: the ACL data size (doesn't contain header) */
	u32 roomLeft, last_len, length, index, actual_length;
	u8 *event_buf;
	int state = btmtk_usb_get_state();
	int err;
	u8 *buf;
	u16 len;
	static u8 picus_blocking_warn;
	static u8 bulk_in_blocking_usb_warn;
	static u32 queueFullTimes = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	roomLeft = 0;
	last_len = 0;
	length = 0;
	index = 0;
	actual_length = 0;

	if (urb->status != 0 && bulk_in_blocking_usb_warn < 10) {
		bulk_in_blocking_usb_warn++;
		BTUSB_INFO("%s: urb %p urb->status %d count %d", __func__, urb, urb->status, urb->actual_length);
	} else if (urb->status == 0) {
		bulk_in_blocking_usb_warn = 0;
	}

	/* Handle FW Dump Data */
	buf = urb->transfer_buffer;
	len = 0;
	if (urb->actual_length > 4) {
		len = buf[2] + ((buf[3] << 8) & 0xff00);
		if (buf[0] == 0x6f && buf[1] == 0xfc && len + 4 == urb->actual_length) {
			static int print_dump_data_counter;

			if (state != BTMTK_USB_STATE_FW_DUMP && state != BTMTK_USB_STATE_RESUME_FW_DUMP) {
				/* This is the first BULK_IN packet of FW dump. */
				BTUSB_INFO("btmtk_usb FW dump begin");

				if (need_reset_stack_type == HW_ERR_NONE)
					need_reset_stack_type = HW_ERR_CODE_BT_FW;

				if (state == BTMTK_USB_STATE_RESUME)
					btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_FW_DUMP);
				else if (state == BTMTK_USB_STATE_SUSPEND)
					btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_FW_DUMP);
				else
					btmtk_usb_set_state(BTMTK_USB_STATE_FW_DUMP);

				/* Print too much log in ISR may cause kernel panic. */
				/* btmtk_usb_hci_snoop_print_to_log(); */
				print_dump_data_counter = 0;
				btmtk_add_timer(&g_data->fw_dump_timer, btmtk_fw_dump_timo_func,
					FW_DUMP_TIMO, g_data);
			}

			/* print dump data to console */
			if (print_dump_data_counter < PRINT_DUMP_PACKET_COUNT) {
				print_dump_data_counter++;
				BTUSB_INFO("btmtk_usb FW dump data (%d): %s", print_dump_data_counter, &buf[4]);
			}

			if (skb_queue_len(&g_data->fwlog_queue) < FWLOG_ASSERT_QUEUE_COUNT) {
				/* sent coredump data to queue, picus will log it */
				btmtk_skb_enq_fwlog(buf, urb->actual_length, 0, &g_data->fwlog_queue);
				wake_up_interruptible(&fw_log_inq);
				picus_blocking_warn = 0;
			} else {
				if (picus_blocking_warn == 0) {
					picus_blocking_warn = 1;
					BTUSB_WARN("btmtk_usb FW dump queue size is full");
				}
			}
			/* Save file by fifo */
			btmtk_fifo_in(FIFO_COREDUMP, g_data->bt_fifo, (const void *)&buf[4], len);

			if (buf[urb->actual_length - 6] == ' ' &&
				buf[urb->actual_length - 5] == 'e' &&
				buf[urb->actual_length - 4] == 'n' &&
				buf[urb->actual_length - 3] == 'd') {
				/* This is the latest BULK_IN packet of FW dump. */
				BTUSB_INFO("btmtk_usb FW dump end");
				btmtk_usb_toggle_rst_pin();
				picus_blocking_warn = 0;
			}

			/* set to 0xff to avoid BlueDroid dropping this ACL packet. */
			buf[0] = 0xff;
			buf[1] = 0xff;
			goto bulk_intr_resub;
		}
	}

	if (urb->status == 0) {
		event_buf = urb->transfer_buffer;
		len = buf[2] + ((buf[3] << 8) & 0xff00);

		btmtk_usb_dispatch_data_bluetooth_kpi(event_buf, urb->actual_length, HCI_ACLDATA_PKT);

		btmtk_usb_hci_snoop_save_acl(urb->actual_length, urb->transfer_buffer);

		if (urb->actual_length > 4 && event_buf[0] == 0x6f
			&& event_buf[1] == 0xfc && len + 4 == urb->actual_length) {
			BTUSB_DBG("Coredump message");
		} else {
			length = urb->actual_length + 1;

			actual_length =
				1 * (event_buf[2] & 0x0f) +
				16 * ((event_buf[2] & 0xf0) >> 4)
				+ 256 * ((event_buf[3] & 0x0f)) +
				4096 * ((event_buf[3] & 0xf0) >> 4);

			btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

			/* roomleft means the usable space */
			if (g_data->metabuffer->read_p <=
				g_data->metabuffer->write_p)
				roomLeft = META_BUFFER_SIZE - g_data->metabuffer->write_p +
						g_data->metabuffer->read_p - 1;
			else
				roomLeft = g_data->metabuffer->read_p - g_data->metabuffer->write_p - 1;

			/* no enough space to store the received data */
			if (roomLeft < length) {
				queueFullTimes++;
				if (queueFullTimes >= FW_QUEUE_FULL_ERR_MAX_TIMES) {
					BTUSB_ERR("%s: Queue full reached 100 times, toggle reset ", __func__);
					btmtk_usb_toggle_rst_pin();
				}
				BTUSB_WARN("%s: Queue is full !!", __func__);
			}
			else
				queueFullTimes = 0;

			if (length + g_data->metabuffer->write_p <
				META_BUFFER_SIZE) {

				if (leftACLSize == 0) {
					/* copy ACL data header: 0x02 */
					g_data->metabuffer->buffer[g_data->metabuffer->write_p] = HCI_ACLDATA_PKT;
					g_data->metabuffer->write_p += 1;
				}

				/* copy event data */
				memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
						event_buf, urb->actual_length);
				g_data->metabuffer->write_p += urb->actual_length;
			} else {
				last_len = META_BUFFER_SIZE - g_data->metabuffer->write_p;
				if (leftACLSize == 0) {
					if (last_len != 0) {
						/* copy ACL data header: 0x02 */
						g_data->metabuffer->buffer[g_data->metabuffer->write_p]
							= HCI_ACLDATA_PKT;
						g_data->metabuffer->write_p += 1;
						last_len--;
						/* copy event data */
						memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
								event_buf, last_len);
						memcpy(g_data->metabuffer->buffer, event_buf + last_len,
								urb->actual_length - last_len);
						g_data->metabuffer->write_p = urb->actual_length - last_len;
					} else {
						g_data->metabuffer->buffer[0] = HCI_ACLDATA_PKT;
						g_data->metabuffer->write_p = 1;
						/* copy event data */
						memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
								event_buf, urb->actual_length);
						g_data->metabuffer->write_p += urb->actual_length;
					}
				} else {	/* leftACLSize !=0 */

					/* copy event data */
					memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
							event_buf, last_len);
					memcpy(g_data->metabuffer->buffer, event_buf + last_len,
							urb->actual_length - last_len);
					g_data->metabuffer->write_p = urb->actual_length - last_len;
				}
			}
			btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

			/* the maximize bulk in ACL data packet size is 512 (4byte header + 508 byte data) */
			/* maximum receieved data size of one packet is 1025 (4byte header + 1021 byte data) */
			if (leftACLSize == 0 && actual_length > 1021) {
				/* the data in next interrupt event */
				leftACLSize = actual_length + 4 - urb->actual_length;
				BTUSB_ERR("ERROR !!! too large ACL data length %d", leftACLSize);
			} else if (leftACLSize > 0) {
				leftACLSize -= urb->actual_length;

				/* error handling. Length wrong, drop some bytes to recovery counter!! */
				if (leftACLSize < 0) {
					BTUSB_WARN("* size wrong(%d), this acl data may be wrong!!", leftACLSize);
					leftACLSize = 0;	/* reset count */
				}

				if (leftACLSize == 0) {
					if (state == BTMTK_USB_STATE_WORKING || state == BTMTK_USB_STATE_FW_DUMP ||
						state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
						wake_up(&BT_wq);
						wake_up_interruptible(&inq);
					} else {
						BTUSB_DBG("%s: now is in suspend/resume(%d), Don't wake-up wait queue",
								__func__, state);
					}
				}
			} else if (leftACLSize == 0 && actual_length <= 1021) {
				if (state == BTMTK_USB_STATE_WORKING || state == BTMTK_USB_STATE_FW_DUMP ||
					state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
					wake_up(&BT_wq);
					wake_up_interruptible(&inq);
				} else {
					BTUSB_DBG("%s: current is in suspend/resume (%d), Don't wake-up wait queue",
							__func__, state);
				}
			} else {
				BTUSB_WARN("ACL data count fail, leftACLSize:%d", leftACLSize);
			}
		}
	} else {
		BTUSB_DBG("%s: urb->status:%d", __func__, urb->status);
	}
bulk_intr_resub:

	usb_anchor_urb(urb, &g_data->bulk_in_anchor);
	usb_mark_last_busy(g_data->udev);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err != 0) {
		/* -EPERM: urb is being killed; */
		/* -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s: urb %p failed to resubmit bulk_in_urb(%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static void btmtk_usb_tx_complete_meta(const struct urb *urb)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	usb_free_coherent(g_data->udev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);

	g_data->meta_tx = 0;
	kfree(urb->setup_packet);
}

static int btmtk_usb_send_apcf_reserved(void)
{
	int ret = -1;
	u8 reserve_apcf_cmd[] = { 0x5C, 0xFC, 0x01, 0x0A };
	u8 reserve_apcf_event[] = { 0x0e, 0x06, 0x01, 0x5C, 0xFC, 0x00 };

	ret = btmtk_usb_send_hci_cmd(reserve_apcf_cmd, sizeof(reserve_apcf_cmd),
		reserve_apcf_event, sizeof(reserve_apcf_event));
	if (ret > 0)
		ret = 0;
	else
		BTUSB_ERR("%s: btmtk_usb_send_hci_cmd return error ret %d", __func__, ret);

	BTUSB_INFO("%s: ret %d", __func__, ret);
	return ret;
}

static int btmtk_usb_send_get_vendor_cap(void)
{
	int ret = -1;
	u8 get_vendor_cap_cmd[] = { 0x53, 0xFD, 0x00 };
	u8 get_vendor_cap_event[] = { 0x0e, 0x12, 0x01, 0x53, 0xFD, 0x00,
		/* 0x64, 0x01, 0xb0, 0x4f, 0x32, 0x01 */ };

	ret = btmtk_usb_send_hci_cmd(get_vendor_cap_cmd, sizeof(get_vendor_cap_cmd),
		get_vendor_cap_event, sizeof(get_vendor_cap_event));
	if (ret > 0)
		ret = 0;
	else
		BTUSB_ERR("%s: btmtk_usb_send_hci_cmd return error ret %d", __func__, ret);

	BTUSB_INFO("%s: ret %d", __func__, ret);
	return ret;
}

static int btmtk_usb_standby(void)
{
	int ret = -1;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	BTUSB_INFO("%s", __func__);
	if (is_support_unify_woble(g_data)) {
		ret = btmtk_usb_reset_power_on();
		if (ret < 0)
			return ret;
		ret = btmtk_usb_send_apcf_reserved();
		if (ret < 0)
			return ret;
		ret = btmtk_usb_send_get_vendor_cap();
		if (ret < 0)
			return ret;
		ret = btmtk_usb_unify_woble_suspend(g_data);
		if (ret < 0)
			return ret;
	} else {
		ret = btmtk_usb_handle_entering_WoBLE_state();
		if (ret < 0)
			return ret;
	}

	BTUSB_INFO("%s: End after 500ms delay", __func__);
	msleep(500); /* Add 500ms delay to avoid log lost. */
	return 0;
}

/*============================================================================*/
/* Internal Functions : SCO and Isochronous Related */
/*============================================================================*/
#define ___________________________________Internal_Functions_SCO_Isochronous
static int btmtk_usb_set_isoc_interface(bool close)
{
	int ret = -1, i = 0;
	struct usb_endpoint_descriptor *ep_desc;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENXIO;
	}

	if ((sco_handle && g_data->isoc_urb_submitted && close == false)
			|| (sco_handle == 0 && g_data->isoc_urb_submitted == 0)) {
		return 0;

	} else if (sco_handle && g_data->isoc_urb_submitted == 0) {
		/* Alternate setting */
		ret = usb_set_interface(g_data->udev, 1, ISOC_IF_ALT);
		if (ret < 0) {
			BTUSB_ERR("%s: Set ISOC alternate(%d) fail", __func__, ISOC_IF_ALT);
			return ret;
		}
		BTUSB_INFO("%s: Set alternate to %d", __func__, ISOC_IF_ALT);

		for (i = 0; i < g_data->isoc->cur_altsetting->desc.bNumEndpoints; i++) {
			ep_desc = &g_data->isoc->cur_altsetting->endpoint[i].desc;

			if (usb_endpoint_is_isoc_out(ep_desc)) {
				g_data->isoc_tx_ep = ep_desc;
				BTUSB_INFO("iso_out: length: %d, addr: 0x%02X, maxSize: %d, interval: %d",
						ep_desc->bLength, ep_desc->bEndpointAddress,
						ep_desc->wMaxPacketSize, ep_desc->bInterval);
				continue;
			}

			if (usb_endpoint_is_isoc_in(ep_desc)) {
				g_data->isoc_rx_ep = ep_desc;
				BTUSB_INFO("iso_in: length: %d, addr: 0x%02X, maxSize: %d, interval: %d",
						ep_desc->bLength, ep_desc->bEndpointAddress,
						ep_desc->wMaxPacketSize, ep_desc->bInterval);
				continue;
			}
		}
		if (!g_data->isoc_tx_ep || !g_data->isoc_rx_ep) {
			BTUSB_ERR("Invalid SCO descriptors");
			return -ENODEV;
		}

		ret = btmtk_usb_submit_isoc_urb();
		if (ret < 0)
			return ret;
		BTUSB_INFO("%s: Start isoc_in.", __func__);

	} else if ((sco_handle == 0 || close == true) && g_data->isoc_urb_submitted) {
		u8 count = 0;

		while (atomic_read(&g_data->isoc_out_count) && ++count <= RETRY_TIMES) {
			BTUSB_INFO("There are isoc out packet remaining: %d ",
					atomic_read(&g_data->isoc_out_count));
			mdelay(10);
		}
		usb_kill_anchored_urbs(&g_data->isoc_in_anchor);
		g_data->isoc_urb_submitted = 0;
		BTUSB_INFO("%s: Stop isoc_in.", __func__);

		ret = usb_set_interface(g_data->udev, 1, 0);
		if (ret < 0) {
			BTUSB_ERR("%s: Set ISOC alternate(0) fail", __func__);
			return ret;
		}
		BTUSB_INFO("%s: Set alternate to 0", __func__);

	} else {
		BTUSB_INFO("%s: sco: 0x%04X, isoc_urb_submitted: %d",
				__func__, sco_handle, g_data->isoc_urb_submitted);
	}
	return 0;
}

static int btmtk_usb_submit_isoc_urb(void)
{
	struct urb *urb;
	u8 *buf;
	unsigned int pipe;
	int err, size;

	BTUSB_INFO("%s", __func__);

	if (g_data->isoc_urb_submitted) {
		BTUSB_WARN("%s: already submitted", __func__);
		return 0;
	}
	g_data->isoc_urb_submitted = 0;

	if (!g_data->isoc_rx_ep) {
		BTUSB_ERR("%s: error 1", __func__);
		return -ENODEV;
	}

	urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_KERNEL);
	if (!urb) {
		BTUSB_ERR("%s: error 2", __func__);
		return -ENOMEM;
	}

	size = le16_to_cpu(g_data->isoc_rx_ep->wMaxPacketSize) *
		BTUSB_MAX_ISOC_FRAMES;

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		usb_free_urb(urb);
		BTUSB_ERR("%s: error 3", __func__);
		return -ENOMEM;
	}

	/* For isoc in URB */
	pipe = usb_rcvisocpipe(g_data->udev, g_data->isoc_rx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, g_data->udev, pipe, buf, size, (usb_complete_t)btmtk_usb_isoc_complete,
			(void *)g_data, g_data->isoc_rx_ep->bInterval);

	urb->transfer_flags = URB_FREE_BUFFER | URB_ISO_ASAP;

	__fill_isoc_descriptor(urb, size,
			le16_to_cpu(g_data->isoc_rx_ep->wMaxPacketSize));

	usb_anchor_urb(urb, &g_data->isoc_in_anchor);

	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s urb %p submission failed (%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	} else {
		g_data->isoc_urb_submitted = 1;
	}

	usb_free_urb(urb);
	return err;
}

static void btmtk_usb_isoc_complete(struct urb *urb)
{
	int err;
	struct sk_buff *skb_isoc = NULL;
	unsigned long flags = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	BTUSB_DBG("%s Start(status: %d, len: %d)", __func__, urb->status, urb->actual_length);

	/* Controller reply nothing to ISO_IN. */
	if (urb->status == -EPROTO) {
		goto isoc_resub;
	/* Controller reply something to ISO_IN. */
	} else if (urb->status == -EINPROGRESS || urb->status == 0) {
		if (urb->actual_length != 0 && sco_handle > 0) {
			skb_isoc = alloc_skb(urb->actual_length, GFP_ATOMIC);
			if (skb_isoc == NULL) {
				BTUSB_ERR("%s: alloc_skb return 0, error", __func__);
				goto isoc_resub;
			}

			/* queue */
			memset(skb_isoc->data, 0, urb->actual_length);
			memcpy(skb_isoc->data, urb->transfer_buffer, urb->actual_length);
			/** This print will block ISOC_IN
			 * BTUSB_DBG_RAW(skb_isoc->data, urb->actual_length,
					"%s: (len=%2d)", __func__, urb->actual_length);
			 */
			skb_isoc->len = urb->actual_length;
			ISOC_SPIN_LOCK(flags);
			skb_queue_tail(&g_data->isoc_in_queue, skb_isoc);
			ISOC_SPIN_UNLOCK(flags);
			wake_up(&BT_sco_wq);
			wake_up_interruptible(&inq_isoc);
		}
	} else {
		BTUSB_WARN("%s WARNING! status:%d", __func__, urb->status);
	}

isoc_resub:
	usb_mark_last_busy(g_data->udev);
	usb_anchor_urb(urb, &g_data->isoc_in_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected
		 */
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s urb %p failed to resubmit (%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	}
	BTUSB_DBG("%s End", __func__);
}

static void btmtk_usb_isoc_tx_complete(const struct urb *urb)
{
	if (urb->status != 0 || urb->actual_length == 0)
		BTUSB_DBG("%s urb %p status %d count %d", __func__,
				urb, urb->status, urb->actual_length);

	kfree(urb->transfer_buffer);
	kfree(urb->setup_packet);
	if (atomic_read(&g_data->isoc_out_count))
		atomic_dec(&g_data->isoc_out_count);
}

/*============================================================================*/
/* Interface Functions : BT Stack */
/*============================================================================*/
#define ______________________________________Interface_Function_for_BT_Stack
static ssize_t btmtk_usb_fops_write(struct file *file, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	int retval = 0;
	static u8 waiting_for_hci_without_packet_type; /* INITIALISED_STATIC: do not initialise statics to 0 */
	static u8 hci_packet_type = 0xff;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!, (pid:%d)", __func__, current->pid);
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)!, (pid:%d)", __func__, fstate, current->pid);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_WARN_LIMITTED("%s: current is in suspend/resume (%d), (pid:%d).", __func__, state, current->pid);
		USB_MUTEX_UNLOCK();
		msleep(10);
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	if (need_reopen) {
		BTUSB_WARN("%s: need_reopen (%d)! (pid:%d)", __func__, need_reopen, current->pid);
		return -EFAULT;
	}

	if (need_reset_stack) {
		BTUSB_WARN("%s: need_reset_stack (%d)! (pid:%d)", __func__, need_reset_stack, current->pid);
		return -EFAULT;
	}

	if (g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWER_OFF) {
		BTUSB_WARN("%s: dongle state already power off, do not write", __func__);
		return -EFAULT;
	}

	/* semaphore mechanism, the waited process will sleep */
	down(&g_data->wr_mtx);

	if (waiting_for_hci_without_packet_type == 1 && count == 1) {
		BTUSB_WARN("%s: Waiting for hci_without_packet_type, but receive data count is 1!", __func__);
		BTUSB_WARN("%s: Treat this packet as packet_type", __func__);
		retval = copy_from_user(&hci_packet_type, &buf[0], 1);
		waiting_for_hci_without_packet_type = 1;
		retval = 1;
		goto OUT;
	}

	if (waiting_for_hci_without_packet_type == 0) {
		if (count == 1) {
			retval = copy_from_user(&hci_packet_type, &buf[0], 1);
			waiting_for_hci_without_packet_type = 1;
			retval = 1;
			goto OUT;
		}
	}

	if (count > 0) {
		u32 pkt_len;
		u32 copy_size = (count < BUFFER_SIZE) ? count : BUFFER_SIZE;

		if (waiting_for_hci_without_packet_type) {
			retval = copy_from_user(&g_data->o_buf[1], &buf[0], copy_size);
			g_data->o_buf[0] = hci_packet_type;
			copy_size += 1;
		} else {
			retval = copy_from_user(&g_data->o_buf[0], &buf[0], copy_size);
		}

		if (retval) {
			retval = -EFAULT;
			BTUSB_ERR("%s: copy data from user fail", __func__);
			goto OUT;
		}

		btmtk_usb_dispatch_data_bluetooth_kpi(g_data->o_buf, copy_size, HCI_COMMAND_PKT);

		/* command */
		if (g_data->o_buf[0] == HCI_COMMAND_PKT) {
			/* parsing commands */
			u8 fw_assert_cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
			u8 reset_cmd[] = { 0x01, 0x03, 0x0C, 0x00 };
			u8 read_ver_cmd[] = { 0x01, 0x01, 0x10, 0x00 };
			u8 le_scan_parm[] = { 0x01, 0x0B, 0x20, };
			u8 le_scan_enable[] = { 0x01, 0x0C, 0x20, };

			if (copy_size == sizeof(fw_assert_cmd) &&
				!memcmp(g_data->o_buf, fw_assert_cmd, sizeof(fw_assert_cmd))) {
				BTUSB_INFO("%s: Donge FW Assert Triggered by BT Stack!", __func__);
				if (need_reset_stack_type == HW_ERR_NONE)
					need_reset_stack_type = HW_ERR_CODE_BT_HOST;
				btmtk_usb_hci_snoop_print_to_log();
			} else if (copy_size == sizeof(reset_cmd) &&
					!memcmp(g_data->o_buf, reset_cmd, sizeof(reset_cmd))) {
				BTUSB_INFO("%s: got command: 0x03 0C 00 (HCI_RESET)", __func__);

			} else if (copy_size == sizeof(read_ver_cmd) &&
					!memcmp(g_data->o_buf, read_ver_cmd, sizeof(read_ver_cmd))) {
				BTUSB_INFO("%s: got command: 0x01 10 00 (READ_LOCAL_VERSION)", __func__);
			} else if (copy_size == 11 &&
					!memcmp(g_data->o_buf, le_scan_parm, sizeof(le_scan_parm))) {
				BTUSB_DBG("%s: got command: 0x0B 20 (LE_SET_SCAN_PARAMETERS)", __func__);
				btmtk_usb_save_le_scan_parm(g_data->o_buf[4], *(u16 *)(g_data->o_buf + 5),
						*(u16 *)(g_data->o_buf + 7), g_data->o_buf[9],
						g_data->o_buf[10]);
			} else if (copy_size == 6 &&
					!memcmp(g_data->o_buf, le_scan_enable, sizeof(le_scan_enable))) {
				BTUSB_DBG("%s: got command: 0x0C 20 (LE_SET_SCAN_ENABLE)", __func__);
				btmtk_usb_save_le_scan_state(g_data->o_buf[4], g_data->o_buf[5]);
			}

			/* HCI command : Type(8b) OpCode(16b) length(8b)
			 * Header length = 1 + 2 + 1
			 */
			pkt_len = g_data->o_buf[3] + HCI_CMD_HEADER_LEN;

			/* check frame length is valid */
			if (pkt_len != copy_size) {
				BTUSB_ERR("%s: input HCI command len(%d) error (expect %d)\n", __func__, copy_size, pkt_len);
				retval = -EFAULT;
				goto OUT;
			}

			btmtk_usb_hci_snoop_save_cmd(copy_size, &g_data->o_buf[0]);
			retval = btmtk_usb_meta_send_data(&g_data->o_buf[0], copy_size);

		/* ACL data */
		} else if (g_data->o_buf[0] == HCI_ACLDATA_PKT) {
			/* ACL data : Type(8b) handle+flag(16b) length(16b)
			 * Header length = 1 + 2 + 2
			 */
			pkt_len = (g_data->o_buf[3] | (g_data->o_buf[4] << 8)) + HCI_ACL_HEADER_LEN;

			/* check frame length is valid */
			if (pkt_len != copy_size) {
				BTUSB_ERR("%s: input ACL packet len(%d) error (expect %d)\n", __func__, copy_size, pkt_len);
				retval = -EFAULT;
				goto OUT;
			}

			retval = btmtk_usb_send_data(&g_data->o_buf[0], copy_size);
		/* Unknown */
		} else {
			BTUSB_WARN("%s: this is unknown bt data:0x%02x", __func__, g_data->o_buf[0]);
		}

		if (waiting_for_hci_without_packet_type) {
			hci_packet_type = 0xff;
			waiting_for_hci_without_packet_type = 0;
			if (retval > 0)
				retval -= 1;
		}
	} else {
		retval = -EFAULT;
		BTUSB_ERR("%s: target packet length:%zu is not allowed, retval = %d", __func__, count, retval);
	}

OUT:
	up(&g_data->wr_mtx);
	return retval;
}

static ssize_t btmtk_usb_fops_writefwlog(struct file *filp, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	int i = 0, len = 0, ret = -1;

	int state = BTMTK_USB_STATE_UNKNOWN;

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	USB_MUTEX_UNLOCK();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_WARN("%s: current is in suspend/resume/standby/dump/disconnect (%d).", __func__, state);
		return -EBADFD;
	}

	/* Command example : echo 01 be fc 01 05 > /dev/stpbtfwlog */
	if (count > HCI_MAX_COMMAND_BUF_SIZE) {
		BTUSB_ERR("%s: your command is larger than buffer length, count = %zd/%d",
				__func__, count, HCI_MAX_COMMAND_BUF_SIZE);
		return -ENOMEM;
	}

	memset(g_data->i_fwlog_buf, 0, HCI_MAX_COMMAND_BUF_SIZE);
	memset(g_data->o_fwlog_buf, 0, HCI_MAX_COMMAND_SIZE);
	if (copy_from_user(g_data->i_fwlog_buf, buf, count) != 0) {
		BTUSB_ERR("%s: Failed to copy data", __func__);
		return -ENODATA;
	}

	/* For log_lvl, EX: echo log_lvl=4 > /dev/stpbtfwlog */
	if (strcmp(g_data->i_fwlog_buf, "log_lvl=") >= 0) {
		u8 val = *(g_data->i_fwlog_buf + strlen("log_lvl=")) - 48;

		if (val > BTMTK_LOG_LEVEL_MAX || val <= 0) {
			BTUSB_ERR("%s: Got incorrect value for log level(%d)", __func__, val);
			return -EINVAL;
		}
		btmtk_log_lvl = val;
		BTUSB_INFO("%s: btmtk_log_lvl = %d", __func__, btmtk_log_lvl);
		return count;
	} else if (strcmp(g_data->i_fwlog_buf, "bperf=") >= 0) {
		u8 val = *(g_data->i_fwlog_buf + strlen("bperf=")) - 48;

		btmtk_bluetooth_kpi = val;
		BTUSB_INFO("%s: set bluetooth KPI feature(bperf) to %d", __func__, btmtk_bluetooth_kpi);
		return count;
	/* Toggle reset pin directly, only for test, TOGGLE_RESET define in btmtk_config.h */
	} else if (strcmp(g_data->i_fwlog_buf, RESET_BT) >= 0) {
		if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING
				&& need_reset_stack == HW_ERR_NONE && need_reopen == 0) {
			btmtk_usb_toggle_rst_pin();
			btmtk_add_timer(&chip_reset_timer, btmtk_chip_reset_timo_func,
					RESET_TIMO, g_data);
		}
		return count;
	}

	if (is_mt7668(g_data) && g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON) {
		BTUSB_ERR("%s: 7668 is not opening(%d)", __func__, g_data->is_mt7668_dongle_state);
		return -EBADFD;
	}
	/* hci input command format : echo 01 be fc 01 05 > /dev/stpbtfwlog */
	/* We take the data from index three to end. */
	for (i = 0; i < count; i++) {
		char *pos = g_data->i_fwlog_buf + i;
		char temp_str[3] = {'\0'};
		long res = 0;

		if (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') {
			continue;
		} else if (*pos == '0' && (*(pos + 1) == 'x' || *(pos + 1) == 'X')) {
			i++;
			continue;
		} else if (!(*pos >= '0' && *pos <= '9') && !(*pos >= 'A' && *pos <= 'F')
			&& !(*pos >= 'a' && *pos <= 'f')) {
			BTUSB_ERR("%s: There is an invalid input(%c)", __func__, *pos);
			return -EINVAL;
		}
		temp_str[0] = *pos;
		temp_str[1] = *(pos + 1);
		i++;
		ret = kstrtol(temp_str, 16, &res);
		if (ret == 0)
			g_data->o_fwlog_buf[len++] = (u8)res;
		else
			BTUSB_ERR("%s: Convert %s failed(%d)", __func__, temp_str, ret);
	}

	/* Receive command from stpbtfwlog, then Sent hci command to controller */
	BTUSB_DBG_RAW(g_data->o_fwlog_buf, len, "%s: Input is:", __func__);

	if (g_data->o_fwlog_buf[0] != HCI_COMMAND_PKT) {
		BTUSB_ERR("%s: Not support 0x%02X yet", __func__, g_data->o_fwlog_buf[0]);
		return -EPROTONOSUPPORT;
	}
	/* check HCI command length */
	if (len > HCI_MAX_COMMAND_SIZE) {
		BTUSB_ERR("%s: command is larger than max buf size, length = %d", __func__, len);
		return -ENOMEM;
	}

	/* send HCI command */
	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
			0, DEVICE_CLASS_REQUEST_OUT, 0, 0,
			(void *)g_data->o_fwlog_buf + 1, len, USB_CTRL_IO_TIMO);
	if (ret < 0) {
		BTUSB_ERR("%s: command send failed(%d)", __func__, ret);
		return -EIO;
	}
	BTUSB_INFO("%s: Write end(len: %d)", __func__, len);
	return count;	/* If input is correct should return the same length */
}

static ssize_t btmtk_usb_fops_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
	int copyLen = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	unsigned int tailLen = 0;
	u8 *buffer = NULL;
	u8 hwerr_event[] = { 0x04, 0x10, 0x01, 0xff };
	unsigned long ret_len = 0;

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)! (pid:%d)", __func__, fstate, current->pid);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	/* no need to check state after need reset stack move to probe end*/
#if 0
	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING && state != BTMTK_USB_STATE_FW_DUMP) {
		BTUSB_WARN("%s: current is not in working/dump state  state (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();
#endif

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL! (pid:%d)", __func__, current->pid);
		return -ENODEV;
	}
	buffer = g_data->i_buf;

	down(&g_data->rd_mtx);

	if (count > BUFFER_SIZE) {
		count = BUFFER_SIZE;
		BTUSB_WARN("read size is bigger than 1024");
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_SUSPEND_FW_DUMP) {
		if (printk_ratelimit())
			BTUSB_ERR("%s: current is BTMTK_USB_STATE_SUSPEND_FW_DUMP (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		copyLen = 0;
		goto OUT;
	} else if (need_reset_stack) {
		BTUSB_WARN("%s: need_reset_stack (%d)!", __func__, need_reset_stack);
		USB_MUTEX_UNLOCK();
		hwerr_event[3] = need_reset_stack;
		BTUSB_WARN("%s: go if send_hw_err_event_count %d", __func__, send_hw_err_event_count);
		if (send_hw_err_event_count < sizeof(hwerr_event)) {
			if (count < (sizeof(hwerr_event) - send_hw_err_event_count)) {
				copyLen = count;
				BTUSB_INFO("call wake_up_interruptible");
				wake_up_interruptible(&inq);
			} else
				copyLen = (sizeof(hwerr_event) - send_hw_err_event_count);

			BTUSB_WARN("%s: in if copyLen = %d", __func__, copyLen);
			if (copy_to_user(buf, hwerr_event + send_hw_err_event_count, copyLen)) {
				BTUSB_ERR("send_hw_err_event_count %d copy to user fail, count = %d, go out",
					send_hw_err_event_count, copyLen);
				copyLen = -EFAULT;
				goto OUT;
			}
			send_hw_err_event_count += copyLen;
			BTUSB_WARN("%s: in if send_hw_err_event_count = %d", __func__, send_hw_err_event_count);
			if (send_hw_err_event_count >= sizeof(hwerr_event)) {
				send_hw_err_event_count = 0;
				BTUSB_WARN("%s: set need_reset_stack=0", __func__);
				need_reset_stack = HW_ERR_NONE;
				need_reset_stack_type = HW_ERR_NONE;
				need_reopen = 1;
			}
			BTUSB_WARN("%s: set call up", __func__);
			goto OUT;
		} else {
			BTUSB_WARN("%s: xx set copyLen = -EFAULT", __func__);
			copyLen = -EFAULT;
			goto OUT;
		}

	}
	USB_MUTEX_UNLOCK();

	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	/* means the buffer is empty */
	while (g_data->metabuffer->read_p == g_data->metabuffer->write_p) {

		/* unlock the buffer to let other process write data to buffer */
		btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

		/* If nonblocking mode, return directly O_NONBLOCK is specified during open() */
		if (file->f_flags & O_NONBLOCK) {
			/* BTUSB_DBG("Non-blocking btmtk_usb_fops_read()"); */
			copyLen = 0;
			goto OUT;
		}
		wait_event(BT_wq, g_data->metabuffer->read_p != g_data->metabuffer->write_p);
		btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	}

	while (g_data->metabuffer->read_p != g_data->metabuffer->write_p) {
		if (g_data->metabuffer->write_p > g_data->metabuffer->read_p) {
			copyLen = g_data->metabuffer->write_p - g_data->metabuffer->read_p;
			if (copyLen > count)
				copyLen = count;
			memcpy(g_data->i_buf, g_data->metabuffer->buffer + g_data->metabuffer->read_p, copyLen);
			g_data->metabuffer->read_p += copyLen;
			break;

		} else {
			tailLen = META_BUFFER_SIZE - g_data->metabuffer->read_p;
			if (tailLen > count) {	/* exclude equal case to skip wrap check */
				copyLen = count;
				memcpy(g_data->i_buf, g_data->metabuffer->buffer + g_data->metabuffer->read_p, copyLen);
				g_data->metabuffer->read_p += copyLen;
			} else {
				/* part 1: copy tailLen */
				memcpy(g_data->i_buf, g_data->metabuffer->buffer + g_data->metabuffer->read_p, tailLen);

				buffer += tailLen;	/* update buffer offset */

				/* part 2: check if head length is enough */
				copyLen = count - tailLen;

				/* if write_p < copyLen: means we can copy all data until write_p; */
				/* else: we can only copy data for copyLen */
				copyLen = (g_data->metabuffer->write_p < copyLen) ? g_data->
					metabuffer->write_p : copyLen;

				/* if copylen not 0, copy data to buffer */
				if (copyLen)
					memcpy(buffer, g_data->metabuffer->buffer + 0, copyLen);
				/* Update read_p final position */
				g_data->metabuffer->read_p = copyLen;

				/* update return length: head + tail */
				copyLen += tailLen;
			}
			break;
		}
	}

	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	/* BTUSB_DBG_RAW(g_data->i_buf, 16, "%s:  (len=%2d)", __func__, copyLen); */
	ret_len = copy_to_user(buf, g_data->i_buf, copyLen);
	if (ret_len) {
		BTUSB_ERR("copy to user fail, copyLen = %d, ret_len = %ld, count = %zd", copyLen, ret_len, count);
		copyLen = -EFAULT;
		goto OUT;
	}
OUT:
	up(&g_data->rd_mtx);
	return copyLen;
}

static int btmtk_usb_send_init_cmds(void)
{
#if SUPPORT_MT7662
	if (is_mt7662(g_data)) {
		btmtk_usb_send_hci_reset_cmd();
		btmtk_usb_send_hci_low_power_cmd_7662(TRUE);
		btmtk_usb_send_hci_set_ce_cmd_7662();
		btmtk_usb_send_hci_set_tx_power_cmd_7662();
		btmtk_usb_send_hci_radio_on_cmd_7662();
	}
#endif

#if SUPPORT_MT7668
	if (is_mt7668(g_data)) {
		btmtk_usb_send_wmt_power_on_cmd_7668();
		if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON) {
			BTUSB_ERR("Power on MT7668 failed, reset it");
			btmtk_usb_toggle_rst_pin();
			return -1;
		}
		if (btmtk_usb_send_hci_tci_set_sleep_cmd_7668() < 0) {
			btmtk_usb_toggle_rst_pin();
			return -1;
		}
	}
#endif
	return 0;
}

static int btmtk_usb_send_deinit_cmds(void)
{
#if SUPPORT_MT7662
	if (is_mt7662(g_data))
		btmtk_usb_send_hci_radio_off_cmd_7662();
#endif

#if SUPPORT_MT7668
	if (is_mt7668(g_data)) {
		btmtk_usb_send_wmt_power_off_cmd_7668();
		if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_OFF) {
			BTUSB_ERR("Power off MT7668 failed, reset it");
			btmtk_usb_toggle_rst_pin();
			return -1;
		}
	}
#endif
	return 0;
}

static int btmtk_usb_fops_open(struct inode *inode, struct file *file)
{
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	BTUSB_PRINT(btmtk_fops_open_log, "%s: major %d minor %d (pid %d), probe counter: %d",
			__func__, imajor(inode), iminor(inode), current->pid, probe_counter);

	if (g_data == NULL) {
		btmtk_fops_open_log = BTMTK_LOG_FULL_PRINT;
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_INIT || state == BTMTK_USB_STATE_DISCONNECT) {
		btmtk_fops_open_log = BTMTK_LOG_LIMIT_PRINT;
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		if (need_reopen) {
			BTUSB_PRINT(btmtk_fops_open_log, "%s:need do fops close firstly", __func__);
			btmtk_fops_open_log = BTMTK_LOG_LIMIT_PRINT;
			FOPS_MUTEX_UNLOCK();
			return -EAGAIN;
		} else {
			BTUSB_PRINT(btmtk_fops_open_log, "%s: fops opened!", __func__);
			btmtk_fops_open_log = BTMTK_LOG_FULL_PRINT;
			FOPS_MUTEX_UNLOCK();
			return 0;
		}
	}

	if (fstate == BTMTK_FOPS_STATE_CLOSING) {
		BTUSB_PRINT(btmtk_fops_open_log, "%s: fops close is on-going !", __func__);
		btmtk_fops_open_log = BTMTK_LOG_LIMIT_PRINT;
		FOPS_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	FOPS_MUTEX_UNLOCK();

	BTUSB_PRINT(btmtk_fops_open_log, "%s: Mediatek Bluetooth USB driver ver %s", __func__, VERSION);

	if (current->pid == 1) {
		btmtk_fops_open_log = BTMTK_LOG_FULL_PRINT;
		BTUSB_WARN("%s: return 0", __func__);
		return 0;
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		btmtk_fops_open_log = BTMTK_LOG_FULL_PRINT;
		BTUSB_WARN("%s: not in working state(%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		return -ENODEV;
	}
	USB_MUTEX_UNLOCK();

	sema_init(&g_data->wr_mtx, 1);
	sema_init(&g_data->rd_mtx, 1);
	sema_init(&g_data->isoc_wr_mtx, 1);
	sema_init(&g_data->isoc_rd_mtx, 1);

	/* init wait queue */
	init_waitqueue_head(&(inq));

	if (btmtk_usb_send_init_cmds()) {
		btmtk_fops_open_log = BTMTK_LOG_LIMIT_PRINT;
		msleep(btmtk_chip_reset_delay + 10);	/* wait chip reset */
		return -EAGAIN;
	}

	if (is_support_unify_woble(g_data))
		btmtk_usb_send_apcf_reserved();

	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	BTUSB_PRINT(btmtk_fops_open_log, "enable interrupt and bulk in urb");
	if (btmtk_usb_submit_intr_urb() != 0) {
		BTUSB_PRINT(btmtk_fops_open_log, "Submit interrupt URB failed");
		btmtk_fops_open_log = BTMTK_LOG_LIMIT_PRINT;
		return -EAGAIN;
	}
	if (btmtk_usb_submit_bulk_in_urb() != 0) {
		BTUSB_PRINT(btmtk_fops_open_log, "Submit bulk in URB failed");
		btmtk_fops_open_log = BTMTK_LOG_LIMIT_PRINT;
		return -EAGAIN;
	}

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_OPENED);
	FOPS_MUTEX_UNLOCK();
	need_reopen = 0;
	need_reset_stack_type = HW_ERR_NONE;
	need_reset_stack = HW_ERR_NONE;
	btmtk_fops_open_log = BTMTK_LOG_FULL_PRINT;

	BTUSB_INFO("%s: OK", __func__);

	return 0;
}

static int btmtk_usb_fops_close(struct inode *inode, struct file *file)
{
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	BTUSB_INFO("%s begin: major %d minor %d (pid %d), probe:%d", __func__,
			imajor(inode), iminor(inode), current->pid, probe_counter);

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not allow close(%d)", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSING);
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_WARN("%s: not in working state(%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		goto exit;
	}
	USB_MUTEX_UNLOCK();

	if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON) {
		BTUSB_WARN("%s: dongle state(%d) return.", __func__,
			g_data->is_mt7668_dongle_state);
		goto exit;
	}

	btmtk_usb_stop_traffic();
	btmtk_usb_send_hci_reset_cmd();

	btmtk_usb_send_deinit_cmds();

exit:
	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSED);
	FOPS_MUTEX_UNLOCK();

	/* In case no read from stack, and close directly */
	need_reset_stack = HW_ERR_NONE;
	need_reset_stack_type = HW_ERR_NONE;
	send_hw_err_event_count = 0;

	BTUSB_INFO("%s: OK", __func__);
	return 0;
}

static long btmtk_usb_fops_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	switch (cmd) {
	case IOCTL_FW_ASSERT:
		/* BT trigger fw assert for debug */
		BTUSB_INFO("BT Set fw assert..., reason:%lu", arg);
		break;
	default:
		retval = -EFAULT;
		BTUSB_WARN("BT_ioctl(): unknown cmd (%d)", cmd);
		break;
	}

	return retval;
}

static unsigned int btmtk_usb_fops_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	if (g_data->metabuffer->read_p == g_data->metabuffer->write_p) {
		poll_wait(file, &inq, wait);

		/* empty let select sleep */
		if ((g_data->metabuffer->read_p != g_data->metabuffer->write_p) || need_reset_stack)
			mask |= POLLIN | POLLRDNORM;		/* readable */
	} else {
		mask |= POLLIN | POLLRDNORM;			/* readable */
	}

	/* do we need condition? */
	mask |= POLLOUT | POLLWRNORM;				/* writable */

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING)		/* BTMTK_USB_STATE_WORKING: do nothing */
		mask = 0;
	USB_MUTEX_UNLOCK();

	return mask;
}

static ssize_t btmtk_usb_fops_readfwlog(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int copyLen = 0;
	ulong flags = 0;
	struct sk_buff *skb = NULL;
	unsigned long ret_len = 0;

	if (g_data == NULL)
		return -ENODEV;

	/* picus read a queue, it may occur performace issue */
	FWLOG_SPIN_LOCK(flags);
	if (skb_queue_len(&g_data->fwlog_queue))
		skb = skb_dequeue(&g_data->fwlog_queue);
	FWLOG_SPIN_UNLOCK(flags);
	if (skb == NULL)
		return 0;

	if (skb->len <= count) {
		ret_len = copy_to_user(buf, skb->data, skb->len);
		if (ret_len) {
			BTUSB_ERR("%s: copy_to_user failed!, skb->len = %d, ret_len = %ld, count = %zd",
					__func__, skb->len, ret_len, count);
			/* copy_to_user failed, add skb to fwlog_fops_queue */
			skb_queue_head(&g_data->fwlog_queue, skb);
			copyLen = -EFAULT;
			goto OUT;
		}
		copyLen = skb->len;
	} else {
		BTUSB_ERR("%s: Drop data!! skb->len err(count: %d, skb.len: %d)", __func__, (int)count, skb->len);
		copyLen = -EFAULT;
	}
	kfree_skb(skb);
OUT:
	return copyLen;
}

static int btmtk_usb_fops_openfwlog(struct inode *inode, struct file *file)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	BTUSB_INFO("%s: OK", __func__);
	return 0;
}

static int btmtk_usb_fops_closefwlog(struct inode *inode, struct file *file)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	BTUSB_INFO("%s: OK", __func__);
	return 0;
}

static long btmtk_usb_fops_unlocked_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	BTUSB_ERR("%s: ->", __func__);
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	return retval;
}

static unsigned int btmtk_usb_fops_pollfwlog(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	poll_wait(file, &fw_log_inq, wait);
	if (skb_queue_len(&g_data->fwlog_queue) > 0)
		mask |= POLLIN | POLLRDNORM;			/* readable */

	return mask;
}

/*============================================================================*/
/* Interface Functions : SCO */
/*============================================================================*/
#define ___________________________________________Interface_Function_for_SCO
static ssize_t btmtk_usb_fops_sco_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	int real_num;
	int pos = 0;
	int retval = 0;
	int remain = 0;
	int multiple = 0;
	int i = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	u8 *tmp = NULL;

	if (g_data == NULL || buf == NULL || count <= 0) {
		BTUSB_ERR("%s: ERROR, %s is NULL!", __func__,
				g_data == NULL ? "g_data" : buf == NULL ? "buf" : "count");
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_DBG("%s: current is in suspend/resume (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	BTUSB_DBG("%s start(%d)", __func__, (int)count);
	/* semaphore mechanism, the waited process will sleep */
	down(&g_data->isoc_wr_mtx);
	retval = btmtk_usb_set_isoc_interface(false);
	if (retval < 0 || g_data->isoc_urb_submitted == 0)
		goto OUT;

	g_data->o_sco_buf[0] = HCI_SCODATA_PKT;
	tmp = g_data->o_sco_buf + 1;

	real_num = (BUFFER_SIZE - 1) / ISOC_HCI_PKT_SIZE;
	/* check remain buffer, could send more data but not a whole ISOC_HCI_PKT_SIZE */
	if ((BUFFER_SIZE - 1) > (real_num * ISOC_HCI_PKT_SIZE + HCI_SCO_HDR_SIZE))
		real_num += 1;

	/* Upper layer should take care if write size more then driver buffer */
	if (count > BUFFER_SIZE - 1 - (real_num * HCI_SCO_HDR_SIZE)) {
		BTUSB_WARN("%s: Write length more than driver buffer size(%d/%d)",
				__func__, (int)count, BUFFER_SIZE - 1 - real_num * HCI_SCO_HDR_SIZE);
		count = BUFFER_SIZE - 1 - (real_num * HCI_SCO_HDR_SIZE);
	}

	multiple = count / (ISOC_HCI_PKT_SIZE - HCI_SCO_HDR_SIZE);

	if (count % (ISOC_HCI_PKT_SIZE - HCI_SCO_HDR_SIZE))
		multiple += 1;

	remain = count;
	BTUSB_DBG("remain = %d, multiple = %d", remain, multiple);
	for (i = 0; i < multiple; i++) {
		*tmp = (u8)(sco_handle & 0x00FF);
		*(tmp + 1) = sco_handle >> 8;
		*(tmp + 2) = remain < ISOC_HCI_PKT_SIZE - HCI_SCO_HDR_SIZE
			? remain : ISOC_HCI_PKT_SIZE - HCI_SCO_HDR_SIZE;
		remain -= *(tmp + 2);
		BTUSB_DBG("remain = %d, pkt_len = %d", remain, *(tmp + 2));

		if (copy_from_user(tmp + 3, buf + pos, *(tmp + 2))) {
			retval = -EFAULT;
			BTUSB_ERR("%s: copy data from user fail", __func__);
			goto OUT;
		}
		pos += *(tmp + 2);
		tmp += (3 + *(tmp + 2));
	}
	retval = btmtk_usb_send_data(g_data->o_sco_buf,
			(count - remain) + HCI_SCO_HDR_SIZE * multiple + 1);
	if (retval > 0)
		retval = count;

 OUT:
	up(&g_data->isoc_wr_mtx);
	return retval;
}

static ssize_t btmtk_usb_fops_sco_read(struct file *file, char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	struct sk_buff *skb = NULL;
	unsigned long flags = 0;
	unsigned long ret_len = 0;

	BTUSB_DBG("%s", __func__);
	if (g_data == NULL || buf == NULL || count <= 0) {
		BTUSB_ERR("%s: ERROR, %s is NULL!", __func__,
				g_data == NULL ? "g_data" : buf == NULL ? "buf" : "count");
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_ERR("%s: current is in suspend/resume (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	down(&g_data->isoc_rd_mtx);
	retval = btmtk_usb_set_isoc_interface(false);
	if (retval < 0 || g_data->isoc_urb_submitted == 0)
		goto OUT;

	/* means the buffer is empty */
	while (skb_queue_len(&g_data->isoc_in_queue) == 0) {
		/* If nonblocking mode, return directly O_NONBLOCK is specified during open() */
		if (file->f_flags & O_NONBLOCK) {
			/* BTUSB_DBG("Non-blocking btmtk_usb_fops_read()"); */
			retval = -EAGAIN;
			goto OUT;
		}
		wait_event(BT_sco_wq, skb_queue_len(&g_data->isoc_in_queue) > 0);
	}

	if (skb_queue_len(&g_data->isoc_in_queue) > 0) {
		u32 remain = 0;
		u32 shift = 0;
		u8 real_num = 0;
		u8 i = 1;

		ISOC_SPIN_LOCK(flags);
		skb = skb_dequeue(&g_data->isoc_in_queue);
		ISOC_SPIN_UNLOCK(flags);
		if (skb == NULL) {
			BTUSB_WARN("sbk is NULL");
			goto OUT;
		}

		real_num = skb->len / ISOC_HCI_PKT_SIZE;
		if (skb->len % ISOC_HCI_PKT_SIZE)
			real_num += 1;
		BTUSB_DBG("real_num: %d, mod: %d", real_num, skb->len % ISOC_HCI_PKT_SIZE);
		if (count < skb->len - real_num * HCI_SCO_HDR_SIZE) {
			int num = count / (ISOC_HCI_PKT_SIZE - HCI_SCO_HDR_SIZE);
			struct sk_buff *new_skb = NULL;

			remain = skb->len - num * ISOC_HCI_PKT_SIZE;
			new_skb = alloc_skb(remain, GFP_KERNEL);
			if (new_skb == NULL) {
				BTUSB_WARN("new_skb is NULL");
				goto OUT;
			}

			memcpy(new_skb->data, skb->data + num * ISOC_HCI_PKT_SIZE, remain);
			new_skb->len = remain;
			ISOC_SPIN_LOCK(flags);
			skb_queue_head(&g_data->isoc_in_queue, new_skb);
			ISOC_SPIN_UNLOCK(flags);
		}
		retval = skb->len - remain;	/* Include 3 bytes header */
		shift = 0;
		while (retval > 0) {
			size_t copy = *(skb->data + ((i - 1) * HCI_SCO_HDR_SIZE) + shift + 2);

			ret_len = copy_to_user(buf + shift, skb->data + (i * HCI_SCO_HDR_SIZE) + shift, copy);
			if (ret_len)
				BTUSB_ERR("copy to user fail, copy = %ld, ret_len = %zd, count = %zd",
						ret_len, copy, count);

			shift += copy;
			i++;
			retval -= (HCI_SCO_HDR_SIZE + copy);
			BTUSB_DBG("copy: %d, shift: %d, retval: %d", (int)copy, (int)shift, (int)retval);
		}
		kfree_skb(skb);
		retval = shift;			/* 3 bytes header removed */
	}

OUT:
	up(&g_data->isoc_rd_mtx);
	BTUSB_DBG("Read: %d", (int)retval);
	return retval;
}

static int btmtk_usb_fops_sco_open(struct inode *inode, struct file *file)
{
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	unsigned long flags = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_INIT || state == BTMTK_USB_STATE_DISCONNECT) {
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops not opened!", __func__);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_WARN("%s: current is in suspend/resume (%d).", __func__,
			   state);
		USB_MUTEX_UNLOCK();
		return 0;
	}
	USB_MUTEX_UNLOCK();

	atomic_set(&g_data->isoc_out_count, 0);
	ISOC_SPIN_LOCK(flags);
	skb_queue_purge(&g_data->isoc_in_queue);
	ISOC_SPIN_UNLOCK(flags);
	BTUSB_INFO("%s: OK", __func__);
	return 0;
}

static int btmtk_usb_fops_sco_close(struct inode *inode, struct file *file)
{
	unsigned long flags = 0;
	int ret = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	ret = btmtk_usb_set_isoc_interface(true);

	ISOC_SPIN_LOCK(flags);
	skb_queue_purge(&g_data->isoc_in_queue);
	ISOC_SPIN_UNLOCK(flags);
	if (ret == 0)
		BTUSB_INFO("%s: OK", __func__);
	return ret;
}

static long btmtk_usb_fops_sco_unlocked_ioctl(struct file *file, unsigned int cmd,
					  unsigned long arg)
{
	long retval = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}
	return retval;
}

static unsigned int btmtk_usb_fops_sco_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	if (skb_queue_len(&g_data->isoc_in_queue) == 0) {
		poll_wait(file, &inq_isoc, wait);

		/* empty let select sleep */
		if (skb_queue_len(&g_data->isoc_in_queue) > 0)
			mask |= POLLIN | POLLRDNORM;		/* readable */
	} else
		mask |= POLLIN | POLLRDNORM;			/* readable */

	/* do we need condition? */
	mask |= POLLOUT | POLLWRNORM;				/* writable */

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING)  /* BTMTK_USB_STATE_WORKING: do nothing */
		mask = 0;
	USB_MUTEX_UNLOCK();

	return mask;
}

/*============================================================================*/
/* Interface Functions : Proc */
/*============================================================================*/
#define __________________________________________Interface_Function_for_Proc
static int btmtk_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fw_version_str);
	return 0;
}

static int btmtk_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, btmtk_proc_show, NULL);
}

static int btmtk_chip_reset_delay_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", btmtk_chip_reset_delay);
	return 0;
}

static int btmtk_chip_reset_delay_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, btmtk_chip_reset_delay_proc_show, NULL);
}

static ssize_t btmtk_chip_reset_delay_proc_write(struct file *filp, const char __user *buffer,
                                    size_t count, loff_t *ppos)
{
	u32 delay;

	if (kstrtou32_from_user(buffer, count, 0, &delay))
		return -EFAULT;

	btmtk_chip_reset_delay = delay;

	BTUSB_INFO("%s bt_chip_reset_delay=%d", __func__, btmtk_chip_reset_delay);

	return count;
}

static void btmtk_proc_create_new_entry(void)
{
	struct proc_dir_entry *proc_show_entry;

	BTUSB_INFO("proc initialized");
	/* /proc/stpbt/bt_fw_version */
	g_proc_dir = proc_mkdir("stpbt", 0);
	if (g_proc_dir == 0) {
		BTUSB_INFO("Unable to creat dir");
		return;
	}
	proc_show_entry = proc_create("bt_fw_version", 0644, g_proc_dir, &BT_proc_fops);
	proc_show_entry = proc_create("bt_chip_reset_delay", 0644, g_proc_dir, &BT_chip_reset_delay_proc_fops);
}

/*============================================================================*/
/* Interface Functions : Kernel */
/*============================================================================*/
#define ________________________________________Interface_Function_for_Kernel
static int btmtk_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_endpoint_descriptor *ep_desc;
	int i;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int err = -1;

	probe_counter++;

	BTUSB_INFO("%s: begin", __func__);
	BTUSB_INFO("========================================================");
	BTUSB_INFO("btmtk_usb Mediatek Bluetooth USB driver ver %s", VERSION);
	BTUSB_INFO("========================================================");
	BTUSB_INFO("probe_counter = %d", probe_counter);

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_SUSPEND_DISCONNECT)
		btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_PROBE);
	else if (state == BTMTK_USB_STATE_RESUME_DISCONNECT)
		btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_PROBE);
	else
		btmtk_usb_set_state(BTMTK_USB_STATE_PROBE);
	USB_MUTEX_UNLOCK();

	if (!g_data) {
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		USB_MUTEX_UNLOCK();
		atomic_set(&doing_reset, 0);
		BTUSB_ERR("btmtk_usb_probe end Error 2");
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"g-data-null", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"g-data-null", VITALS_NORMAL);
#endif
		err = -ENOMEM;
		goto reset;
	}

	if (timer_pending(&g_data->chip_rst_disc_timer)) {
		btmtk_del_timer(&g_data->chip_rst_disc_timer);
	}
	atomic_set(&doing_reset, 0);

	/* interface numbers are hardcoded in the spec */
	if (intf->cur_altsetting->desc.bInterfaceNumber != 0) {
		BTUSB_ERR("[ERR] interface number != 0 (%d)", intf->cur_altsetting->desc.bInterfaceNumber);

		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		USB_MUTEX_UNLOCK();

		BTUSB_ERR("btmtk_usb_probe end Error 1");
		return -ENODEV;
	}

	if (timer_pending(&chip_reset_timer)) {
		btmtk_del_timer(&chip_reset_timer);
		btmtk_skb_enq_fwlog(RESET_BT_DONE, strlen(RESET_BT_DONE), 0, &g_data->fwlog_queue);
		wake_up_interruptible(&fw_log_inq);
	}
	btmtk_usb_init_memory();

	/* set the endpoint type of the interface to btmtk_usb_data */
	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		if (usb_endpoint_is_int_in(ep_desc)) {
			g_data->intr_ep = ep_desc;
			continue;
		}

		if (usb_endpoint_is_bulk_out(ep_desc)) {
			g_data->bulk_tx_ep = ep_desc;
			continue;
		}

		if (usb_endpoint_is_bulk_in(ep_desc)) {
			g_data->bulk_rx_ep = ep_desc;
			continue;
		}
	}

	if (!g_data->intr_ep || !g_data->bulk_tx_ep || !g_data->bulk_rx_ep) {
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		USB_MUTEX_UNLOCK();

		BTUSB_ERR("btmtk_usb_probe end Error 3");
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"usb-ep-null", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_PROBE, 1, "count",
				"usb-ep-null", VITALS_NORMAL);
#endif
		err = -ENODEV;
		goto reset;
	}

	g_data->udev = interface_to_usbdev(intf);
	g_data->intf = intf;

	INIT_WORK(&g_data->waker, btmtk_usb_waker);

	g_data->meta_tx = 0;

	/* init all usb anchor */
	init_usb_anchor(&g_data->bulk_out_anchor);
	init_usb_anchor(&g_data->intr_in_anchor);
	init_usb_anchor(&g_data->bulk_in_anchor);
	init_usb_anchor(&g_data->isoc_in_anchor);
	init_usb_anchor(&g_data->isoc_out_anchor);

	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	memset(g_data->metabuffer->buffer, 0, META_BUFFER_SIZE);
	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	btmtk_usb_cap_init();

	err = btmtk_usb_load_rom_patch();
	if (err < 0) {
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		USB_MUTEX_UNLOCK();

		BTUSB_ERR("btmtk_usb_probe end Error 4");
		goto reset;
	}

	/* Interface numbers are hardcoded in the specification */
	g_data->isoc = usb_ifnum_to_if(g_data->udev, 1);

	/* bind isoc interface to usb driver */
	if (g_data->isoc) {
		err = usb_driver_claim_interface(&btmtk_usb_driver, g_data->isoc, g_data);
		if (err < 0) {
			USB_MUTEX_LOCK();
			btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
			USB_MUTEX_UNLOCK();

			BTUSB_ERR("btmtk_usb_probe end Error 7");
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
			log_counter_to_vitals_v2(ANDROID_LOG_INFO,
					BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
					BT_OPERATION, BT_KEY_PROBE, 1, "count",
					"isoc-bind-error", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
			log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
					BT_OPERATION, BT_KEY_PROBE, 1, "count",
					"isoc-bind-error", VITALS_NORMAL);
#endif
			goto reset;
		}
	}

	usb_set_intfdata(intf, g_data);

	sco_handle = 0;
	g_data->isoc_urb_submitted = 0;
	memset((void *)&host_le_scan, 0, sizeof(struct le_scan_parm_s));
	memset(g_data->bdaddr, 0, BD_ADDRESS_SIZE);
	if (is_support_unify_woble(g_data)) {
		btmtk_usb_load_woble_setting(g_data->woble_setting_file_name,
			&g_data->udev->dev,
			&g_data->woble_setting_len,
			g_data);
		if (need_reset_stack)
			btmtk_usb_reset_power_on();
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	switch (state) {
	case BTMTK_USB_STATE_SUSPEND_FW_DUMP:
		BTUSB_INFO("%s State is BTMTK_USB_STATE_SUSPEND_FW_DUMP", __func__);
		break;

	case BTMTK_USB_STATE_RESUME_FW_DUMP:
		BTUSB_INFO("%s State is BTMTK_USB_STATE_RESUME_FW_DUMP", __func__);
		break;

	case BTMTK_USB_STATE_SUSPEND_PROBE:
	case BTMTK_USB_STATE_RESUME_PROBE:
		BTUSB_WARN("%s State is %d", __func__, state);
	default:
		btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
	}
	USB_MUTEX_UNLOCK();

	if (need_reset_stack) {
		BTUSB_INFO("%s: need_reset_stack %d", __func__, need_reset_stack);
		wake_up_interruptible(&inq);
	}

	btmtk_usb_woble_wake_unlock(g_data);

	/* Init Hci Snoop */
	btmtk_usb_hci_snoop_init();

	BTUSB_INFO("%s: end", __func__);
	return 0;

reset:
	btmtk_usb_toggle_rst_pin();
	return err;
}

static int btmtk_usb_L0_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	int ret = 0;
	/* Set flags/functions here to leave HW reset mark before probe. */

	ret = btmtk_usb_probe(intf, id);
	if (ret) {
		BTUSB_ERR("btmtk_usb_L0_probe failed, ret %d", ret);
		goto exit;
	}

	if (need_reset_stack_type != HW_ERR_NONE)
		need_reset_stack = need_reset_stack_type;
	else {
		BTUSB_INFO("%s: need_reset_stack_type is HW_ERR_NONE when do chip reset", __func__);
		need_reset_stack = HW_ERR_CODE_BT_DRIVER;
	}

	BTUSB_INFO("need_reset_stack %d probe_ret %d", need_reset_stack, ret);
	wake_up_interruptible(&inq);

	btmtk_usb_L0_hook_new_probe(btmtk_usb_probe);

 exit:
	return ret;
}

static void btmtk_usb_disconnect(struct usb_interface *intf)
{
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (!usb_get_intfdata(intf))
		return;

	BTUSB_INFO("%s: begin", __func__);

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_SUSPEND || state == BTMTK_USB_STATE_SUSPEND_DISCONNECT) {
		if (register_late_resume_func) {
			BTUSB_WARN("%s: state=%d disc happens in suspend --> should stay in suspend state later!",
					__func__, state);
			btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_DISCONNECT);
		} else {
			BTUSB_WARN("%s: state=%d, disc happens in suspend state --> go to disconnect state!",
					__func__, state);
			btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		}
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_BUS_DISC, 1, "count",
				"in-suspend", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_BUS_DISC, 1, "count",
				"in-suspend", VITALS_NORMAL);
#endif
	} else if (state == BTMTK_USB_STATE_RESUME || state == BTMTK_USB_STATE_RESUME_FW_DUMP ||
			state == BTMTK_USB_STATE_RESUME_DISCONNECT) {
		BTUSB_WARN("%s: state=%d disc happens when driver is in resume, should stay in resume state later!",
				__func__, state);
		btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_DISCONNECT);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_BUS_DISC, 1, "count",
				"in-resume", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_BUS_DISC, 1, "count",
				"in-resume", VITALS_NORMAL);
#endif
	} else {
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_BUS_DISC, 1, "count",
				"in-other-state", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_BUS_DISC, 1, "count",
				"in-other-state", VITALS_NORMAL);
#endif
	}
	USB_MUTEX_UNLOCK();

	if (is_mt7668(g_data))
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_OFF;

	if (!g_data)
		return;

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (atomic_read(&doing_reset) == 0 &&
		(fstate == BTMTK_FOPS_STATE_OPENED || fstate == BTMTK_FOPS_STATE_CLOSING)) {
		BTUSB_WARN("%s: fstate = %d, set need_reset_stack to HW_ERR_CODE_USB_DISC", __func__, fstate);
		if (need_reset_stack == HW_ERR_NONE)
			need_reset_stack = HW_ERR_CODE_USB_DISC;
	}
	FOPS_MUTEX_UNLOCK();

	usb_set_intfdata(g_data->intf, NULL);

	if (g_data->isoc)
		usb_set_intfdata(g_data->isoc, NULL);

	if (intf == g_data->isoc)
		usb_driver_release_interface(&btmtk_usb_driver, g_data->intf);
	else if (g_data->isoc)
		usb_driver_release_interface(&btmtk_usb_driver, g_data->isoc);

	g_data->meta_tx = 0;
	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	cancel_work_sync(&g_data->waker);

	btmtk_usb_woble_free_setting();

	//atomic_set(&doing_reset, 0);

	if (timer_pending(&g_data->fw_dump_timer)) {
		btmtk_del_timer(&g_data->fw_dump_timer);
	}

	if (timer_pending(&g_data->chip_rst_disc_timer)) {
		btmtk_del_timer(&g_data->chip_rst_disc_timer);
	}

	BTUSB_INFO("%s: end", __func__);
}

static int btmtk_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
	int ret = -1;

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND);
	USB_MUTEX_UNLOCK();

	if ((g_data->suspend_count++)) {
		BTUSB_WARN("%s: Has suspended. suspend_count: %d end", __func__, g_data->suspend_count);
		return 0;
	}

	BTUSB_INFO("%s: begin", __func__);
	btmtk_usb_stop_traffic();
	usb_kill_anchored_urbs(&g_data->bulk_out_anchor);
	usb_kill_anchored_urbs(&g_data->isoc_out_anchor);

	if (!is_support_unify_woble(g_data)) {
		ret = btmtk_usb_handle_entering_WoBLE_state();
		if (ret)
			BTUSB_ERR("%s: btmtk_usb_handle_entering_WoBLE_state return fail  %d", __func__, ret);

	} else if (register_early_suspend_func == NULL) {
		/* no early suspend OS, call btmtk_usb_unify_woble_suspend here */
		ret = btmtk_usb_unify_woble_suspend(g_data);
	} else {
		BTUSB_INFO("%s: No unify woble & early suspend, do nothing", __func__);
		ret = 0;
	}

	BTUSB_INFO("%s: end(%d)", __func__, ret);

	return ret;
}

static int btmtk_usb_resume(struct usb_interface *intf)
{
	int ret = 0;

	g_data->suspend_count--;
	if (g_data->suspend_count) {
		BTUSB_WARN("%s: data->suspend_count %d, return 0", __func__, g_data->suspend_count);
		return 0;
	}

	BTUSB_INFO("%s: begin", __func__);

	if (is_mt7668(g_data) && g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_ERROR) {
		BTUSB_INFO("%s: In BTMTK_USB_7668_DONGLE_STATE_ERROR(Could suspend caused), do assert", __func__);
		btmtk_usb_send_assert_cmd();
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_WOBLE, 1, "count",
				"dongle-state-error", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_WOBLE, 1, "count",
				"dongle-state-error", VITALS_NORMAL);
#endif
		return -EBADFD;
	} else if (is_mt7668(g_data) && g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_WOBLE) {
		BTUSB_INFO("%s: is_mt7668_dongle_state %d return", __func__, g_data->is_mt7668_dongle_state);
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
		USB_MUTEX_UNLOCK();
		return 0;
	} else if (need_reset_stack) {
		BTUSB_INFO("%s: need_reset_stack (%d)!", __func__, need_reset_stack);
		wake_up_interruptible(&inq);
	}

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_RESUME);
	USB_MUTEX_UNLOCK();

	ret = btmtk_usb_handle_resume();
	if (ret) {
		/* avoid rtc to to suspend again, do FW dump first */
		btmtk_usb_woble_wake_lock(g_data);
		BTUSB_ERR("%s: do assert", __func__);
		btmtk_usb_send_assert_cmd();
#ifdef CONFIG_AMAZON_MINERVA_METRICS_LOG
		log_counter_to_vitals_v2(ANDROID_LOG_INFO,
				BT_GROUP_ID, BT_SCHEMA_ID, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_WOBLE, 1, "count",
				"leave-woble-fail", VITALS_NORMAL, NULL, NULL);
#elif defined(CONFIG_AMAZON_METRICS_LOG)
		log_counter_to_vitals(ANDROID_LOG_INFO, BT_DOMAIN, BT_PROGRAM,
				BT_OPERATION, BT_KEY_WOBLE, 1, "count",
				"leave-woble-fail", VITALS_NORMAL);
#endif
	}

	BTUSB_INFO("%s: end(%d)", __func__, ret);
	return ret;
}

#if !BT_DISABLE_RESET_RESUME
static int btmtk_usb_reset_resume(struct usb_interface *intf)
{
	BTUSB_INFO("%s: Call resume directly", __func__);
	return btmtk_usb_resume(intf);
}
#endif

static struct usb_driver btmtk_usb_driver = {
	.name = "btmtk_usb",
	.probe = btmtk_usb_probe,
	.disconnect = btmtk_usb_disconnect,
	.suspend = btmtk_usb_suspend,
	.resume = btmtk_usb_resume,
#if !BT_DISABLE_RESET_RESUME
	.reset_resume = btmtk_usb_reset_resume,
#endif
	.id_table = btmtk_usb_table,
	.supports_autosuspend = 1,
	.disable_hub_initiated_lpm = 1,
};

static int __init btmtk_usb_init(void)
{
	int retval = 0;

	BTUSB_INFO("%s: btmtk usb driver ver %s", __func__, VERSION);

	retval = btmtk_usb_BT_init();
	if (retval < 0)
		return retval;

	btmtk_fifo_start(g_data->bt_fifo);

	retval = usb_register(&btmtk_usb_driver);
	if (retval)
		BTUSB_INFO("%s: usb registration failed!(%d)", __func__, retval);
	else
		BTUSB_INFO("%s: usb registration success.", __func__);

	return retval;
}

static void __exit btmtk_usb_exit(void)
{
	BTUSB_INFO("%s: btmtk usb driver ver %s", __func__, VERSION);

	usb_deregister(&btmtk_usb_driver);
	btmtk_usb_BT_exit();
}

module_init(btmtk_usb_init);
module_exit(btmtk_usb_exit);

/**
 * Module information
 */
MODULE_DESCRIPTION("Mediatek Bluetooth USB driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
