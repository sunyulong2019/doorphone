//////////////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  File name: GPLE/doorphone.c
//
//  Author: Vincent Lin (林世霖)  微信公众号：秘籍酷
//
//  Date: 2017-04
//  
//  Description: 智能门禁对讲系统——客户端程序
//
//  GitHub: github.com/vincent040   Bug Report: 2437231462@qq.com
//
//////////////////////////////////////////////////////////////////

#include "audio.h"
#include "login.h"
#include "network.h"
#include "lcd.h"
#include "cam.h"
#include "ts.h"

#include "common.h"

unsigned char *jpgdata_connect_off;
unsigned char *jpgdata_connect_on;
unsigned char *jpgdata_door_off;
unsigned char *jpgdata_door_on;
unsigned char *jpgdata_bg;
int size_connect_off;
int size_connect_on;
int size_door_off;
int size_door_on;
int size_bg;


unsigned char *jpgdata[10];
unsigned char *jpgdata_on[10];
unsigned char *jpgdata_del, *jpgdata_del_on;
unsigned char *jpgdata_ent, *jpgdata_ent_on;


int size[10];
int size_on[10];
int size_del, size_del_on;
int size_ent, size_ent_on;

int pos_x[12];
int pos_y[12];


int g_logging_in = ON; // 输入房间号主界面
int g_recv_pcm = OFF;
int g_send_pcm = ON;
int g_recv_jpg = OFF;
int g_send_jpg = ON;

extern int g_pip; // 表示当前是否存在画中画（ON/OFF）


// 触摸屏控制面板
void *touch_panel(void *arg)
{
	// 准备控制面板界面素材

	// 背景等待图
	size_bg = read_image_from_file("btns/bg.jpg", &jpgdata_bg);

	// 左侧栏接通状态图
	size_connect_off = read_image_from_file("btns/connect_off.jpg", &jpgdata_connect_off);
	size_connect_on  = read_image_from_file("btns/connect_on.jpg",  &jpgdata_connect_on);

	// 右侧栏门开启状态图
	size_door_off = read_image_from_file("btns/door_off.jpg", &jpgdata_door_off);
	size_door_on  = read_image_from_file("btns/door_on.jpg",  &jpgdata_door_on);


	while(1)
	{
		// 显示输入房间号验证界面，输入了正确的房间号之后，往下继续执行
		g_key_pass_offset = 0;
		login();

		// 左边显示对讲是否已经接通（灰色意味着未接通，绿色意味着已接通）
		show_jpg(jpgdata_connect_off, size_connect_off, NOTPIP, 0, 0);

		// 右边显示门是否已经开启（灰色意味着未接通，绿色意味着已接通）
		show_jpg(jpgdata_door_off, size_door_off, NOTPIP, 720, 0);


		// 输入了正确的房间号
		// 1，给业主端发去来访请求字段
		char cmd = 'e';
		if(sendto(g_udp_sockfd[STATE], &cmd, 1, 0,
					(struct sockaddr *)g_peer_addr[STATE], sizeof(*(g_peer_addr[STATE]))) > 0)
		{
			printf("发出了来访请求 --> [%c]\n", cmd);
		}
		else
		{
			fprintf(stderr, "[%s][%s][%d] sendto() failed: %s\n",
							__FILE__, __FUNCTION__, __LINE__, strerror(errno));
			exit(0);
		}

		// 2，显示一张等待图案，静静等待业主的接通
		show_jpg(jpgdata_bg, size_bg, NOTPIP, 80, 0); // 显示一张等待图标
		g_logging_in = OFF;

		// 3，如果业主端发来关闭监控的reset请求，则重新进入输入房号界面
		//    否则在下面的小循环持续睡眠
		while(1)
		{
			usleep(200*1000);

			if(g_logging_in == ON)
				break;
		}
	}

	pthread_exit(NULL);
}


// 接收对方图像并显示出来
void *recv_jpg(void *arg)
{
	unsigned char *jpgdata = calloc(50, 1024);

	// 不断从socket中读取图像数据，并写入LCD显存显示出来
	while(1)
	{
		while(1)
		{
			if(g_recv_jpg == ON)
				break;

			usleep(200*1000);
		}


		int m = recvfrom(g_udp_sockfd[VIDEO], jpgdata, 50*1024, 0, NULL, NULL);
		if(m < 0 && errno == EAGAIN)
		{
			continue; // 对方未发来视频数据，继续尝试（默认延迟0.5秒）
		}
		if(m < 0)
		{
			fprintf(stderr, "[%s][%s][%d]: recvfrom() failed: %s\n",
							__FILE__, __FUNCTION__, __LINE__, strerror(errno));
			exit(0);
		}

		// 业主端发来的视频数据，一律用大屏（640*480）显示
		if(g_pip == ON)
			show_jpg(jpgdata, m, NOTPIP, 80, 0);
		else
		{
			usleep(10*1000);
			show_jpg(jpgdata_bg, size_bg, NOTPIP, 80, 0);
		}
	}
	pthread_exit(NULL);
}



