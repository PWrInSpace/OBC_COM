/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 26.03.2026
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os2.h"
void usart_task(void *argument);
void USART_Task_Init(void); 
void 