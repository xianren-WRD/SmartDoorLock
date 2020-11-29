#ifndef __PASSWORD_H
#define __PASSWORD_H

#include "stm32f4xx.h"
#include "sys.h"
#include <includes.h>

void Show_Fun_Interface(void);//显示功能界面
void Show_Enter_Password(void);
unsigned char Enter_Password(OS_FLAG_GRP *flag_grp, unsigned char *password);
void Show_Password_Setting(void);
bool Password_validation(OS_FLAG_GRP *flag_grp, unsigned char *password);

#endif
