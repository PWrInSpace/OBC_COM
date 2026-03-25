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

static QueueHandle_t log_queue;
static TaskHandle_t packer_task_handle;
static TaskHandle_t sd_task_handle;
// static TaskHandle_t monitor_task_handle;

static SemaphoreHandle_t buffer_free_sem[2];
static SemaphoreHandle_t sd_mutex;

static FATFS fs;
static FIL log_file;
static volatile bool is_mounted = false;

extern Diskio_drvTypeDef USER_Driver;
char SDPath[4];

static void packer_task_thread(void *arg);
static void sd_task_thread(void *arg);
static void monitor_task_thread(void *arg);

bool sd_mount(void) {
    xSemaphoreTake(sd_mutex, portMAX_DELAY);
    if (is_mounted) {
        xSemaphoreGive(sd_mutex);
        return true;
    }

    if (f_mount(&fs, SDPath, 1) != FR_OK) {
        xSemaphoreGive(sd_mutex);
        return false;
    }

    FRESULT res = f_open(&log_file, LOG_FILE_NAME, FA_WRITE | FA_OPEN_APPEND);
    if (res != FR_OK) {
        f_mount(NULL, SDPath, 0);
        xSemaphoreGive(sd_mutex);
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

    xSemaphoreGive(sd_mutex);
    return true;
}

void sd_unmount(void) {
    xSemaphoreTake(sd_mutex, portMAX_DELAY);

    if (!is_mounted) {
        xSemaphoreGive(sd_mutex);
        return;
    }
    f_sync(&log_file);

    is_mounted = false;
    
    f_close(&log_file);
    f_mount(NULL, SDPath, 0);
    HAL_GPIO_WritePin(SD_STATUS_GPIO_Port, SD_STATUS_Pin, GPIO_PIN_RESET);

    xSemaphoreGive(sd_mutex);
}

bool sd_remount(void) {
    sd_unmount();
    return sd_mount();
}

void sd_sync(void) {
    xSemaphoreTake(sd_mutex, portMAX_DELAY);
    if (is_mounted) f_sync(&log_file);
    xSemaphoreGive(sd_mutex);
}

bool sd_is_mounted(void) {
    return is_mounted;
}

HAL_StatusTypeDef sd_logger_init(void) {
    log_queue = xQueueCreate(SD_QUEUE_LENGTH, sizeof(BoardData_t));
    if (log_queue == NULL) {
        return HAL_ERROR;
    }
    
    if (FATFS_LinkDriver(&USER_Driver, SDPath) != 0) {
        return HAL_ERROR;
    }

    buffer_free_sem[0] = xSemaphoreCreateBinary();
    buffer_free_sem[1] = xSemaphoreCreateBinary();
    if (buffer_free_sem[0] == NULL || buffer_free_sem[1] == NULL) {
        return HAL_ERROR;
    }

    sd_mutex = xSemaphoreCreateMutex();
    if (sd_mutex == NULL) {
        return HAL_ERROR;
    }

    xSemaphoreGive(buffer_free_sem[0]);
    xSemaphoreGive(buffer_free_sem[1]);

    xTaskCreate(packer_task_thread, "sd_packer_task", 1024, NULL, osPriorityAboveNormal, &packer_task_handle);
    xTaskCreate(sd_task_thread, "sd_write_task", 4096, NULL, osPriorityNormal, &sd_task_handle);
    // xTaskCreate(monitor_task_thread, "sd_monitor", 512, NULL, osPriorityLow, &monitor_task_handle);

    return HAL_OK;
}

HAL_StatusTypeDef sd_logger_log_data(const BoardData_t *data) {
    if (!is_mounted) return HAL_ERROR; 
    
    return ((xQueueSend(log_queue, data, 0) == pdPASS) ? HAL_OK : HAL_ERROR); 
}

static void packer_task_thread(void *arg) {
    (void)arg;
    BoardData_t temp_data;
    char temp_str[512];

    while (!sd_mount()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    xSemaphoreTake(buffer_free_sem[active_idx], portMAX_DELAY);
    while(1) {
        BaseType_t xStatus = xQueueReceive(log_queue, &temp_data, pdMS_TO_TICKS(SD_FORCE_WRITE_TIMEOUT_MS));

        if (xStatus == pdPASS) {
            if (!is_mounted) continue;
            // HAL_GPIO_WritePin(SD_STATUS_GPIO_Port, SD_STATUS_Pin, GPIO_PIN_RESET);

            int len = board_data_serialize(&temp_data, temp_str, sizeof(temp_str));
            if (len > 0) {
                if ((active_buffer_pos + len) >= SD_BUFFER_BYTES) {
                    uint8_t ready_idx = active_idx;
                    bytes_to_write[ready_idx] = active_buffer_pos; 

                    //active_idx = !active_idx;
                    //active_buffer_pos = 0;

                    UINT bytes_written;
                    size_t write_size = bytes_to_write[ready_idx];
                    xSemaphoreTake(sd_mutex, portMAX_DELAY);
                    if (is_mounted) {
                        FRESULT res = f_write(&log_file, double_buffer[ready_idx], write_size, &bytes_written);
                        f_sync(&log_file); 

                        if (res != FR_OK || bytes_written != write_size) {
                            // HANDLE WRITE ERROR
                        }
                    }
                    xSemaphoreGive(sd_mutex);                    

                    // xSemaphoreTake(buffer_free_sem[active_idx], portMAX_DELAY);
                    // xTaskNotify(sd_task_handle, ready_idx, eSetValueWithOverwrite);
                }

                memcpy(&double_buffer[active_idx][active_buffer_pos], temp_str, len);
                active_buffer_pos += len;
            }
        } else {
            if (is_mounted && active_buffer_pos > 0) {
                uint8_t ready_idx = active_idx;
                bytes_to_write[ready_idx] = active_buffer_pos; 

                //active_idx = !active_idx;
                //active_buffer_pos = 0; 

                UINT bytes_written;
                size_t write_size = bytes_to_write[ready_idx];
                xSemaphoreTake(sd_mutex, portMAX_DELAY);
                if (is_mounted) {
                    FRESULT res = f_write(&log_file, double_buffer[ready_idx], write_size, &bytes_written);
                    f_sync(&log_file); 

                    if (res != FR_OK || bytes_written != write_size) {
                        // HANDLE WRITE ERROR
                    }
                }
                xSemaphoreGive(sd_mutex);

                //xSemaphoreTake(buffer_free_sem[active_idx], portMAX_DELAY);
                //xTaskNotify(sd_task_handle, ready_idx, eSetValueWithOverwrite);
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
        size_t write_size = bytes_to_write[ready_buffer_idx];

        xSemaphoreTake(sd_mutex, portMAX_DELAY);
        if (is_mounted) {
            FRESULT res = f_write(&log_file, double_buffer[ready_buffer_idx], write_size, &bytes_written);
            f_sync(&log_file); 

            if (res != FR_OK || bytes_written != write_size) {
                // HANDLE WRITE ERROR
            }
        }
        xSemaphoreGive(sd_mutex);

        xSemaphoreGive(buffer_free_sem[ready_buffer_idx]);
    }
}

static void monitor_task_thread(void *arg) {
    (void)arg;

    while(1) {
        bool is_card_inserted = (HAL_GPIO_ReadPin(SD_DETECT_GPIO_Port, SD_DETECT_Pin) == GPIO_PIN_RESET);

        if (is_card_inserted && !is_mounted) {
            sd_mount();
        } else if (!is_card_inserted && is_mounted) {
            sd_unmount();
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}