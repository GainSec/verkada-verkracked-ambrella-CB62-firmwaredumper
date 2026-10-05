#include "libusb_backend.h"

#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CB62_USB_TIMEOUT_MS 1000u

struct cb62_libusb {
    libusb_context *usb;
    libusb_device_handle *handle;
    uint8_t claimed_interface;
    char error[160];
};

static int fail(struct cb62_libusb *ctx, const char *operation, int code)
{
    snprintf(ctx->error, sizeof(ctx->error), "%s: %s",
             operation, libusb_error_name(code));
    return -1;
}

static int read_ascii(libusb_device_handle *handle, uint8_t index,
                      char *out, size_t out_size)
{
    if (index == 0 || out_size == 0) return -1;
    int n = libusb_get_string_descriptor_ascii(handle, index,
                                                (unsigned char *)out,
                                                (int)out_size - 1);
    if (n < 0) return n;
    out[n] = '\0';
    return 0;
}

static int describe_device(struct cb62_libusb *ctx, libusb_device *usb_device,
                           const struct libusb_device_descriptor *descriptor,
                           struct cb62_usb_device *out)
{
    memset(out, 0, sizeof(*out));
    out->vid = descriptor->idVendor;
    out->pid = descriptor->idProduct;
    out->bus = libusb_get_bus_number(usb_device);
    out->address = libusb_get_device_address(usb_device);

    libusb_device_handle *handle = NULL;
    int rc = libusb_open(usb_device, &handle);
    if (rc != LIBUSB_SUCCESS) return fail(ctx, "open for descriptors", rc);
    rc = read_ascii(handle, descriptor->iManufacturer,
                    out->manufacturer, sizeof(out->manufacturer));
    if (rc == 0)
        rc = read_ascii(handle, descriptor->iProduct,
                        out->product, sizeof(out->product));
    if (rc == 0 && descriptor->iSerialNumber != 0)
        rc = read_ascii(handle, descriptor->iSerialNumber,
                        out->serial, sizeof(out->serial));
    libusb_close(handle);
    if (rc != 0) return fail(ctx, "read string descriptor", rc);

    struct libusb_config_descriptor *config = NULL;
    rc = libusb_get_active_config_descriptor(usb_device, &config);
    if (rc != LIBUSB_SUCCESS) return fail(ctx, "active configuration", rc);
    for (uint8_t i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface *interface = &config->interface[i];
        for (int a = 0; a < interface->num_altsetting; a++) {
            const struct libusb_interface_descriptor *alt = &interface->altsetting[a];
            if (alt->bInterfaceClass != CB62_USB_INTERFACE_CLASS ||
                alt->bInterfaceSubClass != CB62_USB_INTERFACE_SUBCLASS ||
                alt->bInterfaceProtocol != CB62_USB_INTERFACE_PROTOCOL)
                continue;
            out->matching_interface_count++;
            if (out->matching_interface_count != 1) continue;
            out->interface_number = alt->bInterfaceNumber;
            out->interface_class = alt->bInterfaceClass;
            out->interface_subclass = alt->bInterfaceSubClass;
            out->interface_protocol = alt->bInterfaceProtocol;
            for (uint8_t e = 0; e < alt->bNumEndpoints; e++) {
                const struct libusb_endpoint_descriptor *endpoint = &alt->endpoint[e];
                if ((endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) !=
                    LIBUSB_TRANSFER_TYPE_BULK)
                    continue;
                out->bulk_endpoint_count++;
                if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN)
                    out->endpoint_in = endpoint->bEndpointAddress;
                else
                    out->endpoint_out = endpoint->bEndpointAddress;
            }
        }
    }
    libusb_free_config_descriptor(config);
    return 0;
}

static int discover(void *opaque, struct cb62_usb_device *out,
                    size_t capacity, size_t *count)
{
    struct cb62_libusb *ctx = opaque;
    libusb_device **devices = NULL;
    ssize_t n = libusb_get_device_list(ctx->usb, &devices);
    if (n < 0) return fail(ctx, "enumerate", (int)n);
    *count = 0;
    int result = 0;
    for (ssize_t i = 0; i < n; i++) {
        struct libusb_device_descriptor descriptor;
        int rc = libusb_get_device_descriptor(devices[i], &descriptor);
        if (rc != LIBUSB_SUCCESS) {
            result = fail(ctx, "device descriptor", rc);
            break;
        }
        if (descriptor.idVendor != CB62_USB_VID ||
            descriptor.idProduct != CB62_USB_PID)
            continue;
        if (*count < capacity &&
            describe_device(ctx, devices[i], &descriptor, &out[*count]) != 0) {
            result = -1;
            break;
        }
        (*count)++;
    }
    libusb_free_device_list(devices, 1);
    return result;
}

