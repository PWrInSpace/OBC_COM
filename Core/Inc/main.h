/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

#include "stm32h5xx_ll_crc.h"
#include "stm32h5xx_ll_bus.h"
#include "stm32h5xx_ll_cortex.h"
#include "stm32h5xx_ll_rcc.h"
#include "stm32h5xx_ll_system.h"
#include "stm32h5xx_ll_utils.h"
#include "stm32h5xx_ll_pwr.h"
#include "stm32h5xx_ll_gpio.h"
#include "stm32h5xx_ll_dma.h"

#include "stm32h5xx_ll_exti.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "queue.h"
#include "stream_buffer.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define RX_BUF_SIZE 512
extern volatile uint8_t rx_buffer[RX_BUF_SIZE];

#define POOL_SIZE 8
#define BUFFER_SIZE 512

#define USB_EVENT_BIT ( 1 << 2 )
#define USART_LORA_EVENT_BIT  ( 1 << 3 )
#define USART_LORA_EVENT_BIT  ( 1 << 3 )

typedef struct {
    uint8_t data[BUFFER_SIZE];
    uint16_t len;
} CMD_Buffer_t;
extern QueueHandle_t free_pool_queue;

extern StreamBufferHandle_t xUsbStreamBuffer;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LNA2_CTRL_Pin GPIO_PIN_2
#define LNA2_CTRL_GPIO_Port GPIOE
#define PA2_CTRL_Pin GPIO_PIN_3
#define PA2_CTRL_GPIO_Port GPIOE
#define SW2_CTRL1_Pin GPIO_PIN_4
#define SW2_CTRL1_GPIO_Port GPIOE
#define SW2_CTRL2_Pin GPIO_PIN_5
#define SW2_CTRL2_GPIO_Port GPIOE
#define HEADER1_Pin GPIO_PIN_13
#define HEADER1_GPIO_Port GPIOC
#define PA1_PWR_SENSE_Pin GPIO_PIN_1
#define PA1_PWR_SENSE_GPIO_Port GPIOC
#define PA2_PWR_SENSE_Pin GPIO_PIN_2
#define PA2_PWR_SENSE_GPIO_Port GPIOC
#define PA3_PWR_SENSE_Pin GPIO_PIN_3
#define PA3_PWR_SENSE_GPIO_Port GPIOC
#define SX1280_RESET_Pin GPIO_PIN_4
#define SX1280_RESET_GPIO_Port GPIOA
#define SX1280_SCK_Pin GPIO_PIN_5
#define SX1280_SCK_GPIO_Port GPIOA
#define SX1280_MISO_Pin GPIO_PIN_6
#define SX1280_MISO_GPIO_Port GPIOA
#define SX1280_MOSI_Pin GPIO_PIN_7
#define SX1280_MOSI_GPIO_Port GPIOA
#define PA1_DETECT_Pin GPIO_PIN_4
#define PA1_DETECT_GPIO_Port GPIOC
#define PA2_DETECT_Pin GPIO_PIN_5
#define PA2_DETECT_GPIO_Port GPIOC
#define PA3_DETECT_Pin GPIO_PIN_0
#define PA3_DETECT_GPIO_Port GPIOB
#define PA4_DETECT_Pin GPIO_PIN_1
#define PA4_DETECT_GPIO_Port GPIOB
#define SX1280_CS_Pin GPIO_PIN_7
#define SX1280_CS_GPIO_Port GPIOE
#define SX1280_DIO1_Pin GPIO_PIN_8
#define SX1280_DIO1_GPIO_Port GPIOE
#define SX1280_DIO2_Pin GPIO_PIN_9
#define SX1280_DIO2_GPIO_Port GPIOE
#define SX1280_BUSY_Pin GPIO_PIN_10
#define SX1280_BUSY_GPIO_Port GPIOE
#define PA4_CTRL_Pin GPIO_PIN_11
#define PA4_CTRL_GPIO_Port GPIOE
#define SW4_CTRL1_Pin GPIO_PIN_12
#define SW4_CTRL1_GPIO_Port GPIOE
#define SW4_CTRL2_Pin GPIO_PIN_13
#define SW4_CTRL2_GPIO_Port GPIOE
#define LNA4_CTRL_Pin GPIO_PIN_14
#define LNA4_CTRL_GPIO_Port GPIOE
#define CAN_STANDBY_Pin GPIO_PIN_10
#define CAN_STANDBY_GPIO_Port GPIOB
#define RFM95W_SCK_Pin GPIO_PIN_13
#define RFM95W_SCK_GPIO_Port GPIOB
#define RFM95W_MISO_Pin GPIO_PIN_14
#define RFM95W_MISO_GPIO_Port GPIOB
#define RFM95W_MOSI_Pin GPIO_PIN_15
#define RFM95W_MOSI_GPIO_Port GPIOB
#define RFM95W_CS_Pin GPIO_PIN_8
#define RFM95W_CS_GPIO_Port GPIOD
#define RFM95W_RST_Pin GPIO_PIN_10
#define RFM95W_RST_GPIO_Port GPIOD
#define RFM95W_DIO_Pin GPIO_PIN_11
#define RFM95W_DIO_GPIO_Port GPIOD
#define RFM95W_DIO_EXTI_IRQn EXTI11_IRQn
#define LNA3_CTRL_Pin GPIO_PIN_12
#define LNA3_CTRL_GPIO_Port GPIOD
#define PA3_CTRL_Pin GPIO_PIN_13
#define PA3_CTRL_GPIO_Port GPIOD
#define SW3_CTRL1_Pin GPIO_PIN_14
#define SW3_CTRL1_GPIO_Port GPIOD
#define SW3_CTRL2_Pin GPIO_PIN_15
#define SW3_CTRL2_GPIO_Port GPIOD
#define SD_STATUS_Pin GPIO_PIN_7
#define SD_STATUS_GPIO_Port GPIOC
#define SD_DETECT_Pin GPIO_PIN_9
#define SD_DETECT_GPIO_Port GPIOA
#define STATUS_LED_Pin GPIO_PIN_10
#define STATUS_LED_GPIO_Port GPIOA
#define SD_CMD_Pin GPIO_PIN_2
#define SD_CMD_GPIO_Port GPIOD
#define PA1_CTRL_Pin GPIO_PIN_3
#define PA1_CTRL_GPIO_Port GPIOD
#define GPS_RST_Pin GPIO_PIN_7
#define GPS_RST_GPIO_Port GPIOD
#define SW1_CTRL2_Pin GPIO_PIN_3
#define SW1_CTRL2_GPIO_Port GPIOB
#define SW1_CTRL1_Pin GPIO_PIN_4
#define SW1_CTRL1_GPIO_Port GPIOB
#define LNA1_CTRL_Pin GPIO_PIN_5
#define LNA1_CTRL_GPIO_Port GPIOB
#define GPS_UART_TX_Pin GPIO_PIN_6
#define GPS_UART_TX_GPIO_Port GPIOB
#define GPS_UART_RX_Pin GPIO_PIN_7
#define GPS_UART_RX_GPIO_Port GPIOB
#define GPS_I2C_SCL_Pin GPIO_PIN_8
#define GPS_I2C_SCL_GPIO_Port GPIOB
#define GPS_I2C_SDA_Pin GPIO_PIN_9
#define GPS_I2C_SDA_GPIO_Port GPIOB
#define GPS_INIT_Pin GPIO_PIN_0
#define GPS_INIT_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
