#include "password.h"
#include "OLED_I2C.h"
#include "delay.h"
#include "spiflash.h"

extern OS_MUTEX mutex_oled;		//�������Ķ���

//��ʾ�����������
void Show_Enter_Password(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();
    OLED_ShowCN(42, 0, "����������", 5);

    OLED_ShowStr(42, 2, (unsigned char *)"(", 2);
    OLED_ShowCN(50, 2, "��λ", 2);
    OLED_ShowStr(82, 2, (unsigned char *)")", 2);
    OLED_ShowStr(90, 2, (unsigned char *)":", 2);

    OLED_ShowStr(20, 6, (unsigned char *)"C.", 2);
    OLED_ShowCN(36, 6, "ȷ��", 2);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "����", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}

//���ܣ���ʾ�û���������롣����A--�˸񣻰���C--ȷ�ϣ�����D--����
//������flag_grp�������¼���־��
//		password������û����������
unsigned char Enter_Password(OS_FLAG_GRP *flag_grp, unsigned char *password)
{
    unsigned char i = 0;
    unsigned char key_value = 255;
    OS_ERR err;

    while(1)
    {
        key_value = GetKeyValue(flag_grp);
        if(key_value == 255)
            continue;
        if(key_value == KEY_A)
        {
            if(i == 0)
                continue;
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            OLED_ShowChar(42 + (--i) * 8, 4, ' ', 16);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            continue;
        }
        if(key_value == KEY_D)
        {
            return key_value;
        }
        if(key_value == KEY_C)
        {
            if(i == 6)//ֻ�е����������λ���룬���Ұ���C��ʱ���Ż��˳�����
                break;
            continue;
        }

        if(i < 6)
        {
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
            OLED_ShowChar(42 + i * 8, 4, key_value, 16);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
            password[i++] = key_value;
        }
    }
    return 0;
}

void Show_Password_Setting(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "����Ա", 4);

    OLED_ShowStr(42, 2, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 2, "�û�", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "����", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
}

//���ܣ���֤�û�����������Ƿ�Ϊ����Ա����
bool Password_validation(OS_FLAG_GRP *flag_grp, unsigned char *password)
{
    OS_ERR err;
    unsigned char password_admin[7] = {0};//���flash��ȡ���Ĺ���Ա���û�����

    W25Q128_Read_Data(PASSWORD_ADDR, password_admin, 7);
    if(strcmp((const char *)password_admin, (const char *)password) == 0)
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "������ȷ", 4);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
        return true;
    }
    else
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //�����ȴ�������
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "�������", 4);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //�ͷŻ�����
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
        return false;
    }
}
