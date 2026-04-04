/*
 * Author: Szymon Rzewuski, Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "projdefs.h"
#include "stm32h5xx.h"
#include "stm32h5xx_hal_gpio.h"
#include "task.h"
#include "sx1280.h"
#include "main.h"
#include "logger.h"
#include "usb_config.h"
#include"usbd_cdc_if.h"
#include "usbd_cdc.h"
#include <string.h>
#include "timers.h"

#include "sx1280_task.h"
#include "lora_config.h"
#include"stdbool.h"

#define TX_DONE_TIMEOUT_MS_DEFAULT 20

extern volatile uint16_t USB_Rx_Data_Len; 
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

//#define USB_FLAG   //!< USB COMMUNICATION uncomment flag to use WARNING: this might corrupt the OBC timings dont use in flight setup
#define GroundStationFlag 1 //!< Ground Station Setup // uncomment to use module as ground station
//! Later on add runtime variables to modify the state or setup
TimerHandle_t xTelemetryTimer;
#define TIMER_EVENT_BIT  ( 1 << 1 ) // Bit 1 timera
#define RADIO_EVENT_BIT  ( 1 << 0 ) // Bit 0 EXTI (DIO)
#define USB_EVENT_BIT    ( 1 << 2 ) // Bit 2 USB Data


osThreadId_t sx1280TaskHandle = NULL;
const osThreadAttr_t sx1280Task_attributes = {
  .name = "sx1280Task",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 1024
};

void vTelemetryTimerCallback(TimerHandle_t xTimer) {
    xTaskNotify(sx1280TaskHandle, TIMER_EVENT_BIT, eSetBits);
}

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
  SX1280GetPayload(radio, out_buf, &rx_size, (uint8_t)512);
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

void verify_freq(SX1280_t *sx1280_radio)
{
  // --- SEKCJA KONFIGURACJI (RAZ) ---
SX1280Reset(sx1280_radio);
SX1280SetStandby(sx1280_radio, STDBY_RC);
SX1280SetPacketType(sx1280_radio, PACKET_TYPE_LORA); 
SX1280SetRfFrequency(sx1280_radio, 2450000000); // 2.45 GHz
SX1280SetTxParams(sx1280_radio, 13, RADIO_RAMP_20_US);

// Uruchom nadawanie
//SX1280SetTxContinuousWave(sx1280_radio);
}

void sx1280TaskEntry(void *argument) {
    LoRaDevs_t *lora_devs = get_lora_devs_instance();
    SX1280_t* sx1280_radio = lora_devs->sx1280;

    
    // uint8_t rx_buff[255];
    // uint8_t rx_size = 0;
    // sx1280_config_init();
    // SX1280ClearIrqStatus(sx1280_radio, IRQ_RADIO_ALL);
    // SX1280SetRx(sx1280_radio, RX_TX_CONTINUOUS);

    // xTelemetryTimer = xTimerCreate("TelTimer", pdMS_TO_TICKS(500), pdTRUE, (void*)0, vTelemetryTimerCallback);

    // if (xTelemetryTimer != NULL) {
    //     xTimerStart(xTelemetryTimer, 0);
    // }
    // uint32_t ulNotifiedValue = 0;
    verify_freq(sx1280_radio);
    
   for(;;) {
      // 1. Daj radiu czas na stabilizację po resecie
osDelay(100); 

// 2. Pobierz wersję firmware (funkcja sama obsługuje SPI)
uint16_t fw_version = SX1280GetFirmwareVersion(sx1280_radio);

// 3. Przygotuj czytelny komunikat tekstowy
char debug_msg[50];
int msg_len = snprintf(debug_msg, sizeof(debug_msg), "SX1280 FW Version: 0x%04X\r\n", fw_version);

// 4. Wyślij przez USB (zakładając, że USB_Transmit przyjmuje wskaźnik i długość)
USB_Transmit((uint8_t*)debug_msg, msg_len);
        // if (xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotifiedValue, pdMS_TO_TICKS(100)) == pdTRUE) {

        //     if (ulNotifiedValue & RADIO_EVENT_BIT) {
        //         uint16_t irqStatus = SX1280GetIrqStatus(sx1280_radio);
        //         SX1280ClearIrqStatus(sx1280_radio, IRQ_RADIO_ALL);

        //         if (irqStatus & IRQ_RX_DONE) {
        //             SX1280GetPayload(sx1280_radio, rx_buff, &rx_size, 255);
        //             if(rx_size > 0) USB_Transmit(rx_buff, rx_size);
        //             USB_Transmit((uint8_t*)"RX Done\r\n", 9);
        //         }
                
        //         if (irqStatus & IRQ_TX_DONE) {
        //           USB_Transmit((uint8_t*)"TX Done\r\n", 9);
        //             SX1280SetRx(sx1280_radio, RX_TX_CONTINUOUS);
        //         }
        //     }

        //     //! TUTAJ POWINNO BYĆ OCZEKIWANIE NA IRQ Z USARTA WSM ZAMIAST TIMERA ALE NA RAZIE NIE MAMY USARTA PODŁĄCZONEGO DO SX1280
        //     //! JEŚLI DOSTANE DANE OD MCB TO POWINNO PAKOWAĆ SIĘ DO DO KOLEJKI Z BUFFER POOLA I TUTAJ POWINIENEM SCIAGAC TE DANE I ZWRACAC WSKAZNIK DO WOLNEGO BUFFFER POOLA
        //     if ((ulNotifiedValue & TIMER_EVENT_BIT) & GroundStationFlag) { //! Add flag t be read from nvs EEPROM Emulator
        //         uint8_t my_data[] = "PWrInSpace_Telemetry_Test";
        //         SX1280SendPayload(sx1280_radio, my_data, sizeof(my_data), RX_TX_SINGLE);
        //     }


        //      //!< USB COMMUNICATION oncomment flag on top of file to use
        //     #ifdef USB_FLAG
        //      if (ulNotifiedValue & USB_EVENT_BIT) {
        //     if (USB_Rx_Data_Len > 0) {
        //         SX1280SendPayload(sx1280_radio, UserRxBufferFS, (uint8_t)USB_Rx_Data_Len, RX_TX_SINGLE);
        //         USB_Rx_Data_Len = 0;
        //     }
        // }
        // #endif
        // }

       
    }
}

void SX1280_task_init(void){
    //HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);
    uint8_t msg2[] = "ZA MAŁO PAMIECI!\r\n";
    sx1280TaskHandle = osThreadNew(sx1280TaskEntry, NULL, &sx1280Task_attributes);
    if (sx1280TaskHandle == NULL) {
      osDelay(pdMS_TO_TICKS(3000));
      //HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET);
      //LOG_ERROR("SX1280 TASK ERROR!!");
      USB_Transmit(msg2, strlen((char*)msg2));
      return;
    }    
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET);
    //HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);
    return;
}