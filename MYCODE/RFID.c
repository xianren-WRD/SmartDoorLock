#include "RFID.h"
#include <includes.h>

extern OS_MUTEX mutex_oled;		//互斥量的对象

//MFRC522数据区
u8 card_typebuf[2];
u8 card_numberbuf[7] = {0};
u8 card_keyAbuf[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

u8 rfid_buf[4096] = {0};
u8 rfid_buf_temp[4096] = {0};
u8 rfid_number = 0;

void Show_RFID_Management(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "录入卡", 4);

    OLED_ShowStr(42, 2, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 2, "删一张卡", 4);

    OLED_ShowStr(42, 4, (unsigned char *)"3.", 2);
    OLED_ShowCN(58, 4, "删所有卡", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "返回", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}

void Show_RFID_Input(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();

    OLED_ShowCN(42, 0, "请刷要录入", 5);
    OLED_ShowCN(42, 2, "的卡", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}

void Show_Delete_Card(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();

    OLED_ShowCN(42, 0, "请刷要删除", 5);
    OLED_ShowCN(42, 2, "的卡", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}

//功能：等待用户刷卡，并获取卡的相关数据
//返回值：0--有读到卡片
//		  -1--卡序列号校验有错误
int Wait_RFID_Card(void)
{
    u8 i, status, card_size, buffer = 0;

    MFRC522_Initializtion();
    status = MFRC522_Request(0x52, card_typebuf);			//寻卡

    if(status == 0)		//如果读到卡
    {
        status = MFRC522_Anticoll(card_numberbuf);			//防撞处理
        card_size = MFRC522_SelectTag(card_numberbuf);		//选卡
        status = MFRC522_Auth(0x60, 4, card_keyAbuf, card_numberbuf);	//验卡

        //卡类型显示
        printf("卡类型 = %d%d\n", card_typebuf[0], card_typebuf[1]);

        printf("卡的容量 = %d\n", card_size);

        //卡序列号显示，最后一字节为卡的校验码
        printf("卡序列号 = ");
        for(i = 0; i < 4; i++)
        {
            buffer  ^=	card_numberbuf[i];	//卡校验码
            printf("%X", card_numberbuf[i]);
        }
        printf("\n");

        if(buffer != card_numberbuf[4])
            return -1;
    }

    return status;
}

//功能：将记录了RFID卡的数据和RFID卡的数量写到对应的flash中
//参数：rfid_buf--存放RFID卡数据的地址
//		rfid_number--RFID卡的数量
void RFID_Write_Flash(u8 *rfid_buf, u8 rfid_number)
{
    unsigned int len = 0;
    u8 j;

    len = strlen((char *)rfid_buf);
    printf("rfid_buf_len = %d\n", len);
    //要用临界段保护的宏
    CPU_SR_ALLOC();
    //进入临界区，保护以下的代码，关闭总中断，停止了ucos的任务调度，其他任务已经停止执行
    OS_CRITICAL_ENTER();
    W25Q128_Erase_Sector(RFID_CARD_ADDR);
    for(j = 0; len > 0; len -= 255, j++)
    {
        if(len <= 256)
        {
            W25Q128_Writer_Data(RFID_CARD_ADDR + 256 * j, rfid_buf + 256 * j, len);
            break;
        }
        //如果大于256个字节，则写到下一个页
        W25Q128_Writer_Data(RFID_CARD_ADDR + 256 * j, rfid_buf + 256 * j, 256);
    }

    //写RFID卡数量
    W25Q128_Erase_Sector(RFID_NUMBER_ADDR);
    W25Q128_Writer_Byte(RFID_NUMBER_ADDR, rfid_number);

    //退出临界区，开启总中断，允许ucos的任务调度
    OS_CRITICAL_EXIT();
}

//功能：录入新的RFID卡片，将新的卡片id存到flash中
//返回值：0--成功录入新卡
//		  -1--卡已经存在
//		  -2--超过默认所能录入卡片数量的上限
int Input_RFID_Card(void)
{
    OS_ERR err;

    while(Wait_RFID_Card() != 0) OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

    W25Q128_Read_Data(RFID_CARD_ADDR, rfid_buf, strlen((char *)rfid_buf));
    if(Card_Already_Exist(rfid_buf, card_numberbuf))
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "该卡已存在", 5);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
        return -1;
    }

    card_numberbuf[5] = ' ';//空格作为分隔符
    if(strlen((char *)rfid_buf) + strlen((char *)card_numberbuf) >= sizeof(rfid_buf))
    {
        printf("已经超过所能录入卡片数量的上限\n");
        return -2;
    }
    strcat((char *)rfid_buf, (char *)card_numberbuf);
    rfid_number++;
    RFID_Write_Flash(rfid_buf, rfid_number);
    //	W25Q128_Read_Data(RFID_CARD_ADDR, rfid_buf, strlen((char *)rfid_buf));
    //	printf("rfid:%s card:%s\n", rfid_buf, card_numberbuf);

    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
    My_OLED_CLR();
    OLED_ShowCN(42, 0, "录入成功", 4);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

    return 0;
}

//功能：判断即将要录入的卡片是否已经录入过
bool Card_Already_Exist(u8 *rfid_buf, u8 *card_number)
{
    strcpy((char *)rfid_buf_temp, (char *)rfid_buf);
    char *tokenPtr = strtok((char *)rfid_buf_temp, " ");

    card_number[5] = '\0';

    while(tokenPtr != NULL)
    {
        if(strcmp(tokenPtr, (char *)card_number) == 0)
        {
            return true;
        }
        tokenPtr = strtok(NULL, " ");
    }

    return false;
}

//功能：从flash删除一张存在的卡
void Delete_Exist_Card(void)
{
    OS_ERR err;
    char token_temp[5] = {0};

    while(Wait_RFID_Card() != 0) OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

    if(Card_Already_Exist(rfid_buf, card_numberbuf))
    {
        strcpy((char *)rfid_buf_temp, (char *)rfid_buf);
        memset(rfid_buf, 0, sizeof(rfid_buf));
        card_numberbuf[5] = '\0';

        char *tokenPtr = strtok((char *)rfid_buf_temp, " ");
        while(tokenPtr != NULL)
        {
            if(strcmp(tokenPtr, (char *)card_numberbuf) != 0)
            {
                strcpy(token_temp, tokenPtr);//teokenPtr不要修改它
                strcat(token_temp, " ");
                strcat((char *)rfid_buf, token_temp);
            }
            tokenPtr = strtok(NULL, " ");
        }

        rfid_number--;
        RFID_Write_Flash(rfid_buf, rfid_number);

        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "已删一张卡", 5);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
    else
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "该卡不存在", 5);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void Delete_All_Card(void)
{
    OS_ERR err;
    memset(rfid_buf, 0, sizeof(rfid_buf));
    rfid_number = 0;

    //要用临界段保护的宏
    CPU_SR_ALLOC();
    //进入临界区，保护以下的代码，关闭总中断，停止了ucos的任务调度，其他任务已经停止执行
    OS_CRITICAL_ENTER();
    W25Q128_Erase_Sector(RFID_CARD_ADDR);
    W25Q128_Erase_Sector(RFID_NUMBER_ADDR);
    W25Q128_Writer_Byte(RFID_NUMBER_ADDR, rfid_number);
    //退出临界区，开启总中断，允许ucos的任务调度
    OS_CRITICAL_EXIT();

    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
    My_OLED_CLR();
    OLED_ShowCN(42, 0, "已删所有卡", 5);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}
