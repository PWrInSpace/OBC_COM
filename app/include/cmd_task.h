#ifndef CMD_TASK_H
#define CMD_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os2.h"

#define MAX_CMD_LEN 128

extern QueueHandle_t cmd_queue;

void CMD_Task_Init(void);
void cmd_task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* CMD_TASK_H */