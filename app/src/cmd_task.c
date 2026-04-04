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
#include "stream_buffer.h"
QueueHandle_t cmd_queue = NULL; 
osThreadId_t cmdTaskHandle = NULL;

void CMD_Task_Init(void) {
    if (cmd_queue == NULL) {
        cmd_queue = xQueueCreate(POOL_SIZE, sizeof(CMD_Buffer_t*));
    }

    const osThreadAttr_t cmdTask_attributes = {
        .name = "usbCmdTask",
        .stack_size = 4096,
        .priority = (osPriority_t) osPriorityNormal,
    };

    cmdTaskHandle = osThreadNew(cmd_task, NULL, &cmdTask_attributes);
}
extern osThreadId_t rfm95wTaskHandle;
extern volatile uint16_t USB_Rx_Data_Len;
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
void cmd_task(void *argument) {
    (void)argument;
    
    CMD_Buffer_t *received_ptr = NULL;
    uint8_t usb_byte;
    static uint8_t usb_frame_buf[BUFFER_SIZE];
    static uint16_t usb_idx = 0;
    uint32_t ulNotifiedValue;

    for(;;) {

        xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotifiedValue, portMAX_DELAY);

         while (xStreamBufferReceive(xUsbStreamBuffer, &usb_byte, 1, 0) > 0) {
            // Ignoruj znaki sterujące na samym początku nowej ramki
            if (usb_idx == 0 && (usb_byte == '\n' || usb_byte == '\r' || usb_byte == ' ')) {
                continue; 
            }

            if (usb_idx < BUFFER_SIZE - 1) {
                usb_frame_buf[usb_idx++] = usb_byte;
            }

            if (usb_byte == '\n' || usb_byte == '\r') {
                // Usuń znak końca linii z bufora przed przetwarzaniem
                usb_idx--; 
                usb_frame_buf[usb_idx] = '\0';

                if (usb_idx >= 4 && memcmp(usb_frame_buf, "CMD:", 4) == 0) {
                    // Logika dla komend
                    process_command(usb_frame_buf, usb_idx);
                } 
                else if (usb_idx > 0) {
                    // Logika dla surowych danych LORA (tylko jeśli bufor nie jest pusty)
                    uint16_t lora_len = (usb_idx < LORA_BUFF_SIZE) ? usb_idx : LORA_BUFF_SIZE;
                    memcpy(LoraRxBuffer, usb_frame_buf, lora_len);
                    lora_cmd_len = lora_len;

                    if (rfm95wTaskHandle != NULL) {
                        xTaskNotify(rfm95wTaskHandle, LORA_TX_EVENT_BIT, eSetBits);
                    }
                }

                // Pełny reset przed następną paczką
                usb_idx = 0;
                memset(usb_frame_buf, 0, BUFFER_SIZE);
            }
        }

        while (cmd_queue != NULL && xQueueReceive(cmd_queue, &received_ptr, 0) == pdPASS) {
            if (received_ptr != NULL) {
                uint16_t actual_len = 0;
                uint8_t *data = received_ptr->data;

                if (data[0] == 0x32) {
                    actual_len = data[1] + 5;
                    if (actual_len > BUFFER_SIZE) actual_len = BUFFER_SIZE;
                } else {
                    actual_len = received_ptr->len;
                }

                process_command(data, actual_len);
                memset(data, 0, BUFFER_SIZE);
                xQueueSend(free_pool_queue, &received_ptr, 0);
            }
        }
    }
}

