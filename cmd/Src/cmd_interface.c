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
#include "logger.h"
#include "nvs_config.h"

static void process_text_packet(char *raw_str);
static void process_binary_packet(uint8_t *buf, uint16_t len);

uint8_t LoraRxBuffer[LORA_BUFF_SIZE] = {0};
uint16_t volatile lora_cmd_len = 0;
extern osThreadId_t rfm95wTaskHandle;

static const CommandMap_t cmd_map[] = {
    {"HELP",      CMD_HELP,         handle_help,   "- Show menu"},
    {"FREQ",      CMD_SX1280_FREQ,   handle_freq,   ":Hz - Set freq"},
    {"POWER",     CMD_SX1280_PWR,    handle_power,  ":dBm - Set TX power"},
    { "SF"  ,     CMD_SF,           handle_sf,       "change SF Lora"},
    { "SF"  ,     CMD_BW,           handle_bw,       "change BW Lora"},
    { "CR"  ,     CMD_CR,           handle_cr,       "change CR Lora"},
    { "CRC" ,     CMD_CRC,          handle_crc,      "change CRC Lora"},
    { "SYNC",     CMD_SYNC,         handle_sync,     "change SYNC Lora"},
    {"RESET",     CMD_RESET,        handle_reset,  "- System reboot"},
    {"STATUS",    CMD_STATUS,       handle_status, "- Radio status"},
    {"LORATX",    CMD_LORA_TX,      handle_lora_tx, ":data - Send via LoRa"},
    {"LOGON",     CMD_LOG_ON,       handle_log_on,    "- Enable all logs"},
    {"LOGOFF",    CMD_LOG_OFF,      handle_log_off,   "- Disable all logs"},
    {"LOGMUTE",   CMD_LOG_MUTE,     handle_log_mute,  ":TAG - Mute specific tag"},
    {"LOGUNMUTE", CMD_LOG_UNMUTE,   handle_log_unmute, "- Clear all mutes"},
    {"LORA_MODE", CMD_LORA_MODE,    handle_lora_mode, "- Switch LoRa mode (0 -> sleep mode / 1 -> normal mode)"}
};

extern osThreadId_t rfm95wTaskHandle;
extern volatile uint16_t USB_Rx_Data_Len;


void handle_lora_mode(cmd_params_t *params) {
    int16_t mode = 0;
    if (params->is_binary && params->len >= 2) {
        memcpy(&mode, params->data, 2);
    } else if (params->data) {
        mode = (int16_t)atoi((char*)params->data);
    }
    NVS_Write((RFM95W_PARAM_STATE), (uint32_t)mode);
    xTaskNotify(rfm95wTaskHandle, SETTINGS_CHANGE_EVENT_BIT, eSetBits);
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: LoRa mode set to %d\r\n", mode);
    USB_Transmit((uint8_t*)resp, len);
}
void handle_lora_tx(cmd_params_t *params)
{
    if (params->len == 0 || params->data == NULL) {
        USB_Transmit((uint8_t*)"ERR: No data to send\r\n", 22);
        return;
    }
    uint16_t copy_len = (params->len < LORA_BUFF_SIZE) ? params->len : LORA_BUFF_SIZE;
    memcpy(LoraRxBuffer, params->data, copy_len);
    lora_cmd_len = copy_len;
    if (rfm95wTaskHandle != NULL) {
        xTaskNotify(rfm95wTaskHandle, LORA_TX_EVENT_BIT, eSetBits);
    }

    if (!params->is_binary) {
        USB_Transmit((uint8_t*)"OK: Data queued  text for RFM95W TX\r\n", 31);
    }
    else {
        USB_Transmit((uint8_t*)"OK: Data queued binary for RFM95W TX\r\n", 34); 
    }
    
}

// void handle_lora_tx(cmd_params_t *params)
// {
//     if (params->len == 0 || params->data == NULL) {
//         USB_Transmit((uint8_t*)"ERR: No data to send\r\n", 22);
//         return;
//     }
    
//     // Kopiujemy dane do bufora komend Lora
//     uint16_t copy_len = (params->len < LORA_BUFF_SIZE) ? params->len : LORA_BUFF_SIZE;
//     memcpy(LoraRxBuffer, params->data, copy_len);
//     lora_cmd_len = copy_len;
    
//     // POWIADOMIENIE BITOWE (USART/CMD EVENT)
//     if (rfm95wTaskHandle != NULL) {
//         // Używamy bitu USART_LORA_EVENT_BIT, bo LoraRxBuffer to Twój bufor "interfejsowy"
//         xTaskNotify(rfm95wTaskHandle, USART_LORA_EVENT_BIT, eSetBits);
//     }

//     if (!params->is_binary) {
//         USB_Transmit((uint8_t*)"OK: Data queued for RFM95W TX\r\n", 31);
//     }
// }

const size_t cmd_map_size = sizeof(cmd_map) / sizeof(CommandMap_t);


void process_text_packet(char *raw_str) {
    strtok(raw_str, ";"); // Skip "CMD"
    char *cmd_str = strtok(NULL, ";");
    char *val_str = strtok(NULL, ";");

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
    if (len < 3) return; 
    
    uint8_t cmd_id = buf[1];
    uint8_t data_len = buf[2];

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
    else if (len >= 4 && memcmp(rx_buf, "CMD;", 4) == 0) {
        if (len < MAX_CMD_LEN) {
            rx_buf[len] = '\0'; 
            process_text_packet((char*)rx_buf);
        }
    }
}


