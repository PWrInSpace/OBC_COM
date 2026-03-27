/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 26.03.2026
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "semphr.h"

typedef struct {
    uint32_t timestamp_ms;
    float temperature;
    float pressure;
    uint32_t status_flags;
    // Dane RFM95W
    int16_t RSSI;
    // Dane GPS
    int32_t lat;
    int32_t lon;
    int32_t hMSL;
    int32_t gSpeed;
    uint8_t numSV;
    uint8_t fixType;
} BoardData_t;

extern BoardData_t g_system_state;
extern SemaphoreHandle_t g_state_mutex;

void board_data_init(void);
int board_data_serialize(const BoardData_t *data, char *out_buffer, size_t buffer_size);
int board_data_get_header(char *out_buffer, size_t buffer_size);