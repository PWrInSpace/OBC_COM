/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 26.09.2026
 */
#include "buzzer.h"
#include "logger.h"

#ifdef BUZZER_POLY_ENABLE

#define BUZZER_POLY_TIMCLK_HZ  64000000u   //!< MUST match the TIM kernel clock
#define BUZZER_SIN_LUT_N       1024u
#define BUZZER_SIN_LUT_BITS    10u         //!< log2(BUZZER_SIN_LUT_N)
#define BUZZER_POLY_MAX_VOICES 4u

static int16_t s_sin_lut[BUZZER_SIN_LUT_N];
static uint8_t s_lut_ready = 0u;

/* One period of sine as Q15, built with a parabolic approximation refined once
 * (~0.1% error). Uses only FPU add/sub/mul (no libm: avoids sin/lrint). The
 * table holds a full period, so any phase offset is irrelevant. */
static int16_t buzzer_approx_sin(uint32_t i, uint32_t n) {
    float x  = (2.0f * (float)i / (float)n) - 1.0f;   // x in [-1, 1) -> angle pi*x
    float ax = (x < 0.0f) ? -x : x;
    float y  = (1.27323954f * x) + (-0.405284735f * x * ax);  // 4/pi, -4/pi^2
    float ay = (y < 0.0f) ? -y : y;
    y = 0.225f * ((y * ay) - y) + y;                   // refinement pass
    int32_t q = (int32_t)(y * 32767.0f);
    if (q > 32767)  q = 32767;
    if (q < -32768) q = -32768;
    return (int16_t)q;
}

static void buzzer_poly_lut_build(void) {
    if (s_lut_ready) return;
    for (uint32_t i = 0; i < BUZZER_SIN_LUT_N; i++) {
        s_sin_lut[i] = buzzer_approx_sin(i, BUZZER_SIN_LUT_N);
    }
    s_lut_ready = 1u;
}
#endif /* BUZZER_POLY_ENABLE */

