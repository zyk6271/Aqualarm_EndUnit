/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-06     Rick       the first version
 */
#ifndef APPLICATIONS_PIN_CONFIG_H_
#define APPLICATIONS_PIN_CONFIG_H_
/*
 * RF
 */
#define RF_SW1_PIN                      6
#define RF_SW2_PIN                      7
#define TCXO_PWR_PIN                    16
/*
 * STORAGE
 */
#define EEPROM_VCC_PIN                  21
#define EEPROM_SCL_PIN                  22
#define EEPROM_SDA_PIN                  23
/*
 * WATER
 */
#ifdef MINI
#define WATER_SIG_PIN                   11
/*
 * LED
 */
#define LED_G_PIN                       28
#define LED_R_PIN                       10
#define BUZZER_PIN                      18
/*
 * KEY
 */
#define KEY_OFF_PIN                     12
#else
#define WATER_LOS_PIN                   20
#define WATER_SIG_PIN                   15
/*
 * LED
 */
#define LED_G_PIN                       11
#define LED_R_PIN                       10
#define BUZZER_PIN                      24
/*
 * KEY
 */
#define KEY_OFF_PIN                     18
#define KEY_ON_PIN                      19
#endif
#endif /* APPLICATIONS_PIN_CONFIG_H_ */
