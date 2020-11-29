#include "OLED_I2C.h"
#include "delay.h"
#include "oledfont.h"

unsigned char GDDRAM[8][128] = {0};//虚拟缓存
unsigned char gddram_temp[8][128] = {0};//虚拟显存缓冲区

extern OS_MUTEX mutex_oled;     //互斥量的对象
/*
引脚说明：
PE8 -- SCL
PE10 -- SDA
PD1 -- VCC
PD15 -- GND
*/
void I2C_Configuration(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);//使能GPIO E组时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);//使能GPIO D组时钟

    GPIO_InitStruct.GPIO_Pin    = GPIO_Pin_1 | GPIO_Pin_15; //引脚1和15
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;            //输出模式
    GPIO_InitStruct.GPIO_OType  = GPIO_OType_PP;            //推挽
    GPIO_InitStruct.GPIO_Speed  = GPIO_Speed_50MHz;         //快速
    GPIO_InitStruct.GPIO_PuPd   = GPIO_PuPd_UP;             //上拉
    GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin    = GPIO_Pin_8 | GPIO_Pin_10; //引脚8和10
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;            //输出模式
    GPIO_InitStruct.GPIO_OType  = GPIO_OType_PP;            //推挽
    GPIO_InitStruct.GPIO_Speed  = GPIO_Speed_50MHz;         //快速
    GPIO_InitStruct.GPIO_PuPd   = GPIO_PuPd_UP;             //上拉
    GPIO_Init(GPIOE, &GPIO_InitStruct);

    //给OLED供电
    PDout(1) = 1;
    PDout(15) = 0;

    //总线空闲
    SCL = 1;
    SDA_OUT = 1;
}

//设置数据线的IO工作模式
void OLED_Sda_Mode(GPIOMode_TypeDef Mode)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    GPIO_InitStruct.GPIO_Pin    = GPIO_Pin_10;          //引脚10
    GPIO_InitStruct.GPIO_Mode   = Mode;                 //工作模式
    GPIO_InitStruct.GPIO_OType  = GPIO_OType_PP;        //输出推挽
    GPIO_InitStruct.GPIO_Speed  = GPIO_Speed_50MHz;     //快速
    GPIO_InitStruct.GPIO_PuPd   = GPIO_PuPd_UP;         //上拉
    GPIO_Init(GPIOE, &GPIO_InitStruct);
}

//开始信号
void OLED_Start(void)
{
    OLED_Sda_Mode(GPIO_Mode_OUT);

    SCL = 1;
    SDA_OUT = 1;
    delay_us(5);

    SDA_OUT = 0;
    delay_us(5);
    SCL = 0;
}

//停止信号
void OLED_Stop(void)
{
    OLED_Sda_Mode(GPIO_Mode_OUT);

    SCL = 0;
    SDA_OUT = 0;
    delay_us(5);

    SCL = 1;
    delay_us(5);
    SDA_OUT = 1;
}

//发送一个字节
void OLED_Send_Byte(u8 data)
{
    u8 i;

    OLED_Sda_Mode(GPIO_Mode_OUT);

    SCL = 0;
    for(i = 0; i < 8; i++)
    {
        if(data & (1 << (7 - i)))
        {
            SDA_OUT = 1;
        }
        else
        {
            SDA_OUT = 0;
        }
        delay_us(5);
        SCL = 1;
        delay_us(5);
        SCL = 0;
    }
}

//接收应答信号
u8 OLED_Recv_Ack(void)
{
    u8 ack = 0;

    OLED_Sda_Mode(GPIO_Mode_IN);

    SCL = 0;
    delay_us(5);
    SCL = 1;
    delay_us(5);
    if(SDA_IN == 1)
    {
        ack = 1;
    }
    if(SDA_IN == 0)
    {
        ack = 0;
    }

    SCL = 0;
    return ack;
}

void I2C_WriteByte(uint8_t addr, uint8_t data)
{
    u8 ack;

    OLED_Start();

    OLED_Send_Byte(0x78);
    ack = OLED_Recv_Ack();
    if(ack == 1)
    {
        printf("ack failure1\n");
        OLED_Stop();
        return;
    }

    OLED_Send_Byte(addr);
    ack = OLED_Recv_Ack();
    if(ack == 1)
    {
        printf("ack failure2\n");
        OLED_Stop();
        return;
    }

    OLED_Send_Byte(data);
    ack = OLED_Recv_Ack();
    if(ack == 1)
    {
        printf("ack failure3\n");
        OLED_Stop();
        return;
    }

    OLED_Stop();
}

