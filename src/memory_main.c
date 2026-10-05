#include "libusb_backend.h"
#include "memory_probe.h"
#include "memory_protocol.h"

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
    fputs("Usage: cb62-amboot-header-probe --execute-one-read\n"
          "Performs exactly one fixed 32-bit read at 0x01400038 against "
          "the allowlisted CB62 recovery identity.\n"
          "After that read, no further transaction is sent.\n",
          stream);
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage(stdout);
        return 0;
    }
    if (argc != 2 || strcmp(argv[1], "--execute-one-read") != 0) {
        usage(stderr);
        fputs("result: unsupported invocation; no transaction sent\n", stderr);
        return 2;
    }

    uint8_t request[CB62_MEMORY_READ_REQUEST_SIZE];
    cb62_build_memory_read_request(request);
    print_bytes("fixed request", request, sizeof(request));

    struct cb62_libusb *usb = NULL;
    if (cb62_libusb_init(&usb) != 0) {
        fputs("result: libusb initialization failed; no transaction sent\n",
              stderr);
        return 1;
    }

    struct cb62_memory_probe_observation observation;
    int result = cb62_probe_memory_once(cb62_libusb_ops(), usb, &observation);
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
    if (result != CB62_MEMORY_PROBE_OK) {
        fprintf(stderr, "result: fixed read refused or failed (%d): %s; "
                "no further transaction sent\n",
                result, cb62_libusb_last_error(usb));
        cb62_libusb_destroy(usb);
        return 1;
    }

    printf("decoded: signature=0x%08x status=%u value=0x%08x "
           "p1=0x%08x expected=0x%08x\n",
           observation.response.signature, observation.response.status,
           observation.response.value, observation.response.p1,
           CB62_MEMORY_EXPECTED_VALUE);
    cb62_libusb_destroy(usb);
    if (observation.response.value != CB62_MEMORY_EXPECTED_VALUE) {
        fprintf(stderr, "result: valid response, unexpected value "
                "(expected 0x%08x); no further transaction sent\n",
                CB62_MEMORY_EXPECTED_VALUE);
        return 3;
    }
    puts("result: fixed AMBOOT header read matched expected value; "
         "no further transaction sent");
    return 0;
}
