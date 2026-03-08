// SPDX-License-Identifier: GPL-2.0+
/*
 *	uvc_v4l2.c  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/usb/g_uvc.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>

#include <linux/sync_file.h>
#include <linux/dma-fence.h>

#include "f_uvc.h"
#include "uvc.h"
#include "uvc_queue.h"
#include "uvc_video.h"
#include "uvc_v4l2.h"
//#include "../../../media/platform/mtk-jpeg/mtk_sync.h"
#include <mtk_sync.h>

/* --------------------------------------------------------------------------
 * Requests handling
 */

static int
uvc_send_response(struct uvc_device *uvc, struct uvc_request_data *data)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct usb_request *req = uvc->control_req;

	if (data->length < 0)
		return usb_ep_set_halt(cdev->gadget->ep0);

	req->length = min_t(unsigned int, uvc->event_length, data->length);
	req->zero = data->length < uvc->event_length;

	memcpy(req->buf, data->data, req->length);

	return usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
}

/* --------------------------------------------------------------------------
 * V4L2 ioctls
 */
#define UVC_FORMAT_NV12_SUPPORT			0

struct uvc_format {
	u8 bpp;
	u32 fcc;
};

static struct uvc_format uvc_formats[] = {
	{ 16, V4L2_PIX_FMT_YUYV  },
	{ 0,  V4L2_PIX_FMT_MJPEG },
	#if UVC_FORMAT_NV12_SUPPORT
	{ 12, V4L2_PIX_FMT_NV12M  },
	#endif
};

static int
uvc_v4l2_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct usb_composite_dev *cdev = uvc->func.config->cdev;

	strlcpy(cap->driver, "g_uvc", sizeof(cap->driver));
	strlcpy(cap->card, cdev->gadget->name, sizeof(cap->card));
	strlcpy(cap->bus_info, dev_name(&cdev->gadget->dev),
		sizeof(cap->bus_info));

	cap->device_caps = V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int
uvc_v4l2_get_format(struct file *file, void *fh, struct v4l2_format *fmt)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;

	fmt->fmt.pix.pixelformat = video->fcc;
	fmt->fmt.pix.width = video->width;
	fmt->fmt.pix.height = video->height;
	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = video->bpp * video->width / 8;
	fmt->fmt.pix.sizeimage = video->imagesize;
	fmt->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	fmt->fmt.pix.priv = 0;

	return 0;
}

static int
uvc_v4l2_get_format_m(struct file *file, void *fh, struct v4l2_format *fmt)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;

	fmt->fmt.pix_mp.width = video->width;
	fmt->fmt.pix_mp.height = video->height;
	fmt->fmt.pix_mp.pixelformat = video->fcc;
	fmt->fmt.pix_mp.field = V4L2_FIELD_NONE;
	fmt->fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;

	if (video->fcc == V4L2_PIX_FMT_NV12M) {
		fmt->fmt.pix_mp.num_planes = 2;
		fmt->fmt.pix_mp.plane_fmt[0].bytesperline =  video->width;

		fmt->fmt.pix_mp.plane_fmt[1].bytesperline = video->width / 2;

		if (video->width == 640) {
			/*video->imagesize * 2 / 3*/
			fmt->fmt.pix_mp.plane_fmt[0].sizeimage = 640*384;
			/*video->imagesize / 3*/
			fmt->fmt.pix_mp.plane_fmt[1].sizeimage = 640*384/2;
		} else if (video->width == 1280) {
			fmt->fmt.pix_mp.plane_fmt[0].sizeimage = 1280*736;
			fmt->fmt.pix_mp.plane_fmt[1].sizeimage = 1280*736/2;
		}
		pr_debug("uvc v4l2 get format m %d %d\n", fmt->fmt.pix_mp.plane_fmt[0].bytesperline,
			 fmt->fmt.pix_mp.plane_fmt[1].bytesperline);
	} else {
		fmt->fmt.pix_mp.num_planes = 1;
		fmt->fmt.pix_mp.plane_fmt[0].bytesperline = video->bpp * video->width / 8;
		fmt->fmt.pix_mp.plane_fmt[0].sizeimage = video->imagesize;
	}

	return 0;
}

static int uvc_enum_fmt(struct file *file, void *fh,
				      struct v4l2_fmtdesc *fmt)
{
	u32 f_index = 0;

