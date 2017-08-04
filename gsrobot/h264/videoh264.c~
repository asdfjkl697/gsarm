#include "videoh264.h"
#include "v4l2uvc.h"
#include "h264_xu_ctrls.h"
#include "nalu.h"
#include "debug.h"
#include "cap_desc_parser.h"
#include "cap_desc.h"

#define TESTAP_VERSION		"v1.0.14.0_H264_UVC_TestAP_Multi"

#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif

#define V4L2_PIX_FMT_H264 v4l2_fourcc('H','2','6','4') /* H264 */
#define V4L2_PIX_FMT_MP2T v4l2_fourcc('M','P','2','T') /* MPEG-2 TS */

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)<<16+(b)<<8+(c))
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,32)
#define V4L_BUFFERS_DEFAULT	6//16
#define V4L_BUFFERS_MAX		16//32
#else
#define V4L_BUFFERS_DEFAULT	3
#define V4L_BUFFERS_MAX		3
#endif

#define H264_SIZE_HD				((1280<<16)|720)
#define H264_SIZE_VGA				((640<<16)|480)
#define H264_SIZE_QVGA				((320<<16)|240)
#define H264_SIZE_QQVGA				((160<<16)|112)
#define H264_SIZE_360P				((640<<16)|360)
#define H264_SIZE_180P				((320<<16)|180)

#define MULTI_STREAM_HD_QVGA		0x01
#define MULTI_STREAM_HD_180P		0x02
#define MULTI_STREAM_HD_360P	   0x04
#define MULTI_STREAM_HD_VGA	   0x08
#define MULTI_STREAM_HD_QVGA_VGA 0x10

#define MULTI_STREAM_QVGA_VGA	    0x20
#define MULTI_STREAM_HD_180P_360P	0x40
#define MULTI_STREAM_360P_180P	    0x80

struct H264Format *gH264fmt = NULL;
int Dbg_Param = 0x1f;

int video_get_framerate(int dev, int *framerate) {
	struct v4l2_streamparm parm;
	int ret;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(dev, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		TestAp_Printf(TESTAP_DBG_ERR, "Unable to get frame rate: %d.\n", errno);
		return ret;
	}

	TestAp_Printf(TESTAP_DBG_FLOW, "Current frame rate: %u/%u\n",
			parm.parm.capture.timeperframe.numerator,
			parm.parm.capture.timeperframe.denominator);
	*framerate = parm.parm.capture.timeperframe.denominator;

	return 0;
}

void videoh264init()
{
	unsigned int i;
	int framerate = 30;
	/* Video buffers */
	void *mem0[V4L_BUFFERS_MAX];
	void *mem1[V4L_BUFFERS_MAX];
	unsigned int pixelformat = V4L2_PIX_FMT_MJPEG;
	unsigned int width = 1280;
	unsigned int height = 720;
	unsigned int nbufs = V4L_BUFFERS_DEFAULT;
	unsigned int input = 0;
	unsigned int skip = 0;

	struct v4l2_buffer buf0;
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers rb;
	int dev, ret;

	dev = open("/dev/video1", O_RDWR);
	memset(&cap, 0, sizeof cap);
	ioctl(dev, VIDIOC_QUERYCAP, &cap);

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	ioctl(dev, VIDIOC_S_FMT, &fmt);

	struct v4l2_streamparm parm;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ioctl(dev, VIDIOC_G_PARM, &parm);
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = framerate;

	ioctl(dev, VIDIOC_S_PARM, &parm);

	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rb.memory = V4L2_MEMORY_MMAP;

	ioctl(dev, VIDIOC_REQBUFS, &rb);
	nbufs = rb.count;

	for (i = 0; i < nbufs; ++i) {
		memset(&buf0, 0, sizeof buf0);
		buf0.index = i;
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		ioctl(dev, VIDIOC_QUERYBUF, &buf0);

		mem0[i] = mmap(0, buf0.length, PROT_READ, MAP_SHARED, dev,
				buf0.m.offset);
	}
	for (i = 0; i < nbufs; ++i) {
		memset(&buf0, 0, sizeof buf0);
		buf0.index = i;
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(dev, VIDIOC_QBUF, &buf0);
	}

	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(dev, VIDIOC_STREAMON, &type);

	FILE *rec_fp = fopen("Record.H264", "wb");
	while (1) {
		memset(&buf0, 0, sizeof buf0);
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;

		ioctl(dev, VIDIOC_DQBUF, &buf0);
		fwrite(mem0[buf0.index], buf0.bytesused, 1, rec_fp);
		ioctl(dev, VIDIOC_QBUF, &buf0);
	}
}

