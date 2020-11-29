#include "RFID.h"
#include <includes.h>

extern OS_MUTEX mutex_oled;		//�������Ķ���

//MFRC522������
u8 card_typebuf[2];
u8 card_numberbuf[7] = {0};
u8 card_keyAbuf[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

u8 rfid_buf[4096] = {0};
u8 rfid_buf_temp[4096] = {0};
u8 rfid_number = 0;

void Show_RFID_Management(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "¼�뿨", 4);

    OLED_ShowStr(42, 2, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 2, "ɾһ�ſ�", 4);

    OLED_ShowStr(42, 4, (unsigned char *)"3.", 2);
    OLED_ShowCN(58, 4, "ɾ���п�", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "����", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}

void Show_RFID_Input(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();

    OLED_ShowCN(42, 0, "��ˢҪ¼��", 5);
    OLED_ShowCN(42, 2, "�Ŀ�", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}

void Show_Delete_Card(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();

    OLED_ShowCN(42, 0, "��ˢҪɾ��", 5);
    OLED_ShowCN(42, 2, "�Ŀ�", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}

//���ܣ��ȴ��û�ˢ��������ȡ�����������
//����ֵ��0--�ж�����Ƭ
//		  -1--�����к�У���д���
int Wait_RFID_Card(void)
{
    u8 i, status, card_size, buffer = 0;

    MFRC522_Initializtion();
    status = MFRC522_Request(0x52, card_typebuf);			//Ѱ��

    if(status == 0)		//���������
    {
        status = MFRC522_Anticoll(card_numberbuf);			//��ײ����
        card_size = MFRC522_SelectTag(card_numberbuf);		//ѡ��
        status = MFRC522_Auth(0x60, 4, card_keyAbuf, card_numberbuf);	//�鿨

        //��������ʾ
        printf("������ = %d%d\n", card_typebuf[0], card_typebuf[1]);

        printf("�������� = %d\n", card_size);

        //�����к���ʾ�����һ�ֽ�Ϊ����У����
        printf("�����к� = ");
        for(i = 0; i < 4; i++)
        {
            buffer  ^=	card_numberbuf[i];	//��У����
            printf("%X", card_numberbuf[i]);
        }
        printf("\n");

        if(buffer != card_numberbuf[4])
            return -1;
    }

    return status;
}

//���ܣ�����¼��RFID�������ݺ�RFID��������д����Ӧ��flash��
//������rfid_buf--���RFID�����ݵĵ�ַ
//		rfid_number--RFID��������
void RFID_Write_Flash(u8 *rfid_buf, u8 rfid_number)
{
    unsigned int len = 0;
    u8 j;

    len = strlen((char *)rfid_buf);
    printf("rfid_buf_len = %d\n", len);
    //Ҫ���ٽ�α����ĺ�
    CPU_SR_ALLOC();
    //�����ٽ������������µĴ��룬�ر����жϣ�ֹͣ��ucos��������ȣ����������Ѿ�ִֹͣ��
    OS_CRITICAL_ENTER();
    W25Q128_Erase_Sector(RFID_CARD_ADDR);
    for(j = 0; len > 0; len -= 255, j++)
    {
        if(len <= 256)
        {
            W25Q128_Writer_Data(RFID_CARD_ADDR + 256 * j, rfid_buf + 256 * j, len);
            break;
        }
        //�������256���ֽڣ���д����һ��ҳ
        W25Q128_Writer_Data(RFID_CARD_ADDR + 256 * j, rfid_buf + 256 * j, 256);
    }

    //дRFID������
    W25Q128_Erase_Sector(RFID_NUMBER_ADDR);
    W25Q128_Writer_Byte(RFID_NUMBER_ADDR, rfid_number);

    //�˳��ٽ������������жϣ�����ucos���������
    OS_CRITICAL_EXIT();
}

//���ܣ�¼���µ�RFID��Ƭ�����µĿ�Ƭid�浽flash��
//����ֵ��0--�ɹ�¼���¿�
//		  -1--���Ѿ�����
//		  -2--����Ĭ������¼�뿨Ƭ����������
int Input_RFID_Card(void)
{
    OS_ERR err;

    while(Wait_RFID_Card() != 0) OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

    W25Q128_Read_Data(RFID_CARD_ADDR, rfid_buf, strlen((char *)rfid_buf));
    if(Card_Already_Exist(rfid_buf, card_numberbuf))
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "�ÿ��Ѵ���", 5);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
        return -1;
    }

    card_numberbuf[5] = ' ';//�ո���Ϊ�ָ���
    if(strlen((char *)rfid_buf) + strlen((char *)card_numberbuf) >= sizeof(rfid_buf))
    {
        printf("�Ѿ���������¼�뿨Ƭ����������\n");
        return -2;
    }
    strcat((char *)rfid_buf, (char *)card_numberbuf);
    rfid_number++;
    RFID_Write_Flash(rfid_buf, rfid_number);
    //	W25Q128_Read_Data(RFID_CARD_ADDR, rfid_buf, strlen((char *)rfid_buf));
    //	printf("rfid:%s card:%s\n", rfid_buf, card_numberbuf);

    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
    My_OLED_CLR();
    OLED_ShowCN(42, 0, "¼��ɹ�", 4);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);

    return 0;
}

//���ܣ��жϼ���Ҫ¼��Ŀ�Ƭ�Ƿ��Ѿ�¼���
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

//���ܣ���flashɾ��һ�Ŵ��ڵĿ�
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
                strcpy(token_temp, tokenPtr);//teokenPtr��Ҫ�޸���
                strcat(token_temp, " ");
                strcat((char *)rfid_buf, token_temp);
            }
            tokenPtr = strtok(NULL, " ");
        }

        rfid_number--;
        RFID_Write_Flash(rfid_buf, rfid_number);

        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "��ɾһ�ſ�", 5);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
    else
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "�ÿ�������", 5);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

void Delete_All_Card(void)
{
    OS_ERR err;
    memset(rfid_buf, 0, sizeof(rfid_buf));
    rfid_number = 0;

    //Ҫ���ٽ�α����ĺ�
    CPU_SR_ALLOC();
    //�����ٽ������������µĴ��룬�ر����жϣ�ֹͣ��ucos��������ȣ����������Ѿ�ִֹͣ��
    OS_CRITICAL_ENTER();
    W25Q128_Erase_Sector(RFID_CARD_ADDR);
    W25Q128_Erase_Sector(RFID_NUMBER_ADDR);
    W25Q128_Writer_Byte(RFID_NUMBER_ADDR, rfid_number);
    //�˳��ٽ������������жϣ�����ucos���������
    OS_CRITICAL_EXIT();

    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
    My_OLED_CLR();
    OLED_ShowCN(42, 0, "��ɾ���п�", 5);
    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
    OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}
