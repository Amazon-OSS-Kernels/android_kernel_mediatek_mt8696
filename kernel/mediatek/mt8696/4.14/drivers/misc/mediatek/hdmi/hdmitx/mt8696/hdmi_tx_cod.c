/*
 * Current overdraw detection logic
 */

#include <linux/iio/consumer.h>
#include <linux/iio/types.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/err.h>
#include <linux/printk.h>
#include "hdmi_tx_cod.h"
#include "hdmi_ioctl.h"
#include "../../../thermal/mt8696/inc/tmp_bts.h"

#define COD_RATE 2*HZ
#define COD_CHANNEL 2
#define COD_HDMIV_LIMIT 2094

static int cnt, det, state;
static struct iio_channel *channels;
static struct workqueue_struct *hdmi_cod_wq;
static struct delayed_work work_cod;

static struct notify_dev hdmi_cod_switch_data;

static int get_hdmiv(unsigned int ch)
{
	int ret = 0, val = 0;

#ifdef CONFIG_BTS_THERMISTOR
	ret = adc_cod_read(COD_CHANNEL, &val);
#endif
	if (ret < 0) {
		pr_err("IIO channel read failed %d\n", ret);
		return ret;
	}
	pr_debug("IIO channel [%d] read %d\n", COD_CHANNEL, val);

	return val;
}

static void work_cod_handler(struct work_struct *work)
{
	int mv;
	mv = get_hdmiv(COD_CHANNEL);

	// Spec allows up to 50mA to be drawn by the sink
	// Follow Mantis, Set threshold to 4.25V which is 2094.
	if (state ? (mv >= COD_HDMIV_LIMIT) : (mv < COD_HDMIV_LIMIT)) {
		det++;
	}
	hdmi_cod_switch_data.value = mv;
	if (++cnt < 10) {
		if (det > 0) {
			state = !state;
			pr_warn("current overdraw detection: %d\n", state);
			notify_uevent_user(&hdmi_cod_switch_data, state ? mv : 0);
			cnt = det = 0;
		}
	} else {
		cnt = det = 0;
	}

	queue_delayed_work(hdmi_cod_wq, &work_cod, COD_RATE);
}

void cod_init(struct platform_device *pdev)
{
	int ret = 0;

	channels = iio_channel_get_all(&pdev->dev);
	if (IS_ERR(channels))
		pr_err("get all iio channel failed!\n");

	hdmi_cod_wq = alloc_workqueue("cod", WQ_HIGHPRI | WQ_CPU_INTENSIVE, 0);

	hdmi_cod_switch_data.name = "hdmi_cod";
	hdmi_cod_switch_data.index = 0;
	hdmi_cod_switch_data.state = 0;
	ret = hdmitx_uevent_dev_register(&hdmi_cod_switch_data);
	if (ret < 0) {
		pr_err("Register hdmi swtich cod failed %d\n", ret);
	}

	INIT_DELAYED_WORK(&work_cod, work_cod_handler);
}

void cod_test(void)
{
	cancel_delayed_work_sync(&work_cod);
	notify_uevent_user(&hdmi_cod_switch_data, 0);
	cnt = det = state = 0;
	queue_delayed_work(hdmi_cod_wq, &work_cod, COD_RATE);
}

void cod_fini(void)
{
	iio_channel_release(channels);
	cancel_delayed_work_sync(&work_cod);
	hdmitx_uevent_dev_unregister(&hdmi_cod_switch_data);
}