	if (fmt->index >= ARRAY_SIZE(uvc_formats))
		return -EINVAL;

	f_index = fmt->index;
	memset(fmt, 0, sizeof(*fmt));
	fmt->index = f_index;
	fmt->pixelformat = uvc_formats[fmt->index].fcc;
	pr_debug("fmt->index %d, fmt->pixelformat %d\n", fmt->index, fmt->pixelformat);
	return 0;
}

static int
uvc_v4l2_set_format(struct file *file, void *fh, struct v4l2_format *fmt)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	struct uvc_format *format;
	unsigned int imagesize;
	unsigned int bpl;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(uvc_formats); ++i) {
		format = &uvc_formats[i];
		if (format->fcc == fmt->fmt.pix.pixelformat)
			break;
	}

	if (i == ARRAY_SIZE(uvc_formats)) {
		uvcg_info(&uvc->func, "Unsupported format 0x%08x.\n",
			fmt->fmt.pix.pixelformat);
		return -EINVAL;
	}

	bpl = format->bpp * fmt->fmt.pix.width / 8;
	imagesize = bpl ? bpl * fmt->fmt.pix.height : fmt->fmt.pix.sizeimage;

	video->fcc = format->fcc;
	video->bpp = format->bpp;
	video->width = fmt->fmt.pix.width;
	video->height = fmt->fmt.pix.height;
	video->imagesize = imagesize;

	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = bpl;
	fmt->fmt.pix.sizeimage = imagesize;
	fmt->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	fmt->fmt.pix.priv = 0;

	return 0;
}

static int
uvc_v4l2_set_format_m(struct file *file, void *fh, struct v4l2_format *fmt)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	struct uvc_format *format;
	unsigned int imagesize;
	unsigned int bpl;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(uvc_formats); ++i) {
		format = &uvc_formats[i];
		if (format->fcc == fmt->fmt.pix_mp.pixelformat)
			break;
	}

	if (i == ARRAY_SIZE(uvc_formats)) {
		pr_debug("Unsupported format 0x%08x.\n",
			fmt->fmt.pix_mp.pixelformat);
		return -EINVAL;
	}

	if (fmt->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_NV12M) {
		imagesize = fmt->fmt.pix_mp.width * fmt->fmt.pix_mp.height * 3 / 2;
		fmt->fmt.pix_mp.plane_fmt[0].bytesperline = fmt->fmt.pix_mp.width;
		fmt->fmt.pix_mp.plane_fmt[1].bytesperline = fmt->fmt.pix_mp.width/2;
		if (fmt->fmt.pix_mp.width == 640) {
			fmt->fmt.pix_mp.plane_fmt[0].sizeimage = 640*384;
			fmt->fmt.pix_mp.plane_fmt[1].sizeimage = 640*384/2;
		} else if (video->width == 1280) {
			fmt->fmt.pix_mp.plane_fmt[0].sizeimage = 1280*736;
			fmt->fmt.pix_mp.plane_fmt[1].sizeimage = 1280*736/2;
		}
		pr_debug("uvc set format_m %d %d\n", fmt->fmt.pix_mp.plane_fmt[0].bytesperline,
			 fmt->fmt.pix_mp.plane_fmt[1].bytesperline);
	} else {
		bpl = format->bpp * fmt->fmt.pix_mp.width / 8;
		if (bpl)
			imagesize = bpl * fmt->fmt.pix_mp.height;
		else
			imagesize = fmt->fmt.pix_mp.plane_fmt[0].sizeimage;
		fmt->fmt.pix_mp.plane_fmt[0].bytesperline = bpl;
		fmt->fmt.pix_mp.plane_fmt[0].sizeimage = imagesize;
	}


	video->fcc = format->fcc;
	video->bpp = format->bpp;
	video->width = fmt->fmt.pix_mp.width;
	video->height = fmt->fmt.pix_mp.height;
	video->imagesize = imagesize;

	fmt->fmt.pix_mp.field = V4L2_FIELD_NONE;

	fmt->fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;

	return 0;
}

static int
uvc_v4l2_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;

	if (b->type != video->queue.queue.type)
		return -EINVAL;

	return uvcg_alloc_buffers(&video->queue, b);
}

