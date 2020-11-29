#ifndef __SPIFLASH_H
#define __SPIFLASH_H

#include "stm32f4xx.h"
#include "sys.h"
#include "delay.h"

#define F_CS 	PBout(14)
#define SCK     PBout(3)
#define MISO    PBin(4)
#define MOSI    PBout(5)

#define PASSWORD_ADDR 0x010000
#define RFID_CARD_ADDR 0x020000
#define FINGERPRINT_ADDR 0x030000
#define RFID_NUMBER_ADDR	0X040000	//存放rfid卡数量
#define FINGERPRINT_LEN_ADDR 0X050000//指纹数据长度

void SpiFlash_Init(void);
u16 W25q128_Id(void);
void W25Q128_Erase_Sector(u32 addr);
void W25Q128_Writer_Data(u32 addr, u8 *buf, u32 len);
void W25Q128_Read_Data(u32 addr, u8 *buf, u32 len);
void W25Q128_Writer_Byte(u32 addr, u8 buf);
void W25Q128_Read_Byte(u32 addr, u8 *buf);

#endif