void WriteCmd(unsigned char I2C_Command)//写命令
{
    I2C_WriteByte(0x00, I2C_Command);
}

void WriteDat(unsigned char I2C_Data)//写数据
{
    I2C_WriteByte(0x40, I2C_Data);
}

void OLED_Init(void)
{
    delay_ms(100); //这里的延时很重要

    WriteCmd(0xAE); //display off
    WriteCmd(0x20); //设置内存寻址模式
    WriteCmd(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
    WriteCmd(0xb0); //Set Page Start Address for Page Addressing Mode,0-7
    WriteCmd(0xc8); //Set COM Output Scan Direction
    WriteCmd(0x00); //---set low column address
    WriteCmd(0x10); //---set high column address
    WriteCmd(0x40); //--set start line address
    WriteCmd(0x81); //--set contrast control register
    WriteCmd(0xff); //亮度调节 0x00~0xff
    WriteCmd(0xa1); //--set segment re-map 0 to 127
    WriteCmd(0xa6); //--set normal display
    WriteCmd(0xa8); //--set multiplex ratio(1 to 64)
    WriteCmd(0x3F); //
    WriteCmd(0xa4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    WriteCmd(0xd3); //-set display offset
    WriteCmd(0x00); //-not offset
    WriteCmd(0xd5); //--set display clock divide ratio/oscillator frequency
    WriteCmd(0xf0); //--set divide ratio
    WriteCmd(0xd9); //--set pre-charge period
    WriteCmd(0x22); //
    WriteCmd(0xda); //--set com pins hardware configuration
    WriteCmd(0x12);
    WriteCmd(0xdb); //--set vcomh
    WriteCmd(0x20); //0x20,0.77xVcc
    WriteCmd(0x8d); //--set DC-DC enable
    WriteCmd(0x14); //
    WriteCmd(0xaf); //--turn on oled panel
}

void OLED_SetPos(unsigned char x, unsigned char y) //设置起始点坐标
{
    WriteCmd(0xb0 + y); //相当于设置从哪一行开始
    //设置从哪一列开始
    WriteCmd(((x & 0xf0) >> 4) | 0x10); //设置列起始地址高位
    WriteCmd((x & 0x0f) | 0x00); //设置列起始地址低位，从第0列开始
}

void OLED_Fill(unsigned char fill_Data)//全屏填充
{
    unsigned char m, n;
    for(m = 0; m < 8; m++)
    {
        WriteCmd(0xb0 + m);     //page0-page1
        WriteCmd(0x00);     //low column start address
        WriteCmd(0x10);     //high column start address
        for(n = 0; n < 128; n++)
        {
            WriteDat(fill_Data);
        }
    }
}

//功能：将缓存中的数据显示出来
void OLED_Refresh_Gram(unsigned char gddram[][128])
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        WriteCmd(0xB0 + i); //设置页地址（0~7）每页中的GDDRAM列地址是自动增加的。
        WriteCmd(0x00);     //设置显示开始位置—列低地址
        WriteCmd(0x10);     //设置显示开始位置—列高地址
        for(n = 0; n < 128; n++)
        {
            WriteDat(gddram[i][n]);
        }
    }
}

//功能：拷贝数据。主要是实现类似锁存功能
void Gddram_Copy(unsigned char dest[][128], unsigned char src[][128])
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        for(n = 0; n < 128; n++)
            dest[i][n] = src[i][n];
    }
}

void OLED_CLS(void)//清屏
{
    OLED_Fill(0x00);
}

//--------------------------------------------------------------
// Prototype      : void OLED_ON(void)
// Calls          :
// Parameters     : none
// Description    : 将OLED从休眠中唤醒
//--------------------------------------------------------------
void OLED_ON(void)
{
    WriteCmd(0X8D);  //设置电荷泵
    WriteCmd(0X14);  //开启电荷泵
    WriteCmd(0XAF);  //OLED唤醒
}

