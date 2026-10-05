#include "../src/addr_protocol.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_fixed_request(void)
{
    static const uint8_t expected[CB62_ADDR_INQUIRY_REQUEST_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x04, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    uint8_t actual[CB62_ADDR_INQUIRY_REQUEST_SIZE];
    memset(actual, 0xa5, sizeof(actual));
    cb62_build_addr_inquiry_request(actual);
    assert(memcmp(actual, expected, sizeof(expected)) == 0);
}

static void test_valid_response(void)
{
    static const uint8_t wire[CB62_ADDR_INQUIRY_RESPONSE_SIZE] = {
        0x50, 0x53, 0x52, 0x55, 0x00, 0x00, 0x00, 0x00,
        0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab, 0x90,
    };
    struct cb62_addr_inquiry_response response;
    assert(cb62_parse_addr_inquiry_response(wire, sizeof(wire), &response) ==
           CB62_ADDR_PARSE_OK);
    assert(response.signature == 0x55525350u);
    assert(response.status == 0u);
    assert(response.address == 0x12345678u);
    assert(response.p1 == 0x90abcdefu);
}

static void test_rejections(void)
{
    uint8_t wire[CB62_ADDR_INQUIRY_RESPONSE_SIZE] = {
        0x50, 0x53, 0x52, 0x55, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    struct cb62_addr_inquiry_response response;
    assert(cb62_parse_addr_inquiry_response(wire, sizeof(wire) - 1,
                                             &response) ==
           CB62_ADDR_PARSE_WRONG_LENGTH);
    wire[0] = 0;
    assert(cb62_parse_addr_inquiry_response(wire, sizeof(wire), &response) ==
           CB62_ADDR_PARSE_BAD_SIGNATURE);
    wire[0] = 0x50;
    wire[4] = 1;
    assert(cb62_parse_addr_inquiry_response(wire, sizeof(wire), &response) ==
           CB62_ADDR_PARSE_DEVICE_FAILURE);
}

int main(void)
{
    test_fixed_request();
    test_valid_response();
    test_rejections();
    puts("test_addr_protocol: PASS");
    return 0;
}
