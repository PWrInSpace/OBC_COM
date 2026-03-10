/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */
/* USER CODE END 1 */

/** Configure pins
     PH0-OSC_IN(PH0)   ------> RCC_OSC_IN
     PH1-OSC_OUT(PH1)   ------> RCC_OSC_OUT
     PA13(JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
     PA14(JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
     PD2   ------> SDMMC1_CMD
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LNA2_CTRL_Pin|PA2_CTRL_Pin|SW2_CTRL1_Pin|SW2_CTRL2_Pin
                          |SX1280_CS_Pin|PA4_CTRL_Pin|SW4_CTRL1_Pin|SW4_CTRL2_Pin
                          |LNA4_CTRL_Pin|GPS_INIT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(HEADER1_GPIO_Port, HEADER1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SX1280_RESET_Pin|STATUS_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CAN_STANDBY_Pin|SW1_CTRL2_Pin|SW1_CTRL1_Pin|LNA1_CTRL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, RFM95W_CS_Pin|LNA3_CTRL_Pin|PA3_CTRL_Pin|SW3_CTRL1_Pin
                          |SW3_CTRL2_Pin|PA1_CTRL_Pin|GPS_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RFM95W_RST_GPIO_Port, RFM95W_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_STATUS_GPIO_Port, SD_STATUS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LNA2_CTRL_Pin PA2_CTRL_Pin SW2_CTRL1_Pin SW2_CTRL2_Pin
                           SX1280_CS_Pin PA4_CTRL_Pin SW4_CTRL1_Pin SW4_CTRL2_Pin
                           LNA4_CTRL_Pin GPS_INIT_Pin */
  GPIO_InitStruct.Pin = LNA2_CTRL_Pin|PA2_CTRL_Pin|SW2_CTRL1_Pin|SW2_CTRL2_Pin
                          |SX1280_CS_Pin|PA4_CTRL_Pin|SW4_CTRL1_Pin|SW4_CTRL2_Pin
                          |LNA4_CTRL_Pin|GPS_INIT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : HEADER1_Pin SD_STATUS_Pin */
  GPIO_InitStruct.Pin = HEADER1_Pin|SD_STATUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SX1280_RESET_Pin STATUS_LED_Pin */
  GPIO_InitStruct.Pin = SX1280_RESET_Pin|STATUS_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SX1280_DIO1_Pin SX1280_DIO2_Pin SX1280_BUSY_Pin */
  GPIO_InitStruct.Pin = SX1280_DIO1_Pin|SX1280_DIO2_Pin|SX1280_BUSY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : CAN_STANDBY_Pin SW1_CTRL2_Pin SW1_CTRL1_Pin LNA1_CTRL_Pin */
  GPIO_InitStruct.Pin = CAN_STANDBY_Pin|SW1_CTRL2_Pin|SW1_CTRL1_Pin|LNA1_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : RFM95W_CS_Pin LNA3_CTRL_Pin PA3_CTRL_Pin SW3_CTRL1_Pin
                           SW3_CTRL2_Pin PA1_CTRL_Pin GPS_RST_Pin */
  GPIO_InitStruct.Pin = RFM95W_CS_Pin|LNA3_CTRL_Pin|PA3_CTRL_Pin|SW3_CTRL1_Pin
                          |SW3_CTRL2_Pin|PA1_CTRL_Pin|GPS_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : RFM95W_RST_Pin */
  GPIO_InitStruct.Pin = RFM95W_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RFM95W_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RFM95W_DIO_Pin */
  GPIO_InitStruct.Pin = RFM95W_DIO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RFM95W_DIO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_DETECT_Pin */
  GPIO_InitStruct.Pin = SD_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SD_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CMD_Pin */
  GPIO_InitStruct.Pin = SD_CMD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(SD_CMD_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI11_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI11_IRQn);

}

/* USER CODE BEGIN 2 */
/* USER CODE END 2 */
