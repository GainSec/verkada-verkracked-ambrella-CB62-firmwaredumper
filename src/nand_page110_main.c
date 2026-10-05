#include "libusb_backend.h"
#include "nand_probe.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "--execute-fixed-page110-uart-proof") != 0) {
        fputs("Usage: cb62-nand-page110-uart-proof --execute-fixed-page110-uart-proof\n", stderr);
        return 2;
    }
    struct cb62_libusb *usb = NULL;
    struct cb62_nand_observation observation;
    if (cb62_libusb_init(&usb) != 0) {
        fputs("libusb init failed\n", stderr);
        return 1;
    }
    int result = cb62_upload_fixed_nand_page0_once(
        cb62_libusb_ops(), usb, &observation);
    if (result != CB62_NAND_OK) {
        fprintf(stderr, "result=%d error=%s out=%u in=%u; no retry sent\n",
                result, cb62_libusb_last_error(usb),
                observation.out_transfers, observation.in_transfers);
        cb62_libusb_destroy(usb);
        return 1;
    }
    printf("uploaded fixed page-110 payload: address=0x%08x bytes=%u "
           "crc32=0x%08x; jump sent once; UART frame expected\n",
           CB62_NAND_PAYLOAD_ADDRESS, observation.payload_length,
           observation.payload_crc32);
    cb62_libusb_destroy(usb);
    return 0;
}
