/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 26.03.2026
 */
#include "cmd_task.h"
#include "cmd_interface.h"
#include "rfm95w_task.h"
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
        cmd_queue = xQueueCreate(POOL_SIZE, sizeof(CMD_Buffer_t *));
    }

    const osThreadAttr_t cmdTask_attributes = {
        .name = "usbCmdTask",
        .stack_size = 4096,
        .priority = (osPriority_t)osPriorityNormal,
    };

    cmdTaskHandle = osThreadNew(cmd_task, NULL, &cmdTask_attributes);
}

void cmd_task(void *argument) {
    (void)argument;

    CMD_Buffer_t *received_ptr = NULL;
    uint8_t usb_byte;
    static uint8_t usb_frame_buf[BUFFER_SIZE];
    static uint16_t usb_idx = 0;
    uint32_t ulNotifiedValue;

    for (;;) {
        xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotifiedValue, portMAX_DELAY);

        for (;;) {
            if (xStreamBufferReceive(xUsbStreamBuffer, &usb_byte, 1, pdMS_TO_TICKS(4)) == 0) {
                if (usb_idx > 0) {
                    bool stray_nl = (usb_idx == 1 && (usb_frame_buf[0] == '\n' || usb_frame_buf[0] == '\r'));
                    if (usb_idx >= 4 && memcmp(usb_frame_buf, "CMD;", 4) == 0) {
                        usb_frame_buf[usb_idx] = '\0';
                        process_command(usb_frame_buf, usb_idx);
                    } else if (!stray_nl) {
                        lora_gs_tx_enqueue(usb_frame_buf, usb_idx);
                    }
                    usb_idx = 0;
                    memset(usb_frame_buf, 0, BUFFER_SIZE);
                }
                break;
            }

            if (usb_idx < BUFFER_SIZE - 1) {
                usb_frame_buf[usb_idx++] = usb_byte;
            }

            if ((usb_byte == '\n' || usb_byte == '\r') &&
                usb_idx >= 5 && memcmp(usb_frame_buf, "CMD;", 4) == 0) {
                usb_idx--;
                usb_frame_buf[usb_idx] = '\0';
                process_command(usb_frame_buf, usb_idx);
                usb_idx = 0;
                memset(usb_frame_buf, 0, BUFFER_SIZE);
            }
        }

        while (cmd_queue != NULL && xQueueReceive(cmd_queue, &received_ptr, 0) == pdPASS) {
            if (received_ptr != NULL) {
                uint8_t *data = received_ptr->data;
                uint16_t actual_len = (data[0] == 0x32) ? (uint16_t)(data[2] + 5) : received_ptr->len;
                if (actual_len > BUFFER_SIZE) actual_len = BUFFER_SIZE;

                process_command(data, actual_len);
                memset(data, 0, BUFFER_SIZE);
                xQueueSend(free_pool_queue, &received_ptr, 0);
            }
        }
    }
}
