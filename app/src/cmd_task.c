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

void send_to_lora(uint8_t* buf, uint16_t len) {
    uint16_t actual_len = (len < LORA_BUFF_SIZE) ? len : LORA_BUFF_SIZE;
    memcpy(LoraRxBuffer, buf, actual_len);
    lora_cmd_len = actual_len;
    if (rfm95wTaskHandle != NULL) {
        xTaskNotify(rfm95wTaskHandle, LORA_TX_EVENT_BIT, eSetBits);
    }
}



void cmd_task(void *argument) {
    (void)argument;
    uint8_t usb_byte;
    static uint8_t usb_frame_buf[BUFFER_SIZE];
    static uint16_t usb_idx = 0;
    CMD_Buffer_t *received_ptr = NULL;
    // Packet State
    typedef enum { MODE_IDLE, MODE_TEXT, MODE_BINARY, MODE_LORA } rx_mode_t;
    static rx_mode_t mode = MODE_IDLE;
    static uint16_t expected_len = 0;

    for(;;) {
        uint32_t ulNotifiedValue;
        xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotifiedValue, portMAX_DELAY);

        while (xStreamBufferReceive(xUsbStreamBuffer, &usb_byte, 1, 0) > 0) {
            
            if (usb_idx == 0) {
                if (usb_byte == 0x32) {
                    HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
                    mode = MODE_BINARY;
                    expected_len = 0;
                } else if (usb_byte == 'C') {
                    mode = MODE_TEXT;
                } else {
                    mode = MODE_LORA;
                }
            }

            if (usb_idx < BUFFER_SIZE - 1) {
                usb_frame_buf[usb_idx++] = usb_byte;
            }

            if (mode == MODE_BINARY) {
                if (usb_idx == 3) {
                    expected_len = usb_frame_buf[2] + 5; 
                }
                
                if (expected_len > 0 && usb_idx >= expected_len) {
                    process_command(usb_frame_buf, usb_idx);
                    goto reset_state;
                }
            } 
            else if (mode == MODE_TEXT || mode == MODE_LORA) {
                if (usb_byte == '\n' || usb_byte == '\r') {
                    if (mode == MODE_TEXT && usb_idx >= 4 && memcmp(usb_frame_buf, "CMD:", 4) == 0) {
                        process_command(usb_frame_buf, usb_idx);
                    } else {
                        send_to_lora(usb_frame_buf, usb_idx);
                    }
                    goto reset_state;
                }
            }

            if (usb_idx >= BUFFER_SIZE - 1) {
                goto reset_state;
            }
            continue;

        reset_state:
            usb_idx = 0;
            mode = MODE_IDLE;
            expected_len = 0;
            HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
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
                USB_Transmit((uint8_t*)"RX CMD: ", 11); 
                USB_Transmit(data, actual_len);
                USB_Transmit((uint8_t*)"\r\n", 2);  
                process_command(data, actual_len);
                memset(data, 0, BUFFER_SIZE);
                xQueueSend(free_pool_queue, &received_ptr, 0);
            }
        }
    }
}

// Helper function to keep the task clean


