#ifndef CMD_INTERFACE_H
#define CMD_INTERFACE_H

#include <stdint.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

#define MAX_CMD_LEN  128

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_HELP,
    CMD_SX1280_FREQ,
    CMD_SX1280_PWR,
    CMD_RESET,
    CMD_STATUS,
    CMD_COUNT // Pomocne do iteracji
} Command_t;

void process_command(char *rx_buf);

#endif