static int open_claim(void *opaque, const struct cb62_usb_device *device)
{
    struct cb62_libusb *ctx = opaque;
    libusb_device **devices = NULL;
    ssize_t n = libusb_get_device_list(ctx->usb, &devices);
    if (n < 0) return fail(ctx, "enumerate for claim", (int)n);
    libusb_device *selected = NULL;
    for (ssize_t i = 0; i < n; i++) {
        if (libusb_get_bus_number(devices[i]) == device->bus &&
            libusb_get_device_address(devices[i]) == device->address) {
            selected = devices[i];
            break;
        }
    }
    int result = -1;
    if (selected == NULL) {
        snprintf(ctx->error, sizeof(ctx->error), "selected device disappeared");
        goto done;
    }
    int rc = libusb_open(selected, &ctx->handle);
    if (rc != LIBUSB_SUCCESS) {
        result = fail(ctx, "open for inquiry", rc);
        goto done;
    }
    rc = libusb_kernel_driver_active(ctx->handle, device->interface_number);
    if (rc == 1) {
        snprintf(ctx->error, sizeof(ctx->error),
                 "kernel driver is active; refusing to detach it");
        libusb_close(ctx->handle);
        ctx->handle = NULL;
        goto done;
    }
    if (rc < 0 && rc != LIBUSB_ERROR_NOT_SUPPORTED) {
        result = fail(ctx, "kernel-driver check", rc);
        libusb_close(ctx->handle);
        ctx->handle = NULL;
        goto done;
    }
    rc = libusb_claim_interface(ctx->handle, device->interface_number);
    if (rc != LIBUSB_SUCCESS) {
        result = fail(ctx, "claim interface", rc);
        libusb_close(ctx->handle);
        ctx->handle = NULL;
        goto done;
    }
    ctx->claimed_interface = device->interface_number;
    result = 0;
done:
    libusb_free_device_list(devices, 1);
    return result;
}

static int bulk_out(void *opaque, uint8_t endpoint, const uint8_t *data,
                    size_t len, size_t *transferred)
{
    struct cb62_libusb *ctx = opaque;
    int actual = 0;
    int rc = libusb_bulk_transfer(ctx->handle, endpoint,
                                  (unsigned char *)(uintptr_t)data,
                                  (int)len, &actual, CB62_USB_TIMEOUT_MS);
    *transferred = actual > 0 ? (size_t)actual : 0;
    return rc == LIBUSB_SUCCESS ? 0 : fail(ctx, "bulk OUT", rc);
}

static int bulk_in(void *opaque, uint8_t endpoint, uint8_t *data,
                   size_t len, size_t *transferred)
{
    struct cb62_libusb *ctx = opaque;
    int actual = 0;
    int rc = libusb_bulk_transfer(ctx->handle, endpoint, data,
                                  (int)len, &actual, CB62_USB_TIMEOUT_MS);
    *transferred = actual > 0 ? (size_t)actual : 0;
    return rc == LIBUSB_SUCCESS ? 0 : fail(ctx, "bulk IN", rc);
}

static void close_device(void *opaque)
{
    struct cb62_libusb *ctx = opaque;
    if (ctx->handle == NULL) return;
    libusb_release_interface(ctx->handle, ctx->claimed_interface);
    libusb_close(ctx->handle);
    ctx->handle = NULL;
}

static const struct cb62_usb_ops operations = {
    .discover = discover,
    .open_claim = open_claim,
    .bulk_out = bulk_out,
    .bulk_in = bulk_in,
    .close = close_device,
};

int cb62_libusb_init(struct cb62_libusb **out)
{
    struct cb62_libusb *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) return -1;
    int rc = libusb_init(&ctx->usb);
    if (rc != LIBUSB_SUCCESS) {
        free(ctx);
        return -1;
    }
    *out = ctx;
    return 0;
}

void cb62_libusb_destroy(struct cb62_libusb *ctx)
{
    if (ctx == NULL) return;
    close_device(ctx);
    libusb_exit(ctx->usb);
    free(ctx);
}

const struct cb62_usb_ops *cb62_libusb_ops(void)
{
    return &operations;
}

const char *cb62_libusb_last_error(const struct cb62_libusb *ctx)
{
    return ctx->error[0] ? ctx->error : "unspecified libusb failure";
}
