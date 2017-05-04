#ifndef __TS_H
#define __TS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/input.h>

#include "tslib.h"

#define SENSITIVITY 1

#define BTN1 1
#define BTN2 2
#define BTN3 3
#define BTN4 4

/* ++++++++ 变量声明 ++++++++ */
extern struct tsdev *g_ts;


/* ++++++++ 函数声明 ++++++++ */
void ts_init(char *device_name);
void get_xy(int *px, int *py);
bool within_pip(int x, int y);
void move_pip(int x_base, int y_base);
int pos(int x, int y);

#endif