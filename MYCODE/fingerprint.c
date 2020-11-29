#include "fingerprint.h"
#include "as608.h"
#include "delay.h"

SysPara AS608Para;//指纹模块AS608参数
u16 ValidN;//模块内有效指纹个数
u8 FingerID_buf[1200] = {0};
u8 FingerID_buf_temp[1200] = {0};
u16 Fingerprint_data_len = 0;//指纹数据长度

extern OS_MUTEX mutex_oled;		//互斥量的对象
extern OS_FLAG_GRP	key_flag_grp;	//事件标志组的对象

void Show_Fingerprint_Management(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "录入指纹", 4);

    OLED_ShowStr(42, 2, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 2, "删除指纹", 4);

    OLED_ShowStr(42, 4, (unsigned char *)"3.", 2);
    OLED_ShowCN(58, 4, "查看指纹", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "返回", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}

void Show_Press_Fingerprint(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
    My_OLED_CLR();
    OLED_ShowCN(42, 0, "请按指纹", 4);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}

void Show_Fingerprint_Normal(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
    My_OLED_CLR();
    OLED_ShowCN(42, 0, "指纹正常", 4);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}

void Show_Del_Fingerprint(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "删指定", 3);

    OLED_ShowStr(42, 2, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 2, "删所有", 3);

    OLED_ShowStr(42, 4, (unsigned char *)"3.", 2);
    OLED_ShowCN(58, 4, "刷指纹删", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "返回", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}

//功能：获取用户输入的id号
u16 Get_Input_ID(void)
{
    OS_ERR err;
    u8 j = 0, key_value = 255;
    u16 id = 0;

    while(1)
    {
        key_value = GetKeyValue(&key_flag_grp);
        if(key_value == 255)
            continue;
        if(key_value == KEY_C && id < AS608Para.PS_max)
        {
            return id;
        }
        if(key_value == KEY_A)
        {
            id /= 10;
            if(j == 0)
                continue;
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            OLED_ShowChar(42 + (--j) * 8, 3, ' ', 16);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            continue;
        }
        if(key_value == KEY_SHIFT_3 || key_value == KEY_SHIFT_8)
            continue;

        id *= 10;
        id += (key_value - '0');

        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        OLED_ShowChar(42 + j * 8, 3, key_value, 16);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
        j++;
    }
}

//功能：将指纹ID数据和长度写到默认的flash中
//参数：FingerID_buf--存放指纹数据的地址
void Fingerprint_Write_Flash(u8 *FingerID_buf)
{
    unsigned short len = 0;
    unsigned char len_t = 0;
    u8 j;

    len = strlen((char *)FingerID_buf);
    printf("len = %d\n", len);
    //要用临界段保护的宏
    CPU_SR_ALLOC();
    //进入临界区，保护以下的代码，关闭总中断，停止了ucos的任务调度，其他任务已经停止执行
    OS_CRITICAL_ENTER();
    W25Q128_Erase_Sector(FINGERPRINT_ADDR);
    for(j = 0; len > 0; len -= 255, j++)
    {
        if(len <= 256)
        {
            W25Q128_Writer_Data(FINGERPRINT_ADDR + 256 * j, FingerID_buf + 256 * j, len);
            break;
        }
        //如果大于256个字节，则写到下一个页
        W25Q128_Writer_Data(FINGERPRINT_ADDR + 256 * j, FingerID_buf + 256 * j, 256);
    }

    W25Q128_Erase_Sector(FINGERPRINT_LEN_ADDR);
    if(len > 255)//指纹长度是两个字节的，所以要分开存储
    {
        len_t = len >> 8;
        W25Q128_Writer_Byte(FINGERPRINT_LEN_ADDR, len_t);
    }
    else
    {
        W25Q128_Writer_Byte(FINGERPRINT_LEN_ADDR, 0x00);
    }

    W25Q128_Writer_Byte(FINGERPRINT_LEN_ADDR + 1, (u8)len);

    //退出临界区，开启总中断，允许ucos的任务调度
    OS_CRITICAL_EXIT();
}

//功能：从缓冲区中删除指定的指纹id
//参数：要删除的指纹id
void Del_Fingerprint_ID(u16 id)
{
    u8 id_temp[4] = {0};
    char token_temp[5] = {0};

    strcpy((char *)FingerID_buf_temp, (char *)FingerID_buf);
    memset(FingerID_buf, 0, sizeof(FingerID_buf));

    sprintf((char *)id_temp, "%d", id);

    char *tokenPtr = strtok((char *)FingerID_buf_temp, " ");
    while(tokenPtr != NULL)
    {
        if(strcmp(tokenPtr, (char *)id_temp) != 0)
        {
            strcpy(token_temp, tokenPtr);//teokenPtr不要修改它
            strcat(token_temp, " ");
            strcat((char *)FingerID_buf, token_temp);
        }
        tokenPtr = strtok(NULL, " ");
    }
}

