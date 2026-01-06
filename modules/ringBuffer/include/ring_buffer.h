#pragma once
 
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "semphr.h"

typedef struct {
    uint8_t *buffer;
    size_t head;
    size_t tail;
    size_t max; //of the buffer
    bool full;
    SemaphoreHandle_t mutex;
} RingBuffer_t;

void RingBuffer_Init(RingBuffer_t *rb, uint8_t* buffer, size_t size);
void RingBuffer_Deinit(RingBuffer_t *rb);
void RingBuffer_Reset(RingBuffer_t *rb);
bool RingBuffer_Put(RingBuffer_t *rb, uint8_t data);
bool RingBuffer_Get(RingBuffer_t *rb, uint8_t *data);
bool RingBuffer_IsFull(RingBuffer_t *rb);
bool RingBuffer_IsEmpty(RingBuffer_t *rb);
size_t RingBuffer_Capacity(RingBuffer_t *rb);
size_t RingBuffer_Size(RingBuffer_t *rb);