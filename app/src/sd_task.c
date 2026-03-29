/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 23.03.2026
 */
#include "sd_task.h"

#include "board_data.h"
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

static char filename[32] = {};

static char buffer_A[SD_BUFFER_BYTES] __attribute__((section(".sram3")));
static char buffer_B[SD_BUFFER_BYTES] __attribute__((section(".sram3")));
static char* double_buffer[2] = {buffer_A, buffer_B};
static BoardData_t log_pool_mem[SD_QUEUE_LENGTH] __attribute__((section(".sram3")));

static uint8_t active_idx = 0;
static size_t active_buffer_pos = 0;
static size_t bytes_to_write[2] = {0, 0};

const osMemoryPoolAttr_t pool_attr = { .mp_mem = log_pool_mem, .mp_size = sizeof(log_pool_mem) };
static osMemoryPoolId_t log_pool_id;
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

const osThreadAttr_t packer_attr = { .name = "packer_task", .priority = osPriorityAboveNormal, .stack_size = 2048 };
const osThreadAttr_t sd_write_attr = { .name = "sd_task", .priority = osPriorityNormal, .stack_size = 4096 };
#ifdef SD_DETECT_PIN_OPERATIONAL
const osThreadAttr_t monitor_attr = { .name = "monitor_task", .priority = osPriorityLow, .stack_size = 1024 };
#endif

static void packer_task_thread(void *arg);
static void sd_task_thread(void *arg);
#ifdef SD_DETECT_PIN_OPERATIONAL
static void monitor_task_thread(void *arg);
#endif

static bool find_next_filename() {
    for (int i = 0; i < LOG_MAX_FILES; i++) {
        snprintf(filename, sizeof(filename), "%s/%s%03d%s", LOG_DIR, LOG_FILENAME_PREFIX, i, LOG_FILENAME_EXT);

        FILINFO fno;
        if (f_stat(filename, &fno) == FR_NO_FILE) return true;
    }

    return false;
}

bool sd_mount(void) {
    if (is_mounted) return true;
    
    osMutexAcquire(sd_mutex_id, osWaitForever);
    FRESULT res = f_mount(&fs, SDPath, 1);
    if (res != FR_OK) {
        osMutexRelease(sd_mutex_id);
        return false;
    }

    res = f_mkdir(LOG_DIR);
    if (res != FR_OK && res != FR_EXIST) {
        f_mount(NULL, SDPath, 0);
        osMutexRelease(sd_mutex_id);
        return false;
    }

    if (!find_next_filename()) {
        f_mount(NULL, SDPath, 0);
        osMutexRelease(sd_mutex_id);
        return false;
    }

    res = f_open(&log_file, filename, FA_WRITE | FA_CREATE_NEW);
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
    if (!is_mounted) return;

    osMutexAcquire(sd_mutex_id, osWaitForever);

    f_close(&log_file);
    f_mount(NULL, SDPath, 0);

    is_mounted = false;
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
    log_pool_id = osMemoryPoolNew(SD_QUEUE_LENGTH, sizeof(BoardData_t), &pool_attr);
    if (log_pool_id == NULL) goto error_exit;

    log_queue_id = osMessageQueueNew(SD_QUEUE_LENGTH, sizeof(BoardData_t*), NULL);
    if (log_queue_id == NULL) goto error_exit;
    
    if (FATFS_LinkDriver(&USER_Driver, SDPath) != 0) goto error_exit;

    buffer_free_sem_id[0] = osSemaphoreNew(1, 1, NULL);
    buffer_free_sem_id[1] = osSemaphoreNew(1, 1, NULL);
    if (buffer_free_sem_id[0] == NULL || buffer_free_sem_id[1] == NULL) goto error_exit;

    sd_mutex_id = osMutexNew(NULL);
    if (sd_mutex_id == NULL) goto error_exit;

    packer_task_id = osThreadNew(packer_task_thread, NULL, &packer_attr);
    if (packer_task_id == NULL) goto error_exit;

    sd_task_id = osThreadNew(sd_task_thread, NULL, &sd_write_attr);
    if (sd_task_id == NULL) goto error_exit;

#ifdef SD_DETECT_PIN_OPERATIONAL
    monitor_task_id = osThreadNew(monitor_task_thread, NULL, &monitor_attr);
    if (monitor_task_id == NULL) goto error_exit;
#endif

    return HAL_OK;

error_exit:
    if (packer_task_id) osThreadTerminate(packer_task_id);
    if (sd_task_id) osThreadTerminate(sd_task_id);
#ifdef SD_DETECT_PIN_OPERATIONAL
    if (monitor_task_id)   osThreadTerminate(monitor_task_id);
#endif
    
    if (sd_mutex_id) osMutexDelete(sd_mutex_id);
    if (buffer_free_sem_id[0]) osSemaphoreDelete(buffer_free_sem_id[0]);
    if (buffer_free_sem_id[1]) osSemaphoreDelete(buffer_free_sem_id[1]);
    
    FATFS_UnLinkDriver(SDPath);
    
    if (log_queue_id) osMessageQueueDelete(log_queue_id);
    if (log_pool_id) osMemoryPoolDelete(log_pool_id);

    log_pool_id = NULL;
    log_queue_id = NULL;
    sd_mutex_id = NULL;
    packer_task_id = NULL;

    return HAL_ERROR;
}

