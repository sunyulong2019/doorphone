////////////////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  File name: GPLE/doorphone.c
//
//  Author: Vincent Lin (林世霖)  微信公众号：秘籍酷
//
//  Date: 2017-04
//  
//  Description: 智能门禁对讲系统——主控程序（业主端）
//
//  GitHub: github.com/vincent040   Bug Report: 2437231462@qq.com
//
//////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/time.h>

#include "audio.h"
#include "network.h"
#include "lcd.h"
#include "cam.h"
#include "ts.h"

#define OFF -1 
#define ON   1 

#define FLIP_OVER(x) ((x)*=-1)

/*                     四个按钮功能图
***
*** 左上角btn1（监控开关）        右上角btn2（开门）
***
***
*** 左下角btn3（本地语音开关）    右下角btn4（本地视频开关）
*/

sem_t send_pcm_jpg_state;
sem_t door_ring;
int g_err = 0;

unsigned char *btn1_off_jpgdata = NULL;
unsigned char *btn2_off_jpgdata = NULL;
unsigned char *btn3_off_jpgdata = NULL;
unsigned char *btn4_off_jpgdata = NULL;
unsigned char *btn1_on_jpgdata  = NULL;
unsigned char *btn2_on_jpgdata  = NULL;
unsigned char *btn3_on_jpgdata  = NULL;
unsigned char *btn4_on_jpgdata  = NULL;
unsigned char *bg_jpgdata       = NULL;

int btn1_off_size = 0;
int btn2_off_size = 0;
int btn3_off_size = 0;
int btn4_off_size = 0;
int btn1_on_size = 0;
int btn2_on_size = 0;
int btn3_on_size = 0;
int btn4_on_size = 0;
int bg_size = 0;

int g_monitor  = OFF; // btn1: 初始状态是OFF，点击btn1后变成ON，此时将开始接受客户端的语音和视频，并给客户端发去确认接通的字段；再次点击则将其翻转为OFF，停止接收客户端的所有数据并给客户端发去断开接通的字段
int g_door_ctl = OFF; // btn2
int g_send_pcm = OFF; // btn3
int g_send_jpg = OFF; // btn4

int g_someone_is_calling = OFF;
int g_pcm_device_ready = OFF;

void *touch_panel(void *arg)
{
	int x, y;
	while(1)
	{
		get_xy(&x, &y); // 获取一对儿坐标马上返回

		// 如果按压在画中画里面，则开始移动画中画的位置
		if(within_pip(x, y) && g_pip==1)
		{
			move_pip(x, y);
			continue;
		}

		// 判断按压了哪个按键（以手指离开时为准）
		switch(pos(x, y))
		{
		case BTN1: // （监控开关，可主动打开观察大堂情况，也可被客户端的来访请求激活）
			FLIP_OVER(g_monitor);
			show_jpg(g_monitor==ON ? btn1_on_jpgdata : btn1_off_jpgdata,
					 g_monitor==ON ? btn1_on_size : btn1_off_size, NOTPIP, 0, 0);
			if(g_monitor == OFF)
				show_jpg(bg_jpgdata, bg_size, NOTPIP, 80, 0);
			sem_post(&send_pcm_jpg_state);
			break;

		case BTN2: // （开门）
			FLIP_OVER(g_door_ctl);
			show_jpg(g_door_ctl==ON ? btn2_on_jpgdata : btn2_off_jpgdata,
					 g_door_ctl==ON ? btn2_on_size : btn2_off_size, NOTPIP, 720, 0);
			sem_post(&send_pcm_jpg_state);
			break;

		case BTN3: // （本地语音开关）
			FLIP_OVER(g_send_pcm);
			printf("%s\n", g_send_pcm==ON ? "g_send_pcm --> ON" : "g_send_pcm --> OFF");
			show_jpg(g_send_pcm==ON ? btn3_on_jpgdata : btn3_off_jpgdata,
					 g_send_pcm==ON ? btn3_on_size : btn3_off_size, NOTPIP, 0, 240);
			sem_post(&send_pcm_jpg_state);
			break;

		case BTN4: //（本地视频开关） 
			FLIP_OVER(g_pip);
			FLIP_OVER(g_send_jpg);
			printf("%s\n", g_send_jpg==ON ? "g_send_jpg --> ON" : "g_send_jpg --> OFF");
			show_jpg(g_pip==ON ? btn4_on_jpgdata : btn4_off_jpgdata,
					 g_pip==ON ? btn4_on_size : btn4_off_size, NOTPIP, 720, 240);
			sem_post(&send_pcm_jpg_state);
			break;
		}
	}

	pthread_exit(NULL);
}


