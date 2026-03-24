#include "nvs_config.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "rfm95w.h"

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
    uint8_t status = 1;
    uint32_t tmp = 0;
    status|= NVS_Read(PARAM_FREQ,&tmp); //test for frequency
    dev->param->frequency = tmp;
    if (status!=EE_OK)
    {
        /* code */return;
    }
    return;
}

void nvs_set_rfm95_settings(rfm95_t * dev, uint32_t data)
{
    uint8_t status = 1;
    status|= NVS_Write(PARAM_FREQ, data); //test for saving the frequency parameter in nvs
    if (status!=EE_OK)
    {
        /* code */return;
    }
    return;
}