//功能：判断指纹id是否已经有对应的有效指纹了
//参数：FingerID_buf--存放指纹id的缓冲区
//		id--要判断的指纹id
bool Fingerprint_Already_Exist(u8 *FingerID_buf, u16 id)
{
    u8 id_temp[4] = {0};
    strcpy((char *)FingerID_buf_temp, (char *)FingerID_buf);
    printf("192:%s\n", FingerID_buf);
    char *tokenPtr = strtok((char *)FingerID_buf_temp, " ");

    sprintf((char *)id_temp, "%d", id);

    while(tokenPtr != NULL)
    {
        if(strcmp(tokenPtr, (char *)id_temp) == 0)
        {
            return true;
        }
        tokenPtr = strtok(NULL, " ");
    }

    return false;
}

//显示确认码错误信息
void ShowErrMessage(u8 ensure)
{
    printf("%s\n", (u8 *)EnsureMessage(ensure));
}
//录指纹
void Add_FR(void)
{
    OS_ERR err;
    u8 ensure, i = 0, processnum = 0;
    u16 id = 0;
    unsigned char surplus[4] = {0}; //剩余容量显示
    char temp[5] = {0};

    if(ValidN == 300)
    {
        printf("已经超过指纹库容量上限，无法继续录入指纹\n");
        return;
    }

    while(1)
    {
        switch (processnum)
        {
        case 0:
            i++;
            Show_Press_Fingerprint();

            ensure = PS_GetImage();
            if(ensure == 0x00)
            {
                ensure = PS_GenChar(CharBuffer1); //生成特征
                if(ensure == 0x00)
                {
                    Show_Fingerprint_Normal();
                    i = 0;
                    processnum = 1; //跳到第二步
                }
                else ShowErrMessage(ensure);
            }
            else ShowErrMessage(ensure);
            break;

        case 1:
            i++;
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            OLED_ShowCN(42, 0, "请再按一次", 5);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

            ensure = PS_GetImage();
            if(ensure == 0x00)
            {
                ensure = PS_GenChar(CharBuffer2); //生成特征
                if(ensure == 0x00)
                {
                    Show_Fingerprint_Normal();
                    i = 0;
                    processnum = 2; //跳到第三步
                }
                else ShowErrMessage(ensure);
            }
            else ShowErrMessage(ensure);
            break;

        case 2:
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "对比两次", 4);
            OLED_ShowCN(42, 2, "指纹", 2);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

            ensure = PS_Match();
            if(ensure == 0x00)
            {
                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                My_OLED_CLR();
                OLED_ShowCN(42, 0, "对比成功", 4);
                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                processnum = 3; //跳到第四步
            }
            else
            {
                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                My_OLED_CLR();
                OLED_ShowCN(42, 0, "对比失败", 4);
                OLED_ShowCN(42, 2, "需重录指纹", 5);
                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                ShowErrMessage(ensure);
                i = 0;
                processnum = 0; //跳回第一步
            }
            delay_ms(1200);
            break;

        case 3:
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "生成指纹", 4);
            OLED_ShowCN(42, 2, "模板", 2);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

            ensure = PS_RegModel();
            if(ensure == 0x00)
            {
                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                My_OLED_CLR();
                OLED_ShowCN(42, 0, "生成指纹", 4);
                OLED_ShowCN(42, 2, "模板成功", 4);
                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                processnum = 4; //跳到第五步
            }
            else
            {
                processnum = 0;
                ShowErrMessage(ensure);
            }
            delay_ms(1200);
            break;

        case 4:
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "请输入储存", 5);
            OLED_ShowStr(42, 2, (unsigned char *)"ID(0=<ID<=299)", 1);
            OLED_ShowStr(20, 6, (unsigned char *)"A.", 2);
            OLED_ShowCN(36, 6, "删除", 2);
            OLED_ShowStr(80, 6, (unsigned char *)"C.", 2);
            OLED_ShowCN(96, 6, "确定", 2);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

            W25Q128_Read_Data(FINGERPRINT_ADDR, FingerID_buf, strlen((char *)FingerID_buf));
            id = Get_Input_ID();
            //TODO：这里可以加一个判断，看用户输入的id是否对应着有效指纹
            sprintf(temp, "%u ", id);
            strcat((char *)FingerID_buf, temp);
            Fingerprint_Write_Flash(FingerID_buf);
            printf("key_value:%d\n", id);

            ensure = PS_StoreChar(CharBuffer2, id); //储存模板
            if(ensure == 0x00)
            {
                PS_ValidTempleteNum(&ValidN);//读库指纹个数

                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                My_OLED_CLR();
                OLED_ShowCN(42, 0, "录入成功", 4);
                OLED_ShowCN(42, 2, "剩余容量为", 5);
                sprintf((char *)surplus, "%d", AS608Para.PS_max - ValidN);
                OLED_ShowStr(42, 4, surplus, 2);
                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

                printf("剩余容量为 %d\n", AS608Para.PS_max - ValidN); //显示剩余指纹容量
                delay_ms(1500);
                return;
            }
            else
            {
                processnum = 0;
                ShowErrMessage(ensure);
            }
            break;
        }
        delay_ms(400);
        if(i == 5) //超过5次没有按手指则退出
        {
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "超过五次没", 5);
            OLED_ShowCN(42, 2, "按手指返回", 5);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
            break;
        }
    }
}

