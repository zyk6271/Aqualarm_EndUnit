/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-27     Rick       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include <stdint.h>
#include "pin_config.h"
#include "key.h"
#include "led.h"
#include "status.h"
#include "radio_encoder.h"

#define DBG_TAG "status"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

enum Warning_Type Warning_Status;

WariningEvent NowStatusEvent;
WariningEvent WaterAlarmActiveEvent;
WariningEvent OfflineEvent;
WariningEvent LowPowerEvent;
WariningEvent UltraLowPowerEvent;
WariningEvent LostPeakEvent;

static uint8_t warn_water_count;
static uint8_t release_warn_water_count;

static struct rt_lptimer warn_water_timer;
static struct rt_lptimer release_warn_water_timer;

static uint8_t ValveStore;

uint8_t Get_ValveStore(void)
{
    return ValveStore;
}
void Set_ValveStore(uint8_t value)
{
    ValveStore = value;
}
uint8_t Get_Warning_Status(void)
{
    return Warning_Status;
}
void Heart_Refresh(void)
{
    if(Warning_Status == Offline)
    {
        Warning_Disable(Offline);
    }
}
void Warning_Enable(WariningEvent event)
{
    if(event.priority >= NowStatusEvent.priority)
    {
        NowStatusEvent.last_id = event.warning_id;
        NowStatusEvent.priority = event.priority;
        if(event.callback!=RT_NULL)
        {
            NowStatusEvent.callback = event.callback;
            NowStatusEvent.callback(RT_NULL);
        }
        LOG_D("Warning_Enable Success,warning id is %d,now priority is %d\r\n",event.warning_id,event.priority);
    }
    else
    {
        LOG_D("Warning_Enable Fail last is %d Now is %d\r\n",NowStatusEvent.priority,event.priority);
    }
}
void Warning_Active_Num(uint8_t id)
{
    Set_ValveStore(1);
    switch(id)
    {
    case 1:
        Warning_Enable(WaterAlarmActiveEvent);
        break;
    case 2:
        Warning_Enable(OfflineEvent);
        break;
    case 3:
        Warning_Enable(LowPowerEvent);
        break;
    case 4:
        Warning_Enable(UltraLowPowerEvent);
        break;
    case 5:
        Warning_Enable(LostPeakEvent);
        break;
    default:
        break;
    }
}
void WarningEventInit(uint8_t warning_id,uint8_t priority,WariningEvent *event,void (*callback)(void*))
{
    rt_memset(event,0,sizeof(&event));
    event->warning_id = warning_id;
    event->last_id = 0;
    event->priority = priority;
    event->callback = callback;
}
void Warning_Disable(enum Warning_Type type)
{
    if(Warning_Status <= type && Warning_Status != 0)
    {
        Warning_Status = Normal;
        NowStatusEvent.last_id = 0;
        NowStatusEvent.priority = 0;
        Led_Alarm_Disable();
        WaterScan_Clear();
        switch(type)
        {
        case LostPeak:
            RF_Peak_Alarm_Disable();
            break;
        case WaterAlarmActive:
            RF_Water_Alarm_Disable();
            break;
        }
        LOG_I("Warning is Disable\r\n");
    }
    else
    {
        LOG_I("Warning Disable fail,now is %d,request is %d\r\n",Warning_Status,type);
    }
}

void Period_WaterAlarm(void)
{
    LOG_I("Period_WaterAlarm\r\n");
    release_warn_water_count = 0;
    warn_water_count = 0;
    RF_Water_Alarm_Enable();
}

void warn_water_timer_callback(void *parameter)
{
    LOG_D("warn_water_count is %d\r\n",warn_water_count);
    RF_Water_Alarm_Enable();
}
void Start_Warn_Water_Timer(void)
{
    if(warn_water_count++ < 5)
    {
        LOG_I("Start_Warn_Water_Timer\r\n");
        rt_lptimer_start(&warn_water_timer);
    }
}
void Stop_Warn_Water_Timer(void)
{
    LOG_I("Stop_Warn_Water_Timer\r\n");
    rt_lptimer_stop(&warn_water_timer);
}

void release_warn_water_timer_callback(void *parameter)
{
    LOG_D("release_warn_water_count is %d\r\n",release_warn_water_count);
    RF_Water_Alarm_Disable();
}
void Start_Release_Warn_Water_Timer(void)
{
    if(release_warn_water_count++ < 5)
    {
        LOG_I("Start_Release_Warn_Water_Timer\r\n");
        rt_lptimer_start(&release_warn_water_timer);
    }
}
void Stop_Release_Warn_Water_Timer(void)
{
    LOG_I("Stop_Release_Warn_Water_Timer\r\n");
    rt_lptimer_stop(&release_warn_water_timer);
}

void WaterAlarmActiveEvent_Callback(void *parameter)
{
    Warning_Status = WaterAlarmActive;
    Led_Alarm_Enable(3,5);
    Period_WaterAlarm();
    LOG_I("WaterAlarmActiveEvent_Callback\r\n");
}
void OfflineEvent_Callback(void *parameter)
{
    Warning_Status = Offline;
    Led_Alarm_Enable(4,15);
    LOG_I("OfflineEvent_Callback\r\n");
}
void LowPowerEvent_Callback(void *parameter)
{
    Warning_Status = LowPower;
    Led_Alarm_Enable(1,30);
    RF_HeartWithMain();
    LOG_I("LowPowerEvent_Callback\r\n");
}
void UltraLowPowerEvent_Callback(void *parameter)
{
    Warning_Status = UltraLowPower;
    Led_Alarm_Enable(1,15);
    RF_HeartWithMain();
    LOG_I("UltraLowPowerEvent_Callback\r\n");
}
void LostPeakEvent_Callback(void *parameter)
{
    Warning_Status = LostPeak;
    Led_Alarm_Lost_Enable();
    RF_Peak_Alarm_Enable();
    LOG_I("LostPeakEvent_Callback\r\n");
}
void WarningInit(void)
{
    rt_lptimer_init(&warn_water_timer, "warn_water_timer", warn_water_timer_callback, RT_NULL,5000, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    rt_lptimer_init(&release_warn_water_timer, "release_warn_water_timer", release_warn_water_timer_callback, RT_NULL,5000, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    WarningEventInit(0,0,&NowStatusEvent,RT_NULL);//本地存储器
    WarningEventInit(1,1,&LowPowerEvent,LowPowerEvent_Callback);
    WarningEventInit(2,2,&LostPeakEvent,LostPeakEvent_Callback);
    WarningEventInit(3,3,&UltraLowPowerEvent,UltraLowPowerEvent_Callback);
    WarningEventInit(4,4,&OfflineEvent,OfflineEvent_Callback);
    WarningEventInit(5,5,&WaterAlarmActiveEvent,WaterAlarmActiveEvent_Callback);
}
