#include "libusb_backend.h"
#include "nand_endurance_probe.h"

#include <stdio.h>
#include <string.h>

#ifndef ENDURANCE_PAGE_COUNT
#error fixed ENDURANCE_PAGE_COUNT required
#endif

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)

int main(int argc, char **argv)
{
    static const char token[] = "--execute-fixed-endurance-" STRINGIFY(ENDURANCE_PAGE_COUNT);
    struct cb62_libusb *usb = NULL;
    struct cb62_nand_observation observation;
    if (argc != 2 || strcmp(argv[1], token) != 0) {
        fprintf(stderr, "Usage: cb62-nand-endurance-%s %s\n", STRINGIFY(ENDURANCE_PAGE_COUNT), token);
        return 2;
    }
    if (cb62_libusb_init(&usb) != 0) return 1;
    int result = cb62_upload_fixed_nand_endurance_once(cb62_libusb_ops(), usb, &observation);
    if (result != CB62_NAND_OK) {
        fprintf(stderr, "result=%d error=%s out=%u in=%u; no retry sent\n", result,
                cb62_libusb_last_error(usb), observation.out_transfers, observation.in_transfers);
        cb62_libusb_destroy(usb); return 1;
    }
    printf("uploaded fixed %s-page endurance payload: address=0x%08x bytes=%u crc32=0x%08x; jump sent once\n",
           STRINGIFY(ENDURANCE_PAGE_COUNT), CB62_NAND_PAYLOAD_ADDRESS,
           observation.payload_length, observation.payload_crc32);
    cb62_libusb_destroy(usb);
    return 0;
}
