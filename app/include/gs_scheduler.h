/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 24.09.2026
 */
#ifndef GS_SCHEDULER_H
#define GS_SCHEDULER_H

#include <stdint.h>
#include <stddef.h>
#include "lora_tx_queue.h"

#define LORA_FORWARD_MS_1 300U            // listen and relay to app (OBC/MCB)
#define LORA_SEND_MS_1_START_DEADLINE 25U // TX must begin within this frame
#define LORA_SEND_MS_1 100U               // send one queued frame
#define LORA_FORWARD_MS_2 200U            // listen and relay to app (TANWA)
#define LORA_DRAIN_QUEUE_MS 400U          // trailing listen + guarantee empty queue
#define LORA_MIX_POLL_MS 20U              // in TX/RX windows: max RX block before re-checking the TX queue

typedef struct gs_radio_iface {
    uint32_t (*now_ms)(void *ctx);                                                // e.g. HAL_GetTick()
    void (*enter_rx)(void *ctx);                                                  // Put the radio into continuous receive
    size_t (*await_rx)(void *ctx, uint8_t *buf, size_t cap, uint32_t timeout_ms); // Block up to timeout_ms for one packet (DIO0 event), copy up to cap bytes, return length (0 = none)
    void (*send)(void *ctx, const uint8_t *buf, size_t len);                      // Transmit len bytes, blocking until done
    void (*forward)(void *ctx, const uint8_t *buf, size_t len);                   // Hand a received frame to the app link

    void *ctx; // radio handle
    lora_tx_queue_t *txq;
} gs_radio_iface_t;

void gs_run_cycle(const gs_radio_iface_t *io);

#endif /* GS_SCHEDULER_H */
