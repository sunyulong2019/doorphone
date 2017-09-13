///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: doorphone/client/inc/login.h
//  日期: 2017-9
//  描述: 
//
//  作者: Vincent Lin (林世霖)  微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////

#ifndef __LOGIN_H
#define __LOGIN_H

void login(void);
void key_add(int n);
void key_del(void);
void enter_login(void);
void enter_change_key(void);

extern int  g_key_pass_offset;

#endif
