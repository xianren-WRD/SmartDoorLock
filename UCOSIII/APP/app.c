#include  <includes.h>

#define FLASH_DEBUG 0
#define MSG_SIZE	20//�����ڽ���Ϣ���еĴ�С

//MFRC522������
extern u8 card_numberbuf[7];
extern u8 rfid_buf[4096];

OS_FLAG_GRP	key_flag_grp;	//�¼���־��Ķ���
OS_MUTEX mutex_rfid;		//�������Ķ���
OS_MUTEX mutex_oled;		//�������Ķ���
OS_MUTEX mutex_fingerprint; //�������Ķ���

u8 admin_password[7] = "123456";//����Ա��ʼ����
u8 user_password[7] = "123456";//�û���ʼ����

extern unsigned char gddram_temp[8][128];//OLED�����Դ滺����
extern unsigned char GDDRAM[8][128];//OLED�����Դ�

extern unsigned char BMP1[];//����ͼƬ
extern unsigned char BMP2[];//�����ɹ���ʾ��ͼƬ

extern u8 FingerID_buf[1200];//ָ�����ݻ�����
extern u16 Fingerprint_data_len;//ָ�����ݳ���

extern u8 rfid_number;//RFID��������

//Ӳ����ʼ������
static OS_TCB  HardwareInitTCB;
static CPU_STK HardwareInitStk[APP_CFG_TASK_START_STK_SIZE];
static void HardwareInit(void *p_arg);

//��������
static OS_TCB  OperationTCB;
static CPU_STK OperationStk[APP_CFG_TASK_START_STK_SIZE];
static void OperationTask(void *p_arg);

//RFID��������
static OS_TCB  RFIDTCB;
static CPU_STK RFIDStk[APP_CFG_TASK_START_STK_SIZE];
static void RFIDTask(void *p_arg);

//ָ�ƽ�������
static OS_TCB  FingerprintTCB;
static CPU_STK FingerprintStk[APP_CFG_TASK_START_STK_SIZE];
static void FingerprintTask(void *p_arg);

//������������
OS_TCB  BluetoothTCB;
static CPU_STK BluetoothStk[APP_CFG_TASK_START_STK_SIZE];
static void BluetoothTask(void *p_arg);

//����������
static OS_TCB  BuzzerTCB;
static CPU_STK BuzzerStk[APP_CFG_TASK_START_STK_SIZE];
static void BuzzerTask(void *p_arg);

