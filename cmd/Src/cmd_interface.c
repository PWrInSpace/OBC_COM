#include "cmd_interface.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include"main.h"
#include "usb_config.h"

//! TO IMPLEMENT: ECSS STANDARD and DUAL PARSER MCB <-> OBC-COM and OBC-COM <-> HUMAN INTERFACE for efficiency
/*
header Length Command Data CRC EOF
 0x32   0x05   0x17   0x01     0x89 
*/
typedef void (*cmd_handler_t)(char *val_str);


typedef struct {
    const char *name;
    Command_t id;
    cmd_handler_t handler;
    const char *help; 
} CommandMap_t;

static const CommandMap_t cmd_map[] = {
    {"help",      CMD_HELP,      handle_help,   "- Show menu"},
    {"freq",      CMD_SX1280_FREQ, handle_freq, ":Hz - Set freq"},
    {"reset",     CMD_RESET,     handle_reset,  "- System reboot"},
};

const size_t cmd_map_size = sizeof(cmd_map) / sizeof(CommandMap_t);


void process_command(char *rx_buf) {
    char *prefix  = strtok(rx_buf, ":");
    char *cmd_str = strtok(NULL, ":");
    char *val_str = strtok(NULL, ":");

    if (prefix == NULL || strcmp(prefix, "CMD") != 0 || cmd_str == NULL) {
        USB_Transmit((uint8_t*)"ERROR: Invalid format. Use CMD:name:val\r\n", 41);
        return;
    }

    Command_t found_id = CMD_UNKNOWN;
    
    char *endptr;
    uint32_t parsed_id = strtoul(cmd_str, &endptr, 0);

    if (*endptr == '\0' && cmd_str[0] != '\0') {
        found_id = (Command_t)parsed_id;
    }

    for (size_t i = 0; i < cmd_map_size; i++) {
        if ((found_id != CMD_UNKNOWN && cmd_map[i].id == found_id) || 
            (strcmp(cmd_str, cmd_map[i].name) == 0)) {
            
            cmd_map[i].handler(val_str);
            return;
        }
    }

    char err[64];
    int len = snprintf(err, sizeof(err), "ERROR: Unknown command [%s]\r\n", cmd_str);
    USB_Transmit((uint8_t*)err, len);
}

void handle_help(char *val) {
    char help_line[80];
    const char *header = "\r\n--- OBC COMMAND HELP ---\r\n";
    USB_Transmit((uint8_t*)header, strlen(header));

    extern const CommandMap_t cmd_map[];
    extern const size_t cmd_map_size;

    for (size_t i = 0; i < cmd_map_size; i++) {
        int len = snprintf(help_line, sizeof(help_line), "CMD:%s %s\r\n", 
                           cmd_map[i].name, cmd_map[i].help);
        USB_Transmit((uint8_t*)help_line, len);
    }
}

void handle_freq(char *val) {
    if (val == NULL) return;
    uint32_t freq = strtoul(val, NULL, 10);
    char resp[64];
    snprintf(resp, sizeof(resp), "OK: Freq set to %lu Hz\r\n", freq);
    USB_Transmit((uint8_t*)resp, strlen(resp));
}

void handle_power(char *val) {
    if (val == NULL) return;
    int8_t pwr = (int8_t)atoi(val);
    char resp[64];
    snprintf(resp, sizeof(resp), "OK: Power set to %d dBm\r\n", pwr);
    USB_Transmit((uint8_t*)resp, strlen(resp));
}

void handle_reset(char *val) {
    USB_Transmit((uint8_t*)"SYSTEM: Resetting...\r\n", 22);
    vTaskDelay(pdMS_TO_TICKS(100));
    NVIC_SystemReset();
}

void handle_status(char *val) {
    const char *msg = "STATUS: Radio Link OK\r\n";
    USB_Transmit((uint8_t*)msg, strlen(msg));
}

void handle_sx1280_tx(char *val) {
    if (val == NULL) return;
    //!ADD LORA_TX_QUEUE HERE with buffer pool to send data to sx1280 task and notify it about it via eBits
    char resp[64];
    snprintf(resp, sizeof(resp), "SENDING DATA SX1280\r\n");
    USB_Transmit((uint8_t*)resp, strlen(resp));
}