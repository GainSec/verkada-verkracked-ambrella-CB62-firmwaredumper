#include "nand_protocol.h"

#include <string.h>

static void put_u32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void begin(uint8_t *out, uint32_t command)
{
    memset(out, 0, CB62_NAND_COMMAND_SIZE);
    put_u32le(out, CB62_NAND_COMMAND_TOKEN);
    put_u32le(out + 4, command);
}

void cb62_nand_build_address_command(uint8_t out[CB62_NAND_COMMAND_SIZE])
{
    begin(out, CB62_NAND_READY_TO_RECEIVE);
    put_u32le(out + 8, CB62_NAND_FLAG_ADDRESS);
    put_u32le(out + 12, CB62_NAND_PAYLOAD_ADDRESS);
}

void cb62_nand_build_data_command(uint8_t out[CB62_NAND_COMMAND_SIZE],
                                  uint32_t crc)
{
    begin(out, CB62_NAND_RECEIVE_DATA);
    put_u32le(out + 8, crc);
}

void cb62_nand_build_jump_command(uint8_t out[CB62_NAND_COMMAND_SIZE])
{
    begin(out, CB62_NAND_READY_TO_RECEIVE);
    put_u32le(out + 8, CB62_NAND_FLAG_JUMP_EXEC |
              CB62_NAND_FLAG_LAST_TRANS | CB62_NAND_FLAG_FORCE_FINISH);
    put_u32le(out + 12, CB62_NAND_PAYLOAD_ADDRESS);
    put_u32le(out + 16, CB62_NAND_PAYLOAD_ADDRESS);
}

uint32_t cb62_nand_crc32(const uint8_t *data, size_t length)
{
    uint32_t c = 0xffffffffu;
    for (size_t i = 0; i < length; i++) {
        c ^= data[i];
        for (unsigned j = 0; j < 8; j++)
            c = (c >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(c & 1u));
    }
    return ~c;
}
