#include "fingerprint.h"
#include "as608.h"
#include "delay.h"

SysPara AS608Para;//ָ��ģ��AS608����
u16 ValidN;//ģ������Чָ�Ƹ���
u8 FingerID_buf[1200] = {0};
u8 FingerID_buf_temp[1200] = {0};
u16 Fingerprint_data_len = 0;//ָ�����ݳ���

extern OS_MUTEX mutex_oled;		//�������Ķ���
extern OS_FLAG_GRP	key_flag_grp;	//�¼���־��Ķ���

void Show_Fingerprint_Management(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "¼��ָ��", 4);

    OLED_ShowStr(42, 2, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 2, "ɾ��ָ��", 4);

    OLED_ShowStr(42, 4, (unsigned char *)"3.", 2);
    OLED_ShowCN(58, 4, "�鿴ָ��", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "����", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}

void Show_Press_Fingerprint(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
    My_OLED_CLR();
    OLED_ShowCN(42, 0, "�밴ָ��", 4);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}

void Show_Fingerprint_Normal(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
    My_OLED_CLR();
    OLED_ShowCN(42, 0, "ָ������", 4);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}

void Show_Del_Fingerprint(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "ɾָ��", 3);

    OLED_ShowStr(42, 2, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 2, "ɾ����", 3);

    OLED_ShowStr(42, 4, (unsigned char *)"3.", 2);
    OLED_ShowCN(58, 4, "ˢָ��ɾ", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "����", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}

//���ܣ���ȡ�û������id��
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
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            OLED_ShowChar(42 + (--j) * 8, 3, ' ', 16);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            continue;
        }
        if(key_value == KEY_SHIFT_3 || key_value == KEY_SHIFT_8)
            continue;

        id *= 10;
        id += (key_value - '0');

        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        OLED_ShowChar(42 + j * 8, 3, key_value, 16);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        j++;
    }
}

//���ܣ���ָ��ID���ݺͳ���д��Ĭ�ϵ�flash��
//������FingerID_buf--���ָ�����ݵĵ�ַ
void Fingerprint_Write_Flash(u8 *FingerID_buf)
{
    unsigned short len = 0;
    unsigned char len_t = 0;
    u8 j;

    len = strlen((char *)FingerID_buf);
    printf("len = %d\n", len);
    //Ҫ���ٽ�α����ĺ�
    CPU_SR_ALLOC();
    //�����ٽ������������µĴ��룬�ر����жϣ�ֹͣ��ucos��������ȣ����������Ѿ�ִֹͣ��
    OS_CRITICAL_ENTER();
    W25Q128_Erase_Sector(FINGERPRINT_ADDR);
    for(j = 0; len > 0; len -= 255, j++)
    {
        if(len <= 256)
        {
            W25Q128_Writer_Data(FINGERPRINT_ADDR + 256 * j, FingerID_buf + 256 * j, len);
            break;
        }
        //�������256���ֽڣ���д����һ��ҳ
        W25Q128_Writer_Data(FINGERPRINT_ADDR + 256 * j, FingerID_buf + 256 * j, 256);
    }

    W25Q128_Erase_Sector(FINGERPRINT_LEN_ADDR);
    if(len > 255)//ָ�Ƴ����������ֽڵģ�����Ҫ�ֿ��洢
    {
        len_t = len >> 8;
        W25Q128_Writer_Byte(FINGERPRINT_LEN_ADDR, len_t);
    }
    else
    {
        W25Q128_Writer_Byte(FINGERPRINT_LEN_ADDR, 0x00);
    }

    W25Q128_Writer_Byte(FINGERPRINT_LEN_ADDR + 1, (u8)len);

    //�˳��ٽ������������жϣ�����ucos���������
    OS_CRITICAL_EXIT();
}

//���ܣ��ӻ�������ɾ��ָ����ָ��id
//������Ҫɾ����ָ��id
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
            strcpy(token_temp, tokenPtr);//teokenPtr��Ҫ�޸���
            strcat(token_temp, " ");
            strcat((char *)FingerID_buf, token_temp);
        }
        tokenPtr = strtok(NULL, " ");
    }
}

//���ܣ��ж�ָ��id�Ƿ��Ѿ��ж�Ӧ����Чָ����
//������FingerID_buf--���ָ��id�Ļ�����
//		id--Ҫ�жϵ�ָ��id
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

