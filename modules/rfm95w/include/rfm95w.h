// Copyright 2023 PWr in Space, Krzysztof Gliwiński
#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/*
 * IRQ masks
 */
#define IRQ_TX_DONE_MASK 0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK 0x40
#define IRQ_RX_TIMEOUT_MASK 0x80
#define IRQ_ALL 0xFF

#define PA_OUTPUT_RFO_PIN 0
#define PA_OUTPUT_PA_BOOST_PIN 1

#define TIMEOUT_RESET 100

/*
 * Register definitions
 */
#define REG_FIFO 0x00
#define REG_OP_MODE 0x01
#define REG_FRF_MSB 0x06
#define REG_FRF_MID 0x07
#define REG_FRF_LSB 0x08
#define REG_PA_CONFIG 0x09
#define REG_LNA 0x0c
#define REG_FIFO_ADDR_PTR 0x0d
#define REG_FIFO_TX_BASE_ADDR 0x0e
#define REG_FIFO_RX_BASE_ADDR 0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS 0x12
#define REG_RX_NB_BYTES 0x13
#define REG_PKT_SNR_VALUE 0x19
#define REG_PKT_RSSI_VALUE 0x1a
#define REG_MODEM_CONFIG_1 0x1d
#define REG_MODEM_CONFIG_2 0x1e
#define REG_PREAMBLE_MSB 0x20
#define REG_PREAMBLE_LSB 0x21
#define REG_PAYLOAD_LENGTH 0x22
#define REG_MODEM_CONFIG_3 0x26
#define REG_RSSI_WIDEBAND 0x2c
#define REG_DETECTION_OPTIMIZE 0x31
#define REG_DETECTION_THRESHOLD 0x37
#define REG_SYNC_WORD 0x39
#define REG_DIO_MAPPING_1 0x40
#define REG_VERSION 0x42

/*
 * Transceiver modes
 */
#define MODE_LONG_RANGE_MODE 0x80
#define MODE_SLEEP 0x00
#define MODE_STDBY 0x01
#define MODE_TX 0x03
#define MODE_RX_CONTINUOUS 0x05
#define MODE_RX_SINGLE 0x06

/*
 * PA configuration
 */
#define PA_BOOST 0x80

/*!
  \brief Rfm95 functions return values enum
*/
typedef enum {
  RFM95_OK = 0,
  RFM95_INIT_ERR,
  RFM95_WRITE_ERR,
  RFM95_TRANSMIT_ERR,
  RFM95_RECEIVE_ERR,
  RFM95_CONFIG_ERR
} rfm95_err_t;

typedef enum {
  RFM95_GPIO_MODE_DISABLE = 0,
  RFM95_GPIO_MODE_INPUT,
  RFM95_GPIO_MODE_OUTPUT
} rfm95_gpio_mode_t;


typedef enum {
  RFM95_IRQ_D0_RXDONE = 0x00,
  RFM95_IRQ_D0_TXDONE = 0x01,
  RFM95_IRQ_D0_CADDONE = 0x10,
} rfm95_dio0_mapping_t;

/*!
 * \brief Enum for Rfm95 bandwith in Hz
 */
typedef enum {
  RFM95_BW_7_8_kHz = 0,
  RFM95_BW_10_4_kHz,
  RFM95_BW_15_6_kHz,
  RFM95_BW_20_8_kHz,
  RFM95_BW_31_25_kHz,
  RFM95_BW_41_7_kHz,
  RFM95_BW_62_5_kHz,
  RFM95_BW_125_kHz,
  RFM95_BW_250_kHz,
  RFM95_BW_500_kHz,
} rfm95_bandwith_t;

/*!
 * \brief Enum for Rfm95 spreading factor in chips / symbol
 *        Used in the rfm95_set_spreading_factor method
 */
typedef enum {
  RFM95_SF_64_CoS = 6,
  RFM95_SF_128_CoS,
  RFM95_SF_256_CoS,
  RFM95_SF_512_CoS,
  RFM95_SF_1024_CoS,
  RFM95_SF_2048_CoS,
  RFM95_SF_4096_CoS
} rfm95_spreading_factor_t;

