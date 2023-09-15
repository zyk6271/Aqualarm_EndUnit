#include <rtthread.h>
#include <rtdevice.h>
#include "pin_config.h"
#include "rtc.h"
#include "board.h"

#define DBG_TAG "RTC"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

rt_sem_t RTC_IRQ_Sem;
RTC_HandleTypeDef rtc_handle;
rt_thread_t RTC_Scan = RT_NULL;

uint8_t RTC_Counter;
uint8_t heart_count;

struct rt_lptimer heart_timer;
struct rt_lptimer once_heart_timer;

void heart_timer_callback(void *parameter)
{
    LOG_D("Heart Retry Count is %d\r\n",heart_count);
    if(heart_count++ < 10)
    {
        RF_HeartWithMain();
    }
    else
    {
        Warning_Active_Num(2);
    }
}

void Start_Heart_Timer(void)
{
    LOG_D("Start Watting Heart Response\r\n");
    rt_lptimer_start(&heart_timer);
}

void Stop_Heart_Timer(void)
{
    LOG_D("Stop_Heart_Timer\r\n");
    rt_lptimer_stop(&heart_timer);
}

void heart_request_send(void)
{
    heart_count = 0;
    RF_HeartWithMain();
}

void once_heart_timer_callback(void *parameter)
{
    heart_request_send();
}

void RTC_Timer_Entry(void *parameter)
{
    while(1)
    {
        rt_sem_take(RTC_IRQ_Sem, RT_WAITING_FOREVER);
        if(RTC_Counter < 23)
        {
            RTC_Counter++;
        }
        else
        {
            RTC_Counter=0;
            heart_request_send();
        }
        LOG_I("RTC alarm,time is %d\r\n",RTC_Counter);
        rt_pm_sleep_release(PM_RTC_ID, PM_SLEEP_MODE_NONE);
    }
}
void HW_RTC_Init(void)
{
    /* USER CODE BEGIN RTC_Init 0 */

    /* USER CODE END RTC_Init 0 */

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    RTC_AlarmTypeDef sAlarm = {0};

    /* USER CODE BEGIN RTC_Init 1 */

    /* USER CODE END RTC_Init 1 */

    /** Initialize RTC Only
    */
    rtc_handle.Instance = RTC;
    rtc_handle.Init.HourFormat = RTC_HOURFORMAT_24;
    rtc_handle.Init.AsynchPrediv = 127;
    rtc_handle.Init.SynchPrediv = 255;
    rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE;
    rtc_handle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    rtc_handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    rtc_handle.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
    rtc_handle.Init.BinMode = RTC_BINARY_NONE;
    if (HAL_RTC_Init(&rtc_handle) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN Check_RTC_BKUP */

    /* USER CODE END Check_RTC_BKUP */

    /** Initialize RTC and set the Time and Date
    */
    sTime.Hours = 0x0;
    sTime.Minutes = 0x0;
    sTime.Seconds = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&rtc_handle, &sTime, RTC_FORMAT_BCD) != HAL_OK)
    {
        Error_Handler();
    }
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month = RTC_MONTH_FEBRUARY;
    sDate.Date = 0x18;
    sDate.Year = 0x14;

    if (HAL_RTC_SetDate(&rtc_handle, &sDate, RTC_FORMAT_BCD) != HAL_OK)
    {
        Error_Handler();
    }

    /** Enable the Alarm A
    */
    sAlarm.AlarmTime.Hours = 0x1;
    sAlarm.AlarmTime.Minutes = 0x0;
    sAlarm.AlarmTime.Seconds = 0x0;
    sAlarm.AlarmTime.SubSeconds = 0x0;
    sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    sAlarm.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY;
    sAlarm.Alarm = RTC_ALARM_A;
    if (HAL_RTC_SetAlarm_IT(&rtc_handle, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
    {
        Error_Handler();
    }
  /* USER CODE BEGIN RTC_Init 2 */
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  /* USER CODE END RTC_Init 2 */

}
void RTC_Alarm_IRQHandler(void)
{
    /* USER CODE BEGIN RTC_Alarm_IRQn 0 */

    /* USER CODE END RTC_Alarm_IRQn 0 */
    HAL_RTC_AlarmIRQHandler(&rtc_handle);
    /* USER CODE BEGIN RTC_Alarm_IRQn 1 */

    /* USER CODE END RTC_Alarm_IRQn 1 */
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    rt_pm_sleep_request(PM_RTC_ID, PM_SLEEP_MODE_NONE);
    rt_sem_release(RTC_IRQ_Sem);

    RTC_TimeTypeDef sTime = {0};
    sTime.Hours = 0x0;
    sTime.Minutes = 0x0;
    sTime.Seconds = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&rtc_handle, &sTime, RTC_FORMAT_BCD) != HAL_OK)
    {
        Error_Handler();
    }
}
void RTC_Init(void)
{
    HW_RTC_Init();
    RTC_IRQ_Sem = rt_sem_create("RTC_IRQ", 0, RT_IPC_FLAG_FIFO);

    rt_lptimer_init(&heart_timer, "heart_timer", heart_timer_callback, RT_NULL,5*60*1000, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    rt_lptimer_init(&once_heart_timer, "once_heart_timer", once_heart_timer_callback, RT_NULL,30*1000, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    rt_lptimer_start(&once_heart_timer);

    RTC_Scan = rt_thread_create("RTC_Scan", RTC_Timer_Entry, RT_NULL, 2048, 10, 10);
    rt_thread_startup(RTC_Scan);
}
