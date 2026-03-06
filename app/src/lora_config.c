/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 27.01.2026
 */
#include "cmsis_os2.h"

#include "lora_config.h"
#include "rfm95w.h"
#include "sx1280.h"
#include "sx1280_hal_wrapper.h"
#include "rfm95w_wrapper.h"
#include "spi.h"
#include "spi_callbacks.h"

SX1280_t sx1280_radio = {
    .ctx = &hspi1,
    .spi_read = sx1280_spi_read,
    .spi_write = sx1280_spi_write,
    .spi_read_dma = sx1280_spi_read_dma,
    .spi_write_dma = sx1280_spi_write_dma,
    .set_reset = sx1280_set_reset,
    .get_busy = sx1280_get_busy,
    .get_dio = { sx1280_get_dio1, sx1280_get_dio2, NULL,},
    .delay_ms = sx1280_delay_ms
};

rfm95_t rfm95w_radio = {
    ._spi_transmit = rfm95w_spi_transmit,
    ._gpio_set_level = rfm95w_gpio_set_level,
    ._delay = rfm95w_delay,
    .rst_gpio_num = RFM95W_RST_Pin,
    .cs_gpio_num = RFM95W_CS_Pin,
    .d0_gpio_num = RFM95W_DIO_Pin,
    .frequency = 868000,
    .implicit_header = 0,
    .log = rfm95w_log
};

static LoRaDevs_t lora_devs_instance = {
  .sx1280 = &sx1280_radio,
  .rfm95w = &rfm95w_radio
};


LoRaDevs_t* get_lora_devs_instance(void) {
  return &lora_devs_instance;
}

static void rfm95w_config_init(void) {
  rfm95_reset(&rfm95w_radio);
  rfm95_default_config(&rfm95w_radio);
}

static void sx1280_config_init(void) {
  SX1280Reset(&sx1280_radio);
  osDelay(10);
  SX1280Init(&sx1280_radio, NULL);
  osDelay(10);
  SX1280SetRegulatorMode(&sx1280_radio, USE_LDO);
  osDelay(10);
  SX1280SetDioIrqParams(&sx1280_radio, IRQ_RADIO_ALL, IRQ_RADIO_ALL, IRQ_RADIO_NONE, IRQ_RADIO_NONE);
  osDelay(10);
  SX1280SetTxParams(&sx1280_radio, 14, RADIO_RAMP_20_US);
  osDelay(10);
  SX1280SetPacketType(&sx1280_radio, PACKET_TYPE_LORA);
  osDelay(10);
  SX1280SetModulationParams(&sx1280_radio, &(ModulationParams_t){
    .PacketType = PACKET_TYPE_LORA,
    .Params.LoRa = {
      .SpreadingFactor = LORA_SF7,
      .Bandwidth = LORA_BW_0800,
      .CodingRate = LORA_CR_4_5
    }
  });
  osDelay(10);
  SX1280SetPacketParams(&sx1280_radio, &(PacketParams_t){
    .PacketType = PACKET_TYPE_LORA,
    .Params.LoRa = {
      .PreambleLength = 3,
      .HeaderType = LORA_PACKET_VARIABLE_LENGTH,
      .PayloadLength = 255,
      .CrcMode = LORA_CRC_ON,
    }
  });
  osDelay(10);
}

void lora_config_init(void) {
    sx1280_config_init();
    rfm95w_config_init();
    sx1280_wrapper_init();
    rfm95w_wrapper_init();
}