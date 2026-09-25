/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 24.09.2026
 */
#include "lora_frame.h"
#include "lora.h"
#include <string.h>

uint8_t lora_frame_checksum(const uint8_t *data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += data[i];
    }
    return sum;
}

size_t lora_frame_encode_appcmd(uint8_t *out, size_t cap, uint32_t lora_dev_id, uint32_t sys_dev_id, uint32_t command, int32_t payload) {
    if (out == NULL || cap < LORA_PACKET_PREFIX_LEN + 2u) return 0;

    memcpy(out, LORA_PACKET_PREFIX, LORA_PACKET_PREFIX_LEN);
    size_t off = LORA_PACKET_PREFIX_LEN;

    uint8_t workspace[256];
    struct obc_lo_ra_frame_t *frame = obc_lo_ra_frame_new(workspace, sizeof(workspace));
    if (frame == NULL) return 0;

    struct obc_app_frame_t app = {0};
    frame->frame = obc_lo_ra_frame_frame_app_frame_e;
    frame->app_frame_p = &app;

    app.lora_dev_id.is_present = true;  app.lora_dev_id.value = lora_dev_id;
    app.sys_dev_id.is_present  = true;  app.sys_dev_id.value  = sys_dev_id;
    app.command.is_present     = true;  app.command.value     = command;
    app.payload.is_present     = true;  app.payload.value     = payload;

    int pb_len = obc_lo_ra_frame_encode(frame, out + off, cap - off - 1u);
    if (pb_len <= 0) {
        return 0;
    }

    uint8_t cksum = lora_frame_checksum(out + off, (size_t)pb_len);
    off += (size_t)pb_len;
    out[off++] = cksum;

    return off;
}

size_t lora_frame_build_sync(uint8_t *out, size_t cap) {
    return lora_frame_encode_appcmd(out, cap,
                                    LORA_DEV_ID_ALL, LORA_SYS_ID_MCB,
                                    CMD_LORA_SYNC, 0);
}

bool lora_frame_validate(const uint8_t *pkt, size_t len, const uint8_t **payload_p, size_t *payload_len_p) {
    if (pkt == NULL || len < LORA_PACKET_PREFIX_LEN + 2u) return false;
    if (memcmp(pkt, LORA_PACKET_PREFIX, LORA_PACKET_PREFIX_LEN) != 0) return false;

    size_t pb_len = len - LORA_PACKET_PREFIX_LEN - 1u;
    uint8_t want = pkt[len - 1u];
    if (lora_frame_checksum(pkt + LORA_PACKET_PREFIX_LEN, pb_len) != want) return false;

    if (payload_p != NULL) *payload_p = pkt + LORA_PACKET_PREFIX_LEN;
    if (payload_len_p != NULL) *payload_len_p = pb_len;
    return true;
}

// static const uint8_t k_sync_reference[] = {
//     0x53, 0x50, 0x33, 0x4D, 0x49, 0x4B,  /* "SP3MIK" */
//     0x1A, 0x09,                          /* LoRaFrame.app_frame, len 9 */
//     0x08, 0x00,                          /*   lora_dev_id = 0 (ALL)    */
//     0x10, 0x01,                          /*   sys_dev_id  = 1 (MCB)    */
//     0x18, 0xBA, 0x01,                    /*   command     = 0xBA       */
//     0x20, 0x00,                          /*   payload     = 0          */
//     0x2F                                 /* checksum over protobuf     */
// };

// bool lora_frame_selftest(void) {
//     uint8_t buf[LORA_FRAME_MAX];
//     size_t n = lora_frame_build_sync(buf, sizeof(buf));
//     return n == sizeof(k_sync_reference) &&
//            memcmp(buf, k_sync_reference, n) == 0;
// }
