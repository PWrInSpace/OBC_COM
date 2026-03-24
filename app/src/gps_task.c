#include <stdio.h>      // Dla snprintf
#include <string.h>     // Dla memset (jeśli używasz)
#include "gps_task.h"
#include "usart.h"
#include "GNSS.h"
#include "usb_config.h"

GNSS_StateHandle gpsHandle;



osThreadId_t gpsTaskHandle;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(gpsTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

osThreadAttr_t gpsTask_attributes = {
        .name = "gpsTask",
        .stack_size = 512 * 4,
        .priority = (osPriority_t) osPriorityNormal,
    };

void start_gps_task() {
    gpsTaskHandle = osThreadNew(gps_task, NULL, &gpsTask_attributes);
}

// Definicja ramki zapytania (musi być poza funkcją lub na jej początku)
 const uint8_t UBX_SEC_UNIQID_POLL[] = {0xB5, 0x62, 0x27, 0x03, 0x00, 0x00, 0x2A, 0xA5};
const uint8_t UBX_MON_VER_POLL[] = {0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34};

void gps_task(void *argument) {
    (void)argument;
    uint8_t rx_buf[64];
    uint8_t dummy;
    char msg[128];
    HAL_StatusTypeDef status;

    USB_Transmit((uint8_t*)"\r\n--- GPS M10 CONFIG START ---\r\n", 32);

    for(;;) {
        // 1. CZYSZCZENIE (FLUSH): Wyrzucamy wszystko, co GPS wysłał sam z siebie (NMEA)
        // Robimy to tak długo, aż przez 10ms nic nowego nie wpadnie
        while(HAL_UART_Receive(&huart1, &dummy, 1, 10) == HAL_OK);

        // 2. WYŁĄCZENIE NMEA: Wysyłamy komendę VALSET
        USB_Transmit((uint8_t*)"GPS: Wylaczam NMEA...\r\n", 23);
        HAL_UART_Transmit(&huart1, (uint8_t*)disableNmeaUart1, sizeof(disableNmeaUart1), 100);
        HAL_Delay(200); // Daj modułowi czas na przetworzenie

        // Ponownie czyścimy resztki NMEA, które mogły być "w drodze"
        while(HAL_UART_Receive(&huart1, &dummy, 1, 10) == HAL_OK);

        // 3. PYTANIE O ID: Wysyłamy zapytanie o Unique ID
        USB_Transmit((uint8_t*)"GPS: Pytam o ID...\r\n", 20);
        HAL_UART_Transmit(&huart1, (uint8_t*)getDeviceID_M10, sizeof(getDeviceID_M10), 100);

        // 4. ODBIÓR ODPOWIEDZI: Czekamy na 18 bajtów (Nagłówek 6 + Payload 10 + CK 2)
        memset(rx_buf, 0, sizeof(rx_buf));
        status = HAL_UART_Receive(&huart1, rx_buf, 18, 1000);

        if (status == HAL_OK) {
            // Sprawdzamy czy to ramka UBX-SEC-UNIQID (0xB5 0x62 0x27 0x03)
            if (rx_buf[0] == 0xB5 && rx_buf[1] == 0x62 && rx_buf[2] == 0x27 && rx_buf[3] == 0x03) {
                
                // ID zaczyna się od 10-go bajtu (indeks 10, długość 6 bajtów)
                int len = snprintf(msg, sizeof(msg), 
                                   "GPS SUCCESS! Unique ID: %02X%02X%02X%02X%02X%02X\r\n", 
                                   rx_buf[10], rx_buf[11], rx_buf[12], 
                                   rx_buf[13], rx_buf[14], rx_buf[15]);
                USB_Transmit((uint8_t*)msg, (uint16_t)len);
                
                // Jeśli sukces, możemy wyjść z pętli testowej i przejść do pracy z DMA
                break; 
            } else {
                int len = snprintf(msg, sizeof(msg), "GPS: Odebrano dane, ale zly naglowek: %02X %02X\r\n", rx_buf[0], rx_buf[1]);
                USB_Transmit((uint8_t*)msg, (uint16_t)len);
            }
        } else {
            USB_Transmit((uint8_t*)"GPS: Brak odpowiedzi (Timeout). Sprawdz Baudrate 9600!\r\n", 55);
        }

        osDelay(3000); // Ponów próbę za 3 sekundy
    }

    // --- KONIEC TESTU, START NORMALNEJ PRACY ---
    USB_Transmit((uint8_t*)"GPS: Konfiguracja zakonczona. Przechodze w tryb DMA.\r\n", 54);
    
    // Tutaj możesz odpalić właściwe DMA do odbierania pozycji NAV-PVT
    // HAL_UART_Receive_DMA(&huart1, gpsHandle.uartWorkingBuffer, 100);

    for(;;) {
        osDelay(1000);
    }
}

// 2. FUNKCJA DIAGNOSTYCZNA
void check_gps_communication(void) {
    uint8_t response[20] = {0}; // Bufor na 18 bajtów odpowiedzi
    char msg[128];
    
    USB_Transmit((uint8_t*)"GPS: Odczyt Unikalnego ID...\r\n", 30);

    // 1. Wyślij zapytanie (Poll)
    HAL_UART_Transmit(&huart1, (uint8_t*)UBX_SEC_UNIQID_POLL, sizeof(UBX_SEC_UNIQID_POLL), 100);

    // 2. Odbierz odpowiedź (18 bajtów zgodnie z tabelą: Header(6) + Payload(10) + CK(2))
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, response, 18, 1000);

    if (status == HAL_OK) {
        // Sprawdź czy to na pewno UBX-SEC-UNIQID (0xB5 0x62 0x27 0x03)
        if (response[0] == 0xB5 && response[1] == 0x62 && response[2] == 0x27 && response[3] == 0x03) {
            
            // Wersja wiadomości (bajt 6 bufora, offset 0 payloadu)
            uint8_t version = response[6]; 
            
            // Unique ID (6 bajtów, zaczyna się od bajtu 10 bufora - offset 4 payloadu)
            int len = snprintf(msg, sizeof(msg), 
                               "GPS SUCCESS! Ver: 0x%02X, ID: %02X%02X%02X%02X%02X%02X\r\n", 
                               version, response[10], response[11], response[12], 
                               response[13], response[14], response[15]);
            
            USB_Transmit((uint8_t*)msg, (uint16_t)len);
        } else {
            // Jeśli nagłówek jest inny, wypisz co przyszło (pomoże w debugowaniu NMEA)
            int len = snprintf(msg, sizeof(msg), "GPS: Bledny naglowek! Odebrano: %02X %02X %02X %02X\r\n", 
                               response[0], response[1], response[2], response[3]);
            USB_Transmit((uint8_t*)msg, (uint16_t)len);
        }
    } else {
        USB_Transmit((uint8_t*)"GPS: Timeout - Brak odpowiedzi z modulu.\r\n", 42);
    }
}
// void gps_task(void *argument) {
//     uint8_t raw_byte;
//     char debug[32];

//     for(;;) {
//         // Czekamy na DOWOLNY 1 bajt przez 2 sekundy
//         if (HAL_UART_Receive(&huart1, &raw_byte, 1, 2000) == HAL_OK) {
//             // Jeśli cokolwiek przyszło, mrugnij diodą lub wyślij na USB
//             int len = snprintf(debug, sizeof(debug), "Recv: 0x%02X ('%c')\r\n", raw_byte, (raw_byte > 31 ? raw_byte : ' '));
//             USB_Transmit((uint8_t*)debug, len);
//         } else {
//             USB_Transmit((uint8_t*)"Linia milczy...\r\n", 17);
//         }
//     }
// }