static int
uvc_v4l2_querybuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;

	return uvcg_query_buffer(&video->queue, b);
}

static int
uvc_v4l2_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	int ret;

	if (b->flags & V4L2_BUF_FLAG_IN_FENCE) {
		struct dma_fence *fence = NULL;
		signed long timeout = 5*HZ;
		struct mtk_v4l2_fence_data *buf_fence = NULL;

		if (!b->fence_fd) {
			pr_info("%s %d fence_fd is 0\n", __func__, __LINE__);
			return -EINVAL;
		}

		fence = sync_file_get_fence(b->fence_fd);
		if (!fence) {
			pr_info("%s dma_fence is NULL,fd %d\n", __func__, b->fence_fd);
			return -EINVAL;
		}

		ret = dma_fence_wait_timeout(fence, false, timeout);
		if (ret <= 0) {
			dma_fence_put(fence);
			pr_info("%s fence timeout ret %d fd %d\n", __func__, ret, b->fence_fd);
			return -EBUSY;
		}

		dma_fence_put(fence);
		if (video->fcc == V4L2_PIX_FMT_MJPEG) {
			buf_fence = container_of(fence, struct mtk_v4l2_fence_data, base);
			b->m.planes[0].bytesused = buf_fence->length[0];
		}

		ret = uvcg_queue_buffer(&video->queue, b);
		if (ret < 0) {
			pr_info("%s %d uvcg_queue_buffer ret %d\n", __func__, __LINE__, ret);
			return ret;
		}
	} else {
		ret = uvcg_queue_buffer(&video->queue, b);
		if (ret < 0)
			return ret;
	}

	schedule_work(&video->pump);

	return ret;
}

static int
uvc_v4l2_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;

	return uvcg_dequeue_buffer(&video->queue, b, file->f_flags & O_NONBLOCK);
}

static int
uvc_v4l2_streamon(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	int ret;

	if (type != video->queue.queue.type)
		return -EINVAL;

	/* Enable UVC video. */
	ret = uvcg_video_enable(video, 1);
	if (ret < 0)
		return ret;

	/*
	 * Complete the alternate setting selection setup phase now that
	 * userspace is ready to provide video frames.
	 */
	uvc_function_setup_continue(uvc);
	uvc->state = UVC_STATE_STREAMING;

	return 0;
}

static int
uvc_v4l2_streamoff(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;

	if (type != video->queue.queue.type)
		return -EINVAL;

	return uvcg_video_enable(video, 0);
}

static int
uvc_v4l2_subscribe_event(struct v4l2_fh *fh,
			 const struct v4l2_event_subscription *sub)
{
	if (sub->type < UVC_EVENT_FIRST || sub->type > UVC_EVENT_LAST)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, 2, NULL);
}

static int
uvc_v4l2_unsubscribe_event(struct v4l2_fh *fh,
			   const struct v4l2_event_subscription *sub)
{
	return v4l2_event_unsubscribe(fh, sub);
}

static long
uvc_v4l2_ioctl_default(struct file *file, void *fh, bool valid_prio,
		       unsigned int cmd, void *arg)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);

	switch (cmd) {
	case UVCIOC_SEND_RESPONSE:
		return uvc_send_response(uvc, arg);

	default:
		return -ENOIOCTLCMD;
	}
}

static int uvc_v4l2_enum_framesizes(struct file *file, void *fh,
				     struct v4l2_frmsizeenum *fsize)
{

	pr_debug("%s %d %d\n", __func__, fsize->pixel_format, fsize->index);
	if (fsize->pixel_format == V4L2_PIX_FMT_MJPEG) {
		switch (fsize->index) {
		case 0:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 640;
			fsize->discrete.height = 360;
			break;
		case 1:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 1280;
			fsize->discrete.height = 720;
			break;
		case 2:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 1920;
			fsize->discrete.height = 1080;
			break;
		case 3:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 3840;
			fsize->discrete.height = 2160;
			break;
		default:
			return -EINVAL;
		}
	} else if (fsize->pixel_format == V4L2_PIX_FMT_YUYV) {
		switch (fsize->index) {
		case 0:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 640;
			fsize->discrete.height = 360;
			break;
		case 1:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 1280;
			fsize->discrete.height = 720;
			break;
		case 2:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 1920;
			fsize->discrete.height = 1080;
			break;
		default:
			return -EINVAL;
		}
	} else if (fsize->pixel_format == V4L2_PIX_FMT_NV12M) {
		switch (fsize->index) {
		case 0:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 640;
			fsize->discrete.height = 360;
			break;
		case 1:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 1280;
			fsize->discrete.height = 720;
			break;
		case 2:
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = 1920;
			fsize->discrete.height = 1080;
			break;
		default:
			return -EINVAL;
		}
	}
	return 0;
}

