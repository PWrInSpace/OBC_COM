/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 24.09.2026
 */
#ifndef LORA_FRAME_H
#define LORA_FRAME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "lora_commands.h"

#define LORA_PACKET_PREFIX "SP3MIK"
#define LORA_PACKET_PREFIX_LEN (sizeof(LORA_PACKET_PREFIX) - 1)

#define LORA_FRAME_MAX 255
#define LORA_SYS_ID_MCB 0x01

uint8_t lora_frame_checksum(const uint8_t *data, size_t len);

/**
 * @param  out,cap  Output buffer and its capacity.
 * @return Total frame length (prefix + protobuf + checksum), or 0 on error/overflow.
 */
size_t lora_frame_encode_appcmd(uint8_t *out, size_t cap, uint32_t lora_dev_id, uint32_t sys_dev_id, uint32_t command, int32_t payload);

size_t lora_frame_build_sync(uint8_t *out, size_t cap);

/**
 * @param  pkt,len  Received bytes.
 * @param  payload_p,payload_len_p  On success, point at the protobuf region in place (may be NULL if not needed).
 */
bool lora_frame_validate(const uint8_t *pkt, size_t len, const uint8_t **payload_p, size_t *payload_len_p);

#endif /* LORA_FRAME_H */
