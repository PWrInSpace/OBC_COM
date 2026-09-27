/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 27.09.2026
 */
#include "led.h"
#include "ws2812.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

#define LED_COUNT                   5U

#define LED_REFRESH_RATE_HZ         45U
#define LED_FRAME_MS                (1000U / LED_REFRESH_RATE_HZ)

#define LED_BRIGHTNESS_PERCENT      18U

#define RAINBOW_HUE_STEP_PER_FRAME  1U
#define RAINBOW_HUE_STEP_PER_LED    (256U / LED_COUNT)

#define LED_BLINK_ON_MS             300U
#define LED_BLINK_OFF_MS            300U
#define LED_BLINK_ON_FRAMES         (LED_BLINK_ON_MS  / LED_FRAME_MS)
#define LED_BLINK_OFF_FRAMES        (LED_BLINK_OFF_MS / LED_FRAME_MS)

static ws2812_t g_ws;
static osTimerId_t g_timer;

static volatile led_effect_t g_effect[LED_COUNT];
static volatile led_color_t g_color[LED_COUNT];
static uint32_t g_frame;

static inline uint8_t scale(uint8_t c) { return (uint8_t)(((uint16_t)c * LED_BRIGHTNESS_PERCENT) / 100u); }

static inline void unpack(led_color_t c, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (uint8_t)((c >> 16) & 0xFFu);
    *g = (uint8_t)((c >> 8) & 0xFFu);
    *b = (uint8_t)(c & 0xFFu);
}

// RGB color wheel (for rainbow effect)
static void wheel(uint8_t pos, uint8_t *r, uint8_t *g, uint8_t *b) {
    pos = (uint8_t)(255u - pos);
    if (pos < 85u) {
        *r = (uint8_t)(255u - pos * 3u);
        *g = 0u;
        *b = (uint8_t)(pos * 3u);
    } else if (pos < 170u) {
        pos = (uint8_t)(pos - 85u);
        *r = 0u;
        *g = (uint8_t)(pos * 3u);
        *b = (uint8_t)(255u - pos * 3u);
    } else {
        pos = (uint8_t)(pos - 170u);
        *r = (uint8_t)(pos * 3u);
        *g = (uint8_t)(255u - pos * 3u);
        *b = 0u;
    }
}

static void led_render_pixel(uint16_t i, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = 0u; *g = 0u; *b = 0u;

    switch (g_effect[i]) {
        case LED_EFFECT_SOLID:
            unpack(g_color[i], r, g, b);
            break;

        case LED_EFFECT_BLINK:
            if ((g_frame % (LED_BLINK_ON_FRAMES + LED_BLINK_OFF_FRAMES)) < LED_BLINK_ON_FRAMES) {
                unpack(g_color[i], r, g, b);
            }
            break;

        case LED_EFFECT_RAINBOW:
            wheel((uint8_t)(g_frame * RAINBOW_HUE_STEP_PER_FRAME + i * RAINBOW_HUE_STEP_PER_LED), r, g, b);
            break;

        default: break;
    }
}

static void led_cb(void *arg) {
    (void)arg;

    for (uint16_t i = 0; i < LED_COUNT; i++) {
        uint8_t r, g, b;
        led_render_pixel(i, &r, &g, &b);
        ws2812_set_pixel(&g_ws, i, scale(r), scale(g), scale(b));
    }
    g_frame++;
    ws2812_show(&g_ws);
}

void led_init(void) {
    const ws2812_t cfg = { .htim = &htim4, .channel = TIM_CHANNEL_4, .num_leds = LED_COUNT };
    ws2812_init(&g_ws, &cfg);

    for (uint16_t i = 0; i < LED_COUNT; i++) {
        g_effect[i] = LED_EFFECT_OFF;
        g_color[i] = LED_COLOR_OFF;
    }

    g_timer = osTimerNew(led_cb, osTimerPeriodic, NULL, NULL);
    if (g_timer) osTimerStart(g_timer, LED_FRAME_MS);
}

void led_set(uint16_t index, led_effect_t effect, led_color_t color) {
    if (index >= LED_COUNT || effect >= LED_EFFECT_COUNT) return;

    taskENTER_CRITICAL();
    g_effect[index] = effect;
    g_color[index] = color;
    taskEXIT_CRITICAL();
}

void led_set_all(led_effect_t effect, led_color_t color) { for (uint16_t i = 0; i < LED_COUNT; i++) led_set(i, effect, color); }
void led_stop(void) { led_set_all(LED_EFFECT_OFF, LED_COLOR_OFF); }
