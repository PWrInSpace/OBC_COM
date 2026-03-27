/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 26.03.2026
 */
#pragma once
#include "GNSS.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os2.h"

void gps_task(void *argument);
void start_gps_task();