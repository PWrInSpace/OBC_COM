/*
 * Author: Szymon Rzewuski & Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#pragma once

#include "lora_config.h"

void RFM95W_task_init(void);
void rfm95_send_window(rfm95_t *radio, const uint8_t *payload, uint8_t payload_len, uint32_t window_ms);