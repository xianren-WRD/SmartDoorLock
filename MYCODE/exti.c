#include "exti.h"
#include <includes.h>

extern OS_FLAG_GRP  key_flag_grp;

/*
引脚说明：
PC6 -- 5 -- 第一行
PC7 -- 6 -- 第二行
PC8 -- 7 -- 第三行
PC9 -- 8 -- 第四行
按键的行所对应的中断线
*/
void Exti_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    EXTI_InitTypeDef  EXTI_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

    //使能GPIO A组时钟，
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    //使能SYSCFG时钟：
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    GPIO_InitStruct.GPIO_Pin    = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;//引脚6，7，8，9
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_IN;     //输入模式
    GPIO_InitStruct.GPIO_PuPd   = GPIO_PuPd_UP;     //上拉
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    //设置IO口与中断线的映射关系。确定什么引脚对应哪个中断线
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource6);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource7);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource8);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource9);

    EXTI_InitStruct.EXTI_Line   = EXTI_Line6 | EXTI_Line7 | EXTI_Line8 | EXTI_Line9;//中断线
    EXTI_InitStruct.EXTI_Mode   = EXTI_Mode_Interrupt;  //中断模式
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿触发
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;              //中断线使能
    //初始化线上中断，设置触发条件等。
    EXTI_Init(&EXTI_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel                     = EXTI9_5_IRQn;  //NVIC通道，在stm32f4xx.h可查看通道 （可变）
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority   = 0x01;         //抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority          = 0x01;         //响应优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd                  = ENABLE;       //使能
    //配置中断分组（NVIC），并使能中断。
    NVIC_Init(&NVIC_InitStruct);
}

//按键中断服务函数。
void EXTI9_5_IRQHandler(void)
{
    OS_ERR err;
    //进入中断
    OSIntEnter();

    //检测标志位
    if(EXTI_GetITStatus(EXTI_Line6) == SET)
    {
        //设置事件标志组
        OSFlagPost(&key_flag_grp, 0x01, OS_OPT_POST_FLAG_SET, &err);
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
    else if(EXTI_GetITStatus(EXTI_Line7) == SET)
    {
        //设置事件标志组
        OSFlagPost(&key_flag_grp, 0x02, OS_OPT_POST_FLAG_SET, &err);
        EXTI_ClearITPendingBit(EXTI_Line7);
    }
    else if(EXTI_GetITStatus(EXTI_Line8) == SET)
    {
        //设置事件标志组
        OSFlagPost(&key_flag_grp, 0x04, OS_OPT_POST_FLAG_SET, &err);
        EXTI_ClearITPendingBit(EXTI_Line8);
    }
    else if(EXTI_GetITStatus(EXTI_Line9) == SET)
    {
        //设置事件标志组
        OSFlagPost(&key_flag_grp, 0x08, OS_OPT_POST_FLAG_SET, &err);
        EXTI_ClearITPendingBit(EXTI_Line9);
    }

    //退出中断
    OSIntExit();
}
