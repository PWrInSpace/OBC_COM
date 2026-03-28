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

    // 2. ODCZYT (teraz powinien już odczytać to, co zapisaliśmy wyżej)
    if (NVS_Read(PARAM_FREQ, &tmp) == EE_OK) {
        // Formatuje wynik do wyświetlenia, żebyś widział co jest w pamięci
        int len = snprintf(msg, sizeof(msg), "NVS READ: %lu Hz\r\n", tmp);
        USB_Transmit((uint8_t*)msg, (uint16_t)len);

        // Przypisanie do radia
        dev->param->frequency = (uint32_t)tmp;
}

    // --- TX POWER (PARAM_PWR) ---
    if (NVS_Read((PARAM_PWR), &tmp) == EE_OK) {
        dev->param->power = (uint8_t)tmp;
    }

    if (NVS_Read((PARAM_SF), &tmp) == EE_OK) {
        dev->param->LoRa_Rate = (uint8_t)tmp;
    }

     if (NVS_Read((PARAM_BW), &tmp) == EE_OK) {
        dev->param->LoRa_BW = (uint8_t)tmp;
    }
}


void nvs_save_rfm95_settings(rfm95_t * dev)
{
    if (dev == NULL || dev->param == NULL) return;
    NVS_Write((PARAM_FREQ), (uint32_t)dev->param->frequency);
    NVS_Write((PARAM_PWR),  (uint32_t)dev->param->power);
    NVS_Write((PARAM_SF),   (uint32_t)dev->param->LoRa_Rate);
    NVS_Write((PARAM_BW),   (uint32_t)dev->param->LoRa_BW);
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
        *muted = false; // Domyślnie odblokowane
    }
}