//刷指纹
bool Press_FR(SearchResult *search)
{
    u8 ensure;

    ensure = PS_GetImage();
    if(ensure == 0x00) //获取图像成功
    {
        ensure = PS_GenChar(CharBuffer1);
        if(ensure == 0x00) //生成特征成功
        {
            ensure = PS_HighSpeedSearch(CharBuffer1, 0, AS608Para.PS_max, search);
            if(ensure == 0x00) //搜索成功
            {
                printf("确有此人,ID:%d  匹配得分:%d", search->pageID, search->mathscore);
                return true;
            }
            else
                ShowErrMessage(ensure);
        }
        else
            ShowErrMessage(ensure);
    }
    return false;
}

//删除指纹
void Del_FR(void)
{
    OS_ERR err;
    u8  ensure, key_value = 255;
    u16 id;
    SearchResult search;
    unsigned char surplus[4] = {0}; //剩余容量显示

    Show_Del_Fingerprint();
    key_value = GetKeyValue(&key_flag_grp);
    if(key_value == KEY_1)
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "请输入指纹", 5);
        OLED_ShowStr(42, 2, (unsigned char *)"ID(0=<ID<=299)", 1);
        OLED_ShowStr(20, 6, (unsigned char *)"A.", 2);
        OLED_ShowCN(36, 6, "删除", 2);
        OLED_ShowStr(80, 6, (unsigned char *)"C.", 2);
        OLED_ShowCN(96, 6, "确定", 2);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

        id = Get_Input_ID(); //获取输入的数值
        if(Fingerprint_Already_Exist(FingerID_buf, id))
        {
            ensure = PS_DeletChar(id, 1); //删除单个指纹
            Del_Fingerprint_ID(id);
            Fingerprint_Write_Flash(FingerID_buf);
        }
        else
        {
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "指纹不存在", 5);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
            return;
        }
    }
    else if(key_value == KEY_2)
    {
        ensure = PS_Empty(); //清空指纹库
        memset(FingerID_buf, 0, sizeof(FingerID_buf));
        Fingerprint_Write_Flash(FingerID_buf);
    }
    else if(key_value == KEY_3)
    {
        Show_Press_Fingerprint();
        if(Press_FR(&search))
        {
            ensure = PS_DeletChar(search.pageID, 1); //删除单个指纹
            Del_Fingerprint_ID(search.pageID);
            Fingerprint_Write_Flash(FingerID_buf);
        }
        else
        {
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "指纹不存在", 5);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
        }
    }

    if(ensure == 0)
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "删指纹成功", 5);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
    }
    else
        ShowErrMessage(ensure);

    PS_ValidTempleteNum(&ValidN);//读库指纹个数
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
    OLED_ShowCN(42, 2, "剩余容量为", 5);
    sprintf((char *)surplus, "%d", AS608Para.PS_max - ValidN);
    OLED_ShowStr(42, 4, surplus, 2);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    printf("库指纹个数 %d\n", AS608Para.PS_max - ValidN);
}

//功能：显示已经录入的指纹，这里显示3条记录示意
void View_Exist_Fingerprint(void)
{
    OS_ERR err;
    unsigned char temp[3] = {0};
    u8 i = 0, j = 0;

    strcpy((char *)FingerID_buf_temp, (char *)FingerID_buf);
    printf("499:%s\n", FingerID_buf);
    char *tokenPtr = strtok((char *)FingerID_buf_temp, " ");

    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
    My_OLED_CLR();
    while(tokenPtr != NULL)
    {
        if(i > 3)
            break;
        sprintf((char *)temp, "%d.%s", ++i, tokenPtr);
        OLED_ShowStr(42, j, temp, 2);
        j += 2;
        tokenPtr = strtok(NULL, " ");
    }
    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "返回", 2);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}

void Get_Fingerprint_Module_Parameters(void)
{
    u8 ensure;

    PS_StaGPIO_Init();	//初始化FR读状态引脚

    while(PS_HandShake(&AS608Addr))//与AS608模块握手
    {
        delay_ms(400);
        printf("未检测到模块!!!");
        delay_ms(800);
        printf("尝试连接模块...");
    }
    printf("通讯成功!!!");
    printf("波特率:%d   地址:%x", 57600, AS608Addr);

    ensure = PS_ValidTempleteNum(&ValidN); //读库指纹个数
    if(ensure != 0x00)
        ShowErrMessage(ensure);//显示确认码错误信息
    ensure = PS_ReadSysPara(&AS608Para); //读参数
    if(ensure == 0x00)
    {
        printf("库容量:%d     对比等级: %d", AS608Para.PS_max - ValidN, AS608Para.PS_level);
    }
    else
        ShowErrMessage(ensure);
}
