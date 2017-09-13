///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/doorphone/lcd.c
//  日期: 2017-9
//  描述: 提供LCD显示子模块功能
//
//  作者: Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////



#include "lcd.h"
#include "cam.h"
#include "jpeglib.h"

int g_lcdfd = -1;
unsigned int *g_fb_mem = NULL;
struct fb_var_screeninfo *g_lcdinfo = NULL;
pthread_mutex_t lcd_lock;
int g_pip = -1;

int g_xoffset = 100, g_yoffset = 200;


/*
*** 功能：将指定的jpg文件内容读出并放置到jpgdata指向的指针所指向的内存中
*** 参数：jpg_file_name >> jpg文件名称
***       jpgdata >> jpg数据存放位置的内存指针的指针
*** 返回：读取到的数据大小（以字节为单位）
*/
int read_image_from_file(char *jpg_file_name, unsigned char **jpgdata)
{
	int fd = open(jpg_file_name, O_RDONLY);
	if(fd == -1)
	{
		fprintf(stderr, "[%s][%s][%d] open() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	// 获取文件大小，并根据大小分配适当的内存
	int jpg_size = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0l, SEEK_SET);

	*jpgdata = calloc(1, jpg_size);

	int n, total_bytes = 0;
	unsigned char *tmp = *jpgdata;
	while(jpg_size > 0)
	{
		while((n=read(fd, tmp, jpg_size)) == -1 && errno == EINTR);

		if(n == 0)
			break;
		if(n == -1)
		{
			fprintf(stderr, "[%s][%s][%d] read() failed: %s\n",
							__FILE__, __FUNCTION__, __LINE__, strerror(errno));
			exit(0);
		}

		tmp += n;
		total_bytes += n;
		jpg_size -= n;
	}

	return total_bytes;
}

/*
*** 功能：将指定的jpg数据显示到LCD上
*** 参数：jpgdata >> jpg格式的数据
***       size >> jpg数据大小（字节）
***       pip  >> 是否正在显示画中画图像（如果为真，参数x和y则无效）
***       x y  >> 图像左上角显示的位置
*** 返回：
*/
int show_jpg(unsigned char *jpgdata, int size, bool pip, int x, int y)
{
	if(size <= 0)
		return -1;

	// 声明解压缩结构体，以及错误管理结构体
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	// 使用缺省的出错处理来初始化解压缩结构体
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// 配置该cinfo，使其从jpg_buffer中读取jpg_size个字节
	// 这些数据必须是完整的JPEG数据
	jpeg_mem_src(&cinfo, jpgdata, size);

	// 读取JPEG文件的头
	if(jpeg_read_header(&cinfo, true) != JPEG_HEADER_OK)
	{
		return -1;
	}

	// 开始解压jpeg数据
	jpeg_start_decompress(&cinfo);

	// 定义一个存放一行数据的缓冲区
	char *row_buf = calloc(cinfo.output_width, cinfo.output_components);

	// 以行为单位，将JPEG数据解压出来之后，按照RGBA方式填入g_fb_mem
	int red  = 0;
	int green= 1;
	int blue = 2;

	// 以320 * 240大小显示本地视频（画中画）
	pthread_mutex_lock(&lcd_lock);
	unsigned int *tmp_fb_mem = g_fb_mem;
	if(pip)
	{
		tmp_fb_mem += (g_yoffset*800 + g_xoffset);

		int count = 1, line=0;
		while (cinfo.output_scanline < cinfo.output_height &&
			   cinfo.output_scanline <= g_lcdinfo->yres)
		{
			jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&row_buf, 1);

			// 每两行显示一行
			if((count++)%2 == 0)
				continue;

			// 将row_buf中的24bit的图像数据写入32bit的g_fb_mem
			int i=0, x;
			for(x=0; x<cinfo.output_width/2 && x < g_lcdinfo->xres/2; x++)
			{
				*(tmp_fb_mem+line*800+x) =
					row_buf[i+blue ]<<0 |
					row_buf[i+green]<<8 |
					row_buf[i+red  ]<<16;

				i += 6; // 每两个像素点显示一个
			}
			line++;
		}
	}

	// 显示普通jpg图像（包括以640 * 480大小显示对端监控视频）
	else
	{
		tmp_fb_mem += (y*800 + x);

		while (cinfo.output_scanline < cinfo.output_height && cinfo.output_scanline <= g_lcdinfo->yres)
		{
			jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&row_buf, 1);

			// 将row_buf中的24bit的图像数据写入32bit的g_fb_mem
			int i=0, j;
			for(j=0; j<cinfo.output_width && j < g_lcdinfo->xres; j++)
			{
				// 当（1）不显示画中画或者（2）不在画中画范围内的时候，显示图像
				if(g_pip!=1 || (!(j > g_xoffset-x && j < g_xoffset-x+320 &&
				  (cinfo.output_scanline-1) > g_yoffset && (cinfo.output_scanline-1) < g_yoffset+240)))
				{
					*(tmp_fb_mem+(cinfo.output_scanline-1)*800+j) =
						row_buf[i+blue ]<<0 |
						row_buf[i+green]<<8 |
						row_buf[i+red  ]<<16;
				}
				i += 3;
			}
		}	
	}
	pthread_mutex_unlock(&lcd_lock);

	// 释放相关结构体和内存，解压后的图片信息被保留在g_fb_mem，直接映射到了LCD
	free(row_buf);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return 0;
}


void lcd_init(char *device_name)
{
	// 初始化LCD设备访问互斥锁
	pthread_mutex_init(&lcd_lock, NULL);

	g_lcdfd = open(device_name, O_RDWR);
	if(g_lcdfd == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: open() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	// 获取LCD设备的参数规格
	g_lcdinfo = calloc(1, sizeof(struct fb_var_screeninfo));
	if(ioctl(g_lcdfd, FBIOGET_VSCREENINFO, g_lcdinfo) == -1)
	{
		fprintf(stderr, "[%s][%s][%d]: ioctl() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	// 申请LCD设备映射内存（显存）
	g_fb_mem = mmap(NULL, g_lcdinfo->xres * g_lcdinfo->yres * g_lcdinfo->bits_per_pixel/8,
					PROT_READ|PROT_WRITE, MAP_SHARED, g_lcdfd, 0);
	if(g_fb_mem == MAP_FAILED)
	{
		fprintf(stderr, "[%s][%s][%d]: mmap() failed: %s\n",
						__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		exit(0);
	}

	// 将屏幕刷成黑色
	unsigned long black = 0x0;
	unsigned int i;
	for(i=0; i<g_lcdinfo->xres * g_lcdinfo->yres; i++)
	{
		memcpy(g_fb_mem+i, &black, sizeof(unsigned long));
	}

}
