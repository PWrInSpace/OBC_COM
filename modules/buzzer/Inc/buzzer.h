/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 26.09.2026
 */
#pragma once

#include <stdint.h>
#include "main.h"

typedef enum {
    BUZZER_OK = 0,
    BUZZER_ERROR = 1,
} buzzer_error_t;

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint32_t timer_hz;
} buzzer_t;

buzzer_error_t buzzer_init(buzzer_t *dev, const buzzer_t *config);

buzzer_error_t buzzer_set_tone(buzzer_t *dev, uint32_t freq_hz);

buzzer_error_t buzzer_mute(buzzer_t *dev);
