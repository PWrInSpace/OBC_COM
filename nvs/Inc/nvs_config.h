#ifndef NVS_CONFIG_H
#define NVS_CONFIG_H

#include <stdint.h>
#include "eeprom_emul.h"
#include "rfm95w.h"

/**
 * @brief Parameter Offsets
 * These define the "index" of each parameter. 
 * Scalability: Add new parameters to the END of this list.
 */
typedef enum {
    RFM95W_PARAM_MODE = 1,
    RFM95W_PARAM_FREQ,
    RFM95W_PARAM_BW,
    RFM95W_PARAM_SF,
    RFM95W_PARAM_CR,
    RFM95W_PARAM_PWR,
    RFM95W_PARAM_SYNC,
    RFM95W_PARAM_CRC,
    RFM95W_PARAM_STATE,
    PARAM_LOG_MUTED,
    PARAM_LOG_TAG_MUTE,
    PARAM_MAX_COUNT
} NVS_Param_t;

typedef enum {
    LOG_TAG_GPS     = (1 << 0),  // 0x00000001
    LOG_TAG_RFM95   = (1 << 1),  // 0x00000002
    LOG_TAG_USB     = (1 << 2),  // 0x00000004
    LOG_TAG_SYSTEM  = (1 << 3),  // 0x00000008
    LOG_TAG_PWR     = (1 << 4),  // 0x00000010
    LOG_TAG_ALL     = 0xFFFFFFFF
} LogTag_t;

/**
 * @brief Virtual Address Base Mapping
 * Prohibited IDs: 0x0000, 0xFFFF.
 * Scalability: Add new base addresses (e.g., 30, 40) for more devices.
 */
#define VIRT_BASE_RFM95W    10
#define VIRT_BASE_SX1280    20

#define GET_RFM95_ID(param)  ((uint16_t)(VIRT_BASE_RFM95W + (param)))
#define GET_SX1280_ID(param) ((uint16_t)(VIRT_BASE_SX1280 + (param)))

EE_Status NVS_Init(void);
EE_Status NVS_Write(uint16_t virt_addr, uint32_t data);
EE_Status NVS_Read(uint16_t virt_addr, uint32_t* data);

void nvs_get_rfm95_settings(rfm95_t * dev);
void nvs_save_rfm95_settings(rfm95_t * dev);

void nvs_set_log_muted(uint32_t muted);
void nvs_get_log_muted(bool *muted);


#endif /* NVS_CONFIG_H */