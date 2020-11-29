#include "exti.h"
#include <includes.h>

extern OS_FLAG_GRP  key_flag_grp;

/*
����˵����
PC6 -- 5 -- ��һ��
PC7 -- 6 -- �ڶ���
PC8 -- 7 -- ������
PC9 -- 8 -- ������
������������Ӧ���ж���
*/
void Exti_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    EXTI_InitTypeDef  EXTI_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

    //ʹ��GPIO A��ʱ�ӣ�
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    //ʹ��SYSCFGʱ�ӣ�
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    GPIO_InitStruct.GPIO_Pin    = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;//����6��7��8��9
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_IN;     //����ģʽ
    GPIO_InitStruct.GPIO_PuPd   = GPIO_PuPd_UP;     //����
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    //����IO�����ж��ߵ�ӳ���ϵ��ȷ��ʲô���Ŷ�Ӧ�ĸ��ж���
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource6);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource7);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource8);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource9);

    EXTI_InitStruct.EXTI_Line   = EXTI_Line6 | EXTI_Line7 | EXTI_Line8 | EXTI_Line9;//�ж���
    EXTI_InitStruct.EXTI_Mode   = EXTI_Mode_Interrupt;  //�ж�ģʽ
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling; //�½��ش���
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;              //�ж���ʹ��
    //��ʼ�������жϣ����ô��������ȡ�
    EXTI_Init(&EXTI_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel                     = EXTI9_5_IRQn;  //NVICͨ������stm32f4xx.h�ɲ鿴ͨ�� ���ɱ䣩
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority   = 0x01;         //��ռ���ȼ�
    NVIC_InitStruct.NVIC_IRQChannelSubPriority          = 0x01;         //��Ӧ���ȼ�
    NVIC_InitStruct.NVIC_IRQChannelCmd                  = ENABLE;       //ʹ��
    //�����жϷ��飨NVIC������ʹ���жϡ�
    NVIC_Init(&NVIC_InitStruct);
}

//�����жϷ�������
void EXTI9_5_IRQHandler(void)
{
    OS_ERR err;
    //�����ж�
    OSIntEnter();

    //����־λ
    if(EXTI_GetITStatus(EXTI_Line6) == SET)
    {
        //�����¼���־��
        OSFlagPost(&key_flag_grp, 0x01, OS_OPT_POST_FLAG_SET, &err);
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
    else if(EXTI_GetITStatus(EXTI_Line7) == SET)
    {
        //�����¼���־��
        OSFlagPost(&key_flag_grp, 0x02, OS_OPT_POST_FLAG_SET, &err);
        EXTI_ClearITPendingBit(EXTI_Line7);
    }
    else if(EXTI_GetITStatus(EXTI_Line8) == SET)
    {
        //�����¼���־��
        OSFlagPost(&key_flag_grp, 0x04, OS_OPT_POST_FLAG_SET, &err);
        EXTI_ClearITPendingBit(EXTI_Line8);
    }
    else if(EXTI_GetITStatus(EXTI_Line9) == SET)
    {
        //�����¼���־��
        OSFlagPost(&key_flag_grp, 0x08, OS_OPT_POST_FLAG_SET, &err);
        EXTI_ClearITPendingBit(EXTI_Line9);
    }

    //�˳��ж�
    OSIntExit();
}
