#ifndef __LOGIN_H
#define __LOGIN_H

void login(void);
void key_add(int n);
void key_del(void);
void enter_login(void);
void enter_change_key(void);

extern int  g_key_pass_offset;

#endif