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
#define WS2812_DUTY_LO 26u // 0.40 us / 1.25 us * 80
#define WS2812_DUTY_HI 51u // 0.80 us / 1.25 us * 80

#define WS2812_RESET_SLOTS 48u // 48 * 1.25 us = 60 us

#define WS2812_BUF_LEN                                                         \
  (WS2812_MAX_LEDS * WS2812_BITS_PER_LED + WS2812_RESET_SLOTS)

static uint8_t s_grb[WS2812_MAX_LEDS][3]; // GBR
static uint16_t s_dma[WS2812_BUF_LEN];
static volatile bool s_busy = false;
static ws2812_t *s_active = NULL;

ws2812_error_t ws2812_init(ws2812_t *dev, const ws2812_t *config) {
  if (dev == NULL || config == NULL || config->htim == NULL)
    return WS2812_ERROR;
  if (config->num_leds == 0 || config->num_leds > WS2812_MAX_LEDS)
    return WS2812_ERROR;

  *dev = *config;
  memset(s_grb, 0, sizeof(s_grb));
  memset(s_dma, 0, sizeof(s_dma));
  return WS2812_OK;
}

void ws2812_set_pixel(ws2812_t *dev, uint16_t idx, uint8_t r, uint8_t g,
                      uint8_t b) {
  if (dev == NULL || idx >= dev->num_leds)
    return;
  s_grb[idx][0] = g;
  s_grb[idx][1] = r;
  s_grb[idx][2] = b;
}

void ws2812_set_all(ws2812_t *dev, uint8_t r, uint8_t g, uint8_t b) {
  if (dev == NULL)
    return;
  for (uint16_t i = 0; i < dev->num_leds; i++)
    ws2812_set_pixel(dev, i, r, g, b);
}

void ws2812_clear(ws2812_t *dev) {
  if (dev == NULL)
    return;
  memset(s_grb, 0, sizeof(s_grb));
}

// Encode the frame buffer (s_grb) into per-bit PWM duties + reset slots.
// Returns the number of half-word slots written.
static uint16_t ws2812_encode(ws2812_t *dev) {
  uint16_t k = 0;
  for (uint16_t led = 0; led < dev->num_leds; led++) {
    for (uint8_t byte = 0; byte < 3; byte++) {
      uint8_t val = s_grb[led][byte];
      for (int8_t bit = 7; bit >= 0; bit--) {
        s_dma[k++] = (val & (1u << bit)) ? WS2812_DUTY_HI : WS2812_DUTY_LO;
      }
    }
  }
  for (uint16_t i = 0; i < WS2812_RESET_SLOTS; i++)
    s_dma[k++] = 0;
  return k;
}

ws2812_error_t ws2812_show(ws2812_t *dev) {
  if (dev == NULL || dev->htim == NULL)
    return WS2812_ERROR;
  if (s_busy)
    return WS2812_BUSY;

  uint16_t k = ws2812_encode(dev);
  s_active = dev;
  s_busy = true;

  if (HAL_TIM_PWM_Start_DMA(dev->htim, dev->channel, (const uint32_t *)s_dma,
                            (uint16_t)(k * sizeof(uint16_t))) != HAL_OK) {
    s_busy = false;
    return WS2812_ERROR;
  }

  return WS2812_OK;
}

bool ws2812_is_busy(void) { return s_busy; }

// Map a timer channel to the DMA handle HAL linked to it.
static DMA_HandleTypeDef *ws2812_hdma(ws2812_t *dev) {
  switch (dev->channel) {
  case TIM_CHANNEL_1:
    return dev->htim->hdma[TIM_DMA_ID_CC1];
  case TIM_CHANNEL_2:
    return dev->htim->hdma[TIM_DMA_ID_CC2];
  case TIM_CHANNEL_3:
    return dev->htim->hdma[TIM_DMA_ID_CC3];
  default:
    return dev->htim->hdma[TIM_DMA_ID_CC4];
  }
}

ws2812_error_t ws2812_show_blocking(ws2812_t *dev) {
  if (dev == NULL || dev->htim == NULL)
    return WS2812_ERROR;

  // Emergency use (e.g. fault handler): RTOS and the DMA IRQ are assumed dead.
  // Force any in-flight transfer to stop so the channel is usable again.
  HAL_TIM_PWM_Stop_DMA(dev->htim, dev->channel);
  s_busy = false;

  uint16_t k = ws2812_encode(dev); // sends the current buffer (per-LED colours)

  s_active = dev;
  if (HAL_TIM_PWM_Start_DMA(dev->htim, dev->channel, (const uint32_t *)s_dma,
                            (uint16_t)(k * sizeof(uint16_t))) != HAL_OK) {
    return WS2812_ERROR;
  }

  // Poll the DMA byte counter to completion instead of waiting on the IRQ.
  DMA_HandleTypeDef *hdma = ws2812_hdma(dev);
  uint32_t guard = 0;
  while ((hdma->Instance->CBR1 & DMA_CBR1_BNDT) != 0u) {
    if (++guard > 2000000u)
      break; // safety: never hang the caller
  }

  HAL_TIM_PWM_Stop_DMA(dev->htim, dev->channel);
  s_busy = false;
  return WS2812_OK;
}

ws2812_error_t ws2812_set_color_blocking(ws2812_t *dev, uint8_t r, uint8_t g,
                                         uint8_t b) {
  if (dev == NULL)
    return WS2812_ERROR;
  ws2812_set_all(dev, r, g, b);
  return ws2812_show_blocking(dev);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
  if (s_active != NULL && htim->Instance == s_active->htim->Instance) {
    HAL_TIM_PWM_Stop_DMA(htim, s_active->channel);
    s_busy = false;
  }
}
