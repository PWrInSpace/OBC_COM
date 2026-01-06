#include "ring_buffer.h"

#include "FreeRTOS.h"
#include "semphr.h"
 
void RingBuffer_Init(RingBuffer_t *rb, uint8_t* buffer, size_t size) {
     rb->buffer = buffer;
     rb->max = size;
     RingBuffer_Reset(rb);

    if (rb->mutex == NULL) {
        rb->mutex = xSemaphoreCreateMutex();
    }
}

void RingBuffer_Deinit(RingBuffer_t *rb) {
    if (rb->mutex != NULL) {
        vSemaphoreDelete(rb->mutex);
        rb->mutex = NULL;
    }
}

void RingBuffer_Reset(RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->full = false;
}

bool RingBuffer_Put(RingBuffer_t *rb, uint8_t data) {
    bool ret = false;
    if (xSemaphoreTake(rb->mutex, portMAX_DELAY) == pdTRUE) {
        rb->buffer[rb->head] = data;
        if (rb->full) {
            rb->tail = (rb->tail + 1) % rb->max;
        }
        rb->head = (rb->head + 1) % rb->max;
        rb->full = (rb->head == rb->tail);
        ret = true;
        xSemaphoreGive(rb->mutex);
    }
    return ret;
}

bool RingBuffer_Get(RingBuffer_t *rb, uint8_t *data) {
    bool ret = false;
    if (!RingBuffer_IsEmpty(rb)) {
        if (xSemaphoreTake(rb->mutex, portMAX_DELAY) == pdTRUE) {
            *data = rb->buffer[rb->tail];
            rb->full = false;
            rb->tail = (rb->tail + 1) % rb->max;
            ret = true;
        }
        xSemaphoreGive(rb->mutex);
    }
    return ret;
}

bool RingBuffer_IsFull(RingBuffer_t *rb) {
    bool full;
    if (xSemaphoreTake(rb->mutex, portMAX_DELAY) == pdTRUE) {
        full = rb->full;
        xSemaphoreGive(rb->mutex);
    } else {
        full = false;
    }
    return full;
}

bool RingBuffer_IsEmpty(RingBuffer_t *rb) {
    bool empty;
    if (xSemaphoreTake(rb->mutex, portMAX_DELAY) == pdTRUE) {
        empty = (!rb->full && (rb->head == rb->tail));
        xSemaphoreGive(rb->mutex);
    } else {
        empty = true;
    }
    return empty;
}

size_t RingBuffer_Capacity(RingBuffer_t *rb) {
    return rb->max;
}

size_t RingBuffer_Size(RingBuffer_t *rb) {
    size_t size = 0;
    if (xSemaphoreTake(rb->mutex, portMAX_DELAY) == pdTRUE) {
        if (rb->full) {
            size = rb->max;
        } else if (rb->head >= rb->tail) {
            size = rb->head - rb->tail;
        } else {
            size = rb->max + rb->head - rb->tail;
        }
        xSemaphoreGive(rb->mutex);
    }
    return size;
}