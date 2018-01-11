///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/doorphone/network.c
//  日期: 2017-9
//  描述: 提供网络子模块功能
//
//  作者: Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////


#include "network.h"

struct sockaddr_in *g_peer_addr[COMM_CHANNELS];
int g_udp_sockfd[COMM_CHANNELS];

/*
*** 功能：初始化UDP通信端点
*** 参数：num 序号
***       peer_ip 对端的IP地址（字符串形式）
***       port  对端端口号
*** 返回：通信套接字
*/
int sock_init(int num, char *peer_ip, unsigned short port)
{
	// 创建数据传输通道socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1)
	{
		fprintf(stderr, "[%s][%s][%d] socket() failed: %s\n",
				__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}


	// 设置超时状态监测（0.5秒）
	struct timeval tv = {0, 500*1000};
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
	{
		fprintf(stderr, "[%s][%s][%d] setsockopt() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}


	// 使能地址复用功能
	int on = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
	{
		fprintf(stderr, "[%s][%s][%d] socket() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	// 将套接字绑定到固定的IP+PORT上，将来方便对端给我端发来数据
	struct sockaddr_in *addr = calloc(1, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(port);
	if(bind(sockfd, (struct sockaddr *)addr, sizeof(*addr)))
	{
		fprintf(stderr, "[%s][%s][%d] bind() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	// 定义对端地址结构体并赋值
	g_peer_addr[num] = calloc(1, sizeof(struct sockaddr_in));
	g_peer_addr[num]->sin_family = AF_INET;
	g_peer_addr[num]->sin_addr.s_addr = inet_addr(peer_ip);
	g_peer_addr[num]->sin_port = htons(port);

	return sockfd;
}



void wait_each_other(void)
{
	// 给对方发去一声问候！
	char *sayHey = "hello!";
	sendto(g_udp_sockfd[STATE], sayHey, strlen(sayHey), 0,
				(struct sockaddr *)g_peer_addr[STATE], sizeof(*(g_peer_addr[STATE])));

	// 静静等候对方的回复（等10毫秒）
	char msg[20] = {0};
	struct timeval tv = {0, 10*1000};

	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(g_udp_sockfd[STATE], &rset);

	if(select(g_udp_sockfd[STATE]+1, &rset, NULL, NULL, &tv) == 0)
	{
		printf("对方还没启动，静静等待对方的问候！\n");

		while(1)
		{
			int m = recvfrom(g_udp_sockfd[STATE], msg, 20, 0, NULL, NULL);
			if(m < 0 && errno == EAGAIN)
			{
				usleep(150*1000); // 对端没有启动，延迟150毫秒
				continue;	
			}
			break;
		}

		// 对方终于来了，并带来了一句问候，立马给对方一句回复！
		sendto(g_udp_sockfd[STATE], sayHey, strlen(sayHey), 0,
					(struct sockaddr *)g_peer_addr[STATE], sizeof(*(g_peer_addr[STATE])));
	}
	else
	{
		while(1)
		{
			int m = recvfrom(g_udp_sockfd[STATE], msg, 20, 0, NULL, NULL);
			if(m < 0 && errno == EAGAIN)
			{
				usleep(150*1000); // 对端没有启动，延迟150毫秒
				continue;	
			}
			break;
		}

	}
}
