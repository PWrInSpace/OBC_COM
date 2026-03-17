#include "cmd_task.h"
#include "cmd_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "usb_config.h"
QueueHandle_t cmd_queue = NULL; 

void CMD_Task_Init(void) {

    if (cmd_queue == NULL) {
        cmd_queue = xQueueCreate(10, MAX_CMD_LEN);
    }

    const osThreadAttr_t cmdTask_attributes = {
        .name = "usbCmdTask",
        .stack_size = 1024,
        .priority = (osPriority_t) osPriorityNormal,
    };
    osThreadNew(usb_cmd_task, NULL, &cmdTask_attributes);
}

//! DODAĆ BUFFER POOL WYGODA ZE ZMIENNA DLUGOŚCIĄ DANYCH I BARDZIEJ EFEKTYWNE
void usb_cmd_task(void *argument) {
    (void)argument;
    char rx_buf[128];
    char log_msg[128];

    for(;;) {

        if(cmd_queue != NULL && xQueueReceive(cmd_queue, rx_buf, portMAX_DELAY) == pdPASS) {
            
            snprintf(log_msg, sizeof(log_msg), "Recv: %s\r\n", rx_buf);
            USB_Transmit((uint8_t*)log_msg, strlen(log_msg));

            process_command(rx_buf);
            USB_Transmit((uint8_t*)log_msg, strlen(log_msg));
        }
    }
}