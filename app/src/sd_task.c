/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 23.03.2026
 */
#include "sd_task.h"

#include "ff.h"
#include "logger_macros.h"
#include "main.h"
#include "projdefs.h"
#include "stm32h5xx_hal_gpio.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>
#include "semphr.h"
#include <string.h>
#include "cmsis_os2.h"
#include "stm32h5xx_hal.h"
#include "FreeRTOSConfig.h"
#include "stm32h5xx_hal_def.h"
#include "ff_gen_drv.h"
#include "user_diskio.h"

static char buffer_A[SD_BUFFER_BYTES];
static char buffer_B[SD_BUFFER_BYTES];
static char* double_buffer[2] = {buffer_A, buffer_B};

static uint8_t active_idx = 0;
static size_t active_buffer_pos = 0;
static size_t bytes_to_write[2] = {0, 0};

static osMessageQueueId_t log_queue_id;
static osThreadId_t packer_task_id;
static osThreadId_t sd_task_id;
#ifdef SD_DETECT_PIN_OPERATIONAL
static osThreadId_t monitor_task_id;
#endif

static osSemaphoreId_t buffer_free_sem_id[2];
static osMutexId_t sd_mutex_id;

static FATFS fs;
static FIL log_file;
static volatile bool is_mounted = false;

extern Diskio_drvTypeDef USER_Driver;
char SDPath[4];

const osThreadAttr_t packer_attr = { .name = "packer_task", .priority = osPriorityNormal, .stack_size = 2048 };
const osThreadAttr_t sd_write_attr = { .name = "sd_task", .priority = osPriorityAboveNormal, .stack_size = 4096 };
#ifdef SD_DETECT_PIN_OPERATIONAL
const osThreadAttr_t monitor_attr = { .name = "monitor_task", .priority = osPriorityLow, .stack_size = 1024 };
#endif

static void packer_task_thread(void *arg);
static void sd_task_thread(void *arg);
#ifdef SD_DETECT_PIN_OPERATIONAL
static void monitor_task_thread(void *arg);
#endif

bool sd_mount(void) {
    osMutexAcquire(sd_mutex_id, osWaitForever);
    if (is_mounted) {
        osMutexRelease(sd_mutex_id);
        return true;
    }

    if (f_mount(&fs, SDPath, 1) != FR_OK) {
        osMutexRelease(sd_mutex_id);
        return false;
    }

    FRESULT res = f_open(&log_file, LOG_FILE_NAME, FA_WRITE | FA_OPEN_APPEND);
    if (res != FR_OK) {
        f_mount(NULL, SDPath, 0);
        osMutexRelease(sd_mutex_id);
        return false;
    }

    if (f_size(&log_file) == 0) {
        char header[128];
        int len = board_data_get_header(header, sizeof(header));
        if (len > 0) {
            UINT bw;
            f_write(&log_file, header, len, &bw);
            f_sync(&log_file);
        }
    }

    is_mounted = true;
    HAL_GPIO_WritePin(SD_STATUS_GPIO_Port, SD_STATUS_Pin, GPIO_PIN_SET);

    osMutexRelease(sd_mutex_id);
    return true;
}

void sd_unmount(void) {
    osMutexAcquire(sd_mutex_id, osWaitForever);

    if (!is_mounted) {
        osMutexRelease(sd_mutex_id);
        return;
    }
    f_sync(&log_file);

    is_mounted = false;
    
    f_close(&log_file);
    f_mount(NULL, SDPath, 0);
    HAL_GPIO_WritePin(SD_STATUS_GPIO_Port, SD_STATUS_Pin, GPIO_PIN_RESET);

    osMutexRelease(sd_mutex_id);
}

bool sd_remount(void) {
    sd_unmount();
    return sd_mount();
}

void sd_sync(void) {
    osMutexAcquire(sd_mutex_id, osWaitForever);
    if (is_mounted) f_sync(&log_file);
    osMutexRelease(sd_mutex_id);
}

bool sd_is_mounted(void) {
    return is_mounted;
}

