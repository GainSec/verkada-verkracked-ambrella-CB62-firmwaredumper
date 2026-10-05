#ifndef CB62_NAND_PROBE_H
#define CB62_NAND_PROBE_H

#include "nand_protocol.h"
#include "probe.h"

#define CB62_NAND_MAX_TRANSFER 4096u

struct cb62_nand_observation {
    struct cb62_usb_device device;
    uint32_t payload_length;
    uint32_t payload_crc32;
    unsigned out_transfers;
    unsigned in_transfers;
};

enum cb62_nand_result {
    CB62_NAND_OK=0, CB62_NAND_DISCOVERY=-40, CB62_NAND_COUNT=-41,
    CB62_NAND_IDENTITY=-42, CB62_NAND_TOPOLOGY=-43, CB62_NAND_OPEN=-44,
    CB62_NAND_ADDRESS_OUT=-45, CB62_NAND_ADDRESS_RSP=-46,
    CB62_NAND_DATA_CMD_OUT=-47, CB62_NAND_PAYLOAD_OUT=-48,
    CB62_NAND_PAYLOAD_RSP=-49, CB62_NAND_JUMP_OUT=-50,
    CB62_NAND_JUMP_RSP=-51
};

int cb62_upload_fixed_nand_page0_once(const struct cb62_usb_ops *ops,
    void *ctx, struct cb62_nand_observation *observation);

#endif
