#include "libusb_backend.h"
#include "probe.h"
#include "protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void print_bytes(const char *label, const uint8_t *bytes, size_t length)
{
    printf("%s (%zu bytes):", label, length);
    for (size_t i = 0; i < length; i++) printf(" %02x", bytes[i]);
    putchar('\n');
}

static void usage(FILE *stream)
{
    fputs("Usage: cb62-inquiry-probe --execute-one-inquiry\n"
          "Performs exactly one fixed inquiry against the allowlisted "
          "CB62 recovery identity.\n",
          stream);
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage(stdout);
        return 0;
    }
    if (argc != 2 || strcmp(argv[1], "--execute-one-inquiry") != 0) {
        usage(stderr);
        return 2;
    }

    uint8_t request[CB62_INQUIRY_REQUEST_SIZE];
    cb62_build_inquiry_request(request);
    print_bytes("fixed request", request, sizeof(request));

    struct cb62_libusb *usb = NULL;
    if (cb62_libusb_init(&usb) != 0) {
        fputs("result: libusb initialization failed\n", stderr);
        return 1;
    }

    struct cb62_probe_observation observation;
    int rc = cb62_probe_once(cb62_libusb_ops(), usb, &observation);
    if (observation.device.vid != 0) {
        printf("device: bus=%u address=%u vid:pid=%04x:%04x\n",
               observation.device.bus, observation.device.address,
               observation.device.vid, observation.device.pid);
        printf("manufacturer: %s\nproduct: %s\nserial: %s\n",
               observation.device.manufacturer,
               observation.device.product,
               observation.device.serial);
        printf("interface: %02x/%02x/%02x number=%u matches=%u "
               "bulk_endpoints=%u out=%02x in=%02x\n",
               observation.device.interface_class,
               observation.device.interface_subclass,
               observation.device.interface_protocol,
               observation.device.interface_number,
               observation.device.matching_interface_count,
               observation.device.bulk_endpoint_count,
               observation.device.endpoint_out,
               observation.device.endpoint_in);
    }
    if (observation.response_transferred != 0)
        print_bytes("response", observation.response_wire,
                    observation.response_transferred);
    if (rc != CB62_PROBE_OK) {
        fprintf(stderr, "result: inquiry refused or failed (%d): %s\n",
                rc, cb62_libusb_last_error(usb));
        cb62_libusb_destroy(usb);
        return 1;
    }

    printf("decoded: signature=0x%08x status=%u chip_id=%u (0x%08x) p1=0x%08x\n",
           observation.response.signature, observation.response.status,
           observation.response.chip_id, observation.response.chip_id,
           observation.response.p1);
    cb62_libusb_destroy(usb);
    if (observation.response.chip_id != CB62_EXPECTED_CHIP_ID) {
        fprintf(stderr, "result: valid response, unexpected chip ID "
                "(expected 0x%08x)\n", CB62_EXPECTED_CHIP_ID);
        return 3;
    }
    puts("result: CV22 inquiry matched expected chip ID; no further transaction sent");
    return 0;
}
