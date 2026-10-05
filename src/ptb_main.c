#include "libusb_backend.h"
#include "ptb_probe.h"
#include "ptb_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_bytes(const char *label, const uint8_t *bytes, size_t length)
{
    printf("%s (%zu bytes):", label, length);
    for (size_t i = 0; i < length; i++) printf(" %02x", bytes[i]);
    putchar('\n');
}

static void usage(FILE *stream)
{
    fputs("Usage: cb62-amboot-ptb-snapshot-probe "
          "--execute-one-fixed-ptb-snapshot-read --output FILE\n"
          "Performs one read-only Ambarella transaction using fixed selector "
          "16 and fixed length 0x1000.\n"
          "FILE must not already exist. No retry or trailing command is sent.\n",
          stream);
}

static int remove_failed_output(FILE *output, const char *path)
{
    if (output != NULL) fclose(output);
    remove(path);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage(stdout);
        return 0;
    }
    if (argc != 4 ||
        strcmp(argv[1], "--execute-one-fixed-ptb-snapshot-read") != 0 ||
        strcmp(argv[2], "--output") != 0 || argv[3][0] == '\0') {
        usage(stderr);
        fputs("result: unsupported invocation; no transaction sent\n", stderr);
        return 2;
    }

    FILE *output = fopen(argv[3], "wbx");
    if (output == NULL) {
        fprintf(stderr, "result: output must be a new creatable file: %s; "
                "no transaction sent\n", argv[3]);
        return 2;
    }

    struct cb62_ptb_probe_observation *observation =
        calloc(1u, sizeof(*observation));
    if (observation == NULL) {
        fputs("result: observation allocation failed; no transaction sent\n",
              stderr);
        return remove_failed_output(output, argv[3]);
    }

    uint8_t request[CB62_PTB_COMMAND_SIZE];
    cb62_build_fixed_ptb_ready_request(request);
    print_bytes("fixed PTB ready-to-send request", request, sizeof(request));

    struct cb62_libusb *usb = NULL;
    if (cb62_libusb_init(&usb) != 0) {
        free(observation);
        fputs("result: libusb initialization failed; no transaction sent\n",
              stderr);
        return remove_failed_output(output, argv[3]);
    }

    int result = cb62_probe_fixed_ptb_once(cb62_libusb_ops(), usb,
                                            observation);
    if (observation->device.vid != 0) {
        printf("device: bus=%u address=%u vid:pid=%04x:%04x\n",
               observation->device.bus, observation->device.address,
               observation->device.vid, observation->device.pid);
        printf("manufacturer: %s\nproduct: %s\nserial: %s\n",
               observation->device.manufacturer,
               observation->device.product,
               observation->device.serial);
        printf("interface: %02x/%02x/%02x number=%u matches=%u "
               "bulk_endpoints=%u out=%02x in=%02x\n",
               observation->device.interface_class,
               observation->device.interface_subclass,
               observation->device.interface_protocol,
               observation->device.interface_number,
               observation->device.matching_interface_count,
               observation->device.bulk_endpoint_count,
               observation->device.endpoint_out,
               observation->device.endpoint_in);
    }
    if (observation->ready_response_transferred != 0)
        print_bytes("ready response", observation->ready_response_wire,
                    observation->ready_response_transferred);
    if (observation->data_transferred >= 64u)
        print_bytes("data prefix", observation->data, 64u);

    if (result != CB62_PTB_PROBE_OK) {
        fprintf(stderr, "result: fixed PTB snapshot read refused or failed "
                "(%d): %s; no retry or trailing command sent\n",
                result, cb62_libusb_last_error(usb));
        cb62_libusb_destroy(usb);
        free(observation);
        return remove_failed_output(output, argv[3]);
    }

    printf("decoded: signature=0x%08x status=%u length=%u "
           "device_crc32=0x%08x host_crc32=0x%08x\n",
           observation->ready.signature, observation->ready.status,
           observation->ready.length, observation->ready.crc32,
           observation->computed_crc32);
    cb62_libusb_destroy(usb);

    size_t magic_count = cb62_ptb_count_record_magics(
        observation->data, sizeof(observation->data));
    size_t written = fwrite(observation->data, 1, sizeof(observation->data),
                            output);
    int close_result = fclose(output);
    if (written != sizeof(observation->data) || close_result != 0) {
        free(observation);
        remove(argv[3]);
        fputs("result: verified data received but output write failed; "
              "partial file removed\n", stderr);
        return 1;
    }

    free(observation);
    printf("ptb-snapshot: selector=%u bytes=%u record_magics=%zu output=%s\n",
           CB62_PTB_FIXED_SELECTOR, CB62_PTB_FIXED_LENGTH, magic_count,
           argv[3]);
    puts("result: CRC-valid fixed PTB snapshot saved; no further transaction "
         "sent");
    return 0;
}