//--------------------------------------------------------------
// Prototype      : void OLED_OFF(void)
// Calls          :
// Parameters     : none
// Description    : 让OLED休眠 -- 休眠模式下,OLED功耗不到10uA
//--------------------------------------------------------------
void OLED_OFF(void)
{
    WriteCmd(0X8D);  //设置电荷泵
    WriteCmd(0X10);  //关闭电荷泵
    WriteCmd(0XAE);  //OLED休眠
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
// Calls          :
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); ch[] -- 要显示的字符串; TextSize -- 字符大小(1:6*8 ; 2:8*16)
// Description    : 显示oledfont.h中的ASCII字符,有6*8和8*16可选择
//--------------------------------------------------------------
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
{
    unsigned char c = 0, i = 0, j = 0;
    switch(TextSize)
    {
    case 1:
    {
        while(ch[j] != '\0')
        {
            c = ch[j] - 32;//ascii码值-32 算出对应的索引
            if(x > 126)
            {
                x = 0;
                y++;
            }
            OLED_SetPos(x, y);
            for(i = 0; i < 6; i++)
            {
                GDDRAM[y][x + i] = F6x8[c][i];
                WriteDat(F6x8[c][i]);
            }
            x += 6;
            j++;
        }
    }
    break;
    case 2:
    {
        while(ch[j] != '\0')
        {
            c = ch[j] - 32;
            if(x > 120)
            {
                x = 0;
                y++;
            }
            OLED_SetPos(x, y);
            for(i = 0; i < 8; i++)
            {
                GDDRAM[y][x + i] = F8X16[c * 16 + i];
                WriteDat(F8X16[c * 16 + i]);
            }
            OLED_SetPos(x, y + 1);
            for(i = 0; i < 8; i++)
            {
                GDDRAM[y + 1][x + i] = F8X16[c * 16 + i + 8];
                WriteDat(F8X16[c * 16 + i + 8]);
            }
            x += 8;
            j++;
        }
    }
    break;
    }
}

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示
//size:选择字体 16/12
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    unsigned char c = 0, i = 0;
    c = chr - ' '; //得到偏移后的值
    if(x > 127 - 1)
    {
        x = 0;
        y = y + 2;
    }
    if(Char_Size == 16)
    {
        OLED_SetPos(x, y);
        for(i = 0; i < 8; i++)
        {
            GDDRAM[y][x + i] = F8X16[c * 16 + i];
            WriteDat(F8X16[c * 16 + i]);
        }
        OLED_SetPos(x, y + 1);
        for(i = 0; i < 8; i++)
        {
            GDDRAM[y + 1][x + i] = F8X16[c * 16 + i + 8];
            WriteDat(F8X16[c * 16 + i + 8]);
        }
    }
    else
    {
        OLED_SetPos(x, y);
        for(i = 0; i < 6; i++)
        {
            GDDRAM[y][x + i] = F6x8[c][i];
            WriteDat(F6x8[c][i]);
        }
    }
}

//功能：查找字符串中的汉字在F16x16中的索引，并将结果保存至result中
//参数：chinese：源字符串
//      result：索引的结果
//      len：源字符串中汉字的个数
void Chinese_Search(char *chinese, unsigned char *result, unsigned char len)
{
    unsigned char i, j;
    unsigned char k = 0;

    for(i = 0; i < len; i++)
    {
        for(j = 1; j < FONTSIZE; j++)
        {
            if(chinese[i * 2] == F16x16[j].index[0])
            {
                if(chinese[i * 2 + 1] == F16x16[j].index[1])
                {
                    result[k++] = j;
                    break;
                }
            }
        }
        if(j == FONTSIZE)
            result[k++] = 0;
    }
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowCN(unsigned char x, unsigned char y, char *chinese, unsigned char len)
// Calls          :
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); chinese:要显示的汉字 len:要显示的汉字的个数（汉字库在codetab.h中定义）
// Description    : 显示oledfont.h中的汉字,16*16点阵
//--------------------------------------------------------------
void OLED_ShowCN(unsigned char x, unsigned char y, char *chinese, unsigned char len)
{
    unsigned char i, j;
    unsigned char wm = 0;
    unsigned char result[len];

    Chinese_Search(chinese, result, len);

    for(i = 0; i < len; i++)
    {
        OLED_SetPos(x + i * 16, y);
        for(wm = 0; wm < 16; wm++)
        {
            GDDRAM[y][x + i * 16 + wm] = F16x16[result[i]].font[wm];
            WriteDat(F16x16[result[i]].font[wm]);
        }
        OLED_SetPos(x + i * 16, y + 1);
        for(j = 0; wm < 32; wm++, j++)
        {
            GDDRAM[y + 1][x + i * 16 + j] = F16x16[result[i]].font[wm];
            WriteDat(F16x16[result[i]].font[wm]);
        }
    }
}

