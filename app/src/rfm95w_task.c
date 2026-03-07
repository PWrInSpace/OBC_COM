/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "projdefs.h"
#include "stm32h5xx.h"
#include "stm32h5xx_hal_gpio.h"
#include "task.h"
#include "rfm95w.h"
#include "main.h"
#include "logger.h"
#include "usb_config.h"

#include <stdint.h>
#include <string.h>

#include "lora_config.h"
#include "usbd_cdc_if.h"

#define TX_DONE_TIMEOUT_MS_DEFAULT 20

osThreadId_t rfm95wTaskHandle = NULL;
const osThreadAttr_t rfm95wTask_attributes = {
  .name = "rfm95wTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 2048
};

static bool wait_for_tx_done(rfm95_t *radio, uint32_t timeout_ms) {
  uint32_t t0 = HAL_GetTick();
  while (rfm95_check_tx_done(radio) &&
         ((HAL_GetTick() - t0) < timeout_ms)) {
    osDelay(1);
  }
  return (rfm95_check_tx_done(radio));
}

void rfm95_start_rx(rfm95_t *radio, uint32_t ceiling_ms) {
  rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);
  rfm95_set_receive_mode(radio);
}

static void stop_radio_standby(rfm95_t *radio) {
  rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);
  rfm95_idle(radio);
}

static bool handle_rx_done_and_get_payload(rfm95_t *radio, uint8_t *out_buf, uint8_t *out_size) {
  if (out_buf == NULL || out_size == NULL) return false;
  uint8_t rx_size = 0;
  rx_size = rfm95_receive_packet(radio, out_buf, 255);
  rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);
  if (rx_size == 0) return false;
  *out_size = rx_size;
  return true;
}

void rfm95_clear_irqs_and_standby(rfm95_t *radio) {
    rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);
    rfm95_idle(radio);
}

static bool rx_wait_for_event(rfm95_t *radio, uint32_t ceiling_ms, uint8_t *out_buf, uint8_t *out_size) {

  uint32_t start = HAL_GetTick();
  if (out_buf == NULL || out_size == NULL) return false;
  *out_size = 0;

  for (;;) {
    uint16_t irq = rfm95_read_reg(radio, REG_IRQ_FLAGS);
    if ((irq & IRQ_RX_DONE_MASK) == IRQ_RX_DONE_MASK) {
      if (handle_rx_done_and_get_payload(radio, out_buf, out_size)) return true;
      return false;
    }
    if ((irq & IRQ_RX_TIMEOUT_MASK) == IRQ_RX_TIMEOUT_MASK) {
      rfm95_clear_irqs_and_standby(radio);
      return false;
    }
    if ((HAL_GetTick() - start) >= (ceiling_ms + 10)) {
      /* safety fallback */
      rfm95_clear_irqs_and_standby(radio);
      return false;
    }
    osDelay(5);
  }
}

void rfm95_send_window(rfm95_t *radio, const uint8_t *payload, uint8_t payload_len, uint32_t window_ms) {
  rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);
  uint32_t start = HAL_GetTick();
  rfm95_set_transmit_mode(radio);
  while ((HAL_GetTick() - start) < window_ms) {
    rfm95_send_packet(radio, payload, payload_len);
    /* wait for TX_DONE (short timeout) */
    wait_for_tx_done(radio, TX_DONE_TIMEOUT_MS_DEFAULT);
    rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);
    osDelay(200);
  }
  rfm95_idle(radio);
}

void recv_once_ceiling(rfm95_t *radio, uint32_t ceiling_ms, uint8_t *out_buf, uint8_t *out_size) {

  /* Start RX using helper */
  rfm95_start_rx(radio, ceiling_ms);

  /* Wait for event and fill buffer */
  if (rx_wait_for_event(radio, ceiling_ms, out_buf, out_size)) {
    /* process received frame */
    if (*out_size > 0) {
        LOG_INFO("Received frame: size=%d, data=", *out_size);
        for (uint8_t i = 0; i < *out_size; i++) {
            LOG_INFO("%02X ", out_buf[i]);
        }
        LOG_INFO("\n");
    }
  } else {
    /* no frame received or timeout - ensure radio is in standby */
    stop_radio_standby(radio);
  }
}


/*
void rfm95wTaskEntry(void *argument)
{
*/

  /*HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);*/
  /* USER CODE BEGIN rfm95wTask */
  /* Mode selector: true = SEND, false = RECV */
  /*
   bool send_mode = false;
   LoRaDevs_t *lora_devs = get_lora_devs_instance();
   rfm95_t *rfm95_radio = lora_devs->rfm95w;
   uint8_t tx_payload[] = { 0x01, 0x05, 'H', 'e', 'l', 'l', 'o' };
   uint8_t rx_buf[255];
   uint8_t rx_size = 0;

   */

   /*
   uint8_t msg[] = "RFM95W WYSTARTOWAL\r\n";
   CDC_Transmit_FS(msg, strlen((char*)msg));
   uint8_t msg2[] = "RFM95W RUNNING!\r\n";

  for(;;)
  {
    
    if (send_mode) {
      //LOG_INFO("Sending frames...");
      rfm95_send_window(rfm95_radio, tx_payload, (uint8_t)sizeof(tx_payload), RFM95W_TX_TIMEOUT_MS);
    } else {
      //LOG_INFO("Waiting for frames...");
      recv_once_ceiling(rfm95_radio, RFM95W_RX_TIMEOUT_MS, rx_buf, &rx_size);
      if (rx_size > 0) {
        CDC_Transmit_FS(rx_buf, rx_size);
      }
    }
      

    send_mode = !send_mode;
    
    USB_Transmit(msg2, strlen((char*)msg2));
    osDelay(500);
  }
  */

//TESTOWY TASK DO ODCZYTANIA ID I SPRAWDZENIA SPI

void rfm95wTaskEntry(void *argument)
{
    uint8_t tx_buf[2] = {0x42 & 0x7F, 0x00}; // Adres rejestru 0x42 (Read) + dummy byte
    uint8_t rx_buf[2] = {0x00, 0x00};
    char debug_msg[64];

    HAL_GPIO_WritePin(RFM95W_CS_GPIO_Port, RFM95W_CS_Pin, GPIO_PIN_RESET);
  
    HAL_StatusTypeDef spi_status = HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, 2, 100); //NA CHAMA WALNIETE ZEBY POTEM LIBKE OGARNAC
    
    HAL_GPIO_WritePin(RFM95W_CS_GPIO_Port, RFM95W_CS_Pin, GPIO_PIN_SET);

    if (spi_status == HAL_OK) {
        sprintf(debug_msg, "SPI OK! Reg 0x42: 0x%02X (Expected 0x12)\r\n", rx_buf[1]);
    } else {
        sprintf(debug_msg, "SPI ERROR! HAL Status: %d\r\n", spi_status);
    }

    for(;;)
    {
        USB_Transmit((uint8_t*)debug_msg, strlen(debug_msg));
        
        HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
        osDelay(1000);
    }
}

  /* USER CODE END rfm95wTask */

void RFM95W_task_init(void){
  uint8_t msg2[] = "ZA MAŁO PAMIECI!\r\n";
    //HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_4);
    rfm95wTaskHandle = osThreadNew(rfm95wTaskEntry, NULL, &rfm95wTask_attributes);
    if (rfm95wTaskHandle == NULL) {
      osDelay(pdMS_TO_TICKS(3000));
      USB_Transmit(msg2, strlen((char*)msg2));
      //HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_4);
      //LOG_ERROR("RFM95W TASK ERROR!!");
      return;
    }
    //HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);
    return;
}