#include "../src/block_protocol.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put_u32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void test_fixed_ready_to_send_request(void)
{
    static const uint8_t expected[CB62_BLOCK_COMMAND_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x40, 0x01, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    uint8_t actual[CB62_BLOCK_COMMAND_SIZE];
    memset(actual, 0xa5, sizeof(actual));
    cb62_build_fixed_block_ready_request(actual);
    assert(memcmp(actual, expected, sizeof(expected)) == 0);
}

static void test_send_data_request(void)
{
    static const uint8_t expected[CB62_BLOCK_COMMAND_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    uint8_t actual[CB62_BLOCK_COMMAND_SIZE];
    memset(actual, 0xa5, sizeof(actual));
    cb62_build_block_send_data_request(actual);
    assert(memcmp(actual, expected, sizeof(expected)) == 0);
}

static void test_ready_response(void)
{
    uint8_t wire[CB62_BLOCK_RESPONSE_SIZE] = {0};
    struct cb62_block_ready_response response;
    put_u32le(wire + 0, CB62_BLOCK_STATUS_TOKEN);
    put_u32le(wire + 4, CB62_BLOCK_RESPONSE_SUCCESS);
    put_u32le(wire + 8, CB62_BLOCK_FIXED_LENGTH);
    put_u32le(wire + 12, 0xfa9e17fau);
    assert(cb62_parse_block_ready_response(wire, sizeof(wire), &response) ==
           CB62_BLOCK_PARSE_OK);
    assert(response.length == CB62_BLOCK_FIXED_LENGTH);
    assert(response.crc32 == 0xfa9e17fau);

    wire[0] = 0;
    assert(cb62_parse_block_ready_response(wire, sizeof(wire), &response) ==
           CB62_BLOCK_PARSE_BAD_SIGNATURE);
    wire[0] = 0x50;
    wire[4] = 1;
    assert(cb62_parse_block_ready_response(wire, sizeof(wire), &response) ==
           CB62_BLOCK_PARSE_DEVICE_FAILURE);
    wire[4] = 0;
    put_u32le(wire + 8, CB62_BLOCK_FIXED_LENGTH - 1);
    assert(cb62_parse_block_ready_response(wire, sizeof(wire), &response) ==
           CB62_BLOCK_PARSE_WRONG_DATA_LENGTH);
    assert(cb62_parse_block_ready_response(wire, sizeof(wire) - 1, &response) ==
           CB62_BLOCK_PARSE_WRONG_RESPONSE_LENGTH);
}

static void test_crc32(void)
{
    static const uint8_t header[CB62_BLOCK_FIXED_LENGTH] = {
        0x18, 0x00, 0x00, 0x14,
        [0x38] = 0x09, [0x39] = 0x03,
    };
    assert(cb62_block_crc32(header, sizeof(header)) == 0xfa9e17fau);
}

int main(void)
{
    test_fixed_ready_to_send_request();
    test_send_data_request();
    test_ready_response();
    test_crc32();
    puts("test_block_protocol: PASS");
    return 0;
}
