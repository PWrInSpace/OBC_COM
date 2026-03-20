/*
 * GNSS.c
 *
 *  Created on: 03.10.2020
 *      Author: SimpleMethod
 *
 *Copyright 2020 SimpleMethod
 *
 *Permission is hereby granted, free of charge, to any person obtaining a copy of
 *this software and associated documentation files (the "Software"), to deal in
 *the Software without restriction, including without limitation the rights to
 *use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *of the Software, and to permit persons to whom the Software is furnished to do
 *so, subject to the following conditions:
 *
 *The above copyright notice and this permission notice shall be included in all
 *copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *THE SOFTWARE.
 ******************************************************************************
 */

#include "GNSS.h"
#include <string.h>


union u_Short uShort;
union i_Short iShort;
union u_Long uLong;
union i_Long iLong;



/*!
 * Structure initialization.
 * @param GNSS Pointer to main GNSS structure.
 * @param huart Pointer to uart handle.
 */
void GNSS_Init(GNSS_StateHandle *GNSS, UART_HandleTypeDef *huart) {
	memset(GNSS, 0, sizeof(GNSS_StateHandle));
	GNSS->huart = huart;
}
/**
 * Parse received data from GPS module.
 * @param GNSS Pointer to main GNSS structure.
 */

void GNSS_ParsePVTData(GNSS_StateHandle *GNSS) {

    GNSS->year = (uint16_t)(GNSS->uartWorkingBuffer[10] | (GNSS->uartWorkingBuffer[11] << 8));
    GNSS->month = GNSS->uartWorkingBuffer[12];
    GNSS->day = GNSS->uartWorkingBuffer[13];
    GNSS->hour = GNSS->uartWorkingBuffer[14];
    GNSS->min = GNSS->uartWorkingBuffer[15];
    GNSS->sec = GNSS->uartWorkingBuffer[16];
 
    GNSS->fixType = GNSS->uartWorkingBuffer[26];

    int32_t tempLon = (int32_t)(GNSS->uartWorkingBuffer[30] | (GNSS->uartWorkingBuffer[31] << 8) | (GNSS->uartWorkingBuffer[32] << 16) | (GNSS->uartWorkingBuffer[33] << 24));
    int32_t tempLat = (int32_t)(GNSS->uartWorkingBuffer[34] | (GNSS->uartWorkingBuffer[35] << 8) | (GNSS->uartWorkingBuffer[36] << 16) | (GNSS->uartWorkingBuffer[37] << 24));
    
    GNSS->lon = tempLon;
    GNSS->lat = tempLat;
    GNSS->fLon = (float)tempLon / 10000000.0f;
    GNSS->fLat = (float)tempLat / 10000000.0f;
    GNSS->hMSL = (int32_t)(GNSS->uartWorkingBuffer[42] | (GNSS->uartWorkingBuffer[43] << 8) | (GNSS->uartWorkingBuffer[44] << 16) | (GNSS->uartWorkingBuffer[45] << 24));

    // 5. Prędkość (gSpeed) i Heading (headMot)
    // gSpeed [mm/s] (Offset 60 + 6 = Indeks 66)
    GNSS->gSpeed = (int32_t)(GNSS->uartWorkingBuffer[66] | (GNSS->uartWorkingBuffer[67] << 8) | (GNSS->uartWorkingBuffer[68] << 16) | (GNSS->uartWorkingBuffer[69] << 24));
    
    // Konwersja na km/h (mm/s * 0.0036) - kluczowe dla Mach 1
    GNSS->fGSpeedKmH = (float)GNSS->gSpeed * 0.0036f;

    // headMot [deg * 1e-5] (Offset 64 + 6 = Indeks 70)
    int32_t tempHead = (int32_t)(GNSS->uartWorkingBuffer[70] | (GNSS->uartWorkingBuffer[71] << 8) | (GNSS->uartWorkingBuffer[72] << 16) | (GNSS->uartWorkingBuffer[73] << 24));
    GNSS->headMot = (float)tempHead / 100000.0f;
}

void GNSS_LoadConfig(GNSS_StateHandle *GNSS) {

    HAL_UART_Transmit(GNSS->huart, (uint8_t*)getDeviceID_M10, 8, 100);
    if(HAL_UART_Receive(GNSS->huart, GNSS->uartWorkingBuffer, 18, 200) == HAL_OK) {
        for (int i = 0; i < 6; i++) {
            GNSS->uniqueID[i] = GNSS->uartWorkingBuffer[10 + i];
        }
    }
    HAL_Delay(50);
    HAL_UART_Transmit(GNSS->huart, (uint8_t*)disableNmeaUart1, sizeof(disableNmeaUart1), 100);
    HAL_Delay(50);
    HAL_UART_Transmit(GNSS->huart, (uint8_t*)setRocketMode4G, sizeof(setRocketMode4G), 100);
    HAL_Delay(50);
    HAL_UART_Transmit(GNSS->huart, (uint8_t*)setRate1Hz_M10, sizeof(setRate1Hz_M10), 100);
    HAL_Delay(50);
    HAL_UART_Transmit(GNSS->huart, (uint8_t*)enableNavPvt, sizeof(enableNavPvt), 100);
    HAL_Delay(50);
}



/*!
 * Calculate checksum for sent data.
 * Look at: 32.3.2 u-blox 8 Receiver description.
 * @param data Pointer to data array.
 * @param len Length of data array.
 * @param ck_a Pointer to variable where CK_A will be stored.
 * @param ck_b Pointer to variable where CK_B will be stored.
 */
void GNSS_CalcChecksum(uint8_t* data, uint8_t len, uint8_t* ck_a, uint8_t* ck_b) {
    *ck_a = 0;
    *ck_b = 0;
    for (uint8_t i = 2; i < len - 2; i++) { // Pomijamy 0xB5 0x62 i ostatnie 2 bajty sumy
        *ck_a = *ck_a + data[i];
        *ck_b = *ck_b + *ck_a;
    }
}
