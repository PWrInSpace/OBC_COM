#ifndef CMD_INTERFACE_H
#define CMD_INTERFACE_H

#include <stdint.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

#define MAX_CMD_LEN  512

typedef enum {
    CMD_UNKNOWN = 0x00,
    CMD_HELP = 0x01,
    CMD_SX1280_FREQ = 0x02,
    CMD_SX1280_PWR = 0x03,
    CMD_RESET = 0x04,
    CMD_STATUS = 0x05,
    CMD_SX1280_SEND_DATA = 0x06,
    CMD_COUNT
} Command_t;

void process_command(char *rx_buf);
void handle_help(char *val);
void handle_freq(char *val);
void handle_power(char *val);
void handle_reset(char *val);
void handle_status(char *val);

#endif