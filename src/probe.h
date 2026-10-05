#ifndef CB62_INQUIRY_PROBE_H
#define CB62_INQUIRY_PROBE_H

#include "protocol.h"

#include <stddef.h>
#include <stdint.h>

#define CB62_USB_VID 0x4255u
#define CB62_USB_PID 0x0001u
#define CB62_USB_MANUFACTURER "AMBA"
#define CB62_USB_PRODUCT "Ambarella USB generic class"
#define CB62_USB_INTERFACE_CLASS 0xffu
#define CB62_USB_INTERFACE_SUBCLASS 0xffu
#define CB62_USB_INTERFACE_PROTOCOL 0x00u
#define CB62_USB_ENDPOINT_OUT 0x01u
#define CB62_USB_ENDPOINT_IN 0x81u

struct cb62_usb_device {
    uint16_t vid;
    uint16_t pid;
    uint8_t bus;
    uint8_t address;
    uint8_t interface_number;
    uint8_t matching_interface_count;
    uint8_t bulk_endpoint_count;
    uint8_t interface_class;
    uint8_t interface_subclass;
    uint8_t interface_protocol;
    uint8_t endpoint_out;
    uint8_t endpoint_in;
    char manufacturer[64];
    char product[64];
    char serial[64];
};

struct cb62_usb_ops {
    int (*discover)(void *ctx, struct cb62_usb_device *out,
                    size_t capacity, size_t *count);
    int (*open_claim)(void *ctx, const struct cb62_usb_device *device);
    int (*bulk_out)(void *ctx, uint8_t endpoint, const uint8_t *data,
                    size_t len, size_t *transferred);
    int (*bulk_in)(void *ctx, uint8_t endpoint, uint8_t *data,
                   size_t len, size_t *transferred);
    void (*close)(void *ctx);
};

struct cb62_probe_observation {
    struct cb62_usb_device device;
    uint8_t request[CB62_INQUIRY_REQUEST_SIZE];
    uint8_t response_wire[CB62_INQUIRY_RESPONSE_SIZE];
    size_t request_transferred;
    size_t response_transferred;
    struct cb62_inquiry_response response;
};

enum cb62_probe_result {
    CB62_PROBE_OK = 0,
    CB62_PROBE_DISCOVERY_FAILED = -10,
    CB62_PROBE_DEVICE_COUNT_REJECTED = -11,
    CB62_PROBE_IDENTITY_REJECTED = -12,
    CB62_PROBE_TOPOLOGY_REJECTED = -13,
    CB62_PROBE_OPEN_FAILED = -14,
    CB62_PROBE_OUT_FAILED = -15,
    CB62_PROBE_IN_FAILED = -16,
    CB62_PROBE_RESPONSE_REJECTED = -17,
};

int cb62_probe_once(const struct cb62_usb_ops *ops, void *ctx,
                    struct cb62_probe_observation *observation);

#endif
