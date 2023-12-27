/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-19     Rick       the first version
 */
#include <rtthread.h>
#include <board.h>
#include <stm32wlxx.h>
#include "pin_config.h"

#define DBG_TAG "low"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

void Pin_DeInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
#ifdef MINI
void Pin_Init(void)
{
    RF_Switch_Init();
    rt_pin_mode(KEY_OFF_PIN,PIN_MODE_INPUT);
    rt_pin_mode(WATER_SIG_PIN,PIN_MODE_INPUT);
    rt_pin_mode(LED_G_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(LED_R_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(BUZZER_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(WATER_SIG_PIN,PIN_MODE_INPUT);
}
void IRQ_Bind(void)
{
    rt_pin_mode(KEY_OFF_PIN,PIN_MODE_INPUT);
    rt_pin_attach_irq(KEY_OFF_PIN, PIN_IRQ_MODE_RISING_FALLING, IRQ_Callback, RT_NULL);
    rt_pin_irq_enable(KEY_OFF_PIN, PIN_IRQ_ENABLE);

    rt_pin_attach_irq(WATER_SIG_PIN, PIN_IRQ_MODE_FALLING, IRQ_Callback, RT_NULL);
    rt_pin_irq_enable(WATER_SIG_PIN, PIN_IRQ_ENABLE);
}
void IRQ_Unbind(void)
{
    rt_pin_detach_irq(KEY_OFF_PIN);
    rt_pin_irq_enable(KEY_OFF_PIN, PIN_IRQ_DISABLE);

    rt_pin_detach_irq(WATER_SIG_PIN);
    rt_pin_irq_enable(WATER_SIG_PIN, PIN_IRQ_DISABLE);
}
#else
void Pin_Init(void)
{
    rt_pin_mode(KEY_OFF_PIN,PIN_MODE_INPUT);
    rt_pin_mode(KEY_ON_PIN,PIN_MODE_INPUT);
    rt_pin_mode(WATER_LOS_PIN,PIN_MODE_INPUT);
    rt_pin_mode(WATER_SIG_PIN,PIN_MODE_INPUT);
    rt_pin_mode(LED_G_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(LED_R_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(BUZZER_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(WATER_LOS_PIN,PIN_MODE_INPUT);
    rt_pin_mode(WATER_SIG_PIN,PIN_MODE_INPUT);
}
void IRQ_Callback(void *paramter)
{
    //Re-sleep after 3000ms
    rt_pm_module_delay_sleep(PM_WAIT_ID,1000);
}

void IRQ_Bind(void)
{
    rt_pin_mode(KEY_OFF_PIN,PIN_MODE_INPUT);
    rt_pin_mode(KEY_ON_PIN,PIN_MODE_INPUT);
    rt_pin_attach_irq(KEY_OFF_PIN, PIN_IRQ_MODE_RISING_FALLING, IRQ_Callback, RT_NULL);
    rt_pin_attach_irq(KEY_ON_PIN, PIN_IRQ_MODE_RISING_FALLING, IRQ_Callback, RT_NULL);
    rt_pin_irq_enable(KEY_OFF_PIN, PIN_IRQ_ENABLE);
    rt_pin_irq_enable(KEY_ON_PIN, PIN_IRQ_ENABLE);

    rt_pin_attach_irq(WATER_LOS_PIN, PIN_IRQ_MODE_RISING_FALLING, IRQ_Callback, RT_NULL);
    rt_pin_attach_irq(WATER_SIG_PIN, PIN_IRQ_MODE_FALLING, IRQ_Callback, RT_NULL);
    rt_pin_irq_enable(WATER_LOS_PIN, PIN_IRQ_ENABLE);
    rt_pin_irq_enable(WATER_SIG_PIN, PIN_IRQ_ENABLE);
}
void IRQ_Unbind(void)
{
    rt_pin_detach_irq(KEY_OFF_PIN);
    rt_pin_detach_irq(KEY_ON_PIN);
    rt_pin_irq_enable(KEY_OFF_PIN, PIN_IRQ_DISABLE);
    rt_pin_irq_enable(KEY_ON_PIN, PIN_IRQ_DISABLE);

    rt_pin_detach_irq(WATER_LOS_PIN);
    rt_pin_detach_irq(WATER_SIG_PIN);
    rt_pin_irq_enable(WATER_LOS_PIN, PIN_IRQ_DISABLE);
    rt_pin_irq_enable(WATER_SIG_PIN, PIN_IRQ_DISABLE);
}
#endif
void Debug_DeInit(void)
{
    LOG_D("Goto sleep,now tick is %d\r\n",rt_tick_get());
    rt_device_control(rt_console_get_device(), RT_DEVICE_CTRL_CLOSE, RT_NULL);
}
void Debug_Init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    rt_device_control(rt_console_get_device(), RT_DEVICE_CTRL_CONFIG, &config);
}

void low_power_notify(rt_uint8_t event, rt_uint8_t mode, void *data)
{
    if (event == RT_PM_ENTER_SLEEP)
    {
        //SYSTICK
        rt_hw_systick_deinit();
        //RF
        RF_Sleep();
        //PIN
        Pin_DeInit();
        //KEY
        IRQ_Bind();
        //DEBUG UART
        Debug_DeInit();
    }
    else if (event == RT_PM_EXIT_SLEEP_WITHOUT_ISR)
    {
        //DEBUG UART
        Debug_Init();
        //RF Switch
        RF_Switch_Init();
        //RF
        RF_Wake();
    }
    else if (event == RT_PM_EXIT_SLEEP_WITH_ISR)
    {
        LOG_D("Wake up,now tick is %d\r\n",rt_tick_get());
        //KEY
        IRQ_Unbind();
        //PIN
        Pin_Init();
        //SYSTICK
        rt_hw_systick_init();
    }
}
void Low_Init(void)
{
    rt_pm_notify_set(low_power_notify, RT_NULL);
    rt_pm_request(PM_SLEEP_MODE_DEEP);
    rt_pm_release(PM_SLEEP_MODE_NONE);
}
