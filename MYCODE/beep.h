#ifndef __BEEP_H
#define __BEEP_H
#include "stm32f4xx.h"

#define BEEP_ON 	GPIO_SetBits(GPIOF, GPIO_Pin_8)
#define BEEP_OFF 	GPIO_ResetBits(GPIOF, GPIO_Pin_8)

void Beep_Init(void);

#endif