buzzer_error_t buzzer_init(buzzer_t *dev, const buzzer_t *config) {
    if (dev == NULL || config == NULL) return BUZZER_ERROR;
    *dev = *config;

    if (dev->htim == NULL || dev->timer_hz == 0U) return BUZZER_ERROR;

#ifdef BUZZER_POLY_ENABLE
    buzzer_poly_lut_build();
#endif

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

#ifdef BUZZER_POLY_ENABLE

buzzer_error_t buzzer_poly_enter(buzzer_t *dev, uint16_t *duty_steps, uint32_t *fs) {
    if (dev == NULL || dev->htim == NULL || dev->carrier_hz == 0U) return BUZZER_ERROR;

    uint32_t arr = BUZZER_POLY_TIMCLK_HZ / dev->carrier_hz;
    if (arr == 0U) arr = 1U;
    arr -= 1U;

    HAL_TIM_PWM_Stop(dev->htim, dev->channel);                 // stop before reprogram
    __HAL_TIM_SET_PRESCALER(dev->htim, 0U);                    // full kernel clock
    __HAL_TIM_SET_AUTORELOAD(dev->htim, arr);
    __HAL_TIM_SET_COMPARE(dev->htim, dev->channel, (arr + 1U) / 2U);  // 50% bias
    (void)HAL_TIM_GenerateEvent(dev->htim, TIM_EVENTSOURCE_UPDATE);   // latch PSC/ARR

    if (duty_steps != NULL) *duty_steps = (uint16_t)(arr + 1U);
    if (fs != NULL)         *fs = BUZZER_POLY_TIMCLK_HZ / (arr + 1U);
    return BUZZER_OK;
}

static uint32_t buzzer_gcd_u32(uint32_t a, uint32_t b) {
    while (b != 0U) { uint32_t t = a % b; a = b; b = t; }
    return a;
}

buzzer_error_t buzzer_render_chord(const uint16_t *freqs, uint8_t nvoices,
                                   uint16_t *duty, uint16_t len_cap,
                                   uint32_t fs, uint16_t duty_steps,
                                   uint16_t *len_out) {
    if (freqs == NULL || duty == NULL || len_cap == 0U || fs == 0U ||
        duty_steps == 0U || nvoices == 0U || nvoices > BUZZER_POLY_MAX_VOICES) {
        return BUZZER_ERROR;
    }
    buzzer_poly_lut_build();

    uint32_t acc[BUZZER_POLY_MAX_VOICES]  = {0};
    uint32_t incr[BUZZER_POLY_MAX_VOICES] = {0};
    uint32_t g = 0U;
    for (uint8_t v = 0; v < nvoices; v++) {
        if (freqs[v] == 0U) return BUZZER_ERROR;
        /* per-sample phase step for the actual frequency (grid = 1 Hz, so the
         * tone itself is exact; the loop length below closes the period). */
        incr[v] = (uint32_t)((((uint64_t)freqs[v] << 32) + (fs / 2U)) / fs);
        g = buzzer_gcd_u32(g, freqs[v]);       /* gcd of integer-Hz frequencies */
    }

    /* Minimal seamless loop = one combined period = fs / gcd(freqs) samples.
     * Single tone / harmonic chord -> tiny; near-coprime -> up to len_cap. */
    uint32_t len = (g != 0U) ? ((fs + (g / 2U)) / g) : (uint32_t)len_cap;
    if (len == 0U)       len = 1U;
    if (len > len_cap)   len = len_cap;        /* cap: tiny loop discontinuity */

    const int32_t bias = (int32_t)duty_steps / 2;
    const int32_t ampl = (int32_t)duty_steps * 45 / 100;   // headroom below full scale
    const int32_t dmin = (int32_t)duty_steps *  5 / 100;   // keep off 0% / 100%
    const int32_t dmax = (int32_t)duty_steps * 95 / 100;

    for (uint32_t n = 0; n < len; n++) {
        int32_t s = 0;
        for (uint8_t v = 0; v < nvoices; v++) {
            s += s_sin_lut[(acc[v] >> (32U - BUZZER_SIN_LUT_BITS)) & (BUZZER_SIN_LUT_N - 1U)];
            acc[v] += incr[v];
        }
        s /= (int32_t)nvoices;                 // average voices -> Q15 ~[-1,1]
        int32_t d = bias + ((s * ampl) >> 15);
        if (d < dmin) d = dmin;
        if (d > dmax) d = dmax;
        duty[n] = (uint16_t)d;
    }

    if (len_out != NULL) *len_out = (uint16_t)len;
    return BUZZER_OK;
}

buzzer_error_t buzzer_poly_start(buzzer_t *dev, const uint16_t *duty, uint16_t len) {
    if (dev == NULL || dev->htim == NULL || duty == NULL || len == 0U) return BUZZER_ERROR;

    /* The channel DMA must be CIRCULAR (CubeMX) for continuous looping.
     * On this GPDMA, the HAL length is in BYTES (as in the WS2812 driver):
     * samples * sizeof(uint16_t). */
    if (HAL_TIM_PWM_Start_DMA(dev->htim, dev->channel,
                              (const uint32_t *)duty,
                              (uint16_t)((uint32_t)len * sizeof(uint16_t))) != HAL_OK) {
        return BUZZER_ERROR;
    }
    return BUZZER_OK;
}

buzzer_error_t buzzer_poly_swap(buzzer_t *dev, const uint16_t *duty, uint16_t len) {
    if (dev == NULL || dev->htim == NULL) return BUZZER_ERROR;
    HAL_TIM_PWM_Stop_DMA(dev->htim, dev->channel);
    return buzzer_poly_start(dev, duty, len);
}

buzzer_error_t buzzer_poly_stop(buzzer_t *dev) {
    if (dev == NULL || dev->htim == NULL) return BUZZER_ERROR;
    HAL_TIM_PWM_Stop_DMA(dev->htim, dev->channel);
    __HAL_TIM_SET_COMPARE(dev->htim, dev->channel, 0U);  // silent, no DC through coil
    return BUZZER_OK;
}

buzzer_error_t buzzer_tone_enter(buzzer_t *dev) {
    if (dev == NULL || dev->htim == NULL || dev->timer_hz == 0U) return BUZZER_ERROR;

    HAL_TIM_PWM_Stop_DMA(dev->htim, dev->channel);
    uint32_t psc = BUZZER_POLY_TIMCLK_HZ / dev->timer_hz;
    if (psc == 0U) psc = 1U;
    __HAL_TIM_SET_PRESCALER(dev->htim, psc - 1U);
    (void)HAL_TIM_GenerateEvent(dev->htim, TIM_EVENTSOURCE_UPDATE);
    return buzzer_mute(dev);
}

#endif /* BUZZER_POLY_ENABLE */
