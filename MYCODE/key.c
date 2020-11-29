#include "key.h"
#include "stdio.h"
#include "delay.h"
#include <includes.h>

/*
����˵����
PB7 -- 1
PA4 -- 2
PD6 -- 3
PD7 -- 4
PC6 -- 5
PC7 -- 6
PC8 -- 7
PC9 -- 8
*/
void Key_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ��GPIO A��ʱ��
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹ��GPIO B��ʱ��
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//ʹ��GPIO C��ʱ��
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);//ʹ��GPIO D��ʱ��

    GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_7;		//����7
    GPIO_InitStruct.GPIO_Mode	= GPIO_Mode_OUT;	//���ģʽ
    GPIO_InitStruct.GPIO_OType	= GPIO_OType_PP;	//����
    GPIO_InitStruct.GPIO_Speed	= GPIO_Speed_50MHz; //����
    GPIO_InitStruct.GPIO_PuPd	= GPIO_PuPd_UP;     //����
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_4;		//����4
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_6 | GPIO_Pin_7;		//����6,7
    GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;//����6789
    GPIO_InitStruct.GPIO_Mode	= GPIO_Mode_IN;		//����ģʽ
    GPIO_InitStruct.GPIO_PuPd	= GPIO_PuPd_UP;     //����
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    //����0
    PBout(7) = 0;
    PAout(4) = 0;
    PDout(6) = 0;
    PDout(7) = 0;
}

//���ܣ�ȷ�����µİ���������һ�У����ȴ������ͷ�
//������row -- ���µİ������ڵ���
//����ֵ�����°������ڵ���
unsigned int Key_Scan(unsigned int row)
{
    PBout(7) = 1;
    if(PCin(row) == 1)
    {
        PBout(7) = 0;
        while(PCin(row) == 0)delay_ms(1);//�ȴ������ͷ�
        return 1;
    }
    PBout(7) = 0;//������Ǹ�����������0�����ﲻҪ�����ˣ���Ȼ���ܻ�Ӱ�������ɨ����

    PAout(4) = 1;
    if(PCin(row) == 1)
    {
        PAout(4) = 0;
        while(PCin(row) == 0)delay_ms(1);
        return 2;
    }
    PAout(4) = 0;

    PDout(6) = 1;
    if(PCin(row) == 1)
    {
        PDout(6) = 0;
        while(PCin(row) == 0)delay_ms(1);
        return 3;
    }
    PDout(6) = 0;

    PDout(7) = 1;
    if(PCin(row) == 1)
    {
        PDout(7) = 0;
        while(PCin(row) == 0)delay_ms(1);
        return 4;
    }
    PDout(7) = 0;
}

//����:�ȴ��������£���ʶ������µİ����ļ�ֵ
//����:�������¼���־��
//����ֵ:���°����ļ�ֵ
unsigned char GetKeyValue(OS_FLAG_GRP *flag_grp)
{
    unsigned char key_value = 255;//Ĭ��255Ϊ��Чֵ
    unsigned short column = 0;

    OS_ERR  err;
    OS_FLAGS flags = 0;
	
	OSFlagPost(flag_grp, 0x0F, OS_OPT_POST_FLAG_CLR, &err);//������֮ǰ�����״̬���������Ҫ��
	
    //�����ȴ��¼���־��bit0/bit1/bit2/bit3��1���ȴ��ɹ��󣬽�����0
    flags = OSFlagPend(flag_grp, 0x0F, 0,
                       OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_BLOCKING + OS_OPT_PEND_FLAG_CONSUME,
                       NULL, &err);

    if(err != OS_ERR_NONE)
    {
        printf("[taskKey][OSFlagPend]Error Code = %d\r\n", err);
        return key_value;
    }

    if(flags & 0x01)//��һ���а�������
    {
        NVIC_DisableIRQ(EXTI9_5_IRQn);//��ֹEXTI5-9�����ж�
        if(PCin(6) == 0)//��ⰴ��״̬
        {
            delay_ms(15);//��ʱ����
            if(PCin(6) == 0)
            {
                //printf("flag group bit0 set ok\r\n");
                column = Key_Scan(6);
                if(column == 1) 	 key_value = KEY_1;
                else if(column == 2) key_value = KEY_2;
                else if(column == 3) key_value = KEY_3;
                else if(column == 4) key_value = KEY_A;
            }
        }
        NVIC_EnableIRQ(EXTI9_5_IRQn);//����EXTI5-9�����ж�
        EXTI_ClearITPendingBit(EXTI_Line6);//����жϱ�־λ
    }
    if(flags & 0x02)
    {
        NVIC_DisableIRQ(EXTI9_5_IRQn);
        if(PCin(7) == 0)//��ⰴ��״̬
        {
            delay_ms(15);//��ʱ����
            if(PCin(7) == 0)
            {
                column = Key_Scan(7);
                if(column == 1) 	 key_value = KEY_4;
                else if(column == 2) key_value = KEY_5;
                else if(column == 3) key_value = KEY_6;
                else if(column == 4) key_value = KEY_B;
            }
        }

        NVIC_EnableIRQ(EXTI9_5_IRQn);
        EXTI_ClearITPendingBit(EXTI_Line7);
    }
    if(flags & 0x04)
    {
        NVIC_DisableIRQ(EXTI9_5_IRQn);
        if(PCin(8) == 0)//��ⰴ��״̬
        {
            delay_ms(15);//��ʱ����
            if(PCin(8) == 0)
            {
                column = Key_Scan(8);
                if(column == 1)		 key_value = KEY_7;
                else if(column == 2) key_value = KEY_8;
                else if(column == 3) key_value = KEY_9;
                else if(column == 4) key_value = KEY_C;
            }
        }

        NVIC_EnableIRQ(EXTI9_5_IRQn);
        EXTI_ClearITPendingBit(EXTI_Line8);
    }
    if(flags & 0x08)
    {
        NVIC_DisableIRQ(EXTI9_5_IRQn);
        if(PCin(9) == 0)//��ⰴ��״̬
        {
            delay_ms(15);//��ʱ����
            if(PCin(9) == 0)
            {
                column = Key_Scan(9);
                if(column == 1) 	 key_value = KEY_SHIFT_8;
                else if(column == 2) key_value = KEY_0;
                else if(column == 3) key_value = KEY_SHIFT_3;
                else if(column == 4) key_value = KEY_D;
            }
        }

        NVIC_EnableIRQ(EXTI9_5_IRQn);
        EXTI_ClearITPendingBit(EXTI_Line9);
    }
    return key_value;
}
