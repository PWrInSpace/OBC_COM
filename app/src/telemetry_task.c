#include "telemetry_task.h"
#include "board_data.h"
#include "sd_task.h"
#include "stm32h5xx_hal_gpio.h"
#include <string.h>

osThreadId_t telemetryTaskHandle;

const osThreadAttr_t telemetryTask_attributes = {
    .name = "telemetryTask",
    .stack_size = 1024,
    .priority = (osPriority_t) osPriorityAboveNormal,
};

void start_telemetry_task(void) {
     uint8_t msg2[] = "ZA MAŁO PAMIECI!\r\n";
    //HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_4);
    telemetryTaskHandle = osThreadNew(telemetry_task_thread, NULL, &telemetryTask_attributes);
    if (telemetryTaskHandle == NULL) {
     // HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
      //LOG_ERROR("RFM95W TASK ERROR!!");
      return;
    }
}

void telemetry_task_thread(void *arg) {
    (void)arg;
    BoardData_t snapshot = { 0 };
    osDelay(100);
HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
osDelay(200);
HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
osDelay(200);
HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
osDelay(200);
HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
osDelay(200);
HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
osDelay(200);
    for (;;) {
        if (g_state_mutex != NULL && xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
           
            memcpy(&snapshot, &g_system_state, sizeof(BoardData_t));
            xSemaphoreGive(g_state_mutex);
            snapshot.timestamp_ms = osKernelGetTickCount();
            if (sd_logger_log_data(&snapshot) != HAL_OK) {
            }
        }
         HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
        osDelay(1000);
    }
}