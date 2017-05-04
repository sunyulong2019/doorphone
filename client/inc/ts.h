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

#define NOWAIT 0
#define WAIT   1

// 九宫格按键
#define ONE   1
#define TWO   2
#define THREE 3
#define FOUR  4
#define FIVE  5
#define SIX   6
#define SEVEN 7
#define EIGHT 8
#define NINE  9
#define ZERO  0
#define DEL   10
#define ENT   11


/* ++++++++ 变量声明 ++++++++ */
extern struct tsdev *g_ts;


/* ++++++++ 函数声明 ++++++++ */
void ts_init(char *device_name);
void get_xy(int *px, int *py, int flag); // flag可以是NOWAIT，或者WAIT
bool within_pip(int x, int y);
void move_pip(int x_base, int y_base);
int pos(int x, int y);

#endif