/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 23.03.2026
 */
#pragma once

#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdint.h>
#include <stdbool.h>
#include "board_data.h"

#define LOG_FILE_NAME "data_WOW.csv"
#define SD_QUEUE_LENGTH 50
#define SD_BUFFER_BYTES 4096

HAL_StatusTypeDef sd_logger_init(void);
HAL_StatusTypeDef sd_logger_log_data(const BoardData_t *data);

// NOT THREAD SAFE
bool sd_mount(void);
void sd_unmount(void);
bool sd_remount(void);
void sd_sync(void);
bool sd_is_mounted(void);