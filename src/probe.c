#include "probe.h"

#include <string.h>

static int identity_allowed(const struct cb62_usb_device *d)
{
    return d->vid == CB62_USB_VID &&
           d->pid == CB62_USB_PID &&
           strcmp(d->manufacturer, CB62_USB_MANUFACTURER) == 0 &&
           strcmp(d->product, CB62_USB_PRODUCT) == 0;
}

static int topology_allowed(const struct cb62_usb_device *d)
{
    return d->matching_interface_count == 1 &&
           d->bulk_endpoint_count == 2 &&
           d->interface_class == CB62_USB_INTERFACE_CLASS &&
           d->interface_subclass == CB62_USB_INTERFACE_SUBCLASS &&
           d->interface_protocol == CB62_USB_INTERFACE_PROTOCOL &&
           d->endpoint_out == CB62_USB_ENDPOINT_OUT &&
           d->endpoint_in == CB62_USB_ENDPOINT_IN;
}

int cb62_probe_once(const struct cb62_usb_ops *ops, void *ctx,
                    struct cb62_probe_observation *obs)
{
    struct cb62_usb_device devices[2];
    size_t count = 0;
    memset(obs, 0, sizeof(*obs));
    memset(devices, 0, sizeof(devices));

    if (ops->discover(ctx, devices, 2, &count) != 0)
        return CB62_PROBE_DISCOVERY_FAILED;
    if (count != 1) return CB62_PROBE_DEVICE_COUNT_REJECTED;
    obs->device = devices[0];
    if (!identity_allowed(&obs->device)) return CB62_PROBE_IDENTITY_REJECTED;
    if (!topology_allowed(&obs->device)) return CB62_PROBE_TOPOLOGY_REJECTED;

    cb62_build_inquiry_request(obs->request);
    if (ops->open_claim(ctx, &obs->device) != 0) return CB62_PROBE_OPEN_FAILED;

    int rc = CB62_PROBE_OUT_FAILED;
    if (ops->bulk_out(ctx, CB62_USB_ENDPOINT_OUT, obs->request,
                      sizeof(obs->request), &obs->request_transferred) != 0 ||
        obs->request_transferred != sizeof(obs->request))
        goto done;

    rc = CB62_PROBE_IN_FAILED;
    if (ops->bulk_in(ctx, CB62_USB_ENDPOINT_IN, obs->response_wire,
                     sizeof(obs->response_wire), &obs->response_transferred) != 0 ||
        obs->response_transferred != sizeof(obs->response_wire))
        goto done;

    rc = CB62_PROBE_RESPONSE_REJECTED;
    if (cb62_parse_inquiry_response(obs->response_wire,
                                    obs->response_transferred,
                                    &obs->response) != CB62_PARSE_OK)
        goto done;
    rc = CB62_PROBE_OK;

done:
    ops->close(ctx);
    return rc;
}