HAL_StatusTypeDef sd_logger_log_data(const BoardData_t *data) {
    if (!is_mounted || data == NULL) return HAL_ERROR;

    BoardData_t *p_data = osMemoryPoolAlloc(log_pool_id, 0); 
    if (p_data == NULL) return HAL_ERROR;

    memcpy(p_data, data, sizeof(BoardData_t));

    if (osMessageQueuePut(log_queue_id, &p_data, 0, 0) != osOK) {
        osMemoryPoolFree(log_pool_id, p_data);
        return HAL_ERROR;
    }

    return HAL_OK;
}

static void flush_active_buffer(void) {
    if (active_buffer_pos == 0) return;

    uint8_t ready_idx = active_idx;
    bytes_to_write[ready_idx] = active_buffer_pos;

    active_idx = !active_idx;
    active_buffer_pos = 0;

    osSemaphoreAcquire(buffer_free_sem_id[active_idx], osWaitForever);
    osThreadFlagsSet(sd_task_id, (ready_idx == 0) ? 0x01 : 0x02);
}

static void packer_task_thread(void *arg) {
    (void)arg;
    BoardData_t *p_item;
    
    uint32_t last_flush_tick = osKernelGetTickCount();
    const uint32_t flush_interval = (SD_FORCE_FLUSH_INTERVAL_MS * osKernelGetTickFreq()) / 1000U;

    osSemaphoreAcquire(buffer_free_sem_id[active_idx], osWaitForever);
    for(;;) {
        osStatus_t status = osMessageQueueGet(log_queue_id, &p_item, NULL, flush_interval);

        if (status == osOK) {
            if (active_buffer_pos > (SD_BUFFER_BYTES - SD_BUFFER_MARGIN)) {
                flush_active_buffer();
                last_flush_tick = osKernelGetTickCount();
            }

            int len = board_data_serialize(p_item, &double_buffer[active_idx][active_buffer_pos], SD_BUFFER_BYTES - active_buffer_pos);
            if (len > 0) active_buffer_pos += (size_t)len;
            osMemoryPoolFree(log_pool_id, p_item);

            if ((osKernelGetTickCount() - last_flush_tick) >= flush_interval) {
                flush_active_buffer();
                last_flush_tick = osKernelGetTickCount();
            }
        } else if (status == osErrorTimeout) {
            flush_active_buffer();
            last_flush_tick = osKernelGetTickCount();
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