#ifndef CB62_BLOCK_PROBE_H
#define CB62_BLOCK_PROBE_H

#include "block_protocol.h"
#include "probe.h"

struct cb62_block_probe_observation {
    struct cb62_usb_device device;
    uint8_t ready_request[CB62_BLOCK_COMMAND_SIZE];
    uint8_t ready_response_wire[CB62_BLOCK_RESPONSE_SIZE];
    uint8_t send_data_request[CB62_BLOCK_COMMAND_SIZE];
    uint8_t data[CB62_BLOCK_FIXED_LENGTH];
    size_t ready_request_transferred;
    size_t ready_response_transferred;
    size_t send_data_request_transferred;
    size_t data_transferred;
    struct cb62_block_ready_response ready;
    uint32_t computed_crc32;
};

enum cb62_block_probe_result {
    CB62_BLOCK_PROBE_OK = 0,
    CB62_BLOCK_PROBE_DISCOVERY_FAILED = -30,
    CB62_BLOCK_PROBE_DEVICE_COUNT_REJECTED = -31,
    CB62_BLOCK_PROBE_IDENTITY_REJECTED = -32,
    CB62_BLOCK_PROBE_TOPOLOGY_REJECTED = -33,
    CB62_BLOCK_PROBE_OPEN_FAILED = -34,
    CB62_BLOCK_PROBE_READY_OUT_FAILED = -35,
    CB62_BLOCK_PROBE_READY_IN_FAILED = -36,
    CB62_BLOCK_PROBE_READY_REJECTED = -37,
    CB62_BLOCK_PROBE_SEND_DATA_OUT_FAILED = -38,
    CB62_BLOCK_PROBE_DATA_IN_FAILED = -39,
    CB62_BLOCK_PROBE_CRC_MISMATCH = -40,
};

int cb62_probe_fixed_block_once(
    const struct cb62_usb_ops *ops, void *ctx,
    struct cb62_block_probe_observation *observation);

#endif
