#include "addr_probe.h"

#include <string.h>

static int identity_allowed(const struct cb62_usb_device *device)
{
    return device->vid == CB62_USB_VID &&
           device->pid == CB62_USB_PID &&
           strcmp(device->manufacturer, CB62_USB_MANUFACTURER) == 0 &&
           strcmp(device->product, CB62_USB_PRODUCT) == 0;
}

static int topology_allowed(const struct cb62_usb_device *device)
{
    return device->matching_interface_count == 1 &&
           device->bulk_endpoint_count == 2 &&
           device->interface_class == CB62_USB_INTERFACE_CLASS &&
           device->interface_subclass == CB62_USB_INTERFACE_SUBCLASS &&
           device->interface_protocol == CB62_USB_INTERFACE_PROTOCOL &&
           device->endpoint_out == CB62_USB_ENDPOINT_OUT &&
           device->endpoint_in == CB62_USB_ENDPOINT_IN;
}

int cb62_probe_addr_once(const struct cb62_usb_ops *ops, void *ctx,
                         struct cb62_addr_probe_observation *observation)
{
    struct cb62_usb_device devices[2];
    size_t count = 0;
    memset(observation, 0, sizeof(*observation));
    memset(devices, 0, sizeof(devices));

    if (ops->discover(ctx, devices, 2, &count) != 0)
        return CB62_ADDR_PROBE_DISCOVERY_FAILED;
    if (count != 1) return CB62_ADDR_PROBE_DEVICE_COUNT_REJECTED;
    observation->device = devices[0];
    if (!identity_allowed(&observation->device))
        return CB62_ADDR_PROBE_IDENTITY_REJECTED;
    if (!topology_allowed(&observation->device))
        return CB62_ADDR_PROBE_TOPOLOGY_REJECTED;

    cb62_build_addr_inquiry_request(observation->request);
    if (ops->open_claim(ctx, &observation->device) != 0)
        return CB62_ADDR_PROBE_OPEN_FAILED;

    int result = CB62_ADDR_PROBE_OUT_FAILED;
    if (ops->bulk_out(ctx, CB62_USB_ENDPOINT_OUT, observation->request,
                      sizeof(observation->request),
                      &observation->request_transferred) != 0 ||
        observation->request_transferred != sizeof(observation->request))
        goto done;

    result = CB62_ADDR_PROBE_IN_FAILED;
    if (ops->bulk_in(ctx, CB62_USB_ENDPOINT_IN, observation->response_wire,
                     sizeof(observation->response_wire),
                     &observation->response_transferred) != 0 ||
        observation->response_transferred != sizeof(observation->response_wire))
        goto done;

    result = CB62_ADDR_PROBE_RESPONSE_REJECTED;
    if (cb62_parse_addr_inquiry_response(observation->response_wire,
                                         observation->response_transferred,
                                         &observation->response) !=
        CB62_ADDR_PARSE_OK)
        goto done;
    result = CB62_ADDR_PROBE_OK;

done:
    ops->close(ctx);
    return result;
}
