/*
 * Author: Szymon Rzewuski, Mateusz Kłosiński
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

//CZACIOR NASZPONCIL COS DO DEBUGOWANIA
void USB_Transmit_Hex(uint8_t* data, uint16_t len) {
    static char large_hex_buf[1024]; 
    char header_buf[48];
    static const char hex_chars[] = "0123456789ABCDEF";
    
    if (data == NULL || len == 0) return;
    int header_len = snprintf(header_buf, sizeof(header_buf), "Sent %u bytes as hex:\r\n", (unsigned int)len);
    //USB_Transmit((uint8_t*)header_buf, (uint16_t)header_len);
    if (len * 3 >= sizeof(large_hex_buf)) {
        len = (sizeof(large_hex_buf) - 1) / 3;
    }
    char *ptr = large_hex_buf;
    for (uint16_t i = 0; i < len; i++) {
        *ptr++ = hex_chars[(data[i] >> 4) & 0x0F];
        *ptr++ = hex_chars[data[i] & 0x0F];
        *ptr++ = ' ';
    }
    *ptr++ = '\r'; 
    *ptr++ = '\n';
    *ptr = '\0';

    // 4. Wysyłka całego bufora danych
    USB_Transmit((uint8_t*)large_hex_buf, (uint16_t)(ptr - large_hex_buf));
}