#include "sys.h"
#include "usart.h"
#include "timer.h"
#include "includes.h"					//ucos 使用	  

extern OS_TCB  BluetoothTCB;

//加入以下代码,支持printf函数,而不需要选择use MicroLIB
//#pragma import(__use_no_semihosting)
//标准库需要的支持函数
struct __FILE
{
    int handle;
};

FILE __stdout;
//定义_sys_exit()以避免使用半主机模式
int _sys_exit(int x)
{
    x = x;
}
_ttywrch(int ch)
{
    ch = ch;
}
//重定义fputc函数
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, ch); //通过串口发送数据
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

//初始化IO 串口1
//baud:波特率--默认：115200
void Usart1_Init(u32 baud)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

    // 串口是挂载在 APB2 下面的外设，所以使能函数为
    //使能 USART1 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    //使用的是串口 1，串口 1 对应着芯片引脚 PA9,PA10 需要使能PA的时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    //设置引脚复用器映射
    //引脚复用器映射配置，需要配置PA9，PA10 的引脚，调用函数为：
    //PA9 复用为 USART1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    //PA10 复用为 USART1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9 与 GPIOA10
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;				//配置IO口复用功能
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz; 		//速度 50MHz
    GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP; 			//推挽复用输出
    GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_UP; 			//上拉
    //初始化 PA9，PA10
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStruct.USART_BaudRate 	= baud;					//一般设置为 115200;
    USART_InitStruct.USART_WordLength 	= USART_WordLength_8b;	//字长为 8 位数据格式
    USART_InitStruct.USART_StopBits 	= USART_StopBits_1;		//一个停止位
    USART_InitStruct.USART_Parity 		= USART_Parity_No;		//无奇偶校验位
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //硬件控制流
    USART_InitStruct.USART_Mode 		= USART_Mode_Rx | USART_Mode_Tx;	//收发模式  双全工
    //初始化串口
    USART_Init(USART1, &USART_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel						= USART1_IRQn;	//NVIC通道，在stm32f4xx.h可查看通道 （可变）
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority	= 0x01;			//抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority			= 0x01;			//响应优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd					= ENABLE;		//使能
    //配置中断分组（NVIC），并使能中断。
    NVIC_Init(&NVIC_InitStruct);

    //配置串口接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART1, ENABLE);
}

//初始化IO 串口2
//波特率:9600
void Usart2_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

    //使能 USART1 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    //使用的是串口2，串口2对应着芯片引脚 PA2,PA3 需要使能PA的时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    //设置引脚复用器映射
    //引脚复用器映射配置，需要配置PA2，PA3 的引脚，调用函数为：
    //PA2 复用为 USART2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    //PA3 复用为 USART2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    GPIO_InitStruct.GPIO_Pin 	= GPIO_Pin_2 | GPIO_Pin_3; 	//GPIOA2 与 GPIOA3
    GPIO_InitStruct.GPIO_Mode 	= GPIO_Mode_AF;				//配置IO口复用功能
    GPIO_InitStruct.GPIO_Speed 	= GPIO_Speed_50MHz; 		//速度50MHz
    GPIO_InitStruct.GPIO_OType 	= GPIO_OType_PP; 			//推挽复用输出
    GPIO_InitStruct.GPIO_PuPd 	= GPIO_PuPd_UP; 			//上拉
    //初始化 PA2，PA3
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate 	= 9600;					//一般设置为9600;
    USART_InitStruct.USART_WordLength 	= USART_WordLength_8b;	//字长为 8 位数据格式
    USART_InitStruct.USART_StopBits 	= USART_StopBits_1;		//一个停止位
    USART_InitStruct.USART_Parity 		= USART_Parity_No;		//无奇偶校验位
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //硬件控制流
    USART_InitStruct.USART_Mode 		= USART_Mode_Rx | USART_Mode_Tx;		//收发模式  双全工
    //初始化串口
    USART_Init(USART2, &USART_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel						= USART2_IRQn;  //NVIC通道，在stm32f4xx.h可查看通道 （可变）
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority	= 0x01;			//抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority			= 0x01;			//响应优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd					= ENABLE;		//使能
    //配置中断分组（NVIC），并使能中断。
    NVIC_Init(&NVIC_InitStruct);

    //配置串口接收中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART2, ENABLE);
}

