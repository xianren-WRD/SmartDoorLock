#include "beep.h"

/*
����˵����
BEEP	PF8����
*/

void Beep_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);//ʹ��GPIO F��ʱ��
	
	GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_8;		//����8
	GPIO_InitStruct.GPIO_Mode	= GPIO_Mode_OUT;	//���ģʽ
	GPIO_InitStruct.GPIO_OType	= GPIO_OType_PP;	//����
	GPIO_InitStruct.GPIO_Speed	= GPIO_Speed_50MHz; //����
	GPIO_InitStruct.GPIO_PuPd	= GPIO_PuPd_UP;     //����
	GPIO_Init(GPIOF, &GPIO_InitStruct);
	
	GPIO_ResetBits(GPIOF,GPIO_Pin_8);
}
