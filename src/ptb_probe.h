#ifndef CB62_PTB_PROBE_H
#define CB62_PTB_PROBE_H

#include "probe.h"
#include "ptb_protocol.h"

struct cb62_ptb_probe_observation {
    struct cb62_usb_device device;
    uint8_t ready_request[CB62_PTB_COMMAND_SIZE];
    uint8_t ready_response_wire[CB62_PTB_RESPONSE_SIZE];
    uint8_t send_data_request[CB62_PTB_COMMAND_SIZE];
    uint8_t data[CB62_PTB_FIXED_LENGTH];
    size_t ready_request_transferred;
    size_t ready_response_transferred;
    size_t send_data_request_transferred;
    size_t data_transferred;
    struct cb62_ptb_ready_response ready;
    uint32_t computed_crc32;
};

enum cb62_ptb_probe_result {
    CB62_PTB_PROBE_OK = 0,
    CB62_PTB_PROBE_DISCOVERY_FAILED = -70,
    CB62_PTB_PROBE_DEVICE_COUNT_REJECTED = -71,
    CB62_PTB_PROBE_IDENTITY_REJECTED = -72,
    CB62_PTB_PROBE_TOPOLOGY_REJECTED = -73,
    CB62_PTB_PROBE_OPEN_FAILED = -74,
    CB62_PTB_PROBE_READY_OUT_FAILED = -75,
    CB62_PTB_PROBE_READY_IN_FAILED = -76,
    CB62_PTB_PROBE_READY_REJECTED = -77,
    CB62_PTB_PROBE_SEND_DATA_OUT_FAILED = -78,
    CB62_PTB_PROBE_DATA_IN_FAILED = -79,
    CB62_PTB_PROBE_CRC_MISMATCH = -80,
};

int cb62_probe_fixed_ptb_once(
    const struct cb62_usb_ops *ops, void *ctx,
    struct cb62_ptb_probe_observation *observation);

#endif
