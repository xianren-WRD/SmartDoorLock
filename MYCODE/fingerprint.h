#ifndef __FINGERPRINT_H
#define __FINGERPRINT_H

#include "stm32f4xx.h"
#include "as608.h"
#include <stdbool.h>

void Show_Fingerprint_Management(void);
u16 Get_Input_ID(void);
void Add_FR(void);	//录指纹
void Del_FR(void);	//删除指纹
bool Press_FR(SearchResult *search);//刷指纹
void ShowErrMessage(u8 ensure);//显示确认码错误信息
void Get_Fingerprint_Module_Parameters(void);
void View_Exist_Fingerprint(void);
bool Fingerprint_Already_Exist(u8 *FingerID_buf, u16 id);

#endif
