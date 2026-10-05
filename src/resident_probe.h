#ifndef CB62_RESIDENT_PROBE_H
#define CB62_RESIDENT_PROBE_H

#include "probe.h"
#include "resident_protocol.h"

struct cb62_resident_probe_observation {
    struct cb62_usb_device device;
    uint8_t ready_request[CB62_RESIDENT_COMMAND_SIZE];
    uint8_t ready_response_wire[CB62_RESIDENT_RESPONSE_SIZE];
    uint8_t send_data_request[CB62_RESIDENT_COMMAND_SIZE];
    uint8_t data[CB62_RESIDENT_FIXED_LENGTH];
    size_t ready_request_transferred;
    size_t ready_response_transferred;
    size_t send_data_request_transferred;
    size_t data_transferred;
    struct cb62_resident_ready_response ready;
    uint32_t computed_crc32;
};

enum cb62_resident_probe_result {
    CB62_RESIDENT_PROBE_OK = 0,
    CB62_RESIDENT_PROBE_DISCOVERY_FAILED = -50,
    CB62_RESIDENT_PROBE_DEVICE_COUNT_REJECTED = -51,
    CB62_RESIDENT_PROBE_IDENTITY_REJECTED = -52,
    CB62_RESIDENT_PROBE_TOPOLOGY_REJECTED = -53,
    CB62_RESIDENT_PROBE_OPEN_FAILED = -54,
    CB62_RESIDENT_PROBE_READY_OUT_FAILED = -55,
    CB62_RESIDENT_PROBE_READY_IN_FAILED = -56,
    CB62_RESIDENT_PROBE_READY_REJECTED = -57,
    CB62_RESIDENT_PROBE_SEND_DATA_OUT_FAILED = -58,
    CB62_RESIDENT_PROBE_DATA_IN_FAILED = -59,
    CB62_RESIDENT_PROBE_CRC_MISMATCH = -60,
};

int cb62_probe_fixed_resident_once(
    const struct cb62_usb_ops *ops, void *ctx,
    struct cb62_resident_probe_observation *observation);

int cb62_resident_probe_has_complete_data(
    int result, const struct cb62_resident_probe_observation *observation);

#endif
