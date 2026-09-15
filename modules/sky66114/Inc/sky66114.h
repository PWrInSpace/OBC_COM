#pragma once

#include <stdint.h>
#include "main.h"

#define SKY66114_MODULES_COUNT 4

#ifndef SHARED_CSD_GPIO_Port
#define SHARED_CSD_GPIO_Port  GPIOA
#define SHARED_CSD_Pin        GPIO_PIN_0
#define SHARED_CPS_GPIO_Port  GPIOA
#define SHARED_CPS_Pin        GPIO_PIN_1
#define SHARED_CRX_GPIO_Port  GPIOA
#define SHARED_CRX_Pin        GPIO_PIN_2
#define SHARED_CTX_GPIO_Port  GPIOA
#define SHARED_CTX_Pin        GPIO_PIN_3
#define SHARED_CHL_GPIO_Port  GPIOA
#define SHARED_CHL_Pin        GPIO_PIN_4

#define ANT1_EN_GPIO_Port     GPIOB
#define ANT1_EN_Pin           GPIO_PIN_0
#define ANT2_EN_GPIO_Port     GPIOB
#define ANT2_EN_Pin           GPIO_PIN_1
#define ANT3_EN_GPIO_Port     GPIOB
#define ANT3_EN_Pin           GPIO_PIN_2
#define ANT4_EN_GPIO_Port     GPIOB
#define ANT4_EN_Pin           GPIO_PIN_3
#endif

typedef enum {
    SKY66114_MODE_SLEEP_0 = 0,
    SKY66114_MODE_RX_LNA  = 1,
    SKY66114_MODE_TX_HIGH = 2,
    SKY66114_MODE_TX_LOW  = 3,
    SKY66114_MODE_RX_BYP  = 4,
    SKY66114_MODE_TX_BYP  = 5,
    SKY66114_MODE_SLEEP_6 = 6
} sky66114_mode_t;

typedef enum {
    SKY66114_OK = 0,
    SKY66114_ERROR = 1,
} sky66114_error_t;

typedef struct {
    GPIO_TypeDef *csd_port; uint16_t csd_pin;
    GPIO_TypeDef *cps_port; uint16_t cps_pin;
    GPIO_TypeDef *crx_port; uint16_t crx_pin;
    GPIO_TypeDef *ctx_port; uint16_t ctx_pin;
    GPIO_TypeDef *chl_port; uint16_t chl_pin;
    
    GPIO_TypeDef *enable_port; uint16_t enable_pin;
} sky66114_t;

extern sky66114_t sky66114_default_config;
extern sky66114_t sky66114[SKY66114_MODULES_COUNT];

sky66114_error_t sky66114_init(sky66114_t *dev, const sky66114_t *config);
sky66114_error_t sky66114_set_mode(sky66114_t *dev, sky66114_mode_t mode);