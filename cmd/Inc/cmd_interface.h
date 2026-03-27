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



#endif