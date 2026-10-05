#include "ptb_probe.h"

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
           device->interface_number == 0 &&
           device->bulk_endpoint_count == 2 &&
           device->interface_class == CB62_USB_INTERFACE_CLASS &&
           device->interface_subclass == CB62_USB_INTERFACE_SUBCLASS &&
           device->interface_protocol == CB62_USB_INTERFACE_PROTOCOL &&
           device->endpoint_out == CB62_USB_ENDPOINT_OUT &&
           device->endpoint_in == CB62_USB_ENDPOINT_IN;
}

int cb62_probe_fixed_ptb_once(
    const struct cb62_usb_ops *ops, void *ctx,
    struct cb62_ptb_probe_observation *observation)
{
    struct cb62_usb_device devices[2];
    size_t count = 0;
    int result;

    memset(observation, 0, sizeof(*observation));
    memset(devices, 0, sizeof(devices));
    if (ops->discover(ctx, devices, 2, &count) != 0)
        return CB62_PTB_PROBE_DISCOVERY_FAILED;
    if (count != 1) return CB62_PTB_PROBE_DEVICE_COUNT_REJECTED;
    observation->device = devices[0];
    if (!identity_allowed(&observation->device))
        return CB62_PTB_PROBE_IDENTITY_REJECTED;
    if (!topology_allowed(&observation->device))
        return CB62_PTB_PROBE_TOPOLOGY_REJECTED;

    cb62_build_fixed_ptb_ready_request(observation->ready_request);
    cb62_build_ptb_send_data_request(observation->send_data_request);
    if (ops->open_claim(ctx, &observation->device) != 0)
        return CB62_PTB_PROBE_OPEN_FAILED;

    result = CB62_PTB_PROBE_READY_OUT_FAILED;
    if (ops->bulk_out(ctx, CB62_USB_ENDPOINT_OUT, observation->ready_request,
                      sizeof(observation->ready_request),
                      &observation->ready_request_transferred) != 0 ||
        observation->ready_request_transferred !=
            sizeof(observation->ready_request))
        goto done;

    result = CB62_PTB_PROBE_READY_IN_FAILED;
    if (ops->bulk_in(ctx, CB62_USB_ENDPOINT_IN,
                     observation->ready_response_wire,
                     sizeof(observation->ready_response_wire),
                     &observation->ready_response_transferred) != 0 ||
        observation->ready_response_transferred !=
            sizeof(observation->ready_response_wire))
        goto done;

    result = CB62_PTB_PROBE_READY_REJECTED;
    if (cb62_parse_ptb_ready_response(
            observation->ready_response_wire,
            observation->ready_response_transferred,
            &observation->ready) != CB62_PTB_PARSE_OK)
        goto done;

    result = CB62_PTB_PROBE_SEND_DATA_OUT_FAILED;
    if (ops->bulk_out(ctx, CB62_USB_ENDPOINT_OUT,
                      observation->send_data_request,
                      sizeof(observation->send_data_request),
                      &observation->send_data_request_transferred) != 0 ||
        observation->send_data_request_transferred !=
            sizeof(observation->send_data_request))
        goto done;

    result = CB62_PTB_PROBE_DATA_IN_FAILED;
    if (ops->bulk_in(ctx, CB62_USB_ENDPOINT_IN, observation->data,
                     sizeof(observation->data),
                     &observation->data_transferred) != 0 ||
        observation->data_transferred != sizeof(observation->data))
        goto done;

    observation->computed_crc32 =
        cb62_ptb_crc32(observation->data, observation->data_transferred);
    result = observation->computed_crc32 == observation->ready.crc32
                 ? CB62_PTB_PROBE_OK
                 : CB62_PTB_PROBE_CRC_MISMATCH;

done:
    ops->close(ctx);
    return result;
}
