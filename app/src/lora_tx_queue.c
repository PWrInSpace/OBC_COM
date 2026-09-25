/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 24.09.2026
 */
#include "lora_tx_queue.h"
#include <string.h>

static inline uint8_t next_index(uint8_t i) {
    return (uint8_t)((i + 1U) % LORA_TX_QUEUE_DEPTH);
}

void lora_txq_init(lora_tx_queue_t *q) {
    if (q == NULL) return;

    q->head = 0U;
    q->tail = 0U;
}

bool lora_txq_push(lora_tx_queue_t *q, const uint8_t *data, uint16_t len) {
    if (q == NULL || data == NULL || len == 0U || len > LORA_TX_PACKET_MAX) return false;

    uint8_t tail = q->tail;
    uint8_t next = next_index(tail);
    if (next == q->head) return false; // full

    q->items[tail].len = len;
    memcpy(q->items[tail].data, data, len);
    q->tail = next;
    return true;
}

bool lora_txq_pop(lora_tx_queue_t *q, uint8_t *data, uint16_t *len) {
    if (q == NULL || data == NULL || len == NULL) return false;

    uint8_t head = q->head;
    if (head == q->tail) return false; // empty 

    *len = q->items[head].len;
    memcpy(data, q->items[head].data, q->items[head].len);
    q->head = next_index(head);
    return true;
}

bool lora_txq_is_empty(const lora_tx_queue_t *q) {
    return (q == NULL) || (q->head == q->tail);
}
