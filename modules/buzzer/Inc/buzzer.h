/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 26.09.2026
 */
#pragma once

#include <stdint.h>
#include "main.h"

/* ============================================================================
 *  POLYPHONY (PWM-as-DAC) — EXPERIMENTAL, OPT-IN
 *  Uncomment BUZZER_POLY_ENABLE to compile the polyphonic path. All poly code
 *  in buzzer.h / buzzer.c lives behind this define.
 *
 *  !!!  HARDWARE WARNING — READ BEFORE ENABLING  !!!
 *   - D4 (flyback / freewheel diode across the speaker) MUST be fitted. The
 *     poly path hard-switches a ~48 kHz carrier into the inductive voice coil;
 *     without D4 the MOSFET takes inductive turn-off spikes (avalanche at
 *     Vds(max) ~30 V) -> heating/stress, and the PWM->average-current "DAC"
 *     linearity breaks. Mono buzzer_set_tone() is low-frequency and tolerates
 *     a missing D4; the poly carrier does NOT.
 *   - EMI: ~48 kHz switching into an inductive load injects ripple on the 3V3
 *     rail and radiates harmonics into the MHz range. Do NOT run during
 *     RF-critical operations (RFM 868 MHz / SX1280 2.4 GHz). For flight use the
 *     mono tone path (buzzer_set_tone); keep polyphony for ground/demo only.
 * ========================================================================== */
// #define BUZZER_POLY_ENABLE

typedef enum {
    BUZZER_OK = 0,
    BUZZER_ERROR = 1,
} buzzer_error_t;

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint32_t timer_hz;
#ifdef BUZZER_POLY_ENABLE
    uint32_t carrier_hz;
#endif
} buzzer_t;

buzzer_error_t buzzer_init(buzzer_t *dev, const buzzer_t *config);
buzzer_error_t buzzer_set_tone(buzzer_t *dev, uint32_t freq_hz);
buzzer_error_t buzzer_mute(buzzer_t *dev);

#ifdef BUZZER_POLY_ENABLE

/**
 * @brief Switch the timer into carrier mode (PSC=0, ARR = tim_clk/carrier_hz).
 * @param duty_steps  out: number of duty levels (ARR+1), i.e. full-scale.
 * @param fs          out: effective sample rate (one duty per carrier period).
 * @note Requires dev->carrier_hz set. Call before buzzer_poly_start.
 */
buzzer_error_t buzzer_poly_enter(buzzer_t *dev, uint16_t *duty_steps, uint32_t *fs);

/**
 * @brief Render a chord (sum of sines) into duty[] as a seamlessly loopable
 *        buffer. Frequencies are integer Hz; the buffer only needs ONE full
 *        combined period = fs / gcd(freqs) samples to loop cleanly, which is
 *        far smaller than len_cap for single tones / harmonic chords. The CPU
 *        renders only that many samples -> the optimization lives here.
 *        Pure function: Q15 sine LUT + phase accumulators, ~50% bias.
 * @param len_cap  buffer capacity (max samples; RAM upper bound, e.g. 8192).
 * @param len_out  out: actual samples filled = min(fs/gcd, len_cap). Pass THIS
 *                 length to buzzer_poly_start/swap for the DMA loop.
 * @param nvoices  1..BUZZER_POLY_MAX_VOICES.
 * @param fs/duty_steps  as returned by buzzer_poly_enter.
 * @note If fs/gcd exceeds len_cap (near-coprime freqs) it is capped, which
 *       leaves a tiny loop discontinuity (sub-degree phase step, inaudible).
 */
buzzer_error_t buzzer_render_chord(const uint16_t *freqs, uint8_t nvoices,
                                   uint16_t *duty, uint16_t len_cap,
                                   uint32_t fs, uint16_t duty_steps,
                                   uint16_t *len_out);

/**
 * @brief Start circular DMA streaming of duty[len] to the PWM compare register.
 * @note  The channel's DMA (htim->hdma[TIM_DMA_ID_CCx]) MUST be configured
 *        CIRCULAR in CubeMX for continuous looping.
 */
buzzer_error_t buzzer_poly_start(buzzer_t *dev, const uint16_t *duty, uint16_t len);

/** @brief Point the DMA at a freshly rendered buffer (double-buffered note change). */
buzzer_error_t buzzer_poly_swap(buzzer_t *dev, const uint16_t *duty, uint16_t len);
buzzer_error_t buzzer_poly_stop(buzzer_t *dev);
buzzer_error_t buzzer_tone_enter(buzzer_t *dev);
#endif /* BUZZER_POLY_ENABLE */
