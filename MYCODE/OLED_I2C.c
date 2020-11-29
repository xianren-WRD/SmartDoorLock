#include "OLED_I2C.h"
#include "delay.h"
#include "oledfont.h"

unsigned char GDDRAM[8][128] = {0};//���⻺��
unsigned char gddram_temp[8][128] = {0};//�����Դ滺����

extern OS_MUTEX mutex_oled;     //�������Ķ���
/*
����˵����
PE8 -- SCL
PE10 -- SDA
PD1 -- VCC
PD15 -- GND
*/
void I2C_Configuration(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);//ʹ��GPIO E��ʱ��
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);//ʹ��GPIO D��ʱ��

    GPIO_InitStruct.GPIO_Pin    = GPIO_Pin_1 | GPIO_Pin_15; //����1��15
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;            //���ģʽ
    GPIO_InitStruct.GPIO_OType  = GPIO_OType_PP;            //����
    GPIO_InitStruct.GPIO_Speed  = GPIO_Speed_50MHz;         //����
    GPIO_InitStruct.GPIO_PuPd   = GPIO_PuPd_UP;             //����
    GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin    = GPIO_Pin_8 | GPIO_Pin_10; //����8��10
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;            //���ģʽ
    GPIO_InitStruct.GPIO_OType  = GPIO_OType_PP;            //����
    GPIO_InitStruct.GPIO_Speed  = GPIO_Speed_50MHz;         //����
    GPIO_InitStruct.GPIO_PuPd   = GPIO_PuPd_UP;             //����
    GPIO_Init(GPIOE, &GPIO_InitStruct);

    //��OLED����
    PDout(1) = 1;
    PDout(15) = 0;

    //���߿���
    SCL = 1;
    SDA_OUT = 1;
}

//���������ߵ�IO����ģʽ
void OLED_Sda_Mode(GPIOMode_TypeDef Mode)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    GPIO_InitStruct.GPIO_Pin    = GPIO_Pin_10;          //����10
    GPIO_InitStruct.GPIO_Mode   = Mode;                 //����ģʽ
    GPIO_InitStruct.GPIO_OType  = GPIO_OType_PP;        //�������
    GPIO_InitStruct.GPIO_Speed  = GPIO_Speed_50MHz;     //����
    GPIO_InitStruct.GPIO_PuPd   = GPIO_PuPd_UP;         //����
    GPIO_Init(GPIOE, &GPIO_InitStruct);
}

//��ʼ�ź�
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

//ֹͣ�ź�
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

//����һ���ֽ�
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

//����Ӧ���ź�
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

void WriteCmd(unsigned char I2C_Command)//д����
{
    I2C_WriteByte(0x00, I2C_Command);
}

void WriteDat(unsigned char I2C_Data)//д����
{
    I2C_WriteByte(0x40, I2C_Data);
}

void OLED_Init(void)
{
    delay_ms(100); //�������ʱ����Ҫ

    WriteCmd(0xAE); //display off
    WriteCmd(0x20); //�����ڴ�Ѱַģʽ
    WriteCmd(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
    WriteCmd(0xb0); //Set Page Start Address for Page Addressing Mode,0-7
    WriteCmd(0xc8); //Set COM Output Scan Direction
    WriteCmd(0x00); //---set low column address
    WriteCmd(0x10); //---set high column address
    WriteCmd(0x40); //--set start line address
    WriteCmd(0x81); //--set contrast control register
    WriteCmd(0xff); //���ȵ��� 0x00~0xff
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

void OLED_SetPos(unsigned char x, unsigned char y) //������ʼ������
{
    WriteCmd(0xb0 + y); //�൱�����ô���һ�п�ʼ
    //���ô���һ�п�ʼ
    WriteCmd(((x & 0xf0) >> 4) | 0x10); //��������ʼ��ַ��λ
    WriteCmd((x & 0x0f) | 0x00); //��������ʼ��ַ��λ���ӵ�0�п�ʼ
}

void OLED_Fill(unsigned char fill_Data)//ȫ�����
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

//���ܣ��������е�������ʾ����
void OLED_Refresh_Gram(unsigned char gddram[][128])
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        WriteCmd(0xB0 + i); //����ҳ��ַ��0~7��ÿҳ�е�GDDRAM�е�ַ���Զ����ӵġ�
        WriteCmd(0x00);     //������ʾ��ʼλ�á��е͵�ַ
        WriteCmd(0x10);     //������ʾ��ʼλ�á��иߵ�ַ
        for(n = 0; n < 128; n++)
        {
            WriteDat(gddram[i][n]);
        }
    }
}

//���ܣ��������ݡ���Ҫ��ʵ���������湦��
void Gddram_Copy(unsigned char dest[][128], unsigned char src[][128])
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        for(n = 0; n < 128; n++)
            dest[i][n] = src[i][n];
    }
}

void OLED_CLS(void)//����
{
    OLED_Fill(0x00);
}

//--------------------------------------------------------------
// Prototype      : void OLED_ON(void)
// Calls          :
// Parameters     : none
// Description    : ��OLED�������л���
//--------------------------------------------------------------
void OLED_ON(void)
{
    WriteCmd(0X8D);  //���õ�ɱ�
    WriteCmd(0X14);  //������ɱ�
    WriteCmd(0XAF);  //OLED����
}

