#pragma once

#include "sx1280.h"

void flash_write(uint32_t address, uint8_t *data, uint16_t size);
void flash_read(uint32_t address, uint8_t *buffer, uint16_t size);

