/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 26.03.2026
 */
#ifndef CMD_TASK_H
#define CMD_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os2.h"

extern QueueHandle_t cmd_queue;

void CMD_Task_Init(void);
void cmd_task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* CMD_TASK_H */