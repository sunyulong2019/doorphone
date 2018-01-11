///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/client/login.c
//  日期: 2017-9
//  描述: 提供验证子模块功能
//
//  作者: 黄康 
//  Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////

#include "common.h"
#include "login.h"
#include "lcd.h"
#include "ts.h"

extern int g_logging_in;

int  g_key_pass_offset;
static char g_key_pass_buf[6]={0};
static char g_key_login[6] = "888";
static char g_pass_login_success = 0;
static char *g_number_path[10] =
{
       	"./btns/0.jpg",
       	"./btns/1.jpg",
       	"./btns/2.jpg",
       	"./btns/3.jpg",
       	"./btns/4.jpg",
       	"./btns/5.jpg",
       	"./btns/6.jpg",
       	"./btns/7.jpg",
       	"./btns/8.jpg",
       	"./btns/9.jpg"
};


void show_login(char *menu_path)
{
	int x=100,y=160;	

	unsigned char *jpgdata;
	int size;
	
	size = read_image_from_file("./btns/background.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, 0, 0);
	size = read_image_from_file(menu_path, &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, 450, 50);
	
	x=100;y=160;
	size = read_image_from_file("./btns/1.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x += 96;
	size = read_image_from_file("./btns/2.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x+=96;
	size = read_image_from_file("./btns/3.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x = 100;
	y += 64;
	size = read_image_from_file("./btns/4.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x+=96;
	size = read_image_from_file("./btns/5.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x+=96;
	size = read_image_from_file("./btns/6.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x = 100;
	y += 64;
	size = read_image_from_file("./btns/7.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x+=96;
	size = read_image_from_file("./btns/8.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x+=96;
	size = read_image_from_file("./btns/9.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);


	x = 100;
	y += 64;
	size = read_image_from_file("./btns/del.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x+=96;
	size = read_image_from_file("./btns/0.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);

	x+=96;
	size = read_image_from_file("./btns/ent.jpg", &jpgdata);
	show_jpg(jpgdata, size, NOTPIP, x, y);
}


void login(void)
{
	show_login("./btns/login.jpg");

	int ts_x,ts_y;
	int ts_x_s=100, ts_y_s=160;
	
	while(1)
	{
		get_xy(&ts_x, &ts_y, WAIT);
	
		printf("ts_x=%d ts_y=%d\n",ts_x,ts_y);

		if(ts_x>ts_x_s && ts_x<ts_x_s+96)
		{
			if(ts_y>ts_y_s && ts_y<ts_y_s+64)
			{
				printf("get key1\n");
				key_add(1);	
			}
			if(ts_y>ts_y_s+64 && ts_y<ts_y_s+64*2)
			{
				printf("get key4\n");
				key_add(4);	
			}
			if(ts_y>ts_y_s+64*2 && ts_y<ts_y_s+64*3)
			{
				printf("get key7\n");
				key_add(7);	
			}
			if(ts_y>ts_y_s+64*3 && ts_y<ts_y_s+64*4)
			{
				printf("get key del\n");
				key_del();	
			}
		}
		if(ts_x>ts_x_s+96 && ts_x<ts_x_s+96+64)
		{
			if(ts_y>ts_y_s && ts_y<ts_y_s+64)
			{
				printf("get key2\n");
				key_add(2);	
			}
			if(ts_y>ts_y_s+64 && ts_y<ts_y_s+64*2)
			{
				printf("get key5\n");
				key_add(5);	
			}
			if(ts_y>ts_y_s+64*2 && ts_y<ts_y_s+64*3)
			{
				printf("get key8\n");
				key_add(8);	
			}
			if(ts_y>ts_y_s+64*3 && ts_y<ts_y_s+64*4)
			{
				printf("get key0\n");
				key_add(0);	
			}
		}
		if(ts_x>ts_x_s+96*2 && ts_x<ts_x_s+96*2+64)
		{
			if(ts_y>ts_y_s && ts_y<ts_y_s+64)
			{
				printf("get key3\n");
				key_add(3);	
			}
			if(ts_y>ts_y_s+64 && ts_y<ts_y_s+64*2)
			{
				printf("get key6\n");
				key_add(6);	
			}
			if(ts_y>ts_y_s+64*2 && ts_y<ts_y_s+64*3)
			{
				printf("get key9\n");
				key_add(9);	
			}
			if(ts_y>ts_y_s+64*3 && ts_y<ts_y_s+64*4)
			{
				printf("get key enter\n");
				enter_login();
				if(g_pass_login_success)
					break;
			}
		}
		if(ts_x>550 && ts_x<750 && ts_y>300 && ts_y<400)
		{
			enter_change_key();
			continue;
		}
	}	
}



void key_add(int n)
{
	int x=30,y=86;
	unsigned char *jpgdata;
	int size;

	if(g_key_pass_offset < 6)
	{
		size = read_image_from_file(g_number_path[n], &jpgdata);
		show_jpg(jpgdata, size, NOTPIP, x+g_key_pass_offset*64, y);

		g_key_pass_buf[g_key_pass_offset] = (char)(n+'0');
		g_key_pass_offset++;
	}	
}


void enter_login()
{
	int ret=1; 
	printf("pass buf = %s\n",g_key_pass_buf);
	printf("login key = %s\n",g_key_login);
	if((ret = (strncmp(g_key_login,g_key_pass_buf, strlen(g_key_login)))) == 0)
	{
		printf("enter login\n");
		g_pass_login_success = 1;
	}
	else
	{
		printf("error password\n");
		g_pass_login_success = 0;	
	}	
	printf("ret = %d\n",ret);
}

void key_del()
{
	int x=30, y=86;
	unsigned char *jpgdata;

	if(g_key_pass_offset > 0)
	{
		g_key_pass_offset--;


		int size = read_image_from_file("./btns/blank.jpg", &jpgdata);
		show_jpg(jpgdata, size, NOTPIP, x+g_key_pass_offset*64, y);
	}	
}

void login_key_change(int n)
{
	int x=30,y=86;
	if(g_key_pass_offset < 6)
	{
		unsigned char *jpgdata;
		int size = read_image_from_file(g_number_path[n], &jpgdata);
		show_jpg(jpgdata, size, NOTPIP, x+g_key_pass_offset*64, y);

		g_key_login[g_key_pass_offset] = n+'0';
		g_key_pass_offset++;
	}	
}


void enter_change_key()
{
	//进入密码登录界面
	show_login("./btns/change_key.jpg");
	int ts_x,ts_y;
	int ts_x_s=100,ts_y_s=160;
	g_key_pass_offset = 0;
	bzero(g_key_login,6);
	while(1)
	{
		get_xy(&ts_x, &ts_y, WAIT);
		printf("ts_x=%d ts_y=%d\n",ts_x,ts_y);

		if(ts_x>ts_x_s && ts_x<ts_x_s+96)
		{
			if(ts_y>ts_y_s && ts_y<ts_y_s+64)
			{
				printf("get key1\n");
				login_key_change(1);	
			}
			if(ts_y>ts_y_s+64 && ts_y<ts_y_s+64*2)
			{
				printf("get key4\n");
				login_key_change(4);	
			}
			if(ts_y>ts_y_s+64*2 && ts_y<ts_y_s+64*3)
			{
				printf("get key7\n");
				login_key_change(7);	
			}
			if(ts_y>ts_y_s+64*3 && ts_y<ts_y_s+64*4)
			{
				printf("get key del\n");
				key_del();	
			}
		}
		if(ts_x>ts_x_s+96 && ts_x<ts_x_s+96+64)
		{
			if(ts_y>ts_y_s && ts_y<ts_y_s+64)
			{
				printf("get key2\n");
				login_key_change(2);	
			}
			if(ts_y>ts_y_s+64 && ts_y<ts_y_s+64*2)
			{
				printf("get key5\n");
				login_key_change(5);	
			}
			if(ts_y>ts_y_s+64*2 && ts_y<ts_y_s+64*3)
			{
				printf("get key8\n");
				login_key_change(8);	
			}
			if(ts_y>ts_y_s+64*3 && ts_y<ts_y_s+64*4)
			{
				printf("get key0\n");
				login_key_change(0);	
			}
		}
		if(ts_x>ts_x_s+96*2 && ts_x<ts_x_s+96*2+64)
		{
			if(ts_y>ts_y_s && ts_y<ts_y_s+64)
			{
				printf("get key3\n");
				login_key_change(3);	
			}
			if(ts_y>ts_y_s+64 && ts_y<ts_y_s+64*2)
			{
				printf("get key6\n");
				login_key_change(6);	
			}
			if(ts_y>ts_y_s+64*2 && ts_y<ts_y_s+64*3)
			{
				printf("get key9\n");
				login_key_change(9);	
			}
			if(ts_y>ts_y_s+64*3 && ts_y<ts_y_s+64*4)
			{
				printf("get key enter\n");
				g_key_pass_offset = 0;
				show_login("./btns/logoin.jpg");
					break;
			}
		}
	}	
}