//��ʾȷ���������Ϣ
void ShowErrMessage(u8 ensure)
{
    printf("%s\n", (u8 *)EnsureMessage(ensure));
}
//¼ָ��
void Add_FR(void)
{
    OS_ERR err;
    u8 ensure, i = 0, processnum = 0;
    u16 id = 0;
    unsigned char surplus[4] = {0}; //ʣ��������ʾ
    char temp[5] = {0};

    if(ValidN == 300)
    {
        printf("�Ѿ�����ָ�ƿ��������ޣ��޷�����¼��ָ��\n");
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
                ensure = PS_GenChar(CharBuffer1); //��������
                if(ensure == 0x00)
                {
                    Show_Fingerprint_Normal();
                    i = 0;
                    processnum = 1; //�����ڶ���
                }
                else ShowErrMessage(ensure);
            }
            else ShowErrMessage(ensure);
            break;

        case 1:
            i++;
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            OLED_ShowCN(42, 0, "���ٰ�һ��", 5);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

            ensure = PS_GetImage();
            if(ensure == 0x00)
            {
                ensure = PS_GenChar(CharBuffer2); //��������
                if(ensure == 0x00)
                {
                    Show_Fingerprint_Normal();
                    i = 0;
                    processnum = 2; //����������
                }
                else ShowErrMessage(ensure);
            }
            else ShowErrMessage(ensure);
            break;

        case 2:
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "�Ա�����", 4);
            OLED_ShowCN(42, 2, "ָ��", 2);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

            ensure = PS_Match();
            if(ensure == 0x00)
            {
                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                My_OLED_CLR();
                OLED_ShowCN(42, 0, "�Աȳɹ�", 4);
                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                processnum = 3; //�������Ĳ�
            }
            else
            {
                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                My_OLED_CLR();
                OLED_ShowCN(42, 0, "�Ա�ʧ��", 4);
                OLED_ShowCN(42, 2, "����¼ָ��", 5);
                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                ShowErrMessage(ensure);
                i = 0;
                processnum = 0; //���ص�һ��
            }
            delay_ms(1200);
            break;

        case 3:
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "����ָ��", 4);
            OLED_ShowCN(42, 2, "ģ��", 2);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

            ensure = PS_RegModel();
            if(ensure == 0x00)
            {
                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                My_OLED_CLR();
                OLED_ShowCN(42, 0, "����ָ��", 4);
                OLED_ShowCN(42, 2, "ģ��ɹ�", 4);
                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
                processnum = 4; //�������岽
            }
            else
            {
                processnum = 0;
                ShowErrMessage(ensure);
            }
            delay_ms(1200);
            break;

        case 4:
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "�����봢��", 5);
            OLED_ShowStr(42, 2, (unsigned char *)"ID(0=<ID<=299)", 1);
            OLED_ShowStr(20, 6, (unsigned char *)"A.", 2);
            OLED_ShowCN(36, 6, "ɾ��", 2);
            OLED_ShowStr(80, 6, (unsigned char *)"C.", 2);
            OLED_ShowCN(96, 6, "ȷ��", 2);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

            W25Q128_Read_Data(FINGERPRINT_ADDR, FingerID_buf, strlen((char *)FingerID_buf));
            id = Get_Input_ID();
            //TODO��������Լ�һ���жϣ����û������id�Ƿ��Ӧ����Чָ��
            sprintf(temp, "%u ", id);
            strcat((char *)FingerID_buf, temp);
            Fingerprint_Write_Flash(FingerID_buf);
            printf("key_value:%d\n", id);

            ensure = PS_StoreChar(CharBuffer2, id); //����ģ��
            if(ensure == 0x00)
            {
                PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���

                OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
                My_OLED_CLR();
                OLED_ShowCN(42, 0, "¼��ɹ�", 4);
                OLED_ShowCN(42, 2, "ʣ������Ϊ", 5);
                sprintf((char *)surplus, "%d", AS608Para.PS_max - ValidN);
                OLED_ShowStr(42, 4, surplus, 2);
                OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
                OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

                printf("ʣ������Ϊ %d\n", AS608Para.PS_max - ValidN); //��ʾʣ��ָ������
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
        if(i == 5) //����5��û�а���ָ���˳�
        {
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "�������û", 5);
            OLED_ShowCN(42, 2, "����ָ����", 5);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
            break;
        }
    }
}

