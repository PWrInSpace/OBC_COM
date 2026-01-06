/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"
#include "cmsis_os2.h"
#include "sx1280.h"
#include "main.h"
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lora_utilities.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for sx1280Task */
osThreadId_t sx1280TaskHandle;
const osThreadAttr_t sx1280Task_attributes = {
  .name = "sx1280Task",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 4096 * 4
};

/* Definitions for rfm95wTask */
osThreadId_t rfm95wTaskHandle;
const osThreadAttr_t rfm95wTask_attributes = {
  .name = "rfm95wTask",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = 4096 * 4
};
/* Definitions for GPSTask */
osThreadId_t GPSTaskHandle;
const osThreadAttr_t GPSTask_attributes = {
  .name = "GPSTask",
  .priority = (osPriority_t) osPriorityLow5,
  .stack_size = 2048 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of sx1280Task */
  sx1280TaskHandle = osThreadNew(sx1280TaskEntry, NULL, &sx1280Task_attributes);

  /* creation of rfm95wTask */
  rfm95wTaskHandle = osThreadNew(rfm95wTaskEntry, NULL, &rfm95wTask_attributes);

  /* creation of GPSTask */
  GPSTaskHandle = osThreadNew(GPSTaskEntry, NULL, &GPSTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
* @brief Function implementing the defaultTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END defaultTask */
}

/* USER CODE BEGIN Header_sx1280TaskEntry */
/**
* @brief Function implementing the sx1280Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_sx1280TaskEntry */
void sx1280TaskEntry(void *argument)
{
  /* USER CODE BEGIN sx1280Task */
  extern SX1280_t sx1280_radio;
  /* Mode selector: true = SEND, false = RECV */
  bool send_mode = false;

  /* Example payload to send */
  uint8_t tx_payload[] = { 0x01, 0x05, 'H', 'e', 'l', 'l', 'o' };

  for(;;)
  {
    if (send_mode) {
      /* SEND variant: prepare payload and transmit for fixed 500 ms */
      send_window(&sx1280_radio, tx_payload, (uint8_t)sizeof(tx_payload), 500);
    } else {
      /* RECV variant: receive once with 250 ms ceiling */
      recv_once_ceiling(&sx1280_radio, 250);
    }

    /* Toggle mode for next cycle */
    send_mode = !send_mode;

    /* Small gap before next cycle */
    osDelay(10);
  }
  /* USER CODE END sx1280Task */
}

/* USER CODE BEGIN Header_rfm95wTaskEntry */
/**
* @brief Function implementing the rfm95wTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_rfm95wTaskEntry */
void rfm95wTaskEntry(void *argument)
{
  /* USER CODE BEGIN rfm95wTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END rfm95wTask */
}

/* USER CODE BEGIN Header_GPSTaskEntry */
/**
* @brief Function implementing the GPSTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_GPSTaskEntry */
void GPSTaskEntry(void *argument)
{
  /* USER CODE BEGIN GPSTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END GPSTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

