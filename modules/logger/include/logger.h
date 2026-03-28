/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifndef TAG
#define TAG "SYS"
#endif

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_ERROR = 2,
    LOG_LEVEL_NONE  = 3,
    LOG_LEVEL_MAX
} log_level_t;

/* Callback dla wysyłania logów w real-time */
typedef void (*logger_output_callback_t)(const char* data, uint16_t len);

void logger_init(void);
void logger_set_level(log_level_t level);

void logger_set_output_callback(logger_output_callback_t callback);


const char* logger_get_buffer(void);
uint32_t logger_get_write_position(void);
void logger_clear(void);

#ifdef LOG_DISABLED
    #define LOG_INFO(fmt, ...)  ((void)0)
    #define LOG_ERROR(fmt, ...) ((void)0)
    #define LOG_DEBUG(fmt, ...) ((void)0)
#else
    #define LOG_INFO(fmt, ...)  log_info(TAG, "INFO", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) log_error(TAG, "ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) log_debug(TAG, "DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#endif

// Deklaracje funkcji (dodajemy parametr 'level')
void log_info(const char* tag, const char* level, const char* file, int line, const char* fmt, ...);
void log_error(const char* tag, const char* level, const char* file, int line, const char* fmt, ...);
void log_debug(const char* tag, const char* level, const char* file, int line, const char* fmt, ...);
void logger_set_level(log_level_t level);
void logger_filter_add(const char* tag);
void logger_filter_clear(void);
void logger_enable(bool enable);

// #endif