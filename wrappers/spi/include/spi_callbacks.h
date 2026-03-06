/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 29.01.2026
 */

#pragma once

#include "stdint.h"
#include "stdbool.h"
#include "spi.h"

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);