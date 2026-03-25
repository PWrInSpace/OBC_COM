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

GNSS_StateHandle GNSS_Handle;

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
    // Offsety z dokumentacji + 6 bajtów nagłówka UBX
    
    // 1. Czas i Data
    // Rok (offset 4 w Payloadzie) -> indeks 10
    GNSS->year = (uint16_t)(GNSS->uartWorkingBuffer[10] | (GNSS->uartWorkingBuffer[11] << 8));
    GNSS->month = GNSS->uartWorkingBuffer[12]; // offset 6 -> indeks 12
    GNSS->day   = GNSS->uartWorkingBuffer[13]; // offset 7 -> indeks 13
    GNSS->hour  = GNSS->uartWorkingBuffer[14]; // offset 8 -> indeks 14
    GNSS->min   = GNSS->uartWorkingBuffer[15]; // offset 9 -> indeks 15
    GNSS->sec   = GNSS->uartWorkingBuffer[16]; // offset 10 -> indeks 16

    // 2. Status Fixa i Satelity
    // fixType (offset 20) -> indeks 26
    GNSS->fixType = GNSS->uartWorkingBuffer[26];
    // numSV (offset 23) -> indeks 29
    GNSS->numSV = GNSS->uartWorkingBuffer[29];

    // 3. Pozycja (Lon/Lat: 1e-7 deg)
    // lon (offset 24) -> indeks 30
    int32_t tempLon = (int32_t)(GNSS->uartWorkingBuffer[30] | (GNSS->uartWorkingBuffer[31] << 8) | 
                                (GNSS->uartWorkingBuffer[32] << 16) | (GNSS->uartWorkingBuffer[33] << 24));
    // lat (offset 28) -> indeks 34
    int32_t tempLat = (int32_t)(GNSS->uartWorkingBuffer[34] | (GNSS->uartWorkingBuffer[35] << 8) | 
                                (GNSS->uartWorkingBuffer[36] << 16) | (GNSS->uartWorkingBuffer[37] << 24));
    
    GNSS->lon = tempLon;
    GNSS->lat = tempLat;
    GNSS->fLon = (float)tempLon / 10000000.0f;
    GNSS->fLat = (float)tempLat / 10000000.0f;

    // 4. Wysokość (hMSL: mm) 
    // hMSL (offset 36) -> indeks 42
    GNSS->hMSL = (int32_t)(GNSS->uartWorkingBuffer[42] | (GNSS->uartWorkingBuffer[43] << 8) | 
                           (GNSS->uartWorkingBuffer[44] << 16) | (GNSS->uartWorkingBuffer[45] << 24));

    // 5. Prędkość Ground Speed (offset 60) -> indeks 66
    GNSS->gSpeed = (int32_t)(GNSS->uartWorkingBuffer[66] | (GNSS->uartWorkingBuffer[67] << 8) | 
                             (GNSS->uartWorkingBuffer[68] << 16) | (GNSS->uartWorkingBuffer[69] << 24));
    GNSS->fGSpeedKmH = (float)GNSS->gSpeed * 0.0036f; // Konwersja mm/s na km/h

    // 6. Kurs Heading of Motion (offset 64) -> indeks 70
    int32_t tempHead = (int32_t)(GNSS->uartWorkingBuffer[70] | (GNSS->uartWorkingBuffer[71] << 8) | 
                                 (GNSS->uartWorkingBuffer[72] << 16) | (GNSS->uartWorkingBuffer[73] << 24));
    GNSS->headMot = (float)tempHead / 100000.0f;
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
