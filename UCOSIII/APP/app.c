#include  <includes.h>

#define FLASH_DEBUG 0
#define MSG_SIZE	20//蓝牙内建消息队列的大小

//MFRC522数据区
extern u8 card_numberbuf[7];
extern u8 rfid_buf[4096];

OS_FLAG_GRP	key_flag_grp;	//事件标志组的对象
OS_MUTEX mutex_rfid;		//互斥量的对象
OS_MUTEX mutex_oled;		//互斥量的对象
OS_MUTEX mutex_fingerprint; //互斥量的对象

u8 admin_password[7] = "123456";//管理员初始密码
u8 user_password[7] = "123456";//用户初始密码

extern unsigned char gddram_temp[8][128];//OLED虚拟显存缓冲区
extern unsigned char GDDRAM[8][128];//OLED虚拟显存

extern unsigned char BMP1[];//上锁图片
extern unsigned char BMP2[];//解锁成功显示的图片

extern u8 FingerID_buf[1200];//指纹数据缓冲区
extern u16 Fingerprint_data_len;//指纹数据长度

extern u8 rfid_number;//RFID卡的数量

//硬件初始化任务
static OS_TCB  HardwareInitTCB;
static CPU_STK HardwareInitStk[APP_CFG_TASK_START_STK_SIZE];
static void HardwareInit(void *p_arg);

//操作任务
static OS_TCB  OperationTCB;
static CPU_STK OperationStk[APP_CFG_TASK_START_STK_SIZE];
static void OperationTask(void *p_arg);

//RFID解锁任务
static OS_TCB  RFIDTCB;
static CPU_STK RFIDStk[APP_CFG_TASK_START_STK_SIZE];
static void RFIDTask(void *p_arg);

//指纹解锁任务
static OS_TCB  FingerprintTCB;
static CPU_STK FingerprintStk[APP_CFG_TASK_START_STK_SIZE];
static void FingerprintTask(void *p_arg);

//蓝牙解锁任务
OS_TCB  BluetoothTCB;
static CPU_STK BluetoothStk[APP_CFG_TASK_START_STK_SIZE];
static void BluetoothTask(void *p_arg);

//蜂鸣器任务
static OS_TCB  BuzzerTCB;
static CPU_STK BuzzerStk[APP_CFG_TASK_START_STK_SIZE];
static void BuzzerTask(void *p_arg);

