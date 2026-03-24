/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 23.03.2026
 */
#include "sd_task.h"

#include "ff.h"
#include "task.h"
#include <stdio.h>
#include "semphr.h"
#include <string.h>
#include "cmsis_os2.h"
#include "stm32h5xx_hal.h"
#include "FreeRTOSConfig.h"
#include "stm32h5xx_hal_def.h"

static char buffer_A[SD_BUFFER_BYTES];
static char buffer_B[SD_BUFFER_BYTES];
static char* double_buffer[2] = {buffer_A, buffer_B};

static uint8_t active_idx = 0;
static size_t active_buffer_pos = 0;
static size_t bytes_to_write[2] = {0, 0};

static QueueHandle_t log_queue;
static TaskHandle_t packer_task_handle;
static TaskHandle_t sd_task_handle;

static SemaphoreHandle_t buffer_free_sem[2];

static FATFS fs;
static FIL log_file;
static volatile bool is_mounted = false;

static void packer_task_thread(void *arg);
static void sd_task_thread(void *arg);

bool sd_logger_mount(void) {
    if (is_mounted) return true;

    if (f_mount(&fs, "", 1) != FR_OK) return false;

    FRESULT res = f_open(&log_file, LOG_FILE_NAME, FA_WRITE | FA_OPEN_APPEND);
    if (res != FR_OK) {
        f_mount(NULL, "", 0);
        return false;
    }

    if (f_size(&log_file) == 0) {
        char header[128];
        int len = board_data_get_header(header, sizeof(header));
        if (len > 0) {
            UINT bw;
            f_write(&log_file, header, len, &bw);
        }
    }

    is_mounted = true;
    return true;
}

void sd_logger_unmount(void) {
    if (!is_mounted) return;
    sd_logger_sync();

    is_mounted = false;
    
    f_close(&log_file);
    f_mount(NULL, "", 0);
}

bool sd_logger_remount(void) {
    sd_logger_unmount();
    return sd_logger_mount();
}

void sd_logger_sync(void) {
    if (is_mounted) {
        f_sync(&log_file);
    }
}

bool sd_logger_is_mounted(void) {
    return is_mounted;
}

HAL_StatusTypeDef sd_logger_init(void) {
    log_queue = xQueueCreate(SD_QUEUE_LENGTH, sizeof(BoardData_t));
    if (log_queue == NULL) {
        return HAL_ERROR;
    }

    buffer_free_sem[0] = xSemaphoreCreateBinary();
    buffer_free_sem[1] = xSemaphoreCreateBinary();

    xSemaphoreGive(buffer_free_sem[0]);
    xSemaphoreGive(buffer_free_sem[1]);

    xTaskCreate(packer_task_thread, "sd_packer_task", 1024, NULL, osPriorityAboveNormal, &packer_task_handle);
    xTaskCreate(sd_task_thread, "sd_write_task", 2048, NULL, osPriorityNormal, &sd_task_handle);

    return HAL_OK;
}

HAL_StatusTypeDef sd_logger_log_data(const BoardData_t *data) {
    if (!is_mounted) return HAL_ERROR; 
    
    return ((xQueueSend(log_queue, data, 0) == pdPASS) ? HAL_OK : HAL_ERROR); 
}

static void packer_task_thread(void *arg) {
    (void)arg;
    BoardData_t temp_data;
    char temp_str[256]; 

    xSemaphoreTake(buffer_free_sem[active_idx], portMAX_DELAY);
    while(1) {
        if (xQueueReceive(log_queue, &temp_data, portMAX_DELAY) == pdTRUE) {
            if (!is_mounted) continue;

            int len = board_data_serialize(&temp_data, temp_str, sizeof(temp_str));
            if (len > 0) {
                if ((active_buffer_pos + len) >= SD_BUFFER_BYTES) {
                    uint8_t ready_idx = active_idx;
                    bytes_to_write[ready_idx] = active_buffer_pos; 

                    active_idx = !active_idx;
                    active_buffer_pos = 0; 

                    xTaskNotify(sd_task_handle, ready_idx, eSetValueWithOverwrite);
                    xSemaphoreTake(buffer_free_sem[active_idx], portMAX_DELAY);
                }

                memcpy(&double_buffer[active_idx][active_buffer_pos], temp_str, len);
                active_buffer_pos += len;
            }
        }
    }
}

static void sd_task_thread(void *arg) {
    (void)arg;
    uint32_t ready_buffer_idx;
    UINT bytes_written;

    while(1) {
        xTaskNotifyWait(0x00, 0xFFFFFFFF, &ready_buffer_idx, portMAX_DELAY);
        if (!is_mounted) continue; 

        size_t write_size = bytes_to_write[ready_buffer_idx];

        // HAL_DCACHE_CleanByAddr((uint32_t*)double_buffer[ready_buffer_idx], write_size);
        FRESULT res = f_write(&log_file, double_buffer[ready_buffer_idx], write_size, &bytes_written);

        if (res != FR_OK || bytes_written != write_size) {
            // HANDLE WRITE ERROR
        }

        xSemaphoreGive(buffer_free_sem[ready_buffer_idx]);
    }
}