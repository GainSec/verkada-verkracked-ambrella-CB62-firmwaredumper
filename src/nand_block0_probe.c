#include "nand_block0_probe.h"
#include "nand_block0_payload_blob.h"

#include <string.h>

#define STATUS_TOKEN 0x55525350u

static uint32_t u32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int response_ok(const uint8_t *p, size_t n)
{
    return n == 16u && u32le(p) == STATUS_TOKEN && u32le(p + 4) == 0u;
}

static int identity_ok(const struct cb62_usb_device *d)
{
    return d->vid == CB62_USB_VID && d->pid == CB62_USB_PID &&
           strcmp(d->manufacturer, CB62_USB_MANUFACTURER) == 0 &&
           strcmp(d->product, CB62_USB_PRODUCT) == 0;
}

static int topology_ok(const struct cb62_usb_device *d)
{
    return d->matching_interface_count == 1 && d->interface_number == 0 &&
           d->bulk_endpoint_count == 2 && d->interface_class == 0xff &&
           d->interface_subclass == 0xff && d->interface_protocol == 0 &&
           d->endpoint_out == 1 && d->endpoint_in == 0x81;
}

int cb62_upload_fixed_nand_block0_once(const struct cb62_usb_ops *ops,
    void *ctx, struct cb62_nand_observation *o)
{
    struct cb62_usb_device devices[2];
    size_t count = 0, done = 0;
    int result;
    uint8_t command[CB62_NAND_COMMAND_SIZE], response[16];

    memset(o, 0, sizeof(*o));
    memset(devices, 0, sizeof(devices));
    if (ops->discover(ctx, devices, 2, &count) != 0) return CB62_NAND_DISCOVERY;
    if (count != 1) return CB62_NAND_COUNT;
    o->device = devices[0];
    if (!identity_ok(&o->device)) return CB62_NAND_IDENTITY;
    if (!topology_ok(&o->device)) return CB62_NAND_TOPOLOGY;
    o->payload_length = cb62_nand_block0_payload_len;
    o->payload_crc32 = cb62_nand_crc32(cb62_nand_block0_payload,
                                      o->payload_length);
    if (ops->open_claim(ctx, &o->device) != 0) return CB62_NAND_OPEN;

    cb62_nand_build_address_command(command);
    result = CB62_NAND_ADDRESS_OUT;
    if (ops->bulk_out(ctx, 1, command, sizeof(command), &done) != 0 ||
        done != sizeof(command)) goto out;
    o->out_transfers++;
    result = CB62_NAND_ADDRESS_RSP;
    if (ops->bulk_in(ctx, 0x81, response, sizeof(response), &done) != 0 ||
        !response_ok(response, done)) goto out;
    o->in_transfers++;

    cb62_nand_build_data_command(command, o->payload_crc32);
    result = CB62_NAND_DATA_CMD_OUT;
    if (ops->bulk_out(ctx, 1, command, sizeof(command), &done) != 0 ||
        done != sizeof(command)) goto out;
    o->out_transfers++;
    result = CB62_NAND_PAYLOAD_OUT;
    if (ops->bulk_out(ctx, 1, cb62_nand_block0_payload,
                      o->payload_length, &done) != 0 ||
        done != o->payload_length) goto out;
    o->out_transfers++;
    result = CB62_NAND_PAYLOAD_RSP;
    if (ops->bulk_in(ctx, 0x81, response, sizeof(response), &done) != 0 ||
        !response_ok(response, done)) goto out;
    o->in_transfers++;

    cb62_nand_build_jump_command(command);
    result = CB62_NAND_JUMP_OUT;
    if (ops->bulk_out(ctx, 1, command, sizeof(command), &done) != 0 ||
        done != sizeof(command)) goto out;
    o->out_transfers++;
    result = CB62_NAND_JUMP_RSP;
    if (ops->bulk_in(ctx, 0x81, response, sizeof(response), &done) != 0 ||
        !response_ok(response, done)) goto out;
    o->in_transfers++;
    result = CB62_NAND_OK;

out:
    ops->close(ctx);
    return result;
}
