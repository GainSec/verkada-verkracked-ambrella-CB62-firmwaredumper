#ifndef CB62_ADDR_INQUIRY_PROBE_H
#define CB62_ADDR_INQUIRY_PROBE_H

#include "addr_protocol.h"
#include "probe.h"

struct cb62_addr_probe_observation {
    struct cb62_usb_device device;
    uint8_t request[CB62_ADDR_INQUIRY_REQUEST_SIZE];
    uint8_t response_wire[CB62_ADDR_INQUIRY_RESPONSE_SIZE];
    size_t request_transferred;
    size_t response_transferred;
    struct cb62_addr_inquiry_response response;
};

enum cb62_addr_probe_result {
    CB62_ADDR_PROBE_OK = 0,
    CB62_ADDR_PROBE_DISCOVERY_FAILED = -20,
    CB62_ADDR_PROBE_DEVICE_COUNT_REJECTED = -21,
    CB62_ADDR_PROBE_IDENTITY_REJECTED = -22,
    CB62_ADDR_PROBE_TOPOLOGY_REJECTED = -23,
    CB62_ADDR_PROBE_OPEN_FAILED = -24,
    CB62_ADDR_PROBE_OUT_FAILED = -25,
    CB62_ADDR_PROBE_IN_FAILED = -26,
    CB62_ADDR_PROBE_RESPONSE_REJECTED = -27,
};

int cb62_probe_addr_once(const struct cb62_usb_ops *ops, void *ctx,
                         struct cb62_addr_probe_observation *observation);

#endif
