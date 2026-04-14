#include "nvs_config.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "rfm95w.h"
#include "usb_config.h"

static SemaphoreHandle_t xNvsMutex = NULL;

/**
  * @brief  Unlock Flash and Init EEPROM Emulation
  */
EE_Status NVS_Init(void) {
    if (xNvsMutex == NULL) {
        xNvsMutex = xSemaphoreCreateMutex();
    }

    HAL_FLASH_Unlock();
    
    /* EE_CONDITIONAL_ERASE: Checks if flash is valid before erasing.
     * Prevents data loss on soft resets. */
    return EE_Init(EE_CONDITIONAL_ERASE);
}

static uint32_t current_log_mute_mask = 0;

void nvs_sync_log_mask(void) {
    uint32_t val = 0;
    if (NVS_Read(PARAM_LOG_TAG_MUTE, &val) == EE_OK) {
        current_log_mute_mask = val;
    }
}


EE_Status NVS_Write(uint16_t virt_addr, uint32_t data) {
    EE_Status status = 1;

    if (xSemaphoreTake(xNvsMutex, portMAX_DELAY) == pdTRUE) {
        status = EE_WriteVariable32bits(virt_addr, data);
        
        /* If flash pages are full, the library requires a 'Transfer' (Cleanup) */
        if ((status & EE_STATUSMASK_CLEANUP) == EE_STATUSMASK_CLEANUP) {
            status = EE_CleanUp();
        }
        xSemaphoreGive(xNvsMutex);
    }
    HAL_Delay(10); // wait for save the data
    return status;
}


EE_Status NVS_Read(uint16_t virt_addr, uint32_t* data) {
    EE_Status status = 1;

    if (xSemaphoreTake(xNvsMutex, portMAX_DELAY) == pdTRUE) {
        status = EE_ReadVariable32bits(virt_addr, data);
        xSemaphoreGive(xNvsMutex);
    }
    return status;
}

void nvs_get_rfm95_settings(rfm95_t * dev)
{
    if (dev == NULL || dev->param == NULL) return;

    uint32_t tmp = 0;
    char msg[64];

    if (NVS_Read(RFM95W_PARAM_FREQ, &tmp) == EE_OK) {
        int len = snprintf(msg, sizeof(msg), "NVS READ: %lu Hz\r\n", tmp);
        USB_Transmit((uint8_t*)msg, (uint16_t)len);
        dev->param->frequency = (uint32_t)tmp;
}

    if (NVS_Read((RFM95W_PARAM_PWR), &tmp) == EE_OK) {
        int len = snprintf(msg, sizeof(msg), "NVS READ: Power %d dBm\r\n", (int8_t)tmp);
        USB_Transmit((uint8_t*)msg, (uint16_t)len);
        dev->param->power = (uint8_t)tmp;
    }

    if (NVS_Read((RFM95W_PARAM_SF), &tmp) == EE_OK) {
            int len = snprintf(msg, sizeof(msg), "NVS READ: SF %d\r\n", (int8_t)tmp);
            USB_Transmit((uint8_t*)msg, (uint16_t)len);
        dev->param->LoRa_Rate = (uint8_t)tmp;
    }

     if (NVS_Read((RFM95W_PARAM_BW), &tmp) == EE_OK) {
            int len = snprintf(msg, sizeof(msg), "NVS READ: BW %d\r\n", (int8_t)tmp);
            USB_Transmit((uint8_t*)msg, (uint16_t)len); 
        dev->param->LoRa_BW = (uint8_t)tmp;
    }

    if (NVS_Read((RFM95W_PARAM_CR), &tmp) == EE_OK) {
            int len = snprintf(msg, sizeof(msg), "NVS READ: CR %d\r\n", (int8_t)tmp);
            USB_Transmit((uint8_t*)msg, (uint16_t)len); 
        dev->param->CR= (int16_t)tmp;
    }

    if (NVS_Read((RFM95W_PARAM_CRC), &tmp) == EE_OK && ( tmp == 0 || 1)) {
            int len = snprintf(msg, sizeof(msg), "NVS READ: CRC %s\r\n", (tmp) ? "ON" : "OFF");
            USB_Transmit((uint8_t*)msg, (uint16_t)len); 
        dev->param->crc= (bool)tmp;
    }
    if (NVS_Read((RFM95W_PARAM_SYNC), &tmp) == EE_OK) {
            int len = snprintf(msg, sizeof(msg), "NVS READ: SYNC %d\r\n", (int16_t)tmp);
            USB_Transmit((uint8_t*)msg, (uint16_t)len);
        dev->param->sync= (int16_t)tmp;
    }
    if (NVS_Read(RFM95W_PARAM_STATE, &tmp) == EE_OK) {
            int len = snprintf(msg, sizeof(msg), "NVS READ: STATE %d\r\n", (uint8_t)tmp);
            USB_Transmit((uint8_t*)msg, (uint16_t)len);
        dev->param->state = (uint8_t)tmp;
    }
     return;

}


void nvs_save_rfm95_settings(rfm95_t * dev)
{
    if (dev == NULL || dev->param == NULL) return;
    NVS_Write((RFM95W_PARAM_FREQ), (uint32_t)dev->param->frequency);
    NVS_Write((RFM95W_PARAM_PWR),  (uint32_t)dev->param->power);
    NVS_Write((RFM95W_PARAM_SF),   (uint32_t)dev->param->LoRa_Rate);
    NVS_Write((RFM95W_PARAM_BW),   (uint32_t)dev->param->LoRa_BW);
    NVS_Write((RFM95W_PARAM_CR),   (uint32_t)dev->param->CR);
    NVS_Write((RFM95W_PARAM_CRC),   (uint32_t)dev->param->crc);
    NVS_Write((RFM95W_PARAM_SYNC),   (uint32_t)dev->param->sync);
    NVS_Write((RFM95W_PARAM_STATE),   (uint32_t)dev->param->state);
   return;
}

void nvs_set_log_muted(uint32_t muted) {
    NVS_Write(PARAM_LOG_MUTED, (uint32_t)muted);
}

void nvs_get_log_muted(bool *muted) {
    uint32_t val = 0;
    if (NVS_Read(PARAM_LOG_MUTED, &val) == EE_OK) {
        *muted = (val != 0);
    } else {
        *muted = false;
    }
}


