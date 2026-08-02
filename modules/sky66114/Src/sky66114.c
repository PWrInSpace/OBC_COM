#include "sky66114.h"
#include "logger.h"

sky66114_t sky66114[SKY66114_MODULES_COUNT];

sky66114_error_t sky66114_init(sky66114_t *dev, const sky66114_t *config) {
    if (dev == NULL || config == NULL) return SKY66114_ERROR;
    *dev = *config;

    if (dev->enable_port) {
        HAL_GPIO_WritePin(dev->enable_port, dev->enable_pin, GPIO_PIN_SET);
    }

    LOG_DEBUG("SKY66114 initialized with shared control lines and unique enable pin");
    return sky66114_set_mode(dev, SKY66114_MODE_SLEEP_0);
}

sky66114_error_t sky66114_init_all(void) {
    const sky66114_t shared_config = {
        .csd_port = SHARED_CSD_GPIO_Port, .csd_pin = SHARED_CSD_Pin,
        .cps_port = SHARED_CPS_GPIO_Port, .cps_pin = SHARED_CPS_Pin,
        .crx_port = SHARED_CRX_GPIO_Port, .crx_pin = SHARED_CRX_Pin,
        .ctx_port = SHARED_CTX_GPIO_Port, .ctx_pin = SHARED_CTX_Pin,
        .chl_port = SHARED_CHL_GPIO_Port, .chl_pin = SHARED_CHL_Pin
    };

    GPIO_TypeDef *en_ports[SKY66114_MODULES_COUNT] = {
        ANT1_EN_GPIO_Port, ANT2_EN_GPIO_Port, ANT3_EN_GPIO_Port, ANT4_EN_GPIO_Port
    };
    uint16_t en_pins[SKY66114_MODULES_COUNT] = {
        ANT1_EN_Pin, ANT2_EN_Pin, ANT3_EN_Pin, ANT4_EN_Pin
    };

    for (size_t i = 0; i < SKY66114_MODULES_COUNT; ++i) {
        sky66114_t cfg = shared_config;
        cfg.enable_port = en_ports[i];
        cfg.enable_pin  = en_pins[i];

        if (sky66114_init(&sky66114[i], &cfg) != SKY66114_OK) {
            LOG_ERROR("Failed to initialize SKY66114 instance %d", i);
            return SKY66114_ERROR;
        }
    }

    return SKY66114_OK;
}

sky66114_error_t sky66114_set_mode(sky66114_t *dev, sky66114_mode_t mode) {
    if (dev == NULL) return SKY66114_ERROR;

    #define SET_PIN(p, val) if(dev->p##_port) HAL_GPIO_WritePin(dev->p##_port, dev->p##_pin, (val) ? GPIO_PIN_SET : GPIO_PIN_RESET)

    switch (mode) {
        case SKY66114_MODE_SLEEP_0:
            SET_PIN(csd, 0); SET_PIN(cps, 0); SET_PIN(crx, 0); SET_PIN(ctx, 0); SET_PIN(chl, 0);
            break;
        case SKY66114_MODE_RX_LNA:
            SET_PIN(csd, 1); SET_PIN(cps, 0); SET_PIN(crx, 1); SET_PIN(ctx, 0); SET_PIN(chl, 0);
            break;
        case SKY66114_MODE_TX_HIGH:
            SET_PIN(csd, 1); SET_PIN(cps, 0); SET_PIN(crx, 0); SET_PIN(ctx, 1); SET_PIN(chl, 1);
            break;
        case SKY66114_MODE_TX_LOW:
            SET_PIN(csd, 1); SET_PIN(cps, 0); SET_PIN(crx, 0); SET_PIN(ctx, 1); SET_PIN(chl, 0);
            break;
        case SKY66114_MODE_RX_BYP:
            SET_PIN(csd, 1); SET_PIN(cps, 1); SET_PIN(crx, 1); SET_PIN(ctx, 0); SET_PIN(chl, 0);
            break;
        case SKY66114_MODE_TX_BYP:
            SET_PIN(csd, 1); SET_PIN(cps, 1); SET_PIN(crx, 0); SET_PIN(ctx, 1); SET_PIN(chl, 0);
            break;
        case SKY66114_MODE_SLEEP_6:
            SET_PIN(csd, 1); SET_PIN(cps, 0); SET_PIN(crx, 0); SET_PIN(ctx, 0); SET_PIN(chl, 0);
            break;
        default:
            return SKY66114_ERROR;
    }

    #undef SET_PIN
    return SKY66114_OK;
}

sky66114_t set_mode_all(sky66114_mode_t mode) {
    for (size_t i = 0; i < SKY66114_MODULES_COUNT; ++i) {
        if (sky66114_set_mode(&sky66114[i], mode) != SKY66114_OK) {
            LOG_ERROR("Failed to set mode for SKY66114 instance %d", i);
        }
    }
    return sky66114[0];
}