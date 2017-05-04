#ifndef __LCD_H
#define __LCD_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>
#include <linux/fb.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>


#define OFF -1
#define ON  1

#define NOTPIP -1
#define PIP    1

/* ++++++++ 变量声明 ++++++++ */
extern int g_lcdfd;
extern unsigned int *g_fb_mem;
extern struct fb_var_screeninfo *g_lcdinfo;
extern int g_xoffset, g_yoffset; // 画中画的坐标（左上角）
extern int g_pip;

/* ++++++++ 函数声明 ++++++++ */
void lcd_init(char *device_name);
int read_image_from_file(char *jpg_file_name, unsigned char **jpgdata);
void show_jpg(unsigned char * jpgdata, int size, int mode, int x, int y);


#endif
