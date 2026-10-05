#include "fake_libusb.h"

#include <string.h>

static void put_u32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static int discover(void *opaque, struct cb62_usb_device *out,
                    size_t capacity, size_t *count)
{
    struct fake_libusb *f = opaque;
    if (f->discover_result != 0) return f->discover_result;
    *count = f->device_count;
    size_t n = f->device_count < capacity ? f->device_count : capacity;
    memcpy(out, f->devices, n * sizeof(*out));
    return 0;
}

static int open_claim(void *opaque, const struct cb62_usb_device *device)
{
    struct fake_libusb *f = opaque;
    (void)device;
    f->open_calls++;
    return f->open_result;
}

static int bulk_out(void *opaque, uint8_t endpoint, const uint8_t *data,
                    size_t len, size_t *transferred)
{
    struct fake_libusb *f = opaque;
    (void)endpoint;
    f->out_calls++;
    f->sent_len = len;
    memcpy(f->sent, data, len <= sizeof(f->sent) ? len : sizeof(f->sent));
    *transferred = f->short_out && len ? len - 1 : len;
    return f->out_result;
}

static int bulk_in(void *opaque, uint8_t endpoint, uint8_t *data,
                   size_t len, size_t *transferred)
{
    struct fake_libusb *f = opaque;
    (void)endpoint;
    f->in_calls++;
    memcpy(data, f->response, len);
    *transferred = f->short_in && len ? len - 1 : len;
    return f->in_result;
}

static void close_device(void *opaque)
{
    struct fake_libusb *f = opaque;
    f->close_calls++;
}

static const struct cb62_usb_ops ops = {
    .discover = discover,
    .open_claim = open_claim,
    .bulk_out = bulk_out,
    .bulk_in = bulk_in,
    .close = close_device,
};

void fake_libusb_init_valid(struct fake_libusb *f)
{
    memset(f, 0, sizeof(*f));
    f->device_count = 1;
    f->devices[0].vid = CB62_USB_VID;
    f->devices[0].pid = CB62_USB_PID;
    strcpy(f->devices[0].manufacturer, CB62_USB_MANUFACTURER);
    strcpy(f->devices[0].product, CB62_USB_PRODUCT);
    f->devices[0].interface_class = CB62_USB_INTERFACE_CLASS;
    f->devices[0].interface_subclass = CB62_USB_INTERFACE_SUBCLASS;
    f->devices[0].interface_protocol = CB62_USB_INTERFACE_PROTOCOL;
    f->devices[0].interface_number = 0;
    f->devices[0].matching_interface_count = 1;
    f->devices[0].bulk_endpoint_count = 2;
    f->devices[0].endpoint_out = CB62_USB_ENDPOINT_OUT;
    f->devices[0].endpoint_in = CB62_USB_ENDPOINT_IN;
    put_u32le(f->response + 0, CB62_STATUS_TOKEN);
    put_u32le(f->response + 4, CB62_RESPONSE_SUCCESS);
    put_u32le(f->response + 8, CB62_EXPECTED_CHIP_ID);
    put_u32le(f->response + 12, 0);
}

const struct cb62_usb_ops *fake_libusb_ops(void)
{
    return &ops;
}
