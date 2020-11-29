#ifndef __RFID_H
#define __RFID_H

#include "stm32f4xx.h"
#include <stdbool.h>

void Show_RFID_Management(void);
void Show_RFID_Input(void);
void Show_Delete_Card(void);
int Wait_RFID_Card(void);
void RFID_Write_Flash(u8 *rfid_buf, u8 rfid_number);
int Input_RFID_Card(void);
bool Card_Already_Exist(u8 *rfid_buf, u8 *card_number);
void Delete_Exist_Card(void);
void Delete_All_Card(void);

#endif
