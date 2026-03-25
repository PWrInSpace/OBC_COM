#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t timestamp_ms;
    float temperature;
    float pressure;
    uint32_t status_flags;
} BoardData_t;

/**
 * @brief Serializes the BoardData_t struct into a string.
 * @param data Pointer to the struct to serialize.
 * @param out_buffer Pointer to the character buffer where the string will be written.
 * @param buffer_size The maximum size of out_buffer to prevent overflows.
 * @return The number of characters written, or a negative number on error/truncation.
 */
int board_data_serialize(const BoardData_t *data, char *out_buffer, size_t buffer_size);

/**
 * @brief Generates the header string.
 * @param out_buffer Pointer to the character buffer where the string will be written.
 * @param buffer_size The maximum size of out_buffer to prevent overflows.
 * @return The number of characters written, or a negative number on error/truncation.
 */
int board_data_get_header(char *out_buffer, size_t buffer_size);