/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-26     Rick       the first version
 */
#include <rtthread.h>
#include <stdint.h>
#include <rtdevice.h>
#include "pin_config.h"
#include "status.h"

#define DBG_TAG "water"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static uint8_t NowStatus = 0;
static uint8_t PastStatus = 0;

static uint8_t WaterAlarm = 0;

rt_thread_t waterscan_t = RT_NULL;

uint8_t Get_WaterAlarm(void)
{
    return WaterAlarm;
}

void WarningWithPeak(uint8_t status)
{
    switch (status)
    {
    case 0: //恢复正常
        LOG_D("Water Status %d\r\n", status);
        WaterAlarm = 0;
        Warning_Disable(LostPeak);
        break;
    case 1: //测水线掉落
        LOG_D("Water Status %d\r\n", status);
        Warning_Active_Num(5);
        break;
    case 2: //测水线短路
        LOG_D("Water Status %d\r\n", status);
        Warning_Active_Num(1);
        WaterAlarm = 1;
        break;
    case 3: //测水线短路解除
        WaterAlarm = 0;
        LOG_D("Water Status %d\r\n", status);
        break;
    }
}
void WaterScan_Clear(void)
{
    PastStatus = 0;
    NowStatus = 0;
}
void WaterScan_Callback(void *parameter)
{
    uint8_t Peak_ON_Level = 0;
    uint8_t Peak_Loss_Level = 0;
    rt_pin_mode(WATER_SIG_PIN, PIN_MODE_INPUT);
#ifndef MINI
    rt_pin_mode(WATER_LOS_PIN, PIN_MODE_INPUT);
#endif
    rt_thread_mdelay(1000);
    while (1) //插入是0，短路是0
    {
        Peak_ON_Level = rt_pin_read(WATER_SIG_PIN);
#ifndef MINI
        Peak_Loss_Level = rt_pin_read(WATER_LOS_PIN);
        if (Peak_Loss_Level)
        {
            NowStatus = 1; //测水线掉落
        }
        else
        {
#endif
            if (Peak_ON_Level == 0)
            {
                NowStatus = 2; //测水线短路
            }
            else
            {
                NowStatus = 0; //状态正常
#ifndef MINI
            }
#endif
        }
        if (NowStatus != PastStatus)
        {
            switch (NowStatus)
            {
            case 0:
                if(WaterAlarm)
                {
                    WarningWithPeak(3);
                }
                else
                {
                    WarningWithPeak(0);
                }
                break;
            case 1:
                WarningWithPeak(1);
                break;
            case 2:
                WarningWithPeak(2);
                break;
            }
            PastStatus = NowStatus;
        }
        rt_thread_mdelay(100);
    }
}
void WaterScan_Init(void)
{
    waterscan_t = rt_thread_create("waterscan_t", WaterScan_Callback, RT_NULL, 512, 10, 10);
    if (waterscan_t != RT_NULL)rt_thread_startup(waterscan_t);
}
