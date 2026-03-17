#include "cmd_interface.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include"main.h"

// Zakładamy, że USB_Transmit jest zdefiniowane w innym pliku
extern void USB_Transmit(uint8_t* Buf, uint16_t Len);

typedef struct {
    const char *cmd_str;   // Nazwa komendy (np. "reset")
    Command_t cmd_enum;    // Odpowiadający enum
    const char *help_info; // Opis dla komendy help
} CommandMap_t;

static const CommandMap_t cmd_map[] = {
    {"help",             CMD_HELP,     ""}, // help nie potrzebuje opisu samego siebie
    {"sx1280_frequency", CMD_SX1280_FREQ, ":Hz  - Set frequency"},
    {"sx1280_tx_power",  CMD_SX1280_PWR,  ":dBm - Set TX power"},
    {"reset",            CMD_RESET,    "     - Reboot system"},
    {"status",           CMD_STATUS,   "    - Show radio status"}
};

#define CMD_MAP_SIZE (sizeof(cmd_map) / sizeof(CommandMap_t))

void process_command(char *rx_buf) {
    char response[160];
    
    char *prefix  = strtok(rx_buf, ":");
    char *cmd_str = strtok(NULL, ":");
    char *val_str = strtok(NULL, ":");

    if (prefix == NULL || strcmp(prefix, "CMD") != 0 || cmd_str == NULL) {
        USB_Transmit((uint8_t*)"ERROR: Invalid format. Use CMD:name:val\r\n", 41);
        return;
    }

    // 2. Szukanie komendy w tabeli
    Command_t active_cmd = CMD_UNKNOWN;
    for (size_t i = 0; i < CMD_MAP_SIZE; i++) {
        if (strcmp(cmd_str, cmd_map[i].cmd_str) == 0) {
            active_cmd = cmd_map[i].cmd_enum;
            break;
        }
    }

    // 3. Wykonanie akcji
    switch (active_cmd) {
        case CMD_HELP:
           {
    char help_line[80];
    const char *header = "\r\n--- OBC COMMAND HELP ---\r\n";
    USB_Transmit((uint8_t*)header, strlen(header));

    for (size_t i = 0; i < CMD_MAP_SIZE; i++) {
        // Formatujemy linię: "CMD:nazwa:opis\r\n"
        int len = snprintf(help_line, sizeof(help_line), "CMD:%s%s\r\n", 
                           cmd_map[i].cmd_str, 
                           cmd_map[i].help_info);
        
        USB_Transmit((uint8_t*)help_line, len);
    }
    
    const char *footer = "------------------------\r\n";
    USB_Transmit((uint8_t*)footer, strlen(footer));
}
break;

        case CMD_SX1280_FREQ:
            if (val_str != NULL) {
                uint32_t freq = strtoul(val_str, NULL, 10);
                // Tutaj logika zapisu do radia (RadioCommands_t -> RADIO_SET_RFFREQUENCY)
                snprintf(response, sizeof(response), "OK: Frequency set to %lu Hz\r\n", freq);
                USB_Transmit((uint8_t*)response, strlen(response));
            }
            break;

        case CMD_SX1280_PWR:
             if (val_str != NULL) {
                int8_t pwr = (int8_t)atoi(val_str);
                // Tutaj logika zapisu (RadioCommands_t -> RADIO_SET_TXPARAMS)
                snprintf(response, sizeof(response), "OK: Power set to %d dBm\r\n", pwr);
                USB_Transmit((uint8_t*)response, strlen(response));
            }
            break;

        case CMD_RESET:
            USB_Transmit((uint8_t*)"SYSTEM: Resetting...\r\n", 22);
            vTaskDelay(pdMS_TO_TICKS(100)); // Używamy vTaskDelay bo jesteśmy w RTOS
            NVIC_SystemReset();
            break;

        default:
            snprintf(response, sizeof(response), "ERROR: Unknown command [%s]\r\n", cmd_str);
            USB_Transmit((uint8_t*)response, strlen(response));
            break;
    }
}