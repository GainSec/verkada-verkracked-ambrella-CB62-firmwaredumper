#include "../src/nand_protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); failures++; } } while (0)

static uint32_t get_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int main(void)
{
    uint8_t command[CB62_NAND_COMMAND_SIZE];
    cb62_nand_build_address_command(command);
    CHECK(get_u32le(command + 0) == CB62_NAND_COMMAND_TOKEN);
    CHECK(get_u32le(command + 4) == CB62_NAND_READY_TO_RECEIVE);
    CHECK(get_u32le(command + 8) == CB62_NAND_FLAG_ADDRESS);
    CHECK(get_u32le(command + 12) == CB62_NAND_PAYLOAD_ADDRESS);

    cb62_nand_build_data_command(command, 0x11223344u);
    CHECK(get_u32le(command + 4) == CB62_NAND_RECEIVE_DATA);
    CHECK(get_u32le(command + 8) == 0x11223344u);

    cb62_nand_build_jump_command(command);
    CHECK(get_u32le(command + 8) == (CB62_NAND_FLAG_JUMP_EXEC |
          CB62_NAND_FLAG_LAST_TRANS | CB62_NAND_FLAG_FORCE_FINISH));
    CHECK(get_u32le(command + 12) == CB62_NAND_PAYLOAD_ADDRESS);
    CHECK(get_u32le(command + 16) == CB62_NAND_PAYLOAD_ADDRESS);

    CHECK(cb62_nand_crc32((const uint8_t *)"123456789", 9) == 0xcbf43926u);
    if (failures) return 1;
    puts("test_nand_protocol: PASS");
    return 0;
}
