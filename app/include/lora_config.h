/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#pragma once

#include "sx1280.h"
#include "rfm95w.h"
#include "spi.h"

#define SX1280_RX_TIMEOUT_MS 500
#define RFM95W_RX_TIMEOUT_MS 1000
#define SX1280_TX_TIMEOUT_MS 250
#define RFM95W_TX_TIMEOUT_MS 500

#define RFM95W_MAX_PACKET	256

typedef struct lora_devs_t {
  SX1280_t *sx1280;
  rfm95_t *rfm95w;
} LoRaDevs_t;



LoRaDevs_t* get_lora_devs_instance(void);

void lora_config_init(void);
void rfm95w_config_init(void);
void rfm95w_config_init_param(void);
void rfm95_print_actual_settings(rfm95_t *rfm);
uint8_t rfm95w_read_status(rfm95_t *rfm);
void sx1280_config_init(void);