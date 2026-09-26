/*
 * Organization: PWr in Space
 */
#ifndef LORA_COMMANDS_H
#define LORA_COMMANDS_H

#define LORA_DEV_ID_ALL 0x00
#define LORA_DEV_ID_ALL_SUDO 0x01
#define LORA_DEV_ID_TANWA 0x04
#define LORA_DEV_ID_TANWA_SUDO 0x05

typedef enum {
    CMD_STATE_CHANGE          = 0x00,
    CMD_ABORT                 = 0x01,
    CMD_HOLD_IN               = 0x02,
    CMD_HOLD_OUT              = 0x03,
    CMD_LORA_TRANSMIT_F       = 0x10,
    CMD_LORA_TRANSMIT_T       = 0x11,
    CMD_SEND_SETTINGS         = 0x15,
    CMD_SOFT_ARM              = 0x29,
    CMD_SOFT_DISARM           = 0x30,
    CMD_RESTART_WEIGHT        = 0x31,
    CMD_CALIBRATE_WEIGHT      = 0x34,
    CMD_TARE_WEIGHT           = 0x35,
    CMD_SET_CAL_FACTOR_WEIGHT = 0x36,
    CMD_SET_OFFSET_WEIGHT     = 0x37,
    CMD_N2O_FILL_OPEN         = 0x42,
    CMD_N2O_FILL_CLOSE        = 0x43,
    CMD_N2O_FILL_OPEN_TIME    = 0x44,
    CMD_N2O_DEPR_OPEN         = 0x45,
    CMD_N2O_DEPR_CLOSE        = 0x46,
    CMD_N2O_DEPR_OPEN_TIME    = 0x47,
    CMD_QD_N2O_UNPLUG         = 0x48,
    CMD_QD_N2O_STOP           = 0x49,
    CMD_QD_N2_UNPLUG          = 0x4A,
    CMD_QD_N2_STOP            = 0x4B,
    CMD_HEATING_TANK_START    = 0x4C,
    CMD_HEATING_TANK_STOP     = 0x4D,
    CMD_HEATING_VALVE_START   = 0x4E,
    CMD_HEATING_VALVE_STOP    = 0x4F,
    CMD_N2_FILL_OPEN          = 0x50,
    CMD_N2_FILL_CLOSE         = 0x51,
    CMD_N2_FILL_OPEN_TIME     = 0x52,
    CMD_N2_DEPR_OPEN          = 0x53,
    CMD_N2_DEPR_CLOSE         = 0x54,
    CMD_N2_DEPR_OPEN_TIME     = 0x55,
    CMD_FIRE                  = 0x60,
    CMD_VENT_OPEN             = 0x70,
    CMD_VENT_CLOSE            = 0x71,
    CMD_EXTERNAL_SWITCH_ON    = 0x72,
    CMD_EXTERNAL_SWITCH_OFF   = 0x73,
    CMD_RESET                 = 0x80,
    CMD_LORA_SYNC             = 0xBA,
} lora_command_t;

#endif /* LORA_COMMANDS_H */
