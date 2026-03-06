#include "logger.h"
#include "logger_serial_output.h"

#include "stm32h5xx.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Ring buffer do logowania bez używania UART */
#define LOGGER_BUFFER_SIZE 4096

static char logger_buffer[512];
static char log_ring_buffer[LOGGER_BUFFER_SIZE];
static uint32_t ring_write_pos = 0;
static log_level_t current_log_level = LOG_LEVEL_INFO;
static logger_output_callback_t output_callback = NULL;

/* Funkcja pomocnicza do zapisu do ring buffera */
static void logger_write_to_ring(const char* msg) {
    if (msg == NULL) return;
    
    uint16_t len = strlen(msg);
    
    for (uint16_t i = 0; i < len; i++) {
        log_ring_buffer[ring_write_pos] = msg[i];
        ring_write_pos = (ring_write_pos + 1) % LOGGER_BUFFER_SIZE;
    }
}

/* Funkcja pomocnicza do wysyłania przez callback */
static void logger_send_via_callback(const char* msg) {
    if (output_callback != NULL && msg != NULL) {
        uint16_t len = strlen(msg);
        output_callback(msg, len);
    }
}

/* Funkcja pomocnicza do formatowania i zapisu wiadomości */
static void logger_format_and_write(const char* level,
                                    const char* file,
                                    int line,
                                    const char* fmt,
                                    va_list args)
{
    /* Wyciągnij samą nazwę pliku z pełnej ścieżki */
    const char *filename = file;
    const char *slash1 = strrchr(file, '/');
    const char *slash2 = strrchr(file, '\\');

    if (slash1 || slash2)
    {
        const char *last_slash = slash1 > slash2 ? slash1 : slash2;
        filename = last_slash + 1;
    }

    /* Sformatuj wiadomość użytkownika */
    vsnprintf(logger_buffer, sizeof(logger_buffer), fmt, args);

    /* Timestamp w ms od startu systemu */
    uint32_t timestamp = HAL_GetTick();

    /* Pełna wiadomość */
    char formatted[768];

    snprintf(formatted, sizeof(formatted),
             "[%lu ms] [%s] %s:%d - %s\r\n",
             timestamp,
             level,
             filename,
             line,
             logger_buffer);

    logger_write_to_ring(formatted);
    logger_send_via_callback(formatted);
}

void logger_init(void) {
    logger_set_level(LOG_LEVEL_INFO);
    memset(log_ring_buffer, 0, LOGGER_BUFFER_SIZE);
    ring_write_pos = 0;
    logger_set_output_callback(logger_serial_output_callback);
    logger_write_to_ring("\r\n=== Logger initialized ===\r\n");
}

void logger_set_level(log_level_t level) {
    current_log_level = level;
}

void logger_set_output_callback(logger_output_callback_t callback) {
    output_callback = callback;
}

void log_info(const char* file, int line, const char* fmt, ...) {
    if (current_log_level > LOG_LEVEL_INFO) return;

    va_list args;
    va_start(args, fmt);
    logger_format_and_write("INFO", file, line, fmt, args);
    va_end(args);
}

void log_error(const char* file, int line, const char* fmt, ...) {
    if (current_log_level > LOG_LEVEL_ERROR) return;

    va_list args;
    va_start(args, fmt);
    logger_format_and_write("ERROR", file, line, fmt, args);
    va_end(args);
}

void log_debug(const char* file, int line, const char* fmt, ...) {
    if (current_log_level > LOG_LEVEL_DEBUG) return;

    va_list args;
    va_start(args, fmt);
    logger_format_and_write("DEBUG", file, line, fmt, args);
    va_end(args);
}

/* Publiczne API do odczytu logów (np. przez debugger lub USART) */
const char* logger_get_buffer(void) {
    return log_ring_buffer;
}

uint32_t logger_get_write_position(void) {
    return ring_write_pos;
}

void logger_clear(void) {
    memset(log_ring_buffer, 0, LOGGER_BUFFER_SIZE);
    ring_write_pos = 0;
}

