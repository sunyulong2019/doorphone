///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/doorphone/ts.c
//  日期: 2017-9
//  描述: 提供触摸屏子模块功能
//
//  作者: Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////



#include "lcd.h"
#include "ts.h"

struct tsdev *g_ts = NULL;


void ts_init(char *device_name)
{
	// 以阻塞的方式打开触摸屏，得到代表触摸屏的指针ts
	g_ts = ts_open(device_name, 0);
	if(g_ts == NULL)
	{
		fprintf(stderr, "[%s][%s][%d] ts_open() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	// 配置触摸屏算法模块
	ts_config(g_ts);
}


void get_xy(int *px, int *py)
{
	struct ts_sample samp;
	bzero(&samp, sizeof(samp));

	// 获取当前手指触碰的坐标
	ts_read(g_ts, &samp, SENSITIVITY);
	*px = samp.x;
	*py = samp.y;
}


/*
*** 功能：判断坐标(x,y)是否在画中画的范围之内
*/
bool within_pip(int x, int y)
{
	return(x >= g_xoffset && x <= g_xoffset+320 &&
		   y >= g_yoffset && y <= g_yoffset+240);
}


/*
*** 功能：根据手指滑动的轨迹，移动画中画的位置
*** 参数：x_base 本次画面移动的x轴开始基准点
***       y_base 本次画面移动的y轴开始基准点
*** 返回：无
*/
void move_pip(int x_base, int y_base)
{
	struct ts_sample samp;

	int x_movement = 0;
	int y_movement = 0;
	while(1)
	{
		bzero(&samp, sizeof(samp));

		ts_read(g_ts, &samp, SENSITIVITY);

		// 手指松开时停止移动画中画
		if(samp.pressure == 0)
			break;

		x_movement = samp.x - x_base;
		y_movement = samp.y - y_base;

		g_xoffset += x_movement;
		g_yoffset += y_movement;

		// 将画中画固定在主监控画面之内
		g_xoffset = g_xoffset>400 ? 400 : g_xoffset;	
		g_xoffset = g_xoffset<80  ? 80  : g_xoffset;	
		g_yoffset = g_yoffset>240 ? 240 : g_yoffset;	
		g_yoffset = g_yoffset<0   ? 0   : g_yoffset;	

		x_base = samp.x;
		y_base = samp.y;
	}
}


int __get_cur_pos(int x, int y)
{
	int btn;

	if(x > 0 && x < 80 && y > 0 && y < 240)	
		btn = BTN1;

	if(x > 720 && x < 800 && y > 0 && y < 240)	
		btn = BTN2;

	if(x > 0 && x < 80 && y > 240 && y < 480)	
		btn = BTN3;

	if(x > 720 && x < 800 && y > 240 && y < 480)	
		btn = BTN4;

	return btn;
}

/*
*** 功能：判断按下了哪个按键
*** 参数：手指刚按下时的坐标
*** 返回：按键序号（左上角BTN1、右上角BTN2、左下角BTN3、右下角BTN4）
*/
int pos(int x_ori, int y_ori)
{
	// 记录刚按下的按键序号
	int btn_ori = __get_cur_pos(x_ori, y_ori);

	struct ts_sample samp;
	while(1)
	{
		bzero(&samp, sizeof(samp));
		ts_read(g_ts, &samp, SENSITIVITY);

		// 手指松开时判定为一次按压
		if(samp.pressure == 0)
		{
			break;
		}
	}

	// 记录手指松开的按键序号
	int btn_release = __get_cur_pos(samp.x, samp.y);

	// 将手指离开后残留的(0,0)坐标去除
	ts_read(g_ts, &samp, SENSITIVITY);
	return (btn_release == btn_ori) ? btn_ori : -1;
}	
