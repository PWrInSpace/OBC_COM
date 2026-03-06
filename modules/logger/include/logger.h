/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#pragma once

#include <stdint.h>

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_MAX
} log_level_t;

/* Callback dla wysyłania logów w real-time */
typedef void (*logger_output_callback_t)(const char* data, uint16_t len);

/* Publiczne funkcje loggera */
void logger_init(void);
void logger_set_level(log_level_t level);
void log_info(const char* file, int line, const char* fmt, ...);
void log_error(const char* file, int line, const char* fmt, ...);
void log_debug(const char* file, int line, const char* fmt, ...);

/* Ustawianie callback'a dla real-time wysyłania logów */
void logger_set_output_callback(logger_output_callback_t callback);

/* API do odczytu logów z ring buffera */
const char* logger_get_buffer(void);
uint32_t logger_get_write_position(void);
void logger_clear(void);

/* Makra dostępne tylko po include logger_macros.h */
// #ifdef LOGGER_USE_MACROS
#define LOG_INFO(fmt, ...) log_info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log_debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
// #endif