//--------------------------------------------------------------
// Prototype      : void OLED_OFF(void)
// Calls          :
// Parameters     : none
// Description    : ��OLED���� -- ����ģʽ��,OLED���Ĳ���10uA
//--------------------------------------------------------------
void OLED_OFF(void)
{
    WriteCmd(0X8D);  //���õ�ɱ�
    WriteCmd(0X10);  //�رյ�ɱ�
    WriteCmd(0XAE);  //OLED����
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
// Calls          :
// Parameters     : x,y -- ��ʼ������(x:0~127, y:0~7); ch[] -- Ҫ��ʾ���ַ���; TextSize -- �ַ���С(1:6*8 ; 2:8*16)
// Description    : ��ʾoledfont.h�е�ASCII�ַ�,��6*8��8*16��ѡ��
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
            c = ch[j] - 32;//ascii��ֵ-32 �����Ӧ������
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

//��ָ��λ����ʾһ���ַ�,���������ַ�
//x:0~127
//y:0~63
//mode:0,������ʾ;1,������ʾ
//size:ѡ������ 16/12
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    unsigned char c = 0, i = 0;
    c = chr - ' '; //�õ�ƫ�ƺ��ֵ
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

//���ܣ������ַ����еĺ�����F16x16�е��������������������result��
//������chinese��Դ�ַ���
//      result�������Ľ��
//      len��Դ�ַ����к��ֵĸ���
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
// Parameters     : x,y -- ��ʼ������(x:0~127, y:0~7); chinese:Ҫ��ʾ�ĺ��� len:Ҫ��ʾ�ĺ��ֵĸ��������ֿ���codetab.h�ж��壩
// Description    : ��ʾoledfont.h�еĺ���,16*16����
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
// Parameters     : x0,y0 -- ��ʼ������(x0:0~127, y0:0~7); x1,y1 -- ���Խ���(������)������(x1:1~128,y1:1~8)
// Description    : ��ʾBMPλͼ
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

void Unlock_Rest_OLED_Fill(unsigned char fill_Data)//�������
{
    unsigned char m, n;
    for(m = 0; m < 8; m++)
    {
        WriteCmd(0xb0 + m);     //page0-page1

        if(m < 5)//ǰ5�У�ͼƬռ��5�У�
        {
            WriteCmd(0x0A);     //10
            WriteCmd(0x12);     //������48
            for(n = 42; n < 128; n++)//10+32=42�������˿���ͼƬ����Ĳ���
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

void Unlock_OLED_Fill(unsigned char fill_Data)//�������
{
    unsigned char m, n;
    for(m = 0; m < 5; m++)//ǰ5�У�ͼƬռ��5�У�
    {
        WriteCmd(0xb0 + m); //page0-page1
        WriteCmd(0x00);     //10
        WriteCmd(0x10);     //������48
        for(n = 0; n < 42; n++)//10+32=42����俪��ͼƬ�Ĳ���
            WriteDat(fill_Data);
    }
}

void Lock_Rest_OLED_Fill(unsigned char fill_Data)//�������
{
    unsigned char m, n;
    for(m = 0; m < 8; m++)
    {
        WriteCmd(0xb0 + m);     //page0-page1

        if(m < 5)//ǰ5�У�ͼƬռ��5�У�
        {
            WriteCmd(0x09);     //9
            WriteCmd(0x11);     //������32
            for(n = 25; n < 128; n++)//16+9=25������������ͼƬ����Ĳ���
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
    //�����Ͻ�ԭ����ʾͼƬ��λ�õ�����д�������Դ滺������
    for(y = 0; y < 6; y++)
    {
        for(x = 0; x < 42; x++)
        {
            gddram_temp[y][x] = GDDRAM[y][x];
        }
    }
    memset(GDDRAM, 0, sizeof(GDDRAM)); //������⻺��

    Gddram_Copy(GDDRAM, gddram_temp);//��֮ǰ��������ݷŻ�ȥ
    OLED_Refresh_Gram(GDDRAM);//������ʾ֮ǰ�Ľ���
}

void My_OLED_CLR(void)
{
    memset(GDDRAM, 0, sizeof(GDDRAM)); //������⻺��
    Unlock_Rest_OLED_Fill(0x00);
    delay_ms(2);
}

void Show_Virtual_Lock(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    Unlock_OLED_Fill(0x00);
    OLED_DrawBMP(0, 0, 24, 5, (unsigned char *)BMP1);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
    delay_ms(2);
}

//��ʾ������
void Show_Main(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();

    OLED_ShowCN(47, 0, "��������", 4);

    OLED_ShowStr(42, 4, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 4, "���ܽ���", 4);

    OLED_ShowStr(42, 6, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 6, "�������", 4);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}

//��ʾ���ܽ���
void Show_Fun_Interface(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "�����趨", 4);

    OLED_ShowStr(42, 2, (unsigned char *)"2.RFID", 2);
    OLED_ShowCN(90, 2, "����", 2);

    OLED_ShowStr(42, 4, (unsigned char *)"3.", 2);
    OLED_ShowCN(58, 4, "ָ�ƹ���", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "����", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}
