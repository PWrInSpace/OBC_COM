#pragma once
#include "GNSS.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os2.h"

void gps_task(void *argument);
void start_gps_task();