void *recv_jpg(void *arg)
{
	pthread_detach(pthread_self());

	unsigned char *jpgdata = calloc(50, 1024);

	// 不断从socket中读取图像数据，并写入LCD显存显示出来
	while(1)
	{
		int m = recvfrom(g_udp_sockfd[VIDEO], jpgdata, 50*1024, 0, NULL, NULL);

		if(m < 0 && errno == EAGAIN)
		{
			continue; // 对方没有发来视频数据，继续尝试（默认延迟0.5秒）
		}
		if(m < 0)
		{
			fprintf(stderr, "[%s][%s][%d]: recvfrom() failed: %s\n",
							__FILE__, __FUNCTION__, __LINE__, strerror(errno));
			exit(0);
		}

		if(g_monitor == ON)
			show_jpg(jpgdata, m, NOTPIP, 80, 0);
	}
	pthread_exit(NULL);
}


// 摄像，并将图像传送给对端
void *send_jpg(void *arg)
{
	pthread_detach(pthread_self());

	struct v4l2_buffer v4lbuf;
	bzero(&v4lbuf, sizeof(v4lbuf));
	v4lbuf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4lbuf.memory= V4L2_MEMORY_MMAP;

	int i = 0;
	int cursize;
	while(1)
	{
		// 从队列中取出填满数据的缓存
		v4lbuf.index = i%CAM_BUF_NUM;

		// VIDIOC_DQBUF在摄像头没数据的时候会阻塞
		ioctl(g_caminfo->camfd , VIDIOC_DQBUF, &v4lbuf);

		if(g_send_jpg == OFF)
		{
			usleep(150*1000); // 延迟10毫秒
			continue;
		}

		cursize = g_caminfo->v4l2bufs[i%CAM_BUF_NUM].bytesused * 2;
		int m = sendto(g_udp_sockfd[VIDEO], g_caminfo->frames[i%CAM_BUF_NUM], cursize, 0,
					(struct sockaddr *)g_peer_addr[VIDEO], sizeof(*(g_peer_addr[VIDEO])));
		if(m < 0)
		{
			fprintf(stderr, "[%s][%s][%d]: sendto() failed: %s\n",
							__FILE__, __FUNCTION__, __LINE__, strerror(errno));
			exit(0);
		}

		if(g_pip == ON)
		{
			// 将本地的图像以画中画的形式显示出来
			show_jpg(g_caminfo->frames[i%CAM_BUF_NUM], cursize, PIP, g_xoffset, g_yoffset);
		}

	 	// 将已经读取过数据的缓存块重新置入队列中 
		v4lbuf.index = i%CAM_BUF_NUM;
		ioctl(g_caminfo->camfd , VIDIOC_QBUF, &v4lbuf);

		i++;
	}

	pthread_exit(NULL);
}


// 接收对端的声音，并把声音播放出来
void *recv_pcm(void *arg)
{
	pthread_detach(pthread_self());

	// 不断从socket中读取数据（每次最多一个周期的数据量），并发送到PCM设备播放出来
	while(1)
	{
		while(g_pcm_device_ready == OFF)
		{
			printf("g_pcm_device_ready is NOT ready!\n");
			usleep(200*1000);
		}

		int m = recvfrom(g_udp_sockfd[AUDIO], g_sound_recv->period_buf,
						 g_sound_recv->frames_per_period * g_sound_recv->bytes_per_frame, 0, NULL, NULL);

		if(m < 0 && errno == EAGAIN)
		{
			continue; // 对端没有启动，继续尝试（默认延迟0.5秒）
		}

		if(m < 0)
		{
			fprintf(stderr, "[%s][%s][%d] sendto() failed: %s\n",
							__FILE__, __FUNCTION__, __LINE__, strerror(errno));
			exit(0);
		}

		if(g_monitor == ON)
		{
			write_pcm_to_device(g_sound_recv, m/g_sound_recv->bytes_per_frame);
		}
	}
}


// 录音，并把声音传输给对端
void *send_pcm(void *arg)
{
	pthread_detach(pthread_self());

	// 不断从PCM设备中读取数据
	// 每次读取并发送一个周期的音频数据到对端
	while(1)
	{
		while(g_pcm_device_ready == OFF)
		{
			usleep(200*1000);
		}

		snd_pcm_uframes_t n = read_pcm_data(g_sound_send, g_sound_send->frames_per_period);

		if(g_send_pcm == OFF)
		{
			continue;
		}

		int m = sendto(g_udp_sockfd[AUDIO], g_sound_send->period_buf,
					   n*g_sound_send->bytes_per_frame, 0,
					   (struct sockaddr *)g_peer_addr[AUDIO], sizeof(*(g_peer_addr[AUDIO])));
		if(m < 0)
		{
			perror("sendto() failed");
			exit(0);
		}
	}
}

