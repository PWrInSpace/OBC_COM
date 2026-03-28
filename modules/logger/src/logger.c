#include "logger.h"
#include "logger_serial_output.h"

#include "stm32h5xx.h"

#include <stdarg.h>
#include <stdio.h>
#include "nvs_config.h"



/* Ring buffer do logowania bez używania UART */
#define LOGGER_BUFFER_SIZE 4096
//#define EXTENDED_LOG 1

static char logger_buffer[512];
static char log_ring_buffer[LOGGER_BUFFER_SIZE];
static uint32_t ring_write_pos = 0;
static log_level_t current_log_level = LOG_LEVEL_INFO;
static logger_output_callback_t output_callback = NULL;

#define MAX_FILTERED_TAGS 10
static char filtered_tags[MAX_FILTERED_TAGS][12]; // Tablica 10 tagów po max 12 znaków
static uint8_t filtered_tags_count = 0;


static bool global_log_enabled = true;

void logger_enable(bool enable) {
    global_log_enabled = enable;
}
/* Funkcja pomocnicza do zapisu do ring buffera */
static void logger_write_to_ring(const char* msg) {
    if (msg == NULL) return;
    
    uint16_t len = strlen(msg);
    
    for (uint16_t i = 0; i < len; i++) {
        log_ring_buffer[ring_write_pos] = msg[i];
        ring_write_pos = (ring_write_pos + 1) % LOGGER_BUFFER_SIZE;
    }
}

// Funkcja do dodawania taga do czarnej listy
void logger_filter_add(const char* tag) {
    if (filtered_tags_count < MAX_FILTERED_TAGS) {
        strncpy(filtered_tags[filtered_tags_count], tag, 11);
        filtered_tags[filtered_tags_count][11] = '\0';
        filtered_tags_count++;
    }
}

// Funkcja do czyszczenia filtrów
void logger_filter_clear(void) {
    filtered_tags_count = 0;
}



/* Funkcja pomocnicza do wysyłania przez callback */
static void logger_send_via_callback(const char* msg) {
    if (output_callback != NULL && msg != NULL) {
        uint16_t len = strlen(msg);
        output_callback(msg, len);
    }
}static void logger_format_and_write(const char* tag, 
                                    const char* level, 
                                    const char* file, 
                                    int line, 
                                    const char* fmt, 
                                    va_list args)
{

    // 1. Sprawdź, czy logowanie ogólne jest włączone
    if (!global_log_enabled) return;

    // 2. FILTROWANIE PO TAGU
    for (uint8_t i = 0; i < filtered_tags_count; i++) {
        if (strcmp(tag, filtered_tags[i]) == 0) {
            return; // Ten tag jest zablokowany - wyjdź z funkcji
        }
    }
    /* 1. Formatuj treść użytkownika */
    vsnprintf(logger_buffer, sizeof(logger_buffer), fmt, args);

    /* 2. Pobierz czas */
    uint32_t timestamp = HAL_GetTick();

    /* 3. Statyczny bufor (bezpieczeństwo stosu!) */
    static char formatted[768];

#ifdef EXTENDED_LOG
    /* Wyciąganie nazwy pliku (Twoja poprzednia logika) */
    const char *filename = file;
    const char *slash1 = strrchr(file, '/');
    const char *slash2 = strrchr(file, '\\');
    if (slash1 || slash2) {
        const char *last_slash = (slash1 > slash2) ? slash1 : slash2;
        filename = last_slash + 1;
    }

    // Format rozszerzony: [Czas] [TAG:LEVEL] Plik:Linia - Wiadomość
    snprintf(formatted, sizeof(formatted),
             "[%lu ms] [%s:%s] %s:%d - %s\r\n",
             timestamp, tag, level, filename, line, logger_buffer);
#else
    // Format lekki: [Czas] [TAG:LEVEL] Wiadomość
    snprintf(formatted, sizeof(formatted),
             "[%lu ms] [%s:%s] %s\r\n",
             timestamp, tag, level, logger_buffer);
#endif

    logger_write_to_ring(formatted);
    logger_send_via_callback(formatted);
}

void logger_init(void) {
   // uint32_t default_muted = true; // Tworzymy zmienną w RAM
   // nvs_set_log_muted(default_muted); // Przekazujemy adres (&)
    bool is_muted = false;
   nvs_get_log_muted(&is_muted);
    global_log_enabled = !is_muted;

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

void log_info(const char* tag, const char* level, const char* file, int line, const char* fmt, ...) {
    if (current_log_level > LOG_LEVEL_INFO) return;

    va_list args;
    va_start(args, fmt);
    logger_format_and_write(tag, level, file, line, fmt, args);
    va_end(args);
}

void log_error(const char* tag, const char* level, const char* file, int line, const char* fmt, ...) {
    if (current_log_level > LOG_LEVEL_ERROR) return;

    va_list args;
    va_start(args, fmt);
    logger_format_and_write(tag, level, file, line, fmt, args);
    va_end(args);
}

void log_debug(const char* tag, const char* level, const char* file, int line, const char* fmt, ...) {
    if (current_log_level > LOG_LEVEL_DEBUG) return;

    va_list args;
    va_start(args, fmt);
    logger_format_and_write(tag, level, file, line, fmt, args);
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

/**
 * @brief Włącza lub wyłącza proces logowania globalnie.
 * @param enable true aby włączyć, false aby wyłączyć.
 */
