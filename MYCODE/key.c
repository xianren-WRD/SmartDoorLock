#include "key.h"
#include "stdio.h"
#include "delay.h"
#include <includes.h>

/*
引脚说明：
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

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIO A组时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能GPIO B组时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能GPIO C组时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);//使能GPIO D组时钟

    GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_7;		//引脚7
    GPIO_InitStruct.GPIO_Mode	= GPIO_Mode_OUT;	//输出模式
    GPIO_InitStruct.GPIO_OType	= GPIO_OType_PP;	//推挽
    GPIO_InitStruct.GPIO_Speed	= GPIO_Speed_50MHz; //快速
    GPIO_InitStruct.GPIO_PuPd	= GPIO_PuPd_UP;     //上拉
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_4;		//引脚4
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_6 | GPIO_Pin_7;		//引脚6,7
    GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;//引脚6789
    GPIO_InitStruct.GPIO_Mode	= GPIO_Mode_IN;		//输入模式
    GPIO_InitStruct.GPIO_PuPd	= GPIO_PuPd_UP;     //上拉
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    //列置0
    PBout(7) = 0;
    PAout(4) = 0;
    PDout(6) = 0;
    PDout(7) = 0;
}

//功能：确定按下的按键属于哪一列，并等待按键释放
//参数：row -- 按下的按键所在的行
//返回值：按下按键所在的列
unsigned int Key_Scan(unsigned int row)
{
    PBout(7) = 1;
    if(PCin(row) == 1)
    {
        PBout(7) = 0;
        while(PCin(row) == 0)delay_ms(1);//等待按键释放
        return 1;
    }
    PBout(7) = 0;//如果不是该列则重新置0。这里不要忘记了，不然可能会影响下面的扫描结果

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

//功能:等待按键按下，并识别出按下的按键的键值
//参数:按键的事件标志组
//返回值:按下按键的键值
unsigned char GetKeyValue(OS_FLAG_GRP *flag_grp)
{
    unsigned char key_value = 255;//默认255为无效值
    unsigned short column = 0;

    OS_ERR  err;
    OS_FLAGS flags = 0;
	
	OSFlagPost(flag_grp, 0x0F, OS_OPT_POST_FLAG_CLR, &err);//先清理之前保存的状态，这里很重要！
	
    //阻塞等待事件标志组bit0/bit1/bit2/bit3置1，等待成功后，将其清0
    flags = OSFlagPend(flag_grp, 0x0F, 0,
                       OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_BLOCKING + OS_OPT_PEND_FLAG_CONSUME,
                       NULL, &err);

    if(err != OS_ERR_NONE)
    {
        printf("[taskKey][OSFlagPend]Error Code = %d\r\n", err);
        return key_value;
    }

    if(flags & 0x01)//第一行有按键按下
    {
        NVIC_DisableIRQ(EXTI9_5_IRQn);//禁止EXTI5-9触发中断
        if(PCin(6) == 0)//检测按键状态
        {
            delay_ms(15);//延时消抖
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
        NVIC_EnableIRQ(EXTI9_5_IRQn);//允许EXTI5-9触发中断
        EXTI_ClearITPendingBit(EXTI_Line6);//清空中断标志位
    }
    if(flags & 0x02)
    {
        NVIC_DisableIRQ(EXTI9_5_IRQn);
        if(PCin(7) == 0)//检测按键状态
        {
            delay_ms(15);//延时消抖
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
        if(PCin(8) == 0)//检测按键状态
        {
            delay_ms(15);//延时消抖
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
        if(PCin(9) == 0)//检测按键状态
        {
            delay_ms(15);//延时消抖
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
