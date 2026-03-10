/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 29.01.2026
 */

#pragma once

#include "stdint.h"
#include "stdbool.h"
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "spi.h"

void rfm95w_wrapper_init(void);
bool rfm95w_spi_transmit(uint8_t *in, uint8_t *out);
void rfm95w_delay(uint32_t ms);
bool rfm95w_gpio_set_level(uint16_t _gpio_num, uint8_t _level);

void rfm95w_log(const char *info);