int main(void)
{
    OS_ERR err;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//中断分组配置

    CPU_SR_ALLOC();

    OSInit(&err);                                               /* Init uC/OS-III.*/
    OS_CRITICAL_ENTER();

    OSTaskCreate((OS_TCB *)&HardwareInitTCB,                    /* Create the start task*/
                 (CPU_CHAR *)"HardwareInit Start",
                 (OS_TASK_PTR   )HardwareInit,
                 (void *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK *)&HardwareInitStk[0u],
                 (CPU_STK_SIZE  )HardwareInitStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0u,
                 (OS_TICK       )0u,
                 (void *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OS_CRITICAL_EXIT();
    OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */
}

void HardwareInit(void *p_arg)
{
    OS_ERR err;
    unsigned char len = 0;//最大的指纹数据长度不会超过1200，两个字节

    CPU_SR_ALLOC();

    BSP_Init();                                                 /* Initialize BSP functions*/

#if FLASH_DEBUG
    W25Q128_Erase_Sector(PASSWORD_ADDR);
    W25Q128_Writer_Data(PASSWORD_ADDR, admin_password, 7);
    W25Q128_Writer_Data(PASSWORD_ADDR, user_password, 7);

    W25Q128_Erase_Sector(RFID_NUMBER_ADDR);
    W25Q128_Writer_Byte(RFID_NUMBER_ADDR, 0x00);
    W25Q128_Erase_Sector(FINGERPRINT_LEN_ADDR);
    W25Q128_Writer_Byte(FINGERPRINT_LEN_ADDR, 0x00);
    W25Q128_Writer_Byte(FINGERPRINT_LEN_ADDR + 1, 0x00);
#endif
    //读取flash中的数据
    W25Q128_Read_Data(PASSWORD_ADDR, admin_password, 7);
    W25Q128_Read_Data(PASSWORD_ADDR + 7, user_password, 7);
    printf("admin = %s, user = %s\n", admin_password, user_password);

    W25Q128_Read_Byte(RFID_NUMBER_ADDR, &rfid_number);//读取RFID卡数量
    W25Q128_Read_Data(RFID_CARD_ADDR, rfid_buf, rfid_number * 6); //读取RFID卡数据，一张卡6个字节数据
    printf("rifd_num = %d\n", rfid_number);

    W25Q128_Read_Byte(FINGERPRINT_LEN_ADDR, &len);//读取指纹数据的长度，分情况读
    if(len > 255)
    {
        Fingerprint_data_len += len << 8;
        W25Q128_Read_Byte(FINGERPRINT_LEN_ADDR + 1, &len);
        Fingerprint_data_len += len;
        printf("%d\n", Fingerprint_data_len);
        W25Q128_Read_Data(FINGERPRINT_LEN_ADDR, (u8 *)FingerID_buf, Fingerprint_data_len);
    }
    else
    {
        W25Q128_Read_Byte(FINGERPRINT_LEN_ADDR + 1, &len);
        W25Q128_Read_Data(FINGERPRINT_ADDR, (u8 *)FingerID_buf, len);
    }
    printf("fingerbuf = %s\n", FingerID_buf);

    OS_CRITICAL_ENTER();

    OSTaskCreate((OS_TCB *)&OperationTCB,
                 (CPU_CHAR *)"OperationTask Start",
                 (OS_TASK_PTR   )OperationTask,
                 (void *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK *)&OperationStk[0u],
                 (CPU_STK_SIZE  )OperationStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0u,
                 (OS_TICK       )0u,
                 (void *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&RFIDTCB,
                 (CPU_CHAR *)"RFIDTask Start",
                 (OS_TASK_PTR   )RFIDTask,
                 (void *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK *)&RFIDStk[0u],
                 (CPU_STK_SIZE  )RFIDStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0u,
                 (OS_TICK       )0u,
                 (void *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&FingerprintTCB,
                 (CPU_CHAR *)"FingerprintTask Start",
                 (OS_TASK_PTR   )FingerprintTask,
                 (void *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK *)&FingerprintStk[0u],
                 (CPU_STK_SIZE  )FingerprintStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0u,
                 (OS_TICK       )0u,
                 (void *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&BluetoothTCB,
                 (CPU_CHAR *)"BluetoothTask Start",
                 (OS_TASK_PTR   )BluetoothTask,
                 (void *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK *)&BluetoothStk[0u],
                 (CPU_STK_SIZE  )BluetoothStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )MSG_SIZE,	//使用内建消息队列，大小为MSG_SIZE
                 (OS_TICK       )0u,
                 (void *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&BuzzerTCB,
                 (CPU_CHAR *)"BuzzerTask Start",
                 (OS_TASK_PTR   )BuzzerTask,
                 (void *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK *)&BuzzerStk[0u],
                 (CPU_STK_SIZE  )BuzzerStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0u,
                 (OS_TICK       )0u,
                 (void *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    //创建事件标志组，所有标志位初值为0
    OSFlagCreate(&key_flag_grp, "key_flag_grp", 0, &err);

    //创建互斥量
    OSMutexCreate(&mutex_rfid, "mutex_rfid", &err);
    OSMutexCreate(&mutex_oled, "mutex_oled", &err);
    OSMutexCreate(&mutex_fingerprint, "mutex_fingerprint", &err);

    OS_CRITICAL_EXIT();

    while(DEF_TRUE)
    {
        GPIO_ToggleBits(GPIOF, GPIO_Pin_9);
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void OperationTask(void *p_arg)
{
    OS_ERR err;
    unsigned char key_value = 255;
    unsigned char password[7] = {0};//存放用户输入的密码
    unsigned char password_admin[7] = {0}, password_user[7] = {0};//存放flash读取到的管理员和用户密码
    unsigned char flash_buf[14] = {0};

    OLED_CLS();
    OLED_DrawBMP(0, 0, 24, 5, (unsigned char *)BMP1);
    delay_ms(2);

    while(DEF_TRUE)
    {
        Show_Main();

        key_value = GetKeyValue(&key_flag_grp);
        if(key_value == KEY_1)
        {
            while(1)
            {
                Show_Fun_Interface();
                key_value = GetKeyValue(&key_flag_grp);
                if(key_value == KEY_D) break;
                else if(key_value == KEY_B)//模拟上锁
                {
                    Show_Virtual_Lock();
                }

                switch (key_value)
                {
                case KEY_1:
                    while(1)
                    {
                        Show_Enter_Password();
                        key_value = Enter_Password(&key_flag_grp, password);
                        if(key_value == KEY_D) break;

                        if(Password_validation(&key_flag_grp, password))
                        {
                            Show_Password_Setting();
                            while(1)
                            {
                                key_value = GetKeyValue(&key_flag_grp);
                                if(key_value == KEY_D) break;
                                if(key_value == KEY_1)//管理员密码设定
                                {
                                    Show_Enter_Password();
                                    key_value = Enter_Password(&key_flag_grp, password);
                                    if(key_value == KEY_D) break;

                                    W25Q128_Read_Data(PASSWORD_ADDR, flash_buf, 14);
                                    strcpy((char *)flash_buf, (const char *)password);
                                    //要用临界段保护的宏
                                    CPU_SR_ALLOC();
                                    //进入临界区，保护以下的代码，关闭总中断，停止了ucos的任务调度，其他任务已经停止执行
                                    OS_CRITICAL_ENTER();
                                    W25Q128_Erase_Sector(PASSWORD_ADDR);
                                    W25Q128_Writer_Data(PASSWORD_ADDR, flash_buf, 14);
                                    //退出临界区，开启总中断，允许ucos的任务调度
                                    OS_CRITICAL_EXIT();
                                    break;
                                }
                                else if(key_value == KEY_2)//用户密码设定
                                {
                                    Show_Enter_Password();
                                    key_value = Enter_Password(&key_flag_grp, password);
                                    if(key_value == KEY_D) break;

                                    W25Q128_Read_Data(PASSWORD_ADDR, flash_buf, 14);
                                    strcpy((char *)(flash_buf + 7), (const char *)password);
                                    //要用临界段保护的宏
                                    CPU_SR_ALLOC();
                                    //进入临界区，保护以下的代码，关闭总中断，停止了ucos的任务调度，其他任务已经停止执行
                                    OS_CRITICAL_ENTER();
                                    W25Q128_Erase_Sector(PASSWORD_ADDR);
                                    W25Q128_Writer_Data(PASSWORD_ADDR, flash_buf, 14);
                                    //退出临界区，开启总中断，允许ucos的任务调度
                                    OS_CRITICAL_EXIT();
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    break;
                case KEY_2:
                    Show_Enter_Password();
                    key_value = Enter_Password(&key_flag_grp, password);
                    if(key_value == KEY_D) break;

                    if(Password_validation(&key_flag_grp, password))
                    {
                        while(1)
                        {
                            Show_RFID_Management();

                            key_value = GetKeyValue(&key_flag_grp);
                            if(key_value == KEY_D) break;
                            if(key_value == KEY_1)
                            {
                                Show_RFID_Input();
                                OSMutexPend(&mutex_rfid, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                                Input_RFID_Card();
                                OSTimeDlyHMSM(0, 0, 2, 0, OS_OPT_TIME_HMSM_STRICT, &err); //延时两秒再释放
                                OSMutexPost(&mutex_rfid, OS_OPT_POST_NONE, &err); //释放互斥量
                            }
                            else if(key_value == KEY_2)
                            {
                                Show_Delete_Card();
                                OSMutexPend(&mutex_rfid, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                                Delete_Exist_Card();
                                OSMutexPost(&mutex_rfid, OS_OPT_POST_NONE, &err); //释放互斥量
                            }
                            else if(key_value == KEY_3)
                            {
                                OSMutexPend(&mutex_rfid, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                                Delete_All_Card();
                                OSMutexPost(&mutex_rfid, OS_OPT_POST_NONE, &err); //释放互斥量
                            }
                        }
                    }
                    break;
                case KEY_3:
                    Show_Enter_Password();
                    key_value = Enter_Password(&key_flag_grp, password);
                    if(key_value == KEY_D) break;

                    if(Password_validation(&key_flag_grp, password))
                    {
                        while(1)
                        {
                            Show_Fingerprint_Management();

                            key_value = GetKeyValue(&key_flag_grp);
                            if(key_value == KEY_D) break;
                            if(key_value == KEY_1)
                            {
                                OSMutexPend(&mutex_fingerprint, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                                Add_FR();
                                OSMutexPost(&mutex_fingerprint, OS_OPT_POST_NONE, &err); //释放互斥量
                            }
                            else if(key_value == KEY_2)
                            {
                                OSMutexPend(&mutex_fingerprint, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                                Del_FR();
                                OSMutexPost(&mutex_fingerprint, OS_OPT_POST_NONE, &err); //释放互斥量
                            }
                            else if(key_value == KEY_3)
                            {
                                while(GetKeyValue(&key_flag_grp) != KEY_D)
                                {
                                    View_Exist_Fingerprint();
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
        else if(key_value == KEY_2)
        {
            while(1)
            {
                Show_Enter_Password();
                key_value = Enter_Password(&key_flag_grp, password);
                if(key_value == KEY_D) break;

                W25Q128_Read_Data(PASSWORD_ADDR, password_admin, 7);
                W25Q128_Read_Data(PASSWORD_ADDR + 7, password_user, 7);

                if(strcmp((const char *)password_admin, (const char *)password) == 0
                        || strcmp((const char *)password_user, (const char *)password) == 0)
                {
                    OSTaskSemPost(&BuzzerTCB, OS_OPT_POST_NONE, &err); //发送蜂鸣器任务的内嵌信号量

                    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                    My_OLED_CLR();
                    OLED_DrawBMP(0, 0, 41, 5, (unsigned char *)BMP2);
                    OLED_ShowCN(42, 0, "密码解锁", 4);
                    OLED_ShowCN(42, 2, "成功", 2);
                    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
                    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                    break;
                }
                else
                {
                    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
                    My_OLED_CLR();
                    OLED_ShowCN(42, 0, "密码错误", 4);
                    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
                    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                }
            }
        }
        else if(key_value == KEY_B)//模拟上锁
        {
            Show_Virtual_Lock();
        }
    }
}

void RFIDTask(void *p_arg)
{
    OS_ERR err;

    while(1)
    {
        OSMutexPend(&mutex_rfid, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        memset(card_numberbuf, 0, sizeof(card_numberbuf));
        if(Wait_RFID_Card() == 0)
        {
            if(Card_Already_Exist(rfid_buf, card_numberbuf))
            {
                OSTaskSemPost(&BuzzerTCB, OS_OPT_POST_NONE, &err); //发送蜂鸣器任务的内嵌信号量

                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

                Gddram_Copy(gddram_temp, GDDRAM);//锁存数据
                Unlock_Rest_OLED_Fill(0x00);
                OLED_DrawBMP(0, 0, 41, 5, (unsigned char *)BMP2);
                OLED_ShowStr(42, 0, (unsigned char *)"RFID", 2);
                OLED_ShowCN(42, 2, "解锁成功", 4);
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                Show_Previous_Interface();

                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            }
        }
        OSMutexPost(&mutex_rfid, OS_OPT_POST_NONE, &err); //释放互斥量
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void FingerprintTask(void *p_arg)
{
    OS_ERR err;
    SearchResult search;

    while(1)
    {
        OSMutexPend(&mutex_fingerprint, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        if(Press_FR(&search))
        {
            OSTaskSemPost(&BuzzerTCB, OS_OPT_POST_NONE, &err); //发送蜂鸣器任务的内嵌信号量

            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

            Gddram_Copy(gddram_temp, GDDRAM);//锁存数据
            Unlock_Rest_OLED_Fill(0x00);
            OLED_DrawBMP(0, 0, 41, 5, (unsigned char *)BMP2);
            OLED_ShowCN(42, 0, "指纹解锁", 4);
            OLED_ShowCN(42, 2, "成功", 2);
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
            Show_Previous_Interface();

            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
        }
        OSMutexPost(&mutex_fingerprint, OS_OPT_POST_NONE, &err); //释放互斥量
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void BluetoothTask(void *p_arg)
{
    OS_ERR err;
    OS_MSG_SIZE msg_size;
    u8 *msg;
    char password[6];//存放蓝牙发送过来的密码
    unsigned char i = 0, len = 0;
    unsigned char flash_buf[14] = {0};
    unsigned char password_admin[7] = {0}, password_user[7] = {0};//存放flash读取到的管理员和用户密码
    u8 buffer[64] = {0};//存放蓝牙数据

    while(1)
    {
        msg = OSTaskQPend(0, OS_OPT_PEND_BLOCKING, &msg_size, 0, &err);

        if(i == 64)
        {
            printf("蓝牙数据过长，将覆盖之前的数据\n");
            i = 0;
        }

        if(*msg != '#')
        {
            buffer[i++] =  *msg;
            continue;
        }
        i = 0;//置为0,下一帧数据从buffer[0]开始存储
        printf("msg_buffer = %s\n", buffer);

        //TODO：用户输入的密码可以增加出错判断，比如：输入字母E是不合法的，因为没有这个按键
        if(strstr((const char *)buffer, "BG1CHGUSER")) //发送格式：BG1CHGUSERXXXXXXE# 长度：18
        {
            if(strlen((char *)buffer) > 18)
            {
                memset(buffer, 0, sizeof(buffer)); //清空之前缓冲区的数据
                continue;
            }
            len = strlen("BG1CHGUSER");
            strncpy(password, (char *)(buffer + len), 6); //6位密码
            printf("password %s\n", password);
            char *tokPtr = strtok(password, "1234567890");//限制密码只能是数字
            if(tokPtr != NULL)
            {
                printf("密码不合法\n");
                continue;
            }
            len = strlen((char *)buffer);
            if(buffer[len - 1] != 'E')
            {
                printf("校验和错误\n");
                continue;
            }

            W25Q128_Read_Data(PASSWORD_ADDR, flash_buf, 14);
            strcpy((char *)(flash_buf + 7), password);
            //要用临界段保护的宏
            CPU_SR_ALLOC();
            //进入临界区，保护以下的代码，关闭总中断，停止了ucos的任务调度，其他任务已经停止执行
            OS_CRITICAL_ENTER();
            W25Q128_Erase_Sector(PASSWORD_ADDR);
            W25Q128_Writer_Data(PASSWORD_ADDR, flash_buf, 14);
            //退出临界区，开启总中断，允许ucos的任务调度
            OS_CRITICAL_EXIT();
        }
        if(strstr((const char *)buffer, "BG1CHGADMIN")) //发送格式：BG1CHGADMINXXXXXXF#	长度：19
        {
            if(strlen((char *)buffer) > 19)
            {
                memset(buffer, 0, sizeof(buffer)); //清空之前缓冲区的数据
                continue;
            }
            len = strlen("BG1CHGADMIN");
            strncpy(password, (char *)(buffer + len), 6); //6位密码
            printf("password %s\n", password);
            char *tokPtr = strtok(password, "1234567890");//限制密码只能是数字
            if(tokPtr != NULL)
            {
                printf("密码不合法\n");
                continue;
            }
            len = strlen((char *)buffer);
            if(buffer[len - 1] != 'F')
            {
                printf("校验和错误\n");
                continue;
            }

            W25Q128_Read_Data(PASSWORD_ADDR, flash_buf, 14);
            strcpy((char *)flash_buf, password);
            //要用临界段保护的宏
            CPU_SR_ALLOC();
            //进入临界区，保护以下的代码，关闭总中断，停止了ucos的任务调度，其他任务已经停止执行
            OS_CRITICAL_ENTER();
            W25Q128_Erase_Sector(PASSWORD_ADDR);
            W25Q128_Writer_Data(PASSWORD_ADDR, flash_buf, 14);
            //退出临界区，开启总中断，允许ucos的任务调度
            OS_CRITICAL_EXIT();
        }
        if(strstr((const char *)buffer, "BG1OPEN")) //发送格式：BG1OPENXXXXXXB#	长度：15
        {
            if(strlen((char *)buffer) > 15)
            {
                memset(buffer, 0, sizeof(buffer)); //清空之前缓冲区的数据
                continue;
            }

            len = strlen("BG1OPEN");
            strncpy(password, (char *)(buffer + len), 6); //6位密码
            printf("password %s\n", password);
            len = strlen((char *)buffer);
            if(buffer[len - 1] != 'B')
            {
                printf("校验和错误\n");
                continue;
            }

            W25Q128_Read_Data(PASSWORD_ADDR, password_admin, 7);
            W25Q128_Read_Data(PASSWORD_ADDR + 7, password_user, 7);

            if(strcmp((const char *)password_admin, password) == 0
                    || strcmp((const char *)password_user, password) == 0)
            {
                OSTaskSemPost(&BuzzerTCB, OS_OPT_POST_NONE, &err); //发送蜂鸣器任务的内嵌信号量

                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

                Gddram_Copy(gddram_temp, GDDRAM);//锁存数据
                Unlock_Rest_OLED_Fill(0x00);
                OLED_DrawBMP(0, 0, 41, 5, (unsigned char *)BMP2);
                OLED_ShowCN(42, 0, "手机解锁", 4);
                OLED_ShowCN(42, 2, "成功", 2);
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                Show_Previous_Interface();

                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            }
            else
            {
                printf("蓝牙解锁失败，密码有误\n");
            }
        }
        memset(buffer, 0, sizeof(buffer)); //清空之前缓冲区的数据
        OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void BuzzerTask(void *p_arg)
{
    OS_ERR err;

    while(1)
    {
        OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, 0, &err); //等待自己的内嵌信号量，默认初值为0
        BEEP_ON;
        OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_HMSM_STRICT, &err);
        BEEP_OFF;
    }
}


