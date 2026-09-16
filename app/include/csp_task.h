/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 16.09.2026
 */
#ifndef CSP_TASK_H
#define CSP_TASK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void CSP_Task_Init(void);

/* Feed raw bytes received on the USB CDC link into the CSP/KISS interface. */
void CSP_USB_Kiss_Feed(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CSP_TASK_H */
