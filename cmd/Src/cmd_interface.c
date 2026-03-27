#include "cmd_interface.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "usb_config.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "cmsis_os2.h"
static void process_text_packet(char *raw_str);
static void process_binary_packet(uint8_t *buf, uint16_t len);

static const CommandMap_t cmd_map[] = {
    {"HELP",   CMD_HELP,         handle_help,   "- Show menu"},
    {"FREQ",   CMD_SX1280_FREQ,   handle_freq,   ":Hz - Set freq"},
    {"POWER",  CMD_SX1280_PWR,    handle_power,  ":dBm - Set TX power"},
    {"RESET",  CMD_RESET,        handle_reset,  "- System reboot"},
    {"STATUS", CMD_STATUS,       handle_status, "- Radio status"},
    {"HELP",   CMD_SX1280_TX,    handle_sx1280_tx, ":data - Send via LoRa"},
    {"LORATX",   CMD_LORA_TX,    handle_lora_tx, ":data - Send via LoRa"}
};

extern osThreadId_t rfm95wTaskHandle;
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
extern volatile uint16_t USB_Rx_Data_Len;

void handle_lora_tx(cmd_params_t *params)
{
    // 1. Sprawdź czy mamy dane (np. CMD:LORATX:TEST)
    if (params->len == 0 || params->data == NULL) {
        USB_Transmit((uint8_t*)"ERR: No data to send\r\n", 22);
        return;
    }

    // 2. Skopiuj dane do bufora radiowego (używamy UserRxBufferFS, bo task go czyta)
    uint16_t copy_len = (params->len < APP_RX_DATA_SIZE) ? params->len : APP_RX_DATA_SIZE;
    memcpy(UserRxBufferFS, params->data, copy_len);
    
    // 3. Ustaw długość (to aktywuje 'if' w Twoim rfm95wTaskEntry)
    USB_Rx_Data_Len = copy_len;

    // 4. Powiadom zadanie rfm95w, żeby przerwało ulTaskNotifyTake
    if (rfm95wTaskHandle != NULL) {
        xTaskNotifyGive(rfm95wTaskHandle);
    }

    if (!params->is_binary) {
        USB_Transmit((uint8_t*)"OK: Data queued for RFM95W TX\r\n", 31);
    }
}


const size_t cmd_map_size = sizeof(cmd_map) / sizeof(CommandMap_t);


void process_text_packet(char *raw_str) {
    strtok(raw_str, ":"); // Skip "CMD"
    char *cmd_str = strtok(NULL, ":");
    char *val_str = strtok(NULL, ":");

    if (cmd_str == NULL) return;

    for (size_t i = 0; i < cmd_map_size; i++) {
        if (strcmp(cmd_str, cmd_map[i].name) == 0) {
            cmd_params_t p = {
                .data = (uint8_t*)val_str,
                .len = (val_str ? (uint16_t)strlen(val_str) : 0),
                .is_binary = false
            };
            cmd_map[i].handler(&p);
            return;
        }
    }
}

void process_binary_packet(uint8_t *buf, uint16_t len) {
    if (len < 3) return; // Minimum: Header + Len + ID
    uint8_t data_len = buf[1]; 
    uint8_t cmd_id = buf[2];

    for (size_t i = 0; i < cmd_map_size; i++) {
        if (cmd_map[i].id == (Command_t)cmd_id) {
            cmd_params_t p = {
                .data = (data_len > 0) ? &buf[3] : NULL,
                .len = data_len,
                .is_binary = true
            };
            cmd_map[i].handler(&p);
            return;
        }
    }
}

void process_command(uint8_t *rx_buf, uint16_t len) {
    if (len == 0) return;

    if (rx_buf[0] == 0x32 && len >= 5) {
        process_binary_packet(rx_buf, len);
    }
    else if (len >= 4 && memcmp(rx_buf, "CMD:", 4) == 0) {
        if (len < MAX_CMD_LEN) {
            rx_buf[len] = '\0'; 
            process_text_packet((char*)rx_buf);
        }
    }
}

// HANDLER IMPLEMENTATIONS

void handle_help(cmd_params_t *params) {
    (void)params; // Unused
    char help_line[128];
    USB_Transmit((uint8_t*)"\r\n--- OBC HELP ---\r\n", 19);

    for (size_t i = 0; i < cmd_map_size; i++) {
        int len = snprintf(help_line, sizeof(help_line), "ID:0x%02X | %s %s\r\n", 
                           cmd_map[i].id, cmd_map[i].name, cmd_map[i].help);
        USB_Transmit((uint8_t*)help_line, len);
    }
}

void handle_freq(cmd_params_t *params) {
    uint32_t freq = 0;
    if (params->is_binary && params->len >= 4) {
        memcpy(&freq, params->data, 4);
    } else if (params->data) {
        freq = strtoul((char*)params->data, NULL, 10);
    }

    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: Freq set to %lu Hz\r\n", freq);
    USB_Transmit((uint8_t*)resp, len);
}

void handle_power(cmd_params_t *params) {
    int8_t pwr = 0;
    if (params->is_binary && params->len >= 1) {
        pwr = (int8_t)params->data[0];
    } else if (params->data) {
        pwr = (int8_t)atoi((char*)params->data);
    }
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: Power set to %d dBm\r\n", pwr);
    USB_Transmit((uint8_t*)resp, len);
}

void handle_reset(cmd_params_t *params) {
    (void)params;
    USB_Transmit((uint8_t*)"SYSTEM: Resetting...\r\n", 22);
    vTaskDelay(pdMS_TO_TICKS(100));
    NVIC_SystemReset();
}

void handle_status(cmd_params_t *params) {
    if (params->is_binary) {
        uint8_t resp[] = {0x32, 0x01, CMD_STATUS, 0x01, 0x00, 0x89};
        USB_Transmit(resp, sizeof(resp));
    } else {
        const char *msg = "STATUS: Radio Link OK\r\n";
        USB_Transmit((uint8_t*)msg, strlen(msg));
    }
}

void handle_sx1280_tx(cmd_params_t *params) {
    if (params->len == 0) return;
    // Logika wysyłania...
    if (!params->is_binary) {
        USB_Transmit((uint8_t*)"OK: Text data queued for LoRa\r\n", 31);
    }
}