/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-16     Rick       the first version
 */
#include <board.h>
#include <rtthread.h>

#define DBG_TAG "PVD"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static uint8_t Bat_Level;
static uint8_t Detect_Level;

static uint8_t Detect_Flag;
static uint8_t Detect_Count;

PWR_PVDTypeDef pvd_handle;
rt_sem_t PVD_IRQ_Sem = RT_NULL;
rt_thread_t PVD_scan_t = RT_NULL;

void PVD_PVM_IRQHandler(void)
{
    rt_pm_sleep_request(PM_PVD_ID, PM_SLEEP_MODE_NONE);
    rt_sem_release(PVD_IRQ_Sem);

    if(Detect_Level == 2)
    {
        Detect_Flag = 1;
    }

    HAL_PWREx_PVD_PVM_IRQHandler();
}

void PVD_scan_entry(void *parameter)
{
    while(1)
    {
        rt_sem_take(PVD_IRQ_Sem, RT_WAITING_FOREVER);
        switch(Detect_Level)
        {
        case 0://80
            pvd_handle.PVDLevel = PWR_PVDLEVEL_3;
            Bat_Level = 3;
            Detect_Level = 1;
            break;
        case 1://60
            pvd_handle.PVDLevel = PWR_PVDLEVEL_1;
            Bat_Level = 4;
            Detect_Level = 2;
            break;
        case 2://40
            if(Detect_Count < 3)
            {
                Detect_Count++;
                LOG_D("Detect_Count is %d\n",Detect_Count);
            }
            else
            {
                pvd_handle.PVDLevel = PWR_PVDLEVEL_0;
                Bat_Level = 1;
                Detect_Level = 3;
                Warning_Active_Num(3);
            }
            break;
        case 3://10
            Detect_Level = 4;
            Bat_Level = 2;
            Warning_Active_Num(4);
            break;
        default:
            break;
        }
        HAL_PWR_ConfigPVD(&pvd_handle);
        rt_pm_sleep_release(PM_PVD_ID, PM_SLEEP_MODE_NONE);
    }
}
void PVD_Init(void)
{
    HAL_NVIC_SetPriority(PVD_PVM_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(PVD_PVM_IRQn);

    pvd_handle.PVDLevel = PWR_PVDLEVEL_4;
    pvd_handle.Mode = PWR_PVD_MODE_IT_FALLING;
    HAL_PWR_ConfigPVD(&pvd_handle);

    PVD_IRQ_Sem = rt_sem_create("RTC_IRQ", 0, RT_IPC_FLAG_FIFO);
    PVD_scan_t = rt_thread_create("PVD_scan", PVD_scan_entry, RT_NULL, 2048, 10, 10);
    rt_thread_startup(PVD_scan_t);
}
void PVD_Open(void)
{
    Detect_Flag = 0;
    HAL_PWR_EnablePVD();
    LOG_D("Enable the PVD Output\r\n");
}
void PVD_Close(void)
{
    HAL_PWR_DisablePVD();
    LOG_D("Disable the PVD Output\r\n");
    if(Detect_Flag == 0)
    {
        Detect_Count = 0;
    }
}
uint8_t Get_Bat_Level(void)
{
    return Bat_Level;
}
