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
/**
 * @brief Zadanie przetwarzające komendy z UART przy użyciu puli buforów.
 */
void cmd_task(void *argument) {
    (void)argument;
    
    UART_Buffer_t *received_ptr = NULL;
    char log_msg[128];

    for(;;) {
        // Czekamy na wskaźnik do wypełnionego bufora z kolejki
        if (cmd_queue != NULL && xQueueReceive(cmd_queue, &received_ptr, portMAX_DELAY) == pdPASS) {
            
            if (received_ptr != NULL) {
                
                uint16_t actual_len = 0;
                uint8_t *data = received_ptr->data;
                
                if (data[0] == 0x32) {
                    // Format BINARNY LCDC: Header(0x32) + Len(1) + ID(1) + Data(N) + CRC(1) + EOF(1)
                    // actual_len = N + 5 (gdzie N to data[1])
                    actual_len = data[1] + 5;
                    if (actual_len > BUFFER_SIZE) actual_len = BUFFER_SIZE;

                    int log_len = snprintf(log_msg, sizeof(log_msg), 
                                           "LCDC Bin Recv: ID 0x%02X, Len %d\r\n", 
                                           data[2], actual_len);
                    USB_Transmit((uint8_t*)log_msg, log_len);
                } 
                else {
                    actual_len = received_ptr->len;
                    int log_len = snprintf(log_msg, sizeof(log_msg), "Text Recv: %.*s\r\n", 
                                           actual_len, (char*)data);
                    USB_Transmit((uint8_t*)log_msg, log_len);
                }
                process_command(data, actual_len);
                memset(data, 0, BUFFER_SIZE);
                xQueueSend(free_pool_queue, &received_ptr, 0);
            }
        }
    }
}