/*
引脚说明：
PB10 -- USART3_TX
PB11 -- USART3_RX
*/
void Usart3_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

    //串口是挂载在 APB2 下面的外设，使能 USART3 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    //设置引脚复用器映射
    //引脚复用器映射配置，需要配置PB10，PB11 的引脚
    //PB10 复用为 USART3
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
    //PB11 复用为 USART3
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);

    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_10 | GPIO_Pin_11; //GPIOA9 与 GPIOA10
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;				//配置IO口复用功能
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz; 		//速度 50MHz
    GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP; 			//推挽复用输出
    GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_UP; 			//上拉
    //初始化
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    USART_InitStruct.USART_BaudRate 	= 57600;				//一般设置为 115200;
    USART_InitStruct.USART_WordLength 	= USART_WordLength_8b;	//字长为 8 位数据格式
    USART_InitStruct.USART_StopBits 	= USART_StopBits_1;		//一个停止位
    USART_InitStruct.USART_Parity 		= USART_Parity_No;		//无奇偶校验位
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //硬件控制流
    USART_InitStruct.USART_Mode 		= USART_Mode_Rx | USART_Mode_Tx;	//收发模式  双全工
    //初始化串口
    USART_Init(USART3, &USART_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel						= USART3_IRQn;  //NVIC通道，在stm32f4xx.h可查看通道 （可变）
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority	= 0x01;			//抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority			= 0x01;			//响应优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd					= ENABLE;		//使能
    //配置中断分组（NVIC），并使能中断。
    NVIC_Init(&NVIC_InitStruct);

    //配置串口接收中断
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART3, ENABLE);

    TIM7_Int_Init(1000 - 1, 8400 - 1);		//100ms中断
    USART3_RX_STA = 0;		//清零
    TIM_Cmd(TIM7, DISABLE); //关闭定时器7
}

//void USART1_IRQHandler(void)	//串口1中断服务程序
//{
//    uint8_t d = 0;

//    //进入中断
//    OSIntEnter();
//	//接收中断
//    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
//    {
//        //接收串口数据
//        d = USART_ReceiveData(USART1);

//        //清空串口接收中断标志位
//        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//    }

//    //退出中断
//    OSIntExit();
//}

//串口二的中断服务函数
void USART2_IRQHandler(void)
{
    OS_ERR err;
    u8 buffer;
    //进入中断
    OSIntEnter();
    //若是非空，则返回值为1，与RESET（0）判断，不相等则判断为真
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        //判断为真后，为下次中断做准备，则需要对中断的标志清零
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        /* 读取接受到的数据*/
        buffer = USART_ReceiveData(USART2);
        OSTaskQPost(&BluetoothTCB, &buffer, 1, OS_OPT_POST_FIFO, &err);
    }
    //退出中断
    OSIntExit();
}

//串口发送缓存区
__align(8) u8 USART3_TX_BUF[USART3_MAX_SEND_LEN]; //发送缓冲,最大USART2_MAX_SEND_LEN字节
#ifdef USART3_RX_EN   								//如果使能了接收   	  
//串口接收缓存区
u8 USART3_RX_BUF[USART3_MAX_RECV_LEN]; 				    //接收缓冲,最大USART2_MAX_RECV_LEN个字节

//通过判断接收连续2个字符之间的时间差不大于100ms来决定是不是一次连续的数据.
//如果2个字符接收间隔超过100ms,则认为不是1次连续数据.也就是超过100ms没有接收到任何数据,则表示此次接收完毕。
//接收到的数据状态
//[15]:0--没有接收到数据;1--接收到了一批数据.
//[14:0]:接收到的数据长度
u16 USART3_RX_STA = 0;
void USART3_IRQHandler(void)
{
    u8 res;

    //进入中断
    OSIntEnter();
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)//接收到数据
    {
        res = USART_ReceiveData(USART3);
        if((USART3_RX_STA & (1 << 15)) == 0) //接收完的一批数据,还没有被处理,则不再接收其他数据
        {
            if(USART3_RX_STA < USART3_MAX_RECV_LEN)		//还可以接收数据
            {
                TIM_SetCounter(TIM7, 0); //计数器清空
                if(USART3_RX_STA == 0)
                    TIM_Cmd(TIM7, ENABLE);  //使能定时器7
                USART3_RX_BUF[USART3_RX_STA++] = res;		//记录接收到的值
            }
            else
            {
                USART3_RX_STA |= 1 << 15;					//强制标记接收完成
            }
        }
    }
    //退出中断
    OSIntExit();
}
#endif
