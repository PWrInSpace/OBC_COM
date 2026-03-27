#include "board_data.h"
#include <stdio.h>
#include <stdlib.h>

BoardData_t g_system_state = {0};
SemaphoreHandle_t g_state_mutex = NULL;

void board_data_init(void) {
    g_state_mutex = xSemaphoreCreateMutex();
    xSemaphoreGive(g_state_mutex);
}

int board_data_serialize(const BoardData_t *data, char *out_buffer, size_t buffer_size) {
    if (data == NULL || out_buffer == NULL || buffer_size == 0) return -1;
    
    int chars_written = snprintf(out_buffer, buffer_size, 
        "%lu,%.2f,%.2f,%lu,%d,%ld.%07ld,%ld.%07ld,%ld,%ld,%u,%u\r\n",
        data->timestamp_ms,
        data->temperature,
        data->pressure,
        data->status_flags,
        data->RSSI,
        (long)(data->lat / 10000000), (unsigned long)abs(data->lat % 10000000),
        (long)(data->lon / 10000000), (unsigned long)abs(data->lon % 10000000),
        data->hMSL,
        data->gSpeed,
        data->numSV,
        data->fixType
    );

    if (chars_written >= 0 && (size_t)chars_written < buffer_size) return chars_written;
    return -1;
}

int board_data_get_header(char *out_buffer, size_t buffer_size) {
    if (out_buffer == NULL || buffer_size == 0) return -1;

    int chars_written = snprintf(out_buffer, buffer_size, 
        "Timestamp_ms,Temp_C,Press_hPa,Flags,RSSI,Lat,Lon,hMSL_mm,Speed_mm_s,Sats,Fix\r\n");

    if (chars_written >= 0 && (size_t)chars_written < buffer_size) return chars_written;
    return -1;
}