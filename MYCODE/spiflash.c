#include "spiflash.h"

/*
CS 	 -- PB14(普通输出功能)
SCK  -- PB3(复用SPI1)
MOSI -- PB5(复用SPI1)
MISO -- PB4(复用SPI1)
*/
void SpiFlash_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    //1、使能SPIx和IO口时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStruct.GPIO_Pin 	= GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_14; 	//GPIOB 3 5 14
    GPIO_InitStruct.GPIO_Mode 	= GPIO_Mode_OUT;						//输出
    GPIO_InitStruct.GPIO_Speed 	= GPIO_Speed_50MHz; 					//速度 50MHz
    GPIO_InitStruct.GPIO_OType 	= GPIO_OType_PP; 						//推挽复用
    GPIO_InitStruct.GPIO_PuPd 	= GPIO_PuPd_UP; 						//上拉
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin  	= GPIO_Pin_4;	//配置CS为普通输出端口
    GPIO_InitStruct.GPIO_Mode 	= GPIO_Mode_IN;	//输入
    GPIO_InitStruct.GPIO_PuPd 	= GPIO_PuPd_UP; //上拉
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    F_CS = 1;  //不使能芯片
}

//发送数据即可接收数据
u8 Spi_Send_Byte(u8 data) //data = 0x87 1000 0111
{
    u8 i, rx_data = 0x00;

    SCK = 0;

    for(i = 0; i < 8; i++)
    {
        //准备数据
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
        //接受数据
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

    F_CS = 0;  //使能芯片

    //发送读ID命令
    Spi_Send_Byte(0x90);

    //发地址
    Spi_Send_Byte(0x00);
    Spi_Send_Byte(0x00);
    Spi_Send_Byte(0x00);

    id |= Spi_Send_Byte(0xFF) << 8;	//生产商ID
    id |= Spi_Send_Byte(0xFF);		//设备ID

    F_CS = 1;  //不使能芯片
    return id;
}

//写使能
void Write_Enable(void)
{
    F_CS = 0;  //使能芯片
    //发送写使能命令
    Spi_Send_Byte(0x06);

    F_CS = 1;  //不使能芯片
}

//读状态寄存器1
u8 W25Q128_Read_Status1(void)
{
    u8 status1;
    F_CS = 0;  //使能芯片

    //发送命令
    Spi_Send_Byte(0x05);

    //发送任意字节
    status1 = Spi_Send_Byte(0xFF);

    F_CS = 1;  //不使能芯片

    return status1;
}

void W25Q128_Erase_Sector(u32 addr)
{
    //写使能
    Write_Enable();

    F_CS = 0;  //使能芯片

    //发送擦除命令
    Spi_Send_Byte(0x20);

    //拆分地址发送
    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    F_CS = 1;  //使能芯片

    //判断是否擦除完成
    while(1)
    {
        //判断BUSY位是否为0
        if((W25Q128_Read_Status1() & 0x01) == 0)
            break;
    }
}

void W25Q128_Writer_Data(u32 addr, u8 *buf, u32 len)
{
    Write_Enable();
    //芯片使能
    F_CS = 0;
    //写页命令
    Spi_Send_Byte(0x02);

    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    //写数据
    while(len--)
    {
        Spi_Send_Byte(*buf);
        buf++;
    }

    F_CS = 1;

    //判断是否写完成
    while(1)
    {
        if((W25Q128_Read_Status1() & 0x01) == 0)
            break;
    }
}

void W25Q128_Writer_Byte(u32 addr, u8 buf)
{
    Write_Enable();
    //芯片使能
    F_CS = 0;
    //写页命令
    Spi_Send_Byte(0x02);

    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    //写数据
	Spi_Send_Byte(buf);

    F_CS = 1;

    //判断是否写完成
    while(1)
    {
        if((W25Q128_Read_Status1() & 0x01) == 0)
            break;
    }
}

void W25Q128_Read_Data(u32 addr, u8 *buf, u32 len)
{
    //芯片使能
    F_CS = 0;

    Spi_Send_Byte(0x03);

    //读数据地址
    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    //读数据
    while(len--)
    {
        //发送任意数据
        *buf = Spi_Send_Byte(0xff);
        buf++;
    }

    F_CS = 1;
}

void W25Q128_Read_Byte(u32 addr, u8 *buf)
{
    //芯片使能
    F_CS = 0;

    Spi_Send_Byte(0x03);

    //读数据地址
    Spi_Send_Byte((addr >> 16) & 0xFF);
    Spi_Send_Byte((addr >> 8) & 0xFF);
    Spi_Send_Byte(addr & 0xFF);

    //读数据
	//发送任意数据
    *buf = Spi_Send_Byte(0xff);

    F_CS = 1;
}