/*!
 * \brief Enum for Rfm95 TX Power
 */
// TODO(Glibus): check if it is ok
typedef enum {
  RFM95_TX_POWER_14_dBm = 2,
  RFM95_TX_POWER_20_dBm = 17
} rfm95_tx_power_t;

typedef bool (*rfm95_SPI_transmit)(uint8_t *in, uint8_t *out);
typedef void (*rfm95_delay)(uint32_t _ms);
typedef bool (*rfm95_GPIO_set_level)(uint16_t _gpio_num, uint8_t _level);
typedef void (*rfm95_log)(const char *info);

typedef struct {
  rfm95_SPI_transmit _spi_transmit;
  rfm95_delay _delay;
  rfm95_GPIO_set_level _gpio_set_level;
  rfm95_log log;
  uint16_t rst_gpio_num;
  uint16_t cs_gpio_num;
  uint16_t d0_gpio_num;
  int16_t implicit_header;
  int32_t frequency;
} rfm95_t;

/*!
 * \brief Perform hardware initialization.
 */
rfm95_err_t rfm95_init(rfm95_t *rfm95);

/*!
 * \brief Create default config and set parameters
 */
rfm95_err_t rfm95_default_config(rfm95_t *rfm95);

/*!
 * \brief Write a value to a register.
 * \param reg Register index.
 * \param val Value to write.
 * \return rfm95_err_t value
 */
rfm95_err_t rfm95_write_reg(rfm95_t *rfm95, int16_t reg, int16_t val);

/*!
 * \brief Read the current value of a register.
 * \param reg Register index.
 * \returns Value of the register.
 */
uint8_t rfm95_read_reg(rfm95_t *rfm95, int16_t reg);

/*!
 * \brief Perform physical reset on the Rfm95 chip
 * \throw Assert if _gpio_set_level fails
 */
void rfm95_reset(rfm95_t *rfm95);

/*!
 * \brief Configure explicit header mode.
 * Packet size will be included in the frame.
 */
rfm95_err_t rfm95_explicit_header_mode(rfm95_t *rfm95);

/*!
 * \brief Configure implicit header mode.
 * All packets will have a predefined size.
 * \param size Size of the packets.
 */
rfm95_err_t rfm95_implicit_header_mode(rfm95_t *rfm95, int16_t size);

/*!
 * \brief Sets the radio transceiver in idle mode.
 * \note Must be used to change registers and access the FIFO.
 */
rfm95_err_t rfm95_idle(rfm95_t *rfm95);

/*!
 * \brief Sets the radio transceiver in sleep mode.
 * \note Low power consumption and FIFO is lost.
 */
rfm95_err_t rfm95_sleep(rfm95_t *rfm95);

/*!
 * \brief Sets the radio transceiver in receive mode.
 * \note Incoming packets will be received.
 */
rfm95_err_t rfm95_set_receive_mode(rfm95_t *rfm95);

/*!
 * \brief Sets the radio transceiver in transmit mode.
 * \note Outgoing packets will be sent.
 */
rfm95_err_t rfm95_set_transmit_mode(rfm95_t *rfm95);

/*!
 * \brief Configure power level for transmission
 * \param level 2 or 17, from least to most power
 */
rfm95_err_t rfm95_set_tx_power(rfm95_t *rfm95, rfm95_tx_power_t level);

/*!
 * \brief Set carrier frequency.
 * \param frequency Frequency in Hz
 */
rfm95_err_t rfm95_set_frequency(rfm95_t *rfm95, int32_t frequency);

/*!
 * \brief Get the frequency set on Rfm95
 * \returns Rfm95 frequency in Hz
 */
int32_t rfm95_get_frequency(rfm95_t *rfm95);

/*!
 * \brief Set spreading factor.
 * \param sf 6-12, Spreading factor to use.
 * \returns RFM95_OK if operation successful, RFM95_CONFIG_ERR otherwise
 */