static int uvc_v4l2_enum_frameintervals(struct file *file, void *fh,
					 struct v4l2_frmivalenum *fival)
{

	if (fival->index > 2)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = 15;

	return 0;
}

const struct v4l2_ioctl_ops uvc_v4l2_ioctl_ops = {
	.vidioc_querycap = uvc_v4l2_querycap,
	.vidioc_enum_fmt_vid_out = uvc_enum_fmt,
	.vidioc_g_fmt_vid_out = uvc_v4l2_get_format,
	.vidioc_s_fmt_vid_out = uvc_v4l2_set_format,
	//.vidioc_enum_fmt_vid_out_mplane = uvc_enum_fmt_m,
	.vidioc_g_fmt_vid_out_mplane = uvc_v4l2_get_format_m,
	.vidioc_s_fmt_vid_out_mplane = uvc_v4l2_set_format_m,
	.vidioc_reqbufs = uvc_v4l2_reqbufs,
	.vidioc_querybuf = uvc_v4l2_querybuf,
	.vidioc_qbuf = uvc_v4l2_qbuf,
	.vidioc_dqbuf = uvc_v4l2_dqbuf,
	.vidioc_streamon = uvc_v4l2_streamon,
	.vidioc_streamoff = uvc_v4l2_streamoff,
	.vidioc_subscribe_event = uvc_v4l2_subscribe_event,
	.vidioc_unsubscribe_event = uvc_v4l2_unsubscribe_event,
	.vidioc_default = uvc_v4l2_ioctl_default,
	.vidioc_enum_framesizes = uvc_v4l2_enum_framesizes,
	.vidioc_enum_frameintervals = uvc_v4l2_enum_frameintervals,
};

/* --------------------------------------------------------------------------
 * V4L2
 */

static int
uvc_v4l2_open(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_file_handle *handle;
	int uvc_ret = 0;
	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL)
		return -ENOMEM;

	v4l2_fh_init(&handle->vfh, vdev);
	v4l2_fh_add(&handle->vfh);

	handle->device = &uvc->video;
	file->private_data = &handle->vfh;

	uvc_ret = uvc_function_connect(uvc);
	return uvc_ret;
}

static int
uvc_v4l2_release(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_file_handle *handle = to_uvc_file_handle(file->private_data);
	struct uvc_video *video = handle->device;

	uvc_function_disconnect(uvc);

	mutex_lock(&video->mutex);
	uvcg_video_enable(video, 0);
	uvcg_free_buffers(&video->queue);
	mutex_unlock(&video->mutex);

	file->private_data = NULL;
	v4l2_fh_del(&handle->vfh);
	v4l2_fh_exit(&handle->vfh);
	kfree(handle);

	return 0;
}

static int
uvc_v4l2_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);

	return uvcg_queue_mmap(&uvc->video.queue, vma);
}

static __poll_t
uvc_v4l2_poll(struct file *file, poll_table *wait)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);

	return uvcg_queue_poll(&uvc->video.queue, file, wait);
}

#ifndef CONFIG_MMU
static unsigned long uvcg_v4l2_get_unmapped_area(struct file *file,
		unsigned long addr, unsigned long len, unsigned long pgoff,
		unsigned long flags)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);

	return uvcg_queue_get_unmapped_area(&uvc->video.queue, pgoff);
}
#endif

const struct v4l2_file_operations uvc_v4l2_fops = {
	.owner		= THIS_MODULE,
	.open		= uvc_v4l2_open,
	.release	= uvc_v4l2_release,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= uvc_v4l2_mmap,
	.poll		= uvc_v4l2_poll,
#ifndef CONFIG_MMU
	.get_unmapped_area = uvcg_v4l2_get_unmapped_area,
#endif
};