void *prepare_pcm(void *arg)
{
	pthread_detach(pthread_self());

	while(1)
	{
		// 打开PCM设备中的音箱/耳机（PLAYBACK）
		g_sound_recv = calloc(1, sizeof(pcm_container));
		if(snd_pcm_open(&g_sound_recv->handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_ASYNC) != 0)
		{
			perror("snd_pcm_open() failed");
			exit(0);
		}	

		// 准备PCM设备工作参数，并保存在sound中
		prepare_pcm_param(g_sound_recv);
		set_pcm_param(g_sound_recv);



		// 打开PCM设备中的麦克风（CAPTURE）
		g_sound_send = calloc(1, sizeof(pcm_container));
		if((err=snd_pcm_open(&g_sound_send->handle, "default",
							 SND_PCM_STREAM_CAPTURE, SND_PCM_ASYNC)) != 0)
		{
			fprintf(stderr, "snd_pcm_open() failed: %s\n", snd_strerror(err));
			exit(0);
		}	

		// 准备PCM设备工作参数，并保存在sound中
		prepare_pcm_param(g_sound_send);
		set_pcm_param(g_sound_send);

		g_pcm_device_ready = ON;


		// 等待来客的来访铃声
		printf("[%s][%d]\n", __FUNCTION__, __LINE__);
		sem_wait(&door_ring);

		// 关闭当前赈灾使用的音频设备
		printf("[%s][%d]\n", __FUNCTION__, __LINE__);

	    snd_pcm_drain(g_sound_recv->handle);  
    	snd_pcm_close(g_sound_recv->handle);  

	    snd_pcm_drain(g_sound_send->handle);  
    	snd_pcm_close(g_sound_send->handle);  
		printf("[%s][%d]\n", __FUNCTION__, __LINE__);

    	// 来访铃声结束，重新打开音频设备
    	sem_wait(&door_ring);
		printf("[%s][%d]\n", __FUNCTION__, __LINE__);
	}

	pthread_exit(NULL);
}


void *send_state(void *arg)
{
	pthread_detach(pthread_self());

	int g_monitor_old  = g_monitor;
	int g_door_ctl_old = g_door_ctl;
	int g_send_pcm_old = g_send_pcm;
	int g_send_jpg_old = g_send_jpg;

	char cmd;
	int m;
	while(1)
	{
		// 等待按键指令
		sem_wait(&send_pcm_jpg_state);

		// 1，是否打开监控（或接通客户的来访请求）
		if(g_monitor_old != g_monitor)
		{
			cmd = ((g_monitor == ON) ? 'a' : 'A');

			m = sendto(g_udp_sockfd[STATE], &cmd, 1, 0,
						(struct sockaddr *)g_peer_addr[STATE], sizeof(*(g_peer_addr[STATE])));
			if(m > 0)
			{
				fprintf(stderr, "[%d] %c is sent..\n", __LINE__, cmd);
			}
		}

		// 2，是否给客人开门
		if(g_door_ctl_old != g_door_ctl)
		{
			cmd = ((g_door_ctl == ON) ? 'b' : 'B');

			m = sendto(g_udp_sockfd[STATE], &cmd, 1, 0,
						(struct sockaddr *)g_peer_addr[STATE], sizeof(*(g_peer_addr[STATE])));
			if(m > 0)
			{
				fprintf(stderr, "[%d] %c is sent..\n", __LINE__, cmd);
			}
		}

		// 3，是否跟客人进行语音对讲
		if(g_send_pcm_old != g_send_pcm)
		{
			cmd = ((g_send_pcm == ON) ? 'c' : 'C');

			m = sendto(g_udp_sockfd[STATE], &cmd, 1, 0,
						(struct sockaddr *)g_peer_addr[STATE], sizeof(*(g_peer_addr[STATE])));
			if(m > 0)
			{
				fprintf(stderr, "[%d] %c is sent..\n", __LINE__, cmd);
			}
		}

		// 4，是否开启画中画视频对讲
		if(g_send_jpg_old != g_send_jpg)
		{
			cmd = ((g_send_jpg == ON) ? 'd' : 'D');

			m = sendto(g_udp_sockfd[STATE], &cmd, 1, 0,
						(struct sockaddr *)g_peer_addr[STATE], sizeof(*(g_peer_addr[STATE])));
			if(m > 0)
			{
				fprintf(stderr, "[%d] %c is sent..\n", __LINE__, cmd);
			}
		}

		g_monitor_old  = g_monitor;
		g_door_ctl_old = g_door_ctl;
		g_send_pcm_old = g_send_pcm;
		g_send_jpg_old = g_send_jpg;

	}
}


