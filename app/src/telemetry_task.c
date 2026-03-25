#include "telemetry_task.h"
#include "board_data.h"
#include "sd_task.h"
#include <string.h>

osThreadId_t telemetryTaskHandle;

const osThreadAttr_t telemetryTask_attributes = {
    .name = "telemetryTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

void start_telemetry_task(void) {
    telemetryTaskHandle = osThreadNew(telemetry_task_thread, NULL, &telemetryTask_attributes);
}

void telemetry_task_thread(void *arg) {
    (void)arg;
    BoardData_t snapshot;
    vTaskDelay(pdMS_TO_TICKS(100));

    while(1) {
        if (g_state_mutex != NULL && xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            memcpy(&snapshot, &g_system_state, sizeof(BoardData_t));
            xSemaphoreGive(g_state_mutex);
            snapshot.timestamp_ms = osKernelGetTickCount();
            if (sd_logger_log_data(&snapshot) != HAL_OK) {
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}