//ˢָ��
bool Press_FR(SearchResult *search)
{
    u8 ensure;

    ensure = PS_GetImage();
    if(ensure == 0x00) //��ȡͼ��ɹ�
    {
        ensure = PS_GenChar(CharBuffer1);
        if(ensure == 0x00) //���������ɹ�
        {
            ensure = PS_HighSpeedSearch(CharBuffer1, 0, AS608Para.PS_max, search);
            if(ensure == 0x00) //�����ɹ�
            {
                printf("ȷ�д���,ID:%d  ƥ��÷�:%d", search->pageID, search->mathscore);
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

//ɾ��ָ��
void Del_FR(void)
{
    OS_ERR err;
    u8  ensure, key_value = 255;
    u16 id;
    SearchResult search;
    unsigned char surplus[4] = {0}; //ʣ��������ʾ

    Show_Del_Fingerprint();
    key_value = GetKeyValue(&key_flag_grp);
    if(key_value == KEY_1)
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "������ָ��", 5);
        OLED_ShowStr(42, 2, (unsigned char *)"ID(0=<ID<=299)", 1);
        OLED_ShowStr(20, 6, (unsigned char *)"A.", 2);
        OLED_ShowCN(36, 6, "ɾ��", 2);
        OLED_ShowStr(80, 6, (unsigned char *)"C.", 2);
        OLED_ShowCN(96, 6, "ȷ��", 2);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

        id = Get_Input_ID(); //��ȡ�������ֵ
        if(Fingerprint_Already_Exist(FingerID_buf, id))
        {
            ensure = PS_DeletChar(id, 1); //ɾ������ָ��
            Del_Fingerprint_ID(id);
            Fingerprint_Write_Flash(FingerID_buf);
        }
        else
        {
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "ָ�Ʋ�����", 5);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
            return;
        }
    }
    else if(key_value == KEY_2)
    {
        ensure = PS_Empty(); //���ָ�ƿ�
        memset(FingerID_buf, 0, sizeof(FingerID_buf));
        Fingerprint_Write_Flash(FingerID_buf);
    }
    else if(key_value == KEY_3)
    {
        Show_Press_Fingerprint();
        if(Press_FR(&search))
        {
            ensure = PS_DeletChar(search.pageID, 1); //ɾ������ָ��
            Del_Fingerprint_ID(search.pageID);
            Fingerprint_Write_Flash(FingerID_buf);
        }
        else
        {
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            My_OLED_CLR();
            OLED_ShowCN(42, 0, "ָ�Ʋ�����", 5);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
        }
    }

    if(ensure == 0)
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "ɾָ�Ƴɹ�", 5);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
    }
    else
        ShowErrMessage(ensure);

    PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
    OLED_ShowCN(42, 2, "ʣ������Ϊ", 5);
    sprintf((char *)surplus, "%d", AS608Para.PS_max - ValidN);
    OLED_ShowStr(42, 4, surplus, 2);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    printf("��ָ�Ƹ��� %d\n", AS608Para.PS_max - ValidN);
}

//���ܣ���ʾ�Ѿ�¼���ָ�ƣ�������ʾ3����¼ʾ��
void View_Exist_Fingerprint(void)
{
    OS_ERR err;
    unsigned char temp[3] = {0};
    u8 i = 0, j = 0;

    strcpy((char *)FingerID_buf_temp, (char *)FingerID_buf);
    printf("499:%s\n", FingerID_buf);
    char *tokenPtr = strtok((char *)FingerID_buf_temp, " ");

    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
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
    OLED_ShowCN(96, 6, "����", 2);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}

void Get_Fingerprint_Module_Parameters(void)
{
    u8 ensure;

    PS_StaGPIO_Init();	//��ʼ��FR��״̬����

    while(PS_HandShake(&AS608Addr))//��AS608ģ������
    {
        delay_ms(400);
        printf("δ��⵽ģ��!!!");
        delay_ms(800);
        printf("��������ģ��...");
    }
    printf("ͨѶ�ɹ�!!!");
    printf("������:%d   ��ַ:%x", 57600, AS608Addr);

    ensure = PS_ValidTempleteNum(&ValidN); //����ָ�Ƹ���
    if(ensure != 0x00)
        ShowErrMessage(ensure);//��ʾȷ���������Ϣ
    ensure = PS_ReadSysPara(&AS608Para); //������
    if(ensure == 0x00)
    {
        printf("������:%d     �Աȵȼ�: %d", AS608Para.PS_max - ValidN, AS608Para.PS_level);
    }
    else
        ShowErrMessage(ensure);
}
