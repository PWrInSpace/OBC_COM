#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "sx1280.h"

void send_window(SX1280_t *radio, const uint8_t *payload, uint8_t payload_len, uint32_t window_ms);
void recv_once_ceiling(SX1280_t *radio, uint32_t ceiling_ms);