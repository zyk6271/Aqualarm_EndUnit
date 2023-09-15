/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-25     Rick       the first version
 */
#include <rtthread.h>
#include <stdint.h>
#include "pin_config.h"
#include "key.h"
#include "button.h"
#include "status.h"

#define DBG_TAG "key"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

rt_thread_t button_t = RT_NULL;
rt_thread_t key_response_t = RT_NULL;

Button_t Key_OFF;
Button_t Key_ON;

rt_event_t Key_Event;

uint8_t KEY_OFF_Long_Counter,KEY_OFF_OnceFlag;
uint8_t KEY_ON_Long_Counter,KEY_ON_OnceFlag;

#define KEY_OFF_EVENT           1<<0
#define KEY_OFF_LONG_EVENT      1<<1
#define KEY_ON_EVENT            1<<2
#define KEY_ON_LONG_EVENT       1<<3

#ifndef MINI
uint8_t Read_ON_Level(void)
{
    return rt_pin_read(KEY_ON_PIN);
}
void KEY_ON_Press(void *parameter) //on
{
    rt_pm_module_delay_sleep(PM_BUTTON_ID, 3000);
    LOG_D("KEY_ON_Press\r\n");
    rt_event_send(Key_Event,KEY_ON_EVENT);
}
void KEY_ON_Long(void *parameter) //on
{
    rt_pm_module_delay_sleep(PM_BUTTON_ID, 3000);
    if (KEY_ON_OnceFlag == 0)
    {
        if (KEY_ON_Long_Counter > 3)
        {
            KEY_ON_OnceFlag = 1;
            rt_event_send(Key_Event,KEY_ON_LONG_EVENT);
            LOG_D("Key_ON is Long\r\n");
        }
        else
        {
            KEY_ON_Long_Counter++;
        }
    }
}
void KEY_ON_Long_Free(void *parameter) //on
{
    rt_pm_module_delay_sleep(PM_BUTTON_ID, 3000);
    KEY_ON_OnceFlag = 0;
    KEY_ON_Long_Counter = 0;
    LOG_D("KEY_ON_Long_Free\r\n");
}
#endif
uint8_t Read_OFF_Level(void)
{
    return rt_pin_read(KEY_OFF_PIN);
}
void KEY_OFF_Press(void *parameter) //off
{
    rt_pm_module_delay_sleep(PM_BUTTON_ID, 3000);
    LOG_D("KEY_OFF_Press\r\n");
    rt_event_send(Key_Event,KEY_OFF_EVENT);
}

void KEY_OFF_Long(void *parameter) //off
{
    rt_pm_module_delay_sleep(PM_BUTTON_ID, 3000);
    if (KEY_OFF_OnceFlag == 0)
    {
        if (KEY_OFF_Long_Counter > 3)
        {
            KEY_OFF_OnceFlag = 1;
            rt_event_send(Key_Event,KEY_OFF_LONG_EVENT);
            LOG_D("Key_OFF is Long\r\n");
        }
        else
        {
            KEY_OFF_Long_Counter++;
        }
    }
}
void KEY_OFF_Long_Free(void *parameter) //off
{
    rt_pm_module_delay_sleep(PM_BUTTON_ID, 3000);
    KEY_OFF_OnceFlag = 0;
    KEY_OFF_Long_Counter = 0;
    LOG_D("KEY_OFF_Long_Free\r\n");
}

void button_task_entry(void *parameter)
{
#ifndef MINI
    rt_pin_mode(KEY_ON_PIN, PIN_MODE_INPUT);
    Button_Create("ON", &Key_ON, Read_ON_Level, 0);
    Button_Attach(&Key_ON, BUTTON_DOWM, KEY_ON_Press);
    Button_Attach(&Key_ON, BUTTON_LONG, KEY_ON_Long);
    Button_Attach(&Key_ON, BUTTON_LONG_FREE, KEY_ON_Long_Free);
#endif
    rt_pin_mode(KEY_OFF_PIN, PIN_MODE_INPUT);
    Button_Create("OFF", &Key_OFF, Read_OFF_Level, 0);
    Button_Attach(&Key_OFF, BUTTON_DOWM, KEY_OFF_Press);
    Button_Attach(&Key_OFF, BUTTON_LONG, KEY_OFF_Long);
    Button_Attach(&Key_OFF, BUTTON_LONG_FREE, KEY_OFF_Long_Free);
    while (1)
    {
        Button_Process();
        rt_thread_mdelay(10);
    }
}
void Key_Reponse_Callback(void *parameter)
{
    rt_uint32_t ret;
    while(1)
    {
        if (rt_event_recv(Key_Event, KEY_OFF_EVENT | KEY_OFF_LONG_EVENT | KEY_ON_EVENT | KEY_ON_LONG_EVENT,RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,RT_WAITING_FOREVER, &ret) == RT_EOK)
        {
            rt_pm_module_delay_sleep(PM_BUTTON_ID, 3000);
            if(ret & KEY_OFF_EVENT)//OFF
            {
#ifdef MINI
                if(Get_ValveStore())
                {
#endif
                    Led_KeyOff();
                    switch(Get_Warning_Status())
                    {
                    case WaterAlarmActive:
                        if(Get_WaterAlarm())
                        {
                            Led_Alarm_DisableBeep();
                        }
                        else
                        {
                            RF_Close_Valve();
                            Warning_Disable(WaterAlarmActive);
                        }
                        break;
                    default:
                        RF_Close_Valve();
                        //send off
                        break;
                    }
#ifdef MINI
                }
                else
                {
                    switch(Get_Warning_Status())
                    {
                    case WaterAlarmActive:
                        break;
                    default:
                        Led_KeyOn();
                        RF_Open_Valve();
                        break;
                    }
                }
#endif
            }
            else if(ret & KEY_OFF_LONG_EVENT)//OFF LONG
            {
                RF_Learn_Request();
                //Self Learn
            }
#ifndef MINI
            else if(ret & KEY_ON_EVENT)//ON
            {
                switch(Get_Warning_Status())
                {
                case WaterAlarmActive:
                    break;
                default:
                    Led_KeyOn();
                    RF_Open_Valve();
                    //send on
                    break;
                }
            }
            else if(ret & KEY_ON_LONG_EVENT)//ON LONG
            {
                RF_Request_Main_Learn();
                //Main Learn
            }
#endif
        }
    }
}
void Button_Init(void)
{
    Key_Event = rt_event_create("Key_Event", RT_IPC_FLAG_PRIO);
    button_t = rt_thread_create("button_t", button_task_entry, RT_NULL, 256, 10, 20);
    if (button_t != RT_NULL)
    {
        rt_thread_startup(button_t);
    }
    key_response_t = rt_thread_create("key_response_t", Key_Reponse_Callback, RT_NULL, 2048, 10, 10);
    if(key_response_t!=RT_NULL)
    {
        rt_thread_startup(key_response_t);
    }
}
