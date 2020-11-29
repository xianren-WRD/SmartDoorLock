#ifndef __OLED_I2C_H
#define	__OLED_I2C_H

#include "stm32f4xx.h"
#include "sys.h"
#include "stdio.h"

#define OLED_ADDRESS	0x78 //通过调整0R电阻,屏可以0x78和0x7A两个地址 -- 默认0x78

#define SCL  		PEout(8)
#define SDA_OUT  	PEout(10)
#define SDA_IN  	PEin(10)

void I2C_Configuration(void);
void I2C_WriteByte(uint8_t addr, uint8_t data);
void WriteCmd(unsigned char I2C_Command);
void WriteDat(unsigned char I2C_Data);
void OLED_Init(void);
void OLED_SetPos(unsigned char x, unsigned char y);
void OLED_Fill(unsigned char fill_Data);
void OLED_CLS(void);
void OLED_ON(void);
void OLED_OFF(void);
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize);
void OLED_ShowCN(unsigned char x, unsigned char y, char *chinese, unsigned char len);
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);

void Chinese_Search(char *chinese, unsigned char *result, unsigned char len);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size);
void My_OLED_CLR(void);
void Show_Main(void);
void Show_Fun_Interface(void);
void OLED_Refresh_Gram(unsigned char gddram[][128]);
void Gddram_Copy(unsigned char dest[][128], unsigned char src[][128]);
void Unlock_OLED_Fill(unsigned char fill_Data);//部分填充
void Unlock_Rest_OLED_Fill(unsigned char fill_Data);//部分填充
void Show_Virtual_Lock(void);
void Show_Previous_Interface(void);

#endif
