/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 26.09.2026
 */
#include "buzzer.h"
#include "logger.h"

buzzer_error_t buzzer_init(buzzer_t *dev, const buzzer_t *config) {
    if (dev == NULL || config == NULL) return BUZZER_ERROR;
    *dev = *config;

    if (dev->htim == NULL || dev->timer_hz == 0U) return BUZZER_ERROR;

    buzzer_mute(dev);
    return BUZZER_OK;
}

buzzer_error_t buzzer_set_tone(buzzer_t *dev, uint32_t freq_hz) {
    if (dev == NULL || dev->htim == NULL) return BUZZER_ERROR;
    if (freq_hz == 0U) return buzzer_mute(dev);

    uint32_t arr = dev->timer_hz / freq_hz;
    if (arr == 0U) arr = 1U;
    arr -= 1U;

    __HAL_TIM_SET_AUTORELOAD(dev->htim, arr);
    __HAL_TIM_SET_COMPARE(dev->htim, dev->channel, (arr + 1U) / 2U);  // ~50% duty

    if (HAL_TIM_PWM_Start(dev->htim, dev->channel) != HAL_OK) return BUZZER_ERROR;
    return BUZZER_OK;
}

buzzer_error_t buzzer_mute(buzzer_t *dev) {
    if (dev == NULL || dev->htim == NULL) return BUZZER_ERROR;
    HAL_TIM_PWM_Stop(dev->htim, dev->channel);
    return BUZZER_OK;
}
