#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

#define USART3_MAX_RECV_LEN		400					//�����ջ����ֽ���
#define USART3_MAX_SEND_LEN		400					//����ͻ����ֽ���
#define USART3_RX_EN 			1					//0,������;1,����

extern u8  USART3_RX_BUF[USART3_MAX_RECV_LEN]; 		//���ջ���,���USART2_MAX_RECV_LEN�ֽ�
extern u8  USART3_TX_BUF[USART3_MAX_SEND_LEN]; 		//���ͻ���,���USART2_MAX_SEND_LEN�ֽ�
extern u16 USART3_RX_STA;   				 		//��������״̬

void Usart1_Init(uint32_t baud);
void Usart2_Init(void);
void Usart3_Init(void);

#endif


