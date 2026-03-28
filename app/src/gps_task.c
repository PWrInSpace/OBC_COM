/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 26.03.2026
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gps_task.h"
#include "usart.h"
#include "GNSS.h"
#include "usb_config.h"
#include "sd_task.h"
#include "logger.h"

#define TAG "GPS"

GNSS_StateHandle gpsHandle;
osThreadId_t gpsTaskHandle;

const uint8_t disableNmeaAll[]          = {0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x9D, 0xDF};
const uint8_t disableInfMessages[]      = {0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x92, 0x20, 0x00, 0xB0, 0x63};
const uint8_t setRocketMode4G[]         = {0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x21, 0x00, 0x11, 0x20, 0x08, 0x94, 0xB7};
const uint8_t enableNavPvt[]            = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x18, 0xE1};
const uint8_t setRate1Hz_M10[]          = {0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x21, 0x30, 0xE8, 0x03, 0xF1, 0xAE};
const char* setUart1OnlyUbxNmea         = "$PUBX,41,1,0003,0001,9600,0*16\r\n";

osThreadAttr_t gpsTask_attributes = {
        .name = "gpsTask",
        .stack_size = 512 * 4,
        .priority = (osPriority_t) osPriorityNormal,
    };

void start_gps_task() {
    gpsTaskHandle = osThreadNew(gps_task, NULL, &gpsTask_attributes);
}

GNSS_StateHandle gpsHandle;
osThreadId_t gpsTaskHandle;


/**
 * @brief Szuka nagłówka NAV-PVT w buforze 256B i kopiuje 100B na początek bufora roboczego.
 */
uint8_t GPS_AlignBuffer(GNSS_StateHandle *GNSS) {
    int found_at = -1;
    uint8_t temp[100];
    const uint16_t BUF_SIZE = 256;

    for (int i = 0; i < (BUF_SIZE - 4); i++) {
        if (GNSS->uartWorkingBuffer[i] == 0xB5 && 
            GNSS->uartWorkingBuffer[i + 1] == 0x62 &&
            GNSS->uartWorkingBuffer[i + 2] == 0x01 &&
            GNSS->uartWorkingBuffer[i + 3] == 0x07) {
            found_at = i;
            break;
        }
    }

    if (found_at != -1) {
        for (int j = 0; j < 100; j++) {
            temp[j] = GNSS->uartWorkingBuffer[(found_at + j) % BUF_SIZE];
        }
        memcpy(GNSS->uartWorkingBuffer, temp, 100);
        return 1;
    }
    return 0;
}

void GPS_PrintStatus(GNSS_StateHandle *GNSS) {
    // Logujemy bezpośrednio przez LOG_INFO, aby uniknąć podwójnego buforowania 
    // i marnowania stosu na char log_msg[160]
    
    LOG_INFO("GNSS > [%02u:%02u:%02u] SVs:%u Fix:%u | %.7f, %.7f | Alt:%.2fm | Spd:%.1f km/h", 
        (unsigned int)GNSS->hour, 
        (unsigned int)GNSS->min, 
        (unsigned int)GNSS->sec,
        (unsigned int)GNSS->numSV, 
        (unsigned int)GNSS->fixType, 
        GNSS->fLat, 
        GNSS->fLon, 
        (float)GNSS->hMSL / 1000.0f,
        GNSS->fGSpeedKmH);
}

void configure_gps() {
    uint8_t dummy;
    
    HAL_GPIO_WritePin(GPS_RST_GPIO_Port, GPS_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPS_RST_GPIO_Port, GPS_RST_Pin, GPIO_PIN_SET);
    osDelay(200);

    while(HAL_UART_Receive(&huart1, &dummy, 1, 5) == HAL_OK);
    
    HAL_UART_Transmit(&huart1, (uint8_t*)setUart1OnlyUbxNmea, strlen(setUart1OnlyUbxNmea), 100);
    osDelay(50);
    HAL_UART_Transmit(&huart1, (uint8_t*)disableInfMessages, sizeof(disableInfMessages), 100);
    osDelay(50);
    HAL_UART_Transmit(&huart1, (uint8_t*)disableNmeaAll, sizeof(disableNmeaAll), 100);
    osDelay(50);
    HAL_UART_Transmit(&huart1, (uint8_t*)setRocketMode4G, sizeof(setRocketMode4G), 100);
    osDelay(50);
    HAL_UART_Transmit(&huart1, (uint8_t*)enableNavPvt, sizeof(enableNavPvt), 100);
    osDelay(50);
    HAL_UART_Transmit(&huart1, (uint8_t*)setRate1Hz_M10, sizeof(setRate1Hz_M10), 100);
    osDelay(100);

    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, gpsHandle.uartWorkingBuffer, 256);
}

void gps_task(void *argument) {
    (void)argument;
    
    configure_gps();

    for(;;) {

        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1100)) > 0) {
            
            if (GPS_AlignBuffer(&gpsHandle)) {
    GNSS_ParsePVTData(&gpsHandle);
    
    if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
        g_system_state.lat = gpsHandle.lat;
        g_system_state.lon = gpsHandle.lon;
        g_system_state.hMSL = gpsHandle.hMSL;
        g_system_state.gSpeed = gpsHandle.gSpeed;
        g_system_state.numSV = gpsHandle.numSV;
        g_system_state.fixType = gpsHandle.fixType;
        xSemaphoreGive(g_state_mutex);
        
    }
        
       GPS_PrintStatus(&gpsHandle);
}

            HAL_UART_AbortReceive(&huart1);
            __HAL_UART_SEND_REQ(&huart1, UART_RXDATA_FLUSH_REQUEST);
            HAL_UARTEx_ReceiveToIdle_DMA(&huart1, gpsHandle.uartWorkingBuffer, 256);

        } else {
            USB_Transmit((uint8_t*)"OBC_GPS: TIMEOUT\r\n", 18);
            HAL_UART_AbortReceive(&huart1);
            HAL_UARTEx_ReceiveToIdle_DMA(&huart1, gpsHandle.uartWorkingBuffer, 256);
        }
    }
}
