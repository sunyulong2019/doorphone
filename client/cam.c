///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/client/cam.c
//  日期: 2017-9
//  描述: 提供摄像头子模块功能
//
//  作者: Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////

#include "cam.h"
#include "lcd.h"

camera_info *g_caminfo = NULL;

/*
*** 功能：显示当前摄像头的参数规格
*** 参数：无
*** 返回：无
*/
void cam_info_display(void)
{
	printf("\n↓↓↓↓↓↓↓ 摄像头参数 ↓↓↓↓↓↓↓\n");

	// 显示摄像头的“能力集”
	struct v4l2_capability cap;
	if(ioctl(g_caminfo->camfd, VIDIOC_QUERYCAP, &cap) == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: ioctl() failed: %s\n",
			__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}
	printf("driver:%s\n", cap.driver);
	printf("card:  %s\n", cap.card);
	printf("bus_info:%s\n", cap.bus_info);
	printf("version: %08X\n", cap.version);
	printf("capabilities: %08X\n", cap.capabilities);

	// 显示摄像头的数据格式
	struct v4l2_format fmt;
	bzero(&fmt, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(g_caminfo->camfd, VIDIOC_G_FMT, &fmt) == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: ioctl() failed: %s\n",
			__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	/* 参考：/usr/include/linux/videodev2.h:131 */
	printf("type:  %s\n", fmt.type==1 ? "V4L2_BUF_TYPE_VIDEO_CAPTURE" : "unknown");
	printf("width: %d, ", g_caminfo->width = fmt.fmt.pix.width);
	printf("height:%d\n", g_caminfo->height= fmt.fmt.pix.height);

	char fmtstr[8] = {0};
	memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
	printf("pixelformat: %s\n", fmtstr);
	printf("field: %d\n", fmt.fmt.pix.field /* 参考：/usr/include/linux/videodev2.h:85 */);
	printf("bytesperline: %d\n", fmt.fmt.pix.bytesperline);
	printf("sizeimage: %d\n", fmt.fmt.pix.sizeimage);

	/* 参考：/usr/include/linux/videodev2.h:183 */
	printf("colorspace: %s\n", fmt.fmt.pix.colorspace==7 ? "V4L2_COLORSPACE_JPEG" : "unknown");
	printf("priv: %d\n", fmt.fmt.pix.priv);

	// 显示当前FPS
	struct v4l2_streamparm parm;
	bzero(&parm, sizeof(parm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(g_caminfo->camfd, VIDIOC_G_PARM, &parm) == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: ioctl() failed: %s\n",
			__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	printf("Frame rate: %u/%u\n",
		parm.parm.capture.timeperframe.denominator,
		parm.parm.capture.timeperframe.numerator);

	printf("USB CAM's output mode: %d\n", parm.parm.output.outputmode);

	printf("↑↑↑↑↑↑↑ 摄像头参数 ↑↑↑↑↑↑↑\n\n");
}

void cam_config(int xres, int yres, uint32_t pixfmt)
{
	// 填充想要设置的摄像头基本工作参数
	g_caminfo->width = xres;
	g_caminfo->height= yres;
	g_caminfo->pixfmt= pixfmt;

	// 配置摄像头的采集格式
	struct v4l2_format *fmt = calloc(1, sizeof(*fmt));
	bzero(fmt, sizeof(*fmt));
	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt->fmt.pix.width = g_caminfo->width;
	fmt->fmt.pix.height = g_caminfo->height;
	fmt->fmt.pix.pixelformat = g_caminfo->pixfmt;
	fmt->fmt.pix.field = V4L2_FIELD_INTERLACED;
	ioctl(g_caminfo->camfd, VIDIOC_S_FMT, fmt);

	// 再次获取摄像头当前的采集格式
	bzero(fmt, sizeof(*fmt));
	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl (g_caminfo->camfd, VIDIOC_G_FMT, fmt);

	if(fmt->fmt.pix.pixelformat != V4L2_PIX_FMT_JPEG)
	{
		printf("\nthe pixel format is NOT JPEG.\n\n");
		exit(1);
	}

	// 设置摄像头的帧率FPS
	struct v4l2_streamparm parm;
	bzero(&parm, sizeof(parm));

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator  = 1;
	parm.parm.capture.timeperframe.denominator= FPS;
	if(ioctl(g_caminfo->camfd, VIDIOC_S_PARM, &parm) == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: ioctl() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
	}

	// 再次获取帧率，看看是否设置成功
	ioctl(g_caminfo->camfd, VIDIOC_G_PARM, &parm);
	if(parm.parm.capture.timeperframe.numerator  != 1 ||
	   parm.parm.capture.timeperframe.denominator!= FPS)
	{
		printf("Frame rate: %u/%u fps (requested frame rate %u fps is "
				"not supported by device)\n",
				parm.parm.capture.timeperframe.denominator,
				parm.parm.capture.timeperframe.numerator,
				FPS);
	}

	// 设置摄像头的亮度
	struct v4l2_queryctrl queryctrl;
	bzero(&queryctrl, sizeof(queryctrl));
	queryctrl.id = V4L2_CID_BRIGHTNESS;
	queryctrl.type = V4L2_CTRL_TYPE_INTEGER;
	if(ioctl(g_caminfo->camfd, VIDIOC_QUERYCTRL, &queryctrl) == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: ioctl() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
	}

	struct v4l2_control control;
	bzero(&control, sizeof(control));
	control.id = V4L2_CID_BRIGHTNESS;
	control.value = 200;
	if(ioctl(g_caminfo->camfd, VIDIOC_S_CTRL, &control) == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: ioctl() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
	}

	// 设置摄像头缓存的参数
	struct v4l2_requestbuffers reqbuf;
	bzero(&reqbuf, sizeof (reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = CAM_BUF_NUM;
	ioctl(g_caminfo->camfd, VIDIOC_REQBUFS, &reqbuf);

	// 根据刚刚设置的reqbuf.count的值，来定义相应数量的struct v4l2_buffer
	// 每一个struct v4l2_buffer对应内核摄像头驱动中的一个缓存
	g_caminfo->v4l2bufs = calloc(CAM_BUF_NUM, sizeof(struct v4l2_buffer));

	int i;
	for(i=0; i<CAM_BUF_NUM; i++)
	{
		g_caminfo->v4l2bufs[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		g_caminfo->v4l2bufs[i].memory = V4L2_MEMORY_MMAP;
		g_caminfo->v4l2bufs[i].index = i;
		ioctl(g_caminfo->camfd, VIDIOC_QUERYBUF, &g_caminfo->v4l2bufs[i]);

		g_caminfo->len[i] = g_caminfo->v4l2bufs[i].length;
		g_caminfo->frames[i] = mmap(NULL, g_caminfo->v4l2bufs[i].length,
								  PROT_READ|PROT_WRITE, MAP_SHARED,
								  g_caminfo->camfd, g_caminfo->v4l2bufs[i].m.offset);

		ioctl(g_caminfo->camfd , VIDIOC_QBUF, &g_caminfo->v4l2bufs[i]);
	}

	// 启动摄像头数据采集
	enum v4l2_buf_type vtype= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(g_caminfo->camfd, VIDIOC_STREAMON, &vtype);
}


void cam_open(char *device_name)
{
	// 分配摄像头管理结构体内存空间
	g_caminfo = calloc(1, sizeof(*g_caminfo));

	// 打开摄像头设备文件
	g_caminfo->camfd = open(device_name, O_RDWR);
	if(g_caminfo->camfd == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: open() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}
}
