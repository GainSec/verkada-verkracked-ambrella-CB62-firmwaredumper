#ifndef CB62_MEMORY_PROBE_H
#define CB62_MEMORY_PROBE_H

#include "memory_protocol.h"
#include "probe.h"

struct cb62_memory_probe_observation {
    struct cb62_usb_device device;
    uint8_t request[CB62_MEMORY_READ_REQUEST_SIZE];
    uint8_t response_wire[CB62_MEMORY_READ_RESPONSE_SIZE];
    size_t request_transferred;
    size_t response_transferred;
    struct cb62_memory_read_response response;
};

enum cb62_memory_probe_result {
    CB62_MEMORY_PROBE_OK = 0,
    CB62_MEMORY_PROBE_DISCOVERY_FAILED = -30,
    CB62_MEMORY_PROBE_DEVICE_COUNT_REJECTED = -31,
    CB62_MEMORY_PROBE_IDENTITY_REJECTED = -32,
    CB62_MEMORY_PROBE_TOPOLOGY_REJECTED = -33,
    CB62_MEMORY_PROBE_OPEN_FAILED = -34,
    CB62_MEMORY_PROBE_OUT_FAILED = -35,
    CB62_MEMORY_PROBE_IN_FAILED = -36,
    CB62_MEMORY_PROBE_RESPONSE_REJECTED = -37,
};

int cb62_probe_memory_once(
    const struct cb62_usb_ops *ops, void *ctx,
    struct cb62_memory_probe_observation *observation);

#endif
