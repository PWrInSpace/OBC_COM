/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 3.02.2026 
 */

#pragma once

#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_desc.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint8_t CDC_InstID;


void USB_CDC_Config(void);
void USB_Transmit(uint8_t* Buf, uint16_t Len);
void USB_Transmit_Hex(uint8_t* data, uint16_t len);
