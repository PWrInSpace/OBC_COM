/*
 * Author: Szymon Rzewuski & Mateusz Kłosiński
 * Updated: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 24.09.2026
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lora_config.h"

void RFM95W_task_init(void);
void rfm95_send_window(rfm95_t *radio, const uint8_t *payload, uint8_t payload_len, uint32_t window_ms);

/**
 * @brief Queue a fully-formed frame for the GS to transmit in its next slot.
 * @return false if the queue is full.
 */
bool lora_gs_tx_enqueue(const uint8_t *buf, uint16_t len);

void lora_gs_mark_settings_dirty(void);