void *recv_state(void *arg)
{
	pthread_detach(pthread_self());

	char state;
	int m;
	while(1)
	{
		m = recvfrom(g_udp_sockfd[STATE], &state, 1, 0, NULL, NULL);
		if(m < 0 && errno == EAGAIN)
		{
			usleep(150*1000); // 对端没有启动，延迟150毫秒
			continue;	
		}
		if(m < 0)
		{
			fprintf(stderr, "[%s][%s][%d] recvfrom() failed: %s\n",
							__FILE__, __FUNCTION__, __LINE__, strerror(errno));
			exit(0);
		}

		switch(state)
		{
		case 'e': // 客人发来来访请求，门铃响起.....
			printf("[%s][%s][%d] 收到了来访请求[%c]\n", __FILE__, __FUNCTION__, __LINE__, state);

			g_someone_is_calling = ON;
			sem_post(&door_ring);

			usleep(100*1000);
			fprintf(stderr, "门铃响起... ...");
			system("./madplay door_ring.mp3"); // 播放门铃音乐
			fprintf(stderr, "门铃结束\n");

			g_someone_is_calling = OFF;
			sem_post(&door_ring);

			break;
		}
	}
}


int main(int argc, char *argv[]) // ./talk <IP>
{
	// 检查参数合法性
	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s <peer's IP>\n", argv[0]);	
		exit(0);
	}

	// 初始化互斥锁
	pthread_mutex_init(&pcm_lock, NULL); // 互斥访问音频设备

	// 初始化相关资源信号量
	sem_init(&send_pcm_jpg_state, 0, 0);
	sem_init(&door_ring, 0, 0);


	// 初始化网络语音通道、视频通道
	g_udp_sockfd[AUDIO] = sock_init(AUDIO, argv[1], AUDIO_PORT);
	g_udp_sockfd[VIDEO] = sock_init(VIDEO, argv[1], VIDEO_PORT);
	g_udp_sockfd[STATE] = sock_init(STATE, argv[1], STATE_PORT);

	// 等待彼此启动
	wait_each_other();

	// 初始化LCD，并显示主控制面板
	lcd_init("/dev/fb0");
	btn1_off_size = read_image_from_file("btns/btn1_off.jpg", &btn1_off_jpgdata);
	btn2_off_size = read_image_from_file("btns/btn2_off.jpg", &btn2_off_jpgdata);
	btn3_off_size = read_image_from_file("btns/btn3_off.jpg", &btn3_off_jpgdata);
	btn4_off_size = read_image_from_file("btns/btn4_off.jpg", &btn4_off_jpgdata);
	btn1_on_size  = read_image_from_file("btns/btn1_on.jpg", &btn1_on_jpgdata);
	btn2_on_size  = read_image_from_file("btns/btn2_on.jpg", &btn2_on_jpgdata);
	btn3_on_size  = read_image_from_file("btns/btn3_on.jpg", &btn3_on_jpgdata);
	btn4_on_size  = read_image_from_file("btns/btn4_on.jpg", &btn4_on_jpgdata);
	bg_size       = read_image_from_file("btns/bg.jpg", &bg_jpgdata);

	show_jpg(btn1_off_jpgdata, btn1_off_size, NOTPIP, 0, 0);
	show_jpg(btn2_off_jpgdata, btn2_off_size, NOTPIP, 720, 0);
	show_jpg(btn3_off_jpgdata, btn3_off_size, NOTPIP, 0, 240);
	show_jpg(btn4_off_jpgdata, btn4_off_size, NOTPIP, 720, 240);
	show_jpg(bg_jpgdata, bg_size, NOTPIP, 80, 0);


	// 线程属性: 高优先级和分离
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	struct sched_param xpa;



	// 创建语音处理线程
	pthread_t t1, t2;
	pthread_create(&t1, NULL, prepare_pcm, NULL); // 开启音频设备
	pthread_create(&t1, NULL, recv_pcm, NULL); // 语音接收并播放
	pthread_create(&t2, NULL, send_pcm, NULL); // 语音录制并发送



	// 初始化摄像头设备
	cam_open("/dev/video3");
	cam_config(640, 480, V4L2_PIX_FMT_JPEG); // 设置摄像头工作参数、设置缓存、启动摄像头
	cam_info_display();

	// 创建视频处理线程
	pthread_t t3, t4;
	pthread_create(&t3, NULL, recv_jpg, NULL); // 图像接收并显示
	pthread_create(&t4, NULL, send_jpg, NULL); // 图像摄制并发送


	// 初始化触摸屏设备
	ts_init("/dev/event0");
	
	// 创建触摸屏处理线程（高优先级）
	pthread_t t5;
	xpa.sched_priority = 1;
	pthread_attr_setschedparam(&attr, &xpa);
	pthread_create(&t5, &attr, touch_panel, NULL); // 触摸屏控制逻辑

	// 控制信号线程
	pthread_t t6, t7;
	pthread_create(&t6, NULL, send_state, NULL);
	pthread_create(&t7, NULL, recv_state, NULL);

	pause();
	return 0;
}
