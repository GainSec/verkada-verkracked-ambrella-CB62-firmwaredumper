#ifndef CB62_TEST_FAKE_LIBUSB_H
#define CB62_TEST_FAKE_LIBUSB_H

#include "../src/probe.h"

struct fake_libusb {
    struct cb62_usb_device devices[2];
    size_t device_count;
    int discover_result;
    int open_result;
    int out_result;
    int in_result;
    int short_out;
    int short_in;
    uint8_t response[CB62_INQUIRY_RESPONSE_SIZE];
    unsigned open_calls;
    unsigned out_calls;
    unsigned in_calls;
    unsigned close_calls;
    uint8_t sent[CB62_INQUIRY_REQUEST_SIZE];
    size_t sent_len;
};

void fake_libusb_init_valid(struct fake_libusb *fake);
const struct cb62_usb_ops *fake_libusb_ops(void);

#endif
