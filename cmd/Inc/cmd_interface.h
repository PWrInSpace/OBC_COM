#ifndef CMD_INTERFACE_H
#define CMD_INTERFACE_H

#include <stdint.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"

#define MAX_CMD_LEN  512

typedef enum {
    CMD_UNKNOWN = 0x00,
    CMD_HELP = 0x01,
    CMD_SX1280_FREQ = 0x02,
    CMD_SX1280_PWR = 0x03,
    CMD_RESET = 0x04,
    CMD_STATUS = 0x05,
    CMD_SX1280_TX = 0x06,
    CMD_LORA_TX = 0x07,
    CMD_LOG_ON = 0x08,
    CMD_LOG_OFF = 0x09,
    CMD_LOG_MUTE = 0x0A,
    CMD_LOG_UNMUTE = 0x0B,
    CMD_SF = 0x0C,
    CMD_BW = 0x0D,
    CMD_CR = 0x0E,
    CMD_CRC = 0x0F,
    CMD_SYNC = 0x10,
    CMD_LORA_MODE = 0x11,
    CMD_COUNT
} Command_t;

typedef struct {
    uint8_t *data;    
    uint16_t len;     
    bool is_binary;   
} cmd_params_t;

typedef void (*cmd_handler_t)(cmd_params_t *params);

typedef struct {
    const char *name;
    Command_t id;
    cmd_handler_t handler;
    const char *help;
} CommandMap_t;

void process_command(uint8_t *rx_buf, uint16_t len);
void handle_help(cmd_params_t *params);
void handle_freq(cmd_params_t *params);
void handle_power(cmd_params_t *params);
void handle_reset(cmd_params_t *params);
void handle_status(cmd_params_t *params);
void handle_sx1280_tx(cmd_params_t *params);
void handle_lora_tx(cmd_params_t *params);
void handle_log_on(cmd_params_t *params);
void handle_log_off(cmd_params_t *params);
void handle_log_mute(cmd_params_t *params);
void handle_log_unmute(cmd_params_t *params);
void handle_sf(cmd_params_t *params);
void handle_bw(cmd_params_t *params);
void handle_cr(cmd_params_t *params);
void handle_crc(cmd_params_t *params);
void handle_sync(cmd_params_t *params);
void handle_lora_mode(cmd_params_t *params);


#define LORA_BUFF_SIZE 512
extern uint8_t LoraRxBuffer[LORA_BUFF_SIZE];
extern volatile uint16_t lora_cmd_len;



#endif