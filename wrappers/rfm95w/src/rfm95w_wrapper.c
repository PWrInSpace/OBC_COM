/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 29.01.2026
 */


#include "rfm95w_wrapper.h"
#include "spi.h"
#include "stm32h5xx_hal_spi.h"
#include <stdint.h>

#define MAX_SPI_BUFFER 260

static uint8_t spi_dma_tx_buffer[MAX_SPI_BUFFER];
SemaphoreHandle_t rfm95w_spi_sem = NULL;

void rfm95w_wrapper_init(void) {
    rfm95w_spi_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(rfm95w_spi_sem); 
}

bool rfm95w_spi_transmit(uint8_t *in, uint8_t *out) {
    if (in == NULL || out == NULL) return HAL_ERROR;

    HAL_GPIO_WritePin(RFM95W_CS_GPIO_Port, RFM95W_CS_Pin, GPIO_PIN_RESET);

    // Wywołanie HALa
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi2, out, in, 2, 1000);

    HAL_GPIO_WritePin(RFM95W_CS_GPIO_Port, RFM95W_CS_Pin, GPIO_PIN_SET);
    
    return status;
}

void rfm95w_delay(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

bool rfm95w_gpio_set_level(uint16_t _gpio_num, uint8_t _level) {
    GPIO_PinState pin_state = (_level == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;

    if (_gpio_num == RFM95W_RST_Pin) {
        HAL_GPIO_WritePin(RFM95W_RST_GPIO_Port, RFM95W_RST_Pin, pin_state);
        return true;
    }

    return false; 
}

void rfm95w_log(const char *info) {
    // Implement logging mechanism if needed
}


