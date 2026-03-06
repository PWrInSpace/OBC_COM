// Copyright 2023 PWr in Space, Krzysztof Gliwiński
#include "rfm95w.h"

rfm95_err_t rfm95_init(rfm95_t *rfm95) {
  rfm95_err_t ret = RFM95_OK;

  /*
   * Perform hardware reset.
   */
  rfm95_reset(rfm95);

  /*
   * Check version.
   */
  uint8_t version;
  uint8_t i = 0;
  while (i++ < TIMEOUT_RESET) {
    version = rfm95_read_reg(rfm95, REG_VERSION);
    if (version == 0x12) break;
    rfm95->_delay(2);
  }
  assert(i <= TIMEOUT_RESET + 1);  // at the end of the loop above, the max
                                   // value i can reach is TIMEOUT_RESET + 1

  ret |= rfm95_default_config(rfm95);

  return ret;
}

rfm95_err_t rfm95_default_config(rfm95_t *rfm95) {
  rfm95_err_t ret = RFM95_OK;
  rfm95_sleep(rfm95);
  ret |= rfm95_write_reg(rfm95, REG_FIFO_RX_BASE_ADDR, 0);
  ret |= rfm95_write_reg(rfm95, REG_FIFO_TX_BASE_ADDR, 0);
  ret |= rfm95_write_reg(rfm95, REG_LNA, rfm95_read_reg(rfm95, REG_LNA) | 0x03);
  ret |= rfm95_write_reg(rfm95, REG_MODEM_CONFIG_3, 0x04);
  rfm95_set_tx_power(rfm95, 17);

  rfm95_idle(rfm95);
  return ret;
}

rfm95_err_t rfm95_write_reg(rfm95_t *rfm95, int16_t reg, int16_t val) {
  uint8_t out[2] = {0x80 | reg, val};
  uint8_t in[2];

    if (rfm95 == NULL) {
      return RFM95_WRITE_ERR;
    }

    return rfm95->_spi_transmit(in, out) == 1 ? RFM95_OK : RFM95_WRITE_ERR;
}

uint8_t rfm95_read_reg(rfm95_t *rfm95, int16_t reg) {
  uint8_t out[2] = {reg, 0xff};
  uint8_t in[2];

  if (rfm95 == NULL) {
    return 0x00;
  }

  rfm95->_spi_transmit(in, out);
  return in[1];
}

void rfm95_reset(rfm95_t *rfm95) {
  if (rfm95 == NULL) {
    return;
  }

  rfm95->_gpio_set_level(rfm95->rst_gpio_num, 0);
  rfm95->_delay(1);
  rfm95->_gpio_set_level(rfm95->rst_gpio_num, 1);
  rfm95->_delay(10);
}

rfm95_err_t rfm95_explicit_header_mode(rfm95_t *rfm95) {
  rfm95_err_t ret = RFM95_OK;
  rfm95->implicit_header = 0;
  ret |= rfm95_write_reg(rfm95, REG_MODEM_CONFIG_1,
                        rfm95_read_reg(rfm95, REG_MODEM_CONFIG_1) & 0xfe);
  return ret;
}

rfm95_err_t rfm95_implicit_header_mode(rfm95_t *rfm95, int16_t size) {
  rfm95_err_t ret = RFM95_OK;
  rfm95->implicit_header = 1;
  ret |= rfm95_write_reg(rfm95, REG_MODEM_CONFIG_1,
                        rfm95_read_reg(rfm95, REG_MODEM_CONFIG_1) | 0x01);
  ret |= rfm95_write_reg(rfm95, REG_PAYLOAD_LENGTH, size);
  return ret;
}

