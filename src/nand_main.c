#include "libusb_backend.h"
#include "nand_payload_blob.h"
#include "nand_probe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "--execute-fixed-page0-uart-proof") != 0) {
        fputs("Usage: cb62-nand-page0-uart-proof --execute-fixed-page0-uart-proof\n", stderr);
        return 2;
    }
    struct cb62_libusb *usb=NULL; struct cb62_nand_observation o;
    if (cb62_libusb_init(&usb)!=0) { fputs("libusb init failed\n",stderr); return 1; }
    int result=cb62_upload_fixed_nand_page0_once(cb62_libusb_ops(),usb,&o);
    if (result!=CB62_NAND_OK) {
        fprintf(stderr,"result=%d error=%s out=%u in=%u; no retry sent\n",
                result,cb62_libusb_last_error(usb),o.out_transfers,o.in_transfers);
        cb62_libusb_destroy(usb); return 1;
    }
    printf("uploaded fixed payload: address=0x%08x bytes=%u crc32=0x%08x; jump sent once; UART frame expected\n",
           CB62_NAND_PAYLOAD_ADDRESS,o.payload_length,o.payload_crc32);
    cb62_libusb_destroy(usb); return 0;
}
