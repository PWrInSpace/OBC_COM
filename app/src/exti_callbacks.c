/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 29.09.2026
 */

#include "main.h"
#include "sd_task.h"
#include "rfm95w_task.h"

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
    sd_task_exti_notify(GPIO_Pin);

    if (GPIO_Pin == RFM95W_DIO_Pin) rfm95_dio0_isr_notify();
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
    sd_task_exti_notify(GPIO_Pin);
}
