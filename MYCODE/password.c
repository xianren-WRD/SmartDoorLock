#include "password.h"
#include "OLED_I2C.h"
#include "delay.h"
#include "spiflash.h"

extern OS_MUTEX mutex_oled;		//互斥量的对象

//显示密码输入界面
void Show_Enter_Password(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();
    OLED_ShowCN(42, 0, "请输入密码", 5);

    OLED_ShowStr(42, 2, (unsigned char *)"(", 2);
    OLED_ShowCN(50, 2, "六位", 2);
    OLED_ShowStr(82, 2, (unsigned char *)")", 2);
    OLED_ShowStr(90, 2, (unsigned char *)":", 2);

    OLED_ShowStr(20, 6, (unsigned char *)"C.", 2);
    OLED_ShowCN(36, 6, "确认", 2);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "返回", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}

//功能：显示用户输入的密码。按键A--退格；按键C--确认；按键D--返回
//参数：flag_grp：按键事件标志组
//		password：存放用户输入的密码
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
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            OLED_ShowChar(42 + (--i) * 8, 4, ' ', 16);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            continue;
        }
        if(key_value == KEY_D)
        {
            return key_value;
        }
        if(key_value == KEY_C)
        {
            if(i == 6)//只有当输入的是六位密码，并且按下C键时，才会退出函数
                break;
            continue;
        }

        if(i < 6)
        {
            OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
            OLED_ShowChar(42 + i * 8, 4, key_value, 16);
            OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
            password[i++] = key_value;
        }
    }
    return 0;
}

void Show_Password_Setting(void)
{
    OS_ERR err;
    OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量

    My_OLED_CLR();

    OLED_ShowStr(42, 0, (unsigned char *)"1.", 2);
    OLED_ShowCN(58, 0, "管理员", 4);

    OLED_ShowStr(42, 2, (unsigned char *)"2.", 2);
    OLED_ShowCN(58, 2, "用户", 4);

    OLED_ShowStr(80, 6, (unsigned char *)"D.", 2);
    OLED_ShowCN(96, 6, "返回", 2);

    OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
}

//功能：验证用户输入的密码是否为管理员密码
bool Password_validation(OS_FLAG_GRP *flag_grp, unsigned char *password)
{
    OS_ERR err;
    unsigned char password_admin[7] = {0};//存放flash读取到的管理员和用户密码

    W25Q128_Read_Data(PASSWORD_ADDR, password_admin, 7);
    if(strcmp((const char *)password_admin, (const char *)password) == 0)
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "密码正确", 4);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
        return true;
    }
    else
    {
        OSMutexPend(&mutex_oled, 0, OS_OPT_PEND_BLOCKING, NULL, &err); //阻塞等待互斥量
        My_OLED_CLR();
        OLED_ShowCN(42, 0, "密码错误", 4);
        OSMutexPost(&mutex_oled, OS_OPT_POST_NONE, &err); //释放互斥量
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
        return false;
    }
}
