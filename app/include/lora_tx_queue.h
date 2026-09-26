/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 24.09.2026
 */
#ifndef LORA_TX_QUEUE_H
#define LORA_TX_QUEUE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LORA_TX_QUEUE_DEPTH 8U
#define LORA_TX_PACKET_MAX 256U

typedef struct {
    uint16_t len;
    uint8_t data[LORA_TX_PACKET_MAX];
} lora_tx_item_t;

typedef struct {
    lora_tx_item_t items[LORA_TX_QUEUE_DEPTH];
    volatile uint8_t head;
    volatile uint8_t tail;
} lora_tx_queue_t;

void lora_txq_init(lora_tx_queue_t *q);

bool lora_txq_push(lora_tx_queue_t *q, const uint8_t *data, uint16_t len);
bool lora_txq_pop(lora_tx_queue_t *q, uint8_t *data, uint16_t *len);
bool lora_txq_is_empty(const lora_tx_queue_t *q);

#endif /* LORA_TX_QUEUE_H */
