///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/client/inc/common.h
//  日期: 2017-9
//  描述: 本项目通用头文件
//
//  作者: Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////

#ifndef __COMMON_H
#define __COMMON_H

// 在主程序中定义的全局变量


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/time.h>



#define OFF -1
#define ON  1

#define FLIP_OVER(x) ((x)*=-1)


extern 	int g_logging_in; // 输入房间号主界面
extern 	bool door_is_open;

extern 	unsigned char *jpgdata_connect_off;
extern 	unsigned char *jpgdata_connect_on;
extern 	unsigned char *jpgdata_door_off;
extern 	unsigned char *jpgdata_door_on;
extern 	int size_connect_off;
extern 	int size_connect_on;
extern 	int size_door_off;
extern 	int size_door_on;


extern 	unsigned char *jpgdata[10];
extern 	unsigned char *jpgdata_on[10];
extern 	unsigned char *jpgdata_del, *jpgdata_del_on;
extern 	unsigned char *jpgdata_ent, *jpgdata_ent_on;


extern 	int size[10];
extern 	int size_on[10];
extern 	int size_del, size_del_on;
extern 	int size_ent, size_ent_on;

extern 	int pos_x[12];
extern 	int pos_y[12];

extern 	int g_recv_pcm;


#endif
