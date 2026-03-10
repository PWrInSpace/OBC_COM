/*
 * Author: Szymon Rzewuski & Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 27.01.2026
 */
#include "cmsis_os2.h"

#include "lora_config.h"
#include "rfm95w.h"
#include "sx1280.h"
#include "sx1280_hal_wrapper.h"
#include "rfm95w_wrapper.h"
#include "spi.h"
#include "spi_callbacks.h"
#include "usb_config.h"

SX1280_t sx1280_radio = {
    .ctx = &hspi1,
    .spi_read = sx1280_spi_read,
    .spi_write = sx1280_spi_write,
    .spi_read_dma = sx1280_spi_read_dma,
    .spi_write_dma = sx1280_spi_write_dma,
    .set_reset = sx1280_set_reset,
    .get_busy = sx1280_get_busy,
    .get_dio = { sx1280_get_dio1, sx1280_get_dio2, NULL,},
    .delay_ms = sx1280_delay_ms
};

RFM95_param_t rfm95w_param = {
    .frequency = 868000000,
    .power = 17,               // 17 dBm
    .LoRa_Rate = 7,            // SF7
    .LoRa_BW = RFM95_BW_125_kHz,              // 125 kHz
    .packetLength = 256,         // 0 dla
    .readBytes = 0,
    .last_pkt_RSSI=0,
    .last_pkt_SNR = 0
};

// 2. Główna struktura radia
rfm95_t rfm95w_radio = {
    ._spi_transmit = rfm95w_spi_transmit,
    ._gpio_set_level = rfm95w_gpio_set_level,
    ._delay = rfm95w_delay,
    .rst_gpio_num = RFM95W_RST_Pin,
    .cs_gpio_num = RFM95W_CS_Pin,
    .d0_gpio_num = RFM95W_DIO_Pin,
    .frequency = 868000000,
    .implicit_header = 0,
    .log = rfm95w_log,
    .param = &rfm95w_param
};

static LoRaDevs_t lora_devs_instance = {
  .sx1280 = &sx1280_radio,
  .rfm95w = &rfm95w_radio
};


void rfm95w_config_init_param(void) {
    LoRaDevs_t *lora_devs = get_lora_devs_instance();
    rfm95_t *radio = lora_devs->rfm95w;
    // 5. Wywołanie konfiguracji domyślnej bazującej na rfm95w_param
    if (rfm95_default_config_param(radio) == RFM95_OK) {
        if (radio->log) radio->log("RFM95W: Configured successfully with parameters.\r\n");
    }
}

LoRaDevs_t* get_lora_devs_instance(void) {
  return &lora_devs_instance;
}

void rfm95w_config_init(void) {
  rfm95_reset(&rfm95w_radio);
  rfm95_default_config(&rfm95w_radio);
}

static void sx1280_config_init(void) {
  SX1280Reset(&sx1280_radio);
  osDelay(10);
  SX1280Init(&sx1280_radio, NULL);
  osDelay(10);
  SX1280SetRegulatorMode(&sx1280_radio, USE_LDO);
  osDelay(10);
  SX1280SetDioIrqParams(&sx1280_radio, IRQ_RADIO_ALL, IRQ_RADIO_ALL, IRQ_RADIO_NONE, IRQ_RADIO_NONE);
  osDelay(10);
  SX1280SetTxParams(&sx1280_radio, 14, RADIO_RAMP_20_US);
  osDelay(10);
  SX1280SetPacketType(&sx1280_radio, PACKET_TYPE_LORA);
  osDelay(10);
  SX1280SetModulationParams(&sx1280_radio, &(ModulationParams_t){
    .PacketType = PACKET_TYPE_LORA,
    .Params.LoRa = {
      .SpreadingFactor = LORA_SF7,
      .Bandwidth = LORA_BW_0800,
      .CodingRate = LORA_CR_4_5
    }
  });
  osDelay(10);
  SX1280SetPacketParams(&sx1280_radio, &(PacketParams_t){
    .PacketType = PACKET_TYPE_LORA,
    .Params.LoRa = {
      .PreambleLength = 3,
      .HeaderType = LORA_PACKET_VARIABLE_LENGTH,
      .PayloadLength = 255,
      .CrcMode = LORA_CRC_ON,
    }
  });
  osDelay(10);
}

