/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "stm32h5xx.h"
#include "stm32h5xx_hal_gpio.h"
#include "task.h"
#include "sx1280.h"
#include "main.h"
#include "logger.h"

#include <string.h>

#include "sx1280_task.h"
#include "lora_config.h"

#define TX_DONE_TIMEOUT_MS_DEFAULT 20

osThreadId_t sx1280TaskHandle = NULL;
const osThreadAttr_t sx1280Task_attributes = {
  .name = "sx1280Task",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 4096
};

static void parse_frame(uint8_t *buf, uint8_t size) {

    (void)buf;
    (void)size;
}

static bool wait_for_tx_done(SX1280_t *radio, uint32_t timeout_ms) {
  uint32_t t0 = HAL_GetTick();
  while (((SX1280GetIrqStatus(radio) & IRQ_TX_DONE) != IRQ_TX_DONE) &&
         ((HAL_GetTick() - t0) < timeout_ms)) {
    osDelay(1);
  }
  return ((SX1280GetIrqStatus(radio) & IRQ_TX_DONE) == IRQ_TX_DONE);
}

static void start_rx(SX1280_t *radio, uint32_t ceiling_ms) {
  SX1280ClearIrqStatus(radio, IRQ_RADIO_ALL);
  TickTime_t rx_time = { RADIO_TICK_SIZE_1000_US, (uint16_t)ceiling_ms };
  SX1280SetRx(radio, rx_time);
}

static void stop_radio_standby(SX1280_t *radio) {
  SX1280ClearIrqStatus(radio, IRQ_RADIO_ALL);
  SX1280SetStandby(radio, STDBY_RC);
}

static bool handle_rx_done_and_get_payload(SX1280_t *radio, uint8_t *out_buf, uint8_t *out_size) {
  if (out_buf == NULL || out_size == NULL) return false;
  uint8_t rx_size = 0;
  SX1280GetPayload(radio, out_buf, &rx_size, (uint8_t)255);
  SX1280ClearIrqStatus(radio, IRQ_RADIO_ALL);
  if (rx_size == 0) return false;
  *out_size = rx_size;
  return true;
}

static void clear_irqs_and_standby(SX1280_t *radio) {
  SX1280ClearIrqStatus(radio, IRQ_RADIO_ALL);
  SX1280SetStandby(radio, STDBY_RC);
}

static bool rx_wait_for_event(SX1280_t *radio, uint32_t ceiling_ms, uint8_t *out_buf, uint8_t *out_size) {

  uint32_t start = HAL_GetTick();
  if (out_buf == NULL || out_size == NULL) return false;
  *out_size = 0;

  for (;;) {
    uint16_t irq = SX1280GetIrqStatus(radio);
    if ((irq & IRQ_RX_DONE) == IRQ_RX_DONE) {
      if (handle_rx_done_and_get_payload(radio, out_buf, out_size)) return true;
      return false;
    }
    if ((irq & IRQ_RX_TX_TIMEOUT) == IRQ_RX_TX_TIMEOUT) {
      clear_irqs_and_standby(radio);
      return false;
    }
    if ((HAL_GetTick() - start) >= (ceiling_ms + 10)) {
      /* safety fallback */
      clear_irqs_and_standby(radio);
      return false;
    }
    osDelay(5);
  }
}

static void send_window(SX1280_t *radio, const uint8_t *payload, uint8_t payload_len, uint32_t window_ms) {
  SX1280ClearIrqStatus(radio, IRQ_RADIO_ALL);
  uint32_t start = HAL_GetTick();
  while ((HAL_GetTick() - start) < window_ms) {
    SX1280SendPayload(radio, (uint8_t*)payload, payload_len, RX_TX_SINGLE);
    /* wait for TX_DONE (short timeout) */
    wait_for_tx_done(radio, TX_DONE_TIMEOUT_MS_DEFAULT);
    SX1280ClearIrqStatus(radio, IRQ_RADIO_ALL);
    osDelay(1);
  }
  SX1280SetStandby(radio, STDBY_RC);
}

static void sx1280_recv_once_ceiling(SX1280_t *radio, uint32_t ceiling_ms) {
  uint8_t rx_buf[255];
  uint8_t rx_size = 0;

  /* Start RX using helper */
  start_rx(radio, ceiling_ms);

  /* Wait for event and fill buffer */
  if (rx_wait_for_event(radio, ceiling_ms, rx_buf, &rx_size)) {
    /* process received frame */
    if (rx_size > 0) {
      parse_frame(rx_buf, rx_size);
    }
  } else {
    /* no frame received or timeout - ensure radio is in standby */
    stop_radio_standby(radio);
  }
}

void sx1280TaskEntry(void *argument)
{

  //HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);
  /* USER CODE BEGIN sx1280Task */
  /* Mode selector: true = SEND, false = RECV */
  bool send_mode = false;
  LoRaDevs_t *lora_devs = get_lora_devs_instance();
  SX1280_t* sx1280_radio = lora_devs->sx1280;
  uint8_t tx_payload[] = { 0x01, 0x05, 'H', 'e', 'l', 'l', 'o' };

  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);
    if (send_mode) {
      //LOG_INFO("Sending frames...");
      send_window(sx1280_radio, tx_payload, (uint8_t)sizeof(tx_payload), SX1280_TX_TIMEOUT_MS);
    } else {
      //LOG_INFO("Waiting for frames...");
      sx1280_recv_once_ceiling(sx1280_radio, SX1280_RX_TIMEOUT_MS);
    }

    send_mode = !send_mode;
    osDelay(10);
  }
  /* USER CODE END sx1280Task */
}

void SX1280_task_init(void){
    HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);
    sx1280TaskHandle = osThreadNew(sx1280TaskEntry, NULL, &sx1280Task_attributes);
    if (sx1280TaskHandle == NULL) {
      HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);
      LOG_ERROR("SX1280 TASK ERROR!!");
      return;
    }
    //HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);
    return;
}