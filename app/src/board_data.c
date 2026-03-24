#include "board_data.h"
#include <stdio.h>

int board_data_serialize(const BoardData_t *data, char *out_buffer, size_t buffer_size) {
    if (data == NULL || out_buffer == NULL || buffer_size == 0) return -1;

    int chars_written = snprintf(out_buffer, buffer_size, "%lu,%.2f,%.2f,%lu\r\n",
                                    data->timestamp_ms,
                                    data->temperature,
                                    data->pressure,
                                    data->status_flags
                                );


    if (chars_written >= 0 && (size_t)chars_written < buffer_size) return chars_written;
    return -1;
}

int board_data_get_header(char *out_buffer, size_t buffer_size) {
    if (out_buffer == NULL || buffer_size == 0) return -1;

    int chars_written = snprintf(out_buffer, buffer_size, "Timestamp_ms,Temperature_C,Pressure_hPa,Status_Flags\r\n");

    if (chars_written >= 0 && (size_t)chars_written < buffer_size) return chars_written;
    return -1;
}