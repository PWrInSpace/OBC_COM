/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 27.09.2026
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

#define WS2812_MAX_LEDS 5

typedef enum {
    WS2812_OK = 0,
    WS2812_ERROR = 1,
    WS2812_BUSY = 2,
} ws2812_error_t;

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint16_t num_leds;
} ws2812_t;

ws2812_error_t ws2812_init(ws2812_t *dev, const ws2812_t *config);

void ws2812_set_pixel(ws2812_t *dev, uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
void ws2812_set_all(ws2812_t *dev, uint8_t r, uint8_t g, uint8_t b);
void ws2812_clear(ws2812_t *dev);

/** 
 * @brief Encode the frame and begin the DMA transfer (non-blocking)
 * @return WS2812_BUSY - the previous transfer is still in flight
*/
ws2812_error_t ws2812_show(ws2812_t *dev);

bool ws2812_is_busy(void);

// use ONLY in emergency when system is presumed dead (e.g. hardfault)
ws2812_error_t ws2812_set_color_blocking(ws2812_t *dev, uint8_t r, uint8_t g, uint8_t b);
