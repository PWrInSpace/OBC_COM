/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 26.03.2026
 */
#include "cmd_task.h"
#include "cmd_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "usb_config.h"
#include "usart.h"
QueueHandle_t cmd_queue = NULL; 

void CMD_Task_Init(void) {

    if (cmd_queue == NULL) {
        cmd_queue = xQueueCreate(10, MAX_CMD_LEN);
    }

    const osThreadAttr_t cmdTask_attributes = {
        .name = "usbCmdTask",
        .stack_size = 4096,
        .priority = (osPriority_t) osPriorityNormal,
    };
    osThreadNew(cmd_task, NULL, &cmdTask_attributes);
}

//! DODAĆ BUFFER POOL WYGODA ZE ZMIENNA DLUGOŚCIĄ DANYCH I BARDZIEJ EFEKTYWNE
void cmd_task(void *argument) {
    (void)argument;
   // Start_UART2_DMA_Receiver();
    uint8_t rx_buf[MAX_CMD_LEN]; 
    char log_msg[64];

    for(;;) {
        if(cmd_queue != NULL && xQueueReceive(cmd_queue, rx_buf, portMAX_DELAY) == pdPASS) {
            
            uint16_t actual_len = 0;
            if (rx_buf[0] == 0x32) {
                // Format BINARNY LCDC
                // rx_buf[1] to długość pola DATA
                // Całość to: Header(1) + Len(1) + ID(1) + Data(N) + CRC(1) + EOF(1) = N + 5
                actual_len = rx_buf[1] + 5;
                if (actual_len > MAX_CMD_LEN) actual_len = MAX_CMD_LEN;

                int log_len = snprintf(log_msg, sizeof(log_msg), "LCDC Bin Recv: ID 0x%02X, Len %d\r\n", rx_buf[2], actual_len);
                USB_Transmit((uint8_t*)log_msg, log_len);
            } 
            else {
                actual_len = (uint16_t)strlen((char*)rx_buf);
                int log_len = snprintf(log_msg, sizeof(log_msg), "Text Recv: %s\r\n", (char*)rx_buf);
                USB_Transmit((uint8_t*)log_msg, log_len);
            }
            process_command(rx_buf, actual_len);
            memset(rx_buf, 0, MAX_CMD_LEN);
        }
    }
}