///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/client/inc/audio.h
//  日期: 2017-9
//  描述: 音频处理模块头文件
//
//  作者: Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////

#ifndef __AUDIO_H
#define __AUDIO_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "alsa/asoundlib.h"


#if __BYTE_ORDER == __LITTLE_ENDIAN

	#define LE_SHORT(val) (val) 
	#define LE_INT(val)   (val) 

#elif __BYTE_ORDER == __BIG_ENDIAN

	#define LE_SHORT(val) bswap_16(val) 
	#define LE_INT(val)   bswap_32(val) 

#endif 


// PCM设备管理结构体
typedef struct
{  
	snd_pcm_t *handle; // PCM设备操作句柄
	snd_pcm_format_t format; // 量化位数

	uint16_t channels;	   // 声道数目
	uint32_t sample_rate;	 // 采样频率
	size_t bits_per_sample;  // 一个采样点内的位数（8位、16位）
	size_t bytes_per_frame;  // 一个帧内的字节个数

	snd_pcm_uframes_t frames_per_period; // 一个周期内的帧数
	snd_pcm_uframes_t frames_per_buffer; // 系统buffer的帧数

	uint8_t *period_buf; // 一个周期的PCM数据缓冲区

}pcm_container; 


/* ++++++++ 变量声明 ++++++++ */
extern pcm_container *g_sound_recv;
extern pcm_container *g_sound_send;

extern int err;
extern pthread_mutex_t pcm_lock;

/* ++++++++ 函数声明 ++++++++ */
snd_pcm_uframes_t read_pcm_data(pcm_container *sound, snd_pcm_uframes_t frames);
ssize_t write_pcm_to_device(pcm_container *sound, size_t frames);
void set_pcm_param(pcm_container *pcm); 
void prepare_pcm_param(pcm_container *pcm);


#endif
