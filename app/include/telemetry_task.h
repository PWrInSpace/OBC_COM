/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 26.03.2026
 */
#ifndef TELEMETRY_TASK_H
#define TELEMETRY_TASK_H

#include "cmsis_os2.h"

/**
 * @brief Inicjalizuje i uruchamia wątek telemetrii.
 */
void start_telemetry_task(void);

/**
 * @brief Główna pętla wątku telemetrii.
 */
void telemetry_task_thread(void *arg);

#endif /* TELEMETRY_TASK_H */