//--------------------------------------------------------------
// Prototype      : void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
// Calls          :
// Parameters     : x0,y0 -- 起始点坐标(x0:0~127, y0:0~7); x1,y1 -- 起点对角线(结束点)的坐标(x1:1~128,y1:1~8)
// Description    : 显示BMP位图
//--------------------------------------------------------------
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[])
{
    unsigned int j = 0;
    unsigned char x, y;

    if(y1 % 8 == 0)
        y = y1 / 8;
    else
        y = y1 / 8 + 1;

    for(y = y0; y < y1; y++)
    {
        OLED_SetPos(x0, y);
        for(x = x0; x < x1; x++)
        {
            GDDRAM[y][x] = BMP[j];
            WriteDat(BMP[j++]);
        }
    }
}

void Unlock_Rest_OLED_Fill(unsigned char fill_Data)//部分填充
{
    unsigned char m, n;
    for(m = 0; m < 8; m++)
    {
        WriteCmd(0xb0 + m);     //page0-page1

        if(m < 5)//前5列（图片占了5列）
        {
            WriteCmd(0x0A);     //10
            WriteCmd(0x12);     //不超过48
            for(n = 42; n < 128; n++)//10+32=42，填充除了开锁图片以外的部分
                WriteDat(fill_Data);
        }
        else
        {
            WriteCmd(0x00);     //low column start address
            WriteCmd(0x10);     //high column start address
            for(n = 0; n < 128; n++)
                WriteDat(fill_Data);
        }
    }
}

void Unlock_OLED_Fill(unsigned char fill_Data)//部分填充
{
    unsigned char m, n;
    for(m = 0; m < 5; m++)//前5列（图片占了5列）
    {
        WriteCmd(0xb0 + m); //page0-page1
        WriteCmd(0x00);     //10
        WriteCmd(0x10);     //不超过48
        for(n = 0; n < 42; n++)//10+32=42，填充开锁图片的部分
            WriteDat(fill_Data);
    }
}

void Lock_Rest_OLED_Fill(unsigned char fill_Data)//部分填充
{
    unsigned char m, n;
    for(m = 0; m < 8; m++)
    {
        WriteCmd(0xb0 + m);     //page0-page1

        if(m < 5)//前5列（图片占了5列）
        {
            WriteCmd(0x09);     //9
            WriteCmd(0x11);     //不超过32
            for(n = 25; n < 128; n++)//16+9=25，填充除了上锁图片以外的部分
                WriteDat(fill_Data);
        }
        else
        {
            WriteCmd(0x00);     //low column start address
            WriteCmd(0x10);     //high column start address
            for(n = 0; n < 128; n++)
                WriteDat(fill_Data);
        }
    }
}

void Show_Previous_Interface(void)
{
    unsigned char x, y;
    //将坐上角原本显示图片的位置的数据写入虚拟显存缓冲区中
    for(y = 0; y < 6; y++)
    {
        for(x = 0; x < 42; x++)
        {
            gddram_temp[y][x] = GDDRAM[y][x];
        }
    }
    memset(GDDRAM, 0, sizeof(GDDRAM)); //清空虚拟缓存

    Gddram_Copy(GDDRAM, gddram_temp);//将之前界面的数据放回去
    OLED_Refresh_Gram(GDDRAM);//重新显示之前的界面
}

void My_OLED_CLR(void)
{
    memset(GDDRAM, 0, sizeof(GDDRAM)); //清空虚拟缓存
    Unlock_Rest_OLED_Fill(0x00);
    delay_ms(2);
}

void Show_Virtual_Lock(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    Unlock_OLED_Fill(0x00);
    OLED_DrawBMP(0, 0, 24, 5, (unsigned char *)BMP1);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
    delay_ms(2);
}

//显示主界面
void Show_Main(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();

    OLED_ShowCN(47, 0, "智能门锁", 4);

    OLED_ShowStr(42, 4, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 4, "功能界面", 4);

    OLED_ShowStr(42, 6, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 6, "密码解锁", 4);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}

//显示功能界面
void Show_Fun_Interface(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "密码设定", 4);

    OLED_ShowStr(42, 2, (unsigned char *)"2.RFID", 2);
    OLED_ShowCN(90, 2, "管理", 2);

    OLED_ShowStr(42, 4, (unsigned char *)"3.", 2);
    OLED_ShowCN(58, 4, "指纹管理", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "返回", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}
