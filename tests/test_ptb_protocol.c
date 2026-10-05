#include "../src/ptb_protocol.h"

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

static void test_fixed_ptb_request(void)
{
    static const uint8_t expected[CB62_PTB_COMMAND_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    uint8_t actual[CB62_PTB_COMMAND_SIZE];
    memset(actual, 0xa5, sizeof(actual));
    cb62_build_fixed_ptb_ready_request(actual);
    assert(memcmp(actual, expected, sizeof(expected)) == 0);
}

static void test_send_data_request(void)
{
    static const uint8_t expected[CB62_PTB_COMMAND_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x03, 0x00, 0x00, 0x00,
    };
    uint8_t actual[CB62_PTB_COMMAND_SIZE];
    memset(actual, 0xa5, sizeof(actual));
    cb62_build_ptb_send_data_request(actual);
    assert(memcmp(actual, expected, sizeof(expected)) == 0);
}

static void test_ready_response_requires_exact_length(void)
{
    uint8_t wire[CB62_PTB_RESPONSE_SIZE] = {0};
    struct cb62_ptb_ready_response response;
    put_u32le(wire + 0, CB62_PTB_STATUS_TOKEN);
    put_u32le(wire + 4, CB62_PTB_RESPONSE_SUCCESS);
    put_u32le(wire + 8, CB62_PTB_FIXED_LENGTH);
    put_u32le(wire + 12, 0x12345678u);
    assert(cb62_parse_ptb_ready_response(wire, sizeof(wire), &response) ==
           CB62_PTB_PARSE_OK);
    assert(response.length == 0x1000u);
    assert(response.crc32 == 0x12345678u);
    put_u32le(wire + 8, CB62_PTB_FIXED_LENGTH - 1u);
    assert(cb62_parse_ptb_ready_response(wire, sizeof(wire), &response) ==
           CB62_PTB_PARSE_WRONG_DATA_LENGTH);

    put_u32le(wire + 8, CB62_PTB_FIXED_LENGTH);
    assert(cb62_parse_ptb_ready_response(wire, sizeof(wire) - 1u, &response) ==
           CB62_PTB_PARSE_WRONG_RESPONSE_LENGTH);
    put_u32le(wire + 0, 0xdeadbeefu);
    assert(cb62_parse_ptb_ready_response(wire, sizeof(wire), &response) ==
           CB62_PTB_PARSE_BAD_SIGNATURE);
    put_u32le(wire + 0, CB62_PTB_STATUS_TOKEN);
    put_u32le(wire + 4, 1u);
    assert(cb62_parse_ptb_ready_response(wire, sizeof(wire), &response) ==
           CB62_PTB_PARSE_DEVICE_FAILURE);
}

static void test_crc32_reference_vector(void)
{
    static const uint8_t vector[] = "123456789";
    assert(cb62_ptb_crc32(vector, sizeof(vector) - 1u) == 0xcbf43926u);
}

static void test_record_magic_count_uses_fixed_stride(void)
{
    uint8_t data[CB62_PTB_FIXED_LENGTH] = {0};
    put_u32le(data + CB62_PTB_RECORD_MAGIC_OFFSET, CB62_PTB_RECORD_MAGIC);
    put_u32le(data + CB62_PTB_RECORD_MAGIC_OFFSET + CB62_PTB_RECORD_SIZE,
              CB62_PTB_RECORD_MAGIC);
    put_u32le(data + 7u, CB62_PTB_RECORD_MAGIC);
    assert(cb62_ptb_count_record_magics(data, sizeof(data)) == 2u);
}

int main(void)
{
    test_fixed_ptb_request();
    test_send_data_request();
    test_ready_response_requires_exact_length();
    test_crc32_reference_vector();
    test_record_magic_count_uses_fixed_stride();
    puts("test_ptb_protocol: PASS");
    return 0;
}