rfm95_err_t rfm95_set_spreading_factor(rfm95_t *rfm95,
                                     rfm95_spreading_factor_t sf);

/*!
 * \brief Set bandwidth (bit rate)
 * \param sbw Bandwidth in Hz (up to 500000)
 * \note When using low frequency (below 169 MHz), only sf up to
 *       125 kHz is supported
 */
rfm95_err_t rfm95_set_bandwidth(rfm95_t *rfm95, rfm95_bandwith_t sbw);

/*!
 * \brief Set coding rate
 * \param denominator 5-8, Denominator for the coding rate 4/x
 */
rfm95_err_t rfm95_set_coding_rate(rfm95_t *rfm95, int16_t denominator);

/*!
 * \brief Set the size of preamble.
 * \param length Preamble length in symbols.
 */
rfm95_err_t rfm95_set_preamble_length(rfm95_t *rfm95, int32_t length);

/*!
 * \brief Change radio sync word.
 * \param sw New sync word to use.
 */
rfm95_err_t rfm95_set_sync_word(rfm95_t *rfm95, int16_t sw);

/*!
 * \brief Enable appending/verifying packet CRC.
 */
rfm95_err_t rfm95_enable_crc(rfm95_t *rfm95);

/*!
 * \brief Disable appending/verifying packet CRC.
 */
rfm95_err_t rfm95_disable_crc(rfm95_t *rfm95);

/*!
 * \brief Fills the REG_FIFO buffer with desired values, and the
 * REG_PAYLOAD_LENGTH with buffer size
 * \param buf - array of 8bit values
 * \param size - sizeof(buf)
 * \returns RFM95_OK if writing to buffers is ok,
 * RFM95_WRITE_ERR otherwise
 */
rfm95_err_t rfm95_fill_fifo_buf_to_send(rfm95_t *rfm95, uint8_t *buf,
                                      int16_t size);

/*!
 * \brief Writes the REG_OP_MODE to mode TX
 * \returns RFM95_OK if all goes good, RFM95_WRITE_ERR otherwise
 */
rfm95_err_t rfm95_start_transmission(rfm95_t *rfm95);

/*!
 * \brief Checks whether the transmission has finished
 * \returns True if finished, false otherwise
 */
bool rfm95_check_tx_done(rfm95_t *rfm95);

/*!
 * \brief Writes the REG_IRQ_FLAGS buffer with IRQ_TX_DONE_MASK
 * \returns RFM95_OK :D - RFM95_WRITE_ERR :C
 */
rfm95_err_t rfm95_write_irq_flags(rfm95_t *rfm95);

/*!
 * \brief Send a packet. DOES NOT go into receive mode automatically afterwards.
 * \param buf Data to be sent
 * \param size Size of data.
 */
rfm95_err_t rfm95_send_packet(rfm95_t *rfm95, uint8_t *buf, int16_t size);

/*!
 * \brief Read a received packet.
 * \param buf Buffer for the data.
 * \param size Available size in buffer (bytes).
 * \returns Number of bytes received (zero if no packet available).
 */
int16_t rfm95_receive_packet(rfm95_t *rfm95, uint8_t *buf, int16_t size);

/*!
 * \returns non-zero if there is data to read (packet received).
 */
rfm95_err_t rfm95_received(rfm95_t *rfm95);

/*!
 * \returns last packet's RSSI.
 */
int16_t rfm95_packet_rssi(rfm95_t *rfm95);

/*!
 * \returns last packet's SNR (signal to noise ratio).
 */
float rfm95_packet_snr(rfm95_t *rfm95);

rfm95_err_t rfm95_map_d0_interrupt(rfm95_t *rfm95, rfm95_dio0_mapping_t mode);

/*!
 * \brief Shutdown hardware.
 */
void rfm95_close(rfm95_t *rfm95);

/// \brief Not supported
int16_t rfm95_initialized(rfm95_t *rfm95);

/// \brief Dump registers :D
void rfm95_dump_registers(rfm95_t *rfm95);