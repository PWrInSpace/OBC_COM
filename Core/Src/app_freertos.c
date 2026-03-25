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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "board_data.h"
#include "cmsis_os2.h"
#include "logger.h"
#include "logger_macros.h"
#include "main.h"
#include "projdefs.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_sd.h"
#include "usb_config.h"
#include "stm32h5xx_it.h"
#include "sx1280_task.h"
#include "rfm95w_task.h"
#include "sd_task.h"
#include "cmd_task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
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
uint32_t MyBufferTask00[ 128 ];
osStaticThreadDef_t MycontrolBlocTask00;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_mem = &MyBufferTask00[0],
  .stack_size = sizeof(MyBufferTask00),
  .cb_mem = &MycontrolBlocTask00,
  .cb_size = sizeof(MycontrolBlocTask00),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for usbMutex */
osMutexId_t usbMutexHandle;
const osMutexAttr_t usbMutex_attributes = {
  .name = "usbMutex"
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
  /* creation of usbMutex */
  usbMutexHandle = osMutexNew(&usbMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
//   /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
//   /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
//   /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
//   /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
//   /* add threads, ... */
  //CMD_Task_Init();
  //RFM95W_task_init();
  //SX1280_task_init();
  sd_logger_init();

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
//   /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
// /**
// * @brief Function implementing the defaultTask thread.
// * @param argument: Not used
// * @retval None
// */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
  USB_CDC_Config(); 
  uint8_t msg[] = "Jebać kurwy z STM\r\n";

  for(;;)
  {
    HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
    BoardData_t data = { HAL_GetTick(), 0.0f, 0.0f, 1 };
    sd_logger_log_data(&data);

    osDelay(1000);
  }
  /* USER CODE END defaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

