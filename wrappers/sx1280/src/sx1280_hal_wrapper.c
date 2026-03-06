/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "sx1280.h"
#include "main.h"
#include "spi.h"
#include <string.h>

#define MAX_SPI_BUFFER 260 

static uint8_t spi_dma_tx_buffer[MAX_SPI_BUFFER];

SemaphoreHandle_t sx1280_spi_sem = NULL;

void sx1280_wrapper_init(void) {
    sx1280_spi_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(sx1280_spi_sem); 
}

// --- SPI WRAPPERS ---

int32_t sx1280_spi_write_dma(void* ctx, uint8_t *prefix, uint16_t prefix_len, uint8_t* out, uint16_t out_len) {
    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)ctx;
    uint16_t total_len = prefix_len + out_len;

    if (xSemaphoreTake(sx1280_spi_sem, pdMS_TO_TICKS(100)) != pdTRUE) return -1;
    if (total_len > MAX_SPI_BUFFER) { xSemaphoreGive(sx1280_spi_sem); return -2; }

    // while(HAL_GPIO_ReadPin(BUSY_SX1280_GPIO_Port, BUSY_SX1280_Pin) == GPIO_PIN_SET);

    memcpy(spi_dma_tx_buffer, prefix, prefix_len);
    if (out != NULL && out_len > 0) {
        memcpy(spi_dma_tx_buffer + prefix_len, out, out_len);
    }

    HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_RESET);

    if (HAL_SPI_Transmit_DMA(hspi, spi_dma_tx_buffer, total_len) != HAL_OK) {
        HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_SET);
        xSemaphoreGive(sx1280_spi_sem);
        return -3;
    }
    return 0; 
}

int32_t sx1280_spi_read_dma(void* ctx, uint8_t *prefix, uint16_t prefix_len, uint8_t* in, uint16_t in_len) {
    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)ctx;

    if (xSemaphoreTake(sx1280_spi_sem, pdMS_TO_TICKS(100)) != pdTRUE) return -1;

    // while(HAL_GPIO_ReadPin(BUSY_SX1280_GPIO_Port, BUSY_SX1280_Pin) == GPIO_PIN_SET);

    HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi, prefix, prefix_len, HAL_MAX_DELAY);

    if (in != NULL && in_len > 0) {
        if (HAL_SPI_Receive_DMA(hspi, in, in_len) != HAL_OK) {
            HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_SET);
            xSemaphoreGive(sx1280_spi_sem);
            return -3;
        }
    } else {
        HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_SET);
        xSemaphoreGive(sx1280_spi_sem);
    }
    return 0;
}

int32_t sx1280_spi_write(void* ctx, uint8_t *prefix, uint16_t prefix_len, uint8_t* out, uint16_t out_len) {
    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)ctx;
    HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi, prefix, prefix_len, HAL_MAX_DELAY);
    if (out != NULL && out_len > 0) HAL_SPI_Transmit(hspi, out, out_len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_SET);
    return 0;
}

int32_t sx1280_spi_read(void* ctx, uint8_t *prefix, uint16_t prefix_len, uint8_t* in, uint16_t in_len) {
    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)ctx;
    HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi, prefix, prefix_len, HAL_MAX_DELAY);
    if (in != NULL && in_len > 0) HAL_SPI_Receive(hspi, in, in_len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SX1280_CS_GPIO_Port, SX1280_CS_Pin, GPIO_PIN_SET);
    return 0;
}

void sx1280_delay_ms(void* ctx, uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

int32_t sx1280_set_reset(void* ctx, bool value) {
    if (value) {
        HAL_GPIO_WritePin(SX1280_RESET_GPIO_Port, SX1280_RESET_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(SX1280_RESET_GPIO_Port, SX1280_RESET_Pin, GPIO_PIN_RESET);
    }
    return 0;
}

int32_t sx1280_get_busy(void* ctx) {
    GPIO_PinState state = HAL_GPIO_ReadPin(SX1280_BUSY_GPIO_Port, SX1280_BUSY_Pin);
    return (state == GPIO_PIN_SET) ? 1 : 0;
}

int32_t sx1280_get_dio1(void* ctx) {
    GPIO_PinState state = HAL_GPIO_ReadPin(SX1280_DIO1_GPIO_Port, SX1280_DIO1_Pin);
    return (state == GPIO_PIN_SET) ? 1 : 0;
}

int32_t sx1280_get_dio2(void* ctx) {
    GPIO_PinState state = HAL_GPIO_ReadPin(SX1280_DIO2_GPIO_Port, SX1280_DIO2_Pin);
    return (state == GPIO_PIN_SET) ? 1 : 0;
}