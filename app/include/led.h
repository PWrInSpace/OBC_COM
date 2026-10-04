/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 27.09.2026
 */
#ifndef LED_H
#define LED_H

#include <stdint.h>

typedef enum {
    LED_COLOR_OFF = 0x000000,
    LED_COLOR_RED = 0xFF0000,
    LED_COLOR_GREEN = 0x00FF00,
    LED_COLOR_BLUE = 0x0000FF,
    LED_COLOR_YELLOW = 0xFF8800,
    LED_COLOR_CYAN = 0x00FFFF,
    LED_COLOR_MAGENTA = 0xFF0088,
    LED_COLOR_PURPLE = 0x8800FF,
    LED_COLOR_WHITE = 0xFFFFFF,
} led_color_t;

typedef enum {
    LED_EFFECT_OFF = 0,
    LED_EFFECT_SOLID,
    LED_EFFECT_BLINK,
    LED_EFFECT_RAINBOW,
    LED_EFFECT_COUNT
} led_effect_t;

void led_init(void);

// Set one LED's effect + colour. `color` applies to all effects except RAINBOW
void led_set(uint16_t index, led_effect_t effect, led_color_t color);
// Set all LED's effect + colour. `color` applies to all effects except RAINBOW
void led_set_all(led_effect_t effect, led_color_t color);
void led_stop(void);

// Emergency (fault handler) helpers: synchronous, no RTOS. Use ONLY when the
// system is presumed dead (e.g. hardfault).
// Whole strip one colour
void led_fault_color(led_color_t color);
// Per-LED: set individual pixels, then flush them with led_fault_show()
void led_fault_pixel(uint16_t index, led_color_t color);
void led_fault_show(void);

#endif /* LED_H */
