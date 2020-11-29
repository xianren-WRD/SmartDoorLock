#ifndef __KEY_H
#define __KEY_H

#include "stm32f4xx.h"
#include "sys.h"
#include <includes.h>

#define KEY_0 '0'
#define KEY_1 '1'
#define KEY_2 '2'
#define KEY_3 '3'
#define KEY_4 '4'
#define KEY_5 '5'
#define KEY_6 '6'
#define KEY_7 '7'
#define KEY_8 '8'
#define KEY_9 '9'
#define KEY_A 'A' 		//删除键
#define KEY_B 'B' 		//上锁键（模拟上锁操作）
#define KEY_C 'C' 		//确认键
#define KEY_D 'D'		//返回键
#define KEY_SHIFT_3  '#'
#define KEY_SHIFT_8  '*'

void Key_Init(void);
unsigned int Key_Scan(unsigned int row);
unsigned char GetKeyValue(OS_FLAG_GRP *flag_grp);

#endif
