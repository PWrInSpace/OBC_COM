/*
 * Logger Serial Output Implementation - USB CDC
 * Wysyła logi na USB Virtual COM Port
 * 
 * WAŻNE: Wymaga skonfigurowania USB Device w STM32CubeMX:
 * 1. Dodaj USB Device Stack
 * 2. Włącz CDC (Communications Device Class)
 * 3. Wygeneruj kod
 * 
 * Użycie w main.c:
 *   logger_serial_init();
 *   logger_set_output_callback(logger_serial_output_callback);
 *   LOG_INFO("Hello USB!");
 */

#include "logger_serial_output.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usb_config.h"

void logger_serial_output_callback(const char* data, uint16_t len) {
    /* Use application-level helper to set TX buffer and request transmit.
       CDC_Transmit_FS handles setting the buffer and calling the class transmit
       function; it returns USBD_OK or USBD_BUSY/FAIL. Don't block here.
    */
    (void)CDC_Transmit_FS((uint8_t*)data, len);
}
