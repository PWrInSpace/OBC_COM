/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 24.09.2026
 */
#include "gs_scheduler.h"
#include "lora_frame.h"

static inline uint32_t elapsed_since(const gs_radio_iface_t *io, uint32_t start) {
    return io->now_ms(io->ctx) - start;
}

static void forward_window(const gs_radio_iface_t *io, uint32_t duration_ms) {
    uint8_t buf[LORA_FRAME_MAX];
    uint32_t start = io->now_ms(io->ctx);

    io->enter_rx(io->ctx);
    while (elapsed_since(io, start) < duration_ms) {
        size_t n = io->poll_rx(io->ctx, buf, sizeof(buf));
        if (n > 0U) {
            io->forward(io->ctx, buf, n);
            io->enter_rx(io->ctx);
        }
    }
}

static void send_one_from_queue(const gs_radio_iface_t *io, uint32_t duration_ms, uint32_t start_deadline_ms) {
    uint8_t tx[LORA_TX_PACKET_MAX];
    uint8_t rx[LORA_FRAME_MAX];
    uint16_t tx_len = 0U;
    bool sent = false;
    uint32_t start = io->now_ms(io->ctx);

    io->enter_rx(io->ctx);
    while (elapsed_since(io, start) < duration_ms) {
        if (!sent && elapsed_since(io, start) < start_deadline_ms &&
            lora_txq_pop(io->txq, tx, &tx_len)) {
            io->send(io->ctx, tx, tx_len);
            io->enter_rx(io->ctx);
            sent = true;
            continue;
        }

        size_t n = io->poll_rx(io->ctx, rx, sizeof(rx));
        if (n > 0U) {
            io->forward(io->ctx, rx, n);
            io->enter_rx(io->ctx);
        }
    }
}

static void drain_queue(const gs_radio_iface_t *io, uint32_t duration_ms) {
    uint8_t tx[LORA_TX_PACKET_MAX];
    uint8_t rx[LORA_FRAME_MAX];
    uint16_t tx_len = 0U;
    uint32_t start = io->now_ms(io->ctx);

    io->enter_rx(io->ctx);
    while (elapsed_since(io, start) < duration_ms) {
        if (lora_txq_pop(io->txq, tx, &tx_len)) {
            io->send(io->ctx, tx, tx_len);
            io->enter_rx(io->ctx);
            continue;
        }

        size_t n = io->poll_rx(io->ctx, rx, sizeof(rx));
        if (n > 0U) {
            io->forward(io->ctx, rx, n);
            io->enter_rx(io->ctx);
        }
    }

    while (lora_txq_pop(io->txq, tx, &tx_len)) {
        io->send(io->ctx, tx, tx_len);
    }
    io->enter_rx(io->ctx);
}

void gs_run_cycle(const gs_radio_iface_t *io) {
    if (io == NULL || io->now_ms == NULL || io->enter_rx == NULL ||
        io->poll_rx == NULL || io->send == NULL || io->forward == NULL ||
        io->txq == NULL) {
        return;
    }

    uint8_t sync[LORA_FRAME_MAX];
    size_t sync_len = lora_frame_build_sync(sync, sizeof(sync));
    if (sync_len > 0U) {
        io->send(io->ctx, sync, sync_len);
    }

    forward_window(io, LORA_FORWARD_MS_1);
    send_one_from_queue(io, LORA_SEND_MS_1, LORA_SEND_MS_1_START_DEADLINE);
    forward_window(io, LORA_FORWARD_MS_2);
    drain_queue(io, LORA_DRAIN_QUEUE_MS);
}