rfm95_err_t rfm95_idle(rfm95_t *rfm95) {
  rfm95_err_t ret = RFM95_OK;
  ret |= rfm95_write_reg(rfm95, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
  return ret;
}

rfm95_err_t rfm95_sleep(rfm95_t *rfm95) {
  rfm95_err_t ret = RFM95_OK;
  ret |= rfm95_write_reg(rfm95, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
  return ret;
}

rfm95_err_t rfm95_set_receive_mode(rfm95_t *rfm95) {
  rfm95_err_t ret = RFM95_OK;
  ret |= rfm95_write_reg(rfm95, REG_OP_MODE,
                        MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
  return ret;
}

rfm95_err_t rfm95_set_transmit_mode(rfm95_t *rfm95) {
  rfm95_err_t ret = RFM95_OK;
  ret |= rfm95_write_reg(rfm95, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
  return ret;
}

rfm95_err_t rfm95_set_tx_power(rfm95_t *rfm95, rfm95_tx_power_t level) {
  rfm95_err_t ret = RFM95_OK;
  // RF9x module uses PA_BOOST pin
  int16_t new_level = (int16_t)level;
  ret |= rfm95_write_reg(rfm95, REG_PA_CONFIG, PA_BOOST | (new_level - 2));
  return ret;
}

rfm95_err_t rfm95_set_frequency(rfm95_t *rfm95, int32_t frequency) {
  rfm95_err_t ret = RFM95_OK;
  rfm95->frequency = frequency;

  uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

  ret |= rfm95_write_reg(rfm95, REG_FRF_MSB, (uint8_t)(frf >> 16));
  ret |= rfm95_write_reg(rfm95, REG_FRF_MID, (uint8_t)(frf >> 8));
  ret |= rfm95_write_reg(rfm95, REG_FRF_LSB, (uint8_t)(frf >> 0));
  return ret;
}

int32_t rfm95_get_frequency(rfm95_t *rfm95) { return rfm95->frequency; }

rfm95_err_t rfm95_set_spreading_factor(rfm95_t *rfm95,
                                     rfm95_spreading_factor_t sf) {
  rfm95_err_t ret = RFM95_OK;
  if (sf == RFM95_SF_64_CoS) {
    ret |= rfm95_write_reg(rfm95, REG_DETECTION_THRESHOLD, 0x0c);
    ret |= rfm95_write_reg(rfm95, REG_DETECTION_OPTIMIZE, 0xc5);
  } else if (sf == RFM95_SF_4096_CoS) {
    ret |= rfm95_write_reg(rfm95, REG_DETECTION_OPTIMIZE, 0xc3);
    ret |= rfm95_write_reg(rfm95, REG_DETECTION_THRESHOLD, 0x0a);
  }

  ret |= rfm95_write_reg(
      rfm95, REG_MODEM_CONFIG_2,
      (rfm95_read_reg(rfm95, REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
  return ret;
}

rfm95_err_t rfm95_set_bandwidth(rfm95_t *rfm95, rfm95_bandwith_t sbw) {
  if (sbw < 0 || sbw > 9) {
    rfm95->log("ERROR: Setting bandwith unsuccessful: sbw out of range");
    return RFM95_CONFIG_ERR;
  }

  if (sbw >= 8) {
    int32_t freq = rfm95_get_frequency(rfm95);
    if (freq <= 169E6) {
      rfm95->log("INFO: In the set frequency set bandwith is not supported!");
      return RFM95_CONFIG_ERR;
    }
  }

  int16_t bw;
  rfm95_err_t ret = RFM95_OK;
  bw = (int16_t)sbw;

  ret |= rfm95_write_reg(
      rfm95, REG_MODEM_CONFIG_1,
      (rfm95_read_reg(rfm95, REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
  return ret;
}

rfm95_err_t rfm95_set_coding_rate(rfm95_t *rfm95, int16_t denominator) {
  rfm95_err_t ret = RFM95_OK;
  if (denominator < 5)
    denominator = 5;
  else if (denominator > 8)
    denominator = 8;

  int16_t cr = denominator - 4;
  ret |= rfm95_write_reg(
      rfm95, REG_MODEM_CONFIG_1,
      (rfm95_read_reg(rfm95, REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
  return ret;
}

rfm95_err_t rfm95_set_preamble_length(rfm95_t *rfm95, int32_t length) {
  rfm95_err_t ret = RFM95_OK;
  ret |= rfm95_write_reg(rfm95, REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
  ret |= rfm95_write_reg(rfm95, REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
  return ret;
}

rfm95_err_t rfm95_set_sync_word(rfm95_t *rfm95, int16_t sw) {
  return rfm95_write_reg(rfm95, REG_SYNC_WORD, sw);
}

rfm95_err_t rfm95_enable_crc(rfm95_t *rfm95) {
  return rfm95_write_reg(rfm95, REG_MODEM_CONFIG_2,
                        rfm95_read_reg(rfm95, REG_MODEM_CONFIG_2) | 0x04);
}

rfm95_err_t rfm95_disable_crc(rfm95_t *rfm95) {
  return rfm95_write_reg(rfm95, REG_MODEM_CONFIG_2,
                        rfm95_read_reg(rfm95, REG_MODEM_CONFIG_2) & 0xfb);
}

rfm95_err_t rfm95_fill_fifo_buf_to_send(rfm95_t *rfm95, uint8_t *buf,
                                      int16_t size) {
  rfm95_err_t ret = RFM95_OK;
  /*
   * Transfer data to radio.
   */
  ret |= rfm95_idle(rfm95);
  ret |= rfm95_write_reg(rfm95, REG_FIFO_ADDR_PTR, 0);

  for (int16_t i = 0; i < size; i++) {
    ret |= rfm95_write_reg(rfm95, REG_FIFO, *buf++);
  }
  ret |= rfm95_write_reg(rfm95, REG_PAYLOAD_LENGTH, size);
  return ret;
}

rfm95_err_t rfm95_start_transmission(rfm95_t *rfm95) {
  /*
   * Start transmission and wait for conclusion.
   */
  return rfm95_write_reg(rfm95, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
}

bool rfm95_check_tx_done(rfm95_t *rfm95) {
  return (rfm95_read_reg(rfm95, REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) != 0x00;
}

rfm95_err_t rfm95_write_irq_flags(rfm95_t *rfm95) {
  return rfm95_write_reg(rfm95, REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
}

rfm95_err_t rfm95_send_packet(rfm95_t *rfm95, uint8_t *buf, int16_t size) {

  rfm95_err_t ret = RFM95_OK;
  ret |= rfm95_fill_fifo_buf_to_send(rfm95, buf, size);
  ret |= rfm95_start_transmission(rfm95);

  while (!rfm95_check_tx_done(rfm95)) {
    int8_t read_reg = rfm95_read_reg(rfm95,REG_IRQ_FLAGS);
    rfm95->_delay(2);
  }

  ret |= rfm95_write_irq_flags(rfm95);
  return ret == RFM95_OK ? RFM95_OK : RFM95_TRANSMIT_ERR;
}

int16_t rfm95_receive_packet(rfm95_t *rfm95, uint8_t *buf, int16_t size) {
  int16_t len = 0;

  /*
   * Check interrupts.
   */
  int16_t irq = rfm95_read_reg(rfm95, REG_IRQ_FLAGS);
  rfm95_write_reg(rfm95, REG_IRQ_FLAGS, irq);
  if ((irq & IRQ_RX_DONE_MASK) == 0) return 0;
  if (irq & IRQ_PAYLOAD_CRC_ERROR_MASK) return 0;

  /*
   * Find packet size.
   */
  if (rfm95->implicit_header) {
    len = rfm95_read_reg(rfm95, REG_PAYLOAD_LENGTH);
  } else {
    len = rfm95_read_reg(rfm95, REG_RX_NB_BYTES);
  }

  /*
   * Transfer data from radio.
   */
  rfm95_idle(rfm95);
  rfm95_write_reg(rfm95, REG_FIFO_ADDR_PTR,
                 rfm95_read_reg(rfm95, REG_FIFO_RX_CURRENT_ADDR));
  if (len > size) {
    len = size;
  }
  for (int16_t i = 0; i < len; i++) {
    buf[i] = rfm95_read_reg(rfm95, REG_FIFO);
  }

  return len;
}

rfm95_err_t rfm95_received(rfm95_t *rfm95) {
  if (rfm95 == NULL) {
    return RFM95_RECEIVE_ERR;
  }

  if (rfm95_read_reg(rfm95, REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK) {
    return RFM95_OK;
  }

  if (rfm95->log) rfm95->log("ERROR: No packet received");
  return RFM95_OK;
}

int16_t rfm95_packet_rssi(rfm95_t *rfm95) {
  return (rfm95_read_reg(rfm95, REG_PKT_RSSI_VALUE) -
          (rfm95->frequency < 868E6 ? 164 : 157));
}

float rfm95_packet_snr(rfm95_t *rfm95) {
  return ((int8_t)rfm95_read_reg(rfm95, REG_PKT_SNR_VALUE)) * 0.25;
}

rfm95_err_t rfm95_map_d0_interrupt(rfm95_t *rfm95, rfm95_dio0_mapping_t mode) {
  rfm95_err_t ret = rfm95_write_reg(rfm95, REG_DIO_MAPPING_1,(mode << 6));
  if(ret != RFM95_OK) {
    rfm95->log("ERROR: Failed to map DIO0 interrupt");
    return ret;
  }
  rfm95->_delay(2);  // wait for the register to be updated
  return RFM95_OK;
}

void rfm95_close(rfm95_t *rfm95) {
  rfm95_sleep(rfm95);
  //   close(__spi);  FIXME: end hardware features after rfm95_close
  //   close(__cs);
  //   close(__rst);
  //   __spi = -1;
  //   __cs = -1;
  //   __rst = -1;
}

// void rfm95_dump_registers(rfm95_t *rfm95) {
//   int16_t i;
//   printf("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
//   for (i = 0; i < 0x40; i++) {
//     printf("%02X ", rfm95_read_reg(rfm95, i));
//     if ((i & 0x0f) == 0x0f) printf("\n");
//   }
//   printf("\n");
// }