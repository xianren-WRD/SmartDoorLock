#include "spiflash.h"

/*
CS 	 -- PB14(��ͨ�������)
SCK  -- PB3(����SPI1)
MOSI -- PB5(����SPI1)
MISO -- PB4(����SPI1)
*/
void SpiFlash_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    //1��ʹ��SPIx��IO��ʱ��
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStruct.GPIO_Pin 	= GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_14; 	//GPIOB 3 5 14
    GPIO_InitStruct.GPIO_Mode 	= GPIO_Mode_OUT;						//���
    GPIO_InitStruct.GPIO_Speed 	= GPIO_Speed_50MHz; 					//�ٶ� 50MHz
    GPIO_InitStruct.GPIO_OType 	= GPIO_OType_PP; 						//���츴��
    GPIO_InitStruct.GPIO_PuPd 	= GPIO_PuPd_UP; 						//����
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin  	= GPIO_Pin_4;	//����CSΪ��ͨ����˿�
    GPIO_InitStruct.GPIO_Mode 	= GPIO_Mode_IN;	//����
    GPIO_InitStruct.GPIO_PuPd 	= GPIO_PuPd_UP; //����
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    F_CS = 1;  //��ʹ��оƬ
}

//�������ݼ��ɽ�������
u8 Spi_Send_Byte(u8 data) //data = 0x87 1000 0111
{
    u8 i, rx_data = 0x00;

    SCK = 0;

    for(i = 0; i < 8; i++)
    {
        //׼������
        if(data & (1 << (7 - i)))
        {
            MOSI = 1;
        }
        else
        {
            MOSI = 0;
        }
        delay_us(2);
        SCK = 1;
        delay_us(2);
        //��������
        if(MISO == 1)
        {
            rx_data |= (1 << (7 - i));
        }

        SCK = 0;
    }

    return rx_data;
}

u16 W25q128_Id(void)
{
    u16 id;

    F_CS = 0;  //ʹ��оƬ

    //���Ͷ�ID����
    Spi_Send_Byte(0x90);

    //����ַ
    Spi_Send_Byte(0x00);
    Spi_Send_Byte(0x00);
    Spi_Send_Byte(0x00);

    id |= Spi_Send_Byte(0xFF) << 8;	//������ID
    id |= Spi_Send_Byte(0xFF);		//�豸ID

    F_CS = 1;  //��ʹ��оƬ
    return id;
}

//дʹ��
void Write_Enable(void)
{
    F_CS = 0;  //ʹ��оƬ
    //����дʹ������
    Spi_Send_Byte(0x06);

    F_CS = 1;  //��ʹ��оƬ
}

//��״̬�Ĵ���1
u8 W25Q128_Read_Status1(void)
{
    u8 status1;
    F_CS = 0;  //ʹ��оƬ

    //��������
    Spi_Send_Byte(0x05);

    //���������ֽ�
    status1 = Spi_Send_Byte(0xFF);

    F_CS = 1;  //��ʹ��оƬ

    return status1;
}

void W25Q128_Erase_Sector(u32 addr)
{
    //дʹ��
    Write_Enable();

    F_CS = 0;  //ʹ��оƬ

    //���Ͳ�������
    Spi_Send_Byte(0x20);

    //��ֵ�ַ����
    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    F_CS = 1;  //ʹ��оƬ

    //�ж��Ƿ�������
    while(1)
    {
        //�ж�BUSYλ�Ƿ�Ϊ0
        if((W25Q128_Read_Status1() & 0x01) == 0)
            break;
    }
}

void W25Q128_Writer_Data(u32 addr, u8 *buf, u32 len)
{
    Write_Enable();
    //оƬʹ��
    F_CS = 0;
    //дҳ����
    Spi_Send_Byte(0x02);

    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    //д����
    while(len--)
    {
        Spi_Send_Byte(*buf);
        buf++;
    }

    F_CS = 1;

    //�ж��Ƿ�д���
    while(1)
    {
        if((W25Q128_Read_Status1() & 0x01) == 0)
            break;
    }
}

void W25Q128_Writer_Byte(u32 addr, u8 buf)
{
    Write_Enable();
    //оƬʹ��
    F_CS = 0;
    //дҳ����
    Spi_Send_Byte(0x02);

    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    //д����
	Spi_Send_Byte(buf);

    F_CS = 1;

    //�ж��Ƿ�д���
    while(1)
    {
        if((W25Q128_Read_Status1() & 0x01) == 0)
            break;
    }
}

void W25Q128_Read_Data(u32 addr, u8 *buf, u32 len)
{
    //оƬʹ��
    F_CS = 0;

    Spi_Send_Byte(0x03);

    //�����ݵ�ַ
    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    //������
    while(len--)
    {
        //������������
        *buf = Spi_Send_Byte(0xff);
        buf++;
    }

    F_CS = 1;
}

void W25Q128_Read_Byte(u32 addr, u8 *buf)
{
    //оƬʹ��
    F_CS = 0;

    Spi_Send_Byte(0x03);

    //�����ݵ�ַ
    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    //������
	//������������
    *buf = Spi_Send_Byte(0xff);

    F_CS = 1;
}