HAL_StatusTypeDef sd_logger_init(void) {
    log_queue_id = osMessageQueueNew(SD_QUEUE_LENGTH, sizeof(BoardData_t), NULL);
    if (log_queue_id == NULL) {
        return HAL_ERROR;
    }
    
    if (FATFS_LinkDriver(&USER_Driver, SDPath) != 0) return HAL_ERROR;

    buffer_free_sem_id[0] = osSemaphoreNew(1, 1, NULL);
    buffer_free_sem_id[1] = osSemaphoreNew(1, 1, NULL);
    if (buffer_free_sem_id[0] == NULL || buffer_free_sem_id[1] == NULL) {
        return HAL_ERROR;
    }

    sd_mutex_id = osMutexNew(NULL);
    if (sd_mutex_id == NULL) {
        return HAL_ERROR;
    }

    packer_task_id = osThreadNew(packer_task_thread, NULL, &packer_attr);
    sd_task_id = osThreadNew(sd_task_thread, NULL, &sd_write_attr);
#ifdef SD_DETECT_PIN_OPERATIONAL
    monitor_task_id = osThreadNew(monitor_task_thread, NULL, &monitor_attr);
#endif

    return HAL_OK;
}

HAL_StatusTypeDef sd_logger_log_data(const BoardData_t *data) {
    if (!is_mounted) return HAL_ERROR;

    osStatus_t status = osMessageQueuePut(log_queue_id, data, 0, 0);
    return (status == osOK) ? HAL_OK : HAL_ERROR;
}

// char temp_str[128];
// int len = board_data_serialize(data, temp_str, sizeof(temp_str));

// UINT bytes_written;
// xSemaphoreTake(sd_mutex, portMAX_DELAY);
// if (is_mounted) {
//     FRESULT res = f_write(&log_file, temp_str, len, &bytes_written);
//     f_sync(&log_file); 

//     if (res != FR_OK || bytes_written != len) {
//         // HANDLE WRITE ERROR
//     }
// }
// xSemaphoreGive(sd_mutex); 

// return HAL_OK;

static void packer_task_thread(void *arg) {
    (void)arg;
    BoardData_t temp_data;
    char temp_str[256];

    osSemaphoreAcquire(buffer_free_sem_id[active_idx], osWaitForever);
    for(;;) {
        osStatus_t status = osMessageQueueGet(log_queue_id, &temp_data, NULL, 500);

        if (status == osOK) {
            int len = board_data_serialize(&temp_data, temp_str, sizeof(temp_str));
            if (len > 0) {
                if ((active_buffer_pos + len) >= SD_BUFFER_BYTES) {
                    uint8_t ready_idx = active_idx;
                    bytes_to_write[ready_idx] = active_buffer_pos;

                    active_idx = !active_idx;
                    active_buffer_pos = 0;

                    osSemaphoreAcquire(buffer_free_sem_id[active_idx], osWaitForever);
                    
                    osThreadFlagsSet(sd_task_id, (ready_idx == 0) ? 0x01 : 0x02);
                }

                memcpy(&double_buffer[active_idx][active_buffer_pos], temp_str, len);
                active_buffer_pos += len;
            }
        } else {
            if (is_mounted && active_buffer_pos > 0) {
                uint8_t ready_idx = active_idx;
                bytes_to_write[ready_idx] = active_buffer_pos;
                active_idx = !active_idx;
                active_buffer_pos = 0;

                osSemaphoreAcquire(buffer_free_sem_id[active_idx], osWaitForever);
                osThreadFlagsSet(sd_task_id, (ready_idx == 0) ? 0x01 : 0x02);
            }
        }
    }
}

static void sd_task_thread(void *arg) {
    (void)arg;
    uint32_t flags;
    UINT bw;

#ifndef SD_DETECT_PIN_OPERATIONAL
    while (!sd_mount()) osDelay(1000);
#endif

    for(;;) {
        flags = osThreadFlagsWait(0x03, osFlagsWaitAny, osWaitForever);
        uint8_t idx = (flags & 0x01) ? 0 : 1;

        osMutexAcquire(sd_mutex_id, osWaitForever);
        if (is_mounted) {
            f_write(&log_file, double_buffer[idx], bytes_to_write[idx], &bw);
            f_sync(&log_file);
        }
        osMutexRelease(sd_mutex_id);

        osSemaphoreRelease(buffer_free_sem_id[idx]);
    }
}

#ifdef SD_DETECT_PIN_OPERATIONAL
static void monitor_task_thread(void *arg) {
    (void)arg;

    for(;;) {
        bool inserted = (HAL_GPIO_ReadPin(SD_DETECT_GPIO_Port, SD_DETECT_Pin) == GPIO_PIN_RESET);

        if (inserted && !is_mounted) sd_mount();
        else if (!inserted && is_mounted) sd_unmount();

        osDelay(500);
    }
}
#endif