void handle_help(cmd_params_t *params) {
    (void)params;
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
    NVS_Write((RFM95W_PARAM_FREQ), (uint32_t)freq);
    xTaskNotify(rfm95wTaskHandle, SETTINGS_CHANGE_EVENT_BIT, eSetBits);
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
    NVS_Write((RFM95W_PARAM_PWR), (uint32_t)pwr);
    xTaskNotify(rfm95wTaskHandle, SETTINGS_CHANGE_EVENT_BIT, eSetBits);
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: Power set to %d dBm\r\n", pwr);
    USB_Transmit((uint8_t*)resp, len);
}

void handle_sf(cmd_params_t *params) {
    int8_t sf = 0;
    if (params->is_binary && params->len >= 1) {
        sf = (int8_t)params->data[0];
    } else if (params->data) {
        sf = (int8_t)atoi((char*)params->data);
    }
    NVS_Write((RFM95W_PARAM_SF), (uint32_t)sf);
    xTaskNotify(rfm95wTaskHandle, SETTINGS_CHANGE_EVENT_BIT, eSetBits);
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: SF set to %d\r\n", sf);
    USB_Transmit((uint8_t*)resp, len);
}

void handle_bw(cmd_params_t *params) {
    int8_t bw = 0;
    if (params->is_binary && params->len >= 1) {
        bw = (int8_t)params->data[0];
    } else if (params->data) {
        bw = (int8_t)atoi((char*)params->data);
    }
    NVS_Write((RFM95W_PARAM_BW), (uint32_t)bw);
    xTaskNotify(rfm95wTaskHandle, SETTINGS_CHANGE_EVENT_BIT, eSetBits);
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: BW set to %d\r\n", bw);
    USB_Transmit((uint8_t*)resp, len);
}

void handle_cr(cmd_params_t *params) {
    int8_t cr = 0;
    if (params->is_binary && params->len >= 1) {
        cr = (int8_t)params->data[0];
    } else if (params->data) {
        cr = (int8_t)atoi((char*)params->data);
    }
    NVS_Write((RFM95W_PARAM_CR), (uint32_t)cr);
    xTaskNotify(rfm95wTaskHandle, SETTINGS_CHANGE_EVENT_BIT, eSetBits);
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: CR set to %d\r\n", cr);
    USB_Transmit((uint8_t*)resp, len);
}

void handle_crc(cmd_params_t *params) {
    int8_t crc = 0;
    if (params->is_binary && params->len >= 1) {
        crc = (int8_t)params->data[0];
    } else if (params->data) {
        crc = (int8_t)atoi((char*)params->data);
    }
    NVS_Write((RFM95W_PARAM_CRC), (uint32_t)crc);
    xTaskNotify(rfm95wTaskHandle, SETTINGS_CHANGE_EVENT_BIT, eSetBits);
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: CRC set to %d\r\n", crc);
    USB_Transmit((uint8_t*)resp, len);
}

void handle_sync(cmd_params_t *params) {
    int8_t sync = 0;
    if (params->is_binary && params->len >= 1) {
        sync = (int8_t)params->data[0];
    } else if (params->data) {
        sync = (int8_t)atoi((char*)params->data);
    }
    NVS_Write((RFM95W_PARAM_CRC), (uint32_t)sync);
    xTaskNotify(rfm95wTaskHandle, SETTINGS_CHANGE_EVENT_BIT, eSetBits);
    char resp[64];
    int len = snprintf(resp, sizeof(resp), "OK: Power set to %d dBm\r\n", sync);
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
void handle_log_on(cmd_params_t *params) {
    (void)params;
    
    // 1. Aktywuj logger w RAM
    logger_enable(true); 
    logger_set_level(LOG_LEVEL_INFO);
    
    // 2. Zapisz stan do NVS, aby po resecie logi były włączone
    uint32_t muted = 0;
    nvs_set_log_muted(muted); 
    
    USB_Transmit((uint8_t*)"LOG: All logs ENABLED & Saved to NVS\r\n", 38);
}

void handle_log_off(cmd_params_t *params) {
    (void)params;
    
    // 1. Dezaktywuj logger w RAM
    logger_enable(false);
    logger_set_level(LOG_LEVEL_NONE);
    
    // 2. Zapisz stan do NVS, aby po resecie logi były wyłączone
    uint32_t muted = 1;
    nvs_set_log_muted(muted);
    
    USB_Transmit((uint8_t*)"LOG: All logs DISABLED & Saved to NVS\r\n", 39);
}

extern uint32_t current_log_mute_mask;

void handle_log_mute(cmd_params_t *params) {
    const char* tag_str = (char*)params->data;
    uint32_t bit = logger_get_bit_from_tag(tag_str);

    if (bit == 0) {
        USB_Transmit((uint8_t*)"ERR: Unknown TAG\r\n", 18);
        return;
    }
    current_log_mute_mask ^= bit;
    NVS_Write(PARAM_LOG_TAG_MUTE, current_log_mute_mask);

    char resp[64];
    bool muted = (current_log_mute_mask & bit);
    snprintf(resp, sizeof(resp), "LOG: %s is now %s\r\n", tag_str, muted ? "MUTED" : "ACTIVE");
    USB_Transmit((uint8_t*)resp, strlen(resp));
}

void handle_log_unmute(cmd_params_t *params) {
    (void)params;
    
    // 1. Czyścimy starą tablicę (dla świętego spokoju)
    logger_filter_clear();
    
    // 2. KLUCZ: Zerujemy maskę bitową
    current_log_mute_mask = 0;
    
    // 3. Zapisujemy do NVS, żeby po restarcie też było czysto
    NVS_Write(PARAM_LOG_TAG_MUTE, 0);
    
    USB_Transmit((uint8_t*)"LOG: All filters cleared (Unmuted all)\r\n", 40);
}