int main(void)
{
    OS_ERR err;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//�жϷ�������

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
    unsigned char len = 0;//����ָ�����ݳ��Ȳ��ᳬ��1200�������ֽ�

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
    //��ȡflash�е�����
    W25Q128_Read_Data(PASSWORD_ADDR, admin_password, 7);
    W25Q128_Read_Data(PASSWORD_ADDR + 7, user_password, 7);
    printf("admin = %s, user = %s\n", admin_password, user_password);

    W25Q128_Read_Byte(RFID_NUMBER_ADDR, &rfid_number);//��ȡRFID������
    W25Q128_Read_Data(RFID_CARD_ADDR, rfid_buf, rfid_number * 6); //��ȡRFID�����ݣ�һ�ſ�6���ֽ�����
    printf("rifd_num = %d\n", rfid_number);

    W25Q128_Read_Byte(FINGERPRINT_LEN_ADDR, &len);//��ȡָ�����ݵĳ��ȣ��������
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
                 (OS_MSG_QTY    )MSG_SIZE,	//ʹ���ڽ���Ϣ���У���СΪMSG_SIZE
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

    //�����¼���־�飬���б�־λ��ֵΪ0
    OSFlagCreate(&key_flag_grp, "key_flag_grp", 0, &err);

    //����������
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
    unsigned char password[7] = {0};//����û����������
    unsigned char password_admin[7] = {0}, password_user[7] = {0};//���flash��ȡ���Ĺ���Ա���û�����
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
                else if(key_value == KEY_B)//ģ������
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
                                if(key_value == KEY_1)//����Ա�����趨
                                {
                                    Show_Enter_Password();
                                    key_value = Enter_Password(&key_flag_grp, password);
                                    if(key_value == KEY_D) break;

                                    W25Q128_Read_Data(PASSWORD_ADDR, flash_buf, 14);
                                    strcpy((char *)flash_buf, (const char *)password);
                                    //Ҫ���ٽ�α����ĺ�
                                    CPU_SR_ALLOC();
                                    //�����ٽ������������µĴ��룬�ر����жϣ�ֹͣ��ucos��������ȣ����������Ѿ�ִֹͣ��
                                    OS_CRITICAL_ENTER();
                                    W25Q128_Erase_Sector(PASSWORD_ADDR);
                                    W25Q128_Writer_Data(PASSWORD_ADDR, flash_buf, 14);
                                    //�˳��ٽ������������жϣ�����ucos���������
                                    OS_CRITICAL_EXIT();
                                    break;
                                }
                                else if(key_value == KEY_2)//�û������趨
                                {
                                    Show_Enter_Password();
                                    key_value = Enter_Password(&key_flag_grp, password);
                                    if(key_value == KEY_D) break;

                                    W25Q128_Read_Data(PASSWORD_ADDR, flash_buf, 14);
                                    strcpy((char *)(flash_buf + 7), (const char *)password);
                                    //Ҫ���ٽ�α����ĺ�
                                    CPU_SR_ALLOC();
                                    //�����ٽ������������µĴ��룬�ر����жϣ�ֹͣ��ucos��������ȣ����������Ѿ�ִֹͣ��
                                    OS_CRITICAL_ENTER();
                                    W25Q128_Erase_Sector(PASSWORD_ADDR);
                                    W25Q128_Writer_Data(PASSWORD_ADDR, flash_buf, 14);
                                    //�˳��ٽ������������жϣ�����ucos���������
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
                                OSMutexPend(&mutex_rfid, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                                Input_RFID_Card();
                                OSTimeDlyHMSM(0, 0, 2, 0, OS_OPT_TIME_HMSM_STRICT, &err); //��ʱ�������ͷ�
                                OSMutexPost(&mutex_rfid, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                            }
                            else if(key_value == KEY_2)
                            {
                                Show_Delete_Card();
                                OSMutexPend(&mutex_rfid, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                                Delete_Exist_Card();
                                OSMutexPost(&mutex_rfid, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                            }
                            else if(key_value == KEY_3)
                            {
                                OSMutexPend(&mutex_rfid, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                                Delete_All_Card();
                                OSMutexPost(&mutex_rfid, OS_OPT_POST_NONE, &err); //�ͷŻ�����
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
                                OSMutexPend(&mutex_fingerprint, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                                Add_FR();
                                OSMutexPost(&mutex_fingerprint, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                            }
                            else if(key_value == KEY_2)
                            {
                                OSMutexPend(&mutex_fingerprint, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                                Del_FR();
                                OSMutexPost(&mutex_fingerprint, OS_OPT_POST_NONE, &err); //�ͷŻ�����
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
                    OSTaskSemPost(&BuzzerTCB, OS_OPT_POST_NONE, &err); //���ͷ������������Ƕ�ź���

                    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                    My_OLED_CLR();
                    OLED_DrawBMP(0, 0, 41, 5, (unsigned char *)BMP2);
                    OLED_ShowCN(42, 0, "�������", 4);
                    OLED_ShowCN(42, 2, "�ɹ�", 2);
                    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                    break;
                }
                else
                {
                    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                    My_OLED_CLR();
                    OLED_ShowCN(42, 0, "�������", 4);
                    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                }
            }
        }
        else if(key_value == KEY_B)//ģ������
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
        OSMutexPend(&mutex_rfid, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        memset(card_numberbuf, 0, sizeof(card_numberbuf));
        if(Wait_RFID_Card() == 0)
        {
            if(Card_Already_Exist(rfid_buf, card_numberbuf))
            {
                OSTaskSemPost(&BuzzerTCB, OS_OPT_POST_NONE, &err); //���ͷ������������Ƕ�ź���

                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

                Gddram_Copy(gddram_temp, GDDRAM);//��������
                Unlock_Rest_OLED_Fill(0x00);
                OLED_DrawBMP(0, 0, 41, 5, (unsigned char *)BMP2);
                OLED_ShowStr(42, 0, (unsigned char *)"RFID", 2);
                OLED_ShowCN(42, 2, "�����ɹ�", 4);
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                Show_Previous_Interface();

                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            }
        }
        OSMutexPost(&mutex_rfid, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void FingerprintTask(void *p_arg)
{
    OS_ERR err;
    SearchResult search;

    while(1)
    {
        OSMutexPend(&mutex_fingerprint, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        if(Press_FR(&search))
        {
            OSTaskSemPost(&BuzzerTCB, OS_OPT_POST_NONE, &err); //���ͷ������������Ƕ�ź���

            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

            Gddram_Copy(gddram_temp, GDDRAM);//��������
            Unlock_Rest_OLED_Fill(0x00);
            OLED_DrawBMP(0, 0, 41, 5, (unsigned char *)BMP2);
            OLED_ShowCN(42, 0, "ָ�ƽ���", 4);
            OLED_ShowCN(42, 2, "�ɹ�", 2);
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
            Show_Previous_Interface();

            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        }
        OSMutexPost(&mutex_fingerprint, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void BluetoothTask(void *p_arg)
{
    OS_ERR err;
    OS_MSG_SIZE msg_size;
    u8 *msg;
    char password[6];//����������͹���������
    unsigned char i = 0, len = 0;
    unsigned char flash_buf[14] = {0};
    unsigned char password_admin[7] = {0}, password_user[7] = {0};//���flash��ȡ���Ĺ���Ա���û�����
    u8 buffer[64] = {0};//�����������

    while(1)
    {
        msg = OSTaskQPend(0, OS_OPT_PEND_BLOCKING, &msg_size, 0, &err);

        if(i == 64)
        {
            printf("�������ݹ�����������֮ǰ������\n");
            i = 0;
        }

        if(*msg != '#')
        {
            buffer[i++] =  *msg;
            continue;
        }
        i = 0;//��Ϊ0,��һ֡���ݴ�buffer[0]��ʼ�洢
        printf("msg_buffer = %s\n", buffer);

        //TODO���û����������������ӳ����жϣ����磺������ĸE�ǲ��Ϸ��ģ���Ϊû���������
        if(strstr((const char *)buffer, "BG1CHGUSER")) //���͸�ʽ��BG1CHGUSERXXXXXXE# ���ȣ�18
        {
            if(strlen((char *)buffer) > 18)
            {
                memset(buffer, 0, sizeof(buffer)); //���֮ǰ������������
                continue;
            }
            len = strlen("BG1CHGUSER");
            strncpy(password, (char *)(buffer + len), 6); //6λ����
            printf("password %s\n", password);
            char *tokPtr = strtok(password, "1234567890");//��������ֻ��������
            if(tokPtr != NULL)
            {
                printf("���벻�Ϸ�\n");
                continue;
            }
            len = strlen((char *)buffer);
            if(buffer[len - 1] != 'E')
            {
                printf("У��ʹ���\n");
                continue;
            }

            W25Q128_Read_Data(PASSWORD_ADDR, flash_buf, 14);
            strcpy((char *)(flash_buf + 7), password);
            //Ҫ���ٽ�α����ĺ�
            CPU_SR_ALLOC();
            //�����ٽ������������µĴ��룬�ر����жϣ�ֹͣ��ucos��������ȣ����������Ѿ�ִֹͣ��
            OS_CRITICAL_ENTER();
            W25Q128_Erase_Sector(PASSWORD_ADDR);
            W25Q128_Writer_Data(PASSWORD_ADDR, flash_buf, 14);
            //�˳��ٽ������������жϣ�����ucos���������
            OS_CRITICAL_EXIT();
        }
        if(strstr((const char *)buffer, "BG1CHGADMIN")) //���͸�ʽ��BG1CHGADMINXXXXXXF#	���ȣ�19
        {
            if(strlen((char *)buffer) > 19)
            {
                memset(buffer, 0, sizeof(buffer)); //���֮ǰ������������
                continue;
            }
            len = strlen("BG1CHGADMIN");
            strncpy(password, (char *)(buffer + len), 6); //6λ����
            printf("password %s\n", password);
            char *tokPtr = strtok(password, "1234567890");//��������ֻ��������
            if(tokPtr != NULL)
            {
                printf("���벻�Ϸ�\n");
                continue;
            }
            len = strlen((char *)buffer);
            if(buffer[len - 1] != 'F')
            {
                printf("У��ʹ���\n");
                continue;
            }

            W25Q128_Read_Data(PASSWORD_ADDR, flash_buf, 14);
            strcpy((char *)flash_buf, password);
            //Ҫ���ٽ�α����ĺ�
            CPU_SR_ALLOC();
            //�����ٽ������������µĴ��룬�ر����жϣ�ֹͣ��ucos��������ȣ����������Ѿ�ִֹͣ��
            OS_CRITICAL_ENTER();
            W25Q128_Erase_Sector(PASSWORD_ADDR);
            W25Q128_Writer_Data(PASSWORD_ADDR, flash_buf, 14);
            //�˳��ٽ������������жϣ�����ucos���������
            OS_CRITICAL_EXIT();
        }
        if(strstr((const char *)buffer, "BG1OPEN")) //���͸�ʽ��BG1OPENXXXXXXB#	���ȣ�15
        {
            if(strlen((char *)buffer) > 15)
            {
                memset(buffer, 0, sizeof(buffer)); //���֮ǰ������������
                continue;
            }

            len = strlen("BG1OPEN");
            strncpy(password, (char *)(buffer + len), 6); //6λ����
            printf("password %s\n", password);
            len = strlen((char *)buffer);
            if(buffer[len - 1] != 'B')
            {
                printf("У��ʹ���\n");
                continue;
            }

            W25Q128_Read_Data(PASSWORD_ADDR, password_admin, 7);
            W25Q128_Read_Data(PASSWORD_ADDR + 7, password_user, 7);

            if(strcmp((const char *)password_admin, password) == 0
                    || strcmp((const char *)password_user, password) == 0)
            {
                OSTaskSemPost(&BuzzerTCB, OS_OPT_POST_NONE, &err); //���ͷ������������Ƕ�ź���

                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

                Gddram_Copy(gddram_temp, GDDRAM);//��������
                Unlock_Rest_OLED_Fill(0x00);
                OLED_DrawBMP(0, 0, 41, 5, (unsigned char *)BMP2);
                OLED_ShowCN(42, 0, "�ֻ�����", 4);
                OLED_ShowCN(42, 2, "�ɹ�", 2);
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                Show_Previous_Interface();

                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            }
            else
            {
                printf("��������ʧ�ܣ���������\n");
            }
        }
        memset(buffer, 0, sizeof(buffer)); //���֮ǰ������������
        OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void BuzzerTask(void *p_arg)
{
    OS_ERR err;

    while(1)
    {
        OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, 0, &err); //�ȴ��Լ�����Ƕ�ź�����Ĭ�ϳ�ֵΪ0
        BEEP_ON;
        OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_HMSM_STRICT, &err);
        BEEP_OFF;
    }
}


