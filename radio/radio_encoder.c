/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-22     Rick       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include "radio_encoder.h"
#include "radio_app.h"

#define DBG_TAG "RADIO_ENCODER"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static rt_mq_t rf_en_mq;
static rt_thread_t rf_encode_t = RT_NULL;
static struct rt_completion rf_txdone_sem;

uint32_t Self_Id = 0;
#ifdef MINI
uint32_t Self_Default_Id = 20000002;
#else
uint32_t Self_Default_Id = 20000001;
#endif

char radio_send_buf[255];

typedef struct
{
    uint32_t Taget_Id;
    uint8_t Counter;
    uint8_t Command;
    uint8_t Data;
}Radio_Normal_Format;

void RadioEnqueue(uint32_t Taget_Id, uint8_t Counter, uint8_t Command, uint8_t Data)
{
    Radio_Normal_Format Send_Buf;
    Send_Buf.Taget_Id = Taget_Id;
    Send_Buf.Counter = Counter;
    Send_Buf.Command = Command;
    Send_Buf.Data = Data;

    LOG_D("Send command:%d value:%d ID:%d\r\n",Command,Data,Taget_Id);
    rt_mq_send(rf_en_mq, &Send_Buf, sizeof(Radio_Normal_Format));
    rt_pm_module_delay_sleep(PM_RF_ID, 3000);
}

void SendPrepare(Radio_Normal_Format Send)
{
    rt_memset(radio_send_buf, 0, sizeof(radio_send_buf));
    uint8_t check = 0;
    Send.Counter++ <= 255 ? Send.Counter : 0;
    rt_sprintf(radio_send_buf, "{%08ld,%08ld,%03d,%02d,%d}", Send.Taget_Id, Self_Id, Send.Counter, Send.Command, Send.Data);
    for (uint8_t i = 0; i < 28; i++)
    {
        check += radio_send_buf[i];
    }
    radio_send_buf[28] = ((check >> 4) < 10) ? (check >> 4) + '0' : (check >> 4) - 10 + 'A';
    radio_send_buf[29] = ((check & 0xf) < 10) ? (check & 0xf) + '0' : (check & 0xf) - 10 + 'A';
    radio_send_buf[30] = '\r';
    radio_send_buf[31] = '\n';
}

void Reponse_Before(uint8_t command,uint8_t data)
{
    switch(command)
    {
    case 2://heart
        PVD_Open();
        Start_Heart_Timer();
        break;
    case 4://water
        if(data)
        {
            Start_Warn_Water_Timer();
        }
        break;
    default:
        break;
    }
}
void Reponse_After(uint8_t command,uint8_t data)
{
    switch(command)
    {
    case 2://heart
        PVD_Close();
        break;
    default:
        break;
    }
}

void rf_txdone_callback(void)
{
    rt_completion_done(&rf_txdone_sem);
}

void rf_encode_entry(void *paramaeter)
{
    Radio_Normal_Format Send_Data;
    while (1)
    {
        if (rt_mq_recv(rf_en_mq,&Send_Data, sizeof(Radio_Normal_Format), RT_WAITING_FOREVER) == RT_EOK)
        {

            rt_pm_module_delay_sleep(PM_RF_ID, 3000);
            /*
             * Clear RF Flag
             */
            rt_completion_init(&rf_txdone_sem);
            /*
             * Start RF Send
             */
            SendPrepare(Send_Data);
            Reponse_Before(Send_Data.Command,Send_Data.Data);
            RF_Send(radio_send_buf, rt_strlen(radio_send_buf));
            Reponse_After(Send_Data.Command,Send_Data.Data);
            /*
             * Wait RF TxDone
             */
            rt_completion_wait(&rf_txdone_sem,200);
        }
    }
}

void RadioQueue_Init(void)
{
    int *p;
    p=(int *)(0x0803FFF0);//这就是已知的地址，要强制类型转换
    Self_Id = *p;//从Flash加载ID
    if (Self_Id == 0xFFFFFFFF || Self_Id == 0)
    {
        Self_Id = Self_Default_Id;
    }
    LOG_I("Self ID is %d\r\n", Self_Id);

    rf_en_mq = rt_mq_create("rf_en_mq", sizeof(Radio_Normal_Format), 10, RT_IPC_FLAG_PRIO);
    rf_encode_t = rt_thread_create("radio_send", rf_encode_entry, RT_NULL, 1024, 9, 10);
    if (rf_encode_t)rt_thread_startup(rf_encode_t);
}

uint32_t Get_Self_ID(void)
{
    return Self_Id;
}

uint8_t Get_Factory_Self_ID(void)
{
    if(Self_Id == 20000000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
