#ifndef CB62_LIBUSB_BACKEND_H
#define CB62_LIBUSB_BACKEND_H

#include "probe.h"

struct cb62_libusb;

int cb62_libusb_init(struct cb62_libusb **out);
void cb62_libusb_destroy(struct cb62_libusb *ctx);
const struct cb62_usb_ops *cb62_libusb_ops(void);
const char *cb62_libusb_last_error(const struct cb62_libusb *ctx);

#endif