void rfm95_print_actual_settings(rfm95_t *rfm) {
    char buf[128];
    
    // 1. Read Registers
    uint8_t version   = rfm95_read_reg(rfm, 0x42);
    uint8_t opMode    = rfm95_read_reg(rfm, 0x01);
    uint8_t pa_config = rfm95_read_reg(rfm, 0x09);
    uint8_t mc1       = rfm95_read_reg(rfm, 0x1D);
    uint8_t mc2       = rfm95_read_reg(rfm, 0x1E);

    // 2. Power Decoding (Integer Math)
    uint8_t pa_select = (pa_config >> 7) & 0x01;
    uint8_t max_pwr_i = (pa_config >> 4) & 0x07;
    uint8_t out_pwr_i = (pa_config & 0x0F);

    // Pmax = 10.8 + 0.6 * MaxPower -> working in 1/10th dBs
    // 108 + (6 * max_pwr_i)
    int32_t p_max_x10 = 108 + (6 * max_pwr_i);
    int32_t p_out_x10 = 0;

    if (pa_select) {
        // PA_BOOST: Pout = 17 - (15 - OutputPower)
        // In tenths: 170 - (150 - (out_pwr_i * 10))
        p_out_x10 = 170 - (150 - (out_pwr_i * 10));
    } else {
        // RFO: Pout = Pmax - (15 - OutputPower)
        p_out_x10 = p_max_x10 - (150 - (out_pwr_i * 10));
    }

    // 3. Frequency Calculation
    uint32_t frf = ((uint32_t)rfm95_read_reg(rfm, 0x06) << 16) |
                   ((uint32_t)rfm95_read_reg(rfm, 0x07) << 8)  |
                   ((uint32_t)rfm95_read_reg(rfm, 0x08));
    uint32_t freq_hz = (uint32_t)((uint64_t)frf * 32000000 / 524288);

    // --- 4. Print Table ---
    USB_Transmit((uint8_t*)"\r\n+-----------------------+-----------------------+\r\n", 51);
    USB_Transmit((uint8_t*)"| RFM95 HARDWARE DEBUG  | VALUE                 |\r\n", 51);
    USB_Transmit((uint8_t*)"+-----------------------+-----------------------+\r\n", 51);

    sprintf(buf, "| Silicon Version       | 0x%02X (Expect 0x12)  |\r\n", version);
    USB_Transmit((uint8_t*)buf, strlen(buf));

    sprintf(buf, "| PA_BOOST Pin          | %-21s |\r\n", pa_select ? "ON" : "OFF (RFO Pin)");
    USB_Transmit((uint8_t*)buf, strlen(buf));

    // Splitting float into two ints: p_max_x10 / 10 and p_max_x10 % 10
    sprintf(buf, "| MaxPower (Pmax)       | %2ld.%1ld dBm (Val: %d) |\r\n", 
            p_max_x10 / 10, p_max_x10 % 10, max_pwr_i);
    USB_Transmit((uint8_t*)buf, strlen(buf));

    sprintf(buf, "| Calculated Pout       | %2ld.%1ld dBm            |\r\n", 
            p_out_x10 / 10, abs(p_out_x10 % 10));
    USB_Transmit((uint8_t*)buf, strlen(buf));

    sprintf(buf, "| Frequency             | %-18lu Hz |\r\n", freq_hz);
    USB_Transmit((uint8_t*)buf, strlen(buf));

    sprintf(buf, "| Spreading Factor      | SF%-17d |\r\n", (mc2 >> 4));
    USB_Transmit((uint8_t*)buf, strlen(buf));

    sprintf(buf, "| Bandwidth Index       | %-21d |\r\n", (mc1 >> 4));
    USB_Transmit((uint8_t*)buf, strlen(buf));

    // Raw Register Dump
    USB_Transmit((uint8_t*)"+-----------------------+-----------------------+\r\n", 51);
    USB_Transmit((uint8_t*)"| REGISTER ADDRESS      | HEX VALUE             |\r\n", 51);
    USB_Transmit((uint8_t*)"+-----------------------+-----------------------+\r\n", 51);
    
    sprintf(buf, "| RegOpMode (0x01)      | 0x%02X                |\r\n", opMode);
    USB_Transmit((uint8_t*)buf, strlen(buf));

    sprintf(buf, "| RegPaConfig (0x09)    | 0x%02X                |\r\n", pa_config);
    USB_Transmit((uint8_t*)buf, strlen(buf));

    USB_Transmit((uint8_t*)"+-----------------------+-----------------------+\r\n", 51);
}

void lora_config_init(void) {
    sx1280_config_init();
    rfm95w_config_init();
    sx1280_wrapper_init();
    rfm95w_wrapper_init();
}