// 摄像，并将图像持续不断地传送给业主端（不可关闭），但自己看不见自己
// 如果业主发来视频对讲命令，则开启画中画模式，同时显示双方的图像
void *send_jpg(void *arg)
{
	struct v4l2_buffer v4lbuf;
	bzero(&v4lbuf, sizeof(v4lbuf));
	v4lbuf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4lbuf.memory= V4L2_MEMORY_MMAP;

	int i = 0;
	int cursize;

	while(1)
	{
		while(1)
		{
			if(g_send_jpg == ON)
				break;

			usleep(200*1000);
		}

		// 从队列中取出填满数据的缓存
		v4lbuf.index = i%CAM_BUF_NUM;

		// VIDIOC_DQBUF在摄像头没数据的时候会阻塞
		ioctl(g_caminfo->camfd , VIDIOC_DQBUF, &v4lbuf);

		cursize = g_caminfo->v4l2bufs[i%CAM_BUF_NUM].bytesused * 2;
		int m = sendto(g_udp_sockfd[VIDEO], g_caminfo->frames[i%CAM_BUF_NUM], cursize, 0,
					(struct sockaddr *)g_peer_addr[VIDEO], sizeof(*(g_peer_addr[VIDEO])));
		if(m < 0)
		{
			fprintf(stderr, "[%s][%s][%d]: sendto() failed: %s\n",
							__FILE__, __FUNCTION__, __LINE__, strerror(errno));
			exit(0);
		}

		// 在g_pip==ON（即业主要求视频对讲）的条件下，将本地视频图像以画中画的形式显示出来
		if(g_pip == ON)
			show_jpg(g_caminfo->frames[i%CAM_BUF_NUM], cursize, PIP, g_xoffset, g_yoffset);

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
	// 打开PCM设备中的音箱/耳机（PLAYBACK）
	pthread_mutex_lock(&pcm_lock);
	g_sound_recv = calloc(1, sizeof(pcm_container));
	if(snd_pcm_open(&g_sound_recv->handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_ASYNC) != 0)
	{
		perror("snd_pcm_open() failed");
		exit(0);
	}	

	// 准备PCM设备工作参数，并保存在sound中
	prepare_pcm_param(g_sound_recv);
	set_pcm_param(g_sound_recv);
	pthread_mutex_unlock(&pcm_lock);

	// 不断从socket中读取数据（每次最多一个周期的数据量），并发送到PCM设备播放出来
	while(1)
	{
		while(1)
		{
			if(g_recv_pcm == ON)
				break;

			usleep(200*1000);
		}

		int n = recvfrom(g_udp_sockfd[AUDIO], g_sound_recv->period_buf,
						 g_sound_recv->frames_per_period * g_sound_recv->bytes_per_frame, 0, NULL, NULL);

		if(n < 0 && errno == EAGAIN)
		{
			continue; // 对方未发来语音数据，继续尝试（默认延迟0.5秒）
		}

		if(n < 0)
		{
			fprintf(stderr, "[%s][%s][%d] recvfrom() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		}

		write_pcm_to_device(g_sound_recv, n/g_sound_recv->bytes_per_frame);
	}
}



// 录音，并把声音传输给对端
void *send_pcm(void *arg)
{
	pthread_detach(pthread_self());

	// 打开PCM设备中的麦克风（CAPTURE）
	pthread_mutex_lock(&pcm_lock);
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
	pthread_mutex_unlock(&pcm_lock);

	// 不断从PCM设备中读取数据
	// 每次读取并发送一个周期的音频数据到对端
	while(1)
	{
		while(1)
		{
			if(g_send_pcm == ON)
				break;

			usleep(200*1000);
		}

		snd_pcm_uframes_t n = read_pcm_data(g_sound_send,
											g_sound_send->frames_per_period);

		// 业主端发来监控请求，或者客户端进入了来访请求界面，则给业主端发送语音数据
		if(g_recv_pcm == ON || g_logging_in == OFF)
		{
			int m = sendto(g_udp_sockfd[AUDIO],
						   g_sound_send->period_buf,
						   n*g_sound_send->bytes_per_frame, 0,
						   (struct sockaddr *)g_peer_addr[AUDIO],
						   sizeof(*(g_peer_addr[AUDIO])));
			if(m < 0)
			{
				fprintf(stderr, "[%s][%s][%d] sendto() failed: %s\n",
								__FILE__, __FUNCTION__, __LINE__, strerror(errno));
				exit(0);
			}
		}
	}
}

/*
*** 功能：接收业主端的命令字
*** 参数：无
*** 返回：无
*/ 
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
		case 'a': // 业主接通了呼叫请求，客人可以开始与业主对讲
			printf("[%c] <-- recved\n", state);
			if(g_logging_in == OFF)
				show_jpg(jpgdata_connect_on, size_connect_on, NOTPIP, 0, 0);
			g_recv_pcm = ON;
			break;

		case 'A': // 业主关闭了监控系统，停止对讲和视频
			printf("[%c] <-- recved\n", state);
			if(g_logging_in == OFF)
				show_jpg(jpgdata_connect_off, size_connect_off, NOTPIP, 0, 0);
			g_recv_pcm = OFF;
			g_recv_jpg = OFF;
			g_logging_in = ON; // 回到输入房号的界面
			break;


		case 'b': // 业主端发来开门指令
			printf("[%c] <-- recved\n", state);
			if(g_logging_in == OFF)
				show_jpg(jpgdata_door_on, size_door_on, NOTPIP, 720, 0);
			break;

		case 'B': // 业主端发来关门指令
			printf("[%c] <-- recved\n", state);
			if(g_logging_in == OFF)
				show_jpg(jpgdata_door_off, size_door_off, NOTPIP, 720, 0);
			break;

	
		case 'c': // 业主要求语音对讲（双向语音传输开始）
			printf("[%c] <-- recved\n", state);
			g_recv_pcm = ON;
			break;

		case 'C': // 业主端关闭了语音对讲（业主端不再发送语音过来，但客人端依然发送语音给业主）
			printf("[%c] <-- recved\n", state);
			g_recv_pcm = OFF;
			break;


		case 'd': // 业主端要求视频对讲（双向视频传输开始，以画中画显示本地视频）
			printf("[%c] <-- recved\n", state);
			g_pip = ON;
			g_recv_jpg = ON;
			break;

		case 'D': // 业主端关闭了视频对讲（业主端不再发送视频过来，但客人端依然发送视频给业主）
			printf("[%c] <-- recved\n", state);
			g_pip = OFF;
			g_recv_jpg = OFF;
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
	pthread_mutex_init(&pcm_lock, NULL);


	// 初始化网络语音通道、视频通道、状态通道
	g_udp_sockfd[AUDIO] = sock_init(AUDIO, argv[1], AUDIO_PORT);
	g_udp_sockfd[VIDEO] = sock_init(VIDEO, argv[1], VIDEO_PORT);
	g_udp_sockfd[STATE] = sock_init(STATE, argv[1], STATE_PORT);

	// 等待彼此启动
	wait_each_other();


	// 线程属性: 高优先级和分离
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	struct sched_param xpa;


	// 初始化LCD和触摸屏，并显示主控制面板
	lcd_init("/dev/fb0");
	ts_init("/dev/event0");
	pthread_t t3;
	int err = pthread_create(&t3, NULL, touch_panel, NULL); // 触摸屏控制逻辑
	if(err != 0)
	{
		exit(0);
	}

	// 创建语音处理线程
	pthread_t t1, t2;
	pthread_create(&t1, NULL, recv_pcm, NULL); // 语音接收并播放
	pthread_create(&t2, NULL, send_pcm, NULL); // 语音录制并发送


	// 初始化摄像头设备
	cam_open("/dev/video3");
	cam_config(640, 480, V4L2_PIX_FMT_JPEG); // 设置摄像头工作参数、设置缓存、启动摄像头
	cam_info_display();


	// // 创建视频处理线程
	pthread_t t4, t5;
	xpa.sched_priority = 1;
	pthread_attr_setschedparam(&attr, &xpa);
	pthread_create(&t4, &attr, recv_jpg, NULL); // 图像接收并显示
	pthread_create(&t5, &attr, send_jpg, NULL); // 图像摄制并发送

	// 状态处理线程
	pthread_t t6;
	xpa.sched_priority = 0;
	pthread_attr_setschedparam(&attr, &xpa);
	err = pthread_create(&t6, &attr, recv_state, NULL); // 接收业主端的控制指令
	if(err != 0)
	{
		printf("pthread_create() failed: %s\n", strerror(err));
		exit(0);
	}

	pause();
	return 0;
}