/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os2.h"
#include "adc.h"
#include "crc.h"
#include "fdcan.h"
#include "gpdma.h"
#include "i2c.h"
#include "sdmmc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "core_cm33.h"
#include "usb_config.h"
#include "cmd_task.h"
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

/* USER CODE BEGIN PV */
/* W sekcji Private variables (PV) */
#define RX_BUF_SIZE 512
uint8_t volatile rx_buffer[RX_BUF_SIZE];
uint16_t last_packet_size = 0;


UART_Buffer_t pool[POOL_SIZE];
QueueHandle_t free_pool_queue = NULL;
extern QueueHandle_t cmd_queue;

void BufferPool_Init(void) {
    free_pool_queue = xQueueCreate(POOL_SIZE, sizeof(UART_Buffer_t*));
    cmd_queue = xQueueCreate(POOL_SIZE, sizeof(UART_Buffer_t*));

    for (int i = 0; i < POOL_SIZE; i++) {
        UART_Buffer_t *ptr = &pool[i];
        xQueueSend(free_pool_queue, &ptr, 0);
    }
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

extern osThreadId_t sx1280TaskHandle;
extern osThreadId_t rfm95wTaskHandle;
extern osThreadId_t gpsTaskHandle;

#define RADIO_EVENT_BIT (1 << 0)

// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;

//     // --- Obsługa SX1280 ---
//     if (GPIO_Pin == SX1280_DIO1_Pin || GPIO_Pin == SX1280_DIO2_Pin) {
//         if (sx1280TaskHandle != NULL) {
//             xTaskNotifyFromISR(sx1280TaskHandle, RADIO_EVENT_BIT, eSetBits, &xHigherPriorityTaskWoken);
//         }
//     }

//     // --- Obsługa RFM95W ---
//     if (GPIO_Pin == RFM95W_DIO_Pin) {
//         if (rfm95wTaskHandle != NULL) {
//     vTaskNotifyGiveFromISR(rfm95wTaskHandle, &xHigherPriorityTaskWoken);
//         }
//     }

//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// }

/**
  * @brief  Obsługa zdarzenia Idle Line lub Full Buffer na USART2
  * @param  huart: wskaźnik na strukturę UART
  * @param  Size: liczba bajtów obliczona przez HAL (ile faktycznie wpadło)
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
   if (huart->Instance == USART2)
{
    // 1. Wizualne potwierdzenie
    HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
    
    // 2. Wysłanie danych do kolejki (jeśli kolejka istnieje)
    if (cmd_queue != NULL) 
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        // WYSYŁAMY odebrany bufor (lub jego fragment o długości 'Size') do kolejki
        // Uwaga: Kolejka musi być zainicjalizowana na odpowiedni rozmiar elementu!
        xQueueSendFromISR(cmd_queue, (void *)rx_buffer, &xHigherPriorityTaskWoken);
        memset(rx_buffer, 0, 512);
        
        // Wymuszenie przełączenia kontekstu, jeśli zadanie czekające na kolejkę ma wyższy priorytet
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    // 3. Czyścimy flagi i uzbrajamy ponownie
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_IDLEF);
    
    if (HAL_UARTEx_ReceiveToIdle_DMA(huart, rx_buffer, 512) == HAL_OK)
    {
        __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
    }
}
        else if (huart->Instance == USART1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(gpsTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
        else
        {
            // Jeśli status nie jest OK (np. HAL_BUSY), oznacza to, że UART ma błąd
            // Wywołujemy procedurę ratunkową
            HAL_UART_ErrorCallback(huart);
        }
    }


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_ADC1_Init();
  MX_FDCAN1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_USB_PCD_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_SDMMC1_SD_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(50);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t*)rx_buffer, RX_BUF_SIZE);
  BufferPool_Init();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Call init function for freertos objects (in app_freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    GPIOA->BSRR = (1U << (10 + 16)); // Zapal
      for(volatile int i = 0; i < 1000000; i++);
      GPIOA->BSRR = (1U << 10);         // Zgaś
      for(volatile int i = 0; i < 1000000; i++);
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }

   /* Select SysTick source clock */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

   /* Re-Initialize Tick with new clock source */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_ConfigAttributes(RCC_ALL, RCC_NSEC_PRIV);

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_1);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    HAL_SD_DeInit(&hsd1);
    return;
  }
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
