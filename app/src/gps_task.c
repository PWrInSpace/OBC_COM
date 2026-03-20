#include "gps_task.h"
#include "usart.h"
#include "GNSS.h"

GNSS_StateHandle gpsHandle;

osThreadId_t gpsTaskHandle;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(gpsTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

osThreadAttr_t gpsTask_attributes = {
        .name = "gpsTask",
        .stack_size = 512 * 4,
        .priority = (osPriority_t) osPriorityNormal,
    };

void start_gps_task() {
    gpsTaskHandle = osThreadNew(gps_task, NULL, &gpsTask_attributes);
}

void gps_task(void *argument) {
    GNSS_Init(&gpsHandle, &huart1);
    GNSS_LoadConfig(&gpsHandle);
    HAL_UART_Receive_DMA(&huart1, gpsHandle.uartWorkingBuffer, 100);

    for(;;) {

        GNSS_ParseBuffer(&gpsHandle);
        
        if (gpsHandle.fixType == 3) {
            // Mamy FIX 3D - rakieta jest gotowa do startu
        }

        osDelay(500);
    }
}