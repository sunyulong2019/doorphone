///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/doorphone/inc/cam.h
//  日期: 2017-9
//  描述: 摄像头处理头文件
//
//  作者: Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////

#ifndef __CAM_H
#define __CAM_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/time.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define FPS 30 // frames per second（帧率）

typedef struct
{
	int camfd;

	int width;
	int height;
	uint32_t pixfmt; // V4L2_PIX_FMT_JPEG or V4L2_PIX_FMT_YUYV  /* see /usr/include/linux/videodev2.h: 580*/

	#define CAM_BUF_NUM 3
	unsigned char *frames[CAM_BUF_NUM];
	struct v4l2_buffer *v4l2bufs;

	int len[CAM_BUF_NUM];
}camera_info;


/* ++++++++ 变量声明 ++++++++ */
extern camera_info *g_caminfo;


/* ++++++++ 函数声明 ++++++++ */
void cam_open(char *device_name);
void cam_config(int xres, int yres, uint32_t pixfmt);
void cam_info_display(void);


#endif
