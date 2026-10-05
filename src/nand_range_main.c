#include "libusb_backend.h"
#include "nand_endurance_probe.h"

#include <stdio.h>
#include <string.h>

#ifndef RANGE_START_PAGE
#error fixed RANGE_START_PAGE required
#endif
#ifndef RANGE_PAGE_COUNT
#error fixed RANGE_PAGE_COUNT required
#endif
#ifndef TARGET_UART_REQUESTED_BAUD
#error fixed TARGET_UART_REQUESTED_BAUD required
#endif
#ifndef TARGET_PROFILE_NAME
#error fixed TARGET_PROFILE_NAME required
#endif

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)

int main(int argc, char **argv)
{
    static const char token[] = "--execute-fixed-range-" TARGET_PROFILE_NAME
        "-" STRINGIFY(RANGE_START_PAGE) "-" STRINGIFY(RANGE_PAGE_COUNT);
    struct cb62_libusb *usb = NULL;
    struct cb62_nand_observation observation;

    if (argc != 2 || strcmp(argv[1], token) != 0) {
        fprintf(stderr, "Usage: cb62-nand-range-%s-%s-%s %s\n",
                TARGET_PROFILE_NAME, STRINGIFY(RANGE_START_PAGE),
                STRINGIFY(RANGE_PAGE_COUNT), token);
        return 2;
    }
    if (cb62_libusb_init(&usb) != 0) return 1;
    int result = cb62_upload_fixed_nand_endurance_once(
        cb62_libusb_ops(), usb, &observation);
    if (result != CB62_NAND_OK) {
        fprintf(stderr, "result=%d error=%s out=%u in=%u; no retry sent\n",
                result, cb62_libusb_last_error(usb),
                observation.out_transfers, observation.in_transfers);
        cb62_libusb_destroy(usb);
        return 1;
    }
    printf("uploaded fixed NAND range %u..%u with profile %s: "
           "address=0x%08x bytes=%u crc32=0x%08x; jump sent once; "
           "target UART requested %u baud\n",
           (unsigned)RANGE_START_PAGE,
           (unsigned)(RANGE_START_PAGE + RANGE_PAGE_COUNT - 1u),
           TARGET_PROFILE_NAME, CB62_NAND_PAYLOAD_ADDRESS,
           observation.payload_length, observation.payload_crc32,
           (unsigned)TARGET_UART_REQUESTED_BAUD);
    cb62_libusb_destroy(usb);
    return 0;
}
