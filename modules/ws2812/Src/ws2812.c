/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 27.09.2026
 */
#include "ws2812.h"
#include "tim.h"
#include <string.h>

// Timer runs at 800 kHz: ARR+1 = 80 ticks = 1.25 us per bit.
#define WS2812_BITS_PER_LED 24
#define WS2812_DUTY_LO 26u     // 0.40 us / 1.25 us * 80
#define WS2812_DUTY_HI 51u     // 0.80 us / 1.25 us * 80

#define WS2812_RESET_SLOTS 48u   // 48 * 1.25 us = 60 us

#define WS2812_BUF_LEN (WS2812_MAX_LEDS * WS2812_BITS_PER_LED + WS2812_RESET_SLOTS)

static uint8_t  s_grb[WS2812_MAX_LEDS][3];  // GBR
static uint16_t s_dma[WS2812_BUF_LEN];
static volatile bool s_busy = false;
static ws2812_t *s_active = NULL;

ws2812_error_t ws2812_init(ws2812_t *dev, const ws2812_t *config) {
    if (dev == NULL || config == NULL || config->htim == NULL) return WS2812_ERROR;
    if (config->num_leds == 0 || config->num_leds > WS2812_MAX_LEDS) return WS2812_ERROR;

    *dev = *config;
    memset(s_grb, 0, sizeof(s_grb));
    memset(s_dma, 0, sizeof(s_dma));
    return WS2812_OK;
}

void ws2812_set_pixel(ws2812_t *dev, uint16_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (dev == NULL || idx >= dev->num_leds) return;
    s_grb[idx][0] = g;
    s_grb[idx][1] = r;
    s_grb[idx][2] = b;
}

void ws2812_set_all(ws2812_t *dev, uint8_t r, uint8_t g, uint8_t b) {
    if (dev == NULL) return;
    for (uint16_t i = 0; i < dev->num_leds; i++) ws2812_set_pixel(dev, i, r, g, b);
}

void ws2812_clear(ws2812_t *dev) {
    if (dev == NULL) return;
    memset(s_grb, 0, sizeof(s_grb));
}

ws2812_error_t ws2812_show(ws2812_t *dev) {
    if (dev == NULL || dev->htim == NULL) return WS2812_ERROR;
    if (s_busy) return WS2812_BUSY;

    uint16_t k = 0;
    for (uint16_t led = 0; led < dev->num_leds; led++) {
        for (uint8_t byte = 0; byte < 3; byte++) {
            uint8_t val = s_grb[led][byte];
            for (int8_t bit = 7; bit >= 0; bit--) {
                s_dma[k++] = (val & (1u << bit)) ? WS2812_DUTY_HI : WS2812_DUTY_LO;
            }
        }
    }
    for (uint16_t i = 0; i < WS2812_RESET_SLOTS; i++) s_dma[k++] = 0;

    s_active = dev;
    s_busy = true;

    if (HAL_TIM_PWM_Start_DMA(dev->htim, dev->channel,
                              (const uint32_t *)s_dma,
                              (uint16_t)(k * sizeof(uint16_t))) != HAL_OK) {
        s_busy = false;
        return WS2812_ERROR;
    }

    return WS2812_OK;
}

bool ws2812_is_busy(void) { return s_busy; }

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    if (s_active != NULL && htim->Instance == s_active->htim->Instance) {
        HAL_TIM_PWM_Stop_DMA(htim, s_active->channel);
        s_busy = false;
    }
}
