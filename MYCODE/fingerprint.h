#ifndef __FINGERPRINT_H
#define __FINGERPRINT_H

#include "stm32f4xx.h"
#include "as608.h"
#include <stdbool.h>

void Show_Fingerprint_Management(void);
u16 Get_Input_ID(void);
void Add_FR(void);	//¼ָ��
void Del_FR(void);	//ɾ��ָ��
bool Press_FR(SearchResult *search);//ˢָ��
void ShowErrMessage(u8 ensure);//��ʾȷ���������Ϣ
void Get_Fingerprint_Module_Parameters(void);
void View_Exist_Fingerprint(void);
bool Fingerprint_Already_Exist(u8 *FingerID_buf, u16 id);

#endif
