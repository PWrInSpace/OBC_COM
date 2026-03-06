/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#pragma once

void sx1280_wrapper_init(void);
int32_t sx1280_spi_write_dma(void* ctx, uint8_t *prefix, uint16_t prefix_len, uint8_t* out, uint16_t out_len);
int32_t sx1280_spi_read_dma(void* ctx, uint8_t *prefix, uint16_t prefix_len, uint8_t* in, uint16_t in_len);
int32_t sx1280_spi_write(void* ctx, uint8_t *prefix, uint16_t prefix_len, uint8_t* out, uint16_t out_len);
int32_t sx1280_spi_read(void* ctx, uint8_t *prefix, uint16_t prefix_len, uint8_t* in, uint16_t in_len);
void sx1280_delay_ms(void* ctx, uint32_t ms);
int32_t sx1280_set_reset(void* ctx, bool value);
int32_t sx1280_get_busy(void* ctx);
int32_t sx1280_get_dio1(void* ctx);
int32_t sx1280_get_dio2(void* ctx);