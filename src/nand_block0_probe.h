#ifndef CB62_NAND_BLOCK0_PROBE_H
#define CB62_NAND_BLOCK0_PROBE_H

#include "nand_probe.h"

int cb62_upload_fixed_nand_block0_once(const struct cb62_usb_ops *ops,
    void *ctx, struct cb62_nand_observation *observation);

#endif
