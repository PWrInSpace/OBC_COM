/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 3.02.2026 
 */

#include "usb_config.h"
#include "main.h"
#include <stdint.h>
#include "usbd_cdc_if.h"
#include "usbd_cdc.h"
#include "usbd_core.h"
#include "usb.h"
#include "app_freertos.h"

USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef FS_Desc;

extern osMutexId_t usbMutexHandle;

void USB_CDC_Config(void) 
{
    hpcd_USB_DRD_FS.pData = &hUsbDeviceFS;
    if(USBD_Init(&hUsbDeviceFS, &FS_Desc, 0) != USBD_OK)
        Error_Handler();

    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK)
        Error_Handler();

    if(USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_CDC_Template_fops) != USBD_OK)
        Error_Handler();

    if(USBD_Start(&hUsbDeviceFS) != USBD_OK)
        Error_Handler();
}

void USB_Transmit(uint8_t* Buf, uint16_t Len) {

    if (osMutexAcquire(usbMutexHandle, 100) == osOK) {
        
        uint8_t result = CDC_Transmit_FS(Buf, Len);
        
        if (result == 0) { // USBD_OK
            osDelay(10); 
        }

        osMutexRelease(usbMutexHandle);
    }
}