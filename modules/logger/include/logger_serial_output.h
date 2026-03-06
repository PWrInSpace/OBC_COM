/*
 * Logger Serial Output Helper
 * Wysyła logi na serial port (USB CDC lub UART)
 * Author: PWr in Space
 */

#pragma once

#include "logger.h"

/* Inicjalizacja serial output do logów */
void logger_serial_init(void);

/* Callback dla serial output - implementacja zależy od interfejsu */
void logger_serial_output_callback(const char* data, uint16_t len);
