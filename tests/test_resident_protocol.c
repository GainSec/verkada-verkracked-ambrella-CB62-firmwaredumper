#include "../src/resident_protocol.h"

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

static void test_fixed_resident_request(void)
{
    static const uint8_t expected[CB62_RESIDENT_COMMAND_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x40, 0x01, 0x00, 0x40, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    uint8_t actual[CB62_RESIDENT_COMMAND_SIZE];
    memset(actual, 0xa5, sizeof(actual));
    cb62_build_fixed_resident_ready_request(actual);
    assert(memcmp(actual, expected, sizeof(expected)) == 0);
}

static void test_send_data_request(void)
{
    static const uint8_t expected[CB62_RESIDENT_COMMAND_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x03, 0x00, 0x00, 0x00,
    };
    uint8_t actual[CB62_RESIDENT_COMMAND_SIZE];
    memset(actual, 0xa5, sizeof(actual));
    cb62_build_resident_send_data_request(actual);
    assert(memcmp(actual, expected, sizeof(expected)) == 0);
}

static void test_ready_response_requires_exact_length(void)
{
    uint8_t wire[CB62_RESIDENT_RESPONSE_SIZE] = {0};
    struct cb62_resident_ready_response response;
    put_u32le(wire + 0, CB62_RESIDENT_STATUS_TOKEN);
    put_u32le(wire + 4, CB62_RESIDENT_RESPONSE_SUCCESS);
    put_u32le(wire + 8, CB62_RESIDENT_FIXED_LENGTH);
    put_u32le(wire + 12, 0x12345678u);
    assert(cb62_parse_resident_ready_response(wire, sizeof(wire), &response) ==
           CB62_RESIDENT_PARSE_OK);
    assert(response.length == 0x00034000u);
    assert(response.crc32 == 0x12345678u);
    put_u32le(wire + 8, CB62_RESIDENT_FIXED_LENGTH - 1u);
    assert(cb62_parse_resident_ready_response(wire, sizeof(wire), &response) ==
           CB62_RESIDENT_PARSE_WRONG_DATA_LENGTH);
}

static void test_crc32_reference_vector(void)
{
    static const uint8_t vector[] = "123456789";
    assert(cb62_resident_crc32(vector, sizeof(vector) - 1u) == 0xcbf43926u);
}

int main(void)
{
    test_fixed_resident_request();
    test_send_data_request();
    test_ready_response_requires_exact_length();
    test_crc32_reference_vector();
    puts("test_resident_protocol: PASS");
    return 0;
}
