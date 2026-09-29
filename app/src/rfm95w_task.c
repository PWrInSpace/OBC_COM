/*
 * Author: Szymon Rzewuski, Mateusz Kłosiński
 * Updated: Mateusz Kluczka
 * Organization: PWr in Space
 */

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_gpio.h"
#include "rfm95w.h"
#include "main.h"
#include "logger.h"
#include "usb_config.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "lora_config.h"
#include "nvs_config.h"
#include "board_data.h"
#include "rfm95w_task.h"
#include "gs_scheduler.h"
#include "lora_tx_queue.h"
#include "lora_frame.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "RFM95"

#define RFM_DIO0_EVT 0x01U              // thread flag set by the DIO0 ISR
#define RFM_DIO0_MAP_TXDONE (1U << 6)   // REG_DIO_MAPPING_1[7:6]=01 -> DIO0 = TxDone
#define RFM_DIO0_MAP_RXDONE (0U << 6)   // REG_DIO_MAPPING_1[7:6]=00 -> DIO0 = RxDone
#define RFM_TX_TIMEOUT_MS 1500U         // hang backstop only; DIO0 ends TX immediately

osThreadId_t rfm95wTaskHandle = NULL;
const osThreadAttr_t rfm95wTask_attributes = {
  .name = "rfm95wTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 4096
};

static lora_tx_queue_t s_txq;
static volatile bool s_settings_dirty = false;

bool lora_gs_tx_enqueue(const uint8_t *buf, uint16_t len) {
    return lora_txq_push(&s_txq, buf, len);
}

void lora_gs_mark_settings_dirty(void) {
    s_settings_dirty = true;
}

void rfm95_dio0_isr_notify(void) {
    if (rfm95wTaskHandle != NULL) {
        (void)osThreadFlagsSet(rfm95wTaskHandle, RFM_DIO0_EVT);
    }
}

void rfm95_send_window(rfm95_t *radio, const uint8_t *payload, uint8_t payload_len, uint32_t window_ms) {
    (void)window_ms;

    (void)osThreadFlagsClear(RFM_DIO0_EVT);
    rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);
    rfm95_write_reg(radio, REG_DIO_MAPPING_1, RFM_DIO0_MAP_TXDONE);

    rfm95_fill_fifo_buf_to_send(radio, (uint8_t *)payload, payload_len);
    rfm95_start_transmission(radio);  // MODE_TX

    uint32_t fl = osThreadFlagsWait(RFM_DIO0_EVT, osFlagsWaitAny, pdMS_TO_TICKS(RFM_TX_TIMEOUT_MS));
    if ((fl & osFlagsError) != 0U) {
        LOG_INFO("TX timeout (no TxDone)");
    }

    rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);
    rfm95_idle(radio);
}

static uint32_t gs_now_ms(void *ctx) {
    (void)ctx;
    return HAL_GetTick();
}

static void gs_enter_rx(void *ctx) {
    rfm95_t *radio = (rfm95_t *)ctx;
    (void)osThreadFlagsClear(RFM_DIO0_EVT);          // drop edge (e.g. leftover TxDone)
    rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);  // W1C: clear, drop DIO0
    rfm95_set_receive_mode(radio);                   // MODE_RX_CONT + DIO0 = RxDone (0x00)
}

static size_t gs_await_rx(void *ctx, uint8_t *buf, size_t cap, uint32_t timeout_ms) {
    rfm95_t *radio = (rfm95_t *)ctx;

    uint32_t fl = osThreadFlagsWait(RFM_DIO0_EVT, osFlagsWaitAny, pdMS_TO_TICKS(timeout_ms));
    if ((fl & osFlagsError) != 0U) return 0;

    uint8_t irq = rfm95_read_reg(radio, REG_IRQ_FLAGS);
    if (!(irq & IRQ_RX_DONE_MASK)) return 0;  // woke, but not RxDone

    uint8_t max = (cap > 255U) ? 255U : (uint8_t)cap;
    uint8_t n = rfm95_receive_packet(radio, buf, max);
    rfm95_write_reg(radio, REG_IRQ_FLAGS, IRQ_ALL);  // clear -> DIO0 re-arm
    if (n == 0U) return 0;

    int16_t rssi = rfm95_packet_rssi(radio);
    if (g_state_mutex != NULL && xSemaphoreTake(g_state_mutex, 0) == pdTRUE) {
        g_system_state.RSSI = rssi;
        xSemaphoreGive(g_state_mutex);
    }
    HAL_GPIO_TogglePin(RX_RFM_GPIO_Port, RX_RFM_Pin);
    LOG_INFO("RX len=%u rssi=%d", (unsigned)n, (int)rssi);
    return n;
}

static void gs_send(void *ctx, const uint8_t *buf, size_t len) {
    HAL_GPIO_TogglePin(TX_RFM_GPIO_Port, TX_RFM_Pin);
    LOG_INFO("TX len=%u", (unsigned)len);
    rfm95_send_window((rfm95_t *)ctx, buf, (uint8_t)len, 0);
}

static void gs_forward(void *ctx, const uint8_t *buf, size_t len) {
    (void)ctx;
    USB_Transmit((uint8_t *)buf, (uint16_t)len);
}

static void apply_nvs_settings(rfm95_t *radio) {
    rfm95_sleep(radio);
    nvs_get_rfm95_settings(radio);
    rfm95w_config_init_param();
}

void rfm95wTaskEntry(void *argument) {
    (void)argument;

    rfm95_t *radio = get_lora_devs_instance()->rfm95w;
    lora_txq_init(&s_txq);

    rfm95w_config_init();
    nvs_get_rfm95_settings(radio);
    rfm95w_config_init_param();

    gs_radio_iface_t io = {
        .now_ms = gs_now_ms,
        .enter_rx = gs_enter_rx,
        .await_rx = gs_await_rx,
        .send = gs_send,
        .forward = gs_forward,
        .ctx = radio,
        .txq = &s_txq,
    };

    gs_enter_rx(radio);

    for (;;) {
        if (s_settings_dirty) {
            s_settings_dirty = false;
            LOG_INFO("GS RFM95W: reloading radio settings from NVS");
            apply_nvs_settings(radio);
        }

        gs_run_cycle(&io);
    }
}

void RFM95W_task_init(void) {
    rfm95wTaskHandle = osThreadNew(rfm95wTaskEntry, NULL, &rfm95wTask_attributes);
    if (rfm95wTaskHandle == NULL) {
        LOG_ERROR(TAG, "NO MEMORY TO CREATE TASK");
        return;
    }
}
