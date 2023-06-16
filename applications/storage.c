/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-16     Rick       the first version
 */
#include <fal.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#define DBG_TAG "Storage"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static uint32_t Main_ID = 0;
static const struct fal_partition *part_dev = NULL;

void StorageInit(void)
{
    fal_init();
    part_dev = fal_partition_find("cfg");
    if (part_dev != RT_NULL)
    {
        LOG_D("Probed a flash partition | %s | flash_dev: %s | offset: %ld | len: %d |.\n",
                part_dev->name, part_dev->flash_name, part_dev->offset, part_dev->len);
    }
    fal_partition_read(part_dev, 0, &Main_ID, 4);
    if(Main_ID == 0 || Main_ID == 0xFFFFFFFF)
    {
        Main_ID = 10000001;
    }
    LOG_D("Main ID %d\r\n",Main_ID);
}
void Storage_Main_Write(uint32_t id)
{
    Main_ID = id;
    fal_partition_erase(part_dev, 0, 8);
    fal_partition_write(part_dev, 0, &id, 8);
}
uint32_t Storage_Main_Read(void)
{
    return Main_ID;
}
